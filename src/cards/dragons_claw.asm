%include "ManalinkEh.mac"

section .text

extern _generic_life_artifact_charm

global _card_dragons_claw
_card_dragons_claw:
	GenericLifeArtifactCharmCall RedColor
align 32