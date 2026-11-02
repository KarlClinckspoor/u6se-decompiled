/* flags: -d -mh -p -c -Z -O -zTFAR_DATA */
/*
	The far message strings (segment 2F65, after the code): messages printed
	with CON_printf by the combat, movement, command and system modules. A '`'
	makes the console capitalise the next character (the start of an inserted
	name). Compiled huge model with the data class FAR_DATA (-zT), so they are in
	a far data segment of their own, placed after the code and before C0M's
	start-up tables and DGROUP
	(Ultima VI's msgs.c, rewritten for Savage Empire). Other modules declare a
	message as `extern char far Name[];`.
*/

/*C_2F65_0000*/char CriticalMsg[] = "critical!\n";
/*C_2F65_000B*/char HeavilyMsg[] = "heavily";
/*C_2F65_0013*/char LightlyMsg[] = "lightly";
/*C_2F65_001B*/char BarelyMsg[] = "barely";
/*C_2F65_0022*/char WoundedMsg[] = " wounded.\n";
/*C_2F65_002D*/char GrazedMsg[] = "`%s grazed.\n";
/*C_2F65_003A*/char KilledMsg[] = "`%s killed!\n";
/*C_2F65_0047*/char FallsMsg[] = "`%s falls!\n";
/*C_2F65_0053*/char BareHandsMsg[] = "with bare hands-";
/*C_2F65_0064*/char WithMsg[] = "with %s-";
/*C_2F65_006D*/char OutOfArrowsMsg[] = "Out of arrows!\n";
/*C_2F65_007D*/char OutOfAmmoMsg[] = "Out of ammunition!\n";
/*C_2F65_0091*/char OutOfDartsMsg[] = "Out of darts!\n";
/*C_2F65_00A0*/char OutOfSpearsMsg[] = "Out of spears!\n";
/*C_2F65_00B0*/char FlintlockNotLoadedMsg[] = "Flintlock not loaded!\n";
/*C_2F65_00C7*/char BucketEmptyMsg[] = "The bucket is empty!\n";
/*C_2F65_00DD*/char AttackSelfPassMsg[] = "pass.\n";
/*C_2F65_00E4*/char NothingMsg[] = "nothing!\n";
/*C_2F65_00EE*/char AttackWithMsg[] = "\nAttack with %s-";
/*C_2F65_00FF*/char AttackMsg[] = "Attack";
/*C_2F65_0106*/char WhatMsg[] = "what?\n";
/*C_2F65_010D*/char PartyModeMsg[] = "Party mode\n";
/*C_2F65_0119*/char FailsToRespondMsg[] = "`%s fails to respond.\n";
/*C_2F65_0130*/char SoloModeMsg[] = "Solo mode\n";
/*C_2F65_013B*/char ExitToDOSMsg[] = "Exit to DOS?";
/*C_2F65_0148*/char RestMsg[] = "Rest!\n";
/*C_2F65_014F*/char SaveMsg[] = "Save game? ";
/*C_2F65_015B*/char PassMsg[] = "Pass!\n";
/*C_2F65_0162*/char HearSomethingMsg[] = "You hear something to the %s!\n";
/*C_2F65_0181*/char InterferenceMsg[] = "`%s interferes!\n";
/*C_2F65_0192*/char PoisonedMsg[] = "`%s poisoned!\n";
/*C_2F65_01A1*/char BurnOutMsg[] = "A torch burned out.\n";
/*C_2F65_01B6*/char VineKilledMsg[] = "Tangle vine killed!\n";
/*C_2F65_01CB*/char TeleportMsg[] = "Wisp teleports!\n";
/*C_2F65_01DC*/char CombatMsg[] = "Begin combat!\n";
/*C_2F65_01EB*/char BreakOffMsg[] = "Break off combat!\n";
/*C_2F65_01FE*/char OutOfRangeMsg[] = "Out of range!\n";
/*C_2F65_020D*/char NotOnShipMsg[] = "Not while aboard ship!\n";
/*C_2F65_0225*/char UnconsciousMsg[] = "\nOvercome by your wounds, you fall unconscious..... \n";
/*C_2F65_025B*/char AwakenAtIntanyaMsg[] = "\n....You awaken in Intanya's hut, feeling restored.\n";
/*C_2F65_0290*/char BrokenMsg[] = "`%s broken!\n";
/*C_2F65_029D*/char NotInSoloMsg[] = "Not in solo mode!\n";
/*C_2F65_02B0*/char NotInCombatMsg[] = "Not in combat mode!\n";
/*C_2F65_02C5*/char AttacksMsg[] = "`%s attacks!\n";
/*C_2F65_02D3*/char StuckInWebMsg[] = "`%s is stuck in a web!\n";
/*C_2F65_02EB*/char MustBePartyModeMsg[] = "You must be in party mode to enter.\n";
/*C_2F65_0310*/char NotAllHereMsg[] = "Not everyone is here.\n";
/*C_2F65_0327*/char SoundOnMsg[] = "Sound on\n";
/*C_2F65_0331*/char SoundOffMsg[] = "Sound off\n";
/*C_2F65_033C*/char InsertDiskMsg[] = "\nPlease insert %s.";
/*C_2F65_034F*/char NoEffectMsg[] = "No effect.\n";
/*C_2F65_035B*/char NoResponseMsg[] = "\nNo response.\n";
/*C_2F65_036A*/char SuccessMsg[] = "Success!\n";
/*C_2F65_0374*/char NotPossibleMsg[] = "Not possible.\n";
/*C_2F65_0383*/char TooMuchMsg[] = "You are carrying too much already.\n";
/*C_2F65_03A7*/char FailedMsg[] = "Failed.\n";
/*C_2F65_03B0*/char DoneMsg[] = "Done.\n";
/*C_2F65_03B7*/char BlockedMsg[] = "Blocked!\n";
/*C_2F65_03C1*/char CantUseMsg[] = "`%s can't use that!\n";
/*C_2F65_03D6*/char VersionMsg[] = "Version 1.6\n";

/*(13 bytes to the next paragraph: the next segment is paragraph aligned)*/
