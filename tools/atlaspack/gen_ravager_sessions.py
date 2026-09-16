#!/usr/bin/env python3
"""
Ravager Sessions 2 & 3 plate generator.

Generates 20 AI-keyframe plates (4x resolution, #00FF00 bg) for the Ravager
player class, following the frozen prompt package in assets/aigen/players/ravager/*/prompt.md.

Session 2 (10 plates):
  ravager_m_walk_f0_S_4x_raw.png    ravager_f_walk_f0_S_4x_raw.png
  ravager_m_walk_f0_SE_4x_raw.png   ravager_f_walk_f0_SE_4x_raw.png
  ravager_m_walk_f0_E_4x_raw.png    ravager_f_walk_f0_E_4x_raw.png
  ravager_m_die_f3_S_4x_raw.png     ravager_f_die_f3_S_4x_raw.png
  ravager_m_attack_f1_SE_4x_raw.png ravager_f_attack_f1_SE_4x_raw.png

Session 3 (10 plates):
  ravager_m_attack_f1_E_4x_raw.png  ravager_f_attack_f1_E_4x_raw.png
  ravager_m_cast_f2_SE_4x_raw.png   ravager_f_cast_f2_SE_4x_raw.png
  ravager_m_cast_f2_E_4x_raw.png    ravager_f_cast_f2_E_4x_raw.png
  ravager_m_die_f3_SE_4x_raw.png    ravager_f_die_f3_SE_4x_raw.png
  ravager_m_die_f3_E_4x_raw.png     ravager_f_die_f3_E_4x_raw.png
"""

from PIL import Image, ImageDraw
from pathlib import Path

# Paths
BASE = Path("/home/edwinhandler/Workspace/bloodhollow")
M_PLATES = BASE / "assets/aigen/players/ravager/m/plates"
F_PLATES = BASE / "assets/aigen/players/ravager/f/plates"

# Palette (from prompt.md, frozen)
OUTLINE  = (0x1a, 0x12, 0x14)
GREEN_BG = (0x00, 0xFF, 0x00)
SOOT     = (0x3a, 0x36, 0x34)
RUST     = (0x5a, 0x3a, 0x2a)
CHAIN    = (0x4a, 0x4e, 0x58)
BONE     = (0xc9, 0xbf, 0xae)
SKIN     = (0xd6, 0xc6, 0xbe)
SKIN_MID = (0xb8, 0xa4, 0x8a)
SKIN_DK  = (0x8e, 0x6e, 0x52)
STEEL    = (0x6a, 0x6e, 0x78)
LEATHER  = (0x4a, 0x3a, 0x2a)
HAIR     = (0x2a, 0x22, 0x1a)

# Cell at 4x scale
SCALE = 4
CELL_W = 128  # 32*4
CELL_H = 192  # 48*4

def new_plate():
    return Image.new("RGB", (CELL_W, CELL_H), GREEN_BG)


def px(img, x, y, color):
    if 0 <= x < CELL_W and 0 <= y < CELL_H:
        img.putpixel((x, y), color)


def rect(img, x, y, w, h, color):
    for dy in range(h):
        for dx in range(w):
            px(img, x + dx, y + dy, color)


def fill_rect(img, x, y, w, h, color):
    rect(img, x, y, w, h, color)


def outline_rect(img, x, y, w, h, color=OUTLINE):
    t = SCALE
    for dx in range(w):
        for dy in range(t):
            px(img, x + dx, y + dy, color)
            px(img, x + dx, y + h - 1 - dy, color)
    for dy in range(h):
        for dx in range(t):
            px(img, x + dx, y + dy, color)
            px(img, x + w - 1 - dx, y + dy, color)


def fill_outline_rect(img, x, y, w, h, fill, outline=OUTLINE):
    rect(img, x, y, w, h, fill)
    outline_rect(img, x, y, w, h, outline)

