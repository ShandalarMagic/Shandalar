#include "manalink.h"


int is_forest(int player, int card){
    if( has_subtype(player, card, SUBTYPE_FOREST )){
        return 1;
    }
    return 0;
}

int card_land_grant(int player, int card, event_t event){
    //if( player == AI ){ return 0; }
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.zone = TARGET_ZONE_HAND;

    if(event == EVENT_CAN_CAST ){
        return 1;
    }
    else if( event == EVENT_MODIFY_COST && ! can_target(&td) ){
			COST_GREEN--;
			COST_COLORLESS--;
    }
    else if(event == EVENT_RESOLVE_SPELL ){
			char msg[100] = "Select a Forest card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, msg);
			this_test.subtype = SUBTYPE_FOREST;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int card_living_wish(int player, int card, event_t event){
    return generic_wish(player, card, event, TYPE_CREATURE | TYPE_LAND, CARD_ID_GAEAS_CRADLE);
}

int card_overrun(int player, int card, event_t event){
    if(event == EVENT_CAN_CAST ){
        return 1;
    }
    else if(event == EVENT_RESOLVE_SPELL ){
            pump_subtype_until_eot(player, card, player, -1, 3, 3, KEYWORD_TRAMPLE, 0);
            kill_card(player, card, KILL_DESTROY );
    }
    return 0;
}

int is_swamp_or_mountain_or_island_or_plains(int player, int card ){
    if( has_subtype(player, card, SUBTYPE_SWAMP) || has_subtype(player, card, SUBTYPE_MOUNTAIN)
    || has_subtype(player, card, SUBTYPE_ISLAND) || has_subtype(player, card, SUBTYPE_PLAINS) ){
        return 1;
    }
    return 0;
}

int is_green(int player, int card){
    card_instance_t *instance = get_card_instance(player, card);
    if( instance->card_color & COLOR_TEST_GREEN ){
        return 1;
    }
    return 0;
}

int card_farseek(int player, int card, event_t event){
    if( event == EVENT_CAN_CAST ){
        return 1;
    }
    else if(event == EVENT_RESOLVE_SPELL ){
			char buffer[100] = "Select a Swamp, Island, Mountain or Plains card";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, buffer);
			this_test.subtype = SUBTYPE_SWAMP;
			this_test.multi_subtype_tutor = 1;
			this_test.sub2 = SUBTYPE_ISLAND;
			this_test.sub3 = SUBTYPE_MOUNTAIN;
			this_test.sub4 = SUBTYPE_PLAINS;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &this_test);
			kill_card(player, card, KILL_DESTROY );
    }
    return 0;
}


int card_reap_and_sow(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_LAND);
    td.preferred_controller = 1-player;

    card_instance_t *instance = get_card_instance(player, card);;

    if(event == EVENT_CAN_CAST ){
        return 1;
    }

    if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
        int choice = 0;
        if( target_available(player, card, &td) ){
            if( has_mana_multi( player, 1, 0, 0, 1, 0, 0 ) ) {
                char buffer[100];
                load_text(0, "REAP_AND_SOW");
                snprintf(buffer, 100, " %s\n %s\n %s", text_lines[0], text_lines[1], text_lines[2] );
                choice = do_dialog(player, player, card, -1, -1, buffer, 0);
            }
            else{
                 char buffer[100];
                 load_text(0, "REAP_AND_SOW");
                 snprintf(buffer, 100, " %s\n %s", text_lines[0], text_lines[1] );
                 choice = do_dialog(player, player, card, -1, -1, buffer, 0);
            }
        }
		else{
			 choice = 1;
        }
			 
        instance->info_slot = choice;

        if( choice == 2 ){
            charge_mana_multi( player, 1, 0, 0, 1, 0, 0 );
        }

        if( choice != 1){
			pick_target(&td, "TARGET_LAND");
        }

    }
    else if(event == EVENT_RESOLVE_SPELL ){
            int choice = instance->info_slot;
            if( choice != 1 ){
                if(validate_target(player, card, &td, 0)){
                    kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
                }
            }
            if( choice != 0 ){
				char buffer[100];
				scnprintf(buffer, 100, "Select a land card");
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_LAND, buffer);
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
            }
            kill_card(player, card, KILL_SACRIFICE);

    }

    return 0;
}

