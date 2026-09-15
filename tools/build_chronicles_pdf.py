#!/usr/bin/env python3
"""Blood & Banners — Chronicles of the Old Korean MMORPG Wars (1997-2016).

Executes docs/prompts/ancestral-chronicles-pdf-brief.md.
Run:  ./.venv/bin/python tools/build_chronicles_pdf.py
Out:  docs/media/ancestral-chronicles.pdf
"""
import os, sys
from reportlab.lib.pagesizes import A4
from reportlab.lib.units import mm
from reportlab.lib import colors
from reportlab.lib.colors import HexColor
from reportlab.lib.styles import ParagraphStyle
from reportlab.lib.enums import TA_LEFT, TA_CENTER
from reportlab.pdfbase import pdfmetrics
from reportlab.pdfbase.ttfonts import TTFont
from reportlab.platypus import (BaseDocTemplate, PageTemplate, Frame, Flowable,
    Paragraph, Spacer, Image, Table, TableStyle, PageBreak, KeepTogether,
    NextPageTemplate)
from reportlab.platypus.tableofcontents import TableOfContents

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
M = lambda *p: os.path.join(ROOT, *p)
OUT = M("docs", "media", "ancestral-chronicles.pdf")

W, H = A4                      # 595.27 x 841.89
LM = RM = 56
TM = BM = 64
CW = W - LM - RM               # content width

PAPER = HexColor("#F7F1E5"); INK = HexColor("#1A1410")
BLOOD = HexColor("#8A1B1B"); GOLD = HexColor("#B08A3C")
STEEL = HexColor("#274B63"); FAINT = HexColor("#D9CDB2")
PANEL = HexColor("#EFE6D2")

# ---------------------------------------------------------------- fonts
F = "/usr/share/fonts/truetype/dejavu/"
pdfmetrics.registerFont(TTFont("Serif", F + "DejaVuSerif.ttf"))
pdfmetrics.registerFont(TTFont("SerifB", F + "DejaVuSerif-Bold.ttf"))
pdfmetrics.registerFont(TTFont("Sans", F + "DejaVuSans.ttf"))
pdfmetrics.registerFont(TTFont("SansB", F + "DejaVuSans-Bold.ttf"))
pdfmetrics.registerFont(TTFont("Mono", F + "DejaVuSansMono.ttf"))
pdfmetrics.registerFont(TTFont("MonoB", F + "DejaVuSansMono-Bold.ttf"))
GOTHIC = "SerifB"
try:
    RL = M(".venv", "lib")
    rlfonts = None
    for d in os.listdir(RL):
        c = os.path.join(RL, d, "site-packages", "reportlab", "fonts")
        if os.path.isdir(c): rlfonts = c
    face = pdfmetrics.EmbeddedType1Face(os.path.join(rlfonts, "DarkGardenMK.afm"),
                                        os.path.join(rlfonts, "DarkGardenMK.pfb"))
    pdfmetrics.registerTypeFace(face)
    pdfmetrics.registerFont(pdfmetrics.Font("DarkGarden", "DarkGardenMK", "WinAnsiEncoding"))
    GOTHIC = "DarkGarden"
except Exception as e:
    print("note: gothic face unavailable (%s); using SerifB" % e)

# ---------------------------------------------------------------- styles
def S(name, **kw):
    return ParagraphStyle(name, **kw)
st_body   = S("body", fontName="Serif", fontSize=9.5, leading=14.2,
              textColor=INK, firstLineIndent=11, spaceAfter=6, alignment=TA_LEFT)
st_body0  = S("body0", parent=st_body, firstLineIndent=0)
st_small  = S("small", fontName="Serif", fontSize=7.6, leading=10.6, textColor=INK)
st_note   = S("note", fontName="Sans", fontSize=7.3, leading=10.2,
              textColor=HexColor("#4A4036"), spaceAfter=2.5)
st_mono   = S("mono", fontName="Mono", fontSize=7.2, leading=10, textColor=INK)
st_monoB  = S("monoB", fontName="MonoB", fontSize=7.2, leading=10, textColor=BLOOD)
st_label  = S("label", fontName="MonoB", fontSize=7, leading=9, textColor=GOLD,
              spaceBefore=0, spaceAfter=4)
st_chap   = S("chap", fontName="SerifB", fontSize=21, leading=25, textColor=INK)
st_chsub  = S("chsub", fontName="Serif", fontSize=10.5, leading=15,
              textColor=HexColor("#5A4C3A"))
st_epi    = S("epi", fontName="Serif", fontSize=9, leading=13,
              textColor=HexColor("#6A5A44"), leftIndent=14, rightIndent=8)
st_episrc = S("episrc", fontName="Mono", fontSize=6.8, leading=9,
              textColor=HexColor("#8A7A5C"), leftIndent=14)
st_pull   = S("pull", fontName="SerifB", fontSize=12.5, leading=18, textColor=BLOOD)
st_pullsrc= S("pullsrc", fontName="Mono", fontSize=6.8, leading=9,
              textColor=HexColor("#8A7A5C"))
st_sb_t   = S("sbt", fontName="MonoB", fontSize=8, leading=11, textColor=BLOOD)
st_sb_b   = S("sbb", fontName="Serif", fontSize=8.2, leading=11.8, textColor=INK)
st_sb_k   = S("sbk", fontName="MonoB", fontSize=7.4, leading=10.5, textColor=STEEL)
st_cap    = S("cap", fontName="Sans", fontSize=7.2, leading=10.4,
              textColor=HexColor("#5A4C3A"), spaceAfter=2)
st_toch1  = S("toch1", fontName="SerifB", fontSize=10.5, leading=17, textColor=INK)
st_toch0  = S("toch0", fontName="MonoB", fontSize=8, leading=14, textColor=BLOOD,
              spaceBefore=6)
st_h2     = S("h2", fontName="MonoB", fontSize=8.4, leading=12, textColor=STEEL,
              spaceBefore=10, spaceAfter=4)

# ---------------------------------------------------------------- helpers
class Rule(Flowable):
    def __init__(self, w=CW, color=FAINT, thick=0.8, space=6):
        Flowable.__init__(self); self.w, self.c, self.t, self.sp = w, color, thick, space
    def wrap(self, aw, ah): return (self.w, self.t + self.sp)
    def draw(self):
        self.canv.setStrokeColor(self.c); self.canv.setLineWidth(self.t)
        self.canv.line(0, self.sp*0.5, self.w, self.sp*0.5)

class ChapterOpener(Flowable):
    """Big gothic numeral + title + epigraph banner."""
    def __init__(self, num, title, sub, epi=None, esrc=None):
        Flowable.__init__(self)
        self.num, self.title, self.sub, self.epi, self.esrc = num, title, sub, epi, esrc
        self.h = 152 if epi else 120
    def wrap(self, aw, ah): return (CW, self.h)
    def draw(self):
        c = self.canv
        c.setFillColor(BLOOD); c.rect(0, self.h-46, 46, 46, fill=1, stroke=0)
        c.setFillColor(PAPER); c.setFont(GOTHIC, 30)
        c.drawCentredString(23, self.h-38, self.num)
        c.setFillColor(INK); c.setFont("SerifB", 20)
        c.drawString(58, self.h-30, self.title)
        c.setFillColor(HexColor("#5A4C3A")); c.setFont("Serif", 10)
        c.drawString(58, self.h-44, self.sub)
        c.setStrokeColor(BLOOD); c.setLineWidth(1.4); c.line(0, self.h-58, CW, self.h-58)
        c.setStrokeColor(FAINT); c.setLineWidth(0.7); c.line(0, self.h-61, CW, self.h-61)
        y = self.h - 84
        if self.epi:
            c.setFillColor(HexColor("#6A5A44")); c.setFont("Serif", 9.4)
            for line in self.epi:
                c.drawString(14, y, line); y -= 13
            c.setFont("Mono", 6.8); c.setFillColor(HexColor("#8A7A5C"))
            c.drawString(14, y - 2, "— " + self.esrc)

class PullQuote(Flowable):
    def __init__(self, text, src=None):
        Flowable.__init__(self); self.p = Paragraph(text, st_pull); self.src = src
    def wrap(self, aw, ah):
        self.aw = aw
        w, h = self.p.wrap(aw - 30, 1000)
        self.ph = h
        self.base = 14 if self.src else 4
        return (aw, h + self.base + (12 if self.src else 6))
    def draw(self):
        c = self.canv
        c.setStrokeColor(BLOOD); c.setLineWidth(2.4)
        c.line(4, self.base - 5, 4, self.base + self.ph + 5)
        self.p.drawOn(c, 22, self.base)
        if self.src:
            c.setFont("Mono", 6.8); c.setFillColor(HexColor("#8A7A5C"))
            c.drawString(22, 0, self.src)

