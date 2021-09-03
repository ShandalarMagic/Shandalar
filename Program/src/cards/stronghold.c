#include "manalink.h"

// --- GLOBAL FUNCTIONS ---

int generic_spike(int player, int card, event_t event, int counters){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, counters);

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
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_1_1_COUNTER, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

// --- CARDS

int acidic_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}


		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td2.allow_cancel = 0;

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( would_validate_arbitrary_target(&td2, instance->targets[2].player, instance->targets[2].card) ){
				damage_creature(instance->targets[2].player, instance->targets[2].card, 2, player, card);
			}
		}

		return shared_sliver_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_X(2), 0, &td2, "TARGET_CREATURE_OR_PLAYER");
	}

	return 0;
}

int card_acidic_sliver(int player, int card, event_t event){
	/*
	  Acidic Sliver |B|R
	  Creature - Sliver 2/2
	  All Slivers have "{2}, Sacrifice this permanent: This permanent deals 2 damage to target creature or player."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &acidic_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &acidic_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int card_amok(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_DISCARD | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		charge_mana_for_activated_ability(player, card, MANACOST_X(1));
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
			discard(player, DISC_RANDOM, player);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_awakening2(int player, int card, event_t event){

	if( upkeep_trigger(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE | TYPE_LAND);
		new_manipulate_all(player, card, 2, &this_test, ACT_UNTAP);
	}

	return global_enchantment(player, card, event);
}

int card_bottomless_pit(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		discard(current_turn, DISC_RANDOM, player);
	}

	return global_enchantment(player, card, event);
}

int card_brush_with_death(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		instance->info_slot = buyback(player, card, MANACOST_XB(2, 2));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 2);
			gain_life(player, 2);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bullwhip(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, instance->parent_controller, instance->parent_card);
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, 0, SP_KEYWORD_MUST_ATTACK);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

int card_burgeoning(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me( player, card ) && reason_for_trigger_controller == affected_card_controller &&
		trigger_cause_controller != player && ! is_humiliated(player, card)
	 ){
		int trig = 0;

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land card to put into play.");
		this_test.zone = TARGET_ZONE_HAND;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) && check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_AI(player);
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_cannibalize(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		int i;
		for(i=0; i<2; i++){
			if( player == HUMAN || (player == AI && i == 1-player) ){
				td.allowed_controller = i;
				td.preferred_controller = i;
				if( generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 2, NULL) ){
					return 1;
				}
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( new_pick_target(&td, "Select target creature to exile.", 0, 1 | GS_LITERAL_PROMPT) ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			td.allowed_controller = instance->targets[0].player;
			td.preferred_controller = instance->targets[0].player;
			if( can_target(&td) ){
				new_pick_target(&td, "Select target creature for the +1/+1 counters.", 1, 1 | GS_LITERAL_PROMPT);
			}
			else{
				spell_fizzled = 1;
			}
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		td.allowed_controller = instance->targets[0].player;
		td.preferred_controller = instance->targets[0].player;
		if( validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		if( validate_target(player, card, &td, 1) ){
			add_1_1_counters(instance->targets[1].player, instance->targets[1].card, 2);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}


int card_change_of_heart(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( current_turn == player || current_phase > PHASE_DECLARE_ATTACKERS ){
			ai_modifier-=50;
		}
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE")  ){
			instance->info_slot = buyback(player, card, MANACOST_X(3));
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if (valid_target(&td)){
			create_targetted_legacy_effect(player, card, effect_cannot_attack_until_eot, instance->targets[0].player, instance->targets[0].card);

			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_constant_mists(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! is_token(player, card) ){
			if( can_sacrifice_type_as_cost(player, 1, TYPE_LAND) ){
				int choice = do_dialog(player, player, card, -1, -1, " Pay Buyback\n Pass", count_subtype(player, TYPE_LAND, -1) < 4);
				if( choice == 0 ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_LAND, "Select a land to sacrifice");

					if( new_sacrifice(player, card, player, 0, &this_test) ){
						instance->info_slot = 66;
					}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		fog_effect(player, card);
		if( instance->info_slot == 66 ){
			bounce_permanent(player, card);
			return 0;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_contemplation(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		gain_life(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_crovax_the_cursed(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // ~ enters the battlefield with four +1/+1 counters on it.
  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 4);

  // At the beginning of your upkeep, you may sacrifice a creature. If you do, put a +1/+1 counter on ~. If you don't, remove a +1/+1 counter from ~.
  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  int ai_priority_sac = 0;
	  if (player == AI || ai_is_speculating == 1)
		{
		  target_definition_t td;
		  default_target_definition(player, card, &td, TYPE_CREATURE);
		  td.allowed_controller = player;
		  td.preferred_controller = player;
		  td.illegal_abilities = 0;
		  td.special = TARGET_SPECIAL_NOT_ME;

		  ai_priority_sac = can_target(&td) ? 3 : -1;
		}

	  if (DIALOG(player, card, EVENT_ACTIVATE,
				 DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_RANDOM,
				 "Sacrifice for Crovax", 1, ai_priority_sac,
				 "Pass", 1, 1) == 1
		  && controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0))
		{
		  if (in_play(player, card))	// might've sacced to himself, for some reason
			add_1_1_counter(player, card);
		}
	  else
		remove_1_1_counter(player, card);

	  get_card_instance(player, card)->number_of_targets = 0;
	}

  // |B: Crovax gains flying until end of turn.
  return generic_shade(player, card, event, 0, MANACOST_B(1), 0,0, KEYWORD_FLYING,0);
}

int card_crystalline_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_SHROUD, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

static int dream_halls_effect(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && hand_count[player] > 1 ){
		int result = 0;
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && get_color(player, i) != COLOR_TEST_COLORLESS ){
				int can_play_this_spell = can_legally_play_iid(player, get_card_instance(player, i)->internal_card_id);
				if( can_play_this_spell ){
					int clr = get_color(player, i);
					int k;
					for(k=0; k<active_cards_count[player]; k++){
						if( k != i && in_hand(player, k) ){
							if( get_color(player, k) & clr ){
								if( can_play_this_spell == 99 ){
									return 99;
								}
								else{
									if( is_what(player, i, TYPE_INSTANT) || can_sorcery_be_played(player, event) ){
										result = 1;
									}
									break;
								}
							}
						}
					}
				}
			}
		}
		return result;
	}

	if(event == EVENT_ACTIVATE && affect_me(player, card) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select a card to play with Dream Halls.");

		int pwdh[2][hand_count[player]];
		int pwdhc = 0;
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && get_color(player, i) != COLOR_TEST_COLORLESS ){
				int can_play_this_spell = can_legally_play_iid(player, get_card_instance(player, i)->internal_card_id);
				if( can_play_this_spell && (is_what(player, i, TYPE_INSTANT | TYPE_INTERRUPT) || can_sorcery_be_played(player, event))){
					int clr = get_color(player, i);
					int k;
					for(k=0; k<active_cards_count[player]; k++){
						if( k != i && in_hand(player, k) ){
							if( get_color(player, k) & clr ){
								pwdh[0][pwdhc] = get_card_instance(player, i)->internal_card_id;
								pwdh[1][pwdhc] = i;
								pwdhc++;
								break;
							}
						}
					}
				}
			}
		}
		int selected = select_card_from_zone(player, player, pwdh[0], pwdhc, 0, AI_MAX_VALUE, -1, &test);
		if( selected != -1 ){
			int clr = get_color(player, pwdh[1][selected]);

			int ctsc[2][hand_count[player]];
			int ctscc = 0;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_hand(player, i) && i != pwdh[1][selected]){
					if( get_color(player, i) & clr ){
						ctsc[0][ctscc] = get_card_instance(player, i)->internal_card_id;
						ctsc[1][ctscc] = i;
						ctscc++;
					}
				}
			}
			if( ctscc == 0 ){
				spell_fizzled = 1;
				return 0;
			}
			strcpy(test.message, "Select a card which shares a color with the previous.");
			int sel2 = select_card_from_zone(player, player, ctsc[0], ctscc, 0, AI_MIN_VALUE, -1, &test);
			if( sel2 != -1 ){
				discard_card(player, ctsc[1][sel2]);
				play_card_in_hand_for_free(player, pwdh[1][selected]);
				cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
			}
			else {
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}

static int dream_halls_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		if( leaves_play(instance->targets[0].player, instance->targets[0].card, event) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return dream_halls_effect(player, card, event);
}

int card_dream_halls(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int fake = add_card_to_hand(1-player, get_original_internal_card_id(player, card));
		int legacy = create_legacy_activate(1-player, fake, &dream_halls_legacy);
		card_instance_t *instance = get_card_instance(1-player, legacy);
		instance->targets[0].player = player;
		instance->targets[0].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	if( IS_GAA_EVENT(event) ){
		return dream_halls_effect(player, card, event);
	}

	return global_enchantment(player, card, event);
}

int card_elven_rite(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			add_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
			new_pick_target(&td, "TARGET_CREATURE", 1, 0);
			remove_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
			if (instance->number_of_targets == 2
				&& instance->targets[0].card == instance->targets[1].card
				&& instance->targets[0].player == instance->targets[1].player){
				instance->number_of_targets = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				add_1_1_counters(instance->targets[i].player, instance->targets[i].card, (2 / instance->number_of_targets));
			}
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_ensnaring_bridge(int player, int card, event_t event){
	/*
	  Ensnaring Bridge |3
	  Artifact
	  Creatures with power greater than the number of cards in your hand can't attack.
	*/

	if( event == EVENT_ATTACK_LEGALITY && ! is_humiliated(player, card) && get_power(affected_card_controller, affected_card) > hand_count[player] ){
		event_result = 1;
	}

	return 0;
}

