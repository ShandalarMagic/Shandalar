#include "manalink.h"

int card_lich(int player, int card, event_t event){
    if( event == EVENT_GRAVEYARD_FROM_PLAY && get_challenge4() == 2 && is_unlocked(player, card, event, 37) ){
        return 0;
    }
    return card_lich_exe(player, card, event);
}

int effect_necropotence(int player, int card, event_t event){
    if( eot_trigger(player, card, event ) && current_turn == player ){
        card_instance_t *instance = get_card_instance(player, card);
        add_card_to_hand(player, instance->targets[0].card );
        kill_card(player, card, KILL_REMOVE);
    }
    return 0;
}

int card_necropotence(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

    skip_your_draw_step(player, event);

    if(event == EVENT_CAN_ACTIVATE && ! get_forbid_activation_flag(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_pay_life(player, 1);
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			lose_life(player, 1);
		}
	}
    if(event == EVENT_RESOLVE_ACTIVATION){
        int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			int legacy_card = create_legacy_effect(player, instance->parent_card, &effect_necropotence );
			card_instance_t *legacy = get_card_instance(player, legacy_card);
			legacy->targets[0].card = deck[0];
			remove_card_from_deck( player, 0 );
		}
    }
    if( current_turn == player && hand_count[player] > 7 && eot_trigger(player, card, event ) ){
			int disc = 1;
			if( check_battlefield_for_id(player, CARD_ID_SPELLBOOK) ||
				check_battlefield_for_id(player, CARD_ID_VENSERS_JOURNAL) ||
				check_battlefield_for_id(player, CARD_ID_LIBRARY_OF_LENG) ||
				check_battlefield_for_id(player, CARD_ID_RELIQUARY_TOWER) ||
				check_battlefield_for_id(player, CARD_ID_ANVIL_OF_BOGARDAN)
			  ){
				disc = 0;
			}
			while( 	disc == 1 && hand_count[player] > 7 ){
					discard(player, 0, 0);
					rfg_card_from_grave(player, count_graveyard(player) - 1);
			}
    }

	return global_enchantment(player, card, event);
}

int card_nether_void(int player, int card, event_t event){
    if( event == EVENT_CAN_CAST ){
        return 1;
    }
    enchant_world(player, card, event);
    if( event == EVENT_MODIFY_COST_GLOBAL ){
        card_data_t* card_d = get_card_data(affected_card_controller, affected_card);
        if( card_d->cc[2] != 10 && ! ( card_d->type & TYPE_LAND) ){
            COST_COLORLESS += 3;
        }
    }
    return global_enchantment(player, card, event);
}



