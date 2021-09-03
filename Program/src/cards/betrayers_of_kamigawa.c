#include "manalink.h"

// global functions

static int eot_flipper(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( arcane_spirit_spell_trigger(player, card, event, 1+duh_mode(player)) ){
		add_counter(player, card, COUNTER_KI);
	}

	if( count_counters(player, card, COUNTER_KI) > 1 && trigger_condition == TRIGGER_EOT && affect_me(player, card) && reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			event_result |= 1+player;
		}
		else if( event == EVENT_RESOLVE_TRIGGER){
				true_transform(player, card);
				verify_legend_rule(player, card, instance->targets[13].card);
		}
	}

	return 0;
}

static int get_card_with_specific_cmc_from_hand(int player, int card, int req_cmc, int mode){
	// mode = 1 --> card with cmc = req_cmc
	// mode = 2 --> card with cmc < req_cmc
	// mode = 4 --> card with cmc > req_cmc
	// mode = 8 --> selected card cannot be "card"
	int result = -1;
	int count = 0;
	int par_value = 1000;
	while( count < active_cards_count[player] ){
			if( in_hand(player, count) && ( !(mode & 4) || count != card) ){
				if( ((mode & 1)&& get_cmc(player, count) == req_cmc) ||
					((mode & 2) && get_cmc(player, count) < req_cmc) ||
					((mode & 4) && get_cmc(player, count) > req_cmc)
				  ){
					if( get_base_value(player, count) < par_value ){
						par_value = get_base_value(player, count);
						result = count;
					}
				}
			}
			count++;
	}
	return result;
}

static int shoal(int player, int card, event_t event, int clr, target_definition_t *td1, const char *prompt){

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_NONE);
	td.required_color = color_to_color_test(clr);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.zone = TARGET_ZONE_HAND;
	td.special = TARGET_SPECIAL_NOT_ME;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( can_target(&td) ){
			null_casting_cost(player, card);
		}
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if (!played_for_free(player, card) && !is_token(player, card)){
				int choice = 1;
				if( can_target(&td) ){
					choice = do_dialog(player, player, card, -1, -1, " RFG a card\n Play normally\n Cancel", 0);
				}

				if( choice == 2 ){
					spell_fizzled = 1;
				}
				else if( choice == 0 ){
					instance->state |= STATE_CANNOT_TARGET;
					if( !select_target(player, card, &td, "Choose a card in hand", NULL)){
						instance->state &= ~STATE_CANNOT_TARGET;
						spell_fizzled = 1;
						return 0;
					}
					instance->state &= ~STATE_CANNOT_TARGET;
					instance->number_of_targets = 0;
					instance->info_slot = get_cmc(player, instance->targets[0].card);
					rfg_card_in_hand(player, instance->targets[0].card);
				}
				else if( choice == 1 ){
						if( can_target(&td) ){
							charge_mana(player, clr, 2);
						}
						charge_mana(player, COLOR_COLORLESS, -1);
						if( spell_fizzled != 1 ){
							instance->info_slot = x_value;
						}
				}
				if( clr != COLOR_GREEN ){
					load_text(0, prompt);
					if( choice == 0 ){
						td1->allow_cancel = 0;
					}
					int result = select_target(td1->player, td1->card, td1, text_lines[0], NULL);
					if( ! result ){
						spell_fizzled = 1;
					}
					if( result && clr == COLOR_WHITE ){
						SET_BYTE2(instance->info_slot) = instance->targets[0].player;
						SET_BYTE3(instance->info_slot) = instance->targets[0].card;
						instance->number_of_targets = 0;
						target_definition_t td2;
						default_target_definition(player, card, &td2, TYPE_CREATURE);
						td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
						if( choice == 0 ){
							td2.allow_cancel = 0;
						}
						if( ! pick_target(&td2, "TARGET_CREATURE_OR_PLAYER") ){
							spell_fizzled = 1;
						}
					}
				}
			}
	}

	return 0;
}

static int genju_return_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( do_dialog(player, player, card, -1, -1, " Return Genju to your hand\n Pass", 0) == 0 ){
			seek_grave_for_id_to_reanimate(player, card, instance->targets[0].player, cards_data[instance->targets[0].card].id, REANIMATE_RETURN_TO_HAND);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

static int genju(int player, int card, event_t event, int subtype, int pow, int tou, int key, int s_key, int clr){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != - 1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( affect_me(t_player, t_card) && in_play(t_player, t_card) ){
				card_instance_t *land = get_card_instance(t_player, t_card);
				if( land->kill_code > 0 && land->kill_code < KILL_REMOVE ){
					set_special_flags3(player, card, SF3_ENCHANTED_PERMANENT_DYING);
				}
			}
			if( affect_me(player, card) && in_play(player, card) ){
				if( instance->kill_code > 0 && instance->kill_code < KILL_REMOVE ){
					if( check_special_flags3(player, card, SF3_ENCHANTED_PERMANENT_DYING) ){
						int legacy = create_legacy_effect(player, card, &genju_return_legacy);
						get_card_instance(player, legacy)->targets[0].player = get_owner(player, card);
						get_card_instance(player, legacy)->targets[0].card = get_original_internal_card_id(player, card);
						get_card_instance(player, legacy)->targets[11].player = 66;
					}
				}
			}
		}

		if( event == EVENT_CAN_ACTIVATE && ! is_what(t_player, t_card, TYPE_CREATURE) ){
			return generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL);
		}

		if(event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL);
		}

		if(event == EVENT_RESOLVE_ACTIVATION ){
			add_a_subtype(t_player, t_card, SUBTYPE_SPIRIT);
			land_animation2(instance->parent_controller, instance->parent_card, t_player, t_card, 1, pow, tou, key, s_key, clr, 0);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.required_subtype = subtype;

	return targeted_aura_custom_prompt(player, card, event, &td, subtype > 0 ? get_hacked_land_text(player, card, "Select target %s.", subtype) : "Select target land.");
}

static int patron(int player, int card, event_t event, int subtype){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_subtype = subtype;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		instance->info_slot = 0;
		if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			int costs[6];
			card_ptr_t* c = cards_ptr[ get_id(player, card) ];
			costs[0] = get_updated_casting_cost(player, card, -1, event, c->req_colorless);
			costs[1] = c->req_black;
			costs[2] = c->req_blue;
			costs[3] = c->req_green;
			costs[4] = c->req_red;
			costs[5] = c->req_white;
			int count = 0;
			while( count < active_cards_count[player]  ){
					if( in_play(player, count) && is_what(player, count, TYPE_PERMANENT) && has_subtype(player, count, subtype) ){
						card_ptr_t* c1 = cards_ptr[ get_id(player, count) ];
						if( has_mana_multi(player, costs[0]-c1->req_colorless, costs[1]-c1->req_black, costs[2]-c1->req_blue, costs[3]-c1->req_green,
											costs[4]-c1->req_red, costs[5]-c1->req_white)
						  ){
							instance->info_slot = 1;
							null_casting_cost(player, card);
							break;
						}
					}
					count++;
			}
			if( instance->info_slot != 1 && ! can_sorcery_be_played(player, event) ){
				infinite_casting_cost();
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! played_for_free(player, card) && ! is_token(player, card) ){
				int choice = 0;
				if( has_mana_to_cast_iid(player, get_card_instance(player, card)->internal_card_id, event) ){
					if( instance->info_slot == 1 ){
						choice = do_dialog(player, player, card, -1, -1, " Play normally\n Pay an Offering\n Cancel", 1);
					}
				}
				else{
					choice = 1;
				}
				if( choice == 0 ){
					if( instance->info_slot == 1 ){
						charge_mana_from_id(player, card, event, get_id(player, card));
					}
				}
				if( choice == 1 ){
					if( pick_target(&td, "TARGET_PERMANENT") ){
						card_ptr_t* c1 = cards_ptr[ get_id(instance->targets[0].player, instance->targets[0].card) ];
						int costs[6];
						card_ptr_t* c = cards_ptr[ get_id(player, card) ];
						costs[0] = get_updated_casting_cost(player, card, -1, event, c->req_colorless)-c1->req_colorless;
						costs[1] = c->req_black-c1->req_black;
						costs[2] = c->req_blue-c1->req_blue;
						costs[3] = c->req_green-c1->req_green;
						costs[4] = c->req_red-c1->req_red;
						costs[5] = c->req_white-c1->req_white;
						int i;
						for(i=0; i<6; i++){
							if( costs[i] < 0 ){
								costs[i] = 0;
							}
						}
						charge_mana_multi(player, costs[0], costs[1], costs[2], costs[3], costs[4], costs[5]);
						if( spell_fizzled != 1 ){
							kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
						}
					}
					else{
						spell_fizzled = 1;
						return 0;
					}
				}
				if( choice == 2 ){
					spell_fizzled = 1;
					return 0;
				}
			}
	}

	return flash(player, card, event);
}

