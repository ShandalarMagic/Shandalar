#include "manalink.h"

/****************
* Set mechanics *
****************/
int m15_paragon(int player, int card, event_t event, int clr, int pow, int tou, int key, int s_key){

	boost_creature_by_color(player, card, event, clr, 1, 1, 0, BCT_CONTROLLER_ONLY);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_color = clr;
	td.special = TARGET_SPECIAL_NOT_ME;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								pow, tou, key, s_key);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET,
									(clr & COLOR_TEST_BLACK) ? 2 : 0,
									(clr & COLOR_TEST_BLACK) ? 1 : 0,
									(clr & COLOR_TEST_BLUE) ? 1 : 0,
									(clr & COLOR_TEST_GREEN) ? 1 : 0,
									(clr & COLOR_TEST_RED) ? 1 : 0,
									(clr & COLOR_TEST_WHITE) ? 1 : 0,
									0,
									&td, "TARGET_CREATURE");
}

int cast_spell_with_convoke(int player, int card, event_t event){
	if( ! played_for_free(player, card) && ! is_token(player, card) && get_card_instance(player, card)->info_slot == 1 ){
		card_ptr_t* c = cards_ptr[ get_id(player, card)  ];
		if( ! charge_convoked_mana_extended(player, card, event, c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white) ){
			spell_fizzled = 1;
			return 0;
		}
	}
	return 1;
}

int generic_spell_with_convoke(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST){
		card_ptr_t* c = cards_ptr[ get_id(player, card)  ];
		if( has_convoked_mana_extended(player, card, event, c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white) == 2 ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! cast_spell_with_convoke(player, card, event) ){
			spell_fizzled = 1;
			return 0;
		}
		instance->info_slot = 0;
	}

	return 0;
}


/********
* Cards *
********/

/********
* White *
********/

int card_ajani_steadfast_emblem(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player &&
				damage->info_slot > 1
			  ){
				damage->info_slot = 1;
			}
		}
	}

	return 0;
}

int card_ajani_steadfast(int player, int card, event_t event)
{
	/*
	  Ajani Steadfast |3|W
	  Planeswalker - Ajani
	  [+1]: Until end of turn, up to one target creature gets +1/+1 and gains first strike, vigilance, and lifelink.
	  [-2]: Put a +1/+1 counter on each creature you control and a loyalty counter on each other planeswalker you control.
	  [-7]: You get an emblem with "If a source would deal damage to you or a planeswalker you control, prevent all but 1 of that damage."
	  4
	*/

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		enum{
			CHOICE_PUMP_CREATURE = 1,
			CHOICE_COUNTERS_TO_ALL,
			CHOICE_EMBLEM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Pump a creature", 1, count_counters(player, card, COUNTER_LOYALTY) < 2 ? 20 : 5, 1,
						"Add counters to creatures and planeswalkers", 1, count_counters(player, card, COUNTER_LOYALTY) > 2 ? 20 : 5, -2,
						"Emblem", 1, check_battlefield_for_id(player, CARD_ID_AJANI_STEADFAST_EMBLEM) ? -1 : 20, -7);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  switch (choice)
			{
				case CHOICE_PUMP_CREATURE:
				{
					instance->number_of_targets = 0;
					select_target(player, card, &td, "Select target creature", NULL);
				}
				break;

				case CHOICE_COUNTERS_TO_ALL:
					break;

				case CHOICE_EMBLEM:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_PUMP_CREATURE:
				if ( instance->number_of_targets == 1 && valid_target(&td)){
					pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
											1, 1, KEYWORD_FIRST_STRIKE, SP_KEYWORD_VIGILANCE | SP_KEYWORD_LIFELINK);
				}
				break;

			case CHOICE_COUNTERS_TO_ALL:
				{
					int k;
					for(k=active_cards_count[player]-1; k>-1; k--){
						if( in_play(player, k) ){
							if( is_what(player, k, TYPE_CREATURE) ){
								add_1_1_counter(player, k);
							}
							if( is_planeswalker(player, k) && k!= instance->parent_card ){
								add_counter(player, k, COUNTER_LOYALTY);
							}
						}
					}
				}
				break;

			case CHOICE_EMBLEM:
			  generate_token_by_id(player, card, CARD_ID_AJANI_STEADFAST_EMBLEM);
			  break;
		  }
	}

  return planeswalker(player, card, event, 4);
}

static int avacyn_activation_ai_helper(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.zone = TARGET_ZONE_PLAYERS;
	td1.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	int max_damage = 0;
	int rvalue = 0;

	int i;
	for(i=0; i<2; i++){
		int count = 0;
		while( count < active_cards_count[i] ){
				if( get_card_instance(i, count)->internal_card_id == damage_card && ! check_state(i, count, STATE_CANNOT_TARGET) ){
					card_instance_t *dmg = get_card_instance(i, count);
					if( dmg->damage_target_player == player ){
						if( dmg->damage_target_card == -1 ){
							if( would_validate_arbitrary_target(&td1, dmg->damage_target_player, dmg->damage_target_card) ){
								int clr = in_play(dmg->damage_source_player, dmg->damage_source_card) ?
											get_color(dmg->damage_source_player, dmg->damage_source_card) : dmg->initial_color;
								if( instance->targets[1].player == -1 || !(instance->targets[1].player & clr) ){
									if( has_mana_for_activated_ability(player, card, MANACOST_XW(5, 2)) ){
										if( event == EVENT_CAN_ACTIVATE ){
											return 2;
										}
										if( dmg->info_slot > max_damage ){
											max_damage = dmg->info_slot;
											instance->targets[0].player = dmg->damage_target_player;
											instance->targets[0].card = dmg->damage_target_card;
											instance->number_of_targets = 1;
											int k;
											for(k=1; k<5; k++){
												if( clr & (1<<k) ){
													rvalue = k;
													instance->targets[2].card = 1 | (1<<k);
													break;
												}
											}
											instance->info_slot = 67;
										}
									}
								}
							}
						}
						else{
							if( would_validate_arbitrary_target(&td, dmg->damage_target_player, dmg->damage_target_card) ){
								int clr = in_play(dmg->damage_source_player, dmg->damage_source_card) ?
											get_color(dmg->damage_source_player, dmg->damage_source_card) : dmg->initial_color;
								if( instance->targets[1].card == -1 || !(instance->targets[1].card & clr) ){
									if( has_mana_for_activated_ability(player, card, MANACOST_XW(1, 2)) ){
										if( event == EVENT_CAN_ACTIVATE ){
											return 1;
										}
										if( dmg->info_slot > max_damage ){
											max_damage = dmg->info_slot;
											instance->targets[0].player = dmg->damage_target_player;
											instance->targets[0].card = dmg->damage_target_card;
											instance->number_of_targets = 1;
											int k;
											for(k=1; k<5; k++){
												if( clr & (1<<k) ){
													rvalue = k;
													instance->targets[2].card = 1 | (1<<k);
													break;
												}
											}
											instance->info_slot = 66;
										}
									}
								}
							}
						}
					}
				}
				count++;
		}
	}
	return rvalue;
}

int card_avacyn_guardian_angel(int player, int card, event_t event){
	/*
	  Avacyn, Guardian Angel |2|W|W|W
	  Legendary Creature - Angel
	  Flying, vigilance
	  1W: Prevent all damage that would be dealt to another target creature this turn by sources of the color of your choice.
	  5WW: Prevent all damage that would be dealt to target player this turn by sources of the color of your choice.
	  5/4
	*/
	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.zone = TARGET_ZONE_PLAYERS;
	td1.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int rval = generic_activated_ability(player, card, event, GAA_DAMAGE_PREVENTION, MANACOST_XW(1, 1), 0, NULL, NULL);
		if( rval ){
			return player == HUMAN ? rval : (avacyn_activation_ai_helper(player, card, event) ? rval : 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( player == HUMAN ){
			int choice = 0;
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_DAMAGE_PREVENTION, MANACOST_XW(1, 1), 0, &td, "TARGET_CREATURE") ){
				if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_DAMAGE_PREVENTION, MANACOST_XW(5, 2), 0, &td1, "TARGET_PLAYER") ){
					choice = do_dialog(player, player, card, -1, -1, " Prevent damage to a creature\n Prevent damage to a player\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 2 ){
				spell_fizzled = 1;
				return 0;
			}
			else{
				if( charge_mana_for_activated_ability(player, card, MANACOST_XW(1+(5*choice), 1+choice)) ){
					if( choice == 0 ){
						if( pick_target(&td, "TARGET_CREATURE") ){
							instance->number_of_targets = 1;
							instance->info_slot = 66+choice;
							instance->targets[2].card = 1 | (1<<choose_a_color(player, 0));
						}
					}
					if( choice == 1 ){
						if( pick_target(&td1, "TARGET_PLAYER") ){
							instance->number_of_targets = 1;
							instance->info_slot = 66+choice;
							instance->targets[2].card = 1 | (1<<choose_a_color(player, 0));
						}
					}
				}
			}
		}
		else{
			int result = avacyn_activation_ai_helper(player, card, event);
			if( ! result ){
				spell_fizzled = 1;
				return 0;
			}
			load_text(0, "COLORWORDS");
			char buffer[100];
			scnprintf(buffer, 100, "Opponent chosen: %s", text_lines[4+result]);
			do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( (instance->info_slot == 66 && valid_target(&td)) || (instance->info_slot == 67 && valid_target(&td1)) ){
			prevent_all_damage_to_target(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
										instance->targets[2].card);
		}
	}

	return 0;
}

int card_boonweaver_giant(int player, int card, event_t event){
	/*
	  Boonweaver Giant |6|W
	  Creature - Giant Monk
	  When Boonweaver Giant enters the battlefield, you may search your graveyard, hand, and/or library for an Aura card
	  and put it onto the battlefield attached to Boonweaver Giant. If you search your library this way, shuffle it.
	  4/4
	*/
	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select an Aura card with enchant creature.");
		this_test.subtype = SUBTYPE_AURA_CREATURE;

		int result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_CMC, &this_test);
		if( result == -1 ){
			result = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
			if( result == -1 ){
				result = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_CMC, &this_test);
			}
		}

		if (result > -1){
			put_into_play_aura_attached_to_target(player, result, player, card);
		}
	}

	return 0;
}

int constricting_sliver_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		if( trigger_condition == TRIGGER_LEAVE_PLAY ){
			if( affect_me( player, card) && trigger_cause == instance->damage_target_card && trigger_cause_controller == instance->damage_target_player
				&& reason_for_trigger_controller == player )
			{
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						int iid = instance->targets[1].card;
						if( iid != -1 ){
							int card_added = add_card_to_hand(instance->targets[1].player, iid);
							put_into_play(instance->targets[1].player, card_added);
						}
				}
			}
		}
	}
	return 0;
}

int card_constricting_sliver(int player, int card, event_t event){
	// to insert
	/*
	  Constricting Sliver |5|W
	  Creature - Sliver
	  Sliver creatures you control have "When this creature enters the battlefield,
	  you may exile target creature an opponent controls until this creature leaves the battlefield."
	  3/3
	*/
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_SLIVER;

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		int ai_mode = can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0;

		if( new_specific_cip(player, card, event, player, player == AI ? ai_mode : RESOLVE_TRIGGER_OPTIONAL, &this_test) ){
			int p = instance->targets[1].player;
			int c = instance->targets[1].card;

			default_target_definition(p, c, &td, TYPE_CREATURE);
			td.allowed_controller = 1-player;

			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				int owner = get_owner(instance->targets[0].player, instance->targets[0].card);
				int iid = ! is_token(instance->targets[0].player, instance->targets[0].card) ?
							get_original_internal_card_id(instance->targets[0].player, instance->targets[0].card) : -1;
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
				int legacy = create_targetted_legacy_effect(player, card, &constricting_sliver_legacy, p, c);
				get_card_instance(player, legacy)->targets[1].player = owner;
				get_card_instance(player, legacy)->targets[1].card = iid;
			}
		}
	}

	return 0;
}

int card_dauntless_river_marshal(int player, int card, event_t event){
	/*
	  Dauntless River Marshal |1|W
	  Creature - Human Soldier
	  Dauntless River Marshal gets +1/+1 as long as you control an Island.
	  3U: Tap target creature.
	  2/1
	*/
	uthden_creature(player, card, event, COLOR_BLUE);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		action_on_target(player, card, 0, ACT_TAP);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XU(3, 1), 0, &td, "TARGET_CREATURE");
}

int card_ephemeral_shield(int player, int card, event_t event){
	/*
	  Ephemeral Shields |1|W
	  Instant
	  Convoke
	  Target creature gains indestructible until end of turn.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST){
		return generic_spell_with_convoke(player, card, event);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = cast_spell_with_convoke(player, card, event);
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		td.allow_cancel = result & (1<<31) ? 0 : 1;
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_first_response(int player, int card, event_t event){
	/*
	  First Response |3|W
	  Enchantment
	  At the beginning of each upkeep, if you lost life last turn, put a 1/1 white Soldier creature token onto the battlefield.
	*/
	if( get_card_instance(player, card)->info_slot ){
		upkeep_trigger_ability(player, card, event, ANYBODY);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SOLDIER, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}


	if( event == EVENT_CLEANUP ){
		get_card_instance(player, card)->info_slot = get_trap_condition(player, TRAP_LIFE_LOST);
	}

	return global_enchantment(player, card, event);
}

/*
Geist of the Moors |1|W|W --> vanilla
Creature - Spirit
Flying
3/1
*/

