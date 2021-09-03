#include "manalink.h"

/***** Functions *****/
int player_has_generic_and_colorless_available(int player, int generic_needed, int colorless_needed){
    int total = mana_pool[player][COLOR_ANY];
    int colorless = mana_pool[player][COLOR_COLORLESS];

    total += raw_mana_available[player][COLOR_ANY];
    colorless += raw_mana_available[player][COLOR_COLORLESS];

	int i;
    for (i = 0; i < 50 && raw_mana_available_hex[player][i] != -1; ++i){
			total += HIWORD(raw_mana_available_hex[player][i]);
			if (LOWORD(raw_mana_available_hex[player][i]) & COLOR_TEST_COLORLESS)
				colorless += HIWORD(raw_mana_available_hex[player][i]);
    }
    return colorless >= colorless_needed && (total - colorless) >= generic_needed;
}

static int has_paid_non_colorless_mana(int player){
	int i;
	int result = 0;
	for(i=1; i<7; i++){
		if( mana_paid[i] ){
			result++;
		}
	}
	return result;
}

static int has_colorless_only_mana(int player, int amount){
    int colorless = mana_pool[player][COLOR_COLORLESS];

    colorless += raw_mana_available[player][COLOR_COLORLESS];

	int i;
    for (i = 0; i < 50 && raw_mana_available_hex[player][i] != -1; ++i){
			if (LOWORD(raw_mana_available_hex[player][i]) & COLOR_TEST_COLORLESS)
				colorless += HIWORD(raw_mana_available_hex[player][i]);
    }
    return colorless >= amount ? 1 : 0;
}

static void modify_casting_cost_for_colorless_only_mana(int player, int card, event_t event, int number_of_colorless_only_mana){
	if( event == EVENT_MODIFY_COST ){
		if( ! has_colorless_only_mana(player, number_of_colorless_only_mana) ){
			infinite_casting_cost();
		}
		else{
			COST_COLORLESS-=number_of_colorless_only_mana;
		}
	}
}

static int charge_colorless_mana_only(int player, int amount){
	charge_mana(player, COLOR_COLORLESS, amount);
	if( has_paid_non_colorless_mana(player) ){
		return 0;
	}
	return 1;
}

int charge_generic_mana_with_colorless_only(int player, int total_amount, int amount_of_colorless_only){
	charge_mana(player, COLOR_COLORLESS, total_amount);
	if( has_paid_non_colorless_mana(player) >= amount_of_colorless_only ){
		return 0;
	}
	return 1;
}

static void has_colorless_only_mana_in_casting_cost(int player, int card, event_t event, int number_of_colorless_only_mana){

	modify_casting_cost_for_colorless_only_mana(player, card, event, number_of_colorless_only_mana);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			if( ! charge_colorless_mana_only(player, number_of_colorless_only_mana) ){
				spell_fizzled = 1;
			}
		}
	}
}

static void support(int player, int card, int max_targets){
	//	Support X. (Put a +1/+1 counter on each of up to X other target creatures.)

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_NOT_ME;

	card_instance_t *instance = get_card_instance(player, card);

	if( can_target(&td) ){
		int i;
		for(i=0; i<max_targets; i++){
			char prompt[100];
			scnprintf(prompt, 100, "Select target for Support (%d of %d)", i+1, max_targets);
			if( new_pick_target(&td, prompt, i, GS_LITERAL_PROMPT) ){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 1);
			}
		}
		for(i=0; i<instance->number_of_targets; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			add_1_1_counter(instance->targets[i].player, instance->targets[i].card);
		}
		instance->number_of_targets = 0;
	}
}

static int can_use_cohort(int player, int card){
	// Cohort - {T}, Tap an untapped Ally you control: [do something]
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.allowed_controller = td.preferred_controller = player;
	td.special = TARGET_SPECIAL_NOT_ME;
	td.required_subtype = SUBTYPE_ALLY;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;

	if( ! is_tapped(player, card) && can_target(&td) ){
		return 1;
	}

	return 0;
}

static int pick_target_for_cohort(int player, int card){
	// Cohort - {T}, Tap an untapped Ally you control: [do something]
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.allowed_controller = td.preferred_controller = player;
	td.special = TARGET_SPECIAL_NOT_ME;
	td.required_subtype = SUBTYPE_ALLY;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;

	if( new_pick_target(&td, "Select an untapped Ally you control.", 0, GS_LITERAL_PROMPT) ){
		get_card_instance(player, card)->number_of_targets = 0;
		return get_card_instance(player, card)->targets[0].card;
	}

	return -1;
}

static void modify_casting_cost_for_surge(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white){
	if( event == EVENT_MODIFY_COST ){
		if( get_specific_storm_count(player) > 0 && has_mana_multi(player, cless, black, blue, green, red, white) ){
			null_casting_cost(player, card);
			get_card_instance(player, card)->info_slot = 1;
		}
		else{
			get_card_instance(player, card)->info_slot = 0;
		}
	}
}

static int surge(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white){

	enum
	{
		CHOICE_SURGE = 1,
		CHOICE_CAST_NORMALLY,
	};

	if( ! played_for_free(player, card) && ! is_token(player, card) && get_card_instance(player, card)->info_slot == 1 ){
		if( has_mana_to_cast_iid(player, event, get_card_instance(player, card)->internal_card_id) ){
			int choice = DIALOG(player, card, event, DLG_NO_STORAGE,
								"Pay Surge cost", 1, 10,
								"Play normally", 1, 5);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;;
			}

			if( choice == CHOICE_SURGE ){
				charge_mana_multi(player, cless, black, blue, green, red, white);
				if( spell_fizzled != 1 ){
					return 2;
				}
			}

			if( choice == CHOICE_CAST_NORMALLY ){
				charge_mana_from_id(player, card, event, get_id(player, card));
				if( spell_fizzled != 1 ){
					return 1;
				}
			}
		}
	}
	return 1;
}

static int is_land_creature(int player, int card)
{
	if( is_what(player, card, TYPE_LAND) && is_what(player, card, TYPE_CREATURE) ){
		return 1;
	}
	return 0;
}

static const char* is_land_creature_target(int who_chooses, int player, int card)
{
	if( is_land_creature(player, card) ){
		return NULL;
	}
	return "must be a land creature";
}

/***** Cards *****/

/*** Colorless ***/

int card_deceiver_of_form(int player, int card, event_t event){
	/* Deceiver of Form	|6|C	0x200e427
	 * Creature - Eldrazi 8/8
	 * At the beginning of combat on your turn, reveal the top card of your library.
	 If a creature card is revealed this way, you may have creatures you control other than ~ become copies of that card until end of turn.
	 You may put that card on the bottom of your library. */

	has_colorless_only_mana_in_casting_cost(player, card, event, 1);

	if( beginning_of_combat(player, card, event, player, card) ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			reveal_card_iid(player, card, deck[0]);
			if( is_what(-1, deck[0], TYPE_CREATURE) ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				int syc_priority = check_battlefield_for_special_card(player, card, player, CBFSC_GET_MAX_CMC, &this_test) >=
									get_cmc_by_internal_id(deck[0]) ? 10 : 0;
				int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_NO_CANCEL,
									"Shapeshift your creatures", 1, syc_priority,
									"Decline", 1, 1);
				if( choice == 1 ){
					shapeshift_all(player, card, player, &this_test, -1, deck[0], 1);
				}
				choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_RANDOM,
								"Put top card in grave", 1, 1,
								"Decline", 1, 1);
				if( choice == 1 ){
					mill(player, 1);
				}
			}
		}
	}

	return 0;
}

int card_eldrazi_mimic(int player, int card, event_t event){
	/* Eldrazi Mimic	|2	0x200e422
	 * Creature - Eldrazi 2/1
	 * Whenever another colorless creature enters the battlefield under your control, you may change ~'s base power and toughness
	 to that creature's power and toughness until end of turn. */
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.color = COLOR_TEST_COLORLESS;
		test.not_me = 1;

		int ai_mode = (player == HUMAN ? RESOLVE_TRIGGER_OPTIONAL : 0);
		if( player == AI && get_power(trigger_cause_controller, trigger_cause) > get_power(player, card) ){
			ai_mode = RESOLVE_TRIGGER_MANDATORY;
		}

		if( new_specific_cip(player, card, event, player, ai_mode, &test) ){
			set_pt_and_abilities_until_eot(player, card, player, card, get_power(trigger_cause_controller, trigger_cause),
											get_toughness(trigger_cause_controller, trigger_cause), 0, 0, 0);
		}
	}

	return 0;
}

int card_endbringer(int player, int card, event_t event){
	/* Endbringer	|5|C	0x200e42c
	 * Creature - Eldrazi 5/5
	 * Untap ~ during each other player's untap step.
	 * |T: ~ deals 1 damage to target creature or player.
	 * |C, |T: Target creature can't attack or block this turn.
	 * |C|C, |T: Draw a card. */

	has_colorless_only_mana_in_casting_cost(player, card, event, 1);

	if( ! is_humiliated(player, card) && current_phase == PHASE_UNTAP && event == EVENT_UNTAP && current_turn == 1-player &&
		is_tapped(player, card) )
	{
		untap_card(player, card);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);

	enum {
			CHOICE_DAMAGE = 1,
			CHOICE_TARGET_CREATURE,
			CHOICE_DRAW
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td, NULL) ){
			return 1;
		}
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_X(1), 0, &td2, NULL) )
		{
			return 1;
		}
		if( has_colorless_only_mana(player, 2) &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL) )
		{
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = instance->number_of_targets = 0;
		int abilities[3] = {0, 0, 0};
		int priorities[3] = {0, 0, 0};
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td, NULL) ){
			abilities[0] = 1;
			priorities[0] = 10;
		}
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_X(1), 0, &td2, NULL) )
		{
			abilities[1] = 1;
			if( (current_turn == player && current_phase < PHASE_DECLARE_BLOCKERS) ||
				(current_turn != player && current_phase < PHASE_DECLARE_ATTACKERS) )
			{
				priorities[1] = 15;
			}
		}
		if( has_colorless_only_mana(player, 2) &&
			generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL) )
		{
			abilities[2] = 1;
			priorities[2] = 5;
		}
		int choice = DIALOG(player, card, event, DLG_RANDOM,
							"Damage creature or player", abilities[0], priorities[0],
							"Target cannot attack / cannot block", abilities[1], priorities[1],
							"Draw a card", abilities[2], priorities[2]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_DAMAGE ){
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
		}
		if( choice == CHOICE_TARGET_CREATURE ){
			if( ! charge_colorless_mana_only(player, 1) ){
				spell_fizzled = 1;
				return 0;
			}
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td2, "TARGET_CREATURE");
		}
		if( choice == CHOICE_DRAW ){
			if( ! charge_colorless_mana_only(player, 2) ){
				spell_fizzled = 1;
				return 0;
			}
			return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == CHOICE_DAMAGE && valid_target(&td) ){
			damage_target0(player, card, 1);
		}
		if( instance->info_slot == CHOICE_TARGET_CREATURE && valid_target(&td2) ){
			int priorities[2] = {
									current_turn != player && current_phase < PHASE_DECLARE_ATTACKERS ? 10 : 0,
									current_turn == player && current_phase < PHASE_DECLARE_BLOCKERS ? 10 : 0
								};
			int ab_choice = DIALOG(player, card, event, DLG_NO_CANCEL,
								"Cannot attack", 1, priorities[0],
								"Cannot block", 1, priorities[1]);
			if( ab_choice == 1 ){
				create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &effect_cannot_attack_until_eot,
										instance->targets[0].player, instance->targets[0].card);
			}
			if( ab_choice == 2 ){
				pump_ability_until_eot(instance->parent_controller, instance->parent_card,
										instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
			}
		}
		if( instance->info_slot == CHOICE_DRAW ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_kozilek_the_great_distortion(int player, int card, event_t event){
	/* Kozilek, the Great Distortion	|8|C|C 0x200e431
	 * Legendary Creature - Eldrazi 12/12
	 * When you cast ~, if you have fewer than seven cards in hand, draw cards equal to the difference.
	 * Menace
	 * Discard a card with converted mana cost X: Counter target spell with converted mana cost X. */

	has_colorless_only_mana_in_casting_cost(player, card, event, 2);

	check_legend_rule(player, card, event);

	menace(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			int amount = hand_count[1-player] - hand_count[player];
			if( amount ){
				draw_cards(player, amount);
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK, MANACOST0, 0, NULL, NULL);
		if( result ){
			if( player == HUMAN ){
				return result;
			}
			if( player == AI ){
				test_definition_t test;
				default_test_definition(&test, TYPE_ANY);
				test.cmc = get_cmc(card_on_stack_controller, card_on_stack);
				test.zone = TARGET_ZONE_HAND;
				if( check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test) ){
					return result;
				}
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t test;
			default_test_definition(&test, TYPE_ANY);
			if( player == AI ){
				test.cmc = get_cmc(card_on_stack_controller, card_on_stack);
			}
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_FIRST_FOUND, -1, &test);
			if( selected != -1 ){
				instance->info_slot = get_cmc(player, selected);
				discard_card(player, selected);

				instance->targets[0].player = card_on_stack_controller;
				instance->targets[0].card = card_on_stack;
				instance->number_of_targets = 1;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( counterspell_validate(player, card, NULL, 0) &&
			get_cmc(instance->targets[0].player, instance->targets[0].card) == instance->info_slot )
		{
			real_counter_a_spell(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

static int kozileks_pathfinder_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player < 0 ){
		return 0;
	}

	if( event == EVENT_BLOCK_LEGALITY && affect_me(instance->damage_target_player, instance->damage_target_card) ){
		if( attacking_card_controller == instance->targets[0].player && attacking_card == instance->targets[0].card ){
			event_result = 1;
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_kozileks_pathfinder(int player, int card, event_t event){
	/* Kozilek's Pathfinder	|6	0x200e436
	 * Creature - Eldrazi 5/5
	 * |C: Target creature can't block ~ this turn. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(1), 0, &td2, NULL) )
		{
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td2, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			int legacy = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &kozileks_pathfinder_legacy,
									instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(instance->parent_controller, legacy);
			leg->targets[0].player = instance->parent_controller;
			leg->targets[0].card = instance->parent_card;
			leg->number_of_targets = 1;
		}
	}

	return 0;
}

int card_matter_reshaper(int player, int card, event_t event){
	/* Matter Reshaper	|2|C	0x200e43b
	 * Creature - Eldrazi 3/2
	 * When ~ dies, reveal the top card of your library.
	 You may put that card onto the battlefield if it's a permanent card with converted mana cost 3 or less.
	 Otherwise, put that card into your hand. */

	has_colorless_only_mana_in_casting_cost(player, card, event, 1);

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			reveal_card_iid(player, card, deck[0]);
			int card_added = add_card_to_hand(player, deck[0]);
			remove_card_from_deck(player, 0);
			if( is_what(player, card_added, TYPE_PERMANENT) && get_cmc(player, card_added) <= 3 ){
				int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_NO_CANCEL,
									"Put this card in play", 1, 5,
									"Decline", 1, 1);
				if( choice == 1 ){
					put_into_play(player, card_added);
				}
			}
		}
	}

	return 0;
}

int card_reality_smasher(int player, int card, event_t event){
	/* Reality Smasher	|4|C	0x200e440
	 * Creature - Eldrazi 5/5
	 * Trample, haste
	 * Whenever ~ becomes the target of a spell an opponent controls, counter that spell unless its controller discards a card. */

	has_colorless_only_mana_in_casting_cost(player, card, event, 1);

	haste(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( trigger_cause_controller == 1-player ){
			int good = 0;
			card_instance_t *instance = get_card_instance(trigger_cause_controller, trigger_cause);
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				if( instance->targets[0].player == player && instance->targets[0].card == card ){
					good = 1;
				}
			}
			if( good ){
				if( new_specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, NULL) ){
					int choice = 2;
					test_definition_t test;
					default_test_definition(&test, TYPE_CREATURE);
					if( new_can_sacrifice(player, card, 1-player, &test) ){
						choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_WHO_CHOOSES(1-player),
										"Sac a creature", 1, (count_subtype(1-player, TYPE_CREATURE, -1) > 3 ? 10 : 0),
										"Decline", 1, 1);
					}
					if( choice == 1 ){
						new_sacrifice(player, card, 1-player, SAC_NO_CANCEL, &test);
					}
					if( choice == 2 ){
						real_counter_a_spell(player, card, trigger_cause_controller, trigger_cause);
					}
				}
			}
		}
	}

	return 0;
}

int card_spatial_contortion(int player, int card, event_t event){
	/* Spatial Contortion	|1|C	0x200e445
	 * Instant
	 * Target creature gets +3/-3 until end of turn. */

	has_colorless_only_mana_in_casting_cost(player, card, event, 1);

	return vanilla_instant_pump(player, card, event, ANYBODY, 1-player, 3, -3, 0, 0);
}

int card_thought_knot_seer(int player, int card, event_t event){
	/* Thought-Knot Seer	|3|C	0x200e44a
	 * Creature - Eldrazi 4/4
	 * When ~ enters the battlefield, target opponent reveals his or her hand. You choose a nonland card from it and exile that card.
	 * When ~ leaves the battlefield, target opponent draws a card. */

	has_colorless_only_mana_in_casting_cost(player, card, event, 1);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		if( would_validate_arbitrary_target(&td, 1-player, -1) && hand_count[1-player] > 0 ){
			test_definition_t test;
			default_test_definition(&test, TYPE_LAND);
			test.type_flag = DOESNT_MATCH;
			new_global_tutor(player, 1-player, TUTOR_FROM_HAND, TUTOR_RFG, 1, AI_MAX_CMC, &test);
		}
	}

	if( leaves_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		if( would_validate_arbitrary_target(&td, 1-player, -1) ){
			draw_cards(1-player, 1);
		}
	}

	return 0;
}

