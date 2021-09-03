#include "manalink.h"

// ------------ General functions
void uthden_creature(int player, int card, event_t event, int land_color){
	if( ! is_humiliated(player, card) && (event== EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) &&
		basiclandtypes_controlled[player][get_hacked_color(player, card, land_color)] >= 1
	  ){
		event_result++;
	}
}

static int uthden_m13_creature(int player, int card, event_t event, int land_color, int cless, int black, int blue, int green, int red, int white, int k_pump, int sp_key_pump){
	uthden_creature(player, card, event, land_color);
	return generic_shade(player, card, event, 0, cless, black, blue, green, red, white, 0, 0, k_pump, sp_key_pump);
}

static int m13_ring(int player, int card, event_t event, int clr, int key, int s_key){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_equipping(player, card) ){
		if( get_color(instance->targets[8].player, instance->targets[8].card) & clr ){
			upkeep_trigger_ability(player, card, event, player);

			if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
				add_1_1_counter(instance->targets[8].player, instance->targets[8].card);
			}
		}
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 0, 0, key);
		special_abilities(instance->targets[8].player, instance->targets[8].card, event, s_key, player, card);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_equipping(player, card) ){
			return can_activate_basic_equipment(player, card, event, 1);
		}
		else{
			if( get_id(player, card) == CARD_ID_RING_OF_EVOS_ISLE ){
				if( has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
					return 1;
				}
			}
			if( get_id(player, card) == CARD_ID_RING_OF_XATHRID ){
				if( has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) && (land_can_be_played & LCBP_REGENERATION) ){
					return can_regenerate(instance->targets[8].player, instance->targets[8].card);
				}
			}
			return can_activate_basic_equipment(player, card, event, 1);
		}
	}
	else if( event == EVENT_ACTIVATE ){
			if( ! is_equipping(player, card) ){
				ai_modifier+=100;
			}
			int choice = 0;
			if( is_equipping(player, card) ){
				if( get_id(player, card) == CARD_ID_RING_OF_EVOS_ISLE && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
					if( can_activate_basic_equipment(player, card, event, 1) ){
						choice = do_dialog(player, player, card, -1, -1, " Equip\n Give Hexproof\n Cancel", 0);
					}
					else{
						choice = 1 ;
					}
				}
				if( get_id(player, card) == CARD_ID_RING_OF_XATHRID ){
					if( has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) && (land_can_be_played & LCBP_REGENERATION) ){
						if( can_regenerate(instance->targets[8].player, instance->targets[8].card) ){
							choice = 1;
						}
					}
				}
			}
			if( choice == 0 ){
				activate_basic_equipment(player, card, 1);
				instance->info_slot = 1;
			}
			else if( choice == 1 ){
					charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
					if( spell_fizzled != 1 ){
						instance->info_slot = 2;
					}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 1 ){
				resolve_activation_basic_equipment(player, card);
			}
			if( instance->info_slot == 2 ){
				if( get_id(player, instance->parent_card) == CARD_ID_RING_OF_EVOS_ISLE ){
					pump_ability_until_eot(player, instance->parent_card, instance->targets[8].player, instance->targets[8].card,
											0, 0, 0, SP_KEYWORD_HEXPROOF);
				}
				if( get_id(player, instance->parent_card) == CARD_ID_RING_OF_XATHRID ){
					regenerate_target(instance->targets[8].player, instance->targets[8].card);
				}
			}
	}
	return 0;
}

//--------------- Cards

// Ajani Sunstriker --> Child of Night

int card_ajani_caller_of_the_pride(int player, int card, event_t event){

	/* Ajani, Caller of the Pride	|1|W|W
	 * Planeswalker - Ajani (4)
	 * +1: Put a +1/+1 counter on up to one target creature.
	 * -3: Target creature gains flying and double strike until end of turn.
	 * -8: Put X 2/2 |Swhite Cat creature tokens onto the battlefield, where X is your life total. */

	if (IS_ACTIVATING(event)){

		card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		int priority_hyperpump = 0;
		if( event == EVENT_ACTIVATE ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING | KEYWORD_REACH;
			priority_hyperpump = ! check_battlefield_for_special_card(player, card, 1-player, 0, &this_test) ? 15 : 5;
			priority_hyperpump += ((count_counters(player, card, COUNTER_LOYALTY)*10)-30);
		}

		int priority_caturday = 0;
		if( event == EVENT_ACTIVATE ){
			priority_caturday += life[player] * 4;
			priority_hyperpump += ((count_counters(player, card, COUNTER_LOYALTY)*10)-80);
		}

		enum{
			CHOICE_COUNTER = 1,
			CHOICE_HYPERPUMP,
			CHOICE_CATURDAY
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Add a +1/+1 counter", 1, 10, 1,
						"Hyperpump a creature", can_target(&td), priority_hyperpump, -3,
						"Super Caturday", 1, priority_caturday, -8);

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
				case CHOICE_COUNTER:
				{
					if( can_target(&td) ){
						new_pick_target(&td, "TARGET_CREATURE", 0, 0);
					}
				}
				break;

				case CHOICE_HYPERPUMP:
					pick_target(&td, "TARGET_CREATURE");
					break;

				case CHOICE_CATURDAY:
					break;

			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_COUNTER:
			{
				if( instance->number_of_targets == 1 && valid_target(&td)){
					add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				}
			}
			break;
			case CHOICE_HYPERPUMP:
			{
				if( valid_target(&td)){
					pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
											0, 0, KEYWORD_FLYING | KEYWORD_DOUBLE_STRIKE, 0);
				}
			}
			break;

			case CHOICE_CATURDAY:
			{
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_CAT, &token);
				token.pow = 2;
				token.tou = 2;
				token.qty = life[player];
				generate_token(&token);
			}
			break;
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_archaeomancer(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, "Select target Instant or Sorcery card.");

		if( new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(player) ){
			if( comes_into_play(player, card, event) ){
				new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
			}
		}
	}

	return 0;
}