int card_heliod_pilgrim(int player, int card, event_t event){
	/*
	  Heliod's Pilgrim |2|W
	  Creature - Human Cleric
	  When Heliod's Pilgrim enters the battlefield, you may search your library for an Aura card, reveal it, put it into your hand, then shuffle your library.
	  1/2
	*/
	if( comes_into_play(player, card, event) ){
		if( do_dialog(player, player, card, -1, -1, " Tutor an Aura\n Pass", duh_mode(player)) == 0 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select an Aura card.");
			this_test.subtype = SUBTYPE_AURA;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

/*
Hushwing Gryff |2|W --> Ashcoat Bear
Creature - Hippogriff
Flash
Flying
Creatures entering the battlefield don't cause abilities to trigger.
*/

int card_marked_for_honour(int player, int card, event_t event){
	// unconfirmed
	// Marked for Honour 3W
	// Enchantment - Aura (C)
	// Enchant creature
	// Enchanted creature gets +2/+2 and has vigilance.
	return generic_aura(player, card, event, player, 2, 2, 0, SP_KEYWORD_VIGILANCE, 0, 0, 0);
}

int card_meditation_puzzle(int player, int card, event_t event){
	// unconfirmed
	// Meditation Puzzle 3WW
	// Instant (C)
	// Convoke
	// You gain 8 life.
	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 8);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell_with_convoke(player, card, event);
}

int card_paragon_of_new_dawns(int player, int card, event_t event){
	// unconfirmed
	// Paragon of New Dawns 3W
	// Creature - Human Soldier (U)
	// Other white creatures you control get +1/+1.
	// W, T: Another target white creature you control gains vigilance until end of turn.
	// 2/2
	return m15_paragon(player, card, event, COLOR_TEST_WHITE, 0, 0, 0, SP_KEYWORD_VIGILANCE);
}

int card_pillar_of_light(int player, int card, event_t event){
	// to insert
	/*
	  Pillar of Light |2|W
	  Instant
	  Exile target creature with toughness 4 or greater.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.toughness_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature with toughness 4 or more.", 1, NULL);
}

int card_return_to_the_ranks(int player, int card, event_t event){
	/*
	  Return to the Ranks |X|W|W
	  Sorcery
	  Convoke
	  Return X target creature cards with converted mana cost 2 or less from your graveyard to the battlefield.
	*/
	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( has_convoked_mana_extended(player, card, event, MANACOST_W(2)) ){
			null_casting_cost(player, card);
			set_special_flags3(player, card, SF3_MANACOST_NULLIFIED);
		}
		else{
			remove_special_flags3(player, card, SF3_MANACOST_NULLIFIED);
		}
	}

	if(event == EVENT_CAN_CAST){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.cmc = 3;
		test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		return generic_spell(player, card, event, GS_GRAVE_RECYCLER | GS_X_SPELL, NULL, NULL, 1, &test);
	}

	if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		if( (played_for_free(player, card) || is_token(player, card)) && ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;
			return 0;
		}
		if( check_special_flags3(player, card, SF3_MANACOST_NULLIFIED) ){
			if( ! charge_convoked_mana_extended(player, card, event, MANACOST_W(2)) ){
				spell_fizzled = 1;
				return 0;
			}
		}

		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "");
		test.cmc = 3;
		test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		int max_cards = new_special_count_grave(player, &test);

		test_definition_t test2;
		default_test_definition(&test2, TYPE_CREATURE);
		test2.state = STATE_TAPPED;
		test2.state_flag = DOESNT_MATCH;
		int generic_mana_available = has_mana(player, COLOR_ANY, 1);	// A useful trick from card_fireball().
		generic_mana_available+=check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &test2);
		max_cards = MIN(max_cards, generic_mana_available);
		max_cards = MIN(max_cards, 19);

		if( check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			max_cards = instance->info_slot;
		}

		if (max_cards <= 0 || graveyard_has_shroud(player)){
			ai_modifier -= 48;
			instance->info_slot = 0;
			return 0;
		}

		if (ai_is_speculating != 1){
			if (max_cards == 1)
				strcpy(test.message, "Select a creature card with CMC <= 2.");
			else
				sprintf(test.message, "Select up to %d creature cards with CMC <= 2.", max_cards);
		}

		target_t targets[19];
		int num_chosen = select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, &test, max_cards, &targets[0]);

		instance->info_slot = num_chosen;

		if (num_chosen == 0)
			return 0;

		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			if( ! charge_convoked_mana_extended_generic(player, card, event, MANACOST_X(num_chosen), 0) ){
				spell_fizzled = 1;
				return 0;
			}
		}
	}

	if (event == EVENT_RESOLVE_SPELL){
		int num_chosen = instance->info_slot;
		if (num_chosen == 0)
			return 0;

		int i;
		for (i = 0; i < num_chosen; ++i){
			int selected = validate_target_from_grave(player, card, player, i);
			if (selected != -1){
				reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_resolute_archangel(int player, int card, event_t event){
	// unconfirmed
	// Resolute Archangel 5WW
	// Creature - Angel (R)
	// Flying
	// When Resolute Archangel enters the battlefield, if your life total is lower than your starting life total, it becomes your starting life total.
	// 4/4
	if(comes_into_play(player, card, event) ){
		int amount = get_starting_life_total(player)-life[player];
		if( amount ){
			gain_life(player, amount);
		}
	}
	return 0;
}

int card_sanctified_charge(int player, int card, event_t event){
	/*
	  Sanctified Charge |4|W
	  Instant
	  Creatures you control get +2/+1 until end of turn. White creatures you control also gain first strike until end of turn.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=active_cards_count[player]-1; i>-1; i--){
			if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) ){
				if( get_color(player, i) & COLOR_TEST_WHITE ){
					pump_ability_until_eot(player, card, player, i, 2, 1, KEYWORD_FIRST_STRIKE, 0);
				}
				else{
					pump_until_eot(player, card, player, i, 2, 1);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_seraph_of_the_masses(int player, int card, event_t event){
	// Seraph of the Masses 5WW --> Beast of Burden
	// Creature - Angel (U)
	// Convoke
	// Flying
	// Seraph of the Masses power and toughness are each equal to the number of creatures you control.

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += count_subtype(player, TYPE_CREATURE, -1);
	}

	return generic_spell_with_convoke(player, card, event);
}

int card_soul_of_theros(int player, int card, event_t event){
	/*
	  Soul of Theros |4|W|W
	  Creature - Avatar
	  Vigilance
	  4WW: Creatures you control get +2/+2 and gain first strike and lifelink until end of turn.
	  4WW, Exile Soul of Theros from your graveyard: Creatures you control get +2/+2 and gain first strike and lifelink until end of turn.
	*/

	vigilance(player, card, event);

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XW(4, 2)) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_multi(player, MANACOST_XW(4, 2)) ){
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION || event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_subtype_until_eot(instance->parent_controller, instance->parent_card, player, -1, 2, 2, KEYWORD_FIRST_STRIKE, SP_KEYWORD_LIFELINK);
	}
	return generic_activated_ability(player, card, event, 0, MANACOST_XW(4, 2), 0, NULL, NULL);
}

int card_spectral_ward(int player, int card, event_t event){
	/*
	  Spectra Ward |3|W|W
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature gets +2/+2 and has protection from all colors. This effect doesn't remove auras.
	*/
	return generic_aura(player, card, event, player, 2, 2, KEYWORD_PROT_COLORED, 0, 0, 0, 0);
}

int card_spirit_bond(int player, int card, event_t event){
	/*
	  Spirit Bonds |1|W
	  Enchantment
	  Whenever a nontoken creature enters the battlefield under your control, you may pay W.
	  If you do, put a 1/1 white Spirit creature token with flying into play.
	  1W, Sacrifice a Spirit: Target non-Spirit creature you control gains indestructible until end of turn.
	*/
	if( has_mana(player, COLOR_WHITE, 1) && specific_cip(player, card, event, player, RESOLVE_TRIGGER_DUH, TYPE_CREATURE, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0) ){
		charge_mana(player, COLOR_WHITE, 1);
		if( spell_fizzled != 1 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIRIT, &token);
			token.key_plus = KEYWORD_FLYING;
			token.color_forced = COLOR_TEST_WHITE;
			generate_token(&token);
		}
	}


	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_SPIRIT;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int rval = generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XW(1, 1), 0, &td, "TARGET_CREATURE");
		if( rval ){
			return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XW(1, 1)) ){
			if( sacrifice(player, card, player, 0,  TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0) ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
	}

	return 0;
}

/*
Sungrace Pegasus |1|W --> child of night
Creature - Pegasus
Flying
Lifelink
1/2
*/

int card_warden_of_the_beyond(int player, int card, event_t event){
	/*
	  Warden of the Beyond |2|W
	  Creature - Human Wizard
	  Vigilance
	  Warden of the Beyond gets +2/+2 as long as an opponent owns a card in exile.
	  2/2
	*/
	if( ! is_humiliated(player, card) && (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		if( count_rfg(1-player) ){
			event_result+=2;
		}
	}

	return 0;
}

int card_triplicated_spirit(int player, int card, event_t event){
	// unconfirmed
	// Triplicate Spirits 4WW
	// Sorcery (C)
	// Convoke
	// Put three 1/1 white Spirit creature tokens with flying onto the battlefield.

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		token.qty = 3;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell_with_convoke(player, card, event);
}

/*******
 * Blue *
 *******/

int card_aeronaut_tinkerer(int player, int card, event_t event){
	/*
	  Aeronaut Tinkerer |2|U
	  Creature - Human Artificer
	  Aeronaut Tinkerer has flying as long as you control an artifact.
	  2/3
	*/
	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result |= check_battlefield_for_subtype(player, TYPE_ARTIFACT, -1) ? KEYWORD_FLYING : 0;
	}
	return 0;
}

int card_aether_whirlwind(int player, int card, event_t event){
	/*
	  AEther Whirlwind |3|U|U
	  Instant
	  For each attacking creature, its owner chooses to put it on the top or bottom of his or her deck.
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		int ccount = 0;
		for(i=active_cards_count[current_turn]-1; i>-1; i--){
			if( in_play(current_turn, i) && check_state(current_turn, i, STATE_ATTACKING) ){
				int choice = do_dialog(current_turn, current_turn, i, -1, -1, " Put on top of deck\n Put on bottom of deck", ccount < 2 ? 0 : 1);
				action_on_card(player, card, current_turn, i, choice == 0 ? ACT_PUT_ON_TOP : ACT_PUT_ON_BOTTOM);
				ccount++;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_amphin_pathmage(int player, int card, event_t event){
	/*
	  Amphin Pathmage |3|U
	  Creature - Salamander Wizard
	  2U: Target creature can't be blocked this turn.
	  3/2
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;

	return vanilla_creature_pumper(player, card, event, MANACOST_XU(2, 1), 0, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE, &td1);
}

int card_chasm_skulker(int player, int card, event_t event){
	// unconfirmed
	// Chasm Skulker 2U
	// Creature - Squid Horror (R)
	// Whenever you draw a card, put a +1/+1 counter on Chasm Skulker.
	// When Chasm Skulker dies, put X 1/1 blue Squid creature tokens with islandwalk onto the battlefield, where X is the number of +1/+1 counters on Chasm Skulker.
	// 1/1

	if( card_drawn_trigger(player, card, event, player, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counter(player, card);
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SQUID, &token);
		token.key_plus = get_hacked_walk(player, card, KEYWORD_ISLANDWALK);
		token.qty = count_counters(player, card, COUNTER_P1_P1);
		generate_token(&token);
	}

	return 0;
}

int card_chief_engineer(int player, int card, event_t event){
	/*
	  Chief Engineer |1|U
	  Creature - Vedalken Artificer
	  Artifact spells you cast have convoke.
	  1/3
	*/
	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_CAN_ACTIVATE ){
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_hand(player, i) && is_what(player, i, TYPE_ARTIFACT) ){
					card_ptr_t* c = cards_ptr[ get_id(player, i) ];
					if( has_convoked_mana_extended(player, i, event, c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white) ){
						if( is_what(player, i, TYPE_INSTANT) ){
							return 1;
						}
						if( can_sorcery_be_played(player, event) ){
							return 1;
						}
					}
				}
			}
		}
		if( event == EVENT_ACTIVATE ){
			int ap[2][hand_count[player]];
			int apc = 0;
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_hand(player, i) && is_what(player, i, TYPE_ARTIFACT) ){
					card_ptr_t* c = cards_ptr[ get_id(player, i) ];
					if( has_convoked_mana_extended(player, i, event, c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white) ){
						if( is_what(player, i, TYPE_INSTANT) ){
							ap[0][apc] = get_card_instance(player, i)->internal_card_id;
							ap[1][apc] = i;
							apc++;
						}
						if( can_sorcery_be_played(player, event) ){
							ap[0][apc] = get_card_instance(player, i)->internal_card_id;
							ap[1][apc] = i;
							apc++;
						}
					}
				}
			}
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ARTIFACT, "Select an artifact card to play.");
			int selected = select_card_from_zone(player, player, ap[0], apc, 0, AI_MAX_VALUE, -1, &test);
			if( selected != -1 ){
				card_ptr_t* c = cards_ptr[ get_id(player, ap[1][selected] ) ];
				if( charge_convoked_mana_extended(player, ap[1][selected], event, c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white) ){
					play_card_in_hand_for_free(player, ap[1][selected]);
					cant_be_responded_to = 1;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
	}
	return 0;
}

