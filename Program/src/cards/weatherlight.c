#include "manalink.h"

// ---- GLOBAL FUNCTION

static int remove_top_creature_card_from_grave( int player ){
	const int *grave = get_grave(player);
	int count = count_graveyard(player)-1;
	while(count > -1){
			if( cards_data[grave[count]].type & TYPE_CREATURE ){
				rfg_card_from_grave(player, count);
				return 1;
			}
			count--;
	}
	return 0;
}

static int generic_creature_with_discard_type_on_cip(int player, int card, event_t event, int selected_type){

	target_definition_t td;
	default_target_definition(player, card, &td, selected_type);
	td.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! can_target(&td) ){
			ai_modifier -= 1000;
		}
	}
	else if( comes_into_play(player, card, event) ){
			 int kill = 1;
			 if( can_target(&td) ){
				 if( IS_AI(player) ){
					 int count = 0;
					 while(count < active_cards_count[player] ){
						   if( in_hand(player, count) && is_what(player, count, selected_type) ){
							   discard_card(player, count);
							   kill = 0;
							   break;
						   }
						   count++;
					 }
				 }
				 else{
					  if( pick_target(&td, "TARGET_CARD") ){
						  discard_card(player, instance->targets[0].card);
						  kill = 0;
					  }
				 }
			 }
			 if( kill == 1 ){
				 kill_card(player, card, KILL_SACRIFICE);
			 }
	}

	return 0;
}

// ---- CARDS

int card_abeyance(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_PERMANENT);
				new_manipulate_all(player, card, instance->targets[0].player, &this_test, ACT_DISABLE_NONMANA_ACTIVATED_ABILITIES);
				int spc = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//64
				card_instance_t *leg = get_card_instance(player, spc);
				leg->targets[2].player = 64;
				leg->targets[3].player = instance->targets[0].player;
				leg->targets[2].card = get_id(player, card);
				create_card_name_legacy(player, spc, get_id(player, card));
				draw_cards(player, 1);
			}
			kill_card(player, card, KILL_DESTROY);
	}

   return 0;
}


int card_abjure(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		if( counterspell(player, card, event, NULL, 0) ){
			if( can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, 0, 0, COLOR_TEST_BLUE, 0, 0, 0, -1, 0) ){
				return 99;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, 0, 0, COLOR_TEST_BLUE, 0, 0, 0, -1, 0) ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		return counterspell(player, card, event, NULL, 0);
	}

   return 0;
}

static int can_have_m1_m1_counters(int player, int card, int number_of_age_counters)
{
  return !check_battlefield_for_id(player, CARD_ID_MELIRA_SYLVOK_OUTCAST);
}
static int add_m1_m1_counters_thunk(int player, int card, int number_of_age_counters)
{
  add_counters(player, card, COUNTER_M1_M1, number_of_age_counters);
  return 1;
}
int card_aboroth(int player, int card, event_t event){

  /* Aboroth	|4|G|G
   * Creature - Elemental 9/9
   * Cumulative upkeep-Put a -1/-1 counter on ~. */

  cumulative_upkeep_general(player, card, event, can_have_m1_m1_counters, add_m1_m1_counters_thunk);
  return 0;
}

int card_abyssal_gatekeeper(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_agonizing_memories(int player, int card, event_t event){

	/* Agonizing Memories	|2|B|B
	 * Sorcery
	 * Look at target player's hand and choose two cards from it. Put them on top of that player's library in any order. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){

				ec_definition_t ec;
				default_ec_definition(instance->targets[0].player, player, &ec);
				ec.effect = EC_PUT_ON_TOP;
				ec.qty = 2;

				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);

				new_effect_coercion(&ec, &this_test);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_alabaster_dragon(int player, int card, event_t event)
{
  // When ~ dies, shuffle it into its owner's library.
	  int owner, position;
  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)
	  && find_in_owners_graveyard(player, card, &owner, &position))
	from_graveyard_to_deck(owner, position, 3);

  return 0;
}

int card_alms2(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_CREATURE;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( count_graveyard(player) > 0 ){
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_X(1), 0, &td, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if(	charge_mana_for_activated_ability(player, card, MANACOST_X(1)) && pick_target(&td, "TARGET_DAMAGE") ){
			rfg_card_from_grave(player, count_graveyard(player)-1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( target->info_slot > 0 ){
				target->info_slot--;
			}
		}
	}

	return 0;
}

int card_ancestral_knowledge(int player, int card, event_t event){

	/* Ancestral Knowledge	|1|U
	 * Enchantment
	 * Cumulative upkeep |1
	 * When ~ enters the battlefield, look at the top ten cards of your library, then exile any number of them and put the rest back on top of your library in
	 * any order.
	 * When ~ leaves the battlefield, shuffle your library. */

	cumulative_upkeep(player, card, event, MANACOST_X(1));	// AI probably won't pay upkeep ever, but it doesn't ever exile or rearrange cards, either.

	if (comes_into_play(player, card, event)){
		int *deck = deck_ptr[player];

		int amount = MIN(10, count_deck(player));

		if (!IS_AI(player)){
			while( amount ){
					int selected = show_deck( player, deck, amount, "Choose a card to remove", 0, 0x7375B0);
					if( selected != -1 ){
						rfg_card_in_deck(player, selected);
						amount--;
					}
					else{
						break;
					}
			}
		}
		rearrange_top_x(player, player, amount);
	}

	if( leaves_play(player, card, event) ){
		shuffle(player);
	}

	return global_enchantment(player, card, event);
}

int card_angelic_renewal(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	count_for_gfp_ability_and_store_values(player, card, event, player, TYPE_CREATURE, NULL, GFPC_TRACK_DEAD_CREATURES | GFPC_EXTRA_SKIP_TOKENS, 0);

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		state_untargettable(player, card, 1); //Prevent unwanted interactions
		int added[20];
		int ac = 0;
		int k;
		for(k=0; k<10; k++){
			if( instance->targets[k].player != -1 ){
				int result = seek_grave_for_id_to_reanimate(player, -1, player, cards_data[instance->targets[k].player].id, REANIMATEXTRA_LEAVE_IN_GRAVEYARD);
				if( result != -1 ){
					remove_card_from_grave(player, result);
					added[ac] = instance->targets[k].player;
					ac++;
				}
				instance->targets[k].player = -1;
			}
			if( instance->targets[k].card != -1 ){
				int result = seek_grave_for_id_to_reanimate(player, -1, player, cards_data[instance->targets[k].card].id, REANIMATEXTRA_LEAVE_IN_GRAVEYARD);
				if( result != -1 ){
					remove_card_from_grave(player, result);
					added[ac] = instance->targets[k].card;
					ac++;
				}
			}
			instance->targets[k].card = -1;
		}
		int	csvid = -1;
		if( ac ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature to reanimate.");
			int selected = select_card_from_zone(player, player, added, ac, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				csvid = cards_data[added[selected]].id;
			}
		}
		if( csvid > -1 ){
			seek_grave_for_id_to_reanimate(player, card, player, csvid, REANIMATE_DEFAULT);
			kill_card(player, card, KILL_SACRIFICE);
		}
		instance->targets[11].player = 0;
		state_untargettable(player, card, 0);
	}

	return global_enchantment(player, card, event);
}

int card_argivian_find(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST && (count_graveyard_by_type(player, TYPE_ENCHANTMENT) > 0 || count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 ) ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		char buffer[100] = "Select an Artifact or Enchantment card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT | TYPE_ARTIFACT, buffer);
		if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1 ){
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_SPELL){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			const int *grave = get_grave(player);
			add_card_to_hand(player, grave[selected]);
			remove_card_from_grave(player, selected);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_argivian_restoration(int player, int card, event_t event){

  card_instance_t *instance = get_card_instance(player, card);

  if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_ARTIFACT) ){
	  return 1;
  }

  else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			const int *grave = get_grave(player);
			int selected = select_a_card(player, player, 2, 1, 2, -1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			instance->targets[0].card = selected;
			instance->targets[1].card = grave[selected];
  }
  else if( event == EVENT_RESOLVE_SPELL){
		   int selected = instance->targets[0].card;
		   const int *grave = get_grave(player);
		   if( grave[selected] == instance->targets[1].card ){
			   reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		   }
		   kill_card(player, card, KILL_DESTROY);
  }
  return 0;
}

