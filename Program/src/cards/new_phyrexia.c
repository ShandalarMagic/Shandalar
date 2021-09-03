#include "manalink.h"

// ---- GLOBAL FUNCTIONS ----

int has_phyrexian_mana(int player, int cless, int black, int blue, int green, int red, int white, int clred_artifact){

	int manacost[5] = { black, blue, green, red, white };

	if( has_mana_multi(player, cless, black, blue, green, red, white) ){
		return 1;
	}
	else if( has_mana(player, clred_artifact ? COLOR_ARTIFACT : COLOR_COLORLESS, cless) && can_pay_life(player, 1) ){
		int life_to_pay = 50;
		int i;
		for(i=0; i<5; i++){
			if( manacost[i] < 1 ){
				life_to_pay-=10;
			}
			else{
				int z;
				for(z=4; z>-1; z--){
					if( manacost[i] < z+1 ){
						life_to_pay-=2;
					}
					else{
						if( has_mana(player, i+1, z+1) ){
							life_to_pay-=2;
						}
					}
				}
			}
		}


		if( !IS_AI(player) ){
			if( life[player] >= life_to_pay ){
				return 1;
			}
		}
		else{
			if( (life[player] - life_to_pay) > 6 ){
				return 1;
			}
		}
	}

	return 0;
}

static int has_phyrexian_mana_for_activated_ability(int player, int card, int cless, int black, int blue, int green, int red, int white)
{
  if (cless < 0)
	cless = 0;
  cless = get_cost_mod_for_activated_abilities(player, card, cless, black, blue, green, red, white);
  int rval = has_phyrexian_mana(player, cless, black, blue, green, red, white, 0);
  return rval;
}

static int charge_phyrexian_mana_impl(int player, int card, int cless, int black, int blue, int green, int red, int white, int clred_artifact){

	int manacost[5] = { black, blue, green, red, white };
	int manacost_modified[5] = { black, blue, green, red, white };
	int cpl = can_pay_life(player, 1);
	int life_to_pay = 10;

	if( ! has_mana(player, clred_artifact ? COLOR_ARTIFACT : COLOR_COLORLESS, cless) ){
		spell_fizzled = 1;
		return 0;
	}

	char buffer[500];
	if( cpl ){
		int pos = scnprintf(buffer, 500, " ");
		if (clred_artifact ? has_mana_multi_a(player, cless, black, blue, green, red, white)
			: has_mana_multi(player, cless, black, blue, green, red, white)){
			pos += scnprintf(buffer + pos, 500-pos, "Pay ", buffer );
			if( cless > 0 ){
				pos += scnprintf(buffer + pos, 500-pos, "%d", cless );
			}
			else if( cless == -1 ){
					pos += scnprintf(buffer + pos, 500-pos, "X" );
			}
			int q;
			for(q=0; q<5; q++){
				if( manacost[q] > 0 ){
					int w;
					for(w=0;w<manacost[q]; w++){
						if( q == 0 ){
							pos += scnprintf(buffer + pos, 500-pos, "B" );
						}
						if( q == 1 ){
							pos += scnprintf(buffer + pos, 500-pos, "U" );
						}
						if( q == 2 ){
							pos += scnprintf(buffer + pos, 500-pos, "G" );
						}
						if( q == 3 ){
							pos += scnprintf(buffer + pos, 500-pos, "R" );
						}
						if( q == 4 ){
							pos += scnprintf(buffer + pos, 500-pos, "W" );
						}
					}
				}
			}
			pos += scnprintf(buffer + pos, 500-pos, "\n ");
		}

		int i;
		for(i=0; i<5; i++){
			if( manacost[i] > 0 ){
				int k;
				for(k=5;k>0;k--){
					if( manacost[i] < k ){
						life_to_pay-=2;
					}
					else{
						if( k==2 ){
							pos += scnprintf(buffer + pos, 500-pos, "Pay ");
							if( cless > 0 ){
								pos += scnprintf(buffer + pos, 500-pos, "%d", cless );
							}
							else if( cless == -1 ){
									pos += scnprintf(buffer + pos, 500-pos, "X");
							}
							if( i == 0 ){
								pos += scnprintf(buffer + pos, 500-pos, "B and ");
								manacost_modified[i]--;
							}
							if( i == 1 ){
								pos += scnprintf(buffer + pos, 500-pos, "U and ");
								manacost_modified[i]--;
							}
							if( i == 2 ){
								pos += scnprintf(buffer + pos, 500-pos, "G and ");
								manacost_modified[i]--;
							}
							if( i == 3 ){
								pos += scnprintf(buffer + pos, 500-pos, "R and ");
								manacost_modified[i]--;
							}
							if( i == 4 ){
								pos += scnprintf(buffer + pos, 500-pos, "W and ");
								manacost_modified[i]--;
							}
							pos += scnprintf(buffer + pos, 500-pos, "%d life\n", life_to_pay-2);
						}
						else if( k== 1 ){
								pos += scnprintf(buffer + pos, 500-pos, "Pay ");
								if( cless > 0 ){
									pos += scnprintf(buffer + pos, 500-pos, "%d and %d life\n", cless, life_to_pay);
								}
								else if( cless == -1 ){
										pos += scnprintf(buffer + pos, 500-pos, "X and %d life\n", life_to_pay);
								}
								else{
									pos += scnprintf(buffer + pos, 500-pos, "%d life\n", life_to_pay);
								}
						}
					}
				}
			}
		}
		pos += scnprintf(buffer + pos, 500-pos, " Do nothing");
	}

	int choice = 0;
	if( cpl ){
		if (clred_artifact ? has_mana_multi_a(player, cless, black, blue, green, red, white)
			: has_mana_multi(player, cless, black, blue, green, red, white)){
			choice = do_dialog(player, player, card, -1, -1, buffer, 0);
		}
		else{
			if( life_to_pay == 4){
				if (clred_artifact ? has_mana_multi_a(player, cless, manacost_modified[0], manacost_modified[1], manacost_modified[2], manacost_modified[3], manacost_modified[4])
					: has_mana_multi(player, cless, manacost_modified[0], manacost_modified[1], manacost_modified[2], manacost_modified[3], manacost_modified[4])){
					int ai_choice = 0;
					if( life[player]-=4 > 6 ){
						ai_choice = 1;
					}
					choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
					choice++;
				}
				else{
					choice = 2;
				}
			}
			else{
				choice = 1;
			}
		}
	}

	int result = 0;
	if( choice == 0 ){
		charge_mana_multi_a(player, clred_artifact ? 0 : cless, black, blue, green, red, white, clred_artifact ? cless : 0);
		if( spell_fizzled != 1 ){
			result = 1;
		}
	}

	else if( choice == 1 ){
			if( life_to_pay > 2 ){
				charge_mana_multi_a(player, clred_artifact ? 0 : cless,
									manacost_modified[0], manacost_modified[1], manacost_modified[2], manacost_modified[3], manacost_modified[4],
									clred_artifact ? cless : 0);
			}
			else{
				charge_mana(player, clred_artifact ? COLOR_ARTIFACT : COLOR_COLORLESS, cless);
			}
			if( spell_fizzled != 1 ){
				lose_life(player, 2);
				result = 1;
			}
	 }

	else if( choice == 2 && life_to_pay == 4 ){
			charge_mana(player, clred_artifact ? COLOR_ARTIFACT : COLOR_COLORLESS, cless);
			if( spell_fizzled != 1 ){
				lose_life(player, 4);
				result = 1;
			}
	}

	else{
		 spell_fizzled = 1;
		 return 0;
	}

	 return result;
}

int charge_phyrexian_mana(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white, int clred_artifact)
{
  cless = get_updated_casting_cost(player, card, -1, event, cless);
  return charge_phyrexian_mana_impl(player, card, cless, black, blue, green, red, white, clred_artifact);
}

static int charge_phyrexian_mana_for_activated_ability(int player, int card, int cless, int black, int blue, int green, int red, int white)
{
  int x = cless < 0 ? cless : 0;
  if (x)
	cless = 0;
  cless = get_cost_mod_for_activated_abilities(player, card, cless, black, blue, green, red, white);
  if (x && !cless)
	{
	  cless = x;
	  x = 0;
	}
  return (charge_phyrexian_mana_impl(player, card, cless, black, blue, green, red, white, 0)
		  && (x == 0 || charge_mana_multi(player, MANACOST_X(x))));
}

static int phyrexian_casting_costs(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white, int clred_artifact ){

	card_instance_t *instance = get_card_instance(player, card);

	int manacost[5] = { black, blue, green, red, white };

	if( event == EVENT_MODIFY_COST && affect_me(player, card) ){
		int c1 = get_updated_casting_cost(player, card, -1, event, -1);
		int can_cast = 0;

		if (clred_artifact ? has_mana_multi_a(player, c1, black, blue, green, red, white) : has_mana_multi(player, c1, black, blue, green, red, white) ){
			can_cast = 1;
		 }
		 else if( has_mana(player, clred_artifact ? COLOR_ARTIFACT : COLOR_COLORLESS, c1) && can_pay_life(player, 1) ){
				int life_to_pay = 50;
				int i;
				for(i=0; i<5; i++){
					if( manacost[i] < 1 ){
						life_to_pay-=10;
					}
					else{
						int z;
						for(z=4; z>-1; z--){
							if( manacost[i] < z+1 ){
								life_to_pay-=2;
							}
							else{
								if( has_mana(player, i+1, z+1) ){
									life_to_pay-=2;
								}
							}
						}
					}
				}
				if( player != AI ){
					if( life[player] >= life_to_pay ){
						can_cast = 1;
					}
				}
				else{
					if( (life[player] - life_to_pay) > 6 ){
						can_cast = 1;
					}
				}

		}

		if( can_cast ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
			if( clred_artifact){
				infinite_casting_cost();
			}
		}
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( instance->info_slot == 1){
			int result = played_for_free(player, card) | is_token(player, card);
			if( ! result ){
				result = charge_phyrexian_mana(player, card, event, cless, black, blue, green, red, white, clred_artifact);
			}
			if( result == 0 ){
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}


static int generic_splicer(int player, int card, event_t event, int pow, int tou, int keyword, int golems ){

	if( comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_GOLEM, golems);
	}

	return boost_creature_type(player, card, event, SUBTYPE_GOLEM, pow, tou, keyword, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
}

static int np_shrines(int player, int card, event_t event, int clr, int act_cost){

	/* Shrine of [Something]	|[cost]
	 * Artifact
	 * At the beginning of your upkeep or whenever you cast a [clr] spell, put a charge counter on ~.
	 * |[act_cost] |T, Sacrifice ~: [do something] */

	// This shouldn't include resolution or construction of the target object - you can't factor out a function by special-casing all of its callers.

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( clr == COLOR_TEST_RED ){
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	}

	if( clr == COLOR_TEST_BLACK ){
		td.zone = TARGET_ZONE_PLAYERS;
	}

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_CHARGE);
	}

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		player == reason_for_trigger_controller && player == trigger_cause_controller
	  ){
		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) &&
			(get_color(trigger_cause_controller, trigger_cause) & clr)
		  ){
			trig = 1;
		}
		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					add_counter(player, card, COUNTER_CHARGE);
			}
		}
	}

	if (event == EVENT_CAN_ACTIVATE && (clr == COLOR_TEST_GREEN ? CAN_TAP_FOR_MANA(player, card)
										: (CAN_TAP(player, card) && CAN_ACTIVATE(player, card, MANACOST_X(act_cost))))
	   ){
		int result = 1;

		if( clr == COLOR_TEST_RED  || clr == COLOR_TEST_BLACK ){
			result = can_target(&td);
		}

		if( IS_AI(player) && count_counters(player, card, COUNTER_CHARGE) < 2 ){
			result = 0;
		}
		return result;
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, act_cost, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			if( clr == COLOR_TEST_RED ){
				if( ! pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					spell_fizzled = 1;
				}
			}
			if( clr == COLOR_TEST_BLACK ){
				if( ! pick_target(&td, "TARGET_PLAYER") ){
					spell_fizzled = 1;
				}
			}
			instance->state |= STATE_TAPPED;
			instance->info_slot = count_counters(player, card, COUNTER_CHARGE);
			if( clr == COLOR_TEST_GREEN ){
				produce_mana_tapped(player, card, COLOR_COLORLESS, instance->info_slot);
			} else {
				tap_card(player, card);
			}
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int amount = instance->info_slot;
		if( amount > 0 ){
			if( clr == COLOR_TEST_BLACK ){
				new_multidiscard(instance->targets[0].player, amount, 0, player);
			}
			if( clr == COLOR_TEST_BLUE ){
				impulse_effect(player, amount, 0);
			}
			// Green resolved at activation
			if( clr == COLOR_TEST_RED ){
				damage_creature_or_player(player, card, event, amount);
			}
			if( clr == COLOR_TEST_WHITE ){
				generate_tokens_by_id(player, card, CARD_ID_MYR, amount);
			}
		}
	}
	return 0;
}

