# 📶 WiFiTouchConsole

Exemple SDK "outil réseau tactile".  
Doc en 2 parties : rapide puis technique.

---

## 🎉 Partie 1 — version simple

## À quoi sert ce projet ?
- voir les Wi‑Fi autour
- se connecter au réseau perso
- scanner les appareils du LAN
- garder un historique sur SD

## Contrôles rapides
- onglets haut : M1/M2/M3/M4
- `Quit` : retour launcher
- glisser vertical : scroll
- bandeau rouge : annuler action en cours

## Fichiers SD à avoir
Dans `/games/WiFiTouchConsole/` :
- `meta.json`, `config.json`, `game.bin`, `icon.raw`, `title.raw`
- `sauv.json` sera créé automatiquement

---

## 🛠️ Partie 2 — version très technique

## A) Structure SD détaillée

```text
/games/WiFiTouchConsole/
├── meta.json
├── config.json
├── game.bin
├── icon.raw
├── title.raw
├── sauv.json
└── assets/
```

Templates :
- `RaptorLauncher_GameSDK/sd_template/games/WiFiTouchConsole/`
- `RaptorLauncher_GameSDK/sd_template/settings.json`

Contrainte : nom dossier SD == `SDK_GAME_FOLDER_NAME` compilé.

## B) Build Arduino IDE

Ce dossier inclut des fichiers pont :
- `raptor_game_sdk.cpp/.h`
- `raptor_game_config.h`

But : éviter `undefined reference to RaptorGameSDK::...` en compile directe.

## C) Menus et responsabilités

- **M1 WiFi** : scan SSID/RSSI/canal/chiffrement
- **M2 LAN** : connexion Wi‑Fi + découverte hôtes locaux
- **M3 WiFiH** : historique réseaux vus (min/max/dernier)
- **M4 DevH** : historique devices vus (min/max/dernier)

## D) Paramètres Wi‑Fi

Source utilisée : `/settings.json` (racine SD)  
Champs attendus : `wifi_ssid`, `wifi_pass`.

Au tap `Se connecter`, tentative forcée avec ces valeurs.

## E) Mécanisme de découverte LAN

Pipeline en 2 étages sur `/24` :
1. stimulation ARP indirecte via UDP
2. fallback TCP multi-ports (80, 443, 53, 22, 445, 139, 1883, 554, 8008)

Limites connues : AP isolation/VLAN/firewall peuvent masquer certains hôtes.

## F) Anti-freeze / anti-scintillement

- rendu événementiel uniquement
- scan Wi‑Fi asynchrone
- scan LAN progressif en `loop()`
- feedback d’état immédiat côté UI

## G) Historique persistant

Fichiers générés :
- `seen_wifi.log`
- `seen_devices.log`
- `seen_wifi_stats.json`
- `seen_devices_stats.json`

Ces fichiers alimentent M3/M4.
