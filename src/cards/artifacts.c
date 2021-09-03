#include "manalink.h"

int effect_slowtrip(int player, int card, event_t event){
    if( eot_trigger(player, card, event) ){
        kill_card(player, card, KILL_REMOVE);
        draw_a_card(player);
    }
    return 0;
}


int card_wayfarers_bauble(int player, int card, event_t event){
    if( event == EVENT_RESOLVE_ACTIVATION ){
        tutor_basic_land(player, 1, 1);
    }
    return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_ME, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_urzas_bauble(int player, int card, event_t event){
    if( event == EVENT_RESOLVE_ACTIVATION ){
        int legacy_card = create_legacy_effect(player, card, &effect_slowtrip );
        card_instance_t *legacy = get_card_instance(player, legacy_card);
        legacy->targets[0].card = current_turn;

        if(hand_count[opp]==0){
            return 0;
        }
        int r =  internal_rand( hand_count[opp] );
        int cards_array[ 1 ];
        int i=0;
        int hand_index = 0;
        for(i=0;i<active_cards_count[opp]; i++){
            card_instance_t *instance = get_card_instance(opp, i);
            if( ! ( instance->state & STATE_INVISIBLE ) && ! ( instance->state & STATE_IN_PLAY )  ){
                int id = instance->internal_card_id;
                if( id > -1 ){
                    if( hand_index == r ){
                        cards_array[0] = id;
                    }
                    hand_index++;
                }
            }
        }
        show_deck( player, cards_array, 1, "Card Revealed at Random", 0, 0x7375B0 );
    }
    return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_teferis_puzzle_box(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);	

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
            target_definition_t td;
            default_target_definition(player, card, &td, TYPE_NONE);
            td.allowed_controller = current_turn;
            td.preferred_controller = current_turn;
            td.who_chooses = current_turn;
            td.zone = TARGET_ZONE_HAND;
            td.illegal_abilities = 0;
            td.allow_cancel = 0;
            int cards = hand_count[current_turn];
            int i;
            for(i=0;i<cards;i++){
                pick_target(&td, "BRONZE_TABLET");
                put_on_top_of_deck(current_turn, instance->targets[0].card);
                put_top_card_of_deck_to_bottom(current_turn);
            }
            draw_cards(current_turn, cards);
	}

    return 0;
}

int card_vedalken_shackles(int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.power_requirement = count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND) | TARGET_PT_LESSER_OR_EQUAL;

    card_instance_t *instance = get_card_instance(player, card);

	choose_to_untap(player, card, event);

    if( event == EVENT_RESOLVE_ACTIVATION ){
        td.power_requirement = count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND) | TARGET_PT_LESSER_OR_EQUAL;
		if( valid_target(&td) && in_play(instance->parent_controller, instance->parent_card) && is_tapped(instance->parent_controller, instance->parent_card)){
			gain_control_until_source_is_in_play_and_tapped(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
        }
    }
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}


int card_millstone(int player, int card, event_t event){
    target_definition_t td1;
    default_target_definition(player, card, &td1, TYPE_CREATURE );
    td1.zone = TARGET_ZONE_PLAYERS;

    card_instance_t *instance = get_card_instance(player, card);

    if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			mill( instance->targets[0].player, 2);
		}
    }
    return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td1, "TARGET_PLAYER");
}

int card_serum_powder(int player, int card, event_t event){
    if( ! is_unlocked(player, card, event, 7) ){ return 0; }
    return mana_producer(player, card, event);
}

int card_goblin_charbelcher(int player, int card, event_t event){
    if( ! is_unlocked(player, card, event, 12) ){ return 0; }

    target_definition_t td1;
    default_target_definition(player, card, &td1, TYPE_CREATURE );
    td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

    if(event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(player, card, &td1, 0) ){
				int *deck = deck_ptr[player];
				int i = 0;
				int dmg = 0;
				while( deck[i] != -1 ){
						if( !( cards_data[ deck[i] ].type & TYPE_LAND ) ){
							dmg++;
						}
						else{
							break;
						}
						i++;
				}
				if( deck[i] != -1 ){
					if( has_subtype_by_id(cards_data[ deck[i] ].id, SUBTYPE_MOUNTAIN) ){
						dmg *= 2;
					}
				}

				if( dmg > 0 ){
					damage_creature_or_player(player, card, event, dmg);
				}

				if( i > 0 ){
					put_top_x_on_bottom(player, player, i+1);
				}
			}
    }
    return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td1, "TARGET_PLAYER");
}

