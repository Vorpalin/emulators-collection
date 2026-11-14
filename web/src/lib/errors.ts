/**
 * Errors the player can see, identified by a translation key instead of hard-coded text.
 *
 * Code that runs outside React (the emulator session, the storage helpers, the WebAssembly
 * loader) cannot call `useI18n()`. It throws a {@link LocalizedError} instead, whose `message`
 * is already written in the language the interface is currently displayed in, so every place
 * that shows `error.message` stays correct without knowing anything about translations.
 *
 * The message keys live in `i18n/I18nProvider.tsx`, which registers its `t` function here.
 */

/** Translates a message key, replacing the `{name}` placeholders with `values`. */
export type Translate = (key: string, values?: Record<string, string | number>) => string;

let translate: Translate | null = null;

/**
 * Registers the function used to translate error messages. Called by `I18nProvider` each time
 * the displayed language changes.
 * @param next The translation function, or null to unregister it.
 */
export function setErrorTranslator(next: Translate | null): void {
  translate = next;
}

/**
 * Returns a message in the current language.
 * @param key The translation key.
 * @param fallback The English text used while no translator is registered (before the
 * interface has mounted, or in tests).
 * @param values The values of the `{name}` placeholders of the message, if any.
 * @returns The translated message.
 */
export function localizedMessage(
  key: string,
  fallback: string,
  values?: Record<string, string | number>,
): string {
  return translate ? translate(key, values) : fallback;
}

/**
 * An error whose message is translated into the current interface language when it is created.
 */
export class LocalizedError extends Error {
  /** The translation key of the message. */
  readonly key: string;

  /**
   * Creates the error.
   * @param key The translation key of the message.
   * @param fallback The English text used while no translator is registered.
   * @param options The values of the message placeholders, and the error that caused this one.
   */
  constructor(
    key: string,
    fallback: string,
    options: { values?: Record<string, string | number>; cause?: unknown } = {},
  ) {
    super(localizedMessage(key, fallback, options.values), { cause: options.cause });
    this.name = 'LocalizedError';
    this.key = key;
  }
}
