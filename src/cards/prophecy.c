#include "manalink.h"

// Functions
static int control_an_untapped_land(int player){
	int i;
	for(i=0; i<2; i++){
		if( player == i || player == 2 ){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && is_what(i, count, TYPE_LAND) && ! is_tapped(i, count) ){
						return 1;
					}
					count++;
			}
		}
	}
	return 0;
}


// Cards
int card_abolish(int player, int card, event_t event){
	/*
	  Abolish |1|W|W
	  Instant
	  You may discard a Plains card rather than pay Abolish's mana cost.
	  Destroy target artifact or enchantment.
	*/

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a Plains card.");
		this_test.zone = TARGET_ZONE_HAND;
		this_test.subtype = SUBTYPE_PLAINS;	// can't hack in time
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, "Select a Plains card.");
	this_test.zone = TARGET_ZONE_HAND;
	this_test.subtype = SUBTYPE_PLAINS;	// can't hack in time

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
				choice = do_dialog(player, player, card, -1, -1, " Discard a Plains card\n Play normally\n Cancel", 1);
			}

			if( choice == 0 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
				if( selected > -1 ){
					discard_card(player, selected);
					td.allow_cancel = 0;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
			}
		}
		if( spell_fizzled != 1 ){
			pick_target(&td, "DISENCHANT");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_alexi_zephyr_mage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		hand_count[player] > 1 && has_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0)
	  ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		int trgs = 0;
		while( has_mana_for_activated_ability(player, card, trgs, 0, 1, 0, 0, 0) && can_target(&td) ){
				if( new_pick_target(&td, "TARGET_CREATURE", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		if( trgs == 0 ){
			spell_fizzled = 1;
		}
		else{
			int i;
			for(i=0; i<trgs; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			instance->number_of_targets = trgs;
			charge_mana_for_activated_ability(player, card, trgs, 0, 1, 0, 0, 0);
			if( spell_fizzled != 1 ){
				tap_card(player, card);
				multidiscard(player, 2, 0);
				instance->info_slot = trgs;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				bounce_permanent(instance->targets[i].player, instance->targets[i].card);
			}
		}
	}

	return 0;
}

int card_aura_fracture(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
			sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			int result = 0;
			while( result == 0 ){
					if( new_pick_target(&td, "TARGET_ENCHANTMENT", 0, 0) ){
						instance->number_of_targets = 1;
						if( ! is_planeswalker(instance->targets[0].player, instance->targets[0].card) ){
							result = 1;
						}
					}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return global_enchantment(player, card, event);
}

int card_avatar_of_hope(int player, int card, event_t event)
{
	// 415910
	if (event == EVENT_MODIFY_COST && life[player] <= 3){
		COST_COLORLESS -= 6;
	}

	creature_can_block_additional(player, card, event, 255);

	return 0;
}

int card_avatar_of_fury(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		if( count_subtype(1-player, TYPE_LAND, -1) > 6 ){
			COST_COLORLESS-=6;
		}
	}

	return generic_shade(player, card, event, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0);
}

int card_avatar_of_might(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		if( count_subtype(1-player, TYPE_CREATURE, -1)-count_subtype(player, TYPE_CREATURE, -1) > 3){
			COST_COLORLESS-=6;
		}
	}

	return 0;
}

int card_avatar_of_will(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		if( hand_count[1-player] < 1 ){
			COST_COLORLESS-=6;
		}
	}

	return 0;
}

