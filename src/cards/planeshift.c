#include "manalink.h"

// Functions
int count_domain(int player, int card){
	int i, count = 0;
	for (i = COLOR_BLACK; i <= COLOR_WHITE; ++i){
		if (basiclandtypes_controlled[player][i] > 0){
			++count;
		}
	}
	return count;
}

static void familiar(int player, int card, event_t event, color_test_t colors)
{
  /* [Something]scape Familiar	|1|C
   * Creature
   * |S[Color1] spells and |S[Color2] spells you cast cost |1 less to cast. */

  if (event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player
	  && (get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, colors)))
	--COST_COLORLESS;
}

static int gating(int player, int card, event_t event, int c1, int c2){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.required_color = c1 | c2;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		if( get_id(player, card) == CARD_ID_MARSH_CROCODILE ){
			APNAP(p, {discard(p, 0, player);};);
		}
		if( get_id(player, card) == CARD_ID_SAWTOOTH_LOON ){
			draw_cards(player, 2);
			int amount = 2;
			if( amount > hand_count[player] ){
				amount = hand_count[player];
			}
			char msg[100] = "Select a card to put on bottom.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			while( amount > 0 ){
					int result = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
					put_on_bottom_of_deck(player, result);
					amount--;
			}
		}
		if( get_id(player, card) == CARD_ID_SPARKCASTER ){
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_CREATURE);
			td1.zone = TARGET_ZONE_PLAYERS;
			td1.allow_cancel = 0;
			if( can_target(&td1) && pick_target(&td1, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				damage_player(instance->targets[0].player, 1, player, card);
			}
		}
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

// Cards

