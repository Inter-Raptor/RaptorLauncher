#pragma once
#include <Arduino.h>

#define BB_RAINBOW_COUNT 8

enum ColorPattern : uint8_t {
  CP_LEFT_TO_RIGHT = 0,     // gauche -> droite
  CP_TOP_TO_BOTTOM,         // haut -> bas
  CP_DIAG_DOWN,             // diagonale descendante
  CP_DIAG_UP,               // diagonale montante
  CP_CHECKER,               // damier
  CP_DOUBLE_VERTICAL,       // bandes verticales doubles
  CP_DOUBLE_HORIZONTAL,     // bandes horizontales doubles
  CP_CENTER,                // depuis le centre
  CP_FRAME,                 // cadre exterieur vers interieur
  CP_SPIRAL                 // pseudo spirale
};

// Palette arc-en-ciel RGB565
static const uint16_t RAINBOW_COLORS[BB_RAINBOW_COUNT] PROGMEM = {
  0xF800, // rouge
  0xFD20, // orange
  0xFFE0, // jaune
  0x07E0, // vert
  0x07FF, // cyan
  0x001F, // bleu
  0x781F, // violet
  0xF81F  // rose / magenta
};

// Motif couleur par niveau
static const uint8_t LEVEL_COLOR_PATTERNS[10] PROGMEM = {
  CP_LEFT_TO_RIGHT,     // niveau 1
  CP_TOP_TO_BOTTOM,     // niveau 2
  CP_DIAG_DOWN,         // niveau 3
  CP_DIAG_UP,           // niveau 4
  CP_CHECKER,           // niveau 5
  CP_DOUBLE_VERTICAL,   // niveau 6
  CP_DOUBLE_HORIZONTAL, // niveau 7
  CP_CENTER,            // niveau 8
  CP_FRAME,             // niveau 9
  CP_SPIRAL             // niveau 10
};

// Couleur plateforme par niveau
static const uint16_t LEVEL_PADDLE_COLORS[10] PROGMEM = {
  0xFFFF, // 1 blanc
  0x07FF, // 2 cyan
  0x87F0, // 3 vert clair
  0x781F, // 4 violet
  0xFD20, // 5 orange
  0xF800, // 6 rouge
  0x001F, // 7 bleu
  0xFFE0, // 8 jaune
  0xC618, // 9 gris clair
  0xD7FF  // 10 blanc bleuté
};

// Couleur bille par niveau
static const uint16_t LEVEL_BALL_COLORS[10] PROGMEM = {
  0xFD20, // 1 orange
  0xF800, // 2 rouge
  0xFFE0, // 3 jaune
  0x07FF, // 4 cyan
  0x5DFF, // 5 bleu clair
  0x07E0, // 6 vert
  0xF81F, // 7 rose
  0x781F, // 8 violet
  0x87FF, // 9 cyan clair
  0xF800  // 10 rouge vif
};

static inline uint16_t bbRainbowColor(uint8_t index) {
  return pgm_read_word(&RAINBOW_COLORS[index % BB_RAINBOW_COUNT]);
}

static inline uint8_t bbGetPattern(uint8_t level) {
  return pgm_read_byte(&LEVEL_COLOR_PATTERNS[level % 10]);
}

static inline uint16_t bbGetPaddleColor(uint8_t level) {
  return pgm_read_word(&LEVEL_PADDLE_COLORS[level % 10]);
}

static inline uint16_t bbGetBallColor(uint8_t level) {
  return pgm_read_word(&LEVEL_BALL_COLORS[level % 10]);
}

// Calcule l'index de couleur arc-en-ciel d'une brique selon le niveau, la ligne et la colonne
static inline uint8_t bbGetBrickColorIndex(uint8_t level, uint8_t row, uint8_t col) {
  const uint8_t pattern = bbGetPattern(level);

  switch (pattern) {
    case CP_LEFT_TO_RIGHT:
      return col % BB_RAINBOW_COUNT;

    case CP_TOP_TO_BOTTOM:
      return row % BB_RAINBOW_COUNT;

    case CP_DIAG_DOWN:
      return (row + col) % BB_RAINBOW_COUNT;

    case CP_DIAG_UP:
      return (uint8_t)((col + BB_RAINBOW_COUNT - (row % BB_RAINBOW_COUNT)) % BB_RAINBOW_COUNT);

    case CP_CHECKER:
      return (uint8_t)(((row * 2) + col) % BB_RAINBOW_COUNT);

    case CP_DOUBLE_VERTICAL:
      return (uint8_t)((col / 2) % BB_RAINBOW_COUNT);

    case CP_DOUBLE_HORIZONTAL:
      return (uint8_t)((row / 2) % BB_RAINBOW_COUNT);

    case CP_CENTER: {
      int cx = 4; // centre approx pour 10 colonnes
      int cy = 3; // centre approx pour 8 lignes
      int dx = abs((int)col - cx);
      int dy = abs((int)row - cy);
      return (uint8_t)((dx + dy) % BB_RAINBOW_COUNT);
    }

    case CP_FRAME: {
      uint8_t left   = col;
      uint8_t right  = (BB_COLS - 1) - col;
      uint8_t top    = row;
      uint8_t bottom = (BB_ROWS - 1) - row;

      uint8_t minDist = left;
      if (right  < minDist) minDist = right;
      if (top    < minDist) minDist = top;
      if (bottom < minDist) minDist = bottom;

      return minDist % BB_RAINBOW_COUNT;
    }

    case CP_SPIRAL:
    default:
      // Pseudo spirale simple et légère
      return (uint8_t)((row * 3 + col * 5) % BB_RAINBOW_COUNT);
  }
}

static inline uint16_t bbGetBrickColor(uint8_t level, uint8_t row, uint8_t col) {
  return bbRainbowColor(bbGetBrickColorIndex(level, row, col));
}