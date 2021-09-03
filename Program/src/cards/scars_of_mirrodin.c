#include "manalink.h"

// ---- General functions

static int smiths(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		player == reason_for_trigger_controller && player == trigger_cause_controller ){

		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_ARTIFACT) ){
			trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					return 1;
			}
		}
	}
	return 0;
}

static int idols(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) &&
		player == reason_for_trigger_controller && player == trigger_cause_controller ){

		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_ARTIFACT) ){
			trig = 1;
		}

		if( trigger_cause_controller == player && trigger_cause == card ){
			trig = 0;
		}

		if( trig == 1 && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) &&
			check_battlefield_for_id(2, CARD_ID_TORPOR_ORB)
		  ){
			trig = 0;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					return 1;
			}
		}
	}
	return 0;
}

static int som_spellbombs(int player, int card, event_t event, int manacolor){

	if( graveyard_from_play(player, card, event) && has_mana(player, manacolor, 1) ){
		charge_mana(player, manacolor, 1);
		if( spell_fizzled != 1 ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

static int trigon(int player, int card, event_t event, color_t color, const char* dialog_prompt, target_definition_t* td, const char* target_prompt)
{
  /* Trigon of [Something]	|[cost]
   * Artifact
   * ~ enters the battlefield with three charge counters on it.
   * |C|C, |T: Put a charge counter on ~.
   * |2, |T, Remove a charge counter from ~: [Do something]. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, 3);

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !(CAN_TAP(player, card) && can_use_activated_abilities(player, card)))
		return 0;

	  enum
	  {
		CHOICE_ACTIVATE = 1,
		CHOICE_CHARGE,
	  } choice= DIALOG(player, card, event,
					   dialog_prompt,
							count_counters(player, card, COUNTER_CHARGE) && (!td || can_target(td)),
							2,
							DLG_MANA(MANACOST_X(2)),
					   "Add a charge counter",
							1,
							current_turn == 1-player && current_phase == PHASE_DISCARD ? 3 : 1,
							DLG_MANA(MANACOST_CLR(color, 2)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice ? 1 : 0;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_ACTIVATE:
			  if (td)
				{
				  get_card_instance(player, card)->number_of_targets = 0;
				  if (!pick_target(td, target_prompt))
					break;
				}
			  remove_counter(player, card, COUNTER_CHARGE);
			  tap_card(player, card);
			  break;

			case CHOICE_CHARGE:
			  tap_card(player, card);
			  break;
		  }
	  else	// EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_ACTIVATE:
			  return 2;

			case CHOICE_CHARGE:
			  add_counter(player, card, COUNTER_CHARGE);
			  break;
		  }
	}

  if (event == EVENT_SHOULD_AI_PLAY)
	ai_modifier += (player == AI ? 8 : -8) * count_counters(player, card, COUNTER_CHARGE);

  return 0;
}

int metalcraft(int player, int card){
	if( !is_humiliated(player, card) && count_permanents_by_type(player, TYPE_ARTIFACT) > 2 ){
		return 1;
	}
	return 0;
}

void proliferate(int player, int card)
{
  // You choose any number of permanents and/or players with counters on them, then give each another counter of a kind already there.

  if (IS_AI(player))
	{
	  if (poison_counters[1-player] > 0)
		++poison_counters[1-player];

	  int p, c;
	  counter_t counter_type;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_PERMANENT) && count_counters(p, c, -1) > 0
			  && (counter_type = choose_existing_counter_type(player, player, card, p, c, CECT_AI_CAN_CANCEL | CECT_AI_NO_DIALOG, -1, -1)) != COUNTER_invalid)
			add_counter(p, c, counter_type);
	}
  else if (DIALOG(player, card, EVENT_ACTIVATE,
				  DLG_NO_STORAGE, DLG_NO_CANCEL,
				  "Automatic", 1, 1,
				  "Manual", 1, 1) == 1)
	{
	  // Automatic mode

	  if (poison_counters[1-player] > 0)
		++poison_counters[1-player];

	  int p, c;
	  counter_t counter_type;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_PERMANENT) && count_counters(p, c, -1) > 0
			  && (counter_type = choose_existing_counter_type(player, player, card, p, c, CECT_AI_CAN_CANCEL | CECT_HUMAN_CAN_CANCEL | CECT_AUTOCHOOSE_BEST, -1, -1)) != COUNTER_invalid)
			add_counter(p, c, counter_type);
	}
  else
	{
	  // Manual mode

	  target_definition_t td;
	  base_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.allow_cancel = 2;	// Done button, no cancel

	  td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	  td.extra = 0;
	  SET_BYTE0(td.extra) = COUNTER_invalid;

	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	  if (poison_counters[0] <= 0 && poison_counters[1] <= 0)
		td.zone = TARGET_ZONE_IN_PLAY;	// no targettable players

	  int chosen_players[2] = {0, 0};

	  target_t tgt;

	  while (can_target(&td))
		{
		  load_text(0, "PROLIFERATE");
		  if (!select_target(player, card-1000, &td, text_lines[0], &tgt))
			break;

		  if (tgt.card != -1)
			{
			  counter_t counter_type = choose_existing_counter_type(player, player, card, tgt.player, tgt.card, CECT_HUMAN_CAN_CANCEL, -1, -1);
			  if (counter_type == COUNTER_invalid)
				cancel = 0;
			  else
				{
				  state_untargettable(tgt.player, tgt.card, 1);
				  add_counter(tgt.player, tgt.card, counter_type);
				}
			}
		  else if (poison_counters[tgt.player] > 0 && chosen_players[tgt.player] != 1)
			{
			  poison_counters[tgt.player]++;
			  chosen_players[tgt.player] = 1;
			  if (chosen_players[1 - tgt.player] || poison_counters[1 - tgt.player] <= 0)
				td.zone = TARGET_ZONE_IN_PLAY;	// no more targettable chosen_players
			}

		}

	  remove_untargettable_from_all();
	}
}

// ------- Cards

int card_wurmcoil_engine(int player, int card, event_t event){
	/* Wurmcoil Engine	|6
	 * Artifact Creature - Wurm 6/6
	 * Deathtouch, lifelink
	 * When ~ dies, put a 3/3 colorless Wurm artifact creature token with deathtouch and a 3/3 colorless Wurm artifact creature token with lifelink onto the battlefield. */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WURM, &token);
		token.pow = token.tou = 3;
		token.color_forced = COLOR_TEST_COLORLESS;
		token.action = TOKEN_ACTION_CONVERT_INTO_ARTIFACT;
		token.s_key_plus = SP_KEYWORD_DEATHTOUCH;
		generate_token(&token);
		token.s_key_plus = SP_KEYWORD_LIFELINK;
		generate_token(&token);
	}

	return 0;
}

int card_myr_battlesphere(int player, int card, event_t event)
{
  /* Myr Battlesphere	|7
   * Artifact Creature - Myr Construct 4/7
   * When ~ enters the battlefield, put four 1/1 colorless Myr artifact creature tokens onto the battlefield.
   * Whenever ~ attacks, you may tap X untapped Myr you control. If you do, ~ gets +X/+0 until end of turn and deals X damage to defending player. */

  if (comes_into_play(player, card, event))
	generate_tokens_by_id(player, card, CARD_ID_MYR, 4);

  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.required_subtype = SUBTYPE_MYR;
	  td.illegal_state = TARGET_STATE_TAPPED;
	  td.illegal_abilities = 0;
	  td.allow_cancel = 3;

	  char marked[2][151] = {{0}};
	  int num = mark_up_to_n_targets_noload(&td, "Select an untapped Myr you control.", -1, marked);
	  if (num > 0)
		{
		  int c;
		  for (c = 0; c < active_cards_count[player]; ++c)
			if (marked[player][c] && in_play(player, c))	// Tapping one of the previous Myr might've destroyed this one.
			  tap_card(player, c);

		  damage_player(1-current_turn, num, player, card);
		  if (in_play(player, card))	// Tapping one of the Myr might've destroyed the Battlesphere.  Stupid instantaneous Manalink triggers.
			pump_until_eot(player, card, player, card, num,0);
		}
	}

  return 0;
}

int card_ezuris_brigade(int player, int card, event_t event){
	if( in_play(player, card) && affect_me(player, card) && metalcraft(player, card) ){
		if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
			event_result+=4;
		}
		else if(event == EVENT_ABILITIES ){
			event_result |= KEYWORD_TRAMPLE;
		}
	}
	return 0;
}

int card_koth_of_the_hammer(int player, int card, event_t event){

	/* Koth of the Hammer	|2|R|R
	 * Planeswalker - Koth (3)
	 * +1: Untap target |H2Mountain. It becomes a 4/4 |Sred Elemental creature until end of turn. It's still a land.
	 * -2: Add |R to your mana pool for each |H2Mountain you control.
	 * -5: You get an emblem with "|H1Mountains you control have '|T: This land deals 1 damage to target creature or player.'" */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.required_subtype = get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN);
		td.preferred_controller = player;

		int priority_mana = 0;
		int priority_emblem = 0;

		if( event == EVENT_ACTIVATE ){
			int mountains = count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN));
			priority_mana = (4*mountains)+((count_counters(player, card, COUNTER_LOYALTY)*4)-8);
			priority_emblem = (5*mountains)+((count_counters(player, card, COUNTER_LOYALTY)*5)-25);
		}

		enum{
			CHOICE_ANIMATE = 1,
			CHOICE_MANA,
			CHOICE_EMBLEM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Animate a land", can_target(&td), 10, 1,
						"Produce mana", 1, priority_mana, -2,
						"Emblem", 1, priority_emblem, -5);

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
				case CHOICE_ANIMATE:
					if( select_target(player, card, &td, get_hacked_land_text(player, card, "Select target %d", SUBTYPE_MOUNTAIN), NULL) ){
						instance->number_of_targets = 1;
					}
					else{
						spell_fizzled = 1;
					}
					break;

				case CHOICE_MANA:
				case CHOICE_EMBLEM:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_ANIMATE:
				if( valid_target(&td) ){
					untap_card(instance->targets[0].player, instance->targets[0].card);
					land_animation2(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 4, 4, 0, 0, COLOR_TEST_RED, 0);
				}
				break;

			case CHOICE_MANA:
				produce_mana(player, COLOR_RED, count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN)));
				break;

			case CHOICE_EMBLEM:
				generate_token_by_id(player, card, CARD_ID_KOTHS_EMBLEM);
				break;
		}
	}

	return planeswalker(player, card, event, 3);
}

int card_koth_emblem(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.required_subtype = SUBTYPE_MOUNTAIN;
		td.preferred_controller = player;
		td.allowed_controller = player;
		td.illegal_state = TARGET_STATE_TAPPED;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE );
		td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td1.allow_cancel = 0;

		if(event == EVENT_CAN_ACTIVATE && affect_me(player, card)){
			if( target_available(player, card, &td) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				return can_target(&td1);
			}
		}
		else if( event == EVENT_ACTIVATE ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_LAND") ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
					if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
						instance->number_of_targets = 1;
					}
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				if( valid_target(&td1)  ){
					damage_creature_or_player(player, card, event, 1);
				}
		}

	return 0;
}

int card_venser_emblem(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = 2;
	td.preferred_controller = 1-player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if(trigger_condition == TRIGGER_SPELL_CAST && player == affected_card_controller
	   && player == reason_for_trigger_controller && player == trigger_cause_controller && card == affected_card )
	{
		card_instance_t *played = get_card_instance(trigger_cause_controller, trigger_cause);
		if( !(cards_data[ played->internal_card_id].type & TYPE_LAND) ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					pick_target(&td, "TARGET_PERMANENT");
					if( valid_target(&td) ){
					   kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
					}
			}
		}
	}

  return 0;
}

int card_venser_the_sojourner(int player, int card, event_t event){

	/* Venser, the Sojourner	|3|W|U
	 * Planeswalker - Venser (3)
	 * +2: Exile target permanent you own. Return it to the battlefield under your control at the beginning of the next end step.
	 * -1: Creatures are unblockable this turn.
	 * -8: You get an emblem with "Whenever you cast a spell, exile target permanent." */

	if (IS_ACTIVATING(event)){

		card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allowed_controller = player | TARGET_PLAYER_OWNER;
		td.preferred_controller = player | TARGET_PLAYER_OWNER;

		int priority_unb = 0;
		int priority_emblem = 0;

		if( event == EVENT_ACTIVATE ){
			priority_unb = (3*count_subtype(player, TYPE_CREATURE, -1))-((count_counters(player, card, COUNTER_LOYALTY)*4)-4);
			priority_emblem = 55+((count_counters(player, card, COUNTER_LOYALTY)*5)-40);
		}

		enum{
			CHOICE_BLINK = 1,
			CHOICE_UNBLOCKABLE,
			CHOICE_EMBLEM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Blink a permanent", can_target(&td), 10, 2,
						"Give unblockable to all", 1, priority_unb, -1,
						"Emblem", 1, priority_emblem, -8);

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
				case CHOICE_BLINK:
					new_pick_target(&td, "Select a permanent you own.", 0, 1 | GS_LITERAL_PROMPT);
					break;

				case CHOICE_UNBLOCKABLE:
				case CHOICE_EMBLEM:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_BLINK:
				if( valid_target(&td) ){
					set_legacy_image(player, cards_data[get_original_internal_card_id(instance->targets[0].player, instance->targets[0].card)].id,
									remove_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card));
				}
				break;

			case CHOICE_UNBLOCKABLE:
				creatures_cannot_block(instance->parent_controller, instance->parent_card, NULL, 1);
				break;

			case CHOICE_EMBLEM:
				generate_token_by_id(player, card, CARD_ID_VENSERS_EMBLEM );
				break;
		}
	}

	return planeswalker(player, card, event, 3);
}

static int is_land_or_token(int player, int card){
	if( is_what(player, card, TYPE_LAND) || is_token(player, card) ){
		return 1;
	}
	return 0;
}

static int count_land_or_token(int p){
	int result = 0;
	int count = active_cards_count[p]-1;
	while( count > -1 ){
			if( in_play(p, count) && is_what(p, count, TYPE_PERMANENT) && is_land_or_token(p, count) ){
				result++;
			}
			count--;
	}
	return result;
}