def sidebar(title, rows, accent=STEEL):
    """rows: list of (key, value) or ('para', text)."""
    inner = [Paragraph(title, st_sb_t), Spacer(1, 3)]
    for k, v in rows:
        if k == "para":
            inner.append(Paragraph(v, st_sb_b))
        else:
            inner.append(Paragraph('<font name="MonoB" size="7.2" color="#274B63">%s</font>'
                                   '  <font name="Serif" size="8.2">%s</font>' % (k, v), st_sb_b))
    t = Table([[inner]], colWidths=[CW - 26])
    t.setStyle(TableStyle([
        ("BACKGROUND", (0, 0), (-1, -1), PANEL),
        ("LINEBEFORE", (0, 0), (0, -1), 3, accent),
        ("TOPPADDING", (0, 0), (-1, -1), 9), ("BOTTOMPADDING", (0, 0), (-1, -1), 9),
        ("LEFTPADDING", (0, 0), (-1, -1), 12), ("RIGHTPADDING", (0, 0), (-1, -1), 12),
    ]))
    return t

def figure(path, caption, credit, maxw=CW, maxh=330):
    if not os.path.exists(path):
        return Paragraph("[image missing: %s] %s" % (os.path.basename(path), caption), st_cap)
    from PIL import Image as PI
    iw, ih = PI.open(path).size
    scale = min(maxw / iw, maxh / ih)
    img = Image(path, width=iw * scale, height=ih * scale)
    cap = Paragraph('<font name="MonoB" size="6.6" color="#8A1B1B">%s</font>'
                    '  <font name="Sans" size="7.2" color="#5A4C3A">%s</font>' % (credit, caption),
                    st_cap)
    t = Table([[img], [cap]], colWidths=[max(iw * scale, 10)])
    t.setStyle(TableStyle([
        ("ALIGN", (0, 0), (-1, -1), "CENTER"),
        ("TOPPADDING", (0, 0), (-1, -1), 0), ("BOTTOMPADDING", (0, 0), (-1, -1), 2),
        ("LEFTPADDING", (0, 0), (-1, -1), 0), ("RIGHTPADDING", (0, 0), (-1, -1), 0),
        ("LINEBELOW", (0, 0), (0, 0), 0.6, FAINT),
    ]))
    return KeepTogether([t, Spacer(1, 8)])

def dropcap(text, char=None):
    ch = char or text[0]
    rest = text[len(ch):]
    big = Paragraph('<font name="SerifB" size="26" color="#8A1B1B">%s</font>' % ch,
                    ParagraphStyle("dc", fontName="SerifB", fontSize=26, leading=26,
                                   textColor=BLOOD))
    body = Paragraph(rest, st_body0)
    t = Table([[big, body]], colWidths=[22, CW - 26])
    t.setStyle(TableStyle([
        ("VALIGN", (0, 0), (0, 0), "TOP"), ("VALIGN", (1, 0), (1, 0), "TOP"),
        ("TOPPADDING", (0, 0), (-1, -1), 0), ("BOTTOMPADDING", (0, 0), (-1, -1), 6),
        ("LEFTPADDING", (0, 0), (-1, -1), 0), ("RIGHTPADDING", (0, 0), (-1, -1), 4),
    ]))
    return t

def timeline(rows, head=("YEAR", "EVENT")):
    data = [[Paragraph(h, st_monoB) for h in head]]
    for y, e in rows:
        data.append([Paragraph(y, st_monoB), Paragraph(e, st_mono)])
    t = Table(data, colWidths=[64, CW - 68], hAlign="LEFT")
    t.setStyle(TableStyle([
        ("LINEBELOW", (0, 0), (-1, 0), 1.2, BLOOD),
        ("LINEBELOW", (0, 1), (-1, -1), 0.5, FAINT),
        ("TOPPADDING", (0, 0), (-1, -1), 3), ("BOTTOMPADDING", (0, 0), (-1, -1), 3),
        ("VALIGN", (0, 0), (-1, -1), "TOP"),
    ]))
    return t

def notes_block(items):
    out = [Spacer(1, 6), Rule(color=FAINT), Spacer(1, 4),
           Paragraph("CHAPTER NOTES", st_label)]
    for i in items:
        out.append(Paragraph(i, st_note))
    return out

def h2(t): return Paragraph(t, st_h2)
def p(t, style=st_body): return Paragraph(t, style)

# ================================================================ STORY
story = []

# ---- cover handled by template; first real page:
story.append(NextPageTemplate("normal"))
story.append(PageBreak())

# ---- colophon / how to read
story.append(Paragraph("HOW TO READ THIS FOLIO", st_chap))
story.append(Spacer(1, 4)); story.append(Rule(color=BLOOD, thick=1.2)); story.append(Spacer(1, 10))
story.append(p("This is a history of governments that never existed. Between 1997 and 2016, a "
 "handful of Korean (and Korea-adjacent) online worlds — <i>Lineage</i>, <i>Helbreath</i>, "
 "<i>Dark Eden</i>, <i>Myth of Soma</i>, <i>Lineage II</i>, the Asian experiments of "
 "<i>EverQuest</i>, the Chinese mirror of <i>Legend of Mir</i>, and the Greek heir "
 "<i>Darkfall</i> — let players tax each other, monopolize land, declare war, spy, defect, "
 "and revolt. The chapters that follow retell those wars as political history, because that is "
 "what they were.", st_body0))
story.append(p("Every factual claim is footnoted to the source registry printed in the back. "
 "Where sources disagree — Helbreath's exact commercial date, Dark Eden's birth year — the "
 "disagreement is flagged as <font color='#8A1B1B'>contested</font> instead of being silently "
 "arbitrated. Two kinds of pictures appear: <font name='MonoB' size='7.4' color='#8A1B1B'>"
 "ILLUSTRATION (AI)</font> plates are evocative paintings in the period style and are not game "
 "assets; <font name='MonoB' size='7.4' color='#274B63'>ARCHIVAL FRAME</font> plates are real "
 "captured frames archived in this repository's research dossiers "
 "(<font name='Mono' size='7.4'>docs/research-notes/</font>).", st_body))
story.append(p("The folio was compiled for the BloodHollow team as ancestral research: our own "
 "design pillar four — <i>politics as endgame</i> — is a direct inheritance from these wars. "
 "Read it as a chronicle, mine it as a design document.", st_body))
story.append(Spacer(1, 10))
story.append(sidebar("CONVENTIONS", [
    ("GAME TITLES", "set italic on first use per chapter; Korean titles given where they matter."),
    ("DATES", "KST-era dates as published; server names kept in their community spelling (Bartz, Deporoju, Unrest)."),
    ("NAMES", "player handles preserved as recorded: Akirus, shadow여솔, Red Revolution, DK, Naebok-dan."),
    ("MONEY", "contemporary figures quoted unadjusted; conversions as printed by the source."),
    ("para", "Compiled 2026-09-15 on branch arena/01a0a5ea-bloodhollow. Produced by "
             "tools/build_chronicles_pdf.py from docs/prompts/ancestral-chronicles-pdf-brief.md."),
], accent=GOLD))

# ---- TOC
story.append(PageBreak())
story.append(Paragraph("CONTENTS", st_chap))
story.append(Spacer(1, 4)); story.append(Rule(color=BLOOD, thick=1.2)); story.append(Spacer(1, 8))
toc = TableOfContents()
toc.levelStyles = [st_toch0, st_toch1]
story.append(toc)
story.append(PageBreak())

CH = []   # (num, title, sub, epi, esrc)

# ================================================================ CH I
CH.append(("I", "The PC-Bang Republic", "Prologue — how a financial crisis built the player-state",
           ["Life was good, especially when", "the internet cafe had solid bandwidth."],
           "after a Lineage retrospective"))
story.append(ChapterOpener(*CH[-1]))
story.append(dropcap("In November 1997 the Korean won collapsed and the Republic of Korea went "
 "to the IMF for fifty-eight billion dollars. Within three years the same country had wired "
 "itself more densely than anywhere on earth: the crisis money became cheap office space, the "
 "laid-off salarymen became PC-bang owners, and the government's broadband build-out became the "
 "plumbing for everything that follows in this book."))
story.append(p("The PC bang was not an arcade and not a home. It was a third place with a "
 "billing address. NCsoft, founded in Seoul on 11 March 1997 by Kim Taek-jin, understood this "
 "before anyone else: instead of charging households a ~US$20 monthly subscription, Lineage "
 "charged the cafés a licensing fee and let the players in for the price of a cola. A blood "
 "pledge could therefore assemble every night at the same physical table, shoulder to shoulder, "
 "in the same room, for the same siege. Politics that would have been impossible among "
 "strangers at home became routine among neighbours."))