int card_avatar_of_wOE(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	fear(player, card, event);

	if( event == EVENT_MODIFY_COST ){
		if( count_graveyard_by_type(1-player, TYPE_CREATURE)+count_graveyard_by_type(player, TYPE_CREATURE) > 9){
			COST_COLORLESS-=6;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_blessed_wind(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			set_life_total(instance->targets[0].player, 20);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_branded_brawlers(int player, int card, event_t event){
  if (event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && control_an_untapped_land(1-player))
	event_result = 1;

  if (event == EVENT_BLOCK_LEGALITY && affect_me(player, card) && control_an_untapped_land(player))
	event_result = 1;

  return 0;
}

int card_calming_verses(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		new_manipulate_all(player, card, 1-player, &this_test, KILL_DESTROY);
		if( control_an_untapped_land(player) ){
			new_manipulate_all(player, card, player, &this_test, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_chimeric_idol(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		new_manipulate_all(player, card, player, &this_test, ACT_TAP);
		artifact_animation(player, instance->parent_card, player, instance->parent_card, 1, 3, 3, 0, 0);
	}

	return 0;
}

int card_citadel_of_pain(int player, int card, event_t event){

	if( eot_trigger(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.state = STATE_TAPPED;
		this_test.state_flag = 1;
		int amount = check_battlefield_for_special_card(player, card, current_turn, 4, &this_test);
		if( amount > 0 ){
			damage_player(current_turn, amount, player, card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_coastal_hornclaw(int player, int card, event_t event){
	/*
	  Coastal Hornclaw |4|U
	  Creature - Bird 3/3
	  Sacrifice a land: Coastal Hornclaw gains flying until end of turn.
	*/
	if (!IS_GAA_EVENT(event))
		return 0;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( ! sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								0, 0, KEYWORD_FLYING, 0);
	}
	return 0;
}

int card_denying_wind(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			char msg[100] = "Select a card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);

			int amount = 7;
			if( amount > count_deck(instance->targets[0].player) ){
				amount = count_deck(instance->targets[0].player);
			}
			while( amount > 0 ){
					if( new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test) != -1 ){
						amount--;
					}
					else{
						break;
					}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int token_generated_by_me(int player, int card, int tok_player, int tok_card){
	if( ! is_token(tok_player, tok_card) ){
		return 0;
	}
	card_instance_t *instance = get_card_instance(tok_player, tok_card);
	if( instance->damage_source_player > -1 ){
		if( instance->damage_source_player == player && instance->damage_source_card == card ){
			return 1;
		}
	}
	return 0;
}

int card_dual_nature(int player, int card, event_t event){
	/*
	  Dual Nature English |4|G|G

	  Whenever a nontoken creature enters the battlefield, its controller puts a token that's a copy of that creature onto the battlefield.

	  Whenever a nontoken creature leaves the battlefield, exile all tokens with the same name as that creature.

	  When Dual Nature leaves the battlefield, exile all tokens put onto the battlefield with Dual Nature.
	*/
	if( ! is_humiliated(player, card) ){
		if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0) ){
			card_instance_t *instance = get_card_instance(player, card);

			int id = get_id(instance->targets[1].player, instance->targets[1].card);
			token_generation_t token;
			default_token_definition(player, card, id, &token);
			token.t_player = trigger_cause_controller;
			token.legacy = 1;
			token.special_code_for_legacy = &empty;
			generate_token(&token);
		}

		if( trigger_condition == TRIGGER_LEAVE_PLAY && affect_me( player, card ) &&
		   reason_for_trigger_controller == affected_card_controller
		 ){
			int trig = 0;
			int id = get_card_name(trigger_cause_controller, trigger_cause);
			if( id != -1 ){ //If a face-down card leaves play, Double Nature will trigger but won't do nothing as it has no name
				if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) && ! is_token(trigger_cause_controller, trigger_cause) ){
					trig = 1;
				}

				if( trig > 0 ){
					if(event == EVENT_TRIGGER){
						event_result |= RESOLVE_TRIGGER_MANDATORY;
					}
					else if(event == EVENT_RESOLVE_TRIGGER){
							test_definition_t this_test;
							default_test_definition(&this_test, TYPE_CREATURE);
							this_test.type_flag = F1_IS_TOKEN;
							this_test.id = id;
							APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_REMOVE);};);
					}
				}
			}
		}

		if( leaves_play(player, card, event) ){
			int i;
			for(i=0; i<2; i++){
				int p = i == 0 ? player : 1-player;
				int c;
				for(c=active_cards_count[p]-1; c > -1; c--){
					if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) ){
						if( token_generated_by_me(player, card, p, c) ){
							kill_card(p, c, KILL_REMOVE);
						}
					}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_entangler(int player, int card, event_t event)
{
  attached_creature_can_block_additional(player, card, event, 255);
  return vanilla_aura(player, card, event, player);
}

int card_foil(int player, int card, event_t event){

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, "Select an Island card.");
	this_test.zone = TARGET_ZONE_HAND;
	this_test.subtype = SUBTYPE_ISLAND;	// can't hack in time

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		if( hand_count[player] > 1 && check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}
	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
				int choice = 0;
				if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
					choice = do_dialog(player, player, card, -1, -1, " Discard an Island + another card\n Play normally\n Cancel", 1);
				}

				if( choice == 0 ){
					int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected > -1 ){
						discard_card(player, selected);
						discard(player, 0, player);
					}
					else{
						spell_fizzled = 1;
					}
				}
				else if( choice == 1 ){
						charge_mana_from_id(player, card, event, get_id(player, card));
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			if( spell_fizzled != 1 ){
				counterspell(player, card, event, NULL, 0);
			}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		return counterspell(player, card, event, NULL, 0);
	}

	return 0;
}

int card_greel_mind_raker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		hand_count[player] > 1 && has_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0)
	  ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, -1, 1, 0, 0, 0, 0);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
			multidiscard(player, 2, 0);
			instance->number_of_targets = 1;
			instance->info_slot = x_value;
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			new_multidiscard(instance->targets[0].player, instance->info_slot, DISC_RANDOM, player);
		}
	}

	return 0;
}

