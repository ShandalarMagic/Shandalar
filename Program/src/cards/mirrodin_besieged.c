#include "manalink.h"

// GLOBAL FUNCTIONS

static int battle_cry(int player, int card, event_t event)
{
  // Whenever this creature attacks, each other attacking creature gets +1/+0 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  test.not_me = 1;
	  pump_creatures_until_eot(player, card, current_turn, 0, 1,0, 0,0, &test);
	}

  return 0;
}


// CARDS

int card_thopter_assembly(int player, int card, event_t event){
	/* Thopter Assembly	|6
	 * Artifact Creature - Thopter 5/5
	 * Flying
	 * At the beginning of your upkeep, if you control no Thopters other than ~, return ~ to its owner's hand and put five 1/1 colorless Thopter artifact creature tokens with flying onto the battlefield. */

	if( current_turn == player && upkeep_trigger(player, card, event) ){
	 if( in_play(player, card) && count_subtype(player, TYPE_CREATURE, SUBTYPE_THOPTER) < 2 ){
		bounce_permanent(player, card);
		generate_tokens_by_id(player, card, CARD_ID_THOPTER, 5);
	 }
  }
  return 0;
}

int card_glissa_the_traitor(int player, int card, event_t event){

  deathtouch(player, card, event);

  check_legend_rule(player, card, event);

  card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) && affected_card_controller == 1-player){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && count_graveyard_by_type(player, TYPE_ARTIFACT) > 0){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code && affected->kill_code != KILL_REMOVE ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				instance->targets[11].player++;
			}
		}
	}

	if( instance->targets[11].player > 0 && resolve_graveyard_trigger(player, card, event) == 1 ){
		char buffer[100] = "Select an artifact card";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, buffer);
		int ca = count_graveyard_by_type(player, TYPE_ARTIFACT);
		while( ca && instance->targets[11].player ){
				if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test) != -1 ){
					ca--;
				}
				instance->targets[11].player--;
		}
		instance->targets[11].player = 0;
	}

	return 0;
}

int card_hero_of_bladehold( int player, int card, event_t event)
{
  // Battle cry (Whenever this creature attacks, each other attacking creature gets +1/+0 until end of turn.)
  // Whenever ~ attacks, put two 1/1 |Swhite Soldier creature tokens onto the battlefield tapped and attacking.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_SOLDIER, &token);
	  token.qty = 2;
	  token.action = TOKEN_ACTION_ATTACKING;
	  generate_token(&token);

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  test.not_me = 1;
	  pump_creatures_until_eot(player, card, current_turn, 0, 1,0, 0,0, &test);
	}

  return 0;
}

int card_tezzeret_agent_of_bolas(int player, int card, event_t event){

	/* Tezzeret, Agent of Bolas	|2|U|B
	 * Planeswalker - Tezzeret (3)
	 * +1: Look at the top five cards of your library. You may reveal an artifact card from among them and put it into your hand. Put the rest on the bottom of
	 * your library in any order.
	 * -1: Target artifact becomes a 5/5 artifact creature.
	 * -4: Target player loses X life and you gain X life, where X is twice the number of artifacts you control. */

	if (IS_ACTIVATING(event)){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.preferred_controller = player;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		int priority_steal_life = 0;
		if( event == EVENT_ACTIVATE ){
			int life_to_steal = count_subtype(player, TYPE_ARTIFACT, -1)*2;
			priority_steal_life = (life[player] < 6 && life[player]+life_to_steal > 6) ? 20 : 0;
			priority_steal_life += (life[1-player] - life_to_steal < 1) ? 20 : 0;
			priority_steal_life += ((count_counters(player, card, COUNTER_LOYALTY)*2)-8);
		}

		int priority_aa = 0;
		if( event == EVENT_ACTIVATE ){
			priority_aa += ((count_counters(player, card, COUNTER_LOYALTY)*15)-15);
		}

		enum{
			CHOICE_LOOK5 = 1,
			CHOICE_ANIMATE_ARTIFACT,
			CHOICE_STEAL_LIFE
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Look at the top 5", 1, 10, 1,
						"Animate an artifact", can_target(&td), priority_aa, -1,
						"My artifacts, your life", can_target(&td1), priority_steal_life, -4);

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
				case CHOICE_ANIMATE_ARTIFACT:
					pick_target(&td, "TARGET_ARTIFACT");
					break;

				case CHOICE_LOOK5:
					break;

				case CHOICE_STEAL_LIFE:
					pick_target(&td1, "TARGET_PLAYER");
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_LOOK5:
				reveal_x_and_choose_a_card_type( instance->parent_controller, instance->parent_card, 5, TYPE_ARTIFACT);
				break;

			case CHOICE_ANIMATE_ARTIFACT:
				{
					if( valid_target(&td) ){
						artifact_animation(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
											0, 5, 5, 0, 0);
					}
				}
				break;

			case CHOICE_STEAL_LIFE:
				{
					if( valid_target(&td1) ){
						int count = count_subtype(player, TYPE_ARTIFACT, -1);
						gain_life(player, count*2);
						lose_life(instance->targets[0].player, count*2);
					}
				}
				break;
		}
	}

	return planeswalker(player, card, event, 3);
}

int card_sword_of_feast_and_famine(int player, int card, event_t event)
{
  /* Sword of Feast and Famine	|3
   * Artifact - Equipment
   * Equipped creature gets +2/+2 and has protection from |Sblack and from |Sgreen.
   * Whenever equipped creature deals combat damage to a player, that player discards a card and you untap all lands you control.
   * Equip |2 */

  if (event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card))
		{
		  if (event == EVENT_ABILITIES)
			event_result |= get_sleighted_protection(player, card, KEYWORD_PROT_BLACK | KEYWORD_PROT_GREEN);
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
			discard(p, 0, player);
			manipulate_type(player, card, player, TYPE_LAND, ACT_UNTAP);
		  }
	}

  return basic_equipment(player, card, event, 2);
}

int card_viridian_emissary(int player, int card, event_t event ){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		tutor_basic_land(player, 1, 1);
	}
	return 0;
}


int card_blightsteel_colossus(int player, int card, event_t event)
{
  /* Blightsteel Colossus	|12
   * Artifact Creature - Golem 11/11
   * Trample, infect
   * ~ is indestructible.
   * If ~ would be put into a graveyard from anywhere, reveal ~ and shuffle it into its owner's library instead. */

  // Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

  indestructible(player, card, event);
  return 0;
}


int card_leonin_relic_warder(int player, int card, event_t event){

   card_instance_t *instance = get_card_instance(player, card);

		return_from_oblivion(player, card, event);

	   if( comes_into_play(player, card, event) ){
		   target_definition_t td;
		   default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);
		   td.allowed_controller = 2;
		   td.preferred_controller = 1-player;
		   if( can_target(&td) ){
				if( pick_target(&td, "DISENCHANT") ){
				   obliviation(player, card, instance->targets[0].player, instance->targets[0].card);
				}
		   }
	   }

	return 0;
}

