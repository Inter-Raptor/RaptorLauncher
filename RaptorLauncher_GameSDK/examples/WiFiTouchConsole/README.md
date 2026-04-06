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


## Compilation Arduino IDE (important)
Cet exemple inclut des fichiers pont (`raptor_game_sdk.cpp/.h` et `raptor_game_config.h`)
pour éviter l'erreur de link "undefined reference to `RaptorGameSDK::...`"
quand tu compiles directement `WiFiTouchConsole.ino`.

Tu peux donc ouvrir ce dossier tel quel dans Arduino IDE et compiler sans copier manuellement le SDK.

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

## Contrôles (100% tactile)
- **Onglet haut gauche**: Menu Wi‑Fi autour
- **Onglet haut centre**: Menu réseau perso
- **Bouton haut droite `Quitter`**: retour launcher
- **Barre action (ligne 2)**: lancer scan Wi‑Fi, se connecter, ou rescanner le LAN
- **Glisser verticalement** dans la zone de contenu: scroll longues listes
- **Bandeau rouge en bas** (quand visible): annuler une connexion ou un scan réseau en cours

## Note sur la découverte d'appareils
La détection est faite par tentative TCP (ports 80/443) sur le `/24` local.
Donc certains appareils peuvent ne pas apparaître s'ils n'écoutent pas ces ports.

## Anti-scintillement
Le rendu est maintenant **événementiel**: l'écran se redessine seulement quand quelque chose change (tap, scroll, nouveaux résultats), au lieu d'un refresh permanent.
