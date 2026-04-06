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