int card_allied_strategies(int player, int card, event_t event){

	/* Allied Strategies	|4|U
	 * Sorcery
	 * Domain - Target player draws a card for each basic land type among lands he or she controls. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card)){
		pick_target(&td, "TARGET_PLAYER");
	}
	if(event == EVENT_RESOLVE_SPELL){
		card_instance_t* instance = get_card_instance(player, card);
		draw_cards(instance->targets[0].player, count_domain(instance->targets[0].player, 0) );
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_arctic_merfolk(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			if( can_target(&td) ){
				int choice = do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", 0);
				if( choice == 0 && new_pick_target(&td, "TARGET_CREATURE", 0, 0) ){
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);
					set_special_flags(player, card, SF_KICKED);
				}
			}
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( kicked(player, card) ){
			add_1_1_counter(player, card);
		}
	}

	return 0;
}

int card_bog_down(int player, int card, event_t event)
{
  /* Bog Down	|2|B
   * Sorcery
   * Kicker-Sacrifice two lands.
   * Target player discards two cards. If ~ was kicked, that player discards three cards instead. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  test_definition_t test;
  new_default_test_definition(&test, TYPE_LAND, "");
  test.qty = 2;

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
		if( ! is_token(player, card) ){
			get_card_instance(player, card)->number_of_targets = 0;

			char marked[151];
			if (new_can_sacrifice_as_cost(player, card, &test)
				&& do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass",
						   basiclandtypes_controlled[player][COLOR_ANY] >= 6 && hand_count[1-player] >= 3 ? 0 : 1) == 0)
			{
			  if (mark_sacrifice(player, card, player, SAC_JUST_MARK|SAC_ALL_OR_NONE|SAC_AS_COST, &test, marked) == 2)
				pick_target(&td, "TARGET_PLAYER");

			  int c;
			  for (c = 0; c < active_cards_count[player]; ++c)
				if (marked[c])
				  {
					get_card_instance(player, c)->state &= ~STATE_CANNOT_TARGET;
					if (cancel != 1)
					  kill_card(player, c, KILL_SACRIFICE);
				  }

			  if (cancel != 1)
				set_special_flags(player, card, SF_KICKED);
			}
		}
		td.allow_cancel = 1-kicked(player, card);
		pick_target(&td, "TARGET_PLAYER");
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		new_multidiscard(get_card_instance(player, card)->targets[0].player, kicked(player, card) ? 3 : 2, 0, player);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_cavern_harpy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return player == HUMAN ? generic_activated_ability(player, card, event, 0, MANACOST0, 1, NULL, NULL) :
								generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST0, 1, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( in_play(instance->parent_controller, instance->parent_card) ){
			bounce_permanent(instance->parent_controller, instance->parent_card);
		}
	}

	return gating(player, card, event, COLOR_TEST_BLACK, COLOR_TEST_BLUE);
}

int card_crosis_catacombs(int player, int card, event_t event){
	// also code for the others "Lair" lands

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.required_subtype = SUBTYPE_LAIR;
		td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
		td.illegal_abilities = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if (select_target(player, card, &td, "Select a non-Lair land.", &(instance->targets[0]))){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		} else {
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return mana_producer(player, card, event);
}

int card_crosis_charm(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.illegal_color = COLOR_TEST_BLACK;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST && ( can_target(&td) || can_target(&td1) || can_target(&td2)) ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int ai_choice = 0;
		int mode = 0;
		mode = (1<<3);
		if( can_target(&td) ){
			mode+=(1<<0);
		}
		if( can_target(&td1) ){
			mode+=(1<<1);
		}
		if( can_target(&td2) ){
			mode+=(1<<2);
		}
		int choice = 0;
		char buffer[500];
		int pos = 0;
		if( mode & (1<<0) ){
			pos += scnprintf(buffer + pos, 500-pos, " Bounce permanent\n");
		}
		if( mode & (1<<1) ){
			pos += scnprintf(buffer + pos, 500-pos, " Kill a nonblack creature\n");
			ai_choice = 1;
		}
		if( mode & (1<<2) ){
			pos += scnprintf(buffer + pos, 500-pos, " Kill an artifact\n");
			ai_choice = 2;
		}
		pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
		choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
		while( !( (1<<choice) & mode) ){
			choice++;
		}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
		else{
			if( choice == 0 ){
				if( pick_target(&td, "TARGET_PERMANENT") ){
					instance->info_slot = 66+choice;
				}
			}
			else if( choice == 1 ){
				if( pick_target(&td1, "TARGET_CREATURE") ){
					instance->info_slot = 66+choice;
				}
			}
			else if( choice == 2 ){
				if( pick_target(&td2, "TARGET_ARTIFACT") ){
					instance->info_slot = 66+choice;
				}
			}
		}
	}
	else if(event == EVENT_RESOLVE_SPELL){
		int choice = instance->info_slot;
		if( choice == 66 && validate_target(player, card, &td, 0) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		if( choice == 67 && validate_target(player, card, &td1, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		if( choice == 68 && validate_target(player, card, &td2, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_darigaaz_charm(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	enum{
		CHOICE_RAISE_DEAD = 1,
		CHOICE_DAMAGE,
		CHOICE_PUMP
	};

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
			return 1;
		}
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td1, NULL, 1, NULL) ){
			return 1;
		}
		if( generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test) ){
			return 1;
		}
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;
			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"Raise Dead", (count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(player)), 6,
								"Lightning Bolt", can_target(&td) , 8,
								"Giant Growth", can_target(&td1), (current_phase == PHASE_AFTER_BLOCKING ? 10 : 5)
								);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}

		if( instance->info_slot == CHOICE_RAISE_DEAD ){
			int result = select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0);
			if( result == -1 ){
				spell_fizzled = 1;
			}
		}
		if( instance->info_slot == CHOICE_DAMAGE ){
			pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
		}
		if( instance->info_slot == CHOICE_PUMP ){
			pick_target(&td1, "TARGET_CREATURE");
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		if( instance->info_slot == CHOICE_RAISE_DEAD ){
			int result = validate_target_from_grave_source(player, card, player, 0);
			if( result > -1 ){
				from_grave_to_hand(player, result, TUTOR_HAND);
			}
		}
		if( instance->info_slot == CHOICE_DAMAGE && validate_target(player, card, &td, 0) ){
			damage_target0(player, card, 3);
		}
		if( instance->info_slot == CHOICE_PUMP && validate_target(player, card, &td1, 0) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 3);
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_dark_suspicions(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 1-player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int amount = hand_count[1-player]-hand_count[player];
		if( amount > 0 ){
			lose_life(1-player, amount);
		}
	}

	return global_enchantment(player, card, event);
}

int card_deadapult(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) &&
		can_target(&td)
		){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_ZOMBIE, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
		if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_ZOMBIE, 0, 0, 0, 0, 0, -1, 0) ){
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				instance->number_of_targets = 1;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
	}

	return global_enchantment(player, card, event);
}

int card_destructive_flow(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_LAND);
	this_test.subtype = SUBTYPE_BASIC;
	this_test.subtype_flag = 1;

	if( !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card)  ){
		int count = count_upkeeps(player);
		if(event == EVENT_TRIGGER && count > 0 && check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
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
		impose_sacrifice(player, card, current_turn, 1, TYPE_LAND, 0, SUBTYPE_BASIC, 1, 0, 0, 0, 0, -1, 0);
	}

	return global_enchantment(player, card, event);
}

int card_diabolic_intent(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	else if(event == EVENT_RESOLVE_SPELL ){
		char msg[100] = "Select a card to tutor";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_doomsday_specter(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		ec_definition_t ec;
		default_ec_definition(1-player, player, &ec);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		new_effect_coercion(&ec, &this_test);
	}

	return gating(player, card, event, COLOR_TEST_BLACK, COLOR_TEST_BLUE);
}

int card_draco(int player, int card, event_t event){
	basic_upkeep(player, card, event, 10-2*count_domain(player, card), 0,0,0,0,0);
	if(event == EVENT_MODIFY_COST ){
		COST_COLORLESS -= 2*count_domain(player, card);
	}
	return 0;
}

int card_dralnus_crusade(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	boost_creature_type(player, card, event, SUBTYPE_GOBLIN, 1, 1, 0, BCT_INCLUDE_SELF);

	if( event == EVENT_RESOLVE_SPELL ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
			if( in_play(player, count) && has_subtype(player, count, SUBTYPE_GOBLIN) ){
				add_a_subtype(player, count, SUBTYPE_ZOMBIE);
			}
			count--;
		}
	}

	if( specific_cip(player, card, event, player, 2, 0, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0) ){
		add_a_subtype(instance->targets[1].player, instance->targets[1].card, SUBTYPE_ZOMBIE);
	}

	if (leaves_play(player, card, event)){
		int p, c;
		for (p = 0; p < 2; ++p){
			for (c = 0; c < active_cards_count[p]; ++c){
				if (has_subtype(p, c, SUBTYPE_GOBLIN)){
					reset_subtypes(p, c, 2);
				}
			}
		}
	}

	if (event == EVENT_SET_COLOR && has_subtype(affected_card_controller, affected_card, SUBTYPE_GOBLIN)){
		event_result |= COLOR_TEST_BLACK;
	}

	return global_enchantment(player, card, event);
}

int card_dralnus_pet(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to discard.");
			this_test.zone = TARGET_ZONE_HAND;

			if( check_battlefield_for_special_card(player, card, player, 0, &this_test) && has_mana_multi(player, 2, 1, 0, 0, 0, 0) ){
				int choice = do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", 0);
				if( choice == 0 ){
					charge_mana_multi(player, 2, 1, 0, 0, 0, 0);
					if( spell_fizzled != 1 ){
						int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MAX_CMC, -1, &this_test);
						instance->info_slot = get_cmc(player, selected);
						discard_card(player, selected);
						set_special_flags(player, card, SF_KICKED);
					}
				}
			}
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( kicked(player, card) ){
			add_1_1_counters(player, card, instance->info_slot);
		}
	}

	if( kicked(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FLYING);
	}

	return 0;
}

int card_dromar_charm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			return result;
		}
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int mode = (1<<0) | (1<<3);
		if( card_on_stack_controller != -1 && card_on_stack != -1){
			if( ! check_state(card_on_stack_controller, card_on_stack, STATE_CANNOT_TARGET) ){
				mode+=(1<<1);
			}
		}
		if( can_target(&td) ){
			mode+=(1<<2);
		}

		int choice = 0;
		int ai_choice = 0;
		char buffer[500];
		int pos = 0;
		if( mode & (1<<0) ){
			pos += scnprintf(buffer + pos, 500-pos, " Gain 5 life\n");
		}
		if( mode & (1<<1) ){
			pos += scnprintf(buffer + pos, 500-pos, " Counterspell\n");
			ai_choice = 1;
		}
		if( mode & (1<<2) ){
			pos += scnprintf(buffer + pos, 500-pos, " -2/-2 to target creature\n");
			if( ai_choice != 1 ){
				ai_choice = 2;
			}
		}
		pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
		choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
		while( !( (1<<choice) & mode) ){
			choice++;
		}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
		else{
			if( choice == 0 ){
				instance->info_slot = 66+choice;
			}
			else if( choice == 1 ){
				instance->targets[0].player = card_on_stack_controller;
				instance->targets[0].card = card_on_stack;
				instance->number_of_targets = 1;
				instance->info_slot = 66+choice;
			}
			else if( choice == 2 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->info_slot = 66+choice;
				}
			}
		}
	}
	else if(event == EVENT_RESOLVE_SPELL){
		if( instance->info_slot == 66 ){
			gain_life(player, 5);
		}
		if( instance->info_slot == 67 ){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		}
		if( instance->info_slot == 68 && validate_target(player, card, &td, 0) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -2, -2);
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_eladamris_call(int player, int card, event_t event){
	/*
	  Eladamri's Call |W|G
	  Instant
	  Search your library for a creature card, reveal that card, and put it into your hand. Then shuffle your library.
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_ertai_the_corrupted(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
			int result = card_spiketail_hatchling(player, card, event);
			if( result > 0 && can_sacrifice_as_cost(player, 1, TYPE_CREATURE | TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				return result;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0);
		if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_CREATURE | TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			tap_card(player, card);
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		set_flags_when_spell_is_countered(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
	}

	return 0;
}

int card_flametongue_kavu(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	return cip_damage_creature(player, card, event, &td, "TARGET_CREATURE", 4);
}

int card_fleetfoot_panther(int player, int card, event_t event){
	gating(player, card, event, COLOR_TEST_GREEN, COLOR_TEST_WHITE);
	return flash(player, card, event);
}

int card_gaeas_herald(int player, int card, event_t event){

	type_uncounterable(player, card, event, ANYBODY, TYPE_CREATURE, NULL);

	return 0;
}

int card_gerrards_command(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_horned_kavu(int player, int card, event_t event){
	return gating(player, card, event, COLOR_TEST_GREEN, COLOR_TEST_RED);
}

int card_hull_breach(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ENCHANTMENT);

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_ENCHANTMENT | TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( can_target(&td) || can_target(&td1) ){
			return 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( can_target(&td) ){
			if( new_pick_target(&td, "TARGET_ARTIFACT", 0, 0) ){
				instance->info_slot = 1;
				if( can_target(&td1) && new_pick_target(&td1, "TARGET_ENCHANTMENT", 1, 0) ){
					instance->info_slot = 2;
				}
			}
			else{
				if( pick_target(&td1, "TARGET_ENCHANTMENT") ){
					instance->info_slot = 1;
				}
			}
		}
		else{
			if( pick_target(&td1, "TARGET_ENCHANTMENT") ){
				instance->info_slot = 1;
			}
		}
	}


	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td2, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_lashknife_barrier(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_target_player == player &&
				damage->info_slot > 0
			  ){
				damage->info_slot--;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_lava_zombie(int player, int card, event_t event){
	gating(player, card, event, COLOR_TEST_BLACK, COLOR_TEST_RED);
	return generic_shade(player, card, event, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_lord_of_the_undead(int player, int card, event_t event){
	/* Lord of the Undead	|1|B|B
	 * Creature - Zombie 2/2
	 * Other Zombie creatures get +1/+1.
	 * |1|B, |T: Return target Zombie card from your graveyard to your hand. */

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select a Zombie card.");
	this_test.subtype = SUBTYPE_ZOMBIE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) &&
			has_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0 )
		  ){
			if( any_in_graveyard_by_subtype(player, SUBTYPE_ZOMBIE) ){
				return ! graveyard_has_shroud(2);
			}
		}
	}

	else if( event == EVENT_ACTIVATE ){
			 charge_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0);
			 if( spell_fizzled != 1 ){
				int result = new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0);
				if( result > -1 ){
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			 }
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 int selected = instance->targets[0].player;
			 const int *grave = get_grave(player);
			 if( grave[selected] == instance->targets[0].card ){
				 add_card_to_hand(player, grave[selected]);
				 remove_card_from_grave(player, selected);
			 }
	}

	return boost_creature_type(player, card, event, SUBTYPE_ZOMBIE, 1, 1, 0, 0);
}