int card_walker_of_the_wastes(int player, int card, event_t event){
	/* Walker of the Wastes	|4|C	0x200e44f
	 * Creature - Eldrazi 4/4
	 * Trample
	 * ~ gets +1/+1 for each land you control named Wastes. */

	has_colorless_only_mana_in_casting_cost(player, card, event, 1);

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			test_definition_t test;
			default_test_definition(&test, TYPE_ANY);
			test.id = CARD_ID_WASTES;
			event_result += check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &test);
		}
	}

	return 0;
}

int card_warden_of_geometries(int player, int card, event_t event){
	/* Warden of Geometries	|4	0x200e454
	 * Creature - Eldrazi Drone 2/3
	 * Vigilance
	 * |T: Add |C to your mana pool. */

	vigilance(player, card, event);

	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		return 0;	// Don't make extra-reluctant to attack, due to vigilance
	}

	return mana_producing_creature(player, card, event, 12, COLOR_COLORLESS, 1);
}

static const char* pt_1_or_less(int who_chooses, int player, int card)
{
	if( get_power(player, card) <= 1 || get_toughness(player, card) <= 1 ){
		return NULL;
	}
	return "should have power or toughness 1 or less";
}

int card_warping_wail(int player, int card, event_t event){
	/* Warping Wail	|1|C	0x200e459
	 * Instant
	 * Choose one -
	 * * Exile target creature with power or toughness 1 or less.
	 * * Counter target sorcery spell.
	 * * Put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |C to your mana pool." */

	has_colorless_only_mana_in_casting_cost(player, card, event, 1);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int)pt_1_or_less;

	target_definition_t td2;
	counterspell_target_definition(player, card, &td2, TYPE_SORCERY);

	enum {
			CHOICE_EXILE = 1,
			CHOICE_COUNTER_SORCERY,
			CHOICE_ELDRAZI_SCION
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_COUNTERSPELL, &td2, NULL, 1, NULL) ){
			return 99;
		}
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = instance->number_of_targets = 0;
		int abilities[3] = {0, 0, 1};
		int priorities[3] = {0, 0, 5};
		if( generic_spell(player, card, EVENT_CAN_CAST, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
			abilities[0] = 1;
			priorities[0] = 15;
		}
		if( generic_spell(player, card, EVENT_CAN_CAST, GS_COUNTERSPELL, &td2, NULL, 1, NULL) ){
			abilities[1] = 1;
			priorities[1] = 25;
		}
		int choice = DIALOG(player, card, event, DLG_RANDOM,
							"Kill a crit with pow/tou 1", abilities[0], priorities[0],
							"Counter a sorcery", abilities[1], priorities[1],
							"Generate an Eldrazi Scion", abilities[2], priorities[2]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_EXILE ){
			return generic_spell(player, card, event, GS_CAN_TARGET, &td, "Select target creature with power or toughness 1 or less.", 1, NULL);
		}
		if( choice == CHOICE_COUNTER_SORCERY ){
			return generic_spell(player, card, event, GS_COUNTERSPELL, &td2, NULL, 1, NULL);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_EXILE && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		if( instance->info_slot == CHOICE_COUNTER_SORCERY ){
			return counterspell(player, card, event, &td2, 0);
		}
		if( instance->info_slot == CHOICE_ELDRAZI_SCION ){
			generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


/*** White - Devoid ***/

static void tap_before_putting_card_in_play(int player, int card){
	add_state(player, card, STATE_TAPPED);
}

int card_eldrazi_displacer(int player, int card, event_t event){
	/* Eldrazi Displacer	|2|W	0x200e45e
	 * Creature - Eldrazi 3/3
	 * Devoid
	 * |2|C: Exile another target creature, then return it to the battlefield tapped under its owner's control. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.special = TARGET_SPECIAL_NOT_ME;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_X(3), 0, &td2, NULL) )
		{
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_X(2), 0, &td2, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			blink_effect(instance->targets[0].player, instance->targets[0].card, &tap_before_putting_card_in_play);
		}
	}

	return 0;
}


/*** White ***/

/* Affa Protector	|2|W	--> Serra Angel	0x200c122
 * Creature - Human Soldier Ally 1/4
 * Vigilance */

int card_allied_reinforcements(int player, int card, event_t event){
	/* Allied Reinforcements	|3|W	0x200e463
	 * Sorcery
	 * Put two 2/2 |Swhite Knight Ally creature tokens onto the battlefield. */

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_KNIGHT_ALLY, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_call_the_gatewatch(int player, int card, event_t event){
	/* Call the Gatewatch	|2|W	0x200e468
	 * Sorcery
	 * Search your library for a planeswalker card, reveal it, and put it into your hand. Then shuffle your library. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a Planeswalker card.");
		this_test.subtype = SUBTYPE_PLANESWALKER;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

static int dazzling_reflection_legacy(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t* instance = get_card_instance(player, card);
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 ){
			if( source->damage_source_card == instance->damage_target_card && source->damage_source_player == instance->damage_target_player ){
				source->info_slot = 0;
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}


int card_dazzling_reflection(int player, int card, event_t event){
	/* Dazzling Reflection	|1|W	0x200e46d
	 * Instant
	 * You gain life equal to target creature's power. The next time that creature would deal damage this turn, prevent that damage. */
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life(player, get_power(instance->targets[0].player, instance->targets[0].card));
			create_targetted_legacy_effect(player, card, &dazzling_reflection_legacy, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_expedition_raptor(int player, int card, event_t event){
	/* Expedition Raptor	|3|W|W	0x200e472
	 * Creature - Bird 2/2
	 * Flying
	 * When ~ enters the battlefield, support 2. */

	if( comes_into_play(player, card, event) ){
		support(player, card, 2);
	}

	return 0;
}

int card_general_tazri(int player, int card, event_t event){
	/* General Tazri	|4|W	0x200e477
	 * Legendary Creature - Human Ally 3/4
	 * When ~ enters the battlefield, you may search your library for an Ally creature card, reveal it,
	 * put it into your hand, then shuffle your library.
	 * |W|U|B|R|G: Ally creatures you control get +X/+X until end of turn, where X is the number of colors among those creatures. */

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select an Ally card.");
		this_test.subtype = SUBTYPE_ALLY;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		int i;
		int color_mask = 0;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) ){
				color_mask |= get_color(player, i);
				color_mask &= ~COLOR_TEST_COLORLESS;
				if( color_mask == COLOR_TEST_ANY_COLORED ){
					break;
				}
			}
		}
		int pump = 0;
		for(i=1; i<6; i++){
			if( color_mask & (1<<i) ){
				pump++;
			}
		}
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_ALLY;
		pump_creatures_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, 0, pump, pump, 0, 0, &this_test);
	}

	return generic_activated_ability(player, card, event, 0, 0, 1, 1, 1, 1, 1, 0, NULL, NULL);
}

/* Immolating Glare	|1|W --> Rebuke	0x20049f4
 * Instant
 * Destroy target attacking creature. */

int card_ionas_blessing(int player, int card, event_t event){
	/* Iona's Blessing	|3|W	0x200e47c
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +2/+2, has vigilance, and can block an additional creature. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			event_flags |= EA_SELECT_BLOCK;
			creature_can_block_additional(instance->damage_target_player , instance->damage_target_card, event, 1);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, SP_KEYWORD_VIGILANCE, 0, 0, 0);
}

int card_isolation_zone(int player, int card, event_t event){
	/* Isolation Zone	|2|W|W	0x200e481
	 * Enchantment
	 * When ~ enters the battlefield, exile target creature or enchantment an opponent controls until ~ leaves the battlefield. */

	return_from_oblivion(player, card, event);

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ENCHANTMENT);
		td.allowed_controller = 1-player;

		if( can_target(&td) && new_pick_target(&td, "Select target target creature or enchantment an opponent controls.", 0, GS_LITERAL_PROMPT) ){
			card_instance_t *instance = get_card_instance( player, card );
			obliviation(player, card, instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
	}

	return global_enchantment(player, card, event);
}

int card_kor_scythemaster(int player, int card, event_t event){
	/* Kor Scythemaster	|2|W	0x200e486
	 * Creature - Kor Soldier Ally 3/1
	 * ~ has first strike as long as it's attacking. */

 	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_ABILITIES && affect_me(player, card) && is_attacking(player, card) ){
			event_result |= KEYWORD_FIRST_STRIKE;
		}
	}

	return 0;
}

int card_kor_sky_climber(int player, int card, event_t event){
	/* Kor Sky Climber	|2|W	0x200e48b
	 * Creature - Kor Soldier Ally 3/2
	 * |1|W: ~ gains flying until end of turn. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								0, 0, KEYWORD_FLYING, 0);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XW(1, 1), 0, NULL, NULL);
}

int card_linvala_the_preserver(int player, int card, event_t event){
	/* Linvala, the Preserver	|4|W|W	0x200e490
	 * Legendary Creature - Angel 5/5
	 * Flying
	 * When ~ enters the battlefield, if an opponent has more life than you, you gain 5 life.
	 * When Linvala enters the battlefield, if an opponent controls more creatures than you, put a 3/3 |Swhite Angel
	 * creature token with flying onto the battlefield. */

	check_legend_rule(player, card, event);

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( life[player] < life[1-player] ){
			gain_life(player, 5);
		}
		if( count_subtype(player, TYPE_CREATURE, -1) < count_subtype(1-player, TYPE_CREATURE, -1) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ANGEL, &token);
			token.pow = token.tou = 3;
			token.key_plus = KEYWORD_FLYING;
			token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			generate_token(&token);
		}
	}

	return 0;
}

int card_make_a_stand(int player, int card, event_t event){
	/* Make a Stand	|2|W	0x200e495
	 * Instant
	 * Creatures you control get +1/+0 and gain indestructible until end of turn. */

	if( event == EVENT_RESOLVE_SPELL ){
		pump_creatures_until_eot(player, card, player, 0, 1, 0, 0, SP_KEYWORD_INDESTRUCTIBLE, NULL);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

/* Makindi Aeronaut	|1|W --> vanilla	0x401000
 * Creature - Kor Scout Ally 1/3
 * Flying */

/* Mighty Leap	|1|W	=>m11.c:card_mighty_leap()	reprint
 * Instant
 * Target creature gets +2/+2 and gains flying until end of turn. */

int card_mundas_vanguard(int player, int card, event_t event){
	/* Munda's Vanguard	|4|W	0x200e49a
	 * Creature - Kor Knight Ally 3/3
	 * Cohort - |T, Tap an untapped Ally you control: Put a +1/+1 counter on each creature you control. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_cohort(player, card) ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->number_of_targets = 0;
		int result = pick_target_for_cohort(player, card);
		if( result == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		if( spell_fizzled != 1 ){
			tap_card(player, result);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(instance->parent_controller, instance->parent_card, instance->parent_controller, &this_test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}

	return 0;
}

int card_oath_of_gideon(int player, int card, event_t event){
	/* Oath of Gideon	|2|W	0x200e49f
	 * Legendary Enchantment
	 * When ~ enters the battlefield, put two 1/1 |Swhite Kor Ally creature tokens onto the battlefield.
	 * Each planeswalker you control enters the battlefield with an additional loyalty counter on it. */

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_KOR_ALLY, &token);
		token.pow = token.tou = 1;
		token.qty = 2;
		token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		generate_token(&token);
	}

	if( event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE ){
		if( affected_card_controller == player && is_planeswalker(affected_card_controller, affected_card) ){
			if( in_play(player, card) && ! is_humiliated(player, card) ){
				enters_the_battlefield_with_counters(affected_card_controller, affected_card, event, COUNTER_LOYALTY, 1);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_ondu_war_cleric(int player, int card, event_t event){
	/* Ondu War Cleric	|1|W	0x200e4a4
	 * Creature - Human Cleric Ally 2/2
	 * Cohort - |T, Tap an untapped Ally you control: You gain 2 life. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_cohort(player, card) ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->number_of_targets = 0;
		int result = pick_target_for_cohort(player, card);
		if( result == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		if( spell_fizzled != 1 ){
			tap_card(player, result);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		gain_life(instance->parent_controller, 2);
	}

	return 0;
}

int card_relief_captain(int player, int card, event_t event){
	/* Relief Captain	|2|W|W	0x200e4a9
	 * Creature - Kor Knight Ally 3/2
	 * When ~ enters the battlefield, support 3. */

	if( comes_into_play(player, card, event) ){
		support(player, card, 3);
	}

	return 0;
}

int card_searing_light(int player, int card, event_t event){
	/* Searing Light	|W	0x200e4ae
	 * Instant
	 * Destroy target attacking or blocking creature with power 2 or less. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;
	td.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						"Select target attacking or blocking creature with power 2 or less.", 1, NULL);
}

int card_shoulder_to_shoulder(int player, int card, event_t event){
	/* Shoulder to Shoulder	|2|W	0x200e4b3
	 * Sorcery
	 * Support 2.
	 * Draw a card. */

	if(event == EVENT_RESOLVE_SPELL){
		support(player, card, 2);
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_spawnbinder_mage(int player, int card, event_t event){
	/* Spawnbinder Mage	|3|W	0x200e4b8
	 * Creature - Human Wizard Ally 2/4
	 * Cohort - |T, Tap an untapped Ally you control: Tap target creature. */

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_cohort(player, card) ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->number_of_targets = 0;
		int result = pick_target_for_cohort(player, card);
		if( result == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
		if( spell_fizzled != 1 ){
			tap_card(player, result);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_steppe_glider(int player, int card, event_t event){
	/* Steppe Glider	|4|W	0x200e4bd
	 * Creature - Elemental 2/4
	 * Flying, vigilance
	 * |1|W: Target creature with a +1/+1 counter on it gains flying and vigilance until end of turn. */

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, KEYWORD_FLYING, SP_KEYWORD_VIGILANCE);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XW(1, 1), 0,
									&td, "Select target creature with a +1/+1 counter.");
}

static void count_for_stone_haven_outfitter_ability(int player, int card, event_t event){
	if( event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( is_what(player, card, TYPE_EFFECT) ){
			if( affect_me(instance->damage_target_player, instance->damage_target_card) && instance->kill_code > 0 ){
				instance->damage_target_player = instance->damage_target_card = -1;
				remove_status(player, card, STATUS_INVISIBLE_FX);
			}
		}
		if( ! check_status(player, card, STATUS_INVISIBLE_FX) ){
			count_for_gfp_ability_and_store_values_extra(player, card, event, ANYBODY, TYPE_CREATURE, is_equipped, 0, 0);
		}
	}
}

static int stone_haven_outfitter_effect(int player, int card, event_t event){

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( get_id(instance->damage_target_player, instance->damage_target_card) != CARD_ID_STONE_HAVEN_OUTFITTER ){
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	count_for_stone_haven_outfitter_ability(player, card, event);

	if( ! check_status(player, card, STATUS_INVISIBLE_FX) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t *inst = get_card_instance(player, card);
		draw_cards(player, inst->targets[11].card);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_stone_haven_outfitter(int player, int card, event_t event){
	/* Stone Haven Outfitter	|1|W	0x200e4c2
	 * Creature - Kor Artificer Ally 2/2
	 * Equipped creatures you control get +1/+1.
	 * Whenever an equipped creature you control dies, draw a card. */

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		int found = 0;
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_EFFECT) ){
				card_instance_t *inst = get_card_instance(player, c);
				if( inst->info_slot == (int)stone_haven_outfitter_effect ){
					found = 1;
					break;
				}
			}
		}
		if( ! found ){
			int legacy = create_targetted_legacy_effect(player, card, &stone_haven_outfitter_effect, player, card);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
			if( in_play(affected_card_controller, affected_card) && is_equipped(affected_card_controller, affected_card) ){
				event_result++;
			}
		}
	}

	count_for_stone_haven_outfitter_ability(player, card, event);

	if( get_card_instance(player, card)->kill_code <= 0 && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t *inst = get_card_instance(player, card);
		draw_cards(player, inst->targets[11].card);
		inst->targets[11].card = 0;
	}

	return 0;
}

int card_stoneforger_acolyte(int player, int card, event_t event){
	/* Stoneforge Acolyte	|W	0x200e4c7
	 * Creature - Kor Artificer Ally 1/2
	 * Cohort - |T, Tap an untapped Ally you control: Look at the top four cards of your library.
	 * You may reveal an Equipment card from among them and put it into your hand. Put the rest on the bottom of your library in any order. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_cohort(player, card) ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->number_of_targets = 0;
		int result = pick_target_for_cohort(player, card);
		if( result == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		if( spell_fizzled != 1 ){
			tap_card(player, result);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);

		int amount = MIN(4, count_deck(instance->parent_controller));

		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select an Equipment card.");
		test.subtype = SUBTYPE_EQUIPMENT;
		test.create_minideck = amount;
		test.no_shuffle = 1;

		if( new_global_tutor(instance->parent_controller, instance->parent_controller, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test) != -1 ){
			amount--;
		}

		put_top_x_on_bottom(instance->parent_controller, instance->parent_controller, amount);
	}

	return 0;
}

int card_wall_of_resurgence(int player, int card, event_t event){
	/* Wall of Resurgence	|2|W	0x200e4cc
	 * Creature - Wall 0/6
	 * Defender
	 * When ~ enters the battlefield, you may put three +1/+1 counters on target land you control.
	 If you do, that land becomes a 0/0 Elemental creature with haste that's still a land. */

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = td.preferred_controller = player;

		if( can_target(&td) && new_pick_target(&td, "Select target land you control.", 0, GS_LITERAL_PROMPT) ){
			card_instance_t* inst = get_card_instance(player, card);
			add_a_subtype(inst->targets[0].player, inst->targets[0].card, SUBTYPE_ELEMENTAL);
			add_1_1_counters(inst->targets[0].player, inst->targets[0].card, 3);
			animate_other(player, card, inst->targets[0].player, inst->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE, 0, 1);
			inst->number_of_targets = 0;
		}
	}

	return 0;
}

/*** Blue - Devoid ***/

int card_abstruse_interference(int player, int card, event_t event){
	/* Abstruse Interference	|2|U	0x200e4d1
	 * Instant
	 * Devoid
	 * Counter target spell unless its controller pays |1. You put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |C to your mana pool." */

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		return counterspell(player, card, event, NULL, 1);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, 1);
		generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_blinding_drone(int player, int card, event_t event){
	/* Blinding Drone	|1|U	0x200e4d6
	 * Creature - Eldrazi Drone 1/3
	 * Devoid
	 * |C, |T: Tap target creature. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_X(1), 0, &td2, NULL) )
		{
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td2, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

/* Cultivator Drone	|2|U --> Skipped (mana usage restriction is impossible righ now)
 * Creature - Eldrazi Drone 2/3
 * Devoid
 * |T: Add |C to your mana pool. Spend this mana only to cast a colorless spell, activate an ability of a colorless permanent, or pay a cost that contains |C. */

int card_deepfathom_skulker(int player, int card, event_t event){
	/* Deepfathom Skulker	|5|U	0x200e4db
	 * Creature - Eldrazi 4/4
	 * Devoid
	 * Whenever a creature you control deals combat damage to a player, you may draw a card.
	 * |3|C: Target creature can't be blocked this turn. */

	check_damage_test(player, -1, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER, player, card, TYPE_CREATURE, NULL);

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( ! is_humiliated(player, card) ){
			card_instance_t *instance = get_card_instance(player, card);
			if( instance->targets[8].player > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_AI(player);
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						if (--instance->targets[8].player > 0){
							instance->state &= ~STATE_PROCESSING;	// More optional triggers left.  Must be the first thing done during resolve trigger.
						}
						draw_cards(player, 1);
				}
				else if (event == EVENT_END_TRIGGER){
						instance->targets[8].player = 0;
				}
			}
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(4), 0, &td2, NULL) )
		{
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(3), 0, &td2, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card,
									instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		}
	}

	return 0;
}

int card_dimensional_infiltrator(int player, int card, event_t event){
	/* Dimensional Infiltrator	|1|U	0x200e4e0
	 * Creature - Eldrazi 2/1
	 * Devoid
	 * Flash
	 * Flying
	 * |1|C: Target opponent exiles the top card of his or her library. If it's a land card, you may return ~ to its owner's hand. */

	if( ! IS_GAA_EVENT(event) ){
		return flash(player, card, event);
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, 0);
	td2.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_X(2), 0, &td2, NULL) )
		{
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_X(1), 0, &td2, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			int *deck = deck_ptr[instance->targets[0].player];
			int bounce = 0;
			if( deck[0] != -1 ){
				show_deck(player, deck, 1, "Card exiled by Dimensional Infiltrator.", 0, 0x7375B0 );
				int iid = deck[0];
				rfg_top_card_of_deck(instance->targets[0].player);
				if( is_what(-1, iid, TYPE_LAND) ){
					int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_NO_CANCEL,
										"Return Dimensional Infiltrator to hand", 1, 5,
										"Decline", 1, 1);
					bounce = (choice == 1 ? 1 : 0);
				}
			}
			if( bounce ){
				bounce_permanent(instance->parent_controller, instance->parent_card);
			}
		}
	}

	return 0;
}