int card_arctic_aven(int player, int card, event_t event){

	return uthden_m13_creature(player, card, event, COLOR_WHITE, MANACOST_W(1), 0, SP_KEYWORD_LIFELINK);
}

int card_attended_knight(int player, int card, event_t event){
	/* Attended Knight	|2|W
	 * Creature - Human Knight 2/2
	 * First strike
	 * When ~ enters the battlefield, put a 1/1 |Swhite Soldier creature token onto the battlefield. */

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SOLDIER, &token);
		token.pow = token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return 0;
}

int card_augur_of_bolas(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int to_reveal = MIN(3, count_deck(player));
		if( to_reveal > 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_SPELL);
			this_test.type_flag = F1_NO_CREATURE;
			this_test.create_minideck = to_reveal;
			this_test.no_shuffle = 1;
			if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test) != -1){
				to_reveal--;
			}
			if( to_reveal > 0 ){
				put_top_x_on_bottom(player, player, to_reveal);
			}
		}
	}

	return 0;
}

int card_battlefligh_eagle(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.preferred_controller = player;
		td1.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td1) && comes_into_play(player, card, event) ){
			if( pick_target(&td1, "TARGET_CREATURE") ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2, KEYWORD_FLYING, 0);
			}
		}
	}

	return 0;
}

int card_blood_reckoning(int player, int card, event_t event)
{
  // Whenever a creature attacks you or a planeswalker you control, that creature's controller loses 1 life.
  if (declare_attackers_trigger(player, card, event, DAT_SEPARATE_TRIGGERS, 1-player, -1))
	lose_life(current_turn, 1);

  return global_enchantment(player, card, event);
}