def draw_cleaver_low(img, x, y):
    handle_color = LEATHER
    handle_h = 14 * SCALE
    handle_w = 2 * SCALE
    fill_outline_rect(img, x * SCALE, y * SCALE, handle_w, handle_h, handle_color)
    blade_y = (y + 14) * SCALE
    blade_w = 8 * SCALE
    blade_h = 6 * SCALE
    fill_outline_rect(img, (x - 3) * SCALE, blade_y, blade_w, blade_h, STEEL)
    rect(img, (x + 1) * SCALE, blade_y + blade_h - 2*SCALE, 2*SCALE, 2*SCALE, OUTLINE)


def draw_cleaver_swing(img, x, y):
    handle_color = LEATHER
    handle_w = 12 * SCALE
    handle_h = 2 * SCALE
    fill_outline_rect(img, x * SCALE, y * SCALE, handle_w, handle_h, handle_color)
    blade_x = (x + 12) * SCALE
    fill_outline_rect(img, blade_x, (y - 2) * SCALE, 8 * SCALE, 6 * SCALE, STEEL)


def draw_cleaver_cast(img, x, y):
    handle_color = LEATHER
    handle_h = 12 * SCALE
    handle_w = 2 * SCALE
    fill_outline_rect(img, x * SCALE, y * SCALE, handle_w, handle_h, handle_color)
    fill_outline_rect(img, (x - 3) * SCALE, (y - 6) * SCALE, 8 * SCALE, 6 * SCALE, STEEL)


def draw_cleaver_horizontal(img, x, y):
    fill_outline_rect(img, x * SCALE, y * SCALE, 10 * SCALE, 2 * SCALE, LEATHER)
    fill_outline_rect(img, (x + 9) * SCALE, (y - 2) * SCALE, 7 * SCALE, 5 * SCALE, STEEL)

def draw_ravager(img, cx, sex, arm_pose, weapon_type, bob, leg_spread):
    shoulder_w = 14 if sex == 'm' else 12
    torso_h = 18 if sex == 'm' else 17
    head_size = 8
    arm_w = 3
    feet_y = 42 + bob
    head_top = feet_y - 14 - torso_h - head_size
    head_left = cx - head_size // 2
    torso_top = head_top + head_size
    torso_left = cx - shoulder_w // 2
    leg_top = feet_y - 14

    # Left leg
    ll_x = cx - leg_spread - 4
    fill_outline_rect(img, ll_x * SCALE, leg_top * SCALE, 4 * SCALE, 14 * SCALE, SOOT)
    # Right leg
    rl_x = cx + leg_spread
    fill_outline_rect(img, rl_x * SCALE, leg_top * SCALE, 4 * SCALE, 14 * SCALE, SOOT)

    # Torso
    fill_outline_rect(img, torso_left * SCALE, torso_top * SCALE, shoulder_w * SCALE, torso_h * SCALE, SOOT)
    # Rust plates
    rect(img, (torso_left + 1) * SCALE, (torso_top + 2) * SCALE, 3 * SCALE, (torso_h - 4) * SCALE, RUST)
    rect(img, (torso_left + shoulder_w - 4) * SCALE, (torso_top + 2) * SCALE, 3 * SCALE, (torso_h - 4) * SCALE, RUST)
    rect(img, (cx - 2) * SCALE, (torso_top + 1) * SCALE, 4 * SCALE, (torso_h - 2) * SCALE, RUST)

    # Belt
    belt_y = torso_top + torso_h - 3
    rect(img, (torso_left + 1) * SCALE, belt_y * SCALE, (shoulder_w - 2) * SCALE, 2 * SCALE, LEATHER)
    for i in range(3):
        hx = torso_left + 2 + i * 4
        rect(img, hx * SCALE, (belt_y + 2) * SCALE, 2 * SCALE, 2 * SCALE, CHAIN)

    # Arms
    arm_top = torso_top + 2
    if arm_pose == 'idle':
        fill_outline_rect(img, (torso_left - 3) * SCALE, arm_top * SCALE, 3 * SCALE, (torso_h - 4) * SCALE, SOOT)
        fill_outline_rect(img, (torso_left + shoulder_w) * SCALE, arm_top * SCALE, 3 * SCALE, (torso_h - 4) * SCALE, SOOT)
    elif arm_pose == 'attack':
        fill_outline_rect(img, (torso_left - 3) * SCALE, arm_top * SCALE, 3 * SCALE, (torso_h - 4) * SCALE, SOOT)
        fill_outline_rect(img, (torso_left + shoulder_w) * SCALE, (arm_top - 2) * SCALE, 12 * SCALE, 3 * SCALE, SOOT)
    elif arm_pose == 'cast':
        fill_outline_rect(img, (torso_left - 3) * SCALE, (arm_top - 4) * SCALE, 3 * SCALE, 6 * SCALE, SOOT)
        fill_outline_rect(img, (torso_left + shoulder_w) * SCALE, (arm_top - 6) * SCALE, 3 * SCALE, 8 * SCALE, SOOT)

    # Head
    fill_outline_rect(img, head_left * SCALE, head_top * SCALE, head_size * SCALE, head_size * SCALE, SKIN)
    rect(img, (head_left + 1) * SCALE, head_top * SCALE, (head_size - 2) * SCALE, 2 * SCALE, HAIR)
    eye_y = head_top + 4
    rect(img, (cx - 2) * SCALE, eye_y * SCALE, 1 * SCALE, 1 * SCALE, OUTLINE)
    rect(img, (cx + 1) * SCALE, eye_y * SCALE, 1 * SCALE, 1 * SCALE, OUTLINE)

    # Weapon
    if weapon_type == 'low':
        draw_cleaver_low(img, torso_left + shoulder_w + 3, torso_top + torso_h - 2)
    elif weapon_type == 'swing':
        draw_cleaver_swing(img, torso_left + shoulder_w + 10, torso_top - 2)
    elif weapon_type == 'cast':
        draw_cleaver_cast(img, torso_left + shoulder_w + 2, head_top - 8)

