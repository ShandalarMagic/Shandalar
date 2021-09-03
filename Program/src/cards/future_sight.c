#include "manalink.h"

// Functions

static const char* exchangeable_aura(int who_chooses, int player, int card)
{
	if( has_subtype(player, card, SUBTYPE_AURA_CREATURE) || has_subtype(player, card, SUBTYPE_AURA_PERMANENT) ){
		return NULL;
	}
	return "this aura cannot enchant a creature.";
}

static int aura_swap(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white,
					 int p_pump, int t_pump, int key_pump, int s_key_pump ){

	/* Aura swap |2|U (|2|U: Exchange this Aura with an Aura card in your hand.) */

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){

		if( event == EVENT_CAN_ACTIVATE ){
			if( generic_activated_ability(player, card, event, 0, cless, black, blue, green, red, white, 0, NULL, NULL) ){
				target_definition_t td;
				base_target_definition(player, card, &td, TYPE_ENCHANTMENT);
				td.zone = TARGET_ZONE_HAND;
				td.extra = (int32_t)exchangeable_aura;
				td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
				td.illegal_abilities = 0;

				return can_target(&td);
			}
		}

		if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, cless, black, blue, green, red, white);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			target_definition_t td;
			base_target_definition(player, card, &td, TYPE_ENCHANTMENT);
			td.zone = TARGET_ZONE_HAND;
			td.extra = (int32_t)exchangeable_aura;
			td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
			td.illegal_abilities = 0;

			if( can_target(&td) && new_pick_target(&td, "Select an Aura that can enchant this creature.", 0, GS_LITERAL_PROMPT) ){
				int selected = instance->targets[0].card;
				put_into_play_aura_attached_to_target(player, selected, instance->damage_target_player, instance->damage_target_card);
				bounce_permanent(instance->parent_controller, instance->parent_card);
			}
		}
	}

	return generic_aura(player, card, event, player, p_pump, t_pump, key_pump, s_key_pump, 0, 0, 0);
}

static int discard_id_from_hand(int player, int id){

	if( hand_count[player] > 0 ){
		int count = active_cards_count[player]-1;
		while( count > -1  ){
				if( in_hand(player, count) && get_id(player, count) == id ){
					discard_card(player, count);
					return count;
				}
				count--;
		}
	}

	return 0;
}

static int grandeur(int player, int card, event_t event ){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return is_id_in_hand(player, get_id(player, card));
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			discard_id_from_hand(player, get_id(player, card));
		}
	}

	return 0;
}

void modify_cost_for_delve(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_MODIFY_COST ){
		const int *grave = get_grave(player);
		if( grave[0] != -1 ){
			card_ptr_t* c = cards_ptr[ get_id(player, card) ];
			int cless = get_updated_casting_cost(player, card, -1, event, -1);
			cless-=count_graveyard(player);
			if( cless < 0 ){
				cless = 0;
			}
			if( has_mana_multi(player, cless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white) ){
				instance->info_slot = 1;
				null_casting_cost(player, card);
			}
		}
		else{
			instance->info_slot = 0;
		}
	}
}

int cast_spell_with_delve(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	int choice = 0;
	if( has_mana_to_cast_iid(player, EVENT_CAST_SPELL, get_card_instance(player, card)->internal_card_id) ){
		choice = do_dialog(player, player, card, -1, -1, " Use Delve\n Play normally\n Cancel", 1);
	}
	if( choice == 2 ){
		spell_fizzled = 1;
		return 0;
	}
	int i;
	int id = get_id(player, card);
	card_ptr_t* c = cards_ptr[ id ];
	int cless = get_updated_casting_cost(player, card, -1, EVENT_CAST_SPELL, -1);
	if( choice == 0 ){
		int max = MIN(cless, count_graveyard(player));
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile for Delve.");
		int trc = select_multiple_cards_from_graveyard(player, player, 0, AI_MIN_VALUE, &this_test, max, &instance->targets[0]);
		charge_mana_multi(player, cless-trc, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white);
		if( spell_fizzled != 1 ){
			int result = get_id(player, card) == CARD_ID_SOULFLAYER ? 0 : 2;
			const int *grave = get_grave(player);
			for(i=0; i<trc; i++){
				if( get_id(player, card) == CARD_ID_SOULFLAYER ){
					int ability_mask = KEYWORD_FIRST_STRIKE | KEYWORD_DOUBLE_STRIKE | KEYWORD_TRAMPLE | KEYWORD_FLYING | KEYWORD_REACH;
					result |= (cards_data[grave[instance->targets[i].player]].static_ability & ability_mask);
				}
				rfg_card_from_grave(player, instance->targets[i].player);
			}
			return result;
		}
		else{
			return 0;
		}
	}

	if( choice == 1 ){
		charge_mana_multi(player, cless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white);
		if( spell_fizzled != 1 ){
			return 1;
		}
	}
	return 0;
}

int delve(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	modify_cost_for_delve(player, card, event);
	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			if( ! cast_spell_with_delve(player, card) ){
				spell_fizzled = 1;
			}
		}
	}
	return 0;
}

int slivercycling(int player, int card, event_t event){
	if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
		return cycling(player, card, event, 3, 0, 0, 0, 0, 0);
		/*
		if( check_battlefield_for_id(2, CARD_ID_HOMING_SLIVER) || get_id(player, card) == CARD_ID_HOMING_SLIVER ){
			int amount = 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
			int colorless = 3-amount;

			if( colorless < 0 ){
				colorless = 0;
			}
			if( has_mana(player, COLOR_COLORLESS, colorless) ){
				return 1;
			}
		}
		*/
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			return cycling(player, card, event, 3, 0, 0, 0, 0, 0);
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			char msg[100] = "Select a Sliver card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			this_test.subtype = SUBTYPE_SLIVER;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}
	return 0;
}

static int generic_pact_legacy(int player, int card, event_t event)
{
  card_instance_t *instance = get_card_instance(player, card);

  if (current_turn == instance->targets[0].player && upkeep_trigger(player, card, event))
	{
	  if (!charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, instance->targets[0].player,
											 instance->targets[1].player, instance->targets[2].player, instance->targets[3].player,
											 instance->targets[4].player, instance->targets[5].player, instance->targets[6].player))
		lose_the_game(instance->targets[0].player);

	  kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}

static int create_pact_legacy(int player, int card, int t_player, int cless, int black, int blue, int green, int red, int white){
	int legacy = create_legacy_effect(player, card, &generic_pact_legacy);
	card_instance_t *instance = get_card_instance(player, legacy);
	instance->targets[0].player = t_player;
	instance->targets[1].player = cless;
	instance->targets[2].player = black;
	instance->targets[3].player = blue;
	instance->targets[4].player = green;
	instance->targets[5].player = red;
	instance->targets[6].player = white;
	return legacy;
}

static int has_virtual_mana_multi(int player, int cless, int black, int blue, int green, int red, int white){
	int total[] = { cless, black, blue, green, red, white };
	int i, card;
	for (card = 0; card < active_cards_count[player]; ++card){
		if (in_play(player, card) && is_what(player, card, TYPE_PERMANENT)){
			card_data_t* card_d = get_card_data(player, card);
			if (card_d->extra_ability & EA_MANA_SOURCE){
				card_instance_t *instance = get_card_instance(player, card);
				int clr = get_color_of_mana_produced_by_id(card_d->id, instance->info_slot, player);
				if (clr == -1){
					clr = card_d->color;
				}
				for (i = COLOR_COLORLESS; i <= COLOR_WHITE; ++i){
					if (clr & (1 << i)){
						if (total[i] > 0){	// apply it to the appropriate color if needed
							--total[i];
						} else {
							--total[COLOR_COLORLESS];	// otherwise, apply it to colorless
						}
					}
				}
			}
		}
	}
	for (i = COLOR_COLORLESS; i <= COLOR_WHITE; ++i){
		if (total[i] > 0){
			return 0;
		}
	}
	return 1;
}

static void fateseal(int player, int number_of_cards){
	return scrylike_effect(player, 1-player, number_of_cards);
}


// Cards

int card_akromas_memorial(int player, int card, event_t event)
{
  // Legendary Artifact
  check_legend_rule(player, card, event);

  // Creatures you control have flying, first strike, vigilance, trample, haste, and protection from black and from red.
  if (event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && !is_humiliated(player, card))
	{
	  event_result |= get_sleighted_protection(player, card, KEYWORD_FLYING | KEYWORD_FIRST_STRIKE | KEYWORD_TRAMPLE | KEYWORD_PROT_BLACK | KEYWORD_PROT_RED);
	  vigilance(affected_card_controller, affected_card, event);
	  haste(affected_card_controller, affected_card, event);
	}

  return 0;
}

