# 🦖 JurassicLife — Code Guide (EN)

This file is split in two levels:
- **Part 1**: simple onboarding
- **Part 2**: technical, step-by-step reference

---

## 🎉 Part 1 — Friendly onboarding

## 1) What to edit first
At the top of the sketch, update:
- `DISPLAY_PROFILE`
- `ENABLE_AUDIO`

Then upload. That’s the minimum path.

## 2) If you want physical controls
- choose rotary encoder **or** 3-button mode
- keep unused pins to `-1`

## 3) If you want real save persistence
Insert microSD card.

---

## 🛠️ Part 2 — Full technical reference

## A) Supported profiles
- `DISPLAY_PROFILE_2432S022`
- `DISPLAY_PROFILE_2432S028`
- `DISPLAY_PROFILE_ILI9341_320x240`

Macro shape (do not rename left side):

```cpp
#define DISPLAY_PROFILE DISPLAY_PROFILE_2432S022
```

## B) Audio mode

```cpp
#define ENABLE_AUDIO 1
```

Behavior:
- value `0`: audio code paths disabled
- value `1`: audio controls shown and active

## C) Input routing strategy

The code typically uses compile-time pin assignment blocks (`#if DISPLAY_PROFILE ...`).

Rules:
1. `-1` => logical disable
2. Encoder mode: define `ENC_A`, `ENC_B`, optional `ENC_BTN`; keep `BTN_*=-1`
3. Button mode: set `ENC_*=-1`; define `BTN_LEFT/BTN_OK/BTN_RIGHT`
4. Avoid mixed mode unless explicitly implemented

## D) Electrical notes
- Some input pins may require pull-up resistor depending on board/pin type.
- Validate debouncing strategy in software if mechanical jitter appears.

## E) Display/UI expectations
- 2432S028 offers more room than 2432S022.
- UI differences between profiles are expected and normal.

## F) Save behavior
- Persistent save after reboot/power loss requires SD.
- Without SD, runtime state is volatile.

## G) Recommended validation checklist
- profile selected correctly
- touch input mapped correctly
- physical input mode consistent with pin settings
- audio button visible only when expected
- save survives reboot with SD inserted
- visual screens match screenshot/GIF documentation