int ninjutsu(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white, int csvid)
{
  if (!IS_ACTIVATING_FROM_HAND(event))
	return 0;

  target_definition_t td;
  base_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = player;
  td.preferred_controller = player;
  td.required_state = TARGET_STATE_ATTACKING;
  td.illegal_state = TARGET_STATE_ISBLOCKED;

  if (event == EVENT_CAN_ACTIVATE_FROM_HAND)
	return current_phase == PHASE_AFTER_BLOCKING && has_mana_multi(player, cless, black, blue, green, red, white) && can_target(&td);

  if (event == EVENT_ACTIVATE_FROM_HAND)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (charge_mana_multi(player, cless, black, blue, green, red, white) && pick_target(&td, "NINJUTSU"))
		bounce_permanent(instance->targets[0].player, instance->targets[0].card);
	}

  if (event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1)
	{
	  card_instance_t* inst;
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if ((inst = in_hand(player, c)) && get_id(player, c) == csvid)
		  {
			inst->state |= STATE_ATTACKING | STATE_TAPPED;
			put_into_play(player, c);
			break;
		  }
	}

 return 0;
}

// cards
int card_azamuki_treachery_incarnate(int player, int card, event_t event){

	// Remove a ki counter from Azamuki, Treachery Incarnate: Gain control of target creature until end of turn.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			gain_control_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_KI), &td, "TARGET_CREATURE");
}

int card_cunning_bandit(int player, int card, event_t event){	// flip => Azamuki, Treachery Incarnate
	/* Whenever you cast a Spirit or Arcane spell, you may put a ki counter on ~.
	 * At the beginning of the end step, if there are two or more ki counters on ~, you may flip it. */
	return eot_flipper(player, card, event);
}

int card_baku_altar(int player, int card, event_t event){
	/* Baku Altar	|2
	 * Artifact
	 * Whenever you cast a Spirit or Arcane spell, you may put a ki counter on ~.
	 * |2, |T, Remove a ki counter from ~: Put a 1/1 colorless Spirit creature token onto the battlefield. */

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		add_counter(player, card, COUNTER_KI);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && count_counters(player, card, COUNTER_KI) > 0){
		return has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			remove_counter(player, card, COUNTER_KI);
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_SPIRIT);
	}

	return 0;
}

int card_bile_urchin(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_blademane_baku(int player, int card, event_t event){

	/* Whenever you cast a Spirit or Arcane spell, you may put a ki counter on ~.
	 * |1, Remove X ki counters from ~: For each counter removed, ~ gets +2/+0 until end of turn. */

	card_instance_t *instance = get_card_instance(player, card);

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		add_counter(player, card, COUNTER_KI);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && count_counters(player, card, COUNTER_KI) > 0){
		return has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			int amount = count_counters(player, card, COUNTER_KI);
			if( player != AI ){
				amount = choose_a_number(player, "Remove how many counters?", count_counters(player, card, COUNTER_KI));
				if( amount == 0 || amount > count_counters(player, card, COUNTER_KI) ){
					spell_fizzled = 1;
				}
			}
			remove_counters(player, card, COUNTER_KI, amount);
			instance->info_slot = amount*2;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, instance->info_slot, 0);
	}

	return 0;
}

ARCANE(card_blazing_shoal){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, instance->info_slot, 0);
		}
	}
	return shoal(player, card, event, COLOR_RED, &td, "TARGET_CREATURE");
}

// budoka pupil --> Cunning bandit

int card_ichiga_who_topples_oaks(int player, int card, event_t event){

	/* Ichiga, Who Topples Oaks
	 * Legendary Creature - Spirit
	 * 4/3
	 * Trample
	 * Remove a ki counter from Ichiga, Who Topples Oaks: Target creature gets +2/+2 until end of turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_KI), &td, "TARGET_CREATURE");
}

// callow jushi --> Cunning bandit

int card_jaraku_the_interloper(int player, int card, event_t event){

	/* Jaraku the Interloper
	 * Legendary Creature - Spirit
	 * 3/4
	 * Remove a ki counter from Jaraku the Interloper: Counter target spell unless its controller pays |2. */

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, &td, 0, 2);
	}

	return generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK, MANACOST0, GVC_COUNTER(COUNTER_KI), &td, NULL);
}

int card_child_of_thorns(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

static const char* target_must_have_counters(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return count_counters(player, card, -1) ? NULL : "must have counters";
}
int card_chisei_heart_of_oceans(int player, int card, event_t event)
{
  /* Chisei, Heart of Oceans	|2|U|U
   * Legendary Creature - Spirit 4/4
   * Flying
   * At the beginning of your upkeep, sacrifice ~ unless you remove a counter from a permanent you control. */

  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  target_definition_t td;
	  base_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.preferred_controller = player;
	  td.allowed_controller = player;
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td.extra = (int)target_must_have_counters;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_PERMANENT"))
		{
		  instance->number_of_targets = 0;
		  counter_t typ = choose_existing_counter_type(player, player, card, instance->targets[0].player, instance->targets[0].card,
													   CECT_HUMAN_CAN_CANCEL | CECT_REMOVE | CECT_CONSIDER_ALL, -1, -1);
		  if (typ != COUNTER_invalid)
			{
			  remove_counter(instance->targets[0].player, instance->targets[0].card, typ);
			  return 0;
			}
		}

	  kill_card(player, card, KILL_SACRIFICE);
	}

  return 0;
}

ARCANE(card_crack_the_earth)
{
  // Each player sacrifices a permanent.
  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	APNAP(p,
		  player_sacrifices_a_permanent(player, card, p, TYPE_PERMANENT, SAC_NO_CANCEL));

  return 0;
}

int card_day_of_destiny(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( affected_card_controller == player && is_what(affected_card_controller,affected_card, TYPE_CREATURE) &&
		is_legendary(affected_card_controller,affected_card)
	  ){
		modify_pt_and_abilities(affected_card_controller,affected_card, event, 2, 2, 0);
	}
	return global_enchantment(player, card, event);
}

ARCANE(card_disrupting_shoal){

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_NONE);
	td.required_color = COLOR_TEST_BLUE;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.zone = TARGET_ZONE_HAND;
	td.special = TARGET_SPECIAL_NOT_ME;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( can_target(&td) ){
			null_casting_cost(player, card);
		}
	}

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( !IS_AI(player) ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			return result;
		}
		else{
			if( result > 0 ){
				instance->targets[0].player = card_on_stack_controller;
				instance->targets[0].card = card_on_stack;
				int cmc = get_cmc(card_on_stack_controller, card_on_stack);
				if( get_card_with_specific_cmc_from_hand(player, card, cmc, 9) ){
					return result;
				}
				else{
					if( has_mana(player, COLOR_COLORLESS, cmc) ){
						return result;
					}
				}
			}
		}
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if (!played_for_free(player, card) && !is_token(player, card)){
				int orig_player = instance->targets[0].player;
				int orig_card = instance->targets[0].card;
				int choice = 1;
				int ai_choice = 1;
				int cmc = get_cmc(instance->targets[0].player, instance->targets[0].card);
				int ai_result = get_card_with_specific_cmc_from_hand(player, card, cmc, 9);
				if( ai_result != -1 ){
					ai_choice = 0;
				}

				if( can_target(&td) ){
					choice = do_dialog(player, player, card, -1, -1, " RFG a card\n Play normally\n Cancel", ai_choice);
				}

				if( choice == 2 ){
					spell_fizzled = 1;
				}
				else if( choice == 0 ){
					if( IS_AI(player) ){
						instance->targets[0].player = player;
						instance->targets[0].card = ai_result;
					}
					else{
						instance->state |= STATE_CANNOT_TARGET;
						if( !select_target(player, card, &td, "Choose a card in hand", NULL)){
							instance->state &= ~STATE_CANNOT_TARGET;
							spell_fizzled = 1;
							return 0;
						}
					}
					instance->state &= ~STATE_CANNOT_TARGET;
					instance->number_of_targets = 1;
					instance->info_slot = get_cmc(player, instance->targets[0].card);
					rfg_card_in_hand(player, instance->targets[0].card);
					instance->targets[0].player = orig_player;
					instance->targets[0].card = orig_card;
				}
				else if( choice == 1 ){
						if( can_target(&td) ){
							charge_mana(player, COLOR_BLUE, 2);
						}
						if( !IS_AI(player) ){
							charge_mana(player, COLOR_COLORLESS, -1);
							if( spell_fizzled != 1 ){
								instance->targets[0].player = orig_player;
								instance->targets[0].card = orig_card;
								instance->info_slot = x_value;
							}
						}
						else{
							charge_mana(player, COLOR_COLORLESS, cmc);
							if( spell_fizzled != 1 ){
								instance->targets[0].player = orig_player;
								instance->targets[0].card = orig_card;
								instance->info_slot = cmc;
							}
						}
				}
			}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( get_cmc(instance->targets[0].player, instance->targets[0].card) == instance->info_slot ){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		}
	}
	return 0;
}

