#include "manalink.h"

// Functions
static int graft_trigger(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) && reason_for_trigger_controller == player &&
		count_1_1_counters(player, card) > 0
	  ){
		int trig_mode = player == AI ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
		if( player == AI ){
			if( (is_what(player, card, TYPE_CREATURE) && get_toughness(player, card) <= 1) || trigger_cause_controller != player ){
				trig_mode = 0;
			}
		}
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		if( new_specific_cip(player, card, event, ANYBODY, trig_mode, &this_test) ){
			card_instance_t *instance = get_card_instance(player, card);
			move_counters(instance->targets[1].player, instance->targets[1].card, player, card, COUNTER_P1_P1, 1);
		}
	}

	return 0;
}

int graft(int player, int card, event_t event, int amount){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, amount);

	graft_trigger(player, card, event);

	return 0;
}

void protection_from_multicolored(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( count_colors(affected_card_controller, affected_card) > 1 ){
				event_result =  1;
			}
		}
	}
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == player &&
			damage->damage_target_card == card && damage->info_slot > 0 && count_colors(damage->damage_source_player, damage->damage_source_card) > 1
		  ){
			damage->info_slot = 0;
		}
	}
}

const char* is_multicolored(int who_chooses, int player, int card){
	if( count_colors(player, card) > 1 ){
		return NULL;
	}
	return EXE_STR(0x739060);	// ",color"
}


// Cards
#if 1
int card_aethermages_touch(int player, int card, event_t event){return 0;}
#else
static int aethermages_touch_legacy(int player, int card, event_t event ){
	card_instance_t *instance = get_card_instance(player, card);
	int p = instance->targets[0].player;
	int c = instance->targets[0].card;
	if( eot_trigger(player, card, event) ){
		if( p > -1 && ! check_state(p, c, STATE_OUBLIETTED) ){
			bounce_permanent(p, c);
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_aethermages_touch(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card");
		int result = reveal_top_cards_of_library_and_choose(player, card, player, 4, 0, TUTOR_PLAY, 1, TUTOR_BOTTOM_OF_DECK, 0, &this_test);
		if( result > -1 ){
			create_targetted_legacy_effect(player, card, &aethermages_touch_legacy, player, result);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}
#endif

int card_anthem_of_rakdos(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
	  // Whenever a creature you control attacks, it gets +2/+0 until end of turn and ~ deals 1 damage to you.
	  int amt;
	  if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, player, -1)))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		  for (--amt; amt >= 0; --amt)
			{
			  if (in_play(current_turn, attackers[amt]))
				pump_until_eot(player, card, current_turn, attackers[amt], 2, 0);

			  damage_player(player, 1, player, card);
			}
		}

	  /* Hellbent - As long as you have no cards in hand, if a source you control would deal damage to a creature or player, it deals double that damage to that
	   * creature or player instead. */
	  card_instance_t* damage = damage_being_dealt(event);
	  if (damage
		  && damage->damage_source_player == player
		  && hand_count[player] <= 0
		  && !damage_is_to_planeswalker(damage))
		damage->info_slot *= 2;
	}

	return global_enchantment(player, card, event);
}

int card_aquastrand_spider(int player, int card, event_t event){

	/* Aquastrand Spider	|1|G
	 * Creature - Spider Mutant 0/0
	 * Graft 2
	 * |G: Target creature with a +1/+1 counter on it gains reach until end of turn. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	graft_trigger(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_G(1), 0, &td, "Select target creature with a +1/+1 counter.");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_REACH, 0);
		}
	}

	return 0;
}

int card_avatar_of_discord(int player, int card, event_t event){
	hybrid(player, card, event);
	if( comes_into_play(player, card, event) ){
		int kill_me = 1;
		if( hand_count[player] > 1 ){
			int p_hand[2][hand_count[player]];
			int pc = 0;
			int discarded[2];
			int dc = 0;
			int count = 0;
			while( count < active_cards_count[player] ){
					if( in_hand(player, count) ){
						p_hand[0][pc] = get_original_internal_card_id(player, count);
						p_hand[1][pc] = count;
						pc++;
					}
					count++;
			}
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard.");
			while( dc < 2 ){
					int selected = select_card_from_zone(player, player, p_hand[0], pc, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 ){
						discarded[dc] = p_hand[1][selected];
						dc++;
						for(count = selected; count < pc; count++){
							p_hand[0][count] = p_hand[0][count+1];
							p_hand[1][count] = p_hand[1][count+1];
							pc--;
						}
					}
					else{
						break;
					}
			}
			if( dc == 2 ){
				discard_card(player, discarded[0]);
				discard_card(player, discarded[1]);
				kill_me = 0;
			}
		}
		if( kill_me ){
			kill_card(player, card, KILL_SACRIFICE );
		}
	}

	return 0;
}

int card_azorius_guildmage(int player, int card, event_t event){

  card_instance_t* instance = get_card_instance( player, card );

  hybrid(player, card, event);

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
	if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
	  return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	int can_counter =  has_mana_for_activated_ability(player, card, MANACOST_XU(2,1)) && can_counter_activated_ability(player, card, event, NULL);

	enum
	{
	  CHOICE_TAP = 1,
	  CHOICE_COUNTER = 2
	} choice = DIALOG(player, card, event,
					  "Tap target creature", has_mana_for_activated_ability(player, card, MANACOST_XW(2,1)) && can_target(&td), 1,
					  "Counter activated ability", can_counter, 2);

	if (event == EVENT_CAN_ACTIVATE)
	  return choice ? (can_counter ? 99 : 1) : 0;
	else if (event == EVENT_ACTIVATE)
	  {
		instance->number_of_targets = 0;
		switch (choice)
		  {
			case CHOICE_TAP:
			  if (charge_mana_for_activated_ability(player, card, MANACOST_XW(2,1)))
				pick_target(&td, "TARGET_CREATURE");
			  break;
			case CHOICE_COUNTER:
			  if (charge_mana_for_activated_ability(player, card, MANACOST_XU(2,1)))
				cast_counter_activated_ability(player, card, 0);
			  break;
		  }
	  }
	else	// event == EVENT_RESOLVE_ACTIVATION
	  switch (choice)
		{
		  case CHOICE_TAP:
			if (valid_target(&td))
			  tap_card(instance->targets[0].player, instance->targets[0].card);
			break;
		  case CHOICE_COUNTER:
			resolve_counter_activated_ability(player, card, NULL, 0);
			break;
		}
  }
  return 0;
}

int card_azorius_herald(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 1, 0, 0, 0);

	unblockable(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 1, 0, 0, 0);
		}
	}

	if( comes_into_play(player, card, event) ){
		gain_life(player, 4);
		if( instance->targets[1].player != 2 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_azorius_ploy(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			maze_of_ith_effect(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_beacon_hawk(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_OPPONENT) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_shade(player, card, event, 0, MANACOST_W(1), 0, 1, 0, 0);
}

int card_biomantic_mastery(int player, int card, event_t event){

	hybrid(player, card, event);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( target_available(player, card, &td) > 1 ){
			return 1;
		}
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_PLAYER") ){
			if( instance->targets[0].player == player ){
				instance->targets[1].player = 1-player;
				instance->targets[1].card = -1;
				instance->number_of_targets = 2;
			}
			else{
				instance->targets[1].player = player;
				instance->targets[1].card = -1;
				instance->number_of_targets = 2;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			draw_cards(player, count_subtype(instance->targets[0].player, TYPE_CREATURE, -1));
		}
		if( validate_target(player, card, &td, 1) ){
			draw_cards(player, count_subtype(instance->targets[1].player, TYPE_CREATURE, -1));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_blessing_of_the_nephilim(int player, int card, event_t event){
	card_instance_t *instance= get_card_instance(player, card);
	int amount = 0;
	if( in_play(player, card) && instance->damage_target_player != -1 ){
		amount = count_colors(instance->damage_target_player, instance->damage_target_card);
	}
	return generic_aura(player, card, event, player, amount, amount, 0, 0, 0, 0, 0);
}

int card_bond_of_agony(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL) ){
			return can_pay_life_as_cost_for_spells_or_activated_abilities(player, 1);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int amount = x_value;
		if( can_pay_life(player, amount) ){
			instance->info_slot = amount;
			lose_life(player, instance->info_slot);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		lose_life(1-player, instance->info_slot);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int determined_effect(int player, int card, event_t event){
	type_uncounterable(player, card, event, player, 0, NULL);
	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_bound_determined(int player, int card, event_t event){
	/*
	  Bound |3|B|G
	  Instant
	  Sacrifice a creature. Return up to X cards from your graveyard to your hand, where X is the number of colors that creature was. Exile this card.

	  Determined |U|G
	  Instant
	  Other spells you control can't be countered by spells or abilities this turn.
	  Draw a card.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		int can_play_bound = can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		int can_play_determined = card_on_stack_controller != -1 ? (player == HUMAN ? 99 : 1) : (card_on_stack_controller == player ? 99 : 0);

		generic_split_card(player, card, event, can_play_bound, 1, MANACOST_UG(1, 1), can_play_determined, 1, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF ){
		if( ! sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			spell_fizzled = 1;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	int can_play_bound = can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	int can_play_determined = card_on_stack_controller != -1 ? (player == HUMAN ? 99 : (card_on_stack_controller == player ? 99 : 0)) : 1;

	if(event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ANY, "Select a card");
			int cd = count_graveyard(player);
			int amount = instance->targets[1].player;
			int i;
			for(i=0; i<amount; i++){
				if( amount > 1 ){
					scnprintf(test.message, 100, "Select a card (%d of %d)", i+1, amount);
				}
				if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test) ){
					cd--;
				}
			}
		}
		if( (instance->info_slot & 2) ){
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( ! in_play(player, i) && check_state(player, i, STATE_INVISIBLE) ){
					add_state(player, i, STATE_CANNOT_TARGET);
				}
			}
			create_legacy_effect(player, card, &determined_effect);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_play_bound, 10, MANACOST_UG(1, 1), can_play_determined, 5, 0, "Bound", "Determined");
}

int card_brain_pry(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

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
				if( player == HUMAN ){
					reveal_target_player_hand(opponent);
				}
				int count = active_cards_count[opponent]-1;
				int result = 0;
				while( count > -1 ){
						if( in_hand(opponent, count) && get_id(opponent, count) == id ){
							new_discard_card(opponent, count, player, 0);
							result = 1;
							break;
						}
						count--;
				}
				if( result == 0 ){
					draw_cards(player, 1);
				}
			}

			kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_bronze_bombshell(int player, int card, event_t event)
{
  /* Bronze Bombshell	|4
   * Artifact Creature - Construct 4/1
   * When a player other than ~'s owner controls it, that player sacrifices it. If the player does, ~ deals 7 damage to him or her. */

  if (event == EVENT_STATIC_EFFECTS && is_stolen(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->info_slot == CARD_ID_BRONZE_BOMBSHELL)	// Prevent reentrance
		return 0;
	  instance->info_slot = CARD_ID_BRONZE_BOMBSHELL;
	  damage_player(player, 7, player, card);
	  kill_card(player, card, KILL_SACRIFICE);
	}

  return 0;
}

