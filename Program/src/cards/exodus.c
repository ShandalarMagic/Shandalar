#include "manalink.h"

//----- GLOBAL FUNCTION ----

static int generic_type_recycler_creature(int player, int card, event_t event, test_definition_t *this_test){

	if( comes_into_play(player, card, event) && ! graveyard_has_shroud(2) ){
		char buffer[100];
		int pos = scnprintf(buffer, 100, "Select a");
		if( this_test->type == TYPE_ENCHANTMENT ){
			pos+=scnprintf(buffer+pos, 100-pos, "n Enchantment card.");
		}

		if( this_test->type == TYPE_SORCERY ){
			pos+=scnprintf(buffer+pos, 100-pos, " Sorcery card.");
		}

		if( (this_test->type & TYPE_INSTANT) || (this_test->type & TYPE_INTERRUPT) ){
			pos+=scnprintf(buffer+pos, 100-pos, "n Instant card.");
		}

		if( this_test->type == TYPE_ARTIFACT ){
			pos+=scnprintf(buffer+pos, 100-pos, "n Artifact card");
		}

		if( this_test->type == TYPE_LAND ){
			pos+=scnprintf(buffer+pos, 100-pos, " Land card");
		}

		if( this_test->type == TYPE_CREATURE ){
			pos+=scnprintf(buffer+pos, 100-pos, " Creature card");
		}
		strcpy(this_test->message, buffer);

		if( new_special_count_grave(player, this_test) > 0 ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, this_test);
		}
	}
	return 0;
}

static void pump_when_blocked(int player, int card, event_t event, int pow_plus, int tou_plus, int ability){

	if( current_turn == player && current_phase == PHASE_DECLARE_BLOCKERS && is_attacking(player, card) &&
		event == EVENT_DECLARE_BLOCKERS ){
		int count = 0;
		while(count < active_cards_count[1-player]){
			  if(in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
				 card_instance_t *instance = get_card_instance( 1-player, count);
				 if( instance->blocking == card ){
					  pump_ability_until_eot(player, card, player, card, pow_plus, tou_plus, ability, 0);
				 }

			  }
			  count++;
	   }
	}
}

static int exodus_keepers(int player, int card, event_t event, int color){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	int c[6] = {0, 0, 0, 0, 0, 0};
	c[color] = 1;

	target_definition_t td;
	default_target_definition(player, card, &td, 0 );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT | GAA_UNTAPPED, c[0], c[1], c[2], c[3], c[4], c[5], 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, c[0], c[1], c[2], c[3], c[4], c[5]) ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
		}
	}

	return 0;
}

//---- CARDS ----

int card_allay(int player, int card, event_t event)
{
  // Buyback |3
  // Destroy target enchantment.

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST || event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ENCHANTMENT", 1, NULL);
	}

	if (event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_ENCHANTMENT") ){
			instance->info_slot = buyback(player, card, MANACOST_X(3));
		}
	}

	if (event == EVENT_RESOLVE_SPELL){
		if (valid_target(&td)){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if (instance->info_slot){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_anarchist(int player, int card, event_t event){
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_SORCERY);
		this_test.type_flag = F1_NO_CREATURE;
		return generic_type_recycler_creature(player, card, event, &this_test);
	}
	return 0;
}

int card_carnophage(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( ! is_tapped(player, card) ){
			int to_tap = 1;
			if( can_pay_life(player, 1) ){
				int ai_choice = 0;
				if(life[player]-1 < 6){
					ai_choice = 1;
				}

				int choice = do_dialog(player, player, card, -1, -1, " Pay 1 life\n Tap this", ai_choice);
				if( choice == 0){
					lose_life(player, 1);
					to_tap = 0;
				}
			}

			if( to_tap == 1 ){
				tap_card(player, card);
			}
		}
	}

	return 0;
}

int card_cartographer(int player, int card, event_t event){
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		return generic_type_recycler_creature(player, card, event, &this_test);
	}
	return 0;
}

int not_marked(int iid, int markedptr, int player, int card)
{
  char (*marked)[2][151] = (char(*)[2][151])markedptr;
  return !(*marked)[player][card];
}

int card_cataclysm(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
			char marked[2][151] = {{0}};
			APNAP(p, {
				target_definition_t td;
				default_target_definition(player, card, &td, 0);
				td.allowed_controller = p;
				td.preferred_controller = p;
				td.who_chooses = p;
				td.allow_cancel = 0;
				td.illegal_abilities = 0;

				type_t types[] = { TYPE_ARTIFACT, TYPE_CREATURE, TYPE_ENCHANTMENT, TYPE_LAND };
				const char* prompts[] = { "TARGET_ARTIFACT", "TARGET_CREATURE", "TARGET_ENCHANTMENT", "TARGET_LAND" };

				int i;
				for (i = 0; i < 4; ++i){
					td.required_type = types[i];
					if (can_target(&td) && pick_target(&td, prompts[i])){
						marked[instance->targets[0].player][instance->targets[0].card] = 1;
						card_instance_t* aff = get_card_instance(player, card);

						// It's legal to choose the same permanent for more than one type, but don't let the AI
						aff->state |= STATE_TARGETTED;
						if (p == AI || ai_is_speculating == 1){
							aff->state |= STATE_CANNOT_TARGET;
						}
					}
				}

				int c;
				for (c = 0; c < active_cards_count[p]; ++c){
					if (marked[p][c]){
						// In particular, don't leave the cannot-target markers in place while sacrificing, or they interfere with leave-play triggers
						get_card_instance(p, c)->state &= ~(STATE_TARGETTED | STATE_CANNOT_TARGET);
					}
				}
			});
			APNAP(p, {
				if( can_sacrifice(player, p, 1, TYPE_PERMANENT, 0) ){
					test_definition_t this_test;
					default_test_definition(&this_test, TYPE_PERMANENT);
					this_test.special_selection_function = &not_marked;
					this_test.value_for_special_selection_function = (int)(marked);
					new_manipulate_all(player, card, p, &this_test, KILL_SACRIFICE);
				}
			});
			kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_city_of_traitors(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 2) ){ return 0; }

	if( trigger_condition == TRIGGER_SPELL_CAST && player == affected_card_controller
	   && card == affected_card && trigger_cause_controller == player && ! is_humiliated(player, card) )
	{
		card_data_t* card_d = get_card_data(trigger_cause_controller, trigger_cause);
		if(  card_d->type & TYPE_LAND ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
				kill_card(player, card, KILL_SACRIFICE );
			}
		}
	}

	return two_mana_land(player, card, event, COLOR_COLORLESS, COLOR_COLORLESS);
}

