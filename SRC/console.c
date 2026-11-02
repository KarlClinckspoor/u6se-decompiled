/* flags: -d -mm -p -c -Z -O -y */
/*
 * The Savage Empire (Worlds of Ultima) - GAME.EXE
 * segment 05D0: the console and user input (Ultima VI's console.c).
 *
 * - Text output: a small "console" driver writing 8x8 characters into
 *   rectangular text windows (struct tClipInfo; CurClip is the current one,
 *   TextWindows[] holds the game's windows: [0] full screen, [1] status
 *   panel, [3] message scroll area). Characters are collected in a line
 *   buffer (LineBufChars / LineBufColors) and drawn one line at a time,
 *   which allows word wrap, centering, scrolling and "more" pagination
 *   according to the window's attrFlags:
 *     0x01 word wrap        0x02 center lines
 *     0x04 '`' capitalizes  0x08 runic font (char | 0x80)
 *     0x10 paginate         0x20 scroll at bottom
 *   CON_putch interprets the in-text escapes of conversations and messages
 *   (# $ / \ ^ @ * `), CON_printf is a minimal printf.
 * - Input: CON_prompt (animated cursor + one key), CON_ReadCommand (keys
 *   and mouse clicks to command codes), CON_getch (also runs the status
 *   panel commands itself), CON_gets (line input).
 * - Mouse: pointer shape, direction from the pointer position, hit-testing
 *   of the map view, the command icons and the status panel views.
 * - The keyboard "slot cursor" of the equipment/portrait views.
 * - Game load/save/restore (savegame\objlist).
 *
 * Command codes (Savage Empire's): 0x80 direction, 0x81 move, 0x82 get,
 * 0x83 drop, 0x84 use, 0x85 talk, 0x86 look, 0x87 attack, 0x88 rest,
 * 0x89 combat mode, 0x8a, 0x8e target, 0x8f pass, 0x90 portrait view,
 * 0x91 party view, 0x92 equipment view, 0x99 next member, 0x9a previous
 * member, 0x9b/0x9c/0x9d inventory slot / equipment slot / map object hit,
 * 0x9e conversation view, 0x9f container up, 0xa0 open container,
 * 0xa1, 0xa6 refresh status, 0xa7 ready, 0xa8 unready, 0xa9 default
 * action, 0xaa save.
 */
#include "se.h"

/*a text window*/
struct tClipInfo/*size:9*/ {
	unsigned char originX, originY, cols, rows;
	unsigned char curX, curY;
	unsigned char fgColor, bgColor;
	unsigned char attrFlags;/*1 wrap, 2 center, 4 capitalize, 8 runes, 0x10 paginate, 0x20 scroll*/
};

/*a drawing surface of the mouse code (Ultima VI's)*/
struct tSurface {
	uint16 segment;/*_00*/
	uint16 far *pScanlines;/*_02*/
	uint16 right, left, top, bottom;/*_06,_08,_0a,_0c*/
};

/*the mouse state (Ultima VI's)*/
struct tMouse {
	unsigned char buttonL, buttonR;
	int16 posX, posY;
	uint16 unused06;
	void far *shapes;/*_08 pointer shapes*/
	uint16 shapeIndex;
	struct tSurface *surface;/*_0e*/
	void far *saveBuf;/*_10 background save area*/
	void (far *onMove)();/*_14*/
	unsigned char installed;
};

/*the graphic driver's state (Ultima VI's tGr)*/
struct tGr {
	unsigned char header[6];/*_00*/
	void far *font;/*_06*/
	unsigned char textFg, textBg;/*_0a,_0b*/
	unsigned char xorMode;
	unsigned char unknown0d;
	unsigned char animOn;
	unsigned char toBackBuffer;/*_0f*/
};

/*mouse (OSI assembly)*/
extern cdecl MOUSE_POINTER(void far *, int16);/*MOUSE_POINTER*/
extern cdecl MOUSE_SERVE();/*MOUSE_SERVE*/
extern cdecl MOUSE_SHOW();/*MOUSE_SHOW*/
extern cdecl MOUSE_HIDE();/*MOUSE_HIDE*/
extern int16 cdecl MOUSE_IN_AREA(struct tSurface *);/*MOUSE_IN_AREA*/
/*OSI file routines (asm)*/
extern int16 OSI_open(char *);/*OSI_open*/
extern OSI_close(int16);/*OSI_close*/
extern int16 OSI_create(char *);/*OSI_create*/
extern int32 OSI_read(int16, int32, int32, void far *);/*OSI_read*/
extern int32 OSI_write(int16, int32, int32, void far *);/*OSI_write*/
extern int16 KBD_GetKey(int16);/*KBD_GetKey*/
extern cdecl TimerDelay(long);/*wait (2383 per millisecond)*/
/*other game modules*/
extern PaletteAnimation();/*PaletteAnimation (main)*/
extern DrawMapWindow(int16);/*DrawMapWindow?*/
extern DisplayTick();/*DisplayTick*/
extern PassTime(int16);/*PassTime*/
extern RefreshStatus();/*RefreshStatus?*/
extern MAP_LoadChunkIndexes(int16, int16, int16);/*MAP_LoadChunkIndexes*/
extern MAP_InitChunkCache();/*MAP_InitChunkCache*/
extern MAP_LoadAround();/*MAP_LoadAround*/
extern VIS_UpdateView();/*VIS_UpdateView*/
extern int16 OBJ_MakeDirFrame(int16, int16);/*OBJ_MakeDirFrame(type, direction)*/
extern MoveObj(int16, int16, int16, int16);/*MoveObj*/
extern int16 QuanType(int16);/*QuanType: 4 quantity on 16 bits, 2 on 8 bits, else 0*/
extern OBJ_DeleteTmpBlocks();/*OBJ_DeleteTmpBlocks*/
extern LoadNewRegions(int16, int16, int16);/*LoadNewRegions*/
extern OBJ_CommitBlocks();/*OBJ_CommitBlocks*/
extern OBJ_InitLists();/*OBJ_InitLists*/
extern OBJ_RelinkNPCs();/*OBJ_RelinkNPCs*/
extern STAT_refreshComMode();/*STAT_refreshComMode*/
extern STAT_RefreshInventory(int16);/*STAT_RefreshInventory*/
extern int16 STAT_PickInvenSlot();/*STAT_PickInvenSlot*/
extern int16 STAT_PickEquipSlot();/*STAT_PickEquipSlot*/
extern STAT_Ready();/*STAT_Ready*/
extern STAT_Unready();/*STAT_Unready*/
extern TALK_talkTo(int16, int16, int16);/*TALK_talkTo(partyId, objNum, showName)*/
extern InitNPCTracker();/*InitNPCTracker*/
extern int16 COMBAT_TileDistance(int16, int16, int16);/*COMBAT_TileDistance(objNum, x, y)*/
extern COMBAT_breakOff();/*COMBAT_breakOff*/
extern int16 COMBAT_GetTargetAt(int16, int16, int16);/*COMBAT_GetTargetAt(objNum, x, y)*/
extern int16 IsContainerType(int16);/*IsContainerType*/
extern MUS_PlayAreaSong();/*MUS_PlayAreaSong?*/
extern DISK_confirm(int16);/*DISK_confirm*/

extern char DirIncrX[8], DirIncrY[8];/*DirIncrX, DirIncrY*/
extern int16 GrAdapter;/*GrAdapter*/
extern unsigned char MouseOn;/*MouseOn*/
extern char StatusDirty;/*StatusDirty*/
extern char PromptFlag;/*PromptFlag?*/
extern char IsArmageddon;/*IsArmageddon?*/
extern int16 IdleAnimTick;/*IdleAnimTick*/
extern int16 MapAnimEnabled;/*MapAnimEnabled*/
extern int16 ObjBlkTmpPending[0x45];/*ObjBlkTmpPending*/
extern int16 MoveAppliesTerrain;/*MoveAppliesTerrain*/
extern unsigned char PartyListTop;/*PartyListTop*/
extern unsigned char InvenScroll;/*InvenScroll*/
extern unsigned char IsInConversation;/*IsInConversation*/
extern int16 BuiltAreaX, BuiltAreaY, BuiltAreaZ;/*BuiltAreaX, BuiltAreaY, BuiltAreaZ (map)*/
extern int16 AdvanceBlocked;/*AdvanceBlocked*/
extern int16 HatchingDisabled;
extern int16 ScreenFade;/*ScreenFade*/
extern int16 ColorRed;/*ColorRed*/
extern int16 ColorPaper;/*ColorPaper*/
extern int16 D_24E6;/*ColorText*/
extern int16 ColorBrown;/*color of the default command's frame*/
extern int16 D_24EE;/*color to erase it*/
extern unsigned char IsOnQuest;/*GameStateStart: first byte of the saved game state*/
extern unsigned char Date_D;/*day of the month*/
extern unsigned char Date_M;/*month*/
extern int16 Date_Y;/*year*/
extern unsigned char Active;/*Active*/
extern int16 MapX, MapY, MapZ;/*MapX, MapY, MapZ*/
extern char InCombat;/*InCombat*/
extern unsigned char DefaultCommand;/*DefaultCommand*/
extern char GameStateEnd;/*GameStateEnd*/
extern int16 DiskFullChoice;/*DiskFullChoice*/
extern int16 TextIdle;/*TextIdle*/
extern char far *D_3298;/*0x100 bytes*/
extern unsigned char D_329C[];/*ObjStatus*/
extern struct tMouse D_3E9C;/*Mouse*/
extern char D_44FA[];/*MovePts*/
extern char far *D_45FA;/*0x100 bytes (SchedIndex?)*/
extern char far *D_45FE;/*0x100 bytes (PathData?)*/
extern char far *D_4602;/*0x100 bytes*/
extern unsigned char D_4606[];/*DEXTE*/
extern unsigned char far *D_4706;/*NPCMode*/
extern unsigned char D_4EDA[];/*Party*/
extern unsigned char far *D_4EEC;/*Level*/
extern char far *D_4EF0;/*Schedule (5 bytes each)*/
extern int16 D_4EF4[0x101];/*SchedPointer*/
extern void far *D_50FC;/*PointerShapes*/
extern char far *D_5100;/*0x100 bytes*/
extern char D_5104[][14];/*Names*/
extern uint16 D_51E4[];/*ExpPoints*/
extern unsigned char far *D_53E4;/*NPCComMode*/
extern unsigned char D_53E8[];/*STREN*/
extern int16 D_54F0[];/*Amount*/
extern unsigned char D_6CF0[];/*NPCStatus*/
extern unsigned char D_6DF4[];/*HitPoints*/
extern int16 far *D_6EF4;/*BaseTile*/
extern unsigned char D_6EFC[];/*TalkFlags*/
extern unsigned char D_7644[];/*INTEL*/
extern struct coord D_78E4[];/*ObjPos*/
extern unsigned char D_9CF6;/*PartySize*/
extern int16 D_9CF8;/*NumAnimData*/
extern uint16 D_9CFA[];/*ObjShapeType*/
extern int16 far *D_B53E;/*OrigShapeType*/
extern unsigned char D_B570[];/*StateAnimData*/
extern unsigned char D_B590;/*PromptCnt: animated cursor frame*/
extern int16 D_B591;/*MouseHeldCmd: command of the mouse button held*/
extern unsigned char D_B593[];/*LineBufChars*/
extern unsigned char D_B5BC[];/*LineBufColors*/
extern int16 D_B5E6;/*AdvanceDir*/
extern int16 D_B5E8, D_B5EA;/*PointerX, PointerY*/
extern struct tSurface D_B5EC;/*MouseArea*/
extern int16 D_B5FA[10+26];/*VarInt*/
extern int16 D_B642;/*Selection.obj*/
extern struct tClipInfo D_B644[4];/*TextWindows*/
extern int16 D_B668;/*MouseButtons: bit 0 left, bit 1 right*/
extern int16 D_B66A, D_B66C;/*Selection.x, Selection.y*/
extern char *D_B66E[10+26];/*VarStr*/
extern int16 D_B6B6;/*LinesBeforeMore*/
extern int16 D_B6B8;/*SelectionIsDark*/
extern unsigned char D_BD10[11][11];/*Obj_11x11: objects shown in the map window*/
extern int16 D_C08A[11][11];/*Tile_11x11: the tiles of the map window*/
extern int16 D_E64A;/*InvenOwner*/
extern unsigned char D_E64C[8];/*Party_2Handed*/
extern int16 D_E654;/*ShowNPCInven*/
extern int16 D_E656;/*NPC_2Handed*/
extern struct tGr far *D_ED0C;/*GrInfo*/
extern union tHandler D_ED10;/*GrDriver*/

