# The Savage Empire: game mechanics

How *The Savage Empire* (Origin, 1990) plays, as its code implements it. Everything here
was read from the byte-identical decompilation of `GAME.EXE` in `SRC/`, and each rule
cites the function that implements it: `Name` and its address `C_SSSS_OOOO` (segment and
offset in GAME.EXE, see `SRC/modules.txt`). Where the code has a bug, the rule is given as
the code runs, with the bug noted. Where something could not be established, each section
says so at its end.

The game runs on Ultima VI's engine; `docs/FINDINGS.md` and `docs/WRITEUP.md` cover how it
differs. Object and NPC names are scarce: GAME.EXE holds no name table, and the game's
data files are not part of this repository. Numbers are hexadecimal where they are object,
tile or NPC numbers (0x..), decimal otherwise.

## At a glance

Some of what the code does is not what a player would expect:

- **Attacks never miss.** The to-hit roll is computed but ignored (`__TryHit` C_1810_337D
  tests the result against -1, and it is 0 or 1). Only armour that absorbs the whole blow
  ("grazed") or a missile stopped on the way acts like a miss.
- **Experience comes only from killing hostile creatures with ordinary hits**, worked out
  from the creature's class tables. Explosions, boulders, poison and terrain give none.
  Monsters' armour values count only for experience: a monster is protected only by
  armour objects it carries.
- **Levels rise only in the dream while resting**, one level per rest, at most level 8
  (experience 0, 100, 200, 400, 800, 1600, 3200, 6400). The bear, jaguar and monkey
  answers decide which attribute grows.
- **Hit points do not come back with time.** They return by healing, by the meal on
  resting (+5, at most once in 12 hours), and when the party wakes after being defeated.
- **There is no hunger**, and **poison never wears off** by itself.
- **Defeat is not the end**: the party wakes in Intanya's hut seven days later, with a
  quarter of its experience lost.
- **The story is kept in the talk flags**: 16 flags per NPC, set and tested by the
  conversation scripts and by a few engine routines (cut scenes, the drums, combat).
- **Every step of the leader passes a minute**, besides the minute per round of NPC turns.
  A day is 24 hours, a month 28 days, a year 12 months.

## Conversations and quest progression

Savage Empire keeps Ultima VI's conversation engine: each NPC has a compiled script in
`talk.lzc`, run by a small interpreter (`talkdr.c`). The story itself is tracked almost
entirely with the per-NPC **talk flags**, which scripts set and test, and which a handful
of engine routines (cut scenes, object handlers, combat) also read and write. This section
describes both, as the code implements them.

Notation: NPC numbers are object numbers (hex). "Flag *n* of NPC *x*" means talk flag
*n* (0-15) of that NPC, as scripts address it with `OP_SET`/`OP_CLR`/`OP_TST`.
`TalkFlags[x] & 0x80` is flag 7 of NPC *x*.

### Starting a conversation

A conversation starts in one of these ways:

| How | Code | Notes |
|---|---|---|
| The Talk command (`T`, or the talk mouse command) and a target | `GameLoop` C_0359_1A65 → `TALK_talkTo(Active, SelectionObj, 1)` | The keyboard cursor for Talk has `SelectRange = 7` (tiles from the active member). |
| An NPC whose AI mode is 0x96 ("converse") comes to the party | `NPC_TakeTurn` C_130F_4121, `TalkToParty` C_130F_4079 | When the party is within 4 tiles of the NPC's schedule spot (`CLOSE_ENOUGH0_S(5, ...)`), the NPC walks to the nearest visible member; once adjacent its mode becomes 0x90 and it talks. Otherwise it loiters near its spot. Scripts can set mode 0x96 with `OP_SETMODE`. |
| First run: NPC 0x21 (Intanya) | `GameLoop` C_0359_1A65, `GAME_Restore` C_05D0_03A2 | See "The first conversation". |
| A flash near NPC 0x19 | `Flash` C_1DBD_7204 | See "Story events in the engine". |
| Opening the door at (0x184, 0x235) | `UseDoor` C_1DBD_7580 | NPC 4 joins and talks. |

`TALK_talkTo` (C_0F79_00E0) accepts only targets 0-255 (actors); anything else prints
"nothing!". With `showName` set it first prints the target's name if flag 0 of the target
is set ("already met"), otherwise its description string. It does not check distance; the
selection cursor does that. The party-index argument is ignored: `TALK_initTalk`
(C_0F79_000E) always passes speaker 1 (the Avatar) to `TalkDriver`.

`TalkDriver` (C_0F8D_20C8) then refuses the conversation, in this order:

| Check | Message |
|---|---|
| Target is player-controlled (a party member, `NPCStatus & 0x80`) and not within the 11x9 tiles around the party (`C_0A5D_0613`: \|dx\| ≤ 5, \|dy\| ≤ 4, no level check) | "No response." |
| Solo mode with a member other than party slot 0 (`SoloMember != -1 && SoloMember != 0`), unless the target is NPC 0x19 | "Not in solo mode!" |
| Target dead (`0x10`) or asleep (`0x04`), or its AI mode is 0x9b, 0x13 or 7 | "No response." |
| `IsArmageddon` set (a U6 leftover: only ever cleared, in `GAME_Restore`) | "No response." |
| Target is the speaker, i.e. NPC 1 | "Talking to yourself?" |
| Target is object 0, or has no script | "No response." |

Before the last check the active member turns to face the pointer.

**Which script an actor uses** (`LoadConversation` C_276B_1BFC, `HasConversation`
C_276B_1CC5; scripts are entries of `talk.lzc` on disk 4):

| Actor | `talk.lzc` entry | Name shown |
|---|---|---|
| 2 - 0x3F | actor − 2 | from the script |
| 0x40 - 0x97 | ((actor − 0x40) >> 3) + 0x3E: eight actors share one script; `#N` tells them apart | "a " + name from the script |
| 0x98 - 0xBF | none | |
| 0xC0 - 0xFF (temporary creatures) of type 0x140 / 0x13C / 0x13D / 0x123 | 0x49 / 0x4A / 0x4B / 0x4C | portrait label "guard" / "guard" / "parrot" / "wisp" (`ShowPortrait` C_1DBD_0D8F) |
| other temporary creatures | none | |

An entry that is empty (size ≤ 0) means "no script". An actor's name is the text between
the script's NPC-number byte and the first byte ≥ 0x80 (`TALK_GetNpcName` C_0F8D_0094).
Party members are named from the party name table instead (`TALK_GetCharName`
C_0F8D_011C). The portrait panel (`ShowPortrait`, also used by Look) labels an NPC below 0x98 with its
name only once its flag 0 is set; before that it uses the description string.

### The conversation, step by step

`TalkDriver` (C_0F8D_20C8):

1. Loads the script, copies the NPC's name into `$N`, shows the portrait, opens the large
   text window (`TextWindows[2]`) with status view 0x9E.
2. Prints "You see " and the description (the text after `OP_DESC`). It remembers where the
   description starts, for "look".
3. Skips to `OP_MAIN` and runs the statements there, unless the first opcode is
   `OP_ASKTOP`.
4. Loops: reads an input opcode, gets the player's input, sets `$Z` to it, then runs the
   statements that follow (normally the keyword blocks). Typing **look** (any case) at an
   input prompt instead reprints "You see" and the description and asks again; no keyword
   block runs.
