#!/usr/bin/env python3
"""bh_terrain — B1/B2 terrain chain: painterly 4x plates → 512×256 ground plates
(zone palette ≤ 32, D3 floor ≈ 51, min-luma clamp ≥ 24) → D12 edge pieces per
manifest adjacency pair (8 pieces × N variants, organic-over-built, WALL never
bleeds) → prism skin (top 64×32 + left/right 32×28 faces + footing skirt) →
QA (plate gates via bh_qa_sheet, edge seam audit, map preview board).

Usage
  python3 tools/atlaspack/bh_terrain.py <zone> [--variants 3] [--preview]

Zone recipe = ZONES[zone] below (terrain id → raw plate file). Everything is
deterministic (fixed seeds) and stays inside assets/aigen/terrain/<zone>/ and
docs/research-notes/qa/. Output layout = REGISTRY.md "Terrain" section:
  plates/<id>_<NAME>.png            512×256 RGBA, cut by bhpix.cut_diamond at world px
  edges/<A>_<B>/v<k>/{edge_NE,edge_SE,edge_SW,edge_NW,corner_N,corner_E,corner_S,corner_W}.png
                                    64×32 RGBA alpha-cut overlays: material B (the bleeder)
                                    opaque where it creeps onto the A tile, transparent elsewhere
  prism/{top,left,right,skirt_*}.png  WALL skin (engine iso::drawPrism, 28 px)
  palette_<zone>.png                the ≤ 32 zone strip
  terrain.json                      lookup manifest (ids, plates, pairs, bleed direction, variants)
UNVALIDATED in engine: there is no textured-ground renderer yet (T-ART-12 / D6).
"""
from __future__ import annotations

import argparse
import json
import subprocess
import sys
from pathlib import Path

import numpy as np
from PIL import Image, ImageEnhance, ImageFilter

sys.path.insert(0, str(Path(__file__).resolve().parent))
import bhpix as P  # noqa: E402
import make_style_tile as M  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
TER = ROOT / "assets/aigen/terrain"
QA = ROOT / "docs/research-notes/qa"
TW, TH = 64, 32
PLATE_W, PLATE_H = 512, 256
PRISM_H = 28  # game.cpp:696 drawPrism(..., 28, ...)

# D12: organic-over-built. Rank = how "organic/soft" a material is; the higher rank
# bleeds onto the lower one. WALL is never in this table — it gets the footing skirt.
BLEED_RANK = {"WATER": 6, "MUD": 5, "GRASS": 4, "DARKGRASS": 4, "DIRT": 3, "PATH": 1, "WOOD": 0,
              "BONEPIT": 5, "CANDLE": 5, "SLAB": 0, "FLOOR": 1, "STONE": 1}
# explicit rulings from B0-GATE-DECISION D12 (override rank when listed)
BLEED_RULES = {("GRASS", "PATH"): "GRASS", ("MUD", "GRASS"): "MUD", ("WATER", "DIRT"): "WATER",
               ("BONEPIT", "FLOOR"): "BONEPIT", ("CANDLE", "FLOOR"): "CANDLE",
               ("GRASS", "DARKGRASS"): "DARKGRASS", ("GRASS", "WATER"): "WATER", ("GRASS", "DIRT"): "GRASS",
               ("PATH", "DARKGRASS"): "DARKGRASS", ("WATER", "PATH"): "WATER", ("DIRT", "PATH"): "DIRT",
               ("PATH", "MUD"): "MUD", ("DIRT", "DARKGRASS"): "DARKGRASS", ("GRASS", "MUD"): "MUD",
               ("WALL", "*"): None}

