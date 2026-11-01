"""Run DOS batch files in a headless DOSBox (or DOSBox-X) on a scratch C: drive."""
import os, shutil, subprocess

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
TOOLS = os.environ.get('SE_DOSTOOLS') or os.path.join(ROOT, 'tools')


def dosbox():
    return os.environ.get('SE_DOSBOX') or shutil.which('dosbox-x') or shutil.which('dosbox') or 'dosbox'


def crlf(src, dst):
    b = open(src, 'rb').read().replace(b'\r\n', b'\n').replace(b'\n', b'\r\n')
    open(dst, 'wb').write(b)


def run(cdir, lines, timeout=1800):
    """Write lines to cdir/RUN.BAT and run it with cdir mounted as C:."""
    open(os.path.join(cdir, 'RUN.BAT'), 'w', newline='').write('\r\n'.join(['@echo off'] + lines) + '\r\n')
    conf = os.path.join(cdir, '..', 'dosbox.conf')
    open(conf, 'w').write('[sdl]\noutput=surface\n[dosbox]\nmemsize=16\n[cpu]\ncycles=max\n'
                          '[autoexec]\nmount c "%s"\nc:\ncall \\RUN.BAT\nexit\n' % os.path.abspath(cdir))
    env = dict(os.environ, SDL_VIDEODRIVER='dummy', SDL_AUDIODRIVER='dummy')
    db = dosbox()
    args = [db, '-conf', conf, '-noconsole'] if 'dosbox-x' not in os.path.basename(db).lower() \
        else [db, '-conf', conf, '-nogui', '-exit']
    subprocess.run(args, env=env, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=timeout)


def install_tools(cdir, masm=True):
    """Copy Turbo C++ 1.00 (tools/tcpp100) to C:\\TC and MASM 3.0 to C:\\TC\\MASM.EXE."""
    tc = os.path.join(TOOLS, 'tcpp100')
    if not os.path.isdir(tc):
        raise SystemExit('tools/tcpp100 missing: run python3 tools/se/setup.py')
    shutil.copytree(tc, os.path.join(cdir, 'TC'))
    if masm:
        mdir = os.path.join(TOOLS, 'masm300')
        exe = [n for n in os.listdir(mdir) if n.upper() == 'MASM.EXE']
        if exe:
            shutil.copy(os.path.join(mdir, exe[0]), os.path.join(cdir, 'TC', 'MASM.EXE'))
        else:
            img = [n for n in os.listdir(mdir) if n.lower().endswith('.img')][0]
            subprocess.run(['mcopy', '-o', '-i', os.path.join(mdir, img), '::MASM.EXE',
                            os.path.join(cdir, 'TC', 'MASM.EXE')], check=True)
