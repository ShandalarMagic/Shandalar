#include "manalink.h"

int card_zombie_master2(int player, int card, event_t event){
    // do the swampwalk bit
    if( event == EVENT_ABILITIES ){
        if( has_creature_type(affected_card_controller, affected_card, SUBTYPE_ZOMBIE) && ! affect_me(player, card) ){
            event_result |= get_hacked_walk(player, card, KEYWORD_SWAMPWALK);
        }
    }

    // do the regeneration bit
    card_instance_t *instance = get_card_instance(player, card);
    if( instance->info_slot == 1 ){
        return 0;
    }
    instance->info_slot = 1;
    if( ! get_forbid_activation_flag(player, card) && is_creature_dead() && ( land_can_be_played & 0x200) ){
        int p=0;
        for(p=0;p<2;p++){
            int c=0;
            for(c=0;c< active_cards_count[p];c++){
                card_data_t* card_d = get_card_data(p, c);
                if( !(p==player && c==card) && (card_d->type & TYPE_CREATURE) && in_play(p, c) && has_creature_type(p, c, SUBTYPE_ZOMBIE) ){
                    card_instance_t *creature = get_card_instance(p, c);
                    if( creature->kill_code == KILL_DESTROY && has_mana(p, COLOR_BLACK, 1) ){
                        int choice = do_dialog(p, player, card, p, c, " Regenerate for B\n Do not regenerate", 0);
                        if( choice == 0 ){
                            charge_mana(p, COLOR_BLACK, 1);
                            if( spell_fizzled != 1 ){
                                regenerate_target(p, c);
                            }
                        }
                    }
                }
            }
        }
    }
    instance->info_slot = 0;
    return 0;
}


int card_spinneret_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_REACH, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_triskelion2(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		add_1_1_counters(player, card, 3);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_1_1_COUNTER+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}


int card_lord_of_atlantis2(int player, int card, event_t event){

  if( event == EVENT_ABILITIES && in_play(affected_card_controller, affected_card) ){
     if( affect_me(player, card) ){ return 0; }
     if( has_subtype(affected_card_controller, affected_card, SUBTYPE_MERFOLK) ){
        event_result |= get_hacked_walk(player, card, KEYWORD_ISLANDWALK);
     }
  }

  if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
     if( affect_me(player, card) ){ return 0; }
     if( has_subtype(affected_card_controller, affected_card, SUBTYPE_MERFOLK) ){
        event_result++;
     }
  }

  return 0;
}

int card_bribery(int player, int card, event_t event){
// original code : 0040F730
	return steal_permanent_from_target_opponent_deck(player, card, event, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
}

int card_kid_ape2(int player, int card, event_t event){
// original code : 00453E50

  if( event == EVENT_POWER && affect_me(player, card) && check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_FOREST) ){
      event_result++;
  }

  if( event == EVENT_TOUGHNESS && affect_me(player, card) &&
      check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_FOREST)
    ){
      event_result+=2;
  }

  return 0;
}


int card_tetravus2(int player, int card, event_t event){
//  original code : 00421590

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		add_1_1_counters(player, card, 3);
		int ids = internal_rand(100);
		instance->targets[8].player = ids;
	}

	upkeep_trigger_ability(player, card, event, player);	

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int tetravites = 0;
		int i;
		int choice;

		if( count_1_1_counters(player, card) > 0 ){
			choice = do_dialog(player, player, card, -1, -1,
                             " Release some Tetravites (Auto)\n Release some Tetravites (Manual)\n Pass", 0);
			int number = 0;

			if( choice == 0 ){
				number = count_1_1_counters(player, card);
			}

			if( choice == 1 ){
				number = choose_a_number(player, "Remove how many counters?", count_1_1_counters(player, card));
				if( number > count_1_1_counters(player, card) ){
					number = count_1_1_counters(player, card);
				}
			}

			if( number > 0 ){
				for(i=0; i<number;i++){
					remove_1_1_counter(player, card);
				}
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_TETRAVITE, &token);
				token.qty = number;
				token.special_infos = instance->targets[8].player;
				generate_token(&token);
			}
		}

		for(i =0; i<2; i++){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && get_id(i, count) == CARD_ID_TETRAVITE ){
						if( instance->targets[8].player == get_special_infos(i, count) ){
							tetravites++;
						}
					}
					count++;
			}
		}

		if( tetravites > 0 ){
			int ai_choice = 2;
			if( check_battlefield_for_id(player, CARD_ID_DOUBLING_SEASON) ||
				check_battlefield_for_id(player, CARD_ID_PARALLEL_LIVES) ){
				ai_choice = 0;
			}

			choice = do_dialog(player, player, card, -1, -1, " Dock some Tetravites (Auto)\n Dock some Tetravites (Manual)\n Pass", ai_choice);
			if( choice == 0 ){
				i=0;
				for(i =0; i<2; i++){
					int count = 0;
					while( count < active_cards_count[i] ){
							if( in_play(i, count) && get_id(i, count) == CARD_ID_TETRAVITE ){
								if( instance->targets[8].player == get_special_infos(i, count) ){
									kill_card(i, count, KILL_SACRIFICE);
									add_1_1_counter(player, card);
								}
							}
							count++;
					}
				}
			}

			if( choice == 1 ){
				while( tetravites > 0 ){
						if( pick_target(&td, "TETRAVITE") ){
							if( get_id(instance->targets[0].player, instance->targets[0].card) == get_id_by_name("Tetravite") ){
								if( instance->targets[8].player == get_special_infos(instance->targets[0].player,instance->targets[0].card) ){
									kill_card(instance->targets[0].player,instance->targets[0].card, KILL_SACRIFICE);
									add_1_1_counter(player, card);
									tetravites--;
								}	

							}
						}
						else{
							break;
						}
				}
			}

		}	
	}

	return 0;
}



int card_slith_predator(int player, int card, event_t event){

    if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
        add_1_1_counter(player, card);
    }

    return 0;
}

int card_slith_firewalker(int player, int card, event_t event){

    haste(player, card);

    return card_slith_predator(player, card, event);
}

int card_slith_bloodletter(int player, int card, event_t event){

    if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
        add_1_1_counter(player, card);
    }

    return regeneration(player, card, event, 1, 1, 0, 0, 0, 0);
}

int card_juggernaut(int player, int card, event_t event){
// original code : 00453C40

    attack_if_able(player, card, event);

	if(event == EVENT_BLOCK_LEGALITY ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( has_subtype(affected_card_controller, affected_card, SUBTYPE_WALL) ){
				event_result = 1;
			}    
		}      
	}

   return 0;
}

int card_carven_caryatid(int player, int card, event_t event){
// original code : 0040E990

   if( event == EVENT_RESOLVE_SPELL ){
       draw_cards(player, 1);
   }

   cannot_attack(player, card, event);

   return 0;
}

int card_swords_to_plowshares2(int player, int card, event_t event){
	// original code : 004A3750
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);

    card_instance_t *instance = get_card_instance( player, card);

    if(event == EVENT_CAN_CAST ){
        return can_target(&td);
    }
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
            pick_target(&td, "TARGET_CREATURE");
    }
    else if( event == EVENT_RESOLVE_SPELL ){
             if( valid_target(&td) ){
				gain_life(instance->targets[0].player, get_power(instance->targets[0].player, instance->targets[0].card));
				rfg_target_permanent(instance->targets[0].player, instance->targets[0].card);
             }
             kill_card(player, card, KILL_DESTROY);
    }
    return 0;
}

int card_fog2(int player, int card, event_t event){
// original code : 004A3700
// also code for Holy Day and Darkness

    if( event == EVENT_RESOLVE_SPELL ){
			 fog_effect(player, card);
			 kill_card(player, card, KILL_DESTROY);
    }
	else{
		 return card_fog(player, card, event);
	}

    return 0;
}

int card_lady_evangela(int player, int card, event_t event){
// original code : 0040CD80

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;
	
	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);
	
    if( event == EVENT_CAN_ACTIVATE && ! get_forbid_activation_flag(player, card) && has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 1) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) ){
			return can_target(&td);
		}
    }

    else if(event == EVENT_ACTIVATE ){
            charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 1);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				tap_card(player, card);
			}
	}

    else if( event == EVENT_RESOLVE_ACTIVATION ){
			 negate_combat_damage_this_turn(player, instance->parent_card, 
											instance->targets[0].player, instance->targets[0].card, 0);
    }

    return 0;
}

int card_angus_mackenzie(int player, int card, event_t event){
// original code : 00406250

	check_legend_rule(player, card, event);
	
	card_instance_t *instance = get_card_instance(player, card);

    if( event == EVENT_CAN_ACTIVATE && ! get_forbid_activation_flag(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 1, 1, 0, 1) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && current_phase < PHASE_DECLARE_ATTACKERS ){
			return 1;
		}
    }

    else if(event == EVENT_ACTIVATE ){
            charge_mana_for_activated_ability(player, card, 0, 0, 1, 1, 0, 1);
			if( spell_fizzled != 1 ){
				tap_card(player, card);
			}
	}

    else if( event == EVENT_RESOLVE_ACTIVATION ){
			 fog_effect(player, instance->parent_card);
    }

    return 0;
}

