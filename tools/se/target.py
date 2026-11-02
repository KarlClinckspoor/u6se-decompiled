"""The two executables of the decompilation and what differs between them.

Pick one with --target or the SE_TARGET environment variable (default: savage)."""
import os, struct

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))

TARGETS = {
    # SAVAGE.EXE: launcher, title, intro, character creation, endgame (cdecl)
    'savage': dict(exe='SAVAGE.EXE', src='SRC_S', dgroup=0x0DAA, syms='savage_syms.txt', bss=0x27FE,
                   flags='-mm -c -O -Z -d'),
    # GAME.EXE: the game (pascal)
    'game': dict(exe='GAME.EXE', src='SRC', dgroup=0x2FA6, syms='game_syms.txt', bss=0x3250,
                 link_first=['SEGORDER.ASM'],  # linked before C0M (see the file)
                 flags='-mm -p -c -O -Z -d'),
}


def get(name=None):
    name = name or os.environ.get('SE_TARGET') or 'savage'
    t = dict(TARGETS[name])
    t['name'] = name
    t['src'] = os.path.join(ROOT, t['src'])
    t['syms'] = os.path.join(HERE, t['syms'])
    t['exe_path'] = find_exe(t['exe'])
    return t


def find_exe(name):
    for p in (os.environ.get('SE_GAMEDIR') and os.path.join(os.environ['SE_GAMEDIR'], name),
              os.path.join(ROOT, 'gamedata', 'SAVAGE', name), os.path.join(ROOT, 'gamedata', name)):
        if p and os.path.exists(p):
            return p
    return os.path.join(ROOT, 'gamedata', 'SAVAGE', name)


def load_image(path):
    """(image bytes, set of image addresses holding relocated segment words, header bytes)"""
    d = open(path, 'rb').read()
    last, pages = struct.unpack_from('<HH', d, 2)
    size = (pages - 1) * 512 + (last or 512)
    hdrp = struct.unpack_from('<H', d, 8)[0]
    nrel, relofs = struct.unpack_from('<H', d, 6)[0], struct.unpack_from('<H', d, 0x18)[0]
    relocs = set()
    for i in range(nrel):
        off, seg = struct.unpack_from('<HH', d, relofs + 4 * i)
        relocs.add(seg * 16 + off)
    return d[hdrp * 16:size], relocs, d[:hdrp * 16]


def _from_argv():
    """--target NAME anywhere on a tool's command line selects the target (and is removed)"""
    import sys
    a = sys.argv
    for i, x in enumerate(a):
        if x == '--target' and i + 1 < len(a):
            os.environ['SE_TARGET'] = a[i + 1]
            del a[i:i + 2]
            return
        if x.startswith('--target='):
            os.environ['SE_TARGET'] = x.split('=', 1)[1]
            del a[i]
            return


_from_argv()
T = get()