#define DirIncrX DirIncrX
#define DirIncrY DirIncrY
#define GrAdapter GrAdapter
#define MouseOn MouseOn
#define StatusDirty StatusDirty
#define IdleAnimTick IdleAnimTick
#define MapAnimEnabled MapAnimEnabled
#define ObjBlkTmpPending ObjBlkTmpPending
#define MoveAppliesTerrain MoveAppliesTerrain
#define PartyListTop PartyListTop
#define InvenScroll InvenScroll
#define IsInConversation IsInConversation
#define AdvanceBlocked AdvanceBlocked
#define BuiltAreaX BuiltAreaX
#define BuiltAreaY BuiltAreaY
#define BuiltAreaZ BuiltAreaZ
#define ScreenFade ScreenFade
#define ColorRed ColorRed
#define ColorPaper ColorPaper
#define ColorText D_24E6
#define Active Active
#define MapX MapX
#define MapY MapY
#define MapZ MapZ
#define InCombat InCombat
#define DefaultCommand DefaultCommand
#define DiskFullChoice DiskFullChoice
#define TextIdle TextIdle
#define ObjStatus D_329C
#define Mouse D_3E9C
#define NPCMode D_4706
#define Party D_4EDA
#define SchedPointer D_4EF4
#define PointerShapes D_50FC
#define NPCComMode D_53E4
#define Amount D_54F0
#define BaseTile D_6EF4
#define TalkFlags D_6EFC
#define ObjPos D_78E4
#define PartySize D_9CF6
#define NumAnimData D_9CF8
#define ObjShapeType D_9CFA
#define StateAnimData D_B570
#define PromptCnt D_B590
#define MouseHeldCmd D_B591
#define LineBufChars D_B593
#define LineBufColors D_B5BC
#define AdvanceDir D_B5E6
#define PointerX D_B5E8
#define PointerY D_B5EA
#define MouseArea D_B5EC
#define VarInt D_B5FA
#define SelectionObj D_B642
#define TextWindows D_B644
#define MouseButtons D_B668
#define SelectionX D_B66A
#define SelectionY D_B66C
#define VarStr D_B66E
#define LinesBeforeMore D_B6B6
#define SelectionIsDark D_B6B8
#define Obj_11x11 D_BD10
#define Tile_11x11 D_C08A
#define InvenOwner D_E64A
#define Party_2Handed D_E64C
#define ShowNPCInven D_E654
#define NPC_2Handed D_E656
#define GrInfo D_ED0C
#define GrDriver D_ED10

#define MOUSE_POINTER MOUSE_POINTER
#define MOUSE_SERVE MOUSE_SERVE
#define MOUSE_SHOW MOUSE_SHOW
#define MOUSE_HIDE MOUSE_HIDE
#define MOUSE_IN_AREA MOUSE_IN_AREA
#define OSI_open OSI_open
#define OSI_close OSI_close
#define OSI_create OSI_create
#define OSI_read OSI_read
#define OSI_write OSI_write
#define KBD_GetKey KBD_GetKey
#define PaletteAnimation PaletteAnimation
#define DrawMapWindow DrawMapWindow
#define DisplayTick DisplayTick
#define PassTime PassTime
#define RefreshStatus RefreshStatus
#define MAP_LoadChunkIndexes MAP_LoadChunkIndexes
#define MAP_InitChunkCache MAP_InitChunkCache
#define MAP_LoadAround MAP_LoadAround
#define VIS_UpdateView VIS_UpdateView
#define OBJ_MakeDirFrame OBJ_MakeDirFrame
#define MoveObj MoveObj
#define QuanType QuanType
#define OBJ_DeleteTmpBlocks OBJ_DeleteTmpBlocks
#define LoadNewRegions LoadNewRegions
#define OBJ_CommitBlocks OBJ_CommitBlocks
#define OBJ_InitLists OBJ_InitLists
#define OBJ_RelinkNPCs OBJ_RelinkNPCs
#define STAT_refreshComMode STAT_refreshComMode
#define STAT_RefreshInventory STAT_RefreshInventory
#define STAT_PickInvenSlot STAT_PickInvenSlot
#define STAT_PickEquipSlot STAT_PickEquipSlot
#define STAT_Ready STAT_Ready
#define STAT_Unready STAT_Unready
#define TALK_talkTo TALK_talkTo
#define InitNPCTracker InitNPCTracker
#define COMBAT_TileDistance COMBAT_TileDistance
#define COMBAT_breakOff COMBAT_breakOff
#define COMBAT_GetTargetAt COMBAT_GetTargetAt
#define IsContainerType IsContainerType
#define MUS_PlayAreaSong MUS_PlayAreaSong
#define DISK_confirm DISK_confirm

#define GR_SET_COLOR(c)              (GrDriver.iii.ofs = 0x06, (*GrDriver.f)(c))
#define GR_PUT_PIXEL(x,y)            (GrDriver.iii.ofs = 0x09, (*GrDriver.f)(x,y))
#define GR_FILL_RECT(x0,y0,x1,y1)    (GrDriver.iii.ofs = 0x12, (*GrDriver.f)(x0,y0,x1,y1))
#define GR_SCROLL_RECT_Y(x0,y0,x1,y1,dy) (GrDriver.iii.ofs = 0x15, (*GrDriver.f)(x0,y0,x1,y1,dy))
#define GR_PUT_CHAR(ch,x,y)          (GrDriver.iii.ofs = 0x1e, (*GrDriver.f)(ch,x,y))
#define GR_PUT_TILE_SCREEN(tile,x,y) (GrDriver.iii.ofs = 0x2d, (*GrDriver.f)(tile,x,y))
#define GR_PUT_TILE(tile,x,y)        (GrDriver.iii.ofs = 0x42, (*GrDriver.f)(tile,x,y))
#define GR_COPY_TO_SCREEN(x0,y0,x1,y1,a,b) (GrDriver.iii.ofs = 0x45, (*GrDriver.f)(x0,y0,x1,y1,a,b))

#define GetType(i) (ObjShapeType[i] & 0x3ff)
#define GetFrame(i) (ObjShapeType[i] >> 10)
#define TILE_FRAME(i) (BaseTile[GetType(i)] + GetFrame(i))
#define GetQuan(i) (((unsigned char *)(Amount + (i)))[0])
#define GetAssoc(i) (*(int16 *)&(ObjPos[i]))
#define GetX(i) ((*(int16 *)&(ObjPos[i].raw[0]))&0x3ff)
#define GetY(i) (((*(int16 *)&(ObjPos[i].raw[1]))>>2)&0x3ff)
#define GetZ(i) (ObjPos[i].raw[2] >> 4)
#define IsNPC(i) ((i) < 0x100 && (i) >= 0)
#define IN_VEHICLE (Active == PartySize)

/*isalpha with the ctype table read unsigned (Turbo C++ 1.00's ctype.h has it signed)*/
#undef isalpha
#define isalpha(c) (((unsigned char *)_ctype)[(c) + 1] & (_IS_UPP | _IS_LOW))

/*Turbo C++ 1.00 increments the argument pointer first for these*/
#undef VA_INT
#undef VA_CHAR
#undef VA_STR
#define VA_INT(ap) (((int16 *)++(ap))[-1])
#define VA_CHAR(ap) (((char *)++(ap))[-2])
#define VA_STR(ap) (((char **)++(ap))[-1])

#define LEFT_CLICK (MouseButtons & 1)
#define RIGHT_CLICK (MouseButtons & 2)

/*03A4*/unsigned char SelectMode = 0;/*0 none, 1 aiming on the map, 2 slot cursor*/
/*03A5*/char AimX = 0;
/*03A6*/char AimY = 0;
/*03A7*/char SelectRange = 0;
/*03A8*/static unsigned char unused_03A8 = 0;/*never used (Ultima VI's SpellbookSelectFirst)*/
/*03A9*/char FreeAimMode = 0;/*free aiming (TAB pressed with no command pending)*/
/*03AA*/static unsigned char CapitalizeNext = 0;/*'`' seen: capitalize the next char (if attrFlags & 4)*/
/*03AB*/char SlotCursorCol = 3;/*slot cursor column (0~2 equipment, 3~6 inventory; bottom row: button #)*/
/*03AC*/char SlotCursorRow = 0;/*slot cursor row (-1 container slot, 0~3 slots, 4 buttons row)*/
/*03AD*/static unsigned char InGetString = 0;/*CON_gets() in progress*/
/*03AE*/unsigned char AnimateAtPrompt = 1;/*run the animations while waiting at the prompt*/
/*03AF*/int16 LastNumberPlural = 1;/*last number printed was != 1 (for '/' and '\\'); cleared at each new line*/
/*03B1*/int16 EndPagingAtPrompt = 0;/*clear the paginate attribute (0x10) at the next prompt*/
/*03B3*/int16 HelpOn = 1;/*highlight '@' keywords*/
/*03B5*/int16 InHiddenSection = 0;/*Ultima VI's '&' section flag: only cleared here*/
/*03B7*/int16 TextOn = 1;
/*03B9*/int16 LastAttackTarget = -1;/*not used in this module*/
/*03BB*/int16 SkipNewlines = 0;/*ignore '\n' in a scrolling window until a printable char*/
/*03BD*/static int16 KbdKeepLastOnly = 1;/*keyboard: keep only the last key in the BIOS buffer*/
/*03BF*/int16 GetsAborted = 0;/*last CON_gets() was aborted with ESC*/
/*03C1*/int16 CookedInput = 1;/*0: return raw keys (the "more" prompt), 1: full command processing*/
/*03C3*/int16 InputToUpper = 1;/*convert input keys to uppercase*/
/*03C5*/int16 StatusMember = 0;/*party member shown in the portrait/equipment view*/
/*03C7*/int16 MousePress = 0;
/*03C9*/int16 MouseTrigger = 0;
/*03CB*/int16 MouseMode = 0;
/*03CD*/int16 StatusDisplay = 0x91;
/*03CF*/int16 PendingCommand = 0x8f;/*command being executed (waiting for a target)*/
/*03D1*/int16 AllowMouseMov = 0;
/*03D3*/int16 MouseMapX = -1;
/*03D5*/int16 MouseMapY = -1;
/*screen pixel used as origin by MkDirection() (88,88: party leader in the map view)*/
/*03D7*/unsigned char DirOriginX = 88;
/*03D8*/unsigned char DirOriginY = 88;
void CON_DrawCharCb(int16 ch, int16 x, int16 y);
void CON_ScrollUpCb(int16 x, int16 y, int16 w, int16 h);
/*03D9*/void (*CON_PutCharFn)(int16, int16, int16) = CON_DrawCharCb;/*"put char" callback*/
/*03DD*/void (*CON_ScrollUpFn)(int16, int16, int16, int16) = CON_ScrollUpCb;/*"scroll up" callback*/
/*03E1*/int16 PromptCh = 5;/*first char of the animated cursor (5~8; 1 for "more")*/
/*initial text window: full 40x25 screen, white on black*/
/*03E3*/struct tClipInfo BootClip = {
	0, 0, 40, 25,
	0, 0,
	0xf, 0,
	0
};
/*03EC*/struct tClipInfo *CurClip = &BootClip;/*current text window*/
/*03EE*/int16 DefaultClipAttr = 1;/*attrFlags given to new windows by CON_createClip (word wrap)*/
/*3x5 font, for the quantities over inventory tiles
  (5 rows of 3 bits, MSB first; the low bit is unused)*/
