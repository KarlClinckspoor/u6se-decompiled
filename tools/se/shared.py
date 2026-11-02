"""Sources one target takes from another (GAME.EXE uses SAVAGE.EXE's library): their
symbols at this target's addresses.

A line of <SRC>/modules.txt whose source is outside <SRC> (../SRC_S/LIB/HEAP.C) is a module
whose code is the same in both programs. Its definitions carry the other program's
addresses in their comments; code moves by the difference between the two modules' first
bytes, data by the difference between the two modules' _DATA ranges (the fifth column of
both modules.txt; data of a module without a range here is left out).
"""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target  # noqa: E402


def read_modules(src):
    out = []
    p = os.path.join(src, 'modules.txt')
    if not os.path.exists(p):
        return out
    for line in open(p):
        f = line.split('#')[0].split()
        if len(f) < 4:
            continue
        fr, at, ln, path = f[:4]
        data = f[4] if len(f) > 4 else None
        out.append(dict(frame=None if fr == '-' else int(fr, 16), at=None if at == '-' else int(at, 16),
                        len=int(ln, 16), src=path, data=tuple(int(x, 16) for x in data.split('-')) if data else None))
    return out


def translated(parse_file):
    """{key: (name, file, lang, static)} for the shared sources of the current target"""
    me = target.T
    out = {}
    mine = [m for m in read_modules(me['src']) if m['src'].startswith('..')]
    if not mine:
        return out
    others = {}
    for name, t in target.TARGETS.items():
        if name == me['name']:
            continue
        src = os.path.join(target.ROOT, t['src'])
        for m in read_modules(src):
            others[os.path.normpath(os.path.join(src, m['src']))] = m
    for m in mine:
        path = os.path.normpath(os.path.join(me['src'], m['src']))
        o = others.get(path)
        if not o or o['frame'] is None or m['frame'] is None or not os.path.exists(path):
            continue
        t = {}
        parse_file(path, m['src'], t)
        ostart = o['frame'] * 16 + o['at']
        for k, v in t.items():
            if k[0] == 'C':
                rel = k[1] * 16 + k[2] - ostart
                if 0 <= rel < o['len']:
                    out[('C', m['frame'], m['at'] + rel)] = v
            elif o['data'] and m['data']:
                rel = k[1] - o['data'][0]
                if 0 <= rel < o['data'][1] - o['data'][0]:
                    out[('D', m['data'][0] + rel)] = v
    return out