int card_bloodhunter_bat(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;
		td1.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td1) && comes_into_play(player, card, event) ){
			if( pick_target(&td1, "TARGET_PLAYER") ){
				lose_life(instance->targets[0].player, 2);
				gain_life(player, 2);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

int card_boundless_realms(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = count_subtype(player, TYPE_LAND, -1);
		if( amount > 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.subtype = SUBTYPE_BASIC;
			this_test.qty = amount;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_captains_call(int player, int card, event_t event){
	/* Captain's Call	|3|W
	 * Sorcery
	 * Put three 1/1 |Swhite Soldier creature tokens onto the battlefield. */

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SOLDIER, &token);
		token.pow = token.tou = 1;
		token.qty = 3;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_cathedral_of_war(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	exalted(player, card, event, 0, 0);

	return mana_producer(player, card, event);
}


int card_chandras_fury(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			damage_player(instance->targets[0].player, 4, player, card);
			new_damage_all(player, card, instance->targets[0].player, 1, 1, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_PLAYER", 1, NULL);
}

int card_chadras_fury(int player, int card, event_t event){
	return 0;
}

int card_chronomaton(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_1_1_counter(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(1), 0, NULL, NULL);
}

int card_cleaver_riot(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_courtly_provacateur(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int ai_choice = 0;
			if( current_turn == player  ){
				ai_choice = 1;
			}
			int choice = do_dialog(player, player, card, -1, -1," Force to attack\n Force to block", ai_choice);
			int key = SP_KEYWORD_MUST_ATTACK;
			if( choice == 1 ){
				key = SP_KEYWORD_MUST_BLOCK;
			}
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, key);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_cower_in_fear(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, 1-player, -1, -1, -1, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

// craterize --> stone rain

int card_crimson_muckwader(int player, int card, event_t event){
	if( (event== EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) &&
		basiclandtypes_controlled[player][get_hacked_color(player, card, COLOR_BLACK)] >= 1
	  ){
		event_result++;
	}
	return regeneration(player, card, event, 2, 1, 0, 0, 0, 0);
}

int card_crippling_blight(int player, int card, event_t event){
	return generic_aura(player, card, event, 1-player, -1, -1, 0, SP_KEYWORD_CANNOT_BLOCK, 0, 0, 0);
}

// crusader of odric -->keldon warlord

int card_diabolic_revelation(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 ){
			instance->info_slot = x_value;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot > 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			this_test.qty = instance->info_slot;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, &this_test);
			shuffle(player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_disciple_of_bolas(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.allow_cancel = 0;
		td1.preferred_controller = player;
		td1.allowed_controller = player;
		td1.illegal_abilities = 0;

		if( comes_into_play(player, card, event) ){
			state_untargettable(player, card, 1);
			if( can_target(&td1) ){
				int result = pick_creature_for_sacrifice(player, card, 1);
				int amount = get_power(player, result);
				kill_card(player, result, KILL_SACRIFICE);
				gain_life(player, amount);
				draw_cards(player, amount);
			}
			state_untargettable(player, card, 0);
		}
	}

	return 0;
}

int card_downpour(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 3, NULL);
}

// dragon hatchling --> shivan dragon

int card_duskmantle_prowler(int player, int card, event_t event){
	haste(player, card, event);
	exalted(player, card, event, 0, 0);
	return 0;
}

int card_duty_bound_dead(int player, int card, event_t event){
	exalted(player, card, event, 0, 0);
	return regeneration(player, card, event, MANACOST_XB(3, 1));
}

int card_elderscale_wurm(int player, int card, event_t event)
{
  if (comes_into_play(player, card, event) && life[player] < 7)
	set_life_total(player, 7);

  card_instance_t* damage;
  if (event == EVENT_DAMAGE_REDUCTION
	  && event_result < 7
	  && life[player] >= 7
	  && (damage = get_card_instance(affected_card_controller, affected_card))
	  && damage->damage_target_player == player && damage->damage_target_card == -1)
	event_result = 7;

  if (event == EVENT_SHOULD_AI_PLAY && in_play(player, card) && life[player] >= 7)
	{
	  if (player == AI)
		ai_modifier += 480;
	  else
		ai_modifier -= 480;
	}

  return 0;
}

int card_encrust(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
		if( leaves_play(player, card, event) ){
			disable_all_activated_abilities(instance->damage_target_player, instance->damage_target_card, 0);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE | TYPE_ARTIFACT);

	return disabling_targeted_aura(player, card, event, &td1, "Select target artifact or creature.", 2);
}

int card_faerie_invaders(int player, int card, event_t event){
	return flash(player, card, event);
}

int card_faiths_reward(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		reanimate_all_dead_this_turn(player, TYPE_PERMANENT | GDC_NONTOKEN);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_firewing_phoenix(int player, int card, event_t event){
	if (event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, MANACOST_XR(1, 3))){
		return GA_RETURN_TO_HAND;
	}

	if (event == EVENT_PAY_FLASHBACK_COSTS){
		charge_mana_multi(player, MANACOST_XR(1, 3));
		if( spell_fizzled != 1 ){
			return GAPAID_REMOVE;
		}
	}
	return 0;
}

int card_flinthoof_boar(int player, int card, event_t event){
	return uthden_m13_creature(player, card, event, COLOR_RED, MANACOST_R(1), 0, SP_KEYWORD_HASTE);
}


int card_flames_of_the_firebrand(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL){
		divide_damage(player, card, &td);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 3, NULL);
}

int card_fungal_sprouting(int player, int card, event_t event){
	/* Fungal Sprouting	|3|G
	 * Sorcery
	 * Put X 1/1 |Sgreen Saproling creature tokens onto the battlefield, where X is the greatest power among creatures you control. */

	if( event == EVENT_RESOLVE_SPELL){
		int amount = 0;
		int count = 0;
		while(count < active_cards_count[player] ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					if( get_power(player, count) > amount ){
						amount = get_power(player, count);
					}
				}
				count++;
		}

		generate_tokens_by_id(player, card, CARD_ID_SAPROLING, amount);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_gem_of_becoming(int player, int card, event_t event)
{
  /* Gem of Becoming	|3
   * Artifact
   * |3, |T, Sacrifice ~: Search your library for an |H2Island card, a |H2Swamp card, and a |H2Mountain card. Reveal those cards and put them into your
   * hand. Then shuffle your library. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  int i, subtypes[3] = { SUBTYPE_ISLAND, SUBTYPE_SWAMP, SUBTYPE_MOUNTAIN };
	  test_definition_t test;
	  for (i = 0; i < 3; ++i)
		{
		  new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", subtypes[i]));
		  test.subtype = get_hacked_subtype(player, card, subtypes[i]);
		  if (i != 2)
			test.no_shuffle = 1;

		  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_ME, MANACOST_X(3), 0, NULL, NULL);
}

// glorious charge --> warrior's honor

int card_goblin_battle_jester(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0, COLOR_TEST_RED, 0, 0, 0, -1, 0)  ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) ){
			pick_target(&td, "TARGET_CREATURE");
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			instance->number_of_targets = 1;
		}
	}
	return 0;
}

int card_griffin_protector(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		pump_ability_until_eot(player, card, player, card, 1, 1, 0, 0);
	}

  return 0;
}

// ground seal --> vanilla
// guardian lions --> serra angel

int card_harbor_bandit(int player, int card, event_t event){
	return uthden_m13_creature(player, card, event, COLOR_BLUE, 1, 0, 1, 0, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
}

int card_healer_of_the_pride(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		gain_life(player, 2);
	}

  return 0;
}

int card_hellion_crucible(int player, int card, event_t event){

	/* Hellion Crucible	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |1|R, |T: Put a pressure counter on ~.
	 * |1|R, |T, Remove two pressure counters from ~ and sacrifice it: Put a 4/4 |Sred Hellion creature token with haste onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	enum{
		CHOICE_MANA = 1,
		CHOICE_COUNTER,
		CHOICE_HELLION
	};

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = CHOICE_MANA;
		if( ! paying_mana() ){
			int abil_counter = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_XR(2, 1), 0, NULL, NULL);
			int abil_hellion = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_XR(2, 1),
														GVC_COUNTERS(COUNTER_PRESSURE, 2), NULL, NULL);
			choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Produce mana", 1, 8,
							"Add Pressure counter", abil_counter, 10,
							"Generate an Hellion", abil_hellion, count_subtype(player, TYPE_CREATURE, -1) < 2 ? 15 : 0);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
		}
		instance->info_slot = choice;
		if( choice == CHOICE_MANA ){
			return mana_producer(player, card, event);
		}
		if( choice == CHOICE_COUNTER || choice == CHOICE_HELLION ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_XR(1, 1)) ){
				tap_card(player, card);
				if( choice == CHOICE_HELLION ){
					remove_counters(player, card, COUNTER_PRESSURE, 2);
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == CHOICE_COUNTER ){
			add_counter(instance->parent_controller, instance->parent_card, COUNTER_PRESSURE);
		}
		if( instance->info_slot == CHOICE_HELLION ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_HELLION, &token);
			token.color_forced = COLOR_TEST_RED;
			token.pow = token.tou = 4;
			token.s_key_plus = SP_KEYWORD_HASTE;
			generate_token(&token);
		}
	}

	return 0;
}

int card_hydrosurge(int player, int card, event_t event){
	/* Hydrosurge	|U
	 * Instant
	 * Target creature gets -5/-0 until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, 1-player, -5, 0, 0, 0);
}

int card_jaces_phantasm(int player, int card, event_t event){
	if( (event== EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( count_graveyard(1-player) >  9 ){
			event_result+=4;
		}
	}
	return 0;
}

int card_kindled_fury(int player, int card, event_t event){
	/* Kindled Fury	|R
	 * Instant
	 * Target creature gets +1/+0 and gains first strike until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 1, 0, KEYWORD_FIRST_STRIKE, 0);
}

// knight of glory --> servant_of_nefarox

// knight of infamy --> servant_of_nefarox

// krenko's command --> dragon fodder

int card_krenko_mob_boss(int player, int card, event_t event){
	/* Krenko, Mob Boss	|2|R|R
	 * Legendary Creature - Goblin Warrior 3/3
	 * |T: Put X 1/1 |Sred Goblin creature tokens onto the battlefield, where X is the number of Goblins you control. */

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, count_subtype(player, TYPE_PERMANENT, SUBTYPE_GOBLIN));
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_liliana_of_the_dark_realms(int player, int card, event_t event){

	/* Liliana of the Dark Realms	|2|B|B
	 * Planeswalker - Liliana (3)
	 * +1: Search your library for a |H2Swamp card, reveal it, and put it into your hand. Then shuffle your library.
	 * -3: Target creature gets +X/+X or -X/-X until end of turn, where X is the number of |H1Swamps you control.
	 * -6: You get an emblem with "|H1Swamps you control have '|T: Add |B|B|B|B to your mana pool.'" */

	if (IS_ACTIVATING(event)){
		card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

	  enum{
			CHOICE_TUTOR_SWAMP = 1,
			CHOICE_PUMP_OR_WEAKEN = 2,
			CHOICE_EMBLEM = 3
	  };
	  int choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Tutor a Swamp", 1, 5, 1,
						"Pump / Weaken creature", can_target(&td), 10, -3,
						"Get an Emblem", 1, check_battlefield_for_id(player, CARD_ID_LILIANAS_EMBLEM) ? -1 : 10, -6);

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
			  case CHOICE_PUMP_OR_WEAKEN:
				pick_target(&td, "TARGET_CREATURE");
				break;

			  case CHOICE_TUTOR_SWAMP:
				break;

			  case CHOICE_EMBLEM:
				break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_TUTOR_SWAMP:
			{
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_SWAMP));
				this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_SWAMP);
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, &this_test);
				break;
			}


			case CHOICE_PUMP_OR_WEAKEN:
				if( valid_target(&td) ){
					int amount = basiclandtypes_controlled[player][get_hacked_color(player, card, COLOR_BLACK)];
					int pump = amount;
					int choice2 = do_dialog(player, player, instance->parent_card, -1, -1, " Weaken target\n Pump target", instance->targets[0].player == player ? 1 : 0);
					if( choice2 == 0 ){
						pump = -amount;
					}
					pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, pump, pump);
				}
				break;

			case CHOICE_EMBLEM:
			  generate_token_by_id(player, card, CARD_ID_LILIANAS_EMBLEM);
			  break;
		  }
	}
	return planeswalker(player, card, event, 3);
}

