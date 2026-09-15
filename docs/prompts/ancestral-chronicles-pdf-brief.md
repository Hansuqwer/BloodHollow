# PRODUCTION PROMPT — “BLOOD & BANNERS: Chronicles of the Old Korean MMORPG Wars (1997–2016)”

> A fully specified, executable prompt for producing an illustrated PDF history of
> player politics, sieges, and drama in the old Korean (and Korea-adjacent) MMORPGs.
> Status: **EXECUTED** by `tools/build_chronicles_pdf.py` in this repo; output at
> `docs/media/ancestral-chronicles.pdf`. This file is the prompt; the script is the run.

---

## 0. Mission

Produce a single, handsome, book-quality PDF (A4; the executed run yields 27 dense
pages — expand body leading/figure sizes if a longer folio is ever wanted) titled
**“Blood & Banners”** that tells, game by game, the *human* history of the old
2-D/early-3-D MMORPGs: in-game politics, taxation, monopolies, espionage,
betrayal, sieges, and the moments when a game server briefly became a polis.
Tone: a war correspondent’s chronicle written by someone who was there —
dramatic but disciplined. Every factual claim must trace to the source table in
§6; contested facts must be flagged inline as “contested” rather than smoothed
over. AI-generated art must be labelled as illustration; archival footage
frames captured in this repo’s research dossiers must be labelled as evidence.

Audience: the BloodHollow dev team (this is ancestral research for the game —
see `docs/01-research.md`), plus any reader who ever stayed up for a scheduled
siege.

## 1. Roster of games (one chapter each, in this order)

1. **Prologue — The PC-Bang Republic.** 1997 IMF crisis; broadband build-out;
   PC-bang economics; NCsoft’s decision to bill PC-bangs instead of homes;
   why Korea turned MMOs into political simulators.
2. **Lineage (Lineage 1, NCsoft, 1998).** The tax state.
3. **Helbreath (Siementech, 1999).** The eternal crusade.
4. **Dark Eden (SOFTON, alpha 2000 / service 2002).** The night economy.
5. **Myth of Soma (Comnjoy/Wizgate/MGame, 2000–05 KR).** The small kingdom.
6. **Lineage II (NCsoft, 2003) — The Bartz Liberation War.** The centerpiece.
7. **EverQuest Goes East (SOE × NCsoft/UbiSoft/Gamania, 2000–06).** The
   Korean, Chinese, Taiwanese and Japanese server experiments and their
   collapses.
8. **The Chinese Mirror (Legend of Mir 2/3, WeMade/Shanda).** Sabak siege
   culture and the 2005 Dragon Sabre murder.
9. **Darkfall (Aventurine, 2009; MGame Korea 2013–16).** The Western heir and
   the Korean detour. **Correction required in-text:** Darkfall is Greek
   (Aventurine S.A., Athens); there was never a 1999–2003 “Darkfall Korea”.
   The Korean connection is MGame’s 2013 service of *Darkfall: Unholy Wars*
   (Korean title “다크폴: 잔혹한 전쟁”) on a merged Korea–Japan server.
10. **Anatomy of Player Politics.** Comparative systems chapter + design
    lessons for BloodHollow (`docs/02-gdd.md` pillar 4: “politics as endgame”).

## 2. Mandatory story beats per chapter (sourced; see §6)

### Lineage 1
- Founded 1997-03-11 by Kim Taek-jin; *Lineage* launches 1998; based on Shin
  Il-suk’s manhwa; US$1.49M revenue in 1998, >1M users in first 15 months,
  ~4M subscriptions by 2001 (IGN).
- Only the **Prince/Princess** class (Charisma stat) may found a Blood Pledge
  and declare wars/sieges → forced symbiosis.
- Castle siege mechanics: break gate → Guardian Tower → touch the crown before
  sunset; biweekly sieges; **tax has no cap — minimum 10%, lords pushed
  50–60%** to crush rivals.
- Castles: Kent, Aden, Diad Fortress, Dwarf Castle, etc.
- **DK (Dragon Knights)** on the *Deporoju* (데포로쥬) server: Lineage’s first
  hunting-ground control (사냥터 통제) — the dictatorship template later
  imported into Lineage II.
- Open PK / karma / red names; real-money item markets; Western servers
  shuttered June 2011.

### Helbreath
- Siementech; development from 1998; Korean release Aug 1999 / commercial
  service Nov 1999 (flag as contested); 100,000 users within two months;
  export enquiries from Spain & Japan (Hankyoreh, Dec 1999).
