# API publique réellement exposée par `RaptorGameSDK`

Ce fichier sert de référence rapide pour éviter tout décalage entre doc et code.

## I/O jeu
- `begin()`
- `updateInputs()`
- `isHeld(...)`, `isPressed(...)`, `isReleased(...)`

## Tactile
- `isTouchHeld()` / `isTouching()`
- `isTouchPressed()` / `isTouchReleased()`
- `touchX()` / `touchY()`

## Affichage
- `clear(...)`
- `drawRect(...)`
- `fillRect(...)`
- `drawSmallText(...)`
- `drawCenteredText(...)`
- `drawRaw565(path, x, y, w, h)`
- `drawBmp(path, x, y)`
- `drawPng(path, x, y)`

## Audio
- `playBeep(freq, ms)`
- `playWav(path)`
- `playMp3(path)`

## Boot/launcher
- `armReturnToLauncherOnNextBoot()`
- `requestReturnToLauncher()`

## Stockage / chemins
- `gameRootPath()`
- `saveJsonPath()`
- `assetPath(filename)`
- `saveJson(...)`
- `loadJson(...)`
- `isSdReady()`
- `loadLauncherSettings(...)`
- `validateGameMeta(...)`
- `sdkHealthReport()`

## LED / capteurs / réseau
- `setLedRgb(r,g,b)`
- `ledOff()`
- `readLightRaw()`
- `readLightPercent()`
- `hasBatterySense()`
- `batteryMilliVolts()`
- `batteryPercent()`
- `hasPngDecoder()`
- `hasAdvancedAudio()`
- `wifiConnectFromSettings()`
- `wifiDisconnect()`
- `wifiIsConnected()`