int card_horn_of_deafening(int player, int card, event_t event){
// original code : 00405F70

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	
	card_instance_t *instance = get_card_instance(player, card);

    if( event == EVENT_RESOLVE_ACTIVATION ){
		negate_combat_damage_this_turn(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0);
    }

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int maze_of_ith_legacy(int player, int card, event_t event){
    card_instance_t *instance = get_card_instance( player, card );

    if( event == EVENT_PREVENT_DAMAGE && 
        (current_phase == PHASE_FIRST_STRIKE_DAMAGE || current_phase == PHASE_NORMAL_COMBAT_DAMAGE) 
	  ){
        card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

        if( damage_card != source->internal_card_id || source->info_slot <= 0 ){
            return 0;
        }

		if( source->damage_source_player == instance->targets[0].player && 
            source->damage_source_card == instance->targets[0].card
		  ){
             source->info_slot = 0;
        }

		if( source->damage_target_player == instance->targets[0].player && 
            source->damage_target_card == instance->targets[0].card
		  ){
             source->info_slot = 0;
        }

	}

    if( eot_trigger(player, card, event) ){
		card_instance_t *trg = get_card_instance( instance->targets[0].player, instance->targets[0].card );
		if( trg->targets[14].card == 66  ){
			trg->targets[14].card = -1;
		}
        kill_card(player, card, KILL_REMOVE);
    }

    return 0;
}

int maze_of_ith_effect(int player, int card, int t_player, int t_card){
	card_instance_t *trg = get_card_instance(t_player, t_card);
	trg->targets[14].card = 66;
	int legacy = create_targetted_legacy_effect( player, card, &maze_of_ith_legacy, t_player, t_card);
	return legacy;
}

int card_maze_of_ith2(int player, int card, event_t event){
// original code : 00406100

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;
	
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

    if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=100;
    }

    if( event == EVENT_RESOLVE_ACTIVATION ){
		if(valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
			maze_of_ith_effect(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
    }

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_nuisance_engine(int player, int card, event_t event){
// original code : 004DAC3A

    if( event == EVENT_RESOLVE_ACTIVATION ){
        generate_token_by_id(player, CARD_ID_PEST);
    }

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

int clone_legacy(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance( player, card );

	if(	trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) && reason_for_trigger_controller == affected_card_controller ){
		if( trigger_cause_controller == instance->targets[0].player && trigger_cause == instance->targets[0].card ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					card_instance_t *orig = get_card_instance( instance->targets[0].player, instance->targets[0].card );
					orig->targets[10].player = 66;
					if( instance->targets[1].card == CARD_ID_EVIL_TWIN ){
						int effect = generate_token_by_id(player, CARD_ID_EVIL_TWIN_EFFECT);
						card_instance_t *twin = get_card_instance(player, effect);
						twin->targets[9].player = player;
						twin->targets[9].card = card;
						twin->targets[8].card = get_id(instance->targets[0].player, instance->targets[0].card);
					}
			}
		}
    }
	
	return 0;
}


int card_clone_fixed(int player, int card, event_t event ){
// original code :  020027DF 

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
    td.illegal_abilities = 0;
	
    card_instance_t *instance = get_card_instance(player, card);

    if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
        if( pick_target(&td, "TARGET_CREATURE") ){
			cloning(player, card, instance->targets[0].player, instance->targets[0].card);
			int legacy = create_targetted_legacy_effect(player, card, &clone_legacy, player, card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].card = get_id(player, card);
		}
    }
	
	cloning_card(player, card, event);

    return 0;
}

int card_fire_elemental(int player, int card, event_t event ){
// original code : 00401000

	if( get_special_infos(player, card) > 0 ){
		int amount = get_special_infos(player, card);
		if( event == EVENT_POWER && affect_me(player, card)  ){
			event_result+=(amount-5);
		}
		if( event == EVENT_TOUGHNESS && affect_me(player, card)  ){
			event_result+=(amount-4);
		}
	}

    return 0;
}


int card_fork_fixed(int player, int card, event_t event){
// original code : 

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[2].player != -1 && instance->targets[2].card != -1 ){
		int p = instance->targets[2].player;
		int c = instance->targets[2].card;
		if( event == EVENT_SET_COLOR && affect_me(p, c) ){
			event_result = COLOR_TEST_RED;
		}
	}

    if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			if( ! is_what(card_on_stack_controller, card_on_stack, TYPE_PERMANENT) ){
				instance->targets[1].player = card_on_stack_controller;
				instance->targets[1].card = card_on_stack;
				return result;
			}
		}
		else{
			 return 0;
		}
    }

    else if( event == EVENT_RESOLVE_SPELL ){
			 int id = get_id(instance->targets[1].player, instance->targets[1].card);
			 instance->targets[2].player = player;
			 instance->targets[2].card = copy_spell(player, id);
			 kill_card(player, card, KILL_DESTROY);
    }
    else{
		 return card_counterspell(player, card, event);
    }

    return 0;
}

int card_rock_hydra(int player, int card, event_t event){
// original code : 00454ED0

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 add_1_1_counters(player, card, instance->info_slot);
 			 instance->info_slot = 0;
	}
	else{
		return card_rock_hydra_exe(player, card, event);
	}
	return 0;
}

int card_coastal_piracy(int player, int card, event_t event){
// original code : 00405650
    card_instance_t *instance = get_card_instance(player, card);

    if( current_turn == player && event == EVENT_DEAL_DAMAGE &&
		( current_phase == PHASE_FIRST_STRIKE_DAMAGE || current_phase == PHASE_NORMAL_COMBAT_DAMAGE ) 
	  ){
        card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
        if( damage->internal_card_id == damage_card ){
            if( damage->damage_source_player == player && damage->damage_target_card == -1 ){
				if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) ){
					card_instance_t *target = get_card_instance(damage->damage_source_player, damage->damage_source_card);
					if( damage->info_slot > 0 || target->targets[16].player > 0 ){
						instance->info_slot++;
					}
				}
            }
        }
    }
    
    if(	trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && 
		reason_for_trigger_controller == player && instance->info_slot > 0 ){
        if(event == EVENT_TRIGGER){
            event_result |= 2;
        }
        else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0; i<instance->info_slot; i++){
					draw_some_cards_if_you_want(player, card, player, 1);
				}
				instance->info_slot = 0;
        }
    }

    return global_enchantment(player, card, event);
}

int card_craw_wurm(int player, int card, event_t event ){
// original code : 00401000

	if( get_special_infos(player, card) == 66 ){
		modify_pt_and_abilities(player, card, event, 0, 2, 0);
	}

    return 0;
}

int card_pentavite(int player, int card, event_t event ){
// original code as Tetravite : 00421B70;

	if( get_special_infos(player, card) > 0 ){
		if(	trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && 
			player == reason_for_trigger_controller 
		  ){
			int trig = 0;

			if( is_what(trigger_cause_controller, trigger_cause, TYPE_ENCHANTMENT) &&
				is_aura(trigger_cause_controller, trigger_cause, 1) 
			  ){
				card_instance_t *instance = get_card_instance(player, card);
				if( instance->targets[0].player == player && instance->targets[0].card == card ){
					trig = 1;
				}
			}

			if( trig > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= 2;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						kill_card(trigger_cause_controller, trigger_cause, KILL_SACRIFICE);
				}
			}
		}
	}

    return 0;
}

int card_giant_spider(int player, int card, event_t event ){
// original code : 004C7D10

	if( get_special_infos(player, card) == 66 ){
		modify_pt_and_abilities(player, card, event, -1, -2, 0);
	}

    return card_giant_spider_exe(player, card, event);
}

