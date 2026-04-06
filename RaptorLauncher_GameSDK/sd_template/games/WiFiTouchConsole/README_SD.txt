Arborescence SD recommandee pour WiFiTouchConsole:

/games/WiFiTouchConsole/
├── meta.json
├── config.json
├── game.bin            <- binaire compile
├── icon.raw            <- 50x50 RGB565
├── title.raw           <- 320x240 RGB565
├── sauv.json           <- cree automatiquement au besoin
└── assets/             <- optionnel

Important:
- Le dossier sur SD doit correspondre au nom compile dans SDK_GAME_FOLDER_NAME.
- Si tu gardes SDK_GAME_FOLDER_NAME = "MonJeu", renomme le dossier en /games/MonJeu/
  ou adapte la macro avant compilation.


Wi-Fi (menu 2):
- Le jeu lit en priorite /settings.json (racine SD) avec wifi_ssid / wifi_pass.
- Un exemple est fourni dans RaptorLauncher_GameSDK/sd_template/settings.json
- /games/WiFiTouchConsole/config.json est seulement un fallback compatibilite.