int card_jesters_cap(int player, int card, event_t event){

    target_definition_t td1;
    default_target_definition(player, card, &td1, TYPE_CREATURE );
    td1.zone = TARGET_ZONE_PLAYERS;

    card_instance_t *instance = get_card_instance(player, card);

    if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			char msg[100] = "Select a card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			
			int count = 0;
			while( count < 3 ){
					if( new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test) != -1 ){
						count++;
					}
					else{
						break;
					}
			}
			gain_life(instance->targets[0].player, 0);
			shuffle(instance->targets[0].player);
		}
    }
    return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_SACRIFICE_ME, 2, 0, 0, 0, 0, 0, 0, &td1, "TARGET_PLAYER");
}

int get_updated_casting_cost_exc_trinisphere(int player, int card, event_t event, int cless){
	// Seriously?

	int id = -1;
	int fake = -1;
	if( player == -1 ){
		id = cards_data[card].id;
		fake = card;
	}
	else{
		id = get_id(player, card);
		card_instance_t *instance = get_card_instance( player, card );
		fake = instance->internal_card_id;
	}

	card_ptr_t* c = cards_ptr[ id ];
	if( cless == -1 ){
		cless = c->req_colorless;
	}

	int clr = get_color_by_id(player, id);

    int i;
    for(i=0; i<2; i++){
        int count = 0;
        while(count < active_cards_count[i] ){
            if( in_play(i, count) ){
				if( get_id(i, count) == CARD_ID_SPECIAL_EFFECT ){
					card_instance_t *instance = get_card_instance(i, count);
					if( instance->targets[2].player > 0 ){
						if( (instance->targets[2].player & 4) && id == instance->targets[2].card ){
							cless+=49;
						}
						if( ((instance->targets[2].player & 8) || (instance->targets[2].player & 64)) && player == instance->targets[3].player ){
							cless+=49;
						}
						if( (instance->targets[2].player & 128) && player == instance->targets[3].player && ! is_what(-1, fake, TYPE_CREATURE) ){
							cless+=49;
						}
					}
				}
				
				if( get_id(i, count) == CARD_ID_ANGELIC_ARBITER ){
					card_instance_t *instance = get_card_instance(i, count);
					if( instance->targets[1].card > 0 && (instance->targets[1].card & 1) && i != player ){
						cless+=49;
					}
				}

				if( get_id(i, count) == CARD_ID_ANIMAR_SOUL_OF_ELEMENTS && i == player ){
					cless-=count_1_1_counters(i, count);
				}

				if( get_id(i, count) == CARD_ID_ARCANE_LABORATORY || get_id(i, count) == CARD_ID_RULE_OF_LAW ){
					card_instance_t *instance = get_card_instance(i, count);
					if( instance->targets[1].player > 0 && (instance->targets[1].player & (1+player)) ){
						cless+=49;
					}
				}

				if( get_id(i, count) == CARD_ID_ARCANE_MELEE && ! is_what(-1, fake, TYPE_CREATURE) && is_what(-1, fake, TYPE_SPELL) ){
					cless-=2;
				}

				if( get_id(i, count) == CARD_ID_AURA_OF_SILENCE && i != player && is_what(-1, fake, TYPE_ARTIFACT | TYPE_ENCHANTMENT) ){
					cless+=2;
				}

				if( get_id(i, count) == CARD_ID_BALLYRUSH_BANNERET && (has_subtype_by_id(id, SUBTYPE_SOLDIER) || has_subtype_by_id(id, SUBTYPE_KITHKIN)) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_BASANDRA_BATTLE_SERAPH && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 ){
					cless+=49;
				}

				if( get_id(i, count) == CARD_ID_BOSK_BANNERET && (has_subtype_by_id(id, SUBTYPE_TREEFOLK) || has_subtype_by_id(id, SUBTYPE_SHAMAN)) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_BRIGHTHEARTH_BANNERET && (has_subtype_by_id(id, SUBTYPE_ELEMENTAL) || has_subtype_by_id(id, SUBTYPE_WARRIOR)) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_CENTAUR_OMENREADER && is_tapped(i, count) && i == player && is_what(-1, fake, TYPE_CREATURE) ){
					cless-=2;
				}

				if( get_id(i, count) == CARD_ID_CLOUD_KEY ){
					card_instance_t *instance = get_card_instance(i, count);
					if( instance->targets[1].card > 0 && is_what(-1, fake, instance->targets[1].card) ){
						cless--;
					}
				}

				if( get_id(i, count) == CARD_ID_CURSE_OF_EXHAUSTION ){
					card_instance_t *instance = get_card_instance(i, count);
					if( player == instance->targets[0].player && instance->targets[1].player == 66 ){
						cless+=49;
					}
				}

				if( get_id(i, count) == CARD_ID_DEFENSE_GRID && current_turn != player ){
					cless+=3;
				}

				if( get_id(i, count) == CARD_ID_DRAGONSPEAKER_SHAMAN && i == player && has_subtype_by_id(id, SUBTYPE_DRAGON) ){
					cless-=2;
				}

				if( get_id(i, count) == CARD_ID_EMERALD_MEDALLION && i == player &&
					(clr & COLOR_TEST_GREEN)  
                  ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_ETHERIUM_SCULPTOR && is_what(-1, fake, TYPE_ARTIFACT) &&
					i == player
                 ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_ETHERSWORN_CANONIST && ! is_what(-1, fake, TYPE_ARTIFACT) ){
					card_instance_t *instance = get_card_instance(i, count);
					if( instance->targets[1].player > 0 && (instance->targets[1].player & (1+player)) ){
						cless+=49;
					}
				}

				if( get_id(i, count) == CARD_ID_EXCLUSION_RITUAL ){
					card_instance_t *instance = get_card_instance(i, count);
					if( instance->targets[1].card == id ){
						cless+=49;
					}
				}

				if( get_id(i, count) == CARD_ID_EYE_OF_UGIN && i == player && has_subtype_by_id(id, SUBTYPE_ELDRAZI) && 
					get_color_by_id(player, id) == COLOR_TEST_COLORLESS
				  ){
					cless-=2;
				}

				if( get_id(i, count) == CARD_ID_FROGTOSSER_BANNERET && (has_subtype_by_id(id, SUBTYPE_GOBLIN) || has_subtype_by_id(id, SUBTYPE_ROGUE)) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_GADDOCK_TEEG && ! is_what(-1, fake, TYPE_CREATURE) ){
					if( get_cmc_by_id(id) > 3 ){
						cless+=49;
					}	
				}

				if( get_id(i, count) == CARD_ID_GLOOM && (get_color_by_id(player, id) & COLOR_TEST_WHITE)  ){
					cless+=3;
				}

				if( get_id(i, count) == CARD_ID_GOBLIN_ELECTROMANCER && i == player && ! is_what(-1, fake, TYPE_CREATURE) && is_what(-1, fake, TYPE_SPELL) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_GOBLIN_WARCHIEF && i == player && has_subtype_by_id(id, CARD_ID_GOBLIN) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_GRAND_ABOLISHER && i != player && current_turn == i ){
					cless+=49;
				}

				if( get_id(i, count) == CARD_ID_GRAND_ARBITER_AUGUSTIN_IV ){
					if( i == player ){
						cless--;
					}
					else{
						cless++;
					}
				}

				if( (get_id(i, count) == CARD_ID_GRID_MONITOR || get_id(i, count) == CARD_ID_STEEL_GOLEM) && 
					i == player && is_what(-1, fake, TYPE_CREATURE)
				  ){
					cless+=49;
				}

				if( get_id(i, count) == CARD_ID_HEARTLESS_SUMMONING  && i == player &&
					is_what(-1, fake, TYPE_CREATURE)  
                  ){
					cless-=2;
				}

				if( get_id(i, count) == CARD_ID_HELM_OF_AWAKENING ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_HERALD_OF_WAR && i == player && 
					(has_subtype_by_id(id, SUBTYPE_ANGEL) || has_subtype_by_id(id, SUBTYPE_HUMAN))
				  ){
					cless-=count_1_1_counters(i, count);
				}


				if( get_id(i, count) == CARD_ID_IN_THE_EYE_OF_CHAOS && is_what(-1, fake, TYPE_INSTANT) &&
					! is_what(-1, fake, TYPE_CREATURE)
				  ){
					cless+=get_cmc_by_id(id);
				}

				if( get_id(i, count) == CARD_ID_IONA_SHIELD_OF_EMERIA  && i == 1-player &&
					! is_what(-1, fake, TYPE_LAND) 
				  ){
					card_instance_t *instance = get_card_instance(i, count);
					if( clr & instance->targets[10].card ){
						cless+=49;
					}
				}

				if( get_id(i, count) == CARD_ID_JET_MEDALLION  && i == player &&
					(clr & COLOR_TEST_BLACK)  
                  ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_LOCKET_OF_YESTERDAYS && i == player ){
					cless-=count_graveyard_by_id(player, id);
				}

				if( get_id(i, count) == CARD_ID_LODESTONE_GOLEM && ! is_what(-1, fake, TYPE_ARTIFACT) ){
					cless++;
				}

				if( get_id(i, count) == CARD_ID_MANA_MATRIX && i == player &&
					( is_what(-1, fake, TYPE_INSTANT) || is_what(-1, fake, TYPE_ENCHANTMENT) )
                  ){
					cless-=2;
				}

				if( get_id(i, count) == CARD_ID_MEDDLING_MAGE || get_id(i, count) == CARD_ID_NEVERMORE ){
					card_instance_t *instance = get_card_instance(i, count);
					if( instance->info_slot == id ){
						cless+=49;
					}
				}

				if( get_id(i, count) == CARD_ID_MYCOSYNTH_GOLEM && is_what(-1, fake, TYPE_ARTIFACT) && is_what(-1, fake, TYPE_CREATURE) ){
					cless-=count_subtype(player, TYPE_ARTIFACT, -1);
				}

				if( get_id(i, count) == CARD_ID_NIGHTSCAPE_FAMILIAR && i == player){
					if( (clr & COLOR_TEST_RED) || (clr & COLOR_TEST_BLUE) ){
						cless--;
					}
				}

				if( get_id(i, count) == CARD_ID_OMNISCIENCE && i == player){
					cless-=c->req_colorless;
				}

				if( get_id(i, count) == CARD_ID_PEARL_MEDALLION  && i == player && (clr & COLOR_TEST_WHITE) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_PLANAR_GATE  && i == player && is_what(-1, fake, TYPE_CREATURE) ){
					cless-=2;
				}

				if( get_id(i, count) == CARD_ID_RAKDOS_LORD_OF_RIOTS  && i == player && is_what(-1, fake, TYPE_CREATURE) ){
					cless-=get_trap_condition(1-player, TRAP_LIFE_LOST);
				}

				if( get_id(i, count) == CARD_ID_ROOFTOP_STORM && i == player && has_subtype_by_id(id, CARD_ID_ZOMBIE) ){
					cless-=c->req_colorless;
				}

				if( get_id(i, count) == CARD_ID_RUBY_MEDALLION  && i == player && (clr & COLOR_TEST_RED) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_SAPPHIRE_MEDALLION  && i == player && (clr & COLOR_TEST_BLUE) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_SEMBLANCE_ANVIL && i == player ){
					card_instance_t *leg = get_card_instance(i, count);
					if (is_what(-1, fake, leg->info_slot)){
						cless -= 2;
					}
				}

				if( get_id(i, count) == CARD_ID_SPHERE_OF_RESISTANCE ){
					cless++;
				}

				if( get_id(i, count) == CARD_ID_STINKDRINKER_DAREDEVIL && i == player && has_subtype_by_id(id, CARD_ID_GIANT) ){
					cless-=2;
				}

				if( get_id(i, count) == CARD_ID_STONE_CALENDAR && i == player ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_STONYBROOK_BANNERET && (has_subtype_by_id(id, SUBTYPE_MERFOLK) || has_subtype_by_id(id, SUBTYPE_WIZARD)) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_TEFERI_MAGE_OF_ZHALFIR && i != player && ! can_sorcery_be_played(player, event) ){
					cless+=49;
				}

				if( get_id(i, count) == CARD_ID_THALIA_GUARDIAN_OF_THRABEN  && i != player && ! is_what(-1, fake, TYPE_CREATURE) ){
					cless++;
				}

				if( get_id(i, count) == CARD_ID_THORN_OF_AMETHYST && ! is_what(-1, fake, TYPE_CREATURE) ){
					cless++;
				}

				if( get_id(i, count) == CARD_ID_UNDEAD_WARCHIEF && i == player && has_subtype_by_id(id, CARD_ID_ZOMBIE) ){
					cless--;
				}

				if( get_id(i, count) == CARD_ID_VOIDSTONE_GARGOYLE ){
					card_instance_t *instance = get_card_instance(i, count);
					if( instance->targets[1].card == id ){
						cless+=49;
					}
				}

			}
             count++;
        }
    }

    if( cless < 0 ){
        cless = 0;
    }

    return cless;
}

