import { useCallback, useEffect, useState } from 'react';
import { supabase } from '../lib/supabase';
import { useAuth } from '../auth/AuthProvider';
import { SYSTEMS, type SystemId } from '../emulator/systems';
import { localizeBindings, useLayoutMap } from '../emulator/keyboardLayout';
import type { Settings } from '../types';

const DEFAULTS: Settings = { key_bindings: {}, crt_filter: true, volume: 80 };

/**
 * A custom React hook for managing the user's settings.
 * @returns An object containing the user's settings, a loading state, an update function, and functions to get default and current key bindings for a given system.
 */
export function useSettings() {
  const { user } = useAuth();
  const [settings, setSettings] = useState<Settings>(DEFAULTS);
  const [loading, setLoading] = useState(true);
  const layout = useLayoutMap();

  useEffect(() => {
    if (!user) return;
    let cancelled = false;
    supabase
      .from('user_settings')
      .select('key_bindings, crt_filter, volume')
      .maybeSingle()
      .then(({ data }) => {
        if (cancelled) return;
        if (data) setSettings({ ...DEFAULTS, ...(data as Settings) });
        setLoading(false);
      });
    return () => {
      cancelled = true;
    };
  }, [user]);

  const update = useCallback(
    async (patch: Partial<Settings>): Promise<void> => {
      if (!user) return;
      setSettings((prev) => ({ ...prev, ...patch }));
      await supabase.from('user_settings').upsert({
        user_id: user.id,
        ...patch,
        updated_at: new Date().toISOString(),
      });
    },
    [user],
  );

  const defaultsFor = useCallback(
    (system: SystemId): Record<string, string> =>
      localizeBindings(SYSTEMS[system].defaultBindings, layout),
    [layout],
  );

  const bindingsFor = useCallback(
    (system: SystemId): Record<string, string> => ({
      ...defaultsFor(system),
      ...settings.key_bindings[system],
    }),
    [defaultsFor, settings.key_bindings],
  );

  return { settings, loading, update, bindingsFor, defaultsFor };
}