int card_celestial_ancient(int player, int card, event_t event){
	/* Celestial Ancient	|3|W|W
	 * Creature - Elemental 3/3
	 * Flying
	 * Whenever you cast an enchantment spell, put a +1/+1 counter on each creature you control. */

	if( specific_spell_played(player, card, event, player, 2, TYPE_ENCHANTMENT, F1_NO_PWALKER, 0, 0, 0, 0, 0, 0, -1, 0) ){
		manipulate_type(player, card, player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}
	return 0;
}

int card_coiling_oracle(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		if( count_deck(player) > 0 ){
			int *deck = deck_ptr[player];
			int card_added = add_card_to_hand(player, deck[0] );
			reveal_card(player, card, player, card_added);
			card_data_t* card_d = get_card_data(player, card_added);
			if ( card_d->type & TYPE_LAND ){
				put_into_play(player, card_added);
			}
			remove_card_from_deck( player, 0 );
		}
	}
	return 0;
}

int card_condemn(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life( instance->targets[0].player, get_toughness(instance->targets[0].player, instance->targets[0].card) );
			put_on_bottom_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_court_hussar(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 0, 0, 0, 1);

	vigilance(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 0, 0, 0, 1);
		}
	}

	if( comes_into_play(player, card, event) ){
		select_one_and_put_the_rest_on_bottom(player, player, 3);
		if( instance->targets[1].player != 2 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_crime_punishment(int player, int card, event_t event){
	/*
	  Crime |3|W|B
	  Sorcery
	  Put target creature or enchantment card from an opponent's graveyard onto the battlefield under your control.

	  Punishment |X|B|G
	  Sorcery
	  Destroy each artifact, creature, and enchantment with converted mana cost X.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		int can_play_crime = count_graveyard_by_type(1-player, TYPE_CREATURE | TYPE_ENCHANTMENT) && ! graveyard_has_shroud(1-player);
		generic_split_card(player, card, event, can_play_crime, 1, MANACOST_BG(1, 1), 1, 1, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_ENCHANTMENT, "Select a creature or enchantment card");
		if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 0) == -1 ){
			spell_fizzled = 1;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	int can_play_crime = count_graveyard_by_type(1-player, TYPE_CREATURE | TYPE_ENCHANTMENT) && ! graveyard_has_shroud(1-player);
	int priority_crime = 10;
	int priority_punishment = 5;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( (instance->info_slot & (1<<8)) && player == AI ){
			int p_cmc = get_average_cmc(player);
			int opp_cmc = get_average_cmc(1-player);
			if( p_cmc != opp_cmc && has_mana_multi(player, MANACOST_XBG(opp_cmc, 1, 1)) ){
				priority_punishment = 15;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( 1 ){
			int selected = validate_target_from_grave_source(player, card, 1-player, 0);
			if( selected != -1 ){
				reanimate_permanent(player, card, 1-player, selected, REANIMATE_DEFAULT);
			}
		}
		if( instance->info_slot & 2 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_ENCHANTMENT);
			this_test.cmc = instance->targets[1].card;
			new_manipulate_all(player, card, ANYBODY, &this_test, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_play_crime, priority_crime, MANACOST_XBG(-1, 1, 1), 1, priority_punishment, 0, "Crime", "Punishment");
}

int card_crypt_champion(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 0, 0, 1, 0);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 0, 0, 1, 0);
		}
	}

	if( comes_into_play(player, card, event)  ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card with CMC 3 or less.");
		this_test.cmc = 4;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_VALUE, &this_test);
		}
		if( count_graveyard_by_type(1-player, TYPE_CREATURE) > 0 ){
			new_global_tutor(1-player, 1-player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_VALUE, &this_test);
		}

		if( instance->targets[1].player != 2 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_cytoplast_manipulator(int player, int card, event_t event){

	/* Cytoplast Manipulator	|2|U|U
	 * Creature - Human Wizard Mutant 0/0
	 * Graft 2
	 * |U, |T: Gain control of target creature with a +1/+1 counter on it for as long as ~ remains on the battlefield. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	graft_trigger(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;
	}
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_U(1), 0,
										&td, "Select target creature with a +1/+1 counter.");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			gain_control_until_source_is_in_play_and_tapped(instance->parent_controller, instance->parent_card,
															instance->targets[0].player, instance->targets[0].card, GCUS_CONTROLLED);
		}
	}

	return 0;
}


int card_cytoplast_root_kin(int player, int card, event_t event)
{
  /* Cytoplast Root-Kin	|2|G|G
   * Creature - Elemental Mutant 0/0
   * Graft 4
   * When ~ enters the battlefield, put a +1/+1 counter on each other creature you control that has a +1/+1 counter on it.
   * |2: Move a +1/+1 counter from target creature you control onto ~. */

  graft(player, card, event, 4);

  int c;
  if (comes_into_play(player, card, event))
	for (c = 0; c < active_cards_count[player]; ++c)
	  if (c != card && in_play(player, c) && is_what(player, c, TYPE_CREATURE) && count_1_1_counters(player, c) > 0)
		add_1_1_counter(player, c);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  base_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = player;
  td.preferred_controller = player;
  // It's legal to target itself, or a creature without a +1/+1 counter, but the effect does nothing in that case.
  if (IS_AI(player))
	{
	  td.special = TARGET_SPECIAL_REQUIRES_COUNTER | TARGET_SPECIAL_NOT_ME;
	  td.extra = COUNTER_P1_P1;
	}

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int pp = instance->parent_controller, pc = instance->parent_card;
	  if (in_play(pp, pc))
		move_counters(pp, pc, instance->targets[0].player, instance->targets[0].card, COUNTER_P1_P1, 1);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

int card_cytoshape(int player, int card, event_t event){

	/* Cytoshape	|1|G|U
	 * Instant
	 * Choose a nonlegendary creature on the battlefield. Target creature becomes a copy of that creature until end of turn. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_LEGEND;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td) && can_target(&td1);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->number_of_targets = 0;
			if( pick_target(&td, "SELECT_A_NONLEGENDARY_CREATURE") ){
				/* Stash in damage_source_player/card, which will still get updated if the creature changes control before resolution, but won't be considered
				 * by things that with becomes-targeted triggers.  On the minus side, you can't see what was chosen while this is on the stack. */
				instance->damage_source_player = instance->targets[0].player;
				instance->damage_source_card = instance->targets[0].card;

				instance->number_of_targets = 0;
				if (!pick_target(&td1, "TARGET_CREATURE")){
					instance->damage_source_player = instance->damage_source_card = -1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				shapeshift_target(player, card,
								  instance->targets[0].player, instance->targets[0].card,
								  instance->damage_source_player, instance->damage_source_card,
								  SHAPESHIFT_UNTIL_EOT);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_cytospawn_shambler(int player, int card, event_t event){

	/* Cytospawn Shambler	|6|G
	 * Creature - Elemental Mutant 0/0
	 * Graft 6
	 * |G: Target creature with a +1/+1 counter on it gains trample until end of turn. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 6);

	graft_trigger(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_G(1), 0, &td, "Select target creature with a +1/+1 counter.");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_TRAMPLE, 0);
		}
	}

	return 0;
}

int card_delirium_skeins(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {new_multidiscard(p, 3, 0, player);};);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_demand_supply(int player, int card, event_t event){
	/*
	  Supply |X|W|G
	  Sorcery
	  Put X 1/1 green Saproling creature tokens onto the battlefield.

	  Demand |1|W|U
	  Sorcery
	  Search your library for a multicolored card, reveal it, and put it into your hand. Then shuffle your library.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		return generic_split_card(player, card, event, 1, 0, MANACOST_XUW(1, 1, 1), 1, 0, 0, NULL, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			generate_tokens_by_id(player, card, CARD_ID_SAPROLING, instance->targets[1].card);
		}
		if( instance->info_slot & 2 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			this_test.color_flag = F3_MULTICOLORED;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, 1, 10, MANACOST_XUW(1, 1, 1), 8, 0, 0, "Supply", "Demand");
}

int card_demonfire(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance( player, card );


	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = x_value;
			set_special_flags2(player, card, SF2_X_SPELL);
			if( hand_count[player] < 1 ){
				state_untargettable(player, card, 1);
				instance->targets[1].player = 66;
			}
		}
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL){
		state_untargettable(player, card, 0);
		if( valid_target(&td) ){
			if( instance->targets[1].player == 66 ){
				int legacy = create_legacy_effect(player, card, &my_damage_cannot_be_prevented);
				get_card_instance(player, legacy)->targets[0].player = player;
				get_card_instance(player, legacy)->targets[0].card = card;
			}
			if( instance->targets[0].card != -1 ){
				int dc = damage_target0(player, card, instance->info_slot);
				get_card_instance(player, dc)->targets[3].card |= DMG_EXILE_IF_LETHALLY_DAMAGED_THIS_WAY;
			}
			else{
				damage_player(instance->targets[0].player, instance->info_slot, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_development_research(int player, int card, event_t event){
	/*
	  Development |3|U|R
	  Instant
	  Put a 3/1 red Elemental creature token onto the battlefield unless any opponent has you draw a card. Repeat this process two more times.

	  Research |U|G
	  Instant
	  Choose up to four cards you own from outside the game and shuffle them into your library.
	*/

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		return generic_split_card(player, card, event, 1, 0, MANACOST_UG(1, 1), 1, 0, 0, NULL, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			int i;
			for(i=0; i<3; i++){
				int ai_choice = 1;
				if( hand_count[1-player] < hand_count[player] ){
					ai_choice = 0;
				}
				int choice = do_dialog(1-player, player, card, -1, -1, " Opponent gets an Elemental\n Opponent draws", ai_choice);
				if( choice == 0 ){
					token_generation_t token;
					default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
					token.pow = 3;
					token.tou = 1;
					token.color_forced = COLOR_TEST_RED;
					generate_token(&token);
				}
				else{
					draw_cards(player, 1);
				}
			}
		}
		if( instance->info_slot & 2 ){
			int i;
			int cd = count_deck(player);
			int *deck = deck_ptr[player];
			for(i=0; i<4; i++){
				int crd = -1;
				if( player != AI ){
					crd = card_from_list(player, 4, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				}
				else{
					if( i == 0 ){
						crd = get_internal_card_id_from_csv_id(CARD_ID_SIMIC_SKY_SWALLOWER);
					}
					if( i == 1 ){
						crd = get_internal_card_id_from_csv_id(CARD_ID_REMAND);
					}
					if( i == 2 ){
						crd = get_internal_card_id_from_csv_id(CARD_ID_CHAR);
					}
					if( i == 3 ){
						crd = get_internal_card_id_from_csv_id(CARD_ID_ELECTROLYZE);
					}
				}
				if( crd != -1 ){
					update_rules_engine(check_card_for_rules_engine(get_internal_card_id_from_csv_id(crd)));
					deck[cd] = crd;
					cd++;
				}
			}
			shuffle(player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, 1, 10, MANACOST_UG(1, 1), 1, 8, 0, "Development", "Research");
}


int card_dovescape(int player, int card, event_t event){
	/* Dovescape	|3|WU|WU|WU
	 * Enchantment
	 * Whenever a player casts a noncreature spell, counter that spell. That player puts X 1/1 |Swhite and |Sblue Bird creature tokens with flying onto the battlefield, where X is the spell's converted mana cost. */

	card_instance_t *instance = get_card_instance( player, card );

	hybrid(player, card, event);

	if( specific_spell_played(player, card, event, 2, 2, TYPE_CREATURE, 1, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int amount = get_cmc(instance->targets[1].player, instance->targets[1].card);
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BIRD, &token);
		token.t_player = instance->targets[1].player;
		token.color_forced = COLOR_TEST_WHITE | COLOR_TEST_BLUE;
		token.qty = amount;
		generate_token(&token);
		real_counter_a_spell(player, card, instance->targets[1].player, instance->targets[1].card);
	}

	return global_enchantment(player, card, event);
}

int card_dread_slag(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result -= (4*hand_count[player]);
	}

	return 0;
}

int card_drekavac(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.type_flag = DOESNT_MATCH;
	this_test.zone = TARGET_ZONE_HAND;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			ai_modifier-=100;
		}
	}

	if( comes_into_play(player, card, event) ){
		ec_definition_t this_definition;
		default_ec_definition(player, player, &this_definition);
		this_definition.ai_selection_mode = AI_MIN_VALUE;
		int result = new_effect_coercion(&this_definition, &this_test);
		if( result == -1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_elemental_resonance(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( current_turn == player && current_phase == PHASE_MAIN1 && instance->targets[0].player != 66 ){
			card_ptr_t* c = cards_ptr[ get_id(instance->damage_target_player, instance->damage_target_card) ];
			produce_mana_multi(player, c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white);
		}
	}
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_odds_ends(int player, int card, event_t event){
	/*
	  Odds |U|R
	  Instant
	  Flip a coin. If it comes up heads, counter target instant or sorcery spell. If it comes up tails, copy that spell and you may choose new targets for the copy.

	  Ends |3|W|R
	  Instant
	  Target player sacrifices two attacking creatures.
	*/
	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		counterspell_target_definition(player, card, &td, TYPE_SPELL);
		td.illegal_type = TYPE_CREATURE;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		return generic_split_card(player, card, event, counterspell(player, card, EVENT_CAN_CAST, &td, 0), 0, MANACOST_XRW(3, 1, 1), can_target(&td2), 0, 0, NULL, NULL);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		counterspell_target_definition(player, card, &td, TYPE_SPELL);
		td.illegal_type = TYPE_CREATURE;
		counterspell(player, card, EVENT_CAST_SPELL, &td, 0);
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		pick_target(&td2, "TARGET_PLAYER");
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_SPELL);
	td.illegal_type = TYPE_CREATURE;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && counterspell_validate(player, card, &td, 0) ){
			int result = flip_a_coin(player, card);
			if( ! result ){
				real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			}
			else{
				copy_spell_from_stack(player, instance->targets[0].player,  instance->targets[0].card);
			}
		}
		if( (instance->info_slot & 2) && valid_target(&td2) ){
			if( can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
				target_definition_t td1;
				default_target_definition(player, card, &td1, TYPE_CREATURE );
				td1.allowed_controller = instance->targets[0].player;
				td1.preferred_controller = instance->targets[0].player;
				td1.who_chooses = instance->targets[0].player;
				td1.illegal_abilities = 0;
				td1.required_state = TARGET_STATE_ATTACKING;
				td1.allow_cancel = 0;
				int i;
				 for(i=0; i<2; i++){
					if( can_target(&td1) ){
						pick_target(&td1, "LORD_OF_THE_PIT");
						instance->number_of_targets = 1;
						kill_card( instance->targets[0].player,  instance->targets[0].card, KILL_SACRIFICE);
					}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, counterspell(player, card, EVENT_CAN_CAST, &td, 0), 10, MANACOST_XRW(3, 1, 1), can_target(&td2), 8, 0, "Odds", "Ends");
}

int card_enemy_of_the_guildpact(int player, int card, event_t event){
	protection_from_multicolored(player, card, event);
	return 0;
}

int card_error_trial(int player, int card, event_t event){
	/*
	  Trial |W|U
	  Instant
	  Return all creatures blocking or blocked by target creature to their owner's hand.

	  Error |U|B
	  Instant
	  Counter target multicolored spell.
	*/
	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		target_definition_t td2;
		counterspell_target_definition(player, card, &td2, TYPE_SPELL);
		td2.extra = (int32_t)is_multicolored;
		td2.special = TARGET_SPECIAL_EXTRA_FUNCTION;

		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_UB(1, 1), counterspell(player, card, EVENT_CAN_CAST, &td2, 0), 0, 0, NULL, NULL);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		target_definition_t td2;
		counterspell_target_definition(player, card, &td2, TYPE_SPELL);
		td2.extra = (int32_t)is_multicolored;
		td2.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		counterspell(player, card, EVENT_CAST_SPELL, &td2, 0);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td2;
	counterspell_target_definition(player, card, &td2, TYPE_SPELL);
	td2.extra = (int32_t)is_multicolored;
	td2.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	if(event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && valid_target(&td) ){
			if( current_turn != instance->targets[0].player && get_card_instance(instance->targets[0].player, instance->targets[0].card)->blocking != 255 ){
				bounce_permanent(1-instance->targets[0].player, get_card_instance(instance->targets[0].player, instance->targets[0].card)->blocking);
			}
			if( current_turn == instance->targets[0].player && is_attacking(instance->targets[0].player, instance->targets[0].card) ){
				int i;
				for(i=0; i<active_cards_count[1-instance->targets[0].player]; i++){
					if( in_play(1-instance->targets[0].player, i) ){
						if( get_card_instance(1-instance->targets[0].player, i)->blocking == instance->targets[0].card ){
							bounce_permanent(1-instance->targets[0].player, i);
						}
					}
				}
			}
		}
		if( (instance->info_slot & 2) && counterspell_validate(player, card, &td2, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 8, MANACOST_UB(1, 1), counterspell(player, card, EVENT_CAN_CAST, &td2, 0), 10, 0, NULL, NULL);
}

static int evolution_vat_legacy(int player, int card, event_t event){

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	if( get_card_instance(player, card)->damage_target_player > -1 ){
		card_instance_t *instance = get_card_instance(player, card);

		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int amount = count_1_1_counters(p, c);
			add_1_1_counters(p, c, amount);
		}

		return granted_generic_activated_ability(player, card, p, c, event, 0, MANACOST_XUG(2, 1, 1), 0, NULL, NULL);
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_evolution_vat(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			if( instance->targets[0].player == player ){
				create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &evolution_vat_legacy,
												instance->targets[0].player, instance->targets[0].card);
			}
			else{
				int fake = add_card_to_hand(1-player, get_card_instance(instance->parent_controller, instance->parent_card)->internal_card_id);
				create_targetted_legacy_effect(1-player, fake, &evolution_vat_legacy, instance->targets[0].player, instance->targets[0].card);
				obliterate_card(1-player, fake);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_CREATURE");
}

int card_experiment_kraj(int player, int card, event_t event ){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			int i, result = 0;
			for(i=0; i < 2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && count_1_1_counters(i, count) > 0 &&
							get_id(i, count) != CARD_ID_EXPERIMENT_KRAJ
						  ){
							card_data_t* card_d = get_card_data(i, count);
							int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
							int this_result = ptFunction(player, card, EVENT_CAN_ACTIVATE );
							if( this_result ){
								if( this_result == 99 ){
									return 99;
								}
								result = this_result;
							}
						}
						count--;
				}
			}
			return result;
		}
	}
	if( event == EVENT_ACTIVATE){
		int stolen_abilities[12] = {-1, -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}; //For simplicity sake, let's consider 10 to be the maximum amount of the "stolen" abilities. Position '0' in this array is 'cancel' and position '1' is the main Experiment Kraj ability.
		char stolen_abilities_name[12][100];
		int i, sac = 2;
		for(i=0; i < 2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && count_1_1_counters(i, count) > 0 &&
						get_id(i, count) != CARD_ID_EXPERIMENT_KRAJ
					  ){
						card_data_t* card_d = get_card_data(i, count);
						int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
						if( ptFunction(player, card, EVENT_CAN_ACTIVATE ) ){
							stolen_abilities[sac] = get_card_instance(i, count)->internal_card_id;
							strcpy(stolen_abilities_name[sac], cards_ptr[get_id(i, count)]->name);
							sac++;
						}
					}
					if( sac == 11 ){
						break;
					}
					count--;
			}
			if( sac == 11 ){
				break;
			}
		}
		int ek_ability = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE, DLG_OMIT_ILLEGAL,
						"Give a +1/+1 counter", ek_ability, 5,
						stolen_abilities_name[2], stolen_abilities[2] > -1, 1,
						stolen_abilities_name[3], stolen_abilities[3] > -1, 1,
						stolen_abilities_name[4], stolen_abilities[4] > -1, 1,
						stolen_abilities_name[5], stolen_abilities[5] > -1, 1,
						stolen_abilities_name[6], stolen_abilities[6] > -1, 1,
						stolen_abilities_name[7], stolen_abilities[7] > -1, 1,
						stolen_abilities_name[8], stolen_abilities[8] > -1, 1,
						stolen_abilities_name[9], stolen_abilities[9] > -1, 1,
						stolen_abilities_name[10], stolen_abilities[10] > -1, 1,
						stolen_abilities_name[11], stolen_abilities[11] > -1, 1);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == 1 ){
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
			if( spell_fizzled != 1 ){
				instance->info_slot = 0;
			}
		}
		else{
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[stolen_abilities[choice-1]].code_pointer;
			ptFunction(player, card, EVENT_ACTIVATE );
			if( spell_fizzled != 1 ){
				instance->info_slot = stolen_abilities[choice-1];
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 0 ){
			if( valid_target(&td) ){
				add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			}
		}
		if( instance->info_slot > 0 ){
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[instance->info_slot].code_pointer;
			ptFunction(instance->parent_controller, instance->parent_card, EVENT_RESOLVE_ACTIVATION);
		}
	}

	return 0;
}

int card_fall_rise(int player, int card, event_t event){
	/*
	  Rise |U|B
	  Sorcery
	  Return target creature card from a graveyard and target creature on the battlefield to their owners' hands.

	  Fall |B|R
	  Sorcery
	  Target player reveals two cards at random from his or her hand, then discards each nonland card revealed this way.
	*/

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_PLAYERS;

		int can_play_rise = can_target(&td) && count_graveyard_by_type(2, TYPE_CREATURE) && ! graveyard_has_shroud(2);

		return generic_split_card(player, card, event, can_play_rise, 0, MANACOST_BR(1, 1), can_target(&td2), 0, 0, NULL, NULL);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( pick_target(&td, "TARGET_CREATURE") ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card");
			select_target_from_either_grave(player, card, 0, AI_MAX_VALUE, AI_MIN_VALUE, &this_test, 1, 2);
		}
	}


	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_PLAYERS;

	int can_play_rise = can_target(&td) && count_graveyard_by_type(2, TYPE_CREATURE) && ! graveyard_has_shroud(2);

	if(event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.type_flag = DOESNT_MATCH;

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			this_definition.cards_to_reveal = 2;
			this_definition.effect = EC_ALL_WHICH_MATCH_CRITERIA | EC_DISCARD;
			new_effect_coercion(&this_definition, &this_test);
		}
		if( instance->info_slot & 2 ){
			if( validate_target(player, card, &td2, 0) ){
				bounce_permanent(instance->targets[0].player,  instance->targets[0].card);
			}
			int selected = validate_target_from_grave_source(player, card, instance->targets[1].player, 2);
			if( selected != -1 ){
				from_grave_to_hand(instance->targets[1].player, selected, TUTOR_HAND);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_play_rise, 0, MANACOST_BR(1, 1), can_target(&td2), 0, 0, "Rise", "Fall");
}

int card_flame_kin_war_scout(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  damage_creature(instance->targets[1].player, instance->targets[1].card, 4, player, card);
		  kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_flaring_flame_kin(int player, int card, event_t event)
{
  // As long as ~ is enchanted, it gets +2/+2, has trample, and has "|R: ~ gets +1/+0 until end of turn."
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && is_enchanted(player, card) && ! is_humiliated(player, card) )
	event_result += 2;

  if (event == EVENT_ABILITIES && affect_me(player, card) && is_enchanted(player, card) && ! is_humiliated(player, card) )
	event_result |= KEYWORD_TRAMPLE;

  if ((event == EVENT_CAN_ACTIVATE || event == EVENT_POW_BOOST) && !is_enchanted(player, card))
	return 0;

  return generic_shade(player, card, event, 0, MANACOST_R(1), 1,0, 0,0);
}

int card_freewind_equenaut(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( is_enchanted(player, card) && ! is_humiliated(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( current_turn == player ){
			td.required_state = TARGET_STATE_BLOCKING;
		}
		else{
			td.required_state = TARGET_STATE_ATTACKING;
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, instance->parent_card);
			}
		}
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0,
														&td, "TARGET_CREATURE");
	}

	return 0;
}

