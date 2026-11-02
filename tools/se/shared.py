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


def data_map():
    """{other program's DS offset: this program's} for the uninitialised data the shared modules
    use: their code is the same in both programs but for DS offsets, so the operands of the two
    copies, instruction by instruction, pair the addresses. (The initialised data move with
    their modules' _DATA ranges, translated() above.) Cached in build/."""
    import json
    me = target.T
    cache = os.path.join(target.ROOT, 'build', 'shared_dmap_%s.json' % me['name'])
    mine = [m for m in read_modules(me['src']) if m['src'].startswith('..') and m['frame'] is not None]
    if not mine:
        return {}
    srcs = [os.path.join(me['src'], 'modules.txt')]
    for name, t in target.TARGETS.items():
        if name != me['name']:
            srcs.append(os.path.join(target.ROOT, t['src'], 'modules.txt'))
    stamp = max(os.path.getmtime(p) for p in srcs if os.path.exists(p))
    if os.path.exists(cache) and os.path.getmtime(cache) >= stamp:
        return {int(k): v for k, v in json.load(open(cache)).items()}
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    from capstone.x86 import X86_OP_MEM, X86_OP_IMM
    md = Cs(CS_ARCH_X86, CS_MODE_16)
    md.detail = True
    gimg = target.load_image(me['exe_path'])[0]
    out = {}
    for name, t in target.TARGETS.items():
        if name == me['name']:
            continue
        o = target.get(name)
        oimg = target.load_image(o['exe_path'])[0]
        oinit = o['bss']  # where its uninitialised data start
        others = {os.path.normpath(os.path.join(o['src'], m['src'])): m for m in read_modules(o['src'])}
        for m in mine:
            om = others.get(os.path.normpath(os.path.join(me['src'], m['src'])))
            if not om or om['frame'] is None:
                continue
            n = min(m['len'], om['len'])
            gs, os_ = m['frame'] * 16 + m['at'], om['frame'] * 16 + om['at']
            for gi, oi in zip(md.disasm(gimg[gs:gs + n], 0), md.disasm(oimg[os_:os_ + n], 0)):
                if gi.address != oi.address or gi.size != oi.size:
                    break
                for go, oo in zip(gi.operands, oi.operands):
                    if go.type == oo.type == X86_OP_MEM:
                        a, b = oo.mem.disp & 0xFFFF, go.mem.disp & 0xFFFF
                    elif go.type == oo.type == X86_OP_IMM:
                        a, b = oo.imm & 0xFFFF, go.imm & 0xFFFF
                    else:
                        continue
                    if a != b and a >= oinit:
                        out.setdefault(a, b)
    os.makedirs(os.path.dirname(cache), exist_ok=True)
    json.dump(out, open(cache, 'w'))
    return out
