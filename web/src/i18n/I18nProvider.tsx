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
    signIn: 'Sign in', signUp: 'Sign up', createAccount: 'Create an account', resetPassword: 'Reset your password',
    username: 'Username', email: 'Email', emailAddress: 'Email address', password: 'Password',
    hidePassword: 'Hide password', showPassword: 'Show password', forgotPassword: 'Forgot password?',
    noAccount: 'No account? Sign up', hasAccount: 'Already have an account? Sign in', backSignIn: 'Back to sign in',
    usernameRule: 'Username: 3 to 20 characters (letters, numbers, _).', usernameTaken: 'This username is already taken.',
    tooManyRequests: 'Too many requests. Please wait a minute before trying again.',
    resetEmailSent: 'If an account exists for this email, a reset link has been sent.',
    accountCreated: 'Account created: check your email to confirm.',
    authFailed: 'Authentication request failed.', resetInstructions: "Enter the email linked to your account and we'll send you a link to choose a new password.",
    resendIn: 'Resend in {seconds}s', sendResetLink: 'Send reset link',
    chooseNewPassword: 'Choose a new password', checkingLink: 'Checking your link…',
    invalidResetLink: 'This reset link is invalid or has expired. Request a new one from the sign-in page.',
    newPassword: 'New password', confirmPassword: 'Confirm new password', updatePassword: 'Update password',
    passwordsMismatch: 'Passwords do not match.', unableUpdatePassword: 'Unable to update your password.',
    passwordUpdated: 'Password updated. Redirecting…',
    dismissError: 'Dismiss error', restart: 'Restart', toggleCrt: 'Toggle CRT filter',
    pauseGame: 'Pause game', resumeGame: 'Resume game', downloadSaveState: 'Download save state',
    loadSaveState: 'Load save state', muteAudio: 'Mute audio', unmuteAudio: 'Unmute audio',
    enterFullscreen: 'Enter fullscreen', touchInstructions: 'Use the on-screen buttons to play. Rotate your phone or use fullscreen for a bigger screen.',
    keyboardInstructions: 'The keys are configured in the « Controls » tab. Click on the screen if the keyboard does not respond. A controller works too: press any button to connect it.',
    newVersion: 'A new version is available.', offlineReady: 'Emulators Collection is ready to work offline.',
    reload: 'Reload', later: 'Later', ok: 'OK', dismissNotice: 'Dismiss',
    signIn: 'Sign in', createAccount: 'Create an account', resetPassword: 'Reset your password',
    username: 'Username', email: 'Email', password: 'Password', forgotPassword: 'Forgot password?',
    noAccount: 'No account? Sign up', haveAccount: 'Already have an account? Sign in',
    backToSignIn: 'Back to sign in', sendResetLink: 'Send reset link', resendIn: 'Resend in {seconds}s',
    usernameHint: '3 to 20 characters: letters, numbers, or _',
    usernameValidation: 'Username: 3 to 20 characters (letters, numbers, _).',
    usernameTaken: 'This username is already taken.',
    tooManyRequests: 'Too many requests. Please wait a minute before trying again.',
    resetEmailInfo: 'If an account exists for this email, a reset link has been sent.',
    accountCreated: 'Account created: check your email to confirm.',
    enterResetEmail: "Enter the email linked to your account and we'll send you a link to choose a new password.",
    chooseNewPassword: 'Choose a new password', checkingLink: 'Checking your link…',
    invalidResetLink: 'This reset link is invalid or has expired. Request a new one from the sign-in page.',
    newPassword: 'New password', confirmPassword: 'Confirm new password',
    updatePassword: 'Update password', passwordsMismatch: 'Passwords do not match.',
    passwordUpdated: 'Password updated. Redirecting…', unableUpdatePassword: 'Unable to update your password.',
    hidePassword: 'Hide password', showPassword: 'Show password',
    restartGame: 'Restart game', toggleCrt: 'Toggle CRT filter',
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
    signIn: 'Se connecter', signUp: 'Créer un compte', createAccount: 'Créer un compte', resetPassword: 'Réinitialiser le mot de passe',
    username: 'Nom d’utilisateur', email: 'E-mail', emailAddress: 'Adresse e-mail', password: 'Mot de passe',
    hidePassword: 'Masquer le mot de passe', showPassword: 'Afficher le mot de passe', forgotPassword: 'Mot de passe oublié ?',
    noAccount: 'Pas de compte ? Créer un compte', hasAccount: 'Déjà un compte ? Se connecter', backSignIn: 'Retour à la connexion',
    usernameRule: 'Nom d’utilisateur : 3 à 20 caractères (lettres, chiffres, _).', usernameTaken: 'Ce nom d’utilisateur est déjà utilisé.',
    tooManyRequests: 'Trop de tentatives. Veuillez patienter une minute avant de réessayer.',
    resetEmailSent: 'Si un compte existe pour cette adresse, un lien de réinitialisation a été envoyé.',
    accountCreated: 'Compte créé : consultez votre e-mail pour le confirmer.',
    authFailed: 'Échec de la requête d’authentification.', resetInstructions: 'Saisissez l’adresse e-mail associée à votre compte pour recevoir un lien de réinitialisation du mot de passe.',
    resendIn: 'Renvoyer dans {seconds}s', sendResetLink: 'Envoyer le lien',
    chooseNewPassword: 'Choisissez un nouveau mot de passe', checkingLink: 'Vérification du lien…',
    invalidResetLink: 'Ce lien est invalide ou a expiré. Demandez-en un nouveau depuis la page de connexion.',
    newPassword: 'Nouveau mot de passe', confirmPassword: 'Confirmer le mot de passe', updatePassword: 'Modifier le mot de passe',
    passwordsMismatch: 'Les mots de passe ne correspondent pas.', unableUpdatePassword: 'Impossible de modifier votre mot de passe.',
    passwordUpdated: 'Mot de passe modifié. Redirection…',
    dismissError: 'Fermer le message d’erreur', restart: 'Redémarrer', toggleCrt: 'Activer/désactiver le filtre CRT',
    pauseGame: 'Mettre le jeu en pause', resumeGame: 'Reprendre le jeu', downloadSaveState: 'Télécharger la sauvegarde',
    loadSaveState: 'Charger une sauvegarde', muteAudio: 'Couper le son', unmuteAudio: 'Rétablir le son',
    enterFullscreen: 'Passer en plein écran', touchInstructions: 'Utilisez les boutons à l’écran pour jouer. Tournez votre téléphone ou passez en plein écran pour agrandir l’affichage.',
    keyboardInstructions: 'Les touches sont configurées dans l’onglet « Commandes ». Cliquez sur l’écran si le clavier ne répond pas. Une manette fonctionne également : appuyez sur un bouton pour la connecter.',
    newVersion: 'Une nouvelle version est disponible.', offlineReady: 'Emulators Collection est prêt à fonctionner hors ligne.',
    reload: 'Recharger', later: 'Plus tard', ok: 'OK', dismissNotice: 'Fermer',
    signIn: 'Se connecter', createAccount: 'Créer un compte', resetPassword: 'Réinitialiser le mot de passe',
    username: 'Nom d’utilisateur', email: 'Adresse e-mail', password: 'Mot de passe', forgotPassword: 'Mot de passe oublié ?',
    noAccount: 'Pas de compte ? S’inscrire', haveAccount: 'Déjà un compte ? Se connecter',
    backToSignIn: 'Retour à la connexion', sendResetLink: 'Envoyer le lien de réinitialisation', resendIn: 'Renvoyer dans {seconds}s',
    usernameHint: '3 à 20 caractères : lettres, chiffres ou _',
    usernameValidation: 'Nom d’utilisateur : 3 à 20 caractères (lettres, chiffres, _).',
    usernameTaken: 'Ce nom d’utilisateur est déjà utilisé.',
    tooManyRequests: 'Trop de requêtes. Veuillez patienter une minute avant de réessayer.',
    resetEmailInfo: 'Si un compte existe pour cette adresse, un lien de réinitialisation a été envoyé.',
    accountCreated: 'Compte créé : consultez votre boîte mail pour confirmer votre adresse.',
    enterResetEmail: 'Saisissez l’adresse e-mail associée à votre compte pour recevoir un lien de réinitialisation.',
    chooseNewPassword: 'Choisir un nouveau mot de passe', checkingLink: 'Vérification du lien…',
    invalidResetLink: 'Ce lien est invalide ou a expiré. Demandez-en un nouveau depuis la page de connexion.',
    newPassword: 'Nouveau mot de passe', confirmPassword: 'Confirmer le nouveau mot de passe',
    updatePassword: 'Mettre à jour le mot de passe', passwordsMismatch: 'Les mots de passe ne correspondent pas.',
    passwordUpdated: 'Mot de passe mis à jour. Redirection…', unableUpdatePassword: 'Impossible de mettre à jour le mot de passe.',
    hidePassword: 'Masquer le mot de passe', showPassword: 'Afficher le mot de passe',
    restartGame: 'Redémarrer le jeu', toggleCrt: 'Activer/désactiver le filtre CRT',
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
