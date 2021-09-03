#include "manalink.h"

// Global functions

int modular(int player, int card, event_t event, int initial_counters){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, initial_counters);

	if (event == EVENT_ABILITIES && affect_me(player, card) && !is_humiliated(player, card)){
		set_special_flags(player, card, SF_MODULAR);
	}

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME | TARGET_SPECIAL_ARTIFACT_CREATURE;
		td.illegal_state = TARGET_STATE_DYING;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && select_target(player, card, &td, "Select target artifact creature", &(instance->targets[0])) ){
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, count_1_1_counters(player, card));
		}
	}

	return 0;
}

static int echoing_spell(int player, int card, event_t event, target_definition_t *td, const char *prompt, int effect){

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(td) ){
			card_instance_t* instance = get_card_instance(player, card);
			int id = get_id(instance->targets[0].player, instance->targets[0].card);
			manipulate_all(player, card, player, 0, 0, 0, 0, 0, 0, id, 0, -1, 0, effect);
			manipulate_all(player, card, 1-player, 0, 0, 0, 0, 0, 0, id, 0, -1, 0, effect);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, td, prompt, 1, NULL);
}

int echoing_pump(int player, int card, event_t event, int p_plus, int t_plus){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( p_plus > 0 ){
		td.preferred_controller = player;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int id = get_id(instance->targets[0].player, instance->targets[0].card);
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && get_id(i, count) == id ){
							pump_until_eot(player, card, i, count, p_plus, t_plus);
						}
						count--;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

// Cards

int card_angels_feather2(int player, int card, event_t event){
	return lifegaining_charm(player, card, event, 2, COLOR_TEST_WHITE, 1+duh_mode(player), 0);
}

int card_arcbound_crusher(int player, int card, event_t event)
{
  /* Arcbound Crusher	|4
   * Artifact Creature - Juggernaut 0/0
   * Trample
   * Whenever another artifact enters the battlefield, put a +1/+1 counter on ~.
   * Modular 1 */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ARTIFACT, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, &test))
		add_1_1_counter(player, card);
	}

  return modular(player, card, event, 1);
}

int card_arcbound_worker(int player, int card, event_t event){
	return modular(player, card, event, 1);
}

int card_arcbound_hybrid(int player, int card, event_t event){
	haste(player, card, event);
	return modular(player, card, event, 2);
}

int card_arcbound_overseer(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) && is_what(player, count, TYPE_ARTIFACT) ){
					if( check_special_flags(player, count, SF_MODULAR) ){
						add_1_1_counters(player, count, 1);
					}
				}
				count--;
		}
	}

	return modular(player, card, event, 6);
}

int card_arcbound_ravager(int player, int card, event_t event){
	/* Arcbound Ravager	|2
	 * Artifact Creature - Beast 0/0
	 * Sacrifice an artifact: Put a +1/+1 counter on ~.
	 * Modular 1 */
	modular(player, card, event, 1);
	return generic_husk(player, card, event, TYPE_ARTIFACT, 101, 101, 0, 0);
}

int card_arcbound_reclaimer(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		if( count_1_1_counters(player, card) > 0 && count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 ){
			return ! graveyard_has_shroud(player);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
				remove_1_1_counter(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			from_graveyard_to_deck(player, selected, 1);
		}
	}

	return modular(player, card, event, 2);
}

int card_arcbound_slith(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_OPPONENT) ){
		add_1_1_counter(player, card);
	}
	return modular(player, card, event, 1);
}

// Arcbound Stinger -> Arcbound Worker

int card_aether_snap(int player, int card, event_t event){

	/* AEther Snap	|3|B|B
	 * Sorcery
	 * Remove all counters from all permanents and exile all tokens. */

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) ){
						if( is_token(i, count) ){
							kill_card(i, count, KILL_REMOVE);
						}
						else{
							remove_all_counters(i, count, -1);
						}
					}
					count--;
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_auriok_glaivemaster(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( (event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) &&
			equipments_attached_to_me(player, card, EATM_CHECK)
		  ){
			if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
				event_result++;
			}
			if( event == EVENT_ABILITIES ){
				event_result |= KEYWORD_FIRST_STRIKE;
			}
		}
	}

	return 0;
}

