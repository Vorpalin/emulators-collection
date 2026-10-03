import { createContext, useContext, useEffect, useState, type ReactNode } from 'react';
import type { Session, User } from '@supabase/supabase-js';
import { supabase } from '../lib/supabase';

interface AuthState {
  user: User | null;
  loading: boolean;
  signOut: () => Promise<void>;
}

const AuthContext = createContext<AuthState | null>(null);

export function AuthProvider({ children }: { children: ReactNode }) {
  const [session, setSession] = useState<Session | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    let active = true;

    // Lecture de la session stockée. Si elle échoue (verrou du navigateur, stockage
    // illisible...), on considère l'utilisateur déconnecté : sans ce catch, `loading`
    // restait à true et l'écran « Loading… » bloquait la page de connexion à jamais.
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

    // Se déclenche aussi au chargement (INITIAL_SESSION), à la connexion, à la
    // déconnexion et au rafraîchissement du jeton : l'état initial est donc connu
    // dès le premier événement, même si getSession() tarde.
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

export function useAuth(): AuthState {
  const ctx = useContext(AuthContext);
  if (!ctx) throw new Error('useAuth needs to be used within <AuthProvider>');
  return ctx;
}
