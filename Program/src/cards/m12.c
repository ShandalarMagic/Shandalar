#include "manalink.h"

// global functions
static int empire_set(int player){

	int score = 0;
	int count = 0;
	while( count < active_cards_count[player] ){
			if( in_play(player, count) ){
				if( get_id(player, count) == CARD_ID_CROWN_OF_EMPIRES ){
					score |=1;
				}
				else if( get_id(player, count) == CARD_ID_SCEPTER_OF_EMPIRES ){
						 score |=2;
				}
				else if( get_id(player, count) == CARD_ID_THRONE_OF_EMPIRES ){
						 score |=4;
				}
			}
			if( score == 7 ){
				return 1;
			}
			count++;
	}
	return 0;
}

static int mages(int player, int card, event_t event, int mode){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	int mana[6] = {0, 0, 0, 0, 0, 0};
	mana[mode] = 1;
	if( mode == COLOR_BLUE ){
		mana[0] = 3;
	}
	if( mode == COLOR_GREEN ){
		mana[0] = 2;
	}
	if( mode == COLOR_BLACK || mode == COLOR_WHITE ){
		mana[0] = 1;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( mode == COLOR_BLACK || mode == COLOR_RED || mode == COLOR_WHITE ){
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET, mana[0], mana[1],mana[2],mana[3],mana[4],mana[5], 0, &td, NULL);
		}
		else{
			return generic_activated_ability(player, card, event, 0, mana[0], mana[1],mana[2],mana[3],mana[4],mana[5], 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, mana[0], mana[1],mana[2],mana[3],mana[4],mana[5]) ){
			if( mode == COLOR_BLACK || mode == COLOR_RED || mode == COLOR_WHITE ){
				pick_target(&td, "TARGET_CREATURE");
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		switch( mode ){
				case COLOR_BLACK:
				{
					if( valid_target(&td) ){
						pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
												0, 0, 0, SP_KEYWORD_DEATHTOUCH);
					}
				}
				break;

				case COLOR_BLUE:
					draw_cards(player, 1);
					break;

				case COLOR_GREEN:
					generate_token_by_id(player, card, CARD_ID_SAPROLING);
					break;

				case COLOR_RED:
				{
					if( valid_target(&td) ){
						pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
												0, 0, 0, SP_KEYWORD_HASTE);
					}
				}
				break;

				case COLOR_WHITE:
				{
					if( valid_target(&td) ){
						pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
												0, 0, 0, SP_KEYWORD_LIFELINK);
					}
				}
				break;

				default:
					break;

		}
	}

	return 0;
}

// cards
int card_adaptive_automaton(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		if( player == AI ){
			state_untargettable(player, card, 1);
		}
		int new_subt = select_a_subtype(player, card);
		add_a_subtype(player, card, new_subt);
		if( player == AI ){
			state_untargettable(player, card, 0);
		}
	}

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && ! affect_me(player, card) ){
		int subt = get_added_subtype(player, card);
		if( subt > 0 && ! is_humiliated(player, card) ){
			if( in_play(affected_card_controller, affected_card) && affected_card_controller == player &&
				has_subtype(affected_card_controller, affected_card, subt )
			   ){
				 event_result++;
			}
		}
	}

	return 0;
}

int card_angelic_destiny(int player, int card, event_t event){

	when_enchanted_permanent_dies_return_aura_to_hand(player, card, event);

	return generic_aura(player, card, event, player, 4, 4, KEYWORD_FLYING | KEYWORD_FIRST_STRIKE, 0, SUBTYPE_ANGEL, 0, 0);
}

static int aegis_angel_effect(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[0].player != -1 && instance->targets[0].card != -1 &&
		in_play(instance->targets[0].player, instance->targets[0].card)
	  ){
		indestructible(instance->targets[0].player, instance->targets[0].card, event);
	}

	if( instance->targets[1].player > -1 && leaves_play(instance->targets[1].player, instance->targets[1].card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_aegis_angel(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;
		td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && new_pick_target(&td, "Select another target permanent.", 0, GS_LITERAL_PROMPT) ){
			int legacy = create_targetted_legacy_effect(player, card, &aegis_angel_effect, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].player = player;
			leg->targets[1].card = card;
			leg->number_of_targets = 2;
		}
	}

	return 0;
}

int card_alabaster_mage(int player, int card, event_t event){
	/* Alabaster Mage	|1|W
	 * Creature - Human Wizard 2/1
	 * |1|W: Target creature you control gains lifelink until end of turn. */

	return mages(player, card, event, COLOR_WHITE);
}

int card_angels_mercy(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 7);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_arbalest_elite(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, instance->parent_card);
			effect_frost_titan(player, instance->parent_card, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XW(2, 1), 0,
									&td, "Select target attacking or blocking creature.");
}

