/**
 * Shared IndexedDB database for browser-local emulator data.
 *
 * Keep the database schema in one place: ROM binaries and battery-backed RAM
 * must coexist in the same database regardless of which module opens it first.
 */
const DATABASE_NAME = 'emulators-collection';
const DATABASE_VERSION = 2;

export const ROM_STORE = 'roms';
export const BATTERY_STORE = 'battery';

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

        // Create every store during the same version upgrade. This also
        // migrates existing version-1 databases created by either old module.
        if (!database.objectStoreNames.contains(ROM_STORE)) {
          database.createObjectStore(ROM_STORE);
        }
        if (!database.objectStoreNames.contains(BATTERY_STORE)) {
          database.createObjectStore(BATTERY_STORE, { keyPath: 'gameId' });
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
