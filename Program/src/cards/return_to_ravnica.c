#include "manalink.h"

// Functions
static int detain_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	cannot_attack(instance->targets[0].player, instance->targets[0].card, event);
	cannot_block(instance->targets[0].player, instance->targets[0].card, event);

	if (event == EVENT_BEGIN_TURN && current_turn == player){
		disable_all_activated_abilities(instance->targets[0].player, instance->targets[0].card, 0);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int detain(int player, int card, int t_player, int t_card){
	disable_all_activated_abilities(t_player, t_card, 1);
	return create_targetted_legacy_effect(player, card, &detain_legacy, t_player, t_card);
}

static const char* target_is_token_too(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  // As there's no way to require a given target to have more than one of the types in required_type, we can't just use TARGET_TYPE_TOKEN.
  return is_token(player, card) ? NULL : "token";
}

void populate(int player, int card)
{
  if (player == AI || ai_is_speculating == 1)
	{
	  test_definition_t this_test;
	  default_test_definition(&this_test, TYPE_CREATURE);
	  this_test.type_flag = F1_IS_TOKEN;

	  int result = check_battlefield_for_special_card(player, card, player, CBFSC_AI_MAX_VALUE, &this_test);

	  if (result > -1)
		copy_token(player, card, player, result);
	}
  else
	{
	  target_definition_t td;
	  base_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.allow_cancel = 0;
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td.extra = (int)target_is_token_too;

	  target_t tgt;

	  if (can_target(&td) && select_target(player, card-1000, &td, "Select a creature token you control.", &tgt))
		copy_token(player, card, tgt.player, tgt.card);
	}
}

int overload(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_MODIFY_COST ){
		int cless = get_updated_casting_cost(player, card, -1, event, colorless);
		if( has_mana_multi(player, cless, black, blue, green, red, white) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
	}
	return 0;
}

int pay_overload(int player, int card, int colorless, int black, int blue, int green, int red, int white){

	card_instance_t *instance = get_card_instance(player, card);

	if( played_for_free(player, card) || is_token(player, card) || instance->info_slot == 0 ){
		return 1;
	}

	int double_x = 0;
	if( colorless == -2 ){
		double_x = 1;
		colorless = 0;
	}
	int cless = get_updated_casting_cost(player, card, -1, EVENT_CAST_SPELL, colorless);
	if( has_mana_multi(player, cless, black, blue, green, red, white) ){
		int choice = do_dialog(player, player, card, -1, -1, " Pay Overload cost\n Play the spell normally\n Cancel", 0);
		if( choice == 0 ){
			charge_mana_multi(player, cless, black, blue, green, red, white);
			if( spell_fizzled != 1 ){
				if( double_x ){
					instance->targets[1].player = charge_mana_for_double_x(player, COLOR_COLORLESS);
					if( spell_fizzled == 1 ){
						return 0;
					}
				}
				return 2;
			}
		}
		else if( choice == 1 ){
				return charge_mana_from_id(player, card, EVENT_CAST_SPELL, get_id(player, card));
		}
		else{
			spell_fizzled = 1;
		}
	}
	return 0;
}

int unleash(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		int choice = do_dialog(player, player, card, -1, -1, " Give a +1/+1 counter\n Pass", 0);
		if( choice == 0 ){
			add_1_1_counter(player, card);
		}
	}

	if( count_1_1_counters(player, card) > 0 ){
		cannot_block(player, card, event);
	}
	return 0;
}

int scavenge(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi( player, colorless, black, blue, green, red, white );
		if( spell_fizzled != 1){
			return GAPAID_EXILE;
		}
	}

	if (event != EVENT_GRAVEYARD_ABILITY && event != EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY){
	  return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if(event == EVENT_GRAVEYARD_ABILITY){
		if( has_mana_multi( player, colorless, black, blue, green, red, white ) && can_sorcery_be_played(player, event) && can_target(&td)){
			return GA_SCAVENGE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		if( can_target(&td) ){
			td.allow_cancel = 0;
			if( pick_target(&td, "TARGET_CREATURE") ){
				card_instance_t *instance = get_card_instance(player, card);
				int plus = get_base_power_iid(player, instance->internal_card_id);
				add_1_1_counters(instance->targets[0].player, instance->targets[0].card, plus);
			}
		}
	}

	return 0;
}

int keyrune(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white,
			int pow, int tou, int key, int s_key, int clr, int subtype){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_COUNT_MANA && affect_me(player, card)  ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, cless, black, blue, green, red, white, 0, 0, 0) ){
			return 1;
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int abilities[2] = {0, 0};
		if( mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			abilities[0] = 1;
		}
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_TYPE_CHANGE, cless, black, blue, green, red, white, 0, 0, 0) ){
			abilities[1] = 1;
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		int choice = 1;
		if( ! paying_mana() && abilities[1] ){
			choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Get mana", abilities[0], 1,
							"Animate", abilities[1], 5);
		}
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		instance->info_slot = choice;
		if( instance->info_slot == 1 ){
			return mana_producer(player, card, event);
		}
		if( instance->info_slot == 2){
			set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
			charge_mana_multi(player, cless, black, blue, green, red, white);
			remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 2 ){
			card_instance_t *parent = get_card_instance(player, instance->parent_card);
			add_a_subtype(player, instance->parent_card, subtype);
			int legacy = artifact_animation(player, instance->parent_card, player, instance->parent_card, 1, pow, tou, key, s_key);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[6].player = clr;
			parent->targets[1].player = 66;
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

// Cards

// White
int card_angel_of_serenity(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		int trgts = 3;
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		state_untargettable(player, card, 1);
		while( trgts > 0 && can_target(&td) ){
				if( select_target(player, card, &td, "Select target creature to exile.", NULL) ){
					instance->number_of_targets = 1;
					int result = rfg_target_permanent(instance->targets[0].player, instance->targets[0].card);
					if( result != -1 ){
						if( instance->targets[1].player < 2 ){
							instance->targets[1].player = 2;
						}
						int pos = instance->targets[1].player;
						instance->targets[pos].player = instance->targets[0].player;
						if( result > available_slots ){
							result-=available_slots;
							instance->targets[pos].player = 1-instance->targets[0].player;
						}
						create_card_name_legacy(player, card, result);
						instance->targets[pos].card = get_internal_card_id_from_csv_id(result);
						instance->targets[1].player++;
					}
					trgts--;
				}
				else{
					break;
				}
		}
		if( trgts > 0 ){
			if( ! graveyard_has_shroud(2) ){
				int cg[2];
				cg[1-player] = count_graveyard_by_type(1-player, TYPE_CREATURE);
				cg[player] = count_graveyard_by_type(player, TYPE_CREATURE);

				target_definition_t td1;
				default_target_definition(player, card, &td1, TYPE_CREATURE);
				td1.zone = TARGET_ZONE_PLAYERS;
				td1.illegal_abilities = 0;

				while( trgts > 0 && cg[player]+cg[1-player] > 0 ){
						instance->targets[0].player = 1-player;
						instance->targets[0].card = -1;
						if( cg[1-player] > 0 ){
							if( cg[player] > 0 && player != AI ){
								if( ! select_target(player, card, &td1, "Select the owner of the graveyard you want to search into.", NULL) ){
									break;
								}
							}
						}
						else{
							if( player == AI ){
								break;
							}
							else{
								instance->targets[0].player = player;
							}
						}
						char msg[100] = "Select a creature card.";
						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_CREATURE, msg);
						int selected = new_select_a_card(player, instance->targets[0].player, TUTOR_FROM_GRAVE, 0, AI_MAX_VALUE, -1, &this_test);
						if( selected > -1 ){
							const int *grave = get_grave(instance->targets[0].player);
							if( instance->targets[1].player < 2 ){
								instance->targets[1].player = 2;
							}
							int pos = instance->targets[1].player;
							instance->targets[pos].player = instance->targets[0].player;
							instance->targets[pos].card = grave[selected];
							instance->targets[1].player++;
							trgts--;
							create_card_name_legacy(player, card, cards_data[grave[selected]].id);
							rfg_card_from_grave(instance->targets[0].player, selected);
							cg[instance->targets[0].player]--;
						}
						else{
							break;
						}
				}

			}
		}
		state_untargettable(player, card, 0);
	}

	if( leaves_play(player, card, event) ){
		int count = instance->targets[1].player-1;
		while( count > 1 ){
				if( check_rfg(instance->targets[count].player, cards_data[instance->targets[count].card].id) ){
					remove_card_from_rfg(instance->targets[count].player, cards_data[instance->targets[count].card].id);
					add_card_to_hand(instance->targets[count].player, instance->targets[count].card);
				}
				count--;
		}
	}
	return 0;
}

int card_armory_guard(int player, int card, event_t event){

	if (event == EVENT_ABILITIES && affect_me(player, card) && check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_GATE)){
		vigilance(player, card, event);
	}

	return 0;
}