int card_gobhobbler_rats(int player, int card, event_t event){
	if( hand_count[player] < 1 && ! is_humiliated(player, card) ){
		modify_pt_and_abilities(player, card, event, 1, 0, KEYWORD_REGENERATION);
		return regeneration(player, card, event, MANACOST_B(1));
	}
	return 0;
}

int card_grand_arbiter_augustin_iv(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	if(event == EVENT_MODIFY_COST_GLOBAL && ! is_humiliated(player, card) ){
		if( affected_card_controller == player ){
			int clr = get_color_by_internal_id(player, get_card_instance(affected_card_controller, affected_card)->internal_card_id);
			if( clr & COLOR_TEST_BLUE ){
				COST_COLORLESS--;
			}
			if( clr & COLOR_TEST_WHITE ){
				COST_COLORLESS--;
			}
		}
		else{
			card_data_t* card_d = get_card_data(affected_card_controller, affected_card);
			if( ! ( card_d->type & TYPE_LAND)  ){
				COST_COLORLESS++;
			}
		}
	}
	return 0;
}

int card_guardian_of_the_guildpact(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( count_colors(affected_card_controller, affected_card) == 1 ){
				event_result =  1;
			}
		}
	}
	if( event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == player &&
			damage->damage_target_card == card && damage->info_slot > 0 && count_colors(damage->damage_source_player, damage->damage_source_card) == 1
		  ){
			damage->info_slot = 0;
		}
	}
	return 0;
}

