Structure SD attendue pour lancer Pokemon Bleu via l'emulateur GB.

Fichiers a copier manuellement:
- /games/PokemonBleu_GB/gb_emulator.bin    (binaire ESP32 de l'emulateur)
- /games/PokemonBleu_GB/roms/PokemonBleu.gb (ROM GB, usage legal uniquement)
- /games/PokemonBleu_GB/icon.raw           (50x50)
- /games/PokemonBleu_GB/title.raw          (320x240)

Le launcher charge gb_emulator.bin (champ "bin" de meta.json).
Le binaire emulateur lit ensuite boot.json pour trouver rom_path/save_path.

Ou trouver gb_emulator.bin ?
- Il n'est pas fourni dans ce depot.
- Tu dois compiler le projet emulateur GB avec Arduino IDE (ou arduino-cli).
- Un template compilable est fourni ici:
  RaptorLauncher_GameSDK/examples/GB_Emulator/gb_emulator.ino
- Dans ce depot, les jeux compiles sortent en general dans:
  <ProjetEmulateur>/build/esp32.esp32.esp32/game.bin
- Prends ce game.bin et renomme-le en gb_emulator.bin dans ce dossier.
- Important: n'utilise pas merged.bin / bootloader.bin / partitions.bin.
