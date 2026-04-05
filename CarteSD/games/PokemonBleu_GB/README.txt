Structure SD attendue pour lancer Pokemon Bleu via l'emulateur GB.

Fichiers a copier manuellement:
- /games/PokemonBleu_GB/gb_emulator.bin    (binaire ESP32 de l'emulateur)
- /games/PokemonBleu_GB/roms/PokemonBleu.gb (ROM GB, usage legal uniquement)
- /games/PokemonBleu_GB/icon.raw           (50x50)
- /games/PokemonBleu_GB/title.raw          (320x240)

Le launcher charge gb_emulator.bin (champ "bin" de meta.json).
Le binaire emulateur lit ensuite boot.json pour trouver rom_path/save_path.
