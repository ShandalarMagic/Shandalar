%include "ManalinkEh.mac"

section .text

extern _generic_protection_from

global _protection_from_white
_protection_from_white:
GenericProtectionProcedureCall WhiteColor
align 32