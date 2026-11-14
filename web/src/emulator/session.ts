import { loadEmulatorModule, type EmulatorInstance, type EmulatorModule } from './wasm';
import type { SystemDef } from './systems';
import { GAMEPAD_MAPPINGS, readGamepadActions } from './gamepad';
import { LocalizedError } from '../lib/errors';

const TARGET_BUFFER_SECONDS = 0.08;
const MAX_STEPS_PER_TICK = 4;

/**
 * Nominal frame rate (Hz) of each system. Only used to pace the emulation when the audio
 * output cannot act as the clock (suspended or blocked audio context).
 */
const FRAME_RATES: Record<string, number> = {
  chip8: 60,
  atari2600: 59.92,
  gameboy: 59.7275,
  nes: 60.0988,
};
const DEFAULT_FRAME_RATE = 60;

/**
 * Longest wall-clock gap (ms) converted into emulated frames when pacing without audio,
 * so that a stalled or backgrounded tab does not fast-forward the game when it wakes up.
 */
const MAX_WALL_DELTA_MS = 100;

/**
 * A game "saves" by writing to its cartridge RAM, there is no save event. We
 * consider the save finished once the RAM has not changed for this many
 * emulated frames (about one second).
 */
const BATTERY_QUIET_FRAMES = 60;

/**
 * Compares two byte arrays.
 * @param a The first array.
 * @param b The second array.
 * @returns The number of positions where they differ (the length difference counts too).
 */
function countDifferences(a: Uint8Array, b: Uint8Array): number {
  let diff = Math.abs(a.length - b.length);
  const n = Math.min(a.length, b.length);

  for (let i = 0; i < n; i++) {
    if (a[i] !== b[i]) diff++;
  }

  return diff;
}

/**
 * Options for creating an emulator session.
 * @props canvas The HTML canvas element where the emulator will render its output.
 * @props system The system definition for the emulator (e.g., Game Boy, Atari 2600).
 * @props bindings A record mapping action IDs to keyboard key codes.
 * @props volume The initial volume level (0 to 100).
 */
export interface SessionOptions {
  /**
   * The HTML canvas element where the emulator will render its output.
   */
  canvas: HTMLCanvasElement;
  /**
   * The system definition for the emulator (e.g., Game Boy, Atari 2600).
   */
  system: SystemDef;
  /**
   * A record mapping action IDs to keyboard key codes.
   */
  bindings: Record<string, string>;
  /**
   * The initial volume level (0 to 100).
   */
  volume: number;
  /**
   * Called with the cartridge's battery RAM (a `.sav` image) after the game has
   * saved. Only used by cartridges that have a battery.
   */
  onBatterySave?: (data: Uint8Array) => void;
}

/**
 * Represents an emulator session that manages the state and behavior of an emulator instance.
 * It handles audio, video rendering, input controls, and save states.
 */
export class EmulatorSession {
  /** The emulator module. */
  private module!: EmulatorModule;
  /** The emulator instance. */
  private emu!: EmulatorInstance;
  /** The audio context for managing audio playback. */
  private audioCtx!: AudioContext;
  /** The audio worklet node for processing audio data. */
  private node!: AudioWorkletNode;
  /** The gain node for controlling audio volume. */
  private gain!: GainNode;
  /** The 2D rendering context for the canvas. */
  private ctx2d: CanvasRenderingContext2D;

  /** The requestAnimationFrame ID for the main loop. */
  private raf = 0;
  /** Indicates whether the emulator session is paused. */
  private paused = false;
  /** Indicates whether the emulator session has been destroyed. */
  private destroyed = false;
  /** Indicates whether the emulator instance has already been deleted. */
  private emuDeleted = false;
  /** Indicates whether the audio is muted. */
  private muted = false;
  /** The current volume level (0 to 100). */
  private volume: number;
  /** The number of audio frames queued for playback. */
  private queuedFrames = 0;
  /** What currently drives the emulation speed: the audio output or the wall clock. */
  private pacing: 'audio' | 'clock' = 'audio';
  /** Timestamp (ms) of the last wall-clock pacing update, or 0 if there is none yet. */
  private lastWallTime = 0;
  /** Wall-clock time (ms) owed to the emulator, spent in whole frames. */
  private wallDebt = 0;
  /** A set of currently pressed keyboard keys. */
  private pressed = new Set<string>();
  /** A set of currently pressed gamepad actions. */
  private padPressed = new Set<string>();

