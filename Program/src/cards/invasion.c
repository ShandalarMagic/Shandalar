#include "manalink.h"

// Functions
int do_kicker(int player, int card, int colorless, int black, int blue, int green, int red, int white){
	int diff = 0;
	if( ! played_for_free(player, card) ){
		diff = MIN(0, cards_ptr[get_id(player, card)]->req_colorless+true_get_updated_casting_cost(player, card, -1, EVENT_CAST_SPELL, -1));
	}
	if( colorless != -1 ){
		int cless = MAX(0, colorless+diff);
		if( has_mana_multi(player, cless, black, blue, green, red, white) ){
			int choice = do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", 0);
			if( choice == 0 ){
				charge_mana_multi(player, cless, black, blue, green, red, white);
				if( spell_fizzled != 1 ){
					set_special_flags(player, card, SF_KICKED);
					return 1;
				}
			}
		}
	}
	else{
		int choice = do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", 0);
		if( choice == 0 ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				get_card_instance(player, card)->info_slot = x_value;
				if( diff < 0 ){
					get_card_instance(player, card)->info_slot-=diff;
				}
				set_special_flags(player, card, SF_KICKED);
				return 1;
			}
		}
	}

	return 0;
}

int kicker(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! is_token(player, card) && ! check_special_flags(player, card, SF_NOT_CAST) ){
			do_kicker(player, card, colorless, black, blue, green, red, white);
		}
	}
	return 0;
}

int kicked(int player, int card){
	if( check_special_flags(player, card, SF_KICKED) ){
		return 1;
	}
	return 0;
}

static int invasion_dragon(int player, int card, event_t event, int cless, int black, int blue, int green , int red, int white){

	card_instance_t *instance = get_card_instance(player, card);


	if( has_mana_multi(player, cless, black, blue, green, red, white) && instance->targets[1].player != 66 &&
		damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL+DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE)
	  ){
		charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, cless, black, blue, green, red, white);
		if( spell_fizzled != 1 ){
			instance->info_slot = 1<<choose_a_color(player, get_deck_color(player, 1-player));
			return 1;
		}
	}

	return 0;
}

int generic_split_card(int player, int card, event_t event, int can_play_first_half, int priority_first_half,
						int colorless2, int black2, int blue2, int green2, int red2, int white2, int can_play_second_half, int priority_second_half,
						int can_be_fused, const char *name_first_half, const char *name_second_half
  ){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		int c1 = get_updated_casting_cost(player, card, -1, event, colorless2);
		if( has_mana_multi(player, c1, black2, blue2, green2, red2, white2) && can_play_second_half ){
			null_casting_cost(player, card);
			instance->info_slot |= (1<<8);
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		if( (instance->info_slot & (1<<8)) && can_play_second_half){
			return can_play_second_half;
		}
		return can_play_first_half;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( instance->info_slot & (1<<8) ){
			instance->info_slot = (1<<8);
		}
		else{
			instance->info_slot = 0;
		}
		instance->number_of_targets = 0;
		int choice = 1;
		int c1 = get_updated_casting_cost(player, card, -1, event, colorless2);
		int ff = played_for_free(player, card) | is_token(player, card);
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			if( ff || (instance->info_slot & (1<<8)) ){
				if( (ff || has_mana_to_cast_iid(player, event, get_card_instance(player, card)->internal_card_id)) && can_play_first_half){
					if( (ff || has_mana_multi(player, c1, black2, blue2, green2, red2, white2)) && can_play_second_half ){
						if( can_be_fused ){
							int can_fuse = ! not_played_from_hand(player, card);
							if( can_fuse ){
								card_ptr_t* c = cards_ptr[ get_id(player, card)  ];
								int c2 = get_updated_casting_cost(player, card, -1, event, c->req_colorless+colorless2);
								if( ! has_mana_multi(player, c2, c->req_black+black2, c->req_blue+blue2, c->req_green+green2, c->req_red+red2, c->req_white+white2) ){
									can_fuse = 0;
								}
							}
							choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
											name_first_half, can_play_first_half, priority_first_half,
											name_second_half, can_play_second_half, priority_second_half,
											"Fuse", can_fuse, 200);
						}
						else{
							choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
											name_first_half, can_play_first_half, priority_first_half,
											name_second_half, can_play_second_half, priority_second_half);
						}
					}
				}
				else{
					choice = 2;
				}
			}
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot |= choice;
		}
		if( (instance->info_slot & 1) && (instance->info_slot & 2) ){ // Fuse
			card_ptr_t* c = cards_ptr[ get_id(player, card)  ];
			int c2 = get_updated_casting_cost(player, card, -1, event, c->req_colorless+colorless2);
			charge_mana_multi(player, c2, c->req_black+black2, c->req_blue+blue2, c->req_green+green2, c->req_red+red2, c->req_white+white2);
			if( spell_fizzled != 1 ){
				ff = 1;
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
		if( instance->info_slot & 1 ){
			if( !ff && (instance->info_slot & (1<<8)) ){
				charge_mana_from_id(player, card, event, get_id(player, card));
				if( is_x_spell(player, card) ){
					instance->targets[1].card = x_value;
				}
			}
			if( spell_fizzled != 1 ){
				call_card_function(player, card, EVENT_PLAY_FIRST_HALF);
				if( spell_fizzled == 1 ){
					return 0;
				}
			}
		}
		if( instance->info_slot & 2 ){
			if( !ff && (instance->info_slot & (1<<8)) ){
				charge_mana_multi(player, c1, black2, blue2, green2, red2, white2);
				if( colorless2 == -1 ){
					charge_mana(player, COLOR_COLORLESS, -1);
				}
			}
			instance->targets[1].card = x_value;
			if( spell_fizzled != 1 ){
				call_card_function(player, card, EVENT_PLAY_SECOND_HALF);
				if( spell_fizzled == 1 ){
					return 0;
				}
			}
		}
	}
	return 0;
}

static int invasion_apprentices(int player, int card, event_t event, int clr1, int clr2){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;
	td2.allowed_controller = player;

	target_definition_t td3;
	default_target_definition(player, card, &td3, TYPE_CREATURE);
	td3.preferred_controller = player;

	target_definition_t td4;
	default_target_definition(player, card, &td4, TYPE_CREATURE);


	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( (clr1 == COLOR_BLACK || clr2 == COLOR_BLACK) &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_B(1), 0, &td1, NULL)
		  ){
			return 1;
		}
		if( (clr1 == COLOR_BLUE || clr2 == COLOR_BLUE) &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_U(1), 0, &td2, NULL)
		  ){
			return 1;
		}
		if( (clr1 == COLOR_GREEN || clr2 == COLOR_GREEN ) &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_G(1), 0, &td3, NULL)
		  ){
			return 1;
		}
		if( (clr1 == COLOR_RED || clr2 == COLOR_RED ) &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(1), 0, &td3, NULL)
		  ){
			return 1;
		}
		if( (clr1 == COLOR_WHITE || clr2 == COLOR_WHITE) &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_W(1), 0, &td4, NULL)
		  ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		instance->info_slot = 0;
		int abilities[5] = {generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_B(1), 0, &td1, NULL),
							generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_U(1), 0, &td2, NULL),
							generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_G(1), 0, &td3, NULL),
							generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(1), 0, &td3, NULL),
							generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_W(1), 0, &td4, NULL)
							};
		int priorities[5] = {	life[1-player] < 6 && would_validate_arbitrary_target(&td1, 1-player, -1) ? 5 : 15,
								8,
								10,
								5+(10*current_phase == PHASE_AFTER_BLOCKING),
								10
							};

		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE, DLG_OMIT_ILLEGAL,
							"Target player loses 1 life", abilities[0] && (clr1 == COLOR_BLACK || clr2 == COLOR_BLACK), priorities[0],
							"Put target creature on top of your deck", abilities[1] && (clr1 == COLOR_BLUE || clr2 == COLOR_BLUE), priorities[1],
							"Pump a creature", abilities[2] && (clr1 == COLOR_GREEN || clr2 == COLOR_GREEN), priorities[2],
							"Give first strike", abilities[3] && (clr1 == COLOR_RED || clr2 == COLOR_RED), priorities[3],
							"Tap target creature", abilities[4] && (clr1 == COLOR_WHITE || clr2 == COLOR_WHITE), priorities[4]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, 0, (choice == 1), (choice == 2), (choice == 3), (choice == 4), (choice == 5)) ){
			instance->number_of_targets = 0;
			if( choice == 1 ){
				pick_target(&td1, "TARGET_PLAYER");
			}
			if( choice == 2 ){
				new_pick_target(&td2, "Select target creature you control.", 0, 1);
			}
			if( choice == 3 ){
				pick_target(&td3, "TARGET_CREATURE");
			}
			if( choice == 4 ){
				pick_target(&td3, "TARGET_CREATURE");
			}
			if( choice == 5 ){
				pick_target(&td4, "TARGET_CREATURE");
			}
			if( spell_fizzled != 1 ){
				instance->number_of_targets = 1;
				instance->info_slot = choice;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && valid_target(&td1) ){
			lose_life(instance->targets[0].player, 1);
		}
		if( instance->info_slot == 2 && valid_target(&td2) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == 3 && valid_target(&td3) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
		if( instance->info_slot == 4 && valid_target(&td3) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_FIRST_STRIKE, 0);
		}
		if( instance->info_slot == 5 && valid_target(&td4) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

static int mutation_spell(int player, int card, event_t event, int type, const char *buffer){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, type);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int cmc = get_cmc(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			generate_tokens_by_id(player, card, CARD_ID_SAPROLING, cmc);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, buffer, 1, NULL);
}

static int invasion_masters(int player, int card, event_t event, int clr1, int clr2){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;

	target_definition_t td3;
	default_target_definition(player, card, &td3, TYPE_CREATURE);


	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( (clr1 == COLOR_BLACK || clr2 == COLOR_BLACK) &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_B(2), 0, &td1, NULL)
		  ){
			return 1;
		}
		if( (clr1 == COLOR_BLUE || clr2 == COLOR_BLUE) &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_U(2), 0, &td3, NULL)
		  ){
			return 1;
		}
		if( (clr1 == COLOR_GREEN || clr2 == COLOR_GREEN ) && generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_G(2), 0, NULL, NULL) ){
			return 1;
		}
		if( (clr1 == COLOR_RED || clr2 == COLOR_RED ) &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(2), 0, &td3, NULL)
		  ){
			return 1;
		}
		if( (clr1 == COLOR_WHITE || clr2 == COLOR_WHITE) &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_W(2), 0, &td2, NULL)
		  ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		instance->info_slot = 0;
		int abilities[5] = {generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_B(2), 0, &td1, NULL),
							generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_U(2), 0, &td3, NULL),
							generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_G(2), 0,  NULL, NULL),
							generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(2), 0, &td3, NULL),
							generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_W(2), 0, &td2, NULL)
							};
		int priorities[5] = {	life[1-player] < 6 && would_validate_arbitrary_target(&td1, 1-player, -1) ? 5 : 15,
								8,
								current_phase <= PHASE_AFTER_BLOCKING ? 10 : 5,
								8,
								8
							};

		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE, DLG_OMIT_ILLEGAL,
							"Target player loses 2 life", abilities[0] && (clr1 == COLOR_BLACK || clr2 == COLOR_BLACK), priorities[0],
							"Bounce a creature", abilities[1] && (clr1 == COLOR_BLUE || clr2 == COLOR_BLUE), priorities[1],
							"Pump all your creatures", abilities[2] && (clr1 == COLOR_GREEN || clr2 == COLOR_GREEN), priorities[2],
							"Deal 2 damages", abilities[3] && (clr1 == COLOR_RED || clr2 == COLOR_RED), priorities[3],
							"Give protection", abilities[4] && (clr1 == COLOR_WHITE || clr2 == COLOR_WHITE), priorities[4]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, 0,
															(choice == 1 ? 2 : 0),
															(choice == 2 ? 2 : 0),
															(choice == 3 ? 2 : 0),
															(choice == 4 ? 2 : 0),
															(choice == 5 ? 2 : 0)
															)
		  ){
			if( choice == 1 ){
				pick_target(&td1, "TARGET_PLAYER");
			}
			if( choice == 2 ){
				pick_target(&td3, "TARGET_CREATURE");
			}
			if( choice == 4 ){
				pick_target(&td3, "TARGET_CREATURE");
			}
			if( choice == 5 ){
				pick_target(&td2, "TARGET_CREATURE");
			}
			if( spell_fizzled != 1 ){
				instance->number_of_targets = 1;
				instance->info_slot = choice;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && valid_target(&td1) ){
			lose_life(instance->targets[0].player, 2);
			gain_life(player, 2);
		}
		if( instance->info_slot == 2 && valid_target(&td3) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == 3 ){
			pump_subtype_until_eot(instance->parent_controller, instance->parent_card, player, -1, 2, 2, 0, 0);
		}
		if( instance->info_slot == 4 && valid_target(&td3) ){
			damage_target0(player, card, 2);
		}
		if( instance->info_slot == 5 && valid_target(&td2) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, select_a_protection(player), 0);
		}
	}

	return 0;
}