int card_maggot_carrier(int player, int card, event_t event){
	/*
	  Maggot Carrier |B
	  Creature - Zombie 1/1
	  When Maggot Carrier enters the battlefield, each player loses 1 life.
	*/
	if( comes_into_play(player, card, event) ){
		lose_life(player, 1);
		lose_life(1-player, 1);
	}
	return 0;
}

int card_malicious_advice(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int trgs = 0;
		while( can_target(&td) && has_mana(player, COLOR_COLORLESS, trgs) ){
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
		if( trgs == 0 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana(player, COLOR_COLORLESS, trgs);
			if( spell_fizzled != 1 ){
				instance->info_slot = trgs;
			}
		}
	}


	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		int good = 0;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
				good++;
			}
		}
		if( good > 0 ){
			lose_life(player, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_march_of_souls(int player, int card, event_t event){//UNUSEDCARD
	/* March of Souls	|4|W
	 * Sorcery
	 * Destroy all creatures. They can't be regenerated. For each creature destroyed this way, its controller puts a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		int i;
		for(i=0; i<2; i++){
			int amount = check_battlefield_for_special_card(player, card, i, 4, &this_test);
			new_manipulate_all(player, card, i, &this_test, KILL_BURY);

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIRIT, &token);
			token.t_player = i;
			token.qty = amount;
			token.color_forced = COLOR_TEST_WHITE;
			token.key_plus = KEYWORD_FLYING;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_marsh_crocodile(int player, int card, event_t event){
	return gating(player, card, event, COLOR_TEST_BLACK, COLOR_TEST_BLUE);
}

void meddling_mage_effect(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card);
	if( event == EVENT_RESOLVE_SPELL ){
		if( player == AI ){
			int *deck = deck_ptr[1-player];
			int rounds = 0;
			if( deck[0] != -1 ){
				int i = internal_rand(count_deck(1-player));
				while( 1 ){
						if( deck[i] != -1 && ! is_what(-1, deck[i], TYPE_LAND) ){
							break;
						}
						i++;
						if( deck[i] == -1 ){
							i = 0;
							rounds++;
						}
						if( rounds > 1 ){
							break;
						}
				}
				if( rounds < 2 ){
					instance->targets[1].card = cards_data[deck[i]].id;
				}
				else{
					instance->targets[1].card = get_id(player, card);
				}
			}
			else{
				instance->targets[1].card = get_id(player, card);
			}
		}
		else{
			if( ai_is_speculating != 1 ){
				instance->targets[1].card = card_from_list(player, 3, TYPE_LAND, 1, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
		create_card_name_legacy(player, card, instance->targets[1].card);
	}
	else if( event == EVENT_MODIFY_COST_GLOBAL ){
			if( get_id(affected_card_controller, affected_card) == instance->targets[1].card ){
				infinite_casting_cost();
			}
	}
}

int card_meddling_mage(int player, int card, event_t event){
	meddling_mage_effect(player, card, event);
	return 0;
}

int card_mogg_sentry(int player, int card, event_t event){
	/*
	  Mogg Sentry |R
	  Creature - Goblin Warrior 1/1
	  Whenever an opponent casts a spell, Mogg Sentry gets +2/+2 until end of turn.
	*/
	if( specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot(player, card, player, card, 2, 2);
	}
	return 0;
}

int card_nemata_grove_guardian(int player, int card, event_t event){
	/* Nemata, Grove Guardian	|4|G|G
	 * Legendary Creature - Treefolk 4/5
	 * |2|G: Put a 1/1 |Sgreen Saproling creature token onto the battlefield.
	 * Sacrifice a Saproling: Saproling creatures get +1/+1 until end of turn. */

	check_legend_rule(player, card, event);

	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card)){
			return 0;
		}

		int can_generate = has_mana_for_activated_ability(player, card, MANACOST_XG(2, 1));
		int ai_priority_generate = 2;
		int can_sacr = (has_mana_for_activated_ability(player, card, MANACOST_X(0))
						&& can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0));
		int ai_priority_sacr = count_subtype(player, TYPE_CREATURE, SUBTYPE_SAPROLING) > 1 ? 1 : -1;

		int choice = DIALOG(player, card, event,
							"Generate a Saproling", can_generate, ai_priority_generate,
							"Sacrifice a Saproling", can_sacr, ai_priority_sacr);

		if (event == EVENT_CAN_ACTIVATE){
			return choice;
		} else if (event == EVENT_ACTIVATE){
			if (choice == 1){
				charge_mana_for_activated_ability(player, card, MANACOST_XG(2, 1));
			} else if (choice == 2 && charge_mana_for_activated_ability(player, card, MANACOST_X(0))){
				if (current_phase != PHASE_DECLARE_BLOCKERS){
					/* Discourage sacrifice in phases other than declare blockers, but do it via ai_modifier instead of dialog priority so it can still
					 * sacrifice in response to either the sacrificed saproling being target for destruction, or a different one being damaged */
					ai_modifier -= 24;
				}
				if (!sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0)){
					cancel = 1;
				}
			}
		} else {	// EVENT_RESOLVE_ACTIVATION
			if (choice == 1){
				generate_token_by_id(player, card, CARD_ID_SAPROLING);
			} else {
				pump_subtype_until_eot(player, card, 2, SUBTYPE_SAPROLING, 1, 1, 0, 0);
			}
		}
	}

	return 0;
}

