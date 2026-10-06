import { useRegisterSW } from 'virtual:pwa-register/react';

const CHECK_EVERY_MS = 60 * 60 * 1000;

function isInstalledPWA(): boolean {
  return (
    window.matchMedia('(display-mode: standalone)').matches ||
    ('standalone' in navigator &&
      (navigator as Navigator & { standalone?: boolean }).standalone === true)
  );
}

export default function UpdatePrompt() {
  const {
    needRefresh: [needRefresh, setNeedRefresh],
    offlineReady: [offlineReady, setOfflineReady],
    updateServiceWorker,
  } = useRegisterSW({
    onRegisteredSW(_url, registration) {
      if (registration) {
        setInterval(() => void registration.update(), CHECK_EVERY_MS);
      }
    },
  });

  const installedPWA = isInstalledPWA();

  if (!needRefresh && !(offlineReady && installedPWA)) {
    return null;
  }

  const close = () => {
    setNeedRefresh(false);
    setOfflineReady(false);
  };

  return (
    <div
      className="fixed inset-x-0 bottom-0 z-50 flex justify-center px-4 pointer-events-none"
      style={{
        paddingBottom: 'calc(env(safe-area-inset-bottom, 0px) + 1rem)',
      }}
    >
      <div
        role="status"
        className="pointer-events-auto flex items-center gap-3 bg-slate-900 border border-slate-700 text-slate-200 text-sm rounded-xl shadow-2xl px-4 py-3 max-w-sm w-full"
      >
        <span className="flex-1">
          {needRefresh
            ? 'A new version is available.'
            : 'Emulators Collection is ready to work offline.'}
        </span>

        {needRefresh && (
          <button
            onClick={() => void updateServiceWorker(true)}
            className="bg-cyan-500 hover:bg-cyan-400 text-slate-950 font-semibold px-3 py-1.5 rounded-lg transition"
          >
            Reload
          </button>
        )}

        <button
          onClick={close}
          className="text-slate-400 hover:text-slate-200 px-2 py-1.5 rounded-lg transition"
          aria-label="Dismiss"
        >
          {needRefresh ? 'Later' : 'OK'}
        </button>
      </div>
    </div>
  );
}