static int is_poisoned(int player){
	if( poison_counters[player] > 0 ){
		return 1;
	}
	return 0;
}

//---- CARDS ----

int card_batterskull(int player, int card, event_t event){

	/* Batterskull	|5
	 * Artifact - Equipment
	 * Living weapon
	 * Equipped creature gets +4/+4 and has vigilance and lifelink.
	 * |3: Return ~ to its owner's hand.
	 * Equip |5 */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 5);
	}
	else if( event == EVENT_ACTIVATE ){
			int equip_cost = get_updated_equip_cost(player, card, 5);
			int choice = 0;
			if( has_mana(player, COLOR_COLORLESS, equip_cost) && check_for_equipment_targets(player, card) &&
				can_sorcery_be_played(player, event)
			  ){
				if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Equip\n Bounce Batterskull\n Do nothing", 0);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				activate_basic_equipment(player, card, 5);
				instance->targets[9].card = 66;
			}
			else if( choice == 1 ){
					charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0);
					if( spell_fizzled != 1){
						instance->targets[9].card = 67;
						instance->number_of_targets = 1;
					}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[9].card == 66 ){
				resolve_activation_basic_equipment(player, card);
			}
			else if( instance->targets[9].card == 67 ){
					bounce_permanent(player, instance->parent_card);
			}
	}

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GERM, &token);
		token.action = TOKEN_ACTION_EQUIP;
		generate_token(&token);
	}

	if ((event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS) &&
		affect_me(instance->targets[8].player, instance->targets[8].card) &&
		in_play(player, card) && !is_humiliated(player, card) && is_equipping(player, card)
	   ){
		if (event == EVENT_ABILITIES){
			vigilance(instance->targets[8].player, instance->targets[8].card, event);
			lifelink(instance->targets[8].player, instance->targets[8].card, event);
		} else {
			event_result += 4;
		}
	}

	return 0;
}


int card_blade_splicer(int player, int card, event_t event ){
	/* Blade Splicer	|2|W
	 * Creature - Human Artificer 1/1
	 * When ~ enters the battlefield, put a 3/3 colorless Golem artifact creature token onto the battlefield.
	 * Golem creatures you control have first strike. */

	return generic_splicer(player, card, event, 0, 0, KEYWORD_FIRST_STRIKE, 1);
}

