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
    return open(p, encoding='latin-1', newline='').read()


def table():
    """{('C', seg, off) or ('D', off): (name, file, lang, static)}

    For a target whose modules.txt names sources of another target (GAME.EXE uses
    SAVAGE.EXE's library, ../SRC_S/...), those files' definitions are moved to this
    target's addresses (tools/se/shared.py)."""
    t = {}
    for p in files():
        parse_file(p, os.path.relpath(p, SRC), t)
    for (sg, o), n in PASCAL.items():
        t[('C', sg, o)] = (n, '?', 'pascal', False)
    try:
        import shared
        for k, v in shared.translated(parse_file).items():
            t.setdefault(k, v)
    except ImportError:
        pass
    return t


def default_flags(p):
    """the flags a C source is compiled with: its own flags line, or its target's default"""
    import tcc
    ap = os.path.abspath(p)
    dflt = target.T['flags']
    for name, tt in target.TARGETS.items():
        if ap.startswith(os.path.join(ROOT, tt['src']) + os.sep):
            dflt = tt['flags']
    return tcc.file_flags(p, dflt)


def parse_file(p, fn, t):
    """add the definitions of source file p (named fn) to table t"""
    if True:
        s = read(p)
        if p.lower().endswith(('.c', '.h')):
            # compiled with -p (GAME.EXE's modules), functions are pascal unless declared cdecl
            dpascal = '-p' in default_flags(p).split()
            for m in re.finditer(r'/\*C_([0-9A-F]{4})_([0-9A-F]{4})\*/\s*((?:[\w\*]+\s+)*?)\**\s*(\w+)\s*\(', s):
                quals = m.group(3).split()
                if 'extern' in quals:  # a declaration, not the definition
                    continue
                lang = 'pascal' if 'pascal' in quals or (dpascal and 'cdecl' not in quals) else 'c'
                t[('C', int(m.group(1), 16), int(m.group(2), 16))] = (m.group(4), fn, lang, 'static' in quals)
            for m in re.finditer(r'/\*([0-9A-F]{4})\*/\s*(?:static\s+)?[\w\s\*]+?\b(D_[0-9A-F]{4}|\w+)\s*[\[=;]', s):
                if re.search(r'\bextern\b', m.group(0)):
                    continue
                # -p makes variables pascal too (upper case, no underscore)
                dl = 'pascal' if dpascal and not re.search(r'\bcdecl\b', m.group(0)) else 'c'
                t.setdefault(('D', int(m.group(1), 16)), (m.group(2), fn, dl, False))
            # function pointers: /*XXXX*/void (*name)(...)
            for m in re.finditer(r'/\*([0-9A-F]{4})\*/\s*(?:static\s+)?[\w\s\*]*?\(\s*(?:far\s+)?\*\s*(\w+)\s*\)\s*\(', s):
                if re.search(r'\bextern\b', m.group(0)):
                    continue
                dl = 'pascal' if dpascal and not re.search(r'\bcdecl\b', m.group(0)) else 'c'
                t.setdefault(('D', int(m.group(1), 16)), (m.group(2), fn, dl, False))
            # far data (a huge-model module such as msgs.c): /*C_SSSS_OOOO*/ before a variable
            for m in re.finditer(r'/\*C_([0-9A-F]{4})_([0-9A-F]{4})\*/\s*((?:[\w\*]+\s+)*?)\**\s*(\w+)\s*[\[=;]', s):
                quals = m.group(3).split()
                if 'extern' in quals:
                    continue
                lang = 'pascal' if dpascal and 'cdecl' not in quals else 'c'
                t[('C', int(m.group(1), 16), int(m.group(2), 16))] = (m.group(4), fn, lang, 'static' in quals)
        else:
            for m in re.finditer(r'^(?:FProc|ProcStart|NProc)\s+(\w+)\s*;\s*([0-9A-F]{4}):([0-9A-F]{4})', s, re.M):
                t[('C', int(m.group(2), 16), int(m.group(3), 16))] = (m.group(1), fn, 'asm', False)
            for m in re.finditer(r'^\s*PUBLIC\s+(\w+)\s*;\s*([0-9A-F]{4}):([0-9A-F]{4})', s, re.M):
                t[('C', int(m.group(2), 16), int(m.group(3), 16))] = (m.group(1), fn, 'asm', False)
            for m in re.finditer(r'^(\w+)\s+PROC\s+\w+\s*;\s*([0-9A-F]{4}):([0-9A-F]{4})', s, re.M):
                t[('C', int(m.group(2), 16), int(m.group(3), 16))] = (m.group(1), fn, 'asm', False)
            for m in re.finditer(r'^(\w+)\s+(?:db|dw|dd|LABEL)\b[^;\n]*;\s*D_([0-9A-F]{4})\b', s, re.M):
                t[('D', int(m.group(2), 16))] = (m.group(1), fn, 'asm', False)
            # public data in a code segment: `NAME db ... ;SSSS:OOOO` (with PUBLIC NAME)
            pub = {n.lower() for l in re.findall(r'^\s*PUBLIC\s+([^;\n]+)', s, re.M | re.I)
                   for n in re.split(r'[\s,]+', l.strip()) if n}
            for m in re.finditer(r'^(\w+)\s+(?:db|dw|dd|DB|DW|DD|LABEL)\b[^;\n]*;\s*([0-9A-F]{4}):([0-9A-F]{4})\b', s, re.M):
                if m.group(1).lower() in pub:
                    t[('C', int(m.group(2), 16), int(m.group(3), 16))] = (m.group(1), fn, 'asm', False)
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
                refs = set(re.findall(r'\b(C_[0-9A-F]{4}_[0-9A-F]{4}|D_[0-9A-F]{4})\b', src))
                if '-p' in default_flags(p).split():
                    # compiled -p (GAME.EXE): every name is pascal but those declared cdecl
                    nocom = re.sub(r'/\*.*?\*/', ' ', src, flags=re.S)
                    cd = set(re.findall(r'\bcdecl\b[^;{\n]*?\b(C_[0-9A-F]{4}_[0-9A-F]{4}|D_[0-9A-F]{4})\b', nocom))
                    used_by_c.update(cd)
                    pascal_in_c.update(refs - cd)
                else:
                    used_by_c.update(refs)
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
        # in assembly also the C link name of an address: _C_SSSS_OOOO, _D_XXXX
        atok = re.compile(r'(?<![\w@$?])_?(C_[0-9A-F]{4}_[0-9A-F]{4}|D_[0-9A-F]{4})\b')
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
                    code = atok.sub(lambda m: amap.get(m.group(1), m.group(0)), code)
                    for old, new in rename_asm_defs.get(os.path.relpath(p, SRC), {}).items():
                        code = re.sub(r'(?<!\w)%s\b' % re.escape(old), new, code)
                    out.append(code + sep + com)
                s = '\n'.join(out)
            if s != o:
                open(p, 'w', encoding='latin-1', newline='').write(s)
                print('rewrote', os.path.relpath(p, SRC))


if __name__ == '__main__':
    main()