// Cards

int card_absorb(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			card_instance_t *instance = get_card_instance(player, card);
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			gain_life(player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 0, NULL);
}

int card_addle(int player, int card, event_t event){

	/* Addle	|1|B
	 * Sorcery
	 * Choose a color. Target player reveals his or her hand and you choose a card of that color from it. That player discards that card. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int clr = 1<<choose_a_color(player, get_deck_color(player, 1-player));
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			this_test.color = clr;

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			new_effect_coercion(&this_definition, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_aether_rift(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int chosen = get_random_card_in_hand(player);
		int id = -1;
		if( is_what(player, chosen, TYPE_CREATURE) ){
			id = get_id(player, chosen);
		}
		discard_card(player, chosen);
		if( id > -1 ){
			int result = 0;
			int i;
			for(i=0; i<2; i++){
				int ai_choice = 1;
				if( life[player] < 11 ){
					ai_choice = 0;
				}
				char buffer[500];
				card_ptr_t* c = cards_ptr[ id ];
				scnprintf(buffer, 500, " Make %s return\n Pay 5 life", c->name);
				int choice = do_dialog(i, player, card, -1, -1, buffer, ai_choice);
				if( choice == 0 ){
					result++;
				}
				else{
					lose_life(i, 5);
					result--;
				}
			}
			if( result == 2 ){
				seek_grave_for_id_to_reanimate(player, card, player, id, REANIMATE_DEFAULT);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_ancient_spring(int player, int card, event_t event){
	return sac_land_tapped(player, card, event, COLOR_BLUE, COLOR_WHITE, COLOR_BLACK);
}

int card_angelic_shield(int player, int card, event_t event){

	boost_creature_type(player, card, event, -1, 0, 1, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_armadillo_cloack(int player, int card, event_t event){
	if (in_play(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if (instance->damage_target_player >= 0 && instance->damage_target_card >= 0){
			spirit_link_effect(instance->damage_target_player, instance->damage_target_card, event, player);
		}
	}

	return generic_aura(player, card, event, player, 2, 2, KEYWORD_TRAMPLE, 0, 0, 0, 0);
}

int card_artifact_mutation(int player, int card, event_t event){
	/* Artifact Mutation	|R|G
	 * Instant
	 * Destroy target artifact. It can't be regenerated. Put X 1/1 |Sgreen Saproling creature tokens onto the battlefield, where X is that artifact's converted mana cost. */

	return mutation_spell(player, card, event, TYPE_ARTIFACT, "TARGET_ARTIFACT");
}

int card_assault_battery(int player, int card, event_t event){
	/*
	  Assault (Assault/Battery) |R
	  Sorcery
	  Assault deals 2 damage to target creature or player.

	  Battery |3|G
	  Sorcery
	  Put a 3/3 green Elephant creature token onto the battlefield.
	*/
	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XG(3, 1), 1, 0, 0, NULL, NULL);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}


	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
		if( (instance->info_slot & 2) ){
			generate_token_by_id(player, card, CARD_ID_ELEPHANT);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 10, MANACOST_XG(3, 1), 1, 5, 0, "Assault", "Battery");
}

int card_atalya_samite_master(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.special = TARGET_SPECIAL_DAMAGE_CREATURE;
	td1.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_DAMAGE_PREVENTION_CREATURE, MANACOST_W(1), 0, &td1, "TARGET_DAMAGE") )
			return 99;
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_W(1), 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = instance->number_of_targets = 0;
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_DAMAGE_PREVENTION_CREATURE, MANACOST0, 0, &td1, NULL) ){
			int ai_choice = life[player] < 6 ? 1 : 0;
			choice = do_dialog(player, player, card, -1, -1, " Prevent damage to a creature\n Gain life\n Cancel", ai_choice);
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			charge_mana(player, COLOR_WHITE, -1);
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
				if( choice == 0 ){
					if( new_pick_target(&td1, "Select a damage card which targets a creature.", 0, 1 | GS_LITERAL_PROMPT) ){
						instance->targets[1].player = 66+choice;
						tap_card(player, card);
					}
				}
				if( choice == 1 ){
					instance->targets[1].player = 66+choice;
					tap_card(player, card);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player == 66 && valid_target(&td1) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( dmg->info_slot > instance->info_slot ){
				dmg->info_slot-=instance->info_slot;
			}
			else{
				dmg->info_slot = 0;
			}
		}
		if( instance->targets[1].player == 67){
			gain_life(player, instance->info_slot);
		}
	}

	return 0;
}

int card_aura_mutation(int player, int card, event_t event){
	/* Aura Mutation	|G|W
	 * Instant
	 * Destroy target enchantment. Put X 1/1 |Sgreen Saproling creature tokens onto the battlefield, where X is that enchantment's converted mana cost. */

	return mutation_spell(player, card, event, TYPE_ENCHANTMENT, "TARGET_ENCHANTMENT");
}

