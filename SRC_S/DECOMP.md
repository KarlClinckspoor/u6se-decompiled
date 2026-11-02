# SAVAGE.EXE decompiled

`SAVAGE.EXE` is the program you start: it checks the configuration and the saved game,
plays the title, the introduction and character creation (Intanya's questions), shows the
endgame, and runs `GAME.EXE`. This folder holds its decompilation: C and assembly that
Turbo C++ 1.00, MASM 3.0 and TLINK 3.0 build back into the same `SAVAGE.EXE`, byte for
byte. The method and tools are those of U6's `U.EXE` (`u6-decompiled`, branch `uexe`,
`SRC_U/DECOMP.md`), moved to Turbo C++.

## Status

`modules.txt` lists every module after the run-time library in link order: its frame, its
first byte, its length, and its source file once one exists. `python3 tools/se/secheck.py
--target savage` compares every module that has a source.

## The tools

All of them take `--target savage` (the default) or `--target game`.

    python3 tools/se/setup.py                 unpack Turbo C++ 1.00/1.01, build CMSE.LIB (once)
    python3 tools/se/sedis.py SEG [END]       annotated disassembly of a frame (build/lst/ has them all)
    python3 tools/se/secmp.py FILE --seg SEG --at AT [--data-range START END]
                                              compile one module and compare it with the original
    python3 tools/se/secheck.py [FILE|FRAME]  compare every module in modules.txt
    python3 tools/se/sesym.py list|apply      the symbol table: names given in the sources
    python3 tools/se/libscan.py               where the run-time library modules are
    python3 tools/se/modmap.py, dsrefs.py     how modules.txt was made; DS references per module

`secmp.py` compiles the file in DOSBox, then compares its code segment with the original
at `SEG:AT`. Bytes under fixups are not compared byte for byte; each fixup is checked
instead (far calls by name or C_SSSS_OOOO address, DS offsets by D_XXXX or by the module's
_DATA base). It prints the first instruction that really differs, with both disassemblies.

## Rules of the sources

- **Compiler:** Turbo C++ 1.00. A C file names its flags on its first line:
  `/* flags: -mm -c -O */`. Without that line the target's default is used
  (`-mm -c -O -Z -d`). Known so far: the main program wants `-mm -c -O` (cdecl);
  Origin's library routines are often `pascal` (declare them `pascal`, or use `-p`).
  Turbo C++ 1.00 always makes a stack frame (`push bp / mov bp,sp`) and reuses AX after
  `mov si,ax` (`cmp ax,...`): that is how it is told from Turbo C 2.0.
