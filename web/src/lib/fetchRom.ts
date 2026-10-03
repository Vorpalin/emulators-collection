export const MAX_ROM_BYTES = 16 * 1024 * 1024;

export async function fetchRom(url: string): Promise<Uint8Array> {
  let res: Response;
  try {
    res = await fetch(url, {
      mode: "cors",
      credentials: "omit",
      referrerPolicy: "no-referrer",
    });
  } catch {
    throw new Error(
      "Failed to download ROM. Check the URL and that the server allows this site " +
        "(CORS header « Access-Control-Allow-Origin »).",
    );
  }
  if (!res.ok) throw new Error(`Failed to download ROM (HTTP ${res.status}).`);

  const declared = Number(res.headers.get("content-length") ?? 0);
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

function tooLarge(): Error {
  return new Error(
    `ROM too large (maximum ${MAX_ROM_BYTES / 1024 / 1024} Mo).`,
  );
}