int card_ash_zealot(int player, int card, event_t event){
	/*
	  Ash Zealot |R|R
	  Creature - Human Warrior 2/2
	  First strike, haste
	  Whenever a player casts a spell from a graveyard, Ash Zealot deals 3 damage to that player.
	*/
	haste(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( check_special_flags(trigger_cause_controller, trigger_cause, SF_PLAYED_FROM_GRAVE) ){
			if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				damage_player(get_card_instance(player, card)->targets[1].player, 3, player, card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_azorius_arrester(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( can_target(&td) && comes_into_play(player, card, event) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			detain(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_azorius_justiciar(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( can_target(&td) && comes_into_play(player, card, event) ){
		int trgs = 0;
		while( trgs < 2 && can_target(&td) ){
				if( select_target(player, card, &td, "Select target creature an opponent controls.", &(instance->targets[trgs])) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		int i;
		for(i=0; i<trgs; i++){
			if( instance->targets[i].player != -1 ){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
				detain(player, card, instance->targets[i].player, instance->targets[i].card);
			}
		}
	}
	return 0;
}

int card_bazaar_krovod(int player, int card, event_t event)
{
  // Whenever ~ attacks, another target attacking creature gets +0/+2 until end of turn. Untap that creature.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;
	  td.allow_cancel = 0;
	  td.required_state = TARGET_STATE_ATTACKING;
	  td.special = TARGET_SPECIAL_NOT_ME;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_ANOTHER_CREATURE"))
		{
		  pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0,2);
		  untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

  return 0;
}

// concordia pegasus --> vanilla

int card_ethereal_armor(int player, int card, event_t event)
{
  /* Ethereal Armor	|W
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +1/+1 for each enchantment you control and has first strike. */

  card_instance_t* instance;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_ABILITIES)
	  && (instance = in_play(player, card))
	  && affect_me(instance->damage_target_player, instance->damage_target_card))
	{
	  if (event == EVENT_ABILITIES)
		event_result |= KEYWORD_FIRST_STRIKE;
	  else
		event_result += count_subtype(player, TYPE_ENCHANTMENT, -1);
	}

  return vanilla_aura(player, card, event, player);
}

int card_eyes_in_the_sky(int player, int card, event_t event){
	/* Eyes in the Skies	|3|W
	 * Instant
	 * Put a 1/1 |Swhite Bird creature token with flying onto the battlefield, then populate. */

	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			generate_token_by_id(player, card, CARD_ID_BIRD);
			populate(player, card);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// fencing ace --> vanilla

// keening apparition --> kami of the ancient law

int card_knightly_valor(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_KNIGHT, &token);
			token.pow = token.tou = 2;
			token.s_key_plus = SP_KEYWORD_VIGILANCE;
			generate_token(&token);
		}
	}
	return generic_aura(player, card, event, player, 2, 2, 0, SP_KEYWORD_VIGILANCE, 0, 0, 0);
}

int card_martial_law(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			detain(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_palisade_giant(int player, int card, event_t event){

  /* Palisade Giant	|4|W|W
   * Creature - Giant Soldier 2/7
   * All damage that would be dealt to you or another permanent you control is dealt to ~ instead. */

  card_instance_t* damage = damage_being_prevented(event);
  if (damage && damage->damage_target_player == player)
	damage->damage_target_card = card;

  return 0;
}

int card_phantom_general(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player && is_token(affected_card_controller, affected_card) ){
		event_result++;
	}

	return 0;
}

int card_precint_captain(int player, int card, event_t event){
	/* Precinct Captain	|W|W
	 * Creature - Human Soldier 2/2
	 * First strike
	 * Whenever ~ deals combat damage to a player, put a 1/1 |Swhite Soldier creature token onto the battlefield. */

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		generate_token_by_id(player, card, CARD_ID_SOLDIER);
	}

	return 0;
}

int card_rest_in_peace(int player, int card, event_t event){

  /* Rest in Peace	|1|W
   * Enchantment
   * When ~ enters the battlefield, exile all cards from all graveyards.
   * If a card or token would be put into a graveyard from anywhere, exile it instead. */

  test_definition_t test, *test_ptr;
  if (event == EVENT_GRAVEYARD_FROM_PLAY)
	{
	  new_default_test_definition(&test, 0, "");	// Give it an empty test, not null (which would make it not affect tokens)
	  test_ptr = &test;
	}
  else
	test_ptr = NULL;

  if_a_card_would_be_put_into_graveyard_from_anywhere_exile_it_instead(player, card, event, ANYBODY, test_ptr);

  if (comes_into_play(player, card, event))
	{
	  rfg_whole_graveyard(current_turn);
	  rfg_whole_graveyard(1-current_turn);
	}

  return global_enchantment(player, card, event);
}

int card_rootborn_defenses(int player, int card, event_t event){

	/* Rootborn Defenses	|2|W
	 * Instant
	 * Populate. Creatures you control gain indestructible until end of turn. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		populate(player, card);
		pump_creatures_until_eot(player, card, player, 0, 0,0, 0,SP_KEYWORD_INDESTRUCTIBLE, NULL);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_security_blockade(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=100;
		pick_target(&td, "TARGET_LAND");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_KNIGHT, &token);
			token.s_key_plus = SP_KEYWORD_VIGILANCE;
			token.pow = token.tou = 2;
			generate_token(&token);
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

		target_definition_t td1;
		default_target_definition(t_player, t_card, &td1, TYPE_CREATURE);
		td1.extra = damage_card;
		td1.special = TARGET_SPECIAL_DAMAGE_PLAYER;
		td1.required_type = 0;

		card_instance_t *trg = get_card_instance(t_player, t_card);

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_DAMAGE_PREVENTION, 0, 0, 0, 0, 0, 0, 0,
											&td1, "TARGET_DAMAGE");
		}
		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_DAMAGE_PREVENTION_ME, 0, 0, 0, 0, 0, 0, 0,
											&td1, "TARGET_DAMAGE");
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *target = get_card_instance(trg->targets[0].player, trg->targets[0].card);
			if( target->info_slot > 0 ){
				target->info_slot--;
			}
		}
	}

	return 0;
}

int card_selesnya_sentry(int player, int card, event_t event){
	return regeneration(player, card, event, 5, 0, 0, 1, 0, 0);
}

int card_seller_of_songbirds(int player, int card, event_t event){
	/* Seller of Songbirds	|2|W
	 * Creature - Human 1/2
	 * When ~ enters the battlefield, put a 1/1 |Swhite Bird creature token with flying onto the battlefield. */

	if( comes_into_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_BIRD);
	}

	return 0;
}

int card_soul_tithe(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

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

		upkeep_trigger_ability(player, card, event, t_player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int sac = 1;
			int cmc = get_cmc(t_player, t_card);
			if( has_mana(t_player, COLOR_COLORLESS, cmc) ){
				charge_mana(t_player, COLOR_COLORLESS, cmc);
				if( spell_fizzled != 1 ){
					sac = 0;
				}
			}
			if( sac == 1 ){
				kill_card(t_player, t_card, KILL_SACRIFICE);
			}
		}
	}

	return 0;
}

// sphere of safety --> propaganda.

// sunspire griffin --> vanilla

int card_swift_justice(int player, int card, event_t event){
	/* Swift Justice	|W
	 * Instant
	 * Until end of turn, target creature gets +1/+0 and gains first strike and lifelink. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 1, 0, KEYWORD_FIRST_STRIKE, SP_KEYWORD_LIFELINK);
}

// trained caracal --> child of night

int card_trostanis_judgement(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			populate(player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Blue
int card_aquus_steed(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -2, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 1, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_blustersquall(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XU(2, 1));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target creature an opponent controls.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				new_manipulate_all(player, card, 1-player, &this_test, ACT_TAP);
			}
			else{
				if( valid_target(&td) ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XU(3, 1));
}

int card_chronic_flooding(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

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

	if( in_play(player, card) && instance->targets[0].player != -1 ){
		if( event == EVENT_TAP_CARD && affect_me(instance->targets[0].player, instance->targets[0].card) ){
			mill(instance->targets[0].player, 3);
		}
	}

	return 0;
}

int card_crosstown_courier(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_REPORT_DAMAGE_DEALT) ){
		mill(1-player, instance->targets[16].player);
		instance->targets[16].player = 0;
	}

	return 0;
}

int card_cyclonic_rift(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XU(6, 1));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target nonland permanent your opponent controls.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_LAND);
				this_test.type_flag = DOESNT_MATCH;
				new_manipulate_all(player, card, 1-player, &this_test, ACT_BOUNCE);
			}
			else{
				if( valid_target(&td) ){
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XU(6, 1));
}

// dispel --> flash counter

int card_doorkeeper(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, count_defenders(player));
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 1, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_downsize(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XU(2, 1));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target creature an opponent controls.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				pump_creatures_until_eot(player, card, 1-player, 0, -4, 0, 0, 0, &this_test);
			}
			else{
				if( valid_target(&td) ){
					pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -4, 0);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XU(2, 1));
}

int card_faerie_impostor(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		int sac = 1;
		state_untargettable(player, card, 1);
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			sac = 0;
		}
		if( sac == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
		state_untargettable(player, card, 0);
	}
	return 0;
}

// hover barrier --> vanilla

int card_inaction_injunction(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			detain(player, card, instance->targets[0].player, instance->targets[0].card);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// isperia skywatch --> azorius arrester
static int jace_weaken_attackers(int player, int card, event_t event){
	if( current_turn != player ){
	  int amt;
	  if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, 1-player, -1)))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		  for (--amt; amt >= 0; --amt)
			if (in_play(current_turn, attackers[amt]))
			  pump_until_eot(player, card, current_turn, attackers[amt], -1, 0);
		}
	}
	if (event == EVENT_BEGIN_TURN && current_turn == player){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}


int card_jace_architect_of_thought(int player, int card, event_t event){

	/* Jace, Architect of Thought	|2|U|U
	 * Planeswalker - Jace (4)
	 * +1: Until your next turn, whenever a creature an opponent controls attacks, it gets -1/-0 until end of turn.
	 * -2: Reveal the top three cards of your library. An opponent separates those cards into two piles. Put one pile into your hand and the other on the bottom of your library in any order.
	 * -8: For each player, search that player's library for a nonland card and exile it, then that player shuffles his or her library. You may cast those cards without paying their mana costs. */


	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{

		enum {
			CHOICE_WEAKEN = 1,
			CHOICE_LFOF,
			CHOICE_SSPELL
		}

		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Weaken attackers", 1, count_subtype(1-player, TYPE_CREATURE, -1)*5, 1,
						"Little FoF", 1, 15, -2,
						"Steal a Spell", 1, 25, -8);

		if (event == EVENT_CAN_ACTIVATE){
			if (!choice)
				return 0;	// else fall through to planeswalker()
		}
		else if (event == EVENT_RESOLVE_ACTIVATION){// event == EVENT_RESOLVE_ACTIVATION

			if( choice == CHOICE_WEAKEN )
				create_legacy_effect(instance->parent_controller, instance->parent_card, &jace_weaken_attackers);

			else if( choice == CHOICE_LFOF )
				effect_fof(player, player, 3, TUTOR_BOTTOM_OF_DECK);

			else{
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_INTERRUPT | TYPE_LAND, "Select a nonland (non-interrupt) card to exile.");
				this_test.type_flag = DOESNT_MATCH;

				int i, selected[2];
				for (i = 0; i <= 1; i++){
					selected[i] = new_global_tutor(player, i, TUTOR_FROM_DECK, TUTOR_RFG, AI_MAX_CMC, -1, &this_test);
				}
				while( selected[0] != -1 || selected[1] != -1 ){
						int can_play_exiled_spell[2] = {	(selected[0] != -1 && can_legally_play_csvid(player, selected[0])),
															(selected[1] != -1 && can_legally_play_csvid(player, selected[1]))
						};
						int spell_to_play = DIALOG(player, card, event,	DLG_RANDOM, DLG_OMIT_ILLEGAL, DLG_NO_CANCEL, DLG_NO_STORAGE,
											selected[0] == -1 ? "" : get_card_name_by_id(selected[0]), can_play_exiled_spell[0], 3,
											selected[1] == -1 ? "" : get_card_name_by_id(selected[1]), can_play_exiled_spell[1], 3,
											"Done", (event != EVENT_CAN_CAST), 1);
						if( spell_to_play == 3 ){
							break;
						}
						else{
							int csvid = selected[spell_to_play- 1];
							selected[spell_to_play - 1] = -1;
							play_card_in_exile_for_free(player, spell_to_play - 1, csvid);
						}
				}

			}
		}
	}

	if( event == EVENT_BEGIN_TURN && current_turn == player ){
		instance->targets[1].player = 0;
	}

	return planeswalker(player, card, event, 4);
}

int card_mizzium_skin(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XU(1, 1));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				pump_creatures_until_eot(player, card, player, 0, 0, 1, 0, SP_KEYWORD_HEXPROOF, &this_test);
			}
			else{
				if( valid_target(&td) ){
					pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 1, 0, SP_KEYWORD_HEXPROOF);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XU(1, 1));
}

int card_psychic_spiral(int player, int card, event_t event){

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


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int result = reshuffle_grave_into_deck(player, 0);
			mill(instance->targets[0].player, result);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_runewing(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		draw_cards(player, 1);
	}
	return 0;
}

int card_search_the_city(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		instance->targets[6].player = 0;
		int n = 5;
		int *deck = deck_ptr[player];
		if( n > count_deck(player) ){
			n = count_deck(player);
		}
		while( n > 0 ){
				instance->targets[n].player = cards_data[deck[n]].id;
				instance->targets[n].card = create_card_name_legacy(player, card, cards_data[deck[n]].id);
				rfg_card_in_deck(player, n);
				n--;
				instance->targets[6].player++;
		}
		instance->targets[6].card = 66;
	}

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller ){
		int trig = 0;
		int pos = 0;
		if( trigger_cause_controller == player ){
			int i;
			for(i=1; i<6; i++){
				if( instance->targets[i].player != -1 && get_id(trigger_cause_controller, trigger_cause) == instance->targets[i].player &&
					check_rfg(player, instance->targets[i].player)
				  ){
					trig = 1;
					pos = i;
					break;
				}
			}
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					add_card_to_hand(player, get_internal_card_id_from_csv_id(instance->targets[pos].player));
					remove_card_from_rfg(player, instance->targets[pos].player);
					kill_card(player, instance->targets[pos].card, KILL_REMOVE);
					instance->targets[pos].player = -1;
					instance->targets[6].player--;
			}
		}
	}

	if( instance->targets[6].card == 66 && instance->targets[6].player < 1 ){
		return card_time_walk(player, card, EVENT_RESOLVE_SPELL);
	}

	return global_enchantment(player, card, event);
}

// skyline predator --> a creature with flash

int card_soulsworn_spirit(int player, int card, event_t event){
	unblockable(player, card, event);
	return card_azorius_arrester(player, card, event);
}

int card_sphinx_of_the_chimes(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		int i;
		int id_map[100];
		int id_map_count = 0;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && ! is_what(player, i, TYPE_LAND) ){
				int id = get_id(player, i);
				if( id_map_count > 0 ){
					int k;
					for(k=0; k<id_map_count; k++){
						if( id_map[k] == id ){
							instance->targets[1].card = i;
							return 1;
						}
					}
				}
				id_map[id_map_count] = id;
				id_map_count++;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( player != AI ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_LAND);
				this_test.type_flag = 1;
				instance->targets[1].card = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
				if( instance->targets[1].card == -1 ){
					spell_fizzled = 1;
					return 0;
				}

			}
			int good = 0;
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_hand(player, i) && ! is_what(player, i, TYPE_LAND) && i != instance->targets[1].card ){
					if( get_id(player, i) == get_id(player, instance->targets[1].card) ){
						instance->targets[2].card = i;
						good = 1;
						break;
					}
				}
			}
			if( good == 1 ){
				for(i=1; i<3; i++){
					discard_card(player, instance->targets[i].card);
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 4);
	}

	return 0;
}