int card_aura_shards(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, player, 1+player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

		if( can_target(&td) && pick_target(&td, "DISENCHANT") ){
			instance->number_of_targets = 1;
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return global_enchantment(player, card, event);
}

int card_backlash(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			int dmg = get_power(instance->targets[0].player, instance->targets[0].card);
			damage_player(instance->targets[0].player, dmg, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target untapped creature.", 1, NULL);
}

int card_barrins_spite(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
			td.allowed_controller = 1-player;
			td.preferred_controller = 1-player;
			if( target_available(player, card, &td) > 1 ){
				return 1;
			}
			td.allowed_controller = player;
			td.preferred_controller = player;
			if( target_available(player, card, &td) > 1 ){
				return 1;
			}
			default_target_definition(player, card, &td, TYPE_CREATURE);
		}
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			td.allowed_controller = instance->targets[0].player;
			td.preferred_controller = instance->targets[0].player;
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			if( can_target(&td) ){
				new_pick_target(&td, "TARGET_CREATURE", 1, 1);
			} else {
				cancel = 1;
			}
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}
	if(event == EVENT_RESOLVE_SPELL ){
		int legal_targets = validate_target(player, card, &td, 0) | (2*validate_target(player, card, &td, 1));
		if( legal_targets ){
			if( legal_targets & 1 ){
				if( legal_targets & 2 ){
					int p, c;
					for(p=0; p<2; p++){
						for(c=0; c<active_cards_count[p]; c++){
							if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) ){
								state_untargettable(p, c, 1);
							}
						}
					}
					state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
					state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);

					target_definition_t td2;
					default_target_definition(player, card, &td2, TYPE_CREATURE);
					td2.allowed_controller = instance->targets[0].player;
					td2.preferred_controller = instance->targets[0].player;
					td2.who_chooses = instance->targets[0].player;
					td2.allow_cancel = 0;

					if( new_pick_target(&td2, "Select the creature to sacrifice.", 2, GS_LITERAL_PROMPT) ){
						if( instance->targets[0].player == instance->targets[2].player && instance->targets[0].card == instance->targets[2].card ){
							state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
							if( can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
								kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
							}
							else{
								state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
							}
							bounce_permanent(instance->targets[1].player, instance->targets[1].card);
						}
						else{
							state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
							if( can_sacrifice(player, instance->targets[1].player, 1, TYPE_CREATURE, 0) ){
								kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
							}
							else{
								state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
							}
							bounce_permanent(instance->targets[0].player, instance->targets[0].card);
						}
					}
					for(p=0; p<2; p++){
						for(c=0; c<active_cards_count[p]; c++){
							if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) ){
								state_untargettable(p, c, 0);
							}
						}
					}
				}
			}
			else{
				if( can_sacrifice(player, instance->targets[1].player, 1, TYPE_CREATURE, 0) ){
					kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_benalish_heralds(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XU(3, 1), 0, NULL, NULL);
}

int card_bind2(int player, int card, event_t event)
{
  if (event == EVENT_CAN_CAST)
	return can_counter_activated_ability(player, card, event, NULL);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  get_card_instance(player, card)->number_of_targets = 0;
	  return cast_counter_activated_ability(player, card, 0);
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (resolve_counter_activated_ability(player, card, NULL, 0))
		draw_cards(player, 1);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_blazing_specter(int player, int card, event_t event)
{
  //0x4d8fd0

  /* Blazing Specter	|2|B|R
   * Creature - Specter 2/2
   * Flying, haste
   * Whenever ~ deals combat damage to a player, that player discards a card. */

  haste(player, card, event);
  whenever_i_deal_damage_to_a_player_he_discards_a_card(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE, 0);
  return 0;
}

int card_bloodstone_cameo(int player, int card, event_t event){
	return mana_producer(player, card, event);
}

int card_blurred_mangoose(int player, int card, event_t event){
	cannot_be_countered(player, card, event);
	return flash(player, card, event);
}

int card_breaking_wave(int player, int card, event_t event)
{
  /* Breaking Wave	|2|U|U
   * Sorcery
   * You may cast ~ any time you could cast an instant if you pay |2 more to cast it.
   * Simultaneously untap all tapped creatures and tap all untapped creatures. */

  if (event == EVENT_MODIFY_COST && !can_sorcery_be_played(player, event))
	COST_COLORLESS += 2;

  if (event == EVENT_CAN_CAST)
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  char tapped[2][151] = {{0}};
	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_CREATURE))
			tapped[p][c] = is_tapped(p, c) ? 1 : 2;

	  APNAP(pl,
			{
			  for (c = 0; c < active_cards_count[pl]; ++c)
				if (tapped[pl][c] && in_play(pl, c))
				  {
					if (tapped[pl][c] == 1)
					  untap_card(pl, c);
					else
					  tap_card(pl, c);
				  }
			});

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_breath_of_darigaaz(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if(event == EVENT_RESOLVE_SPELL ){
		int dmg = kicked(player, card) ? 4 : 1;
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		this_test.keyword_flag = 1;
		new_damage_all(player, card, 2, dmg, NDA_PLAYER_TOO, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return kicker(player, card, event, MANACOST_X(2));
}

int card_canopy_surge(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
			int dmg = kicked(player, card) ? 4 : 1;
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			new_damage_all(player, card, 2, dmg, NDA_PLAYER_TOO, &this_test);
			kill_card(player, card, KILL_DESTROY);
	}

	return kicker(player, card, event, MANACOST_X(2));
}

int card_captain_sisay(int player, int card, event_t event){

	/* Captain Sisay	|2|G|W
	 * Legendary Creature - Human Soldier 2/2
	 * |T: Search your library for a legendary card, reveal that card, and put it into your hand. Then shuffle your library. */

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION  ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, 0, "Select a legendary card.");
		this_test.subtype = SUBTYPE_LEGEND;

		card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
		parent->targets[2].card = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_DECK_SEARCHER, MANACOST0, 0, NULL, NULL);
}

static int cauldron_dance_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	haste(instance->targets[0].player, instance->targets[0].card, event);

	if( eot_trigger(player, card, event) ){
		if( ! check_state(instance->targets[0].player, instance->targets[0].card, STATE_OUBLIETTED) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_cauldron_dance(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	if( event == EVENT_CAN_CAST && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 ){
		return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &this_test);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected > -1 ){
			int zombo = reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			if( zombo > -1 ){
				create_targetted_legacy_effect(player, card, &cauldron_dance_legacy, player, zombo);
			}
			this_test.zone = TARGET_ZONE_HAND;
			if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
				int zombo2 = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
				if( zombo2 > -1  ){
					create_targetted_legacy_effect(player, card, &haste_and_sacrifice_eot, player, zombo2);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_charging_troll(int player, int card, event_t event)
{
  //0x1205390

  /* Charging Troll	|2|G|W
   * Creature - Troll 3/3
   * Vigilance
   * |G: Regenerate ~. */

  vigilance(player, card, event);
  return regeneration(player, card, event, MANACOST_G(1));
}

int card_chromatic_sphere(int player, int card, event_t event)
{
  if (event == EVENT_CAN_ACTIVATE)
	return !is_tapped(player, card) && !is_animated_and_sick(player, card) && has_mana(player, COLOR_ANY, 1) && can_produce_mana(player, card);

  if (event == EVENT_ACTIVATE)
	{
	  tap_card(player, card);
	  charge_mana(player, COLOR_COLORLESS, 1);
	  if (spell_fizzled != 1 && produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, 1))
		{
		  kill_card(player, card, KILL_SACRIFICE);
		  draw_a_card(player);
		}
	  else
		untap_card_no_event(player, card);
	}

  return 0;
}

int card_cinder_shade(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return generic_shade(player, card, event, 0, MANACOST_B(1), 1, 1, 0, 0);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST_R(1), 0, &td, NULL) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = instance->number_of_targets = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_B(1), 0, NULL, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST_R(1), 0, &td, NULL) ){
				int ai_choice = current_phase == PHASE_AFTER_BLOCKING ? 0 : 1;
				choice = do_dialog(player, player, card, -1, -1, " Pump\n Sac & damage\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
				instance->info_slot = -1;
			}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, MANACOST_R(1));
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
					instance->info_slot = get_power(player, card);
					kill_card(player, card, KILL_SACRIFICE);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot < 0 ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, 1);
		}
		else{
			if( valid_target(&td) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot, player, card);
			}
		}
	}

	return 0;
}

int card_coalition_victory(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if(event == EVENT_RESOLVE_SPELL ){
		int you_win = 0;

		if( count_domain(player, card) == 5 ){
			int c;
			int global_colors = 0;
			for(c=0; c<active_cards_count[player]; c++){
				if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) ){
					global_colors |= get_color(player, c);
					if( global_colors == COLOR_TEST_BLACK + COLOR_TEST_BLUE + COLOR_TEST_GREEN + COLOR_TEST_RED + COLOR_TEST_WHITE ){
						you_win = 1;
						break;
					}
				}
			}
		}

		if( you_win ){
			lose_the_game(1-player);
		}

		kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_collapsing_borders(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		gain_life(current_turn, count_domain(current_turn, card));
		damage_player(current_turn, 3, player, card);
	}

	return global_enchantment(player, card, event);
}

int card_collective_restraint(int player, int card, event_t event){
	tax_attack(player, card, event );
	return global_enchantment(player, card, event);
}

int card_crosis_the_purger(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( invasion_dragon(player, card, event, MANACOST_XB(2, 1)) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		this_test.color = instance->info_slot;

		ec_definition_t this_definition;
		default_ec_definition(1-player, player, &this_definition);
		this_definition.effect = EC_ALL_WHICH_MATCH_CRITERIA | EC_DISCARD;
		new_effect_coercion(&this_definition, &this_test);

		instance->info_slot = 0;
	}

	return 0;
}

