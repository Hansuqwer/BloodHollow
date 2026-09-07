#!/usr/bin/env python3
"""bhfont — hand-authored red-caps callout bitmap font (rulebook R-TEXT, A4).

Two provisional cap heights so the director can pick (decision D4):
  * CAP7  — 5x7 glyphs, parity with the client's current raylib default font
            at size 10 (game.cpp:904 DrawText ... 10) ⟨UNVERIFIED cap height⟩
  * CAP11 — 7x11 glyphs, the rulebook's R-TEXT size (HB ~14 px caps @640x480
            scaled to 1024x768 ≈ 11 px)

Glyph set: A–Z 0–9 ! - ' . : + / and space. Rendering: 1px black outline,
optional 2px night plate. Export: PNG strip + .fnt-style JSON (BMFont-like
fields: id, x, y, w, h, xadvance) so a client bitmap-font path (proposed
T-ART-13) can consume it directly. Pure Pillow/numpy; deterministic.

The 5x7 set is authored below as ASCII art; the 7x11 set is derived by an
integer-aware stretch (rows 1.57x, cols 1.4x) then hand-corrected in the
CORR11 table for the letters where a naive stretch breaks the era look.
"""
from __future__ import annotations

import json
from pathlib import Path

import numpy as np
from PIL import Image

CALLOUT_RED = (255, 60, 40)
OUTLINE = (0, 0, 0)
PLATE = (0x0C, 0x0A, 0x0A)

# 5x7 caps — classic arcade/HB register: flat tops, square counters.
G5 = {
"A": ["01110","10001","10001","11111","10001","10001","10001"],
"B": ["11110","10001","10001","11110","10001","10001","11110"],
"C": ["01111","10000","10000","10000","10000","10000","01111"],
"D": ["11110","10001","10001","10001","10001","10001","11110"],
"E": ["11111","10000","10000","11110","10000","10000","11111"],
"F": ["11111","10000","10000","11110","10000","10000","10000"],
"G": ["01111","10000","10000","10011","10001","10001","01111"],
"H": ["10001","10001","10001","11111","10001","10001","10001"],
"I": ["11111","00100","00100","00100","00100","00100","11111"],
"J": ["00111","00010","00010","00010","00010","10010","01100"],
"K": ["10001","10010","10100","11000","10100","10010","10001"],
"L": ["10000","10000","10000","10000","10000","10000","11111"],
"M": ["10001","11011","10101","10101","10001","10001","10001"],
"N": ["10001","11001","10101","10011","10001","10001","10001"],
"O": ["01110","10001","10001","10001","10001","10001","01110"],
"P": ["11110","10001","10001","11110","10000","10000","10000"],
"Q": ["01110","10001","10001","10001","10101","10010","01101"],
"R": ["11110","10001","10001","11110","10100","10010","10001"],
"S": ["01111","10000","10000","01110","00001","00001","11110"],
"T": ["11111","00100","00100","00100","00100","00100","00100"],
"U": ["10001","10001","10001","10001","10001","10001","01110"],
"V": ["10001","10001","10001","10001","10001","01010","00100"],
"W": ["10001","10001","10001","10101","10101","11011","10001"],
"X": ["10001","10001","01010","00100","01010","10001","10001"],
"Y": ["10001","10001","01010","00100","00100","00100","00100"],
"Z": ["11111","00001","00010","00100","01000","10000","11111"],
"0": ["01110","10011","10101","10101","10101","11001","01110"],
"1": ["00100","01100","00100","00100","00100","00100","01110"],
"2": ["01110","10001","00001","00010","00100","01000","11111"],
"3": ["11110","00001","00001","01110","00001","00001","11110"],
"4": ["00010","00110","01010","10010","11111","00010","00010"],
"5": ["11111","10000","10000","11110","00001","00001","11110"],
"6": ["01110","10000","10000","11110","10001","10001","01110"],
"7": ["11111","00001","00010","00100","01000","01000","01000"],
"8": ["01110","10001","10001","01110","10001","10001","01110"],
"9": ["01110","10001","10001","01111","00001","00001","01110"],
"!": ["00100","00100","00100","00100","00100","00000","00100"],
"-": ["00000","00000","00000","01110","00000","00000","00000"],
"'": ["00100","00100","01000","00000","00000","00000","00000"],
".": ["00000","00000","00000","00000","00000","00000","00100"],
":": ["00000","00100","00000","00000","00000","00100","00000"],
"+": ["00000","00100","00100","11111","00100","00100","00000"],
"/": ["00001","00010","00010","00100","01000","01000","10000"],
" ": ["00000","00000","00000","00000","00000","00000","00000"],
}

