import { createContext, useContext, useEffect, useMemo, useState, type ReactNode } from 'react';

export type Language = 'en' | 'fr';

const messages: Record<Language, Record<string, string>> = {
  en: {
    library: 'Library', controls: 'Controls', signOut: 'Sign out', playClassics: 'Play the classics',
    mainNavigation: 'Main navigation', mobileNavigation: 'Mobile navigation', theme: 'Color theme',
    systemTheme: 'System', lightTheme: 'Light', darkTheme: 'Dark', language: 'Language',
    english: 'English', french: 'Français', github: 'View project on GitHub',
    retroCollection: 'Your retro collection', gameLibrary: 'Game library',
    libraryDescription: 'Your personal collection, ready to play. Add your own ROMs and launch them directly in your browser.',
    game: 'game', games: 'games', systemsSupported: 'systems supported', closeUpload: 'Close upload',
    addGame: 'Add a game', browseGames: 'Browse games', loadingCollection: 'Loading your collection…',
    removingGame: 'Removing game…', showingGames: 'Showing {shown} of {total} games',
    searchCollection: 'Search your collection…', searchGames: 'Search games', clearSearch: 'Clear search',
    sortGames: 'Sort games', recentlyAdded: 'Recently added', oldestFirst: 'Oldest first',
    nameAZ: 'Name A–Z', nameZA: 'Name Z–A', filterBySystem: 'Filter by system',
    allSystems: 'All systems', shown: '{count} shown', noGamesFound: 'No games found',
    libraryWaiting: 'Your library is waiting', addRomStart: 'Add a ROM you own to get started. Supported formats: {formats}.',
    tryAnotherFilter: 'Try another search or system filter.', addFirstGame: 'Add your first game',
    resetFilters: 'Reset filters', deleteConfirm: 'Delete « {title} » from your library?',
    gameRemoved: '“{title}” was removed from your library.', dismiss: 'Dismiss',
    personalSetup: 'Personal setup', controlsDescription: 'Configure keyboard bindings for each system. Changes are synced to your account.',
    chooseSystem: 'Choose a system', keyboardLayout: 'keyboard layout', actions: 'actions',
    clickKeyRemap: 'Click a key to remap', restoreDefaults: 'Restore defaults',
    pressKeyFor: 'Press a key for', escapeCancels: 'Escape cancels.',
    keyAlreadyAssigned: '{key} was already assigned to {action}; the bindings were swapped.',
    keyAssignmentSwapped: 'An existing key assignment was swapped.',
    savingKeyboard: 'Saving keyboard changes…', keyboardSaved: 'Keyboard changes saved.',
    keyboardAutoSaved: 'Keyboard changes are saved automatically after each update.',
    unassigned: 'Unassigned', remapKey: 'Remap {action} key',
    addGameTitle: 'Add a game', closeAddGame: 'Close add game form',
    chooseRom: 'Choose a ROM file…', gameTitle: 'Game title', emulatorSystem: 'Emulator system',
    acceptedFormats: 'Accepted formats: {formats}. Maximum 16 MB. The file is stored privately in your account: only you can play it.',
    uploading: 'Uploading…', add: 'Add', playGame: 'Play game', added: 'Added',
    deleting: 'Deleting {title}', deleteGame: 'Delete {title}', externalRom: 'External ROM',
    emulatorCollection: 'Emulator collection', unknownSystem: 'Unknown system',
    controller: 'Controller', loading: 'Loading…', returnLibrary: 'Return to the library',
    saveStateDownloaded: 'Save state downloaded.', saveStateLoaded: 'Save state loaded successfully.',
    saveStateLoadFailed: 'The save state could not be loaded.', unableFullscreen: 'Unable to toggle fullscreen.',
    savingState: 'Loading save state…', loadState: 'Load state', saveState: 'Save state',
    pause: 'Pause', resume: 'Resume', mute: 'Mute', unmute: 'Unmute', fullscreen: 'Fullscreen',
    exitFullscreen: 'Exit fullscreen', volume: 'Volume', startGame: 'Start game',
    newVersion: 'A new version is available.', offlineReady: 'Emulators Collection is ready to work offline.',
    reload: 'Reload', later: 'Later', ok: 'OK', dismissNotice: 'Dismiss',
  },
  fr: {
    library: 'Bibliothèque', controls: 'Commandes', signOut: 'Déconnexion', playClassics: 'Jouez aux classiques',
    mainNavigation: 'Navigation principale', mobileNavigation: 'Navigation mobile', theme: 'Thème de couleur',
    systemTheme: 'Système', lightTheme: 'Clair', darkTheme: 'Sombre', language: 'Langue',
    english: 'English', french: 'Français', github: 'Voir le projet sur GitHub',
    retroCollection: 'Votre collection rétro', gameLibrary: 'Bibliothèque de jeux',
    libraryDescription: 'Votre collection personnelle, prête à jouer. Ajoutez vos ROM et lancez-les directement dans votre navigateur.',
    game: 'jeu', games: 'jeux', systemsSupported: 'systèmes pris en charge', closeUpload: 'Fermer l’ajout',
    addGame: 'Ajouter un jeu', browseGames: 'Parcourir les jeux', loadingCollection: 'Chargement de votre collection…',
    removingGame: 'Suppression du jeu…', showingGames: 'Affichage de {shown} sur {total} jeux',
    searchCollection: 'Rechercher dans votre collection…', searchGames: 'Rechercher des jeux', clearSearch: 'Effacer la recherche',
    sortGames: 'Trier les jeux', recentlyAdded: 'Ajouts récents', oldestFirst: 'Plus anciens',
    nameAZ: 'Nom A–Z', nameZA: 'Nom Z–A', filterBySystem: 'Filtrer par système',
    allSystems: 'Tous les systèmes', shown: '{count} affichés', noGamesFound: 'Aucun jeu trouvé',
    libraryWaiting: 'Votre bibliothèque vous attend', addRomStart: 'Ajoutez une ROM que vous possédez pour commencer. Formats acceptés : {formats}.',
    tryAnotherFilter: 'Essayez une autre recherche ou un autre filtre système.', addFirstGame: 'Ajouter votre premier jeu',
    resetFilters: 'Réinitialiser les filtres', deleteConfirm: 'Supprimer « {title} » de votre bibliothèque ?',
    gameRemoved: '« {title} » a été retiré de votre bibliothèque.', dismiss: 'Fermer',
    personalSetup: 'Configuration personnelle', controlsDescription: 'Configurez les touches de chaque système. Les modifications sont synchronisées avec votre compte.',
    chooseSystem: 'Choisir un système', keyboardLayout: 'configuration du clavier', actions: 'actions',
    clickKeyRemap: 'Cliquez sur une touche pour la modifier', restoreDefaults: 'Rétablir les valeurs par défaut',
    pressKeyFor: 'Appuyez sur une touche pour', escapeCancels: 'Échap pour annuler.',
    keyAlreadyAssigned: '{key} était déjà assignée à {action} ; les touches ont été échangées.',
    keyAssignmentSwapped: 'Une affectation de touche existante a été échangée.',
    savingKeyboard: 'Enregistrement des commandes…', keyboardSaved: 'Commandes enregistrées.',
    keyboardAutoSaved: 'Les commandes sont enregistrées automatiquement après chaque modification.',
    unassigned: 'Non assignée', remapKey: 'Modifier la touche de {action}',
    addGameTitle: 'Ajouter un jeu', closeAddGame: 'Fermer le formulaire d’ajout',
    chooseRom: 'Choisir un fichier ROM…', gameTitle: 'Titre du jeu', emulatorSystem: 'Système d’émulation',
    acceptedFormats: 'Formats acceptés : {formats}. Taille maximale : 16 Mo. Le fichier est stocké dans votre compte privé : vous seul pouvez y jouer.',
    uploading: 'Envoi…', add: 'Ajouter', playGame: 'Jouer', added: 'Ajouté le',
    deleting: 'Suppression de {title}', deleteGame: 'Supprimer {title}', externalRom: 'ROM externe',
    emulatorCollection: 'Collection d’émulateurs', unknownSystem: 'Système inconnu',
    controller: 'Manette', loading: 'Chargement…', returnLibrary: 'Retour à la bibliothèque',
    saveStateDownloaded: 'Sauvegarde téléchargée.', saveStateLoaded: 'Sauvegarde chargée avec succès.',
    saveStateLoadFailed: 'Impossible de charger cette sauvegarde.', unableFullscreen: 'Impossible de basculer en plein écran.',
    savingState: 'Chargement de la sauvegarde…', loadState: 'Charger une sauvegarde', saveState: 'Télécharger la sauvegarde',
    pause: 'Pause', resume: 'Reprendre', mute: 'Couper le son', unmute: 'Rétablir le son', fullscreen: 'Plein écran',
    exitFullscreen: 'Quitter le plein écran', volume: 'Volume', startGame: 'Lancer le jeu',
    newVersion: 'Une nouvelle version est disponible.', offlineReady: 'Emulators Collection est prêt à fonctionner hors ligne.',
    reload: 'Recharger', later: 'Plus tard', ok: 'OK', dismissNotice: 'Fermer',
  },
};

