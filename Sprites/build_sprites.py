#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import sys
import re
from pathlib import Path
from collections import Counter, deque, defaultdict
from PIL import Image, ImageSequence

# ============================================================
# CONFIG
# ============================================================
OUT_DIR_NAME = "generated_headers"
SOURCE_DIR_NAME = "ImagePNG"

ALLOWED_EXTS = {".png", ".jpg", ".jpeg", ".bmp", ".gif", ".webp"}

AUTO_REMOVE_BG_IF_NO_ALPHA = True
BG_TOLERANCE = 22
ALPHA_THRESHOLD = 0

# Trim uniquement pour les images simples
TRIM_SINGLE_IMAGES = True

KEY_CANDIDATES_565 = [0xF81F, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF800, 0x0010]

# "nom12"
ENDING_NUMBER_RE = re.compile(r"^(?P<base>.*?)(?P<num>\d+)$", re.IGNORECASE)

# "nom - Copie" ou "nom - Copie (3)"
COPY_RE = re.compile(
    r"^(?P<base>.+?)\s*-\s*copie(?:\s*\((?P<copy_num>\d+)\))?$",
    re.IGNORECASE
)

# ============================================================
# BASE DIR
# ============================================================
def get_base_dir() -> Path:
    if getattr(sys, "frozen", False):
        return Path(sys.executable).resolve().parent
    return Path(__file__).resolve().parent

# ============================================================
# STRING UTILS
# ============================================================
def sanitize_name(name: str) -> str:
    name = name.strip().lower()
    name = re.sub(r"[^a-z0-9_]+", "_", name)
    name = re.sub(r"_+", "_", name).strip("_")
    if not name:
        name = "sprite"
    if name[0].isdigit():
        name = "_" + name
    return name

def pretty_name(name: str) -> str:
    cleaned = re.sub(r"[^a-zA-Z0-9]+", " ", name).strip()
    if not cleaned:
        return "Sprite"
    return "".join(part[:1].upper() + part[1:] for part in cleaned.split())

# ============================================================
# IMAGE UTILS
# ============================================================
def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def load_first_frame(path: Path) -> Image.Image:
    im = Image.open(path)
    try:
        frame0 = next(ImageSequence.Iterator(im))
        return frame0.copy()
    except Exception:
        return im.copy()

def color_dist(c1, c2) -> int:
    return abs(c1[0]-c2[0]) + abs(c1[1]-c2[1]) + abs(c1[2]-c2[2])