int card_evacuation(int player, int card, event_t event){
	/*
	  Evacuation |3|U|U
	  Instant
	  Return all creatures to their owners' hands.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, ANYBODY, &this_test, ACT_BOUNCE);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_fanning_the_flames(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			instance->info_slot = x_value;
			instance->targets[1].player = buyback(player, card, MANACOST_X(3));
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, instance->info_slot);
			if( instance->targets[1].player == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_flame_wave(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_damage_all(player, card, instance->targets[0].player, 4, NDA_ALL_CREATURES+NDA_PLAYER_TOO, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_flowstone_mauler(int player, int card, event_t event){
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_R(1), 1, -1);
}

int card_flowstone_hellion(int player, int card, event_t event){
	haste(player, card, event);
	return generic_shade_merge_pt(player, card, event, 0, MANACOST0, 1, -1);
}

int card_foul_imp(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		lose_life(player, 2);
	}

	return 0;
}

int card_grave_pact(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);

	if( in_play(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		impose_sacrifice(player, card, 1-player, instance->targets[11].card, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_hammerhead_shark(int player, int card, event_t event)
{
  if (event == EVENT_ATTACK_LEGALITY && affect_me(player, card)
	  && !basiclandtypes_controlled[1 - player][get_hacked_color(player, card, COLOR_BLUE)])
	event_result = 1;

  return 0;
}

int card_heartstone(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		set_cost_mod_for_activated_abilities(2, -1, 1, &this_test);
	}

	if( event == EVENT_CAST_SPELL ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			set_cost_mod_for_activated_abilities(affected_card_controller, affected_card, 1, 0);
		}
	}


	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		remove_cost_mod_for_activated_abilities(2, -1, 1, &this_test);
	}

	return 0;
}

int card_hermit_druid(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION  ){
		int *deck = deck_ptr[player];
		int count = 0;
		int good = 0;
		while( deck[count] != -1 ){
				if( is_basic_land_by_id(cards_data[deck[count]].id) ){
					good = 1;
					break;
				}
				else{
					count++;
				}
		}
		if( count > 0 ){
			show_deck( HUMAN, deck, count+1, "Cards revealed by Hermit Druid", 0, 0x7375B0 );
			mill(player, count);
		}
		if( good == 1 ){
			add_card_to_hand(player, deck[0]);
			remove_card_from_deck(player, 0);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_G(1), 0, NULL, NULL);
}

int card_hesitation(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
		kill_card(player, card, KILL_SACRIFICE);
	}

	return global_enchantment(player, card, event);
}

int hybernating_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		if( event == EVENT_RESOLVE_ACTIVATION && in_play(instance->targets[0].player, instance->targets[0].card) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}

		return shared_sliver_activated_ability(player, card, event, 0, MANACOST0, 2, NULL, NULL);
	}

	return 0;
}

int card_hybernating_sliver(int player, int card, event_t event){
	/*
	  Hibernation Sliver |U|B
	  Creature - Sliver 2/2
	  All Slivers have "Pay 2 life: Return this permanent to its owner's hand."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &hybernating_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &hybernating_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int card_horn_of_greed(int player, int card, event_t event){
	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(trigger_cause_controller, trigger_cause);
		if( cards_data[ instance->internal_card_id].type & TYPE_LAND ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					draw_a_card(trigger_cause_controller);
			}
		}
	}
	return 0;
}

int card_intruder_alarm(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
	  // Creatures don't untap during their controllers' untap steps.
	  if (current_phase == PHASE_UNTAP && event == EVENT_UNTAP && is_what(affected_card_controller, affected_card, TYPE_CREATURE))
		get_card_instance(affected_card_controller, affected_card)->untap_status &= ~3;

	  // Whenever a creature enters the battlefield, untap all creatures.
	  if (specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0))
		{
		  test_definition_t test;
		  default_test_definition(&test, TYPE_CREATURE);
		  new_manipulate_all(player, card, ANYBODY, &test, ACT_UNTAP);
		}

	  // AI
	  if (event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI && check_battlefield_for_id(ANYBODY, CARD_ID_INTRUDER_ALARM))
		ai_modifier -= 64;

	  if (event == EVENT_SHOULD_AI_PLAY)
		{
		  int p, c, mod;
		  for (p = 0; p <= 1; ++p)
			{
			  for (c = mod = 0; c < active_cards_count[p]; ++c)
				if (in_play(p, c) && is_what(p, c, TYPE_CREATURE))
				  {
					if (!check_for_special_ability(player, card, SP_KEYWORD_VIGILANCE))
					  mod += get_power(p, c);

					if (get_card_data(p, c)->extra_ability & EA_MANA_SOURCE)
					  mod -= 4;
				  }

			  if (p == HUMAN)
				ai_modifier += 12 * mod;
			  else
				ai_modifier -= 12 * mod;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_jinxed_ring(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.type_flag = F1_NO_TOKEN;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( in_play(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		damage_player(player, instance->targets[11].card, player, card);
		instance->targets[11].card = 0;
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			give_control_of_self(instance->parent_controller, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT | GAA_SACRIFICE_CREATURE, MANACOST0, 0, &td, NULL);
}

int card_lab_rats(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = buyback(player, card, MANACOST_X(4));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_RAT, &token);
		token.pow = token.tou = 1;
		generate_token(&token);
		if( instance->info_slot == 1 ){
			bounce_permanent(player, card);
			return 0;
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// lancer en-kor --> nomads en-kor

void lowland_basilisk_effect(int player, int card, int t_player, int t_card){
	create_targetted_legacy_effect(player, card, &die_at_end_of_combat, t_player, t_card);
}

int card_lowland_basilisk(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if (event == EVENT_CHANGE_TYPE && affect_me(player, card) ){
			card_instance_t* instance = get_card_instance(player, card);
			instance->destroys_if_blocked |= DIFB_DESTROYS_UNPROTECTED;
		}

		for_each_creature_damaged_by_me(player, card, event, 0, &lowland_basilisk_effect, player, card);
	}

	return 0;
}

int card_mana_leak(int player, int card, event_t event){
	/*
	  Mana Leak |1|U
	  Instant
	  Counter target spell unless its controller pays {3}.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, 3);
		kill_card(player, card, KILL_DESTROY);
	}
	else{
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_mask_of_the_mimic(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_type = TARGET_TYPE_TOKEN;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_ptr_t* c = cards_ptr[ get_id(instance->targets[0].player, instance->targets[0].card)  ];
			char msg[100];
			scnprintf(msg, 100, "Select a card named %s", c->name);
			test_definition_t this_test2;
			new_default_test_definition(&this_test2, TYPE_CREATURE, msg);
			this_test2.id = get_id(instance->targets[0].player, instance->targets[0].card);
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test2);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_SAC_CREATURE_AS_COST | GS_LITERAL_PROMPT, &td, "Select target nontoken creature.", 1, NULL);
}

int card_megrim(int player, int card, event_t event){
	/*
	  Megrim |2|B
	  Enchantment
	  Whenever an opponent discards a card, Megrim deals 2 damage to that player.
	*/

	if( ! is_humiliated(player, card) && discard_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, 0) ){
		damage_player(1-player, 2, player, card);
	}

	return global_enchantment(player, card, event);
}

