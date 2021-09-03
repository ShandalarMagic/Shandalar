#include "manalink.h"
int simple_dialog(int player, int card, const char* string){
    char buffer[20];
    load_text(0, string);
    snprintf(buffer, 20, " %s\n %s", text_lines[0], text_lines[1]);
    return do_dialog(player, player, card, -1, -1, buffer, 0);
}


int cmc_is_one(int player, int card){
    if( get_cmc(player, card) <= 1 ){
        return 1;
    }
    return 0;
}

int card_auriok_salvagers(int player, int card, event_t event){
	char msg[100] = "Select an artifact card with CMC 1 or less.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);
	this_test.cmc = 2;
	this_test.cmc_flag = 3;

    if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0) ){
			if( new_special_count_grave(player, &this_test) > 0 ){
				return ! graveyard_has_shroud(2);
            }
        }
    }
    if( event == EVENT_ACTIVATE ){
        if( charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1) ){
			if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1 ){
				spell_fizzled = 1;
			}
		}
    }
    if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
        }
    }
    return 0;
}


int card_magus_of_the_tabernacle(int player, int card, event_t event){
    return card_pendrell_mists(player, card, event);
}

void double_strike(int player, int card, event_t event){
    card_instance_t *instance = get_card_instance(player, card);

    // determine this creature's combat buddy
    if( current_phase == PHASE_FIRST_STRIKE_DAMAGE ){
        int buddy = -2;
        if(  instance->state & STATE_ATTACKING ){
            buddy = -1;
            int count = 0;
            while(count < active_cards_count[1-player]){
                if(in_play(1-player, count) ){
                    card_instance_t *instance2 = get_card_instance( 1-player, count);
                    if( instance2->blocking == card ){
                        buddy = count;
                        break;
                    }
                }
                count++;
            }
        }
        else if( instance->blocking < 255 ){
            buddy = instance->blocking;
        }
        instance->targets[8].card = buddy;
        poison_counters[AI] = buddy;
    }
    else if( current_phase == PHASE_NORMAL_COMBAT_DAMAGE ){
        poison_counters[HUMAN] = 1;
    if( instance->targets[8].card != -2 ){
        if( instance->targets[8].card == -1 ){
            instance->targets[8].card = -2;
            damage_player(1-player, get_power( player, card ), player, card);
        }
        else if( instance->targets[8].card > -1 && in_play(1-player, instance->targets[8].card) ){
            life[HUMAN] = instance->targets[8].card;
            damage_creature(1-player, instance->targets[8].card, get_power( player, card ), player, card);
            instance->targets[8].card = -2;
        }
    }}
    else if( event == EVENT_ABILITIES ){
        event_result |= KEYWORD_FIRST_STRIKE;
    }
}

int card_pearl_dragon(int player, int card, event_t event){
    return generic_shade(player, card, event, 0, 1, 0, 0, 0, 0, 1, 0, 1, 0, 0);
}

int card_ivory_guardians(int player, int card, event_t event){
    if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
        if( get_id(affected_card_controller, affected_card ) == get_id(player, card)){
            target_definition_t td;
            default_target_definition(player, card, &td, TYPE_PERMANENT);
            td.required_color = COLOR_TEST_RED;
            td.allowed_controller = 1-player;
            td.preferred_controller = 1-player;
            if( target_available(player, card, &td) ){
                event_result++;
            }
        }
    }
    return 0;
}

