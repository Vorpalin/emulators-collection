import { useEffect, useState, type FormEvent } from 'react';
import { Navigate, useLocation } from 'react-router-dom';
import { Cpu, Eye, EyeOff } from 'lucide-react';
import { supabase } from '../lib/supabase';
import { useAuth } from '../auth/AuthProvider';

const USERNAME_RE = /^[A-Za-z0-9_]{3,20}$/;
const RESET_COOLDOWN_SECONDS = 60;

type Mode = 'signin' | 'signup' | 'forgot';

export default function LoginPage() {
  const { user, loading } = useAuth();
  const location = useLocation();
  const from = (location.state as { from?: string } | null)?.from ?? '/';

  const [mode, setMode] = useState<Mode>('signin');
  const [username, setUsername] = useState('');
  const [email, setEmail] = useState('');
  const [password, setPassword] = useState('');
  const [showPassword, setShowPassword] = useState(false);
  const [busy, setBusy] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const [info, setInfo] = useState<string | null>(null);
  const [cooldown, setCooldown] = useState(0);

  useEffect(() => {
    if (cooldown <= 0) return;
    const timer = setTimeout(() => setCooldown((s) => s - 1), 1000);
    return () => clearTimeout(timer);
  }, [cooldown]);

  if (!loading && user) return <Navigate to={from} replace />;

  const switchMode = (next: Mode) => {
    setMode(next);
    setError(null);
    setInfo(null);
  };

  const onSubmit = async (e: FormEvent) => {
    e.preventDefault();
    setBusy(true);
    setError(null);
    setInfo(null);

    if (mode === 'forgot') {
      const { error: err } = await supabase.auth.resetPasswordForEmail(email, {
        redirectTo: `${window.location.origin}${import.meta.env.BASE_URL}reset-password`,
      });

      if (err?.status === 429) {
        setError('Too many requests. Please wait a minute before trying again.');
      } else {
        if (err) console.error(err);
        setInfo('If an account exists for this email, a reset link has been sent.');
        setCooldown(RESET_COOLDOWN_SECONDS);
      }
      setBusy(false);
      return;
    }

    if (mode === 'signin') {
      const { error: err } = await supabase.auth.signInWithPassword({
        email,
        password,
      });
      if (err) setError(err.message);
    } else {
      if (!USERNAME_RE.test(username)) {
        setError('Username : 3 to 20 characters (letters, numbers, _).');
        setBusy(false);
        return;
      }

      const { data: free, error: rpcErr } = await supabase.rpc('is_username_available', {
        name: username,
      });
      if (rpcErr) {
        setError(rpcErr.message);
        setBusy(false);
        return;
      }
      if (!free) {
        setError('This username is already taken.');
        setBusy(false);
        return;
      }

      const { data, error: err } = await supabase.auth.signUp({
        email,
        password,
        options: {
          data: { username },

          emailRedirectTo: `${window.location.origin}${import.meta.env.BASE_URL}`,
        },
      });
      if (err) setError(err.message);
      else if (!data.session) setInfo('Account created: check your email to confirm.');
    }
    setBusy(false);
  };

  const input =
    'w-full bg-slate-950 border border-slate-800 rounded-lg px-3 py-2 text-sm text-slate-200 placeholder-slate-500 focus:outline-none focus:border-cyan-500 transition';

  const subtitle =
    mode === 'signin' ? 'Sign in' : mode === 'signup' ? 'Create an account' : 'Reset your password';

  const submitLabel = busy
    ? '…'
    : mode === 'signin'
      ? 'Sign in'
      : mode === 'signup'
        ? 'Sign up'
        : cooldown > 0
          ? `Resend in ${cooldown}s`
          : 'Send reset link';

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
            <p className="text-xs text-slate-400">{subtitle}</p>
          </div>
        </div>

        {mode === 'forgot' && (
          <p className="text-sm text-slate-400">
            Enter the email linked to your account and we'll send you a link to choose a new
            password.
          </p>
        )}

        {mode === 'signup' && (
          <input
            type="text"
            required
            minLength={3}
            maxLength={20}
            pattern="[A-Za-z0-9_]{3,20}"
            title="3 to 20 characters: letters, numbers, or _"
            autoComplete="username"
            placeholder="Username"
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            className={input}
          />
        )}
        <input
          type="email"
          required
          autoComplete="email"
          placeholder="Email"
          value={email}
          onChange={(e) => setEmail(e.target.value)}
          className={input}
        />

        {mode !== 'forgot' && (
          <div className="relative">
            <input
              type={showPassword ? 'text' : 'password'}
              required
              minLength={6}
              autoComplete={mode === 'signin' ? 'current-password' : 'new-password'}
              placeholder="Password"
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
        )}

        {mode === 'signin' && (
          <div className="text-right -mt-2">
            <button
              type="button"
              onClick={() => switchMode('forgot')}
              className="text-xs text-slate-400 hover:text-cyan-400 transition"
            >
              Forgot password?
            </button>
          </div>
        )}

        {error && <p className="text-sm text-rose-400">{error}</p>}
        {info && <p className="text-sm text-emerald-400">{info}</p>}

        <button
          type="submit"
          disabled={busy || (mode === 'forgot' && cooldown > 0)}
          className="w-full bg-gradient-to-r from-cyan-500 to-blue-600 hover:from-cyan-400 hover:to-blue-500 disabled:opacity-60 text-slate-950 font-semibold py-2.5 rounded-xl transition"
        >
          {submitLabel}
        </button>

        <button
          type="button"
          onClick={() =>
            switchMode(mode === 'signup' ? 'signin' : mode === 'signin' ? 'signup' : 'signin')
          }
          className="w-full text-xs text-slate-400 hover:text-cyan-400 transition"
        >
          {mode === 'signin'
            ? 'No account ? Sign up'
            : mode === 'signup'
              ? 'Already have an account ? Sign in'
              : 'Back to sign in'}
        </button>
      </form>
    </div>
  );
}
