# T-169 — In-client discoverability: `/help`, verb panel, first five minutes (P0, audit 20/20)

## Context
Audit finding **D / E4.1 / P0-5**. The client's entire on-screen guidance is two
lines (`client/src/game.cpp:1586-1588`): `LMB walk/fight - 1 PowerSwing - Q sip -
I bag - T trade (P commit/X cancel) - Enter chat` and `I inventory - V vendor -
Space recenter - F3 grid - ESC quit`. The server accepts **18 chat verbs**
(`/accept /breach /confess /crown /duel /forfeit /invite /kick /kit /leave /mine
/oath /p /pledge /refine /repair /repent /siege-reg`) plus skills on `2..5`, stat
spends on `F5/F6/F7`, anvil on `F`, sell-junk on `G`, vendor buys on `F1..F12`,
anvil tiers on `F8+`, and `gm *` for operators. None of it is discoverable, so a
new player cannot find their class oath, the refine syntax, mining, duels, pledges
or the siege without a Discord message. MVP §6 leg 1 assumes ten humans can race
to L12 in three hours; today they cannot even pick a class without being told.
Debug keys `H`/`N` (hour offset) and `F3`/`F4` are also player-reachable.

## Scope
- `/help` (and `?`) → a directed, paginated system line listing every verb the
  *player* may use, generated from **one table in the server** (the same switch
  that parses them — no second source of truth to drift).
- A persistent era-parchment help panel (toggle key, default visible for the first
  N minutes or until dismissed) listing hotkeys + the verb list; text-only, no art
  dependency (icons come with T-156).
- **First five minutes**: on first login ever, a short scripted system sequence —
  welcome, `/kit` prompt with the three kit one-liners, stat-spend keys, vendor +
  bag keys, party key, and "type /help for everything". One-time flag persisted
  (or derived from level 1 + no kit) so it never repeats.
- Gate the debug keys: `H`/`N`/`F3`/`F4` behind an env/flag
  (`BH_CLIENT_DEBUG=1`) so players do not desync their own clock view.
- OUT: a tutorial zone, quest-driven onboarding, tooltips on art (T-156), key
  rebinding.

## Acceptance criteria
1. `/help` output lists all 18 player verbs + hotkeys, and matches the parser
   table (unit pin that walks the table, so a new verb cannot ship unlisted).
2. First-login sequence fires once per character and never again (persist/derive
   pin); a fresh bot/client run shows the lines in order (paste the chat log).
3. Help panel toggles and does not overlap the party/anvil/trade panels (screenshot
   at 1024×768).
4. With `BH_CLIENT_DEBUG` unset, `H`/`N`/`F3`/`F4` do nothing.
5. No wire change beyond chat text; replay `mm=0` (help is unjournaled like
   `/pledge who` — state that explicitly).

## Tests required
Verb-table/help parity pin; first-login once-only pin; debug-gate pin.

## Evidence owed at merge
Chat log of the first five minutes, `/help` output, screenshots, suite count,
devlog, board row.
