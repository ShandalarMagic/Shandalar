#include "manalink.h"

/*
Code that was changed in other files:

token_generation.c
int card_elemental_token(int player, int card, event_t event){

    // Rakka Mar Tokens
	if(get_special_infos(player, card) == 141 ){
		haste(player, card);
		modify_pt_and_abilities(player, card, event, 3, 1, 0);
	}
	else if( get_special_infos(player, card) > 0 && get_special_infos(player, card) < 100 ){
		int pow = get_special_infos(player, card);
		modify_pt_and_abilities(player, card, event, pow, pow, 0);
    }

    return 0;
}

upkeep.c
void echo(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[9].card = 1;
    }

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		if( instance->targets[9].card == 1 ){
			instance->targets[9].card--;
			int kill = 1;
			if( has_mana_multi(player, colorless, black, blue, green, red, white) ){
				int choice = do_dialog(player, player, card, -1, -1, " Pay Echo\n Pass", 0);
				if( choice == 0 ){
					charge_mana_multi(player, colorless, black, blue, green, red, white);
					if( spell_fizzled != 1 ){
						kill = 0;
					}
				}
			}

			if( kill == 1 ){
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}
}

// NEW FUNCTIONS TO ADD:

*/

// Choose a creature in a graveyard.  4th and 5th params are which graveyards to search
// instance->targets[0].player will hold the player whose yard was chosen
// instance->targets[10].player will hold the id of the card selected
// instance->targets[10].card will hold the index of the card in the graveyard
// Return the index of the card selected (-1 for none)
int pick_creature_in_a_graveyard(int player, int card, int mandatory, int current_player, int opponent){
	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	instance->targets[10].player = -1;
    
	int selected = -1;
	
	// Set the target player
	if( current_player ){
		instance->targets[0].player = player;
	} 
	else if( opponent ){
		instance->targets[0].player = 1 - player;
	}
			
	
	if(player != AI ){ 
		if( current_player && opponent ){
			pick_target(&td, "TARGET_PLAYER");
		}
		int done = 0;
		int count = 0;
		while( done == 0 ){
			selected = select_a_card(player, instance->targets[0].player, 2, 0, 2, -1, TYPE_CREATURE , 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( selected > -1 || mandatory == 0 || count++ == 50 ){
				done = 1;
			}
		}
    }
	else{
		if( current_player ){
			selected = select_a_card(player, player, 2, 1, 2, -1, TYPE_CREATURE , 0, 0, 0, 0, 0, 0, 0, -1, 0);
			instance->targets[0].player = player;
		}
		if( selected == -1 && opponent){ 
			selected = select_a_card(	1-player, player, 2, 1, 2, -1, TYPE_CREATURE , 0, 0, 0, 0, 0, 0, 0, -1, 0);
			instance->targets[0].player = 1-player;
        }
    }
	if( selected > -1 ){
		int *grave = graveyard_ptr[instance->targets[0].player];
		instance->targets[10].player = grave[selected];
		instance->targets[10].card = selected;
	}
	return selected;
}

int generic_legacy_effect(int player, int card, event_t event){
	return 0;
}

/* *************************************************************************************** */

// Flickerwisp goes to card_flickerwhisp() in eventide.c, not this.
int card_flickerwisp(int player, int card, event_t event){

	target_definition_t td;
    default_target_definition(player, card, &td, TYPE_PERMANENT);
    td.allow_cancel = 0;

    card_instance_t *instance = get_card_instance(player, card);

    if( comes_into_play(player, card, event) ){
		state_untargettable(player, card, 1);
		if (can_target(&td) && pick_target(&td, "TARGET_PERMANENT")){
			state_untargettable(player, card, 0);
			remove_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
        } else {
			state_untargettable(player, card, 0);
		}
    }

    return 0;
}

int card_kami_of_ancient_law(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
	
	card_instance_t *instance = get_card_instance(player, card);

    if(event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
    }
	
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_ENCHANTMENT");
}

int card_eight_and_a_half_tails(int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_PERMANENT);
    td.allowed_controller = player;
	td.preferred_controller = player;
	
	target_definition_t td1;
    default_target_definition(player, card, &td1, TYPE_PERMANENT);
    
    card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);
	
    if( event == EVENT_CAN_ACTIVATE && ! get_forbid_activation_flag(player, card) ){
		int result = card_spiketail_hatchling(player, card, event);
		if( result && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			return result;
		}
		
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			return can_target(&td1);
		}
    }
    else if( event == EVENT_ACTIVATE){
		// If there is a spell on the stack, assume that was the choice
		if( card_on_stack_controller > -1){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->targets[0].player = card_on_stack_controller;
				instance->targets[0].card = card_on_stack;
				instance->info_slot = 2;
			}
			return 0;
		}
		
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && can_target(&td) ){
			int clr = get_sleighted_color(player, card, COLOR_WHITE);
			const char* color = (clr == COLOR_BLACK ? "BLACK"
								 : clr == COLOR_BLUE ? "BLUE"
								 : clr == COLOR_GREEN ? "GREEN"
								 : clr == COLOR_RED ? "RED"
								 : "white");
			char buf[128];
			sprintf(buf, " Gains Prot. from %s\n Becomes %s", color, color);
			choice = do_dialog(player, player, card, -1, -1, buf, 0);
		}
		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1);
			if( spell_fizzled != 1 ){
				if( pick_target(&td, "TARGET_PERMANENT") ){
					instance->number_of_targets = 1;
					instance->info_slot = 0;
				}
			}	
		}
		else{
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				if( pick_target(&td1, "TARGET_PERMANENT") ){
					instance->number_of_targets = 1;
					instance->info_slot = 1;
				}
			}	
		}
    }
    else if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 0 && valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_PROT_WHITE, 0 );
		}
		else if( (instance->info_slot == 1 && valid_target(&td1) )
				 || instance->info_slot == 2 ){
			change_color(player, card, instance->targets[0].player, instance->targets[0].card, COLOR_TEST_WHITE, CHANGE_COLOR_SET|CHANGE_COLOR_END_AT_EOT);
		}
	}
	return 0;
}

