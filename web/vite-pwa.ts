import { VitePWA } from 'vite-plugin-pwa';

/**
 * Configure the Vite PWA plugin for the Emulators Collection web application. This function sets up the PWA manifest, including app name, description, theme colors, icons, and workbox settings for caching assets.
 * It ensures that the application can be installed as a Progressive Web App and provides offline capabilities.
 * @returns A configured VitePWA plugin instance for use in the Vite build process.
 */
export function pwaPlugin() {
  return VitePWA({
    registerType: 'prompt',

    includeAssets: ['icons/apple-touch-icon.png'],

    manifest: {
      name: 'Emulators Collection',
      short_name: 'Emulators',
      description: 'Play your CHIP-8, Atari 2600 and Game Boy ROMs right in your browser.',
      theme_color: '#020617',
      background_color: '#020617',
      display: 'standalone',
      orientation: 'any',
      categories: ['games', 'entertainment'],
      icons: [
        { src: 'icons/pwa-192x192.png', sizes: '192x192', type: 'image/png' },
        { src: 'icons/pwa-512x512.png', sizes: '512x512', type: 'image/png' },
        {
          src: 'icons/pwa-maskable-512x512.png',
          sizes: '512x512',
          type: 'image/png',
          purpose: 'maskable',
        },
      ],
    },

    workbox: {
      globPatterns: ['**/*.{js,css,html,ico,png,svg,wasm,woff2}'],
      maximumFileSizeToCacheInBytes: 8 * 1024 * 1024,
      cleanupOutdatedCaches: true,
      navigateFallback: 'index.html',
    },
  });
}
