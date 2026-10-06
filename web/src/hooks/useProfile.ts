import { useEffect, useState } from 'react';
import { supabase } from '../lib/supabase';
import { useAuth } from '../auth/AuthProvider';

/**
 * A custom React hook for managing the user's profile information.
 * It fetches the username of the currently authenticated user from the Supabase database.
 * The hook updates the username state whenever the user changes.
 * @returns An object containing the username of the authenticated user, or null if no user is authenticated.
 */
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
