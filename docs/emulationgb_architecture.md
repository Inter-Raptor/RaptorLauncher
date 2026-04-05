# emulationGB sur RaptorLauncher (ESP32)

Ce document décrit l'architecture minimale pour intégrer un mode `emulationGB` sans frontend multi-émulateur.

## Validation architecture

Architecture validée pour un ESP32 :

1. `launcher` en `factory`.
2. `gb_runner.bin` dans le slot OTA jeu (`ota_0`).
3. ROM externe sur SD (`.gb/.gbc`) et jamais copiée en flash programme.
4. Chemin de ROM transmis avant reboot via un petit fichier JSON sur SD (`/.gb_launch.json`).

Cette approche garde un moteur unique, remplaçable, avec un coût mémoire prévisible.

## Transmission du chemin ROM : NVS vs fichier SD

### Recommandation
Utiliser un **fichier JSON atomique sur SD** pour le chemin de ROM (approche implémentée côté launcher).

### Pourquoi
- lisible/déboguable depuis un PC,
- supporte des chemins longs sans contrainte stricte,
- très simple à versionner (ajout futur de flags : scaling, mute, profil manette),
- évite de mélanger stockage système NVS et catalogue jeux.

### Format proposé

```json
{
  "rom": "/emulationGB/Tetris/tetris.gb",
  "game": "Tetris",
  "updated_at_ms": 123456
}
```

## Cœur d'émulation recommandé

- **Peanut-GB** : bon candidat léger, intégration C simple, footprint raisonnable.
- Démarrage conseillé : mode DMG puis extension GBC optionnelle.

## Affichage ILI9341 (objectif FPS stable)

- Résolution source 160x144.
- Mode par défaut recommandé : **x2 centré** (320x288 recadré verticalement, ou letterbox selon UX).
- Pipeline conseillé :
  - framebuffer GB 16bpp en RAM,
  - conversion palette -> RGB565,
  - push SPI par lignes/blocs (DMA si dispo dans votre driver).

## Mémoire

- ROM : privilégier lecture SD + cache banque (MBC) plutôt qu'un chargement complet systématique.
- Framebuffer : 160x144x2 = 46080 octets.
- Éviter les copies intermédiaires ; écrire directement dans un buffer de rendu.

## Intégration launcher (état du code)

Le launcher supporte maintenant :

- scan des jeux classiques (`/games/*/meta.json`) et `emulationGB` (`/emulationGB/*/game.json`),
- type `emulationGB` avec champ `rom`,
- sauvegarde du chemin ROM sélectionné avant lancement dans `/.gb_launch.json`.

## Étape suivante recommandée

Créer un projet `gb_runner` séparé qui :

1. initialise écran/boutons/SD,
2. lit `/.gb_launch.json`,
3. ouvre la ROM cible,
4. boucle `input -> emulateFrame -> renderFrame`.