int card_time_walk(int player, int card, event_t event ){
// original code : 0041B710

	if( event == EVENT_RESOLVE_SPELL ){
		if( ! check_battlefield_for_id(1-player, CARD_ID_STRANGLEHOLD) ){
			return card_time_walk_exe(player, card, event);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	else{
		return card_time_walk_exe(player, card, event);
	}
	return 0;
}

int card_demonic_tutor(int player, int card, event_t event ){
// original code : 0041D7C0

	if( event == EVENT_RESOLVE_SPELL ){
		if( ! check_battlefield_for_id(1-player, CARD_ID_STRANGLEHOLD) ){
			return card_demonic_tutor_exe(player, card, event);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	else{
		return card_demonic_tutor_exe(player, card, event);
	}
	return 0;
}

int card_grey_ogre(int player, int card, event_t event ){
// original code : 00401000

	if( get_special_infos(player, card) == 66 ){
		modify_pt_and_abilities(player, card, event, 1, 1, 0);
	}

    return 0;
}

int card_solemn_simulacrum(int player, int card, event_t event){

//    card_instance_t *instance = get_card_instance(player, card);

    if( event == EVENT_RESOLVE_SPELL ){
        int legacy = create_legacy_effect(player, card, &solemn_simulacrum_legacy);
        card_instance_t *leg = get_card_instance(player, legacy);
        leg->targets[0].card = card;
        leg->targets[0].player = player;
		if(	check_for_cip_effects_removal(player, card) > 0 ){
			tutor_basic_land(player, 1, 1);
		}
    }

    return 0;
}

int card_raise_the_alarm(int player, int card, event_t event){
// original code : 0x4DED1D

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	
	else if( event == EVENT_RESOLVE_SPELL ){
			generate_tokens_by_id(player, CARD_ID_SOLDIER, 2);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_battering_ram(int player, int card, event_t event){
// original code : 0x422CC0

	if( event == EVENT_ABILITIES && affect_me(player, card) ){
		if( current_turn == player && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 ){
			event_result |= KEYWORD_BANDING;
		}
	}

	return card_battering_ram_exe(player, card, event);
}

int card_voice_of_all(int player, int card, event_t event){
// original code :  

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[8].player != -1 ){
		modify_pt_and_abilities(player, card, event, 0, 0, instance->targets[8].player);
	}

	if( comes_into_play(player, card, event) ){
		instance->targets[8].player = select_a_protection(player);
	}

	return 0;
}

int card_larceny(int player, int card, event_t event){
// original code : 0x40DC50
    card_instance_t *instance = get_card_instance(player, card);

    if( current_turn == player && event == EVENT_DEAL_DAMAGE &&
		( current_phase == PHASE_FIRST_STRIKE_DAMAGE || current_phase == PHASE_NORMAL_COMBAT_DAMAGE ) 
	  ){
        card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
        if( damage->internal_card_id == damage_card ){
            if( damage->damage_source_player == player && damage->damage_target_card == -1 ){
				if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) ){
					card_instance_t *target = get_card_instance(damage->damage_source_player, damage->damage_source_card);
					if( damage->info_slot > 0 || target->targets[16].player > 0 ){
						instance->info_slot++;
					}
				}
            }
        }
    }
    
    if(	trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && 
		reason_for_trigger_controller == player && instance->info_slot > 0 ){
        if(event == EVENT_TRIGGER){
            event_result |= 2;
        }
        else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0; i<instance->info_slot; i++){
					discard(1-player, 0, 0);
				}
				instance->info_slot = 0;
        }
    }

    return global_enchantment(player, card, event);
}

int card_noble_purpose(int player, int card, event_t event){
// original code : 0x403D20

	if (event == EVENT_DEAL_DAMAGE){
		card_instance_t* source = get_card_instance(affected_card_controller, affected_card);

		if (source->internal_card_id == damage_card && source->info_slot > 0
			&& source->damage_source_player == player && source->damage_source_card >= 0
			&& (source->token_status & (STATUS_FIRST_STRIKE_DAMAGE | STATUS_COMBAT_DAMAGE))
			&& is_what(source->damage_source_player, source->damage_source_card, TYPE_CREATURE)){
			gain_life(player, source->info_slot);
		}
	}

    return global_enchantment(player, card, event);
}

int card_ashnods_trasmogrant(int player, int card, event_t event){
// original code : 0x044E240

    card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_ashnods_trasmogrant_exe(player, card, event);
		card_instance_t *target = get_card_instance( instance->targets[0].player, instance->targets[0].card );
		target->counters--;
		target->counter_power--;
		target->counter_toughness--;
		add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
	}
	else{
		return card_ashnods_trasmogrant_exe(player, card, event);
	}
	return 0;
}

int card_powerleech(int player, int card, event_t event){
// original code : 0x4327D0

//    card_instance_t *instance = get_card_instance( player, card );
	
	if( event == EVENT_PLAY_ABILITY && affected_card_controller == 1-player &&
		is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) && 
		! is_tapped(affected_card_controller, affected_card)
	  ){
		gain_life(player, 1);
	}
	if( event == EVENT_TAP_CARD && affected_card_controller == 1-player &&
		is_what(affected_card_controller, affected_card, TYPE_ARTIFACT)
	  ){
		gain_life(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_phyrexian_gremlins(int player, int card, event_t event){
// original code : 0x4549D0

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

    card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_ARTIFACT");
    }

    else if(event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_ARTIFACT");
    }

    else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				does_not_untap_until_im_tapped(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			}
    }

    else{
		return card_tawnoss_weaponry(player, card, event);
    }

	return 0;
}

int card_karakas(int player, int card, event_t event){
// original code : 0x40D220

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_LEGEND;

    card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

    if( event == EVENT_ACTIVATE ){
        int choice = 0;
        instance->info_slot = 0;
        if( ! paying_mana() && ! get_forbid_activation_flag(player, card) &&  can_target(&td) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
		  ){
            choice = do_dialog(player, player, card, -1, -1, " Produce W\n Bounce a Legend\n Do nothing", 1);
        }
        if( choice == 0 ){
			return mana_producer(player, card, event);
        }
        if( choice == 1 ){
            if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
				if( is_legendary(instance->targets[0].player, instance->targets[0].card) ){
					tap_card(player, card);
					instance->info_slot = 1;
				}
				else{
					spell_fizzled = 1;
				}
			}
        }
        else{
            spell_fizzled = 1;
        }
    }
    else if( event == EVENT_RESOLVE_ACTIVATION ){
             if( instance->info_slot == 1 ){
				card_instance_t *parent = get_card_instance(player, instance->parent_card);
				if( valid_target(&td)  ){
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				}
				parent->info_slot = 0;
			}
    }

    else{
		return mana_producer(player, card, event);
    }

	return 0;
}

int card_martyrs_of_korlis(int player, int card, event_t event ){
// original code : 0x453EB0

	if( event == EVENT_PREVENT_DAMAGE && ! is_tapped(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player &&  
                damage->info_slot > 0 
              ){
				if( in_play(damage->damage_source_player, damage->damage_source_card) &&
					is_what(damage->damage_source_player, damage->damage_source_card, TYPE_ARTIFACT)
				  ){
					damage->damage_target_card = card;
				}
            }
        }
    }
	
	return 0;
}

int card_willow_satyr(int player, int card, event_t event){
// original code : 

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.required_subtype = SUBTYPE_LEGEND;

    card_instance_t *instance = get_card_instance(player, card);

	choose_to_untap(player, card, event);

    if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && in_play(instance->parent_controller, instance->parent_card) && is_tapped(instance->parent_controller, instance->parent_card)){
			gain_control_until_source_is_in_play_and_tapped(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_ageless_sentinels(int player, int card, event_t event){
        
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[5].player != 66 ){
		cannot_attack(player, card, event);
    }

    if(current_turn == 1-player && instance->targets[5].player != 66 && 
		instance->blocking < 255
	  ){
		instance->targets[5].player = 66;
    }
    return 0;    
}


int card_consume_spirit(int player, int card, event_t event){
	return generic_x_spell(player, card, event, TARGET_ZONE_CREATURE_OR_PLAYER, COLOR_BLACK, 9);	
}

int card_drain_life(int player, int card, event_t event){
// Original code : 0x41E9B0

	return generic_x_spell(player, card, event, TARGET_ZONE_CREATURE_OR_PLAYER, COLOR_BLACK, 17);	
}

int card_blaze(int player, int card, event_t event){
// Original code : 0x4049B0

	return generic_x_spell(player, card, event, TARGET_ZONE_CREATURE_OR_PLAYER, 0, 1);	
}

int card_stream_of_life(int player, int card, event_t event){
// Original code : 

	return generic_x_spell(player, card, event, 0, 0, 2);	
}

int card_braingeyser(int player, int card, event_t event){
// Original code : 

	return generic_x_spell(player, card, event, TARGET_ZONE_PLAYERS, 0, 64);	
}

int card_mind_twist(int player, int card, event_t event){
// Original code : 

	return generic_x_spell(player, card, event, TARGET_ZONE_PLAYERS, 0, 512);	
}

int card_howl_from_beyond(int player, int card, event_t event){
// Original code : 

	return generic_x_spell(player, card, event, TARGET_ZONE_IN_PLAY, 0, 2048);	
}

int generic_damage_all_with_x_spell(int player, int card, event_t event, test_definition_t *this_test){

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
        
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

    else if(event == EVENT_RESOLVE_SPELL ){
			new_damage_all(player, card, 2, instance->info_slot, NDA_PLAYER_TOO, this_test);
			kill_card(player, card, KILL_DESTROY);
    }
	return 0;	
}

int card_earthquake(int player, int card, event_t event){
// Original code : 0x41C7E0
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.keyword = KEYWORD_FLYING;
	this_test.keyword_flag = 1;
	return generic_damage_all_with_x_spell(player, card, event, &this_test);	
}

int card_hurricane(int player, int card, event_t event){
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.keyword = KEYWORD_FLYING;
	return generic_damage_all_with_x_spell(player, card, event, &this_test);	
}

int generic_x_spell_recoded(int player, int card, event_t event, int result){

	if( event == EVENT_CAN_CAST ){
		if( result > 0 && ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG) ){
			return result; 
		}
	}
	else{
		return result;
	}
	return 0;		
}