int card_mind_games(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TWIDDLE") ){
			instance->info_slot = buyback(player, card, MANACOST_XU(2, 1));
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mind_peel(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_PLAYER") ){
			instance->info_slot = buyback(player, card, MANACOST_XB(2, 2));
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			discard(instance->targets[0].player, 0, player);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mindwarper(int player, int card, event_t event){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 3);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			discard(instance->targets[0].player, 0, player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_SORCERY_BE_PLAYED | GAA_1_1_COUNTER | GAA_CAN_TARGET, MANACOST_XB(2, 1), 0,
									&td, "TARGET_PLAYER");
}

int card_mob_justice(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, count_subtype(player, TYPE_CREATURE, -1), player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_mogg_bombers(int player, int card, event_t event){

	if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			damage_target0(player, card, 3);
		}

		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_mogg_flunkies(int player, int card, event_t event)
{
	cannot_attack_alone(player, card, event);
	cannot_block_alone(player, card, event);
	return 0;
}

int card_mogg_infestation(int player, int card, event_t event){
	//APPROXIMATION
	/*
	  Mogg Infestation |3|R|R
	  Sorcery
	  Destroy all creatures target player controls. For each creature that died this way, put two 1/1 red Goblin creature tokens
	  onto the battlefield under that player's control.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t test;
			default_test_definition(&test, TYPE_CREATURE);
			int amount = new_manipulate_all(player, card, instance->targets[0].player, &test, KILL_DESTROY);

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_GOBLIN, &token);
			token.t_player = instance->targets[0].player;
			token.qty = amount * 2;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_mogg_maniac(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
	  card_instance_t* damage = damage_being_dealt(event);
	  if (damage
		  && damage->damage_target_card == card && damage->damage_target_player == player)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  if (instance->info_slot < 10)
			{
			  if (instance->info_slot < 1)
				instance->info_slot = 1;

			  instance->targets[instance->info_slot++].player = damage->info_slot;
			}
		}

	  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  if (instance->info_slot > 1)
			{
			  if (event == EVENT_TRIGGER)
				event_result |= RESOLVE_TRIGGER_MANDATORY;

			  if (event == EVENT_RESOLVE_TRIGGER)
				{
				  target_definition_t td;
				  default_target_definition(player, card, &td, 0);
				  td.zone = TARGET_ZONE_PLAYERS;

				  // Free up info_slot and targets, just in case something triggers instantly (and improperly) which eventually results in dealing damage back.
				  int i, dam[10];
				  for (i = 1; i < instance->info_slot; ++i)
					dam[i] = instance->targets[i].player;

				  int numdam = instance->info_slot;
				  instance->info_slot = 1;

				  for (i = 1; i < numdam; ++i)
					{
					  if ( would_validate_arbitrary_target(&td, 1-player, -1) )
						damage_player(1-player, dam[i], player, card);
					}
				}
			}
		}
	}

	return 0;
}

int card_morgue_thrull(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		mill(player, 3);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}

int card_mortuary(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_humiliated(player, card) ){
		count_for_gfp_ability_and_store_values(player, card, event, player, TYPE_CREATURE, NULL, GFPC_TRACK_DEAD_CREATURES | GFPC_EXTRA_SKIP_TOKENS, 0);

		if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
			int k;
			int total = 0;
			for(k=0; k<10; k++){
				if( instance->targets[k].player != -1 ){
					int result = seek_grave_for_id_to_reanimate(player, -1, player, cards_data[instance->targets[k].player].id, REANIMATEXTRA_LEAVE_IN_GRAVEYARD);
					if( result != -1 ){
						int card_added = add_card_to_hand(player, instance->targets[k].player);
						remove_card_from_grave(player, result);
						put_on_top_of_deck(player, card_added);
						total++;
					}
					instance->targets[k].player = -1;
				}
				if( instance->targets[k].card != -1 ){
					int result = seek_grave_for_id_to_reanimate(player, -1, player, cards_data[instance->targets[k].card].id, REANIMATEXTRA_LEAVE_IN_GRAVEYARD);
					if( result != -1 ){
						int card_added = add_card_to_hand(player, instance->targets[k].card);
						remove_card_from_grave(player, result);
						put_on_top_of_deck(player, card_added);
						total++;
					}
					instance->targets[k].card = -1;
				}
			}
			if( total > 1 ){
				rearrange_top_x(player, player, total);
			}
			instance->targets[11].player = 0;
		}
	}
	return global_enchantment(player, card, event);
}

int card_mox_diamond(int player, int card, event_t event)
{
	/* Mox Diamond	|0
	 * Artifact
	 * If ~ would enter the battlefield, you may discard a land card instead. If you do, put ~ onto the battlefield. If you don't, put it into its owner's
	 * graveyard.
	 * |T: Add one mana of any color to your mana pool. */

	if (event == EVENT_RESOLVE_SPELL){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.zone = TARGET_ZONE_HAND;

		if(!select_target(player, card, &td, "Discard a land card.", NULL)){
			kill_card(player, card, KILL_STATE_BASED_ACTION);
		}
		else{
			card_instance_t* instance = get_card_instance(player, card);
			instance->number_of_targets = 0;
			discard_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return artifact_mana_all_one_color(player, card, event, 1, 0);
}

static int kor_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	int t_player = instance->targets[0].player;
	int t_card = instance->targets[0].card;

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( source->damage_target_player != t_player || source->damage_target_card != t_card ){
			return 0;
		}

		if( damage_card != source->internal_card_id ){
			return 0;
		}

		if( source->info_slot <= 0 ){
			return 0;
		}

		else{
			 damage_creature(instance->targets[1].player, instance->targets[1].card, 1,
							 source->damage_source_player, source->damage_source_card);
			 source->info_slot--;
			 kill_card(player, card, KILL_REMOVE);
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;

}

int card_nomads_en_kor(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int legacy = create_targetted_legacy_effect(player, card, &kor_effect, player, instance->parent_card);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[1].player = instance->targets[0].player;
		leg->targets[1].card = instance->targets[0].card;
		leg->number_of_targets = 2;
	}

	return generic_activated_ability(player, card, event, GAA_NOT_ME_AS_TARGET | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_overgrowth(int player, int card, event_t event){
	return wild_growth_aura(player, card, event, -1, COLOR_GREEN, 2);
}

static int portcullis_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_LEAVE_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == player ){

		int trig = 0;

		if( trigger_cause_controller == instance->targets[0].player && trigger_cause == instance->targets[0].card ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			remove_status(player, card, STATUS_INVISIBLE_FX);
			trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if( event == EVENT_RESOLVE_TRIGGER){
				   int i;
				   for( i=2; i<instance->targets[1].card; i++){
						int iid = instance->targets[i].card;
						if( check_rfg(instance->targets[i].player, cards_data[iid].id) ){
							int card_added = add_card_to_hand(instance->targets[i].player, iid);
							remove_card_from_rfg(instance->targets[i].player, cards_data[iid].id);
							put_into_play(instance->targets[i].player, card_added);
						}
				   }
				   kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	return 0;
}

int card_portcullis(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_my_legacy(player, card, &portcullis_effect);
		add_status(player, legacy, STATUS_INVISIBLE_FX);
		instance->info_slot = legacy;
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) && reason_for_trigger_controller == affected_card_controller &&
		! check_state(player, card, STATE_CANNOT_TARGET)
	  ){
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) && count_subtype(2, TYPE_CREATURE, -1) > 2 ){
			test_definition_t test;
			default_test_definition(&test, TYPE_CREATURE);
			if( new_specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, &test) ){
				int result = exile_permanent_and_remember_it(player, instance->info_slot, instance->targets[1].player, instance->targets[1].card, 1);
				if( result > -1 ){
					create_card_name_legacy(player, card, cards_data[result].id);
				}
			}
		}
	}

	return 0;
}

int card_primal_rage(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && ! is_humiliated(player, card) ){
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			event_result |= KEYWORD_TRAMPLE;
		}
	}

	return global_enchantment(player, card, event);
}

