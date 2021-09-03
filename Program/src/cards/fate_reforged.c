#include "manalink.h"

// Functions
void bolster(int player, int card, int num_counters){
	/* Choose a creature with the least toughness or tied with the least toughness among creatures you control. Put "num_counters" +1/+1 counters on it.*/
	card_instance_t *instance = get_card_instance( player, card );
	int t_player = -1;
	int t_card = -1;
	int doubles_count = 1;
	int min_t = 100;
	int count = active_cards_count[player]-1;
	while( count > -1 ){
			if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
				int this_t = get_toughness(player, count);
				if( this_t < min_t ){
					t_player = player;
					t_card = count;
					min_t = this_t;
					doubles_count = 1;
				}
				else if( this_t == min_t ){
						doubles_count++;
				}
			}
			count--;
	}

	if( doubles_count > 1 ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.toughness_requirement = min_t;
		td.allow_cancel = 0;

		instance->number_of_targets = 0;
		char msg[100];
		scnprintf(msg, 100, "Select a creature you control with toughness %d.", min_t);
		if( new_pick_target(&td, msg, 0, GS_LITERAL_PROMPT) ){
			t_player = instance->targets[0].player;
			t_card = instance->targets[0].card;
		}
	}

	if( t_player != -1 && t_card != -1 ){
		add_counters(t_player, t_card, COUNTER_P1_P1, num_counters);
	}

}

static int manifest_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		if( instance->dummy3 > 0 ){
			if( instance->targets[1].player != 66 ){
				get_card_instance(instance->damage_target_player, instance->damage_target_card)->internal_card_id = instance->dummy3;
			}
			else{
				if( event == EVENT_CHANGE_TYPE && affect_me(instance->damage_target_player, instance->damage_target_card) &&
					check_special_flags2(instance->damage_target_player, instance->damage_target_card, SF2_FACE_DOWN_DUE_TO_MANIFEST)
				  ){
					event_result = instance->dummy3;
				}
			}
		}
		if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
			if( trigger_cause_controller == instance->damage_target_player && trigger_cause == instance->damage_target_card ){
				instance->targets[1].player = 66;
			}
		}
		if( event == EVENT_CAN_ACTIVATE && is_what(-1, instance->targets[1].card, TYPE_CREATURE) ){
			if( has_mana_to_cast_iid(player, event, instance->targets[1].card) ){
				return 1;
			}
		}
		if( event == EVENT_ACTIVATE && is_what(-1, instance->targets[1].card, TYPE_CREATURE) ){
			charge_mana_from_id(player, -1, event, cards_data[instance->targets[1].card].id);
			if( spell_fizzled != 1 ){
				if( call_card_function(player, card, EVENT_HAS_MORPH) == 1 ){
					instance->state |= STATE_DONT_RECOPY_ONTO_STACK;
					flip_card(player, card);
					add_status(player, card, STATUS_INVISIBLE_FX);
				}
				else{
					check_for_turned_face_up_card_interactions(player, card, instance->targets[1].card);
					instance->dummy3 = -1;
					instance->state |= STATE_DONT_RECOPY_ONTO_STACK;
					add_status(player, card, STATUS_INVISIBLE_FX);
				}
			}
		}
	}

	if( event == EVENT_CLEANUP && instance->dummy3 < 1 ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

static int manifest_iid(int player, int card, int t_player, int iid){
	int fdc_iid = get_internal_card_id_from_csv_id(CARD_ID_FACE_DOWN_CREATURE);
	int card_added = add_card_to_hand(t_player, iid);
	int legacy = -1;
	if( player == t_player ){
		legacy = create_targetted_legacy_activate(player, card, &manifest_legacy, t_player, card_added);
	}
	else{
		int fake = add_card_to_hand(t_player, get_original_internal_card_id(player, card));
		create_targetted_legacy_activate(t_player, fake, &manifest_legacy, t_player, card_added);
		obliterate_card(t_player, fake);
	}
	card_instance_t *instance = get_card_instance(player, legacy);
	instance->dummy3 = fdc_iid;
	instance->targets[1].card = iid;
	if( call_card_function(t_player, card_added, EVENT_HAS_MORPH) == 1 ){
		turn_face_down(t_player, card_added);
	}
	else{
		set_special_flags2(t_player, card_added, SF2_FACE_DOWN_DUE_TO_MANIFEST);
	}
	put_into_play(t_player, card_added);
	return card_added;
}

static int manifest(int player, int card){
	/*
	  To manifest a card, put it onto the battlefield face down as a 2/2 creature.
	  You may turn it face up at any time for its mana cost if it's a creature card.
	*/
	int card_added = -1;
	int *deck = deck_ptr[player];
	if( deck[0] != -1 ){
		int iid = deck[0];
		remove_card_from_deck(player, 0);
		card_added = manifest_iid(player, card, player, iid);
	}
	return card_added;
}

static int manifesting_aura(int player, int card, event_t event, int key, int s_key){

	if( event == EVENT_RESOLVE_SPELL ){
		int result = manifest(player, card);
		if( result > -1 ){
			add_a_subtype(player, card, SUBTYPE_AURA_CREATURE);
			attach_aura_to_target(player, card, event, player, result);
		}
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 && in_play(player, card) && ! is_humiliated(player, card) ){
		return generic_aura(player, card, event, player, 0, 0, key, s_key, 0, 0, 0);
	}

	return global_enchantment(player, card, event);
}


enum{
	KHANS = 0,
	DRAGONS = 1,
};

static int choose_khans_or_dragons_no_storage(int player, int card, int priority_khans, int priority_dragons){
	return do_dialog(player, player, card, -1, -1, " Khans\n Dragons", priority_khans > priority_dragons ? 0 : 1);
}

static void choose_khans_or_dragons(int player, int card, int priority_khans, int priority_dragons){
	int result = choose_khans_or_dragons_no_storage(player, card, priority_khans, priority_dragons);
	get_card_instance(player, card)->targets[1].player = result;
}

static int get_khans_or_dragons(int player, int card){
	return get_card_instance(player, card)->targets[1].player;
}

static int dash_cost_cless(int player, int cless)
{
  int c;
  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_play(player, c) && get_id(player, c) == CARD_ID_WARBRINGER && !is_humiliated(player, c)
		&& (cless -= 2) <= 0)
	  return 0;
  return cless;
}

void dash(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white){
	//Dash: You may cast this spell for its dash cost. If you do, it gains haste, and it's returned from the battlefield to its owner's hand
	//	at the beginning of the next end step.

	if (card == -1){
		return;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( has_mana_multi(player, dash_cost_cless(player, cless), black, blue, green, red, white) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 1;
		if( instance->info_slot == 1 && ! played_for_free(player, card) && ! is_token(player, card) ){
			choice = do_dialog(player, player, card, -1, -1, " Use Dash\n Cast normally\n Cancel", 0);
		}
		if( choice == 0 ){
			charge_mana_multi(player, dash_cost_cless(player, cless), black, blue, green, red, white);
			if( spell_fizzled != 1 ){
				instance->info_slot |= 2;
			}
		}
		else if( choice == 1 ){
				charge_mana_from_id(player, -1, event, get_id(player, card));
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 2 ){
			pump_ability_t pump;
			default_pump_ability_definition(player, card, &pump, 0, 0, 0, SP_KEYWORD_HASTE);
			pump.paue_flags = PAUE_END_AT_EOT | PAUE_REMOVE_TARGET_AT_EOT;
			pump.eot_removal_method = ACT_BOUNCE;
			alternate_legacy_text(2, player, pump_ability(player, card, player, card, &pump));
		}
	}
}

static int runemark(int player, int card, event_t event, int col_test1, int col_test2, int key, int s_key){
	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_ABILITIES && affect_me(instance->damage_target_player, instance->damage_target_card) && ! is_humiliated(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.color = get_sleighted_color_test(player, card, col_test1) | get_sleighted_color_test(player, card, col_test1);
			if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
				if( key ){
					event_result |= key;
				}
				if( s_key ){
					special_abilities(player, card, event, s_key, instance->damage_target_player, instance->damage_target_card);
				}
			}
		}
	}

	return generic_aura(player, card, event, player, 2, 2, 0, 0, 0, 0, 0);
}

// Cards

// Colorless
int card_ugin_the_spirit_dragon(int player, int card, event_t event){
	/*
	  Ugin, the Spirit Dragon |8
	  Planeswalker - Ugin
	  +2: Ugin, the Spirit Dragon deals 3 damage to target creature or player.
	  -X: Exile each permanent with converted mana cost X or less that's one or more colors.
	  -10: You gain 7 life, draw 7 cards, then put up to seven permanent cards from your hand onto the battlefield.
	  7
	*/
	if (event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){	// planeswalker() for EVENT_CAN_ACTIVATE; always at least one choice legal

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = COLOR_TEST_ANY_COLORED;

		card_instance_t *instance = get_card_instance(player, card);

		int priorities[3] = {10, 0, 20};
		if( event == EVENT_ACTIVATE ){
			int ai_value = 0;
			int i;
			for(i=0; i<=count_counters(player, card, COUNTER_LOYALTY); i++){
				this_test.cmc = i+1;
				this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
				int actual_value = 	3*check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test)-
									2*check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
				if( actual_value > ai_value ){
					ai_value = actual_value;
				}
			}
			priorities[1] = ai_value;
		}

		enum{
			CHOICE_DAMAGE = 1,
			CHOICE_SWIPE,
			CHOICE_ULTIMATUM
		} choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
						  "3 damage to creature or player", 1, priorities[0], 2,
						  "Exile permanents", 1, priorities[1], 0,
						  "Ultimatum", 1, priorities[2], -10
						  );

		if (event == EVENT_ACTIVATE){
			instance->number_of_targets = 0;
			if( choice == CHOICE_DAMAGE ){
				pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
			}
			if( choice == CHOICE_SWIPE ){
				int loyalty_to_remove = 0;
				int ai_value = 0;
				int i;
				for(i=0; i<=count_counters(player, card, COUNTER_LOYALTY); i++){
					this_test.cmc = i+1;
					int actual_value = 	3*check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test)-
										2*check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
					if( actual_value > ai_value ){
						ai_value = actual_value;
						loyalty_to_remove = i;
					}
				}
				if( player == HUMAN ){
					loyalty_to_remove = choose_a_number(player, "Remove how much Loyalty?", priorities[1]);
				}
				if( loyalty_to_remove > -1 && loyalty_to_remove <= count_counters(player, card, COUNTER_LOYALTY) ){
					remove_counters(player, card, COUNTER_LOYALTY, loyalty_to_remove);
					instance->targets[1].player = loyalty_to_remove;
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
		else {	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice){
				case CHOICE_DAMAGE:
				{
					if( valid_target(&td) ){
						damage_target0(player, card, 3);
					}
					break;
				}
				case CHOICE_SWIPE:
				{
					this_test.cmc = instance->targets[1].player+1;
					this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
					APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_REMOVE);};);
				}
				break;
				case CHOICE_ULTIMATUM:
				{
					gain_life(player, 7);
					draw_cards(player, 7);
					test_definition_t this_test2;
					new_default_test_definition(&this_test2, TYPE_PERMANENT, "Select a permanent to put into play.");
					this_test2.zone = TARGET_ZONE_HAND;
					int amount = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test2);
					this_test2.qty = MIN(7, amount);
					tutor_multiple_card_from_hand(player, player, player, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test2);
				}
				break;
			}
		}
	}

	return planeswalker(player, card, event, 7);
}

// White
int card_abzan_advantage(int player, int card, event_t event){
	/*
	  Abzan Advantage |1|W
	  Instant
	  Target player sacrifices an enchantment. Bolster 1
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
			impose_sacrifice(player, card, instance->targets[0].card, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			bolster(player, card, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_abzan_runemark(int player, int card, event_t event){
	/*
	  Abzan Runemark |2|W
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature gets +2/+2.
	  Enchanted creature has vigilance as long as you control a black or green permanent.
	*/
	return runemark(player, card, event, COLOR_TEST_BLACK, COLOR_TEST_GREEN, 0, SP_KEYWORD_VIGILANCE);
}

int card_abzan_skycaptain(int player, int card, event_t event){
	/*
	  Abzan Skycaptain |3|W
	  Creature - Bird Soldier
	  Flying
	  When Abzan Captain dies, bolster 2.
	  2/2
	*/
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		bolster(player, card, 2);
	}

	return 0;
}

/*
Arashin Cleric |1|W --> Cathedral Sanctifier
Creature - Human Cleric
When Arashin Cleric enters the battlefield, gain 3 life.
1/3

Aven Skirmisher |W --> vanilla
Creature - Bird Warrior
Flying
1/1
*/

