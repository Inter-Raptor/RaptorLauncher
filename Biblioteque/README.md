# Bibliothèques du projet

Ce dossier contient les bibliothèques Arduino embarquées dans le dépôt pour garder des versions compatibles.

## Nettoyage effectué
Les bibliothèques suivantes ont été retirées car elles ne sont pas utilisées par les projets actifs (`RaptorLauncher_V3`, `RaptorLauncher_V4`, `RaptorLauncher_GameSDK`) ou faisaient doublon :

- `Arduboy`
- `Arduboy2`
- `MCP23017` (doublon avec `Adafruit_MCP23017_Arduino_Library`)
- `PCD8544`
- `Adafruit_PCD8544_Nokia_5110_LCD_library`

## Nom du dossier
`Biblioteque/` fonctionne, mais si tu veux un nom plus standard et plus clair pour d'autres développeurs, tu peux renommer ce dossier en :

- `libraries/` (recommandé)
- ou `arduino-libraries/`

> Important : si tu renommes le dossier, pense à mettre à jour tes chemins dans l'IDE/PlatformIO.