int card_angel_of_salvation(int player, int card, event_t event)
{
  /* Angel of Salvation	|6|W|W
   * Creature - Angel 5/5
   * Flash; convoke
   * Flying
   * When ~ enters the battlefield, prevent the next 5 damage that would be dealt this turn to any number of target creatures and/or players, divided as you
   * choose. */

  if (event == EVENT_MODIFY_COST && has_convoked_mana(player, card, 6, 0, 2))
	{
	  COST_COLORLESS -= 6;
	  COST_WHITE -= 2;
	}

  if (event == EVENT_CAN_CAST)
	return (land_can_be_played & LCBP_DAMAGE_PREVENTION) ? 99 : 1;

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && !played_for_free(player, card) && !is_token(player, card)
	  && !charge_convoked_mana(player, card, 6, 0, 2))
	cancel = 1;

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	  td.preferred_controller = player;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);

	  instance->number_of_targets = 0;
	  int i, j;
	  for (i = 0; i < 5; ++i)
		{
		  char prompt[200];
		  if (ai_is_speculating == 1)
			prompt[0] = 0;
		  else
			{
			  load_text(0, "TARGET_CREATURE_OR_PLAYER");
			  scnprintf(prompt, 200, "%s (%d of 5)", text_lines[0], i + 1);
			}

		  if (can_target(&td) && pick_next_target_noload(&td, prompt)
			  && instance->targets[i].card != -1)
			get_card_instance(instance->targets[i].player, instance->targets[i].card)->state |= STATE_TARGETTED;
		}

	  for (i = 0; i < 5; ++i)
		if (instance->targets[i].player != -1)
		  {
			if (instance->targets[i].card != -1)
			  get_card_instance(instance->targets[i].player, instance->targets[i].card)->state &= ~STATE_TARGETTED;

			// consolidate targets chosen more than once
			int prev = 1;
			for (j = i + 1; j < 5; ++j)
			  if (instance->targets[j].player == instance->targets[i].player
				  && instance->targets[j].card == instance->targets[i].card)
				{
				  ++prev;
				  instance->targets[j].player = -1;
				}

			prevent_the_next_n_damage(player, card, instance->targets[i].player, instance->targets[i].card, prev, 0, -1, -1);
		  }
	}

  return flash(player, card, event);
}

int card_arcanum_wings(int player, int card, event_t event){
	return aura_swap(player, card, event, 2, 0, 1, 0, 0, 0, 0, 0, KEYWORD_FLYING, 0);
}

int card_augur_of_skulls(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST_XB(1, 1), 0, NULL, NULL) == 99){
			return 99;
		}
		return generic_activated_ability(player, card, event, GAA_ONLY_ON_UPKEEP | GAA_IN_YOUR_TURN | GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST0, 0,
										&td, "TARGET_PLAYER");
	}

	if(event == EVENT_ACTIVATE ){
		if( regeneration(player, card, EVENT_CAN_ACTIVATE, MANACOST_XB(1, 1)) ){
			generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST_XB(1, 1), 0, NULL, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66;
			}
		}
		else{
			generic_activated_ability(player, card, event, GAA_ONLY_ON_UPKEEP | GAA_IN_YOUR_TURN | GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST0, 0,
										&td, "TARGET_PLAYER");
			if( spell_fizzled != 1 ){
				instance->info_slot = 67;
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && can_regenerate(instance->parent_controller, instance->parent_card) ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			new_multidiscard(instance->targets[0].player, 2, 0, player);
		}
	}

	return 0;
}

int card_aven_mindcensor(int player, int card, event_t event){
	return flash(player, card, event);
}

int card_barren_glory(int player, int card, event_t event){

	if( hand_count[player] < 1 && ! is_humiliated(player, card) &&
		(event == EVENT_SHOULD_AI_PLAY || (current_turn == player && upkeep_trigger(player, card, event))) &&
		count_subtype(player, TYPE_PERMANENT, -1) == 1
	  ){
		lose_the_game(1-player);
	}
	return global_enchantment(player, card, event);
}

int card_baru_fist_of_krosa(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( specific_cip(player, card, event, player, ANYBODY, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_GREEN;
		pump_creatures_until_eot(player, card, player, 0, 1, 1, KEYWORD_TRAMPLE, 0, &this_test);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WURM, &token);
		token.pow = count_subtype(player, TYPE_LAND, -1);
		token.tou = count_subtype(player, TYPE_LAND, -1);
		generate_token(&token);
	}

	return grandeur(player, card, event);
}

int card_bitter_ordeal(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PLAYER")  ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");

			int amount = get_dead_count(2, TYPE_PERMANENT);
			int i = 0;
			int cd = count_deck(instance->targets[0].player);
			while( i < amount && cd > 0 ){
					if( new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_RFG, 0, 1, &this_test) != -1 ){
						cd--;
					}
					i++;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) && deck_ptr[instance->targets[0].player][0] != -1){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_RFG, 0, 1, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bonded_fetch(int player, int card, event_t event){
	haste(player, card, event);
	return card_merfolk_looter(player, card, event);
}

int card_bridge_from_below(int player, int card, event_t event){
	/* Bridge from Below	|B|B|B
	 * Enchantment
	 * Whenever a nontoken creature is put into your graveyard from the battlefield, if ~ is in your graveyard, put a 2/2 |Sblack Zombie creature token onto the battlefield.
	 * When a creature is put into an opponent's graveyard from the battlefield, if ~ is in your graveyard, exile ~. */

	// the code for this card is below
	if( event == EVENT_CAN_CAST ){
		if( player != AI ){
			return 1;
		}
	}
	return 0;
}

int bridge_from_below(int player, int card, event_t event){

	if (event == EVENT_GRAVEYARD_ABILITY){
		return 0;
	}

	card_instance_t *instance= get_card_instance(player, card);
	if( event == EVENT_GRAVEYARD_FROM_PLAY){
		if( ! in_play(affected_card_controller, affected_card) || count_graveyard_by_id(player, CARD_ID_BRIDGE_FROM_BELOW) < 1 ){
			return 0;
		}
		card_instance_t *affected= get_card_instance(affected_card_controller, affected_card);
		if( affected->kill_code > 0 && affected->kill_code != 4) {
			if( cards_data[ affected->internal_card_id ].type & TYPE_CREATURE ){
				if( instance->parent_controller < 0 ){
					instance->parent_controller = 0;
				}
				if( instance->parent_card < 0 ){
					instance->parent_card = 0;
				}

				// if it's my creature, make a zombie
				if( affected_card_controller == player ){
					// skip tokens and unearthed cards
					if( ! ( affected->token_status & STATUS_TOKEN ) ){
						instance->parent_controller++;
					}
				}
				else{
					// if it's an opponent's creature, RFG the bridges
					instance->parent_card++;
					instance->parent_controller++;
				}
			}
		}
	}

	int temp = instance->parent_controller;
	instance->targets[11].player = instance->parent_controller;
	int result = resolve_graveyard_trigger(player, card, event );
	instance->targets[11].player = temp;

	if( result == 1 ){
		int i=0;
		int j=0;

		if( instance->parent_card > 0 ){
			const int *graveyard = get_grave(player);
			for(i=0;i<count_graveyard(player);i++){
				card_data_t* card_d = &cards_data[ graveyard[i] ];
				if( card_d->id == CARD_ID_BRIDGE_FROM_BELOW ){
					rfg_card_from_grave(player, i);
					i--;
				}
			}
		}

		for(j=0;j<instance->parent_controller - instance->parent_card;j++){
			for(i=0;i<count_graveyard_by_id(player, CARD_ID_BRIDGE_FROM_BELOW);i++){
				generate_token_by_id(player, card, CARD_ID_ZOMBIE);
			}
		}

		instance->parent_controller = 0;
		instance->parent_card = 0;
	}
	return 0;
}

// bound in silence --> pacifism

int card_centaur_omenreader(int player, int card, event_t event){
	if( is_tapped(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
			if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
				COST_COLORLESS-=2;
			}
		}
	}
	return 0;
}

int card_chronomantic_escape(int player, int card, event_t event){

	/* Chronomantic Escape	|4|W|W
	 * Sorcery
	 * Until your next turn, creatures can't attack you. Exile ~ with three time counters on it.
	 * Suspend 3-|2|W */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		nobody_can_attack_until_your_next_turn(player, card, 1-player);
		suspend_a_card(player, card, player, card, 3);
	}

	return suspend(player, card, event, 3, MANACOST_XW(2, 1), NULL, NULL);
}

int card_cloud_key(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int choice = do_dialog(player, player, card, -1, -1, " Creature\n Enchantment\n Sorcery\n Instant\n Artifact", internal_rand(5));
		if( choice == 4 ){
			choice++;
		}
		instance->targets[1].card = 1<<(choice+1);
	}

	if( instance->targets[1].card > -1 && event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player && ! is_humiliated(player, card) ){
		int good = 0;
		if( instance->targets[1].card != TYPE_INSTANT ){
			if( is_what(affected_card_controller, affected_card, instance->targets[1].card) && ! is_planeswalker(affected_card_controller, affected_card) ){
				good = 1;
			}
		}
		else{
			if( (is_what(affected_card_controller, affected_card, TYPE_INSTANT) || is_what(affected_card_controller, affected_card, TYPE_INTERRUPT)) &&
				! is_what(affected_card_controller, affected_card, TYPE_CREATURE)
			  ){
				good = 1;
			}
		}
		if( good == 1 ){
			COST_COLORLESS--;
		}
	}
	return 0;
}

int card_cloudseeder(int player, int card, event_t event){
	/* Cloudseeder	|1|U
	 * Creature - Faerie Spellshaper 1/1
	 * Flying
	 * |U, |T, Discard a card: Put a 1/1 |Sblue Faerie creature token named Cloud Sprite onto the battlefield. It has flying and "Cloud Sprite can block only creatures with flying." */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_CLOUD_SPRITE);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD, MANACOST_U(1), 0, NULL, NULL);
}

