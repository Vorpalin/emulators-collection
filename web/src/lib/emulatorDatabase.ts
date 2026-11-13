/**
 * Shared IndexedDB database for browser-local emulator data.
 *
 * All user-owned game data stays in this browser. Supabase is used only to
 * bootstrap metadata for ROMs already present locally during the migration.
 */
const DATABASE_NAME = 'emulators-collection';
const DATABASE_VERSION = 3;

export const ROM_STORE = 'roms';
export const BATTERY_STORE = 'battery';
export const GAME_STORE = 'games';

let databasePromise: Promise<IDBDatabase> | undefined;

export function openEmulatorDatabase(): Promise<IDBDatabase> {
  if (typeof indexedDB === 'undefined') {
    return Promise.reject(new Error('IndexedDB is not available in this browser.'));
  }

  if (!databasePromise) {
    databasePromise = new Promise((resolve, reject) => {
      const request = indexedDB.open(DATABASE_NAME, DATABASE_VERSION);

      request.onupgradeneeded = () => {
        const database = request.result;

        if (!database.objectStoreNames.contains(ROM_STORE)) {
          database.createObjectStore(ROM_STORE);
        }
        if (!database.objectStoreNames.contains(BATTERY_STORE)) {
          database.createObjectStore(BATTERY_STORE, { keyPath: 'gameId' });
        }
        if (!database.objectStoreNames.contains(GAME_STORE)) {
          database.createObjectStore(GAME_STORE, { keyPath: 'id' });
        }
      };

      request.onsuccess = () => {
        const database = request.result;
        database.onversionchange = () => {
          database.close();
          databasePromise = undefined;
        };
        resolve(database);
      };

      request.onerror = () => {
        databasePromise = undefined;
        reject(request.error ?? new Error('Unable to open emulator browser storage.'));
      };

      request.onblocked = () => {
        databasePromise = undefined;
        reject(
          new Error('IndexedDB upgrade is blocked by another tab. Close other app tabs and retry.'),
        );
      };
    });
  }

  return databasePromise;
}