int card_beast_within(int player, int card, event_t event ){
	/* Beast Within	|2|G
	 * Instant
	 * Destroy target permanent. Its controller puts a 3/3 |Sgreen Beast creature token onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 pick_target(&td, "TARGET_PERMANENT");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			 if( valid_target(&td) ){
				 kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				 token_generation_t token;
				 default_token_definition(player, card, CARD_ID_BEAST, &token);
				 token.t_player = instance->targets[0].player;
				 generate_token(&token);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_cathedral_membrane(int player, int card, event_t event ){

	phyrexian_casting_costs(player, card, event, MANACOST_XW(1, 1), 1);

	if( current_phase > PHASE_DECLARE_BLOCKERS && current_phase < PHASE_MAIN2 &&
		graveyard_from_play(player, card, event)  ){
		card_instance_t* instance = get_card_instance(player, card);
		if( instance->blocking < 255 && in_play(1-player, instance->blocking) ){
			damage_creature(1-player, instance->blocking, 6, player, card);
		}
	}

	return 0;
}

// Chancellor of the Tangle code is in "rules_engine.c" in "functions" folder

int card_deceiver_exarch(int player, int card, event_t event ){

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		int active = 0;

		if( player != AI ){
			if( can_target(&td) ){
				pick_target(&td, "TARGET_PERMANENT");
				active = 1;
			}
		}
		else{
			 td.allowed_controller = 1-player;
			 td.preferred_controller = 1-player;
			 td.illegal_state = TARGET_STATE_TAPPED;
			 if( can_target(&td) ){
				 pick_target(&td, "TARGET_PERMANENT");
				 active = 1;
			 }
			 else{
				  td.allowed_controller = player;
				  td.preferred_controller = player;
				  td.required_state = TARGET_STATE_TAPPED;
				  if( can_target(&td) ){
					  pick_target(&td, "TARGET_PERMANENT");
					  active = 1;
				  }

			 }
		}

		if( active == 1 ){
			if( instance->targets[0].player == player ){
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				 tap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return flash(player, card, event);
}

int card_dismember(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	 card_instance_t *instance = get_card_instance(player, card);

	 int manacost[5] = {2, 0, 0, 0, 0};

	 if( event == EVENT_MODIFY_COST && affect_me(player, card) ){
		 int can_cast = 0;

		 if( has_mana_multi(player, 1, 2, 0, 0, 0, 0) ){
			 can_cast = 1;
		 }
		 else if( has_mana(player, COLOR_COLORLESS, 1) ){
				  int life_to_pay = 50;
				  int i;
				  for(i=0; i<4; i++){
					  if( manacost[i] < 1 ){
						  life_to_pay-=10;
					  }
					  else{
						   int z;
						   for(z=4; z>-1; z--){
							   if( manacost[i] < z+1 ){
								   life_to_pay-=2;
							   }
							   else{
									if( has_mana(player, i+1, z+1) ){
										life_to_pay-=2;
									}
							   }
						   }
					  }
				  }


				  if( player != AI && life[player] >= life_to_pay ){
					  can_cast = 1;
				  }
				  else if( player == AI && (life[player] - life_to_pay) > 6 ){
						   can_cast = 1;
				  }

		 }

		 if( can_cast > 0){
			 COST_COLORLESS-=1;
			 COST_BLACK-=2;
		 }
	 }

	 else if( event == EVENT_CAN_CAST ){
			  return can_target(&td) ;
	 }

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 int result = played_for_free(player, card) + is_token(player, card);

			if( result < 1 ){
				result = charge_phyrexian_mana(player, card, event, MANACOST_XB(1, 2), 0);
			}

			if( result > 0 ){
				pick_target(&td, "TARGET_CREATURE");
			}
			else{
				 spell_fizzled = 1;
			}
	 }

	 else if( event == EVENT_RESOLVE_SPELL ){
			  if( valid_target(&td) ){
				  pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -5, -5);
			  }
			  kill_card(player, card, KILL_DESTROY);
	 }

	 return 0;
}

int card_dispatch(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( player == AI && metalcraft(player, card) ){
			return can_target(&td);
		}
		else if( player != AI ){
				 return can_target(&td);
		}
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 pick_target(&td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			 if( valid_target(&td) ){
				 tap_card(instance->targets[0].player, instance->targets[0].card);
				 if( metalcraft(player, card) ){
					 kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
				 }
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_elesh_norn(int player, int card, event_t event ){

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	if( in_play(player, card) && (event == EVENT_POWER || event == EVENT_TOUGHNESS) ){
		if( affect_me(player, card) ){ return 0; }

		if( affected_card_controller == player ){
			event_result+=2;
		}
		else{
			 event_result-=2;
		}
	}

	return 0;
}

int card_fresh_meat(int player, int card, event_t event){
	/* Fresh Meat	|3|G
	 * Instant
	 * Put a 3/3 |Sgreen Beast creature token onto the battlefield for each creature put into your graveyard from the battlefield this turn. */

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_BEAST, get_dead_count(player, TYPE_CREATURE));
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_geths_verdict(int player, int card, event_t event ){

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
				 impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				 lose_life(instance->targets[0].player, 1);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_gitaxian_probe(int player, int card, event_t event ){

	if( event == EVENT_MODIFY_COST ){
		return phyrexian_casting_costs(player, card, event, MANACOST_U(1), 0);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = played_for_free(player, card) + is_token(player, card);

		if( result < 1 ){
			result = charge_phyrexian_mana(player, card, event, MANACOST_U(1), 0);
		}

		if( ! result ){
			spell_fizzled = 1;
			return 0;
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
	}


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( player != instance->targets[0].player ){
				reveal_target_player_hand(instance->targets[0].player);
			}
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static const char* target_must_have_counters(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return count_counters(player, card, -1) ? NULL : "must have counters";
}
int card_hex_parasite(int player, int card, event_t event)
{
  /* Hex Parasite	|1
   * Artifact Creature - Insect 1/1
   * |X|PB: Remove up to X counters from target permanent. For each counter removed this way, ~ gets +1/+0 until end of turn. */

  if (event == EVENT_POW_BOOST)
	{
	  // An overestimate - assumes |PB is payable for free, that there's never a cost modifier, and that all permanents are targetable.
	  int mana_avail = has_mana(player, COLOR_ANY, 1);
	  int counters_avail = count_counters_by_counter_and_card_type(player, -1, TYPE_PERMANENT);
	  return MIN(mana_avail, counters_avail);
	}

  if (!IS_ACTIVATING(event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.preferred_controller = ANYBODY;
  if (IS_AI(player) && event != EVENT_RESOLVE_ACTIVATION)
	{
	  /* Prevents the AI from considering removal of counters from permanents without them (thus speculating more on which type of counter to remove from
	   * permanents that *do* have them).  However, it also prevents it from using this ability to kill a Skulking Ghost or activate another whenever-this-
	   * becomes-targeted trigger.  I think this is an acceptable tradeoff. */
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td.extra = (int)target_must_have_counters;
	}

  if (event == EVENT_CAN_ACTIVATE)
	return has_phyrexian_mana_for_activated_ability(player, card, MANACOST_XB(IS_AI(player) ? 1 : 0, 1)) && can_target(&td);

  if (event == EVENT_ACTIVATE)
	{
	  instance->number_of_targets = 0;

	  if (!pick_target(&td, "TARGET_PERMANENT"))
		return 0;

	  int old_max_x_value = max_x_value;
	  if (IS_AI(player))
		{
		  int counters = count_counters(instance->targets[0].player, instance->targets[0].card, -1);
		  max_x_value = counters;
		}
	  charge_phyrexian_mana_for_activated_ability(player, card, MANACOST_XB(-1, 1));
	  max_x_value = old_max_x_value;
	  if (cancel == 1)
		{
		  instance->number_of_targets = 0;
		  return 0;
		}

	  instance->info_slot = x_value;
	}

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  int amt = 0;
	  counter_t typ;
	  for (;
		   instance->info_slot > 0
			 && count_counters(instance->targets[0].player, instance->targets[0].card, -1)
			 && ((typ = choose_existing_counter_type(player, player, card, instance->targets[0].player, instance->targets[0].card,
													 CECT_HUMAN_CAN_CANCEL | CECT_AI_CAN_CANCEL | CECT_REMOVE | CECT_CONSIDER_ALL, -1, -1)) != COUNTER_invalid);
		   --instance->info_slot)
		{
		  remove_counter(instance->targets[0].player, instance->targets[0].card, typ);
		  ++amt;
		}

	  if (amt <= 0 || !in_play(instance->parent_controller, instance->parent_card))
		cancel = 1;
	  else
		{
		  cancel = 0;
		  pump_until_eot_merge_previous(player, card, instance->parent_controller, instance->parent_card, amt,0);
		}
	}

  return 0;
}

int card_jin_gitaxias_core_augur(int player, int card, event_t event){

	/* Jin-Gitaxias, Core Augur	|8|U|U
	 * Legendary Creature - Praetor 5/4
	 * Flash
	 * At the beginning of your end step, draw seven cards.
	 * Each opponent's maximum hand size is reduced by seven. */

	check_legend_rule(player, card, event);

	if( current_turn == player && eot_trigger(player, card, event) ){
		draw_cards(player, 7);
		if( hand_count[player] > 7 ){
			multidiscard(player, hand_count[player] - 7, 0);
		}
	}

	if( event == EVENT_MAX_HAND_SIZE && in_play(player, card) && current_turn == 1-player ){
		event_result-=7;
	}

	return flash(player, card, event);
}

static void imprint_iid_on_karn_legacy(int player, int card, int owner, int iid){
	int found = 0;
	card_instance_t *lnd;
	int p;
	for(p=0; p<2; p++){
		int c;
		for(c=active_cards_count[p]-1; c>-1; c--){
			if( is_what(p, c, TYPE_EFFECT)  ){
				lnd = get_card_instance(p, c);
				if( lnd->targets[1].card == CARD_ID_KARN_LIBERATED ){
					if( lnd->targets[0].player == player && lnd->targets[0].card == card ){
						int k;
						for(k=2; k<19; k++){
							if( lnd->targets[k].player == -1 ){
								lnd->targets[k].player = owner;
								lnd->targets[k].card = iid;
								found = 1;
								break;
							}
						}
					}
				}
			}
			if( found ){
				break;
			}
		}
		if( found ){
			break;
		}
	}
	if( ! found ){
		int legacy = create_legacy_activate(player, card, &empty);
		add_status(player, legacy, STATUS_INVISIBLE_FX);
		lnd = get_card_instance(player, legacy);
		lnd->targets[0].player = player;
		lnd->targets[0].card = card;
		lnd->number_of_targets = 1;
		lnd->targets[1].card = CARD_ID_KARN_LIBERATED;
		lnd->targets[2].player = owner;
		lnd->targets[2].card = iid;
	}
}


int card_karn_liberated(int player, int card, event_t event){

	/* Karn Liberated
	 * |7
	 * Planeswalker - Karn (6)
	 * +4: Target player exiles a card from his or her hand.
	 * -3: Exile target permanent.
	 * -14: Restart the game, leaving in exile all non-Aura permanent cards exiled with ~. Then put those cards onto the battlefield under your control. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_PERMANENT);

		enum{
			CHOICE_EXILE_CARD_IN_HAND = 1,
			CHOICE_EXILE_PERMANENT,
			CHOICE_RESTART_GAME,
		};

		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
							"Exile a card in target's hand", can_target(&td), would_validate_arbitrary_target(&td, 1-player, -1) && hand_count[1-player] ? 15 : 0, 4,
							"Exile a permanent", can_target(&td1), 10, -3,
							"Restart the game", 1, 20, -14);

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
			  case CHOICE_EXILE_CARD_IN_HAND:
					pick_target(&td, "TARGET_PLAYER");
					break;

			  case CHOICE_EXILE_PERMANENT:
					pick_target(&td1, "TARGET_PERMANENT");
					break;

			  case CHOICE_RESTART_GAME:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_EXILE_CARD_IN_HAND:
				{
					if( valid_target(&td)){
						if( hand_count[instance->targets[0].player] ){
							int selected = -1;
							test_definition_t this_test;
							if( instance->targets[0].player == HUMAN ){
								default_test_definition(&this_test, TYPE_ANY);
								selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
							}
							else{
								default_test_definition(&this_test, TYPE_PERMANENT);
								this_test.type_flag = DOESNT_MATCH;
								selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
								if( selected == -1 ){
									default_test_definition(&this_test, TYPE_ANY);
									selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
								}
							}
							int owner = get_owner(instance->targets[0].player, selected);
							int iid = get_original_internal_card_id(instance->targets[0].player, selected);
							if( instance->parent_controller != instance->targets[0].player || player == AI ){
								reveal_card(instance->parent_controller, instance->parent_card, instance->targets[0].player, selected);
							}
							rfg_card_in_hand(instance->targets[0].player, selected);
							imprint_iid_on_karn_legacy(instance->parent_controller, instance->parent_card, owner, iid);
							create_card_name_legacy(instance->parent_controller, instance->parent_card, cards_data[iid].id);
						}
					}
				}
				break;

			case CHOICE_EXILE_PERMANENT:
				{
					if( valid_target(&td1)){
						if( ! is_token(instance->targets[0].player, instance->targets[0].card) ){
							int owner = get_owner(instance->targets[0].player, instance->targets[0].card);
							int iid = get_original_internal_card_id(instance->targets[0].player, instance->targets[0].card);
							imprint_iid_on_karn_legacy(instance->parent_controller, instance->parent_card, owner, iid);
							create_card_name_legacy(instance->parent_controller, instance->parent_card, cards_data[iid].id);
						}
						kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
					}
				}
				break;

			case CHOICE_RESTART_GAME:
				{
					// approximation for "restart the game"
					int exiled_by_karn[2][500];
					int ekc[2] = {0, 0};
					card_instance_t *lnd;
					int p;
					for(p=0; p<2; p++){
						int c;
						for(c=active_cards_count[p]-1; c>-1; c--){
							if( is_what(p, c, TYPE_EFFECT)  ){
								lnd = get_card_instance(p, c);
								if( lnd->targets[1].card == CARD_ID_KARN_LIBERATED ){
									if( lnd->targets[0].player == instance->parent_controller && lnd->targets[0].card == instance->parent_card ){
										int k;
										for(k=2; k<19; k++){
											if( lnd->targets[k].player != -1 ){
												if( check_rfg(lnd->targets[k].player, cards_data[lnd->targets[k].card].id) ){
													exiled_by_karn[lnd->targets[k].player][ekc[lnd->targets[k].player]] = lnd->targets[k].card;
													ekc[lnd->targets[k].player]++;
													remove_card_from_rfg(lnd->targets[k].player, cards_data[lnd->targets[k].card].id);
												}
											}
											else{
												break;
											}
										}
										kill_card(p, c, KILL_REMOVE);
									}
								}
							}
						}
					}
					reshuffle_all_into_deck(player, 3);
					reshuffle_all_into_deck(1-player, 3);
					life[player] = get_starting_life_total(player);
					life[1-player] = get_starting_life_total(1-player);

					for(p=0; p<2; p++){
						int c;
						for(c=0; c<ekc[p]; c++){
							int iid = exiled_by_karn[p][c];
							if( ! is_what(-1, iid, TYPE_PERMANENT) || has_subtype_by_id(cards_data[iid].id, SUBTYPE_AURA) ){
								int card_added = add_card_to_hand(p, iid);
								put_on_top_of_deck(p, card_added);
								exiled_by_karn[p][c] = -1;
							}
						}
					}
					shuffle(player);
					shuffle(1-player);
					draw_cards(player, 7);
					draw_cards(1-player, 7);
					poison_counters[HUMAN] = 0;
					poison_counters[AI] = 0;

					for(p=0; p<2; p++){
						int c;
						for(c=0; c<ekc[p]; c++){
							int iid = exiled_by_karn[p][c];
							if( iid != -1 ){
								int card_added = add_card_to_hand(instance->parent_controller, iid);
								if( instance->parent_controller != p ){
									get_card_instance(player, card_added)->state ^= STATE_OWNED_BY_OPPONENT;
								}
								put_into_play(player, card_added);
							}
						}
					}
				}
				break;
		  }
	}

	return planeswalker(player, card, event, 6);
}


int card_lashwrithe(int player, int card, event_t event){

	/* Lashwrithe	|4
	 * Artifact - Equipment
	 * Living weapon
	 * Equipped creature gets +1/+1 for each |H2Swamp you control.
	 * Equip |PB|PB */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_sorcery_be_played(player, event) ){
		int c1 = get_updated_equip_cost(player, card, get_cost_mod_for_activated_abilities(player, card, 0, 2, 0, 0, 0, 0));
		if( has_phyrexian_mana( player, MANACOST_XB(c1, 2), 0) && check_for_equipment_targets(player, card) ){
			return 1;
		}
		if( metalcraft(player, card) && check_battlefield_for_id(player, CARD_ID_PURESTEEL_PALADIN) ){
			return check_for_equipment_targets(player, card);
		}
	}
	else if( event == EVENT_ACTIVATE ){
			int c1 = get_updated_equip_cost(player, card, get_cost_mod_for_activated_abilities(player, card, 0, 2, 0, 0, 0, 0));
			int good = 0;
			if( metalcraft(player, card) && check_battlefield_for_id(player, CARD_ID_PURESTEEL_PALADIN) ){
				good = 1;
			}
			if( good != 1 ){
				charge_phyrexian_mana( player, card, event, MANACOST_XB(c1, 2), 0);
			}
			if( spell_fizzled != 1 ){
				activate_basic_equipment(player, card, -2);
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			resolve_activation_basic_equipment(player, card);
	}

	if( is_equipping(player, card) ){
		if( (event== EVENT_POWER || event == EVENT_TOUGHNESS) &&
			affect_me(instance->targets[8].player, instance->targets[8].card)
		  ){
			event_result+=count_subtype(player, TYPE_PERMANENT, SUBTYPE_SWAMP);
		}
	}

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GERM, &token);
		token.action = TOKEN_ACTION_EQUIP;
		generate_token(&token);
	}

	return 0;
}

int card_master_splicer(int player, int card, event_t event ){
	/* Master Splicer	|3|W
	 * Creature - Human Artificer 1/1
	 * When ~ enters the battlefield, put a 3/3 colorless Golem artifact creature token onto the battlefield.
	 * Golem creatures you control get +1/+1. */

	return generic_splicer(player, card, event, 1, 1, 0, 1);
}

int card_melira_sylvok_outcast(int player, int card, event_t event ){

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	// Deal with Infect
	if(event == EVENT_ABILITIES &&
	   affected_card_controller == 1-player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
	   !is_humiliated(player, card)
	  ){
		event_result &= ~KEYWORD_INFECT;
	}

	// Deal with Poison Counters
	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[9].player = poison_counters[player];
	}

	if( instance->targets[9].player > -1 && poison_counters[player] != instance->targets[9].player  ){
		if( poison_counters[player] > instance->targets[9].player && !is_humiliated(player, card) ){
			poison_counters[player] = instance->targets[9].player;
		}
		else{
			 instance->targets[9].player = poison_counters[player];
		}
	}

	// The interaction with -1/-1 counters is coded in counters.c

	return 0;
}

static const char* cmc_is_1(int who_chooses, int player, int card){
	if( get_cmc(player, card) == 1 ){
		return NULL;
	}
	return "must be a spell with CMC 1";
}

int card_mental_misstep(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		return phyrexian_casting_costs(player, card, event, MANACOST_U(1), 0);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ANY);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)cmc_is_1;

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, &td, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( instance->info_slot == 1 && ! played_for_free(player, card) && ! is_token(player, card) ){
			int choice = 0;
			if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
				choice = do_dialog(player, player, card, -1, -1, " Pay 2 life for Phyrexian Mana\n Play normally\n Cancel", life[player] > 7 ? 0 : 1);
			}
			if( choice == 0 ){
				int c1 = get_updated_casting_cost(player, card, -1, event, 0);
				charge_mana(player, COLOR_COLORLESS, c1);
				if( spell_fizzled != 1 ){
					lose_life(player, 2);
				}
			}
			else if( choice == 1 ){
					int c1 = get_updated_casting_cost(player, card, -1, event, 0);
					charge_mana_multi(player, MANACOST_XU(c1, 1));
			}
			else{
				spell_fizzled = 1;
			}
		}
		if( spell_fizzled != 1 ){
			return counterspell(player, card, event, &td, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		return counterspell(player, card, event, &td, 0);
	}

	return 0;
}

int card_immolating_souleater(int player, int card, event_t event);
int card_moltensteel_dragon(int player, int card, event_t event)
{
  /* Moltensteel Dragon	|4|PR|PR
   * Artifact Creature - Dragon 4/4
   * Flying
   * |PR: ~ gets +1/+0 until end of turn. */

  phyrexian_casting_costs(player, card, event, MANACOST_XR(4, 2), 1);

  return card_immolating_souleater(player, card, event);
}

int card_norns_annex(int player, int card, event_t event ){

	phyrexian_casting_costs(player, card, event, 3, 0, 0, 0, 0, 2, 1);

	tax_attack(player, card, event);

	return 0;
}

