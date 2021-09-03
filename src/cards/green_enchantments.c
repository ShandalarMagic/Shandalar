#include "manalink.h"

int card_elephant_grass(int player, int card, event_t event){
    if( event == EVENT_ATTACK_LEGALITY && current_turn == 1-player ){
        int color = get_color(affected_card_controller, affected_card);
        if( color & COLOR_TEST_BLACK ){
            event_result = 1;
        }
    }
    tax_attack(player, card, event);
    cumulative_upkeep(player, card, event, 1, 0, 0, 0, 0, 0);
    return global_enchantment(player, card, event);
}

int card_utopia_sprawl(int player, int card, event_t event){
	card_instance_t* instance = get_card_instance(player, card);
	int result = wild_growth_aura(player, card, event, SUBTYPE_FOREST, instance->targets[2].player, instance->targets[2].card);
	if (cancel != 1	// failed to verify target
		&& comes_into_play(player, card, event)){
        instance->targets[2].player = choose_a_color(player, get_deck_color(player, player));
		instance->targets[2].card = 1;
		instance->info_slot = 1 << instance->targets[2].player;
	}
    return result;
}

int card_enchantresss_presence(int player, int card, event_t event){
    if(event == EVENT_CAN_CAST){
        return 1;
    }
    return card_argothian_enchantress(player, card,  event);
}

int default_tutor3(int player, int card){
    return 1;
}

int pattern_of_rebirth_tutor (int player){
    return tutor(player, TYPE_CREATURE, &default_tutor3, TUTOR_PLAY );
}

