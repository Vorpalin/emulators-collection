import { VitePWA } from 'vite-plugin-pwa';

/**
 * Configuration PWA. À utiliser dans vite.config.ts :
 *
 *   import { pwaPlugin } from './vite-pwa';
 *   export default defineConfig({ plugins: [react(), pwaPlugin()] });
 *
 * Le service worker n'est actif que dans le build de production
 * (`npm run build && npm run preview`), pas avec `npm run dev`.
 */
export function pwaPlugin() {
  return VitePWA({
    // 'prompt' : une nouvelle version n'est appliquée que quand l'utilisateur accepte
    // (voir UpdatePrompt.tsx), pour ne jamais recharger la page en pleine partie.
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
      // Met en cache l'application et le module WebAssembly de l'émulateur
      globPatterns: ['**/*.{js,css,html,ico,png,svg,wasm,woff2}'],
      maximumFileSizeToCacheInBytes: 8 * 1024 * 1024,
      cleanupOutdatedCaches: true,
      // Navigation SPA : /reset-password, /controls... renvoient index.html hors ligne
      navigateFallback: 'index.html',
      // Aucun cache d'exécution (runtimeCaching) volontairement : les requêtes vers
      // Supabase (authentification, jeux, ROMs, réglages) passent toujours par le réseau.
    },
  });
}