int card_gravity_negator(int player, int card, event_t event){
	/* Gravity Negator	|3|U	0x200e4e5
	 * Creature - Eldrazi Drone 2/3
	 * Devoid
	 * Flying
	 * Whenever ~ attacks, you may pay |C. If you do, another target creature gains flying until end of turn. */

	store_attackers(player, card, event, 0, player, card, NULL);

	if (xtrigger_condition() == XTRIGGER_ATTACKING && affect_me(player, card) && reason_for_trigger_controller == player){
		if(  has_colorless_only_mana(player, 1) ){
			if( resolve_declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
				if( charge_colorless_mana_only(player, 1) ){
					target_definition_t td2;
					default_target_definition(player, card, &td2, TYPE_CREATURE);
					td2.preferred_controller = player;
					td2.special = TARGET_SPECIAL_NOT_ME;

					if( can_target(&td2) && new_pick_target(&td2, "Select another target creature.", 0, GS_LITERAL_PROMPT) ){
						card_instance_t *instance = get_card_instance(player, card);
						pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
												0, 0, KEYWORD_FLYING, 0);
					}
				}
			}
		}
	}

	return 0;
}

int card_prophet_of_distortion(int player, int card, event_t event){
	/* Prophet of Distortion	|U	0x200e4ea
	 * Creature - Eldrazi Drone 1/2
	 * Devoid
	 * |3|C: Draw a card. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, 0, MANACOST_X(4), 0, NULL, NULL) )
		{
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, 0, MANACOST_X(3), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *inst = get_card_instance(player, card);
		draw_cards(inst->parent_controller, 1);
	}

	return 0;
}


int card_slip_through_space(int player, int card, event_t event){
	/* Slip Through Space	|U	0x200e4ef
	 * Sorcery
	 * Devoid
	 * Target creature can't be blocked this turn.
	 * Draw a card. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_thought_harvester(int player, int card, event_t event){
	/* Thought Harvester	|3|U	0x200e4f4
	 * Creature - Eldrazi Drone 2/4
	 * Devoid
	 * Flying
	 * Whenever you cast a colorless spell, target opponent exiles the top card of his or her library. */

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller ){
		test_definition_t test;
		default_test_definition(&test, TYPE_ANY);
		test.color = COLOR_TEST_COLORLESS;

		target_definition_t td2;
		default_target_definition(player, card, &td2, 0);
		td2.zone = TARGET_ZONE_PLAYERS;

		if( would_validate_arbitrary_target(&td2, 1-player, -1) && new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test) ){
			rfg_top_card_of_deck(1-player);
		}
	}

	return 0;
}

/* Void Shatter	|1|U|U --> Dissipate	0x200191b
 * Instant
 * Devoid
 * Counter target spell. If that spell is countered this way, exile it instead of putting it into its owner's graveyard. */

/*** Blue ***/

/* Ancient Crab	|1|U|U --> vanilla	0x401000
 * Creature - Crab 1/5 */

int card_comparative_analysis(int player, int card, event_t event){
	/* Comparative Analysis	|3|U	0x200e4f9
	 * Instant
	 * Surge |2|U
	 * Target player draws two cards. */

	modify_casting_cost_for_surge(player, card, event, MANACOST_XU(2, 1));

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = surge(player, card, event, MANACOST_XU(2, 1));
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(instance->targets[0].player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_containment_membrane(int player, int card, event_t event){
	/* Containment Membrane	|2|U	0x200e4fe
	 * Enchantment - Aura
	 * Surge |U
	 * Enchant creature
	 * Enchanted creature doesn't untap during its controller's untap step. */

	modify_casting_cost_for_surge(player, card, event, MANACOST_U(1));

	if( event == EVENT_UNTAP ){
		card_instance_t *instance = get_card_instance( player, card);
		if( instance->damage_target_player > -1 ){
			does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( !(event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = surge(player, card, event, MANACOST_U(1));
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
	}

	return 0;
}

int card_crush_of_tentacles(int player, int card, event_t event){
	/* Crush of Tentacles	|4|U|U	0x200e503
	 * Sorcery
	 * Surge |3|U|U
	 * Return all nonland permanents to their owners' hands.
	 * If ~'s surge cost was paid, put an 8/8 |Sblue Octopus creature token onto the battlefield. */

	modify_casting_cost_for_surge(player, card, event, MANACOST_XU(3, 2));

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = surge(player, card, event, MANACOST_XU(3, 2));
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		if( result == 2 ){
			get_card_instance(player, card)->info_slot |= 2;
		}
		return basic_spell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		default_test_definition(&test, TYPE_LAND);
		test.type_flag = DOESNT_MATCH;
		APNAP(p, { new_manipulate_all(player, card, p, &test, ACT_BOUNCE); };);

		if( get_card_instance(player, card)->info_slot & 2 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_OCTOPUS, &token);
			token.pow = token.tou = 8;
			token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
			generate_token(&token);
		}

		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cyclone_sire(int player, int card, event_t event){
	/* Cyclone Sire	|4|U	0x200e508
	 * Creature - Elemental 3/4
	 * Flying
	 * When ~ dies, you may put three +1/+1 counters on target land you control.
	 * If you do, that land becomes a 0/0 Elemental creature with haste that's still a land. */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = td.preferred_controller = player;

		if( can_target(&td) && new_pick_target(&td, "Select target land you control.", 0, GS_LITERAL_PROMPT) ){
			card_instance_t* inst = get_card_instance(player, card);
			add_a_subtype(inst->targets[0].player, inst->targets[0].card, SUBTYPE_ELEMENTAL);
			add_1_1_counters(inst->targets[0].player, inst->targets[0].card, 3);
			animate_other(player, card, inst->targets[0].player, inst->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE, 0, 1);
			inst->number_of_targets = 0;
		}
	}

	return 0;
}

int card_gift_of_tusks(int player, int card, event_t event){
	/* Gift of Tusks	|U	0x200e50d
	 * Instant
	 * Until end of turn, target creature loses all abilities and becomes a |Sgreen Elephant with base power and toughness 3/3. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int legacy = humiliate_and_set_pt_abilities(player, card, instance->targets[0].player, instance->targets[0].card, 6, NULL);
			card_instance_t *leg = get_card_instance( player, legacy );
			leg->targets[1].player = 3;
			leg->targets[1].card = 3;
			leg->targets[2].card = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_grip_of_the_roil(int player, int card, event_t event){
	/* Grip of the Roil	|2|U	0x200e512
	 * Instant
	 * Surge |1|U
	 * Tap target creature. It doesn't untap during its controller's next untap step.
	 * Draw a card. */

	modify_casting_cost_for_surge(player, card, event, MANACOST_XU(1, 1));

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = surge(player, card, event, MANACOST_XU(1, 1));
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int has_me_in_all_zones(int player, int card){
	int csvid = get_id(player, card);
	test_definition_t test;
	default_test_definition(&test, TYPE_ANY);
	test.id = csvid;
	test.zone = TARGET_ZONE_HAND;
	if( check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test) ){//In hand
		if( new_special_count_grave(player, &test) ){//In your graveyard
			if( check_rfg(player, csvid) ){//In exile
				APNAP(p,{
							int c;
							for(c=0; c<active_cards_count[p]; c++){
								if( in_play(p, c) && get_id(p, c) == csvid ){
									if( p == player || get_owner(p, c) == player ){
										return 1;
									}
								}
							}
						};
				);
			}
		}
	}
	return 0;
}

int card_hedron_alignment(int player, int card, event_t event){
	/* Hedron Alignment	|2|U	0x200e517
	 * Enchantment
	 * Hexproof
	 * At the beginning of your upkeep, you may reveal your hand. If you do, you win the game if you own a card named ~ in exile, in your hand, in your graveyard, and on the battlefield.
	 * |1|U: Scry 1. */

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		int ai_mode = has_me_in_all_zones(player, card) ? RESOLVE_TRIGGER_MANDATORY : 0;
		if( current_turn == player && upkeep_trigger_mode(player, card, event, (player == HUMAN ? RESOLVE_TRIGGER_OPTIONAL : ai_mode)) ){
			reveal_target_player_hand(player);
			if( has_me_in_all_zones(player, card) ){
				lose_the_game(1-player);
			}
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		scry(player, 1);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XU(1, 1), 0, NULL, NULL);
}

int card_jwar_isle_avenger(int player, int card, event_t event){
	/* Jwar Isle Avenger	|4|U	0x200e51c
	 * Creature - Sphinx 3/3
	 * Surge |2|U
	 * Flying */

	modify_casting_cost_for_surge(player, card, event, MANACOST_XU(2, 1));

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = surge(player, card, event, MANACOST_XU(2, 1));
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		if( result == 2 ){
			get_card_instance(player, card)->info_slot |= 2;
		}
		return basic_spell(player, card, event);
	}

	return 0;
}


/* Negate	|1|U	--> morningtide.c:card_negate()	reprint
 * Instant
 * Counter target noncreature spell. */

int card_oath_of_jace(int player, int card, event_t event){
	/* Oath of Jace	|2|U	0x200e521
	 * Legendary Enchantment
	 * When ~ enters the battlefield, draw three cards, then discard two cards.
	 * At the beginning of your upkeep, scry X, where X is the number of planeswalkers you control. */

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_ANY);
		test.subtype = SUBTYPE_PLANESWALKER;
		int amount = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &test);
		upkeep_trigger_ability_mode(player, card, event, player, (amount ? RESOLVE_TRIGGER_MANDATORY : 0));
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t test;
		default_test_definition(&test, TYPE_ANY);
		test.subtype = SUBTYPE_PLANESWALKER;
		int amount = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &test);
		scry(player, amount);
	}

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 3);
		new_multidiscard(player, 2, 0, player);
	}

	return global_enchantment(player, card, event);
}

int card_overwhelming_denial(int player, int card, event_t event){
	/* Overwhelming Denial	|2|U|U	0x200e526
	 * Instant
	 * Surge |U|U
	 * ~ can't be countered by spells or abilities.
	 * Counter target spell. */

	modify_casting_cost_for_surge(player, card, event, MANACOST_U(2));

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = surge(player, card, event, MANACOST_U(2));
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		state_untargettable(player, card, 1);
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		state_untargettable(player, card, 0);
		return counterspell(player, card, event, NULL, 0);
	}

	return 0;
}

int card_roiling_waters(int player, int card, event_t event){
	/* Roiling Waters	|5|U|U	0x200e52b
	 * Sorcery
	 * Return up to two target creatures your opponents control to their owners' hands. Target player draws two cards. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = td.preferred_controller = 1-player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, 0);
	td2.zone = TARGET_ZONE_PLAYERS;
	td2.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		int base_target = 0;
		if( can_target(&td) ){
			int i;
			for(i=0; i<2; i++){
				if( new_pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS", base_target, 0) ){
					base_target++;
				}
				else{
					break;
				}
			}
		}
		new_pick_target(&td2, "TARGET_PLAYER", base_target, 1);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( instance->targets[i].card != -1 ){
				if( validate_target(player, card, &td, i) ){
					bounce_permanent(instance->targets[i].player, instance->targets[i].card);
				}
			}
			else{
				if( validate_target(player, card, &td2, i) ){
					draw_cards(instance->targets[i].player, 2);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sphinx_of_the_final_word(int player, int card, event_t event){
	/* Sphinx of the Final Word	|5|U|U	0x200e530
	 * Creature - Sphinx 5/5
	 * ~ can't be countered.
	 * Flying, hexproof
	 * Instant and sorcery spells you control can't be countered by spells or abilities. */

	cannot_be_countered(player, card, event);

	hexproof(player, card, event);

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_CAST_SPELL && is_what(affected_card_controller, affected_card, TYPE_SPELL) &&
			affected_card_controller == player)
		{
			state_untargettable(affected_card_controller, affected_card, 1);
		}
	}

	return 0;
}