int card_prison_term(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

    if(trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) && 
		reason_for_trigger_controller == player && trigger_cause_controller == 1-player 
	  ){
		int trig = 0;
		
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			trig =1 ;
		}
		if( trig == 1 && check_battlefield_for_id(2, CARD_ID_TORPOR_ORB) ){
			trig =0 ;
		}

        if(  trig == 1 ){
            if(event == EVENT_TRIGGER){
                event_result |= RESOLVE_TRIGGER_OPTIONAL;
            }
            else if(event == EVENT_RESOLVE_TRIGGER){
				instance->targets[0].player = trigger_cause_controller;
				instance->targets[0].card = trigger_cause;
				instance->damage_target_player = trigger_cause_controller;
				instance->damage_target_card = trigger_cause;
				gain_life(player, 1);
            }
        }
    }
    return card_arrest(player, card, event);
}

int card_magma_jet(int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE );
    td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
        return 1;
    }
    else if( event == EVENT_CAST_SPELL ){
        if(! affect_me(player, card)){ return 0; }
        load_text(0, "TARGET_CREATURE_OR_PLAYER");
        if(!select_target(player, card, &td, text_lines[0], NULL)){
            spell_fizzled = 1;
        }
    }
    else if(event == EVENT_RESOLVE_SPELL ){
        if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
			scry(player, 2);
        }
		kill_card(player, card, KILL_DESTROY );
    }
    return 0;
}

int card_flame_slash(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
        return can_target(&td);
    }
    else if( event == EVENT_CAST_SPELL && affect_me(player, card)){
			pick_target(&td, "TARGET_CREATURE");
    }
    else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
			}
			kill_card(player, card, KILL_DESTROY );
    }
    return 0;
}

int card_condescend(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);
	
	if( event == EVENT_CAN_CAST && card_on_stack_controller != -1 && card_on_stack != -1){
		if( ! check_state(card_on_stack_controller, card_on_stack, STATE_CANNOT_TARGET) && 
			! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG)
		  ){
			return 0x63;
		}
    }
	
    if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			instance->info_slot = x_value;
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
		}
	}

    if( event == EVENT_RESOLVE_SPELL ){
		charge_mana(instance->targets[0].player, COLOR_COLORLESS, instance->info_slot);
		if( spell_fizzled == 1 ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		}
		scry(player, 2);
		kill_card(player, card, KILL_DESTROY);
    }

    return 0;
}

