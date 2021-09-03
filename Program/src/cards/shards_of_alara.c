#include "manalink.h"

// Functions

static int is_basic_land_by_internal_id(int iid, int unused, int unused1, int unused2){
	return is_basic_land_by_id(cards_data[iid].id);
}

static int panorama(int player, int card, event_t event, int sub1, int sub2, int sub3){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) && can_use_activated_abilities(player, card) &&
			can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac and tutor land\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				 untap_card_no_event(player, card);
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select a basic %s, %s, or %s card.", sub1, sub2, sub3));
				this_test.subtype = get_hacked_subtype(player, card, sub1);
				this_test.sub2 = get_hacked_subtype(player, card, sub2);
				this_test.sub3 = get_hacked_subtype(player, card, sub3);
				this_test.subtype_flag = F2_MULTISUBTYPE;
				this_test.special_selection_function = &is_basic_land_by_internal_id;
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &this_test);
			}
			else{
				return mana_producer(player, card, event);
			}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int unearth(int player, event_t event, int colorless, int black, int blue, int green, int red, int white){
	if(event == EVENT_GRAVEYARD_ABILITY){
		if( (has_mana_multi( player, colorless, black, blue, green, red, white )) && ( can_sorcery_be_played(player, event)) ){
			return GA_UNEARTH;
		}
	}
	else if( event == EVENT_PAY_FLASHBACK_COSTS ){
			charge_mana_multi( player, colorless, black, blue, green, red, white );
			if( spell_fizzled != 1 ){
				return GAPAID_REMOVE;
			}
	}
	return 0;
}

int devouring(int player, int card){
	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	int devoured[151];
	int dc = 0;
	int to_devour = IS_AI(player) ? count_subtype(player, TYPE_CREATURE, -1) / 3 : -1;
	instance->targets[1].player = 0;
	instance->targets[1].card = 0;
	int count_goblins = get_id(player, card) == CARD_ID_VORACIOUS_DRAGON;
	while( can_target(&td) ){
			instance->number_of_targets = 0;
			int result = pick_creature_for_sacrifice(player, card, 0);
			if( result != -1 ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				instance->targets[1].card++;
				if( count_goblins && has_subtype(player, result, SUBTYPE_GOBLIN) ){
					instance->targets[1].player++;
				}
				devoured[dc] = instance->targets[0].card;
				dc++;
				if( dc == to_devour ){
					break;
				}
			}
			else{
				break;
			}
	}
	int count = dc-1;
	while( count > -1 ){
			kill_card(player, devoured[count], KILL_SACRIFICE);
			count--;
	}
	return dc;
}


void devour(int player, int card, event_t event, int number){
	/* 702.81. Devour
	 *
	 * 702.81a Devour is a static ability. "Devour N" means "As this object enters the battlefield, you may sacrifice any number of creatures. This permanent
	 * enters the battlefield with N +1/+1 counters on it for each creature sacrificed this way."
	 *
	 * 702.81b Some objects have abilities that refer to the number of creatures the permanent devoured. "It devoured" means "sacrificed as a result of its
	 * devour ability as it entered the battlefield." */

	if( event == EVENT_RESOLVE_SPELL && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t* instance = get_card_instance(player, card);
		instance->state |= STATE_OUBLIETTED;
		int result = devouring(player, card);
		instance->state &= ~STATE_OUBLIETTED;
		add_1_1_counters(player, card, number * result);
	}
}

// Cards

int card_ad_nauseam(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		int flag = can_pay_life(player, 0);
		int cd = count_deck(player);
		int *deck = deck_ptr[player];
		while( cd > 0 ){
			int cmc = get_cmc_by_id(cards_data[deck[0]].id);
			int card_added = add_card_to_hand(player, deck[0] );
			remove_card_from_deck( player, 0 );
			if( flag ){
				lose_life(player, cmc);
			}
			cd--;
			if( cd > 0 ){
				if( player == AI ){
					reveal_card(player, card, player, card_added);
					if( life[AI] < 6 && ! flag ){
						break;
					}
				}
				else{
					int choice = do_dialog(player, player, card, player, card_added, " Reveal another card\n Stop", 1);
					if( choice == 1 ){
						break;
					}
				}
			}
		}
		kill_card( player, card, KILL_DESTROY );
	}
	return 0;
}

int card_agony_warp(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->number_of_targets = 0;
			if( select_target(player, card, &td, "Select target creature for -3/-0", &(instance->targets[0]))
				&& select_target(player, card, &td, "Select target creature for -0/-3", &(instance->targets[1])) ){
				instance->number_of_targets = 2;
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 0) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -3, 0);
			}
			if( validate_target(player, card, &td, 1) ){
				pump_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, 0, -3);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_ajani_vengeant(int player, int card, event_t event)
{
  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  target_definition_t td_permanent;
	  default_target_definition(player, card, &td_permanent, TYPE_PERMANENT);

	  target_definition_t td_creature_or_player;
	  default_target_definition(player, card, &td_creature_or_player, TYPE_CREATURE);
	  td_creature_or_player.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	  target_definition_t td_player;
	  default_target_definition(player, card, &td_player, 0);
	  td_player.zone = TARGET_ZONE_PLAYERS;

	  int ai_priority_helix = 1;
	  if (player == AI || ai_is_speculating == 1)
		{
		  target_definition_t td_opponent;
		  default_target_definition(player, card, &td_opponent, 0);
		  td_opponent.allowed_controller = 1-player;
		  td_opponent.zone = TARGET_ZONE_PLAYERS;

		  target_definition_t td_weak_creature;
		  default_target_definition(player, card, &td_weak_creature, TYPE_CREATURE);
		  td_weak_creature.allowed_controller = 1-player;
		  td_weak_creature.toughness_requirement = 3 | TARGET_PT_LESSER_OR_EQUAL | TARGET_PT_INCLUDE_DAMAGE;

		  if (life[1-player] <= 6 && can_target(&td_opponent))
			ai_priority_helix = life[1-player] <= 3 ? 15 : 5;
		  else if (can_target(&td_weak_creature))
			ai_priority_helix = 5;
		}

	  enum {
		CHOICE_LOCKDOWN = 1,
		CHOICE_HELIX,
		CHOICE_LANDS
	  } choice = DIALOG(player, card, event,
						DLG_PLANESWALKER, DLG_RANDOM,
						"Lock down card", can_target(&td_permanent), 1, 1,
						"Lightning Helix", can_target(&td_creature_or_player), ai_priority_helix, -2,
						"Nuke lands", can_target(&td_player), 10, -7);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;	// else fall through to planeswalker()
		}
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  switch (choice)
			{
			  case CHOICE_LOCKDOWN:
				if (player == AI || ai_is_speculating == 1)
				  {
					target_definition_t td_tapped_permanent;
					default_target_definition(player, card, &td_tapped_permanent, TYPE_PERMANENT);
					td_tapped_permanent.required_state = TARGET_STATE_TAPPED;

					if (can_target(&td_tapped_permanent))
					  pick_target(&td_tapped_permanent, "TARGET_PERMANENT");
					else
					  pick_target(&td_permanent, "TARGET_PERMANENT");
				  }
				else
				  pick_target(&td_permanent, "TARGET_PERMANENT");
				break;

			  case CHOICE_HELIX:
				pick_target(&td_creature_or_player, "TARGET_CREATURE_OR_PLAYER");
				break;

			  case CHOICE_LANDS:
				pick_target(&td_player, "TARGET_PLAYER");
				break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_LOCKDOWN:
			  if (valid_target(&td_permanent))
				does_not_untap_effect(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 1);
			  break;

			case CHOICE_HELIX:
			  if (valid_target(&td_creature_or_player))
				{
				  damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
				  gain_life(player, 3);
				}
			  break;

			case CHOICE_LANDS:
			  if (valid_target(&td_player))
				{
				  test_definition_t this_test;
				  default_test_definition(&this_test, TYPE_LAND);
				  new_manipulate_all(player, instance->parent_card, instance->targets[0].player, &this_test, KILL_DESTROY);
				}
			  break;
		  }
	}

  return planeswalker(player, card, event, 3);
}

int card_akrasan_squire( int player, int card, event_t event){
	exalted(player, card, event, 0, 0);
	return 0;
}

int card_angelic_benediction( int player, int card, event_t event){
	int attacker = exalted(player, card, event, 0, 0);
	if (attacker != -1){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		card_instance_t* instance = get_card_instance(player, card);

		if (select_target(player, card, &td, "Select target creature to tap.", NULL)){
			tap_card( instance->targets[0].player, instance->targets[0].card );
		}

		instance->number_of_targets = 0;
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	return global_enchantment(player, card, event);;
}

int card_angelsong(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		fog_effect(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return cycling(player, card, event, 2, 0, 0, 0, 0, 0);
}

int card_bant_battlemage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) || has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
				if( current_phase < PHASE_DECLARE_BLOCKERS ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Give Trample\n Give Flying\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		int blue = 0;
		int green = 1;
		if( choice == 1 ){
			blue = 1;
			green = 0;
		}
		charge_mana_for_activated_ability(player, card, 0, 0, blue, green, 0, 0);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			instance->info_slot = 66+choice;
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_TRAMPLE, 0);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
		}
	}

	return 0;
}