int card_guardian_angel(int player, int card, event_t event){
	return generic_x_spell_recoded(player, card, event, card_guardian_angel_exe(player, card, event));
}

int card_disintegrate(int player, int card, event_t event){
	return generic_x_spell_recoded(player, card, event, card_disintegrate_exe(player, card, event));
}

int card_fireball(int player, int card, event_t event){
	return generic_x_spell_recoded(player, card, event, card_fireball_exe(player, card, event));
}

int card_spell_blast(int player, int card, event_t event){
	return generic_x_spell_recoded(player, card, event, card_spell_blast_exe(player, card, event));
}

int card_power_sink(int player, int card, event_t event){
	if (event == EVENT_RESOLVE_SPELL){
		ldoubleclicked = 0;
	}
	return generic_x_spell_recoded(player, card, event, card_power_sink_exe(player, card, event));
}

int card_detonate(int player, int card, event_t event){
	return generic_x_spell_recoded(player, card, event, card_detonate_exe(player, card, event));
}

int card_word_of_binding(int player, int card, event_t event){
	return generic_x_spell_recoded(player, card, event, card_word_of_binding_exe(player, card, event));
}

int card_winter_blast(int player, int card, event_t event){
	return generic_x_spell_recoded(player, card, event, card_winter_blast_exe(player, card, event));
}

int card_whimsy(int player, int card, event_t event){
	return generic_x_spell_recoded(player, card, event, card_whimsy_exe(player, card, event));
}

int card_alabaster_potion(int player, int card, event_t event){

    target_definition_t td1;
    default_target_definition(player, card, &td1, TYPE_CREATURE);
    td1.extra = damage_card;
    td1.required_type = 0;

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG) ){
		if( can_target(&td1) ){
			return card_guardian_angel_exe(player, card, event);
		}
		else{
			return 1;
		}
	}
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			if( can_target(&td1) ){
				choice = do_dialog(player, player, card, -1, -1, " Gain X life\n Prevent X damage\n Cancel", 1);
			}

			if( choice == 0 ){
				instance->targets[1].player = x_value;
				instance->info_slot = 66;
			}
			else if( choice == 1 ){
					if( pick_target(&td1, "TARGET_DAMAGE") ){
						instance->targets[1].player = x_value;
						instance->info_slot = 67;
					}
			}
			else{
				spell_fizzled = 1;
			}
    }

	else if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot == 66 ){
				gain_life(player, instance->targets[1].player);
			}
			if( instance->info_slot == 67 ){
				if( valid_target(&td1) ){
					card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
					if( target->info_slot <= instance->targets[1].player ){
						target->info_slot = 0;
					}
					else{
						target->info_slot-=instance->targets[1].player;
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

    return 0;
}

void add_oc_counters(int player, int card, int howmany){

    card_instance_t *instance = get_card_instance(player, card);

	if( howmany < 1 ){
		return ;
	}

    int i;
    for( i = 0; i < howmany; i++){
           instance->counters++;
           instance->counter_toughness--;
    }
	play_sound_effect(WAV_COUNTER);
}

int add_my_legacy(int player, int card, int t_player, int t_card){
	int legacy = create_targetted_legacy_effect(player, card, &empty, t_player, t_card);
	card_instance_t *instance = get_card_instance(player, legacy);
	instance->targets[2].card = get_id(player, card);
	return legacy;
}


int card_orcish_catapult(int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	
    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
        
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
		instance->targets[0].player = 0;
		int trgs[99];
		int n_target = 0;
		int i;
		for(i=0; i<2; i++){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && 
						! is_protected_from_me(player, card, i, count)
					  ){
						trgs[n_target] = (8192 * i) + count;
						n_target++;
					}
					count++;
			}
		}
		int r_targets = internal_rand(n_target);
		int count = 0;
		while( count < r_targets){
				int rnd = internal_rand(n_target);
				if( trgs[rnd] >= 8192 ){
					instance->targets[i+1].player = AI;
					instance->targets[i+1].card = trgs[rnd]-8192;
				}
				else{
					instance->targets[i+1].player = HUMAN;
					instance->targets[i+1].card = trgs[rnd];
				}
				add_my_legacy(player, card, instance->targets[i+1].player, instance->targets[i+1].card);
				int k;
				for(k=rnd; k<n_target; k++){
					trgs[k] = trgs[k+1];
				}
				n_target--;
				count++;
		}
		instance->targets[0].player = r_targets;
	}

    else if(event == EVENT_RESOLVE_SPELL ){
			int i;
			int amount = instance->info_slot;
			for(i=0; i<instance->targets[0].player; i++){
				if( validate_target(player, card, &td, i+1) ){
					int cnters = 1;
					if( amount > 1 ){
						cnters = internal_rand(amount);
					}
					int cters = get_updated_counters_number(instance->targets[i+1].player, cnters);
					add_oc_counters(instance->targets[i+1].player, instance->targets[i+1].card, cters);
					amount-=cnters;
				}
				if( amount < 1 ){
					break;
				}
			}
			kill_my_legacy(player, card);
			kill_card(player, card, KILL_DESTROY);
    }
	return 0;	
}

int card_volcanic_eruption(int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_LAND);
	
    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && has_mana_multi(player, 1, 0, 3, 0, 0, 0) ){
		if( player == AI ){
			if( check_battlefield_for_targettable_subtype(player, card, 1-player, TYPE_LAND, SUBTYPE_MOUNTAIN) &&
				has_mana(player, COLOR_COLORLESS, 1)
			  ){
				return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
			}
		}
		else{
			if( check_battlefield_for_targettable_subtype(player, card, 2, TYPE_LAND, SUBTYPE_MOUNTAIN) ){
				return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
			}
		}
	}
        
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int count = count_subtype(2, TYPE_LAND, SUBTYPE_MOUNTAIN);
		if( player != AI ){
			count = count_subtype(1-player, TYPE_LAND, SUBTYPE_MOUNTAIN);
		}
		int amount = 0;
		if( player != AI ){
			while( count > 0 && can_target(&td) ){
					if( amount > 0 ){
						td.allow_cancel = 0;
					}
					pick_target(&td, "TARGET_LAND");
					if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_MOUNTAIN) ){
						instance->targets[amount+1] = instance->targets[0];
						amount++;
						state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
						count--;
						if( count > 0 && can_target(&td) ){
							int choice = do_dialog(player, player, card, player, card, " Continue\n Stop", 0);
							if( choice == 0 ){
								break;
							}
						}
					}
			}
			if( amount > 0 ){
				int i;
				for(i=0; i<amount; i++){
					state_untargettable(instance->targets[i+1].player, instance->targets[i+1].card, 0);
				}
			}

		}
		else{
			int c2 = 0;
			while( c2 < active_cards_count[1-player] && amount < count ){
					if( in_play(1-player, count) && is_what(1-player, count, TYPE_LAND) &&
						has_subtype(1-player, count, SUBTYPE_MOUNTAIN) && 
						! is_protected_from_me(player, card, 1-player, count)
					  ){
						add_my_legacy(player, card, 1-player, count);
						instance->targets[amount+1].player = 1-player;
						instance->targets[amount+1].card = count;
						amount++;
					}
					if( amount < count ){
						if( ! has_mana(player, COLOR_COLORLESS, amount+1) ){
							break;
						}
					}
					c2++;
			}
		}
		if( amount > 0 ){
			charge_mana(player, COLOR_COLORLESS, amount);
			if( spell_fizzled != 1 	){
				instance->info_slot = amount;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

    else if(event == EVENT_RESOLVE_SPELL ){
			int i;
			int dmg = 0;
			for(i=0; i<instance->info_slot; i++){
				if( validate_target(player, card, &td, i+1) ){
					kill_card(instance->targets[i+1].player, instance->targets[i+1].card, KILL_DESTROY);
					dmg++;
				}
			}
			if( dmg > 0 ){
				damage_all(player, card, player, dmg, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				damage_all(player, card, 1-player, dmg, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			kill_my_legacy(player, card);
			kill_card(player, card, KILL_DESTROY);
    }
	return 0;	
}

int card_crystal_rod(int player, int card, event_t event){
    return lifegaining_charm(player, card, event, 2, COLOR_TEST_BLUE, 1+player, 1);
}

int card_iron_star(int player, int card, event_t event){
    return lifegaining_charm(player, card, event, 2, COLOR_TEST_RED, 1+player, 1);
}

int card_ivory_cup(int player, int card, event_t event){
    return lifegaining_charm(player, card, event, 2, COLOR_TEST_WHITE, 1+player, 1);
}

int card_throne_of_bone(int player, int card, event_t event){
    return lifegaining_charm(player, card, event, 2, COLOR_TEST_BLACK, 1+player, 1);
}

int card_wooden_sphere(int player, int card, event_t event){
    return lifegaining_charm(player, card, event, 2, COLOR_TEST_GREEN, 1+player, 1);
}

int card_lone_missionary(int player, int card, event_t event){
// Also code for Teroh's Faithful
    return cip_lifegain(player, card, event, 4);
}

int card_angel_of_mercy(int player, int card, event_t event){

    return cip_lifegain(player, card, event, 3);
}


int card_diamond_valley(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, instance->targets[1].card);
	}
	return altar_basic(player, card, event, 4, TYPE_CREATURE);
}

int card_life_chisel(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, instance->targets[1].card);
	}
	return altar_basic(player, card, event, 24, TYPE_CREATURE);
}