int card_haazda_shield_mate(int player, int card, event_t event){
	/* Haazda Shield Mate	|2|W
	 * Creature - Human Soldier 1/1
	 * At the beginning of your upkeep, sacrifice ~ unless you pay |W|W.
	 * |W: The next time a source of your choice would deal damage to you this turn, prevent that damage. */

	basic_upkeep(player, card, event, MANACOST_W(2));

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_EFFECT);
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;
	td.extra = damage_card;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			target->info_slot = 0;
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_W(1), 0, &td, "TARGET_DAMAGE");
}

int card_helium_squirter(int player, int card, event_t event){

	/* Helium Squirter	|4|U
	 * Creature - Beast Mutant 0/0
	 * Graft 3
	 * |1: Target creature with a +1/+1 counter on it gains flying until end of turn. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 3);

	graft_trigger(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_X(1), 0, &td, "Select target creature with a +1/+1 counter.");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
		}
	}

	return 0;
}

int card_hellhole_rats(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	haste(player, card, event);

	if( comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, instance->targets[0].player, &this_definition);
			this_definition.ai_selection_mode = AI_MIN_CMC;
			int result = new_effect_coercion(&this_definition, &this_test);
			if( result > -1 ){
				damage_player(instance->targets[0].player, get_cmc_by_internal_id(result), player, card);
			}
		}
	}

	return 0;
}

int card_hide_seek(int player, int card, event_t event){
	/*
	  Hide |W|R
	  Instant
	  Put target artifact or enchantment on the bottom of its owner's library.

	  Seek |W|B
	  Instant
	  Search target opponent's library for a card and exile it. You gain life equal to its converted mana cost. Then that player shuffles his or her library.
	*/
	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );

		target_definition_t td2;
		default_target_definition(player, card, &td2, 0 );
		td2.zone = TARGET_ZONE_PLAYERS;

		int can_play_seek = would_validate_arbitrary_target(&td2, 1-player, -1);
		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_WB(1, 1), can_play_seek, 0, 0, NULL, NULL);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_PLAY_FIRST_HALF ){
		instance->number_of_targets = 0;

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );
		pick_target(&td, "DISENCHANT");
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE );
	td2.zone = TARGET_ZONE_PLAYERS;

	int can_play_seek = would_validate_arbitrary_target(&td2, 1-player, -1);
	int priority_hide = 10;
	int priority_seek = life[player] < 6 ? 15 : 5;

	if( event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && valid_target(&td) ){
			put_on_bottom_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		if( (instance->info_slot & 2) && valid_target(&td2) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			int selected = new_select_a_card(player, instance->targets[0].player, TUTOR_FROM_DECK, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				int amount = get_cmc_by_id(cards_data[deck_ptr[1-player][selected]].id);
				rfg_card_in_deck(1-player, selected);
				gain_life(player, amount);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), priority_hide, MANACOST_WB(1, 1), can_play_seek, priority_seek, 0, "Hide", "Seek");
}

int card_hit_run(int player, int card, event_t event){
	/*
	  Hit |1|B|R
	  Instant
	  Target player sacrifices an artifact or creature. Hit deals damage to that player equal to that permanent's converted mana cost.

	  Run |3|R|G
	  Instant
	  Attacking creatures you control get +1/+0 until end of turn for each other attacking creature.
	*/

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.zone = TARGET_ZONE_PLAYERS;

		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XGR(3, 1, 1), 1, 0, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.zone = TARGET_ZONE_PLAYERS;
		pick_target(&td, "TARGET_PLAYER");
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	int priority_hit = 10;
	int priority_run = 5;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI && current_turn == player ){
		priority_run = 4*count_attackers(player);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && valid_target(&td) ){
			instance->targets[1].card = -1;
			impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE | TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( instance->targets[1].card > -1 ){
				damage_player(instance->targets[0].player, get_cmc_by_id(instance->targets[1].card), player, card);
			}
		}
		if( (instance->info_slot & 2) ){
			int amount = count_attackers(player);
			if( amount > 0 ){
				pump_subtype_until_eot(player, card, player, -1, amount, 0, 0, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), priority_hit, MANACOST_XGR(3, 1, 1), 1, priority_run, 0, "Hit", "Run");
}

int card_indrik_stomphowler(int player, int card, event_t event){
	/*
	  Indrik Stomphowler |4|G
	  Creature - Beast 4/4
	  When Indrik Stomphowler enters the battlefield, destroy target artifact or enchantment.
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ENCHANTMENT | TYPE_ARTIFACT);
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "DISENCHANT") ){
			action_on_target(player, card, 0, KILL_DESTROY);
			get_card_instance(player, card)->number_of_targets = 0;
		}
	}

	return 0;
}


int card_infernal_tutor(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		if( hand_count[player] < 1 ){
			return card_demonic_tutor(player, card, event);
		}
		else{
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ANY, "Select a card");
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MAX_VALUE, -1, &test);
			int csvid = get_id(player, selected);
			reveal_card(player, card, player, selected);

			char buffer[100];
			scnprintf(buffer, 100, "Select a card named %s", cards_ptr[csvid]->name);
			test_definition_t test2;
			new_default_test_definition(&test2, TYPE_ANY, buffer);
			test2.id = csvid;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &test2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_isperia_the_inscrutable(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT|DDBM_MUST_BE_COMBAT_DAMAGE) ){
		int *deck = deck_ptr[1-player];
		int id = cards_data[deck[internal_rand(count_deck(1-player))]].id;
		if( player != AI ){
			char buffer[300];
			int pos = scnprintf(buffer, 300, "Opponent named:");
			card_ptr_t* c_me = cards_ptr[ id ];
			pos += scnprintf(buffer+pos, 300-pos, " %s", c_me->name);
			do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
		}
		else{
			id = card_from_list(player, 4, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		reveal_target_player_hand(1-player);
		if( is_id_in_hand(1-player, id) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

int card_jagged_poppet(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player ){
				int good = 0;
				if( damage->info_slot > 0 ){
					good = damage->info_slot;
				}
				else{
					card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
					if( trg->targets[16].player > 0 ){
						good = trg->targets[16].player;
					}
				}

				if( good > 0 ){
					instance->info_slot+=good;
				}
			}
		}
	}

	if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player && ! is_humiliated(player, card) ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				multidiscard(player, instance->info_slot, 0);
				instance->info_slot = 0;
		}
	}

	if( hand_count[player] < 1 && damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_OPPONENT|DDBM_REPORT_DAMAGE_DEALT)){
		new_multidiscard(1-player, instance->targets[16].player, 0, player);
		instance->targets[16].player = 0;
	}
	return 0;
}

int card_loaming_shaman(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;
		td1.allow_cancel = 0;

		if( can_target(&td1) && pick_target(&td1, "TARGET_PLAYER") ){
			int cg = count_graveyard(instance->targets[0].player)-1;
			if( player == AI ){
				while( cg > -1 ){
						from_graveyard_to_deck(instance->targets[0].player, cg, 1);
						cg--;
				}
				shuffle(instance->targets[0].player);
			}
			else{
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);
				this_test.no_shuffle = 1;
				while( cg > -1 && new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_GRAVE, TUTOR_DECK, 0, AI_MAX_VALUE, &this_test) != -1 ){
						cg--;
				}
				shuffle(instance->targets[0].player);
			}
		}
	}
	return 0;
}

int card_lyzolda_the_blood_witch(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE, MANACOST_X(2), 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, MANACOST_X(2));
		if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				instance->number_of_targets = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( (instance->targets[1].player & COLOR_TEST_RED) && valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
		if( instance->targets[1].player & COLOR_TEST_BLACK ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_macabre_waltz(int player, int card, event_t event)
{
  /* Macabre Waltz	|1|B
   * Sorcery
   * Return up to two target creature cards from your graveyard to your hand, then discard a card. */

  if (!IS_CASTING(player, card, event))
	return 0;
  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select up to two target creature cards.");
  int rval = spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(player, card, event, 2, &test, 1);
  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (spell_fizzled != 1)
		discard(player, 0, player);
	  kill_card(player, card, KILL_DESTROY);
	}
  return rval;
}