int card_sweep_away(int player, int card, event_t event){
	/* Sweep Away	|2|U	0x200e535
	 * Instant
	 * Return target creature to its owner's hand. If that creature is attacking, you may put it on top of its owner's library instead. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int bounce = 1;
			if( is_attacking(instance->targets[0].player, instance->targets[0].card) ){
				int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_NO_CANCEL,
									"Put on top of deck", 1, 10,
									"Bounce", 1, 5);
				if( choice == 1 ){
					bounce = 0;
					put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
				}
			}
			if( bounce ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}


/* Umara Entangler	|1|U --> Jeskai Windscout	0x200ce74
 * Creature - Merfolk Rogue Ally 2/1
 * Prowess */

static int has_1_1_counter(int unused, int unused2, int player, int card){
	return count_1_1_counters(player, card) ? 1 : 0;
}

int card_unity_of_purpose(int player, int card, event_t event){
	/* Unity of Purpose	|3|U	0x200e53a
	 * Instant
	 * Support 2.
	 * Untap each creature you control with a +1/+1 counter on it. */

	if( event == EVENT_RESOLVE_SPELL ){
		support(player, card, 2);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.special_selection_function = &has_1_1_counter;
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}


/*** Black - Devoid ***/

int card_bearer_of_silence(int player, int card, event_t event){
	/* Bearer of Silence	|1|B	0x200e544
	 * Creature - Eldrazi 2/1
	 * Devoid
	 * When you cast ~, you may pay |1|C. If you do, target opponent sacrifices a creature.
	 * Flying
	 * ~ can't block. */

	cannot_block(player, card, event);

	if( comes_into_play(player, card, event) ){
		if( player_has_generic_and_colorless_available(player, 2, 1) ){
			if( charge_generic_mana_with_colorless_only(player, 2, 1) ){

			}
		}
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		int ai_mode = RESOLVE_TRIGGER_OPTIONAL;
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		if( player == AI ){
			if( player_has_generic_and_colorless_available(player, 2, 1) && would_validate_arbitrary_target(&td, 1-player, -1)  ){
				ai_mode = RESOLVE_TRIGGER_MANDATORY;
			}
			else{
				ai_mode = 0;
			}
		}
		if( comes_into_play_mode(player, card, event, ai_mode) ){
			if( player_has_generic_and_colorless_available(player, 2, 1) && would_validate_arbitrary_target(&td, 1-player, -1) ){
				if( charge_generic_mana_with_colorless_only(player, 2, 1) ){
					player_sacrifices_a_permanent(player, card, 1-player, TYPE_CREATURE, SAC_CAUSED);
				}
			}
		}
	}

	return 0;
}

int card_dread_defiler(int player, int card, event_t event){
	/* Dread Defiler	|6|B	0x200e549
	 * Creature - Eldrazi 6/8
	 * Devoid
	 * |3|C, Exile a creature card from your graveyard: Target opponent loses life equal to the exiled card's power. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, 0);
	td2.zone = TARGET_ZONE_PLAYERS;

	test_definition_t test;
	new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card to exile.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_X(4), 0, &td2, NULL) )
		{
			return new_special_count_grave(player, &test) ? 1 : 0;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_CMC, -1, &test);
			if( selected == -1 ){
				spell_fizzled = 1;
				return 0;
			}
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
			instance->info_slot = get_cmc_by_internal_id(get_grave(player)[selected]);
			rfg_card_from_grave(player, selected);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			lose_life(instance->targets[0].player, instance->info_slot);
		}
	}

	return 0;
}

int card_essence_depleter(int player, int card, event_t event){
	/* Essence Depleter	|2|B	0x200e54e
	 * Creature - Eldrazi Drone 2/3
	 * Devoid
	 * |1|C: Target opponent loses 1 life and you gain 1 life. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, 0);
	td2.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) ){
			return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_X(2), 0, &td2, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_X(1), 0, &td2, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td2) ){
			lose_life(instance->targets[0].player, 1);
			gain_life(player, 1);
		}
	}

	return 0;
}

int card_flaying_tendrils(int player, int card, event_t event){
	/* Flaying Tendrils	|1|B|B	0x200e553
	 * Sorcery
	 * Devoid
	 * All creatures get -2/-2 until end of turn. If a creature would die this turn, exile it instead. */

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
					int c;
					for(c=active_cards_count[p]-1; c>-1; c--){
						if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) ){
							exile_if_would_be_put_into_graveyard(player, card, p, c, 1);
						}
					}
				};
		);
		APNAP(p,{
					int c;
					for(c=active_cards_count[p]-1; c>-1; c--){
						if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) ){
							pump_until_eot(player, card, p, c, -2, -2);
						}
					}
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_havoc_sower(int player, int card, event_t event){
	/* Havoc Sower	|3|B	0x200e558
	 * Creature - Eldrazi Drone 3/3
	 * Devoid
	 * |1|C: ~ gets +2/+1 until end of turn. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) ){
			return generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_until_eot_merge_previous(instance->parent_controller, instance->parent_card,
										instance->parent_controller, instance->parent_card, 2, 1);
	}

	if (event == EVENT_POW_BOOST ){
		if( has_colorless_only_mana(player, 1) ){
			if( has_mana(player, COLOR_ANY, 2) ){
				event_result += 2;
			}
		}
	}

	if (event == EVENT_TOU_BOOST){
		if( has_colorless_only_mana(player, 1) ){
			if( has_mana(player, COLOR_ANY, 2) ){
				event_result++;
			}
		}
	}

	return 0;
}

int card_inverter_of_truth(int player, int card, event_t event){
	/* Inverter of Truth	|2|B|B	0x200e55d
	 * Creature - Eldrazi 6/6
	 * Devoid
	 * Flying
	 * When ~ enters the battlefield, exile all cards from your library face down,
	 then shuffle all cards from your graveyard into your library. */

	if( comes_into_play(player, card, event) ){
		int *deck = deck_ptr[player];
		while( deck[0] != -1 ){
				rfg_top_card_of_deck(player);
		}
		const int *grave = get_grave(player);
		int cd = 0;
		int cg = count_graveyard(player)-1;
		while( cg > -1 ){
				deck[cd] = grave[cg];
				remove_card_from_grave(player, cg);
				cd++;
				cg--;
		}
		if( cd > 0 ){
			shuffle(player);
		}
	}

	return 0;
}

int card_kozileks_shrieker(int player, int card, event_t event){
	/* Kozilek's Shrieker	|2|B	0x200e562
	 * Creature - Eldrazi Drone 3/2
	 * Devoid
	 * |C: ~ gets +1/+0 and gains menace until end of turn. */

	if( get_card_instance(player, card)->targets[1].player == 66 ){
		menace(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) ){
			return generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_until_eot_merge_previous(instance->parent_controller, instance->parent_card,
										instance->parent_controller, instance->parent_card, 1, 0);
		card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
		parent->targets[1].player = 66;
	}

	if (event == EVENT_POW_BOOST ){
		if( has_colorless_only_mana(player, 1) ){
			event_result++;
		}
	}

	if( event == EVENT_CLEANUP ){
		get_card_instance(player, card)->targets[1].player = 0;
	}

	return 0;
}

int card_kozileks_translator(int player, int card, event_t event){
	/* Kozilek's Translator	|4|B	0x200e567
	 * Creature - Eldrazi Drone 3/5
	 * Devoid
	 * Pay 1 life: Add |C to your mana pool. Activate this ability only once each turn. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_pay_life(player, 1) && get_card_instance(player, card)->info_slot != 66 ){
			return can_produce_mana(player, card);
		}
	}

	if( event == EVENT_ACTIVATE ){
		lose_life(player, 1);
		produce_mana(player, COLOR_COLORLESS, 1);
		get_card_instance(player, card)->info_slot = 66;
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_pay_life(player, 1) && get_card_instance(player, card)->info_slot != 66 ){
			if( can_produce_mana(player, card) ){
				declare_mana_available(player, COLOR_COLORLESS, 1);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		get_card_instance(player, card)->info_slot = 0;
	}

	return 0;
}

int card_oblivion_strike(int player, int card, event_t event){
	/* Oblivion Strike	|3|B	0x200e56c
	 * Sorcery
	 * Devoid
	 * Exile target creature. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_reaver_drone(int player, int card, event_t event){
	/* Reaver Drone	|B	0x200e571
	 * Creature - Eldrazi Drone 2/1
	 * Devoid
	 * At the beginning of your upkeep, you lose 1 life unless you control another colorless creature. */
	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_ANY);
		test.color = COLOR_COLORLESS;
		test.not_me = 1;
		int amount = check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test);
		upkeep_trigger_ability_mode(player, card, event, player, (amount ? RESOLVE_TRIGGER_MANDATORY : 0));
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(player, 1);
	}

	return 0;
}

static int sifter_of_skulls_effect(int player, int card, event_t event){

	//In case something is copying Sifter of Skulls and then reverts to its original form.
	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( get_id(instance->damage_target_player, instance->damage_target_card) != CARD_ID_SIFTER_OF_SKULLS ){
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			card_instance_t *aff = get_card_instance(affected_card_controller, affected_card);
			if( aff->kill_code > 0 ){
				instance->damage_target_player = instance->damage_target_card = -1;
				remove_status(player, card, STATUS_INVISIBLE_FX);
			}
			return 0;
		}
		if( ! check_status(player, card, STATUS_INVISIBLE_FX) && ! is_token(affected_card_controller, affected_card) ){
			count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);
		}
	}

	if( ! check_status(player, card, STATUS_INVISIBLE_FX) ){
		int amount = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
		if( amount ){

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ELDRAZI_SCION, &token);
			token.qty = amount;
			generate_token(&token);

			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_sifter_of_skulls(int player, int card, event_t event){
	/* Sifter of Skulls	|3|B	0x200e576
	 * Creature - Eldrazi 4/3
	 * Devoid
	 * Whenever another nontoken creature you control dies, put a 1/1 colorless Eldrazi Scion creature token onto the battlefield.
	 * It has "Sacrifice this creature: Add |C to your mana pool." */

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		int found = 0;
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_EFFECT) ){
				card_instance_t *inst = get_card_instance(player, c);
				if( inst->info_slot == (int)sifter_of_skulls_effect ){
					found = 1;
					break;
				}
			}
		}
		if( ! found ){
			int legacy = create_targetted_legacy_effect(player, card, &sifter_of_skulls_effect, player, card);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) ){
		if( ! is_token(affected_card_controller, affected_card) && ! affect_me(player, card) ){
			count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);
		}
	}

	if( get_card_instance(player, card)->kill_code <= 0 ){
		int amount = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
		if( amount ){

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ELDRAZI_SCION, &token);
			token.qty = amount;
			generate_token(&token);

		}
	}

	return 0;
}

int card_sky_scourer(int player, int card, event_t event){
	/* Sky Scourer	|1|B	0x200e57b
	 * Creature - Eldrazi Drone 1/2
	 * Devoid
	 * Flying
	 * Whenever you cast a colorless spell, ~ gets +1/+0 until end of turn. */
	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_ANY);
		test.color_flag = DOESNT_MATCH;
		test.color = COLOR_TEST_ANY_COLORED;
		if( new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test) ){
			pump_until_eot_merge_previous(player, card, player, card, 1, 0);
		}
	}

	return 0;
}

int card_slaughter_drone(int player, int card, event_t event){
	/* Slaughter Drone	|1|B	0x200e580
	 * Creature - Eldrazi Drone 2/2
	 * Devoid
	 * |C: ~ gains deathtouch until end of turn. */
	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) ){
			return generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								0, 0, 0, SP_KEYWORD_DEATHTOUCH);
	}

	return 0;
}

/* Unnatural Endurance	|B --> Boon of Erebos	0x200aed5
 * Instant
 * Devoid
 * Target creature gets +2/+0 until end of turn. Regenerate it. */

int card_visions_of_brutality(int player, int card, event_t event){
	/* Visions of Brutality	|1|B	0x200e585
	 * Enchantment - Aura
	 * Devoid
	 * Enchant creature
	 * Enchanted creature can't block.
	 * Whenever enchanted creature deals damage, its controller loses that much life. */

	if( get_card_instance(player, card)->damage_target_player > -1 ){
		card_instance_t *instance = get_card_instance(player, card);
		if( damage_dealt_by_me_arbitrary(player, card, event, DDBM_REPORT_DAMAGE_DEALT,
										instance->damage_target_player, instance->damage_target_card) )
		{
			lose_life(instance->damage_target_player, instance->targets[16].player);
			instance->targets[16].player = 0;
		}
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK, 0, 0, 0);
}

int card_witness_the_end(int player, int card, event_t event){
	/* Witness the End	|3|B	0x200e58a
	 * Sorcery
	 * Devoid
	 * Target opponent exiles two cards from his or her hand and loses 2 life. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.allowed_controller = 1-player;
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ANY, "Select a card to exile");
			test.qty = MIN(2, hand_count[instance->targets[0].player]);
			new_global_tutor(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, TUTOR_RFG, 1, AI_MIN_VALUE, &test);
			lose_life(instance->targets[0].player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}


/*** Black ***/

int card_corpse_churn(int player, int card, event_t event){
	/* Corpse Churn	|1|B	0x200e58f
	 * Instant
	 * Put the top three cards of your library into your graveyard, then you may return a creature card from your graveyard to your hand. */

	if( event == EVENT_RESOLVE_SPELL ){
		mill(player, 3);
		if( count_graveyard_by_type(player, TYPE_CREATURE) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return basic_spell(player, card, event);
}

int card_dranas_chosen(int player, int card, event_t event){
	/* Drana's Chosen	|3|B	0x200e594
	 * Creature - Vampire Shaman Ally 2/2
	 * Cohort - |T, Tap an untapped Ally you control: Put a 2/2 |Sblack Zombie creature token onto the battlefield tapped. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_cohort(player, card) ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->number_of_targets = 0;
		int result = pick_target_for_cohort(player, card);
		if( result == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		if( spell_fizzled != 1 ){
			tap_card(player, result);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		token.pow = token.tou = 2;
		token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		generate_token(&token);
	}

	return 0;
}

/* Grasp of Darkness	|B|B	=>scars_of_mirrodin.c:card_grasp_of_darkness()
 * Instant
 * Target creature gets -4/-4 until end of turn. */

static int kalitas_traitor_of_ghet(int player, int card, event_t event){

	//In case something is copying Sifter of Skulls and then reverts to its original form.
	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( get_id(instance->damage_target_player, instance->damage_target_card) != CARD_ID_KALITAS_TRAITOR_OF_GHET ){
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			card_instance_t *aff = get_card_instance(affected_card_controller, affected_card);
			if( aff->kill_code > 0 ){
				instance->damage_target_player = instance->damage_target_card = -1;
				remove_status(player, card, STATUS_INVISIBLE_FX);
			}
		}
		if( ! check_status(player, card, STATUS_INVISIBLE_FX) && ! is_token(affected_card_controller, affected_card) ){
			count_for_gfp_ability(player, card, event, 1-player, TYPE_CREATURE, NULL);
		}
	}

	if( ! check_status(player, card, STATUS_INVISIBLE_FX) ){
		int amount = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
		if( amount ){

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
			token.pow = token.tou = 2;
			token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
			token.qty = amount;
			generate_token(&token);

			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_kalitas_traitor_of_ghet(int player, int card, event_t event){
	/* Kalitas, Traitor of Ghet	|2|B|B	0x200e599
	 * Legendary Creature - Vampire Warrior 3/4
	 * Lifelink
	 * If a nontoken creature an opponent controls would die, instead exile that card and put a 2/2 |Sblack Zombie
	 * creature token onto the battlefield.
	 * |2|B, Sacrifice another Vampire or Zombie: Put two +1/+1 counters on ~. */

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		int found = 0;
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_EFFECT) ){
				card_instance_t *inst = get_card_instance(player, c);
				if( inst->info_slot == (int)kalitas_traitor_of_ghet ){
					found = 1;
					break;
				}
			}
		}
		if( ! found ){
			int legacy = create_targetted_legacy_effect(player, card, &kalitas_traitor_of_ghet, player, card);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) ){
		if( ! is_token(affected_card_controller, affected_card) ){
			count_for_gfp_ability(player, card, event, 1-player, TYPE_CREATURE, NULL);
		}
	}

	if( get_card_instance(player, card)->kill_code <= 0 ){
		int amount = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
		if( amount ){

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
			token.pow = token.tou = 2;
			token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
			token.qty = amount;
			generate_token(&token);

		}
	}

	check_legend_rule(player, card, event);

	lifelink(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t test;
	new_default_test_definition(&test, TYPE_PERMANENT, "Select a Zombie or a Vampire to sacrifice.");
	test.subtype = SUBTYPE_ZOMBIE;
	test.sub2 = SUBTYPE_VAMPIRE;
	test.subtype_flag = F2_MULTISUBTYPE;

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XB(2, 1), 0, NULL, NULL) ){
			return new_can_sacrifice_as_cost(player, card, &test);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XB(2, 1), 0, NULL, NULL) ){
			if( ! new_sacrifice(player, card, player, SAC_AS_COST, &test) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		add_1_1_counters(instance->parent_controller, instance->parent_card, 2);
	}

	return 0;
}

int card_malakir_soothsayer(int player, int card, event_t event){
	/* Malakir Soothsayer	|4|B	0x200e59e
	 * Creature - Vampire Shaman Ally 4/4
	 * Cohort - |T, Tap an untapped Ally you control: You draw a card and you lose 1 life. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_cohort(player, card) ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->number_of_targets = 0;
		int result = pick_target_for_cohort(player, card);
		if( result == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		if( spell_fizzled != 1 ){
			tap_card(player, result);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
		lose_life(player, 1);
	}

	return 0;
}


int card_null_caller(int player, int card, event_t event){
	/* Null Caller	|3|B	0x200e5a3
	 * Creature - Vampire Shaman 2/4
	 * |3|B, Exile a creature card from your graveyard: Put a 2/2 |Sblack Zombie creature token onto the battlefield tapped. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t test;
	new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card to exile.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_XB(3, 1), 0, NULL, NULL) )
		{
			return new_special_count_grave(player, &test) ? 1 : 0;
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XB(3, 1)) ){
			if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &test) == -1 ){
				spell_fizzled = 1;
				return 0;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(instance->parent_controller, instance->parent_card, CARD_ID_ZOMBIE, &token);
		token.pow = token.tou = 2;
		token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		generate_token(&token);
	}

	return 0;
}

static int is_creature_or_pwalker(int unused, int unused2, int player, int card){
	if( is_what(player, card, TYPE_CREATURE) || is_planeswalker(player, card) ){
		return 1;
	}
	return 0;
}

int card_remorseless_punishment(int player, int card, event_t event){
	/* Remorseless Punishment	|3|B|B	0x200e5a8
	 * Sorcery
	 * Target opponent loses 5 life unless that player discards two cards or sacrifices a creature or planeswalker.
	 Repeat this process once. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int p = instance->targets[0].player;
			int i;
			for(i=0; i<2; i++){
				test_definition_t test;
				new_default_test_definition(&test, TYPE_PERMANENT, "Select a creature or planeswalker to sacrifice.");
				test.special_selection_function = &is_creature_or_pwalker;

				int priorities[3] = {20, 5, 10};
				if( life[p]-5 < 6 ){
					priorities[0] = 5;
				}
				if( life[p]-5 <= 0 ){
					priorities[0] = 1;
				}
				if( hand_count[p] > 4 ){
					priorities[1] = 20;
				}
				if( check_battlefield_for_special_card(player, card, p, CBFSC_GET_COUNT, &test) < 2 ){
					priorities[1] = 5;
				}
				int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM, DLG_NO_CANCEL, DLG_WHO_CHOOSES(p),
									"Lose 5 life", 1, priorities[0],
									"Discard 2 cards", (hand_count[p] >= 2 ? 1 : 0), priorities[1],
									"Sac a creature or planeswalker", new_can_sacrifice(player, card, p, &test), priorities[2]);
				if( choice == 1 ){
					lose_life(p, 5);
				}
				if( choice == 2 ){
					new_multidiscard(p, 2, 0, player);
				}
				if( choice == 3 ){
					new_sacrifice(player, card, p, 1, &test);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_tar_snare(int player, int card, event_t event){
	/* Tar Snare	|2|B	0x200e5ad
	 * Instant
	 * Target creature gets -3/-2 until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, 1-player, -3, -2, 0, 0);
}

int card_untamed_hunger(int player, int card, event_t event){
	/* Untamed Hunger	|2|B	0x200e5b2
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +2/+1 and has menace. */

	if( get_card_instance(player, card)->damage_target_player > -1 ){
		fx_menace(player, card, event);
	}

	return generic_aura(player, card, event, player, 2, 1, 0, 0, 0, 0, 0);
}

int card_vampire_envoy(int player, int card, event_t event){
	/* Vampire Envoy	|2|B	0x200e5b7
	 * Creature - Vampire Cleric Ally 1/4
	 * Flying
	 * Whenever ~ becomes tapped, you gain 1 life. */

	if( ! is_humiliated(player, card) && event == EVENT_TAP_CARD && affect_me(player, card) ){
		gain_life(player, 1);
	}

	return 0;
}

int card_zulaport_chainmage(int player, int card, event_t event){
	/* Zulaport Chainmage	|3|B	0x200e5bc
	 * Creature - Human Shaman Ally 4/2
	 * Cohort - |T, Tap an untapped Ally you control: Target opponent loses 2 life. */

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_cohort(player, card) ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST0, 0, &td, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->number_of_targets = 0;
		int result = pick_target_for_cohort(player, card);
		if( result == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST0, 0, &td, NULL);
		if( spell_fizzled != 1 ){
			tap_card(player, result);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 2);
		}
	}

	return 0;
}

