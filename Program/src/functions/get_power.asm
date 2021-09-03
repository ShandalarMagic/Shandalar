%include "ManalinkEh.mac"

section .text

global _get_power
_get_power:
  push ebp
  mov ebp, esp
  push -1
  push 0x32
  push CardID
  push PlayerID
  call 0x4352D0
  add esp, 0x10
  leave
  ret
  align 32

global _get_toughness
_get_toughness:
  push ebp
  mov ebp, esp
  push -1
  push 0x33
  push CardID
  push PlayerID
  call 0x4352D0
  add esp, 0x10
  leave
  ret
  align 32