int card_empty_shrine_kannushi(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	if( event == EVENT_ABILITIES && affect_me(player, card) ){
		int i;
		int keyword = 0;
		for(i=1; i<6; i++){
			td.required_color = (1<<i);
			if( can_target(&td) ){
				keyword |= (1<<(10+i));
			}
		}
		event_result |= keyword;
	}
	return 0;
}

int card_enshrined_memories(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_CAN_CAST){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->info_slot = x_value;
	}
	else if(event == EVENT_RESOLVE_SPELL){
			int max = instance->info_slot;
			int *deck = deck_ptr[player];
			show_deck( HUMAN, deck, max, "Cards revealed by Enshrined Memories", 0, 0x7375B0 );
			int i;
			int after = max;
			for(i=max-1; i > -1; i--){
				if( is_what(-1, deck[i], TYPE_CREATURE) ){
				   add_card_to_hand(player, deck[i]);
				   remove_card_from_deck(player, i);
				   after--;
				}
			}
			if( after > 0 ){
			   put_top_x_on_bottom(player, player, after);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// faithful squire --> Cunning bandit

int card_kaiso_memory_of_loyalty(int player, int card, event_t event){

	/* Kaiso, Memory of Loyalty
	 * Legendary Creature - Spirit
	 * 3/4
	 * Flying
	 * Remove a ki counter from Kaiso, Memory of Loyalty: Prevent all damage that would be dealt to target creature this turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			prevent_all_damage_to_target(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_KI), &td, "TARGET_CREATURE");
}

int card_final_judgment(int player, int card, event_t event){
	/*
	  Final Judgment |4|W|W
	  Sorcery
	  Exile all creatures.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, ANYBODY, &this_test, KILL_REMOVE);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

ARCANE(card_first_volley){
	// ~ deals 1 damage to target creature and 1 damage to that creature's controller.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
			damage_player(instance->targets[0].player, 1, player, card);
		}
	}

	return 0;
}

static int flames_of_the_blood_hand_legacy(int player, int card, event_t event){
	my_damage_cannot_be_prevented(player, card, event);
	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_flames_of_the_blood_hand(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			int legacy = create_legacy_effect(player, card, &flames_of_the_blood_hand_legacy);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[0].player = player;
			leg->targets[0].card = card;
			leg->targets[3].player = instance->targets[0].player;
			leg->targets[3].card = CARD_ID_FLAMES_OF_THE_BLOOD_HAND;

			damage_player(instance->targets[0].player, 4, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_forked_branch_garami(int player, int card, event_t event){
	return soulshift(player, card, event, 4, 2);
}

int card_frostling(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_fumiko_the_lowblood(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	all_must_attack_if_able(1-player, event, -1);

	if (event == EVENT_DECLARE_BLOCKERS){
		instance->targets[8].player = count_attackers(current_turn);
	}

	return bushido(player, card, event, 0);
}

int card_genju_of_the_cedars(int player, int card, event_t event){
	return genju(player, card, event, SUBTYPE_FOREST, 4, 4, 0, 0, COLOR_TEST_GREEN);
}

int card_genju_of_the_falls(int player, int card, event_t event){
	return genju(player, card, event, SUBTYPE_ISLAND, 3, 2, KEYWORD_FLYING, 0, COLOR_TEST_BLUE);
}

int card_genju_of_the_fields(int player, int card, event_t event){
	// Special-cased in the legacy effect
	return genju(player, card, event, SUBTYPE_PLAINS, 2, 5, 0, 0, COLOR_TEST_WHITE);
}

int card_genju_of_the_realm(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	return genju(player, card, event, -1, 8, 12, KEYWORD_TRAMPLE, 0, 0);
}

int card_genju_of_the_spires(int player, int card, event_t event){
	return genju(player, card, event, SUBTYPE_MOUNTAIN, 6, 1, 0, 0, COLOR_TEST_RED);
}

// gnarled mass --> vanilla

// goblin cohorth -->Mogg Conscripts

int card_gods_eye_gate_to_reikai(int player, int card, event_t event){
	/* Gods' Eye, Gate to the Reikai	""
	 * Legendary Land
	 * |T: Add |1 to your mana pool.
	 * When ~ is put into a graveyard from the battlefield, put a 1/1 colorless Spirit creature token onto the battlefield. */

	check_legend_rule(player, card, event);

	if( graveyard_from_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_SPIRIT);
	}

	return mana_producer(player, card, event);
}

ARCANE_WITH_SPLICE(card_goryos_vengeance, MANACOST_XB(2,1)){
	// Return target legendary creature card from your graveyard to the battlefield. That creature gains haste. Exile it at the beginning of the next end step.
	// Splice onto Arcane |2|B

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && special_count_grave(player, TYPE_CREATURE, 0, SUBTYPE_LEGEND, 0, 0, 0, 0, 0, -1, 0) > 0 ){
		return 1;
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int selected = select_a_card(player, player, 2, 0, 2, -1, TYPE_CREATURE, 0, SUBTYPE_LEGEND, 0, 0, 0, 0, 0, -1, 0);
			if( selected == -1 ){
				spell_fizzled = 1;
			}
			else{
				instance->targets[0].player = selected;
				const int *grave = get_grave(player);
				instance->targets[0].card = grave[selected];
			}
	}
	if(event == EVENT_RESOLVE_SPELL){
		int selected = instance->targets[0].player;
		const int *grave = get_grave(player);
		if( instance->targets[0].card == grave[selected] ){
			reanimate_permanent(player, card, player, selected, REANIMATE_HASTE_AND_EXILE_AT_EOT);
		}
	}

	return 0;
}