ZONES = {
    "town": {
        "manifest": "MAPS_thornwall.json",
        "plates": {0: ("GRASS", "turf"), 1: ("DIRT", "plaza_dirt"), 3: ("WATER", "river"), 5: ("PATH", "cobble"),
                   6: ("MUD", "lane_mud"), 7: ("DARKGRASS", "graveyard_turf")},
        "prism": "palisade_face",
        "prism_top": "face:0.78",      # top of a palisade = the plank ends (face paint, 22 % darker)
        "wood": None,                   # id 4 WOOD (chapel floor, 0.4 %) — plate shared from the crypt gangway in B2; town uses PATH plate until then
    },
    "fields": {
        "manifest": "MAPS_fields_overflow.json",
        "plates": {0: ("GRASS", "furrow_turf"), 5: ("PATH", "cart_track"), 6: ("MUD", "field_mud"),
                   7: ("DARKGRASS", "graveyard_turf")},   # shadow turf = the town graveyard plate re-cut darker (see fields recipe)
        "prism": "hedge_face",
        "prism_top": "face:0.92",       # hedge top = the bramble itself
        "wood": None,
    },
}
# raw sources that live in another zone's raw/ folder (shared paint, separate usage manifests)
SHARED_RAW = {("fields", "furrow_turf"): ROOT / "docs/research-notes/style-tile/plates/fields_ground_4x_raw.png",
              ("fields", "graveyard_turf"): TER / "town/raw/graveyard_turf_4x_raw.png"}
DARKEN = {("fields", "graveyard_turf"): 0.82}  # shadow turf: same paint, 18 % darker (still ≥ 24 after clamp)
# D3 mean targets per material (rulebook window 45–70; ≤ 54 keeps the darkest B4 mob at Δnight ≥ 15, see normalise_mean)
MEAN_TARGET = {"DARKGRASS": 47.5, "WATER": 48.0, "MUD": 49.5}
MEAN_DEFAULT = 51.0


# ----------------------------------------------------------------------------
def raw_path(zone: str, name: str) -> Path:
    return SHARED_RAW.get((zone, name), TER / zone / "raw" / f"{name}_4x_raw.png")