int card_red_suns_zenith(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( instance->targets[0].card > -1 ){
				exile_if_would_be_put_into_graveyard(player, card, instance->targets[0].player, instance->targets[0].card, 1);
			}
			damage_target0(player, card, instance->info_slot);
			if( ! is_token(player, card) ){
				shuffle_into_library(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_blue_suns_zenith(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_PLAYERS;
  td.allow_cancel =  0;

  card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

  else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		  if( spell_fizzled != 1 ){
			 instance->info_slot = x_value;
			 pick_target(&td, "TARGET_PLAYER");
		  }
  }

  else if( event == EVENT_RESOLVE_SPELL ){
		  if( valid_target(&td) ){
			 draw_cards(instance->targets[0].player, instance->info_slot);
		  }
		  shuffle_into_library(player, card);

  }

  return 0;
}

int card_black_suns_zenith(int player, int card, event_t event){

	/* Black Sun's Zenith	|X|B|B
	 * Sorcery
	 * Put X -1/-1 counters on each creature. Shuffle ~ into its owner's library. */

	if( event == EVENT_CAN_CAST){
		return !check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			get_card_instance(player, card)->info_slot = x_value;
		}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		manipulate_type(player, card, ANYBODY, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_M1_M1, get_card_instance(player, card)->info_slot));
		shuffle_into_library(player, card);
	}

	return 0;
}

int card_green_suns_zenith(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		x_value = 0; //For avoiding problems with creatures with X in the CMC cost.
		char msg[100];
		scnprintf(msg, 100, "Select a Green creature card with CMC %d or less", instance->info_slot);
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.color = COLOR_TEST_GREEN;
		this_test.cmc = instance->info_slot+1;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		shuffle_into_library(player, card);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_white_suns_zenith(int player, int card, event_t event){
	/* White Sun's Zenith	|X|W|W|W
	 * Instant
	 * Put X 2/2 |Swhite Cat creature tokens onto the battlefield. Shuffle ~ into its owner's library. */

  card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

  else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		  if( spell_fizzled != 1 ){
			 instance->info_slot = x_value;
		  }
  }

  else if( event == EVENT_RESOLVE_SPELL ){
		   int amount = instance->info_slot;
		   if( amount > 0 ){
					generate_tokens_by_id(player, card, CARD_ID_CAT, amount);
		   }
		   shuffle_into_library(player, card);
  }

  return 0;
}

int card_phyrexian_vatmother(int player, int card, event_t event ){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		poison_counters[player]++;
	}

	return 0;
}

int card_victorys_herald(int player, int card, event_t event)
{
  // Whenever ~ attacks, attacking creatures gain flying and lifelink until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  pump_creatures_until_eot(player, card, current_turn, 0, 0,0, KEYWORD_FLYING,SP_KEYWORD_LIFELINK, &test);
	}

  return 0;
}

int card_steel_sabotage(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance( player, card);

	if( event != EVENT_RESOLVE_SPELL && event != EVENT_CAST_SPELL ){
		card_counterspell(player, card, event);
	}

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 && is_what(card_on_stack_controller, card_on_stack, TYPE_ARTIFACT) ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			return result;
		}
		else{
			return can_target(&td);
		}
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			if( instance->targets[0].player != -1 ){
				if( can_target(&td) ){
					choice = do_dialog(player, player, card, -1, -1, " Counter an artifact spell\n Bounce an artifact", 0);
				}
			}
			else{
				 choice = 1;
			}

			if( choice == 1){
				if( pick_target(&td, "TARGET_ARTIFACT") ){
					instance->info_slot = 1;
				}
				else{
					 spell_fizzled = 1;
				}
			}
			else{
				 instance->info_slot = 0;
				 card_counterspell(player, card, event);
			}
	}
	else if(event == EVENT_RESOLVE_SPELL){
			int choice = instance->info_slot;

			// boomerang target if necessary
			if( choice == 1 ){
				if( valid_target(&td) ){
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				}
			}
			// counter a spell if necessary
			else if( choice == 0 ){
					set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
					card_counterspell(player, card, event);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_treasure_mage(int player, int card, event_t event ){

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		global_tutor(player, player, 1, TUTOR_HAND, 0, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, 5, 2);
	}

	return 0;
}


int card_sangromancer(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if (discard_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_DUH, 0)){
		gain_life(player, 3);
	}

	count_for_gfp_ability(player, card, event, 1-player, TYPE_CREATURE, NULL);

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && affect_me(player, card) && reason_for_trigger_controller == player &&
		instance->kill_code < KILL_DESTROY
	  ){
		if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
			gain_life(player, 3*instance->targets[11].card);
			instance->targets[11].card = 0;
		}
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		gain_life(player, 3*instance->targets[11].player);
	}

	return 0;
}


int card_phyrexian_hydra(int player, int card, event_t event ){

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_source_player == player &&  damage->damage_source_card == card && damage->info_slot > 0 ){
			   int quantity = damage->info_slot;
			   int poisoning = 0;
			   if( get_abilities(damage->damage_source_player, damage->damage_source_card, EVENT_ABILITIES, -1) & KEYWORD_TRAMPLE){
				  if( get_toughness(damage->damage_target_player, damage->damage_target_card) < quantity){
					 poisoning = quantity - get_toughness(damage->damage_target_player, damage->damage_target_card);
					 quantity-=poisoning;
				  }
				}
				add_minus1_minus1_counters(damage->damage_target_player, damage->damage_target_card, quantity);
				if( poisoning > 0){
				   poison_counters[1-player] += poisoning;
				}
				damage->info_slot = 0;
			}
			else if( damage->damage_target_player == 1-player && damage->damage_target_card == -1 && damage->damage_source_player == player &&  damage->damage_source_card == card && damage->info_slot > 0 ){
					poison_counters[1-player] += damage->info_slot;
					damage->info_slot = 0;
			}

			else if( damage->damage_target_player == player && damage->damage_target_card == card &&
					 damage->info_slot > 0 ){
					 add_minus1_minus1_counters(damage->damage_target_player, damage->damage_target_card, damage->info_slot);
					 damage->info_slot = 0;
			}
		}
	}

 return 0;
}

int card_rot_wolf(int player, int card, event_t event)
{
  if (sengir_vampire_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)))
	draw_cards(player, get_card_instance(player, card)->targets[11].card);

  return 0;
}

int card_ichor_wellspring(int player, int card, event_t event ){

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}

	if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)){
		draw_cards(player, 1);
	}

	return 0;
}

int card_signal_pest(int player, int card, event_t event ){

	battle_cry(player, card, event);

	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		ai_defensive_modifier -= 12;
	}

	return card_orchard_spirit(player, card, event);
}