int card_trinisphere(int player, int card, event_t event){
    if( ! is_tapped(player, card) ){
        if( event == EVENT_MODIFY_COST_GLOBAL && ! is_what(affected_card_controller, affected_card, TYPE_LAND) ){
			int cless = get_updated_casting_cost_exc_trinisphere(affected_card_controller, affected_card, event, -1);
			card_ptr_t* c = cards_ptr[ get_id(affected_card_controller, affected_card)  ];
			int cmc = cless + c->req_black + c->req_blue + c->req_green + c->req_red + c->req_white;
            if( cmc < 3 ){
                COST_COLORLESS += 3 - cmc;
            }
        }
    }
    return 0;
}

int card_crucible_of_worlds(int player, int card, event_t event){
    card_instance_t *instance= get_card_instance(player, card);
    if( event == EVENT_CAN_ACTIVATE ){
        if( can_sorcery_be_played(player, event) && ( count_cards_by_id(player, CARD_ID_FASTBOND) > 0 || ! ( land_can_be_played & 1 ) ) ){
            if(count_graveyard_by_type(player, TYPE_LAND) > 0 && instance->targets[0].player != 66 ){
                return 1;
            }
        }
    }
    else if( event == EVENT_ACTIVATE ){
			instance->targets[0].player = 66;
			cant_be_responded_to = 1;

			char msg[100] = "Select a land to play from graveyard.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, msg);
			int result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
			if( result != -1 ){
				land_can_be_played |= 1;
				int amount = count_cards_by_id(2, CARD_ID_HORN_OF_GREED);
				if( amount > 0 ){
					draw_cards(player, amount);
				}
				test_definition_t this_test2;
				new_default_test_definition(&this_test2, TYPE_LAND, msg);
				this_test2.id = CARD_ID_CITY_OF_TRAITORS;
				state_untargettable(player, result, 1);
				new_manipulate_all(player, card, player, &this_test2, KILL_SACRIFICE);
				state_untargettable(player, result, 0);
            }
			instance->targets[0].player = 0;
    }
    return 0;
}


