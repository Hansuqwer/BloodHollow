# Lineage 1 / Lineage 2 loot, crafting, class & clan — reference for comparison

*Pasted by the director 2026-09-16 (external author, no repo access). Part A is
a research prompt describing L1/L2 mechanics; Part B is a BloodHollow design
proposal written against `<placeholder>` assumptions about our code. Treat as
the comparison baseline for
`docs/prompts/systems-audit-race-class-loot-vs-lineage.md` — not as law.*

---

# PART A — Research Prompt

> ## Role
> You are a systems designer + reverse-engineer documenting the **loot, crafting, enchanting, class, and clan systems of Lineage 1 (Lineage: The Blood Pledge) and Lineage 2 (Prelude → Interlude, with notes on later chronicles)** for the purpose of re-implementing a faithful-but-adapted version in an indie dark-fantasy game. Precision matters more than breadth: prefer exact numbers, formulas, tables, and state machines over prose.
>
> ## Sources to prioritize (cite everything)
> - **Open-source emulators** (treat as primary technical sources): L1J / L1J-TW / L1J-JP (Lineage 1), L2J / L2JServer / L2J-Mobius / aCis / L2JFrozen (Lineage 2). Pull actual data from `droplist`, `spoil`, `recipes.xml`, `skills/*.xml`, `enchant` config, `player class templates`, `clan` config, `castle` config.
> - Official/semi-official docs: Lineage 2 Interlude/C4 official "Game Guide", PMfun / L2Wiki (Classic + Chronicle), Lineage 1 official (lineage.plaync) class/item pages, LinDB, LinCraft-era fan DBs.
> - Community mechanic write-ups (enchant rate tests, drop formula analyses, party-loot behavior), flagged as secondary.
>
> ## Deliverables (in this order)
>
> ### 1. Lineage 1 (Blood Pledge)
> 1.1 **Loot**: how drop tables are structured (per-monster item list, chance, min/max count), adena/gold drops, boss vs. normal drops, drop-on-death rules for players (PK/karma, lawful points, which slots, chance), item weight and inventory limits, "class-locked" items (Royal/Knight/Elf/Wizard/Dark Elf), quest-gated items.
> 1.2 **Crafting/creation**: confirm that L1 has no player craft skill; document all NPC-based item creation/exchange (material-hunting NPCs, Elven quest items, Dwarf NPC roles as warehouse keepers, polymorph scroll economy, elixirs). Document any later-added crafting (Evolution/Episode era) as an appendix.
> 1.3 **Enchanting**: safe enchant caps (weapon vs. armor), success behavior, failure behavior (item destruction), Blessed/Cursed scrolls, stacking of enchant with damage/AC, and any level-gated enchant differences.
> 1.4 **Royal (Prince/Princess) class and the Blood Pledge**: creation requirements, pledge ranks, pledge warehouse, castle sieges (which castles, siege windows, tax), Royal-only skills (Call Clan, Run Clan, Brave Aura, Shining/Glowing Aura, etc.) with numbers, and how the Royal's weaker personal combat was balanced by leadership utility.
> 1.5 Lineage 1 **class list** with role summary, primary stats, and signature skills.