- Two nations at permanent war: **Aresden** (red, god Aresien) vs **Elvine**
  (blue, god Eldiniel); travelers capped at level 19 until citizenship;
  Civilian vs Combatant status; six stats STR/DEX/VIT/INT/MAG/CHR.
- **EK (Enemy Kill) points → Hero Armor** (red/blue), the uniform of status.
- “Crusade” upgrade (July 2001): twice-weekly all-out wars; players take roles
  soldier / builder / commander; attackers raise mana collectors in Middleland
  to drop the enemy town’s shield; low levels could sway the fight.
- Annual national guild tournament Nov–Dec.
- Level-cap riots (160 → 180) and the P2P switch that bled the West; 2002
  source leak → private-server diaspora (Olympia et al.), C&Ds to ISPs;
  client-side security sins (dupes, tigerworm spawns, GM-account hacks).

### Dark Eden
- SOFTON (ex-Metrotech); horror pioneer; Helea/Eslania; Lilith and the 12
  master vampires (Vlad Tepes, Elizabeth Báthory, Gilles de Rais); the 13
  Blood Bibles — the 13th was **never shipped** (GM admitted “in development”).
- Three races with asymmetric economies: Slayers sell heads highest; Vampires
  drop best loot; Ousters level fastest; biting turns humans into vampires.
- **Day/night power inversion** (full day 9:00–16:59, full night 21:00–4:59) —
  the schedule itself was politics.
- Race Wars, Castle Wars, Caligo Wars; Perona market gated behind paywall.
- **The China disaster:** ~40% royalty deal never paid; partner renamed the
  game and ran it as their own; server+client sources leaked; piratical
  private servers plagued Korea; second leak after a second contract.
- Publishing: Netmarble mirror service (Nov 2002, separate servers); Japan
  closed 2013-09-30; NA (Ignited Games) 2011–2013; Steam relaunch 2016-11-28.

### Myth of Soma
- Comnjoy; KR 2000–01; later Wizgate → MGame; Humans vs Devils after the
  Great War of the Monster world (Macheonru; the hero Pacheon King’s sword).
- Castle/territory wars, guild wars, crafting; EU published by Game Network
  (it had its **own TV show** on the GameNetwork Sky channel); IT Digital
  Bros; CN Jie San Feng.
- KR service ended **2005-05-31** (balance issues, declining players); late
  2008 hackings hastened the EU decline; EU closed 2009-03-31; SomaDev revival.

### Lineage II — Bartz Liberation War (longest chapter)
- OBT begins 2003-07-06; DK (name borrowed from L1, distinct pledge) rules
  Bartz (1st server); triple alliance 2003-09-14 (DK + Gods’ Knights +
  Genesis); Akirus reaches server-first level 51 in Aug 2003.
- Grievances: hunting-ground monopoly, auto-macro (오토) economy, purge orders
  (척살령), **2004 tax raise 10% → 15%**.
- 2004-05-09: **Red Revolution** (붉은혁명), 50 players, takes Giran and
  declares **0% tax**; loses it two weeks later — but ignites the server.
- The **Naebok-dan** (내복단, “underwear corps”): players from every server
  reroll naked beginners on Bartz to fight; human barricades of corpses at the
  Hunter’s Village gate slow DK’s march.
- June 2004: Genesis defects from DK with a public apology; July: 32-clan
  alliance takes Oren.
- **2004-07-17 Aden siege:** siege-registration mind games (swap 10 minutes
  before deadline); feigned retreat; DK’s archer corps wiped; DK lord
  *shadow여솔* falls; Aden taken → “Bartz Liberation Day”; press coverage.
- Spoils poison the victors: Revenge→Oren, Genesis→Aden, Red Revolution
  nothing; anti–Red Revolution bloc forms; Red Revolution defect**s to DK**;
  “the four-bloods are bad, but the anti–four-bloods are worse.”
- DK retakes everything; 2005-01-27 unlimited purge order, **~700 kills/day**.
- May 2006: Akirus dissolves DK: “DK chose evil over good; because evil
  existed, good could shine brighter.” Second war 2007 (Neutral League);
  conflict winds down by 2008-03; **cumulative ~200,000 participants over
  ~4 years**; retold in comics, literature, and an art exhibition; essayized
  by novelist Prof. Lee In-hwa (Shin Dong-A, Aug 2005).

### EverQuest East
- 2000-08-30: Hanbitsoft imports EQ1 to Korea (English client, ₩11,000/month)
  — packaging, not servers.
