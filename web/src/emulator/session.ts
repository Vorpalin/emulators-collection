import { loadEmulatorModule, type EmulatorInstance, type EmulatorModule } from './wasm';
import type { SystemDef } from './systems';

/** Audio gardé d'avance dans le worklet (compromis latence / sécurité). */
const TARGET_BUFFER_SECONDS = 0.08;
/** Plafond de frames émulées par rafraîchissement d'écran (évite la spirale). */
const MAX_STEPS_PER_TICK = 4;

export interface SessionOptions {
  canvas: HTMLCanvasElement;
  system: SystemDef;
  /** action.id -> KeyboardEvent.code (même forme que SystemDef.defaultBindings). */
  bindings: Record<string, string>;
  volume: number; // 0..100
}

/**
 * Pilote un cœur WebAssembly : charge la ROM, fait tourner l'émulation,
 * dessine dans le canvas, envoie le son à un AudioWorklet et relaie le clavier.
 *
 * Principe de synchronisation : on n'émule une frame que si le worklet
 * a moins de ~80 ms d'audio en attente. La vitesse du jeu suit donc
 * l'horloge de la carte son, quel que soit le taux de rafraîchissement
 * de l'écran (60, 120, 144 Hz...).
 *
 * À créer depuis un geste utilisateur (clic) à cause de la politique
 * d'autoplay des navigateurs.
 */
export class EmulatorSession {
  private module!: EmulatorModule;
  private emu!: EmulatorInstance;
  private audioCtx!: AudioContext;
  private node!: AudioWorkletNode;
  private gain!: GainNode;
  private ctx2d: CanvasRenderingContext2D;
  private image: ImageData | null = null;

  private raf = 0;
  private paused = false;
  private destroyed = false;
  private muted = false;
  private volume: number;
  private queuedFrames = 0; // audio en attente dans le worklet (en frames)
  private pressed = new Set<string>(); // KeyboardEvent.code actuellement enfoncés

  /** code clavier -> indice de touche du cœur */
  private keyMap = new Map<string, number>();

  constructor(private opts: SessionOptions) {
    const ctx = opts.canvas.getContext('2d');
    if (!ctx) throw new Error('Canvas 2D indisponible');
    this.ctx2d = ctx;
    this.volume = opts.volume;
    this.rebuildKeyMap(opts.system, opts.bindings);
  }

  async start(rom: Uint8Array): Promise<void> {
    this.module = await loadEmulatorModule();
    this.emu = new this.module.Emulator();

    // --- Audio ---
    this.audioCtx = new AudioContext({ latencyHint: 'interactive' });
    await this.audioCtx.audioWorklet.addModule(`${import.meta.env.BASE_URL}emu-audio-worklet.js`);
    this.node = new AudioWorkletNode(this.audioCtx, 'emu-audio', { outputChannelCount: [2] });
    this.node.port.onmessage = (e: MessageEvent<number>) => {
      this.queuedFrames = e.data;
    };
    this.gain = this.audioCtx.createGain();
    this.applyVolume();
    this.node.connect(this.gain).connect(this.audioCtx.destination);
    await this.audioCtx.resume();

    // --- ROM ---
    this.emu.setSampleRate(this.audioCtx.sampleRate);
    if (!this.emu.load(this.opts.system.id, rom)) {
      throw new Error('ROM refusée par l\'émulateur (fichier invalide ou non supporté).');
    }

    window.addEventListener('keydown', this.onKeyDown);
    window.addEventListener('keyup', this.onKeyUp);
    window.addEventListener('blur', this.releaseAll);

    this.raf = requestAnimationFrame(this.loop);
  }

  // ───────────── Contrôles ─────────────

  pause(): void {
    this.paused = true;
    void this.audioCtx.suspend();
    this.releaseAll();
  }

  resume(): void {
    this.paused = false;
    void this.audioCtx.resume();
  }

  get isPaused(): boolean {
    return this.paused;
  }

  reset(): void {
    this.emu.reset();
    this.queuedFrames = 0;
  }

  setVolume(volume: number): void {
    this.volume = volume;
    this.applyVolume();
  }

  setMuted(muted: boolean): void {
    this.muted = muted;
    this.applyVolume();
  }

  updateBindings(bindings: Record<string, string>): void {
    this.rebuildKeyMap(this.opts.system, bindings);
  }

  destroy(): void {
    this.destroyed = true;
    cancelAnimationFrame(this.raf);
    window.removeEventListener('keydown', this.onKeyDown);
    window.removeEventListener('keyup', this.onKeyUp);
    window.removeEventListener('blur', this.releaseAll);
    this.node?.disconnect();
    void this.audioCtx?.close();
    this.emu?.delete();
  }

  // ───────────── Boucle principale ─────────────

  private loop = (): void => {
    if (this.destroyed) return;

    if (!this.paused) {
      const target = this.audioCtx.sampleRate * TARGET_BUFFER_SECONDS;
      let steps = 0;
      while (this.queuedFrames < target && steps < MAX_STEPS_PER_TICK) {
        this.emu.stepFrame();
        this.pushAudio();
        steps++;
      }
      if (steps > 0) this.draw();
    }

    this.raf = requestAnimationFrame(this.loop);
  };

  private pushAudio(): void {
    const frames = this.emu.audioFrameCount();
    if (frames === 0) return;
    // Copie : la mémoire WASM peut être réallouée, et on transfère le buffer.
    const src = new Float32Array(this.module.HEAPF32.buffer, this.emu.audioPtr(), frames * 2);
    const copy = new Float32Array(src);
    this.node.port.postMessage(copy, [copy.buffer]);
    this.queuedFrames += frames;
  }

  private draw(): void {
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

  // ───────────── Clavier ─────────────

  private rebuildKeyMap(system: SystemDef, bindings: Record<string, string>): void {
    this.keyMap.clear();
    for (const action of system.actions) {
      const code = bindings[action.id];
      if (code) this.keyMap.set(code, action.key);
    }
  }

  private onKeyDown = (e: KeyboardEvent): void => {
    const key = this.keyMap.get(e.code);
    if (key === undefined) return;
    e.preventDefault(); // évite le scroll (flèches/espace) et le focus (Tab)
    if (e.repeat || this.paused) return;
    this.pressed.add(e.code);
    this.emu.setKey(key, true);
  };

  private onKeyUp = (e: KeyboardEvent): void => {
    const key = this.keyMap.get(e.code);
    if (key === undefined) return;
    e.preventDefault();
    this.pressed.delete(e.code);
    this.emu.setKey(key, false);
  };

  /** Relâche toutes les touches (perte de focus, pause). */
  private releaseAll = (): void => {
    for (const code of this.pressed) {
      const key = this.keyMap.get(code);
      if (key !== undefined) this.emu.setKey(key, false);
    }
    this.pressed.clear();
  };

  private applyVolume(): void {
    if (this.gain) this.gain.gain.value = this.muted ? 0 : this.volume / 100;
  }
}
