from pathlib import Path
from PIL import Image

def rgb888_to_rgb565(r: int, g: int, b: int) -> int:
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def convert_png_to_raw(png_path: Path) -> None:
    img = Image.open(png_path).convert("RGB")
    width, height = img.size

    raw_path = png_path.with_suffix(".raw")
    info_path = png_path.with_suffix(".txt")

    with open(raw_path, "wb") as f:
        for y in range(height):
            for x in range(width):
                r, g, b = img.getpixel((x, y))
                rgb565 = rgb888_to_rgb565(r, g, b)

                # écriture en little-endian
                f.write(bytes(((rgb565 >> 8) & 0xFF, rgb565 & 0xFF)))

    with open(info_path, "w", encoding="utf-8") as f:
        f.write(f"file={raw_path.name}\n")
        f.write(f"width={width}\n")
        f.write(f"height={height}\n")
        f.write("format=RGB565_LE\n")

    print(f"[OK] {png_path.name} -> {raw_path.name} ({width}x{height})")

def main():
    folder = Path(__file__).resolve().parent
    png_files = sorted(folder.glob("*.png"))

    if not png_files:
        print("Aucun fichier PNG trouve dans le dossier du script.")
        return

    print(f"Dossier : {folder}")
    print(f"{len(png_files)} fichier(s) PNG trouve(s).\n")

    for png_path in png_files:
        try:
            convert_png_to_raw(png_path)
        except Exception as e:
            print(f"[ERREUR] {png_path.name} : {e}")

    print("\nConversion terminee.")

if __name__ == "__main__":
    main()