int card_stealer_of_secrets(int player, int card, event_t event){

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_tower_drake(int player, int card, event_t event){

	return generic_shade(player, card, event, 0, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0);
}

int card_voidwielder(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

// Black
int card_assassins_strike(int player, int card, event_t event){

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
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			discard(instance->targets[0].player, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// catacomb slug --> vanilla

// daggerdrome imp --> child of night

int card_dark_revenant(int player, int card, event_t event){

	if (graveyard_from_play(player, card, event)){
		put_on_top_of_deck(player, card);
		event_result = 1;
	}

	return 0;
}

int card_dark_reveler(int player, int card, event_t event){
	// dead reveler
	return unleash(player, card, event);
}


int card_desecration_demon(int player, int card, event_t event){

	if ( beginning_of_combat(player, card, event, ANYBODY, -1) ){
		if (can_sacrifice(player, 1-player, 1, TYPE_CREATURE, 0)){
			int ai_choice = 1;
			if (1-player == AI
				&& !is_tapped(player, card)
				&& current_turn == player
				&& !is_sick(player, card)
				&& life[1-player]-get_attack_power(player, card) < 6){
				ai_choice = 0;
			}

			int choice = do_dialog(1-player, player, card, -1, -1, " Sac to tap Desecration Demon\n Pass", ai_choice);
			if (choice == 0){
				test_definition_t test;
				new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
				int sac = new_sacrifice(player, card, 1-current_turn, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
				if (!sac){
					return 0;
				}
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
				tap_card(player, card);
				add_1_1_counter(player, card);
			}
		}
	}

	return 0;
}

int card_destroy_the_evidence(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_LAND");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			int *deck = deck_ptr[instance->targets[0].player];
			int count = 0;
			while( deck[count] != -1 && ! is_what(-1, deck[count], TYPE_LAND) ){
				count++;
			}
			show_deck( HUMAN, deck, count+1, "Cards revealed by Destroy the Evidence", 0, 0x7375B0 );
			mill(instance->targets[0].player, count+1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_deviant_glee(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(p, c, event, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
		}

		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(p, c, event, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_ability_until_eot(player, instance->parent_card, p, c, 0, 0, KEYWORD_TRAMPLE, 0);
		}
	}

	return generic_aura(player, card, event, player, 2, 1, 0, 0, 0, 0, 0);
}

int card_drainpipe_vermin(int player, int card, event_t event)
{
	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	if (can_target(&td) && this_dies_trigger(player, card, event, hand_count[1-player] > 0 ? RESOLVE_TRIGGER_AI(player) : RESOLVE_TRIGGER_OPTIONAL)){
		charge_mana(player, COLOR_BLACK, 1);
		if (cancel != 1 && pick_target(&td, "TARGET_PLAYER") ){
			discard(get_card_instance(player, card)->targets[0].player, 0, player);
		}
	}

	return 0;
}

static int grave_betrayal_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( eot_trigger(player, card, event) ){
		int zombo = seek_grave_for_id_to_reanimate(instance->targets[0].player, instance->targets[0].card, 1-player, instance->targets[1].card, REANIMATE_DEFAULT);
		if( zombo > -1 ){
			add_1_1_counter(player, zombo);
			add_a_subtype(player, zombo, SUBTYPE_ZOMBIE);
			change_color(player, card, player, zombo, COLOR_TEST_BLACK, CHANGE_COLOR_ADD|CHANGE_COLOR_NO_SOUND);
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}


int card_grave_betrayal(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affected_card_controller == 1-player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			! is_token(affected_card_controller, affected_card)
			){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				int pos = instance->targets[11].player;
				if( pos < 10 ){
					instance->targets[pos].card = get_id(affected_card_controller, affected_card);
					instance->targets[11].player++;
				}
			}
		}
	}

	if( instance->targets[11].player > 0 && resolve_graveyard_trigger(player, card, event) ){
		int i;
		for(i=0; i<instance->targets[11].player; i++){
			int legacy = create_legacy_effect(player, card, &grave_betrayal_legacy);
			card_instance_t *this = get_card_instance(player, legacy);
			this->targets[0].player = player;
			this->targets[0].card = card;
			this->number_of_targets = 1;
			this->targets[1].card = instance->targets[i].card;
		}
		instance->targets[11].player = 0;
	}

	return global_enchantment(player, card, event);
}

int card_launch_party(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			pick_target(&td, "TARGET_CREATURE");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			lose_life(instance->targets[0].player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_grim_roustabout(int player, int card, event_t event){
	unleash(player, card, event);
	return regeneration(player, card, event, 1, 1, 0, 0, 0, 0);
}

int card_necropolis_regent(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	card_instance_t* damage = combat_damage_being_dealt(event);
	if( damage &&
		damage->damage_source_player == player &&
		(damage->targets[3].player & TYPE_CREATURE) &&	// probably redundant to status check
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER) &&
		instance->info_slot < 26
		){
		unsigned char* creatures = (unsigned char*)(&instance->targets[0].player);
		creatures[3 * instance->info_slot] = damage->damage_source_player;
		creatures[3 * instance->info_slot + 1] = damage->damage_source_card;
		creatures[3 * instance->info_slot + 2] = MIN(255, damage->info_slot);	// can't store more than 255 +1+1 counters anyway, so one byte is ok
		instance->info_slot++;
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player && instance->info_slot > 0 ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			unsigned char* creatures = (unsigned char*)(&instance->targets[0].player);
			int i;
			for (i = 0; i < instance->info_slot; i++){
				add_1_1_counters(creatures[3 * i], creatures[3 * i + 1], creatures[3 * i + 2]);
			}
			instance->info_slot = 0;
		}
	}
	return 0;
}

int card_ogre_jailbreaker(int player, int card, event_t event)
{
	if (event == EVENT_ABILITIES && affect_me(player, card) && check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_GATE))
		get_card_instance(player, card)->token_status |= STATUS_WALL_CAN_ATTACK;

	return 0;
}

int card_pack_rat(int player, int card, event_t event){
	/* Pack Rat	|1|B
	 * Creature - Rat 100/100
	 * ~'s power and toughness are each equal to the number of Rats you control.
	 * |2|B, Discard a card: Put a token onto the battlefield that's a copy of ~. */

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		event_result+=count_subtype(player, TYPE_PERMANENT, SUBTYPE_RAT);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;

		card_instance_t* instance = get_card_instance(player, card);
		card_instance_t* parent = get_card_instance(instance->parent_controller, instance->parent_card);
		int iid = parent->internal_card_id;
		if (iid >= 0){
			copy_token_definition(player, card, &token, instance->parent_controller, instance->parent_card);
		} else {
			default_token_definition(player, card, cards_data[instance->backup_internal_card_id].id, &token);
			token.no_sleight = 1;
		}
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, 2, 1, 0, 0, 0, 0, 0, 0, 0);
}

int card_perilous_shadow(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 1, 1, 0, 0, 0, 0, 2, 2, 0, 0);
}

int card_sewer_shambler(int player, int card, event_t event){
	return scavenge(player, card, event, 2, 1, 0, 0, 0, 0);
}

int card_shrieking_affliction(int player, int card, event_t event){

	if( !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
		current_turn != player
	  ){
		int count = count_upkeeps(1-player);
		if(event == EVENT_TRIGGER && count > 0 && hand_count[1-player] < 2 ){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				card_instance_t *instance= get_card_instance(player, card);
				card_data_t* card_d = &cards_data[ instance->internal_card_id ];
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				while( count > 0 ){
						ptFunction(player, card, EVENT_UPKEEP_TRIGGER_ABILITY );
						count--;
				}
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(1-player, 3);
	}

	return global_enchantment(player, card, event);
}

int card_stab_wound(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){

		upkeep_trigger_ability(player, card, event, instance->damage_target_player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			lose_life(instance->damage_target_player, 2);
		}
	}

	return generic_aura(player, card, event, player, -2, -2, 0, 0, 0, 0, 0);
}

int card_tavern_swindler(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_pay_life(player, 3);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			tap_card(player, card);
			lose_life(player, 3);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int result = flip_a_coin(player, instance->parent_card);
		if( result ){
			gain_life(player, 6);
		}
	}

	return 0;
}

int card_terrus_wurm(int player, int card, event_t event){
	return scavenge(player, card, event, 6, 1, 0, 0, 0, 0);
}

int card_thrill_kill_assassin(int player, int card, event_t event){
	deathtouch(player, card, event);
	return unleash(player, card, event);
}

const char* target_is_monocolored(int who_chooses, int player, int card){
	if (num_bits_set(get_color(player, card) & COLOR_TEST_ANY_COLORED) == 1){
		return NULL;
	} else {
		return "color";
	}
}

int card_ultimate_price(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = (int32_t)target_is_monocolored;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		pick_next_target_noload(&td, "Select target monocolored creature.");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_underworld_connections(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

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

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 1, 0, 0);
		}
		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 1, 0, 0);
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_zanikev_locust(int player, int card, event_t event){
	return scavenge(player, card, event, 2, 2, 0, 0, 0, 0);
}

// Red

// Annihilating Fire => Yamabushi's Flame

// ash zealot --> raging goblin

int card_batterhorn(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);

		if (can_target(&td) && pick_target(&td, "TARGET_ARTIFACT")){
			card_instance_t* instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_bellows_lizard(int player, int card, event_t event){

	return generic_shade(player, card, event, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 0);
}

// bloodfray giant --> dark reveler

int card_chaos_imps(int player, int card, event_t event){

	if( count_1_1_counters(player, card) > 0 ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_TRAMPLE);
	}

	return unleash(player, card, event);
}

// cobblebrute --> vanilla

int card_dynacharge(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XR(2, 1));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				pump_creatures_until_eot(player, card, player, 0, 2, 0, 0, 0, &this_test);
			}
			else{
				if( valid_target(&td) ){
					pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 0, 0, 0);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XR(2, 1));
}

int card_electrickery(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XR(1, 1));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target creature your opponent controls.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				new_damage_all(player, card, 1-player, 1, NDA_ALL_CREATURES, &this_test);
			}
			else{
				if( valid_target(&td) ){
					damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XR(1, 1));
}

// explosive impact --> thunderous wrath

int card_goblin_rally(int player, int card, event_t event){
	/* Goblin Rally	|3|R|R
	 * Sorcery
	 * Put four 1/1 |Sred Goblin creature tokens onto the battlefield. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 4);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// gore-house chainwalker --> dark reveler

int card_guild_feud(int player, int card, event_t event){
	/* Guild Feud	|5|R
	 * Enchantment
	 * At the beginning of your upkeep, target opponent reveals the top three cards of his or her library, may put a creature card from among them onto the battlefield, then puts the rest into his or her graveyard. You do the same with the top three cards of your library. If two creatures are put onto the battlefield this way, those creatures fight each other. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.no_shuffle = 1;
		int i;
		int p = 1-player;
		int monster[2] = {-1, -1};
		for(i=0; i<2; i++){
			int good = 1;
			if( p == 1-player ){
				instance->targets[0].player = 1-player;
				instance->number_of_targets = 1;
				good = would_valid_target(&td);
			}
			if( good == 1 ){
				int minideck = 3;
				if( minideck > count_deck(p) ){
					minideck = count_deck(p);
				}
				if( minideck > 0 ){
					this_test.create_minideck = minideck;
					if( p == AI ){
						show_deck( HUMAN, deck_ptr[AI], minideck, "Cards revealed with Guild Feud", 0, 0x7375B0 );
					}
					monster[i] = new_global_tutor(p, p, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
					if( monster[i] != -1 ){
						minideck--;
					}
					mill(p, minideck);
				}
			}
			p = 1-p;
		}
		if( monster[0] != -1 && monster[1] != -1 ){
			fight(player, monster[1], 1-player, monster[0]);
		}
	}

	return global_enchantment(player, card, event);
}

int card_guttersnipe(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, 2, TYPE_SPELL, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		damage_player(1-player, 2, player, card);
	}

	return 0;
}

int card_lobber_crew(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0, 0, F3_MULTICOLORED, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		damage_player(1-player, 1, player, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

// minotaur aggressor --> raging goblin

int card_mizzium_mortars(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XR(3, 3));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target creature your opponent controls.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				new_damage_all(player, card, 1-player, 4, NDA_ALL_CREATURES, &this_test);
			}
			else{
				if( valid_target(&td) ){
					damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XR(3, 3));
}

int card_pursuit_of_flight(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(p, c, event, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0);
		}

		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(p, c, event, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_ability_until_eot(player, instance->parent_card, p, c, 0, 0, KEYWORD_FLYING, 0);
		}
	}

	return generic_aura(player, card, event, player, 2, 2, 0, 0, 0, 0, 0);
}

int card_pyroconvergence(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( can_target(&td) && specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0, 0, F3_MULTICOLORED, 0, 0, -1, 0) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			instance->number_of_targets = 1;
			damage_creature_or_player(player, card, event, 2);
		}
	}

	return global_enchantment(player, card, event);
}

int card_racecourse_fury(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

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

	if( in_play(player, card) && instance->targets[0].player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		target_definition_t td1;
		default_target_definition(t_player, t_card, &td1, TYPE_CREATURE);
		td1.preferred_controller = player;
		if( player == AI ){
			td1.required_state = TARGET_STATE_SUMMONING_SICK;
		}

		card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);

		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
			can_target(&td1) && ! is_tapped(instance->targets[0].player, instance->targets[0].card)
		  ){
			return 1;
		}
		if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
				select_target(instance->targets[0].player, instance->targets[0].card, &td1, "TARGET_CREATURE", &(instance->targets[1]))
			  ){
				target->number_of_targets = 1;
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(instance->targets[0].player, instance->targets[0].card, &td1, 1) ){
				pump_ability_until_eot(player, instance->parent_card, target->targets[1].player, target->targets[1].card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
		}
	}

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.preferred_controller = player;

		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		card_instance_t *trg = get_card_instance(t_player, t_card);

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE");
		}
		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE");
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( validate_target(t_player, t_card, &td1, 0) ){
				pump_ability_until_eot(player, instance->parent_card, trg->targets[0].player, trg->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
		}
	}
	return 0;
}

// splatter thug --> dark reveler

int card_street_spasm(int player, int card, event_t event){
	/*
	  Street Spasm |X|R
	  Instant
	  Street Spasm deals X damage to target creature without flying you don't control.
	  Overload {X}{X}{R}{R} (You may cast this spell for its overload cost. If you do, change its text by replacing all instances of "target" with "each.")
	*/
	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.illegal_abilities = KEYWORD_FLYING;


		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = instance->targets[1].player = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XR(-2, 2));
			}
			if( result == 1 ){
				charge_mana(player, COLOR_COLORLESS, -1);
				if( spell_fizzled != 1 && new_pick_target(&td, "Select target creature without flying your opponent controls.", 0, 1 | GS_LITERAL_PROMPT) ){
					instance->targets[1].player = x_value;
				}
				else{
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				this_test.keyword = KEYWORD_FLYING;
				this_test.keyword_flag = 1;
				new_damage_all(player, card, 1-player, instance->targets[1].player/2, 0, &this_test);
			}
			else{
				if( valid_target(&td) ){
					damage_target0(player, card, instance->targets[1].player);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XR(2, 2));
}

int card_survey_the_wreckage(int player, int card, event_t event){
	/* Survey the Wreckage	|4|R
	 * Sorcery
	 * Destroy target land. Put a 1/1 |Sred Goblin creature token onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_LAND");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			generate_token_by_id(player, card, CARD_ID_GOBLIN);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// tenement crasher --> raging goblin

int card_utvara_hellkite(int player, int card, event_t event)
{
  /* Utvara Hellkite	|6|R|R
   * Creature - Dragon 6/6
   * Flying
   * Whenever a Dragon you control attacks, put a 6/6 |Sred Dragon creature token with flying onto the battlefield. */

  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_DRAGON;

	  if (declare_attackers_trigger_test(player, card, event, DAT_SEPARATE_TRIGGERS, player, -1, &test))
		{
		  token_generation_t token;
		  default_token_definition(player, card, CARD_ID_DRAGON, &token);
		  token.pow = 6;
		  token.tou = 6;
		  generate_token(&token);
		}
	}

  return 0;
}