int card_coalition_relic(int player, int card, event_t event){

	/* Coalition Relic	|3
	 * Artifact
	 * |T: Add one mana of any color to your mana pool.
	 * |T: Put a charge counter on ~.
	 * At the beginning of your precombat main phase, remove all charge counters from ~. Add one mana of any color to your mana pool for each charge counter
	 * removed this way. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL)  ){
			return 1;
		}
		if( can_produce_mana(player, card) ){
			return 1;
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = 0;
		if( can_produce_mana(player, card) ){
			if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST0, 0, NULL, NULL) && player != AI  ){
				choice = do_dialog(player, player, card, -1, -1, " Get Mana\n Add Charge counter\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST0)  ){
					tap_card(player, card);
					instance->info_slot = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
			parent->targets[1].player = 66;
			add_counter(instance->parent_controller, instance->parent_card, COUNTER_CHARGE);
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	if( instance->targets[1].player != 66 && event != EVENT_RESOLVE_ACTIVATION ){
		if( current_turn == player && current_phase == PHASE_MAIN1 && count_counters(player, card, COUNTER_CHARGE) > 0 ){
			instance->targets[1].player = 66;
			int counters = count_counters(player, card, COUNTER_CHARGE);
			FORCE(produce_mana_any_combination_of_colors(player, COLOR_TEST_ANY_COLORED, counters, NULL));
			remove_all_counters(player, card, COUNTER_CHARGE);
		}
	}

	if( player == AI && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && current_turn == 1-player && eot_trigger(player, card, event) &&
		generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST0, 0, NULL, NULL)
	  ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0)  ){
			tap_card(player, card);
			add_counter(player, card, COUNTER_CHARGE);
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_cryptic_anellid(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		int i;
		for(i = 1; i <= 3; i++){
			scry(player, i);
		}
	}
	return 0;
}

int card_dakmor_salvage(int player, int card, event_t event)
{
	comes_into_play_tapped(player, card, event);

	int dr = dredge(player, card, event, 2);
	if (dr){
		return dr;
	}

	if (in_play(player, card)){
		return mana_producer(player, card, event);
	}

	return 0;
}

static const char* target_is_already_enchanted(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return is_enchanted(player, card) ? NULL : "another Aura attached";
}
int card_daybreak_coronet(int player, int card, event_t event)
{
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int32_t)target_is_already_enchanted;

  card_instance_t* instance;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && (instance = in_play(player, card))
	  && affect_me(instance->damage_target_player, instance->damage_target_card))
	event_result += 3;

  if (event == EVENT_ABILITIES
	  && (instance = in_play(player, card))
	  && affect_me(instance->damage_target_player, instance->damage_target_card))
	{
	  event_result |= KEYWORD_FIRST_STRIKE;
	  vigilance(instance->damage_target_player, instance->damage_target_card, event);
	  lifelink(instance->damage_target_player, instance->damage_target_card, event);
	}

  // Until aura target legality is handled centrally, though in this case it'll probably end up forwarding here anyway.
  if (event == EVENT_STATIC_EFFECTS
	  && (instance = in_play(player, card))
	  && instance->damage_target_player >= 0
	  && (instance->damage_target_card == -1
		  || count_auras_enchanting_me(instance->damage_target_player, instance->damage_target_card) < 2
		  || !is_what(instance->damage_target_player, instance->damage_target_card, TYPE_CREATURE)))
	kill_card(player, card, KILL_STATE_BASED_ACTION);

  return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

int card_deepcavern_imp(int player, int card, event_t event){

	haste(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		set_special_flags(player, card, SF_ECHO_TO_PAY);
	}

	if( check_special_flags(player, card, SF_ECHO_TO_PAY) && current_turn == player && upkeep_trigger(player, card, event) ){
		int kill = 1;
		if( hand_count[player] > 0 ){
			int choice = do_dialog(player, player, card, -1, -1, " Pay Echo\n Pass", 0);
			if( choice == 0 ){
				discard(player, 0, player);
				kill = 0;
				set_special_flags(player, card, SF_ECHO_PAID);
			}
		}
		if( kill == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return 0;
}

int card_death_rattle(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_GREEN;

	card_instance_t *instance = get_card_instance(player, card);

	modify_cost_for_delve(player, card, event);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		int result = 0;
		if( played_for_free(player, card) || is_token(player, card) ){
			result = 1;
		}
		else{
			result = cast_spell_with_delve(player, card);
		}
		if( result == 1 ){
			pick_target(&td, "TARGET_CREATURE");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
	}

	return 0;
}

int card_delay(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, NULL, 0);
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			int result = manage_counterspell_linked_hacks(player, card, instance->targets[0].player, instance->targets[0].card);
			if( result != KILL_REMOVE ){
				suspend_a_card(player, card, instance->targets[0].player, instance->targets[0].card, 3);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_dryad_arbor(int player, int card, event_t event){
	return mana_producer_fixed(player, card, event, COLOR_GREEN);
}

int card_edge_of_autumn(int player, int card, event_t event){

	/* Edge of Autumn	|1|G
	 * Sorcery
	 * If you control four or fewer lands, search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library.
	 * Cycling-Sacrifice a land. */

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( count_permanents_by_type(player, TYPE_LAND) < 4 ){
			ai_modifier-=1000;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( count_permanents_by_type(player, TYPE_LAND) < 5 ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, 4, TYPE_LAND, 0, SUBTYPE_BASIC, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return cycling_special_cost(player, card, event, 1);
}

int card_emberwilde_augur(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, 3, player, card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_ONLY_ON_UPKEEP+GAA_IN_YOUR_TURN+GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PLAYER");
}

int card_emblem_of_the_warmind(int player, int card, event_t event)
{
  boost_subtype(player, card, event, -1, 0,0, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

  return generic_aura(player, card, event, player, 0,0, 0,0, 0, 0, 0);
}

int card_epochrasite(int player, int card, event_t event){

	/* Epochrasite	|2
	 * Artifact Creature - Construct 1/1
	 * ~ enters the battlefield with three +1/+1 counters on it if you didn't cast it from your hand.
	 * When ~ dies, exile it with three time counters on it and it gains suspend. */

	if (not_played_from_hand(player, card)){
		enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 3);
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(player, card) ){
			card_instance_t *affected = get_card_instance(player, card);
			if( affected->kill_code > 0 && affected->kill_code < 4){
				suspend_a_card(player, card, player, card, 3);
				affected->kill_code = KILL_REMOVE;
			}
		}
	}

	if (event == EVENT_TOU_BOOST){
		return 99;
	}

	return 0;
}

int card_festering_march(int player, int card, event_t event){

	/* Festering March	|3|B|B
	 * Sorcery
	 * Creatures your opponents control get -1/-1 until end of turn. Exile ~ with three time counters on it.
	 * Suspend 3-|2|B */

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, 1-player, -1, -1, -1, 0, 0);
		suspend_a_card(player, card, player, card, 3);
	}

	return suspend(player, card, event, 3, MANACOST_XB(2, 1), NULL, NULL);
}

int card_fleshwrither(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card with CMC 4");
		this_test.cmc = 4;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_SORCERY_BE_PLAYED, MANACOST_XB(1, 2), 0, NULL, NULL);
}

int card_foresee(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		scry(player, 4);
		draw_cards (player, 2);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_frenzy_sliver(int player, int card, event_t event){
	if( event == EVENT_DECLARE_BLOCKERS && ! is_humiliated(player, card) ){
		int count = active_cards_count[current_turn]-1;
		while( count > -1 ){
			if( in_play(current_turn, count) && has_subtype(current_turn, count, SUBTYPE_SLIVER) && is_attacking(current_turn, count) &&
				is_unblocked(current_turn, count)
			  ){
				pump_until_eot(player, card, current_turn, count, 1, 0);
			}
			count--;
		}
	}
	return slivercycling(player, card, event);
}

int card_gathan_rider(int player, int card, event_t event){

	if( hand_count[player] < 1 && ! is_humiliated(player, card) ){
		modify_pt_and_abilities(player, card, event, 2, 2, 0);
	}

	if( event == EVENT_CAN_UNMORPH ){
		if( hand_count[player] > 0 ){
			return 1;
		}
	}

	if( event == EVENT_UNMORPH ){
		discard(player, 0, player);
	}

	return morph(player, card, event, MANACOST0);
}

int card_gibbering_descend(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST || event == EVENT_SHOULD_AI_PLAY ){
		return global_enchantment(player, card, event);
	}

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(current_turn, 1);
		discard(current_turn, 0, player);
	}

	return madness(player, card, event, MANACOST_XB(2, 2));
}

int card_glittering_wish(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		if( player == AI || ai_is_speculating == 1){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			this_test.color_flag = F3_MULTICOLORED;
			int id = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GET_ID, 0, 11, &this_test);
			if( id > -1 ){
				int iid = get_internal_card_id_from_csv_id(id);
				update_rules_engine(check_card_for_rules_engine(iid));
				int card_added = add_card_to_hand(player, iid);
				reveal_card(player, card, player, card_added);
			}
		}
		else{
			int crd = card_from_list(player, 4, TYPE_ANY, 0, 0, 0, 0, F3_MULTICOLORED, 0, 0, -1, 0);
			if( crd != -1 ){
				update_rules_engine(check_card_for_rules_engine(crd));
				add_card_to_hand(player, crd);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_goldmeadow_lookout(int player, int card, event_t event){
	/* Goldmeadow Lookout	|3|W
	 * Creature - Kithkin Spellshaper 2/2
	 * |W, |T, Discard a card: Put a 1/1 |Swhite Kithkin Soldier creature token named Goldmeadow Harrier onto the battlefield. It has "|W, |T: Tap target creature." */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_GOLDMEADOW_HARRIER);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD, MANACOST_W(1), 0, NULL, NULL);
}

int card_grinning_ignus(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_sorcery_be_played(player, event) && instance->targets[1].player != 66 ){
		if( has_mana(player, COLOR_RED, 1) ){
			declare_mana_available(player, COLOR_COLORLESS, 2);
			declare_mana_available(player, COLOR_RED, 1);
		}
	}

	if( event == EVENT_CAN_ACTIVATE && has_mana(player, COLOR_RED, 1) && can_sorcery_be_played(player, event) && instance->targets[1].player != 66 ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		instance->targets[1].player = 66;
		charge_mana(player, COLOR_RED, 1);
		if( spell_fizzled != 1 ){
			produce_mana_multi(player, MANACOST_XR(2, 1));
			bounce_permanent(player, card);
		}
	}

	return 0;
}