interface I18nContextValue {
  language: Language;
  setLanguage: (language: Language) => void;
  t: (key: string, values?: Record<string, string | number>) => string;
}

const I18nContext = createContext<I18nContextValue | null>(null);

function getInitialLanguage(): Language {
  try {
    const saved = localStorage.getItem('emulators-language');
    if (saved === 'en' || saved === 'fr') return saved;
  } catch {
    // Fall back to the browser language.
  }
  return navigator.language.toLowerCase().startsWith('fr') ? 'fr' : 'en';
}

export function I18nProvider({ children }: { children: ReactNode }) {
  const [language, setLanguage] = useState<Language>(getInitialLanguage);

  useEffect(() => {
    document.documentElement.lang = language;
    try {
      localStorage.setItem('emulators-language', language);
    } catch {
      // The selected language remains active for this session.
    }
  }, [language]);

  const value = useMemo<I18nContextValue>(
    () => ({
      language,
      setLanguage,
      t: (key, values = {}) => {
        const template = messages[language][key] ?? messages.en[key] ?? key;
        return template.replace(/\{(\w+)\}/g, (_, name: string) => String(values[name] ?? ''));
      },
    }),
    [language],
  );

  return <I18nContext.Provider value={value}>{children}</I18nContext.Provider>;
}

export function useI18n() {
  const context = useContext(I18nContext);
  if (!context) throw new Error('useI18n must be used within <I18nProvider>');
  return context;
}
