/**
 * Browser storage for "in-game saves" (battery-backed cartridge RAM).
 *
 * The ROM is never modified: the save lives next to it, in IndexedDB, keyed by
 * game id. The bytes have the same layout as a `.sav` file.
 */

const DB_NAME = 'emulators-collection';
const DB_VERSION = 1;
const STORE = 'battery';

interface BatteryRecord {
  gameId: string;
  data: Uint8Array;
  updatedAt: number;
}

function openDb(): Promise<IDBDatabase> {
  return new Promise((resolve, reject) => {
    if (typeof indexedDB === 'undefined') {
      reject(new Error('IndexedDB is not available.'));
      return;
    }

    const request = indexedDB.open(DB_NAME, DB_VERSION);

    request.onupgradeneeded = () => {
      if (!request.result.objectStoreNames.contains(STORE)) {
        request.result.createObjectStore(STORE, { keyPath: 'gameId' });
      }
    };
    request.onsuccess = () => resolve(request.result);
    request.onerror = () => reject(request.error ?? new Error('Unable to open IndexedDB.'));
    request.onblocked = () => reject(new Error('IndexedDB is blocked by another tab.'));
  });
}

async function withStore<T>(
  mode: IDBTransactionMode,
  run: (store: IDBObjectStore) => IDBRequest<T>,
): Promise<T> {
  const db = await openDb();

  try {
    return await new Promise<T>((resolve, reject) => {
      const tx = db.transaction(STORE, mode);
      const request = run(tx.objectStore(STORE));
      let result: T;

      request.onsuccess = () => {
        result = request.result;
      };
      // Resolve on commit, not on request success, so a write is really durable.
      tx.oncomplete = () => resolve(result);
      tx.onerror = () => reject(tx.error ?? new Error('IndexedDB transaction failed.'));
      tx.onabort = () => reject(tx.error ?? new Error('IndexedDB transaction aborted.'));
    });
  } finally {
    db.close();
  }
}

/**
 * Reads the stored in-game save of a game.
 * @param gameId The game id.
 * @returns The saved bytes, or null if the game has no stored save.
 */
export async function loadBattery(gameId: string): Promise<Uint8Array | null> {
  const record = await withStore<BatteryRecord | undefined>('readonly', (store) =>
    store.get(gameId),
  );

  return record ? record.data : null;
}

/**
 * Stores (replaces) the in-game save of a game.
 * @param gameId The game id.
 * @param data The battery RAM bytes.
 */
export async function saveBattery(gameId: string, data: Uint8Array): Promise<void> {
  const record: BatteryRecord = { gameId, data, updatedAt: Date.now() };

  await withStore('readwrite', (store) => store.put(record));
}

/**
 * Deletes the stored in-game save of a game.
 * @param gameId The game id.
 */
export async function deleteBattery(gameId: string): Promise<void> {
  await withStore('readwrite', (store) => store.delete(gameId));
}
