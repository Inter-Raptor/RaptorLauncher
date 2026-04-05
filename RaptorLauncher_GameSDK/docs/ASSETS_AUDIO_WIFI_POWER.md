# Guide SDK - Assets, Audio, Wi-Fi, Alimentation, SD

Ce document complète le SDK pour répondre aux besoins "console complète".

## 1) Images / Sprites

### A. Sprite en `.h` (recommandé pour petits assets UI)
- Convertir une image vers tableau C (`uint16_t[]` RGB565).
- Inclure le `.h` dans le jeu.
- Dessiner avec l'API bas niveau écran (ou helpers futurs).

Avantages:
- très rapide
- pas de lecture SD à runtime

Inconvénients:
- firmware plus gros

Helpers SDK disponibles:
- `assetPath("sprite.raw")`
- `drawRaw565(path, x, y, width, height)`
- `drawBmp(path, x, y)`
- `drawPng(path, x, y)` (si support PNG actif)

### B. BMP depuis SD
- Utiliser des BMP si tu veux de la simplicité.
- Stocker dans `/games/<NomJeu>/assets/`.

### C. PNG depuis SD
- Utile pour compression/transparence.
- Plus coûteux CPU/RAM selon lib de décodage.

## 2) Audio

SDK de base:
- `playBeep(freq, ms)` = effets simples (UI, collision, score).

Audio avancé (module optionnel):
- WAV mono (préféré pour effets courts)
- MP3 (plutôt musiques/voix)

Wrappers SDK disponibles:
- `playWav(path)`
- `playMp3(path)`
- `hasAdvancedAudio()`

Recommandation:
- Commencer par beep + WAV mono.
- Garder MP3 en option pour ne pas complexifier les jeux simples.

## 3) Wi-Fi (optionnel)

Le SDK peut lire dans `/settings.json`:
- `wifi_enabled`
- `wifi_ssid`
- `wifi_pass`

API:
- `wifiConnectFromSettings()`
- `wifiIsConnected()`
- `wifiDisconnect()`

Contrainte:
- Aucun Wi-Fi lancé si `wifi_enabled=false` ou SSID vide.

## 4) LED RGB

API:
- `setLedRgb(r,g,b)`
- `ledOff()`

Usage recommandé:
- feedback UI léger (menu, erreur, succès)
- éviter animations lourdes pendant gameplay si FPS critique

## 5) Luminosité (LDR)

API:
- `readLightRaw()`
- `readLightPercent()`

Important:
- selon révision de carte, le LDR peut être influencé par le rétroéclairage écran.
- valeur à considérer comme indicateur, pas mesure scientifique.

## 6) Batterie / alimentation

Règle pratique:
- Alimentation dev standard: USB/port principal 5V prévu.
- Batterie: utiliser le connecteur batterie dédié de la carte.
- Injection directe 3V3: réservé aux cas experts (alimentation propre/régulée, sans logique de charge).

Le comportement charge batterie dépend de la révision exacte de carte et du PMIC monté.
Toujours vérifier le schéma ou la fiche de ta révision matérielle.

Helpers SDK disponibles (si pin batterie configurée):
- `hasBatterySense()`
- `batteryMilliVolts()`
- `batteryPercent()`

## 7) Politique SD (sandbox jeu)

Un jeu doit lire/écrire uniquement dans:
- `/games/<NomJeu>/...`

Exception contrôlée:
- lecture de `/settings.json` (paramètres globaux launcher)

Recommandation forte:
- ne jamais modifier les dossiers d'autres jeux.
- ne jamais écrire hors dossier jeu sauf besoin système explicite.

## 8) Espace SD

Il n'existe pas de quota matériel "par jeu" imposé par dossier.
Chaque jeu doit respecter une discipline logicielle et limiter ses assets.

Le SDK fournit actuellement:
- `isSdReady()`
- validation `meta.json`
- chemins standards de save.