  /** A map of keyboard key codes to emulator action keys. */
  private keyMap = new Map<string, number>();

  /** True if the loaded cartridge has battery-backed RAM. */
  private hasBattery = false;
  /** True if the game wrote to its battery RAM and that was not handed over yet. */
  private batteryPending = false;
  /** Emulated frames since the last write to the battery RAM. */
  private batteryQuiet = 0;
  /** The battery RAM as last loaded or stored: a "save" is only a real change from it. */
  private batteryBaseline: Uint8Array | null = null;

  /**
   * Creates a new emulator session with the specified options.
   * @param opts The options for configuring the emulator session.
   * @throws {LocalizedError} If the canvas cannot provide a 2D rendering context.
   */
  constructor(private opts: SessionOptions) {
    const ctx = opts.canvas.getContext('2d');
    if (!ctx) {
      throw new LocalizedError(
        'canvasUnavailable',
        'The 2D canvas is not available in this browser.',
      );
    }

    this.ctx2d = ctx;
    this.volume = opts.volume;
    this.rebuildKeyMap(opts.system, opts.bindings);
  }

  /**
   * Starts the emulator session by loading the specified ROM and initializing audio and video rendering.
   *
   * If startup fails, everything created so far (audio context, worklet node, emulator
   * instance) is released before the error is rethrown. If {@link destroy} is called while
   * startup is still in progress, startup stops quietly and releases what it created.
   * @param rom  The ROM data to load into the emulator.
   * @param battery The previously stored in-game save (battery RAM) of this game, if any.
   * @throws Will throw an error if the ROM is invalid or not supported by the emulator.
   * @returns A promise that resolves when the emulator session has started successfully.
   */
  async start(rom: Uint8Array, battery: Uint8Array | null = null): Promise<void> {
    try {
      await this.init(rom, battery);
    } catch (err) {
      const destroyedMeanwhile = this.destroyed;

      // Release whatever was created before the failure, so that a refused ROM does not
      // leak the audio context or the emulator instance. Both calls are idempotent.
      this.destroy();
      this.releaseResources();

      // A failure caused by destroy() itself (e.g. the audio context was closed while the
      // worklet was loading) is expected: the caller asked for the session to go away.
      if (!destroyedMeanwhile) throw err;
    }
  }

  /**
   * Performs the asynchronous startup. After every `await`, the session may have been
   * destroyed in the meantime (fast navigation, React StrictMode double-mount...), in
   * which case nothing more must be created or registered.
   * @param rom  The ROM data to load into the emulator.
   * @param battery The previously stored in-game save (battery RAM) of this game, if any.
   */
  private async init(rom: Uint8Array, battery: Uint8Array | null): Promise<void> {
    this.audioCtx = new AudioContext({ latencyHint: 'interactive' });
    void this.audioCtx.resume();

    this.module = await loadEmulatorModule();
    if (this.destroyed) return;

    this.emu = new this.module.Emulator();

    // --- Audio ---
    await this.audioCtx.audioWorklet.addModule(`${import.meta.env.BASE_URL}emu-audio-worklet.js`);
    if (this.destroyed) return;

    this.node = new AudioWorkletNode(this.audioCtx, 'emu-audio', {
      outputChannelCount: [2],
    });

    this.node.port.onmessage = (e: MessageEvent<number>) => {
      this.queuedFrames = e.data;
    };

    this.gain = this.audioCtx.createGain();
    this.applyVolume();

    this.node.connect(this.gain).connect(this.audioCtx.destination);

    await Promise.race([
      this.audioCtx.resume(),
      new Promise<void>((resolve) => setTimeout(resolve, 1500)),
    ]);
    if (this.destroyed) return;

    // --- ROM ---
    this.emu.setSampleRate(this.audioCtx.sampleRate);

    if (!this.emu.load(this.opts.system.id, rom)) {
      throw new LocalizedError(
        'romRejected',
        'The emulator rejected this ROM (invalid or unsupported file).',
      );
    }

    // --- In-game save (battery RAM) ---
    this.hasBattery = this.emu.hasBattery();

    if (this.hasBattery && battery && !this.emu.loadRam(battery)) {
      console.warn('Stored in-game save ignored: it does not match this cartridge.');
    }

    this.batteryBaseline = this.hasBattery ? this.emu.saveRam() : null;

    window.addEventListener('keydown', this.onKeyDown);
    window.addEventListener('keyup', this.onKeyUp);
    window.addEventListener('blur', this.releaseAll);
    window.addEventListener('pagehide', this.flushBattery);
    document.addEventListener('visibilitychange', this.onVisibilityChange);

    this.raf = requestAnimationFrame(this.loop);
  }

