#pragma once
#include <Arduino.h>

#include "marche1.h"
#include "marche2.h"
#include "saut1.h"
#include "saut2.h"
#include "baisse_1.h"
#include "baisse_2.h"
#include "cactus1.h"
#include "cactus2.h"
#include "cactus3.h"
#include "ptera1.h"
#include "ptera2.h"
#include "coeur.h"

struct SpriteFrame {
  const uint16_t* pixels;
  uint16_t w;
  uint16_t h;
  uint16_t key;
};

static const SpriteFrame SPR_RUN_1    = { marche1,   marche1_W,   marche1_H,   marche1_KEY };
static const SpriteFrame SPR_RUN_2    = { marche2,   marche2_W,   marche2_H,   marche2_KEY };
static const SpriteFrame SPR_JUMP_1   = { saut1,     saut1_W,     saut1_H,     saut1_KEY };
static const SpriteFrame SPR_JUMP_2   = { saut2,     saut2_W,     saut2_H,     saut2_KEY };
static const SpriteFrame SPR_DUCK_1   = { baisse_1,  baisse_1_W,  baisse_1_H,  baisse_1_KEY };
static const SpriteFrame SPR_DUCK_2   = { baisse_2,  baisse_2_W,  baisse_2_H,  baisse_2_KEY };

static const SpriteFrame SPR_CACTUS_1 = { cactus1, cactus1_W, cactus1_H, cactus1_KEY };
static const SpriteFrame SPR_CACTUS_2 = { cactus2, cactus2_W, cactus2_H, cactus2_KEY };
static const SpriteFrame SPR_CACTUS_3 = { cactus3, cactus3_W, cactus3_H, cactus3_KEY };

static const SpriteFrame SPR_PTERA_1  = { ptera1, ptera1_W, ptera1_H, ptera1_KEY };
static const SpriteFrame SPR_PTERA_2  = { ptera2, ptera2_W, ptera2_H, ptera2_KEY };

static const SpriteFrame SPR_HEART    = { coeur, coeur_W, coeur_H, coeur_KEY };