int card_vandalblast(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XR(5, 1));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target artifact your opponent controls.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ARTIFACT);
				new_manipulate_all(player, card, 1-player, &this_test, KILL_DESTROY);
			}
			else{
				if( valid_target(&td) ){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XR(5, 1));
}

int card_viashino_racketeer(int player, int card, event_t event){

	if( hand_count[player] > 0 && comes_into_play(player, card, event) ){
		char msg[100] = "Select a card to discard.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		int result = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
		if( result != -1 ){
			discard_card(player, result);
			draw_cards(player, 1);
		}
	}
	return 0;
}

// Green

int card_aerial_predation(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			gain_life(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature with flying.", 1, NULL);
}

// archweaver --> vanilla

int card_axebane_guardian(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && can_produce_mana(player, card) ){
			return 1;
		}
	}
	else if(event == EVENT_ACTIVATE ){
		int amount = count_defenders(player);
		produce_mana_tapped_any_combination_of_colors(player, card, COLOR_TEST_ANY_COLORED, amount, NULL);
	}
	else if( event == EVENT_COUNT_MANA ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && affect_me(player, card) && can_produce_mana(player, card) ){
			declare_mana_available_any_combination_of_colors(player, COLOR_TEST_ANY_COLORED, count_defenders(player));
		}
	}

	return 0;
}

// axebane stag --> vanilla

// brushstrider --> serra angel

int card_centaurs_herald(int player, int card, event_t event){
	/* Centaur's Herald	|G
	 * Creature - Elf Scout 0/1
	 * |2|G, Sacrifice ~: Put a 3/3 |Sgreen Centaur creature token onto the battlefield. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_CENTAUR);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 2, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_chorus_of_might(int player, int card, event_t event)
{
  /* Chorus of Might	|3|G
   * Instant
   * Until end of turn, target creature gets +1/+1 for each creature you control and gains trample. */
  int amt = (event == EVENT_CHECK_PUMP || event == EVENT_RESOLVE_SPELL) ? count_subtype(player, TYPE_CREATURE, -1) : 0;
  return vanilla_instant_pump(player, card, event, ANYBODY, player, amt, amt, KEYWORD_TRAMPLE, 0);
}

int card_deadbridge_goliath(int player, int card, event_t event){
	return scavenge(player, card, event, 4, 0, 0, 2, 0, 0);
}

int card_deaths_presence(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE){
				if( instance->targets[11].player < 1 ){
					instance->targets[11].player = 1;
				}
				int pos = instance->targets[11].player;
				if( pos < 10 ){
					int dp = get_power(affected_card_controller, affected_card);
					if( dp ){
						instance->targets[pos].card = dp;
						instance->targets[11].player++;
					}
				}
			}
		}
	}

	if( instance->targets[11].player > 1 && resolve_graveyard_trigger(player, card, event) ){
		int i;
		for(i=1; i<instance->targets[11].player; i++){
			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				add_1_1_counters(instance->targets[0].player, instance->targets[0].card, instance->targets[i].card);
			}
		}
		instance->targets[11].player = 1;
	}
	return global_enchantment(player, card, event);
}

int card_drudge_beetle(int player, int card, event_t event){
	return scavenge(player, card, event, 5, 0, 0, 1, 0, 0);
}

int druids_deliverance_legacy(int player, int card, event_t event ){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 ){
			if( is_what(source->damage_source_player, source->damage_source_card, TYPE_CREATURE) &&
				! check_special_flags(source->damage_source_player, source->damage_source_card, SF_ATTACKING_PWALKER)
			  ){
				if( source->damage_target_player == player && source->damage_target_card == -1 ){
					source->info_slot = 0;
				}
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_druids_deliverance(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		fog_effect(player, card);
		populate(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_gatecreeper_vine(int player, int card, event_t event){

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int choice = 0;
		unsigned int subt = SUBTYPE_GATE;
		int type = TYPE_PERMANENT;
		const char* msg = "Select a Gate card.";
		if( !IS_AI(player) ){
			choice = do_dialog(player, player, card, -1, -1, " Tutor a Gate card\n Tutor a basic land", 0);
		}
		if( choice == 1 ){
			subt = SUBTYPE_BASIC;
			type = TYPE_LAND;
			msg = "Select a basic land card.";
		}

		test_definition_t this_test;
		new_default_test_definition(&this_test, type, msg);
		this_test.subtype = subt;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}
	return 0;
}

int card_gobbling_ooze(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_1_1_counter(player, instance->parent_card);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE+GAA_NOT_ME_AS_TARGET, 0, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_golgari_decoy(int player, int card, event_t event){

	everybody_must_block_me(player, card, event);

	return scavenge(player, card, event, 3, 0, 0, 2, 0, 0);
}

int card_horncallers_chant(int player, int card, event_t event){

	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_RHINO, &token);
			token.key_plus = KEYWORD_TRAMPLE;
			generate_token(&token);

			populate(player, card);

			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_korozda_monitor(int player, int card, event_t event){
	return scavenge(player, card, event, 5, 0, 0, 2, 0, 0);
}

int card_mana_bloom(int player, int card, event_t event)
{
  /* Mana Bloom	|X|G
   * Enchantment
   * ~ enters the battlefield with X charge counters on it.
   * Remove a charge counter from ~: Add one mana of any color to your mana pool. Activate this ability only once each turn.
   * At the beginning of your upkeep, if ~ has no charge counters on it, return it to its owner's hand. */
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  if (player == AI && x_value <= 0)
		ai_modifier -= 36;

	  instance->info_slot = x_value;
	}

  enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, instance->info_slot);

  if (event == EVENT_CAN_ACTIVATE)
	return count_counters(player, card, COUNTER_CHARGE) && can_produce_mana(player, card) && instance->targets[2].player <= 0;

  if (event == EVENT_ACTIVATE && produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, 1))
	{
	  instance->targets[2].player = 1;
	  remove_counter(player, card, COUNTER_CHARGE);
	}

  if (event == EVENT_COUNT_MANA && instance->targets[2].player <= 0 && count_counters(player, card, COUNTER_CHARGE))
	declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, 1);

  if (event == EVENT_CLEANUP && affect_me(player, card))
	instance->targets[2].player = -1;

  if (count_counters(player, card, COUNTER_CHARGE) <= 0 && current_turn == player && upkeep_trigger(player, card, event))	// deliberately not upkeep_trigger_ability
	bounce_permanent(player, card);

  return global_enchantment(player, card, event);
}

int card_oak_street_innkeeper(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES
	  && current_turn != player
	  && affected_card_controller == player
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && is_tapped(affected_card_controller, affected_card)
	  && !is_humiliated(player, card))
	hexproof(affected_card_controller, affected_card, event);

  // Force recomputation when a creature taps or untaps; this normally isn't done until a card or effect resolves
  if ((event == EVENT_UNTAP_CARD || event == EVENT_TAP_CARD)
	  && current_turn != player
	  && affected_card_controller == player
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE))
	{
	  get_card_instance(affected_card_controller, affected_card)->regen_status |= KEYWORD_RECALC_ABILITIES;
	  get_abilities(affected_card_controller, affected_card, EVENT_ABILITIES, -1);
	}

  return 0;
}

// rubbleback rhino --> Gladecover Scout

int card_savage_surge(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_slime_molding(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_OOZE, &token);
			token.pow = instance->info_slot;
			token.tou = instance->info_slot;
			generate_token(&token);

			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_stonefare_crocodile(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0, SP_KEYWORD_LIFELINK);
}

// towering indrik --> vanilla


int card_urban_burgeoning(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

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

	if( in_play(player, card) && instance->targets[0].player != -1 ){
		if( current_turn != player && current_phase == PHASE_UNTAP && is_tapped(instance->targets[0].player, instance->targets[0].card) ){
			if( event == EVENT_UNTAP && affected_card_controller == 1-player ){
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_wild_beastmaster(int player, int card, event_t event)
{
  // Whenever ~ attacks, each other creature you control gets +X/+X until end of turn, where X is ~'s power.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  default_test_definition(&test, TYPE_CREATURE);
	  test.not_me = 1;

	  int amt = get_power(player, card);

	  pump_creatures_until_eot(player, card, player, 0, amt,amt, 0,0, &test);
	}

  return 0;
}

int card_worldspine_wurm(int player, int card, event_t event){

	/* Worldspine Wurm	|8|G|G|G
	 * Creature - Wurm 15/15
	 * Trample
	 * When ~ dies, put three 5/5 |Sgreen Wurm creature tokens with trample onto the battlefield.
	 * When ~ is put into a graveyard from anywhere, shuffle it into its owner's library. */

	if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WURM, &token);
		token.qty = 3;
		token.pow = 5;
		token.tou = 5;
		token.key_plus = KEYWORD_TRAMPLE;
		generate_token(&token);
	}

	return 0;
}

// Gold
int card_abrupt_decay(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.special = TARGET_SPECIAL_CMC_LESSER_OR_EQUAL;
	td.extra = 3;
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PERMANENT") ){
			instance->number_of_targets = 1;
			state_untargettable(player, card, 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		state_untargettable(player, card, 0);
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_archon_of_the_triumvirate(int player, int card, event_t event)
{
  // Whenever ~ attacks, detain up to two target nonland permanents your opponents control.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.illegal_type = TYPE_LAND;
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 3;

	  pick_up_to_n_targets_noload(&td, "Select target nonland permanent an opponent controls.", 2);

	  card_instance_t* instance = get_card_instance(player, card);
	  int i;
	  for (i = 0; i < instance->number_of_targets; ++i)
		detain(player, card, instance->targets[i].player, instance->targets[i].card);
	}

  return 0;
}

int card_armada_wurm(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WURM, &token);
		token.pow = 5;
		token.tou = 5;
		token.key_plus = KEYWORD_TRAMPLE;
		generate_token(&token);
	}

	return 0;
}