  // ───────────── Controls ─────────────

  /**
   * Pauses the emulator session, suspending audio playback and releasing all input controls.
   */
  pause(): void {
    if (this.destroyed) return;

    this.paused = true;
    this.lastWallTime = 0;
    this.wallDebt = 0;
    this.flushBattery();
    void this.audioCtx?.suspend();
    this.releaseAll();
    this.releasePad();
  }

  /**
   * Resumes the emulator session, resuming audio playback and allowing input controls to be processed again.
   */
  resume(): void {
    if (this.destroyed) return;

    this.paused = false;
    void this.audioCtx?.resume();
  }

  /**
   * Returns whether the emulator session is currently paused.
   * @returns True if the emulator session is paused, false otherwise.
   */
  get isPaused(): boolean {
    return this.paused;
  }

  /**
   * Resets the emulator session, clearing the current state and returning to the initial state of the loaded ROM.
   */
  reset(): void {
    if (this.destroyed || !this.emu) return;

    this.emu.reset();
    this.queuedFrames = 0;
  }

  /**
   * Sets the volume level for the emulator session.
   * The volume level is clamped between 0 (muted) and 100 (maximum volume).
   * @param volume The desired volume level (0 to 100).
   */
  setVolume(volume: number): void {
    this.volume = Math.max(0, Math.min(100, volume));
    this.applyVolume();
  }

  /**
   * Sets whether the emulator session's audio is muted.
   * @param muted True to mute the audio, false to unmute.
   */
  setMuted(muted: boolean): void {
    this.muted = muted;
    this.applyVolume();
  }

  /**
   * Updates the key bindings for the emulator session.
   * @param bindings A record of key bindings to update.
   */
  updateBindings(bindings: Record<string, string>): void {
    this.rebuildKeyMap(this.opts.system, bindings);
  }

  /**
   * Sets the pressed state of a specific action in the emulator session.
   * @param actionId The ID of the action to set the pressed state for.
   * @param pressed True if the action is pressed, false if it is released.
   */
  setActionPressed(actionId: string, pressed: boolean): void {
    if (this.destroyed || !this.emu) return;
    if (pressed && this.paused) return;

    const action = this.opts.system.actions.find((a) => a.id === actionId);
    if (action) this.emu.setKey(action.key, pressed);
  }

  // ─────────── Save states ───────────

  /**
   * Saves the current state of the emulator session as a string.
   * @throws Will throw an error if the emulator session is not running or has been destroyed.
   * @returns A string representing the current state of the emulator session.
   */
  saveState(): string {
    if (this.destroyed || !this.emu) {
      throw new LocalizedError('sessionNotRunning', 'The emulator session is not running.');
    }

    return this.emu.saveState();
  }

  /**
   * Loads a previously saved state into the emulator session.
   * @throws Will throw an error if the emulator session is not running, has been destroyed, or if the save state is empty.
   * @param data The string representing the saved state to load into the emulator session.
   * @returns True if the save state was loaded successfully, false otherwise.
   */
  loadState(data: string): boolean {
    if (this.destroyed || !this.emu) {
      throw new LocalizedError('sessionNotRunning', 'The emulator session is not running.');
    }

    if (!data.trim()) {
      throw new LocalizedError('saveStateEmpty', 'The save state is empty.');
    }

    this.releaseAll();

    const success = this.emu.loadState(data);

    if (!success) {
      return false;
    }

    this.queuedFrames = 0;

    this.draw();

    return true;
  }

  /**
   * Destroys the emulator session, releasing all resources and stopping the main loop.
   * After calling this method, the emulator session cannot be used again.
   * It is recommended to call this method when the emulator session is no longer needed to free up resources.
   * Calling it more than once is safe.
   */
  destroy(): void {
    if (this.destroyed) return;

    this.flushBattery();
    this.destroyed = true;

    cancelAnimationFrame(this.raf);

    window.removeEventListener('keydown', this.onKeyDown);
    window.removeEventListener('keyup', this.onKeyUp);
    window.removeEventListener('blur', this.releaseAll);
    window.removeEventListener('pagehide', this.flushBattery);
    document.removeEventListener('visibilitychange', this.onVisibilityChange);

    this.releaseAll();
    this.releaseResources();
  }

