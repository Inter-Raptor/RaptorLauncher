# RaptorLauncher00

## Erreurs fréquentes (version courte)
- **Ne surchargez pas `setup()`** : initialisez seulement le matériel critique, puis laissez le reste non-bloquant dans `loop()`.
- **Toujours valider la SD avant lancement** : un jeu sans `game.bin` ou avec `meta.json` invalide doit être refusé proprement.
- **Assets visuels optionnels** : `home_bg.bmp` et `splash.bmp` doivent toujours avoir un fallback (pas de crash si absents/corrompus).
- **Sprites/images trop lourds** : surveiller RAM/PSRAM, surtout pendant les chargements BMP/RAW.
- **Ne pas masquer les erreurs série** : le moniteur série est la source principale de diagnostic terrain.

## Retours d'expérience (détaillé)

### 1) ParaRunner
- **Problème observé** : ralentissements/intermittences quand beaucoup de sprites sont manipulés en même temps.
- **Cause probable** : allocations fréquentes et redraw trop complet.
- **Solution recommandée** :
  - Précharger les ressources critiques.
  - Éviter les allocations répétées dans la boucle jeu.
  - Limiter les redraws aux zones réellement modifiées.

### 2) WiFiTouchConsole
- **Problème observé** : démarrage instable quand WiFi + init tactile + rendu lourd sont déclenchés trop tôt.
- **Cause probable** : pics CPU/mémoire au boot et délais bloquants dans `setup()`.
- **Solution recommandée** :
  - Garder `setup()` court.
  - Déporter les actions non critiques en tâches progressives dans `loop()`.
  - Ajouter des logs explicites avant/après chaque étape de boot.

### 3) Gestion sprites / performances
- **Problème observé** : frame drops et micro-freezes avec de nombreuses images ou redimensionnements.
- **Cause probable** : traitements graphiques coûteux en continu.
- **Solution recommandée** :
  - Préférer assets à la bonne résolution.
  - Éviter les conversions à chaud.
  - Garder une cadence stable (boucle courte, pas de délais longs).

### 4) `setup()` vs `loop()`
- **Problème observé** : crashes aléatoires quand "trop de choses" démarrent en même temps.
- **Cause probable** : contention mémoire/temps CPU au boot.
- **Solution recommandée** :
  - `setup()` : init minimale et robuste.
  - `loop()` : orchestration progressive/non-bloquante.
  - Sur erreur, toujours remonter un code/raison lisible au série.

### 5) Logique d'état launcher
- **Problème observé** : ambiguïtés UX (touches non évidentes, pagination confuse, retour page 1 implicite).
- **Cause probable** : zones tactiles trop proches ou feedback visuel insuffisant.
- **Solution recommandée** :
  - Définir des hitbox nettes.
  - Ajouter un feedback visuel instantané.
  - Afficher un indicateur de page clair (`1/3`) et un retour page 1 explicite.

## Historique / corrigé
Certaines remarques anciennes peuvent être partiellement obsolètes (ex. correctifs matériels/pins RGB déjà appliqués). Conserver ces points comme **historique**, et préciser l'état **corrigé/non corrigé** au fur et à mesure.

## Vérification rapide — RaptorLauncher_V0.9

### Fond d'écran de l'accueil (derrière les icônes)
- Dans `RaptorLauncher_V0.9`, l'écran d'accueil efface d'abord l'écran avec `displayClear()`, puis dessine les icônes de jeux.
- Il n'y a pas de chargement d'image de fond globale pour la grille de jeux (pas de `home_bg` lu depuis la SD dans `drawHomeScreen()`).
- Résultat : en l'état, la v0.9 n'affiche pas de fond d'écran derrière les icônes sur la page qui liste les jeux.

### Ordre d'affichage des jeux
- L'ordre est bien modifiable via `meta.json` de chaque jeu avec le champ `index` (ou `indice` en fallback).
- Le launcher trie d'abord par `index` croissant.
- En cas d'égalité d'`index`, il trie ensuite par nom (`name`) en ordre alphabétique.

### Action pratique
- Pour changer l'ordre, modifier `index` dans `CarteSD/games/<NomDuJeu>/meta.json`, puis redémarrer le launcher.

## Nouveaux réglages v0.9 (images RAW)

Dans `settings.json` à la racine de la SD :

```json
{
  "boot_splash_raw": "/boot.raw",
  "boot_splash_ms": 1500,
  "boot_splash_w": 320,
  "boot_splash_h": 240,
  "home_bg_raw": "/home_bg.raw",
  "home_bg_w": 320,
  "home_bg_h": 240
}
```

- `boot_splash_raw` : image RAW affichée au démarrage (si absente/invalide = démarrage classique).
- `boot_splash_ms` : durée d'affichage au boot en millisecondes (0 à 10000).
- `boot_splash_w` / `boot_splash_h` : dimensions EXACTES du fichier RAW de splash.
- `home_bg_raw` : fond RAW de l'écran d'accueil (les icônes et noms des jeux sont dessinés par-dessus).
- `home_bg_w` / `home_bg_h` : dimensions EXACTES du fichier RAW du fond d'accueil.
- Le bouton **Paramètres** de l'accueil est remplacé par une icône **roue dentée** dessinée en code (pas chargée depuis la SD).
- Si les dimensions ne correspondent pas au fichier RAW, l'image peut apparaître brouillée/cassée.
- L'affichage RAW détecte automatiquement l'endianness (LE/BE) pour éviter les couleurs corrompues.
