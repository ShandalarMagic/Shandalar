#include "manalink.h"

// Functions
static int bloodfire(int player, int card, event_t event, int dmg){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
		if( spell_fizzled != 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_damage_all(player, card, 2, dmg, NDA_PLAYER_TOO, &this_test);
	}

	return 0;
}

static int recruiter(int player, int card, event_t event, int subtype){

	/* Creature
	 * When ~ enters the battlefield, reveal the top four cards of your library. Put all [subtype] cards revealed this way into your hand and the rest on the
	 * bottom of your library in any order. */

	if( comes_into_play(player, card, event) ){
		int amount = reveal_top_cards_of_library(player, 4);
		if (amount <= 0){
			return 0;
		}

		int i, *deck = deck_ptr[player];
		for (i = amount - 1; i >= 0; --i){
			if( has_subtype(-1, deck[i], subtype) ){
				add_card_to_hand(player, deck[i]);
				remove_card_from_deck(player, i);
				amount--;
			}
		}
		if( amount > 0 ){
			put_top_x_on_bottom(player, player, amount);
		}
	}
	return 0;
}

static void phyrexians(int player, int card, event_t event, int amount){

	if( comes_into_play(player, card, event) ){
	   lose_life(player, amount);
	   draw_cards(player, amount);
	}

}

void volver(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white,
			int colorless2, int black2, int blue2, int green2, int red2, int white2, int kicker1_priority, int kicker2_priority
  ){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			int options[10] = { black > 0 && has_mana_multi(player, colorless, black, 0, 0, 0, 0),
							black2 > 0 && has_mana_multi(player, colorless, black2, 0, 0, 0, 0),
							blue > 0 && has_mana_multi(player, colorless, 0, blue, 0, 0, 0),
							blue2 > 0 && has_mana_multi(player, colorless, 0, blue2, 0, 0, 0),
							green > 0 && has_mana_multi(player, colorless, 0, 0, green, 0, 0),
							green2 > 0 && has_mana_multi(player, colorless, 0, 0, green2, 0, 0),
							red > 0 && has_mana_multi(player, colorless, 0, 0, 0, red, 0),
							red2 > 0 && has_mana_multi(player, colorless, 0, 0, 0, red2, 0),
							white > 0 && has_mana_multi(player, colorless, 0, 0, 0, 0, white),
							white2 > 0 && has_mana_multi(player, colorless, 0, 0, 0, 0, white2)
			};

			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_OMIT_ILLEGAL,
						"Pay black kicker", options[0], kicker1_priority,
						"Pay black kicker", options[1], kicker2_priority,
						"Pay blue kicker", options[2], kicker1_priority,
						"Pay blue kicker", options[3], kicker2_priority,
						"Pay green kicker", options[4], kicker1_priority,
						"Pay green kicker", options[5], kicker2_priority,
						"Pay red kicker", options[6], kicker1_priority,
						"Pay red kicker", options[7], kicker2_priority,
						"Pay white kicker", options[8], kicker1_priority,
						"Pay white kicker", options[9], kicker2_priority,
						"Pay both", has_mana_multi(player, colorless+colorless2, black+black2, blue+blue2, green+green2, red+red2, white+white2), kicker1_priority+kicker2_priority);
			if( choice ){
				int mana_to_charge[6];
				if( choice == 11 ){
					mana_to_charge[0] = colorless+colorless2;
				}
				else{
					mana_to_charge[0] = choice == 1 || choice == 3 || choice == 5 || choice == 7 || choice == 9 ? colorless : colorless2;
				}
				if( choice == 11 ){
					mana_to_charge[1] = black+black2;
				}
				else{
					mana_to_charge[1] = choice == 1 || choice == 3 || choice == 5 || choice == 7 || choice == 9 ? black : black2;
				}
				if( choice == 11 ){
					mana_to_charge[2] = blue+blue2;
				}
				else{
					mana_to_charge[2] = choice == 1 || choice == 3 || choice == 5 || choice == 7 || choice == 9 ? blue : blue2;
				}
				if( choice == 11 ){
					mana_to_charge[3] = green+green2;
				}
				else{
					mana_to_charge[3] = choice == 1 || choice == 3 || choice == 5 || choice == 7 || choice == 9 ? green : green2;
				}
				if( choice == 11 ){
					mana_to_charge[4] = red+red2;
				}
				else{
					mana_to_charge[4] = choice == 1 || choice == 3 || choice == 5 || choice == 7 || choice == 9 ? red : red2;
				}
				if( choice == 11 ){
					mana_to_charge[5] = white+white2;
				}
				else{
					mana_to_charge[5] = choice == 1 || choice == 3 || choice == 5 || choice == 7 || choice == 9 ? white : white2;
				}
				charge_mana_multi(player, mana_to_charge[0], mana_to_charge[1], mana_to_charge[2], mana_to_charge[3], mana_to_charge[4], mana_to_charge[5]);
				if( spell_fizzled != 1 ){
					if( choice == 11 ){
						set_special_flags(player, card, SF_KICKED | SF_KICKED2);
					}
					else{
						int sf = (choice == 1 || choice == 3 || choice == 5 || choice == 7 || choice == 9) ? SF_KICKED : SF_KICKED2;
						set_special_flags(player, card, sf);
					}
				}
			}
		}
	}
}

// Cards
int card_aether_mutation(int player, int card, event_t event){
	/* AEther Mutation	|3|G|U
	 * Sorcery
	 * Return target creature to its owner's hand. Put X 1/1 |Sgreen Saproling creature tokens onto the battlefield, where X is that creature's converted mana cost. */

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td1, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			generate_tokens_by_id(player, card, CARD_ID_SAPROLING, get_cmc(instance->targets[0].player, instance->targets[0].card));
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_ana_disciple(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) || has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
			return can_target(&td1);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && can_target(&td1) ){
				if( current_phase > PHASE_MAIN1 ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Give Flying\n Give -2/-0\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				tap_card(player, card);
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td1, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					tap_card(player, card);
					instance->info_slot = 66+choice;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -2, 0);
		}
	}

	return 0;
}