int card_auger_spree(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 4, -4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_azorius_charm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int ai_choice = 0;
		int choice = 0;
		if( can_target(&td) ){
			ai_choice = 2;
			choice = do_dialog(player, player, card, -1, -1, " Gain Lifelink\n Draw a card\n Put creature on deck\n Cancel", ai_choice);
		}
		else{
			if( count_permanents_by_type(player, TYPE_CREATURE) < 1 ){
				ai_choice = 1;
			}
			choice = do_dialog(player, player, card, -1, -1, " Gain Lifelink\n Draw a card\n Cancel", ai_choice);
			if( choice == 2 ){
				choice++;
			}
		}
		if( choice == 0 || choice == 1  ){
			instance->info_slot = 66+choice;
		}
		else if( choice == 2 ){
				if( pick_target(&td, "TARGET_ATTACKING_BLOCKING_CREATURE") ){
					instance->info_slot = 66+choice;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == 66 ){
			pump_subtype_until_eot(player, card, player, -1, 0, 0, 0, SP_KEYWORD_LIFELINK);
		}
		if( instance->info_slot == 67 ){
			draw_cards(player, 1);
		}
		if( instance->info_slot == 68 && valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_call_of_the_conclave(int player, int card, event_t event){
	/* Call of the Conclave	|G|W
	 * Sorcery
	 * Put a 3/3 |Sgreen Centaur creature token onto the battlefield. */

	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			generate_token_by_id(player, card, CARD_ID_CENTAUR);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_carnival_hellsteed(int player, int card, event_t event){
	haste(player, card, event);
	return unleash(player, card, event);
}

int card_centaur_healer(int player, int card, event_t event){
	return cip_lifegain(player, card, event, 3);
}

int card_chemisters_trick(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( current_turn == player || current_phase > PHASE_DECLARE_ATTACKERS ){
				ai_modifier-=25;
			}
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XUR(3, 1, 1));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target creature your opponent controls.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				pump_creatures_until_eot(player, card, 1-player, 0, -2, 0, 0, SP_KEYWORD_MUST_ATTACK, &this_test);
			}
			else{
				if( valid_target(&td) ){
					pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -2, 0, 0, SP_KEYWORD_MUST_ATTACK);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XUR(3, 1, 1));
}

int card_collective_blessing(int player, int card, event_t event){
	boost_creature_type(player, card, event, -1, 3, 3, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	return global_enchantment(player, card, event);
}

int card_common_bond(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int i;
		for(i=0; i<2; i++){
			if(i>0){
				td.allow_cancel = 0;
				select_target(player, card, &td, "Select target creature.", &(instance->targets[i]));
			}
			else{
				pick_target(&td, "TARGET_CREATURE");
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				add_1_1_counter(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// corpsejack menace --> vanilla

int card_coursers_accord(int player, int card, event_t event){
	/* Coursers' Accord	|4|G|W
	 * Sorcery
	 * Put a 3/3 |Sgreen Centaur creature token onto the battlefield, then populate. */

	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			generate_token_by_id(player, card, CARD_ID_CENTAUR);
			populate(player, card);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_detention_sphere(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.extra = instance->internal_card_id;
		td.special = TARGET_SPECIAL_EXTRA_NOT_IID;
		td.allow_cancel = 0;

		if( comes_into_play_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( new_pick_target(&td, "Select another nonland permanent.", 0, GS_LITERAL_PROMPT) ){
				int id = get_id(instance->targets[0].player, instance->targets[0].card);
				int result = -1;
				instance->targets[0].player = 1;
				int p;
				for (p = 0; p < 2; ++p){
					int count = active_cards_count[p]-1;
					while( count > -1 ){
							if (in_play(p, count) && get_id(p, count) == id ){
								if( ! is_token(p, count) ){
									int i;
									for(i=1; i<10; i++){
										if( instance->targets[i].player == -1 ){
											instance->targets[i].player = get_owner(p, count);
											instance->targets[i].card = get_original_internal_card_id(p, count);
											kill_card(p, count, KILL_REMOVE);
											result = cards_data[instance->targets[i].card].id;
											break;
										}
									}
								}
							}
							count--;
					}
				}
				if( result > -1 ){
					create_card_name_legacy(player, card, result);
				}
			}
		}
	}

	if( leaves_play(player, card, event) ){
		int i;
		for(i=1; i<10; i++){
			if( instance->targets[i].player > -1 ){
				if( check_rfg(instance->targets[i].player, cards_data[instance->targets[i].card].id) ){
					int card_added = add_card_to_hand(instance->targets[i].player, instance->targets[i].card);
					remove_card_from_rfg(instance->targets[i].player, cards_data[instance->targets[i].card].id);
					put_into_play(instance->targets[i].player, card_added);
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_dramatic_rescue(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life(player, 2);
			return card_unsummon(player, card, event);
		}
	}
	else{
		return card_unsummon(player, card, event);
	}

	return 0;
}

int card_dreadbore(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TARGET_TYPE_PLANESWALKER);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PERMANENT");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dreg_mangler(int player, int card, event_t event){
	haste(player, card, event);
	return scavenge(player, card, event, 3, 1, 0, 1, 0, 0);
}

int card_epic_experiment(int player, int card, event_t event){

	/* Epic Experiment	|X|U|R
	 * Sorcery
	 * Exile the top X cards of your library. For each instant and sorcery card with converted mana cost X or less among them, you may cast that card without
	 * paying its mana cost. Then put all cards exiled this way that weren't cast into your graveyard. */

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		int max = MIN(count_deck(player), instance->info_slot);
		if( max ){
			int *deck = deck_ptr[player];
			show_deck(player, deck, max, "Cards revealed with Epic Experiment.", 0, 0x7375B0 );
			int i;
			int cards_exiled[max];
			for(i=0; i<max; i++){
				cards_exiled[i] = deck[0];
				rfg_card_in_deck(player, 0);
			}
			int playable[max];
			int pc = 0;
			for(i=0; i<max; i++){
				if( is_what(-1, cards_exiled[i], TYPE_SPELL) && ! is_what(-1, cards_exiled[i], TYPE_CREATURE) &&
					get_cmc_by_internal_id(cards_exiled[i]) <= instance->info_slot && can_legally_play_iid(player, cards_exiled[i])
				  ){
					playable[pc] = cards_exiled[i];
					pc++;
				}
			}
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_SPELL, "Select a spell to play for free.");
			while( pc ){
					int selected = select_card_from_zone(player, player, playable, pc, 0, AI_MAX_VALUE, -1, &this_test);
					if( selected != -1 ){
						int csvid = cards_data[playable[selected]].id;
						int k;
						for(k=0; k<max; k++){
							if( cards_exiled[k] == playable[selected] ){
								cards_exiled[k] = -1;
								break;
							}
						}
						for(k=selected; k<pc; k++){
							playable[k] = playable[k+1];
						}
						pc--;
						play_card_in_exile_for_free(player, player, csvid);
					}
					else{
						break;
					}
			}
			for(i=0; i<max; i++){
				if( cards_exiled[i] != -1 ){
					int pos = find_iid_in_rfg(player, cards_exiled[i]);
					if( pos > -1 ){
						from_exile_to_graveyard(player, pos);
					}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_essence_backlash(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, &td, 0) ){
			damage_player(instance->targets[0].player, get_base_power(instance->targets[0].player, instance->targets[0].card), player, card);
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, &td, NULL, 1, NULL);
}

int card_fall_of_the_gavel(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 5);
		return card_counterspell(player, card, event);
	}
	else{
		return card_counterspell(player, card, event);
	}

	return 0;
}

int card_fireminds_foresight(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=3; i>0;i--){
				char buffer[100];
				scnprintf(buffer, 100, "Select an Instant card with CMC = %d", i);
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_INSTANT | TYPE_INTERRUPT, buffer);
				this_test.type_flag = F1_NO_CREATURE;
				this_test.cmc = i;
				this_test.no_shuffle = 1;
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			}
			shuffle(player);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_goblin_electromancer(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_SPELL) && ! is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			COST_COLORLESS--;
		}
	}

	return 0;
}

int card_golgari_charm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = card_death_ward(player, card, event);
		if( result > 0 ){
			instance->info_slot = 1;
			return card_death_ward(player, card, event);
		}
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int ai_choice = 0;
		int choice = 0;
		if( instance->info_slot == 1 ){
			choice = 2;
		}
		else{
			if( can_target(&td) ){
				if( count_creatures_by_toughness(player, 1, 0) > count_creatures_by_toughness(1-player, 1, 0) ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Weaken all creatures\n Kill an Enchantment\n Cancel", ai_choice);
				if( choice == 2 ){
					choice++;
				}
			}
		}
		if( choice == 0 || choice == 2  ){
			instance->info_slot = 66+choice;
		}
		else if( choice == 1 ){
				if( pick_target(&td, "TARGET_ENCHANTMENT") ){
					instance->number_of_targets = 1;
					if( ! is_planeswalker(instance->targets[0].player, instance->targets[0].card) ){
						instance->info_slot = 66+choice;
					}
					else{
						spell_fizzled = 1;
					}
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == 66 ){
			pump_subtype_until_eot(player, card, 2, -1, -1, -1, 0, 0);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( instance->info_slot == 68 ){
			return card_wrap_in_vigor(player, card, event);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_grisly_salvage(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
				int amount = 5;
				if( amount > count_deck(player) ){
					amount = count_deck(player);
				}
				if( amount > 0 ){
					char buffer[100];
					scnprintf(buffer, 100, "Select an Creature or Land card.");
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_LAND, buffer);
					this_test.create_minideck = amount;
					this_test.no_shuffle = 1;
					if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test) > -1 ){
						amount--;
					}
					if( amount > 0 ){
						mill(player, amount);
					}
				}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_havoc_festival(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int amount = (life[current_turn]+1)/2;
		lose_life(current_turn, amount);
	}

	return global_enchantment(player, card, event);
}

int card_hellhole_flailer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	unleash(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, instance->info_slot, player, card);
		}
	}

	if (event == EVENT_ACTIVATE){
		instance->info_slot = get_power(player, card);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 2, 1, 0, 0, 1, 0, 0, &td, "TARGET_PLAYER");
}

int card_hussar_patrol(int player, int card, event_t event){
	vigilance(player, card, event);
	return flash(player, card, event);
}

int card_hypersonic_dragon(int player, int card, event_t event){

	haste(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && hand_count[player] > 0 && ! is_humiliated(player, card) ){
		int count = 0;
		while( count < active_cards_count[player] ){
				if( in_hand(player, count) && is_what(player, count, TYPE_SORCERY) ){
					if( has_mana_to_cast_iid(player, event, get_original_internal_card_id(player, count)) &&
						can_legally_play_iid(player, get_original_internal_card_id(player, count))
					  ){
						return 1;
					}
				}
				count++;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int playable[2][hand_count[player]];
		int pc = 0;
		int count = 0;
		while( count < active_cards_count[player] ){
				if( in_hand(player, count) && is_what(player, count, TYPE_SORCERY) ){
					if( has_mana_to_cast_iid(player, event, get_original_internal_card_id(player, count)) &&
						can_legally_play_iid(player, get_original_internal_card_id(player, count))
					  ){
						playable[0][pc] = get_original_internal_card_id(player, count);
						playable[1][pc] = count;
						pc++;
					}
				}
				count++;
		}
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SORCERY, "Select a Sorcery to play");
		int selected = select_card_from_zone(player, player, playable[0], pc, 0, AI_MAX_VALUE, -1, &this_test);
		if( selected != -1 ){
			charge_mana_from_id(player, -1, event, cards_data[playable[0][selected]].id);
			if( spell_fizzled != 1 ){
				play_card_in_hand_for_free(player, playable[1][selected]);
				cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
			}
		}
		else{
			spell_fizzled = 1;
			return 0;
		}
	}

	return flash(player, card, event);
}

int card_isperia_supreme_judge(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // Whenever a creature attacks you or a planeswalker you control, you may draw a card.
  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player) | DAT_SEPARATE_TRIGGERS, 1-player, -1))
	draw_a_card(player);

  return 0;
}

int card_izzet_charm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			if( ! is_what(card_on_stack_controller, card_on_stack, TYPE_CREATURE) ){
				instance->info_slot = 1;
				return result;
			}
			else{
				instance->info_slot = 0;
			}
		}
		else{
			instance->info_slot = 0;
		}
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		if( instance->info_slot == 1 ){
			choice = 2;
		}
		else{
			choice = 1;
			if( can_target(&td) ){
				choice = do_dialog(player, player, card, -1, -1, " Damage creature\n Draw 2 and discard 2\n Cancel", 0);
				if( choice == 2 ){
					choice++;
				}
			}
		}
		if( choice == 1 || choice == 2  ){
			instance->info_slot = 66+choice;
			if( choice == 2 ){
				return card_counterspell(player, card, event);
			}
		}
		else if( choice == 0 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->info_slot = 66+choice;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
		if( instance->info_slot == 67 ){
			draw_cards(player, 2);
			multidiscard(player, 2, 0);
		}
		if( instance->info_slot == 68 ){
			counterspell_resolve_unless_pay_x(player, card, NULL, 0, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_izzet_staticaster(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	haste(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			tap_card(player, card);
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.id = get_id(instance->targets[0].player, instance->targets[0].card);
			new_damage_all(player, instance->parent_card, 2, 1, 0, &this_test);
		}
	}

	return flash(player, card, event);
}

static const char* is_forest_or_swamp(int who_chooses, int player, int card, int targeting_player, int targeting_card){
	return has_subtype(player, card, SUBTYPE_SWAMP) || has_subtype(player, card, SUBTYPE_FOREST) ? NULL : EXE_STR(0x73964C);	// ",subtype"
}

int card_jarad_golgari_lich_lord(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if(event == EVENT_GRAVEYARD_ABILITY){
		if( can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			int f_found = 0;
			int s_found = 0;
			int good = 0;
			int i;
			for(i=0; i<active_cards_count[player]; i++ ){
				if( in_play(player, i) && is_what(player, i, TYPE_LAND) ){
					if( has_subtype(player, i, SUBTYPE_FOREST) || has_subtype(player, i, SUBTYPE_SWAMP) ){
						good++;
						if( has_subtype(player, i, SUBTYPE_FOREST) ){
							f_found++;
						}
						if( has_subtype(player, i, SUBTYPE_SWAMP) ){
							s_found++;
						}
					}
				}
				if( f_found > 0 && s_found > 0 && good > 1 ){
					return GA_RETURN_TO_HAND;
				}
			}
		}
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_LAND);
		td1.allowed_controller = player;
		td1.preferred_controller = player;
		td1.illegal_abilities = 0;
		td1.extra = (int32_t)is_forest_or_swamp;
		td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;

		if( select_target(player, card, &td1, "Select a Forest or Swamp card to sacrifice", &(instance->targets[0])) ){
			instance->number_of_targets = 1;
			int mode = 0;
			int lnds[2] = {-1, -1};
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			lnds[0] = instance->targets[0].card;
			mode |= has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_SWAMP) ? 1 : 0;
			mode |= has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_FOREST) ? 2 : 0;
			while( 1 ){
					if( select_target(player, card, &td1, "Select a Forest or Swamp card to sacrifice", &(instance->targets[0])) ){
						instance->number_of_targets = 1;
						if( (mode & 1) && has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_FOREST) ){
							lnds[1] = instance->targets[0].card;
							break;
						}
						if( (mode & 2) && has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_SWAMP) ){
							lnds[1] = instance->targets[0].card;
							break;
						}
					}
					else{
						break;
					}
			}
			state_untargettable(player, lnds[0], 0);
			if( lnds[1] != -1 ){
				kill_card(player, lnds[0], KILL_SACRIFICE);
				kill_card(player, lnds[1], KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
			}
		}
		else{
			spell_fizzled = 1;
		}
		return spell_fizzled != 1 ? GAPAID_REMOVE : 0;
	}

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result+=count_graveyard_by_type(player, TYPE_CREATURE);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_NOT_ME_AS_TARGET | GAA_SACRIFICE_CREATURE, MANACOST_XBG(1, 1, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XBG(1, 1, 1)) ){
			state_untargettable(player, card, 1);
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			state_untargettable(player, card, 0);
			if (!sac){
				cancel = 1;
				return 0;
			}
			instance->info_slot = get_power(BYTE2(sac), BYTE3(sac));
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		lose_life(1-player, instance->info_slot);
	}

	return 0;
}

