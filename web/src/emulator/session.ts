import { loadEmulatorModule, type EmulatorInstance, type EmulatorModule } from './wasm';
import type { SystemDef } from './systems';
import { GAMEPAD_MAPPINGS, readGamepadActions } from './gamepad';

const TARGET_BUFFER_SECONDS = 0.08;
const MAX_STEPS_PER_TICK = 4;

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
  /** The image data used for rendering the emulator's framebuffer. */
  private image: ImageData | null = null;

  /** The requestAnimationFrame ID for the main loop. */
  private raf = 0;
  /** Indicates whether the emulator session is paused. */
  private paused = false;
  /** Indicates whether the emulator session has been destroyed. */
  private destroyed = false;
  /** Indicates whether the audio is muted. */
  private muted = false;
  /** The current volume level (0 to 100). */
  private volume: number;
  /** The number of audio frames queued for playback. */
  private queuedFrames = 0;
  /** A set of currently pressed keyboard keys. */
  private pressed = new Set<string>();
  /** A set of currently pressed gamepad actions. */
  private padPressed = new Set<string>();

  /** A map of keyboard key codes to emulator action keys. */
  private keyMap = new Map<string, number>();

  /**
   * Creates a new emulator session with the specified options.
   * @param opts The options for configuring the emulator session.
   */
  constructor(private opts: SessionOptions) {
    const ctx = opts.canvas.getContext('2d');
    if (!ctx) throw new Error('Canvas 2D indisponible');

    this.ctx2d = ctx;
    this.volume = opts.volume;
    this.rebuildKeyMap(opts.system, opts.bindings);
  }

  /**
   * Starts the emulator session by loading the specified ROM and initializing audio and video rendering.
   * @param rom  The ROM data to load into the emulator.
   * @throws Will throw an error if the ROM is invalid or not supported by the emulator.
   * @returns A promise that resolves when the emulator session has started successfully.
   */
  async start(rom: Uint8Array): Promise<void> {
    this.audioCtx = new AudioContext({ latencyHint: 'interactive' });
    void this.audioCtx.resume();

    this.module = await loadEmulatorModule();
    this.emu = new this.module.Emulator();

    // --- Audio ---
    await this.audioCtx.audioWorklet.addModule(`${import.meta.env.BASE_URL}emu-audio-worklet.js`);

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

    // --- ROM ---
    this.emu.setSampleRate(this.audioCtx.sampleRate);

    if (!this.emu.load(this.opts.system.id, rom)) {
      throw new Error("ROM refusée par l'émulateur (fichier invalide ou non supporté).");
    }

    window.addEventListener('keydown', this.onKeyDown);
    window.addEventListener('keyup', this.onKeyUp);
    window.addEventListener('blur', this.releaseAll);

    this.raf = requestAnimationFrame(this.loop);
  }

  // ───────────── Controls ─────────────

  /**
   * Pauses the emulator session, suspending audio playback and releasing all input controls.
   */
  pause(): void {
    if (this.destroyed) return;

    this.paused = true;
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

  // ───────────── Save states ─────────────

  /**
   * Saves the current state of the emulator session as a string.
   * @throws Will throw an error if the emulator session is not running or has been destroyed.
   * @returns A string representing the current state of the emulator session.
   */
  saveState(): string {
    if (this.destroyed || !this.emu) {
      throw new Error('Emulator session is not running.');
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
      throw new Error('Emulator session is not running.');
    }

    if (!data.trim()) {
      throw new Error('Save state is empty.');
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
   */
  destroy(): void {
    this.destroyed = true;

    cancelAnimationFrame(this.raf);

    window.removeEventListener('keydown', this.onKeyDown);
    window.removeEventListener('keyup', this.onKeyUp);
    window.removeEventListener('blur', this.releaseAll);

    this.releaseAll();

    this.node?.disconnect();
    void this.audioCtx?.close();

    this.emu?.delete();
  }

  // ───────────── Main loop ─────────────

  /**
   * The main loop of the emulator session, responsible for updating the emulator state, processing input, and rendering video and audio.
   * This method is called repeatedly using requestAnimationFrame to achieve smooth updates.
   * It checks if the emulator session is paused or destroyed, and if not, it polls for gamepad input, steps the emulator forward, and pushes audio data to the audio worklet.
   * The loop continues until the emulator session is destroyed or paused.
   */
  private loop = (): void => {
    if (this.destroyed || !this.module || !this.emu || !this.audioCtx || !this.node) {
      return;
    }

    if (!this.paused) {
      this.pollGamepad();

      const target = this.audioCtx.sampleRate * TARGET_BUFFER_SECONDS;
      let steps = 0;

      while (this.queuedFrames < target && steps < MAX_STEPS_PER_TICK) {
        this.emu.stepFrame();
        this.pushAudio();
        steps++;
      }

      if (steps > 0) {
        this.draw();
      }
    }

    this.raf = requestAnimationFrame(this.loop);
  };

  /**
   * Pushes audio data from the emulator to the audio worklet for playback.
   * This method retrieves the audio frames from the emulator's audio buffer and sends them to the audio worklet node for processing.
   * It checks if the emulator module and audio buffer are available, and if there are any audio frames to push.
   * The method is called during each iteration of the main loop to ensure that audio playback remains in sync with the emulator's state.
   */
  private pushAudio(): void {
    if (!this.module || !this.module.HEAPF32) return;

    const frames = this.emu.audioFrameCount();

    if (frames === 0) return;

    const src = new Float32Array(this.module.HEAPF32.buffer, this.emu.audioPtr(), frames * 2);

    const copy = new Float32Array(src);

    this.node.port.postMessage(copy, [copy.buffer]);
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

    if (!this.image || this.image.width !== w || this.image.height !== h) {
      this.image = this.ctx2d.createImageData(w, h);
    }

    const pixels = new Uint8Array(this.module.HEAPU8.buffer, this.emu.framebufferPtr(), w * h * 4);

    this.image.data.set(pixels);
    this.ctx2d.putImageData(this.image, 0, 0);
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
   * @param e The keyboard event triggered by the keydown action.
   */
  private onKeyDown = (e: KeyboardEvent): void => {
    const key = this.keyMap.get(e.code);

    if (key === undefined) return;

    e.preventDefault();

    if (e.repeat || this.paused) return;

    this.pressed.add(e.code);
    this.emu.setKey(key, true);
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