int card_coat_of_arms(int player, int card, event_t event){
	// original code : 004057C0

	/* Coat of Arms	|5
	 * Artifact
	 * Each creature gets +1/+1 for each other creature on the battlefield that shares at least one creature type with it. */

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
		in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card)
	  ){
		int p, c;
		for (p = 0; p < 2; p++){
			for (c = 0; c < active_cards_count[p]; ++c){
				if( !(c == affected_card && p == affected_card_controller) && in_play(p, c) && is_what(p, c, TYPE_CREATURE) &&
					shares_creature_subtype(affected_card_controller, affected_card, p, c)
				  ){
					++event_result;
				}
			}
		}
	}
	return 0;
}

int card_convalescence(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( life[player] < 10 ){
			gain_life(player, 1);
		}
	}

	return global_enchantment(player, card, event);
}

int card_culling_the_weak(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 produce_mana(player, COLOR_BLACK, 4);
			 kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_curiosity(int player, int card, event_t event)
{
  // original code : 0x405D70

  /* Curiosity	|U
   * Enchantment - Aura
   * Enchant creature
   * Whenever enchanted creature deals damage to an opponent, you may draw a card. */

  int packets;
  if ((packets = attached_creature_deals_damage(player, card, event, DDBM_MUST_DAMAGE_OPPONENT|DDBM_TRIGGER_OPTIONAL)))
	draw_cards(player, packets);

  return vanilla_aura(player, card, event, player);
}

int card_dauthi_jackal(int player, int card, event_t event){

	shadow(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.required_state = TARGET_STATE_BLOCKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, MANACOST_B(2), 0, &td1, "TARGET_CREATURE");
}

int card_dauthi_warlord(int player, int card, event_t event){
	shadow(player, card, event);
	if(event == EVENT_POWER && affect_me(player, card) && ! is_humiliated(player, card) ){
		int p;
		for( p = 0; p < 2; p++){
			int count = 0;
			while(count < active_cards_count[p]){
				if( in_play(p, count) && check_for_special_ability(p, count, SP_KEYWORD_SHADOW) ){
					event_result++;
				}
				count++;
		   }
		}
	}
	return 0;
}

int card_deaths_duet(int player, int card, event_t event)
{
  /* Death's Duet	|2|B
   * Sorcery
   * Return two target creature cards from your graveyard to your hand. */

  if (event == EVENT_CAN_CAST)
	return count_graveyard_by_type(player, TYPE_CREATURE) >= 2 && !graveyard_has_shroud(player);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select two target creature cards.");

	  card_instance_t* instance = get_card_instance(player, card);
	  select_multiple_cards_from_graveyard(player, player, -1, AI_MAX_VALUE, &test, 2, &instance->targets[0]);
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int i, num_validated = 0, selected;
	  for (i = 0; i < 2; ++i)
		if ((selected = validate_target_from_grave(player, card, player, i)) != -1)
		  {
			from_grave_to_hand(player, selected, TUTOR_HAND);
			++num_validated;
		  }

	  if (num_validated == 0)
		spell_fizzled = 1;

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_elvish_berserk(int player, int card, event_t event){

	pump_when_blocked(player, card, event, 1, 1,0);

	return 0;
}

int card_ephemeron(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( in_play(instance->parent_controller, instance->parent_card) ){
			bounce_permanent(instance->parent_controller, instance->parent_card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST0, 0, NULL, NULL);
}


int card_equilibrium(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){
		if( ! is_humiliated(player, card) ){
			int trig = 0;

			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);

			if( trigger_cause_controller == player && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) &&
			   can_target(&td) && has_mana(player, COLOR_COLORLESS, 1)
			 ){
			   trig = 1;
			}

			if( trig > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_AI(player);
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						charge_mana(player, COLOR_COLORLESS, 1);
						if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
							bounce_permanent(instance->targets[0].player, instance->targets[0].card);
						}
				}
			}
		}
	}
	return global_enchantment(player, card, event);
}