def guess_background_color(im_rgba: Image.Image):
    w, h = im_rgba.size
    px = im_rgba.load()
    samples = []
    coords = [
        (0, 0), (w-1, 0), (0, h-1), (w-1, h-1),
        (w//2, 0), (w//2, h-1), (0, h//2), (w-1, h//2),
    ]
    for x, y in coords:
        r, g, b, a = px[x, y]
        samples.append((r, g, b))
    return Counter(samples).most_common(1)[0][0]

def remove_background_to_alpha(im_rgba: Image.Image, tolerance: int) -> Image.Image:
    if im_rgba.mode != "RGBA":
        im_rgba = im_rgba.convert("RGBA")

    w, h = im_rgba.size
    px = im_rgba.load()
    bg = guess_background_color(im_rgba)

    visited = [[False] * w for _ in range(h)]
    q = deque()

    for x in range(w):
        q.append((x, 0))
        q.append((x, h - 1))
    for y in range(h):
        q.append((0, y))
        q.append((w - 1, y))

    def is_bg(x, y) -> bool:
        r, g, b, a = px[x, y]
        if a == 0:
            return True
        return color_dist((r, g, b), bg) <= tolerance

    while q:
        x, y = q.popleft()
        if x < 0 or x >= w or y < 0 or y >= h:
            continue
        if visited[y][x]:
            continue
        visited[y][x] = True
        if not is_bg(x, y):
            continue
        r, g, b, a = px[x, y]
        px[x, y] = (r, g, b, 0)
        q.append((x + 1, y))
        q.append((x - 1, y))
        q.append((x, y + 1))
        q.append((x, y - 1))

    return im_rgba

def trim_transparent(im_rgba: Image.Image):
    if im_rgba.mode != "RGBA":
        im_rgba = im_rgba.convert("RGBA")
    alpha = im_rgba.getchannel("A")
    bbox = alpha.getbbox()
    if bbox is None:
        return im_rgba, None
    return im_rgba.crop(bbox), bbox

def choose_best_key_for_images(images_rgba):
    opaque_565 = set()
    for im in images_rgba:
        rgba = im.convert("RGBA")
        for (r, g, b, a) in rgba.getdata():
            if a > ALPHA_THRESHOLD:
                opaque_565.add(rgb888_to_rgb565(r, g, b))
    for k in KEY_CANDIDATES_565:
        if k not in opaque_565:
            return k, False
    return KEY_CANDIDATES_565[0], True

def image_to_rgb565_array(im: Image.Image, key565: int):
    im_rgba = im.convert("RGBA")
    w, h = im_rgba.size
    values = []
    collision = False

    for (r, g, b, a) in im_rgba.getdata():
        if a <= ALPHA_THRESHOLD:
            values.append(key565)
        else:
            v = rgb888_to_rgb565(r, g, b)
            if v == key565:
                collision = True
            values.append(v)

    return w, h, values, collision

def prepare_single_image(path: Path) -> Image.Image:
    im = load_first_frame(path).convert("RGBA")

    if AUTO_REMOVE_BG_IF_NO_ALPHA:
        alpha = im.getchannel("A")
        min_a, _ = alpha.getextrema()
        if min_a != 0:
            im = remove_background_to_alpha(im, tolerance=BG_TOLERANCE)

    if TRIM_SINGLE_IMAGES:
        im, _ = trim_transparent(im)

    return im

def prepare_anim_image(path: Path) -> Image.Image:
    im = load_first_frame(path).convert("RGBA")

    if AUTO_REMOVE_BG_IF_NO_ALPHA:
        alpha = im.getchannel("A")
        min_a, _ = alpha.getextrema()
        if min_a != 0:
            im = remove_background_to_alpha(im, tolerance=BG_TOLERANCE)

    # PAS de trim pour les animations
    return im

# ============================================================
# FILE CLASSIFICATION
# ============================================================
def parse_stem_info(stem: str):
    """
    Cas gérés :
    - herbe
    - cactus1
    - marche2 - Copie
    - ptera - Copie (3)
    - ptera
    """
    original_stem = stem.strip()

    copy_order = 0
    base_after_copy_strip = original_stem

    m_copy = COPY_RE.match(original_stem)
    if m_copy:
        base_after_copy_strip = m_copy.group("base").strip()
        n = m_copy.group("copy_num")
        copy_order = 1 if n is None else int(n) + 1

    m_num = ENDING_NUMBER_RE.match(base_after_copy_strip)
    if m_num:
        anim_base = m_num.group("base").strip()
        frame_num = int(m_num.group("num"))
        if anim_base:
            return {
                "type": "anim",
                "anim_name": anim_base,
                "frame_sort": (frame_num, copy_order),
            }

    if copy_order > 0:
        return {
            "type": "anim_copy_family",
            "anim_name": base_after_copy_strip,
            "frame_sort": (1, copy_order),
        }

    return {
        "type": "single_or_maybe_anim_root",
        "name": original_stem,
    }

def collect_source_files(source_dir: Path):
    return [
        p for p in sorted(source_dir.iterdir())
        if p.is_file() and p.suffix.lower() in ALLOWED_EXTS
    ]

def build_groups(files):
    singles_candidates = []
    anims = defaultdict(list)

    stem_to_path = {p.stem.strip().lower(): p for p in files}

    for p in files:
        info = parse_stem_info(p.stem)

        if info["type"] == "anim":
            anims[info["anim_name"]].append((info["frame_sort"], p))
        elif info["type"] == "anim_copy_family":
            anims[info["anim_name"]].append((info["frame_sort"], p))
        else:
            singles_candidates.append((p.stem.strip(), p))

    for anim_name in list(anims.keys()):
        root_key = anim_name.strip().lower()
        root_path = stem_to_path.get(root_key)
        if root_path is not None:
            already = any(path.resolve() == root_path.resolve() for _, path in anims[anim_name])
            if not already:
                anims[anim_name].append(((1, 0), root_path))

    cleaned_anims = {}
    used_anim_paths = set()

    for anim_name, entries in anims.items():
        seen = set()
        unique_entries = []
        for sort_key, path in entries:
            rp = path.resolve()
            key = (sort_key, str(rp))
            if key in seen:
                continue
            seen.add(key)
            unique_entries.append((sort_key, path))

        unique_entries.sort(key=lambda t: (t[0][0], t[0][1], t[1].name.lower()))
        cleaned_anims[anim_name] = unique_entries

        for _, path in unique_entries:
            used_anim_paths.add(path.resolve())

    singles = []
    for base_name, path in singles_candidates:
        if path.resolve() not in used_anim_paths:
            singles.append((base_name, path))

    return singles, cleaned_anims

# ============================================================
# WRITERS
# ============================================================
def write_single_header(out_path: Path, pretty: str, w: int, h: int, key565: int, values):
    def fmt(v):
        return f"0x{v:04X}"

    lines = []
    lines += ["#pragma once", "#include <Arduino.h>", ""]
    lines += ["// Auto-generated: single image -> RGB565 (KEY transparency)", ""]
    lines += [f"static const uint16_t {pretty}W = {w};"]
    lines += [f"static const uint16_t {pretty}H = {h};"]
    lines += [f"static const uint16_t {pretty}KEY = 0x{key565:04X};"]
    lines += [""]
    lines += [f"static const uint16_t {pretty}[{pretty}W * {pretty}H] PROGMEM = {{"]

    per_line = 12
    for i in range(0, len(values), per_line):
        chunk = values[i:i + per_line]
        lines.append("  " + ", ".join(fmt(v) for v in chunk) + ("," if i + per_line < len(values) else ""))
    lines += ["};", ""]

    out_path.write_text("\n".join(lines), encoding="utf-8")

def write_anim_header(out_path: Path, anim_name: str, frames_data, key565: int, warn: bool):
    def fmt(v):
        return f"0x{v:04X}"

    pretty = pretty_name(anim_name)
    w0 = frames_data[0]["w"]
    h0 = frames_data[0]["h"]

    lines = []
    lines += ["#pragma once", "#include <Arduino.h>", ""]
    lines += [f"// Auto-generated: animation '{pretty}' -> RGB565 (KEY transparency)"]
    lines += [f"// KEY (RGB565): 0x{key565:04X}"]
    if warn:
        lines += ["// WARNING: KEY collides with some opaque pixels."]
    lines += [""]

    lines += [f"static const uint16_t {pretty}W = {w0};"]
    lines += [f"static const uint16_t {pretty}H = {h0};"]
    lines += [f"static const uint16_t {pretty}KEY = 0x{key565:04X};"]
    lines += [""]

    var_names = []

    for item in frames_data:
        i = item["generated_index"]
        w = item["w"]
        h = item["h"]
        values = item["values"]

        if w != w0 or h != h0:
            raise RuntimeError(
                f"Animation '{anim_name}' : tailles incohérentes ({w}x{h}) vs ({w0}x{h0})"
            )

        var_name = f"{pretty}{i}"
        var_names.append(var_name)

        lines.append(f"static const uint16_t {var_name}[{pretty}W * {pretty}H] PROGMEM = {{")
        per_line = 12
        for j in range(0, len(values), per_line):
            chunk = values[j:j + per_line]
            lines.append("  " + ", ".join(fmt(v) for v in chunk) + ("," if j + per_line < len(values) else ""))
        lines.append("};")
        lines.append("")

    lines.append(f"static const uint16_t* const {pretty}Frames[] PROGMEM = {{")
    lines.append("  " + ", ".join(var_names))
    lines.append("};")
    lines.append(f"static const uint8_t {pretty}Count = {len(var_names)};")
    lines.append("")

    out_path.write_text("\n".join(lines), encoding="utf-8")

def write_single_info_txt(out_path: Path, pretty: str, src_name: str, w: int, h: int, key565: int):
    lines = [
        f"Type: image simple",
        f"Nom: {pretty}",
        f"Header: {pretty}.h",
        f"Source: {src_name}",
        f"Taille: {w}x{h}",
        f"Couleur KEY RGB565: 0x{key565:04X}",
        f"Nom genere: {pretty}",
    ]
    out_path.write_text("\n".join(lines), encoding="utf-8")

def write_anim_info_txt(out_path: Path, pretty: str, key565: int, frames_data):
    w0 = frames_data[0]["w"]
    h0 = frames_data[0]["h"]

    lines = [
        "Type: animation",
        f"Nom: {pretty}",
        f"Header: {pretty}.h",
        f"Taille commune: {w0}x{h0}",
        f"Couleur KEY RGB565: 0x{key565:04X}",
        f"Nombre de frames: {len(frames_data)}",
        "",
        "Ordre des frames:"
    ]

    for item in frames_data:
        lines.append(
            f'{item["generated_index"]} -> {item["source_name"]} -> {item["generated_name"]}'
        )

    out_path.write_text("\n".join(lines), encoding="utf-8")

# ============================================================
# BUILD
# ============================================================
def build_headers(source_dir: Path, out_dir: Path):
    files = collect_source_files(source_dir)
    if not files:
        print(f"[INFO] Aucun fichier image dans {source_dir}")
        return

    singles, anims = build_groups(files)

    print(f"[DEBUG] singles détectés : {len(singles)}")
    print(f"[DEBUG] animations détectées : {len(anims)}")
    print("")

    # Images simples
    for base_name, path in singles:
        try:
            im = prepare_single_image(path)
            key565, key_warn = choose_best_key_for_images([im])
            w, h, values, collision = image_to_rgb565_array(im, key565)

            pretty = pretty_name(base_name)
            out_h_path = out_dir / f"{pretty}.h"
            out_txt_path = out_dir / f"{pretty}.txt"

            write_single_header(out_h_path, pretty, w, h, key565, values)
            write_single_info_txt(out_txt_path, pretty, path.name, w, h, key565)

            print(f"[SINGLE OK] {path.name} -> {out_h_path.name} / {out_txt_path.name} (warn={key_warn or collision})")
            print(f"    source={path.name} | taille={w}x{h} | nom_genere={pretty}")
            print()

        except Exception as e:
            print(f"[SINGLE ERROR] {path.name} -> {e}")
            print()

    # Animations
    for anim_name, entries in sorted(anims.items(), key=lambda t: t[0].lower()):
        try:
            images = []
            for _, path in entries:
                im = prepare_anim_image(path)
                images.append((path, im))

            if not images:
                continue

            key565, key_warn = choose_best_key_for_images([im for _, im in images])

            frames_data = []
            any_collision = False
            pretty = pretty_name(anim_name)

            for idx, (path, im) in enumerate(images, start=1):
                w, h, values, collision = image_to_rgb565_array(im, key565)
                any_collision = any_collision or collision

                frames_data.append({
                    "generated_index": idx,
                    "generated_name": f"{pretty}{idx}",
                    "source_name": path.name,
                    "w": w,
                    "h": h,
                    "values": values,
                })

            out_h_path = out_dir / f"{pretty}.h"
            out_txt_path = out_dir / f"{pretty}.txt"

            write_anim_header(out_h_path, anim_name, frames_data, key565, key_warn or any_collision)
            write_anim_info_txt(out_txt_path, pretty, key565, frames_data)

            print(f"[ANIM OK] {pretty} -> {out_h_path.name} / {out_txt_path.name} ({len(frames_data)} frames)")
            print(f"    taille_commune={frames_data[0]['w']}x{frames_data[0]['h']}")
            print("    ordre:")
            for item in frames_data:
                print(f"      {item['generated_index']} -> {item['source_name']} -> {item['generated_name']}")
            print()

        except Exception as e:
            print(f"[ANIM ERROR] {anim_name} -> {e}")
            print()

# ============================================================
# MAIN
# ============================================================
def main():
    base = get_base_dir()
    source_dir = base / SOURCE_DIR_NAME
    out_dir = base / OUT_DIR_NAME
    out_dir.mkdir(parents=True, exist_ok=True)

    print("=== Sprites Builder (ImagePNG -> headers + txt) ===")
    print(f"Dossier source : {source_dir}")
    print(f"Dossier sortie : {out_dir}")
    print("")

    if not source_dir.exists():
        print(f"[ERREUR] dossier '{SOURCE_DIR_NAME}' absent")
        return

    build_headers(source_dir, out_dir)
    print("Terminé.")

if __name__ == "__main__":
    try:
        main()
    finally:
        input("\nAppuie sur Entrée pour fermer...")