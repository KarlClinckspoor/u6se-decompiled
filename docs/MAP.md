# Savage Empire executables: map

The GOG release's `SAVAGE.EXE` and `GAME.EXE`. All addresses are in the load image
(header removed): code as `FRAME:OFFSET` (the paragraph a far call uses), data as DS offsets.
`tools/se/` reproduces every table here (`segmap.py`, `modmap.py`, `libscan.py`, `dsrefs.py`).

## Toolchain

Both were built with **Turbo C++ 1.00** (May 1990) and **TLINK 3.0**:

- `C0M.OBJ` of 1.00 is the first 0242h bytes of both, byte for byte (fixups aside).
- The run-time library is 1.00's `CM.LIB`, except `FARHEAP`, `NEARHEAP` and `FINDFIRS`, which
  are 1.01's and sit at the end of the library (they are linked last): Origin replaced them
  with `tlib -+`. `tools/se/setup.py` builds that library as `CMSE.LIB`.
- Medium model (far code, near data), 8086 instructions. Origin's own code is compiled with
  `-O`; `SAVAGE.EXE`'s program is cdecl, `GAME.EXE`'s pascal (`-p`), as in Ultima VI.
- Turbo C++ code segments are word aligned (TLINK pads with a zero byte).
- `INSTALL.EXE` is Turbo C++ 1.00 too, with the unpatched heap.

## SAVAGE.EXE

76,400 bytes; 699 relocations; header 0C00h bytes; entry 0000:0000; DGROUP 0DAA;
stack 11DF:0080. Code 55,968 bytes.

| Frames | Bytes | What | Status |
|---|---|---|---|
| 0000 | 13,515 | C0M (0000-0241) + 78 CMSE.LIB modules (0242-34CA), in library order | linked from Turbo C++ |
| 034C | 11,928 | the main program | being decompiled |
| 0636, 071C | 4,025 | program modules (character creation, endgame?) | being decompiled |
| 0731-08E2 (except 07D9) | ~4,500 | the OSI library, a newer version of U.EXE's (0756 = TXTJUST, 078E = TXTPRINT) | being decompiled |
| 07D9 | 2,310 | program module | being decompiled |
| 08E4 | 7,091 | library module, 97% like GAME.EXE 2B5C | being decompiled |
| 0AA0-0D23 | ~10,000 | Savage Empire's library, identical in GAME.EXE (2D17-2F30): files, .LZC, EMS, music, drivers | being decompiled |
| 0D28-0DA9 | ~2,100 | last modules (driver loading, TM.DRV/SUPERTM.DRV) | being decompiled |

The per-module table, with first bytes and lengths, is `SRC_S/modules.txt` (69 modules).

Data segment (DGROUP 0DAA):

| DS | What |
|---|---|
| 0000-00A7 | C0M's data |
| 00A8-~1FFF | the program's data: driver names, Intanya's questions, intro and endgame texts |
| ~2003-201A | title screen data (`TITLE.LZC`) |
| 201A-20D0 | the OSI library's data |
| 23B8-2510 | Savage Empire's library data (`strax.drv`, `MThd`, `EMMXXXX0`) |
| ~2510-27FD | CMSE.LIB's data (`COMPAQ`, `PATH`, `(null)`) |
| 27FE-4350 | uninitialised data (all zeros in the file) |
| (11DF:0000) | the 80h-byte start-up stack segment |

## GAME.EXE

270,016 bytes; 3,240 relocations; header 3400h bytes; entry 0000:0000; DGROUP 2FA6;
stack 3EA4:0080. Code 195,168 bytes. The same layout as Ultima VI's GAME.EXE: the modules
have U6's names (U6 frame = U6 image frame + 0838).

| Frame | Bytes | Module (U6 name) | Similar to U6 |
|---|---|---|---|
| 0000 | 9,861 | C0M + Origin's OSI file assembly (0242-0662, in `_TEXT`) + 59 CMSE.LIB modules | |
| 0268 | 3,856 | main | 0.65 |
| 0359 | 10,096 | gameloop | 0.43 |
| 05D0 | 12,208 | console | 0.72 |
| 08CB | 4,336 | map | 0.64 |
| 09DA | 2,096 | visible | 0.71 |
| 0A5D | 15,200 | object | 0.70 |
| 0E13 | 5,728 | status | 0.77 |
| 0F79 | 320 | talk | 0.47 |
| 0F8D | 10,016 | talkdr | 0.71 |
| 11FF | 4,352 | spell | 0.58 |
| 130F | 20,496 | npctrack | 0.54 |
| 1810 | 23,248 | combat | 0.53 |
| 1DBD | 35,488 | objcmd | 0.52 |
| 2667 | 2,464 | egg | 0.33 |
| 2701 | 1,696 | music | 0.37 |
| 276B | 9,824 | gfx | 0.67 |
| 29D1 | 112 | kbd (asm) | 1.00 |
| 29D8 | 2,704 | rest | 0.50 |
| 2A81 | 992 | disk | 0.86 |
| 2ABF-2B57 | ~2,500 | OSI assembly (U6's OSILIB modules) | 0.97 |
| 2B5C | 7,090 | library (= SAVAGE 08E4, 97%) | new |
| 2D17-2F30 | ~10,000 | Savage Empire's library (= SAVAGE 0AA0-0D11) | new |
| 2F43, 2F65 | 1,552 | far data: messages, monster tables (U6's `msgs`, `monsdata`) | |
| 2FA4, 2FA5 | 32 | two 16-byte segments (also in SAVAGE.EXE as 0DA8, 0DA9) | |

The shared library, SAVAGE frame = GAME frame: 0AA0=2D17, 0AB2=2D29, 0AD2=2D49, 0AE3=2D5A,
0AF9=2D70, 0BB1=2E29, 0BC5=2E3C, 0BE6=2E5D, 0C02=2E79, 0C06=2E7E, 0C1C=2E89, 0C1F=2E8C,
0C27=2E94, 0C3F=2EAC, 0C49=2EB7, 0C58=2EC5, 0CAC=2F04, 0CED=2F0D, 0D09=2F29, 0D0C=2F2B,
0D0E=2F2D, 0D11=2F30 (identical code); 08E4~2B5C, 0D23~2B57, 0C97~2F30 (similar).

Data segment (DGROUP 2FA6): C0M's data 0000-00A7, the game's data, the OSI and Savage
Empire library data (`EMMXXXX0` at 2ED4), CMSE.LIB's data (`PATH` at 3242), the
uninitialised data from about 3250 (the file holds it as zeros up to the stack segment,
F060).
