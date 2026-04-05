# GB_Emulator (Peanut-GB)

Ce sketch embarque un vrai coeur d'emulation Game Boy DMG via **Peanut-GB**.

## Ce que fait ce binaire
- Charge `boot.json` depuis `/games/PokemonBleu_GB/boot.json`
- Charge la ROM `.gb` depuis `rom_path`
- Lance l'emulation frame par frame (`gb_run_frame`)
- Affiche l'image GB 160x144 sur l'ecran
- Charge/sauvegarde la SRAM cartouche vers `save_path`

## Build (Arduino IDE)
1. Ouvrir `gb_emulator.ino`.
2. Verifier que `GB_GAME_FOLDER` correspond au dossier SD cible.
3. Compiler pour ESP32.
4. Recuperer `game.bin` genere (souvent sous `build/esp32.esp32.esp32/game.bin`).
5. Copier et renommer en `/games/PokemonBleu_GB/gb_emulator.bin`.

## Controles
- D-pad, A, B, START, SELECT mappes vers la manette GB
- `START + SELECT` : sauvegarde puis retour launcher
- `A + B` : sauvegarde manuelle + beep

## Limites
- DMG (Game Boy classic) uniquement.
- Audio GB non active dans ce template (`ENABLE_SOUND 0`).