/*03F0*/uint16 TinyFont[] = {
	0xF6DE,/*'0'*/
	0x4924,/*'1'*/
	0xE7CE,/*'2'*/
	0xE59E,/*'3'*/
	0xB792,/*'4'*/
	0xF39E,/*'5'*/
	0xF3DE,/*'6'*/
	0xE4A4,/*'7'*/
	0xF7DE,/*'8'*/
	0xF79E,/*'9'*/
	0xF7DA,/*'A'*/
	0xD75C,/*'B'*/
	0xF24E,/*'C'*/
	0xD6DC,/*'D'*/
	0xF34E,/*'E'*/
	0xF348,/*'F'*/
	0xF2DE,/*'G'*/
	0xB7DA,/*'H'*/
	0xE92E,/*'I'*/
	0x24DE,/*'J'*/
	0xB75A,/*'K'*/
	0x924E,/*'L'*/
	0xBEDA,/*'M'*/
	0x03DA,/*'N'*/
	0x03DE,/*'O'*/
	0xF7C8 /*'P'*/
};
/*x position (in the 16x16 tile) of each digit, least significant first,
  for a number of 1~4 digits: right aligned*/
/*0424*/unsigned char TinyDigitX[][4] = {
	{ 7,0,0,0},
	{ 9,4,0,0},
	{11,7,3,0},
	{13,9,5,1}
};
/*map view tile last pointed at by the mouse (for continuous walking)*/
/*0434*/int16 LastMouseTileX = -1;
/*0436*/int16 LastMouseTileY = -1;
/*mouse zones {left,top,right,bottom}, see mkMouseCommand()*/
/*0438*/int16 MouseZones[][4] = {
	{  8, 16,167,159},/*map view*/
	{  8,178,169,196},/*command icons*/
	{183,  6,314,118},/*status panel*/
	{  0,  0,319,199} /*the whole screen*/
};
/*equipment view slots: screen x [column] and y [row][column]
(columns 0~2: equipment, 3~6: inventory)*/
/*0458*/int16 SlotScrX[] = {184,208,232,248,264,280,296};
/*0466*/unsigned char SlotScrY[][7] = {
	{24,16,24,32,32,32,32},
	{40,40,40,48,48,48,48},
	{56,64,56,64,64,64,64},
	{80,80,80,80,80,80,80}
};
/*tiles of the buttons row (x = 184 + 16 * i, y = 102) of the portrait and
  equipment views, to erase the slot cursor (set by DrawSlotCursor)*/
/*0482*/int16 SlotButtonTiles[] = {0x183,0x180,0x181,0x184,0x16d};
/*048C*/int16 PendingSelect = 0;/*next CON_getch() returns 0x8e (target) at once*/
/*048E*/unsigned char LineBufLen = 0;/*line buffer: length (= column of the next char)*/
/*048F*/unsigned char LineBufDrawn = 0;/*line buffer: the chars before this column are on screen*/
/*0490*/unsigned char LineAcceptSpace = 1;/*0: skip spaces (at the start of a wrapped line)*/
/*0491*/unsigned char PendingEscape = 0;/*pending CON_putch() escape char or 0*/

int16 CON_putch0(int16 ch);
int16 CON_getch(void);
int16 GAME_Load(void);
static int16 setDefaultCommand(int16 cmd);

/*hide the mouse pointer if it is inside the screen rectangle
  (x0,y0)-(x1,y1), before drawing there.
  returns 1 if it was hidden: pass the result to CON_mouseOn() afterwards*/
/*C_05D0_0004*/CON_mouseOffAt(int16 x0, int16 y0, int16 x1, int16 y1) {
	int16 inArea;

	if(MouseOn) {
		MouseArea.left = x0;
		MouseArea.top = y0;
		MouseArea.right = x1;
		MouseArea.bottom = y1;
		inArea = MOUSE_IN_AREA(&MouseArea);
		if(MOUSE_IN_AREA(&MouseArea))
			MOUSE_HIDE();
	}
	return inArea;
}

/*show the mouse pointer again if mustShow (result of CON_mouseOffAt)*/
/*C_05D0_004D*/CON_mouseOn(int16 mustShow) {
	if(MouseOn && mustShow)
		MOUSE_SHOW();
}

/*direction (0=N,1=NE,2=E ... 7=NW, -1 = none) pointed at by the screen
  pixel (x,y), relative to the origin (DirOriginX,DirOriginY).
  Near the origin (48x48 box) the 16x16 cell decides (-1 for the center cell);
  farther, the slope decides: mostly horizontal/vertical (ratio >= 2) gives
  E/W/N/S, else a diagonal.*/
/*C_05D0_0066*/MkDirection(int16 x, int16 y) {
	int16 dir;
	int16 deltaX, deltaY;

	dir = -1;
	deltaX = abs(x - DirOriginX);
	deltaY = abs(y - DirOriginY);
	if(
		DirOriginX - 24 <= x && DirOriginX + 24 > x &&
		DirOriginY - 24 <= y && DirOriginY + 24 > y
	) {
		if(DirOriginX - 8 > x) {
			if(DirOriginY - 8 > y)
				dir = 7;
			else if(DirOriginY + 8 > y)
				dir = 6;
			else
				dir = 5;
		} else if(DirOriginX + 8 > x) {
			if(DirOriginY - 8 > y)
				dir = 0;
			else if(DirOriginY + 8 > y)
				dir = -1;
			else
				dir = 4;
		} else {
			if(DirOriginY - 8 > y)
				dir = 1;
			else if(DirOriginY + 8 > y)
				dir = 2;
			else
				dir = 3;
		}
	} else {
		if(deltaX > deltaY) {
			if(deltaY == 0 || (deltaX / deltaY) >= 2) {
				dir = (DirOriginX <= x)?2:6;
			}
		} else {
			if(deltaX == 0 || (deltaY / deltaX) >= 2) {
				dir = (DirOriginY <= y)?4:0;
			}
		}
		if(dir == -1)
			dir = (DirOriginX <= x)?((DirOriginY <= y)?3:1):((DirOriginY <= y)?5:7);
	}
	return dir;
}

/*mouse callback (Mouse.onMove), also called after mode changes:
update the mouse pointer shape according to its position.
shapes: 0 arrow (outside the map view), 1 crosshair (MouseMode != 0,
or center of the map), 2~9 direction arrows (2 + direction)*/
/*C_05D0_01F5*/CON_UpdateMousePointer() {
	int16 dir;

	if(MouseOn == 0)
		return;
	if(Mouse.posX >= 8 && Mouse.posX < 168 && Mouse.posY >= 16 && Mouse.posY < 159) {
		if(MouseMode == 0) {
			dir = MkDirection(Mouse.posX, Mouse.posY);
			/*(on one line: no line record, so -O merges it with the last call)*/
			if(dir < 0) MOUSE_POINTER(PointerShapes, 1);
			else
				MOUSE_POINTER(PointerShapes, dir + 2);
		} else {
			MOUSE_POINTER(PointerShapes, 1);
		}
	} else if(MouseMode == 0) {
		MOUSE_POINTER(PointerShapes, 0);
	} else {
		MOUSE_POINTER(PointerShapes, 1);
	}
}

/*"put char" callback (CON_PutCharFn):
draw character ch at text cell (x,y) with the current window's colors*/
/*C_05D0_028B*/void CON_DrawCharCb(int16 ch, int16 x, int16 y) {
	int16 mouseHidden;

	x <<= 3;
	y <<= 3;
	mouseHidden = CON_mouseOffAt(x, y, x + 7, y + 7);
	GrInfo->textBg = CurClip->bgColor;
	GrInfo->textFg = CurClip->fgColor;
	GR_PUT_CHAR(ch, x, y);
	CON_mouseOn(mouseHidden);
}

/*"scroll up" callback (CON_ScrollUpFn):
scroll the text window at cell (x,y) of w x h cells up by one line (8 pixels)
and clear its last line with the background color*/
/*C_05D0_02FD*/void CON_ScrollUpCb(int16 x, int16 y, int16 w, int16 h) {
	int16 mouseHidden;

	x <<= 3;
	y <<= 3;
	w <<= 3;
	h <<= 3;
	mouseHidden = CON_mouseOffAt(x, y, x + w - 1, y + h - 1);
	GR_SCROLL_RECT_Y(x, y, x + w - 1, y + h - 1, -8);
	GR_SET_COLOR(CurClip->bgColor);
	GR_FILL_RECT(x, y + h - 8, x + w - 1, y + h - 1);
	CON_mouseOn(mouseHidden);
}

/*restore the game: throw away the current state and reload the saved
game; the first time (Savage Empire's NPC 0x21 not met yet), start
the game's opening conversation on day 1/1/0*/
/*C_05D0_03A2*/GAME_Restore() {
	int16 i;

	if(InCombat)
		COMBAT_breakOff();
	IsArmageddon = 0;
	MapAnimEnabled = 1;
	SelectMode = 0;
	StatusDisplay = 0x91;
	StatusMember = 0;
	StatusDirty = 1;
	DiskFullChoice = ' ';
	OBJ_DeleteTmpBlocks();
	for(i = 0; i < 0x45; i++)
		ObjBlkTmpPending[i] = 0;
	OBJ_InitLists();
	MAP_InitChunkCache();
	setDefaultCommand(0xff);
	StatusDirty = 1;
	HatchingDisabled = 1;
	GAME_Load();
	if(!(TalkFlags[0x21] & 1)) {
		TALK_talkTo(Active, 0x21, 0);
		Date_D = 1;
		Date_M = 1;
		Date_Y = 0;
		RefreshStatus();
	}
	HatchingDisabled = 0;
	InitNPCTracker();
	MUS_PlayAreaSong();
	PassTime(0);
	VIS_UpdateView();
	ScreenFade = 1;
	DrawMapWindow(1);
	PromptFlag = 1;
}

