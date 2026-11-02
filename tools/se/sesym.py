#!/usr/bin/env python3
"""The symbol table of the decompilation (one target at a time), and the pass that links the modules by
name: references by address (C_SSSS_OOOO, D_XXXX) become the names the defining module
gave them.

    python3 tools/se/sesym.py [--target savage|game] list    the table (address, name, defined in)
    python3 tools/se/sesym.py [--target savage|game] apply   rewrite the sources (C and assembly)

Where the names come from:
  - C:        /*C_SSSS_OOOO*/ before a function definition; /*XXXX*/ before a variable.
  - assembly: `FProc NAME ;SSSS:OOOO`, `NAME PROC ... ;SSSS:OOOO`, and data
              `NAME dw ... ;D_XXXX`.
A C function's link name is _name; assembly called from C gets a leading underscore so
that C can call it by the same name (pascal routines keep their names: Turbo C upper-
cases them and TLINK ignores case).
"""
import collections, os, re, sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import target  # noqa: E402

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = target.T['src']
# pascal routines of the hand-written assembly, by address: {(seg, off): name}
PASCAL = {}


def files():
    out = []
    for dp, ds, fs in os.walk(SRC):
        ds.sort()
        for fn in sorted(fs):
            if fn.lower().endswith(('.c', '.h', '.asm', '.inc')):
                out.append(os.path.join(dp, fn))
    return out


def read(p):
    return open(p, encoding='latin-1').read()


def table():
    """{('C', seg, off) or ('D', off): (name, file, lang)}"""
    t = {}
    for p in files():
        s = read(p)
        fn = os.path.relpath(p, SRC)
        if p.lower().endswith(('.c', '.h')):
            for m in re.finditer(r'/\*C_([0-9A-F]{4})_([0-9A-F]{4})\*/\s*((?:[\w\*]+\s+)*?)\**\s*(\w+)\s*\(', s):
                quals = m.group(3)
                lang = 'pascal' if 'pascal' in quals.split() else 'c'
                t[('C', int(m.group(1), 16), int(m.group(2), 16))] = (m.group(4), fn, lang, 'static' in quals)
            for m in re.finditer(r'/\*([0-9A-F]{4})\*/\s*(?:static\s+)?[\w\s\*]+?\b(D_[0-9A-F]{4}|\w+)\s*[\[=;]', s):
                t.setdefault(('D', int(m.group(1), 16)), (m.group(2), fn, 'c', False))
        else:
            for m in re.finditer(r'^(?:FProc|ProcStart|NProc)\s+(\w+)\s*;\s*([0-9A-F]{4}):([0-9A-F]{4})', s, re.M):
                t[('C', int(m.group(2), 16), int(m.group(3), 16))] = (m.group(1), fn, 'asm', False)
            for m in re.finditer(r'^(\w+)\s+PROC\s+\w+\s*;\s*([0-9A-F]{4}):([0-9A-F]{4})', s, re.M):
                t[('C', int(m.group(2), 16), int(m.group(3), 16))] = (m.group(1), fn, 'asm', False)
            for m in re.finditer(r'^(\w+)\s+(?:db|dw|dd|LABEL)\b[^;\n]*;\s*D_([0-9A-F]{4})\b', s, re.M):
                t[('D', int(m.group(2), 16))] = (m.group(1), fn, 'asm', False)
    for (sg, o), n in PASCAL.items():
        t[('C', sg, o)] = (n, '?', 'pascal', False)
    return t


def cname(entry):
    """the name C code uses for a symbol"""
    name, fn, lang, static = entry
    if lang == 'asm':
        return name[1:] if name.startswith('_') else name
    return name


def main():
    t = table()
    if len(sys.argv) < 2 or sys.argv[1] == 'list':
        byname = collections.defaultdict(list)
        for k, v in sorted(t.items()):
            addr = 'C_%04X_%04X' % k[1:] if k[0] == 'C' else 'D_%04X' % k[1]
            print('%-12s %-28s %s%s' % (addr, v[0], v[1], ' (static)' if v[3] else ''))
            if not v[3]:
                byname[cname(v).lower()].append(addr)
        for n, addrs in byname.items():
            if len(addrs) > 1:
                print('DUPLICATE public name %s: %s' % (n, ' '.join(addrs)))
        return
    if sys.argv[1] == 'apply':
        cmap, amap = {}, {}  # address name -> name for C code / for assembly
        rename_asm_defs = {}  # file -> {asm label: new label (gets an underscore)}
        used_by_c = set()
        pascal_in_c = set()  # routines C declares pascal: their link name has no underscore
        for p in files():
            if p.lower().endswith(('.c', '.h')):
                src = read(p)
                used_by_c.update(re.findall(r'\b(C_[0-9A-F]{4}_[0-9A-F]{4}|D_[0-9A-F]{4})\b', src))
                pascal_in_c.update(re.findall(r'\bpascal\b[^;{()]*?\b(C_[0-9A-F]{4}_[0-9A-F]{4})\s*\(', src))
        used_by_c -= pascal_in_c
        for k, v in t.items():
            addr = 'C_%04X_%04X' % k[1:] if k[0] == 'C' else 'D_%04X' % k[1]
            if v[2] in ('c', 'pascal') and v[0] == addr:
                # still named by its address: only assembly needs the C link name
                amap[addr] = ('_' + addr) if v[2] == 'c' else addr
                continue
            cmap[addr] = cname(v)
            if v[2] == 'asm':
                label = v[0]
                if addr in used_by_c and not label.startswith('_'):
                    label = '_' + v[0]
                    rename_asm_defs.setdefault(v[1], {})[v[0]] = label
                cmap[addr] = label[1:] if label.startswith('_') else label
                amap[addr] = label
            elif v[2] == 'pascal':
                amap[addr] = v[0]
            else:
                amap[addr] = '_' + v[0]
        tok = re.compile(r'\b(C_[0-9A-F]{4}_[0-9A-F]{4}|D_[0-9A-F]{4})\b')
        for p in files():
            s = read(p)
            o = s
            if p.lower().endswith(('.c', '.h')):
                # not the address comments /*C_SSSS_OOOO*/ themselves
                s = re.sub(r'(/\*)?\b(C_[0-9A-F]{4}_[0-9A-F]{4}|D_[0-9A-F]{4})\b(\*/)?',
                           lambda m: m.group(0) if (m.group(1) and m.group(3)) else
                           (m.group(1) or '') + cmap.get(m.group(2), m.group(2)) + (m.group(3) or ''), s)
            else:
                out = []
                for line in s.split('\n'):
                    code, sep, com = line.partition(';')
                    code = tok.sub(lambda m: amap.get(m.group(1), m.group(1)), code)
                    for old, new in rename_asm_defs.get(os.path.relpath(p, SRC), {}).items():
                        code = re.sub(r'(?<!\w)%s\b' % re.escape(old), new, code)
                    out.append(code + sep + com)
                s = '\n'.join(out)
            if s != o:
                open(p, 'w', encoding='latin-1').write(s)
                print('rewrote', os.path.relpath(p, SRC))


if __name__ == '__main__':
    main()
