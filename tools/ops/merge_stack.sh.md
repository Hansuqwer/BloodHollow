# Merge prompt — stack #29→#48 (bottom-up, 20 PRs)

All PRs are stacked: #29 → #30 → #31 → … → #48. Merge bottom-up so each
base resolves cleanly. After each merge, the next PR auto-retargets.

Run from repo root (`/home/edwinhandler/Workspace/bloodhollow`).

## One-shot (safe)

```bash
bash tools/ops/merge_stack.sh
```

## Or manual (copy-paste)

```bash
for pr in 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48; do
  echo "=== merging #$pr ==="
  gh pr merge $pr --merge --admin
  sleep 5
  # verify mergeability before proceeding
  gh pr view $pr --json state --jq '.state'
done
```

## Notes

- `--admin` skips branch protection (adjust if repo has restrictions).
- If a conflict blocks one PR, stop at that PR — it's the blocker.
- After all merge: `git checkout master && git pull && ctest --preset linux-gcc`
- Open PRs: 29(T-129)→30(T-130)→31(T-127)→32(T-128)→33(T-129)→34(T-130)→35(T-131)→36(T-132)→37(T-133)→38(T-134)→39(T-135)→40(T-136)→41(T-137)→42(T-138)→43(T-139)→44(T-140)→45(T-141)→46(T-143)→47(T-144)→48(T-145)