  /**
   * Releases the audio graph and the emulator instance. Each resource is released at most
   * once, so this can safely be called again after {@link destroy}, which is what
   * {@link start} does to free anything created while the session was being destroyed.
   */
  private releaseResources(): void {
    this.node?.disconnect();

    if (this.audioCtx && this.audioCtx.state !== 'closed') {
      void this.audioCtx.close();
    }

    if (this.emu && !this.emuDeleted) {
      this.emuDeleted = true;
      this.emu.delete();
    }
  }

  // ───────────── Main loop ─────────────

  /**
   * The main loop of the emulator session, responsible for updating the emulator state, processing input, and rendering video and audio.
   * This method is called repeatedly using requestAnimationFrame to achieve smooth updates.
   * It checks if the emulator session is paused or destroyed, and if not, it polls for gamepad input, steps the emulator forward, and pushes audio data to the audio worklet.
   * The audio output is the clock while it is running; otherwise (suspended or blocked audio context) the wall clock takes over so that the game keeps running, silently.
   * The loop continues until the emulator session is destroyed or paused.
   * @param now The timestamp (ms) given by requestAnimationFrame.
   */
  private loop = (now: number): void => {
    if (this.destroyed || !this.module || !this.emu || !this.audioCtx || !this.node) {
      return;
    }

    if (!this.paused) {
      this.pollGamepad();
      this.updatePacing(now);

      const audioPaced = this.pacing === 'audio';
      const steps = audioPaced ? this.stepWithAudioClock() : this.stepWithWallClock(now);

      if (steps > 0) {
        this.draw();
      }
    }

    this.raf = requestAnimationFrame(this.loop);
  };

  /**
   * Chooses what drives the emulation speed: the audio output while the audio context is
   * running, the wall clock otherwise. Resets the bookkeeping of both clocks on a change,
   * because whatever the worklet last reported is stale after the switch.
   * @param now The timestamp (ms) given by requestAnimationFrame.
   */
  private updatePacing(now: number): void {
    const next = this.audioCtx.state === 'running' ? 'audio' : 'clock';
    if (next === this.pacing) return;

    this.pacing = next;
    this.queuedFrames = 0;
    this.lastWallTime = now;
    this.wallDebt = 0;
  }

  /**
   * Steps the emulator until the audio queue holds enough samples (the audio output drains
   * the queue in real time, which is what keeps the game at the right speed).
   * @returns The number of frames emulated.
   */
  private stepWithAudioClock(): number {
    const target = this.audioCtx.sampleRate * TARGET_BUFFER_SECONDS;
    let steps = 0;

    while (this.queuedFrames < target && steps < MAX_STEPS_PER_TICK) {
      this.stepFrame(true);
      steps++;
    }

    return steps;
  }

  /**
   * Steps the emulator according to the elapsed wall-clock time, at the nominal frame rate
   * of the system. Used when no audio is playing, since an audio queue that is never
   * drained would otherwise freeze the game after a few frames. No audio is produced.
   * @param now The timestamp (ms) given by requestAnimationFrame.
   * @returns The number of frames emulated.
   */
  private stepWithWallClock(now: number): number {
    const frameMs = 1000 / (FRAME_RATES[this.opts.system.id] ?? DEFAULT_FRAME_RATE);

    if (this.lastWallTime === 0) this.lastWallTime = now;
    this.wallDebt += Math.min(now - this.lastWallTime, MAX_WALL_DELTA_MS);
    this.lastWallTime = now;

    let steps = 0;

    while (this.wallDebt >= frameMs && steps < MAX_STEPS_PER_TICK) {
      this.stepFrame(false);
      this.wallDebt -= frameMs;
      steps++;
    }

    // Running behind: drop the backlog instead of trying to catch up forever.
    if (this.wallDebt >= frameMs) this.wallDebt = 0;

    return steps;
  }

  /**
   * Emulates exactly one frame.
   * @param withAudio Whether to hand the generated samples to the audio worklet. This must
   * be false when the audio output is not running, otherwise samples pile up unplayed.
   */
  private stepFrame(withAudio: boolean): void {
    this.emu.stepFrame();
    this.trackBattery();
    if (withAudio) this.pushAudio();
  }

