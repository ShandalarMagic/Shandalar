%include "ManalinkEh.mac"

section .text

extern _generic_protection_from

global _protection_from_black
_protection_from_black:
GenericProtectionProcedureCall BlackColor
align 32