/*
static int autumns_veil_effect(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && ! affect_me(player, card) && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			set_special_flags2(affected_card_controller, affected_card, SF2_AUTUMN_VEIL);
		}
	}
	if( eot_trigger(player, card, event) ){
		remove_special_flags2(player, -1, SF2_AUTUMN_VEIL);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}
*/
int card_autumns_veil(int player, int card, event_t event){//UNUSEDCARD

	/* Autumn's Veil	|G
	 * Instant
	 * Spells you control can't be countered by |Sblue or |Sblack spells this turn, and creatures you control can't be the targets of |Sblue or |Sblack spells
	 * this turn. */
	/*
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( get_deck_color(-1, 1-player) & (COLOR_TEST_BLACK | COLOR_TEST_BLUE) ){
			ai_modifier += 25;
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		set_special_flags2(player, -1, SF2_AUTUMN_VEIL);
		create_legacy_effect(player, card, &autumns_veil_effect);
		kill_card(player, card, KILL_DESTROY);
	}
	*/
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_azure_mage(int player, int card, event_t event){
	/* Azure Mage	|1|U
	 * Creature - Human Wizard 2/1
	 * |3|U: Draw a card. */

	return mages(player, card, event, COLOR_BLUE);
}

int card_benalish_veteran(int player, int card, event_t event)
{
  // Whenever ~ attacks, it gets +1/+1 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	pump_until_eot(player, card, player, card, 1, 1);

  return 0;
}

int card_bloodlord_of_vaasgoth(int player, int card, event_t event){

	bloodthirst(player, card, event, 3);

	card_instance_t *instance= get_card_instance(player, card);

	if( (event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && ! affect_me(player, card) && affected_card_controller == player &&
		! is_humiliated(player, card)
	  ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			if( instance->targets[1].player != 66 ){
				instance->targets[1].player = get_trap_condition(1-player, TRAP_DAMAGE_TAKEN) > 0 ? 66 : 0;
			}
			if( instance->targets[1].player == 66 ){
				if( has_subtype(affected_card_controller, affected_card, SUBTYPE_VAMPIRE) ){
					enters_the_battlefield_with_counters(affected_card_controller, affected_card, event, COUNTER_P1_P1, 3);
				}
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int chandra_firebrand_legacy(int player, int card, event_t event){
	card_instance_t* instance = get_card_instance(player, card);
	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_SPELL, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		copy_spell_from_stack(player, instance->targets[1].player, instance->targets[1].card);
		kill_card(player, card, KILL_REMOVE);
	}
	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_chandra_firebrand(int player, int card, event_t event){

	/* Chandra, the Firebrand	|3|R
	 * Planeswalker - Chandra (3)
	 * +1: ~ deals 1 damage to target creature or player.
	 * -2: When you cast your next instant or sorcery spell this turn, copy that spell. You may choose new targets for the copy.
	 * -6: ~ deals 6 damage to each of up to six target creatures and/or players. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);

		enum{
			CHOICE_PING = 1,
			CHOICE_COPY_SPELL,
			CHOICE_6TARGETS
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Ping creature or player", can_target(&td), 15, 1,
						"Copy spell", 1, count_counters(player, card, COUNTER_LOYALTY) > 2 ? 20 : 5, -2,
						"6 damage to 6 targets", 1, 3*target_available(player, card, &td), -6);

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
			  case CHOICE_PING:
					instance->number_of_targets = 0;
					pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
					break;

			  case CHOICE_COPY_SPELL:
					break;

			  case CHOICE_6TARGETS:
				{
					int trgs = 0;
					int targetted_players[2] = {! would_validate_arbitrary_target(&td, 0, -1),
												player == AI ? 1 : ! would_validate_arbitrary_target(&td, 1, -1)};
					if( targetted_players[1-player] != 1 && player == AI ){
						instance->targets[0].player = 1-player;
						instance->targets[0].card = -1;
						instance->number_of_targets = 1;
						trgs++;
					}
					while( trgs < 6 ){
							char buffer[100];
							scnprintf(buffer, 100, "Select target creature or player (%d of 6)", trgs+1);
							if( new_pick_target(&td, buffer, trgs, GS_LITERAL_PROMPT) ){
								if( instance->targets[trgs].card != -1 ){
									state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
									trgs++;
								}
								else{
									if( targetted_players[instance->targets[trgs].player] != 1 ){
										targetted_players[instance->targets[trgs].player] = 1;
										trgs++;
									}
									else{
										instance->number_of_targets--;
									}
								}
							}
							else{
								break;
							}
							if( targetted_players[0]+targetted_players[1] == 2 && ! can_target(&td2) ){
								break;
							}

					}
					int i;
					for(i=0; i<instance->number_of_targets; i++){
						if( instance->targets[i].card != -1 ){
							state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
						}
					}
				}
				break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_PING:
				if( valid_target(&td) ){
					damage_target0(player, card, 1);
				}
				break;

			case CHOICE_COPY_SPELL:
				create_legacy_effect(player, card, &chandra_firebrand_legacy);
				break;

			case CHOICE_6TARGETS:
				{
					int i;
					for(i=0; i<instance->number_of_targets; i++){
						if( validate_target(player, card, &td, i) ){
							damage_creature(instance->targets[i].player, instance->targets[i].card, 6, player, card);
						}
					}
				}
			  break;
		  }
	}

	return planeswalker(player, card, event, 3);
}

int card_crimson_mage(int player, int card, event_t event){
	/* Crimson Mage	|1|R
	 * Creature - Human Shaman 2/1
	 * |R: Target creature you control gains haste until end of turn. */

	return mages(player, card, event, COLOR_RED);
}

int card_divine_favor(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		gain_life(player, 3);
	}

	return generic_aura(player, card, event, player, 1, 3, 0, 0, 0, 0, 0);
}

int card_doubling_chant(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		if( IS_AI(player) && player == AI && count_permanents_by_type(player, TYPE_CREATURE) < 1 ){
			return 0;
		}
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( ! check_battlefield_for_id(ANYBODY, CARD_ID_GRAFDIGGERS_CAGE) ){
			int choice = do_dialog(player, player, card, -1, -1, " Auto mode\n Manual mode", 0);
			int *deck = deck_ptr[player];
			int count = active_cards_count[player]-1;
			int found[100];
			int fc = 0;
			while( count > -1 ){
					if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
						int to_search = 1;
						int id = get_id(player, count);
						if( choice == 1 ){
							to_search = 1-do_dialog(player, player, count, -1, -1, " Search your deck for this creature\n Pass", 0);
						}

						if( choice == 0 && has_subtype(player, count, SUBTYPE_LEGEND) && player == AI){
							to_search = 0;
						}

						if( to_search == 1 ){
							int cards = 0;
							while( cards < count_deck(player) ){
									if( cards_data[deck[cards]].id == id ){
										found[fc] = deck[cards];
										fc++;
										remove_card_from_deck(player, cards);
										break;
									}
									cards++;
							}

						}
					}
					count--;
			}
			int i;
			for(i=0; i<fc; i++){
				int card_added = add_card_to_hand(player, found[i]);
				put_into_play(player, card_added);
			}
		}
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_druidic_satchel(int player, int card, event_t event){
	/* Druidic Satchel	|3
	 * Artifact
	 * |2, |T: Reveal the top card of your library. If it's a creature card, put a 1/1 |Sgreen Saproling creature token onto the battlefield. If it's a land card, put that card onto the battlefield under your control. If it's a noncreature, nonland card, you gain 2 life. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int *deck = deck_ptr[player];
		show_deck( HUMAN, deck, 1, "Here's the first card of the deck", 0, 0x7375B0 );
		int mode = 0;
		if( cards_data[deck[0]].type & TYPE_CREATURE ){
			mode |= 1;
		}
		if( cards_data[deck[0]].type & TYPE_LAND ){
			mode |= 2;
		}

		if( mode & 1 ){
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
		}
		if( mode & 2 ){
			int card_added = add_card_to_hand(player, deck[0]);
			remove_card_from_deck(player, 0);
			put_into_play(player, card_added);
		}
		if( mode == 0 ){
			gain_life(player, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
}

int card_garruk_primal_hunter(int player, int card, event_t event){

	/* Garruk, Primal Hunter	|2|G|G|G
	 * Planeswalker - Garruk (3)
	 * +1: Put a 3/3 |Sgreen Beast creature token onto the battlefield.
	 * -3: Draw cards equal to the greatest power among creatures you control.
	 * -6: Put a 6/6 |Sgreen Wurm creature token onto the battlefield for each land you control. */

	if (IS_ACTIVATING(event)){

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);

		int priority_draw = 0;
		if( event == EVENT_ACTIVATE ){
			priority_draw = (2*check_battlefield_for_special_card(player, card, player, CBFSC_GET_MAX_POW, &this_test))-
							(3*((count_counters(player, card, COUNTER_LOYALTY)*10)-30));
		}

		int priority_wurm = 0;
		if( event == EVENT_ACTIVATE ){
			priority_draw = (4*count_subtype(player, TYPE_LAND, -1))-
							(3*((count_counters(player, card, COUNTER_LOYALTY)*10)-60));
		}

		enum{
			CHOICE_BEAST = 1,
			CHOICE_DRAW,
			CHOICE_WURMFEST
		}
		choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
						"Generate a Beast", 1, 10, 1,
						"Draw cards", 1, priority_draw, -3,
						"Wurm fest", 1, priority_wurm, -6);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
			switch (choice)
			{
				case CHOICE_BEAST:
					generate_token_by_id(player, card, CARD_ID_BEAST);
					break;

				case CHOICE_DRAW:
					draw_cards(player, check_battlefield_for_special_card(player, card, player, CBFSC_GET_MAX_POW, &this_test));
					break;

				case CHOICE_WURMFEST:
					generate_tokens_by_id(player, card, CARD_ID_WURM, count_subtype(player, TYPE_LAND, -1));
					break;
			}
		}
	}

	return planeswalker(player, card, event, 3);
}


int card_gideons_avenger(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_TAP_CARD && affected_card_controller == 1-player && ! is_humiliated(player, card) &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  ){
		instance->targets[1].card = 66;
		if( instance->targets[1].card == 66 ){
			add_1_1_counter(player, card);
			instance->targets[1].card = 0;
		}
	}

	return 0;
}