int card_jarads_orders(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			char buffer[100];
			scnprintf(buffer, 100, "Select an Creature to add to your hand.");
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
			this_test.no_shuffle = 1;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 11, &this_test);
			strcpy(buffer, "Select a Creature to put into the graveyard.");
			new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GRAVE, 0, AI_MAX_CMC, &this_test);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_korozda_guildmage(int player, int card, event_t event){
	/* Korozda Guildmage	|B|G
	 * Creature - Elf Shaman 2/2
	 * |1|B|G: Target creature gets +1/+1 and gains intimidate until end of turn.
	 * |2|B|G, Sacrifice a nontoken creature: Put X 1/1 |Sgreen Saproling creature tokens onto the battlefield, where X is the sacrificed creature's toughness. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 1, 0, 1, 0, 0) && can_target(&td) ){
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, 2, 1, 0, 1, 0, 0) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( has_mana_for_activated_ability(player, card, 1, 1, 0, 1, 0, 0) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 2, 1, 0, 1, 0, 0) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0) ){
				if( current_phase > PHASE_AFTER_BLOCKING ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Pump a creature\n Sac & generate saprolings\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, 1, 1, 0, 1, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 2, 1, 0, 1, 0, 0);
				if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_CREATURE, F1_NO_TOKEN, 0, 0, 0, 0, 0, 0, -1, 0) ){
					instance->info_slot = 66+choice;
				}
				else{
					spell_fizzled = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1, 0, SP_KEYWORD_INTIMIDATE);
		}
		if( instance->info_slot == 67 ){
			generate_tokens_by_id(player, card, CARD_ID_SAPROLING, instance->targets[2].card);
		}
	}

	return 0;
}

int card_lotleth_troll(int player, int card, event_t event){
	char buffer[100];
	scnprintf(buffer, 100, "Select a Creature card to discard.");
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		if( land_can_be_played & LCBP_REGENERATION ){
			if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && can_regenerate(player, card) ){
				return 0x63;
			}
		}
		return check_battlefield_for_special_card(player, card, player, 0, &this_test);
	}

	if( event == EVENT_ACTIVATE ){
		if( land_can_be_played & LCBP_REGENERATION ){
			if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && can_regenerate(player, card) ){
				charge_mana(player, COLOR_BLACK, 1);
				if( spell_fizzled != 1 ){
					instance->info_slot = 66;
				}
			}
		}
		else{
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
				instance->info_slot = 67;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			regenerate_target(player, instance->parent_card);
		}
		if( instance->info_slot == 67 ){
			add_1_1_counter(player, instance->parent_card);
		}
	}
	return 0;
}

int card_lyev_skyknight(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;
	td.allowed_controller = 1-player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( can_target(&td) && comes_into_play(player, card, event) ){
		if( pick_target(&td, "TARGET_PERMANENT") ){
			detain(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_loxodon_smither(int player, int card, event_t event){
	cannot_be_countered(player, card, event);
	return 0;
}

int card_mercurial_chemister(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && hand_count[player] > 0){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) && hand_count[player] > 0 && can_target(&td) ){
				if( hand_count[player] >= hand_count[1-player] ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Draw 2 card\n Discard & damage creature\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
				charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0);
				if( spell_fizzled != 1 ){
					tap_card(player, card);
					instance->info_slot = 66+choice;
				}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;

					char buffer[100];
					scnprintf(buffer, 100, "Select a card to discard.");
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_ANY, buffer);
					int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
					if( selected != -1 ){
						instance->targets[1].player = get_cmc(player, selected);
						discard_card(player, selected);
						instance->info_slot = 66+choice;
						tap_card(player, card);
					}
					else{
						spell_fizzled = 1;
					}
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			draw_cards(player, 2);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, player, instance->parent_card);
		}
	}

	return 0;
}

int card_new_prahv_guildmage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 1) && can_target(&td) ){
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 1) && can_target(&td1) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 1) && can_target(&td) ){
			if( has_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 1) && can_target(&td1) ){
				if( current_turn != player && current_phase < PHASE_DECLARE_ATTACKERS ){
					ai_choice = 1;
				}
				choice = do_dialog(player, player, card, -1, -1, " Give Flying\n Detain a nonland permanent\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
				charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 1);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					instance->info_slot = 66+choice;
				}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 1);
				if( spell_fizzled != 1 && pick_target(&td1, "TARGET_PERMANENT") ){
					instance->number_of_targets = 1;
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
			detain(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_nivix_guildmage(int player, int card, event_t event){
	/*
	  Nivix Guildmage |U|R
	  Creature - Human Wizard 2/2, UR (2)
	  {1}{U}{R}: Draw a card, then discard a card.
	  {2}{U}{R}: Copy target instant or sorcery spell you control. You may choose new targets for the copy.
	*/

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_INSTANT|TYPE_SORCERY);
	td.allowed_controller = td.preferred_controller = player;

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK, MANACOST_XUR(2, 1, 1), 0, &td, NULL) ){
			return 99;
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_XUR(1, 1, 1), 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = instance->info_slot = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SPELL_ON_STACK, MANACOST_XUR(2, 1, 1), 0, &td, NULL) ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_XUR(2, 1, 1)) ){
				instance->targets[0].player = card_on_stack_controller;
				instance->targets[0].card = card_on_stack;
				instance->number_of_targets = 1;
				instance->info_slot = 2;
			}
		}
		else{
			instance->info_slot = 1;
			return generic_activated_ability(player, card, event, 0, MANACOST_XUR(1, 1, 1), 0, NULL, NULL);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			draw_cards(player, 1);
			discard(player, 0, player);
		}
		if( instance->info_slot == 2 && valid_target(&td) ){
			copy_spell_from_stack(player, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_niv_mizzet_dracogenius(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	check_legend_rule(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL+DDBM_MUST_DAMAGE_PLAYER) ){
		draw_cards(player, 1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 0, 0, 1, 0, 1, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_rakdos_charm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int mode = (1<<2)+(1<<3);
			int ai_choice = 3;
			int choice = 0;
			char buffer[500];
			int pos = 0;
			if( count_subtype(player, TYPE_CREATURE, -1) < life[player] && count_subtype(1-player, TYPE_CREATURE, -1) >= life[1-player] ){
				ai_choice = 2;
			}
			if( can_target(&td) ){
				mode |= (1<<0);
				pos += scnprintf(buffer + pos, 500-pos, " Exile a graveyard\n", buffer);
				ai_choice = 0;
			}
			if( can_target(&td1) ){
				mode |= (1<<1);
				pos += scnprintf(buffer + pos, 500-pos, " Kill an Artifact\n", buffer);
				ai_choice = 1;
			}
			pos += scnprintf(buffer + pos, 500-pos, " Creatures damage controllers\n", buffer);
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
					if( pick_target(&td1, "TARGET_ARTIFACT") ){
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

	else if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				int count = count_graveyard(instance->targets[0].player)-1;
				while( count > -1 ){
						rfg_card_from_grave(instance->targets[0].player, count);
						count--;
				}
			}
			if( instance->info_slot == 67 && valid_target(&td1) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			if( instance->info_slot == 68 ){
				int i;
				for(i=0; i<2; i++){
					int count = active_cards_count[i]-1;
					while( count > -1 ){
							if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
								damage_player(i, 1, i, count);
							}
							count--;
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rakdos_ragemutt(int player, int card, event_t event){
	haste(player, card, event);
	lifelink(player, card, event);
	return 0;
}

int card_rakdos_ringleader(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL+DDBM_MUST_DAMAGE_OPPONENT) ){
		discard(1-player, DISC_RANDOM, player);
	}
	return regeneration(player, card, event, 0, 1, 0, 0, 0, 0);
}

int card_rakdos_lord_of_riots(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_MODIFY_COST ){
		if( get_trap_condition(1-player, TRAP_LIFE_LOST) < 1 ){
			infinite_casting_cost();
		}
	}

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		COST_COLORLESS-=get_trap_condition(1-player, TRAP_LIFE_LOST);
	}

	return 0;
}

int card_rakdoss_return(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_PLAYER") ){
				instance->info_slot = x_value;
			}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_player(instance->targets[0].player, instance->info_slot, player, card);
				new_multidiscard(instance->targets[0].player, instance->info_slot, 0, player);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_righteous_autority(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( current_turn == player && event == EVENT_DRAW_PHASE ){
			event_result++;
		}
	}

	return generic_aura(player, card, event, player, hand_count[player], hand_count[player], 0, 0, 0, 0, 0);
}

// Risen sanctuary --> Serra Angel

int card_rites_of_reaping(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && target_available(player, card, &td) > 1 ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			td.preferred_controller = 1-player;
			if( select_target(player, card, &td, "Select target creature.", &(instance->targets[1])) ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
			}
			else{
				instance->number_of_targets = 0;
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 3);
		}
		if( validate_target(player, card, &td, 1) ){
			pump_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, -3, -3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rix_maadi_guildmage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_BLOCKING;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 1, 0) ){
			if( can_target(&td) ){
				return 1;
			}
			else{
				instance->targets[0].player = 1-player;
				if( would_valid_target(&td1) && get_trap_condition(1-player, TRAP_LIFE_LOST) > 0 ){
					return 1;
				}
				if( player != AI ){
					instance->targets[0].player = player;
					if( would_valid_target(&td1) && get_trap_condition(player, TRAP_LIFE_LOST) > 0 ){
						return 1;
					}
				}
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( can_target(&td) ){
			if( can_target(&td1) && (get_trap_condition(1-player, TRAP_LIFE_LOST) > 0 || get_trap_condition(player, TRAP_LIFE_LOST) > 0) ){
				ai_choice = 1;
				choice = do_dialog(player, player, card, -1, -1, " Give +1/-1 to a blocker\n Make player lose life\n Cancel", ai_choice);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 1, 0);
			if( spell_fizzled != 1 ){
				if( choice == 0 ){
					if( pick_target(&td, "TARGET_CREATURE")  ){
						instance->number_of_targets = 1;
						instance->info_slot = 66+choice;
					}
				}
				else if( choice == 1 ){
						if( pick_target(&td1, "TARGET_PLAYER") ){
							if( get_trap_condition(instance->targets[0].player, TRAP_LIFE_LOST) > 0 ){
								instance->number_of_targets = 1;
								instance->info_slot = 66+choice;
							}
							else{
								spell_fizzled = 1;
							}
						}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			lose_life(instance->targets[0].player, 1);
		}
	}

	return 0;
}

int card_search_warrant(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td) ;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				reveal_target_player_hand(instance->targets[0].player);
				gain_life(player, hand_count[instance->targets[0].player]);
		}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_selesnya_charm(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.power_requirement = 5 | TARGET_PT_GREATER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int mode = (1<<2) | (1<<3);
			int ai_choice = 2;
			int choice = 0;
			char buffer[500];
			int pos = 0;
			if( can_target(&td) ){
				mode |= (1<<0);
				pos += scnprintf(buffer + pos, 500-pos, " Pump a creature\n");
				ai_choice = 0;
			}
			if( can_target(&td1) ){
				mode |= (1<<1);
				pos += scnprintf(buffer + pos, 500-pos, " Destroy creature with pow >= 5\n");
				ai_choice = 1;
			}
			pos += scnprintf(buffer + pos, 500-pos, " Generate a Knight\n");
			pos += scnprintf(buffer + pos, 500-pos, " Cancel");
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

	else if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2, KEYWORD_TRAMPLE, 0);
			}
			if( instance->info_slot == 67 && valid_target(&td1) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
			if( instance->info_slot == 68 ){
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_KNIGHT, &token);
				token.s_key_plus = SP_KEYWORD_VIGILANCE;
				token.pow = 2;
				token.tou = 2;
				generate_token(&token);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_skull_rend(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			damage_player(1-player, 2, player, card);
			new_multidiscard(1-player, 2, DISC_RANDOM, player);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_skymark_roc(int player, int card, event_t event)
{
  // Whenever ~ attacks, you may return target creature defending player controls with toughness 2 or less to its owner's hand.
  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.toughness_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;
	  td.allowed_controller = 1-player;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		bounce_permanent(instance->targets[0].player, instance->targets[0].card);
	}

  return 0;
}

int card_slaughter_games(int player, int card, event_t event){

	/* Slaughter Games	|2|B|R
	 * Sorcery
	 * ~ can't be countered by spells or abilities.
	 * Name a nonland card. Search target opponent's graveyard, hand, and library for any number of cards with that name and exile them. Then that player
	 * shuffles his or her library. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_PLAYER") ){
				state_untargettable(player, card, 1);
			}
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			state_untargettable(player, card, 0);
			if( valid_target(&td) ){
				int opponent = instance->targets[0].player;
				int id = -1;
				int card_selected  = -1;
				if( player != AI ){
					if( ai_is_speculating != 1 ){
						while(1){
							card_selected = choose_a_card("Choose a card", -1, -1);
							if( ! is_what(-1, card_selected, TYPE_LAND) && is_valid_card(cards_data[card_selected].id)){
								id = cards_data[card_selected].id;
								break;
							}
						}
					}
				}
				else{
					 int count = count_deck(opponent)-1;
					 int *deck = deck_ptr[opponent];
					 while( count > -1 ){
							if( deck[count] != -1 && ! is_what(-1, deck[count], TYPE_LAND) ){
								id = cards_data[deck[count]].id;
								break;
							}
							count--;
					}
				}

				if( id != -1 && player == AI ){
					char buffer[300];
					int pos = scnprintf(buffer, 300, "Opponent named:");
					card_ptr_t* c_me = cards_ptr[ id ];
					pos += scnprintf(buffer+pos, 300-pos, " %s", c_me->name);
					do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
				}

				if( id > -1 ){
					lobotomy_effect(player, opponent, id, 0);
				}
			}

			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sluiceway_scorpion(int player, int card, event_t event){
	deathtouch(player, card, event);
	return scavenge(player, card, event, 1, 1, 0, 1, 0, 0);
}

// spawn of rix maadi --> dark reveler

int card_sphynxs_revelation(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->info_slot = x_value;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			draw_cards(player, instance->info_slot);
			gain_life(player, instance->info_slot);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_supreme_verdict(int player, int card, event_t event){
	cannot_be_countered(player, card, event);
	return card_day_of_judgment(player, card, event);
}

int card_teleportal(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allowed_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
				if( instance->info_slot == 1 ){
					return 1;
				}
				return can_target(&td);
			}
		}

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = 1;
			instance->number_of_targets = 0;
			if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
				result = pay_overload(player, card, MANACOST_XUR(3, 1, 1));
			}
			if( result == 1 ){
				new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT);
				if(	spell_fizzled == 1 ){
					return 0;
				}
			}
			instance->info_slot |= result;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot & 2 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				pump_creatures_until_eot(player, card, player, 0, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE, &this_test);
			}
			else{
				if( valid_target(&td) ){
					pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -2, 0, 0, SP_KEYWORD_UNBLOCKABLE);
				}
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return overload(player, card, event, MANACOST_XUR(3, 1, 1));
}

int card_thoughtflare(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			draw_cards(player, 4);
			multidiscard(player, 2, 0);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_treasured_find(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && count_graveyard(player) > 0 ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		char msg[100] = "Select a card to return to your hand.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, 1, -1, &this_test);
		if( selected != -1 ){
			const int *grave = get_grave(player);
			instance->targets[0].player = selected;
			instance->targets[0].card = grave[selected];
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		const int *grave = get_grave(player);
		int selected = instance->targets[0].player;
		if( instance->targets[0].card == grave[selected] ){
			add_card_to_hand(player, grave[selected]);
			remove_card_from_grave(player, selected);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_trestle_troll(int player, int card, event_t event){
	return regeneration(player, card, event, 1, 1, 0, 1, 0, 0);
}

int card_trostani_selesnyas_voice(int player, int card, event_t event){

	/* Trostani, Selesnya's Voice	|G|G|W|W
	 * Legendary Creature - Dryad 2/5
	 * Whenever another creature enters the battlefield under your control, you gain life equal to that creature's toughness.
	 * |1|G|W, |T: Populate. */

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.not_me = 1;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( new_specific_cip(player, card, event, player, 2, &this_test) ){
		gain_life(player, get_toughness(instance->targets[1].player, instance->targets[1].card));
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		populate(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 1, 0, 0, 1, 0, 1, 0, 0, 0);
}

int card_vitu_ghazi_guildmage(int player, int card, event_t event){
	/* Vitu-Ghazi Guildmage	|G|W
	 * Creature - Dryad Shaman 2/2
	 * |4|G|W: Put a 3/3 |Sgreen Centaur creature token onto the battlefield.
	 * |2|G|W: Populate. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 2, 0, 0, 1, 0, 1) ){
			int result = 1;
			if( player == AI ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				this_test.type_flag = F1_IS_TOKEN;

				result = check_battlefield_for_special_card(player, card, 2, 0, &this_test);
			}
			return result;
		}
		if( has_mana_for_activated_ability(player, card, 4, 0, 0, 1, 0, 1) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 4, 0, 0, 1, 0, 1) ){
			choice = do_dialog(player, player, card, -1, -1, " Populate\n Generate a Centaur\n Cancel", 1);
		}

		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			int cless = 2;
			if( choice == 1 ){
				cless+=2;
			}
			charge_mana_for_activated_ability(player, card, cless, 0, 0, 1, 0, 1);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			populate(player, instance->parent_card);
		}
		if( instance->info_slot == 67 ){
			generate_token_by_id(player, card, CARD_ID_CENTAUR);
		}
	}

	return 0;
}

static int is_damaging_this_planeswalker(card_instance_t* damage, int pw_player, int pw_card)
{
  // Must be damaging either the planeswalker's player or the planeswalker directly.
  if (damage->damage_target_player != pw_player)
	return 0;

  // Damage card already targeting the planeswalker directly?
  if (damage->damage_target_card == pw_card)
	return 1;

  // Then must be damaging the player.
  if (damage->damage_target_card != -1)
	return 0;

  int cr_player = damage->damage_source_player;
  int cr_card = damage->damage_source_card;

  // With the Attacking-A-Planeswalker bit.
  if (!check_special_flags(cr_player, cr_card, SF_ATTACKING_PWALKER))
	return 0;

  // And must have exactly one attacking-a-planeswalker legacy attached, with targets[1].card == pw_card.
  card_instance_t* effect;
  int c;
  for (c = 0; c < active_cards_count[cr_player]; ++c)
	if ((effect = in_play(cr_player, c))
		&& effect->damage_target_player == cr_player
		&& effect->damage_target_card == cr_card
		&& effect->internal_card_id == LEGACY_EFFECT_CUSTOM
		&& check_special_flags(cr_player, c, SF_ATTACKING_PWALKER))
	  return effect->targets[1].card == pw_card;

  return 0;
}

static int vraska_legacy(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_dealt(event), *instance;
  if (damage
	  && (damage->targets[3].player & TYPE_CREATURE)
	  && (instance = get_card_instance(player, card))->number_of_targets < 19
	  && is_damaging_this_planeswalker(damage, instance->damage_source_player, instance->damage_source_card))
	{
	  instance->targets[instance->number_of_targets].player = damage->damage_source_player;
	  instance->targets[instance->number_of_targets].card = damage->damage_source_card;
	  ++instance->number_of_targets;
	}

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player
	  && (instance = get_card_instance(player, card))->number_of_targets > 0)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  int i;
		  for (i = 0; i < instance->number_of_targets; ++i)
			if (in_play(instance->targets[i].player, instance->targets[i].card))
			  kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);

		  instance->number_of_targets = 0;
		}
	}

  if (event == EVENT_BEGIN_TURN && current_turn == player)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_vraska_the_unseen(int player, int card, event_t event){

	/* Vraska the Unseen	|3|B|G
	 * Planeswalker - Vraska (5)
	 * +1: Until your next turn, whenever a creature deals combat damage to ~, destroy that creature.
	 * -3: Destroy target nonland permanent.
	 * -7: Put three 1/1 |Sblack Assassin creature tokens onto the battlefield with "Whenever this creature deals combat damage to a player, that player loses
	 * the game." */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);


	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
		int ai_first_vraska_ability = 15;
		if(player == AI || ai_is_speculating == 1){
			ai_first_vraska_ability -= count_counters(player, card, COUNTER_LOYALTY);
			ai_first_vraska_ability -= count_subtype(player, TYPE_CREATURE, -1)*2;
			ai_first_vraska_ability += count_subtype(1-player, TYPE_CREATURE, -1)*3;
			if( ai_first_vraska_ability < 5 )
				ai_first_vraska_ability = 5;
		}

		enum {
			CHOICE_DIE_IF_TOUCH_ME = 1,
			CHOICE_KILL_PERMANENT,
			CHOICE_ASSASSINS
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Who damages Vraska dies", 1, ai_first_vraska_ability, 1,
						"Destroy a nonland", can_target(&td), 15, -3,
						"Unleash 3 Assassins", 1, 10+(15 * !check_battlefield_for_subtype(1-player, TYPE_CREATURE, -1)), -7);
		if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;	// else fall through to planeswalker()
		}

		else if (event == EVENT_ACTIVATE)
		{
			if( choice == CHOICE_KILL_PERMANENT ){
				instance->number_of_targets = 0;
				pick_target(&td, "TARGET_NONLAND_PERMANENT");
			}
		}

		else{	// event == EVENT_RESOLVE_ACTIVATION
			if( choice == CHOICE_DIE_IF_TOUCH_ME ){
				int legacy = create_legacy_effect(player, instance->parent_card, &vraska_legacy);
				card_instance_t* leg = get_card_instance(player, legacy);
				leg->number_of_targets = 0;
			}
			else if( choice == CHOICE_KILL_PERMANENT ){
					if (valid_target(&td)){
					  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
					}
			}
			else if( choice == CHOICE_ASSASSINS ){
					token_generation_t token;
					default_token_definition(player, card, CARD_ID_ASSASSIN, &token);
					token.pow = token.tou = 1;
					token.qty = 3;
					token.special_infos = 66;
					generate_token(&token);
			}
		}
	}

	return planeswalker(player, card, event, 5);
}