int card_zuran_orb(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 2);
	}
	return altar_basic(player, card, event, 0, TYPE_LAND);
}


int card_dark_heart_of_the_woods(int player, int card, event_t event){

    if( event == EVENT_SHOULD_AI_PLAY ){
        return should_ai_play(player, card);
    } 

    else if( event == EVENT_CAN_CAST ){
			return 1;
    }

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			gain_life(player, 3);
	}
	else{
		return altar_extended(player, card, event, 0, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_farmstead(int player, int card, event_t event){

	if( current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		int trig = 0;
		if( has_mana(player, COLOR_WHITE, 3) ){
			trig = 1;
        }
		if( trig == 1 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					charge_mana(player, COLOR_WHITE, 3);
					if( spell_fizzled != 1 ){
						gain_life(player, 1);
					}
			}
		}
    }
	else{
		return card_farmstead_exe(player, card, event); 
    }

    return 0;
}

int card_lifeblood(int player, int card, event_t event){
// Also code for Sanctimony
	if( event == EVENT_TAP_CARD && affected_card_controller == 1-player ){
//		if( has_subtype(affected_card_controller, affected_card, get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN)) ){
		if( has_subtype(affected_card_controller, affected_card, SUBTYPE_MOUNTAIN) ){
			gain_life(player, 1);
		}
	}

    return global_enchantment(player, card, event);
}

int card_spiritual_sanctury(int player, int card, event_t event){

    if(	get_forbid_activation_flag(player, card) != 4 && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		int count = count_upkeeps(player);
        if(event == EVENT_TRIGGER && count > 0 && check_battlefield_for_subtype(current_turn, TYPE_LAND, SUBTYPE_PLAINS) ){
            event_result |= 2;
        }
        else if(event == EVENT_RESOLVE_TRIGGER){
				
				card_instance_t *instance= get_card_instance(player, card);
				card_data_t* card_d = &cards_data[ instance->internal_card_id ]; 
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				while( count > 0 ){
						ptFunction(player, card, EVENT_UPKEEP_TRIGGER_ABILITY );
						count--;
				}
        }
    }

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		gain_life(current_turn, 1);
	}

    return global_enchantment(player, card, event);
}

int card_chastise(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
        
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

    else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = get_power(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				gain_life(player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
    }
	return 0;	
}

int card_divine_offering(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_ARTIFACT);

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
        
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ARTIFACT");
	}

    else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = get_cmc(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				gain_life(player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
    }
	return 0;	
}

int card_healing_salve(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

    target_definition_t td1;
    default_target_definition(player, card, &td1, TYPE_CREATURE);
    td1.extra = damage_card;
    td1.required_type = 0;

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( can_target(&td1) ){
			return card_guardian_angel_exe(player, card, event);
		}
		else{
			return can_target(&td);
		}
	}
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			if( can_target(&td) ){
				if( can_target(&td1) ){
					choice = do_dialog(player, player, card, -1, -1, " Gain 3 life\n Prevent 3 damage\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				if( pick_target(&td, "TARGET_PLAYER") ){
					instance->info_slot = 66;
				}
			}
			else if( choice == 1 ){
					if( pick_target(&td1, "TARGET_DAMAGE") ){
						instance->info_slot = 67;
					}
			}
			else{
				spell_fizzled = 1;
			}
    }

	else if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				gain_life(instance->targets[0].player, 3);
			}
			if( instance->info_slot == 67 ){
				if( valid_target(&td1) ){
					card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
					if( target->info_slot <= 3 ){
						target->info_slot = 0;
					}
					else{
						target->info_slot-=3;
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

    return 0;
}

int card_punish_ignorance(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			instance->targets[1].player = card_on_stack_controller;
			return result;
		}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			gain_life(player, 3);
			lose_life(instance->targets[1].player, 3);
			return card_counterspell(player, card, event);
	}
	else{
		return card_counterspell(player, card, event);
	}

    return 0;
}

int card_reverse_damage(int player, int card, event_t event){

    target_definition_t td1;
    default_target_definition(player, card, &td1, TYPE_ANY);
    td1.extra = damage_card;
	td1.special = 32; // Only damage to players

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( (land_can_be_played & 4) && can_target(&td1) ){
			return 99;
		}
	}
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td1, "TARGET_DAMAGE") ){
				card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				if( target->damage_target_card != -1 ){
					spell_fizzled = 1;
				}
			}
    }

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				gain_life(player, target->info_slot);
				target->info_slot = 0;
			}
			kill_card(player, card, KILL_DESTROY);
	}

    return 0;
}

int card_kiss_of_the_amesha(int player, int card, event_t event){

    target_definition_t td1;
    default_target_definition(player, card, &td1, TYPE_CREATURE);
    td1.zone = TARGET_ZONE_PLAYERS;
	td1.preferred_controller = player;

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td1, "TARGET_PLAYER");
    }

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				gain_life(instance->targets[0].player, 7);
				draw_cards(instance->targets[0].player, 2);
			}
			kill_card(player, card, KILL_DESTROY);
	}

    return 0;
}

int card_peach_garden_oath(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;
//    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
        
    else if(event == EVENT_RESOLVE_SPELL ){
			int amount = target_available(player, card, &td) * 2;
			gain_life(player, amount);
			kill_card(player, card, KILL_DESTROY);
    }
	return 0;	
}

int card_sacred_nectar(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
        
    else if(event == EVENT_RESOLVE_SPELL ){
			gain_life(player, 4);
			kill_card(player, card, KILL_DESTROY);
    }
	return 0;	
}


int card_living_artifact(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = player;

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			ai_modifier+=100;
			pick_target(&td, "TARGET_ARTIFACT");
    }

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				instance->damage_target_player = instance->targets[0].player;
				instance->damage_target_card = instance->targets[0].card;
			}
			else{
				kill_card(player, card, KILL_SACRIFICE);
			}
	}

	if( in_play(player, card) && instance->targets[0].player != -1 ){
		damage_effects(player, card, event);
		
		if( current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
			reason_for_trigger_controller == player
		  ){
			int trig = 0;
			if( count_counters(player, card) > 0 ){
				trig = 1;
			}
			if( trig == 1 ){
				if(event == EVENT_TRIGGER){
					event_result |= 1+player;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						remove_counter(player, card);
						gain_life(player, 1);
				}
			}
		}
	}

    return 0;
}

int card_crumble(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_ARTIFACT);

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
        
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ARTIFACT");
	}

    else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = get_cmc(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
				gain_life(instance->targets[0].player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
    }
	return 0;	
}

int card_biorythm(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	if( event == EVENT_CAN_CAST ){
		if( player == AI ){
			return can_target(&td);
		}
		else{
			return 1;
		}
	}
        
    else if(event == EVENT_RESOLVE_SPELL ){
			set_life_total(player, count_permanents_by_type(player, card, TYPE_CREATURE));
			set_life_total(1-player, count_permanents_by_type(1-player, card, TYPE_CREATURE));
			kill_card(player, card, KILL_DESTROY);
    }
	return 0;	
}

int card_sol_kanar_the_swamp_king(int player, int card, event_t event){

	check_legend_rule(player, card, event);
	
	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, COLOR_TEST_BLACK, 0, 0, 0, -1, 0) ){
		gain_life(player, 1);
	}

	return 0;	
}

int card_merchant_ship(int player, int card, event_t event){

//	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) ){
		if( ! check_battlefield_for_subtype(1-player, TYPE_LAND, SUBTYPE_ISLAND) ){
			event_result = 1;
		}     
	}     

	if( current_turn == player && is_attacking(player, card) && event == EVENT_DECLARE_BLOCKERS &&
		affect_me(player, card) && is_unblocked(player, card)
	  ){
		gain_life(player, 2);
	}     
	
	if( ! check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_ISLAND) ){
		kill_card(player, card, KILL_SACRIFICE);
	}     

	return 0;
}

int card_lifetap(int player, int card, event_t event){

	if( event == EVENT_TAP_CARD && affected_card_controller == 1-player ){
		if( has_subtype(affected_card_controller, affected_card, get_hacked_subtype(player, card, SUBTYPE_FOREST)) ){
			gain_life(player, 1);
		}
	}

    return global_enchantment(player, card, event);
}