int card_aether_flash(int player, int card, event_t event){

  card_instance_t *instance = get_card_instance(player, card);

  if( event == EVENT_CAN_CAST ){
	  return 1;
  }

  if( specific_cip(player, card, event, 2, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
	  damage_creature(instance->targets[1].player, instance->targets[1].card, 2, player, card);
  }

  return global_enchantment(player, card, event);
}

int card_aura_of_silence(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL && in_play(player, card) && ! is_humiliated(player, card) ){
		if( affected_card_controller == 1-player && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT | TYPE_ENCHANTMENT) &&
			! is_what(affected_card_controller, affected_card, TYPE_LAND)
		  ){
			COST_COLORLESS += 2;
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, &td, "DISENCHANT");
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, &td, "DISENCHANT");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_barishi(int player, int card, event_t event){

	// When ~ dies, exile ~, then shuffle all creature cards from your graveyard into your library.
	if( this_dies_trigger(player, card, event, 2) ){
		exile_from_owners_graveyard(player, card);
		int count = count_graveyard(player)-1;
		const int *grave = get_grave(player);
		while(count > -1){
			if( cards_data[grave[count]].type & TYPE_CREATURE ){
				from_graveyard_to_deck(player, count, 2);
			}
			count--;
		}
		shuffle(player);
	}

	return 0;
}

int card_barrow_ghoul(int player, int card, event_t event){

	/* Barrow Ghoul	|1|B
	 * Creature - Zombie 4/4
	 * At the beginning of your upkeep, sacrifice ~ unless you exile the top creature card of your graveyard. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( any_in_graveyard_by_type(player, TYPE_CREATURE) ){
			int choice = do_dialog(player, player, card, -1, -1, " Pay upkeep\n Decline", 0);
			if( choice == 0 && remove_top_creature_card_from_grave(player) ){
				kill = 0;
			}
		}

		if( kill != 0 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_bethrothed_of_fire(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( can_target(&td) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				return 1;
			}
			if( can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				return 1;
			}
		}

		if( event == EVENT_ACTIVATE  ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				int choice = 0;
				int ai_choice = 0;
				if( can_target(&td) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					if( can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
						if( is_attacking(instance->damage_target_player, instance->damage_target_card) &&
							! is_unblocked(instance->damage_target_player, instance->damage_target_card) && count_attackers(instance->damage_target_player) > 1
						  ){
							ai_choice = 1;
						}
						choice = do_dialog(player, player, card, -1, -1, " Sac to pump this\n Sac this to pump everyone\n Cancel", ai_choice);
					}
				}
				else{
					choice = 1;
				}
				if( choice == 0 ){
					if( pick_target(&td, "LORD_OF_THE_PIT") ){
						kill_card(player, instance->targets[0].card, KILL_SACRIFICE);
						instance->info_slot = 66+choice;
					}
				}
				else if( choice == 1 ){
						kill_card(player, card, KILL_SACRIFICE);
						instance->info_slot = 66+choice;
				}
				else{
					 spell_fizzled = 1;
				}
			}
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[2].card == 66 ){
				pump_until_eot(player, instance->parent_card, instance->damage_target_player, instance->damage_target_card, 2, 0);
			}
			if( instance->targets[2].card == 67 ){
				pump_subtype_until_eot(player, card, player, -1, 2, 0, 0, 0);
			}
		}
	}
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_blossoming_wreath(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, count_graveyard_by_type(player, TYPE_CREATURE));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bone_dancer(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && is_attacking(player, card) && is_unblocked(player, card) &&
		current_phase == PHASE_AFTER_BLOCKING && instance->info_slot != 66 &&
		count_graveyard_by_type(1-player, TYPE_CREATURE) ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE){
			instance->info_slot = 66;
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			negate_combat_damage_this_turn(player, instance->parent_card, player, instance->parent_card, 0);

			const int *grave = get_grave(1-player);
			int count = count_graveyard(1-player)-1;
			while(count > -1){
				if( cards_data[grave[count]].type & TYPE_CREATURE ){
					reanimate_permanent(player, card, 1-player, count, REANIMATE_DEFAULT);
					break;
				}
				count--;
			}

	}


	if( instance->info_slot == 66 && current_phase == PHASE_UPKEEP ){
		instance->info_slot = 0;
	}

	return 0;
}

static int bosium_strip_legacy(int player, int card, event_t event)
{
  if (event == EVENT_CLEANUP )
	kill_card(player, card, KILL_REMOVE);

  if (!IS_ACTIVATING(event))
	return 0;

  if (event == EVENT_CAN_ACTIVATE)
	{
		const int *grave = get_grave(player);
		int top_card = count_graveyard(player) - 1;
		if( grave[0] != -1 && is_what(-1, grave[top_card], TYPE_SPELL) )
			if( has_mana_to_cast_iid(player, event, grave[top_card]) )
				return can_legally_play_iid(player, grave[top_card]);
	}

  if (event == EVENT_ACTIVATE )
	{
		const int *grave = get_grave(player);
		int top_card = count_graveyard(player) - 1;
		int id = cards_data[grave[top_card]].id;
		if( charge_mana_from_id(player, -1, event, id) )
		  {
			play_card_in_grave_for_free_and_exile_it(player, player, top_card);
			cant_be_responded_to = 1;	// already had opportunity to respond to the spell; no response to this fake activation
		  }
	}

  return 0;
}

int card_bosium_strip(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		create_legacy_activate(instance->parent_controller, instance->parent_card, &bosium_strip_legacy);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 3, 0, 0, 0, 0, 0, 0, NULL, NULL);
}

int card_briar_shield(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}
		if( event == EVENT_ACTIVATE  ){
			return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_until_eot(player, card, instance->damage_target_player, instance->damage_target_card, 3, 3);
		}
	}
	return generic_aura(player, card, event, player, 1, 1, 0, 0, 0, 0, 0);
}

int card_bubble_matrix(int player, int card, event_t event ){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

		if( damage_card != source->internal_card_id || source->damage_target_card == -1 || source->info_slot < 1){
			return 0;
		}

		else{
			 source->info_slot = 0;
		}
	}

	return 0;
}

int card_buried_alive(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		char msg[100] = "Select a creature card to put into grave.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.qty = 3;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GRAVE, 0, AI_GOOD_TO_PUT_IN_GRAVE, &this_test);
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_call_of_the_wild(int player, int card, event_t event)
{
  // 0x404fc0

  /* Call of the Wild	|2|G|G
   * Enchantment
   * |2|G|G: Reveal the top card of your library. If it's a creature card, put it onto the battlefield. Otherwise, put it into your graveyard. */

  if (global_enchantment(player, card, event))
	return 1;

  if (event == EVENT_RESOLVE_ACTIVATION && deck_ptr[player][0] != -1)
	{
	  reveal_card_iid(player, card, deck_ptr[player][0]);

	  if (is_what(-1, deck_ptr[player][0], TYPE_CREATURE))
		put_into_play_a_card_from_deck(player, player, 0);
	  else
		mill(player, 1);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_XG(2,2), 0, NULL, NULL);
}

int card_chimeric_sphere(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 2, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 2, 0, 0, 0, 0, 0, 0, 0, 0) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allowed_controller = 1-player;
			td.required_abilities = KEYWORD_FLYING;
			td.illegal_abilities = 0;
			int ai_choice = can_target(&td) ? 0 : 1;

			int choice = do_dialog(player, player, card, -1, -1, " 2/1 with Flying\n 3/2 without Flying\n Do nothing", ai_choice);
			if( choice < 2 ){
				instance->targets[1].card = 66+choice;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int pow = instance->targets[1].card == 66 ? 2 : 3;
			int tou = instance->targets[1].card == 66 ? 1 : 2;
			int key = instance->targets[1].card == 66 ? KEYWORD_FLYING : 0;
			add_a_subtype(player, instance->parent_card, SUBTYPE_CONSTRUCT);
			artifact_animation(player, instance->parent_card, player, instance->parent_card, 1, pow, tou, key, 0);
	}

	return 0;
}

int card_circling_vultures(int player, int card, event_t event){

	/* Circling Vultures	|B
	 * Creature - Bird 3/2
	 * Flying
	 * You may discard ~ any time you could cast an instant.
	 * At the beginning of your upkeep, sacrifice ~ unless you exile the top creature card of your graveyard. */

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
		return 1;
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
		discard_card(player, card);
		return 2;
	}

	return card_barrow_ghoul(player, card, event);
}

