#include "manalink.h"

void spirit_link_effect(int player, int card, event_t event, int who_gains){

    if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

		if( damage_card != source->internal_card_id || source->info_slot <= 0 ||
			source->damage_source_player != player || source->damage_source_card != card
		  ){
			return;
		}

		else{
			gain_life(who_gains, source->info_slot);
		}
	}
}

int card_spirit_link(int player, int card, event_t event){
/*
    card_instance_t *instance = get_card_instance(player, card);
    if( event == EVENT_ATTACK_LEGALITY && instance->targets[0].player == AI ){
        event_result = 1;
    }
*/
    card_instance_t *instance = get_card_instance(player, card);

	
    if( in_play(player, card) && instance->targets[0].player != -1 && instance->targets[0].card != -1 ){
		spirit_link_effect(instance->targets[0].player, instance->targets[0].card, event, player);
	}

    load_text(0, "TARGET_CREATURE");
    int result =  aura_pump(player, card, event, 0, 0);
    return result;
}

int card_in_the_presence_of_the_master(int player, int card, event_t event){
    if( event == EVENT_CAST_SPELL && ! affect_me(player, card) ){
        int p = affected_card_controller;
        int c = affected_card;
        card_data_t* card_d = get_card_data(p, c);
        if( ( card_d->type & TYPE_ENCHANTMENT) && card_d->cc[2] != 9  ){
            kill_card(p, c, KILL_DESTROY);
        }
    }
    return global_enchantment(player, card, event);
}

int card_story_circle(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;
	td.required_color = instance->info_slot;

	if (comes_into_play(player, card, event)){
		int color = choose_a_color(player, get_deck_color(player, 1-player));
		instance->info_slot = 1<<color;
		create_card_name_legacy(player, card, CARD_ID_BLACK+(color-1));
	}

	if( event == EVENT_CAN_ACTIVATE ){
		return instance->info_slot <= 0 ? 0 : generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_DAMAGE_PREVENTION, 0, 0, 0, 0, 0, 1, 0, &td, "TARGET_DAMAGE");
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_DAMAGE_PREVENTION, 0, 0, 0, 0, 0, 1, 0, &td, "TARGET_DAMAGE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			dmg->info_slot = 0;
		}
	}
	return global_enchantment(player, card, event);
}

int card_angelic_benediction( int player, int card, event_t event){
    int attacker = exalted(player, card, event, 0, 0);
	if (attacker != -1){
		target_definition_t td;
        default_target_definition(player, card, &td, TYPE_CREATURE);
		card_instance_t* instance = get_card_instance(player, card);

        if (select_target(player, card, &td, "Select target creature to tap.", NULL)){
            tap_card( instance->targets[0].player, instance->targets[0].card );
        }

		instance->number_of_targets = 0;
    }

	if( event == EVENT_CAN_CAST ){
        return 1;
    }

    return global_enchantment(player, card, event);;
}

