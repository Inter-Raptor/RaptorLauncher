Dossier SD prêt pour un jeu GB via émulateur.

IMPORTANT:
1) Le launcher Raptor démarre uniquement un .bin (OTA), pas un .gb directement.
2) Copie ton émulateur compilé en:
   /games/PokemonBleu_GB/gb_emulator.bin
3) Copie ta ROM (si tu as le droit légal) en:
   /games/PokemonBleu_GB/roms/PokemonBleu.gb
4) Vérifie boot.json pour le chemin ROM utilisé par ton émulateur.
5) Ajoute icon.raw (50x50) et title.raw (320x240) si tu veux un rendu propre.

Ou trouver gb_emulator.bin ?
- Ce fichier n'est pas fourni dans le repo.
- Compile ton projet emulateur GB pour ESP32.
- Un template compilable est fourni ici:
  RaptorLauncher_GameSDK/examples/GB_Emulator/gb_emulator.ino
- Le binaire est souvent généré sous:
  <ProjetEmulateur>/build/esp32.esp32.esp32/game.bin
- Copie ce game.bin ici en le renommant gb_emulator.bin.
- Important: n'utilise pas merged.bin / bootloader.bin / partitions.bin.