/*** Red - Devoid ***/

int card_consuming_sinkhole(int player, int card, event_t event){
	/* Consuming Sinkhole	|3|R	0x200e5c1
	 * Instant
	 * Devoid
	 * Choose one -
	 * * Exile target land creature.
	 * * ~ deals 4 damage to target player. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int)is_land_creature_target;

	target_definition_t td2;
	default_target_definition(player, card, &td2, 0);
	td2.zone = TARGET_ZONE_PLAYERS;

	enum
	{
		CHOICE_KILL_MANLAND = 1,
		CHOICE_LOSE_LIFE
	};

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
			return 1;
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td2, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			int choice = DIALOG(player, card, event, DLG_RANDOM,
								"Kill mandland", can_target(&td), 10,
								"Target loses 4 life", can_target(&td2), (life[1-player]-4 < 6 ? 15 : 5));
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
		}
		if( instance->info_slot == CHOICE_KILL_MANLAND ){
			new_pick_target(&td, "Select target land creature.", 0, 1 | GS_LITERAL_PROMPT);
		}
		if( instance->info_slot == CHOICE_LOSE_LIFE ){
			pick_target(&td, "TARGET_PLAYER");
		}

	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == CHOICE_KILL_MANLAND && validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		if( instance->info_slot == CHOICE_LOSE_LIFE && validate_target(player, card, &td2, 0) ){
			lose_life(instance->targets[0].player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_eldrazi_aggressor(int player, int card, event_t event){
	/* Eldrazi Aggressor	|2|R	0x200e5c6
	 * Creature - Eldrazi Drone 2/3
	 * Devoid
	 * ~ has haste as long as you control another colorless creature. */

	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.color = COLOR_TEST_COLORLESS;
		if( check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test) ){
			haste(player, card, event);
		}
	}

	return 0;
}

int card_eldrazi_obligator(int player, int card, event_t event){
	/* Eldrazi Obligator	|2|R	0x200e5cb
	 * Creature - Eldrazi 3/1
	 * Devoid
	 * When you cast ~, you may pay |1|C. If you do, gain control of target creature until end of turn, untap that creature,
	 * and it gains haste until end of turn.
	 * Haste */

	haste(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			if( player_has_generic_and_colorless_available(player, 2, 1) && can_target(&td) ){
				if( charge_generic_mana_with_colorless_only(player, 2, 1) && new_pick_target(&td, "TARGET_CREATURE", 0, 0) ){
					card_instance_t* instance = get_card_instance(player, card);
					effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
				}
			}
		}
	}

	return 0;
}

static int immobilizer_eldrazi_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player < 0 ){
		return 0;
	}

	if( event == EVENT_BLOCK_LEGALITY && affect_me(instance->damage_target_player, instance->damage_target_card) ){
		if( attacking_card_controller == instance->targets[0].player && attacking_card == instance->targets[0].card ){
			if( get_toughness(instance->damage_target_player, instance->damage_target_card) >
				get_toughness(instance->damage_target_player, instance->damage_target_card) )
			{
				event_result = 1;
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_immobilizer_eldrazi(int player, int card, event_t event){
	/* Immobilizer Eldrazi	|1|R	0x200e5d0
	 * Creature - Eldrazi Drone 2/1
	 * Devoid
	 * |2|C: Each creature with toughness greater than its power can't block this turn. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(3), 0, NULL, NULL) )
		{
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(2), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		int c;
		for(c=active_cards_count[1-player]; c>-1; c--){
			if( in_play(1-player, c) && is_what(1-player, c, TYPE_CREATURE) ){
				int legacy = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &immobilizer_eldrazi_legacy,
															1-player, c);
				card_instance_t *leg = get_card_instance(instance->parent_controller, legacy);
				leg->targets[0].player = instance->parent_controller;
				leg->targets[0].card = instance->parent_card;
				leg->number_of_targets = 1;
			}
		}
	}

	return 0;
}

/* Kozilek's Return	|2|R --> Pyroclasm (the graveyard effect is in "rules_engine.c")	0x200cd98
 * Instant
 * Devoid
 * ~ deals 2 damage to each creature.
 * Whenever you cast an Eldrazi creature spell with converted mana cost 7 or greater, you may exile ~ from your graveyard.
 * If you do, ~ deals 5 damage to each creature. */

int card_maw_of_kozilek(int player, int card, event_t event){
	/* Maw of Kozilek	|3|R	0x200e5d5
	 * Creature - Eldrazi Drone 2/5
	 * Devoid
	 * |C: ~ gets +2/-2 until end of turn. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) ){
			return generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_until_eot_merge_previous(instance->parent_controller, instance->parent_card,
										instance->parent_controller, instance->parent_card, 2, -1);
	}

	if (event == EVENT_POW_BOOST ){
		if( has_colorless_only_mana(player, 1) ){
			event_result+=2;
		}
	}

	if (event == EVENT_TOU_BOOST ){
		if( has_colorless_only_mana(player, 1) ){
			event_result-=2;
		}
	}

	return 0;
}

/* Reality Hemorrhage	|1|R --> Shock	0x200cb8b
 * Instant
 * Devoid
 * ~ deals 2 damage to target creature or player. */

/*** Red ***/

int card_akoum_flameseeker(int player, int card, event_t event){
	/* Akoum Flameseeker	|2|R	0x200e5da
	 * Creature - Human Shaman Ally 3/2
	 * Cohort - |T, Tap an untapped Ally you control: Discard a card. If you do, draw a card. */

	if (!IS_GAA_EVENT(event))
		return 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_cohort(player, card) ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->number_of_targets = 0;
		int result = pick_target_for_cohort(player, card);
		if( result == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		if( spell_fizzled != 1 ){
			tap_card(player, result);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int p = instance->parent_controller;
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard");
		int selected = new_select_a_card(p, p, TUTOR_FROM_DECK, 0, AI_MIN_VALUE, -1, &this_test);
		if( selected != -1 ){
			lose_life(p, selected);
			draw_cards(p, 1);
		}
	}

	return 0;
}

int card_boulder_salvo(int player, int card, event_t event){
	/* Boulder Salvo	|4|R	0x200e5df
	 * Sorcery
	 * Surge |1|R
	 * ~ deals 4 damage to target creature. */

	modify_casting_cost_for_surge(player, card, event, MANACOST_XR(1, 1));

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = surge(player, card, event, MANACOST_XR(1, 1));
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_brute_strenght(int player, int card, event_t event){
	/* Brute Strength	|1|R	0x200e5e4
	 * Instant
	 * Target creature gets +3/+1 and gains trample until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 3, 1, KEYWORD_TRAMPLE, 0);
}

int card_chandra_flamecaller(int player, int card, event_t event){
	/* Chandra, Flamecaller	|4|R|R	0x200e5e9
	 * Planeswalker - Chandra (4)
	 * +1: Put two 3/1 |Sred Elemental creature tokens with haste onto the battlefield. Exile them at the beginning of the next end step.
	 * 0: Discard all the cards in your hand, then draw that many cards plus one.
	 * -X: ~ deals X damage to each creature. */

	card_instance_t *instance = get_card_instance( player, card);

	if( IS_GAA_EVENT(event) ){
		enum
		{
			CHOICE_ELEMENTALS = 1,
			CHOICE_DISCARD_DRAW,
			CHOICE_DAMAGE
		};

		if( event == EVENT_ACTIVATE ){
			instance->info_slot = 0;
			int priority[3] = {10, 1, 1};
			int tou_arc[2][count_counters(player, card, COUNTER_LOYALTY)+1];
			APNAP(p,{
						int c;
						for(c=0; c<(count_counters(player, card, COUNTER_LOYALTY)+1); c++){
							tou_arc[p][c] = 0;
						}
					};
			);
			APNAP(p,{
						int c;
						for(c=0; c<active_cards_count[p]; c++){
							if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) &&
								! check_for_ability(p, c, KEYWORD_PROT_RED) &&
								! check_for_special_ability(p, c, SP_KEYWORD_INDESTRUCTIBLE) )
							{
								int tou = get_toughness(p, c);
								if( tou <= count_counters(player, card, COUNTER_LOYALTY) ){
									tou_arc[p][tou]++;
								}
							}
						}
					};
			);
			int par = 0;
			int t_chosen = 0;
			int i;
			for(i=0; i<(count_counters(player, card, COUNTER_LOYALTY)+1); i++){
				if( (tou_arc[1-player][i] - tou_arc[player][i]) > par ){
					par = tou_arc[1-player][i] - tou_arc[player][i];
					t_chosen = i;
				}
			}
			priority[2] = MIN(1, 5*par);
			priority[1] = MIN(1, (hand_count[1-player]-hand_count[player])*4);
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
								"Generate 2 Elementals",		1,	priority[0],	+1,
								"Discard & draw",				1,	priority[1],	0,
								"Damage all creatures",			1,	priority[2],	0);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}

			if( choice == CHOICE_DAMAGE ){
				int amount = choose_a_number(player, "How many loyalty you'll remove?", t_chosen);
				if( amount > 0 && amount < count_counters(player, card, COUNTER_LOYALTY) ){
					remove_counters(player, card, COUNTER_LOYALTY, amount),
					instance->targets[1].player = amount;
				}
				else{
					if( amount < 0 || amount > count_counters(player, card, COUNTER_LOYALTY) ){
						spell_fizzled = 1;
						return 0;
					}
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int pl = instance->parent_controller;
			int ca = instance->parent_card;
			if( instance->info_slot == CHOICE_ELEMENTALS ){
				token_generation_t token;
				default_token_definition(pl, ca, CARD_ID_ELEMENTAL, &token);
				token.pow = 3;
				token.tou = 1;
				token.special_infos = 66;
				token.qty = 2;
				token.s_key_plus = SP_KEYWORD_HASTE;
				token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_RED);
				generate_token(&token);
			}
			if( instance->info_slot == CHOICE_DISCARD_DRAW ){
				int amount = hand_count[pl]+1;
				discard_all(pl),
				draw_cards(pl, amount);
			}
			if( instance->info_slot == CHOICE_DAMAGE ){
				APNAP(p, {new_damage_all(pl, ca, p, instance->targets[1].player, NDA_ALL_CREATURES, NULL);};);
			}
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_cinder_hellion(int player, int card, event_t event){
	/* Cinder Hellion	|4|R	0x200e5ee
	 * Creature - Hellion 4/4
	 * Trample
	 * When ~ enters the battlefield, it deals 2 damage to target opponent. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		int ai_mode = RESOLVE_TRIGGER_OPTIONAL;
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		if( player == AI ){
			ai_mode = would_validate_arbitrary_target(&td, 1-player, -1) ? RESOLVE_TRIGGER_MANDATORY : 0;
		}
		if( comes_into_play_mode(player, card, event, ai_mode) ){
			if( player_has_generic_and_colorless_available(player, 2, 1) && would_validate_arbitrary_target(&td, 1-player, -1) ){
				damage_player(1-player, 2, player, card);
			}
		}
	}

	return 0;
}

int card_devour_in_flames(int player, int card, event_t event){
	/* Devour in Flames	|2|R	0x200e5f3
	 * Sorcery
	 * As an additional cost to cast ~, return a land you control to its owner's hand.
	 * ~ deals 5 damage to target creature or planeswalker. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TARGET_TYPE_PLANESWALKER);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 5);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target target creature or planeswalker.", 1, NULL);
}

int card_embodiment_of_fury(int player, int card, event_t event){
	/* Embodiment of Fury	|3|R	0x200e5f8
	 * Creature - Elemental 4/3
	 * Trample
	 * Land creatures you control have trample.
	 * Landfall - Whenever a land enters the battlefield under your control, you may have target land you control become a 3/3 Elemental
	 * creature with haste until end of turn. It's still a land. */

	if( landfall_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = td.preferred_controller = player;

		card_instance_t *instance = get_card_instance( player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && new_pick_target(&td, "Select target land you control.", 0, GS_LITERAL_PROMPT) ){
			int legacy = turn_into_creature(player, card, instance->targets[0].player, instance->targets[0].card, 1, 3, 3);
			card_instance_t *leg = get_card_instance( player, legacy );
			leg->targets[8].card = SP_KEYWORD_HASTE;
		}
	}

	if( event == EVENT_ABILITIES && in_play(player, card) && ! is_humiliated(player, card) ){
		if( in_play(affected_card_controller, affected_card) && is_land_creature(affected_card_controller, affected_card) ){
			event_result |= KEYWORD_TRAMPLE;
		}
	}

	return 0;
}

int card_expedite(int player, int card, event_t event){
	/* Expedite	|R	0x200e5fd --> also code for Accelerate
	 * Instant
	 * Target creature gains haste until end of turn.
	 * Draw a card. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_fall_of_the_titans(int player, int card, event_t event){
	/* Fall of the Titans	|X|X|R	0x200e602
	 * Instant
	 * Surge |X|R
	 * ~ deals X damage to each of up to two target creatures and/or players. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, "TARGET_CREATURE", 2, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = played_for_free(player, card) || is_token(player, card) ?
								0 : charge_mana_for_double_x(player, COLOR_ANY) / 2;
		}
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, "TARGET_CREATURE", 2, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			damage_creature(instance->targets[i].player, instance->targets[i].card, instance->info_slot, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_goblin_dark_dwellers(int player, int card, event_t event){
	/* Goblin Dark-Dwellers	|3|R|R	0x200e607
	 * Creature - Goblin 4/4
	 * Menace
	 * When ~ enters the battlefield, you may cast target instant or sorcery card with converted mana cost 3 or less
	 * from your graveyard without paying its mana cost. If that card would be put into your graveyard this turn, exile it instead. */

	menace(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_SPELL, "Select an instant or sorcery card with converted mana cost 3 or less.");
		test.cmc = 4;
		test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		int ai_mode = RESOLVE_TRIGGER_OPTIONAL;
		if( player == AI ){
			ai_mode = new_special_count_grave(player, &test) ? RESOLVE_TRIGGER_MANDATORY : 0;
		}
		if( comes_into_play_mode(player, card, event, ai_mode) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_VALUE, -1, &test);
			if( selected != -1 ){
				play_card_in_grave_for_free_and_exile_it(player, player, selected);
			}
		}
	}

	return 0;
}

int card_goblin_freerunner(int player, int card, event_t event){
	/* Goblin Freerunner	|3|R	0x200e60c
	 * Creature - Goblin Warrior Ally 3/2
	 * Surge |1|R
	 * Menace */

	modify_casting_cost_for_surge(player, card, event, MANACOST_XR(1, 1));

	menace(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = surge(player, card, event, MANACOST_XR(1, 1));
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
	}

	return 0;
}

int toll_collector_card = -1;
static const char* equipment_already_attached_to_me(int who_chooses, int player, int card)
{
	if( who_chooses == HUMAN ){
		return NULL;
	}
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player == -1 ||
		!(instance->damage_target_player == who_chooses && instance->damage_target_card == toll_collector_card))
	{
		return NULL;
	}
	return "already attached to Kazuul's Toll Collector";
}

int card_kazuuls_toll_collector(int player, int card, event_t event){
	/* Kazuul's Toll Collector	|2|R	0x200e611
	 * Creature - Ogre Warrior 3/2
	 * |0: Attach target Equipment you control to ~. Activate this ability only any time you could cast a sorcery. */
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_EQUIPMENT;
	td.allowed_controller = td.preferred_controller = player;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int)equipment_already_attached_to_me; //AI helping function
	toll_collector_card = card;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		equip_target_creature(instance->parent_controller, instance->parent_card,
									instance->targets[0].player, instance->targets[0].card);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_SORCERY_BE_PLAYED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target Equipment you control.");
}

int card_oath_of_chandra(int player, int card, event_t event){
	/* Oath of Chandra	|1|R	0x200e616
	 * Legendary Enchantment
	 * When ~ enters the battlefield, it deals 3 damage to target creature an opponent controls.
	 * At the beginning of each end step, if a planeswalker entered the battlefield under your control this turn,
	 * ~ deals 2 damage to each opponent. */

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		int ai_mode = RESOLVE_TRIGGER_OPTIONAL;
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = td.preferred_controller = 1-player;
		td.allow_cancel = 0;
		if( player == AI ){
			ai_mode = can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0;
		}
		if( comes_into_play_mode(player, card, event, ai_mode) ){
			if( new_pick_target(&td, "Select target creature an opponent controls.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance( player, card);
				damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
				instance->number_of_targets = 0;
			}
		}
	}

	if(trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player && ! is_humiliated(player, card) ){
		int trigs = get_trap_condition(player, TRAP_PLANESWALKERS_PLAYED) ? 1 : 0;
		if( eot_trigger_mode(player, card, event, ANYBODY, (trigs ? RESOLVE_TRIGGER_MANDATORY : 0)) ){
			damage_player(1-player, 2, player, card);
		}
	}
	return global_enchantment(player, card, event);
}

int card_press_into_service(int player, int card, event_t event){
	/* Press into Service	|4|R	0x200e61b
	 * Sorcery
	 * Support 2.
	 * Gain control of target creature until end of turn. Untap that creature. It gains haste until end of turn. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			support(player, card, 2);
			effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_pyromancers_assault(int player, int card, event_t event){
	/* Pyromancer's Assault	|3|R	0x200e620
	 * Enchantment
	 * Whenever you cast your second spell each turn, ~ deals 2 damage to target creature or player. */

	if( get_specific_storm_count(player) == 2 && new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, NULL) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_reckless_bushwacker(int player, int card, event_t event){
	/* Reckless Bushwhacker	|2|R	0x200e625
	 * Creature - Goblin Warrior Ally 2/1
	 * Surge |1|R
	 * Haste
	 * When ~ enters the battlefield, if its surge cost was paid, other creatures you control get +1/+0 and gain haste until end of turn. */

	modify_casting_cost_for_surge(player, card, event, MANACOST_XR(1, 1));

	haste(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = surge(player, card, event, MANACOST_XR(1, 1));
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		get_card_instance(player, card)->info_slot |= result;
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( (instance->info_slot & 2)  && comes_into_play(player, card, event) ){
			pump_creatures_until_eot(player, card, player, 0, 1, 0, 0, SP_KEYWORD_HASTE, NULL);
		}
	}

	return 0;
}

int card_sparkmages_gambit(int player, int card, event_t event){
	/* Sparkmage's Gambit	|1|R	0x200e62a
	 * Sorcery
	 * ~ deals 1 damage to each of up to two target creatures. Those creatures can't block this turn. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card,
										0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
				damage_creature(instance->targets[i].player, instance->targets[i].card, 1, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_tears_of_valakut(int player, int card, event_t event){
	/* Tears of Valakut	|1|R	0x200e62f
	 * Instant
	 * ~ can't be countered by spells or abilities.
	 * ~ deals 5 damage to target creature with flying. */

	cannot_be_countered(player, card, event);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			damage_target0(player, card, 5);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature with flying.", 1, NULL);
}

int card_tyrant_of_valakut(int player, int card, event_t event){
	/* Tyrant of Valakut	|5|R|R	0x200e634
	 * Creature - Dragon 5/4
	 * Surge |3|R|R
	 * Flying
	 * When ~ enters the battlefield, if its surge cost was paid, it deals 3 damage to target creature or player. */

	modify_casting_cost_for_surge(player, card, event, MANACOST_XR(3, 2));

	haste(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = surge(player, card, event, MANACOST_XR(3, 2));
		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		get_card_instance(player, card)->info_slot |= result;
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card);
		if( (instance->info_slot & 2)  && can_target(&td) && comes_into_play(player, card, event) ){
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				damage_target0(player, card, 3);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

int card_zadas_commando(int player, int card, event_t event){
	/* Zada's Commando	|1|R	0x200e639
	 * Creature - Goblin Archer Ally 2/1
	 * First strike
	 * Cohort - |T, Tap an untapped Ally you control: ~ deals 1 damage to target opponent. */

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_cohort(player, card) ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST0, 0, &td, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->number_of_targets = 0;
		int result = pick_target_for_cohort(player, card);
		if( result == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST0, 0, &td, NULL);
		if( spell_fizzled != 1 ){
			tap_card(player, result);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->parent_controller, instance->parent_card, 1, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

/*** Green - Devoid ***/

int card_birthing_hulk(int player, int card, event_t event){
	/* Birthing Hulk	|6|G	0x200e63e
	 * Creature - Eldrazi Drone 5/4
	 * Devoid
	 * When ~ enters the battlefield, put two 1/1 colorless Eldrazi Scion creature tokens onto the battlefield.
	 * They have "Sacrifice this creature: Add |C to your mana pool."
	 * |1|C: Regenerate ~. */

	if( comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SCION, 2);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST_X(2), 0, NULL, NULL) )
		{
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST_X(1), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		if( can_regenerate(instance->parent_controller, instance->parent_card) ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_ruin_on_their_wake(int player, int card, event_t event){
	/* Ruin in Their Wake	|1|G	0x200e643
	 * Sorcery
	 * Devoid
	 * Search your library for a basic land card and reveal it. You may put that card onto the battlefield tapped if you control a
	 * land named Wastes. Otherwise, put that card into your hand. Then shuffle your library. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_LAND, "Select a basic land card.");
		test.subtype = SUBTYPE_BASIC;
		int result = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &test);
		if( result != 1 ){
			if( check_battlefield_for_name(player, TYPE_LAND, CARD_ID_WASTES) ){
				int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM, DLG_NO_CANCEL,
									"Put land into play tapped", 1, 10,
									"Decline", 1, 1);
				if( choice == 1 ){
					add_state(player, result, STATE_TAPPED);
					put_into_play(player, result);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_scion_summoner(int player, int card, event_t event){
	/* Scion Summoner	|2|G	0x200e648
	 * Creature - Eldrazi Drone 2/2
	 * Devoid
	 * When ~ enters the battlefield, put a 1/1 colorless Eldrazi Scion creature token onto the battlefield.
	 * It has "Sacrifice this creature: Add |C to your mana pool." */

	if( comes_into_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);
	}

	return 0;
}

int card_stalking_drone(int player, int card, event_t event){
	/* Stalking Drone	|1|G	0x200e64d
	 * Creature - Eldrazi Drone 2/2
	 * Devoid
	 * |C: ~ gets +1/+2 until end of turn. Activate this ability only once each turn. */

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) ){
			return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST_X(1), 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_until_eot_merge_previous(instance->parent_controller, instance->parent_card,
										instance->parent_controller, instance->parent_card, 1, 2);
	}

	if (event == EVENT_POW_BOOST &&
		generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_ONCE_PER_TURN, MANACOST_X(1), 0, NULL, NULL) )
	{
		if( has_colorless_only_mana(player, 1) ){
			if( has_mana(player, COLOR_ANY, 2) ){
				event_result += 1;
			}
		}
	}

	if (event == EVENT_TOU_BOOST &&
		generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_ONCE_PER_TURN, MANACOST_X(1), 0, NULL, NULL) )
	{
		if( has_colorless_only_mana(player, 1) ){
			if( has_mana(player, COLOR_ANY, 2) ){
				event_result+=2;
			}
		}
	}

	return 0;
}

int card_vile_redeemer(int player, int card, event_t event){
	/* Vile Redeemer	|2|G	0x200e652
	 * Creature - Eldrazi 3/3
	 * Devoid
	 * Flash
	 * When you cast ~, you may pay |C. If you do, put a 1/1 colorless Eldrazi Scion creature token onto the battlefield for
	 * each nontoken creature that died under your control this turn. Those tokens have "Sacrifice this creature: Add |C to your mana pool." */

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			if( player_has_generic_and_colorless_available(player, 1, 1) ){
				if( charge_colorless_mana_only(player, 1) ){
					generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SCION, get_dead_count(player, TYPE_CREATURE));
				}
			}
		}
	}

	return flash(player, card, event);
}

int card_world_breaker(int player, int card, event_t event){
	/* World Breaker	|6|G	0x200e657
	 * Creature - Eldrazi 5/7
	 * Devoid
	 * When you cast ~, exile target artifact, enchantment, or land.
	 * Reach
	 * |2|C, Sacrifice a land: Return ~ from your graveyard to your hand. */

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT | TYPE_LAND );
			td.allow_cancel = 0;
			if( can_target(&td) && new_pick_target(&td, "Select target artifact, enchantment, or land.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance(player, card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
				instance->number_of_targets = 0;
			}
		}
	}

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( player_has_generic_and_colorless_available(player, 2, 1) ){
			return GA_RETURN_TO_HAND;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_generic_mana_with_colorless_only(player, 2, 1) ){
			return GAPAID_REMOVE;
		}
	}

	return 0;
}


