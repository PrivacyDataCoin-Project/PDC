#!/usr/bin/env python3
"""Build PDC brand assets from the official lock+wordmark PNG."""
from __future__ import annotations

import io
import struct
import zlib
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

ROOT = Path(__file__).resolve().parents[1]
SRC_CANDIDATES = [
    Path("/home/ubuntu/.cursor/projects/workspace/assets/8a56629f-53cc-4e67-8f98-fba20b867263.png"),
    Path("/home/ubuntu/.cursor/projects/workspace/assets/bb66936e-13d5-4a06-ad97-ef91dda598a8.png"),
    ROOT / ".brand-source/logo-full-source.png",
]
BG = np.array([7, 9, 26], dtype=np.int16)
BG_RGBA = (7, 9, 26, 255)
BG_TOP = np.array([5, 6, 13], dtype=np.float32)  # #05060d
BG_BOT = np.array([7, 9, 26], dtype=np.float32)  # #07091a
ACCENT = np.array([59, 199, 255], dtype=np.float32)  # #3bc7ff
INSTALLER_SIZES = ((164, 313), (191, 385), (246, 457), (328, 628))
DMG_SIZE = (1512, 1164)


def find_source() -> Path:
    for p in SRC_CANDIDATES:
        if p.is_file():
            return p
    raise SystemExit("official logo PNG not found")


def to_transparent(im: Image.Image, thresh: int = 35) -> Image.Image:
    a = np.array(im.convert("RGBA"))
    diff = np.abs(a[:, :, :3].astype(np.int16) - BG).sum(axis=2)
    alpha = np.clip((diff - thresh) * 8, 0, 255).astype(np.uint8)
    a[:, :, 3] = np.minimum(a[:, :, 3], alpha)
    return Image.fromarray(a, "RGBA")


def content_bbox(im: Image.Image, thresh: int = 40):
    a = np.array(im.convert("RGBA"))
    diff = np.abs(a[:, :, :3].astype(np.int16) - BG).sum(axis=2)
    mask = diff > thresh
    ys, xs = np.where(mask)
    return int(xs.min()), int(ys.min()), int(xs.max()), int(ys.max())


def crop_pad(im: Image.Image, box, pad: int = 4) -> Image.Image:
    x0, y0, x1, y1 = box
    x0 = max(0, x0 - pad)
    y0 = max(0, y0 - pad)
    x1 = min(im.width - 1, x1 + pad)
    y1 = min(im.height - 1, y1 + pad)
    return im.crop((x0, y0, x1 + 1, y1 + 1))