static int effect_hero_of_oxid_ridge(int player, int card, event_t event)
{
  if (event == EVENT_BLOCK_LEGALITY
	  && get_power(affected_card_controller, affected_card) <= 1)
	event_result = 1;

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_hero_of_oxid_ridge(int player, int card, event_t event)
{
  haste(player, card, event);

  battle_cry(player, card, event);

  // Whenever ~ attacks, creatures with power 1 or less can't block this turn.
  if (event == EVENT_DECLARE_ATTACKERS && is_attacking(player, card))
	create_legacy_effect(player, card, &effect_hero_of_oxid_ridge);

  return 0;
}

int card_creeping_corrosion(int player, int card, event_t event ){

	if( event == EVENT_CAN_CAST && affect_me(player, card) ){
	   return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			manipulate_all(player, card, player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
			manipulate_all(player, card, 1-player,TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
			kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_go_for_the_throat(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_type = TYPE_ARTIFACT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && affect_me(player, card) && can_target(&td) ){
	   return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( spell_fizzled !=1 ){
			   pick_target(&td, "TARGET_CREATURE");
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_vivisection(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;
	td.illegal_abilities = 0;

	if( event == EVENT_CAN_CAST && affect_me(player, card) && can_target(&td) ){
	   return  can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0)  ){
				spell_fizzled =1;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			draw_cards(player, 3);
			kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_thrun_the_last_troll(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	cannot_be_countered(player, card, event);

	hexproof(player, card, event);

	return regeneration(player, card, event, 1, 0, 0, 1, 0, 0);
}


int card_spine_of_ish_sah(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_PERMANENT") ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	if (graveyard_from_play(player, card, event)){
		bounce_permanent(player, card);
		event_result = 1;
	}

	return 0;
}

int card_turn_the_tide(int player, int card, event_t event){

  if( event == EVENT_CAN_CAST){
	 return 1;
  }

  else if( event == EVENT_RESOLVE_SPELL ){
		  pump_subtype_until_eot(player, card, 1-player, -1, -2, 0, 0, 0);
		  kill_card(player, card, KILL_DESTROY);
  }

  return 0;
}

int card_massacre_wurm(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		pump_subtype_until_eot(player, card, 1-player, -1, -2, -2, 0, 0);
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, 1-player, TYPE_CREATURE, NULL);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		lose_life(1-player, instance->targets[11].card*2);
		instance->targets[11].card = 0;
	}
	return 0;
}

int card_ardent_recruit(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && metalcraft(player, card) ){
	   event_result+=2;
	}

	return 0;
}

int card_galvanoth(int player, int card, event_t event){

	/* Galvanoth	|3|R|R
	 * Creature - Beast 3/3
	 * At the beginning of your upkeep, you may look at the top card of your library. If it's an instant or sorcery card, you may cast it without paying its
	 * mana cost. */

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int* deck = deck_ptr[player];
		if( deck[0] != -1 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_SPELL, "You could play this card if it is an instant or sorcery.");
			this_test.create_minideck = 1;

			int selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_FIRST_FOUND, -1, &this_test);
			if( selected != -1 ){
				play_card_in_deck_for_free(player, player, selected);
			}
		}
	}

	return 0;
}

int card_flayer_husk(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_equipping(player, card) ){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 1, 1, 0);
	}

	return living_weapon(player, card, event, 2);
}

int card_sphere_of_the_suns(int player, int card, event_t event)
{
  /* Sphere of the Suns	|2
   * Artifact
   * ~ enters the battlefield tapped and with three charge counters on it.
   * |T, Remove a charge counter from ~: Add one mana of any color to your mana pool. */

  comes_into_play_tapped(player, card, event);
  enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, 3);

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_TAP_FOR_MANA(player, card) && count_counters(player, card, COUNTER_CHARGE) > 0;

  if (event == EVENT_ACTIVATE && produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, 1))
	remove_counter(player, card, COUNTER_CHARGE);

  if (event == EVENT_COUNT_MANA && affect_me(player, card)
	  && CAN_TAP_FOR_MANA(player, card) && count_counters(player, card, COUNTER_CHARGE) > 0)
	declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, 1);

  return 0;
}

int card_inkmoth_nexus(int player, int card, event_t event){
	return manland_normal(player, card, event, 1, 0, 0, 0, 0, 0);
}

int card_infective_blinkmoth(int player, int card, event_t event){
	return manland_animated(player, card, event, 1, 0, 0, 0, 0, 0);
}

int card_titan_forge(int player, int card, event_t event ){

	/* Titan Forge	|3
	 * Artifact
	 * |3, |T: Put a charge counter on ~.
	 * |T, Remove three charge counters from ~: Put a 9/9 colorless Golem artifact creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
			if( IS_AI(player) ){
				if( count_counters(player, card, COUNTER_CHARGE) >= 3 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					return 1;
				}
			}
			else{
				if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
					return 1;
				}
				if( count_counters(player, card, COUNTER_CHARGE) >= 3 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					return 1;
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
			if( count_counters(player, card, COUNTER_CHARGE) >= 3 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Add counter\n Make a Golem\n Do nothing", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 3-(3*choice), 0, 0, 0, 0, 0) ){
				instance->info_slot = 66+choice;
				if( choice == 1 ){
					remove_counters(player, card, COUNTER_CHARGE, 3);
				}
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			add_counter(player, card, COUNTER_CHARGE);
		}
		else if( instance->info_slot == 67 ){
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_GOLEM, &token);
				token.pow = 9;
				token.tou = 9;
				generate_token(&token);
		}
	}

	if( player == AI && current_turn != player && has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) && ! is_tapped(player, card) &&
		! is_animated_and_sick(player, card) && eot_trigger(player, card, event)
	   ){
		 charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0);
		 if( spell_fizzled != 1 ){
			tap_card(player, card);
			add_counter(player, card, COUNTER_CHARGE);
		 }
	}

	return 0;
}

int card_mortarpod(int player, int card, event_t event){

	/* Mortarpod	|2
	 * Artifact - Equipment
	 * Living weapon
	 * Equipped creature gets +0/+1 and has "Sacrifice this creature: This creature deals 1 damage to target creature or player."
	 * Equip |2 */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ABILITIES && affect_me(player, card) && is_equipping(player, card) ){
		// Mimic the special abilities of the equipped creature.
		int result = get_special_abilities(instance->targets[8].player, instance->targets[8].card);
		if( result > 0 ){
			special_abilities(player, card, event, result & ~SP_KEYWORD_LURE, player, card);
		}
		if( check_for_ability(instance->targets[8].player, instance->targets[8].card, KEYWORD_INFECT) ){
			event_result |= KEYWORD_INFECT;
		}
	}

	if (event == EVENT_TOUGHNESS && is_equipping(player, card) && affect_me(instance->targets[8].player, instance->targets[8].card)){
		event_result++;
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( is_equipping(player, card) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			int p = instance->targets[8].player;
			int c = instance->targets[8].card;
			td1.illegal_abilities = get_protections_from(p, c);
			return can_target(&td1);
		}
		return can_activate_basic_equipment(player, card, event, 2);
	}
	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( can_activate_basic_equipment(player, card, event, 2) ){
			if( is_equipping(player, card) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				int p = instance->targets[8].player;
				int c = instance->targets[8].card;
				td1.illegal_abilities = get_protections_from(p, c);
				if( can_target(&td1) ){
					choice = do_dialog(player, player, card, -1, -1, " Equip\n Sac & damage\n Cancel", 1);
				}
			}
		}
		else{
			td1.illegal_abilities = get_protections_from(instance->targets[8].player, instance->targets[8].card);
			choice = 1;
		}
		if( choice == 0 ){
			activate_basic_equipment(player, card, 2);
			instance->info_slot = 66;
		}
		else if( choice == 1 ){
				if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
					instance->number_of_targets = 1;
					instance->info_slot = 67;
					kill_card(instance->targets[8].player, instance->targets[8].card, KILL_SACRIFICE);
				}
				else{
					spell_fizzled = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			resolve_activation_basic_equipment(player, card);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, instance->parent_controller, instance->parent_card);
		}
	}

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GERM, &token);
		token.action = TOKEN_ACTION_EQUIP;
		generate_token(&token);
	}

	return 0;
}

