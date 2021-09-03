%include "ManalinkEh.mac"

section .text

global _generic_protection_from
_generic_protection_from:
	push	ebp
	mov		ebp, esp
	cmp		EventCode, EventSetAttr
	jnz		.endofcode
	mov		ecx, CardID
	cmp		AffectedCard, ecx
	jnz		.endofcode
	mov		eax, PlayerID
	cmp		AffectedPlayer, eax
	jnz		.endofcode
	push	UserParam1
	push	ecx
	push	eax
	call	ColorToSleightColorFunction
	add		esp, 0xc
	mov		edx, 0x800
	dec		eax
	mov		cl, al
	shl		edx, cl
	or		[0x6205b4], edx
	push    dword [0x6205b4]
	push	UserParam1
	push	CardID
	push	PlayerID
	call	CheckAurasAttachedProcedure
	add		esp, 0xc
	pop		dword [0x6205b4]
.endofcode:
	xor		eax, eax
	leave
	ret
	align 32