#!/usr/bin/env python3
"""Match every function of GAME.EXE's game modules with its closest Ultima VI function.

    python3 tools/se/u6match.py U6_SRC U6_BUILD_DIR U6_GAME.EXE > SRC/u6map.txt

U6_SRC: u6-decompiled's SRC/ (readable names); U6_BUILD_DIR: a kept tools/dosbuild.py build
of it (objects and GAME.MAP); U6_GAME.EXE: the original, EXEPACKed. U6's functions come from
the objects' public names placed with the map, and from the /*C_SSSS_OOOO*/ comments in the
source (static functions). Savage Empire's functions start at each `push bp / mov bp,sp`
(Turbo C++ always makes a frame). The similarity is difflib's ratio over the instructions
with addresses and constants masked; the U6 module in the same place is tried first.
"""
import difflib, glob, os, re, struct, sys
from collections import Counter, defaultdict
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import omf, target  # noqa: E402
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

md = Cs(CS_ARCH_X86, CS_MODE_16)
U6BASE = 0x838
# SE frame -> U6 module (file name in U6's SRC)
U6MOD = {0x0268: 'main', 0x0359: 'gameloop', 0x05D0: 'console', 0x08CB: 'map', 0x09DA: 'visible', 0x0A5D: 'object',
         0x0E13: 'status', 0x0F79: 'talk', 0x0F8D: 'talkdr', 0x11FF: 'spell', 0x130F: 'npctrack', 0x1810: 'combat',
         0x1DBD: 'objcmd', 0x2667: 'egg', 0x2701: 'music', 0x276B: 'gfx', 0x29D8: 'rest', 0x2A81: 'disk'}


def toks(code, base):
    out = []
    for i in md.disasm(code, base):
        s = i.mnemonic + ' ' + i.op_str
        if i.mnemonic.startswith(('j', 'call', 'lcall', 'loop')):
            s = i.mnemonic + ' T'
        s = re.sub(r'\b0x[0-9a-f]{3,}\b', 'N', s)
        out.append(s)
    return out


def unexepack(path):
    sys.path.insert(0, os.path.join(target.ROOT, '..', 'u6-decompiled', 'tools'))
    import importlib.util
    spec = importlib.util.spec_from_file_location('exepack', os.path.join(os.path.dirname(os.path.abspath(path)), 'exepack.py')) if False else None
    d = open(path, 'rb').read()
    hdr = struct.unpack_from('<H', d, 8)[0]
    last, pages = struct.unpack_from('<HH', d, 2)
    img = d[hdr * 16:(pages - 1) * 512 + (last or 512)]
    cs = struct.unpack_from('<H', d, 0x16)[0]
    base = cs * 16
    dlen = struct.unpack_from('<H', img, base + 12)[0]
    buf = bytearray(img[:base]) + bytearray(max(0, dlen * 16 - base))
    src, dst = base, dlen * 16
    while buf[src - 1] == 0xFF:
        src -= 1
    while True:
        cmd = buf[src - 1]
        n = buf[src - 3] | buf[src - 2] << 8
        src -= 3
        if cmd & 0xFE == 0xB0:
            b = buf[src - 1]; src -= 1
            buf[dst - n:dst] = bytes([b]) * n; dst -= n
        else:
            buf[dst - n:dst] = buf[src - n:src]; dst -= n; src -= n
        if cmd & 1:
            break
    return bytes(buf[:dlen * 16])


