#include "manalink.h"

// ------------ General functions
int miracle(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && !suppress_draw ){
		if( reason_for_trigger_controller == player ){
			int trig = 0;
			int cost[6] = {0, 0, 0, 0, 0, 0};
			int ai_priority = 2;
			if( cards_drawn_this_turn[player] < 2 ){
				int *deck = deck_ptr[player];
				int iid = deck[0];
				int id = cards_data[iid].id;
				if( cards_data[deck[0]].cc[2] == 15 ){
					if( id == CARD_ID_BANISHING_STROKE ){
						if( player == AI ){
							target_definition_t td;
							default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT | TYPE_CREATURE);
							td.allowed_controller = 1-player;
							td.illegal_abilities = KEYWORD_PROT_WHITE;
							ai_priority = can_target(&td) ? 2 : 0;
						}
						cost[5] = 1;
					}
					if( id == CARD_ID_TEMPORAL_MASTERY ){
						cost[0] = 1;
						cost[2] = 1;
					}
					if( id == CARD_ID_THUNDEROUS_WRATH ){
						if( player == AI ){
							target_definition_t td;
							default_target_definition(player, card, &td, TYPE_CREATURE);
							td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
							ai_priority = can_target(&td) ? 2 : 0;
						}
						cost[4] = 1;
					}
					if( id == CARD_ID_BONFIRE_OF_THE_DAMNED ){
						if( player == AI ){
							target_definition_t td;
							default_target_definition(player, card, &td, TYPE_CREATURE);
							td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
							ai_priority = can_target(&td) ? 2 : 0;
						}
						cost[4] = 1;
					}
					if( id == CARD_ID_REFORGE_THE_SOUL ){
						ai_priority = (7-hand_count[1-player]) < (7-hand_count[player]) ? 2 : 0;
						cost[0] = 1;
						cost[4] = 1;
					}
					if( id == CARD_ID_REVENGE_OF_THE_HUNTED || id == CARD_ID_BLESSINGS_OF_NATURE){
						if( player == AI ){
							target_definition_t td;
							default_target_definition(player, card, &td, TYPE_CREATURE);
							td.preferred_controller = player;
							ai_priority = can_target(&td) ? 2 : 0;
						}
						cost[3] = 1;
					}
					if( id == CARD_ID_ENTREAT_THE_ANGELS ){
						ai_priority = has_mana_multi(player, MANACOST_XW(2, 2)) ? 2 : 0;
						cost[5] = 2;
					}
					if( id == CARD_ID_TERMINUS ){
						if( player == AI ){
							ai_priority = count_subtype(1-player, TYPE_CREATURE, -1)-count_subtype(player, TYPE_CREATURE, -1) > 1 ? 2 : 0;
						}
						cost[5] = 1;
					}
					if( id == CARD_ID_DEVASTATION_TIDE ){
						if( player == AI ){
							test_definition_t this_test;
							default_test_definition(&this_test, TYPE_LAND);
							this_test.type_flag = DOESNT_MATCH;
							ai_priority = 	check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test)-
											check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test) > 2 ? 2 : 0;
						}
						cost[0] = 1;
						cost[2] = 1;
					}
					if( id == CARD_ID_VANISHMENT ){
						if( player == AI ){
							target_definition_t td;
							default_target_definition(player, card, &td, TYPE_PERMANENT);
							td.illegal_type = TYPE_LAND;
							ai_priority = can_target(&td) ? 2 : 0;
						}
						cost[2] = 1;
					}
					int cless = get_updated_casting_cost(player, -1, deck[0], event, cost[0]);
					if( has_mana_multi(player, cless, cost[1], cost[2], cost[3], cost[4], cost[5]) ){
						trig = 1;
					}
					if( trig == 1 && (! can_legally_play_csvid(player, id) || (get_cmc_by_id(id) > 3 && check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG))) ){
						trig = 0;
					}

				}
			}

			if( trig == 1){
				if(event == EVENT_TRIGGER){
					event_result |= 2u;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						int *deck = deck_ptr[player];
						int iid = deck[0];
						int id = cards_data[iid].id;
						int cless = get_updated_casting_cost(player, -1, iid, event, cost[0]);
						if (DIALOG(player, card, EVENT_ACTIVATE,
								   DLG_FULLCARD_ID(iid), DLG_NO_STORAGE, DLG_NO_CANCEL,
								   "Cast", 1, ai_priority,
								   "Pass", 1, 1) == 1 &&
							charge_mana_multi_while_resolving_csvid(id, EVENT_RESOLVE_SPELL, player, cless, cost[1], cost[2], cost[3], cost[4], cost[5])
						   ){
							remove_card_from_deck(player, 0);
							play_card_in_hand_for_free(player, add_card_to_hand(player, iid));	// Inaccurate - won't run draw triggers
							suppress_draw = 1;
						}
				}
			}
		}
	}
	return 0;
}

static int is_paired(int player, int card){
	return check_special_flags2(player, card, SF2_PAIRED);
}

static int paired_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		if( ! in_play(instance->targets[1].player, instance->targets[1].card) ||
			instance->damage_target_player != instance->targets[1].player
		  ){
			remove_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_PAIRED);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

static void pair(int player, int card, int player2, int card2){
	card_instance_t *instance = get_card_instance(player, card);

	instance->targets[3].player = player2;
	instance->targets[3].card = card2;

	set_special_flags2(player, card, SF2_PAIRED);
	set_special_flags2(player2, card2, SF2_PAIRED);

	int legacy = create_targetted_legacy_effect(player, card, &paired_legacy, player2, card2);
	card_instance_t *instance3 = get_card_instance(player,legacy);
	instance3->targets[1].player = player;
	instance3->targets[1].card = card;
}

static int boost_paired_creature(int player, int card, event_t event, int pow, int tou, int key, int s_key){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[3].player > -1 ){
		modify_pt_and_abilities(instance->targets[3].player, instance->targets[3].card, event, pow, tou, key);
		if( s_key > 0 ){
			special_abilities(instance->targets[3].player, instance->targets[3].card, event, s_key, player, card);
		}
	}
	return 0;
}

static void pump_paired_creature_until_eot(int player, int card, int pow, int tou, int key, int s_key){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[3].player > -1 ){
		pump_ability_until_eot(player, card, instance->targets[3].player, instance->targets[3].card, pow, tou, key, s_key);
	}
}

static const char* target_is_unpaired(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return is_paired(player, card) ? "already paired" : NULL;
}

static int soulbond(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_SOULBOND);
	}

	card_instance_t *instance = get_card_instance(player, card);
	if (instance->token_status & STATUS_DYING){
		return 0;
	}

	if (!is_paired(player, card) && specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)){
		if (instance->targets[1].player == player && instance->targets[1].card == card){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.preferred_controller = player;
			td.allowed_controller = player;
			td.illegal_abilities = 0;
			td.special = TARGET_SPECIAL_NOT_ME | TARGET_SPECIAL_EXTRA_FUNCTION;
			td.extra = (int32_t)target_is_unpaired;

			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				pair(player, card, instance->targets[0].player, instance->targets[0].card);
				if( check_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_SOULBOND) ){
					pair(instance->targets[0].player, instance->targets[0].card, player, card);
				}
			}
			instance->number_of_targets = 0;
		}
		else{
			if( ! is_paired(instance->targets[1].player, instance->targets[1].card) ){
				pair(player, card, instance->targets[1].player, instance->targets[1].card);
				if( check_special_flags2(instance->targets[1].player, instance->targets[1].card, SF2_SOULBOND) ){
					pair(instance->targets[1].player, instance->targets[1].card, player, card);
				}
			}
		}
	}

	if( instance->targets[3].player != -1 && ! in_play(instance->targets[3].player, instance->targets[3].card) ){
		remove_special_flags2(player, card, SF2_PAIRED);
		instance->targets[3].player = -1;
		instance->targets[3].card = -1;
	}
	return 0;
}

//--------------- Cards

// WHITE

int card_angel_of_glorys_rise(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int i;
		for(i=0; i<2; i++){
			manipulate_all(player, card, i, TYPE_PERMANENT, 0, SUBTYPE_ZOMBIE, 0, 0, 0, 0, 0, -1, 0, KILL_REMOVE);
		}
		reanimate_all(player, card, player, TYPE_PERMANENT, 0, SUBTYPE_HUMAN, 0, 0, 0, 0, 0, -1, 0, REANIMATE_DEFAULT);
	}
	return 0;
}

int card_angel_of_jubilation(int player, int card, event_t event)
{
  /* Angel of Jubilation	|1|W|W|W
   * Creature - Angel 3/3
   * Flying
   * Other non|Sblack creatures you control get +1/+1.
   * Players can't pay life or sacrifice creatures to cast spells or activate abilities. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player && affected_card != card
	  && !(get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_BLACK))
	  && in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card))
	event_result += 1;

  // Second ability implemented as hacks in can_pay_life(), can_sacrifice_as_cost(), and new_sacrifice().

  return 0;
}

// Angelic Wall --> vanilla

int card_avacyn_angel_of_hope(int player, int card, event_t event)
{
  /* Legendary Creature - Angel 8/8
   * Flying, vigilance
   * ~ and other permanents you control are indestructible. */

  check_legend_rule(player, card, event);

  vigilance(player, card, event);

  if (event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_PERMANENT)
	  && in_play(player, card) && !is_humiliated(player, card))
	indestructible(affected_card_controller, affected_card, event);

  return 0;
}

int card_banishing_stroke(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PERMANENT");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_on_bottom_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_builders_blessing(int player, int card, event_t event){

	if( event == EVENT_TOUGHNESS && affected_card_controller == player ){
		if( ! is_tapped(affected_card_controller, affected_card) ){
			event_result+=2;
		}
	}

	return global_enchantment(player, card, event);
}

int card_call_to_serve(int player, int card, event_t event){

	return generic_aura(player, card, event, player, 1, 2, KEYWORD_FLYING, 0, SUBTYPE_ANGEL, 0, 0);
}

int card_cathars_crusade(int player, int card, event_t event){

	/* Cathars' Crusade	|3|W|W
	 * Enchantment
	 * Whenever a creature enters the battlefield under your control, put a +1/+1 counter on each creature you control. */

	if( specific_cip(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		manipulate_type(player, card, player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}

	return global_enchantment(player, card, event);
}

// Cathedral Sanctifier --> Angel of Mercy

int card_commanders_autority(int player, int card, event_t event){
	/* Commander's Authority	|4|W
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature has "At the beginning of your upkeep, put a 1/1 |Swhite Human creature token onto the battlefield." */

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			generate_token_by_id(player, card, CARD_ID_HUMAN);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_cloudshift(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( player == AI ){
			return card_death_ward(player, card, event);
		}
		else{
			return can_target(&td);
		}
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( player == AI ){
				return card_death_ward(player, card, event);
			}
			else{
				pick_target(&td, "TARGET_CREATURE");
			}
	}
	else if( event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
				if( player == AI ){
					regenerate_target(instance->targets[0].player, instance->targets[0].card);
				}
				blink_effect(player, instance->targets[0].card, 0);
			}
			kill_card(player, card, KILL_DESTROY );
  }
  return 0;
}

int card_cursebreak(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_ENCHANTMENT");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				gain_life(player, 2);
			}
			kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

// Defang (unconfirmed) --> Muzzle

int card_defy_death(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_CREATURE) > 0){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int selected = -1;
			if( player == HUMAN ){
				selected = select_a_card(player, player, 2, 1, 2, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			else{
				selected = select_a_card(player, player, 2, 1, 2, -1, TYPE_CREATURE, 0, SUBTYPE_ANGEL, 0, 0, 0, 0, 0, -1, 0);
				if( selected == -1 ){
					selected = select_a_card(player, player, 2, 1, 2, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				}
			}
			instance->targets[0].player = selected;
			const int *grave = get_grave(player);
			instance->targets[0].card = grave[selected];
	}
	else if(event == EVENT_RESOLVE_SPELL){
			int selected = instance->targets[0].player;
			const int *grave = get_grave(player);
			if( instance->targets[0].card == grave[selected] ){
				int zombo = reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
				if( zombo != -1 && has_subtype(player, zombo, SUBTYPE_ANGEL) ){
					add_1_1_counters(player, zombo, 2);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_devout_chaplain(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_HUMAN;
	td1.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && target_available(player, card, &td1) > 2 ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				return can_target(&td);
			}
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			state_untargettable(player, card, 1);
			int trgs = 0;
			while( trgs < 2 ){
					if( new_pick_target(&td1, "TARGET_PERMANENT", trgs, 0) ){
						state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
						trgs++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<trgs; i++){
				if( instance->targets[i].card != -1 ){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
					if( trgs == 2 ){
						tap_card(instance->targets[i].player, instance->targets[i].card);
					}
				}
			}
			state_untargettable(player, card, 0);
			if( trgs == 2 ){
				tap_card(player, card);
				if( pick_target(&td, "DISENCHANT") ){
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	return 0;
}

static int divine_deflection_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	// instance->targets[1].card -> max damage to prevent

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == player && damage->info_slot > 0 ){
			if( damage->info_slot > instance->targets[0].card ){
				int amount = instance->targets[0].card;
				damage->info_slot-=instance->targets[0].card;
				damage_creature(instance->targets[1].player, instance->targets[1].card, amount, player, card);
			}
			else{
				instance->targets[0].card-=damage->info_slot;
				damage->damage_target_player = instance->targets[1].player;
				damage->damage_target_card = instance->targets[1].card;
			}
			if( instance->targets[0].card < 1 ){
				 kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_divine_deflection(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST && can_target(&td) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card)){
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				instance->info_slot = x_value;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			int legacy = create_legacy_effect(player, card, &divine_deflection_effect);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[0].card = instance->info_slot;
			leg->targets[1] = instance->targets[0];
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_emancipation_angel(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! can_target(&td)  ){
			ai_modifier-=1000;
		}
	}
	if( comes_into_play(player, card, event) && can_target(&td) ){
		pick_target(&td, "TARGET_PERMANENT");
		bounce_permanent(player, instance->targets[0].card);
	}
	return 0;
}

int card_entreat_the_angels(int player, int card, event_t event){
	/* Entreat the Angels	|X|X|W|W|W
	 * Sorcery
	 * Put X 4/4 |Swhite Angel creature tokens with flying onto the battlefield.
	 * Miracle |X|W|W */

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( played_for_free(player, card) || is_token(player, card) ){
				charge_mana(player, COLOR_COLORLESS, -1);
				if( spell_fizzled != 1 ){
					instance->info_slot = x_value;
				}
			}
			else{
				int amount = charge_mana_for_double_x(player, COLOR_COLORLESS);
				instance->info_slot = amount/2;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
		   int tokens = instance->info_slot;
		   if( tokens > 0 ){
			  generate_tokens_by_id(player, card, CARD_ID_ANGEL, tokens);
		   }
		   kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Farbog Explorer --> vanilla

int card_goldnight_commander(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, 0);
	}

  return 0;
}

int card_goldnight_redeemer(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		gain_life(player, 2*(count_permanents_by_type(player, TYPE_CREATURE)-1));
	}
	return 0;
}

int card_herald_of_war(int player, int card, event_t event){

	// Whenever ~ attacks, put a +1/+1 counter on it.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		add_1_1_counter(player, card);
	}

	// Angel spells and Human spells you cast cost |1 less to cast for each +1/+1 counter on ~.
	if( event == EVENT_MODIFY_COST_GLOBAL ){
		if( has_subtype(affected_card_controller, affected_card, SUBTYPE_ANGEL) ||
			has_subtype(affected_card_controller, affected_card, SUBTYPE_HUMAN)
		  ){
			COST_COLORLESS-=count_1_1_counters(player, card);
		}
	}

	return 0;
}

int card_holy_justicar(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_ZOMBIE) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 1, 0, &td, "TARGET_CREATURE");
}

static int leap_of_faith_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	modify_pt_and_abilities(instance->targets[0].player, instance->targets[0].card, event, 0, 0, KEYWORD_FLYING);

		if( event == EVENT_PREVENT_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_target_card == instance->targets[0].card && damage->damage_target_player == instance->targets[0].player &&
					damage->info_slot > 0
				  ){
					damage->info_slot=0;
				}
			}
		}
	return 0;
}

