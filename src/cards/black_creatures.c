#include "manalink.h"

#include <windows.h>

void
popup(const char* title, const char* fmt, ...)
{
  char buf[8000];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 8000, fmt, args);
  va_end(args);

  MessageBox(0, buf, title, MB_ICONERROR|MB_TASKMODAL);
}

int card_horobi_deaths_wail(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );
    check_legend_rule(player, card, event);
    if( in_play(player, card) && ! ai_is_speculating ){
		if( specific_spell_played(player, card, event, 2, 2, TYPE_LAND | TYPE_EFFECT, 1, 0, 0, 0, 0, 0, 0, -1, 0) ){
			int p = instance->targets[1].player;
			int c = instance->targets[1].card;
			card_instance_t *source = get_card_instance( p, c );
			int i;
			for(i=0; i<source->number_of_targets; i++){
				if( source->targets[i].card != -1 && is_what(source->targets[i].player, source->targets[i].card, TYPE_CREATURE) ){
					kill_card(source->targets[i].player, source->targets[i].card, KILL_DESTROY);
				}
			}
		}
		if (stack_size >= 1 ){
			if( (cards_data[get_card_instance(stack_cards[stack_size - 1].player, stack_cards[stack_size - 1].card)->internal_card_id].type & TYPE_EFFECT)
			  && stack_data[stack_size - 1].generating_event != EVENT_RESOLVE_TRIGGER       // Redundant for now, since the engine doesn't let you respond to triggers with interrupts OR fast effects.
			  && cards_data[stack_data[stack_size - 1].internal_card_id].id != 904         // "Draw a card" effect in draw phase
			  ){
				card_instance_t *source = get_card_instance( stack_cards[stack_size - 1].player, stack_cards[stack_size - 1].card );
				int p = source->parent_controller;
				int c = source->parent_card;
				card_instance_t *source2 = get_card_instance( p, c );
				int i;
				for(i=0; i<source2->number_of_targets; i++){
					if( source2->targets[i].card != -1 && is_what(source2->targets[i].player, source2->targets[i].card, TYPE_CREATURE) ){
						kill_card(source2->targets[i].player, source2->targets[i].card, KILL_DESTROY);
					}
				}
			}
		}
    }
    return 0;
}


int card_swamp_mosquito(int player, int card, event_t event){
    if( event == EVENT_DECLARE_BLOCKERS ){
        if( is_unblocked(player, card) ){
            poison_counters[1-player]++;
        }
    }
    return 0;
}

int card_suqata_assassin(int player, int card, event_t event){
    fear(player, card, event);
    return card_swamp_mosquito(player, card, event);
}

int card_zombie_master(int player, int card, event_t event){
    if( event == EVENT_ABILITIES && affect_me(player, card )){
        return 0;
    }
    return card_zombie_master_exe(player, card, event);
}

int card_nightscape_familiar(int player, int card, event_t event){
    if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
        card_instance_t *instance = get_card_instance( affected_card_controller, affected_card);
        if(  affected_card_controller == player && ((instance->card_color & COLOR_TEST_RED) || (instance->card_color & COLOR_TEST_BLUE)) ){
            COST_COLORLESS--;
        }
    }
    return regeneration(player, card, event, 1, 1, 0, 0, 0, 0);
}

void dark_confidant_effect(int player, int card, int t_player){
    int *deck = deck_ptr[t_player];
	if( deck[0] != -1 ){
		int card_added = add_card_to_hand(t_player, deck[0] );
		remove_card_from_deck( t_player, 0 );
		int cmc = get_cmc(t_player, card_added);
		lose_life(t_player, cmc);
		reveal_card(HUMAN, player, card, t_player, card_added, "Reveals", 0);
	}
}

int card_dark_confidant(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);	

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		dark_confidant_effect(player, card, player);
	}

    return 0;
}

int disciple_of_the_vault_legacy(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_ARTIFACT);
	this_test.type_flag = F1_NO_TOKEN;

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );
	
	if( instance->targets[1].player > -1 && in_play(instance->targets[1].player, instance->targets[1].card) ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( ! in_play(affected_card_controller, affected_card) ){ return 0; } 
				if( new_make_test_in_play(affected_card_controller, affected_card, -1, &this_test) ){
					card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
					if( affected->kill_code > 0 && affected->kill_code < 4 ){
						if( instance->targets[11].player < 0 ){
							instance->targets[11].player = 0;
						}
						instance->targets[11].player++;
						instance->targets[11].card = 0;
					}
				}
		}

		if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller && 
			affect_me(player, card ) 
		  ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
			if( valid_target(&td) ){
				if(event == EVENT_TRIGGER){
					event_result |= 2;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						lose_life(instance->targets[0].player, instance->targets[11].player);
						instance->targets[11].player = 0;
				}
			}
		}
    }
	
	if( ! in_play(instance->targets[1].player, instance->targets[1].card) && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
    }

	return 0;
}