int card_might_of_the_nephilim(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = 2*count_colors(instance->targets[0].player, instance->targets[0].card);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, amount, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_momir_vig(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && ! is_humiliated(player, card) &&
		player == reason_for_trigger_controller && trigger_cause_controller == player
	  ){

		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			if( get_color(trigger_cause_controller, trigger_cause) & COLOR_TEST_GREEN ){
				trig |=1;
			}

			if( get_color(trigger_cause_controller, trigger_cause) & COLOR_TEST_BLUE ){
				trig |=2;
			}
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				if( trig & 2 ){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				else{
					 event_result |= RESOLVE_TRIGGER_AI(player);
				}
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( trig & 1 ){
						int choice = 0;
						if( (trig & 2) && player != AI ){
							choice = do_dialog(player, player, card, -1, -1, " Tutor creature\n Pass", 0);
						}
						if( choice == 0 ){
							test_definition_t test;
							new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card");
							new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, AI_MAX_VALUE, &test);
						}
					}
					if( trig & 2 ){
						int *deck = deck_ptr[player];
						if( deck[0] != -1 ){
							show_deck( 1-player, deck, 1, "Here's the first card of deck", 0, 0x7375B0 );
							if( is_what(-1, deck[0], TYPE_CREATURE) ){
								add_card_to_hand(player, deck[0]);
								remove_card_from_deck(player, 0);
							}
						}
					}
			}

		}
	}

	return 0;
}

int card_nihilistic_glee(int player, int card, event_t event){
	/* Nihilistic Glee	|2|B|B
	 * Enchantment
	 * |2|B, Discard a card: Target opponent loses 1 life and you gain 1 life.
	 * Hellbent - |1, Pay 2 life: Draw a card. Activate this ability only if you have no cards in hand. */

	if( event == EVENT_CAN_CAST || event == EVENT_SHOULD_AI_PLAY ){
		return global_enchantment(player, card, event);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_DISCARD | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_XB(2, 1), 0, &td, NULL) ){
			return 1;
		}
		return hand_count[player] < 1 && generic_activated_ability(player, card, event, 0, MANACOST_X(1), 2, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_DISCARD | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_XB(2, 1), 0, &td, NULL) ){
			if( hand_count[player] < 1 && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_X(1), 2, NULL, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Suck life\n Draw a card\n Cancel", life[1-player] < 6 && life[player] < 6 ? 0 : 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			generic_activated_ability(player, card, event, GAA_DISCARD | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_XB(2, 1), 0, &td, "TARGET_PLAYER");
			if( spell_fizzled != 1 ){
				instance->info_slot = 66;
			}
		}
		else if( choice == 1 ){
				generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_X(1), 2, NULL, NULL);
				if( spell_fizzled != 1 ){
					instance->info_slot = 67;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			lose_life(1-player, 1);
			gain_life(player, 1);
		}
		if( instance->info_slot == 67 ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_novijen_sages(int player, int card, event_t event){

	/* Novijen Sages	|4|U|U
	 * Creature - Human Advisor Mutant 0/0
	 * Graft 4
	 * |1, Remove two +1/+1 counters from among creatures you control: Draw a card. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 4);

	graft_trigger(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL) ){
			int result = 0;
			int count = 0;
			while( count < active_cards_count[player] ){
					if( is_what(player, count, TYPE_CREATURE) && in_play(player, count) && count_1_1_counters(player, count) > 0 ){
						result+=count_1_1_counters(player, count);
					}
					if( result > 1 ){
						return 1;
					}
					count++;
			}
		}
	}

	if( event == EVENT_ACTIVATE){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				remove_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				td.allow_cancel = 0;
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					remove_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				}
			}
		}
		td.allow_cancel = 1;
		instance->number_of_targets = 0;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_novijen_heart_of_progress(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event != EVENT_ACTIVATE && event != EVENT_RESOLVE_ACTIVATION ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_UG(1, 1), 0, NULL, NULL) ){
			int ai_choice = 0;
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) && check_special_flags(player, i, SF_JUST_CAME_INTO_PLAY) ){
					ai_choice = 1;
					break;
				}
			}
			choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Pump Creatures\n Cancel", ai_choice);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, MANACOST_UG(1, 1));
				if( spell_fizzled != 1 ){
					tap_card(player, card);
					instance->info_slot = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			activate_oran_rief_pump(player, card, 0);
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	if( event == EVENT_CLEANUP ){
		remove_special_flags(2, -1, SF_JUST_CAME_INTO_PLAY);
	}

	return 0;
}

static int omnibian_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_POWER && affect_me(instance->targets[0].player, instance->targets[0].card) ){
		event_result += (3 - get_base_power(instance->targets[0].player, instance->targets[0].card));
	}

	if( event == EVENT_TOUGHNESS && affect_me(instance->targets[0].player, instance->targets[0].card) ){
		event_result += (3 - get_base_toughness(instance->targets[0].player, instance->targets[0].card));
	}

	if( eot_trigger( player, card, event) ){
		reset_subtypes(instance->targets[0].player, instance->targets[0].card, 1);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_omnibian(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			force_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_FROG);
			create_targetted_legacy_effect(player, instance->parent_card, &omnibian_legacy, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_NONSICK | GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_overrule(int player, int card, event_t event){
	/* Overrule	|X|W|U
	 * Instant
	 * Counter target spell unless its controller pays |X. You gain X life. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 && ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG) ){
			return result;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
		return card_counterspell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL){
		if (counterspell_resolve_unless_pay_x(player, card, NULL, 0, instance->info_slot)){
			gain_life(player, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_pain_magnification(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_target_card == -1 && damage->damage_target_player == 1-player ){
					if( damage->info_slot > 2 ){
						instance->info_slot++;
					}
					else{
						card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
						if( trg->targets[16].player > 2 ){
							instance->info_slot++;
						}
					}
				}
			}
		}

		if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == player ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					new_multidiscard(1-player, instance->info_slot, 0, player);
					instance->info_slot = 0;
			}
		}
	}
	return global_enchantment(player, card, event);
}

int card_paladin_of_prahv(int player, int card, event_t event){
	/* Paladin of Prahv	|4|W|W
	 * Creature - Human Knight 3/4
	 * Whenever ~ deals damage, you gain that much life.
	 * Forecast - |1|W, Reveal ~ from your hand: Whenever target creature deals damage this turn, you gain that much life. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	spirit_link_effect(player, card, event, player);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
		if( has_mana_multi(player, MANACOST_XW(1, 1)) && can_target(&td) && instance->info_slot != 1 && current_phase == PHASE_UPKEEP){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE_FROM_HAND ){
		charge_mana_multi(player, MANACOST_XW(1, 1));
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			return 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		if( valid_target(&td) ){
			int legacy = pump_ability_until_eot(player, get_card_instance(player, card)->targets[1].card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, 0);
			card_instance_t* leg = get_card_instance(player, legacy);
			if (leg->targets[4].card == -1){
				leg->targets[4].card = 0;
			}
			leg->targets[4].card |= (player == 0 ? 1 : 2);
		}
	}

	return 0;
}

int card_patagia_viper(int player, int card, event_t event){
	/* Patagia Viper	|3|G
	 * Creature - Snake 2/1
	 * Flying
	 * When ~ enters the battlefield, put two 1/1 |Sgreen and |Sblue Snake creature tokens onto the battlefield.
	 * When ~ enters the battlefield, sacrifice it unless |U was spent to cast it. */

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 1, 0, 0, 0);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 1, 0, 0, 0);
		}
	}

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SNAKE, &token);
		token.color_forced = COLOR_TEST_BLUE | COLOR_TEST_GREEN;
		token.qty = 2;
		generate_token(&token);
		if( instance->targets[1].player != 2 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_plaxcaster_frogling(int player, int card, event_t event){

	/* Plaxcaster Frogling	|1|G|U
	 * Creature - Frog Mutant 0/0
	 * Graft 3
	 * |2: Target creature with a +1/+1 counter on it gains shroud until end of turn. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 3);

	graft_trigger(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_SHROUD, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_X(2), 0, &td, "Select target creature with a +1/+1 counter.");
}

int card_plaxmanta(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 0, 0, 1, 0, 0);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 0, 0, 1, 0, 0);
		}
	}

	if( comes_into_play(player, card, event) ){
		pump_subtype_until_eot(player, card, player, -1, 0, 0, KEYWORD_SHROUD, 0);
		if( instance->targets[1].player != 2 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return flash(player, card, event);
}

int card_plumes_of_peace(int player, int card, event_t event){
	/* Plumes of Peace	|1|W|U
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature doesn't untap during its controller's untap step.
	 * Forecast - |W|U, Reveal ~ from your hand: Tap target creature. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
		if( has_mana_multi(player, MANACOST_UW(1, 1)) && can_target(&td) && instance->info_slot != 1 && current_phase == PHASE_UPKEEP){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE_FROM_HAND ){
		charge_mana_multi(player, MANACOST_UW(1, 1));
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			return 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_prahv_spires_of_order(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_XUW(5, 1, 1), 0, &td, NULL) ){
			return 99;
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION,
														MANACOST_XUW(5, 1, 1), 0, &td, NULL)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Prevent damage\n Cancel", 1);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				instance->number_of_targets = 0;
				add_state(player, card, STATE_TAPPED);
				charge_mana_for_activated_ability(player, card, MANACOST_XUW(4, 1, 1));
				if( spell_fizzled == 1 || ! pick_target(&td, "TARGET_DAMAGE") ){
					untap_card_no_event(player, card);
					spell_fizzled = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( valid_target(&td) ){
				card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				dmg->info_slot = 0;
				prevent_damage_until_eot(instance->parent_controller, instance->parent_card, dmg->damage_source_player, dmg->damage_source_card, -1);
			}
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

int card_pride_of_clouds(int player, int card, event_t event){
	/* Pride of the Clouds	|W|U
	 * Creature - Elemental Cat 1/1
	 * Flying
	 * ~ gets +1/+1 for each other creature with flying on the battlefield.
	 * Forecast - |2|W|U, Reveal ~ from your hand: Put a 1/1 |Swhite and |Sblue Bird creature token with flying onto the battlefield. */

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		this_test.keyword = KEYWORD_FLYING;
		event_result += check_battlefield_for_special_card(player, card, ANYBODY, CBFSC_GET_COUNT, &this_test);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
		if( has_mana_multi(player, MANACOST_XUW(2, 1, 1)) && instance->info_slot != 1 && current_phase == PHASE_UPKEEP){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE_FROM_HAND ){
		charge_mana_multi(player, MANACOST_XUW(2, 1, 1));
		if( spell_fizzled != 1 ){
			instance->number_of_targets = 1;
			return 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BIRD, &token);
		token.color_forced = COLOR_TEST_WHITE | COLOR_TEST_BLUE;
		generate_token(&token);
	}

	return 0;
}

int card_proclamation_of_rebirth(int player, int card, event_t event){

	/* Proclamation of Rebirth	|2|W
	 * Sorcery
	 * Return up to three target creature cards with converted mana cost 1 or less from your graveyard to the battlefield.
	 * Forecast - |5|W, Reveal ~ from your hand: Return target creature card with converted mana cost 1 or less from your graveyard to the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.cmc = 2;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		if( has_mana_multi(player, MANACOST_XW(5, 1)) && instance->info_slot != 1 && current_phase == PHASE_UPKEEP &&
			new_special_count_grave(player, &this_test) > 0 && ! graveyard_has_shroud(player)
		  ){
			return ! graveyard_has_shroud(2);
		}
	}

	if( event == EVENT_ACTIVATE_FROM_HAND ){
		charge_mana_multi(player, MANACOST_XW(5, 1));
		if( spell_fizzled != 1 ){
			return 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select up to 3 target creature cards with CMC 1 or less.");
		test.cmc = 2;
		test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, 1, &test);
	}

	if( event == EVENT_CAN_CAST && special_count_grave(player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, 2, F5_CMC_LESSER_THAN_VALUE) > 0 ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select up to 3 target creature cards with CMC 1 or less.");
		test.cmc = 2;
		test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

		select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, &test, 3, &instance->targets[1]);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		// Move out of graveyard first, then animate one at a time, to prevent strange interactions with Gravedigger, Angel of Glory's Rise, etc.
		int i, num_validated = 0, num_targeted = 0, selected, dead[3];
		for (i = 0; i < 3; ++i){
			if (instance->targets[i + 1].card == -1){
				dead[i] = -1;
			} else {
				++num_targeted;
				if ((selected = validate_target_from_grave(player, card, player, i + 1)) == -1){
					dead[i] = -1;
				} else {
					dead[i] = get_grave(player)[selected];
					obliterate_card_in_grave(player, selected);
					++num_validated;
				}
			}
		}
		for (i = 0; i < 3; ++i){
			if (dead[i] != -1){
				int pos = raw_put_iid_on_top_of_graveyard(player, dead[i]);
				increase_trap_condition(player, TRAP_CARDS_TO_GY_FROM_ANYWHERE, -1);	// Since not really putting in graveyard
				reanimate_permanent(player, card, player, pos, REANIMATE_DEFAULT);
			}
		}

		if (num_validated == 0 && num_targeted != 0){
			spell_fizzled = 1;
		}

		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_proper_burial(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	count_for_gfp_ability_and_store_values(player, card, event, player, TYPE_CREATURE, NULL, GFPC_STORE_TOUGHNESS, 0);

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		gain_life(player, instance->targets[0].player);
		instance->targets[0].player = instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_protean_hulk(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){
		int max_cmc = 6;
		while( max_cmc > 0 ){
				char msg[100];
				scnprintf(msg, 100, "Select a creature card with CMC %d or less.", max_cmc);
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, msg);
				this_test.cmc = max_cmc+1;
				this_test.cmc_flag = 3;
				int crd = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
				if( crd != -1 ){
					max_cmc -= get_cmc(player, crd);
				}
				else{
					break;
				}
		}
	}

	return 0;
}

int card_psychic_possession(int player, int card, event_t event){

	/* Psychic Possession	|2|U|U
	 * Enchantment - Aura
	 * Enchant opponent
	 * Skip your draw step.
	 * Whenever enchanted opponent draws a card, you may draw a card. */

	skip_your_draw_step(player, event);

	if( event == EVENT_CAN_CAST ){
		return opponent_is_valid_target(player, card);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_opponent(player, card);
	}

	if( event == EVENT_RESOLVE_SPELL  ){
		if( ! opponent_is_valid_target(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if(card_drawn_trigger(player, card, event, get_card_instance(player, card)->targets[0].player, RESOLVE_TRIGGER_AI(player))){
		draw_a_card(player);
	}

	return global_enchantment(player, card, event);
}

int card_psychotic_fury(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int)is_multicolored;


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target multicolored creature.", 1, NULL);
}

int card_pure_simple(int player, int card, event_t event){

	/* Pure // Simple
	 * |1|R|G|/|1|G|W
	 * Sorcery // Sorcery
	 * Destroy target multicolored permanent.
	 * //
	 * Destroy all Auras and Equipment. */

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT );
		td.extra = (int32_t)is_multicolored;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		return generic_split_card(player, card, event, can_target(&td), 0, MANACOST_XGW(1, 1, 1), 1, 0, 0, NULL, NULL);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT );
		td.extra = (int32_t)is_multicolored;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		new_pick_target(&td, "Select target multicolored permanent.", 0, 1 | GS_LITERAL_PROMPT);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.extra = (int32_t)is_multicolored;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	int priority_simple = 5;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_AURA;
		this_test.sub2 = SUBTYPE_EQUIPMENT;
		this_test.subtype_flag = F2_MULTISUBTYPE;
		int my_p = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
		int opp_p = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test);
		priority_simple+=3*(opp_p-my_p);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( instance->info_slot & 2 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.subtype = SUBTYPE_AURA;
			this_test.sub2 = SUBTYPE_EQUIPMENT;
			this_test.subtype_flag = F2_MULTISUBTYPE;
			new_manipulate_all(player, card, ANYBODY, &this_test, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), 10, MANACOST_XGW(1, 1, 1), 1, priority_simple, 0, NULL, NULL);
}

