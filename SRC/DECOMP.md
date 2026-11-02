# GAME.EXE decompiled

`GAME.EXE` is the game itself, started by `SAVAGE.EXE`. Its layout is Ultima VI's
`GAME.EXE`: the same modules in the same order (main, gameloop, console, map, visible,
object, status, talk, talkdr, spell, npctrack, combat, objcmd, egg, music, gfx, kbd, rest,
disk), the same OSI assembly library, then Savage Empire's own library, which `SAVAGE.EXE`
shares (`../SRC_S`). The method and the tools are those of `SRC_S/DECOMP.md`; this file says
what differs for GAME.EXE.

## Status (2026-11-02)

**Done: `python3 tools/se/sebuild.py --target game` builds a GAME.EXE equal to the
original**, header and its 3,240 relocations included (sha1
8d6c103d12fbe93d4fc4f4f74ccecbe571bca1a9). All 53 modules have a source and
`python3 tools/se/secheck.py --target game` finds them identical one by one.

`modules.txt` lists every module after the run-time library in link order (frame, first
byte, length, source, `_DATA` range); data-only modules have "-" as frame. About 950 of
the 1,220 functions and variables the sources define have a name; the rest keep their
address names with a comment. `docs/GAME_NOTES.md` collects what the work taught about
Turbo C++ 1.00 (register choice, -y and cross-jumping, condition forms), module by module.

What the link needed:
- `msgs.c` and `monsdata.c` (far data, after the code): compiled huge model with the data
  class renamed FAR_DATA (`-zTFAR_DATA`), so that TLINK puts them after the code and
  before the start-up tables. monsdata ends with 14 zero bytes nothing uses.
- `SEGORDER.ASM`, linked before C0M: in GAME.EXE the start-up code's `_BSSEND` comes before
  `_BSS` (bdata@ = edata@ = DS 3268: the start-up code clears nothing). The module names
  C0M's segments in that order. How Origin got it is not known.
- `BSS.ASM` (`tools/se/mkbss.py --target game --start 3268 --end EEB2`): the uninitialised
  data, DS 3268-EEB2; the run-time library's follow (EEB2-EFDC), then the stack.
- `gamestat.c` (DS 27E8-286C) and `PARTYTOP.ASM` (DS 08A2): data-only modules, as in U6.
- The OSI file routines (`OSILIB/OSI_FILE.ASM`) are linked right after `main.c`: their
  code joins C0M's `_TEXT`, their data follow main's.
- Names: with `-p` Turbo C names variables the pascal way too (upper case, no underscore),
  so a variable defined in cdecl code (the shared library, `_soundCard`) is declared
  `cdecl` in the game's modules; `secmp.py` reports one that is not.

## Where to start from

GAME.EXE is Ultima VI's engine, changed. For each module start from Ultima VI's readable
source, `/home/claude/u6-decompiled/SRC/<module>.c` (its headers are next to it), and change
it until it compiles to Savage Empire's bytes:

- `u6map.txt` gives, for every GAME.EXE function, the closest Ultima VI function and how
  close it is (0-1, by instruction sequence). Above about 0.85 the function is U6's with
  small changes; around 0.5 it is rewritten but recognisable; "(new)" means no U6
  function is close (the name after "nearest" is only a hint).
- Keep U6's function names where the function is the same function (same job), even when
  the code changed. Name new functions only when you are sure what they do; otherwise
  keep `C_SSSS_OOOO` and say in a comment what it seems to do.
- U6's object, tile and spell numbers (`obj.h`, `tile.h`, `spells.h`, `cmd.h`, `ai.h`) are
  Ultima VI's. Savage Empire has its own; do not reuse U6's macros: write the numbers, and
  name them in a comment when you know them.

## Rules of the sources

All of `SRC_S/DECOMP.md`'s rules hold (address comments before every definition, address
names for what other modules define, `--data-range`, uninitialised data `extern` only,
comments). Besides:

- **Flags:** the game modules are compiled `-d -mm -p -c -Z -O` (pascal calling convention,
  as in Ultima VI), the target's default. If two copies of the same statement were merged
  into a jump where the original has both, or the original has a `jmp` onto a `jmp`, the
  module was compiled with line numbers: add `-y` (`/* flags: -d -mm -p -c -Z -O -y */` on
  the first line). `-G` is worse. Functions that are `cdecl` in the original (variable
  arguments, or called by C code that pushes and pops) say `cdecl`.
- **Headers:** `se.h` has the types and macros that do not depend on where a variable is
  (`int16`, `union tHandler`, `struct coord`, `DELTA`, `VA_LIST`, ...). Each module
  declares the variables and functions it uses itself, near the top, by address name
  (`D_XXXX`, `C_SSSS_OOOO`) or by name when the defining module has given one. Declare
  structures a module needs in the module (copy them from U6's headers when they still
  fit). Common headers come at the end, when every module is done.
- **Big modules** may be split in parts while they are being decompiled (`objcmd_1.c`,
  `objcmd_2.c`, ...), each one compared at its own first byte; they are joined before the
  link.

## Checking

    python3 tools/se/sedis.py --target game FRAME          annotated listing of a module
    python3 tools/se/secmp.py --target game SRC/disk.c --seg 2A81 --at 3
    python3 tools/se/secmp.py --target game SRC/disk.c --seg 2A81 --at 3 --data-range 2A64 2B34
    python3 tools/se/dsparts.py --target game               where each module's _DATA likely is
    python3 tools/se/secheck.py --target game [FRAME]

`dsparts.py` cuts the data segment into one range per module from the DS references each
module makes: a first guess, off by some bytes or a whole variable at the ends. A module's
own data are its initialised variables in order, then its string literals in order of use.

## The data segment

| DS | What |
|---|---|
| 0000-00A7 | C0M's data |
| 00A8-2B34 | the game modules' data, in link order (see modules.txt) |
| 2B34-2BE2 | the OSI assembly's data |
| 2BE2-2BE6 | the random seed (2B57) |
| 2BE6-2F64 | Savage Empire's library (music, EMS, heap, LZW, files, timer, calibration) |
| 2F64-3268 | CMSE.LIB's data (3250-3268 zero-initialised) |
| 3268-EEB2 | the game's uninitialised data (BSS.ASM; zeros in the file, `TLINK /i`) |
| EEB2-EFDC | CMSE.LIB's uninitialised data |
| EFE0-F060 | the start-up stack segment (the game's stack is moved to the top of DGROUP) |
