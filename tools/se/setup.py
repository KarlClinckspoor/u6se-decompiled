#!/usr/bin/env python3
"""Unpack Turbo C++ 1.00 and 1.01 from the disk-image archives in tools/ and build the
run-time library Savage Empire was linked with.

    python3 tools/se/setup.py

Needs: py7zr (pip), mtools (mcopy), unzip (Info-ZIP: Borland's ZIPs are imploded), DOSBox.
Result:
  tools/tcpp100/   TCC.EXE TLINK.EXE TLIB.EXE INCLUDE\\ LIB\\ (C0M.OBJ, CM.LIB, ...)
  tools/tcpp100/LIB/CMSE.LIB   1.00's CM.LIB with FARHEAP, NEARHEAP and FINDFIRS replaced by
                               1.01's, moved to the end of the library as `tlib -+` does:
                               the order GAME.EXE and SAVAGE.EXE have them in
  tools/tcpp101/LIB/CM.LIB     (kept for reference)
"""
import glob, os, shutil, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import dos, omf  # noqa: E402

FROM_101 = ['FARHEAP', 'NEARHEAP', 'FINDFIRS']


def unpack(archive, dst):
    import py7zr
    with tempfile.TemporaryDirectory() as tmp:
        with py7zr.SevenZipFile(archive) as z:
            z.extractall(tmp)
        zips = os.path.join(tmp, 'zips')
        os.makedirs(zips)
        for img in sorted(glob.glob(os.path.join(tmp, '**', '*.img'), recursive=True)):
            subprocess.run(['mcopy', '-n', '-i', img, '::*.ZIP', zips + '/'], check=False,
                           stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        os.makedirs(os.path.join(dst, 'INCLUDE'), exist_ok=True)
        os.makedirs(os.path.join(dst, 'LIB'), exist_ok=True)
        want = {'TCC.ZIP': '', 'BIN1.ZIP': '', 'INCLUDE.ZIP': 'INCLUDE', 'MLIB.ZIP': 'LIB',
                'STARTUP.ZIP': 'STARTUP'}
        for z, sub in want.items():
            out = os.path.join(dst, sub)
            os.makedirs(out, exist_ok=True)
            subprocess.run(['unzip', '-qo', '-UU', os.path.join(zips, z), '-d', out], check=True)
        for dp, _, fs in os.walk(dst):  # DOS names in upper case
            for f in fs:
                if f != f.upper():
                    os.rename(os.path.join(dp, f), os.path.join(dp, f.upper()))


def main():
    tools = dos.TOOLS
    a100 = glob.glob(os.path.join(tools, '*1.00*.7z'))
    a101 = glob.glob(os.path.join(tools, '*1.01*.7z'))
    if not a100 or not a101:
        sys.exit('put the Turbo C++ 1.00 and 1.01 .7z archives in tools/')
    for arc, d in ((a100[0], 'tcpp100'), (a101[0], 'tcpp101')):
        dst = os.path.join(tools, d)
        shutil.rmtree(dst, ignore_errors=True)
        unpack(arc, dst)
        print('unpacked', os.path.basename(arc), '->', dst)
    # CMSE.LIB: 1.00's CM.LIB in its order, then the three modules from 1.01
    mods = [m.name.upper() for m in omf.parse_lib(os.path.join(tools, 'tcpp100', 'LIB', 'CM.LIB'))]
    mods = [m for m in mods if m not in FROM_101] + FROM_101
    with tempfile.TemporaryDirectory() as tmp:
        c = os.path.join(tmp, 'c')
        os.makedirs(c)
        dos.install_tools(c, masm=False)
        os.makedirs(os.path.join(c, 'M'))
        shutil.copy(os.path.join(tools, 'tcpp100', 'LIB', 'CM.LIB'), os.path.join(c, 'M', 'CM100.LIB'))
        shutil.copy(os.path.join(tools, 'tcpp101', 'LIB', 'CM.LIB'), os.path.join(c, 'M', 'CM101.LIB'))
        lines = ['cd \\M']
        rest = [m for m in mods if m not in FROM_101]
        for i in range(0, len(rest), 12):
            lines.append('\\TC\\TLIB CM100.LIB %s > NUL' % ' '.join('*' + m for m in rest[i:i + 12]))
        lines.append('\\TC\\TLIB CM101.LIB %s > NUL' % ' '.join('*' + m for m in FROM_101))
        rsp = [' '.join('+' + x for x in mods[i:i + 8]) + (' &' if i + 8 < len(mods) else '')
               for i in range(0, len(mods), 8)]
        open(os.path.join(c, 'M', 'CMSE.RSP'), 'w', newline='').write('\r\n'.join(rsp) + '\r\n')
        lines.append('\\TC\\TLIB CMSE.LIB @CMSE.RSP > TLIB.LOG')
        dos.run(c, lines)
        out = os.path.join(c, 'M', 'CMSE.LIB')
        if not os.path.exists(out):
            sys.exit('TLIB failed')
        shutil.copy(out, os.path.join(tools, 'tcpp100', 'LIB', 'CMSE.LIB'))
    got = [m.name.upper() for m in omf.parse_lib(os.path.join(tools, 'tcpp100', 'LIB', 'CMSE.LIB'))]
    print('CMSE.LIB: %d modules%s' % (len(got), '' if got == mods else ' (ORDER DIFFERS)'))


if __name__ == '__main__':
    main()