int card_elspeth_tirel(int player, int card, event_t event){

	/* Elspeth Tirel	|3|W|W
	 * Planeswalker - Elspeth (4)
	 * +2: You gain 1 life for each creature you control.
	 * -2: Put three 1/1 |Swhite Soldier creature tokens onto the battlefield.
	 * -5: Destroy all other permanents except for lands and tokens. */

	card_instance_t *instance = get_card_instance(player, card);

	if (IS_ACTIVATING(event)){

		int priorities[3] = {0, 0, 0};

		if( event == EVENT_ACTIVATE ){
			priorities[0] = count_subtype(player, TYPE_CREATURE, -1)*3;
			if( priorities[0] ){
				priorities[0]+=(60-(life[player]*3));
			}
			priorities[1] = ((count_counters(player, card, COUNTER_LOYALTY)*4)-8)-(count_subtype(player, TYPE_CREATURE, -1)*5);
			priorities[2] = ((count_counters(player, card, COUNTER_LOYALTY)*4)-20);
			priorities[2] += (5*count_land_or_token(player));
			priorities[2] -= (5*count_land_or_token(1-player));
		}

		enum{
			CHOICE_LIFE = 1,
			CHOICE_SOLDIERS,
			CHOICE_KILL_NONTOKENS
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Gain life", 1, priorities[0], 2,
						"3 Soldiers", 1, priorities[1], -2,
						"Nontokens must die !", 1, priorities[2], -5);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
			switch (choice)
			{
				case CHOICE_LIFE:
					gain_life(player, count_permanents_by_type(player, TYPE_CREATURE));
					break;

				case CHOICE_SOLDIERS:
					generate_tokens_by_id(player, card, CARD_ID_SOLDIER, 3);
					break;

				case CHOICE_KILL_NONTOKENS:
				{
					APNAP(p, {
								int count = active_cards_count[p]-1;
								while( count > -1 ){
										if( in_play(p, count) && is_what(p, count, TYPE_PERMANENT) &&
											!(p == instance->parent_controller && count == instance->parent_card) &&
											! is_land_or_token(p, count)
										  ){
											kill_card(p, count, KILL_DESTROY);
										}
										count--;
								}
							};
						);
				}
				break;
			}
		}
	}

	return planeswalker(player, card, event, 4);
}


int card_tempered_steel(int player, int card, event_t event){

  if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
	 if( is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		if(affected_card_controller == player ){
		   event_result += 2;
		}
	 }
  }

  return global_enchantment(player, card, event);
}


int card_myr_galvanizer(int player, int card, event_t event){

	boost_creature_type(player, card, event, SUBTYPE_MYR, 1, 1, 0, BCT_CONTROLLER_ONLY);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_MYR;
		this_test.not_me = 1;
		new_manipulate_all(player, instance->parent_card, player, &this_test, ACT_UNTAP);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_kuldotha_rebirth(int player, int card, event_t event){
	/* Kuldotha Rebirth	|R
	 * Sorcery
	 * As an additional cost to cast ~, sacrifice an artifact.
	 * Put three 1/1 |Sred Goblin creature tokens onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;
	td.illegal_abilities = 0;

	if(event == EVENT_CAN_CAST && can_target(&td) && affect_me(player, card) ){
			return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_ARTIFACT") ){
			   kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
			   instance->info_slot=66;
		   }
	}
	else if(event == EVENT_RESOLVE_SPELL && instance->info_slot==66 && affect_me(player, card) ){
			generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 3);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_spikeshot_elder(int player, int card, event_t event){

	/* Spikeshot Elder	|R
	 * Creature - Goblin Shaman 1/1
	 * |1|R|R: ~ deals damage equal to its power to target creature or player. */

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			damage_target0(player, card, get_power(instance->parent_controller, instance->parent_card));
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 2, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_argentum_armor(int player, int card, event_t event)
{
  // Whenever equipped creature attacks, destroy target permanent.
  card_instance_t* instance = get_card_instance(player, card);
  if (declare_attackers_trigger(player, card, event, 0, instance->targets[8].player, instance->targets[8].card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.allow_cancel = 0;

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_PERMANENT"))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

  // Equipped creature gets +6/+6.
  // Equip |6
  return vanilla_equipment(player, card, event, 6, 6,6, 0,0);
}

int card_sword_of_body_and_mind(int player, int card, event_t event)\
{
  /* Sword of Body and Mind	|3
   * Artifact - Equipment
   * Equipped creature gets +2/+2 and has protection from |Sgreen and from |Sblue.
   * Whenever equipped creature deals combat damage to a player, you put a 2/2 |Sgreen Wolf creature token onto the battlefield and that player puts the top ten
   * cards of his or her library into his or her graveyard.
   * Equip |2 */

  if (event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card))
		{
		  if (event == EVENT_ABILITIES)
			event_result |= get_sleighted_protection(player, card, KEYWORD_PROT_GREEN | KEYWORD_PROT_BLUE);
		  else
			event_result += 2;
		}
	}

  if (equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRACE_DAMAGED_PLAYERS))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
	  instance->targets[1].player = 0;
	  int p;
	  for (p = 0; p <= 1; ++p)
		for (; times_damaged[p] > 0; --times_damaged[p])
		  {
			generate_token_by_id(player, card, CARD_ID_WOLF);
			mill(p, 10);
		  }
	}

  return basic_equipment(player, card, event, 2);
}

int card_glint_hawk(int player, int card, event_t event){

  card_instance_t *instance = get_card_instance( player, card );

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_ARTIFACT);
  td.allowed_controller = player;
  td.preferred_controller = player;
  td.illegal_abilities = 0;

  if( comes_into_play(player, card, event) ){
	 if( can_target(&td)  && pick_target(&td, "TARGET_ARTIFACT") ){
		bounce_permanent(instance->targets[0].player, instance->targets[0].card);
	 }
	 else{
		  kill_card(player, card, KILL_SACRIFICE);
	 }
  }

	return 0;
}


int card_genesis_wave(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_CAN_CAST){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->info_slot = x_value;
	}
	else if(event == EVENT_RESOLVE_SPELL){
			int max = instance->info_slot;
			int *deck = deck_ptr[player];
			int i;
			int to_mill = 0;
			for(i=max-1; i > -1; i--){
				int cc = get_cmc_by_id( cards_data[deck[i]].id );
				if( (cards_data[deck[i]].type & TYPE_PERMANENT) && cc <= max){
				   int card_added = add_card_to_hand(player, deck[i]);
				   remove_card_from_deck(player, i);
				   put_into_play(player, card_added);
				}
				else{
					 to_mill++;
				}
			}
			if( to_mill > 0 ){
			   mill(player, to_mill);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ratchet_bomb(int player, int card, event_t event)
{
  /* Ratchet Bomb	|2
   * Artifact
   * |T: Put a charge counter on ~.
   * |T, Sacrifice ~: Destroy each nonland permanent with converted mana cost equal to the number of charge counters on ~. */

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !(CAN_TAP(player, card) && CAN_ACTIVATE0(player, card)))
		return 0;

	  enum
	  {
		CHOICE_CHARGE = 1,
		CHOICE_DETONATE
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Add charge counter", 1, 1,
						"Detonate", can_sacrifice_this_as_cost(player, card), 1);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_CHARGE:
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(0)))
				tap_card(player, card);
			  break;

			case CHOICE_DETONATE:
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(0)))
				{
				  tap_card(player, card);
				  kill_card(player, card, KILL_SACRIFICE);
				}
			  break;
		  }
	  else	// EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_CHARGE:
			  add_counter(player, card, COUNTER_CHARGE);
			  break;

			case CHOICE_DETONATE:
			  ;test_definition_t test;
			  default_test_definition(&test, TYPE_LAND);
			  test.type_flag = DOESNT_MATCH;
			  test.cmc = count_counters(player, card, COUNTER_CHARGE);
			  new_manipulate_all(player, card, ANYBODY, &test, KILL_DESTROY);
			  break;
		  }
	}

  if (event == EVENT_SHOULD_AI_PLAY)
	{
	  int most_common = get_most_common_cmc_nonland(1-player);
	  int counters = count_counters(player, card, COUNTER_CHARGE);
	  /* Moderately high modifier to increase towards most common.  If at most common or higher, and the AI doesn't want to sacrifice (perhaps it has better
	   * permanents at that cmc), then tiny modifier for more counters so it increases until it finds a cmc it's willing to sacrifice at. */
	  ai_modifier += (player == AI ? 1 : -1) * (counters <= most_common ? 32 : 4) * counters;
	}

	return 0;
}

int card_ezuri_renegade_leader(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_ELF;
	td.required_state = TARGET_STATE_DESTROYED;
	td.special = TARGET_SPECIAL_NOT_ME;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_REGENERATION | GAA_CAN_TARGET, MANACOST_G(1), 0, &td, NULL) ){
			return 99;
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_XG(2, 3), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE  ){
		instance->number_of_targets = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE , GAA_REGENERATION | GAA_CAN_TARGET, MANACOST_G(1), 0, &td, NULL) ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_G(1)) ){
				if( new_pick_target(&td, "Select target Elf to regenerate", 0, 1 | GS_LITERAL_PROMPT) ){
					instance->targets[1].player = 66;
				}
			}
		}
		else{
			if( charge_mana_for_activated_ability(player, card, MANACOST_XG(2, 3)) ){
				instance->targets[1].player = 67;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player == 66 && valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->targets[1].player == 67 ){
			pump_subtype_until_eot(instance->parent_controller, instance->parent_card, player, SUBTYPE_ELF, 3, 3, KEYWORD_TRAMPLE, 0);
		}
	}

	return 0;
}

int card_skinrender(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		card_instance_t* instance = get_card_instance( player, card );
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 3);
		}
	}

	return 0;
}

int card_moltentail_masticore(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	char msg[100] = "Select a creature card to exile.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	card_instance_t *instance = get_card_instance(player, card);

	// upkeep cost
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( hand_count[player] > 0 ){
			int choice = do_dialog(player, player, card, player, card, " Pay upkeep\n Pass", 0);
			if( choice == 0 ){
				discard(player, 0, player);
				kill--;
			}
		}
		if( kill > 0 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	// activated ability
	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( can_regenerate(player, card) && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
			return 0x63;
		}
		else if( new_special_count_grave(player, &this_test) > 0 && has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) && can_target(&td) ){
				return 1;
		}
	}

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		if( can_regenerate(player, card) ){
			if( charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
				instance->targets[1].player = 66;
			}
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MIN_VALUE, -1, &this_test);
				if( selected != -1 && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					rfg_card_from_grave(player, selected);
					instance->targets[1].player = 67;
					instance->number_of_targets = 1;
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[1].player == 66 && can_regenerate(player, instance->parent_card) ){
				regenerate_target(player, instance->parent_card);
			}
			if( instance->targets[1].player == 67 && valid_target(&td) ){
				damage_creature_or_player(player, card, event, 4);
			}
	}

	return 0;
}

int card_skithiryx_blight_dragon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( can_regenerate(player, card) && has_mana_for_activated_ability(player, card, 0, 2, 0, 0, 0, 0) ){
			return 0x63;
		}
		else if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) ){
				return 1;
		}
	}

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		if( can_regenerate(player, card) && has_mana_for_activated_ability(player, card, 0, 2, 0, 0, 0, 0) ){
			if( charge_mana_for_activated_ability(player, card, 0, 2, 0, 0, 0, 0) ){
				instance->targets[1].player = 66;
			}
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) ){
				instance->targets[1].player = 67;
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player == 66 && can_regenerate(player, instance->parent_card) ){
				regenerate_target(player, instance->parent_card);
		}
		if( instance->targets[1].player == 67 ){
				pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
	}

	return 0;
}


int card_mimic_vat(int player, int card, event_t event){

	/* Mimic Vat	|3
	 * Artifact
	 * Imprint - Whenever a nontoken creature dies, you may exile that card. If you do, return each other card exiled with ~ to its owner's graveyard.
	 * |3, |T: Put a token onto the battlefield that's a copy of the exiled card. It gains haste. Exile it at the beginning of the next end step. */

  card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_token(affected_card_controller, affected_card) ){
		   card_instance_t *dead = get_card_instance(affected_card_controller, affected_card);
		   if( dead->kill_code != KILL_REMOVE ){
				if( instance->targets[11].player < 1 ){
					instance->targets[11].player = 1;
				}
				int position = instance->targets[11].player;
				if (position < 11){
					instance->targets[position].player = get_owner(affected_card_controller, affected_card);
					instance->targets[position].card = get_original_id(affected_card_controller, affected_card);
					instance->targets[11].player++;
				}
		   }
		}
	}

	if( resolve_graveyard_trigger(player, card, event) == 1 && instance->targets[11].player > 1){
		int i;
		char buffer[600];
		int pos = scnprintf(buffer, 600, " Pass\n");
		for (i = 1; i < instance->targets[11].player; i++){
			int id = instance->targets[i].card;
			if( id != -1 ){
				card_ptr_t* c_me = cards_ptr[ instance->targets[i].card ];
				pos += scnprintf(buffer+pos, 600-pos, " Put %s in the vat\n", c_me->name);
			}
		}

	   int choice = do_dialog(player, player, card, -1, -1, buffer, 1);
	   if( choice > 0 ){
			int k = count_graveyard(instance->targets[choice].player)-1;
			const int *grave = get_grave(instance->targets[choice].player);
			for (; k >= 0; --k){
				if( cards_data[grave[k]].id == instance->targets[choice].card ){
					rfg_card_from_grave(instance->targets[choice].player, k);
					if( instance->targets[0].player != -1 ){
						int position = find_iid_in_rfg(instance->targets[0].player, get_internal_card_id_from_csv_id(instance->targets[0].card));
						if (position != -1){
							from_exile_to_graveyard(instance->targets[0].player, position);
						}
					}

					if( instance->targets[0].card != instance->targets[choice].card){
						if( instance->info_slot > 0){
							kill_card(player, instance->info_slot, KILL_REMOVE);
						}
						instance->info_slot = create_card_name_legacy(player, card, instance->targets[choice].card );
					}
					instance->targets[0] = instance->targets[choice];	// struct copy
					break;
				}
			}

		}
		instance->targets[11].player = 1;
		for (i = 1; i < 11; i++){
			instance->targets[i].card = -1;
		}
	}

	if( event == EVENT_CAN_ACTIVATE && instance->targets[0].player != -1 ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, 3, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_ACTIVATE){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, 3, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	if( event == EVENT_RESOLVE_ACTIVATION){
		token_generation_t token;
		default_token_definition(instance->parent_controller, instance->parent_card, instance->targets[0].card, &token);
		token.legacy = 1;
		token.special_code_for_legacy = &haste_and_remove_eot;
		generate_token(&token);
	}

	return 0;
}