int card_grand_abolisher(int player, int card, event_t event){

  if (event == EVENT_MODIFY_COST_GLOBAL)
	{
	  if (current_turn == player && in_play(player, card) && affected_card_controller == 1-player && !is_what(affected_card_controller, affected_card, TYPE_LAND) && !is_humiliated(player, card))
		infinite_casting_cost();
	  return 0;
	}

  card_instance_t* instance = get_card_instance(player, card);
  if (event == EVENT_RESOLVE_SPELL)
	instance->targets[2].player = 0;

  if (event == EVENT_RESOLVE_SPELL || event == EVENT_BEGIN_TURN || leaves_play(player, card, event))
	{
	  int mode = -1;
	  if (event == EVENT_RESOLVE_SPELL || event == EVENT_BEGIN_TURN)
		{
		  if (current_turn == player && instance->targets[2].player == 0 && !is_humiliated(player, card))
			mode = 1;
		  if (current_turn == 1-player && instance->targets[2].player == 1)
			mode = 0;
		}
	  else	// leaving play
		{
		  if (instance->targets[2].player == 1)
			mode = 0;
		}

	  if (mode >= 0)
		{
		  instance->targets[2].player = mode;
		  int c;
		  for (c = 0; c < active_cards_count[1-player]; ++c)
			if (in_play(1-player, c) && is_what(1-player, c, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_ENCHANTMENT))
			  disable_all_activated_abilities(1-player, c, mode);
		}
	}

  return 0;
}

int card_guardians_pledge(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		pump_color_until_eot(player, card, player, COLOR_TEST_WHITE, 2, 2, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_griffin_rider(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			if( check_battlefield_for_subtype(player, TYPE_CREATURE, SUBTYPE_GRIFFIN) ){
				event_result +=3;
			}
		}

		if( event == EVENT_ABILITIES && affect_me(player, card) ){
			if( check_battlefield_for_subtype(player, TYPE_CREATURE, SUBTYPE_GRIFFIN) ){
				event_result |= KEYWORD_FLYING;
			}
		}
	}

	return 0;
}