int card_chronostutter(int player, int card, event_t event){
	/*
	  Chronostutter |5|U
	  Instant
	  Put target creature into its owner's library second from the top.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
			int *deck = deck_ptr[instance->targets[0].player];
			int transit = deck[0];
			deck[0] = deck[1];
			deck[1] = transit;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
}

int card_coral_barrier(int player, int card, event_t event){
	// unconfirmed
	// Coral Barrier 2U
	// Creature - Wall (C)
	// Defender
	// When Coral Barrier enters the battlefield, put a 1/1 blue Squid creature token with Islandwalk onto the battlefield.
	// 1/3
	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SQUID, &token);
		token.key_plus = get_hacked_walk(player, card, KEYWORD_ISLANDWALK);
		generate_token(&token);
	}


	return 0;
}

#pragma message "As now, it only counters the spells that targets Slivers if the controller of the spell doesn't pay 2"
int card_diffusion_sliver(int player, int card, event_t event){
	/*
	  Diffusion Sliver |1|U
	  Creature - Sliver
	  Whenever a Sliver creature you control becomes the target of a spell or ability an opponent controls,
	  counter that spell or ability unless its controller pays 2.
	  1/1
	*/
	if( is_humiliated(player, card) ){
		return 0;
	}

	if (trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		trigger_cause_controller == 1-player
	  ){
		card_instance_t *instance = get_card_instance(trigger_cause_controller, trigger_cause);
		int trig = 0;
		int i;
		int t_card = -1;
		for(i=0; i<instance->number_of_targets; i++){
			if( instance->targets[i].player == player && instance->targets[i].card != -1 ){
				if( has_subtype(instance->targets[i].player, instance->targets[i].card, SUBTYPE_SLIVER) ){
					t_card = instance->targets[i].card;
					trig = 1;
					break;
				}
			}
		}
		if (event == EVENT_TRIGGER){
			event_result = trig ? RESOLVE_TRIGGER_MANDATORY : 0;
		}
		else if( event == EVENT_RESOLVE_TRIGGER ){
				if (has_mana(1-player, COLOR_ANY, 2) && do_dialog(1-player, player, t_card, player, t_card, " Pay 2\n Decline", 0) == 0){
					ldoubleclicked = 0;
					charge_mana(1-player, COLOR_COLORLESS, 2);
					if (spell_fizzled != 1){
						return 0;
					}
				}
				real_counter_a_spell(player, t_card, trigger_cause_controller, trigger_cause);
		}
	}
	return 0;
}

int card_ensoul_artifact(int player, int card, event_t event){
	/*
	  Ensoul Artifact |1|U
	  Enchantment - Aura
	  Enchant artifact
	  Enchanted artifact is a creature with base power and toughness 5/5 in addition to its other types.
	*/

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = player;

	return generic_animating_aura(player, card, event, &td, "TARGET_ARTIFACT", 5, 5, 0, 0);
}


int card_frost_lynx(int player, int card, event_t event){
	// unconfirmed
	// Frost Lynx 2U
	// Creature - Elemental Cat (C)
	// When Frost Lynx enters the battlefield, tap target creature an opponent controls. It doesn't untap during its controller's next untap step.
	// 2/2

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		card_instance_t *instance= get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}


	return 0;
}

int card_glacial_crasher(int player, int card, event_t event){
	/*
	  Glacial Crasher |4|U|U
	  Creature - Elemental
	  Trample
	  Glacial Crasher can't attack unless there is a Mountain on the battlefield.
	  5/5
	*/
	if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( ! check_battlefield_for_subtype(ANYBODY, TYPE_LAND, SUBTYPE_MOUNTAIN) ){
			event_result = 1;
		}
	}

	return 0;
}

int card_jalira_master_polymorphogist(int player, int card, event_t event){
	// unconfirmed
	// Jalira, Master Polymorphogist 3U
	// Legendary Creature - Human Wizard (R)
	// 3U, T, Sacrifice another creature: Reveal cards from the top of your library until you reveal a nonlegendary creature card.
	//	Put that card onto the battlefield and the rest on the the bottom of your library in a random order.
	// 2/2
	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_CREATURE, MANACOST_XU(3, 1), 0, NULL, NULL) ){
			if( count_subtype(player, TYPE_CREATURE, -1) > 1 ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XU(3, 1)) ){
			state_untargettable(player, card, 1);
			if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				tap_card(player, card);
			}
			state_untargettable(player, card, 0);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			int count = 0;
			int good = 0;
			while( deck[count] != -1 ){
				if( is_what(-1, deck[count], TYPE_CREATURE) && ! is_legendary(-1, deck[count]) ){
					good = 1;
					break;
				}
				count++;
			}
			show_deck( player, deck, count+1, "Cards revealed by Jallax", 0, 0x7375B0 );
			if( good ){
				put_into_play_a_card_from_deck(player, player, count);
				count--;
			}
			if( count > -1 ){
				put_top_x_on_bottom_in_random_order(player, count+1);
			}
		}
	}

	return 0;
}

int card_jace_the_living_guildpact(int player, int card, event_t event)
{
	/*
	  Jace, the Living Guildpact |2|U|U
	  Planeswalker - Jace
	  +1: Look at the top two cards of your library. Put one of them into your graveyard.
	  -3: Return another target nonland permanent to its owner's hand.
	  -8: Each player shuffles his or her hand and graveyard into his or her library. You draw seven cards.
	  5
	*/

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.special = TARGET_SPECIAL_NOT_ME;

		enum{
			CHOICE_LOOK_2_CARDS = 1,
			CHOICE_BOUNCE_PERMANENT,
			CHOICE_DRAW_SEVEN
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Check the first two cards of your deck", 1, 5, 1,
						"Bounce another permanent", can_target(&td), count_counters(player, card, COUNTER_LOYALTY) > 3 ? 15 : 1, -3,
						"Reshuffle all and draw 7", 1, count_counters(player, card, COUNTER_LOYALTY) > 8 ? 20 : 1, -8);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  switch (choice)
			{
			  case CHOICE_LOOK_2_CARDS:
					break;

			  case CHOICE_BOUNCE_PERMANENT:
					instance->number_of_targets = 0;
					if( select_target(player, card, &td, "Select another target permanent.", NULL) ){
						instance->number_of_targets = 1;
					}
					else{
						spell_fizzled = 1;
					}
					break;


			  case CHOICE_DRAW_SEVEN:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_LOOK_2_CARDS:
				{
					if( count_deck(player) ){
						test_definition_t test;
						new_default_test_definition(&test, TYPE_ANY, "Select a card to put into grave");
						test.create_minideck = 2;
						test.no_shuffle = 1;
						new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GRAVE, 1, AI_MIN_VALUE, &test);
					}
				}
				break;

			case CHOICE_BOUNCE_PERMANENT:
				{
					if( valid_target(&td) ){
						action_on_target(player, card, 0, ACT_BOUNCE);
					}
				}
				break;

			case CHOICE_DRAW_SEVEN:
				{
					int i;
					for(i=0; i<2; i++){
						int p = i == 0 ? player : 1-player;
						reshuffle_grave_into_deck(p, 1);
						reshuffle_hand_into_deck(p, 1);
						shuffle(p);
						if( p == player ){
							draw_cards(p, 7);
						}
					}
				}

			  break;
		  }
	}

  return planeswalker(player, card, event, 5);
}

int card_jorubai_murk_lurker(int player, int card, event_t event){
	/*
	  Jorubai Murk Lurker |2|U
	  Creature - Leech
	  Jorubai Murk Lurker gets +1/+1 as long as you control a Swamp.
	  1B: Target creature gains lifelink until end of turn.
	  1/3
	*/
	uthden_creature(player, card, event, COLOR_BLACK);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	return vanilla_creature_pumper(player, card, event, MANACOST_XB(1, 1), 0, 0, 0, 0, SP_KEYWORD_LIFELINK, &td);
}

int card_kapsho_kitefin(int player, int card, event_t event){
	/*
	  Kapsho Kitefins |4|U|U
	  Creature - Fish
	  Flying
	  Whenever Kapsho Kitefins or another creature enters the battlefield under your control, tap target creature an opponent controls.
	  3/3
	*/
	if( specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			get_card_instance(player, card)->number_of_targets = 1;
			action_on_target(player, card, 0, ACT_TAP);
		}
	}

	return 0;
}

int card_master_of_predicaments(int player, int card, event_t event){
	/*
	  Master of Predicaments |3|U|U
	  Creature - Sphinx
	  Flying
	  Whenever Master of Predicaments deals combat damage to a player, choose a card in your hand.
	  That player guesses whether the card's converted mana cost is greater than 4.
	  If the player guessed wrong, you may cast the card without paying its mana cost.
	  4/4
	*/
	if( hand_count[player] > 0 && damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select a card.");
		int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MAX_VALUE, -1, &test);
		int choice = do_dialog(1-player, player, card, player, card, " CMC > 4\n CMC <=4", get_average_cmc(player)<= 4);
		if( (choice == 0 && get_cmc(player, selected) <= 4) || (choice == 1 && get_cmc(player, selected) > 4) ){
			play_card_in_hand_for_free(player, selected);
		}
	}

	return 0;
}

int effect_mercurial_pretender(int player, int card, event_t event)
{
  if (!IS_GAA_EVENT(event))
	return 0;

  card_instance_t* inst = get_card_instance(player, card);
  if (event == EVENT_RESOLVE_ACTIVATION && in_play(inst->damage_target_player, inst->damage_target_card))
	bounce_permanent(inst->damage_target_player, inst->damage_target_card);

  return generic_activated_ability(inst->damage_target_player, inst->damage_target_card, event, 0, MANACOST_XU(2, 2), 0, NULL, NULL);
}

int card_mercurial_pretender(int player, int card, event_t event)
{
  /* Mercurial Pretender	|4|U
   * Creature - Shapeshifter 0/0
   * You may have ~ enter the battlefield as a copy of any creature you control except it gains "|2|U|U: Return this creature to its owner's hand." */

  target_definition_t td;
  if (event == EVENT_RESOLVE_SPELL)
	{
	  base_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = td.preferred_controller = player;
	}
  if (enters_the_battlefield_as_copy_of_any(player, card, event, &td, "SELECT_A_CREATURE_YOU_CONTROL"))
	set_legacy_image(player, CARD_ID_MERCURIAL_PRETENDER, create_targetted_legacy_activate(player, card, effect_mercurial_pretender, player, card));

  return 0;
}

int card_military_intelligence(int player, int card, event_t event){
	// unconfirmed
	// Military Intelligence 1U
	// Enchantment (U)
	// Whenever you attack with two or more creatures, draw a card.
	if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS){
		if( ! is_humiliated(player, card) ){
			if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, -1) > 1 ){
				draw_cards(player, 1);
			}
		}
	}

	return global_enchantment(player, card, event);
}

/*
  Nimbus of the Isles |4|U --> vanilla
  Creature - Elemental
  Flying
  3/3
*/

int card_paragon_of_gathering_mists(int player, int card, event_t event){
	// unconfirmed
	// Creature - Human Wizard (U)
	// Other blue creatures you control get +1/+1.
	// U, T: Another target blue creature you control gains flying until end of turn.
	// 2/2
	return m15_paragon(player, card, event, COLOR_TEST_BLUE, 0, 0, KEYWORD_FLYING, 0);
}

int card_polymorphists_jest(int player, int card, event_t event){
	/*
	  Polymorphist's Jest |1|U|U --> Sudden Spoiling
	  Instant
	  Until end of turn, each creature target player controls loses all abilities and becomes a blue Frog with base power and toughness 1/1.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td)){
			int p = instance->targets[0].player;

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			test_definition_t hc;
			default_test_definition(&hc, 0);
			hc.power = 1;
			hc.toughness = 1;
			hc.color = COLOR_TEST_BLUE;
			hc.subtype = SUBTYPE_FROG;

			int c;
			for (c = active_cards_count[p]-1; c > -1; c--){
				if (in_play(p, c) && is_what(p, c, TYPE_CREATURE)){
					force_a_subtype(p, c, SUBTYPE_FROG);
					humiliate_and_set_pt_abilities(player, card, p, c, 4, &hc);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_quickling(int player, int card, event_t event){
	// unconfirmed
	// Quickling 1U
	// Creature - Faerie Rogue (U)
	// Flying
	// Flash
	// When Quickling enters the battlefield, sacrifice it unless you return another creature you control to its owner's hand.
	// 2/2
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME;

		card_instance_t *instance= get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}


	return flash(player, card, event);
}

int card_research_assistant(int player, int card, event_t event){
	/*
	  Research Assistant |1|U
	  Creature - Human Wizard
	  3U, T: Draw a card, then discard a card.
	  1/3
	*/

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XU(3, 1), 0, NULL, NULL);
}

static int get_num_colors_of_permanents(int player){
	int count = 0;
	int clr = 0;
	while( count < active_cards_count[player] && num_bits_set(clr) < 5 ){
			if( in_play(player, count) && is_what(player, count, TYPE_PERMANENT) ){
				int this_clr = get_color(player, count);
				if( this_clr != COLOR_TEST_COLORLESS ){
					clr |= this_clr;
				}
			}
			count++;
	}
	return num_bits_set(clr);
}

int card_soul_of_ravnica(int player, int card, event_t event){
	// unconfirmed
	// Soul of Ravnica 4UU
	// Creature - Avatar (M)
	// Flying
	// 5UU: Draw a card for each colour among permanents you control.
	// 5UU, Exile Soul of Ravnica from your graveyard: Draw a card for each colour among permanents you control.
	// 6/6

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XU(5, 2)) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_multi(player, MANACOST_XU(5, 2)) ){
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION || event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		draw_cards(player, get_num_colors_of_permanents(player));
	}
	return generic_activated_ability(player, card, event, 0, MANACOST_XU(5, 2), 0, NULL, NULL);
}