int card_heartless_hidetsugu(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<2; i++){
			int dmg = life[i]/2;
			if( dmg > 0 ){
				damage_player(i, dmg, player, instance->parent_card);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_higure_the_still_wind(int player, int card, event_t event){

	/* Higure, the Still Wind	|3|U|U
	 * Legendary Creature - Human Ninja 3/4
	 * Ninjutsu |2|U|U
	 * Whenever ~ deals combat damage to a player, you may search your library for a Ninja card, reveal it, and put it into your hand. If you do, shuffle your
	 * library.
	 * |2: Target Ninja creature is unblockable this turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_subtype = SUBTYPE_NINJA;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
			return can_target(&td);
		}
	}

	if( event == EVENT_ACTIVATE){
		if (charge_mana_for_activated_ability(player, card, MANACOST_X(2)) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		}
	}

	int packets;
	if( (packets = damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRIGGER_OPTIONAL)) ){
		test_definition_t test;
		new_default_test_definition(&test, 0, "Select a Ninja card.");
		test.subtype = SUBTYPE_NINJA;
		test.subtype_flag = MATCH;
		test.qty = packets;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_CMC, &test);
	}

	return ninjutsu(player, card, event, MANACOST_XU(2, 2), CARD_ID_HIGURE_THE_STILL_WIND);
}

// Hired muscle --> Cunning bandit

int card_scarmaker(int player, int card, event_t event){

	/* Scarmaker
	 * Legendary Creature - Spirit
	 * 4/4
	 * Remove a ki counter from Scarmaker: Target creature gains fear until end of turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_FEAR);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_KI), &td, "TARGET_CREATURE");
}

int card_hokori_dust_drinker(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	untap_only_1_permanent_per_upkeep(player, card, event, 2, TYPE_LAND);

	return 0;
}

ARCANE_WITH_SPLICE(card_horobis_whisper, MANACOST_X(-10)){
	// If you control |Ha Swamp, destroy target non|Sblack creature.
	// Splice onto Arcane - Exile four cards from your graveyard.

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
	td.allow_cancel = 0;

	if (event == EVENT_CAN_SPLICE){
		return count_graveyard(player) >= 4;
	}

	if( event == EVENT_SPLICE){
		int i;
		for(i=0; i<4; i++){
			int selected = select_a_card(player, player, TUTOR_FROM_GRAVE, 1, AI_MIN_VALUE, -1, 0,0, 0,0, 0,0, 0,0, -1,0);
			rfg_card_from_grave(player, selected);
		}
		return 1;
	}

	if( event == EVENT_CAN_CAST  ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) && check_battlefield_for_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_SWAMP)) ){
			card_instance_t *instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_in_the_web_of_war(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)  ){
		pump_ability_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, 2, 0, 0, SP_KEYWORD_HASTE);
	}

	return global_enchantment(player, card, event);
}

int card_indebted_samurai(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_SAMURAI;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return bushido(player, card, event, 1);
}

int card_ink_eyes_servant_of_oni(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		if( count_graveyard_by_type(1-player, TYPE_CREATURE) > 0 ){
			global_tutor(player, 1-player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}
	if( land_can_be_played & LCBP_REGENERATION ){
		return regeneration(player, card, event, 1, 1, 0, 0, 0, 0);
	}

	return ninjutsu(player, card, event, 3, 2, 0, 0, 0, 0, CARD_ID_INK_EYES_SERVANT_OF_ONI);
}

ARCANE(card_ire_of_kaminari){
	// ~ deals damage to target creature or player equal to the number of Arcane cards in your graveyard.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, count_graveyard_by_subtype(player, SUBTYPE_ARCANE));
			}
	}
	return 0;
}

int card_isao_enlightned_bushi(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_SAMURAI;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	cannot_be_countered(player, card, event);

	if( land_can_be_played & LCBP_REGENERATION ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) &&
			can_target(&td)
		  ){
			return 0x63;
		}
		else if( event == EVENT_ACTIVATE ){
				charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
					regenerate_target(instance->targets[0].player, instance->targets[0].card);
				}
		}
	}

	return bushido(player, card, event, 2);
}

int card_iwamori_the_open_fist(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		global_tutor(1-player, 1-player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, 2, TYPE_CREATURE, 0, SUBTYPE_LEGEND, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_kami_of_false_hope(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		fog_effect(player, card);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_kentaro_the_smiling_cat(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && affect_me(player, card) ){
		int i, sorc = can_sorcery_be_played(player, event);
		card_instance_t* aff;
		for (i = 0; i < active_cards_count[player]; ++i){
			if ((aff = in_hand(player, i)) &&
				has_subtype(player, i, SUBTYPE_SAMURAI) &&
				/* Instants, interrupts, cards with flash can be played at any time.  Otherwise, only when a sorcery is legal.  Check card_data_t::type directly
				 * instead of using is_what(), which strips TYPE_INSTANT from permanent types. */
				(sorc || cards_data[aff->internal_card_id].type & (TYPE_INSTANT | TYPE_INTERRUPT)) &&
				has_mana(player, COLOR_COLORLESS, get_cmc(player, i))
			   ){
				return 1;
			}
		}
	}

	else if(event == EVENT_ACTIVATE && affect_me(player, card) ){
			target_definition_t td;
			default_target_definition(player, card, &td, 0);
			td.zone = TARGET_ZONE_HAND;
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.required_subtype = SUBTYPE_SAMURAI;
			td.illegal_abilities = 0;
			td.illegal_type = TYPE_LAND;

			instance->number_of_targets = 0;
			if( pick_next_target_noload(&td, "Select a Samurai card.") ){
				instance->number_of_targets = 0;
				card_instance_t* aff = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				if( !(cards_data[aff->internal_card_id].type & (TYPE_INSTANT | TYPE_INTERRUPT)) &&
					!can_sorcery_be_played(player, event)
				  ){
					spell_fizzled = 1;
				} else {
					charge_mana(player, COLOR_COLORLESS, get_cmc(instance->targets[0].player, instance->targets[0].card));
					if( spell_fizzled != 1  ){
						play_card_in_hand_for_free(instance->targets[0].player, instance->targets[0].card);
						cant_be_responded_to = 1;	// The cast-for-X samurai will be respondable to, but the extra activation effect from Kentaro won't be
					}
				}
			}
	}

	return bushido(player, card, event, 1);
}

/* This is just an approximation */
int card_kira_great_glass_spinner(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! is_humiliated(player, card) ){

		if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller ){
			int trig = 0;
			if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
				card_instance_t *instance = get_card_instance(trigger_cause_controller, trigger_cause);
				int i;
				for(i=0; i<instance->number_of_targets; i++){
					if( instance->targets[i].card != -1 && instance->targets[i].player == player &&
						is_what(instance->targets[i].player, instance->targets[i].card, TYPE_CREATURE) &&
						! check_special_flags2(instance->targets[i].player, instance->targets[i].card, SF2_KIRA_GREAT_GLASS_SPINNER)
					  ){
						trig = 1;
						break;
					}
				}
			}

			if( trig > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= 2;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						card_instance_t *instance = get_card_instance(trigger_cause_controller, trigger_cause);
						int i;
						for(i=0; i<instance->number_of_targets; i++){
							if( instance->targets[i].card != -1 && instance->targets[i].player == player &&
								is_what(instance->targets[i].player, instance->targets[i].card, TYPE_CREATURE)
							  ){
								set_special_flags2(instance->targets[i].player, instance->targets[i].card, SF2_KIRA_GREAT_GLASS_SPINNER);
							}
						}
						kill_card(trigger_cause_controller, trigger_cause, KILL_SACRIFICE);
				}
			}
		}

		if ( stack_size >= 1){
			if( stack_data[stack_size - 1].generating_event != EVENT_RESOLVE_TRIGGER &&
				cards_data[stack_data[stack_size - 1].internal_card_id].id != 904
			  ){
				int trig = 0;
				card_instance_t *instance = get_card_instance(stack_cards[stack_size - 1].player, stack_cards[stack_size - 1].card);
				int i;
				for(i=0; i<instance->number_of_targets; i++){
					if( instance->targets[i].card != -1 && instance->targets[i].player == player &&
						is_what(instance->targets[i].player, instance->targets[i].card, TYPE_CREATURE) &&
						! check_special_flags2(instance->targets[i].player, instance->targets[i].card, SF2_KIRA_GREAT_GLASS_SPINNER)
					 ){
						trig = 1;
						break;
					}
				}
				if( trig == 1 ){
					for(i=0; i<instance->number_of_targets; i++){
						if( instance->targets[i].card != -1 && instance->targets[i].player == player &&
							is_what(instance->targets[i].player, instance->targets[i].card, TYPE_CREATURE) &&
							! check_special_flags2(instance->targets[i].player, instance->targets[i].card, SF2_KIRA_GREAT_GLASS_SPINNER)
						 ){
							set_special_flags2(instance->targets[i].player, instance->targets[i].card, SF2_KIRA_GREAT_GLASS_SPINNER);
						}
					}
					instance->internal_card_id = -1;
				}
			}
		}

		if( event == EVENT_CLEANUP || leaves_play(player, card, event) ){
			remove_special_flags2(player, -1, SF2_KIRA_GREAT_GLASS_SPINNER);
		}
	}
	return 0;
}

int card_kyoki_sanitys_eclipse(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	check_legend_rule(player, card, event);

	if( arcane_spirit_spell_trigger(player, card, event, 2) ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		if( would_validate_target(player, card, &td, 0) && hand_count[1-player] > 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);

			ec_definition_t this_definition;
			default_ec_definition(1-player, 1-player, &this_definition);
			this_definition.effect = EC_RFG;
			this_definition.ai_selection_mode = AI_MIN_VALUE;

			new_effect_coercion(&this_definition, &this_test);
		}
	}

	return 0;
}