int card_hideous_visage(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					pump_ability_until_eot(player, card, player, count, 0, 0, 0, SP_KEYWORD_INTIMIDATE);
				}
				count--;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

static int hi_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if ( damage_dealt_by_me_arbitrary(instance->targets[0].player, instance->targets[0].card, event,
									 DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_DAMAGE_PLANESWALKER | DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_REPORT_DAMAGE_DEALT,
									 player, card)
	  ){
		draw_cards(player, instance->targets[16].player);
		instance->targets[16].player = 0;
	}

	if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_hunters_insight(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int legacy = create_legacy_effect(player, card, &hi_legacy);
			get_card_instance(player, legacy)->targets[0] = instance->targets[0];
			get_card_instance(player, legacy)->number_of_targets = 1;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature you control.", 1, NULL);
}


int card_jace_memory_adept(int player, int card, event_t event){

	/* Jace, Memory Adept	|3|U|U
	 * Planeswalker - Jace (4)
	 * +1: Draw a card. Target player puts the top card of his or her library into his or her graveyard.
	 * 0: Target player puts the top ten cards of his or her library into his or her graveyard.
	 * -7: Any number of target players each draw twenty cards. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		enum{
			CHOICE_DRAW_MILL = 1,
			CHOICE_MILL10,
			CHOICE_DRAW7
		};
		int mill10_priority = would_validate_arbitrary_target(&td, 1-player, -1) ? 40-count_deck(1-player) : 0;
		int draw7_priority = would_validate_arbitrary_target(&td, player, -1) && count_deck(player) > 30 ? 42-(hand_count[player]*6) : 0;
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
						"Draw & Mill", can_target(&td), 35-(hand_count[player]*5), 1,
						"Mill 10", can_target(&td), mill10_priority, 0,
						"Tutor land and animate them", can_target(&td), draw7_priority, -7);

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
			  case CHOICE_DRAW_MILL:
			  case CHOICE_MILL10:
				pick_target(&td, "TARGET_PLAYER");
				break;

			  case CHOICE_DRAW7:
				{
					if( player == HUMAN ){
						if( pick_target(&td, "TARGET_PLAYER") ){
							new_pick_target(&td, "TARGET_PLAYER", 1, 0);
							if( instance->targets[1].player == instance->targets[0].player ){
								instance->number_of_targets--;
							}
						}
					}
				}
				break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_DRAW_MILL:
				draw_cards(player, 1);
				if ( valid_target(&td)){
					mill(instance->targets[0].player, 1);
				}
				break;

			case CHOICE_MILL10:
				if ( valid_target(&td)){
					mill(instance->targets[0].player, 10);
				}
				break;

			case CHOICE_DRAW7:
				{
					int i;
					for(i=0; i<instance->number_of_targets; i++){
						if( validate_target(player, card, &td, i) ){
							draw_cards(instance->targets[0].player, 20);
						}
					}
				}
				break;
		  }
	}

	return planeswalker(player, card, event, 4);
}

int card_jaces_archivist(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int to_draw = MAX(hand_count[player], hand_count[1-player]);
		APNAP(p,{
					new_discard_all(p, player);
					draw_cards(p, to_draw);
				};
		);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_U(1), 0, NULL, NULL);
}

int card_jade_mage(int player, int card, event_t event){
	/* Jade Mage	|1|G
	 * Creature - Human Shaman 2/1
	 * |2|G: Put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	return mages(player, card, event, COLOR_GREEN);
}

int card_lord_of_the_unreal(int player, int card, event_t event)
{
  // Illusion creatures you control get +1/+1 and have hexproof.
  boost_subtype(player, card, event, SUBTYPE_ILLUSION, 1,1, 0,SP_KEYWORD_HEXPROOF, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
  return 0;
}

int card_onyx_mage(int player, int card, event_t event){
	/* Onyx Mage	|1|B
	 * Creature - Human Wizard 2/1
	 * |1|B: Target creature you control gains deathtouch until end of turn. */

	return mages(player, card, event, COLOR_BLACK);
}


int card_personal_sanctuary(int player, int card, event_t event){

	if( current_turn == player && event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player &&
				damage->info_slot > 0 ){
				damage->info_slot = 0;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_phantasmal_image(int player, int card, event_t event)
{
  /* Phantasmal Image	|1|U
   * Creature - Illusion 0/0
   * You may have ~ enter the battlefield as a copy of any creature on the battlefield, except it's an Illusion in addition to its other types and it gains
   * "When this creature becomes the target of a spell or ability, sacrifice it." */

  if (enters_the_battlefield_as_copy_of_any_creature(player, card, event))
	{
	  add_a_subtype(player, card, SUBTYPE_ILLUSION);
	  set_legacy_image(player, CARD_ID_PHANTASMAL_IMAGE, create_targetted_legacy_effect(player, card, attached_creature_gains_sacrifice_when_becomes_target, player, card));
	}

  return 0;
}

int card_pride_guardian(int player, int card, event_t event){

	if( current_turn == 1-player && event == EVENT_DECLARE_BLOCKERS && blocking(player, card, event) && ! is_humiliated(player, card) ){
		gain_life(player, 3);
	}

	return 0;
}

int card_primordial_hydra(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
		enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, instance->info_slot);
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int number = count_1_1_counters(player, card);
		add_1_1_counters(player, card, number);
	}

	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( count_1_1_counters(player, card) > 9 ){
			event_result |= KEYWORD_TRAMPLE;
		}
	}

	return 0;
}

