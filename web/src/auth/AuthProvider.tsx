import { createContext, useContext, useEffect, useState, type ReactNode } from 'react';
import type { Session, User } from '@supabase/supabase-js';
import { supabase } from '../lib/supabase';

/**
 * AuthState interface defines the shape of the authentication state.
 * @property user - The current user, or null if not authenticated.
 * @property loading - Whether the authentication state is being loaded.
 * @property signOut - A function to sign out the current user.
 */
interface AuthState {
  /** The current user, or null if not authenticated. */
  user: User | null;
  /** Whether the authentication state is being loaded. */
  loading: boolean;
  /** A function to sign out the current user. */
  signOut: () => Promise<void>;
}

const AuthContext = createContext<AuthState | null>(null);

/**
 * Provides authentication context to its children.
 * @param children The children to render within the authentication context.
 * @returns The authentication context provider.
 */
export function AuthProvider({ children }: { children: ReactNode }) {
  const [session, setSession] = useState<Session | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    let active = true;

    supabase.auth
      .getSession()
      .then(({ data }) => {
        if (active) setSession(data.session);
      })
      .catch((err) => {
        console.error('Could not read the session:', err);
        if (active) setSession(null);
      })
      .finally(() => {
        if (active) setLoading(false);
      });

    const { data } = supabase.auth.onAuthStateChange((_event, next) => {
      setSession(next);
      setLoading(false);
    });

    return () => {
      active = false;
      data.subscription.unsubscribe();
    };
  }, []);

  const value: AuthState = {
    user: session?.user ?? null,
    loading,
    signOut: async () => {
      await supabase.auth.signOut();
    },
  };
  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}

/**
 * Hook to access the authentication state.
 * @returns The current authentication state.
 */
export function useAuth(): AuthState {
  const ctx = useContext(AuthContext);
  if (!ctx) throw new Error('useAuth needs to be used within <AuthProvider>');
  return ctx;
}