def u6_functions(srcdir, builddir):
    segs = {}
    for l in open(os.path.join(builddir, 'GAME.MAP'), encoding='latin-1'):
        m = re.match(r'\s*([0-9A-F]{5})H ([0-9A-F]{5})H ([0-9A-F]{5})H (\S+)\s+(\S+)', l)
        if m and m.group(5) == 'CODE':
            segs[m.group(4).upper()] = (int(m.group(1), 16), int(m.group(3), 16))
    funcs = {}   # linear -> (name, module)
    proper = {}  # the source's spelling of every function name (pascal publics are upper case)
    for p in glob.glob(os.path.join(srcdir, '*.c')):
        for m in re.finditer(r'^\s*(?:/\*C_\w+\*/)?\s*(?:(?:static|far|near|pascal|cdecl|unsigned|signed|int|char|long|void|short|struct\s+\w+)\s+|\*)*(\w+)\s*\([^;]*\)\s*\{', open(p, encoding='latin-1').read(), re.M):
            proper.setdefault(m.group(1).upper(), m.group(1))
    for objp in glob.glob(os.path.join(builddir, '*.OBJ')):
        mod = os.path.splitext(os.path.basename(objp))[0].lower()
        try:
            m = omf.parse_obj(objp)
        except Exception:
            continue
        for name, (si, off) in m.pubs.items():
            if si and m.segs[si].cls == 'CODE' and m.segs[si].name.upper() in segs:
                n = name.lstrip('_')
                funcs[segs[m.segs[si].name.upper()][0] + off] = (proper.get(n.upper(), n), mod)
    for p in glob.glob(os.path.join(srcdir, '*.c')):
        mod = os.path.splitext(os.path.basename(p))[0]
        s = open(p, encoding='latin-1').read()
        for m in re.finditer(r'/\*C_([0-9A-F]{4})_([0-9A-F]{4})\*/\s*(?:[\w\*]+\s+)*?\**\s*(\w+)\s*\(', s):
            lin = (int(m.group(1), 16) - U6BASE) * 16 + int(m.group(2), 16)
            funcs.setdefault(lin, (m.group(3), mod))
    return funcs, segs


def u6addr(a, segs):
    st = max(s for s, l in segs.values() if s <= a)
    return 'C_%04X_%04X' % ((st >> 4) + U6BASE, a - ((st >> 4) << 4))


def main():
    srcdir, builddir, exe = sys.argv[1:4]
    uimg = unexepack(exe)
    ufuncs, usegs = u6_functions(srcdir, builddir)
    ends = sorted(set(ufuncs) | {s for s, l in usegs.values()} | {s + l for s, l in usegs.values()})
    U = {}
    for st in sorted(ufuncs):
        en = next(e for e in ends if e > st)
        name, mod = ufuncs[st]
        U[st] = dict(name=name, mod=mod, size=en - st, toks=toks(uimg[st:en], st))
    t = target.get('game')
    img, _, _ = target.load_image(t['exe_path'])
    import shared
    mods = shared.read_modules(t['src'])
    starts = [(m['frame'] * 16 + m['at'], m) for m in mods if m['frame']]
    rows = []
    for i, (st, m) in enumerate(starts):
        if m['frame'] not in U6MOD:
            continue
        en = st + m['len']
        # function starts: push bp / mov bp,sp at instruction boundaries
        fs = [ins.address for ins in md.disasm(img[st:en], st) if ins.bytes[:3] == b'\x55\x8b\xec' or
              (ins.bytes == b'\x55' and img[ins.address + 1:ins.address + 3] == b'\x8b\xec')]
        fs = sorted(set(fs)) or [st]
        if fs[0] != st:
            fs = [st] + fs
        for k, fst in enumerate(fs):
            fen = fs[k + 1] if k + 1 < len(fs) else en
            T = toks(img[fst:fen], fst)
            best = (0.0, None)
            home = [a for a, u in U.items() if u['mod'] == U6MOD[m['frame']]]
            for pool in (home, list(U)):
                for a in pool:
                    u = U[a]
                    if abs(len(u['toks']) - len(T)) > max(len(T), len(u['toks'])) * 0.6:
                        continue
                    sm = difflib.SequenceMatcher(None, T, u['toks'], autojunk=False)
                    if sm.real_quick_ratio() <= best[0] or sm.quick_ratio() <= best[0]:
                        continue
                    r = sm.ratio()
                    if r > best[0]:
                        best = (r, a)
                if best[0] >= 0.7:
                    break
            r, a = best
            rows.append((m['frame'], fst - m['frame'] * 16, fen - fst, r, a))
    print('# GAME.EXE function -> closest Ultima VI function (tools/se/u6match.py)')
    print('# SE address  size  similarity  U6 name (U6 module, U6 address as in its source C_SSSS_OOOO)')
    for fr, off, size, r, a in rows:
        if a is None or r < 0.5:
            print('C_%04X_%04X %5d  %.2f  (new)%s' % (fr, off, size, r, ('  nearest %s' % U[a]['name']) if a else ''))
        else:
            u = U[a]
            print('C_%04X_%04X %5d  %.2f  %s (%s, %s)' % (fr, off, size, r, u['name'], u['mod'], u6addr(a, usegs)))


if __name__ == '__main__':
    main()