int card_pursuit_of_knowledge(int player, int card, event_t event){

	/* Pursuit of Knowledge	|3|W
	 * Enchantment
	 * If you would draw a card, you may put a study counter on ~ instead.
	 * Remove three study counters from ~, Sacrifice ~: Draw seven cards. */

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && !suppress_draw &&
		! is_humiliated(player, card)
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= (!IS_AI(player) ? RESOLVE_TRIGGER_OPTIONAL
							 : count_counters(player, card, COUNTER_STUDY) < 3 ? RESOLVE_TRIGGER_MANDATORY
							 : 0);
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				add_counter(player, card, COUNTER_STUDY);
				suppress_draw = 1;
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 7);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, GVC_COUNTERS(COUNTER_STUDY, 3), NULL, NULL);
}

int card_rabid_rats(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_BLOCKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_ransack(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = MIN(5, count_deck(instance->targets[0].player));
			if( amount > 0 ){
				scrylike_effect(instance->targets[0].player, instance->targets[0].player, amount);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_reins_of_power(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_manipulate_all(player, card, 2, &this_test, ACT_UNTAP);

			add_state(player, -1, STATE_CANNOT_TARGET);

			new_manipulate_all(player, card, 1-player, &this_test, ACT_OF_TREASON);

			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) ){
					if( ! check_state(player, i, STATE_CANNOT_TARGET) ){
						add_state(player, i, STATE_CANNOT_TARGET);
					}
					else{
						remove_state(player, i, STATE_CANNOT_TARGET);
					}
				}
			}

			int fake = add_card_to_hand(1-player, instance->internal_card_id);
			new_manipulate_all(1-player, fake, player, &this_test, ACT_OF_TREASON);
			obliterate_card(1-player, fake);

			remove_state(player, -1, STATE_CANNOT_TARGET);

		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_revenant(int player, int card, event_t event){
	/* Boneyard Wurm	|1|G
	 * Creature - Wurm 100/100
	 * ~'s power and toughness are each equal to the number of creature cards in your graveyard. */
	/* Revenant	|4|B
	 * Creature - Spirit 100/100
	 * Flying
	 * ~'s power and toughness are each equal to the number of creature cards in your graveyard. */

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1 ){
		event_result = count_graveyard_by_type(player, TYPE_CREATURE);
	}

	return 0;
}