int card_crypt_angel(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		char msg[100] = "Select a blue or red creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.color = COLOR_TEST_BLUE | COLOR_TEST_RED;
		if( new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(player) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

int card_darigaaz_the_igniter(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( instance->targets[1].player != 66 && invasion_dragon(player, card, event,  MANACOST_XR(2, 1)) ){
		instance->targets[1].player = 66;
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = 1;
		this_test.color = instance->info_slot;
		this_test.zone = TARGET_ZONE_HAND;
		reveal_target_player_hand(1-player);
		int amount = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test);
		if( amount > 0 ){
			damage_player(1-player, amount, player, card);
		}
		instance->info_slot = 0;
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_death_or_glory(int player, int card, event_t event)
{
	if (event == EVENT_CAN_CAST)
	  return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	else if (event == EVENT_RESOLVE_SPELL)
	  {
		/* Move creatures into local dead[] array and obliterate from graveyard, to prevent interactions with creatures that manipulate the graveyard as they
		 * enter the battlefield (such as Gravedigger, Angel of Glory's Rise, Sutured Ghoul, etc.).  This causes problems with Grafdigger's Cage - the creatures
		 * will get moved to the top of their graveyard.  On the other hand, it's only one card.  We could also work around it directly here, by adding another
		 * code path if there's a Cage on the battlefield - don't obliterate cards early, and keep track of the graveyard indices when building piles so the
		 * right ones can be exiled.  Let's wait on that until someone complains.  And at that point, probably better to fix reanimate_all() to work right with
		 * Gravedigger-like cards. */
		int dead[500];
		int dead_count = 0;
		const int* grave = get_grave(player);
		int i;
		for (i = 0; i < 500 && grave[i] != -1;)
		  if (is_what(-1, grave[i], TYPE_CREATURE))
			{
			  dead[dead_count] = grave[i];
			  ++dead_count;
			  obliterate_card_in_grave(player, i);
			}
		  else
			++i;

		if (dead_count > 0)
		  {
			int piles[2][dead_count];

			separate_into_two_piles(player, dead, dead_count, piles);

			int chosen = choose_between_two_piles(1-player, dead_count, piles, AI_MAX_VALUE, "Exile these");

			// Exile the chosen pile
			int any_exiled = 0;
			int* rfg = rfg_ptr[player];
			int rfg_pos = count_rfg(player);
			for (i = 0; i < dead_count; ++i)
			  if (piles[chosen][i] != iid_draw_a_card && piles[chosen][i] != -1)
				{
				  rfg[rfg_pos] = piles[chosen][i];
				  ++rfg_pos;
				  any_exiled = 1;
				}
			if (any_exiled)
			  play_sound_effect(WAV_DESTROY);

			// Now add the non-chosen pile to graveyard and reanimate them, one at a time
			for (i = 0; i < dead_count; ++i)
			  if (piles[1 - chosen][i] != iid_draw_a_card && piles[1 - chosen][i] != -1)
				{
				  int pos = raw_put_iid_on_top_of_graveyard(player, piles[1 - chosen][i]);
				  increase_trap_condition(player, TRAP_CARDS_TO_GY_FROM_ANYWHERE, -1);	// Since not really putting in graveyard
				  reanimate_permanent(player, card, player, pos, REANIMATE_DEFAULT);
				}
		  }

		kill_card(player, card, KILL_DESTROY);
	  }

	return 0;
}

int card_desperate_research(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
	  return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if(event == EVENT_RESOLVE_SPELL ){
		int amount = MIN(7, count_deck(player));
		if( amount > 0 ){
			int rnd = internal_rand(count_deck(player));
			int rounds = 0;
			int selected = -1;
			int *deck = deck_ptr[player];
			while( has_subtype_by_id(cards_data[deck[rnd]].id, SUBTYPE_BASIC) ){
					rnd++;
					if( deck[rnd] == -1 ){
						rnd = 0;
						rounds++;
						if( rounds > 1 ){
							selected = CARD_ID_AIR_ELEMENTAL;
							break;
						}
					}
			}
			if( selected == -1 ){
				selected = cards_data[deck[rnd]].id;
			}
			if( player == HUMAN ){
				selected = card_from_list(player, 3, TYPE_ANY, 0, SUBTYPE_BASIC, DOESNT_MATCH, 0, 0, 0, 0, -1, 0);
			}
			else{
				card_ptr_t* c = cards_ptr[ selected ];
				char buffer[100];
				scnprintf(buffer, 100, "Opponent named : %s", c->name);
				do_dialog(1-player, player, card, -1, -1, buffer, 0);
			}
			show_deck( HUMAN, deck, amount, "Cards revealed with Desperate Research.", 0, 0x7375B0 );
			while( amount ){
					if( cards_data[deck[0]].id == selected ){
						add_card_to_hand(player, deck[0]);
						remove_card_from_deck(player, 0);
					}
					else{
						rfg_card_in_deck(player, 0);
					}
					amount--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dismantling_blow(int player, int card, event_t event){

	/* Dismantling Blow	|2|W
	 * Instant
	 * Kicker |2|U
	 * Destroy target artifact or enchantment.
	 * If ~ was kicked, draw two cards. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "DISENCHANT") ){
			do_kicker(player, card, MANACOST_XU(2, 1));
		}
	}
	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if( kicked(player, card) ){
				draw_cards(player, 2);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_devouring_strossus(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION && can_regenerate(instance->parent_controller, instance->parent_card) ){
		regenerate_target(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_REGENERATION, MANACOST0, 0, NULL, NULL);
}

int card_distorting_wake(int player, int card, event_t event){
	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int trgs = instance->number_of_targets = 0;
		while( can_target(&td) && has_mana(player, COLOR_COLORLESS, trgs+1) ){
				if( new_pick_target(&td, "TARGET_PERMANENT", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		int i;
		for(i=0; i<trgs; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		}
		if( trgs < 1 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana(player, COLOR_COLORLESS, trgs);
			if( spell_fizzled != 1 ){
				instance->info_slot = trgs;
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				bounce_permanent(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// drake skull cameo -->  bloodstone cameo

int card_dromar_the_banisher(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( invasion_dragon(player, card, event, 2, 0, 1, 0, 0, 0) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = instance->info_slot;
		new_manipulate_all(player, card, 2, &this_test, ACT_BOUNCE);
		instance->info_slot = 0;
	}

	return 0;
}

int card_elfhame_sanctuary(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && !is_humiliated(player, card)){
		if (event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_AI(player);
		} else if (event == EVENT_RESOLVE_TRIGGER){
			int count = count_upkeeps(current_turn);
			while( count > 0 ){
				if (!(IS_AI(player) && count_permanents_by_type(player, TYPE_LAND) > 6)){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
					this_test.subtype = SUBTYPE_BASIC;
					/* Ruling 10/4/2004: The "if you do" means "if you search your library".  You get to skip your draw step and shuffle your deck even if you
					 * don't find (or choose not to find) a basic land. */
					new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
					instance->targets[1].player = 66;
				}
				count--;
			}
		}
	}
	if( current_turn == player && event == EVENT_DRAW_PHASE && instance->targets[1].player == 66 ){
		event_result-=99;
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}
	return global_enchantment(player, card, event);
}

int card_elvish_champion(int player, int card, event_t event){
	if( in_play(player, card) && ! affect_me(player, card) && has_creature_type(affected_card_controller, affected_card, SUBTYPE_ELF) &&
		! is_humiliated(player, card)
	){
		switch( event ){
				case( EVENT_POWER ):
				case( EVENT_TOUGHNESS ):
					event_result++;
					break;
				case( EVENT_ABILITIES ):
					event_result |= get_hacked_walk(player, card, KEYWORD_FORESTWALK);
					break;
				default:
					break;
		}
	}
	return 0;
}

int card_empress_galina(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;
	td.required_subtype = SUBTYPE_LEGEND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td)  ){
			gain_control(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_U(2), 0, &td, "TARGET_LEGENDARY_PERMANENT");
}

int card_exclude(int player, int card, event_t event){
	/*
	  Exclude |2|U
	  Instant, 2U
	  Counter target creature spell.
	  Draw a card.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, &td, 0);
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, &td, 0) ){
			draw_cards(player, 1);
			card_instance_t *instance= get_card_instance(player, card);
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_fact_or_fiction(int player, int card, event_t event){
	if (event == EVENT_CAN_CAST){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		ai_modifier += 48;
	}
	if (event == EVENT_RESOLVE_SPELL){
		effect_fof(player, player, 5, TUTOR_GRAVE);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_fires_of_yavimaya(int player, int card, event_t event){

	if( affected_card_controller == player && in_play(affected_card_controller, affected_card) && ! is_humiliated(player, card) &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  ){
		haste(player, affected_card, event);
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_geothermal_crevice(int player, int card, event_t event){
	return sac_land_tapped(player, card, event, COLOR_RED, COLOR_BLACK, COLOR_GREEN);
}

int card_galinas_knight(int player, int card, event_t event){
	/*
	  Galina's Knight |W|U
	  Creature - Merfolk Knight 2/2
	  Protection from red
	*/
	if( ! is_humiliated(player, card) ){
		protection_from_red(player, card, event);
	}
	return 0;
}

int card_ghitu_fire(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST && ! can_sorcery_be_played(player, event) ){
		COST_COLORLESS+=2;
	}

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_glimmering_angel(int player, int card, event_t event){
	/*
	  Glimmering Angel |3|W
	  Creature - Angel 2/2
	  Flying
	  {U}: Glimmering Angel gains shroud until end of turn. (It can't be the target of spells or abilities.)
	*/
	return generic_shade(player, card, event, 0, MANACOST_U(1), 0, 0, KEYWORD_SHROUD, 0);
}

int card_global_ruin(int player, int card, event_t event)
{
  // Each player chooses from the lands he or she controls a land of each basic land type, then sacrifices the rest.

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int subtypes[5] = {SUBTYPE_SWAMP, SUBTYPE_ISLAND, SUBTYPE_FOREST, SUBTYPE_MOUNTAIN, SUBTYPE_PLAINS};

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_LAND);
	  td.illegal_abilities = 0;

	  card_instance_t* instance = get_card_instance(player, card);

	  int pl, i;
	  char marked[2][151] = {{0}};
	  // Each player targets a land of each type.  Mark them.
	  APNAP(p,
			{
			  td.allowed_controller = td.preferred_controller = td.who_chooses = p;
			  for (i = 0; i < 5; ++i)
				{
				  td.required_subtype = subtypes[i];
				  if (can_target(&td))
					{
					  load_text(0, "WOOD_ELVES");
					  if (select_target(player, card, &td, text_lines[i], &instance->targets[0]))
						{
						  instance->number_of_targets = 0;
						  marked[instance->targets[0].player][instance->targets[0].card] = 1;
						  state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
						}
					}
				}
			});

	  int can_sac[2];

	  // Unmark the chosen lands; mark all other lands.
	  for (pl = 0; pl <= 1; ++pl)
		{
		  for (i = 0; i < active_cards_count[pl]; ++i)
			if (marked[pl][i])
			  {
				marked[pl][i] = 0;
				state_untargettable(pl, i, 0);
			  }
			else if (in_play(pl, i) && is_what(pl, i, TYPE_LAND))
			  marked[pl][i] = 1;

		  can_sac[pl] = can_sacrifice(player, pl, 1, TYPE_LAND, 0);
		}

	  // Each player who can sacrifice sacrifices all marked lands.
	  APNAP(p,
			{
			  if (can_sac[p])
				for (i = 0; i < active_cards_count[p]; ++i)
				  if (marked[p][i])
					kill_card(p, i, KILL_SACRIFICE);
			});

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_hanna_ships_navigator(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	char msg[100] = "Select an artifact or enchantment card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT | TYPE_ENCHANTMENT, msg);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XUW(1, 1, 1), 0, NULL, NULL) ){
			return new_special_count_grave(player, &this_test) > 0 && ! graveyard_has_shroud(player);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XUW(1, 1, 1)) ){
			int result = select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0);
			if( result > -1 ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int result = validate_target_from_grave(player, card, player, 0);
		if( result > -1 ){
			const int *grave = get_grave(player);
			add_card_to_hand(player, grave[result]);
			remove_card_from_grave(player, result);
		}
	}

	return 0;
}

int card_harsh_judgement(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = 1<<choose_a_color_and_show_legacy(player, card, 1-player, -1);
	}

	if( event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player && damage->damage_source_player != damage->damage_target_player &&
				damage->info_slot > 0
			  ){
				if( instance->targets[3].player & TYPE_SPELL ){
					if( instance->initial_color & get_card_instance(player, card)->info_slot ){
						damage->damage_target_player = damage->damage_source_player;
					}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_heroes_reunion(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) )
			gain_life(instance->targets[0].player, 7);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

// hunting kavu --> giant trap door spider

int card_irrigation_ditch(int player, int card, event_t event){
	return sac_land_tapped(player, card, event, COLOR_WHITE, COLOR_GREEN, COLOR_BLUE);
}

int card_jade_leech(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player && ! is_humiliated(player, card) ){
		if( get_color(affected_card_controller, affected_card) & COLOR_TEST_GREEN ){
			COST_GREEN++;
		}
	}

	return 0;
}