# 7x11 hand corrections (after the stretch) for letters where round diagonals matter.
CORR11 = {
"A": ["0011100","0100010","1000001","1000001","1000001","1111111","1000001","1000001","1000001","1000001","1000001"],
"M": ["1000001","1100011","1110111","1010101","1010101","1001001","1000001","1000001","1000001","1000001","1000001"],
"N": ["1000001","1100001","1110001","1011001","1001101","1000111","1000011","1000001","1000001","1000001","1000001"],
"S": ["0111111","1000000","1000000","1000000","0111110","0000001","0000001","0000001","0000001","0000001","1111110"],
"V": ["1000001","1000001","1000001","1000001","1000001","1000001","0100010","0100010","0010100","0010100","0001000"],
"W": ["1000001","1000001","1000001","1000001","1001001","1001001","1010101","1010101","1010101","1101011","1000001"],
"X": ["1000001","1000001","0100010","0010100","0001000","0001000","0001000","0010100","0100010","1000001","1000001"],
"Y": ["1000001","1000001","0100010","0010100","0001000","0001000","0001000","0001000","0001000","0001000","0001000"],
"Z": ["1111111","0000001","0000010","0000100","0001000","0001000","0010000","0100000","1000000","1000000","1111111"],
"K": ["1000001","1000010","1000100","1001000","1010000","1100000","1010000","1001000","1000100","1000010","1000001"],
"R": ["1111110","1000001","1000001","1000001","1111110","1001000","1000100","1000100","1000010","1000010","1000001"],
"!": ["0011000","0011000","0011000","0011000","0011000","0011000","0011000","0011000","0000000","0011000","0011000"],
# punctuation: the naive stretch turns ' into a slanted 2x2 blob and . : into
# 1-px dots (seen in font_specimen_3x.png, 2026-09-07) — era fonts use a
# straight 2-px tick and 2x2 dots at this cap height.
"'": ["0011000","0011000","0011000","0010000","0100000","0000000","0000000","0000000","0000000","0000000","0000000"],
".": ["0000000","0000000","0000000","0000000","0000000","0000000","0000000","0000000","0000000","0011000","0011000"],
":": ["0000000","0000000","0011000","0011000","0000000","0000000","0000000","0011000","0011000","0000000","0000000"],
}


def glyph5(ch: str) -> np.ndarray:
    rows = G5.get(ch.upper(), G5[" "])
    return np.array([[int(c) for c in r] for r in rows], dtype=np.uint8)


def glyph11(ch: str) -> np.ndarray:
    ch = ch.upper()
    if ch in CORR11:
        return np.array([[int(c) for c in r] for r in CORR11[ch]], dtype=np.uint8)
    g = glyph5(ch)
    # stretch 5x7 -> 7x11 by nearest with a fixed row/col map (deterministic)
    rmap = [0, 0, 1, 2, 2, 3, 4, 4, 5, 6, 6]
    cmap = [0, 1, 1, 2, 3, 3, 4]
    return g[np.ix_(rmap, cmap)]