int card_grove_of_the_burnwillows(int player, int card, event_t event){

	if( event == EVENT_ACTIVATE ){
		int red_mana_before = mana_pool[COLOR_RED+8*player];
		int green_mana_before = mana_pool[COLOR_GREEN+8*player];
		mana_producer(player, card, event);
		if( red_mana_before < mana_pool[COLOR_RED+8*player] || green_mana_before < mana_pool[COLOR_GREEN+8*player] ){
			gain_life(1-player, 1);
		}
	}
	else{
		return mana_producer(player, card, event);
	}
	return 0;
}

int card_haze_of_rage(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}
	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = buyback(player, card, MANACOST_X(2));
		int i;
		for(i=0; i < get_storm_count(); i++){
			pump_subtype_until_eot(player, card, player, -1, 1, 0, 0, 0);
		}
	}
	if(event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 1, 0, 0, 0);
		if( instance->info_slot == 1){
			  bounce_permanent(player, card);
		}
		else{
			 kill_card(player, card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_heartwood_storyteller(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_CHECK_DECK_COUNT(player, 1), TYPE_CREATURE, DOESNT_MATCH, 0, 0, 0, 0, 0, 0, -1, 0) ){
		draw_cards(player, 1);
	}

	return 0;
}

#pragma message "Probably needs its own PLAYER_BITS"
int card_homing_sliver(int player, int card, event_t event){
	if( in_play(player, card) && event == EVENT_CHANGE_TYPE && affect_me(player, card) && ! is_humiliated(player, card) ){
		set_trap_condition(player, TRAP_HOMING_SLIVER, 1);
	}
	if( leaves_play(player, card, event) ){
		set_trap_condition(player, TRAP_HOMING_SLIVER, 0);
	}
	return slivercycling(player, card, event);
}

int card_horizon_canopy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( ! IS_GAA_EVENT(event) ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_produce_mana(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_pay_life(player, 1)){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL);
	}
	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( can_produce_mana(player, card) && can_pay_life(player, 1) ){
			if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Add G or W\n Sac and draw a card\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0){
			mana_producer(player, card, event);
			if( spell_fizzled != 1 ){
				lose_life(player, 1);
			}
		}
		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
					instance->info_slot = 1;
					kill_card(player, card, KILL_SACRIFICE);
				}
				else{
					remove_state(player, card, STATE_TAPPED);
				}
		}
		else if( choice == 2 ){
				spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION && instance->info_slot == 1 ){
		draw_a_card(player);
	}

	return 0;
}

int card_ichor_slick(int player, int card, event_t event){

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XB(3, 1));
	}

	if (event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND || event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
		return cycling(player, card, event, MANACOST_X(2));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card );
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -3, -3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_imperial_mask(int player, int card, event_t event){
	give_hexproof_to_player(player, card, event);
	return global_enchantment(player, card, event);
}

int card_intervention_pact(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.required_type = 0;
	td1.special = TARGET_SPECIAL_DAMAGE_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! has_virtual_mana_multi(player, MANACOST_XW(1, 2)) ){
			ai_modifier -= 1500;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			int amount = target->info_slot;
			target->info_slot = 0;
			gain_life(player, amount);
			create_pact_legacy(player, card, player, MANACOST_XW(1, 2));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td1, "TARGET_DAMAGE", 1, NULL);
}

int card_jhoira_of_the_ghitu(int player, int card, event_t event){

	/* Jhoira of the Ghitu	|1|U|R
	 * Legendary Creature - Human Wizard 2/2
	 * |2, Exile a nonland card from your hand: Put four time counters on the exiled card. If it doesn't have suspend, it gains suspend. */

	if( current_turn == 1-player && player == AI && trigger_condition == TRIGGER_EOT && affect_me(player, card ) && reason_for_trigger_controller == player&&
		! is_humiliated(player, card)
	  ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = DOESNT_MATCH;
		if( player == AI ){
			this_test.cmc = 2;
			this_test.cmc_flag = F5_CMC_GREATER_THAN_VALUE;
		}
		this_test.zone = TARGET_ZONE_HAND;
		if( eot_trigger_mode(player, card, event, 1-player,
							check_battlefield_for_special_card(player, card, player, 0, &this_test) ? RESOLVE_TRIGGER_MANDATORY : 0)
		  ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
				if( selected != -1 ){
					suspend_a_card(player, card, player, selected, 4);
				}
			}
		}
	}

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, "Select a nonland card to Suspend with four time counters.");
	this_test.type_flag = DOESNT_MATCH;
	if( player == AI ){
		this_test.cmc = 2;
		this_test.cmc_flag = F5_CMC_GREATER_THAN_VALUE;
	}
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL) ){
			return check_battlefield_for_special_card(player, card, player, 0, &this_test);
		}
	}

	if( event == EVENT_ACTIVATE){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				instance->targets[0].card = get_original_internal_card_id(player, selected);
				rfg_card_in_hand(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		suspend_a_card(instance->parent_controller, instance->parent_card, -1, instance->targets[0].card, 4);
	}

	return 0;
}

int card_keldon_megaliths(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	if( event == EVENT_CAN_ACTIVATE || ! IS_GAA_EVENT(event) ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance( player, card );

	comes_into_play_tapped(player, card, event);

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XR(2, 1), 0, &td, NULL) &&
			hand_count[player] < 1
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Damage creature or player\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				instance->number_of_targets = 0;
				add_state(player, card, STATE_TAPPED);
				generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XR(1, 1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
				if( spell_fizzled != 1 ){
					instance->info_slot = 1;
				}
				else{
					remove_state(player, card, STATE_TAPPED);
				}
		}

		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			if( valid_target(&td) ){
				damage_target0(player, card, 1);
			}
		}
	}

	return 0;
}

int card_knight_of_sursi(int player, int card, event_t event){
	/* Knight of Sursi	|3|W
	 * Creature - Human Knight 2/2
	 * Flying; flanking
	 * Suspend 3-|W */
	flanking(player, card, event);
	return suspend(player, card, event, 3, MANACOST_W(1), NULL, NULL);
}

int card_korlash_heir_to_blackblade(int player, int card, event_t event){
	/* Korlash, Heir to Blackblade	|2|B|B
	 * Legendary Creature - Zombie Warrior 100/100
	 * ~'s power and toughness are each equal to the number of |H1Swamps you control.
	 * |1|B: Regenerate Korlash.
	 * Grandeur - Discard another card named ~: Search your library for up to two |H2Swamp cards, put them onto the battlefield tapped, then shuffle your library. */

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( regeneration(player, card, event, MANACOST_XB(1, 1)) ){
			return 99;
		}
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) && is_id_in_hand(player, get_id(player, card)) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( land_can_be_played & LCBP_REGENERATION ){
			return regeneration(player, card, event, MANACOST_XB(1, 1));
		}
		else{
			if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
				discard_id_from_hand(player, get_id(player, card));
				get_card_instance(player, card)->info_slot = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		if( instance->info_slot > 0 ){
			card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
			parent->info_slot = 0;
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_SWAMP));
			this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_SWAMP);
			this_test.qty = 2;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &this_test);
		}
		else{
			if( can_regenerate(player, instance->parent_card) ){
				regenerate_target(player, instance->parent_card);
			}
		}
	}

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_SWAMP));
	}
	return 0;
}