int card_cinder_wall(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		if( blocking(player, card, event) ){
			get_card_instance(player, card)->info_slot = 66;
		}
		if( get_card_instance(player, card)->info_slot == 66 && end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
			kill_card(player, card, KILL_DESTROY);
		}
	}
	if( event == EVENT_CLEANUP ){
		get_card_instance(player, card)->info_slot = 0;
	}
	return 0;
}
int card_debt_of_loyalty(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
			if( player != instance->targets[0].player ){
				gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET+GS_REGENERATION, &td, "TARGET_CREATURE", 1, NULL);
}

int card_dense_foliage(int player, int card, event_t event){
	/*
	if( event == EVENT_RESOLVE_SPELL ){
		set_special_flags2(player, -1, SF2_DENSE_FOLIAGE);
	}

	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		this_test.id = get_id(player, card);
		this_test.not_me = 1;
		if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			remove_special_flags2(player, -1, SF2_DENSE_FOLIAGE);
		}
	}

	if( event == EVENT_CAST_SPELL && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		set_special_flags2(affected_card_controller, affected_card, SF2_DENSE_FOLIAGE);
	}
	*/
	return global_enchantment(player, card, event);
}

int card_dingus_staff(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		card_instance_t *instance= get_card_instance(player, card);
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
			if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
				card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
				if( affected->kill_code > 0 && affected->kill_code < 4 ){
					if( instance->targets[affected_card_controller].player < 0 ){
						instance->targets[affected_card_controller].player = 0;
					}
					instance->targets[affected_card_controller].player+=2;
				}
			}
		}

		if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
			affect_me(player, card ) && (instance->targets[0].player > 0 || instance->targets[1].player > 0)
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( instance->targets[player].player > 0 ){
						damage_player(player, instance->targets[player].player, player, card);
					}
					if( instance->targets[1-player].player > 0 ){
						damage_player(1-player, instance->targets[1-player].player, player, card);
					}
					instance->targets[0].player = instance->targets[1].player = 0;
			}
			else if (event == EVENT_END_TRIGGER){
					instance->targets[0].player = instance->targets[1].player = 0;
			}
		}
	}

	return 0;
}

int card_disrupt(int player, int card, event_t event)
{
  target_definition_t td;
  counterspell_target_definition(player, card, &td, TYPE_INSTANT | TYPE_INTERRUPT | TYPE_SORCERY);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (counterspell_resolve_unless_pay_x(player, card, &td, 0, 1))
		draw_a_card(player);

	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}
  else
	return counterspell(player, card, event, &td, 0);
}

int card_doomsday(int player, int card, event_t event)
{
  /* Doomsday	|B|B|B
   * Sorcery
   * Search your library and graveyard for five cards and exile the rest. Put the chosen cards on top of your library in any order. You lose half your life,
   * rounded up. */

	if (event == EVENT_RESOLVE_SPELL){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		int cd = count_deck(player);
		int cg = count_graveyard(player);
		int cards = MIN(5, cd+cg);

		int i = 0, chosen[5] = { -1, -1, -1, -1, -1 };
		while (i < cards){
			int selected = -1;
			if( cd > 0 ){
				selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					chosen[i] = deck_ptr[player][selected];
					remove_card_from_deck(player, selected);
					i++;
					cd--;
				}
			}
			if( selected == -1 && cg > 0){
				selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					chosen[i] = get_grave(player)[selected];
					remove_card_from_grave(player, selected);
					i++;
					cg--;
				}
			}
		}

		reshuffle_grave_into_deck(player, 1);

		rfg_whole_library(player);

		for (i = 0; i < cards; ++i){
			deck_ptr[player][i] = chosen[i];
		}
		if( cards > 1 ){
			rearrange_top_x(player, player, cards);
		}
		lose_life(player, (life[player] + 1) / 2);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_downdraft(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) && can_target(&td) ){
				return 1;
			}
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
				can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
			  ){
				return 1;
			}
	}
	else if( event == EVENT_ACTIVATE ){
			int choice = 0;
			int ai_choice = 0;
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) && can_target(&td) ){
				if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
					can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
				 ){
					target_definition_t td1;
					default_target_definition(player, card, &td1, TYPE_CREATURE);
					td1.required_abilities = KEYWORD_FLYING;
					td1.toughness_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;
					if( target_available(player, card, &td1) > 1 ){
						ai_choice = 1;
					}
					choice = do_dialog(player, player, card, -1, -1, " Target loses Flying\n 2 damage to all flyers\n Pass", ai_choice);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 1-(1*choice), 0, 0) ){
					if( choice == 0 ){
						if( pick_target(&td, "TARGET_CREATURE")){
							instance->number_of_targets = 1;
							instance->info_slot = 66;
						}
					}
					else if( choice == 1 ){
								instance->info_slot = 67;
								kill_card(player, card, KILL_SACRIFICE);
					}
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				negate_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,                                                                                                                KEYWORD_FLYING);
			}
			else if( instance->info_slot == 67 ){
					damage_all(player, instance->parent_card, player, 2, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0, 0, 0, 0, -1, 0);
					damage_all(player, instance->parent_card, 1-player, 2, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
	}

	return global_enchantment(player, card, event);
}

int card_dwarven_berserk(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) && ! is_unblocked(player, card) ){
		pump_ability_until_eot(player, card, player, card, 3, 0, KEYWORD_TRAMPLE, 0);
	}

	return 0;
}

int card_dwarven_thaumaturgist(int player, int card, event_t event)
{
  // |T: Switch target creature's power and toughness until end of turn.
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = ANYBODY;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	switch_power_and_toughness_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_CREATURE");
}

int card_empyrial_armor(int player, int card, event_t event){
	return generic_aura(player, card, event, player, hand_count[player], hand_count[player], 0, 0, 0, 0, 0);
}

int spatial_binding_legacy(int player, int card, event_t event);