static int channel_harm_legacy(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *dmg = get_card_instance(affected_card_controller, affected_card);
		if( dmg->internal_card_id == damage_card && dmg->info_slot > 0 ){
			if( dmg->damage_source_player != player && dmg->damage_target_player == player ){
				int amount = dmg->info_slot;
				dmg->info_slot = 0;

				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_CREATURE);

				card_instance_t *instance = get_card_instance(player, card);

				instance->number_of_targets = 0;
				if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
					damage_target0(player, card, amount);
				}
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_channel_harm(int player, int card, event_t event){
	/*
	  Channel Harm |5|W
	  Instant
	  Prevent all damage that would be dealt to you and permanents you control this turn by sources you don't control.
	  If damage is prevented this way, you may have Channel Harm deal that much damage to target creature.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &channel_harm_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_citadel_siege(int player, int card, event_t event){
	/*
	  Citadel Siege |2|W|W
	  Enchantment
	  As Citadel Siege enters the battlefield, choose Khans or Dragons.
	  * Khans - At the beginning of combat on your turn, put two +1/+1 counters on target creature you control.
	  * Dragons - At the beginning of combat on each opponent's turn, tap target creature that player controls.
	  */
	if( event == EVENT_RESOLVE_SPELL ){
		choose_khans_or_dragons(player, card, 3*count_subtype(player, TYPE_CREATURE, -1), 2*count_subtype(1-player, TYPE_CREATURE, -1));
	}

	if( beginning_of_combat(player, card, event, ANYBODY, -1) ){
		card_instance_t *instance = get_card_instance(player, card);

		if( current_turn == player && get_khans_or_dragons(player, card) == KHANS ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.preferred_controller = player;
			td.allowed_controller = player;
			td.allow_cancel = 0;

			instance->number_of_targets = 0;

			if( can_target(&td) && new_pick_target(&td, "Select target creature you control.", 0, GS_LITERAL_PROMPT) ){
				add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 2);
			}
		}

		if( current_turn != player && get_khans_or_dragons(player, card) == DRAGONS ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.preferred_controller = 1-player;
			td.allowed_controller = 1-player;
			td.allow_cancel = 0;

			instance->number_of_targets = 0;

			if( can_target(&td) && new_pick_target(&td, "Select target creature your opponent controls.", 0, GS_LITERAL_PROMPT) ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_daghatar_the_adamant(int player, int card, event_t event){
	/*
	  Daghatar the Adamant |3|W
	  Legendary Creature - Human Warrior
	  Vigilance
	  Daghatar the Adamant enters the battlefield with four +1/+1 counters on it.
	  1(B/G)(B/G): Move a +1/+1 counter from target creature onto a second target creature.
	  0/0
	*/
	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 4);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XB(2, 1));
			if( has_mana_hybrid(player, 2, COLOR_BLACK, COLOR_GREEN, c1) ){
				if( target_available(player, card, &td2) > 1 ){
					return 1;
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XB(2, 1));
		if( charge_mana_hybrid(player, card, 2, COLOR_BLACK, COLOR_GREEN, c1) ){
			if( new_pick_target(&td, "Select target creature with a +1/+1 counter.", 0, 1 | GS_LITERAL_PROMPT) ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				new_pick_target(&td2, "TARGET_CREATURE", 1, 1);
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td2, 1) ){
			move_counters(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card,
							COUNTER_P1_P1, 1);
		}
	}

	return 0;
}

int card_dragon_bell_monk(int player, int card, event_t event){
	/*
	  Dragon Bell Monk |2|W
	  Creature - Human Monk 2/2
	  Vigilance
	  Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
	*/
	vigilance(player, card, event);
	prowess(player, card, event);
	return 0;
}

int card_dragonscale_general(int player, int card, event_t event){
	/*
	  Dragonscale General |3|W
	  Creature - Human Warrior
	  At the beginning of your end step, bolster X, where X is the number of tapped creatures you control. (Choose a creature with the least toughness among creatures you control and put X +1/+1 counters on it.)
	  2/3
	*/
	if( current_turn == player && trigger_condition == TRIGGER_EOT && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_TAPPED;
		int amount = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
		if( eot_trigger_mode(player, card, event, player, amount > 0 ? RESOLVE_TRIGGER_MANDATORY : 0) ){
			bolster(player, card, amount);
		}
	}

	return 0;
}

static int has_1_1_counter(int iid, int unused, int player, int card)
{
	return count_counters(player, card, COUNTER_P1_P1) > 0;
}

int card_elite_scaleguard(int player, int card, event_t event){
	/*
	  Elite Scaleguard |4|W
	  Creature - Human Soldier
	  When Elite Scaleguard enters the battlefield, bolster 2.
	  Whenever a creature you control with a +1/+1 counter on it attacks, tap target creature defending player controls.
	  2/3
	*/
	if( comes_into_play(player, card, event) ){
		bolster(player, card, 2);
	}

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.special_selection_function = &has_1_1_counter;

	int amt;
	if ((amt = declare_attackers_trigger_test(player, card, event, DAT_TRACK, player, -1, &this_test))){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = 1-player;
		td.allowed_controller = 1-player;

		card_instance_t* instance = get_card_instance(player, card);

		for (--amt; amt >= 0; --amt){
			if( can_target(&td) ){
				instance->number_of_targets = 0;
				if( new_pick_target(&td, "Select target creature your opponent controls.", 0, GS_LITERAL_PROMPT) ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
			}
		}
	}

	return 0;
}

/*
Great-Horn Krushok |4|W --> vanilla
Creature - Beast
3/5
*/

int card_honors_reward(int player, int card, event_t event){
	/*
	  Honor's Reward |2|W
	  Instant
	  You gain 4 life. Bolster 2. (Choose a creature with the least toughness among creatures you control and put two +1/+1 counters on it.)
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 4);
		bolster(player, card, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_jeskai_barricade(int player, int card, event_t event){
	/*
	  Jeskai Barricade |1|W
	  Creature - Wall
	  Flash
	  Defender
	  When Jeskai Barricade enters the battlefield, you may return another creature you control to its owner's hand.
	  0/4
	*/

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME;

		card_instance_t *instance = get_card_instance(player, card);

		if( comes_into_play_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( new_pick_target(&td, "Select target creature you control.", 0, GS_LITERAL_PROMPT) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return flash(player, card, event);
}

int card_lightform(int player, int card, event_t event){
	/*
	  Lightform |1|W|W
	  Enchantment
	  When Lightform enters the battlefield, it becomes an Aura with enchant creature. Manifest the top card of your library and attach Lightform to it.
	  Enchanted creature has flying and lifelink.
	*/
	return manifesting_aura(player, card, event, KEYWORD_FLYING, SP_KEYWORD_LIFELINK);
}

int card_lotus_eye_mystics(int player, int card, event_t event){
	/*
	  Lotus-Eye Mystics |3|W
	  Creature - Human Monk
	  Prowess
	  When Lotus-Eye Mystics enters the battlefield, return target enchantment card from your graveyard to your hand.
	  3/2
	*/
	prowess(player, card, event);
	return card_auramancer(player, card, event);
}

int card_mardu_woe_reaper(int player, int card, event_t event){
	/*
	  Mardu Woe-Reaper |W
	  Creature - Human Warrior
	  Whenever Mardu Woe-Reaper or another Warrior enters the battlefield under your control,
	  you may exile target creature card from a graveyard. If you do, you gain 1 life.
	  2/1
	*/
	if( ! is_humiliated(player, card) && trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		if( (count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player)) ||
			(count_graveyard_by_type(1-player, TYPE_CREATURE) && ! graveyard_has_shroud(1-player))
		  ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.subtype = SUBTYPE_WARRIOR;
			this_test.not_me = 1;
			if( comes_into_play(player, card, event) || new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &this_test) ){
				test_definition_t test2;
				new_default_test_definition(&test2, TYPE_CREATURE, "Select target creature card.");
				if(select_target_from_either_grave(player, card, 0, AI_MAX_CMC, AI_MAX_CMC, &test2, 0, 1) != -1){
					card_instance_t *instance = get_card_instance( player, card );
					int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
					if( selected != -1 ){
						rfg_card_from_grave(instance->targets[0].player, selected);
						gain_life(player, 1);
					}
				}
			}
		}
	}
	return 0;
}

int card_mastery_of_the_unseen(int player, int card, event_t event){
	/*
	  Mastery of the Unseen |1|W
	  Enchantment
	  Whenever a permanent you control is turned face up, you gain 1 life for each creature you control.
	  3W: Manifest the top card of your library.
	  (Put it onto the battlefield face down as a 2/2 creature. Turn it face up at any time for its mana cost if it's a creature card.)
	*/

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		manifest(player, card);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XW(3, 1), 0, NULL, NULL);
}


// Monk token --> Rhino token

int card_monastery_mentor(int player, int card, event_t event){
	/*
	  Monastery Mentor |2|W
	  Creature - Human Monk
	  Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
	  Whenever you cast a noncreature spell, put a 1/1 white Monk creature token with prowess onto the battlefield.
	  2/2
	*/
	if( prowess(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_MONK, &token);
		token.pow = token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return 0;
}

int card_pressure_point(int player, int card, event_t event){
	/*
	  Pressure Point |1|W
	  Instant
	  Tap target creature.
	  Draw a card.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			action_on_target(player, card, 0, ACT_TAP);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_rally_the_ancestors(int player, int card, event_t event){
	/*
	  Rally the Ancestors |X|W|W
	  Instant
	  Return each creature card with converted mana cost X or less from your graveyard to the battlefield.
	  Exile those creatures at the beginning of your next upkeep. Exile Rally the Ancestors.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.cmc = get_card_instance(player, card)->info_slot + 1;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

		new_reanimate_all(player, card, player, &this_test, REANIMATE_HASTE_AND_EXILE_AT_EOT);
		kill_card(player, card, KILL_REMOVE);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

static int count_aura_attached_to_creatures(int player){
	int result = 0;
	int p;
	for(p=0; p<2; p++){
		if( p == player || player == ANYBODY ){
			int c;
			for(c=0; c<active_cards_count[p]; c++){
				if( in_play(p, c) && is_what(p, c, TYPE_ENCHANTMENT) && has_subtype(p, c, SUBTYPE_AURA) ){
					card_instance_t *instance = get_card_instance(p, c);
					if( instance->damage_target_player > -1 && instance->damage_target_card > -1 ){
						if( is_what(instance->damage_target_player, instance->damage_target_card, TYPE_CREATURE) ){
							result++;
						}
					}
				}
			}
		}
	}

	return result;
}

int card_sages_reverie(int player, int card, event_t event){
	/*
	  Sage's Reverie |3|W
	  Enchantment - Aura
	  Enchant creature.
	  When Sage's Reverie enters the battlefield, draw a card for each aura you control that's attached to a creature.
	  Enchanted creature gets +1/+1 for each aura you control that's attached to a creature.
	*/

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result += count_aura_attached_to_creatures(player);
		}
	}

	if( comes_into_play(player, card, event) ){
		draw_cards(player, count_aura_attached_to_creatures(player));
	}

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

int card_sandblast(int player, int card, event_t event){
	/*
	  Sandblast |2|W
	  Instant
	  Sandblast deals 5 damage to target attacking or blocking creature.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 5);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target attacking or blocking creature.", 1, NULL);
}

int card_sandsteppe_outcast(int player, int card, event_t event){
	/*
	  Sandsteppe Outcast |2|W
	  Creature - Human Warrior
	  When Sandsteppe Outcast enters the battlefield, choose one -
	  * Put a +1/+1 counter on Sandsteppe Outcast.
	  * Put a 1/1 white Spirit creature token with flying onto the battlefield.
	  */
	if( comes_into_play(player, card, event) ){
		int choice = do_dialog(player, player, card, -1, -1, " Add a +1/+1 counter\n Generate a Spirit", count_subtype(player, TYPE_CREATURE, -1) > 5 ? 0 : 1);
		if( choice == 0 ){
			add_1_1_counter(player, card);
		}
		else{
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIRIT, &token);
			token.color_forced = COLOR_TEST_WHITE;
			token.key_plus = KEYWORD_FLYING;
			generate_token(&token);
		}
	}

	return 0;
}

int card_soul_summons(int player, int card, event_t event){
	/*
	  Soul Summons |1|W
	  Sorcery
	  Manifest the top card of your library. (Put it onto the battlefield face down as a 2/2 creature. Turn it face up at any time for its mana cost if it's a creature card.)
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		manifest(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_soulfire_grand_master(int player, int card, event_t event){
	/*
	  Soulfire Grand Master |1|W
	  Creature - Human Monk
	  Lifelink
	  Instant and sorcery spells you control have lifelink.
	  2(U/R)(U/R): The next time you cast an instant or sorcery spell from your hand this turn, put that card into your hand instead of your graveyard as it resolves.
	  2/2
	*/

	card_instance_t *instance = get_card_instance(player, card);

	lifelink(player, card, event);

	if( ! is_humiliated(player, card) ){
		if (event == EVENT_DEAL_DAMAGE){
			card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
			if (damage->internal_card_id == damage_card && is_what(-1, damage->targets[3].player, TYPE_SPELL) && damage->info_slot > 0 &&
				damage->damage_source_player == player
			   ){
				instance->info_slot+=damage->info_slot;
			}
		}

		if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == player ){

			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					gain_life(player, instance->info_slot);
					instance->info_slot = 0;
			}
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_SPELL);
	td.allowed_controller = player;
	td.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK, MANACOST0, 0, &td, NULL) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XU(2, 1));
			if( has_mana_hybrid(player, 2, COLOR_BLUE, COLOR_RED, c1) ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XU(2, 1));
		if( charge_mana_hybrid(player, card, 2, COLOR_BLUE, COLOR_RED, c1) ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].player = card_on_stack;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			set_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_SOULFIRE_GRAND_MASTER);
		}
	}

	return 0;
}