int card_bant_charm(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT );

	if( event == EVENT_CAN_CAST ){
		int result = card_flash_counter(player, card, event);
		if( result != 0 ){
			return result;
		}
		if( can_target(&td) || can_target(&td1) ){
			return 1;
		}
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		// Choose which mode.
		if( card_on_stack_controller != -1 ){
			instance->info_slot = 2;
			return card_flash_counter(player, card, event);
		}

		int choice = 0;
		if( can_target(&td) || can_target(&td1) ){
			choice = do_dialog(player, player, card, -1, -1, " Destroy Artifact\n Bottom Creature", 1);
		}
		else if( can_target(&td) ){
			choice = 1;
		}

		if( choice == 0 ){
			pick_target(&td1, "TARGET_ARTIFACT");
		}
		else{
			pick_target(&td, "TARGET_CREATURE");
		}
		instance->info_slot = choice;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == 2 ){
			return card_flash_counter(player, card, event);
		}
		else{
			if( instance->info_slot == 1 ){
				if( valid_target(&td) ){
					put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
					put_top_card_of_deck_to_bottom(instance->targets[0].player);
				}
			}
			else{
				if( valid_target(&td1) ){
					set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_bant_panorama(int player, int card, event_t event){
	return panorama(player, card, event, SUBTYPE_FOREST, SUBTYPE_ISLAND, SUBTYPE_PLAINS);
}

int card_battlegrace_angel( int player, int card, event_t event){

	exalted(player, card, event, 0, SP_KEYWORD_LIFELINK);

	return 0;
}

int card_blightning(int player, int card, event_t event)
{
  /* Blightning	|1|B|R
   * Sorcery
   * ~ deals 3 damage to target player. That player discards two cards. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  damage_target0(player, card, 3);
		  new_multidiscard(get_card_instance(player, card)->targets[0].player, 2, 0, player);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_blister_beetle( int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE );
		td1.allow_cancel = 0;
		if (can_target(&td1) && pick_target(&td1, "TARGET_CREATURE")){
			card_instance_t* instance = get_card_instance( player, card);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_blood_cultist(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( sengir_vampire_trigger(player, card, event, 2) ){
		add_1_1_counters(player, card, instance->targets[11].card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_bloodthorn_taunter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 5 | TARGET_PT_GREATER_OR_EQUAL;
	td.required_state = TARGET_STATE_SUMMONING_SICK;

	card_instance_t *instance = get_card_instance( player, card);

	haste(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_bone_splinters(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				pick_target(&td, "TARGET_CREATURE");
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 0) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_branching_bolt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST  ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				if( ! check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_FLYING) ){
					td.required_abilities = KEYWORD_FLYING;
				}
				else{
					td.illegal_abilities |= KEYWORD_FLYING;
				}
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				instance->info_slot = 1;
				if( can_target(&td) ){
					if( select_target(player, card, &td, "Select target creature.", &(instance->targets[1])) ){
						instance->info_slot++;
					}
				}
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
			}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<instance->info_slot; i++){
				if( validate_target(player, card, &td, i) ){
					damage_creature(instance->targets[i].player, instance->targets[i].card, 3, player, card);
				}
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_brilliant_ultimatum(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST  ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL  ){
		int number = count_deck(player);
		number = MIN(5, number);
		if( number > 0 ){
			int piles[2][number];

			separate_into_two_piles(1-player, deck_ptr[player], number, piles);

			int chosen = choose_between_two_piles(player, number, piles, AI_MAX_VALUE, "Play these");

			rfg_top_n_cards_of_deck(player, number);

			while (1){
				int playable[number];
				int i, num_playable = 0;
				for (i = 0; i < number; ++i){
					if (piles[chosen][i] != iid_draw_a_card && piles[chosen][i] != -1){
						/* Since we're playing these immediately as they're chosen instead of putting them on the stack, some of them
						 * might have been removed from exile already */
						if (!check_rfg(player, cards_data[piles[chosen][i]].id)){
							piles[chosen][i] = -1;
						} else if (can_legally_play_iid(player, piles[chosen][i])){
							playable[num_playable] = piles[chosen][i];
							++num_playable;
						}
					}
				}
				if (num_playable == 0){
					break;
				}

				int selected;
				if (player == AI){
					selected = 0;
				} else {
					selected = show_deck(player, playable, num_playable, "Play a card", 0, 0x7375B0);
				}
				if (selected == -1){
					break;
				}

				play_card_in_exile_for_free(player, player, cards_data[playable[selected]].id);

				for (i = 0; i < number; ++i){
					if (piles[chosen][i] == playable[selected]){
						piles[chosen][i] = -1;
						break;
					}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_broodmate_dragon(int player, int card, event_t event){
	/* Broodmate Dragon	|3|B|R|G
	 * Creature - Dragon 4/4
	 * Flying
	 * When ~ enters the battlefield, put a 4/4 |Sred Dragon creature token with flying onto the battlefield. */

	// original code : 004DEB1D

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_DRAGON, &token);
		token.pow = token.tou = 4;
		generate_token(&token);
	}

	return 0;
}

int card_bull_cerodon(int player, int card, event_t event){
	/*
	  Bull Cerodon |4|W|R
	  Creature - Beast 5/5
	  Vigilance, haste
	*/
	vigilance(player, card, event);
	haste(player, card, event);
	return 0;
}

int card_caldera_hellion(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	if( comes_into_play(player, card, event) ){
		int devoured = 0;
		state_untargettable(player, card, 1);
		if( can_target(&td) ){
			if( player != AI ){
				state_untargettable(player, card, 1);
				while( can_target(&td) ){
						if( controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0) ){
							devoured++;
						}
						else{
							break;
						}
				}
			}
			else{
				int count = active_cards_count[player]-1;
				int par = 1000;
				int sac = -1;
				while( count > -1 ){
						if( count != card &&in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
							if( is_nice_creature_to_sacrifice(player, count) ){
								sac = count;
								break;
							}
							else if( get_base_value(player, card) < par ){
									sac = count;
									par = get_base_value(player, card);
							}
						}
						count--;
				}
				if( sac != -1 ){
					kill_card(player, sac, KILL_SACRIFICE);
					devoured++;
				}
				count = active_cards_count[player]-1;
				while( count > -1 ){
						if( count != card && in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
							if( get_toughness(player, count) < 4 &&
								! is_protected_from_me(player, card, player, count)
							  ){
								kill_card(player, count, KILL_SACRIFICE);
								devoured++;
							}
						}
						count--;
				}
			}
			add_1_1_counters(player, card, devoured);
		}
		state_untargettable(player, card, 0);
		damage_all(player, card, ANYBODY, 3, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_call_to_heel(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST  ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				draw_cards(instance->targets[0].player, 1);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_clarion_ultimatum(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST  ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			int selected = 0;
			while( selected < 5 && can_target(&td) ){
					if( select_target(player, card, &td, "Select a permanent you control", &(instance->targets[selected])) ){
						state_untargettable(instance->targets[selected].player, instance->targets[selected].card, 1);
						selected++;
					}
			}
			int i;
			for(i=0; i<selected; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
				int id = get_id(instance->targets[i].player, instance->targets[i].card);
				card_ptr_t* c_me = cards_ptr[ id ];
				char buffer[100];
				scnprintf(buffer, 100, "Select a card called %s", c_me->name);
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_PERMANENT, buffer);
				this_test.id = id;
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &this_test);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_corpse_conoisseur(int player, int card, event_t event){

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		char buffer[100] = "Select a creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GRAVE, 0, AI_MAX_CMC, &this_test);
	}
	return unearth(player, event, 3, 1, 0, 0, 0, 0);
}

int card_couriers_capsule( int player, int card, event_t event){

	if(event == EVENT_RESOLVE_ACTIVATION){
		draw_cards(player, 2);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_ME, 1, 0, 1, 0, 0, 0, 0, 0, 0);
}

// court archers --> akrasan squire

int card_cradle_of_vitality(int player, int card, event_t event){

	return global_enchantment(player, card, event);
}

int card_crucible_of_fire(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_DRAGON, 3, 3, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	return global_enchantment(player, card, event);
}

int card_cruel_ultimatum(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		return would_valid_target(&td);
	}

	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->targets[0].player = 1-player;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				new_multidiscard(instance->targets[0].player, 3, 0, player);
				lose_life(instance->targets[0].player, 5);

				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
					new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
				}
				draw_cards(player, 3);
				gain_life(player, 5);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cunning_lethemancer(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		APNAP(p, {discard(p, 0, player);};);
	}

	return 0;
}

int card_death_baron(int player, int card, event_t event)
{
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_ABILITIES)
	  && affected_card_controller == player
	  && (has_subtype(affected_card_controller, affected_card, SUBTYPE_SKELETON)
		  || (affected_card != card && has_subtype(affected_card_controller, affected_card, SUBTYPE_ZOMBIE)))
	  && !is_humiliated(player, card))
	{
	  if (event == EVENT_ABILITIES)
		deathtouch(affected_card_controller, affected_card, event);
	  else
		++event_result;
	}

	return 0;
}

int card_deathgreeter(int player, int card, event_t event){//UNUSEDCARD
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		gain_life(player, instance->targets[11].card);
		instance->targets[11].card = 0;
	}
	return 0;
}

// deft duelist --> vanilla

int card_dispeller_capsule( int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_SACRIFICE_ME, 2, 0, 0, 0, 0, 1, 0,
									&td, "DISENCHANT");
}

int card_dragon_fodder(int player, int card, event_t event){
	/* Dragon Fodder	|1|R
	 * Sorcery
	 * Put two 1/1 |Sred Goblin creature tokens onto the battlefield. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 2);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dregscape_zombie(int player, int card, event_t event){
	return unearth(player, event, 0, 1, 0, 0, 0, 0);
}

int card_drumhunter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 5 | TARGET_PT_GREATER_OR_EQUAL;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	if( current_turn == player && eot_trigger(player, card, event) && can_target(&td) ){
		draw_some_cards_if_you_want(player, card, player, 1);
	}
	return mana_producing_creature(player, card, event, 0, COLOR_COLORLESS, 1);
}

int card_elspeths_emblem(int player, int card, event_t event)
{
  /* Elspeth's Emblem	""
   * Emblem
   * Artifacts, creatures, enchantments, and lands you control are indestructible. */

  if (event == EVENT_ABILITIES && affected_card_controller == player
	  /* Can't pass all four types to is_what() at once, or it'll interpret it as "all permanents" including planeswalkers.  And can't just check
	   * !is_planeswalker(), because sometimes they're also creatures (e.g. Gideon Jura) or artifacts (e.g. Memnarch) or enchantments (Enchanted Evening). */
	  && (is_what(affected_card_controller, affected_card, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND)
		  || is_what(affected_card_controller, affected_card, TYPE_ENCHANTMENT)))
	indestructible(affected_card_controller, affected_card, event);

  return 0;
}

int card_elspeth_knight_errant(int player, int card, event_t event){

	/* Elspeth, Knight-Errant	|2|W|W
	 * Planeswalker - Elspeth (4)
	 * +1: Put a 1/1 |Swhite Soldier creature token onto the battlefield.
	 * +1: Target creature gets +3/+3 and gains flying until end of turn.
	 * -8: You get an emblem with "Artifacts, creatures, enchantments, and lands you control are indestructible." */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		int priority_emblem = 0;
		if( event == EVENT_ACTIVATE && ! check_battlefield_for_id(player, CARD_ID_ELSPETHS_EMBLEM) ){
			priority_emblem = (count_counters(player, card, COUNTER_LOYALTY)*3)-21;
			priority_emblem += count_subtype(player, TYPE_PERMANENT, -1)*2;
		}

		enum{
			CHOICE_SOLDIER = 1,
			CHOICE_PUMP,
			CHOICE_EMBLEM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Generate a Soldier", 1, 10, 1,
						"Pump a creature", can_target(&td), 12, 1,
						"Emblem", 1, check_battlefield_for_id(player, CARD_ID_ELSPETHS_EMBLEM) ? -1 : 20, -8);

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
				case CHOICE_PUMP:
					pick_target(&td, "TARGET_CREATURE");
					break;
				default:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_SOLDIER:
				generate_token_by_id(player, card, CARD_ID_SOLDIER);
				break;

			case CHOICE_PUMP:
				if( valid_target(&td) ){
					pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
											3, 3, KEYWORD_FLYING, 0);
				}
				break;

			case CHOICE_EMBLEM:
				generate_token_by_id(player, card, CARD_ID_ELSPETHS_EMBLEM);
				break;
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_empyrial_archangel(int player, int card, event_t event ){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 ){
			if( source->damage_target_player == player && source->damage_target_card == -1 ){
				source->damage_target_card = card;
			}
		}
	}

	return 0;
}