int monolith (int player, int card, event_t event, int untap_cost){
    does_not_untap(player, card, event);
    card_instance_t *instance = get_card_instance( player, card);
    int active = ! is_tapped(player, card) && ! is_animated_and_sick(player, card ) && can_produce_mana(player, card);
    if( event == EVENT_COUNT_MANA && active && affect_me(player, card)){
        declare_mana_available(player, COLOR_COLORLESS, 3);
    }
    if(event == EVENT_CAN_ACTIVATE ){
        if( active == 1 || ( is_tapped(player, card) && has_mana_for_activated_ability(player, card, untap_cost, 0, 0, 0, 0, 0)) ){
            return 1;
        }
    }
    if(event == EVENT_ACTIVATE){
		if( active ){
			produce_mana_tapped(player, card, COLOR_COLORLESS, 3);
		}
		else{
			charge_mana_for_activated_ability(player, card, untap_cost, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
			}
		}
    }
    if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance( player, instance->parent_card);
		if( parent->info_slot == 1 ){
			untap_card( instance->parent_controller, instance->parent_card );
			parent->info_slot = 0;
		}
    }
	if( player == AI && is_tapped(player, card) && has_mana_for_activated_ability(player, card, untap_cost, 0, 0, 0, 0, 0) && current_turn != player && 
		eot_trigger(player, card, event)
	  ){
		charge_mana_for_activated_ability(player, card, untap_cost, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			untap_card(player, card);
		}
    }
    return 0;
}