int card_erratic_portal(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int bounce = 1;
			if( has_mana(instance->targets[0].player, COLOR_COLORLESS, 1) ){
				int ai_choice = instance->targets[0].player != player ? 0 : 1;
				int choice = do_dialog(instance->targets[0].player, player, card, -1, -1, " Pay 1\n Pass", ai_choice);
				if( ! choice ){
					charge_mana(instance->targets[0].player, COLOR_COLORLESS, 1);
					if( spell_fizzled != 1 ){
						bounce = 0;
					}
				}
			}
			if( bounce == 1 ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_ertai_wizard_adept(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( counterspell_validate(player, card, &td, 0) ){
			real_counter_a_spell(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_NONSICK | GAA_SPELL_ON_STACK, MANACOST_XU(2, 2), 0, &td, NULL);
}

int card_exalted_dragon(int player, int card, event_t event){
	if( trigger_condition == TRIGGER_PAY_TO_ATTACK && current_phase == PHASE_DECLARE_ATTACKERS && ! is_humiliated(player, card) &&
		affect_me( player, card ) && reason_for_trigger_controller == player && forbid_attack == 0 &&
		trigger_cause_controller == player && trigger_cause == card
	  ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( !sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
						forbid_attack = 1;
					}
			}
	}
	return 0;
}

int card_flowstone_flood(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_LAND") ){
			if( ! is_token(player, card) && can_pay_life(player, 3) && hand_count[player] > 0 ){
				int choice = do_dialog(player, player, card, -1, -1, " Play normally\n Buyback", 1);
				if( choice == 1 ){
					lose_life(player, 3);
					discard(player, DISC_RANDOM, player);
					instance->info_slot = 1;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_forbid(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		counterspell(player, card, event, NULL, 0);
		if( spell_fizzled != 1 && ! is_token(player, card) && hand_count[player] >= 2 ){
			int choice = do_dialog(player, player, card, -1, -1, " Play normally\n Buyback", 0);
			instance->info_slot = choice;
			if( choice == 1){
				multidiscard(player, 2, 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
				return 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_hatred(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_pay_life(player, 1) && generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			int to_pay = 0;
			if( player == AI ){
				to_pay = life[1-player]-get_attack_power(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				 to_pay = choose_a_number(player, "Pay how much life?", life[player]);
			}

			if( to_pay > life[player] ){
				to_pay = life[player]-1;
			}

			if( to_pay < 1 ){
				spell_fizzled = 1;
			}
			else{
				 lose_life(player, to_pay);
				 instance->info_slot = to_pay;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, instance->info_slot, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_high_ground(int player, int card, event_t event)
{
  if (affected_card_controller == player)
	arbitrary_can_block_additional(event, 1);

  return global_enchantment(player, card, event);
}

int card_jackalope_herd(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		bounce_permanent(player, card);
	}
	return 0;
}

int card_keeper_of_the_beasts(int player, int card, event_t event){
	/*
	  Keeper of the Beasts English |G|G
	  Creature - Human Wizard 1/2
	  {G}, {T}: Choose target opponent who controlled more creatures than you did as you activated this ability.
	  Put a 2/2 green Beast creature token onto the battlefield.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0 );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_ACTIVATE ){
		if( exodus_keepers(player, card, event, COLOR_GREEN) && count_subtype(1-player, TYPE_CREATURE, -1) > count_subtype(player, TYPE_CREATURE, -1) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		exodus_keepers(player, card, event, COLOR_GREEN);
		if( spell_fizzled != 1 ){
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_BEAST, &token);
			token.pow = 2;
			token.tou = 2;
			generate_token(&token);
		}
	}

	return 0;
}

int card_keeper_of_the_dead(int player, int card, event_t event){
	/*
	  Keeper of the Dead |B|B
	  Creature - Human Wizard 1/2
	  {B}, {T}: Choose target opponent who had at least two fewer creature cards in his or her graveyard than you did as you activated this ability.
	  Destroy target nonblack creature he or she controls.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0 );
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
	td1.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( exodus_keepers(player, card, event, COLOR_BLACK) ){
			return count_graveyard_by_type(player, TYPE_CREATURE)-count_graveyard_by_type(1-player, TYPE_CREATURE) > 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		exodus_keepers(player, card, event, COLOR_BLACK);
		if( spell_fizzled != 1 ){
			if( new_pick_target(&td1, get_sleighted_color_text(player, card, "Select target non%s creature.", COLOR_BLACK), 1, 1 | GS_LITERAL_PROMPT) ){
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && validate_target(player, card, &td1, 1) ){
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_keeper_of_the_flame(int player, int card, event_t event){
	/*
	  Keeper of the Flame |R|R
	  Creature - Human Wizard 1/2
	  {R}, {T}: Choose target opponent who had more life than you did as you activated this ability. Keeper of the Flame deals 2 damage to him or her.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0 );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_ACTIVATE ){
		if( exodus_keepers(player, card, event, COLOR_RED) ){
			return life[1-player] > life[player];
		}
	}

	if( event == EVENT_ACTIVATE ){
		exodus_keepers(player, card, event, COLOR_RED);
		if( spell_fizzled != 1 ){
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 2);
		}
	}

	return 0;
}

int card_keeper_of_the_light(int player, int card, event_t event){
	/*
	  Keeper of the Light |W|W
	  Creature - Human Wizard 1/2
	  {W}, {T}: Choose target opponent who had more life than you did as you activated this ability. You gain 3 life.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0 );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_ACTIVATE ){
		if( exodus_keepers(player, card, event, COLOR_WHITE) ){
			return life[player] < life[1-player];
		}
	}

	if( event == EVENT_ACTIVATE ){
		exodus_keepers(player, card, event, COLOR_WHITE);
		if( spell_fizzled != 1 ){
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			gain_life(player, 3);
		}
	}

	return 0;
}

int card_keeper_of_the_mind(int player, int card, event_t event){
	/*
	  Keeper of the Mind |U|U
	  Creature - Human Wizard 1/2
	  {U}, {T}: Choose target opponent who had at least two more cards in hand than you did as you activated this ability. Draw a card.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0 );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_ACTIVATE ){
		if( exodus_keepers(player, card, event, COLOR_BLUE) && (hand_count[1-player]-hand_count[player]) > 1){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		exodus_keepers(player, card, event, COLOR_BLUE);
		if( spell_fizzled != 1 ){
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_mana_breach(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.illegal_abilities = 0;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( specific_spell_played(player, card, event, ANYBODY, can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			td.allowed_controller = instance->targets[1].player;
			td.preferred_controller = instance->targets[1].player;
			td.who_chooses = instance->targets[1].player;
			if( can_target(&td) && pick_target(&td, "TARGET_LAND") ){
				instance->number_of_targets = 1;
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_manabond(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 13) ){ return 0; }

	if( current_turn == player && trigger_condition == TRIGGER_EOT ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.zone = TARGET_ZONE_HAND;

		if( eot_trigger_mode(player, card, event, player, check_battlefield_for_special_card(player, card, player, 0, &this_test) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			int i = active_cards_count[player]-1;
			while( i > -1 ){
					if( in_hand(player, i) ){
						if( is_what(player, i, TYPE_LAND) ){
							put_into_play(player, i);
						}
					}
					i--;
			}
			discard_all(player);
		}
	}

	return global_enchantment(player, card, event);
}

int card_memory_crystal(int player, int card, event_t event){

	return 0;
}

int card_merfolk_looter(int player, int card, event_t event)
{
  // 0x1202CB8

  /* Merfolk Looter	|1|U
   * Creature - Merfolk Rogue 1/1
   * |T: Draw a card, then discard a card. */

  if (!IS_GAA_EVENT(event))
	return 0;

  if (event == EVENT_ACTIVATE && IS_AI(player) && player == AI)
	{
	  // Same logic as Jalum Tome.
	  int c, total_lands = 0;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if ((in_play(player, c) || in_hand(player, c)) && is_what(player, c, TYPE_LAND))
		  ++total_lands;

	  if (total_lands <= 2 || total_lands >= 8)
		ai_modifier += 48;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  draw_a_card(player);
	  discard(player, 0, player);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_mind_maggots(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		char msg[100] = "Select a creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.zone = TARGET_ZONE_HAND;

		if( comes_into_play_mode(player, card, event, check_battlefield_for_special_card(player, card, player, 0, &this_test) ? RESOLVE_TRIGGER_MANDATORY : 0) ){
			while( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
					int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 ){
						discard_card(player, selected);
						add_1_1_counters(player, card, 2);
					}
					else{
						break;
					}
			}
		}
	}

	return 0;
}

int card_mind_over_matter(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST || event == EVENT_SHOULD_AI_PLAY ){
		return global_enchantment(player, card, event);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	char msg[100] = "Select a card to discard.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, msg);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_LAND | TYPE_ARTIFACT);

	if(event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DISCARD, MANACOST0, 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DISCARD, MANACOST0, 0, &td, "TWIDDLE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td, 0) ){
			twiddle(player, card, 0);
		}
	}

	return 0;
}

int card_mindless_automaton(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, GVC_COUNTERS(COUNTER_P1_P1, 2), NULL, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_X(1), 0, NULL, NULL) ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_DISCARD, MANACOST_X(1), 0, NULL, NULL)  ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST0, GVC_COUNTERS(COUNTER_P1_P1, 2), NULL, NULL) ){
				int ai_choice =  is_attacking(player, card) && current_phase == PHASE_AFTER_BLOCKING  ? 0 : 1;
				choice = do_dialog(player, player, card, -1, -1, " Discard & pump\n Draw a card\n Do nothing", ai_choice);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1-choice)) ){
			if( choice == 0 ){
				discard(player, 0, player);
				instance->info_slot = 66;
			}
			if( choice == 1 ){
				remove_1_1_counters(player, card, 2);
				instance->info_slot = 67;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			add_1_1_counter(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 67 ){
			draw_cards(player, 1);
		}
	}


	return 0;
}

int card_mirri_cat_warrior(int player, int card, event_t event)
{
  /* Mirri, Cat Warrior	|1|G|G
   * Legendary Creature - Cat Warrior 2/3
   * First strike, |H2forestwalk, vigilance */

  check_legend_rule(player, card, event);
  vigilance(player, card, event);
  return 0;
}

int card_nausea(int player, int card, event_t event){
  /* Mirri, Cat Warrior	|1|G|G
   * Legendary Creature - Cat Warrior 2/3
   * First strike, |H2forestwalk, vigilance */

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, ANYBODY, -1, -1, -1, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}


int card_null_brooch(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ANY);
	td.illegal_type = TYPE_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SPELL_ON_STACK, MANACOST_X(2), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if(	charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
			tap_card(player, card);
			discard_all(player);
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( counterspell_validate(player, card, &td, 0) ){
			real_counter_a_spell(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

#pragma message "For some strange reasons, adding the 'would_validate_arbitrary' check won't make the Oaths trigger. Even checking directly for PB flags screws is. So, currently, that check is removed."
int card_oath_of_druids(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		if( count_subtype(current_turn, TYPE_CREATURE, -1) < count_subtype(1-current_turn, TYPE_CREATURE, -1) ){
			upkeep_trigger_ability(player, card, event, ANYBODY);
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( count_subtype(current_turn, TYPE_CREATURE, -1) < count_subtype(1-current_turn, TYPE_CREATURE, -1) ){
			if( do_dialog(current_turn, player, card, -1, -1, " Activate Oath of Druids\n Pass", count_deck_by_type(current_turn, TYPE_CREATURE) ? 0 : 1) == 0 ){
				int *deck = deck_ptr[current_turn];
				if( deck[0] != -1 ){
					int c1 = 0;
					int good = 0;
					while( deck[c1] != -1 ){
							if( is_what(-1, deck[c1], TYPE_CREATURE) ){
								good = 1;
								break;
							}
							c1++;
					}
					show_deck( 1-current_turn, deck, c1+1, "Cards revealed by Oath of Druids", 0, 0x7375B0 );
					if( good == 1 && ! check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE) ){
						int card_added = add_card_to_hand(current_turn, deck[c1]);
						remove_card_from_deck(current_turn, c1);
						get_card_instance(current_turn, card_added)->state |= STATE_PROCESSING;	// so it doesn't activate its own upkeep trigger
						put_into_play(current_turn, card_added);
						c1--;
					}
					if( c1 > -1 ){
						mill(current_turn, c1+1);
					}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_oath_of_ghouls(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		if( count_graveyard_by_type(current_turn, TYPE_CREATURE) > count_graveyard_by_type(1-current_turn, TYPE_CREATURE) ){
			upkeep_trigger_ability(player, card, event, ANYBODY);
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( count_graveyard_by_type(current_turn, TYPE_CREATURE) > count_graveyard_by_type(1-current_turn, TYPE_CREATURE) ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			new_global_tutor(current_turn, current_turn, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
	}

	return global_enchantment(player, card, event);
}

int card_oath_of_lieges(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		if( landsofcolor_controlled[current_turn][COLOR_ANY] < landsofcolor_controlled[1-current_turn][COLOR_ANY] ){
			upkeep_trigger_ability(player, card, event, ANYBODY);
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( landsofcolor_controlled[current_turn][COLOR_ANY] < landsofcolor_controlled[1-current_turn][COLOR_ANY] ){
			if( do_dialog(current_turn, player, card, -1, -1, " Activate Oath of Lieges\n Pass", 0) == 0 ){
				char msg[100] = "Select a basic land card.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_LAND, msg);
				this_test.subtype = SUBTYPE_BASIC;
				new_global_tutor(current_turn, current_turn, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_oath_of_scholars(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		if( hand_count[current_turn] <  hand_count[1-current_turn] ){
			upkeep_trigger_ability(player, card, event, ANYBODY);
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( hand_count[current_turn] <  hand_count[1-current_turn] ){
			if( do_dialog(current_turn, player, card, -1, -1, " Activate Oath of Scholars\n Pass", hand_count[current_turn] < 3 ? 0 : 1) == 0 ){
				new_discard_all(current_turn, player);
				draw_cards(current_turn, 3);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_oath_of_mages(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		if( life[current_turn] < life[1-current_turn] ){
			upkeep_trigger_ability(player, card, event, ANYBODY);
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( life[current_turn] < life[1-current_turn] ){
			if( do_dialog(current_turn, player, card, -1, -1, " Damage opponent\n Pass", 0) == 0 ){
				damage_player(1-current_turn, 1, player, card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_ogre_shaman(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DISCARD, MANACOST_X(2), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			discard(player, DISC_RANDOM, player);
		}
	}

   if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 2);
		}
   }

	return 0;
}

int card_onslaught(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me( player, card ) && ! is_humiliated(player, card) &&
		reason_for_trigger_controller == affected_card_controller && trigger_cause_controller == player ){

		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			trig = 1;
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		if( trig > 0 && can_target(&td) ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					instance->number_of_targets = 0;
					if ( pick_target(&td, "TARGET_CREATURE")){
						tap_card(instance->targets[0].player, instance->targets[0].card);
					}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_paladin_en_vec(int player, int card, event_t event){
	/*
	  Paladin en-Vec |1|W|W
	  Creature - Human Knight 2/2
	  First strike, protection from black and from red
	*/
	if( ! is_humiliated(player, card) ){
		protection_from_black(player, card, event);
		protection_from_red(player, card, event);
	}

	return 0;
}

void pandemonium_effect(int player, int card){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t* instance = get_card_instance(player, card);
	int pow = get_power(player, card);
	instance->number_of_targets = 0;

	if( pow > 0 && can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
		damage_creature(instance->targets[0].player, instance->targets[0].card, pow, player, card);
	}
}

static void pandemonium_trigger(int player, int card, event_t event){

	if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t *instance = get_card_instance(player, card);
		pandemonium_effect(instance->targets[1].player, instance->targets[1].card);
	}
}

int card_pandemonium(int player, int card, event_t event){

	/* Pandemonium	|3|R
	 * Enchantment
	 * Whenever a creature enters the battlefield, that creature's controller may have it deal damage equal to its power to target creature or player of his or
	 * her choice. */

	pandemonium_trigger(player, card, event);

	return global_enchantment(player, card, event);
}

int card_peace_of_mind(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST || event == EVENT_SHOULD_AI_PLAY ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 3);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_W(1), 0, NULL, NULL);
}

int card_pegasus_stampede(int player, int card, event_t event){
	/* Pegasus Stampede	|1|W
	 * Sorcery
	 * Buyback-Sacrifice a land.
	 * Put a 1/1 |Swhite Pegasus creature token with flying onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);

	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! is_token(player, card) && can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				int ai_choice = 0;
				if( count_subtype(player, TYPE_LAND, -1) > 6 ){
					ai_choice = 1;
				}
				int choice = do_dialog(player, player, card, -1, -1, " Play normally\n Buyback", ai_choice);
				if( choice == 1 ){
					if( sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
						instance->info_slot = 1;
					}
				}
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			generate_token_by_id(player, card, CARD_ID_PEGASUS);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return 0;
}

int card_pit_spawn(int player, int card, event_t event){

	if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card)){
		get_card_instance(player, card)->destroys_if_blocked |= DIFB_DESTROYS_UNPROTECTED;
	}

	basic_upkeep(player, card, event, 0, 2, 0, 0, 0, 0);

	rfg_when_damage(player, card, event);

	return 0;
}

static int target_for_plaguebearer(int player, int card, int mode){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance= get_card_instance(player, card);

	int i;
	int trg = -1;
	int par = -1;
	for(i=0; i<2; i++){
		if( i == 1-player || player == HUMAN ){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
						int cost = get_cmc(i, count)*2;
						if( has_mana_for_activated_ability(player, card, MANACOST_XB(cost, 1)) ){
							instance->targets[0].player = i;
							instance->targets[0].card = count;
							instance->number_of_targets = 1;
							if( would_validate_target(player, card, &td, 0) ){
								if( mode == 0 || player == HUMAN ){
									return 1;
								}
								if( mode == 1 && get_base_value(i, count) > par ){
									par = get_base_value(i, count);
									trg = count;
								}
							}
						}
					}
					count++;
			}
		}
	}
	if( mode == 1 ){
		return trg;
	}
	return 0;
}

int card_plaguebearer(int player, int card, event_t event){
	/*
	  Plaguebearer English |1|B
	  Creature - Zombie 1/1
	  {X}{X}{B}: Destroy target nonblack creature with converted mana cost X.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		return target_for_plaguebearer(player, card, 0);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		int op = -1;
		int oc = -1;
		if( player == HUMAN ){
			if( select_target(player, card, &td, "Select target nonblack creature.", NULL) ){
				op = instance->targets[0].player;
				oc = instance->targets[0].card;
				int cost = 2*get_cmc( instance->targets[0].player, instance->targets[0].card );
				if( ! has_mana_for_activated_ability(player, card, MANACOST_XB(cost, 1)) ){
					spell_fizzled = 1;
					return 0;
				}
				if( charge_mana_for_activated_ability(player, card, MANACOST_XB(cost, 1)) ){
					instance->targets[0].player = op;
					instance->targets[0].card = oc;
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
		else{
			int result = target_for_plaguebearer(player, card, 1);
			if( result != -1 ){
				int cost = 2*get_cmc( 1-player, result );
				if( charge_mana_for_activated_ability(player, card, MANACOST_XB(cost, 1)) ){
					instance->targets[0].player = 1-player;
					instance->targets[0].card = result;
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
		}
	}
	return 0;
}

int card_price_of_progress(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_SPELL){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.subtype = SUBTYPE_BASIC;
		this_test.subtype_flag = DOESNT_MATCH;
		APNAP(p,
		{
			int amount = 2*check_battlefield_for_special_card(player, card, p, CBFSC_GET_COUNT, &this_test);
			if( amount ){
				damage_player(p, amount, player, card);
			}
		});
		kill_card(player, card, KILL_BURY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_pygmy_troll(int player, int card, event_t event){

	pump_when_blocked(player, card, event, 1, 1, 0);

	return card_river_boa(player, card, event);
}

int card_ravenous_baboons(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.required_subtype = SUBTYPE_BASIC;
		td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( new_pick_target(&td, "Select target non-Basic land.", 0, GS_LITERAL_PROMPT) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

int card_reaping_the_rewards(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! is_token(player, card) && can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				int ai_choice = 0;
				if( count_subtype(player, TYPE_LAND, -1) > 6 ){
					ai_choice = 1;
				}
				int choice = do_dialog(player, player, card, -1, -1, " Play normally\n Buyback", ai_choice);
				if( choice == 1 ){
					if( sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
						instance->info_slot = 1;
					}
				}
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			gain_life(player, 2);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return 0;
}

int card_reclaim(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put on top.");

	if( event == EVENT_RESOLVE_SPELL){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			const int *grave = get_grave(player);
			int card_added = add_card_to_hand(player, grave[selected]);
			remove_card_from_grave(player, selected);
			put_on_top_of_deck(player, card_added);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
}

int card_recurring_nightmare(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_SORCERY_BE_PLAYED | GAA_BOUNCE_ME | GAA_SACRIFICE_CREATURE, MANACOST0, 0, NULL, NULL) ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(player) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int result = pick_creature_for_sacrifice(player, card, 0);
			if( result > -1 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

				if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 1) != -1 ){
					kill_card(player, result, KILL_SACRIFICE);
					bounce_permanent(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			}
			else{
				 spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 1);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}
	}

	return global_enchantment(player, card, event);
}

int card_rootwater_mystic(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[instance->targets[0].player];
			if( deck[0] != -1 ){
				show_deck( player, deck, 1, "Here's the first card of deck.", 0, 0x7375B0 );
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XU(1, 1), 0, &td, "TARGET_PLAYER");
}

int card_scalding_salamander(int player, int card, event_t event)
{
  // Whenever ~ attacks, you may have it deal 1 damage to each creature without flying defending player controls.
  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.keyword = KEYWORD_FLYING;
	  test.keyword_flag = DOESNT_MATCH;
	  new_damage_all(player, card, 1-current_turn, 1, 0, &test);
	}

  return 0;
}

int card_scare_tactics(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL){
		pump_subtype_until_eot(player, card, player, -1, 1, 0, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_school_of_piranha(int player, int card, event_t event){

	basic_upkeep(player, card, event, MANACOST_XU(1, 1));

	return 0;
}

int card_srivener(int player, int card, event_t event){
	// Scrivener
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_INSTANT | TYPE_INTERRUPT);
		this_test.type_flag = F1_NO_CREATURE;
		return generic_type_recycler_creature(player, card, event, &this_test);
	}
	return 0;
}

int card_seismic_assault(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, "Select a land card to discard.");
	this_test.zone = TARGET_ZONE_HAND;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GS_CAN_TARGET, MANACOST0, 0, &td1, NULL) ){
			return check_battlefield_for_special_card(player, card, player, 0, &this_test);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			instance->number_of_targets = 0;
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
					instance->number_of_targets = 1;
					discard_card(player, selected);
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			damage_target0(player, card, 2);
		}
	}

	return 0;
}

int card_shackles(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
		if( event == EVENT_CAN_ACTIVATE ||  event == EVENT_ACTIVATE  ){
			return generic_activated_ability(player, card, event, 0, MANACOST_W(1), 0, NULL, NULL);
		}

		if( event == EVENT_RESOLVE_ACTIVATION && in_play(instance->parent_controller, instance->parent_card) ){
			bounce_permanent(instance->parent_controller, instance->parent_card);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.required_state = TARGET_STATE_TAPPED;
	}

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

int card_shattering_pulse(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_ARTIFACT") ){
			instance->number_of_targets = 1;
			instance->info_slot = buyback(player, card, 3, 0, 0, 0, 0, 0);
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_skyshaper(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, card, player, -1, 0, 0, KEYWORD_FLYING, 0);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}

int card_skyshroud_elite(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_POWER && affect_me(player, card) ){
			if( control_nonbasic_land(1-player) ){
				event_result++;
			}
		}

		if( event == EVENT_TOUGHNESS && affect_me(player, card) ){
			if( control_nonbasic_land(1-player) ){
				event_result+=2;
			}
		}
	}

	return 0;
}

int card_skyshroud_warbeast(int player, int card, event_t event){
	/* Skyshroud War Beast	|1|G
	 * Creature - Beast 100/100
	 * Trample
	 * As ~ enters the battlefield, choose an opponent.
	 * ~'s power and toughness are each equal to the number of nonbasic lands the chosen player controls. */

	if (card == -1){
		return 0;	// No opponent chosen yet.
	}

	// This doesn't account for change of control - it should always be the player chosen as it entered the bf, not the opponent of whoever currently controls it
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result += count_nonbasic_lands(1-player);
	}

	return 0;
}


int card_slaughter2(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			if( ! is_token(player, card) && can_pay_life(player, 4) ){
				int choice = do_dialog(player, player, card, -1, -1, " Play normally\n Buyback", 1);
				if( choice == 1 ){
					lose_life(player, 4);
					instance->info_slot = 1;
				}
			}
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_soltari_visionary(int player, int card, event_t event){

	shadow(player, card, event);

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ENCHANTMENT );
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_ENCHANTMENT") ){
			card_instance_t *instance= get_card_instance(player, card);
			kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
		}
	}

	return 0;
}

int card_sonic_burst(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST && hand_count[player] > 0 ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			discard(player, DISC_RANDOM, player);
		}
	}

	else if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_soul_warden(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;

		if( new_specific_cip(player, card, event, 2, 2, &this_test) ){
			gain_life(player, 1);
		}
	}

	return 0;
}

int card_spellbook(int player, int card, event_t event){
	/*
	  Spellbook |0
	  Artifact
	  You have no maximum hand size.
	*/
	if( ! is_humiliated(player, card) ){
		if (event == EVENT_MAX_HAND_SIZE && current_turn == player ){
			event_result = 1000;
		}
	}

	return 0;
}


int card_spellshock(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			damage_player(get_card_instance(player, card)->targets[1].player, 2, player, card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_sphere_of_resistance(int player, int card, event_t event){
	if( event == EVENT_MODIFY_COST_GLOBAL && ! is_what(affected_card_controller, affected_card, TYPE_LAND) ){
		COST_COLORLESS++;
	}
	return 0;
}

int card_spike_cannibal(int player, int card, event_t event)
{
  /* Spike Cannibal	|1|B|B
   * Creature - Spike 0/0
   * ~ enters the battlefield with a +1/+1 counter on it.
   * When ~ enters the battlefield, move all +1/+1 counters from all creatures onto it. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 1);

  if (comes_into_play(player, card, event))
	{
	  // Carefully not moving counters that are added as a result of removing counters from other creatures from some horrid combination of death triggers
	  int num_counters[2][151] = {{0}};
	  int p, c;
	  for (p = 0; p < 2; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_CREATURE))
			num_counters[p][c] = count_counters(p, c, COUNTER_P1_P1);

	  num_counters[player][card] = 0;

	  for (p = 0; p < 2; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (num_counters[p][c])
			move_counters(player, card, p, c, COUNTER_P1_P1, num_counters[p][c]);
	}

  return 0;
}

int card_spike_hatcher(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 6);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE  ){
		if( generic_activated_ability(player, card, event, GAA_REGENERATION | GAA_1_1_COUNTER, MANACOST_X(1), 0, NULL, NULL) ){
			return 99;
		}
		return generic_spike(player, card, event, 6);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_spike(player, card, EVENT_CAN_ACTIVATE, 6) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_REGENERATION | GAA_1_1_COUNTER, MANACOST_X(1), 0, NULL, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Move a counter\n Regenerate\n Cancel", 1);
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
				generic_activated_ability(player, card, event, GAA_REGENERATION | GAA_1_1_COUNTER, MANACOST_X(1), 0, NULL, NULL);
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
		if( instance->info_slot == 67 && can_regenerate(instance->parent_controller, instance->parent_card) ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_spike_rogue(int player, int card, event_t event){

	/* Spike Rogue	|1|G|G
	 * Creature - Spike 0/0
	 * ~ enters the battlefield with two +1/+1 counters on it.
	 * |2, Remove a +1/+1 counter from ~: Put a +1/+1 counter on target creature.
	 * |2, Remove a +1/+1 counter from a creature you control: Put a +1/+1 counter on ~. */

	card_instance_t *instance = get_card_instance(player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	base_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.special = TARGET_SPECIAL_REQUIRES_COUNTER | TARGET_SPECIAL_NOT_ME;
	td1.extra = COUNTER_P1_P1;

	if( event == EVENT_CAN_ACTIVATE  ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_1_1_COUNTER, MANACOST_X(2), 0, &td1, NULL) ){
			return 1;
		}
		return generic_spike(player, card, event, 6);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_spike(player, card, EVENT_CAN_ACTIVATE, 6) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_X(2), 0, &td1, NULL) ){
				int ai_choice = count_1_1_counters(player, card) < 2 ? 1 : 0;
				choice = do_dialog(player, player, card, -1, -1, " Move a counter\n Steal a counter\n Cancel", ai_choice);
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
				generic_activated_ability(player, card, event, GAA_CAN_TARGET | GS_LITERAL_PROMPT, MANACOST_X(2), 0,
											&td1, "Select target creature you control with a +1/+1 counter.");
				if( spell_fizzled != 1 ){
					remove_1_1_counter(instance->targets[0].player, instance->targets[0].card);
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
			add_1_1_counter(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_spike_weaver(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 3);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST_X(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_spike(player, card, EVENT_CAN_ACTIVATE, 6) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_1_1_COUNTER, MANACOST_X(1), 0, NULL, NULL) ){
				int ai_choice = current_turn != player ? 1 : 0;
				choice = do_dialog(player, player, card, -1, -1, " Move a counter\n Fog\n Cancel", ai_choice);
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
				generic_activated_ability(player, card, event, GAA_1_1_COUNTER, MANACOST_X(1), 0, NULL, NULL);
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
			fog_effect(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_survival_of_the_fittest(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);
	this_test.zone = TARGET_ZONE_HAND;

	if(event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) &&
		check_battlefield_for_special_card(player, card, player, 0, &this_test)
	   ){
		if( player == AI ){
			if( new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_GOOD_TO_PUT_IN_GRAVE, -1, &this_test) > -1 ){
				return 1;
			}
		}
		else{
			return 1;
		}
	}
	else if(event == EVENT_ACTIVATE){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
			if( spell_fizzled != 1 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_GOOD_TO_PUT_IN_GRAVE, -1, &this_test);
				if( selected != -1 ){
					discard_card(player, selected);
				}
				else{
					 spell_fizzled = 1;
				}
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION){
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

int card_thalakos_drifters(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 0, 0, 0, SP_KEYWORD_SHADOW);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST0, 0, NULL, NULL);
}

int card_thalakos_scout(int player, int card, event_t event){
   shadow(player, card, event);
   return card_ephemeron(player, card, event);
}

int card_theft_of_dreams(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0 );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card );

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.state = STATE_TAPPED;

			draw_cards(player, check_battlefield_for_special_card(player, card, instance->targets[0].player, CBFSC_GET_COUNT, &this_test));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_thrull_surgeon(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			new_effect_coercion(&this_definition, &this_test);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_SORCERY_BE_PLAYED|GAA_SACRIFICE_ME|GAA_CAN_TARGET, MANACOST_XB(1, 1), 2, &td, "TARGET_PLAYER");
}

int card_treasure_hunter(int player, int card, event_t event){
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		return generic_type_recycler_creature(player, card, event, &this_test);
	}
	return 0;
}

int card_treasure_trove(int player, int card, event_t event){
	/*
	  Treasure Trove |2|U|U
	  Enchantment
	  {2}{U}{U}: Draw a card.
	*/
	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST_XU(2, 2), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_vampire_hounds(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	char msg[100] = "Select a creature card to discard.";
	test_definition_t this_test2;
	new_default_test_definition(&this_test2, TYPE_CREATURE, msg);
	this_test2.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return check_battlefield_for_special_card(player, card, player, 0, &this_test2);
		}
	}

	if( event == EVENT_ACTIVATE){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test2);
			if( selected != -1 ){
				discard_card(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
	}

	return 0;
}

static int volraths_dungeon_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		if( leaves_play(instance->targets[0].player, instance->targets[0].card, event) ){
			kill_card(player, card, KILL_REMOVE);
		}

		if( event == EVENT_CAN_ACTIVATE ){
			if( can_use_activated_abilities(instance->targets[0].player, instance->targets[0].card) && current_turn == player ){
				int c1 = get_cost_mod_for_activated_abilities(instance->targets[0].player, instance->targets[0].card, MANACOST0);
				if( has_mana(player, COLOR_COLORLESS, c1) ){
					return can_pay_life(player, 5);
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			int c1 = get_cost_mod_for_activated_abilities(instance->targets[0].player, instance->targets[0].card, MANACOST0);
			charge_mana(player, COLOR_COLORLESS, c1);
			if( spell_fizzled != 1 ){
				lose_life(player, 5);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}

	}

	return 0;
}

int card_volraths_dungeon(int player, int card, event_t event){
	/*
	  Volrath's Dungeon |2|B|B
	  Enchantment
	  Pay 5 life: Destroy Volrath's Dungeon. Any player may activate this ability but only during his or her turn.
	  Discard a card: Target player puts a card from his or her hand on top of his or her library. Activate this ability only any time you could cast a sorcery.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int fake = add_card_to_hand(1-player, instance->internal_card_id);
		int legacy = create_legacy_activate(1-player, fake, &volraths_dungeon_legacy);
		card_instance_t *leg = get_card_instance(1-player, legacy);
		leg->targets[0].player = player;
		leg->targets[0].card = card;
		leg->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_DISCARD | GAA_CAN_SORCERY_BE_PLAYED | GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL) ){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_IN_YOUR_TURN, MANACOST0, 5, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		int abilities[2] = {	generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_DISCARD | GAA_CAN_SORCERY_BE_PLAYED | GAA_CAN_TARGET,
								MANACOST0, 0, &td1, NULL),
								generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_IN_YOUR_TURN, MANACOST0, 5, NULL, NULL)
		};
		choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
						"Targeted ability", abilities[0], 2,
						"Destroy Volrath's Dungeon", abilities[1], 1);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == 1 ){
			generic_activated_ability(player, card, event, GAA_DISCARD | GAA_CAN_SORCERY_BE_PLAYED | GAA_CAN_TARGET, MANACOST0, 0, &td1, "TARGET_PLAYER");
			if( spell_fizzled != 1 ){
				instance->info_slot = choice;
			}
		}
		if( choice == 2 ){
			generic_activated_ability(player, card, event, GAA_IN_YOUR_TURN, MANACOST0, 5, NULL, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( validate_target(player, card, &td1, 0) && hand_count[instance->targets[0].player] > 0 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put on top.");
				new_global_tutor(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, TUTOR_DECK, 1, AI_MIN_VALUE, &this_test);
			}
		}
		if( instance->info_slot == 2 ){
			kill_card(instance->parent_controller, instance->parent_card, KILL_DESTROY);
		}
	}

	return 0;
}

static int won_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( trigger_condition == TRIGGER_LEAVE_PLAY ){
		if( affect_me( player, card) && trigger_cause == c && trigger_cause_controller == p &&
			reason_for_trigger_controller == p
		  ){
			if( event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					int i;
					for(i=2; i<instance->targets[1].card; i++){
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

int card_wall_of_nets(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_my_legacy(player, card, &won_legacy);
		instance->targets[3].card = legacy;
	}

	if( trigger_condition == TRIGGER_END_COMBAT && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		int trig = 0;
		if( instance->blocking < 255 ){
			trig = 1;
		}
		if( trig == 1 ){
			if( event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					if( ! is_token(1-player, instance->blocking) ){
						create_card_name_legacy(player, card, cards_data[get_original_internal_card_id(1-player, instance->blocking)].id);
					}
					exile_permanent_and_remember_it(player, instance->targets[3].card, 1-player, instance->blocking, 1);
			}
		}
	}

	return 0;
}

int card_welkin_hawk(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		char msg[100] = "Select a Wekin Hawk card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		this_test.id = get_id(player, card);

	   new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
	}

	return 0;
}

int card_workhorse(int player, int card, event_t event){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 4);

	if( event == EVENT_CAN_ACTIVATE && count_1_1_counters(player, card) > 0 ){
	   return can_produce_mana(player, card);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && count_1_1_counters(player, card) > 0 && can_produce_mana(player, card)){
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}

	if( event == EVENT_ACTIVATE){
		remove_1_1_counter(player, card);
		produce_mana(player, COLOR_COLORLESS, 1);
	}

	return 0;
}

int card_zealots_en_dal(int player, int card, event_t event){//UNUSEDCARD

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = 1;

		test_definition_t this_test2;
		default_test_definition(&this_test2, TYPE_LAND);
		this_test2.type_flag = 1;
		this_test2.color = COLOR_TEST_WHITE;

		if( check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test) ==
			check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test2)
		  ){
			gain_life(player, 1);
		}
	}

	return 0;
}

// ---- DOES NOT WORK

int card_limited_resources(int player, int card, event_t event){
	/*
	  Limited Resources |W
	  Enchantment
	  When Limited Resources enters the battlefield, each player chooses five lands he or she controls and sacrifices the rest.
	  Players can't play lands as long as ten or more lands are on the battlefield.
	*/
	if( in_play(player, card) ){
		player_bits[player] |= PB_COUNT_TOTAL_PLAYABLE_LANDS;
		player_bits[1-player] |= PB_COUNT_TOTAL_PLAYABLE_LANDS;

		if( !(land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED) && landsofcolor_controlled[0][COLOR_ANY]+landsofcolor_controlled[1][COLOR_ANY] >= 10){
			land_can_be_played |= LCBP_LAND_HAS_BEEN_PLAYED;
		}
	}

	if( leaves_play(player, card, event) && lands_played < total_playable_lands(current_turn) ){
		land_can_be_played &= ~LCBP_LAND_HAS_BEEN_PLAYED;
		player_bits[player] &= ~PB_COUNT_TOTAL_PLAYABLE_LANDS;
		player_bits[1-player] &= ~PB_COUNT_TOTAL_PLAYABLE_LANDS;
	}

	if( comes_into_play(player, card, event) ){
		APNAP(p, {
					int amount = count_subtype(p, TYPE_LAND, -1);
					if( amount > 5 ){
						impose_sacrifice(player, card, p, amount-5, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
					}
				};
			);
	}

	return global_enchantment(player, card, event);
}

int card_dominating_licid(int player, int card, event_t event){
	/*
	  Dominating Licid |1|U|U
	  Creature - Licid 1/1
	  {1}{U}{U}, {T}: Dominating Licid loses this ability and becomes an Aura enchantment with enchant creature.
	  Attach it to target creature. You may pay {U} to end this effect.

	  You control enchanted creature.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) && instance->targets[2].card > 0 ){
		event_result = instance->targets[2].card;
	}

	if( ! is_what(player, card, TYPE_CREATURE) ){
		if( leaves_play(player, card, event) ){
			if( instance->targets[3].card > -1 ){
				kill_card(instance->targets[3].player, instance->targets[3].card, KILL_REMOVE);
			}
		}

		if( ! IS_AURA_EVENT(player, card, event) && ! IS_GAA_EVENT(event) ){
			return 0;
		}
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		if( instance->damage_target_player > -1 ){
			if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
				return generic_activated_ability(player, card, event, 0, MANACOST_U(1), 0, NULL, NULL);
			}

			if( event == EVENT_RESOLVE_ACTIVATION ){
				card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
				reset_subtypes(instance->parent_controller, instance->parent_card, 2);
				parent->targets[2].card = parent->damage_target_player = parent->damage_target_card = parent->targets[3].card = -1;
				if( instance->targets[3].card > -1 ){
					kill_card(instance->targets[3].player, instance->targets[3].card, KILL_REMOVE);
				}
			}
			return generic_stealing_aura(player, card, event, &td, "TARGET_CREATURE");
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, MANACOST_XU(1, 2), 0, &td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
			int newtype = create_a_card_type(parent->internal_card_id);
			cards_at_7c7000[newtype]->type = TYPE_ENCHANTMENT;
			cards_at_7c7000[newtype]->power = 0;
			cards_at_7c7000[newtype]->toughness = 0;
			parent->targets[2].card = newtype;
			add_a_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_AURA_CREATURE);
			gain_control_and_attach_as_aura(instance->parent_controller, instance->parent_card, event, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_trasmogrifying_licid(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) && instance->targets[3].player > 0 ){
		event_result = instance->targets[3].player;
	}

	if( ! is_what(player, card, TYPE_CREATURE) ){
		if( instance->damage_target_player > -1 ){
			int p = instance->damage_target_player;
			int c = instance->damage_target_card;

			if( p != instance->targets[2].player || c != instance->targets[2].card ){
				instance->dummy3 = -1;
				instance->targets[2].player = p;
				instance->targets[2].card = c;
			}

			if( ! is_what(-1, get_original_internal_card_id(p, c), TYPE_ARTIFACT) ){
				if( event == EVENT_CHANGE_TYPE && affect_me(p, c) ){
					if( instance->dummy3 == -1 ){
						int newtype = create_a_card_type(get_original_internal_card_id(p, c));
						cards_at_7c7000[newtype]->type |= (cards_data[get_original_internal_card_id(p, c)].type | TYPE_ARTIFACT | TYPE_CREATURE);
						cards_at_7c7000[newtype]->power = get_base_power(p, c);
						cards_at_7c7000[newtype]->toughness = get_base_toughness(p, c);
						instance->dummy3 = newtype;
					}
					event_result = instance->dummy3;
				}
			}

			if( leaves_play(p, c, event) ){
				kill_card(player, card, KILL_SACRIFICE);
			}
		}

		if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
			reset_subtypes(instance->parent_controller, instance->parent_card, 2);
			parent->dummy3 = parent->damage_target_player = parent->damage_target_card = -1;
			parent->targets[3].player = -1;
		}

		if( event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_CAN_MOVE_AURA || event == EVENT_MOVE_AURA || event == EVENT_RESOLVE_MOVING_AURA ){
			return generic_aura(player, card, event, player, 1, 1, 0, 0, 0, 0, 0);
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
			if( parent->targets[3].card == -1 ){
				int newtype = create_a_card_type(parent->internal_card_id);
				cards_at_7c7000[newtype]->type = TYPE_ENCHANTMENT;
				cards_at_7c7000[newtype]->power = 0;
				cards_at_7c7000[newtype]->toughness = 0;
				parent->targets[3].card = newtype;
			}
			parent->targets[3].player = parent->targets[3].card;
			add_a_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_AURA_CREATURE);
			attach_aura_to_target(instance->parent_controller, instance->parent_card, event, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, "TARGET_CREATURE");
}