int card_ana_sanctuary(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = COLOR_TEST_BLUE;
		int score = 1*check_battlefield_for_special_card(player, card, player, 0, &this_test);
		this_test.color = COLOR_TEST_BLACK;
		score |= 2*check_battlefield_for_special_card(player, card, player, 0, &this_test);
		if( score > 0 ){
			int pump = 0;
			if( (score & 1) || (score & 2) ){
				pump = 1;
			}
			if( score == 3 ){
				pump = 5;
			}
			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, pump, pump);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_anavolver(int player, int card, event_t event){
	/*
	  Anavolver |3|G
	  Creature - Volver 3/3
	  Kicker {1}{U} and/or {B} (You may pay an additional {1}{U} and/or {B} as you cast this spell.)
	  If Anavolver was kicked with its {1}{U} kicker, it enters the battlefield with two +1/+1 counters on it and with flying.
	  If Anavolver was kicked with its {B} kicker, it enters the battlefield with a +1/+1 counter on it and with "Pay 3 life: Regenerate Anavolver."
	*/
	volver(player, card, event, MANACOST_XU(1, 1), MANACOST_B(1), 10, 5);

	if( in_play(player, card) && check_special_flags(player, card, SF_KICKED) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FLYING);
	}

	if( in_play(player, card) && check_special_flags(player, card, SF_KICKED2) ){
		card_instance_t *instance = get_card_instance( player, card );

		if( land_can_be_played & LCBP_REGENERATION ){
			if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && can_pay_life(player, 3) && can_regenerate(player, card) &&
				has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
			  ){
				return 0x63;
			}
			else if( event == EVENT_ACTIVATE ){
					if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
						lose_life(player, 3);
					}
			}
			else if( event == EVENT_RESOLVE_ACTIVATION && can_regenerate(player, instance->parent_card) ){
					 regenerate_target(player, instance->parent_card);
			}
		}
	}

	return 0;
}

int card_bloodfire_colossus(int player, int card, event_t event){
	return bloodfire(player, card, event, 6);
}

int card_bloodfire_dwarf(int player, int card, event_t event){
	return bloodfire(player, card, event, 1);
}

int card_bloodfire_kavu(int player, int card, event_t event){
	return bloodfire(player, card, event, 2);
}

int card_brass_herald(int player, int card, event_t event){
	/* Brass Herald	|6
	 * Artifact Creature - Golem 2/2
	 * As ~ enters the battlefield, choose a creature type.
	 * When ~ enters the battlefield, reveal the top four cards of your library. Put all creature cards of the chosen type revealed this way into your hand and the rest on the bottom of your library in any order.
	 * Creatures of the chosen type get +1/+1. */

	card_instance_t* instance;

	if (event == EVENT_RESOLVE_SPELL){
		instance = get_card_instance(player, card);

		instance->info_slot = select_a_subtype(player, card) + 1;
		// + 1 so if nothing's chosen - perhaps something turned into a copy of this after entering the battlefield - it's distinct from SUBTYPE_ADVISOR at 0
	}

	if( comes_into_play(player, card, event) ){
		instance = get_card_instance(player, card);

		int creature_type = instance->info_slot - 1;

		int i=0;
		int *deck = deck_ptr[player];
		for(i=0;i<4;i++){
			int card_added = add_card_to_hand(player, deck[0] );
			reveal_card(player, card, player, card_added);
			if( creature_type >= 0 && has_subtype(player, card_added, creature_type) ){
				remove_card_from_deck( player, 0 );
			}
			else{
				obliterate_card(player, card_added);
				put_top_card_of_deck_to_bottom(player);
			}
		}
	}
	else if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) &&
			 (instance = in_play(player, card)) &&
			 (instance->info_slot >= 1) &&
			 has_subtype(affected_card_controller, affected_card, instance->info_slot - 1)
			){
			event_result++;
	}
	return 0;
}

