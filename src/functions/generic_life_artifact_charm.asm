%include "ManalinkEh.mac"

section .text

global _generic_life_artifact_charm
_generic_life_artifact_charm:
	push    ebp
	mov     ebp, esp
	push    esi
	cmp     EventCode, 0x6c
	jnz     .0001
	mov     eax, AffectedCard
	cmp     CardID, eax
	jnz     .0001
	mov     eax, AffectedPlayer
	cmp     PlayerID, eax
	jnz     .0001
	mov     eax, [0x73684c]
	shl     eax, 5
	mov     ecx, UserParam1
	mov     eax, [eax+ecx*4+0x4ef4a0]
	lea     eax, [eax+eax*2]
	shl     eax, 2
	add     [0x7a31a8], eax
.0001:	
	cmp     TriggerEvent, 0xd3
	jnz     .endofproc
	mov     ecx, AffectedCard
	cmp     CardID, ecx
	jnz     .endofproc
	mov     eax, AffectedPlayer
	cmp     PlayerID, eax
	jnz     .endofproc
	mov     eax, [0x737e1c]
	cmp     PlayerID, eax
	jnz     .endofproc
	call    0x401a40
	jnz     .endofproc
	mov     eax, UserParam1
	SleightColorProcedure eax
	mov     cl, al
	mov     eax, 1
	shl     eax, cl
	mov     UserParam1, eax
	mov     eax, TriggerPlayer
	mov     ecx, TriggerCard
	call    GetCardInfoFunction
	call    0x401710
	test    UserParam1, eax
	jz      .endofproc
	mov     eax, [esi+0x6c]
	test    eax, eax
	js      .endofproc
	mov     eax, [eax*4+0x7c7000]
	cmp     byte [eax+0x28], 1
	jz      .endofproc
	cmp     EventCode, EventTrigger
	jnz     .0003
	mov     eax, PlayerID
	cmp     [0x73684c], eax
	jnz     .0002
	test    byte [0x790640], 2
	jnz     .0002
	or      dword [0x6205b4], 2
	jmp     .0003
.0002:	
	or      dword [0x6205b4], 1
.0003:	
	cmp     EventCode, EventResolveTrigger
	jnz     .endofproc
	push    0
	push    0
	push    0x72
	push    CardID
	push    PlayerID
	call    0x436550
	add     esp, 0x14
	call    0x436930
	cmp     dword [0x4ef194], 1
	jz      .endofproc
	push    0x91
	push    CardID
	push    PlayerID
	call    0x4359b0
	add     esp, 0xc
	PlayerGainLife PlayerID, 1
	mov     eax, PlayerID
	cmp     [0x73684c], eax
	jnz     .endofproc
	sub     dword [0x7a31a8], 0x18
.endofproc:	
	xor     eax, eax
	pop     esi
	leave
	ret
	align 32