#include "obstacles.h"
#include "game_state.h"
#include "sprites.h"
#include "player.h"

static bool overlap(int ax, int ay, int aw, int ah, int bx, int by, int bw, int bh) {
  return (ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by);
}

void resetObstacles() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    obstacles[i].active = false;
  }
}

void spawnObstacle() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) {
      obstacles[i].active = true;
      obstacles[i].counted = false;
      obstacles[i].x = SCREEN_W + random(0, 24);

      if (random(0, 100) < 68) {
        obstacles[i].type = OBS_CACTUS;
        obstacles[i].variant = random(0, 3);

        const SpriteFrame* s = &SPR_CACTUS_1;
        if (obstacles[i].variant == 1) s = &SPR_CACTUS_2;
        if (obstacles[i].variant == 2) s = &SPR_CACTUS_3;

        obstacles[i].w = s->w;
        obstacles[i].h = s->h;
        obstacles[i].y = GROUND_TOP_Y - s->h + 1;
      } else {
        obstacles[i].type = OBS_PTERA;
        obstacles[i].variant = random(0, 2);

        const SpriteFrame* s = (obstacles[i].variant == 0) ? &SPR_PTERA_1 : &SPR_PTERA_2;
        obstacles[i].w = s->w;
        obstacles[i].h = s->h;

        // plus haut pour que l'accroupi serve vraiment
        int band = random(0, 3);
        if      (band == 0) obstacles[i].y = 106;
        else if (band == 1) obstacles[i].y = 122;
        else                obstacles[i].y = 138;
      }
      return;
    }
  }
}

void updateObstacles() {
  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) continue;

    obstacles[i].x -= gameSpeed;

    if (!obstacles[i].counted && (obstacles[i].x + obstacles[i].w < PLAYER_X)) {
      obstacles[i].counted = true;
      score += 1;
    }

    if (obstacles[i].x + obstacles[i].w < -4) {
      obstacles[i].active = false;
    }
  }

  if (millis() - lastSpawnMs >= nextSpawnDelayMs) {
    spawnObstacle();
    lastSpawnMs = millis();
    nextSpawnDelayMs = 1750 + random(250, 1000);
  }
}

void checkCollisions() {
  if (millis() < invulnUntil) return;

  int px, py, pw, ph;
  getPlayerHitbox(px, py, pw, ph);

  for (int i = 0; i < MAX_OBSTACLES; i++) {
    if (!obstacles[i].active) continue;

    int ox = (int)obstacles[i].x;
    int oy = (int)obstacles[i].y;
    int ow = obstacles[i].w;
    int oh = obstacles[i].h;

    if (obstacles[i].type == OBS_CACTUS) {
      ox += 8;
      oy += 6;
      ow -= 16;
      oh -= 8;
    } else {
      ox += 4;
      oy += 3;
      ow -= 8;
      oh -= 6;
    }

    if (overlap(px, py, pw, ph, ox, oy, ow, oh)) {
      loseOneLife();
      obstacles[i].active = false;
      return;
    }
  }
}