int card_esper_battlemage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.special = TARGET_SPECIAL_DAMAGE_PLAYER;
	td1.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) && can_target(&td1) ){
			return 0x63;
		}
		if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && can_target(&td) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) && can_target(&td1) ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1);
			if( spell_fizzled != 1 && pick_target(&td1, "TARGET_DAMAGE") ){
				instance->number_of_targets = 1;
				card_instance_t *trg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				if( trg->damage_target_player != player || trg->damage_target_card != -1 ){
					spell_fizzled = 1;
				}
				else{
					instance->info_slot = 66;
					tap_card(player, card);
				}
			}
		}
		else{
			charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->info_slot = 67;
				instance->number_of_targets = 1;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			card_instance_t *trg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( trg->info_slot <= 2 ){
				trg->info_slot = 0;
			}
			else{
				trg->info_slot-=2;
			}
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return 0;
}

int card_esper_charm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			char buffer[500];
			int pos = 0;
			int ai_choice = -1;

			buffer[pos++] = ' ';
			if (can_target(&td)){
				ai_choice = 0;
			} else {
				buffer[pos++] = '_';
			}
			pos += scnprintf(buffer + pos, 500-pos, "Destroy an enchantment\n", buffer);

			pos += scnprintf(buffer + pos, 500-pos, " Draw two cards\n", buffer);
			if (ai_choice < 0){
				ai_choice = 1;
			}

			buffer[pos++] = ' ';
			if (can_target(&td1)){
				if( player == AI && hand_count[1-player] < 3 ){
					instance->targets[0].player = 1-player;
					instance->targets[0].card = -1;
					instance->number_of_targets = 1;
					if( would_valid_target(&td1) ){
						ai_choice = 2;
					}
				}
			} else {
				buffer[pos++] = '_';
			}
			pos += scnprintf(buffer + pos, 500-pos, "Target discards two cards\n", buffer);

			pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);

			instance->info_slot = 66 + (choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice));

			if( choice == 0 ){
				if(!pick_target(&td, "TARGET_ENCHANTMENT") || is_planeswalker(instance->targets[0].player, instance->targets[0].card) ){
					spell_fizzled = 1;
				}
			}
			// Don't need to do anything for choice == 1
			else if( choice == 2 ){
				if (!pick_target(&td1, "TARGET_PLAYER")){
					spell_fizzled = 1;
				}
			}
			else if( choice == 3 ){
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			if( instance->info_slot == 67 ){
				draw_cards(player,2);
			}
			if( instance->info_slot == 68 && valid_target(&td1) ){
				new_multidiscard(instance->targets[0].player, 2, 0, player);
			}
			kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_esper_panorama(int player, int card, event_t event){
	return panorama(player, card, event, SUBTYPE_SWAMP, SUBTYPE_ISLAND, SUBTYPE_PLAINS);
}

int card_etherium_sculptor(int player, int card, event_t event){

	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) ){
			COST_COLORLESS--;
		}
	}

	return 0;
}

int card_ethersworn_canonist(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && ! is_what(affected_card_controller, affected_card, TYPE_LAND | TYPE_ARTIFACT) ){
		if( instance->targets[1].player < 0 ){
			instance->targets[1].player = 0;
		}
		instance->targets[1].player |= (1+affected_card_controller);
	}

	if( event == EVENT_MODIFY_COST_GLOBAL && ! is_humiliated(player, card) ){
		if( ! is_what(affected_card_controller, affected_card, TYPE_LAND | TYPE_ARTIFACT) && instance->targets[1].player > -1 ){
			if( instance->targets[1].player & (1+affected_card_controller) ){
				infinite_casting_cost();
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_executioners_capsule( int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.illegal_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_SACRIFICE_ME, 1, 1, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_exuberant_firestoker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 5 | TARGET_PT_GREATER_OR_EQUAL;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( current_turn == player && eot_trigger(player, card, event) ){
		if( can_target(&td) && can_target(&td1) && pick_target(&td1, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
			damage_player(instance->targets[0].player, 2, player, card);
		}
	}
	return mana_producing_creature(player, card, event, 0, COLOR_COLORLESS, 1);
}

int card_fatestitcher(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td);
		}
	}
	else if(event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				state_untargettable(player, card, 1);
				if( pick_target(&td, "TARGET_PERMANENT") ){
					instance->number_of_targets = 1;
					tap_card(player, card);
				}
				state_untargettable(player, card, 0);
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				int choice = 0;
				if( is_tapped(instance->targets[0].player, instance->targets[0].card) ){
					choice = 1;
				}
				if( player != AI ){
					choice = do_dialog(player, player, card, -1,-1," Tap\n Untap", 0);
				}
				if( choice == 0){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
				else{
					untap_card(instance->targets[0].player, instance->targets[0].card);
				}
			}
	}
	return unearth(player, event, 0, 0, 1, 0, 0, 0);
}

int card_filigree_sages(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 2, 0, 1, 0, 0, 0, 0, &td, "TARGET_ARTIFACT");
}

int card_fire_field_ogre(int player, int card, event_t event){
	return unearth(player, event, 0, 1, 1, 0, 1, 0);
}

int card_flameblast_dragon(int player, int card, event_t event)
{
  // Whenever ~ attacks, you may pay |X|R. If you do, ~ deals X damage to target creature or player.
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && has_mana(player, COLOR_RED, 1))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	  // AI only activates if it can target opponent or opponent's creature, and X can be at least 1.
	  int trig_mode;
	  if (player == HUMAN && ai_is_speculating != 1)
		trig_mode = RESOLVE_TRIGGER_OPTIONAL;
	  else
		{
		  td.allowed_controller = 1-player;
		  if (has_mana_multi(player, MANACOST_XR(1,1)) && can_target(&td))
			trig_mode = RESOLVE_TRIGGER_MANDATORY;
		  td.allowed_controller = ANYBODY;
		}

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td)
		  && declare_attackers_trigger(player, card, event, trig_mode, player, card)
		  && charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_XR(-1,1))
		  && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
		damage_creature(instance->targets[0].player, instance->targets[0].card, x_value, player, card);
	}

	return 0;
}


int card_fleshbag_marauder(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

static int gather_specimens_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( specific_spell_played(player, card, event, 1-player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		if( ! is_token(instance->targets[1].player, instance->targets[1].card) ){
			int iid = get_original_internal_card_id(instance->targets[1].player, instance->targets[1].card);
			obliterate_card(instance->targets[1].player, instance->targets[1].card);
			int card_added = add_card_to_hand(player, iid);
			if( 1-player == AI ){
				add_state(player, card_added, STATE_OWNED_BY_OPPONENT);
			}
			else{
				remove_state(player, card_added, STATE_OWNED_BY_OPPONENT);
			}
			put_into_play(player, card_added);
		}
	}

	if( specific_cip(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		gain_control(player, card, instance->targets[1].player, instance->targets[1].card);
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}


int card_gather_specimens(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( counterspell(player, card, event, NULL, 0) ){
			return 99;
		}
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( card_on_stack_controller > -1 && card_on_stack > -1 ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( instance->number_of_targets == 1 && is_what(instance->targets[0].player, instance->targets[0].card, TYPE_CREATURE) ){
			int iid = get_original_internal_card_id(instance->targets[0].player, instance->targets[0].card);
			obliterate_card(instance->targets[0].player, instance->targets[0].card);
			int card_added = add_card_to_hand(player, iid);
			if( 1-player == AI ){
				add_state(player, card_added, STATE_OWNED_BY_OPPONENT);
			}
			else{
				remove_state(player, card_added, STATE_OWNED_BY_OPPONENT);
			}
			real_put_into_play(player, card_added);
		}
		create_legacy_effect(player, card, &gather_specimens_legacy);
		kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_gift_of_the_gargantuan(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			int amount = 4;
			if( amount > count_deck(player) ){
				amount = count_deck(player);
			}
			if( amount > 0 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_LAND, "Select a creature or land.");
				this_test.create_minideck = amount;
				this_test.no_shuffle = 1;
				if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test) != -1 ){
					amount--;
				}
				if( amount > 0 ){
					put_top_x_on_bottom(player, player, amount);
				}
			}
			kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_glaze_fiend(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_ARTIFACT);
	this_test.not_me = 1;

	if( new_specific_cip(player, card, event, player, 2, &this_test) ){
		pump_until_eot(player, card, player, card, 2, 2);
	}

	return 0;
}

int card_goblin_assault(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GOBLIN, &token);
		token.special_infos = 67;
		generate_token(&token);
	}

	all_must_attack_if_able(player, event, SUBTYPE_GOBLIN);

	return global_enchantment(player, card, event);
}

// goblin deathriders --> vanilla

