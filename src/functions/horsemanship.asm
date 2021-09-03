%include "ManalinkEh.mac"

section .text

global _horsemanship
_horsemanship:
	push    ebp
	mov     ebp, esp
	push    esi
	cmp     EventCode, EventBlockLegality
	jnz     .endofproc1
	; 
	mov     eax, PlayerID
	cmp     AttackingPlayer, eax
	jnz     .endofproc1
	mov     eax, CardID
	cmp     AttackingCard, eax
	jnz     .endofproc1
	;
	CreatureInfo DefendingPlayer, DefendingCard, citAttr
	bt      eax, 0x18
	jc      .endofproc1
	;
	mov     EventData, 1
.endofproc1:
	xor     eax, eax
	pop     esi
	leave
	ret
	align 32