int card_basalt_monolith (int player, int card, event_t event){
    return monolith(player, card, event, 3);
}

int signet2(int player, int card, event_t event, int c1, int c2){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		instance->targets[1].player != 66 && has_mana(player, COLOR_COLORLESS, 1) 
	  ){
		if( can_produce_mana(player, card) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->targets[1].player = 66;
		charge_mana(player, COLOR_COLORLESS, 1);
		instance->targets[1].player = 0;
		if( spell_fizzled != 1 ){
			produce_mana_tapped2(player, card, c1, 1, c2, 1);
		}
	}
	
	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( ! is_tapped(player, card) && has_mana(player, COLOR_COLORLESS, 2) && instance->targets[1].player != 66 && !is_animated_and_sick(player, card) ){
		  /* Best we can do - this says that it can produce one mana of either color1 or color2.  There may be a way to manipulate the has_mana() backend data
		   * to account for color-fixing, but it's not understood well enough yet.
		   *
		   * Investigating card_celestial_prism(), and in particular dword_738B48, dword_7A2FE4, sub_498F20(), and sub_499050(), might help, but I doubt it;
		   * celestial prism (and the former asm version of signet) doesn't handle EVENT_COUNT_MANA at all. */

		  declare_mana_available_hex(player, (1<<c1) | (1<<c2), 1);
		}
	}

	return 0;
}

int card_golgari_signet  (int player, int card, event_t event){
    return signet2(player, card, event, COLOR_BLACK, COLOR_GREEN);
}

int card_dimir_signet  (int player, int card, event_t event){
    return signet2(player, card, event, COLOR_BLACK, COLOR_BLUE);
}

int card_orzhov_signet  (int player, int card, event_t event){
    return signet2(player, card, event, COLOR_BLACK, COLOR_WHITE);
}

int card_rakdos_signet  (int player, int card, event_t event){
    return signet2(player, card, event, COLOR_BLACK, COLOR_RED);
}

int card_selesnya_signet(int player, int card, event_t event){
    return signet2(player, card, event, COLOR_WHITE, COLOR_GREEN);
}

int card_simic_signet  (int player, int card, event_t event){
    return signet2(player, card, event, COLOR_BLUE, COLOR_GREEN);
}

