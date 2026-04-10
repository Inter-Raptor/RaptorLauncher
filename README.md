# 🦖 RaptorLauncher00 — Le repaire des jeux dinos

Bienvenue ! Ici tu peux lancer des jeux sur ESP32 depuis la SD, avec une UX propre, des sauvegardes stables et un debug sérieux.  
Ce README est volontairement en **2 niveaux** :

- **Partie 1 (Fun & simple)** 👉 pour démarrer vite sans se noyer.
- **Partie 2 (Ultra technique)** 👉 pour comprendre *tous* les détails, les limites et les bonnes pratiques pro.

---

## 🎉 Partie 1 — Version fun et simple

## 1) Le concept en 20 secondes
RaptorLauncher = un menu tactile qui lit des dossiers de jeux sur la carte SD.  
Chaque jeu doit avoir des fichiers clés (`meta.json`, `game.bin`, visuels, save).

Si un fichier est absent, on ne lance pas le jeu.  
Si un jeu plante, on veut un retour launcher propre au boot suivant.

## 2) Les 5 règles d’or (anti galère)
1. **`setup()` court** : init minimale seulement.
2. **`loop()` intelligente** : tâches lourdes en mode progressif/non bloquant.
3. **SD validée** avant lancement (`meta.json` + `game.bin`).
4. **Fallback visuel** si image absente/corrompue (`home_bg.bmp`, `splash.bmp`).
5. **Logs série** activés pendant le dev.

## 3) Aperçu visuel (images + GIF)

> Tu me l’as demandé : voici les visuels directement dans le README principal ✅

### 🚀 Lancement du launcher
![Animation lancement launcher](presentation%20Lancement%20Launcher.gif)

### 🧩 Pages applications
![Page 1 applications](page1%20des%20application.jpg)
![Page 2 applications](page2%20des%20application.jpg)

### ⚙️ Paramètres
![Affichage des paramètres](affichage%20des%20setting%20parametre.jpg)

### 🎮 Présentation jeux / applications
![Présentation jeux et applications](Presentation%20de%20queljeu%20et%20application.gif)

## 4) Ta mini checklist avant test
- [ ] le launcher démarre sans freeze
- [ ] la pagination est claire et tactile
- [ ] un jeu invalide est refusé proprement
- [ ] retour launcher fonctionne après reboot forcé
- [ ] pas de redraw complet en boucle inutile

## 5) Résumé “ce qui casse le plus souvent”
- trop de choses lancées dans `setup()`
- grosses images mal dimensionnées
- scans réseau bloquants
- peu de logs quand ça part en vrille

---

## 🛠️ Partie 2 — Version très technique (détaillée)

## A) Pipeline de boot recommandé

### Étape A1 — Init critique (dans `setup()`)
- init écran + I/O minimum
- init stockage SD
- afficher un état de boot explicite
- **ne pas** lancer les jobs longs ici

### Étape A2 — Armement sécurité launcher
- activer le mécanisme de retour launcher “prochain boot”
- utile contre crash watchdog / reboot inattendu

### Étape A3 — Passage en orchestration `loop()`
- scanner, décoder, charger progressif
- éviter les `delay()` longs et appels bloquants

## B) Validation stricte d’un dossier jeu

Un dossier jeu valide doit fournir au minimum :
- `meta.json`
- `game.bin`

Fichiers usuels recommandés :
- `icon.raw`
- `title.raw`
- `sauv.json` (généré ou pré-existant selon jeu)

### Contrôles `meta.json` recommandés
- champs texte non vides (`name`, `author`)
- dimensions cohérentes (`icon_w/h`, `title_w/h`)
- noms de fichiers référencés réellement présents
- format JSON strict (pas de trailing comma)

## C) Rendu graphique et performances

### C1 — Éviter le scintillement
- utiliser un rendu événementiel
- redessiner uniquement les zones invalidées
- éviter refresh plein écran à chaque frame

### C2 — Budget mémoire
- surveiller RAM/PSRAM avant/après chargements
- préférer assets à résolution native écran
- limiter les conversions runtime (BMP/JPG/PNG -> RAW)

### C3 — Boucle de rendu
- cadence stable
- pas de double boucle pixel si pas nécessaire
- grouper les opérations de dessin par couche logique

## D) UX launcher (navigation tactile)

- hitbox séparées et lisibles
- feedback visuel immédiat au tap
- indicateur de page (`1/3`, `2/3`, ...)
- transitions simples, prévisibles et rapides

## E) Journalisation / Debug terrain

Logs minimum utiles :
- état SD (montage OK/KO)
- parse JSON OK/KO + code erreur
- chargement assets (succès/échec + taille)
- état mémoire avant/après action lourde
- transitions d’état UI (menu/page/selection)

## F) Gestion des erreurs (stratégie)

- **Erreur non critique** (asset manquant) : fallback visuel + log
- **Erreur critique** (binaire absent/corrompu) : refus lancement + message clair
- **Erreur système** (reset/crash) : retour launcher armé

## G) Historique des retours dev (synthèse technique)

### ParaRunner
- ralentissements causés par redraw trop global + allocations répétées
- fix : préchargement + redraw local

### WiFiTouchConsole
- boot instable quand Wi‑Fi + tactile + rendu lourd démarrent ensemble
- fix : setup minimal + tâches réseau progressives en loop

### Cas général
- collisions/logique gameplay à verrouiller tôt pour éviter désynchronisations
- constantes critiques centralisées pour faciliter tuning et maintenance

---

## 📌 Convention de maintenance docs

Pour chaque changement important, documenter :
1. **Quoi** (fonction, écran, pipeline)
2. **Pourquoi** (bug, perf, UX, compat)
3. **Impact** (launcher, jeux existants, assets)
4. **Statut** (`corrigé`, `à surveiller`, `non corrigé`)

---

Amuse-toi bien, et fais rugir les dinos… sans faire rugir le watchdog 🦕🔥