static int generic_artifact_with_proliferate(int player, int card, event_t event, int manacost, int prolly){

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for (i = 0; i < prolly; ++i){
			proliferate(player, card);
		}
	}

	if (event == EVENT_ACTIVATE){
		get_card_instance(player, card)->number_of_targets = 0;
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, manacost, 0, 0, 0, 0, 0, 0, 0, 0);
}


int card_contagion_clasp(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
			card_instance_t* instance = get_card_instance(player, card);
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
		}
	}

  return generic_artifact_with_proliferate(player, card, event, 4, 1);
}

int card_contagion_engine(int player, int card, event_t event){

	/* Contagion Engine	|6
	 * Artifact
	 * When ~ enters the battlefield, put a -1/-1 counter on each creature target player controls.
	 * |4, |T: Proliferate, then proliferate again. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allow_cancel = 0;
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && count_permanents_by_type(1-player, TYPE_CREATURE) < 1 ){
			ai_modifier -= 64;
		}
	}

	if (comes_into_play(player, card, event) && can_target(&td)){
		int target = -1;
		if (duh_mode(player)){
			instance->targets[0].player = 1 - player;
			instance->targets[0].card = -1;
			if (valid_target(&td)){
				target = 1 - player;
			}
		}
		if (target == -1	// duh mode off, or opponent isn't targetable
			&& can_target(&td) && pick_target(&td, "TARGET_PLAYER")){
			target = instance->targets[0].player;
		}
		if (target != -1){
			// add one -1/-1 counter on all opponent's creature
			manipulate_type(player, card, target, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_M1_M1, 1));
		}
	}

	return generic_artifact_with_proliferate(player, card, event, 4, 2);
}


int card_thrummingbird(int player, int card, event_t event){

	if (has_combat_damage_been_inflicted_to_a_player(player, card, event)){
		proliferate(player, card);
	}

	return 0;
}

int card_lux_cannon(int player, int card, event_t event){

	/* Lux Cannon	|4
	 * Artifact
	 * |T: Put a charge counter on ~.
	 * |T, Remove three charge counters from ~: Destroy target permanent. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(0), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( count_counters(player, card, COUNTER_CHARGE) >= 3 ){
			choice = do_dialog(player, player, card, -1, -1, " Add a charge counter\n Kill a permanent\n Cancel", 1);
		}
		if( choice == 2  ){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				if( choice == 0  ){
					instance->info_slot = 66;
					tap_card(player, card);
				}
				if( choice == 1  ){
					if( pick_target(&td, "TARGET_PERMANENT") ){
						instance->number_of_targets = 0;
						instance->info_slot = 67;
						remove_counters(player, card, COUNTER_CHARGE, 3);
						tap_card(player, card);
					}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			add_counter(player, card, COUNTER_CHARGE);
		}

		if( instance->info_slot == 67 && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	if (event == EVENT_SHOULD_AI_PLAY){
		int count = count_counters(player, card, COUNTER_CHARGE);
		ai_modifier += (player == AI ? 1 : -1) * (count + (12 * (count / 3)));
	}

	return 0;
}

int card_etched_champion(int player, int card, event_t event){

  if( event == EVENT_ABILITIES ){
	 if( ! affect_me(player, card) ){ return 0; }
	 if( metalcraft(player, card) ){
		event_result |= KEYWORD_PROT_BLACK;
		event_result |= KEYWORD_PROT_BLUE;
		event_result |= KEYWORD_PROT_GREEN;
		event_result |= KEYWORD_PROT_RED;
		event_result |= KEYWORD_PROT_WHITE;
	 }
  }

  return 0;
}

int card_shape_anew(int player, int card, event_t event){
  card_instance_t *instance = get_card_instance( player, card );
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_ARTIFACT );
  td.allowed_controller = 2;
  td.preferred_controller = player;

  if(event == EVENT_CAN_CAST){
	 return 1;
  }

  else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		  pick_target(&td, "TARGET_ARTIFACT");
  }

  else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card)){
		  if( valid_target(&td) ){
			 metamorphosis(instance->targets[0].player, instance->targets[0].card, TYPE_ARTIFACT, KILL_SACRIFICE);
		  }
		  kill_card(player, card, KILL_DESTROY);
  }

 return 0;
}

int card_hand_of_the_praetors(int player, int card, event_t event){

  if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
	 if( affect_me(player, card) ){ return 0; }
	 if( affected_card_controller == player &&
		get_abilities(affected_card_controller, affected_card, EVENT_ABILITIES, -1) & KEYWORD_INFECT ){
		event_result++;
	}
  }


	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){
	   if( trigger_cause_controller == player && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) &&
		   (get_abilities(trigger_cause_controller, trigger_cause, EVENT_ABILITIES, -1) & KEYWORD_INFECT ) ){
		  if(event == EVENT_TRIGGER){
			 event_result |= RESOLVE_TRIGGER_MANDATORY;
		  }
		  else if(event == EVENT_RESOLVE_TRIGGER){
				  poison_counters[1-player]++;
		  }
	   }
	}

 return 0;
}

int card_ichorclaw_myr(int player, int card, event_t event){

  if( current_turn == player && event == EVENT_DECLARE_BLOCKERS && ! is_unblocked(player, card) &&
	  (get_card_instance(player, card)->state & STATE_ATTACKING) ){
	  pump_until_eot(player, card, player, card, 2, 2);
  }

 return 0;
}

int card_vector_asp(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 0, 1, 0, 0, 0, 0, 0, 0, KEYWORD_INFECT, 0);
}

int card_tainted_strike(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE );
  td.allowed_controller = 2;
  td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_CAN_CAST && can_target(&td) ){
	   return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( spell_fizzled != 1 ){
			   pick_target(&td, "TARGET_CREATURE");
			}
	}
	else if(event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
			   pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									  1, 0, KEYWORD_INFECT, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}

 return 0;
}

int card_grafted_exoskeleton(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[8].player != -1 && instance->targets[8].card != -1 &&
		in_play(instance->targets[8].player, instance->targets[8].card) ){

		if( event == EVENT_ABILITIES && affect_me(instance->targets[8].player, instance->targets[8].card) ){
			event_result |= KEYWORD_INFECT;
		}

		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) &&
			affect_me(instance->targets[8].player, instance->targets[8].card) ){
			event_result += 2;
		}
	}

  return basic_equipment(player, card, event, 2);	// graft effect hard-coded to CARD_ID_GRAFTED_EXOSKELETON in unattach()
}

int card_semblance_anvil(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ANY );
		td.zone = TARGET_ZONE_HAND;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;

		if( player == AI ){
			td.illegal_type = TYPE_LAND;
		}
		if( pick_target(&td, "TARGET_CARD") ){
			instance->info_slot = get_type(player, instance->targets[0].card);
			create_card_name_legacy(player, card, get_id(player, instance->targets[0].card));
			kill_card(player, instance->targets[0].card, KILL_DESTROY);
			remove_card_from_grave(player, count_graveyard(player)-1);
		}
		else{
			 instance->info_slot = 0;
		}
	}

	if (event == EVENT_MODIFY_COST_GLOBAL){
		if (instance->info_slot > 0
			&& affected_card_controller == player
			&& is_what(affected_card_controller, affected_card, instance->info_slot)){
			COST_COLORLESS-=2;
		}
	}

  return 0;
}

int card_throne_of_geth(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	else if( event == EVENT_ACTIVATE){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
				sacrifice(player, card, player, 0, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
			  ){
				tap_card(player, card);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION){
		proliferate(player, card);
	}

  return 0;
}

int card_kuldotha_forgemaster(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_sacrifice_as_cost(player, 3, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	else if( event == EVENT_ACTIVATE){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
				sacrifice(player, card, player, 0, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
			  ){
				impose_sacrifice(player, card, player, 2, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

  return 0;
}

int card_tumble_magnet(int player, int card, event_t event){

	/* Tumble Magnet	|3
	 * Artifact
	 * ~ enters the battlefield with three charge counters on it.
	 * |T, Remove a charge counter from ~: Tap target artifact or creature. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, 3);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_CHARGE), &td, "TARGET_PERMANENT");
}

int card_necrotic_ooze(int player, int card, event_t event ){

	card_instance_t *instance= get_card_instance(player, card);

	int i, p, count;
	int activable[2][500];
	if( event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) ){
		for (p = 0; p <= 1; ++p){
			count = count_graveyard(p);
			if( count > 0 ){
				const int *graveyard = get_grave(p);
				for(i=0;i<count;i++){
					activable[p][i+1] = -1;
					card_data_t* card_d = &cards_data[ graveyard[i] ];
					if( (card_d->type & TYPE_CREATURE) && card_d->id != CARD_ID_NECROTIC_OOZE ){
						int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
						if( ptFunction(player, card, EVENT_CAN_ACTIVATE ) ){
							return 1;
						}
					}
				}
			}
		}
	}
	else if( event == EVENT_ACTIVATE ){
			int act_count[2] = { 0, 0 };
			for (p = 0; p <= 1; ++p){
				count = count_graveyard(p);
				const int *graveyard = get_grave(p);
				for(i=0;i<count;i++){
					activable[p][i+1] = -1;
					card_data_t* card_d = &cards_data[ graveyard[i] ];
					if( (card_d->type & TYPE_CREATURE) && card_d->id != CARD_ID_NECROTIC_OOZE){
						int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
						if( ptFunction(player, card, EVENT_CAN_ACTIVATE ) ){
							act_count[p]++;
							activable[p][act_count[p]] = graveyard[i];
						}
					}
				}
			}

			char buffer[500];
			int pos = scnprintf(buffer, 500, " Do nothing\n" );
			for (p = 0; p <= 1; ++p){
				for(i=0;i<act_count[p];i++){
					card_ptr_t* c = cards_ptr[ cards_data[activable[p][i+1]].id ];
					pos += scnprintf(buffer + pos, 500-pos, " %s\n", c->name );
				}
			}
			int choice = 1;
			if( act_count[0] + act_count[1] > 1 ){
				choice = do_dialog(player, player, card, -1, -1, buffer, 1);
			}
			if( choice != 0 && spell_fizzled != 1 ){
				if (choice > act_count[0]){
					choice -= act_count[0];
					p = 1;
				} else {
					p = 0;
				}
				// pay the costs for the selected card
				int (*ptFunction)(int, int, event_t) = (void*)cards_data[ activable[p][choice] ].code_pointer;
				ptFunction(player, card, EVENT_ACTIVATE);
				if( spell_fizzled != 1 ){
					instance->targets[11].card = cards_data[ activable[p][choice] ].id;
					instance->targets[11].player = 1;
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int int_id = get_internal_card_id_from_csv_id( instance->targets[11].card );
			if( instance->targets[11].player > 0 ){
				int (*ptFunction)(int, int, event_t) = (void*)cards_data[int_id].code_pointer;
				ptFunction(player, card, EVENT_RESOLVE_ACTIVATION);
			}
	}

	return 0;
}

int card_generic_som_tapland(int player, int card, event_t event)
{
  /* Blackcleave Cliffs	""
   * Land
   * ~ enters the battlefield tapped unless you control two or fewer other lands.
   * |T: Add |B or |R to your mana pool. */

  /* Copperline Gorge	""
   * Land
   * ~ enters the battlefield tapped unless you control two or fewer other lands.
   * |T: Add |R or |G to your mana pool. */

  /* Darkslick Shores	""
   * Land
   * ~ enters the battlefield tapped unless you control two or fewer other lands.
   * |T: Add |U or |B to your mana pool. */

  /* Razorverge Thicket	""
   * Land
   * ~ enters the battlefield tapped unless you control two or fewer other lands.
   * |T: Add |G or |W to your mana pool. */

  /* Seachrome Coast	""
   * Land
   * ~ enters the battlefield tapped unless you control two or fewer other lands.
   * |T: Add |W or |U to your mana pool. */

  // inlines comes_into_play_tapped()
  if ((event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card)
	  && !check_special_flags2(player, card, SF2_FACE_DOWN_DUE_TO_MANIFEST))
	{
	  int num_needed = 2;
	  if (in_play(player, card))	// will be false for EVENT_CAST_SPELL, true for EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE
		num_needed = 3;

	  if (basiclandtypes_controlled[player][COLOR_ANY] > num_needed)
		get_card_instance(player, card)->state |= STATE_TAPPED;	// avoid sending event
	}

  return mana_producer(player, card, event);
}

int card_nim_deathmantle(int player, int card, event_t event){

	/* Nim Deathmantle	|2
	 * Artifact - Equipment
	 * Equipped creature gets +2/+2, has intimidate, and is a |Sblack Zombie.
	 * Whenever a nontoken creature is put into your graveyard from the battlefield, you may pay |4. If you do, return that card to the battlefield and attach ~
	 * to it.
	 * Equip |4 */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY && has_mana(player, COLOR_COLORLESS, 4) &&
		in_play(affected_card_controller, affected_card) && affected_card_controller == player
	 ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			! is_token(affected_card_controller, affected_card)
		 ){
			card_instance_t *dead = get_card_instance(affected_card_controller, affected_card);
			if( dead->kill_code > 0 && dead->kill_code != KILL_REMOVE ){
				if( instance->targets[11].player < 2 ){
					instance->targets[11].player = 2;
				}
				int pos = instance->targets[11].player;
				if( pos < 10 ){
					instance->targets[pos].card = get_id(affected_card_controller, affected_card);
					instance->targets[11].player++;
				}
			}
		}
	}

	if( is_equipping(player, card) ){
		if( event == EVENT_SET_COLOR && affect_me(instance->targets[8].player, instance->targets[8].card) ){
			event_result = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		}
	}

	if( instance->targets[11].player > 2 && has_mana(player, COLOR_COLORLESS, 4) && resolve_graveyard_trigger(player, card, event) == 1 ){
		int i;
		char buffer[600];
		int pos = scnprintf(buffer, 600, " Pass\n");
		for(i=2; i<instance->targets[11].player; i++){
			int creature_id = instance->targets[i].card;
			if( creature_id > -1 ){
				card_ptr_t* c_me = cards_ptr[ creature_id ];
				pos += scnprintf(buffer+pos, 600-pos, " Reanimate %s\n", c_me->name);
			}
		}

		instance->targets[11].player = 2;

		int choice = do_dialog(player, player, card, -1, -1, buffer, 1);
		if( choice > 0 ){
			charge_mana(player, COLOR_COLORLESS, 4);
			if( spell_fizzled != 1 ){
				int zombie = seek_grave_for_id_to_reanimate(player, card, player, instance->targets[1+choice].card, REANIMATE_DEFAULT);
				if( zombie > -1 ){
					equip_target_creature(player, card, player, zombie);
				}
			}
		}
	}
	if( leaves_play(player, card, event) ){
		if( instance->targets[8].player != -1 && instance->targets[8].card != -1 ){
			if( in_play(instance->targets[8].player, instance->targets[8].card) ){
				reset_subtypes(instance->targets[8].player, instance->targets[8].card, 1);
			}
		}
	}

	return vanilla_equipment(player, card, event, 4, 2, 2, 0, SP_KEYWORD_INTIMIDATE);
}