int card_godsire(int player, int card, event_t event){
	/* Godsire	|4|R|G|G|W
	 * Creature - Beast 8/8
	 * Vigilance
	 * |T: Put an 8/8 Beast creature token that's |Sred, |Sgreen, and |Swhite onto the battlefield. */

	// original code : 004E33F7

	vigilance(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BEAST, &token);
		token.pow = 8;
		token.tou = 8;
		token.color_forced = COLOR_TEST_WHITE | COLOR_TEST_GREEN | COLOR_TEST_RED;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_grixis_battlemage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) || (has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && can_target(&td)) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && can_target(&td) ){
				choice = do_dialog(player, player, card, -1, -1, " Draw & discard\n Target creature can't block\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		int blue = 1;
		int red = 0;
		if( choice == 1 ){
			blue = 0;
			red = 1;
		}
		charge_mana_for_activated_ability(player, card, 0, 0, blue, 0, red, 0);
		if( spell_fizzled != 1 ){
			if( choice == 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				instance->info_slot = 66+choice;
				tap_card(player, card);
			}
			else{
				instance->info_slot = 66+choice;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			draw_cards(player, 1);
			discard(player, 0, player);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}

	return 0;
}

int card_grixis_charm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	if( player == AI ){
		td.toughness_requirement = 4 | TARGET_PT_LESSER_OR_EQUAL;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			int mode = (1<<2)+(1<<3);
			if( can_target(&td) ){
				mode |=(1<<0);
			}
			if( can_target(&td1) ){
				mode |=(1<<1);
			}
			char buffer[500];
			int pos = 0;
			int ai_choice = 0;
			if( mode & (1<<0) ){
				pos += scnprintf(buffer + pos, 500-pos, " Bounce a permanent\n", buffer);
			}
			if( mode & (1<<1) ){
				pos += scnprintf(buffer + pos, 500-pos, " -4/-4 to a creature\n", buffer);
				ai_choice = 1;
			}
			if( mode & (1<<2) ){
				pos += scnprintf(buffer + pos, 500-pos, " +2/+0 to your guys\n", buffer);
				if( current_phase == PHASE_AFTER_BLOCKING && count_attackers(player) > 1 && count_subtype(1-player, TYPE_CREATURE, -1) < 1 ){
					ai_choice = 2;
				}
			}
			pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
			choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			while( !( (1<<choice) & mode) ){
					choice++;
			}
			if( choice == 0 ){
				if( pick_target(&td, "TARGET_PERMANENT") ){
					instance->info_slot = 66+choice;
				}
			}
			else if( choice == 1 ){
					if( pick_target(&td1, "TARGET_CREATURE") ){
						instance->info_slot = 66+choice;
					}
			}
			else if( choice == 2 ){
					instance->info_slot = 66+choice;
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
			if( instance->info_slot == 67 && valid_target(&td1) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -4, -4);
			}
			if( instance->info_slot == 68){
				pump_subtype_until_eot(player, card, player, -1, 2, 0, 0, 0);
			}
			kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_grixis_panorama(int player, int card, event_t event){
	return panorama(player, card, event, SUBTYPE_SWAMP, SUBTYPE_ISLAND, SUBTYPE_MOUNTAIN);
}

int card_hell_thunder(int player, int card, event_t event){
	card_ball_lightning(player, card, event);
	return unearth(player, event, 4, 0, 0, 0, 1, 0);
}

int card_hellkite_overlord(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	haste(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 1, 0, 1, 0, 0) && can_regenerate(player, card) ){
			return 0x63;
		}
		else if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) ){
				return 1;
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( has_mana_for_activated_ability(player, card, 0, 1, 0, 1, 0, 0) && can_regenerate(player, card) ){
			charge_mana_for_activated_ability(player, card, 0, 1, 0, 1, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66;
			}
		}
		else{
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 67;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66){
			regenerate_target(player, instance->parent_card);
		}
		if( instance->info_slot == 67){
			pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, 0);
		}
	}
	return 0;
}