int card_statute_of_denial(int player, int card, event_t event){
	/*
	  Statute of Denial |2|U|U
	  Instant
	  Counter target spell. If you control a blue creature, draw a card, then discard a card.
	*/
	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return card_counterspell(player, card, event);
	}
	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t* instance = get_card_instance(player, card);
		if (counterspell_validate(player, card, NULL, 0)){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
			if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
				draw_cards(player, 1);
				discard(player, 0, player);
			}
		}
		instance->number_of_targets = 0;
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_void_snare(int player, int card, event_t event){
	// unconfirmed
	// Void Snare U
	// Sorcery (C)
	// Return target nonland permanent to its owner's hand.

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
}

/********
 * Black *
 ********/

int card_blood_host(int player, int card, event_t event){
	/*
	  Blood Host |3|B|B
	  Creature - Vampire
	  1B, Sacrifice another creature: Put a +1/+1 counter on Blood Host and you gain 2 life.
	  3/3
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_counter(instance->parent_controller, instance->parent_card, COUNTER_P1_P1);
		gain_life(player, 2);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_NOT_ME_AS_TARGET, MANACOST_XB(1, 1), 0, NULL, NULL);
}

int card_carrion_crow(int player, int card, event_t event){
	/*
	  Carrion Crow |2|B
	  Creature - Zombie Bird
	  Flying
	  Carrion Crow enters the battlefield tapped.
	  2/2
	*/
	comes_into_play_tapped(player, card, event);
	return 0;
}

int card_covenant_of_blood(int player, int card, event_t event){
	/*
	  Covenant of Blood 6B
	  Sorcery (C)
	  Convoke
	  Covenant of Blood deals 4 damage to target creature or player and you gain 4 life.
	*/

	if( event == EVENT_MODIFY_COST){
		return generic_spell_with_convoke(player, card, event);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = cast_spell_with_convoke(player, card, event);
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		td.allow_cancel = result & (1<<31) ? 0 : 1;
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 4);
			gain_life(player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cruel_sadist(int player, int card, event_t event){
	// unconfirmed
	// Cruel Sadist B
	// Craeture - Human Assassin (R)
	// B, T, Pay 1 life: Put a +1/+1 counter on Cruel Sadist.
	// 2B, T, Remove X +1/+1 counters from Cruel Sadist: Crusel Sadist deals X damage to target creature.
	// 1/1

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_B(1), 1, NULL, NULL);
		result |= generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_1_1_COUNTER, MANACOST_XB(2, 1), 0, &td, "TARGET_CREATURE");
		return result;
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		instance->number_of_targets = 0;
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_B(1), 1, NULL, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_1_1_COUNTER, MANACOST_XB(2, 1), 0, &td, "TARGET_CREATURE")){
				target_definition_t td1;
				default_target_definition(player, card, &td1, TYPE_CREATURE);
				td1.toughness_requirement = count_1_1_counters(player, card);
				choice = do_dialog(player, card, player, -1, -1, " Add a +1/+1 counter\n Damage creature\n Cancel", can_target(&td1) ? 1 : 0);
			}
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XB(choice*2, 1)) ){
			if( choice == 0 ){
				lose_life(player, 1);
			}
			if( choice == 1 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					int amount = get_toughness(instance->targets[0].player, instance->targets[0].card) > count_1_1_counters(player, card) ?
						count_1_1_counters(player, card) : get_toughness(instance->targets[0].player, instance->targets[0].card);
					if( player == HUMAN && count_1_1_counters(player, card) > 1){
						amount = choose_a_number(player, "How many +1/+1 counters you'll remove ?", count_1_1_counters(player, card));
						if( amount < 1 || amount > count_1_1_counters(player, card) ){
							spell_fizzled = 1;
							return 0;
						}
					}
					remove_1_1_counters(player, card, amount);
					instance->targets[1].player = amount;
				}
			}
			if( spell_fizzled != 1 ){
				tap_card(player, card);
				instance->info_slot |= 1<<(30+choice);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot & (1<<30) ){
			add_1_1_counter(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot & (1<<31) && valid_target(&td)){
			damage_target0(player, card, instance->targets[1].player);
		}
	}
	return 0;
}

int card_endless_obedience(int player, int card, event_t event){
	/*
	  Endless Obedience |4|B|B
	  Sorcery
	  Convoke
	  Put target creature card from a graveyard onto the battlefield under your control.
	*/

	if( event == EVENT_MODIFY_COST){
		return generic_spell_with_convoke(player, card, event);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && any_in_graveyard_by_type(ANYBODY, TYPE_CREATURE) ){
		return ! graveyard_has_shroud(2);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = cast_spell_with_convoke(player, card, event);
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		int mode = result & (1<<31) ? 0 : 1;
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card.");

		if( select_target_from_either_grave(player, card, mode, AI_MAX_CMC, AI_MAX_CMC, &this_test, 0, 1) != -1 ){
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_eternal_thirst(int player, int card, event_t event){
	/*
	  Eternal Thirst |1|B
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature has lifelink and "Whenever a creature an opponent controls dies, put a +1/+1 counter on this creature."
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			count_for_gfp_ability(player, card, event, 1-player, TYPE_CREATURE, 0);
		}

		if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
			add_counters(instance->damage_target_player, instance->damage_target_card, COUNTER_P1_P1, instance->targets[11].card);
			instance->targets[11].card = 0;
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, SP_KEYWORD_LIFELINK, 0, 0, 0);
}

int card_feast_on_the_fallen(int player, int card, event_t event){

	if( get_card_instance(player, card)->info_slot ){
		upkeep_trigger_ability(player, card, event, ANYBODY);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance( player, card );

		instance->number_of_targets = 0;

		if( can_target(&td) && new_pick_target(&td, "Select target creature you control.", 0, GS_LITERAL_PROMPT) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	if( event == EVENT_CLEANUP ){
		get_card_instance(player, card)->info_slot = get_trap_condition(1-player, TRAP_LIFE_LOST);
	}

	return global_enchantment(player, card, event);
}

int card_festergloom(int player, int card, event_t event ){
	/*
	  Festergloom |2|B
	  Sorcery
	  Nonblack creatures get -1/-1 until end of turn.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		test.color_flag = DOESNT_MATCH;
		pump_creatures_until_eot(player, card, ANYBODY, 0, -1, -1, 0, 0, &test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_flesh_to_dust(int player, int card, event_t event){
	// unconfirmed
	// Flesh to Dust 3BB
	// Instant (C)
	// Destroy target creature. It can't be regenerated.
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_in_garruks_wake(int player, int card, event_t event){
	/*
	  In Garruk's Wake |7|B|B
	  Sorcery
	  Destroy all creatures you don't control and all planeswalkers you don't control.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE | TARGET_TYPE_PLANESWALKER);
		new_manipulate_all(player, card, 1-player, &test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_indulgent_tormenter(int player, int card, event_t event){
	// unconfirmed
	// Indulgent Tormenter 3BB
	// Creature - Demon (R)
	// Flying
	// At the beginning of your upkeep, draw a card unless target opponent sacrifices a creature or pays 3 life.
	// 5/3

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		int choice = 1;
		if( would_validate_arbitrary_target(&td, 1-player, -1) ){
			choice = DIALOG(player, card, EVENT_ACTIVATE, DLG_WHO_CHOOSES(1-player), DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_RANDOM,
							"Opponent draw 1 card", 1, 1,
							"Sac a creature", count_subtype(1-player, TYPE_CREATURE, -1), 10+(5*(count_subtype(1-player, TYPE_CREATURE, -1)-count_subtype(player, TYPE_CREATURE, -1))),
							"Pay 3 life", can_pay_life(1-player, 3), 5+(2*(life[1-player]-life[player])));
		}
		if( choice == 2 ){
			if( ! sacrifice(player, card, 1-player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				choice = 1;
			}
		}
		if( choice == 3 ){
			lose_life(1-player, 3);
		}
		if( choice == 1 ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_leeching_sliver(int player, int card, event_t event){
	/*
	  Leeching Sliver |1|B
	  Creature - Sliver
	  Whenever a Sliver you control attacks, defending player loses 1 life.
	  1/1
	*/
	if( ! is_humiliated(player, card) ){
		if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS){
			test_definition_t test;
			default_test_definition(&test, TYPE_CREATURE);
			test.subtype = SUBTYPE_SLIVER;

			int amount = declare_attackers_trigger_test(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, -1, &test);
			if( amount ){
				lose_life(1-player, amount);
			}
		}
	}

	return slivercycling(player, card, event);
}

int card_necromancers_assistant(int player, int card, event_t event)
{
  /* Necromancer's Assistant	|2|B UNCONFIRMED (not yet on wizards.com; DOTP screenshot, not m15 expansion symbol)
   * Creature - Zombie 3/1
   * When ~ enters the battlefield, put the top three cards of your library into your graveyard. */

  if (comes_into_play(player, card, event))
	mill(player, 3);

  return 0;
}

int card_necromancers_stockpile(int player, int card, event_t event){
	/*
	  Necromancer's Stockpile |1|B
	  Enchantment
	  1B, Discard a creature card: Draw a card. If the discarded card was a Zombie card, put a 2/2 black Zombie creature token onto the battlefield tapped.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if (IS_ACTIVATING(event)){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		this_test.zone = TARGET_ZONE_HAND;

		if( event == EVENT_CAN_ACTIVATE ){
			return CAN_ACTIVATE(player, card, MANACOST_XB(1, 1)) && check_battlefield_for_special_card(player, card, player, 0, &this_test);
		}

		if( event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST_XB(1, 1)) ){
			instance->info_slot = 1;
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				instance->info_slot = has_subtype(player, selected, SUBTYPE_ZOMBIE);
				discard_card(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			draw_cards(player, 1);

			if( instance->info_slot ){
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
				token.action = TOKEN_ACTION_TAPPED;
				generate_token(&token);
			}
		}
   }

   return global_enchantment(player, card, event);
}

int card_nightfire_giant(int player, int card, event_t event){
	/*
	  Nightfire Giant |4|B
	  Creature - Zombie Giant
	  Nightfire Giant gets +1/+1 as long as you control a Mountain.
	  4R: Nightfire Giant deals 2 damage to target creature or player.
	  4/3
	*/
	uthden_creature(player, card, event, COLOR_RED);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		damage_target0(player, card, 2);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(4, 1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}


int card_ob_nixilis_unshackled(int player, int card, event_t event){
	/*
	  Ob Nixilis, Unshackled |4|B|B
	  Legendary Creature - Demon
	  Flying, trample
	  Whenever an opponent searches his or her library, that player sacrifices a creature and loses 10 life.
	  Whenever another creature dies, put at +1/+1 counter on Ob Nixilis, Unshackled.
	  4/4
	*/

	check_legend_rule(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counters(player, card, get_card_instance(player, card)->targets[11].card);
		get_card_instance(player, card)->targets[11].card = 0;
	}

	return 0;
}

int card_paragon_of_open_graves(int player, int card, event_t event){
	// unconfirmed
	// Paragon of Open Graves 3B
	// Creature - Skeleton Warrior (U)
	// Other black creatures you control get +1/+1.
	// 2B, T: Another target black creature you control gains deathtouch until end of turn.
	// 2/2
	return m15_paragon(player, card, event, COLOR_TEST_BLACK, 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
}

int card_rotfeaster_maggot(int player, int card, event_t event)
{
  /* Rotfeaster Maggot	|4|B
   * Creature - Insect 3/5
   * When ~ enters the battlefield, exile target creature card from a graveyard. You gain life equal to that card's toughness. */

  if (comes_into_play(player, card, event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select target creature card.");

	  int iid = select_target_from_either_grave(player, card, SFG_CANNOT_CANCEL, AI_MAX_CMC, AI_MAX_CMC, &test, 0, 1);
	  if (iid == -1)
		return 0;

	  card_instance_t* inst = get_card_instance(player, card);
	  int whose = inst->targets[0].player;
	  int pos = inst->targets[1].player;
	  int tgh = get_base_toughness_iid(whose, get_grave(whose)[pos]);
	  rfg_card_from_grave(whose, pos);
	  gain_life(player, tgh);
	}

  return 0;
}

int card_shadowcloak_vampire(int player, int card, event_t event){
	// unconfirmed
	// Shadowcloak Vampire 4B
	// Creature - Vampire (C)
	// Pay 2 life: Shadowcloak Vampire gains flying until end of turn.
	// 4/3
	return generic_shade(player, card, event, 2, MANACOST0, 0, 0, KEYWORD_FLYING, 0);
}

int card_soul_of_innistrad(int player, int card, event_t event){
	/*
	  Soul of Innistrad |4|B|B
	  Creature - Avatar
	  Deathtouch
	  3BB: Return up to three target creature cards from your graveyard to your hand.
	  3BB, Exile Soul of Innistrad from your graveyard: Return up to three target creature cards from your graveyard to your hand.
	  6/6
	*/
	deathtouch(player, card, event);

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XB(3, 2)) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_multi(player, MANACOST_XB(3, 2)) ){
			return GAPAID_EXILE;
		}
	}

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XB(3, 2), 0, NULL, NULL) ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player) ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XB(3, 2)) ){
			select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, NULL, 3, &instance->targets[1]);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for (i = 1; i <= 3; ++i){
			if (instance->targets[i].player != -1){
				int selected = validate_target_from_grave(player, card, player, i);
				if (selected != -1){
					from_grave_to_hand(player, selected, TUTOR_HAND);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		if( count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player) ){
			select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, NULL, 3, &instance->targets[1]);
			int i;
			for (i = 1; i <= 3; ++i){
				if (instance->targets[i].player != -1){
					int selected = validate_target_from_grave(player, card, player, i);
					if (selected != -1){
						from_grave_to_hand(player, selected, TUTOR_HAND);
					}
				}
			}
		}
	}

	return 0;
}

