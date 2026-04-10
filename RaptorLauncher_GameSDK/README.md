# 🧰 RaptorLauncher Game SDK PRO (Arduino IDE)

Ce SDK te permet de créer des jeux compatibles RaptorLauncher avec un cadre stable.  
Doc en **2 niveaux** :
- **Partie 1** : démarrage rapide, fun, sans friction.
- **Partie 2** : détails techniques complets (API, contraintes, workflow pro).

> Source de vérité API publique : `arduino_template/src/raptor_game_sdk.h`

---

## 🎉 Partie 1 — Démarrage rapide (fun & simple)

## 1) Ce que fait le SDK
- te donne une API de rendu, input, save, Wi‑Fi, etc.
- uniformise le comportement entre jeux
- protège la compatibilité launcher

## 2) Les dossiers clés
- `arduino_template/` : base de ton jeu
- `sd_template/` : structure SD modèle
- `docs/API_PUBLIC.md` : fonctions réellement exposées
- `examples/SDK_TestLab/` : validation de référence

## 3) Les 4 étapes pour lancer un jeu
1. Installe le core ESP32 + libs listées
2. Copie `arduino_template/` dans ton dossier sketch
3. Mets le bon `SDK_GAME_FOLDER_NAME`
4. Compile, flash, place `game.bin` + `meta.json` sur la SD

## 4) Astuce sécurité
Le SDK arme automatiquement le retour launcher au prochain boot via `sdk.begin()` (boot safety).

---

## 🧪 Partie 2 — Détails techniques complets

## A) Arborescence du kit (rôle de chaque bloc)

- `arduino_template/`
  - `arduino_template.ino` : point d’entrée sketch
  - `src/raptor_game_sdk.h/.cpp` : API + implémentation
  - `src/raptor_game_config.h` : constantes projet (dossier jeu, pins, options)
- `docs/API_PUBLIC.md` : index API officiel
- `docs/ASSETS_AUDIO_WIFI_POWER.md` : détails assets/audio/réseau/power
- `sd_template/games/MonJeu/` : structure SD compatible launcher
- `examples/SDK_TestLab/` : exemple de non-régression fonctionnelle
- `dependencies/README.md` : stratégie dépendances

## B) API publique — périmètre réel

### B1 — Fonctions cœur disponibles
- rendu 2D basique (`clear`, `drawRect`, `fillRect`, texte)
- input boutons MCP23017
- input tactile (`touchX`, `touchY`, pressed/released)
- rendu raw (`drawRaw565`)
- bip (`playBeep`)
- persistance (`saveJson`, `loadJson`, `saveJsonPath`)
- LED RGB
- capteur lumière
- batterie (si ADC configuré)
- Wi‑Fi via `settings.json`

### B2 — Fonctions optionnelles selon build/libs
- `drawBmp`, `drawJpg`, `drawPng`
- `playWav`, `playMp3`

> Elles peuvent exister dans l’API mais retourner `false` si le support n’est pas compilé ou dépendances absentes.

## C) Négociation de capacités (obligatoire avant appel optionnel)

Vérifie systématiquement :
- `hasBmpSupport()`
- `hasJpgSupport()`
- `hasPngSupport()`
- `hasWavSupport()`
- `hasMp3Support()`

Exemple recommandé :

```cpp
if (sdk.hasPngSupport()) {
  (void)sdk.drawPng(sdk.assetPath("overlay.png"), 0, 0);
} else {
  // fallback simple (texte / raw)
}
```

## D) Conventions launcher imposées par le SDK

### D1 — Sauvegarde
Chemin standard :
`/games/<SDK_GAME_FOLDER_NAME>/sauv.json`

### D2 — `meta.json`
Conventions attendues :
- `icon_w=50`, `icon_h=50`
- `title_w=320`, `title_h=240`
- `save="sauv.json"`
- `bin="game.bin"`

### D3 — Calibration tactile
Le SDK lit `/settings.json` pour :
- `touch_x_min/max`, `touch_y_min/max`
- `touch_offset_x/y`

Fallback sur macros de `raptor_game_config.h` si fichier absent/invalide.

## E) Installation et build (procédure complète)

1. Installer **ESP32 by Espressif** (Board Manager).
2. Installer les libs de `docs/libraries_arduino_ide.txt`.
3. Copier `arduino_template/` vers `MonJeu/`.
4. Vérifier la structure :

```text
MonJeu/
├── MonJeu.ino
└── src/
    ├── raptor_game_sdk.h
    ├── raptor_game_sdk.cpp
    └── raptor_game_config.h
```

5. Renseigner `SDK_GAME_FOLDER_NAME`.
6. Compiler.
7. Produire `game.bin`.
8. Créer dossier SD `/games/MonJeu/` + `meta.json` + assets.

## F) Contrat de test avant release

### F1 — Fonctionnel
- boot jeu sans freeze
- START/quit retour launcher
- tactile cohérent avec calibration
- save/load JSON validé

### F2 — Robustesse
- comportement si asset manquant
- comportement si `settings.json` absent
- comportement si fonction optionnelle indisponible

### F3 — Performance
- pas de redraw intégral permanent
- pas de charges lourdes en `setup()`
- logs série exploitables

## G) Documentation visuelle (images/GIF)

Si tu ajoutes des images/GIF par écran/realm :
- garde les médias proches du README concerné,
- ajoute un caption “ce qu’on voit / ce qu’on teste”,
- indique si le visuel correspond à 2432S022, 2432S028 ou ILI9341.

---

## ✅ Checklist rapide finale
- [ ] API utilisée = déclarée dans `raptor_game_sdk.h`
- [ ] `meta.json` conforme
- [ ] fallback si optionnel absent
- [ ] save path correct
- [ ] tests manuels validés