/*** Green ***/

int card_baloth_pup(int player, int card, event_t event){
	/* Baloth Pup	|1|G	0x200e65c
	 * Creature - Beast 3/1
	 * ~ has trample as long as it has a +1/+1 counter on it. */

	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( count_1_1_counters(player, card) ){
			event_result |= KEYWORD_TRAMPLE;
		}
	}

	return 0;
}

int card_bonds_of_mortality(int player, int card, event_t event){
	/* Bonds of Mortality	|1|G	0x200e661
	 * Enchantment
	 * When ~ enters the battlefield, draw a card.
	 * |G: Creatures your opponents control lose hexproof and indestructible until end of turn. */

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST_G(1), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		negate_ability_and_special_ability_until_eot(instance->parent_controller, instance->parent_card, 1-instance->parent_controller, -1,
													0, SP_KEYWORD_INDESTRUCTIBLE | SP_KEYWORD_HEXPROOF);
	}

	return global_enchantment(player, card, event);
}

/* Canopy Gorger	|4|G|G --> vanilla	0x401000
 * Creature - Wurm 6/5 */

int card_elemental_uprising(int player, int card, event_t event){
	/* Elemental Uprising	|1|G	0x200e666
	 * Instant
	 * Target land you control becomes a 4/4 Elemental creature with haste until end of turn. It's still a land.
	 * It must be blocked this turn if able. */
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int legacy = turn_into_creature(player, card, instance->targets[0].player, instance->targets[0].card, 1, 4, 4);
			get_card_instance( player, legacy )->targets[8].card = SP_KEYWORD_HASTE | SP_KEYWORD_MUST_BE_BLOCKED;
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target land you control.", 1, NULL);
}

int card_embodiment_of_insight(int player, int card, event_t event){
	/* Embodiment of Insight	|4|G	0x200e66b
	 * Creature - Elemental 4/4
	 * Vigilance
	 * Land creatures you control have vigilance.
	 * Landfall - Whenever a land enters the battlefield under your control, you may have target land you control become a 3/3 Elemental
	 * creature with haste until end of turn. It's still a land. */

	if( landfall_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = td.preferred_controller = player;

		card_instance_t *instance = get_card_instance( player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && new_pick_target(&td, "Select target land you control.", 0, GS_LITERAL_PROMPT) ){
			int legacy = turn_into_creature(player, card, instance->targets[0].player, instance->targets[0].card, 1, 3, 3);
			card_instance_t *leg = get_card_instance( player, legacy );
			leg->targets[8].card = SP_KEYWORD_HASTE;
		}
	}

	if( event == EVENT_ABILITIES && in_play(player, card) && ! is_humiliated(player, card) ){
		if( in_play(affected_card_controller, affected_card) && is_land_creature(affected_card_controller, affected_card) ){
			vigilance(affected_card_controller, affected_card, event);
		}
	}

	return 0;
}

static void count_for_gladehart_cavalry_ability(int player, int card, event_t event){
	if( event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( is_what(player, card, TYPE_EFFECT) ){
			if( affect_me(instance->damage_target_player, instance->damage_target_card) && instance->kill_code > 0 ){
				instance->damage_target_player = instance->damage_target_card = -1;
				remove_status(player, card, STATUS_INVISIBLE_FX);
			}
		}
		if( ! check_status(player, card, STATUS_INVISIBLE_FX) && count_1_1_counters(affected_card_controller, affected_card) ){
			count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);
		}
	}
}

static int gladehart_cavalry_effect(int player, int card, event_t event){

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( get_id(instance->damage_target_player, instance->damage_target_card) != CARD_ID_GLADEHART_CAVALRY ){
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	count_for_gladehart_cavalry_ability(player, card, event);

	if( ! check_status(player, card, STATUS_INVISIBLE_FX) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t *inst = get_card_instance(player, card);
		gain_life(player, inst->targets[11].card*2);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_gladehart_cavalry(int player, int card, event_t event){
	/* Gladehart Cavalry	|5|G|G	0x200e670
	 * Creature - Elf Knight 6/6
	 * When ~ enters the battlefield, support 6.
	 * Whenever a creature you control with a +1/+1 counter on it dies, you gain 2 life. */

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		int found = 0;
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_EFFECT) ){
				card_instance_t *inst = get_card_instance(player, c);
				if( inst->info_slot == (int)gladehart_cavalry_effect ){
					found = 1;
					break;
				}
			}
		}
		if( ! found ){
			int legacy = create_targetted_legacy_effect(player, card, &gladehart_cavalry_effect, player, card);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	count_for_gladehart_cavalry_ability(player, card, event);

	if( get_card_instance(player, card)->kill_code <= 0 && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t *inst = get_card_instance(player, card);
		gain_life(player, inst->targets[11].card*2);
		inst->targets[11].card = 0;
	}

	if( comes_into_play(player, card, event) ){
		support(player, card, 6);
	}

	return 0;
}

int card_harvester_troll(int player, int card, event_t event){
	/* Harvester Troll	|3|G	0x200e675
	 * Creature - Troll 2/3
	 * When ~ enters the battlefield, you may sacrifice a creature or land. If you do, put two +1/+1 counters on ~. */

	if( event == EVENT_RESOLVE_SPELL ){
		add_state(player, card, STATE_OUBLIETTED);

		char msg[100] = "Select a creature or land to sacrifice.";
		test_definition_t test;
		new_default_test_definition(&test, TYPE_LAND | TYPE_CREATURE, msg);

		if( new_can_sacrifice(player, card, player, &test) && new_sacrifice(player, card, player, 0, &test) ){
			add_1_1_counters(player, card, 2);
		}

		remove_state(player, card, STATE_OUBLIETTED);
	}

	return 0;
}

int card_lead_by_example(int player, int card, event_t event){
	/* Lead by Example	|1|G	0x200e67a
	 * Instant
	 * Support 2. */

	if( event == EVENT_RESOLVE_SPELL ){
		support(player, card, 6);
		kill_card(player, card, KILL_DESTROY);
	}
	return basic_spell(player, card, event);
}

int card_loam_larva(int player, int card, event_t event){
	/* Loam Larva	|1|G	0x200e67f
	 * Creature - Insect 1/3
	 * When ~ enters the battlefield, you may search your library for a basic land card, reveal it,
	 then shuffle your library and put that card on top of it. */

	if( comes_into_play(player, card, event) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_LAND, "Select a basic land card.");
		test.subtype = SUBTYPE_BASIC;
		new_global_tutor(player, player, TUTOR_DECK, TUTOR_DECK, 0, AI_FIRST_FOUND, &test);
	}

	return 0;
}

