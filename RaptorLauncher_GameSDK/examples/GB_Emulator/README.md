# GB_Emulator (template compilable)

Ce sketch genere un binaire ESP32 utilisable comme `gb_emulator.bin` pour valider le pipeline launcher + SD.

Il ne contient pas encore un coeur d'emulation Game Boy complet.
Il charge `boot.json`, verifie la presence de la ROM `.gb`, puis affiche un ecran de statut.

## Build (Arduino IDE)
1. Ouvrir `gb_emulator.ino`.
2. Verifier que `GB_GAME_FOLDER` correspond au dossier SD cible.
3. Compiler pour ESP32.
4. Recuperer `game.bin` genere (souvent sous `build/esp32.esp32.esp32/game.bin`).
5. Copier et renommer en `/games/PokemonBleu_GB/gb_emulator.bin`.

## Controle
- `A` : beep test
- `START` : retour launcher
