#!/usr/bin/env python3
"""Which DS offsets each module refers to (absolute memory operands, and immediates
that point at a string or are pushed as a near/far pointer): a guide to where each
module's _DATA is.

    python3 tools/se/dsrefs.py [--target savage|game]
"""
import os, re, sys
HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import target, modmap  # noqa: E402
from capstone import Cs, CS_ARCH_X86, CS_MODE_16
from capstone.x86 import X86_OP_MEM, X86_OP_IMM

md = Cs(CS_ARCH_X86, CS_MODE_16)
md.detail = True


def refs(img, st, en, dgroup):
    ds = dgroup * 16
    init = len(img) - ds
    out = set()
    for ins in md.disasm(img[st:en], st):
        if ins.mnemonic.startswith(('j', 'call', 'loop', 'lcall', 'ljmp')):
            continue
        for op in ins.operands:
            if op.type == X86_OP_MEM and not op.mem.base and not op.mem.index and op.mem.segment in (0,) :
                out.add(op.mem.disp & 0xFFFF)
            elif op.type == X86_OP_MEM and (op.mem.base or op.mem.index) and op.mem.disp >= 0x80:
                out.add(op.mem.disp & 0xFFFF)
            elif op.type == X86_OP_IMM and ins.mnemonic in ('mov', 'push') and 0x40 <= (op.imm & 0xFFFF):
                v = op.imm & 0xFFFF
                if v < init + 0x6000:
                    out.add(v)
    return out


if __name__ == '__main__':
    t = target.T
    img, _, _ = target.load_image(t['exe_path'])
    init = len(img) - t['dgroup'] * 16
    print('initialised data ends at DS:%04X' % init)
    for fr, st, en, e in modmap.module_list(t):
        r = sorted(refs(img, st, en, t['dgroup']))
        ini = [x for x in r if x < init]
        bss = [x for x in r if x >= init]
        print('%04X  init %-30s bss %s' % (fr, ('%04X..%04X (%d)' % (ini[0], ini[-1], len(ini))) if ini else '-',
                                           ('%04X..%04X (%d)' % (bss[0], bss[-1], len(bss))) if bss else '-'))