int card_tooth_and_nail(int player, int card, event_t event){
    card_instance_t *instance = get_card_instance(player, card);

    if(event == EVENT_CAN_CAST ){
        return 1;
    }
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.cmc = 5;
			this_test.cmc_flag = 2;
			this_test.zone = TARGET_ZONE_HAND;

			int choice = 0;
			int ai_choice = 0;
			if( has_mana(player, COLOR_COLORLESS, 2) ){
				choice = do_dialog(player, player, card, -1, -1, " Search for 2 creatures.\n Put 2 Creatures into play.\n Do both.\n Cancel", 2);
			}
			else{
				if( check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test) > 1 ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Search for 2 creatures.\n Put 2 Creatures into play.\n Cancel", ai_choice);
				if( choice > 1 ){
					choice++;
				}
			}
			if( choice == 3 ){
				spell_fizzled = 1;
			}
			else{
				if( choice == 2 ){
					charge_mana_multi( player, 2, 0, 0, 0, 0, 0 );
				}
				if( spell_fizzled != 1 ){
					instance->info_slot = choice+1;
				}
			}
    }
    else if(event == EVENT_RESOLVE_SPELL ){
			char buffer[100];
			scnprintf(buffer, 100, "Select a creature card");
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
			int i;
			for(i=0; i<2; i++){
				if( instance->info_slot & (1<<i) ){
					int from = TUTOR_FROM_DECK;
					int dest = TUTOR_HAND;
					int mode = AI_MAX_VALUE;
					if( instance->info_slot & (1<<1) ){
						mode = AI_MAX_CMC;
					}
					if( i == 1 ){
						from = TUTOR_FROM_HAND;
						dest = TUTOR_PLAY;
						mode = AI_MAX_CMC;
					}
					new_global_tutor(player, player, from, dest, 0, 10+mode, &this_test);
					new_global_tutor(player, player, from, dest, 0, mode, &this_test);
				}
			}
			kill_card(player, card, KILL_SACRIFICE);
    }
    return 0;
}

int card_eureka(int player, int card, event_t event){
    if( event == EVENT_CAN_CAST ){
        return 1;
    }
    if( event == EVENT_RESOLVE_SPELL ){
		char msg[100] = "Choose a permanent to put into play.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, msg);

        int cancel0 = 0;
        int cancel1 = 0;
        while( cancel0 + cancel1 > -2){
				cancel0 = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
				cancel1 = new_global_tutor(1-player, 1-player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
        }
        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int card_early_harvest(int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
    td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

    card_instance_t *instance = get_card_instance(player, card);

    if(event == EVENT_CAN_CAST){
        return 1;
    }
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
    }
    else if(event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_LAND);
				this_test.subtype = SUBTYPE_BASIC;
				new_manipulate_all(player, card, instance->targets[0].player, &this_test, ACT_UNTAP);
			}
			kill_card(player, card, KILL_SACRIFICE);
    }
    return 0;
}

int effect_glimpse_of_nature(int player, int card, event_t event){
    if(trigger_condition == TRIGGER_SPELL_CAST && player == affected_card_controller
       && player == reason_for_trigger_controller && player == trigger_cause_controller && card == affected_card )
    {
        card_instance_t *instance = get_card_instance(trigger_cause_controller, trigger_cause);
        if( cards_data[ instance->internal_card_id].type & TYPE_CREATURE ){
            if(event == EVENT_TRIGGER){
                event_result |= RESOLVE_TRIGGER_MANDATORY;
            }
            else if(event == EVENT_RESOLVE_TRIGGER){
                draw_a_card(player);
            }
        }
    }
    if( eot_trigger(player, card, event ) ){
        kill_card(player, card, KILL_REMOVE);
    }
    return 0;
}

int card_glimpse_of_nature(int player, int card, event_t event){
    if(event == EVENT_CAN_CAST ){
        return 1;
    }
    if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( has_mana(player, COLOR_COLORLESS, 6) ){
			ai_modifier+=50;
		}
    }
    if( event == EVENT_RESOLVE_SPELL ){
        create_legacy_effect(player, card, &effect_glimpse_of_nature );
        kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

