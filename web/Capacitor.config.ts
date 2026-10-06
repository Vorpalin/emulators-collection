import type { CapacitorConfig } from '@capacitor/cli';

/**
 * Capacitor configuration for the Emulators Collection web application. This configuration defines the app's ID, name, web directory, and background color for the native wrapper.
 * It is used by Capacitor to build and run the application on native platforms (iOS, Android) while maintaining a consistent web experience.
 * @type {CapacitorConfig}
 */
const config: CapacitorConfig = {
  appId: 'io.github.vorpalin.emulatorscollection',
  appName: 'Emulators Collection',
  webDir: 'dist',
  backgroundColor: '#020617',
};

export default config;
