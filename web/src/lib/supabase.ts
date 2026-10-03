import { createClient } from "@supabase/supabase-js";

const url = import.meta.env.VITE_SUPABASE_URL as string | undefined;
const anonKey = import.meta.env.VITE_SUPABASE_ANON_KEY as string | undefined;

if (!url || !anonKey) {
  throw new Error(
    "Variables manquantes : copiez .env.example vers .env et renseignez " +
      "VITE_SUPABASE_URL et VITE_SUPABASE_ANON_KEY.",
  );
}

// La clé "anon" est publique par conception : la sécurité repose sur les
// politiques RLS définies dans supabase/schema.sql.
export const supabase = createClient(url, anonKey);