int card_barbed_lighting(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td2, "TARGET_CREATURE_OR_PLAYER") ){
			if( has_mana(player, COLOR_COLORLESS, 2) ){
				if( (instance->targets[0].card == -1 && can_target(&td)) || (instance->targets[0].card != -1 && can_target(&td1)) ){
					int choice = do_dialog(player, player, card, -1, -1, " Entwine\n Pass", 0);
					if( choice == 0 ){
						charge_mana(player, COLOR_COLORLESS, 2);
						if (cancel != 1){
							if( instance->targets[0].card == -1 ){
								new_pick_target(&td, "TARGET_CREATURE", 1, 1);
							}
							else{
								new_pick_target(&td1, "TARGET_PLAYER", 1, 1);
							}
						}
					}
				}
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->targets[0].card > -1 ){
			if( validate_target(player, card, &td, 0) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
			}
		}
		else{
			if( validate_target(player, card, &td1, 0) ){
				damage_player(instance->targets[0].player, 3, player, card);
			}
		}
		if( instance->targets[1].player > -1 ){
			if( instance->targets[1].card > -1 ){
				if( validate_target(player, card, &td, 1) ){
					damage_creature(instance->targets[1].player, instance->targets[1].card, 3, player, card);
				}
			}
			else{
				if( validate_target(player, card, &td1, 1) ){
					damage_player(instance->targets[1].player, 3, player, card);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_blinkmoth_nexus(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if (event == EVENT_COUNT_MANA && affect_me(player, card)){
		if (!is_tapped(player, card) && !is_animated_and_sick(player, card) && instance->targets[1].player != 66){
			return mana_producer(player, card, event);
		}
	} else if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		if (event == EVENT_CAN_ACTIVATE && instance->targets[1].player == 66){
			return 0;
		}
		if (paying_mana()){
			return mana_producer(player, card, event);
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.required_subtype = SUBTYPE_BLINKMOTH;

		int can_generate_mana = (!is_tapped(player, card) && !is_animated_and_sick(player, card)
								 && instance->targets[1].player != 66 && can_produce_mana(player, card));
		int ai_priority_generate_mana = -1;

		int can_animate = can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(1));
		int ai_priority_animate = (is_what(player, card, TYPE_CREATURE)
								   || (current_turn == player && current_phase > PHASE_MAIN1)
								   || (current_turn != player && current_phase != PHASE_BEFORE_BLOCKING)) ? 0 : 2;

		int can_pump = (can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(2))
						&& !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_target(&td));
		int ai_priority_pump = current_phase == PHASE_AFTER_BLOCKING ? 3 : 1;

		int choice = DIALOG(player, card, event,
							"Generate mana", can_generate_mana, ai_priority_generate_mana,
							"Animate", can_animate, ai_priority_animate,
							"Pump a Blinkmoth", can_pump, ai_priority_pump);

		if (event == EVENT_CAN_ACTIVATE){
			return choice;
		} else if (event == EVENT_ACTIVATE){
			if (choice == 1){
				return mana_producer(player, card, event);
			} else if (choice == 2){
				if (player == AI){
					instance->targets[1].player = 66;
				}
				charge_mana_for_activated_ability(player, card, MANACOST_X(1));
				instance->targets[1].player = 0;
			} else if (choice == 3){
				instance->targets[1].player = 66;
				instance->number_of_targets = 0;
				if (charge_mana_for_activated_ability(player, card, MANACOST_X(1)) && pick_target(&td, "TARGET_CREATURE")){
					tap_card(player, card);
				}
				instance->targets[1].player = 0;
			}
		} else {	// event == EVENT_RESOLVE_ACTIVATION
			if (choice == 2 && instance->targets[13].player == instance->targets[13].card){
				land_animation(player, instance->parent_card);
			} else if (choice == 3 && valid_target(&td)){
				pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
			}
		}
	} else {
		return mana_producer(player, card, event);
	}
	return 0;
}

int card_blinkmoth_nexus_animated(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( eot_trigger(player, card, event ) ){
		land_animation(player, card);
	}
	else if( event == EVENT_SET_COLOR && affect_me(player, card) ){
			event_result = cards_data[ instance->internal_card_id ].color ;
	}
	return card_blinkmoth_nexus(player, card, event);
}

int card_carry_away(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET || event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ||
		event == EVENT_CAN_MOVE_AURA || event == EVENT_MOVE_AURA
	  ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.required_subtype = SUBTYPE_EQUIPMENT;
		return targeted_aura(player, card, event, &td, "TARGET_EQUIPMENT");
	}

	if( event == EVENT_RESOLVE_SPELL || event == EVENT_RESOLVE_MOVING_AURA ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.required_subtype = SUBTYPE_EQUIPMENT;
		if( valid_target(&td) ){
			unattach(instance->targets[0].player, instance->targets[0].card);
			return generic_stealing_aura(player, card, event, &td, "TARGET_EQUIPMENT");
		}
		else{
			if( event == EVENT_RESOLVE_SPELL ){
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}

	return 0;
}

int card_chimeric_egg(int player, int card, event_t event){

	/* Chimeric Egg	|3
	 * Artifact
	 * Whenever an opponent casts a nonartifact spell, put a charge counter on ~.
	 * Remove three charge counters from ~: ~ becomes a 6/6 Construct artifact creature with trample until end of turn. */


	card_instance_t *instance= get_card_instance(player, card);

	if( specific_spell_played(player, card, event, 1-player, 2, TYPE_ARTIFACT, 1, 0, 0, 0, 0, 0, 0, -1, 0) ){
		add_counter(player, card, COUNTER_CHARGE);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST0, 0, NULL, NULL) ){
			return count_counters(player, card, COUNTER_CHARGE) > 2;
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			remove_counters(player, card, COUNTER_CHARGE, 3);
			set_special_flags(player, card, SF_TYPE_ALREADY_CHANGED);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int pp = instance->parent_controller, pc = instance->parent_card;
		card_instance_t *parent = get_card_instance(pp, pc);
		add_a_subtype(pp, pc, SUBTYPE_CONSTRUCT);
		artifact_animation(pp, pc, pp, pc, 1, 6, 6, KEYWORD_TRAMPLE, 0);
		parent->targets[1].player = 0;
	}

	return 0;
}

int card_chittering_rats(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		if( would_validate_arbitrary_target(&td, 1-player, -1)  ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);

			ec_definition_t this_definition;
			default_ec_definition(1-player, 1-player, &this_definition);
			this_definition.ai_selection_mode = AI_MIN_CMC;
			this_definition.effect = EC_PUT_ON_TOP;
			new_effect_coercion(&this_definition, &this_test);
		}
	}

	return 0;
}

int card_chromescale_drake(int player, int card, event_t event){

	affinity(player, card, event, TYPE_ARTIFACT, -1);

	if( comes_into_play(player, card, event) ){
		int *deck = deck_ptr[player];
		int amount = 3;
		if( count_deck(player) < amount ){
			amount = count_deck(player);
		}
		if( amount > 0 ){
			show_deck( HUMAN, deck, amount, "Here's the first 3 card of deck", 0, 0x7375B0 );
			int i;
			for(i=0; i<amount; i++){
				if( is_what(-1, deck[0], TYPE_ARTIFACT) ){
					add_card_to_hand(player, deck[0]);
					remove_card_from_deck(player, 0);
				}
				else{
					mill(player, 1);
				}
			}
		}
	}
	return 0;
}

static const char* target_must_natively_use_charge_counters(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return get_counter_type_by_id(get_id(player, card)) == COUNTER_CHARGE ? NULL : "must use charge counters";
}

int card_coretapper(int player, int card, event_t event)
{
  /* Coretapper	|2
   * Artifact Creature - Myr 1/1
   * |T: Put a charge counter on target artifact.
   * Sacrifice ~: Put two charge counters on target artifact. */

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_ARTIFACT);
	  td.preferred_controller = player;
	  if (IS_AI(player))
		{
		  td.extra = (int)target_must_natively_use_charge_counters;
		  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		}

	  target_definition_t td_sac = td;	// struct copy
	  td_sac.special |= TARGET_SPECIAL_NOT_ME;

	  enum
	  {
		CHOICE_TAP = 1,
		CHOICE_SAC
	  } choice = DIALOG(player, card, event,
						"Tap for 1 charge counter", 1, 3, DLG_MANA(MANACOST0), DLG_TAP, DLG_TARGET(&td, "TARGET_ARTIFACT"),
						"Sacrifice for 2 charge counters", can_sacrifice_this_as_cost(player, card), 1, DLG_MANA(MANACOST0), DLG_TARGET(&td, "TARGET_ARTIFACT"));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_TAP:
			  if (current_turn == 1-player && current_phase == PHASE_DISCARD)
				ai_modifier += 256;
			  break;

			case CHOICE_SAC:
			  ai_modifier -= 128;
			  break;
		  }
	  else	// EVENT_RESOLVE_ACTIVATION
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  add_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_CHARGE, choice == CHOICE_TAP ? 1 : 2);
		}
	}

  return 0;
}

int card_darksteel_citadel(int player, int card, event_t event)
{
  // 0x1204026

  /* Darksteel Citadel	""
   * Artifact Land
   * ~ is indestructible.
   * |T: Add |1 to your mana pool. */

  indestructible(player, card, event);
  return mana_producer(player, card, event);
}

int card_darksteel_colossus(int player, int card, event_t event)
{
  /* Darksteel Colossus	|11
   * Artifact Creature - Golem 11/11
   * Trample
   * ~ is indestructible.
   * If ~ would be put into a graveyard from anywhere, reveal ~ and shuffle it into its owner's library instead. */

  // Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

  indestructible(player, card, event);
  return 0;
}

int card_darksteel_forge(int player, int card, event_t event)
{
  /* Darksteel Forge	|9
   * Artifact
   * Artifacts you control are indestructible. */

  if (event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT)
	  && in_play(player, card) && !is_humiliated(player, card))
	indestructible(affected_card_controller, affected_card, event);

  return 0;
}

int card_darksteel_ingot(int player, int card, event_t event)
{
  // 0x1202996
  indestructible(player, card, event);
  return artifact_mana_all_one_color(player, card, event, 1, 0);
}

