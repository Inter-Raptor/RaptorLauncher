# ParaRunner — Guide de tuning (style Dino Google)

Si ton runner est **mou**, dans 90% des cas c'est un souci de:

1. **Physique mal calibrée** (vitesse de saut / gravité / hitbox)
2. **Game loop instable** (delta time irrégulier)
3. **Rendu plein écran à chaque frame** (clignotement, scintillement)

---

## 1) Réglages gameplay conseillés (point de départ)

Utilise des unités en pixels/seconde, avec un `dt` en secondes.

```cpp
// Mouvement monde
float worldSpeed = 210.0f;      // px/s (départ)
float worldAccel = 6.0f;        // px/s² (progression difficulté)

// Saut
float jumpVelocity = -430.0f;   // impulsion initiale (haut = négatif)
float gravity = 1450.0f;        // chute plus rapide que montée = sensation nerveuse
float maxFallSpeed = 700.0f;

// Hitbox dino (plus petite que le sprite pour un jeu "juste")
Rect hitRun   = {x+6,  y+8, 28, 34};
Rect hitDuck  = {x+8,  y+20, 30, 20};

// Collision obstacles
Rect cactusHit = {cx+3, cy+2, cw-6, ch-3};
Rect pteraHit  = {px+4, py+6, pw-8, ph-10};
```

### Effet attendu
- Saut plus franc et plus haut.
- Retombée rapide (jeu dynamique, style Chrome Dino).
- Canard plus efficace sous les ptéras.
- Collisions moins frustrantes.

---

## 2) Boucle de jeu stable (anti-lenteur)

Évite la logique dépendante de `delay()`.

```cpp
uint32_t now = millis();
float dt = (now - prevMs) * 0.001f;
prevMs = now;
if (dt > 0.033f) dt = 0.033f; // clamp anti freeze

// Physique
vy += gravity * dt;
if (vy > maxFallSpeed) vy = maxFallSpeed;
y += vy * dt;

// Sol
if (y >= groundY) {
  y = groundY;
  vy = 0;
  isJumping = false;
}

// Vitesse monde
worldSpeed += worldAccel * dt;
```

---

## 3) Saut plus réactif (feeling important)

Ajoute ces 3 mécaniques:

- **Jump buffer** (input avant atterrissage accepté ~100 ms)
- **Coyote time** (saut autorisé ~80 ms après quitter le sol)
- **Short hop** (si bouton relâché tôt, coupe la montée)

Exemple simplifié:

```cpp
if (jumpReleased && vy < -120.0f) {
  vy = -120.0f; // petit saut si relâchement rapide
}
```

---

## 4) Ptéra: hauteur et fenêtre de canard

Le problème "on ne se baisse pas assez" vient souvent d'un ennemi trop bas.

Recommandation:
- 3 couloirs de vol: **haut / milieu / bas**
- Le couloir "bas" doit passer juste au-dessus du dino baissé, avec une marge de 2–4 px.
- N'utiliser le couloir "bas" qu'après quelques secondes de jeu.

---

## 5) Clignotement / scintillement: ne pas tout redraw

### À éviter
- `fillScreen(...)` + redraw complet à chaque frame.

### À faire
- Redessiner uniquement les zones sales (**dirty rectangles**):
  - ancienne position dino
  - nouvelle position dino
  - ancienne/nouvelle position obstacles
  - zone HUD modifiée
- Dessiner le fond d'abord, puis sprites, puis HUD.
- Cadencer à ~60 FPS (`frame budget` ≈ 16 ms).

Pseudo-ordre:

```cpp
restoreBackground(oldDinoRect);
for each obstacle oldRect: restoreBackground(oldRect);

updatePhysics(dt);
updateObstacles(dt);

drawObstacle(newRect);
drawDino(newRect);
drawHudIfNeeded();
```

---

## 6) Check-list debug rapide

- Le dino saute-t-il à au moins **1.4×** sa hauteur de hitbox run ?
- Le mode canard réduit-il vraiment la hitbox en hauteur ?
- Les collisions utilisent-elles la **hitbox** et non le sprite complet ?
- La vitesse monde augmente-t-elle graduellement ?
- Le rendu évite-t-il le full-screen redraw ?

---

## 7) Profil cible "Dino-like" prêt à tester

- `jumpVelocity = -430`
- `gravity = 1450`
- `worldSpeed = 210`
- `worldAccel = 6`
- `coyoteTime = 80 ms`
- `jumpBuffer = 100 ms`
- `shortHopCut = -120`

Teste ce preset 5 minutes avant d'ajuster plus finement.