int card_disciple_of_the_vault(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_effect(player, card, &disciple_of_the_vault_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[1].player = player;
		leg->targets[1].card = card;
	}
	
	return 0;
}

int card_scavenger_drake(int player, int card, event_t event){
    card_instance_t *instance= get_card_instance(player, card);
    if(event == EVENT_GRAVEYARD_FROM_PLAY){
        if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
        card_instance_t *affected= get_card_instance(affected_card_controller, affected_card);
        if( affected->kill_code != 4) {
            if( cards_data[ affected->internal_card_id ].type & TYPE_CREATURE ){
                if( instance->targets[11].player < 0 ){
                    instance->targets[11].player = 0;
                }
                instance->targets[11].player++;
            }
        }
    }

    if( resolve_graveyard_trigger(player, card, event) == 1 ){
		int i;
		for(i=0; i<instance->targets[11].player; i++){
			add_1_1_counter(player, card);
		}
        instance->targets[11].player = 0;
    }
    return 0;
}

int card_demonic_hordes( int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_LAND);

    card_instance_t *instance= get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);	

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( has_mana_multi(player, 0, 3, 0, 0, 0, 0) ){
			int choice = do_dialog(player, player, card, -1, -1, " Pay Upkeep\n Pass", 0);
			if( choice == 0 ){
				charge_mana_multi(player, 0, 3, 0, 0, 0, 0);
				if( spell_fizzled != 1 ){
					kill = 0;
				}
			}
		}
		if( kill > 0 ){
			tap_card(player, card);
			impose_sacrifice(player, card, player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

    if(event == EVENT_RESOLVE_ACTIVATION){
        if( valid_target(&td) ){
            kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
        }
    }
    return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_LAND");
}

int card_hells_caretaker(int player, int card, event_t event){

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

    card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && ! get_forbid_activation_flag(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( ! is_tapped(player, card) && ! is_sick(player, card) && current_turn == player && current_phase == PHASE_UPKEEP &&
				count_graveyard_by_type(player, TYPE_CREATURE) > 0 && count_upkeeps(player) > 0 &&
				can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)
			  ){	
				return ! graveyard_has_shroud(2);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int selected = new_select_target_from_grave(player, card, player, 0, AI_MAX_CMC, &this_test, 0);
			if( selected != -1 ){
				if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					int *grave = graveyard_ptr[player];
					instance->targets[1].player = selected;
					instance->targets[1].card = grave[selected];
					tap_card(player, card);
				}
				else{
					 spell_fizzled = 1;
				}
			}
			else{
				 spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, player, 1);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, 0, 0);
		}
	}

    return 0;
}

int card_ashen_ghoul(int player, int card, event_t event){
  haste(player, card);

  if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
     int position = 0;
     int *graveyard = graveyard_ptr[player];
     int victims = 0;
     int count = count_graveyard(player) - 1;
     while( count > -1 ){
           if( cards_data[  graveyard[count] ].id == CARD_ID_ASHEN_GHOUL ){
              position = count;
           }
           count--;
     }

     count = count_graveyard(player) - 1;
     while( count > position ){
           if( (cards_data[ graveyard[count] ].type & TYPE_CREATURE) ){
              victims++;
           }
           count--;
     }

     if( victims >= 3){
        int choice = do_dialog(player, player, card, -1, -1," Return Ashen Ghoul\n Do not return Ashen Ghoul\n", 0);
        if( choice == 0 ){
           charge_mana_multi(player, 0, 1, 0, 0, 0, 0);
           if( spell_fizzled != 1){
              put_into_play(player, card);
              return -1;
           }
        }
     }
     return -2;
  }

 return 0;
}

int card_nshadow(int player, int card, event_t event){
  haste(player, card);

  if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
     int position = 0;
     int *graveyard = graveyard_ptr[player];
     int victims = 0;
     int count = count_graveyard(player) - 1;
     while( count > -1 ){
           if( cards_data[  graveyard[count] ].id == CARD_ID_NETHER_SHADOW ){
              position = count;
           }
           count--;
     }

     count = count_graveyard(player) - 1;
     while( count > position ){
           if( (cards_data[ graveyard[count] ].type & TYPE_CREATURE) ){
              victims++;
           }
           count--;
     }

     if( victims >= 3){
        int choice = do_dialog(player, player, card, -1, -1," Return Nether Shadow\n Do not return Nether Shadow\n", 0);
        if( choice == 0 ){
           put_into_play(player, card);
           return -1;
        }
     }
     return -2;
  }

 return 0;
}