int card_nightscape_battlemage(int player, int card, event_t event){
	/*
	  Nightscape Battlemage |2|B
	  Creature - Zombie Wizard 2/2
	  Kicker {2}{U} and/or {2}{R} (You may pay an additional {2}{U} and/or {2}{R} as you cast this spell.)
	  When Nightscape Battlemage enters the battlefield, if it was kicked with its {2}{U} kicker, return up to two target nonblack creatures to their owners' hands.
	  When Nightscape Battlemage enters the battlefield, if it was kicked with its {2}{R} kicker, destroy target land.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.illegal_color = COLOR_TEST_BLACK;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_LAND);
		td1.allow_cancel = 0;

		volver(player, card, event, MANACOST_XU(2, 1), MANACOST_XR(2, 1), 5*target_available(player, card, &td), 5*can_target(&td1));
	}

	if( comes_into_play(player, card, event) ){
		if( check_special_flags(player, card, SF_KICKED) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

			int trgs = 0;
			while( can_target(&td) && trgs < 2 ){
					instance->number_of_targets = 0;
					if( new_pick_target(&td, get_sleighted_color_text(player, card, "Select target non%s creature", COLOR_BLACK), 0, GS_LITERAL_PROMPT) ){
						bounce_permanent(instance->targets[0].player, instance->targets[0].card);
						trgs++;
					}
					else{
						break;
					}
			}
		}
		if( check_special_flags(player, card, SF_KICKED2) ){
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_LAND);
			td1.allow_cancel = 0;

			if( can_target(&td1) && pick_target(&td1, "TARGET_LAND") ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_nightscape_familiar(int player, int card, event_t event)
{
  /* Nightscape Familiar	|1|B
   * Creature - Zombie 1/1
   * |SBlue spells and |Sred spells you cast cost |1 less to cast.
   * |1|B: Regenerate ~. */

  familiar(player, card, event, COLOR_TEST_BLUE | COLOR_TEST_RED);
  return regeneration(player, card, event, MANACOST_XB(1,1));
}