5. Stops when a statement executes `OP_LEAVE`, when it reaches `OP_SCRIPT_MARK` (the end of
   the script), or on a script error ("Command for input key expected.", "Logical error
   occured.", a malformed `OP_GETSTR`/`OP_GETINT`/`OP_GETDIGIT`).
6. Prints "Press any key...", waits, and restores the screen. If the party member shown in
   the status area has left the party, the status area returns to the party list.

The conversation passes no game time.

### Script format

A script is a byte stream. Bytes below 0x80 are text; bytes 0x80 and up are opcodes.
Layout:

```
FF <npc number> <name> F1 <description...> F2 <main statements...>
F7 (or another input opcode)
EF "kw1,kw2,..." F6 <answer statements> EE
EF "..." F6 ... EE
...                         (usually a jump back to the input opcode)
```

#### Text

Text bytes are printed through `CON_putch` (C_05D0_2B58), so all the in-text escapes work
in scripts:

| Escape | Effect |
|---|---|
| `#c` | prints integer variable `#c` (c = 0-9, A-Z) |
| `$c` | prints string variable `$c` |
| `/` | the letters that follow print only if the last printed number was 1 |
| `\` | the letters that follow print only if it was not 1 (both end at the next non-letter) |
| `^c` | prints control character `c & 0x1F` (special glyphs) |
| `@` | the next word is a keyword: red when help is on (Ctrl+H) |
| `*` | waits for a key |
| `` ` `` | capitalises the next character |
| `%` | cancels a pending escape |

A text byte below 0x20 other than newline prints "Try to print out a control char" and
ends the statement run (`parse_statement` C_0F8D_1EEC).

`parse_statement` executes statements until it meets byte 0, `OP_ENDRES`, `OP_GET`, or
any opcode ≥ 0xF0 (`OP_DESC`, `OP_MAIN`, `OP_RES`, the input opcodes, `OP_SCRIPT_MARK`),
or until `OP_LEAVE` has run.

#### Statements

Arguments marked *expr* are each one expression ending with `OP_END_OF_FACTOR` (0xA7).
"npc" arguments accept `OP_NPC` (0xEB), which stands for the NPC being talked to
(`mk_npcnum` C_0F8D_007E). Statements are run by `execute_op` (C_0F8D_18A3).

| Opcode | Name | Arguments | Effect |
|---|---|---|---|
| 0xA1 | IF | expr | False (0): skips to the next `ELSE` or `ENDIF` byte. No nesting: the skip stops at the first one found. While skipping it steps over the operands of `GOTO` (4 bytes), `BYTE` (1) and `WORD` (2), but not of `ADDRESS`. |
| 0xA3 | ELSE | | Skips to the next `ENDIF` (same rules). |
| 0xA2 | ENDIF | | Nothing. |
| 0xA4 | SET | npc, bit | Sets talk flag *bit* (0-15) of npc. Bits > 15: nothing. |
| 0xA5 | CLR | npc, bit | Clears it. |
| 0xA6 | LET | destination, `OP_LET_VALUE`, value | `<i> B2` (`#i`) or an array element (`OP_ADDRESS` ...) takes an expression; `<i> B3` (`$i`) takes a constant string (`OP_ADDRESS` ...) or another `<j> B3`, copied into `$i`'s buffer. |
| 0xB0 | GOTO | 4-byte script offset | Jumps. |
| 0xB5 | PRINTSTR | `OP_ADDRESS` string, or `<i> B3 <byte>`, or 0xD5 | Prints a constant string of the script, or `$i`, or nothing. |
| 0xB6 | LEAVE | | Ends the conversation. |
| 0xB9 | GIVEOBJ | npc, type, quality, quantity | Creates the object in npc's inventory (`GiveObj`); `#T` = 1 if that worked, else 0. |
| 0xBA | TAKEOBJ | npc, type, quality, quantity | Removes it (`TakeObj`). |
| 0xBF | SHOW_CONVERSE | npc | Shows npc's portrait. |
| 0xC8 | MOVEOBJ | object, owner | Puts the object into owner's inventory. (`OP_NPC` is translated for the first argument only.) |
| 0xC9 | TRANSFEROBJ | type, quality, from npc, to npc | Moves the first such object of *from*'s inventory to *to*'s. A missing object is not checked for. |
| 0xCB | WAIT | | Waits for a key. |
| 0xCD | SETMODE | npc, mode | Sets the NPC's AI mode. Mode 0x91 also puts it to sleep. |
| 0xD0 | DELAY | n | Runs n × 20 animation ticks. |
| 0xD1 | EVENT | n | Runs scripted event *n* (`TALK_Event`, below). New in Savage Empire. |
| 0xD6 | RESURRECT | npc | Revives the first dead body (type 0xF9) in npc's inventory (`SPELL_ReviveNPC`). `#R` = the revived NPC and `$Y` = its name, or `#R` = −1. |
| 0xD8 | SETNAME_Y | npc | `$Y` = npc's name. |
| 0xD9 | HEAL | npc | Hit points to maximum. |
| 0xDB | CURE | npc | Clears poison. |
| 0xDF | SETNAME_Y_YOU | npc | `$Y` = npc's name, or "you" for NPC 1. |
| 0x9C, 0x9E, 0xBE, 0xCF, 0xA7 | | | Do nothing (0x9C was U6's "get horse", 0x9E "rest"). |
| others ≥ 0x80 | | | Print "Unknown command." |

Errors returned by `execute_op` (a bad `PRINTSTR`/`LET` form, an `IF` whose value is −1)
are never checked. A side effect: an `IF` whose expression equals −1 (0xFFFF) does not skip,
so it behaves as true.

#### Expressions and the value stack

Expressions are postfix (`parse_factor` C_0F8D_0543). The stack is a local array of ten
32-bit values, with no overflow check. `OP_BYTE` (0xD3) pushes the next byte, `OP_WORD`
(0xD4) the next 16-bit word, `OP_ADDRESS` (0xD2) the next 32-bit value (a script offset).
Any other byte that is not an operator is pushed as its own value, so 0x00-0x7F are small
constants and `OP_NPC` is pushed as 0xEB. The expression ends at `OP_END_OF_FACTOR` or
`OP_LET_VALUE`; the result is the bottom stack entry, cut to 16 bits. If more than one
entry is left the game prints "Error occured.".

Operators pop their operands (listed in push order) and push one result:

| Opcode | Operands | Result |
|---|---|---|
| 0x81 > , 0x82 >= , 0x83 < , 0x84 <= | a, b | 1/0, compared as unsigned 32-bit |
| 0x85 != | a, b | 1/0 |
| 0x86 == | a, b | 1/0; once the expression has used a string variable, compares the two as strings, case-insensitive |
| 0x90 + , 0x91 − , 0x92 × | a, b | a+b, a−b, a×b |
| 0x93 ÷ | a, b | a ÷ b unsigned; b = 0 prints "Divide error from talkdr." and leaves a |
| 0x94 OR , 0x95 AND | a, b | logical, 1/0 |
| 0xA0 RND | min, max | random number in [min, max] (8-bit values) |
| 0xAB TST | npc, bit | talk flag *bit* (0-15) of npc, 1/0. For bit > 15 the result is the NPC number (nothing is computed). |
| 0x9A CANCARRY | npc | STR × 200 − (equipment + inventory weight) × 10, at least 0 |
| 0x9B WEIGHT | type, quantity | quantity × type weight, × 10 unless the type is weighed per ten |
| 0x9D HORSED | npc | always 0 |
| 0x9F OWNS | npc, type, quality | 1 if npc carries such an object (containers included) |
| 0xB2 VARINT | i | `#i` |
| 0xB3 VARSTR | i | pointer to `$i` |
| 0xB4 DATA | array, index | the 16-bit word at array + 2×index |
| 0xB7 STRSEARCH | string array, i | index of the first entry whose first min(4, length) characters match a word of `$i` (as keywords match); the number of entries if none. The list ends with 0xB8. |
| 0xC3 VALSEARCH | word array, i | index of the entry equal to `#i`, or 0x8001. The list ends with 0xB8. |
| 0xBB TEST_OBJ | npc, type | `TestObj(npc, type)` |
| 0xBC, 0xBD | a, b | always 100 |
| 0xC1 OWNER | object | the object's owner field. Also rewrites the next script byte in place through `mk_npcnum`. |
| 0xC2 OBJTYPE | object | its type |
| 0xC6 ISINPARTY | npc | 1 if in the party, on screen, and not dead, asleep or paralysed |
| 0xC7 WHOSGOT | type, quality | first on-screen party member carrying it, or 0x8001 |
| 0xCA JOIN | npc | `JoinParty` result (below) |
| 0xCC LEAVEPARTY | npc | `LeaveParty` result (below) |
| 0xD7 ISONSCREEN | npc | 1 if inside the visible 9×9 and on a visible square (`IsNpcOnScreen` C_0F8D_0159; party members always count while in a vehicle) |
| 0xDA WOUNDED | npc | 1 if HP < max HP |
| 0xDC POISONED | npc | poison status bit |
| 0xDD PARTYMEMBER | index, any | the NPC in party slot *index*; if *any* is 0, the *index*-th on-screen member. 0 if none. |
| 0xE0 ADDEXP | npc, n | adds n to experience, capped at 9999; result is the new value |
| 0xE1 ADDLVL | npc, n | adds n to the level (no cap) |
| 0xE2 ADDSTR, 0xE3 ADDINT, 0xE4 ADDDEX | npc, n | adds n, capped at 30. The stats are bytes, so a negative n that goes below 0 wraps round and is then capped to 30. |

#### Variables

There are 36 integer variables `#0`-`#9`, `#A`-`#Z` (`VarInt`) and 36 string variables
`$0`-`$9`, `$A`-`$Z` (`VarStr`); letter *X* is index `X − 0x37` (10-35).

| Variable | Set by | Value |
|---|---|---|
| `#0`-`#9` | `TALK_initTalk` | 0 at the start of each conversation |
| `$0`-`$9` | `TALK_initTalk` | point at ten 100-byte buffers (`D_E6F2`); the contents are not cleared |
| `#D`, `#M`, `#H` | `TALK_initTalk` | day, month, hour |
| `#N` | `TALK_initTalk` | for actors ≥ 0x40, ((actor − 0x40) & 7) + 1; else 0 |
| `#O` | `TALK_initTalk` | party size − 1 |
| `#W` | `TALK_initTalk` | the NPC's AI mode |
| `#T` | `TALK_initTalk`, `GIVEOBJ` | 0; then 1/0 for the last `GIVEOBJ` |
| `#X`, `#Y`, `#Z` | `TALK_initTalk` | the party's map position |
| `#R` | `RESURRECT` | revived NPC or −1 |
| `#Q` | `GAME_Load` / `GAME_Save` | the saved `IsOnQuest` byte; the only variable kept in the saved game |
| `$N` | `TalkDriver` | the NPC's name (the buffer is the 34-byte string "Winona Ryder is a really hot babe") |
| `$P` | `TALK_initTalk` | the name in party slot 0 (the player) |
| `$Y` | `SETNAME_Y`, `SETNAME_Y_YOU`, `RESURRECT` | a name |
| `$Z` | `TalkDriver` | the player's last input |

The other letter variables keep their values from one conversation to the next while the
game runs, but are lost on quitting (only `#Q` is saved). The engine never sets the other
`$` letters.

#### Keywords and input

Input opcodes, read by `TalkDriver`'s loop; each clears "already answered":

| Opcode | Input |
|---|---|
| 0xF7 ASKTOP | prints "You say:", reads a line of up to 50 characters |
| 0xF9 GETSTR `<i> B3` | reads a line into `$i` |
| 0xFB GETINT `<i> B2` | reads a line as a number into `#i`; invalid input gives 0 |
| 0xFC GETDIGIT `<i> B2` | waits for a digit key (Enter or Esc give 0) and stores it in `#i` |
| 0xFA GETCHR | reads one key |
| 0xF8 GET `<chars>` | waits until a key equal to one of the listed characters (upper-cased) is pressed; the list ends at the next `OP_KEY` |
| 0xCB WAIT | waits for a key |
| 0xEE ENDRES | no input; continues |

A keyword block is `OP_KEY` (0xEF), a comma-separated list, `OP_RES` (0xF6), the answer,
`OP_ENDRES` (0xEE). `TALK_MatchKeywords` (C_0F8D_1FA8) compares each keyword with the
input:

- An empty input counts as "bye".
- The input is split into words at spaces. A keyword matches if its **whole length**
  matches the start of any word, ignoring case; `?` in a keyword matches any character
  (`str_i_compare` C_0F8D_0004). So "job" matches "jobs" and "my job", but "jobs" does not
  match "job".
- A keyword `*` matches anything.
- Only the first matching block after an input runs. Later blocks are skipped up to their
  `OP_ENDRES`.
- If nothing matches, the engine prints nothing; a script needs a `*` block for that.

The keyword buffer (`TalkKeyword`, D_E66C) has 12 bytes before the next variable (the saved
description position, D_E678), so a keyword or `GET` list longer than 11 characters
overwrites it.

### Joining and leaving the party

`JoinParty` (C_0F8D_0290), from `OP_JOIN`, cut scenes and object handlers:

- Returns 0 at once if the NPC is already in the party.
- Counts the party members other than NPCs 2, 5 and 0x0B; 5 minus that count is the free
  room. The Avatar is counted. If there is no room and the NPC is not 2, 5 or 0x0B, it
  returns **2** (party full). So the party holds at most 8: the Avatar, four others, and the
  three NPCs 2, 5 and 0x0B, which can always join.
- In a vehicle it returns **1**. (The full check comes first.)
- Otherwise the NPC is appended; its party name is its script name, cut to 13 characters;
  it becomes player-controlled; its wait flag (flag 15) is cleared; a neutral NPC becomes
  good; its AI mode becomes 1 (follow) and its combat mode 8. Returns **0**.

`LeaveParty` (C_0F8D_03B4), only from `OP_LEAVEPARTY`:

- In a vehicle it returns **1**; if the NPC is not in the party, **2**.
- Otherwise it is removed (later members move up), stops being player-controlled, and a
  good NPC becomes neutral. Returns **0**.
- If its wait flag (flag 15) is set, its AI mode becomes 0x92: it stays where it is, with
  its inventory.
- If not, its mode becomes 0x8F (wanders at random) and it drops its **whole inventory** at
  its feet, where the party can take it.

### Scripted events (`OP_EVENT`, 0xD1)

`TALK_Event` (C_0F8D_15A2) runs one of these; other numbers do nothing.

| Event | Effect |
|---|---|
| 1, 3, 4 | count = random 2-10 creatures of type 0x140 are attempted around the party: chaotic, AI mode 6 (berserk). |
| 2, 5 | Unless flag 7 of NPC 0x28 is set: count = random 5-10 creatures of type 0x143, evil, AI mode 6. |
| 10 | End of the game: removes the temporary map files, shuts down graphics and sound, exits to DOS. The game is not saved. |
| 0x95 | `DrunkCounter` + 1 (see below). |
| 0x96 | Creates a permanent object 0xC94 (type 0x94, the drum type that `PlayDrum` handles, frame 3) at (0x215, 0x1C5, 0). |
| 0xAF | Moves NPC 0x19 to (0x3E5, 0x153, 0); NPC 0xB3 becomes neutral and motionless (mode 0). |
| 0xDE | Deletes the object of type 0xA1 (the doors that `C_1DBD_7800` opens) at (0x92, 0x89) on the current level. |

Creature placement for events 1-5: each attempt picks a square on the edge of the view:
either x = party x + (−5..+5) and y = party y ± 4, or x = party x ± 4 and y = party y +
(−5..+5), with equal chance. A creature appears only if the active member could stand
there (`CanMoveTo`). The loop counter goes up by one for every attempt and by one more for
every creature made, so at most ⌈count/2⌉ creatures appear (3-5 for events 2 and 5, 1-5
for 1, 3 and 4), fewer if squares are blocked.

`DrunkCounter` (saved): every party step while it is above 3 has a 1 in 2 chance of
printing "Hic!" and going in a random direction (`PartyAdvance` C_130F_24C3). It goes down
by 1 every game hour (`PassTime`). Event 0x95 and drinking (`UseDrink` C_1DBD_405D) raise
it by 1.

### How quest state is kept

#### Talk flags

Every actor (0-255) has 16 flags:

| Flags | Storage | Notes |
|---|---|---|
| 0-7 | `TalkFlags[actor]` (D_6EFC), bit *n* | Flag 0 = "met": name shown instead of description. |
| 8-15 | `D_3298[actor]`, bit *n* − 8 (a far 256-byte array) | Only bit 7 (**flag 15**) is used by the engine: the **wait flag**. |

The wait flag (flag 15): a non-party NPC that has it gets AI mode 0x92 and stays put
instead of following its schedule, every hour (`UpdateSchedules` C_130F_4F1B) and when a
schedule entry starts (`StartScheduleAction` C_130F_28BD). It also decides what
`LeaveParty` does. `JoinParty` and cut scenes 1 and 3 clear it. `EGG_generate`
(C_2667_015D) clears the whole `D_3298` byte for a new creature, but no code clears
`TalkFlags` for a new creature, so a temporary creature slot (0xC0-0xFF) keeps the flags of
the creature that used it before.

Scripts set the flags with `SET`/`CLR` and read them with `TST`. NPC 1's flags (the
Avatar's) serve as the main story flags.

#### The saved game

Both arrays are saved in `savegame\objlist` with everything else (`GAME_Save` C_05D0_08B7,
`GAME_Load` C_05D0_047A). Offsets in the file:

| Offset | Size | Contents |
|---|---|---|
| 0x0000 | 0x100 | object status, actors |
| 0x0100 | 0x300 | positions, actors |
| 0x0400 | 0x200 | shape/type, actors |
| 0x0600 | 0x200 | quality/quantity, actors |
| 0x0800 | 0x100 | NPC status |
| 0x0900 / 0x0A00 / 0x0B00 | 0x100 each | strength / dexterity / intelligence |
| 0x0C00 | 0x200 | experience (16-bit) |
| 0x0E00 | 0x100 | hit points |
| 0x0F00 | 0xE0 | party names (16 × 14) |
| 0x0FE0 | 0x10 | party list |
| 0x0FF0 | 1 | party size |
| 0x0FF1 | 0x100 | levels |
| 0x10F1 | 0x100 | `D_45FA` |
| 0x11F1 | 0x100 | AI modes |
| **0x12F1** | 0x100 | **`D_3298`: talk flags 8-15** |
| 0x13F1 | 0x100 | combat modes |
| 0x14F1, 0x15F1 | 0x100 each | movement points (written twice) |
| 0x16F1 | 0x200 | original shapes |
| **0x18F1** | 0x100 | **`TalkFlags`: talk flags 0-7** |
| 0x19F1 | 0x100 | `D_5100` (leaders / path slots) |
| 0x1AF1 | 0x100 | `D_4602` |
| 0x1BF1 | 0x100 | `D_45FE` |
| 0x1CF1 | 0x82 | the game-state block |

The **game-state block** (`gamestat.c`, DS 27E8-2869) is saved as one piece. Most of it
belongs to other sections. The parts that matter here:

| Offset in block | Variable | Use |
|---|---|---|
| +0x00 | `IsOnQuest` | the saved copy of `#Q`: `GAME_Load` copies it into `#Q`, `GAME_Save` copies `#Q` back. Nothing else reads it (Ultima VI's quest barrier is gone: `ShowQuestBarrierMsg` is set but never read). |
| +0x02 - +0x07 | `Time_M`, `Time_H`, `Date_D`, `Date_M`, `Date_Y` (16-bit) | the clock; scripts see `#H`, `#D`, `#M` |
| +0x26 | `DrunkCounter` | event 0x95 |
| +0x5E | `SpeakGargish` | U6's name; here, when it is 1 the active member can move onto any square (`CanMoveTo` C_130F_0265). Toggled by debug key 222. |
| +0x81 | `AvatarPortrait` | 0 = no character created yet: GAME.EXE prints "You must first create a character." and runs `SAVAGE.EXE` again with argument "X" (`main` in main.c) |

`KARMA`, `MoonstonePos`, the moon variables, `VanishedShape` and `AvatarFemale` are kept
from Ultima VI and not referenced by name in GAME.EXE's sources.

#### Flags the engine itself reads or writes

Everything else about the story is up to the scripts. These are the flags the engine
touches:

| Flag | Written by | Read by |
|---|---|---|
| any NPC, flag 0 (met) | scripts; cut scene 1 sets it for NPC 0x17 | `TALK_talkTo`, `ShowPortrait` (name or description) |
| NPC 1, flag 0 | cut scene 1 | `C_1DBD_6AD8` (cut scene 1 runs once) |
| NPC 1, flag 1 | cut scene 2 | `UseGem` C_1DBD_6732, `CmdMove` C_1DBD_26E8 (cut scene 2 runs once) |
| NPC 1, flag 2 | cut scene 3 | `LoseHP` C_1810_289B (cut scene 3 runs once); the trap door (`C_08CB_0855`); earth tremors (`DisplayTick` C_0359_0CD4); level 2 light (`PassTime`); objects of type 0x0B stop giving light (`visible.c`) |
| NPC 1, flag 3 | cut scene 4 | `PlayDrum` C_1DBD_785B (cut scene 4 runs once); `EGG_hatches` C_2667_04F7: eggs make only one creature of type 0x143 |
| NPC 1, flag 6 | nothing in the engine | `GameLoop`: enables the debug keys |
| NPC 1, flag 7 | cut scene 1, set during its `EGG_hatchArea` and cleared after | nothing |
| NPC 2, flag 5 | scripts | last cut scene: ending "LA" |
| NPC 3, flag 1 | scripts | `Flash`: blocks NPC 0x19's talk |
| NPC 4, flag 4 | `UseDoor` | `UseDoor` (NPC 4 joins once) |
| NPC 0x0B, flag 6 | `UseStatueHead` C_1DBD_6577, `UseHeadlessStatue` C_1DBD_666A | nothing in the engine |
| NPC 0x10, flag 7 | cut scene 1 | `PlayDrum` |
| NPC 0x11, 0x13, 0x1B, 0x1D, 0x20, 0x22, flag 7 | scripts | `PlayDrum` |
| NPC 0x14, flag 7 | `UseBell` C_1DBD_36C3 (bell put on NPC 0xB5) | `PlayDrum` |
| NPC 0x19, flags 0-7 | `Flash`: set to 0x28 (flags 3 and 5, the others cleared) | scripts |
| NPC 0x21, flag 0 | scripts | first-run conversation |
| NPC 0x28, flag 7 | cut scene 3 | `PlayDrum`; events 2 and 5 (no more type-0x143 creatures) |
| NPC 0x30, flag 7 | `COMBAT_Kill` C_1810_25AD when NPC 0xB4 dies | `PlayDrum` |
| NPC 0x31, flag 5 | cut scene 3 | scripts |
| NPC 0x33, flag 5 | scripts | last cut scene: ending "LT" |
| NPC 0x35, flag 5 | `COMBAT_Kill` when NPC 0x17 dies | scripts |
| NPC 0x35, flag 7 | `Flash` | `PlayDrum`; `Flash` (once); `EGG_generate`: before it is set, creatures of type 0x13C are made chaotic instead of their class alignment |
| a dinosaur, flags 0-7 | `UseBell`: all set (0xFF) | `UseBell` ("It already has a bell on its neck." if any of flags 0-7 is set); `CmdLook` C_1DBD_12BD: an actor of type 0x126 with all eight set is described "with a bell around its neck" |

### Story events in the engine

Five cut scenes live at the start of `objcmd.c`. Each plays a song, shows pictures and text
pages from `MIDGAMEn.LZC` (`ShowMidgame` C_276B_245A: picture entries 4i+3, up to three text
pages 4i+0..2 each, "*E" ends the pages early), then changes the world. While a cut scene
runs, `IsInConversation` is set, so deaths it causes print no "killed" message.

| # | Function | Trigger | Song | Pictures |
|---|---|---|---|---|
| 1 | C_1DBD_002E | Using an object of type 0x113 at frame 1 (it switches to frame 3) when it is at most 2 tiles east and 2 tiles south of NPC 2 on the same level, and NPC 1's flag 0 is clear (`C_1DBD_6AD8` C_1DBD_6AD8). The offsets are signed with no absolute value, so any distance to the west or north also passes. | 0x0E | 3, NPC 2's portrait first |
| 2 | C_1DBD_01A0 | Putting an object on a gem stand (type 0x42) while NPC 1's flag 1 is clear: using a gem (type 6) next to the stand (`UseGem`), or moving any inventory object onto the stand (`CmdMove`; the code does not check the object's type there). | 0x0F | 3 |
| 3 | C_1DBD_0304 | Destroying an object of type 0xDE (its quantity runs out) while NPC 1's flag 2 is clear (`LoseHP`). | 0x10 | 4, NPC 5's portrait first |
| 4 | C_1DBD_05DC | Using a drum (type 0x94) while NPC 1's flag 3 is clear and flag 7 is set on all eleven NPCs 0x10, 0x11, 0x13, 0x14, 0x1B, 0x1D, 0x20, 0x22, 0x28, 0x30, 0x35 (`PlayDrum`). | 0x11 | 3 |
| 5 | C_1DBD_077E | Destroying an object of type 0x93 (`LoseHP`). No flag check. | 0x0C | 3 |

`PlayDrum` means to require an object of type 0x5B ("You have nothing with which to play
the drum."), but it tests `if(FindInvType(...))`, and `FindInvType` returns −1 when there
is none, which counts as true. In practice the drum works without it.

**Cut scene 1** (C_1DBD_002E): NPC 2's wait flag is cleared and NPC 2 joins the party. NPC
0x17 is moved to (0x73, 0xA8) on the current level, its wait flag cleared, its flag 0 set,
its AI mode set to 8 (assault). Flag 7 of NPC 0x10 is set. The eggs of the area hatch
(`EGG_hatchArea`, with NPC 1's flag 7 set during it). NPC 1's flag 0 is set and **combat
begins** (`COMBAT_begin`).

**Cut scene 2** (C_1DBD_01A0): if the minutes are not 0, time passes to the next full hour
and then hour by hour to 12:00 (at exactly hh:00 no time passes). A permanent object 0x4A0
(type 0xA0, frame 1) is created at (0xC0, 0x1CC, 0); going through a type-0xA0 entrance
takes the party to (0x9D, 0x89) on level 3 (`C_08CB_0855`). NPC 1's flag 1 is set; tile
0x20A's animation frame is set to tile 0x11C.

**Cut scene 3** (C_1DBD_0304), after the explosion of the type-0xDE object (an explosion of
type 0x106 that does 1-30 damage to every non-stackable object in it; every object of type
0x100 becomes type 0xDF):

- NPC 0x0B takes frame 11 and drops its inventory; a map object with its shape is left on
  its square; then NPC 0x0B dies.
- NPCs 0x2F, 0x1F, 0x37 and 0x23 die.
- NPC 5 is moved to NPC 0x0B's square, brought back to life with full hit points, and joins
  the party.
- NPCs 0x28 and 0x2C lose their wait flag and take shape 0x142 (current and original).
- Flag 7 of NPC 0x28, flag 2 of NPC 1 and flag 5 of NPC 0x31 are set.
- 24 hours pass.
- Every actor of type 0x12D or 0x129 dies. Every egg whose first embryo is of type 0x129
  or 0x12D is deleted with it. Every object of type 0x44 (a travel object) becomes type
  0x26.
- After the pictures, tile 0x20A's animation frame becomes tile 0x11D.

The world after flag 2 of NPC 1 (cut scene 3):

- Leaving through an entrance of type 0x103 (which leads to (0xC0, 0x1CC, 0)) prints "The
  mechanism supporting the trap door fails just as you and your companions leave the
  stairwell, sealing the entrance to the underground city." and deletes the type-0xA0
  entrance at (0xC0, 0x1CC). The message is printed every time the party uses a 0x103
  entrance, even when the entrance is already gone (`C_08CB_0855`).
- On level 3, each display tick shakes the screen with chance 1 / (⌊max(\|dx\|, \|dy\|) / 4⌋
  + 2), where dx, dy are the party's distance from (0x9D, 0x89) (`DisplayTick`).
- Level 2's ambient light drops from 2 to 0 (`PassTime`).
- Objects of type 0x0B give no light (`visible.c`).

**Cut scene 4** (C_1DBD_05DC): NPC 1's flag 3 is set. The party is teleported to (0xF3,
0xCC) on level 1. An object of type 0x16 on that square is set to frame 1 (a rope that can
be climbed). If the member in the status area carries an object of type 0x15, its tile is
animated (frame 0x10C).

**The last cut scene** (C_1DBD_077E): after the pictures it builds four arguments and runs
`SAVAGE.EXE` (which plays the endgame), in an endless `execl` loop:

| Argument | Built from | Example |
|---|---|---|
| configuration | `BuildConfigString`: graphics letter, M/K, music letter | `VMA` |
| `D` + 2 digits | day − 1 (`Date_D + 99` printed, first digit replaced) | day 1 → `D00` |
| `B` + 2 digits | month − 1 | month 3 → `B02` |
| `Y` + 2 digits | year | year 0 → `Y00` |
| `LA`, `LT` or empty | `LA` if NPC 2's flag 5 is set, else `LT` if NPC 0x33's flag 5 is set, else nothing | |

`SAVAGE.EXE`'s `ParseConfig` reads them: any of D, B, Y starts the endgame instead of the
title; `LA` and `LT` add extra endgame pages, which the `SAVAGE.EXE` sources name the
Aiela and Tristia endings. Before running it, GAME.EXE deletes its temporary map files
and shuts down graphics and sound. **It does not call `GAME_Save`**; the game in progress
is not saved.

**NPC 0x19 and the flash** (`Flash` C_1DBD_7204, from the camera and flash powder): if NPC
3's flag 1 and NPC 0x35's flag 7 are both clear, and NPC 0x19 is in the 11x9 tiles around
the party: when the flash is more than 1 tile east or south, or more than 2 tiles west or
north, of NPC 0x19, it prints "You hear a barely audible voice whisper, "Closer..."".
Otherwise NPC 0x19's flags 0-7 become 0x28, NPC 0x35's flag 7 is set, and NPC 0x19 talks.

**NPC 4 at the door** (`UseDoor` C_1DBD_7580): closing (frame 5 → 1) the door at (0x184,
0x235, level 0), when NPC 4's flag 4 is clear and NPC 4 is in the 11x9 tiles around the
party: NPC 4 wakes, its flag 4 is set, it joins the party and talks.

**The statue** (`UseStatueHead` / `UseHeadlessStatue`): the head (type 0xBE) and the
headless statue (type 0xF7) next to each other: both are deleted, NPC 0x0B appears there
and joins the party, and its flag 6 is set. The doors of type 0xA1 open only when NPC 0x0B
is in the party and in the 11x9 tiles around it (`C_1DBD_7800`): "Yunapotli closes its eyes
and appears to concentrate...".

### Death: waking in Intanya's hut

When the Avatar (NPC 1) is dead, `CheckActiveMember` (C_130F_4943) calls `C_2701_0556`:

1. Prints "Overcome by your wounds, you fall unconscious.....". With music, plays song 11
   to its end (up to 100 waits), then queues the area song (1 on the surface, 2 on levels
   1-2, 3 on level 3). (It returns at once if `Active` is 0x10; why is not clear.)
2. Prints "....You awaken in Intanya's hut, feeling restored." and calls `C_08CB_0C73`.

`C_08CB_0C73`:

- Every player-controlled actor (0-0xBF) that is dead is revived from its dead body (type
  0xF9 with its number as quality): the Avatar directly (`OBJ_ReviveFromBody`), the others
  with `SPELL_ReviveNPC`.
- Every actor in AI mode 0x9B goes back to its schedule (mode 0x81).
- Every living party member: wakes, gets its original shape back if asleep or of type
  0x133, becomes good, loses paralysis, sleep, poison and the dead bit, gets full hit
  points, **loses a quarter of its experience** (experience × 3 / 4, integer), and gets
  movement points equal to its dexterity.
- Spell effects 0 and 1 end. Combat ends. Party mode (or the solo member) is restored.
- The party is teleported to **(0x19C, 0x123, level 0)**.
- **Seven days pass**: the date moves on 7 days (months of 28 days, 12 months), the hour
  stays the same. No `PassTime` runs, so the hourly updates do not happen for that week.

### The first conversation

`GameLoop` (C_0359_1A65, at start) and `GAME_Restore` (C_05D0_03A2, after every load,
including the reload that follows a save) both do this: if NPC 0x21's flag 0 is clear, the
active member talks to NPC 0x21 (through the usual `TALK_talkTo` checks), then the date is
set to day 1, month 1, year 0. If the script never sets NPC 0x21's flag 0, this happens on
every load.

### Debug keys

`GameLoop` handles nine key codes that come from typing Alt + a decimal code on the
numeric keypad (213, 214, ...). Except for 213, they work only while **NPC 1's flag 6**
(`TalkFlags[1] & 0x40`) is set; no engine code sets it, so a script (or a saved-game edit at
offset 0x18F1 + 1) must.

| Code | Effect |
|---|---|
| 213 (0xD5) | prints time and position (hex). With the flag, also shows the map (`PeerMap`). Works without the flag (text only). |
| 214 (0xD6) | teleport: x (3 hex digits), y (3), level (1). Level ≠ 0 masks x and y to 0xFF; level is capped at 5. Not in a vehicle. |
| 215 (0xD7) | one hour passes, NPCs path with teleporting allowed |
| 222 (0xDE) | toggles `SpeakGargish`: "Avatar desolidifies!" (the active member walks through anything, `CanMoveTo`) / "Avatar solidifies!" |
| 224 (0xE0) | teleports next to NPC n (2 hex digits) if it is not dead and n ≤ 0x98 |
| 232 (0xE8) | shows NPC n's name and portrait |
| 233 (0xE9) | plays sound effect nn (two decimal digits), if sound is on |
| 242 (0xF2) | shows and sets a talk flag: NPC (2 hex digits), bit (1 hex digit), new value (1 hex digit, 0 = off). Bits 0-7 are `TalkFlags`; any bit 8-F means the wait flag (flag 15). |
| 252 (0xFC) | prints the free tile memory |

### What could not be determined

- The meaning of most object and creature types named above (0x113, 0xDE, 0x93, 0x140,
  0x143, ...) and of most NPC numbers: the game data files (`talk.lzc`, `look.lzd`) are not
  in this tree, so names here come only from strings and comments in the code.
- Which flags the scripts set and test: that lives in `talk.lzc`. The tables above list only
  what the engine does.
- `GiveObj`, `TakeObj` and `TestObj` are in other modules; their exact rules (stacking,
  failure cases) are not covered here.
- The `Active == 0x10` early return in `C_2701_0556` has no clear purpose.

## Time, rest, character stats and levels

This section covers the game clock and calendar, what happens as time passes, NPC
schedules, resting and eating, levelling up, the character attributes, carrying
capacity, status effects, movement points and harmful terrain. Every rule cites the
function that implements it. Random rolls use `OSI_rand(lo, hi)`, which returns a
number from lo to hi, both included. So "1 in 26" below means `OSI_rand(0, 25) == 0`.

### The clock and the calendar

The time is kept in five saved variables (`gamestat.c`): `Time_M` (minute, 0-59),
`Time_H` (hour, 0-23), `Date_D` (day, 1-28), `Date_M` (month, 1-12) and `Date_Y`
(year, counted from 0). The values built into GAME.EXE are 08:00 on day 1, month 1,
year 0. A saved game overrides them. When a new game starts (the introduction
conversation with NPC 0x21, Intanya), the date is set back to 1/1/0 but the time of
day is not changed (`GameLoop` C_0359_1A65, `GAME_Restore` C_05D0_03A2).

The calendar has 60 minutes per hour, 24 hours per day, **28 days per month** and
**12 months per year**, so a year has 336 days (`PassTime` C_0359_11B6). For NPC
schedules, the day of the week is `(Date_D - 1) % 7 + 1` (1-7). Each month is
therefore exactly four weeks (`UpdateSchedules` C_130F_4F1B).

#### What makes time pass

Time passes only when something calls `PassTime(minutes)` (C_0359_11B6).
`PassTime(0)` advances nothing. It only refreshes the light, the sky and the
per-call effects listed below.

| Event | Minutes | Where |
|---|---|---|
| A scheduler round ends: no actor in the area has movement points left. Every actor gets new points (see "Movement points"). | 1 | `RunNPCTurns` C_130F_4C0F |
| The party leader (or the raft) takes a step. A blocked step costs nothing. | 1 | `PartyAdvance` C_130F_24C3 → `CheckGatesAndEntrances` C_130F_2404 |
| Combat is broken off | 1 | `COMBAT_breakOff` C_1810_023B |
| Resting, for each hour asked | 3 × 20 | `REST_Command` C_29D8_050B |
| Cut scene 2 (MIDGAME2) | up to the next 12:00, but nothing passes if `Time_M` is exactly 0 (the wait sits inside `if(Time_M)`) | C_1DBD_01A0 |
| Cut scene 3 (MIDGAME3) | 24 × 60 | C_1DBD_0304 |
| Debug key (ALT-2-x, only with story flag `TalkFlags[1] & 0x40`) | 60 | `GameLoop` C_0359_1A65 |
| The party is "overcome by its wounds" and wakes in Intanya's hut | `Date_D += 7` (wrapping at 28 into the next month and year). This is set directly, **without** `PassTime`: the hour does not change, no schedules update and no status effects tick. | C_08CB_0C73 |

The other `PassTime(0)` callers (loading a game, teleports, entrances, lighting a
torch, the Light spell, picking up a lit torch) only refresh the state.

Walking therefore costs one minute per step, **plus** one minute each time every
actor has used up its movement points. For example, a DEX 20 party member walking
out of combat on terrain of cost 0 pays 3 points per step (see below). It makes
about 7 steps per round, so about 8 minutes pass per 7 steps.

#### What happens in each `PassTime` call (C_0359_11B6)

The steps below run in this order.

1. **Spell timers.** `SpellFx[0]` and `SpellFx[1]` go down by `minutes`, with 0 as
   the minimum. `SpellFx[0]` is the Light spell: "Triolo creates light" sets it to
   200 minutes (`SPELL_Execute` C_11FF_0D96). `SpellFx[1]` is read in `object.c` but
   nothing in Savage Empire sets it.
2. **Explosive fuse.** If `KegFuse > minutes`, it goes down by `minutes`. Otherwise,
   if it was non-zero, it becomes 0 and the lit object `KegObject` explodes
   (`Detonate` C_1DBD_3C68). Lighting a grenade (type 0x89) or activating a device
   (0xf1) sets the fuse to **3 minutes**. Only one can be lit at a time
   (`UseExplosive` C_1DBD_3AA1).
3. **Eruption flag.** With a 1 in 8 chance, `Erupting` is set to 1. `ShowObjects`
   clears it and nothing else reads it, so it has no effect.
4. **For each actor 0..0xFF that exists:**
   - *Torches* (party members only, `NPCStatus & 0x80`). See "Torches" below.
     This happens once per call.
   - *Status effects*, rolled **once per minute**. See "Status effects".
   - *Harmful terrain*: `ApplyTerrainEffects`, once per call (also when
     `minutes` is 0). See "Harmful terrain".
5. **Calendar.** For each hour boundary crossed: the hour goes up (rolling over
   day, month and year as above). `NextSleep` goes down by 1 if it is non-zero, and
   so does `DrunkCounter`. Then `UpdateSchedules` runs. If several hours pass in one
   call, this happens once for each hour.
6. **Hourly flags.** When the hour differs from `LastHourUpdate`, `minutes` is
   forced to 15 so that the sky is redrawn. Except on the first call after start-up,
   `HourlyUnused` and `CaughtFish` are cleared and `MustRingBell` is set. Nothing in
   Savage Empire reads these three variables.
7. **Sundial tile.** This happens if `minutes > 10`, which includes any call that
   changes the hour. With `t = Time_H*60 + Time_M`, the animated tile of object type
   0x92 frame 3 becomes 0x143 (night) when `t < 400` or `t > 1079`. Otherwise it
   becomes `0x148 + (t - 400) / 85`, giving 8 day frames 0x148-0x14F from 6:40 to
   17:59. Type 0x92 is the sundial: *Look* at it between 5:00 and 19:59 prints the
   time as "h:mm A.M./P.M." (`CmdLook` C_1DBD_12BD). The sky bar is then redrawn
   (`DrawSkyBar`).
8. **Ambient light.** See the table below. If the level changed, the view is
   recomputed (`VIS_UpdateView`).

There is **no hunger and no thirst**. `PassTime` has no food logic. Food matters only
when resting (see "Rest"). Eating food with *Use* only removes one unit
(`UseFood` C_1DBD_4225).

#### Ambient light (`PassTime` C_0359_11B6)

`t` is the time of day in minutes (`Time_H*60 + Time_M`).

| Where / when | `AmbientLight` |
|---|---|
| Level 2, before story flag `TalkFlags[1] & 4` | 2 |
| Any other level below the surface (`MapZ > 0`) | 0 |
| Surface, `t < 400` (before 6:40) or `t > 1080` (after 18:00) | 1 |
| Surface, 400 ≤ t < 440 (dawn) | `(t - 400)/10 + 2`, which is 2, 3, 4 or 5 |
| Surface, 1040 < t ≤ 1080 (dusk) | `(1080 - t)/10 + 2`, which is 5 down to 2 |
| Surface, 440 ≤ t ≤ 1040 | 7 |
| Then: if the value is below 2 and a party member holds a lit torch, or the Light spell is running | raised to 2 |

The party lights the squares it can see. A square at distance *d* from the centre
(from `DistanceTable`, roughly max + min/2) gets light `4 - d + AmbientLight`,
clamped to 0..4. A square with light 0 is drawn black (`VIS_FloodFill` C_09DA_0144,
`VIS_UpdateView` C_09DA_0327). So a square is lit out to distance 10 by day (the
whole window), distance 4 on a surface night, distance 5 with a torch or the spell,
and distance 3 underground without one. Squares lit 1-3 are drawn with a shading
tile `0x1BC + light` (`ShowObjects` C_0A5D_3577). Light-giving map tiles (`TileFlag & 3`) add
their own light within radius `2 × (flag) - 1`, but only while `AmbientLight < 7`.

#### The sky bar (`DrawSkyBar` C_276B_1756)

The strip above the map shows:

- **Underground:** a row of cave ceiling tiles (0x17A).
- **Surface, 360 ≤ t ≤ 1080 (6:00-18:00):** day sky tiles (0x17B), with the sun
  (0x171) at pixel x = `(1080 - t) / 34 × 8`. The sun moves from right (x=168 at
  6:00) to left (x=0 at 18:00).
- **Night:** night sky tiles (0x179), with the moon (0x172) placed the same way
  using `(t + 721) % 1440`. The moon also crosses from right to left during the
  night. Only one moon is drawn. U6's two-moon phase variables (`TrammelPhase` and
  the others in `gamestat.c`) are never used.
- **Middle ornament:** combat tiles 0x177/0x178 while `InCombat` is set; 0x164/0x165
  while NPC 1 has shape 0xF9 (a body); otherwise 0x173/0x174. Resting shows
  0x164/0x165, and the level-up dream shows 0x175/0x176 (`REST_Command`,
  `REST_LevelUp`).

The bar is redrawn at every leader step (`CheckGatesAndEntrances`), on level
changes, whenever the party list is drawn, and from `PassTime` as described above.
Portrait backgrounds also have a night version: for `t ≥ 1081` or `t ≤ 359`,
backgrounds 0-4 use background number + 4 (`PortraitBg` C_276B_1AFD).
`DrawClockHands` (C_0359_0A7E) can draw hour and minute hands on a clock tile. It
uses `ClockHourToDir` for the hour hand and `Time_M*2/15` for the minute hand, but
nothing calls it.

#### DisplayTick (C_0359_0CD4)

`DisplayTick` is the per-frame display update, not a time step. It recomputes the
view if needed, animates tiles and creatures (`AnimateTilesAndActors` C_0359_0030),
and redraws the map window. One gameplay effect lives here. Once story flag
`TalkFlags[1] & 4` is set, while the party is on level 3, the screen shakes on each
tick with a chance of `1 / (max(|dx|, |dy|)/4 + 2)`. Here dx and dy are the party's
distance from (0x9D, 0x89). The shaking gets more frequent closer to that spot.

### Torches (`PassTime` C_0359_11B6)

A torch burns only while it is a lit torch (type 0xD1) **equipped**
(`ObjStatus & 0x18 == 0x18`) by a party member. On each `PassTime` call, each such
torch has a 1 in 2 chance to lose `minutes` from its quality. If its quality is
`≤ minutes` when that roll succeeds, it burns out and is deleted (`BurnOutMsg`).
Lighting a torch (*Use* on type 0xD0, `UseTorch` C_1DBD_32FC) or picking up a lit
one (`CmdGet` C_1DBD_1FEA) sets its quality to **100**. On average a torch lasts
about 200 one-minute calls. During rest (20-minute calls) it lasts about 10 calls,
which is about 3 hours. Unreadying a lit torch destroys it at once
(`STAT_CheckUnready` C_0E13_14F3).

### NPC schedules

Each NPC 2..0x97 has a list of schedule entries
(`Schedule[SchedPointer[n] .. SchedPointer[n+1]-1]`). Each entry has a location, an
action (an AI mode) and a time byte. Bits 0-4 of the time byte are the hour. Bits 5-7
are the day of the week, 1-7, where 0 means every day (`UpdateSchedules`
C_130F_4F1B).

**Every hour** (`UpdateSchedules` C_130F_4F1B, called from `PassTime`), each NPC
2..0x97 is handled in turn:

- Mode 0x9B (a scheduled NPC provoked by the player) calms down to AI_FINDPATH
  (0x81).
- An NPC with the WAIT flag (`D_3298 & 0x80`) that is not in the party gets mode
  0x92 and is skipped.
- Otherwise, an NPC that exists, is not in the party, and is in a schedule mode
  (≥ 0x80) or in mode 7, searches its entries **backwards**. The first entry
  whose hour equals `Time_H` and whose day is 0 or today becomes its current entry
  (`SchedIndex`), and the NPC is set to AI_FINDPATH (walk there). If several
  entries match, the last one in the list wins. If none matches, the NPC keeps what
  it is doing.

**Getting there** (`PTH_SearchPaths` C_130F_4847, called at the start of each
`RunNPCTurns`): for NPCs in AI_FINDPATH, at most 3 are teleported and at most 3 get
a path search per turn, while path slots are free. An NPC is teleported straight to
its scheduled spot unless it, or the spot, is within 5 squares of the player on
the player's level (`TeleportToSchedule` C_130F_2F03). So NPCs walk visibly only
near the party and jump into place elsewhere. A path that stays blocked for 3 turns
is abandoned (AI_PATH_FAILED) and tried again later. Arriving within 1 square of the
spot starts the action (`__DoOnPath` C_130F_3D98).

**Starting the action** (`StartScheduleAction` C_130F_28BD): the NPC's mode becomes
the entry's action, with these special cases:

- NPC 0x23 stays motionless.
- A waiting NPC gets mode 0x92.
- **Sleep (0x91):** the NPC is marked asleep (`NPCStatus |= 4`), gets its sleeping
  shape (`SetTypeUnconscious`), and the bed object (type 0xF6) on its square gets
  frame 1. If it is outside the loaded area, its mode stays 0x80 until it is loaded.
- Any other action wakes it: asleep cleared, own shape back, bed to frame 0.

If the NPC is not on its scheduled square, its mode becomes AI_FINDPATH instead.

The actions that `NPC_TakeTurn` (C_130F_4121) runs:

| Mode | Behaviour |
|---|---|
| 0x8F (wander) | random step 1 in 8, else wait |
| 0x90, 0x94 (loiter, farm) | 1 in 8: a step biased back toward the spot; else wait |
| 0x96 (converse) | if the spot is within 5 squares of the party: walk to the nearest member and start talking (then mode 0x90); else loiter |
| 0x99 (brawl) | 1 in 4: go and harmlessly "hit" an opponent within 5 of the spot, then come back; else loiter |
| 0x8B-0x8E | becomes a creature of type 0x149 that moves in a fixed direction (see `AnimateTilesAndActors`) |
| others (0x87-0x8A, 0x91, 0x92, ...) | stand: spend 5 points |

### Status effects and how they wear off

The status bits are spread over two arrays. `NPCStatus` holds 0x01 protected, 0x02
paralysed, 0x04 asleep, 0x08 poisoned, 0x10 dead, 0x60 alignment and 0x80 party
member. `ObjStatus` (for actors) holds 0x04 charmed, 0x40 cursed and 0x80 "wild".
The rolls below are made **once per minute** for every existing actor 0..0xFF,
anywhere in the world (`PassTime` C_0359_11B6).

| Effect | How it is gained | How it wears off (per minute) |
|---|---|---|
| Protected (NPCStatus 0x01) | "Aphazz protects the party": every party member near the caster (`SPELL_CastOnNearby` C_11FF_0A08, spell 0x12) | 1 in 26 |
| Cursed (ObjStatus 0x40) | curse spell on hostile NPCs that fail to resist (spell 0x20) | 1 in 26 |
| Charmed (ObjStatus 0x04) | charm spell (0x11) | 1 in 26, then `COMBAT_stopCharm` C_1810_00BB restores the alignment and mode |
| Wild (ObjStatus 0x80) | spell 0x22 on party members | cleared at the first minute **out of combat** |
| Paralysed (NPCStatus 0x02) | webs (see "Harmful terrain") | 1 in 4 **and** `GetStr ≥ 1d30`, both tested each minute |
| Asleep (NPCStatus 0x04) | schedule sleep (0x91); script `OP_SETMODE 0x91`; weapon type 1 used against creature type 0x126 (`__TryHit` C_1810_337D) | 1 in 21, only if the NPC's mode is not 0x91 (schedule or script sleep never wears off this way) and its square is loaded |
| Poisoned (NPCStatus 0x08) | hit by a monster whose class has flag 0x40 (1 in 4, `__DoHit` C_1810_3134); weapon type 0x20 (1 in 2, `__TryHit`). Protected and invulnerable actors are immune. | **never by itself**. Each minute, 1 in 8: lose 1 HP (`LoseHP`), unless protected or the actor is NPC 0x19. `LoseHP` does nothing to an actor on another level than the party's. |

Poison can kill. It is cured by:

- the healing object, type 0xFC: full HP and cure, but only used on someone below
  max HP (`UseHealing` C_1DBD_7DA2);
- the heal spell: 1d10 HP and cure (`SPELL_Heal` C_11FF_04AC);
- the script ops `OP_CURE` and `OP_HEAL` (full HP only) (talkdr.c);
- revival (`SPELL_ReviveNPC` C_11FF_00B0);
- waking after the party's defeat (C_08CB_0C73).

When an actor wakes from non-scheduled sleep it gets its own shape back. A wakened
creature of type 0x126 rebuilds its extra parts (removes the 0x118 two squares
south, adds parts 0xD25/0xD27). The code also tries to reset a bed under a variable
`obj` that is never set (a bug), so that part acts on whatever the stack holds
(`PassTime`).

Two more per-minute rules:

- The creature summoned by the "help" spell (`D_EADE`) turns hostile (alignment
  0x60, mode 0x13) at the first minute out of combat, and stops being the summoned
  one (`PassTime`; summoned by `SPELL_Help` C_11FF_05AE).
- When the party gathers to change place with animation (teleports, entrances,
  some level changes), all members are woken and freed from paralysis
  (`PartyEnter` C_08CB_0564).

Effects on attributes are in "Character attributes" below. Asleep actors have
effective DEX 1. Paralysed, asleep or dead actors get no turns (`RunNPCTurns`).

**Drunkenness.** Each drink raises `DrunkCounter` by 1: *Use* on type 0x5F
(`UseDrink` C_1DBD_405D), or conversation event 0x95 (talkdr.c). The counter drops
by 1 every hour. While it is **above 3**, each step of the leader has a 1 in 2
chance to go in a random direction, with "Hic!" (`PartyAdvance` C_130F_24C3).

### Rest (`REST_Command` C_29D8_050B)

**Conditions.** The checks are made in this order, and the first failure refuses
the rest:

1. Not in combat.
2. "Not while foes are near!": any actor 0x10-0xFF that exists, has the hostile bit
   (`NPCStatus & 0x20`, which includes chaotic) and is on the party's level within
   39 squares in both x and y. Dead or asleep status is not checked
   (`REST_FoesNear` C_29D8_00C2).
3. "It's too noisy to sleep here!": any actor 1-0xFF that is not dead, not asleep
   and not a party member, is within 5 squares on the party's level, and that
   `COMBAT_canSee(1, it)` says the Avatar can see (`REST_OthersNear` C_29D8_014B).
4. Party mode, not solo.
5. Not aboard the raft.
6. No party member is of type 0x133 (the turtle): "Not on a turtle!".

**How long.** Before 7:00 or after 17:00 the game offers "Rest until sunrise?",
which means `7 - Time_H` hours before 7:00, or `31 - Time_H` hours after 17:00.
From 7:00 to 17:59 it offers "until sunset?", which is `18 - Time_H` hours.
Minutes are ignored, so the party wakes at 7:MM or 18:MM. Answering N asks "How
many hours?", and only one digit 1-9 is accepted. Anything else cancels the rest.

**Camp.** The party gathers around the leader in at most 20 tries. Member k goes to
the leader's square plus (`CampOfsX[k]`, `CampOfsY[k]`) and faces `CampFacing[k]`
(table at DS 28B6-28C6):

| Member | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| dx | 0 | 0 | 1 | -1 | 1 | -1 | 1 | -1 |
| dy | -1 | 1 | 0 | 0 | -1 | 1 | 1 | -1 |
| faces | S | N | W | E | S | N | W | E |

**Meal and healing.** This happens only if the rest is **more than 4 hours**.
Each member, except NPC 11 (Yunapotli, "does not eat"), eats one unit of food. It
comes from the member's own pack first, else from the first member (in party order)
who has some (`TryEat` C_29D8_0081, `TryEatExclusive` C_29D8_0027). The edible types
are (`EdibleTypes`, DS 286C):

`0x3D, 0x3E, 0x5D, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x6B, 0x6C, 0x6D, 0x6E, 0x6F, 0x71, 0x72, 0x78, 0xC5, 0xD5, 0xD6`

0x6C is corn and 0x6D is beans (from `UseCornPlant` and `UseBeanPlant`). 0x3D and
0x3E are the corn and bean plants themselves. The same list less 0x3D/0x3E is what
*Use* treats as food.

A member who ate gets **+5 HP**, but only if `NextSleep` is 0 **and**
`MaxHP ≥ HP + 5`. A member within 4 HP of maximum gets nothing. If at least one
member was healed, `NextSleep` is set to **12**. It counts down 1 per game hour, so
the meal heals again only after 12 hours. Food is eaten even when no healing
happens. **Resting heals nothing else**: no HP come back with time (`PassTime` has no
regeneration).

**Sleep.** Every member except NPC 11 takes the sleeping shape and gets a bedroll
(object 0xF6, frame 1) on its square. Members are not given the "asleep" status.
Then time passes as `hours × 3` calls of `PassTime(20)`, so status effects, poison,
torches and schedules all run during the rest.

**Waking.** Shapes come back, bedrolls are removed, and every member's movement
points are set to its DEX. On the surface, between 5:00 and 11:59, the dawn song
plays if music is on and the sound card is not type 3 (`C_2701_023F`).

### Experience and levels

**Experience comes from:**

- Killing a hostile monster with a hit. The attacker gets the XP that `LoseHP`
  returns through `__DoHit` (`AddXP(attacker, LoseHP(...))`, C_1810_3134). Only
  actors with the hostile bit (0x20) and a monster class give XP. The XP is
  (`COMBAT_Kill` C_1810_25AD):

  `XP = ( A × Damage + (HP + Armour) × Dex ) / 16` (integer division by a shift)

  Here A is the class's Str if the class has flag 0x20, else its Dex. All the
  values come from the class tables (`MonsterStr/Dex/HP/Armor/Damage`). A
  500-XP branch exists but its test is `OrigShapeType & 0`, which is never true.
  Kills made other ways give no XP: poison, terrain, explosions, and the
  instant-kill weapon type 1 (`__TryHit` drops `LoseHP`'s result).
- Conversation scripts: `OP_ADDEXP` (talkdr.c).

`AddXP` (C_1810_05F7) caps experience at **9999**, and so does `OP_ADDEXP`.

**Levels.** A character's level (`Level[]`) goes up only in the rest dream, or
through the script op `OP_ADDLVL`, which has no cap. It is **not** computed from XP.
`ComputeLevel` (C_1810_05D4) would give 1 + the number of halvings needed to bring
XP to 99 or less, but nothing in GAME.EXE calls it.

**The dream** (`REST_LevelUp` C_29D8_01F8). If `hours × 3 > PartySize`, then after
the i-th 20-minute step of the rest, party member i (for i < PartySize) is checked.
This includes NPC 11, who does not sleep. The member gains a level if
`ExpPoints ≥ expTable[Level]`, using the table built by the function's local
initialiser:

| Current level | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 |
|---|---|---|---|---|---|---|---|---|---|
| XP needed for the next level | 0 | 100 | 200 | 400 | 800 | 1600 | 3200 | 6400 | 32767 (never, since XP stops at 9999) |

So resting can take a character to **level 8** at most, with **one level per member
per rest**. A level above 8 (possible only through `OP_ADDLVL`) would index past the
end of the 9-entry table.

The dream text is:

> *Name* has a dream... He chases three animals through the jungle: a bear, a jaguar and a monkey. The path branches, each animal going a separate way. Does *Name* follow the bear, the jaguar or the monkey?

The pronoun is "She" only for NPC 2. The Avatar's sex (`AvatarFemale`) is not
consulted. Only the **first letter** of the typed answer counts. The input buffer
starts out holding "Winona Ryder".

| Answer starts with | Raises (by 1, not above 30) |
|---|---|
| B / b (bear) | STR |
| J / j (jaguar) | DEX |
| M / m (monkey) | INT |
| anything else | the highest of the three. With S, D, I: if S > D then (S > I ? STR : INT), else (D > I ? DEX : INT). Ties go to the later attribute: INT over both, DEX over STR. |

A new level raises maximum HP by 4 (see `MaxHP`). Current HP do not change.

### Character attributes

Each actor has base STR (`STREN`), DEX (`DEXTE`), INT (`INTEL`), HP (`HitPoints`,
0-255), `Level` and `ExpPoints`. The status panel shows the **base** values, HP
(green when poisoned, red below 10), "HM" (`MaxHP`), level and experience
(`STAT_ShowPortrait` C_0E13_0254). In the party list, HP are green when poisoned,
red below 10, and blue when "wild" (not for the Avatar) (`STAT_ShowParty`
C_0E13_000A).

Most rules use the **effective** values:

| Function | Rule |
|---|---|
| `GetStr` C_1810_04A0 | If cursed: STR − 3 (min 1), then, if also wild, + 3 (max 30). Wild alone does nothing, so wild only cancels the curse's penalty. |
| `GetDex` C_1810_04E0 | Cursed: − 3 (min 1). Wild: + 3 (max 30), cursed or not. Asleep: always 1. |
| `GetInt` C_1810_052A | − 3 (min 1, applied once) if cursed, or if wild and not NPC 1 (the Avatar) |
| `MaxHP` C_1810_062A | `2 × GetStr + 4 × Level`, at least 1 (if 0), at most 255. A curse lowers max HP by 6. |

Bounds and sources of change:

- The dream and the script ops `OP_ADDSTR`, `OP_ADDDEX` and `OP_ADDINT` cap
  attributes at **30**.
- New monsters from eggs roll their STR, DEX, INT and HP from their class
  (`mkRandom`) and start at **level 0**. Classless creatures get 10/10/10, 30 HP and
  level 1 (`EGG_generate` C_2667_015D).
- After loading, any actor with DEX 0 is given DEX 1 (`InitNPCTracker`
  C_130F_4EEF).

Where the attributes are used, within this section:

- **STR** sets carrying capacity and max HP. It is also the roll against webs and
  for shaking off paralysis. The source of `gameloop.c` spells this call `GetDex`,
  but the call goes to `GetStr` C_1810_04A0, as in Ultima VI.
- **DEX** sets movement points, and therefore how often an actor acts.
- **INT** is used by spell resistance and by the flee rule (`COMBAT_displaySeverity`,
  INT ≥ 5). Those are covered in the combat and magic sections.

Some actors cannot be hurt (`IsInvulnerable` C_1810_000D). `LoseHP` keeps them at
255 HP and they cannot be poisoned. They are actors whose original type is 0x11E,
0x11F or 0x126, and NPCs 0x0B, 0x19, 0x1F, 0x21, 0x23, 0x2F and 0x37.

**How HP come back** (none comes back with time):

| Source | Effect |
|---|---|
| Rest meal | +5 if allowed (above) |
| Bandage, type 0x86 (`UseBandage` C_1DBD_6036) | +1d5, up to max |
| Healing, type 0xFC (`UseHealing` C_1DBD_7DA2) | to max, and cures poison |
| Heal spell (`SPELL_Heal` C_11FF_04AC) | +1d10, up to max, and cures poison |
| Script `OP_HEAL` | to max |
| Revival (`SPELL_ReviveNPC`) | to max, cured, alignment good |

**Defeat.** If NPC 1 (the Avatar) is dead, "Overcome by your wounds..." plays
(`CheckActiveMember` C_130F_4943 → C_2701_0556), then C_08CB_0C73 runs. It revives
dead party members. Every surviving member then:

- is woken;
- has its alignment set to good;
- has cursed and charmed cleared, as well as paralysed, asleep, poisoned and dead;
- is set to full HP (`MaxHP`);
- **loses a quarter of its experience** (`XP × 3 / 4`);
- has its movement points set to its DEX.

Both spell timers are cleared. The party is moved to (0x19C, 0x123) on the surface
and the date jumps 7 days.

**Unconscious shape** (`SetTypeUnconscious` C_1810_0664). It is used for sleep and
death. An actor takes the body shape listed for its type in `DeadShape` (pairs at DS
0EA3). NPCs 0x0B, 0x1F, 0x2F and 0x37 and monster classes with flag 0x800 or 0x1000
have none, and vanish on death; class 0x1000 also flashes the screen when within 5
squares. Multi-square creatures (types 0x126, 0x11C, 0x121, 0x131, 0x12F, 0x129)
lose their extra parts and leave a multi-part body, with random blood when dead.
Classes with flag 0x2000 bleed (object 0xF8) when killed. The function returns 0
when the actor has no body shape.

### Carrying capacity and weight

Weights are kept in **tenths of a stone**. `TypeWeight[type]` is the weight of one
object. For the 17 types of `WeightPer10Types` (0x29, 0x36, 0x37, 0x38, 0x47, 0x40,
0x49, 0x4A, 0x7B, 0x81, 0x82, 0x86, 0xBF, 0x45, 0x78, 0x48, 0x30) it is the weight of
10 units (`IsWeightPer10Type` C_0E13_057F).

| Function | Rule |
|---|---|
| `GetTypeWeight` C_0E13_05FC | `TypeWeight × qty`, divided by 10 (integer) for the per-10 types |
| `GetWeight` C_0E13_05A6 | one object, without contents. qty = the 16-bit amount if the type's quantity kind is 4, the 8-bit quantity if it is 2, else 1 |
| `Encumbrance` C_0E13_0637 | total `GetWeight` of everything inside the object, at any depth |
| `STAT_ComputeWeights` C_0E13_0672 | `WeightEquip` = sum of `TypeWeight` of equipped items (one unit each, contents not counted). `WeightInven` = sum of `GetWeight + Encumbrance` of the other carried items. Also counts `InvenSize`. |
| `WeightToStones` C_0E13_0B63 | display only: tenths / 10, rounded to the nearest stone, but any weight below one stone that is not zero shows as 1 |

**The limit** is total weight (equipped + pack + the new object) of at most
**20 × STR tenths, i.e. 2 × STR stones**, using **base** STR. It is checked when
picking up ("The total is too heavy.", `CmdGet` C_1DBD_1FEA), when anything is
added to or moved into an actor's inventory (`AddInvObj` C_0A5D_102A, `InsertObj`
C_0A5D_1EDB), and so on. A dead body (0xF9) being picked up counts its own weight
only, without contents. An object of weight 0 (unless it is a per-10 type) or 255
cannot be taken.

The inventory panel shows two lines. "E:" is equipped weight / STR, and "I:" is
total weight / 2 × STR (`STAT_DrawInventory` C_0E13_0BA1). The equipped figure is
**not enforced**: `STAT_Ready` (C_0E13_12D4) reads STR but never compares it.

A pack also holds at most 16 non-stackable objects (`AddInvObj`/`InsertObj`).

### Movement points and turns

**The scheduler** (`RunNPCTurns` C_130F_4C0F) runs after every player command. Each
actor has `MovePts` (a signed byte). Repeatedly, among the actors on the party's
level inside the loaded 40×40 area that can act, it picks the one with the highest
`MovePts / DEX` ratio. Actors that cannot act are those that are motionless (mode 0),
paralysed, asleep (unless in schedule mode 0x80), dead, inside a container or
inventory, or have `MovePts ≤ 0`. An actor with `MovePts ≥ DEX` is taken at once.
Ties go to the higher DEX. Followers (mode 1) are skipped, because they move with
the leader. While any follower has `MovePts ≤ 0`, `Party[0]` gets 0 points, so the
leader waits for the slowest follower.

When nobody has points left, every actor 0..0xFF gets its **DEX** in points. A
negative balance is carried over (`MovePts + DEX`); otherwise the balance is set to
DEX, so unspent points are not saved. Then **1 minute passes**. The loop ends when
the player's commanded character (mode 2) is picked, and the game waits for a key.
Actors that walk out of the area are frozen with 0 points. Scheduled walkers are
teleported or re-pathed.

**Costs** (`SubMov` C_130F_1DCC). Every cost is reduced by 2 for actors 0..0x0F
**out of combat**, but is at least 1. The numbers below are before that reduction.
`SubTerrainMov` (C_130F_3766) charges `5 + (TerrainType >> 4)` of the base tile,
plus the same upper nibble for every object tile on the square, skipping impassable
tiles. It is charged on the square the actor is **leaving**, because it is applied
before the move.

| Action | Cost | Where |
|---|---|---|
| Leader step, straight | `SubTerrainMov` (5 + terrain) | `PartyAdvance` C_130F_24C3 |
| Leader step, diagonal | the same + 5, charged as a separate `SubMov` and so reduced separately | `PartyAdvance` |
| Follower step (any terrain) | 5 | `MoveFollowers` C_130F_1EC7 |
| NPC step (`TryStraightMove`) | `SubTerrainMov`, paid even when blocked | C_130F_384C |
| Attack | 10 | `COMBAT_attack` C_1810_3B65 |
| Get | 3, plus 3 for putting it in the pack | `CmdGet`, `AddInvObj`/`InsertObj` |
| Drop/throw | 3 | `CmdDrop` C_1DBD_1960 |
| Any object put into an actor's inventory | 3 (to the receiver) | `AddInvObj`, `InsertObj` |
| Move (push) an object | 5 | `CmdMove` C_1DBD_26E8 |
| Use | 5 | `CmdUse` C_1DBD_8276 |
| Pass (space/Esc) | all remaining points (set to 0); followers regroup | `GameLoop` C_0359_1A65 |
| NPC loitering, waiting, standing | 5 | `LoiterNear` C_130F_3930, `NPC_TakeTurn` |
| An NPC turn that spent nothing | 10 | `NPC_TakeTurn` C_130F_4121 |
| Teleport to schedule | 10 | `TeleportToSchedule` C_130F_2F03 |

So a character's speed is its DEX divided by what it does. Out of combat the party
pays less: a straight step on cost-0 terrain is 3 points instead of 5. A lower-DEX
follower also slows the leader. After resting, waking from defeat, boarding the
raft or being created, `MovePts` is set to DEX.

### Harmful terrain

**When it applies.** On each `PassTime` call, every actor 0..0xFF standing on a
damaging square is affected once (`ApplyTerrainEffects` C_0359_1014). Actors are
also affected on entering one, through `MoveObj` (C_0A5D_1C76) whenever
`MoveAppliesTerrain` is set. Teleports to schedules and some map transitions turn it
off. A flag (`NPCFlag 8`) stops a second application in the same call.

**Skipped:** type 0xE9 (the beached raft), monster classes with both flags 0x100 and
0x400, actors on another level, and actors outside the loaded 40×40 area.

**What counts as damaging.** The objects on the square are checked first, in order:

- An object whose tile has `TileFlag2 & 4` makes the square safe and stops the check
  (it covers the ground).
- An object whose tile has `TerrainType & 8`, or an object of type 0x124 (a trap),
  applies its damage. The object is also revealed (`ObjStatus &= ~2`).

If no object decided, the map tile applies if `TerrainType & 8`.

**Damage** (`TerrainDamage` C_0359_0DA8). Dead actors are unaffected.
"Fire-immune" means `NPCStatus & 1` (protected) or monster class flag 0x08.
"Fire-sensitive" means monster class flag 0x10 (damage doubled).

| Tile | Effect |
|---|---|
| Lava: tiles 0x0F-0x1B, 0xAA, 0xAB (`IsLavaTile` C_1DBD_0A4C) | 1d40, ×2 for fire-sensitive. Fire-immune take none. |
| 0x383, 0x406, 0x42B-0x42F, 0x434-0x437 | 1d15, ×2 for fire-sensitive. Fire-immune take none. |
| 0x54A-0x54F | 1d5. Protected actors and type 0x11A take none. |
| 0x3A7 | 1d7, only to creature types 0x143 and 0x148 (protection does not help) |
| Webs 0x210-0x219 | not protected and not type 0x150 (the spider): if `GetStr < 1d30`, the actor is **paralysed** ("stuck in a web"). It frees itself by the per-minute roll above. |

Damage goes through `LoseHP` (C_1810_289B), so it can kill. It gives no experience.
Party followers avoid damaging squares when they can: a hazardous square scores 128
instead of 256 when they pick their step, and is refused outright if they were in
contact with the chain (`MoveFollowers` C_130F_1EC7). NPCs walking a path avoid them
too (`TryStraightMove` with `avoidHazard`).

### Uncertain or not determined

- The names of most object types and NPC numbers (food types, NPC 2, 0x19, 0x23)
  come only from code comments and messages. GAME.EXE has no name table to use.
- What object type 0x124 and the 0x3A7 tile are in the game world is not stated in
  the code.
- How `D_3298 & 0x80` (the WAIT flag) is set in normal play was not traced in this
  section.
- `REST_FoesNear` counts every actor with the hostile bit and a shape. Whether dead
  hostile NPCs keep that bit and shape (and so block rest) depends on
  `OBJ_MakeDeadBody`, which was not checked here.

## Combat

This section covers combat mode, the Attack command, how an attack is resolved
(hit, damage, armour, wounds, poison, death, experience), weapons, ranged weapons and
ammunition, explosives, boulders, the creatures made of several tiles or of segments,
the combat AI of party members and monsters, and the creatures that eggs spawn. Almost
all of it is in `SRC/combat.c` (segment 1810); the monster tables are in `SRC/monsdata.c`,
creature generation in `SRC/egg.c`, the AI dispatcher and turn scheduler in
`SRC/npctrack.c`. Random rolls use `OSI_rand(lo, hi)`, which returns a number from lo to
hi, both included; "1dN" below means `OSI_rand(1, N)`.

**About names.** GAME.EXE does not contain object names (they are in the game's look
data, which is not in `gamedata/`). Names given below in *italics* come from an outside
source, Nuvie's `data/scripts/se/actor.lua` and `usecode.lua` (the Nuvie/ScummVM engine
reimplementation), not from the game files. They agree with the strings GAME.EXE prints
where there are any ("Out of arrows!" for the bow's ammunition, "Flintlock not loaded!",
"web" for type 0x11, "with turtle" for type 0x133, "gunpowder" for 0xCC). Names without
italics come from strings in GAME.EXE. Where no name is known the type number is given.

### Entering and leaving combat mode

`InCombat` is the flag. The combat icon (tiles 0x177/0x178) or the peace icon (tiles
0x173/0x174) is drawn at the top of the screen.

Combat begins:

- with the combat command ("Begin combat!"), only in party mode ("Not in solo mode!")
  and not in a vehicle ("Not while aboard ship!") (`GameLoop` C_0359_1A65, command 0x89);
- automatically, when an attacker whose alignment has bit 0x20 (evil or chaotic) does
  damage (after armour) to a player-controlled actor, out of combat, in party mode
  (`__DoHit` C_1810_3134 calls `COMBAT_beginAttacked` C_1810_59C8);
- in one scripted cut scene (objcmd.c, the scene that ends with `COMBAT_begin`).

On entering (`COMBAT_begin` C_1810_012A, `COMBAT_beginAttacked` C_1810_59C8): every party
member that is not charmed takes its combat mode (`NPCMode = NPCComMode`) and becomes its
own leader. The controlled member (in party mode the first one that is not asleep,
paralysed, dead or dragged under; in solo mode the solo member) gets mode 2 (command).
`CombatOutcome` is reset (only in `COMBAT_begin`). The two functions differ only in the
music: `COMBAT_begin` plays jingle 6, `COMBAT_beginAttacked` jingle 7; both then queue
song 8 (`C_2701_04D7`, `C_2701_02D6`).

Combat ends:

- with the same command again ("Break off combat!");
- when the party boards a vehicle (`Board` C_130F_21CB), takes an entrance (map.c), or a
  game is restored (`GAME_Restore` C_05D0_03A2);
- when the party is knocked out (`C_08CB_0C73` clears `InCombat`).

There is no automatic end when the enemies are gone. On leaving (`COMBAT_breakOff`
C_1810_023B): dead members get mode 0 (motionless); the others that are not charmed
follow the leader again (party mode) or stand still (solo mode); the controlled member
gets mode 2; the music plays jingle 11 if a party member died during the fight, jingle 12
if a hostile actor died, else fades back to the area song (`C_2701_0355`); one minute
passes (`PassTime(1)`).

While in combat (each rule cites its function):

| Rule | Where |
|---|---|
| No switching to party mode or solo mode, no saving, no resting ("Not in combat mode!") | `GameLoop` C_0359_1A65, rest.c |
| A player-controlled actor cannot step to a tile more than 8 tiles (either axis) from the screen centre (`MapX`,`MapY`) | `TryStraightMove` C_130F_384C |
| Party members out of combat pay 2 movement points less for everything (minimum 1); in combat they pay full price | `SubMov` C_130F_1DCC |
| The "wild" effect (ObjStatus 0x80) lasts only while in combat; out of combat it is cleared every minute | `PassTime` C_0359_11B6 |
| The Motazz invocation works only in combat | spell.c |
| Each turn, while `CombatOutcome` is ±1, it is doubled and the music queues song 9 (party loss) or 10 (enemy killed) | `GameLoop`, `C_2701_045E` |

### Combat modes of party members

Each actor has a combat mode `NPCComMode`, used as its AI mode (`NPCMode`) during combat.
The status panel shows it with these names (`ComModeNames` DS 08A5, `STAT_refreshComMode`
C_0E13_016E):

| NPCComMode | Panel name | AI in combat |
|---|---|---|
| 2 | CMND | the player controls the member (the turn scheduler stops on it) |
| 4 | RANGE | `COMBAT_AI_Range` |
| 6 | WILD | `COMBAT_AI_Berserk` |
| 7 | FLEE | `COMBAT_AI_Retreat` |
| 8 | CLOSE | `COMBAT_AI_Assault` |

Clicking the combat-mode button (screen 248-263, y >= 102) in the equipment view cycles
CMND -> RANGE -> FLEE -> CLOSE -> CMND (values 3 and 5 are skipped, 6 is never reached;
`MouseInEquipView` C_05D0_12B1). It is not available for the first member (StatusMember
0) nor for a member with ObjStatus 0x80. In combat the new mode applies at once, unless the
member is the active one or charmed. A member with ObjStatus 0x80 ("wild", from the wild
spell) shows "WILD" (except object 1) and uses `COMBAT_AI_Berserk` whatever its mode
(`STAT_refreshComMode`, `NPC_TakeTurn` C_130F_4121); `SetPartyMode` also gives it mode 6 in
combat.

Defaults: an NPC that joins the party gets CLOSE (talkdr.c, join function). After a restore,
unless game flag D_6F1D bit 0 is set, NPCs 1-15 get full hit points and NPC 1 gets CMND,
NPCs 7 and 9 get RANGE, the others CLOSE (`OBJ_RelinkNPCs` C_0A5D_390D). Generated
monsters get mode 8 (`EGG_generate` C_2667_015D).

### Turn cost

Each round every actor receives its DEX in movement points (debts carry over) and one
minute passes; the actor with the highest points/DEX ratio acts next (`RunNPCTurns`
C_130F_4C0F). An attack costs 10 points: the player's Attack command (`COMBAT_attack`
C_1810_3B65) and every AI attack (`SubMov(npc, 10)`). An actor whose AI spent nothing loses
10 (`NPC_TakeTurn`). Waiting in the Like/Unfriendly/Immobile modes costs 5; Berserk costs
5 more after whatever it did.