int card_perilous_myr(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_target_creature_or_player(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
	}

	return 0;
}

int card_abuna_acolyte(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		int minus = 1;
		if( target->damage_target_card != -1 ){
			if( in_play(target->damage_target_player, target->damage_target_card) &&
				is_what(target->damage_target_player, target->damage_target_card, TYPE_CREATURE) &&
				is_what(target->damage_target_player, target->damage_target_card, TYPE_ARTIFACT)
			  ){
				minus = 2;
			}
		}
		if( target->info_slot <= minus ){
			target->info_slot = 0;
		}
		else{
			target->info_slot-=minus;
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_DAMAGE_PREVENTION, 0, 0, 0, 0, 0, 0, 0,
									&td, "TARGET_DAMAGE");
}

int card_accorder_shield(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 3, 0, 3, 0, SP_KEYWORD_VIGILANCE);
}

int card_acid_web_spider(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.required_subtype = SUBTYPE_EQUIPMENT;
		card_instance_t *instance = get_card_instance(player, card);
		if( can_target(&td) && pick_target(&td, "TARGET_ARTIFACT")  ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_arc_trail(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	if( player == AI ){
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	if( player == AI ){
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;
	}

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_CAN_CAST && target_available(player, card, &td)+target_available(player, card, &td1) > 1){
	   return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int players[2] = {0,0};
			if( player != AI ){
				if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
					instance->number_of_targets = 1;
					if( instance->targets[0].card == -1 ){
						players[instance->targets[0].player] = 1;
					}
					instance->targets[1].player = instance->targets[0].player;
					instance->targets[1].card = instance->targets[0].card;
					if( instance->targets[1].card != -1 ){
						state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
					}
				}
				td1.allow_cancel = 0;
				int stop = 0;
				while( stop == 0 ){
						pick_target(&td1, "TARGET_CREATURE_OR_PLAYER");
						if( instance->targets[0].card == -1 ){
							if( players[instance->targets[0].player] != 1 ){
								stop = 1;
							}
						}
						else{
							stop = 1;
						}
				}
			}
			else{
				 instance->info_slot = 2;
				 pick_target(&td1, "TARGET_CREATURE_OR_PLAYER");
				 instance->number_of_targets = 1;
				 if( instance->targets[0].card == -1 ){
					 players[instance->targets[0].player] = 1;
				 }
				 instance->targets[1].player = instance->targets[0].player;
				 instance->targets[1].card = instance->targets[0].card;
				 if( instance->targets[1].card != -1 ){
					 state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
				 }

				 instance->info_slot = 1;
				 if( can_target(&td) ){
					pick_target(&td, "TARGET_CREATURE");
				 }
				 else{
					  pick_target(&td1, "TARGET_CREATURE_OR_PLAYER");
					  if( instance->targets[0].card == -1 ){
						  if( players[instance->targets[0].player] == 1 ){
							  instance->targets[0].player = 1-instance->targets[0].player;
						  }
					  }
				}
			}
			if( instance->targets[1].card != -1 ){
				state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
			}

	}

	else if(event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td1) ){
				damage_creature_or_player(player, card, event, 1);
			}
			instance->targets[0].player = instance->targets[1].player;
			instance->targets[0].card = instance->targets[1].card;
			if( valid_target(&td1) ){
				damage_creature_or_player(player, card, event, 2);
			}
			kill_card(player, card, KILL_DESTROY);
	}

 return 0;
}

int card_argent_sphynx(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && metalcraft(player, card) ){
		return generic_activated_ability(player, card, event, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_ACTIVATE){
		return generic_activated_ability(player, card, event, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		remove_until_eot(player, instance->parent_card, player, instance->parent_card);
	}

	return 0;
}

int card_asceticism(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance= get_card_instance(player, card);

	// Creatures you control have hexproof.
	boost_subtype(player, card, event, -1, 0,0, 0,SP_KEYWORD_HEXPROOF, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

	// |1|G: Regenerate target creature.
	if( land_can_be_played & LCBP_REGENERATION ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  &&
			has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) && can_target(&td)
		  ){
			return 0x63;
		}
		else if( event == EVENT_ACTIVATE ){
				charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0);
				if( spell_fizzled != 1 && pick_creature_to_regen(td) ){
					instance->number_of_targets = 1;
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
					regenerate_target( instance->targets[0].player, instance->targets[0].card );
				}
		}
	}

	return global_enchantment(player, card, event);
}

int card_assault_strobe(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_CAN_CAST && can_target(&td) ){
	   return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if(event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
			   pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									  0, 0, KEYWORD_DOUBLE_STRIKE, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}

 return 0;
}

int card_auriok_edgewright(int player, int card, event_t event){

	if( metalcraft(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_DOUBLE_STRIKE);
	}

	return 0;
}

int card_auriok_replica(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		prevent_damage_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 1, 0, &td, "TARGET_DAMAGE");
}

int card_auriok_sunchaser(int player, int card, event_t event){

	if( metalcraft(player, card) ){
		modify_pt_and_abilities(player, card, event, 2, 2, KEYWORD_FLYING);
	}

	return 0;
}

int card_barbed_battlegear(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 2, 4, -1, 0, 0);
}

int card_barrage_ogre(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  &&
		! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	else if( event == EVENT_ACTIVATE){
			 if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
				controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, 0)
			  ){
				if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
					instance->number_of_targets = 1;
					tap_card(player, card);
				}
			}
			else{
				 spell_fizzled = 1;
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION){
			 if( valid_target(&td1) ){
				 damage_target0(player, card, 2);
			 }
	}

  return 0;
}

int card_bellowing_tanglewurm(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES
	  && affected_card_controller == player
	  && (affected_card == card
		  || (is_what(affected_card_controller, affected_card, TYPE_CREATURE)
			  && (get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_GREEN))))
	  && !is_humiliated(player, card))
	intimidate(affected_card_controller, affected_card, event);

	return 0;
}

int card_blade_tribe_berserkers(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && metalcraft(player, card) ){
		pump_ability_until_eot(player, card, player, card, 3, 3, 0, SP_KEYWORD_HASTE);
	}

	return 0;
}

int card_bladed_pinons(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 2, 0, 0, KEYWORD_FLYING+KEYWORD_FIRST_STRIKE, 0);
}

int card_bleak_coven_vampires(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) && metalcraft(player, card) && can_target(&td) ){
		pick_target(&td, "TARGET_PLAYER");
		lose_life(instance->targets[0].player, 4);
		gain_life(player, 4);
	}

	return 0;
}

int card_blistergrub(int player, int card, event_t event){

	if( graveyard_from_play(player, card, event) ){
		lose_life(1-player, 2);
	}

	return 0;
}

int card_bloodshoot_trainee(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && get_power(player, card) > 3 ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	else if( event == EVENT_ACTIVATE){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_ACTIVATION){
			if( valid_target(&td) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, instance->parent_card);
			}
	}

	return 0;
}

int card_blunt_the_assault(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL){
		int amount = count_permanents_by_type(player, TYPE_CREATURE) +
					count_permanents_by_type(1-player, TYPE_CREATURE);
		gain_life(player, amount);
		fog_effect(player, card);
		kill_card(player, card, KILL_DESTROY);
	}
	else{
		 return card_fog(player, card, event);
	}

	return 0;
}

int card_bonds_of_quicksilver(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		ai_modifier -= 16;
	}

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_carnifex_demon(int player, int card, event_t event){

	/* Carnifex Demon	|4|B|B
	 * Creature - Demon 6/6
	 * Flying
	 * ~ enters the battlefield with two -1/-1 counters on it.
	 * |B, Remove a -1/-1 counter from ~: Put a -1/-1 counter on each other creature. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_M1_M1, 2);

	if( event == EVENT_RESOLVE_ACTIVATION){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		card_instance_t* instance = get_card_instance(player, card);
		new_manipulate_all(instance->parent_controller, instance->parent_card, ANYBODY, &this_test, ACT_ADD_COUNTERS(COUNTER_M1_M1, 1));
	}

	return generic_activated_ability(player, card, event, GAA_MINUS1_MINUS1_COUNTER, MANACOST_B(1), 0, NULL, NULL);
}

int card_carrion_call(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_INSECT, &token);
		token.qty = 2;
		token.legacy = 1;
		token.special_code_for_legacy = &empty;
		token.special_infos = 67;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cerebral_eruption(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL){
		int *deck = deck_ptr[1-player];
		if( deck[0] != -1 ){
			show_deck(HUMAN, deck, 1, "Here's the first card of deck", 0, 0x7375B0 );
			int cmc = get_cmc_by_id(cards_data[deck[0]].id);
			if( cmc > 0 ){
				damage_all(player, card, 1-player, cmc, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}

			if( ! is_what(-1, deck[0], TYPE_LAND) ){
				kill_card(player, card, KILL_DESTROY);
			}
			else{
				bounce_permanent(player, card);
			}
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_chimeric_mass(int player, int card, event_t event){

	/* Chimeric Mass	|X
	 * Artifact
	 * ~ enters the battlefield with X charge counters on it.
	 * |1: Until end of turn, ~ becomes a Construct artifact creature with "This creature's power and toughness are each equal to the number of charge counters
	 * on it." */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG) ){
			infinite_casting_cost();
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 0;
		if( !played_for_free(player, card) && !is_token(player, card) ){
			charge_mana(player, COLOR_ARTIFACT, -1);
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
			}
		}
		if (instance->info_slot < 2){
			ai_modifier += (player == AI ? -256 : 256);
		}
	}

	enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, instance->info_slot);

	if( event == EVENT_RESOLVE_ACTIVATION){
		int pp = instance->parent_controller, pc = instance->parent_card;
		force_a_subtype(pp, pc, SUBTYPE_CONSTRUCT);
		artifact_animation(pp, pc, pp, pc, 1, count_counters(player, card, COUNTER_CHARGE), count_counters(player, card, COUNTER_CHARGE), 0, 0);
	}

	return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_clone_shell(int player, int card, event_t event){
	/*
	  Clone Shell |5
	  Artifact Creature - Shapeshifter 2/2
	  Imprint -	When Clone Shell enters the battlefield, look at the top four cards of your library, exile one face down,
	  then put the rest on the bottom of your library in any order.
	  When Clone Shell dies, turn the exiled card face up. If it's a creature card, put it onto the battlefield under your control.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		int number = MIN(4, count_deck(player));
		if( number > 0 ){
			char msg[100] = "Select a card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			this_test.create_minideck = number;
			this_test.no_shuffle = 1;

			int *deck = deck_ptr[player];

			int selected = -1;
			if( player != AI ){
				selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 1, AI_MAX_CMC, -1, &this_test);
			}
			else{
				new_default_test_definition(&this_test, TYPE_CREATURE, msg);
				selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 1, AI_MAX_CMC, -1, &this_test);
				if( selected == -1 ){
					new_default_test_definition(&this_test, TYPE_ANY, msg);
					selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 1, AI_MAX_CMC, -1, &this_test);
				}
			}
			instance->targets[9].player = deck[selected];
			rfg_card_from_grave(player, selected);
			number--;
			if( number > 0 ){
				put_top_x_on_bottom(player, player, number);
			}
		}
	}

	if( instance->targets[9].player != -1 && this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( check_rfg(player, cards_data[instance->targets[9].player].id) ){
			if( is_what(-1, instance->targets[9].player, TYPE_CREATURE) ){
				remove_card_from_rfg(player, cards_data[instance->targets[9].player].id );
				int card_added = add_card_to_hand(player, instance->targets[9].player);
				put_into_play(player, card_added);
			}
			else{
				int card_stored[1] = {instance->targets[9].player};
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Here's the card exiled with Clone Shell");
				select_card_from_zone(1-player, 1-player, card_stored, 1, 0, AI_FIRST_FOUND, -1, &this_test);
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE && player == HUMAN && instance->targets[9].player > -1 ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		int card_stored[1] = {instance->targets[9].player};
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Here's the card exiled with Clone Shell");
		select_card_from_zone(player, player, card_stored, 1, 0, AI_FIRST_FOUND, -1, &this_test);
		spell_fizzled = 1;
	}

	return 0;
}

int card_copperhorn_scout(int player, int card, event_t event)
{
  // Whenever ~ attacks, untap each other creature you control.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;
	  new_manipulate_all(player, card, player, &test, ACT_UNTAP);
	}

  return 0;
}

int card_corpse_cur(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card with Infect.");
		this_test.keyword = KEYWORD_INFECT;
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

int card_corrupted_harvester(int player, int card, event_t event)
{
  /* Corrupted Harvester	|4|B|B
   * Creature - Horror 6/3
   * |B, Sacrifice a creature: Regenerate ~. */

  if (!(land_can_be_played & LCBP_REGENERATION))
	return 0;

  if (!IS_ACTIVATING(event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "");
  if (IS_AI(player))
	test.not_me = 1;

  if (event == EVENT_CAN_ACTIVATE && CAN_ACTIVATE(player, card, MANACOST_B(1)) && new_can_sacrifice_as_cost(player, card, &test))
	return can_regenerate(player, card);

  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST_B(1)))
	new_sacrifice(player, card, player, SAC_AS_COST, &test);

  if (event == EVENT_RESOLVE_ACTIVATION && can_regenerate(instance->parent_controller, instance->parent_card))
	regenerate_target(instance->parent_controller, instance->parent_card);

  return 0;
}