story.append(figure(M("docs", "media", "chronicles", "02-pcbang.jpg"),
 "A Seoul PC bang at night, 1999: the room where the pledge assembles. The licence fee was "
 "paid by the cafe; the siege was paid by the players.", "ILLUSTRATION (AI)"))
story.append(p("Three design accidents of the era compounded into a political technology. "
 "First, scarcity: items, castles and hunting grounds were deliberately finite, so ownership "
 "meant power over other people. Second, persistence: death, debt and taxes carried over from "
 "session to session, so grudges did too. Third, legibility: red names, karma, EK counters and "
 "pledge emblems made allegiance and reputation visible at a glance — a uniform for the "
 "player-state. Everything in the following chapters is a variation on those three accidents."))
story.append(p("This folio moves chronologically through the games and ends with an anatomy of "
 "the machinery they shared. The wars are real in the only sense that matters: two hundred "
 "thousand people spent four years of nights on one of them."))

# ================================================================ CH II  LINEAGE 1
CH.append(("II", "Lineage: The Tax State", "NCsoft, 1998 — the charter of the blood pledge",
           ["Only a prince may raise a banner;", "only charisma may hold it."],
           "charter of the blood pledge"))
story.append(ChapterOpener(*CH[-1]))
story.append(dropcap("Lineage launched in September 1998 and became, within fifteen months, a "
 "million-user polity. It was based on Shin Il-suk's manhwa of betrayed princes, and it kept "
 "the manhwa's central intuition: legitimacy is a class stat. Only the Prince/Princess class, "
 "whose defining attribute was Charisma, could found a blood pledge, declare pledge wars and "
 "castle sieges. The prince was weak alone and indispensable together — a constitutional "
 "monarchy compiled into the class table."))
story.append(p("The siege was the franchise. Attackers broke the outer gate, fought through the "
 "courtyard to the Guardian Tower, and won only when the pledge leader touched the crown — "
 "before sunset, or the attempt dissolved. Sieges ran on a two-week cadence, and the winner "
 "became castle lord with the right to tax every player-to-NPC transaction in the territory. "
 "There was a floor of ten percent and <font color='#8A1B1B'>no ceiling</font>: lords squeezed "
 "fifty, sixty percent, not for profit alone but to starve rival pledges of the adena their "
 "members needed to train. Taxation was not a mechanic in Lineage; it was the mechanic."))
story.append(figure(M("docs", "media", "chronicles", "03-lineage-siege.jpg"),
 "A castle gate under assault in the period style: hundreds of sprites, one crown. Touching it "
 "before sunset was the whole of the law.", "ILLUSTRATION (AI)"))
story.append(p("On the Deporoju server, the blood pledge DK — Dragon Knights — perfected the "
 "dark arts that every later server-politics story repeats: hunting-ground control ( monopolizing "
 "the fields where experience and money spawn), boss-monster monopolies, and punitive expeditions "
 "against anyone who farmed without permission. DK held every castle on Deporoju for years and "
 "taxed accordingly. When Lineage II opened its beta in 2003, DK crossed over — and brought the "
 "template with it."))
story.append(figure(M("docs", "research-notes", "lineage1", "l1-void-4v-pledge-contact.jpg"),
 "Archival frames, Lineage 1: pledge bodies massed in the Tower region, name-tints marking "
 "alignment. The ground is dark, the sprites are bright; the uniform is the name.",
 "ARCHIVAL FRAME — repo dossier lineage1"))
story.append(p("Around the crown grew the first fully financialized player economy: open PK "
 "outside safe zones, red-name karma, and a grey market in items and whole accounts that "
 "Korean courts would spend a decade learning to price. In the West the servers went dark in "
 "June 2011 — reportedly just as the game was surging again at home. The diaspora now lives on "
 "classic-style private and remaster servers where the same pledges, grayer, still register for "
 "the same sieges."))
story.append(PullQuote("The best thing about Lineage is that you don't have to be good at "
 "games. You have to be good at people.", "after Shin Il-suk, creator of the source manhwa"))
story.append(sidebar("DOSSIER — LINEAGE (LINEAGE 1)", [
    ("DEVELOPER", "NCsoft, Seoul. Founded 1997-03-11 (Kim Taek-jin)."),
    ("LAUNCH", "Korea, September 1998; based on Shin Il-suk's manhwa."),
    ("SCALE", "US$1.49M revenue in 1998; >1M users in 15 months; ~4M subscriptions by 2001."),
    ("CHARTER", "Only Prince/Princess (Charisma) founds pledges, declares war and sieges."),
    ("TAX", "Castle tax floor 10%, no cap; 50-60% documented as siegecraft."),
    ("SIEGE", "gate -> Guardian Tower -> touch the crown before sunset; biweekly."),
    ("LEGACY", "DK of Deporoju invents hunting-ground control; exports it to Lineage II."),
    ("WEST", "NA servers closed June 2011."),
]))
story.extend(notes_block([
 "[L1-1] Porter's Five Forces — Brief History of NCsoft (founding, 1998 launch, KOSDAQ 2000).",
 "[L1-2] Gamota Lab — From IMF Crisis to Gaming Empire (PC-bang licensing, no-cap tax, 50-60%).",
 "[L1-3] Baidu Baike — Game Lineage (prince-only pledges, siege procedure, crown-before-sunset).",
 "[L1-4] DualShockers — How Lineage Put South Korea On The MMO Map ($1.49M/1998, 1M users, castles).",
 "[L1-5] IGN 2001 — Lineage: The Blood Pledge (~4M subscriptions; Lord British promo siege).",
 "[BZ-1]/[BZ-3] DK's Deporoju hunting-ground control as Lineage's first (Inven; NamuWiki).",
 "[RD-1] repo dossier docs/research-notes/lineage1/findings.md (karma tint, siege crowd).",
]))

# ================================================================ CH III  HELBREATH
CH.append(("III", "Helbreath: The Eternal Crusade", "Siementech, 1999 — two churches, one battlefield",
           ["On full-war nights, every user", "cancels their personal plans."],
           "Siementech company profile, 2001"))
story.append(ChapterOpener(*CH[-1]))
story.append(dropcap("Helbreath began development in 1998 at Siementech, a venture of young "
 "developers, and opened in Korea in 1999 — August by the international records, November by "
 "the Korean press; treat the month as <font color='#8A1B1B'>contested</font>. Within two "
 "months it claimed a hundred thousand users, and export enquiries arrived from Spain and "
 "Japan. Its premise was the purest political machine of the era: two theocracies, Aresden "
 "(red, faithful to Aresien) and Elvine (blue, faithful to Eldiniel), in a war with no victory "
 "condition."))
story.append(p("The machine had a constitution. A new player was a neutral Traveler, hard-"
 "capped at level 19 until choosing citizenship — every citizen was a convert, and conversion "
 "was permanent enmity. Citizens chose Civilian status (safe from enemies, prey to monsters) "
 "or Combatant (fair game for the other nation). Killing enemy combatants earned EK — Enemy "
 "Kill — points, and EK bought the Hero Armor of your church, red or blue: status worn as "
 "uniform, exactly the legibility the era prized."))
story.append(p("In July 2001 Siementech shipped the Crusade upgrade and turned the war into a "
 "schedule. Twice a week the nations met in full war; on war nights, the company profile "
 "boasted, users cancelled their personal appointments. Players drew roles like a real "
 "campaign — soldier, builder, commander — and the objective was industrial: attackers raised "
 "mana collectors in the Middleland until the enemy town's defense shield collapsed, while "
 "defenders contested every collector site. Because the collectors mattered more than the "
 "duels, even low-level players could sway a crusade — the earliest documented case of a game "
 "designing mass participation into its politics."))
story.append(figure(M("docs", "media", "chronicles", "04-helbreath-crusade.jpg"),
 "Aresden crimson against Elvine azure on the Middleland: the twice-weekly full war.",
 "ILLUSTRATION (AI)"))
story.append(figure(M("docs", "research-notes", "helbreath", "hb-koreahb-official-contact.jpg"),
 "Archival frames from the Korean official server: crusade combat in the dungeons and fields, "
 "green party names, town shields under pressure.", "ARCHIVAL FRAME — repo dossier helbreath"))
story.append(p("Every year in November and December Siementech ran a national guild tournament, "
 "formalizing the guild layer beneath the national one. But the state's foundations were "
 "client-side: security checks lived on players' PCs, and the record shows the bill — dupes, "
 "item creation, hacked GM accounts, 'tigerworm' spawns in Middleland, a p2p crack. When the "
 "level cap rose to 160 and then 180 and the service went pay-to-play, much of the West "
 "defected; in 2002 the server sources leaked and a private-server diaspora — Olympia among "
 "the famous — carried the war beyond Siementech's reach, pursued by cease-and-desist letters "
 "to ISPs. The Korean server outlived its maker."))