int card_stain_the_mind(int player, int card, event_t event){
	/*
	  Stain the Mind |4|B
	  Sorcery
	  Convoke
	  Name a nonland card. Search target player's graveyard, hand, and library for any number of card's with that name and exile them.
	  Then that player shuffles his or her library.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST){
		return generic_spell_with_convoke(player, card, event);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = cast_spell_with_convoke(player, card, event);
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		td.allow_cancel = result & (1<<31) ? 0 : 1;
		if( pick_target(&td, "TARGET_PLAYER") ){
			instance->info_slot = 0;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
			int opponent = instance->targets[0].player;
			int id = -1;
			int card_selected  = -1;
			if( player != AI ){
				if( ai_is_speculating != 1 ){
					while(1){
						card_selected = choose_a_card("Choose a card", -1, -1);
						if( ! is_what(-1, card_selected, TYPE_LAND) && is_valid_card(cards_data[card_selected].id)){
							id = cards_data[card_selected].id;
							break;
						}
					}
				}
			}
			else{
				 int count = count_deck(opponent)-1;
				 int *deck = deck_ptr[opponent];
				 while( count > -1 ){
						if( deck[count] != -1 && ! is_what(-1, deck[count], TYPE_LAND) ){
							id = cards_data[deck[count]].id;
							break;
						}
						count--;
				}
			}

			if( id != -1 && player == AI ){
				char buffer[300];
				int pos = scnprintf(buffer, 300, "Opponent named:");
				card_ptr_t* c_me = cards_ptr[ id ];
				pos += scnprintf(buffer+pos, 300-pos, " %s", c_me->name);
				do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
			}

			if( id > -1 ){
				lobotomy_effect(player, opponent, id, 0);
			}

			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_ulcerate(int player, int card, event_t event){
	// unconfirmed
	// Ulcerate B
	// Instant (U)
	// Target creature gets -3/-3 until end of turn. You lose 3 life.
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -3, -3);
			lose_life(player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_unmake_the_graves(int player, int card, event_t event){
	/*
	  Unmake the Graves |4|B
	  Instant
	  Convoke
	  Return up to two target creature cards from your graveyard to your hand.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST){
		return generic_spell_with_convoke(player, card, event);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t test;
	new_default_test_definition(&test, TYPE_CREATURE, "Select up to two target creature cards.");

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( cast_spell_with_convoke(player, card, event) ){
			instance->info_slot = 0;
			return spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(player, card, event, 2, &test, 0);
		}
	}

	if( event == EVENT_CAN_CAST || event == EVENT_RESOLVE_SPELL ){
		return spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(player, card, event, 2, &test, 0);
	}

	return 0;
}

int card_wall_of_limbs(int player, int card, event_t event){
	/*
	  Wall of Limbs |2|B
	  Creature - Zombie Wall
	  Defender
	  Whenever you gain life, put a +1/+1 counter on Wall of Limbs.
	  5BB, Sacrifice Wall of Limbs: Target player loses X life, where X is Wall of Limbs's power.
	  0/3
	*/
	if (trigger_gain_life(player, card, event, player, RESOLVE_TRIGGER_DUH)){
		add_1_1_counter(player, card);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST_XB(5, 2), 0, &td1, "TARGET_PLAYER");
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XB(5, 2)) ){
			if( pick_target(&td1, "TARGET_PLAYER") ){
				instance->info_slot = get_power(player, card);
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td1) ){
		lose_life(instance->targets[0].player, instance->info_slot);
	}

	return 0;
}

int card_waste_not(int player, int card, event_t event)
{
  /* Waste Not	|1|B
   * Enchantment
   * Whenever an opponent discards a creature card, put a 2/2 |Sblack Zombie creature token onto the battlefield.
   * Whenever an opponent discards a land card, add |B|B to your mana pool.
   * Whenever an opponent discards a noncreature, nonland card, draw a card. */

  if (discard_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, 0))
	{
	  int creature = is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE);
	  int land = is_what(trigger_cause_controller, trigger_cause, TYPE_LAND);

	  if (creature)
		generate_token_by_id(player, card, CARD_ID_ZOMBIE);

	  if (land)
		produce_mana(player, COLOR_BLACK, 2);

	  if (!creature && !land)
		draw_a_card(player);
	}

  return global_enchantment(player, card, event);
}

int card_zof_shade(int player, int card, event_t event){
	/*
	  Zof Shade |3|B
	  Creature - Shade
	  2B: Zof Shade gets +2/+2 until end of turn.
	  2/2
	*/
	return generic_shade(player, card, event, 0, MANACOST_XB(2, 1), 2, 2, 0, 0);
}

/*
Witch's Familiar |2|B --> vanilla
Creature - Frog
2/3
*/

int card_xathrid_slyblade(int player, int card, event_t event){
	/*
	  Xathrid Slyblade |2|B
	  Creature - Human Assassin
	  Hexproof
	  3B: Until end of turn, Xathrid Slyblade loses hexproof and gains first strike and deathtouch.
	  2/1
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( instance->targets[1].player != 66 ){
		hexproof(player, card, event);
	}

	int flags = player == AI ? GAA_ONCE_PER_TURN : 0;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		get_card_instance(instance->parent_controller, instance->parent_card)->targets[1].player = 66;
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								0, 0, KEYWORD_FIRST_STRIKE, SP_KEYWORD_DEATHTOUCH);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return generic_activated_ability(player, card, event, flags, MANACOST_XB(3, 1), 0, NULL, NULL);
}


/******
* Red *
******/

int card_act_on_impulse(int player, int card, event_t event)
{
	/*
	  Act on Impulse |2|R
	  Sorcery
	  Exile the top three cards of your library. Until end of turn, you may play cards exiled this way.
	*/

	if(event == EVENT_RESOLVE_SPELL){
		int i;
		for (i = 0; i < 3; ++i){
			if (deck_ptr[player][0] != -1){
				int csvid = cards_data[deck_ptr[player][0]].id;
				rfg_top_card_of_deck(player);
				create_may_play_card_from_exile_effect(player, card, player, csvid, MPCFE_UNTIL_EOT);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_aggressive_mining(int player, int card, event_t event){
	/*
	  Aggressive Mining |3|R
	  Enchantment
	  You can't play lands.
	  Sacrifice a land: Draw two cards. Activate this ability only once each turn.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier-=(60-(count_subtype(player, TYPE_LAND, -1)*10));
	}

	if( event == EVENT_CAN_ACTIVATE && instance->targets[1].player != 66 ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				instance->targets[1].player = 66;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 2);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	if( in_play(player, card) ){
		player_bits[player] |= PB_COUNT_TOTAL_PLAYABLE_LANDS;

		if( current_turn == player && !(land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED) ){
			land_can_be_played |= LCBP_LAND_HAS_BEEN_PLAYED;
		}
	}

	if( leaves_play(player, card, event) && lands_played < total_playable_lands(player) ){
		land_can_be_played &= ~LCBP_LAND_HAS_BEEN_PLAYED;
		player_bits[player] &= ~PB_COUNT_TOTAL_PLAYABLE_LANDS;
	}

	return global_enchantment(player, card, event);
}

int card_altac_bloodseeker(int player, int card, event_t event){
	/*
	  Altac Bloodseeker |1|R
	  Creature - Human Berserker
	  Whenever a creature an opponent controls dies, Altac Bloodseeker gets +2/+0 and gains first strike and haste until end of turn.
	  2/1
	*/
	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, 1-player, TYPE_CREATURE, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		pump_ability_until_eot(player, card, player, card, 2*get_card_instance(player, card)->targets[11].card, 0, KEYWORD_FIRST_STRIKE, SP_KEYWORD_HASTE);
		get_card_instance(player, card)->targets[11].card = 0;
	}

	return 0;
}

int card_belligerent_sliver(int player, int card, event_t event){
	/* Belligerent Sliver	|2|R
	 * Creature - Sliver 2/2
	 * Sliver creatures you control have menace. */

	if( event == EVENT_DECLARE_BLOCKERS && current_turn == player && ! is_humiliated(player, card) ){
		int c;
		for (c = 0; c < active_cards_count[player]; ++c){
			if (in_play(player, c) && has_subtype(player, c, SUBTYPE_SLIVER)){
				menace(player, c, event);
			}
		}
	}

	return 0;
}

int card_blastfire_bolt(int player, int card, event_t event){
	/*
	  Blastfire Bolt |5|R
	  Instant
	  Blastfire Bolt deals 5 damage to target creature. Destroy all Equipment attached to that creature.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if (event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		card_instance_t *instance= get_card_instance(player, card);
		equipments_attached_to_me(instance->targets[0].player, instance->targets[0].card, EATM_DESTROY);
		damage_target0(player, card, 5);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/*
Borderland Marauder |1|R --> Lurking Nightstalker
Creature - Human Warrior
Whenever Borderland Marauder attacks, it gets +2/+0 until end of turn.
1/2
*/

int card_brood_keeper(int player, int card, event_t event){
	// unconfirmed
	// Brood Keeper 3R
	// Creature - Human Shaman (U)
	// Whenever an Aura becomes attached to Brood Keeper, put a 2/2 red Dragon creature token with flying onto the battlefield. It has "R: This creature gets +1/+0 until end of turn".
	// 2/3

	if( aura_attached_to_me(player, card, event, RESOLVE_TRIGGER_MANDATORY, NULL) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_DRAGON, &token);
		token.pow = 2;
		token.tou = 2;
		token.special_infos = 67; //"R: This creature gets +1/+0 until end of turn".
		token.legacy = 1;
		token.special_code_for_legacy = &empty;
		generate_token(&token);
	}

	return 0;
}

int card_burning_anger(int player, int card, event_t event){
	// unconfirmed
	// Burning Anger 4R
	// Enchantment - Aura (R)
	// Enchant creature
	// Enchant creature has "T: This creature deals damage equal to its power to target creature or player.

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1){
		if( (event >= EVENT_CAN_MOVE_AURA && event <= EVENT_RESOLVE_MOVING_AURA) || event == EVENT_RESOLVE_SPELL ){
			return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td1) ){
			damage_target0(player, card, get_power(instance->damage_target_player, instance->damage_target_card));
		}

		return granted_generic_activated_ability(player, card, instance->damage_target_player, instance->damage_target_card, event,
												 GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td1, "TARGET_CREATURE_OR_PLAYER");
	}

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

int card_cone_of_flame(int player, int card, event_t event){
	/*
	  Cone of Flame |3|R|R
	  Sorcery
	  Cone of Flame deals 1 damage to target creature or player, 2 damage to another target creature or player, and 3 damage to a third target creature or player.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<3; i++){
			if( validate_target(player, card, &td, i) ){
				damage_creature(instance->targets[i].player, instance->targets[i].card, i+1, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 3, NULL);
}

int card_crowds_favor(int player, int card, event_t event){
	/*
	  Crowd's Favor |R
	  Instant
	  Convoke
	  Target creature gets +1/+0 and gains first strike until end of turn.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST){
		return generic_spell_with_convoke(player, card, event);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = cast_spell_with_convoke(player, card, event);
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		td.allow_cancel = result & (1<<31) ? 0 : 1;
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = 0;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 0, KEYWORD_FIRST_STRIKE, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_generator_servant(int player, int card, event_t event){
	/*
	  Generator Servant |1|R
	  Creature - Elemental
	  T, Sacrifice Generator Servant: Add 2 to your mana pool. If that mana is spent on a creature spell, it gains haste until end of turn.
	  2/1
	*/

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_sacrifice_this_as_cost(player, card) ){
			return mana_producer(player, card, event);
		}
	}

	if( event == EVENT_ACTIVATE ){
		tap_card(player, card);
		produce_mana(player, COLOR_COLORLESS, 2);
		kill_card(player, card, KILL_SACRIFICE);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_sacrifice_this_as_cost(player, card) && mana_producer(player, card, EVENT_CAN_ACTIVATE)){
			declare_mana_available(player, COLOR_COLORLESS, 2);
		}
	}

	return 0;
}

int card_land_mine(int player, int card, event_t event){
	/*
	  Land Mine |0
	  Artifac
	  R, Sacrifice this artifact: This artifact deals 2 damage to target attacking creature without flying.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;
	td.illegal_abilities = KEYWORD_FLYING;

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		damage_target0(player, card, 2);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME | GAA_LITERAL_PROMPT, MANACOST_R(1), 0,
									&td, "Select target attacking creature without flying.");
}

int card_goblin_kaboomist(int player, int card, event_t event){
	/*
	  Goblin Kaboomist |1|R
	  Creature - Goblin Warrior
	  At the beginning of your upkeep, put a colorless artifact token named Land Mine onto the battlefield with
	  "R, Sacrifice this artifact: This artifact deals 2 damage to target attacking creature without flying."
	  Then flip a coin. If you lose the flip, Goblin Kaboomist deals 2 damage to itself.
	  1/2
	*/
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		generate_token_by_id(player, card, CARD_ID_LAND_MINE);
		if( ! flip_a_coin(player, card) ){
			damage_creature(player, card, 2, player, card);
		}
	}

	return 0;
}

