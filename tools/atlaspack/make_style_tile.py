#!/usr/bin/env python3
"""B0 style-lock proof (bible §0 step 2, §4): one 256x256 scene —
field terrain + mob + player + spell — pushed through the real cleanup
pipeline (tools/atlaspack/bhpix.py) at true engine scale, then rendered
in the three QA views the §15 checklist demands (day, capped night,
greyscale) plus a 3x nearest blow-up for humans.

Inputs (docs/research-notes/style-tile/plates/*_4x_raw.png) are 4x AI
plates; outputs land in docs/research-notes/style-tile/. Deterministic.
"""
from __future__ import annotations

import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageDraw

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
PLATES = ROOT / "docs/research-notes/style-tile/plates"
OUT = ROOT / "docs/research-notes/style-tile"
TW, TH = 64, 32  # engine tile

# ---- reserved accent ramp (bible §4.2) — only ever on magic/curse pixels
VIOLET_CURSE = (0x6B, 0x4A, 0x8A)  # era-violet (44ad327); highlight #A884C4
ARTERIAL = (0x8E, 0x10, 0x1C)
CHOIR_GOLD = (0xD9, 0xB0, 0x4A)
CALLOUT_RED = (0xFF, 0x3C, 0x28)  # T-066 kind-5 red-caps (255,60,40)


def iso(tx: float, ty: float, ox: float, oy: float):
    """render/iso.cpp tileToWorldF, offset into the 256x256 canvas."""
    return ((tx - ty) * TW * 0.5 + ox, (tx + ty) * TH * 0.5 + oy)