### The Attack command

`COMBAT_attack` (C_1810_3B65):

- Not possible in a vehicle ("Attack-Not while aboard ship!").
- The attacker's readied weapons are listed: every equipped object (ObjStatus position
  0x18) for which `GetDamagePoints` is not -1, at most 8 (`COMBAT_InitWeaponList`
  C_1810_3840). With none, "with bare hands-". An actor of type 0x133 (*party member on
  turtle*) attacks "with turtle".
- The player picks a target for each weapon in turn ("Attack with <weapon>-"). The
  target cursor is limited to the weapon's range (7 for type 0x15 whatever its quality).
  All the attacks of one command cost 10 movement points in total, so two readied
  weapons give two attacks for the price of one (`COMBAT_AttackWithNextWeapon`
  C_1810_38E7).
- Before each attack, the ammunition is checked and the attack skipped with "Out of
  arrows!", "Out of ammunition!", "Out of darts!", "Out of spears!" or "Flintlock not
  loaded!" (see the weapon table). Targeting oneself prints "pass." and does nothing.
  An empty tile prints its terrain name, or "nothing!" in the dark.
- If the target is farther than the weapon's range: "Out of range!". Attacking a web
  (0x11) with anything but a lit torch (0xD1) or a grenade (0x89/0x8A), or object 0x107
  with anything but a grenade or a fire axe (0x76): "Not possible." (`__TryHit`
  C_1810_337D).