int card_lifegift(int player, int card, event_t event){

	if( specific_cip(player, card, event, 2, 1+player, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		gain_life(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_lifespinner(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_sacrifice_as_cost(player, 3, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0) ){
				tap_card(player, card);
				impose_sacrifice(player, card, player, 2, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, 2, TYPE_PERMANENT, 5, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_loam_dweller(int player, int card, event_t event){

	if( arcane_spirit_spell_trigger(player, card, event, 2) ){
		global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY_TAPPED, 0, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_mannichi_the_fevered_dream(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // |1|R: Switch each creature's power and toughness until end of turn.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  int p, c;
	  char marked[2][151] = {{0}};
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_CREATURE))
			marked[p][c] = 1;

	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (marked[p][c])
			switch_power_and_toughness_until_eot(instance->parent_controller, instance->parent_card, p, c);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_XR(1,1), 0, NULL, NULL);
}

int card_mark_of_the_oni(int player, int card, event_t event){

	/* Mark of the Oni	|2|B
	 * Enchantment - Aura
	 * Enchant creature
	 * You control enchanted creature.
	 * At the beginning of the end step, if you control no Demons, sacrifice ~. */

	if( eot_trigger(player, card, event) && ! check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DEMON) ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return card_control_magic(player, card, event);
}

int card_mark_of_sakiko(int player, int card, event_t event)
{
  /* Mark of Sakiko	|1|G	0x200dbd0
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature has "Whenever this creature deals combat damage to a player, add that much |G to your mana pool. Until end of turn, this mana doesn't empty from your mana pool as steps and phases end." */

  if (attached_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_REPORT_DAMAGE_DEALT))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int amt = instance->targets[16].player;
	  instance->targets[16].player = 0;
	  produce_mana(player, COLOR_GREEN, amt);
	  mana_doesnt_drain_until_eot(player, COLOR_GREEN, amt);
	}

  return vanilla_aura(player, card, event, player);
}

int card_matsu_tribe_sniper(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, instance->parent_card);
		}
	}

	return freeze_when_damage(player, card, event);
}

int card_mirror_gallery(int player, int card, event_t event){
	if( leaves_play(player, card, event) ){
		int i;
		for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && is_legendary(i, count) ){
							verify_legend_rule(i, count, get_id(i, count));
						}
						count--;
				}
		}
	}

	return 0;
}

int card_mistblade_shinobi(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE );
  td.allowed_controller = 1-player;
  td.preferred_controller = 1-player;

  card_instance_t *instance = get_card_instance( player, card );

  if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
	 if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
		if( valid_target(&td) ){
		   bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	 }
  }

  return ninjutsu(player, card, event, 0, 0, 1, 0, 0, 0, CARD_ID_MISTBLADE_SHINOBI);
}

int card_moonlit_strider(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, select_a_protection(player) | KEYWORD_RECALC_SET_COLOR, 0);
	}

	return soulshift(player, card, event, 3, 1);
}

int neko_te_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
		if( leaves_play(instance->targets[1].player, instance->targets[1].card, event) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_neko_te(int player, int card, event_t event)
{
  /* Neko-Te	|3
   * Artifact - Equipment
   * Whenever equipped creature deals damage to a creature, tap that creature. That creature doesn't untap during its controller's untap step for as long as ~ remains on the battlefield.
   * Whenever equipped creature deals damage to a player, that player loses 1 life.
   * Equip |2 */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE && is_equipping(player, card)){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_source_card == instance->targets[8].card && damage->damage_source_player == instance->targets[8].player ){
				int good = 0;
				if( damage->info_slot > 0 ){
					good = 1;
				}
				else{
					card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
					if( trg->targets[16].player > 0 ){
						good = 1;
					}
				}

				if( good == 1 && instance->info_slot < 8){
					if( instance->info_slot < 0 ){
						instance->info_slot = 0;
					}
					instance->targets[instance->info_slot].player = damage->damage_target_player;
					instance->targets[instance->info_slot].card = damage->damage_target_card;
					instance->info_slot++;
				}
			}
		}
	}

	if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0;i<instance->info_slot;i++){
					if( instance->targets[i].card == -1 ){
						lose_life(instance->targets[i].player, 1);
					}
					else{
						if( in_play(instance->targets[i].player, instance->targets[i].card) ){
							tap_card(instance->targets[i].player, instance->targets[i].card);
							int legacy = create_targetted_legacy_effect(player, card, &neko_te_legacy, instance->targets[i].player, instance->targets[i].card);
							card_instance_t *leg = get_card_instance(player, legacy);
							leg->targets[1].player = player;
							leg->targets[1].card = card;
						}
					}
				}
				instance->info_slot = 0;
		}
	}

	return basic_equipment(player, card, event, 2);
}

int card_ninja_of_the_deep_hours(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_TRIGGER_OPTIONAL | DDBM_MUST_DAMAGE_PLAYER) ){
		draw_cards(player, 1);
	}

	return ninjutsu(player, card, event, MANACOST_XU(1, 1), CARD_ID_NINJA_OF_THE_DEEP_HOURS);
}

ARCANE(card_nourishing_shoal){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, instance->info_slot);
	}
	return shoal(player, card, event, COLOR_GREEN, NULL, NULL);
}

int card_ogre_marauder(int player, int card, event_t event)
{
  // Whenever ~ attacks, it gains "~ is unblockable" until end of turn unless defending player sacrifices a creature.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  int choice = 1;
	  if (can_sacrifice(1-player, player, 1, TYPE_CREATURE, 0))
		choice = DIALOG(player, card, EVENT_ACTIVATE,
						DLG_WHO_CHOOSES(1-player), DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_RANDOM,
						"Ogre Marauder is unblockable", 1, life[1-player] < get_attack_power(player, card) ? 1 : 6,
						"Sacrifice a creature", 1, 3);
	  if (choice == 1)
		pump_ability_until_eot(player, card, player, card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
	  else
		impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

  return 0;
}

int card_okiba_gang_shinobi(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		new_multidiscard(1-player, 2, 0, player);
	}

  return ninjutsu(player, card, event, MANACOST_XB(3, 1), CARD_ID_OKIBA_GANG_SHINOBI);
}

static int opal_eye_card = -1;
static const char* is_damaging_opal_eye(int who_chooses, int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if( is_what(player, card, TYPE_EFFECT) && instance->internal_card_id == damage_card ){
		if( instance->damage_target_player == who_chooses && instance->damage_target_card == opal_eye_card ){
			return NULL;
		}
	}
	return "must be damage to Opal Eye, Konda's Jojimbo";
}

int card_opal_eye_kondas_yojimbo(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.extra = damage_card;

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.extra = (int32_t)is_damaging_opal_eye;
	td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	opal_eye_card = card;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_DAMAGE_PREVENTION | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_DAMAGE");
		if( ! result ){
			result = generic_activated_ability(player, card, event, GAA_DAMAGE_PREVENTION | GAA_CAN_TARGET, MANACOST_XW(1, 1), 0, &td1, "TARGET_DAMAGE");
		}
		return result;
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_DAMAGE_PREVENTION | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_DAMAGE") ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_DAMAGE_PREVENTION | GAA_CAN_TARGET, MANACOST_XW(1, 1), 0, &td1, "TARGET_DAMAGE") ){
				choice = do_dialog(player, player, card, -1, -1, " Redirect damage to Opal Eye\n Prevent damage to Opal-Eye\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST0) && pick_target(&td, "TARGET_DAMAGE") ){
				instance->info_slot = 66;
				tap_card(player, card);
			}
		}

		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST_XW(1, 1)) && pick_target(&td1, "TARGET_DAMAGE") ){
					instance->info_slot = 67;
				}
		}
		else{
			spell_fizzled = 1;
		}

	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			if( valid_target(&td) ){
				card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				target->damage_target_player = instance->parent_controller;
				target->damage_target_card = instance->parent_card;
			}
		}
		if( instance->info_slot == 67 ){
			if( valid_target(&td) ){
				card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				if( target->info_slot > 0 ){
					target->info_slot--;
				}
			}
		}
	}

	return bushido(player, card, event, 1);
}

int card_orb_of_dreams(int player, int card, event_t event){

	/* Orb of Dreams	|3
	 * Artifact
	 * Permanents enter the battlefield tapped. */

	permanents_enters_battlefield_tapped(player, card, event, ANYBODY, TYPE_PERMANENT, NULL);

	return 0;
}

int card_oyobi_who_splits_the_heavens(int player, int card, event_t event){
	/* Oyobi, Who Split the Heavens	|6|W
	 * Legendary Creature - Spirit 3/6
	 * Flying
	 * Whenever you cast a Spirit or Arcane spell, put a 3/3 |Swhite Spirit creature token with flying onto the battlefield. */

	check_legend_rule(player, card, event);

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.pow = 3;
		token.tou = 3;
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_WHITE;
		generate_token(&token);
	}

	return 0;
}

int card_patron_of_the_akki(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	// Whenever ~ attacks, creatures you control get +2/+0 until end of turn.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		pump_creatures_until_eot(player, card, player, 0, 2,0, 0,0, NULL);
	}

	// Goblin offering
	return patron(player, card, event, SUBTYPE_GOBLIN);
}

