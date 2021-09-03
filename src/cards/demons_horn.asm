%include "ManalinkEh.mac"

section .text

extern _generic_life_artifact_charm

global _card_demons_horn
_card_demons_horn:
	GenericLifeArtifactCharmCall BlackColor
align 32