int card_relic_bind2(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = 1-player;
	td.allowed_controller = 1-player;

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			ai_modifier+=100;
			pick_target(&td, "TARGET_ARTIFACT");
    }

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				instance->damage_target_player = instance->targets[0].player;
				instance->damage_target_card = instance->targets[0].card;
			}
			else{
				kill_card(player, card, KILL_SACRIFICE);
			}
	}

	if( in_play(player, card) && instance->targets[0].player != -1 ){
		if( event == EVENT_TAP_CARD && affect_me(instance->targets[0].player, instance->targets[0].card) ){
			int choice = do_dialog(player, player, card, player, card, " Deal 1 damge\n Gain 1 life", 0);
			if( choice == 0 ){
				damage_player(instance->targets[0].player, 1, player, card);
			}
			if( choice == 1 ){
				gain_life(player, 1);
			}
		}
	}

    return 0;
}

void exchange_life_totals(int player, int t_player){
	if( player == t_player ){
		return;
	}
	int my_life = life[player];
	if( check_battlefield_for_id(player, CARD_ID_LICH) ){
		my_life = 0;
	}
	int his_life = life[t_player];
	if( check_battlefield_for_id(t_player, CARD_ID_LICH) ){
		his_life = 0;
	}
	set_life_total(player, his_life);
	set_life_total(t_player, my_life);
}

int card_mirror_universe(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

    if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td)  ){
			exchange_life_totals(player, 1-player);
		}
    }

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_ONLY_TARGET_OPPONENT+GAA_SACRIFICE_ME+GAA_ONLY_ON_UPKEEP+GAA_IN_YOUR_TURN, 
									0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_magus_of_the_mirror(int player, int card, event_t event){
	return card_mirror_universe(player, card, event);
}

int card_vicious_hunger(int player, int card, event_t event){
// Also code for Douse in Gloom and Sorin't Thirst

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
    }

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
				gain_life(player, 2);
			}
			kill_card(player, card, KILL_DESTROY);
	}

    return 0;
}

int card_simulacrum(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( player == AI ){
			if( get_trap_condition(player, TRAP_DAMAGE_TAKEN) > 0 ){
				return can_target(&td);
			}
		}
		else{
			return can_target(&td);
		}
	}
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
    }

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = get_trap_condition(player, TRAP_DAMAGE_TAKEN);
				damage_creature(instance->targets[0].player, instance->targets[0].card, amount, player, card);
				gain_life(player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
	}

    return 0;
}

int card_sever_soul(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
    else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
    }

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = get_toughness(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
				gain_life(player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
	}

    return 0;
}

int card_syphon_soul(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			int amount = lose_life(1-player, 2);
			gain_life(player, amount);
			kill_card(player, card, KILL_DESTROY);
	}

    return 0;
}

int card_fountain_of_youth(int player, int card, event_t event){

    if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 1);
    }

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_ivory_tower(int player, int card, event_t event){

    if(	get_forbid_activation_flag(player, card) != 4 && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
		current_turn == player
	  ){
		int count = count_upkeeps(player);
        if(event == EVENT_TRIGGER && count > 0 && hand_count[player] > 4 ){
            event_result |= 2;
        }
        else if(event == EVENT_RESOLVE_TRIGGER){
				card_instance_t *instance= get_card_instance(player, card);
				card_data_t* card_d = &cards_data[ instance->internal_card_id ]; 
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				while( count > 0 ){
						ptFunction(player, card, EVENT_UPKEEP_TRIGGER_ABILITY );
						count--;
				}
        }
    }

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int amount = hand_count[player]-4;
		gain_life(player, amount);
	}

    return 0;
}

int card_onulet(int player, int card, event_t event){

	if( graveyard_from_play(player, card, event) ){
		gain_life(player, 2);
	}

    return 0;
}

int card_soul_net(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);

	if( has_mana(player, COLOR_COLORLESS, 1) && 
		count_graveyard_from_play(player, card, event, 10, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		int i;
		for(i=0; i<instance->targets[11].card; i++){
			if( ! has_mana(player, COLOR_COLORLESS, 1) ){
				break;
			}
			charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, COLOR_COLORLESS, 1);
			if( spell_fizzled == 1 ){
				break;
			} else {
				gain_life(player, 1);
			}
		}
		instance->targets[11].card = 0;
	}

    return 0;
}

int card_tablet_of_epityr(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);

	if( has_mana(player, COLOR_COLORLESS, 1) && 
		count_graveyard_from_play(player, card, event, player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		int i;
		for(i=0; i<instance->targets[11].card; i++){
			if( ! has_mana(player, COLOR_COLORLESS, 1) ){
				break;
			}
			charge_mana(player, COLOR_COLORLESS, 1);
			if( spell_fizzled != 1 ){
				gain_life(player, 1);
			}
		}
		instance->targets[11].card = 0;
	}

    return 0;
}

int card_urzas_chalice(int player, int card, event_t event){

	if( has_mana(player, COLOR_COLORLESS, 1) && 
		specific_spell_played(player, card, event, 2, 1+player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			gain_life(player, 1);
		}
	}

    return 0;
}


int card_enlightened_tutor(int player, int card, event_t event){
        
	if( event == EVENT_CAN_CAST ){
		return 1;
    }
	
    else if(event == EVENT_RESOLVE_SPELL ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, 1, TYPE_ARTIFACT | TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			kill_card(player, card, KILL_DESTROY);
    }
	
    return 0;    
}

int card_moroii(int player, int card, event_t event){
        
	upkeep_trigger_ability(player, card, event, player);	

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(player, 1);
	}

    return 0;    
}

int card_nevinyrrals_disk(int player, int card, event_t event){
// 0x425A40
	comes_into_play_tapped(player, card, event);
	
	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_NONLAND);
		this_test.type_flag = F1_NO_PWALKER;
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
	}
	
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_goblin_shrine(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);
	
	if( in_play(player, card) && instance->targets[0].player != -1 ){
		if( get_id(instance->targets[0].player, instance->targets[0].card) == CARD_ID_MOUNTAIN ){
			boost_creature_type(player, card, event, SUBTYPE_GOBLIN, 1, 0, 0, BCT_INCLUDE_SELF);
		}
		if( leaves_play(player, card, event) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.subtype = SUBTYPE_GOBLIN;
			new_damage_all(player, card, 2, 1, 0, &this_test);
		}
	}
	else{
		return card_goblin_shrine_exe(player, card, event);
	}
	return 0;		
}

int card_goblin_caves(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);
	
	if( in_play(player, card) && instance->targets[0].player != -1 ){
		if( get_id(instance->targets[0].player, instance->targets[0].card) == CARD_ID_MOUNTAIN ){
			boost_creature_type(player, card, event, SUBTYPE_GOBLIN, 0, 2, 0, BCT_INCLUDE_SELF);
		}
	}
	else{
		return card_goblin_shrine_exe(player, card, event);
	}
	return 0;		
}

extern int hack_animate_dead_player, hack_animate_dead_card;
int card_animate_dead(int player, int card, event_t event){
// 0x4B5780
	// See also card_dance_of_the_dead() in ice_age.c and card_necromancy() in visions.c

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		modify_pt_and_abilities(instance->damage_target_player, instance->damage_target_card, event, -1, 0, 0);

		if( leaves_play(player, card, event) ){
			kill_card(instance->damage_target_player, instance->damage_target_card, KILL_SACRIFICE);
		}
		
		if( leaves_play(instance->damage_target_player, instance->damage_target_card, event) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	if( event == EVENT_CAN_CAST && (count_graveyard_by_type(player, TYPE_CREATURE) > 0 || count_graveyard_by_type(1-player, TYPE_CREATURE)) ){
		return ! graveyard_has_shroud(2);
	}
	
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = 1-player;
		if( count_graveyard_by_type(1-player, TYPE_CREATURE) > 0 ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
				pick_target(&td, "TARGET_PLAYER");
			}
		}
		else{
			instance->targets[0].player = player;
		}
		if( new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_CMC, &this_test, 1) == -1 ){
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_SPELL){
		/* real_put_into_play removes STATE_INVISIBLE just after sending EVENT_RESOLVE_SPELL instead of just before; remove it now, so in_play() works right in
		   any come-into-play effects.  (This might be a more reliable way of determining whether a card was played from hand.) */
		instance->state &= ~STATE_INVISIBLE;

		int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			hack_animate_dead_player = player;
			hack_animate_dead_card = card;
			reanimate_permanent(player, card, instance->targets[0].player, selected, 9, 0);
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_whirlwind_derwish(int player, int card, event_t event){
// whirling dervish
	card_instance_t *instance = get_card_instance( player, card );

    if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT) ){
		instance->targets[1].player = 66;
    }
	if( instance->targets[1].player == 66 && eot_trigger(player, card, event) ){
		add_1_1_counter(player, card);
		instance->targets[1].player = 0;
    }

    return 0;
}

int card_raise_dead(int player, int card, event_t event){

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

    if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
        return ! graveyard_has_shroud(2);
    }

    if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1 ){
			spell_fizzled = 1;
		}
    }

    if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
    }

    return 0;
}