int card_noxious_revival(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	char msg[100] = "Select a card to put on top.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, msg);

	card_instance_t *instance = get_card_instance(player, card);

	int rval = phyrexian_casting_costs(player, card, event, MANACOST_G(1), 0);

	if (event == EVENT_CAN_CAST){
		return rval && (count_graveyard(player) > 0 || count_graveyard(1-player) > 0) && !graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if (spell_fizzled != 1){	// may have been set by phyrexian_casting_costs()
			instance->targets[0].player = player;
			instance->targets[0].card = -1;
			if( count_graveyard(player) ){
				if( count_graveyard(1-player) && player != AI ){
					pick_target(&td, "TARGET_PLAYER");
					instance->number_of_targets = 0;
				}
			}
			else{
				instance->targets[0].player = 1-player;
			}
			int ai_mode = instance->targets[0].player == player ? AI_MAX_VALUE : AI_MIN_VALUE;
			if( spell_fizzled != 1 ){
				if( new_select_target_from_grave(player, card, instance->targets[0].player, 0, ai_mode, &this_test, 1) == -1 ){
					spell_fizzled = 1;
				}
			}
		}
	}

	else if( event == EVENT_RESOLVE_SPELL){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
			if( selected != -1 ){
				from_graveyard_to_deck(instance->targets[0].player, selected, 1);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_phyrexian_metamorph(int player, int card, event_t event)
{
  /* Phyrexian Metamorph	|3|PU
   * Artifact Creature - Shapeshifter 0/0
   * You may have ~ enter the battlefield as a copy of any artifact or creature on the battlefield, except it's an artifact in addition to its other types. */

  phyrexian_casting_costs(player, card, event, MANACOST_XU(3,1), 1);

  target_definition_t td;
  if (event == EVENT_RESOLVE_SPELL)
	base_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ARTIFACT);

  enters_the_battlefield_as_copy_of_any(player, card, event, &td, "SELECT_AN_ARTIFACT_OR_CREATURE");
  // type artifact added in cloning()

  return 0;
}

int card_phyrexian_obliterator(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	// An approximation - tells the AI that anything with at least 1 power that blocks this or is blocked by this will be destroyed.
	if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card)){
		instance->destroys_if_blocked |= DIFB_ASK_CARD;
	}
	if (event == EVENT_CHECK_DESTROY_IF_BLOCKED && affect_me(player, card) && !is_humiliated(player, card)
		&& get_power(attacking_card_controller, attacking_card) >= 1){
		event_result |= 1;
	}

	if( event == EVENT_DEAL_DAMAGE){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && instance->info_slot != current_phase ){
			if( damage->damage_target_card == card && damage->damage_target_player == player && damage->info_slot > 0 ){

				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_PERMANENT);
				td.allowed_controller = damage->damage_source_player;
				td.preferred_controller = damage->damage_source_player;
				td.who_chooses = damage->damage_source_player;
				td.illegal_abilities = 0;
				td.allow_cancel = 0;

				int i;
				for( i=0;i<damage->info_slot;i++){
					if( can_target(&td) ){
						pick_target(&td, "TARGET_PERMANENT");
						kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
					}
				}
				instance->info_slot = current_phase;
			}
		}
	}
	else if( instance->info_slot != -1 && eot_trigger(player, card, event) ){
			 instance->info_slot = -1;
	}


	return 0;
}

int card_phyrexian_swarmlord(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_INSECT, &token);
		token.legacy = 1;
		token.special_code_for_legacy = &empty;
		token.special_infos = 67;
		token.qty = poison_counters[1-player];
		generate_token(&token);
	}

	return 0;
}

int card_phyrexian_unlife(int player, int card, event_t event)
{
  cannot_lose_the_game_for_having_less_than_0_life(player, card, event, player);

  card_instance_t* damage = damage_being_dealt(event), *instance;
  if (damage
	  && damage->damage_target_card == -1 && damage->damage_target_player == player
	  && (instance = in_play(player, card))
	  && instance->targets[5].player > 0)
	damage->regen_status |= KEYWORD_INFECT;

  return global_enchantment(player, card, event);
}

int card_postmortem_lunge(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	char msg[100] = "Select a creature card";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);
	this_test.has_mana_to_pay_cmc = 1;

	card_instance_t *instance= get_card_instance(player, card);

	int rval = phyrexian_casting_costs(player, card, event, MANACOST_B(1), 0);

	if (event == EVENT_CAN_CAST){
		return rval && new_special_count_grave(player, &this_test) && !graveyard_has_shroud(player)
			&& !check_battlefield_for_id(ANYBODY, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if (spell_fizzled != 1){	// may have been set by phyrexian_casting_costs()
			// Will also store the grave[selected] value in instance->targets[0].card
			int selected = new_select_target_from_grave(player, card, player, 0, AI_MAX_CMC, &this_test, 0);
			if( selected != -1 ){
				charge_mana(player, COLOR_COLORLESS, get_cmc_by_internal_id(instance->targets[0].card));
			}
		}
	}

	else if( event == EVENT_RESOLVE_SPELL){
			int selected = validate_target_from_grave(player, card, player, 0);
			if( selected != -1 ){
				reanimate_permanent(player, card, player, selected, REANIMATE_HASTE_AND_EXILE_AT_EOT);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_puresteel_paladin(int player, int card, event_t event ){

   if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
	   reason_for_trigger_controller == player){

	   int trig = 0;

	   if( trigger_cause_controller == player && has_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_EQUIPMENT) ){
		   trig = 1;
	   }

	   if( trig > 0 ){
		  if(event == EVENT_TRIGGER){
			 event_result |= 1+player;
		  }
		  else if( event == EVENT_RESOLVE_TRIGGER){
				   draw_cards(player, 1);
		  }
	   }
	}

	return 0;
}

int card_sheoldred_whispering_one(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( current_turn == player && count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(2) ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			this_test.ai_selection_mode = AI_MAX_CMC;

			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
		}
		if( current_turn != player ){
			impose_sacrifice(player, card, current_turn, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}

int card_birthing_pod(int player, int card, event_t event){

	/* Birthing Pod	|3|PG
	 * Artifact
	 * |1|PG, |T, Sacrifice a creature: Search your library for a creature card with converted mana cost equal to 1 plus the sacrificed creature's converted
	 * mana cost, put that card onto the battlefield, then shuffle your library. Activate this ability only any time you could cast a sorcery. */

	phyrexian_casting_costs(player, card, event, MANACOST_XG(3, 1), 1);

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_ACTIVATE){
		return can_use_activated_abilities(player, card) && CAN_TAP(player, card) && has_phyrexian_mana_for_activated_ability(player, card, MANACOST_XG(1,1)) &&
		  can_target(&td) && can_sorcery_be_played(player, event);
	}

	else if( event == EVENT_ACTIVATE){
			if (charge_phyrexian_mana_for_activated_ability(player, card, MANACOST_XG(1,1))){
				int trg = pick_creature_for_sacrifice(player, card, 0);
				if( trg != -1 ){
					int amount = get_cmc(player, trg)+1;
					instance->targets[1].card = amount;
					kill_card(player, trg, KILL_SACRIFICE);
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int amount = instance->targets[1].card;
			global_tutor(player, player, 1, TUTOR_PLAY, 0, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, amount, 0);
	}

	return 0;
}

int card_shrine_of_boundless_growth(int player, int card, event_t event){

	/* Shrine of Boundless Growth	|3
	 * Artifact
	 * At the beginning of your upkeep or whenever you cast a |Sgreen spell, put a charge counter on ~.
	 * |T, Sacrifice ~: Add |1 to your mana pool for each charge counter on ~. */

	if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_TAP_FOR_MANA(player, card)){
		int counters = count_counters(player, card, COUNTER_CHARGE);
		if (counters > 0){
			declare_mana_available(player, COLOR_COLORLESS, counters);
		}
	}

	return np_shrines(player, card, event, COLOR_TEST_GREEN, 0);
}

int card_shrine_of_burning_rage(int player, int card, event_t event ){
	/* Shrine of Burning Rage	|2
	 * Artifact
	 * At the beginning of your upkeep or whenever you cast a |Sred spell, put a charge counter on ~.
	 * |3, |T, Sacrifice ~: ~ deals damage equal to the number of charge counters on it to target creature or player. */
	return np_shrines(player, card, event, COLOR_TEST_RED, 3);
}

int card_shrine_of_limitless_power(int player, int card, event_t event ){
	/* Shrine of Limitless Power	|3
	 * Artifact
	 * At the beginning of your upkeep or whenever you cast a |Sblack spell, put a charge counter on ~.
	 * |4, |T, Sacrifice ~: Target player discards a card for each charge counter on ~. */
	return np_shrines(player, card, event, COLOR_TEST_BLACK, 4);
}

int card_shrine_of_loyal_legions(int player, int card, event_t event ){
	/* Shrine of Loyal Legions	|2
	 * Artifact
	 * At the beginning of your upkeep or whenever you cast a |Swhite spell, put a charge counter on ~.
	 * |3, |T, Sacrifice ~: Put a 1/1 colorless Myr artifact creature token onto the battlefield for each charge counter on ~. */
	return np_shrines(player, card, event, COLOR_TEST_WHITE, 3);
}

int card_shrine_of_piercing_vision(int player, int card, event_t event ){
	/* Shrine of Piercing Vision	|2
	 * Artifact
	 * At the beginning of your upkeep or whenever you cast a |Sblue spell, put a charge counter on ~.
	 * |T, Sacrifice ~: Look at the top X cards of your library, where X is the number of charge counters on ~. Put one of those cards into your hand and the
	 * rest on the bottom of your library in any order. */
	return np_shrines(player, card, event, COLOR_TEST_BLUE, 0);
}

int card_spined_thopter(int player, int card, event_t event ){

	phyrexian_casting_costs(player, card, event, 2, 0, 1, 0, 0, 0, 1);

	return 0;
}

int card_sword_of_war_and_peace(int player, int card, event_t event)
{
  /* Sword of War and Peace	|3
   * Artifact - Equipment
   * Equipped creature gets +2/+2 and has protection from |Sred and from |Swhite.
   * Whenever equipped creature deals combat damage to a player, ~ deals damage to that player equal to the number of cards in his or her hand and you gain 1
   * life for each card in your hand.
   * Equip |2 */

  if (event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card))
		{
		  if (event == EVENT_ABILITIES)
			event_result |= get_sleighted_protection(player, card, KEYWORD_PROT_RED | KEYWORD_PROT_WHITE);
		  else
			event_result += 2;
		}
	}

  if (equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRACE_DAMAGED_PLAYERS))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
	  instance->targets[1].player = 0;
	  int p;
	  for (p = 0; p <= 1; ++p)
		for (; times_damaged[p] > 0; --times_damaged[p])
		  {
			damage_player(p, hand_count[p], player, card);
			gain_life(player, hand_count[player]);
		  }
	}

	return basic_equipment(player, card, event, 2);
}