int card_magus_of_the_scroll(int player, int card, event_t event){

    target_definition_t td1;
    default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
    td1.allow_cancel = 0;

    if( event == EVENT_CAN_ACTIVATE && hand_count[player] > 0 ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE_OR_PLAYER");
    }

    if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE_OR_PLAYER");
    }
    else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td1) ){
				if( hand_count[player] == 0 ){ return 0; }
				char msg[100] = "Select a card to name.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
				if( player == AI ){
					reveal_card(HUMAN, player, card, player, selected, "AI Names...", 0);
				}
				int sel2 = get_random_card_in_hand(player);
				reveal_card(player, player, card, player, sel2, "You revealed...", 0);
				if( sel2 == selected ){
					damage_creature_or_player(player, card, event, 2);
				}
			}
	}
    return 0;
}

int card_jack_in_the_mox(int player, int card, event_t event){
	if( event == EVENT_ACTIVATE ){
		tap_card(player, card);
		int roll = internal_rand(6);
		if( roll == 0 ){
			lose_life(player, 5);
			kill_card(player, card, KILL_SACRIFICE);
			spell_fizzled = 1;
			return 0;
		}
		else if( roll == 1 ){
			produce_mana(player, COLOR_WHITE, 1);
        }
		else if( roll == 2 ){
			produce_mana(player, COLOR_BLUE, 1);
		}
		else if( roll == 3 ){
			produce_mana(player, COLOR_BLACK, 1);
		}
		else if( roll == 4 ){
			produce_mana(player, COLOR_RED, 1);
		}
		else if( roll == 5 ){
			produce_mana(player, COLOR_GREEN, 1);
		}
		spell_fizzled = 1;
		return 0;
	}
    int result = mana_producer(player, card, event);
	return result;
}


int card_pulse_of_the_forge(int player, int card, event_t event){
    target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
    td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST ){
		return 1;
    }
    else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");   
    } 
    else if( event == EVENT_RESOLVE_SPELL ){
        card_instance_t *instance = get_card_instance(player, card);
        int target = instance->targets[0].player;
		damage_player(target, 4, player, card);
		if( life[player] < life[target] - 4 ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
    return 0;
}


int card_binding_grasp(int player, int card, event_t event){
    card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		if( event == EVENT_TOUGHNESS && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result++;
		}
	}
	basic_upkeep(player, card, event, 1, 0, 1, 0, 0, 0);
	return card_control_magic(player, card, event);
}

int card_arboria(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance( player, card );

	enchant_world(player, card, event);
	
	if( event == EVENT_RESOLVE_SPELL  ){
		instance->targets[1].player = 1;
		instance->targets[2].player = 1;
	}

	// You can only be attacked if you are not safe
    if( trigger_condition == TRIGGER_DECLARE_ATTACK && current_phase == PHASE_DECLARE_ATTACKERS ){
        if( affect_me( trigger_cause_controller, trigger_cause ) && reason_for_trigger_controller == affected_card_controller ){
			int trig = 0;
			if( instance->targets[(1-current_turn)+1].player == 1 ){
				trig = 1;
			}
			if( trig == 1 ){
                if(event == EVENT_TRIGGER){
                   event_result |= 2;
                }
                else if( event == EVENT_RESOLVE_TRIGGER){
                         forbid_attack = 1;
                }
			}
        }
    }


	// Reset "safeness" each turn
	if( current_phase == PHASE_UNTAP ){
		instance->targets[1 + current_turn].player = 1;
	}
	
	// keep track of if a player played a spell on their turn
	if( instance->targets[1 + current_turn].player == 1 && trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card)
       && player == reason_for_trigger_controller && trigger_cause_controller == current_turn )
    {
        if(event == EVENT_TRIGGER){
            event_result |= RESOLVE_TRIGGER_MANDATORY;
        }
        else if(event == EVENT_RESOLVE_TRIGGER){
            instance->targets[1 + current_turn].player = 0;
        }
    }
	
	// did a creature come into play?
    if( instance->targets[1 + current_turn].player == 1 && trigger_condition == TRIGGER_COMES_INTO_PLAY && 
		affect_me( trigger_cause_controller, trigger_cause ) && reason_for_trigger_controller == affected_card_controller && 
		trigger_cause_controller == current_turn
	  ){
        if( ! is_token( trigger_cause_controller, trigger_cause ) ){
			instance->targets[1 + current_turn].player = 0;
		}
    }
	
    return global_enchantment(player, card, event);
}