int card_hindering_light(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && card_on_stack_controller != -1 && card_on_stack != -1){
		if( ! check_state(card_on_stack_controller, card_on_stack, STATE_CANNOT_TARGET) ){
			card_instance_t *spell = get_card_instance(card_on_stack_controller, card_on_stack);
			int i;
			for(i=0; i<spell->number_of_targets; i++){
				if( spell->targets[i].player == player ){
					return 0x63;
				}
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_hissing_iguanar(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;
		if( resolve_gfp_ability(player, card, event, can_target(&td1) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( pick_target(&td1, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				damage_player(instance->targets[0].player, instance->targets[11].card, player, card);
			}
			instance->targets[11].card = 0;
		}
	}
	return 0;
}

int card_immortal_coil(int player, int card, event_t event){

	/* Immortal Coil	|2|B|B
	 * Artifact
	 * |T, Exile two cards from your graveyard: Draw a card.
	 * If damage would be dealt to you, prevent that damage. Exile a card from your graveyard for each 1 damage prevented this way.
	 * When there are no cards in your graveyard, you lose the game. */

	const int *grave = get_grave(player);

	if( event == EVENT_STATIC_EFFECTS && in_play(player, card) && grave[0] == -1 ){
		lose_the_game(player);
	}

	if (event == EVENT_SHOULD_AI_PLAY){
		ai_modifier += (player == AI ? 12 : -12) * count_graveyard(player);
	}

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 ){
			if( source->damage_target_player == player && source->damage_target_card == -1 ){
				int count = count_graveyard(player);
				while( count > 0 && source->info_slot > 0 ){
						char msg[100] = "Select a card to exile.";
						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_ANY, msg);
						int result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test);
						if( result != -1 ){
							count--;
							source->info_slot--;
						}
				}
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( grave[1] != -1 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			char msg[100] = "Select a card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			int result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test);
			if( result != -1 ){
				new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test);
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_invincible_hymn(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = count_deck(player);
		set_life_total(player, amount);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_jund_battlemage(int player, int card, event_t event){
	/* Jund Battlemage	|2|R
	 * Creature - Human Shaman 2/2
	 * |B, |T: Target player loses 1 life.
	 * |G, |T: Put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) || (has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) && can_target(&td)) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Target player loses 1 life\n Generate a saproling\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		int black = 1;
		int green = 0;
		if( choice == 1 ){
			black = 0;
			green = 1;
		}
		charge_mana_for_activated_ability(player, card, 0, black, 0, green, 0, 0);
		if( spell_fizzled != 1 ){
			if( choice == 0 && pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				instance->info_slot = 66+choice;
				tap_card(player, card);
			}
			else{
				instance->info_slot = 66+choice;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			lose_life(instance->targets[0].player, 1);
		}
		if( instance->info_slot == 67 ){
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
		}
	}

	return 0;
}

int card_jund_charm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			int mode = (1<<1)+(1<<3);
			if( can_target(&td) ){
				mode |=(1<<0);
			}
			if( can_target(&td1) ){
				mode |=(1<<2);
			}
			char buffer[500];
			int pos = 0;
			int ai_choice = 0;
			if( mode & (1<<0) ){
				pos += scnprintf(buffer + pos, 500-pos, " Exile target player's grave\n", buffer);
			}
			if( mode & (1<<1) ){
				pos += scnprintf(buffer + pos, 500-pos, " 2 damage to all creatures\n", buffer);
				if( count_creatures_by_toughness(player, 3, 3) < count_creatures_by_toughness(1-player, 3, 3) ){
					ai_choice = 1;
				}
			}
			if( mode & (1<<2) ){
				pos += scnprintf(buffer + pos, 500-pos, " 2 +1/+1 counters\n", buffer);
				ai_choice = 2;
			}
			pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
			choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			while( !( (1<<choice) & mode) ){
					choice++;
			}
			if( choice == 0 ){
				if( pick_target(&td, "TARGET_PLAYER") ){
					instance->info_slot = 66+choice;
				}
			}
			else if( choice == 1 ){
					instance->info_slot = 66+choice;
			}
			else if( choice == 2 ){
					if( pick_target(&td1, "TARGET_CREATURE") ){
						instance->info_slot = 66+choice;
					}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				int count = count_graveyard(instance->targets[0].player)-1;
				while( count > -1 ){
						rfg_card_from_grave(instance->targets[0].player, count);
						count--;
				}
			}
			if( instance->info_slot == 67 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				new_damage_all(player, card, 2, 2, NDA_ALL_CREATURES, &this_test);
			}
			if( instance->info_slot == 68 && valid_target(&td1) ){
				add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 2);
			}
			kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_jund_panorama(int player, int card, event_t event){
	return panorama(player, card, event, SUBTYPE_SWAMP, SUBTYPE_FOREST, SUBTYPE_MOUNTAIN);
}

int card_kederekt_creeper(int player, int card, event_t event){
	minimum_blockers(player, card, event, 2);
	deathtouch(player, card, event);
	return 0;
}

int card_kederekt_leviathan(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = DOESNT_MATCH;
		this_test.not_me = 1;
		new_manipulate_all(player, card, 2, &this_test, ACT_BOUNCE);
	}

	return unearth(player, event, 6, 0, 1, 0, 0, 0);
}

int card_keeper_of_progenitus(int player, int card, event_t event){
	if ((event == EVENT_COUNT_MANA || event == EVENT_TAP_CARD) && is_what(affected_card_controller, affected_card, TYPE_LAND)
		&& (has_subtype(affected_card_controller, affected_card, SUBTYPE_FOREST)
			|| has_subtype(affected_card_controller, affected_card, SUBTYPE_MOUNTAIN)
			|| has_subtype(affected_card_controller, affected_card, SUBTYPE_PLAINS))){
		// See comments in card_mana_flare().

		if (!in_play(player, card)){
			return 0;
		}

		if (event == EVENT_COUNT_MANA){
			if (is_tapped(affected_card_controller, affected_card) || is_animated_and_sick(affected_card_controller, affected_card)
				|| !can_produce_mana(affected_card_controller, affected_card) ){
				return 0;
			}

			int num_colors = 0;
			color_t col;
			card_instance_t* aff_instance = get_card_instance(affected_card_controller, affected_card);
			for (col = COLOR_COLORLESS; col <= COLOR_ARTIFACT; ++col){
				if (aff_instance->card_color & (1 << col)){
					++num_colors;
				}
			}

			if (num_colors > 0){
				declare_mana_available_hex(affected_card_controller, aff_instance->card_color, 1);
			} else {
				declare_mana_available(affected_card_controller, single_color_test_bit_to_color(aff_instance->card_color), 1);
			}
		} else {	// event == EVENT_TAP_CARD
			produce_mana_of_any_type_tapped_for(player, card, 1);
		}
	}

	return 0;
}

int card_kiss_of_the_amesha(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;
	td1.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td1, "TARGET_PLAYER");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				gain_life(instance->targets[0].player, 7);
				draw_cards(instance->targets[0].player, 2);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_knight_of_the_white_orchid(int player, int card, event_t event)
{
  /* Knight of the White Orchid	|W|W
   * Creature - Human Knight 2/2
   * First strike
   * When ~ enters the battlefield, if an opponent controls more lands than you, you may search your library for |Ha Plains card, put it onto the battlefield, then shuffle your library. */

  if (landsofcolor_controlled[1-player][COLOR_ANY] > landsofcolor_controlled[player][COLOR_ANY]
	  && comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_PLAINS));
	  test.subtype = get_hacked_subtype(player, card, SUBTYPE_PLAINS);
	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &test);
	}

  return 0;
}

int card_knight_captain_of_eos(int player, int card, event_t event){
	/* Knight-Captain of Eos	|4|W
	 * Creature - Human Knight 2/2
	 * When ~ enters the battlefield, put two 1/1 |Swhite Soldier creature tokens onto the battlefield.
	 * |W, Sacrifice a Soldier: Prevent all combat damage that would be dealt this turn. */

	if( comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_SOLDIER, 2);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SOLDIER, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1);
		if( spell_fizzled != 1 ){
			if( ! sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_SOLDIER, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		fog_effect(player, instance->parent_card);
	}

	return 0;
}

int card_kresh_the_bloodbraided(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability_and_store_values(player, card, event, ANYBODY, 0, &this_test, 1, 1);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		add_1_1_counters(player, card, instance->targets[1].player);
		instance->targets[11].card = 0;
		instance->targets[1].player = 0;
	}
	return 0;
}

int card_magma_spray(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			exile_if_would_be_put_into_graveyard(player, card, instance->targets[0].player, instance->targets[0].card, 1);
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_manaplasm(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int amount = get_cmc(instance->targets[1].player, instance->targets[1].card);
		pump_until_eot(player, card, player, card, amount, amount);
	}
	return 0;
}

int card_marble_chalice(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_master_of_etherium(int player, int card, event_t event)
{
  /* Master of Etherium	|2|U
   * Artifact Creature - Vedalken Wizard 100/100
   * ~'s power and toughness are each equal to the number of artifacts you control.
   * Other artifact creatures you control get +1/+1. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player && !is_humiliated(player, card) && player != -1)
	{
	  if (affected_card == card)
		event_result += count_permanents_by_type(player, TYPE_ARTIFACT);
	  else if (is_what(affected_card_controller, affected_card, TYPE_ARTIFACT))
		event_result += 1;
	}

  return 0;
}

int card_mayael_the_anima(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int amount = 5;
		if( count_deck(player) < amount ){
			amount = count_deck(player);
		}
		if( amount > 0 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature with power 5 or higher.");
			this_test.power = 4;
			this_test.power_flag = 2;
			this_test.create_minideck = amount;
			this_test.no_shuffle = 1;
			if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test) != -1 ){
				amount--;
			}
			if( amount > 0 ){
				put_top_x_on_bottom(player, player, amount);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 3, 0, 0, 1, 1, 1, 0, 0, 0);
}

int card_memory_erosion(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, 1-player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		mill(1-player, 2);
	}

	return global_enchantment(player, card, event);
}

int card_mighty_emergence(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.power = 4;
	this_test.power_flag = 2;

	if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &this_test) ){
		add_1_1_counters(instance->targets[1].player, instance->targets[1].card, 2);
	}

	return global_enchantment(player, card, event);
}

int card_mindlock_orb(int player, int card, event_t event){
	// the true code is in "tutors.c"
	return 0;
}

int card_minion_reflector(int player, int card, event_t event){
	// minion reflector

	card_instance_t *instance = get_card_instance(player, card);

	if( has_mana(player, COLOR_COLORLESS, 2) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.type_flag = F1_NO_TOKEN;

		if( new_specific_cip(player, card, event, player, 1+player, &this_test) ){
			charge_mana(player, COLOR_COLORLESS, 2);
			if( spell_fizzled != 1 ){
				token_generation_t token;
				copy_token_definition(player, card, &token, instance->targets[1].player, instance->targets[1].card);
				token.legacy = 1;
				token.special_code_for_legacy = &haste_and_sacrifice_eot;
				generate_token(&token);
			}
		}
	}

	return 0;
}

int card_mosstodon(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 5 | TARGET_PT_GREATER_OR_EQUAL;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_TRAMPLE, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_mycoloth(int player, int card, event_t event){

	/* Mycoloth	|3|G|G
	 * Creature - Fungus 4/4
	 * Devour 2
	 * At the beginning of your upkeep, put a 1/1 |Sgreen Saproling creature token onto the battlefield for each +1/+1 counter on ~. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		generate_tokens_by_id(player, card, CARD_ID_SAPROLING, count_1_1_counters(player, card) );
	}

	devour(player, card, event, 2);

	return 0;
}

int card_naya_battlemage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( (has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && can_target(&td)) ||
			(has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) && can_target(&td1))
		  ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) && can_target(&td1) ){
				choice = do_dialog(player, player, card, -1, -1, " Pump creature\n Tap target creature\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		int white = 0;
		int red = 1;
		if( choice == 1 ){
			red = 0;
			white = 1;
		}
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, red, white);
		if( spell_fizzled != 1 ){
			if( choice == 0 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				instance->info_slot = 66+choice;
				tap_card(player, card);
			}
			if( choice == 1 && pick_target(&td1, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				instance->info_slot = 66+choice;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66  && valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 0);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_naya_charm(int player, int card, event_t event){

	/* Naya Charm	|R|G|W
	 * Instant
	 * Choose one - ~ deals 3 damage to target creature; or return target card from a graveyard to its owner's hand; or tap all creatures target player
	 * controls. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return (((get_grave(player)[0] != -1 || get_grave(1-player)[0] != -1) && !graveyard_has_shroud(ANYBODY))
				|| can_target(&td)
				|| can_target(&td1));
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			int mode = (1<<3);
			if( can_target(&td) ){
				mode |=(1<<0);
			}
			if( (get_grave(player)[0] != -1 || get_grave(1-player)[0] != -1) && ! graveyard_has_shroud(2) ){
				mode |=(1<<1);
			}
			if( can_target(&td1) ){
				mode |=(1<<2);
			}
			char buffer[500];
			int pos = 0;
			int ai_choice = 0;
			if( mode & (1<<0) ){
				pos += scnprintf(buffer + pos, 500-pos, " 3 damage to target creature\n", buffer);
			}
			if( mode & (1<<1) ){
				pos += scnprintf(buffer + pos, 500-pos, " Regrowth\n", buffer);
				ai_choice = 1;
			}
			if( mode & (1<<2) ){
				pos += scnprintf(buffer + pos, 500-pos, " Tap all creatures\n", buffer);
				if( count_subtype(player, TYPE_CREATURE, -1) > 0 && count_subtype(1-player, TYPE_CREATURE, -1) > 0 ){
					ai_choice = 2;
				}
			}
			pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
			choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			while( !( (1<<choice) & mode) ){
					choice++;
			}
			if( choice == 0 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->info_slot = 66+choice;
				}
			}
			else if( choice == 1 ){
					test_definition_t test;
					new_default_test_definition(&test, 0, "Select target card.");

					if (select_target_from_either_grave(player, card, 0, AI_MAX_VALUE, AI_MIN_VALUE, &test, 0, 1) != -1){
						if (player == AI && instance->targets[0].player != player){
							ai_modifier -= 128;
						}
						instance->info_slot = 66 + choice;
					}
			}
			else if( choice == 2 ){
					if( pick_target(&td1, "TARGET_PLAYER") ){
						instance->info_slot = 66+choice;
					}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
			}
			if( instance->info_slot == 67 ){
				int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
				if( selected != -1 ){
					const int *grave = get_grave(instance->targets[0].player);
					add_card_to_hand(instance->targets[0].player, grave[selected]);
					remove_card_from_grave(instance->targets[0].player, selected);
				}
			}
			if( instance->info_slot == 68 && valid_target(&td1) ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				new_manipulate_all(player, card, instance->targets[0].player, &this_test, ACT_TAP);
			}
			kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_naya_panorama(int player, int card, event_t event){
	return panorama(player, card, event, SUBTYPE_FOREST, SUBTYPE_PLAINS, SUBTYPE_MOUNTAIN);
}

int card_necrogenesis(int player, int card, event_t event){
	/* Necrogenesis	|B|G
	 * Enchantment
	 * |2: Exile target creature card from a graveyard. Put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL) ){
		if( count_graveyard_by_type(1-player, TYPE_CREATURE) && ! graveyard_has_shroud(1-player) ){
			return 1;
		}
		if( count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
			instance->number_of_targets = 0;
			instance->targets[0].player = 1-player;
			if( count_graveyard_by_type(1-player, TYPE_CREATURE) && ! graveyard_has_shroud(1-player) ){
				if( count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player) && player != AI ){
					target_definition_t td;
					default_target_definition(player, card, &td, TYPE_CREATURE);
					td.zone = TARGET_ZONE_PLAYERS;
					td.illegal_abilities = 0;

					pick_target(&td, "TARGET_PLAYER");
				}
			}
			else{
				instance->targets[0].player = player;
			}
			if( spell_fizzled != 1 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				int ai_mode = instance->targets[0].player == player ? AI_MIN_VALUE : AI_MAX_VALUE;
				if( select_target_from_grave_source(player, card, instance->targets[0].player, 0, ai_mode, &this_test, 1) == -1 ){
					spell_fizzled = 1;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			rfg_card_from_grave(instance->targets[0].player, selected);
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
		}
	}

	return global_enchantment(player, card, event);
}

int card_obelisk_of_bant(int player, int card, event_t event){
	// also code for all the others obelisks.
	return mana_producer(player, card, event);
}

int card_onyx_goblet(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_ooze_garden(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) &&
		can_sorcery_be_played(player, event)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, SUBTYPE_OOZE, 1, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0);
		if( ! sacrifice(player, card, player, 0, TYPE_CREATURE, 0, SUBTYPE_OOZE, 1, 0, 0, 0, 0, -1, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_OOZE, &token);
		token.pow = instance->targets[2].player;
		token.tou = instance->targets[2].player;
		generate_token(&token);
	}

	return global_enchantment(player, card, event);;
}

int card_predator_dragon(int player, int card, event_t event){
	haste(player, card, event);
	devour(player, card, event, 2);
	return 0;
}

int card_prince_of_thralls(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) && affected_card_controller != player ){
		if( is_what(affected_card_controller, affected_card, TYPE_PERMANENT) && ! is_token(affected_card_controller, affected_card) ){
			card_instance_t *dead = get_card_instance(affected_card_controller, affected_card);
			if( dead->kill_code != KILL_REMOVE ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				int position = instance->targets[11].player;
				if( position < 10 ){
					instance->targets[position].card = get_id(affected_card_controller, affected_card);
					instance->targets[11].player++;
				}
			}
		}
	}

	if( resolve_graveyard_trigger(player, card, event) == 1 && instance->targets[11].player > 0){
		int i;
		for(i=0; i<instance->targets[11].player; i++){
			int id = instance->targets[i].card;
			if( id != -1 ){
				int choice = 1;
				int ai_choice = 0;
				if( life[player] < 9 ){
					ai_choice = 1;
				}
				if( can_pay_life(1-player, 3) ){
					card_ptr_t* c_me = cards_ptr[ id ];
					char buffer[1500];
					scnprintf(buffer, 1500, " Pay 3 life and keep %s\n Pass", c_me->name);
					choice = do_dialog(1-player, player, card, -1, -1, buffer, ai_choice);
				}
				if( choice == 0 ){
					lose_life(1-player, 3);
				}
				if( choice == 1 ){
					seek_grave_for_id_to_reanimate(player, card, 1-player, id, REANIMATE_DEFAULT);
				}
			}
			instance->targets[i+1].card = -1;
		}
		instance->targets[11].player = 0;
	}
	return 0;
}

int card_punish_ignorance(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			instance->targets[1].player = card_on_stack_controller;
			return result;
		}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			gain_life(player, 3);
			lose_life(instance->targets[1].player, 3);
			return card_counterspell(player, card, event);
	}
	else{
		return card_counterspell(player, card, event);
	}

	return 0;
}

int card_qasali_ambusher(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_FOREST) &&
			check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_PLAINS) && count_attackers(1-player) > 0
		  ){
			null_casting_cost(player, card);
		} else if (!can_sorcery_be_played(player, event)){
			infinite_casting_cost();
		}
	}

	return flash(player, card, event);
}

int card_quietus_spike(int player, int card, event_t event)
{
  /* Quietus Spike	|3
   * Artifact - Equipment
   * Equipped creature has deathtouch.
   * Whenever equipped creature deals combat damage to a player, that player loses half his or her life, rounded up.
   * Equip |3 */

  if (equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRACE_DAMAGED_PLAYERS))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
	  instance->targets[1].player = 0;
	  int p;
	  for (p = 0; p <= 1; ++p)
		for (; times_damaged[p] > 0; --times_damaged[p])
		  lose_life(p, (life[p] + 1) / 2);
	}

  return vanilla_equipment(player, card, event, 3, 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
}

int card_rafiq_of_the_many( int player, int card, event_t event){

	check_legend_rule(player, card, event);

	exalted(player, card, event, KEYWORD_DOUBLE_STRIKE, 0);

	return 0;
}

int card_rakeclaw_gargantuan(int player, int card, event_t event){//UNUSEDCARD

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 5 | TARGET_PT_GREATER_OR_EQUAL;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FIRST_STRIKE, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_ranger_of_eos(int player, int card, event_t event){
	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card with CMC 1 or less.");
		this_test.cmc = 2;
		this_test.cmc_flag = 3;
		this_test.qty = 2;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		shuffle(player);
	}
	return 0;
}

static int realm_razer_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_ACTIVATE && player != AI ){
		int p;
		for(p=0; p<2; p++){
			int c;
			for(c=active_cards_count[p]-1; c>-1; c--){
				if( is_what(p, c, TYPE_EFFECT) && !(p == player && c == card) ){
					card_instance_t *lnd = get_card_instance(p, c);
					if( lnd->targets[1].card == instance->targets[1].card ){
						if( lnd->targets[0].player == instance->targets[0].player && lnd->targets[0].card == instance->targets[0].card ){
							int k;
							for(k=2; k<19; k++){
								if( lnd->targets[k].player != -1 ){
									if( check_rfg(lnd->targets[k].player, cards_data[lnd->targets[k].card].id) ){
										return 1;
									}
								}
								else{
									break;
								}
							}
						}
					}
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int lands[2][500];
		int lc[2] = {0, 0};
		int p;
		for(p=0; p<2; p++){
			int c;
			for(c=active_cards_count[p]-1; c>-1; c--){
				if( is_what(p, c, TYPE_EFFECT) && !(p == player && c == card) ){
					card_instance_t *lnd = get_card_instance(p, c);
					if( lnd->targets[1].card == instance->targets[1].card ){
						if( lnd->targets[0].player == instance->targets[0].player && lnd->targets[0].card == instance->targets[0].card ){
							int k;
							for(k=2; k<19; k++){
								if( lnd->targets[k].player != -1 ){
									if( check_rfg(lnd->targets[k].player, cards_data[lnd->targets[k].card].id) ){
										lands[lnd->targets[k].player][lc[lnd->targets[k].player]] = lnd->targets[k].card;
										lc[lnd->targets[k].player]++;
									}
								}
								else{
									break;
								}
							}
						}
					}
				}
			}
		}
		show_deck( player, lands[player], lc[player], "Realm Razer exiled these lands you own.", 0, 0x7375B0 );
		show_deck( player, lands[1-player], lc[1-player], "Realm Razer exiled these lands onwed by your opponent.", 0, 0x7375B0 );
		spell_fizzled = 1;
	}

	if( leaves_play(instance->targets[0].player, instance->targets[0].card, event) ){
		int lands[2][500];
		int lc[2] = {0, 0};
		int p;
		for(p=0; p<2; p++){
			int c;
			for(c=active_cards_count[p]-1; c>-1; c--){
				if( is_what(p, c, TYPE_EFFECT) && !(p == player && c == card) ){
					card_instance_t *lnd = get_card_instance(p, c);
					if( lnd->targets[1].card == instance->targets[1].card ){
						if( lnd->targets[0].player == instance->targets[0].player && lnd->targets[0].card == instance->targets[0].card ){
							int k;
							for(k=2; k<19; k++){
								if( lnd->targets[k].player != -1 ){
									if( check_rfg(lnd->targets[k].player, cards_data[lnd->targets[k].card].id) ){
										lands[lnd->targets[k].player][lc[lnd->targets[k].player]] = lnd->targets[k].card;
										lc[lnd->targets[k].player]++;
										remove_card_from_rfg(lnd->targets[k].player, cards_data[lnd->targets[k].card].id);
									}
								}
								else{
									break;
								}
							}
							kill_card(player, card, KILL_REMOVE);
						}
					}
				}
			}
		}
		for(p=0; p<2; p++){
			int c;
			for(c=0; c<lc[p]; c++){
				int card_added = add_card_to_hand(p, lands[p][c]);
				put_into_play(p, card_added);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_realm_razer(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int effect_legacy = create_legacy_activate(player, card, &realm_razer_legacy);
		card_instance_t *instance = get_card_instance( player, effect_legacy);
		instance->targets[0].player = player;
		instance->targets[0].card = card;
		instance->number_of_targets = 1;
		instance->targets[1].card = get_id(player, card);
		int storage_legacy = create_legacy_effect(player, card, &empty);
		add_status(player, storage_legacy, STATUS_INVISIBLE_FX);
		instance = get_card_instance( player, storage_legacy);
		instance->targets[0].player = player;
		instance->targets[0].card = card;
		instance->number_of_targets = 1;
		instance->targets[1].card = get_id(player, card);
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_LAND) ){
						if( ! is_token(i, count) ){
							int iid = get_original_internal_card_id(i, count);
							int owner = get_owner(i, count);
							int found = 0;
							while( found == 0 ){
									int k;
									for(k=2; k<19; k++){
										if( instance->targets[k].player == -1 ){
											instance->targets[k].player = owner;
											instance->targets[k].card = iid;
											found = 1;
											break;
										}
									}
									if( ! found ){
										storage_legacy = create_legacy_effect(player, card, &empty);
										add_status(player, storage_legacy, STATUS_INVISIBLE_FX);
										instance = get_card_instance( player, storage_legacy);
										instance->targets[0].player = player;
										instance->targets[0].card = card;
										instance->number_of_targets = 1;
										instance->targets[1].card = get_id(player, card);
									}
							}
							kill_card(i, count, KILL_REMOVE);
						}
					}
					count--;
			}
		}
	}

	return 0;
}

int card_relic_of_progenitus(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_target(&td) ){
				return 1;
			}
			if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Target exile a card\n Exile all graveyards\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, choice, 0, 0, 0, 0, 0) ){
				if( choice == 0 ){
					if( pick_target(&td, "TARGET_PLAYER") ){
						tap_card(player, card);
						instance->number_of_targets = 1;
						instance->info_slot = 66+choice;
					}
				}
				else if( choice == 1 ){
						instance->info_slot = 66+choice;
						kill_card(player, card, KILL_REMOVE);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");
			new_global_tutor(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test);
		}
		if( instance->info_slot == 67 ){
			int i;
			for(i=0; i<2; i++){
				int count = count_graveyard(i)-1;
				while( count > -1 ){
						rfg_card_from_grave(i, count);
						count--;
				}
			}
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_resounding_roar(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 2);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && can_target(&td) ){
		td.allow_cancel = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 6, 6);
		}
	}

	return cycling(player, card, event, 5, 0, 0, 1, 1, 1);
}

