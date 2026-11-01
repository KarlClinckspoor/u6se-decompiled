#!/usr/bin/env python3
"""Split the code after the run-time library into modules: one per frame (a TLINK
segment's paragraph), starting at the first instruction after the previous module's last
return within the frame's first 16 bytes. Lists each module's functions.

    python3 tools/se/modmap.py [--target savage|game] [--lst DIR]
"""
import argparse, os, re, struct, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target, segmap  # noqa: E402
from capstone import Cs, CS_ARCH_X86, CS_MODE_16

md = Cs(CS_ARCH_X86, CS_MODE_16)
RET = {0xC3, 0xCB, 0xCF, 0xC2, 0xCA, 0xE9, 0xEB, 0xEA}


def modules(img, relocs, dgroup, lib_end):
    ent = segmap.entries(img, relocs, dgroup)
    frames = sorted(f for f in ent if f * 16 >= lib_end - 15)
    starts = []
    prev_start = lib_end
    for fr in frames:
        lo = fr * 16
        if lo < prev_start:
            lo = prev_start
        # linear sweep of the previous module up to here: instruction boundaries
        st = None
        pos = prev_start
        last_end_ret = None
        for ins in md.disasm(img[prev_start:fr * 16 + 16], prev_start):
            a = ins.address
            if a >= lo and a < fr * 16 + 16 and last_end_ret == a:
                st = a
                break
            if ins.bytes[0] in RET:
                last_end_ret = a + ins.size
        if st is None:
            st = max(lo, fr * 16)
        first = min(ent[fr]) if ent[fr] else None
        if first is not None and first < 16 and fr * 16 + first != st:
            st = fr * 16 + first   # a far entry in the first paragraph is the module's start
        starts.append((fr, st))
        prev_start = st
    return starts, ent


def module_list(t=None):
    t = t or target.T
    img, relocs, _ = target.load_image(t['exe_path'])
    lib_end = {'savage': 0x34CB, 'game': 0x2685}[t['name']]
    starts, ent = modules(img, relocs, t['dgroup'], lib_end)
    ends = [s for _, s in starts[1:]] + [t['dgroup'] * 16]
    return [(fr, st, en, sorted(ent[fr])) for (fr, st), en in zip(starts, ends)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--lst')
    a = ap.parse_args()
    t = target.T
    img, relocs, _ = target.load_image(t['exe_path'])
    lib_end = int(os.environ.get('SE_LIBEND', '0'), 16) or {'savage': 0x34CB, 'game': 0x2685}[t['name']]
    starts, ent = modules(img, relocs, t['dgroup'], lib_end)
    ends = [s for _, s in starts[1:]] + [t['dgroup'] * 16]
    for (fr, st), en in zip(starts, ends):
        code = img[st:en]
        offs = sorted(ent[fr])
        prologues = [m.start() + st - fr * 16 for m in re.finditer(b'\x55\x8b\xec', code)]
        far_rets = code.count(b'\xcb') + len(re.findall(b'\xca..', code, re.S))
        print('%04X  start +%X  len %5d  far entries %2d  bp-frames %3d   %s' % (
            fr, st - fr * 16, en - st, len(offs), len(prologues), ' '.join('%04X' % o for o in offs[:10])))


if __name__ == '__main__':
    main()