/*load the game: read "schedule", "basetile" and the saved state in
"savegame\\objlist" (same layout as written by GAME_Save), then rebuild
the map around the party*/
/*C_05D0_047A*/GAME_Load() {
	int16 fd, i;

	BuiltAreaX = BuiltAreaY = BuiltAreaZ = -1;

	DISK_confirm(1);
	fd = OSI_open(/*0492*/"schedule");
	OSI_read(fd, 0, (0x100 + 1) * sizeof(int16), SchedPointer);
	OSI_read(fd, -1, (int32)SchedPointer[0x100] * 5, D_4EF0);
	OSI_close(fd);

	DISK_confirm(1);
	fd = OSI_open(/*049B*/"basetile");
	OSI_read(fd, 0, 0x800, BaseTile);
	OSI_close(fd);

	DISK_confirm(8);
	fd = OSI_open(/*04A4*/"savegame\\objlist");
	OSI_read(fd, 0, 0x100, ObjStatus);
	OSI_read(fd, -1, 0x100 * sizeof(struct coord), ObjPos);
	OSI_read(fd, -1, 0x200, ObjShapeType);
	OSI_read(fd, -1, 0x200, Amount);
	OSI_read(fd, -1, 0x100, D_6CF0);
	OSI_read(fd, -1, 0x100, D_53E8);
	OSI_read(fd, -1, 0x100, D_4606);
	OSI_read(fd, -1, 0x100, D_7644);
	OSI_read(fd, -1, 0x200, D_51E4);
	OSI_read(fd, -1, 0x100, D_6DF4);
	OSI_read(fd, -1, 16 * 14, D_5104);
	OSI_read(fd, -1, 0x10, Party);
	OSI_read(fd, -1, 1, &PartySize);
	OSI_read(fd, -1, 0x100, D_4EEC);
	OSI_read(fd, -1, 0x100, D_45FA);
	OSI_read(fd, -1, 0x100, NPCMode);
	OSI_read(fd, -1, 0x100, D_3298);
	OSI_read(fd, -1, 0x100, NPCComMode);
	OSI_read(fd, -1, 0x100, D_44FA);
	OSI_read(fd, -1, 0x100, D_44FA);
	OSI_read(fd, -1, 0x100 * sizeof(int16), D_B53E);
	OSI_read(fd, -1, 0x100, TalkFlags);
	OSI_read(fd, -1, 0x100, D_5100);
	OSI_read(fd, -1, 0x100, D_4602);
	OSI_read(fd, -1, 0x100, D_45FE);
	OSI_read(fd, -1, (int32)((char *)&GameStateEnd - (char *)&IsOnQuest), &IsOnQuest);
	OSI_close(fd);

	OBJ_RelinkNPCs();
	VarInt['Q' - 0x37] = IsOnQuest;
	InCombat = 0;
	MapX = GetX(Party[Active]);
	MapY = GetY(Party[Active]);
	MapZ = GetZ(Party[Active]);

	for(i = 0; i < NumAnimData; i ++)
		StateAnimData[i] = 1;

	setDefaultCommand(DefaultCommand);
	LoadNewRegions(MapX, MapY, MapZ);
	MAP_LoadChunkIndexes(MapX, MapY, MapZ);

	DISK_confirm(3);
	MoveAppliesTerrain = 1;
	MAP_LoadAround();
}

/*save the game: write the state to "savegame\\objlist" (the party members
aboard a vehicle are put back on the map first)*/
/*C_05D0_08B7*/GAME_Save() {
	int16 fd, i;

	DiskFullChoice = ' ';
	BuiltAreaX = BuiltAreaY = BuiltAreaZ = -1;
	if(IN_VEHICLE) {
		for(i = 0; i < PartySize; i++)
			MoveObj(Party[i], MapX, MapY, MapZ);
	}
	DISK_confirm(8);
	OBJ_CommitBlocks();
	IsOnQuest = VarInt['Q' - 0x37];

	DISK_confirm(8);
	fd = OSI_create(/*04A4*/"savegame\\objlist");
	OSI_write(fd, 0, 0x100, ObjStatus);
	OSI_write(fd, -1, 0x100 * sizeof(struct coord), ObjPos);
	OSI_write(fd, -1, 0x200, ObjShapeType);
	OSI_write(fd, -1, 0x200, Amount);
	OSI_write(fd, -1, 0x100, D_6CF0);
	OSI_write(fd, -1, 0x100, D_53E8);
	OSI_write(fd, -1, 0x100, D_4606);
	OSI_write(fd, -1, 0x100, D_7644);
	OSI_write(fd, -1, 0x200, D_51E4);
	OSI_write(fd, -1, 0x100, D_6DF4);
	OSI_write(fd, -1, 16 * 14, D_5104);
	OSI_write(fd, -1, 0x10, Party);
	OSI_write(fd, -1, 1, &PartySize);
	OSI_write(fd, -1, 0x100, D_4EEC);
	OSI_write(fd, -1, 0x100, D_45FA);
	OSI_write(fd, -1, 0x100, NPCMode);
	OSI_write(fd, -1, 0x100, D_3298);
	OSI_write(fd, -1, 0x100, NPCComMode);
	OSI_write(fd, -1, 0x100, D_44FA);
	OSI_write(fd, -1, 0x100, D_44FA);
	OSI_write(fd, -1, 0x100 * sizeof(int16), D_B53E);
	OSI_write(fd, -1, 0x100, TalkFlags);
	OSI_write(fd, -1, 0x100, D_5100);
	OSI_write(fd, -1, 0x100, D_4602);
	OSI_write(fd, -1, 0x100, D_45FE);
	OSI_write(fd, -1, (int32)((char *)&GameStateEnd - (char *)&IsOnQuest), &IsOnQuest);
	OSI_close(fd);

	MAP_LoadAround();
	StatusDirty ++;
}

/*draw tile ftile over background tile btile at screen (x,y)
(composed in the backbuffer's top-left 16x16 corner, then copied)*/
/*C_05D0_0BF5*/DrawTileOnBg(int16 ftile, int16 btile, int16 x, int16 y) {
	GR_PUT_TILE(btile, 0, 0);
	GR_PUT_TILE(ftile, 0, 0);
	GR_COPY_TO_SCREEN(0, 0, 15, 15, x, y);
}

/*draw a 3x5 character in the backbuffer's 16x16 work tile, at
column xOfs, rows 11~15 (bottom of the tile), in text color.
glyph: 0~9 = '0'-'9', 10~25 = 'A'-'P'*/
/*C_05D0_0C37*/DrawTinyGlyph(int16 glyph, int16 xOfs) {
	int16 px, py;
	uint16 bits;

	GrInfo->toBackBuffer = 1;
	bits = TinyFont[glyph];
	GR_SET_COLOR(ColorText);
	for(py = 11; py < 16; py++) {
		for(px = xOfs; px < xOfs + 3; px ++) {
			if(bits & 0x8000) {
				GR_PUT_PIXEL(px, py);
			}
			bits <<= 1;
		}
	}
	GrInfo->toBackBuffer = 0;
}

/*draw tile ftile on an empty slot at screen (x,y) with the number val
(0~9999, right aligned, 3x5 font) printed over its bottom*/
/*C_05D0_0CA3*/DrawTileWithNumber(int16 ftile, register int16 val, int16 x, int16 y) {
	register int16 numDigits;
	int16 digit;

	GR_PUT_TILE(0x189, 0, 0);
	GR_PUT_TILE(ftile, 0, 0);
	if(val < 10)
		numDigits = 1;
	else if(val < 100)
		numDigits = 2;
	else if(val < 1000)
		numDigits = 3;
	else if(val < 10000)
		numDigits = 4;
	else
		numDigits = 0;
	for(digit = 0; digit < numDigits; digit ++) {
		DrawTinyGlyph(val % 10, TinyDigitX[numDigits - 1][digit]);
		val /= 10;
	}
	GR_COPY_TO_SCREEN(0, 0, 15, 15, x, y);
}

/*draw an inventory object in its slot at screen (x,y): objects counted
on 16 bits (QuanType 4) show their amount, stackable ones (QuanType 2)
their quantity (an amount below 1 is made 1)*/
/*C_05D0_0D53*/DrawInvObject(int16 objNum, int16 x, int16 y) {
	int16 quanType, ftile;

	quanType = QuanType(ObjShapeType[objNum]);
	ftile = TILE_FRAME(objNum);
	if(Amount[objNum] < 1)
		Amount[objNum] = 1;
	if(quanType == 4) {
		GR_PUT_TILE_SCREEN(0x189, x, y);
		DrawTileWithNumber(ftile, Amount[objNum], x, y);
	} else if(quanType == 2) {
		GR_PUT_TILE_SCREEN(0x189, x, y);
		DrawTileWithNumber(ftile, GetQuan(objNum), x, y);
	} else {
		DrawTileOnBg(ftile, 0x189, x, y);
	}
}

/*same as DrawTileOnBg (entry point used by the status views)*/
/*C_05D0_0E16*/DrawSlotTile(int16 ftile, int16 btile, int16 x, int16 y) {
	DrawTileOnBg(ftile, btile, x, y);
}

/*poll for one key at the text cursor of the current window.
while no key is available, draw one frame of the animated cursor
(PromptCh + 0~3) and run the idle animations once.
returns the key code (KBD_GetKey: ASCII, or 0x100 + scan code)
or 0 if none*/
/*C_05D0_0E2D*/CON_prompt() {
	int16 ch;
	int16 promptX, promptY;

	/*the player is reading: reset the "more" line counter*/
	LinesBeforeMore = CurClip->rows;
	if(EndPagingAtPrompt) {
		CurClip->attrFlags &= ~0x10;/*end of paginated text*/
		EndPagingAtPrompt = 0;
	}
	TextIdle = 0;
	if(CurClip->curX >= CurClip->cols)
		CON_putch0(0);/*line full: new line*/
	promptX = CurClip->curX + CurClip->originX;
	promptY = CurClip->curY + CurClip->originY;
	ch = KBD_GetKey(KbdKeepLastOnly);
	if(ch == 0) {/*animated cursor*/
		(*CON_PutCharFn)(((PromptCnt ++) & 3) + PromptCh, promptX, promptY);
		IdleAnimTick = 1;
		if(AnimateAtPrompt) {
			DisplayTick();
			PaletteAnimation();
		} else {
			if(GrAdapter == 0)
				PaletteAnimation();
			TimerDelay(150000L);
		}
		if(MouseOn)
			MOUSE_SERVE();
	}
	(*CON_PutCharFn)(' ', promptX, promptY);
	TextIdle = 1;

	return ch;
}

/*echo one typed character for CON_gets(), directly at the cursor
(no line buffer); '\b' erases the previous char, also across lines*/
/*C_05D0_0F0E*/CON_EchoChar(int16 ch) {
	int16 scrX, scrY;

	scrX = CurClip->curX + CurClip->originX;
	scrY = CurClip->curY + CurClip->originY;
	TextIdle = 0;
	if(ch == '\b') {/*backspace*/
		if(CurClip->curX) {
			CurClip->curX --;
			scrX --;
		} else if(CurClip->curY) {
			CurClip->curY --;
			scrY --;
			CurClip->curX = CurClip->cols - 1;
			scrX = CurClip->curX + CurClip->originX;
		}
		(*CON_PutCharFn)(' ', scrX, scrY);
	} else {
		(*CON_PutCharFn)(ch, scrX, scrY);
		if(++CurClip->curX >= CurClip->cols) {
			CurClip->curX = 0;
			if(CurClip->curY < CurClip->rows - 1) {
				CurClip->curY ++;
			} else if(CurClip->attrFlags & 0x20) {
				TextIdle = 1;
				(*CON_ScrollUpFn)(CurClip->originX, CurClip->originY, CurClip->cols, CurClip->rows);
				TextIdle = 0;
			}
		}
	}
	TextIdle = 1;
}

/*draw string str directly at text cell (x,y), no clipping (uncalled?)*/
/*C_05D0_0FE8*/CON_PutStrAt(unsigned char *str, int16 x, int16 y) {
	while(*str) {
		(*CON_PutCharFn)(*str++, x++, y);
	}
}