int card_gulf_squid(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		new_manipulate_all(player, card, instance->targets[0].player, &this_test, ACT_TAP);
	}

	return 0;
}

int card_heightened_awareness(int player, int card, event_t event){

	if( current_turn == player && event == EVENT_DRAW_PHASE ){
		event_result++;
	}

	if( comes_into_play(player, card, event) ){
		discard_all(player);
	}

	return global_enchantment(player, card, event);
}

int card_lesser_gargadon(int player, int card, event_t event){
	/*
	  Lesser Gargadon |2|R|R
	  Creature - Beast 6/4
	  Whenever Lesser Gargadon attacks or blocks, sacrifice a land.
	*/
	if( ! is_humiliated(player, card) ){
		if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) || blocking(player, card, event) ){
			impose_sacrifice(player, card, player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}

int card_infernal_genesis(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int *deck = deck_ptr[current_turn];
		if( deck[0] != -1 ){
			int cmc = get_cmc_by_id(cards_data[deck[0]].id);
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_MINION, &token);
			token.t_player = current_turn;
			token.pow = 1;
			token.tou = 1;
			token.qty = cmc;
			generate_token(&token);
			mill(current_turn, 1);
		}
	}

	return global_enchantment(player, card, event);
}

int card_jolrael_empress_of_beasts(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		hand_count[player] > 1 && has_mana_for_activated_ability(player, card, 2, 0, 0, 1, 0, 0)
	  ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
			tap_card(player, card);
			multidiscard(player, 2, 0);
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			land_animation2(player, instance->parent_card, instance->targets[0].player, -1, 1, 3, 3, 0, 0, 0, &this_test);
		}
	}

	return 0;
}

int card_keldon_firebombers(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int i;
		for(i=0; i<2; i++){
			int amount = count_subtype(i, TYPE_LAND, -1);
			while( amount > 3 ){
					impose_sacrifice(player, card, i, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
					amount--;
			}
		}
	}

	return 0;
}

int card_latulla_keldon_overseer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && hand_count[player] > 1 ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 1, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, -1, 0, 0, 0, 1, 0);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			multidiscard(player, 2, 0);
			instance->number_of_targets = 1;
			instance->info_slot = x_value;
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, instance->info_slot);
		}
	}

	return 0;
}

int card_mageta_the_lion(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		hand_count[player] > 1 && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 2)
	  ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 2);
		if( spell_fizzled != 1 ){
			multidiscard(player, 2, 0);
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		new_manipulate_all(player, instance->parent_card, 2, &this_test, KILL_BURY);
	}

	return 0;
}

int card_mungha_wurm(int player, int card, event_t event){

	if( current_turn == player ){
		return card_winter_orb(player, card, event);
	}

	return 0;
}

int card_overburden(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.illegal_abilities = 0;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, 2, 2, TYPE_CREATURE, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0) ){
		td.allowed_controller = instance->targets[1].player;
		td.preferred_controller = instance->targets[1].player;
		td.who_chooses = instance->targets[1].player;
		if( pick_target(&td, "TARGET_LAND")  ){
			instance->number_of_targets = 1;
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return global_enchantment(player, card, event);
}

// pigmy razorback -> vanilla

int card_panic_attack(int player, int card, event_t event){
	/*
	  Panic Attack |2|R
	  Sorcery
	  Up to three target creatures can't block this turn.
	*/

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		int i;
		for(i = 0; i < instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 0,0, 0,SP_KEYWORD_CANNOT_BLOCK);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 3, NULL);
}