int card_wayfaring_temple(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL|DDBM_MUST_DAMAGE_OPPONENT|DDBM_MUST_BE_COMBAT_DAMAGE) ){
		populate(player, card);
	}

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += creature_count[player];
	}

	return 0;
}

// Hybrid
int card_azors_elocutors(int player, int card, event_t event)
{
  /* Azor's Elocutors	|3|WU|WU
   * Creature - Human Advisor 3/5
   * At the beginning of your upkeep, put a filibuster counter on ~. Then if ~ has five or more filibuster counters on it, you win the game.
   * Whenever a source deals damage to you, remove a filibuster counter from ~. */

  hybrid(player, card, event);

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY || event == EVENT_SHOULD_AI_PLAY)
	{
	  add_counter(player, card, COUNTER_FILIBUSTER);
	  if (count_counters(player, card, COUNTER_FILIBUSTER) >= 5)
		lose_the_game(1-player);
	}

  if (event == EVENT_SHOULD_AI_PLAY){
	  int counters = count_counters(player, card, COUNTER_FILIBUSTER);
	  if (player == AI){
		  ai_modifier += 8 * counters * counters * counters;
	  } else {
		  ai_modifier -= 8 * counters * counters * counters;
	  }
  }

  card_instance_t* damage = damage_being_dealt(event);
  if (damage && damage->damage_target_card == -1 && damage->damage_target_player == player && !damage_is_to_planeswalker(damage))
	get_card_instance(player, card)->info_slot++;

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (event == EVENT_TRIGGER && instance->info_slot > 0)
		event_result |= 2;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  remove_counters(player, card, COUNTER_FILIBUSTER, instance->info_slot);
		  instance->info_slot = 0;
		}
	}

  return 0;
}

int card_blistercoil_weird(int player, int card, event_t event){

	hybrid(player, card, event);

	if( specific_spell_played(player, card, event, player, 2, TYPE_SPELL, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot(player, card, player, card, 1, 1);
		untap_card(player, card);
	}

	return 0;
}

int card_cryptborn_horror(int player, int card, event_t event){

	hybrid(player, card, event);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, get_trap_condition(1-player, TRAP_LIFE_LOST));

	return 0;
}

int card_deathrite_shaman(int player, int card, event_t event)
{
  /* Deathrite Shaman	|BG
   * Creature - Elf Shaman 1/2
   * |T: Exile target land card from a graveyard. Add one mana of any color to your mana pool.
   * |B, |T: Exile target instant or sorcery card from a graveyard. Each opponent loses 2 life.
   * |G, |T: Exile target creature card from a graveyard. You gain 2 life. */

	// The first ability isn't a mana ability, since it has a target.  Therefore it's also not "tapping to produce mana" for e.g. Mana Reflection.

	hybrid(player, card, event);

#define CAN_PRODUCE_MANA(player, card) (CAN_TAP(player, card) && CAN_ACTIVATE0(player, card) && any_in_graveyard_by_type(ANYBODY, TYPE_LAND))

	if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_PRODUCE_MANA(player, card) )
		declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, 1);

	if (!IS_ACTIVATING(event))
		return 0;

	enum{
		CHOICE_CANCEL = 0,
		CHOICE_MANA = 1,
		CHOICE_LOSE_LIFE = 2,
		CHOICE_GAIN_LIFE = 3
	} choice = CHOICE_CANCEL;

	if (event == EVENT_CAN_ACTIVATE){
		if (!can_use_activated_abilities(player, card) || !CAN_TAP(player, card) || graveyard_has_shroud(ANYBODY))
			return 0;

		if (paying_mana() && has_mana_for_activated_ability(player, card, MANACOST0) && any_in_graveyard_by_type(ANYBODY, TYPE_LAND))
			choice = CHOICE_MANA;
	}

	if (choice == CHOICE_CANCEL){
		int abils[3] = {any_in_graveyard_by_type(ANYBODY, TYPE_LAND),
						any_in_graveyard_by_type(ANYBODY, TYPE_SPELL) && has_mana_for_activated_ability(player, card, MANACOST_B(CAN_PRODUCE_MANA(player, card) ? 2 : 1)),
						any_in_graveyard_by_type(ANYBODY, TYPE_CREATURE) && has_mana_for_activated_ability(player, card, MANACOST_G(CAN_PRODUCE_MANA(player, card) ? 2 : 1))
		};
		choice = DIALOG(player, card, event, DLG_RANDOM,
						"Produce mana", abils[0], -1,
						"Lose life", abils[1], 20-life[1-player],
						"Gain life", abils[2], 20-life[player]);

	}
	if (event == EVENT_CAN_ACTIVATE)
		return choice;
	else if (event == EVENT_ACTIVATE){
			if( choice == CHOICE_CANCEL ){
				spell_fizzled = 1;
				return 0;
			}
			test_definition_t test;
			add_state(player, card, STATE_TAPPED);
			if( charge_mana_for_activated_ability(player, card, 0, (choice == CHOICE_LOSE_LIFE ? 1 : 0), 0,	(choice == CHOICE_GAIN_LIFE ? 1 : 0), 0, 0) ){
				switch (choice)
				{
					case CHOICE_CANCEL:
						return 0;

					case CHOICE_MANA:
						new_default_test_definition(&test, TYPE_LAND, "Select target land card.");
						break;

					case CHOICE_LOSE_LIFE:
						new_default_test_definition(&test, TYPE_SPELL, "Select target instant or sorcery card.");
						break;

					case CHOICE_GAIN_LIFE:
						new_default_test_definition(&test, TYPE_CREATURE, "Select target creature card.");
						break;
				}

				if (select_target_from_either_grave(player, card, 0, AI_MIN_VALUE, AI_MAX_VALUE, &test, 0, 1) != -1){
					tap_card(player, card);
				}
				else{
					remove_state(player, card, STATE_TAPPED);
				}
			}
			else{
				remove_state(player, card, STATE_TAPPED);
			}
	}
  else	// event == EVENT_RESOLVE_ACTIVATION
	{
	  int whose_graveyard = get_card_instance(player, card)->targets[0].player;

	  int selected = validate_target_from_grave_source(player, card, whose_graveyard, 1);
	  if (selected == -1)
		{
		  spell_fizzled = 1;
		  return 0;
		}

	  rfg_card_from_grave(whose_graveyard, selected);
	  switch (choice)
		{
		  case CHOICE_CANCEL:
			return 0;

		  case CHOICE_MANA:
			FORCE(produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, 1));
			break;

		  case CHOICE_LOSE_LIFE:
			lose_life(1-player, 2);
			break;

		  case CHOICE_GAIN_LIFE:
			gain_life(player, 2);
			break;
		}
	}

  return 0;
}