int card_darksteel_pendant(int player, int card, event_t event){

	indestructible(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		scrylike_effect(player, player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_NONSICK, MANACOST_X(1), 0, NULL, NULL);
}

int card_darksteel_reactor(int player, int card, event_t event){

	/* Darksteel Reactor	|4
	 * Artifact
	 * ~ is indestructible.
	 * At the beginning of your upkeep, you may put a charge counter on ~.
	 * When ~ has twenty or more charge counters on it, you win the game. */

	indestructible(player, card, event);

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_DUH);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_CHARGE);
	}

	if (event == EVENT_STATIC_EFFECTS && count_counters(player, card, COUNTER_CHARGE) >= 20 && ! is_humiliated(player, card)){
		lose_the_game(1-player);
	}

	return 0;
}

int card_death_cloud(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);

		int j;
		for(j=0;j<2;j++){
			int p = player;
			if( j == 1){ p = 1-player; }
			lose_life(p, instance->info_slot);

			int count = instance->info_slot;

			new_multidiscard(p, count, 0, player);

			impose_sacrifice(player, card, p, count, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);

			impose_sacrifice(player, card, p, count, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);

		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_demons_horn2(int player, int card, event_t event){
	return lifegaining_charm(player, card, event, 2, COLOR_TEST_BLACK, 1+duh_mode(player), 0);
}

int card_dismantle(int player, int card, event_t event){

	/* Dismantle	|2|R
	 * Sorcery
	 * Destroy target artifact. If that artifact had counters on it, put that many +1/+1 counters or charge counters on an artifact you control. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){

				int cplus = count_counters(instance->targets[0].player, instance->targets[0].card, -1);

				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

				td.allowed_controller = player;
				td.preferred_controller = player;
				td.allow_cancel = 0;

				if( cplus > 0 && can_target(&td) ){
					int ai_choice = 0;
					if (IS_AI(player)){
						// If can target an artifact creature, choose +1/+1 counters.
						td.special = TARGET_SPECIAL_ARTIFACT_CREATURE;

						if (!can_target(&td)){
							ai_choice = 1;

							// If can target an artifact that uses charge counters, pick it
							td.extra = (int)target_must_natively_use_charge_counters;
							td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

							if (!can_target(&td)){
								// Otherwise, any artifact
								td.extra = 0;
								td.special = 0;
							}
						}
					}
					int choice = do_dialog(player, player, card, -1, -1, " Add +1/+1 counters\n Add charge counters", ai_choice);
					pick_target(&td, "TARGET_ARTIFACT");
					if( choice == 0 ){
						add_1_1_counters(instance->targets[0].player, instance->targets[0].card, cplus);
					} else {
						add_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_CHARGE, cplus);
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 1, NULL);
}

int card_dragons_claw2(int player, int card, event_t event){
	return lifegaining_charm(player, card, event, 2, COLOR_TEST_RED, 1+duh_mode(player), 0);
}

int card_dross_golem(int player, int card, event_t event){

	affinity(player, card, event, TYPE_LAND, SUBTYPE_SWAMP);

	fear(player, card, event);

	return 0;
}

int card_eater_of_days(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		skip_next_turn(player, card, player);
		skip_next_turn(player, card, player);
	}

	return 0;
}

int card_echoing_calm(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	return echoing_spell(player, card, event, &td, "TARGET_ENCHANTMENT", KILL_DESTROY);
}

int card_echoing_courage(int player, int card, event_t event){
	return echoing_pump(player, card, event, 2, 2);
}

int card_echoing_decay(int player, int card, event_t event){
	return echoing_pump(player, card, event, -2, -2);
}

int card_echoing_ruin(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	return echoing_spell(player, card, event, &td, "TARGET_ARTIFACT", KILL_DESTROY);
}

int card_echoing_truth(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonland permanent.", 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		return echoing_spell(player, card, event, &td, NULL, ACT_BOUNCE);
	}

	return 0;
}

int card_essence_drain(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 3);
			gain_life(player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}


int card_fangren_firstborn(int player, int card, event_t event)
{
  /* Fangren Firstborn	|1|G|G|G
   * Creature - Beast 4/2
   * Whenever ~ attacks, put a +1/+1 counter on each attacking creature. */

  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  new_manipulate_all(player, card, player, &test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}

  return 0;
}

int card_flamebreak(int player, int card, event_t event)
{
  // ~ deals 3 damage to each creature without flying and each player. Creatures dealt damage this way can't be regenerated this turn.

	if (event == EVENT_RESOLVE_SPELL){
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.keyword = KEYWORD_FLYING;
	  test.keyword_flag = DOESNT_MATCH;
	  new_damage_all(player, card, ANYBODY, 3, NDA_PLAYER_TOO|NDA_CANT_REGENERATE_IF_DEALT_DAMAGE_THIS_WAY, &test);

	  kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_furnace_dragon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	affinity(player, card, event, TYPE_ARTIFACT, -1);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = not_played_from_hand(player, card);
	}

	if( comes_into_play(player, card, event) ){
		if( instance->info_slot != 1 ){
			manipulate_all(player, card, player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_REMOVE);
			manipulate_all(player, card, 1-player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_REMOVE);
		}
	}

	return 0;
}

int card_gemini_engine(int player, int card, event_t event){

	/* Whenever ~ attacks, put a colorless Construct artifact creature token named Twin onto the battlefield attacking. Its power is equal to ~'s power and its
	 * toughness is equal to ~'s toughness. Sacrifice the token at end of combat. */
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_TWIN, &token);
		token.pow = get_power(player, card);
		token.tou = get_toughness(player, card);
		token.action = TOKEN_ACTION_ATTACKING;
		token.legacy = 1;
		token.special_code_for_legacy = sacrifice_at_end_of_combat;
		generate_token(&token);
	}

	return 0;
}

int card_genesis_chamber(int player, int card, event_t event){
	/* Genesis Chamber	|2
	 * Artifact
	 * Whenever a nontoken creature enters the battlefield, if ~ is untapped, that creature's controller puts a 1/1 colorless Myr artifact creature token onto the battlefield. */

	if( ! is_tapped(player, card) && specific_cip(player, card, event, 2, 2, TYPE_CREATURE, 3, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t *instance = get_card_instance(player, card);
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_MYR, &token);
		token.t_player = instance->targets[1].player;
		generate_token(&token);
	}

	return 0;
}