int card_leap_of_faith(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
				create_targetted_legacy_effect(player, card, &leap_of_faith_effect, instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_midnight_duelist(int player, int card, event_t event){
	protection_from_subtype(player, card, event, SUBTYPE_VAMPIRE);
	return 0;
}

int card_midvast_protector(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card,  instance->targets[0].player, instance->targets[0].card, 0, 0,
									select_a_protection(player) | KEYWORD_RECALC_SET_COLOR, 0);
		}
	}
	return 0;
}

int card_moonlight_geist(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 1) && current_phase == PHASE_AFTER_BLOCKING &&
			instance->blocking < 255 ){
			return 1;
		}
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 1);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->blocking < 255 ){
				create_targetted_legacy_effect(player, instance->parent_card, &no_combat_damage_this_turn,
												1-player, instance->blocking);
			}
	}
	return 0;
}

int card_moorland_inquisitor(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 2, 0, 0, 0, 0, 1, 0, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_nearheath_pilgrim(int player, int card, event_t event){

	if( is_paired(player, card) ){
		lifelink(player, card, event);
		boost_paired_creature(player, card, event, 0, 0, 0, SP_KEYWORD_LIFELINK);
	}

	return soulbond(player, card, event);
}

int card_restoration_angel(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_ANGEL;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	card_instance_t *instance = get_card_instance(player, card);

	if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player))){
		instance->number_of_targets = 0;
		if (can_target(&td) && pick_next_target_noload(&td, "Select target non-Angel creature you control.")){
			blink_effect(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}
	return flash(player, card, event);
}

int card_riders_of_gavony(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	vigilance(player, card, event);

	if( comes_into_play(player, card, event) ){
		instance->targets[4].card = select_a_subtype(player, card);
	}

	if(event == EVENT_BLOCK_LEGALITY ){
		if( player == attacking_card_controller && has_creature_type(attacking_card_controller, attacking_card, SUBTYPE_HUMAN) ){
			if( has_creature_type(affected_card_controller, affected_card, instance->targets[4].card) ){
				event_result = 1;
			}
		}
	}

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

		if( damage_card != source->internal_card_id || source->info_slot <= 0 ){
			return 0;
		}

		if( source->damage_target_player == player && source->damage_target_card != -1 ){
			if( in_play(source->damage_source_player, source->damage_source_card) &&
				has_subtype(source->damage_source_player, source->damage_source_card, instance->targets[4].card)
			  ){
				source->info_slot = 0;
			}
		}
	}
	return 0;
}

int card_righteous_blow(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	if( current_turn == player ){
		td.required_state = TARGET_STATE_BLOCKING;
	}
	else{
		td.required_state = TARGET_STATE_ATTACKING;
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
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Seraph of Dawn --> Child of Night

int card_silverblade_paladin(int player, int card, event_t event){

	if( is_paired(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_DOUBLE_STRIKE);
		boost_paired_creature(player, card, event, 0, 0, KEYWORD_DOUBLE_STRIKE, 0);
	}

	return soulbond(player, card, event);
}

int card_spectral_gateguards(int player, int card, event_t event){

	if( is_paired(player, card) ){
		vigilance(player, card, event);
		boost_paired_creature(player, card, event, 0, 0, 0, SP_KEYWORD_VIGILANCE);
	}

	return soulbond(player, card, event);
}

// Thraben Stalwart --> Serra Angel

int card_terminus(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_PUT_ON_BOTTOM);
		manipulate_all(player, card, 1-player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_PUT_ON_BOTTOM);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_voice_of_the_provinces(int player, int card, event_t event){
	/* Voice of the Provinces	|4|W|W
	 * Creature - Angel 3/3
	 * Flying
	 * When ~ enters the battlefield, put a 1/1 |Swhite Human creature token onto the battlefield. */

	if( comes_into_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_HUMAN);
	}

	return 0;
}

int card_zealous_strike(int player, int card, event_t event){
	/* Zealous Strike	|1|W
	 * Instant
	 * Target creature gets +2/+2 and gains first strike until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 2, 2, KEYWORD_FIRST_STRIKE, 0);
}

//BLUE
int card_alchemists_apprentice(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_ACTIVATION){
		draw_cards(player, 1);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_amass_the_components(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_HAND;
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			draw_cards(player, 3);
			if( can_target(&td) ){
				pick_target(&td, "TARGET_CARD");
				put_on_bottom_of_deck(instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_arcane_melee(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL ){
		if( is_what(affected_card_controller, affected_card, TYPE_SORCERY | TYPE_INSTANT | TYPE_INTERRUPT) &&
			! is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		  ){
			COST_COLORLESS-=2;
		}
	}

	return global_enchantment(player, card, event);
}

int card_captain_of_the_mists(int player, int card, event_t event)
{
  /* Captain of the Mists	|2|U
   * Creature - Human Wizard 2/3
   * Whenever another Human enters the battlefield under your control, untap ~.
   * |1|U, |T: You may tap or untap target permanent. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "");
	  test.subtype = SUBTYPE_HUMAN;
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		untap_card(player, card);
	}

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.preferred_controller = ANYBODY;

  int rval = generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_XU(1,1), 0, &td, "TARGET_PERMANENT");

  if (event == EVENT_ACTIVATE && player == AI && cancel != 1)
	ai_modifier_twiddle(player, card, 0);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	twiddle(player, card, 0);

  return rval;
}

int card_crippling_chill(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
				draw_cards(player, 1);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_deadeye_navigator(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_paired(player, card) ){
		if( event == EVENT_CAN_ACTIVATE ){
			return has_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0);
		}
		else if( event == EVENT_ACTIVATE ){
				charge_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0);
				if( spell_fizzled != 1 ){
					int choice = 0;
					if( is_target(instance->targets[3].player, instance->targets[3].card, 1-player) ){
						choice = 1;
					}
					if( player != AI ){
						choice = do_dialog(player, player, card, -1, -1, " Blink this creature\n Blink paired creature", 0);
					}
					instance->info_slot = choice+1;
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				if( instance->info_slot == 1 ){
					blink_effect(player, instance->parent_card, 0);
				}
				else{
					blink_effect(instance->targets[3].player, instance->targets[3].card, 0);
				}
		}
	}

	return soulbond(player, card, event);
}

int card_devastation_tide(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		new_manipulate_all(player, card, player, &this_test, ACT_BOUNCE);
		new_manipulate_all(player, card, 1-player, &this_test, ACT_BOUNCE);
		this_test.type = TYPE_LAND;
		this_test.type_flag = 1;
		new_manipulate_all(player, card, player, &this_test, ACT_BOUNCE);
		new_manipulate_all(player, card, 1-player, &this_test, ACT_BOUNCE);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dreadwaters(int player, int card, event_t event){

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
		if( valid_target(&td)  ){
			mill(instance->targets[0].player, count_permanents_by_type(player, TYPE_LAND));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_elgaud_shieldmate(int player, int card, event_t event){

	if( is_paired(player, card) ){
		hexproof(player, card, event);
		boost_paired_creature(player, card, event, 0, 0, 0, SP_KEYWORD_HEXPROOF);
	}

	return soulbond(player, card, event);
}

int card_favourable_winds(int player, int card, event_t event){

	if( affected_card_controller == player && check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING) ){
		modify_pt_and_abilities(affected_card_controller, affected_card, event, 1, 1, 0);
	}

	return global_enchantment(player, card, event);
}

int card_fettergeist(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		int mana = count_permanents_by_type(player, TYPE_CREATURE) - 1;
		if( has_mana(player, COLOR_COLORLESS, mana) ){
			charge_mana(player, COLOR_COLORLESS, mana);
			if( spell_fizzled != 1 ){
				kill = 0;
			}
		}
		if( kill == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_fleeting_distraction(int player, int card, event_t event){

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
			if( validate_target(player, card, &td, 0) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, 0);
				draw_cards(player, 1);
			}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_galvanic_alchemist(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_paired(player, card) ){
		if( event == EVENT_CAN_ACTIVATE ){
			return has_mana_for_activated_ability(player, card, 2, 0, 1, 0, 0, 0);
		}
		else if( event == EVENT_ACTIVATE ){
				charge_mana_for_activated_ability(player, card, 2, 0, 1, 0, 0, 0);
				if( spell_fizzled != 1 ){
					int choice = 0;
					if( is_tapped(instance->targets[3].player, instance->targets[3].card) ){
						choice = 1;
					}
					if( player != AI ){
						choice = do_dialog(player, player, card, -1, -1, " Untap this creature\n Untap paired creature", 0);
					}
					instance->info_slot = choice+1;
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				if( instance->info_slot == 1 ){
					untap_card(player, instance->parent_card);
				}
				else{
					untap_card(instance->targets[3].player, instance->targets[3].card);
				}
		}
	}

	return soulbond(player, card, event);
}

int card_geist_snatch(int player, int card, event_t event){
	/* Geist Snatch	|2|U|U
	 * Instant
	 * Counter target creature spell. Put a 1/1 |Sblue Spirit creature token with flying onto the battlefield. */

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_BLUE;
		generate_token(&token);
		return card_remove_soul(player, card, event);
	}
	else{
		return card_remove_soul(player, card, event);
	}

	return 0;
}