int card_lilianas_emblem(int player, int card, event_t event){
	/* The obvious and user-friendly implementation of adding BBB whenever a swamp is tapped for mana unfortunately isn't accurate - it breaks for e.g. an
	 * Underground Sea tapped for U (which should produce U, not UBBB).  So we fall back on the icky "click on this card, then choose a swamp to tap" interface.
	 * I considered combining this with a Mana Flare-like effect when a swamp is tapped for exactly B; however, this will break if something else has already
	 * replaced it.  For example, a Mana Reflection that's already doubled the mana before this can modify it will make this see that it's been tapped for BB
	 * and do nothing, when the player is expecting this to produce BBBB and the Mana Reflection to then double it to BBBBBBBB.  Since EVENT_TAP_CARD is handled
	 * as an unordered event instead of a proper trigger, the player can't sort the effects properly to fix it explicitly, either.
	 *
	 * The interface is poor, but at least the right mana ends up being produced.
	 *
	 * It doesn't end up being declared correctly, unfortunately, due to the usual problem with EVENT_COUNT_MANA and permanents_you_control_can_tap_for_mana() -
	 * it's impossible to replace the mana that the permanent itself declares available.  On the other hand, we know that they're swamps, so they can produce at
	 * least B; as a closer-than-usual approximation, this declares an additional BBB for cards it can tap, instead of the BBBB it normally would.  It works out
	 * right for basic swamps, though the Underground Sea from before will still end up declaring as "either BBBB or UBBB". */
	return permanents_you_control_can_tap_for_mana(player, card, event, TYPE_LAND, SUBTYPE_SWAMP, COLOR_BLACK, event == EVENT_COUNT_MANA ? 3 : 4);
}