int card_ertais_familiar(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	int ph = phasing(player, card, event);
	if (ph){
		return ph;
	}

	if (leaves_play(player, card, event)){
		mill(player, 3);	// also in phase_out_impl()
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		set_special_flags(player, instance->parent_card, SF_CANNOT_PHASE_OUT);
		create_targetted_legacy_effect(player, instance->parent_card, &spatial_binding_legacy, player, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0);
}

int card_fallow_wurm(int player, int card, event_t event){

	return generic_creature_with_discard_type_on_cip(player, card, event, TYPE_LAND);
}

int card_fervor(int player, int card, event_t event)
{
  // Creatures you control have haste.
  boost_subtype(player, card, event, -1, 0,0, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

  return global_enchantment(player, card, event);
}

int card_festering_evil(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		APNAP(p, {new_damage_all(player, card, p, 1, NDA_PLAYER_TOO | NDA_ALL_CREATURES, NULL);};);
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		APNAP(p, {new_damage_all(player, card, p, 3, NDA_PLAYER_TOO | NDA_ALL_CREATURES, NULL);};);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_B(2), 0, NULL, NULL);
}

int card_fire_whip(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		target_definition_t td2;
		default_target_definition(t_player, t_card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		if( event == EVENT_CAN_ACTIVATE ){
			if( generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td2, "TARGET_CREATURE_OR_PLAYER")
			  ){
				return 1;
			}
			return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE_OR_PLAYER");
		}

		if( event == EVENT_ACTIVATE  ){
			instance->info_slot = 1;
			int choice = 0;
			if( generic_activated_ability(t_player, t_card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td2, "TARGET_CREATURE_OR_PLAYER")
			  ){
				if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0,
												&td1, "TARGET_CREATURE_OR_PLAYER")
				  ){
				   choice = do_dialog(player, player, card, -1, -1, " Tap enchanted creature\n Sacrifice Fire Whip\n Cancel", 0);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 0 ){
				instance->info_slot = 66;
				return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td2, "TARGET_CREATURE_OR_PLAYER");
			}
			else if( choice == 1 ){
					instance->info_slot = 67;
					return generic_activated_ability(player, card, EVENT_ACTIVATE, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0,
													&td1, "TARGET_CREATURE_OR_PLAYER");
			}
			else{
				spell_fizzled = 1;
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 ){
				if( validate_target(t_player, t_card, &td2, 0) ){
					damage_creature_or_player(t_player, t_card, event, 1);
				}
			}
			if( instance->info_slot == 67 && valid_target(&td1) ){
				damage_creature_or_player(player, card, event, 1);
			}
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_firestorm(int player, int card, event_t event)
{
  /* Firestorm	|R
   * Instant
   * As an additional cost to cast ~, discard X cards.
   * ~ deals X damage to each of X target creatures and/or players. */

  if (event == EVENT_CAN_CAST)
	return 1;	// can always cast for X == 0

  if (!IS_CASTING(player, card, event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
  td.allow_cancel = 3;

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  if (!is_token(player, card))
		{
		  int max_targets = target_available(player, card, &td);
		  max_targets = MIN(max_targets, hand_count[player]);

		  int chosen = pick_up_to_n_targets(&td, "TARGET_CREATURE_OR_PLAYER", max_targets);
		  if (cancel != 1 && chosen > 0)
			{
			  instance->info_slot = chosen;
			  multidiscard(player, chosen, 0);
			}
		}
	  else
		{
		  td.allow_cancel = 1;
		  pick_up_to_n_targets(&td, "TARGET_CREATURE_OR_PLAYER", instance->info_slot);
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (instance->info_slot > 0)
		{
		  int num_validated = 0;

		  int i;
		  for (i = 0; i < instance->info_slot; ++i)
			if (validate_target(player, card, &td, i))
			  {
				if (player == AI && instance->targets[i].player == AI)
				  ai_modifier -= 32;

				++num_validated;
				damage_creature(instance->targets[i].player, instance->targets[i].card, instance->info_slot, player, card);
			  }

		  if (num_validated == 0)
			spell_fizzled = 1;
		}
	  else if (player == AI)
		ai_modifier -= 64;

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_flux(int player, int card, event_t event){

	/* Flux	|2|U
	 * Sorcery
	 * Each player discards any number of cards, then draws that many cards.
	 * Draw a card. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL){
			int i;
			for(i=0; i<2; i++){
				int discarded = choose_a_number(i, "Discard how many cards?", hand_count[i]/2);
				if( discarded > hand_count[i] ){
					discarded = hand_count[i];
				}
				new_multidiscard(i, discarded, 0, player);
				draw_cards(i, discarded);
			}
			draw_cards(player, 1);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_fog_elemental(int player, int card, event_t event)
{
	// When ~ attacks or blocks, sacrifice it at end of combat.
	if( ! is_humiliated(player, card) && (declare_attackers_trigger(player, card, event, 0, player, card) || blocking(player, card, event)) ){
		create_targetted_legacy_effect(player, card, sacrifice_at_end_of_combat, player, card);
	}

	return 0;
}

int card_gaeas_blessing(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			select_multiple_cards_from_graveyard(player, instance->targets[0].player, 0, player == instance->targets[0].player ? AI_MAX_VALUE : AI_MIN_VALUE, NULL, 3, &instance->targets[1]);
		}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int i, any_shuffled_in = 0, num_targets = 0;
			for (i = 1; i <= 3; ++i){
				if( instance->targets[i].player != -1 ){
					++num_targets;
					int selected = validate_target_from_grave(player, card, instance->targets[0].player, i);
					if (selected != -1){
						from_graveyard_to_deck(instance->targets[0].player, selected, 2);
						any_shuffled_in = 1;
					}
				}
			}
			if (any_shuffled_in){
				shuffle(instance->targets[0].player);
			}
			if (any_shuffled_in	// a card was targeted and it was still legal
				|| valid_target(&td)){	// the targeted player was still legal
				draw_cards(player, 1);
			}
			kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

static int can_pay_1_life_thunk(int player, int card, int number_of_age_counters)
{
  return can_pay_life(player, number_of_age_counters);
}
static int lose_1_life_thunk(int player, int card, int number_of_age_counters)
{
  lose_life(player, number_of_age_counters);
  return 1;
}
int card_gallowbraid(int player, int card, event_t event)
{
  /* Gallowbraid	|3|B|B
   * Legendary Creature - Horror 5/5
   * Trample
   * Cumulative upkeep-Pay 1 life. */
  /* Morinfen	|3|B|B
   * Legendary Creature - Horror 5/4
   * Flying
   * Cumulative upkeep-Pay 1 life. */

  check_legend_rule(player, card, event);
  cumulative_upkeep_general(player, card, event, can_pay_1_life_thunk, lose_1_life_thunk);
  return 0;
}

int card_gemstone_mine(int player, int card, event_t event)
{
  if (!is_unlocked(player, card, event, 16))
	return 0;

  /* Gemstone Mine	""
   * Land
   * ~ enters the battlefield with three mining counters on it.
   * |T, Remove a mining counter from ~: Add one mana of any color to your mana pool. If there are no mining counters on ~, sacrifice it. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_MINING, 3);

  if (event == EVENT_CAN_ACTIVATE && count_counters(player, card, COUNTER_MINING) <= 0)
	return 0;	// else fall through to mana_producer()

  if (event == EVENT_ACTIVATE)
	{
	  mana_producer(player, card, event);
	  if (cancel != 1)
		{
		  remove_counter(player, card, COUNTER_MINING);
		  if (count_counters(player, card, COUNTER_MINING) <= 0)
			kill_card(player, card, KILL_SACRIFICE);
		}
	  return 0;
	}

  return mana_producer(player, card, event);
}

int card_gerrards_wisdom(int player, int card, event_t event){

  if( event == EVENT_CAN_CAST ){
	  return 1;
  }

  else if( event == EVENT_RESOLVE_SPELL){
		   gain_life(player, 2*hand_count[player]);
		   kill_card(player, card, KILL_DESTROY);
  }
  return 0;
}

int card_goblin_bomb(int player, int card, event_t event)
{
  /* Goblin Bomb	|1|R
   * Enchantment
   * At the beginning of your upkeep, you may flip a coin. If you win the flip, put a fuse counter on ~. If you lose the flip, remove a fuse counter from ~.
   * Remove five fuse counters from ~, Sacrifice ~: ~ deals 20 damage to target player. */

  if (global_enchantment(player, card, event))
	return 1;

  upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  if (flip_a_coin(player, card))
		add_counter(player, card, COUNTER_FUSE);
	  else
		remove_counter(player, card, COUNTER_FUSE);
	}

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	damage_target0(player, card, 20);

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_SACRIFICE_ME, MANACOST0, GVC_COUNTERS(COUNTER_FUSE, 5), &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_goblin_grenadiers(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_LAND);
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && is_attacking(player, card) && is_unblocked(player, card) &&
		current_phase == PHASE_AFTER_BLOCKING && can_target(&td) && can_target(&td1) ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE){
		kill_card(player, card, KILL_SACRIFICE);
		pick_target(&td, "TARGET_CREATURE");
		instance->targets[1].player = instance->targets[0].player;
		instance->targets[1].card = instance->targets[0].card;
		pick_target(&td1, "TARGET_LAND");

	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td1) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			instance->targets[0].player = instance->targets[1].player;
			instance->targets[0].card = instance->targets[1].card;
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
	}

	return 0;
}

int card_goblin_vandal(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && is_attacking(player, card) && is_unblocked(player, card) &&
		current_phase == PHASE_AFTER_BLOCKING && instance->info_slot != 66 && has_mana(player, COLOR_RED, 1) &&
		can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE){
			charge_mana(player, COLOR_RED, 1);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_ARTIFACT") ){
				instance->info_slot = 66;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			negate_combat_damage_this_turn(player, instance->parent_card, player, instance->parent_card, 0);

			if( valid_target(&td) ){
				kill_card(1-player, instance->targets[0].card, KILL_DESTROY);
			}
	}


	if( instance->info_slot == 66 && eot_trigger(player, card, event) ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_haunting_misery(int player, int card, event_t event)
{
  /* Haunting Misery	|1|B|B
   * Sorcery
   * As an additional cost to cast ~, exile X creature cards from your graveyard.
   * ~ deals X damage to target player. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  pick_target(&td, "TARGET_PLAYER");
	  if (!is_token(player, card) && cancel != 1)
		{
		  int num_exiled = 0;

		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card to exile.");

		  while (new_global_tutor(player, player, TUTOR_FROM_GRAVE_NOTARGET, TUTOR_RFG, 0, AI_MIN_VALUE, &test) != -1)
			++num_exiled;

		  get_card_instance(player, card)->info_slot = num_exiled;

		  ai_modifier += (player == AI ? 12 : -12) * (num_exiled - 2);
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (valid_target(&td))
		damage_target0(player, card, instance->info_slot);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_harvest_wurm(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( !any_in_graveyard_by_type(player, TYPE_LAND) ){
			ai_modifier -= 1000;
		}
	}
	if( comes_into_play(player, card, event) ){
			 int kill = 1;
			 int selected = -1;
			 const int *grave = get_grave(player);

			 if( !IS_AI(player) ){
				 selected = show_deck( player, grave, count_graveyard(player), "Select a land card.", 0, 0x7375B0 );
			 }

			 else{
				  int count =  0;
				  while( count < count_graveyard(player) ){
						 if( cards_data[grave[count]].type & TYPE_LAND ){
							 selected = count;
							 break;
						 }
						 count++;
				  }
			}

			if( selected != -1 && cards_data[grave[selected]].type & TYPE_LAND ){
				add_card_to_hand(player, grave[selected]);
				remove_card_from_grave(player, selected);
				kill = 0;
			}

			if( kill == 1 ){
				kill_card(player, card, KILL_SACRIFICE);
			}
	}

	return 0;
}

int card_heart_of_bogardan(int player, int card, event_t event)
{
  /* Heart of Bogardan	|2|R|R
   * Enchantment
   * Cumulative upkeep |2
   * When a player doesn't pay ~'s cumulative upkeep, ~ deals X damage to target player and each creature he or she controls, where X is twice the number of age
   * counters on ~ minus 2. */

  if (cumulative_upkeep(player, card, event, MANACOST_X(2)))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  // Some shenanigans here - resurrect the card long enough to target and deal damage, then zorch it again.
	  instance->internal_card_id = get_internal_card_id_from_csv_id(CARD_ID_HEART_OF_BOGARDAN);
	  instance->state |= STATE_INVISIBLE | STATE_CANNOT_TARGET | STATE_IN_PLAY;

	  target_definition_t td;
	  default_target_definition(player, card, &td, 0);
	  td.zone = TARGET_ZONE_PLAYERS;
	  td.allow_cancel = 0;

	  if (count_counters(player, card, COUNTER_AGE) >= 2 && can_target(&td) && pick_target(&td, "TARGET_PLAYER"))
		new_damage_all(player, card, instance->targets[0].player, count_counters(player, card, COUNTER_AGE)*2 - 2, NDA_PLAYER_TOO, NULL);

	  instance->internal_card_id = -1;
	  instance->state &= ~(STATE_INVISIBLE | STATE_CANNOT_TARGET);
	}

  return global_enchantment(player, card, event);
}

int card_heavy_ballista(int player, int card, event_t event){
	/*
	  Heavy Ballista |3|W
	  Creature - Human Soldier 2/3
	  {T}: Heavy Ballista deals 2 damage to target attacking or blocking creature.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_hidden_horror(int player, int card, event_t event){

	return generic_creature_with_discard_type_on_cip(player, card, event, TYPE_CREATURE);
}

static int can_pay_2_life_thunk(int player, int card, int number_of_age_counters)
{
  return can_pay_life(player, 2 * number_of_age_counters);
}
static int lose_2_life_thunk(int player, int card, int number_of_age_counters)
{
  lose_life(player, 2 * number_of_age_counters);
  return 1;
}
int card_inner_sanctum(int player, int card, event_t event){

	/* Inner Sanctum	|1|W|W
	 * Enchantment
	 * Cumulative upkeep-Pay 2 life.
	 * Prevent all damage that would be dealt to creatures you control. */

	cumulative_upkeep_general(player, card, event, can_pay_2_life_thunk, lose_2_life_thunk);

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

		if( damage_card != source->internal_card_id || source->damage_target_card == -1 || source->info_slot < 1 ||
			source->damage_target_player != player ){
			return 0;
		}

		else{
			 source->info_slot = 0;
		}
	}

	return global_enchantment(player, card, event);
}

int card_infernal_tribute(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 && ! sacrifice(player, card, player, 0, TYPE_PERMANENT, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_jabaris_banner(int player, int card, event_t event){

	// |1, |T: Target creature gains flanking until end of turn.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			alternate_legacy_text(2, player, pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_FLANKING));
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(1), 0, &td, "TARGET_CREATURE");
}

int card_lava_hounds(int player, int card, event_t event){
	/*
	  Lava Hounds |2|R|R
	  Creature - Hound 4/4
	  Haste
	  When Lava Hounds enters the battlefield, it deals 4 damage to you.
	*/
	haste(player, card, event);

	if( comes_into_play(player, card, event) ){
		damage_player(player, 4, player, card);
	}

	return 0;
}

int card_liege_of_hollows(int player, int card, event_t event){
	/* Liege of the Hollows	|2|G|G
	 * Creature - Spirit 3/4
	 * When ~ dies, each player may pay any amount of mana. Then each player who paid mana this way puts that many 1/1 |Sgreen Squirrel creature tokens onto the battlefield. */

	if( this_dies_trigger(player, card, event, 2) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SQUIRREL, &token);

		int i;
		for(i=0; i<2; i++){
			charge_mana(i, COLOR_COLORLESS, -1);
			if( x_value > 0 ){
				token.t_player = i;
				token.qty = x_value;
				generate_token(&token);
			}
		}
	}

	return 0;
}

int card_llanowar_sentinel(int player, int card, event_t event){

	if( has_mana_multi(player, 1, 0, 0, 1, 0, 0) && comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		charge_mana_multi(player, 1, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			this_test.id = get_id(player, card);
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
		}
	}

	return 0;
}

static int if_would_enter_bf_sac_two_untapped_lands_instead(int player, int card, event_t event)
{
  /* Land
   * If ~ would enter the battlefield, sacrifice two untapped lands instead. If you do, put ~ onto the battlefield. If you don't, put it into its owner's
   * graveyard. */

  if (event == EVENT_CAST_SPELL && affect_me(player, card) && IS_AI(player))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "");
	  test.state = STATE_TAPPED;
	  test.state_flag = DOESNT_MATCH;

	  if (!check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &test) < 2)
		ai_modifier -= 512;
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  play_land_sound_effect(player, card);

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "Select two untapped lands to sacrifice.");
	  test.state = STATE_TAPPED;
	  test.state_flag = DOESNT_MATCH;
	  test.qty = 2;
	  test.not_me = 1;

	  if (!new_sacrifice(player, card, player, SAC_NO_CANCEL|SAC_ALL_OR_NONE, &test))
		kill_card(player, card, KILL_STATE_BASED_ACTION);

	  return 1;	// so the sound effect doesn't play again in mana_producer() after prompting for cip effects
	}

  return 0;
}