story.append(figure(M("docs", "research-notes", "helbreath", "hb-olympia-pestilence-ep21-contact.jpg"),
 "Archival frames from a private-server crusade (Olympia lineage): the war continued outside "
 "the official state, on leaked law.", "ARCHIVAL FRAME — repo dossier helbreath"))
story.append(sidebar("DOSSIER — HELBREATH", [
    ("DEVELOPER", "Siementech (Korea). Dev from 1998; 1999 release (Aug intl / Nov KR: contested)."),
    ("NATIONS", "Aresden (red, Aresien) vs Elvine (blue, Eldiniel); Traveler cap lvl 19."),
    ("STATUS", "Civilian vs Combatant; EK points -> Hero Armor."),
    ("WAR", "Crusade update 2001-07: twice-weekly full wars; mana collectors break town shields."),
    ("ROLES", "soldier / builder / commander on war nights; national guild war Nov-Dec."),
    ("COLLAPSE", "cap riots 160->180; P2P switch; 2002 source leak; private-server diaspora."),
]))
story.extend(notes_block([
 "[HB-4] koit.co.kr 2001 — Siementech profile (dev 1998, commercialization Nov 1999, Crusade July 2001, roles, guild war).",
 "[HB-5] Hankyoreh 1999-12-26 — 100k users in two months; Spain/Japan enquiries.",
 "[HB-1]/[HB-7] Wikipedia/en-academic/second.wiki — release dates, citizenship, EK, hero armor colors.",
 "[HB-3] helbreath.net event log — living crusade/siege/relic/apocalypse schedule.",
 "[HB-6] r/Games 2019 retrospective — mana-collector crusades, low-level participation, Olympia.",
 "[HB-2] mmorpg.com 2004 — gods' war lore; hacking and client-side security sins.",
]))

# ================================================================ CH IV  DARK EDEN
CH.append(("IV", "Dark Eden: The Night Economy", "SOFTON, 2000/2002 — the state that runs on a clock",
           ["The sun never rose over Helea again;","the vampires made it a constitution."],
           "after the Dark Eden creation myth"))
story.append(ChapterOpener(*CH[-1]))
story.append(dropcap("Dark Eden is the first horror MMORPG, and its horror was administrative. "
 "In the quarantined province of Helea, in the fictional Eslania, three master vampires — "
 "Vlad Tepes, Elizabeth Bathory, Gilles de Rais — failed to wake Lilith and instead blotted "
 "out the sun. Development began in the late 1990s at SOFTON (then Metrotech); the Korean "
 "alpha dates to June 2000, the open beta to July 2001, official service to 2002 — while a "
 "fandom genealogy claims 1997; treat the birth year as <font color='#8A1B1B'>contested</font> "
 "and the darkness as permanent."))
story.append(p("The game's master stroke was the clock. Full day ran 9:00 to 16:59, when "
 "vampires were weakest and Slayers ruled; full night 21:00 to 4:59, when the same vampire "
 "who fled you at lunch could erase you; the hours between were transition, parity. Power "
 "itself was scheduled, so politics was too: raids were planned like tide tables, and the "
 "races' asymmetric economies — Slayers selling monster heads highest, Vampires drawing the "
 "best drops, Ousters leveling fastest — made each faction need the others' territories at "
 "the wrong hours."))
story.append(figure(M("docs", "media", "chronicles", "05-darkeden-night.jpg"),
 "Helea under the cloud: hunters with lanterns, pale shapes on the roofs, the chapel cross "
 "still lit. Between 21:00 and 04:59 this street belongs to the other side.",
 "ILLUSTRATION (AI)"))
story.append(p("Race Wars, Castle Wars and Caligo Wars gave the hatred a calendar; the market "
 "town of Perona gave it a price floor — free players were locked out of the trading hub until "
 "they paid. A bitten human could turn vampire mid-career, and a vampire could buy his way "
 "back to humanity: allegiance in Dark Eden was literally convertible, which made every "
 "defection legible as a transaction. The lore promised thirteen Blood Bibles; players "
 "collected twelve and asked where the thirteenth was. The operators answered: in development. "
 "It never shipped. A state that prints a prophecy and never honors it teaches its citizens "
 "what to believe about the state."))
story.append(p("The foreign chapters read like a cautionary treaty. Netmarble ran a parallel "
 "Korean service from November 2002 on separate servers. In China, SOFTON signed for roughly "
 "forty percent royalties and collected, by its own account, nothing; the partner renamed the "
 "game and operated it as its own, and the server and client sources leaked into the wild — "
 "pirate servers haunted the Korean service for years. A second Chinese contract leaked again. "
 "Japan ran seven years before closing on 30 September 2013; North America flickered 2011 to "
 "2013; a Steam relaunch arrived in 2016. The Korean original, four servers, one of them "
 "non-PK, outlasted them all."))
story.append(figure(M("docs", "research-notes", "dark-eden", "darkeden-awakening-pvp-contact.jpg"),
 "Archival frames, Korean Dark Eden PvP: the race war at street level, neon on black.",
 "ARCHIVAL FRAME — repo dossier dark-eden"))
story.append(sidebar("DOSSIER — DARK EDEN", [
    ("DEVELOPER", "SOFTON (ex-Metrotech), Korea. Alpha 2000-06-15; beta 2001-07; service 2002; 1997 claim contested."),
    ("RACES", "Slayers (heads), Vampires (drops), Ousters (XP); biting converts; cures exist."),
    ("CLOCK", "full day 9:00-16:59; full night 21:00-4:59; transitions parity."),
    ("WARS", "Race Wars, Castle Wars, Caligo Wars; Perona market paywalled."),
    ("LORE DEBT", "13th Blood Bible promised, never shipped."),
    ("EXPORTS", "Netmarble mirror 2002; China royalty disaster + double source leak; JP closed 2013-09-30; NA 2011-13; Steam 2016-11-28."),
]))
story.extend(notes_block([
 "[DE-1]/[DE-2]/[DE-3] fandom/Gamia/Wikipedia — races, clock windows, asymmetries, version history.",
 "[DE-5] NamuWiki (EN) — 2001 beta / 2002 service; Netmarble Nov 2002; China royalty disaster and leaks; JP/NA closures.",
 "[DE-4] Massively OP 2015 — 'one of the older continuously running graphical MMORPGs'.",
 "[RD-4] repo dossier docs/research-notes/dark-eden/findings.md.",
]))

# ================================================================ CH V  SOMA
CH.append(("V", "Myth of Soma: The Small Kingdom", "Comnjoy, 2000-2005 — the war that had a TV show",
           ["The warrior of light, Soma!"],
           "original Korean promotional poster"))
story.append(ChapterOpener(*CH[-1]))
story.append(dropcap("Myth of Soma — Soma Sinhwa Jeongi — launched in Korea around 2000-2001 "
 "from Comnjoy, passed through Wizgate and MGame, and fought its eternal war between the "
 "Human world and the Devil world after the Great War started by Macheonru of the Monster "
 "world, ended once by the hero Pacheon King's cast-down sword. It was a smaller kingdom than "
 "Lineage, and its smallness is the point: it shows the template working at village scale."))
story.append(p("Humans and Devils ground, crafted, and met in scheduled territorial wars — the "
 "War of the Worlds — plus guild wars over castles. The European edition, published by Game "
 "Network (with Digital Bros in Italy), had something no rival could claim: its own television "
 "program on the GameNetwork satellite channel, broadcasting the war into living rooms. In "
 "Korea the balance never held; the official service ended on 31 May 2005, drained by balance "
 "problems and declining numbers. The European server followed on 31 March 2009 after the late "
 "2008 hackings finished the decline — and then the faithful did what Helbreath's faithful "
 "had done: a community team, SomaDev, resurrected the abandoned client and still runs it."))
story.append(figure(M("docs", "media", "chronicles", "06-soma-war.jpg"),
 "The small kingdom at war: humans on the marble heights, devils up the scorched slope.",
 "ILLUSTRATION (AI)"))
story.append(figure(M("docs", "research-notes", "soma", "soma-web-contact.jpg"),
 "Archival frames, Myth of Soma web captures: painted-plate terrain cut into diamonds — the "
 "ground grammar this repo's art pipeline inherits.", "ARCHIVAL FRAME — repo dossier soma"))
story.append(p("Soma's afterlife matters to this folio's anatomy chapter: it is the cleanest "
 "case of a player-state surviving the death of its government. The war continued without the "
 "king — which is to say, the politics were always the players'."))