int card_rolling_stones(int player, int card, event_t event)
{
  // 0x404020
  if (event == EVENT_ABILITIES
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && has_subtype(affected_card_controller, affected_card, SUBTYPE_WALL)
	  && in_play(player, card) && in_play(affected_card_controller, affected_card)
	  && !is_humiliated(player, card))
	add_status(affected_card_controller, affected_card, STATUS_WALL_CAN_ATTACK);

  return global_enchantment(player, card, event);
}

int card_ruination(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.subtype = SUBTYPE_BASIC;
		this_test.subtype_flag = DOESNT_MATCH;
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_scapegoat(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_SAC_CREATURE_AS_COST, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (!sac){
			cancel = 1;
			return 0;
		}
		int trgs = instance->number_of_targets = 0;
		while( trgs < 10 && can_target(&td) ){
				if( new_pick_target(&td, "Select target creature you control.", trgs, GS_LITERAL_PROMPT) ){
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
			int choice = 1;
			if( player == HUMAN ){
				choice = do_dialog(player, player, card, -1, -1, " Continue\n Abort casting", 0);
			}
			if( choice == 0 ){
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
				spell_fizzled = 1;
				return 0;
			}
		}
		kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
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

int card_seething_anger(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 0, NULL);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = buyback(player, card, MANACOST_X(3));
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 0);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_shaman_en_kor(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_NOT_ME_AS_TARGET, MANACOST0, 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, MANACOST_XW(1, 1)) ){
			choice = do_dialog(player, player, card, -1, -1, " Pass damage\n Take damage\n Do nothing", 1);
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 1*choice, 0, 0, 0, 0, 1*choice) ){
				state_untargettable(player, card, 1);
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					instance->info_slot = 66+choice;
				}
				state_untargettable(player, card, 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int legacy = create_targetted_legacy_effect(player, card, &kor_effect, player, instance->parent_card);
		card_instance_t *leg = get_card_instance(player, legacy);
		if( instance->info_slot == 66 ){
			leg->targets[0].player = player;
			leg->targets[0].card = instance->parent_card;
			leg->targets[1].player = instance->targets[0].player;
			leg->targets[1].card = instance->targets[0].card;
			leg->number_of_targets = 2;
		}
		else if( instance->info_slot == 67 ){
				leg->targets[1].player = player;
				leg->targets[1].card = instance->parent_card;
				leg->targets[0].player = instance->targets[0].player;
				leg->targets[0].card = instance->targets[0].card;
				leg->number_of_targets = 2;
		}
	}

	return 0;
}