int card_culling_dais(int player, int card, event_t event)
{
  /* Culling Dais	|2
   * Artifact
   * |T, Sacrifice a creature: Put a charge counter on ~.
   * |1, Sacrifice ~: Draw a card for each charge counter on ~. */

  if (IS_ACTIVATING(event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");

	  enum
	  {
		CHOICE_CHARGE = 1,
		CHOICE_DRAW
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Charge counter", can_sacrifice_type_as_cost(player, 1, TYPE_CREATURE), 2, DLG_TAP, DLG_MANA(MANACOST0),
						"Draw cards", can_sacrifice_this_as_cost(player, card), count_counters(player, card, COUNTER_CHARGE), DLG_MANA(MANACOST_X(1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return can_use_activated_abilities(player, card) && choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_CHARGE:
			  if (controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0))
				tap_card(player, card);
			  break;

			case CHOICE_DRAW:
			  kill_card(player, card, KILL_SACRIFICE);
			  break;
		  }
	  else	// EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_CHARGE:
			  add_counter(player, card, COUNTER_CHARGE);
			  break;

			case CHOICE_DRAW:
			  draw_cards(player, count_counters(player, card, COUNTER_CHARGE));
			  break;
		  }
	}

  if (event == EVENT_SHOULD_AI_PLAY)
	ai_modifier += (player == AI ? 12 : -12) * count_counters(player, card, COUNTER_CHARGE);

  return 0;
}

int card_darkslick_drake(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_darksteel_axe(int player, int card, event_t event){

	indestructible(player, card, event);

	return vanilla_equipment(player, card, event, 2, 2, 0, 0, 0);
}

int card_darksteel_juggernaut(int player, int card, event_t event){

	indestructible(player, card, event);

	attack_if_able(player, card, event);

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		event_result += count_permanents_by_type(player, TYPE_ARTIFACT);
	}
	return 0;
}

int card_darksteel_myr(int player, int card, event_t event){

	indestructible(player, card, event);

	return 0;
}

int card_darksteel_sentinel(int player, int card, event_t event){

	indestructible(player, card, event);

	vigilance(player, card, event);

	return flash(player, card, event);
}

int card_dispense_justice(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = current_turn;
	td.preferred_controller = current_turn;
	td.who_chooses = current_turn;
	td.illegal_abilities = 0;
	td.required_state = TARGET_STATE_ATTACKING;
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		if( player != AI ){
			return 1;
		}
		else{
			return can_target(&td);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td1, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL){
		int amount = 1;
		if( metalcraft(player, card) ){
			amount = 2;
		}
		if( can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
			 int i;
			 for(i=0; i<amount; i++){
				if( can_target(&td) ){
					pick_target(&td, "LORD_OF_THE_PIT");
					instance->number_of_targets = 1;
					kill_card( instance->targets[0].player,  instance->targets[0].card, KILL_SACRIFICE);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dissipation_field(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player ){
				card_instance_t *target = get_card_instance(damage->damage_source_player, damage->damage_source_card);
				if( damage->info_slot > 0 || target->targets[16].player > 0 ){
					if( instance->info_slot < 0 ){
						instance->info_slot = 0;
					}
					instance->targets[instance->info_slot].player = damage->damage_source_player;
					instance->targets[instance->info_slot].card = damage->damage_source_card;
					instance->info_slot++;
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
				int i;
				for(i=0;i<instance->info_slot;i++){
					if( instance->targets[i].player != -1 && instance->targets[i].card != -1 &&
						in_play(instance->targets[i].player, instance->targets[i].card)
					  ){
						bounce_permanent(instance->targets[i].player, instance->targets[i].card);
					}
				}
				instance->info_slot = 0;
		}
	}

	return global_enchantment(player, card, event);
}

int card_dross_hopper(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, KEYWORD_FLYING, 0);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_echo_circlet(int player, int card, event_t event){

	if( is_equipping(player, card) ){
		attached_creature_can_block_additional(player, card, event, 1);
	}
	return basic_equipment(player, card, event, 1);
}

int card_embersmith(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance( player, card );

	if( can_target(&td) && has_mana(player, COLOR_COLORLESS, 1) && smiths(player, card, event) ){
		charge_mana(player, COLOR_COLORLESS, 1);
		instance->info_slot = 1;
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, 1);
			instance->number_of_targets = 1;
		}
		else{
			spell_fizzled = 1;
		}
		instance->info_slot = 0;
	}

	return 0;
}

static void destroy_creature_and_gain_life(int player, int card, int t_player, int t_card)
{
  int tgh = get_toughness(t_player, t_card);
  kill_card(t_player, t_card, KILL_DESTROY);
  gain_life(player, tgh);
}

int card_engulfing_slagwurm(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card))
	instance->destroys_if_blocked |= DIFB_DESTROYS_ALL;

  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
	{
	  if (player == current_turn && (instance->state & STATE_ATTACKING))
		for_each_creature_blocking_me(player, card, destroy_creature_and_gain_life, player, card);

	  if (player == 1-current_turn && (instance->state & STATE_BLOCKING))
		for_each_creature_blocked_by_me(player, card, destroy_creature_and_gain_life, player, card);
	}

  return 0;
}

int card_exsanguinate(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

	if( event == EVENT_RESOLVE_SPELL){
		gain_life(player, instance->info_slot);
		lose_life(1-player, instance->info_slot);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ezuris_archers(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_DECLARE_BLOCKERS && current_turn != player ){
		if( instance->blocking < 255 ){
			if( check_for_ability(1-player, instance->blocking, KEYWORD_FLYING) ){
				pump_until_eot(player, card, player, card, 3, 0);
			}
		}
	}

	return 0;
}

int card_ferrovore(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
		if( spell_fizzled != 1 && ! controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, 0) ){
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, card, instance->parent_controller, instance->parent_card, 3,0);
	}

	return 0;
}

int card_flameborn_hellion(int player, int card, event_t event){

	haste(player, card, event);
	attack_if_able(player, card, event);

	return 0;
}

int card_flesh_allergy(int player, int card, event_t event){

	/* Flesh Allergy	|2|B|B
	 * Sorcery
	 * As an additional cost to cast ~, sacrifice a creature.
	 * Destroy target creature. Its controller loses life equal to the number of creatures that died this turn. */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0) ){
			pick_target(&td1, "TARGET_CREATURE");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			lose_life(instance->targets[0].player, creatures_dead_this_turn);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_flight_spellbomb(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}
	else if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
			}
	}

	return som_spellbombs(player, card, event, COLOR_BLUE);
}

int card_fulgent_distraction(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->targets[1].player = instance->targets[0].player;
			instance->targets[1].card = instance->targets[0].card;
			if( can_target(&td) ){
				state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
				int choice = do_dialog(player, player, card, -1, -1, " Continue\n Stop", 0);
				if( choice == 0 ){
					td.allow_cancel = 0;
					pick_target(&td, "TARGET_CREATURE");
				}
				state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
				equipments_attached_to_me(instance->targets[i].player, instance->targets[i].card, EATM_UNATTACH);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fume_spitter(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_furnace_celebration(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY && can_target(&td) && has_mana(player, COLOR_COLORLESS, 2) ){
		card_instance_t* affected = in_play(affected_card_controller, affected_card);
		if (affected && affected_card_controller == player && affected_card != card &&
			is_what(affected_card_controller, affected_card, TYPE_PERMANENT) &&
			affected->kill_code == KILL_SACRIFICE && !check_special_flags(affected_card_controller, affected_card, SF_KILL_STATE_BASED_ACTION)
		   ){
			if( instance->targets[11].player < 0 ){
				instance->targets[11].player = 0;
			}
			instance->targets[11].player++;
		}
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller && affect_me(player, card) &&
		instance->targets[11].player > 0
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 1+player;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int amount = instance->targets[11].player;
				while( amount > 0 && can_target(&td) && has_mana(player, COLOR_COLORLESS, 2) ){
						instance->number_of_targets = 0;
						if (charge_mana_while_resolving(player, card, player, EVENT_RESOLVE_TRIGGER, COLOR_COLORLESS, 2)
							&& pick_target(&td, "TARGET_CREATURE_OR_PLAYER")
						   ){
							damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
							amount--;
						}
						else{
							cancel = 0;
							break;
						}
				}
				instance->targets[11].player = 0;
		}
		else if (event == EVENT_END_TRIGGER){
			instance->targets[11].player = 0;
		}
	}

	return global_enchantment(player, card, event);
}

int card_galvanic_blast(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, metalcraft(player, card) ? 4 : 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int can_activate_geth(int player, int card, int mode){
	if( graveyard_has_shroud(2) ){
		return 0;
	}
	const int *grave = get_grave(1-player);
	int count = 0;
	int par = 0;
	int result = 0;
	while( grave[count] != -1 ){
			if( is_what(-1, grave[count], TYPE_CREATURE | TYPE_ARTIFACT) ){
				int cmc = get_cmc_by_id(cards_data[grave[count]].id);
				if( has_mana_for_activated_ability(player, card, cmc, 1, 0, 0, 0, 0) ){
					if( mode == 0 ){
						result = 1;
						break;
					}
					if( mode == 1 ){
						if( cmc > par ){
							par = cmc;
							result = count;
						}
					}
				}
			}
			count++;
	}
	if( mode == 1  && result == 0 ){
		result = -1;
	}
	return result;
}

int card_geth_lord_of_the_vault(int player, int card, event_t event ){

	char msg[100] = "Select a creature or artifact card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_ARTIFACT, msg);

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	intimidate(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return can_activate_geth(player, card, 0);
	}
	if( event == EVENT_ACTIVATE ){
		const int *grave = get_grave(1-player);
		int selected = -1;
		if( player != AI ){
			selected = new_select_target_from_grave(player, card, 1-player, 0, AI_MAX_VALUE, &this_test, 0);
		}
		else{
			selected = can_activate_geth(player, card, 1);
		}
		if( selected != -1 ){
			int cmc = get_cmc_by_id(cards_data[grave[selected]].id);
			if( charge_mana_for_activated_ability(player, card, cmc, 1, 0, 0, 0, 0) ){
				instance->info_slot = cmc;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, 1-player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, instance->parent_card, 1-player, selected, REANIMATE_TAP);
			mill(1-player, instance->info_slot);
		}
	}

	return 0;
}

int card_glimmerpoint_stag(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	vigilance(player, card, event);

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

int card_glimmerpost(int player, int card, event_t event)
{
  // When ~ enters the battlefield, you gain 1 life for each Locus on the battlefield.
  if (comes_into_play(player, card, event))
	gain_life(player, count_subtype(ANYBODY, TYPE_LAND, SUBTYPE_LOCUS));

  // |T: Add |1 to your mana pool.
  return mana_producer(player, card, event);
}

static void animate_glint_hawk_idol(int player, int card){
	add_a_subtype(player, card, SUBTYPE_BIRD);
	int legacy = artifact_animation(player, card, player, card, 1, 2, 2, KEYWORD_FLYING, 0);
	card_instance_t *leg = get_card_instance( player, legacy );
	leg->targets[6].player = COLOR_TEST_WHITE;
}

int card_glint_hawk_idol(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION){
		animate_glint_hawk_idol(player, instance->parent_card);
	}

	if( idols(player, card, event) ){
		animate_glint_hawk_idol(player, card);
	}

	return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 0, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_goblin_gaveleer(int player, int card, event_t event){

	if( event == EVENT_POWER && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result += (equipments_attached_to_me(player, card, EATM_REPORT_TOTAL) *2);
	}

	return 0;
}

int card_golden_urn(int player, int card, event_t event){

	/* Golden Urn	|1
	 * Artifact
	 * At the beginning of your upkeep, you may put a charge counter on ~.
	 * |T, Sacrifice ~: You gain life equal to the number of charge counters on ~. */

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_CHARGE);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, count_counters(player, card, COUNTER_CHARGE));
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_golem_artisan(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.special = TARGET_SPECIAL_ARTIFACT_CREATURE;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0)
	  ){
		return can_target(&td);
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				if( is_what(instance->targets[0].player, instance->targets[0].card, TYPE_ARTIFACT) ){
					int choice = 0;
					int plus = 0;
					int keyword = 0;
					int sp_keyword = 0;
					int ai_choice = 0;
					if( instance->targets[0].player == player ){
						if( current_phase < PHASE_DECLARE_BLOCKERS && ! check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_FLYING) ){
							ai_choice = 1;
						}
						if( is_sick(instance->targets[0].player, instance->targets[0].card) ){
							ai_choice = 3;
						}
						if( current_phase == PHASE_AFTER_BLOCKING && is_attacking(instance->targets[0].player, instance->targets[0].card) &&
							! check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_TRAMPLE)
						  ){
							int count = 0;
							int att_pow = 0;
							int def_pow = 0;
							while( count < active_cards_count[1-player] ){
									if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
										card_instance_t *crt = get_card_instance(1-player, count);
										if( crt->blocking == card ){
											att_pow+=get_power(1-player, count);
											def_pow+=get_toughness(1-player, count);
										}
									}
									count++;
							}
							if( att_pow < get_toughness(player, card) && def_pow < get_power(player, card) ){
								ai_choice = 2;
							}
						}
						choice = do_dialog(player, player, card, -1, -1, " Pump\n Give Flying\n Give Trample\n Give Haste\n Do nothing", ai_choice);
					}
					if( choice == 0 ){
						plus = 1;
					}
					else if( choice == 1 ){
							keyword = KEYWORD_FLYING;
					}
					else if( choice == 2 ){
							keyword = KEYWORD_TRAMPLE;
					}
					else if( choice == 3 ){
							sp_keyword = SP_KEYWORD_HASTE;
					}
					else{
						spell_fizzled = 1;
					}
					instance->targets[1].player = plus;
					instance->targets[2].player = keyword;
					instance->targets[2].card = sp_keyword;
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								   instance->targets[1].player, instance->targets[1].player,
								   instance->targets[2].player, instance->targets[2].card);
		}
	}

	if( player == AI && has_mana(player, COLOR_COLORLESS, 2) && trigger_condition == TRIGGER_COMES_INTO_PLAY &&
		affect_me(player, card) && player == reason_for_trigger_controller && player == trigger_cause_controller
	  ){
		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_ARTIFACT+TYPE_CREATURE) ){
			trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					charge_mana(player, COLOR_COLORLESS, 2);
					if( spell_fizzled != 1 ){
						pump_ability_until_eot(player, card, trigger_cause_controller, trigger_cause, 0, 0, 0, SP_KEYWORD_HASTE);
					}
			}
		}
	}

	return 0;
}

