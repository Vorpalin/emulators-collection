export interface EmulatorInstance {
  load(type: string, rom: Uint8Array): boolean;
  setSampleRate(hz: number): void;
  reset(): void;
  stepFrame(): void;
  setKey(key: number, pressed: boolean): void;
  isHalted(): boolean;
  width(): number;
  height(): number;
  framebufferPtr(): number;
  audioFrameCount(): number;
  audioPtr(): number;
  delete(): void;
}

export interface EmulatorModule {
  Emulator: new () => EmulatorInstance;
  HEAPU8: Uint8Array;
  HEAPF32: Float32Array;
}

let modulePromise: Promise<EmulatorModule> | null = null;

export function loadEmulatorModule(): Promise<EmulatorModule> {
  if (!modulePromise) {
    const url = `${import.meta.env.BASE_URL}wasm/emulators.js`;
    modulePromise = import(/* @vite-ignore */ url)
      .then((m) => m.default() as Promise<EmulatorModule>)
      .catch((err) => {
        modulePromise = null;
        throw new Error(
          `Impossible to load the WebAssembly module (${url}).`
        );
      });
  }
  return modulePromise;
}