int card_captains_maneuver(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) && can_target(&td1) && ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG) ){
		return 0x63;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			instance->info_slot = x_value;
			if( pick_target(&td, "TARGET_DAMAGE") ){
				new_pick_target(&td1, "TARGET_CREATURE_OR_PLAYER", 1, 1);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td1, 1) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( instance->info_slot >= dmg->info_slot ){
				dmg->damage_target_player = instance->targets[1].player;
				dmg->damage_target_card = instance->targets[1].card;
			}
			else{
				dmg->info_slot-=instance->info_slot;
				damage_creature(instance->targets[1].player, instance->targets[1].card, instance->info_slot, dmg->damage_source_player, dmg->damage_source_card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_ceta_disciple(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( (has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && can_target(&td)) || has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana(player, COLOR_RED, 1) && can_target(&td) ){
			if( has_mana(player, COLOR_GREEN, 1) ){
				choice = do_dialog(player, player, card, -1, -1, " Give +2/+0\n Produce mana\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				tap_card(player, card);
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
				if( spell_fizzled != 1 ){
					instance->info_slot = 66+choice;
					return mana_producer(player, card, event);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 0);
		}
		if( instance->info_slot == 67 ){
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

int card_cetavolver(int player, int card, event_t event){
	/*
	  Cetavolver English |1|U
	  Creature - Volver 1/1
	  Kicker {1}{R} and/or {G} (You may pay an additional {1}{R} and/or {G} as you cast this spell.)
	  If Cetavolver was kicked with its {1}{R} kicker, it enters the battlefield with two +1/+1 counters on it and with first strike.
	  If Cetavolver was kicked with its {G} kicker, it enters the battlefield with a +1/+1 counter on it and with trample.
	*/
	volver(player, card, event, MANACOST_XR(1, 1), MANACOST_G(1), 5, 10);

	if( in_play(player, card) && check_special_flags(player, card, SF_KICKED) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FIRST_STRIKE);
	}

	if( in_play(player, card) && check_special_flags(player, card, SF_KICKED2) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_TRAMPLE);
	}

	return 0;
}

int card_ceta_sanctuary(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = COLOR_TEST_RED;
		int score = 1*check_battlefield_for_special_card(player, card, player, 0, &this_test);
		this_test.color = COLOR_TEST_GREEN;
		score |= 2*check_battlefield_for_special_card(player, card, player, 0, &this_test);
		if( score > 0 ){
			int crds = 0;
			if( (score & 1) || (score & 2) ){
				crds = 1;
			}
			if( score == 3 ){
				crds = 2;
			}
			if( crds > 0  ){
				draw_cards(player, crds);
				discard(player, 0, player);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_order_chaos(int player, int card, event_t event){
	/*
	  Order (Order/Chaos) |3|W
	  Instant
	  Exile target attacking creature.

	  Chaos (Order/Chaos) |2|R
	  Instant
	  Creatures can't block this turn.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_state = TARGET_STATE_ATTACKING;

		generic_split_card(player, card, event, can_target(&td), 5, MANACOST_XR(2, 1), 1, 10, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_state = TARGET_STATE_ATTACKING;
		new_pick_target(&td, "Select target attacking creature.", 0, 1 | GS_LITERAL_PROMPT);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	int priority_order = 5;
	int priority_chaos = 8;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		priority_order = current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS ? 10 : 0;
		priority_chaos = current_turn != player && can_target(&td) ? 10 : 0;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot & 1) && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		if( instance->info_slot & 2 ){
			pump_subtype_until_eot(player, card, ANYBODY, -1, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), priority_order, MANACOST_XR(2, 1), 1, priority_chaos, 0, "Order", "Chaos");
}

int card_consume_strength(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			new_pick_target(&td1, "TARGET_CREATURE", 1, 1);
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
		if( validate_target(player, card, &td1, 1) ){
			pump_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, -2, -2);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

static const char* is_blocking_crm(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_blocking(card, targeting_card))
	return NULL;
  else
	return "must be blocking Cromat";
}

static const char* crm_is_blocking_him(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_blocking(targeting_card, card))
	return NULL;
  else
	return "must be blocked by Cromat";
}

int card_cromat(int player, int card, event_t event){
	/*
	  Cromat English

	  Legendary Creature - Illusion 5/5, WUBRG (5)

	  {W}{B}: Destroy target creature blocking or blocked by Cromat.

	  {U}{R}: Cromat gains flying until end of turn.

	  {B}{G}: Regenerate Cromat.

	  {R}{W}: Cromat gets +1/+1 until end of turn.

	  {G}{U}: Put Cromat on top of its owner's library.
	*/
	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.allowed_controller =  1-player;
	td2.preferred_controller = 1-player;
	td2.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td2.extra = (int32_t)is_blocking_crm;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller =  1-player;
	td1.preferred_controller = 1-player;
	td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td1.extra = (int32_t)crm_is_blocking_him;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		int result = regeneration(player, card, event, MANACOST_BG(1, 1));
		if( result ){
			return result;
		}
		if( current_turn == player ){
			if( current_phase == PHASE_AFTER_BLOCKING && generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_BW(1, 1), 0, &td2, "TARGET_CREATURE") ){
				return 1;
			}
		}
		if( current_turn != player ){
			if( current_phase == PHASE_AFTER_BLOCKING && generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_BW(1, 1), 0, &td1, "TARGET_CREATURE") ){
				return 1;
			}
		}
		if( has_mana_for_activated_ability(player, card, MANACOST_UR(1, 1)) ){ // gains flying
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, MANACOST_RW(1, 1)) ){ // pump
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, MANACOST_UG(1, 1)) ){ // put on top of deck
			return 1;
		}
	}

	if (event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){

		int can_regen = regeneration(player, card, EVENT_CAN_ACTIVATE, MANACOST_BG(1, 1));
		int can_destroy_blocked_blocking_creature = 0;
		if( current_turn == player ){
			can_destroy_blocked_blocking_creature = (current_phase == PHASE_AFTER_BLOCKING && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_BG(1, 1), 0, &td2, "TARGET_CREATURE"));
		}
		else{
			can_destroy_blocked_blocking_creature = (current_phase == PHASE_AFTER_BLOCKING && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_BG(1, 1), 0, &td1, "TARGET_CREATURE"));
		}
		int can_gain_flying =  has_mana_for_activated_ability(player, card, MANACOST_UR(1, 1)) ;
		int can_pump = has_mana_for_activated_ability(player, card, MANACOST_RW(1, 1)) ;
		int can_be_put_on_top_of_library = has_mana_for_activated_ability(player, card, MANACOST_UG(1, 1)) ;

		enum{
			CHOICE_DESTR_BLOCK = 1,
			CHOICE_FLYING = 2,
			CHOICE_REGENERATE = 3,
			CHOICE_PUMP = 4,
			CHOICE_RETURN_DECK = 5,
		};
		int choice = can_regen ? CHOICE_REGENERATE : 0;
		if( ! choice ){
			choice = DIALOG(player, card, event,
							DLG_RANDOM,
							"Destroy a creature blocking / blocked by me", can_destroy_blocked_blocking_creature, 15, DLG_MANA(MANACOST_BW(1, 1)),
							"Gains flying", can_gain_flying, 5+30*(current_phase < PHASE_DECLARE_BLOCKERS), DLG_MANA(MANACOST_UR(1, 1)),
							"Regenerate", can_regen, 35, DLG_MANA(MANACOST_BG(1, 1)),
							"Gains +1/+1", can_pump, 25-20*can_destroy_blocked_blocking_creature, DLG_MANA(MANACOST_RW(1, 1)),
							"Put on top of deck", can_be_put_on_top_of_library, 5, DLG_MANA(MANACOST_UG(1, 1))
							);
		}

		if (event == EVENT_ACTIVATE){
			if( choice == CHOICE_DESTR_BLOCK ){
				if( current_turn == player ){
					if( ! select_target(player, card, &td2, "Select a creature that is blocking Cromat", &(instance->targets[0])) ){
						spell_fizzled = 1;
						return 0;
					}
				}
				else{
					if( ! select_target(player, card, &td1, "Select a creature that is blocked by Cromat", &(instance->targets[0])) ){
						spell_fizzled = 1;
						return 0;
					}
				}
			}
		}
		else	// event == EVENT_RESOLVE_ACTIVATION
			{
			  switch (choice)
				{
				  case CHOICE_REGENERATE:
					{
						if( can_be_regenerated(instance->parent_controller, instance->parent_card) ){
							regenerate_target(instance->parent_controller, instance->parent_card);
						}
						break;
					}

				  case CHOICE_DESTR_BLOCK:
					{
						if( (current_turn == player && valid_target(&td2)) || (current_turn != player && valid_target(&td1)) ){
							kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
						}
						break;
					}
				  case CHOICE_FLYING:
					{
						pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
												0, 0, KEYWORD_FLYING, 0);
						break;
					}
				  case CHOICE_PUMP:
					{
						pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, 1);
						break;
					}
				  case CHOICE_RETURN_DECK:
					{
						if( in_play(instance->parent_controller, instance->parent_card) ){
							put_on_top_of_deck(instance->parent_controller, instance->parent_card);
						}
						break;
					}
				}
		}
	}

	return 0;
}

int card_death_grasp(int player, int card, event_t event){
	return generic_x_spell(player, card, event, TARGET_ZONE_CREATURE_OR_PLAYER, 0, 9);
}

int card_dega_disciple(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( (has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && can_target(&td1)) ||
			(has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && can_target(&td))
		  ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && can_target(&td1) ){
				choice = do_dialog(player, player, card, -1, -1, " Give -2/-0\n Give +2/+0\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				tap_card(player, card);
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
				if( spell_fizzled != 1 && pick_target(&td1, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					tap_card(player, card);
					instance->info_slot = 66+choice;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -2, 0);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 0);
		}
	}

	return 0;
}