def fit_center(src: Image.Image, size: int, bg=BG_RGBA) -> Image.Image:
    canvas = Image.new("RGBA", (size, size), bg)
    im = src.copy()
    im.thumbnail((int(size * 0.78), int(size * 0.78)), Image.Resampling.LANCZOS)
    canvas.alpha_composite(im, ((size - im.width) // 2, (size - im.height) // 2))
    return canvas


def horizontal_logo(mark_t: Image.Image, pdc_t: Image.Image, width=412, height=140, light=False) -> Image.Image:
    canvas = Image.new("RGBA", (width, height), (0, 0, 0, 0))
    m = mark_t.copy()
    m_h = int(height * 0.92)
    m.thumbnail((m_h, m_h), Image.Resampling.LANCZOS)

    text = pdc_t.copy()
    if light:
        arr = np.array(text)
        mask = arr[:, :, 3] > 20
        arr[mask, 0] = 12
        arr[mask, 1] = 12
        arr[mask, 2] = 58
        text = Image.fromarray(arr)

    text_h = int(height * 0.42)
    tw = int(text.width * (text_h / max(1, text.height)))
    text = text.resize((max(1, tw), text_h), Image.Resampling.LANCZOS)

    mx, my = 4, (height - m.height) // 2
    canvas.alpha_composite(m, (mx, my))
    canvas.alpha_composite(text, (mx + m.width + 14, (height - text.height) // 2))
    return canvas


def write_ico(path: Path, base: Image.Image, sizes=(16, 24, 32, 48, 64, 128, 256)) -> None:
    """Write a multi-size ICO with embedded PNG frames (Vista+)."""
    path.parent.mkdir(parents=True, exist_ok=True)
    pngs = []
    for s in sizes:
        buf = io.BytesIO()
        fit_center(base, s).convert("RGBA").save(buf, format="PNG")
        pngs.append(buf.getvalue())
    count = len(sizes)
    header = struct.pack("<HHH", 0, 1, count)
    offset = 6 + 16 * count
    entries = b""
    data = b""
    for s, png in zip(sizes, pngs):
        w = 0 if s >= 256 else s
        h = 0 if s >= 256 else s
        entries += struct.pack("<BBBBHHII", w, h, 0, 0, 1, 32, len(png), offset)
        data += png
        offset += len(png)
    path.write_bytes(header + entries + data)
    print("wrote", path, path.stat().st_size, "bytes")


def _png_chunk(tag: bytes, data: bytes) -> bytes:
    return struct.pack(">I", len(data)) + tag + data + struct.pack(">I", zlib.crc32(tag + data) & 0xFFFFFFFF)


def rgba_to_png_bytes(img: Image.Image) -> bytes:
    img = img.convert("RGBA")
    w, h = img.size
    raw = b"".join(b"\x00" + img.crop((0, y, w, y + 1)).tobytes() for y in range(h))
    return b"".join(
        [
            b"\x89PNG\r\n\x1a\n",
            _png_chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 6, 0, 0, 0)),
            _png_chunk(b"IDAT", zlib.compress(raw, 9)),
            _png_chunk(b"IEND", b""),
        ]
    )


def write_icns(path: Path, base: Image.Image) -> None:
    entries = [
        (b"icp4", 16),
        (b"icp5", 32),
        (b"icp6", 64),
        (b"ic07", 128),
        (b"ic08", 256),
        (b"ic09", 512),
        (b"ic10", 1024),
        (b"ic11", 32),
        (b"ic12", 64),
        (b"ic13", 256),
        (b"ic14", 512),
    ]
    parts = []
    for tag, size in entries:
        png = rgba_to_png_bytes(fit_center(base, size))
        parts.append(tag + struct.pack(">I", len(png) + 8) + png)
    body = b"".join(parts)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(b"icns" + struct.pack(">I", len(body) + 8) + body)
    print("wrote", path)


def save(path: Path, im: Image.Image) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    im.save(path)
    print("wrote", path)


def make_installer_bg(w: int, h: int) -> Image.Image:
    """Dark PDC plate with cyan bottom glow (no Zano marks)."""
    y = np.linspace(0, 1, h, dtype=np.float32)[:, None]
    x = np.linspace(-1, 1, w, dtype=np.float32)[None, :]
    grad = BG_TOP[None, None, :] * (1 - y)[..., None] + BG_BOT[None, None, :] * y[..., None]
    glow = np.exp(-((y - 0.92) ** 2) / (2 * 0.08 ** 2)) * np.exp(-(x ** 2) / (2 * 0.55 ** 2))
    rgb = grad + glow[..., None] * (ACCENT - grad) * 0.55
    glow2 = np.exp(-((y - 0.78) ** 2) / (2 * 0.12 ** 2)) * np.exp(-(x ** 2) / (2 * 0.7 ** 2))
    rgb = np.clip(rgb + glow2[..., None] * (ACCENT - rgb) * 0.18, 0, 255).astype(np.uint8)
    canvas = Image.fromarray(rgb, "RGB").convert("RGBA")
    overlay = Image.new("RGBA", (w, h), (0, 0, 0, 0))
    draw = ImageDraw.Draw(overlay)
    cx, cy = w // 2, int(h * 1.05)
    step = max(3, h // 40)
    for i, r in enumerate(range(int(h * 0.12), int(h * 0.55), step)):
        alpha = max(18, 70 - i * 6)
        draw.arc(
            [cx - r, cy - r, cx + r, cy + r],
            start=200,
            end=340,
            fill=(59, 199, 255, alpha),
            width=max(1, h // 220),
        )
    return Image.alpha_composite(canvas, overlay)


def compose_wizard_sidebar(logo: Image.Image, w: int, h: int) -> Image.Image:
    bg = make_installer_bg(w, h)
    fitted = logo.copy()
    fitted.thumbnail((int(w * 0.78), int(h * 0.42)), Image.Resampling.LANCZOS)
    bg.alpha_composite(fitted, ((w - fitted.width) // 2, int(h * 0.10)))
    return bg.convert("RGB")


def write_installer_graphics(logo_vertical_t: Image.Image, mark_t: Image.Image) -> None:
    resources = ROOT / "resources"
    for w, h in INSTALLER_SIZES:
        save(resources / f"installer_bg_{w}x{h}.bmp", compose_wizard_sidebar(logo_vertical_t, w, h))

    dw, dh = DMG_SIZE
    dmg = make_installer_bg(dw, dh)
    wm = mark_t.copy()
    wm.thumbnail((110, 110), Image.Resampling.LANCZOS)
    arr = np.array(wm)
    arr[:, :, 3] = (arr[:, :, 3].astype(np.float32) * 0.10).astype(np.uint8)
    wm = Image.fromarray(arr, "RGBA")
    for row in range(0, dh + 120, 160):
        for col in range(0, dw + 120, 160):
            ox = col + (80 if (row // 160) % 2 else 0)
            dmg.alpha_composite(wm, (ox - 55, row - 40))
    big = logo_vertical_t.copy()
    big.thumbnail((420, 420), Image.Resampling.LANCZOS)
    dmg.alpha_composite(big, (90, (dh - big.height) // 2 - 20))
    save(resources / "dmg_installer_bg.png", dmg.convert("RGB"))


def main() -> None:
    src_path = find_source()
    print("source:", src_path)
    src = Image.open(src_path).convert("RGBA")
    x0, y0, x1, y1 = content_bbox(src)
    print("content bbox", x0, y0, x1, y1)

    # Split bands using known layout of the official 400x400 asset.
    mark = crop_pad(src, (x0, y0, x1, 224), pad=2)
    pdc_word = crop_pad(src, (120, 255, 280, 278), pad=1)
    vertical = crop_pad(src, (x0, y0, x1, y1), pad=4)

    mark_t = to_transparent(mark)
    pdc_t = to_transparent(pdc_word)
    vertical_t = to_transparent(vertical)

    # Masters kept in-repo for regeneration.
    brand_dir = ROOT / "resources/brand"
    save(brand_dir / "logo-source.png", src.convert("RGB"))
    save(brand_dir / "mark-transparent.png", mark_t)
    save(brand_dir / "logo-vertical.png", vertical)
    save(brand_dir / "logo-vertical-transparent.png", vertical_t)
    save(brand_dir / "logo-horizontal-dark.png", horizontal_logo(mark_t, pdc_t, light=False))
    save(brand_dir / "logo-horizontal-light.png", horizontal_logo(mark_t, pdc_t, light=True))

    app256 = fit_center(mark_t, 256)
    app512 = fit_center(mark_t, 512)
    app1024 = fit_center(mark_t, 1024)
    save(ROOT / "resources/app_icon_256.png", app256.convert("RGB"))
    # Keep SVG as a simple wrapper referencing raster look is awkward; write PNG-based splash plate SVG-less:
    # Replace huge legacy SVG with a compact SVG that embeds nothing — use PNG for installers.
    # Write a minimal SVG mark plate for linux scalable icon using embedded PNG is heavy; export PNG only + simple SVG silhouette note.
    # For linux scalable, ship the PNG and a simple SVG that is a navy rounded rect + image is not portable.
    # Generate a lightweight SVG approximating nothing — copy PNG path usage in packaging already uses SVG+PNG.
    # We'll write app_icon.svg as an SVG that references a data-free navy plate and instruct packaging to prefer PNG.
    # Better: write SVG with embedded base64 PNG of the mark for true scalable-ish use.
    buf = io.BytesIO()
    app512.save(buf, format="PNG")
    import base64

    b64 = base64.b64encode(buf.getvalue()).decode("ascii")
    svg = f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="512" height="512" viewBox="0 0 512 512">
  <image width="512" height="512" xlink:href="data:image/png;base64,{b64}"/>
</svg>
'''
    (ROOT / "resources/app_icon.svg").write_text(svg, encoding="utf-8")
    print("wrote resources/app_icon.svg")

    gui = ROOT / "src/gui/qt-daemon"
    write_ico(gui / "app.ico", mark_t)
    write_icns(gui / "app.icns", mark_t)
    save(gui / "brand-mark.png", app256)

    # Website
    website = ROOT / "website"
    save(website / "brand/pdc-mark.png", app256)
    save(website / "brand/pdc-logo-vertical.png", vertical)
    save(website / "brand/pdc-logo-horizontal.png", horizontal_logo(mark_t, pdc_t, light=False))
    # favicon.svg via embedded png
    buf = io.BytesIO()
    fit_center(mark_t, 64).save(buf, format="PNG")
    b64 = base64.b64encode(buf.getvalue()).decode("ascii")
    (website / "favicon.svg").write_text(
        f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" viewBox="0 0 64 64">
  <image width="64" height="64" xlink:href="data:image/png;base64,{b64}"/>
</svg>
''',
        encoding="utf-8",
    )
    print("wrote website/favicon.svg")
    # remove obsolete generated SVGs if present
    for obsolete in (
        website / "brand/pdc-mark.svg",
        website / "brand/pdc-logo-dark.svg",
        website / "brand/pdc-logo-light.svg",
    ):
        if obsolete.exists():
            obsolete.unlink()
            print("removed", obsolete)

    # GUI layout assets (source + built)
    logo_dark = horizontal_logo(mark_t, pdc_t, light=False)
    logo_light = horizontal_logo(mark_t, pdc_t, light=True)
    currency = fit_center(mark_t, 64)
    currency_clear = Image.new("RGBA", (64, 64), (0, 0, 0, 0))
    m = mark_t.copy()
    m.thumbnail((58, 58), Image.Resampling.LANCZOS)
    currency_clear.alpha_composite(m, ((64 - m.width) // 2, (64 - m.height) // 2))

    for base in (
        gui / "layout/html_source/src/assets",
        gui / "layout/html/assets",
    ):
        save(base / "icons/blue/pdc-logo.png", logo_dark)
        save(base / "icons/blue/light-pdc-logo.png", logo_light)
        # Keep svg slots as PNG wrappers for Angular img tags that still point at .svg in some places:
        save(base / "icons/blue/pdc-logo.svg.png", logo_dark)  # unused helper
        # Write SVG wrappers embedding PNG so .svg paths keep working.
        for rel, img in (
            ("icons/blue/pdc-logo.svg", logo_dark),
            ("icons/blue/light-pdc-logo.svg", logo_light),
            ("pdc-icons/pdc-logo.svg", logo_dark),
            ("images/logo-text.svg", logo_dark),
            ("icons/currency-icons/pdc.svg", currency_clear),
            ("currency-icons/pdc.svg", currency_clear),
        ):
            buf = io.BytesIO()
            img.save(buf, format="PNG")
            b64 = base64.b64encode(buf.getvalue()).decode("ascii")
            w, h = img.size
            (base / rel).parent.mkdir(parents=True, exist_ok=True)
            (base / rel).write_text(
                f'''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="{w}" height="{h}" viewBox="0 0 {w} {h}">
  <image width="{w}" height="{h}" xlink:href="data:image/png;base64,{b64}"/>
</svg>
''',
                encoding="utf-8",
            )
            print("wrote", base / rel)
        save(base / "icons/currency-icons/pdc.png", currency)
        save(base / "currency-icons/pdc.png", currency)

    # Favicons
    for fav in (
        gui / "layout/html_source/src/favicon.ico",
        gui / "layout/html/favicon.ico",
        gui / "layout/favicon.ico",
        gui / "layout/html_source/favicon.ico",
    ):
        write_ico(fav, mark_t)

    # Tray / desktop icons
    for files in (
        gui / "layout/html_source/src/files",
        gui / "layout/html/files",
        gui / "layout/html_source/src/assets/files",
        gui / "layout/html/assets/files",
    ):
        files.mkdir(parents=True, exist_ok=True)
        save(files / "app22macos.png", fit_center(mark_t, 88))
        save(files / "app22macos_blocked.png", fit_center(mark_t, 32))
        save(files / "app22windows.png", fit_center(mark_t, 24))
        save(files / "app22windows_blocked.png", fit_center(mark_t, 16))
        save(files / "desktop_linux_icon.png", fit_center(mark_t, 48))

    # Windows Inno Setup wizard sidebar + macOS DMG background
    write_installer_graphics(vertical_t, mark_t)

    # Cleanup accidental helper
    for p in ROOT.glob("**/pdc-logo.svg.png"):
        p.unlink()
        print("removed", p)

    print("done")


if __name__ == "__main__":
    main()