### Resolving an attack

`__TryHit` (C_1810_337D) does every attack, the player's and the AI's:

1. An actor of type 0x133 always attacks with itself (its natural attack).
2. Type 0x15 (*black staff*) gets quality 1 if the selected target is type 0x143 or 0x148
   (*Myrmidex*), else quality 0.
3. Range check (`COMBAT_AttackRange` > weapon range: returns "out of range") and the
   target restrictions above.
4. Ammunition: without it the attack silently does nothing (AI attackers still pay
   10 points). An unloaded flintlock (0x32, frame 0) is loaded instead of fired
   (`LoadFlintlock` C_1DBD_4297).
5. The attack is ranged if the target is farther than 1, or the weapon is one of 0x1F,
   0x20, 0x25, 0x27, 0x28, 0x32, 0x89, 0x8A, 0xF0, 0xF1, or 0x15 with quality 1. A ranged
   attack flies a missile (`COMBAT_FireRangedWeapon` C_1810_1A5A). If the missile stops
   before the target tile (a tile or object that blocks missiles, or out of range), the
   hit goes to whatever `COMBAT_GetTargetAt` finds on the tile where it stopped, possibly
   nothing (`Missiles` C_276B_0390).
6. The to-hit test is computed (`__AttackTest`, below), but **its result is never used**:
   the code tests `didHit != -1` where the value can only be 0 or 1. So every attack whose
   target (or missile) reaches an object other than the attacker lands. This is a
   change from Ultima VI, whose `__TryHit` tests `didHit` itself. Checked in the machine
   code (1810:3611 `cmp word [bp-6],-1`).