int card_rites_of_flourishing(int player, int card, event_t event){

	check_playable_lands(current_turn);

	if( event == EVENT_DRAW_PHASE ){
		event_result++;
	}

   return global_enchantment(player, card, event);
}

int card_rune_scarred_demon(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		global_tutor(player, player, 1, TUTOR_HAND, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_skinshifter(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if ((event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST) &&
		generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_ONCE_PER_TURN, MANACOST_G(1), 0, NULL, NULL)
	  ){
		return event == EVENT_POW_BOOST ? 3 : 7;	// 1/1 => 4/4 or 0/8
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	enum{
		CHOICE_RHINO = 1,
		CHOICE_BIRD,
		CHOICE_PLANT
	};

	if( event == EVENT_ACTIVATE){
		instance->info_slot = 0;
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"4/4 Rhino", 1, current_turn == player && is_attacking(player, card) && current_phase == PHASE_AFTER_BLOCKING ? 10 : 0,
							"2/2 Bird", 1, current_turn == player && current_phase < PHASE_DECLARE_BLOCKERS ? 10 : 0,
							"0/8 Plant", 1, current_turn != player ? 10 : 0
		);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		instance->info_slot = choice;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == CHOICE_RHINO ){
			set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
											4, 4, KEYWORD_TRAMPLE, 0, 1);
			force_a_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_RHINO);
		}
		if( instance->info_slot == CHOICE_BIRD ){
			set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
											2, 2, KEYWORD_FLYING, 0, 1);
			force_a_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_BIRD);
		}
		if( instance->info_slot == CHOICE_PLANT ){
			set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
											0, 8, 0, 0, 1);
			force_a_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_PLANT);
		}
	}

	if( event == EVENT_CLEANUP ){
		reset_subtypes(player, card, 1);
	}

	return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST_G(1), 0, NULL, NULL);
}

int card_spirit_mantle(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player != - 1 ){
		protection_from_creatures(instance->damage_target_player, instance->damage_target_card, event);
	}

	return generic_aura(player, card, event, player, 1, 1, 0, 0, 0, 0, 0);
}

int card_stonehorn_dignitary(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		target_player_skips_his_next_attack_step(player, card, 1-player, 0);
	}
	return 0;

}