  // ───────────── In-game save (battery RAM) ─────────────

  /**
   * Returns whether the loaded cartridge keeps an in-game save (battery RAM).
   * @returns True if in-game saves are persisted for this game.
   */
  get hasBatterySave(): boolean {
    return this.hasBattery;
  }

  /**
   * Watches the battery RAM after each emulated frame. Once the game has
   * written to it and then stopped for a moment, the save is considered done
   * and handed to the `onBatterySave` callback.
   */
  private trackBattery(): void {
    if (!this.hasBattery) return;

    if (this.emu.consumeRamDirty()) {
      this.batteryPending = true;
      this.batteryQuiet = 0;
    } else if (this.batteryPending) {
      this.batteryQuiet++;
    }

    if (this.batteryPending && this.batteryQuiet >= BATTERY_QUIET_FRAMES) {
      this.flushBattery();
    }
  }

  /**
   * Hands any unsaved battery RAM changes to the `onBatterySave` callback right
   * away (used when pausing, hiding the page or leaving the game).
   * Does nothing if the RAM is the same as the last stored save.
   */
  private flushBattery = (): void => {
    if (!this.hasBattery || !this.emu) return;

    // Catch a write from the very last frames, not yet noticed by trackBattery().
    if (this.emu.consumeRamDirty()) this.batteryPending = true;
    if (!this.batteryPending) return;

    this.batteryPending = false;
    this.batteryQuiet = 0;

    const ram = this.emu.saveRam();
    const changed = this.batteryBaseline ? countDifferences(ram, this.batteryBaseline) : ram.length;

    // The game wrote to its RAM but left it as it was (it wrote a value then put
    // it back, or rewrote the same data): that is not a save.
    if (changed === 0) return;

    console.debug(`[in-game save] ${changed} byte(s) changed`);

    this.batteryBaseline = ram;
    this.opts.onBatterySave?.(ram);
  };

  private onVisibilityChange = (): void => {
    if (document.visibilityState === 'hidden') this.flushBattery();
  };

  /**
   * Pushes audio data from the emulator to the audio worklet for playback.
   * This method copies the audio frames of the emulator's audio buffer out of the WebAssembly memory and transfers them to the audio worklet node for processing.
   * It checks if the emulator module and audio buffer are available, and if there are any audio frames to push.
   * The method is called during each iteration of the main loop to ensure that audio playback remains in sync with the emulator's state.
   */
  private pushAudio(): void {
    if (!this.module || !this.module.HEAPF32) return;

    const frames = this.emu.audioFrameCount();

    if (frames === 0) return;

    // audioPtr() is a byte address while HEAPF32 is indexed in 4-byte floats. slice() copies
    // the samples out of the WebAssembly memory with a single allocation, and the resulting
    // buffer can be transferred (not copied again) to the worklet.
    const start = this.emu.audioPtr() >>> 2;
    const samples = this.module.HEAPF32.slice(start, start + frames * 2);

    this.node.port.postMessage(samples, [samples.buffer]);
    this.queuedFrames += frames;
  }

  /**
   * Draws the current framebuffer of the emulator onto the canvas.
   * This method retrieves the pixel data from the emulator's framebuffer and updates the canvas with the new image.
   * It checks if the emulator module and framebuffer are available, and if the canvas size matches the emulator's output dimensions.
   */
  private draw(): void {
    if (!this.module || !this.module.HEAPU8) return;

    const w = this.emu.width();
    const h = this.emu.height();
    const canvas = this.opts.canvas;

    if (canvas.width !== w || canvas.height !== h) {
      canvas.width = w;
      canvas.height = h;
    }

    // View the framebuffer in place: ImageData wraps the array it is given instead of copying
    // it, so putImageData() performs the only copy of the frame. (The view is created and
    // consumed synchronously, so a growth of the WebAssembly memory cannot detach it.)
    const pixels = new Uint8ClampedArray(
      this.module.HEAPU8.buffer as ArrayBuffer,
      this.emu.framebufferPtr(),
      w * h * 4,
    );

    this.ctx2d.putImageData(new ImageData(pixels, w, h), 0, 0);
  }

  // ───────────── Keyboard ─────────────

