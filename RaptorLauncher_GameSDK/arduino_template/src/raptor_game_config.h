#pragma once

// -----------------------------------------------------------------------------
// Profil hardware recommande pour RaptorLauncher V4
// -----------------------------------------------------------------------------
#define SDK_SCREEN_WIDTH   320
#define SDK_SCREEN_HEIGHT  240

// IMPORTANT: dossier jeu sous /games (sans slash).
// Exemple: si ton jeu est dans /games/MonJeu alors mets "MonJeu"
#define SDK_GAME_FOLDER_NAME "MonJeu"

// -----------------------------------------------------------------------------
// Contraintes meta.json recommandees pour compatibilite launcher
// -----------------------------------------------------------------------------
#define SDK_META_ICON_W   50
#define SDK_META_ICON_H   50
#define SDK_META_TITLE_W  320
#define SDK_META_TITLE_H  240
#define SDK_META_SAVE_FILENAME "sauv.json"
#define SDK_META_BIN_FILENAME  "game.bin"

// Ecran SPI ILI9341 / ST7789 style ESP32
#define SDK_PIN_TFT_CS     15
#define SDK_PIN_TFT_DC      2
#define SDK_PIN_TFT_RST    -1
#define SDK_PIN_TFT_MOSI   13
#define SDK_PIN_TFT_MISO   12
#define SDK_PIN_TFT_SCK    14
#define SDK_PIN_TFT_BL     27

// Touch XPT2046
#define SDK_PIN_TOUCH_CS   33
#define SDK_PIN_TOUCH_IRQ  36

// SD (HSPI)
#define SDK_PIN_SD_CS       5
#define SDK_PIN_SD_SCK     18
#define SDK_PIN_SD_MISO    19
#define SDK_PIN_SD_MOSI    23

// Audio buzzer / speaker
#define SDK_PIN_SPEAKER    26

// I2C (MCP23017)
#define SDK_PIN_I2C_SDA    21
#define SDK_PIN_I2C_SCL    22
#define SDK_MCP23017_ADDR  0x20

// LED RGB (optionnel)
#define SDK_PIN_LED_R       4
#define SDK_PIN_LED_G      16
#define SDK_PIN_LED_B      17

// Calibration tactile de base (ajuster si besoin)
#define SDK_TOUCH_X_MIN   200
#define SDK_TOUCH_X_MAX  3800
#define SDK_TOUCH_Y_MIN   200
#define SDK_TOUCH_Y_MAX  3800

// Couleurs utilitaires RGB565
#define SDK_COLOR_BG       0x0000
#define SDK_COLOR_TEXT     0xFFFF
#define SDK_COLOR_ACCENT   0x07FF
#define SDK_COLOR_OK       0x07E0
#define SDK_COLOR_WARN     0xFFE0
#define SDK_COLOR_ERROR    0xF800

// Luminosite (optionnel, selon revision carte)
#define SDK_PIN_LIGHT_SENSOR 34

// Wi-Fi optionnel
#define SDK_WIFI_CONNECT_TIMEOUT_MS 12000