int card_lotus_vale(int player, int card, event_t event)
{
  /* Lotus Vale	""
   * Land
   * If ~ would enter the battlefield, sacrifice two untapped lands instead. If you do, put ~ onto the battlefield. If you don't, put it into its owner's
   * graveyard.
   * |T: Add three mana of any one color to your mana pool. */

  if (if_would_enter_bf_sac_two_untapped_lands_instead(player, card, event))
	return 0;

  return card_gilded_lotus(player, card, event);
}

int card_maraxus_of_keld(int player, int card, event_t event)
{
  /* Maraxus of Keld	|4|R|R
   * Legendary Creature - Human Warrior 100/100
   * ~'s power and toughness are each equal to the number of untapped artifacts, creatures, and lands you control. */

  check_legend_rule(player, card, event);

  int c;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1)
	for (c = 0; c < active_cards_count[player]; c++)
	  if (in_play(player, c) && is_what(player, c, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND) && !is_tapped(player, c))
		++event_result;

  return 0;
}

int card_merfolk_traders(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}

	return 0;
}

int card_mind_stone(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL) ){
			int ai_choice = count_permanents_by_type(player, TYPE_LAND) > 6 ? 1 : 0;
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac and draw 1\n Cancel", ai_choice);
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		if( choice == 1 ){
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
				tap_card(player, card);
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION && instance->info_slot == 1 ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_mistmoon_griffin(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		exile_from_owners_graveyard(player, card);
		const int *grave = get_grave(player);
		int count = count_graveyard(player)-1;
		while (count > -1){
				if( cards_data[grave[count]].type & TYPE_CREATURE ){
					reanimate_permanent(player, card, player, count, REANIMATE_DEFAULT);
					break;
				}
				count--;
		}
	}

	return 0;
}

int card_natures_resurgence(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		if( player == AI && count_graveyard_by_type(player, TYPE_CREATURE) > 1 ){
			return 1;
		}
		else if( player != AI ){
				 return 1;
		}

	}

	else if( event == EVENT_RESOLVE_SPELL){
			 draw_cards(player, count_graveyard_by_type(player, TYPE_CREATURE));
			 draw_cards(1-player, count_graveyard_by_type(1-player, TYPE_CREATURE));
			 kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_necratog(int player, int card, event_t event){

   card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return 1;
	}
	if( event == EVENT_ACTIVATE){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				remove_top_creature_card_from_grave(player);
			}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, card, player, instance->parent_card, 2, 2);
	}

   return 0;
}