int card_golem_foundry(int player, int card, event_t event){

	/* Golem Foundry	|3
	 * Artifact
	 * Whenever you cast an artifact spell, you may put a charge counter on ~.
	 * Remove three charge counters from ~: Put a 3/3 colorless Golem artifact creature token onto the battlefield. */

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_DUH, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		add_counter(player, card, COUNTER_CHARGE);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		count_counters(player, card, COUNTER_CHARGE) >= 3 && has_mana_for_activated_ability(player, card, MANACOST_X(0))
	  ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(0)) ){
			remove_counters(player, card, COUNTER_CHARGE, 3);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_GOLEM);
	}
	return 0;
}

int card_golems_heart(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_AI(player), TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		gain_life(player, 1);
	}

	return 0;
}

int card_grand_architect(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.required_color = COLOR_TEST_BLUE;
	td.illegal_state = TARGET_STATE_TAPPED;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.preferred_controller = player;
	td1.special = TARGET_SPECIAL_ARTIFACT_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( can_target(&td) || (has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) && can_target(&td1)) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) && can_target(&td1) && ! paying_mana() ){
				choice = do_dialog(player, player, card, -1, -1, " Get mana\n Make Art.Creature blue\n Cancel", 1);
			}
		}

		instance->info_slot = 1+choice;

		if( choice == 0 ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				// see comments in tap_a_permanent_you_control_for_mana(), which this should likely be rewritten to use
				produce_mana(player, COLOR_ARTIFACT, 2);
				// This isn't actually "tapping for mana", since it doesn't have the {T} symbol, but still a mana ability, so prevent response
				tapped_for_mana_color = -2;
				if (instance->targets[0].player == player && instance->targets[0].card == card){
					instance->state |= STATE_TAPPED;
				} else {
						get_card_instance(instance->targets[0].player, instance->targets[0].card)->state |= STATE_TAPPED;
						dispatch_event(instance->targets[0].player, instance->targets[0].card, EVENT_TAP_CARD);
				}
			} else {
				spell_fizzled = 1;
			}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td1, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 2 && valid_target(&td1) ){
			change_color(player, card, instance->targets[0].player, instance->targets[0].card, COLOR_TEST_BLUE, CHANGE_COLOR_SET|CHANGE_COLOR_END_AT_EOT);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_target(&td) ){
			declare_mana_available(player, 6, 2);
		}
	}

	return boost_creature_by_color(player, card, event, COLOR_TEST_BLUE, 1, 1, 0, BCT_CONTROLLER_ONLY);
}

int card_grasp_of_darkness(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	if( player == AI ){
		td.toughness_requirement = 4;
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -4, -4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_grindclock(int player, int card, event_t event)
{
  /* Grindclock	|2
   * Artifact
   * |T: Put a charge counter on ~.
   * |T: Target player puts the top X cards of his or her library into his or her graveyard, where X is the number of charge counters on ~. */

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  int ai_priority_charge, ai_priority_mill;
	  if (event == EVENT_ACTIVATE && IS_AI(player))
		{
		  int counters = count_counters(player, card, COUNTER_CHARGE);
		  if (counters == 0)
			{
			  ai_priority_charge = 1;
			  ai_priority_mill = -1;
			}
		  else
			{
			  int cards_in_library = count_deck(1-player);
			  // 1000 - (number of turns to exhaust library, including one normal draw per turn, rounded up)
			  ai_priority_mill   = 1000 - (  (cards_in_library + counters  ) / (1 + counters  ));
			  ai_priority_charge = 1000 - (1+(cards_in_library + counters+1) / (1 + counters+1));
			}
		}
	  else
		ai_priority_charge = ai_priority_mill = 1;

	  target_definition_t td;
	  default_target_definition(player, card, &td, 0);
	  td.zone = TARGET_ZONE_PLAYERS;

	  enum
	  {
		CHOICE_CHARGE = 1,
		CHOICE_MILL
	  } choice = DIALOG(player, card, event,
						"Charge counter", 1, ai_priority_charge, DLG_TAP, DLG_MANA(MANACOST0),
						"Mill",           1, ai_priority_mill,   DLG_TAP, DLG_MANA(MANACOST0), DLG_TARGET(&td, "TARGET_PLAYER"));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		ai_modifier += player == AI ? 24 : -24;
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_CHARGE:
			  add_counter(player, card, COUNTER_CHARGE);
			  break;

			case CHOICE_MILL:
			  mill(get_card_instance(player, card)->targets[0].player, count_counters(player, card, COUNTER_CHARGE));
			  break;
		  }
	}

	return 0;
}

int card_halt_order(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			if( is_what(card_on_stack_controller, card_on_stack, TYPE_ARTIFACT) ){
				return result;
			}
		}
	}

	else if( event == EVENT_RESOLVE_SPELL){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			draw_cards(player, 1);
			return card_counterspell(player, card, event);
	}

	else{
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_heavy_arbalest(int player, int card, event_t event){

	/* Heavy Arbalest	|3
	 * Artifact - Equipment
	 * Equipped creature doesn't untap during its controller's untap step.
	 * Equipped creature has "|T: This creature deals 2 damage to target creature or player."
	 * Equip |4 */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( is_equipping(player, card) ){
		does_not_untap(instance->targets[8].player, instance->targets[8].card, event);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( is_equipping(player, card) && ! is_tapped(instance->targets[8].player, instance->targets[8].card) &&
			! is_sick(instance->targets[8].player, instance->targets[8].card) &&
			has_mana_for_activated_ability(instance->targets[8].player, instance->targets[8].card, 0, 0, 0, 0, 0, 0)
		  ){
			td1.illegal_abilities = get_protections_from(instance->targets[8].player, instance->targets[8].card);
			if( can_target(&td1) ){
				return 1;
			}
		}
		return can_activate_basic_equipment(player, card, event, 4);
	}
	else if( event == EVENT_ACTIVATE ){
			int equip_cost = get_updated_equip_cost(player, card, 4);
			int choice = 0;
			if( has_mana( player, COLOR_COLORLESS, equip_cost) && can_sorcery_be_played(player, event) ){
				if( is_equipping(player, card) && ! is_tapped(instance->targets[8].player, instance->targets[8].card) &&
					! is_sick(instance->targets[8].player, instance->targets[8].card) &&
					has_mana_for_activated_ability(instance->targets[8].player, instance->targets[8].card, 0, 0, 0, 0, 0, 0)
				  ){
					td1.illegal_abilities = get_protections_from(instance->targets[8].player, instance->targets[8].card);
					if( can_target(&td1) ){
						choice = do_dialog(player, player, card, -1, -1, " Equip\n Tap & damage\n Do nothing", 1);
					}
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				activate_basic_equipment(player, card, 4);
				instance->info_slot = 66;
			}
			else if( choice == 1 ){
				if( charge_mana_for_activated_ability(instance->targets[8].player, instance->targets[8].card, 0, 0, 0, 0, 0, 0) &&
					pick_target(&td1, "TARGET_CREATURE_OR_PLAYER")
				  ){
						instance->number_of_targets = 1;
						instance->info_slot = 67;
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
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 ){
				resolve_activation_basic_equipment(player, card);
			}
			if( instance->info_slot == 67 && valid_target(&td1)
			  ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 2,
								instance->targets[8].player, instance->targets[8].card);
				tap_card(instance->targets[8].player, instance->targets[8].card);
			}
	}

	return 0;
}

int card_hoard_smelter_dragon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int pump = get_cmc(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, pump, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(3, 1), 0, &td, "TARGET_ARTIFACT");
}

int card_horizon_spellbomb(int player, int card, event_t event){

 //   card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0)
	  ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				tap_card(player, card);
				kill_card(player, card, KILL_SACRIFICE);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 tutor_basic_land(player, 0, 0);
	}

	return som_spellbombs(player, card, event, COLOR_GREEN);
}

int card_ichor_rats(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		poison_counters[player]++;
		poison_counters[1-player]++;
	}

	return 0;
}

int card_indomitable_archangel(int player, int card, event_t event){

	if( metalcraft(player, card) ){
		if( event == EVENT_ABILITIES && affected_card_controller == player &&
			is_what(affected_card_controller, affected_card, TYPE_ARTIFACT)
		  ){
			event_result |= KEYWORD_SHROUD;
		}
	}

	return 0;
}

int card_inexorable_tide(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		player == reason_for_trigger_controller && player == trigger_cause_controller
	  ){
		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) && trigger_cause_controller == player ){
			trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					proliferate(player, card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_infiltration_lens(int player, int card, event_t event)
{
  /* Infiltration Lens	|1
   * Artifact - Equipment
   * Whenever equipped creature becomes blocked by a creature, you may draw two cards.
   * Equip |1 */

  if (event == EVENT_DECLARE_BLOCKERS && is_equipping(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->targets[8].player;
	  int c = instance->targets[8].card;

	  int num_blockers;
	  if (current_turn == p && is_attacking(p, c))
		for (num_blockers = count_my_blockers(p, c); num_blockers > 0; --num_blockers)
		  if (!draw_some_cards_if_you_want(player, card, player, 2))
			break;
	}

  if (event == EVENT_ATTACK_RATING)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->targets[8].player, instance->targets[8].card) && is_equipping(player, card))
		ai_defensive_modifier -= 36;
	}

  return vanilla_equipment(player, card, event, 1, 0, 0, 0, 0);
}

int card_instill_infection(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	if( player == AI ){
		td.toughness_requirement = 1;
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_kembas_skyguard(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		gain_life(player, 2);
	}

	return 0;
}

int card_kemba_kha_regent(int player, int card, event_t event){
	/* Kemba, Kha Regent	|1|W|W
	 * Legendary Creature - Cat Cleric 2/4
	 * At the beginning of your upkeep, put a 2/2 |Swhite Cat creature token onto the battlefield for each Equipment attached to ~. */

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int amount = equipments_attached_to_me(player, card, EATM_REPORT_TOTAL);
		generate_tokens_by_id(player, card, CARD_ID_CAT, amount);
	}
	return 0;
}

int card_kuldotha_phoenix(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	haste(player, card, event);

	if( metalcraft(player, card) && has_mana(player, COLOR_COLORLESS, 4) && event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
		int choice = do_dialog(player, player, card, -1, -1," Return Kuldotha Phoenix to play\n Pass", 0);
		if( choice == 0 ){
			charge_mana(player, COLOR_COLORLESS, 4);
			if( spell_fizzled != 1){
				instance->state &= ~STATE_INVISIBLE;
				put_into_play(player, card);
				return -1;
			}
		}
		return -2;
	}

	return 0;
}

int card_leonine_arbiter(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CLEANUP ){
		instance->targets[2].player = 0;
		instance->targets[3].player = 0;
	}
	return 0;
}

int card_liege_of_the_tangle(int player, int card, event_t event){

	/* Liege of the Tangle	|6|G|G
	 * Creature - Elemental 8/8
	 * Trample
	 * Whenever ~ deals combat damage to a player, you may choose any number of target lands you control and put an awakening counter on each of them. Each of
	 * those lands is an 8/8 |Sgreen Elemental creature for as long as it has an awakening counter on it. They're still lands. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.illegal_type = TYPE_CREATURE;
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		color_test_t clr = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
		int choice = 0;
		if( !IS_AI(player) ){
			choice = do_dialog(player, player, card, -1, -1," Auto mode\n Manual mode", 0);
		}
		if( choice == 1 ){
			while( can_target(&td) && pick_target(&td, "TARGET_LAND") ){
				instance->number_of_targets = 1;
				add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_AWAKENING);
				add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_ELEMENTAL);
				int leg = land_animation2(player, card, instance->targets[0].player, instance->targets[0].card, 4, 8,8, 0,0, clr, NULL);
				SET_BYTE0(get_card_instance(player, leg)->targets[7].player) = COUNTER_AWAKENING;
			}
		}
		else{
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_play(player, count) && is_what(player, count, TYPE_LAND) && ! is_what(player, count, TYPE_CREATURE) ){
						add_counter(player, count, COUNTER_AWAKENING);
						add_a_subtype(player, count, SUBTYPE_ELEMENTAL);
						int leg = land_animation2(player, card, player, count, 4, 8,8, 0,0, clr, NULL);
						SET_BYTE0(get_card_instance(player, leg)->targets[7].player) = COUNTER_AWAKENING;
					}
					count--;
			}
		}

	}
	return 0;
}

int card_lifesmith(int player, int card, event_t event){

	if( has_mana(player, COLOR_COLORLESS, 1) && smiths(player, card, event) ){
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			gain_life(player, 3);
		}
	}

	return 0;
}


int card_liquidmetal_coating(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.illegal_type = TYPE_ARTIFACT;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			turn_into_artifact(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PERMANENT");
}

int card_livewire_lash(int player, int card, event_t event){

	/* Livewire Lash	|2
	 * Artifact - Equipment
	 * Equipped creature gets +2/+0 and has "Whenever this creature becomes the target of a spell, this creature deals 2 damage to target creature or player."
	 * Equip |2 */

	card_instance_t *instance = get_card_instance( player, card );

	if (is_equipping(player, card)){
		int p = instance->targets[8].player;
		int c = instance->targets[8].card;

		if (!is_humiliated(player, card)
			&& becomes_target_of_spell(player, card, event, p, c, ANYBODY, RESOLVE_TRIGGER_MANDATORY)){

			target_definition_t td;
			default_target_definition(p, c, &td, TYPE_CREATURE);
			td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
			td.illegal_abilities = get_protections_from(p, c);
			td.allow_cancel = 0;

			if (can_target(&td)){
				load_text(0, "TARGET_CREATURE_OR_PLAYER");
				if (select_target(p, c, &td, "TARGET_CREATURE_OR_PLAYER", &instance->targets[0])){
					instance->number_of_targets = 1;
					damage_creature(instance->targets[0].player, instance->targets[0].card, 2, p, c);
				} else {
					cancel = 1;
				}
			}
		}
	}

	return vanilla_equipment(player, card, event, 2, 2, 0, 0, 0);
}

int card_lumengrid_drake(int player, int card, event_t event){

	if( metalcraft(player, card) ){
		return card_man_o_war(player, card, event);
	}

	return 0;
}

