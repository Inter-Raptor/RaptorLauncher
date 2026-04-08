# RaptorLauncher Game SDK PRO (Arduino IDE)

Ce dépôt vise un principe simple : **API publique, documentation et implémentation doivent rester alignées**.

La **source de vérité** est :
- `arduino_template/src/raptor_game_sdk.h`

Si une fonction n'est pas déclarée dans ce fichier, elle ne doit pas être présentée comme disponible.

---

## 1) Structure du kit

- `docs/ASSETS_AUDIO_WIFI_POWER.md`
  - guide complet assets/audio/wifi/luminosité/batterie/sd
- `docs/API_PUBLIC.md`
  - liste exacte des fonctions exposées + distinction core/optionnel
- `arduino_template/`
  - `arduino_template.ino` : template jeu prêt à compiler
  - `src/raptor_game_config.h` : pins + conventions meta/save
  - `src/raptor_game_sdk.h/.cpp` : API publique et implémentation
- `examples/SDK_TestLab/`
  - sketch de validation officiel du SDK (compile = cohérence API)
- `sd_template/games/MonJeu/`
  - `meta.json` modèle compatible launcher
  - `assets/` pour tes ressources
- `docs/libraries_arduino_ide.txt`
  - liste des bibliothèques IDE Arduino
- `dependencies/README.md`
  - stratégie recommandée pour gérer les dépendances sans alourdir le repo

---

## 2) Niveaux de support

### SDK de base (disponible)
- affichage simple (`clear`, `drawRect`, `fillRect`, texte)
- boutons (MCP23017)
- tactile (`touchX/touchY`, pressed/released)
- `drawRaw565(...)`
- `playBeep(...)`
- JSON save/load (`saveJson/loadJson`)
- LED RGB
- capteur luminosité
- batterie (si ADC configuré)
- Wi‑Fi via `settings.json`
- boot safety / retour launcher

### SDK optionnel selon build
- `drawBmp(...)`
- `drawJpg(...)`
- `drawPng(...)`
- `playWav(...)`
- `playMp3(...)`

Ces appels existent toujours dans l'API publique, mais leur réussite dépend des capacités compilées.

---

## 3) Fonctions de capacité (à vérifier avant usage optionnel)

- `hasBmpSupport()`
- `hasJpgSupport()`
- `hasPngSupport()`
- `hasWavSupport()`
- `hasMp3Support()`

Exemple recommandé :

```cpp
if (sdk.hasPngSupport()) {
  (void)sdk.drawPng(sdk.assetPath("overlay.png"), 0, 0);
}
```

---

## 4) Tableau de support réel (état actuel)

| Fonction | État |
|---|---|
| `drawRaw565` | OK |
| `drawBmp` | OK (si fichier présent sur SD) |
| `drawJpg` | OK (si fichier présent sur SD) |
| `drawPng` | OK (si fichier présent sur SD) |
| `playBeep` | OK |
| `playWav` | OK si lib audio installée; sinon `false` |
| `playMp3` | OK si lib audio installée; sinon `false` |
| `saveJson/loadJson` | OK |
| `wifi*` | OK (si paramètres présents) |
| `battery*` | OK (si pin ADC batterie configurée) |
| `touch*` | OK |

### Limites connues
- GIF/WebP non supportés.
- Les wrappers audio WAV/MP3 sont bloquants (lecture synchrone).

### ⚠️ Erreurs fréquentes à éviter (version courte)
- Ne pas faire de redraw complet à chaque frame (risque de scintillement).
- Éviter les boucles de rendu pixel-par-pixel (`fillRect(1x1)`), coûteuses en performances.
- Ne pas lancer de tâches lourdes en `setup()` (scan réseau, traitements bloquants).
- Caler tôt la physique + hitbox (stand/duck/jump) pour garder un gameplay cohérent.
- Centraliser pins/constantes/timings pour faciliter debug et équilibrage.
- Utiliser le moniteur série pour diagnostiquer vite (SD, meta, bin, mémoire, réseau).
- Préférer un rendu événementiel: redraw uniquement quand l'état change.

### Retours d'expérience DEV (version détaillée)
- **Scintillement UI**
  - Cause: redraw complet en boucle.
  - Solution: rendu événementiel (redraw seulement quand l'état change).
- **Chutes de performances**
  - Cause: rendu pixel-par-pixel massif (`fillRect(1x1)` en doubles boucles).
  - Solution: réduire ces zones, regrouper les dessins, pré-calculer ce qui peut l'être.
- **Freeze / retour launcher au boot**
  - Cause: opérations lourdes en `setup()`.
  - Solution: setup minimal, puis traitement progressif en `loop()` (asynchrone/étagé).
- **Collisions ou sensations de saut incohérentes**
  - Cause: physique et hitbox réglées trop tard.
  - Solution: verrouiller tôt `GRAVITY`, `JUMP_VELOCITY`, positions Y et presets hitbox.
- **Debug difficile**
  - Cause: constantes dispersées et manque de logs.
  - Solution: centraliser les paramètres clés + logs série simples (`spawn/hit/despawn/state`).

---

## 5) Règles PRO prévues

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

## 6) Installation rapide

1. Installer la carte **ESP32 by Espressif**.
2. Installer les libs de `docs/libraries_arduino_ide.txt`.
3. Copier `arduino_template/` dans ton dossier de sketch.
4. Garder la structure **officielle unique** suivante :

```text
MonJeu/
├── MonJeu.ino
└── src/
    ├── raptor_game_sdk.h
    ├── raptor_game_sdk.cpp
    └── raptor_game_config.h
```

5. Mettre le bon nom de dossier jeu dans `SDK_GAME_FOLDER_NAME` (`src/raptor_game_config.h`).
6. Compiler/flash.

---

## 7) Exemple meta.json (obligatoire)

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

## 8) Checklist avant test

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
- [ ] test exemple `examples/SDK_TestLab`

Ce kit te donne une base solide pour créer vite, proprement, et garder la compatibilité launcher.