int card_null_rod(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		new_manipulate_all(player, card, 2, &this_test, ACT_DISABLE_ALL_ACTIVATED_ABILITIES);
	}

	if( new_specific_cip(player, card, event, 2, 2, &this_test) ){
		disable_all_activated_abilities(instance->targets[1].player, instance->targets[1].card, 1);
	}

	if( leaves_play(player, card, event) ){
		new_manipulate_all(player, card, 2, &this_test, ACT_ENABLE_ALL_ACTIVATED_ABILITIES);
	}

	return 0;
}

int card_ophidian(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && is_attacking(player, card) && is_unblocked(player, card) &&
		current_phase == PHASE_AFTER_BLOCKING && instance->info_slot != 66 ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE){
			instance->info_slot = 66;
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			negate_combat_damage_this_turn(player, instance->parent_card, player, instance->parent_card, 0);
			draw_cards(player, 1);
	}


	if( instance->info_slot == 66 && eot_trigger(player, card, event) ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_orcish_settlers(int player, int card, event_t event)
{
  /* Orcish Settlers	|1|R
   * Creature - Orc 1/1
   * |X|X|R, |T, Sacrifice ~: Destroy X target lands. */

  if (!IS_ACTIVATING(event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);
  td.allow_cancel = 3;

  if (event == EVENT_CAN_ACTIVATE)
	return (CAN_TAP(player, card) && can_sacrifice_this_as_cost(player, card)
			&& (IS_AI(player) ? CAN_ACTIVATE(player, card, MANACOST_XR(2,1)) && can_target(&td) : CAN_ACTIVATE(player, card, MANACOST_R(1))));

  if (event == EVENT_ACTIVATE)
	{
	  int max_targets;
	  if (IS_AI(player))
		{
		  int mana_avail = has_mana(player, COLOR_ANY, 1);
		  max_targets = (mana_avail - 1) / 2;
		  max_targets = MAX(max_targets, 0);
		  max_targets = MIN(max_targets, 10);
		}
	  else
		max_targets = 10;

	  pick_up_to_n_targets(&td, "TARGET_LAND", max_targets);
	  if (cancel != 1 && charge_mana_for_activated_ability(player, card, MANACOST_XR(2 * instance->number_of_targets, 1)))
		{
		  tap_card(player, card);
		  kill_card(player, card, KILL_SACRIFICE);
		}
	  else
		instance->number_of_targets = 0;	// Since it was set before attempting to charge mana
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  int i;
	  for (i = 0; i < instance->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
	}

  return 0;
}

int card_paradigm_shift(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}


	if( event == EVENT_RESOLVE_SPELL ){
		int amount = count_deck(player);
		int i;
		for( i=0; i<amount; i++){
			 rfg_top_card_of_deck(player);
		}
		amount = count_graveyard(player)-1;
		i=0;
		for( i=0; i<amount; i++){
			 from_graveyard_to_deck(player, 0, 1);
		}
		from_graveyard_to_deck(player, 0, 3);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_peacekeeper(int player, int card, event_t event)
{
  nobody_can_attack(player, card, event, ANYBODY);
  basic_upkeep(player, card, event, 1, 0, 0, 0, 0, 1);
  return 0;
}

void pendrell_effect(int player, int card, event_t event, kill_t kill_mode)
{
  upkeep_trigger_ability(player, card, event, ANYBODY);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  int c, p = current_turn;
	  char marked[151] = {0};
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (in_play(p, c) && is_what(p, c, TYPE_CREATURE))
		  marked[c] = 1;

	  card_instance_t* inst;
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (marked[c]	// was in play and a creature at start of resolution (iterating in reverse isn't sufficient)
			&& (inst = in_play(p, c))				// hasn't left play due to a trigger from a previously-unpaid upkeep
			&& !(inst->token_status & STATUS_DYING)	// hasn't been marked dying by a trigger from a previously-unpaid upkeep
			&& is_what(p, c, TYPE_CREATURE)			// hasn't stopped being a creature due to a trigger from a previously-unpaid upkeep
			&& (!has_mana(p, COLOR_COLORLESS, 1)
				|| do_dialog(p, p, c, player, card, " Pay upkeep\n Pass", 0) == 1
				|| !charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, p, COLOR_COLORLESS, 1)))
		  kill_card(p, c, kill_mode);
	}
}

int card_pendrell_mists(int player, int card, event_t event){
	pendrell_effect(player, card, event, KILL_SACRIFICE);

	return global_enchantment(player, card, event);
}

int card_phyrexian_furnace(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return 1;
		}
		int can_target_player[2] = {count_graveyard(0) > 0 && ! graveyard_has_shroud(0), count_graveyard(1) > 0 && ! graveyard_has_shroud(1)};
		if( can_target_player[0] + can_target_player[1] ){
			if( generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(1), 0, &td, NULL) ){
				return 1;
			}
		}
	}
	if( event == EVENT_ACTIVATE){
		instance->info_slot = instance->number_of_targets = 0;
		int abilities[2] = { generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL), 0};
		int can_target_player[2] = {count_graveyard(0) > 0 && ! graveyard_has_shroud(0), count_graveyard(1) > 0 && ! graveyard_has_shroud(1)};
		if( can_target_player[0] + can_target_player[1] ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_ME, MANACOST_X(1), 0, &td, NULL) ){
				abilities[1] = 1;
			}
		}
		int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
							"Exile bottom card", abilities[0], 6,
							"Exile target card & draw", abilities[1], 5);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(choice-1)) ){
			if( choice == 1 ){
				if( pick_target(&td, "TARGET_PLAYER") ){
					instance->info_slot = choice;
					tap_card(player, card);
				}
			}
			if( choice == 2 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");
				select_target_from_either_grave(player, card, 0, AI_MAX_CMC, AI_MAX_CMC, &this_test, 0, 1);
				if( spell_fizzled != 1 ){
					instance->info_slot = choice;
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION){
		if( instance->info_slot == 1 && valid_target(&td) ){
			rfg_card_from_grave(instance->targets[0].player, 0);
		}
		if( instance->info_slot == 2 ){
			int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
			if( selected != -1 ){
				rfg_card_from_grave(instance->targets[0].player, selected);
				draw_cards(player, 1);
			}
		}
	}

	return 0;
}

static int can_draw_cards_as_cost_thunk(int player, int card, int number_of_age_counters)
{
  return can_draw_cards_as_cost(player, number_of_age_counters);
}
static int draw_cards_thunk(int player, int card, int number_of_age_counters)
{
  draw_cards(player, number_of_age_counters);
  return 1;
}
int card_psychic_vortex(int player, int card, event_t event)
{
  /* Psychic Vortex	|2|U|U
   * Enchantment
   * Cumulative upkeep-Draw a card.
   * At the beginning of your end step, sacrifice a land and discard your hand. */

  cumulative_upkeep_general(player, card, event, can_draw_cards_as_cost_thunk, draw_cards_thunk);

  if (current_turn == player && eot_trigger(player, card, event))
	{
	  controller_sacrifices_a_permanent(player, card, TYPE_LAND, SAC_NO_CANCEL);
	  discard_all(player);
	}

  return global_enchantment(player, card, event);
}

int card_revered_unicorn(int player, int card, event_t event)
{
  /* Revered Unicorn	|1|W
   * Creature - Unicorn 2/3
   * Cumulative upkeep |1
   * When ~ leaves the battlefield, you gain life equal to the number of age counters on it. */

  cumulative_upkeep(player, card, event, MANACOST_X(1));

  if (leaves_play(player, card, event))
	gain_life(player, count_counters(player, card, COUNTER_AGE));

  return 0;
}

int card_rogue_elephant(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		if (!(can_sacrifice(player, player, 1, TYPE_LAND, SUBTYPE_FOREST)
			  && sacrifice(player, card, player, 0, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0))){
			if (player == AI){
				ai_modifier -= 48;
			}
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

  return 0;
}

int card_scorched_ruins(int player, int card, event_t event)
{
  /* Scorched Ruins	""
   * Land
   * If ~ would enter the battlefield, sacrifice two untapped lands instead. If you do, put ~ onto the battlefield. If you don't, put it into its owner's
   * graveyard.
   * |T: Add |4 to your mana pool. */

  if (if_would_enter_bf_sac_two_untapped_lands_instead(player, card, event))
	return 0;

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_TAP_FOR_MANA(player, card);

  if (event == EVENT_ACTIVATE)
	produce_mana_tapped(player, card, COLOR_COLORLESS, 4);

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_TAP_FOR_MANA(player, card))
	declare_mana_available(player, COLOR_COLORLESS, 4);

  return 0;
}

int card_serenity(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT | TYPE_ARTIFACT);

		new_manipulate_all(player, card, 2, &this_test, KILL_BURY);
	}

  return global_enchantment(player, card, event);
}

