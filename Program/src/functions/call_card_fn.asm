section .text

;A handful of exe/asm card functions, such as Tawnos's Coffin and cards with
;buyback, assume that esi holds the instance of the card being called.  That's
;not necessarily true even for calls from within the exe, but until they're
;rewritten in C, allow for bug-for-bug compatibility.

;int call_card_fn(void* address, card_instance_t* instance, int player, int card, event_t event)
global _call_card_fn_impl
_call_card_fn_impl:
	push	ebp			; save stack
	mov	ebp, esp		; new stack
	push	esi			; save esi
	mov	eax, dword [ebp+0x08]	; address to call
	mov	esi, dword [ebp+0x0C]	; put instance in esi where exe cards expect it
	push	dword [ebp+0x18]	; arg3 = event
	push	dword [ebp+0x14]	; arg2 = card
	push	dword [ebp+0x10]	; arg1 = player
	call	eax			; call address
	add	esp, 0xc		; pop three arguments off the stack
	pop	esi			; restore esi
	leave
	ret
	align	32