static int target_for_lynessa(int player, int card, int mode){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance= get_card_instance(player, card);

	int i;
	int trg = -1;
	int par = -1;
	for(i=0; i<2; i++){
		if( i == 1-player || player == HUMAN ){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && is_what(i, count, TYPE_ARTIFACT) && ! is_what(i, count, TYPE_CREATURE) ){
						if( has_mana_for_activated_ability(player, card, MANACOST_XU(get_cmc(i, count), 2)) ){
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

int card_lynessa_zephyr_mage(int player, int card, event_t event){
	/*
	  Linessa, Zephyr Mage |3|U

	  Legendary Creature - Human Wizard 3/3

	  {X}{U}{U}, {T}: Return target creature with converted mana cost X to its owner's hand.

	  Grandeur - Discard another card named Linessa, Zephyr Mage: Target player returns a creature he or she controls to its owner's hand, then repeats this process for an artifact, an enchantment, and a land.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) && target_for_lynessa(player, card, 0) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL) && is_id_in_hand(player, get_id(player, card))){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int abilities[2] = {	generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) && target_for_lynessa(player, card, 0),
								generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL) && is_id_in_hand(player, get_id(player, card))
		};

		int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
							"Bounce a creature", abilities[0], 5,
							"Grandeur's Hyperbounce", abilities[1], 10);
		if( ! choice  ){
			spell_fizzled = 1;
			return 0;
		}

		if( choice == 1 ){
			instance->number_of_targets = 0;
			if( player == HUMAN ){
				pick_target(&td, "TARGET_CREATURE");
			}
			else{
				int result = target_for_lynessa(player, card, 1);
				if( result != -1 ){
					instance->targets[0].player = player;
					instance->targets[0].card = result;
					instance->number_of_targets = 1;
				}
			}
			if( spell_fizzled != 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST_XU(get_cmc(instance->targets[0].player, instance->targets[0].card), 2)) ){
					instance->info_slot = 1;
					tap_card(player, card);
				}
			}
		}

		if( choice == 2 ){
			generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td1, "TARGET_PLAYER");
			if( spell_fizzled != 1 ){
				discard_id_from_hand(player, get_id(player, card));
				instance->info_slot = 2;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && valid_target(&td) ){
			bounce_permanent( instance->targets[0].player, instance->targets[0].card );
		}
		if( instance->info_slot == 2 && valid_target(&td1) ){
			int i;
			for(i=0; i<4; i++){
				int type = 0;
				if( i < 3 ){
					type = 1<<i;
				}
				else{
					type = TYPE_ARTIFACT;
				}
				target_definition_t td2;
				default_target_definition(player, card, &td2, type );
				td2.allowed_controller = instance->targets[0].player;
				td2.preferred_controller = instance->targets[0].player;
				td2.illegal_abilities = 0;
				td2.allow_cancel = 0;
				td2.who_chooses = instance->targets[0].player;
				char buffer[500];
				int pos = 0;
				pos += scnprintf(buffer + pos, 500-pos, " Select a", buffer);
				if( i == 0 ){
					pos += scnprintf(buffer + pos, 500-pos, " Land ", buffer);
				}
				if( i == 1 ){
					pos += scnprintf(buffer + pos, 500-pos, " Creature ", buffer);
				}
				if( i == 2 ){
					pos += scnprintf(buffer + pos, 500-pos, "n Enchantment ", buffer);
				}
				if( i == 3 ){
					pos += scnprintf(buffer + pos, 500-pos, "n Artifact ", buffer);
				}
				pos += scnprintf(buffer + pos, 500-pos, "to bounce.", buffer);
				if( can_target(&td2) && select_target(player, card, &td2, buffer , &(instance->targets[1])) ){
					instance->number_of_targets = 1;
					bounce_permanent( instance->targets[1].player, instance->targets[1].card );
				}
			}
		}
	}

	return 0;
}

int card_llanowar_empath(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			scry(player, 2);
			show_deck( HUMAN, deck, 1, "Here's the first card of deck", 0, 0x7375B0 );
			if( is_what(-1, deck[0], TYPE_CREATURE)  ){
				add_card_to_hand(player, deck[0]);
				remove_card_from_deck(player, 0);
			}
		}
	}
	return 0;
}

int card_llanowar_mentor(int player, int card, event_t event){
	/* Llanowar Mentor	|G
	 * Creature - Elf Spellshaper 1/1
	 * |G, |T, Discard a card: Put a 1/1 |Sgreen Elf Druid creature token named Llanowar Elves onto the battlefield. It has "|T: Add |G to your mana pool." */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_LLANOWAR_ELVES);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD, MANACOST_G(1), 0, NULL, NULL);
}

int card_llanowar_reborn(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	graft(player, card, event, 1);

	return mana_producer(player, card, event);
}

int card_logic_knot(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_COUNTERSPELL | GS_X_SPELL, NULL, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 ){
			int amount = x_value;
			if( do_dialog(player, player, card, -1, -1, " Use Delve\n Pass", 0) == 0 ){
				int cg = count_graveyard(player);
				int max = 0;
				test_definition_t this_test2;
				new_default_test_definition(&this_test2, 0, "Select a card to exile.");
				while( max < cg ){
						if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test2) != -1 ){
							max++;
						}
						else{
							break;
						}
				}
				amount+=max;
			}
			instance->info_slot = amount;
			counterspell(player, card, event, NULL, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, instance->info_slot);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_lost_auramancers(int player, int card, event_t event){

	/* Lost Auramancers	|2|W|W
	 * Creature - Human Wizard 3/3
	 * Vanishing 3
	 * When ~ dies, if it had no time counters on it, you may search your library for an enchantment card and put it onto the battlefield. If you do, shuffle
	 * your library. */

	vanishing(player, card, event, 3);
	if( count_counters(player, card, COUNTER_TIME) == 0 && this_dies_trigger(player, card, event, RESOLVE_TRIGGER_OPTIONAL) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}
	return 0;
}

int card_lumithread_field(int player, int card, event_t event){

	boost_creature_type(player, card, event, -1, 0, 1, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

	if( event == EVENT_SHOULD_AI_PLAY ){
		return should_ai_play(player, card);
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	return morph(player, card, event, MANACOST_XW(1, 1));
}

int card_lymph_sliver(int player, int card, event_t event){
	if( event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && has_subtype(damage->damage_target_player, damage->damage_target_card, SUBTYPE_SLIVER) &&
				damage->info_slot > 0
			  ){
				damage->info_slot--;
			}
		}
	}
	return slivercycling(player, card, event);
}

int card_maelstrom_djinn(int player, int card, event_t event){

	/* Maelstrom Djinn	|7|U
	 * Creature - Djinn 5/6
	 * Flying
	 * Morph |2|U
	 * When ~ is turned face up, put two time counters on it and it gains vanishing. */

	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[1].player == 66 ){
		vanishing(player, card, event, 0);
	}
	if( event == EVENT_TURNED_FACE_UP ){
		instance->targets[1].player = 66;

		// Still face-down until the next EVENT_CHANGE_TYPE, so temporarily turn it face up long enough to get time counters in the normal slot
		int current_iid = instance->internal_card_id;
		instance->internal_card_id = get_internal_card_id_from_csv_id(CARD_ID_MAELSTROM_DJINN);
		add_counters(player, card, COUNTER_TIME, 2);
		instance->internal_card_id = current_iid;

		instance->targets[9].card = 66;
	}
	return morph(player, card, event, MANACOST_XU(2, 1));
}

int card_magus_of_the_abyss(int player, int card, event_t event)
{
  /* Magus of the Abyss	|3|B
   * Creature - Human Wizard 4/3
   * At the beginning of each player's upkeep, destroy target nonartifact creature that player controls of his or her choice. It can't be regenerated. */

  abyss_trigger(player, card, event);
  return 0;
}

// magus of the future --> future sight

int card_magus_of_the_vineyard(int player, int card, event_t event){

  vineyard_effect(player, card, event);

  return 0;
}

int card_marshaling_cry(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 1, 1, 0, SP_KEYWORD_VIGILANCE);
		if( get_flashback() ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND || event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
		return cycling(player, card, event, MANACOST_X(2));
	}

	return do_flashback(player, card, event, MANACOST_XW(3, 1));
}

int card_mesmeric_sliver(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "");
	  test.subtype = SUBTYPE_SLIVER;

	  // Mandatory, since it's harmless; and otherwise, it would be the Mesmeric Sliver's controller who chose whether to activate
	  if (new_specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, &test))
		fateseal(get_card_instance(player, card)->targets[1].player, 1);
	}

  return slivercycling(player, card, event);
}