int card_shard_phoenix(int player, int card, event_t event){

	// |R|R|R: Return ~ from your graveyard to your hand. Activate this ability only during your upkeep.
	if( event == EVENT_GRAVEYARD_ABILITY  && has_mana(player, COLOR_RED, 3) && current_phase == PHASE_UPKEEP && current_turn == player ){
		return GA_RETURN_TO_HAND;
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana(player, COLOR_RED, 3);
		if( spell_fizzled != 1){
			return GAPAID_REMOVE;
		}
	}

	// Sacrifice ~: ~ deals 2 damage to each creature without flying.
	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "");
		test.keyword = KEYWORD_FLYING;
		test.keyword_flag = DOESNT_MATCH;

		card_instance_t* instance = get_card_instance(player, card);
		new_damage_all(instance->parent_controller, instance->parent_card, ANYBODY, 2, 0, &test);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}

int card_shock2(int player, int card, event_t event){
	/*
	  Shock |R
	  Instant
	  Shock deals 2 damage to target creature or player.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_sift(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 3);
		discard(player, 0, player);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_skeleton_scavengers(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 1);

	if( event == EVENT_RESOLVE_ACTIVATION && can_regenerate(instance->parent_controller, instance->parent_card) ){
		regenerate_target(instance->parent_controller, instance->parent_card);
		add_1_1_counter(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST_X(count_1_1_counters(player, card)), 0, NULL, NULL);
}

int card_skyshroud_archer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_LITERAL_PROMPT, MANACOST0, 0, &td, "Select target creature with flying");
}

int card_sliver_queen(int player, int card, event_t event ){
	// original code :  004DE208

	if( IS_GAA_EVENT(event) ){
		if( event == EVENT_RESOLVE_ACTIVATION ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SLIVER, &token);
			token.pow = token.tou = 1;
			generate_token(&token);
		}
		return generic_activated_ability(player, card, event, 0,MANACOST_X(2), 0, NULL, NULL);
	}

	return slivercycling(player, card, event);
}

int card_soltari_champion(int player, int card, event_t event)
{
	shadow(player, card, event);

	// Whenever ~ attacks, all other creatures you control get +1/+1 until end of turn.
	if (declare_attackers_trigger(player, card, event, 0, player, card))
		{
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "");
			test.not_me = 1;
			pump_creatures_until_eot(player, card, player, 0, 1,1, 0,0, &test);
		}

	return 0;
}

// spike token --> rhino token.

int card_spike_breeder(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE  ){
		return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST_X(2), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_spike(player, card, EVENT_CAN_ACTIVATE, 6) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_1_1_COUNTER, MANACOST_X(2), 0, NULL, NULL) ){
				int ai_choice = 1;
				if( current_phase == PHASE_AFTER_BLOCKING ){
					ai_choice = 0;
				}
				choice = do_dialog(player, player, card, -1, -1, " Move a counter\n Gain 2 life\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			generic_spike(player, card, event, 2);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1 ){
			generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST_X(2), 0, NULL, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == 67 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIKE, &token);
			token.pow = 1;
			token.tou = 1;
			generate_token(&token);
		}
	}

	return 0;
}

int card_spike_colony(int player, int card, event_t event){

	return generic_spike(player, card, event, 4);
}

int card_spike_feeder(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE  ){
		return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_spike(player, card, EVENT_CAN_ACTIVATE, 6) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_1_1_COUNTER, MANACOST0, 0, NULL, NULL) ){
				int ai_choice = 0;
				if( life[player] < 6 ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Move a counter\n Gain 2 life\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			generic_spike(player, card, event, 2);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1 ){
			generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST0, 0, NULL, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == 67 ){
			gain_life(player, 2);
		}
	}

	return 0;
}

int card_spike_soldier(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 3);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE  ){
		return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_spike(player, card, EVENT_CAN_ACTIVATE, 6) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_1_1_COUNTER, MANACOST0, 0, NULL, NULL) ){
				int ai_choice = 0;
				if( current_phase == PHASE_AFTER_BLOCKING ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Move a counter\n Pump Spike Soldier\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			generic_spike(player, card, event, 2);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1 ){
			generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST0, 0, NULL, NULL);;
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == 67 ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
		}
	}

	return 0;
}

int card_spike_worker(int player, int card, event_t event){

	return generic_spike(player, card, event, 2);
}

int card_spindrift_drake(int player, int card, event_t event){

	basic_upkeep(player, card, event, 0, 0, 1, 0, 0, 0);

	return 0;
}

int card_spined_sliver(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS ){
		int i;
		for( i=0; i<active_cards_count[current_turn]; i++){
			if( in_play(current_turn, i) && is_what(current_turn, i, TYPE_CREATURE) && is_attacking(current_turn, i) &&
				has_subtype(current_turn, i, SUBTYPE_SLIVER)
				){
				int amount = count_my_blockers(current_turn, i);
				if( amount ){
					pump_until_eot(player, card, current_turn, i, amount, amount);
				}
			}
		}
	}

	return slivercycling(player, card, event);
}

// spirit en-kor --> nomads en-kor

int card_spitting_hydra(int player, int card, event_t event){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 4);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(1, 1), GVC_COUNTER(COUNTER_P1_P1), &td, "TARGET_CREATURE");
}

int card_stronghold_assassin(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE | GAA_LITERAL_PROMPT, MANACOST0, 0,
									 &td1, get_sleighted_color_text(player, card, "Select target non%s creature", COLOR_BLACK));
}

int card_sword_of_the_chosen(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_subtype = SUBTYPE_LEGEND;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									 &td, "Select target legendary creature.");
}

int card_thalakos_deceiver(int player, int card, event_t event){

	shadow(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && is_attacking(player, card) && is_unblocked(player, card) &&
		current_phase == PHASE_AFTER_BLOCKING
		){
		return can_target(&td);
	}

	if( event == EVENT_ACTIVATE){
		if( pick_target(&td, "TARGET_CREATURE") ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_tortured_existence(int player, int card, event_t event){

	/* Tortured Existence	|B
	 * Enchantment
	 * |B, Discard a creature card: Return target creature card from your graveyard to your hand. */

	if (IS_ACTIVATING(event)){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		this_test.zone = TARGET_ZONE_HAND;

		if( event == EVENT_CAN_ACTIVATE ){
			return (CAN_ACTIVATE(player, card, MANACOST_B(1)) && check_battlefield_for_special_card(player, card, player, 0, &this_test) &&
					any_in_graveyard_by_type(player, TYPE_CREATURE) && !graveyard_has_shroud(player));
		}

		if( event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 && select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
				discard_card(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int selected = validate_target_from_grave_source(player, card, player, 0);
			if( selected != -1 ){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
		}
   }

   return global_enchantment(player, card, event);
}

int card_venerable_monk(int player, int card, event_t event){

	return cip_lifegain(player, card, event, 2);
}

int victual_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			gain_life(player, 4);
		}

		return shared_sliver_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL);
	}

	return 0;
}