class BitmapFont:
    def __init__(self, cap: int = 7):
        assert cap in (7, 11)
        self.cap = cap
        self.gw = 5 if cap == 7 else 7
        self.gh = cap
        self.space = 1 if cap == 7 else 2  # inter-glyph gap (before the outline)
        self._g = glyph5 if cap == 7 else glyph11

    def measure(self, text: str) -> int:
        n = len(text)
        return n * self.gw + max(0, n - 1) * self.space + 2  # +2 outline

    def render(self, text: str, rgb=CALLOUT_RED, *, outline: bool = True, plate: bool = False, plate_alpha: int = 160) -> Image.Image:
        w = self.measure(text)
        h = self.gh + 2
        pad = 2 if plate else 0
        canvas = np.zeros((h + pad * 2, w + pad * 2, 4), np.uint8)
        if plate:
            canvas[..., :3] = PLATE
            canvas[..., 3] = plate_alpha
        ink = np.zeros((h, w), bool)
        x = 1
        for ch in text:
            g = self._g(ch).astype(bool)
            ink[1:1 + self.gh, x:x + self.gw] |= g
            x += self.gw + self.space
        if outline:
            o = np.zeros_like(ink)
            for dy, dx in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                o |= np.roll(np.roll(ink, dy, 0), dx, 1)
            o &= ~ink
            sub = canvas[pad:pad + h, pad:pad + w]
            sub[o] = (*OUTLINE, 255)
        sub = canvas[pad:pad + h, pad:pad + w]
        sub[ink] = (*rgb, 255)
        return Image.fromarray(canvas, "RGBA")

    def export(self, out_png: Path, out_json: Path, rgb=(255, 255, 255)) -> dict:
        """Strip of raw glyphs (no outline; the client draws the outline or we
        ship a second outlined strip) + BMFont-like JSON."""
        chars = list(G5.keys())
        cell_w, cell_h = self.gw + 2, self.gh + 2
        strip = np.zeros((cell_h, cell_w * len(chars), 4), np.uint8)
        meta = {"face": f"bh-redcaps-{self.cap}", "size": self.cap, "lineHeight": cell_h,
                "base": self.gh + 1, "outline": 1, "chars": []}
        for i, ch in enumerate(chars):
            g = self._g(ch).astype(bool)
            x0 = i * cell_w + 1
            strip[1:1 + self.gh, x0:x0 + self.gw][g] = (*rgb, 255)
            meta["chars"].append({"id": ord(ch), "char": ch, "x": i * cell_w, "y": 0,
                                  "width": cell_w, "height": cell_h, "xoffset": -1, "yoffset": -1,
                                  "xadvance": self.gw + self.space})
        out_png.parent.mkdir(parents=True, exist_ok=True)
        Image.fromarray(strip, "RGBA").save(out_png)
        out_json.write_text(json.dumps(meta, indent=1) + "\n")
        return meta


# ---------------------------------------------------------------------------
# reproducible export (both sizes + specimen) — python3 bhfont.py [out_dir]
# ---------------------------------------------------------------------------
SPECIMEN_LINES = ("POWER-SWING!", "BLOOD BOLT!", "WIDOW'S RITE +5",
                  "ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 -!'.:+/")


def specimen(scale: int = 3) -> Image.Image:
    """Both cap heights, 4 lines each, on the night plate colour, nearest x3."""
    rows = []
    for cap in (7, 11):
        f = BitmapFont(cap)
        for i, line in enumerate(SPECIMEN_LINES):
            rows.append(f.render(line, plate=(i == 1)))
        rows.append(None)  # gap between sizes
    w = max(r.width for r in rows if r is not None) + 12
    h = sum((r.height if r is not None else 8) + 4 for r in rows) + 8
    canvas = Image.new("RGBA", (w, h), (*PLATE, 255))
    y = 6
    for r in rows:
        if r is None:
            y += 12
            continue
        canvas.alpha_composite(r, (6, y))
        y += r.height + 4
    return canvas.resize((canvas.width * scale, canvas.height * scale), Image.NEAREST)


def main(argv: list[str]) -> int:
    out = Path(argv[1]) if len(argv) > 1 else Path(__file__).resolve().parents[2] / "docs/research-notes/style-tile/export/font"
    out.mkdir(parents=True, exist_ok=True)
    for cap in (7, 11):
        BitmapFont(cap).export(out / f"callout_font_cap{cap}.png", out / f"callout_font_cap{cap}.json")
    specimen().save(out / "font_specimen_3x.png")
    print(f"wrote {out}/callout_font_cap{{7,11}}.{{png,json}} + font_specimen_3x.png")
    return 0


if __name__ == "__main__":
    import sys
    raise SystemExit(main(sys.argv))
