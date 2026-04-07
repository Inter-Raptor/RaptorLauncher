# WiFiTouchConsole

Exemple SDK "pas un jeu" pour piloter un mini outil réseau en tactile.

## Oui, tu peux le mettre sur la carte SD
Le dossier du jeu doit être présent sur la SD dans `/games/<nom_dossier>/` avec `meta.json`, `game.bin`, `icon.raw`, `title.raw` et `config.json`.

Un template prêt à copier est maintenant fourni ici:

`RaptorLauncher_GameSDK/sd_template/games/WiFiTouchConsole/`

Un template de `settings.json` est fourni dans:

`RaptorLauncher_GameSDK/sd_template/settings.json`

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
- **Menu 1 (M1 WiFi)**: scanner tous les Wi-Fi à portée (SSID, puissance RSSI, canal, chiffrement).
- **Menu 2 (M2 LAN)**: se connecter au réseau perso via `/settings.json` puis lister les appareils détectés sur le sous-réseau local.
- **Menu 3 (M3 WiFiH)**: historique des reseaux Wi-Fi vus (meme apres disparition) avec dernier signal + min/max + dernier "vu".
- **Menu 4 (M4 DevH)**: historique des appareils vus sur ton reseau perso avec dernier "vu" + min/max du signal lien Wi-Fi (RSSI AP).
- Scroll **tactile vertical** (glisser haut/bas) pour parcourir de longues listes.

## Wi-Fi depuis /settings.json (racine SD)
Le menu 2 lit d'abord les identifiants dans le fichier global du launcher:

`/settings.json`

Champs attendus:

```json
{
  "wifi_ssid": "MonWifi",
  "wifi_pass": "MonMotDePasse"
}
```

Le menu 2 utilise uniquement `/settings.json` (meme format que le launcher).
Lors d'un tap sur **Se connecter**, le jeu lit `wifi_ssid`/`wifi_pass` et force la tentative de connexion (meme si `wifi_enabled` est a `false`).

## Contrôles (100% tactile)
- **Onglets hauts**: M1 WiFi / M2 LAN / M3 WiFiH / M4 DevH
- **Bouton haut droite `Quit`**: retour launcher
- **Barre action (ligne 2)**: lancer scan Wi‑Fi, se connecter, ou rescanner le LAN
- **Glisser verticalement** dans la zone de contenu: scroll longues listes
- **Tap sur un Wi‑Fi / appareil**: affiche plus de details dans le bandeau du bas
- **Bandeau rouge en bas** (quand visible): annuler une connexion ou un scan réseau en cours

## Note sur la découverte d'appareils
La détection est faite en 2 etapes sur le `/24` local:
1) sonde ARP indirecte (emission UDP pour forcer la resolution ARP),
2) fallback par tentatives TCP multi-ports (80, 443, 53, 22, 445, 139, 1883, 554, 8008).

Important: ce n'est toujours **pas** une liste absolue de tous les clients connectés au routeur.
Le scan marque aussi un hote comme probable actif si la connexion echoue tres vite (heuristique RST/ICMP).
Malgre ca, certains appareils peuvent encore ne pas apparaitre (isolation AP, VLAN, pare-feu, etc.).

## Anti-scintillement
Le rendu est maintenant **événementiel**: l'écran se redessine seulement quand quelque chose change (tap, scroll, nouveaux résultats), au lieu d'un refresh permanent.


## Stabilite launcher
- Le premier scan Wi-Fi est lance apres le boot (pas dans `setup()`).
- Le scan Wi-Fi est non bloquant (asynchrone).
- Le scan LAN est progressif (par etapes dans `loop()`), pour eviter les freezes et retours launcher.


## UX connexion
- Quand tu tapes **Se connecter**, l'etat passe immediatement a `Connexion...` (feedback direct).
- Le jeu reste tactile pendant l'attente et tu peux annuler via le bandeau du bas.


## Fichiers historiques sur SD
Le jeu enregistre automatiquement dans:
- `/games/WiFiTouchConsole/seen_wifi.log` (journal brut)
- `/games/WiFiTouchConsole/seen_devices.log` (journal brut)
- `/games/WiFiTouchConsole/seen_wifi_stats.json` (resume persistant: min/max/dernier vu)
- `/games/WiFiTouchConsole/seen_devices_stats.json` (resume persistant: min/max/dernier vu)

Chaque scan met a jour les journaux + les resumes JSON. Ces historiques sont consultables dans M3 et M4.