int card_serras_blessing(int player, int card, event_t event){

	if( ! is_humiliated(player, card) && event == EVENT_ABILITIES && affected_card_controller == player &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  ){
		vigilance(affected_card_controller, affected_card, event);
	}

	return global_enchantment(player, card, event);
}

int card_serrated_biskelion(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
			add_minus1_minus1_counters(player, instance->parent_card, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_shattered_crypt(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		return !check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		int max_cards = count_graveyard_by_type(player, TYPE_CREATURE);
		int generic_mana_available = has_mana(player, COLOR_ANY, 1);	// A useful trick from card_fireball().
		max_cards = MIN(max_cards, generic_mana_available);
		max_cards = MIN(max_cards, 19);

		if (max_cards <= 0 || graveyard_has_shroud(player)){
			ai_modifier -= 48;
			instance->info_slot = 0;
			return 0;
		}

		char buf[100];
		if (ai_is_speculating == 1){
			*buf = 0;
		} else if (max_cards == 1){
			strcpy(buf, "Select a creature card.");
		} else {
			sprintf(buf, "Select up to %d creature cards.", max_cards);
		}
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, buf);
		instance->info_slot = select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, &this_test, max_cards, &instance->targets[0]);

		if (instance->info_slot > 0){
			charge_mana(player, COLOR_COLORLESS, instance->info_slot);
		}
	} else if (event == EVENT_RESOLVE_SPELL){
		int i, num_validated = 0;
		for (i = 0; i < instance->info_slot; ++i){
			int selected = validate_target_from_grave(player, card, player, i);
			if (selected != -1){
				from_grave_to_hand(player, selected, TUTOR_HAND);
				++num_validated;
			}
		}
		if (num_validated > 0){
			lose_life(player, instance->info_slot);
		} else if (instance->info_slot > 0) {
			spell_fizzled = 1;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_southern_paladin(int player, int card, event_t event)
{
  /* Southern Paladin	|2|W|W
   * Creature - Human Knight 3/3
   * |W|W, |T: Destroy target |Sred permanent. */

  return northern_southern_paladin(player, card, event, COLOR_RED);
}

int card_spinning_darkness(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;
	td.allow_cancel = 0;

	if( event == EVENT_MODIFY_COST ){
		if( count_graveyard_by_color(player, COLOR_TEST_BLACK) > 2 ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	else if( event == EVENT_CAN_CAST && target_available(player, card, &td) ){
			return 1;
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int resolved = 1;
			if( ! played_for_free(player, card) && ! is_token(player, card)  ){
				int choice = 0;
				if( count_graveyard_by_color(player, COLOR_TEST_BLACK) > 2 ){
					if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
						choice = do_dialog(player, player, card, -1, -1, " Pitch Spinning Darkness\n Cast normally\n Cancel", 0);
					}
				}
				else{
					choice = 1;
				}
				if( choice == 1 ){
					if( instance->info_slot == 1 ){
						charge_mana_from_id(player, card, event, get_id(player, card));
						if( spell_fizzled == 1 ){
							resolved = 0;
						}
					}
				}
				else if(choice == 0){
						const int *grave = get_grave(player);
						int count = count_graveyard(player)-1;
						int removed = 0;
						while( count > -1 && removed < 3 ){
								if( cards_data[grave[count]].color & COLOR_TEST_BLACK){
									rfg_card_from_grave(player, count);
									removed++;
								}
								count--;
						}
				}
				else{
					resolved = 0;
					spell_fizzled = 1;
				}
			}
			if( resolved == 1 ){
				pick_target(&td, "TARGET_CREATURE");
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			 if( valid_target(&td) ){
				 damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
				 gain_life(player, 3);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_steel_golem(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL ){
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			infinite_casting_cost();
		}
	}

   return 0;
}

int card_strands_of_night(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 2, 0, 0, 0, 0) &&
		count_graveyard_by_type(player, TYPE_CREATURE) > 0 && can_pay_life(player, 2) &&
		can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_SWAMP, 0, 0, 0, 0, 0, -1, 0) && ! graveyard_has_shroud(2)
	  ){
		int result = 1;
		if( player == AI && life[player] < 8 ){
			result = 0;
		}
		return result;
	}
	else if( event == EVENT_ACTIVATE){
			charge_mana_for_activated_ability(player, card, 0, 2, 0, 0, 0, 0);
			if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_LAND, 0, SUBTYPE_SWAMP, 0, 0, 0, 0, 0, -1, 0) ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creture card.");
				if( new_select_target_from_grave(player, card, player, 0, AI_MAX_CMC, &this_test, 0) != -1 ){
					lose_life(player, 2);
				}
				else{
					spell_fizzled = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION){
			int selected = validate_target_from_grave(player, card, player, 0);
			if( selected != -1 ){
				reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			}
	}

	return global_enchantment(player, card, event);
}

int card_straw_golem(int player, int card, event_t event){

	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){

	   int trig = 0;

	   if( trigger_cause_controller == 1-player && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
		  trig = 1;
	   }

	   if( trig > 0 ){
		  if(event == EVENT_TRIGGER){
			 event_result |= RESOLVE_TRIGGER_MANDATORY;
		  }
		  else if(event == EVENT_RESOLVE_TRIGGER){
				  kill_card(player, card, KILL_SACRIFICE);
		  }
	   }
	}
	return 0;
}

int card_sylvan_hierophant(int player, int card, event_t event)
{
  if (this_dies_trigger(player, card, event, 2))
	{
	  exile_from_owners_graveyard(player, card);
	  if (count_graveyard_by_type(player, TYPE_CREATURE) > 0 && !graveyard_has_shroud(player))
		{
		  test_definition_t this_test;
		  new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card.");
		  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
		}
	}

  return 0;
}

int card_tariff(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {
					instance->targets[0].player = instance->targets[0].card = -1;
					int max_t = -1;
					int count = active_cards_count[p]-1;
					int doubles_count = 1;
					while( count > -1 ){
							if( in_play(p, count) && is_what(p, count, TYPE_CREATURE) ){
								if( get_cmc(p, count) > max_t ){
									instance->targets[0].player = p;
									instance->targets[0].card = count;
									doubles_count = 1;
									max_t = get_cmc(p, count);
								}
								else if( get_cmc(p, count) == max_t ){
										doubles_count++;
								}
							}
							count--;
					}
					if( doubles_count > 1 ){
						target_definition_t td;
						default_target_definition(player, card, &td, TYPE_CREATURE);
						td.allowed_controller = p;
						td.preferred_controller = p;
						td.who_chooses = p;
						td.illegal_abilities = 0;
						td.allow_cancel = 0;
						td.extra = max_t;
						td.special = TARGET_SPECIAL_CMC_GREATER_OR_EQUAL;
						char msg[100];
						scnprintf(msg, 100, "Select a creature you control with CMC %d.", max_t);
						instance->number_of_targets = 0;
						new_pick_target(&td, msg, 0, GS_LITERAL_PROMPT);
					}

					if( instance->targets[0].player != -1 && instance->targets[0].card != -1 ){
						int kill = 1;
						if( has_mana_to_cast_iid(p, event, get_card_instance(instance->targets[0].player, instance->targets[0].card)->internal_card_id) ){
							charge_mana_from_id(p, -1, event, get_id(instance->targets[0].player, instance->targets[0].card));
							if( spell_fizzled != 1 ){
								kill = 0;
							}
						}
						if( kill == 1 ){
							kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
						}
					}
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_teferis_veil(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn == player && event == EVENT_DECLARE_BLOCKERS ){
		int count = 0;
		instance->info_slot = 0;
		while( count < active_cards_count[player] ){
			   if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) && is_attacking(player, count) ){
				   instance->targets[instance->info_slot].card = count;
				   instance->info_slot++;
			   }
			   count++;
		}
	}
	if( instance->info_slot > 0 && end_of_combat_trigger(player, card, event, 2) ){
		int z;
		for(z=0; z < instance->info_slot; z++){
			if( in_play(player, instance->targets[z].card) ){
				phase_out(player, instance->targets[z].card);
			}
		}
		instance->info_slot = 0;
	}

	return global_enchantment(player, card, event);
}