int card_dega_sanctuary(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = COLOR_TEST_BLACK;
		int score = 1*check_battlefield_for_special_card(player, card, player, 0, &this_test);
		this_test.color = COLOR_TEST_RED;
		score |=  2*check_battlefield_for_special_card(player, card, player, 0, &this_test);
		if( score > 0 ){
			int crds = 0;
			if( (score & 1) || (score & 2) ){
				crds = 2;
			}
			if( score == 3 ){
				crds = 4;
			}
			if( crds > 0  ){
				gain_life(player, crds);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_degavolver(int player, int card, event_t event){
	/*
	  Degavolver |1|W
	  Creature - Volver 1/1
	  Kicker {1}{B} and/or {R} (You may pay an additional {1}{B} and/or {R} as you cast this spell.)
	  If Degavolver was kicked with its {1}{B} kicker, it enters the battlefield with two +1/+1 counters on it and with "Pay 3 life: Regenerate Degavolver."
	  If Degavolver was kicked with its {R} kicker, it enters the battlefield with a +1/+1 counter on it and with first strike.
	*/
	volver(player, card, event, MANACOST_XW(1, 1), MANACOST_R(1), 5, 10);

	if( in_play(player, card) && check_special_flags(player, card, SF_KICKED2) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FIRST_STRIKE);
	}

	if( in_play(player, card) && check_special_flags(player, card, SF_KICKED) ){
		card_instance_t *instance = get_card_instance( player, card );

		if( land_can_be_played & LCBP_REGENERATION ){
			if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && can_pay_life(player, 3) && can_regenerate(player, card) &&
				has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
			  ){
				return 0x63;
			}
			else if( event == EVENT_ACTIVATE ){
					if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
						lose_life(player, 3);
					}
			}
			else if( event == EVENT_RESOLVE_ACTIVATION ){
					 regenerate_target(player, instance->parent_card);
			}
		}
	}

	return 0;
}

int card_desolation_angel(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! has_mana_multi(player, 3, 2, 0, 0, 0, 2) ){
			ai_modifier -= 1000;
		}
	}

	kicker(player, card, event, MANACOST_W(2));

	if( comes_into_play(player, card, event) ){
		manipulate_all(player, card, player, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
		if( kicked(player, card) ){
			manipulate_all(player, card, 1-player,TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
		}
	}

	return 0;
}

int card_desolation_giant(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! has_mana_multi(player, 2, 0, 0, 0, 2, 2) && creature_count[player] > 0 ){
			ai_modifier -= 1000;
		}
	}

	kicker(player, card, event, MANACOST_W(2));

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		new_manipulate_all(player, card, player, &this_test, KILL_DESTROY);
		if( kicked(player, card) ){
			new_manipulate_all(player, card, 1-player, &this_test, KILL_DESTROY);
		}
	}

	return 0;
}

// divine light --> sivvi's rouse

// dodecapod --> vanilla

int card_dragon_arc(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a multicolored creature.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.color_flag = F3_MULTICOLORED;
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_ebony_treefolk(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 0, 1, 0, 1, 0, 0, 1, 1, 0, 0);
}

int card_enlistment_officer(int player, int card, event_t event){
	/* Enlistment Officer	|3|W
	 * Creature - Human Soldier 2/3
	 * First strike
	 * When ~ enters the battlefield, reveal the top four cards of your library. Put all Soldier cards revealed this way into your hand and the rest on the
	 * bottom of your library in any order. */
	return recruiter(player, card, event, SUBTYPE_SOLDIER);
}

int card_evasive_action(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && card_on_stack_controller != -1 && card_on_stack != -1){
		if( ! check_state(card_on_stack_controller, card_on_stack, STATE_CANNOT_TARGET) ){
			return 0x63;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, count_domain(player, card));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fervent_charge(int player, int card, event_t event)
{
  // Whenever a creature you control attacks, it gets +2/+2 until end of turn.
  int amt;
  if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, player, -1)))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
	  for (--amt; amt >= 0; --amt)
		if (in_play(current_turn, attackers[amt]))
		  pump_until_eot(player, card, current_turn, attackers[amt], 2, 2);
	}

  return global_enchantment(player, card, event);
}

int card_fire_ice(int player, int card, event_t event){
	/*
	Fire |1|R
	Instant
	Fire deals 2 damage divided as you choose among one or two target creatures and/or players.

	Ice |1|U
	Instant
	Tap target permanent.
	Draw a card.
	*/


	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_PERMANENT);

		generic_split_card(player, card, event, can_target(&td), 10, MANACOST_XU(1, 1), can_target(&td2), 8, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_FIRST_HALF ){
		instance->number_of_targets = 0;

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			if( instance->targets[0].card != -1 ){
				add_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
			}
			new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", 1, 1);
			remove_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);
		}
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		instance->number_of_targets = 0;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_PERMANENT);
		pick_target(&td2, "TARGET_PERMANENT");
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	int priority_fire = 10;
	int priority_ice = 5;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_PERMANENT);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		priority_ice = current_turn != player && current_phase < PHASE_DECLARE_BLOCKERS ? 15 : 5;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			divide_damage(player, card, &td);
		}
		if( instance->info_slot & 2 ){
			if( valid_target(&td2) ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
				draw_a_card(player);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, can_target(&td), priority_fire, MANACOST_XU(1, 1), can_target(&td2), priority_ice, 0, "Fire", "Ice");
}

int card_flowstone_charger(int player, int card, event_t event)
{
  // Whenever ~ attacks, it gets +3/-3 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	pump_until_eot(player, card, player, card, 3, -3);

  return 0;
}

// fungal shambler --> dimir cutpurse

int card_gerrard_capashen(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td_player;
		default_target_definition(player, card, &td_player, 0);
		td_player.zone = TARGET_ZONE_PLAYERS;

		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		if( valid_target(&td_player) ){
			gain_life(player, hand_count[1-player]);
		}
	}

	target_definition_t td_creature;
	default_target_definition(player, card, &td_creature, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td_creature) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}
	if( is_attacking(player, card) ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XW(3, 1), 0, &td_creature, "TARGET_CREATURE");
	}
	return 0;
}

int card_gerrards_verdict(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, instance->targets[0].player, &this_definition);
			this_definition.ai_selection_mode = AI_MIN_VALUE;

			int count = 0;
			while( count < 2 && hand_count[instance->targets[0].player] > 0 ){
					int result = new_effect_coercion(&this_definition, &this_test);
					if( is_what(-1, get_internal_card_id_from_csv_id(result), TYPE_LAND) ){
						gain_life(player, 3);
					}
					count++;
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_goblin_legionnaire(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int result = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && can_target(&td) ){
			result = 1;
		}
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) && can_target(&td1) ){
			result = 0x63;
		}
		return result;
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) && can_target(&td1) ){
			choice = 1;
		}
		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				instance->info_slot = 66+choice;
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
		if( choice == 1 ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1);
			if( spell_fizzled != 1 && pick_target(&td1, "TARGET_DAMAGE") ){
				instance->info_slot = 66+choice;
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
		if( instance->info_slot == 67 ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( dmg->info_slot < 3 ){
				dmg->info_slot = 0;
			}
			else{
				dmg->info_slot-=2;
			}
		}
	}

	return 0;
}