story.append(sidebar("DOSSIER — MYTH OF SOMA", [
    ("DEVELOPER", "Comnjoy (KR), later Wizgate -> MGame. KR 2000/01 - 2005-05-31."),
    ("WAR", "Human vs Devil; territorial War of the Worlds; castle guild wars; crafting."),
    ("EUROPE", "Game Network / Digital Bros; its own TV show on the GameNetwork channel."),
    ("DEATHS", "KR 2005-05-31 (balance, decline); EU 2009-03-31 after 2008 hackings."),
    ("AFTERLIFE", "SomaDev community revival of the abandonware client."),
]))
story.extend(notes_block([
 "[SO-1] mmorpg.com — faction war, classes, release framing.",
 "[SO-2] myth-of-soma.com guide — 2001 GameNetwork launch, TV show, 2008 hackings.",
 "[SO-3] Myth of Soma fandom — Comnjoy/Wizgate/MGame; KR end 31 May 2005; publishers by region.",
 "[RD-5] repo dossier docs/research-notes/soma/findings.md (terrain lock, night cap).",
]))

# ================================================================ CH VI  BARTZ
CH.append(("VI", "Lineage II: The Bartz Liberation War", "NCsoft, 2003-2008 — the first citizen revolution online",
           ["Even a level-1 character, if dozens", "gather, strikes them in mind as well as body."],
           "Bartz alliance appeal, 2004"))
story.append(ChapterOpener(*CH[-1]))
story.append(dropcap("When Lineage II opened its beta on 6 July 2003, the blood pledge DK — "
 "Dragon Knights, the name of the Deporoju dynasty though a distinct organization — moved to "
 "the first server, Bartz, and by 14 September 2003 had sealed a triple alliance with the Gods' "
 "Knights and Genesis pledges. Their leader Akirus held the server's first level 51 by August. "
 "What followed, from June 2004 to March 2008, involved cumulative two hundred thousand "
 "participants and is the largest documented player war in history: the Bartz Liberation War."))
story.append(h2("THE GRIEVANCES"))
story.append(p("DK ruled by the Deporoju playbook, upgraded: hunting grounds monopolized so "
 "that no one outside the alliance could realistically pass level 52; auto-macro (oto) farms "
 "printing adena around the clock; purge orders (cheoksallyeong) killing the disobedient on "
 "sight; castles taxed at will. In 2004 the alliance raised every territory's tax from ten to "
 "fifteen percent. For players under level 40 — who must buy weapons, armor, potions and spell "
 "books from NPC shops — that was not a policy change but a cost-of-living crisis. The tax "
 "made the revolution."))
story.append(h2("GIRAN, ZERO PERCENT"))
story.append(p("On 9 May 2004 the pledge Red Revolution (Bulgeun Hyeokmyeong), fifty strong, "
 "took Giran castle from DK in a guerrilla gamble and declared the tax at zero percent. DK's "
 "counter-siege retook the castle within two weeks — but the image of a zero-percent "
 "proclamation had already left the server. Small pledges that had stayed neutral for a year "
 "began to register for war, and players from other servers began rerolling naked beginners on "
 "Bartz. They wore only the starting underwear the game gives a fresh character, and they "
 "called themselves the Naebok-dan — the underwear corps."))
story.append(figure(M("docs", "media", "chronicles", "07-bartz-barricade.jpg"),
 "The Naebok-dan at a village gate: unarmored beginners from every server, holding the road "
 "against the alliance's armored march.", "ILLUSTRATION (AI)"))
story.append(h2("THE APOSTASY OF GENESIS"))
story.append(p("In June 2004 Genesis — one of the three thrones of the DK alliance — broke "
 "with DK over a petty insult, posted a public apology, and defected to the coalition. In July "
 "the coalition, grown to thirty-two pledges, took Oren. Then came the war's finest trick. "
 "Siege law required registration a day ahead; ten minutes before the deadline, the coalition "
 "flipped its registrations: every pledge but Genesis withdrew from the Aden attack and "
 "registered to defend Oren. DK, reading an Aden siege, marched its main body out of Aden "
 "toward Oren — and met at the gate the Naebok-dan, hundreds of them, blocking the road and "
 "piling their own corpses as a barricade to slow the march. DK burned through them with "
 "forced-attack, losing items to death penalties in its panic, and arrived too late: the "
 "feigned retreat had doubled back, wiped the archer corps left to hold Aden, and killed the "
 "DK lord shadow-Yeosol. Genesis's Kalitzeubeuk touched the seal. The day was declared Bartz "
 "Liberation Day; the forums filled for weeks; real newspapers ran the story."))
story.append(h2("THE SPOILS"))
story.append(p("Revolutions rot at the treasury. Revenge took Oren; Genesis took Aden; Red "
 "Revolution, which had lit the fire and lost Giran for it, received nothing. When Giran "
 "finally fell, the quarrel became a schism; an anti-Red-Revolution bloc declared war on the "
 "original revolutionaries, and Red Revolution — the pledge of the zero-percent proclamation — "
 "defected to DK. The server's mood curdled into the phrase historians of Bartz quote: the "
 "four-bloods are bad, but the anti-four-bloods are worse. The Naebok-dan's name was borrowed "
 "by robbers; the moral economy collapsed."))
story.append(h2("THE TERROR AND THE END"))
story.append(p("DK retook every castle by late 2004 and hid its elite in the Tower of "
 "Insolence. On 27 January 2005 it reissued an unlimited purge order; on some days the "
 "alliance killed seven hundred logged-in former coalition members. Akirus directed the "
 "counterattack personally, his nine-man guard scattering coalition front lines. In May 2006 "
 "he dissolved DK voluntarily, leaving the war's most quoted sentence: DK chose evil over "
 "good, and because evil existed, good could shine brighter. A second war flared in 2007 when "
 "neutral pledges formed a Neutral League; it burned out by March 2008. Six years of Bartz "
 "history closed, and the war passed into culture — comics, literature, an art exhibition, and "
 "an essay by the novelist and professor Lee In-hwa, who called it the first citizen revolution "
 "online."))
story.append(PullQuote("DK chose evil over good; because evil existed, good could shine "
 "brighter.", "Akirus, dissolving the Dragon Knights, May 2006"))
story.append(timeline([
 ("2003-07-06", "Lineage II OBT opens; DK establishes itself on Bartz (1st server)."),
 ("2003-08", "Akirus first on the server to level 51."),
 ("2003-09-14", "Triple alliance: DK + Gods' Knights + Genesis."),
 ("2004 (early)", "Tax raised 10% -> 15% across DK territories; purges expand."),
 ("2004-05-09", "Red Revolution takes Giran with 50; declares 0% tax; loses it in 2 weeks."),
 ("2004-06", "Genesis defects to the coalition with a public apology."),
 ("2004-07", "Coalition grows to 32 pledges; takes Oren."),
 ("2004-07-17", "Aden siege: registration feint, Naebok-dan corpse barricade, Aden falls. Bartz Liberation Day."),
 ("2004-11", "DK stripped of Giran and Gludio; retreats to Tower of Insolence."),
 ("2005-01-27", "Unlimited purge order; ~700 kills on some days."),
 ("2006-05", "Akirus dissolves DK. First war ends."),
 ("2007-2008", "Second war (Neutral League); conflict winds down by 2008-03."),
]))
story.append(sidebar("DOSSIER — BARTZ LIBERATION WAR", [
    ("THEATRE", "Lineage II, Bartz (1st) server; spillover from every KR server."),
    ("PARTIES", "DK alliance (DK, Gods' Knights, Genesis, later Revenge et al.) vs Bartz coalition + Naebok-dan."),
    ("SCALE", "cumulative ~200,000 participants; 2004-06 to 2008-03."),
    ("CAUSES", "hunting-ground monopoly; auto-macro economy; purges; 10->15% tax."),
    ("AFTERLIFE", "comics, literature, art exhibition; Lee In-hwa essay (Shin Dong-A, 2005-08)."),
]))
story.extend(notes_block([
 "[BZ-1] Inven 2014 — full narrative: 0% Giran, Naebok-dan, Aden feint, 700/day purges, dissolution.",
 "[BZ-2] ko.wikipedia — dates 2004-06 to 2008-03; prelude of 9 May 2004.",
 "[BZ-3] NamuWiki — tax 10->15%; registration feint detail; second war 2007.",
 "[BZ-4] Shin Dong-A 2005-08, Lee In-hwa — 'first citizen revolution'; 52+ level monopoly; 700 killed in one evening.",
 "[BZ-5] gameinsight press kit — 200k participants, 4 years, cultural afterlife.",
 "[L1-2]/[L1-3] DK's Lineage 1 Deporoju antecedent.",
]))

