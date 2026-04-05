# RaptorLauncher Game SDK PRO (Arduino IDE)

Oui: ce kit est pensé pour être un **starter kit pro**, pas juste un exemple.
Il intègre les points critiques que tu as demandés:

1. **Retour launcher armé dès le démarrage du jeu** (boot safety).
2. **Sauvegarde standard dans `sauv.json`** dans le même dossier que le jeu.
3. **Contraintes de `meta.json` claires** (icône et titre avec tailles imposées).
4. **Pins/pilotes/libs préconfigurés** pour réduire au maximum la friction.
5. **Support tactile exposé dans l'API** (position + états press/release).

---

## 1) Structure du kit

- `docs/ASSETS_AUDIO_WIFI_POWER.md`
  - guide complet assets/audio/wifi/luminosite/batterie/sd
- `docs/API_PUBLIC.md`
  - liste exacte des fonctions réellement exposées dans le SDK
- `arduino_template/`
  - `arduino_template.ino` : jeu exemple prêt à compiler
  - `src/raptor_game_config.h` : pins + conventions meta/save
  - `src/raptor_game_sdk.h/.cpp` : API simplifiée
- `sd_template/games/MonJeu/`
  - `meta.json` modèle compatible launcher
  - `assets/` pour tes ressources
- `docs/libraries_arduino_ide.txt`
  - liste des bibliothèques IDE Arduino
- `dependencies/README.md`
  - stratégie recommandée pour gérer les dépendances sans alourdir le repo

---

## 2) Règles PRO déjà prévues

### A. Boot safety launcher
Au `sdk.begin()`, le SDK appelle automatiquement `armReturnToLauncherOnNextBoot()`.
Donc même si le jeu crash/reboot, le prochain boot repart sur le launcher (si le label launcher a été mémorisé par le launcher).

### B. Sauvegarde JSON standard
Le chemin de save est construit automatiquement:

`/games/<SDK_GAME_FOLDER_NAME>/sauv.json`

Fonctions prêtes:
- `sdk.saveJson(doc)`
- `sdk.loadJson(doc)`
- `sdk.saveJsonPath()`

### C. Meta.json contraint
Par convention de ce kit:
- `icon_w = 50`, `icon_h = 50`
- `title_w = 320`, `title_h = 240`
- `save = "sauv.json"`
- `bin = "game.bin"`

### D. Calibration tactile depuis le launcher
Le SDK lit automatiquement `/settings.json` (même fichier que le launcher) pour récupérer:
- `touch_x_min`, `touch_x_max`, `touch_y_min`, `touch_y_max`
- `touch_offset_x`, `touch_offset_y`

Si le fichier est absent/invalide, le SDK retombe sur les macros de `raptor_game_config.h`.

---

## 3) Installation rapide

1. Installer la carte **ESP32 by Espressif**.
2. Installer les libs de `docs/libraries_arduino_ide.txt`.
3. Copier `arduino_template/` dans ton dossier de sketch.
4. Mettre le bon nom de dossier jeu dans `SDK_GAME_FOLDER_NAME` (`src/raptor_game_config.h`).
5. Compiler/flash.

---

## 4) API utile

- `begin()` : init hardware + armement retour launcher
- `updateInputs()` : refresh boutons + tactile
- `isHeld/isPressed/isReleased`
- `isTouchHeld()` ou alias `isTouching()`
- `isTouchPressed()/isTouchReleased()`
- `touchX()/touchY()`
- `clear/drawRect/fillRect/drawSmallText/drawCenteredText`
- `drawRaw565(path, x, y, w, h)`
- `drawBmp(path, x, y)`
- `drawPng(path, x, y)`
- `playBeep(freq, ms)`
- `playWav(path)` / `playMp3(path)` (si libs audio avancées installées)
- `requestReturnToLauncher()`
- `saveJson/loadJson/saveJsonPath`
- `assetPath(filename)`
- `isSdReady()`
- `loadLauncherSettings(doc)`
- `validateGameMeta(path, errorOut)`
- `sdkHealthReport()`
- `setLedRgb(r,g,b)` / `ledOff()`
- `readLightRaw()` / `readLightPercent()`
- `hasBatterySense()` / `batteryMilliVolts()` / `batteryPercent()`
- `hasPngDecoder()` / `hasAdvancedAudio()`
- `wifiConnectFromSettings()` / `wifiIsConnected()` / `wifiDisconnect()`

---

## 5) Exemple meta.json (obligatoire)

```json
{
  "name": "Mon Jeu",
  "author": "Ton Nom",
  "description": "Mon premier jeu compatible RaptorLauncher",
  "type": "mixed",
  "icon": "icon.raw",
  "icon_w": 50,
  "icon_h": 50,
  "title": "title.raw",
  "title_w": 320,
  "title_h": 240,
  "bin": "game.bin",
  "save": "sauv.json"
}
```

---

## 6) Checklist avant test

- [ ] dossier SD: `/games/MonJeu/`
- [ ] `meta.json` présent
- [ ] `game.bin` présent
- [ ] `icon.raw` en **50x50**
- [ ] `title.raw` en **320x240**
- [ ] nom de dossier aligné avec `SDK_GAME_FOLDER_NAME`
- [ ] `settings.json` launcher présent (optionnel mais recommandé)
- [ ] test bouton START (retour launcher)
- [ ] test tactile (coordonnées et déplacement)
- [ ] test écriture `sauv.json`

Ce kit te donne une base solide pour créer vite, proprement, et garder la compatibilité launcher.


> Note: `drawBmp/drawPng/playWav/playMp3` sont exposes dans le SDK.
> Leur execution depend des capacites compilees (`hasPngDecoder()`, `hasAdvancedAudio()`).


### Batterie
Par defaut `SDK_PIN_BATTERY_ADC = -1` (desactive).
Renseigne la vraie pin ADC de ta revision carte dans `raptor_game_config.h` pour activer `batteryMilliVolts()` et `batteryPercent()`.