int lead_the_stampede_effect(int player, int csvid, int amount){
	int *deck = deck_ptr[player];
	int result = 0;
	if( amount > count_deck(player) ){
		amount = count_deck(player);
	}
	if( amount > 0 ){
		char buf[200];
		sprintf(buf, "Cards revealed by %s", cards_ptr[csvid]->full_name);
		show_deck( HUMAN, deck, amount, buf, 0, 0x7375B0 );
		int i = amount-1;
		while( i > -1 ){
				if( is_what(-1, deck[i], TYPE_CREATURE) ){
					add_card_to_hand(player, deck[i]);
					remove_card_from_deck(player, i);
					result++;
					amount--;
				}
				i--;
		}
		if( amount > 0 ){
			put_top_x_on_bottom(player, player, amount);
		}
	}
	return result;
}


int card_lead_the_stampede(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			lead_the_stampede_effect(player, CARD_ID_LEAD_THE_STAMPEDE, 5);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_slagstorm(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 3;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int ai_choice = 0;
			if( life[1-player] < 4 && life[player] > 3 ){
				ai_choice = 1;
			}
			int choice = do_dialog(player, player, card, -1, -1, " Damage creatures\n Damage players", ai_choice);
			if( choice == 0 ){
				damage_all(player, card, player, instance->info_slot, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				damage_all(player, card, 1-player, instance->info_slot, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			if( choice == 1 ){
				damage_player(player, instance->info_slot, player, card);
				damage_player(1-player, instance->info_slot, player, card);
			}
			kill_card(player, card, KILL_DESTROY);
  }

  return 0;
}

int card_septic_rats(int player, int card, event_t event )
{
  // Whenever ~ attacks, if defending player is poisoned, it gets +1/+1 until end of turn.
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && poison_counters[1-player] > 0
	  && declare_attackers_trigger(player, card, event, 0, player, card))
	pump_until_eot(player, card, player, card, 1, 1);

  return 0;
}

int card_accorder_paladin(int player, int card, event_t event ){

	battle_cry(player, card, event);

	return 0;
}

int card_banishment_decree(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PERMANENT");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bladed_sentinel(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, SP_KEYWORD_VIGILANCE);
}

int card_blisterstick_shaman(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( comes_into_play(player, card, event) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, 1);
		}
	}
	return 0;
}

int card_bonehoard(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		int amount = count_graveyard_by_type(HUMAN, TYPE_CREATURE) + count_graveyard_by_type(AI, TYPE_CREATURE);
		if( amount < 1 ){
			ai_modifier-=1000;
		}
	}

	if( is_equipping(player, card) ){
		int amount = count_graveyard_by_type(HUMAN, TYPE_CREATURE) + count_graveyard_by_type(AI, TYPE_CREATURE);
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, amount, amount, 0);
	}

	return living_weapon(player, card, event, 2);
}

int card_brass_squire(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.required_subtype = SUBTYPE_EQUIPMENT;
	td.allowed_controller = player;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && can_target(&td) ){
			return can_target(&td1);
		}
	}

	else if( event == EVENT_ACTIVATE){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_EQUIPMENT") ){
				instance->targets[1].player = instance->targets[0].player;
				instance->targets[1].card = instance->targets[0].card;
				pick_target(&td1, "TARGET_CREATURE");
				tap_card(player, card);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION){
			card_instance_t *parent = get_card_instance(player, instance->parent_card);
			if( validate_target(player, card, &td, 1) && validate_target(player, card, &td1, 0) ){
				equip_target_creature(instance->targets[1].player, instance->targets[1].card,
									  instance->targets[0].player, instance->targets[0].card);
			}
			parent->number_of_targets = 1;
	}

	return 0;
}

int card_burn_the_impure(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 3;
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if (check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_INFECT)){
				damage_player(instance->targets[0].player, 3, player, card);
			}
			damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_caustic_hound(int player, int card, event_t event){

	if( graveyard_from_play(player, card, event) ){
		int i;
		for(i=0; i<2; i++){
			lose_life(i, 4);
		}
	}

	return 0;
}

int card_chocking_fumes(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	if( event == EVENT_CAN_CAST ){
		if( player == AI ){
			if( current_phase > PHASE_DECLARE_ATTACKERS && current_phase < PHASE_MAIN2 ){
				return can_target(&td);
			}
		}
		else{
			 return 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int count = active_cards_count[current_turn]-1;
		while( count > -1 ){
				if( in_play(current_turn, count) && is_what(current_turn, count, TYPE_CREATURE) &&
					is_attacking(current_turn, count)
				  ){
					add_minus1_minus1_counters(current_turn, count, 1);
				}
				count--;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_concussive_bolt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 4;
		if( player == AI && current_phase == PHASE_MAIN1 ){
			ai_modifier+=200;
		}
		pick_target(&td, "TARGET_PLAYER");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, 4, player, card);
			if( metalcraft(player, card) ){
				pump_subtype_until_eot(player, card, 1-player, -1, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_consecrated_sphinx(int player, int card, event_t event)
{
  /* Consecrated Sphinx	|4|U|U
   * Creature - Sphinx 4/6
   * Flying
   * Whenever an opponent draws a card, you may draw two cards. */

  if (card_drawn_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_AI(player)))
	draw_cards(player, 2);

  return 0;
}

int card_contested_war_zone(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );
	card_instance_t* damage = combat_damage_being_dealt(event);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	else if( damage
			 && damage->damage_target_player == player
			 && damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER)
			 && (damage->targets[3].player & TYPE_CREATURE)	// probably redundant to status check
			 && damage->damage_source_player != player
		   ){
			give_control_of_self(player, card);
	}

	else if( event == EVENT_ACTIVATE && affect_me(player, card) ){
			int choice = 0;
			if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_X(2)) && can_use_activated_abilities(player, card) ){
				int ai_choice = (current_turn == AI && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2) ? 1 : 0;
				choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Attacking creatures get +1/+0\n Cancel", ai_choice);
			}

			if( choice == 0){
				if (current_turn == AI){
					ai_modifier -= 12;
				}
				return mana_producer(player, card, event);
			}

			if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, MANACOST_X(1));
				if( spell_fizzled != 1){
					instance->info_slot = 1;
				}
				else{
					untap_card_no_event(player, card);
				}
			}

			if( choice == 2 ){
				cancel = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance(player, instance->parent_card);
				parent->info_slot = 0;
				int count = active_cards_count[current_turn]-1;
				while( count > -1 ){
						if( in_play(current_turn, count) && is_what(current_turn, count, TYPE_CREATURE) &&
							is_attacking(current_turn, count)
						){
							pump_until_eot(player, card, current_turn, count, 1, 0);
						}
						count--;
				}
			}
	}
	else{
		 return mana_producer(player, card, event);
	}

	return 0;
}

int card_copper_carapace(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 3, 2, 2, 0, SP_KEYWORD_CANNOT_BLOCK);
}

int card_core_prowler(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		proliferate(player, card);
	}

	return 0;
}

int card_corrupted_conscience(int player, int card, event_t event)
{
  /* Corrupted Conscience	|3|U|U
   * Enchantment - Aura
   * Enchant creature
   * You control enchanted creature.
   * Enchanted creature has infect. */

  card_instance_t* instance;
  if (event == EVENT_ABILITIES && (instance = in_play(player, card)) && affect_me(instance->damage_target_player, instance->damage_target_card))
	event_result |= KEYWORD_INFECT;

  return card_control_magic(player, card, event);
}