def diamond_mask(w=TW, h=TH) -> Image.Image:
    m = Image.new("L", (w, h), 0)
    d = ImageDraw.Draw(m)
    d.polygon([(w // 2, 0), (w - 1, h // 2), (w // 2, h - 1), (0, h // 2)], fill=255)
    return m


def make_ground_tiles(src: Image.Image, pal: np.ndarray, n: int = 6):
    """Six non-repeating 64x32 diamonds cut from a nearest-downscaled painterly
    plate (§6: >=6 base variants per ground type). The plate is scaled so one
    tile spans ~1/4 of it — the Soma 'large zoom' read."""
    # Soma zoom: one 64x32 tile is a *small* patch of ground, so the plate is
    # kept large (1 tile ~ 1/7 of the plate) and low-frequency. Painterly ground
    # is smoothed (box + slight blur), contrast is pulled DOWN and mid-tones
    # lifted so hard-outlined sprites separate (readability rule R-LUMA).
    from PIL import ImageEnhance, ImageFilter
    small = src.convert("RGB").resize((512, 280), Image.BOX).filter(ImageFilter.GaussianBlur(0.8))
    small = ImageEnhance.Contrast(small).enhance(0.70)
    arr = np.asarray(small).astype(np.float32)
    arr = np.clip(arr * 0.95 + 14.0, 0, 255)  # lift floor: terrain min luma > outline #1a1214
    small = Image.fromarray(arr.astype(np.uint8), "RGB").convert("RGBA")
    small = P.quantize(small, pal, dither="bayer2", strength=0.05)
    mask = diamond_mask()
    tiles = []
    rng = np.random.default_rng(1999)
    # per-tile mean-luma normalisation kills the random-crop checkerboard
    g_mean = float(np.asarray(small.convert("L")).mean())
    for _ in range(n):
        x = int(rng.integers(0, small.width - TW))
        y = int(rng.integers(0, small.height - TH))
        t = small.crop((x, y, x + TW, y + TH)).copy()
        ta = np.asarray(t).astype(np.float32)
        t_mean = float(ta[..., :3].mean())
        ta[..., :3] = np.clip(ta[..., :3] + (g_mean - t_mean) * 0.8, 0, 255)
        t = P.quantize(Image.fromarray(ta.astype(np.uint8), "RGBA"), pal, dither="none")
        t.putalpha(mask)
        tiles.append(t)
    return tiles, small


def cut_diamond(plate: Image.Image, x: int, y: int) -> Image.Image:
    """Soma/Mir era technique: the field is one painted plate; each 64x32
    diamond is *cut* from it at its own screen position, so neighbours are
    continuous by construction (no seams, no transition tiles needed inside
    one biome). Shipping tilesets do this per painted 'patch' (e.g. 8x8 tiles)."""
    x %= max(1, plate.width - TW)
    y %= max(1, plate.height - TH)
    t = plate.crop((x, y, x + TW, y + TH)).copy()
    t.putalpha(diamond_mask())
    return t


def make_sprite(path: Path, target_h: int, pal: np.ndarray, *, feet_inset=0, cell=(32, 48), shadow_rx=9, gamma=0.85, contrast=1.0):
    raw = Image.open(path)
    k = P.key_out_green(raw)
    k = P.crop_to_alpha(P.harden_alpha(k))
    s = P.fit_to_cell(k, target_h=target_h)
    s = P.harden_alpha(s)
    sa = np.asarray(s).astype(np.float32)
    lin = sa[..., :3] / 255.0
    lin = lin ** gamma                                   # palette lift (never alpha hacks)
    lin = np.clip((lin - 0.5) * contrast + 0.5, 0, 1)    # a touch more contrast = HB 'crisp' read
    sa[..., :3] = lin * 255.0
    s = Image.fromarray(sa.astype(np.uint8), "RGBA")
    s = P.quantize(s, pal, dither="bayer2", strength=0.10)
    s = P.outline(s)
    return P.place_in_cell(s, cell=cell, feet_y=42 if cell[1] == 48 else cell[1] - 6,
                           shadow_rx=shadow_rx, foot_inset=feet_inset)


def make_tree(path: Path, pal: np.ndarray) -> Image.Image:
    raw = Image.open(path)
    k = P.crop_to_alpha(P.harden_alpha(P.key_out_green(raw)))
    t = P.fit_to_cell(k, target_h=112)  # ~2 tiles tall: Soma zoom signature
    t = P.harden_alpha(t)
    t = P.quantize(t, pal, dither="bayer2", strength=0.06)
    return P.outline(t)


def firebolt_frames(pal_accent) -> list[Image.Image]:
    """4f bolt in 24x12 cells: ember core, arterial rim, soot smoke tail.
    Crowd rule: FX are *shaped* (a comet, not a blob) and never opaque over
    the caster; the 1-frame white contact flash is T-066's white->palette rule."""
    frames = []
    for i in range(4):
        im = Image.new("RGBA", (24, 12), (0, 0, 0, 0))
        d = ImageDraw.Draw(im)
        L = 10 + i * 4
        # smoke tail (soot, dithered)
        for x in range(2, L - 6, 2):
            d.point([(x, 5 + (x // 2) % 2), (x + 1, 6 - (x // 2) % 2)], fill=(0x2A, 0x22, 0x22, 255))
        d.line([(L - 9, 6), (L - 1, 6)], fill=(*ARTERIAL, 255), width=4)
        d.line([(L - 7, 6), (L - 1, 6)], fill=(0xD8, 0x5A, 0x2A, 255), width=2)
        d.ellipse([L - 5, 3, L + 1, 9], fill=(0xF2, 0xC8, 0x8A, 255), outline=(*ARTERIAL, 255))
        d.point([(L - 2, 6)], fill=(0xFF, 0xF4, 0xE0, 255))
        frames.append(im)
    return frames


def hit_flash(cell: Image.Image) -> Image.Image:
    """T-066 frame-1 of the 2-frame hit flash: every opaque non-outline pixel
    goes bone-white for one frame, then back to palette."""
    a = np.asarray(cell.convert("RGBA")).copy()
    body = (a[..., 3] == 255) & ~((a[..., 0] == 0x1A) & (a[..., 1] == 0x12) & (a[..., 2] == 0x14))
    a[body, :3] = (0xF4, 0xEC, 0xE0)
    return Image.fromarray(a, "RGBA")


def draw_callout(canvas: Image.Image, xy, text: str):
    """HB red-caps callout: 1px black-outlined bitmap text, backing plate."""
    d = ImageDraw.Draw(canvas)
    x, y = xy
    w = 6 * len(text)
    x -= w // 2
    for ox, oy in ((-1, 0), (1, 0), (0, -1), (0, 1)):
        d.text((x + ox, y + oy), text, fill=(0, 0, 0, 255))
    d.text((x, y), text, fill=(*CALLOUT_RED, 255))


def main() -> int:
    OUT.mkdir(parents=True, exist_ok=True)
    # ---- palettes: one per family (mob, player, terrain), <=32 each (§4.2)
    rat_raw = P.crop_to_alpha(P.harden_alpha(P.key_out_green(Image.open(PLATES / "marsh_rat_4x_raw.png"))))
    rav_raw = P.crop_to_alpha(P.harden_alpha(P.key_out_green(Image.open(PLATES / "ravager_4x_raw.png"))))
    ground_raw = Image.open(PLATES / "fields_ground_4x_raw.png").convert("RGBA")
    tree_raw = P.crop_to_alpha(P.harden_alpha(P.key_out_green(Image.open(PLATES / "dead_tree_4x_raw.png"))))

    pal_mob = P.build_palette([rat_raw.resize((rat_raw.width // 4, rat_raw.height // 4), Image.BOX)], 32)
    pal_ply = P.build_palette([rav_raw.resize((rav_raw.width // 4, rav_raw.height // 4), Image.BOX)], 32)
    pal_ter = P.build_palette([ground_raw.resize((352, 192), Image.BOX),
                               tree_raw.resize((tree_raw.width // 4, tree_raw.height // 4), Image.BOX)], 32)
    P.palette_strip(pal_mob, OUT / "palette_mob_marsh_rat.png")
    P.palette_strip(pal_ply, OUT / "palette_player_ravager.png")
    P.palette_strip(pal_ter, OUT / "palette_terrain_fields.png")

    # ---- assets at engine scale
    tiles, plate = make_ground_tiles(ground_raw, pal_ter)
    tree = make_tree(PLATES / "dead_tree_4x_raw.png", pal_ter)
    # rat: a low, wide mob — 20 px tall in a 32x48 cell; ravager: 46 px tall (GDD "~56 px incl. headroom")
    rat = make_sprite(PLATES / "marsh_rat_4x_raw.png", 20, pal_mob, shadow_rx=10, gamma=0.80)
    rav = make_sprite(PLATES / "ravager_4x_raw.png", 46, pal_ply, shadow_rx=8, gamma=0.66, contrast=1.15)
    bolt = firebolt_frames(pal_mob)

    for name, im in (("cell_marsh_rat_S.png", rat), ("cell_ravager_S.png", rav), ("scatter_dead_tree.png", tree)):
        im.save(OUT / name)
    for i, t in enumerate(tiles):
        t.save(OUT / f"tile_fields_mud_{i}.png")

    # ---- 256x256 scene: a 5x5 diamond patch, painter-sorted like drawGround()
    W = H = 256
    scene = Image.new("RGBA", (W, H), (0x12, 0x10, 0x10, 255))
    ox, oy = W / 2 - TW / 2, 40  # tile (0,0) top-left
    rng = np.random.default_rng(7)
    ents = []  # (sort_y, image, x, y)
    for ty in range(6):
        for tx in range(6):
            x, y = iso(tx, ty, ox, oy)
            scene.alpha_composite(cut_diamond(plate, int(x), int(y)), (int(x), int(y)))
    # tree anchored on tile (1,1) top-right area; entities anchor feet at tile centre
    def anchor(tx, ty):
        x, y = iso(tx, ty, ox, oy)
        return int(x + TW / 2), int(y + TH / 2)

    tx_, ty_ = anchor(1.0, 0.6)
    ents.append((ty_, tree, tx_ - tree.width // 2, ty_ - tree.height + 10))
    # player on (2.5, 3.2) facing the rat; rat on (3.6, 2.6)
    px, py = anchor(2.4, 3.1)
    ents.append((py, rav, px - 16, py - 42))
    rx, ry = anchor(3.5, 2.5)
    ents.append((ry, hit_flash(rat), rx - 16, ry - 42))  # the struck rat, flash frame
    # second rat further back to prove same-tier silhouette vs pile
    r2x, r2y = anchor(4.6, 1.4)
    ents.append((r2y, rat, r2x - 16, r2y - 42))
    for _, im, x, y in sorted(ents, key=lambda e: e[0]):
        scene.alpha_composite(im, (x, y))
    # spell: firebolt frame 3 travelling from player hand toward rat
    b = bolt[3]
    scene.alpha_composite(b, (px + 10, py - 26))
    # impact spark on the rat (2px ember dots, additive-friendly colours)
    d0 = ImageDraw.Draw(scene)
    d0.point([(rx - 6, ry - 14), (rx - 9, ry - 10), (rx - 4, ry - 18)], fill=(0xF2, 0xC8, 0x8A, 255))
    # blood decal under the far rat (matte, darkest red on screen — §4.4)
    d = ImageDraw.Draw(scene)
    d.ellipse([r2x - 9, r2y - 3, r2x + 7, r2y + 3], fill=(0x3A, 0x08, 0x0C, 255))
    d.point([(r2x - 11, r2y - 1), (r2x + 9, r2y + 1), (r2x - 2, r2y + 5)], fill=(0x3A, 0x08, 0x0C, 255))
    # HB red-caps callout over the caster + name tags (karma bands: lawful/neutral/chaotic)
    draw_callout(scene, (px, py - 62), "FIREBOLT!")
    dd = ImageDraw.Draw(scene)
    for (x, y, label, col) in ((px, py - 52, "Ravager L7", (0xBE, 0xBE, 0xC8)),
                               (rx, ry - 44, "Marsh Rat", (0xBE, 0xBE, 0xC8))):
        w = 6 * len(label)
        dd.text((x - w // 2, y), label, fill=(*col, 255))
    scene.save(OUT / "style_tile_256_day.png")

    # ---- QA views
    night = P.night_floor(scene, hour=2.0)   # deepest key (alpha 145/255 ≈ 57% ≤ 65% cap)
    night.save(OUT / "style_tile_256_night.png")
    P.greyscale(scene).save(OUT / "style_tile_256_grey.png")
    # 3x human-readable board: day | night | grey
    board = Image.new("RGBA", (W * 3 * 3 + 16, H * 3 + 28), (0x0C, 0x0A, 0x0A, 255))
    for i, (lbl, im) in enumerate((("DAY 1x->3x", scene), ("NIGHT hour=02:00 (engine overlay)", night), ("GREYSCALE silhouette test", P.greyscale(scene)))):
        board.alpha_composite(P.upscale(im, 3), (i * (W * 3 + 8), 24))
        ImageDraw.Draw(board).text((i * (W * 3 + 8) + 4, 6), lbl, fill=(0xE6, 0xD2, 0xBE, 255))
    board.convert("RGB").save(OUT / "style_tile_board_3x.png")

    # ---- audit numbers for the report
    def luma(im):
        a = np.asarray(im.convert("RGBA")).astype(np.float32)
        m = a[..., 3] == 255
        return float((a[m][:, 0] * .3 + a[m][:, 1] * .59 + a[m][:, 2] * .11).mean())

    def opaque_colours(im):
        a = np.asarray(im.convert("RGBA"))
        px = a[a[..., 3] == 255][:, :3]
        return int(len(np.unique(px, axis=0)))

    tile_union = Image.fromarray(np.concatenate([np.asarray(t) for t in tiles], axis=1))
    report = {
        "R-LUMA (sprite body mean - terrain mean, want >= 25)": {
            "terrain_mean": round(luma(tile_union), 1),
            "ravager_mean": round(luma(rav), 1), "rat_mean": round(luma(rat), 1),
            "ravager_delta": round(luma(rav) - luma(tile_union), 1),
            "rat_delta": round(luma(rat) - luma(tile_union), 1)},
        "colours_opaque": {"rat_cell": opaque_colours(rat), "ravager_cell": opaque_colours(rav),
                           "tree": opaque_colours(tree), "tiles_union": opaque_colours(tile_union)},
        "colours_incl_shadow_alpha70": {"rat_cell": P.count_colours(rat), "ravager_cell": P.count_colours(rav),
                    "tree": P.count_colours(tree)},
        "palette_sizes": {"mob": int(len(pal_mob)), "player": int(len(pal_ply)), "terrain": int(len(pal_ter))},
        "night_overlay_0200": P.night_overlay(2.0),
        "cells": {"rat": rat.size, "ravager": rav.size, "tree": tree.size},
    }
    (OUT / "style_tile_audit.json").write_text(__import__("json").dumps(report, indent=1) + "\n")
    print(__import__("json").dumps(report, indent=1))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
