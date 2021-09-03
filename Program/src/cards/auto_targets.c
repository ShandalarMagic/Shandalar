#if 0
#include "manalink.h"

static int choose_target_player(int player, int card, event_t event, int (*func_ptr)(int, int, event_t)){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		if( ! select_target(player, card, &td, "Choose a target player", NULL) ){
			spell_fizzled = 1;
		}
		return 0;
	}
	return func_ptr(player, card, event);
}

static int choose_target_creature(int player, int card, event_t event, int (*func_ptr)(int, int, event_t)){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( ! select_target(player, card, &td, "Choose a target creature", NULL) ){
			spell_fizzled = 1;
		}
		return 0;
	}
	return func_ptr(player, card, event);
}

int card_ancestral_recall(int player, int card, event_t event){
	return choose_target_player(player, card, event, &card_ancestral_recall_exe);
}

int card_swords_to_plowshares(int player, int card, event_t event){
	return choose_target_creature(player, card, event, &card_swords_to_plowshares_exe);
}
#endif