int card_patron_of_the_kitsune(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	// Whenever a creature attacks, you may gain 1 life.
	if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player) | DAT_SEPARATE_TRIGGERS, 2, -1)){
		gain_life(player, 1);
	}

	// Fox offering
	return patron(player, card, event, SUBTYPE_FOX);
}

int card_patron_of_the_moon(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.zone = TARGET_ZONE_HAND;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_type = TYPE_LAND;

	char msg[100] = "Select a land card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, msg);
	this_test.zone = TARGET_ZONE_HAND;
	this_test.qty = 2;

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	else if(event == EVENT_ACTIVATE ){
			if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test) )
				ai_modifier-=100;
			return generic_activated_ability(player, card, event, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &this_test);
	}

	return patron(player, card, event, SUBTYPE_MOONFOLK);
}

int card_patron_of_the_nezumi(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, 1-player, TYPE_PERMANENT, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		lose_life(1-player, get_card_instance(player, card)->targets[11].card);
		get_card_instance(player, card)->targets[11].card = 0;
	}

	return patron(player, card, event, SUBTYPE_RAT);
}

int card_patron_of_the_orochi(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && ! is_sick(player, card) && instance->targets[1].player != 66 ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return 1;
		}
	}

	else if(event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				instance->targets[1].player = 66;
				tap_card(player, card);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int i, end = active_cards_count[player], clr = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
			int marked[151] = {0};
			for (i = 0; i < end; ++i){
				if (in_play(player, i) && is_tapped(player, i)
					&& (has_subtype(player, i, SUBTYPE_FOREST)
						|| (is_what(player, i, TYPE_CREATURE) && (get_color(player, i) & clr)))){
					marked[i] = 1;
				}
			}
			for (i = 0; i < end; ++i){
				if (marked[i]){
					untap_card(player, i);
				}
			}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return patron(player, card, event, SUBTYPE_SNAKE);
}

int card_petalmane_baku(int player, int card, event_t event){

	/* Whenever you cast a Spirit or Arcane spell, you may put a ki counter on ~.
	 * |1, Remove X ki counters from ~: Add X mana of any one color to your mana pool. */

	if( arcane_spirit_spell_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		add_counter(player, card, COUNTER_KI);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && count_counters(player, card, COUNTER_KI) > 0){
		return has_mana(player, COLOR_COLORLESS, 1);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			int amount = count_counters(player, card, COUNTER_KI);
			if( !IS_AI(player) ){
				amount = choose_a_number(player, "Remove how many counters?", count_counters(player, card, COUNTER_KI));
				if( amount == 0 || amount > count_counters(player, card, COUNTER_KI) ){
					spell_fizzled = 1;
				}
			}
			remove_counters(player, card, COUNTER_KI, amount);
			produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, amount);
			tapped_for_mana_color = -2;	// mana ability, so can't respond (but not "tapping for mana", or even tapping)
		}
	}

	return 0;
}

int card_reduce_to_dreams(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		manipulate_all(player, card, player, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_BOUNCE);
		manipulate_all(player, card, 1-player, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_BOUNCE);
		manipulate_all(player, card, player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_BOUNCE);
		manipulate_all(player, card, 1-player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_BOUNCE);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_ronin_warclub(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( specific_cip(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		equip_target_creature(player, card, instance->targets[1].player, instance->targets[1].card);
	}

	return vanilla_equipment(player, card, event, 5, 2, 1, 0, 0);
}

int card_sakiko_mother_of_summer(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	card_instance_t* damage = combat_damage_being_dealt(event);
	if (damage &&
		damage->damage_source_player == player &&
		(damage->targets[3].player & TYPE_CREATURE) &&	// probably redundant to combat damage check
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER)
	   ){
		get_card_instance(player, card)->info_slot += damage->targets[16].player;
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player ){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->info_slot <= 0){
			return 0;
		}
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				produce_mana(player, COLOR_GREEN, instance->info_slot);
				mana_doesnt_drain_until_eot(player, COLOR_GREEN, instance->info_slot);
				instance->info_slot = 0;
		}
	}
	return 0;
}

int card_sakura_tribe_springcaller(int player, int card, event_t event)
{
  /* Sakura-Tribe Springcaller	|3|G	0x200dbd5
   * Creature - Snake Shaman 2/4
   * At the beginning of your upkeep, add |G to your mana pool. Until end of turn, this mana doesn't empty from your mana pool as steps and phases end. */

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  produce_mana(player, COLOR_GREEN, 1);
	  mana_doesnt_drain_until_eot(player, COLOR_GREEN, 1);
	}

  return 0;
}

int card_scourge_of_numai(int player, int card, event_t event){

	if( !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && current_turn == player ){
		int count = count_upkeeps(player);
		if(event == EVENT_TRIGGER && count > 0 && ! check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_OGRE) ){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				lose_life(player, 2*count);
		}
	}

	return 0;
}

ARCANE(card_shining_shoal){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ANY);
	td1.extra = damage_card;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( (land_can_be_played & LCBP_DAMAGE_PREVENTION) && can_target(&td1) ){
			return 99;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *damage = get_card_instance(BYTE2(instance->info_slot), BYTE3(instance->info_slot));
		if( damage->info_slot <= instance->info_slot ){
			damage->damage_target_player = instance->targets[0].player;
			damage->damage_target_card = instance->targets[0].card;
		}
		else{
			damage->info_slot-=instance->info_slot;
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot,
							damage->damage_source_player, damage->damage_source_card);
		}
	}
	return shoal(player, card, event, COLOR_WHITE, &td1, "TARGET_DAMAGE");
}

int card_shirei_shizos_caretaker(int player, int card, event_t event){//UNUSEDCARD

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY && (affected_card_controller == player) ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && get_power(affected_card_controller, affected_card) < 2 ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4 ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				int pos = instance->targets[11].player;
				if( pos < 10 ){
					instance->targets[pos].card = get_id(affected_card_controller, affected_card);
				}
				instance->targets[11].player++;
			}
		}
	}

	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_EOT && player == reason_for_trigger_controller){
		if( affect_me(player, card ) ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int i;
					for(i=0; i<instance->targets[11].player; i++){
						if( instance->targets[i].card != -1 ){
							seek_grave_for_id_to_reanimate(player, card, player, instance->targets[i].card, REANIMATE_DEFAULT);
						}
					}
			}
		}
	}
	return 0;
}

int card_shizuko_caller_of_autumn(int player, int card, event_t event)
{
  /* Shizuko, Caller of Autumn	|1|G|G	0x200dbda
   * Legendary Creature - Snake Shaman 2/3
   * At the beginning of each player's upkeep, that player adds |G|G|G to his or her mana pool. Until end of turn, this mana doesn't empty from that player's mana pool as steps and phases end. */

  check_legend_rule(player, card, event);

  upkeep_trigger_ability(player, card, event, ANYBODY);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  produce_mana(current_turn, COLOR_GREEN, 3);
	  mana_doesnt_drain_until_eot(current_turn, COLOR_GREEN, 3);
	}

  return 0;
}

int card_shuko(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 0, 1, 0, 0, 0);
}

ARCANE(card_sickening_shoal){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -instance->info_slot, -instance->info_slot);
		}
	}
	return shoal(player, card, event, COLOR_BLACK, &td, "TARGET_CREATURE");
}

int card_skullsnatcher(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		int count = count_graveyard(1-player);
		if( count > 0 ){
			int removed = 0;
			while( removed < 2 && count > 0){
					int selected = global_tutor(player, 1-player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
					if( selected == -1 ){
						break;
					}
					else{
						removed++;
					}
			}
		}
	}

	return ninjutsu(player, card, event, 0, 1, 0, 0, 0, 0, CARD_ID_SKULLSNATCHER);
}

int card_slumbering_tora(int player, int card, event_t event){

	char msg[100] = "Select an Arcane or Spirit card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, msg);
	this_test.subtype = SUBTYPE_ARCANE;
	this_test.sub2 = SUBTYPE_SPIRIT;
	this_test.subtype_flag = F2_MULTISUBTYPE;
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 2, 0, 0, 0, 0, 0, 0, 0, 0) ){
			return check_battlefield_for_special_card(player, card, player, 0, &this_test);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				instance->info_slot = get_cmc(player, selected);
				discard_card(player, selected);
				set_special_flags(player, card, SF_TYPE_ALREADY_CHANGED);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		artifact_animation(player, instance->parent_card, player, instance->parent_card, 1, instance->info_slot, instance->info_slot, 0, 0 );
	}

	return 0;
}

