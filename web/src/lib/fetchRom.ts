export const MAX_ROM_BYTES = 16 * 1024 * 1024;

/**
 * Fetch a ROM from a URL and return it as a Uint8Array.
 * Throws an error if the ROM is too large or if the fetch fails.
 * @param url The URL of the ROM to fetch.
 * @returns A promise that resolves to a Uint8Array containing the ROM data.
 */
export async function fetchRom(url: string): Promise<Uint8Array> {
  let res: Response;
  try {
    res = await fetch(url, {
      mode: 'cors',
      credentials: 'omit',
      referrerPolicy: 'no-referrer',
    });
  } catch {
    throw new Error(
      'Failed to download ROM. Check the URL and that the server allows this site ' +
        '(CORS header « Access-Control-Allow-Origin »).',
    );
  }
  if (!res.ok) throw new Error(`Failed to download ROM (HTTP ${res.status}).`);

  const declared = Number(res.headers.get('content-length') ?? 0);
  if (declared > MAX_ROM_BYTES) throw tooLarge();

  if (!res.body) {
    const buf = new Uint8Array(await res.arrayBuffer());
    if (buf.byteLength > MAX_ROM_BYTES) throw tooLarge();
    return buf;
  }

  const reader = res.body.getReader();
  const chunks: Uint8Array[] = [];
  let total = 0;
  for (;;) {
    const { done, value } = await reader.read();
    if (done) break;
    total += value.byteLength;
    if (total > MAX_ROM_BYTES) {
      void reader.cancel();
      throw tooLarge();
    }
    chunks.push(value);
  }

  const out = new Uint8Array(total);
  let offset = 0;
  for (const c of chunks) {
    out.set(c, offset);
    offset += c.byteLength;
  }
  return out;
}

/**
 * Create an error indicating that the ROM is too large.
 * @returns An Error object with a message about the ROM size limit.
 */
function tooLarge(): Error {
  return new Error(`ROM too large (maximum ${MAX_ROM_BYTES / 1024 / 1024} Mo).`);
}