# ================================================================ CH VII  EQ EAST
CH.append(("VII", "EverQuest Goes East", "SOE x NCsoft x UbiSoft x Gamania, 2000-2006 — the empire that missed the market",
           ["We will provide the opportunity for","players to transfer characters to US servers."],
           "the Chinese farewell, 2005"))
story.append(ChapterOpener(*CH[-1]))
story.append(dropcap("EverQuest was the West's cathedral: slow, vertical, devout. Between 2000 "
 "and 2006 it attempted to naturalize in Korea, China, Taiwan and Japan, and the attempt is the "
 "era's best-documented collision between two MMO cultures. Korea got it first, by import: on "
 "30 August 2000 Hanbitsoft shipped the Ruins of Kunark expansion as a package — English "
 "client, Korean manual, eleven thousand won a month — selling subscriptions to American "
 "servers rather than running any."))
story.append(p("On 23-24 January 2002 Sony Online Entertainment and NCsoft signed the real "
 "alliance: NCsoft would localize and host EverQuest for Korea, Taiwan and Hong Kong. Closed "
 "beta ran in June 2002, open beta in July, with Kunark folded in by October; commercial "
 "service began in April 2003 on Scars of Velious, and pieces of Luclin followed in August. "
 "NCsoft even bent the physics for the market: new characters regenerated abnormally fast to "
 "level 20, a concession to a player base raised on Lineage's tempo."))
story.append(p("It was not enough. The Korean player of 2003 sat-to-regen in EverQuest and "
 "starved for Lineage; the WASD keyboard felt like arithmetic. In November 2003 NCsoft "
 "announced the end; the Korean servers ran free through 31 December 2003 and then died, "
 "weeks after Lineage II's October launch, with characters offered passage to North America — "
 "and the community's verdict that NCsoft had unplugged EverQuest to feed Lineage 2's server "
 "farm. A devoted remnant crossed the ocean and still plays there."))
story.append(p("China followed the same arc a year slower. SOE and Ubi Soft announced the "
 "mainland deal on 19 September 2002; Shanghai Ubi Soft opened the Chinese EverQuest in 2003 "
 "and grew it past four hundred thousand veteran players; then the contract expired, "
 "management faltered, and on 31 January 2005 the lights went out, with the same consolation: "
 "transfer to the English servers, sixty days free. In Taiwan, Gamania ran the traditional "
 "Chinese service from October 2002."))
story.append(p("The sequel repeated the lesson with interest. EverQuest II: East — a specially "
 "localized edition for China, Taiwan and Korea, shipped April 2005 with Gamania — died on "
 "29-30 March 2006 of its localization's bad reputation, even in populous China. The accounts "
 "were deported en masse: Chinese characters to Mistmoore, Taiwanese to Najena, Koreans to "
 "Unrest, where the exiled Korean guilds consolidated into Chosen and WURI before the lag and "
 "the hack-waves thinned them. Korea's own EQ2 local service closed on 2 March 2006. Only "
 "Japan escaped the pattern: its non-East EQ2 localization survived as the one functioning "
 "Asian server — the exception that indicts the rest."))
story.append(figure(M("docs", "media", "chronicles", "08-eq-east.jpg"),
 "Norrath at dawn, empty: the server-select screen as ghost town.", "ILLUSTRATION (AI)"))
story.append(sidebar("DOSSIER — EVERQUEST EAST", [
    ("2000-08-30", "Hanbitsoft imports EQ1 (Kunark) to KR: package + subscription, no servers."),
    ("2002-01-23", "SOE x NCsoft alliance: KR/TW/HK localization and hosting."),
    ("2002-07", "KR open beta; 2003-04 commercial (Velious); newbie regen boost to lvl 20."),
    ("2003-12-31", "KR servers die, weeks after L2's Oct 2003 launch."),
    ("2003/2005", "CN via Shanghai Ubi Soft (2002-09-19 deal; 400k vets) closed 2005-01-31."),
    ("2005/2006", "EQ2: East (Gamania) dies 2006-03-29/30; KR accounts -> Unrest (Chosen, WURI)."),
    ("EXCEPTION", "Japan's non-East EQ2 localization survives."),
]))
story.extend(notes_block([
 "[EQ-1] ko.wikipedia EverQuest — Hanbitsoft 2000-08-30; NCsoft CBT/OBT/commercial dates; shutdown 12-31; regen boost; EQall.",
 "[EQ-3] SOE milestones — 2002-01-23 alliance; 2003-04-16 KR / 2003-04-28 CN commercial launches.",
 "[EQ-2] Sony press release 2002-09-19 — Ubi Soft mainland China deal.",
 "[EQ-4] Baidu Baike / zh.wikipedia — CN run from 2003-01, 400k players, closed 2005-01-31, transfers; TW Gamania 2002-10-03.",
 "[EQ-5]/[EQ-6] Wikipedia EQ2 / NamuWiki — EQ2: East failure; Mistmoore/Najena/Unrest transfers; KR close 2006-03-02; Japan survives.",
]))

# ================================================================ CH VIII  MIR
CH.append(("VIII", "The Chinese Mirror", "Legend of Mir 2/3 — Sabak, and the price of a sabre",
           ["The weapon was not real property","protected by law."],
           "as reported from the Shanghai police, 2005"))
story.append(ChapterOpener(*CH[-1]))
story.append(dropcap("WeMade's Legend of Mir 2 crossed to China as Shanda's Re Xue Chuan Qi and "
 "became the first MMO monoculture: entire cafes playing one server, guilds as companies, and "
 "the sandstone castle of Sabak (Shabake) as the nation's capital. Sabak sieges were Korean "
 "siegecraft translated intact — gate, courtyard, seal — but scaled to a market where a "
 "castle's tax roll could employ real people. Guild politics in Mir China was not metaphor; it "
 "was payroll."))
story.append(figure(M("docs", "media", "chronicles", "09-mir-sabak.jpg"),
 "Sabak culture in the flesh: fire pillars over the contest hall, the Korean siege grammar "
 "translated and payroll-sized.", "REFERENCE FRAME — Legend of Mir (WeMade), community capture"))
story.append(p("In 2005 the mirror showed the era its true face. Qiu Chengwei, 41, and a "
 "friend had won a Dragon Sabre in Legend of Mir 3 and lent it to Zhu Caoyuan, 26, who sold it "
 "for 7,200 yuan — roughly US$870, five times Shanghai's average monthly wage. Qiu reported "
 "the theft; the police answered that a virtual object was not property the law protected. So "
 "Qiu collected the debt himself, at Zhu's home, with a knife. The Shanghai No. 2 Intermediate "
 "People's Court sentenced him to death with a two-year reprieve — the standard Chinese "
 "suspended death sentence. Contemporary analysts estimated the grey market in virtual goods "
 "at one hundred million to one billion dollars a year."))
story.append(figure(M("docs", "media", "chronicles", "10-dragon-sabre.jpg"),
 "A willow-leaf dao of the family the Shanghai court was asked to price at 7,200 yuan — five "
 "times a monthly wage.", "REFERENCE PHOTO — r/SWORDS community photo, stand-in for the Dragon Sabre"))
story.append(p("The case is the folio's hinge between politics and property. Every tax in "
 "Lineage, every monopoly in Bartz, every head-price in Dark Eden was a claim about who owns "
 "the virtual. Shanghai answered that the state does not — and then spent the next decade "
 "building the legal machinery that says it partly does. The players had known all along."))
story.append(figure(M("docs", "research-notes", "mir2", "mir2-web-contact.jpg"),
 "Archival frames, Legend of Mir 2 web captures: the grind texture and refine theatre this "
 "repo's dossiers audit.", "ARCHIVAL FRAME — repo dossier mir2"))
story.extend(notes_block([
 "[MR-1] Sydney Morning Herald 2005-03-30 — facts of the sabre sale, police refusal, stabbing.",
 "[MR-2] The Guardian gamesblog 2005-06-09 — suspended death sentence; RMT at $100M-$1B.",
 "[MR-3] china.org.cn — Shanghai No. 2 Intermediate Court proceedings.",
 "[RD-6] repo dossier docs/research-notes/mir2/findings.md.",
]))

# ================================================================ CH IX  DARKFALL
CH.append(("IX", "Darkfall: The Western Heir", "Aventurine (Greece) x MGame (Korea), 2009-2016 — full loot, merged flags",
           ["Without passion, there is no drama."],
           "Aventurine community, on Unholy Wars, 2012"))
