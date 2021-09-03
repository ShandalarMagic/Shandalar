%include "ManalinkEh.mac"

section .text

extern _generic_life_artifact_charm

global _card_wurms_tooth
_card_wurms_tooth:
	GenericLifeArtifactCharmCall GreenColor
align 32