int card_surgical_extraction(int player, int card, event_t event){

	/* Surgical Extraction	|PB
	 * Instant
	 * Choose target card in a graveyard other than a basic land card. Search its owner's graveyard, hand, and library for any number of cards with the same
	 * name as that card and exile them. Then that player shuffles his or her library. */

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST && affect_me(player, card) && can_pay_life(player, 1) ){
		 int can_cast = 0;

		 if( has_mana(player, COLOR_BLACK, 1) ){
			 can_cast = 1;
		 }
		 else{
				  if( player != AI && life[player] >= 2 ){
					  can_cast = 1;
				  }
				  else if( player == AI && (life[player] - 2) > 6 ){
						   can_cast = 1;
				  }

		 }

		 if( can_cast > 0){
			 COST_BLACK-=1;
		 }
	}

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	test_definition_t this_test;
	new_default_test_definition(&this_test, 0, "Select a card other than a basic land.");
	this_test.subtype = SUBTYPE_BASIC;
	this_test.subtype_flag = 1;

	if( event == EVENT_CAN_CAST ){
			if( new_special_count_grave(player, &this_test) > 0 || new_special_count_grave(1-player, &this_test) > 0 ){
				return ! graveyard_has_shroud(2);
			}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = played_for_free(player, card) + is_token(player, card);
			if( result < 1 ){
				result = charge_phyrexian_mana(player, card, event, MANACOST_B(1), 0);
			}

			if( result > 0 ){
				instance->targets[0].player = 1-player;
				if( new_special_count_grave(1-player, &this_test) > 0 ){
					if( new_special_count_grave(player, &this_test) > 0 ){
						pick_target(&td, "TARGET_PLAYER");
					}
				}
				else{
					instance->targets[0].player = player;
				}
				int selected = new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_VALUE, &this_test, 1);
				if( selected == -1 ){
					spell_fizzled = 1;
				}
			}
			else{
				 spell_fizzled = 1;
			}
	}

	if( event == EVENT_RESOLVE_SPELL){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
			if( selected != -1 ){
				const int *grave = get_grave(instance->targets[0].player);
				int id = cards_data[grave[selected]].id;
				lobotomy_effect(player, instance->targets[0].player, id, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_tezzerets_gambit(int player, int card, event_t event ){

	 if( event == EVENT_MODIFY_COST && affect_me(player, card) && can_pay_life(player, 1) ){
		 int can_cast = 0;

		 if( has_mana_multi(player, 3, 0, 1, 0, 0, 0) ){
			 can_cast = 1;
		 }
		 else if( has_mana(player, COLOR_COLORLESS, 3) ){

				  if( player != AI && life[player] >= 2 ){
					  can_cast = 1;
				  }
				  else if( player == AI && (life[player] - 2) > 6 ){
						   can_cast = 1;
				  }

		 }

		 if( can_cast > 0){
			 COST_COLORLESS-=3;
			 COST_BLUE-=1;
		 }
	 }

	 if( event == EVENT_CAN_CAST ){
			  return 1;
	 }

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = played_for_free(player, card) + is_token(player, card);
			if( result < 1 ){
				result = charge_phyrexian_mana(player, card, event, MANACOST_XU(3, 1), 0);
			}

			if( result < 1 ){
				spell_fizzled = 1;
			}
	 }

	if( event == EVENT_RESOLVE_SPELL ){
			 draw_cards(player, 2);
			 proliferate(player, card);
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_urabrask_the_hidden(int player, int card, event_t event){

	/* Urabrask the Hidden	|3|R|R
	 * Legendary Creature - Praetor 4/4
	 * Creatures you control have haste.
	 * Creatures your opponents control enter the battlefield tapped. */

	check_legend_rule(player, card, event);

	boost_subtype(player, card, event, -1, 0,0, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

	permanents_enters_battlefield_tapped(player, card, event, 1-player, TYPE_CREATURE, NULL);

	return 0;
}

int card_vault_skirge(int player, int card, event_t event ){

	phyrexian_casting_costs(player, card, event, 1, 1, 0, 0, 0, 0, 1);

	lifelink(player, card, event);

	return 0;
}

int card_vital_splicer(int player, int card, event_t event){
	/* Vital Splicer	|3|G
	 * Creature - Human Artificer 1/1
	 * When ~ enters the battlefield, put a 3/3 colorless Golem artifact creature token onto the battlefield.
	 * |1: Regenerate target Golem you control. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_GOLEM;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_REGENERATION, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_REGENERATION, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target( instance->targets[0].player, instance->targets[0].card );
		}
	}

	return generic_splicer(player, card, event, 0, 0, 0, 1);
}

int card_volt_charge(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 instance->info_slot = 3;
			 pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			 if( valid_target(&td) ){
				 damage_target0(player, card, 3);
				 proliferate(player, card);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_vorinclex_voice_of_hunger(int player, int card, event_t event ){

	check_legend_rule(player, card, event);

	if( in_play(player, card) && event == EVENT_TAP_CARD && tapped_for_mana_color >= 0 && is_what(affected_card_controller, affected_card, TYPE_LAND) ){
		if( affected_card_controller == player ){
			card_mana_flare(affected_card_controller, affected_card, EVENT_TAP_CARD);
		}
		else{
			does_not_untap_effect(player, card, affected_card_controller, affected_card, 0, 1);
		}
	}

	if( event == EVENT_COUNT_MANA && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_LAND) &&
		! is_tapped(affected_card_controller, affected_card)
	  ){
		mana_producer(affected_card_controller, affected_card, EVENT_COUNT_MANA);
	}

	return 0;
}

int card_wing_splicer(int player, int card, event_t event ){
	/* Wing Splicer	|3|U
	 * Creature - Human Artificer 1/1
	 * When ~ enters the battlefield, put a 3/3 colorless Golem artifact creature token onto the battlefield.
	 * Golem creatures you control have flying. */

	return generic_splicer(player, card, event, 0, 0, KEYWORD_FLYING, 1);
}

int card_act_of_aggression(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;
	td.allow_cancel = 0;

	if( event == EVENT_MODIFY_COST && affect_me(player, card) ){
		if( has_phyrexian_mana(player, MANACOST_XR(3, 2), 0) ){
			COST_COLORLESS-=3;
			COST_RED-=2;
		}
	}

	else if( event == EVENT_CAN_CAST ){
			 return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = played_for_free(player, card) + is_token(player, card);
			if( result < 1 ){
				result = charge_phyrexian_mana(player, card, event, MANACOST_XR(3, 2), 0);
			}

			if( result < 1 ){
				spell_fizzled = 1;
			}
			else{
				pick_target(&td, "TARGET_CREATURE");
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			card_instance_t *instance = get_card_instance(player, card);
			if( valid_target(&td) ){
				effect_act_of_treason( player, card, instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_apostles_blessing(int player, int card, event_t event){

	/* Apostle's Blessing	|1|PW
	 * Instant
	 * Target artifact or creature you control gains protection from artifacts or from the color of your choice until end of turn. */

	if( event == EVENT_MODIFY_COST && affect_me(player, card) ){
		if( has_phyrexian_mana(player, MANACOST_XW(1, 1), 0)){
			null_casting_cost(player, card);
		}
	}

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ARTIFACT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = played_for_free(player, card) + is_token(player, card);
		if( result < 1 ){
			result = charge_phyrexian_mana(player, card, event, MANACOST_XW(1, 1), 0);
		}

		if( result < 1 ){
			spell_fizzled = 1;
		}
		else{
			new_pick_target(&td, "Select target artifact or creature you control", 0, 1 | GS_LITERAL_PROMPT);
			int choice = do_dialog(player, player, card, -1, -1,
									" Prot. from Black\n Prot. from Blue\n Prot. from Green\n Prot from Red\n Prot. from White\n Prot from Artifacts",
									get_deck_color(player, 1-player) - 1);
			instance->targets[1].card = (1 << (11+choice)) | KEYWORD_RECALC_SET_COLOR;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, instance->targets[1].card, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int arm_with_aether_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->info_slot > 0){
			if( damage->damage_source_player == player &&
				damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER) ){
				if( instance->targets[1].player < 2 ){
					instance->targets[1].player = 2;
				}
				int pos = instance->targets[1].player;
				if( pos < 10 ){
					instance->targets[pos].card = damage->damage_source_card;
					instance->targets[1].player++;
				}
			}
		}
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player && instance->targets[1].player > 2 ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=2; i<instance->targets[1].player; i++){
					target_definition_t td;
					default_target_definition(player, card, &td, TYPE_CREATURE );
					td.allowed_controller = 1-player;
					td.preferred_controller = 1-player;
					td.illegal_abilities = get_protections_from(player, instance->targets[i].card);
					if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
						bounce_permanent(instance->targets[0].player, instance->targets[0].card);
						instance->number_of_targets = 1;
					}
				}
				instance->targets[0].player = 2;
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_arm_with_aether(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			create_legacy_effect(player, card, &arm_with_aether_legacy);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_artillerize(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	if( event == EVENT_CAN_CAST && can_target(&td1) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE | TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( controller_sacrifices_a_permanent(player, card, TYPE_CREATURE | TYPE_ARTIFACT, 0) ){
				pick_target(&td1, "TARGET_CREATURE_OR_PLAYER");
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				damage_target0(player, card, 5);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_auriok_survivors(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && count_graveyard_by_subtype(player, SUBTYPE_EQUIPMENT) > 0 ){
		int equip = global_tutor(player, player, 2, TUTOR_PLAY, 0, 2, TYPE_ARTIFACT, 0, SUBTYPE_EQUIPMENT, 0, 0, 0, 0, 0, -1, 0);
		equip_target_creature(player, equip, player, card);
	}

	return 0;
}

int card_blind_zealot(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	intimidate(player, card, event);

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		int choice = do_dialog(player, player, card, -1, -1, " Sac and kill creature\n Pass", 0 );
		if( choice == 0 && pick_target(&td, "TARGET_CREATURE") ){
			kill_card(player, card, KILL_SACRIFICE);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_blinding_souleater(int player, int card, event_t event){

	/* Blinding Souleater	|3
	 * Artifact Creature - Cleric 1/3
	 * |PW, |T: Tap target creature. */

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_ACTIVATE){
		return can_use_activated_abilities(player, card) && CAN_TAP(player, card) && has_phyrexian_mana_for_activated_ability(player, card, MANACOST_W(1)) &&
			can_target(&td);
	}
	else if (event == EVENT_ACTIVATE){
		if (charge_phyrexian_mana_for_activated_ability(player, card, MANACOST_W(1)) && pick_target(&td, "TARGET_CREATURE")){
			tap_card(player, card);
			instance->number_of_targets = 1;
		}
	}
	else if (event == EVENT_RESOLVE_ACTIVATION){
		if (valid_target(&td)){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_brutalizer_exarch(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_CREATURE;
	if( player == AI ){
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		int choice = 0;
		if( can_target(&td) ){
			choice = do_dialog(player, player, card, -1, -1, " Tutor Creature\n Remove permanent", 1 );
		}
		if( choice == 0 ){
			global_tutor(player, player, 1, TUTOR_DECK, 0, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		else{
			if( pick_target(&td, "TARGET_PERMANENT") ){
				put_on_bottom_of_deck(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_caged_sun(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int clr = choose_a_color_and_show_legacy(player, card, player, -1);
		instance->targets[9].player = clr;
		instance->targets[9].card = 1<<clr;
	}

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		// do the mana part
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_LAND) ){
			card_data_t* card_d = get_card_data(affected_card_controller, affected_card);
			// I can't think of any land abilities that produce mana but don't tap.
			if( event == EVENT_TAP_CARD
				&& instance->targets[9].player >= 0
				&& (tapped_for_mana_color == instance->targets[9].player
					|| (tapped_for_mana_color == 0x100 && tapped_for_mana[instance->targets[9].player] > 0))
				){
				produce_mana(affected_card_controller, instance->targets[9].player, 1);
			}
			else if (event == EVENT_COUNT_MANA && (card_d->color & instance->targets[9].card) ){
				if( ! is_tapped(affected_card_controller, affected_card) && !is_animated_and_sick(affected_card_controller, affected_card)
					&& can_produce_mana(affected_card_controller, affected_card) ){
					declare_mana_available(affected_card_controller, instance->targets[9].player, 1);
				}
			}
		}

		// do the creature part
		if( affected_card_controller == player && (event == EVENT_POWER || event == EVENT_TOUGHNESS) ){
			if( get_color(affected_card_controller, affected_card) & instance->targets[9].card ){
				event_result++;
			}
		}
	}

	return 0;
}

int card_caress_of_phyrexia(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 if( player != AI ){
				pick_target(&td, "TARGET_PLAYER");
			}
			else{
				 instance->targets[0].player = player;
				 if(life[player] < 7 || poison_counters[player] > 5 || count_deck(player) < 20 || life[1-player] < 4 ||
					poison_counters[1-player] > 6 || count_deck(1-player) < 4 ){
					instance->targets[0].player = 1-player;
				}
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				draw_cards(instance->targets[0].player, 3);
				lose_life(instance->targets[0].player, 3);
				poison_counters[instance->targets[0].player]+=3;
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_chained_throatseeker(int player, int card, event_t event)
{
  if (event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && !is_poisoned(1-player))
	event_result = 1;

  return 0;
}

int card_coa_special_effect(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		reason_for_trigger_controller == player && trigger_cause_controller == 1-player){

		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			trig = 1;
		}


		if( trig == 1 ){
			if( event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					 charge_mana(1-player, COLOR_COLORLESS, 1);
					 if( spell_fizzled == 1 ){
						kill_card(trigger_cause_controller, trigger_cause, KILL_SACRIFICE);
					 }
					 if( get_special_infos(player, card) == 66 ){
						kill_card(player, card, KILL_SACRIFICE);
					 }
			}
		}
	}
	return 0;
}

int card_chancellor_of_the_annex(int player, int card, event_t event){
	return card_coa_special_effect(player, card, event);
}

int card_chancellor_of_the_forge(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int amount = count_permanents_by_type(player, TYPE_CREATURE);
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GOBLIN, &token);
		token.qty = amount;
		token.s_key_plus = SP_KEYWORD_HASTE;
		generate_token(&token);
	}

	return 0;
}

int card_chancellor_of_the_spires(int player, int card, event_t event){

	/* Chancellor of the Spires	|4|U|U|U
	 * Creature - Sphinx 5/7
	 * You may reveal this card from your opening hand. If you do, at the beginning of the first upkeep, each opponent puts the top seven cards of his or her
	 * library into his or her graveyard.
	 * Flying
	 * When ~ enters the battlefield, you may cast target instant or sorcery card from an opponent's graveyard without paying its mana cost. */

	if( comes_into_play(player, card, event) && !graveyard_has_shroud(1-player) ){
		const int *grave = get_grave(1-player);
		int pa[count_graveyard(1-player)];
		int pac = 0;
		int i = 0;
		while( grave[i] != -1 ){
				if( can_legally_play_iid(player, grave[i]) ){
					pa[pac] = grave[i];
					pac++;
				}
				i++;
		}
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, "Select target instant or sorcery card.");
		int selected = select_card_from_zone(player, 1-player, pa, pac, 0, AI_MAX_CMC, -1, &this_test);
		if( selected != -1 ){
			int id = pa[selected];
			for (i = count_graveyard(1-player) - 1; i >= 0; --i){
				if( grave[i] == id ){
					play_card_in_grave_for_free(player, 1-player, i);
					break;
				}
			}
		}
	}

	return 0;
}

int card_chancellor_of_the_tangle(int player, int card, event_t event){
	vigilance(player, card, event);
	return 0;
}

int card_conversion_chamber(int player, int card, event_t event)
{
  /* Conversion Chamber	|3
   * Artifact
   * |2, |T: Exile target artifact card from a graveyard. Put a charge counter on ~.
   * |2, |T, Remove a charge counter from ~: Put a 3/3 colorless Golem artifact creature token onto the battlefield. */

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !(CAN_TAP(player, card) && CAN_ACTIVATE(player, card, MANACOST_X(2))))
		return 0;

	  enum
	  {
		CHOICE_EXILE = 1,
		CHOICE_GOLEM
	  } choice = DIALOG(player, card, event,
						"Exile an artifact", any_in_graveyard_by_type(ANYBODY, TYPE_ARTIFACT) && !graveyard_has_shroud(ANYBODY), 1,
						"Golem token", count_counters(player, card, COUNTER_CHARGE), 2);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		{
		  if (charge_mana_for_activated_ability(player, card, MANACOST_X(2)))
			switch (choice)
			  {
				case CHOICE_EXILE:
				  ;test_definition_t test;
				  new_default_test_definition(&test, TYPE_ARTIFACT, "Select an artifact card to exile.");

				  if (select_target_from_either_grave(player, card, 0, AI_MIN_VALUE, AI_MAX_VALUE, &test, 0, 1) != -1)
					{
					  tap_card(player, card);
					  if (player == AI && !(current_turn == 1-player && current_phase == PHASE_DISCARD))
						ai_modifier -= 64;
					}
				  break;

				case CHOICE_GOLEM:
				  remove_counter(player, card, COUNTER_CHARGE);
				  tap_card(player, card);
				  break;
			  }
		}
	  else	// EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_EXILE:
			  ;int selected, tgt_gy = get_card_instance(player, card)->targets[0].player;
			  if ((selected = validate_target_from_grave_source(player, card, tgt_gy, 1)) != -1)
				{
				  rfg_card_from_grave(tgt_gy, selected);
				  add_counter(player, card, COUNTER_CHARGE);
				}
			  break;

			case CHOICE_GOLEM:
			  generate_token_by_id(player, card, CARD_ID_GOLEM);
			  break;
		  }
	}

  if (event == EVENT_SHOULD_AI_PLAY)
	ai_modifier += (player == AI ? 24 : -24) * count_counters(player, card, COUNTER_CHARGE);

  return 0;
}

int card_corrosive_gale(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( has_phyrexian_mana(player, MANACOST_XG(player == AI, 1), 0)){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! is_token(player, card) ){
			if( ! played_for_free(player, card) && instance->info_slot == 1 ){
				if( ! charge_phyrexian_mana(player, card, event, MANACOST_G(1), 0) ){
					spell_fizzled = 1;
					return 0;
				}
			}
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot > 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			new_damage_all(player, card, ANYBODY, instance->info_slot, 0, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_corrupted_resolve(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			if( is_poisoned(card_on_stack_controller) ){
				return result;
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			return card_counterspell(player, card, event);
	}
	else{
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_darksteel_relic(int player, int card, event_t event){

	indestructible(player, card, event);

	return 0;
}

int card_death_hood_cobra(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) ){
			return 1;
		}
	}

	else if( event == EVENT_ACTIVATE){
			 charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0);
			 if( spell_fizzled != 1 ){
				 int choice = do_dialog(player, player, card, -1, -1, " Gains Reach\n Gains Deathtouch\n Do nothing", 1 );
				 if( choice != 2  ){
					instance->targets[9].player = 66+choice;
				 }
				 else{
					 spell_fizzled = 1;
				 }
			 }
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[9].player == 66 ){
				pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, KEYWORD_REACH, 0);
			}
			if( instance->targets[9].player == 67 ){
				create_targetted_legacy_effect(player, instance->parent_card, &deathtouch_until_eot, player, instance->parent_card);
			}
	}

	return 0;
}

int card_defensive_stance(int player, int card, event_t event){
	return generic_aura(player, card, event, player, -1, 1, 0, 0, 0, 0, 0);
}

int card_dementia_bat(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			new_multidiscard( instance->targets[0].player, 2, 0, player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 4, 1, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_despise(int player, int card, event_t event ){

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
				new_default_test_definition(&this_test, TYPE_CREATURE | TARGET_TYPE_PLANESWALKER, "Select a creature or planeswalker card.");

				ec_definition_t this_definition;
				default_ec_definition(instance->targets[0].player, player, &this_definition);
				new_effect_coercion(&this_definition, &this_test);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int due_respect_legacy(int player, int card, event_t event ){

	permanents_enters_battlefield_tapped(player, card, event, ANYBODY, TYPE_PERMANENT, NULL);

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_due_respect(int player, int card, event_t event ){

	/* Due Respect	|1|W
	 * Instant
	 * Permanents enter the battlefield tapped this turn.
	 * Draw a card. */

	if( event == EVENT_CAN_CAST ){
		return !(IS_AI(player) && current_turn == player);
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			create_legacy_effect(player, card, &due_respect_legacy);
			draw_cards(player, 1);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_card_entomber_exarch(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		int choice = 1;
		if (opponent_is_valid_target(player, card)){
			int ai_choice = 1;
			if( hand_count[1-player] == 0 ){
				ai_choice = 0;
			}
			choice = do_dialog(player, player, card, -1, -1, " Raise dead\n Opponent discards", ai_choice );
		}
		else{
			 choice = 0;
		}

		if( choice == 0 ){
			global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		else{
			if( target_opponent(player, card) ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, "Select a noncreature card.");
				this_test.type_flag = DOESNT_MATCH;

				ec_definition_t this_definition;
				default_ec_definition(instance->targets[0].player, player, &this_definition);
				new_effect_coercion(&this_definition, &this_test);
			}
		}
	}

	return 0;
}

int card_etched_monstrosity(int player, int card, event_t event){

	/* Etched Monstrosity	|5
	 * Artifact Creature - Golem 10/10
	 * ~ enters the battlefield with five -1/-1 counters on it.
	 * |W|U|B|R|G, Remove five -1/-1 counters from ~: Target player draws three cards. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_M1_M1, 5);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 1, 1, 1, 1, 1) && count_counters(player, card, COUNTER_M1_M1) >= 5 ){
			return 1;
		}
	}

	else if( event == EVENT_ACTIVATE){
			charge_mana_for_activated_ability(player, card, 0, 1, 1, 1, 1, 1);
			if( spell_fizzled != 1 &&  pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				remove_counters(player, card, COUNTER_M1_M1, 5);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				draw_cards(instance->targets[0].player, 3);
			}
	}

	return 0;
}

int card_exclusion_ritual(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].card != -1 && event == EVENT_MODIFY_COST_GLOBAL ){
		if( get_id(affected_card_controller, affected_card) == instance->targets[1].card ){
			infinite_casting_cost();
		}
	}

	if( event == EVENT_RESOLVE_SPELL && can_target(&td) ){
		pick_target(&td, "TARGET_PERMANENT");
		instance->targets[1].card = get_id(instance->targets[0].player, instance->targets[0].card);
		rfg_target_permanent(instance->targets[0].player, instance->targets[0].card);
		create_card_name_legacy(player, card, instance->targets[1].card);
	}
	return global_enchantment(player, card, event);
}


int card_fallen_ferromancer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if (valid_target(&td)){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 1, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_forced_worship(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
			if( has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 1) ){
				return 1;
			}
		}

		if( event == EVENT_ACTIVATE){
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 1);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			bounce_permanent(player, instance->parent_card);
		}

		cannot_attack(instance->damage_target_player, instance->damage_target_card, event);
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_furnace_scamp(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		int ai_choice = 1;
		if( life[1-player] < 4 ){
			ai_choice = 0;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Sacrifice and deal damage\n Pass", ai_choice);
		if( choice == 0 ){
			damage_player(1-player, 3, player, card);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return 0;
}

int card_glissas_scorn(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ARTIFACT");
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			lose_life(instance->targets[0].player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_glistening_oil(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	immortal_enchantment(player, card, event);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			add_minus1_minus1_counters(t_player, t_card, 1);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, KEYWORD_INFECT, 0, 0, 0, 0);
}

int card_greenhilt_trainee(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && get_power(player, card) > 3){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	else if( event == EVENT_ACTIVATE){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_ACTIVATION){
			 if( valid_target(&td) ){
				 pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 4, 4);
			 }
	}

  return 0;
}

int card_gremlin_mine(int player, int card, event_t event){

	/* Gremlin Mine	|1
	 * Artifact
	 * |1, |T, Sacrifice ~: ~ deals 4 damage to target artifact creature.
	 * |1, |T, Sacrifice ~: Remove up to four charge counters from target noncreature artifact. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && can_target(&td) &&
		! is_tapped(player, card) && ! is_animated_and_sick(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	else if( event == EVENT_ACTIVATE){
			int choice = 0;
			if( charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_ARTIFACT") ){
				if( ! is_what(instance->targets[0].player, instance->targets[0].card, TYPE_CREATURE) ){
					choice++;
				}
				instance->info_slot = 66+choice;
				kill_card(player, card, KILL_SACRIFICE);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION){
			 if( instance->targets[1].player == 66 && valid_target(&td) && is_what(instance->targets[0].player, instance->targets[0].card, TYPE_CREATURE) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
			 }
			 if( instance->targets[1].player == 67 && valid_target(&td) ){
				 remove_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_CHARGE, 4);
			 }
	}

	return 0;
}

int card_grim_affliction(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 instance->info_slot = 2;
			 pick_target(&td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			 if( valid_target(&td) ){
				 add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
				 proliferate(player, card);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_gut_shot(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST && affect_me(player, card) ){
		if( has_phyrexian_mana(player, MANACOST_R(1), 0)){
			COST_RED-=1;
		}
	}

	else if( event == EVENT_CAN_CAST ){
			 return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = played_for_free(player, card) + is_token(player, card);
			if( result < 1 ){
				result = charge_phyrexian_mana(player, card, event, MANACOST_R(1), 0);
			}

			if( result < 1 ){
				spell_fizzled = 1;
			}
			else{
				instance->info_slot = 1;
				pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, 1);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ichor_explosion(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 if( pick_creature_for_sacrifice(player, card, 0) ){
				 instance->info_slot = get_power(instance->targets[0].player, instance->targets[0].card);
				 kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			 if( instance->info_slot > 0 ){
				 int amount = instance->info_slot;
				 pump_subtype_until_eot(player, card, player, -1, -amount, -amount, 0, 0);
				 pump_subtype_until_eot(player, card, 1-player, -1, -amount, -amount, 0, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_immolating_souleater(int player, int card, event_t event)
{
  /* Immolating Souleater	|2
   * Artifact Creature - Hound 1/1
   * |PR: ~ gets +1/+0 until end of turn. */

  if (event == EVENT_POW_BOOST)
	{
	  if (!can_pay_life(player, 2))
		return generic_shade_amt_can_pump(player, card, 1, 0, MANACOST_R(1), -1);
	  else
		{
		  int mod = get_cost_mod_for_activated_abilities(player, card, MANACOST_X(0));
		  if (mod == 0)
			return generic_shade_amt_can_pump(player, card, 1, 0, MANACOST_R(1), -1) + generic_shade_amt_can_pump(player, card, 1, 2, MANACOST_X(0), -1);
		  else
			{
			  // This isn't going to be perfectly accurate due to mana sources that can tap for variable amounts of mana, but should be close.
			  int red_available = has_mana(player, COLOR_RED, 1);
			  int cless_available = has_mana(player, COLOR_ANY, 1);
			  int life_available = (life[player] - 1) / 2;
			  if (life_available >= cless_available / mod)
				return cless_available / mod;
			  else
				{
				  int amt = life_available;
				  cless_available -= life_available * mod;

				  if (red_available < cless_available / (mod + 1))
					amt += red_available;
				  else
					amt += cless_available / (mod + 1);

				  return amt;
				}
			}
		}
	}

  if (event == EVENT_CAN_ACTIVATE)
	return can_use_activated_abilities(player, card) && has_phyrexian_mana_for_activated_ability(player, card, MANACOST_R(1));

  if (event == EVENT_ACTIVATE)
	charge_phyrexian_mana_for_activated_ability(player, card, MANACOST_R(1));

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (in_play(instance->parent_controller, instance->parent_card))
		pump_until_eot_merge_previous(player, card, instance->parent_controller, instance->parent_card, 1,0);
	}

  return 0;
}

int card_impaler_shrike(int player, int card, event_t event)
{
  /* Impaler Shrike	|2|U|U
   * Creature - Bird 3/1
   * Flying
   * Whenever ~ deals combat damage to a player, you may sacrifice it. If you do, draw three cards. */

  if (damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL | DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER))
	{
	  draw_cards(player, 3);
	  kill_card(player, card, KILL_SACRIFICE);
	}

  return 0;
}

int card_inquisitor_exarch(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		int choice = 0;
		int ai_choice = 0;
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		if( would_valid_target(&td) ){
			ai_choice = 1;
			if( life[player] < 6){
				ai_choice = 0;
			}
			choice = do_dialog(player, player, card, -1, -1, " Gain 2 life\n Opponent lose 2 life", ai_choice );
		}

		if( choice == 0 ){
			gain_life(player, 2);
		}
		else{
			 lose_life(instance->targets[0].player, 2);
		}
	}

	return 0;
}

int card_insatiable_souleater(int player, int card, event_t event){

	/* Insatiable Souleater	|4
	 * Artifact Creature - Beast 5/1
	 * |PG: ~ gains trample until end of turn. */

	if (event == EVENT_CAN_ACTIVATE){
		return can_use_activated_abilities(player, card) && has_phyrexian_mana_for_activated_ability(player, card, MANACOST_G(1));
	}
	else if (event == EVENT_ACTIVATE){
		charge_phyrexian_mana_for_activated_ability(player, card, MANACOST_G(1));
	}
	else if (event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);
		if (in_play(instance->parent_controller, instance->parent_card)){
			pump_ability_until_eot(player, card, instance->parent_controller, instance->parent_card, 0,0, KEYWORD_TRAMPLE,0);
		}
	}

	return 0;
}

int card_invader_parasite(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		if (can_target(&td) && pick_target(&td, "TARGET_LAND")){
			instance->targets[9].player = get_id(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			create_card_name_legacy(player, card, instance->targets[9].player);
		}
	}

	if( instance->targets[9].player != -1 && trigger_condition == TRIGGER_COMES_INTO_PLAY &&
		affect_me( player, card ) && reason_for_trigger_controller == player &&
		trigger_cause_controller == 1-player
	  ){

		int trig = 0;

		if( get_id(trigger_cause_controller, trigger_cause) == instance->targets[9].player ){
		   trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if( event == EVENT_RESOLVE_TRIGGER){
					 damage_player(trigger_cause_controller, 2, player, card);
			}
		}
	}

	return 0;
}

int card_jor_kadeen_the_prevailer(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( metalcraft(player, card) ){
		boost_creature_type(player, card, event, -1, 3, 0, 0, BCT_CONTROLLER_ONLY+BCT_INCLUDE_SELF);
	}

	return 0;
}

int card_kiln_walker(int player, int card, event_t event)
{
  // Whenever ~ attacks, it gets +3/+0 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	pump_until_eot(player, card, player, card, 3, 0);

  if (event == EVENT_POW_BOOST && current_turn == player && current_phase <= PHASE_DECLARE_ATTACKERS)
	return 3;

  return 0;
}

int card_leeching_bite(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_target(&td1);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
				new_pick_target(&td1, "TARGET_CREATURE", 1, 1);
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 0) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1);
			}
			if( validate_target(player, card, &td1, 1) ){
				pump_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, -1, -1);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_lifes_finale(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			 manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
			 manipulate_all(player, card, 1-player,TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
			 int i;
			 for(i=0; i<3; i++){
				 global_tutor(player, 1-player, 1, TUTOR_GRAVE, 0, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_marrow_shards(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST && affect_me(player, card) ){
		if( has_phyrexian_mana(player, MANACOST_W(1), 0)){
			COST_WHITE-=1;
		}
	}

	else if( event == EVENT_CAN_CAST ){
			 if( player == AI ){
				if( current_turn != player ){
					return can_target(&td);
				}
			}
			else{
				return can_target(&td);
			}
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = played_for_free(player, card) + is_token(player, card);
			if( result < 1 ){
				result = charge_phyrexian_mana(player, card, event, MANACOST_W(1), 0);
			}

			if( result < 1 ){
				spell_fizzled = 1;
			}
			else{
				 instance->info_slot = 1;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			int count = active_cards_count[current_turn]-1;
			while( count >  -1 ){
					if( in_play(current_turn, count) && is_what(current_turn, count, TYPE_CREATURE) &&
						is_attacking(current_turn, count)
					  ){
						damage_creature(current_turn, count, 1, player, card);
					}
					count--;
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_maul_splicer(int player, int card, event_t event ){
	/* Maul Splicer	|6|G
	 * Creature - Human Artificer 1/1
	 * When ~ enters the battlefield, put two 3/3 colorless Golem artifact creature tokens onto the battlefield.
	 * Golem creatures you control have trample. */

	return generic_splicer(player, card, event, 0, 0, KEYWORD_TRAMPLE, 2);
}

int card_mindcrank(int player, int card, event_t event ){
	return 0;
}

int card_mindculling(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		return would_valid_target(&td);
	}


	else if( event == EVENT_RESOLVE_SPELL ){
			draw_cards(player, 2);
			new_multidiscard(1-player, 2, 0, player);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mortis_dogs(int player, int card, event_t event)
{
  // Whenever ~ attacks, it gets +2/+0 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	pump_until_eot(player, card, player, card, 2, 0);

  if (event == EVENT_POW_BOOST && current_turn == player && current_phase <= PHASE_DECLARE_ATTACKERS)
	return 2;

  // When ~ dies, target player loses life equal to its power.
  if (this_dies_trigger(player, card, event, 2)
	  && pick_player_duh(player, card, 1-player, 0))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  lose_life(instance->targets[0].player, instance->power);
	}

  return 0;
}

int card_mutagenic_growth(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	if( player == AI ){
		td.allowed_controller = player;
		td.preferred_controller = player;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST && affect_me(player, card) ){
		if( has_phyrexian_mana(player, MANACOST_G(1), 0)){
			COST_GREEN-=1;
		}
	}

	else if( event == EVENT_CAN_CAST ){
			 return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = played_for_free(player, card) + is_token(player, card);
			if( result < 1 ){
				result = charge_phyrexian_mana(player, card, event, MANACOST_G(1), 0);
			}

			if( result < 1 ){
				spell_fizzled = 1;
			}
			else{
				instance->info_slot = 1;
				pick_target(&td, "TARGET_CREATURE");
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mycosynth_fiend(int player, int card, event_t event){

	if( in_play(player, card) ){
		int amount = poison_counters[1-player];
		modify_pt_and_abilities(player, card, event, amount, amount, 0);
	}

	return 0;
}

int card_mycosynth_wellsprin(int player, int card, event_t event){

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		tutor_basic_land(player, 0, 0);
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		tutor_basic_land(player, 0, 0);
	}

	return 0;
}

int card_necropouncer(int player, int card, event_t event){

	if ((event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS) && is_equipping(player, card)){
		card_instance_t* instance = get_card_instance(player, card);
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 3, 1, 0);
		haste(instance->targets[8].player, instance->targets[8].card, event);
	}

	return living_weapon(player, card, event, 2);
}

int card_numbing_dose(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=100;
		pick_target(&td, "TARGET_PERMANENT");
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

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		does_not_untap(t_player, t_card, event);

		upkeep_trigger_ability(player, card, event, t_player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			lose_life(t_player, 1);
		}
	}
	return 0;
}

int card_omen_machine(int player, int card, event_t event){

	/* Omen Machine	|6
	 * Artifact
	 * Players can't draw cards.
	 * At the beginning of each player's draw step, that player exiles the top card of his or her library. If it's a land card, the player puts it onto the
	 * battlefield. Otherwise, the player casts it without paying its mana cost if able. */

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && event == EVENT_TRIGGER ){
		suppress_draw = 1;	// Deliberately do this during EVENT_TRIGGER, not EVENT_RESOLVE_TRIGGER, so nothing else can replace the (nonexistent) draw
	}

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int *deck = deck_ptr[current_turn];
		if( deck[0] != -1 ){
			if( is_what(-1, deck[0], TYPE_PERMANENT) ){
				put_into_play_a_card_from_deck(current_turn, current_turn, 0);
			}
			else{
				int csvid = cards_data[deck[0]].id;
				rfg_card_in_deck(current_turn, 0);
				play_card_in_exile_for_free(current_turn, current_turn, csvid);
			}
		}
	}

	return 0;
}

int card_parasitic_implant(int player, int card, event_t event){
	/* Parasitic Implant	|3|B
	 * Enchantment - Aura
	 * Enchant creature
	 * At the beginning of your upkeep, enchanted creature's controller sacrifices it and you put a 1/1 colorless Myr artifact creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( current_turn == player && upkeep_trigger(player, card, event) ){
			generate_token_by_id(player, card, CARD_ID_MYR);
			kill_card(instance->damage_target_player, instance->damage_target_card, KILL_SACRIFICE);
		}
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_pestilent_souleater(int player, int card, event_t event){

	/* Pestilent Souleater	|5
	 * Artifact Creature - Insect 3/3
	 * |PB: ~ gains infect until end of turn. */

	if (event == EVENT_CAN_ACTIVATE){
		return can_use_activated_abilities(player, card) && has_phyrexian_mana_for_activated_ability(player, card, MANACOST_B(1));
	}
	else if (event == EVENT_ACTIVATE){
		charge_phyrexian_mana_for_activated_ability(player, card, MANACOST_B(1));
	}
	else if (event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);
		if (in_play(instance->parent_controller, instance->parent_card)){
			pump_ability_until_eot(player, card, instance->parent_controller, instance->parent_card, 0,0, KEYWORD_INFECT,0);
		}
	}

	return 0;
}

int card_phyrexias_core(int player, int card, event_t event){

	/* Phyrexia's Core	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |1, |T, Sacrifice an artifact: You gain 1 life. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_TAP_FOR_MANA(player, card)){
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}

	if (event == EVENT_CAN_ACTIVATE && CAN_TAP_FOR_MANA(player, card)){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( !paying_mana() && CAN_ACTIVATE(player, card, MANACOST_X(2)) &&
			can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Sac artifact and gain 1 life\n Cancel", 1);
		}

		instance->info_slot = choice;

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			if (charge_mana_for_activated_ability(player, card, MANACOST_X(1)) && controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, 0)){
				instance->info_slot = 1;
			}
			else{
				untap_card_no_event(player, card);
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			gain_life(player, 1);
		}
	}

	return 0;
}

int card_phyrexian_ingester(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				instance->targets[9].player = get_power(instance->targets[0].player, instance->targets[0].card);
				instance->targets[9].card = get_toughness(instance->targets[0].player, instance->targets[0].card);
				int id = get_id(instance->targets[0].player, instance->targets[0].card);
				create_card_name_legacy(player, card, id);
				rfg_target_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	if( instance->targets[9].player > -1 ){
		modify_pt_and_abilities(player, card, event, instance->targets[9].player, instance->targets[9].card, 0);
	}

	return 0;
}

int card_pith_driller(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		card_instance_t* instance = get_card_instance(player, card);
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allow_cancel = 0;
		td.preferred_controller = 1-player;

		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
		}
	}

	phyrexian_casting_costs(player, card, event, 4, 1, 0, 0, 0, 0, 1);

	return 0;
}

int card_porcelain_legionnaire(int player, int card, event_t event){

	phyrexian_casting_costs(player, card, event, 2, 0, 0, 0, 0, 1, 1);

	return 0;
}

int card_praetors_grasp(int player, int card, event_t event)
{
  /* Praetor's Grasp	|1|B|B
   * Sorcery
   * Search target opponent's library for a card and exile it face down. Then that player shuffles his or her library. You may look at and play that card for as
   * long as it remains exiled. */

  if (event == EVENT_CAN_CAST)
	return opponent_is_valid_target(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	target_opponent(player, card);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (opponent_is_valid_target(player, card) && get_card_instance(player, card)->targets[0].player == 1-player)
		{
		  test_definition_t test;
		  new_default_test_definition(&test, 0, "Select a card to exile.");

		  int csvid = new_global_tutor(player, 1-player, TUTOR_FROM_DECK, TUTOR_RFG, 1, AI_MAX_VALUE, &test);
		  if (csvid != -1)
			{
			  play_sound_effect(WAV_DESTROY);	// exile sound, since we just obliterated the card to simulate it being exiled face-down
			  create_may_play_card_from_exile_effect(player, card, 1-player, csvid, MPCFE_FACE_DOWN);
			}
		}
	  else
		spell_fizzled = 1;

	  kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_priest_of_urabrask(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		produce_mana(player, COLOR_RED, 3);
	}

	return 0;
}

int card_pristine_talisman(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 1);
		return mana_producer(player, card, event);
	}

	return mana_producer(player, card, event);
}

int card_psychic_barrier(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		lose_life(instance->targets[0].player, 1);
		return card_remove_soul(player, card, event);
	}

	else{
		return card_remove_soul(player, card, event);
	}

	return 0;
}

int card_rage_extractor(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	phyrexian_casting_costs(player, card, event, 4, 0, 0, 0, 1, 0, 1);

	if( in_play(player, card) && can_target(&td) && trigger_condition == TRIGGER_SPELL_CAST &&
		affect_me(player, card) && reason_for_trigger_controller == player && trigger_cause_controller == player &&
		trigger_cause != card
	  ){

		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) && is_phyrexian(get_id(trigger_cause_controller, trigger_cause)) ){
			trig = 1;
		}

		if( trig == 1 ){
			if( event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					 if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
						damage_creature_or_player(player, card, event, get_cmc(trigger_cause_controller, trigger_cause));
						instance->number_of_targets = 1;
					 }
			}
		}
	}

	return 0;
}

int card_reaper_of_sheoldred(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player && damage->info_slot > 0 ){
				poison_counters[damage->damage_source_player]++;
			}
		}
	}

	return 0;
}

int card_remember_the_fallen(int player, int card, event_t event)
{
  /* Remember the Fallen	|2|W
   * Sorcery
   * Choose one or both - Return target creature card from your graveyard to your hand; and/or return target artifact card from your graveyard to your hand. */

  return spell_return_one_or_two_cards_from_gy_to_hand(player, card, event, TYPE_CREATURE, TYPE_ARTIFACT);
}

int card_ruthless_invasion(int player, int card, event_t event){

	phyrexian_casting_costs(player, card, event, 3, 0, 0, 0, 0, 1, 0);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
				if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && !  is_what(i, count, TYPE_ARTIFACT) ){
					pump_ability_until_eot(player, card, i, count, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
				}
				count--;
			}
		}
   }

	return 0;
}

int card_scrapyard_salvo(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->info_slot = count_graveyard_by_type(player, TYPE_ARTIFACT);
			pick_target(&td, "TARGET_PLAYER");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, instance->info_slot);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sensor_splicer(int player, int card, event_t event ){
	/* Sensor Splicer	|4|W
	 * Creature - Artificer 1/1
	 * When ~ enters the battlefield, put a 3/3 colorless Golem artifact creature token onto the battlefield.
	 * Golem creatures you control have vigilance. */

	if( event == EVENT_ABILITIES && affected_card_controller == player &&
		has_subtype(affected_card_controller, affected_card, SUBTYPE_GOLEM)
	  ){
		vigilance(affected_card_controller, affected_card, event);
	}

	return generic_splicer(player, card, event, 0, 0, 0, 1);
}

int card_shattered_angel(int player, int card, event_t event ){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		 reason_for_trigger_controller == player && trigger_cause_controller == 1-player
	  ){

		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
		   trig = 1;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if( event == EVENT_RESOLVE_TRIGGER){
					gain_life(player, 3);
			}
		}
	}

	return 0;
}

int card_sickleslicer(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_equipping(player, card) ){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 2, 2, 0);
	}

	return living_weapon(player, card, event, 4);
}