int card_sosukes_summons(int player, int card, event_t event){
	/* Sosuke's Summons	|2|G
	 * Sorcery
	 * Put two 1/1 |Sgreen Snake creature tokens onto the battlefield.
	 * Whenever a nontoken Snake enters the battlefield under your control, you may return ~ from your graveyard to your hand. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_SNAKE, 2);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_stir_the_grave(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int count = count_graveyard(player);
		if( count > 0 ){
			const int *grave = get_grave(player);
			while( count > -1 ){
					if( is_what(-1, grave[count], TYPE_CREATURE) && has_mana(player, COLOR_COLORLESS, get_cmc_by_id(cards_data[grave[count]].id)) ){
						return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
					}
					count--;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int selected = -1;
		if( player != AI ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				selected = select_a_card(player, player, 2, 0, 1, -1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, x_value+1, 3);
			}
		}
		else{
			int max_cmc = -1;
			int count = count_graveyard(player);
			if( count > 0 ){
				const int *grave = get_grave(player);
				while( count > -1 ){
						int cmc = get_cmc_by_id(cards_data[grave[count]].id);
						if( is_what(-1, grave[count], TYPE_CREATURE) && has_mana(player, COLOR_COLORLESS, cmc) ){
							if( cmc > max_cmc ){
								max_cmc = cmc;
								selected = count;
							}
						}
						count--;
				}
			}
			if( selected > -1 ){
				charge_mana(player, COLOR_COLORLESS, max_cmc);
			}
			else{
				spell_fizzled = 1;
			}
		}

		if( selected == -1 ){
			spell_fizzled = 1;
		}
		else{
			const int *grave = get_grave(player);
			instance->targets[0].player = selected;
			instance->targets[0].card = grave[selected];
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		const int *grave = get_grave(player);
		int selected = instance->targets[0].player;
		if( grave[selected] == instance->targets[0].card ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sway_of_the_stars(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			manipulate_all(player, card, i, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_BOUNCE);
			manipulate_all(player, card, i, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_BOUNCE);
			reshuffle_grave_into_deck(i, 1);
			reshuffle_hand_into_deck(i, 0);
			draw_cards(i, 7);
			set_life_total(i, 7);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_takenuma_bleeder(int player, int card, event_t event)
{
  // Whenever ~ attacks or blocks, you lose 1 life if you don't control a Demon.
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS || event == EVENT_DECLARE_BLOCKERS)
	  && !check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DEMON)
	  && (declare_attackers_trigger(player, card, event, 0, player, card)
		  || (blocking(player, card, event) && !is_humiliated(player, card))))
	lose_life(player, 1);

  return 0;
}

int card_tallowisp(int player, int card, event_t event){

	/* Tallowisp	|1|W
	 * Creature - Spirit 1/3
	 * Whenever you cast a Spirit or Arcane spell, you may search your library for an Aura card with enchant creature, reveal it, and put it into your hand. If
	 * you do, shuffle your library. */

	if( arcane_spirit_spell_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ENCHANTMENT, "Select an Aura card with enchant creature.");
		test.subtype = SUBTYPE_AURA_CREATURE;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

	return 0;
}

int card_tendo_ice_bridge(int player, int card, event_t event){

	/* ~ enters the battlefield with a charge counter on it.
	 * |T: Add |1 to your mana pool.
	 * |T, Remove a charge counter from ~: Add one mana of any color to your mana pool. */

	card_instance_t *instance = get_card_instance(player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, 1);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect_force_color(player, card, COLOR_TEST_ANY_COLORED);

		instance->info_slot = COLOR_TEST_ANY;
		instance->targets[1].player = 66;
		return 0;	// so mana_producer() doesn't play the colorless sound
	}

	if( count_counters(player, card, COUNTER_CHARGE) <= 0 && instance->targets[1].player == 66 ){
		instance->info_slot = COLOR_TEST_COLORLESS;
	}

	int colored_mana_before = 0;
	if( event == EVENT_ACTIVATE ){
		int i;
		for(i=1; i<6;i++){
			colored_mana_before+=mana_pool[(1<<i)+8*player];
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int colored_mana_after = 0;
		int i;
		for(i=1; i<6;i++){
			colored_mana_after+=mana_pool[(1<<i)+8*player];
		}
		if( colored_mana_before < colored_mana_after ){
			remove_counter(instance->parent_controller, instance->parent_card, COUNTER_CHARGE);
		}
	}

  return mana_producer(player, card, event);
}

ARCANE(card_terashis_grasp){
	// Destroy target artifact or enchantment. You gain life equal to its converted mana cost.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "DISENCHANT");
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				int amount = get_cmc(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				gain_life(player, amount);
			}
	}
	return 0;
}

int card_that_which_was_taken(int player, int card, event_t event)
{
  /* That Which Was Taken	|5
   * Legendary Artifact
   * |4, |T: Put a divinity counter on target permanent other than ~.
   * Each permanent with a divinity counter on it is indestructible. */

  check_legend_rule(player, card, event);

  if (event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_PERMANENT)
	  && count_counters(affected_card_controller, affected_card, COUNTER_DIVINITY) > 0
	  && in_play(player, card) && !is_humiliated(player, card))
	indestructible(affected_card_controller, affected_card, event);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.preferred_controller = player;
  td.special = TARGET_SPECIAL_NOT_ME;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_DIVINITY);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(4), 0, &td, "TARGET_PERMANENT");
}

int card_threads_of_disloyalty(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.special = TARGET_SPECIAL_CMC_LESSER_OR_EQUAL;
	td.extra = 2;
	td.allowed_controller = 1-player;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			ai_modifier+=15;
			if( ! select_target(player, card, &td, "Select a creature with CMC 2 or less", NULL) ){
				spell_fizzled = 1;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			return card_control_magic(player, card, event);
	}

	else{
		return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
	}
	return 0;
}

ARCANE(card_three_tragedies){
	// Target player discards three cards.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				new_multidiscard(instance->targets[0].player, 3, 0, player);
			}
	}
	return 0;
}


int card_throat_slitter(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE );
  td.allowed_controller = 1-player;
  td.preferred_controller = 1-player;
  td.illegal_color = COLOR_TEST_BLACK;

  card_instance_t *instance = get_card_instance( player, card );

  if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
	 if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
		if( valid_target(&td) ){
		   kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	 }
  }

  return ninjutsu(player, card, event, 2, 1, 0, 0, 0, 0, CARD_ID_THROAT_SLITTER);
}

int card_tomorrow_azamis_familiar(int player, int card, event_t event){

	/* Tomorrow, Azami's Familiar	|5|U
	 * Legendary Creature - Spirit 1/5
	 * If you would draw a card, look at the top three cards of your library instead. Put one of those cards into your hand and the rest on the bottom of your
	 * library in any order. */

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && !suppress_draw ){
		if( reason_for_trigger_controller == player ){
			if(event == EVENT_TRIGGER){
				event_result |= 2u;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int amount = count_deck(player) < 3 ? count_deck(player) : 3;
					if( amount ){
						test_definition_t this_test;
						default_test_definition(&this_test, TYPE_ANY);
						this_test.create_minideck = 3;
						this_test.no_shuffle = 1;
						new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
						amount--;
						if( amount > 0 ){
							put_top_x_on_bottom(player, player, amount);
						}
					}
					suppress_draw = 1;
			}
		}
	}
	return 0;
}

ARCANE_WITH_SPLICE(card_torrent_of_stone, MANACOST_X(-10))
{
  // ~ deals 4 damage to target creature.
  // Splice onto Arcane - Sacrifice two |H1Mountains.

  // (No consideration for Magical Hack; by the time it can be cast, the splice cost's already been paid.)

#define DECL_TEST(test)																\
  test_definition_t test;															\
  new_default_test_definition(&test, TYPE_LAND, "Select a Mountain to sacrifice.");	\
  test.subtype = SUBTYPE_MOUNTAIN

#define DECL_TD(td)												\
  target_definition_t td;										\
  default_target_definition(player, card, &td, TYPE_CREATURE);	\
  td.allow_cancel = 0

  if (event == EVENT_CAN_SPLICE)
	{
	  DECL_TEST(test);
	  test.qty = 2;
	  return new_can_sacrifice_as_cost(player, card, &test);
	}

  if (event == EVENT_SPLICE)
	{
	  DECL_TEST(test);
	  return (new_sacrifice(player, card, player, 0, &test)
			  && new_sacrifice(player, card, player, SAC_NO_CANCEL, &test));
	}

  if (event == EVENT_CAN_CAST)
	{
	  DECL_TD(td);
	  return can_target(&td);
	}

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  DECL_TD(td);
	  pick_target(&td, "TARGET_CREATURE");
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  DECL_TD(td);
	  if (valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
		}
	}

  return 0;
