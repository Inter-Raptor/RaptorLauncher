# 📦 Dépendances Arduino — RaptorLauncher GameSDK

README en 2 parties :
- rapide pour comprendre la logique
- technique pour verrouiller la reproductibilité

---

## 🎉 Partie 1 — version simple

## Pourquoi on ne commit pas toutes les libs ?
Parce que ça alourdit le repo et crée vite des conflits de versions.

## Règle simple
- installer les dépendances via Arduino Library Manager
- garder ici la doc des libs validées

## Dépendances cœur
- LovyanGFX
- XPT2046_Touchscreen
- Adafruit_MCP23X17
- ArduinoJson
- Core ESP32 (Espressif)

## Optionnelles
- PNGdec
- JPEGDEC
- ESP8266Audio

---

## 🛠️ Partie 2 — version très technique

## A) Politique de dépendances

### A1 — Principe
Le repository versionne le code applicatif + docs, pas des copies complètes de libs tierces (sauf exception critique).

### A2 — Objectifs
- réduire poids Git
- simplifier merges/rebases
- faciliter montée de version contrôlée

## B) Installation recommandée

1. Installer le board package ESP32 (Espressif)
2. Installer les libs cœur
3. Ajouter optionnelles selon fonctionnalités activées (PNG/JPG/Audio)

## C) Verrouillage de versions (reproductibilité)

Créer `versions.lock.md` avec :
- nom de chaque lib
- version exacte
- version core ESP32
- version Arduino IDE
- date et contexte de validation

## D) Stratégie de debug dépendances

Quand un build casse après update :
1. comparer avec `versions.lock.md`
2. revenir à la dernière combinaison validée
3. retester `SDK_TestLab`
4. ne promouvoir une nouvelle version qu’après tests fonctionnels (rendu, input, save, audio)