int card_crush(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.illegal_type = TYPE_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ARTIFACT");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int cryptoplasm_legacy(int player, int card, event_t event){

	if (effect_follows_control_of_attachment(player, card, event)){
		return 0;
	}

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){

		card_instance_t *instance = get_card_instance(player, card);
		int p = instance->damage_target_player, c = instance->damage_target_card;

		target_definition_t td;
		default_target_definition(p, c, &td, TYPE_CREATURE);
		td.preferred_controller = ANYBODY;
		td.special = TARGET_SPECIAL_NOT_ME;

		load_text(0, "TARGET_ANOTHER_CREATURE");
		if (select_target(player, card, &td, text_lines[0], NULL)){
			cloning_and_verify_legend(p, c, instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}
int card_cryptoplasm(int player, int card, event_t event){

	/* Cryptoplasm	|1|U|U
	 * Creature - Shapeshifter 2/2
	 * At the beginning of your upkeep, you may have ~ become a copy of another target creature. If you do, ~ gains this ability. */

	cloning_card(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		create_targetted_legacy_effect(player, card, &cryptoplasm_legacy, player, card);
	}

	return 0;
}

int card_darksteel_plate(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	indestructible(player, card, event);

	if( is_equipping(player, card) ){
		indestructible(instance->targets[8].player, instance->targets[8].card, event);
	}

	return basic_equipment(player, card, event, 2);
}

int card_decimator_web(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1 - player;
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 2);
			poison_counters[instance->targets[0].player]++;
			mill(instance->targets[0].player, 6);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 4, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_distant_memories(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int card_added = global_tutor(player, player, 1, TUTOR_HAND, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( 1-player == HUMAN ){
				do_dialog(1-player, player, card_added, -1, -1, "Opponent selected this card", 0);
			}
			card_instance_t *instance = get_card_instance(player, card_added);
			int card_selected = instance->internal_card_id;
			rfg_card_in_hand(player, card_added);
			int ai_choice = 0;
			if( get_base_value(-1, card_selected) > 90 ){
				ai_choice = 1;
			}
			int choice = do_dialog(1-player, player, card, -1, -1, " Opponent gets that card\n Opponent draws 3 cards", ai_choice);
			if( choice == 0 ){
				remove_card_from_rfg(player, cards_data[card_selected].id);
				add_card_to_hand(player, card_selected);
			}
			else{
				 draw_cards(player, 3);
			}
			kill_card(player, card, KILL_DESTROY);
  }

  return 0;
}

int card_dross_ripper(int player, int card, event_t event){

	return generic_shade(player, card, event, 0, 2, 1, 0, 0, 0, 0, 1, 1, 0, 0);
}

int card_fangren_marauder(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_ARTIFACT, NULL);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int amount = instance->targets[11].card;
		gain_life(player, amount*5);
	}

	return 0;
}

int card_frantic_salvage(int player, int card, event_t event){

	char msg[100] = "Select an artifact card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			while( new_special_count_grave(player, &this_test) &&
					new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_DECK, 0, AI_MAX_VALUE, &this_test) != -1
			   ){
			}
			draw_cards(player, 1);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fuel_for_the_cause(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		proliferate(player, card);
		return card_counterspell(player, card, event);
	}
	else{
		return card_counterspell(player, card, event);
	}

	return 0;
}

int card_gnathosaur(int player, int card, event_t event){
	/* Gnathosaur	|4|R|R
	 * Creature - Lizard 5/4
	 * Sacrifice an artifact: ~ gains trample until end of turn. */
	return generic_husk(player, card, event, TYPE_ARTIFACT, 0, 0, KEYWORD_TRAMPLE, 0);
}

int card_gore_vassal(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
			if( in_play(instance->targets[0].player, instance->targets[0].card) &&
				get_toughness(instance->targets[0].player, instance->targets[0].card) > 0 &&
				can_regenerate(instance->targets[0].player, instance->targets[0].card)
			  ){
				regenerate_target( instance->targets[0].player, instance->targets[0].card );
			}
		}
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME+GAA_REGENERATION, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_gruesome_encore(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	if(event == EVENT_RESOLVE_SPELL){
		int selected = validate_target_from_grave_source(player, card, 1-player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, card, 1-player, selected, REANIMATE_UNEARTH);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_GRAVE_RECYCLER_OPP_GRAVE, NULL, NULL, 1, &this_test);
}

int card_gust_skimmer(int player, int card, event_t event){

	return generic_shade(player, card, event, 0, 0, 0, 1, 0, 0, 0, 0, 0, KEYWORD_FLYING, 0);
}

int card_hellkite_igniter(int player, int card, event_t event){

	haste(player, card, event);

	int amount = (event == EVENT_RESOLVE_ACTIVATION || event == EVENT_POW_BOOST) ? count_permanents_by_type(player, TYPE_ARTIFACT) : 0;

	return generic_shade(player, card, event, 0, 1, 0, 0, 0, 1, 0, amount, 0, 0, 0);
}

int card_horryfing_revelation(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			discard(instance->targets[0].player, 0, player);
			mill(instance->targets[0].player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_into_the_core(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_REMOVE);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 2, NULL);
}

int card_kembas_legion(int player, int card, event_t event)
{
  vigilance(player, card, event);

  if (trigger_condition == TRIGGER_BLOCKER_CHOSEN)	// Avoid counting if we can
	creature_can_block_additional(player, card, event, equipments_attached_to_me(player, card, EATM_REPORT_TOTAL));

  return 0;
}

int card_kuldotha_flamefiend(int player, int card, event_t event){
	/* Kuldotha Flamefiend	|4|R|R
	 * Creature - Elemental 4/4
	 * When ~ enters the battlefield, you may sacrifice an artifact. If you do, ~ deals 4 damage divided as you choose among any number of target creatures and/or players. */

	if( comes_into_play(player, card, event) ){
		if( controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, 0) ){
			target_and_divide_damage(player, card, NULL, NULL, 4);
		}
	}

	return 0;
}

int card_kuldotha_ringleader(int player, int card, event_t event){

	attack_if_able(player, card, event);

	battle_cry(player, card, event);

	return 0;
}

int card_magnetic_mine(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_GRAVEYARD_FROM_PLAY && ! affect_me(player, card) ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
		if( is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) && affected->kill_code > 0 &&
			affected->kill_code < 4
		  ){
			if( instance->targets[11].player < 0 ){
				instance->targets[11].player = 0;
			}
			int pos = instance->targets[11].player;
			instance->targets[pos].player = affected_card_controller;
			instance->targets[11].player++;
		}
	}

	if( instance->targets[11].player > 0 && resolve_graveyard_trigger(player, card, event) == 1 ){
		int i;
		for(i=0; i<instance->targets[11].player; i++){
			if( instance->targets[i].player != -1 ){
				damage_player(instance->targets[i].player, 2, player, card);
				instance->targets[i].player = -1;
			}
		}
		instance->targets[11].player = 0;
	}
	return 0;
}

int card_masters_call(int player, int card, event_t event){
	/* Master's Call	|2|W
	 * Instant
	 * Put two 1/1 colorless Myr artifact creature tokens onto the battlefield. */

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			generate_tokens_by_id(player, card, CARD_ID_MYR, 2);
			kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_metallic_mastery(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ARTIFACT");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mirran_mettle(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.allowed_controller = player;
		td.preferred_controller = player;
	}
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( metalcraft(player, card) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 4, 4);
			}
			else{
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mirran_spy(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.required_state = TARGET_STATE_TAPPED;
	}
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( can_target(&td) && trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		reason_for_trigger_controller == player && player == trigger_cause_controller){
		int trig = 0;
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_ARTIFACT) &&
			! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND)
		  ){
			trig = 1;
		}
		if( trig == 1 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( pick_target(&td, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						untap_card(instance->targets[0].player, instance->targets[0].card);
					}
			}
		}
	}
	return 0;
}