int card_goblin_rabblemaster(int player, int card, event_t event){
	// unconfirmed
	// Goblin Rabblemaster 2R
	// Creature - Goblin Warrior (R)
	// Other Goblin creatures you control attack each turn if able.
	// At the beginning of combat on your turn, put a 1/1 red Goblin creature token with haste onto the battlefield.
	// Whenever Goblin Rabblemaster attacks, it gets +1/+0 until end of turn for each other attacking Goblin.
	// 2/2

	event_flags |= EA_FORCE_ATTACK;

	if (event == EVENT_MUST_ATTACK){
		int c;
		for (c = 0; c < active_cards_count[player]; ++c){
			if ( c!= card && in_play(player, c) && is_what(player, c, TYPE_CREATURE) && has_subtype(player, c, SUBTYPE_GOBLIN) ){
				attack_if_able(player, c, event);
			}
		}
	}

	if( beginning_of_combat(player, card, event, player, -1) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GOBLIN, &token);
		token.pow = 1;
		token.tou = 1;
		token.s_key_plus = SP_KEYWORD_HASTE;
		generate_token(&token);
	}

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_GOBLIN;
		this_test.state = STATE_ATTACKING;
		this_test.not_me = 1;
		pump_until_eot(player, card, player, card, check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test), 0);
	}

	return 0;
}

int card_hammerhand(int player, int card, event_t event){
	/*
	  Hammerhand |R
	  Enchantment - Aura
	  Enchant creature
	  When Hammerhand enters the battlefield, target creature can't block this turn.
	  Enchanted creature gets +1/+1 and has haste.
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}

	return generic_aura(player, card, event, player, 1, 1, 0, SP_KEYWORD_HASTE, 0, 0, 0);
}

int card_heat_ray(int player, int card, event_t event){
	/*
	  Heat Ray |X|R
	  Instant
	  Heat Ray deals X damage to target creature.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_CREATURE", 1, NULL);
}

int card_inferno_fist(int player, int card, event_t event){
	// unconfirmed
	// Inferno Fist 1R
	// Enchantment - Aura (C)
	// Enchant creature you control.
	// Enchanted creature gets +2/+0.
	// R, Sacrifice Inferno Fist: Inferno Fist deals 2 damage to target creature or player.
	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1){
		if( IS_GAA_EVENT(event) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

			if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
				damage_target0(player, card, 2);
			}

			return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST_R(1), 0, 	&td, "TARGET_CREATURE_OR_PLAYER");
		}

	}
	return generic_aura(player, card, event, player, 2, 0, 0, 0, 0, 0, 0);
}

int card_kird_chieftain(int player, int card, event_t event){
	/*
	  Kird Chieftain |3|R
	  Creature - Ape
	  Kird Chieftain gets +1/+1 as long as you control a Forest.
	  4G: Target creature gets +2/+2 and gains trample until end of turn.
	  3/3
	*/
	uthden_creature(player, card, event, COLOR_GREEN);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								2, 2, KEYWORD_TRAMPLE, 0);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XG(4, 1), 0, &td, "TARGET_CREATURE");
}

/*
Krenko's Enforcer |1|R|R --> Accursed Spirit
Creature - Goblin Warrior
2/2
*/

/*
Kurkesh, Onakke Ancient |2|R|R --> Impossible
Legendary Creature - Ogre Spirit
Whenever you activate an ability of an artifact, if it isn't a mana ability, you may pay R.
If you do, copy that ability. You may choose new targets for the copy.
4/3
*/

int card_mights_tyranny(int player, int card, event_t event){
	/*
	  Might's Tyrrany |5|R
	  Enchantment
	  At the beginning of combat on your turn, if you control all creatures with the greatest power on the battlefield,
	  gain control of target creature until end of turn. Untap that creature. It gains haste until end of turn.
	*/
	if( beginning_of_combat(player, card, event, player, -1) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		int result = check_battlefield_for_special_card(player, card, ANYBODY, CBFSC_GET_MAX_POW, &test);
		if( result > -1 ){
			test.power = result;
		}
		if( check_battlefield_for_special_card(player, card, player, 0, &test) &&
			! check_battlefield_for_special_card(player, card, 1-player, 0, &test)
		  ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allow_cancel = 0;

			card_instance_t *instance = get_card_instance(player, card);

			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_miners_bane(int player, int card, event_t event){
	/*
	  Miner's Bane |4|R|R
	  Creature - Elemental
	  2R: Miner's Bane gets +1/+0 and gains trample until end of turn.
	  6/3
	*/
	return generic_shade(player, card, event, 0, MANACOST_XR(2, 1), 1, 0, KEYWORD_TRAMPLE, 0);
}

int card_paragon_of_fierce_defiance(int player, int card, event_t event){
	// unconfirmed
	// Paragon of Fierce Defiance 3R
	// Creature - Human Warrior (U)
	// Other red creatures you control get +1/+1.
	// R, T: Another target red creature you control gains haste until end of turn.
	// 2/2
	return m15_paragon(player, card, event, COLOR_TEST_RED, 0, 0, 0, SP_KEYWORD_HASTE);
}

int card_scrapyard_mongrel(int player, int card, event_t event){
	/*
	  Scrapyard Mongrel |3|R
	  Creature - Hound
	  As long as you control an artifact, Scrapyard Mongrel gets +2/+0 and has trample.
	  3/3
	*/
	if( (event == EVENT_POWER || event == EVENT_ABILITIES) && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( check_battlefield_for_subtype(player, TYPE_ARTIFACT, -1) ){
			if( event == EVENT_POWER ){
				event_result+=2;
			}
			else{
				event_result |=  KEYWORD_TRAMPLE;
			}
		}
	}
	return 0;
}

int card_siege_dragon(int player, int card, event_t event){
	// unconfirmed
	// Siege Dragon 5RR
	// Creature - Dragon (R)
	// Flying
	// When Siege Dragon enters the battlefield, destroy all Walls your opponents control.
	// Whenever Siege Dragon attacks, if defending player control no Walls, it deals 2 damage to each creature without flying that player controls.
	// 5/5
	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_WALL;
		new_manipulate_all(player, card, 1-player, &this_test, KILL_DESTROY);
	}

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		if( ! check_battlefield_for_subtype(1-player, TYPE_PERMANENT, SUBTYPE_WALL) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			this_test.keyword_flag = DOESNT_MATCH;
			new_damage_all(player, card, 1-player, 2, 0, &this_test);
		}
	}
	return 0;
}

int card_soul_of_shandalar(int player, int card, event_t event){
	/*
	  Soul of Shandalar |4|R|R
	  Creature - Avatar
	  First strike
	  3RR: Soul of Shandalar deals 3 damage to target player and 3 damage to up to one target creature that player controls.
	  3RR, Exile Soul of Shandalar from your graveyard: Soul of Shandalar deals 3 damage to target player and 3 damage to up to one target creature that player controls.
	  6/6
	*/
	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XR(3, 2)) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_multi(player, MANACOST_XR(3, 2)) ){
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, 0);
		td2.zone = TARGET_ZONE_PLAYERS;

		target_definition_t td3;
		default_target_definition(player, card, &td3, TYPE_CREATURE);

		if( pick_target(&td2, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
			td3.allowed_controller = instance->targets[0].player;
			td3.preferred_controller = instance->targets[0].player;
			if( can_target(&td3) && new_pick_target(&td3, "TARGET_CREATURE", 1, 0) ){
				instance->number_of_targets = 2;
			}
			if( validate_target(player, card, &td2, 0) ){
				damage_player(instance->targets[0].player, 3, player, card);
			}
			if( instance->number_of_targets == 2 && validate_target(player, card, &td3, 1) ){
				damage_creature(instance->targets[1].player, instance->targets[1].card, 3, player, card);
			}
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(3, 2), 0, &td, "TARGET_PLAYER");
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_XR(3, 2)) ){
			if( pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				td1.allowed_controller = instance->targets[0].player;
				td1.preferred_controller = instance->targets[0].player;
				if( can_target(&td1) && new_pick_target(&td1, "TARGET_CREATURE", 1, 0) ){
					instance->number_of_targets = 2;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td, 0) ){
			damage_player(instance->targets[0].player, 3, instance->parent_controller, instance->parent_card);
		}
		if( instance->number_of_targets == 2 && validate_target(player, card, &td1, 1) ){
			damage_creature(instance->targets[1].player, instance->targets[1].card, 3, instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_seismic_strike(int player, int card, event_t event){
	/*
	  Seismic Strike |2|R
	  Instant
	  Seismic Strike deals damage to target creature equal to the number of Mountains you control.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 0, NULL);
}

int card_stoke_the_flames(int player, int card, event_t event){
	/*
	  Stoke the Flames |2|R|R
	  Instant
	  Convoke
	  Stoke the Flames deals 4 damage to target creature or player.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST){
		return generic_spell_with_convoke(player, card, event);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = cast_spell_with_convoke(player, card, event);
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		td.allow_cancel = result & (1<<31) ? 0 : 1;
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

/********
* Green *
********/

int card_carnivorous_moss_beast(int player, int card, event_t event){
	/*
	  Carnivorous Moss-Beast |4|G|G
	  Creature - Plant Elemental Beast
	  5GG: Put a +1/+1 counter on Carnivorous Moss-Beast.
	  4/5
	*/
	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		add_counter(instance->parent_controller, instance->parent_card, COUNTER_P1_P1);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XG(5, 2), 0, NULL, NULL);
}

int card_feral_incarnation(int player, int card, event_t event){
	/*
	  Feral Incarnation |8|G
	  Sorcery
	  Convoke
	  Put three 3/3 green Beast creature tokens onto the battlefield.
	*/

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BEAST, &token);
		token.pow = 3;
		token.tou = 3;
		token.color_forced = COLOR_TEST_GREEN;
		token.qty = 3;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell_with_convoke(player, card, event);
}

static int test_is_nonland(int iid, int unused1, int unused2, int unused3)
{
  return !is_what(-1, iid, TYPE_LAND);
}
int card_genesis_hydra(int player, int card, event_t event)
{
  /* Genesis Hydra	|X|G|G UNCONFIRMED (not yet on wizards.com)
   * Creature - Plant Hydra 0/0
   * When you cast ~, reveal the top X cards of your library. You may put a nonland permanent card with converted mana cost X or less from among them onto the
   * battlefield.  Then shuffle the rest into your library.
   * ~ enters the battlefield with X +1/+1 counters on it. */

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		if( ! is_token(player, card) && ! check_special_flags(player, card, SF_NOT_CAST) ){
			int x = instance->info_slot = x_value;

			test_definition_t test;
			default_test_definition(&test, TYPE_PERMANENT);
			scnprintf(test.message, 100, "Select a nonland permanent card with CMC %d or less.", x);
			test.special_selection_function = test_is_nonland;
			test.cmc = x + 1;
			test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

			if (x > 0){
				// Destination TUTOR_BOTTOM_OF_DECK_RANDOM because both TUTOR_DECK and TUTOR_BOTTOM_OF_DECK prompt you for an order.
				reveal_top_cards_of_library_and_choose(player, card, player, x, 0, TUTOR_PLAY, 0, TUTOR_BOTTOM_OF_DECK_RANDOM, 1, &test);
				shuffle(player);
			}

			/* Something above sets affected_card/controller without stashing the old values; most likely it calls dispatch_trigger_to_one_card(), which is
			 * broken in Manalink.  So enters_the_battlefield_with_counters() below fails.  Fortunately we know what the old values were, since we just checked
			 * them above with affect_me(). */
			affected_card_controller = player;
			affected_card = card;

			enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, x);
		}
	}


	return 0;
}

int card_hornet_nest(int player, int card, event_t event){
	/*
	  Hornet Nest |2|G
	  Creature - Insect
	  Defender
	  Whenever Hornet Nest is dealt damage, put that many 1/1 green Insect creature tokens with flying and deathtouch onto the battlefield.
	  0/2
	*/

	if( ! is_humiliated(player, card) && damage_dealt_to_me_arbitrary(player, card, event, 0, player, card) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_INSECT, &token);
		token.qty = get_card_instance(player, card)->targets[7].card;
		token.key_plus = KEYWORD_FLYING;
		token.s_key_plus = SP_KEYWORD_DEATHTOUCH;
		generate_token(&token);
		get_card_instance(player, card)->targets[7].card = 0;
	}

	return 0;
}

int card_hunters_ambush(int player, int card, event_t event){
	/*
	  Hunter's Ambush |2|G
	  Instant
	  Prevent all combat damage that would be dealt by nongreen creatures this turn.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		fog_special(player, card, ANYBODY, FOG_COMBAT_DAMAGE_ONLY | FOG_GREEN_CREATURES_ONLY | FOG_INVERSE_COLOR_CHECK);
		kill_card(player, card, KILL_DESTROY);
	}
	else{
		 return card_fog(player, card, event);
	}

	return 0;
}

int card_invasive_species(int player, int card, event_t event){
	/*
	  Invasive Species |2|G
	  Creature - Insect
	  When Invasive Species enters the battlefield, return another permanent you control to its owner's hand.
	  3/3
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.special = TARGET_SPECIAL_NOT_ME;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.allow_cancel = 0;
		td.illegal_abilities = 0;
		if( can_target(&td) && select_target(player, card, &td, "Select another permanent you control.", NULL) ){
			action_on_target(player, card, 0, ACT_BOUNCE);
		}
	}

	return 0;
}

int card_treefolk_warrior(int player, int card, event_t event){
	if (card == -1){
		return 0;
	}

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && get_special_infos(player, card) == 66){
		event_result += count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_FOREST));
	}

	return generic_token(player, card, event);
}

int card_kalonian_twingrove(int player, int card, event_t event){
	/*
	  Kalonian Twingrove |5|G
	  Creature - Treefolk Warrior
	  Kalonian Twingrove's power and toughness are each equal to the number of Forests you control.
	  When Kalonian Twingrove enters the battlefield, put a green Treefolk Warrior creature token onto the battlefield with
	  "This creature's power and toughness are each equal to the number of Forests you control."
	  x/x
	*/
	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_TREEFOLK_WARRIOR, &token);
		token.color_forced = COLOR_TEST_GREEN;
		token.special_infos = 66;
		generate_token(&token);
	}

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_FOREST));
	}

	return 0;
}

