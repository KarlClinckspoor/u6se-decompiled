# What the decompilation found

What the decompilation of GAME.EXE showed about Savage Empire's engine, and how it differs
from Ultima VI's. The workers wrote these down as they matched each module. Compiler lessons
are in `docs/GAME_NOTES.md`.

## Build

- Origin built both programs with Turbo C++ 1.00 and TLINK 3.0. The run-time library has
  1.01's far and near heap and `findfirst`. GAME.EXE's own code is pascal (`-p`), like
  Ultima VI's.
- 13 of the 19 game modules were compiled with line numbers (`-y`, or the IDE's debug
  information): console, map, visible, object, status, talkdr, spell, npctrack, combat,
  objcmd, egg, rest, gameloop. Without them, `-O` merges code the original keeps apart.
  main, talk, disk, music and gfx match either way.
- The OSI assembly is Ultima VI's, but newer: a far call within a segment came out as
  `PUSH CS` / near `CALL` (plus `NOP` for a forward reference). U6's has the far call.
  `INFLATE`, `DELAY2`, U6's second `RAND` routine and the sound library are gone.
- `msgs` and `monsdata` are far data compiled huge model with their class set to FAR_DATA.
- The start-up module's `_BSSEND` comes before `_BSS`, unlike SAVAGE.EXE's. The near heap
  base (`__brklvl`) is DS 3268, the start of the game's own variables. The game keeps its
  stack at the top of DGROUP (`_stklen` 0x1000, `_heaplen` 0).
- `SAVAGE.EXE` and GAME.EXE share Savage Empire's own library: EMS heap, LZW and .LZC
  files, file I/O, music and sound, timer, CPU calibration. Origin compiled it once; the two
  copies are the same code at different addresses.

## The engine compared with Ultima VI

- **Disks**: three floppies ("DISK 1-3") identified by `se.<n>`. DISK_confirm maps the
  virtual disks 100 and 101 to 3, and 102 to 4.
- **Commands** (console.c): 0x81 move, 0x82 get, 0x83 drop, 0x84 use, 0x85 talk, 0x86 look,
  0x87 attack, 0x88 rest, 0x89 combat mode. The views are 0x90 portrait, 0x91 party,
  0x92 equipment and 0x9e conversation. U6's codes from 0x8e up are unchanged. CON_putch
  has none of U6's `&`, `<` and `>` escapes.
- **Game start**: GAME_Restore talks to NPC 0x21 (Intanya) on the first run and sets the
  date to 1/1/0. GAME_Save and GAME_Load write MovePts twice.
- **Inventory** (status.c): a 4x4 grid of 16 slots that never scrolls. Ready has no ring
  case and refuses modern weapons to some characters ("can't use modern weapons!").
  Torches are recognised by shape 0xd0 (unlit) and 0xd1 (lit). D_E668 is used as
  `Equipment[8]` (the two-handed slot) in combat.c, though status.c names it WeightEquip.
- **Objects**: monster object slots are 0xC0-0xFF (U6: 0xE0-0xFF). Monster classes come
  from `MonsterTypes` (47 types, class 0 unused). The per-class tables hold strength,
  dexterity, intelligence, armour, damage, hit points, alignment and flags; there are no
  starting-inventory lists. EGG_GivePossessions has fixed items for types 0x140 and 0x13C.
- **Conversations** (talkdr.c): the new opcode 0xd1 (OP_EVENT, TALK_Event) runs scripted
  events: creatures spawning around the party, NPC 0x19 moving, the end of the game and
  exit to DOS. JoinParty allows at most 5 members besides NPCs 2, 5 and 0x0b. Names of NPCs
  0x40 and up get an article ("a ...").
- **Magic** (spell.c): no spellbook. "Invoke:" takes Heluzz, Aphazz or Motazz with the
  offerings 0x37, 0x36 and 0x38. Spells are peer, light, sense danger ("Triolo senses ..."),
  help (summons an ally), heal, charm, protect, curse and wild.
- **Rest** (rest.c): levelling up happens in a dream: a bear, a jaguar and a monkey ask
  questions (REST_LevelUp).
- **Combat** (combat.c): boulders (type 0xA7) can be pushed; objects 0x89/0x8A and 0xF0/0xF1
  explode; destroying 0x107 crushes the actors in an area; there are segmented creatures
  (0x124/0x11A) and growing tangle vines. Monster flags seen: 0x20 attacks with STR,
  0x40 poisonous, 0x800 and 0x1000 leave no body (0x1000 also flashes), 0x2000 bleeds,
  0x4000 "attacks!".
- **Party death** (map.c, music.c): the party wakes in Intanya's hut at (0x19c, 0x123), seven
  days later. Entrances (types 0x14, 0xa3, 0x16, 0xe3, 0x103, 0xa0) and type-0x44 travel
  objects use destination tables.
- **Use handlers** (objcmd.c): tar, clay, grinder, sieve, loom, gunpowder and flash powder,
  camera, rope, drums, bucket of water, turtle bait and riding the turtle, the statue
  heads, Jimmy's notes, the Yunapotli doors, and the machine. Cut scenes 1-4 and the last
  one, which saves the game and runs SAVAGE.EXE, are at the start of the module.
- **View** (visible.c, gfx.c): no X-ray view. The light radius is `level*2 - 1`. Portraits
  have backgrounds by place and time of day (`bkgrnd.lzc`). The MIDGAMEn.LZC pictures come
  with text pages. Wall variants are based at 0x120 or 0x110, with no corner tiles.
- **Leftovers from Ultima VI**: still in the data but unused, or barely used. The
  moonstone and moon variables of the game-state block (same values as U6's), ship wind
  and raft drift costs, the CyclopsFlag, the spread-direction table, "Winona Ryder" as the
  scratch buffer for names (talk.c, rest.c), and a stub OBJ_getHead.
