#!/usr/bin/env python3
"""Write <SRC>/BSS.ASM: the uninitialised data of all the modules, laid out at their
original DS offsets.

    python3 tools/se/mkbss.py [--target savage|game] --start XXXX --end XXXX

Turbo C orders a module's _BSS by a hash of the names, so the C code only declares these
variables (extern D_XXXX or a named extern with its /*XXXX*/ offset comment) and this one
file defines them, in DS order, with LABELs and ORGs. START is the DS offset where the
modules' _BSS begins (the end of all initialised data), END where the run-time library's
_BSS begins. A variable referenced from C gets the C name (_D_XXXX or _name), from
assembly the plain name; both when both use it.
"""
import argparse, os, re, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target, sesym, shared  # noqa: E402


def scan(src):
    """{offset: set of link names}"""
    out = {}
    for dp, _, fs in os.walk(src):
        for fn in fs:
            for k, names in scan_file(os.path.join(dp, fn)).items():
                out.setdefault(k, set()).update(names)
    return out


def scan_file(p):
    """{offset: set of link names} for one source file"""
    out = {}
    low = os.path.basename(p).lower()
    if low == 'bss.asm' or not low.endswith(('.c', '.h', '.asm', '.inc')):
        return out
    s = open(p, encoding='latin-1').read()
    if low.endswith(('.c', '.h')):
        s = re.sub(r'//[^\n]*', '', s)
        # -p (GAME.EXE): Turbo C names variables the pascal way, D_XXXX, no underscore
        pre = '' if '-p' in sesym.default_flags(p).split() else '_'
        for m in re.finditer(r'\bD_([0-9A-F]{4})\b', s):
            out.setdefault(int(m.group(1), 16), set()).add(pre + 'D_' + m.group(1))
        for m in re.finditer(r'/\*([0-9A-F]{4})\*/\s*extern\s+[^;]*?\b(\w+)\s*(?:\[[^\]]*\])*\s*;', s):
            out.setdefault(int(m.group(1), 16), set()).add(pre + m.group(2))
    else:
        for m in re.finditer(r'^\s*EXTRN\s+([^;\n]+)', s, re.M | re.I):
            for item in m.group(1).split(','):
                name = item.split(':')[0].strip()
                mm = re.fullmatch(r'_?D_([0-9A-F]{4})', name)
                if mm:
                    out.setdefault(int(mm.group(1), 16), set()).add(name)
        for m in re.finditer(r'^\s*EXTRN\s+(\w+):\w+\s*;\s*D_([0-9A-F]{4})', s, re.M | re.I):
            out.setdefault(int(m.group(2), 16), set()).add(m.group(1))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--start', required=True)
    ap.add_argument('--end', required=True)
    a = ap.parse_args()
    start, end = int(a.start, 16), int(a.end, 16)
    t = target.T
    v = {k: n for k, n in scan(t['src']).items() if start <= k < end}
    # the other program's sources this one shares (../SRC_S/...): their uninitialised data
    # by the other program's address names, placed at this program's addresses
    dmap = shared.data_map()
    for m in shared.read_modules(t['src']):
        if not m['src'].startswith('..'):
            continue
        p = os.path.normpath(os.path.join(t['src'], m['src']))
        for k, names in scan_file(p).items():
            if k in dmap and start <= dmap[k] < end:
                v.setdefault(dmap[k], set()).update(names)
    lines = [';%s: the uninitialised data of all the modules (tools/se/mkbss.py writes this file)' % t['exe'],
             ';DS %04X-%04X; each module only declares what it uses.' % (start, end),
             '', '_BSS\tSEGMENT WORD PUBLIC \'BSS\'', '_BSS\tENDS', 'DGROUP\tGROUP _BSS', '',
             '_BSS\tSEGMENT WORD PUBLIC \'BSS\'']
    for off in sorted(v):
        lines.append('\tORG\t%04Xh' % (off - start))
        for n in sorted(v[off]):
            lines.append('\tPUBLIC\t%s' % n)
            lines.append('%s\tLABEL\tBYTE\t;D_%04X' % (n, off))
    lines.append('\tORG\t%04Xh' % (end - start))
    lines += ['_BSS\tENDS', '\tEND', '']
    open(os.path.join(t['src'], 'BSS.ASM'), 'w').write('\n'.join(lines))
    print('%d variables, %d names -> %s' % (len(v), sum(len(x) for x in v.values()), os.path.join(t['src'], 'BSS.ASM')))


if __name__ == '__main__':
    main()
