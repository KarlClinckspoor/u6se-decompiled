#!/usr/bin/env python3
"""Guess where each module's _DATA starts: modules' initialised data follow each other in
link order, so cut DS LO..HI into consecutive ranges, one per module, that hold as many of
each module's own DS references (dsrefs.py) as possible. A guide; check with secmp
--data-range.

    python3 tools/se/dsparts.py --target game [LO HI]      (default 00A8 2BE2)
"""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target, modmap, dsrefs  # noqa: E402

t = target.T
a = [x for x in sys.argv[1:]]
lo, hi = (int(a[0], 16), int(a[1], 16)) if len(a) >= 2 else (0xA8, 0x2BE2)
img, _, _ = target.load_image(t['exe_path'])
mods = []
for fr, st, en, e in modmap.module_list(t):
    r = sorted(x for x in dsrefs.refs(img, st, en, t['dgroup']) if lo <= x < hi)
    if r:
        mods.append((fr, r))
cand = sorted({lo, hi} | {x for _, r in mods for x in r})
idx = {c: i for i, c in enumerate(cand)}
N = len(cand)
P = []
for fr, r in mods:
    cnt = [0] * (N + 1)
    for x in r:
        cnt[idx[x] + 1] += 1
    for i in range(N):
        cnt[i + 1] += cnt[i]
    P.append(cnt)          # P[m][i]: refs of m below cand[i]
NEG = -10 ** 9
dp = [0 if i == 0 else NEG for i in range(N)]
back = []
for m in range(len(mods)):
    nd, nb, best, arg = [NEG] * N, [0] * N, NEG, 0
    for b in range(N):
        v = dp[b] - P[m][b]
        if v > best:
            best, arg = v, b
        nd[b], nb[b] = best + P[m][b], arg
    dp = nd
    back.append(nb)
b = N - 1
cuts = []
for m in range(len(mods) - 1, -1, -1):
    a0 = back[m][b]
    cuts.append((mods[m][0], cand[a0], cand[b]))
    b = a0
total = sum(len(r) for _, r in mods)
print('%d of %d references fall in their module\'s range' % (dp[N - 1], total))
for fr, s, e in reversed(cuts):
    r = dict(mods)[fr]
    inside = sum(1 for x in r if s <= x < e)
    print('%04X  %04X-%04X  %3d/%d' % (fr, s, e, inside, len(r)))