int card_juntu_stakes(int player, int card, event_t event){
	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && ! is_humiliated(player, card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && get_power(affected_card_controller, affected_card) < 2 ){
			card_instance_t *instance= get_card_instance(affected_card_controller, affected_card);
			instance->untap_status &= ~3;
		}
	}
	return 0;
}

int card_kangee_aerie_keeper(int player, int card, event_t event)
{
  /* Kangee, Aerie Keeper	|2|W|U
   * Legendary Creature - Bird Wizard 2/2
   * Kicker |X|2
   * Flying
   * When ~ enters the battlefield, if it was kicked, put X feather counters on it.
   * Other Bird creatures get +1/+1 for each feather counter on ~. */

  check_legend_rule(player, card, event);

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && has_mana(player, COLOR_ANY, 2)
	  && do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", has_mana(player, COLOR_ANY, 3) ? 0 : 1) == 0
	  && charge_mana_multi(player, MANACOST_X(2))
	  && charge_mana_multi(player, MANACOST_X(-1)))
	{
	  get_card_instance(player, card)->info_slot = x_value;
	  set_special_flags(player, card, SF_KICKED);
	}

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && kicked(player, card) && comes_into_play(player, card, event))
	add_counters(player, card, COUNTER_FEATHER, get_card_instance(player, card)->info_slot);

  int counters;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && ! is_humiliated(player, card)
	  && (counters = count_counters(player, card, COUNTER_FEATHER)))
	boost_subtype(player, card, event, SUBTYPE_BIRD, counters,counters, 0,0, BCT_CONTROLLER_ONLY);

  return 0;
}

int card_kavu_chameleon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	cannot_be_countered(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int clr = 1<<choose_a_color(player, get_deck_color(player, 1-player));
		change_color(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, clr,
					CHANGE_COLOR_SET|CHANGE_COLOR_END_AT_EOT|CHANGE_COLOR_NO_SLEIGHT);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_G(1), 0, NULL, NULL);
}

int card_kavu_lair(int player, int card, event_t event){

	if (trigger_condition == TRIGGER_COMES_INTO_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.power = 3;
		this_test.power_flag = 2;

		card_instance_t *instance = get_card_instance(player, card);

		if( new_specific_cip(player, card, event, 2, 2, &this_test) ){
			draw_cards(instance->targets[1].player, 1);
		}
	}

	return global_enchantment(player, card, event);
}

int card_kavu_monarch(int player, int card, event_t event){

	if (trigger_condition == TRIGGER_COMES_INTO_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_KAVU;
		this_test.not_me = 1;

		if( new_specific_cip(player, card, event, 2, 2, &this_test) ){
			add_1_1_counter(player, card);
		}
	}

	boost_creature_type(player, card, event, SUBTYPE_KAVU, 0, 0, KEYWORD_TRAMPLE, BCT_INCLUDE_SELF);

	return 0;
}

int card_kavu_titan(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		do_kicker(player, card, MANACOST_XG(2, 1));
		if( kicked(player, card) ){
			add_1_1_counters(player, card, 3);
		}
	}

	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) && kicked(player, card) ){
		event_result |= KEYWORD_TRAMPLE;
	}

	return 0;
}

int card_keldon_necropolis(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_ACTIVATE ){
		int choice = instance->number_of_targets = instance->info_slot = 0;
		if( ! paying_mana() &&
			generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SACRIFICE_CREATURE | GAA_CAN_TARGET, MANACOST_XR(5, 1), 0, &td, NULL)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac & damage\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 1, 0) &&
				sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) &&
				pick_target(&td, "TARGET_CREATURE_OR_PLAYER")
			  ){
					instance->info_slot = 1;
			}
			else{
				remove_state(player, card, STATE_TAPPED);
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			if( valid_target(&td) ){
				damage_target0(player, card, 2);
			}
		}
	}

	return 0;
}