int card_ghostform(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->targets[1] = instance->targets[0];
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
			if( can_target(&td) ){
				int choice = 0;
				if( player == HUMAN ){
					choice = do_dialog(player, player, card, -1, -1, " Continue\n Stop", 0);
				}
				if( choice == 0 ){
					td.allow_cancel = 0;
					pick_target(&td, "TARGET_CREATURE");
				}
			}
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				pump_ability_until_eot(player, card, player, instance->targets[i].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ghostly_flicker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_type = TYPE_ENCHANTMENT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PERMANENT") ){
			instance->targets[1] = instance->targets[0];
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
			if( can_target(&td) ){
				int choice = 0;
				if( player == HUMAN ){
					choice = do_dialog(player, player, card, -1, -1, " Continue\n Stop", 0);
				}
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
			if( validate_target(player, card, &td, i) ){
				blink_effect(instance->targets[i].player, instance->targets[i].card, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ghostly_touch(int player, int card, event_t event)
{
	// Enchanted creature has "Whenever this creature attacks, you may tap or untap target permanent."
	if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS){
		card_instance_t* instance = in_play(player, card);

		if (instance && instance->damage_target_player >= 0 &&
			!is_humiliated(instance->damage_target_player, instance->damage_target_card) &&
			declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), instance->damage_target_player, instance->damage_target_card)
		   ){
			int t_player = instance->damage_target_player;
			int t_card = instance->damage_target_card;

			int ai_choice = 0;
			target_definition_t td;
			default_target_definition(t_player, t_card, &td, TYPE_PERMANENT);

			if( player == AI ){
				target_definition_t td1;
				default_target_definition(t_player, t_card, &td1, TYPE_PERMANENT);
				td1.illegal_state = TARGET_STATE_TAPPED;

				if (!can_target(&td1)){
					td.preferred_controller = player;
					td.illegal_state = TARGET_STATE_TAPPED;
					ai_choice = 1;
				}
			}

			instance->number_of_targets = 0;
			if (can_target(&td) && pick_next_target_arbitrary(&td, "TARGET_PERMANENT", player, card)){
				int choice = do_dialog(player, player, card, -1, -1, " Tap\n Untap", ai_choice);
				if( choice == 0 ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
				else{
					untap_card(instance->targets[0].player, instance->targets[0].card);
				}
			}
		}
	}

	// Enchant creature
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

// Gryff Vanguard (name unconfirmed) --> Wall of Blossoms

int card_havengul_skaab(int player, int card, event_t event)
{
  // Whenever ~ attacks, return another creature you control to its owner's hand.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;
	  td.allowed_controller = player;
	  td.illegal_abilities = 0;
	  td.allow_cancel = 0;
	  td.special = TARGET_SPECIAL_NOT_ME;

	  if (can_target(&td) && pick_target(&td, "TARGET_ANOTHER_CREATURE_CONTROL"))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

  return 0;
}

static int effect_infinite_reflection(int player, int card, event_t event)
{
  if (event == EVENT_CHANGE_TYPE
	  && !(land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS)
	  && check_special_flags2(affected_card_controller, affected_card, SF2_INFINITE_REFLECTION))
	{
	  card_instance_t* instance = get_card_instance(affected_card_controller, affected_card);
	  if (instance->targets[12].card != -1)
		event_result = instance->targets[12].card;
	}

  return 0;
}
int card_infinite_reflection(int player, int card, event_t event)
{
  /* Infinite Reflection	|5|U
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield attached to a creature, each other nontoken creature you control becomes a copy of that creature.
   * Nontoken creatures you control enter the battlefield as a copy of enchanted creature. */

  if (comes_into_play(player, card, event))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->damage_target_card < 0 || !in_play(instance->damage_target_player, instance->damage_target_card))
		return 0;

	  card_instance_t* inst;
	  int c;
	  int last = -1;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if ((inst = in_play(player, c)) && is_what(player, c, TYPE_CREATURE) && !is_token(player, c)
			&& !(instance->damage_target_card == c && instance->damage_target_player == player))
		  {
			set_special_flags2(player, c, SF2_INFINITE_REFLECTION);
			cloning(player, c, instance->damage_target_player, instance->damage_target_card);
			last = c;
		  }

	  int leg = create_legacy_effect(player, card, effect_infinite_reflection);
	  if (leg != -1)
		get_card_instance(player, leg)->token_status |= STATUS_INVISIBLE_FX;

	  if (last != -1)
		{
		  recalculate_all_cards_in_play();
		  int singularity = 0;
		  if ((is_legendary(player, last) || (singularity = check_battlefield_for_id(ANYBODY, CARD_ID_LEYLINE_OF_SINGULARITY)))
			  && !check_battlefield_for_id(ANYBODY, CARD_ID_MIRROR_GALLERY))
			for (c = 0; c < active_cards_count[player]; ++c)
			  if (in_play(player, c) && is_what(player, c, TYPE_CREATURE)
				  && (is_legendary(player, c)
					  || (singularity && !is_what(player, c, TYPE_LAND))))
				true_verify_legend_rule(player, c, get_id(player, c));
		}
	}

  card_instance_t* instance;
  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE,F1_NO_TOKEN, 0,0, 0,0, 0,0, -1,0)
	  && (instance = in_play(player, card)) && instance->damage_target_card >= 0 && in_play(instance->damage_target_player, instance->damage_target_card))
	{
	  int p = instance->targets[1].player, c = instance->targets[1].card;
	  set_special_flags2(p, c, SF2_INFINITE_REFLECTION);
	  cloning(p, c, instance->damage_target_player, instance->damage_target_card);
	  get_card_instance(p, c)->regen_status |= KEYWORD_RECALC_ALL;
	  get_abilities(p, c, EVENT_CHANGE_TYPE, -1);
	  get_abilities(p, c, EVENT_SET_COLOR, -1);
	  get_abilities(p, c, EVENT_POWER, -1);
	  get_abilities(p, c, EVENT_TOUGHNESS, -1);
	}

  return vanilla_aura(player, card, event, player);
}

int card_into_the_void(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->targets[1] = instance->targets[0];
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
			if( can_target(&td) ){
				int choice = 0;
				if( player == HUMAN ){
					choice = do_dialog(player, player, card, -1, -1, " Continue\n Stop", 0);
				}
				if( choice == 0 ){
					td.allow_cancel = 0;
					pick_target(&td, "TARGET_CREATURE");
				}
			}
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				bounce_permanent(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_latch_seeker(int player, int card, event_t event){

	/* Blighted Agent	|1|U
	 * Creature - Human Rogue 1/1
	 * Infect
	 * ~ is unblockable. */

	/* Covert Operative	|4|U
	 * Creature - Human Wizard 3/2
	 * ~ is unblockable. */

	/* Jhessian Infiltrator	|G|U
	 * Creature - Human Rogue 2/2
	 * ~ is unblockable. */

	/* Latch Seeker	|1|U|U
	 * Creature - Spirit 3/1
	 * ~ is unblockable. */

	/* Metathran Soldier	|1|U
	 * Creature - Metathran Soldier 1/1
	 * ~ is unblockable. */

	/* Phantom Warrior	|1|U|U
	 * Creature - Illusion Warrior 2/2
	 * ~ is unblockable. */

	/* Plasma Elemental	|5|U
	 * Creature - Elemental 4/1
	 * ~ is unblockable. */

	/* Talas Warrior	|1|U|U
	 * Creature - Human Pirate Warrior 2/2
	 * ~ is unblockable. */

	/* Tidal Kraken	|5|U|U|U
	 * Creature - Kraken 6/6
	 * ~ is unblockable. */

	/* Triton Shorestalker	|U
	 * Creature - Merfolk Rogue 1/1
	 * ~ can't be blocked. */

	unblockable(player, card, event);

	return 0;
}

int card_lone_revenant(int player, int card, event_t event){

	hexproof(player, card, event);

	if (creature_count[player] == 1 && has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		impulse_effect(player, 4, 0);
	}

	return 0;
}

int card_lunar_mystic(int player, int card, event_t event){

	if( has_mana(player, COLOR_COLORLESS, 1) &&
		specific_spell_played(player, card, event, player, 1+player, TYPE_INSTANT | TYPE_INTERRUPT, 2, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			draw_cards(player, 1);
		}
	}
	return 0;
}

int card_mass_appeal(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_HUMAN));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// mist raven --> Man'o'War

// Mysthollow Griffin --> skipped

int card_nephalia_smugglers(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 0) &&
			target_available(player, card, &td) > 1
		  ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 0);
		if( spell_fizzled != 1 ){
			state_untargettable(player, card, 1);
			if( can_target(&td) ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					tap_card(player, card);
					state_untargettable(player, card, 0);
				}
				else{
					state_untargettable(player, card, 0);
				}
			}
			state_untargettable(player, card, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			blink_effect(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	return 0;
}

int card_outwit(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			card_instance_t *instance = get_card_instance(card_on_stack_controller, card_on_stack);
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				if( instance->targets[i].player > -1 && instance->targets[i].card == -1 ){
					return result;
				}
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			card_instance_t *instance = get_card_instance(player, card);
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			return card_counterspell(player, card, event);
	}
	else{
		return card_counterspell(player, card, event);
	}

	return 0;
}

int card_peel_from_reality(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = 1-player;
	td1.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->targets[1] = instance->targets[0];
			pick_target(&td1, "TARGET_CREATURE");
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 1) ){
			bounce_permanent(instance->targets[1].player, instance->targets[1].card);
		}
		if( validate_target(player, card, &td1, 0) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rotcrown_ghoul(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t* instance = get_card_instance(player, card);

		if( can_target(&td) ){
			if( new_pick_target(&td, "TARGET_PLAYER", 1, 0) ){
				mill(instance->targets[1].player, 5);
			}
		}
	}

	return 0;
}

// Scrapskin Drake --> Cloud Sprite

int card_card_second_guess(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		if( get_storm_count() == 2 ){
			return card_counterspell(player, card, event);
		}
	}
	else{
		return card_counterspell(player, card, event);
	}

	return 0;
}

int card_spectral_prison(int player, int card, event_t event)
{
  if (kill_attachment_when_creature_is_targeted_by_spell(player, card, event, KILL_SACRIFICE))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);
  if (instance->damage_target_card > -1 && in_play(player, card))
	does_not_untap(instance->damage_target_player, instance->damage_target_card, event);

  return vanilla_aura(player, card, event, 1-player);
}

int card_spirit_away(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->targets[0].player != -1 ){
		modify_pt_and_abilities(instance->targets[0].player, instance->targets[0].card, event, 2, 2, KEYWORD_FLYING);
	}
	return card_control_magic(player, card, event);
}

int card_stern_mentor(int player, int card, event_t event){
	// name unconfirmed

	card_instance_t *instance = get_card_instance(player, card);

	if( is_paired(player, card) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		target_definition_t td1;
		default_target_definition(instance->targets[3].player, instance->targets[3].card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_PLAYERS;

		if( event == EVENT_CAN_ACTIVATE ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				if( ! is_tapped(player, card) && ! is_sick(player, card) && can_target(&td) ){
					return 1;
				}
				if( ! is_tapped(instance->targets[3].player, instance->targets[3].card) && ! is_sick(instance->targets[3].player, instance->targets[3].card)
					&& can_target(&td1)
				  ){
					return 1;
				}
			}
		}
		else if( event == EVENT_ACTIVATE ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					int choice = 0;
					if( ! is_tapped(player, card) && ! is_sick(player, card) && can_target(&td) ){
						if( ! is_tapped(instance->targets[3].player, instance->targets[3].card) &&
							! is_sick(instance->targets[3].player, instance->targets[3].card) && can_target(&td1)
						  ){
							choice = do_dialog(player, player, card, -1, -1, " Tap Stern Mentor\n Tap paired creatur\n Cancel", 0);
						}
					}
					else{
						choice = 1;
					}
					if( choice == 0 ){
						if( pick_target(&td, "TARGET_PLAYER") ){
							instance->number_of_targets = 1;
							instance->info_slot = 1;
							tap_card(player, card);
						}
					}
					if( choice == 1 ){
						if( pick_target(&td1, "TARGET_PLAYER") ){
							instance->number_of_targets = 1;
							instance->info_slot = 2;
							tap_card(instance->targets[3].player, instance->targets[3].card);
						}
					}
					if( choice == 2 ){
						spell_fizzled = 1;
					}
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				if( instance->info_slot == 1 && validate_target(player, card, &td, 0) ){
					mill(instance->targets[0].player, 2);
				}
				else if( instance->info_slot == 2 && validate_target(instance->targets[3].player, instance->targets[3].card, &td1, 0) ){
						card_instance_t *bonded = get_card_instance(instance->targets[3].player, instance->targets[3].card);
						mill(bonded->targets[0].player, 2);
				}

		}
	}

	return soulbond(player, card, event);
}

int card_stolen_goods(int player, int card, event_t event){

	/* Stolen Goods	|3|U
	 * Sorcery
	 * Target opponent exiles cards from the top of his or her library until he or she exiles a nonland card. Until end of turn, you may cast that card without
	 * paying its mana cost. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td)  ){
			int t_player = instance->targets[0].player;
			int *deck = deck_ptr[t_player];
			int removed[count_deck(t_player)];
			int z=0;
			while( deck[0] != -1 ){
					removed[z] = deck[0];
					rfg_top_card_of_deck(t_player);
					if( !(cards_data[removed[z]].type & TYPE_LAND) ){
						show_deck(HUMAN, removed, z+1, "These cards were revealed", 0, 0x7375B0 );
						create_may_play_card_from_exile_effect(player, card, t_player, cards_data[removed[z]].id, MPCFE_FOR_FREE|MPCFE_UNTIL_EOT);
						break;
					}
					z++;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_tamiyo_the_moon_sage(int player, int card, event_t event){

	/* Planeswalker - Tamiyo (4)
	 * +1: Tap target permanent. It doesn't untap during its controller's next untap step.
	 * -2: Draw a card for each tapped creature target player controls.
	 * -8: You get an emblem with "You have no maximum hand size" and "Whenever a card is put into your graveyard from anywhere, you may return it to your
	 * hand." */

	if (IS_ACTIVATING(event)){

		card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_PLAYERS;

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_TAPPED;

		int priority_draw_target[2] = {	check_battlefield_for_special_card(player, card, 0, CBFSC_GET_COUNT, &this_test),
										check_battlefield_for_special_card(player, card, 1, CBFSC_GET_COUNT, &this_test)
		};

		enum{
			CHOICE_FROST_TITAN = 1,
			CHOICE_DRAW,
			CHOICE_EMBLEM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Lock permanent", can_target(&td), 10, 1,
						"Draw cards = tapped creatures", can_target(&td2), 3*MAX(priority_draw_target[0], priority_draw_target[1]), -2,
						"Emblem", 1, 15, -8);

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
				case CHOICE_FROST_TITAN:
					pick_target(&td, "TARGET_PERMANENT");
					break;

				case CHOICE_DRAW:
					pick_target(&td2, "TARGET_PLAYER");
					break;

			  case CHOICE_EMBLEM:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_FROST_TITAN:
				{
					if ( valid_target(&td)){
						effect_frost_titan(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
					}
				}
				break;

			case CHOICE_DRAW:
				{
					if ( valid_target(&td2)){
						draw_cards(player, check_battlefield_for_special_card(player, card, instance->targets[0].player, CBFSC_GET_COUNT, &this_test));
					}
				}
				break;

			case CHOICE_EMBLEM:
			  generate_token_by_id(player, card, CARD_ID_TAMIYOS_EMBLEM);
			  break;
		  }
	}

	return planeswalker(player, card, event, 4);
}