int card_timely_reinforcements(int player, int card, event_t event){
	/* Timely Reinforcements	|2|W
	 * Sorcery
	 * If you have less life than an opponent, you gain 6 life. If you control fewer creatures than an opponent, put three 1/1 |Swhite Soldier creature tokens onto the battlefield. */

	if( event == EVENT_RESOLVE_SPELL ){
		if( life[player] < life[1-player] ){
			gain_life(player, 6);
		}

		if( count_permanents_by_type(player, TYPE_CREATURE) < count_permanents_by_type(1-player, TYPE_CREATURE) ){
			generate_tokens_by_id(player, card, CARD_ID_SOLDIER, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_vengeful_pharaoh(int player, int card, event_t event){
	// the graveyard effect is in "rules_engine.c" and "planeswalkers.c"

	deathtouch(player, card, event);

	return 0;
}

int card_vampire_outcasts(int player, int card, event_t event){

	bloodthirst(player, card, event, 2);

	lifelink(player, card, event);

	return 0;
}

int card_worldslayer(int player, int card, event_t event)
{
  /* Worldslayer	|5
   * Artifact - Equipment
   * Whenever equipped creature deals combat damage to a player, destroy all permanents other than ~.
   * Equip |5 */

  if (equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "");
	  test.not_me = 1;

	  APNAP(p, {new_manipulate_all(player, card, p, &test, KILL_DESTROY);};);
	}

  return basic_equipment(player, card, event, 5);
}

int card_crown_of_empires(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( empire_set(player) ){
				gain_control(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			}
			else{
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_CREATURE");
}

int card_scepter_of_empires(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, empire_set(player) ? 3 : 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PLAYER");
}

int card_throne_of_empires(int player, int card, event_t event){
	/* Throne of Empires	|4
	 * Artifact
	 * |1, |T: Put a 1/1 |Swhite Soldier creature token onto the battlefield. Put five of those tokens onto the battlefield instead if you control artifacts named Crown of Empires and Scepter of Empires. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SOLDIER, &token);
		token.pow = token.tou = 1;
		token.qty = empire_set(player) ? 5 : 1;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(1), 0, NULL, NULL);
}

int card_stave_off(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int kw = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, kw, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_aven_fleetwing(int player, int card, event_t event){

	/* Ascended Lawmage	|2|W|U
	 * Creature - Vedalken Wizard 3/2
	 * Flying, hexproof */

	/* Aven Fleetwing	|3|U
	 * Creature - Bird Soldier 2/2
	 * Flying
	 * Hexproof */

	/* Bassara Tower Archer	|G|G
	 * Creature - Human Archer 2/1
	 * Hexproof, reach */

	/* Benthic Giant	|5|U
	 * Creature - Giant 4/5
	 * Hexproof */

	/* Gladecover Scout	|G
	 * Creature - Elf Scout 1/1
	 * Hexproof */

	/* Plated Slagwurm	|4|G|G|G
	 * Creature - Wurm 8/8
	 * Hexproof */

	/* Primal Huntbeast	|3|G
	 * Creature - Beast 3/3
	 * Hexproof */

	/* Rubbleback Rhino	|4|G
	 * Creature - Rhino 3/4
	 * Hexproof */

	/* Sacred Wolf	|2|G
	 * Creature - Wolf 3/1
	 * Hexproof */

	hexproof(player, card, event);

	return 0;
}

int card_belltower_sphinx(int player, int card, event_t event){

	if( event == EVENT_DEAL_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player &&
				damage->info_slot > 0 ){
				mill(damage->damage_source_player, damage->info_slot);
			}
		}
	}

	return 0;
}

int card_chasm_drake(int player, int card, event_t event)
{
  // Whenever ~ attacks, target creature you control gains flying until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;

	  if (can_target(&td))
		{
		  if (player == AI && !(td.illegal_abilities & KEYWORD_FLYING))
			{
			  td.illegal_abilities |= KEYWORD_FLYING;
			  if (!can_target(&td))
				td.illegal_abilities &= ~KEYWORD_FLYING;
			}

		  if (pick_target(&td, "ASHNODS_BATTLEGEAR"))	// Select target creature you control.
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
		}
	}

  return 0;
}

int card_djinn_of_wishes(int player, int card, event_t event){

	/* Djinn of Wishes	|3|U|U
	 * Creature - Djinn 4/4
	 * Flying
	 * ~ enters the battlefield with three wish counters on it.
	 * |2|U|U, Remove a wish counter from ~: Reveal the top card of your library. You may play that card without paying its mana cost. If you don't, exile
	 * it. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_WISH, 3);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int *deck = deck_ptr[player];
		card_ptr_t* c = cards_ptr[ cards_data[deck[0]].id ];
		char buffer[100];
		snprintf(buffer, 100, " Play %s\n Pass", c->name );
		int choice = do_dialog(player, player, card, -1, -1, buffer, 0);
		if( choice == 0 ){
			play_card_in_deck_for_free(player, player, 0);
		}
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XU(2,2), GVC_COUNTER(COUNTER_WISH), NULL, NULL);
}

int card_frost_breath(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				does_not_untap_effect(player, card, instance->targets[i].player, instance->targets[i].card, EDNT_TAP_TARGET, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_master_thief(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && comes_into_play(player, card, event) &&
			new_pick_target(&td, "Select target artifact your opponent controls.", 0, GS_LITERAL_PROMPT)
		  ){
			gain_control_until_source_is_in_play_and_tapped(player, card, instance->targets[0].player, instance->targets[0].card, GCUS_CONTROLLED);
		}
	}

	return 0;
}

int card_merfolk_mesmerist(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_U(1), 0, &td, "TARGET_PLAYER");
}

int card_mind_unbound(int player, int card, event_t event){

	/* Mind Unbound	|4|U|U
	 * Enchantment
	 * At the beginning of your upkeep, put a lore counter on ~, then draw a card for each lore counter on ~. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_LORE);
		draw_cards(player, count_counters(player, card, COUNTER_LORE));
	}

	return global_enchantment(player, card, event);
}

int card_skywinder_drake(int player, int card, event_t event ){

	if( event == EVENT_BLOCK_LEGALITY && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( ! check_for_ability(attacking_card_controller, attacking_card, KEYWORD_FLYING) ){
			event_result = 1;
		}
	}

 return 0;
}

int card_sphinx_of_uthuun(int player, int card, event_t event ){

	if (comes_into_play(player, card, event)){
		effect_fof(player, player, 5, TUTOR_GRAVE);
	}

	return 0;
}

int card_turn_into_frog(int player, int card, event_t event){
	/*
	  Turn to Frog |1|U
	  Instant
	  Target creature loses all abilities and becomes a 1/1 blue Frog until end of turn.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t hc;
			default_test_definition(&hc, 0);
			hc.power = 1;
			hc.toughness = 1;
			hc.subtype = SUBTYPE_FROG;
			hc.color = COLOR_TEST_BLUE;
			force_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_FROG);
			humiliate_and_set_pt_abilities(player, card, instance->targets[0].player, instance->targets[0].card, 4, &hc);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_visions_of_beyond(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		if( count_graveyard(player) > 19 ||  count_graveyard(1-player) > 19 ){
			draw_cards(player, 3);
		}
		else{
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_bloodrage_vampire(int player, int card, event_t event){

	bloodthirst(player, card, event, 1);

	return 0;
}

int card_brink_of_disaster(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->targets[0].player != - 1 && instance->targets[0].card !=-1 && ! is_humiliated(player, card) ){
		if( event == EVENT_TAP_CARD ){
			if( affect_me(instance->targets[0].player, instance->targets[0].card) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_LAND);

	return targeted_aura_custom_prompt(player, card, event, &td, "Select target creature or land.");
}

int card_dark_favor(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( life[player] < 7 ){
			ai_modifier -=1000;
		}
	}

	if( comes_into_play(player, card, event) ){
		lose_life(player, 1);
	}

	return generic_aura(player, card, event, player, 3, 1, 0, 0, 0, 0, 0);
}

int card_devouring_swarm(int player, int card, event_t event){
	/* Devouring Swarm	|1|B|B
	 * Creature - Insect 2/1
	 * Flying
	 * Sacrifice a creature: ~ gets +1/+1 until end of turn. */
	/* Flesh-Eater Imp	|3|B
	 * Creature - Imp 2/2
	 * Flying
	 * Infect
	 * Sacrifice a creature: ~ gets +1/+1 until end of turn. */
	return generic_husk(player, card, event, TYPE_CREATURE, 1, 1, 0, 0);
}

int card_distress(int player, int card, event_t event){

	/* Distress	|B|B
	 * Sorcery
	 * Target player reveals his or her hand. You choose a nonland card from it. That player discards that card. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.type_flag = DOESNT_MATCH;

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			new_effect_coercion(&this_definition, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_monomania(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) && hand_count[instance->targets[0].player] > 1 ){
			char msg[100] = "Select a card to keep.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			int selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, 1, AI_MAX_VALUE, -1, &this_test);
			state_untargettable(instance->targets[0].player, selected, 1);
			int count = active_cards_count[instance->targets[0].player]-1;
			while( count > -1 ){
					if( in_hand(instance->targets[0].player, count) ){
						if( check_state(instance->targets[0].player, count, STATE_CANNOT_TARGET) ){
							remove_state(instance->targets[0].player, count, STATE_CANNOT_TARGET);
						}
						else{
							new_discard_card(instance->targets[0].player, count, player, 0);
						}
					}
					count--;
				}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}


static int pingers(int player, int card, event_t event, int life_to_drain){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, life_to_drain, player, card);
			gain_life(player, life_to_drain);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_sorins_vengeance(int player, int card, event_t event){

	return pingers(player, card, event, 10);
}

int card_taste_of_blood(int player, int card, event_t event){

	return pingers(player, card, event, 1);
}

int card_tormented(int player, int card, event_t event){

	cannot_block(player, card, event);
	unblockable(player, card, event);

	return 0;
}

int card_wring_flesh(int player, int card, event_t event){
	return vanilla_instant_pump(player, card, event, ANYBODY, 1-player, -3, -1, 0, 0);
}

int card_circle_of_flame(int player, int card, event_t event)
{
  // Whenever a creature without flying attacks you or a planeswalker you control, ~ deals 1 damage to that creature.
  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.keyword = KEYWORD_FLYING;
	  test.keyword_flag = DOESNT_MATCH;

	  card_instance_t* instance = get_card_instance(player, card);
	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
	  int amt;
	  if ((amt = declare_attackers_trigger_test(player, card, event, DAT_TRACK, 1-player, -1, &test)))
		for (--amt; amt >= 0; --amt)
		  if (in_play(current_turn, attackers[amt]))
			damage_creature(current_turn, attackers[amt], 1, player, card);
	}

  return global_enchantment(player, card, event);
}

int card_furyborn_hellkite(int player, int card, event_t event){

	bloodthirst(player, card, event, 6);

	return 0;
}

int card_goblin_bangchuckers(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE){
		instance->number_of_targets = instance->info_slot = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			instance->info_slot = flip_a_coin(player, card);
			if( instance->info_slot == 1 ){
				pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
			}
			if( spell_fizzled != 1){
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( valid_target(&td) ){
				damage_creature_or_player(player, instance->parent_card, event, 2);
			}
		}
		else{
			damage_creature(instance->parent_controller, instance->parent_card, 2, instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_goblin_fireslinger(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PLAYER");
}

int card_gorehorn_minotaur(int player, int card, event_t event){

	bloodthirst(player, card, event, 2);

	return 0;
}

extern int suppress_legend_rule;
int card_scrambleverse(int player, int card, event_t event){
	/*
	  Scrambleverse
	  Sorcery, 6RR (8)
	  For each nonland permanent, choose a player at random. Then each player gains control of each permanent for which he or she was chosen.
	  Untap those permanents.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		suppress_legend_rule = 1;

		APNAP(p,{
					int count = 0;
					while( count < active_cards_count[p] ){
							if( in_play(p, count) && is_what(p, count, TYPE_PERMANENT) && ! is_what(p, count, TYPE_LAND) ){
								int number = internal_rand(2);
								if( number == 1 ){
									state_untargettable(p, count, 1);
								}
							}
							count++;
					};
				};
		);

		APNAP(p,{
					int count = active_cards_count[p]-1;
					while( count > -1 ){
							if( in_play(p, count) && is_what(p, count, TYPE_PERMANENT) && check_state(p, count, STATE_CANNOT_TARGET) ){
								remove_state(p, count, STATE_CANNOT_TARGET);
								add_state(p, count, STATE_TARGETTED);
								give_control(player, card, p, count);
							}
							count--;
					};
				};
		);

		APNAP(p,{
					int count = 0;
					while( count < active_cards_count[p] ){
							if( in_play(p, count) && is_what(p, count, TYPE_PERMANENT) && check_state(p, count, STATE_TARGETTED) ){
								remove_state(p, count, STATE_TARGETTED);
								untap_card(p, count);
							}
							count++;
					};
				};
		);

		suppress_legend_rule = 0;

		APNAP(p,{
					int count = active_cards_count[p]-1;
					while( count > -1 ){
							if( in_play(p, count) && is_what(p, count, TYPE_PERMANENT) && is_legendary(p, count) ){
								verify_legend_rule(p, count, get_id(p, count));
							}
							count--;
					};
				};
		);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_stormblood_berserker(int player, int card, event_t event){

	bloodthirst(player, card, event, 2);

	minimum_blockers(player, card, event, 2);

	return 0;
}

int card_tectonic_rift(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			this_test.keyword_flag = 1;
			APNAP(p, {pump_creatures_until_eot(player, card, p, 0, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK, &this_test);};);

		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
}

int card_warstorm_surge(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) &&
		reason_for_trigger_controller == player && trigger_cause_controller == player
	  ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		instance->number_of_targets = 0;

		if( can_target(&td) ){
			if( specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					damage_target0(player, card, get_power(instance->targets[1].player, instance->targets[1].card));
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_arachnus_spinner(int player, int card, event_t event){
	/*
	  Arachnus Spinner |5|G
	  Creature - Spider 5/7
	  Reach (This creature can block creatures with flying.)
	  Tap an untapped Spider you control: Search your graveyard and/or library for a card named Arachnus Web and put it onto the battlefield
	  attached to target creature. If you search your library this way, shuffle it.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.required_subtype = SUBTYPE_SPIDER;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL) ){
			if( can_target(&td) ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int spider = -1;
			if( new_pick_target(&td, "Select an untapped Spider you control.", 0, 1 | GS_LITERAL_PROMPT) ){
				spider = instance->targets[0].card;
				instance->number_of_targets = 0;
				if( pick_target(&td1, "TARGET_CREATURE") ){
					tap_card(player, spider);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			char msg[100] = "Select a card named Arachnus Web.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			this_test.id = CARD_ID_ARACHNUS_WEB;

			int result = -1;
			if( new_special_count_grave(player, &this_test) ){
				result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_FIRST_FOUND, &this_test);
			}
			if( result == -1 ){
				result = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
			}
			if( result > -1 ){
				put_into_play_aura_attached_to_target(player, result, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_arachnus_web(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 && ! is_humiliated(player, card) ){
		if(trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player){
			if( get_power(instance->damage_target_player, instance->damage_target_card) > 3 ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						kill_card(player, card, KILL_SACRIFICE);
				}
			}
		}
	}

	return card_arrest(player, card, event);
}

int card_carnage_wurm(int player, int card, event_t event){

	bloodthirst(player, card, event, 3);

	return 0;
}

int card_dungrove_elder(int player, int card, event_t event){

	hexproof(player, card, event);

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) && player != -1 ){
		event_result += count_subtype(player, TYPE_PERMANENT, get_hacked_subtype(player, card, SUBTYPE_FOREST));
	}

	return 0;
}

int card_garruks_horde(int player, int card, event_t event){

	/* Garruk's Horde	|5|G|G
	 * Creature - Beast 7/7
	 * Trample
	 * Play with the top card of your library revealed.
	 * You may cast the top card of your library if it's a creature card. */

	reveal_top_card(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		return (deck_ptr[player][0] != -1 && is_what(-1, deck_ptr[player][0], TYPE_CREATURE) &&
				can_legally_play_iid_now(player, deck_ptr[player][0], event) && has_mana_to_cast_iid(player, event, deck_ptr[player][0]));
	}

	if( event == EVENT_ACTIVATE ){
		int *deck = deck_ptr[player];
		if( charge_mana_from_id(player, -1, event, cards_data[deck[0]].id) ){
			play_card_in_deck_for_free(player, player, 0);
			cant_be_responded_to = 1;
		}
	}

	return 0;
}

int card_stingerfling_spider(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_abilities = KEYWORD_FLYING;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( comes_into_play_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}
	return 0;
}

int card_trollhide(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if ( instance->damage_target_player > -1 && ! is_humiliated(player, card) && IS_GAA_EVENT(event) ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( can_regenerate(p, c) ){
				regenerate_target(p, c);
			}
		}
		return granted_generic_activated_ability(player, card, p, c, event, GAA_REGENERATION, MANACOST_XG(1, 1), 0, NULL, NULL);
	}

	return generic_aura(player, card, event, player, 2, 2, 0, 0, 0, 0, 0);
}

int card_crumbling_colossus(int player, int card, event_t event)
{
  // When ~ attacks, sacrifice it at end of combat.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	create_targetted_legacy_effect(player, card, &sacrifice_at_end_of_combat, player, card);

  return 0;
}

int card_greatsword(int player, int card, event_t event){

  return vanilla_equipment(player, card, event, 3, 3, 0, 0, 0);
}


int card_kite_shield(int player, int card, event_t event){

  return vanilla_equipment(player, card, event, 3, 0, 3, 0, 0);
}

int card_swiftfoot_boots(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 1, 0,0, 0,SP_KEYWORD_HASTE|SP_KEYWORD_HEXPROOF);
}