int card_slag_fiend(int player, int card, event_t event)
{
  /* Slag Fiend	|R
   * Creature - Construct 100/100
   * ~'s power and toughness are each equal to the number of artifact cards in all graveyards. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card))
	event_result += count_graveyard_by_type(ANYBODY, TYPE_ARTIFACT);

  return 0;
}

int card_slash_panter(int player, int card, event_t event){

	phyrexian_casting_costs(player, card, event, 4, 0, 0, 0, 1, 0, 0);

	haste(player, card, event);

	return 0;
}

int card_soul_conduit(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = 2;
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 6, 0, 0, 0, 0, 0) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->targets[1].player = player;
			instance->targets[1].card = -1;
			instance->number_of_targets = 2;
			if( would_validate_target(player, card, &td, 0) && would_validate_target(player, card, &td, 1) ){
				if( player != AI ){
					return 1;
				}
				else{
					 if( life[1-player] > life[player] ){
						 return 1;
					 }
				}
			}
		}
	}

	else if( event == EVENT_ACTIVATE){
			charge_mana_for_activated_ability(player, card, 6, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->targets[0].player = 1-player;
				instance->targets[0].card = -1;
				instance->targets[1].player = player;
				instance->targets[1].card = -1;
				instance->number_of_targets = 2;
				tap_card(player, card);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(player, card, &td, 0) && validate_target(player, card, &td, 1) ){
				int my_life = life[player];
				set_life_total(player, life[1-player]);
				set_life_total(1-player, my_life);
			}
	}

	return 0;
}

static const char* can_legally_change_target_to_me(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
	card_instance_t* eff = get_card_instance(player, card);
	if( eff->number_of_targets == 0 )
		return "must have at least 1 target";
	eff->targets[9].player = who_chooses;
	eff->targets[9].card = targeting_card;
	set_special_flags2(player, card, SF2_SPELLSKITE);
	int rvalue = call_card_function(player, card, EVENT_CAN_CHANGE_TARGET);
	eff->targets[9].player = -1;
	eff->targets[9].card = -1;
	remove_special_flags2(player, card, SF2_SPELLSKITE);
	return rvalue ? NULL :"cannot target Spellskite";
}

int card_spellskite(int player, int card, event_t event){
	target_definition_t td_spell;
	counterspell_target_definition(player, card, &td_spell, TYPE_SPELL);
	td_spell.extra = (int32_t)can_legally_change_target_to_me;
	td_spell.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance( player, card );

	if (event == EVENT_CAN_ACTIVATE ){
		if( has_phyrexian_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
			if( generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK, 0, 0, 0, 0, 0, 0, 0, &td_spell, "TARGET_SPELL") ){
				return 99;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_phyrexian_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
			set_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_SPELLSKITE);
			int rvalue = call_card_function(instance->targets[0].player, instance->targets[0].card, EVENT_CHANGE_TARGET);
			remove_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_SPELLSKITE);
			if( rvalue != -1 ){
				instance->info_slot = rvalue;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( counterspell_validate(player, card, &td_spell, 0) ){
			card_instance_t *spell = get_card_instance( instance->targets[0].player, instance->targets[0].card );
			spell->targets[instance->info_slot].player = player;
			spell->targets[instance->info_slot].card = instance->parent_card;
		}
	}

	return 0;
}

static const char* target_must_natively_use_charge_counters(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return get_counter_type_by_id(get_id(player, card)) == COUNTER_CHARGE ? NULL : "must use charge counters";
}
int card_surge_node(int player, int card, event_t event)
{
  /* Surge Node	|1
   * Artifact
   * ~ enters the battlefield with six charge counters on it.
   * |1, |T, Remove a charge counter from ~: Put a charge counter on target artifact. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, 6);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_ARTIFACT);
  td.preferred_controller = player;
  if (IS_AI(player))
	{
	  td.extra = (int)target_must_natively_use_charge_counters;
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	}

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_CHARGE);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(1), GVC_COUNTER(COUNTER_CHARGE), &td, "TARGET_ARTIFACT");
}

int card_suture_priest(int player, int card, event_t event ){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		 reason_for_trigger_controller == player
	  ){

		int trig = 0;

		if( trigger_cause_controller == player && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) &&
			trigger_cause != card ){
			trig = 1;
		}

		if( trigger_cause_controller == 1-player && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			trig = 2;
		}

		if( trig > 0 && check_battlefield_for_id(2, CARD_ID_TORPOR_ORB) ){
			trig = 0;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_AI(player);
			}
			else if( event == EVENT_RESOLVE_TRIGGER){
					 if( trig == 1 ){
						gain_life(player, 1);
					 }
					 if( trig == 2 ){
						lose_life(1-player, 1);
					 }
			}
		}
	}

	return 0;
}

int card_thundering_thanadon(int player, int card, event_t event ){

	phyrexian_casting_costs(player, card, event, 4, 0, 0, 2, 0, 0, 1);

	return 0;
}

int card_tormentor_exarch(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = 1-player;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) || can_target(&td1) ){
			int choice = 0;
			if( can_target(&td) ){
				if( can_target(&td1) ){
					choice = do_dialog(player, player, card, -1, -1, " Give +2/+0\n Give +0/-2", 1);
				}
			}
			else{
				 choice = 1;
			}

			if( choice == 0 ){
				pick_target(&td, "TARGET_CREATURE");
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 0);
			}
			else{
				pick_target(&td1, "TARGET_CREATURE");
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, -2);
			}
		}
	}

	return 0;
}

int card_trespassing_souleater(int player, int card, event_t event){

	/* Trespassing Souleater	|3
	 * Artifact Creature - Construct 2/2
	 * |PU: ~ is unblockable this turn. */

	if (event == EVENT_CAN_ACTIVATE){
		return can_use_activated_abilities(player, card) && has_phyrexian_mana_for_activated_ability(player, card, MANACOST_U(1));
	}
	else if (event == EVENT_ACTIVATE){
		charge_phyrexian_mana_for_activated_ability(player, card, MANACOST_U(1));
	}
	else if (event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);
		if (in_play(instance->parent_controller, instance->parent_card)){
			pump_ability_until_eot(player, card, instance->parent_controller, instance->parent_card, 0,0, 0,SP_KEYWORD_UNBLOCKABLE);
		}
	}

	return 0;
}