int card_victual_sliver(int player, int card, event_t event){
	/*
	  Victual Sliver |W|G
	  Creature - Sliver 2/2
	  All Slivers have "{2}, Sacrifice this permanent: You gain 4 life."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &victual_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &victual_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int card_volraths_gardens(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, MANACOST_X(2)) && can_sorcery_be_played(player, event) ){
		return can_target(&td);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) &&
			new_pick_target(&td, "Select target untapped creature.", 0, 1 | GS_LITERAL_PROMPT)
		  ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 2);
	}

	return 0;
}

int card_volraths_laboratory(int player, int card, event_t event){
	/* Volrath's Laboratory	|5
	 * Artifact
	 * As ~ enters the battlefield, choose a color and a creature type.
	 * |5, |T: Put a 2/2 creature token of the chosen color and type onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[1].player = 1<<choose_a_color_and_show_legacy(player, card, 1-player, -1);
		instance->targets[1].card = select_a_subtype(player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_RIPTIDE_REPLICATOR_TOKEN, &token);
		token.pow = token.tou = 2;
		token.color_forced = instance->targets[1].player;
		token.no_sleight = 1;
		token.action = TOKEN_ACTION_ADD_SUBTYPE;
		token.action_argument = instance->targets[1].card;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
}

static int vs_legacy_shapeshift(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		const int *grave = get_grave(p);

		if( event == EVENT_CHANGE_TYPE && affect_me(p, c) ){
			if( grave[0] != -1 && (cards_data[grave[count_graveyard(p)-1]].type & TYPE_CREATURE) ){
				event_result = grave[count_graveyard(p)-1];
			}
		}

	}
	return 0;
}

static int vs_legacy_ability(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( get_id(p, c) == CARD_ID_VOLRATHS_SHAPESHIFTER ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}
		else{
			remove_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( effect_follows_control_of_attachment(player, card, event) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		if( event == EVENT_CAN_ACTIVATE ){
			int result = granted_generic_activated_ability(player, card, p, c, event, GAA_DISCARD, MANACOST_X(2), 0, NULL, NULL);
			if( player == AI ){
				return (result && check_battlefield_for_special_card(player, card, p, 0, &test));
			}
			return result;
		}

		if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(p, c, MANACOST_X(2));
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( player == HUMAN ){
				discard(p, 0, player);
			}
			else{
				int selected = new_select_a_card(p, p, TUTOR_FROM_HAND, 1, AI_MAX_CMC, -1, &test);
				if( selected != -1 ){
					discard_card(p, selected);
				}
			}
		}
		return 0;
	}
	return 0;
}

int card_volraths_shapeshifter(int player, int card, event_t event){

	/* Volrath's Shapeshifter	|1|U|U
	 * Creature - Shapeshifter 0/1
	 * As long as the top card of your graveyard is a creature card, ~ has the full text of that card and has the text "|2: Discard a card."
	 * |2: Discard a card. */

	if( event == EVENT_RESOLVE_SPELL ){
		int l1 = create_targetted_legacy_effect(player, card, &vs_legacy_shapeshift, player, card);
		add_status(player, l1, STATUS_INVISIBLE_FX);
		int l2 = create_targetted_legacy_activate(player, card, &vs_legacy_ability, player, card);
		card_instance_t *instance = get_card_instance( player, l2 );
		instance->targets[0].player = player;
		instance->targets[0].card = card;
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t test;
	default_test_definition(&test, TYPE_CREATURE);
	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_X(2), 0, NULL, NULL);
		if( player == AI ){
			return (result && check_battlefield_for_special_card(player, card, player, 0, &test));
		}
		return result;
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, MANACOST_X(2));
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( player == HUMAN ){
			discard(player, 0, player);
		}
		else{
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MAX_CMC, -1, &test);
			if( selected != -1 ){
				discard_card(player, selected);
			}
		}
	}

	return 0;
}