int card_gruul_signet  (int player, int card, event_t event){
    return signet2(player, card, event, COLOR_RED, COLOR_GREEN);
}

int card_azorius_signet  (int player, int card, event_t event){
    return signet2(player, card, event, COLOR_BLUE, COLOR_WHITE);
}

int card_boros_signet  (int player, int card, event_t event){
    return signet2(player, card, event, COLOR_WHITE, COLOR_RED);
}

int card_izzet_signet  (int player, int card, event_t event){
    return signet2(player, card, event, COLOR_BLUE, COLOR_RED);
}

int card_aether_vial  (int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_CREATURE);
    td.allowed_controller = player;
    td.preferred_controller = player;
    td.illegal_abilities = 0;
    td.zone = TARGET_ZONE_HAND;

	char buffer[100];
	scnprintf(buffer, 100, "Select a Creature with cmc %d.", count_ccounters(player, card));
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
	this_test.cmc = count_ccounters(player, card);
	this_test.zone = TARGET_ZONE_HAND;
			
	upkeep_trigger_ability(player, card, event, player);	

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int AI_choice = 0;
			int most_common_cmc = get_most_common_cmc_in_hand(player, TYPE_CREATURE);
			if( count_ccounters(player, card) >= most_common_cmc ){
				AI_choice = 1;
			}

			char buffer2[50];
			snprintf(buffer2, 50, " Add a Counter (currently at %d)\n Do not add",  count_ccounters(player, card));
			int choice = do_dialog(player, player, card, -1, -1, buffer2, AI_choice);
			if( choice == 0 ){
				add_ccounter(player, card);
			}
	}

    if( event == EVENT_RESOLVE_ACTIVATION ){
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
    }

    return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_tower_of_fortunes( int player, int card, event_t event){

    if(event == EVENT_RESOLVE_ACTIVATION){
        draw_cards(player, 4);
    }
	
    return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 6, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_mind_stone(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_COUNT_MANA && !(is_tapped(player, card)) && affect_me(player, card) && can_produce_mana(player, card)){
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}

	if( event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_produce_mana(player, card)){
		return 1;
	}

    if(event == EVENT_ACTIVATE ){
        int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && count_permanents_by_type(player, card, TYPE_LAND) > 6  &&
			has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) 
		  ){
			ai_choice = 1;
        }
		
        if( ! get_forbid_activation_flag(player, card) && has_mana(player, COLOR_COLORLESS, 2) ){
            choice = do_dialog(player, player, card, -1, -1, " Generate 1\n Sac and draw 1\n Cancel", ai_choice);
        }
        if( choice == 0 ){
			return mana_producer(player, card, event);
        }
        if( choice == 1 ){
            tap_card(player, card);
            charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
            if( spell_fizzled != 1  ){
                instance->info_slot = 1;
                kill_card(player, card, KILL_SACRIFICE);
            }
			else{
				 untap_card(player, card);
			}
		}
	}
	
	if( event == EVENT_RESOLVE_ACTIVATION && instance->info_slot == 1 ){
		draw_cards(player, 1);
	}
	
    return 0;
}

int effect_illusionary_mask(int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);
	
	if( instance->targets[0].player > -1 ){
		if( (event == EVENT_TAP_CARD && is_attacking(instance->targets[0].player, instance->targets[0].card)) ||
			damage_dealt_by_me(instance->targets[0].player, instance->targets[0].card, event, 0) ||
			damage_dealt_to_me(instance->targets[0].player, instance->targets[0].card, event, 0)
		  ){
			instance->targets[1].player = 66;
			int i;
			for(i=0;i<2;i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && get_id(i, count) == CARD_ID_APHETTO_RUNECASTER ){
							draw_some_cards_if_you_want(i, count, i, 1);
						}
						count--;
				}
			}
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[instance->targets[1].card].code_pointer;
			ptFunction(instance->targets[0].player, instance->targets[0].card, EVENT_TURNED_FACE_UP );
			verify_legend_rule(instance->targets[0].player, instance->targets[0].card, get_id(player, card));
			kill_card(player, card, KILL_REMOVE);
		}
			
		if( event == EVENT_CHANGE_TYPE && affect_me(instance->targets[0].player, instance->targets[0].card) && instance->targets[1].player != 66 ){
			event_result = instance->targets[1].card;
		}
	}

    return 0;
}

