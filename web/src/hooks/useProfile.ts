import { useEffect, useState } from 'react';
import { supabase } from '../lib/supabase';
import { useAuth } from '../auth/AuthProvider';

/** Pseudo de l'utilisateur connecté (table `profiles`). */
export function useProfile() {
  const { user } = useAuth();
  const [username, setUsername] = useState<string | null>(null);

  useEffect(() => {
    if (!user) {
      setUsername(null);
      return;
    }
    let cancelled = false;
    supabase
      .from('profiles')
      .select('username')
      .eq('id', user.id)
      .maybeSingle()
      .then(({ data }) => {
        if (!cancelled) setUsername(data?.username ?? null);
      });
    return () => {
      cancelled = true;
    };
  }, [user]);

  return { username };
}