int card_tamiyos_emblem(int player, int card, event_t event)
{
  /* Tamiyo's Emblem	""
   * Emblem
   * You have no maximum hand size.
   * Whenever a card is put into your graveyard from anywhere, you may return it to your hand. */

  if (event == EVENT_MAX_HAND_SIZE && current_turn == player)
	event_result += 99;

  // From library
  enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
  if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == player)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_AI(player);

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  if (IS_AI(player))	// Return all cards.  Hopefully there's no Black Vise-like cards on the bf.
			{
			  // Go in reverse order, to minimize repositioning.
			  int i;
			  for (i = num_cards_milled - 1; i >= 0; --i)
				{
				  int pos = find_in_graveyard_by_source(player, cards_milled[i].source, cards_milled[i].position);
				  if (pos != -1)
					from_grave_to_hand(player, pos, TUTOR_HAND);
				  cards_milled[i].position = -1;	// No longer in graveyard, so keep any other triggers from looking
				}
			}
		  else
			{
			  int iids[500], indices[500];
			  while (1)
				{
				  int i, num = 0;
				  for (i = 0; i < num_cards_milled; ++i)
					{
					  if (cards_milled[i].position != -1)	// update position
						cards_milled[i].position = find_in_graveyard_by_source(player, cards_milled[i].source, cards_milled[i].position);
					  if (cards_milled[i].position != -1)	// still in graveyard?
						{
						  iids[num] = cards_milled[i].internal_card_id;
						  indices[num] = i;
						  ++num;
						}
					}

				  if (num == 0)
					break;

				  int chosen = show_deck(player, iids, num, "Tamiyo's Emblem: choose cards to return to hand", 0, 0x7375b0);	// DIALOGBUTTONS[2] = "Done"
				  if (chosen == -1)
					break;

				  from_grave_to_hand(player, cards_milled[indices[chosen]].position, TUTOR_HAND);
				  cards_milled[indices[chosen]].position = -1;	// No longer in graveyard, so don't look for it in further iterations
				}
			}
		}
	}

  // From anywhere else
  if (when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(player, card, event, player, RESOLVE_TRIGGER_AI(player), NULL)
	  && gy_from_anywhere_pos != -1)
	{
	  int pos = find_in_graveyard_by_source(player, gy_from_anywhere_source, gy_from_anywhere_pos);
	  if (pos != -1)	// already removed, but hadn't been recorded
		from_grave_to_hand(player, pos, TUTOR_HAND);
	  gy_from_anywhere_pos = -1;	// No longer in graveyard, so keep any other triggers from looking
	}

  return 0;
}

int card_tandem_lookout(int player, int card, event_t event){

	/* Tandem Lookout	|2|U
	 * Creature - Human Scout 2/1
	 * Soulbond
	 * As long as ~ is paired with another creature, each of those creatures has "Whenever this creature deals damage to an opponent, draw a card." */


	if( is_paired(player, card) ){
		if (event != EVENT_DEAL_DAMAGE && trigger_condition != TRIGGER_DEAL_DAMAGE){
			return 0;
		}
		card_instance_t* instance = get_card_instance(player, card);
		card_instance_t* damage = damage_being_dealt(event);
		if (damage && damage->damage_target_card == -1 && damage->damage_target_player == 1-player && !damage_is_to_planeswalker(damage)){
			int p_player = instance->targets[3].player;
			int p_card = instance->targets[3].card;
			if( (damage->damage_source_player == player && damage->damage_source_card == card) ||
				(damage->damage_source_player == p_player && damage->damage_source_card == p_card)
			  ){
				if( instance->targets[6].player < 0 ){
					instance->targets[6].player = 0;
				}
				instance->targets[6].player++;
			}
		}
		if( instance->targets[6].player > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE &&
			player == reason_for_trigger_controller && affect_me(player, card )
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					draw_cards(player, instance->targets[6].player);
					instance->targets[6].player = 0;
			}
		}
	}

	return soulbond(player, card, event);
}

int card_temporal_mastery(int player, int card, event_t event ){
	/* Temporal Mastery	|5|U|U
	 * Sorcery
	 * Take an extra turn after this one. Exile ~.
	 * Miracle |1|U */

	if( event == EVENT_RESOLVE_SPELL ){
		time_walk_effect(player, card);
		kill_card(player, card, KILL_REMOVE);
	}

	return basic_spell(player, card, event);
}

int card_vanishment(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PERMANENT");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_wingcrafter(int player, int card, event_t event){

	if( is_paired(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FLYING);
		boost_paired_creature(player, card, event, 0, 0, KEYWORD_FLYING, 0);
	}

	return soulbond(player, card, event);
}

// BLACK
int card_appetite_for_brains(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		instance->targets[0].player = 1-player;
		return valid_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = 1-player;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			this_test.cmc = 3;
			this_test.cmc_flag = F5_CMC_GREATER_THAN_VALUE;

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			this_definition.effect = EC_RFG;
			new_effect_coercion(&this_definition, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int blood_artist_effect(int player, int card, event_t event){

	//In case something is copying Pawn of Ulamog and then reverts to its original form.
	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( get_id(instance->damage_target_player, instance->damage_target_card) != CARD_ID_BLOOD_ARTIST ){
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		card_instance_t *instance = get_card_instance(player, card);
		if( in_play(affected_card_controller, affected_card) ){
			int kill_code = get_card_instance(affected_card_controller, affected_card)->kill_code;
			if( kill_code ){
				if( instance->damage_target_player > -1 ){
					int p = instance->damage_target_player;
					int c = instance->damage_target_card;
					if( affect_me(p, c) ){
						//Will trigger also if this is not a creature when it goes into the graveyard;
						if( ! is_what(p, c, TYPE_CREATURE) ){
							count_for_gfp_ability(player, card, event, player, TYPE_PERMANENT, NULL);
						}
						instance->damage_target_player = instance->damage_target_card = -1;
						remove_status(player, card, STATUS_INVISIBLE_FX);
					}
				}
				count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
			}
		}
	}

	if( ! check_status(player, card, STATUS_INVISIBLE_FX) ){
		int amount = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
		if( amount ){
			life_sucking(player, card, amount);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	//Reset counts if the legacy is still invisible
	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && event == EVENT_END_TRIGGER ){
		get_card_instance(player, card)->targets[11].player = 0;
	}

	return 0;
}

int card_blood_artist(int player, int card, event_t event){

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		int found = 0;
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_EFFECT) ){
				card_instance_t *inst = get_card_instance(player, c);
				if( inst->info_slot == (int)blood_artist_effect ){
					if( (inst->damage_target_player == player && inst->damage_target_card == card) ||
						(inst->targets[0].player == player && inst->targets[0].card == card) )
					{
						found = 1;
						break;
					}
				}
			}
		}
		if( ! found ){
			int legacy = create_targetted_legacy_effect(player, card, &blood_artist_effect, player, card);
			card_instance_t *inst = get_card_instance(player, legacy);
			inst->targets[0].player = player;
			inst->targets[0].card = card;
			inst->number_of_targets = 1;
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
	}

	if( get_card_instance(player, card)->kill_code <= 0 ){
		int amount = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
		if( amount ){
			life_sucking(player, card, amount);
		}
	}

	return 0;
}


// Bloodflow Connoisseur --> Scarland Thrinax

// Butcher Ghoul --> Young Wolf

int card_corpse_traders(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			new_effect_coercion(&this_definition, &this_test);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_CREATURE, 2, 1, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_dark_imposter(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		int can_activate = 0;

		if( has_mana_for_activated_ability(player, card, 4, 2, 0, 0, 0, 0) && can_target(&td) ){
			can_activate = 1;
		}
		if( can_activate == 0 ){
			int i;
			for(i=2; i<instance->targets[1].player; i++){
				int ability = instance->targets[i].card;
				int (*ptFunction)(int, int, event_t) = (void*)cards_data[ability].code_pointer;
				if( ptFunction(player, card, EVENT_CAN_ACTIVATE ) ){
					can_activate = 1;
					break;
				}
			}
		}
		return can_activate;
	}
	else if( event == EVENT_ACTIVATE){
			char buffer[1500];
			int pos = scnprintf(buffer, 1500, " Cancel\n" );
			int my_ability = 0;
			if( has_mana_for_activated_ability(player, card, 4, 2, 0, 0, 0, 0) && can_target(&td) ){
				my_ability = 1;
				pos += scnprintf(buffer + pos, 1500-pos, " Kill & steal ability\n" );
			}
			int act_count = 0;
			int i;
			for(i=2; i<instance->targets[1].player; i++){
				int ability = instance->targets[i].card;
				int (*ptFunction)(int, int, event_t) = (void*)cards_data[ability].code_pointer;
				if( ptFunction(player, card, EVENT_CAN_ACTIVATE) ){
					act_count++;
					card_ptr_t* c = cards_ptr[ cards_data[ability].id ];
					pos += scnprintf(buffer + pos, 1500-pos, " %s\n", c->name );
				}
			}
			int choice = 1;
			if( act_count+my_ability > 1 && player != AI ){
				choice = do_dialog(player, player, card, -1, -1, buffer, 1);
			}
			if( my_ability == 0 ){
				choice++;
			}
			if( choice != 0 && spell_fizzled != 1 ){
				if( choice == 1 ){
					charge_mana_for_activated_ability(player, card, 4, 2, 0, 0, 0, 0);
					if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						instance->targets[1].card = 0;
					}
				}
				else{
					// pay the costs for the selected card
					int ability = instance->targets[choice].card;
					int (*ptFunction)(int, int, event_t) = (void*)cards_data[ability].code_pointer;
					ptFunction(player, card, EVENT_ACTIVATE);
					if( spell_fizzled != 1 ){
						instance->targets[1].card = ability;
					}
				}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[1].card > 0 ){
				int (*ptFunction)(int, int, event_t) = (void*)cards_data[instance->targets[1].card].code_pointer;
				ptFunction(player, card, EVENT_RESOLVE_ACTIVATION);
			}
			else{
				if( valid_target(&td) ){
					card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
					card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
					int this = target->internal_card_id;
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
					add_1_1_counter(player, instance->parent_card);
					if( (cards_data[this].extra_ability & EA_ACT_ABILITY) || (cards_data[this].extra_ability & EA_MANA_SOURCE) ){
						if( parent->targets[1].player < 2 ){
							parent->targets[1].player = 2;
						}
						int pos = parent->targets[1].player;
						if( pos < 9 ){
							parent->targets[pos].card = this;
							parent->targets[1].player++;
						}
					}
				}
			}
	}

	return 0;
}

int card_demonic_taskmaster(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		state_untargettable(player, card, 1);
		sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		state_untargettable(player, card, 0);
	}

	return 0;
}

int card_demonic_rising(int player, int card, event_t event){
	/* Demonic Rising	|3|B|B
	 * Enchantment
	 * At the beginning of your end step, if you control exactly one creature, put a 5/5 |Sblack Demon creature token with flying onto the battlefield. */

	if( current_turn == player && creature_count[player] == 1 &&
		eot_trigger(player, card, event)
	  ){
		generate_token_by_id(player, card, CARD_ID_DEMON);
	}
	return global_enchantment(player, card, event);
}