int hack_illusionary_mask_event = EVENT_CAN_ACTIVATE;
int card_illusionary_mask(int player, int card, event_t event){

	char msg[100] = "Choose a creature card to play face down.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);
	this_test.has_mana_to_pay_cmc = 2;	hack_illusionary_mask_event = event;
	this_test.zone = TARGET_ZONE_HAND;

    card_instance_t *instance = get_card_instance(player, card);

    if( event == EVENT_CAN_ACTIVATE && can_sorcery_be_played(player, event) ){
        return check_battlefield_for_special_card(player, card, player, 0, &this_test);
    }
    else if( event == EVENT_ACTIVATE ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_VALUE, -1, &this_test);
			if( selected != -1 ){
				charge_mana_from_id(player, card, event, get_id(player, selected));
				if( spell_fizzled != 1 ){
					instance->targets[0].card = selected;
				}
			}
			else{
				spell_fizzled = 1;
			}
    }
    else if( event == EVENT_RESOLVE_ACTIVATION ){
        card_instance_t *target = get_card_instance( player, instance->targets[0].card );
		int effect_card = create_targetted_legacy_effect(player, instance->parent_card, &effect_illusionary_mask, player, instance->targets[0].card);
        card_instance_t *leg = get_card_instance( player, effect_card );
		leg->targets[2].card = target->internal_card_id;
		leg->targets[1].card = get_internal_card_id_from_csv_id( CARD_ID_FACE_DOWN_CREATURE );
		leg->targets[3].card = get_id(player, instance->parent_card);
        put_into_play(player, instance->targets[0].card);
    }
    return 0;
}

int card_isochron_scepter  (int player, int card, event_t event){

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_INSTANT, "Choose an instant card with CMC 2 or less.");
	this_test.cmc = 3;
	this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
	this_test.zone = TARGET_ZONE_HAND;

    card_instance_t *instance = get_card_instance(player, card);
	
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			ai_modifier -= 128;
		}
	}
	
	if( comes_into_play(player, card, event) ){
		if( hand_count[player] > 0 ){
			int result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
			if( result > -1 ){
				instance->info_slot = result;
				create_card_name_legacy(player, card, result);
			}
		}
	}

    if( event == EVENT_CAN_ACTIVATE && instance->info_slot > 0 && can_legally_play(player, instance->info_slot ) ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 2, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    if(event == EVENT_ACTIVATE){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 2, 0, 0, 0, 0, 0, 0, 0, 0);
    }
	
    if(event == EVENT_RESOLVE_ACTIVATION ){
			copy_spell( player, instance->info_slot );
    }
	
    return 0;
}

int card_senseis_divining_top  (int player, int card, event_t event){

    card_instance_t *instance = get_card_instance(player, card);
	
    if( event == EVENT_CAN_ACTIVATE && ! get_forbid_activation_flag(player, card) ){
		int good = 0;
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			good = 1;
		}
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			good = 1;
		}
		return good;
    }
    else if(event == EVENT_ACTIVATE){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
				if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					char buffer[52];
					snprintf(buffer, 52, " Rearrange top 3 cards.\n Draw a card and flip Top\n Cancel" );
					choice = do_dialog(player, player, card, -1, -1, buffer, choice);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0){
				if( charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
					instance->info_slot = choice+1;
				}
			}
			else if( choice == 1 ){
					if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
						tap_card( player, card);
						instance->info_slot = choice+1;
					}
			}
			else{
				spell_fizzled = 1;
			}
        
    }
    else if(event == EVENT_RESOLVE_ACTIVATION && spell_fizzled != 1){
        if ( instance->info_slot == 1){
            rearrange_top_x(player, player, 3);
        }
        if ( instance->info_slot == 2){
            draw_a_card(player);
            if( in_play(instance->parent_controller, instance->parent_card) ){
                put_on_top_of_deck(instance->parent_controller, instance->parent_card);
            }
        }
    }
    return 0;
}

int card_chromatic_sphere(int player, int card, event_t event)
{
  if(event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) )
  {
    if( is_animated_and_sick(player, card) ){
        return 0;
    }
    if( has_mana( player, COLOR_ANY, 1 ) && ! is_tapped( player, card ) )
      return 1;
  }
  else if(event == EVENT_ACTIVATE)
  {
    tap_card( player, card);
    charge_mana( player, COLOR_COLORLESS, 1);
    if( spell_fizzled != 1 ){
        kill_card( player, card, KILL_SACRIFICE);
        add_one_mana_any_color(player, card, event);
        draw_a_card(player);
    }
    else{
        untap_card(player, card);
    }
  }
  return 0;
}

int card_helm_of_awakening(int player, int card, event_t event)
{
    if(event == EVENT_MODIFY_COST_GLOBAL ){
        card_data_t* card_d = get_card_data(affected_card_controller, affected_card);
        if(! (card_d->type & TYPE_LAND) ){
            COST_COLORLESS--;
        }
    }
    return 1;
}

