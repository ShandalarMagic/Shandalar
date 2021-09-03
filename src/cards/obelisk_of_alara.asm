%include "ManalinkEh.mac"

section .text

global _card_obelisk_of_alara
_card_obelisk_of_alara:
	push    ebp
	mov     ebp, esp
	sub     esp, 0xbc4
	push    esi
	cmp     EventCode, EventCanActivate ; Can Play Ability?
	jnz     .0001
	GetCardInfo PlayerID, CardID
	; Is Untapped?
	test    CardInfoState, CardInfoStateTap
	jnz     .endofproc1
	; Have Mana of It's Abilities?
	HaveManaOrJump 2, AnyMana, .endofproc1
	mov     eax, 1
	jmp     .endofproc2 
.0001:	
	cmp     EventCode, EventActivated ; Select Ability And Pay Costs
	jnz     .0002
	; AI Helper
	; If it can use red mana, will burn, otherwise
	; If it can use white mana, will gain life, otherwise
	; If it can use blue mana, will draw and discard, otherwise
	; If it can use black mana, try to -2/-2, otherwise
	; If it can use green mana, try to +4/+4, or cancel...
	mov     dword [ebp-8], 5
	; There is a creature?
	call    0x4177a0
	test    eax, eax
	jz      .0001aa
	HaveManaOrJump 1, GreenMana, .0001aa
	mov     dword [ebp-8], 2
	HaveManaOrJump 1, BlackMana, .0001aa
	mov     dword [ebp-8], 0
.0001aa:
	HaveManaOrJump 1, BlueMana, .0001ab
	mov     dword [ebp-8], 1
.0001ab:
	HaveManaOrJump 1, WhiteMana, .0001ac
	mov     dword [ebp-8], 4
.0001ac:
	HaveManaOrJump 1, RedMana, .0001ad
	mov     dword [ebp-8], 3
.0001ad:
	;
	GetCardInfo PlayerID, CardID
	LoadResource ObeliskOfAlaraPrompt
	; Build Window String
	push    ResourceString6
	push    ResourceString5
	push    ResourceString4
	push    ResourceString3
	push    ResourceString2
	push    ResourceString1
	push    ObeliskOfAlaraStr1
	lea     eax, [ebp-0xbc4]
	push    eax
	call    sprintf
	add     esp, 0x20
	; Open Window
	mov     eax, dword [ebp-8]
	push    eax
	lea     eax, [ebp-0xbc4]
	push    eax
	push    0xffffffff
	push    0xffffffff
	push    CardID
	push    PlayerID
	push    PlayerID
	call    0x471eb0
	add     esp, 0x1c
	; Recover Card Pointer Info
	mov     CardInfoSlot1, eax
	; Cancelled?
	cmp     eax, 5
	je      .0001z
	; Validate the mana... Option 0 use mana coded 1, 1 used mana 2... 0 to 4 is 1 to 5 code for mana...
	inc     eax
	HaveManaOrJump 1, eax, .0001
	; Recover the stored option
	mov     eax, CardInfoSlot1
	; Based on selection... make target or cancel...
	cmp     eax, 3
	jne     .0001a	
	LoadResource 0x4d75f8 ; Target a player... or cancel...
	mov     edx, ResourceString1
	call    0x41aea0
	test    eax, eax
	jz     	.0001z
.0001a:	
	cmp     eax, 2
	je     .0001b	
	cmp     eax, 0
	jne     .0001c	
.0001b:
	; Target creature, if there is no one... open the select window again... (invalid option...)
	call    0x4177a0
	test    eax, eax
	jz      .0001
	LoadResource 0x4d7696 ; Target a creature... or cancel...
	mov     edx, ResourceString1
	call    0x4197a0
	test    eax, eax
	jz      .0001z
.0001c:
	; Charge Mana
	mov     eax, CardInfoSlot1
	mov     edx, eax
	inc     edx
	mov     ExtraColorlessMana, 1 
	PayMana 1, edx ; 1B, 1U, 1G, 1R or 1W
	; Cancelled?
	cmp     dword [0x4ef194], 1
	jz      .endofproc1
	; Mana Charged, Option Selected, Target Acquired... Tap and Fire!!!
	TapCard PlayerID, CardID
	jmp .0002
.0001z:
	mov     dword [0x4ef194], 1
.0002:	
	cmp     EventCode, EventResolveActivation ; Do The Ability...
	jnz     .endofproc1
	GetCardInfo PlayerID, CardID
	mov     eax, CardInfoSlot1 ; Selected Ability...
	; Gain 5 Life
	cmp     eax, 4
	jne     .0002a
	PlayerGainLife PlayerID, 5
	jmp     .endofproc1
.0002a:
	; 3 damage to a player...
	cmp     eax, 3
	jne     .0002b
	;
	GetCardInfo PlayerID, CardID
	DamagePlayer CardInfoTargetPlayer, 3
	;
	jmp     .endofproc1
.0002b:
	; +4/+4 to target creature...
	cmp     eax, 2
	jne     .0002c
	; Valid Target Yet?
	call    0x419250
	test    eax, eax
	jz      .endofproc1
	;
	GetCardInfo PlayerID, CardID
	push    CardInfoTargetCard
	push    CardInfoTargetPlayer
	push    dword [0x728370]
	push    CardID
	push    PlayerID
	call    0x4a09a0
	add     esp, 0x14
	mov     ecx, eax
	cmp     eax, -1
	jz      .endofproc1
	mov     eax, PlayerID
	push    esi
	call    0x401a80
	mov     word [esi+0x12], 4
	mov     word [esi+0x1c], 4
	pop     esi
	;
	jmp     .endofproc1
.0002c:	
	; draw a card, discard a card...
	cmp     eax, 1
	jne     .0002d
	DrawCard PlayerID
	DiscardCard PlayerID, dtChoose
	jmp     .endofproc1
.0002d:		
	; -2/-2 to target creature...
	cmp     eax, 0
	jne     .endofproc1
	;
	; Valid Target Yet?
	call    0x419250
	test    eax, eax
	jz      .endofproc1
	;
	GetCardInfo PlayerID, CardID
	push    CardInfoTargetCard
	push    CardInfoTargetPlayer
	push    dword [0x728370]
	push    CardID
	push    PlayerID
	call    0x4a09a0
	add     esp, 0x14
	mov     ecx, eax
	cmp     eax, -1
	jz      .endofproc1
	mov     eax, PlayerID
	push    esi
	call    0x401a80
	mov     word [esi+0x12], -2
	mov     word [esi+0x1c], -2
	pop     esi
	;	
.endofproc1:
	xor     eax, eax
.endofproc2:
	pop     esi
	leave
	ret
	align 32

section .data
ObeliskOfAlaraStr1 db ' %s', 10, ' %s', 10, ' %s', 10, ' %s', 10, ' %s', 10, ' %s', 0
ObeliskOfAlaraPrompt db 'OBELISKOFALARA', 0