int card_plague_wind(int player, int card, event_t event){
	/*
	  Plague Wind |7|B|B
	  Sorcery
	  Destroy all creatures you don't control. They can't be regenerated.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, 1-player, &this_test, KILL_BURY);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_rethink(int player, int card, event_t event){
	/* Rethink	|2|U
	 * Instant
	 * Counter target spell unless its controller pays |X, where X is its converted mana cost. */

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
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, get_cmc(instance->targets[0].player, instance->targets[0].card));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_reveille_squad(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && current_turn != player && instance->targets[1].player != 66 && current_phase == PHASE_BEFORE_BLOCKING ){
		if( ! is_tapped(player, card) && count_attackers(current_turn) > 0 ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->targets[1].player = 66;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_rhystic_circle(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.required_type = 0;
	td1.special = TARGET_SPECIAL_DAMAGE_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_DAMAGE_PREVENTION, 1, 0, 0, 0, 0, 0, 0, &td1, "TARGET_DAMAGE");
	}
	else if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_DAMAGE_PREVENTION, 1, 0, 0, 0, 0, 0, 0, &td1, "TARGET_DAMAGE");
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			charge_mana(1-player, COLOR_COLORLESS, 1);
			if( spell_fizzled == 1 ){
				card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				target->info_slot = 0;
			}
	}
	return global_enchantment(player, card, event);
}

int card_rhystic_deluge(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
		return can_target(&td1);
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td1, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td1) ){
				charge_mana(1-player, COLOR_COLORLESS, 1);
				if( spell_fizzled == 1 ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
			}
	}
	return global_enchantment(player, card, event);
}

int card_rhystic_study(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, 1-player, 1+player, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		charge_mana(1-player, COLOR_COLORLESS, 1);
		if( spell_fizzled == 1 ){
			draw_cards(player, 1);
		}
	}
	return global_enchantment(player, card, event);
}

int card_rhystic_tutor(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( has_mana(1-player, COLOR_COLORLESS, 2) ){
				ai_modifier-=1000;
			}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			charge_mana(1-player, COLOR_COLORLESS, 2);
			if( spell_fizzled == 1 ){
				global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_searing_wind(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 10);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

static int shield_dancer_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 ){
				if( source->damage_target_player == instance->targets[1].player && source->damage_target_card == instance->targets[1].card ){
					source->damage_target_player = instance->targets[0].player;
					source->damage_target_card = instance->targets[0].card;
					kill_card(player, card, KILL_REMOVE);
				}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_shield_dancer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 1) ){
		if( instance->blocking < 255 ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = instance->blocking;
			instance->number_of_targets = 1;
			return would_valid_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 1);
		if( spell_fizzled != 1 ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = instance->blocking;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int legacy = create_targetted_legacy_effect(player, instance->parent_card, &shield_dancer_effect, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *trg = get_card_instance(player, legacy);
			trg->targets[1].player = player;
			trg->targets[1].card = instance->parent_card;
			trg->number_of_targets = 2;
		}
	}

	return 0;
}

int card_snag(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a Forest card.");
		this_test.zone = TARGET_ZONE_HAND;
		this_test.subtype = SUBTYPE_FOREST;	// can't hack in time

		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	else if( event == EVENT_CAN_CAST ){
			return 1;
	}

	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
				int choice = 0;
				if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
					choice = do_dialog(player, player, card, -1, -1, " Discard a Forest\n Play normally\n Cancel", 1);
				}

				if( choice == 0 ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_LAND, "Select a Forest card.");
					this_test.zone = TARGET_ZONE_HAND;
					this_test.subtype = SUBTYPE_FOREST;	// can't hack in time

					int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected > -1 ){
						discard_card(player, selected);
					}
					else{
						spell_fizzled = 1;
					}
				}
				else if( choice == 1 ){
						charge_mana_from_id(player, card, event, get_id(player, card));
				}
				else{
					spell_fizzled = 1;
				}
			}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			fog_special(player, card, ANYBODY, FOG_COMBAT_DAMAGE_ONLY | FOG_UNBLOCKED_CREATURES_ONLY);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_spiketail_drake2(int player, int card, event_t event){
	/* Spiketail Drake	|3|U|U
	 * Creature - Drake 3/3
	 * Flying
	 * Sacrifice ~: Counter target spell unless its controller pays |3. */

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, &td, 0, 3);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_SPELL_ON_STACK, 0, 0, 0, 0, 0, 0, 0, &td, NULL);
}

int card_spiketail_hatchling(int player, int card, event_t event){
	/* Spiketail Hatchling	|1|U
	 * Creature - Drake 1/1
	 * Flying
	 * Sacrifice ~: Counter target spell unless its controller pays |1. */

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	if( event == EVENT_ACTIVATE ){
		if( has_mana(card_on_stack_controller, COLOR_COLORLESS, 1) ){
			ai_modifier-=20;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, &td, 0, 1);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_SPELL_ON_STACK, 0, 0, 0, 0, 0, 0, 0, &td, NULL);
}