def reduce_plate(src: Image.Image, *, darken: float = 1.0) -> Image.Image:
    """4x painterly raw → 512×256 low-frequency plate (B0 make_ground_tiles recipe:
    box reduce → blur 0.8 → contrast 0.70 → floor lift; D3 keeps the mean ≈ 51)."""
    im = src.convert("RGB")
    # crop to 2:1 from the centre before reducing so the reduction is isotropic
    w, h = im.size
    if w / h > 2.0:
        nw = int(h * 2.0); im = im.crop(((w - nw) // 2, 0, (w - nw) // 2 + nw, h))
    elif w / h < 2.0:
        nh = int(w / 2.0); im = im.crop((0, (h - nh) // 2, w, (h - nh) // 2 + nh))
    small = im.resize((PLATE_W, PLATE_H), Image.BOX).filter(ImageFilter.GaussianBlur(0.8))
    small = ImageEnhance.Contrast(small).enhance(0.70)
    arr = np.asarray(small).astype(np.float32)
    arr = np.clip(arr * 0.95 * darken + 14.0, 0, 255)
    return Image.fromarray(arr.astype(np.uint8), "RGB").convert("RGBA")


def normalise_mean(img: Image.Image, target: float, tol: float = 1.5) -> tuple[Image.Image, float]:
    """Pull the plate mean onto the D3 floor (≈ 51) by a pure gain; returns (img, gain).
    Window is deliberately tight (49–53, inside the rulebook's 45–70): R-LUMA night is
    Δnight = Δday × (1 − 145/255) ≈ 0.43·Δday, so the ≥ 15 night gate needs Δday ≥ 34.8,
    and the darkest fields mob (Waxen Celebrant, body 89.7) only clears that on plates ≤ 54.
    Material difference is carried by texture/hue, not by mean value (Soma rule)."""
    a = np.asarray(img).astype(np.float32)
    lum = a[..., 0] * .299 + a[..., 1] * .587 + a[..., 2] * .114
    m = float(lum.mean())
    if abs(m - target) <= tol:
        return img, 1.0
    gain = target / max(1.0, m)
    a[..., :3] = np.clip(a[..., :3] * gain, 0, 255)
    return Image.fromarray(a.astype(np.uint8), "RGBA"), round(gain, 3)


def make_seamless(img: Image.Image, blend_px: int = 96) -> Image.Image:
    """Wrap-blend the plate borders so cut_diamond's modulo wrap has no hard seam
    (the AI plates are 'tileable' in spirit only). Roll-and-mask: the plate rolled by
    (w/2, h/2) is wrap-continuous at the borders by construction; it replaces the original
    across a `blend_px` ramp at each border (weight 0 at the border → 1 in the interior),
    so col 0 / col w-1 (row 0 / row h-1) are adjacent centre columns of the paint.
    Applied before quantize so the fade is re-dithered onto the ramp."""
    a = np.asarray(img.convert("RGBA")).astype(np.float32)
    h, w = a.shape[:2]
    r = np.roll(np.roll(a, w // 2, axis=1), h // 2, axis=0)
    bx, by = min(blend_px, w // 2), min(blend_px, h // 2)
    mx = np.ones(w, np.float32); ramp = (np.arange(bx, dtype=np.float32) + 0.5) / bx
    mx[:bx] = ramp; mx[w - bx:] = ramp[::-1]
    my = np.ones(h, np.float32); ramp = (np.arange(by, dtype=np.float32) + 0.5) / by
    my[:by] = ramp; my[h - by:] = ramp[::-1]
    m = (my[:, None] * mx[None, :])[..., None]
    out = a.copy()
    out[..., :3] = a[..., :3] * m + r[..., :3] * (1.0 - m)
    return Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), "RGBA")


def finish_plate(img: Image.Image, pal: np.ndarray) -> Image.Image:
    q = P.quantize(img, pal, dither="bayer2", strength=0.05)
    q = M.clamp_plate_floor(q)              # min-luma ≥ 24 (the B0 carried fix, now on by default for B1+)
    q = P.quantize(q, pal, dither="none")   # snap the lifted pixels back onto the ramp
    q = M.clamp_plate_floor(q)              # ramp colours below the floor would re-break it; second pass is a no-op when the ramp is clean
    return q


# ----------------------------------------------------------------------------
def bleeder(a: str, b: str) -> str | None:
    """Which of the two materials creeps over the other (D12). None = WALL pair (skirt)."""
    if "WALL" in (a, b):
        return None
    for (x, y), who in BLEED_RULES.items():
        if {x, y} == {a, b}:
            return who
    ra, rb = BLEED_RANK.get(a, 2), BLEED_RANK.get(b, 2)
    if ra == rb:
        return sorted((a, b))[0]
    return a if ra > rb else b


def edge_pieces(plate_a: Image.Image, plate_b: Image.Image, pal: np.ndarray, *, seed: int, cut_xy: tuple[int, int],
                depth: float, cap: float, feather: int) -> dict[str, Image.Image]:
    """8 alpha-cut overlay pieces: B (the bleeder) painted over an A tile, transparent
    where A shows. cut_xy = plate offset used for both materials so variants differ."""
    masks = P.edge_masks(depth=depth, cap=cap, feather=feather, seed=seed)
    ta = P.cut_diamond(plate_a, *cut_xy)
    tb = P.cut_diamond(plate_b, cut_xy[0] + 96, cut_xy[1] + 64)
    out = {}
    for name, m in masks.items():
        comp = P.blend_edge(ta, tb, m, palette=pal)
        # overlay form: keep exactly the pixels blend_edge assigned to B (same Bayer decision → era checker band survives)
        cc = np.asarray(comp).astype(int)
        mm = np.asarray(m)
        bay = np.tile(P._BAYER2 + 0.5 + 0.125, (TH // 2 + 1, TW // 2 + 1))[:TH, :TW]
        use_b = (mm >= 255) | ((mm > 0) & (bay < mm / 255.0))
        piece = np.zeros_like(cc, dtype=np.uint8)
        piece[use_b] = cc[use_b]
        piece[..., 3] = np.where(use_b & (cc[..., 3] > 0), 255, 0)
        out[name] = Image.fromarray(piece, "RGBA")
    return out


def seam_audit(pieces: dict[str, Image.Image]) -> dict:
    """Every pixel on the shared face must be B (opaque) so the piece butts against the
    full-B neighbour without a gap — the A12 'face_all_B' rule, re-checked on the art."""
    dm = np.asarray(P.diamond_mask()) > 0
    ys, xs = np.mgrid[0:TH, 0:TW]
    u = (xs + 0.5 - TW / 2) / (TW / 2); v = (ys + 0.5 - TH / 2) / (TH / 2)
    res = {}
    for face in ("NE", "SE", "SW", "NW"):
        d = P._face_depth(u, v, face)
        ring = dm & (d < 2.0 / TH * 1.01)          # outermost pixel ring on that face
        a = np.asarray(pieces[f"edge_{face}"])[..., 3] > 0
        res[face] = {"face_px": int(ring.sum()), "face_all_B": bool((a | ~ring).all()),
                     "b_frac": round(float(a.sum() / dm.sum()), 3)}
    return res


# ----------------------------------------------------------------------------
def prism_skin(face_raw: Image.Image, top_plate: Image.Image, pal: np.ndarray, *, seed: int = 3) -> dict[str, Image.Image]:
    """Skin for iso::drawPrism(28): top 64×32 diamond (cut from a plate), left/right faces
    32×28 parallelogram-ready strips (the engine draws faces as sheared quads — the strip is
    stored upright, the renderer shears it), plus a 'skirt' = the footing course as an
    alpha-cut overlay for the ground tile in front of the wall (D12: WALL never bleeds,
    the ground shows a footing skirt instead)."""
    im = face_raw.convert("RGB")
    w, h = im.size
    # elevation strip: bottom ~22 % is the footing course; reduce the whole face to 28 px tall
    strip = im.resize((max(64, int(w * (PRISM_H / h))), PRISM_H), Image.BOX).filter(ImageFilter.GaussianBlur(0.6))
    strip = ImageEnhance.Contrast(strip).enhance(0.75)
    arr = np.clip(np.asarray(strip).astype(np.float32) * 0.95 + 10.0, 0, 255).astype(np.uint8)
    strip = Image.fromarray(arr, "RGB").convert("RGBA")
    rng = np.random.default_rng(seed)
    x0 = int(rng.integers(0, max(1, strip.width - 64)))
    left = strip.crop((x0, 0, x0 + 32, PRISM_H))
    right = strip.crop((x0 + 32, 0, x0 + 64, PRISM_H))
    # right face is lit from the top-left (engine: right colour darker) → darken 18 %
    ra = np.asarray(right).astype(np.float32); ra[..., :3] *= 0.82
    right = Image.fromarray(ra.astype(np.uint8), "RGBA")
    la = np.asarray(left).astype(np.float32); la[..., :3] *= 0.94
    left = Image.fromarray(la.astype(np.uint8), "RGBA")
    left = P.quantize(left, pal, dither="none"); right = P.quantize(right, pal, dither="none")
    top = P.cut_diamond(top_plate, 128, 96)
    ta = np.asarray(top).astype(np.float32); ta[..., :3] = np.clip(ta[..., :3] * 1.08, 0, 255)
    top = P.quantize(Image.fromarray(ta.astype(np.uint8), "RGBA"), pal, dither="none")
    top.putalpha(P.diamond_mask())
    # skirt: footing stones creeping 4 px onto the ground tile along the wall's two visible faces
    masks = P.edge_masks(depth=0.28, cap=0.22, feather=1, seed=seed)
    foot = strip.crop((x0, PRISM_H - 8, x0 + 64, PRISM_H)).resize((64, 32), Image.NEAREST)
    foot = P.quantize(foot, pal, dither="none")
    skirt = {}
    for name in ("edge_NE", "edge_SE", "edge_SW", "edge_NW", "corner_N", "corner_E", "corner_S", "corner_W"):
        mm = np.asarray(masks[name]); fa = np.asarray(foot)
        piece = np.zeros_like(fa); sel = mm >= 128
        piece[sel] = fa[sel]; piece[..., 3] = np.where(sel, 255, 0)
        skirt[f"skirt_{name}"] = Image.fromarray(piece, "RGBA")
    return {"top": top, "left": left, "right": right, **skirt}


# ----------------------------------------------------------------------------
def build(zone: str, variants: int, preview: bool) -> int:
    rec = ZONES[zone]
    zdir = TER / zone
    (zdir / "plates").mkdir(parents=True, exist_ok=True)
    (zdir / "edges").mkdir(exist_ok=True); (zdir / "prism").mkdir(exist_ok=True)
    QA.mkdir(parents=True, exist_ok=True)
    mf = json.loads((zdir / rec["manifest"]).read_text())
    names = {int(k): v for k, v in mf["terrain_ids"].items()}

    # 1) raw → reduced plates (pre-palette), zone palette from the union of all plates + the prism face
    reduced: dict[int, Image.Image] = {}
    gains = {}
    for tid, (tname, raw) in rec["plates"].items():
        src = Image.open(raw_path(zone, raw))
        r = reduce_plate(src, darken=DARKEN.get((zone, raw), 1.0))
        r, g = normalise_mean(r, MEAN_TARGET.get(tname, MEAN_DEFAULT))
        gains[tname] = g
        reduced[tid] = make_seamless(r)
    face_raw = Image.open(raw_path(zone, rec["prism"]))
    face_small = face_raw.convert("RGB").resize((256, 64), Image.BOX).convert("RGBA")
    pal = P.build_palette(list(reduced.values()) + [face_small], 32, reserve_outline=False)
    # D3 / R-LUMA: the ramp must not contain colours below the floor, or the clamp fights the quantize
    pal = np.array([c for c in pal if (c[0] * .299 + c[1] * .587 + c[2] * .114) >= M.PLATE_MIN_LUMA], np.uint8)
    P.palette_strip(pal, zdir / f"palette_{zone}.png")

    plates: dict[int, Image.Image] = {}
    plate_stats = {}
    for tid, img in reduced.items():
        tname = names[tid]
        q = finish_plate(img, pal)
        lm = float((np.asarray(q).astype(np.float32)[..., :3] @ [.299, .587, .114]).mean())
        if lm < 45.5 or lm > 69.5:   # the ramp snap moved the mean out of the rulebook window → one corrective gain pass
            corr = (46.5 if lm < 45.5 else 68.5) / max(1.0, lm)
            a = np.asarray(img).astype(np.float32); a[..., :3] = np.clip(a[..., :3] * corr, 0, 255)
            q = finish_plate(Image.fromarray(a.astype(np.uint8), "RGBA"), pal)
            gains[tname] = round(gains[tname] * corr, 3)
        out = zdir / "plates" / f"{tid}_{tname}.png"
        q.save(out)
        plates[tid] = q
        a = np.asarray(q).astype(np.float32); lum = a[..., 0] * .299 + a[..., 1] * .587 + a[..., 2] * .114
        plate_stats[tname] = {"file": str(out.relative_to(ROOT)), "luma_mean": round(float(lum.mean()), 1),
                              "luma_min": round(float(lum.min()), 1), "colours": P.count_colours(q), "gain": gains[tname],
                              "raw": str(raw_path(zone, rec["plates"][tid][1]).relative_to(ROOT))}
    # WOOD (id 4) fallback for the town chapel floor until B2 supplies the gangway plate
    if 4 in names and 4 not in plates and 5 in plates:
        plate_stats["WOOD"] = {"file": None, "note": "id 4 WOOD uses the PATH plate until the B2 crypt gangway plate lands (0.4 % of the town map)"}

    # 2) edges per manifest pair (D12)
    pairs_out = []
    seams = {}
    depth_by_bleeder = {"WATER": 0.7, "MUD": 0.6, "GRASS": 0.55, "DARKGRASS": 0.55, "DIRT": 0.5}
    for i, pr in enumerate(mf["adjacency_pairs"]):
        a, b, n = pr["a"], pr["b"], pr["boundary_len"]
        if n < 4:
            continue
        who = bleeder(a, b)
        entry = {"a": a, "b": b, "boundary_len": n, "bleeder": who}
        if who is None:
            entry["pieces"] = "prism/skirt_* (WALL never bleeds — footing skirt on the ground tile)"
            pairs_out.append(entry); continue
        other = a if who == b else b
        ida = next(k for k, v in names.items() if v == other)
        idb = next(k for k, v in names.items() if v == who)
        if ida not in plates or idb not in plates:
            entry["pieces"] = f"SKIPPED — no plate for {other if ida not in plates else who} in this zone"
            pairs_out.append(entry); continue
        pdir = zdir / "edges" / f"{other}_{who}"
        entry["dir"] = str(pdir.relative_to(ROOT)); entry["variants"] = variants
        entry["base_tile"] = other; entry["overlay"] = who
        for k in range(variants):
            vdir = pdir / f"v{k}"; vdir.mkdir(parents=True, exist_ok=True)
            seed = 1999 + 17 * i + k
            rng = np.random.default_rng(seed)
            cut = (int(rng.integers(0, PLATE_W - TW)), int(rng.integers(0, PLATE_H - TH)))
            pieces = edge_pieces(plates[ida], plates[idb], pal, seed=seed, cut_xy=cut,
                                 depth=depth_by_bleeder.get(who, 0.55), cap=0.45, feather=2)
            for name, im in pieces.items():
                im.save(vdir / f"{name}.png")
            seams[f"{other}_{who}/v{k}"] = seam_audit(pieces)
        pairs_out.append(entry)

    # 3) prism skin
    if rec["prism_top"].startswith("face:"):   # top cut from the face paint itself (plank ends / bramble), darkened
        gain = float(rec["prism_top"].split(":")[1])
        top_src = finish_plate(make_seamless(reduce_plate(face_raw, darken=gain)), pal)
    else:
        top_src = plates[next(k for k, (nm, raw) in rec["plates"].items() if raw == rec["prism_top"])]
    skin = prism_skin(face_raw, top_src, pal)
    for name, im in skin.items():
        im.save(zdir / "prism" / f"{name}.png")

    # 4) manifest
    tj = {"zone": zone, "map": mf["map"], "engine_validated": False, "status": "UNVALIDATED in engine (no textured-ground renderer; T-ART-12 / D6)",
          "tile": [TW, TH], "plate": [PLATE_W, PLATE_H], "cut": "bhpix.cut_diamond(plate, world_px_x, world_px_y) — plate coords = world px mod plate",
          "prism_px": PRISM_H, "palette": f"palette_{zone}.png", "palette_colours": int(len(pal)),
          "terrain_ids": names, "plates": plate_stats,
          "bleed_rule": "D12 organic-over-built: overlay = the bleeder's pieces drawn on the base tile whose 8-neighbour set contains the bleeder (faces first, points only when neither adjoining face bleeds — bhpix.edge_mask_for); WALL never bleeds: ground tiles touching a WALL draw prism/skirt_* instead",
          "pairs": pairs_out, "edge_seam_audit": seams}
    (zdir / "terrain.json").write_text(json.dumps(tj, indent=1) + "\n")

    # 5) QA: plate gates via the mandatory tool, one triptych per plate
    rc = 0
    for tid, q in plates.items():
        tname = names[tid]
        r = subprocess.run([sys.executable, str(Path(__file__).parent / "bh_qa_sheet.py"), str(zdir / "plates" / f"{tid}_{tname}.png"),
                            "--kind", "plate", "--out", str(QA / f"b1_{zone}_plate_{tid}_{tname}")], capture_output=True, text=True)
        if r.returncode != 0:
            rc = 1; print(f"PLATE QA FAIL {zone} {tname}: {r.stdout[-300:]}")
    bad = [k for k, v in seams.items() if not all(f["face_all_B"] for f in v.values())]
    if bad:
        rc = 1; print("EDGE SEAM FAIL", bad)
    if preview:
        from bh_terrain_preview import render_map_preview  # local helper (same folder)
        render_map_preview(zone, plates, names, mf, skin, zdir, pal)
    print(json.dumps({"zone": zone, "plates": {k: (v.get("luma_mean"), v.get("luma_min"), v.get("colours")) for k, v in plate_stats.items()},
                      "palette": int(len(pal)), "pairs": len(pairs_out), "edge_dirs": len([p for p in pairs_out if "dir" in p]), "qa_rc": rc}, indent=1))
    return rc


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("zone", choices=sorted(ZONES))
    ap.add_argument("--variants", type=int, default=3)
    ap.add_argument("--preview", action="store_true")
    a = ap.parse_args()
    return build(a.zone, a.variants, a.preview)


if __name__ == "__main__":
    raise SystemExit(main())
