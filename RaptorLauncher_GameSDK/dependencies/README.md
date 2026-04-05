# Dépendances Arduino - RaptorLauncher GameSDK

Ce dossier remplace l'idée d'un dossier `bibliotheque/` avec des copies complètes de libs.

## Pourquoi
Versionner des bibliothèques entières dans le repo alourdit fortement le projet et crée vite des conflits.

## Stratégie recommandée
- Garder ici uniquement **la liste des dépendances validées**.
- Installer les libs via l'IDE Arduino (Library Manager).
- Ne pas commiter les sources complètes des libs externes, sauf besoin exceptionnel.

## Dépendances nécessaires (SDK de base)
- LovyanGFX
- XPT2046_Touchscreen
- Adafruit MCP23017 (Adafruit_MCP23X17)
- ArduinoJson
- ESP32 board package (Espressif)

## Dépendances optionnelles (features avancées)
- PNGdec (PNG custom)
- JPEGDEC (JPEG custom)
- ESP8266Audio (WAV/MP3 avancé)

## Remarque
Si tu veux figer des versions exactes, ajoute un fichier `versions.lock.md` dans ce dossier
avec les numéros de version testés dans ton environnement.