int card_orims_chant(int player, int card, event_t event){
	/*
	  Orim's Chant |W
	  Instant
	  Kicker {W} (You may pay an additional {W} as you cast this spell.)
	  Target player can't cast spells this turn.
	  If Orim's Chant was kicked, creatures can't attack this turn.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
		if( spell_fizzled != 1 ){
			if( ! is_token(player, card) ){
				do_kicker(player, card, MANACOST_W(1));
			}
		}
	}


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int se_card = target_player_cant_cast_type(player, card, instance->targets[0].player, TYPE_ANY);
			if( kicked(player, card) && se_card > -1 ){
				add_flag_to_special_effect(player, se_card, SE_CREATURES_CANT_ATTACK);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_phyrexian_scuta(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			int ai_choice = 0;
			if( (life[player]-3) < 6 ){
				ai_choice = 1;
			}
			if( can_pay_life(player, 3) ){
				int choice = do_dialog(player, player, card, -1, -1," Pay kicker\n Pass\n", ai_choice);
				if( choice == 0 ){
					lose_life(player, 3);
					set_special_flags(player, card, SF_KICKED);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL && kicked(player, card) ){
		add_1_1_counters(player, card, 2);
	}

	return 0;
}

int card_phyrexian_tyranny(int player, int card, event_t event){

	/* Phyrexian Tyranny	|U|B|R
	 * Enchantment
	 * Whenever a player draws a card, that player loses 2 life unless he or she pays |2. */

	if (card_drawn_trigger(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY)){
		int p = trigger_cause_controller;
		if (!charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, p, COLOR_COLORLESS, 2)){
			lose_life(p, 2);
		}
	}

	return global_enchantment(player, card, event);
}