int card_valorous_stance(int player, int card, event_t event){
	/*
	  Valorous Stance |1|W
	  Instant
	  Choose one -
	  * Target creature gains indestructible until end of turn.
	  * Destroy target creature with toughness 4 or greater.
	  */
	if(event == EVENT_CHECK_PUMP && current_phase == PHASE_AFTER_BLOCKING ){
		if( has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id) ){
			pumpable_toughness[player] += 99;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.power_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
			return 1;
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td2, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			int choice = 1;
			if( generic_spell(player, card, EVENT_CAN_CAST, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
				if( generic_spell(player, card, EVENT_CAN_CAST, GS_CAN_TARGET, &td2, NULL, 1, NULL) ){
					int ai_choice = 2;
					if( current_phase == PHASE_AFTER_BLOCKING ){
						ai_choice = 1;
					}
					choice = 1+do_dialog(player, player, card, -1, -1, " Give Indestructible\n Kill a crit with POW 4\n Cancel", ai_choice);
				}
			}
			else{
				choice = 2;
			}
			if( choice == 3 ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = choice+1;
		}
		if( instance->info_slot == 1 ){
			new_pick_target(&td, "Select target creature.", 0, 1 | GS_LITERAL_PROMPT);
		}
		if( instance->info_slot == 2 ){
			new_pick_target(&td2, "Select target creature with power 4 or more.", 0, 1 | GS_LITERAL_PROMPT);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == 1 && valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
		}
		if( instance->info_slot == 2 && valid_target(&td2) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_wandering_champion(int player, int card, event_t event){
	/*
	  Wandering Champion |1|W
	  Creature - Human Monk
	  Whenever Wandering Champion deals combat damage to a player, if you control a blue or red permanent, you may discard a card. If you do, draw a card.
	  3/1
	*/
	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE) | get_sleighted_color_test(player, card, COLOR_TEST_RED);
		if( hand_count[player] > 0 && check_battlefield_for_special_card(player, card, player, 0, &this_test) &&
			damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER | DDBM_TRIGGER_OPTIONAL) ){
			discard(player, 0, player);
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_wardscale_dragon(int player, int card, event_t event){
	/*
	  Wardscale Dragon |4|W|W
	  Creature - Dragon
	  Flying
	  As long as Wardscale Dragon is attacking, defending player can't cast spells.
	  4/4
	*/
	if( event == EVENT_MODIFY_COST_GLOBAL && is_attacking(player, card) && ! is_humiliated(player, card) ){
		if( affected_card_controller == 1-player ){
			infinite_casting_cost();
		}
	}

	return 0;
}

// Blue

int card_aven_surveyor(int player, int card, event_t event){
	/*
	  Aven Surveyor |3|U|U
	  Creature - Bird Scout
	  When Aven Surveyor enters the battlefield, choose one -
	  *Put a +1/+1 counter on Aven Surveyor
	  *Return target creature to its owner's hand
	  2/2
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		int choice = do_dialog(player, player, card, -1, -1, " Add a +1/+1 counter\n Bounce a creature", can_target(&td) ? 1 : 0);
		if( choice == 0 ){
			add_1_1_counter(player, card);
		}
		else{
			if( pick_target(&td, "TARGET_CREATURE") ){
				action_on_target(player, card, 0, ACT_BOUNCE);
			}
		}
	}

	return 0;
}

int card_cloudform(int player, int card, event_t event){
	/*
	  Cloudform |1|U|U
	  Enchantment
	  When Cloudform enters the battlefield, it becomes an aura with enchant creature. Manifest the top card of your library and attach Cloudform to it.
	  Enchanted creature has flying and hexproof.
	*/
	return manifesting_aura(player, card, event, KEYWORD_FLYING, SP_KEYWORD_HEXPROOF);
}

int card_enchanted_awareness(int player, int card, event_t event){
	/*
	  Enhanced Awareness |4|U
	  Instant
	  Draw three cards, then discard a card.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 3);
		discard(player, 0, player);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_fascination(int player, int card, event_t event){
	/*
	  Fascination |X|U|U
	  Sorcery
	  Choose one -
	  * Each player draws X cards.
	  * Each player puts the top X cards of his or her library into his or her graveyard.
	  */
	if( event == EVENT_RESOLVE_SPELL ){
		int choice = do_dialog(player, player, card, -1, -1, " Draw X\n Mill X", count_deck(player) > 20 && count_deck(1-player) < 20 ? 1 : 0);
		if( choice == 0 ){
			draw_cards(player, get_card_instance(player, card)->info_slot);
		}
		else{
			APNAP(p, {mill(p, get_card_instance(player, card)->info_slot);};);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

/*
Frost Walker |1|U --> Skulking Ghost
Creature - Elemental
When Frost Walker becomes the target of a spell or ability, sacrifice it.
4/1
*/

int card_jeskai_infiltrator(int player, int card, event_t event){
	/*
	  Jeskai Infiltrator |2|U
	  Creature - Human Monk
	  Jeskai Infiltrator is unblockable as long as you control no other creatures.
	  Whenever Jeskai Infiltrator deals combat damage to a player, exile it and the top card of your library in a face-down pile,
	  shuffle that pile, then manifest those cards.
	  2/3
	*/
	unblockable(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER) ){
		int iid = get_original_internal_card_id(player, card);
		kill_card(player, card, KILL_REMOVE);
		remove_card_from_rfg(player, cards_data[iid].id);
		manifest_iid(player, card, player, iid);
		manifest(player, card);
	}

	return 0;
}

int card_jeskai_runemark(int player, int card, event_t event){
	/*
	  Jeskai Runemark |2|U
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature gets +2/+2.
	  Enchanted creature has flying as long as you control a red or white permanent.
	*/
	return runemark(player, card, event, COLOR_TEST_RED, COLOR_TEST_WHITE, KEYWORD_FLYING, 0);
}

int card_jeskai_sage(int player, int card, event_t event){
	/*
	  Jeskai Sage |1|U
	  Creature - Human Monk
	  Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn)
	  When Jeskai Sage dies, draw a card.
	  1/1
	*/
	prowess(player, card, event);
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		draw_cards(player, 1);
	}

	return 0;
}

/*
Lotus Path Djinn |3|U --> Jeskai Windscout
Creature - Djinn Monk
Flying
Prowess
2/3
*/

int card_marang_river_prowler(int player, int card, event_t event){
	/*
	  Marang River Prowler |2|U
	  Creature - Human Rogue
	  Marang River Prowler can't block and can't be blocked.
	  You may cast Marang River Prowler from your graveyard as long as you control a black or green permanent.
	  2/1
	*/
	cannot_block(player, card, event);

	unblockable(player, card, event);

	if(event == EVENT_GRAVEYARD_ABILITY){
		if( can_sorcery_be_played(player, event) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN) | get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
			if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
				if( has_mana_to_cast_iid(player, event, get_original_internal_card_id(player, card)) ){
					return GA_PLAYABLE_FROM_GRAVE;
				}
			}
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_from_id(player, -1, event, get_id(player, card)) ){
			return GAPAID_REMOVE;
		}
	}

	return 0;
}

int card_mindscour_dragon(int player, int card, event_t event){
	/*
	  Mindscour Dragon |4|U|U
	  Creature - Dragon
	  Whenever Mindscour Dragon deals combat damage to an opponent, target player puts the top four cards of his or her library into his or her graveyard.
	  4/4
	*/
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER) ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			mill(instance->targets[0].player, 4);
		}
	}

	return 0;
}

int card_mistfire_adept(int player, int card, event_t event){
	/*
	  Mistfire Adept |3|U
	  Creature - Human Monk
	  Prowess
	  Whenever you cast a noncreature spell, target creature gains flying until end of turn.
	  3/3
	*/

	if( prowess(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			instance->number_of_targets = 0;
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
		}
	}

	return 0;
}

/*
Monastery Siege |2|U --> Skipped, both abilities could only badly approximated.
Enchantment
When Monastery Siege enters the battlefield, choose Khans or Dragons.
* Khans - At the beginning of your draw step, draw an additional card, then discard a card.
* Dragons - Spells your opponents cast that target you or a permanent you control cost 2 more to cast.
*/

int card_neutralizing_blast(int player, int card, event_t event){
	/*
	  Neutralizing Blast |1|U
	  Instant
	  Counter target multicolored spell.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ANY);
	td.illegal_type = TYPE_LAND;
	td.extra = (int32_t)is_multicolored;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	return counterspell(player, card, event, &td, 0);
}

int card_rakshasas_disdain(int player, int card, event_t event){
	/*
	  Rakshasa's Disdain |2|U
	  Instant
	  Counter target spell unless its controller pays 1 for each card in your graveyard.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, count_graveyard(player));
		kill_card(player, card, KILL_DESTROY);
	}

	return counterspell(player, card, event, NULL, 0);
}

int card_reality_shift(int player, int card, event_t event){
	/*
	  Reality Shift |1|U
	  Instant
	  Exile target creature. Its controller manifests the top card of his or her library. (That player puts it onto the battlefield face down as a 2/2 creature. If it's a creature card, it can be turned face up at any time for its mana cost.)
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			int *deck = deck_ptr[instance->targets[0].player];
			if( deck[0] != -1 ){
				int iid = deck[0];
				remove_card_from_deck(instance->targets[0].player, 0);
				manifest_iid(player, card, instance->targets[0].player, iid);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_refocus(int player, int card, event_t event){
	/*
	  Refocus |1|U
	  Instant
	  Untap target creature.
	  Draw a card.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

static int renowned_weaponsmith_tutor(int iid, int unused, int unused2, int unused3){
	return (cards_data[iid].id == CARD_ID_HEART_PIERCER_BOW || cards_data[iid].id == CARD_ID_VIAL_OF_DRAGONFIRE);
}

int card_renowned_weaponsmith(int player, int card, event_t event){
	/*
	  Renowned Weaponsmith |1|U --> Approximation
	  Creature - Human Artificer
	  T: Add 2 to your mana pool. Spend this mana only to cast artifact spells or activate abilities of artifacts.
	  U, T: Search your library for a card named Heart-Piercer Bow or Vial of Dragonfire, reveal it, put it into your hand, then shuffle your library.
	  1/3
	*/
	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			declare_mana_available(player, COLOR_ARTIFACT, 2);
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( mana_producer(player, card, event) ){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_U(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_U(1), 0, NULL, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Tutor\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		instance->info_slot = choice;
		if( choice == 0 ){
			produce_mana_tapped(player, card, COLOR_ARTIFACT, 2);
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST_U(1)) ){
					tap_card(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION && instance->info_slot == 1 ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.special_selection_function = &renowned_weaponsmith_tutor;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
	}

	return 0;
}

int card_rite_of_undoing(int player, int card, event_t event){
	/*
	  Rite of Undoing |4|U
	  Instant
	  Delve
	  Return target nonland permanent you control and target nonland permanent you don't control to their owners' hands.
	*/
	modify_cost_for_delve(player, card, event);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_type = TYPE_LAND;
	td1.allowed_controller = player;
	td1.preferred_controller = player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_PERMANENT);
	td2.illegal_type = TYPE_LAND;
	td2.allowed_controller = player;
	td2.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td1, NULL, 1, NULL) ){
			return can_target(&td2);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		int result = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			result = cast_spell_with_delve(player, card);
			if( ! result ){
				spell_fizzled = 1;
				return 0;
			}
		}
		td1.allow_cancel = (result == 2 ? 0 : 1);
		td2.allow_cancel = (result == 2 ? 0 : 1);
		instance->number_of_targets = 0;
		if( new_pick_target(&td1, "target nonland permanent you control.", 0, 1 | GS_LITERAL_PROMPT) ){
			new_pick_target(&td1, "target nonland permanent your opponent controls.", 1, 1 | GS_LITERAL_PROMPT);
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( validate_target(player, card, &td1, 0) ){
			action_on_target(player, card, 0, ACT_BOUNCE);
		}
		if( validate_target(player, card, &td2, 1) ){
			action_on_target(player, card, 1, ACT_BOUNCE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sage_eye_avengers(int player, int card, event_t event){
	/*
	  Sage-Eye Avengers |4|U|U
	  Creature - Djinn Monk
	  Prowess
	  Whenever Sage-Eye Avengers attacks, you may return target creature to its owner's hand if its power is less than Sage-Eye Avengers's power.
	  4/5
	*/
	prowess(player, card, event);
	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( player == AI ){
			td.power_requirement = (get_power(player, card)-1) | TARGET_PT_LESSER_OR_EQUAL;
		}

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 1;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			action_on_target(player, card, 0, ACT_BOUNCE);
		}
	}

	return 0;
}

int card_shifting_loyalties(int player, int card, event_t event){
	/*
	  Shifting Loyalties |5|U
	  Sorcery
	  Exchange control of two target permanents that share a type.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
			int types[5] = {TYPE_ARTIFACT, TYPE_LAND, TYPE_CREATURE, TYPE_ENCHANTMENT, TARGET_TYPE_PLANESWALKER};
			int i;
			for(i=0; i<5; i++){
				target_definition_t td;
				default_target_definition(player, card, &td, types[i] );
				td.preferred_controller = player;
				td.allowed_controller = player;

				target_definition_t td1;
				default_target_definition(player, card, &td1, types[i] );
				td1.preferred_controller = 1-player;
				td1.allowed_controller = 1-player;

				if( can_target(&td) && can_target(&td1) ){
					return 1;
				}
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT );
		td.preferred_controller = player;
		td.allowed_controller = player;

		instance->number_of_targets = 0;

		if( new_pick_target(&td, "Select target permanent you control.", 0, 1 | GS_LITERAL_PROMPT) ){
			instance->targets[2].player = get_type(instance->targets[0].player, instance->targets[0].card);
			add_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
			target_definition_t td1;
			default_target_definition(player, card, &td1, instance->targets[2].player);
			td1.preferred_controller = 1-player;
			td1.allowed_controller = 1-player;
			new_pick_target(&td, "Select target permanent your opponent controls that share a type with the first target.", 1, 1 | GS_LITERAL_PROMPT);
			remove_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT );
		td.preferred_controller = player;
		td.allowed_controller = player;

		target_definition_t td1;
		default_target_definition(player, card, &td1, instance->targets[2].player);
		td1.preferred_controller = 1-player;
		td1.allowed_controller = 1-player;

		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td1, 1) ){
			exchange_control_of_target_permanents(player, card, instance->targets[0].player, instance->targets[0].card,
													instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_shun_yun_the_silent_tempest(int player, int card, event_t event){
	/*
	  Shu Yun, the Silent Tempest |2|U
	  Legendary Creature - Human Monk
	  Prowess (Whenever you cast a noncreature spell, this creature gets +1/+1 until end of turn.)
	  Whenever you cast a noncreature spell, you may pay (R/W)(R/W). If you do, target creature gains double strike until end of turn.
	  3/2
	*/
	check_legend_rule(player, card, event);

	if( prowess(player, card, event) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && has_mana_hybrid(player, 2, COLOR_RED, COLOR_WHITE, 0) ){
			if( do_dialog(player, player, card, -1, -1, " Give double strike\n Pass", 1) == 0 ){
				if( charge_mana_hybrid(player, card, 2, COLOR_RED, COLOR_WHITE, 0) && pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 0;
					pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
				}
			}
		}
	}

	return 0;
}
/*
Sultai Skullkeeper |1|U --> Screeching Skaab
Creature - Naga Shaman
When Sultai Skullkeeper enters the battlefield, put the top two cards of your library into your graveyard.
2/1
*/