int card_lilianas_shade(int player, int card, event_t event){

	/* Liliana's Shade	|2|B|B
	 * Creature - Shade 1/1
	 * When ~ enters the battlefield, you may search your library for a |H2Swamp card, reveal it, put it into your hand, then shuffle your library.
	 * |B: ~ gets +1/+1 until end of turn. */

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_SWAMP));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_SWAMP);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}
	return generic_shade(player, card, event, 0, MANACOST_B(1), 1, 1, 0, 0);
}

int card_magmaquake(int player, int card, event_t event){

	/* Magmaquake	|X|R|R
	 * Instant
	 * ~ deals X damage to each creature without flying and each planeswalker. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot > 0 ){
			APNAP(p,{
						int c;
						for (c = active_cards_count[p]-1; c > -1 ; c--){
							if (in_play(p, c) && ((is_what(p, c, TYPE_CREATURE) && !check_for_ability(p, c, KEYWORD_FLYING)) || is_planeswalker(p, c))){
								damage_creature(p, c, instance->info_slot, player, card);
							}
						};
					};
			);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_mark_of_the_vampire(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 2, 2, 0, SP_KEYWORD_LIFELINK, 0, 0, 0);
}

int card_master_of_the_pearl_trident(int player, int card, event_t event){
	return boost_creature_type(player, card, event, SUBTYPE_MERFOLK, 1, 1, get_hacked_walk(player, card, KEYWORD_ISLANDWALK), BCT_CONTROLLER_ONLY);
}


int card_mind_sculpt(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			mill(instance->targets[0].player, 7);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td1, "TARGET_PLAYER", 1, NULL);
}

int card_mindclaw_shaman(int player, int card, event_t event){
	/* Mindclaw Shaman	|4|R
	 * Creature - Viashino Shaman 2/2
	 * When ~ enters the battlefield, target opponent reveals his or her hand. You may cast an instant or sorcery card from it without paying its mana cost. */

	if( comes_into_play(player, card, event) && hand_count[1-player] > 0 && target_opponent(player, card) ){

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, "Select an instant or sorcery card.");

		int selected = new_select_a_card(player, 1-player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
		if( selected != -1 ){
			opponent_plays_card_in_your_hand_for_free(1-player, selected);
		}
	}
	return 0;
}

int card_murder2(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_CREATURE", 1, NULL);
}

int card_mwonvuli_beast_tracker(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
#pragma message "We have no way to know if a creature outside play has Deathtouch or Hexproof among its static abilities, so this will only fecth permanents with Reach or Teample"
		this_test.keyword = KEYWORD_REACH | KEYWORD_TRAMPLE;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, AI_MAX_VALUE, &this_test);
	}
	return 0;
}

static int odric_effect(int player, int card, event_t event)
{
  if (event == EVENT_DECLARE_BLOCKERS || event == EVENT_CLEANUP)
	{
	  event_flags &= ~EF_ATTACKER_CHOOSES_BLOCKERS;	// Assumes all such effects last only for this combat
	  kill_card(player, card, event);
	}

  return 0;
}

int card_odric_master_tactician(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // Whenever ~ and at least three other creatures attack, you choose which creatures block this combat and how those creatures block.
  if (event == EVENT_DECLARE_ATTACKERS && is_attacking(player, card) && count_attackers(player) > 3 && ! is_humiliated(player, card) )
	{
	  event_flags |= EF_ATTACKER_CHOOSES_BLOCKERS;
	  create_legacy_effect(player, card, odric_effect);
	}

  return 0;
}

int card_nefarox_overlord_of_grixis(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if (exalted(player, card, event, 0, 0) == card){
		impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		get_card_instance(player, card)->targets[1].player = 0;	// sacrifice() casually overwrites it, confusing declare_attackers_trigger() via exalted()
	}
	return 0;
}