int card_buried_ruin(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	char msg[100] = "Select an artifact card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(3), 0, NULL, NULL) &&
			count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 && ! graveyard_has_shroud(player)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Get an artifact\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
				if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
					instance->info_slot = 1;
					kill_card(player, card, KILL_SACRIFICE);
				}
				else{
					spell_fizzled = 1;
					remove_state(player, card, STATE_TAPPED);
				}
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			int selected = validate_target_from_grave_source(player, card, player, 0);
			if( selected != -1 ){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
		}
	}

	return 0;
}

int card_lifelink(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 0, 0, 0, SP_KEYWORD_LIFELINK, 0, 0, 0);
}

int card_levitation(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && affected_card_controller == player && ! is_humiliated(player, card) ){
		if( is_what(affected_card_controller,affected_card, TYPE_CREATURE) ){
			event_result |= KEYWORD_FLYING;
		}
	}
	return global_enchantment(player, card, event);
}

int card_pentavus(int player, int card, event_t event)
{
  /* Pentavus	|7
   * Artifact Creature - Construct 0/0
   * ~ enters the battlefield with five +1/+1 counters on it.
   * |1, Remove a +1/+1 counter from ~: Put a 1/1 colorless Pentavite artifact creature token with flying onto the battlefield.
   * |1, Sacrifice a Pentavite: Put a +1/+1 counter on ~. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 5);

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !CAN_ACTIVATE(player, card, MANACOST_X(1)))
		return 0;

	  test_definition_t test;
	  new_default_test_definition(&test, 0, "Select a Pentavite to sacrifice.");
	  test.subtype = SUBTYPE_PENTAVITE;

	  enum
	  {
		CHOICE_TOKEN = 1,
		CHOICE_COUNTER
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Counter to Pentavite", count_counters(player, card, COUNTER_P1_P1), 1,
						"Pentavite to counter", new_can_sacrifice_as_cost(player, card, &test), 1);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_TOKEN:
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(1)))
				remove_counter(player, card, COUNTER_P1_P1);
			  break;

			case CHOICE_COUNTER:
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(1)))
				new_sacrifice(player, card, player, 0, &test);
			  break;
		  }
	  else	// EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_TOKEN:
			  generate_token_by_id(player, card, CARD_ID_PENTAVITE);
			  break;

			case CHOICE_COUNTER:
			  add_counter(player, card, COUNTER_P1_P1);
			  break;
		  }
	}

  return 0;
}