- **Assembly:** hand-written modules are `.asm` files that MASM 3.0 (`/Mx`) assembles to
  the same bytes (Origin used TASM). MASM's differences, as found for U.EXE: `OFFSET
  DGROUP:x` for DS offsets, `FAR PTR` for forward far calls, `SHORT` for forward short
  jumps. When MASM insists on another encoding of an instruction, write that one
  instruction as `db` with the instruction in a comment. If TASM wrote fixups in another
  order than MASM (it only matters to the relocation table), the module says so with a
  `;TASM fixup order:` comment (`tools/se/tasmorder.py`).
- **Names.** Every function definition has its address before it: `/*C_SSSS_OOOO*/`
  (frame, offset; asm: `;SSSS:OOOO` after the PROC or label). Every global variable you
  define has its DS offset before it: `/*XXXX*/` (asm: `;D_XXXX` comment). Things defined
  in other modules are referred to by address: `C_SSSS_OOOO` for a function, `D_XXXX` for
  a variable. A function or variable you define may get a real name; `sesym.py apply`
  later turns the address references in other modules into that name. Give a name only
  when you are confident; otherwise keep the address name and say in a comment what it
  seems to do.
- **Initialised data (_DATA):** each module defines its own initialised variables, in the
  order they are in the data segment, followed by its string literals in order of use
  (Turbo C puts a module's string literals after its variables). `--data-range START END`
  checks a module's _DATA exactly. Modules' _DATA follow each other in link order from
  DS:00A8 (C0M's data is DS:0000-00A8). Report the range you find for each module.
- **Uninitialised data:** the variables after the initialised data (DS offsets from about
  27FE up) are only declared, `extern`, in C (EXTRN in asm), never defined. One file,
  `BSS.ASM`, will lay them all out, as in U6, because Turbo C orders a module's
  uninitialised variables by a hash of their names. Write down their sizes and types in
  the declaration.
- **Comments:** each module starts with a comment saying what it holds; each function has
  a comment saying what it does. Keep them short and factual.
- **Header files:** for now each module declares what it uses from other modules itself,
  near the top. Common headers come later.

## The modules

See `modules.txt`. Frames 0AA0-0D23 and 08E4 are a library that GAME.EXE contains too,
identical or nearly: decompiling them here serves both. Two modules match U6's OSI library
(U.EXE): 0756 is TXTJUST, 078E is TXTPRINT; the other small modules 0731-08E2 look like
the same OSI library, a newer version.

## Where it stands (2026-11-01)

All 69 modules rebuild byte-identical, code and data. The last byte (InputName's `jle` at
034C:1F48 landing on a `jmp` that was not threaded) came from the source writing the erase
code three times: at the end of the backspace branch, in the bad-character branch, and in
a final `else`. `-O` folds the copies into jumps after it has threaded the other jumps
(rule from abedegno/Exhume's Turbo C++ profile: a conditional jump onto an unshortened
`jmp` is a statement written twice that the optimiser merged).

Findings that matter for the rest:
- **Origin compiled with line numbers on** (`-y`, or the IDE's default debug information).
  Line-number records stop Turbo C++ from cross-jumping code of different source lines;
  without `-y` main.c and title.c come out shorter. The IDE (TC.EXE, from TC.CA1+TC.CA2 on
  the 1.00 disks, each part minus its first 4 bytes) and `tcc -y` give the same code.
  Other modules match with or without it.
- Library C files want `-mm -c -O -G`; asm code segments are WORD aligned, Turbo C's BYTE.
- Turbo C++ 1.00 puts the first declared local nearest BP (the reverse of Turbo C 2.0).
- `OSIDATA.ASM` and `MOUSDATA.ASM` are data-only modules (frame "-" in modules.txt).
- 0DA8/0DA9 are C0M's start-up/exit tables, made by the link.
- Uninitialised data: DS 2814-4210 (Origin's), 4210-4344 (the run-time library's).
  `tools/se/mkbss.py --start 2814 --end 4210` writes BSS.ASM.

**The full link is identical**: `python3 tools/se/sebuild.py --target savage` builds a
SAVAGE.EXE equal to the original, header and relocation table included (sha1
809c9cff8201cd5c1443589e4cf62d2dae1da2e2). What the link needed:
- `TLINK /i` (initialise all segments): the file holds the uninitialised data and the
  stack as zeros, so its minimum allocation is 0.
- `drvdata.c`, a data-only module linked first (DS:00A8-00BF: video modes, "TM.DRV",
  "SUPERTM.DRV"), used by LIB/tmdrv.c at the end of the code.
- `OSILIB/MOUSDATA.ASM` has an empty WORD-aligned code segment, as Origin's macros gave
  every module: it pads one byte after 0892. `OSILIB/OSIDATA.ASM` starts with an unused
  zero word at DS:2014 (it may as well have been 071C's).
- `LIB/LIBOPEN.C` was compiled through the assembler (`#pragma inline`): the .EXE has its
  relocations in ascending order, as an assembler writes them, not Turbo C's descending.
- `tools/se/mkbss.py --start 2814 --end 4210` writes BSS.ASM; `sesym.py apply` links the
  modules by name (it knows that C calls pascal routines without an underscore).