int card_omniscience(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL ){
		if( affected_card_controller == player && ! is_what(affected_card_controller,affected_card, TYPE_LAND) ){
			null_casting_cost(affected_card_controller,affected_card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_predatory_rampage(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 3, 3, 0, 0);
		pump_subtype_until_eot(player, card, 1-player, -1, 0, 0, 0, SP_KEYWORD_MUST_BLOCK);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

// primal huntbeast --> gladecover scout

int card_prized_elephant(int player, int card, event_t event){

	return uthden_m13_creature(player, card, event, COLOR_GREEN, MANACOST_G(1), KEYWORD_TRAMPLE, 0);
}

int card_public_execution(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			pump_subtype_until_eot(player, card, instance->targets[0].player, -1, -2, 0, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td1, "Select target creature an opponent controls.", 1, NULL);
}

int card_rangers_path(int player, int card, event_t event){

	/* Ranger's Path	|3|G
	 * Sorcery
	 * Search your library for up to two |H2Forest cards and put them onto the battlefield tapped. Then shuffle your library. */

	 if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_FOREST));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		this_test.qty = 2;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &this_test);
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

// Reckless Brute --> Flameborn Hellion

// rhox faithmender --> child of night

int card_card_ring_of_evos_isle(int player, int card, event_t event){
	return m13_ring(player, card, event, COLOR_TEST_BLUE, 0, 0);
}

int card_ring_of_kalonia(int player, int card, event_t event){
	return m13_ring(player, card, event, COLOR_TEST_GREEN, KEYWORD_TRAMPLE, 0);
}

int card_ring_of_thune(int player, int card, event_t event){
	return m13_ring(player, card, event, COLOR_TEST_WHITE, 0, SP_KEYWORD_VIGILANCE);
}

int card_ring_of_valkas(int player, int card, event_t event){
	return m13_ring(player, card, event, COLOR_TEST_RED, 0, SP_KEYWORD_HASTE);
}

int card_ring_of_xathrid(int player, int card, event_t event){
	return m13_ring(player, card, event, COLOR_TEST_BLACK, 0, 0);
}

int card_roaring_primadox(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;

		return bounce_permanent_at_upkeep(player, card, event, &td);
	}

	return 0;
}

int card_rummaging_goblin(int player, int card, event_t event){


	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD, MANACOST0, 0, NULL, NULL);
}

int card_sands_of_delirium(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, instance->info_slot);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(-1), 0, &td, "TARGET_PLAYER");
}

// searing spear --> lighting bolt