int card_liberate(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( player == AI ){
			return card_death_ward(player, card, event);
		}
		else{
			return can_target(&td);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI ){
			return card_death_ward(player, card, event);
		}
		else{
			pick_target(&td, "TARGET_CREATURE");
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			if( player == AI ){
				regenerate_target(instance->targets[0].player, instance->targets[0].card);
			}
			remove_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_malice_spite(int player, int card, event_t event){
	/*
	  Malice |3|B
	  Instant
	  Destroy target nonblack creature. It can't be regenerated.

	  Spite |3|U
	  Instant
	  Counter target noncreature spell.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

		target_definition_t td1;
		counterspell_target_definition(player, card, &td1, 0);
		td1.illegal_type = TYPE_CREATURE;

		generic_split_card(player, card, event, can_target(&td), 10, MANACOST_XU(3, 1), counterspell(player, card, EVENT_CAN_CAST, &td1, 0), 8, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		new_pick_target(&td, "Select target non-black creature.", 0, 1 | GS_LITERAL_PROMPT);
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		instance->targets[0].player = card_on_stack_controller;
		instance->targets[0].card = card_on_stack;
		instance->number_of_targets = 1;
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	target_definition_t td1;
	counterspell_target_definition(player, card, &td1, 0);
	td1.illegal_type = TYPE_CREATURE;

	if(event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( (instance->info_slot & 2) && counterspell_validate(player, card, &td1, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 10, MANACOST_XU(3, 1), counterspell(player, card, EVENT_CAN_CAST, &td1, 0), 8, 0, "Malice", "Spite");
}

int card_metathran_aerostat(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_U(1), 0, NULL, NULL) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_U(1)) ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char buffer[100];
		scnprintf(buffer, 100, "Select a creature card with CMC %d.", instance->info_slot);
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
		this_test.cmc = instance->info_slot;
		if( new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test) != -1 ){
			bounce_permanent(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_meteor_storm(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && hand_count[player] > 1 ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XGR(2, 1, 1), 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_XGR(2, 1, 1)) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			multidiscard(player, 2, 1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 4);
		}
	}

	return 0;
}

int card_nightscape_apprentice(int player, int card, event_t event){
	/*
	  Nightscape Apprentice |B
	  Creature - Zombie Wizard 1/1
	  {U}, {T}: Put target creature you control on top of its owner's library.
	  {R}, {T}: Target creature gains first strike until end of turn.
	*/
	return invasion_apprentices(player, card, event, COLOR_BLUE, COLOR_RED);
}

int card_nightscape_master(int player, int card, event_t event){
	return invasion_masters(player, card, event, COLOR_BLUE, COLOR_RED);
}

int card_noble_panther(int player, int card, event_t event){
	/*
	  Noble Panther English |1|W|G
	  Creature - Cat 3/3
	  {1}: Noble Panther gains first strike until end of turn.
	*/
	return generic_shade(player, card, event, 0, MANACOST_X(1), 0, 0, KEYWORD_FIRST_STRIKE, 0);
}


int card_obliterate(int player, int card, event_t event)
{
  /* Obliterate	|6|R|R
   * Sorcery
   * ~ can't be countered.
   * Destroy all artifacts, creatures, and lands. They can't be regenerated. */

  cannot_be_countered(player, card, event);
  return card_jokulhaups(player, card, event);
}

int card_opt(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if( event == EVENT_RESOLVE_SPELL){
		scrylike_effect(player, player, 1);
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_ordered_migration(int player, int card, event_t event)
{
  /* Ordered Migration	|3|W|U
   * Sorcery
   * Domain - Put a 1/1 |Sblue Bird creature token with flying onto the battlefield for each basic land type among lands you control. */

	if (event == EVENT_CAN_CAST)
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);

	if (event == EVENT_RESOLVE_SPELL)
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_BIRD, &token);
	  token.pow = 1;
	  token.tou = 1;
	  token.color_forced = COLOR_TEST_BLUE;
	  token.qty = count_domain(player, card);
	  generate_token(&token);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_overabundance(int player, int card, event_t event){

	if( event == EVENT_TAP_CARD && is_what(affected_card_controller, affected_card, TYPE_LAND) && tapped_for_mana_color >= 0 &&
		! is_humiliated(player, card)
	  ){
		damage_player(affected_card_controller, 1, player, card);
	}

	return card_mana_flare(player, card, event);
}

int card_overload(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_ARTIFACT") ){
			if( get_cmc(instance->targets[0].player, instance->targets[0].card) > 2 ){
				do_kicker(player, card, MANACOST_X(2));
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			if( kicked(player, card) || get_cmc(instance->targets[0].player, instance->targets[0].card) < 3 ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_phyrexian_altar(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			FORCE(produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, 1));
			tapped_for_mana_color = -2;
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_use_activated_abilities(player, card) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

int card_phyrexian_delver(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		if( count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(2) ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			int result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_VALUE, &this_test);
			lose_life(player, get_cmc(player, result));
		}
	}

	return 0;
}

int card_phyrexian_infiltrator(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			exchange_control_of_target_permanents(player, instance->parent_card, player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XU(2, 2), 0, &td, "Select a creature your opponent controls.");
}

int card_phyrexian_lens(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( mana_producer(player, card, event) ){
			return can_pay_life(player, 1);
		}
	}

	if(event == EVENT_ACTIVATE ){
		ai_modifier -= 36;

		produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, 1);
		if( spell_fizzled != 1){
			lose_life(player, 1);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_pay_life(player, 1) ){
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

int card_plague_spitter(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		APNAP(p, {new_damage_all(player, card, p, 1, NDA_ALL_CREATURES, NULL);});
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		APNAP(p, {new_damage_all(player, card, p, 1, NDA_ALL_CREATURES, NULL);});
	}

	return 0;
}

int card_plague_spores(int player, int card, event_t event)
{
  /* Plague Spores	|4|B|R
   * Sorcery
   * Destroy target non|Sblack creature and target land. They can't be regenerated. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}


  target_definition_t td_creature;
  default_target_definition(player, card, &td_creature, TYPE_CREATURE);
  td_creature.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

  target_definition_t td_land;
  default_target_definition(player, card, &td_land, TYPE_LAND);

  if (event == EVENT_CAN_CAST)
	return can_target(&td_creature) && can_target(&td_land);

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  instance->number_of_targets = 0;
	  if (pick_next_target_noload(&td_creature, get_sleighted_color_text(player, card, "Select target non-%s creature.", COLOR_BLACK)))
		new_pick_target(&td_land, "TARGET_LAND", 1, 1);
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (validate_target(player, card, &td_creature, 0))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);

	  if (validate_target(player, card, &td_land, 1))
		kill_card(instance->targets[1].player, instance->targets[1].card, KILL_BURY);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_planar_portal(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a card to tutor.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(6), 0, NULL, NULL);
}

int card_probe(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 1;
		if( ! is_token(player, card) ){
			if( can_target(&td) && do_kicker(player, card, MANACOST_XB(1, 1)) ){
				pick_target(&td, "TARGET_PLAYER");
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		draw_cards(player, 3);
		multidiscard(player, 2, 0);
		if( kicked(player, card) && valid_target(&td) ){
			multidiscard(instance->targets[0].player, 2, 0);
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_prohibit(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = counterspell(player, card, event, NULL, 0);
		if( result ){
			if( player == AI ){
				int cmc = get_cmc(card_on_stack_controller, card_on_stack);
				if( cmc < 5 ){
					if( cmc <=2 || (cmc > 2 && has_mana(player, COLOR_COLORLESS, 2)) ){
						return result;
					}
				}
			}
			else{
				return result;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		counterspell(player, card, event, NULL, 0);
		if( spell_fizzled != 1 ){
			do_kicker(player, card, MANACOST_X(2));
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if (counterspell_validate(player, card, NULL, 0)){
			int cmc = get_cmc(instance->targets[0].player, instance->targets[0].card);
			int good = (cmc <=2 || (cmc > 2 && cmc <= 4 && kicked(player, card))) ? 1 : 0;
			if( good ){
				real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_pyre_zombie(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
		if( has_mana_multi(player, MANACOST_XB(1, 2)) ){
			int choice = do_dialog(player, player, card, -1, -1," Return Pyre Zombie to hand\n Pass", 0);
			if( choice == 0 ){
				charge_mana_multi(player, MANACOST_XB(1, 2));
				if( spell_fizzled != 1){
					instance->state &= ~STATE_INVISIBLE;
					hand_count[player]++;
					return -1;
				}
			}
		}
		return -2;
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;


	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_XR(1, 2), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_raging_kavu(int player, int card, event_t event){
	haste(player, card, event);
	return flash(player, card, event);
}

int card_reckless_assault(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(1), 2, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_recoil(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			if( hand_count[1-player] > 0 ){
				discard(instance->targets[0].player, 0, 0 );
			}
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
}

int card_repulse(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			draw_a_card(player);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_restock(int player, int card, event_t event){

	/* Restock	|3|G|G
	 * Sorcery
	 * Return two target cards from your graveyard to your hand. Exile ~. */

	if( event == EVENT_CAN_CAST ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		if( generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &this_test)){
			return count_graveyard(player) >= 2;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		card_instance_t* instance = get_card_instance(player, card);
		select_multiple_cards_from_graveyard(player, player, -1, AI_MAX_VALUE, NULL, 2, &instance->targets[0]);
	}

	if( event == EVENT_RESOLVE_SPELL){
		int i, num_validated = 0;
		for (i = 0; i < 2; ++i){
			int selected = validate_target_from_grave(player, card, player, i);
			if (selected != -1){
				from_grave_to_hand(player, selected, TUTOR_HAND);
				++num_validated;
			}
		}
		if (num_validated == 0){
			spell_fizzled = 1;
			kill_card(player, card, KILL_DESTROY);
		}
		else {
			kill_card(player, card, KILL_REMOVE);
		}
  }
  return 0;
}

int card_reviving_vapors(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL){
		int amount = MIN(count_deck(player), 3);
		if( amount ){
			int *deck = deck_ptr[player];
			show_deck(1-player, deck, amount, "Cards revealed by Reviving Vapors", 0, 0x7375B0 );

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to add to your hand.");
			this_test.create_minideck = 3;
			this_test.no_shuffle = 1;
			int result = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_CMC, &this_test);
			amount--;
			if( amount ){
				mill(player, amount);
			}
			gain_life(player, get_cmc(player, result));
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_reya_dawnbringer(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_UPKEEP ){
		int can_reanimate = count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player);
		upkeep_trigger_ability_mode(player, card, event, player, can_reanimate ? RESOLVE_TRIGGER_AI(player) : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
		}
	}

	return 0;
}

int card_rith_the_awakener(int player, int card, event_t event){
	/* Rith, the Awakener	|3|R|G|W
	 * Legendary Creature - Dragon 6/6
	 * Flying
	 * Whenever ~ deals combat damage to a player, you may pay |2|G. If you do, choose a color, then put a 1/1 |Sgreen Saproling creature token onto the battlefield for each permanent of that color. */

	check_legend_rule(player, card, event);

	if( invasion_dragon(player, card, event, MANACOST_XG(2, 1)) ){
		card_instance_t *instance = get_card_instance(player, card);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = instance->info_slot;
		generate_tokens_by_id(player, card, CARD_ID_SAPROLING, check_battlefield_for_special_card(player, card, ANYBODY, CBFSC_GET_COUNT, &this_test));
		instance->info_slot = 0;
	}

	return 0;
}

int card_rout(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 10) ){ return 0; }
	if( event == EVENT_MODIFY_COST && ! can_sorcery_be_played(player, event) ){
		COST_COLORLESS += 2;
	}
	return card_wrath_of_god(player, card, event);
}

int card_saproling_infestation(int player, int card, event_t event){
	/* Saproling Infestation	|1|G
	 * Enchantment
	 * Whenever a player kicks a spell, you put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller ){
		if( check_special_flags(trigger_cause_controller, trigger_cause, SF_KICKED) ||
			check_special_flags(trigger_cause_controller, trigger_cause, SF_KICKED2)
		  ){
			if( new_specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, NULL) ){
				generate_token_by_id(player, card, CARD_ID_SAPROLING);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_saproling_symbiosis(int player, int card, event_t event){
	/* Saproling Symbiosis	|3|G
	 * Sorcery
	 * You may cast ~ as though it had flash if you pay |2 more to cast it.
	 * Put a 1/1 |Sgreen Saproling creature token onto the battlefield for each creature you control. */

	if( event == EVENT_MODIFY_COST && ! can_sorcery_be_played(player, event) ){
		COST_COLORLESS+=2;
	}

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_SAPROLING, count_subtype(player, TYPE_CREATURE, -1));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// seashell cameo --> bloodstone cameo

int card_shivan_harvest(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_NONBASIC;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE | GAA_LITERAL_PROMPT, MANACOST_XR(1, 1), 0,
									&td, "Select target nonbasic land.");
}

int card_skizzik(int player, int card, event_t event){

	haste(player, card, event);

	if( !kicked(player, card) && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return kicker(player, card, event, 0, 0, 0, 0, 1, 0);
}

int card_slay2(int player, int card, event_t event){
	/*
	  Slay |2|B
	  Instant, 2B (3)
	  Destroy target green creature. It can't be regenerated.
	  Draw a card.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						get_sleighted_color_text(player, card, "Select target %d creature.", COLOR_GREEN), 1, NULL);
}

int card_sleepers_robe(int player, int card, event_t event)
{
  // original code : 0x4DBC2B

  /* Sleeper's Robe	|U|B
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature has fear.
   * Whenever enchanted creature deals combat damage to an opponent, you may draw a card. */

  int packets;
  if ((packets = equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_OPPONENT|DDBM_TRIGGER_OPTIONAL)))
	draw_cards(player, packets);

  return generic_aura(player, card, event, player, 0,0, 0,SP_KEYWORD_FEAR, 0,0,0);
}

int card_smoldering_tar(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_UPKEEP && current_turn == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		upkeep_trigger_ability_mode(player, card, event, player, can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		instance->number_of_targets = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			lose_life(instance->targets[0].player, 1);
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME | GAA_CAN_SORCERY_BE_PLAYED, MANACOST0, 0,
									&td1, "TARGET_CREATURE");
}

int card_spirit_of_resistance(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage = get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_card == -1 && damage->damage_target_player == player && damage->info_slot > 0 ){
			int i;
			int clr = 0;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_play(player, i) && is_what(player, i, TYPE_PERMANENT) ){
					clr |= get_color(player, i);
				}
				if( num_bits_set(clr) > 4 ){
					break;
				}
			}
			if( num_bits_set(clr) > 4 ){
				damage->info_slot = 0;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_stalking_assassin(int player, int card, event_t event){
	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td_creature;
		default_target_definition(player, card, &td_creature, TYPE_CREATURE);

		target_definition_t td_tapped_creature;
		default_target_definition(player, card, &td_tapped_creature, TYPE_CREATURE);
		td_tapped_creature.required_state = TARGET_STATE_TAPPED;

		int can_tap = !is_tapped(player, card) && !is_sick(player, card);
		int can_tap_creature = can_tap && can_target(&td_creature) && has_mana_for_activated_ability(player, card, MANACOST_XU(3, 1));
		int can_destroy_creature = can_tap && can_target(&td_tapped_creature) && has_mana_for_activated_ability(player, card, MANACOST_XB(3, 1));

		int choice = DIALOG(player, card, event,
							"Tap a creature", can_tap_creature, 1,
							"Destroy a tapped creature", can_destroy_creature, 2);

		if (event == EVENT_CAN_ACTIVATE){
			return choice;
		} else if (event == EVENT_ACTIVATE){
			instance->number_of_targets = 0;
			if (choice == 1){
				if (current_turn == player){
					ai_modifier -= 48;	// Very relucant to activate to tap during own turn
				} else if (current_phase == PHASE_DISCARD){
					ai_modifier += 24;	// If nothing available to destroy by opponent's end phase, more eager to tap
				}
				if (charge_mana_for_activated_ability(player, card, MANACOST_XU(3, 1)) && pick_target(&td_creature, "TARGET_CREATURE")){
					tap_card(player, card);
				}
			} else if (choice == 2){
				if (charge_mana_for_activated_ability(player, card, MANACOST_XB(3, 1)) && pick_target(&td_creature, "TARGET_CREATURE")){
					tap_card(player, card);
				}
			}
		} else {	// EVENT_RESOLVE_ACTIVATION
			if (choice == 1 && valid_target(&td_creature)){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			} else if (choice == 2 && valid_target(&td_tapped_creature)){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		ai_defensive_modifier += 48;
	} else if (event == EVENT_BLOCK_RATING && affect_me(player, card)){
		ai_defensive_modifier -= 48;
	}
	return 0;
}

int card_sterling_grove(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && ! affect_me(player, card) && affected_card_controller == player && ! is_humiliated(player, card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_ENCHANTMENT) ){
			event_result |= KEYWORD_SHROUD;
		}
	}

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(1), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select an enchantment card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, AI_MAX_VALUE, &this_test);
	}

	return global_enchantment(player, card, event);
}

