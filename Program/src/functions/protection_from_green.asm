%include "ManalinkEh.mac"

section .text

extern _generic_protection_from

global _protection_from_green
_protection_from_green:
GenericProtectionProcedureCall GreenColor
align 32