int card_minions_murmurs(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = count_subtype(player, TYPE_CREATURE, -1);
		draw_cards(player, amount);
		lose_life(player, amount);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_molten_disaster(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! is_token(player, card) ){
			instance->info_slot = x_value;
			do_kicker(player, card, 0, 0, 0, 0, 1, 0);
		}
		if( kicked(player, card) ){
			state_untargettable(player, card, 1);
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			this_test.keyword_flag = 1;
			new_damage_all(player, card, 2, instance->info_slot, NDA_PLAYER_TOO, &this_test);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( !kicked(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			this_test.keyword_flag = 1;
			new_damage_all(player, card, 2, instance->info_slot, NDA_PLAYER_TOO, &this_test);
		}
		state_untargettable(player, card, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mystic_speculation(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = buyback(player, card, MANACOST_X(2));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		scry(player, 3);
		if( instance->info_slot == 1 ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_new_benalia(int player, int card, event_t event)
{
  /* New Benalia	""
   * Land
   * ~ enters the battlefield tapped.
   * When ~ enters the battlefield, scry 1.
   * |T: Add |W to your mana pool. */

  comes_into_play_tapped(player, card, event);

  if (comes_into_play(player, card, event))
	scry(player, 1);

  return mana_producer(player, card, event);
}

int card_nihilith(int player, int card, event_t event){
	/* Nihilith	|4|B|B
	 * Creature - Horror 4/4
	 * Fear
	 * Suspend 7-|1|B
	 * Whenever a card is put into an opponent's graveyard from anywhere, if ~ is suspended, you may remove a time counter from ~. */
	fear(player, card, event);
	return suspend(player, card, event, 7, MANACOST_XB(1, 1), NULL, NULL);
}

int card_nimbus_maze(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  if ((event == EVENT_CHANGE_TYPE && affect_me(player, card) && in_play(player, card))
	  || event == EVENT_RESOLVE_SPELL)	// mostly so the right land sound plays
	{
	  instance->info_slot = COLOR_TEST_COLORLESS;

	  int plains = get_hacked_subtype(player, card, SUBTYPE_PLAINS);
	  int island = get_hacked_subtype(player, card, SUBTYPE_ISLAND);

	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && is_what(player, c, TYPE_LAND))
		  {
			if (has_subtype(player, c, plains))
			  instance->info_slot |= COLOR_TEST_BLUE;

			if (has_subtype(player, c, island))
			  instance->info_slot |= COLOR_TEST_WHITE;

			if (instance->info_slot == (COLOR_TEST_COLORLESS | COLOR_TEST_BLUE | COLOR_TEST_WHITE))
			  break;
		  }
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  play_land_sound_effect_force_color(player, card, instance->info_slot);
	  return 0;	// so mana_producer() doesn't play the colorless sound
	}

  return mana_producer(player, card, event);
}

int card_oriss_samite_guardian(int player, int card, event_t event){
	/*
	  Oriss, Samite Guardian |1|W|W

	  Legendary Creature - Human Cleric 1/3

	  {T}: Prevent all damage that would be dealt to target creature this turn.

	  Grandeur - Discard another card named Oriss, Samite Guardian: Target player can't cast spells this turn,
	  and creatures that player controls can't attack this turn.
	*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0 );
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL) && is_id_in_hand(player, get_id(player, card))){
			return 1 ;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = instance->number_of_targets = 0;
		int abilities[2] = {	generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL),
								generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL) && is_id_in_hand(player, get_id(player, card))
		};

		int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
							"Prevent all damage to a creature", abilities[0], 10,
							"Grandeur's Chant", abilities[1], 5);
		if( ! choice  ){
			spell_fizzled = 1;
			return 0;
		}

		if( choice == 1 ){
			generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
			}
		}

		if( choice == 2 ){
			generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PLAYER");
			if( spell_fizzled != 1 ){
				discard_id_from_hand(player, get_id(player, card));
				instance->info_slot = 2;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && valid_target(&td) ){
			prevent_the_next_n_damage(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									  0, PREVENT_INFINITE, 0, 0);
		}

		if( instance->info_slot == 2 && valid_target(&td1) ){
			int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//8
			card_instance_t *added = get_card_instance(player, card_added);
			added->targets[3].player = instance->targets[0].player;
			added->targets[2].player = 8;
			added->targets[3].card = 1 << instance->targets[0].player;
		}
	}

	return 0;
}

int card_pact_of_negation(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! has_virtual_mana_multi(player, MANACOST_XU(3, 2)) ){
			ai_modifier -= 1500;
		}
		return counterspell(player, card, event, NULL, 0);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		create_pact_legacy(player, card, player, 3, 0, 2, 0, 0, 0);
		return counterspell(player, card, event, NULL, 0);
	}

	return 0;
}

// giant token --> vanilla

int card_pact_of_the_titan(int player, int card, event_t event){
	/* Pact of the Titan	|0
	 * Instant
	 * Put a 4/4 |Sred Giant creature token onto the battlefield.
	 * At the beginning of your next upkeep, pay |4|R. If you don't, you lose the game. */

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! has_virtual_mana_multi(player, MANACOST_XR(4, 1)) ){
			ai_modifier -= 1500;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		generate_token_by_id(player, card, CARD_ID_GIANT);
		create_pact_legacy(player, card, player, 4, 0, 0, 0, 1, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_patricians_scorn(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		if( get_stormcolor_count(player, COLOR_WHITE) ){
			null_casting_cost(player, card);
		}
	}

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_pyromancers_swath(int player, int card, event_t event){
	card_instance_t* damage = damage_being_dealt(event);
	if (damage && (damage->targets[3].player & TYPE_SPELL)){
		damage->info_slot += 2;
	}

	if (eot_trigger(player, card, event)){
		discard_all(player);
	}
	return global_enchantment(player, card, event);
}

int card_quagnoth(int player, int card, event_t event){//UNUSEDCARD
	cannot_be_countered(player, card, event);
	return 0;
}

int card_ramosia_revivalist(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	char msg[100] = "Select a Rebel permanent with CMC 5 or less.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_PERMANENT, msg);
	this_test.subtype = SUBTYPE_REBEL;
	this_test.cmc = 6;
	this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(6), 0, NULL, NULL) && new_special_count_grave(player, &this_test) > 0 ){
			return ! graveyard_has_shroud(2);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(6)) ){
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, instance->parent_card, player, selected, REANIMATE_DEFAULT);
		}
	}

	return 0;
}

int card_ravaging_riftwurm(int player, int card, event_t event){
	/* Ravaging Riftwurm	|1|G|G
	 * Creature - Wurm 6/6
	 * Kicker |4
	 * Vanishing 2
	 * If ~ was kicked, it enters the battlefield with three additional time counters on it. */

	vanishing(player, card, event, 2);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! is_token(player, card) ){
			if( do_kicker(player, card, MANACOST_X(4)) ){
				add_counters(player, card, COUNTER_TIME, 3);
			}
		}
	}
	return 0;
}

int card_reality_strobe(int player, int card, event_t event)
{
  /* Reality Strobe	|4|U|U
   * Sorcery
   * Return target permanent to its owner's hand. Exile ~ with three time counters on it.
   * Suspend 3-|2|U */

	if( event != EVENT_MODIFY_COST && ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	if (event == EVENT_RESOLVE_SPELL){
		if (valid_target(&td)){
			card_instance_t *instance = get_card_instance(player, card);
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			suspend_a_card(player, card, player, card, 3);
		}
		else
			kill_card(player, card, KILL_DESTROY);
	}

	return suspend(player, card, event, 3, MANACOST_XU(2,1), &td, "TARGET_PERMANENT");
}

int card_riddle_of_lightning(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[player];
			if( deck[0] != -1 ){
				scry(player, 3);
				show_deck( HUMAN, deck, 1, "Here's the first card of deck.", 0, 0x7375B0 );
				damage_creature_or_player(player, card, event, get_cmc_by_internal_id(deck[0]));
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

static const char* target_has_a_time_counter(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return count_counters(player, card, COUNTER_TIME) > 0 ? NULL : "time counters";
}

int card_rift_elemental(int player, int card, event_t event)
{
  /* Rift Elemental	|R
   * Creature - Elemental 1/1
   * |1|R, Remove a time counter from a permanent you control or suspended card you own: ~ gets +2/+0 until end of turn. */

  if (event == EVENT_POW_BOOST)
	return 2 * count_counters_by_counter_and_card_type(player, COUNTER_TIME, TYPE_ANY);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  base_target_definition(player, card, &td, TYPE_PERMANENT | TYPE_EFFECT);
  td.preferred_controller = player;
  td.allowed_controller = player;
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int)target_has_a_time_counter;

  card_instance_t *instance = get_card_instance(player, card);

  int rval = generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(1,1), 0, &td, "TARGET_CARD");
  if (event == EVENT_ACTIVATE && cancel != 1)
	{
	  instance->number_of_targets = 0;
	  remove_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_TIME);

	  if (is_what(instance->targets[0].player, instance->targets[0].card, TYPE_EFFECT))
		ai_modifier += 64;	// Remove time counters from suspended cards - good!
	  else
		ai_modifier -= 64;	// Remove time counters from cards with vanishing, or Infinite Hourglass, or such - bad.
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	pump_until_eot_merge_previous(player, card, instance->parent_controller, instance->parent_card, 2, 0);

  return rval;
}

int card_riftsweeper(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;

		instance->targets[0].player = player;
		if( count_rfg(player) > 0 ){
			if( count_rfg(1-player) > 0 ){
				pick_target(&td, "TARGET_PLAYER");
			}
		}
		else{
			instance->targets[0].player = 1-player;
		}
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		int result = new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_RFG, TUTOR_DECK, 1, AI_MAX_VALUE, &this_test);
		if( result != -1 ){
			shuffle(instance->targets[0].player);
		}
	}

	return 0;
}

int card_river_of_tears(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = COLOR_TEST_BLACK;
	}
	if( instance->info_slot == COLOR_TEST_BLUE && specific_cip(player, card, event, player, 2, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		instance->info_slot = COLOR_TEST_BLACK;
	}
	if( event == EVENT_CLEANUP ){
		instance->info_slot = COLOR_TEST_BLUE;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		// This event also handled above
		play_land_sound_effect_force_color(player, card, COLOR_TEST_BLUE|COLOR_TEST_BLACK);
		return 0;	// so mana_producer() doesn't play the black sound
	}
	return mana_producer(player, card, event);
}

int card_saltskitter(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		if( new_specific_cip(player, card, event, 2, 2, &this_test) ){
			remove_until_eot(player, card, player, card);
		}
	}
	return 0;
}

int card_scourge_of_kher_ridges(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST_XR(1, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int abilities[2] = {	has_mana_for_activated_ability(player, card, MANACOST_XR(1, 1)),
								has_mana_for_activated_ability(player, card, MANACOST_XR(5, 1))
		};
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.illegal_abilities = KEYWORD_FLYING;
		td.toughness_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;
		td.illegal_abilities = 0;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.required_abilities = KEYWORD_FLYING;
		td1.toughness_requirement = 6 | TARGET_PT_LESSER_OR_EQUAL;
		td1.illegal_abilities = 0;

		int priorities[2] = {0, 0};
		td.allowed_controller = player;
		priorities[0] -= 2*target_available(player, card, &td);
		priorities[1] -= 2*target_available(player, card, &td1);
		td.allowed_controller = 1-player;
		priorities[0] += 2*target_available(player, card, &td);
		priorities[1] += 2*target_available(player, card, &td1);
		td.allowed_controller = 2;

		int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
							"2 damage to nonflying creatures", abilities[0], priorities[0],
							"6 damage to flying creatures", abilities[1], priorities[1]);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XR((choice == 1 ? 1 : 5), 1)) ){
			instance->info_slot = choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		if( instance->info_slot == 1){
			this_test.keyword_flag = 1;
			APNAP(p, {new_damage_all(instance->parent_controller, instance->parent_card, p, 2, 0, &this_test);};);
		}
		if( instance->info_slot == 2){
			this_test.not_me = 1;
			APNAP(p, {new_damage_all(instance->parent_controller, instance->parent_card, p, 6, 0, &this_test);};);
		}
	}

	return 0;
}

int card_sehts_tiger(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		instance->info_slot = 1<<choose_a_color(player, get_deck_color(player, 1-player));
	}
	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}
	return flash(player, card, event);
}