int card_primal_growth(int player, int card, event_t event){
	/*
	  Primal Growth |2|G
	  Sorcery, 2G
	  Kicker — Sacrifice a creature. (You may sacrifice a creature in addition to any other costs as you cast this spell.)
	  Search your library for a basic land card, put that card onto the battlefield, then shuffle your library.
	  If Primal Growth was kicked, instead search your library for up to two basic land cards, put them onto the battlefield,
	  then shuffle your library.
	*/
	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( can_sacrifice_type_as_cost(player, 1, TYPE_CREATURE) ){
			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_NO_CANCEL,
								"Pay Kicker", 1, (count_subtype(player, TYPE_CREATURE, -1) > 3 ? 10 : 0),
								"Decline", 1, 5);
			if( choice == 1 ){
				test_definition_t test;
				new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice");
				int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
				if( sac ){
					kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
					set_special_flags(player, card, SF_KICKED);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		tutor_basic_lands(player, TUTOR_PLAY, (check_special_flags(player, card, SF_KICKED) ? 2 : 1));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_questing_phelddagrif(int player, int card, event_t event){
	/* Questing Phelddagrif	|1|G|W|U
	 * Creature - Phelddagrif 4/4
	 * |G: ~ gets +1/+1 until end of turn. Target opponent puts a 1/1 |Sgreen Hippo creature token onto the battlefield.
	 * |W: ~ gains protection from |Sblack and from |Sred until end of turn. Target opponent gains 2 life.
	 * |U: ~ gains flying until end of turn. Target opponent may draw a card. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) || has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ||
			has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1)
		  ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
			return would_valid_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
			int mode = (1<<3);
			if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
				mode+=(1<<0);
			}
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
				mode+=(1<<1);
			}
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) ){
				mode+=(1<<2);
			}

			int choice = 0;
			int ai_choice = 0;
			char buffer[500];
			int pos = 0;
			if( mode & (1<<0) ){
				pos += scnprintf(buffer + pos, 500-pos, " Give Flying\n");
			}
			if( mode & (1<<1) ){
				pos += scnprintf(buffer + pos, 500-pos, " Pump\n");
				if( current_phase == PHASE_AFTER_BLOCKING && ! is_unblocked(player, card) ){
					ai_choice = 1;
				}
			}
			if( mode & (1<<2) ){
				int clr = get_sleighted_color(player, card, COLOR_BLACK);
				const char* color1 = (clr == COLOR_BLACK ? "black"
									 : clr == COLOR_BLUE ? "BLUE"
									 : clr == COLOR_GREEN ? "GREEN"
									 : clr == COLOR_RED ? "RED"
									 : "WHITE");
				clr = get_sleighted_color(player, card, COLOR_RED);
				const char* color2 = (clr == COLOR_BLACK ? "BLACK"
									 : clr == COLOR_BLUE ? "BLUE"
									 : clr == COLOR_GREEN ? "GREEN"
									 : clr == COLOR_RED ? "red"
									 : "WHITE");
				pos += scnprintf(buffer + pos, 500-pos, " Give Prot. %s + Prot. %s", color1, color2);
				ai_choice = 2;
			}
			pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
			choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			while( !( (1<<choice) & mode) ){
					choice++;
			}
			if( choice == 3 ){
				spell_fizzled = 1;
			}
			else{
				int c1 = choice+1;
				if( choice == 2 ){
					c1 = 4;
				}
				int clr[5] = {0, 0, 0, 0, 0};
				clr[c1]++;
				charge_mana_for_activated_ability(player, card, 0, 0, clr[1], clr[2], 0, clr[4]);
				if( spell_fizzled != 1 ){
					instance->targets[0].player = 1-player;
					instance->targets[0].card = -1;
					instance->number_of_targets = 1;
					instance->info_slot = 66+choice;
				}
			}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int p = 0;
			int t = 0;
			int keyword = KEYWORD_FLYING;
			if( instance->info_slot == 66 ){
				draw_cards(instance->targets[0].player, 1);
			}
			if( instance->info_slot == 67 ){
				p = t = 1;
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_HIPPO, &token);
				token.t_player = instance->targets[0].player;
				generate_token(&token);
			}
			if( instance->info_slot == 68 ){
				gain_life(instance->targets[0].player, 2);
				keyword = KEYWORD_PROT_BLACK | KEYWORD_PROT_RED;
			}
			pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, p, t, keyword, 0);
		}
	}

	return 0;
}

int card_quirion_dryad(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 4) ){ return 0; }

	if( specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0, COLOR_TEST_BLACK | COLOR_TEST_BLUE | COLOR_TEST_RED | COLOR_TEST_WHITE, 0, 0, 0, -1, 0) ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_rith_charm(int player, int card, event_t event){
	/* Rith's Charm	|R|G|W
	 * Instant
	 * Choose one -
	 * * Destroy target nonbasic land.
	 * * Put three 1/1 |Sgreen Saproling creature tokens onto the battlefield.
	 * * Prevent all damage a source of your choice would deal this turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_NONBASIC;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.required_type = 0;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		if( can_target(&td1) ){
			instance->info_slot = 68;
			return 0x63;
		}
		else{
			instance->info_slot = 0;
			return 1;
		}
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			instance->number_of_targets = 0;
			if( instance->info_slot == 68 ){
				choice = 2;
			}
			else{
				int ai_choice = 0;
				int mode = 0;
				char buffer[500];
				int pos = 0;
				mode = (1<<1) | (1<<2);
				if (can_target(&td)){
					mode+=(1<<0);
					pos += scnprintf(buffer + pos, 500-pos, " Destroy a nonbasic land\n");
				}
				pos += scnprintf(buffer + pos, 500-pos, " Generate 3 Saprolings\n");
				if( current_phase < PHASE_DECLARE_BLOCKERS ){
					ai_choice = 1;
				}
				pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				while( !( (1<<choice) & mode) ){
						choice++;
				}
			}

			if( choice == 3 ){
				spell_fizzled = 1;
			}
			else{
				instance->info_slot = 66+choice;
				if( choice == 0 ){
					pick_target_nonbasic_land(player, card, 0);
				}
				else if( choice == 2 ){
					pick_target(&td1, "TARGET_DAMAGE");
				}
			}
	}
	else if(event == EVENT_RESOLVE_SPELL){
			if( instance->info_slot == 66 && valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			if( instance->info_slot == 67 ){
				generate_tokens_by_id(player, card, CARD_ID_SAPROLING, 3);
			}
			if( instance->info_slot == 68 && valid_target(&td1) ){
				card_instance_t *dmg = get_card_instance( instance->targets[0].player, instance->targets[0].card );
				dmg->info_slot = 0;
				prevent_damage_until_eot(player, card, dmg->damage_source_player, dmg->damage_source_card, -1);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_root_greevil(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int clr = 1<<choose_a_color(player, get_deck_color(player, 1-player));
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		this_test.color = clr;
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_ME, 2, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_rushing_river(int player, int card, event_t event){

	/* Rushing River	|2|U
	 * Instant
	 * Kicker-Sacrifice a land.
	 * Return target nonland permanent to its owner's hand. If ~ was kicked, return another target nonland permanent to its owner's hand. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		int sac = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) && can_sacrifice_type_as_cost(player, 1, TYPE_LAND) &&
			target_available(player, card, &td) >= 2 &&
			do_dialog(player, player, card, -1, -1, " Pay kicker\n Pass", basiclandtypes_controlled[player][COLOR_ANY] >= 5 ? 0 : 1) == 0
		   ){
			sac = controller_sacrifices_a_permanent(player, card, TYPE_LAND, SAC_JUST_MARK|SAC_RETURN_CHOICE|SAC_AS_COST);
			if (!sac){
				spell_fizzled = 1;
				return 0;
			}
			state_untargettable(BYTE2(sac), BYTE3(sac), 1);
		}

		if( pick_target(&td, "TARGET_NONLAND_PERMANENT") ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			if( sac ){
				set_special_flags(player, card, SF_KICKED);
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
		}
		else{
			if( sac ){
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
			}
		}
		if( kicked(player, card) ){
			new_pick_target(&td, "TARGET_NONLAND_PERMANENT", 1, 0);
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
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

int card_sawtooth_loon(int player, int card, event_t event){
	return gating(player, card, event, COLOR_TEST_BLUE, COLOR_TEST_WHITE);
}

int card_shifting_sky(int player, int card, event_t event){
	/*
	  Shifting Sky |2|U
	  Enchantment
	  As Shifting Sky enters the battlefield, choose a color.
	  All nonland permanents are the chosen color.
	*/
	if (event == EVENT_RESOLVE_SPELL){
		get_card_instance(player, card)->targets[1].card = 1<<choose_a_color_and_show_legacy(player, card, 1-player, -1);
	}

	if (event == EVENT_SET_COLOR && ! is_humiliated(player, card) ){
		int col = get_card_instance(player, card)->targets[1].card;
		if (col > 0 && is_what(affected_card_controller, affected_card, TYPE_PERMANENT) && ! is_what(affected_card_controller, affected_card, TYPE_LAND) ){
			event_result = col;
		}
	}

	return global_enchantment(player, card, event);
}

int card_shivan_wurm(int player, int card, event_t event){
	return gating(player, card, event, COLOR_TEST_GREEN, COLOR_TEST_RED);
}

// silver drake --> sawtooth_loon

// spark caster --> shivan wurm

int card_star_compass(int player, int card, event_t event){
	/*
	  Star Compass |2
	  Artifact
	  Star Compass enters the battlefield tapped.
	  {T}: Add to your mana pool one mana of any color that a basic land you control could produce.
	*/
	comes_into_play_tapped(player, card, event);
	return card_fellwar_stone(player, card, event);
}

int card_stormscape_battlemage(int player, int card, event_t event){
	/*
	  Stormscape Battlemage |2|U
	  Creature - Metathran Wizard 2/2
	  Kicker {W} and/or {2}{B}
	  When Stormscape Battlemage enters the battlefield, if it was kicked with its {W} kicker, you gain 3 life.
	  When Stormscape Battlemage enters the battlefield, if it was kicked with its {2}{B} kicker, destroy target nonblack creature. That creature can't be regenerated.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.illegal_color = COLOR_TEST_BLACK;
		td.allow_cancel = 0;

		volver(player, card, event, MANACOST_XB(2, 1), MANACOST_W(1), 10*can_target(&td), 5 + life[player] < 6 ? 10 : 0);
	}

	if( comes_into_play(player, card, event) ){
		if( check_special_flags(player, card, SF_KICKED) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
			td.allow_cancel = 0;
			if( can_target(&td) && new_pick_target(&td, get_sleighted_color_text(player, card, "Select target non%s creature", COLOR_BLACK), 0, GS_LITERAL_PROMPT) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
		if( check_special_flags(player, card, SF_KICKED2) ){
			gain_life(player, 3);
		}
	}

	return 0;
}

int card_stormscape_familiar(int player, int card, event_t event)
{
  /* Stormscape Familiar	|1|U
   * Creature - Bird 1/1
   * Flying
   * |SWhite spells and |Sblack spells you cast cost |1 less to cast. */

  familiar(player, card, event, COLOR_TEST_WHITE | COLOR_TEST_BLACK);
  return 0;
}

int card_strafe(int player, int card, event_t event)
{
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_RED);

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  instance->number_of_targets = 0;
	  pick_next_target_noload(&td, get_sleighted_color_text(player, card, "Select target non%s creature.", COLOR_RED));
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_stratadon(int player, int card, event_t event){
	if(event == EVENT_MODIFY_COST ){
		COST_COLORLESS -= count_domain(player, card);
	}
	return 0;
}

int card_sunken_hope(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = current_turn;
	td.preferred_controller = current_turn;
	td.who_chooses = current_turn;
	td.illegal_abilities = 0;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_sunscape_battlemage(int player, int card, event_t event){
	/*
	  Sunscape Battlemage |2|W
	  Creature - Human Wizard 2/2
	  Kicker {1}{G} and/or {2}{U} (You may pay an additional {1}{G} and/or {2}{U} as you cast this spell.)
	  When Sunscape Battlemage enters the battlefield, if it was kicked with its {1}{G} kicker, destroy target creature with flying.
	  When Sunscape Battlemage enters the battlefield, if it was kicked with its {2}{U} kicker, draw two cards.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_abilities = KEYWORD_FLYING;
		td.allow_cancel = 0;

		volver(player, card, event, MANACOST_XU(2, 1), MANACOST_XG(1, 1), 5 + (hand_count[player] < 6 ? 5 : 0), 10*can_target(&td));
	}

	if( comes_into_play(player, card, event) ){
		if( check_special_flags(player, card, SF_KICKED) ){
			draw_cards(player, 2);
		}
		if( check_special_flags(player, card, SF_KICKED2) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.required_abilities = KEYWORD_FLYING;
			td.allow_cancel = 0;
			if( can_target(&td) && new_pick_target(&td, "Select target creature with flying.", 0, GS_LITERAL_PROMPT) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_sunscape_familiar(int player, int card, event_t event)
{
  /* Sunscape Familiar	|1|W
   * Creature - Wall 0/3
   * Defender
   * |SGreen spells and |Sblue spells you cast cost |1 less to cast. */

  familiar(player, card, event, COLOR_TEST_GREEN | COLOR_TEST_BLUE);
  return 0;
}

int card_tahngarth_talruum_hero(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	if( player == AI ){
		td.power_requirement = get_toughness(player, card)-1;
	}
	card_instance_t *instance= get_card_instance(player, card);

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int my_power = get_power(player, instance->parent_card);
			int his_power = get_power(instance->targets[0].player, instance->targets[0].card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, my_power, player, instance->parent_card);
			damage_creature(player, instance->parent_card, his_power, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 1, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_terminal_moraine(int player, int card, event_t event){

	/* Terminal Moraine	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |2, |T, Sacrifice ~: Search your library for a basic land card and put that card onto the battlefield tapped. Then shuffle your library. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) && can_use_activated_abilities(player, card) &&
			can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac & Tutor basic land\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				untap_card_no_event(player, card);
				spell_fizzled = 1;
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 1);
			}
			else{
				return mana_producer(player, card, event);
			}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_terminate(int player, int card, event_t event){
	/*
	  Terminate |B|R
	  Instant
	  Destroy target creature. It can't be regenerated.
	*/
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

int card_thornscape_battlemage(int player, int card, event_t event){
	/*
	  Thornscape Battlemage |2|G
	  Creature - Elf Wizard 2/2
	  Kicker {R} and/or {W} (You may pay an additional {R} and/or {W} as you cast this spell.)
	  When Thornscape Battlemage enters the battlefield, if it was kicked with its {R} kicker, it deals 2 damage to target creature or player.
	  When Thornscape Battlemage enters the battlefield, if it was kicked with its {W} kicker, destroy target artifact.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_ARTIFACT);
		td1.allow_cancel = 0;

		volver(player, card, event, MANACOST_R(1), MANACOST_W(1), 8*can_target(&td), 5*can_target(&td1));
	}

	if( comes_into_play(player, card, event) ){
		if( check_special_flags(player, card, SF_KICKED) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
			td.allow_cancel = 0;

			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				damage_target0(player, card, 2);
			}
		}
		instance->number_of_targets = 0;
		if( check_special_flags(player, card, SF_KICKED2) ){
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_ARTIFACT);
			td1.allow_cancel = 0;

			if( can_target(&td1) && pick_target(&td1, "TARGET_ARTIFACT") ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
		instance->number_of_targets = 0;
	}

	return 0;
}

int card_thornscape_familiar(int player, int card, event_t event)
{
  /* Thornscape Familiar	|1|G
   * Creature - Insect 2/1
   * |SRed spells and |Swhite spells you cast cost |1 less to cast. */

  familiar(player, card, event, COLOR_TEST_RED | COLOR_TEST_WHITE);
  return 0;
}

int card_thunderscape_battlemage(int player, int card, event_t event){
	/*
	  Thunderscape Battlemage |2|R
	  Creature - Human Wizard 2/2
	  Kicker {1}{B} and/or {G} (You may pay an additional {1}{B} and/or {G} as you cast this spell.)
	  When Thunderscape Battlemage enters the battlefield, if it was kicked with its {1}{B} kicker, target player discards two cards.
	  When Thunderscape Battlemage enters the battlefield, if it was kicked with its {G} kicker, destroy target enchantment.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_ENCHANTMENT);
		td1.allow_cancel = 0;

		volver(player, card, event, MANACOST_XB(1, 1), MANACOST_G(1), 8*can_target(&td), 5*can_target(&td1));
	}

	if( comes_into_play(player, card, event) ){
		if( check_special_flags(player, card, SF_KICKED) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.zone = TARGET_ZONE_PLAYERS;
			td.allow_cancel = 0;

			if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
				new_multidiscard(instance->targets[0].player, 2, 0, player);
			}
		}
		if( check_special_flags(player, card, SF_KICKED2) ){
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_ENCHANTMENT);
			td1.allow_cancel = 0;

			if( can_target(&td1) && pick_target(&td1, "TARGET_ENCHANTMENT") ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_thunderscape_familiar(int player, int card, event_t event)
{
  /* Thunderscape Familiar	|1|R
   * Creature - Kavu 1/1
   * First strike
   * |SBlack spells and |Sgreen spells you cast cost |1 less to cast. */

  familiar(player, card, event, COLOR_TEST_BLACK | COLOR_TEST_GREEN);
  return 0;
}

int card_treva_charm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int ai_choice = 0;
			int mode = 0;
			mode = (1<<2) | (1<<3);
			if( can_target(&td) ){
				mode+=(1<<0);
			}
			if( can_target(&td1) ){
				mode+=(1<<1);
			}
			int choice = 0;
			char buffer[500];
			int pos = 0;
			if( mode & (1<<0) ){
				pos += scnprintf(buffer + pos, 500-pos, " Kill an enchantment\n");
			}
			if( mode & (1<<1) ){
				pos += scnprintf(buffer + pos, 500-pos, " Exile an attacking creature\n");
				ai_choice = 1;
			}
			if( mode & (1<<2) ){
				pos += scnprintf(buffer + pos, 500-pos, " Draw & discard\n");
				if( !(mode & (1<<0)) && ! (mode &(1<<1)) ){
					ai_choice = 2;
				}
			}
			pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
			choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			while( !( (1<<choice) & mode) ){
					choice++;
			}
			if( choice == 3 ){
				spell_fizzled = 1;
			}
			else{
				if( choice == 0 ){
					if( pick_target(&td, "TARGET_ENCHANTMENT") ){
						instance->info_slot = 66+choice;
					}
				}
				else if( choice == 1 ){
						if( pick_target(&td1, "TARGET_CREATURE") ){
							instance->info_slot = 66+choice;
						}
				}
				else if( choice == 2 ){
						instance->info_slot = 66+choice;
				}
			}
	}
	else if(event == EVENT_RESOLVE_SPELL){
			if( instance->info_slot == 66 && valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			if( instance->info_slot == 67 && valid_target(&td1) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
			if( instance->info_slot == 68 ){
				draw_cards(player, 1);
				discard(player, 0, player);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_urzas_guilt(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
				draw_cards(p, 2);
				new_multidiscard(p, 3, 0, player);
				lose_life(p, 4);
			};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_voice_of_all(int player, int card, event_t event){
	// original code : 0x4e4a34

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[8].player != -1 ){
		modify_pt_and_abilities(player, card, event, 0, 0, instance->targets[8].player);
	}

	if( comes_into_play(player, card, event) ){
		instance->targets[8].player = select_a_protection(player);
	}

	return 0;
}

int card_warped_devotion(int player, int card, event_t event){
	/*
	  0x4151A0
	  Warped Devotion |2|B
	  Enchantment
	  Whenever a permanent is returned to a player's hand, that player discards a card.
	*/
	if( ! is_humiliated(player, card) ){
		if( trigger_condition == TRIGGER_BOUNCE_PERMANENT ){
			if( affect_me( player, card) && reason_for_trigger_controller == player ){
				if( trigger_cause != -1 ){ //Won't trigger for tokens
					if(event == EVENT_TRIGGER){
						event_result |= RESOLVE_TRIGGER_MANDATORY;
					}
					else if(event == EVENT_RESOLVE_TRIGGER){
							discard(trigger_cause_controller, 0, player);
					}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}
