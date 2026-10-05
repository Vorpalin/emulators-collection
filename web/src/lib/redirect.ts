import { Capacitor } from '@capacitor/core';

export function appUrl(path = ''): string {
  const normalizedPath = path.replace(/^\/+/, '');

  if (Capacitor.isNativePlatform()) {
    const base = import.meta.env.VITE_PUBLIC_URL ?? '';
    return `${base.replace(/\/+$/, '')}/${normalizedPath}`;
  }

  return `${window.location.origin}${import.meta.env.BASE_URL}${normalizedPath}`;
}