int card_arrogant_bloodlord(int player, int card, event_t event ){
	card_instance_t *instance = get_card_instance(player, card);
	if( current_turn == player && is_attacking(player, card) && event == EVENT_DECLARE_BLOCKERS ){
		int count = active_cards_count[1-player]-1;
		while( count > -1 ){
				if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
					card_instance_t *this = get_card_instance(1-player, count);
					if( this->blocking == card && get_power(1-player, count) < 2 ){
						instance->targets[3].player = 141;
					}
				}
				count--;
		}
	}

	if( current_turn != player && event == EVENT_DECLARE_BLOCKERS ){
		if( instance->blocking < 255 && get_power(1-player, instance->blocking) < 2 ){
			instance->targets[3].player = 141;
		}
	}
	
	if( instance->targets[3].player == 141 && current_phase >= PHASE_MAIN2 ){
		kill_card(player, card, KILL_SACRIFICE);
	}

    return 0;
}


// instance->targets[0].player will hold the player whose yard was chosen
// instance->targets[10].player will hold the id of the card selected
// instance->targets[10].card will hold the index of the card in the graveyard

int effect_exile_at_eot(int player, int card, event_t event ){
    card_instance_t *instance = get_card_instance(player, card);
    int p = instance->targets[0].player;
    int c = instance->targets[0].card;
	haste(p, c);
    if( current_turn == player && eot_trigger(player, card, event) ){
		kill_card(p, c, KILL_REMOVE);
    	kill_card(player, card, KILL_REMOVE);
    }
    return 0;
}

int card_teferis_moat(int player, int card, event_t event){
    card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
		 instance->info_slot = select_a_color(player);
	}
	if( event == EVENT_ATTACK_LEGALITY && current_turn == 1-player ){
		if( trigger_condition == TRIGGER_DECLARE_ATTACK && current_phase == PHASE_DECLARE_ATTACKERS ){
			if( affect_me( trigger_cause_controller, trigger_cause ) && 
				reason_for_trigger_controller == affected_card_controller
			  ){
				int trig = 0;
				if( get_color(trigger_cause_controller, trigger_cause) & instance->info_slot ){
					trig = 1;
				}
				if( trig == 1 ){
					if(event == EVENT_TRIGGER){
						event_result |= 2;
					}
					else if(event == EVENT_RESOLVE_TRIGGER){
							forbid_attack = 1;
					}
				}
			}
		}
    }
    return global_enchantment(player, card, event);
}

int card_razormane_masticore(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);	

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( hand_count[player] > 0 ){
			int choice = do_dialog(player, player, card, -1, -1, " Discard\n Pass", 0);
			if( choice == 0 ){
				kill--;
				discard(player, 0, 0);
			}
		}
		if( kill == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_DRAW_PHASE && current_turn == player ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
		}
	}

	return 0;
}


int get_random_permanent(int p){
	int count;
    int found = 0;
	for(count=active_cards_count[p]-1;count>=0;count--){
		card_data_t* card_d = get_card_data(p, count);
		if( (card_d->type & TYPE_PERMANENT) && in_play(p, count)){
			found++;
		}
    }
	if( found == 0 ){
		return -1;
	}
	
	int roll = internal_rand(found);
	found = 0;
	for(count=active_cards_count[p]-1;count>=0;count--){
		card_data_t* card_d = get_card_data(p, count);
		if( (card_d->type & TYPE_PERMANENT) && in_play(p, count)){
			if( found == roll ){
				return count;
			}
			found++;
		}
    }
	return -1;
}

int card_chaos_orb(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.illegal_abilities = 0;
	
    card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( in_play(player, instance->parent_card) ){
			int p = 1-player;
			int roll = internal_rand(100);
			if( roll < 5 ){
				p = player;
				roll+=50;
				roll+=internal_rand(50);
			}
			int killed = 0;
			int k;
			for(k=50; k<100; k+=10){
				if( roll > k ){
					killed++;
				}
			}
			if( killed > 0 ){
				int p_array[100];
				int pa_count = 0;
				for(k=0; k<active_cards_count[p]; k++){
					if( in_play(p, k) && is_what(p, k, TYPE_PERMANENT) ){
						p_array[pa_count] = k;
						pa_count++;
						if( pa_count > 99 ){
							break;
						}
					}
				}
				int rnd = internal_rand(pa_count);
				while( killed > 0 && rnd < pa_count ){
						if( ! is_token(p, p_array[rnd]) ){
							kill_card(p, p_array[rnd], KILL_DESTROY);
						}
						killed--;
				}
			}
			kill_card(player, instance->parent_card, KILL_DESTROY);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