int card_shah_of_naar_isle(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		set_special_flags(player, card, SF_ECHO_TO_PAY);
	}

	if( check_special_flags(player, card, SF_ECHO_TO_PAY) && current_turn == player && upkeep_trigger(player, card, event) ){
		remove_special_flags(player, card, SF_ECHO_TO_PAY);
		int kill = 1;
		int choice = do_dialog(player, player, card, -1, -1, " Pay Echo\n Pass", 0);
		if( choice == 0 ){
			draw_some_cards_if_you_want(player, card, 1-player, 3);
			kill = 0;
		}

		if( kill == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return 0;
}

int card_shapeshifters_marrow(int player, int card, event_t event){

	/* Shapeshifter's Marrow	|2|U|U
	 * Enchantment
	 * At the beginning of each opponent's upkeep, that player reveals the top card of his or her library. If it's a creature card, the player puts the card
	 * into his or her graveyard and ~ becomes a copy of that card. */

	cloning_card(player, card, event);

	upkeep_trigger_ability(player, card, event, 1-player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int iid = deck_ptr[1-player][0];
		if( iid != -1 ){
			reveal_card_iid(player, card, iid);
			if( is_what(-1, iid, TYPE_CREATURE) ){
				mill(1-player, 1);
				cloning_and_verify_legend(player, card, -1, iid);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_shimian_specter(int player, int card, event_t event){

	/* Shimian Specter	|2|B|B
	 * Creature - Specter 2/2
	 * Flying
	 * Whenever ~ deals combat damage to a player, that player reveals his or her hand. You choose a nonland card from it. Search that player's graveyard, hand,
	 * and library for all cards with the same name as that card and exile them. Then that player shuffles his or her library. */

	if ((hand_count[1-player] > 0 || event == EVENT_END_TRIGGER)
		&& damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_OPPONENT)){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a non-land card.");
		this_test.type_flag = 1;
		this_test.zone = TARGET_ZONE_HAND;
		int cc = check_battlefield_for_special_card(player, card, 1-player, 0, &this_test);
		int selected = new_select_a_card(player, 1-player, TUTOR_FROM_HAND, cc, AI_MAX_VALUE, -1, &this_test);
		if( selected != -1 ){
			int id = get_id(1-player, selected);
			rfg_card_in_hand(1-player, selected);
			lobotomy_effect(player, 1-player, id, 1);
		}
	}
	return 0;
}

int card_skirk_ridge_exhumer(int player, int card, event_t event){
	/* Skirk Ridge Exhumer	|1|B
	 * Creature - Zombie Spellshaper 1/1
	 * |B, |T, Discard a card: Put a 1/1 |Sblack Zombie Goblin creature token named Festering Goblin onto the battlefield. It has "When Festering Goblin dies, target creature gets -1/-1 until end of turn." */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_FESTERING_GOBLIN);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD, MANACOST_B(1), 0, NULL, NULL);
}

int card_slaughter_pact(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.illegal_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! has_virtual_mana_multi(player, MANACOST_XB(2, 1)) ){
			ai_modifier -= 1500;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			create_pact_legacy(player, card, player, MANACOST_XB(2, 1));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_CREATURE", 1, NULL);
}

int card_sliver_legion( int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && has_subtype(affected_card_controller, affected_card, SUBTYPE_SLIVER) &&
		! is_humiliated(player, card)
	  ){
		int total = count_subtype(2, TYPE_PERMANENT, SUBTYPE_SLIVER);
		if (total > 0 && has_subtype(player, card, SUBTYPE_SLIVER)){
			--total;
		}
		event_result += total;
	}
	return slivercycling(player, card, event);
}

int card_sliversmith(int player, int card, event_t event){
	/* Sliversmith	|2
	 * Artifact Creature - Spellshaper 1/1
	 * |1, |T, Discard a card: Put a 1/1 colorless Sliver artifact creature token named Metallic Sliver onto the battlefield. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_METALLIC_SLIVER);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD, MANACOST_X(1), 0, NULL, NULL);
}

int card_soultether_golem(int player, int card, event_t event){

	/* Soultether Golem	|2
	 * Artifact Creature - Golem 3/3
	 * Vanishing 1
	 * Whenever another creature enters the battlefield under your control, put a time counter on ~. */

	vanishing(player, card, event, 1);

	if( specific_cip(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		add_counter(player, card, COUNTER_TIME);
	}

	return 0;
}

int card_sparkspitter(int player, int card, event_t event){
	/* Sparkspitter	|2|R
	 * Creature - Elemental Spellshaper 1/3
	 * |R, |T, Discard a card: Put a 3/1 |Sred Elemental creature token named Spark Elemental onto the battlefield. It has trample, haste, and "At the beginning of the end step, sacrifice Spark Elemental." */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_SPARK_ELEMENTAL);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_DISCARD, MANACOST_R(1), 0, NULL, NULL);
}

static int spellweaver_volute_can_target(void){
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_INSTANT);
	this_test.type_flag = F1_NO_CREATURE;
	if( ! graveyard_has_shroud(0) && new_special_count_grave(0, &this_test) ){
		return 1;
	}
	if( ! graveyard_has_shroud(1) && new_special_count_grave(1, &this_test) ){
		return 1;
	}
	return 0;
}

static int select_target_for_spellweaver_volute(int player, int card, int cannot_cancel){
	card_instance_t *instance = get_card_instance(player, card);

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_INSTANT, "Select an instant or sorcery card to enchant.");
	this_test.type_flag = F1_NO_CREATURE;

	instance->number_of_targets = 0;
	instance->targets[0].player = player;
	if( ! graveyard_has_shroud(player) && new_special_count_grave(player, &this_test) ){
		if( ! graveyard_has_shroud(1-player) && new_special_count_grave(1-player, &this_test) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.zone = TARGET_ZONE_PLAYERS;
			td.illegal_abilities = 0;
			td.allow_cancel = 1-cannot_cancel;

			if( ! new_pick_target(&td, "TARGET_PLAYER", 0, 0) ){
				return 0;
			}
		}
	}
	else{
		instance->targets[0].player = 1-player;
	}

	int selected = new_select_a_card(player, instance->targets[0].player, TUTOR_FROM_GRAVE, cannot_cancel, AI_MAX_CMC, -1, &this_test);
	if( selected != -1 ){
		const int *grave = get_grave(instance->targets[0].player);
		instance->targets[1].player = selected;
		instance->targets[1].card = grave[selected];
		return 1;
	}
	return 0;
}