/*mouse held over the map view without command: return the walking
direction (MkDirection, -1 = center) and, if AllowMouseMov, update the
walk target MouseMapX/Y (map coordinates of the pointed tile). The target
is kept while the pointer stays on the same tile, unless the party is next
to it on the same row/column or the button was just pressed.*/
/*C_05D0_1011*/mkMouseDir() {
	int16 tileX, tileY;
	int16 dir;

	tileX = SCR2TIL(PointerX);
	tileY = SCR2TIL(PointerY);
	dir = MkDirection(PointerX, PointerY);

	if(AllowMouseMov) {
		if(
			MouseTrigger || MouseMapX == -1 ||
			(MouseMapX == MapX && abs(MouseMapY - MapY) < 2) ||
			(MouseMapY == MapY && abs(MouseMapX - MapX) < 2) ||
			tileX != LastMouseTileX || tileY != LastMouseTileY
		) {
			MouseMapX = tileX + MapX - 5;
			MouseMapY = tileY + MapY - 5;
			LastMouseTileX = tileX;
			LastMouseTileY = tileY;
		}
	} else {
		MouseMapX =
		MouseMapY = -1;
	}

	return dir;
}

/*fill Selection (x, y, obj) from the pointer position PointerX/Y.
- map view: Selection.x/y = map coordinates; returns 0x9d if there is
  an object (COMBAT_GetTargetAt), else Selection.obj = -1
  and SelectionIsDark tells if the tile is hidden/darkness.
- equipment view: returns 0x9b for an inventory slot or the container
  slot (STAT_PickInvenSlot), 0x9c for an equipment slot (STAT_PickEquipSlot).
returns 0 when nothing selectable is pointed at*/
static /*C_05D0_10C0*/mkMouseSelection() {
	int16 tile;

	SelectionIsDark = 0;
	if(PointerX < 168) {
		SelectionX = SCR2TIL(PointerX) + MapX - 5;
		SelectionY = SCR2TIL(PointerY) + MapY - 5;
		if(Obj_11x11[SCR2TIL(PointerY)][SCR2TIL(PointerX)] || Tile_11x11[SCR2TIL(PointerY)][SCR2TIL(PointerX)] >= 0x200) {
			SelectionObj = COMBAT_GetTargetAt(Party[Active], SelectionX, SelectionY);
			return 0x9d;
		}
		SelectionObj = -1;
		tile = Tile_11x11[SCR2TIL(PointerY)][SCR2TIL(PointerX)];
		SelectionIsDark = (tile == 0xff || tile == 0x1bc);
	} else if(StatusDisplay == 0x92) {
		SelectionX = -1;
		SelectionY = -1;
		if(STAT_PickInvenSlot())
			return 0x9b;
		if(STAT_PickEquipSlot())
			return 0x9c;
	} else {
		SelectionX = -1;
		SelectionY = -1;
		SelectionObj = -1;
	}
	return 0;
}

/*mouse click in the party view: returns the command to execute (0 = none).
Each member line is 16 pixels high; clicking the portrait column (x > 200)
opens his portrait, the column before it (184 < x <= 200) his equipment.
Selection.obj = party index*/
static /*C_05D0_11D2*/MouseInPartyView() {
	int16 cmd;

	cmd = 0;
	if(!IN_VEHICLE && PointerY >= 8) {
		if(PointerX > 200) {/*to portrait view*/
			cmd = 0x90;
			SelectionObj = PartyListTop + SCR2TIL(PointerY - 12);
			if(PartySize <= SelectionObj)
				cmd = 0;
		} else if(PointerX > 184) {/*to equipment view*/
			cmd = 0x92;
			SelectionObj = PartyListTop + SCR2TIL(PointerY - 12);
			if(PartySize <= SelectionObj)
				cmd = 0;
		}
	}

	return cmd;
}

/*mouse click in the portrait view: only the buttons row (y >= 102) reacts.
returns the command (0 = none)*/
static /*C_05D0_1247*/MouseInPortraitView() {
	int16 cmd;

	cmd = 0;
	if(PointerY >= 102) {
		if(PointerX >= 200 && PointerX < 216) {/*to party view*/
			cmd = 0x91;
		} else if(PointerX >= 216 && PointerX < 232) {/*to equipment view*/
			cmd = 0x92;
			SelectionObj = -1;
		} else if(PointerX >= 232 && PointerX < 248) {/*next member*/
			cmd = 0x99;
		} else if(PointerX >= 184 && PointerX < 200) {/*previous member*/
			cmd = 0x9a;
		}
	}
	return cmd;
}

/*mouse click (or slot cursor validation) in the equipment/inventory view.
returns the command (0 = none). The combat mode button (248,102; not for
the player nor an NPC out of the party's control) cycles the member's
combat mode here directly*/
static /*C_05D0_12B1*/MouseInEquipView() {
	int16 cmd, selKind;
	int16 objType;

	cmd = 0;
	if(IsInConversation)
		return cmd;

	/*combat mode button: cycles 2, 3, 4, 5, 7, 8*/
	if(
		StatusMember != 0 &&
		PointerX >= 248 && PointerX < 264 &&
		PointerY >= 102 &&
		!(ObjStatus[Party[StatusMember]] & 0x80)
	) {
		NPCComMode[Party[StatusMember]] = NPCComMode[Party[StatusMember]] + 1;
		if(NPCComMode[Party[StatusMember]] == 9)
			NPCComMode[Party[StatusMember]] = 2;
		else if(NPCComMode[Party[StatusMember]] == 5)
			NPCComMode[Party[StatusMember]] = 7;
		else if(NPCComMode[Party[StatusMember]] == 3)
			NPCComMode[Party[StatusMember]] = 4;
		if(
			InCombat &&
			Active != StatusMember &&
			!(ObjStatus[Party[StatusMember]] & 4)
		) NPCMode[Party[StatusMember]] = NPCComMode[Party[StatusMember]];
		STAT_refreshComMode();
	} else if(PointerY >= 102) {
		if(PointerX >= 200 && PointerX < 216) {/*to party view*/
			cmd = 0x91;
		} else if(PointerX >= 216 && PointerX < 232) {/*to portrait view*/
			cmd = 0x90;
			SelectionObj = -1;
		} else if(PointerX >= 232 && PointerX < 248) {/*next member*/
			cmd = 0x99;
		} else if(PointerX >= 184 && PointerX < 200) {/*previous member*/
			cmd = 0x9a;
		}
	} else if(
		PointerX >= 272 && PointerX < 288 &&
		PointerY >= 16 && PointerY < 32 &&
		MouseMode == 0
	) {/*container slot (top right): back to the parent container/owner*/
		cmd = 0x9f;
	} else if(MouseMode == 0 && PendingCommand != 0x81) {
		selKind = mkMouseSelection();
		if(SelectionObj != -1) {
			if(selKind == 0x9b) {
				/*inventory object: open it if it is a container, else ready it*/
				objType = GetType(SelectionObj);
				if(IsContainerType(objType))
					cmd = 0xa0;
				else
					cmd = 0xa7;
			} else if(selKind == 0x9c) {
				/*equipped object: unready it*/
				cmd = 0xa8;
			}
		}
	}

	return cmd;
}

/*set the default command (0x81~0x89, or 0xff = none), executed by a
right click; it is shown by a frame around its icon (icons are 18 pixels
wide from x = 8, between y = 178 and y = 196)*/
static /*C_05D0_14F0*/setDefaultCommand(int16 cmd) {
	int16 x;

	if(MouseOn)
		MOUSE_HIDE();
	if(DefaultCommand != 0xff) {
		GR_SET_COLOR(D_24EE);
		x = TIL2SCR(DefaultCommand - 0x81) + ((DefaultCommand - 0x81) << 1);
		GR_FILL_RECT(x + 8, 178, x + 26, 178);
		GR_FILL_RECT(x + 8, 196, x + 26, 196);
		GR_FILL_RECT(x + 8, 178, x + 8, 196);
		GR_FILL_RECT(x + 26, 178, x + 26, 196);
	}
	DefaultCommand = cmd;
	if(DefaultCommand != 0xff) {
		GR_SET_COLOR(ColorBrown);
		x = TIL2SCR(DefaultCommand - 0x81) + ((DefaultCommand - 0x81) << 1);
		GR_FILL_RECT(x + 8, 178, x + 26, 178);
		GR_FILL_RECT(x + 8, 196, x + 26, 196);
		GR_FILL_RECT(x + 8, 178, x + 8, 196);
		GR_FILL_RECT(x + 26, 178, x + 26, 196);
	}
	if(MouseOn)
		MOUSE_SHOW();
}

/*translate the current mouse button state (MouseButtons, MouseTrigger = just
pressed) at PointerX/Y into a command code (0 = none), by zone (MouseZones):
- map view: walk (0x80 + AdvanceDir; repeated while held unless the last
  step was blocked, AdvanceBlocked), pass (center), target (0x8e when a command
  waits for one, MouseMode != 0), right click = default command (0xa9)
- icons: left click = command icon, right click = set the default command
- status panel: depends on StatusDisplay
- elsewhere: pass*/
static /*C_05D0_1668*/mkMouseCommand() {
	int16 mouseCmd, zone;
	int16 mouseDir;

	mouseCmd = 0;

	/*(zone 3 is the whole screen: always found)*/
	for(zone = 0; ; zone ++) {
		if(
			MouseZones[zone][0] <= PointerX && MouseZones[zone][2] >= PointerX &&
			MouseZones[zone][1] <= PointerY && MouseZones[zone][3] >= PointerY
		) break;
	}
	switch(zone) {
		case 0:/*map view*/
			if(MouseMode == 0) {
				if(DefaultCommand != 0xff && RIGHT_CLICK) {
					if(MouseTrigger) {
						mouseCmd = 0xa9;
						mkMouseSelection();
					}
				} else {
					mouseDir = mkMouseDir();
					if(mouseDir == -1) {
						/*(on one line, merged with case 0x9e below)*/
						if(MouseTrigger) mouseCmd = 0x8f;
						else
							mouseCmd = 0;
					} else if(MouseTrigger || AdvanceBlocked == 0 || mouseDir != AdvanceDir) {
						mouseCmd = 0x80;
						AdvanceDir = mouseDir;
					}
				}
			} else if(MouseTrigger) {
				mouseCmd = 0x8e;
				mkMouseSelection();
			}
		break;
		case 1:/*icons: 9 icons of 18 pixels from x = 8, 0x81~0x89*/
			if(MouseTrigger) {
				if(LEFT_CLICK)
					mouseCmd = 0x81 + (PointerX - 8) / 18;
				else
					setDefaultCommand(0x81 + (PointerX - 8) / 18);
			}
		break;
		case 2:/*status panel*/
			if(MouseTrigger) {
				if(DefaultCommand != 0xff && MouseMode == 0 && RIGHT_CLICK) {
					mouseCmd = 0xa9;
					mkMouseSelection();
				} else {
					switch(StatusDisplay) {
						case 0x91: mouseCmd = MouseInPartyView(); break;
						case 0x90: mouseCmd = MouseInPortraitView(); break;
						case 0x92:
							mouseCmd = MouseInEquipView();
							if(mouseCmd == 0 && MouseMode == 1 && mkMouseSelection())
								mouseCmd = 0x8e;
						break;
						case 0x9e: mouseCmd = 0x8f; break;
					}
				}
			}
		break;
		case 3:/*the whole screen*/
			if(MouseTrigger) {
				mouseCmd = 0x8f;
				SelectionX =
				SelectionY = -1;
			}
		break;
	}

	return mouseCmd;
}