int card_supplant_form(int player, int card, event_t event){
	/*
	  Supplant Form |4|U|U
	  Instant
	  Return target creature to its owner's hand. You put a token onto the battlefield that's a copy of that creature.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			token_generation_t token;
			if( is_token(instance->targets[0].player, instance->targets[0].card) ){
				copy_token_definition(player, card, &token, instance->targets[0].player, instance->targets[0].card);
			}
			else{
				default_token_definition(player, card, get_id(instance->targets[0].player, instance->targets[0].card), &token);
			}
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_temporal_trespass(int player, int card, event_t event){
	/*
	  Temporal Trespass |8|U|U|U
	  Sorcery
	  Delve
	  Take an extra turn after this one. Exile Temporal Trespass.
	*/
	modify_cost_for_delve(player, card, event);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		ai_modifier+=96;
		int result = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			result = cast_spell_with_delve(player, card);
			if( ! result ){
				spell_fizzled = 1;
				return 0;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		time_walk_effect(player, card);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_torrent_elemental(int player, int card, event_t event){
	/*
	  Torrent Elemental |4|U
	  Creature - Elemental
	  Flying
	  Whenever Torrent Elemental attacks, tap all creatures defending player controls.
	  3(B/G)(B/G): Put Torrent Elemental from exile onto the battlefield tapped. Activate this ability only any time you could cast a sorcery.
	  3/5
	*/
	//The rest of the code is in "activate_cards_in_hand" in "rules_engine.c"
	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, 1-player, &this_test, ACT_TAP);
	}

	return 0;
}