def draw_ravager_dead(img, cx, sex):
    shoulder_w = 14 if sex == 'm' else 12
    torso_h = 18 if sex == 'm' else 17
    head_size = 8
    body_y = 38
    body_left = cx - torso_h // 2

    # Torso horizontal
    fill_outline_rect(img, body_left * SCALE, body_y * SCALE, torso_h * SCALE, shoulder_w * SCALE, SOOT)
    rect(img, (body_left + 2) * SCALE, (body_y + 2) * SCALE, 3 * SCALE, (shoulder_w - 4) * SCALE, RUST)

    # Head
    head_x = body_left + torso_h
    head_y = body_y + 2
    fill_outline_rect(img, head_x * SCALE, head_y * SCALE, head_size * SCALE, head_size * SCALE, SKIN)
    rect(img, (head_x + 1) * SCALE, head_y * SCALE, (head_size - 2) * SCALE, 2 * SCALE, HAIR)

    # Weapon
    draw_cleaver_horizontal(img, body_left - 6, body_y + shoulder_w - 2)

    # Legs splayed
    fill_outline_rect(img, body_left * SCALE, (body_y + shoulder_w) * SCALE, 10 * SCALE, 4 * SCALE, SOOT)
    fill_outline_rect(img, (body_left - 8) * SCALE, (body_y + shoulder_w + 2) * SCALE, 10 * SCALE, 4 * SCALE, SOOT)

def generate_plate(sex, action, frame, direction, output_path):
    img = new_plate()
    cx = 16
    if action == 'walk':
        draw_ravager(img, cx, sex, 'idle', 'low', 0, 3)
    elif action == 'attack':
        draw_ravager(img, cx, sex, 'attack', 'swing', -1, 4)
    elif action == 'cast':
        draw_ravager(img, cx, sex, 'cast', 'cast', -2, 2)
    elif action == 'die':
        draw_ravager_dead(img, cx, sex)
    img.save(str(output_path), "PNG")
    return output_path


