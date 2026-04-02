#pragma once

#define APP_NAME "RaptorLauncher V2"
#define APP_VERSION "0.1.0"

// =========================
// Profils d'affichage
// =========================
#define DISPLAY_PROFILE_2432S022        1
#define DISPLAY_PROFILE_2432S028        2
#define DISPLAY_PROFILE_ILI9341_320x240 3

// =========================
// TA CARTE
// =========================
#define DISPLAY_PROFILE DISPLAY_PROFILE_2432S028

// =========================
// Dimensions logiques
// =========================
// On reste en rotation 1 : largeur 320, hauteur 240
#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240

// =========================
// SD
// =========================
#define SD_MOUNT_POINT "/games"

// =========================
// Couleurs debug / UI
// =========================
#define COLOR_BG        0x0000
#define COLOR_TEXT      0xFFFF
#define COLOR_ACCENT    0x07FF
#define COLOR_WARNING   0xFFE0
#define COLOR_ERROR     0xF800
#define COLOR_SUCCESS   0x07E0