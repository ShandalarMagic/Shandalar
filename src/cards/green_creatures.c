#include "manalink.h"

int card_gaeas_herald(int player, int card, event_t event){
    if( event == EVENT_CAST_SPELL && in_play(player, card) ){
        card_instance_t *instance = get_card_instance(affected_card_controller, affected_card);
        if( cards_data[ instance->internal_card_id ].type & TYPE_CREATURE ){
            instance->state |= STATE_CANNOT_TARGET;
        }
    }
    if(trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) && reason_for_trigger_controller == player ){
        card_instance_t *instance = get_card_instance(trigger_cause_controller, trigger_cause);
        if(  cards_data[ instance->internal_card_id].type & TYPE_CREATURE  ){
            if(event == EVENT_TRIGGER){
                event_result |= RESOLVE_TRIGGER_MANDATORY;
            }
            else if(event == EVENT_RESOLVE_TRIGGER){
                instance->state &= ~STATE_CANNOT_TARGET;
            }
        }
    }
    return 0;
}

int card_wall_of_roots(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);

    if( event == EVENT_CAN_ACTIVATE ){
        if( instance->info_slot != 1 ){
            return 1;
        }
    }

    if(event == EVENT_ACTIVATE ){
		instance->info_slot = 1;
		add_0_minus1_counters(player, card, 1);
		produce_mana(player, COLOR_GREEN, 1);
    }

    if( event == EVENT_COUNT_MANA ){
        if( instance->info_slot != 1 && affect_me(player, card) ){
            declare_mana_available(player, COLOR_GREEN, 1);
        }
    }
    if( current_phase == PHASE_DISCARD ){
        instance->info_slot = 0;
    }

    return 0;
}

int card_tinder_wall(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

    card_instance_t *instance = get_card_instance(player, card);

    if( event == EVENT_CAN_ACTIVATE ){
		if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return 1;
		}
    }
    if( event == EVENT_ACTIVATE ){
        int choice = 0;
        if( ! paying_mana() && ! get_forbid_activation_flag(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) ){
			if( instance->blocking < 255 ){
				instance->targets[0].player = 1-player;
				instance->targets[0].card = instance->blocking;
				instance->number_of_targets = 1;
				if( valid_target(&td) ){
					choice = do_dialog(player, player, card, -1, -1, " Generate Mana\n Shoot a blocker\n Cancel", 1);
				}
			}
        }

        if( choice == 0){
            produce_mana(player, COLOR_RED, 2);
            kill_card(player, card, KILL_SACRIFICE);
        }
        else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) ){
					instance->targets[0].player = 1-player;
					instance->targets[0].card = instance->blocking;
					instance->number_of_targets = 1;
					instance->info_slot = 1;
					kill_card(player, card, KILL_SACRIFICE);
				}
        }
        else{
			spell_fizzled = 1;
        }
    }
    if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 && valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card );
		}
    }
	
	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		declare_mana_available(player, COLOR_RED, 2);
	}
    return 0;
}

int card_argothian_enchantress(int player, int card, event_t event){
    if(trigger_condition == TRIGGER_SPELL_CAST && player == affected_card_controller
       && player == reason_for_trigger_controller && player == trigger_cause_controller && card == affected_card )
    {
        if (is_what(trigger_cause_controller, trigger_cause, TYPE_ENCHANTMENT)){
            if(event == EVENT_TRIGGER){
                event_result |= RESOLVE_TRIGGER_MANDATORY;
            }
            else if(event == EVENT_RESOLVE_TRIGGER){
                draw_a_card(player);
            }
        }
    }
    return 0;
}

int card_elvish_spirit_guide(int player, int card, event_t event){
    if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
        return 1;
    }
    else if( event == EVENT_ACTIVATE_FROM_HAND ){
        // remove this from the game
        discard_card(player, card);
        rfg_card_from_grave(player, count_graveyard(player) - 1);
    }
    else if( event == EVENT_REVOLVE_ACTIVATION_FROM_HAND ){
        produce_mana(player, COLOR_GREEN, 1);
    }
    return 0;
}

int card_gorilla_titan(int player, int card, event_t event){
    if(event == EVENT_POWER || event == EVENT_TOUGHNESS){
        if( affect_me(player, card) && count_graveyard(player) == 0){
            event_result += 4;
        }
    }
    return 0;
}



/* Also used by Viridian Shaman */
int card_uktabi_orangutan(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		card_instance_t* instance = get_card_instance(player, card);
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_ARTIFACT")){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_terravore(int player, int card, event_t event){
    if(event == EVENT_POWER || event == EVENT_TOUGHNESS){
        if(affect_me(player, card ) ){
            int count = count_graveyard_by_type(HUMAN, TYPE_LAND);
            count += count_graveyard_by_type(AI, TYPE_LAND);
            event_result += count;
        }
    }
    return 0;
}

