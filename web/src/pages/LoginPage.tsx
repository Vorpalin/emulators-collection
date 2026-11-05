import { useEffect, useState, type FormEvent } from 'react';
import { Navigate, useLocation } from 'react-router-dom';
import { Cpu, Eye, EyeOff } from 'lucide-react';
import { supabase } from '../lib/supabase';
import { useAuth } from '../auth/AuthProvider';
import { appUrl } from '../lib/redirect';
import { useI18n } from '../i18n/I18nProvider';
import LanguageSwitcher from '../components/LanguageSwitcher';

const USERNAME_RE = /^[A-Za-z0-9_]{3,20}$/;
const RESET_COOLDOWN_SECONDS = 60;

type Mode = 'signin' | 'signup' | 'forgot';

/**
 * LoginPage component provides a user interface for signing in, signing up, and resetting passwords. It manages the state of the form inputs, handles form submission, and displays appropriate messages based on the user's actions and server responses.
 * @returns A React component that renders the login page for user authentication.
 */
export default function LoginPage() {
  const { user, loading } = useAuth();
  const { t } = useI18n();
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

    try {
      if (mode === 'forgot') {
        const { error: err } = await supabase.auth.resetPasswordForEmail(email, {
          redirectTo: appUrl('reset-password'),
        });

        if (err?.status === 429) {
          setError(t('tooManyRequests'));
        } else {
          if (err) console.error(err);
          setInfo(t('resetEmailInfo'));
          setCooldown(RESET_COOLDOWN_SECONDS);
        }
        return;
      }

      if (mode === 'signin') {
        const { error: err } = await supabase.auth.signInWithPassword({
          email,
          password,
        });
        if (err) setError(err.message);
        return;
      }

      if (!USERNAME_RE.test(username)) {
        setError(t('usernameValidation'));
        return;
      }

      const { data: free, error: rpcErr } = await supabase.rpc('is_username_available', {
        name: username,
      });
      if (rpcErr) {
        setError(rpcErr.message);
        return;
      }
      if (!free) {
        setError(t('usernameTaken'));
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
      else if (!data.session) setInfo(t('accountCreated'));
    } catch (e) {
      setError(e instanceof Error ? e.message : t('authFailed'));
    } finally {
      setBusy(false);
    }
  };

  const input =
    'w-full bg-slate-950 border border-slate-800 rounded-lg px-3 py-2 text-sm text-slate-200 placeholder-slate-500 focus:outline-none focus:border-cyan-500 transition';

  const subtitle =
    mode === 'signin' ? t('signIn') : mode === 'signup' ? t('createAccount') : t('resetPassword');

  const submitLabel = busy
    ? '…'
    : mode === 'signin'
      ? t('signIn')
      : mode === 'signup'
        ? t('createAccount')
        : cooldown > 0
          ? t('resendIn', { seconds: cooldown })
          : t('sendResetLink');

  return (
    <div className="min-h-screen bg-slate-950 flex items-center justify-center p-4">
      <div className="fixed right-4 top-4 z-10"><LanguageSwitcher /></div>
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
            {t('enterResetEmail')}
          </p>
        )}

        {mode === 'signup' && (
          <input
            type="text"
            required
            minLength={3}
            maxLength={20}
            pattern="[A-Za-z0-9_]{3,20}"
            title={t('usernameValidation')}
            autoComplete="username"
            placeholder={t('username')}
            aria-label={t('username')}
            value={username}
            onChange={(e) => setUsername(e.target.value)}
            className={input}
          />
        )}
        <input
          type="email"
          required
          autoComplete="email"
          placeholder={t('email')}
          aria-label={t('email')}
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
              placeholder={t('password')}
              aria-label={t('password')}
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              className={`${input} pr-10`}
            />

            <button
              type="button"
              onClick={() => setShowPassword((visible) => !visible)}
              className="absolute right-3 top-1/2 -translate-y-1/2 text-slate-500 hover:text-slate-300 transition"
              aria-label={showPassword ? t('hidePassword') : t('showPassword')}
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
              {t('forgotPassword')}
            </button>
          </div>
        )}

        {error && (
          <p role="alert" className="text-sm text-rose-400">
            {error}
          </p>
        )}
        {info && (
          <p role="status" aria-live="polite" className="text-sm text-emerald-400">
            {info}
          </p>
        )}

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
            ? t('noAccount')
            : mode === 'signup'
              ? t('haveAccount')
              : t('backToSignIn')}
        </button>
      </form>
    </div>
  );
}
