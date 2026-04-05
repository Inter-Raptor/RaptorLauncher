# API publique réellement exposée par `RaptorGameSDK`

Ce fichier sert de référence pour éviter tout décalage entre doc et code.

**Source de vérité :** `arduino_template/src/raptor_game_sdk.h`

---

## 1) SDK de base

### I/O jeu
- `begin()`
- `updateInputs()`
- `isHeld(...)`, `isPressed(...)`, `isReleased(...)`

### Tactile
- `isTouchHeld()` / `isTouching()`
- `isTouchPressed()` / `isTouchReleased()`
- `touchX()` / `touchY()`

### Affichage
- `clear(...)`
- `drawRect(...)`
- `fillRect(...)`
- `drawSmallText(...)`
- `drawCenteredText(...)`
- `drawRaw565(path, x, y, w, h)`

### Audio de base
- `playBeep(freq, ms)`

### Boot/launcher
- `armReturnToLauncherOnNextBoot()`
- `requestReturnToLauncher()`

### Stockage / chemins
- `gameRootPath()`
- `saveJsonPath()`
- `assetPath(filename)`
- `saveJson(...)`
- `loadJson(...)`
- `isSdReady()`
- `loadLauncherSettings(...)`
- `validateGameMeta(...)`
- `sdkHealthReport()`

### LED / capteurs / réseau
- `setLedRgb(r,g,b)`
- `ledOff()`
- `readLightRaw()`
- `readLightPercent()`
- `hasBatterySense()`
- `batteryMilliVolts()`
- `batteryPercent()`
- `wifiConnectFromSettings()`
- `wifiDisconnect()`
- `wifiIsConnected()`

---

## 2) SDK optionnel (selon build/libs)

Ces fonctions sont déclarées dans l'API publique pour garantir la compilation. Elles peuvent retourner `false` selon les capacités compilées.

- `drawBmp(path, x, y)`
- `drawPng(path, x, y)`
- `playWav(path)`
- `playMp3(path)`

### Fonctions de capacité
- `hasBmpSupport()`
- `hasPngSupport()`
- `hasWavSupport()`
- `hasMp3Support()`

### Compatibilité historique
- `hasPngDecoder()`
- `hasAdvancedAudio()`

---

## 3) Tableau de support réel

| Fonction | État |
|---|---|
| `drawRaw565` | OK |
| `drawBmp` | Partiel : appel API dispo, dépend du build LovyanGFX + SD |
| `drawPng` | Partiel : appel API dispo, retourne `false` si décodeur PNG absent |
| `drawJpg` | Non implémenté (pas encore exposé dans l'API) |
| `playBeep` | OK |
| `playWav` | Partiel : appel API dispo, retourne `false` sans audio avancé |
| `playMp3` | Partiel : appel API dispo, retourne `false` sans audio avancé |
| `saveJson/loadJson` | OK |
| `wifi*` | OK (si paramètres présents) |
| `battery*` | OK (si pin ADC batterie configurée) |
| `touch*` | OK |

## 4) Fonctions absentes / non prévues dans cette version

- `drawJpg(...)` n'est pas encore disponible.
- GIF/WebP non supportés.
- Pas de lecture audio asynchrone/streaming dans l'API publique actuelle.