int card_raggamuffyn(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( hand_count[player] == 0 && generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_CREATURE | TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) &&
			sacrifice(player, card, player, 0, TYPE_CREATURE | TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			tap_card(player, card);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_rakdos_augermage(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);

			ec_definition_t this_definition;
			default_ec_definition(player, instance->targets[0].player, &this_definition);
			new_effect_coercion(&this_definition, &this_test);

			default_ec_definition(instance->targets[0].player, player, &this_definition);
			new_effect_coercion(&this_definition, &this_test);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_ONLY_TARGET_OPPONENT|GAA_CAN_SORCERY_BE_PLAYED, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

static void check_for_tokens_created_at_eot(token_generation_t* token, int gob, int number)
{
	if( current_phase == PHASE_DISCARD ){
		set_special_flags3(token->t_player, gob, SF3_WONT_TRIGGER_AT_THIS_EOT);
	}
}


int card_rakdos_guildmage(int player, int card, event_t event){
/*
Rakdos Guildmage  {B/R}{B/R}
Creature — Zombie Shaman 2/2
{3}{B}, Discard a card: Target creature gets -2/-2 until end of turn.
{3}{R}: Put a 2/1 red Goblin creature token with haste onto the battlefield. Exile it at the beginning of the next end step.
*/
	hybrid(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DISCARD, MANACOST_XB(3, 1), 0, &td, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, 0, MANACOST_XR(3, 1), 0, NULL, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_DISCARD, MANACOST_XB(3, 1), 0, &td, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_XR(3, 1), 0, NULL, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Weaken a creature\n Generate a Goblin\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 2){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_multi(player, MANACOST_XBR(3, choice == 0, choice == 1)) ){
			instance->number_of_targets = 0;
			if( choice == 0 ){
				if( pick_target(&td, "TARGET_CREATURE")){
					discard(player, 0, player);
				}
			}
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td)){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -2, -2);
		}
		if( instance->info_slot == 67 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_GOBLIN, &token);
			token.color_forced = get_sleighted_color_test(instance->parent_controller, instance->parent_card, COLOR_TEST_RED);
			token.pow = 2;
			token.tou = 1;
			token.special_infos = 66;
			token.s_key_plus = SP_KEYWORD_HASTE;
			token.special_code_on_generation = &check_for_tokens_created_at_eot;
			generate_token(&token);
		}
	}

	return 0;
}

// goblin token --> 0x2006e16

int card_rakdos_ickspitter(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, instance->parent_card);
			lose_life(instance->targets[0].player, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_rakdos_pit_dragon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( hand_count[player] < 1){
			event_result |= KEYWORD_DOUBLE_STRIKE;
		}
	}

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST_R(1), 0, NULL, NULL) ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 1;
		int ai_choice = 1;
		if( has_mana_for_activated_ability(player, card, MANACOST_R(2)) ){
			if( current_phase < PHASE_DECLARE_ATTACKERS && ! check_for_ability(player, card, KEYWORD_FLYING) ){
				ai_choice = 0;
			}
			choice = do_dialog(player, player, card, -1, -1, " Gains flying\n Gets +1/+0 until eot\n Cancel", ai_choice);
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		charge_mana_for_activated_ability(player, card, MANACOST_R(2-choice));
		if( spell_fizzled != 1 ){
			instance->info_slot = 66+choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			pump_ability_until_eot(player, card, player, instance->parent_card, 0, 0, KEYWORD_FLYING, 0);
		}
		if( instance->info_slot == 67 ){
			pump_until_eot(player, card, player, instance->parent_card, 1, 0);
		}
	}

  return 0;
}