int card_triumph_of_the_horde(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	if( event == EVENT_CAN_CAST ){
		if( player != AI ){
			return 1;
		}
		else{
			 return can_target(&td);
		}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			pump_subtype_until_eot(player, card, player, -1, 1, 1, KEYWORD_TRAMPLE | KEYWORD_INFECT, 0);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_unwinding_clock(int player, int card, event_t event){

  card_instance_t *instance = get_card_instance( player, card );

	if( current_turn == 1-player && current_phase == PHASE_UNTAP && instance->info_slot != 66 ){
		manipulate_all(player, card, player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_UNTAP);
		instance->info_slot = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_vapor_snag(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 1);
			return card_unsummon(player, card, event);
		}
	}
	else{
		return card_unsummon(player, card, event);
	}

	return 0;
}

int card_victorious_destruction(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 pick_target(&td, "TARGET_PERMANENT");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			 if( valid_target(&td) ){
				 lose_life(instance->targets[0].player, 1);
				 kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_viral_drake(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		proliferate(player, card);
	}

	return generic_activated_ability(player, card, event, 0, 3, 0, 1, 0, 0, 0, 0, 0, 0);
}

int card_viridian_betrayers(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES && affect_me(player, card) && is_poisoned(1-player))
	event_result |= KEYWORD_INFECT;

  return 0;
}

int card_viridian_harvest(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=100;
		pick_target(&td, "TARGET_ARTIFACT");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			instance->damage_target_player = instance->targets[0].player;
			instance->damage_target_card = instance->targets[0].card;
		}
	}

	if( in_play(player, card) && instance->targets[0].player != -1 && instance->targets[0].card != -1 ){
		if( graveyard_from_play(instance->targets[0].player, instance->targets[0].card, event) ){
			gain_life(player, 6);
		}
	}
	return 0;
}