int card_chrome_mox(int player, int card, event_t event){
    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_ANY);
	td.allowed_controller = player;
    td.preferred_controller = player;
    td.illegal_abilities = 0;
    td.zone = TARGET_ZONE_HAND;
	td.illegal_type = TYPE_LAND | TYPE_ARTIFACT;

	if( player == AI && ! can_target(&td) && event == EVENT_MODIFY_COST && affect_me(player, card) ){
		infinite_casting_cost();
    }


    if( comes_into_play(player, card, event) > 0 && can_target(&td) ){

        card_instance_t *instance = get_card_instance(player, card);

        int color = 0;

        if( player != AI ){
           if( select_target(player, card, &td, "Choose a card to imprint", NULL)){
              card_instance_t *target = get_card_instance(player, instance->targets[0].card);
              color = cards_data[target->internal_card_id].color;
           }

        }
        else{
             int count = 0;
             int max_index = -1;
             int max_color = -1;
             while( count < active_cards_count[player] ){
                   if( in_hand(player, count)  && get_color(player, count) > max_color && ! is_what(player, count, TYPE_ARTIFACT) &&
						! is_what(player, count, TYPE_LAND)
					){
                      max_color = get_color(player, count);
                      max_index = count;
                      if( max_color > 61 ){
                         break;
                      }
                   }
                   count++;
             }
             if( max_index > -1 ){
                color = get_color(player, max_index);
                instance->targets[0].card = max_index;
             }

        }

        instance->info_slot = color;
		rfg_card_in_hand(player, instance->targets[0].card);
    }
    return mana_producer(player, card, event);
}

int card_mox_opal(int player, int card, event_t event){
    check_legend_rule(player, card, event);
    if( metalcraft(player, card) ){
        return mana_producer(player, card, event);
    }
    return 0;
}


int card_door_to_nothingness( int player, int card, event_t event){

    target_definition_t td;
    default_target_definition(player, card, &td, TYPE_NONE);
    td.zone = TARGET_ZONE_PLAYERS;

    card_instance_t *instance = get_card_instance( player, card);

	comes_into_play_tapped(player, card, event);

    if(event == EVENT_RESOLVE_ACTIVATION){
        if( valid_target(&td) ){
           lose_the_game( instance->targets[0].player );
        }
    }
    return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_SACRIFICE_ME, 0, 2, 2, 2, 2, 2, 0, &td, "TARGET_PLAYER");
}

int card_ironworks( int player, int card, event_t event){
	// krark clan ironworks
	if(event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) ){
		return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
    }

	if(	event == EVENT_ACTIVATE ){
		if( sacrifice(player, card, player, 0, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
             produce_mana(player, COLOR_COLORLESS, 2);
        }
	}
	
	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card) && 
		can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) 
	  ){
		declare_mana_available(player, COLOR_COLORLESS, 2);
	}

	return 0;
}

int card_myr_incubator( int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select an artifact card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);

		int myr = 0;
		int choice = do_dialog(player, player, card, -1, -1, " Auto mode\n Manual mode", 0);
		int *deck = deck_ptr[player];

		if( choice == 1){
			while( deck[0] != -1 ){
					if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
						myr++;
					}
					else{
						break;
					}
			}
		}
		else{
			int count = count_deck(player)-1;
			while( count > -1 ){
					if( is_what(-1, deck[count], TYPE_ARTIFACT) ){
						rfg_card_in_deck(player, count);
						myr++;
					}
					count--;
			}
		}

		generate_tokens_by_id(player, CARD_ID_MYR, myr);
		gain_life(player, 0);
		shuffle(player);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_ME, 6, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_city_in_a_bottle(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) ){
						if( get_id(i, count) > CARD_ID_WORMS_OF_THE_EARTH && get_id(i, count) < CARD_ID_AMULET_OF_KROOG  ){
							kill_card(i, count, KILL_SACRIFICE);
						}
					}
					count--;
			}
		}
    }

	if( event == EVENT_MODIFY_COST_GLOBAL ){
		if( get_id(affected_card_controller, affected_card) > CARD_ID_WORMS_OF_THE_EARTH &&
			get_id(affected_card_controller, affected_card) < CARD_ID_AMULET_OF_KROOG
		  ){
			infinite_casting_cost();
		}
    }

    return 0;
}

int card_gologothian_sylex(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && ! get_forbid_activation_flag(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( has_mana(player, COLOR_COLORLESS, 1) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			tap_card(player, card);
		}
	}
	
	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) ){
						if( get_id(i, count) > CARD_ID_YDWEN_EFREET && get_id(i, count) < CARD_ID_ABOMINATION  ){
							kill_card(i, count, KILL_SACRIFICE);
						}
					}
					count--;
			}
		}
	}
	
	return 0;
}
