import { useEffect, useRef, useState } from 'react';
import { NavLink, Outlet, Link } from 'react-router-dom';
import { Cpu, FolderOpen, Gamepad2, LogOut, Menu, X, Github, ChevronRight } from 'lucide-react';
import { useAuth } from '../auth/AuthProvider';
import { useProfile } from '../hooks/useProfile';
import { useTheme } from '../theme/ThemeProvider';
import { useI18n } from '../i18n/I18nProvider';

const tabClass = ({ isActive }: { isActive: boolean }) =>
  `flex items-center gap-2 rounded-xl px-3.5 py-2 text-sm font-medium transition-all focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-cyan-400 ${
    isActive
      ? 'bg-cyan-400/10 text-cyan-300 ring-1 ring-cyan-400/20'
      : 'text-slate-400 hover:bg-slate-800/70 hover:text-slate-100'
  }`;

export default function Layout() {
  const { user, signOut } = useAuth();
  const { username } = useProfile();
  const { theme, setTheme } = useTheme();
  const { language, setLanguage, t } = useI18n();
  const [mobileMenuOpen, setMobileMenuOpen] = useState(false);
  const menuButtonRef = useRef<HTMLButtonElement>(null);

  useEffect(() => {
    if (!mobileMenuOpen) return;

    const onKeyDown = (event: KeyboardEvent) => {
      if (event.key !== 'Escape') return;

      setMobileMenuOpen(false);
      menuButtonRef.current?.focus();
    };

    window.addEventListener('keydown', onKeyDown);
    return () => window.removeEventListener('keydown', onKeyDown);
  }, [mobileMenuOpen]);

  const navigation = (
    <>
      <NavLink to="/" end className={tabClass} onClick={() => setMobileMenuOpen(false)}>
        <FolderOpen className="h-4 w-4" /> {t('library')}
      </NavLink>
      <NavLink to="/controls" className={tabClass} onClick={() => setMobileMenuOpen(false)}>
        <Gamepad2 className="h-4 w-4" /> {t('controls')}
      </NavLink>
    </>
  );

  return (
    <div className="flex min-h-screen flex-col bg-slate-950 font-sans text-slate-100 selection:bg-cyan-400 selection:text-slate-950">
      <header className="sticky top-0 z-50 border-b border-slate-800/80 bg-slate-950/85 px-4 backdrop-blur-xl sm:px-6">
        <div className="mx-auto flex min-h-[72px] max-w-7xl items-center justify-between gap-4">
          <Link
            to="/"
            className="group flex min-w-0 items-center gap-3"
            aria-label="Emulators Collection home"
          >
            <div className="rounded-xl bg-gradient-to-br from-cyan-400 to-violet-500 p-2.5 shadow-lg shadow-cyan-500/10 transition group-hover:rotate-3 group-hover:scale-105">
              <Cpu className="h-5 w-5 text-slate-950" />
            </div>
            <div className="min-w-0">
              <span className="block truncate text-sm font-extrabold tracking-[0.12em] text-white sm:text-base">
                EMULATORS <span className="text-cyan-300">COLLECTION</span>
              </span>
              <span className="hidden text-[10px] font-medium uppercase tracking-[0.22em] text-slate-500 sm:block">
                {t('playClassics')}
              </span>
            </div>
          </Link>

          <nav
            className="hidden items-center gap-1 rounded-2xl border border-slate-800/80 bg-slate-900/50 p-1 md:flex"
            aria-label={t('mainNavigation')}
          >
            {navigation}
          </nav>

          <div className="hidden items-center gap-3 md:flex">
            <label className="sr-only" htmlFor="theme-preference">Color theme</label>
            <select
              id="theme-preference"
              value={theme}
              onChange={(event) => setTheme(event.target.value as typeof theme)}
              className="rounded-lg border border-slate-800 bg-slate-900 px-2 py-2 text-xs font-medium text-slate-300 outline-none transition hover:border-slate-600 focus-visible:ring-2 focus-visible:ring-cyan-400"
            >
              <option value="system">{t('systemTheme')}</option>
              <option value="light">{t('lightTheme')}</option>
              <option value="dark">{t('darkTheme')}</option>
            </select>
            <a
              href="https://github.com/Vorpalin/emulators-collection"
              target="_blank"
              rel="noopener noreferrer"
              aria-label={t('github')}
              className="rounded-lg p-2 text-slate-500 transition hover:bg-slate-800 hover:text-white"
            >
              <Github className="h-4 w-4" />
            </a>
            <label className="sr-only" htmlFor="language-preference">{t('language')}</label>
            <select id="language-preference" value={language} onChange={(event) => setLanguage(event.target.value as 'en' | 'fr')} className="rounded-lg border border-slate-800 bg-slate-900 px-2 py-2 text-xs font-medium text-slate-300 outline-none focus-visible:ring-2 focus-visible:ring-cyan-400">
              <option value="en">{t('english')}</option>
              <option value="fr">{t('french')}</option>
            </select>
            <div className="h-6 w-px bg-slate-800" />
            <div
              className="max-w-[180px] truncate text-xs text-slate-400"
              title={username ?? user?.email ?? undefined}
            >
              {username ?? user?.email}
            </div>
            <button
              type="button"
              onClick={() => void signOut()}
              className="inline-flex items-center gap-2 rounded-xl border border-slate-800 bg-slate-900 px-3 py-2 text-xs font-semibold text-slate-300 transition hover:border-rose-500/30 hover:bg-rose-500/10 hover:text-rose-300 focus-visible:outline-none focus-visible:ring-2 focus-visible:ring-cyan-400"
            >
              <LogOut className="h-3.5 w-3.5" /> {t('signOut')}
            </button>
          </div>

          <button
            ref={menuButtonRef}
            type="button"
            className="rounded-xl border border-slate-800 bg-slate-900 p-2 text-slate-300 md:hidden"
            aria-label={mobileMenuOpen ? 'Close menu' : 'Open menu'}
            aria-expanded={mobileMenuOpen}
            aria-controls="mobile-navigation"
            onClick={() => setMobileMenuOpen((open) => !open)}
          >
            {mobileMenuOpen ? <X className="h-5 w-5" /> : <Menu className="h-5 w-5" />}
          </button>
        </div>

        {mobileMenuOpen && (
          <div id="mobile-navigation" className="border-t border-slate-800 py-3 md:hidden">
            <nav className="flex flex-col gap-1" aria-label={t('mobileNavigation')}>
              {navigation}
            </nav>
            <div className="mt-3 flex items-center justify-between gap-3 border-t border-slate-800 pt-3">
              <label className="sr-only" htmlFor="mobile-theme-preference">Color theme</label>
              <select
                id="mobile-theme-preference"
                value={theme}
                onChange={(event) => setTheme(event.target.value as typeof theme)}
                className="rounded-lg border border-slate-800 bg-slate-900 px-2 py-2 text-xs font-medium text-slate-300 outline-none focus-visible:ring-2 focus-visible:ring-cyan-400"
              >
                <option value="system">{t('systemTheme')}</option>
                <option value="light">{t('lightTheme')}</option>
                <option value="dark">{t('darkTheme')}</option>
              </select>
              <span className="max-w-[70%] truncate text-xs text-slate-500">
                {username ?? user?.email}
              </span>
              <label className="sr-only" htmlFor="mobile-language-preference">{t('language')}</label>
              <select id="mobile-language-preference" value={language} onChange={(event) => setLanguage(event.target.value as 'en' | 'fr')} className="rounded-lg border border-slate-800 bg-slate-900 px-2 py-2 text-xs text-slate-300">
                <option value="en">{t('english')}</option>
                <option value="fr">{t('french')}</option>
              </select>
              <button
                type="button"
                onClick={() => void signOut()}
                className="inline-flex items-center gap-2 rounded-lg px-3 py-2 text-xs text-slate-400 hover:bg-rose-500/10 hover:text-rose-300"
              >
                <LogOut className="h-3.5 w-3.5" /> Sign out
              </button>
            </div>
          </div>
        )}
      </header>

      <main className="mx-auto w-full max-w-7xl flex-1 px-4 py-6 sm:px-6 sm:py-8">
        <Outlet />
      </main>

      <footer className="border-t border-slate-800/70 bg-slate-950 px-4 py-5 sm:px-6">
        <div className="mx-auto flex max-w-7xl flex-col items-center justify-between gap-2 text-xs text-slate-600 sm:flex-row">
          <span>© {new Date().getFullYear()} Emulators Collection</span>
          <span className="flex items-center gap-1.5">
            Built for retro gaming <ChevronRight className="h-3 w-3 text-cyan-500" /> React ·
            WebAssembly
          </span>
        </div>
      </footer>
    </div>
  );
}
