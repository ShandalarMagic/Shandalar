%include "ManalinkEh.mac"

section .text

global _in_play
_in_play:
  push  ebp
  mov ebp, esp
  push esi
  mov ecx, CardID
  mov eax, PlayerID
  call 0x401A40
  jnz is_not
  mov eax, 1
  jmp get_out

is_not:
  xor eax, eax

get_out:
  pop   esi
  leave
  ret
  align 32

global _is_not_dead
_is_not_dead:
  push  ebp
  mov ebp, esp
  mov     eax, 0x715dd8
  call    0x471d30
  leave
  ret
  align 32

; void display_error_message(const char* msg);
; Displays msg in the small horizontal window and sleeps for 2 seconds.
;
; This really doesn't belong here, but neither does the misnamed is_not_dead()
; above, and this is a more general version of that.
global _display_error_message
_display_error_message:
  push	ebp
  mov	ebp, esp
  mov	eax, dword [ebp+0x08]
  call	0x471d30
  leave
  ret
  align	32
