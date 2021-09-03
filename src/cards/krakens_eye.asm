%include "ManalinkEh.mac"

section .text

extern _generic_life_artifact_charm

global _card_krakens_eye
_card_krakens_eye:
	GenericLifeArtifactCharmCall BlueColor
align 32