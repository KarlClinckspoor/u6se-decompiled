#!/usr/bin/env python3
"""Build the target from its sources with the original tools in a headless DOSBox, and
compare the result with the original executable byte for byte.

    python3 tools/se/sebuild.py [--target savage|game] [--keep DIR] [--log]

<SRC>/modules.txt gives the modules in link order (their sources: .c compiled with the
flags of their first line or the target's, .asm assembled with MASM 3.0 /Mx and their
fixups put in TASM's order, tools/se/tasmorder.py). The link is
    TLINK /m /i C0M <modules> BSS, <EXE>, <MAP>, CMSE
with C0M.OBJ and CMSE.LIB from Turbo C++ 1.00 (tools/se/setup.py). BSS.ASM, when there is
one, lays out the uninitialised data of all the modules. /i ("initialise all segments")
is Origin's: the .EXE holds the uninitialised data and the stack as zeros.
"""
import argparse, hashlib, os, shutil, struct, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import dos, target, tcc, tasmorder, secheck, sesym  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--keep')
    ap.add_argument('--log', action='store_true')
    a = ap.parse_args()
    t = target.T
    src = t['src']
    mods = secheck.modules()
    missing = [m for m in mods if m[3] == '-']
    if missing:
        sys.exit('%d modules have no source yet (first: frame %s)' % (len(missing), missing[0][0]))
    files = []
    for m in mods:  # a module with several code segments has one line per segment
        if m[3] not in files:
            files.append(m[3])
    # far data compiled huge model (GAME.EXE's msgs.c, monsdata.c) is linked before C0M, as
    # Ultima VI's MAKEFILE does: their empty code segments go first, their data segments
    # first in class DATA, so the data end up right before DGROUP
    def flags_of(f):
        return sesym.default_flags(os.path.join(src, f)) if f.lower().endswith('.c') else ''
    first = [f for f in files if '-mh' in flags_of(f).split()]
    files = first + [f for f in files if f not in first]
    if os.path.exists(os.path.join(src, 'BSS.ASM')):
        files.append('BSS.ASM')
    exe = t['exe']
    with tempfile.TemporaryDirectory() as tmp:
        c = os.path.join(tmp, 'c')
        os.makedirs(c)
        dos.install_tools(c)
        s = os.path.join(c, 'S')
        os.makedirs(s)
        # every source and header, flattened into one directory under short names
        names = []
        for k, f in enumerate(files):
            base = 'M%03d' % k
            ext = os.path.splitext(f)[1].upper()
            dos.crlf(os.path.join(src, f), os.path.join(s, base + ext))
            names.append((f, base, ext))
        dirs = {os.path.normpath(os.path.join(src, os.path.dirname(f))) for f in files}
        for d in [src] + sorted(dirs):
            for dp, _, fs in os.walk(d):
                for fn in fs:
                    if fn.lower().endswith(('.h', '.inc')) and not os.path.exists(os.path.join(s, fn.upper())):
                        dos.crlf(os.path.join(dp, fn), os.path.join(s, fn.upper()))
        lines = ['set PATH=\\TC', 'cd \\S']
        for f, base, ext in names:
            if ext == '.ASM':
                lines.append('\\TC\\MASM /Mx %s; >> \\BUILD.LOG' % base)
            elif tcc.has_inline_asm(os.path.join(src, f)):
                fl = sesym.default_flags(os.path.join(src, f))
                lines.append('TCC -I\\TC\\INCLUDE %s -S %s.C >> \\BUILD.LOG' % (fl.replace('-c', '').replace('  ', ' '), base))
                lines.append('\\TC\\MASM /Mx %s.ASM; >> \\BUILD.LOG' % base)
            else:
                fl = sesym.default_flags(os.path.join(src, f))
                lines.append('TCC -I\\TC\\INCLUDE %s %s.C >> \\BUILD.LOG' % (fl, base))
        dos.run(c, lines)
        for f, base, ext in names:
            if ext == '.ASM' and os.path.exists(os.path.join(s, base + '.OBJ')):
                tasmorder.fix(os.path.join(src, f), os.path.join(s, base + '.OBJ'))
        objs = [base for f, base, ext in names]
        nfirst = len(first)
        rsp, line = [], ' '.join(objs[:nfirst] + ['\\TC\\LIB\\C0M'])
        objs = objs[nfirst:]
        for o in objs:
            if len(line) + len(o) > 100:
                rsp.append(line + '+')
                line = ''
            line += ' ' + o
        rsp.append(line)
        rsp.append('%s,%s,\\TC\\LIB\\CMSE' % (exe.split('.')[0], exe.split('.')[0]))
        open(os.path.join(s, 'LINK.RSP'), 'w', newline='').write('\r\n'.join(rsp) + '\r\n')
        dos.run(c, ['set PATH=\\TC', 'cd \\S', 'TLINK /m /i @LINK.RSP >> \\BUILD.LOG'])
        log = open(os.path.join(c, 'BUILD.LOG'), encoding='latin-1').read() if os.path.exists(os.path.join(c, 'BUILD.LOG')) else ''
        if a.log:
            print(log)
        for l in log.splitlines():
            if l.startswith(('Error', 'Fatal')) or 'Undefined symbol' in l or ': error' in l.lower():
                print(l)
        if a.keep:
            shutil.copytree(s, a.keep, dirs_exist_ok=True)
            open(os.path.join(a.keep, 'NAMES.TXT'), 'w').write('\n'.join('%s %s' % (b, f) for f, b, e in names) + '\n')
        built = os.path.join(s, exe)
        if not os.path.exists(built):
            sys.exit('no %s was built' % exe)
        new = open(built, 'rb').read()
    old = open(t['exe_path'], 'rb').read()
    print('%s %d bytes sha1 %s; original %d bytes sha1 %s' % (
        exe, len(new), hashlib.sha1(new).hexdigest(), len(old), hashlib.sha1(old).hexdigest()))
    if new == old:
        print('identical to the original')
        return
    hn, ho = struct.unpack_from('<H', new, 8)[0] * 16, struct.unpack_from('<H', old, 8)[0] * 16
    if new[:hn] != old[:ho]:
        nrel = struct.unpack_from('<H', new, 6)[0], struct.unpack_from('<H', old, 6)[0]
        print('headers/relocations differ (header %x vs %x bytes, %d vs %d relocations)' % (hn, ho, nrel[0], nrel[1]))
    ni, oi = new[hn:], old[ho:]
    diffs = [i for i in range(min(len(ni), len(oi))) if ni[i] != oi[i]]
    print('image: %d vs %d bytes, %d bytes differ' % (len(ni), len(oi), len(diffs)))
    starts = [(int(m[0], 16) * 16 + int(m[1], 16), m[3]) for m in mods if m[0] != '-']
    shown, last = 0, -100
    for i in diffs:
        if i - last > 16 and shown < 12:
            mod = '?'
            for st, f in starts:
                if st <= i:
                    mod = f
            print('  at image %05X (%s): %s | original %s' % (i, mod, ni[i:i + 8].hex(), oi[i:i + 8].hex()))
            shown += 1
        last = i
    sys.exit(1)


if __name__ == '__main__':
    main()
