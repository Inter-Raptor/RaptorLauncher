# 🦖 JurassicLife — README principal (EN)

Two-level documentation:
1. **Fun quick start** (fast setup)
2. **Technical deep dive** (all key details)

---

## 🎉 Part 1 — Quick start (friendly)

## Hardware supported
- 2432S022
- 2432S028
- ESP32 + ILI9341 320×240

## Super short setup
1. Set `DISPLAY_PROFILE`
2. Set `ENABLE_AUDIO` (0/1)
3. Upload from Arduino IDE
4. Put SD card if you want persistent saves

## Controls
- touch is always supported
- encoder / buttons are optional (board dependent)

## Visual docs
Use your screenshots/GIFs as reference for each screen/realm.

---

## 🛠️ Part 2 — Technical deep dive

## A) Display profile selection
Edit the profile macro in code:

```cpp
#define DISPLAY_PROFILE DISPLAY_PROFILE_2432S022
```

Allowed values:
- `DISPLAY_PROFILE_2432S022`
- `DISPLAY_PROFILE_2432S028`
- `DISPLAY_PROFILE_ILI9341_320x240`

## B) Audio switch

```cpp
#define ENABLE_AUDIO 1
```

- `0`: disable audio paths
- `1`: enable audio UI/control paths

## C) Optional physical inputs

You can configure:
- rotary encoder (A/B/button)
- or 3 buttons (Left/OK/Right)

Pin logic:
- pin `-1` means disabled
- use encoder **or** buttons, not both simultaneously

## D) Board-specific notes
- 2432S022: limited accessible pins, touch-first recommendation
- 2432S028: more practical for physical controls
- ILI9341 profile: flexible custom wiring

## E) UI/layout differences
Screen size differs by board, so layout can differ intentionally.

## F) Persistence model
Persistent dinosaur save requires microSD.  
No SD = no power-off persistence.

## G) Visual QA guidance
For each screenshot/GIF, add:
- hardware profile used
- expected user interaction
- expected result (state/UI feedback)