story.append(ChapterOpener(*CH[-1]))
story.append(p("A correction first, because the folio's rule is to flag rather than smooth: "
 "there was never a 1999-vintage 'Darkfall Korea'. Darkfall is Greek — Aventurine S.A. of "
 "Athens — a 2009 sandbox of unrestricted PvP, full looting, skill-based combat, city and "
 "house ownership, naval warfare and clan politics in which espionage, lying and cheating "
 "were declared legitimate statecraft. Its official servers closed on 15 November 2012. The "
 "Korean chapter comes later, and it is real.", st_body0))
story.append(p("In May 2012 Aventurine signed MGame to run Darkfall in Asia; the original plan "
 "was Korea, China and Japan, but China's licensing wall held, and the service launched as a "
 "Korea-Japan merger — one server, two flags, a novelty MGame advertised hard. The sequel "
 "edition, Darkfall: Unholy Wars, was renamed for the market through a public contest: "
 "Darkfall: Janhokhan Jeonjaeng, 'the cruel war'. Joint Korean-Japanese closed tests ran in "
 "August and September 2013; open service began 30 October 2013."))
story.append(p("For three years the merged server held the old faith: sea sieges between "
 "Korean and Japanese clans, full-loot ambushes on the trade roads, clan charters written in "
 "two languages. MGame's service ended on 30 September 2016, refunds through October; in 2017 "
 "Big Picture Games raised the bloodline again as Darkfall: Rise of Agon. The Greek war "
 "machine had needed a Korean dock to reach Asia — and the Korean players, who had invented "
 "the genre's politics, recognized their own reflection in the full-loot mirror."))
story.append(figure(M("docs", "media", "chronicles", "11-darkfall.jpg"),
 "Darkfall as the West built it: full loot, a burning horizon, the inventory is the war. The "
 "merged Korea-Japan server flew two flags over this sea.",
 "REFERENCE FRAME — Darkfall (Aventurine S.A.), community capture"))
story.append(sidebar("DOSSIER — DARKFALL", [
    ("DEVELOPER", "Aventurine S.A., Athens. DF1 2009; official close 2012-11-15."),
    ("FAITH", "full loot, FFA PvP, skill-based combat, city/house ownership, naval war; espionage legit."),
    ("KOREA", "MGame (Kwon I-hyung): Asia deal 2012-05; KR title 'Cruel War'; KR-JP merged server."),
    ("DATES", "CBTs 2013-08/09; launch 2013-10-30; service end 2016-09-30."),
    ("AFTER", "Rise of Agon (Big Picture Games), 2017."),
]))
story.extend(notes_block([
 "[DF-1] Wikipedia Darkfall — Aventurine, 2009, full loot, close 2012-11-15.",
 "[DF-2] MMO Culture 2012 — MGame Asia deal (Korea confirmed; China hoped).",
 "[DF-3] NamuWiki Darkfall (MGame) — launch 2013-10-30; end 2016-09-30.",
 "[DF-4] Gametoc Hankyung 2013-08-14 — KR/Japan plan, China dropped; 'a game that encourages PK'.",
 "[DF-5] Inven 2013 — Korean title contest; joint KR-JP CBTs.",
]))

# ================================================================ CH X  ANATOMY
CH.append(("X", "Anatomy of Player Politics", "Comparison, and the inheritance of BloodHollow",
           ["Politics is endgame."],
           "BloodHollow GDD, pillar 4"))
story.append(ChapterOpener(*CH[-1]))
story.append(dropcap("Nine worlds, one machinery. Legitimacy always came from a scarce office "
 "(castle lord, crown-toucher, citizen hero); revenue always came from a tax on circulation "
 "(shop tax, head prices, market gates); monopoly always targeted the means of training "
 "(hunting grounds, boss spawns, Tower of Insolence); and every state died the same three "
 "deaths — defection of a great vassal (Genesis), corruption of the liberators (Red "
 "Revolution), or exile of the people (server transfers, private-server diasporas)."))
story.append(timeline([
 ("LEGITIMACY", "prince's charisma (L1); crown-touch (L1/L2); citizenship+EK (HB); race+clock (DE); clan level (L2)."),
 ("REVENUE", "shop tax, uncapped (L1/L2); head sales (DE); market-gate paywall (DE); macro farms (Bartz DK)."),
 ("MONOPOLY", "hunting-ground control (Deporoju, Bartz); boss monopolies; Tower refuge."),
 ("INTEL", "pledge spies; forum appeals (Bartz); registration feints; siege-calendar manipulation."),
 ("PROPAGANDA", "0% tax proclamation; Liberation Day; dissolution speech; apology letters (Genesis)."),
 ("DEFECTION", "Genesis 2004; Red Revolution to DK; EQ players to NA servers; KR guilds to Unrest."),
 ("EXILE", "private-server diasporas (HB, Soma); account transfers (EQ CN/KR, EQ2E); server-hopping Naebok-dan."),
 ("DEATH", "vassal defection; liberator corruption; popular exile — never siege defeat alone."),
], head=("LEVER", "FORMS OBSERVED")))
story.append(p("BloodHollow inherits the machinery deliberately. Our locked design pillars — "
 "scheduled sieges at Weeping Castle, town taxes that matter, EK leaderboards, alignment worn "
 "in the name band, XP debt and item-drop stakes — are each an answer to a specific failure "
 "mode in this book: sieges on a cadence so war becomes institution (Helbreath); taxes with "
 "consequences but floors against tyranny (Bartz's fifteen percent was the revolution); "
 "legible allegiance so reputation is political capital (every ancestor); stakes on death so "
 "that barricades of the willing mean something (the Naebok-dan). What we refuse is the "
 "ancestors' client-side trust and their uncapped tax — the two sins that duped Helbreath and "
 "radicalized Bartz."))
story.append(PullQuote("The ground is dark, the sprites are bright, and the name is the "
 "uniform.", "cross-ancestor finding, repo dossier index"))

# ---- epilogue
story.append(Spacer(1, 10))
story.append(Paragraph("EPILOGUE — DAWN AFTER THE SIEGE", st_h2))
story.append(p("Every server in this book eventually printed the same screen: maintenance "
 "notice, transfer offer, farewell letter. The castles outlived their lords and the lords "
 "outlived their servers; the pledges, gray at the temples, reassemble on remaster shards and "
 "community revivals and register for the same sieges under the same names. What persisted was "
 "never the software. It was the habit of governing each other — the oldest player skill there "
 "is, and the one BloodHollow intends to teach again."))
story.append(figure(M("docs", "media", "chronicles", "12-dawn-after.jpg"),
 "Sunrise over a burned keep: the field empties, the banners stay. Every server in this book "
 "ends on a morning like this one.", "REFERENCE ART — pixel battlefield study, free stock (stockcake.com)"))

