%include "ManalinkEh.mac"

section .text

extern _protection_from_red
extern _protection_from_blue

global _card_sabertooth_nishoba
_card_sabertooth_nishoba:
	ChainProcedureCall _protection_from_red, _protection_from_blue
align 32