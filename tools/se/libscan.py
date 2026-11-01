"""Find the run-time library modules (tools/tcpp100/LIB/CMSE.LIB) in an executable:
their code is compared byte for byte, the bytes under fixups aside."""
import os, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import omf, target  # noqa: E402


def code_segs(m):
    for s in m.segs[1:]:
        if s.cls == 'CODE' and any(s.mask):
            mask = bytearray(s.mask)
            for f in s.fixups:
                for k in range(f.size):
                    if f.offset + k < len(mask):
                        mask[f.offset + k] = 0
            n = max(i for i in range(len(s.mask)) if s.mask[i]) + 1
            yield s, bytes(s.data[:n]), mask[:n]


def scan(img, lo, hi, lib=None):
    """[(image address, length, module name, segment name)] sorted"""
    lib = lib or os.path.join(target.ROOT, 'tools', 'tcpp100', 'LIB', 'CMSE.LIB')
    out = []
    for m in omf.parse_lib(lib):
        for s, code, mask in code_segs(m):
            # anchor on the longest run of compared bytes
            best, i = (0, 0), 0
            while i < len(code):
                if not mask[i]:
                    i += 1
                    continue
                j = i
                while j < len(code) and mask[j]:
                    j += 1
                if j - i > best[1] - best[0]:
                    best = (i, j)
                i = j
            a, b = best
            if b - a < 4:
                continue
            p = img.find(code[a:b], lo)
            while p != -1 and p < hi:
                st = p - a
                if st >= lo and all(not mask[k] or code[k] == img[st + k] for k in range(len(code))):
                    out.append((st, len(code), m.name, s.name))
                    break
                p = img.find(code[a:b], p + 1)
    return sorted(out)


if __name__ == '__main__':
    t = target.get(sys.argv[1] if len(sys.argv) > 1 else None)
    img, rel, _ = target.load_image(t['exe_path'])
    for st, n, name, seg in scan(img, 0, t['dgroup'] * 16):
        print('%05X %5d %-10s %s' % (st, n, name, seg))
