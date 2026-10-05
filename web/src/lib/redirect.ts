import { Capacitor } from '@capacitor/core';

export function appUrl(path = ''): string {
  if (Capacitor.isNativePlatform()) {
    const base = (import.meta.env.VITE_PUBLIC_URL as string | undefined) ?? '';
    return `${base.replace(/\/+$/, '')}/${path}`;
  }
  return `${window.location.origin}${import.meta.env.BASE_URL}${path}`;
}