/*keyboard "slot cursor" of the equipment/portrait views (SelectMode 2),
at column SlotCursorCol / row SlotCursorRow:
 mode 1: draw the cursor,
 mode 0: erase it (redraw the slot's object or button),
 mode 2: move PointerX/Y on the slot (to validate it like a mouse click).
In every mode PointerX/Y are first set inside the slot, so that
mkMouseSelection()/MouseInEquipView() can hit-test it.*/
/*C_05D0_1834*/DrawSlotCursor(int16 mode) {
	int16 x, y;
	int16 selKind, objType, mouseHidden;

	if(SelectMode != 2)
		return;
	if(StatusDisplay != 0x92 && StatusDisplay != 0x90)
		return;
	if(StatusMember == 0) {
		SlotButtonTiles[0] = 0x189;
		SlotButtonTiles[4] = 0x16c;
	} else if(ObjStatus[Party[StatusMember]] & 0x80) {
		SlotButtonTiles[0] = 0x183;
		SlotButtonTiles[4] = 0x16c;
	} else {
		SlotButtonTiles[0] = 0x183;
		SlotButtonTiles[4] = 0x16d;
	}
	if(PartySize - 1 == StatusMember)
		SlotButtonTiles[3] = 0x189;
	else
		SlotButtonTiles[3] = 0x184;
	if(StatusDisplay == 0x90)
		SlotButtonTiles[2] = 0x182;
	else
		SlotButtonTiles[2] = 0x181;
	if(StatusDisplay == 0x90) {
		SlotCursorRow = 4;
		if(SlotCursorCol > 3)
			SlotCursorCol = 3;
	}
	if(SlotCursorRow < 0) {
		/*container/owner slot*/
		x = PointerX = 272;
		y = PointerY = 16;
	} else if(SlotCursorRow == 3) {
		/*last inventory row*/
		if(SlotCursorCol < 3)
			SlotCursorCol = 3;
		PointerX = SlotCursorCol * 16 + 200;
		PointerY = SlotCursorRow * 16 + 32;
	} else if(SlotCursorRow > 3) {
		/*buttons row*/
		if(SlotCursorCol > 4)
			SlotCursorCol = 4;
		x = PointerX = SlotCursorCol * 16 + 184;
		y = PointerY = 102;
	} else if(SlotCursorCol < 3) {
		/*equipment slots*/
		PointerX = SlotCursorCol * 24 + 184;
		PointerY = SlotCursorRow * 20 + 24;
	} else {
		/*inventory slots*/
		PointerX = SlotCursorCol * 16 + 200;
		PointerY = SlotCursorRow * 16 + 32;
	}
	if(SlotCursorRow >= 0 && SlotCursorRow < 4) {
		x = SlotScrX[SlotCursorCol];
		y = SlotScrY[SlotCursorRow][SlotCursorCol];
	}
	mouseHidden = CON_mouseOffAt(x, y, x + 15, y + 15);
	if(mode == 1) {
		GR_PUT_TILE_SCREEN(0x17d, x, y);
	} else if(mode == 0) {
		selKind = mkMouseSelection();
		if(SlotCursorRow > 3) {
			GR_PUT_TILE_SCREEN(SlotButtonTiles[SlotCursorCol], x, y);
		} else if((selKind == 0x9b || selKind == 0x9c) && SelectionObj >= 0) {
			if(SlotCursorRow < 0) {
				/*container slot: object, or character facing south*/
				if(IsNPC(SelectionObj)) {
					objType = GetType(SelectionObj);
					DrawTileOnBg(OBJ_MakeDirFrame(objType, 4) + BaseTile[objType], 0x189, x, y);
				} else {
					DrawTileOnBg(TILE_FRAME(SelectionObj), 0x189, x, y);
				}
			} else {
				DrawInvObject(SelectionObj, x, y);
			}
		} else if(
			SlotCursorRow == 1 && SlotCursorCol == 2 &&
			((!ShowNPCInven && Party_2Handed[StatusMember]) || (ShowNPCInven && NPC_2Handed))
		) {
			/*left hand slot taken by a two-handed weapon*/
			GR_PUT_TILE_SCREEN(0x187, x, y);
		} else {
			/*empty slot*/
			GR_PUT_TILE_SCREEN(0x188, x, y);
		}
	} else if(mode == 2) {
		PointerX = x;
		PointerY = y;
	}
	CON_mouseOn(mouseHidden);
}

/*wait for the next input and translate it into a key or a command code.
- mouse: mkMouseCommand(); while the button is held, the command produced
  by the press (MouseHeldCmd) is repeated (e.g. walking), others are ignored.
- keyboard: direction keys -> 0x80 + AdvanceDir, <ALT+letter> sets the
  default command; then according to SelectMode
  (0 none, 1 aiming cursor AimX/AimY on the map, 2 slot cursor in the
  equipment/portrait view):
  the command letter again or <ENTER> validates, <SPACE>/<ESC> cancels,
  <TAB> switches between map aiming and slot cursor, '1'~'9' selects a
  party member; plus the status view hotkeys ('*' '/' F10 '+' '-' F1~F8 '`').
CookedInput == 0 returns the raw key (used for "more" prompts).
returns a key code or a command code, never 0*/
/*C_05D0_1B11*/CON_ReadCommand() {
	int16 ch, buttons;
	int16 mouseX, mouseY, newX, newY;

	if(CookedInput)
		CON_putch0(-1);
	ch = 0;
	LinesBeforeMore = CurClip->rows;
	while(!ch) {
		/* */
		if(MouseOn) {
			MOUSE_SERVE();
			buttons = (Mouse.buttonL?1:0) | (Mouse.buttonR?2:0);
			mouseX = Mouse.posX;
			mouseY = Mouse.posY;
			if(buttons & 3) {
				if(!MousePress) {
					MousePress =
					MouseTrigger = 1;
				} else {
					MouseTrigger = 0;
				}
				PointerX = mouseX;
				PointerY = mouseY;
				MouseButtons = buttons;
				ch = mkMouseCommand();
				if(MouseTrigger)
					MouseHeldCmd = ch;
				else if(MouseHeldCmd != ch)
					ch = 0;
			} else {
				MousePress =
				MouseTrigger = 0;
			}
		}
		if(ch)
			continue;
		/* */
		ch = CON_prompt();
		if(ch) {
			MouseMapX =
			MouseMapY = -1;
		}
		if(InputToUpper && ch < 0x100)
			ch = toupper(ch);
		/*0x1xx = 0x100 + BIOS scan code (arrows/keypad, <ALT+key>)*/
		switch(ch) {
			case 0x148: ch = 0x80; AdvanceDir = 0; break;/*up*/
			case 0x150: ch = 0x80; AdvanceDir = 4; break;/*down*/
			case 0x14b: ch = 0x80; AdvanceDir = 6; break;/*left*/
			case 0x14d: ch = 0x80; AdvanceDir = 2; break;/*right*/
			case 0x147: ch = 0x80; AdvanceDir = 7; break;/*up left*/
			case 0x149: ch = 0x80; AdvanceDir = 1; break;/*up right*/
			case 0x14f: ch = 0x80; AdvanceDir = 5; break;/*down left*/
			case 0x151: ch = 0x80; AdvanceDir = 3; break;/*down right*/
			case 0x11e: setDefaultCommand(0x87); ch = 0; break;/*<ALT A>*/
			case 0x114: setDefaultCommand(0x85); ch = 0; break;/*<ALT T>*/
			case 0x126: setDefaultCommand(0x86); ch = 0; break;/*<ALT L>*/
			case 0x122: setDefaultCommand(0x82); ch = 0; break;/*<ALT G>*/
			case 0x120: setDefaultCommand(0x83); ch = 0; break;/*<ALT D>*/
			case 0x132: setDefaultCommand(0x81); ch = 0; break;/*<ALT M>*/
			case 0x116: setDefaultCommand(0x84); ch = 0; break;/*<ALT U>*/
			case 0x113: setDefaultCommand(0x88); ch = 0; break;/*<ALT R>*/
			case 0x130: setDefaultCommand(0x89); ch = 0; break;/*<ALT B>*/
		}
		if(MouseMode == 0 && !FreeAimMode) {
			DrawSlotCursor(0);
			SelectMode = 0;
		}
		/*raw key wanted: the loop ends since ch != 0 (unless no key)*/
		if(CookedInput == 0 || (InGetString && ch == '\r'))
			continue;
		if(SelectMode != 0) {
			/*pressing the pending command's letter again = <ENTER>*/
			if(
				(PendingCommand == 0x87 && ch == 'A') ||
				(PendingCommand == 0x85 && ch == 'T') ||
				(PendingCommand == 0x86 && ch == 'L') ||
				(PendingCommand == 0x82 && ch == 'G') ||
				(PendingCommand == 0x83 && ch == 'D') ||
				(PendingCommand == 0x81 && ch == 'M') ||
				(PendingCommand == 0x84 && ch == 'U')
			) ch = '\r';

			if(ch == 9) {
				/*<TAB>: map aiming <-> slot cursor in the equipment view*/
				if(SelectMode == 1) {
					SelectMode = 2;
					if(StatusDisplay != 0x92) {
						if(IN_VEHICLE)
							StatusMember = 0;
						else
							StatusMember = Active;
						StatusDisplay = 0x92;
						StatusDirty = 1;
						RefreshStatus();
					}
					DrawSlotCursor(1);
				} else if(StatusDisplay != 0x9e && !IsInConversation) {
					DrawSlotCursor(0);
					SelectMode = 1;
				}
				DrawMapWindow(1);
				ch = 0;
			} else if(ch == ' ' || ch == 27) {
				DrawSlotCursor(0);
				SelectMode = 0;
				ch = 0;
			} else if(ch == '\r') {
				if(SelectMode == 1) {
					/*target = aimed map tile; free aiming
					(no command) runs the default command*/
					ch = 0x8e;
					if(MouseMode == 0) {
						if(FreeAimMode && DefaultCommand != 0xff)
							ch = 0xa9;
						FreeAimMode = 0;
					}
					SelectMode = 0;
					PointerX = TIL2SCR(AimX);
					PointerY = TIL2SCR(AimY);
					mkMouseSelection();
				} else if(SelectMode == 2) {
					if(StatusDisplay == 0x92 || StatusDisplay == 0x90) {
						DrawSlotCursor(2);
						if(StatusDisplay == 0x92)
							ch = MouseInEquipView();
						else
							ch = MouseInPortraitView();
						if(ch == 0 && MouseMode == 1 && mkMouseSelection())
							ch = 0x8e;
					}
				}
			} else if(ch == 0x80) {
				if(SelectMode == 1) {
					/*move the aiming cursor inside the map view, within
					SelectRange of the party member; SelectRange -1 means
					"adjacent tile": the direction selects at once*/
					ch = 0;
					newX = AimX + DirIncrX[AdvanceDir];
					newY = AimY + DirIncrY[AdvanceDir];
					if(
						newX >= 0 && newX < 11 &&
						newY >= 1 && newY < 10 &&
						COMBAT_TileDistance(Party[Active], newX + MapX - 5, newY + MapY - 5) <= SelectRange
					) {
						AimX = newX;
						AimY = newY;
						DrawMapWindow(1);
					} else if(SelectRange == -1) {
						ch = 0x8e;
						PointerX = TIL2SCR(newX);
						PointerY = TIL2SCR(newY);
						mkMouseSelection();
					}
				} else if(SelectMode == 2 && StatusDisplay == 0x92) {
					/*move the slot cursor; past the edges it goes to
					the container slot (up) or to the buttons row (down)*/
					ch = 0;
					newX = SlotCursorCol + DirIncrX[AdvanceDir];
					newY = SlotCursorRow + DirIncrY[AdvanceDir];
					if(
						newX >= 0 && newX < 7 &&
						newY >= 0 && newY < 4
					) {
						DrawSlotCursor(0);
						if(newX == 1 && newY == 1) {/*skip the figure's body*/
							newX += DirIncrX[AdvanceDir];
							newY += DirIncrY[AdvanceDir];
						}
						SlotCursorCol = newX;
						SlotCursorRow = newY;
						DrawSlotCursor(1);
					} else if((newX > 2 && newX < 7) || newY > 3) {
						if(newY < 0) {
							DrawSlotCursor(0);
							SlotCursorRow = 0xff;
							DrawSlotCursor(1);
						} else if(newY > 3) {
							DrawSlotCursor(0);
							SlotCursorRow = 4;
							SlotCursorCol = newX;
							if(SlotCursorCol > 4)
								SlotCursorCol = 4;
							else if(SlotCursorCol < 0)
								SlotCursorCol = 0;
							DrawSlotCursor(1);
						}
					}
				} else if(SelectMode == 2 && StatusDisplay == 0x90) {
					ch = 0;
					newX = SlotCursorCol + DirIncrX[AdvanceDir];
					if(newX < 0)
						newX = 0;
					else if(newX > 3)
						newX = 3;
					DrawSlotCursor(0);
					SlotCursorCol = newX;
					SlotCursorRow = 3;
					DrawSlotCursor(1);
				}
			} else if(ch >= '1' && ch - '1' < PartySize && SelectMode == 1) {
				/*'1'~: select a party member as target*/
				SelectionIsDark = 0;
				SelectMode = 0;
				SelectionObj = Party[ch - '1'];
				if(IN_VEHICLE) {
					SelectionX = MapX;
					SelectionY = MapY;
				} else {
					SelectionX = GetX(SelectionObj);
					SelectionY = GetY(SelectionObj);
				}
				ch = 0x8e;
			}
		} else if(ch == 9 && StatusDisplay != 0x9e && !IsInConversation) {
			/*<TAB> with no command pending: free aiming (FreeAimMode)*/
			if(StatusDisplay == 0x92) {
				SelectMode = 2;
				DrawSlotCursor(1);
			} else if(StatusDisplay != 0xa1) {
				SelectMode = 1;
			}
			if(SelectMode != 0) {
				AimX =
				AimY = 5;
				SelectRange = 7;
				FreeAimMode = 1;
				DrawMapWindow(1);
				ch = 0;
			}
		}
		if(StatusDisplay == 0x92 && !IsInConversation) {
			/*equipment view*/
			if(ch == '*') {
				ch = 0x90;
				SelectionObj = -1;
			} else if(ch == '/' || ch == 0x144/*F10*/) {
				ch = 0x91;
			} else if(ch == '+') {
				ch = 0x99;
			} else if(ch == '-') {
				ch = 0x9a;
			} else if(ch == '`' || ch == '~') {
				/*"click" on the combat mode button*/
				PointerX = 248;
				PointerY = 102;
				MouseInEquipView();
				ch = 0;
			} else if(ch >= 0x13b/*F1*/ && PartySize + 0x13b > ch) {
				SelectionObj = ch - 0x13b;
				ch = 0x92;
			}
		} else if(StatusDisplay == 0x90 && !IsInConversation) {
			/*portrait view*/
			if(ch == '*') {
				ch = 0x92;
				SelectionObj = -1;
			} else if(ch == '/' || ch == 0x144/*F10*/) {
				ch = 0x91;
			} else if(ch == '+') {
				ch = 0x99;
			} else if(ch == '-') {
				ch = 0x9a;
			} else if(ch >= 0x13b && PartySize + 0x13b > ch) {
				SelectionObj = ch - 0x13b;
				ch = 0x90;
			}
		} else if(StatusDisplay == 0x91 && !IsInConversation) {
			/*party view*/
			if(!IN_VEHICLE && ch >= 0x13b/*F1*/ && PartySize + 0x13b > ch) {
				SelectionObj = ch - 0x13b;
				ch = 0x92;
			}
		}
	}

	return ch;
}