int card_resounding_scream(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				discard(instance->targets[0].player, DISC_RANDOM, player);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && can_target(&td) ){
		td.allow_cancel = 0;
		if( pick_target(&td, "TARGET_PLAYER") ){
			new_multidiscard(instance->targets[0].player, 2, DISC_RANDOM, player);
		}
	}

	return cycling(player, card, event, 5, 1, 1, 0, 1, 0);
}

int card_resounding_silence(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && can_target(&td) ){
		int rfged = 0;
		while( rfged < 2 && can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
				rfged++;
			}
			else{
				break;
			}
		}
	}

	return cycling(player, card, event, 5, 0, 1, 1, 0, 1);
}

int card_resounding_thunder(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, 3);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && can_target(&td) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, 6);
		}
	}

	return cycling(player, card, event, 5, 1, 0, 1, 1, 0);
}

int card_resounding_wave(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PERMANENT");
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && can_target(&td) ){
		int rfged = 0;
		while( rfged < 2 && can_target(&td) ){
			if( pick_target(&td, "TARGET_PERMANENT") ){
				instance->number_of_targets = 1;
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				rfged++;
			}
			else{
				break;
			}
		}
	}

	return cycling(player, card, event, 5, 1, 1, 0, 0, 1);
}

int card_sacellum_godspeaker(int player, int card, event_t event){

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card with power 5 or more.");
	this_test.power = 4;
	this_test.power_flag = 2;
	this_test.zone = TARGET_ZONE_HAND;

	if( event == EVENT_COUNT_MANA && ! is_tapped(player, card) && ! is_sick(player, card) && affect_me(player, card) ){
		declare_mana_available(player, COLOR_GREEN, check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test));
	}

	if(event == EVENT_CAN_ACTIVATE ){
		if( ! is_sick(player, card)  && ! is_tapped(player, card)  ){
			return can_produce_mana(player, card);
		}
	}

	else if( event == EVENT_ACTIVATE && affect_me(player, card) ){
			int result = reveal_cards_from_your_hand(player, card, &this_test);
			produce_mana_tapped(player, card, COLOR_GREEN, result);
	}

	return 0;
}