int card_serpents_gift(int player, int card, event_t event){
	/* Serpent's Gift	|2|G
	 * Instant
	 * Target creature gains deathtouch until end of turn. */
	if (!IS_CASTING(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	return vanilla_pump(player, card, event, &td, 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
}

int card_show_of_valor(int player, int card, event_t event){
	/* Show of Valor	|1|W
	 * Instant
	 * Target creature gets +2/+4 until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 2, 4, 0, 0);
}

int card_slumbering_dragon(int player, int card, event_t event)
{
	// ~ can't attack or block unless it has five or more +1/+1 counters on it.
	if ((event == EVENT_ATTACK_LEGALITY || event == EVENT_BLOCK_LEGALITY) && affect_me(player, card) && count_1_1_counters(player, card) < 5 &&
		! is_humiliated(player, card)
	)
		event_result = 1;

  // Whenever a creature attacks you or a planeswalker you control, put a +1/+1 counter on ~.
  if (declare_attackers_trigger(player, card, event, DAT_SEPARATE_TRIGGERS, 1-player, -1))
	add_1_1_counter(player, card);

  return 0;
}

// smelt --> shatter

int card_spelltwine(int player, int card, event_t event){
	/*
	  Spelltwine |5|U
	  Sorcery
	  Exile target instant or sorcery card from your graveyard and target instant or sorcery card from an opponent's graveyard.
	  Copy those cards. Cast the copies if able without paying their mana costs. Exile Spelltwine.
	*/

	if( event == EVENT_CAN_CAST ){
		if( basic_spell(player, card, event) ){
			if( (count_graveyard_by_type(player, TYPE_SPELL) > 0 && ! graveyard_has_shroud(player)) &&
				(count_graveyard_by_type(1-player, TYPE_SPELL) > 0 && ! graveyard_has_shroud(1-player))
			  ){
				return 1;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, "Select an Instant or Sorcery card.");
		if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 0) != -1 ){
			if( select_target_from_grave_source(player, card, 1-player, 0, AI_MAX_CMC, &this_test, 1) == -1 ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int spells[2] = {-1, -1};
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			spells[0] = get_grave(player)[selected];
			rfg_card_from_grave(player, selected);
		}
		selected = validate_target_from_grave(player, card, 1-player, 1);
		if( selected != -1 ){
			spells[1] = get_grave(1-player)[selected];
			rfg_card_from_grave(1-player, selected);
		}
		int i;
		for(i=0; i<2; i++){
			if( spells[i] != -1 ){
				copy_spell(player, cards_data[spells[i]].id);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_staff_of_nin(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		draw_cards(player, 1);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_sublime_archangel(int player, int card, event_t event)
{
  // Exalted
  // Other creatures you control have exalted.
  if (declare_attackers_trigger(player, card, event, DAT_ATTACKS_ALONE|DAT_TRACK, player, -1))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int attacker = BYTE0(instance->targets[2].player);
	  if (in_play(player, attacker))
		{
		  int c, amount = 0;
		  for (c = 0; c < active_cards_count[player]; ++c)
			if (in_play(player, c) && is_what(player, c, TYPE_CREATURE) && !is_humiliated(player, c))
			  ++amount;

		  if (amount > 0)
			pump_until_eot(player, card, player, attacker, amount, amount);
		}
	}

  return 0;
}

int card_switcheroo(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.allowed_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.preferred_controller = 1-player;
	td1.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
			new_pick_target(&td, "Select target creature opponent controls.", 1, 1 | GS_LITERAL_PROMPT);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td1, 1) ){
			exchange_control_of_target_permanents(player, card, instance->targets[0].player, instance->targets[0].card,
													instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}


int card_talrands_invocation(int player, int card, event_t event){
	/* Talrand's Invocation	|2|U|U
	 * Sorcery
	 * Put two 2/2 |Sblue Drake creature tokens with flying onto the battlefield. */

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_DRAKE, &token);
		token.pow = token.tou = 2;
		token.color_forced = COLOR_TEST_BLUE;
		token.qty = 2;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}


int card_talrand_sky_summoner(int player, int card, event_t event){
	/* Talrand, Sky Summoner	|2|U|U
	 * Legendary Creature - Merfolk Wizard 2/2
	 * Whenever you cast an instant or sorcery spell, put a 2/2 |Sblue Drake creature token with flying onto the battlefield. */

	check_legend_rule(player, card, event);

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_SPELL, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_DRAKE, &token);
		token.pow = token.tou = 2;
		token.color_forced = COLOR_TEST_BLUE;
		generate_token(&token);
	}

	return 0;
}

int card_thragtusk(int player, int card, event_t event){
	/* Thragtusk	|4|G
	 * Creature - Beast 5/3
	 * When ~ enters the battlefield, you gain 5 life.
	 * When ~ leaves the battlefield, put a 3/3 |Sgreen Beast creature token onto the battlefield. */

	if( comes_into_play(player, card, event) ){
		gain_life(player, 5);
	}

	if( leaves_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BEAST, &token);
		token.pow = token.tou = 3;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);
	}

	return 0;
}


int card_thundermaw_hellkite(int player, int card, event_t event){

	haste(player, card, event);

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		new_damage_all(player, card, 1-player, 1, 0, &this_test);
		int count = active_cards_count[1-player]-1;
		while(count > -1){
				if( in_play(1-player, count) && check_for_ability(1-player, count, KEYWORD_FLYING) ){
					tap_card(1-player, count);
				}
				count--;
		}
	}

	return 0;
}

int card_timberpack_wolf(int player, int card, event_t event){
	if( (event== EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result+=(count_cards_by_id(player, get_id(player, card))-1);
	}
	return 0;
}

int card_touch_of_the_eternal(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		set_life_total(player, count_permanents_by_type(player, TYPE_PERMANENT));
	}
	return global_enchantment(player, card, event);
}

int card_trading_post(int player, int card, event_t event){
	/*
	  Trading Post |4
	  Artifact
	  {1}, {T}, Discard a card: You gain 4 life.
	  {1}, {T}, Pay 1 life: Put a 0/1 white Goat creature token onto the battlefield.
	  {1}, {T}, Sacrifice a creature: Return target artifact card from your graveyard to your hand.
	  {1}, {T}, Sacrifice an artifact: Draw a card.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t test;
	new_default_test_definition(&test, TYPE_ARTIFACT, "Select an artifact card.");

	test_definition_t test2;
	new_default_test_definition(&test2, TYPE_ANY, "Select a card to discard.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(1), 0, NULL, NULL) ){
		if( hand_count[player] > 0 ){
			return 1;
		}
		if( can_pay_life(player, 1) ){
			return 1;
		}
		if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) &&
			count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 && ! graveyard_has_shroud(player)
		  ){
			return 1;
		}
		if( can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		const int *grave = get_grave(player);
		int abils[4] = {	hand_count[player] > 0,
							can_pay_life(player, 1),
							can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) && count_graveyard_by_type(player, TYPE_ARTIFACT) > 0
							&& ! graveyard_has_shroud(player),
							can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
						};
		int priorities[4] = { 	player == AI ? 40-(life[player]*2) : 0,
								player == AI ? ((count_subtype(1-player, TYPE_CREATURE, -1)-count_subtype(player, TYPE_CREATURE, -1))*3) +
												(10 * (current_phase < PHASE_DECLARE_BLOCKERS)) + (5*(life[player] > 7)) : 0,
								player == AI ? count_subtype(player, TYPE_CREATURE, -1)*2 +
												get_base_value(-1, grave[new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_VALUE, -1, &test)])
												: 0,
								player == AI ? count_subtype(player, TYPE_ARTIFACT, -1)*2 + (14-(hand_count[player]*2)) : 0
							};

		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"Gain 4 life.", abils[0], priorities[0],
						"Generate a 0/1 white Goat.", abils[1], priorities[1],
						"Return an artifact.", abils[2], priorities[2],
						"Draw a card.", abils[3], priorities[3]);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
			if( choice == 1 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &test2);
				if( selected != -1 ){
					discard_card(player, selected);
					tap_card(player, card);
					instance->info_slot = choice;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			if( choice == 2 ){
				lose_life(player, 1);
				tap_card(player, card);
				instance->info_slot = choice;
			}
			if( choice == 3 ){
				if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					select_target_from_grave_source(player, card, player, 1, AI_MAX_VALUE, &test, 0);
					instance->info_slot = choice;
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			if( choice == 4 ){
				if( sacrifice(player, card, player, 0, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					instance->info_slot = choice;
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			gain_life(player, 4);
		}
		if( instance->info_slot == 2 ){
			generate_token_by_id(player, card, CARD_ID_GOAT);
		}
		if( instance->info_slot == 3 ){
			int selected = validate_target_from_grave_source(player, card, player, 0);
			if( selected != -1 ){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
		}
		if( instance->info_slot == 4 ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_tricks_of_the_trade(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 2, 0, 0, SP_KEYWORD_UNBLOCKABLE, 0, 0, 0);
}

int card_vedalken_entrancer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_U(1), 0, &td, "TARGET_PLAYER");
}


int card_veilborn_ghoul(int player, int card, event_t event){

	cannot_block(player, card, event);

	return 0;
}

int card_vile_rebirth(int player, int card, event_t event){
	/* Vile Rebirth	|B
	 * Instant
	 * Exile target creature card from a graveyard. Put a 2/2 |Sblack Zombie creature token onto the battlefield. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			rfg_card_from_grave(instance->targets[0].player, selected);
			generate_token_by_id(player, card, CARD_ID_ZOMBIE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER_BOTH_GRAVES, NULL, NULL, 1, &this_test);
}

int card_void_stalker(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if (!(instance->targets[0].card == card && instance->targets[0].player == player)){
				shuffle_into_library(instance->targets[0].player, instance->targets[0].card);
			}
			shuffle_into_library(player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XU(2, 1), 0, &td, "TARGET_CREATURE");
}

int card_war_falcon(int player, int card, event_t event){

	if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( ! check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_SOLDIER) &&
			! check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_KNIGHT)
		  ){
			event_result = 1;
		}
	}

	return 0;
}

// Warclamp Mastiff --> vanilla

int card_watercourser(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 0, 0, 1, 0, 0, 0, 1, -1, 0, 0);
}

int card_wild_guess(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST && hand_count[player] > 1 ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		discard(player, 0, player);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_wits_end(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			new_discard_all(instance->targets[0].player, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_worldfire(int player, int card, event_t event){
	/*
	  Worldfire English |6|R|R|R
	  Sorcery
	  Exile all permanents. Exile all cards from all hands and graveyards. Each player's life total becomes 1.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_REMOVE);};);

		APNAP(p, {
					int count = count_graveyard(p)-1;
					while( count > -1 ){
							rfg_card_from_grave(p, count);
							count--;
					}
					count = active_cards_count[p]-1;
					while( count > -1 ){
							if( in_hand(p, count) )
								rfg_card_in_hand(p, count);
							count--;
					};
				};
		);

		APNAP(p, {set_life_total(p, 1);};);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_yeva_forcemage(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.allow_cancel = 0;
		td1.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td1) && comes_into_play(player, card, event) ){
			pick_target(&td1, "TARGET_CREATURE");
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2, 0, 0);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_yeva_natures_herald(int player, int card, event_t event)
{
  /* Yeva, Nature's Herald	|2|G|G
   * Legendary Creature - Elf Shaman 4/4
   * Flash
   * You may cast green creature cards as though they had flash. */

  check_legend_rule(player, card, event);

  if (flash(player, card, event))
	return 1;

  if (!IS_ACTIVATING(event))
	return 0;

  target_definition_t td;
  base_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = td.preferred_controller = player;
  td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
  td.zone = TARGET_ZONE_HAND;

  const char* prompt = (event == EVENT_ACTIVATE ? get_sleighted_color_text(player, card, "Select a %s creature card.", COLOR_GREEN) : NULL);
  return can_play_cards_as_though_they_had_flash(player, card, event, &td, prompt, 1);
}

int card_xathrid_gorgon(int player, int card, event_t event){

	/* Xathrid Gorgon	|5|B
	 * Creature - Gorgon 3/6
	 * Deathtouch
	 * |2|B, |T: Put a petrification counter on target creature. It gains defender and becomes a colorless artifact in addition to its other types.
				Its activated abilities can't be activated. */

	deathtouch(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_PETRIFICATION);	// does nothing, per rulings

			int leg = pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
											 0, 0, KEYWORD_DEFENDER, SP_KEYWORD_DOES_NOT_END_AT_EOT);
			if (leg != -1){
				get_card_instance(instance->parent_controller, leg)->token_status |= STATUS_INVISIBLE_FX;
			}

			if (!is_what(instance->targets[0].player, instance->targets[0].card, TYPE_ARTIFACT)){
				leg = turn_into_artifact(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0);
				if (leg != -1){
					get_card_instance(instance->parent_controller, leg)->token_status |= STATUS_INVISIBLE_FX;
				}
			}

			// The only effect left visible
			change_color(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, CHANGE_COLOR_SET|CHANGE_COLOR_NO_SLEIGHT);

			disable_all_activated_abilities(instance->targets[0].player, instance->targets[0].card, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XB(2, 1), 0, &td, "TARGET_CREATURE");
}

