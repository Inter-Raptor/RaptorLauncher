# 🦖 JurassicLife — Guide code (FR)

Ce guide est en 2 niveaux :
- **Partie 1** : prise en main simple et sympa
- **Partie 2** : référence technique complète, étape par étape

---

## 🎉 Partie 1 — prise en main simple

## 1) Les 2 lignes à modifier en premier
En haut du sketch :
- `DISPLAY_PROFILE`
- `ENABLE_AUDIO`

Puis tu téléverses. C’est le chemin minimal.

## 2) Contrôles physiques (si tu veux)
- soit encodeur rotatif
- soit 3 boutons

Garde les pins inutilisées à `-1`.

## 3) Sauvegarde persistante
Si tu veux garder la progression après extinction : carte microSD obligatoire.

---

## 🛠️ Partie 2 — référence technique complète

## A) Profils d’affichage supportés
- `DISPLAY_PROFILE_2432S022`
- `DISPLAY_PROFILE_2432S028`
- `DISPLAY_PROFILE_ILI9341_320x240`

Structure macro à conserver (ne pas renommer la partie gauche) :

```cpp
#define DISPLAY_PROFILE DISPLAY_PROFILE_2432S022
```

## B) Activation audio

```cpp
#define ENABLE_AUDIO 1
```

Comportement :
- `0` = chemins audio inactifs
- `1` = contrôles audio visibles + actifs

## C) Stratégie de mapping des entrées

Le sketch utilise généralement des blocs de config compile-time (`#if DISPLAY_PROFILE ...`).

Règles :
1. `-1` => entrée désactivée
2. Mode encodeur : renseigner `ENC_A`, `ENC_B`, `ENC_BTN` (optionnel), laisser `BTN_*=-1`
3. Mode boutons : mettre `ENC_*=-1`, définir `BTN_LEFT/BTN_OK/BTN_RIGHT`
4. Éviter mode mixte sauf implémentation explicite

## D) Notes électriques
- Selon la pin/carte, un pull-up peut être nécessaire.
- Si rebonds mécaniques: activer/renforcer le debouncing logiciel.

## E) UI selon la carte
- 2432S028 offre plus d’espace d’affichage que 2432S022.
- Des différences de layout sont donc normales.

## F) Modèle de sauvegarde
- Sauvegarde persistante après reboot/coupure = microSD requise.
- Sans SD, l’état reste volatile.

## G) Checklist de validation recommandée
- bon profil sélectionné
- tactile correctement mappé
- mode d’entrée physique cohérent avec pins définies
- bouton audio visible uniquement quand attendu
- sauvegarde intacte après reboot avec SD
- écrans conformes aux captures/GIF de documentation
