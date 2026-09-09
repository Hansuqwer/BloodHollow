# 0067 — The floor is nowhere near the ceiling (T-098)

T-098, read-only. The decal surface was built with a 512 cap and a promise to check whether 512 was enough. Seven legs say it is barely trying: peak 210, headroom 2.4×, zero evictions, zero callers for two of the three decal kinds.

## Judgment calls

- Arithmetic over instrumentation: kill counts upper-bound blood decals exactly (one kill, at most one stain; TTL covers the longest leg), so a profiling harness would measure what subtraction already proves. The probe cost one grep, not one leg.
- The interesting finding is the empty lanes: T-091's telegraph rings have never bloomed in a live leg (no map-5 traffic), and spell circles have no producer at all. The surface is ready; the content is not. Both callers arrive with their own cards and their own pressure math — this verdict covers blood only, stated plainly.
- FIFO-as-decay-gracefully is worth recording because it makes the cap a soft landing, not a cliff: if pressure ever doubles, the visible change is blood fading slightly early, not breakage. That is why the reopen rule watches for visible pop-out, not for a number crossing 512.
