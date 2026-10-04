import { useEffect, useRef, useState, type FormEvent } from 'react';
import { Link, useNavigate } from 'react-router-dom';
import { Cpu, Eye, EyeOff } from 'lucide-react';
import { supabase } from '../lib/supabase';

export default function ResetPasswordPage() {
  const navigate = useNavigate();

  const [linkValid, setLinkValid] = useState<boolean | null>(null);
  const [password, setPassword] = useState('');
  const [confirm, setConfirm] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [done, setDone] = useState(false);

  const verifyStarted = useRef(false);

  useEffect(() => {
    const params = new URLSearchParams(window.location.search);
    const tokenHash = params.get('token_hash');
    if (!tokenHash || params.get('type') !== 'recovery') return;

    if (verifyStarted.current) return;
    verifyStarted.current = true;

    supabase.auth.verifyOtp({ token_hash: tokenHash, type: 'recovery' }).then(({ error }) => {
      window.history.replaceState({}, '', window.location.pathname);
      setLinkValid(!error);
    });
  }, []);

  useEffect(() => {
    let active = true;

    supabase.auth.getSession().then(({ data }) => {
      if (!active) return;
      if (new URLSearchParams(window.location.search).has('token_hash')) return;
      setLinkValid(!!data.session);
    });

    const { data: sub } = supabase.auth.onAuthStateChange((event) => {
      if (event === 'PASSWORD_RECOVERY' && active) setLinkValid(true);
    });

    return () => {
      active = false;
      sub.subscription.unsubscribe();
    };
  }, []);

  const onSubmit = async (e: FormEvent) => {
    e.preventDefault();
    setError(null);

    if (password !== confirm) {
      setError('Passwords do not match.');
      return;
    }

    setBusy(true);
    const { error: err } = await supabase.auth.updateUser({ password });
    setBusy(false);

    if (err) {
      setError(err.message);
      return;
    }

    setDone(true);
    setTimeout(() => navigate('/', { replace: true }), 1500);
  };

  const input =
    'w-full bg-slate-950 border border-slate-800 rounded-lg px-3 py-2 text-sm text-slate-200 placeholder-slate-500 focus:outline-none focus:border-cyan-500 transition';

  return (
    <div className="min-h-screen bg-slate-950 flex items-center justify-center p-4">
      <form
        onSubmit={onSubmit}
        className="w-full max-w-sm bg-slate-900 border border-slate-800 rounded-2xl p-6 space-y-4 shadow-2xl"
      >
        <div className="flex items-center space-x-3">
          <div className="bg-gradient-to-tr from-cyan-500 to-purple-600 p-2 rounded-xl">
            <Cpu className="w-6 h-6 text-white" />
          </div>
          <div>
            <h1 className="font-extrabold tracking-wider text-white">EMULATORS COLLECTION</h1>
            <p className="text-xs text-slate-400">Choose a new password</p>
          </div>
        </div>

        {linkValid === null && <p className="text-sm text-slate-400">Checking your link…</p>}

        {linkValid === false && (
          <>
            <p className="text-sm text-rose-400">
              This reset link is invalid or has expired. Request a new one from the sign-in page.
            </p>
            <Link
              to="/login"
              className="block w-full text-center bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 text-slate-950 font-semibold py-2.5 rounded-xl transition"
            >
              Back to sign in
            </Link>
          </>
        )}

        {linkValid && !done && (
          <>
            <div className="relative">
              <input
                type={showPassword ? 'text' : 'password'}
                required
                minLength={6}
                autoComplete="new-password"
                placeholder="New password"
                value={password}
                onChange={(e) => setPassword(e.target.value)}
                className={`${input} pr-10`}
              />
              <button
                type="button"
                onClick={() => setShowPassword((visible) => !visible)}
                className="absolute right-3 top-1/2 -translate-y-1/2 text-slate-500 hover:text-slate-300 transition"
                aria-label={showPassword ? 'Hide password' : 'Show password'}
              >
                {showPassword ? <EyeOff className="w-4 h-4" /> : <Eye className="w-4 h-4" />}
              </button>
            </div>

            <input
              type={showPassword ? 'text' : 'password'}
              required
              minLength={6}
              autoComplete="new-password"
              placeholder="Confirm new password"
              value={confirm}
              onChange={(e) => setConfirm(e.target.value)}
              className={input}
            />

            {error && <p className="text-sm text-rose-400">{error}</p>}

            <button
              type="submit"
              disabled={busy}
              className="w-full bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 disabled:opacity-60 text-slate-950 font-semibold py-2.5 rounded-xl transition"
            >
              {busy ? '…' : 'Update password'}
            </button>
          </>
        )}

        {done && <p className="text-sm text-emerald-400">Password updated. Redirecting…</p>}
      </form>
    </div>
  );
}