int card_natural_state(int player, int card, event_t event){
	/* Natural State	|G	0x200e684
	 * Instant
	 * Destroy target artifact or enchantment with converted mana cost 3 or less. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);
	td.special = TARGET_SPECIAL_CMC_LESSER_OR_EQUAL;
	td.extra = 3;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						"Select target artifact or enchantment with converted mana cost 3 or less.", 1, NULL);
}

/* Netcaster Spider	|2|G	=>m15.c:card_netcaster_spider()
 * Creature - Spider 2/3
 * Reach
 * Whenever ~ blocks a creature with flying, ~ gets +2/+0 until end of turn. */

int card_nissa_voice_of_zendikar(int player, int card, event_t event){
	/* Nissa, Voice of Zendikar	|1|G|G	0x200e689
	 * Planeswalker - Nissa (3)
	 * +1: Put a 0/1 |Sgreen Plant creature token onto the battlefield.
	 * -2: Put a +1/+1 counter on each creature you control.
	 * -7: You gain X life and draw X cards, where X is the number of lands you control. */

	card_instance_t *instance = get_card_instance( player, card);

	if( IS_GAA_EVENT(event) ){
		enum
		{
			CHOICE_PLANT = 1,
			CHOICE_PUMP_CRITS,
			CHOICE_GAIN_AND_DRAW
		};

		if( event == EVENT_ACTIVATE ){
			instance->info_slot = 0;
			int priority[3] = {10, 1, 1};
			if( count_subtype(player, TYPE_CREATURE, -1) >= count_subtype(1-player, TYPE_CREATURE, -1) ){
				priority[0] = 1;
				priority[1] = 10;
			}
			priority[3] = count_subtype(player, TYPE_LAND, -1)*3;
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
								"Generate a Plant",	1,	priority[0],	+1,
								"Pump your dudes",	1,	priority[1],	-2,
								"Gain life & draw",	1,	priority[2],	-7);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int pl = instance->parent_controller;
			int ca = instance->parent_card;
			if( instance->info_slot == CHOICE_PLANT ){
				token_generation_t token;
				default_token_definition(pl, ca, CARD_ID_PLANT, &token);
				token.pow = 0;
				token.tou = 1;
				token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
				generate_token(&token);
			}
			if( instance->info_slot == CHOICE_PUMP_CRITS ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				new_manipulate_all(pl, ca, pl, &this_test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
			}
			if( instance->info_slot == CHOICE_GAIN_AND_DRAW ){
				int amount = count_subtype(player, TYPE_LAND, -1);
				draw_cards(pl, amount);
				gain_life(pl, amount);
			}
		}
	}

	return planeswalker(player, card, event, 3);
}


int card_nissas_judgement(int player, int card, event_t event){
	/* Nissa's Judgment	|4|G	0x200e68e
	 * Sorcery
	 * Support 2.
	 * Choose up to one target creature an opponent controls. Each creature you control with a +1/+1 counter on it deals damage equal
	 * to its power to that creature. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int t_card = -1;
		if( instance->number_of_targets == 1 && valid_target(&td) ){
			t_card = instance->targets[0].card;
		}

		support(player, card, 2);

		if( t_card > -1 ){
			int c;
			for(c=active_cards_count[player]-1; c>-1; c--){
				if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) && count_1_1_counters(player, c) ){
					damage_creature(1-player, t_card, get_power(player, c), player, c);
				}
			}
		}

		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_OPTIONAL_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature an opponent controls.", 1, NULL);
}

static int is_creature_land_or_pwalker(int iid, int unused, int player, int card){

	if( is_what(-1, iid, TYPE_CREATURE | TYPE_LAND) || is_planeswalker(-1, iid) ){
		return 1;
	}

	return 0;
}

int card_oath_of_nissa(int player, int card, event_t event){
	/* Oath of Nissa	|G	0x200e693
	 * Legendary Enchantment
	 * When ~ enters the battlefield, look at the top three cards of your library. You may reveal a creature, land, or planeswalker card
	 * from among them and put it into your hand. Put the rest on the bottom of your library in any order.
	 * You may spend mana as though it were mana of any color to cast planeswalker spells. */

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select a a creature, land, or planeswalker card.");
		test.special_selection_function = &is_creature_land_or_pwalker;
		test.create_minideck = MIN(3, count_deck(player));
		test.no_shuffle = 1;
		new_global_tutor(player, player, TUTOR_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && is_planeswalker(player, i) ){
				int amount = get_updated_casting_cost(player, i, -1, event, -1);
				amount+=cards_ptr[get_id(player, i)]->req_black;
				amount+=cards_ptr[get_id(player, i)]->req_blue;
				amount+=cards_ptr[get_id(player, i)]->req_green;
				amount+=cards_ptr[get_id(player, i)]->req_red;
				amount+=cards_ptr[get_id(player, i)]->req_white;
				if( has_mana(player, COLOR_COLORLESS, amount) ){
					if( is_what(player, i, TYPE_INSTANT) || can_sorcery_be_played(player, event) ){
						return 1;
					}
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int playable[3][hand_count[player]];
		int pc = 0;
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && is_planeswalker(player, i) ){
				int amount = get_updated_casting_cost(player, i, -1, event, -1);
				amount+=cards_ptr[get_id(player, i)]->req_black;
				amount+=cards_ptr[get_id(player, i)]->req_blue;
				amount+=cards_ptr[get_id(player, i)]->req_green;
				amount+=cards_ptr[get_id(player, i)]->req_red;
				amount+=cards_ptr[get_id(player, i)]->req_white;
				if( has_mana(player, COLOR_COLORLESS, amount) ){
					if( is_what(player, i, TYPE_INSTANT) || can_sorcery_be_played(player, event) ){
						playable[0][pc] = get_card_instance(player, i)->internal_card_id;
						playable[1][pc] = i;
						playable[2][pc] = amount;
						pc++;
					}
				}
			}
		}

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a planeswalker to play.");
		this_test.subtype = SUBTYPE_PLANESWALKER;

		int selected = select_card_from_zone(player, player, playable[0], pc, 0, AI_MAX_VALUE, -1, &this_test);
		if( selected != -1 ){
			if( charge_mana(player, COLOR_COLORLESS, playable[2][selected])  ){
				play_card_in_hand_for_free(player, playable[1][selected]);
				cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
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

	return global_enchantment(player, card, event);
}

int card_pulse_of_murasa(int player, int card, event_t event){
	/* Pulse of Murasa	|2|G	0x200e698
	 * Instant
	 * Return target creature or land card from a graveyard to its owner's hand. You gain 6 life. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_LAND, "Select a creature or land card.");

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance( player, card );
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 0);
		if( selected != -1 ){
			from_grave_to_hand(instance->targets[0].player, selected, TUTOR_HAND);
			gain_life(player, 6);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER_BOTH_GRAVES, NULL, NULL, 1, &this_test);
}

/* Saddleback Lagac	|3|G --> Expedition Raptor 0x200e472
 * Creature - Lizard 3/1
 * When ~ enters the battlefield, support 2. */

int card_seed_guardian(int player, int card, event_t event){
	/* Seed Guardian	|2|G|G	0x200e69d
	 * Creature - Elemental 3/4
	 * Reach
	 * When ~ dies, put an X/X |Sgreen Elemental creature token onto the battlefield,
	 where X is the number of creature cards in your graveyard. */

 	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.pow = token.tou = count_graveyard_by_type(player, TYPE_CREATURE);
		token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
		generate_token(&token);
	}

	return 0;
}

int card_sylvan_advocate(int player, int card, event_t event){
	/* Sylvan Advocate	|1|G	0x200e6a2
	 * Creature - Elf Druid Ally 2/3
	 * Vigilance
	 * As long as you control six or more lands, ~ and land creatures you control get +2/+2. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player ){
			if( in_play(affected_card_controller, affected_card) && count_subtype(player, TYPE_LAND, -1) &&
				(is_land_creature(affected_card_controller, affected_card) || affect_me(affected_card_controller, affected_card)) )
			{
				event_result += 2;
			}
		}
	}

	return 0;
}

/* Tajuru Pathwarden	|4|G --> Serra Angel	0x200c122
 * Creature - Elf Warrior Ally 5/4
 * Vigilance, trample */

int card_vines_of_the_recluse(int player, int card, event_t event){
 /* Vines of the Recluse	|G	0x200e6a7
 * Instant
 * Target creature gets +1/+2 and gains reach until end of turn. Untap it. */

	if (event == EVENT_CHECK_PUMP ){
		if (!has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id)){
			return 0;
		}
		pumpable_power[player]++;
		pumpable_toughness[player]+=2;
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 2, KEYWORD_REACH, 0);
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}


int card_zendikar_resurgent(int player, int card, event_t event){
	/* Zendikar Resurgent	|5|G|G	0x200e6ac
	 * Enchantment
	 * Whenever you tap a land for mana, add one mana to your mana pool of any type that land produced.
	 * Whenever you cast a creature spell, draw a card. */

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		if( new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test) ){
			draw_cards(player, 1);
		}
	}

	return card_mana_reflection(player, card, event);
}

/*** Multi - Devoid ***/

int card_flayer_drone(int player, int card, event_t event){
	/* Flayer Drone	|1|B|R	0x200e6b1
	 * Creature - Eldrazi Drone 3/1
	 * Devoid
	 * First strike
	 * Whenever another colorless creature enters the battlefield under your control, target opponent loses 1 life. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.color = COLOR_TEST_COLORLESS;

		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		int ai_mode = player == HUMAN ? RESOLVE_TRIGGER_OPTIONAL : 0;
		if( would_validate_arbitrary_target(&td, 1-player, -1) ){
			ai_mode = RESOLVE_TRIGGER_MANDATORY;
		}

		if( new_specific_cip(player, card, event, player, ai_mode, &test) ){
			if( would_validate_arbitrary_target(&td, 1-player, -1) ){
				lose_life(1-player, 1);
			}
		}
	}

	return 0;
}

int card_mindmelter(int player, int card, event_t event){
	/* Mindmelter	|1|U|B	0x200e6b6
	 * Creature - Eldrazi Drone 2/2
	 * Devoid
	 * ~ can't be blocked.
	 * |3|C: Target opponent exiles a card from his or her hand. Activate this ability only any time you could cast a sorcery. */

	unblockable(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT | GAA_CAN_SORCERY_BE_PLAYED, MANACOST_X(4), 0,
									&td, NULL) )
		{
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( ! charge_colorless_mana_only(player, 1) ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT | GAA_CAN_SORCERY_BE_PLAYED, MANACOST_X(3), 0,
										&td, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ANY, "Select a card to exile.");
			new_global_tutor(1-player, 1-player, TUTOR_FROM_HAND, TUTOR_RFG, 1, AI_MIN_VALUE, &test);
		}
	}

	return 0;
}

int card_void_grafter(int player, int card, event_t event){
	/* Void Grafter	|1|G|U	0x200e6bb
	 * Creature - Eldrazi Drone 2/4
	 * Devoid
	 * Flash
	 * When ~ enters the battlefield, another target creature you control gains hexproof until end of turn. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME;

		int ai_mode = player == HUMAN ? RESOLVE_TRIGGER_OPTIONAL : 0;
		if( can_target(&td) ){
			ai_mode = RESOLVE_TRIGGER_MANDATORY;
		}

		if( comes_into_play_mode(player, card, event, ai_mode) ){
			if( new_pick_target(&td, "Select another target creature you control.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance( player, card);
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HEXPROOF);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

/*** Multi ***/

int card_ayli_eternal_pilgrim(int player, int card, event_t event){
	/* Ayli, Eternal Pilgrim	|W|B	0x200e6c0
	 * Legendary Creature - Kor Cleric 2/3
	 * Deathtouch
	 * |1, Sacrifice another creature: You gain life equal to the sacrificed creature's toughness.
	 * |1|W|B, Sacrifice another creature: Exile target nonland permanent.
	 *		 Activate this ability only if you have at least 10 life more than your starting life total. */

	check_legend_rule(player, card, event);

	deathtouch(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	test_definition_t test;
	new_default_test_definition(&test, TYPE_LAND, "Select another creature to sacrifice.");
	test.type_flag = DOESNT_MATCH;
	test.not_me = 1;

	enum {
			CHOICE_SAC_AND_GAIN = 1,
			CHOICE_SAC_AND_EXILE,
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return new_can_sacrifice_as_cost(player, card, &test) ? 1 : 0;
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XBW(1, 1, 1), 0, &td, NULL) ){
			if( life[player] > get_starting_life_total(player)+10 ){
				return new_can_sacrifice_as_cost(player, card, &test) ? 1 : 0;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->targets[1].player = instance->info_slot = instance->number_of_targets = 0;
		int abilities[2] = {0, 0};
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST0, 0, NULL, NULL) ){
			abilities[0] = new_can_sacrifice_as_cost(player, card, &test) ? 1 : 0;
		}
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XBW(1, 1, 1), 0, &td, NULL) ){
			if( life[player] > get_starting_life_total(player)+10 ){
				abilities[1] = new_can_sacrifice_as_cost(player, card, &test) ? 1 : 0;
			}
		}
		int priorities[2] = {(life[player] < 6 ? 20 : 5), 15};
		int choice = DIALOG(player, card, event, DLG_RANDOM,
							"Sac & gain life", abilities[0], priorities[0],
							"Sac & exile nonland permanent", abilities[1], priorities[1]);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_SAC_AND_GAIN ){
			if( ! charge_mana_for_activated_ability(player, card, MANACOST0)){
				return 0;
			}
		}
		if( choice == CHOICE_SAC_AND_EXILE ){
			if( ! charge_mana_for_activated_ability(player, card, MANACOST_XBW(1, 1, 1))){
				return 0;
			}
		}
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (!sac){
			cancel = 1;
			return 0;
		}
		if( choice == CHOICE_SAC_AND_GAIN ){
			instance->targets[1].player = get_toughness(BYTE2(sac), BYTE3(sac));
		}
		if( choice == CHOICE_SAC_AND_EXILE ){
			new_pick_target(&td, "Select target nonland permanent.", 0, 1 | GS_LITERAL_PROMPT);
		}
		if( spell_fizzled != 1 ){
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
		else{
			state_untargettable(BYTE2(sac), BYTE3(sac), 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == CHOICE_SAC_AND_GAIN ){
			gain_life(instance->parent_controller, instance->targets[1].player);
		}
		if( instance->info_slot == CHOICE_SAC_AND_EXILE && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_baloth_null(int player, int card, event_t event){
	/* Baloth Null	|4|B|G	0x200e6c5
	 * Creature - Zombie Beast 4/5
	 * When ~ enters the battlefield, return up to two target creature cards from your graveyard to your hand. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){

		int ai_mode = player == HUMAN ? RESOLVE_TRIGGER_OPTIONAL : 0;
		if( count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player) ){
			ai_mode = RESOLVE_TRIGGER_MANDATORY;
		}

		if( comes_into_play_mode(player, card, event, ai_mode) && count_graveyard_by_type(player, TYPE_CREATURE) &&
			! graveyard_has_shroud(player) )
		{
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");
			test.qty = MIN(2, count_graveyard_by_type(player, TYPE_CREATURE));
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		}
	}

	return 0;
}

int card_cliffhaven_vampire(int player, int card, event_t event){
	/* Cliffhaven Vampire	|2|W|B	0x200e6ca
	 * Creature - Vampire Warrior Ally 2/4
	 * Flying
	 * Whenever you gain life, each opponent loses 1 life. */

	if( ! is_humiliated(player, card) && trigger_gain_life(player, card, event, player, RESOLVE_TRIGGER_MANDATORY) ){
		lose_life(1-player, 1);
	}
	return 0;
}


int card_jorage_auxiliary(int player, int card, event_t event){
	/* Joraga Auxiliary	|1|G|W	0x200e6cf
	 * Creature - Elf Soldier Ally 2/3
	 * |4|G|W: Support 2. */

	if (event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		support(instance->parent_controller, instance->parent_card, 2);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XGW(4, 1, 1), 0, NULL, NULL);
}

int card_jori_en_ruin_driver(int player, int card, event_t event){
	/* Jori En, Ruin Diver	|1|U|R	0x200e6d4
	 * Legendary Creature - Merfolk Wizard 2/3
	 * Whenever you cast your second spell each turn, draw a card. */

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller ){
		if( get_specific_storm_count(player) == 2 &&
			new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, NULL) )
		{
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_mina_and_denn_wildborn(int player, int card, event_t event){
	/* Mina and Denn, Wildborn	|2|R|G	0x200e6d9
	 * Legendary Creature - Elf Ally 4/4
	 * You may play an additional land on each of your turns.
	 * |R|G, Return a land you control to its owner's hand: Target creature gains trample until end of turn. */

	check_legend_rule(player, card, event);

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_LAND);
		td2.preferred_controller = td2.allowed_controller = player;
		td2.illegal_abilities = 0;

		card_instance_t *inst = get_card_instance(player, card);

		if( event == EVENT_CAN_ACTIVATE ){
			if( can_target(&td) ){
				return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_GR(1, 1), 0, &td, NULL);
			}
		}

		if( event == EVENT_ACTIVATE ){
			inst->number_of_targets = 0;
			if( new_pick_target(&td, "Select a land your control.", 0, 1 | GS_LITERAL_PROMPT) ){
				int land = inst->targets[0].card;
				inst->number_of_targets = 0;
				generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_GR(1, 1), 0, &td, NULL);
				if( spell_fizzled != 1 ){
					bounce_permanent(player, land);
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				pump_ability_until_eot(inst->parent_controller, inst->parent_card,
										inst->targets[0].player, inst->targets[0].card, 0, 0, KEYWORD_TRAMPLE, 0);
			}
		}
	}

	return card_exploration(player, card, event);
}

int card_reflector_mage(int player, int card, event_t event){
	/* Reflector Mage	|1|W|U	0x200e6de
	 * Creature - Human Wizard 2/3
	 * When ~ enters the battlefield, return target creature an opponent controls to its owner's hand.
	 * That creature's owner can't cast spells with the same name as that creature until your next turn. */


	if( in_play(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->targets[1].card > -1 && event == EVENT_MODIFY_COST_GLOBAL &&
			affected_card_controller == instance->targets[1].player &&
			get_id(affected_card_controller, affected_card) == instance->targets[1].card )
		{
			infinite_casting_cost();
		}
	}

	if (comes_into_play(player, card, event) ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.allowed_controller = td1.preferred_controller = 1-player;
		td1.allow_cancel = 0;
		if( can_target(&td1) && new_pick_target(&td1, "Select target creature an opponent controls.", 0, GS_LITERAL_PROMPT)){
			card_instance_t *instance = get_card_instance(player, card);
			instance->targets[1].player = get_owner(instance->targets[0].player, instance->targets[0].card);
			instance->targets[1].card = get_card_name(instance->targets[0].player, instance->targets[0].card);
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_relentless_hunter(int player, int card, event_t event){
	/* Relentless Hunter	|1|R|G	0x200e6e3
	 * Creature - Human Warrior 3/3
	 * |1|R|G: ~ gets +1/+1 and gains trample until end of turn. */
	return generic_shade(player, card, event, 0, MANACOST_XGR(1, 1, 1), 1, 1, KEYWORD_TRAMPLE, 0);
}

/* Stormchaser Mage	|U|R --> Monastery Swiftspear	0x200d0bd
 * Creature - Human Wizard 1/3
 * Flying, haste
 * Prowess */

int card_weapons_trainer(int player, int card, event_t event){
	/* Weapons Trainer	|R|W	0x200e6e8
	 * Creature - Human Soldier Ally 3/2
	 * Other creatures you control get +1/+0 as long as you control an Equipment. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER && ! affect_me(player, card) && affected_card_controller == player ){
			test_definition_t test;
			default_test_definition(&test, TYPE_PERMANENT);
			test.subtype = SUBTYPE_EQUIPMENT;
			if( check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test) ){
				event_result++;
			}
		}
	}

	return 0;
}

/*** Artifact ***/

/* Bone Saw	|0	=>conflux.c:card_bone_saw()
 * Artifact - Equipment
 * Equipped creature gets +1/+0.
 * Equip |1 */

int card_captains_claws(int player, int card, event_t event){
	/* Captain's Claws	|2	0x200e6ed
	 * Artifact - Equipment
	 * Equipped creature gets +1/+0.
	 * Whenever equipped creature attacks, put a 1/1 |Swhite Kor Ally creature token onto the battlefield tapped and attacking.
	 * Equip |1 */

	if( in_play(player, card) && is_equipping(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY,
									instance->damage_target_player, instance->damage_target_card) )
		{
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_KOR_ALLY, &token);
			token.pow = token.tou = 1;
			token.s_key_plus = SP_KEYWORD_VIGILANCE;
			token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			token.action = TOKEN_ACTION_ATTACKING;
			generate_token(&token);
		}
	}

	return vanilla_equipment(player, card, event, 1, 1, 0, 0, 0);
}

int card_chitinous_cloak(int player, int card, event_t event){
	/* Chitinous Cloak	|3	0x200e6f2
	 * Artifact - Equipment
	 * Equipped creature gets +2/+2 and has menace.
	 * Equip |3 */

	if( in_play(player, card) && is_equipping(player, card) ){
		fx_menace(player, card, event);
	}

	return vanilla_equipment(player, card, event, 3, 2, 2, 0, 0);
}

/* Hedron Crawler	|2 --> Boreal Druid	0x2002f6e
 * Artifact Creature - Construct 0/1
 * |T: Add |C to your mana pool. */

int card_seers_lanter(int player, int card, event_t event){
	/* Seer's Lantern	|3	0x200e6f7
	 * Artifact
	 * |T: Add |C to your mana pool.
	 * |2, |T: Scry 1. */

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	enum
	{
		CHOICE_MANA = 1,
		CHOICE_SCRY,
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL) ){
			return 1;
		}
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int can_scry = ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL);
		int can_get_mana = mana_producer(player, card, EVENT_CAN_ACTIVATE);
		int choice = DIALOG(player, card, event, DLG_RANDOM,
								"Get mana", can_get_mana, 5,
								"Scry 1", can_scry, (current_phase > PHASE_MAIN2 ? 10 : 1));
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_MANA ){
			return mana_producer(player, card, event);
		}
		if( choice == CHOICE_SCRY ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
				tap_card(player, card);
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == CHOICE_SCRY ){
			scry(player, 1);
		}
	}

	return 0;
}