int card_stormscape_apprentice(int player, int card, event_t event){
	/*
	  Stormscape Apprentice |U
	  Creature - Human Wizard 1/1
	  {W}, {T}: Tap target creature.
	  {B}, {T}: Target player loses 1 life.
	*/
	return invasion_apprentices(player, card, event, COLOR_BLACK, COLOR_WHITE);
}

int card_stormscape_master(int player, int card, event_t event){
	return invasion_masters(player, card, event, COLOR_BLACK, COLOR_WHITE);
}

int card_sulfur_vent(int player, int card, event_t event){
	return sac_land_tapped(player, card, event, COLOR_BLACK, COLOR_RED, COLOR_BLUE);
}

int card_sunscape_apprentice(int player, int card, event_t event){
	/*
	  Sunscape Apprentice |W
	  Creature - Human Wizard 1/1
	  {G}, {T}: Target creature gets +1/+1 until end of turn.
	  {U}, {T}: Put target creature you control on top of its owner's library.
	*/
	return invasion_apprentices(player, card, event, COLOR_BLUE, COLOR_GREEN);
}

int card_sunscape_master(int player, int card, event_t event){
	return invasion_masters(player, card, event, COLOR_BLUE, COLOR_GREEN);
}

int card_tangle2(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier-=(current_turn == player ? 50 : 0);
	}


	if( event == EVENT_RESOLVE_SPELL ){
		fog_effect(player, card);
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		new_manipulate_all(player, card, 2, &this_test, ACT_DOES_NOT_UNTAP);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_tectonic_instability(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		if( new_specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, &this_test) ){
			card_instance_t *instance = get_card_instance(player, card);
			new_manipulate_all(player, card, instance->targets[1].player, &this_test, ACT_TAP);
		}
	}

	return global_enchantment(player, card, event);
}

int card_teferis_moat(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

	if (comes_into_play(player, card, event)){
		instance->targets[1].player = 1<<choose_a_color_and_show_legacy(player, card, 1-player, -1);
	}

  if (event == EVENT_ATTACK_LEGALITY && ! is_humiliated(player, card)
	  && affected_card_controller != player
	  && !(get_card_instance(affected_card_controller, affected_card)->regen_status & KEYWORD_FLYING)
	  && instance->targets[1].player >= 0
	  && (get_color(affected_card_controller, affected_card) & instance->targets[1].player))
	event_result = 1;

  return global_enchantment(player, card, event);
}

static const char* targets_a_land_you_control(int who_chooses, int player, int card)
{
  int i;
  card_instance_t* instance = get_card_instance(player, card);

  for (i = 0; i < instance->number_of_targets; ++i)
	if (instance->targets[0].player == who_chooses
		&& is_what(instance->targets[0].player, instance->targets[0].card, TYPE_LAND))
	  return NULL;

  return "targets land you control";
}

int card_teferis_response(int player, int card, event_t event)
{
	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td_effect;
  counter_activated_target_definition(player, card, &td_effect, 0);
  td_effect.allowed_controller = 1-player;
  td_effect.extra = (int32_t)targets_a_land_you_control;
  td_effect.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

  target_definition_t td_spell;
  counterspell_target_definition(player, card, &td_spell, 0);
  td_spell.allowed_controller = 1-player;
  td_spell.extra = (int32_t)targets_a_land_you_control;
  td_spell.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

  if (event == EVENT_CAN_CAST)
	{
	  if (counterspell(player, card, event, &td_spell, 0))
		{
		  instance->info_slot = 1;
		  return 99;
		}
	  if (can_counter_activated_ability(player, card, event, &td_effect))
		{
		  instance->info_slot = 2;
		  return 99;
		}
	  return 0;
	}
  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  if (instance->info_slot == 1)
		counterspell(player, card, event, &td_spell, 0);
	  else if (instance->info_slot == 2)
		cast_counter_activated_ability(player, card, 0);
	}
  else if (event == EVENT_RESOLVE_SPELL)
	{
	  if (instance->info_slot == 1)
		{
		  if (counterspell_validate(player, card, &td_spell, 0))
			{
			  set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			  draw_cards(player, 2);
			}
		}
	  else if (instance->info_slot == 2)
		{
		  if (resolve_counter_activated_ability(player, card, &td_effect, 0))
			{
			  card_instance_t* ability = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			  if (is_what(-1, ability->original_internal_card_id, TYPE_PERMANENT)
				  && in_play(ability->parent_controller, ability->parent_card))
				kill_card(ability->parent_controller, ability->parent_card, KILL_DESTROY);

			  draw_cards(player, 2);
			}
		}
	  kill_card(player, card, KILL_DESTROY);
	}
  else
	return counterspell(player, card, event, &td_spell, 0);

  return 0;
}

int card_tek(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_POWER && affect_me(player, card) ){
			if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_SWAMP) ){
				event_result+=2;
			}
		}

		if( event == EVENT_TOUGHNESS && affect_me(player, card) ){
			if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_PLAINS) ){
				event_result+=2;
			}
		}

		if( event == EVENT_ABILITIES && affect_me(player, card) ){
			int key = 0;
			int count = 0;
			while( count < active_cards_count[player] ){
					if( in_play(player, count) && is_what(player, count, TYPE_LAND) ){
						if( has_subtype(player, count, SUBTYPE_ISLAND) ){
							key |= KEYWORD_FLYING;
						}
						if( has_subtype(player, count, SUBTYPE_MOUNTAIN) ){
							key |= KEYWORD_FIRST_STRIKE;
						}
						if( has_subtype(player, count, SUBTYPE_FOREST) ){
							key |= KEYWORD_TRAMPLE;
						}
					}
					if( key == KEYWORD_FLYING+KEYWORD_FIRST_STRIKE+KEYWORD_TRAMPLE ){
						break;
					}
					count++;
			}
			event_result |= key;
		}
	}

	return 0;
}

int card_thicket_elemental(int player, int card, event_t event){

	/* Thicket Elemental	|3|G|G
	 * Creature - Elemental 4/4
	 * Kicker |1|G
	 * When ~ enters the battlefield, if it was kicked, you may reveal cards from the top of your library until you reveal a creature card. If you do, put that
	 * card onto the battlefield and shuffle all other cards revealed this way into your library. */

	if( kicked(player, card) && comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			int count = 0;
			int found = 0;
			while( deck[count] != -1 ){
					if(  is_what(-1, deck[count], TYPE_CREATURE) ){
						found = 1;
						break;
					}
					count++;
			}
			show_deck( player, deck, count+1, "Cards revealed with Thicket Elemental.", 0, 0x7375B0 );
			if( found ){
				put_into_play_a_card_from_deck(player, player, count);
			}
		}
		shuffle(player);
	}

	return kicker(player, card, event, MANACOST_XG(1, 1));
}

int card_thornscape_apprentice(int player, int card, event_t event){
	/*
	  Thornscape Apprentice |G
	  Creature - Human Wizard 1/1
	  {R}, {T}: Target creature gains first strike until end of turn.
	  {W}, {T}: Tap target creature.
	*/
	return invasion_apprentices(player, card, event, COLOR_RED, COLOR_WHITE);
}

int card_thornscape_master(int player, int card, event_t event){
	return invasion_masters(player, card, event, COLOR_RED, COLOR_WHITE);
}

int card_thunderscape_apprentice(int player, int card, event_t event){
	/*
	  Thunderscape Apprentice |R
	  Creature - Human Wizard 1/1
	  {B}, {T}: Target player loses 1 life.
	  {G}, {T}: Target creature gets +1/+1 until end of turn.
	*/
	return invasion_apprentices(player, card, event, COLOR_BLACK, COLOR_GREEN);
}

int card_thunderscape_master(int player, int card, event_t event){
	return invasion_masters(player, card, event, COLOR_BLACK, COLOR_GREEN);
}

// tigereye cameo --> bloodstone cameo

int card_tinder_farm(int player, int card, event_t event){
	return sac_land_tapped(player, card, event, COLOR_GREEN, COLOR_RED, COLOR_WHITE);
}

// trench wurm --> Dwarven miner

int card_treva_the_renewer(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( invasion_dragon(player, card, event, MANACOST_XW(2, 1)) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = instance->info_slot;
		gain_life(player, check_battlefield_for_special_card(player, card, 2, 4, &this_test));
		instance->info_slot = 0;
	}

	return 0;
}

int card_tribal_flames(int player, int card, event_t event){

	/* Tribal Flames	|1|R
	 * Sorcery
	 * Domain - ~ deals X damage to target creature or player, where X is the number of basic land types among lands you control. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL){
		if (valid_target(&td)){
			damage_target0(player, card, count_domain(player, card));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

// troll horn cameo --> mox sapphire

int card_tsabo_tavoc(int player, int card, event_t event){

	/* Tsabo Tavoc	|5|B|R
	 * Legendary Creature - Horror 7/4
	 * First strike, protection from legendary creatures
	 * |B|B, |T: Destroy target legendary creature. It can't be regenerated. */

	check_legend_rule(player, card, event);

	// Protection from legends, an approximation
	if( ! is_humiliated(player, card) ){
		if( event == EVENT_PREVENT_DAMAGE ){
			card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

			if( damage_card == source->internal_card_id && source->info_slot > 0 ){
				if( source->damage_target_player == player && source->damage_target_card == card ){
					if( in_play(source->damage_source_player, source->damage_source_card) &&
						is_what(source->damage_source_player, source->damage_source_card, TYPE_CREATURE) &&
						is_legendary(source->damage_source_player, source->damage_source_card)
					  ){
						source->info_slot = 0;
					}
				}
			}
		}

		if(event == EVENT_BLOCK_LEGALITY ){
			if(player == attacking_card_controller && card == attacking_card ){
				if( is_legendary(affected_card_controller, affected_card) ){
					event_result = 1;
				}
			}
		}
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_LEGEND;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td)  ){
			card_instance_t* instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_LITERAL_PROMPT, MANACOST_B(2), 0,
									&td, "Select target Legendary creature.");
}