int card_salvage_titan(int player, int card, event_t event){

	/* Salvage Titan	|4|B|B
	 * Artifact Creature - Golem 6/4
	 * You may sacrifice three artifacts rather than pay ~'s mana cost.
	 * Exile three artifact cards from your graveyard: Return ~ from your graveyard to your hand. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( can_sacrifice_as_cost(player, 3, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
			card_ptr_t* c = cards_ptr[ get_id(player, card) ];
			int cless = get_updated_casting_cost(player, card, -1, event, c->req_colorless);
			if (!has_mana_multi_a(player, cless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white)){
				infinite_casting_cost();
			}
		}
	}

	if( event == EVENT_SET_COLOR && affect_me(player, card) ){
		event_result |= cards_data[ instance->internal_card_id ] .color;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			if( instance->info_slot == 1 ){
				int choice = 0;
				if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
					choice = do_dialog(player, player, card, -1, -1, " Sac 3 artifacts\n Play normally\n Cancel", 1);
				}
				if( choice == 0 ){
					if( sacrifice(player, card, player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
						impose_sacrifice(player, card, player, 2, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
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
	}

	if(event == EVENT_GRAVEYARD_ABILITY){
		if( count_graveyard_by_type(player, TYPE_ARTIFACT) >= 3 ){
			return GA_RETURN_TO_HAND;
		}
	}

  if (event == EVENT_PAY_FLASHBACK_COSTS)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ARTIFACT, "Select three artifact cards.");

	  if (!select_multiple_cards_from_graveyard(player, player, -1, AI_MIN_VALUE, &test, 3, &instance->targets[0]))
		{
		  spell_fizzled = 1;
		  return 0;
		}

	  int i;
	  for (i = 0; i < 3; ++i)
		rfg_card_from_grave(player, instance->targets[i].player);

	  // If no Salvage Titans left in graveyard, then fizzle; otherwise, obliterate the first one found (it'll be added to player's hand at resolution)
	  int pos = seek_grave_for_id_to_reanimate(player, card, player, CARD_ID_SALVAGE_TITAN, REANIMATEXTRA_LEAVE_IN_GRAVEYARD);
	  if (pos == -1)
		spell_fizzled = 1;
	  else
		{
		  /* Done here instead of activate_cards_in_hand() because it might end up being a different Salvage Titan being raised - if there are two or more in
		   * his graveyard, it's not reasonable to make the user guess which one will make the effect fizzle */
		  remove_card_from_grave(player, pos);
		  return GAPAID_REMAIN_IN_GRAVE;
		}
	}

  return 0;
}

int card_sanctum_gargoyle( int player, int card, event_t event){

	if( comes_into_play(player, card, event) && ! graveyard_has_shroud(2) ){
		char msg[100] = "Select an artifact card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

int card_sarkhan_vol(int player, int card, event_t event){

	/* Sarkhan Vol	|2|R|G
	 * Planeswalker - Sarkhan (4)
	 * +1: Creatures you control get +1/+1 and gain haste until end of turn.
	 * -2: Gain control of target creature until end of turn. Untap that creature. It gains haste until end of turn.
	 * -6: Put five 4/4 |Sred Dragon creature tokens with flying onto the battlefield. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		int priority_steal = 0;
		if( event == EVENT_ACTIVATE ){
			priority_steal = ((count_counters(player, card, COUNTER_LOYALTY)*2)-6)+15;
		}

		enum{
			CHOICE_PUMP_CREATURES = 1,
			CHOICE_STEAL,
			CHOICE_DRAGONS
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Pump your creatures", 1, 10, 1,
						"Act of Treason", can_target(&td), priority_steal, -2,
						"5 Dragons", 1, 20, -6);

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
				case CHOICE_STEAL:
					pick_target(&td, "TARGET_CREATURE");
					break;

				default:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_PUMP_CREATURES:
				pump_subtype_until_eot(instance->parent_controller, instance->parent_card, player, -1, 1, 1, 0, SP_KEYWORD_HASTE);
				break;

			case CHOICE_STEAL:
				if( valid_target(&td) ){
					if( instance->targets[0].player == player ){
						untap_card(instance->targets[0].player, instance->targets[0].card);
						pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
												0, 0, 0, SP_KEYWORD_HASTE);
					}
					else{
						effect_act_of_treason(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
					}
				}
				break;

			case CHOICE_DRAGONS:
			{
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_DRAGON, &token);
				token.pow = 4;
				token.tou = 4;
				token.qty = 5;
				generate_token(&token);
			}
			break;
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_scavenger_drake(int player, int card, event_t event){

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		card_instance_t *instance= get_card_instance(player, card);
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_scourglass(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT | TYPE_LAND);
		this_test.type_flag = 1;
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_ME|GAA_ONLY_ON_UPKEEP|GAA_IN_YOUR_TURN, MANACOST_X(0), 0, NULL, NULL);
}

int card_sedraxis_specter(int player, int card, event_t event)
{
  /* Sedraxis Specter	|U|B|R
   * Creature - Specter 3/2
   * Flying
   * Whenever ~ deals combat damage to a player, that player discards a card.
   * Unearth |1|B */

  whenever_i_deal_damage_to_a_player_he_discards_a_card(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE, 0);

  return unearth(player, event, MANACOST_XB(1,1));
}

int card_sedris_the_traitor_king(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_mana_multi(player, 2, 1, 0, 0, 0, 0) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 &&
			can_sorcery_be_played(player, event)
		  ){
		   return 1;
		}
	}

	if( event == EVENT_ACTIVATE){
		charge_mana_multi(player, 2, 1, 0, 0, 0, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature to unearth.");
		int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 1, AI_MAX_CMC, -1, &this_test);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, REANIMATE_UNEARTH);
		}
	}

	return 0;
}

int card_sharding_sphinx(int player, int card, event_t event){
	/* Sharding Sphinx	|4|U|U
	 * Artifact Creature - Sphinx 4/4
	 * Flying
	 * Whenever an artifact creature you control deals combat damage to a player, you may put a 1/1 |Sblue Thopter artifact creature token with flying onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	card_instance_t* damage = combat_damage_being_dealt(event);
	if( damage &&
		damage->damage_source_player == player &&
		(damage->targets[3].player & TYPE_CREATURE) &&	// probably redundant to status check
		(damage->targets[3].player & TYPE_ARTIFACT) &&
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER)
	  ){
		instance->info_slot++;
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player && instance->info_slot > 0 ){
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_AI(player);
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			if (--instance->info_slot > 0){
				instance->state &= ~STATE_PROCESSING;	// More optional triggers left.  Must be the first thing done during resolve trigger.
			}
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_THOPTER, &token);
			token.color_forced = COLOR_TEST_BLUE;
			generate_token(&token);
		}
		else if (event == EVENT_END_TRIGGER){
			instance->info_slot = 0;
		}
	}

	return 0;
}

int card_sharuum_the_hegemon(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		if( count_graveyard_by_type(player, TYPE_ARTIFACT) > 0 ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ARTIFACT, "Select an artifact card.");
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &test);
		}
	}

	return 0;
}

int card_sigil_blessing(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
				pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, 0);
			}
			kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_sigil_of_distinction(int player, int card, event_t event){

	/* Sigil of Distinction	|X
	 * Artifact - Equipment
	 * ~ enters the battlefield with X charge counters on it.
	 * Equipped creature gets +1/+1 for each charge counter on ~.
	 * Equip-Remove a charge counter from ~. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( IS_AI(player) && ! has_mana(player, COLOR_ARTIFACT, 2) ){
			ai_modifier-=1000;
		}
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			charge_mana(player, COLOR_ARTIFACT, -1);
			if( spell_fizzled != 1  ){
				instance->info_slot = x_value;
			}
		}
	}

	enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, instance->info_slot);

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && is_equipping(player, card) && affect_me(instance->targets[8].player, instance->targets[8].card)){
		event_result += count_counters(player, card, COUNTER_CHARGE);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return can_activate_basic_equipment(player, card, event, 0) && count_counters(player, card, COUNTER_CHARGE) > 0;
	}
	if( event == EVENT_ACTIVATE ){
		if( is_equipping(player, card) ){
			ai_modifier-=500;
		}
		if (activate_basic_equipment(player, card, 0)){
			remove_counter(player, card, COUNTER_CHARGE);
		}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
		resolve_activation_basic_equipment(player, card);
	}
	return 0;
}

int card_skill_borrower(int player, int card, event_t event){

	reveal_top_card(player, card, event);

	int *deck = deck_ptr[player];

	if( deck[0] != -1 && (is_what(-1, deck[0], TYPE_CREATURE) || is_what(-1, deck[0], TYPE_ARTIFACT)) ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[deck[0]].code_pointer;
			return ptFunction(player, card, EVENT_CAN_ACTIVATE);
		}

		if(event == EVENT_ACTIVATE ){
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[deck[0]].code_pointer;
			return ptFunction(player, card, EVENT_ACTIVATE);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[deck[0]].code_pointer;
			return ptFunction(player, card, EVENT_RESOLVE_ACTIVATION);
		}
	}
	return 0;
}

int card_skullmulcher(int player, int card, event_t event){
	/* Skullmulcher	|4|G
	 * Creature - Elemental 3/3
	 * Devour 1
	 * When ~ enters the battlefield, draw a card for each creature it devoured. */

	devour(player, card, event, 1);

	if( comes_into_play(player, card, event) ){
		draw_cards(player, get_card_instance(player, card)->targets[1].card);
	}
	return 0;
}

