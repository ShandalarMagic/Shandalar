%include "ManalinkEh.mac"

section .text

extern _generic_protection_from

global _protection_from_blue
_protection_from_blue:
GenericProtectionProcedureCall BlueColor
align 32