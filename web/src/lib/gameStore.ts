import { GAME_STORE, openEmulatorDatabase } from './emulatorDatabase';
import type { Game } from '../types';

async function withGameStore<T>(
  mode: IDBTransactionMode,
  run: (store: IDBObjectStore) => IDBRequest<T>,
): Promise<T> {
  const database = await openEmulatorDatabase();

  return new Promise<T>((resolve, reject) => {
    let result: T;
    const transaction = database.transaction(GAME_STORE, mode);
    const request = run(transaction.objectStore(GAME_STORE));

    request.onsuccess = () => {
      result = request.result;
    };
    transaction.oncomplete = () => resolve(result);
    transaction.onerror = () =>
      reject(transaction.error ?? new Error('Local game metadata transaction failed.'));
    transaction.onabort = () =>
      reject(transaction.error ?? new Error('Local game metadata transaction aborted.'));
  });
}

/** Lists game metadata saved in this browser. */
export function listGames(): Promise<Game[]> {
  return withGameStore('readonly', (store) => store.getAll());
}

/** Gets one game's metadata from this browser. */
export function getGame(gameId: string): Promise<Game | undefined> {
  return withGameStore('readonly', (store) => store.get(gameId));
}

/** Saves game metadata in this browser. */
export async function saveGame(game: Game): Promise<void> {
  await withGameStore('readwrite', (store) => store.put(game));
}

/** Removes game metadata from this browser only. */
export async function deleteGame(gameId: string): Promise<void> {
  await withGameStore('readwrite', (store) => store.delete(gameId));
}
