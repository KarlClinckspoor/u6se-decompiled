#!/usr/bin/env python3
"""Write the target's symbol file with the run-time library's names: C0M.OBJ's publics and
those of every CMSE.LIB module found in the image (tools/se/libscan.py).

    python3 tools/se/seedsyms.py [--target savage|game]      (rewrites tools/se/<target>_syms.txt)
"""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target, omf, libscan  # noqa: E402

t = target.T
img, rel, _ = target.load_image(t['exe_path'])
lines = ['# %s symbols: NAME SEG:OFF (code, far) or NAME OFF (DS offset).' % t['exe'],
         '# Library entries: Turbo C++ 1.00 C0M.OBJ and CMSE.LIB modules matched in the image',
         '# (tools/se/seedsyms.py); the rest learned by tools/se/secmp.py --learn.']
c0 = omf.parse_obj(os.path.join(target.ROOT, 'tools', 'tcpp100', 'LIB', 'C0M.OBJ'))
for n, (si, o) in sorted(c0.pubs.items(), key=lambda x: x[1][1]):
    if si and c0.segs[si].name == '_TEXT':
        lines.append('%s 0000:%04X' % (n, o))
mods = {m.name: m for m in omf.parse_lib(os.path.join(target.ROOT, 'tools', 'tcpp100', 'LIB', 'CMSE.LIB'))}
for st, n, name, seg in libscan.scan(img, 0, t['dgroup'] * 16):
    m = mods[name]
    if st >= 0x10000:
        continue
    for pn, (si, o) in sorted(m.pubs.items(), key=lambda x: x[1][1]):
        if si and m.segs[si].name == seg:
            lines.append('%s 0000:%04X' % (pn, st + o))
open(t['syms'], 'w').write('\n'.join(lines) + '\n')
print('%d names -> %s' % (len(lines) - 3, t['syms']))