int card_volrath_stronghold(int player, int card, event_t event){

  check_legend_rule(player, card, event);

  return recycling_land(player, card, event, TYPE_CREATURE, 1, 1, 0, 0, 0, 0);
}

int card_wall_of_essence(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( damage_dealt_to_me_arbitrary(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE, player, card) ){
			card_instance_t *instance = get_card_instance(player, card);
			gain_life(player, instance->targets[7].card);
			instance->targets[7].card = 0;
		}
	}

	return 0;
}

int card_wall_of_souls(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( damage_dealt_to_me_arbitrary(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE, player, card) ){
			target_definition_t td;
			default_target_definition(player, card, &td, 0);
			td.zone = TARGET_ZONE_PLAYERS;

			card_instance_t *instance = get_card_instance(player, card);

			if( would_validate_arbitrary_target(&td, 1-player, -1) ){
				damage_player(1-player, instance->targets[7].card, player, card);
			}
			instance->targets[7].card = 0;
		}
	}

	return 0;
}

static int wot_effect(int player, int card, event_t event){
	if (end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)){
		card_instance_t* instance = get_card_instance(player, card);
		if( ! check_state(instance->damage_target_player, instance->damage_target_card, STATE_OUBLIETTED) ){
			bounce_permanent(instance->damage_target_player, instance->damage_target_card);
			return 0;
		}
		kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}
static void bounce_at_end_of_combat(int player, int card, int t_player, int t_card)
{
  create_targetted_legacy_effect(player, card, wot_effect, t_player, t_card);
}
int card_wall_of_tears(int player, int card, event_t event)
{
  /* Wall of Tears	|1|U
   * Creature - Wall 0/4
   * Defender
   * Whenever ~ blocks a creature, return that creature to its owner's hand at end of combat. */

  if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card) && current_turn == 1-player)
	get_card_instance(player, card)->destroys_if_blocked |= DIFB_DESTROYS_ALL;

  if (event == EVENT_DECLARE_BLOCKERS && current_turn == 1-player && !is_humiliated(player, card))
	for_each_creature_blocked_by_me(player, card, bounce_at_end_of_combat, player, card);

  return 0;
}

// warrior angel --> child of night

// warrior en-kor --> nomads en-kor

int card_calming_licid(int player, int card, event_t event){
	if (!is_what(player, card, TYPE_CREATURE)){
		effect_cannot_attack(player, card, event);
	}

	return generic_licid(player, card, event, MANACOST_W(1), 0, 0, 0, 0, 1-player);
}

int card_convulsing_licid(int player, int card, event_t event){
	return generic_licid(player, card, event, MANACOST_R(1), 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK, 1-player);
}

int card_corrupting_licid(int player, int card, event_t event){
	return generic_licid(player, card, event, MANACOST_B(1), 0, 0, 0, SP_KEYWORD_FEAR, player);
}

int card_gliding_licid(int player, int card, event_t event){
	return generic_licid(player, card, event, MANACOST_U(1), 0, 0, KEYWORD_FLYING, 0, player);
}

int card_tempting_licid(int player, int card, event_t event){
	return generic_licid(player, card, event, MANACOST_G(1), 0, 0, 0, SP_KEYWORD_LURE, player);
}