7. Special weapon effects (below), else `__DoHit` with the weapon's damage.
8. A boomerang (0x27) then flies back to the attacker (a returning missile, range 100).

A melee attack (not ranged) plays sound 0x1F when the attacker is within 5 tiles of the
screen centre.

#### The to-hit test (computed but ignored)

`__AttackTest` (C_1810_2E13), for the record and for modders who want to restore it:

- Against an object that is not an actor: always 1.
- *Spear of Shamap* (type 1) against 0x126 (*tyrannosaurus*), and a lit torch (0xD1)
  against a web (0x11): always 1.
- Type 0x15 against a Myrmidex: 0 one time in 8, else 1.
- Otherwise the attacker's stat is STR if (natural attack) the attacker's monster class
  has flag 0x20, or (weapon) the weapon is 0x18, 0x1D, 0x21, 0x22, 0x5E, 0x76 or 0x77;
  else DEX (`GetStr`/`GetDex`). The defender's DEX is used, or 20 for types 0x143/0x148.
- Hit when `1d30 >= (defenderDEX + 30 - attackerStat) / 2` (integer division).

The rolls are still made, so they still consume random numbers.

#### Damage

`__DoHit` (C_1810_3134):

1. Maximum damage M comes from `GetDamagePoints` (C_1810_1F3D): for an actor attacking
   with itself, its class's `MonsterDamage`; for type 0x15 with quality 1, 40; else the
   weapon table below. An object that is not a weapon (-1) and an actor without a monster
   class do 1 (`__TryHit` turns -1 into 1).