int card_goblin_ringleader( int player, int card, event_t event){
	/* Goblin Ringleader	|3|R
	 * Creature - Goblin 2/2
	 * Haste
	 * When ~ enters the battlefield, reveal the top four cards of your library. Put all Goblin cards revealed this way into your hand and the rest on the
	 * bottom of your library in any order. */
	haste(player, card, event);
	return recruiter(player, card, event, SUBTYPE_GOBLIN);
}

// goblin soldier --> rhino token

int card_goblin_trenches(int player, int card, event_t event){
	/* Goblin Trenches	|1|R|W
	 * Enchantment
	 * |2, Sacrifice a land: Put two 1/1 |Sred and |Swhite Goblin Soldier creature tokens onto the battlefield. */

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
		return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
		if( ! sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN_SOLDIER, 2);
	}

	return global_enchantment(player, card, event);
}

int card_grave_defiler( int player, int card, event_t event){
	/* Grave Defiler	|3|B
	 * Creature - Zombie 2/1
	 * When ~ enters the battlefield, reveal the top four cards of your library. Put all Zombie cards revealed this way into your hand and the rest on the
	 * bottom of your library in any order.
	 * |1|B: Regenerate ~. */

	recruiter(player, card, event, SUBTYPE_ZOMBIE);
	return regeneration(player, card, event, 1, 1, 0, 0, 0, 0);
}

int card_guided_passage(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( ! get_tutoring_denial(1-player) ){
			int cd = count_deck(player);
			int tutored = 0;
			int *deck = deck_ptr[player];
			while( cd > 0 && tutored < 3 ){
					char buffer[100];
					test_definition_t this_test;
					if( tutored == 0 ){
						scnprintf(buffer, 100, "Select an Creature to add to opponent's hand.");
						new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
					}
					if( tutored == 1 ){
						scnprintf(buffer, 100, "Select a Land to add to opponent's hand.");
						new_default_test_definition(&this_test, TYPE_LAND, buffer);
					}
					if( tutored == 2 ){
						scnprintf(buffer, 100, "Select a noncreature, nonland card.");
						new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_LAND, buffer);
						this_test.type_flag = 1;
					}
					int selected = new_select_a_card(1-player, player, TUTOR_FROM_DECK, 1, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 ){
						add_card_to_hand(player, deck[selected]);
						remove_card_from_deck(player, selected);
						cd--;
					}
					tutored++;
			}
			shuffle(player);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_index2(int player, int card, event_t event){
	/*
	  Index |U
	  Sorcery
	  Look at the top five cards of your library, then put them back in any order.
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		rearrange_top_x(player, player, 5);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_jilt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			if( ! is_token(player, card) ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				if( can_target(&td) && do_kicker(player, card, MANACOST_XR(1,1)) ){
					new_pick_target(&td, "TARGET_CREATURE", 1, 1);
				}
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for (i = 0; i < instance->number_of_targets; i++){
			if( i == 0 && validate_target(player, card, &td, 0) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
			if( i == 1 && validate_target(player, card, &td, 1) ){
				damage_creature(instance->targets[1].player, instance->targets[1].card, 2, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_kavu_howler( int player, int card, event_t event){
	/* Kavu Howler	|4|G|G
	 * Creature - Kavu 4/5
	 * When ~ enters the battlefield, reveal the top four cards of your library. Put all Kavu cards revealed this way into your hand and the rest on the bottom
	 * of your library in any order. */
	return recruiter(player, card, event, SUBTYPE_KAVU);
}

int card_kavu_mauler( int player, int card, event_t event)
{
  // Whenever ~ attacks, it gets +1/+1 until end of turn for each other attacking Kavu.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t this_test;
	  default_test_definition(&this_test, TYPE_CREATURE);
	  this_test.subtype = SUBTYPE_KAVU;
	  this_test.not_me = 1;
	  this_test.state = STATE_ATTACKING;

	  int amount = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
	  if (amount > 0)
		pump_until_eot(player, card, player, card, amount, amount);
	}

  return 0;
}