static void mirrorworks_set_processing(token_generation_t* token, int card, int number)
{
  /* This prevents the following scenario:
   * 1. Mirrorworks (A) in play.
   * 2. Cast another Mirrorworks (B) (or, as originally formulated in the bug, a Copy Artifact or Sculpting Steel).
   * 3. Mirrorworks A triggers and puts a token Mirrorworks (C) in play.
   * 4. Newly-created Mirrorworks token C triggers on B entering play, since we're still resolving its come-into-play triggers, and creates another token
   *    Mirrorworks (D).
   * 5. Newly-created Mirrorworks token D triggers on B entering play, since we're still resolving its come-into-play triggers, and creates another token
   *    Mirrorworks (E).
   * 6. Repeat until out of mana.
   * See bug #12160. */
  get_card_instance(token->t_player, card)->state |= STATE_PROCESSING;
}

int card_mirrorworks(int player, int card, event_t event)
{
  /* Mirrorworks	|5
   * Artifact
   * Whenever another nontoken artifact enters the battlefield under your control, you may pay |2. If you do, put a token that's a copy of that artifact onto
   * the battlefield. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ARTIFACT, "");
	  test.type_flag = F1_NO_TOKEN;
	  test.not_me = 1;

	  int p = trigger_cause_controller, c = trigger_cause;

	  if (has_mana(player, COLOR_COLORLESS, 2)
		  && new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &test)
		  && charge_mana_while_resolving(player, card, event, player, COLOR_COLORLESS, 2))
		{
		  token_generation_t token;
		  copy_token_definition(player, card, &token, p, c);
		  token.special_code_on_generation = mirrorworks_set_processing;
		  generate_token(&token);
		}
	}

  return 0;
}

int card_mitotic_manipulation(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int *deck = deck_ptr[player];
			int id = -1;
			int selected = -1;
			int i;
			if( player != AI ){
				selected = show_deck( player, deck, 7, "Select a permanent", 0, 0x7375B0 );
				if( selected != -1 ){
					id = cards_data[deck[selected]].id;
				}
			}
			else{
				 int ids[available_slots];
				 for(i=0; i<available_slots; i++){
					 ids[i] = 0;
				 }
				 i=0;
				 for(i=0; i<7; i++){
					 if( deck[i] != -1 ){
						ids[cards_data[deck[i]].id] = 1;
					 }
				 }
				 i=0;
				 int par = -1;
				 for(i=0; i<2; i++){
					 int count = 0;
					 while( count < active_cards_count[i] ){
							if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) ){
								if( ids[get_id(i, count)] == 1 && get_base_value(i, count) > par ){
									id = get_id(i, count);
									par = get_base_value(i, count);
								}
							}
							count++;
					 }
				 }
			}
			int crds = 7;
			if( id > -1 ){
				if( player == AI ){
					i = 0;
					for(i=0; i<7; i++){
						if( deck[i] != -1 && cards_data[deck[i]].id == id ){
							int card_added = add_card_to_hand(player, deck[i]);
							remove_card_from_deck(player, i);
							put_into_play(player, card_added);
							crds--;
							break;
						}
					}
				}
				else{
					i = 0;
					for(i=0; i<2; i++){
						int count = 0;
						while( count < active_cards_count[i] ){
								if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) ){
									if( get_id(i, count) == id ){
										int card_added = add_card_to_hand(player, deck[selected]);
										remove_card_from_deck(player, selected);
										put_into_play(player, card_added);
										i = 2;
										crds--;
										break;
									}
								}
								count++;
						}
					}
				}
			}
			put_top_x_on_bottom(player, player, crds);
			kill_card(player, card, KILL_DESTROY);

	}

  return 0;
}

int card_morbid_plunder(int player, int card, event_t event)
{
  /* Morbid Plunder	|1|B|B
   * Sorcery
   * Return up to two target creature cards from your graveyard to your hand. */
  if (!IS_CASTING(player, card, event))
	return 0;
  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select up to two target creature cards.");
  return spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(player, card, event, 2, &test, 0);
}

int card_myr_sire(int player, int card, event_t event){
	/* Myr Sire	|2
	 * Artifact Creature - Myr 1/1
	 * When ~ dies, put a 1/1 colorless Myr artifact creature token onto the battlefield. */

	if( graveyard_from_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_MYR);
	}

	return 0;
}