2. If 1 < M < 255 the damage is `1dM`. M = 255 would be a sure hit of 255 with no
   armour roll; no weapon or class has 255.
3. If the defender is an actor with protection P > 0 (and damage < 255), the damage is
   reduced by `1dP`.
4. Damage 0 or less: "<name> grazed." and nothing else happens to hit points.
   Otherwise `LoseHP(defender, damage)`, and the experience it returns goes to the
   attacker (`AddXP`).
5. If the defender is the selected object, `COMBAT_ProvokeNeutral` runs (below).
6. If the defender is now at 0 hit points (and is not the vehicle 0xED), stop.
7. Poison: if the attacker's monster class has flag 0x40, one time in 4 the defender is
   poisoned ("<name> poisoned!"), unless it is protected (NPCStatus bit 0) or
   invulnerable. This test runs even after a graze, and whatever weapon the attacker used.
8. If the defender is alive, took damage, and is not a vine part (0x124/0x11A), the
   wound message is printed (`COMBAT_displaySeverity`).

STR adds nothing to damage. There are no critical hits: "critical!" is a wound level.

#### Weapons

The weapon tables are `WeaponTypes`/`WeaponDamages` (DS 105E/1094, 27 entries) and
`RangedWeaponTypes`/`WeaponRanges` (DS 0F56/0F72, 14 entries); any type not in the range
table has range 1 (`__GetWeaponRange` C_1810_0EC7).

| Type | Name | Max damage | Range | To-hit stat | Ammunition / notes |
|---|---|---|---|---|---|
| 0x01 | *spear of Shamap* | 10 | 5 | DEX | special, see below |
| 0x15 | *black staff* | 5 (40 vs Myrmidex) | 1 (7 vs Myrmidex) | DEX | missile tile 0x10A vs Myrmidex |
| 0x18 | *club* | 7 | 1 | STR | |
| 0x19 | *obsidian knife* | 5 | 1 | DEX | |
| 0x1A | *spear* | 10 | 6 | DEX | thrown |
| 0x1B | *throwing axe* | 6 | 4 | DEX | thrown |
| 0x1C | *axe* | 8 | 1 | DEX | |
| 0x1D | *rock hammer* | 6 | 1 | STR | |
| 0x1F | *bow* | 8 | 6 | DEX | arrows (0x2D) |
| 0x20 | *blowgun* | 2 | 4 | DEX | darts (0x24); poisons, see below |
| 0x21 | *obsidian sword* | 12 | 1 | STR | |
| 0x22 | *two-handed hammer* | 11 | 1 | STR | |
| 0x25 | *atlatl* | 14 | 7 | DEX | spears (0x1A) |
| 0x27 | *boomerang* | 4 | 4 | DEX | comes back |
| 0x28 | *modern rifle* | 35 | 8 | DEX | bullets (0x29), "ammunition" |
| 0x2C | *knife* | 7 | 4 | DEX | thrown |
| 0x32 | *bamboo flintlock* | 25 | 7 | DEX | must be loaded with gunpowder (0xCC) |
| 0x5E | *fire extinguisher* | 6 | 1 | STR | |
| 0x76 | *fire axe* | 15 | 1 | STR | can break object 0x107 |
| 0x77 | *metal hammer* | 6 | 1 | STR | |
| 0x80 | *bamboo pole* | 3 | 1 | DEX | |
| 0x89 | *grenade* | 15 | 6 | DEX | explodes, see Explosives |
| 0x8A | *lit grenade* | 15 | 6 | DEX | explodes |
| 0xAF | (unknown) | 3 | 1 | DEX | |
| 0xD1 | *lit torch* | 5 | 1 | DEX | sets webs on fire |
| 0xF0 | *device* | 3 | 6 | DEX | explodes, hurts only Myrmidex |
| 0xF1 | *activated device* | 3 | 6 | DEX | explodes, hurts only Myrmidex |

The "to-hit stat" column only matters if the to-hit test is restored (see above).

#### Ranged weapons and ammunition

`COMBAT_FireRangedWeapon` (C_1810_1A5A), after the missile flies:

- **Bow, blowgun, rifle**: one arrow, dart or bullet is used (a stack loses 1; the last one
  is deleted). The ammunition must be carried, not readied, anywhere in the attacker's
  inventory, containers included (`C_0A5D_080A`).
- **Atlatl**: uses a spear. If the spear object is a stack of more than one, one is
  removed and is lost. If it is a single object, it flies and lands on the impact tile,
  or is lost if that tile is water, lava or a pit with nothing on it (`C_1DBD_18FD`).
- **Flintlock**: each shot unloads it (frame back to 0). Loading takes one gunpowder
  (0xCC) from the holder's inventory and sets frame 1 ("Flintlock loaded.", or "Flintlock
  already loaded.", "You don't have any gunpowder."); it is done with Use, or by an AI
  attacker instead of firing (`LoadFlintlock` C_1DBD_4297).
- **Thrown weapons** (spear of Shamap, spear, throwing axe, knife), when the attack is
  ranged: a spare of the same type carried (not readied) replaces the thrown one in the
  same hand slot (2, 5 or 8). The thrown weapon lands on the impact tile if it hit
  something; if it hit nothing it lands there too, unless the tile is water, lava or a
  pit, where it is lost. The code means to throw only beyond 1 tile, but it tests the
  target's map coordinates (`dest_x > 1 || dest_y > 1`) instead of the distance, so in
  practice every ranged use throws the weapon; adjacent attacks are melee and keep it.
- **Grenades and devices** are used up when fired: they explode at the impact tile and a
  spare (0x89 for either grenade, 0xF0 for either device) is readied in their place.
- **Boomerang**: comes back; nothing is used up.

#### Range

The "combat distance" between two tiles is `RangeTable[|dx|][|dy|]` (DS 0DE6), roughly
`max + min/2`; if either axis is 8 or more the distance is 9 (`COMBAT_range` C_1810_3CC2):

| dy \ dx | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| 1 | 1 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |
| 2 | 2 | 2 | 2 | 3 | 4 | 5 | 6 | 7 |
| 3 | 3 | 3 | 3 | 4 | 5 | 6 | 7 | 7 |
| 4 | 4 | 4 | 4 | 5 | 6 | 7 | 7 | 8 |
| 5 | 5 | 5 | 5 | 6 | 7 | 7 | 8 | 8 |
| 6 | 6 | 6 | 6 | 7 | 7 | 8 | 8 | 8 |
| 7 | 7 | 7 | 7 | 7 | 8 | 8 | 8 | 8 |

(The table is symmetric.) `COMBAT_AttackRange` (C_1810_3D13) uses this for ordinary
(one-tile) attackers. For the creatures made of several tiles the distance is 1 when the
target tile lies in a box in front of the creature, in the direction it faces (derived
from its frame), else 9: they can only attack what touches their head end. Attackers of a
type in none of the lists (for example 0x11E/0x11F, *apatosaurus*) always get 9 and can
never attack.

| Kind (npctrack.c) | Types | Facing from frame |
|---|---|---|
| one tile (`C_130F_0136`) | 0x11A, 0x123, 0x124, 0x12B, 0x12D, 0x12E, 0x133-0x146, 0x150 and up | (normal range) |
| two tiles (`C_130F_0002`) | 0x11B, 0x12C, 0x147, 0x14A-0x14F | frame/4 |
| 0x120 *pteranodon* (`C_130F_004D`) | 0x120 | frame/6 |
| three tiles (`C_130F_0070`) | 0x11C, 0x121, 0x129, 0x12F, 0x131 | frame/6 for 0x121, else frame/4 |
| 0x128 *gorilla* (`C_130F_00A7`) | 0x128 | frame/8 |
| 0x126 *tyrannosaurus* (`C_130F_00CA`) | 0x126 | frame/6 |

A part of a multi-tile creature holds its owner's number in its quality; attacking a part
attacks the owner (`COMBAT_GetTargetAt` C_1810_0D5B, `COMBAT_getHead` C_1810_3329).

#### Special weapon and target pairs

All in `__TryHit` (C_1810_337D), applied when the attack lands:

| Weapon | Target | Effect |
|---|---|---|
| *spear of Shamap* (1) | any actor but 0x126 | killed outright (`LoseHP` with all its hit points); no experience |
| *spear of Shamap* (1) | 0x126 *tyrannosaurus* | falls asleep (body shape); hitting it does not wake it (`COMBAT_showHit`) |
| *spear of Shamap* (1) | an object | normal hit |
| (any of the above) | | the spear of Shamap then becomes an ordinary spear (type 0x1A, quantity 1) |
| *blowgun* (0x20) | an actor | one time in 2, poisoned and no damage (unless protected or invulnerable); otherwise a normal hit |
| lit torch (0xD1) | web (0x11) | the web catches fire (fire object 0xF2, `C_1DBD_4F8E`) |
| *black staff* (0x15) | Myrmidex (0x143/0x148) | range 7, damage 40, flies as a missile |

### Armour

Protection is the sum of the armour values of the actor's equipped objects, -3 if it is
cursed (ObjStatus 0x40), +3 if it is protected (NPCStatus bit 0) (`__GetProtection`
C_1810_0E74, `GetArmorPoints` C_1810_0E39, tables `ArmorTypes`/`ArmorValues` DS 0F3E/0F4E).
Protection 0 or less reduces nothing (it never adds damage).

| Type | Name | Protection |
|---|---|---|
| 0x02 | *shield of Krukk* | 4 |
| 0x07 | *cloth armour* | 1 |
| 0x08 | *leather armour* | 3 |
| 0x09 | *bark armour* | 2 |
| 0x0D | *bark shield* | 1 |
| 0x0E | *leather shield* | 2 |
| 0x0F | *stegosaurus shield* | 3 |
| 0x43 | *kotl shield* | 7 |

**The monster classes' armour (`MonsterArmor`) protects nothing**: no combat code reads it.
It is used only in the experience formula. A monster is protected only by armour objects
it wears (for example the bark armour of type 0x140, see Eggs).

### Derived attributes

| Value | Rule | Where |
|---|---|---|
| Effective STR | STR; if cursed: -3 (minimum 1), and then, only if also wild (ObjStatus 0x80), +3 (maximum 30). A wild actor that is not cursed gets no bonus. | `GetStr` C_1810_04A0 |
| Effective DEX | DEX; -3 if cursed (min 1); +3 if wild (max 30); 1 if asleep | `GetDex` C_1810_04E0 |
| Effective INT | INT; -3 (min 1) if cursed, or if wild and not object 1 | `GetInt` C_1810_052A |
| Max HP | `2*STR + 4*level` (effective STR), 1 if that is 0, at most 255 | `MaxHP` C_1810_062A |
| Level (`ComputeLevel` is never called: levels rise only in the rest dream) | 1 + the number of halvings needed to bring XP to 99 or less: 0-99 L1, 100-199 L2, 200-399 L3, 400-799 L4, 800-1599 L5, 1600-3199 L6, 3200-6399 L7, 6400-9999 L8 | `ComputeLevel` C_1810_05D4 |
| XP gain | added, capped at 9999 | `AddXP` C_1810_05F7 |
| Spell resistance | target resists if `INT(target) - INT(caster) > OSI_rand(-12, 12)`; type 0x143 resists 1 time in 10, type 0x148 never | `COMBAT_ResistsSpell` C_1810_0562 |

