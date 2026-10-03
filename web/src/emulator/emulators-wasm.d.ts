declare module '*/wasm/emulators.js' {
  const createEmulatorModule: () => Promise<unknown>;
  export default createEmulatorModule;
}
