#!/usr/bin/env python3
"""bhscene — offline scene compositor for art QA (no raylib needed).

Replicates the client's draw order for a small isometric patch so any cell,
sheet or plate can be judged the way the engine will show it:

    ground diamonds (cut from a plate)  ->  y-sorted entities (feet anchor)
    ->  world-space text (callouts / name tags)  ->  night overlay  [-> HUD]

Verified against client/src/game.cpp (drawGround 671-686, entity draw origin
{w/2, 42} at 730/765, floaters 898-906, name tags 750-751, render order
1290-1303) and engine/render/iso.cpp tileToWorld. Nothing here touches the
engine; it is the B9' node in docs/art/PARALLEL-ROADMAP.md.
"""
from __future__ import annotations

import sys
from dataclasses import dataclass, field
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P  # noqa: E402

TW, TH = 64, 32
NEUTRAL_NAME = (0xBE, 0xBE, 0xC8)   # game.cpp karma band 1
LAWFUL_NAME = (170, 200, 255)       # band 0
CHAOTIC_NAME = (235, 60, 50)        # band 2
CALLOUT_RED = (255, 60, 40)         # T-066 kind 5
BG = (0x12, 0x10, 0x10, 255)


def iso(tx: float, ty: float, ox: float, oy: float) -> tuple[float, float]:
    """render/iso.cpp tileToWorldF (tile top-left), offset into the canvas."""
    return ((tx - ty) * TW * 0.5 + ox, (tx + ty) * TH * 0.5 + oy)


@dataclass
class Ent:
    img: Image.Image
    tx: float
    ty: float
    anchor_y: int = 42           # feet row inside the cell (T-ART-10 anchorY)
    name: str | None = None
    name_rgb: tuple[int, int, int] = NEUTRAL_NAME
    callout: str | None = None
    hp: float | None = None      # 0..1 draws the 28x3 bar at y-46 (game.cpp)
    sort_bias: float = 0.0


@dataclass
class Scene:
    w: int = 256
    h: int = 256
    tiles_x: int = 6
    tiles_y: int = 6
    plate: Image.Image | None = None      # painted ground plate (RGBA)
    plate_fallback: tuple[int, int, int] = (60, 54, 40)  # terrainColor(6) MUD
    ents: list[Ent] = field(default_factory=list)
    decals: list[tuple[Image.Image, float, float]] = field(default_factory=list)  # (img, tx, ty) under entities
    fx: list[tuple[Image.Image, int, int]] = field(default_factory=list)          # (img, px, py) over entities
    font: object | None = None                                                     # PIL font for text
    origin: tuple[float, float] | None = None

    # -- geometry ---------------------------------------------------------
    def _origin(self) -> tuple[float, float]:
        if self.origin:
            return self.origin
        return (self.w / 2 - TW / 2, 40.0)

    def anchor(self, tx: float, ty: float) -> tuple[int, int]:
        ox, oy = self._origin()
        x, y = iso(tx, ty, ox, oy)
        return int(x + TW / 2), int(y + TH / 2)

    # -- render -----------------------------------------------------------
    def render(self, *, hour: float | None = None, grid: bool = False) -> Image.Image:
        ox, oy = self._origin()
        scene = Image.new("RGBA", (self.w, self.h), BG)
        for ty in range(self.tiles_y):
            for tx in range(self.tiles_x):
                x, y = iso(tx, ty, ox, oy)
                if self.plate is not None:
                    t = P.cut_diamond(self.plate, int(x), int(y))
                else:
                    t = Image.new("RGBA", (TW, TH), (*self.plate_fallback, 255))
                    t.putalpha(P.diamond_mask())
                scene.alpha_composite(t, (int(x), int(y)))
                if grid:
                    d = ImageDraw.Draw(scene)
                    d.polygon([(x + TW / 2, y), (x + TW - 1, y + TH / 2), (x + TW / 2, y + TH - 1), (x, y + TH / 2)],
                              outline=(0, 0, 0, 60))
        for img, tx, ty in self.decals:
            ax, ay = self.anchor(tx, ty)
            scene.alpha_composite(img, (ax - img.width // 2, ay - img.height // 2))
        # y-sort like drawEntitiesOnline (painter's by world y)
        order = sorted(self.ents, key=lambda e: (e.tx + e.ty) + e.sort_bias)
        for e in order:
            ax, ay = self.anchor(e.tx, e.ty)
            scene.alpha_composite(e.img, (ax - e.img.width // 2, ay - e.anchor_y))
        for img, px, py in self.fx:
            scene.alpha_composite(img, (px, py))
        d = ImageDraw.Draw(scene)
        for e in order:
            ax, ay = self.anchor(e.tx, e.ty)
            if e.hp is not None:  # game.cpp: 28x3 bar at y-46, red on dark
                d.rectangle([ax - 14, ay - 46, ax + 14, ay - 43], fill=(30, 10, 10, 255))
                d.rectangle([ax - 14, ay - 46, ax - 14 + int(28 * e.hp), ay - 43], fill=(150, 30, 30, 255))
            if e.name:
                self._text(d, (ax, ay - 52), e.name, e.name_rgb, outline=False)
            if e.callout:
                self._text(d, (ax, ay - 62), e.callout, CALLOUT_RED, outline=True)
        if hour is not None:
            scene = P.night_floor(scene, hour)
        return scene

    def _text(self, d: ImageDraw.ImageDraw, xy, text, rgb, *, outline: bool):
        x, y = xy
        f = self.font
        try:
            w = int(d.textlength(text, font=f))
        except Exception:
            w = 6 * len(text)
        x -= w // 2
        if outline:
            for dx, dy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
                d.text((x + dx, y + dy), text, fill=(0, 0, 0, 255), font=f)
        d.text((x, y), text, fill=(*rgb, 255), font=f)


def triptych(scene_day: Image.Image, *, hour: float = 2.0, scale: int = 3, labels=("DAY", "NIGHT 02:00 (engine overlay)", "GREYSCALE")) -> Image.Image:
    """The §15 board: day | engine night | greyscale, nearest-upscaled."""
    night = P.night_floor(scene_day, hour)
    grey = P.greyscale(scene_day)
    W, H = scene_day.size
    board = Image.new("RGBA", (W * scale * 3 + 16, H * scale + 28), (0x0C, 0x0A, 0x0A, 255))
    for i, (lbl, im) in enumerate(zip(labels, (scene_day, night, grey))):
        board.alpha_composite(P.upscale(im, scale), (i * (W * scale + 8), 24))
        ImageDraw.Draw(board).text((i * (W * scale + 8) + 4, 6), lbl, fill=(0xE6, 0xD2, 0xBE, 255))
    return board.convert("RGB")