int card_demonlord_of_ashmouth(int player, int card, event_t event)
{
  undying(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select another creature to sacrifice.");
	  test.not_me = 1;

	  if (!new_sacrifice(player, card, player, 0, &test))
		kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}

int card_death_wind(int player, int card, event_t event){
	return generic_x_spell(player, card, event, TARGET_ZONE_IN_PLAY, 0, 8192);
}

int card_descent_into_madness(int player, int card, event_t event){

	/* At the beginning of your upkeep, put a despair counter on ~, then each player exiles X permanents he or she controls and/or cards from his or her hand,
	 * where X is the number of despair counters on ~. */

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_DESPAIR);
		int amount = count_counters(player, card, COUNTER_DESPAIR);
		int i;
		for(i=0; i<2; i++){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_PERMANENT);
			td.allowed_controller = i;
			td.preferred_controller = i;
			td.who_chooses = i;
			td.illegal_abilities = 0;

			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_ANY);
			td1.zone = TARGET_ZONE_HAND;
			td1.allowed_controller = i;
			td1.preferred_controller = i;
			td1.who_chooses = i;
			td1.illegal_abilities = 0;
			td1.allow_cancel = 0;

			int perms = count_subtype(i, TYPE_PERMANENT, -1);
			amount = MIN(amount, perms);

			int sacrifices = 0;
			while( sacrifices < amount ){
					instance->number_of_targets = 0;
					if( can_target(&td) ){
						td.allow_cancel = ! can_target(&td1) ? 0 : 1;
						if( new_pick_target(&td, "Select a permanent you control to exile.", 0, GS_LITERAL_PROMPT) ){
							kill_card(i, instance->targets[0].card, KILL_REMOVE);
							sacrifices++;
						}
						else{
							if( can_target(&td1) ){
								td1.allow_cancel = 0;
								if( new_pick_target(&td1, "Select a card in your hand to exile.", 0, GS_LITERAL_PROMPT) ){
									rfg_card_in_hand(i, instance->targets[0].card);
									sacrifices++;
								}
							}
						}
					}
					else{
						if( can_target(&td1) ){
							td1.allow_cancel = 0;
							if( new_pick_target(&td1, "Select a card in your hand to exile.", 0, GS_LITERAL_PROMPT) ){
								rfg_card_in_hand(i, instance->targets[0].card);
								sacrifices++;
							}
						}
					}
					if( ! can_target(&td) && ! can_target(&td1) ){
						break;
					}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_driver_of_the_dead(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		char msg[100] = "Select a creature card with cmc 2 or less.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.cmc = 3;
		this_test.cmc_flag = 3;
		if( new_special_count_grave(player, &this_test) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

int card_dread_slaver(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( sengir_vampire_trigger(player, card, event, 2) ){
		int i;
		for(i=2; i<instance->targets[11].card+2; i++){
			int t_player = instance->targets[i].player;
			int id = instance->targets[i].card;
			int count = count_graveyard(t_player)-1;
			const int *grave = get_grave(t_player);
			while( count > -1 ){
					if( cards_data[grave[count]].id == id ){
						reanimate_permanent(player, card, t_player, count, REANIMATE_ADD_BLACK_ZOMBIE);
						break;
					}
					count--;
			}
		}
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_essence_harvest(int player, int card, event_t event){
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
			if( validate_target(player, card, &td, 0) ){
				int amount = get_highest_power(player);
				lose_life(instance->targets[0].player, amount);
				gain_life(player, amount);
			}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_evernight_shade(int player, int card, event_t event){
	undying(player, card, event);
	return generic_shade(player, card, event, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0);
}

int card_exquisite_blood(int player, int card, event_t event){
	return global_enchantment(player, card, event);
}

int card_ghoulflesh(int player, int card, event_t event){
	return generic_aura(player, card, event, 1-player, -1, -1, 0, 0, SUBTYPE_ZOMBIE, COLOR_TEST_BLACK, 0);
}

int card_gloom_surgeon(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage && damage->damage_target_card == card && damage->damage_target_player == player)
	{
	  int amount = damage->info_slot;
	  damage->info_slot = 0;
	  rfg_top_n_cards_of_deck(player, amount);
	}

  return 0;
}

int card_grave_exchange(int player, int card, event_t event){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int selected = select_a_card(player, player, 2, 0, 1, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( selected != -1 && pick_target(&td, "TARGET_PLAYER") ){
				const int *grave = get_grave(player);
				instance->targets[1].player = selected;
				instance->targets[1].card = grave[selected];
			}
			else{
				spell_fizzled = 1;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL){
			 int selected = instance->targets[1].player;
			 const int *grave = get_grave(player);
			 if( instance->targets[1].card == grave[selected] ){
				 add_card_to_hand(player, grave[selected]);
				 remove_card_from_grave(player, selected);
			 }
			if( valid_target(&td) ){
				impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_griselbrand(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	lifelink(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 7);
	}

	return generic_activated_ability(player, card, event, 0, 0, 0, 0, 0, 0, 0, 7, 0, 0);
}

int card_harvester_of_souls(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	deathtouch(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.type_flag = F1_NO_TOKEN;
		this_test.not_me = 1;

		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		draw_some_cards_if_you_want(player, card, player, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_homicidal_seclusion(int player, int card, event_t event)
{
  if ((event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && affected_card_controller == player
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && count_permanents_by_type(player, TYPE_CREATURE) == 1
	  && in_play(player, card) && !is_humiliated(player, card))
	{
	  if (event == EVENT_ABILITIES)
		lifelink(affected_card_controller, affected_card, event);
	  else if (event == EVENT_POWER)
		event_result += 3;
	  else	// event == EVENT_TOUGHNESS
		event_result += 1;
	}

  return global_enchantment(player, card, event);
}

int card_hunted_ghoul(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY && affect_me(player, card) ){
		if( has_subtype(attacking_card_controller, attacking_card, SUBTYPE_HUMAN) ){
			event_result = 1;
		}
	}
	return 0;
}

int card_human_frailty(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_HUMAN;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target Human creature.", 1, NULL);
}

int card_killing_wave(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = instance->info_slot;
		APNAP(p,{
					int cpl = can_pay_life(p, amount);
					if( can_sacrifice(player, p, 1, TYPE_CREATURE, 0) ){
						int count = active_cards_count[p]-1;
						while( count > -1 ){
								if( in_play(p, count) && is_what(p, count, TYPE_CREATURE) ){
									int choice = 1;
									if( cpl && life[p]>= amount ){
										int ai_choice = 0;
										if( life[p]-amount < 6 ){
											ai_choice = 1;
										}
										char buffer[100];
										snprintf(buffer, 100, " Pay %d life\n Sacrifice this", amount );
										choice = do_dialog(p, p, count, -1, -1, buffer, ai_choice);
									}
									if( choice == 0 ){
										lose_life(p, amount);
									}
									else{
										kill_card(p, count, KILL_SACRIFICE);
									}
								}
								count--;
						}
					};
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_maalfeld_twins(int player, int card, event_t event){
	/* Maalfeld Twins	|5|B
	 * Creature - Zombie 4/4
	 * When ~ dies, put two 2/2 |Sblack Zombie creature tokens onto the battlefield. */

	if( graveyard_from_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_ZOMBIE, 2);
	}

	return 0;
}

int card_marrow_bats(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );

	if( land_can_be_played & LCBP_REGENERATION ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && can_pay_life(player, 4) &&
			can_regenerate(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
		  ){
			return 0x63;
		}
		else if( event == EVENT_ACTIVATE ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					lose_life(player, 4);
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				 regenerate_target(player, instance->parent_card);
		}
	}
	return 0;
}

int card_mental_agony(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			new_multidiscard(instance->targets[0].player, 2, 0, player);
			lose_life(instance->targets[0].player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_necrobite(int player, int card, event_t event)
{
	/* Necrobite	|2|B
	 * Instant
	 * Target creature gains deathtouch until end of turn. Regenerate it. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 0,0, VANILLA_PUMP_REGENERATE,SP_KEYWORD_DEATHTOUCH);
}

int card_polluted_dead(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_LAND") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_predators_gambit(int player, int card, event_t event)
{
  /* Predator's Gambit	|B
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +2/+1.
   * Enchanted creature has intimidate as long as its controller controls no other creatures. */

  card_instance_t* inst;
  if (event == EVENT_ABILITIES
	  && (inst = in_play(player, card)) && affect_me(inst->damage_target_player, inst->damage_target_card)
	  && count_permanents_by_type(inst->damage_target_player, TYPE_CREATURE) == 1)
	intimidate(inst->damage_target_player, inst->damage_target_card, event);

  return generic_aura(player, card, event, player, 2,1, 0,0, 0,0,0);
}

// renegade demon --> vanilla

int card_searchlight_geist(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 3, 1, 0, 0, 0, 0, 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
}

int card_soulcage_fiend(int player, int card, event_t event){

	if( graveyard_from_play(player, card, event) ){
		lose_life(player, 3);
		lose_life(1-player, 3);
	}

	return 0;
}

int card_treacherous_pit_dweller(int player, int card, event_t event){
	/*
	  Treacherous Pit-Dweller |B|B
	  Creature - Demon 4/3
	  When Treacherous Pit-Dweller enters the battlefield from a graveyard, target opponent gains control of it.
	  Undying (When this creature dies, if it had no +1/+1 counters on it, return it to the battlefield under its owner's control with a +1/+1 counter on it.)
	*/
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( check_special_flags3(trigger_cause_controller, trigger_cause, SF3_REANIMATED) ){
			target_definition_t td;
			default_target_definition(player, card, &td, 0);
			td.zone = TARGET_ZONE_PLAYERS;
			if( would_validate_arbitrary_target(&td, 1-player, -1) && comes_into_play(player, card, event) ){
				give_control_of_self(player, card);
			}
		}
	}

	return undying(player, card, event);
}

int card_triumph_of_cruelty(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( get_highest_power(player) > -1 && get_highest_power(player) >= get_highest_power(1-player) ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
			if( valid_target(&td) ){
				discard(instance->targets[0].player, 0, player);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_undead_executioner(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			pump_until_eot(player, instance->damage_source_card, instance->targets[0].player, instance->targets[0].card, -2, -2);
		}
	}

	return 0;
}

static int unhallowed_pact_effect(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller && affect_me(player, card ) ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					seek_grave_for_id_to_reanimate(player, card, instance->targets[0].player, cards_data[instance->targets[0].card].id, REANIMATE_DEFAULT);
					kill_card(player, card, KILL_REMOVE);
			}
		}
	}
	return 0;
}

int card_unhallowed_pact(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if(event == EVENT_GRAVEYARD_FROM_PLAY){
			if( affect_me( instance->damage_target_player, instance->damage_target_card) ){
				card_instance_t *affected = get_card_instance(instance->damage_target_player, instance->damage_target_card);
				if(affected->kill_code != KILL_REMOVE && affected->kill_code > 0 ){
					int legacy = create_legacy_effect(player, card, &unhallowed_pact_effect);
					card_instance_t *leg = get_card_instance(player, legacy);
					leg->targets[0].player = get_owner(instance->damage_target_player, instance->damage_target_card);
					leg->targets[0].card = get_original_internal_card_id(instance->damage_target_player, instance->damage_target_card);
				}
			}
		}
	}
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

//RED
// Archwing Dragon --> Viashivan Sandstalker

int card_aggravate(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			int c;
			for(c=active_cards_count[instance->targets[0].player]-1; c > -1; c--){
				if( in_play(instance->targets[0].player, c) && is_what(instance->targets[0].player, c, TYPE_CREATURE) ){
					int dc = damage_creature(instance->targets[0].player, c, 1, player, card);
					get_card_instance(player, dc)->targets[3].card |= DMG_MUST_ATTACK_IF_DEALT_DAMAGE_THIS_WAY;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_banners_rised(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			pump_subtype_until_eot(player, card, player, -1, 1, 0, 0, 0);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_battle_hymn(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			produce_mana(player, COLOR_RED, count_permanents_by_type(player, TYPE_CREATURE));
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bonfire_of_the_damned(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( played_for_free(player, card) || is_token(player, card) ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
				instance->info_slot = x_value;
			}
		}
		else{
			int amount = charge_mana_for_double_x(player, COLOR_COLORLESS);
			if( spell_fizzled != 1 && amount > -1 && pick_target(&td, "TARGET_PLAYER") ){
				instance->info_slot = amount/2;
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( instance->info_slot > 0 ){
				test_definition_t this_test;
				default_test_definition(&this_test, 0);
				new_damage_all(player, card, instance->targets[0].player, instance->info_slot, 3, &this_test);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_burn_at_the_stake(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_abilities = 0;
	td1.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			int trgs = 1;
			while( can_target(&td1) ){
					if( select_target(player, card, &td1, "Select a creature you control to tap.", &(instance->targets[trgs])) ){
						state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
						trgs++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=1; i<trgs; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
			if( trgs > 1 ){
				instance->number_of_targets = 1;
				instance->info_slot = (trgs-1) * 3;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dangerous_wager(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		discard_all(player);
		draw_cards(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_dual_casting(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}

	else if( in_play(player, card) && instance->damage_target_player != -1 ){
			int t_player = instance->damage_target_player;
			int t_card = instance->damage_target_card;

			card_instance_t *trg = get_card_instance(t_player, t_card);

			target_definition_t td_fork;
			counterspell_target_definition(player, card, &td_fork, TYPE_INSTANT|TYPE_SORCERY);
			td_fork.allowed_controller = td_fork.preferred_controller = t_player;

			if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
				return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SPELL_ON_STACK, 0, 0, 0, 0, 1, 0, 0, &td_fork, NULL);
			}

			if( event == EVENT_RESOLVE_ACTIVATION ){
				copy_spell_from_stack(player, trg->targets[0].player, trg->targets[0].card);
			}

			return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
	}
	else{
		return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
	}
	return 0;
}

int card_falkenrath_exterminator(int player, int card, event_t event){
	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 1, 0) && count_1_1_counters(player, card) > 0 ){
			return can_target(&td1);
		}
	}

	else if( event == EVENT_ACTIVATE  ){
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 1, 0);
			if( spell_fizzled != 1 && pick_target(&td1, "TARGET_CREATURE")){
				instance->number_of_targets = 1;
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td1) ){
				damage_creature_or_player(player, card, event, count_1_1_counters(player, card));
			}
	}
	return card_slith_predator(player, card, event);
}

int card_fervent_cathar(int player, int card, event_t event){
	haste(player, card, event);
	return card_goblin_shortcutter(player, card, event);
}

int card_gang_of_devils(int player, int card, event_t event){
	/* Gang of Devils	|5|R
	 * Creature - Devil 3/3
	 * When ~ dies, it deals 3 damage divided as you choose among one, two, or three target creatures and/or players. */

	if( this_dies_trigger(player, card, event, 2) ){
		target_and_divide_damage(player, card, NULL, NULL, 3);
	}

	return 0;
}

int card_guise_of_fire(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 1, -1, 0, SP_KEYWORD_MUST_ATTACK, 0, 0, 0);
}

int card_hanveir_lancer(int player, int card, event_t event){
	if( is_paired(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FIRST_STRIKE);
		boost_paired_creature(player, card, event, 0, 0, KEYWORD_FIRST_STRIKE, 0);
	}
	return soulbond(player, card, event);
}

int card_havengul_vampire(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return card_slith_predator(player, card, event);
}

int card_heirs_of_stromkirk(int player, int card, event_t event){
	intimidate(player, card, event);
	return card_slith_predator(player, card, event);
}

// Hound of Griselbrand --> Young Wolf

int card_kessig_malcontents(int player, int card, event_t event, int amount){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;
	td1.allow_cancel = 0;

	card_instance_t* instance = get_card_instance(player, card);

	if (comes_into_play(player, card, event)){
		if( can_target(&td1) ){
			if( duh_mode(player) ){
				instance->targets[0].player = 1-player;
				instance->targets[0].card = -1;
				if (!valid_target(&td1)){
					instance->targets[0].player = player;
					if (player == AI || ai_is_speculating == 1){
						ai_modifier -= 32;
					}
				}
			}
			else{
				pick_target(&td1, "TARGET_PLAYER");
			}
			damage_player(instance->targets[0].player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_HUMAN), player, card);
		}
	}

	return 0;
}

int card_kruin_striker(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		pump_ability_until_eot(player, card, player, card, 1, 0, KEYWORD_TRAMPLE, 0);
	}

  return 0;
}

int card_lightning_mauler(int player, int card, event_t event){

	if( is_paired(player, card) ){
		haste(player, card, event);
		boost_paired_creature(player, card, event, 0, 0, 0, SP_KEYWORD_HASTE);
	}

	return soulbond(player, card, event);
}

int card_lightning_prowess(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ABILITIES && in_play(player, card) && instance->targets[0].player != -1 ){
		haste(instance->targets[0].player, instance->targets[0].card, event);
	}
	return card_hermetic_study(player, card, event);
}

int card_mad_prophet(int player, int card, event_t event){

	haste(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD+GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_malicious_intent(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
	   return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
	}

	else if( in_play(player, card) && instance->damage_target_player != -1 ){
			int t_player = instance->damage_target_player;
			int t_card = instance->damage_target_card;

			target_definition_t td1;
			default_target_definition(t_player, t_card, &td1, TYPE_CREATURE);

			card_instance_t *trg = get_card_instance(t_player, t_card);

			if( event == EVENT_CAN_ACTIVATE ){
				return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE");
			}

			if( event == EVENT_ACTIVATE ){
				return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE");
			}

			if( event == EVENT_RESOLVE_ACTIVATION ){
				if( validate_target(t_player, t_card, &td1, 0) ){
					pump_ability_until_eot(player, instance->parent_card, trg->targets[0].player, trg->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
				}
			}

			return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
	}
	else{
		return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
	}
	return 0;
}

int card_malignus(int player, int card, event_t event){
	/* Malignus	|3|R|R
	 * Creature - Elemental Spirit 100/100
	 * ~'s power and toughness are each equal to half the highest life total among your opponents, rounded up.
	 * Damage that would be dealt by ~ can't be prevented. */

	my_damage_cannot_be_prevented(player, card, event);

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += (life[1-player] + 1) / 2;
	}

	return 0;
}

// raging poltergeist --> vanilla

// Reforge the soul -> Wheel of Fortune

int card_riot_ringleader(int player, int card, event_t event)
{
  // Whenever ~ attacks, Human creatures you control get +1/+0 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	pump_subtype_until_eot(player, card, player, SUBTYPE_HUMAN, 1, 0, 0, 0);

  return 0;
}

int card_rush_of_blood(int player, int card, event_t event){

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
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
							get_power(instance->targets[0].player, instance->targets[0].card), 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rite_of_ruin(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		int types[3] = {TYPE_LAND, TYPE_ARTIFACT, TYPE_CREATURE};
		int chosen[3];
		int mode = 0;
		int cc;
		for( cc=0; cc<3; cc++){
			int choice = DIALOG(player, card, EVENT_ACTIVATE, DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_OMIT_ILLEGAL,
						"Land", !(mode & (1<<0)), (count_subtype(player, TYPE_LAND, -1)-count_subtype(1-player, TYPE_LAND, -1))*5,
						"Artifact", !(mode & (1<<1)), (count_subtype(player, TYPE_ARTIFACT, -1)-count_subtype(1-player, TYPE_ARTIFACT, -1))*5,
						"Creature", !(mode & (1<<2)), (count_subtype(player, TYPE_CREATURE, -1)-count_subtype(1-player, TYPE_CREATURE, -1))*5);
			mode |= (1<<(choice-1));
			chosen[cc] = types[choice-1];
		}
		APNAP(p, {
					int i;
					for(i=0; i<3; i++){
						impose_sacrifice(player, card, p, i+1, types[chosen[i]], 0, 0, 0, 0, 0, 0, 0, -1, 0);
					}
				}
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_pillar_of_flame(int player, int card, event_t event){
	/*
	  Pillar of Flame |R
	  Sorcery
	  Pillar of Flame deals 2 damage to target creature or player. If a creature dealt damage this way would die this turn, exile it instead.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card );
			if( instance->targets[0].card != -1 ){
				int dc = damage_target0(player, card, 2);
				get_card_instance(player, dc)->targets[3].card |= DMG_EXILE_IF_LETHALLY_DAMAGED_THIS_WAY;
			}
			else{
				damage_player(instance->targets[0].player, 2, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_scalding_devil(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			damage_player(instance->targets[0].player, 1, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 2, 0, 0, 0, 1, 0, 0, &td1, "TARGET_PLAYER");
}

static void block_deal1dam(int player, int card, int blocking_player, int blocking_card)
{
  damage_creature(blocking_player, blocking_card, 1, player, card);
}
int card_somberwald_vigilante(int player, int card, event_t event)
{
  /* Somberwald Vigilante	|R
   * Creature - Human Warrior 1/1
   * Whenever ~ becomes blocked by a creature, ~ deals 1 damage to that creature. */

  if (event == EVENT_DECLARE_BLOCKERS && current_turn == player && !is_humiliated(player, card))
	for_each_creature_blocking_me(player, card, block_deal1dam, player, card);
  return 0;
}

int card_stonewright(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_paired(player, card) ){
		if( event == EVENT_CAN_ACTIVATE ){
			return has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
		}
		else if( event == EVENT_ACTIVATE ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) ){
					int choice = 0;
					if( is_attacking(instance->targets[3].player, instance->targets[3].card) &&
						is_unblocked(instance->targets[3].player, instance->targets[3].card)
					  ){
						choice = 1;
					}
					if( player != AI ){
						choice = do_dialog(player, player, card, -1, -1, " Pump this creature\n Pump paired creature", 0);
					}
					instance->info_slot = choice+1;
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				if( instance->info_slot == 1 ){
					pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, 0);
				}
				else{
					pump_paired_creature_until_eot(player, instance->parent_card, 1, 0, 0, 0);
				}
		}
	}

	return soulbond(player, card, event);
}

int card_thatchers_revolt(int player, int card, event_t event){
	/* Thatcher Revolt	|2|R
	 * Sorcery
	 * Put three 1/1 |Sred Human creature tokens with haste onto the battlefield. Sacrifice those tokens at the beginning of the next end step. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_HUMAN, &token);
			token.qty = 3;
			token.s_key_plus = SP_KEYWORD_HASTE;
			token.color_forced = COLOR_TEST_RED;
			token.special_infos = 66;
			generate_token(&token);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_thunderous_wrath(int player, int card, event_t event){

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
			damage_creature_or_player(player, card, event, 5);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_tibalt_the_fiend_blooded(int player, int card, event_t event){

	/* Planeswalker - Tibalt (2)
	 * +1: Draw a card, then discard a card at random.
	 * -4: ~ deals damage equal to the number of cards in target player's hand to that player.
	 * -6: Gain control of all creatures until end of turn. Untap them. They gain haste until end of turn. */

	if (IS_ACTIVATING(event)){

		card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		int priority_damage = would_validate_arbitrary_target(&td, 1-player, -1) && life[1-player] <= hand_count[1-player] ? 50 : 0;
		int priority_insurrection = (count_subtype(1-player, TYPE_CREATURE, -1)*3)-(12-(count_counters(player, card, COUNTER_LOYALTY)*2));

		enum{
			CHOICE_DRAW_DISCARD = 1,
			CHOICE_DAMAGE,
			CHOICE_INSURRECTION
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Draw & discard", 1, 5, 1,
						"Damage player", can_target(&td), priority_damage , -4,
						"Emblem", 1, priority_insurrection, -6);

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
			  case CHOICE_DAMAGE:
					pick_target(&td, "TARGET_PLAYER");
					break;

			  case CHOICE_DRAW_DISCARD:
			  case CHOICE_INSURRECTION:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
			switch (choice)
			{
				case CHOICE_DRAW_DISCARD:
					draw_cards(player, 1);
					discard(player, DISC_RANDOM, player);
					break;

				case CHOICE_DAMAGE:
				{
					if( valid_target(&td) ){
						int amount = hand_count[instance->targets[0].player];
						damage_player(instance->targets[0].player, amount, player, instance->parent_card);
					}
				}
				break;

				case CHOICE_INSURRECTION:
				  insurrection_effect(instance->parent_controller, instance->parent_card);
				  break;
			}
	}

	return planeswalker(player, card, event, 2);
}

int card_tyrant_of_discord(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_PLAYERS;

	if( comes_into_play(player, card, event) ){
		instance->targets[0].player = 1-player;
		if( valid_target(&td2) && can_sacrifice(player, 1-player, 1, TYPE_PERMANENT, 0) ){
			int targs[active_cards_count[1-player]];
			int count = 0;
			int p_count = 0;
			while(count < active_cards_count[1-player] ){
					if( ! in_hand(1-player, count) && ! is_what(1-player, count, TYPE_EFFECT) ){
						targs[p_count] = count;
						p_count++;
					}
					count++;
			}
			int remaining = p_count;
			while( remaining > 0 ){
				int to_sac = internal_rand(p_count);
				if( targs[to_sac] != -1 ){
					int stop = is_what(1-player, targs[to_sac], TYPE_LAND);
					kill_card(1-player, targs[to_sac], KILL_SACRIFICE);
					targs[to_sac] = -1;
					remaining--;
					if( stop ){
						break;
					}
				}
			}
		}
	}
	return 0;
}

int card_uncanny_speed(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 0, 0, SP_KEYWORD_HASTE);
			}
			kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_vexing_devil(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		int ai_choice = 0;
		if( life[1-player] < 10 ){
			ai_choice = 1;
		}
		int choice = do_dialog(1-player, player, card, -1, -1, " Take 4 damage\n Pass", ai_choice);
		if( choice == 0 ){
			damage_player(1-player, 4, player, card);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return 0;
}

int card_vigilante_justice(int player, int card, event_t event){
	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( can_target(&td2) && specific_cip(player, card, event, player, 2, TYPE_PERMANENT, 0, SUBTYPE_HUMAN, 0, 0, 0, 0, 0, -1, 0) ){
		pick_target(&td2, "TARGTE_CREATURE_OR_PLAYER");
		damage_creature_or_player(player, card, event, 1);
		instance->number_of_targets = 1;
	}
	return global_enchantment(player, card, event);
}

int card_zealous_conscripts(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	haste(player, card, event);

	if( comes_into_play(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_PERMANENT") ){
			if( instance->targets[0].player == player ){
				untap_card(instance->targets[0].player, instance->targets[0].card);
				if( is_what(instance->targets[0].player, instance->targets[0].card, TYPE_CREATURE) ){
					give_haste(instance->targets[0].player, instance->targets[0].card);
				}
			}
			else{
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}
	return 0;
}

// GREEN
int enchanted_land_has_T_add_n_mana_of_any_one_color(int player, int card, event_t event, int amt){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=100;
		pick_target(&td, "TARGET_LAND");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			instance->damage_target_player = instance->targets[0].player;
			instance->damage_target_card = instance->targets[0].card;
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( in_play(player, card) && instance->targets[0].player != - 1 && instance->targets[0].card !=-1 ){
		if( event == EVENT_CAN_ACTIVATE && ! is_tapped(instance->targets[0].player, instance->targets[0].card)
			&& !is_animated_and_sick(instance->targets[0].player, instance->targets[0].card) ){
			return 1;
		}

		if( event == EVENT_ACTIVATE ){
			produce_mana_tapped_all_one_color(instance->targets[0].player, instance->targets[0].card, COLOR_TEST_ANY_COLORED, amt);
			if (cancel != 1){
				// tap_card() would do this, but clear tapped_for_mana_color first.
				dispatch_event(instance->targets[0].player, instance->targets[0].card, EVENT_TAP_CARD);
			}
		}

		if( event == EVENT_COUNT_MANA && affect_me(player, card) && ! is_tapped(instance->targets[0].player, instance->targets[0].card) && !is_animated_and_sick(instance->targets[0].player, instance->targets[0].card) ){
			declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, amt);
		}
	}
	return 0;
}

int card_abundant_growth(int player, int card, event_t event){
	/* Abundant Growth	|G
	 * Enchantment - Aura
	 * Enchant land
	 * When ~ enters the battlefield, draw a card.
	 * Enchanted land has "|T: Add one mana of any color to your mana pool." */

	if (comes_into_play(player, card, event)){
		draw_a_card(player);
	}

	return enchanted_land_has_T_add_n_mana_of_any_one_color(player, card, event, 1);
}


int card_blessing_of_nature(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int i;
			for(i=3; i>-1; i--){
				pick_target(&td,"TARGET_CREATURE");
				instance->targets[i] = instance->targets[0];
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=3; i>-1; i--){
				if( validate_target(player, card, &td, i) ){
					add_1_1_counter(instance->targets[i].player, instance->targets[i].card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bower_passage(int player, int card, event_t event){
	if( event == EVENT_BLOCK_LEGALITY && attacking_card_controller == player ){
		if( check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING) ){
			event_result = 1;
		}
	}

	return global_enchantment(player, card, event);
}

int card_champion_of_lambholt(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		add_1_1_counter(player, card);
	}

  if (event == EVENT_BLOCK_LEGALITY && player == attacking_card_controller
	  && get_power(affected_card_controller, affected_card) < get_power(player, card))
	event_result = 1;

  return 0;
}

int card_craterhoof_behemoth(int player, int card, event_t event){

	haste(player, card, event);

	if( comes_into_play(player, card, event) ){
		int amount = count_permanents_by_type(player, TYPE_CREATURE);
		pump_subtype_until_eot(player, card, player, -1, amount, amount, KEYWORD_TRAMPLE, 0);
	}
	return 0;
}

int card_descendants_path(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int result = 0;
		int *deck = deck_ptr[player];
		show_deck( player, deck, 1, "Here's the first card of the deck", 0, 0x7375B0 );
		if( is_what(-1, deck[0], TYPE_CREATURE) ){
			int id = cards_data[deck[0]].id;
			int count = active_cards_count[player]-1;
			while( count > -1  ){
					if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) &&
						shares_creature_subtype(-1, deck[0], player, count)
					  ){
						card_ptr_t* c_me = cards_ptr[ id ];
						char buffer[600];
						scnprintf(buffer, 600, " Play %s\n Pass", c_me->name);
						int choice = do_dialog(player, player, card, -1, -1, buffer, 0);
						if( choice == 0 ){
							play_card_in_deck_for_free(player, player, 0);
						}
						result = 1;
						break;
					}
					count--;
			}
		}
		if( result == 0 ){
			put_top_card_of_deck_to_bottom(player);
		}
	}
	return global_enchantment(player, card, event);
}

int card_diregraf_escort(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );
	if( is_paired(player, card) ){
		protection_from_subtype(player, card, event, SUBTYPE_ZOMBIE);
		protection_from_subtype(instance->targets[3].player, instance->targets[3].card, event, SUBTYPE_ZOMBIE);
	}

	return soulbond(player, card, event);
}

int card_druids_familiar(int player, int card, event_t event){

	if( is_paired(player, card) ){
		modify_pt_and_abilities(player, card, event, 2, 2, 0);
		boost_paired_creature(player, card, event, 2, 2, 0, 0);
	}

	return soulbond(player, card, event);
}

int card_eaten_by_spiders(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_abilities = KEYWORD_FLYING;

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
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_druids_repository(int player, int card, event_t event)
{
  /* Druids' Repository	|1|G|G
   * Enchantment
   * Whenever a creature you control attacks, put a charge counter on ~.
   * Remove a charge counter from ~: Add one mana of any color to your mana pool. */

  int amt;
  if ((amt = declare_attackers_trigger(player, card, event, 0, player, -1)))
	add_counters(player, card, COUNTER_CHARGE, amt);

  if (event == EVENT_CAN_ACTIVATE && count_counters(player, card, COUNTER_CHARGE) && can_produce_mana(player, card))
	return 1;

  if (event == EVENT_ACTIVATE && produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, 1))
	remove_counter(player, card, COUNTER_CHARGE);

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card))
	declare_mana_available_any_combination_of_colors(player, COLOR_TEST_ANY_COLORED, count_counters(player, card, COUNTER_CHARGE));

  return global_enchantment(player, card, event);
}

int card_flowering_lumberknot(int player, int card, event_t event)
{
  if ((event == EVENT_ATTACK_LEGALITY || event == EVENT_BLOCK_LEGALITY) && affect_me(player, card) && !is_paired(player, card))
	event_result = 1;

  return 0;
}

int card_geist_trapper(int player, int card, event_t event){
	if( is_paired(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_REACH);
		boost_paired_creature(player, card, event, 0, 0, KEYWORD_REACH, 0);
	}

	return soulbond(player, card, event);
}

int card_grounded(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( event == EVENT_ABILITIES && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			if( event_result & KEYWORD_FLYING ){
				event_result &= ~KEYWORD_FLYING;
			}
		}
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_howlgeist(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( get_power(affected_card_controller, affected_card) < get_power(player, card) ){
				event_result = 1;
			}
		}
	}
	return undying(player, card, event);
}

int card_joint_assault(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
			pump_paired_creature_until_eot(instance->targets[0].player, instance->targets[0].card, 2, 2, 0, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_lair_delve(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int *deck = deck_ptr[player];
			int amount = 2;
			if( amount > count_deck(player) ){
				amount = count_deck(player);
			}
			if( amount > 0 ){
				show_deck( HUMAN, deck, amount, "Cards revealed with Lair Delve", 0, 0x7375B0 );
				int amount2 = amount;
				int i;
				for(i=0; i<amount; i++ ){
					if( is_what(-1, deck[0], TYPE_CREATURE) || is_what(-1, deck[0], TYPE_LAND)){
						add_card_to_hand(player, deck[0]);
						remove_card_from_deck(player, 0);
						amount2--;
					}
				}
				if( amount2 > 0 ){
					put_top_x_on_bottom(player, player, amount2);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_natural_end(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td,"DISENCHANT");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				gain_life(player, 3);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// nettle swine -> vanilla

int card_nightshade_peddler(int player, int card, event_t event){

	if( is_paired(player, card) ){
		deathtouch(player, card, event);
		boost_paired_creature(player, card, event, 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
	}

	return soulbond(player, card, event);
}

int card_pathbreaker_wurm(int player, int card, event_t event){
	if( is_paired(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_TRAMPLE);
		boost_paired_creature(player, card, event, 0, 0, KEYWORD_TRAMPLE, 0);
	}

	return soulbond(player, card, event);
}

int card_primal_surge(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int *deck = deck_ptr[player];
			while( deck[0] != -1 ){
					show_deck( HUMAN, deck, 1, "Here's the first card of deck", 0, 0x7375B0 );
					if( is_what(-1, deck[0], TYPE_PERMANENT) ){
						int choice = 0;
						if( player != AI ){
							char buffer[500];
							card_ptr_t* c_me = cards_ptr[ cards_data[deck[0]].id ];
							scnprintf(buffer, 500, " Put %s into play\n Pass", c_me->name);
							choice = do_dialog(player, player, card, -1, -1, buffer, 0);
						}
						if( choice == 0 ){
							put_into_play_a_card_from_deck(player, player, 0);
						}
						else{
							rfg_top_card_of_deck(player);
							break;
						}
					}
					else{
						rfg_top_card_of_deck(player);
						break;
					}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rain_of_thorns(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT|TYPE_ENCHANTMENT|TYPE_LAND);
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			card_instance_t *instance = get_card_instance(player, card);
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_ARTIFACT);

			instance->info_slot = 0;
			int trgs = 0;
			if( can_target(&td) ){
				if( new_pick_target(&td,"TARGET_ARTIFACT", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
					instance->info_slot |= 1;
				}
			}
			td.required_type = TYPE_ENCHANTMENT;
			if( can_target(&td) ){
				if( new_pick_target(&td,"TARGET_ENCHANTMENT", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
					instance->info_slot |= 2;
				}
			}
			td.required_type = TYPE_LAND;
			if( can_target(&td) ){
				if( new_pick_target(&td,"TARGET_LAND", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
					instance->info_slot |= 4;
				}
			}
			if( trgs == 0 ){
				spell_fizzled = 1;
			}
			else{
				int i;
				for (i = 0; i < trgs; i++){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
				}
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			card_instance_t *instance = get_card_instance(player, card);
			target_definition_t td;
			default_target_definition(player, card, &td, 0);

			int i;
			for (i = 0; i < instance->number_of_targets; ++i){
				if (instance->info_slot & 1){
					td.required_type = TYPE_ARTIFACT;
					instance->info_slot &= ~1;
				} else if (instance->info_slot & 2){
					td.required_type = TYPE_ENCHANTMENT;
					instance->info_slot &= ~2;
				} else {
					td.required_type = TYPE_LAND;
				}
				if( validate_target(player, card, &td, i) ){
					kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_revenge_of_the_hunted(int player, int card, event_t event){

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
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 6, 6,
								  KEYWORD_TRAMPLE, SP_KEYWORD_LURE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sheltering_word(int player, int card, event_t event){
	/*
	  Sheltering Word |1|G
	  Instant
	  Target creature you control gains hexproof until end of turn. You gain life equal to that creature's toughness.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HEXPROOF);
			gain_life(player, get_toughness(instance->targets[0].player, instance->targets[0].card) );
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature you control.", 1, NULL);
}

int card_snare_the_skies(int player, int card, event_t event){
	/* Snare the Skies	|G
	 * Instant
	 * Target creature gets +1/+1 and gains reach until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 1, 1, KEYWORD_REACH, 0);
}

int card_somberwald_druid(int player, int card, event_t event){
	// This is an approximation
	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			int amount = 3;
			int color_to_reduce = 0;
			card_ptr_t* c = cards_ptr[get_id(affected_card_controller, affected_card)];
			int clrs[5];
			clrs[0] = c->req_black;
			clrs[1] = c->req_blue;
			clrs[2] = c->req_green;
			clrs[3] = c->req_red;
			clrs[4] = c->req_white;
			int i;
			int par = 0;
			for(i=0; i<5; i++){
				if( clrs[i] > par ){
					par = clrs[i];
					color_to_reduce = i;
				}
			}
			if( color_to_reduce == 0 ){
				amount-=c->req_black;
				if( amount < 0 ){
					amount = 0;
				}
				COST_BLACK-=(3-amount);
			}
			if( color_to_reduce == 1 ){
				amount-=c->req_blue;
				if( amount < 0 ){
					amount = 0;
				}
				COST_BLUE-=(3-amount);
			}
			if( color_to_reduce == 2 ){
				amount-=c->req_green;
				if( amount < 0 ){
					amount = 0;
				}
				COST_GREEN-=(3-amount);
			}
			if( color_to_reduce == 3 ){
				amount-=c->req_red;
				if( amount < 0 ){
					amount = 0;
				}
				COST_RED-=(3-amount);
			}
			if( color_to_reduce == 4 ){
				amount-=c->req_white;
				if( amount < 0 ){
					amount = 0;
				}
				COST_WHITE-=(3-amount);
			}
			if( amount > 0 ){
				COST_COLORLESS-=amount;
			}
		}
	}

	if( specific_spell_played(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		tap_card(player, card);
	}

	return 0;
}

static int terrifying_presence_effect(int player, int card, event_t event)
{
  card_instance_t *instance, *damage = combat_damage_being_prevented(event);

  if (damage
	  && (instance = get_card_instance(player, card))
	  && !(damage->damage_source_card == instance->targets[0].card && damage->damage_source_player == instance->targets[0].player)
	  && (damage->targets[3].player & TYPE_CREATURE))	// probably redundant to status check
	damage->info_slot = 0;

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_terrifying_presence(int player, int card, event_t event){
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
			int legacy = create_legacy_effect(player, card, &terrifying_presence_effect);
			card_instance_t *leg = get_card_instance( player, legacy );
			leg->targets[0] = instance->targets[0];
			leg->number_of_targets = 1;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_soul_of_the_harvest(int player, int card, event_t event){
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.type_flag = F1_NO_TOKEN;
	this_test.not_me = 1;
	if( new_specific_cip(player, card, event, player, 1+player, &this_test) ){
		draw_cards(player, 1);
	}
	return 0;
}

int card_timberland_guide(int player, int card, event_t event){

	/* Bond Beetle	|G
	 * Creature - Insect 0/1
	 * When ~ enters the battlefield, put a +1/+1 counter on target creature. */

	/* Ironshell Beetle	|1|G
	 * Creature - Insect 1/1
	 * When ~ enters the battlefield, put a +1/+1 counter on target creature. */

	/* Satyr Grovedancer	|1|G
	 * Creature - Satyr Shaman 1/1
	 * When ~ enters the battlefield, put a +1/+1 counter on target creature. */

	/* Supply-Line Cranes	|3|W|W
	 * Creature - Bird 2/4
	 * Flying
	 * When ~ enters the battlefield, put a +1/+1 counter on target creature. */

	/* Timberland Guide	|1|G
	 * Creature - Human Scout 1/1
	 * When ~ enters the battlefield, put a +1/+1 counter on target creature. */

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_triumph_of_ferocity(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( get_highest_power(player) >= get_highest_power(1-player) ){
			draw_cards(player, 1);
		}
	}

	return global_enchantment(player, card, event);
}

int card_trusted_forcemage(int player, int card, event_t event){

	if( is_paired(player, card) ){
		modify_pt_and_abilities(player, card, event, 1, 1, 0);
		boost_paired_creature(player, card, event, 1, 1, 0, 0);
	}

	return soulbond(player, card, event);
}

int card_ulvenwald_tracker(int player, int card, event_t event){
	/* Ulvenwald Tracker	|G
	 * Creature - Human Shaman 1/1
	 * |1|G, |T: Target creature you control fights another target creature. */

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allow_cancel = 0;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) && can_target(&td) ){
			return target_available(player, card, &td1) >= 2;
		}
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
			instance->targets[1] = instance->targets[0];
			tap_card(player, card);
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
			pick_target(&td1, "TARGET_CREATURE");
			instance->number_of_targets = 2;
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(player, card, &td, 1) && validate_target(player, card, &td1, 0) ){
				fight(instance->targets[1].player, instance->targets[1].card, instance->targets[0].player, instance->targets[0].card);
			}
	}
	return 0;
}

// Vorstclaw -> vanilla

int card_wandering_wolf(int player, int card, event_t event){
	if( event == EVENT_BLOCK_LEGALITY ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( get_power(affected_card_controller, affected_card) < get_power(player, card) ){
				event_result = 1;
			}
		}
	}
	return 0;
}

int card_wild_defiance(int player, int card, event_t event)
{
  const target_t* targets = any_becomes_target(player, card, event, player, 0, TYPE_CREATURE, -1, TYPE_INSTANT|TYPE_INTERRUPT|TYPE_SORCERY, ANYBODY, RESOLVE_TRIGGER_MANDATORY);
  if (targets)
	for (; targets->player != -1; ++targets)
	  pump_until_eot(player, card, targets->player, targets->card, 3, 3);

  return global_enchantment(player, card, event);
}

int card_wildwood_geist(int player, int card, event_t event){

	if( current_turn == player ){
		modify_pt_and_abilities(player, card, event, 2, 2, 0);
	}

	return 0;
}

int card_wolfir_avenger(int player, int card, event_t event){

	if( in_play(player, card) && (land_can_be_played & LCBP_REGENERATION) ){
		return regeneration(player, card, event, 1, 0, 0, 1, 0, 0);
	}

	return flash(player, card, event);
}

int card_wolfir_silverheart(int player, int card, event_t event){

	if( is_paired(player, card) ){
		modify_pt_and_abilities(player, card, event, 4, 4, 0);
		boost_paired_creature(player, card, event, 4, 4, 0, 0);
	}

	return soulbond(player, card, event);
}

int card_yew_spirit(int player, int card, event_t event){

	/* Yew Spirit	|4|G
	 * Creature - Spirit Treefolk 3/3
	 * |2|G|G: ~ gets +X/+X until end of turn, where X is its power. */

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST){
		int initial_power = get_power(player, card);
		if (initial_power > 0){
			int final_power, num_times_can_activate = generic_shade_amt_can_pump(player, card, 1, 0, MANACOST_XG(2,2), -1);
			for (final_power = initial_power; num_times_can_activate > 0; --num_times_can_activate){
				final_power *= 2;
			}
			return final_power - initial_power;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		int pp = instance->parent_controller, pc = instance->parent_card;
		if (in_play(pp, pc)){
			int plus = get_power(pp, pc);
			pump_until_eot(pp, pc, pp, pc, plus, plus);
		}
	}

	return generic_activated_ability(player, card, event, 0, 2, 0, 0, 2, 0, 0, 0, 0, 0);
}

// GOLD

static int check_targets_for_bruna(int player, int card, int mode){

	int result = -1;
	int par = -1;
	int i;
	for(i=0; i<2; i++){
		int count;
		for(count=0; count<active_cards_count[i]; count++){
			if( in_play(i, count) && is_what(i, count, TYPE_ENCHANTMENT) && has_subtype(i, count, SUBTYPE_AURA_CREATURE) ){
				card_instance_t *instance = get_card_instance(i, count);
				if( !(i== player && count == card) ){
					if( instance->damage_target_player > -1 && is_what(instance->damage_target_player, instance->damage_target_card, TYPE_CREATURE) ){
						if( mode == 0 ){
							if( player == HUMAN ){
								return 1;
							}
							if( player == AI && i == HUMAN){
								return 1;
							}
						}
						if( mode == 1 ){
							if( player == AI && i == HUMAN){
								if( get_cmc(i, count) > par ){
									par = get_cmc(i, count);
									result = count;
								}
							}
						}
					}
				}
			}
		}
	}
	return result;
}

int card_bruna_light_of_alabaster(int player, int card, event_t event){

	/* Bruna, Light of Alabaster	|3|W|W|U
	 * Legendary Creature - Angel 5/5
	 * Flying, vigilance
	 * Whenever ~ attacks or blocks, you may attach to it any number of Auras on the battlefield and you may put onto the battlefield attached to it any number
	 * of Aura cards that could enchant it from your graveyard and/or hand. */

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card) ||
		(blocking(player, card, event) && !is_humiliated(player, card))
	   ){
		while( check_targets_for_bruna(player, card, 0) ){
				if( player == HUMAN ){
					target_definition_t td;
					base_target_definition(player, card, &td, TYPE_ENCHANTMENT);
					td.required_subtype = SUBTYPE_AURA_CREATURE;

					card_instance_t *instance = get_card_instance(player, card);

					instance->number_of_targets = 0;
					if( pick_next_target_noload(&td, "Select an Aura card with enchant creature.") ){
						card_instance_t *aura = get_card_instance(instance->targets[0].player, instance->targets[0].card);
						aura->damage_target_player = player;
						aura->damage_target_card = card;
					}
					else{
						cancel = 0;
						break;
					}
				}
				else{
					int result = check_targets_for_bruna(player, card, 1);
					if( result > -1 ){
						card_instance_t *aura = get_card_instance(1-player, result);
						aura->damage_target_player = player;
						aura->damage_target_card = card;
					}
				}
		}
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select an Aura card with enchant creature.");
		this_test.subtype = SUBTYPE_AURA_CREATURE;
		while( new_special_count_grave(player, &this_test) ){
				int result = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_CMC, &this_test);
				if( result > -1 ){
					put_into_play_aura_attached_to_target(player, result, player, card);
				}
				else{
					break;
				}
		}
		this_test.zone = TARGET_ZONE_HAND;
		while( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
				int result = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
				if( result > -1 ){
					put_into_play_aura_attached_to_target(player, result, player, card);
				}
				else{
					break;
				}
		}
	}

	return 0;
}

int card_gisela_blade_of_goldnight(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  /* Since the way this is supposed to work is the controller of the object being dealt damage chooses the order of replacement effects, assume he chooses to
   * halve damage before doing anything else, to maximize his subsequent damage prevention effects (so apply that during EVENT_PREVENT_DAMAGE); or to double
   * damage after doing everything else, to maximize his previous damage prevention effects (so apply that during EVENT_DEAL_DAMAGE). */
	if( in_play(player, card) && ! is_humiliated(player, card) ){
		card_instance_t* damage = damage_being_prevented(event);
		if (damage && damage->damage_target_player == player && !(damage->state & STATE_CANNOT_TARGET)){
			damage->info_slot -= (damage->info_slot + 1) / 2;
		}

		damage = damage_being_dealt(event);
		if (damage && damage->damage_target_player == 1-player){
			damage->info_slot *= 2;
		}
	}

	return 0;
}

int card_sigarda_host_of_herons(int player, int card, event_t event){

	hexproof(player, card, event);

	check_legend_rule(player, card, event);

	// The true effect is in "sacrifice.c"

	return 0;
}

// ARTIFACTS

int card_angels_tomb(int player, int card, event_t event){

	if( ! is_what(player, card, TYPE_CREATURE) &&
		specific_cip(player, card, event, player, 1+player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		add_a_subtype(player, card, SUBTYPE_ANGEL);
		int result = artifact_animation(player, card, player, card, 1, 3, 3, KEYWORD_FLYING, 0);
		card_instance_t *instance = get_card_instance( player, result );
		instance->targets[6].player = COLOR_TEST_WHITE;
	}
	return 0;
}

int card_angelic_armaments(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( is_equipping(player, card) ){
			reset_subtypes(instance->targets[8].player, instance->targets[8].card, 2);
		}
		if( validate_target(player, card, &td, 0) ){
			add_a_subtype(instance->targets[1].player, instance->targets[1].card, SUBTYPE_ANGEL);
		}
	}

	if (event == EVENT_SET_COLOR && is_equipping(player, card) && affect_me(instance->targets[8].player, instance->targets[8].card)){
		event_result |= COLOR_TEST_WHITE;
	}

	return vanilla_equipment(player, card, event, 4, 2, 2, KEYWORD_FLYING, 0);
}

int card_bladed_bracers(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ABILITIES && is_equipping(player, card) ){
		if( has_subtype(instance->targets[8].player, instance->targets[8].card, SUBTYPE_ANGEL) ||
			has_subtype(instance->targets[8].player, instance->targets[8].card, SUBTYPE_HUMAN)
		  ){
			vigilance(instance->targets[8].player, instance->targets[8].card, event);
		}
	}

	return vanilla_equipment(player, card, event, 2, 1, 1,  0, 0);
}

int card_conjuring_closet(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_EOT && current_turn == player){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;

		if( eot_trigger_mode(player, card, event, player, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( pick_target(&td, "ASHNODS_BATTLEGEAR") ){	// "Select target creature you control."
				card_instance_t *instance = get_card_instance( player, card );
				instance->number_of_targets = 1;
				blink_effect(instance->targets[0].player, instance->targets[0].card, 0);
			}
		}
	}

	return basic_equipment(player, card, event, 4);
}

int card_gallows_at_willow_hill(int player, int card, event_t event){
	/* Gallows at Willow Hill	|3
	 * Artifact
	 * |3, |T, Tap three untapped Humans you control: Destroy target creature. Its controller puts a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_abilities = 0;
	td1.required_subtype = SUBTYPE_HUMAN;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && target_available(player, card, &td1) > 2 ){
			if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
				return can_target(&td);
			}
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
			int trgs = 0;
			while( trgs < 3 ){
					if( new_pick_target(&td1, "TARGET_PERMANENT", trgs, 0) ){
						state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
						trgs++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<trgs; i++){
				if( instance->targets[i].card != -1 ){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
					if( trgs == 3 ){
						tap_card(instance->targets[i].player, instance->targets[i].card);
					}
				}
			}
			if( trgs == 3 ){
				tap_card(player, card);
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIRIT, &token);
			token.t_player = instance->targets[0].player;
			token.key_plus = KEYWORD_FLYING;
			token.color_forced = COLOR_TEST_WHITE;
			generate_token(&token);
		}
	}

	return 0;
}

// Hunted guardian --> Serra Angel

int card_moonsilver_spear(int player, int card, event_t event)
{
  /* Moonsilver Spear	|4
   * Artifact - Equipment
   * Equipped creature has first strike.
   * Whenever equipped creature attacks, put a 4/4 |Swhite Angel creature token with flying onto the battlefield.
   * Equip |4 */

  card_instance_t* instance = get_card_instance(player, card);
  if (declare_attackers_trigger(player, card, event, 0, instance->targets[8].player, instance->targets[8].card))
	generate_token_by_id(player, card, CARD_ID_ANGEL);

  return vanilla_equipment(player, card, event, 4, 0, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_narstad_scrapper(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_otherworld_atlas(int player, int card, event_t event){

	/* |T: Put a charge counter on ~.
	 * |T: Each player draws a card for each charge counter on ~. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( player == HUMAN ){
				return 1;
			}
			else{
				if( count_counters(player, card, COUNTER_CHARGE) > 0 && hand_count[player] > hand_count[1-player] ){
					return 1;
				}
			}
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int choice = 1;
			if( player == HUMAN ){
				choice = 0;
				if( count_counters(player, card, COUNTER_CHARGE) > 0 ){
					choice = do_dialog(player, player, card, -1, -1, " Add a counter\n Draw cards\n Cancel", 0);
				}
			}
			if( choice != 2 ){
				instance->info_slot = choice+1;
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			add_counter(instance->parent_controller, instance->parent_card, COUNTER_CHARGE);
		}
		if( instance->info_slot == 2 ){
			draw_cards(player, count_counters(player, card, COUNTER_CHARGE));
			draw_cards(1-player, count_counters(player, card, COUNTER_CHARGE));
		}
	}

	if( current_turn != player && player == AI && can_use_activated_abilities(player, card) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && eot_trigger(player, card, event) ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				tap_card(player, card);
				add_counter(player, card, COUNTER_CHARGE);
			}
		}
	}

	return 0;
}

int card_scroll_of_avacyn(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
			if( check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_ANGEL)  ){
				gain_life(player, 5);
			}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_scroll_of_griselbrand(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			discard(instance->targets[0].player, 0, player);
			if( check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DEMON)  ){
				lose_life(instance->targets[0].player, 3);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_tormentors_trident(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 3, 3, 0, 0, SP_KEYWORD_MUST_ATTACK);
}

int card_vanguards_shield(int player, int card, event_t event){

	if( is_equipping(player, card) ){
		attached_creature_can_block_additional(player, card, event, 1);
	}

	return vanilla_equipment(player, card, event, 3, 0, 3, 0, 0);
}

int card_vessel_of_endless_rest(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		instance->targets[0].player = player;
		if( count_graveyard(player)> 0 ){
			if( count_graveyard(1-player)> 0 ){
				pick_target(&td, "TARGET_PLAYER");
			}
		}
		else{
			instance->targets[0].player = 1-player;
		}
		if( count_graveyard(instance->targets[0].player)> 0 ){
			int selected = select_a_card(player, instance->targets[0].player, 2, 1, 1, -1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			from_graveyard_to_deck(instance->targets[0].player, selected, 2);
		}
	}
	return mana_producer(player, card, event);
}

// LANDS

int card_alchemists_refuge(int player, int card, event_t event)
{
  /* Alchemist's Refuge	""
   * Land
   * |T: Add |1 to your mana pool.
   * |G|U, |T: You may cast nonland cards this turn as though they had flash. */
  return enable_flash_land(player, card, event, MANACOST_GU(1,1), 0, TYPE_LAND, "Select a nonland card.", 1);
}

int card_cavern_of_souls(int player, int card, event_t event){
	// This is just an approximation

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[1].card = select_a_subtype(player, card);
		play_land_sound_effect_force_color(player, card, COLOR_TEST_ANY_COLORED);
		return 0;	// so the sound effect doesn't play again in mana_producer()
	}

	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player && ! is_tapped(player, card)){
		if( instance->targets[1].card > 0 && has_creature_type(affected_card_controller, affected_card_controller, instance->targets[1].card) ){
			card_ptr_t* c = cards_ptr[get_id(affected_card_controller, affected_card_controller)];
			if( c->req_black > 0 ){
				COST_BLACK-=1;
			}
			else if( c->req_blue > 0  ){
				COST_BLUE-=1;
			}
			else if( c->req_green > 0  ){
					COST_GREEN-=1;
			}
			else if( c->req_red > 0 ){
					COST_RED-=1;
			}
			else if( c->req_white > 0 ){
					COST_WHITE-=1;
			}
		}
	}

	if(! is_tapped(player, card)  && trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){
		if( trigger_cause_controller == player && instance->targets[1].card > 0 ){
			int trig = 0;

			if( make_test_in_play(trigger_cause_controller, trigger_cause, -1, TYPE_CREATURE, 0, instance->targets[1].card, 0, 0, 0, 0, 0, -1, 0) ){
				trig = 1;
				instance->targets[1].player = 66;
			}
			if( trig > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= 2;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						tap_card(player, card);
						instance->targets[1].player = 0;
				}
			}
		}
	}

	if( instance->targets[1].player != 66 ){
		return mana_producer(player, card, event);
	}
	return 0;
}

int card_desolate_lighthouse(int player, int card, event_t event){
	/* Desolate Lighthouse	""
	 * Land
	 * |T: Add |C to your mana pool.
	 * |1|U|R, |T: Draw a card, then discard a card. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 2, 0, 1, 0, 1, 0) &&
			can_use_activated_abilities(player, card)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Draw & discard\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, 1, 0, 1, 0, 1, 0);
				if( spell_fizzled != 1){
					instance->info_slot = 1;
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				draw_cards(player, 1);
				discard(player, 0, player);
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

int card_seraphs_sanctuary(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		gain_life(player, 1);
	}

	if( specific_cip(player, card, event, player, 2, TYPE_PERMANENT, 0, SUBTYPE_ANGEL, 0, 0, 0, 0, 0, -1, 0) ){
		gain_life(player, 1);
	}

	return mana_producer(player, card, event);
}


int card_slayers_stronghold(int player, int card, event_t event){
	/* Slayers' Stronghold	""
	 * Land
	 * |T: Add |C to your mana pool.
	 * |R|W, |T: Target creature gets +2/+0 and gains vigilance and haste until end of turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 1) &&
			can_use_activated_abilities(player, card) && can_target(&td)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Pump a creature\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 1);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
					instance->info_slot = 1;
					instance->number_of_targets = 1;
				}
				else{
					untap_card_no_event(player, card);
				}
		}
		else{
			return mana_producer(player, card, event);
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				if( valid_target(&td) ){
					pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player,
											instance->targets[0].card, 2, 0, 0, SP_KEYWORD_HASTE+SP_KEYWORD_VIGILANCE);
				}
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