int card_dryad_militant(int player, int card, event_t event)
{
  /* Dryad Militant	|GW
   * Creature - Dryad Soldier 2/1
   * If an instant or sorcery card would be put into a graveyard from anywhere, exile it instead. */

  hybrid(player, card, event);

  enable_xtrigger_flags |= ENABLE_XTRIGGER_REPLACE_MILL | ENABLE_XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY;
  if (event == EVENT_GRAVEYARD_FROM_PLAY || xtrigger_condition() == XTRIGGER_REPLACE_MILL || xtrigger_condition() == XTRIGGER_REPLACE_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_SPELL, "");

	  if_a_card_would_be_put_into_graveyard_from_anywhere_exile_it_instead(player, card, event, ANYBODY, &test);
	}

  return 0;
}

int card_frostburn_weird(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	hybrid(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 1, 0, 0, 0);
		if( has_mana_hybrid(player, 1, COLOR_BLUE, COLOR_RED, c1) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 1, 0, 0, 0);
		charge_mana_hybrid(player, card, 1, COLOR_BLUE, COLOR_RED, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, -1);
	}

	return 0;
}

int card_golgari_longlegs(int player, int card, event_t event){

	hybrid(player, card, event);

	return 0;
}

int card_growing_ranks(int player, int card, event_t event){

	hybrid(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		populate(player, card);
	}

	return global_enchantment(player, card, event);
}

int card_judges_familiar(int player, int card, event_t event){

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_SPELL);

	hybrid(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, &td, 0, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_SPELL_ON_STACK, 0, 0, 0, 0, 0, 0, 0, &td, NULL);
}

int card_nivmagus_elemental(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	hybrid(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int result = card_spiketail_hatchling(player, card, event);
			if( result > 0 && card_on_stack_controller == player && ! is_what(card_on_stack_controller, card_on_stack, TYPE_PERMANENT) ){
				return result;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			set_flags_when_spell_is_countered(player, card, card_on_stack_controller, card_on_stack);
			kill_card(card_on_stack_controller, card_on_stack, KILL_REMOVE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_1_1_counters(player, instance->parent_card, 2);
	}

	return 0;
}

int card_rakdos_cackler(int player, int card, event_t event){

	hybrid(player, card, event);

	return unleash(player, card, event);
}

int card_rakdos_shred_freak(int player, int card, event_t event){

	haste(player, card, event);

	return hybrid(player, card, event);
}

int card_slitherhead(int player, int card, event_t event){

	hybrid(player, card, event);

	return scavenge(player, card, event, 0, 0, 0, 0, 0, 0);
}

int card_sundering_growth(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance( player, card );

	modify_cost_for_hybrid_spells(player, card, event, 0);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( hybrid_casting(player, card, 0) ){
			pick_target(&td, "DISENCHANT");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			populate(player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// vassal soul --> golgari longlegs

// Artifacts

int card_azorius_keyrune(int player, int card, event_t event){
	return keyrune(player, card, event, 0, 0, 1, 0, 0, 1, 2, 2, KEYWORD_FLYING, 0, COLOR_TEST_BLUE+COLOR_TEST_WHITE, SUBTYPE_BIRD);
}

int card_chromatic_lantern(int player, int card, event_t event){
	// All of these are also handled later in permanents_you_control_can_tap_for_mana_all_one_color()

	if (event == EVENT_COUNT_MANA && affect_me(player, card) ){
		int result = !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card);
		result |= permanents_you_control_can_tap_for_mana_all_one_color(player, card, EVENT_CAN_ACTIVATE, TYPE_LAND, -1, COLOR_TEST_ANY_COLORED, 1);
		if( result ){
			declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, 1);
		}
	}

	if (event == EVENT_CAN_ACTIVATE){
		if (!is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card)){
			return 1;
		}
		return permanents_you_control_can_tap_for_mana_all_one_color(player, card, event, TYPE_LAND, -1, COLOR_TEST_ANY_COLORED, 1);
	}

	if (event == EVENT_ACTIVATE){
		if (!is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card)){
			produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, 1);
			return 0;
		}
		return permanents_you_control_can_tap_for_mana_all_one_color(player, card, event, TYPE_LAND, -1, COLOR_TEST_ANY_COLORED, 1);
	}

	return 0;
}

int card_civic_saber(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && is_equipping(player, card) ){
		int amount = count_colors(instance->targets[8].player, instance->targets[8].card);
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, amount, 0, 0);
	}
	return basic_equipment(player, card, event, 1);
}

int card_codex_shredder(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( can_target(&td) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)  ){
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, 5, 0, 0, 0, 0, 0) && count_deck(player) > 0 && can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return ! graveyard_has_shroud(2);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( can_target(&td)  ){
			if( has_mana_for_activated_ability(player, card, 5, 0, 0, 0, 0, 0) && count_deck(player) > 0 && can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Mill a player\n Sac and return a card from graveyard\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 5*choice, 0, 0, 0, 0, 0) ){
				if( choice == 0 ){
					if( pick_target(&td, "TARGET_PLAYER") ){
						instance->number_of_targets = 1;
						tap_card(player, card);
						instance->info_slot = 66+choice;
					}
				}
				else if( choice == 1 ){
							test_definition_t this_test;
							new_default_test_definition(&this_test, TYPE_ANY, "Select a card to return to your hand.");
							int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, 1, -1, &this_test);
							if( selected != -1  ){
								const int *grave = get_grave(player);
								instance->targets[0].player = selected;
								instance->targets[0].card = grave[selected];
								instance->info_slot = 66+choice;
								tap_card(player, card);
								kill_card(player, card, KILL_SACRIFICE);
							}
							else{
								spell_fizzled = 1;
							}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			mill(instance->targets[0].player, 1);
		}
		if( instance->info_slot == 67 ){
			const int *grave = get_grave(player);
			int selected = instance->targets[0].player;
			if( instance->targets[0].card == grave[selected] ){
				add_card_to_hand(player, grave[selected]);
				remove_card_from_grave(player, selected);
			}
		}
	}

	return 0;
}

int card_golgari_keyrune(int player, int card, event_t event){
	return keyrune(player, card, event, 0, 1, 0, 1, 0, 0, 2, 2, 0, SP_KEYWORD_DEATHTOUCH, COLOR_TEST_BLACK+COLOR_TEST_GREEN, SUBTYPE_INSECT);
}

int card_izzet_keyrune(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL+DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}
	return keyrune(player, card, event, 0, 0, 1, 0, 1, 0, 2, 1, 0, 0, COLOR_TEST_BLUE+COLOR_TEST_RED, SUBTYPE_ELEMENTAL);
}

int card_rakdos_keyrune(int player, int card, event_t event){
	return keyrune(player, card, event, 0, 1, 0, 0, 1, 0, 3, 1, KEYWORD_FIRST_STRIKE, 0, COLOR_TEST_BLACK+COLOR_TEST_RED, SUBTYPE_DEVIL);
}

int card_selesnya_keyrune(int player, int card, event_t event){
	return keyrune(player, card, event, 0, 0, 0, 1, 0, 1, 3, 3, 0, 0, COLOR_TEST_WHITE+COLOR_TEST_GREEN, SUBTYPE_WOLF);
}

int card_street_sweeper(int player, int card, event_t event)
{
  // Whenever ~ attacks, destroy all Auras attached to target land.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_LAND);
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_LAND"))
		{
		  // Enchantments don't seem to go through a dying/graveyard-order stage like other permanents, so mark/sweep.
		  int marked[2][151] = {{0}};
		  card_instance_t* aura;
		  int p, c;
		  for (p = 0; p <= 1; ++p)
			for (c = 0; c < active_cards_count[p]; ++c)
			  if ((aura = in_play(p, c)) && is_what(p, c, TYPE_ENCHANTMENT) && has_subtype(p, c, SUBTYPE_AURA)
				  && aura->damage_target_player == instance->targets[0].player && aura->damage_target_card == instance->targets[0].card)
				marked[p][c] = 1;

		  for (p = 0; p <= 1; ++p)
			for (c = 0; c < active_cards_count[p]; ++c)
			  if (marked[p][c] && in_play(p, c))
				kill_card(p, c, KILL_DESTROY);
		}
	}
	return 0;
}

int card_tablet_of_the_guilds(int player, int card, event_t event){

	/* Tablet of the Guilds	|2
	 * Artifact
	 * As ~ enters the battlefield, choose two colors.
	 * Whenever you cast a spell, if it's at least one of the chosen colors, you gain 1 life for each of the chosen colors it is. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int clrs = 0;
		if( !IS_AI(player) ){
			int c1 = choose_a_color(player, 0);
			create_card_name_legacy(player, card, CARD_ID_BLACK+(c1-1));
			clrs = 1 << c1;
			c1 = choose_a_color_from(player, 1, COLOR_TEST_ANY_COLORED & ~clrs);
			create_card_name_legacy(player, card, CARD_ID_BLACK+(c1-1));
			clrs |= 1 << c1;
		}
		else{
			int c1 = choose_a_color_and_show_legacy(player, card, player, -1);
			clrs = 1 << c1;
			int c2 = c1+1;
			if( c2 > 5 ){
				c2 = c1-1;
			}
			clrs |= 1 << c2;
			create_card_name_legacy(player, card, CARD_ID_BLACK+(c2-1));
		}
		instance->info_slot = clrs;
	}

	if( specific_spell_played(player, card, event, player, 2, TYPE_LAND,DOESNT_MATCH, 0,0, instance->info_slot,MATCH, 0,0, -1,0) ){
		gain_life(player, num_bits_set(get_color(instance->targets[1].player, instance->targets[1].card) & instance->info_slot));
	}

	return 0;
}

int card_volatile_rig(int player, int card, event_t event){

	attack_if_able(player, card, event);

	if( damage_dealt_to_me_arbitrary(player, card, event, 0, player, card) ){
		if( ! flip_a_coin(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( ! flip_a_coin(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_damage_all(player, card, ANYBODY, 4, NDA_PLAYER_TOO, &this_test);
		}
	}

	return 0;
}

// Lands
int card_azorius_guildgate(int player, int card, event_t event){
	// Also code for other "gate" lands.
	comes_into_play_tapped(player, card, event);
	return mana_producer(player, card, event);
}

int card_grove_of_the_guardian(int player, int card, event_t event){
	/* Grove of the Guardian	""
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |3|G|W, |T, Tap two untapped creatures you control, Sacrifice ~: Put an 8/8 |Sgreen and |Swhite Elemental creature token with vigilance onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card)  ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( ! is_tapped(player, card) ){
			return can_produce_mana(player, card);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 4, 0, 0, 1, 0, 1) && target_available(player, card, &td) > 1 &&
			can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
			){
			ai_choice = 1;
		}
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Generate an Elemental\n Cancel", ai_choice);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 3, 0, 0, 1, 0, 1);
			if( spell_fizzled != 1 ){
				int t = 0;
				while( t<2 ){
					if( select_target(player, card, &td, "Select a creature to tap", &(instance->targets[t])) ){
						state_untargettable(instance->targets[t].player, instance->targets[t].card, 1);
						t++;
					}
					else{
						break;
					}
				}
				int k;
				for(k=0; k<2; k++){
					if( instance->targets[k].player != -1 ){
						state_untargettable(instance->targets[k].player, instance->targets[k].card, 0);
						if( t == 2 ){
							tap_card(instance->targets[k].player, instance->targets[k].card);
						}
					}
				}
				if( t < 2 ){
					spell_fizzled = 1;
					untap_card_no_event(player, card);
				}
				else{
					instance->info_slot = 1;
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
			token.pow = 8;
			token.tou = 8;
			token.s_key_plus = SP_KEYWORD_VIGILANCE;
			token.color_forced = COLOR_TEST_WHITE | COLOR_TEST_GREEN;
			generate_token(&token);
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

int card_rogues_passage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card)  ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( ! is_tapped(player, card) ){
			return can_produce_mana(player, card);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 5, 0, 0, 0, 0, 0) && can_target(&td) ){
			ai_choice = 1;
		}
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Target cannot be blocked until eot\n Cancel", ai_choice);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
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

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
			parent->info_slot = 0;
			if( valid_target(&td) ){
				pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									   0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
			}
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

int card_transguild_promenade(int player, int card, event_t event){

	/* Transguild Promenade	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, sacrifice it unless you pay |1.
	 * |T: Add one mana of any color to your mana pool. */

	/* Rupture Spire	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, sacrifice it unless you pay |1.
	 * |T: Add one mana of any color to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event) && !charge_mana_while_resolving(player, card, 0, player, COLOR_COLORLESS, 1)){
		ai_modifier += player == AI ? -128 : 128;
		kill_card(player, card, KILL_SACRIFICE);
	}

	return mana_producer(player, card, event);
}