int card_lifes_legacy(int player, int card, event_t event){
	/*
	  Life's Legacy |1|G
	  Sorcery
	  As an additional cost to cast Life's Legacy, sacrifice a creature.
	  Draw cards equal to the sacrificed creature's power.
	*/

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = pick_creature_for_sacrifice(player, card, 0);
		if( result > -1 ){
			instance->targets[1].player = get_power(player, result);
			kill_card(player, result, KILL_SACRIFICE);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, instance->targets[1].player);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_living_totem(int player, int card, event_t event){
	/*
	  Living Totem |3|G
	  Creature - Plant Elemental
	  Convoke
	  When Living Totem enters the battlefield, you may put a +1/+1 counter on another target creature.
	  2/3
	*/
	if( event == EVENT_MODIFY_COST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell_with_convoke(player, card, event);
	}

	if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player))){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_netcaster_spider(int player, int card, event_t event){
	/*
	  Netcaster Spider |2|G
	  Creature - Spider
	  Reach
	  Whenever Netcaster Spider blocks a creature with flying, Netcaster Spider gets +2/+0 until end of turn.
	  2/3
	*/

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_DECLARE_BLOCKERS && current_turn != player && ! is_humiliated(player, card) ){
		if( instance->blocking < 255 ){
			if( check_for_ability(1-player, instance->blocking, KEYWORD_FLYING) ){
				pump_until_eot(player, card, player, card, 2, 0);
			}
		}
	}

	return 0;
}

int card_nissa_worldwaker(int player, int card, event_t event)
{
	/*
	  Nissa, Worldwaker |3|G|G
	  Planeswalker - Nissa
	  [+1]: Target land you control becomes a 4/4 Elemental creature with trample. It's still a land.
	  [+1]: Untap up to four target Forests.
	  [-7]: Search your library for any number of basic land cards, put them onto the battlefield, then shuffle your library.
	  Those lands become 4/4 Elemental creatures with trample. They're still lands.
	  3
	*/


	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.preferred_controller = player;
		td.allowed_controller = player;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_LAND);
		td1.required_subtype = SUBTYPE_FOREST;
		td1.preferred_controller = player;

		enum{
			CHOICE_ANIMATE_LAND = 1,
			CHOICE_UNTAP_FORESTS,
			CHOICE_TUTOR_LAND_AND_ANIMATE
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Animate a land", can_target(&td), current_phase < PHASE_AFTER_BLOCKING ? 20 : 5, 1,
						"Untap up to 4 forests", can_target(&td1), (target_available(player, card, &td1) * 5) + (current_phase > PHASE_DECLARE_BLOCKERS * 5) +
																	(current_turn == player * 5), 1,
						"Tutor land and animate them", 1, count_counters(player, card, COUNTER_LOYALTY) > 7 ? 25 : 5, -7);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  switch (choice)
			{
			  case CHOICE_ANIMATE_LAND:
					instance->number_of_targets = 0;
					if( select_target(player, card, &td, "Select target land to animate", NULL) ){
						instance->number_of_targets = 1;
					}
					else{
						spell_fizzled = 1;
					}
					break;

			  case CHOICE_UNTAP_FORESTS:
				{
					int i = 0;
					while( i < 4 && can_target(&td1) ){
							if( select_target(player, card, &td1, "Select target forest to untap", &(instance->targets[i])) ){
								state_untargettable(instance->targets[i].player, instance->targets[i].card, 1);
								i++;
							}
							else{
								break;
							}
					}
					for(i=0; i<instance->number_of_targets; i++){
						state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
					}
					break;
				}
			  case CHOICE_TUTOR_LAND_AND_ANIMATE:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_ANIMATE_LAND:
				if ( valid_target(&td)){
					add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_ELEMENTAL);
					land_animation2(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 4, 4, KEYWORD_TRAMPLE, 0, 0, NULL);
				}
				break;

			case CHOICE_UNTAP_FORESTS:
				{
					int k;
					for(k=0; k<instance->number_of_targets; k++){
						if( validate_target(player, card, &td1, k) ){
							untap_card(instance->targets[k].player, instance->targets[k].card);
						}
					}
				}
				break;

			case CHOICE_TUTOR_LAND_AND_ANIMATE:
				{
					int lands_tutored[count_deck(player)];
					int ltc = 0;

					test_definition_t test;
					new_default_test_definition(&test, TYPE_LAND, "Select a basic land card.");
					test.subtype = SUBTYPE_BASIC;
					while( deck_ptr[player][0] != -1 ){
							int selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_FIRST_FOUND, -1, &test);
							if( selected != -1 ){
								lands_tutored[ltc] = deck_ptr[player][selected];
								remove_card_from_deck(player, selected);
								ltc++;
							}
							else{
								break;
							}
					}
					shuffle(player);

					int i;
					for(i=0; i<ltc; i++){
						int result = add_card_to_hand(player, lands_tutored[i]);
						add_a_subtype(player, result, SUBTYPE_ELEMENTAL);
						put_into_play(player, result);
						land_animation2(instance->parent_controller, instance->parent_card, player, result, 0, 4, 4, 0, 0, 0, NULL);
					}
				}
				break;
		  }
	}

	return planeswalker(player, card, event, 3);
}

int card_nissas_expedition(int player, int card, event_t event){
	/*
	  Nissa's Expedition |4|G
	  Sorcery
	  Convoke
	  Search your library for up to two basic land cards, put them onto the battlefield tapped, then shuffle your library.
	*/
	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
		this_test.subtype = SUBTYPE_BASIC;
		this_test.qty = 2;
		this_test.no_shuffle = 1;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell_with_convoke(player, card, event);
}


int card_paragon_of_eternal_wilds(int player, int card, event_t event){
	// unconfirmed
	// Paragon of Eternal Wilds 3G
	// Creature - Human Druid (U)
	// Other green creatures you control get +1/+1.
	// G, T: Another target green creature you control gains trample until end of turn.
	// 2/2
	return m15_paragon(player, card, event, COLOR_TEST_GREEN, 0, 0, KEYWORD_TRAMPLE, 0);
}

int phytotitan_legacy(int player, int card, event_t event){
	if( current_turn == player && upkeep_trigger(player, card, event) ){
		seek_grave_for_id_to_reanimate(player, card, player, get_card_instance(player, card)->display_pic_csv_id, REANIMATE_TAP);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_phytotitan(int player, int card, event_t event){
	// unconfirmed
	// phytotitan 4GG
	// Creature - Plant Elemental (R)
	// When phytotitan dies, return it to the battlefield tapped under its owner's control at the beginning of his or her next upkeep.
	// 7/2

	if( this_dies_trigger_for_owner(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		create_legacy_effect(player, card, &phytotitan_legacy);
	}

	return 0;
}

int card_reclamation_sage(int player, int card, event_t event){
	// unconfirmed
	// Reclamation Sage 2G
	// Creature - Elf Shaman (U)
	// When Reclamation Sage enters the battlefield, you may destroy target artifact or enchantment.
	// 2/1

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && can_target(&td) && pick_target(&td, "DISENCHANT") ){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return 0;
}

/*
  Shaman of Spring |3|G --> Wall of Blossoms
  Creature - Elf Shaman
  When Shaman of Spring enters the battlefield, draw a card.
  2/2
*/

int card_siege_wurm(int player, int card, event_t event){
	/*
	  Siege Wurm |5|G|G
	  Creature - Wurm
	  Convoke
	  Trample
	  5/5
	*/
	return generic_spell_with_convoke(player, card, event);
}

int card_soul_of_zendikar(int player, int card, event_t event){
	// Soul of Zendikar 4GG
	// Creature - Avatar
	// Reach
	// 3GG: Put a 3/3 green Beast creature token onto the battlefield.
	// 3GG, Exile Soul of Zendikar from your graveyard: Put a 3/3 green Beast creature token onto the battlefield.
	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XG(3, 2)) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_multi(player, MANACOST_XG(3, 2)) ){
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION || event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BEAST, &token);
		token.pow = 3;
		token.tou = 3;
		generate_token(&token);
	}
	return generic_activated_ability(player, card, event, 0, MANACOST_XG(3, 2), 0, NULL, NULL);
}

int card_sunblade_elf(int player, int card, event_t event){
	/*
	  Sunblade Elf |G
	  Creature - Elf Warrior
	  Sunblade Elf gets +1/+1 as long as you control a Plains.
	  4W: Creatures you control get +1/+1 until end of turn.
	  1/1
	*/
	uthden_creature(player, card, event, COLOR_WHITE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, -1, 1, 1, 0, 0);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XW(4, 1), 0, NULL, NULL);
}

int card_undergrowth_scavenger(int player, int card, event_t event){
	/*
	  Undergrowth Scavenger |3|G
	  Creature - Fungus Horror
	  Undergrowth Scavenger enters the battlefield with a number of +1/+1 counters on it equal to the number of creature cards in all graveyards.
	  0/0
	*/
	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, count_graveyard_by_type(2, TYPE_CREATURE));
	return 0;
}

int card_venom_sliver(int player, int card, event_t event){
	/*
	  Venom Sliver |1|G
	  Creature - Sliver
	  Sliver creatures you control have deathtouch.
	  1/1
	*/
	if( ! is_humiliated(player, card) && in_play(player, card) && affected_card_controller == player &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE) && has_subtype(affected_card_controller, affected_card, SUBTYPE_SLIVER)
	  ){
		deathtouch(affected_card_controller, affected_card, event);
	}

	return 0;
}

int card_vineweft(int player, int card, event_t event){
	/*
	  Vineweft |G
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature gets +1/+1.
	  4G: Return Vineweft from your graveyard to your hand.
	*/
	if( event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, MANACOST_XG(4, 1)) ){
		return GA_RETURN_TO_HAND;
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XG(4, 1));
		if( spell_fizzled != 1){
			return GAPAID_REMOVE;
		}
	}

	return generic_aura(player, card, event, player, 1, 1, 0, 0, 0, 0, 0);
}

int card_yisan_the_wanderer_bard(int player, int card, event_t event){
	/*
	  Yisan, the Wanderer Bard |2|G
	  Legendary Creature - Human Rogue
	  2G, T, Put a verse counter on Yisan, the Wanderer Bard: Search your library for a creature card with converted mana cost equal to the number of verse
	  counters on Yisan, put it onto the battlefield, then shuffle your library.
	  2/3
	*/
	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XG(2, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XG(2, 1)) ){
			tap_card(player, card);
			add_counter(player, card, COUNTER_VERSE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char buffer[100];
		scnprintf(buffer, 100, " Select a creature card with CMC %d", count_counters(player, card, COUNTER_VERSE));
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, buffer);
		test.cmc = count_counters(player, card, COUNTER_VERSE);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &test);
	}

	return 0;
}

/*************
* Multicolor *
*************/
int card_garruk_predator_emblem(int player, int card, event_t event){
	/*
	  Whenever a creature attacks you, it gets +5/+5 and gains trample until end of turn.
	*/
	if( current_turn != player ){
		int amt;
		if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, 1-player, -1))){
			card_instance_t* instance = get_card_instance(player, card);
			unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
			for (--amt; amt >= 0; --amt){
				if (in_play(current_turn, attackers[amt]) && ! check_special_flags(current_turn, attackers[amt], SF_ATTACKING_PWALKER) ){
					pump_ability_until_eot(player, card, current_turn, attackers[amt], 5, 5, KEYWORD_TRAMPLE, 0);
				}
			}
		}
	}

	return 0;
}

int card_garruk_apex_predator(int player, int card, event_t event)
{
	/*
	  Garruk, Apex Predator |5|B|G
	  Planeswalker - Garruk
	  +1: Destroy another target planeswalker.
	  +1: Put a 3/3 black Beast creature token with deathtouch onto the battlefield.
	  -3: Destroy target creature. You gain life equal to its toughness.
	  -8: Target opponent gets an emblem with "Whenever a creature attacks you, it gets +5/+5 and gains trample until end of turn."
	  5
	*/

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TARGET_TYPE_PLANESWALKER);
		td.special = TARGET_SPECIAL_NOT_ME;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);

		target_definition_t td2;
		default_target_definition(player, card, &td2, 0);
		td2.zone = TARGET_ZONE_PLAYERS;

		enum{
			CHOICE_KILL_PLANESWALKER = 1,
			CHOICE_GENERATE_BEAST,
			CHOICE_KILL_CREATURE_GAIN_LIFE,
			CHOICE_EMBLEM
		};
		int ai_priority_kill_creature = (5*(15-life[player]))+
										(2*(count_counters(player, card, COUNTER_LOYALTY)-3));
		int ai_priority_emblem = (5*count_subtype(player, TYPE_CREATURE, -1))+
								(2*(count_counters(player, card, COUNTER_LOYALTY)-7))-
								(20*check_battlefield_for_id(player, CARD_ID_GARRUK_PREDATOR_EMBLEM));
		int choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Kill another planeswalker", can_target(&td), 15, 1,
						"Generate a Beast", 1, 10, 1,
						"Kill a creature & gain life", can_target(&td1), ai_priority_kill_creature, -3,
						"Emblem", would_validate_arbitrary_target(&td2, 1-player, -1), ai_priority_emblem, -7);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  switch (choice)
			{
			  case CHOICE_KILL_PLANESWALKER:
					instance->number_of_targets = 0;
					if( select_target(player, card, &td, "Select another target planeswalker", NULL) ){
						instance->number_of_targets = 1;
					}
					else{
						spell_fizzled = 1;
					}
					break;

			  case CHOICE_GENERATE_BEAST:
					break;

			  case CHOICE_KILL_CREATURE_GAIN_LIFE:
					instance->number_of_targets = 0;
					if( select_target(player, card, &td1, "Select target creature", NULL) ){
						instance->number_of_targets = 1;
					}
					else{
						spell_fizzled = 1;
					}
					break;

			 case CHOICE_EMBLEM:
				{
					instance->targets[0].player = 1-player;
					instance->targets[0].card = -1;
					instance->number_of_targets = 1;
					break;
				}
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_KILL_PLANESWALKER:
				if ( valid_target(&td)){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				}
				break;

			case CHOICE_GENERATE_BEAST:
				{
					token_generation_t token;
					default_token_definition(player, card, CARD_ID_BEAST, &token);
					token.pow = 3;
					token.tou = 3;
					token.s_key_plus = SP_KEYWORD_DEATHTOUCH;
					token.color_forced = COLOR_TEST_BLACK;
					generate_token(&token);
				}
				break;

			case CHOICE_KILL_CREATURE_GAIN_LIFE:
				if ( valid_target(&td1)){
					int amount = get_toughness(instance->targets[0].player, instance->targets[0].card);
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
					gain_life(player, amount);
				}
				break;

			case CHOICE_EMBLEM:
				;token_generation_t token;
				default_token_definition(player, card, CARD_ID_GARRUK_PREDATOR_EMBLEM, &token);
				token.t_player = 1-player;
				generate_token(&token);
				break;
		  }
	}

  return planeswalker(player, card, event, 5);
}