int card_rakdos_riteknife(int player, int card, event_t event){

	/* Rakdos Riteknife	|2
	 * Artifact - Equipment
	 * Equipped creature gets +1/+0 for each blood counter on ~ and has "|T, Sacrifice a creature: Put a blood counter on ~."
	 * |B|R, Sacrifice ~: Target player sacrifices a permanent for each blood counter on ~.
	 * Equip |2 */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card) && ! is_humiliated(player, card) ){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, count_counters(player, card, COUNTER_BLOOD), 0, 0);
	}

	enum{
		CHOICE_EQUIP = 1,
		CHOICE_SAC_CREATURE,
		CHOICE_SAC_KNIFE
	};

	if( event == EVENT_CAN_ACTIVATE ){
		if( is_equipping(player, card) && generic_activated_ability(instance->targets[8].player, instance->targets[8].card, event, GAA_UNTAPPED | GAA_SACRIFICE_CREATURE, MANACOST0, 0, NULL, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_BR(1, 1), 0, &td1, NULL) ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 2);
	}

	if( event == EVENT_ACTIVATE ){
		int can_sacrifice_creature = is_equipping(player, card) &&
										generic_activated_ability(instance->targets[8].player, instance->targets[8].card, EVENT_CAN_ACTIVATE,
																	GAA_UNTAPPED | GAA_SACRIFICE_CREATURE, MANACOST0, 0, NULL, NULL);
		int can_sacrifice_knife = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_BR(1, 1), 0, &td1, NULL);

		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Equip", can_activate_basic_equipment(player, card, event, 2), ! is_equipping(player, card) ? 15 : 0,
							"Sac a creature", can_sacrifice_creature, 3*count_subtype(player, TYPE_CREATURE, -1),
							"Sac the knife", can_sacrifice_knife, 4*count_counters(player, card, COUNTER_BLOOD));
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_EQUIP ){
			activate_basic_equipment(player, card, 2);
			if( spell_fizzled != 1 ){
				instance->info_slot = choice;
			}
		}
		if( choice == CHOICE_SAC_CREATURE ){
			if( sacrifice(instance->targets[8].player, instance->targets[8].card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				tap_card(instance->targets[8].player, instance->targets[8].card);
				instance->info_slot = choice;
			}
			else{
				spell_fizzled = 1;
			}
		}
		if( choice == CHOICE_SAC_KNIFE ){
			if( pick_target(&td1, "TARGET_PLAYER") ){
				instance->info_slot = choice+count_counters(player, card, COUNTER_BLOOD);
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == CHOICE_EQUIP ){
			resolve_activation_basic_equipment(player, card);
		}
		if( instance->info_slot == CHOICE_SAC_CREATURE ){
			add_counter(instance->parent_controller, instance->parent_card, COUNTER_BLOOD);
		}
		if( instance->info_slot == CHOICE_SAC_KNIFE && valid_target(&td1) ){
			int amount = instance->info_slot-CHOICE_SAC_KNIFE;
			impose_sacrifice(player, card, instance->targets[0].player, amount, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}

int card_rakdos_the_defiler(int player, int card, event_t event){
	check_legend_rule(player, card, event);

	// Whenever ~ attacks, sacrifice half the non-Demon permanents you control, rounded up.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		int amount = count_permanents_by_type(player, TYPE_PERMANENT)-count_subtype(player, TYPE_PERMANENT, SUBTYPE_DEMON);
		amount = (amount + 1) / 2;
		if( amount > 0 ){
			impose_sacrifice(player, card, player, amount, TYPE_PERMANENT, 0, SUBTYPE_DEMON, 1, 0, 0, 0, 0, -1, 0);
		}
	}

	// Whenever Rakdos deals combat damage to a player, that player sacrifices half the non-Demon permanents he or she controls, rounded up.
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_OPPONENT) ){
		int amount = count_permanents_by_type(1-player, TYPE_PERMANENT)-count_subtype(1-player, TYPE_PERMANENT, SUBTYPE_DEMON);
		amount = (amount + 1) / 2;
		if( amount > 0 ){
			impose_sacrifice(player, card, 1-player, amount, TYPE_PERMANENT, 0, SUBTYPE_DEMON, 1, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}

int card_ratcatcher(int player, int card, event_t event){

	fear(player, card, event);

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a Rat card.");
		this_test.subtype = SUBTYPE_RAT;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

int card_riot_spikes(int player, int card, event_t event){
	hybrid(player, card, event);
	return generic_aura(player, card, event, player, 2, -1, 0, 0, 0, 0, 0);
}

int card_rix_maadi_dungeon_palace(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_XBR(2, 1, 1), 0, NULL, NULL) && can_sorcery_be_played(player, event) ){
			choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Both playes discard\n Cancel", 1);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				charge_mana_for_activated_ability(player, card, MANACOST_XBR(1, 1, 1));
				if( spell_fizzled != 1 ){
					instance->info_slot = 1;
					tap_card(player, card);
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			APNAP(p, {discard(p, 0, player);};);
		}
	}

	return 0;
}

static int simic_basilisk_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	damage_effects(player, card, event);

	if( instance->targets[1].player > 2 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			int i;
			for(i=2; i<instance->targets[1].player; i++){
				if( in_play(instance->targets[i].player, instance->targets[i].card) ){
					create_targetted_legacy_effect(instance->targets[0].player, instance->targets[0].card, &die_at_end_of_combat,
													instance->targets[i].player, instance->targets[i].card);
				}
			}
			instance->targets[1].player = 0;
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_simic_basilisk(int player, int card, event_t event){

	/* Simic Basilisk	|4|G|G
	 * Creature - Basilisk Mutant 0/0
	 * Graft 3
	 * |1|G: Until end of turn, target creature with a +1/+1 counter on it gains "Whenever this creature deals combat damage to a creature, destroy that
	 * creature at end of combat." */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 3);

	graft_trigger(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XG(1, 1), 0, &td, "Select target creature with a +1/+1 counter.");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int legacy = create_targetted_legacy_effect(player, instance->parent_card, &simic_basilisk_legacy,
														instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance( player, legacy );
			leg->targets[1].card = get_id(player, instance->parent_card);
		}
	}

	return 0;
}

static const char* can_move_aura_same_controller(int who_chooses, int player, int card){
	int rvalue = 0;
	set_special_flags(player, card, SF_MOVE_AURA_SAME_CONTROLLER);
	rvalue = call_card_function(player, card, EVENT_CAN_MOVE_AURA);
	remove_special_flags(player, card, SF_MOVE_AURA_SAME_CONTROLLER);
	return rvalue ? NULL : "cannot move this Aura.";
}

int card_simic_guildmage(int player, int card, event_t event){

	/* Simic Guildmage	|GU|GU
	 * Creature - Elf Wizard 2/2
	 * |1|G: Move a +1/+1 counter from target creature onto another target creature with the same controller.
	 * |1|U: Attach target Aura enchanting a permanent to another permanent with the same controller. */

	hybrid(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ENCHANTMENT);
	td1.required_subtype = SUBTYPE_AURA_CREATURE;
	td1.preferred_controller = player;
	td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td1.extra = (int)can_move_aura_same_controller;

	card_instance_t *instance = get_card_instance( player, card );


	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XG(1, 1), 0, &td, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XU(1, 1), 0, &td1, NULL) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XG(1, 1), 0, &td, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XU(1, 1), 0, &td1, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Move a +1/+1 counter\n Move an Enchant Creature\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XUG(1, choice == 1, choice == 0)) ){
			instance->number_of_targets = 0;
			if( choice == 0 ){
				if( new_pick_target(&td, "Select target creature with a +1/+1 counter.", 0, 1 | GS_LITERAL_PROMPT) ){
					state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);

					td2.allowed_controller = td2.preferred_controller = instance->targets[0].player;

					if( can_target(&td2) && select_target(player, card, &td2, "Select target creature with the same controller", &(instance->targets[1])) ){
						instance->info_slot = 66+choice;
					}
					else{
						spell_fizzled = 1;
					}
					state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
				}
			}
			if( choice == 1 ){
				if( new_pick_target(&td1, "Select target Enchant Creature to move.", 0, 1 | GS_LITERAL_PROMPT) ){
					set_special_flags(instance->targets[0].player, instance->targets[0].card, SF_MOVE_AURA_SAME_CONTROLLER);
					card_instance_t *targ = get_card_instance( instance->targets[0].player, instance->targets[0].card );
					int (*ptFunction)(int, int, event_t) = (void*)cards_data[targ->internal_card_id].code_pointer;
					if( ptFunction(instance->targets[0].player, instance->targets[0].card, EVENT_MOVE_AURA) ){
						instance->info_slot = 66+choice;
					}
					else{
						remove_special_flags(instance->targets[0].player, instance->targets[0].card, SF_MOVE_AURA_SAME_CONTROLLER);
						spell_fizzled = 1;
					}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			td2.allowed_controller = instance->targets[0].player;
			if( valid_target(&td) && validate_target(player, card, &td2, 1) ){
				move_counters(instance->targets[1].player, instance->targets[1].card, instance->targets[0].player, instance->targets[0].card, COUNTER_P1_P1, 1);
			}
		}
		if( instance->info_slot == 67 ){
			if( valid_target(&td1) ){
				card_instance_t *targ = get_card_instance( instance->targets[0].player, instance->targets[0].card );
				int (*ptFunction)(int, int, event_t) = (void*)cards_data[targ->internal_card_id].code_pointer;
				ptFunction(instance->targets[0].player, instance->targets[0].card, EVENT_RESOLVE_MOVING_AURA);
				remove_special_flags(instance->targets[0].player, instance->targets[0].card, SF_MOVE_AURA_SAME_CONTROLLER);
			}
			else{
				if( in_play(instance->targets[0].player, instance->targets[0].card) ){
					remove_special_flags(instance->targets[0].player, instance->targets[0].card, SF_MOVE_AURA_SAME_CONTROLLER);
				}
			}
		}
	}

	return 0;
}

int card_simic_initiate(int player, int card, event_t event){
	return graft(player, card, event, 1);
}

