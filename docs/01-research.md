# 01 — Research: the four ancestors

Goal: extract the *systemic DNA* of Helbreath, Lineage 1, Dark Eden, and Legend of
Mir 2 so BLOODHOLLOW steals deliberately instead of vaguely. Every game on this list
still has live servers and active communities in 2025–2026, which is itself a finding:
**the formula is durable and underserved by modern remakes.**

---

## 1. Helbreath (1999, Siementech) — the war-grind sandbox

Two nations, **Aresden vs Elvine**; at level 19 you must swear to one city before you
may level further ([source](https://www.helbreathnemesis.com/guide-how-to-play.php)).

| System | How it works | Steal? |
|---|---|---|
| Stats | STR (carry/weapon reqs/dmg), VIT (HP+regen), DEX (hit/evade), INT (spell-circle unlocks), MAG (MP/cast chance/dmg), CHR (prices, guild creation at 20+) ([source](https://www.helbreathnemesis.com/guide-how-to-play.php)) | **Yes, almost verbatim** — 6 stats, 3 points/level. |
| Combat | Click-to-move, deliberate swing/cast timing, pot weaving, kiting, terrain/door abuse ([source](https://steamcommunity.com/sharedfiles/filedetails/?id=3556235259)) | Yes — timing-based, not GCD-based. |
| EK culture | Public **Enemy-Kill leaderboard** driving nation pride ([source](https://www.helbreath.net/top-eks)) | **Yes** — cheap to build, generates endless drama. |
| Crusade | Scheduled nation war; modern private servers log wars lasting **5–9 hours** ([source](http://www.helbreathnemesis.com/crusadestats.php)) | Post-MVP: monthly mega-event. |
| Skill system | Weapon skills as % (Fencing/Axe/… up to 700%), trade skills (Mining, Alchemy, Enchanting, Salvaging) ([source](https://www.helbreathnemesis.com/guide-how-to-play.php)) | Partially: usage-based % skills for weapons only, post-MVP. |
| Economy | Farming/mining/alchemy/crafting/enchanting; statted "rare drops" are the chase ([source](https://steamcommunity.com/sharedfiles/filedetails/?id=2060939188)) | Yes — crafting-lite at MVP, deepen later. |

**Lessons:** nation binary (2 cities) concentrates PvP better than 3+ factions.
Stat-point allocation *is* the character build — no respec needed if points are
plentiful and gear requirements are the real gate.

## 2. Lineage 1 (1998, NCSoft) — the politics engine

| System | How it works | Steal? |
|---|---|---|
| Alignment | Karma: >500 lawful, 0–500 neutral, <0 **chaotic (red name)**. Chaotic: guards hostile, most NPC shops refuse you, death drops **up to 6 inventory items**; lawfuls drop nothing. Killing a player 20+ levels below you = instant chaotic ([source](https://www.l15server.com/guides/game.html)) | **Yes, core identity.** This is the fairest open-PK ruleset ever shipped. |
| Blood pledges | Guilds led by a Prince/Princess class; emblem over head during declared wars = free PK without chaos penalty ([source](https://www.l15server.com/guides/game.html)) | Yes — war declarations as chaos-immunity. |
| Castle sieges | 7 castles, ~2h windows, breach gates → destroy **Guardian Tower** → prince hits the **crown** to claim; owners set shop **taxes** in the town ([source](https://grokipedia.com/page/Lineage_(video_game)), [source](https://en.wikipedia.org/wiki/Lineage_(video_game))) | **Yes, this is our endgame.** Simplify to 1 castle for MVP. |
| Death | XP loss ≈ 30–90 min of grinding, can de-level; items only at risk if chaotic ([source](https://grokipedia.com/page/Lineage_(video_game))) | Yes — fear is the engine. |
| Alternative siege scoring | Private servers run **point-based sieges** (crown captures + tower hits + level-scaled kills + hold-time) with live `-siegescore` ([source](https://lineage1reborn.com/?page=siege-guide)) | Use as v2 if winner-takes-all proves too snowbally. |

**Lessons:** the alignment system makes open PvP *self-policing* — massively cheaper
than moderation. Siege+taxes create a server-wide economy story that players retell
for decades. The Prince-class gate (only leaders can own pledges) creates a valuable
social role; we map this to CHA stat instead of a dedicated class.

## 3. Dark Eden (1997, SOFTON) — the horror layer

| System | How it works | Steal? |
|---|---|---|
| Day/night warfare | Vampires dominate night (21:00–05:00), slayers dominate day (09:00–17:00), twilight = contested ([source](https://www.reddit.com/r/MMORPG/comments/4r6cps/classic_diabloesque_mmorpg_darkeden_now_available/)) | **Yes — compressed to a 4h real-time cycle** so every session sees both. |
| Blood economy | Vampires gain full XP only by **draining** kills (bite at ≤15% HP); can regen, transform (bat/wolf), go invisible ([source](https://www.reddit.com/r/MMORPG/comments/4r6cps/classic_diabloesque_mmorpg_darkeden_now_available/), [source](https://www.gamerswithjobs.com/node/1249356)) | Post-MVP race. MVP keeps the *infection curse* as foreshadowing. |
| Asymmetric races | 3 races with entirely different stats/gear/skill systems; vampires wear no weapons but stacks of rings/amulets ([source](https://mmos.com/review/darkeden)) | Asymmetry yes, but one-race MVP. |
| Infection | Bitten slayers turn vampire unless cured within 12h ([source](https://mmos.com/review/darkeden)) | MVP: "Blood Curse" night debuff, cured at chapel. Flavor seed. |
| Genre-bending support | Slayer **Healer** (revive/heal/nuke) and **Enchanter** (buffs incl. see-invisible, traps) prove support classes thrive in horror PvP ([source](https://www.gamerswithjobs.com/node/1249356)) | **Yes — directly inspires our Cultist kit.** |
| Race-war objective | **Blood Bible Wars**: steal bible items from the other race for equipable buffs ([source](https://www.reddit.com/r/MMORPG/comments/4r6cps/classic_diabloesque_mmorpg_darkeden_now_available/)) | Post-MVP relic-hunt event. |

**Lessons:** time-of-day asymmetry is the cheapest way to make the world feel alive
and scary; darkness mechanics (light radius, night spawns) give horror *gameplay*,
not just palette.

## 4. Legend of Mir 2 (2000, WeMade) — the casino & the support class

| System | How it works | Steal? |
|---|---|---|
| Weapon upgrade | Refine at blacksmith with **black iron ore** (quality matters) + sacrificed accessories; **failure can break the weapon**; rune sockets on top ([source](https://www.lomcn.net/forum/threads/legend-of-mir2-fourheroes.103126/)) | **Yes, adapted** (see GDD §Enhancement). The break-risk gamble is the genre's heartbeat. |
| Party scaling | Group EXP buff scales with group size (anecdotally up to +100% at 10) ([source](https://www.lomcn.net/forum/threads/legend-of-mir2-fourheroes.103126/)) | Yes — explicit +12%/member curve. |
| Sabuk Castle siege | Scheduled twice-weekly conquest example; guild points for holding, war wins, member kills ([source](https://www.lomcn.net/forum/threads/legend-of-mir2-fourheroes.103126/), [source](https://www.lomcn.net/forum/threads/legend-of-mir-gravity-2-6.87615/)) | Yes — scheduled, recurring, scored. |
| Holder rewards | Winners get +25% EXP/drop buffs, relic items, stat boosts ([source](https://www.invenglobal.com/articles/25481/the-legend-of-mir-2-seungryong-server-launch-effect-new-players-increase-nearly-9-fold)) | Yes — castle buff package. |
| Taoist | Heals, buffs (def/atk), poison, **summons**, invisibility, soul-shield party skills — the classic support+pet hybrid | **Yes — our Cultist is Taoist × DarkEden Enchanter.** |

**Lessons:** enhancement-as-gambling + ore mining creates a complete crafter economy
loop; the support-hybrid class is what turns a grind game into a group game.

---

## 5. Era & modern tech norms (for the architecture doc)

- These games ran **server-authoritative tile sims over TCP** at low update rates
  (≈10 Hz-ish feel), which is exactly why their combat reads as deliberate. We keep
  the feel but run 20 Hz with client-side interpolation.
- Modern MMO server practice: fixed **15–30 Hz ticks** for open-world combat,
  **interest management (AoI)** to avoid quadratic update cost, server-authoritative
  movement/combat for anti-cheat ([source](https://game-ace.com/blog/mmorpg-games-and-how-to-develop-them/)).
- A game-server tick = input drain → movement/AI → actions → timers → delta
  serialization → send, all time-boxed ([source](https://wirepair.org/2023/06/29/so-you-want-to-build-an-mmorpg-server/)).
- ENet (reliable UDP) is the recurring recommendation for this class of game
  ([source](https://wirepair.org/2023/06/29/so-you-want-to-build-an-mmorpg-server/)),
  and raylib+ENet dedicated-server examples exist to crib structure from
  ([source](https://github.com/ThatsAMorais/networking-game-raylib)).
- Old-school MMOs used raw TCP sockets for guaranteed order
  ([source](https://www.reddit.com/r/gamedev/comments/1ogr4r1/is_this_tech_stack_optimal_for_a_largescale/));
  ENet gives us ordering on a control channel plus cheap unreliable movement updates.
- raylib: zero-external-dependency C99, OpenGL abstraction, native **Linux + macOS**
  support out of the box ([source](https://www.raylib.com/), [source](https://github.com/raysan5/raylib)).

## 6. Field research (homework, pre-Phase-2)

5. **Myth of Soma**: any Somadev/LEGOC server — watch weapon-skill raise by use
   and price an aura. That grind-to-mastery loop is what our Anvil must feel like.


Play, don't just read — capture feel notes in `docs/research-notes/`:

1. **Helbreath**: helbreath.net or Nemesis (low-rate = era-accurate) — feel the
   swing timings, EK hunt, and Crusade if one is scheduled.
2. **Lineage 1**: a reborn/1.5 private server — watch an actual siege end-to-end;
   note how the crown-channel moment concentrates tension.
3. **Dark Eden**: the relaunched official server — play an Enchanter to level ~30;
   this is our support-class reference.
4. **Legend of Mir 2**: any LOMCN-listed server — refine a weapon until it breaks.
   Write down how that felt. That's the emotion we're engineering.

## 6b. Myth of Soma (field note, 2026-09-04)

Original 2001 isometric MMO (Digital Bros / later Somadev revival). Screens:
painterly, pre-rendered-looking terrain (dense rooty forests, swamp mud, broken
aqueducts over still water), **tall non-chibi humanoid sprites**; ornate
gothic-silver UI with bottom-left HP orb + potion belt. Darker and moodier than
Mir's palette, less grim than ours — the *zoom level* (sprites larger relative
to tiles than Mir/HB peers) is the signature we want.

Mechanics worth stealing:

- **Weapon skill raises with use** (hits increment skill; each 20 skill = +1~1
  attack; weapon-tier and aura eligibility gated by skill). Weapon choice
  shapes stat growth (2H => more STR-ish gains). Fandom/manual: Sword Mastery
  table, "for each 20 weapon skill you get an attack boost of 1~1".
- **Weapon Auras** — mastery stages at skill 20/50/80/120/150, bought from an
  NPC with *monster parts + gold* (e.g. +3 atk, 2x HP regen, **3-target
  100%-hit multiattack**, +35 power 100%-hit). Perfect fit for our Bonesmith/
  Anvil motif: auras as our affix/refine counterpart.
- **Spear reach 1-2 tiles** — hit over an ally's shoulder; weapon geometry
  creates party roles cheaply.
- **Anvil upgrade risk**: failures destroy the item; first two upgrades are
  safe. Same gamble DNA as Mir refine (already adopted).
- **Moral economy** (private-server innovation): good alignment = faster XP,
  bad alignment = richer loot. Two-sided incentive instead of pure punishment
  for red play.
- **Lesson against pitch-black nights**: servers literally added "permanent
  daytime" because nights were unreadably dark. Our dark-horror night must cap
  tint alpha (floor readability), trade darkness for contrast.
- Crafting breadth (smelt/synthesize/disassemble/repair/cook) and Human-vs-Devil
  race worlds: post-MVP backlog, not alpha.

**Helbreath 2026 reference** (director-linked video, Helbreath koreahb official
channel, watch?v=PUK3za0cBag, thumbnail archived at docs/research-notes/
hb-video-thumb.jpg): mass-PvP proves the crowd-readability rule — 15+ players
with gear-visible sprites stay parseable; spells announce themselves in red caps
callouts ("Ice-Storm!", "Berserk!"); inventory = right-side wooden panel showing
item art, weight "( 50 / 50 )", UPGRADE button; bottom-left red/blue HP/MP bars
with numeric readouts; floor/depth text ("던전 지하 4층"); party names tinted
green. => Visual lock: HB characters/VFX/UI over Soma terrain (see §6b, GDD §2).

(sources: myth-of-soma.fandom.com/wiki/Sword_Mastery, /Human_Upgrading, /Spears;
onrpg.com Myth of Soma revival review; mmorpg.com 2002 feature note;
gamepressure screenshots; LEGOC server features page)

## 7. Synthesis — BLOODHOLLOW's DNA statement

> **Helbreath's** nations + stat builds + EK culture, **Lineage's** alignment/PK +
> bloodpledge + siege/tax politics, **Dark Eden's** day/night horror + support-class
> respect + blood economy (later), **Mir's** upgrade gamble + party-scaled grind +
> Taoist-style support kit. Rendered in era-authentic isometric 2D with modern
> engineering: authoritative 20 Hz server, interest management, replayable
> deterministic sim, agent-maintainable codebase.
