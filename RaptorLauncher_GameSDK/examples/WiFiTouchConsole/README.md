# WiFiTouchConsole

Exemple SDK "pas un jeu" pour piloter un mini outil réseau en tactile.

## Oui, tu peux le mettre sur la carte SD
Le dossier du jeu doit être présent sur la SD dans `/games/<nom_dossier>/` avec `meta.json`, `game.bin`, `icon.raw`, `title.raw` et `config.json`.

Un template prêt à copier est maintenant fourni ici:

`RaptorLauncher_GameSDK/sd_template/games/WiFiTouchConsole/`

## Arborescence SD complète (attendue)

```text
/games/WiFiTouchConsole/
├── meta.json
├── config.json
├── game.bin
├── icon.raw
├── title.raw
├── sauv.json            (créé automatiquement)
└── assets/
```

> Important: le nom du dossier SD doit correspondre à `SDK_GAME_FOLDER_NAME` au moment de la compilation.

## Fonctionnalités
- **Menu 1**: scanner tous les Wi-Fi à portée (SSID, puissance RSSI, canal, chiffrement).
- **Menu 2**: se connecter au réseau perso via `config.json` puis lister les appareils détectés sur le sous-réseau local.
- Scroll **tactile vertical** (glisser haut/bas) pour parcourir de longues listes.

## config.json
Place ce fichier sur la SD dans le dossier du jeu:

`/games/<SDK_GAME_FOLDER_NAME>/config.json`

Exemple:

```json
{
  "ssid": "MonWifi",
  "password": "MonMotDePasse"
}
```

## Contrôles
- `X`: Menu Wi-Fi autour
- `Y`: Menu réseau perso
- `A`: action (scan ou connexion + découverte appareils)
- `B`: annuler pendant certains scans/connexions
- `START`: retour launcher

## Note sur la découverte d'appareils
La détection est faite par tentative TCP (ports 80/443) sur le `/24` local.
Donc certains appareils peuvent ne pas apparaître s'ils n'écoutent pas ces ports.
