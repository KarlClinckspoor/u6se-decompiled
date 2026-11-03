# Decompiling The Savage Empire: what we learned

The Savage Empire (Origin, 1990) runs on the Ultima VI engine. Its two executables,
`SAVAGE.EXE` and `GAME.EXE`, are now C and assembly that rebuild them byte for byte. This
writeup covers what the work taught: how the compiler was identified, what Turbo C++ 1.00
does with source code, what the link needed, and how the result compares with Ultima VI,
both the code and the decompilation.

Shorter references: `docs/FINDINGS.md` lists the engine differences point by point,
`docs/GAME_NOTES.md` the compiler notes as the workers wrote them, and `docs/MAP.md` both
executables. `SRC_S/DECOMP.md` and `SRC/DECOMP.md` give each program's rules and status.

## The result

| Program | What it is | Code | Modules | Rebuilt by |
|---|---|---|---|---|
| `SAVAGE.EXE` | launcher, title, intro, character creation (Intanya's questions), endgame | 56 KB | 70 | `sebuild.py --target savage` |
| `GAME.EXE` | the game | 195 KB | 53 | `sebuild.py --target game` |

Both rebuild equal to the GOG release, file header and relocation table included: 699
relocations for SAVAGE.EXE and 3,240 for GAME.EXE. `INSTALL.EXE` is set aside. It is
Turbo C++ 1.00 too, with the unpatched heap.

It took two days. SAVAGE.EXE came first and served as the calibration run for the
toolchain. GAME.EXE was split among parallel workers, one or two modules each, all
following the same written rules. A worker's module was done when `secmp.py` reported
identical code and data; the full link came last.

## Finding the compiler

Ultima VI was built with Turbo C 2.0. Savage Empire came out a few months later and was
not. Three things settled it:

- **The start-up code.** The first 0242h bytes of both programs are Turbo C++ 1.00's
  `C0M.OBJ`, byte for byte apart from fixups.
- **The library.** `libscan.py` finds the run-time library modules in the image. They are
  1.00's `CM.LIB` except `FARHEAP`, `NEARHEAP` and `FINDFIRS`, which are 1.01's and sit at the
  end of the library. Origin had replaced them with `tlib -+`, which appends.
  `tools/se/setup.py` rebuilds that library as `CMSE.LIB`.
- **The code itself.** Turbo C++ 1.00 always pushes a BP frame and reuses AX after
  `mov si,ax`. Turbo C 2.0 omits the frame in leaf functions and does not reuse AX that way.
  1.00 also puts the first declared local nearest BP, the reverse of 2.0.

Origin's hand-written assembly was TASM. MASM 3.0 (`/Mx`) assembles it to the same bytes
with a few spellings: `OFFSET DGROUP:x`, `FAR PTR` and `SHORT` for forward references, and
`db` for the instructions MASM encodes differently. One of those cost the U6 decompilation
its last byte: TASM writes `AND r/m,imm8` with the sign-extended opcode (83 /4), which MASM
3.0 never does. Working out why SAVAGE.EXE had one stray byte found the same thing in
Ultima VI's `OSI_FILE.ASM` and `MOUSE.ASM`. With that fixed, U6's GAME.EXE now rebuilds
identical too.

## What Turbo C++ 1.00 taught us

The compiler is deterministic, but it decides many things the source does not state
directly. Most of a worker's time went into finding the source form that the original
came from. The rules below held across modules. `docs/GAME_NOTES.md` has the full list,
with addresses.

### Line numbers stop cross-jumping

Origin compiled many modules with line numbers on: `-y`, or the IDE's default debug
information. The IDE (`TC.EXE`) and `tcc -y` give the same code. With line numbers, `-O`
does not merge identical code that comes from different source lines. Without them,
`main.c` and `title.c` of SAVAGE.EXE came out shorter than the original, as did 13 of
GAME.EXE's 19 game modules.

So a difference in length was the first test: if the compiled module is too short, add
`-y`. Under `-y` the merges that remain are clues to the source:

- **A conditional jump onto a `jmp` that was never threaded** means the same statement
  was written twice and `-O` merged the copies late. abedegno's Exhume project states this
  rule in its Turbo C++ profile. SAVAGE.EXE's last differing byte came from it: `InputName`
  writes its erase code three times, once in each branch.
- **`jmp short $+2` (EB 00)** comes from a statement the compiler drops: `else { x = x; }`.
  A `cmp` with no jump after it is `if(...) ;`.
- **A statement on the same line as its `if`** has no line record of its own, so `-O`
  still merges it. Statements meant to stay apart go on their own lines.
- **A `switch` whose `default:` has no `break;`** can block all tail merging of its cases.
  Ending a function with `return;` keeps the last branch from merging with earlier ones.
- **Tail merging still works across identical instruction runs** that end in jumps to the
  same place. A label in between, from the end of a nested `if` or of a block that
  declares locals, stops it.

### Register variables are chosen, not declared

Turbo C++ 1.00 picks register variables itself. The `register` keyword matters only
sometimes.

- The most-used variable gets SI, the next DI. Use as an array index counts for more. On a
  tie, the variable declared later wins SI.
- An explicit `register` beats usage counts. With two `register` locals, the second
  declared gets SI.
- A local whose low byte is stored (`A[k] = v;`) stays off SI and DI even when declared
  `register`. The chained form `A[k] = v = B[i];` does get DI.
- Functions that make no calls also use CX for a third variable, and DX when SI and DI
  are taken.
- To get DI for the first parameter and SI for the second, declare the second as a
  register local with an initialiser: `register int16 d = dir;`.

When SI and DI came out swapped, swapping the declarations usually fixed it.

### How a condition is written shows in the code

- `!(uchar & k)` compiles to `mov al / mov ah,0 / test ax,k`, in any position. `(uchar & k) == 0`
  and a plain `if(uchar & k)` give `test byte`. A positive test in the `mov al` form needs
  `!!x`.
- `return cond;` compiles to `jcc / mov ax,1 / jmp / xor ax,ax`, which `-O` then merges
  with other returns. An `if/else return 1/0` gives different code.
- `if(o == self) continue; if(t == X ...)` reloads `t` from memory. `if(o != self && t == X ...)`
  reuses AX. The end of an `if` statement resets what the compiler knows about registers
  and ES.
- `for(x = f(); x >= 0; x = f())` with identical start and step compares memory
  (`cmp word [bp-x],0`). `while((x = f()) >= 0)` gives `or ax,ax`.
- `d*3` gives `imul`. Origin wrote `d + (d << 1)`, and `frm + (d << 1) + d + 1` must be
  written in that order.

### Layout and calls

- **Data.** A module's `_DATA` holds its initialised variables in source order, then its
  string literals in order of use. Initialisers of local arrays and static locals also sit
  there in source order. With `-d`, a string can share the tail of an earlier one: "east"
  inside "northeast", "\n" at the end of "%3d\n".
- **Uninitialised data.** Turbo C++ orders a module's `_BSS` by a hash of the names. So
  the modules only declare their uninitialised variables `extern`, and one generated file,
  `BSS.ASM`, lays them out at their addresses, as in the U6 decompilation.
- **Calls within a module.** A function called before its definition gets a 9A far call.
  One defined earlier gets `push cs / call near`. So a big module cannot be compiled in
  parts and still match; the objcmd worker found this out and compared function by
  function within one file instead.
- **Pascal names.** With `-p`, variables become pascal too: upper case, no underscore. A
  variable defined in cdecl code, such as the shared library's `_soundCard`, has to be
  declared `cdecl` in a game module, or the link fails. `secmp.py` now reports it.
- **Segment alignment.** C code segments are byte aligned, assembly segments word aligned.
- **Relocation order.** A C file compiled directly lists its relocations in descending
  order. One compiled through the assembler (`#pragma inline`) lists them ascending.
  `LIBOPEN.C` needed that.

## What the link needed

Each module matched on its own long before the programs did. The full links showed what
the modules could not:

- **`TLINK /i`** (initialise all segments). The files hold the uninitialised data and the
  stack as zeros, which is why their minimum allocation is 0.
- **Data-only modules.** Some modules have data but no code: `drvdata.c`, `OSIDATA.ASM`
  and `MOUSDATA.ASM` in SAVAGE.EXE, and `gamestat.c` and `PARTYTOP.ASM` (U6's
  `SEG_155X.ASM`) in GAME.EXE. They only showed up as unexplained bytes between their
  neighbours' data. MOUSDATA's empty word-aligned code segment adds one byte of padding.
- **Module boundaries move by a few bytes.** A module's last switch table can run into
  the next frame, so the next module starts at :3, :4 or :9, not :0. The automatic module
  map had five such starts wrong in GAME.EXE.
- **Far data.** GAME.EXE's messages and monster tables live in their own segments, after
  the code and before the start-up tables. They match as huge-model C with the data class
  renamed `FAR_DATA` (`-mh -zTFAR_DATA`). Declaring the arrays `far` instead puts every
  array in a paragraph-aligned segment of its own.
- **The start-up segments.** In GAME.EXE, C0M's `_BSSEND` comes before `_BSS`, so its
  bdata@ and edata@ are both DS 3268. The start-up code clears no uninitialised data, and
  the near heap would start in the middle of the game's variables. The game puts its
  stack at the top of DGROUP. `SEGORDER.ASM`, linked before C0M, reproduces this by naming
  the segments in that order. SAVAGE.EXE has the normal order. How Origin got it is not
  known.
- **MASM reads `ORG BC4Ah` as a symbol.** A hex number must start with a digit, so
  `mkbss.py` writes `0BC4Ah`. Without the leading 0, `BSS.ASM` silently came out 0x2D36
  bytes short.

## Working with Ultima VI's source

GAME.EXE is Ultima VI's engine, changed. `u6match.py` paired every GAME.EXE function with
the closest U6 function by its instruction sequence:

| Closeness to U6 | Functions | Code |
|---|---|---|
| 0.85 or more (U6's function with small changes) | 71 | 5.8 KB |
| 0.5 to 0.85 (rewritten but recognisable) | 223 | 62 KB |
| below 0.5, or no counterpart | 166 | 97 KB |

So about 40% of the code started from Ultima VI's readable source, and most of the rest
was still easier with it at hand. The modules come in U6's order and keep U6's names
where the function does the same job. Each GAME.EXE module was rewritten from its U6
counterpart, changed until it compiled to Savage Empire's bytes.

What did not carry over:

- **Addresses.** The variables all moved, so each module declares what it uses itself, by
  address name or by the defining module's name. `sesym.py apply` links them by name at
  the end. Common headers are left for a later pass.
- **Object and tile numbers.** U6's `obj.h`, `tile.h` and `spells.h` constants mean
  nothing in Savage Empire. The sources write the numbers, with a comment when the
  meaning is known.
- **Several assumptions in the map.** Functions hidden behind switch tables were missed
  three times (gfx, gameloop, objcmd), and two U6 matches were wrong (`OBJ_GetCount`, not
  `Encumbrance`; `GiveObj`).

The OSI assembly (file I/O, keyboard, mouse, drives, delays, random numbers) is Ultima VI's
and matched with few changes. TASM wrote far calls within a segment as `PUSH CS` / near
`CALL`, plus a `NOP` for a forward reference. The copies in U6's GAME.EXE keep the far
call, so Origin's TASM or its options had changed. `INFLATE`, `DELAY2`, U6's second random
number routine and the sound library are gone. Savage Empire has its own library instead,
shared by both programs: EMS heap, LZW and `.LZC` files, file I/O, MIDI music, timer, CPU
calibration. It was decompiled once, in `SRC_S/LIB/`, and GAME.EXE builds it from there. Its
uninitialised data are at other addresses, which `shared.py` pairs from the two
executables.

## How Savage Empire differs from Ultima VI

`docs/FINDINGS.md` has the list. In short:

| Area | Ultima VI | Savage Empire |
|---|---|---|
| Disks | 8 floppies, "U6.<n>" | 3 floppies, "se.<n>" |
| Inventory | scrolling list | fixed 4x4 grid of 16 slots |
| Magic | spellbook, reagents | "Invoke:" with Heluzz, Aphazz or Motazz and an offering; peer, light, sense danger, help, heal, charm, protect, curse |
| Levelling up | meditating at a shrine (a conversation script raises the level) | a dream on resting: a bear, a jaguar and a monkey ask questions |
| Conversations | U6 script interpreter | the same, plus opcode 0xd1 for scripted events (spawns, end of game) |
| Death | Lord British's resurrection | the party wakes in Intanya's hut, seven days later |
| Monsters | 64 classes, starting inventories | 47 classes, no inventory lists; object slots 0xC0-0xFF |
| Objects used | U6's | tar, clay, grinder, loom, gunpowder, camera, flash powder, rope, drums, the turtle, statue heads, and others |
| Weapons | | modern weapons that some characters refuse |
| View | X-ray view | none; light radius `level*2 - 1` |

Ultima VI leftovers are still in the data and mostly unused: the moonstone and moon
variables of the saved game-state block, ship and raft costs, the cyclops flag. Two
modules use "Winona Ryder" as the writable buffer for a name.

## How the work was organised

- **One check per module, run all the time.** `secmp.py` compiles a file in DOSBox and
  compares it with the original at its address. It checks every fixup by name or address,
  not only the bytes, and prints the first real difference with both disassemblies.
  `secheck.py` runs it over every module in `modules.txt`.
- **Written rules, shared notes.** Workers started from a common brief: `docs/GAME_BRIEF.md`
  and `SRC/DECOMP.md`. Lessons went into `docs/GAME_NOTES.md` as each module finished, and
  later workers began from them. The register and condition rules above came out of that
  file.
- **Each worker kept to its own files.** Workers did not edit the tools or other modules;
  they reported tool bugs and module boundaries instead. Several improvements came back
  that way: value-search switch tables in `secmp.py`, function-pointer variables in
  `sesym.py`, and per-function comparers that list every difference in one compile.
- **Names last.** Modules refer to each other by address (`C_SSSS_OOOO`, `D_XXXX`) while
  they are being matched. `sesym.py apply` turns those into the names the defining modules
  gave, once everything matches. Naming early would have tied the workers to each other.
- **The link last.** The link only works once every module is there. Its problems (data
  boundaries, segment order, link names) are global, and they went quickest with all the
  modules done.
