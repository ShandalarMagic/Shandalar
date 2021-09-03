#include <assert.h>

#include "manalink.h"

/*
 * result is passed in as a bitfield of the exe abilities.  The leftmost bit
 * used is 1<<16 == 0x10000; any higher bits are removed out.
 *
 * The bits set are used in a hardcoded order to retrieve an icon from
 * Ability.bmp.  Bits >= 1<<17, however, are used in order.
 *
 * Complex code cannot be called from this function, since it's running in a
 * different thread from the AI.  In particular, get_ability() cannot be
 * called, since it sends an event to all cards.  The abilities are cached in
 * what's (incorrectly) been labelled card_instance_t::regen_status.
 *
 * Similarly, we use get_displayed_card_instance() to get the version of the
 * card instance that matches the displayed game state, rather than
 * get_card_instance() which is the one the AI is making speculative moves
 * with.
 */
int get_ability_image(int player, int card, int result){
	card_instance_t *instance = get_displayed_card_instance(player, card);

	if (cards_data[instance->internal_card_id].type & TYPE_EFFECT)
		return result;

	/* This statement is the equivalent of !in_play(player, card), using
	 * instance instead of calling get_card_instance() internally. */
	if (!(instance->internal_card_id != -1
		  && (instance->state & (STATE_OUBLIETTED|STATE_INVISIBLE|STATE_IN_PLAY)) == STATE_IN_PLAY))
		return 0;

	if (get_forbid_activation_flag_by_instance(instance) & 4){
		return 0;
	}

	int sp_keywords = get_special_abilities_by_instance(instance);
	if (sp_keywords == -1){
		sp_keywords = 0;
	}

	/* 1<<17: Intimidate/Fear.  Intimidate is evergreen, and Fear formerly so.
	 * Combining them is acceptable since they have the same sort of effect and
	 * intimidate is still fairly rare (25 cards), but they should be split up
	 * once we can have more than 32 icon types. */
	if (sp_keywords & (SP_KEYWORD_FEAR | SP_KEYWORD_INTIMIDATE)){
		result |= 0x00020000;
	}
	// 1<<18: Shroud.  Formerly evergreen and fairly common. (116 cards)
	if (instance->regen_status & KEYWORD_SHROUD){
		result |= 0x00040000;
	}
	/* 1<<19: Unblockable.  Not actually a keyword, but a fairly common ability
	 * (124 cards) and of critical importance to assessing play. */
	if (sp_keywords & SP_KEYWORD_UNBLOCKABLE){
		result |= 0x00080000;
	}
	// 1<<20: Vigilance.  An evergreen keyword and common (190 cards).
	if (instance->state & STATE_VIGILANCE){
		result |= 0x00100000;
	}
	// 1<<21: Deathtouch.  An evergreen keyword, though uncommon (65 cards).
	if (sp_keywords & SP_KEYWORD_DEATHTOUCH){
		result |= 0x00200000;
	}
	// 1<<22: Lifelink.  An evergreen keyword, though uncommon (82 cards).
	if (sp_keywords & SP_KEYWORD_LIFELINK){
		result |= 0x00400000;
	}
	/* 1<<23: Double Strike.  An evergreen keyword, not terribly common (59
	 * cards) but has high impact.
	 *
	 * This is always set with First Strike; KEYWORD_DOUBLE_STRIKE in fact
	 * includes the First Strike bit.  Hence the comparison to
	 * KEYWORD_DOUBLE_STRIKE instead of just nonzero like the others, and
	 * removal of KEYWORD_FIRST_STRIKE. */
	if ((instance->regen_status & KEYWORD_DOUBLE_STRIKE) == KEYWORD_DOUBLE_STRIKE){
		result |= 0x00800000;
		result &= ~KEYWORD_FIRST_STRIKE;
	}
	/* 1<<24: Infect.  Appears only in the Scars of Mirrodin block (67 cards),
	 * but hey, that's the current one! */
	if ((instance->regen_status & KEYWORD_INFECT)
		&& !(instance->targets[14].card > 0
			 && (instance->targets[14].card & SF_INFECT_REMOVED))){
		result |= 0x01000000;
	}
	/* 1<<25: Indestructible.  Evergreen, popular, and high game impact,
	 * despite its relative rarity (61 cards). */
	if (sp_keywords & SP_KEYWORD_INDESTRUCTIBLE){
		result |= 0x02000000;
	}
	/* 1<<26: Shadow.  Appears only in the Tempest and Time Spiral blocks (70
	 * cards).  Only included due to the creatures with shadow in one of the
	 * challenge gauntlet decks. */
	if (sp_keywords & SP_KEYWORD_SHADOW){
		result |= 0x04000000;
	}
	/* 1<<27: Hexproof.  Evergreen, though still rare (48 cards). */
	if (sp_keywords & SP_KEYWORD_HEXPROOF){
		/* Check of SF_HEXPROOF_OVERRIDE is deliberately missing - Glaring
		 * Spotlight doesn't actually remove hexproof, but makes creatures
		 * targettable as if they didn't have it */
		result |= 0x08000000;
	}
	/* 1<<30: Horsemanship.  Rare at 36 cards and won't be returning, but has a
	 * significant game impact when it shows up. */
	if (sp_keywords & SP_KEYWORD_HORSEMANSHIP){
		result |= 0x10000000;
	}

	return result;
}

static char* ability_tooltip_names[32] = { 0 };

// Prerequisite: load_text(0, "ABILITYWORDS") has already been called.
void initialize_ability_tooltip_names(void)
{
	if (!ability_tooltip_names[0]){
		int i;
		for (i = 0; i < 32; ++i){
			ability_tooltip_names[i] = strdup(text_lines[i]);	// won't ever be freed
			assert(strlen(ability_tooltip_names[i]) <= 49 && "initialize_ability_tooltip_names(): ability name too long; must be 49 characters or less");
		}
		// Now copy the first seventeen to where the exe expects them
		for (i = 0; i < 17; ++i){
			strncpy(ability_tooltip_names_exe[i], ability_tooltip_names[i], 49);
			ability_tooltip_names_exe[i][49] = '\0';
		}
	}
}

void fetch_ability_tooltip_name(char* destination, int i)
{
	assert(i >= 0 && i < 32 && "fetch_ability_tooltip_name: parameter out of range");
	strcpy(destination, ability_tooltip_names[i]);
}