#undef DECL_TEST
#undef DECL_TD
}

int card_toshiro_umezawa(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY && affected_card_controller == 1-player ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				instance->targets[11].player++;
			}
		}
	}

	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
		affect_me(player, card )
	  ){
		if( ! graveyard_has_shroud(2) ){
			const int *grave = get_grave(player);
			int i = 0;
			int good = 0;
			while( grave[i] != -1 ){
					if( is_what(-1, grave[i], TYPE_INSTANT | TYPE_INTERRUPT) && can_play_iid(player, event, grave[i]) ){
						good = 1;
						break;
					}
					i++;
			}
			if( good ){
				if(event == EVENT_TRIGGER){
					event_result |= duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						int amount = instance->targets[11].player;
						instance->targets[11].player = 0;
						int pc = 0;
						int playable[count_graveyard(player)];
						i = 0;
						char msg[100] = "Select an instant card.";
						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_INSTANT | TYPE_INTERRUPT, msg);
						while( grave[i] != -1 ){
								if( is_what(-1, grave[i], TYPE_INSTANT | TYPE_INTERRUPT) && can_play_iid(player, event, grave[i]) ){
									playable[pc] = grave[i];
									pc++;
								}
								i++;
						}
						while( amount > 0 && pc > 0 ){
								int selected = select_card_from_zone(player, player, playable, pc, 0, AI_MAX_VALUE, -1, &this_test);
								if( selected == -1 ){
									break;
								}
								int id = cards_data[playable[selected]].id;
								if( charge_mana_from_id(player, -1, event, id) ){
									int count = count_graveyard(player)-1;
									while( count > -1 ){
										if( cards_data[grave[count]].id == id ){
											play_card_in_grave_for_free_and_exile_it(player, player, count);
											return 0;
										}
										count--;
									}
									int k;
									for(k=selected; k<pc; k++)
										playable[k] = playable[k+1];
									pc--;
									amount--;
								}
						}
				}
				else if (event == EVENT_END_TRIGGER){
						instance->targets[11].player = 0;
				}
			}
		}
	}

	return bushido(player, card, event, 1);
}

int card_umezawas_jitte(int player, int card, event_t event)
{
  /* Umezawa's Jitte	|2
   * Legendary Artifact - Equipment
   * Whenever equipped creature deals combat damage, put two charge counters on ~.
   * Remove a charge counter from ~: Choose one - Equipped creature gets +2/+2 until end of turn; or target creature gets -1/-1 until end of turn; or you gain 2
   * life.
   * Equip |2 */

  if (!is_unlocked(player, card, event, 26))
	return 0;

  check_legend_rule(player, card, event);

  // test for if combat damage was dealt
  int packets;
  if ((packets = equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE)))
	add_counters(player, card, COUNTER_CHARGE, 2 * packets);

  if (!IS_ACTIVATING(event))
	return 0;

  if (event == EVENT_CAN_ACTIVATE && !CAN_ACTIVATE0(player, card))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  int counters = count_counters(player, card, COUNTER_CHARGE);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  enum
  {
	CHOICE_EQUIP = 1,
	CHOICE_GROW = 2,
	CHOICE_SHRINK = 3,
	CHOICE_LIFE = 4,
  } choice = DIALOG(player, card, event,
					DLG_RANDOM,
					"Equip", can_activate_basic_equipment(player, card, event, 2), 1,
					"+2/+2", counters > 0 && is_equipping(player, card), 10,
					"-1/-1", counters > 0 && can_target(&td), 10,
					"Gain 2 life", counters > 0, 5);

  if (event == EVENT_CAN_ACTIVATE)
	return choice;
  else if (event == EVENT_ACTIVATE)
	{
	  instance->number_of_targets = 0;
	  switch (choice)
		{
		  case CHOICE_EQUIP:
			activate_basic_equipment(player, card, 2);
			if (player == AI && !is_equipping(player, card))
			  ai_modifier += 128;
			break;

		  case CHOICE_GROW:
		  case CHOICE_LIFE:
			if (charge_mana_for_activated_ability(player, card, MANACOST0))
			  remove_counter(player, card, COUNTER_CHARGE);
			break;

		  case CHOICE_SHRINK:
			if (charge_mana_for_activated_ability(player, card, MANACOST0) && pick_target(&td, "TARGET_CREATURE"))
			  remove_counter(player, card, COUNTER_CHARGE);
			break;
		}
	}
  else	// EVENT_RESOLVE_ACTIVATION
	switch (choice)
	  {
		case CHOICE_EQUIP:
		  resolve_activation_basic_equipment(player, card);
		  break;

		case CHOICE_GROW:
		  if (is_equipping(player, card))
			pump_until_eot(player, card, instance->targets[8].player, instance->targets[8].card, 2, 2);
		  break;

		case CHOICE_SHRINK:
		  if (valid_target(&td))
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		  else
			spell_fizzled = 1;
		  break;

		case CHOICE_LIFE:
		  gain_life(player, 2);
		  break;
	  }

  return 0;
}

ARCANE(card_unchecked_growth){
	// Target creature gets +4/+4 until end of turn. If it's a Spirit, it gains trample until end of turn.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				int key = 0;
				if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_SPIRIT) ){
					key = KEYWORD_TRAMPLE;
				}
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 4, 4, key, 0);
			}
	}
	return 0;
}

int card_waxmane_baku(int player, int card, event_t event){

	/* Whenever you cast a Spirit or Arcane spell, you may put a ki counter on ~.
	 * |1, Remove X ki counters from ~: Tap X target creatures. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		add_counter(player, card, COUNTER_KI);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && count_counters(player, card, COUNTER_KI) > 0 && can_target(&td) ){
		return has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			int amount = count_counters(player, card, COUNTER_KI);
			if( amount < target_available(player, card, &td) ){
				amount = target_available(player, card, &td);
			}
			if( player != AI ){
				amount = choose_a_number(player, "Remove how many counters?", count_counters(player, card, COUNTER_KI));
				if( amount == 0 || amount > count_counters(player, card, COUNTER_KI) ){
					spell_fizzled = 1;
				}
			}
			remove_counters(player, card, COUNTER_KI, amount);
			int i;
			for(i=amount-1; i>-1; i--){
				if( pick_target(&td, "TARGET_CREATURE") ){
					state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
					instance->targets[i] = instance->targets[0];
				}
			}
			instance->info_slot = amount;
			for(i=amount-1; i>-1; i--){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
			int i;
			for(i=instance->info_slot-1; i>-1; i--){
				if( validate_target(player, card, &td, i) ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
			}
	}

	return 0;
}

int card_yomiji_who_bars_the_way(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( affect_me(player, card) ){ return 0; }
		if( make_test_in_play(affected_card_controller, affected_card, -1, TYPE_PERMANENT, 0, SUBTYPE_LEGEND, 0, 0, 0, 0, 0, -1, 0) ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				int pos = instance->targets[11].player;
				if( pos < 10 ){
					instance->targets[pos].player = affected_card_controller;
					instance->targets[pos].card = get_id(affected_card_controller, affected_card);
					instance->targets[11].player++;
				}
			}
		}
	}

	if( instance->targets[11].player > 0 && resolve_graveyard_trigger(player, card, event) == 1 ){
		int i;
		for(i=0; i<instance->targets[11].player; i++){
			int id = instance->targets[i].card;
			const int *grave = get_grave(instance->targets[i].player);
			int count = count_graveyard(instance->targets[i].player)-1;
			while( count > -1 ){
					if( cards_data[grave[count]].id == id ){
						add_card_to_hand(instance->targets[i].player, grave[count]);
						remove_card_from_grave(instance->targets[i].player, count);
						break;
					}
					count--;
			}
		}
	}
	return 0;
}

int card_yukora_the_prisoner(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( leaves_play(player, card, event) ){
		manipulate_all(player, card, player, TYPE_CREATURE, 0, SUBTYPE_OGRE, 1, 0, 0, 0, 0, -1, 0, KILL_SACRIFICE);
	}
	return 0;
}
