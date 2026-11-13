import { openEmulatorDatabase, ROM_STORE } from './emulatorDatabase';

async function checkAvailableStorage(requiredBytes: number): Promise<void> {
  if (!navigator.storage?.estimate) return;

  const { usage = 0, quota } = await navigator.storage.estimate();
  if (quota !== undefined && requiredBytes > Math.max(0, quota - usage)) {
    throw new Error(
      'There is not enough estimated browser storage for this ROM. Free some space or remove unused games and try again.',
    );
  }
}

/**
 * Stores a ROM in this browser. The browser controls the available quota; there
 * is deliberately no application-level ROM size limit.
 */
export async function saveRom(gameId: string, file: File): Promise<void> {
  await checkAvailableStorage(file.size);

  // Persistent storage reduces the risk of automatic eviction where supported.
  // A false result is not fatal: the write is still attempted.
  try {
    await navigator.storage?.persist?.();
  } catch {
    // Persistence is a best-effort browser feature.
  }

  const database = await openEmulatorDatabase();

  try {
    await new Promise<void>((resolve, reject) => {
      const transaction = database.transaction(ROM_STORE, 'readwrite');
      transaction.objectStore(ROM_STORE).put(file, gameId);
      transaction.oncomplete = () => resolve();
      transaction.onerror = () => reject(transaction.error);
      transaction.onabort = () =>
        reject(transaction.error ?? new Error('ROM storage was aborted.'));
    });
  } catch (error) {
    if (error instanceof DOMException && error.name === 'QuotaExceededError') {
      throw new Error(
        'The browser does not have enough storage for this ROM. Free some space or remove unused games.',
      );
    }
    throw error;
  }
}

/** Loads a ROM saved in this browser. */
export async function getRom(gameId: string): Promise<Uint8Array> {
  const database = await openEmulatorDatabase();

  const file = await new Promise<Blob | undefined>((resolve, reject) => {
    const transaction = database.transaction(ROM_STORE, 'readonly');
    const request = transaction.objectStore(ROM_STORE).get(gameId);
    request.onsuccess = () => resolve(request.result as Blob | undefined);
    request.onerror = () => reject(request.error);
  });

  if (!file) {
    throw new Error(
      'This ROM is not stored in this browser. Delete this library entry and import the ROM file again on this device.',
    );
  }

  return new Uint8Array(await file.arrayBuffer());
}

/** Deletes a ROM from this browser. */
export async function deleteRom(gameId: string): Promise<void> {
  const database = await openEmulatorDatabase();

  await new Promise<void>((resolve, reject) => {
    const transaction = database.transaction(ROM_STORE, 'readwrite');
    transaction.objectStore(ROM_STORE).delete(gameId);
    transaction.oncomplete = () => resolve();
    transaction.onerror = () => reject(transaction.error);
    transaction.onabort = () => reject(transaction.error ?? new Error('ROM deletion was aborted.'));
  });
}

/** Lists ROM identifiers available in this browser's IndexedDB. */
export async function listRomIds(): Promise<string[]> {
  const database = await openEmulatorDatabase();

  return new Promise<string[]>((resolve, reject) => {
    const transaction = database.transaction(ROM_STORE, 'readonly');
    const request = transaction.objectStore(ROM_STORE).getAllKeys();
    request.onsuccess = () => resolve(request.result.map(String));
    request.onerror = () => reject(request.error);
  });
}
