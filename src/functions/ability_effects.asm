%include "ManalinkEh.mac"

section .text

global _ability_effect_keyword
_ability_effect_keyword:
  push ebp
  mov ebp, esp

  push dword [ebp + 0xC]
  push dword [ebp + 0x8]
  push dword [0x715B38]
  push dword [0x7876E8]
  push dword [0x7A3610]

  call 0x4A09A0
  add esp, 0x14
  leave
  ret
  align 32