# GB_Emulator (Peanut-GB)

Ce sketch embarque un vrai coeur d'emulation Game Boy DMG via **Peanut-GB**.

## Ce que fait ce binaire
- Charge `boot.json` depuis `/games/PokemonBleu_GB/boot.json`
- Charge la ROM `.gb` depuis `rom_path`
- Lance l'emulation frame par frame (`gb_run_frame`)
- Affiche l'image GB 160x144 sur l'ecran
- Charge/sauvegarde la SRAM cartouche vers `save_path`

## Build (Arduino IDE)
1. Copier le dossier `GB_Emulator/` complet.
2. Ouvrir `gb_emulator.ino`.
3. Le sketch est autonome: le dossier contient deja `raptor_game_sdk.*`, `raptor_game_config.h` et `peanut_gb.h`.
4. Verifier que `GB_GAME_FOLDER` correspond au dossier SD cible.
5. Compiler pour ESP32.
6. Recuperer `game.bin` genere (souvent sous `build/esp32.esp32.esp32/game.bin`).
7. Copier et renommer en `/games/PokemonBleu_GB/gb_emulator.bin`.

## Depannage si le launcher revient immediatement
- Verifie le nom du dossier jeu : `PokemonBleu_GB` (fallback accepte: `pokemon_gb`).
- Verifie que `meta.json` pointe vers le bon binaire (`gb_emulator.bin` ou `game.bin` selon ton choix).
- Utilise le **binaire applicatif** (`*.ino.bin` ou `game.bin`), pas `merged.bin`, pas `bootloader.bin`, pas `partitions.bin`.
- Verifie `boot.json` et la ROM:
  - `/games/<dossier>/boot.json`
  - `/games/<dossier>/roms/PokemonBleu.gb`

## Controles
- D-pad, A, B, START, SELECT mappes vers la manette GB
- `START + SELECT` : sauvegarde puis retour launcher
- `A + B` : sauvegarde manuelle + beep

## Limites
- DMG (Game Boy classic) uniquement.
- Audio GB non active dans ce template (`ENABLE_SOUND 0`).
