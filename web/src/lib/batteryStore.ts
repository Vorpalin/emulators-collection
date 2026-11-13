/**
 * Browser storage for "in-game saves" (battery-backed cartridge RAM).
 *
 * The ROM is never modified: the save lives next to it, in IndexedDB, keyed by
 * game id. The bytes have the same layout as a `.sav` file.
 */

import { BATTERY_STORE, openEmulatorDatabase } from './emulatorDatabase';

interface BatteryRecord {
  gameId: string;
  data: Uint8Array;
  updatedAt: number;
}

async function withStore<T>(
  mode: IDBTransactionMode,
  run: (store: IDBObjectStore) => IDBRequest<T>,
): Promise<T> {
  const db = await openEmulatorDatabase();

  return new Promise<T>((resolve, reject) => {
    let result: T;

    try {
      const transaction = db.transaction(BATTERY_STORE, mode);
      const request = run(transaction.objectStore(BATTERY_STORE));

      request.onsuccess = () => {
        result = request.result;
      };
      // Resolve on commit, not on request success, so a write is really durable.
      transaction.oncomplete = () => resolve(result);
      transaction.onerror = () =>
        reject(transaction.error ?? new Error('IndexedDB transaction failed.'));
      transaction.onabort = () =>
        reject(transaction.error ?? new Error('IndexedDB transaction aborted.'));
    } catch (error) {
      reject(error);
    }
  });
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