int card_clockwork_beast(int player, int card, event_t event){
	return hom_clockwork(player, card, event, 7);
}

int card_drop_of_honey(int player, int card, event_t event){
// 0x430FA0

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
    td.illegal_abilities = 0;
    td.allow_cancel = 0;

    card_instance_t *instance = get_card_instance(player, card);
 
	upkeep_trigger_ability(player, card, event, player);	

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( can_target(&td) ){
			int i;
			int t_player = -1;
			int t_card = -1;
			int doubles_count = 1;
			int min_t = 100;
			int count;
			for(i=0; i<2; i++){
				count = active_cards_count[i]-1;
				while( count > -1 ){
					   if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
						   if( get_power(i, count) < min_t ){
							   t_player = i;
							   t_card = count;
							   min_t = get_power(i, count);
							   doubles_count = 1;
							   instance->targets[doubles_count].player = t_player;
							   instance->targets[doubles_count].card = t_card;
						   }
						   else if( get_power(i, count) == min_t ){
									doubles_count++;
									instance->targets[doubles_count].player = i;
									instance->targets[doubles_count].card = count;
						   }


					   }
					   count--;
				}
			}

			if( doubles_count > 1 ){
				i=0;
				for(i=0; i<2; i++){
					count = 0;
					while( count < active_cards_count[i] ){
							if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
								card_instance_t *this = get_card_instance(i, count);
								this->state |= STATE_CANNOT_TARGET;
							}     
							count++;
					}     
				}     
				int k;
				for(k=0; k<doubles_count; k++){
					card_instance_t *this = get_card_instance(instance->targets[k+1].player, instance->targets[k+1].card);
					this->state  &= ~STATE_CANNOT_TARGET;
				}
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					t_player = instance->targets[0].player;
					t_card = instance->targets[0].card;
					for(i=0; i<2; i++){
						count = 0;
						while( count < active_cards_count[i] ){
								if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
									card_instance_t *this = get_card_instance(i, count);
									if( this->state & STATE_CANNOT_TARGET ){
										this->state  &= ~STATE_CANNOT_TARGET;
									}     
								}     
								count++;
						}     
					}     
				}
			}

			if( t_player != -1 && t_card != -1 ){
				kill_card(t_player, t_card, KILL_BURY);
			}
		}
	}

	if( ! can_target(&td) && in_play(player, card) ){
		kill_card(player, card, KILL_SACRIFICE);
    }

    return global_enchantment(player, card, event);
}

int card_tranquillity(int player, int card, event_t event){

    if( event == EVENT_CAN_CAST ){
		return 1;
    }

    if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		if( ! check_battlefield_for_id(2, CARD_ID_ENCHANTED_EVENING) ){
			default_test_definition(&this_test, TYPE_ENCHANTMENT);
		}
		else{
			default_test_definition(&this_test, TYPE_PERMANENT);
		}
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
    }

    return 0;
}

int card_desert2(int player, int card, event_t event){
// original code : 0x4A8970

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = current_turn;
	td1.preferred_controller = current_turn;
	td1.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

    if( trigger_condition == TRIGGER_END_COMBAT && affect_me(player, card) && reason_for_trigger_controller == player ){
		int trig = 0;
		int count = 0;
		while( count < active_cards_count[current_turn] ){
				if( in_play(current_turn, count) && is_what(current_turn, count, TYPE_CREATURE) && is_attacking(current_turn, count) ){
					instance->targets[0].player = current_turn;
					instance->targets[0].card = count;
					instance->number_of_targets = 1;
					if( valid_target(&td1) ){
						if( player == HUMAN ){
							trig = 1;
							break;
						}
						else{
							if( get_toughness(current_turn, count) < 2 ){
								trig = 1;
								break;
							}
						}
					}
				}
				count++;
        }
		if( ! is_tapped(player, card) && trig == 1 && (( player == AI && current_turn != player) || player != AI) ){
			if( event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					int choice = do_dialog(player, player, card, -1, -1, " Activate Desert\n Pass", 0);
					if( choice == 0 && pick_target(&td1, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						tap_card(player, card);
						damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
					}
			}
		}
	}
	
    return mana_producer(player, card, event);
}

int card_assembly_worker(int player, int card, event_t event){
	if( get_special_infos(player, card) != 66 ){
		return card_assembly_worker_exe(player, card, event);
	}
	return 0;
}

int card_fellwar_stone(int player, int card, event_t event){
// original code : 0x422360.  Also Exotic Orchard.

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CHANGE_TYPE && affect_me(player, card)){
		instance->mana_color = instance->card_color = get_color_of_mana_produced_by_id(get_id(player, card), COLOR_TEST_ANY_COLORED, player);
	}

	if (instance->mana_color == 0){	// Can't produce mana, but can still tap for 0
		if (event == EVENT_CAN_ACTIVATE && player == AI){
			return 0;
		}
		if (event == EVENT_ACTIVATE){
			if (can_produce_mana(player, card)){
				produce_mana_tapped(player, card, COLOR_COLORLESS, 0);
				return 0;
			}
		}
		if (event == EVENT_COUNT_MANA){
			return 0;
		}
	}

	return mana_producer(player, card, event);
}

int is_monolith(int id){
	if( id == CARD_ID_MANA_VAULT ||
		id == CARD_ID_GRIM_MONOLITH ||
		id == CARD_ID_BASALT_MONOLITH
	  ){
		return 1;
	}
	return 0;
}

int card_power_artifact(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->targets[0].player != -1 ){
		if( leaves_play(player, card, event) ){
			remove_cost_mod_for_activated_abilities(instance->targets[0].player, instance->targets[0].card, 2, 0);
		}
	}
	if( event == EVENT_CAN_CAST ){
		if( player != AI ){
			return can_target(&td);
		}
		else{
			int count = 0;
			while( count < active_cards_count[player] ){
					if( in_play(player, count) && is_monolith(get_id(player, count)) ){
						instance->targets[0].player = player;
						instance->targets[0].card = count;
						instance->number_of_targets = 1;
						if( valid_target(&td) ){
							return 1;
						}
					}
					count++;
			}
		}
	}
	
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player != AI ){
			pick_target(&td, "TARGET_ARTIFACT");
		}
		else{
			instance->targets[0].player = -1;
			instance->targets[0].card = -1;
			instance->number_of_targets = 0;
			int count = 0;
			while( count < active_cards_count[player] ){
					if( in_play(player, count) && is_monolith(get_id(player, count)) ){
						instance->targets[0].player = player;
						instance->targets[0].card = count;
						instance->number_of_targets = 1;
						if( valid_target(&td) ){
							ai_modifier+=100;
							break;
						}
					}
					count++;
			}
			if( instance->targets[0].player == -1 ){
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			instance->damage_target_player = instance->targets[0].player;
			instance->damage_target_card = instance->targets[0].card;
			set_cost_mod_for_activated_abilities(instance->targets[0].player, instance->targets[0].card, 2, 0);
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	

	return 0;
}

int card_black_lotus(int player, int card, event_t event){
	//0x420020
	return artifact_mana_all_one_color(player, card, event, 3, 1);
}

int card_sol_ring(int player, int card, event_t event){
	//0x424b30
	if (event == EVENT_CAST_SPELL){
		if (affect_me(player, card)){
			ai_modifier += 192 / (landsofcolor_controlled[1][COLOR_ANY] + 1);
		}
	} else if (event == EVENT_CAN_ACTIVATE){
		return !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card);
	} else if (event == EVENT_ACTIVATE){
		ai_modifier -= 12;
		produce_mana_tapped(player, card, COLOR_COLORLESS, 2);
    } else if (event == EVENT_COUNT_MANA){
		if (affect_me(player, card) && !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card)){
			declare_mana_available(player, COLOR_COLORLESS, 2);
		}
	}
	return 0;
}

int card_mana_flare(int player, int card, event_t event){
	//0x4B8940; also code for Heartbeat of Spring, Zhur-Taa Ancient

	switch (event){
		case EVENT_TAP_CARD:
			if (!is_what(affected_card_controller, affected_card, TYPE_LAND)){
				return 0;
			}

			if (!in_play(player, card)){	// Seems unnecessary, but it's explicitly checked in the exe version, and is harmless
				return 0;
			}

			produce_mana_of_any_type_tapped_for(player, card, 1);

			return 0;

		case EVENT_COUNT_MANA:
			if (!is_what(affected_card_controller, affected_card, TYPE_LAND)){
				return 0;
			}

			if (!in_play(player, card)){	// Seems unnecessary, but it's explicitly checked in the exe version, and is harmless
				return 0;
			}

			// A check for EA_MANA_SOURCE is conspicuous by its absence in the exe version.
			if (is_tapped(affected_card_controller, affected_card) || is_animated_and_sick(affected_card_controller, affected_card)
				|| !can_produce_mana(affected_card_controller, affected_card) ){
				return 0;
			}

			int num_colors = 0;
			color_t col;
			card_instance_t* aff_instance = get_card_instance(affected_card_controller, affected_card);
			for (col = COLOR_COLORLESS; col <= COLOR_ARTIFACT; ++col){
				if (aff_instance->card_color & (1 << col)){
					++num_colors;
				}
			}

			if (num_colors > 0){
				declare_mana_available_hex(affected_card_controller, aff_instance->card_color, 1);
			} else {
				declare_mana_available(affected_card_controller, single_color_test_bit_to_color(aff_instance->card_color), 1);
			}

			return 0;

		case EVENT_CAN_CAST:
			return 1;

		case EVENT_CAST_SPELL:
			if (affect_me(player, card)){
				ai_modifier += 48;
			}
			return 0;

		default:
			return 0;
	}
}