int card_sky_hussar(int player, int card, event_t event){
	/* Sky Hussar	|3|W|U
	 * Creature - Human Knight 4/3
	 * Flying
	 * When ~ enters the battlefield, untap all creatures you control.
	 * Forecast - Tap two untapped |Swhite and/or |Sblue creatures you control, Reveal ~ from your hand: Draw a card. */

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_color = COLOR_TEST_WHITE | COLOR_TEST_BLUE;
	td.illegal_abilities = 0;

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
		if( can_target(&td) && instance->info_slot != 1 && current_phase == PHASE_UPKEEP){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE_FROM_HAND ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			tap_card(instance->targets[0].player, instance->targets[0].card);
			return 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_skyscribing(int player, int card, event_t event){
	/* Skyscribing	|X|U|U
	 * Sorcery
	 * Each player draws X cards.
	 * Forecast - |2|U, Reveal ~ from your hand: Each player draws a card. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
		if( has_mana_multi(player, MANACOST_XU(2, 1)) && instance->info_slot != 1 && current_phase == PHASE_UPKEEP ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE_FROM_HAND ){
		charge_mana_multi(player, MANACOST_XU(2, 1));
		if( spell_fizzled != 1 ){
			return 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		draw_cards(player, 1);
		draw_cards(1-player, 1);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, instance->info_slot);
		draw_cards(1-player, instance->info_slot);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_slithering_shade(int player, int card, event_t event){

	if (event == EVENT_ABILITIES && affect_me(player, card) && hand_count[player] <= 0 && ! is_humiliated(player, card) ){
		get_card_instance(player, card)->token_status |= STATUS_WALL_CAN_ATTACK;
	}

	return generic_shade(player, card, event, 0, MANACOST_B(1), 1, 1, 0, 0);
}

int card_soulsworn_jury(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		real_counter_a_spell(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_SPELL_ON_STACK, MANACOST_XU(1, 1), 0, &td, NULL);
}

static const char* cmc_is_2(int who_chooses, int player, int card){
	if( get_cmc(player, card) == 2){
		return NULL;
	}
	return "CMC must be 2.";
}

int card_spell_snare(int player, int card, event_t event){

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ANY);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int)cmc_is_2;

	return counterspell(player, card, event, &td, 0);
}

int card_sporeback_troll(int player, int card, event_t event){

	/* Sporeback Troll	|3|G
	 * Creature - Troll Mutant 0/0
	 * Graft 2
	 * |1|G: Regenerate target creature with a +1/+1 counter on it. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	graft_trigger(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER | (player == AI ? TARGET_SPECIAL_REGENERATION : 0);
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT | GAA_REGENERATION, MANACOST_XG(1, 1), 0,
											&td, "Select target creature with a +1/+1 counter to regenerate.");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_sprouting_phytohydra(int player, int card, event_t event){
	/* Sprouting Phytohydra	|4|G
	 * Creature - Plant Hydra 0/2
	 * Defender
	 * Whenever ~ is dealt damage, you may put a token that's a copy of ~ onto the battlefield. */

	if( ! is_humiliated(player, card) && damage_dealt_to_me_arbitrary(player, card, event, DDBM_TRIGGER_OPTIONAL, player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		token_generation_t token;
		copy_token_definition(player, card, &token, player, card);
		token.qty = instance->targets[7].player;
		instance->targets[7].player = 0;
		generate_token(&token);
	}

	return 0;
}

int card_squealing_devil(int player, int card, event_t event){


	card_instance_t *instance = get_card_instance( player, card );

	ravnica_manachanger(player, card, event, 1, 0, 0, 0, 0);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			instance->targets[1].player = charge_changed_mana(player, card, 1, 0, 0, 0, 0);
		}
	}

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		if( has_mana(player, COLOR_COLORLESS, 1) && can_target(&td) ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				int amount = x_value;
				if( pick_target(&td, "TARGET_CREATURE") ){
					pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, amount, 0);
				}
			}
		}
		if( instance->targets[1].player != 2 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_stalking_vengeance(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	haste(player, card, event);

	count_for_gfp_ability_and_store_values(player, card, event, player, TYPE_CREATURE, NULL, GFPC_STORE_POWER, 1);

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( resolve_gfp_ability(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0) ){
			instance->number_of_targets = 0;
			if( pick_target(&td, "TARGET_PLAYER") ){
				damage_player(instance->targets[0].player, instance->targets[1].player, player, card);
				instance->targets[1].player = instance->targets[11].card = 0;
			}
		}
	}

	return 0;
}

int card_stomp_and_howl(int player, int card, event_t event){

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ENCHANTMENT);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_ARTIFACT") ){
			new_pick_target(&td1, "TARGET_ENCHANTMENT", 1, 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( validate_target(player, card, &td1, 1) ){
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_stormscale_anarch(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DISCARD, MANACOST_XR(2, 1), 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XR(2, 1)) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			instance->number_of_targets = 1;
			instance->info_slot = 2;
			int hand_index = 0;
			int count = 0;
			int my_hand[100];
			while( count < active_cards_count[player] ){
					if( in_hand(player, count) ){
						my_hand[hand_index] = count;
						hand_index++;
					}
					count++;
			}
			int to_discard = internal_rand(hand_index);
			if( count_colors(player, my_hand[to_discard]) > 1 ){
				instance->info_slot+=2;
			}
			discard_card(player, my_hand[to_discard]);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, instance->info_slot);
		}
	}

	return 0;
}

int card_taste_for_mayhem(int player, int card, event_t event){
	int amount = 2;
	if( hand_count[player] < 1 ){
		amount = 4;
	}
	return generic_aura(player, card, event, player, amount, 0, 0, 0, 0, 0, 0);
}

int card_tidespout_tyrant(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_SPELL_CAST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;

		if( can_target(&td) && specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			card_instance_t *instance = get_card_instance(player, card);
			if( pick_target(&td, "TARGET_PERMANENT") ){
				instance->number_of_targets = 1;
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_trygon_predator(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);
		td.allowed_controller = opp;
		td.preferred_controller = opp;
		card_instance_t *instance = get_card_instance( player, card);

		if( can_target(&td) && pick_target(&td, "DISENCHANT") ){
			kill_card(opp, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_twinstrike(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				if( hand_count[player] < 1 ){
					kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
				}
				else{
					damage_creature(instance->targets[i].player, instance->targets[i].card, 2, player, card);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_unliving_psychopath(int player, int card, event_t event){
	/* Unliving Psychopath	|2|B|B
	 * Creature - Zombie Assassin 0/4
	 * |B: ~ gets +1/-1 until end of turn.
	 * |B, |T: Destroy target creature with power less than ~'s power. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	int pwr = get_power(player, card);
	if (pwr > 0){
		td.power_requirement = (pwr - 1) | TARGET_PT_LESSER_OR_EQUAL;
	} else {
		td.power_requirement = TARGET_PT_LESSER_OR_EQUAL | TARGET_PT_GREATER_OR_EQUAL | TARGET_PT_MASK;	// always impossible
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_B(1), 0, &td, "") ){
			choice = do_dialog(player, player, card, -1, -1, " Pump Unliving Psychopath\n Destroy a creature\n Cancel", 1);
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			instance->number_of_targets = 0;
			if( choice == 1 ){
				new_pick_target(&td, "Select target creature with power less than Unliving Psychopath's power.", 0, 1 | GS_LITERAL_PROMPT);
				if (cancel != 1){
					instance->state |= STATE_TAPPED;	// no, not tap_card(), never tap_card() during something's own EVENT_ACTIVATE
				}
			}
			instance->info_slot = 66+choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			pump_until_eot_merge_previous(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, -1);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_utopia_sprawl(int player, int card, event_t event){
	card_instance_t* instance = get_card_instance(player, card);
	int result = wild_growth_aura(player, card, event, get_hacked_subtype(player, card, SUBTYPE_FOREST), instance->targets[2].player, instance->targets[2].card);
	if (comes_into_play(player, card, event)){
		instance->targets[2].player = choose_a_color(player, get_deck_color(player, player));
		instance->targets[2].card = 1;
		instance->info_slot = 1 << instance->targets[2].player;
	}
	return result;
}

int card_valor_made_real(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if (event == EVENT_RESOLVE_SPELL){
		if (valid_target(&td)){
			card_instance_t* instance = get_card_instance(player, card);
			can_block_additional_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 255);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_vigean_graftmage(int player, int card, event_t event){

	/* Vigean Graftmage	|2|U
	 * Creature - Vedalken Wizard Mutant 0/0
	 * Graft 2
	 * |1|U: Untap target creature with a +1/+1 counter on it. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	graft_trigger(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XU(1, 1), 0,
											&td, "Select target creature with a +1/+1 counter.");
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_vigean_hydropon(int player, int card, event_t event){

	cannot_attack(player, card, event);

	cannot_block(player, card, event);

	return graft(player, card, event, 5);
}

int card_vision_skeins(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		draw_cards(1-player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_voidslime(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td_spell;
  counterspell_target_definition(player, card, &td_spell, 0);

  if (event == EVENT_CAN_CAST)
	{
	  if (counterspell(player, card, event, &td_spell, 0))
		{
		  instance->info_slot = 1;
		  return 99;
		}
	  if (can_counter_activated_ability(player, card, event, NULL))
		{
		  instance->info_slot = 2;
		  return 99;
		}
	  return 0;
	}
  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  instance->number_of_targets = 0;
	  if (instance->info_slot == 1)
		counterspell(player, card, event, &td_spell, 0);
	  else if (instance->info_slot == 2)
		cast_counter_activated_ability(player, card, 0);
	}
  else if (event == EVENT_RESOLVE_SPELL)
	{
	  if (instance->info_slot == 1)
		counterspell(player, card, event, &td_spell, 0);	// destroys card
	  else if (instance->info_slot == 2 && resolve_counter_activated_ability(player, card, NULL, 0))
		kill_card(player, card, KILL_DESTROY);
	}
  else
	return counterspell(player, card, event, &td_spell, 0);

  return 0;
}

static int effect_wakestone_gargoyle(int player, int card, event_t event){
  if (event == EVENT_ABILITIES
	  && affected_card_controller == player
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && in_play(affected_card_controller, affected_card))
	add_status(affected_card_controller, affected_card, STATUS_WALL_CAN_ATTACK);

  if (event == EVENT_CLEANUP && affect_me(player, card))
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_wakestone_gargoyle(int player, int card, event_t event){

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  create_legacy_effect(instance->parent_controller, instance->parent_card, effect_wakestone_gargoyle);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_XW(1, 1), 0, 0, 0);
}

int card_windreaver(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_U(1), 0, NULL, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, 0, MANACOST_W(1), 0, NULL, NULL) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int priorities[4] = {	current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS && ! check_for_special_ability(player, card, SP_KEYWORD_VIGILANCE) ? 15 : 0,
								8,
								is_attacking(player, card) && is_unblocked(player, card) ? 10 : 0,
								5
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"Give Vigilance", has_mana_for_activated_ability(player, card, MANACOST_W(1)), priorities[0],
						"Pump toughness", has_mana_for_activated_ability(player, card, MANACOST_W(1)), priorities[1],
						"Switch P/T", has_mana_for_activated_ability(player, card, MANACOST_U(1)), priorities[2],
						"Return to hand", has_mana_for_activated_ability(player, card, MANACOST_U(1)), priorities[3]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_UW((choice == 2 || choice == 3), (choice == 0 || choice == 1))) ){
			instance->info_slot = 66+choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, 0, SP_KEYWORD_VIGILANCE);
		}
		if( instance->info_slot == 67 ){
			pump_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 1);
		}
		if( instance->info_slot == 68 ){
			switch_power_and_toughness_until_eot(player, card, instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 69 ){
			bounce_permanent(player, instance->parent_card);
		}
	}

	return 0;
}

int card_wrecking_ball(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_LAND);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target artifact or creature.", 1, NULL);
}

int card_rakdos_carnarium(int player, int card, event_t event){
	return karoo( player, card, event, COLOR_RED, COLOR_BLACK, CARD_ID_BADLANDS );
}

int card_simic_growth_chamber(int player, int card, event_t event){
	return karoo( player, card, event, COLOR_GREEN, COLOR_BLUE, CARD_ID_TROPICAL_ISLAND );
}

int card_orzhov_basilica(int player, int card, event_t event){
	return karoo( player, card, event, COLOR_WHITE, COLOR_BLACK, CARD_ID_SCRUBLAND );
}