- 2002-01-23/24: SOE × NCsoft alliance for Korea/Taiwan/HK; CBT June 2002;
  OBT July 2002 (Ruins of Kunark folded into OBT Oct); commercial Apr 2003
  with Scars of Velious; Luclin bits in Aug 2003; newbie regen boost to lvl 20.
- Culture clash: slow tempo, WASD, sit-to-regen vs Lineage appetite; NCsoft
  announces shutdown Nov 2003; **KR servers die 2003-12-31**, weeks after L2’s
  Oct 2003 launch — the community joke that EQ was unplugged to feed L2
  servers; stragglers migrate to NA servers.
- China: SOE × Ubi Soft deal announced 2002-09-19; Shanghai Ubi Soft operates
  from 2003 (commercial April 28, 2003 per SOE milestones); >400,000 veteran
  players; contract expiry + poor management → **closed 2005-01-31**; accounts
  offered transfer to English servers + 60 free days.
- EQ II East (Gamania, Apr 2005): localization reputation sinks it;
  terminated 2006-03-29/30; CN accounts → Mistmoore, TW → Najena, **KR →
  Unrest**, where the Korean guilds consolidate into *Chosen* and *WURI*;
  KR-local EQ2 itself closed 2006-03-02. Japan is the one Asian market whose
  non-East EQ2 localization survives — the exception that indicts the rest.

### Legend of Mir 2/3 (China)
- WeMade; Shanda’s 热血传奇 makes it China’s first MMO monoculture; Sabak
  (沙巴克) castle sieges as guild politics; Korean design exported intact.
- **2005 Dragon Sabre murder (Mir 3, Shanghai):** Qiu Chengwei (41) and a
  friend win the sabre; lend it to Zhu Caoyuan (26), who sells it for
  7,200 yuan (~US$870); police say virtual property isn’t property; Qiu
  stabs Zhu dead; Shanghai No. 2 Intermediate Court sentences Qiu to death
  with reprieve. Contemporary estimates put grey-market RMT at
  US$100M–$1B.

### Darkfall
- Aventurine S.A. (Greece); DF1 2009, full loot, FFA PvP, skill-based combat,
  city/house ownership, naval combat, clan warfare where espionage and lying
  are legitimate tactics; official servers close 2012-11-15.
- MGame (권이형) signs for Asia (May 2012); Korean title “다크폴: 잔혹한
  전쟁”; KR–JP merged server (novelty); CBTs Aug/Sep 2013; launch
  **2013-10-30**; China planned, never happens; service ends **2016-09-30**;
  *Rise of Agon* (Big Picture Games) 2017 keeps the bloodline alive.

## 3. Image manifest (every figure captioned + credited)

AI illustrations (label: “Illustration (AI) — evocative, not a game asset”):
`docs/media/chronicles/01-cover.jpg` (cover), `02-pcbang.jpg` (prologue),
`03-lineage-siege.jpg` (L1), `04-helbreath-crusade.jpg` (HB),
`05-darkeden-night.jpg` (DE), `06-soma-war.jpg` (Soma),
`07-bartz-barricade.jpg` (L2), `08-eq-east.jpg` (EQ),
`09-mir-sabak.jpg` (Mir), `10-dragon-sabre.jpg` (Mir case),
`11-darkfall.jpg` (Darkfall), `12-dawn-after.jpg` (epilogue).

**Execution note (2026-09-15):** the AI-image quota ran out after eight plates;
`09`–`12` were therefore sourced as clearly-credited reference captures instead:
`09` = Legend of Mir community capture (classicgamerhub archive), `10` = r/SWORDS
community photo of a willow-leaf dao, `11` = Darkfall community capture (fandom
archive, converted from webp), `12` = free-stock pixel battlefield study
(stockcake.com). Captions/credits in the builder were rewritten to match; the
AI/reference distinction is preserved on every plate. Provenance URLs:
09 → classicgamerhub.com/the-legend-of-mir-2-pc-review-where-chicken-soup-fuels-castle-sieges •
10 → reddit.com/r/SWORDS/comments/h78kb8/ • 11 → darkfall.fandom.com/wiki/Darkfall •
12 → stockcake.com/s/pixel-castle. Raw downloads kept under `image-search/` at repo root.

Archival evidence plates (label: “Archival frame, repo dossier”):
`docs/research-notes/lineage1/l1-void-4v-pledge-contact.jpg`,
`docs/research-notes/helbreath/hb-koreahb-official-contact.jpg`,
`docs/research-notes/helbreath/hb-olympia-pestilence-ep21-contact.jpg`,
`docs/research-notes/dark-eden/darkeden-awakening-pvp-contact.jpg`,
`docs/research-notes/soma/soma-web-contact.jpg`,
`docs/research-notes/mir2/mir2-web-contact.jpg`.