int card_thran_tome(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int to_reveal = MIN(3, count_deck(player));
			if( ! to_reveal ){
				return 0;
			}

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card.");
			this_test.create_minideck = to_reveal;
			this_test.no_shuffle = 1;
			new_global_tutor(instance->targets[0].player, player, TUTOR_FROM_DECK, TUTOR_GRAVE, 1, AI_MAX_VALUE, &this_test);
			to_reveal--;
			if( to_reveal ){
				draw_cards(player, to_reveal);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_X(5), 0, &td, NULL);
}

int card_thunderbolt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && (can_target(&td) || can_target(&td1)) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			if( can_target(&td) ){
				if( can_target(&td1) ){
					choice = do_dialog(player, player, card, -1, -1, " 3 damage to a player\n 4 damage to a creature", 1);
				}
			}
			else if( can_target(&td1) ){
				choice = 1;
			}

			if( choice == 0 && pick_target(&td, "TARGET_PLAYER") ){
				instance->info_slot = 66;
			}
			else if( choice == 1 && pick_target(&td1, "TARGET_CREATURE") ){
				instance->info_slot = 67;
			}

	}

	else if( event == EVENT_RESOLVE_SPELL){

			if( instance->info_slot == 66 && valid_target(&td) ){
				damage_player(instance->targets[0].player, 3, player, card);
			}

			if( instance->info_slot == 67 && valid_target(&td1) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
			}

			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_thundermare(int player, int card, event_t event){

	haste(player, card, event);

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		new_manipulate_all(player, card, 2, &this_test, ACT_TAP);
	}

	return 0;
}

// tolarian drake --> sandbar crocodile

static int entrancer_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( end_of_combat_trigger(player, card, event, 2) ){
		gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_tolarian_entrancer(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) ){
		int count = active_cards_count[1-player]-1;
		while( count > -1 ){
				if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
					card_instance_t *instance = get_card_instance(1-player, count);
					if( instance->blocking == card ){
						create_targetted_legacy_effect(player, card, &entrancer_effect, 1-player, count);
					}
				}
				count--;
		}
	}

	return 0;
}

int card_tolarian_serpent(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		mill(player, 7);
	}
	return 0;
}

int card_touchstone(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target artifact you don't control.");
}

int card_tranquil_grove(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana_for_activated_ability(player, card, 1, 0, 0, 2, 0, 0) ){
			return 1;
	}
	else if( event == EVENT_ACTIVATE){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 2, 0, 0);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ENCHANTMENT);
			this_test.not_me = 1;
			new_manipulate_all(player, instance->parent_card, 2, &this_test, KILL_DESTROY);
	}

	return global_enchantment(player, card, event);
}

int card_uktabi_efreet(int player, int card, event_t event){
	cumulative_upkeep(player, card, event, 0, 0, 0, 1, 0, 0);
	return 0;
}

int card_urborg_justice(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		return would_valid_target(&td);

	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = get_dead_count(player, TYPE_CREATURE);
				impose_sacrifice(player, card, 1-player, amount, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_urborg_stalker(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = DOESNT_MATCH;
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		this_test.color_flag = DOESNT_MATCH;

		upkeep_trigger_ability_mode(player, card, event, ANYBODY, check_battlefield_for_special_card(player, card, current_turn, 0, &this_test) ?
								RESOLVE_TRIGGER_MANDATORY : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = DOESNT_MATCH;
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		this_test.color_flag = DOESNT_MATCH;
		if( check_battlefield_for_special_card(player, card, current_turn, 0, &this_test) ){
			damage_player(current_turn, 1, player, card);
		}
	}
	return 0;
}

int card_veteran_explorer(int player, int card, event_t event)
{
  int p;
  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	for (p = 0; p < 2; ++p)
	  if (do_dialog(p, player, card, -1, -1, " Search library\n Pass", 0) == 0)
		tutor_basic_lands(p, TUTOR_PLAY, 2);

  return 0;
}

int card_vitalize(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_vodalian_illusionist(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			phase_out(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 2, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_volounteer_reserves(int player, int card, event_t event){
	cumulative_upkeep(player, card, event, 1, 0, 0, 0, 0, 0);
	return 0;
}

int card_wave_of_terror(int player, int card, event_t event){

  /* Wave of Terror	|2|B
   * Enchantment
   * Cumulative upkeep |1
   * At the beginning of your draw step, destroy each creature with converted mana cost equal to the number of age counters on ~. They can't be regenerated. */

  cumulative_upkeep(player, card, event, 1, 0, 0, 0, 0, 0);

  if (trigger_condition == TRIGGER_DRAW_PHASE && affect_me(player, card)
	  && current_turn == player && reason_for_trigger_controller == player)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		manipulate_all(player, card, ANYBODY, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, count_counters(player, card, COUNTER_AGE),MATCH, KILL_BURY);
	}

  return global_enchantment(player, card, event);
}

static int effect_enable_flash_land(int player, int card, event_t event)
{
  /* Local data:
   * targets[2].player: allowable types
   * targets[2].card: illegal types
   * targets[3].player: prompt, a const char* cast to an int.
   * targets[3].card: whether prompt it literal. */

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  if (!IS_ACTIVATING(event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  base_target_definition(player, card, &td, instance->targets[2].player);
  td.illegal_type = instance->targets[2].card;
  td.allowed_controller = td.preferred_controller = player;
  td.zone = TARGET_ZONE_HAND;

  return can_play_cards_as_though_they_had_flash(player, card, event, &td, (const char*)(instance->targets[3].player), instance->targets[3].card);
}
int enable_flash_land(int player, int card, event_t event, int x, int b, int u, int g, int r, int w, int required_type, int illegal_type, const char* prompt, int literal)
{
  /* Land
   * |T: Add |1 to your mana pool.
   * [xbugrw], |T: You may cast [required_type], non[illegal_type] cards this turn as though they had flash. */

  if (IS_ACTIVATING(event))
	{
	  target_definition_t td;
	  base_target_definition(player, card, &td, required_type);
	  td.illegal_type = illegal_type;
	  td.allowed_controller = td.preferred_controller = player;
	  td.zone = TARGET_ZONE_HAND;

	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_FLASH
	  } choice;

	  if (paying_mana())
		choice = event == EVENT_CAN_ACTIVATE ? CAN_TAP_FOR_MANA(player, card) : CHOICE_MANA;
	  else
		choice = DIALOG(player, card, event,
						"Add 1", CAN_TAP_FOR_MANA(player, card), -1,
						"Flash", CAN_TAP(player, card) && CAN_ACTIVATE(player, card, MANACOST6(x+1,b,u,g,r,w)), IS_AI(player) && can_target(&td) ? 1 : -1);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_MANA:
			  mana_producer(player, card, event);
			  break;

			case CHOICE_FLASH:
			  tap_card(player, card);
			  if (!charge_mana_for_activated_ability(player, card, x,b,u,g,r,w))
				untap_card_no_event(player, card);
			  break;
		  }
	  else	// event == EVENT_RESOLVE_ACTIVATION
		if (choice == CHOICE_FLASH)
		  {
			int leg = create_legacy_activate(player, card, effect_enable_flash_land);
			if (leg == -1)
			  return 0;
			card_instance_t* legacy = get_card_instance(player, leg);
			legacy->targets[2].player = required_type;
			legacy->targets[2].card = illegal_type;
			legacy->targets[3].player = (int)prompt;
			legacy->targets[3].card = literal;

			// If there's cards on the stack below this, allow a response before further unwinding the stack.
			if (stack_size > 0)
			  allow_response_to_activation(player, card);
		  }

	  return 0;
	}
  else
	return mana_producer(player, card, event);
}

int card_winding_canyons(int player, int card, event_t event)
{
  /* Winding Canyons	""
   * Land
   * |T: Add |1 to your mana pool.
   * |2, |T: Until end of turn, you may play creature cards as though they had flash. */
  return enable_flash_land(player, card, event, MANACOST_X(2), TYPE_CREATURE, 0, "SELECT_CREATURE", 0);
}

int card_xanthic_statue(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_a_subtype(player, instance->parent_card, SUBTYPE_GOLEM);
		artifact_animation(player, instance->parent_card, player, instance->parent_card, 1, 8, 8, KEYWORD_TRAMPLE, 0);
	}

	return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 5, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_zombie_scavengers(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( regeneration(player, card, event, MANACOST0) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
			return 99;
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			remove_top_creature_card_from_grave(player);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		return regeneration(player, card, event, MANACOST0);
	}

	return 0;
}