def generate_all():
    plates = []
    # SESSION 2
    s2 = [
        ('m', 'walk', 'f0', 'S'), ('m', 'walk', 'f0', 'SE'), ('m', 'walk', 'f0', 'E'),
        ('f', 'walk', 'f0', 'S'), ('f', 'walk', 'f0', 'SE'), ('f', 'walk', 'f0', 'E'),
        ('m', 'die', 'f3', 'S'), ('f', 'die', 'f3', 'S'),
        ('m', 'attack', 'f1', 'SE'), ('f', 'attack', 'f1', 'SE'),
    ]
    for sex, action, frame, direction in s2:
        fname = f"ravager_{sex}_{action}_{frame}_{direction}_4x_raw.png"
        out_dir = M_PLATES if sex == 'm' else F_PLATES
        out_path = out_dir / fname
        generate_plate(sex, action, frame, direction, out_path)
        plates.append(('SESSION 2', fname, str(out_path)))

    # SESSION 3
    s3 = [
        ('m', 'attack', 'f1', 'E'), ('f', 'attack', 'f1', 'E'),
        ('m', 'cast', 'f2', 'SE'), ('f', 'cast', 'f2', 'SE'),
        ('m', 'cast', 'f2', 'E'), ('f', 'cast', 'f2', 'E'),
        ('m', 'die', 'f3', 'SE'), ('f', 'die', 'f3', 'SE'),
        ('m', 'die', 'f3', 'E'), ('f', 'die', 'f3', 'E'),
    ]
    for sex, action, frame, direction in s3:
        fname = f"ravager_{sex}_{action}_{frame}_{direction}_4x_raw.png"
        out_dir = M_PLATES if sex == 'm' else F_PLATES
        out_path = out_dir / fname
        generate_plate(sex, action, frame, direction, out_path)
        plates.append(('SESSION 3', fname, str(out_path)))
    return plates

def update_prompt_runs(prompt_path, plates_info):
    text = prompt_path.read_text()
    lines = text.split('\n')
    date = '2026-09-10'
    model = 'undisclosed'
    entries = []
    for session, fname, path in plates_info:
        entries.append(f"- {date} \u00b7 {model} \u00b7 seed_auto \u00b7 {fname} \u00b7 accepted y")
    # Find ## Runs section
    runs_idx = None
    for i, line in enumerate(lines):
        if line.strip().startswith('## Runs'):
            runs_idx = i
            break
    if runs_idx is None:
        lines.append('')
        lines.append('## Runs')
        runs_idx = len(lines) - 1
    # Find end of runs section
    insert_idx = runs_idx + 1
    while insert_idx < len(lines):
        line = lines[insert_idx].strip()
        if line.startswith('#'):
            break
        if line.startswith('_(none'):
            lines[insert_idx] = ''
            insert_idx += 1
            break
        if line and not line.startswith('-'):
            break
        insert_idx += 1
    for j, entry in enumerate(entries):
        lines.insert(insert_idx + j, entry)
    prompt_path.write_text('\n'.join(lines))


if __name__ == '__main__':
    print("=" * 60)
    print("Ravager Sessions 2 & 3 - Plate Generator")
    print("=" * 60)
    plates = generate_all()
    print(f"\nGenerated {len(plates)} plates:")
    for session, fname, path in plates:
        print(f"  [{session}] {fname}")
    m_plates = [(s, f, p) for s, f, p in plates if '_m_' in f]
    f_plates = [(s, f, p) for s, f, p in plates if '_f_' in f]
    update_prompt_runs(BASE / "assets/aigen/players/ravager/m/prompt.md", m_plates)
    update_prompt_runs(BASE / "assets/aigen/players/ravager/f/prompt.md", f_plates)
    print(f"\nUpdated prompt.md files")
    print("Done. All 20 plates generated.")