/*get char:
wait for a key or a command (CON_ReadCommand), the cursor in text color
(the "more" cursor, PromptCh 1) . The status panel commands (view changes,
containers, ready/unready) are executed here and the wait goes on; any
other key/command is returned.
0xa9 (default command on a right-clicked target) returns DefaultCommand
and, except for 0x8a, rest and save, makes the next call return 0x8e at
once since Selection is already filled*/
/*C_05D0_22C8*/CON_getch() {
	int16 ch, endLoop;
	int16 savedFg;

	savedFg = CurClip->fgColor;
	if(PromptCh == 1)
		CurClip->fgColor = ColorText;
	if(PendingSelect) {
		PendingSelect = 0;
		return 0x8e;
	}
	do {
		endLoop = 0;
		ch = CON_ReadCommand();
		switch(ch) {
			case 0xa9:/*default action*/
				ch = DefaultCommand;
				if(ch != 0x8a && ch != 0x88 && ch != 0xaa)
					PendingSelect = 1;
				endLoop = 1;
			break;
			case 0x91:/*party view*/
				SelectMode = 0;
				StatusDisplay = 0x91;
				StatusMember = 0;
				StatusDirty = 1;
			break;
			case 0x90:/*portrait view*/
				if(!IN_VEHICLE) {
					StatusDisplay = 0x90;
					if(SelectionObj >= 0)
						StatusMember = SelectionObj;
					StatusDirty = 1;
				}
			break;
			case 0x92:/*equipment view*/
				if(!IN_VEHICLE) {
					StatusDisplay = 0x92;
					if(SelectionObj >= 0)
						StatusMember = SelectionObj;
					StatusDirty = 1;
				}
			break;
			case 0x9a:/*previous member*/
				if(StatusMember > 0) {
					StatusMember --;
					StatusDirty = 1;
					SelectionObj = -1;
				}
			break;
			case 0x99:/*next member*/
				if(PartySize - 1 > StatusMember) {
					StatusMember ++;
					StatusDirty = 1;
					SelectionObj = -1;
				}
			break;
			case 0x9f:/*leave the container shown: back to its owner/parent container, or from a character to the party view*/
				if(IsNPC(InvenOwner)) {
					StatusDisplay = 0x91;
					StatusMember = 0;
					StatusDirty = 1;
				} else {
					InvenOwner = GetAssoc(InvenOwner);
					InvenScroll = 0;
					STAT_RefreshInventory(InvenOwner);
					DrawSlotCursor(1);
				}
			break;
			case 0xa0:/*show the content of container Selection.obj*/
				InvenOwner = SelectionObj;
				InvenScroll = 0;
				STAT_RefreshInventory(InvenOwner);
				DrawSlotCursor(1);
			break;
			case 0xa7:/*Ready*/
				STAT_Ready();
				DrawSlotCursor(1);
			break;
			case 0xa8:/*Unready*/
				STAT_Unready();
				DrawSlotCursor(1);
			break;
			default:
				endLoop = 1;
			case 0xa6: ;/*only refreshes the status panel*/
		}
		if(!endLoop)
			RefreshStatus();
	} while(!endLoop);
	CurClip->fgColor = savedFg;

	return ch;
}

/*read a line of at most maxLen printable chars into buf, echoing it.
<ENTER> validates, <ESC> erases it and aborts (GetsAborted = 1, buf = "").
returns the length*/
/*C_05D0_2494*/CON_gets(char *buf, int16 maxLen) {
	int16 len, done;
	int16 ch;

	InGetString = 1;
	InputToUpper = 0;
	CON_putch0(-1);
	len =
	done = 0;
	KbdKeepLastOnly = 0;
	while(!done) {
		while(!(ch = CON_prompt()));
		switch(ch) {
			case '\r':
				buf[len] = 0;
				GetsAborted = 0;
				done ++;
			break;
			case 27:
				buf[0] = 0;
				while(len --)
					CON_EchoChar(8);
				GetsAborted = 1;
				done ++;
			break;
			case 0x10e:/*backspace*/
				if(len) {
					len --;
					CON_EchoChar(8);
				}
			break;
			default:
				if(len < maxLen && ch >= ' ' && ch <= '~') {
					buf[len] = ch;
					len ++;
					CON_EchoChar(ch);
				}
		}
	}
	KbdKeepLastOnly = 1;
	LinesBeforeMore = CurClip->rows;
	InputToUpper = 1;
	InGetString = 0;
	return len;
}

/*initialize text window clip on the text cells (x0,y0)-(x1,y1)
(inclusive) with the default colors/attributes, and make it current*/
/*C_05D0_2562*/CON_createClip(struct tClipInfo *clip, int16 x0, int16 y0, int16 x1, int16 y1) {
	CurClip = clip;
	clip->originX = x0;
	clip->originY = y0;
	clip->cols = x1 - x0 + 1;
	clip->rows = y1 - y0 + 1;
	clip->curX = clip->curY = 0;
	clip->bgColor = ColorPaper;
	clip->fgColor = ColorText;
	clip->attrFlags = DefaultClipAttr;
}

/*make clip the current text window (the pending text of the previous one
is drawn first)*/
/*C_05D0_25AD*/CON_setClip(struct tClipInfo *clip) {
	CON_putch0(-1);
	CurClip = clip;
	GrInfo->textBg = CurClip->bgColor;
	GrInfo->textFg = CurClip->fgColor;
	CON_putch0(-1);
}

/*move the text cursor of the current window to (x,y), clamped to it*/
/*C_05D0_25E4*/CON_gotoxy(int16 x, int16 y)
{
	if(x < 0)
		x = 0;
	if(y < 0)
		y = 0;
	if(x > CurClip->cols)
		x = CurClip->cols;
	if(y > CurClip->rows)
		y = CurClip->rows;
	CON_putch0(-1);
	CurClip->curX = x;
	CurClip->curY = y;
	CON_putch0(-1);
}

/*low level character output to the current window (no escapes).
ch >= 0: the char is appended to the line buffer (LineBufChars/LineBufColors);
  0 or '\n' ends the line. When the line is complete (newline, or buffer
  full = window width) it is drawn: with word wrap (attrFlags & 1) the line is
  cut after the last ' ' or '-' and the rest is carried to the next line,
  with attrFlags & 2 it is centered. Then the cursor goes to the next line,
  scrolling (attrFlags & 0x20) at the bottom. With attrFlags & 0x10 a "more"
  prompt waits for a key when the window is full of new text (LinesBeforeMore).
ch < 0 (-1): flush: draw the pending part of the line without ending it
  (done before moving the cursor, changing the window, or reading input)*/