int card_myr_turbine(int player, int card, event_t event){
	/* Myr Turbine	|5
	 * Artifact
	 * |T: Put a 1/1 colorless Myr artifact creature token onto the battlefield.
	 * |T, Tap five untapped Myr you control: Search your library for a Myr creature card, put it onto the battlefield, then shuffle your library. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_MYR;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int choice = 0;
			if( target_available(player, card, &td) > 4 ){
				choice = do_dialog(player, player, card, -1, -1, " Make Myr\n Tutor Myr\n Do nothing", 1);
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				if( choice == 0 ){
					tap_card(player, card);
					instance->targets[1].player = 66+choice;
				}
				if( choice == 1 ){
					if( tapsubtype_ability(player, card, 5, &td) ){
						tap_card(player, card);
						instance->targets[1].player = 66+choice;
					}
					else{
						spell_fizzled = 1;
					}
				}
			}
		}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player == 66 ){
			generate_token_by_id(player, card, CARD_ID_MYR);
		}
		if( instance->targets[1].player == 67 ){
			global_tutor(player, player, 1, TUTOR_PLAY, 0, 2, TYPE_PERMANENT, 0, SUBTYPE_MYR, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}

int card_myr_welder(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) ){
		int result = 0;
		if( ! is_tapped(player, card) && ! is_sick(player, card) && new_special_count_grave(2, &this_test) > 0 &&
			 has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && ! graveyard_has_shroud(2)
		  ){
			return 1;
		}
		int i;
		for(i=2; i<10; i++){
			if( instance->targets[i].player != -1 ){
				card_data_t* card_d = &cards_data[ instance->targets[i].player ];
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				if( ptFunction(player, card, EVENT_CAN_ACTIVATE ) ){
					result = 1;
					break;
				}
			}
			if( instance->targets[i].card != -1 ){
				card_data_t* card_d = &cards_data[ instance->targets[i].card ];
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				if( ptFunction(player, card, EVENT_CAN_ACTIVATE ) ){
					result = 1;
					break;
				}
			}
		}
		return result;
	}
	if( event == EVENT_ACTIVATE ){
		char buffer[500];
		int pos = scnprintf(buffer, 500, " Do nothing\n" );
		int mode = 1<<0;
		if( ! is_tapped(player, card) && ! is_sick(player, card) && new_special_count_grave(2, &this_test) > 0 &&
			 has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && ! graveyard_has_shroud(2)
		  ){
			mode |= (1<<1);
			pos += scnprintf(buffer + pos, 500-pos, " RFG an artifact\n" );
		}
		int i;
		for(i=2; i < 10; i++){
			if( instance->targets[i].player != -1 ){
				card_data_t* card_d = &cards_data[ instance->targets[i].player ];
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				if( ptFunction(player, card, EVENT_CAN_ACTIVATE ) ){
					card_ptr_t* c = cards_ptr[ cards_data[instance->targets[i].player].id ];
					pos += scnprintf(buffer + pos, 500-pos, " %s\n", c->name );
					mode |= (1<<i);
				}
			}
			if( instance->targets[i].card != -1 ){
				card_data_t* card_d = &cards_data[ instance->targets[i].card ];
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				if( ptFunction(player, card, EVENT_CAN_ACTIVATE ) ){
					card_ptr_t* c = cards_ptr[ cards_data[instance->targets[i].card].id ];
					pos += scnprintf(buffer + pos, 500-pos, " %s\n", c->name );
					mode |= (1<<i);
				}
			}
		}
		int choice = do_dialog(player, player, card, -1, -1, buffer, 1);
		while( !( mode & (1<<choice)) ){
				choice++;
		}
		if( choice == 0 ){
			spell_fizzled = 1;
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					instance->targets[0].player = 1-player;
					if( count_graveyard_by_type(1-player, TYPE_ARTIFACT) ){
						if( count_graveyard_by_type(player, TYPE_ARTIFACT) ){
							if( pick_target(&td, "TARGET_PLAYER") ){
								instance->number_of_targets = 1;
							}
						}
					}
					else{
						instance->targets[0].player = player;
					}
					if( new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_VALUE, &this_test, 1) != -1 ){
						tap_card(player, card);
					}
					else{
						spell_fizzled = 1;
					}
				}
		}
		else{
			int count = 2;
			int selected = -1;
			while( 1 ){
					selected = instance->targets[count].player;
					if( count == choice ){
						break;
					}
					count++;
					selected = instance->targets[count-1].card;
					if( count == choice ){
						break;
					}
			}
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[ selected ].code_pointer;
			ptFunction(player, card, EVENT_ACTIVATE);
			if( spell_fizzled != 1 ){
				instance->targets[1].player = 9999;
				instance->targets[1].card = selected;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player != 9999 ){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
			if( selected != -1 ){
				const int *grave = get_grave(instance->targets[0].player);
				card_instance_t *parent = get_card_instance(player, instance->parent_card);
				int pos = 2;
				while( pos < 10 ){
						if( parent->targets[pos].player == -1 ){
							parent->targets[pos].player = grave[selected];
							create_card_name_legacy(player, instance->parent_card, cards_data[grave[selected]].id);
							break;
						}
						if( parent->targets[pos].card == -1 ){
							parent->targets[pos].card = grave[selected];
							create_card_name_legacy(player, instance->parent_card, cards_data[grave[selected]].id);
							break;
						}
						pos++;
				}
				rfg_card_from_grave(instance->targets[0].player, selected);
			}
		}
		else{
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[instance->targets[1].card].code_pointer;
			ptFunction(player, card, EVENT_RESOLVE_ACTIVATION);
		}
	}

	return 0;
}

int card_nested_ghoul(int player, int card, event_t event ){
	/* Nested Ghoul	|3|B|B
	 * Creature - Zombie Warrior 4/2
	 * Whenever a source deals damage to ~, put a 2/2 |Sblack Zombie creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player &&
				damage->info_slot > 0
			  ){
				if( instance->targets[9].player < 0 ){
					instance->targets[9].player = 0;
				}
				instance->targets[9].player++;
			}
		}
	}

	if( instance->targets[9].player > 0  && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				generate_tokens_by_id(player, card, CARD_ID_ZOMBIE, instance->targets[9].player);
				instance->targets[9].player = 0;
		}
	}
	return 0;
}

int card_neurok_commando(int player, int card, event_t event ){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		draw_some_cards_if_you_want(player, card, player, 1);
	}
	return 0;
}

int card_oculus(int player, int card, event_t event ){

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		draw_cards(player, 1);
	}
	return 0;
}

int card_peace_strider(int player, int card, event_t event ){

	if( comes_into_play(player, card, event) ){
		gain_life(player, 3);
	}
	return 0;
}

int card_phyresis(int player, int card, event_t event ){
	return generic_aura(player, card, event, player, 0, 0, KEYWORD_INFECT, 0, 0, 0, 0);
}

int card_phyrexian_rebirth(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities = 0;

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int amount = target_available(player, card, &td);
			manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
			manipulate_all(player, card, 1-player,TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_HORROR, &token);
			token.pow = amount;
			token.tou = amount;
			token.action = TOKEN_ACTION_CONVERT_INTO_ARTIFACT;
			generate_token(&token);

			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_pierce_strider(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		instance->targets[0].player = 1-player;
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 3);
		}
	}

	return 0;
}

int card_piston_sledge(int player, int card, event_t event)
{
  if (comes_into_play(player, card, event)
	  && check_for_equipment_targets(player, card)
	  && activate_basic_equipment(player, card, -42))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  equip_target_creature(player, card, instance->targets[0].player, instance->targets[0].card);
	}

  return altar_equipment(player, card, event, TYPE_ARTIFACT, 3, 1, 0);
}

int card_pistus_strike(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			poison_counters[instance->targets[0].player]++;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_plaguemaw_beast(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		proliferate(player, card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_CREATURE, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_praetors_counsel(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		if( player == HUMAN ){
			return 1;
		}
		else{
			if( count_graveyard(player) > 1 ){
				return 1;
			}
		}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int cg = count_graveyard(player)-1;
			const int *grave = get_grave(player);
			while( cg > -1 ){
				   add_card_to_hand(player, grave[cg]);
				   remove_card_from_grave(player, cg);
				   cg--;
			}
			create_legacy_effect(player, card, &card_spellbook);
			kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_psychosis_crawler(int player, int card, event_t event){

	/* Psychosis Crawler	|5
	 * Artifact Creature - Horror 100/100
	 * ~'s power and toughness are each equal to the number of cards in your hand.
	 * Whenever you draw a card, each opponent loses 1 life. */

	if (card_drawn_trigger(player, card, event, player, RESOLVE_TRIGGER_MANDATORY)){
		lose_life(1-player, 1);
	}

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += hand_count[player];
	}

	return 0;
}