  /**
   * Rebuilds the key map for the emulator session based on the provided system definition and key bindings.
   * This method clears the existing key map and populates it with new mappings based on the system's actions and the provided bindings.
   * Each action's ID is mapped to its corresponding key code, allowing the emulator session to respond to keyboard input.
   * @param system The system definition containing the actions to map.
   * @param bindings A record of key bindings mapping action IDs to keyboard key codes.
   */
  private rebuildKeyMap(system: SystemDef, bindings: Record<string, string>): void {
    this.keyMap.clear();

    for (const action of system.actions) {
      const code = bindings[action.id];

      if (code) {
        this.keyMap.set(code, action.key);
      }
    }
  }

  /**
   * Handles the keydown event for the emulator session, updating the pressed state of the corresponding action in the emulator.
   * If the key is not mapped to any action or if the emulator session is paused, the event is ignored.
   * The default behavior of the key event is prevented to avoid unwanted side effects in the browser.
   * A key press is also a user gesture, so it is used to start the audio output if the browser blocked it at startup.
   * @param e The keyboard event triggered by the keydown action.
   */
  private onKeyDown = (e: KeyboardEvent): void => {
    const key = this.keyMap.get(e.code);

    if (key === undefined) return;

    e.preventDefault();

    if (e.repeat || this.paused) return;

    this.pressed.add(e.code);
    this.emu.setKey(key, true);

    if (this.audioCtx.state === 'suspended') void this.audioCtx.resume();
  };

  /**
   * Handles the keyup event for the emulator session, updating the pressed state of the corresponding action in the emulator.
   * If the key is not mapped to any action, the event is ignored.
   * The default behavior of the key event is prevented to avoid unwanted side effects in the browser.
   * @param e The keyboard event triggered by the keyup action.
   */
  private onKeyUp = (e: KeyboardEvent): void => {
    const key = this.keyMap.get(e.code);

    if (key === undefined) return;

    e.preventDefault();

    this.pressed.delete(e.code);
    this.emu.setKey(key, false);
  };

  /**
   * Releases all currently pressed keys in the emulator session, resetting their state to unpressed.
   * This method is called when the emulator session loses focus or when the user switches to another application.
   * It ensures that no keys remain in a pressed state when the emulator session is not active.
   */
  private releaseAll = (): void => {
    if (!this.emu) {
      this.pressed.clear();
      return;
    }

    for (const code of this.pressed) {
      const key = this.keyMap.get(code);

      if (key !== undefined) {
        this.emu.setKey(key, false);
      }
    }

    this.pressed.clear();
  };

  // ───────────── Gamepad ─────────────

  /**
   * Polls the connected gamepads and updates the pressed state of actions in the emulator session based on the current gamepad input.
   * This method reads the actions from the gamepad using the defined mappings for the current system and compares them to the previously pressed actions.
   * If an action is newly pressed, it is set to pressed in the emulator; if an action is released, it is set to unpressed.
   * The method ensures that the emulator session accurately reflects the current state of gamepad input.
   * It is called during each iteration of the main loop to continuously monitor gamepad input.
   */
  private pollGamepad(): void {
    const next = readGamepadActions(GAMEPAD_MAPPINGS[this.opts.system.id]);

    for (const id of next) {
      if (!this.padPressed.has(id)) this.setActionPressed(id, true);
    }
    for (const id of this.padPressed) {
      if (!next.has(id)) this.setActionPressed(id, false);
    }

    this.padPressed = next;
  }

  /**
   * Releases all currently pressed gamepad actions in the emulator session, resetting their state to unpressed.
   * This method is called when the emulator session loses focus or when the user switches to another application.
   * It ensures that no gamepad actions remain in a pressed state when the emulator session is not active.
   */
  private releasePad(): void {
    if (this.emu) {
      for (const id of this.padPressed) {
        const action = this.opts.system.actions.find((a) => a.id === id);
        if (action) this.emu.setKey(action.key, false);
      }
    }
    this.padPressed.clear();
  }

  /**
   * Applies the current volume and mute settings to the audio gain node.
   * If the emulator session is muted, the gain value is set to 0; otherwise, it is set based on the current volume level (0 to 100).
   * This method ensures that the audio output of the emulator session reflects the user's desired volume and mute settings.
   * It is called whenever the volume or mute state is changed to update the audio output accordingly.
   */
  private applyVolume(): void {
    if (this.gain) {
      this.gain.gain.value = this.muted ? 0 : this.volume / 100;
    }
  }
}