# ================================================================ BACK MATTER
story.append(PageBreak())
story.append(Paragraph("APPENDIX A — GRAND TIMELINE", st_chap))
story.append(Spacer(1, 6))
story.append(timeline([
 ("1997", "IMF crisis; NCsoft founded 11 Mar; Aventurine-era Greece far away."),
 ("1998", "Lineage launches in Korea (Sep)."),
 ("1999", "Helbreath opens in Korea; EQ ships in NA; 100k HB users in two months."),
 ("2000", "Dark Eden alpha (Jun); Hanbitsoft imports EQ to KR (Aug); Soma beta/launch; Soma KR."),
 ("2001", "HB Crusade update (Jul); DE open beta (Jul); Mir 2 to China (Shanda); Soma EU on GameNetwork TV."),
 ("2002", "SOE x NCsoft EQ alliance (Jan); EQ2-era DE official service; Netmarble DE mirror (Nov); HB source leak."),
 ("2003", "EQ KR commercial (Apr) and death (31 Dec); L2 OBT (Jul); DK triple alliance (Sep); EQ CN opens."),
 ("2004", "Bartz: 10->15% tax; Giran 0% (9 May); Aden falls (17 Jul); Liberation Day."),
 ("2005", "EQ CN closes (31 Jan); Soma KR ends (31 May); Mir 3 Dragon Sabre murder, Shanghai; EQ2: East ships."),
 ("2006", "EQ2: East dies (Mar); KR accounts -> Unrest; Akirus dissolves DK (May)."),
 ("2007-08", "Second Bartz war; winds down 2008-03."),
 ("2009", "Soma EU closes (Mar); Darkfall launches (Greece)."),
 ("2011-13", "L1 West closes (Jun 2011); DE NA closes; DE JP closes 2013-09-30; DF1 closes 2012-11-15; DF KR-JP launches 2013-10-30."),
 ("2016", "DF MGame service ends (30 Sep); DE Steam relaunch (28 Nov)."),
]))
story.append(Spacer(1, 10))
story.append(Paragraph("APPENDIX B — GLOSSARY", st_chap))
story.append(Spacer(1, 6))
gloss = [
 ("hyeolmaeng (blood pledge)", "Lineage's clan: a state in miniature, founded only by royalty."),
 ("gongseongjeon (siege war)", "the scheduled battle for a castle and its tax."),
 ("naebok-dan", "'underwear corps': naked beginner volunteers of the Bartz war."),
 ("cheoksallyeong", "purge order: kill-on-sight decree against named players."),
 ("snaengteo tongje", "hunting-ground control: monopoly of XP/loot fields; the original sin."),
 ("EK (Enemy Kill)", "Helbreath's kill ledger, redeemable for Hero Armor."),
 ("adena", "Lineage's currency; its price was Bartz's inflation index."),
 ("jeonmyeonjeon / crusade", "Helbreath's twice-weekly national war."),
 ("PC bang", "the internet cafe: the room where the pledge was a neighbourhood."),
 ("jeong", "the Korean bond of obligation; what made a blood pledge a family."),
]
story.append(timeline([(k, v) for k, v in gloss], head=("TERM", "MEANING")))
story.append(Spacer(1, 10))
story.append(Paragraph("APPENDIX C — SOURCE REGISTRY", st_chap))
story.append(Spacer(1, 6))
SRC = [
 "[L1-1] Porter's Five Forces — Brief History of NCsoft. [L1-2] Gamota Lab — From IMF Crisis to Gaming Empire.",
 "[L1-3] Baidu Baike — Game Lineage. [L1-4] DualShockers — How Lineage Put South Korea On The MMO Map.",
 "[L1-5] IGN (2001) — Lineage: The Blood Pledge. [L1-6] mmorpg.com — Lineage.",
 "[HB-1] Wikipedia/en-academic — Helbreath. [HB-2] mmorpg.com (2004) — Helbreath Review.",
 "[HB-3] helbreath.net — event log & version archive. [HB-4] koit.co.kr (2001) — Siementech profile.",
 "[HB-5] Hankyoreh (1999-12-26). [HB-6] r/Games (2019) — Helbreath retrospective. [HB-7] second.wiki — Helbreath.",
 "[DE-1] darkeden-legend fandom. [DE-2] Gamia archive. [DE-3] Wikipedia — Darkeden. [DE-4] Massively OP (2015). [DE-5] NamuWiki (EN) — Dark Eden.",
 "[SO-1] mmorpg.com — Soma. [SO-2] myth-of-soma.com guide. [SO-3] Myth of Soma fandom.",
 "[BZ-1] Inven (2014). [BZ-2] ko.wikipedia — Bartz Liberation War. [BZ-3] NamuWiki — Bartz. [BZ-4] Shin Dong-A (2005-08), Lee In-hwa. [BZ-5] gameinsight.co.kr.",
 "[EQ-1] ko.wikipedia — EverQuest. [EQ-2] Sony press release (2002-09-19). [EQ-3] SOE milestones (P1999/B3D mirrors). [EQ-4] Baidu Baike / zh.wikipedia — EQ CN. [EQ-5] Wikipedia — EverQuest II. [EQ-6] NamuWiki — EQ2.",
 "[MR-1] SMH (2005-03-30). [MR-2] The Guardian (2005-06-09). [MR-3] china.org.cn (2005-03).",
 "[DF-1] Wikipedia — Darkfall. [DF-2] MMO Culture (2012). [DF-3] NamuWiki — Darkfall (MGame). [DF-4] Gametoc Hankyung (2013-08-14). [DF-5] Inven (2013).",
 "[RD-1..6] repo dossiers: docs/research-notes/{lineage1,helbreath,dark-eden,soma,mir2}/findings.md.",
]
for s in SRC:
    story.append(Paragraph(s, st_note))
story.append(Spacer(1, 12))
story.append(Paragraph("Compiled 2026-09-15 for the BloodHollow team. The prompt that produced "
 "this folio lives at docs/prompts/ancestral-chronicles-pdf-brief.md; the builder at "
 "tools/build_chronicles_pdf.py.", st_note))

# ================================================================ DOC
class Doc(BaseDocTemplate):
    def __init__(self, path):
        BaseDocTemplate.__init__(self, path, pagesize=A4,
                                 leftMargin=LM, rightMargin=RM, topMargin=TM, bottomMargin=BM,
                                 title="Blood & Banners — Chronicles of the Old Korean MMORPG Wars",
                                 author="BloodHollow research")
        self.chapter = ""
        nrm = Frame(LM, BM, CW, H - TM - BM, id="n")
        cov = Frame(0, 0, W, H, id="c", leftPadding=0, rightPadding=0, topPadding=0, bottomPadding=0)
        self.addPageTemplates([PageTemplate(id="cover", onPage=self._cover, frames=[cov]),
                               PageTemplate(id="normal", onPage=self._normal, frames=[nrm])])

    def _cover(self, c, doc):
        imgp = M("docs", "media", "chronicles", "01-cover.jpg")
        c.setFillColor(HexColor("#0B0806")); c.rect(0, 0, W, H, fill=1, stroke=0)
        if os.path.exists(imgp):
            c.drawImage(imgp, 0, 0, W, H, preserveAspectRatio=False, mask="auto")
        c.setFillColor(colors.black)
        for i in range(40):
            c.setFillAlpha(0.02 * i / 40.0 * 2.2)
            c.rect(0, 0, W, H * 0.45 * i / 40.0, fill=1, stroke=0)
        c.setFillAlpha(0.72); c.setFillColor(HexColor("#0B0806"))
        c.rect(0, 0, W, H * 0.34, fill=1, stroke=0)
        c.setFillAlpha(1)
        c.setFillColor(GOLD); c.setFont("MonoB", 8)
        c.drawString(LM, H * 0.30, "A BLOODHOLLOW RESEARCH FOLIO")
        y0 = H * 0.30 - 52; x0 = LM - 2
        if GOTHIC == "DarkGarden":
            w1 = c.stringWidth("BLOOD  ", GOTHIC, 44)
            w2 = c.stringWidth("&", "SerifB", 40)
            c.setFillColor(PAPER); c.setFont(GOTHIC, 44); c.drawString(x0, y0, "BLOOD")
            w1 = c.stringWidth("BLOOD", GOTHIC, 44)
            c.setFillColor(GOLD); c.setFont("SerifB", 40); c.drawString(x0 + w1 + 10, y0 + 3, "&")
            c.setFillColor(PAPER); c.setFont(GOTHIC, 44)
            c.drawString(x0 + w1 + 10 + w2 + 10, y0, "BANNERS")
        else:
            c.setFillColor(PAPER); c.setFont(GOTHIC, 44)
            c.drawString(x0, y0, "BLOOD & BANNERS")
        c.setFont("Serif", 12.5); c.setFillColor(HexColor("#E8DCC2"))
        c.drawString(LM, H * 0.30 - 76, "Chronicles of the old Korean MMORPG wars —")
        c.drawString(LM, H * 0.30 - 92, "politics, sieges & betrayal, 1997-2016")
        c.setFont("Mono", 7.5); c.setFillColor(HexColor("#B8A888"))
        c.drawString(LM, H * 0.06, "LINEAGE - HELBREATH - DARK EDEN - SOMA - BARTZ - EVERQUEST EAST - MIR - DARKFALL")
        c.drawRightString(W - LM, H * 0.06, "compiled 2026-09-15")

    def _normal(self, c, doc):
        c.setFillColor(PAPER); c.rect(0, 0, W, H, fill=1, stroke=0)
        if doc.page > 2:
            c.setStrokeColor(FAINT); c.setLineWidth(0.6)
            c.line(LM, H - 44, W - RM, H - 44)
            c.setFont("Mono", 6.8); c.setFillColor(HexColor("#8A7A5C"))
            c.drawString(LM, H - 40, "BLOOD & BANNERS")
            c.drawRightString(W - RM, H - 40, doc.chapter or "")
        c.setFont("Mono", 7.5); c.setFillColor(BLOOD)
        c.drawCentredString(W / 2, 40, "- %d -" % doc.page)

    def afterFlowable(self, fl):
        if isinstance(fl, ChapterOpener):
            self.chapter = fl.title
            self.notify("TOCEntry", (0, "%s  %s" % (fl.num, fl.title), self.page))
        elif isinstance(fl, Paragraph) and fl.style == st_chap:
            self.chapter = fl.getPlainText()
            self.notify("TOCEntry", (0, fl.getPlainText(), self.page))

doc = Doc(OUT)
doc.multiBuild(story)
sz = os.path.getsize(OUT)
print("OK pages=%d size=%.1fMB -> %s" % (doc.page, sz / 1e6, OUT))