int card_whisk_away(int player, int card, event_t event){
	/*
	  Whisk Away |2|U
	  Instant
	  Put target attacking or blocking creature on top of its owner's library.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target attacking or blocking creature.", 1, NULL);
}

int card_will_of_the_naga(int player, int card, event_t event){
	/*
	  Will of the Naga |4|U|U
	  Instant
	  Delve
	  Tap up to two target creatures. Those creatures don't untap during their controller's next untap step.
	*/
	modify_cost_for_delve(player, card, event);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		int result = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			result = cast_spell_with_delve(player, card);
			if( ! result ){
				spell_fizzled = 1;
				return 0;
			}
		}
		td.allow_cancel = (result == 2 ? 0 : 1);
		instance->number_of_targets = 0;
		if( new_pick_target(&td, "TARGET_CREATURE", 0, 0) ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			new_pick_target(&td, "TARGET_CREATURE", 1, 0);
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				effect_frost_titan(player, card, instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_write_into_being(int player, int card, event_t event){
	/*
	  Write into Being |2|U
	  Sorcery
	  Look at the top two cards of your library. Manifest one of those cards, then put the other on top or bottom of your library.
	*/
	if( event == EVENT_RESOLVE_SPELL){
		int amount = MIN(2, count_deck(player));
		if( amount ){
			int *deck = deck_ptr[player];
			int selected = 0;
			if( amount > 1 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to Manifest");
				this_test.create_minideck = 2;
				selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 1, AI_MIN_VALUE, -1, &this_test);
			}
			int iid = deck[selected];
			remove_card_from_deck(player, selected);
			manifest_iid(player, card, player, iid);
			amount--;
			if( amount ){
				scry(player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

// Black

int card_aleshas_vanguard(int player, int card, event_t event){
	/*
	  Alesha's Vanguard |3|B
	  Creature - Orc Warrior
	  Dash 2B
	  3/3
	*/
	dash(player, card, event, MANACOST_XB(2, 1));

	return 0;
}

int card_ancestral_vengeance(int player, int card, event_t event){
	/*
	  Ancestral Vengeance |B|B
	  Enchantment - Aura
	  Enchant creature
	  When Ancestral Vengeance enters the battlefield, put a +1/+1 counter on target creature you control.
	  Enchanted creature gets -1/-1.
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card);

		if( can_target(&td) &&  new_pick_target(&td, "Select target creature you control.", 1, GS_LITERAL_PROMPT) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_aura(player, card, event, 1-player, -1, -1, 0, 0, 0, 0, 0);
}

int card_archfiend_of_depravity(int player, int card, event_t event){
	/*
	  Archfiend of Depravity |3|B|B
	  Creature - Demon
	  At the beginning of each opponent's end step, that player chooses up to two creatures he or she controls, then sacrifices the rest.
	  5/4
	*/
	if( current_turn != player && eot_trigger(player, card, event) ){
		if( can_sacrifice(player, 1-player, 1, TYPE_CREATURE, 0) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allowed_controller = 1-player;
			td.preferred_controller = 1-player;
			td.who_chooses = 1-player;
			td.illegal_abilities = 0;

			card_instance_t *instance = get_card_instance( player, card);

			instance->number_of_targets = 0;

			if( can_target(&td) && new_pick_target(&td, "Select a creature to keep (1 of 2).", 0, GS_LITERAL_PROMPT) ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				if( can_target(&td) && new_pick_target(&td, "Select a creature to keep (2 of 2).", 1, GS_LITERAL_PROMPT) ){
					state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
				}
			}

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_manipulate_all(player, card, 1-player, &this_test, KILL_SACRIFICE);

			int i;
			for (i = 0; i < instance->number_of_targets; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
		}
	}

	return 0;
}

int card_battle_brawler(int player, int card, event_t event){
	/*
	  Battle Brawler |1|B
	  Creature - Orc Warrior
	  As long as you control a red or white permanent, Battle Brawler gets +1/+0 and has first strike.
	  2/2
	*/
	if( ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_ABILITIES) && affect_me(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_RED) | get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
				switch( event ){
					case EVENT_POWER:
						event_result++;
						break;
					case EVENT_ABILITIES:
						event_result |= KEYWORD_FIRST_STRIKE;
						break;
					default:
						break;
				}
			}
		}
	}

	return 0;
}

static int brutal_hordechief_legacy(int player, int card, event_t event){
	if( event == EVENT_DECLARE_BLOCKERS ){
		int c;
		for(c=0; c<active_cards_count[1-player]; c++){
			if( in_play(1-player, c) && is_what(1-player, c, TYPE_CREATURE) && can_block(1-player, c) ){
				add_state(1-player, c, STATE_TARGETTED);
				select_blocker(1-player, c, player);
				remove_state(1-player, c, STATE_TARGETTED);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_brutal_hordechief(int player, int card, event_t event){
	/*
	  Brutal Hordechief |3|B
	  Creature - Orc Warrior
	  Whenever a creature you control attacks, defending player loses 1 life and you gain 1 life.
	  3(R/W)(R/W): Creatures your opponents control block this turn if able, and you choose how those creatures block.
	  3/3
	*/
	card_instance_t *instance = get_card_instance( player, card);

	int result = declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, -1);
	if( result ){
		lose_life(1-player, result);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XR(3, 2));
			return has_mana_hybrid(player, 2, COLOR_RED, COLOR_WHITE, c1);
		}
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XR(3, 2));
		charge_mana_hybrid(player, card, 2, COLOR_RED, COLOR_WHITE, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		create_legacy_effect(instance->parent_controller, instance->parent_card, &brutal_hordechief_legacy);
	}

	return 0;
}

int card_crux_of_fate(int player, int card, event_t event){
	/*
	  Crux of Fate |3|B|B
	  Sorcery
	  Choose one -
	  * Destroy all Dragon creatures.
	  * Destroy all non-Dragon creatures.
	  */
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_DRAGON;
		int ai_choice = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test) <
						check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test) ? 0 : 1;
		int choice = do_dialog(player, player, card, player, card, " Kill all Dragons\n Kill all non-Dragons", ai_choice) == 0 ? MATCH : DOESNT_MATCH;
		this_test.subtype_flag = choice;
		new_manipulate_all(player, card, ANYBODY, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_dark_deal(int player, int card, event_t event){
	/*
	  Dark Deal |2|B
	  Sorcery
	  Each player discards all the cards in his or her hand, then draws that many cards minus one.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int discarded[2] = {hand_count[0], hand_count[1]};
		APNAP(p, {
					new_discard_all(p, player);
					if( discarded[p] > 1 ){
						draw_cards(p, discarded[p]-1);
					};
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_diplomacy_of_the_wastes(int player, int card, event_t event){
	/*
	  Diplomacy of the Wastes |2|B
	  Sorcery
	  Target opponent reveals his or her hand. You choose a nonland card from it. That player discards that card. If you control a Warrior, that player loses 2 life.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			ec_definition_t ec;
			default_ec_definition(instance->targets[0].player, player, &ec);

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a nonland card.");
			this_test.type_flag = DOESNT_MATCH;
			new_effect_coercion(&ec, &this_test);

			if( check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_WARRIOR) ){
				lose_life(instance->targets[0].player, 2);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_fearsome_awakening(int player, int card, event_t event){
	/*
	  Fearsome Awakening |4|B
	  Sorcery
	  Return target creature card from your graveyard to the battlefield. If it's a Dragon, put two +1/+1 counters on it.
	*/
	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card.");
	this_test.ai_selection_mode = AI_MAX_CMC;

	if ( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			int result = reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			if( result != -1 && has_subtype(player, result, SUBTYPE_DRAGON) ){
				add_1_1_counters(player, result, 2);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &this_test);
}

int card_ghastly_conscripts(int player, int card, event_t event){
	/*
	  Ghastly Conscription |5|B|B
	  Sorcery
	  Exile all creature cards from target player's graveyard in a face-down pile, shuffle that pile, then manifest those cards. (To manifest a card, put it onto the battlefield face down as a 2/2 creature. Turn it face up at any time for its mana cost if it's a creature card.)
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int removed[100];
			int rc = 0;
			const int* grave = get_grave(instance->targets[0].player);
			int i;
			for(i=count_graveyard(instance->targets[0].player)-1; i>-1; i--){
				if( is_what(-1, grave[i], TYPE_CREATURE) ){
					removed[rc] = grave[i];
					rc++;
					rfg_card_from_grave(instance->targets[0].player, i);
				}
			}
			for(i=0; i<rc; i++){
				if( check_rfg(instance->targets[0].player, cards_data[removed[i]].id) ){
					remove_card_from_rfg(instance->targets[0].player, cards_data[removed[i]].id);
					manifest_iid(player, card, instance->targets[0].player, removed[i]);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_grave_strenght(int player, int card, event_t event){
	/*
	  Grave Strength |1|B
	  Sorcery
	  Choose target creature. Put the top three cards of your library into your graveyard, then put a +1/+1 counter on that creature for each creature card in your graveyard.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[player];
			int amount = 0;
			int milled = 0;
			while( deck[0] != -1 && milled < 3 ){
					if( is_what(-1, deck[milled], TYPE_CREATURE) ){
						amount++;
					}
					milled++;
			}
			mill(player, milled);
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/*
Gurmag Angler |6|B --> Tombstalker
Creature - Zombie Fish
Delve
5/5
*/

static const char* is_already_damaged(int who_chooses, int player, int card){
	if( get_card_instance(player, card)->damage_on_card > 0 ){
		return NULL;
	}
	return "must be a creature that was dealt damage this turn.";
}

int card_hooded_assasin(int player, int card, event_t event){
	/*
	  Hooded Assassin |2|B
	  Creature - Human Assassin
	  When Hooded Assassin enters the battlefield, choose one -
	  * Put a +1/+1 counter on Hooded Assassin.
	  * Destroy target creature that was dealt damage this turn.
	  1/2
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.extra = (int32_t)is_already_damaged;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td.allow_cancel = 0;

		int choice = do_dialog(player, player, card, -1, -1, " Add a +1/+1 counter\n Kill a damaged creature", can_target(&td) ? 1 : 0);
		if( choice == 0 ){
			add_1_1_counter(player, card);
		}
		else{
			if( can_target(&td) && new_pick_target(&td, "Select target creature that was dealt damage this turn.", 0, GS_LITERAL_PROMPT) ){
				action_on_target(player, card, 0, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_mardu_shadowspear(int player, int card, event_t event){
	/*
	  Mardu Shadowspear |B
	  Creature - Human Warrior
	  Whenever Mardu Shadowspear attacks, each opponent loses 1 life.
	  Dash 1B (You may cast this spell for its dash cost. If you do, it gains haste, and it's returned from the battlefield to its owner's hand at the beginning of the next end step.)
	  1/1
	*/
	dash(player, card, event, MANACOST_XB(1, 1));

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		lose_life(1-player, 1);
	}

	return 0;
}

int card_mardu_strike_leader(int player, int card, event_t event){
	/*
	  Mardu Strike Leader |2|B
	  Creature - Human Warrior
	  Whenever Mardu Strike Leader attacks, put a 2/1 black Warrior creature token onto the battlefield.
	  Dash 3B (You may cast this spell for its dash cost. If you do, it gains haste, and it is returned from the battlefield to your hand at the beginning of the next end step.)
	  3/2
	*/
	dash(player, card, event, MANACOST_XB(3, 1));

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WARRIOR, &token);
		token.pow = 2;
		token.pow = 1;
		token.color_forced = COLOR_TEST_BLACK;
		generate_token(&token);
	}

	return 0;
}

/*
Merciless Executioner |2|B -->Flesgbag Marauder
Creature - Orc Warrior
When Merciless Executioner enters the battlefield, each player sacrifices a creature.
3/1
*/

int card_noxious_dragon(int player, int card, event_t event){
	/*
	  Noxious Dragon |4|B|B
	  Creature - Dragon
	  Flying
	  When Noxious Dragon dies, you may destroy target creature with converted mana cost 3 or less.
	  4/4
	*/

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.extra = 3;
		td.special = TARGET_SPECIAL_CMC_LESSER_OR_EQUAL;

		if( can_target(&td) && new_pick_target(&td, "Select target creature with converted mana cost 3 or less.", 0, GS_LITERAL_PROMPT) ){
			action_on_target(player, card, 0, KILL_DESTROY);
		}
	}

	return 0;
}

int card_orc_sureshot(int player, int card, event_t event){
	/*
	  Orc Sureshot |3|B
	  Creature - Orc Archer
	  Whenever another creature enters the battlefield under your control, target creature an opponent controls gets -1/-1 until end of turn.
	  4/2
	*/
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;

		card_instance_t *instance = get_card_instance( player, card);

		instance->number_of_targets = 0;

		if( new_specific_cip(player, card, event, player, can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0, &this_test) ){
			pump_until_eot_merge_previous(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return 0;
}

int card_palace_siege(int player, int card, event_t event){
	/*
	  Palace Siege |3|B|B
	  Enchantment
	  As Palace Siege enters the battlefield, choose Khans or Dragons.
	  * Khans - At the beginning of your upkeep, return target creature card from your graveyard to your hand.
	  * Dragons - At the beginning of your upkeep, each opponent loses 2 life and you gain 2 life.
	  */

	if( event == EVENT_RESOLVE_SPELL ){
		choose_khans_or_dragons(player, card, 2*count_graveyard_by_type(player, TYPE_CREATURE), (20-life[1-player])*2 );
	}

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && reason_for_trigger_controller == player &&
		current_turn == player
	  ){
		if( get_khans_or_dragons(player, card) == KHANS && count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player) ){
			if( upkeep_trigger(player, card, event) ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
				new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
			}
		}
		if( get_khans_or_dragons(player, card) == DRAGONS ){
			if( upkeep_trigger(player, card, event) ){
				lose_life(1-player, 2);
				gain_life(player, 2);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_qarsi_high_priest(int player, int card, event_t event){
	/*
	  Qarsi High Priest |B
	  Creature - Human Cleric
	  1B, t, Sacrifice another creature: Manifest the top card of your library.
	  0/2
	*/
	if( event == EVENT_RESOLVE_ACTIVATION ){
		manifest(player, card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_CREATURE | GAA_NOT_ME_AS_TARGET, MANACOST_XB(1, 1), 0, NULL, NULL);
}

int card_reach_of_shadows(int player, int card, event_t event){
	/*
	  Reach of Shadows |4|B
	  Instant
	  Destroy target creature that's one or more colors.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = COLOR_TEST_ANY_COLORED;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature that's one or more colors.", 1, NULL);
}

int card_sibsig_host(int player, int card, event_t event){
	/*
	  Sibsig Host |4|B
	  Creature - Zombie
	  When Sibsig Host enters the battlefield, each player puts the top three cards of his or her library into his or her graveyard.
	  2/6
	*/
	if( comes_into_play(player, card, event) ){
		APNAP(p, {mill(p, 3);};);
	}

	return 0;
}

int card_sibsig_muckdraggers(int player, int card, event_t event){
	/*
	  Sibsig Muckdraggers |8|B
	  Creature - Zombie
	  Delve
	  When Sibsig Muckdraggers enters the battlefield, return target creature card from your graveyard to your hand.
	  3/6
	*/
	if( comes_into_play(player, card, event) ){
		return card_gravedigger(player, card, event);
	}

	return delve(player, card, event);
}

int card_soulflayer(int player, int card, event_t event){
	/*
	  Soulflayer |4|B|B
	  Creature - Demon
	  Delve
	  If a creature card with flying was exiled with Soulflayer's delve ability, Soulflayer has flying. The same is true for first strike, double strike, deathtouch, haste, hexproof, indestructible, lifelink, reach, trample, and vigilance.
	  4/4
	*/
	card_instance_t *instance = get_card_instance(player, card);

	modify_cost_for_delve(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			int result = cast_spell_with_delve(player, card);
			if( ! result ){
				spell_fizzled = 1;
			}
			else{
				instance->targets[1].player = result;
			}
		}
	}

	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) && instance->targets[1].player > 0 ){
		event_result |= instance->targets[1].player;
	}

	return 0;
}

int card_sultai_emissary(int player, int card, event_t event){
	/*
	  Sultai Emissary |1|B
	  Creature - Zombie Warrior
	  When Sultai Emissary dies, manifest the top card of your library.
	  (Put that card onto the battlefield face down as a 2/2 creature. Turn it face up any time for its mana cost if it's a creature card.)
	  1/1
	*/
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		manifest(player, card);
	}

	return 0;
}

int card_sultai_runemark(int player, int card, event_t event){
	/*
	  Sultai Runemark |2|B
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature gets +2/+2.
	  Enchanted creature has deathtouch as long as you control a green or blue permanent.
	*/
	return runemark(player, card, event, COLOR_TEST_BLUE, COLOR_TEST_GREEN, 0, SP_KEYWORD_DEATHTOUCH);
}

int card_tasigur_the_golden_fang(int player, int card, event_t event){
	/*
	  Tasigur, the Golden Fang |5|B
	  Legendary Creature - Human Shaman
	  Delve
	  2(G/U)(G/U): Put the top two cards of your library into your graveyard, then return a nonland card of an opponent's choice from your graveyard to your hand.
	  4/5
	*/
	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return has_mana_hybrid(player, 2, COLOR_BLUE, COLOR_GREEN, 2);
		}
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_hybrid(player, card, 2, COLOR_BLUE, COLOR_GREEN, 2);
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		mill(player, 2);
		if( count_graveyard(player) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to return to opponent's hand.");
			int selected = new_select_a_card(1-player, player, TUTOR_FROM_GRAVE, 1, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				from_grave_to_hand(player, selected, TUTOR_HAND);
			}
		}
	}

	return delve(player, card, event);
}

int card_tasigur_cruelty(int player, int card, event_t event){
	/*
	  Tasigur's Cruelty |5|B
	  Sorcery
	  Delve
	  Each opponent discards two cards.
	*/
	modify_cost_for_delve(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		int result = 1;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			result = cast_spell_with_delve(player, card);
			if( ! result ){
				spell_fizzled = 1;
				return 0;
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		new_multidiscard(1-player, 2, 0, player);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Red

static int check_for_alesha_reanimation(int player){
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.power = 3;
	this_test.power_flag = F5_POWER_LESSER_THAN_VALUE;
	return new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(player);
}

int card_alesha_who_smileas_at_death(int player, int card, event_t event){
	/*
	  Alesha, Who Smiles at Death |2|R
	  Legendary Creature - Human Warrior
	  First strike
	  Whenever Alesha, Who Smiles at Death attacks, you may pay (W/B)(W/B). If you do, return target creature card with power 2 or less from your graveyard to the battlefield tapped and attacking.
	  3/2
	*/
	check_legend_rule(player, card, event);

	if( declare_attackers_trigger(player, card, event, check_for_alesha_reanimation(player) && has_mana_hybrid(player, 2, COLOR_BLACK, COLOR_WHITE, 0) ?
									RESOLVE_TRIGGER_AI(player) : 0, player, card)
	  ){
		if( charge_mana_hybrid(player, card, 2, COLOR_BLACK, COLOR_WHITE, 0) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creture with Power 2 or less.");
			this_test.power = 3;
			this_test.power_flag = F5_POWER_LESSER_THAN_VALUE;
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY_ATTACKING, 1, AI_MAX_CMC, &this_test);
		}
	}

	return 0;
}

static int arcbond_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		int pp = instance->damage_target_player;
		int cc = instance->damage_target_card;
		card_instance_t* damage = damage_being_dealt(event);
		if (damage && damage->damage_target_card == cc && damage->damage_target_player == pp && damage->info_slot > 0){
			int i;
			for(i=1; i<19; i++){
				if( instance->targets[i].player < 1 ){
					instance->targets[i].player = damage->info_slot;
					break;
				}
				if( instance->targets[i].card < 1 ){
					instance->targets[i].card = damage->info_slot;
					break;
				}
			}
		}

		if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player){
			if( instance->targets[1].player > 0 ){
				if (event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				if (event == EVENT_RESOLVE_TRIGGER){
					// Free up targets, just in case something triggers instantly (and improperly) which eventually results in dealing damage back.
					int i, dam[20], dc = 0;
					for(i=1; i<18; i++){
						if( instance->targets[i].player > 0 ){
							dam[dc] = instance->targets[i].player;
							dc++;
							instance->targets[i].player = 0;
						}
						if( instance->targets[i].card > 0 ){
							dam[dc] = instance->targets[i].card;
							dc++;
							instance->targets[i].card = 0;
						}
					}
					for (i = 0; i < dc; ++i){
						APNAP(p, {new_damage_all(pp, cc, p, dam[i], NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);};);
					}
				}
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_arcbond(int player, int card, event_t event){
	/*
	  Arcbond |2|R
	  Instant
	  Choose target creature. Whenever that creature is dealt damage this turn, it deals that much damage to each other creature and each player.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, &arcbond_legacy, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}


/*
Bathe in Dragonfire |2|R --> Flame Slash
Sorcery
Bathe in Dragonfire deals 4 damage to target creature.
*/

int card_bloodfire_enforcer(int player, int card, event_t event){
	/*
	  Bloodfire Enforcers |3|R
	  Creature - Human Monk
	  Bloodfire Enforcers has first strike and trample as long as an instant card and a sorcery card are in your graveyard.
	  5/2
	*/
	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( count_graveyard_by_type(player, TYPE_SORCERY) && count_graveyard_by_type(player, TYPE_INSTANT | TYPE_INTERRUPT) ){
			haste(player, card, event);
		}
	}

	return 0;
}

int card_break_through_the_line(int player, int card, event_t event){
	/*
	  Break Through the Line |1|R
	  Enchantment
	  R: Target creature with power 2 or less gains haste until end of turn and can't be blocked this turn.
	*/

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, 0, SP_KEYWORD_HASTE | SP_KEYWORD_UNBLOCKABLE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_R(1), 0, &td, "Select target creature with Power 2 or less.");
}

int card_collateral_damage(int player, int card, event_t event){
	/*
	  Collateral Damage |R
	  Instant
	  As an additional cost to cast Collateral Damge, sacrifice a creature.
	  Collateral Damage deals 3 damage to target creature or player.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_SAC_CREATURE_AS_COST, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_defiant_ogre(int player, int card, event_t event){
	/*
	  Defiant Ogre |5|R
	  Creature - Ogre Warrior
	  When Defiant Ogre enters the battlefield, choose one -
	  * Put a +1/+1 counter on Defiant Ogre.
	  * Destroy target artifact.
	  3/5
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT );
		td.allow_cancel = 0;

		int choice = do_dialog(player, player, card, -1, -1, " Add a +1/+1 counter\n Kill an artifact", can_target(&td) ? 1 : 0);
		if( choice == 0 ){
			add_1_1_counter(player, card);
		}
		else{
			if( can_target(&td) && new_pick_target(&td, "TARGET_ARTIFACT", 0, 0) ){
				action_on_target(player, card, 0, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int dragonrage_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( instance->damage_target_player > -1 && IS_GAA_EVENT(event) ){
		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_until_eot_merge_previous(instance->damage_target_player, instance->damage_target_card, instance->damage_target_player, instance->damage_target_card, 1, 0);
		}
		return granted_generic_activated_ability(player, card, instance->damage_target_player, instance->damage_target_card, event, 0, MANACOST_R(1),
												0, NULL, NULL);
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}
int card_dragonrage(int player, int card, event_t event){
	/*
	  Dragonrage |2|R
	  Instant
	  Add R to your mana pool for each attacking creature you control. Until end of turn, attacking creatures you control gain "R: This creature gets +1/+0 until end of turn."
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		produce_mana(player, COLOR_RED, count_attackers(player));
		int i;
		for(i=active_cards_count[player]-1; i > -1; i--){
			if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) && is_attacking(player, i) ){
				create_targetted_legacy_activate(player, card, &dragonrage_legacy, player, i);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_fierce_invocation(int player, int card, event_t event){
	/*
	  Fierce Invocation |4|R
	  Sorcery
	  Manifest the top card of your library, then put two +1/+1 counters on it. (To manifest a card, put it onto the battlefield face down as a 2/2 creature. Turn it face up any time for its mana cost if it's a creature card.)
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		int result = manifest(player, card);
		if( result > -1 ){
			add_1_1_counters(player, result, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_flamerush_rider(int player, int card, event_t event){
	/*
	  Flamerush Rider |4|R
	  Creature - Human Warrior
	  Whenever Flamerush Rider attacks, put a token onto the battlefield tapped and attacking that's a copy of another target attacking creature. Exile the token at end of combat.
	  Dash 2RR (You may cast this spell for its dash cost. If you do, it gains haste, and it's returned from the battlefield to its owner's hand at the beginning of the next end step.)
	  3/3
	*/
	dash(player, card, event, MANACOST_XR(2, 2));

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = current_turn;
		td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && new_pick_target(&td, "Select another attacking creature.", 0, GS_LITERAL_PROMPT) ){
			token_generation_t token;
			if( is_token(instance->targets[0].player, instance->targets[0].card) ){
				copy_token_definition(player, card, &token, instance->targets[0].player, instance->targets[0].card);
			}
			else{
				default_token_definition(player, card, get_id(instance->targets[0].player, instance->targets[0].card), &token);
			}
			token.action = TOKEN_ACTION_ATTACKING;
			generate_token(&token);
		}

	}

	return 0;
}

/*
Flamewake Phoenix |1|R|R --> Flameborn Hellion
Creature - Phoenix
Flying, haste
Flamewake Phoenix attacks each turn if able.
Ferocious - At the beginning of combat on your turn, if you control a creature with power 4 or greater, you may pay R. If you do, return Flamewake Phoenix from your graveyard to the battlefield.
2/2
*/

int card_friendly_fire(int player, int card, event_t event){
	/*
	  Friendly Fire |3|R
	  Instant
	  Target creature's controller reveals a card at random from his or her hand.
	  Friendly fire deals damage to that creature and that player equal to the revealed card's converted mana cost.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int rnd = get_random_card_in_hand(instance->targets[0].player);
			if( rnd != -1 ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, get_cmc(instance->targets[0].player, rnd), player, card);
				damage_player(instance->targets[0].player, get_cmc(instance->targets[0].player, rnd), player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_goblin_heelcutter(int player, int card, event_t event){
	/*
	  Goblin Heelcutter |3|R
	  Creature - Goblin Berserker
	  Whenever Goblin Heelcutter attacks, target creature can't block this turn.
	  Dash 2R (You may cast this spell for its dash cost. If you do, it gains haste, and it's returned from the battlefield to its owner's hand at the beginning of the next end step.)
	  3/2
	*/
	dash(player, card, event, MANACOST_XR(2, 1));

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = 1-current_turn;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && new_pick_target(&td, "TARGET_CREATURE", 0, 0) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}

	return 0;
}

/*
Gore Swine |2|R --> vanilla
Creature - Boar
4/1
*/

int card_humble_defector(int player, int card, event_t event){
	/*
	  Humble Defector |1|R
	  Creature - Human Rogue
	  T: Draw two cards. Target opponent gains control of Humble Defector. Activate this ability only during your turn.
	  2/1
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 2);
		give_control_of_self(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_hungering_yeti(int player, int card, event_t event){
	/*
	  Hungering Yeti |4|R
	  Creature - Yeti
	  As long as you control a green or blue permanent, you may cast Hungering Yeti as though it had flash.
	  4/4
	*/
	if( event == EVENT_MODIFY_COST ){
		if( ! can_sorcery_be_played(player, event) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE) | get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
			if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
				infinite_casting_cost();
			}
		}
	}

	return flash(player, card, event);
}