int card_geths_grimoire(int player, int card, event_t event){

	if( ! is_humiliated(player, card) && discard_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_AI(player), 0) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_greater_harvester(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		sacrifice(player, card, player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		impose_sacrifice(player, card, 1-player, 2, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_hallow2(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_SPELL);
	td1.illegal_type = TYPE_CREATURE;
	td1.extra = damage_card;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			gain_life(player, target->info_slot);
			target->info_slot = 0;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET|GS_DAMAGE_PREVENTION, &td1, "TARGET_DAMAGE", 1, NULL);
}


int card_grimclaw_bat(int player, int card, event_t event){
	return generic_shade(player, card, event, 1, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0);
}

int card_heartseeker(int player, int card, event_t event){

	/* Heartseeker	|4
	 * Artifact - Equipment
	 * Equipped creature gets +2/+1 and has "|T, Unattach ~: Destroy target creature."
	 * Equip |5 */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card)){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 2, 1, 0);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( is_equipping(player, card) &&
			generic_activated_ability(instance->targets[8].player, instance->targets[8].card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td1, NULL)
		  ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 5);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( can_activate_basic_equipment(player, card, event, 5) ){
			if( is_equipping(player, card) &&
				generic_activated_ability(instance->targets[8].player, instance->targets[8].card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0,
										0, &td1, NULL)
			  ){
				choice = do_dialog(player, player, card, -1, -1, " Equip\n Tap to kill\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			activate_basic_equipment(player, card, 5);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66;
			}
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(instance->targets[8].player, instance->targets[8].card, MANACOST0) ){
					instance->targets[2].player = get_protections_from(instance->targets[8].player, instance->targets[8].card);
					td1.illegal_abilities = instance->targets[2].player;
					if( pick_target(&td1, "TARGET_CREATURE") ){
						tap_card(instance->targets[8].player, instance->targets[8].card);
						unattach(player, card);
						instance->number_of_targets = 1;
						instance->info_slot = 67;
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
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			resolve_activation_basic_equipment(player, card);
		}
		if( instance->info_slot == 67 ){
			td1.illegal_abilities = instance->targets[2].player;
			if( valid_target(&td1) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_karstoderm(int player, int card, event_t event){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 5);

	if( specific_cip(player, card, event, 2, 2, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		remove_1_1_counter(player, card);
	}

	return 0;
}

int card_krakens_eye2(int player, int card, event_t event){
	return lifegaining_charm(player, card, event, 2, COLOR_TEST_BLUE, 1+duh_mode(player), 0);
}

int card_krark_clan_stoker(int player, int card, event_t event)
{
  // Sacrifice an artifact: ~ deals 1 damage to each creature without flying.

  if (event == EVENT_ACTIVATE && altar_basic_activation(player, card, 1, TYPE_ARTIFACT))
	{
	  produce_mana_tapped(player, card, COLOR_RED, 2);
	  return 0;
	}

  if (event == EVENT_COUNT_MANA && affect_me(player, card)
	  && !is_tapped(player, card) && !is_sick(player, card)
	  && can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT,MATCH, 0,0, 0,0, 0,0, -1,0))
	declare_mana_available(player, COLOR_RED, 2);

  return altar_basic(player, card, event, 5, TYPE_ARTIFACT);
}

int card_last_word(int player, int card, event_t event){

	cannot_be_countered(player, card, event);

	return card_counterspell(player, card, event);
}

int card_leonin_battlemage(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	return vanilla_creature_pumper(player, card, event, MANACOST0, GAA_UNTAPPED | GAA_NONSICK, 1, 1, 0, 0, &td);
}

int card_leonin_bola(int player, int card, event_t event){

	/* Leonin Bola	|1
	 * Artifact - Equipment
	 * Equipped creature has "|T, Unattach ~: Tap target creature."
	 * Equip |1 */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( is_equipping(player, card) &&
			generic_activated_ability(instance->targets[8].player, instance->targets[8].card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td1, NULL)
		  ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 1);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( can_activate_basic_equipment(player, card, event, 1) ){
			if( is_equipping(player, card) &&
				generic_activated_ability(instance->targets[8].player, instance->targets[8].card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0,
										0, &td1, NULL)
			  ){
				choice = do_dialog(player, player, card, -1, -1, " Equip\n Tap to tap a creature\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			activate_basic_equipment(player, card, 1);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66;
			}
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(instance->targets[8].player, instance->targets[8].card, MANACOST0) ){
					instance->targets[2].player = get_protections_from(instance->targets[8].player, instance->targets[8].card);
					td1.illegal_abilities = instance->targets[2].player;
					if( pick_target(&td1, "TARGET_CREATURE") ){
						tap_card(instance->targets[8].player, instance->targets[8].card);
						unattach(player, card);
						instance->number_of_targets = 1;
						instance->info_slot = 67;
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
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			resolve_activation_basic_equipment(player, card);
		}
		if( instance->info_slot == 67 ){
			td1.illegal_abilities = instance->targets[2].player;
			if( valid_target(&td1) ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_lichs_tomb(int player, int card, event_t event){

	cannot_lose_the_game_for_having_less_than_0_life(player, card, event, player);

	return 0;
}

int card_machinate(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = count_permanents_by_type(player, TYPE_ARTIFACT);
		impulse_effect(player, amount, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_wurms_tooth2(int player, int card, event_t event){
	return lifegaining_charm(player, card, event, 2, COLOR_TEST_GREEN, 1+duh_mode(player), 0);
}

int card_memnarch(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_ARTIFACT;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;

	card_instance_t *instance= get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 0, 2, 0, 0, 0) ){
			return can_target(&td);
		}
		if( has_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 0) ){
			return can_target(&td1);
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 1, 0, 2, 0, 0, 0) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 0) && can_target(&td1) ){
				choice = do_dialog(player, player, card, -1, -1, " Turn into Artifact\n Steal an Artifact\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, 1, 0, 2, 0, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_PERMANENT") ){
					instance->number_of_targets = 1;
					instance->info_slot = 66;
				}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td1, "TARGET_ARTIFACT") ){
					instance->number_of_targets = 1;
					instance->info_slot = 67;
				}
		}
		else{
			spell_fizzled = 1;
		}

	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			turn_into_artifact(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			gain_control(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_mephitic_ooze(int player, int card, event_t event){

	deathtouch(player, card, event);

	return nims(player, card, event);
}

int card_mirrodins_core(int player, int card, event_t event)
{
  /* Mirrodin's Core	""
   * Land
   * |T: Add |1 to your mana pool.
   * |T: Put a charge counter on ~.
   * |T, Remove a charge counter from ~: Add one mana of any color to your mana pool. */

  if (event == EVENT_CHANGE_TYPE)
	get_card_instance(player, card)->info_slot = count_counters(player, card, COUNTER_CHARGE) > 0 ? COLOR_TEST_ANY : COLOR_TEST_COLORLESS;

  if (event == EVENT_RESOLVE_SPELL)
	play_land_sound_effect_force_color(player, card, COLOR_TEST_COLORLESS);

  if (IS_ACTIVATING(event))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  int can_tap = CAN_TAP(player, card);
	  int can_produce = can_produce_mana(player, card);

	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_CHARGE = 2
	  } choice;

	  if (paying_mana() && can_produce)
		choice = CHOICE_MANA;
	  else
		choice = DIALOG(player, card, event,
						DLG_STORE_IN(&instance->targets[1].player),
						"Add mana", can_tap && can_produce, paying_mana() ? 3 : -1,
						"Add charge counter", can_tap && CAN_ACTIVATE0(player, card), 1);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_MANA:
			  if (count_counters(player, card, COUNTER_CHARGE) <= 0)
				produce_mana_tapped(player, card, COLOR_COLORLESS, 1);
			  else if (produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY, 1) && (chosen_colors & COLOR_TEST_ANY_COLORED))
				remove_counter(player, card, COUNTER_CHARGE);
			  break;

			case CHOICE_CHARGE:
			  instance->state |= STATE_TAPPED;
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(0)))
				tapped_for_mana_color = -1;
			  else
				untap_card_no_event(player, card);
			  break;
		  }
	  else	// EVENT_RESOLVE_ACTIVATION
		if (choice == CHOICE_CHARGE)
		  add_counter(player, card, COUNTER_CHARGE);
	}

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_TAP_FOR_MANA(player, card))
	{
	  if (count_counters(player, card, COUNTER_CHARGE) > 0)
		declare_mana_available_hex(player, COLOR_TEST_ANY, 1);
	  else
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}

  return 0;
}

