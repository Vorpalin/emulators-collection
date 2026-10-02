/**
 * Types et chargeur du module WebAssembly produit par Emscripten
 * (voir src/wasm/bindings.cc et web/scripts/build-wasm.sh).
 */

/** Instance de la classe `Emulator` exposée par Embind. */
export interface EmulatorInstance {
  load(type: string, rom: Uint8Array): boolean;
  setSampleRate(hz: number): void;
  reset(): void;
  stepFrame(): void;
  setKey(key: number, pressed: boolean): void;
  isHalted(): boolean;
  width(): number;
  height(): number;
  framebufferPtr(): number; // pointeur RGBA dans la mémoire WASM
  audioFrameCount(): number;
  audioPtr(): number; // pointeur float32 stéréo entrelacé
  delete(): void; // libération Embind
}

export interface EmulatorModule {
  Emulator: new () => EmulatorInstance;
  HEAPU8: Uint8Array;
  HEAPF32: Float32Array;
}

let modulePromise: Promise<EmulatorModule> | null = null;

/**
 * Charge (une seule fois) public/wasm/emulators.js. L'import est dynamique et
 * résolu à l'exécution : le site se compile même si le WASM n'a pas encore
 * été généré, l'erreur n'apparaît qu'à l'ouverture d'un jeu.
 */
export function loadEmulatorModule(): Promise<EmulatorModule> {
  if (!modulePromise) {
    const url = `${import.meta.env.BASE_URL}wasm/emulators.js`;
    modulePromise = import(/* @vite-ignore */ url)
      .then((m) => m.default() as Promise<EmulatorModule>)
      .catch((err) => {
        modulePromise = null; // permet de réessayer
        throw new Error(
          `Impossible de charger le module WebAssembly (${url}). ` +
            `Avez-vous lancé scripts/build-wasm.sh ? (${err})`,
        );
      });
  }
  return modulePromise;
}