## 4. Layout & typography spec

- A4 portrait; margins 56/56/64/64 pt. Paper #F7F1E5, ink #1A1410, accent
  blood #8A1B1B, gold #B08A3C, steel #274B63.
- Cover: full-bleed 01-cover.jpg under a black gradient plate; DarkGarden
  (repo-bundled Type-1 gothic, fallback DejaVu Serif Bold) masthead; subtitle
  and “A BloodHollow research folio” line.
- Body: DejaVu Serif 9.5/14; headings DejaVu Serif Bold; labels/timelines
  DejaVu Mono; emphasis set in DejaVu Sans (no italic faces installed —
  never synthesize).
- Running head: left = book title, right = chapter title, 7 pt mono, hairline
  rule; folio bottom centre; chapter openers and cover suppress both.
- Recurring components: ChapterOpener (giant number + title + epigraph),
  PullQuote, SidebarBox (“dossier card” with fact rows), Figure with caption,
  TimelineTable (mono, red hairlines), first-paragraph drop cap, per-chapter
  “Chapter notes” source list with bracketed ids.
- Back matter: grand timeline table 1997–2017, glossary (혈맹, 공성전,
  내복단, 척살령, 사냥터 통제, EK, 아덴, 전면전/크루세이드, PC방, 정),
  master source list.

## 5. Execution

`./.venv/bin/python tools/build_chronicles_pdf.py` (venv has reportlab +
Pillow; install pymupdf for page-render QA). Script must be two-pass
(TableOfContents), must skip missing images gracefully, and must print page
count + file size. QA: render pages 1, 8, and a chapter opener to PNG and
visually check before declaring done.

## 6. Source registry (id — title — url)

L1-1 Porter’s Five Forces, “Brief History of NCsoft” — portersfiveforce.com •
L1-2 Gamota Lab, “From IMF Crisis to Gaming Empire” — gamota.com •
L1-3 Baidu Baike, “Game Lineage” • L1-4 DualShockers, “How Lineage Put South
Korea On The MMO Map” • L1-5 IGN, “Lineage: The Blood Pledge” (2001) •
L1-6 mmorpg.com, Lineage page • HB-1 en-academic/Wikipedia, “Helbreath” •
HB-2 mmorpg.com, “Helbreath Review” (2004) • HB-3 helbreath.net event log •
HB-4 koit.co.kr, Siementech profile (2001) • HB-5 Hankyoreh, “시멘텍” (1999-12-26) •
HB-6 r/Games retrospective (2019) • HB-7 second.wiki “Helbreath” •
DE-1 darkeden-legend fandom • DE-2 Gamia archive • DE-3 Wikipedia “Darkeden” •
DE-4 Massively OP (2015) • DE-5 NamuWiki (EN) “Dark Eden” •
SO-1 mmorpg.com Soma pages • SO-2 myth-of-soma.com guide • SO-3 Myth of Soma
fandom • BZ-1 Inven, “바츠해방전쟁” (2014) • BZ-2 ko.wikipedia “바츠해방전쟁” •
BZ-3 NamuWiki “바츠해방전쟁” • BZ-4 Shin Dong-A, Lee In-hwa essay (2005-08) •
BZ-5 gameinsight.co.kr press kit • EQ-1 ko.wikipedia “에버퀘스트” •
EQ-2 Sony press release 2002-09-19 (EQ China) • EQ-3 SOE milestones
(project1999/tombraiderforums mirror) • EQ-4 Baidu Baike/Zh-Wiki 无尽的任务 •
EQ-5 Wikipedia “EverQuest II” (EQ2: East) • EQ-6 NamuWiki “에버퀘스트 2” •
MR-1 Sydney Morning Herald 2005-03-30 • MR-2 The Guardian gamesblog 2005-06-09 •
MR-3 china.org.cn 2005-03 • DF-1 Wikipedia “Darkfall” • DF-2 MMO Culture,
“Darkfall – From Greece to Asia” (2012) • DF-3 NamuWiki “다크폴(엠게임)” •
DF-4 Gametoc Hankyung (2013-08-14) • DF-5 Inven (2013) •
RD-1..RD-6 repo dossiers under docs/research-notes/ (lineage1, helbreath×2,
dark-eden, soma, mir2).
