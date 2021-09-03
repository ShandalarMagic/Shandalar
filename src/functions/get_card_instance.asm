%include "ManalinkEh.mac"

section .text

global _get_card_instance
_get_card_instance:
	push ebp
	mov ebp, esp
	push esi
	mov ecx, CardID
	mov eax, PlayerID
	call 0x401a80
	mov eax, esi
	pop esi
	leave
	ret
	align 32

global _get_displayed_card_instance
_get_displayed_card_instance:
	push ebp
	mov ebp, esp
	push esi
	mov ecx, CardID
	mov eax, PlayerID
	call 0x401ab0
	mov eax, esi
	pop esi
	leave
	ret
	align 32