int card_murderous_spoils(int player, int card, event_t event){

	/* Murderous Spoils	|5|B
	 * Instant
	 * Destroy target non|Sblack creature. It can't be regenerated. You gain control of all Equipment that was attached to it. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( i != player && in_play(i, count) && is_what(i, count, TYPE_ARTIFACT) && has_subtype(i, count, SUBTYPE_EQUIPMENT) ){
							card_instance_t *this = get_card_instance( i, count );
							if( this->targets[8].player == instance->targets[0].player && this->targets[8].card == instance->targets[0].card ){
								gain_control(player, card, i, count);
							}
						}
						count--;
				}
			}
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						 get_sleighted_color_text(player, card, "Select target non%s creature.", COLOR_BLACK),
						 1, NULL);
}

int card_mycosynth_lattice(int player, int card, event_t event)
{
  /* Mycosynth Lattice	|6
   * Artifact
   * All permanents are artifacts in addition to their other types.
   * All cards that aren't on the battlefield, spells, and permanents are colorless.
   * Players may spend mana as though it were mana of any color. */

  if (event == EVENT_CHANGE_TYPE && is_what(affected_card_controller, affected_card, TYPE_PERMANENT))
	set_special_flags2(affected_card_controller, affected_card, SF2_MYCOSYNTH_LATTICE);

  if (event == EVENT_SET_COLOR)
	event_result = 0;


  if (event == EVENT_MODIFY_COST_GLOBAL && !is_what(affected_card_controller, affected_card, TYPE_LAND))
	{
	  /* Would be much better to use the Sunglasses of Urza functionality to directly allow mana to be spent as if it were any type, but that can only hold ten
	   * different values and we'd need 25.  (colorless to each of 5, and each of 5 to each of the other 4).  It's referenced in a surprising number of places
	   * in the exe, so not terribly easy to replace it, or make either the from or to colors be bitfields instead of an index. */

	  // Only change casting costs once; so if this isn't the first Mycosynth Lattice in play, do nothing.
	  card_instance_t* inst;
	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (c == card && p == player)
			goto not_found;
		  else if ((inst = in_play(p, c)) && cards_data[inst->internal_card_id].id == CARD_ID_MYCOSYNTH_LATTICE)
			return 0;

	not_found:;
	  card_ptr_t* cp = cards_ptr[get_id(affected_card_controller, affected_card)];
	  int amount = 0;

	  if (cp->req_black < 15)
		amount += cp->req_black;
	  COST_BLACK -= cp->req_black;

	  if (cp->req_blue < 15)
		amount += cp->req_blue;
	  COST_BLUE -= cp->req_blue;

	  if (cp->req_green < 15)
		amount += cp->req_green;
	  COST_GREEN -= cp->req_green;

	  if (cp->req_red < 15)
		amount += cp->req_red;
	  COST_RED -= cp->req_red;

	  if (cp->req_white < 15)
		amount += cp->req_white;
	  COST_WHITE -= cp->req_white;

	  COST_COLORLESS += amount;
	}

  return global_enchantment(player, card, event);
}

int card_myr_matrix(int player, int card, event_t event){//UNUSEDCARD
	/* Myr Matrix	|5
	 * Artifact
	 * Indestructible
	 * Myr creatures get +1/+1.
	 * |5: Put a 1/1 colorless Myr artifact creature token onto the battlefield. */

	indestructible(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, MANACOST_X(5)) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, MANACOST_X(5));
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_MYR);
	}

	return boost_creature_type(player, card, event, -1, 1, 1, 0, BCT_INCLUDE_SELF);
}

int card_myr_moonvessel(int player, int card, event_t event){

	if( graveyard_from_play(player, card, event) ){
		produce_mana(player, COLOR_COLORLESS, 1);
	}
	return 0;
}


int card_nemesis_mask(int player, int card, event_t event)
{
  return vanilla_equipment(player, card, event, 3, 0,0, 0,SP_KEYWORD_LURE);
}

int card_nourish(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 6);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_nim_abomination(int player, int card, event_t event){

	if( ! is_tapped(player, card) && current_turn == player && ! is_humiliated(player, card) && eot_trigger(player, card, event) ){
		lose_life(player, 3);
	}
	return 0;
}

int card_oxidda_golem(int player, int card, event_t event){

	haste(player, card, event);

	affinity(player, card, event, TYPE_LAND, SUBTYPE_MOUNTAIN);

	return 0;
}