Monsters made by eggs have level 0, so their Max HP (used for wound messages and healing
while fleeing) is `2*STR`, not their class hit points.

### Wound messages

After a damaging hit (and for the vehicle's passenger, and for actors burnt by a brazier),
`COMBAT_displaySeverity` (C_1810_2F42) computes `severity = HP*4 / MaxHP` (integer):

| Severity | Message |
|---|---|
| 0 | "<Name> critical!" |
| 1 | "<Name> heavily wounded." |
| 2 | "<Name> lightly wounded." |
| 3 | "<Name> barely wounded." |
| 4 or more | nothing |

Nothing is printed at 0 HP or for the vine parts 0x124/0x11A. A "critical" actor with INT
5 or more flees (mode 7), unless its mode is 0, 1, 2, 6 or 0x0E. So command-mode members,
followers and wild/berserk actors never flee on their own.

### Losing hit points

`LoseHP(obj, damage)` (C_1810_289B) is used by every source of damage (attacks,
explosions, poison, terrain, boulders). Nothing happens if the object is on another
level or damage is negative. Then:

| Case | Effect |
|---|---|
| type 0x124 or 0x11A | handled as a segmented creature (below) |
| type 0x148 | the damage goes to actor 0x36 instead |
| type 0xED (the vehicle being ridden) | a random party member takes the damage, and its wound message is printed |
| invulnerable actor | hit points set to 255, nothing else (no hit flash) |
| actor, HP <= damage | hit flash, `COMBAT_Kill` |
| actor, HP > damage | hit flash, HP -= damage; an actor in mode 9 switches to mode 8 (assault) |
| object (not stackable, quantity > 0) | its quantity is its hit points; at 0 it is destroyed, see below |

Invulnerable (`IsInvulnerable` C_1810_000D): actors whose original type is 0x126
(*tyrannosaurus*), 0x11E or 0x11F (*apatosaurus*), and NPCs 0x0B, 0x19, 0x1F, 0x21, 0x23,
0x2F, 0x37. They also cannot be poisoned by attacks.

The hit flash (`COMBAT_showHit` C_1810_2081) shows tile 0x101 over the target, plays sound
7 (non-player) or 8 (player-controlled) and wakes a sleeping actor (except original type
0x126).

When a party member is hurt, its hit points are redrawn in the party list: green if
poisoned, red under 10.

Destroyed objects with effects:

| Type | Effect when destroyed |
|---|---|
| 0x7F (*bamboo plant*) | replaced by 0x80 (*bamboo pole*) |
| 0x107 | parts 0x108, 0x109, 0x10A removed, rubble objects placed, and the actors in a 2x5 area north of it are crushed (`C_1810_219F`): each is pushed to a free neighbouring tile and takes 2d10, or loses all its hit points if there is no room. |
| 0xDE | explosion (tile 0x106): every actor in the blast takes 1d30; all objects of type 0x100 get tile 0xDF; cut scene 3 plays unless TalkFlags[1] bit 2 is set |
| 0x93 | three double explosions, then the last cut scene (the end of the game, `C_1DBD_077E`) |
| others | deleted |

### Death

When an actor's hit points reach 0 (`COMBAT_Kill` C_1810_25AD):

- Experience is computed (below), hit points set to 0, the actor marked dead, made
  visible, its AI mode set to 0.
- "<Name> killed!" (objects 0x10 and up) or "<Name> falls!" (0-0x0F), not for type
  0x11A and not during a conversation.
- In combat, `CombatOutcome` becomes -1 if a player-controlled actor died, else 1 if a
  hostile (bit 0x20) actor died and nothing worse happened yet.
- A dead player-controlled actor other than `Party[0]` is removed from the party list.
- It gets its dead-body shape (`SetTypeUnconscious` C_1810_0664) and its body object
  (`OBJ_MakeDeadBody`). An actor with no body shape is moved off the map (level 6) and, if
  it is a temporary monster slot (0xC0-0xFF), freed. When actor 0x36 vanishes, the 0x148
  object on its tile is deleted.
- Plot flags: killing actor 0x17 sets TalkFlags[0x35] bit 0x20; killing actor 0xB4 sets
  TalkFlags[0x30] bit 0x80.
- If the Avatar (object 1) is dead, the party is knocked out ("Overcome by your wounds, you
  fall unconscious....."), and wakes in Intanya's hut: dead members are revived, everyone
  healed to Max HP, cured, made good, and every member's XP is multiplied by 3/4
  (`CheckActiveMember` C_130F_4943, `C_2701_0556`, `C_08CB_0C73`).

There is no unconscious state between "hurt" and "dead" for single actors; "unconscious"
in the code is the body shape used both for sleep and death.

**Body shapes** (`DeadShape`, DS 0EA3; type -> body type/frame): human types 0x136-0x13C,
0x13E, 0x140-0x146 and 0x13D give a body of type 0xF9 (frames 0-15 in that order: 0x136/0,
0x137/1, 0x138/2, 0x139/3, 0x13A/4, 0x13B/5, 0x13C/6, 0x13E/7, 0x140/8, 0x141/9, 0x142/10,
0x143/11, 0x144/12, 0x145/13, 0x146/14, 0x13D/15). Animals give type 0x118: 0x11B/1,
0x11C/3, 0x120/6, 0x121/8, 0x126/13, 0x128/21, 0x129/23, 0x12D/25, 0x12F/27, 0x131/30,
0x147/33, 0x14A/35, 0x14B/37, 0x14C/39, 0x14D/41, 0x14E/43, 0x14F/45, 0x150/46, 0x151/47.
Type 0x12B gives type 0x6B frame 0. Every other type, NPCs 0x0B, 0x1F, 0x2F and 0x37, and
monster classes with flag 0x800 or 0x1000 leave no body and vanish. The big creatures
(0x126 and the three-tile kind) leave a body made of several objects, with blood (0xF8)
on some tiles when dead. Classes with flag 0x2000 bleed: a blood object 0xF8 (frame 0-2)
is left on the tile (actors 0x10 and up, not the big creatures). Flag 0x1000 also redraws
the map with the fade effect when within 5 tiles (`C_0359_0D58(1)`). A dead 0x133 (member
on the turtle) first gets off the turtle.

### Experience

Only a kill made through a normal hit (`__DoHit` -> `LoseHP` -> `COMBAT_Kill`) gives
experience, and only to the attacker. Kills by explosions, boulders, crushing, the spear of
Shamap, poison or terrain give nothing: those callers drop the value.

A killed actor gives experience only if its alignment has bit 0x20 (evil or chaotic) when
it dies and its type has a monster class. With the class's table values (not the
individual's rolled stats):

    xp = ( A * Damage + (HP + Armor) * DEX ) / 16      (integer, shift right 4)
    A  = STR if the class has flag 0x20, else DEX

(`COMBAT_Kill` C_1810_25AD. The code also has `xp = 500` under a test that is always false,
`OrigShapeType & 0`.) A neutral creature becomes chaotic when a good actor hits it (see
Provoking neutrals), so provoked animals give experience. The segmented creature gives a
fixed 10 when its last vine dies (below).

### Poison

- Attacks: classes with flag 0x40 poison one time in 4 on any blow that leaves the defender
  alive (`__DoHit`); the blowgun one time in 2 (`__TryHit`). Protected (NPCStatus bit 0)
  and invulnerable actors are immune.
- Effect: each minute, a poisoned actor that is not protected and is not NPC 0x19 loses 1
  hit point one time in 8 (`PassTime` C_0359_11B6). Poison does not wear off with time;
  it is cleared only by healing (the heal spell, a healing item in objcmd.c, a
  conversation, or the party's knock-out).

### Provoking neutrals

When the selected target of an attack is a neutral actor not under player control
(`COMBAT_ProvokeNeutral` C_1810_304D):

- If the attacker is in command mode (the player), the target's class has flag 0x4000 and
  the blow did damage: "<Name> attacks!".
- A named NPC (below 0xC0) on its schedule (mode 0x80 or higher): if the player attacked,
  it switches to mode 0x9B (assault); nothing else.
- Otherwise, if the attacker is good and the target is not evil, the target becomes
  chaotic. A temporary monster (0xC0 and up) switches to mode 8 (assault).

### Monster classes

The monster class of a type is its position in `MonsterTypes`; the other tables are indexed
by that position (`GetMonsterClass` C_2667_004F returns position - 1; the code adds 1).
Class 0 is unused. Alignment: 0x00 neutral, 0x20 evil (attacks the party), 0x40 good,
0x60 chaotic. The "XP" column is the formula above (0 for classes that are not hostile
unless provoked into chaos).

| Class | Type | Name | STR | DEX | INT | Armor | Damage | HP | Alignment | Flags | XP |
|---|---|---|---|---|---|---|---|---|---|---|---|
| 1 | 0x136 | *modern man* | 20 | 20 | 20 | 0 | 3 | 10 | good | 0x6000 | 16 |
| 2 | 0x137 | *professor* | 20 | 20 | 20 | 0 | 3 | 10 | good | 0x6000 | 16 |
| 3 | 0x138 | *reporter* | 20 | 20 | 20 | 0 | 3 | 10 | good | 0x6000 | 16 |
| 4 | 0x139 | *woman* | 20 | 20 | 20 | 0 | 3 | 10 | neutral | 0x6000 | 16 |
| 5 | 0x13A | *shaman* | 20 | 20 | 20 | 0 | 3 | 10 | neutral | 0x6000 | 16 |
| 6 | 0x13B | *chieftain* | 20 | 20 | 20 | 0 | 3 | 10 | neutral | 0x6000 | 16 |
| 7 | 0x13C | *man* | 20 | 20 | 20 | 0 | 3 | 10 | neutral | 0x6000 | 16 |
| 8 | 0x13E | *Sakkhra* | 20 | 20 | 20 | 3 | 5 | 10 | neutral | 0x6000 | 22 |
| 9 | 0x13F | *automaton* | 20 | 20 | 20 | 0 | 3 | 10 | neutral | 0x4000 | 16 |
| 10 | 0x140 | *man* | 20 | 20 | 20 | 0 | 3 | 10 | neutral | 0x6000 | 16 |
| 11 | 0x141 | *woman* | 20 | 20 | 20 | 0 | 3 | 10 | neutral | 0x6000 | 16 |
| 12 | 0x142 | *noble* | 20 | 20 | 20 | 0 | 3 | 10 | neutral | 0x6000 | 16 |
| 13 | 0x143 | *Myrmidex drone* | 10 | 45 | 1 | 8 | 10 | 10 | evil | 0x6000 | 78 |
| 14 | 0x144 | *woman* | 20 | 20 | 20 | 0 | 3 | 10 | neutral | 0x6000 | 16 |
| 15 | 0x145 | *neanderthal* | 25 | 15 | 10 | 0 | 10 | 15 | neutral | 0x6000 | 23 |
| 16 | 0x146 | *man* | 20 | 20 | 20 | 0 | 3 | 10 | neutral | 0x6000 | 16 |
| 17 | 0x13D | *parrot* | 1 | 20 | 1 | 0 | 2 | 3 | neutral | 0x2100 | 6 |
| 18 | 0x126 | *tyrannosaurus* | 60 | 5 | 1 | 255 | 150 | 255 | chaotic | 0x2024 | 721 |
| 19 | 0x128 | *gorilla* | 35 | 15 | 10 | 3 | 25 | 40 | chaotic | 0x2020 | 95 |
| 20 | 0x11C | *baby apatosaurus* | 20 | 1 | 1 | 4 | 5 | 20 | neutral | 0x2025 | 7 |
| 21 | 0x121 | *allosaurus* | 40 | 11 | 2 | 8 | 35 | 50 | chaotic | 0x2025 | 127 |
| 22 | 0x129 | *robosaurus* | 50 | 9 | 60 | 15 | 40 | 60 | chaotic | 0x002D | 167 |
| 23 | 0x12F | *stegosaurus* | 30 | 3 | 1 | 8 | 20 | 25 | neutral | 0x2025 | 43 |
| 24 | 0x131 | *triceratops* | 40 | 8 | 2 | 10 | 30 | 50 | neutral | 0x2025 | 105 |
| 25 | 0x120 | *pteranodon* | 10 | 35 | 2 | 3 | 20 | 10 | chaotic | 0x2105 | 72 |
| 26 | 0x11B | *serpent woman* | 17 | 25 | 12 | 2 | 15 | 20 | evil | 0x2044 | 57 |
| 27 | 0x12C | *plesiosaur* | 25 | 20 | 2 | 5 | 15 | 20 | chaotic | 0x020D | 50 |
| 28 | 0x147 | *ankylosaurus* | 25 | 2 | 1 | 15 | 15 | 25 | neutral | 0x2025 | 28 |
| 29 | 0x14A | *deinonychus* | 7 | 20 | 2 | 3 | 10 | 20 | chaotic | 0x2001 | 41 |
| 30 | 0x14B | *sabretooth tiger* | 30 | 25 | 7 | 3 | 30 | 35 | chaotic | 0x2000 | 106 |
| 31 | 0x14C | *sloth* | 20 | 5 | 3 | 1 | 5 | 20 | neutral | 0x2020 | 12 |
| 32 | 0x14D | *jaguar* | 22 | 30 | 8 | 2 | 25 | 25 | chaotic | 0x2000 | 97 |
| 33 | 0x14E | *dimetrodon* | 30 | 5 | 2 | 6 | 25 | 20 | chaotic | 0x2025 | 55 |
| 34 | 0x14F | *bear* | 30 | 15 | 6 | 3 | 27 | 40 | chaotic | 0x2020 | 90 |
| 35 | 0x123 | *wisp* | 1 | 60 | 60 | 0 | 1 | 30 | neutral | 0x1429 | 112 |
| 36 | 0x12B | *baby pteranodon* | 3 | 2 | 1 | 0 | 3 | 3 | neutral | 0x0011 | 0 |
| 37 | 0x12D | *argent sergeant* | 20 | 15 | 60 | 7 | 15 | 30 | chaotic | 0x002D | 53 |
| 38 | 0x12E | *turtle* | 15 | 5 | 4 | 10 | 10 | 25 | neutral | 0x0A28 | 20 |
| 39 | 0x133 | *party member on turtle* | 15 | 5 | 4 | 5 | 10 | 25 | neutral | 0x4A28 | 18 |
| 40 | 0x134 | *snake* | 3 | 7 | 6 | 1 | 2 | 5 | chaotic | 0x2840 | 3 |
| 41 | 0x135 | *alphadon* | 5 | 13 | 4 | 0 | 6 | 8 | chaotic | 0x2840 | 11 |
| 42 | 0x150 | *giant spider* | 9 | 11 | 4 | 0 | 8 | 10 | chaotic | 0x0051 | 12 |
| 43 | 0x151 | *eohippus* | 5 | 14 | 6 | 0 | 2 | 7 | neutral | 0x2020 | 6 |
| 44 | 0x148 | *Myrmidex queen* | 0 | 0 | 30 | 10 | 0 | 20 | evil | 0x3000 | 0 |
| 45 | 0x149 | *dancer* (shape used in the dance modes) | 20 | 30 | 20 | 0 | 3 | 10 | neutral | 0x6000 | 24 |
| 46 | 0x11A | vine tip (*carnivorous orchid* in Nuvie) | 50 | 30 | 0 | 5 | 10 | 10 | chaotic | 0x1011 | 46* |
| 47 | 0x124 | vine core / vine segment (*carnivorous orchid* in Nuvie) | 0 | 0 | 0 | 5 | 0 | 10 | chaotic | 0x1011 | 0* |

(*) The vine parts' formula experience is not given; see Segmented creatures. The
`monsdata.c` tables end with 14 zero bytes nobody reads. The table XP values assume the
class's own alignment; neutral and good classes give nothing unless their alignment has
turned chaotic or evil by the time they die. Class 18's 255 hit points, randomised by
eggs, can exceed 255 and wrap in the one-byte hit-point field (see Eggs); it is
invulnerable anyway.

#### Monster flags

The meaning of each `MonsterFlags` bit, as far as GAME.EXE reads it:

| Bit | Meaning | Where |
|---|---|---|
| 0x0001 | not read by any code | |
| 0x0004 | not read by any code | |
| 0x0008 | immune to lava and the burning tiles (0x383, 0x406, 0x42B-0x42F, 0x434-0x437) | `TerrainDamage` C_0359_0DA8 |
| 0x0010 | takes double damage from those tiles | `TerrainDamage` |
| 0x0020 | natural attack uses STR (to-hit test); STR replaces DEX in the XP formula | `__AttackTest`, `COMBAT_Kill` |
| 0x0040 | poisonous (1 in 4) | `__DoHit` |
| 0x0100 | flies | `CanMoveTo`-family movement code in npctrack.c |
| 0x0200 | swims | npctrack.c |
| 0x0400 | ethereal (with 0x100 also: no terrain damage; no class has both) | npctrack.c, `ApplyTerrainEffects` C_0359_1014 |
| 0x0800 | leaves no body | `SetTypeUnconscious` |
| 0x1000 | leaves no body; map redrawn with a fade when it dies within 5 tiles | `SetTypeUnconscious` |
| 0x2000 | bleeds when it dies (blood object 0xF8) | `SetTypeUnconscious` |
| 0x4000 | "<Name> attacks!" when the player hits it while it is neutral | `COMBAT_ProvokeNeutral` |

Bits 0x0002, 0x0080 and 0x8000 are set in no class.

### Explosives

- **Grenade** (0x89): Use lights it: it becomes a lit grenade (0x8A) and a 3-minute fuse
  starts; only one fuse can run at a time ("A grenade has already been lit.")
  (`UseExplosive` C_1DBD_3AA1). When the fuse runs out it explodes where it is, even in an
  inventory (`PassTime`, `Detonate` C_1DBD_3C68). Dropping a lit grenade (or an activated
  device) explodes it where it lands (`CmdDrop` C_1DBD_1960). Fired as a weapon, lit or not, it explodes at the
  impact tile.
- **Grenade blast** (`C_1810_1941`): sound 6; the blast reaches the 3x3 square around the
  centre (trails that walls stop, `Explosion` C_276B_0E85); every actor reached takes
  1d20 (no experience). Then, in the 3x3 square (`AfterExplosion` C_1DBD_3D25): unlit
  grenades lying there are lit and explode (chain reaction), boulders (0xA7) are pushed
  away from the centre, object 0x107 takes 30, webs (0x11) are destroyed. Setting fire to
  gunpowder (0xCC) also makes a grenade blast, and its holder takes 1d30 (`UseFire`
  C_1DBD_7AB6).
- **Device** (0xF0, activated 0xF1): same fuse logic ("Device activated."). Its blast
  (`C_1810_19C1`, sounds 0x12 and 0x1D) hurts only Myrmidex (types 0x143/0x148) in the 3x3
  square, 1d35 each, then the same `AfterExplosion`.
- When fired as a weapon the object at the impact tile also takes the weapon's normal hit
  (`__DoHit` with damage 15 for grenades, 3 for devices), after the blast, if it is still
  there.
- **AI aim**: an AI actor whose chosen weapon is a grenade or a device does not aim at its
  opponent. It picks, among the occupied tiles of the 11x9 area around the active party
  member, the one with the most hostile (evil or chaotic) living actors in the 3x3 square
  around it, the centre counting twice; ties are broken at random. For grenades a square
  that holds the thrower or a good living actor is skipped; for devices only Myrmidex
  count and allies are not checked (`C_1810_4561`).

### Boulders

A boulder (type 0xA7) is a 2x2 object whose position is its bottom right corner. In this
code only explosions push boulders (and a pushed boulder pushes the boulder it lands on)
(`AfterExplosion`, `C_1810_14EA`, `C_1810_1101`):

- A boulder with quality 1 is stuck: sound 0x1C, no move.
- It moves one tile away from the blast on each axis where the blast is beside it. It
  does not move east or south onto certain tiles (`TileClass3`/`TileClass2`, DS 102E/1008),
  and is blocked by the tiles 0x90-0x92, 0x95, 0x98-0xA9, 0xAC-0xB6, 0xBA-0xC5, trees, and
  the objects 0x96, 0x97, 0x104-0x10A (`C_1810_0FE6`, `C_1810_103F`).
- While any of its four tiles is in `TileClass1` (DS 0F80, 68 tile numbers, apparently
  slopes or pits) it keeps rolling south-east; when blocked it tries the eight
  neighbouring places.
- Where it lands, another boulder is pushed first (if that fails, the move fails). Actors
  (or the owners of creature parts) are killed outright when the boulder rolls onto them;
  on the first push they are moved to a free neighbouring tile and take 1d30, or are
  killed if there is no room. Other objects are deleted, except dead bodies (0xF9, 0x118)
  and a test on `TileFlag2[obj] & 0x20` that indexes a tile table by an object number
  (looks like a slip). No experience.
- If after a move there is an object 0x96 two tiles south, the boulder becomes stuck and
  the objects 0x96, 0x97, 0xC3 (two) and 0x98 around it, and an egg (0xE6) under the
  boulder, are deleted (a puzzle: the boulder plugs something). A boulder with a tile 0x0C
  or 0x0F under it becomes stuck and permanent (`C_1810_13B1`, `C_1810_1327`).

This is a summary of `C_1810_14EA`; the exact order of its attempts, and the precise role
of the three tile classes, are more involved than stated here.

### Segmented creatures (vines)

Eggs make type 0x124 as a core actor with quality 4 (the number of tips) and four tip
actors of type 0x11A on the four sides, facing outward, in mode 0x0D, with the core's
alignment; each tip's quality is the core (`EGG_generate` C_2667_015D). GAME.EXE has the
message "Tangle vine killed!" but no code prints it.

- **Growing** (`COMBAT_AI_Tangle` C_1810_57F9, `__TryGrowVine` C_1810_564E): each turn
  the core's frame toggles. The tip picks an opponent as if it were the core
  (`COMBAT_pickOpponent` of the core). If the opponent is adjacent (within 1 on both axes)
  the tip attacks it with its own natural attack (damage 10). If it is within 4 tiles the
  tip grows one tile toward it along the longer axis, else along the other. Otherwise one
  time in 4 it grows in a random direction; a direction equal to its facing is turned
  around, and growing backward is refused, so only 90-degree turns succeed. Otherwise it
  spends all its points. Growing moves the tip and leaves a vine segment (non-actor object
  0x124, quality = the tip) behind it. A tip does not grow further east or west when it is
  already more than 5 tiles from the core on that side, nor further north or south when
  more than 4 tiles from it.
- **Damage** (`C_1810_22F0`): hitting the core does not hurt it; instead every tip in actor
  slots 0x10-0xFF (all type-0x11A actors, not only this core's) takes half the damage.
  Hitting a vine segment does the full damage to its tip. A tip that takes damage >= its
  hit points dies, and the segments it grew are removed, back to the core; the core's tip
  count drops by 1. When the count reaches 0, an object 0x4E is left on the core's tile,
  the core dies, and 10 experience is returned. Armour does not apply (tips wear nothing).
- **Experience quirk**: when a tip survives a hit, the function returns the tip's new hit
  points (the machine code leaves them in AX, 1810:258C), so the attacker gains that much
  experience. When a tip dies but the creature lives, the return value is whatever AX
  holds. No wound messages for these parts.

### Combat AI

`NPC_TakeTurn` (C_130F_4121) chooses the routine by `NPCMode`; actors with ObjStatus 0x80
always use Berserk:

| Mode | Routine |
|---|---|
| 0x04 | `COMBAT_AI_Range` C_1810_4AA9 |
| 0x06 | `COMBAT_AI_Berserk` C_1810_4D85 |
| 0x08, 0x9B | `COMBAT_AI_Assault` C_1810_476D |
| 0x07, 0x09, 0x13, 0x9A | `COMBAT_AI_Retreat` C_1810_50CA |
| 0x0A | `COMBAT_AI_Like` C_1810_523E |
| 0x0B | `COMBAT_AI_Unfriendly` C_1810_5370 |
| 0x0D | `COMBAT_AI_Tangle` C_1810_57F9 |
| 0x0E | `COMBAT_AI_Immobile` C_1810_54A9 |
| 0x99 | brawl: one time in 4, approach an opponent within 4 tiles of its spot and "hit" it (a hit flash only, no damage), then go back |

#### Choosing an opponent

`COMBAT_pickOpponent` (C_1810_4391) takes the closest (squared straight-line distance)
actor that is:

- not dead, not itself, within 11 tiles on both axes, visible to it (`COMBAT_canSee`
  C_1810_0CA4: not invisible, except party members to player-controlled viewers; not
  dragged under; not on an "ignore" tile; same level);
- not a fleeing actor (mode 7) more than 5 tiles from the screen centre;
- an enemy by alignment:

| Chooser | Attacks |
|---|---|
| good (0x40) | evil or chaotic (bit 0x20) |
| evil (0x20) | good or chaotic (bit 0x40) |
| chaotic (0x60) | good or evil |
| neutral (0x00) | anyone not neutral |

On a tie in distance the one whose class has the higher `MonsterDamage` wins.

#### Choosing a weapon

`__PickWeapon` (C_1810_4277): the readied weapon with the highest damage whose range
reaches the opponent; else the natural attack. It does not check ammunition, so an
attacker out of arrows can keep choosing its bow and waste its turns. The ranged chooser
(`C_1810_4968`) takes readied weapons of range 3 or more: the longest range until one
reaches the opponent, then the highest damage among those that do.

#### The modes

- **Assault** (CLOSE, 8; `COMBAT_AI_Assault`): pick the opponent and weapon (the
  explosive target rule for grenades/devices). If the opponent is in range and adjacent,
  or a silent test missile reaches it, attack; if the line of fire is blocked, step
  sideways (perpendicular to the opponent, side at random). Then move toward the opponent
  in any case. No opponent: wander.
- **Range** (RANGE, 4; `COMBAT_AI_Range`): if the opponent is within 2 tiles on both
  axes, step away. The code passes `(x - opponentX, y - opponentY)`, a difference, as the
  destination, so the step goes toward a point near the map origin rather than reliably
  away (looks like a bug). With no readied weapon of range 3 or more: "<Name> has no ranged
  weapons!" and nothing else. If more than 5 tiles (x) or 4 tiles (y) from the screen
  centre, walk back toward it. Otherwise shoot when in range and in line of fire, sidestep
  when blocked, approach when out of range.
- **Berserk** (WILD, 6, and wild actors; `COMBAT_AI_Berserk`): the target is the visible
  living actor within 8 tiles with the highest STR+DEX+INT (raw values). Alignment rules
  are stricter than above: good attacks only exactly evil (0x20), evil only exactly good
  (0x40), chaotic anything not chaotic or neutral, neutral anything. Attack if in range
  and line of fire; else move toward it (or sidestep); if it cannot move, act as Assault.
  No target: Assault. Always costs 5 more points.
- **Retreat** (FLEE, 7, and 9, 0x13, 0x9A; `COMBAT_AI_Retreat`): more than 7 tiles from
  the screen centre, it stops (points 0). Otherwise, if its opponent is within 7 tiles
  on both axes, it moves directly away; else good actors go back toward the party and the
  others wander. An actor not under player control also regains 1 HP (up to Max HP) one
  time in 4, and returns to its combat mode when `HP*4/MaxHP` is at least 1 (no longer
  critical).
- **Like** (0x0A): within 2 tiles of any party member: one time in 2 approach the party.
  Otherwise one time in 2 walk 10 tiles in a random straight direction; else wait (5).
- **Unfriendly** (0x0B): within 2 tiles of any party member, one time in 8 it turns to
  Assault (mode 8) and attacks at once; otherwise like Like, without approaching.
- **Immobile** (0x0E; `COMBAT_AI_Immobile`): never moves. With a grenade or device it
  throws it at the best place (rule above). Otherwise the code picks its target at a
  position (`opp_x`,`opp_y`) that is read before it is ever set, then makes 16 random
  tiles within 4; on the first try where that target is a visible actor of another,
  non-neutral alignment, it attacks the random tile (the range check uses the random
  tile, the hit goes to the target). The result depends on stack contents and cannot be
  stated exactly. A neutral immobile actor never attacks. It waits (5) when it does not
  attack.
- **Tangle** (0x0D): see Segmented creatures.

Party members in CMND mode are not run by the AI; the turn scheduler hands them to the
player (`RunNPCTurns` C_130F_4C0F).

### Spawning creatures from eggs

An egg is an invisible object of type 0xE6; its quantity is the percentage chance to
hatch, and the objects in its inventory ("embryos") say what to make, the embryo's
quantity being the maximum number (`EGG_hatches` C_2667_04F7).

When eggs hatch: whenever the 40x40 loaded area is rebuilt, the eggs of the current level
in it that are more than 8 tiles (on either axis) from the screen centre try to hatch
(`MAP_BuildArea` C_08CB_02FA, `EGG_hatchArea` C_2667_08E0). After a party teleport every
egg in the area tries, however close (`PartyTeleport` C_08CB_07FE, `ForceHatching`). Not
while a game is being loaded (`HatchingDisabled`). An egg tries once: whatever the roll,
it is then marked hatched (ObjStatus 0x40) and invisible, and egg.c never clears that
mark.

Hatching (roll `1d100 <= egg quantity`), for each embryo:

| Embryo type | Number made |
|---|---|
| 0x143 (*Myrmidex drone*) | 1 if TalkFlags[1] bit 3 is set; else 5-15 on level 1; else exactly the embryo quantity |
| 0x12D, 0x11A, 0x136-0x146 (other than 0x143) | `1d(embryo quantity)` if the egg quantity is 100 or less; exactly the embryo quantity if more than 100 |
| 0x7F, 0xCB (objects) | 10-20 |
| anything else | 1 |

- Types below 0x11A are plain objects: a boulder (0xA7) appears on the egg with frame 3;
  others at a random spot within 3 tiles (each axis), not on a "lose objects" tile nor on
  a tile with terrain bits 2 or 4. Types 0x7F and 0xCB get quantity 5 (their hit points
  as breakables). If the spot is refused, the code goes on with the previous creature's
  number (an uninitialised or stale variable), so the following teleport step may act on
  another object.
- Creatures are made by `EGG_generate` (below). A hostile or good one (alignment not 0)
  gets mode 8 (assault); a neutral one mode 0x8F (wander).
- The first thing made stays on the egg. Each further one is moved to a random free spot
  `x + 2d4 - 5`, `y + 2d4 - 5` (so -3..+3, centre-weighted), 8 tries, which must be
  outside the visible 11x11 square unless hatching was forced; if no spot is found it is
  deleted (`COMBAT_TryTeleport` C_1810_0394).
- If any creature made is evil or chaotic (bit 0x20) and no warning was given since combat last
  ended, "You hear something to the <direction>!" is printed with a short jingle
  (`EnemyWarningGiven`).

`EGG_generate` (C_2667_015D) for a type with a monster class:

- STR, DEX, INT and HP are `mkRandom(class value)` (`mkRandom` C_2667_00A2): with
  `h = value / 2`, the result is `value - h + 0d..h + 0d..h`, so from `value - h` to
  `value + h`, centre-weighted (two `OSI_rand(0, h)`). Values of 1 stay 1. The fields are
  one byte, so a class value over 170 can wrap (only class 18's HP 255).
- Level 0, XP 0, movement points = DEX, alignment from the class. Exception: type 0x13C
  is chaotic until TalkFlags[0x35] bit 7 is set.
- Mode and combat mode 8.
- A type without a class gets STR, DEX, INT 10, HP 30, level 1, evil.
- Shapes: 0x120 and the two-tile kind start at frame 1, 0x128 at frame 3; the multi-tile
  creatures get their other parts placed around them; 0x124 gets its four vine tips.
- Possessions (`EGG_GivePossessions` C_2667_00DA): 0x140 gets an obsidian sword (0x21)
  and bark armour (0x09), both readied; 0x13C gets a blowgun (0x20) with 30 darts (0x24),
  or an obsidian knife (0x19) if the blowgun could not be added.

### Uncertainties

- Names in italics come from Nuvie, not from the game's own data, and Nuvie gives the
  same name to several types (for example "man", "woman", "carnivorous orchid").
- The to-hit result being ignored is certain from the code; whether Origin meant it is
  not known.
- Flags 0x0001 and 0x0004 are set in many classes but no GAME.EXE code reads them.
- The exact shapes of the multi-tile creatures' reach boxes are given by
  `COMBAT_AttackRange`; this section only summarises them.
- What the objects 0x107, 0xDE, 0x93, 0x96-0x98, 0xC3, 0xCB and 0xAF are in the game world
  was not established.
- The Immobile mode's targeting depends on an uninitialised variable.

## Sources

- The decompiled sources in `SRC/` (GAME.EXE) and `SRC_S/` (SAVAGE.EXE).
- [Nuvie](https://github.com/nuvie/nuvie), `data/scripts/se/actor.lua` and `usecode.lua`: some creature and object names, marked in italics; none of its numbers are used.