int card_lightning_shrieker(int player, int card, event_t event){
	/*
	  Lightning Shrieker |4|R
	  Creature - Dragon
	  Flying, Trample, Haste
	  At the beginning of the end step, Lightning Shrieker's owner shuffles it into his or her library.
	  5/5
	*/
	haste(player, card, event);

	if( eot_trigger(player, card, event) ){
		int owner = get_owner(player, card);
		put_on_top_of_deck(player, card);
		shuffle(owner);
	}

	return 0;
}

int card_mardu_runemark(int player, int card, event_t event){
	/*
	  Mardu Runemark |2|R
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature gets +2/+2.
	  Enchanted creature has first strike as long as you control a white or black permanent.
	*/
	return runemark(player, card, event, COLOR_TEST_BLACK, COLOR_TEST_WHITE, KEYWORD_FIRST_STRIKE, 0);
}

int card_mardu_scout(int player, int card, event_t event){
	/*
	  Mardu Scout |R|R
	  Creature - Goblin Scout
	  Dash 1R (You may cast this spell for its dash cost. If you do, it gains haste, and it's returned from the battlefield to its owner's hand at the beginning of the next end step.)
	  3/1
	*/
	dash(player, card, event, MANACOST_XR(1, 1));

	return 0;
}

int card_mob_rule(int player, int card, event_t event ){
	/*
	  Mob Rule |4|R|R
	  Sorcery
	  Choose one -
	  * Gain control of all creatures with power 4 or greater until end of turn. Untap those creatures. They gain haste until end of turn.
	  * Gain control of all creatures with power 3 or less until end of turn. Untap those creatures. They gain haste until end of turn.
	  */

	if( event == EVENT_RESOLVE_SPELL ){
		int values[2] = {0, 0};
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.power = 3;
		this_test.power_flag = F5_POWER_GREATER_THAN_VALUE;
		values[0] = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test);
		this_test.power = 4;
		this_test.power_flag = F5_POWER_LESSER_THAN_VALUE;
		values[1] = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test);

		int choice = do_dialog(player, player, card, -1, -1, " Steal crits with POW 4 or more\n Steal crits with POW 3 or less", values[0] >= values[1] ? 0 : 1);

		if( choice == 0 ){
			this_test.power = 3;
			this_test.power_flag = F5_POWER_GREATER_THAN_VALUE;
		}
		else{
			this_test.power = 4;
			this_test.power_flag = F5_POWER_LESSER_THAN_VALUE;
		}
		pump_ability_t pump;
		default_pump_ability_definition(player, card, &pump, 0, 0, 0, SP_KEYWORD_HASTE);
		pump.paue_flags = PAUE_END_AT_EOT | PAUE_UNTAP;
		pump_ability_by_test(player, card, player, &pump, &this_test);
		new_manipulate_all(player, card, 1-player, &this_test, ACT_OF_TREASON);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_outpost_siege(int player, int card, event_t event){
	/*
	  Outpost Siege |3|R
	  Enchantment
	  As Outpost Siege enters the battlefield, choose Khans or Dragons.
	  * Khans - At the beginning of your upkeep, exile the top card of your library. Until end of turn, you may play that card.
	  * Dragons - Whenever a creature you control leaves the battlefield, Outpost Siege deals 1 damage to target creature or player.
	  */

	if( event == EVENT_RESOLVE_SPELL ){
		choose_khans_or_dragons(player, card, 4*count_subtype(player, TYPE_CREATURE, -1), 20 );
	}

	if( is_humiliated(player, card) ){
		return 0;
	}

	if( get_khans_or_dragons(player, card) == KHANS ){
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( deck_ptr[player][0] != -1 ){
			create_may_play_card_from_exile_effect(player, card, player, cards_data[deck_ptr[player][0]].id, MPCFE_UNTIL_EOT);
			rfg_top_card_of_deck(player);
		}
	}

	if( trigger_condition == TRIGGER_LEAVE_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( trigger_cause_controller == player && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
			td.allow_cancel = 0;

			card_instance_t *instance = get_card_instance(player, card);

			instance->number_of_targets = 0;

			if( can_target(&td) ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
							damage_target0(player, card, 1);
						}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_rageform(int player, int card, event_t event){
	/*
	  Rageform |2|R|R
	  Enchantment
	  When Rageform enters the battlefield, it becomes an Aura with enchant creature. Manifest the top card of your library and attach Rageform to it.
	  Enchanted creature has double strike. (It deals both first-strike and regular combat damage.)
	*/
	return manifesting_aura(player, card, event, KEYWORD_DOUBLE_STRIKE, 0);
}

int card_shaman_of_the_great_hunt(int player, int card, event_t event){
	/*
	  Shaman of the Great Hunt |3|R
	  Creature - Orc Shaman
	  Haste
	  Whenever a creature you control deals combat damage to a player, put a +1/+1 counter on it.
	  Ferocious - 2(G/U)(G/U): Draw a card for each creature you control with power 4 or greater.
	  4/2
	*/
	haste(player, card, event);

	if( is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		int p = player;

		card_instance_t* damage = combat_damage_being_dealt(event);
		if (damage &&
			damage->damage_target_player == p &&
			damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER) &&
			instance->info_slot < 40	// targets[1] through targets[10], 8 bytes each; 2 bytes per creature
		   ){
			unsigned char* creatures = (unsigned char*)(&instance->targets[1].player);
			creatures[2 * instance->info_slot] = damage->damage_source_player;
			creatures[2 * instance->info_slot + 1] = damage->damage_source_card;
			instance->info_slot++;
		}

		if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == player && instance->info_slot > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					unsigned char* creatures = (unsigned char*)(&instance->targets[1].player);
					int i;
					for (i = 0; i < instance->info_slot; i++){
						add_1_1_counter(creatures[2 * i], creatures[2 * i + 1]);
					}
					instance->info_slot = 0;
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE && ferocious(player, card) ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XU(2, 2));
			if( has_mana_hybrid(player, 2, COLOR_BLUE, COLOR_GREEN, c1) ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XU(2, 2));
		charge_mana_hybrid(player, card, 2, COLOR_BLUE, COLOR_GREEN, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.power = 3;
		this_test.power_flag = F5_POWER_GREATER_THAN_VALUE;
		draw_cards(player, check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test));
	}

	return 0;
}

int card_shockmaw_dragon(int player, int card, event_t event ){
	/*
	  Shockmaw Dragon |4|R|R
	  Creature - Dragon
	  Flying
	  Whenever Shockmaw Dragon deals combat damage to a player, it deals 1 damage to each creature that player controls.
	  4/4
	*/
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER | DDBM_TRACE_DAMAGED_PLAYERS) ){
		if( BYTE0(get_card_instance(player, card)->targets[1].player) ){
			new_damage_all(player, card, 0, 1, NDA_ALL_CREATURES, NULL);
		}
		if( BYTE1(get_card_instance(player, card)->targets[1].player) ){
			new_damage_all(player, card, 1, 1, NDA_ALL_CREATURES, NULL);
		}
	}

	return 0;
}

int card_smoldering_efreet(int player, int card, event_t event ){
	/*
	  Smoldering Efreet |1|R
	  Creature - Efreet Monk
	  When Smoldering Efreet dies, it deals 2 damage to you.
	  2/2
	*/
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		damage_player(player, 2, player, card);
	}

	return 0;
}

int card_temur_battle_rage(int player, int card, event_t event ){
	/*
	  Temur Battle Rage |1|R
	  Instant
	  Target creature gains double strike until end of turn.
	  Ferocious - That creature also gains trample until end of turn if you control a creature with power 4 or greater.
	*/
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 0, 0, KEYWORD_DOUBLE_STRIKE | (ferocious(player, card) ? KEYWORD_TRAMPLE : 0), 0);
}

int card_vaultbreaker(int player, int card, event_t event){
	/*
	  Vaultbreaker |3|R
	  Creature - Orc Rogue
	  Whenever Vaultbreaker attacks, you may discard a card. If you do, draw a card.
	  Dash 2R
	  4/2
	*/
	dash(player, card, event, MANACOST_XR(2, 1));

	if( declare_attackers_trigger(player, card, event, hand_count[player] ? RESOLVE_TRIGGER_AI(player) : 0, player, card) ){
		discard(player, 0, player);
		draw_cards(player, 1);
	}

	return 0;
}

int card_wild_slash(int player, int card, event_t event){
	/*
	  Wild Slash |R
	  Instant
	  Ferocious - If you control a creature with power 4 or greater, damage can't be prevented this turn.
	  Wild Slash deals 2 damage to target creature or player.
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
			if( ferocious(player, card) ){
				damage_cannot_be_prevented_until_eot(player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

// Green
int card_abzan_beastmaster(int player, int card, event_t event){
	/*
	  Abzan Beastmaster |2|G
	  Creature - Hound Shaman
	  At the beginning of your upkeep, draw a card if you control the creature with the greatest toughness or tied for the greatest toughness.
	  2/1
	*/
	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && reason_for_trigger_controller == player && current_turn == player ){
		int max_t[2] = {0, 0};
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		APNAP(p, {max_t[p] =check_battlefield_for_special_card(player, card, p, CBFSC_GET_MAX_TOU, &this_test);};);
		upkeep_trigger_ability_mode(player, card, event, player, max_t[player] >= max_t[1-player] ? RESOLVE_TRIGGER_MANDATORY : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_abzan_kin_guard(int player, int card, event_t event){
	/*
	  Abzan Kin-Guard |3|G
	  Creature - Human Warrior
	  Abzan Kin-Guard has lifelink as long as you control a white or black permanent.
	  3/3
	*/
	if( ! is_humiliated(player, card) ){
		if( event == EVENT_ABILITIES && affect_me(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK) | get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
				lifelink(player, card, event);
			}
		}
	}
	return 0;
}

int card_ainok_guide(int player, int card, event_t event){
	/*
	  Ainok Guide |1|G
	  Creature - Hound Scout
	  When Ainok Guide enters the battlefield, choose one -
	  * Put a +1/+1 counter on Ainok Guide.
	  * Search your library for a basic land card, reveal it, then shuffle your library and put that card on top of it.
	  1/1
	*/
	if( comes_into_play(player, card, event) ){
		int choice = do_dialog(player, player, card, -1, -1, " Add a +1/+1 counter\n Tutor land", count_subtype(player, TYPE_LAND, -1) > 5 ? 0 : 1);
		if( choice == 0 ){
			add_1_1_counter(player, card);
		}
		else{
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
			this_test.subtype = SUBTYPE_BASIC;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, AI_FIRST_FOUND, &this_test);
		}
	}

	return 0;
}