int card_stoneforge_masterwork(int player, int card, event_t event){
	/* Stoneforge Masterwork	|1	0x200e6fc
	 * Artifact - Equipment
	 * Equipped creature gets +1/+1 for each other creature you control that shares a creature type with it.
	 * Equip |2 */

	if( in_play(player, card) && is_equipping(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(instance->damage_target_player, instance->damage_target_card)){
			int amount = 0;
			int c;
			for(c=0; c<active_cards_count[player]; c++){
				if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) ){
					if( shares_creature_subtype(player, c, instance->damage_target_player, instance->damage_target_card) ){
						amount++;
					}
				}
			}
			event_result += amount;
		}
	}

	return basic_equipment(player, card, event, 2);
}


/* Strider Harness	|3	=>scars_of_mirrodin.c:card_strider_harness()
 * Artifact - Equipment
 * Equipped creature gets +1/+1 and has haste.
 * Equip |1 */

/*** Land ***/

/* Cinder Barrens --> mana_producer_tapped	0x2002d57
 * Land
 * ~ enters the battlefield tapped.
 * |T: Add |B or |R to your mana pool. */

/* Corrupted Crossroads	--> Skipped (Restriction on mana usage impossible)
 * Land
 * |T: Add |C to your mana pool.
 * |T, Pay 1 life: Add one mana of any color to your mana pool. Spend this mana only to cast a spell with devoid. */

int card_crumbling_vestige(int player, int card, event_t event){
	/* Crumbling Vestige	0x200e701
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, add one mana of any color to your mana pool.
	 * |T: Add |C to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if( comes_into_play(player, card, event) ){
		do {
			cancel = 0;
			produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, 1);
		} while (cancel == 1);
	}

	return mana_producer(player, card, event);
}

int card_hissing_quagmire(int player, int card, event_t event){
	/* Hissing Quagmire	0x200e706
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T: Add |B or |G to your mana pool.
	 * |1|B|G: ~ becomes a 2/2 |Sblack and |Sgreen Elemental creature with deathtouch until end of turn. It's still a land. */

  comes_into_play_tapped(player, card, event);

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_ANIMATE
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1, DLG_NO_DISPLAY_FOR_AI,
						"Produce mana",
							mana_producer(player, card, EVENT_CAN_ACTIVATE),
							paying_mana(),
						"Become Elemental",
							!paying_mana(),
							!(get_card_instance(player, card)->token_status & STATUS_LEGACY_TYPECHANGE),
						DLG_MANA(MANACOST_XBG(1,1,1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_ANIMATE)
		{
		  if (event == EVENT_RESOLVE_ACTIVATION)
			animate_self(player, card, 2,2, 0,SP_KEYWORD_DEATHTOUCH, COLOR_TEST_GREEN | COLOR_TEST_BLACK, 0);
		  return 0;
		}
	}

  has_subtype_if_animated_self(player, card, event, SUBTYPE_ELEMENTAL);

  return mana_producer(player, card, event);
}

int card_holdout_settlement(int player, int card, event_t event){
	/* Holdout Settlement	0x200e70b
	 * Land
	 * |T: Add |C to your mana pool.
	 * |T, Tap an untapped creature you control: Add one mana of any color to your mana pool. */

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = td.preferred_controller = player;
		td.illegal_state = STATE_TAPPED;
		td.illegal_abilities = 0;
		if( can_target(&td) ){
			declare_mana_available(player, COLOR_ANY, 1);
		}
		else{
			declare_mana_available(player, COLOR_COLORLESS, 1);
		}
	}

	if( event == EVENT_CAN_ACTIVATE  ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = td.preferred_controller = player;
		td.illegal_state = STATE_TAPPED;
		td.illegal_abilities = 0;

		card_instance_t *instance = get_card_instance(player, card);

		produce_mana_tapped_all_one_color(player, card, instance->mana_color, 1);
		if (chosen_colors & ~COLOR_TEST_COLORLESS ){
			if( new_pick_target(&td, "Select an untapped creature you control.", 0, GS_LITERAL_PROMPT) ){
				instance->number_of_targets = 0;
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				remove_state(player, card, STATE_TAPPED);
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}

/* Meandering River --> mana_producer_tapped	0x2002d57
 * Land
 * ~ enters the battlefield tapped.
 * |T: Add |W or |U to your mana pool. */

int card_mirrorpool(int player, int card, event_t event){
	/* Mirrorpool	0x200e710
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T: Add |C to your mana pool.
	 * |2|C, |T, Sacrifice ~: Copy target instant or sorcery spell you control. You may choose new targets for the copy.
	 * |4|C, |T, Sacrifice ~: Put a token onto the battlefield that's a copy of target creature you control. */

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = td.preferred_controller = player;

	target_definition_t td2;
	counterspell_target_definition(player, card, &td2, TYPE_SPELL );
	td2.allowed_controller = td2.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	enum {
			CHOICE_MANA = 1,
			CHOICE_COPY_SPELL,
			CHOICE_TOKEN
	};

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK | GAA_SACRIFICE_ME | GAA_UNTAPPED, MANACOST_X(3), 0, &td2, NULL) )
		{
			return 99;
		}
		if( has_colorless_only_mana(player, 1) &&
			generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME | GAA_UNTAPPED, MANACOST_X(5), 0, &td, NULL) )
		{
			return 1;
		}
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = instance->number_of_targets = 0;
		int abilities[3] = {0, 0, 0};
		if( mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			abilities[0] = 1;
		}
		if( ! paying_mana() ){
			if( has_colorless_only_mana(player, 1) &&
				generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SPELL_ON_STACK | GAA_SACRIFICE_ME | GAA_UNTAPPED,
											MANACOST_X(3), 0, &td2, NULL) )
			{
				abilities[1] = 1;
			}
			if( has_colorless_only_mana(player, 1) &&
				generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_SACRIFICE_ME | GAA_UNTAPPED,
											MANACOST_X(5), 0, &td, NULL) )
			{
				abilities[2] = 1;
			}
		}
		int choice = DIALOG(player, card, event, DLG_RANDOM,
							"Produce mana", abilities[0], 1,
							"Copy spell", abilities[1], 10,
							"Copy creature", abilities[2], 5);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_MANA ){
			return mana_producer(player, card, event);
		}
		if( choice == CHOICE_COPY_SPELL ){
			if( ! charge_colorless_mana_only(player, 1) ){
				spell_fizzled = 1;
				return 0;
			}
			return generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK | GAA_SACRIFICE_ME | GAA_UNTAPPED, MANACOST_X(2),
												0, &td2, "TARGET_SPELL");
		}
		if( choice == CHOICE_TOKEN ){
			if( ! charge_colorless_mana_only(player, 2) ){
				spell_fizzled = 1;
				return 0;
			}
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME | GAA_UNTAPPED | GAA_LITERAL_PROMPT,
												MANACOST_X(4), 0, &td, "Select target creature you control.");
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == CHOICE_COPY_SPELL && valid_target(&td2) ){
			copy_spell_from_stack(player, instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == CHOICE_TOKEN && valid_target(&td) ){
			copy_token(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_needles_spires(int player, int card, event_t event){
	/* Needle Spires	0x200e715
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T: Add |R or |W to your mana pool.
	 * |2|R|W: ~ becomes a 2/1 |Sred and |Swhite Elemental creature with double strike until end of turn. It's still a land. */

  comes_into_play_tapped(player, card, event);

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_ANIMATE
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1, DLG_NO_DISPLAY_FOR_AI,
						"Produce mana",
							mana_producer(player, card, EVENT_CAN_ACTIVATE),
							paying_mana(),
						"Become Elemental",
							!paying_mana(),
							!(get_card_instance(player, card)->token_status & STATUS_LEGACY_TYPECHANGE),
						DLG_MANA(MANACOST_XRW(2,1,1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_ANIMATE)
		{
		  if (event == EVENT_RESOLVE_ACTIVATION)
			animate_self(player, card, 2, 1, KEYWORD_DOUBLE_STRIKE, 0, COLOR_TEST_WHITE|COLOR_TEST_RED, 0);
		  return 0;
		}
	}

  has_subtype_if_animated_self(player, card, event, SUBTYPE_ELEMENTAL);

  return mana_producer(player, card, event);
}

int card_ruins_of_oran_rief(int player, int card, event_t event){
	/* Ruins of Oran-Rief	0x200e71a
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T: Add |C to your mana pool.
	 * |T: Put a +1/+1 counter on target colorless creature that entered the battlefield this turn. */

	card_instance_t *instance= get_card_instance(player, card);

	comes_into_play_tapped(player, card, event);

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		instance->info_slot = 0;
		if( can_use_activated_abilities(player, card) && ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST0) ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Pump Creatures\n Cancel", 1);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
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
			activate_oran_rief_pump(player, instance->parent_card, COLOR_TEST_COLORLESS);
		}
	}

	if( trigger_condition == TRIGGER_EOT && affect_me(player, card ) && reason_for_trigger_controller == player && event == EVENT_TRIGGER ){	// not a real trigger; avoid the "processing" dialog
		remove_special_flags(2, -1, SF_JUST_CAME_INTO_PLAY);
	}

	return 0;
}

int card_sea_gate_wreckage(int player, int card, event_t event){
	/* Sea Gate Wreckage	0x200e71f
	 * Land
	 * |T: Add |C to your mana pool.
	 * |2|C, |T: Draw a card. Activate this ability only if you have no cards in hand. */

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	enum
	{
		CHOICE_MANA = 1,
		CHOICE_DRAW,
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_colorless_only_mana(player, 2) && hand_count[player] < 1 &&
			generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL) ){
			return 1;
		}
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int can_draw = 0;
		if( ! paying_mana() && has_colorless_only_mana(player, 2) && hand_count[player] < 1 &&
			generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL))
		{
			can_draw = 1;
		}
		int can_get_mana = mana_producer(player, card, EVENT_CAN_ACTIVATE);
		int choice = DIALOG(player, card, event, DLG_RANDOM,
								"Get mana", can_get_mana, 5,
								"Draw a card", can_draw, 10);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_MANA ){
			return mana_producer(player, card, event);
		}
		if( choice == CHOICE_DRAW ){
			add_state(player, card, STATE_TAPPED);
			if( charge_colorless_mana_only(player, 1) ){
				generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
				if( spell_fizzled == 1 ){
					remove_state(player, card, STATE_TAPPED);
				}
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == CHOICE_DRAW ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

/* Submerged Boneyard --> mana_producer_tapped	0x2002d57
 * Land
 * ~ enters the battlefield tapped.
 * |T: Add |U or |B to your mana pool. */

/* Timber Gorge --> mana_producer_tapped	0x2002d57
 * Land
 * ~ enters the battlefield tapped.
 * |T: Add |R or |G to your mana pool. */

/* Tranquil Expanse --> mana_producer_tapped	0x2002d57
 * Land
 * ~ enters the battlefield tapped.
 * |T: Add |G or |W to your mana pool. */

/* Unknown Shores	""	=>time_spiral.c:card_prismatic_lens()
 * Land
 * |T: Add |C to your mana pool.
 * |1, |T: Add one mana of any color to your mana pool. */

static int wandering_fumarole_animated_ability(int player, int card, event_t event){
	// |0: Switch this creature's power and toughness until end of turn.

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	if( IS_GAA_EVENT(event) ){
		card_instance_t *inst = get_card_instance(player, card);
		int p = inst->damage_target_player;
		int c = inst->damage_target_card;
		if( p > -1 ){
			if( event == EVENT_RESOLVE_ACTIVATION ){
				switch_power_and_toughness_until_eot(p, c, p, c);
			}
			return granted_generic_activated_ability(player, card, p, c, event, 0, MANACOST0, 0, NULL, NULL);
		}
	}

	return 0;
}

int card_wandering_fumarole(int player, int card, event_t event){
	/* Wandering Fumarole	0x200e724
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T: Add |U or |R to your mana pool.
	 * |2|U|R: Until end of turn, ~ becomes a 1/4 |Sblue and |Sred Elemental creature with
	 * 			"|0: Switch this creature's power and toughness until end of turn." It's still a land. */

  comes_into_play_tapped(player, card, event);

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_ANIMATE
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1, DLG_NO_DISPLAY_FOR_AI,
						"Produce mana",
							mana_producer(player, card, EVENT_CAN_ACTIVATE),
							paying_mana(),
						"Become Elemental",
							!paying_mana(),
							!(get_card_instance(player, card)->token_status & STATUS_LEGACY_TYPECHANGE),
						DLG_MANA(MANACOST_XUR(2,1,1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_ANIMATE)
		{
			if (event == EVENT_RESOLVE_ACTIVATION){
				int legacy = animate_self(player, card, 1,4, 0,SP_KEYWORD_LIFELINK, COLOR_TEST_BLUE | COLOR_TEST_RED, 0);
				add_status(player, legacy, STATUS_INVISIBLE_FX);
				card_instance_t* instance = get_card_instance(player, card);
				create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &wandering_fumarole_animated_ability,
												instance->parent_controller, instance->parent_card);
				return 0;
			}
		}
	}

  has_subtype_if_animated_self(player, card, event, SUBTYPE_ELEMENTAL);

  return mana_producer(player, card, event);
}

int card_wastes(int player, int card, event_t event){
	/* Wastes	0x200e729
	 * Basic Land
	 * |T: Add |C to your mana pool. */
	return mana_producer(player, card, event);
}