int card_memoricide(int player, int card, event_t event){

	/* Memoricide	|3|B
	 * Sorcery
	 * Name a nonland card. Search target player's graveyard, hand, and library for any number of cards with that name and exile them. Then that player shuffles
	 * his or her library. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
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

int card_melt_terrain(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_LAND");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, 2, player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_molder_beast(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_ARTIFACT, NULL);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		pump_until_eot(player, card, player, card, 2*instance->targets[11].card, 0);
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_molten_psyche(int player, int card, event_t event){

	/* Molten Psyche	|1|R|R
	 * Sorcery
	 * Each player shuffles the cards from his or her hand into his or her library, then draws that many cards.
	 * Metalcraft - If you control three or more artifacts, ~ deals damage to each opponent equal to the number of cards that player has drawn this turn. */

	if( event == EVENT_CAN_CAST ){
		if( player != AI ){
			return 1;
		}
		else{
			 if( hand_count[player] > 2 ){
				 return 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int num = reshuffle_hand_into_deck(player, 0);
		draw_cards(player, num);
		num = reshuffle_hand_into_deck(1-player, 0);
		draw_cards(1-player, num);
		if( metalcraft(player, card) ){
			damage_player(1-player, cards_drawn_this_turn[1-player], player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_moriok_replica(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 2);
		lose_life(player, 2);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 1, 1, 0, 0, 0, 0, 0, 0, 0);
}

int card_mox_opal(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	if( metalcraft(player, card) ){
		return mana_producer(player, card, event);
	}
	return 0;
}

int card_myr_propagator(int player, int card, event_t event){
	/* Myr Propagator	|3
	 * Artifact Creature - Myr 1/1
	 * |3, |T: Put a token that's a copy of ~ onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;

		card_instance_t* parent = get_card_instance(instance->parent_controller, instance->parent_card);
		int iid = parent->internal_card_id;
		if (iid >= 0){
			copy_token_definition(player, card, &token, instance->parent_controller, instance->parent_card);
		} else {
			default_token_definition(player, card, cards_data[instance->backup_internal_card_id].id, &token);
			token.no_sleight = 1;
		}
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 3, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_myr_reservoir(int player, int card, event_t event){

	char msg[100] = "Select a Myr card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, msg);
	this_test.subtype = SUBTYPE_MYR;

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL) ){
			if( new_special_count_grave(player, &this_test) > 0 ){
				return ! graveyard_has_shroud(2);
			}
		}
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0);
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int selected = validate_target_from_grave_source(player, card, player, 0);
			if( selected != -1 ){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
	}
	return 0;
}

int card_myrsmith(int player, int card, event_t event){
	/* Myrsmith	|1|W
	 * Creature - Human Artificer 2/1
	 * Whenever you cast an artifact spell, you may pay |1. If you do, put a 1/1 colorless Myr artifact creature token onto the battlefield. */

	if( has_mana(player, COLOR_COLORLESS, 1) && smiths(player, card, event) ){
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			generate_token_by_id(player, card, CARD_ID_MYR);
		}
	}

	return 0;
}

int card_necrogen_censer(int player, int card, event_t event){

	/* Necrogen Censer	|3
	 * Artifact
	 * ~ enters the battlefield with two charge counters on it.
	 * |T, Remove a charge counter from ~: Target player loses 2 life. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, 2);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 2);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_CHARGE), &td, "TARGET_PLAYER");
}

int card_necrogen_scudder(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && life[player] < 9 ){
			ai_modifier-=1000;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		lose_life(player, 3);
	}

	return 0;
}

int card_necropede(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		card_instance_t *instance = get_card_instance( player, card );
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
		}
	}

	return 0;
}

int card_neurok_invisimancer(int player, int card, event_t event){

	unblockable(player, card, event);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		}
	}

	return 0;
}

int card_neurok_replica(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 1, 0, 1, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_nihil_spellbomb(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
		! is_tapped(player, card) && ! is_animated_and_sick(player, card)
	  ){
		return can_target(&td);
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_PLAYER") ){
				tap_card(player, card);
				kill_card(player, card, KILL_SACRIFICE);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 if( valid_target(&td) ){
				 const int *grave = get_grave(instance->targets[0].player);
				int count = count_graveyard(instance->targets[0].player)-1;
				while( count > -1 ){
						add_card_to_rfg(instance->targets[0].player, grave[count]);
						remove_card_from_grave(instance->targets[0].player, count);
						count--;
				}
			}
	}

	return som_spellbombs(player, card, event, COLOR_BLACK);
}

static int effect_unattach_when_lose_control(int player, int card, event_t event)
{
  card_instance_t* instance;
  if (event == EVENT_CARDCONTROLLED
	  && (instance = get_card_instance(player, card))
	  && affect_me(instance->damage_target_player, instance->damage_target_card))
	{
	  unattach(instance->damage_target_player, instance->damage_target_card);
	  kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}

int card_ogre_geargrabber(int player, int card, event_t event)
{
  /* Whenever ~ attacks, gain control of target Equipment an opponent controls until end of turn. Attach it to ~. When you lose control of that Equipment,
   * unattach it. */
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_ARTIFACT);
	  td.allowed_controller = 1-player;
	  td.preferred_controller = 1-player;
	  td.required_subtype = SUBTYPE_EQUIPMENT;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_EQUIPMENT"))
		{
		  gain_control_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
		  // targets[0] will have been updated during the control change.
		  equip_target_creature(instance->targets[0].player, instance->targets[0].card, player, card);
		  if (instance->targets[0].player == player)
			create_targetted_legacy_effect(player, card, &effect_unattach_when_lose_control, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_origin_spellbomb(int player, int card, event_t event){
	/* Origin Spellbomb	|1
	 * Artifact
	 * |1, |T, Sacrifice ~: Put a 1/1 colorless Myr artifact creature token onto the battlefield.
	 * When ~ is put into a graveyard from the battlefield, you may pay |W. If you do, draw a card. */

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0)
	  ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				tap_card(player, card);
				kill_card(player, card, KILL_SACRIFICE);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 generate_token_by_id(player, card, CARD_ID_MYR);
	}

	return som_spellbombs(player, card, event, COLOR_WHITE);
}

int card_oxidda_daredevil(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		else if( event == EVENT_ACTIVATE ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
					! controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, 0)
				  ){
					spell_fizzled = 1;
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card,
									   0, 0, 0, SP_KEYWORD_HASTE);
		}

	return 0;
}

int card_painful_quandary(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		player == reason_for_trigger_controller && 1-player == trigger_cause_controller ){

		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int ai_choice = 0;
					if( life[1-player] < 10 ){
						ai_choice = 1;
					}
					int choice = 0;
					if( hand_count[1-player] > 0 ){
						choice = do_dialog(1-player, player, card, -1, -1, " Lose 5 life\n Discard", ai_choice);
					}

					if( choice == 0 ){
						lose_life(1-player, 5);
					}
					else{
						discard(1-player, 0, player);
					}
			}
		}
	}

	return global_enchantment(player, card, event);
}

