# RaptorLauncher Game SDK PRO (Arduino IDE)

Oui: ce kit est pensé pour être un **starter kit pro**, pas juste un exemple.
Il intègre les points critiques que tu as demandés:

1. **Retour launcher armé dès le démarrage du jeu** (boot safety).
2. **Sauvegarde standard dans `sauv.json`** dans le même dossier que le jeu.
3. **Contraintes de `meta.json` claires** (icône et titre avec tailles imposées).
4. **Pins/pilotes/libs préconfigurés** pour réduire au maximum la friction.

---

## 1) Structure du kit

- `arduino_template/`
  - `arduino_template.ino` : jeu exemple prêt à compiler
  - `src/raptor_game_config.h` : pins + conventions meta/save
  - `src/raptor_game_sdk.h/.cpp` : API simplifiée
- `sd_template/games/MonJeu/`
  - `meta.json` modèle compatible launcher
  - `assets/` pour tes ressources
- `docs/libraries_arduino_ide.txt`
  - liste des bibliothèques IDE Arduino

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
- `updateInputs()` : refresh boutons
- `isHeld/isPressed/isReleased`
- `clear/drawRect/fillRect/drawSmallText/drawCenteredText`
- `playBeep(freq, ms)`
- `requestReturnToLauncher()`
- `saveJson/loadJson/saveJsonPath`

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
- [ ] test bouton START (retour launcher)
- [ ] test écriture `sauv.json`

Ce kit te donne une base solide pour créer vite, proprement, et garder la compatibilité launcher.
