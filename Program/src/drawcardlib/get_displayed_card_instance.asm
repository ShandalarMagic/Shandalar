; -*- tab-width:4 -*-

section .text

extern _get_displayed_card_instance_manalink_exe

global _get_displayed_card_instance_manalink
_get_displayed_card_instance_manalink:
	push ebp
	mov ebp, esp
	push esi
	mov ecx, dword [ebp+0x0c]	; parameter 2: card number
	mov eax, dword [ebp+0x08]	; parameter 1: player number
	call dword [_get_displayed_card_instance_manalink_exe]	; can't use the constant, or it gets relocated by the linker
	mov eax, esi
	pop esi
	leave
	ret
	align 32
