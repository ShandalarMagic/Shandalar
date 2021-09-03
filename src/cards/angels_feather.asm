%include "ManalinkEh.mac"

section .text

extern _generic_life_artifact_charm

global _card_angels_feather
_card_angels_feather:
	GenericLifeArtifactCharmCall WhiteColor
align 32