int card_spellweaver_volute(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[2].player == 66 ){
		const int *grave = get_grave(instance->targets[0].player);
		if( grave[instance->targets[1].player] != instance->targets[1].card ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_SHOULD_AI_PLAY ){
		return should_ai_play(player, card);
	}

	if( event == EVENT_CAN_CAST ){
		return spellweaver_volute_can_target();
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! select_target_for_spellweaver_volute(player, card, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		const int *grave = get_grave(instance->targets[0].player);
		int selected = instance->targets[1].player;
		if( instance->targets[1].card == grave[selected] ){
			instance->targets[2].player = 66;
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( specific_spell_played(player, card, event, player, 2, TYPE_SORCERY, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		copy_spell(player, cards_data[instance->targets[1].card].id);
		instance->targets[2].player = 0;
		rfg_card_from_grave(instance->targets[0].player, instance->targets[1].player);
		if( spellweaver_volute_can_target() ){
			select_target_for_spellweaver_volute(player, card, 1);
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_spin_into_myth(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
			fateseal(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_spirit_en_dal(int player, int card, event_t event){

	shadow(player, card, event);

	if( !(event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND || event == EVENT_RESOLVE_ACTIVATION_FROM_HAND) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND ){
		if( has_mana_multi(player, MANACOST_XW(1, 1)) && can_target(&td) && instance->info_slot != 1 && current_phase == PHASE_UPKEEP &&
			count_upkeeps(player) > 0
		  ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
		instance->number_of_targets = 0;
		charge_mana_multi(player, MANACOST_XW(1, 1));
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = 1;
			return 2;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_SHADOW);
		}
	}
	return 0;
}

static const char* target_has_2_spore_counters_and_can_activate(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (count_counters(player, card, COUNTER_SPORE) < 2)
	return "must have counters";
  if (!CAN_ACTIVATE0(player, card))
	return "cannot activate";
  return NULL;
}
int card_sporoloth_ancient(int player, int card, event_t event)
{
  /* Sporoloth Ancient	|3|G|G
   * Creature - Fungus 4/4
   * At the beginning of your upkeep, put a spore counter on ~.
   * Creatures you control have "Remove two spore counters from this creature: Put a 1/1 |Sgreen Saproling creature token onto the battlefield." */

  add_spore_counters(player, card, event);

  if (!IS_ACTIVATING(event))
	return 0;

  target_definition_t td;
  base_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = td.preferred_controller = player;
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int)target_has_2_spore_counters_and_can_activate;

  if (event == EVENT_CAN_ACTIVATE)
	return !is_humiliated(player, card) && can_target(&td);

  card_instance_t* instance = get_card_instance(player, card);
  if (event == EVENT_ACTIVATE && pick_next_target_noload(&td, "Select a creature you control with two spore counters."))
	{
	  instance->number_of_targets = 0;
	  if (charge_mana_for_activated_ability(instance->targets[0].player, instance->targets[0].card, MANACOST0))
		remove_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_SPORE, 2);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	generate_token_by_id(player, card, CARD_ID_SAPROLING);

  return 0;
}

int card_sprout_swarm(int player, int card, event_t event){
	/* Sprout Swarm	|1|G
	 * Instant
	 * Convoke
	 * Buyback |3
	 * Put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST){
		if( has_convoked_mana_extended(player, card, event, MANACOST_XG(1, 1)) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1){
			if( ! charge_convoked_mana_extended(player, card, event, MANACOST_XG(1, 1)) ){
				spell_fizzled = 1;
			}
		}
		if( ! is_token(player, card) ){
			int amount = 3-(2*count_cards_by_id(player, CARD_ID_MEMORY_CRYSTAL));
			if( has_convoked_mana_extended(player, card, event, MANACOST_X(amount)) ){
				int choice = do_dialog(player, player, card, -1, -1, " Play normally\n Buyback", 1);
				if( choice == 1 ){
					if( charge_convoked_mana_extended(player, card, event, MANACOST_X(amount)) ){
						instance->info_slot |= 2;
					}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_token_by_id(player, card, CARD_ID_SAPROLING);
		if( instance->info_slot & 2){
			bounce_permanent(player, card);

		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_storm_entity(int player, int card, event_t event){

	haste(player, card, event);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, get_storm_count());

	return 0;
}

int card_street_wraith(int player, int card, event_t event){

	/* Street Wraith	|3|B|B
	 * Creature - Wraith 3/4
	 * |H2Swampwalk
	 * Cycling-Pay 2 life. */

	if( ! is_unlocked(player, card, event, 14) ){ return 0; }

	return cycling_special_cost(player, card, event, 2);
}

int card_stronghold_rats(int player, int card, event_t event){
	shadow(player, card, event);
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		discard(player, 0, player);
		discard(1-player, 0, player);
	}
	return 0;
}

int card_summoners_pact(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! has_virtual_mana_multi(player, MANACOST_XG(2, 2)) ){
			ai_modifier -= 1500;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_GREEN;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, &this_test);
		create_pact_legacy(player, card, player, MANACOST_XG(2, 2));
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_sword_of_the_meek(int player, int card, event_t event)
{
	/* Sword of the Meek	|2
	 * Artifact - Equipment
	 * Equipped creature gets +1/+2.
	 * Equip |2
	 * Whenever a 1/1 creature enters the battlefield under your control, you may return ~ from your graveyard to the battlefield, then attach it to that
	 * creature. */

	return vanilla_equipment(player, card, event, 2, 1, 2, 0, 0);
}

int card_take_possession(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( leaves_play(player, card, event) ){
		if( instance->targets[3].card > -1){
			kill_card(player, instance->targets[3].card, KILL_REMOVE);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = 1-player;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		state_untargettable(player, card, 1);
		if( ! check_special_flags(player, card, SF_TARGETS_ALREADY_SET) ){
			instance->number_of_targets = 0;
			pick_target(&td, "TARGET_PERMANENT");
		}
		instance->info_slot = gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
		if( instance->info_slot > -1 ){
			add_status(player, instance->info_slot, STATUS_INVISIBLE_FX);
		}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			state_untargettable(player, card, 0);
			card_instance_t *leg = get_card_instance(player, instance->info_slot);
			if( ! check_for_guardian_beast_protection(leg->damage_target_player, leg->damage_target_card) ){
				attach_aura_to_target(player, card, event, leg->damage_target_player, leg->damage_target_card);
				instance->targets[3].player = player;
				instance->targets[3].card = instance->info_slot;
				set_special_flags(player, card, SF_CORRECTLY_RESOLVED);
			}
			else{
				kill_card(player, instance->info_slot, KILL_REMOVE);
				kill_card(player, card, KILL_SACRIFICE);
			}
	}

	else{
		return generic_stealing_aura(player, card, event, &td, "TARGET_PERMANENT");
	}

	return 0;
}

int card_tarmogoyf(int player, int card, event_t event){
	/* Tarmogoyf	|1|G
	 * Creature - Lhurgoyf 100/101
	 * ~'s power is equal to the number of card types among cards in all graveyards and its toughness is equal to that number plus 1. */

	if(event == EVENT_POWER || event == EVENT_TOUGHNESS){
		if(affect_me(player, card ) && ! is_humiliated(player, card) ){
			int i;
			int result = 0;
			int pump = 0;
			for(i=0; i<2; i++ ){
				int count = 0;
				const int *grave = get_grave(i);
				while( grave[count] != -1 ){
						;
						/* Whitelist allowable types.  get_type() deliberately reports interrupts as TYPE_INSTANT|TYPE_INTERRUPT, for example.  It may be made
						 * to do the same with other cases in the future.  It does deal correctly with flash permanents, instant-speed sorceries, enchantments
						 * vs. planewalkers, etc., though. */
						result |= get_type(-1, grave[count]) & (TYPE_LAND | TYPE_CREATURE | TYPE_ENCHANTMENT | TYPE_SORCERY | TYPE_INSTANT | TYPE_ARTIFACT | TARGET_TYPE_PLANESWALKER);
						if( has_subtype(-1, grave[count], SUBTYPE_TRIBAL) ){
							result |= TARGET_TYPE_TOKEN;	// this bit known to be free, as we didn't whitelist it above
						}
						count++;
				}
				pump = num_bits_set(result);
				if( pump >= 8 ){
					break;
				}
			}
			event_result += pump;
		}
	}
	return 0;
}

int card_tarox_bladewing(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	haste(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int amount = get_power(instance->parent_controller, instance->parent_card);
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, amount, amount);
	}

	return grandeur(player, card, event);
}

int card_tolaria_west(int player, int card, event_t event)
{
  /* Tolaria West	""
   * Land
   * ~ enters the battlefield tapped.
   * |T: Add |U to your mana pool.
   * Transmute |1|U|U */

  if (IS_ACTIVATING_FROM_HAND(event))
	return transmute(player, card, event, MANACOST_XU(1,2), 0);

  comes_into_play_tapped(player, card, event);

  if (in_play(player, card))
	return mana_producer(player, card, event);

  return 0;
}

int card_tombstalker(int player, int card, event_t event){
	return delve(player, card, event);
}

int card_unblinking_bleb(int player, int card, event_t event){

	if( event == EVENT_TURNED_FACE_UP ){
		scry(player, 2);
	}

	return morph(player, card, event, MANACOST_XU(2, 1));
}

int card_utopia_mycon(int player, int card, event_t event)
{
  /* Utopia Mycon	|G
   * Creature - Fungus 0/2
   * At the beginning of your upkeep, put a spore counter on ~.
   * Remove three spore counters from ~: Put a 1/1 |Sgreen Saproling creature token onto the battlefield.
   * Sacrifice a Saproling: Add one mana of any color to your mana pool. */

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
#define DECL_TEST(test)	test_definition_t test;	\
						new_default_test_definition(&test, TYPE_PERMANENT, "Select Saproling to sacrifice.");	\
						test.subtype = SUBTYPE_SAPROLING
	  DECL_TEST(test);

	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_SAPROLING
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1,
						"Produce mana", can_produce_mana(player, card) && new_can_sacrifice_as_cost(player, card, &test), paying_mana() ? 2 : -1,
						"Generate a Saproling", (!paying_mana() && can_make_saproling_from_fungus(player, card) && can_use_activated_abilities(player, card)
												 && has_mana_for_activated_ability(player, card, MANACOST_X(0))), 1);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_MANA:
			  ai_modifier -= 24;
			  if (new_sacrifice(player, card, player, 0, &test))
				{
				  get_card_instance(player, card)->number_of_targets = 0;
				  FORCE(produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, 1));
				  tapped_for_mana_color = -2;
				}
			  break;
			case CHOICE_SAPROLING:
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(0)))
				saproling_from_fungus(player, card);
			  break;
		  }
	  else	// event == EVENT_RESOLVE_ACTIVATION
		if (choice == CHOICE_SAPROLING)
		  generate_token_by_id(player, card, CARD_ID_SAPROLING);
	}

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card))
	{
	  DECL_TEST(test);
	  if (new_can_sacrifice_as_cost(player, card, &test))
		declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, count_subtype(player, TYPE_PERMANENT, SUBTYPE_SAPROLING));
	}

  add_spore_counters(player, card, event);
  return 0;
}

int card_vedalken_aethermage(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND ){
		return cycling(player, card, event, MANACOST_X(3));
	}

	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_WIZARD;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, &this_test);
	}

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.required_subtype = SUBTYPE_SLIVER;
		td.allow_cancel = 0;

		instance->number_of_targets = 0;

		if( can_target(&td) && pick_next_target_noload(&td, "Select target Sliver.") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return flash(player, card, event);
}

int card_veilstone_amulet(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0)  ){
		pump_subtype_until_eot(player, card, player, -1, 0, 0, 0, SP_KEYWORD_HEXPROOF);
	}
	return 0;
}

int card_venser_shaper_savant(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_CAST ){
		if( counterspell(player, card, event, NULL, 0) ){
			instance->info_slot = 1;
			return 99;
		}
		else{
			instance->info_slot = 0;
			return 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( instance->info_slot == 1 ){
			counterspell(player, card, event, NULL, 0);
		}
	}

	if( comes_into_play(player, card, event) ){
		if( instance->info_slot == 1 ){
			if( counterspell_validate(player, card, NULL, 0) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
		else{
			 target_definition_t td;
			 default_target_definition(player, card, &td, TYPE_PERMANENT );

			 pick_target(&td, "TARGET_PERMANENT");
			 if( valid_target(&td) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			 }
		}
	}
	return 0;
}

int card_virulent_sliver(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card && damage->info_slot > 0){
				if( damage->damage_target_card == -1 && has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_SLIVER) ){
					if( damage->info_slot > 0 || get_card_instance(damage->damage_source_player, damage->damage_source_card)->targets[16].player > 0 ){
						int pos = instance->info_slot;
						if( pos < 10 ){
							instance->targets[pos].player = damage->damage_target_player;
							instance->info_slot++;
						}
					}
				}
			}
		}


		if(trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player && instance->info_slot > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int i;
					for(i=0; i<instance->info_slot; i++){
						poison_counters[instance->targets[i].player]++;
					}
					instance->info_slot = 0;
			}
		}
	}
	return 0;
}

int card_wrap_in_vigor(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		int i = active_cards_count[player]-1;;
		while( i > -1 ){
				if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) ){
					if( can_be_regenerated(player, i) ){
						regenerate_target(player, i);
					}
				}
				i--;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_REGENERATION, NULL, NULL, 0, NULL);
}