int card_last_stand(int player, int card, event_t event){
	/* Last Stand	|W|U|B|R|G
	 * Sorcery
	 * Target opponent loses 2 life for each |H2Swamp you control. ~ deals damage equal to the number of |H1Mountains you control to target creature. Put a 1/1 |Sgreen Saproling creature token onto the battlefield for each |H2Forest you control. You gain 2 life for each |H2Plains you control. Draw a card for each |H2Island you control, then discard that many cards. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_PLAYER") ){
			new_pick_target(&td1, "TARGET_CREATURE", 1, 1);
		}
	}


	if( event == EVENT_RESOLVE_SPELL){
		int good = 0;
		int amount = 0;
		if( validate_target(player, card, &td, 0) ){
			amount = count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP);
			lose_life(instance->targets[0].player, amount*2);
			good++;
		}
		if( validate_target(player, card, &td1, 1) ){
			amount = count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN);
			if( amount > 0 ){
				damage_creature(instance->targets[1].player, instance->targets[1].card, amount, player, card);
			}
			good++;
		}
		if( good ){
			amount = count_subtype(player, TYPE_LAND, SUBTYPE_FOREST);
			if( amount > 0 ){
				generate_tokens_by_id(player, card, CARD_ID_SAPROLING, amount);
			}

			amount = count_subtype(player, TYPE_LAND, SUBTYPE_PLAINS);
			gain_life(player, amount*2);

			amount = count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND);
			if( amount > 0 ){
				draw_cards(player, amount);
				int j;
				for(j=0;j<amount;j++){
					discard (player, 0, player);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_lay_of_the_land(int player, int card, event_t event){
	/*
	  Lay of the Land |G
	  Sorcery
	  Search your library for a basic land card, reveal it, put it into your hand, then shuffle your library.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		tutor_basic_lands(player, TUTOR_HAND, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}


int card_legacy_weapon(int player, int card, event_t event){

	/* Legacy Weapon	|7
	 * Legendary Artifact
	 * |W|U|B|R|G: Exile target permanent.
	 * If ~ would be put into a graveyard from anywhere, reveal ~ and shuffle it into its owner's library instead. */

	// Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

	check_legend_rule(player, card, event);

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 1, 1, 1, 1, 1) ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 1, 1, 1, 1, 1);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_PERMANENT") ){
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_life_death(int player, int card, event_t event){
	/*
	  Life |G
	  Sorcery
	  All lands you control become 1/1 creatures until end of turn. They're still lands.

	  Death |1|B
	  Sorcery
	  Return target creature card from your graveyard to the battlefield. You lose life equal to its converted mana cost.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		int can_play_death = count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player);

		generic_split_card(player, card, event, 1, 8, MANACOST_XB(1, 1), can_play_death, 10, 0, NULL, NULL);
	}

	if( event == EVENT_PLAY_SECOND_HALF ){
		char msg[100] = "Select a creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		if( new_select_target_from_grave(player, card, player, 0, AI_MAX_CMC, &this_test, 0) == -1 ){
			spell_fizzled = 1;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	int can_play_death = count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player);

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot & 1 ){
			test_definition_t this_test2;
			default_test_definition(&this_test2, TYPE_LAND);
			land_animation2(player, card, player, -1, 1, 1, 1, 0, 0, 0, &this_test2);
		}
		if( instance->info_slot & 2 ){
			int selected = validate_target_from_grave(player, card, player, 0);
			if( selected != -1 ){
				int zombo = reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
				lose_life(player, get_cmc(player, zombo));
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_split_card(player, card, event, 1, 8, MANACOST_XB(1, 1), can_play_death, 10, 0, "Life", "Death");
}

int card_living_airship(int player, int card, event_t event){
	return regeneration(player, card, event, 2, 0, 0, 1, 0, 0);
}

/* Llanowar Dead	|B|G	=>battle_for_zendikar.c:card_generic_combat_1_mana_producing_creature
 * Creature - Zombie Elf 2/2
 * |T: Add |B to your mana pool. */

int card_mystic_snake(int player, int card, event_t event){
	/* Mystic Snake	|1|G|U|U
	 * Creature - Snake 2/2
	 * Flash
	 * When ~ enters the battlefield, counter target spell. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		if( card_counterspell(player, card, EVENT_CAN_CAST) ){
			instance->info_slot = 1;
			return 99;
		}
		else{
			instance->info_slot = 0;
			return 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  && current_turn == player){
		ai_modifier -= 10;
	}

	if( comes_into_play(player, card, event) ){
		if( instance->info_slot == 1 ){
			real_counter_a_spell(player, card, card_on_stack_controller, card_on_stack);
		}
	}
	return 0;
}

int card_necra_sanctuary(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = COLOR_TEST_GREEN;
		int score = 1*check_battlefield_for_special_card(player, card, player, 0, &this_test);
		this_test.color = COLOR_TEST_WHITE;
		score |=  2*check_battlefield_for_special_card(player, card, player, 0, &this_test);
		if( score > 0 ){
			int pump = 0;
			if( (score & 1) || (score & 2) ){
				pump = 1;
			}
			if( score == 3 ){
				pump = 3;
			}
			if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				lose_life(instance->targets[0].player, pump);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_necravolver(int player, int card, event_t event){
	/*
	  Necravolver |2|B
	  Creature - Volver 2/2
	  Kicker {1}{G} and/or {W} (You may pay an additional {1}{G} and/or {W} as you cast this spell.)
	  If Necravolver was kicked with its {1}{G} kicker, it enters the battlefield with two +1/+1 counters on it and with trample.
	  If Necravolver was kicked with its {W} kicker, it enters the battlefield with a +1/+1 counter on it and with "Whenever Necravolver deals damage, you gain that much life."
	*/
	volver(player, card, event, MANACOST_XG(1, 1), MANACOST_W(1), 8, 10);

	if( in_play(player, card) && check_special_flags(player, card, SF_KICKED) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_TRAMPLE);
	}

	if( in_play(player, card) && check_special_flags(player, card, SF_KICKED2) ){
		spirit_link_effect(player, card, event, player);
	}

	return 0;
}

int card_orims_thunder(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "DISENCHANT") ){
			if( ! is_token(player, card) && can_target(&td1) && do_kicker(player, card, 0, 0, 0, 0, 1, 0) ){
				new_pick_target(&td1, "TARGET_CREATURE", 1, 1);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int dmg = get_cmc(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if( kicked(player, card) && validate_target(player, card, &td1, 1) ){
				damage_creature(instance->targets[1].player, instance->targets[1].card, dmg, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_overgrown_estate(int player, int card, event_t event){

	if( event == EVENT_SHOULD_AI_PLAY ){
		return should_ai_play(player, card);
	}
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 3);
	}
	return altar_basic(player, card, event, 0, TYPE_LAND);
}

int card_penumbra_bobcat(int player, int card, event_t event){
	/* Penumbra Bobcat	|2|G
	 * Creature - Cat 2/1
	 * When ~ dies, put a 2/1 |Sblack Cat creature token onto the battlefield. */

	if( graveyard_from_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_CAT, &token);
		token.color_forced = COLOR_TEST_BLACK;
		token.tou = 1;
		generate_token(&token);
	}
	return 0;
}

int card_penumbra_wurm(int player, int card, event_t event){
	/* Penumbra Wurm	|5|G|G
	 * Creature - Wurm 6/6
	 * Trample
	 * When ~ dies, put a 6/6 |Sblack Wurm creature token with trample onto the battlefield. */

	if( graveyard_from_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WURM, &token);
		token.color_forced = COLOR_TEST_BLACK;
		token.key_plus = KEYWORD_TRAMPLE;
		generate_token(&token);
	}
	return 0;
}

int card_pernicious_deed(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		return has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0);
	}

	else if(event == EVENT_ACTIVATE){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				// For the AI, use the most mana we have, from 2-max
				if( player == AI ){
					int i = 10;
					while(i > 1){
						if( has_mana( player, COLOR_ANY, i) ){
							charge_mana( player, COLOR_COLORLESS, i);
							instance = get_card_instance(player, card);
							instance->info_slot = i;
							break;
						}
						i--;
					}
					kill_card(player, card, KILL_SACRIFICE);

				}
				else{
					charge_mana( player, COLOR_COLORLESS, -1);
					int choice = x_value;
					if( spell_fizzled != 1 ){
						instance->info_slot = choice;
						kill_card(player, card, KILL_SACRIFICE);
					}
				}
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION){
			manipulate_all(player, card, player, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_ENCHANTMENT, 4, 0, 0, 0, 0, 0, 0, instance->info_slot+1, 3, KILL_BURY);
			manipulate_all(player, card, 1-player, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_ENCHANTMENT, 4, 0, 0, 0, 0, 0, 0, instance->info_slot+1, 3, KILL_BURY);
	}
	return global_enchantment(player, card, event);
}

