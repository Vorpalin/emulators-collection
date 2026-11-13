const DATABASE_NAME = 'emulators-collection';
const DATABASE_VERSION = 1;
const STORE_NAME = 'roms';

let databasePromise: Promise<IDBDatabase> | undefined;

function openDatabase(): Promise<IDBDatabase> {
  if (typeof indexedDB === 'undefined') {
    return Promise.reject(new Error('Browser storage is not available in this environment.'));
  }

  if (!databasePromise) {
    databasePromise = new Promise((resolve, reject) => {
      const request = indexedDB.open(DATABASE_NAME, DATABASE_VERSION);

      request.onupgradeneeded = () => {
        const database = request.result;
        if (!database.objectStoreNames.contains(STORE_NAME)) {
          database.createObjectStore(STORE_NAME);
        }
      };

      request.onsuccess = () => resolve(request.result);
      request.onerror = () => {
        databasePromise = undefined;
        reject(request.error ?? new Error('Could not open browser storage.'));
      };
      request.onblocked = () => {
        databasePromise = undefined;
        reject(
          new Error(
            'Browser storage is blocked by another open tab. Close other app tabs and retry.',
          ),
        );
      };
    });
  }

  return databasePromise;
}

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

  const database = await openDatabase();

  try {
    await new Promise<void>((resolve, reject) => {
      const transaction = database.transaction(STORE_NAME, 'readwrite');
      transaction.objectStore(STORE_NAME).put(file, gameId);
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
  const database = await openDatabase();

  const file = await new Promise<Blob | undefined>((resolve, reject) => {
    const transaction = database.transaction(STORE_NAME, 'readonly');
    const request = transaction.objectStore(STORE_NAME).get(gameId);
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
  const database = await openDatabase();

  await new Promise<void>((resolve, reject) => {
    const transaction = database.transaction(STORE_NAME, 'readwrite');
    transaction.objectStore(STORE_NAME).delete(gameId);
    transaction.oncomplete = () => resolve();
    transaction.onerror = () => reject(transaction.error);
    transaction.onabort = () => reject(transaction.error ?? new Error('ROM deletion was aborted.'));
  });
}
