#pragma once
#include "sprites.h"

void resetPlayer();
void updateAnimation();
void updatePlayerInput();
void updatePlayerPhysics();
void updateScoreAndSpeed();
void getPlayerHitbox(int& x, int& y, int& w, int& h);
const SpriteFrame& currentPlayerSprite();
void loseOneLife();