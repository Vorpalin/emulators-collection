import { Capacitor } from '@capacitor/core';

/**
 *  Get the full URL for a given path, taking into account the environment (native or web).
 * @param path The path to append to the base URL.
 * @returns The full URL as a string.
 */
export function appUrl(path = ''): string {
  const normalizedPath = path.replace(/^\/+/, '');

  if (Capacitor.isNativePlatform()) {
    const base = import.meta.env.VITE_PUBLIC_URL ?? '';
    return `${base.replace(/\/+$/, '')}/${normalizedPath}`;
  }

  return `${window.location.origin}${import.meta.env.BASE_URL}${normalizedPath}`;
}
