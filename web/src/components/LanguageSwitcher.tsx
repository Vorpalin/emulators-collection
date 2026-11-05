import { useI18n } from '../i18n/I18nProvider';

export default function LanguageSwitcher() {
  const { language, setLanguage, t } = useI18n();

  return (
    <label className="inline-flex items-center gap-2 text-xs text-slate-400">
      <span>{t('language')}</span>
      <select
        value={language}
        onChange={(event) => setLanguage(event.target.value as typeof language)}
        className="rounded-lg border border-slate-800 bg-slate-900 px-2.5 py-2 text-xs font-medium text-slate-200 outline-none focus-visible:ring-2 focus-visible:ring-cyan-400"
      >
        <option value="en">{t('english')}</option>
        <option value="fr">{t('french')}</option>
      </select>
    </label>
  );
}