int card_ambush_krotiq(int player, int card, event_t event){
	/*
	  Ambush Krotiq |5|G
	  Creature - Insect
	  Trample
	  When Ambush Krotiq enters the battlefield, return another creature you control to its owner's hand.
	  5/5
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.special = TARGET_SPECIAL_NOT_ME;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && new_pick_target(&td, "Select another creature you control.", 0, GS_LITERAL_PROMPT) ){
			action_on_target(player, card, 0, ACT_BOUNCE);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_arashin_war_beast(int player, int card, event_t event){
	/*
	  Arashin War Beast |5|G|G
	  Creature - Beast
	  Whenever Arashin War Beast deals combat damage to one or more blocking creatures, manifest the top card of your library.
	  6/6
	*/
	if( ! is_humiliated(player, card) ){
		card_instance_t* instance = get_card_instance(player, card);
		if (event == EVENT_DEAL_DAMAGE){
			card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
			if (damage->internal_card_id == damage_card
				&& damage->damage_source_card == card
				&& damage->damage_source_player == player
				&& damage->info_slot > 0
				&& damage->damage_target_card != -1
				&& (damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE))
			   ){
				if( check_state(damage->damage_target_player, damage->damage_target_card, STATE_BLOCKING) ){
					if( instance->targets[1].player < 0 ){
						instance->targets[1].player = 0;
					}
					instance->targets[1].player++;
				}
			}
		}

		if( instance->targets[1].player > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == player ){

			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int i;
					for(i=0; i<instance->targets[1].player; i++){
						manifest(player, card);
					}
			}
		}
	}

	return 0;
}

/*Archers of Qarsi |3|G --> vanilla
Creature - Naga Archer
Defender
Reach
5/2
*/

int card_battlefront_krushok(int player, int card, event_t event){
	/*
	  Battlefront Krushok |4|G
	  Creature - Beast
	  Battlefront Krushok can't be blocked by more than one creature.
	  Each creature you control with a +1/+1 counter on it can't be blocked by more than one creature.
	  3/4
	*/

	if( event == EVENT_BLOCK_LEGALITY && attacking_card_controller == player && ! is_humiliated(player, card) ){
		if( attacking_card == card || count_1_1_counters(attacking_card_controller, attacking_card) > 0 ){
			if( count_my_blockers(attacking_card_controller, attacking_card) > 1 ){
				event_result = 1;
			}
		}
	}
	return 0;
}

int card_cached_defenses(int player, int card, event_t event){
	/*
	  Cached Defenses |2|G
	  Sorcery
	  Bolster 3.
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		bolster(player, card, 3);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_destructor_dragon(int player, int card, event_t event){
	/*
	  Destructor Dragon |4|G|G
	  Creature - Dragon
	  Flying
	  When Destructor Dragon dies, destroy target noncreature permanent.
	  4/4
	*/
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_CREATURE;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && new_pick_target(&td, "Select target noncreature permanent.", 0, GS_LITERAL_PROMPT) ){
			action_on_target(player, card, 0, KILL_DESTROY);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

/*
Feral Krushok |4|G --> vanilla
Creature - Beast
5/4
*/

int card_formless_nurturing(int player, int card, event_t event){
	/*
	  Formless Nurturing |3|G
	  Sorcery
	  Manifest the top card of your library, then put a +1/+1 counter on it. (To manifest a card, put it onto the battlefield face down as a 2/2 creature. Turn it face up any time for its mana cost if it's a creature card.)
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		int result = manifest(player, card);
		if( result > -1 ){
			add_1_1_counter(player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_frontier_mastodon(int player, int card, event_t event){
	/*
	  Frontier Mastodon |2|G
	  Creature - Elephant
	  Ferocious - Frontier Mastodon enters the battlefield with a +1/+1 counter on it if you control a creature with power 4 or greater.
	  3/2
	*/
	if((event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card)){
		if( ferocious(player, card) ){
			enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 1);
		}
	}
	return 0;
}