int card_souls_fire(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( select_target(player, card, &td, "Select target creature you control.", &(instance->targets[0])) ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				if( ! select_target(player, card, &td1, "Select target creature or player.", &(instance->targets[1])) ){
					spell_fizzled = 1;
				}
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( validate_target(player, card, &td1, 1) && in_play(instance->targets[0].player, instance->targets[0].card) ){
				int dmg = get_power(instance->targets[0].player, instance->targets[0].card);
				damage_creature(instance->targets[1].player, instance->targets[1].card, dmg, instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_spearbreaker_behemoth(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 5 | TARGET_PT_GREATER_OR_EQUAL;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	indestructible(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_sphinx_sovereign(int player, int card, event_t event){

	if( current_turn == player && eot_trigger(player, card, event) ){
		if( is_tapped(player, card) ){
			lose_life(1-player, 3);
		}
		else{
			gain_life(player, 3);
		}
	}
	return 0;
}

int card_sprouting_thrinax(int player, int card, event_t event){
	/* Sprouting Thrinax	|B|R|G
	 * Creature - Lizard 3/3
	 * When ~ dies, put three 1/1 |Sgreen Saproling creature tokens onto the battlefield. */

	if( graveyard_from_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_SAPROLING, 3);
	}
	return 0;
}

int card_steward_of_valeron(int player, int card, event_t event){
	vigilance(player, card, event);
	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		return 0;	// Don't make extra-reluctant to attack, due to vigilance
	}
	return mana_producing_creature(player, card, event, 24, COLOR_GREEN, 1);
}

int card_stoic_angel(int player, int card, event_t event){
	/*
	  Stoic Angel English |1|W|U|G
	  Creature - Angel 3/4, 1WUG (4)
	  Flying, vigilance
	  Players can't untap more than one creature during their untap steps.
	*/
	vigilance(player, card, event);
	return card_smoke(player, card, event);
}

int card_tar_fiend(int player, int card, event_t event){
	/* Tar Fiend	|5|B
	 * Creature - Elemental 4/4
	 * Devour 2
	 * When ~ enters the battlefield, target player discards a card for each creature it devoured. */

	devour(player, card, event, 2);

	if( comes_into_play(player, card, event) ){
		card_instance_t* instance = get_card_instance(player, card);
		int devoured = instance->targets[1].card;
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;
		if (can_target(&td) && pick_target(&td, "TARGET_PLAYER") && devoured > 0){
			new_multidiscard(instance->targets[0].player, devoured, 0, player);
		}
	}
	return 0;
}
static int tezzeret_the_seeker_legacy(int player, int card, event_t event){
	if( event == EVENT_CHANGE_TYPE ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		this_test.type_flag = F1_NO_CREATURE;
		global_type_change(player, card, event, player, TYPE_CREATURE, &this_test, 5, 5, 0, 0, 0);
	}
	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_tezzeret_the_seeker(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 23) ){ return 0; }

	/* Tezzeret the Seeker	|3|U|U
	 * Planeswalker - Tezzeret (4)
	 * +1: Untap up to two target artifacts.
	 * -X: Search your library for an artifact card with converted mana cost X or less and put it onto the battlefield. Then shuffle your library.
	 * -5: Artifacts you control become 5/5 artifact creatures until end of turn. */

	if (IS_ACTIVATING(event)){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);
		int priority_tutor = 0;
		int priority_animate = 0;
		if( event == EVENT_ACTIVATE ){
			int max_t = count_counters(player, card, COUNTER_LOYALTY)-1;
			int i = 0;
			while( deck_ptr[player][i] != -1 ){
					if( is_what(-1, deck_ptr[player][i], TYPE_ARTIFACT) && get_cmc_by_internal_id(deck_ptr[player][i]) <= max_t ){
						int new_p = (get_cmc_by_internal_id(deck_ptr[player][i]) * 4)+4;
						priority_tutor = new_p > priority_tutor ? new_p : priority_tutor;
						if( priority_tutor == (max_t * 4)+4 ){
							break;
						}
					}
					i++;
			}

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ARTIFACT);
			this_test.type_flag = F1_NO_CREATURE;
			priority_animate += check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test) * 4;
			if( priority_animate ){
				priority_animate += ((count_counters(player, card, COUNTER_LOYALTY)*5)-25);
			}
		}

		enum{
			CHOICE_UNTAP = 1,
			CHOICE_TUTOR,
			CHOICE_ANIMATE
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Untap up to 2 artifacts", 1, 10, 1,
						"Tutor an artifact", 1, priority_tutor, 0,
						"Global artifact animation", 1, priority_animate, -5);

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
				case CHOICE_UNTAP:
				{
					if( pick_target(&td, "TARGET_ARTIFACT") ){
						state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
						if( can_target(&td) ){
							new_pick_target(&td, "TARGET_ARTIFACT", 1, 0);
						}
						state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
					}
				}
				break;

				case CHOICE_TUTOR:
				{
					if( player == AI ){
						remove_counters(player, card, COUNTER_LOYALTY, priority_tutor/4);
						instance->targets[1].player = priority_tutor/4;
					}
					else{
						int ctr = choose_a_number(player, "How many Loyalty you'll pay ?", count_counters(player, card, COUNTER_LOYALTY));
						if( ctr > -1 || ctr <= count_counters(player, card, COUNTER_LOYALTY) ){
							remove_counters(player, card, COUNTER_LOYALTY, ctr);
							instance->targets[1].player = ctr;
						}
						else{
							spell_fizzled = 1;
						}
					}
				}
				break;

			  case CHOICE_ANIMATE:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_UNTAP:
			{
				int i;
				for(i=0;i<instance->number_of_targets;i++){
					if( validate_target(player, card, &td, i) ){
						untap_card( instance->targets[i].player, instance->targets[i].card );
					}
				}
			}
			break;

			case CHOICE_TUTOR:
			{
				char buffer[500];
				scnprintf(buffer, 500, " Select an artifact with CMC %d or less.", instance->targets[1].player);
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ARTIFACT, buffer);
				this_test.cmc = instance->targets[1].player+1;
				this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
			}
			break;

			case CHOICE_ANIMATE:
				create_legacy_effect(instance->parent_controller, instance->parent_card, &tezzeret_the_seeker_legacy);
				break;
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_thorn_thrash_viashino(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	devour(player, card, event, 2);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, card, instance->parent_controller, instance->parent_card, 0, 0, KEYWORD_TRAMPLE, 0);
	}

	return generic_activated_ability(player, card, event, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_thoughtcutter_agent(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 1);
			if( player != AI ){
				reveal_target_player_hand(instance->targets[0].player);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 1, 1, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_thunder_thrash_elder(int player, int card, event_t event){
	devour(player, card, event, 3);
	return 0;
}

int card_tidehollow_sculler(int player, int card, event_t event){
	card_instance_t *this_instance = get_card_instance(player, card);
	if( comes_into_play(player, card, event) > 0  ){
		this_instance->info_slot = -1;
		int opponent = 1-player;
		if(hand_count[opponent]==0){
			return 0;
		}
		int cards_array[ 500 ];
		int i=0;
		int hand_index = 0;
		int AI_pick = -1;
		for(i=0;i<active_cards_count[opponent]; i++){
			card_instance_t *instance = get_card_instance(opponent, i);
			if( ! ( instance->state & STATE_INVISIBLE ) && ! ( instance->state & STATE_IN_PLAY )  ){
				int id = instance->internal_card_id;
				if( id > -1 ){
					if( ! ( cards_data[ id ].type & TYPE_LAND  )){
						AI_pick = hand_index;
					}
					cards_array[hand_index++] = id;
				}
			}

		}

		// pick a card
		int selected = AI_pick;
		int valid_choice = 1;
		if( player == HUMAN ){
			selected = show_deck( player, cards_array, hand_count[opponent], "Pick a card", 0, 0x7375B0 );
			if( selected == -1 ){
				return 0;
			}

			// if a land was picked, that's invalid
			if( cards_data[ cards_array[selected] ].type & TYPE_LAND ){
				valid_choice = 0;
			}
		}

		// find the chosen card and exile it
		if( valid_choice == 1 ){
			hand_index = 0;
			for(i=0;i<active_cards_count[opponent]; i++){
				card_instance_t *instance = get_card_instance(opponent, i);
				if( ! ( instance->state & STATE_INVISIBLE ) && ! ( instance->state & STATE_IN_PLAY )  ){
					if( instance->internal_card_id > -1 ){
						if( hand_index++ == selected ){
							create_card_name_legacy(player, card, get_id( opponent, i ) );
							this_instance->info_slot = instance->internal_card_id;
							kill_card(opponent, i, KILL_REMOVE);
							break;
						}
					}
				}
			}
		}
	}

	if(leaves_play(player, card, event) && this_instance->info_slot >= 0){
		int csvid = cards_data[this_instance->info_slot].id;
		if (check_rfg(1 - player, csvid)){
			remove_card_from_rfg(1 - player, csvid);
			add_card_to_hand( 1-player, this_instance->info_slot);
			this_instance->info_slot = -1;
		}
	}
	return 0;
}

int card_titanic_ultimatum(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			pump_subtype_until_eot(player, card, player, -1, 5, 5, KEYWORD_TRAMPLE+KEYWORD_FIRST_STRIKE, SP_KEYWORD_LIFELINK);
			kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_vein_drinker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		int vd_damage = get_power(player, instance->parent_card);
		int t_damage = get_power(instance->targets[0].player, instance->targets[0].card);
		damage_creature(instance->targets[0].player, instance->targets[0].card, vd_damage, player, instance->parent_card);
		damage_creature(player, instance->parent_card, t_damage,instance->targets[0].player, instance->targets[0].card );
	}

	if( sengir_vampire_trigger(player, card, event, 2) ){
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_vicious_shadows(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, 0);
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;
		if( resolve_gfp_ability(player, card, event, can_target(&td1) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			int i;
			for(i=0; i<instance->targets[11].card; i++){
				if( pick_target(&td1, "TARGET_PLAYER") ){
					instance->number_of_targets = 1;
					if( hand_count[instance->targets[0].player] > 0 ){
						damage_player(instance->targets[0].player, hand_count[instance->targets[0].player], player, card);
					}
				}
			}
			instance->targets[11].card = 0;
		}
	}
	return global_enchantment(player, card, event);
}

int card_violent_ultimatum(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 30) ){ return 0; }
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	card_instance_t *instance = get_card_instance(player, card);

	if ( event == EVENT_CAN_CAST && target_available (player, card, &td) >=3 ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int i;
			for(i=0;i<3;i++){
				if( i > 0 ){
					td.allow_cancel = 0;
				}
				if( new_pick_target(&td, "TARGET_PERMANENT", i, 1) ){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 1);
				}
			}
			for(i=0;i<3;i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0;i<3;i++){
				if( validate_target(player, card, &td, i) ){
					kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}


int card_viscera_dragger(int player, int card, event_t event){
  return cycling(player, card, event, 2, 0, 0, 0, 0, 0) + unearth(player, event, 1, 1, 0, 0, 0, 0);
}

int card_where_ancients_tread(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.power = 4;
	this_test.power_flag = 2;
	if( new_specific_cip(player, card, event, player, 1+player, &this_test) && can_target(&td1) ){
		if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
			instance->number_of_targets = 1;
			damage_creature_or_player(player, card, event, 5);
		}
	}
	return global_enchantment(player, card, event);
}

	int card_wild_nacatal(int player, int card, event_t event){

		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			int count, mountain = 0, plains = 0;
			for( count = 0; count < active_cards_count[player]; ++count ){
				if( in_play(player, count) ){
					if( !mountain && has_subtype(player, count, SUBTYPE_MOUNTAIN) ){
						mountain = 1;
						if (plains)
							break;
					}
					if( !plains && has_subtype(player, count, SUBTYPE_PLAINS) ){
						plains = 1;
						if (mountain)
							break;
					}
				}
			}
			event_result += mountain + plains;
		}

		return 0;
	}

int card_worldheart_phoenix(int player, int card, event_t event){
	if(event == EVENT_GRAVEYARD_ABILITY){
		if( has_mana_multi( player, 0, 1, 1, 1, 1, 1) && can_sorcery_be_played(player, event) ){
			return GA_RETURN_TO_PLAY_MODIFIED;
		}
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi( player, 0, 1, 1, 1, 1, 1);
		if( spell_fizzled != 1){
			return GAPAID_REMOVE;
		}
	}
	if( event == EVENT_RETURN_TO_PLAY_FROM_GRAVE_MODIFIED ){
		add_1_1_counters(player, card, 2);
	}
	return 0;
}