int card_war_report(int player, int card, event_t event ){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			int amount = count_subtype(2, TYPE_ARTIFACT, -1) + count_subtype(2, TYPE_CREATURE, -1);
			gain_life(player, amount);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_whipflare(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_type = TYPE_ARTIFACT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( player != AI ){
			return 1;
		}
		else{
			return can_target(&td);
		}
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 instance->info_slot = 2;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			 int i;
			 for(i=0; i<2; i++){
				 int count = active_cards_count[i]-1;
				 while(count > -1 ){
						if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && ! is_what(i, count, TYPE_ARTIFACT)){
							damage_creature(i, count, instance->info_slot, player, card);
						}
						count--;
				}
			 }
			 kill_card(player, card, KILL_DESTROY);

	}

	return 0;
}

int card_whispering_specter(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		int ai_choice = 1;
		if( poison_counters[1-player] > 1 ){
			ai_choice = 0;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Sac and make opponent discard\n Pass", ai_choice);
		if( choice == 0 ){
			kill_card(player, card, KILL_SACRIFICE);
			new_multidiscard(1-player, poison_counters[1-player], 0, player);
		}
	}
	return 0;
}

int card_xenograft(int player, int card, event_t event){

	 target_definition_t td;
	 default_target_definition(player, card, &td, TYPE_CREATURE);
	 td.illegal_abilities = 0;
	 td.allow_cancel = 0;
	 td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! can_target(&td) ){
			ai_modifier -=1000;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = select_a_subtype(player, card);
	}

	if( instance->info_slot > 0 && in_play(player, card) ){
		if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
			reason_for_trigger_controller == player
		  ){

			int trig = 0;

			if( trigger_cause_controller == player &&
				is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE)
			  ){
				trig = 1;
			}

			if( trig > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= 2;
				}
				else if( event == EVENT_RESOLVE_TRIGGER){
						 add_a_subtype(trigger_cause_controller, trigger_cause, instance->info_slot);
				}
			}
		}
	}

	if( leaves_play(player, card, event) ){
		manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_RESET_ADDED_SUBTYPE);
	}

	return global_enchantment(player, card, event);
}