static int painsmith_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	deathtouch(p, c, event);

	modify_pt_and_abilities(p, c, event, 2, 0, 0);

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_painsmith(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	card_instance_t *instance = get_card_instance( player, card );

	if( can_target(&td) && smiths(player, card, event) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			create_targetted_legacy_effect(player, card, &painsmith_legacy,
											instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_panic_spellbomb(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	if( player == AI ){
		td.allowed_controller = 1-player;
		td.allowed_controller = 1-player;
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
		! is_tapped(player, card) && ! is_animated_and_sick(player, card)
	  ){
		if( player == AI ){
			if( current_turn == player  ){
				return can_target(&td);
			}
		}
		else{
			return can_target(&td);
		}
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
				tap_card(player, card);
				kill_card(player, card, KILL_SACRIFICE);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			}
	}

	return som_spellbombs(player, card, event, COLOR_RED);
}

int card_platinum_emperion(int player, int card, event_t event){
	return 0;
}

int card_precursor_golem(int player, int card, event_t event){
	return 0;
}

int card_prototype_portal(int player, int card, event_t event){
	/* Prototype Portal	|4
	 * Artifact
	 * Imprint - When ~ enters the battlefield, you may exile an artifact card from your hand.
	 * |X, |T: Put a token that's a copy of the exiled card onto the battlefield. X is the converted mana cost of that card. */

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.allowed_controller = player;
	td.allowed_controller = player;
	td.zone = TARGET_ZONE_HAND;
	td.illegal_abilities = 0;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! can_target(&td) ){
			ai_modifier-=1000;
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( can_target(&td) ){
			if( player != AI ){
				pick_target(&td, "TARGET_ARTIFACT");
			}
			else{
				instance->targets[0].card = select_a_card(player, player, 4, 0, 1, -1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			if( instance->targets[0].card != -1 ){
				instance->targets[9].player = get_cmc(player, instance->targets[0].card);
				instance->targets[9].card = get_id(player, instance->targets[0].card);
				rfg_card_in_hand(player, instance->targets[0].card);
				create_card_name_legacy(player, card, instance->targets[9].card);
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE && instance->targets[9].player != -1 ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, instance->targets[9].player, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, instance->targets[9].player, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( check_rfg(player, instance->targets[9].card) ){
			token_generation_t token;
			default_token_definition(player, card, instance->targets[9].card, &token);
			token.no_sleight = 1;
			generate_token(&token);
		}
	}

	return 0;
}

int card_psychic_miasma(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			ec_definition_t ec;
			default_ec_definition(instance->targets[0].player, player, &ec);

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			int result = new_effect_coercion(&ec, &this_test);
			if (result != -1 && is_what(-1, get_internal_card_id_from_csv_id(result), TYPE_LAND)){
				bounce_permanent(player, card);
			} else {
				kill_card(player, card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_quicksilver_gargantuan(int player, int card, event_t event)
{
  /* Quicksilver Gargantuan	|5|U|U
   * Creature - Shapeshifter 7/7
   * You may have ~ enter the battlefield as a copy of any creature on the battlefield, except it's still 7/7. */

  if (enters_the_battlefield_as_copy_of_any_creature(player, card, event))
	set_legacy_image(player, CARD_ID_QUICKSILVER_GARGANTUAN,
					 set_pt_and_abilities_until_eot(player, card, player, card, 7,7, 0,SP_KEYWORD_DOES_NOT_END_AT_EOT, 0));

  return 0;
}

int card_razor_hippogriff(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 ){
		int card_added = global_tutor(player, player, 2, TUTOR_HAND, 0, 2, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		if( card_added != -1 ){
			gain_life(player, get_cmc(player, card_added));
		}
	}
	return 0;
}

int card_relic_putrescence(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->targets[0].player != -1 && instance->targets[0].card != -1 ){
		if( event == EVENT_TAP_CARD && affect_me(instance->targets[0].player, instance->targets[0].card) ){
			poison_counters[instance->targets[0].player]++;
		}
	}
	else{
		return card_relic_bind(player, card, event);
	}
	return 0;
}

int card_revoke_existence(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ARTIFACT_ENCHANTMENT");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_riddlesmith(int player, int card, event_t event){

	if( smiths(player, card, event) ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}

	return 0;
}

int card_rust_tick(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );

	card_instance_t *instance = get_card_instance( player, card );

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && affect_me(player, card) ){
		int ai_choice = 1;
		if( instance->targets[1].player != -1 && instance->targets[1].card != -1 &&
			in_play(instance->targets[1].player, instance->targets[1].player) ){
			ai_choice = 0;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Leave this tapped\n Untap this", ai_choice);
		if( choice == 0 ){
			instance->untap_status &= ~3;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			does_not_untap_until_im_tapped(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *parent = get_card_instance( player, instance->parent_card );
			parent->targets[1].player = instance->targets[0].player;
			parent->targets[1].card = instance->targets[0].card;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_ARTIFACT");
}

int card_rusted_relic(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( metalcraft(player, card) ){
			int newtype = create_a_card_type(instance->internal_card_id);
			cards_at_7c7000[newtype]->type |= (cards_data[instance->internal_card_id].type | TYPE_CREATURE);
			cards_at_7c7000[newtype]->power = 5;
			cards_at_7c7000[newtype]->toughness = 5;
			instance->targets[12].card = newtype;
		}
		else{
			instance->targets[12].card = -1;
		}
		if( instance->targets[12].card > 0 ){
			event_result = instance->targets[12].card;
		}
	}

	return 0;
}

int card_saberclaw_golem(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 0, 0, 0, 0, 0, 1, 0, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_salvage_scout(int player, int card, event_t event){
	/* Salvage Scout	|W
	 * Creature - Human Scout 1/1
	 * |W, Sacrifice ~: Return target artifact card from your graveyard to your hand. */

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) && new_special_count_grave(player, &this_test) > 0 &&
		can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		return ! graveyard_has_shroud(2);
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1);
			if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int selected = validate_target_from_grave(player, card, player, 0);
			if( selected != -1 ){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
	}
	return 0;
}

int card_scrapdiver_serpent(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES && affect_me(player, card) && count_permanents_by_type(1-player, TYPE_ARTIFACT) > 0)
	unblockable(player, card, event);

  return 0;
}

int card_screeching_silcaw(int player, int card, event_t event){

	if( metalcraft(player, card) && has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		mill(1-player, 4);
	}

	return 0;
}

int card_seize_the_initiative(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
							1, 1, KEYWORD_FIRST_STRIKE, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_slice_in_twain(int player, int card, event_t event){
	/* Slice in Twain	|2|G|G
	 * Instant
	 * Destroy target artifact or enchantment.
	 * Draw a card. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ARTIFACT_ENCHANTMENT");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			draw_cards(player, 1);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_snapsail_glider(int player, int card, event_t event){

	if( metalcraft(player, card)  ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FLYING);
	}

	return 0;
}

int card_soul_parry(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->targets[1].player = instance->targets[0].player;
			instance->targets[1].card = instance->targets[0].card;
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
			if( can_target(&td) ){
				int choice = do_dialog(player, player, card, -1, -1, " Pick a second target\n Pass", 0);
				if( choice == 0 ){
					td.allow_cancel = 0;
					pick_target(&td, "TARGET_CREATURE");
				}
			}
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				negate_damage_this_turn(player, card, instance->targets[i].player, instance->targets[i].card, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_steady_progress(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL){
		proliferate(player, card);
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_stoic_rebuttal(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		if( metalcraft(player, card) ){
			COST_COLORLESS--;
		}
	}
	return card_counterspell(player, card, event);
}

int card_strata_scythe(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! can_target(&td) ){
			ai_modifier-=1000;
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		int id = -1;
		int card_added = -1;
		if( player == AI ){
			int ids[available_slots];
			int count = 0;
			while( count < available_slots ){
					ids[count] = 0;
					count++;
			}
			count = 0;
			while( count < active_cards_count[player] ){
					if( in_play(player, count) && is_what(player, count, TYPE_LAND) ){
						ids[get_id(player, count)]++;
					}
					count++;
			}
			count = 0;
			int par = 0;
			while( count < available_slots ){
					if( ids[count] > par ){
						par = ids[count];
						id = count;
					}
					count++;
			}
			if( id != -1 ){
				card_added = global_tutor(player, player, 1, TUTOR_HAND, 0, 1, TYPE_LAND, 0, 0, 0, 0, 0, id, 0, -1, 0);
			}
		}
		else{
			card_added = global_tutor(player, player, 1, TUTOR_HAND, 0, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}

		if( card_added != -1 ){
			id = get_id(player, card_added);
			rfg_card_in_hand(player, card_added);
			instance->targets[9].player = id;
			create_card_name_legacy(player, card, instance->targets[9].player );
		}
	}

	if( in_play(player, card) && is_equipping(player, card) ){
		if( instance->targets[9].player != -1 ){
			int amount = count_cards_by_id(player,instance->targets[9].player) +
						count_cards_by_id(1-player,instance->targets[9].player);
			modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, amount, amount, 0);
		}
	}

	return basic_equipment(player, card, event, 3);
}

int card_strider_harness(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 1, 1, 1, 0, SP_KEYWORD_HASTE);
}

int card_sunblast_angel(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int i;
		for(i = 0; i < 2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && is_tapped(i, count) ){
						kill_card(i, count, KILL_DESTROY);
					}
					count--;

			}
		}
	}
	return 0;
}

int card_sunspear_shikari(int player, int card, event_t event){
	if ((event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) &&
		!is_humiliated(player, card) && in_play(player, card) && equipments_attached_to_me(player, card, EATM_CHECK)
	   ){
		lifelink(player, card, event);
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FIRST_STRIKE);
	}

	return 0;
}

int card_sylvok_lifestaff(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && is_equipping(player, card) ){
		if( graveyard_from_play(instance->targets[8].player, instance->targets[8].card, event) ){
			gain_life(player, 3);
		}
	}

	return vanilla_equipment(player, card, event, 1, 1, 0, 0, 0);
}

int card_sylvok_replica(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 1, 0, 0, 0, &td, "DISENCHANT");
}

int card_tangle_angler(int player, int card, event_t event){
	return 0;
}

static int has_damaged_opponent(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[1].player < 1 ){
		return 0;
	}
	if( player == 0 && BYTE1(instance->targets[1].player) ){
		return 1;
	}
	if( player == 1 && BYTE0(instance->targets[1].player) ){
		return 1;
	}
	return 0;
}

int card_steel_hellkite(int player, int card, event_t event){
	/*
	  Steel Hellkite |6
	  Artifact Creature - Dragon 5/5
	  Flying
	  {2}: Steel Hellkite gets +1/+0 until end of turn.
	  {X}: Destroy each nonland permanent with converted mana cost X whose controller was dealt combat damage by Steel Hellkite this turn.
	  Activate this ability only once each turn.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	check_damage(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_TRACE_DAMAGED_PLAYERS, player, card);

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
		instance->targets[1].card = 0;
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	enum{
		CHOICE_PUMP = 1,
		CHOICE_WIPEOUT
	};


	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			if( has_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
				return 1;
			}
			if( (instance->targets[1].card > -1 && !(instance->targets[1].card & CHOICE_WIPEOUT)) ){
				if( player == HUMAN ){
					return 1;
				}
				else{
					if( has_damaged_opponent(player, card) &&
						has_mana_for_activated_ability(player, card, MANACOST_X(get_most_common_cmc_nonland(1-player)))
					  ){
						return 1;
					}
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int can_wipe = instance->targets[1].card > -1 && !(instance->targets[1].card & CHOICE_WIPEOUT);
		if( player == AI ){
			if( ! has_damaged_opponent(player, card) ){
				can_wipe = 0;
			}
			if( ! has_mana_for_activated_ability(player, card, MANACOST_X(get_most_common_cmc_nonland(1-player))) ){
				can_wipe = 0;
			}
		}
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Pump", has_mana_for_activated_ability(player, card, MANACOST_X(2)), (current_turn == player && is_attacking(player, card) && current_phase == PHASE_AFTER_BLOCKING ? 10 : 0),
							"Wipe out permanents", can_wipe, 5);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_PUMP ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
				instance->info_slot = choice;
			}
		}
		if( choice == CHOICE_WIPEOUT ){
			if( player == HUMAN ){
				if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
					charge_mana(player, COLOR_COLORLESS, -1);
					if( spell_fizzled != 1 ){
						instance->targets[1].card |= CHOICE_WIPEOUT;
						instance->targets[2].player = x_value;
						instance->info_slot = choice;
					}
				}
			}
			else{
				int ai_value = get_most_common_cmc_nonland(1-player);
				if( charge_mana_for_activated_ability(player, card, MANACOST_X(ai_value)) ){
					instance->targets[1].card |= CHOICE_WIPEOUT;
					instance->targets[2].player = ai_value;
					instance->info_slot = choice;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].card == CHOICE_PUMP ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, 0);
		}
		if( instance->targets[1].card == CHOICE_WIPEOUT ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.type_flag = DOESNT_MATCH;
			this_test.cmc = instance->targets[2].player;
			APNAP(p,{
						if( (p == 0 && BYTE0(instance->targets[1].player)) ||
							(p == 1 && BYTE1(instance->targets[1].player))
						  ){
							new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);
						};
					};
			);
		}
	}

	return 0;
}

int card_tel_jilad_defiance(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	if( player == AI ){
		td.allowed_controller = player;
		td.preferred_controller = player;
	}


	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_PROT_ARTIFACTS, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_tower_of_calamities(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td)  ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 12, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 8, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_trigon_of_corruption(int player, int card, event_t event)
{
  /* Trigon of Corruption	|4
   * Artifact
   * ~ enters the battlefield with three charge counters on it.
   * |B|B, |T: Put a charge counter on ~.
   * |2, |T, Remove a charge counter from ~: Put a -1/-1 counter on target creature. */

  target_definition_t td;
  if (IS_ACTIVATING(event))
	default_target_definition(player, card, &td, TYPE_CREATURE);

  switch (trigon(player, card, event, COLOR_BLACK, "-1/-1 counter", &td, "TARGET_CREATURE"))
	{
	  case 1:	return 1;
	  case 2:
		if (valid_target(&td))
		  {
			card_instance_t* instance = get_card_instance(player, card);
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_M1_M1);
		  }
	}
  return 0;
}

int card_trigon_of_infestation(int player, int card, event_t event)
{
  /* Trigon of Infestation	|4
   * Artifact
   * ~ enters the battlefield with three charge counters on it.
   * |G|G, |T: Put a charge counter on ~.
   * |2, |T, Remove a charge counter from ~: Put a 1/1 |Sgreen Insect creature token with infect onto the battlefield. */

  switch (trigon(player, card, event, COLOR_GREEN, "Insect token", NULL, NULL))
	{
	  case 1:	return 1;
	  case 2:
		;token_generation_t token;
		default_token_definition(player, card, CARD_ID_INSECT, &token);
		token.special_infos = 67;
		generate_token(&token);
	}
  return 0;
}

int card_trigon_of_mending(int player, int card, event_t event)
{
  /* Trigon of Mending	|2
   * Artifact
   * ~ enters the battlefield with three charge counters on it.
   * |W|W, |T: Put a charge counter on ~.
   * |2, |T, Remove a charge counter from ~: Target player gains 3 life. */

  target_definition_t td;
  if (IS_ACTIVATING(event))
	{
	  default_target_definition(player, card, &td, 0);
	  td.zone = TARGET_ZONE_PLAYERS;
	  td.preferred_controller = player;
	}

  switch (trigon(player, card, event, COLOR_WHITE, "Gain life", &td, "TARGET_PLAYER"))
	{
	  case 1:	return 1;
	  case 2:
		if (valid_target(&td))
		  gain_life(get_card_instance(player, card)->targets[0].player, 3);
	}
  return 0;
}

int card_trigon_of_rage(int player, int card, event_t event)
{
  /* Trigon of Rage	|2
   * Artifact
   * ~ enters the battlefield with three charge counters on it.
   * |R|R, |T: Put a charge counter on ~.
   * |2, |T, Remove a charge counter from ~: Target creature gets +3/+0 until end of turn. */

  target_definition_t td;
  if (IS_ACTIVATING(event))
	{
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;
	}

  if (event == EVENT_CHECK_PUMP && CAN_ACTIVATE(player, card, MANACOST_X(2)) && CAN_TAP(player, card) && count_counters(player, card, COUNTER_CHARGE))
	pumpable_power[player] += 3;

  switch (trigon(player, card, event, COLOR_RED, "+3/+0 until end of turn", &td, "TARGET_CREATURE"))
	{
	  case 1:	return 1;
	  case 2:
		if (valid_target(&td))
		  {
			card_instance_t* instance = get_card_instance(player, card);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3,0);
		  }
	}
  return 0;
}

int card_trigon_of_thought(int player, int card, event_t event)
{
  /* Trigon of Thought	|5
   * Artifact
   * ~ enters the battlefield with three charge counters on it.
   * |U|U, |T: Put a charge counter on ~.
   * |2, |T, Remove a charge counter from ~: Draw a card. */

  switch (trigon(player, card, event, COLOR_BLUE, "Draw a card", NULL, NULL))
	{
	  case 1:	return 1;
	  case 2:
		draw_a_card(player);
	}
  return 0;
}

int card_true_conviction(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES
	  && affected_card_controller == player
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && in_play(player, card) && !is_humiliated(player, card))
	{
	  event_result |= KEYWORD_DOUBLE_STRIKE;
	  lifelink(affected_card_controller, affected_card, event);
	}

  return global_enchantment(player, card, event);
}

int card_tunnel_ignus(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) &&
		player == reason_for_trigger_controller && 1-player == trigger_cause_controller
	  ){
		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( get_trap_condition(1-player, TRAP_LANDS_PLAYED) > 1 ){
						damage_player(1-player, 3, player, card);
					}
			}
		}
	}

	return 0;
}

int card_turn_aside(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			card_instance_t *instance = get_card_instance( card_on_stack_controller, card_on_stack );
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				if( instance->targets[i].player == player && instance->targets[i].card != -1 &&
					instance->targets[i].card != card_on_stack
				  ){
					return result;
				}
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			card_instance_t *instance = get_card_instance( player, card );
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			return card_counterspell(player, card, event);
	}
	else{
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_turn_to_slag(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 5;
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			equipments_attached_to_me(instance->targets[0].player, instance->targets[0].card, EATM_DESTROY);
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot,
							player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_twisted_image(int player, int card, event_t event)
{
  // Switch target creature's power and toughness until end of turn.
  // Draw a card.
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = ANYBODY;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  switch_power_and_toughness_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
		  draw_cards(player, 1);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_untamed_might(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
							instance->info_slot, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_vault_skyward(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_FLYING, 0);
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_vedalken_certarch(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ARTIFACT | TYPE_LAND );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && metalcraft(player, card) ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TWIDDLE");
	}

	else if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TWIDDLE");
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td)  ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
	}

	return 0;
}

int card_vensers_journal(int player, int card, event_t event){

	if( current_turn == player && event == EVENT_MAX_HAND_SIZE ){
		event_result = 99;
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		gain_life(player, hand_count[player]);
	}
	return 0;
}

int card_vigil_for_the_lost(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( affected_card_controller != player ){ return 0; }

		card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			affected->kill_code > 0 && affected->kill_code < 4 ) {
		   if( instance->targets[11].player < 0 ){
			  instance->targets[11].player = 0;
		   }
		   instance->targets[11].player++;
		}
	}

	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY &&
		player == reason_for_trigger_controller && affect_me(player, card )
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int choice = do_dialog(player, player, card, -1, -1, " Activate Vigil for the Lost\n Pass", 1);
				if( choice == 0 ){
					charge_mana(player, COLOR_COLORLESS, -1);
					if( spell_fizzled != 1 ){
						gain_life(player, x_value);
					}
				}
				instance->targets[11].player = 0;
		}
	}

	return global_enchantment(player, card, event);
}

int card_viridian_revel(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, 1-player, TYPE_ARTIFACT, NULL);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		draw_some_cards_if_you_want(player, card, player, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_volition_reins(int player, int card, event_t event)
{
  if (comes_into_play(player, card, event))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->damage_target_player >= 0 && instance->damage_target_card >= 0
		  && in_play(instance->damage_target_player, instance->damage_target_card)
		  && is_tapped(instance->damage_target_player, instance->damage_target_card))
		untap_card(instance->damage_target_player, instance->damage_target_card);
	}

  return card_confiscate(player, card, event);
}

int card_vulshok_heartstoker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	if( player == AI ){
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_state = TARGET_STATE_SUMMONING_SICK;
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 0);
		}
	}

	return 0;
}

int card_vulshok_replica(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, 3, player, card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 1, 0, 0, 0, 1, 0, 0, &td, "TARGET_PLAYER");
}

int card_wall_of_tanglecord(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 0, 0, 0, 1, 0, 0, 0, 0, KEYWORD_REACH, 0);
}

int card_whitesuns_passage(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 5;
	}

	if( event == EVENT_RESOLVE_SPELL){
		gain_life(player, instance->info_slot);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_wing_puncture(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = get_power(instance->targets[0].player, instance->targets[0].card);
			td.allowed_controller = 2;
			td.preferred_controller = 1-player;
			td.illegal_abilities = get_protections_from(player, card);
			td.required_abilities = KEYWORD_FLYING;
			pick_target(&td, "TARGET_CREATURE");
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td, 1) ){
			int amount = get_power(instance->targets[0].player, instance->targets[0].card);
			damage_creature(instance->targets[1].player, instance->targets[1].card, amount, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_withstand_death(int player, int card, event_t event)
{
  /* Withstand Death	|G
   * Instant
   * Target creature gains indestructible until end of turn. */

  return vanilla_instant_pump(player, card, event, ANYBODY, player, 0,0, 0,SP_KEYWORD_INDESTRUCTIBLE);
}

int card_palladium_myr(int player, int card, event_t event){
	// Was 0x424b30 (Sol Ring)
	return mana_producing_creature(player, card, event, 18, COLOR_COLORLESS, 2);
}