int card_oxidize(int player, int card, event_t event ){
	/*
	  Oxidize |G
	  Instant
	  Destroy target artifact. It can't be regenerated.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 1, NULL);
}

int card_panoptic_mirror(int player, int card, event_t event){
	/*
	  Panoptic Mirror |5
	  Artifact
	  Imprint - {X}, {T}: You may exile an instant or sorcery card with converted mana cost X from your hand.
	  At the beginning of your upkeep, you may copy a card exiled with Panoptic Mirror. If you do, you may cast the copy without paying its mana cost.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			if( player == HUMAN ){
				return 1;
			}
			else{
				int i;
				for(i=0; i<active_cards_count[player]; i++){
					if( in_hand(player, i) && is_what(player, i, TYPE_SPELL) ){
						if( has_mana_for_activated_ability(player, card, MANACOST_X(get_cmc(player, i))) ){
							return 1;
						}
					}
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( player == HUMAN ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(-1), 0, NULL, NULL);
		}
		else{
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_SPELL);
			this_test.type_flag = F1_NO_CREATURE;

			int imprintable[hand_count[player]];
			int ic = 0;
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_hand(player, i) && is_what(player, i, TYPE_SPELL) ){
					if( has_mana_for_activated_ability(player, card, MANACOST_X(get_cmc(player, i))) ){
						imprintable[ic] = get_card_instance(player, i)->internal_card_id;
						ic++;
					}
				}
			}
			int selected = select_card_from_zone(player, player, imprintable, ic, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST_X(get_cmc(player, i))) ){
					instance->info_slot = get_cmc(player, selected);
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
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a card to exile.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, msg);
		this_test.type_flag = F1_NO_CREATURE;
		this_test.cmc = instance->info_slot;
		int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
		if( selected != -1 ){
			card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
			int i = 0;
			while( i < 10 ){
					if( parent->targets[i].player == -1 ){
						int iid = get_original_internal_card_id(instance->parent_controller, selected);
						parent->targets[i].player = iid;
						create_card_name_legacy(instance->parent_controller, instance->parent_card, cards_data[iid].id);
						rfg_card_in_hand(instance->parent_controller, selected);
						break;
					}
					if( parent->targets[i].card == -1 ){
						int iid = get_original_internal_card_id(instance->parent_controller, selected);
						parent->targets[i].card = iid;
						create_card_name_legacy(instance->parent_controller, instance->parent_card, cards_data[iid].id);
						rfg_card_in_hand(instance->parent_controller, selected);
						break;
					}
					i++;
			}
		}
	}

	if( current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && reason_for_trigger_controller == player ){
		int trig = 0;
		int k;
		for(k=0; k<10; k++){
			int result = instance->targets[k].player;
			if( result == -1 ){
				break;
			}
			if( check_rfg(player, cards_data[result].id) && can_legally_play_iid(player, result) ){
				trig = 1;
				break;
			}
			result = instance->targets[k].card;
			if( result == -1 ){
				break;
			}
			if( check_rfg(player, cards_data[result].id) && can_legally_play_iid(player, result) ){
				trig = 1;
				break;
			}
		}

		upkeep_trigger_ability_mode(player, card, event, player, trig ? RESOLVE_TRIGGER_AI(player) : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int playable[20];
		int pc = 0;
		int k;
		for(k=0; k<10; k++){
			int result = instance->targets[k].player;
			if( result == -1 ){
				break;
			}
			if( check_rfg(player, cards_data[result].id) && can_legally_play_iid(player, result) ){
				playable[pc] = result;
				pc++;
			}
			result = instance->targets[k].card;
			if( result == -1 ){
				break;
			}
			if( check_rfg(player, cards_data[result].id) && can_legally_play_iid(player, result) ){
				playable[pc] = result;
				pc++;
			}
		}
		char msg[100] = "Select a spell to copy.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, msg);
		int selected = select_card_from_zone(player, player, playable, pc, 0, AI_MAX_CMC, -1, &this_test);
		if( selected != -1 ){
			copy_spell(player, cards_data[playable[selected]].id);
		}
	}

	return 0;
}

int card_pristine_angel(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	if( ! is_tapped(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_PRISTINE);
	}

	return 0;
}

// Pteron Ghost --> Welding Jar

int card_pulse_of_the_dross(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				ec_definition_t ec;
				default_ec_definition(instance->targets[0].player, player, &ec);
				ec.cards_to_reveal = 3;

				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);
				new_effect_coercion(&ec, &this_test);
			}
			if( hand_count[instance->targets[0].player] > hand_count[player] ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_pulse_of_the_fields(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 4);
		if( life[1-player] > life[player] ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_pulse_of_the_forge(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		int target = instance->targets[0].player;
		damage_player(target, 4, player, card);
		if( life[player] < life[target] && ! is_token(player, card) ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_pulse_of_the_grid(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		discard(player, 0, player);
		if( hand_count[1-player] > hand_count[player] ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_pulse_of_the_tangle(int player, int card, event_t event){
	/* Pulse of the Tangle	|1|G|G
	 * Sorcery
	 * Put a 3/3 |Sgreen Beast creature token onto the battlefield. Then if an opponent controls more creatures than you, return ~ to its owner's hand. */

	if( event == EVENT_RESOLVE_SPELL ){
		generate_token_by_id(player, card, CARD_ID_BEAST);
		if( count_permanents_by_type(1-player, TYPE_CREATURE) > count_permanents_by_type(player, TYPE_CREATURE) ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_razor_golem(int player, int card, event_t event){

	vigilance(player, card, event);

	affinity(player, card, event, TYPE_LAND, SUBTYPE_PLAINS);

	return 0;
}

int card_reap_and_sow(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);;

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						  "Kill a land", can_target(&td), 10,
						  "Tutor a land", 1, 15-(3*count_subtype(player, TYPE_LAND, -1)),
						  "Entwine", has_mana_multi(player, MANACOST_XG(1, 1)), 20);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice;
		}
		if( instance->info_slot == 3 && ! played_for_free(player, card) && ! is_token(player, card)){
			charge_mana_multi(player, MANACOST_XG(1, 1));
		}
		if( instance->info_slot & 1 ){
			instance->number_of_targets = 0;
			pick_target(&td, "TARGET_LAND");
		}
	}
	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			if(validate_target(player, card, &td, 0)){
				kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
			}
		}
		if( instance->info_slot & 2 ){
			char buffer[100];
			scnprintf(buffer, 100, "Select a land card");
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, buffer);
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_reshape(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_sacrifice_as_cost(player, 1,  TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, 0) ){
				instance->info_slot = x_value;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, instance->info_slot+1, 3);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_retract(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		manipulate_all(player, card, player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_BOUNCE);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

// ritual of restoration --> reconstruction

int card_savage_beating(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		int entwine = 0;
		int ai_choice = 1;
				char buffer[100];
				int pos = scnprintf(buffer, 100, " Double Strike to all\n Another combat phase\n");
				if( has_mana_multi(player, 1, 0, 0, 0, 1, 0) ){
					pos += scnprintf(buffer + pos, 100-pos, " Entwine\n");
					entwine = 1;
				}
				if( entwine ){
					ai_choice = 2;
				}
				pos += scnprintf(buffer + pos, 100-pos, " Pass");
				choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
				if( choice == 2 && ! entwine ){
					choice++;
				}
		if( choice == 3 ){
			spell_fizzled = 1;
		}
		if( choice == 2 ){
			charge_mana_multi(player, 1, 0, 0, 0, 1, 0);
		}
		instance->info_slot = 1+choice;

	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			pump_subtype_until_eot(player, card, player, -1, 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
		}
		if( instance->info_slot & 2  ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
			create_legacy_effect(player, card, &finest_hour_legacy);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_second_sight(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Rearrange opponent's deck", would_validate_arbitrary_target(&td, 1-player, -1), 2,
							"Rearrange your deck", 1, 1,
							"Entwine", has_mana(player, COLOR_BLUE, 1), 3);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == 1 ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
		}
		if( choice == 3 ){
			charge_mana(player, COLOR_BLUE, 1);
		}
		if( spell_fizzled != 1 ){
			instance->info_slot = 1+choice;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && valid_target(&td) ){
			rearrange_top_x(1-player, player, 5);
		}
		if( instance->info_slot & 2  ){
			rearrange_top_x(player, player, 5);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_serum_powder(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 7) ){ return 0; }
	return mana_producer(player, card, event);
}

int card_shield_of_kaldra(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_ABILITIES && ! is_humiliated(player, card) ){
		int id = get_id(affected_card_controller, affected_card);
		if( id == CARD_ID_SWORD_OF_KALDRA || id == CARD_ID_SHIELD_OF_KALDRA || id == CARD_ID_HELM_OF_KALDRA ){
			indestructible(affected_card_controller, affected_card, event);
		}
	}

	return vanilla_equipment(player, card, event, 4, 0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
}

static int shriveling_rot_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player & 2 ){
		damage_effects(player, card, event);
	}

	if( (instance->targets[0].player & 1) ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( in_play(affected_card_controller, affected_card) ){
				card_instance_t *affected = get_card_instance(player, card);
				if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE ){
					instance->targets[11].player = 1;
					instance->targets[2+affected_card_controller].player+=get_toughness(affected_card_controller,
																						affected_card);
				}
			}
		}
		if( resolve_graveyard_trigger(player, card, event) ){
			if( instance->targets[2].player > 0 ){
				lose_life(player, instance->targets[2].player);
				instance->targets[2].player = 0;
			}
			if( instance->targets[3].player > 0 ){
				lose_life(1-player, instance->targets[3].player);
				instance->targets[3].player = 0;
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_shriveling_rot(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"All damage is deadly", 1, 2,
						"Dead will drain life", 1, 1,
						"Entwine", has_mana_multi(player, MANACOST_XB(2, 1)), 3);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			if( choice == 2 ){
				charge_mana_multi(player, MANACOST_XB(2, 1));
			}
			if( spell_fizzled != 1 ){
				instance->info_slot = choice;
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_effect(player, card, &shriveling_rot_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[0].player = instance->info_slot;
		if( instance->info_slot & 2  ){
			leg->targets[2].card = get_id(player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_soulscour(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			manipulate_all(player, card, player, TYPE_ARTIFACT, 1, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
			manipulate_all(player, card, 1-player, TYPE_ARTIFACT, 1, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_specters_shroud(int player, int card, event_t event)
{
  /* Specter's Shroud	|2
   * Artifact - Equipment
   * Equipped creature gets +1/+0.
   * Whenever equipped creature deals combat damage to a player, that player discards a card.
   * Equip |1 */

  if (equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRACE_DAMAGED_PLAYERS))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
	  instance->targets[1].player = 0;
	  int p;
	  for (p = 0; p <= 1; ++p)
		new_multidiscard(p, times_damaged[p], 0, player);
	}

  return vanilla_equipment(player, card, event, 1, 1, 0, 0, 0);
}

int card_spellbinder(int player, int card, event_t event){

	/* Spellbinder	|3
	 * Artifact - Equipment
	 * Imprint - When ~ enters the battlefield, you may exile an instant card from your hand.
	 * Whenever equipped creature deals combat damage to a player, you may copy the exiled card. If you do, you may cast the copy without paying its mana cost.
	 * Equip |4 */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int result = global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_RFG, 0, 2, TYPE_INSTANT, 2, 0, 0, 0, 0, 0, 0, -1, 0);
		instance->targets[2].card = result;
		if( result > -1 ){
			create_card_name_legacy(player, card, result);
		}
	}

	int packets;
	if (instance->targets[2].card > -1 &&
		(packets = equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRIGGER_OPTIONAL))
	   ){
		for (; packets > 0; --packets){
			copy_spell(player, instance->targets[2].card);
		}
	}

	return vanilla_equipment(player, card, event, 4, 0, 0, 0, 0);
}

int card_spire_golem(int player, int card, event_t event){

	affinity(player, card, event, TYPE_LAND, SUBTYPE_ISLAND);

	return 0;
}

int card_stand_together(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				add_1_1_counters(instance->targets[i].player, instance->targets[i].card, 2);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_stir_the_pride(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if (event == EVENT_RESOLVE_SPELL || (event == EVENT_CAST_SPELL && affect_me(player, card))){
		int choice = DIALOG(player, card, event,
							"+2/+2", 1, 10,	// Choice 1
							"Spirit Link", 1, life[player] >= 10 ? 5 : 15,	// Choice 2.  If can't entwine, pick this only if life is under 10.
							"Entwine", has_mana_multi(player, MANACOST_XW(1,1)), 20);	// Choice 3.  If can entwine, do so.

		if (event == EVENT_CAST_SPELL && affect_me(player, card)){
			if (choice == 3){
				charge_mana_multi(player, MANACOST_XW(1,1));
			}
		} else {	// event == EVENT_RESOLVE_SPELL
			int c, pump = (choice & 1) ? 2 : 0;
			for (c = active_cards_count[player] - 1; c >= 0; --c){
				if (in_play(player, c) && is_what(player, c, TYPE_CREATURE)){
					int legacy = pump_ability_until_eot(player, card, player, c, pump, pump, 0, 0);
					if (choice & 2){
						card_instance_t* leg = get_card_instance(player, legacy);
						if (leg->targets[4].card == -1){
							leg->targets[4].card = 0;
						}
						leg->targets[4].card |= 3;
					}
				}
			}

			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_sundering_titan(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( (comes_into_play(player, card, event) > 0 || leaves_play(player, card, event)) ){
		instance->number_of_targets = 0;

		// target each land
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allow_cancel = 0;

		int trgs = 0;

		td.required_subtype = SUBTYPE_SWAMP;
		if( target_available(player, card, &td) ){
			if( select_target(player, card, &td, "Choose a Swamp", &(instance->targets[trgs])) ){
				trgs++;
			}
		}

		td.required_subtype = SUBTYPE_ISLAND;
		if( target_available(player, card, &td) ){
			if( select_target(player, card, &td, "Choose an Island", &(instance->targets[trgs])) ){
				trgs++;
			}
		}

		td.required_subtype = SUBTYPE_FOREST;
		if( target_available(player, card, &td) ){
			if( select_target(player, card, &td, "Choose a Forest", &(instance->targets[trgs])) ){
				trgs++;
			}
		}


		td.required_subtype = SUBTYPE_MOUNTAIN;
		if( target_available(player, card, &td) ){
			if( select_target(player, card, &td, "Choose a Mountain", &(instance->targets[trgs])) ){
				trgs++;
			}
		}


		td.required_subtype = SUBTYPE_PLAINS;
		if( target_available(player, card, &td) ){
			if( select_target(player, card, &td, "Choose a Plains", &(instance->targets[trgs])) ){
				trgs++;
			}
		}

		// Now destroy each land
		int i;
		for(i=0;i<trgs;i++){
			if( instance->targets[i].player != -1 && in_play( instance->targets[i].player, instance->targets[i].card ) ){
				kill_card( instance->targets[i].player, instance->targets[i].card, KILL_DESTROY );
			}
		}

	}

	return 0;
}

int card_surestrike_trident(int player, int card, event_t event){

	/* Surestrike Trident	|2
	 * Artifact - Equipment
	 * Equipped creature has first strike and "|T, Unattach ~: This creature deals damage equal to its power to target player."
	 * Equip |4 */

	card_instance_t *instance = get_card_instance(player, card);

	if (is_equipping(player, card)){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 0, 0, KEYWORD_FIRST_STRIKE);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_ACTIVATE ){
		if( is_equipping(player, card) &&
			generic_activated_ability(instance->targets[8].player, instance->targets[8].card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td1, NULL)
		  ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 4);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( can_activate_basic_equipment(player, card, event, 4) ){
			if( is_equipping(player, card) &&
				generic_activated_ability(instance->targets[8].player, instance->targets[8].card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0,
										0, &td1, NULL)
			  ){
				choice = do_dialog(player, player, card, -1, -1, " Equip\n Tap to damage a player\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			activate_basic_equipment(player, card, 4);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66;
			}
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(instance->targets[8].player, instance->targets[8].card, MANACOST0) ){
					instance->targets[2].player = get_protections_from(instance->targets[8].player, instance->targets[8].card);
					td1.illegal_abilities = instance->targets[2].player;
					if( pick_target(&td1, "TARGET_CREATURE") ){
						instance->targets[2].card = get_power(instance->targets[8].player, instance->targets[8].card);
						instance->targets[3] = instance->targets[8];
						tap_card(instance->targets[8].player, instance->targets[8].card);
						unattach(player, card);
						instance->number_of_targets = 1;
						instance->info_slot = 67;
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

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			resolve_activation_basic_equipment(player, card);
		}
		if( instance->info_slot == 67 ){
			td1.illegal_abilities = instance->targets[2].player;
			if( valid_target(&td1) ){
				damage_player(instance->targets[0].player, instance->targets[2].card, instance->targets[3].player, instance->targets[3].card);
			}
		}
	}

	return 0;
}

int card_talon_of_pain(int player, int card, event_t event){

	/* Talon of Pain	|4
	 * Artifact
	 * Whenever a source you control other than ~ deals damage to an opponent, put a charge counter on ~.
	 * |X, |T, Remove X charge counters from ~: ~ deals X damage to target creature or player. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t* damage = damage_being_dealt(event);
	if (damage &&
		damage->damage_source_player == player && damage->damage_source_card != card &&
		damage->damage_target_player == 1-player && damage->damage_target_card == -1
	   ){
		add_counter(player, card, COUNTER_CHARGE);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		count_counters(player, card, COUNTER_CHARGE) > 0 && has_mana(player, COLOR_COLORLESS, 1) &&
		!is_tapped(player, card) && !is_animated_and_sick(player, card)
	  ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int amount = 0;
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				if( x_value > count_counters(player, card, COUNTER_CHARGE) ){
					amount = count_counters(player, card, COUNTER_CHARGE);
				}
				else{
					amount = x_value;
				}
				if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					get_card_instance(player, card)->info_slot = amount;
					remove_counters(player, card, COUNTER_CHARGE, amount);
					tap_card(player, card);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, get_card_instance(player, card)->info_slot);
		}
	}

	return 0;
}

int card_tangle_golem(int player, int card, event_t event){

	affinity(player, card, event, TYPE_LAND, SUBTYPE_FOREST);

	return 0;
}

int card_test_of_faith(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td)  ){
			prevent_the_next_n_damage(player, card, instance->targets[0].player, instance->targets[0].card, 3, PREVENT_ADD_1_1_COUNTER, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_thought_dissector(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[instance->targets[0].player];
			int count = 0;
			int found = 0;
			while( deck[count] != -1 && count < instance->info_slot ){
					if( is_what(-1, deck[count], TYPE_ARTIFACT) ){
						found = 1;
						break;
					}
					count++;
			}
			if( found == 1 ){
				put_into_play_a_card_from_deck(player, instance->targets[0].player, count);
				count--;
				if( in_play(player, instance->parent_card) ){
					kill_card(player, instance->parent_card, KILL_SACRIFICE);
				}
			}
			if( count > 0 ){
				mill(instance->targets[0].player, count);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_X(-1), 0, &td, "TARGET_PLAYER");
}

int card_thunderstaff(int player, int card, event_t event)//UNUSEDCARD
{
  // As long as ~ is untapped, if a creature would deal combat damage to you, prevent 1 of that damage.
  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage
	  && !is_tapped(player, card)
	  && (damage->targets[3].player & TYPE_CREATURE)
	  && damage->damage_target_player == player && damage->damage_target_card == -1 && !damage_is_to_planeswalker(damage))
	--damage->info_slot;

  // |2, |T: Attacking creatures get +1/+0 until end of turn.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  pump_creatures_until_eot(player, card, current_turn, 0, 1,0, 0,0, &test);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
}

extern int dont_account_for_trinisphere;
int card_trinisphere(int player, int card, event_t event){
	if( ! is_tapped(player, card) ){
		if( event == EVENT_MODIFY_COST_GLOBAL && ! is_what(affected_card_controller, affected_card, TYPE_LAND) ){
			++dont_account_for_trinisphere;
			int cless = get_updated_casting_cost(affected_card_controller, affected_card, -1, event, -1);
			--dont_account_for_trinisphere;
			card_ptr_t* c = cards_ptr[ get_id(affected_card_controller, affected_card)  ];
			int cmc = cless + c->req_black + c->req_blue + c->req_green + c->req_red + c->req_white;
			if( cmc < 3 ){
				COST_COLORLESS += 3 - cmc;
			}
		}
	}
	return 0;
}

int card_turn_the_table(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td)  ){
			int legacy = create_legacy_effect(player, card, &damage_redirection);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[0].player = player;
			leg->targets[0].card = -1;
			leg->targets[1] = instance->targets[0];
			instance->number_of_targets = 2;
			leg->targets[2].player = 1;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_vex(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if (counterspell_validate(player, card, &td, 0)){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			draw_some_cards_if_you_want(player, card, instance->targets[0].player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, &td, "", 0, NULL);
}

// Viridian Acolyte --> Boreal Druid

int card_viridian_zealot(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_XG(1, 1), 0, &td, "DISENCHANT");
}

int card_voltaic_construct(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_ARTIFACT_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

int card_vulshok_morningstar(int player, int card, event_t event){

	return vanilla_equipment(player, card, event, 2, 2, 2, 0, 0);
}

int card_vulshok_war_boar(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! can_target(&td) ){
			ai_modifier-=1000;
		}
	}

	if( comes_into_play(player, card, event) ){
		impose_sacrifice(player, card, player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_wand_of_the_elements(int player, int card, event_t event){
	/* Wand of the Elements	|4
	 * Artifact
	 * |T, Sacrifice |Han Island: Put a 2/2 |Sblue Elemental creature token with flying onto the battlefield.
	 * |T, Sacrifice |Ha Mountain: Put a 3/3 |Sred Elemental creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			if( can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0) ){
				return 1;
			}
			if( can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_MOUNTAIN, 0, 0, 0, 0, 0, -1, 0) ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int choice = 0;
			if( can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0) ){
				if( can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_MOUNTAIN, 0, 0, 0, 0, 0, -1, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Sac an Island\n Sac a Mountain\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				int result = pick_special_permanent_for_sacrifice(player, card, 0, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0);
				if( result != -1 ){
					kill_card(player, result, KILL_SACRIFICE);
					tap_card(player, card);
					instance->info_slot = 66+choice;
				}
				else{
					spell_fizzled = 1;
				}
			}
			else if( choice == 1 ){
					int result = pick_special_permanent_for_sacrifice(player, card, 0, TYPE_LAND, 0, SUBTYPE_MOUNTAIN, 0, 0, 0, 0, 0, -1, 0);
					if( result != -1 ){
						kill_card(player, result, KILL_SACRIFICE);
						tap_card(player, card);
						instance->info_slot = 66+choice;
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

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		if( instance->info_slot == 66 ){
			token.pow = 2;
			token.tou = 2;
			token.key_plus = KEYWORD_FLYING;
			token.color_forced = COLOR_TEST_BLUE;
		}
		if( instance->info_slot == 67 ){
			token.pow = 3;
			token.tou = 3;
			token.color_forced = COLOR_TEST_RED;
		}
		generate_token(&token);
	}

	return 0;
}
