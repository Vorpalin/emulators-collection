import { NavLink, Outlet, Link } from 'react-router-dom';
import { Cpu, FolderOpen, Gamepad2, LogOut } from 'lucide-react';
import { useAuth } from '../auth/AuthProvider';
import { useProfile } from '../hooks/useProfile';

const tabClass = ({ isActive }: { isActive: boolean }) =>
  `flex items-center space-x-2 px-3 py-1.5 rounded-lg text-sm font-medium transition ${
    isActive
      ? 'bg-slate-800 text-cyan-400 shadow-sm border border-slate-700'
      : 'text-slate-400 hover:text-slate-200 hover:bg-slate-900 border border-transparent'
  }`;

/** Coquille commune : header collant, navigation, zone de contenu, footer. */
export default function Layout() {
  const { user, signOut } = useAuth();
  const { username } = useProfile();

  return (
    <div className="min-h-screen bg-slate-950 text-slate-100 font-sans flex flex-col selection:bg-cyan-500 selection:text-slate-950">
      <header className="bg-slate-900/90 backdrop-blur border-b border-slate-800 sticky top-0 z-50 px-4 py-3">
        <div className="max-w-7xl mx-auto flex flex-wrap items-center justify-between gap-4">
          <Link to="/" className="flex items-center space-x-3">
            <div className="bg-gradient-to-tr from-cyan-500 to-purple-600 p-2 rounded-xl shadow-lg shadow-cyan-500/20">
              <Cpu className="w-6 h-6 text-white" />
            </div>
            <div>
              <span className="font-extrabold text-lg tracking-wider bg-clip-text text-transparent bg-gradient-to-r from-cyan-400 via-purple-400 to-pink-500">
                RETRO ASSEMBLY
              </span>
              <p className="text-xs text-slate-400">Émulateurs C++ / WebAssembly</p>
            </div>
          </Link>

          <nav className="flex items-center space-x-1 bg-slate-950/60 p-1.5 rounded-xl border border-slate-800/80">
            <NavLink to="/" end className={tabClass}>
              <FolderOpen className="w-4 h-4" />
              <span>Bibliothèque</span>
            </NavLink>
            <NavLink to="/controls" className={tabClass}>
              <Gamepad2 className="w-4 h-4" />
              <span>Contrôles</span>
            </NavLink>
          </nav>

          <div className="flex items-center space-x-3 text-xs">
            <span className="hidden sm:block text-slate-400 font-mono truncate max-w-[200px]">
              {username ?? user?.email}
            </span>
            <button
              onClick={() => void signOut()}
              className="flex items-center space-x-1.5 bg-slate-800 hover:bg-slate-700 text-slate-300 px-3 py-1.5 rounded-lg border border-slate-700 transition"
            >
              <LogOut className="w-3.5 h-3.5" />
              <span>Déconnexion</span>
            </button>
          </div>
        </div>
      </header>

      <main className="flex-1 max-w-7xl w-full mx-auto p-4 md:p-6">
        <Outlet />
      </main>

      <footer className="border-t border-slate-800/80 bg-slate-900/50 py-4 px-6 text-center text-xs text-slate-500">
        Retro Assembly • React, Tailwind, Supabase & WebAssembly
      </footer>
    </div>
  );
}
