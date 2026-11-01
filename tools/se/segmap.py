#!/usr/bin/env python3
"""The code segments (frames) of an executable and the entry points in each, from its
relocations: every relocated segment word in code names a frame, every far call
(9A off seg) or far pointer an entry point.

    python3 tools/se/segmap.py [--target savage|game]
"""
import argparse, collections, os, struct, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target  # noqa: E402


def entries(img, relocs, dgroup):
    """{frame: set(offsets)} of far-called/far-pointed code, and frame -> count of refs"""
    ce = dgroup * 16
    ent = collections.defaultdict(set)
    for r in relocs:
        seg = struct.unpack_from('<H', img, r)[0]
        if seg >= dgroup or r < 2:
            continue
        off = struct.unpack_from('<H', img, r - 2)[0]
        if r < ce and r >= 3 and img[r - 3] in (0x9A, 0xEA):
            ent[seg].add(off)
        elif r >= ce or (r >= 3 and img[r - 3] not in (0x9A, 0xEA)):
            ent[seg].add(off) if seg * 16 + off < ce else None
        ent.setdefault(seg, set())
    # push cs / call near rel16 (0E E8 xx xx): a far function of the same module
    for frame in list(ent):
        pass
    return ent


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--target')
    a = ap.parse_args()
    t = target.get(a.target)
    img, relocs, _ = target.load_image(t['exe_path'])
    ent = entries(img, relocs, t['dgroup'])
    frames = sorted(ent)
    frames.append(t['dgroup'])
    for i, fr in enumerate(frames[:-1]):
        offs = sorted(ent[fr])
        nxt = frames[i + 1]
        print('%04X  next %04X  span %5d  entries %3d  first %s' % (
            fr, nxt, (nxt - fr) * 16, len(offs), ' '.join('%04X' % o for o in offs[:12])))


if __name__ == '__main__':
    main()