int card_phyrexian_arena(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		draw_cards(player, 1);
		lose_life(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_phyrexian_rager(int player, int card, event_t event){

	phyrexians(player, card, event, 1);

	return 0;
}

int card_phyrexian_gargantua(int player, int card, event_t event){

	phyrexians(player, card, event, 2);

	return 0;
}

int card_planar_despair(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, 2, -1, -count_domain(player, card), -count_domain(player, card), 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_powerstone_minefield(int player, int card, event_t event){
	// Whenever a creature attacks or blocks, ~ deals 2 damage to it.
	int amt;
	if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, 2, -1))){
		card_instance_t* instance = get_card_instance(player, card);
		unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		for (--amt; amt >= 0; --amt){
			if (in_play(current_turn, attackers[amt])){
				damage_creature(current_turn, attackers[amt], 2, player, card);
			}
		}
	}

	if( event == EVENT_DECLARE_BLOCKERS){
	   int i;
	   for(i = 0; i < active_cards_count[1-current_turn]; i++){
		   if( in_play(1-current_turn, i) && is_what(1-current_turn, i, TYPE_CREATURE) && blocking(1-current_turn, i, event) ){
			   damage_creature(1-current_turn, i, 2, player, card);
		   }
	   }
	}
	return global_enchantment(player, card, event);
}

int card_prophetic_bolt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 4);
			impulse_effect(player, 4, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_putrid_warrior(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, 0) ){
		int ai_choice = 0;
		if( life[player] == 1 ){
			ai_choice = 1;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Players lose 1 life\n Players gain 1 life", ai_choice);
		if( choice == 0 ){
			lose_life(player, 1);
			lose_life(1-player, 1);
		}
		else{
			gain_life(player, 1);
			gain_life(1-player, 1);
		}
	}
	return 0;
}

int card_quicksilver_dagger(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		target_definition_t td1;
		default_target_definition(t_player, t_card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_PLAYER");
		}

		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_PLAYER");
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *this = get_card_instance( t_player, t_card );
			if( validate_target(t_player, t_card, &td1, 0) ){
				damage_player(this->targets[0].player, 1, t_player, t_card);
				draw_cards(player, 1);
			}
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_raka_sanctuary(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = COLOR_TEST_BLUE;
		int score = 1*check_battlefield_for_special_card(player, card, player, 0, &this_test);
		this_test.color = COLOR_TEST_WHITE;
		score |=  2*check_battlefield_for_special_card(player, card, player, 0, &this_test);
		if( score > 0 ){
			int pump = 0;
			if( (score & 1) || (score & 2) ){
				pump = 1;
			}
			if( score == 3 ){
				pump = 3;
			}
			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				damage_creature(instance->targets[0].player, instance->targets[0].card, pump, player, card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_rakavolver(int player, int card, event_t event){
	/*
	  Rakavolver |2|R
	  Creature - Volver 2/2
	  Kicker {1}{W} and/or {U} (You may pay an additional {1}{W} and/or {U} as you cast this spell.)
	  If Rakavolver was kicked with its {1}{W} kicker, it enters the battlefield with two +1/+1 counters on it and with "Whenever Rakavolver deals damage, you gain that much life."
	  If Rakavolver was kicked with its {U} kicker, it enters the battlefield with a +1/+1 counter on it and with flying.
	*/
	volver(player, card, event, MANACOST_XW(1, 1), MANACOST_U(1), 8, 10);


	if( in_play(player, card) && check_special_flags(player, card, SF_KICKED) ){
		spirit_link_effect(player, card, event, player);
	}

	if( in_play(player, card) && check_special_flags(player, card, SF_KICKED2) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FLYING);
	}

	return 0;
}

int card_soul_link(int player, int card, event_t event)
{
  /* Soul Link	|1|W|B
   * Enchantment - Aura
   * Enchant creature
   * Whenever enchanted creature deals damage, you gain that much life.
   * Whenever enchanted creature is dealt damage, you gain that much life. */

  card_instance_t* instance, *damage = damage_being_dealt(event);
  if (damage
	  && (instance = in_play(player, card)) && instance->damage_target_card >= 0
	  && ((damage->damage_source_card == instance->damage_target_card && damage->damage_source_player == instance->damage_target_player)
		  || (damage->damage_target_card == instance->damage_target_card && damage->damage_target_player == instance->damage_target_player)))
	{
	  /* Store damage amounts in targets[3]..targets[10] - backwards compatibility squanders targets[0], aura movement squanders targets[2], and global storage
	   * squanders targets[11]-targets[18]. This leaves us with 64 bytes out of 144. */
	  uint8_t* storage = (void*)(&instance->targets[3].player);
	  if (instance->info_slot < 64 && damage->info_slot <= 255)
		storage[instance->info_slot++] = damage->info_slot;
	  else
		gain_life(player, damage->info_slot);	// No more room, just gain life immediately
	}

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player
	  && (instance = get_card_instance(player, card))->info_slot > 0)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  uint8_t* storage = (void*)(&instance->targets[3].player);
		  // Free up info_slot and targets, just in case something triggers instantly (and improperly) which eventually results in dealing damage.
		  uint8_t dam[64];
		  int i, numdam = instance->info_slot;
		  memcpy(dam, storage, 64);
		  instance->info_slot = 0;

		  for (i = 0; i < numdam; ++i)
			gain_life(player, dam[i]);
		}
	}

  return vanilla_aura(player, card, event, ANYBODY);
}

int card_spiritmonger(int player, int card, event_t event){

	/* Spiritmonger	|3|B|G
	 * Creature - Beast 6/6
	 * Whenever ~ deals damage to a creature, put a +1/+1 counter on ~.
	 * |B: Regenerate ~.
	 * |G: ~ becomes the color of your choice until end of turn. */

	card_instance_t *instance = get_card_instance(player, card);

	/* Usually we'd deal with this in TRIGGER_DEAL_DAMAGE, but the counter shouldn't be added until after the damage has fully resolved, which happens *after*
	 * TRIGGER_DEAL_DAMAGE, or else it might raise the Spiritmonger's toughness out of lethal damage range. */
	card_instance_t* damage = damage_being_dealt(event);
	if (damage
		&& damage->damage_source_card == card && damage->damage_source_player == player
		&& damage->damage_target_card != -1 && is_what(damage->damage_target_player, damage->damage_target_card, TYPE_CREATURE)
	   ){
		if (instance->targets[3].player < 0){
			instance->targets[3].player = 0;
		}
		++instance->targets[3].player;
	}
	if (event == EVENT_AFTER_DAMAGE && instance->targets[3].player > 0){
		int counters = instance->targets[3].player;
		instance->targets[3].player = 0;
		add_1_1_counters(player, card, counters);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		int result = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
			result = 1;
		}
		if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) ){
			int cr = can_regenerate(player, card);
			if( cr > 0 ){
				result = cr;
			}
		}
		return result;
	}

	if(event == EVENT_ACTIVATE ){
		if( can_regenerate(player, card) > 0 ){
			charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66;
			}
		}
		else{
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 67;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			regenerate_target(player, instance->parent_card);
		}
		if( instance->info_slot == 67 ){
			int clr = 1<<choose_a_color(player, get_deck_color(player, 1-player));
			change_color(player, instance->parent_card, player, instance->parent_card, clr, CHANGE_COLOR_SET|CHANGE_COLOR_END_AT_EOT|CHANGE_COLOR_NO_SLEIGHT);
		}
	}

	return 0;
}