int card_tsabos_decree(int player, int card, event_t event){

	/* Tsabo's Decree	|5|B
	 * Instant
	 * Choose a creature type. Target player reveals his or her hand and discards all creature cards of that type. Then destroy all creatures of that type that
	 * player controls. They can't be regenerated. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}


	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			reveal_target_player_hand(instance->targets[0].player);

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.subtype = select_a_subtype(player, card);

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			this_definition.effect = EC_ALL_WHICH_MATCH_CRITERIA | EC_DISCARD;

			new_effect_coercion(&this_definition, &this_test);
			new_manipulate_all(player, card, instance->targets[0].player, &this_test, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

static int tsabo_web_exceptions(int csvid){
	if( csvid == CARD_ID_BOTTOMLESS_VAULT ||
		csvid == CARD_ID_DWARVEN_HOLD ||
		csvid == CARD_ID_HOLLOW_TREES ||
		csvid == CARD_ID_ICATIAN_STORE ||
		csvid == CARD_ID_SAND_SILOS ||
		csvid == CARD_ID_CRYSTAL_QUARRY ||
		csvid == CARD_ID_DARKWATER_CATACOMBS ||
		csvid == CARD_ID_MOSSFIRE_VALLEY ||
		csvid == CARD_ID_SHADOWBLOOD_RIDGE ||
		csvid == CARD_ID_SKYCLOUD_EXPANSE ||
		csvid == CARD_ID_SUNGRASS_PRAIRIE ||
		csvid == CARD_ID_SHIMMERING_GROTTO ||
		csvid == CARD_ID_UNKNOWN_SHORES
	  ){
		return 1;
	}
	return 0;
}

int card_tsabos_web(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && ! is_humiliated(player, card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_LAND) ){
			card_instance_t *instance= get_card_instance(affected_card_controller, affected_card);
			int affected_land = 0;
			if( (cards_data[instance->internal_card_id].extra_ability & EA_ACT_ABILITY) &&
				! tsabo_web_exceptions(get_id(affected_card_controller, affected_card))
			  ){
				affected_land = 1;
			}
			if( ! affected_land ){
				int special_value = cards_data[instance->internal_card_id].cc[2];
				if( special_value == 4 || special_value == 9 ){ //AKA have a activated ability managed via the Rules Engine card.
					affected_land = 1;
				}
			}
			if( affected_land ){
				instance->untap_status &= ~3;
			}
		}
	}

	return 0;
}

int card_twilights_call(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST && ! can_sorcery_be_played(player, event) ){
		COST_COLORLESS+=2;
	}

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		APNAP(p, {new_reanimate_all(p, -1, p, &this_test, REANIMATE_DEFAULT);});
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_undermine(int player, int card, event_t event){
	/*
	  Undermine |U|U|B
	  Instant
	  Counter target spell. Its controller loses 3 life.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			card_instance_t *instance = get_card_instance(player, card);
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			lose_life(instance->targets[0].player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 0, NULL);
}

int card_urborg_emissary(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allowed_controller = 1-player;
		if( player == HUMAN || (player == AI || can_target(&td)) ){
			return kicker(player, card, event, MANACOST_XU(1, 1));
		}
	}

	if( comes_into_play(player, card, event) && kicked(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_urzas_filter(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL && ! is_humiliated(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		this_test.color_flag = F3_MULTICOLORED;
		if( new_make_test_in_play(affected_card_controller, affected_card, -1, &this_test) ){
			COST_COLORLESS-=2;
		}
	}

	return 0;
}

int card_urzas_rage(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			state_untargettable(player, card, 1);
			if( ! is_token(player, card) ){
				do_kicker(player, card, MANACOST_XR(8,1));
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			int dmg = 3;
			if( kicked(player, card) ){
				dmg = 10;
				int legacy = create_legacy_effect(player, card, &my_damage_cannot_be_prevented);
				get_card_instance(player, legacy)->targets[0].player = player;
				get_card_instance(player, legacy)->targets[0].card = card;
			}
			damage_creature_or_player(player, card, event, dmg);
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_utopia_tree(int player, int card, event_t event){
	return mana_producer(player, card, event);
}

int card_verdeloth_the_ancient(int player, int card, event_t event){
	/* Verdeloth the Ancient	|4|G|G
	 * Legendary Creature - Treefolk 4/7
	 * Kicker |X
	 * Saproling creatures and other Treefolk creatures get +1/+1.
	 * When ~ enters the battlefield, if it was kicked, put X 1/1 |Sgreen Saproling creature tokens onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			do_kicker(player, card, MANACOST_X(-1));
		}
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && kicked(player, card) && comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_SAPROLING, instance->info_slot);
	}

	boost_creature_type(player, card, event, SUBTYPE_SAPROLING, 1, 1, 0, BCT_INCLUDE_SELF);

	boost_creature_type(player, card, event, SUBTYPE_TREEFOLK, 1, 1, 0, 0);

	return 0;
}

int card_verduran_emissary(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.allowed_controller = 1-player;
		if( player == HUMAN || (player == AI || can_target(&td)) ){
			return kicker(player, card, event, MANACOST_XR(1, 1));
		}
	}

	if( comes_into_play(player, card, event) && kicked(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_ARTIFACT") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_vicious_kavu(int player, int card, event_t event){

	// Whenever ~ attacks, it gets +2/+0 until end of turn.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		pump_until_eot(player, card, player, card, 2, 0);
	}

	return 0;
}

int card_vigorous_charge(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST || event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			if( ! is_token(player, card) ){
				do_kicker(player, card, MANACOST_W(1));
			}
		}
	}
	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			int legacy = pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_TRAMPLE, 0);
			if( kicked(player, card) ){
				card_instance_t* leg = get_card_instance(player, legacy);
				if (leg->targets[4].card == -1){
					leg->targets[4].card = 0;
				}
				leg->targets[4].card |= (player == 0 ? 1 : 2);
			}
		}
		kill_card(player, card, KILL_DESTROY );
	}


	return 0;
}

int card_vile_consumption(int player, int card, event_t event){

	if( upkeep_trigger(player, card, event) ){
		int count = active_cards_count[current_turn]-1;
		while( count > -1 ){
				if( in_play(current_turn, count) && is_what(current_turn, count, TYPE_CREATURE) ){
					int ai_choice = 0;
					int choice = 1;
					if( can_pay_life(current_turn, 1) ){
						if( life[player] < 7 ){
							ai_choice = 1;
						}
						choice = do_dialog(current_turn, current_turn, count, -1, -1, " Pay 1 life\n Sac this", ai_choice);
					}
					if( choice == 0 ){
						lose_life(current_turn, 1);
					}
					else{
						kill_card(current_turn, count, KILL_SACRIFICE);
					}
				}
				count--;
		}
	}

	return global_enchantment(player, card, event);
}

int card_void(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int number = 0;
			int i;
			if( player == AI ){
				int cmc[15];

				for(i=0;i<15; i++){
					cmc[i] = 0;
				}

				i=0;
				for(i=0;i<active_cards_count[1-player]; i++){
					if( in_play(1-player, i) && (is_what(1-player, i, TYPE_CREATURE) || is_what(1-player, i, TYPE_ARTIFACT)) ){
						cmc[get_cmc(1-player, i)]++;
					}
				}

				int most_common_cmc = -1;
				int par = 0;
				i=0;
				for(i=0;i<15; i++){
					if( cmc[i] > 0 && cmc[i] > par ){
						par = cmc[i];
						most_common_cmc = i;
					}
				}
				number = most_common_cmc;
			}

			else{
				number = choose_a_number(player, "Choose a number.", 15);
			}
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE | TYPE_ARTIFACT);
			this_test.cmc = number;

			new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);

			default_test_definition(&this_test, TYPE_LAND);
			this_test.type_flag = DOESNT_MATCH;
			this_test.cmc = number;

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			this_definition.effect = EC_ALL_WHICH_MATCH_CRITERIA | EC_DISCARD;
			new_effect_coercion(&this_definition, &this_test);

		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_wax_wane(int player, int card, event_t event){
	/*
	  Wax |G
	  Instant
	  Target creature gets +2/+2 until end of turn.

	  Wane |W
	  Instant
	  Destroy target enchantment.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_ENCHANTMENT);

		generic_split_card(player, card, event, can_target(&td), 0, MANACOST_W(1), can_target(&td2), 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_ENCHANTMENT);
		pick_target(&td2, "TARGET_ENCHANTMENT");
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_ENCHANTMENT);

	if(event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
		if( (instance->info_slot & 2) && valid_target(&td2) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 8, MANACOST_W(1), can_target(&td2), 10, 0, "Wax", "Wane");
}

int card_wash_out(int player, int card, event_t event){

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int clr = 1<<choose_a_color(player, get_deck_color(player, 1-player));
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = clr;
		new_manipulate_all(player, card, 2, &this_test, ACT_BOUNCE);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_yawgmoths_agenda(int player, int card, event_t event)
{
  /* Yawgmoth's Agenda	|3|B|B
   * Enchantment
   * You can't cast more than one spell each turn.
   * You may play cards from your graveyard.
   * If a card would be put into your graveyard from anywhere, exile it instead. */

  if_a_card_would_be_put_into_graveyard_from_anywhere_exile_it_instead(player, card, event, player, NULL);

	if (event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player && get_specific_storm_count(player) > 0
		&& !is_what(affected_card_controller, affected_card, TYPE_LAND) && ! is_humiliated(player, card) )
		infinite_casting_cost();

	if ((event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE) && ! is_humiliated(player, card)){
		type_t types = TYPE_ANY;
		if (get_specific_storm_count(player) > 0)
			types = TYPE_LAND;

		return can_activate_to_play_cards_from_graveyard(player, card, event, types);
	}

	return global_enchantment(player, card, event);
}