int card_quicksilve_geyser(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;
	if( player == AI ){
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PERMANENT") ){
			instance->targets[1].player = instance->targets[0].player;
			instance->targets[1].card = instance->targets[0].card;
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
			if( can_target(&td) ){
				int choice = do_dialog(player, player, card, -1, -1, " Continue\n Stop", 0);
				if( choice == 0 ){
					td.allow_cancel = 0;
					pick_target(&td, "TARGET_PERMANENT");
				}
			}
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<2; i++){
				if( instance->targets[i].card != -1 ){
					if( validate_target(player, card, &td, i) ){
						bounce_permanent(instance->targets[i].player, instance->targets[i].card);
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rally_the_forces(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	// Attacking creatures get +1/+0 and gain first strike until end of turn.
	else if(event == EVENT_RESOLVE_SPELL ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "");
			test.state = STATE_ATTACKING;
			pump_creatures_until_eot(player, card, current_turn, 0, 1,0, KEYWORD_FIRST_STRIKE,0, &test);

			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rusted_slasher(int player, int card, event_t event)
{
  /* Rusted Slasher	|4
   * Artifact Creature - Horror 4/1
   * Sacrifice an artifact: Regenerate ~. */

  if (!(land_can_be_played & LCBP_REGENERATION))
	return 0;

  if (!IS_ACTIVATING(event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  test_definition_t test;
  new_default_test_definition(&test, TYPE_ARTIFACT, "");
  if (IS_AI(player))
	test.not_me = 1;

  if (event == EVENT_CAN_ACTIVATE && CAN_ACTIVATE0(player, card) && new_can_sacrifice_as_cost(player, card, &test))
	return can_regenerate(player, card);

  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST0))
	new_sacrifice(player, card, player, SAC_AS_COST, &test);

  if (event == EVENT_RESOLVE_ACTIVATION && can_regenerate(instance->parent_controller, instance->parent_card))
	regenerate_target(instance->parent_controller, instance->parent_card);

  return 0;
}

int card_serum_raker(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		APNAP(p, {discard(p, 0, player);};);
	}

	return 0;
}

int card_shimmer_myr(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.zone = TARGET_ZONE_HAND;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_target(&td) ){
		return check_playable_permanents(player, TYPE_ARTIFACT, 1);
	}

	if( event == EVENT_ACTIVATE){
		instance->targets[0].card =  -1;
		if( player == AI ){
			instance->targets[0].card = check_playable_permanents(player, TYPE_ARTIFACT, 0);
		}
		else{
			 if( pick_target(&td, "TARGET_ARTIFACT") ){
				 instance->number_of_targets = 1;
			 }
		}
		if( instance->targets[0].card != -1 ){
			int id = get_id(player, instance->targets[0].card);
			card_ptr_t* c = cards_ptr[ id ];
			if( has_mana_multi(player, c->req_colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white) ){
				card_instance_t *this = get_card_instance(player, instance->targets[0].card);
				if( can_legally_play_iid(player, this->internal_card_id) ){
					charge_mana_multi(player, c->req_colorless, c->req_black, c->req_blue, c->req_green,
									  c->req_red, c->req_white);
					if( spell_fizzled != 1 ){
						play_card_in_hand_for_free(player, instance->targets[0].card);
						cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
					}
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

	return flash(player, card, event);
}

int card_shriekhorn(int player, int card, event_t event){

	/* Shriekhorn	|1
	 * Artifact
	 * ~ enters the battlefield with three charge counters on it.
	 * |T, Remove a charge counter from ~: Target player puts the top two cards of his or her library into his or her graveyard. */

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, 3);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_CHARGE), &td, "TARGET_PLAYER");
}

int card_skinwing(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_equipping(player, card) ){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 2, 2, KEYWORD_FLYING);
	}

	return living_weapon(player, card, event, 6);
}

int card_spin_engine(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			int p = instance->parent_controller, c = instance->parent_card;

			creature1_cant_block_creature2_until_eot(p, c, instance->targets[0].player, instance->targets[0].card, p, c);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 0, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_spiraling_duelist(int player, int card, event_t event ){

	if( metalcraft(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_DOUBLE_STRIKE);
	}

	return 0;
}

int card_spire_serpent(int player, int card, event_t event ){

	if( metalcraft(player, card) ){
		modify_pt_and_abilities(player, card, event, 2, 2, 0);
		if (event == EVENT_ABILITIES && affect_me(player, card)){
			get_card_instance(player, card)->token_status |= STATUS_WALL_CAN_ATTACK;
		}
	}

	return 0;
}

int card_spread_the_sickness(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			proliferate(player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_strandwalker(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_equipping(player, card) ){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 2, 4, KEYWORD_REACH);
	}

	return living_weapon(player, card, event, 4);
}

int card_tangle_hulk(int player, int card, event_t event){
	return regeneration(player, card, event, 2, 0, 0, 1, 0, 0);
}

int card_training_drone(int player, int card, event_t event)
{
  if ((event == EVENT_ATTACK_LEGALITY || event == EVENT_BLOCK_LEGALITY) && affect_me(player, card) && ! is_humiliated(player, card) &&
	!equipments_attached_to_me(player, card, EATM_REPORT_TOTAL))
	event_result = 1;

  return 0;
}

int card_unnatural_predation(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									1, 1, KEYWORD_TRAMPLE, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_vedalken_anatomist(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
			int choice = 0;
			if( player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Tap\n Untap", 0);
			}

			if( choice == 0 ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 2, 0, 1, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_vedalken_infuser(int player, int card, event_t event){

	/* Vedalken Infuser	|3|U
	 * Creature - Vedalken Wizard 1/4
	 * At the beginning of your upkeep, you may put a charge counter on target artifact. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_ARTIFACT") ){
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_CHARGE);
		}
	}
	return 0;
}

int card_viridian_claw(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 1, 1, 0, KEYWORD_FIRST_STRIKE, 0);
}

static int virulent_wound_legacy(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance( player, card );

	if( graveyard_from_play(instance->targets[0].player, instance->targets[0].card, event) ){
		poison_counters[instance->targets[0].player]++;
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_virulent_wound(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, &virulent_wound_legacy,
											instance->targets[0].player, instance->targets[0].card);
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_phyrexian_revoker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.illegal_type = TYPE_LAND;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int choice = do_dialog(player, player, card, -1, -1, " Select a card in play\n Choose a card from a list", 0);
		int id = -1;
		if( choice == 0 ){
			int stop = 0;
			while( stop == 0 ){
					if( pick_target(&td, "TARGET_PERMANENT") ){
						id = get_id(instance->targets[0].player, instance->targets[0].card);
						instance->number_of_targets = 1;
						stop = 1;
					}
			}
		}
		else{
			int stop = 0;
			while( stop == 0 ){
					if( ai_is_speculating != 1 ){
						int card_id = choose_a_card("Choose a card", -1, -1);
						if( is_valid_card(cards_data[card_id].id) && ! is_what(-1, card_id, TYPE_LAND) ){
							id = cards_data[card_id].id;
							stop = 1;
						}
					}
			}
		}
		if( id != -1 ){
			instance->targets[9].card = id;
			create_card_name_legacy(player, card, id);
			manipulate_all(player, card, player, TYPE_PERMANENT, 0, 0, 0, 0, 0, id, 0, -1, 0, ACT_DISABLE_ALL_ACTIVATED_ABILITIES);
			manipulate_all(player, card, 1-player,TYPE_PERMANENT, 0, 0, 0, 0, 0, id, 0, -1, 0, ACT_DISABLE_ALL_ACTIVATED_ABILITIES);
		}
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( trigger_cause_controller, trigger_cause ) &&
		reason_for_trigger_controller == affected_card_controller
	  ){
		if( affect_me(player, card) ){ return 0; }
		if( get_id(trigger_cause_controller, trigger_cause) == instance->targets[9].card ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					disable_all_activated_abilities(trigger_cause_controller, trigger_cause, 1);
			}
		}
	}

	if( leaves_play(player, card, event) ){
		int id = instance->targets[9].card;
		if( id != -1 ){
			manipulate_all(player, card, player, TYPE_PERMANENT, 0, 0, 0, 0, 0, id, 0, -1, 0, ACT_ENABLE_ALL_ACTIVATED_ABILITIES);
			manipulate_all(player, card, 1-player,TYPE_PERMANENT, 0, 0, 0, 0, 0, id, 0, -1, 0, ACT_ENABLE_ALL_ACTIVATED_ABILITIES);
		}
	}
	return 0;
}