int card_sliver_hivelord(int player, int card, event_t event){
	/*
	  Sliver Hivelord |W|U|R|B|G
	  Legendary Creature - Sliver
	  Sliver creatures you control have indestructible.
	  5/5
	*/
	check_legend_rule(player, card, event);

	if (event == EVENT_ABILITIES && affected_card_controller == player && has_subtype(affected_card_controller, affected_card, SUBTYPE_SLIVER) &&
		in_play(player, card) && !is_humiliated(player, card)
	  ){
		indestructible(affected_card_controller, affected_card, event);
	}

	return 0;
}


/***********
* Artifact *
***********/

int card_avarice_amulet(int player, int card, event_t event){
	// unconfirmed
	// Avarice Amulet 4
	// Artifact - Equipment (R)
	// Equipped creature gets +2/+0 and has vigilance "At the beginning of your upkeep, draw a card."
	// When equipped creature dies, target opponent gains control of Avarice Amulet.
	// Equip 2
	if( in_play(player, card) && is_equipping(player, card) ){
		upkeep_trigger_ability(player, card, event, player);
		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			draw_cards(player, 1);
		}
		card_instance_t *instance = get_card_instance(player, card);
		if( graveyard_from_play(instance->targets[8].player, instance->targets[8].card, event) ){
			target_definition_t td;
			default_target_definition(player, card, &td, 0);
			td.zone = TARGET_ZONE_PLAYERS;

			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
			int result = would_valid_target(&td);
			instance->targets[0].player = -1;
			instance->targets[0].card = -1;
			instance->number_of_targets = 0;
			if( result ){
				give_control(player, card, player, card);
			}
		}
	}

	return vanilla_equipment(player, card, event, 2, 2, 0, 0, SP_KEYWORD_VIGILANCE);
}

int card_brawlers_plate(int player, int card, event_t event){
	/*
	  Brawler's Plate |3
	  Artifact - Equipment
	  Equipped creature gets +2/+2 and has trample.
	  Equip 4
	*/
	return vanilla_equipment(player, card, event, 4, 2, 2, KEYWORD_TRAMPLE, 0);
}

int card_hot_soup(int player, int card, event_t event){
	/*
	  Hot Soup |1
	  Artifact - Equipment
	  Equipped creature can't be blocked.
	  Whenever equipped creature is dealt damage, destroy it.
	  Equip 3
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && is_equipping(player, card) ){
		if( damage_dealt_to_me_arbitrary(instance->targets[8].player, instance->targets[8].card, event, 0, player, card) ){
			kill_card(instance->targets[8].player, instance->targets[8].card, KILL_DESTROY);
		}
	}

	return vanilla_equipment(player, card, event, 3, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
}

int card_meteorite(int player, int card, event_t event){
	/*
	  Meteorite |5
	  Artifact
	  When Meteorite enters the battlefield, it deals 2 damage to target creature or player.
	  T: Add one mana of any color to your mana pool.
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td1.allow_cancel = 0;
		if( can_target(&td1) && pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
			damage_target0(player, card, 2);
			get_card_instance(player, card)->number_of_targets = 1;
		}
	}

	return mana_producer(player, card, event);
}

int card_obelisk_of_urd(int player, int card, event_t event){
	/*
	  Obelisk of Urd |6
	  Artifact
	  Convoke
	  As Obelisk of Urd enters the battlefield, choose a creature type.
	  Creatures you control of the chosen type get +2/+2.
	*/

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = select_a_subtype(player, card);
	}

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		boost_creature_type(player, card, event, instance->info_slot, 2, 2, 0, BCT_CONTROLLER_ONLY);
	}

	return generic_spell_with_convoke(player, card, event);
}

int card_perilous_vault(int player, int card, event_t event){
	/*
	  Perilous Vault |4
	  Artifact
	  5, T, Exile Perilous Vault: Exile all nonland permanents.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t test;
		default_test_definition(&test, TYPE_LAND);
		test.type_flag = DOESNT_MATCH;
		new_manipulate_all(instance->parent_controller, instance->parent_card, ANYBODY, &test, KILL_REMOVE);
	}

	return generic_activated_ability(player, card, event, GAA_RFG_ME | GAA_UNTAPPED, MANACOST_X(5), 0, NULL, NULL);
}

int card_profane_memento(int player, int card, event_t event){
	/*
	  Profane Memento |1
	  Artifact
	  Whenever a creature card is put into an opponent's graveyard from anywhere, you gain 1 life.
	*/
	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, 1-player, TYPE_CREATURE, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		gain_life(player, get_card_instance(player, card)->targets[11].card);
		get_card_instance(player, card)->targets[11].card = 0;
	}

	enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
	if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == 1-player && !is_humiliated(player, card))
	{
	  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER)
		return 0;

	  int i;
	  for (i = 0; i < num_cards_milled; ++i)
		if (is_what(-1, cards_milled[i].internal_card_id, TYPE_CREATURE))
		  {
			if (event == EVENT_TRIGGER)
			  {
				event_result |= RESOLVE_TRIGGER_MANDATORY;
				return 0;
			  }

			if (event == EVENT_RESOLVE_TRIGGER)
			  {
				if (cards_milled[i].position != -1)
				  {
					gain_life(player, 1);
				  }
			  }
		  }
	}

	return 0;
}

int card_rogues_glove(int player, int card, event_t event){
	/*
	  Rogue's Gloves |2
	  Artifact - Equipment
	  Whenever equipped creature deals combat damage to a player, you may draw a card.
	  Equip 2
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE && player == AI ){
		if( instance->targets[0].player == -1 ){
			ai_modifier+=15;
		}
	}

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( damage_dealt_by_me(instance->damage_target_player, instance->damage_target_card, event,
								DDBM_TRIGGER_OPTIONAL | DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_BE_COMBAT_DAMAGE)
		  ){
			draw_cards(player, 1);
		}
	}

	return basic_equipment(player, card, event, 2);
}

int card_sacred_armory(int player, int card, event_t event){
	/*
	  Sacred Armory |2
	  Artifact
	  2: Target creature gets +1/+0 until end of turn.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	return vanilla_creature_pumper(player, card, event, MANACOST_X(2), 0, 1, 0, 0, 0, &td);
}

int card_scuttling_doom_engine(int player, int card, event_t event){
	/*
	  Scuttling Doom Engine |6
	  Artifact Creature - Construct
	  Scuttling Doom Engine can't be blocked by creatures with power 2 or less.
	  When Scuttling Doom Engine dies, it deals 6 damage to target opponnent.
	  6/6
	*/
	if( ! is_humiliated(player, card) ){
		if( event == EVENT_BLOCK_LEGALITY && attacking_card_controller == player && attacking_card == card ){
			if( get_power(affected_card_controller, affected_card) < 3 ){
				event_result = 1;
			}
		}

		if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_CREATURE);
			td1.zone = TARGET_ZONE_PLAYERS;
			if( would_validate_arbitrary_target(&td1, 1-player, -1) ){
				damage_player(1-player, 6, player, card);
			}
		}
	}

	return 0;
}

int card_shield_of_the_avatar(int player, int card, event_t event){
	/*
	  Shield of the Avatar |1
	  Artifact - Equipment
	  If a source would deal damage to equipped creature, prevent X of that damage, where X is the number of creatures you control.
	  Equip 2
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE && ! is_equipping(player, card) ){
		ai_modifier+=15;
	}

	if( in_play(player, card) && is_equipping(player, card) ){
		if( event == EVENT_PREVENT_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_target_card == instance->targets[8].card && damage->damage_target_player == instance->targets[8].player &&
					damage->info_slot > 0
				  ){
					int amount = count_subtype(player, TYPE_CREATURE, -1);
					damage->info_slot = amount > damage->info_slot ? 0 : damage->info_slot-amount;
				}
			}
		}
	}

	return basic_equipment(player, card, event, 2);
}

int card_soul_of_new_phyrexia(int player, int card, event_t event){
	/*
	  Soul of New Phyrexia |6
	  Artifact Creature - Avatar
	  Trample
	  5: Permanents you control gain indestructible until end of turn.
	  5, Exile Soul of New Phyrexia from your graveyard: Permanents you control gain indestructible until end of turn.
	  6/6
	*/

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_X(5)) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_multi(player, MANACOST_X(5)) ){
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		int i;
		for(i=active_cards_count[player]-1; i>-1; i--){
			if( in_play(player, i) && is_what(player, i, TYPE_PERMANENT) ){
				pump_ability_until_eot(instance->parent_controller, instance->parent_card, player, i, 0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		int i;
		for(i=active_cards_count[player]-1; i>-1; i--){
			if( in_play(player, i) && is_what(player, i, TYPE_PERMANENT) ){
				pump_ability_until_eot(player, card, player, i, 0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
			}
		}
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_X(5), 0, NULL, NULL);
}

int card_the_chain_veil(int player, int card, event_t event){
	/*
	  The Chain Veil |4
	  Legendary Artifact
	  At the beginning of your end step, if you didn't activate a loyalty ability of a planeswalker this turn, you lose 2 life.
	  4, T: For each planeswalker you control, you may activate one of its loyalty abilities once this turn as though none of its loyalty abilities had been activated this turn.
	*/

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=active_cards_count[player]-1; i>-1; i--){
			if( in_play(player, i) && is_planeswalker(player, i) ){
				get_card_instance(player, i)->targets[9].card = 0; // AKA reset the flag for "activate each planeswalker once per turn".
			}
		}
	}

	if( current_turn == player && trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player ){
		int sum = 0;
		int i;
		for(i=active_cards_count[player]-1; i>-1; i--){
			if( in_play(player, i) && is_planeswalker(player, i) ){
				if( get_card_instance(player, i)->targets[9].card > 0 ){ //AKA this planeswalker has used a loyalty ability.
					sum++;
					get_card_instance(player, i)->targets[9].card = 0;
				}
			}
		}
		if( ! sum && ! is_humiliated(player, card) ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					lose_life(player, 2);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL);
}

int card_tyrants_machine(int player, int card, event_t event){
	/*
	  Tyrant's Machine |2
	  Artifact
	  4, T: Tap target creature.
	*/

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		action_on_target(player, card, 0, ACT_TAP);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(4), 0, &td, "TARGET_CREATURE");
}

/*
Will-Forged Golem |6 --> Siege Wurm
Artifact Creature - Golem
Convoke
/4
*/

/*******
* Land *
*******/

int card_radiant_fountain(int player, int card, event_t event){
	/*
	  Radiant Fountain
	  Land
	  When Radiant Fountain enters the battlefield, you gain 2 life.
	  T: Add 1 to your mana pool.
	*/
	if( comes_into_play(player, card, event) ){
		gain_life(player, 2);
	}

	return mana_producer(player, card, event);
}

#pragma message "The second ability cannot be coded"
int card_sliver_hive(int player, int card, event_t event){
	/*
	  Sliver Hive
	  Land
	  T: Add 1 to your mana pool.
	  T: Add one mana of any color to your mana pool. Spend this mana only to cast a Sliver spell.
	  5, T: Put a 1/1 colorless Sliver creature token onto the battlefield. Activate this ability only if you control a Sliver.
	*/

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if (!paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(6)) &&
			count_subtype(player, TYPE_PERMANENT, SUBTYPE_SLIVER)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Generate a Sliver\n Cancel", 1);
		}

		card_instance_t* instance = get_card_instance(player, card);
		instance->info_slot = choice;

		if (choice == 0){
			return mana_producer(player, card, event);
		}
		if (choice == 1){
			add_state(player, card, STATE_TAPPED);
			if (!charge_mana_for_activated_ability(player, card, MANACOST_X(5))){
				remove_state(player, card, STATE_TAPPED);
			}
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		if( instance->info_slot == 1 ){
			generate_token_by_id(player, card, CARD_ID_SLIVER);
		}
	}
	else {
		return mana_producer(player, card, event);
	}

	return 0;
}