int card_frontier_siege(int player, int card, event_t event){
	/*
	  Frontier Siege |3|G
	  Enchantment
	  As Frontier Siege enters the battlefield, choose Khans or Dragons.
	  * Khans - At the beginning of each of your main phases, add GG to your mana pool.
	  * Dragons - Whenever a creature with flying enters the battlefield under your control, you may have it fight target creature you don't control.
	  */
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[2].player = choose_khans_or_dragons_no_storage(player, card, 20, 4*count_subtype(player, TYPE_LAND, -1));
	}

	if( is_humiliated(player, card) ){
		return 0;
	}

	if( current_turn == player && current_phase == PHASE_MAIN1 && instance->targets[2].player == KHANS &&
		instance->targets[2].card != 66
	  ){
		produce_mana(player, COLOR_GREEN, 2);
		instance->targets[2].card = 66;
	}

	if( instance->targets[2].player == DRAGONS && trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = 1-player;
		td.allowed_controller = 1- player;

		instance->number_of_targets = 0;

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;

		if( new_specific_cip(player, card, event, player, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0, &this_test) ){
			if( new_pick_target(&td, "Select target creature your opponent controls.", 0, GS_LITERAL_PROMPT) ){
				fight(instance->targets[1].player, instance->targets[1].card, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[2].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_fruit_of_the_first_tree(int player, int card, event_t event){
	/*
	  Fruit of the First Tree |3|G
	  Enchantment - Aura
	  Enchant Creature
	  When enchanted creature dies, you gain X life and draw X cards, where X is its toughness.
	*/
	if( attached_creature_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t* inst = get_card_instance(player, card);
		int amount = get_toughness(inst->damage_target_player, inst->damage_target_card);
		gain_life(player, amount);
		draw_cards(player, amount);
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_map_the_wastes(int player, int card, event_t event){
	/*
	  Map the Wastes |2|G
	  Sorcery
	  Search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library.
	  Bolster 1.
	*/

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
		this_test.subtype = SUBTYPE_BASIC;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &this_test);
		bolster(player, card, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

static const char* is_artifact_or_enchantment_or_flying_creature(int who_chooses, int player, int card)
{
	if( is_what(player, card, TYPE_ARTIFACT | TYPE_ENCHANTMENT) ){
		return NULL;
	}
	if( is_what(player, card, TYPE_CREATURE) && check_for_ability(player, card, KEYWORD_FLYING) ){
		return NULL;
	}
	return "must be an artifact, enchantment, or creature with flying.";
}

int card_return_to_the_earth(int player, int card, event_t event ){
	/*
	  Return to the Earth |3|G
	  Instant
	  Destroy target artifact, enchantment, or creature with flying.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.extra = (int32_t)is_artifact_or_enchantment_or_flying_creature;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target artifact, enchantment, or creature with flying.", 1, NULL);
}

int card_ruthless_instincts(int player, int card, event_t event){
	/*
	  Ruthless Instincts |2|G
	  Instant
	  Choose one -
	  * Target nonattacking creature gains reach and deathtouch until end of turn. Untap it.
	  * Target attacking creature gets +2/+2 and gains trample until end of turn.
	  */
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}


	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_ATTACKING;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
			return 1;
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td1, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = instance->info_slot = instance->number_of_targets = 0;
		if( can_target(&td) ){
			if( can_target(&td1) ){
				int ai_choice = current_turn == player && current_turn == player ? 0 : 1;
				choice = do_dialog(player, player, card, -1, -1," Pump non-attacker\n Pump attacker\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}
		instance->info_slot = 1+choice;
		if( choice == 1 ){
			new_pick_target(&td, "Select target nonattacking creature.", 0, 1 | GS_LITERAL_PROMPT);
		}
		else if( choice == 2 ){
				new_pick_target(&td, "Select target attacking creature.", 0, 1 | GS_LITERAL_PROMPT);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot == 1 && valid_target(&td)) || (instance->info_slot == 2 && valid_target(&td1)) ){
			pump_ability_t pump;
			if( instance->info_slot == 1 ){
				default_pump_ability_definition(player, card, &pump, 0, 0, KEYWORD_REACH, SP_KEYWORD_DEATHTOUCH);
				pump.paue_flags = PAUE_END_AT_EOT | PAUE_UNTAP;
			}
			else{
				default_pump_ability_definition(player, card, &pump, 2, 2, KEYWORD_TRAMPLE, 0);
				pump.paue_flags = PAUE_END_AT_EOT;
			}
			pump_ability(player, card, instance->targets[0].player, instance->targets[0].card, &pump);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sandsteppe_mastodon(int player, int card, event_t event){
	/*
	  Sandsteppe Mastodon |5|G|G
	  Creature - Elephant
	  Reach
	  When Sandsteppe Mastodon enters the battlefield, bolster 5.
	  (Choose a creature with the least toughness or tied with the least toughness among creatures you control. Put 5 +1/+1 counters on it.)
	  5/5
	*/
	if( comes_into_play(player, card, event) ){
		bolster(player, card, 5);
	}

	return 0;
}

int card_shamanic_revelation(int player, int card, event_t event){
	/*
	  Shamanic Revelation |3|G|G
	  Sorcery
	  Draw a card for each creature you control.
	  Ferocious - You gain 4 life for each creature you control with power 4 or greater.
	*/

	if(event == EVENT_RESOLVE_SPELL ){
		int amount = count_subtype(player, TYPE_CREATURE, -1);
		draw_cards(player, amount);
		gain_life(player, 4*ferocious(player, card));
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_sudden_reclamation(int player, int card, event_t event){
	/*
	  Sudden Reclamation |3|G
	  Instant
	  Put the top four cards of your library into your graveyard, then return a creature card and a land card from your graveyard to your hand.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		mill(player, 4);
		test_definition_t test;
		if( count_graveyard_by_type(player, TYPE_CREATURE) ){
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		}
		if( count_graveyard_by_type(player, TYPE_LAND) ){
			new_default_test_definition(&test, TYPE_LAND, "Select a land card.");
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_temur_runemark(int player, int card, event_t event){
	/*
	  Temur Runemark |2|G
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature gets +2/+2.
	  Enchanted creature has trample as long as you control a blue or red permanent.
	*/
	return runemark(player, card, event, COLOR_TEST_BLUE, COLOR_TEST_RED, KEYWORD_TRAMPLE, 0);
}


int card_temur_sabretooth(int player, int card, event_t event){
	/*
	  Temur Sabertooth |2|G|G
	  Creature - Cat
	  1G: You may return another creature you control to its owner's hand. If you do, Temur Sabertooth gains indestructible until end of turn.
	  4/3
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.special = TARGET_SPECIAL_NOT_ME;

		instance->number_of_targets = 0;

		if( can_target(&td) && new_pick_target(&td, "Select another creature you control.", 0, GS_LITERAL_PROMPT) ){
			action_on_target(player, card, 0, ACT_BOUNCE);
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
									0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
		}
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XG(1, 1), 0, NULL, NULL);
}

int card_temur_war_shaman(int player, int card, event_t event){
	/*
	  Temur War Shaman |4|G|G
	  Creature - Human Shaman
	  When Temur War Shaman enters the battlefield, manifest the top card of your library.
	  (Put that card onto the battlefield face down as a 2/2 creature. You may turn it face up at any time for its mana cost if it is a creature card.)
	  Whenever a permanent you control is turned face up, if it is a creature, you may have it fight target creature you don't control.
	  4/5
	*/
	if( comes_into_play(player, card, event) ){
		manifest(player, card);
	}

	return 0;
}

int card_warden_of_the_first_tree(int player, int card, event_t event){
	/*
	  Warden of the First Tree |G
	  Creature - Human
	  1(W/B): Warden of the First Tree becomes a Human Warrior with base power and toughness 3/3.
	  2(W/B)(W/B): If Warden of the First Tree is a Warrior, it becomes a Human Spirit Warrior with trample and lifelink.
	  3(W/B)(W/B)(W/B): If Warden of the First Tree is a Spirit, put five +1/+1 counters on it.
	  1/1
	*/
	card_instance_t *instance = get_card_instance( player, card);
	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[9].player = 0;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XB(1, 1));
		return has_mana_hybrid(player, 1, COLOR_BLACK, COLOR_WHITE, c1);
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int abilities[3] = {0, 0, 0};
		int i;
		for(i=0; i<3; i++){
			int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XB(i+1, i+1));
			abilities[i] = has_mana_hybrid(player, i+1, COLOR_BLACK, COLOR_WHITE, c1);
		}
		int priorities[3] = { 	! has_subtype(player, card, SUBTYPE_WARRIOR) ? 10 : -50,
								has_subtype(player, card, SUBTYPE_WARRIOR) ? 15 : -50,
								has_subtype(player, card, SUBTYPE_SPIRIT) ? 20 : -50,
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"3/3 Human Warrior", abilities[0], priorities[0],
						"Give Trample & Lifelink", abilities[1], priorities[1],
						"Add 5 +1/+1 counters", abilities[2], priorities[2]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		int c1 = get_cost_mod_for_activated_abilities(player, card, MANACOST_XB(choice, 1));
		charge_mana_hybrid(player, card, choice, COLOR_BLACK, COLOR_WHITE, c1);
		if( spell_fizzled != 1 ){
			instance->info_slot = choice;
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card  );
		if( instance->info_slot == 1 ){
			parent->targets[9].player = instance->info_slot;
			int legacy = real_set_pt(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 3, 3, 0);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
		if( instance->info_slot == 2 && has_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_WARRIOR) ){
			parent->targets[9].player = instance->info_slot;
		}
		if( instance->info_slot == 3 && has_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_SPIRIT) ){
			parent->targets[9].player = instance->info_slot;
			add_1_1_counters(instance->parent_controller, instance->parent_card, 5);
		}
	}

	if( ! is_humiliated(player, card) ){
		if( instance->targets[9].player > -1 && (instance->targets[9].player & 2) ){
			lifelink(player, card, event);
			if(event == EVENT_ABILITIES && affect_me(player, card ) ){
				event_result |= KEYWORD_TRAMPLE;
			}
		}
	}
	return 0;
}

int card_whisperer_of_the_wilds(int player, int card, event_t event){
	/*
	  Whisperer of the Wilds |1|G
	  Creature - Human Shaman
	  T: Add G to your mana pool.
	  Ferocious - T: Add GG to your mana pool. Activate this ability only if you control a creature with power 4 or greater.
	  0/2
	*/
	if( event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		produce_mana(player, COLOR_GREEN, ferocious(player, card) > 0 ? 2 : 1);
		tap_card(player, card);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
		declare_mana_available(player, COLOR_GREEN, ferocious(player, card) > 0 ? 2 : 1);
	}

	return 0;
}

static int whisperwood_elemental_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card);
	if( instance->damage_target_player > -1 ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			if( get_card_instance(instance->damage_target_player, instance->damage_target_card)->kill_code > 0 &&
				get_card_instance(instance->damage_target_player, instance->damage_target_card)->kill_code < KILL_REMOVE
			  ){
				instance->damage_target_player = instance->damage_target_card = -1;
				instance->targets[11].player = 66;
			}
		}
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		manifest(player, card);
		kill_card(player, card, KILL_REMOVE);
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_whisperwood_elemental(int player, int card, event_t event){
	/*
	  Whisperwood Elemental |3|G|G
	  Creature - Elemental
	  At the beginning of your end step, manifest the top card of your library.
	  (Put it onto the battlefield face down as a 2/2 creature. Turn it face up any time for its mana cost if it's a creature card)
	  Sacrifice Whisperwood Elemental: Until end of turn, face-up, nontoken creatures you control gain
	  "When this creature dies, manifest the top card of your library."
	  4/4
	*/
	if( eot_trigger(player, card, event) ){
		manifest(player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) && ! is_token(player, count) &&
					get_id(player, count) != CARD_ID_FACE_DOWN_CREATURE
				 ){
					create_targetted_legacy_effect(player, card, &whisperwood_elemental_legacy, player, count);
				}
				count--;
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}

int card_wildcall(int player, int card, event_t event){
	/*
	  Wildcall |X|G|G
	  Sorcery
	  Manifest the top card of your library, then put X +1/+1 counters on it.
	  (To manifest a card, put it onto the battlefield face down as a 2/2 creature. Turn it face up at any time for its mana cost if it's a creature card.)
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		int result = manifest(player, card);
		if( result > -1 ){
			add_1_1_counters(player, card, get_card_instance(player, card)->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_winds_of_qal_sisma(int player, int card, event_t event){
	/*
	  Winds of Qal Sisma |1|G
	  Instant
	  Prevent all combat damage that would be dealt this turn.
	  Ferocious - If you control a creature with power 4 or greater,
	  instead prevent all combat damage that would be dealt this turn by creatures your opponents control.
	*/
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( current_turn == player || current_phase != PHASE_AFTER_BLOCKING || count_attackers(current_turn) < 1 ){
			ai_modifier-=25;
		}
	}


	if( event == EVENT_RESOLVE_SPELL ){
		fog_special(player, card, ferocious(player, card) > 0 ? 1-player : ANYBODY, FOG_COMBAT_DAMAGE_ONLY);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}


int card_yasova_dragonclaw(int player, int card, event_t event){
	/*
	  Yasova Dragonclaw |2|G
	  Legendary Creature - Human Warrior
	  Trample
	  At the beginning of combat on your turn, you may pay 1(U/R)(U/R).
	  If you do, gain control of target creature an opponent controls with power less than Yasova Dragonclaw's power until end of turn,
	  untap that creature, and it gains haste until end of turn.
	  4/2
	*/
	if( beginning_of_combat(player, card, event, player, -1) && has_mana_hybrid(player, 2, COLOR_BLUE, COLOR_RED, 1) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);
		instance->info_slot = 0;

		if( can_target(&td) ){
			int choice = do_dialog(player, player, card, -1, -1, " Stole a creature\n Pass", 0);
			if (choice == 0){
				charge_mana_hybrid(player, card, 2, COLOR_BLUE, COLOR_RED, 1);
				if( spell_fizzled != 1 && new_pick_target(&td, "Select target creature opponent controls.", 0, GS_LITERAL_PROMPT) ){
					effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
				}
			}
		}
	}

	return 0;
}


// Gold
int card_atarka_world_render(int player, int card, event_t event)
{
	/*
	  Atarka, World Render |5|R|G
	  Legendary Creature - Dragon
	  Flying, trample
	  Whenever a Dragon you control attacks, it gains double strike until end of turn.
	  6/4
	*/
	check_legend_rule(player, card, event);

	if( event == EVENT_DECLARE_ATTACKERS ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_DRAGON;
		store_attackers(player, card, event, DAT_TRACK, player, -1, &this_test);
	}

	int amt;
	if((amt = resolve_declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))){
		card_instance_t* instance = get_card_instance(player, card);
		unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		for (--amt; amt >= 0; --amt){
			if (in_play(current_turn, attackers[amt])){
				pump_ability_until_eot(player, card, current_turn, attackers[amt], 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
			}
		}
	}

	return 0;
}

int card_cunning_strike(int player, int card, event_t event){
	/*
	  Cunning Strike |3|U|R
	  Instant
	  Cunning Strike deals 2 damage to target creature and 2 damage to target player.
	  Draw a card.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
			return can_target(&td1);
		}
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			new_pick_target(&td1, "TARGET_PLAYER", 1, 1);
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		int valid = 0;
		if( valid_target(&td) ){
			damage_target0(player, card, 2);
			valid++;
		}
		if( validate_target(player, card, &td1, 1) ){
			damage_player(instance->targets[1].player, 2, player, card);
			valid++;
		}
		if( valid ){
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dromoka_the_eternal(int player, int card, event_t event){
	/*
	  Dromoka, the Eternal |3|G|W
	  Legendary Creature - Dragon
	  Flying
	  Whenever a Dragon you control attacks, bolster 2. (Put 2 +1/+1 counters on a creature of your choice with the lowest toughness among creatures you control.)
	  5/5
	*/
	check_legend_rule(player, card, event);

	if( event == EVENT_DECLARE_ATTACKERS ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_DRAGON;
		store_attackers(player, card, event, DAT_SEPARATE_TRIGGERS, player, -1, &this_test);
	}

	if( resolve_declare_attackers_trigger(player, card, event, DAT_SEPARATE_TRIGGERS) ){
		bolster(player, card, 2);
	}

	return 0;
}

int card_ethereal_ambush(int player, int card, event_t event){
	/*
	  Ethereal Ambush |3|G|U
	  Instant
	  Manifest the top two cards of your library.
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		manifest(player, card);
		manifest(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_grim_contest(int player, int card, event_t event){
	/*
	  Grim Contest |1|B|G
	  Instant
	  Choose target creature you control and target creature an opponent controls. Each of those creatures deals damage equal to its toughness to the other.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
			return can_target(&td1);
		}
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
			new_pick_target(&td1, "Select target creature your opponent controls.", 1, 1 | GS_LITERAL_PROMPT);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td1, 1) ){
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->regen_status |= KEYWORD_RECALC_TOUGHNESS;
			int pow1 = get_abilities(instance->targets[0].player, instance->targets[0].card, EVENT_TOUGHNESS, -1);

			get_card_instance(instance->targets[1].player, instance->targets[1].card)->regen_status |= KEYWORD_RECALC_TOUGHNESS;
			int pow2 = get_abilities(instance->targets[1].player, instance->targets[1].card, EVENT_TOUGHNESS, -1);

			play_sound_effect(WAV_ASWANJAG);

			damage_creature(instance->targets[1].player, instance->targets[1].card, pow1, instance->targets[0].player, instance->targets[0].card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, pow2, instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_harsh_sustenance(int player, int card, event_t event){
	/*
	  Harsh Sustenance |1|W|B
	  Instant
	  Harsh Sustenance deals X damage to target creature or player and you gain X life, where X is the number of creatures you control.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = count_subtype(player, TYPE_CREATURE, -1);
			damage_target0(player, card, amount);
			gain_life(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_kologhan_the_storms_fury(int player, int card, event_t event)
{
	/*
	  Kolaghan, the Storm's Fury |3|B|R
	  Legendary Creature - Dragon
	  Flying
	  Whenever a Dragon you control attacks, creatures you control get +1/+0 until end of turn.
	  Dash 3BR (You may cast this spell for its dash cost. If you do, it gains haste, and it's returned from the battlefield to its owner's hand at the beginning of the next end step.)
	  4/5
	*/
	check_legend_rule(player, card, event);

	dash(player, card, event, MANACOST_XBR(3, 1, 1));

	if( event == EVENT_DECLARE_ATTACKERS ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_DRAGON;
		store_attackers(player, card, event, DAT_SEPARATE_TRIGGERS, player, -1, &this_test);
	}

	if( resolve_declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		pump_subtype_until_eot(player, card, player, -1, 1, 0, 0, 0);
	}

	return 0;
}

int card_ojutai_soul_of_winter(int player, int card, event_t event)
{
	/*
	  Ojutai, Soul of Winter |5|W|U
	  Legendary Creature - Dragon
	  Flying, vigilance
	  Whenever a Dragon you control attacks, tap target nonland permanent an opponent controls. It doesn't untap during its controller's next untap step.
	  5/6
	*/
	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	if( event == EVENT_DECLARE_ATTACKERS ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_DRAGON;
		store_attackers(player, card, event, DAT_SEPARATE_TRIGGERS, player, -1, &this_test);
	}

	if( resolve_declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && new_pick_target(&td, "Select target nonland permanent your opponent controls.", 0, GS_LITERAL_PROMPT) ){
			effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_silumgar_the_drifting_death(int player, int card, event_t event)
{
	/*
	  Silumgar, the Drifting Death |4|U|B
	  Legendary Creature - Dragon
	  Flying, hexproof
	  Whenever a Dragon you control attacks, each creature defending player controls get -1/-1 until end of turn.
	  3/7
	*/
	check_legend_rule(player, card, event);

	hexproof(player, card, event);

	if( event == EVENT_DECLARE_ATTACKERS ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_DRAGON;
		store_attackers(player, card, event, DAT_SEPARATE_TRIGGERS, player, -1, &this_test);
	}

	if( resolve_declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		pump_subtype_until_eot(player, card, 1-player, -1, -1, -1, 0, 0);
	}

	return 0;
}

int card_war_flare(int player, int card, event_t event)
{
	/*
	  War Flare |2|R|W
	  Instant
	  Creatures you control get +2/+1 until end of turn. Untap those creatures.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		pump_ability_t pump;
		default_pump_ability_definition(player, card, &pump, 2, 1, 0, 0);
		pump.paue_flags = PAUE_END_AT_EOT | PAUE_UNTAP;
		pump_ability_by_test(player, card, player, &pump, NULL);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


//Artifacts
int card_goblin_boom_keg(int player, int card, event_t event){
	/*
	  Goblin Boom Keg |4
	  Artifact
	  At the beginning of your upkeep, sacrifice Goblin Boom Keg.
	  When Goblin Boom Keg is put into a graveyard from the battlefield, it deals 3 damage to target creature or player.
	*/
	if( current_turn == player && upkeep_trigger(player, card, event) ){
		if( ! check_special_flags2(player, card, SF2_CANNOT_BE_SACRIFICED) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_target0(player, card, 3);
		}
	}

	return 0;
}

int card_heros_blade(int player, int card, event_t event){
	/*
	  Hero's Blade |2
	  Artifact - Equipment
	  Equipped creature gets +3/+2.
	  Whenever a legendary creature enters the battlefield under your control, you may attach Hero's Blade to it.
	  Equip 4
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_CREATURE, 0, SUBTYPE_LEGEND, 0, 0, 0, 0, 0, -1, 0) ){
		equip_target_creature(player, card, instance->targets[1].player, instance->targets[1].card);
	}

	return vanilla_equipment(player, card, event, 4, 3, 2, 0, 0);
}

int card_hewer_stone_retainer(int player, int card, event_t event){
	/*
	  Hewed Stone Retainers |3
	  Artifact Creature - Golem
	  Cast Hewed-Stone Retainers only if you've cast another spell this turn.
	  4/4
	*/
	if( event == EVENT_MODIFY_COST && get_specific_storm_count(player) < 1 ){
		infinite_casting_cost();
	}

	return 0;
}

/*
Pilgrim of the Fires |7 --> vanilla
Artifact Creature - Golem
First strike, trample
6/4
*/

int card_scroll_of_the_masters(int player, int card, event_t event){
	/*
	  Scroll of the Masters |2
	  Artifact
	  Whenever you cast a noncreature spell, put a lore counter on Scroll of the Masters.
	  3,T: Target creature you control gets +1/+1 until end of turn for each lore counter on Scroll of the Masters.
	*/

	if( prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_counter(player, card, COUNTER_LORE);
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = count_counters(player, card, COUNTER_LORE);
	}

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
						instance->info_slot, instance->info_slot);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_LITERAL_PROMPT, MANACOST_X(3), 0,
									&td, "Select target creature you control.");
}

int card_ugins_contruct(int player, int card, event_t event){
	/*
	  Ugin's Construct |4
	  Artifact Creature - Construct
	  When Ugin's Construct enters the battlefield, sacrifice a permanent that's one or more colors.
	  4/5
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.required_color = COLOR_TEST_ANY_COLORED;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && new_pick_target(&td, "Select a permanent that's one or more colors that you control.", 0, GS_LITERAL_PROMPT) ){
			action_on_target(player, card, 0, KILL_SACRIFICE);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

// Lands
/*
Crucible of the Spirit Dragon --> Impossible
Land
T: Add 1 to your mana pool.
1, T: Put a storage counter on Crucible of the Spirit Dragon.
T, Remove X storage counters from Crucible of the Spirit Dragon: Add X mana in any combination of colors to your mana pool. Spend this mana only to cast Dragon spells or activate abilities of Dragons.
*/