int card_squees_embrace(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;
		if(event == EVENT_GRAVEYARD_FROM_PLAY){
			if( affect_me( t_player, t_card ) ){
				card_instance_t *affected = get_card_instance(t_player, t_card);
				if(affected->kill_code != KILL_REMOVE && affected->kill_code > 0 ){
					int dest = t_player;
					if( is_stolen(t_player, t_card) ){
						dest = 1-t_player;
					}
					add_card_to_hand( dest, affected->internal_card_id );
					affected->kill_code = KILL_REMOVE;
				}
			}
		}
	}
	return generic_aura(player, card, event, player, 2, 2, 0, 0, 0, 0, 0);
}

int card_squees_revenge(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int flips = choose_a_number(player, "Flip how many coins?", 2);
			int count = 0;
			int result = 0;
			while( count < flips ){
					if( flip_a_coin(player, card) ){
						result++;
					}
					else{
						break;
					}
					count++;
			}
			if( result == flips ){
				draw_cards(player, flips*2);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_strength_of_night(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, 0);
		if( kicked(player, card) ){
			pump_subtype_until_eot(player, card, player, SUBTYPE_ZOMBIE, 2, 2, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return kicker(player, card, event, 0, 1, 0, 0, 0, 0);
}

int card_suffocating_blast(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && card_on_stack_controller != -1 && card_on_stack != -1){
		if( ! check_state(card_on_stack_controller, card_on_stack, STATE_CANNOT_TARGET) && can_target(&td) ){
			return 0x63;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
			new_pick_target(&td, "TARGET_CREATURE", 1, 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		if( validate_target(player, card, &td, 1) ){
			damage_creature(instance->targets[1].player, instance->targets[1].card, 3, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int suppress_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	/* targets[0].player: low byte contains player to return cards to;
	 *                    low bit of BYTE1 is set if this is the same turn the spell was cast.
	 * targets[0].card, targets[1..18].player, targets[1..18].card: contain internal_card_id's of exiled cards. */

	if (event == EVENT_BEGIN_TURN){
		instance->targets[0].player &= ~0x100;
	}
	if (current_turn == instance->targets[0].player && eot_trigger(player, card, event)){
		instance->targets[0].player = -1;	// Not a card id
		int i;
		for (i = 18; i >= 0; --i){
			if (instance->targets[i].card != -1){
				int id = cards_data[instance->targets[i].card].id;
				if( check_rfg(current_turn, id) ){
					add_card_to_hand(current_turn, instance->targets[i].card);
					remove_card_from_rfg(current_turn, id);
				}
			}
			if (instance->targets[i].player != -1){
				int id = cards_data[instance->targets[i].player].id;
				if( check_rfg(current_turn, id) ){
					add_card_to_hand(current_turn, instance->targets[i].player);
					remove_card_from_rfg(current_turn, id);
				}
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_suppress(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( hand_count[instance->targets[0].player] > 0 ){
				int legacy = create_legacy_effect(player, card, &suppress_legacy);
				card_instance_t *leg = get_card_instance(player, legacy);
				int removed = 0;
				int count = active_cards_count[instance->targets[0].player]-1;
				leg->targets[0].player = instance->targets[0].player;
				leg->targets[0].player |= 0x100;	// prevents legacy from triggering at end of this turn, since it won't match current_turn anymore
				while( count > -1 ){
						if( in_hand(instance->targets[0].player, count) && removed < 19 ){
							card_instance_t *crd = get_card_instance(instance->targets[0].player, count);
							if (leg->targets[removed].player == -1){
								leg->targets[removed].player = crd->internal_card_id;
							} else {
								leg->targets[removed].card = crd->internal_card_id;
								++removed;
							}
							rfg_card_in_hand(instance->targets[0].player, count);
							removed++;
						}
						count--;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sylvan_messenger( int player, int card, event_t event){
	/* Sylvan Messenger	|3|G
	 * Creature - Elf 2/2
	 * Trample
	 * When ~ enters the battlefield, reveal the top four cards of your library. Put all Elf cards revealed this way into your hand and the rest on the bottom
	 * of your library in any order. */
	return recruiter(player, card, event, SUBTYPE_ELF);
}

int card_symbiotic_deployment(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	skip_your_draw_step(player, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && target_available(player, card, &td) > 1 ){
		return has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			if( new_pick_target(&td, "TARGET_CREATURE", 1, 1) ){
				instance->number_of_targets = 2;
				tap_card(instance->targets[0].player, instance->targets[0].card);
				tap_card(instance->targets[1].player, instance->targets[1].card);
			}
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_temporal_spring(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td1, "TARGET_PERMANENT");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_tidal_courier( int player, int card, event_t event){
	/* Tidal Courier	|3|U
	 * Creature - Merfolk 1/2
	 * When ~ enters the battlefield, reveal the top four cards of your library. Put all Merfolk cards revealed this way into your hand and the rest on the
	 * bottom of your library in any order.
	 * |3|U: ~ gains flying until end of turn. */
	recruiter(player, card, event, SUBTYPE_MERFOLK);
	return generic_shade(player, card, event, 0, 3, 0, 1, 0, 0, 0, 0, 0, KEYWORD_FLYING, 0);
}

int card_urborg_uprising(int player, int card, event_t event)
{
  /* Urborg Uprising	|4|B
   * Sorcery
   * Return up to two target creature cards from your graveyard to your hand.
   * Draw a card. */
  if (!IS_CASTING(player, card, event))
	return 0;
  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select up to two target creature cards.");
  int rval = spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(player, card, event, 2, &test, 1);
  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (spell_fizzled != 1)
		draw_a_card(player);
	  kill_card(player, card, KILL_DESTROY);
	}
  return rval;
}

int card_wild_research(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_hybrid(player, 1, COLOR_BLUE, COLOR_WHITE, 1) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0) ){
			if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1) ){
				choice = do_dialog(player, player, card, -1, -1, " Tutor an Instant\n Tutor an Enchantment\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			int blue = 1;
			int white = 0;
			if( choice == 1 ){
				blue = 0;
				white = 1;
			}
			charge_mana_for_activated_ability(player, card, 1, 0, blue, 0, 0, white);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select an Instant card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_INSTANT | TYPE_INTERRUPT, msg);
		this_test.type_flag = F1_NO_CREATURE;
		if( instance->info_slot == 67 ){
			strcpy(msg, "Select an Enchantment card.");
			new_default_test_definition(&this_test, TYPE_ENCHANTMENT, msg);
			this_test.type_flag = F1_NO_PWALKER;
		}
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		discard(player, DISC_RANDOM, player);
	}

	return global_enchantment(player, card, event);
}