int card_spitting_spider(int player, int card, event_t event){
	/*
	  Spitting Spider English |3|G|G
	  Creature - Spider 3/5
	  Reach (This creature can block creatures with flying.)
	  Sacrifice a land: Spitting Spider deals 1 damage to each creature with flying.
	*/

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( ! controller_sacrifices_a_permanent(player, card, TYPE_LAND, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.keyword = KEYWORD_FLYING;
		new_damage_all(player, card, ANYBODY, 1, 0, &test);
	}

	return 0;
}

int card_spore_frog(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		fog_effect(player, card);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_squirrel_wrangler(int player, int card, event_t event){
	/* Squirrel Wrangler	|2|G|G
	 * Creature - Human Druid 2/2
	 * |1|G, Sacrifice a land: Put two 1/1 |Sgreen Squirrel creature tokens onto the battlefield.
	 * |1|G, Sacrifice a land: Squirrel creatures get +1/+1 until end of turn. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card,1, 0, 0, 1, 0, 0) ){
		return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 ){
			if( sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				int choice = 0;
				int ai_choice = 0;
				if( current_phase == PHASE_AFTER_BLOCKING ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Generate 2 Squirrels\n Pump your Squirrels", ai_choice);
				instance->info_slot = 66+choice;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			generate_tokens_by_id(player, card, CARD_ID_SQUIRREL, 2);
		}
		if( instance->info_slot == 67 ){
			pump_subtype_until_eot(player, instance->parent_card, 2, SUBTYPE_SQUIRREL, 1, 1, 0, 0);
		}
	}

	return 0;
}

int card_sword_dancer(int player, int card, event_t event){
	/*
	  Sword Dancer |1|W
	  Creature - Human Rebel 1/2
	  {W}{W}: Target attacking creature gets -1/-0 until end of turn.
	*/
	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, 0);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_W(2), 0, &td, "Select target attacking creature.");
}

int card_thrive(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int trgs = 0;
			while( can_target(&td) && has_mana(player, COLOR_COLORLESS, trgs) ){
					if( new_pick_target(&td, "TARGET_CREATURE", trgs, 0) ){
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
			if( trgs < 1 ){
				spell_fizzled = 1;
			}
			else{
				charge_mana(player, COLOR_COLORLESS, trgs);
				if( spell_fizzled != 1 ){
					instance->info_slot = trgs;
				}
			}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<instance->info_slot; i++){
				if( validate_target(player, card, &td, i) ){
					add_1_1_counter(instance->targets[i].player, instance->targets[i].card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_troublesome_spirit(int player, int card, event_t event){

	if( current_turn == player && eot_trigger(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		new_manipulate_all(player, card, player, &this_test, ACT_TAP);
	}

	return 0;
}

// veteran brawlers --> branded brawlers

int card_vitalizing_wind(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 7, 7, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_well_of_discovery(int player, int card, event_t event){

	if(current_turn == player && trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player){
		if( ! control_an_untapped_land(player) ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					draw_cards(player, 1);
			}
		}
	}
	return 0;
}

int card_well_of_life(int player, int card, event_t event){

	if(current_turn == player && trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player){
		if( ! control_an_untapped_land(player) ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					gain_life(player, 2);
			}
		}
	}
	return 0;
}

int card_whip_sergeant(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player,  instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 0, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_wild_might(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST  ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				pump_until_eot(player, card, instance->targets[0].player,  instance->targets[0].card, 1, 1);
				charge_mana(1-instance->targets[0].player, COLOR_COLORLESS, 2);
				if( spell_fizzled == 1 ){
					pump_until_eot(player, card, instance->targets[0].player,  instance->targets[0].card, 4, 4);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_withdraw(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST  ){
		return target_available(player, card, &td) >= 2;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				state_untargettable(instance->targets[0].player,  instance->targets[0].card, 1);
				if( ! new_pick_target(&td, "TARGET_CREATURE", 1, 0) ){
					spell_fizzled = 1;
				}
				state_untargettable(instance->targets[0].player,  instance->targets[0].card, 0);
			}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 0) ){
				bounce_permanent(instance->targets[0].player,  instance->targets[0].card);
			}
			if( validate_target(player, card, &td, 1) ){
				charge_mana(instance->targets[1].player, COLOR_COLORLESS, 1);
				if( spell_fizzled == 1 ){
					bounce_permanent(instance->targets[1].player,  instance->targets[1].card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}