/*C_05D0_264F*/CON_putch0(int16 ch) {
	register int16 key, row;
	int16 i, breakPos, unusedMidX, savedFg;

	if(!TextOn)
		return;
	/*skip empty lines after an attack message (see SkipNewlines)*/
	if(SkipNewlines && (CurClip->attrFlags & 0x20)) {
		if(ch == '\n')
			return;
		if(ch >= 0)
			SkipNewlines = 0;
	}
	TextIdle = 0;
	if((CurClip->attrFlags & 4) && CapitalizeNext)
		ch = toupper(ch);
	CapitalizeNext = 0;
	if(ch == 0)
		ch = '\n';
	/*runic font: upper half of the character set*/
	if((CurClip->attrFlags & 8) && ch > ' ')
		ch |= 0x80;
	if(CurClip->cols == LineBufLen || ch == '\n' || ch < 0) {
		/*-- line complete (or flush): draw it --*/
		if(ch == '\n' || ch < 0)
			LastNumberPlural = 0;
		if(LinesBeforeMore && ch >= 0 && (CurClip->attrFlags & 0x10))
			LinesBeforeMore --;
		if((CurClip->attrFlags & 0x10) && LinesBeforeMore == 0 && ch >= 0) {
			/*"more": other cursor shape (PromptCh 1), raw keys*/
			unusedMidX = (CurClip->cols >> 1) - 2;
			row = (CurClip->rows + CurClip->originY) - 1;
			CookedInput = 0;
			PromptCh = 1;
			do
				key = CON_getch();
			while(key != ' ' && key != '\r' && key != 27 && key != 0x8f);
			PromptCh = 5;
			CookedInput = 1;
			TextIdle = 0;
			/*erase the bottom line (where the "more" cursor was)*/
			for(i = 0; i < CurClip->cols; i ++)
				(*CON_PutCharFn)(' ', i + CurClip->originX, row);
			LinesBeforeMore = CurClip->rows - 1;
		}
		/*word wrap: find where to cut the full line (not before the part
		already drawn; no break found: cut at the window width)*/
		breakPos = LineBufLen;
		if((CurClip->attrFlags & 1) && CurClip->cols == LineBufLen && ch >= 0) {
			LineBufChars[LineBufLen] = (ch == '\n')?' ':ch;
			LineBufColors[LineBufLen] = CurClip->fgColor;
			while(LineBufChars[breakPos] != ' ' && LineBufChars[breakPos - 1] != '-' && LineBufDrawn < breakPos)
				breakPos --;
			if(breakPos == 0)
				breakPos = LineBufLen;
		}
		row = CurClip->originY + CurClip->curY;
		if(CurClip->attrFlags & 2) {
			for(i = LineBufDrawn; i < LineBufDrawn + ((CurClip->cols - breakPos) >> 1); i++)
				(*CON_PutCharFn)(' ', i + CurClip->originX, row);
		}
		savedFg = CurClip->fgColor;
		for(i = LineBufDrawn; i < breakPos; i ++) {
			CurClip->fgColor = LineBufColors[i];
			(*CON_PutCharFn)(LineBufChars[i], i + CurClip->originX, row);
		}
		CurClip->fgColor = savedFg;
		if(LineBufDrawn < breakPos)
			CurClip->curX = breakPos;
		if((CurClip->attrFlags & 1) && CurClip->cols == LineBufLen && ch >= 0) {
			/*carry the cut word to the start of the buffer (dropping the space)*/
			if(LineBufChars[breakPos] == ' ')
				breakPos ++;
			for(i = breakPos; i <= LineBufLen; i ++) {
				LineBufChars[i - breakPos] = LineBufChars[i];
				LineBufColors[i - breakPos] = LineBufColors[i];
			}
			LineBufDrawn = 0;
			LineBufLen = i - breakPos;
			if(ch == ' ')
				LineAcceptSpace = 0;
		} else if(ch >= 0) {
			/*new empty line, then the char that did not fit (if any)*/
			LineBufLen = LineBufDrawn = 0;
			if(ch >= 0 && ch != '\n') {
				LineBufColors[LineBufLen] = CurClip->fgColor;
				LineBufChars[LineBufLen] = ch;
				LineBufLen ++;
			}
		}
		if(ch >= 0) {
			CurClip->curX = 0;
			if(CurClip->curY < CurClip->rows - 1) {
				CurClip->curY ++;
			} else if(CurClip->attrFlags & 0x20) {
				TextIdle = 1;
				(*CON_ScrollUpFn)(CurClip->originX, CurClip->originY, CurClip->cols, CurClip->rows);
				TextIdle = 0;
			}
		} else {
			/*flush: the line stays open at the cursor*/
			LineBufDrawn = LineBufLen = CurClip->curX;
			InHiddenSection = 0;
		}
	} else if(!(CurClip->attrFlags & 1) || ch != ' ' || LineAcceptSpace) {
		/*buffer the char (leading spaces of a wrapped line are dropped)*/
		LineBufColors[LineBufLen] = CurClip->fgColor;
		LineBufChars[LineBufLen] = ch;
		LineBufLen ++;
	}
	if(ch != ' ')
		LineAcceptSpace = 1;
	TextIdle = 1;
}

/*output ch count times (no escapes)*/
/*C_05D0_2A70*/CON_PutRepeat(int16 count, int16 ch) {
	int16 i;

	for(i = 0; i < count; i ++)
		CON_putch0(ch);
}

/*output string str (no escapes)*/
/*C_05D0_2A8A*/CON_PutsRaw(unsigned char far *str) {
	while(*str)
		CON_putch0(*str++);
}

/*convert number to a string in str, in base |base| (2~36); a negative
base means signed (number taken as a signed int, '-' counts in maxLen).
at most maxLen digits are kept (the most significant ones are lost).
returns the length*/
/*C_05D0_2AAC*/CON_itoa(uint16 number, char far *str, int16 maxLen, int16 base) {
	int16 len;
	char digits[8];
	int16 i, isNegative, digit;

	isNegative = 0;
	len = 0;
	if(base < 0) {
		base = -base;
		if(number > 0x7fff) {
			number = -number;
			isNegative = 1;
			maxLen --;
		}
	}
	/*digits, least significant first*/
	if(base) {
		while(number && len < maxLen) {
			digit = number % base;
			digits[len++] = (digit < 10)?(digit + '0'):(digit - 10 + 'A');
			number /= base;
		}
	}
	if(len == 0)
		digits[len++] = '0';
	if(isNegative)
		digits[len++] = '-';
	/*reverse*/
	for(i = 0; i < len; i ++)
		str[i] = digits[len - i - 1];
	str[i] = 0;
	return len;
}

/*character output with the in-text escapes (used by CON_puts/CON_printf,
so by all game messages, books, signs and conversations):
  #c   print VarInt[c] (c = '0'~'9' or 'A'~'Z')
  $c   print VarStr[c]
  /    following letters printed only if the last number was 1 (singular)
  \    following letters printed only if the last number was not 1 (plural)
       (both end at the next non letter)
  ^c   print control char c & 0x1f (special glyphs)
  @    following word is a keyword: highlighted (ColorRed) if help is on
  %    cancels a pending escape (no effect by itself)
  *    flush and wait for a key
  `    capitalize the next char*/
/*C_05D0_2B58*/CON_putch(int16 ch) {
	int16 varIdx;
	char numBuf[8];

	if(!TextOn)
		return;
	switch(ch) {
		case '#': case '$': case '%':
		case '/': case '@': case '\\':
		case '^': PendingEscape = ch; return;
		case '*':
			CON_putch0(-1);
			PromptCh = 1;
			CON_getch();
			PromptCh = 5;
		return;
		case '`': CapitalizeNext = 1; return;
	}
	switch(PendingEscape) {
		case '#':
			ch = toupper(ch);
			if(ch >= '0' && ch <= '9')
				varIdx = ch - '0';
			else if(ch >= 'A' && ch <= 'Z')
				varIdx = (ch - 'A') + 10;
			else
				varIdx = 0;
			LastNumberPlural = (VarInt[varIdx] != 1);
			CON_itoa(VarInt[varIdx], numBuf, 6, -10);
			CON_PutsRaw(numBuf);
			PendingEscape = 0;
		break;
		case '$':
			ch = toupper(ch);
			if(ch >= '0' && ch <= '9')
				varIdx = ch - '0';
			else if(ch >= 'A' && ch <= 'Z')
				varIdx = (ch - 'A') + 10;
			else
				varIdx = 0;
			CON_PutsRaw(VarStr[varIdx]);
			PendingEscape = 0;
		break;
		case '/':
			if(!isalpha(ch) || !LastNumberPlural) {
				CON_putch0(ch);
				if(!isalpha(ch))
					PendingEscape = 0;
			}
		break;
		case '\\':
			if(!isalpha(ch) || LastNumberPlural) {
				CON_putch0(ch);
				if(!isalpha(ch))
					PendingEscape = 0;
			}
		break;
		case '^':
			if(CurClip->attrFlags & 8)
				CON_putch0((ch & 0x1f) | 0x80);
			else
				CON_putch0(ch & 0x1f);
			PendingEscape = 0;
		break;
		case '@':
			/*keyword ends with punctuation/space*/
			if(strchr(/*04B5*/" ,.:;!?'-\"\n", ch)) {
				PendingEscape = 0;
				CurClip->fgColor = ColorText;
				CON_putch0(ch);
			} else {
				if(HelpOn)
					CurClip->fgColor = ColorRed;
				CON_putch0(ch);
			}
		break;
		default:
			CON_putch0(ch);
	}
}

/*output string str with escapes (CON_putch)*/
/*C_05D0_2D97*/CON_puts(unsigned char far *str) {
	while(*str)
		CON_putch(*str++);
}

/*minimal printf: %[-][0][width] followed by d (signed), o, x, u, c, s;
an unknown conversion prints its char raw ("%>" prints the '>' prompt);
the numbers set LastNumberPlural (plural flag); literal chars go through
CON_putch() so escapes apply, the converted values (%s too) also do*/
/*C_05D0_2DB9*/cdecl CON_printf(unsigned char far *format, ...) {
	VA_LIST argp;
	uint16 num;
	char numBuf[8];
	int16 c, fmtCh;
	int16 fieldWidth, len, padCh, leftAlign;
	char *str;

	if(!TextOn)
		return;
	VA_START(argp, format);
	while(*format) {
		c = *format++;
		fieldWidth = len = leftAlign = 0;
		padCh = ' ';
		str = numBuf;
		switch(c) {
		case '%':
			fmtCh = *format++;
			while(fmtCh == '0' || fmtCh == '-') {
				switch(fmtCh) {
					case '0': padCh = '0'; break;
					case '-': leftAlign = 1; break;
				}
				fmtCh = *format++;
			}
			while(fmtCh >= '0' && fmtCh <= '9') {
				fieldWidth *= 10;
				fieldWidth += fmtCh - '0';
				fmtCh = *format++;
			}
			switch(tolower(fmtCh)) {
				case 'd':
					num = VA_INT(argp);
					len = CON_itoa(num, numBuf, 6, -10);
					LastNumberPlural = (num != 1);
				break;
				case 'o':
					num = VA_INT(argp);
					len = CON_itoa(num, numBuf, 6, 8);
					LastNumberPlural = (num != 1);
				break;
				case 'x':
					num = VA_INT(argp);
					len = CON_itoa(num, numBuf, 4, 16);
					LastNumberPlural = (num != 1);
				break;
				case 'u':
					num = VA_INT(argp);
					len = CON_itoa(num, numBuf, 5, 10);
					LastNumberPlural = (num != 1);
				break;
				case 'c':
					numBuf[0] = VA_CHAR(argp);
					numBuf[1] = 0;
				break;
				case 's':
					str = VA_STR(argp);
					len = strlen(str);
				break;
				default:
					numBuf[0] = 0;
					CON_putch0(fmtCh);
			}
			if(leftAlign)
				CON_puts(str);
			CON_PutRepeat(fieldWidth - len, padCh);
			if(leftAlign == 0)
				CON_puts(str);
		break;
		default:
			CON_putch(c);
		}
	}
}