int card_font_of_mythos(int player, int card, event_t event)
{
  if (event == EVENT_DRAW_PHASE )
    event_result+=2;
  return 0;
}

int card_dragon_engine(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_city_of_brass(int player, int card, event_t event){
	if (event == EVENT_TAP_CARD && affect_me(player, card)){
		damage_player(player, 1, player, card);
		return 0;
	} else {
		return mana_producer(player, card, event);
	}
}

int card_generic_noncombat_1_mana_producing_creature(int player, int card, event_t event){
	// Birds of Paradise, Copper Myr, Gold Myr, Iron Myr, Leaden Myr, Silver Myr
	if (event == EVENT_CAN_CAST){
		return 1;	// so it can be assigned to a creature with flash
	}

	int colors = get_card_instance(player, card)->mana_color & COLOR_TEST_ANY;
	if (!colors){
		return 0;
	} else if ((colors & (colors - 1)) == 0){	// exactly one bit set
		return mana_producing_creature(player, card, event, 24, single_color_test_bit_to_color(colors), 1);
	} else {
		return mana_producing_creature_all_one_color(player, card, event, 24, colors, 1);
	}
}

// Just very thin wrappers for now
int card_bazaar_of_baghdad(int player, int card, event_t event){
	if (event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
	}
	return ((int (*)(int, int, event_t))(0x4A8520))(player, card, event);
}

int card_elephant_graveyard(int player, int card, event_t event){
	if (event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
	}
	return ((int (*)(int, int, event_t))(0x4A8D40))(player, card, event);
}

int card_island_of_wak_wak(int player, int card, event_t event){
	if (event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
	}
	return ((int (*)(int, int, event_t))(0x4A8F70))(player, card, event);
}

int card_oasis(int player, int card, event_t event){
	if (event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
	}
	return ((int (*)(int, int, event_t))(0x47A010))(player, card, event);
}

static int ai_choice_for_arena(int player, int card, target_t* ret_tgt){
	card_instance_t* instance;
	int c, best_rating = -1;
	uint32_t prots = get_protections_from(player, card);

	ret_tgt->player = ret_tgt->card = -1;
	for (c = 0; c < active_cards_count[AI]; ++c){
		if (in_play(AI, c)
			&& cards_data[(instance = get_card_instance(AI, c))->internal_card_id].type & TYPE_CREATURE
			&& instance->kill_code != KILL_SACRIFICE	// How would it even get here?
			&& !(instance->state & STATE_CANNOT_TARGET)){
			/* Straight out of the exe.  There's definitely room for improvement here.
			 * For starters, if the human player chose first, the chosen creature should be taken into account.  This will always pick a
			 *    vanilla 4/4 creature over either a 6/1 or a 1/6, even when fighting a known 5/5.
			 * Most abilities, as well as EA_MANA_SOURCE and EA_ACT_ABILITY, should make the AI less likely to pick a given creature.
			 * Protections are an exception.  Especially if the human player's picked and the candidate has protection from that creature.
			 * Base card value should also be looked at. */
			uint32_t abils = get_abilities(AI, c, EVENT_ABILITIES, -1);
			if (abils & prots){
				continue;
			}
			int rating = (get_abilities(AI, c, EVENT_POWER, -1)
						  + get_abilities(AI, c, EVENT_TOUGHNESS, -1)
						  + num_bits_set(abils)
						  + num_bits_set(abils & (KEYWORD_FLYING | KEYWORD_FIRST_STRIKE))
						  + num_bits_set(cards_data[instance->internal_card_id].extra_ability & (EA_MANA_SOURCE | EA_ACT_ABILITY)));

			if (best_rating < rating){
				best_rating = rating;
				ret_tgt->player = AI;
				ret_tgt->card = c;
			}
		}
	}

  return best_rating >= 0;
}

int card_arena(int player, int card, event_t event){
	if (event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI){
		ai_modifier += 48;
		return 0;
	}

	if (event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
		return 0;
	}

	if (event != EVENT_CAN_ACTIVATE && event != EVENT_ACTIVATE && event != EVENT_RESOLVE_ACTIVATION){
		return 0;
	}

	card_instance_t* instance = get_card_instance(player, card);

	target_definition_t td_me, td_opp;

	default_target_definition(player, card, &td_me, TYPE_CREATURE);
	td_me.allowed_controller = td_me.preferred_controller = player;

	default_target_definition(player, card, &td_opp, TYPE_CREATURE);
	td_opp.allowed_controller = td_opp.preferred_controller = td_opp.who_chooses = 1 - player;
	td_opp.allow_cancel = 0;

	if (event == EVENT_CAN_ACTIVATE){
		return (!is_tapped(player, card) && !is_animated_and_sick(player, card) && !paying_mana()
				&& !get_forbid_activation_flag(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(3))
				&& can_target(&td_me) && can_target(&td_opp));
	}

	if (event == EVENT_ACTIVATE){
		if (paying_mana()){
			cancel = 1;
			return 0;
		}

		charge_mana_for_activated_ability(player, card, MANACOST_X(3));

		if (cancel != 1){
			instance->number_of_targets = 0;

			if (player == HUMAN){
				if (!pick_target(&td_me, "ARENA")){
					cancel = 1;
					return 0;
				}

				instance->state |= STATE_TAPPED;
				if (trace_mode & 2){
					((int (*)(int, int))(0x4720E0))(player, 0);	// networking
					new_pick_target(&td_opp, "ARENA", 1, 0);
				} else {
					if (!ai_choice_for_arena(player, card, &instance->targets[1])){
						// shouldn't be possible currently, but in case validate_target()'s definition drifts
						new_pick_target(&td_opp, "ARENA", 1, 0);
					}
					instance->number_of_targets = 2;
				}
				do_dialog(player, player, card, instance->targets[1].player, instance->targets[1].card, text_lines[1] /*"Selected for Arena"*/, 0);
			} else {	// player == AI
				if (trace_mode & 2){
					cancel = pick_target(&td_me, "ARENA") ? 0 : 1;
				} else {
					if (ai_choice_for_arena(player, card, &instance->targets[0])){
						instance->number_of_targets = 1;
						cancel = 0;
					} else {
						// shouldn't be possible currently, but in case validate_target()'s definition drifts
						cancel = pick_target(&td_opp, "ARENA") ? 0 : 1;
					}
				}

				if (cancel != 1){
					instance->state |= STATE_TAPPED;

					do_dialog(player, player, card, instance->targets[0].player, instance->targets[0].card, text_lines[1] /*"Selected for Arena"*/, 0);
					new_pick_target(&td_opp, "ARENA", 1, 0);

					if (trace_mode & 2){
						((int (*)(int, int))(0x4720E0))(1 - player, 0);	// networking
					}
				}
			}

			if (cancel == 1){
				instance->number_of_targets = 0;
			}
		}

		return 0;
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		int target_0_valid = validate_target(player, card, &td_me, 0);
		int target_1_valid = validate_target(player, card, &td_opp, 1);

		if (target_0_valid && target_1_valid){
			fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
			if (in_play(instance->targets[0].player, instance->targets[0].card)){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			if (in_play(instance->targets[1].player, instance->targets[1].card)){
				tap_card(instance->targets[1].player, instance->targets[1].card);
			}
		}
		/* Otherwise, if one target one was valid, the exe version still tapped it and did damage to it.  This contradicts ruling 9/25/2006: "If either target
		 * is illegal at the time the ability resolves (whether because it has left the battlefield, because it has gained Shroud or Protection, or for any
		 * other reason), the ability taps the remaining target, but no damage is dealt. If both targets are illegal, the ability is countered." */
		else if (target_0_valid){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		} else if (target_1_valid){
			tap_card(instance->targets[1].player, instance->targets[1].card);
		} else {
			spell_fizzled = 1;
		}

		get_card_instance(instance->parent_controller, instance->parent_card)->number_of_targets = 0;
		return 0;
	}

	return 0;
}

int card_el_hajjaj(int player, int card, event_t event){
	// 4C8070
	spirit_link_effect(player, card, event, player);
	return 0;
}

int card_two_headed_giant_of_foriys(int player, int card, event_t event)
{
	// 4559F0
	creature_can_block_additional(player, card, event, 1);
	return 0;
}
