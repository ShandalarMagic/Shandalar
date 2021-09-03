%include "ManalinkEh.mac"

section .text

extern _generic_protection_from

global _protection_from_red
_protection_from_red:
GenericProtectionProcedureCall RedColor
align 32