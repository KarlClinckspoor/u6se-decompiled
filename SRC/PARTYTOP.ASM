;Data only: PartyListTop, the first party member shown in the party view (Ultima VI's
;SEG_155X.ASM, linked between object and status). Its empty code segment is Origin's.
INCLUDE OSI.INC

PUBLIC PartyListTop

DSegStart
PartyListTop db 0	;D_08A2
DSegEnd

CSegStart	PARTYTOP_TEXT
CSegEnd	PARTYTOP_TEXT

END
