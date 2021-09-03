#include "manalink.h"

// Functions
static int goyf(int player, int card, event_t event, test_definition_t *this_test){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		int amount = new_special_count_grave(0, this_test) + new_special_count_grave(1, this_test);
		event_result+=amount;
	}

	return 0;
}

int can_pay_flashback(int player, int iid, event_t event, int colorless, int black, int blue, int green, int red, int white){
	int cless = get_updated_casting_cost(player, -1, iid, event, colorless);
	cless-=(2*count_cards_by_id(player, CARD_ID_CATALYST_STONE));
	cless+=(2*count_cards_by_id(1-player, CARD_ID_CATALYST_STONE));
	if( cless < 0 ){
		cless = 0;
	}
	if( has_mana_multi(player, cless, black, blue, green, red, white) ){
		return 1;
	}
	return 0;
}

int pay_flashback(int player, int iid, event_t event, int colorless, int black, int blue, int green, int red, int white){
	int orig_colorless = colorless;
	int cless = get_updated_casting_cost(player, -1, iid, event, colorless);
	cless-=(2*count_cards_by_id(player, CARD_ID_CATALYST_STONE));
	cless+=(2*count_cards_by_id(1-player, CARD_ID_CATALYST_STONE));
	if( cless < 0 ){
		cless = 0;
	}
	charge_mana_multi(player, cless, black, blue, green, red, white);
	if( orig_colorless == -1 ){
		charge_mana(player, COLOR_COLORLESS, -1);
	}
	if( spell_fizzled != 1 ){
		return GAPAID_REMOVE;
	}
	return 0;
}

int do_flashback(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_GRAVEYARD_ABILITY && can_pay_flashback(player, instance->internal_card_id, event, colorless, black, blue, green, red, white) ){
		if( is_what(player, card, TYPE_INSTANT | TYPE_INTERRUPT) ||
			(is_what(player, card, TYPE_SORCERY) && can_sorcery_be_played(player, event))
		  ){
			card_data_t* card_d = &cards_data[ instance->internal_card_id ];
			int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
			return ptFunction(player, card, EVENT_CAN_CAST );
		}
	}
	else if( event == EVENT_PAY_FLASHBACK_COSTS ){
			return pay_flashback(player, instance->internal_card_id, event, colorless, black, blue, green, red, white);
	}
	return 0;
}

static int od_land(int player, int card, event_t event, int color1, int color2){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && has_mana(player, COLOR_COLORLESS, 1) &&
		instance->targets[1].player != 66
	  ){
		return can_produce_mana(player, card);
	}

	if(event == EVENT_ACTIVATE ){
		instance->targets[1].player = 66;
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			produce_mana_tapped2(player, card, color1, 1, color2, 1);
		}
		instance->targets[1].player = 0;
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( ! is_tapped(player, card) && has_mana(player, COLOR_COLORLESS, 1) && instance->targets[1].player != 66 && can_produce_mana(player, card) ){
			declare_mana_available(player, color1, 1);
			declare_mana_available(player, color2, 1);
		}
	}

	return 0;
}

static int od_egg(int player, int card, event_t event, int color1, int color2){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		instance->targets[1].player != 66 && has_mana(player, COLOR_COLORLESS, 2)
	  ){
		if( can_produce_mana(player, card) ){
			return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->targets[1].player = 66;
		charge_mana(player, COLOR_COLORLESS, 2);
		if( spell_fizzled != 1 ){
			produce_mana_tapped2(player, card, color1, 1, color2, 1);
			kill_card(player, card, KILL_SACRIFICE);
		}
		instance->targets[1].player = 0;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( ! is_tapped(player, card) && has_mana(player, COLOR_COLORLESS, 2) && instance->targets[1].player != 66 && !is_animated_and_sick(player, card)
			&& can_produce_mana(player, card) && can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			declare_mana_available(player, color1, 1);
			declare_mana_available(player, color2, 1);
		}
	}

	return 0;
}


// Cards

int card_aboshan_cephalid_emperor(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.required_subtype = SUBTYPE_CEPHALID;
	td1.illegal_abilities = 0;
	td1.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 3, 0, 0, 0) ){
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && can_target(&td1) && can_target(&td) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && can_target(&td1) && can_target(&td) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 3, 0, 0, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Tap target permanent\n Tap all nonflying creatures\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 0, 0, 3*choice, 0, 0, 0) ){
				if( choice == 0 ){
					if( pick_target(&td1, "TARGET_PERMANENT") ){
						instance->number_of_targets = 1;
						if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_CEPHALID) ){
							tap_card(instance->targets[0].player, instance->targets[0].card);
							if( pick_target(&td, "TARGET_PERMANENT") ){
								instance->number_of_targets = 1;
								instance->info_slot = 66+choice;
							}
						}
					}
				}
				else if( choice == 1 ){
						instance->info_slot = 66+choice;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == 67){
			test_definition_t this_test2;
			default_test_definition(&this_test2, TYPE_CREATURE);
			this_test2.keyword = KEYWORD_FLYING;
			this_test2.keyword_flag = 1;
			new_manipulate_all(player, card, 2, &this_test2, ACT_TAP);
		}
	}

	return 0;
}

int card_aegis_of_honor(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_DAMAGE_PREVENTION+GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_DAMAGE");
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			instance->number_of_targets = 0;
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_DAMAGE") ){
				card_instance_t* dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				card_instance_t* src = get_card_instance(dmg->damage_source_player, dmg->damage_source_card);
				int src_id = src->internal_card_id == -1 ? (int)src->original_internal_card_id : src->internal_card_id;
				if (!is_what(-1, src_id, TYPE_SPELL)){
					spell_fizzled = 1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
		if (valid_target(&td)){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			damage_player(dmg->damage_source_player, dmg->info_slot, dmg->damage_source_player, dmg->damage_source_card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return global_enchantment(player, card, event);
}

int card_aether_burst(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 int number = 1 + count_graveyard_by_id(2, CARD_ID_ETHER_BURST);
			 int targ = 0;
			 while( can_target(&td) && targ < number ){
					if( new_pick_target(&td, "TARGET_CREATURE", targ, 0) ){
						card_instance_t *bursted = get_card_instance(instance->targets[targ].player, instance->targets[targ].card);
						bursted->state |= STATE_CANNOT_TARGET;
						targ++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<targ; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			if( targ < 1 ){
				spell_fizzled = 1;
			}
			else{
				instance->info_slot = targ;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			int i;
			for(i=0;i<instance->info_slot; i++){
				if( validate_target(player, card, &td, i) ){
					bounce_permanent(instance->targets[i].player, instance->targets[i].card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_ancestral_tribute(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			gain_life(player, count_graveyard(player)*2);
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 9, 0, 0, 0, 0, 3);
}

int card_animal_boneyard(int player, int card, event_t event){

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
			instance->targets[1] = instance->targets[0];
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		card_instance_t *trg = get_card_instance(t_player, t_card);

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_CREATURE, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}

		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_CREATURE, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			gain_life(player, trg->targets[2].card);
		}
	}

	return 0;
}

int card_atogatog(int player, int card, event_t event){

	/* Atogatog	|W|U|B|R|G
	 * Legendary Creature - Atog 5/5
	 * Sacrifice an Atog creature: ~ gets +X/+X until end of turn, where X is the sacrificed creature's power. */

	check_legend_rule(player, card, event);

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST){
		/* Greatly simplifying.  Assumes that the other atogs can't pump themselves first, and that if Atogatog's activated ability's cost has increased from 0,
		 * it only needs to be paid once. */

		if (!CAN_ACTIVATE0(player, card)){
			return 0;
		}

		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "");
		test.subtype = SUBTYPE_ATOG;
		test.subtype_flag = MATCH;
		test.power = 0;
		test.power_flag = F5_POWER_GREATER_THAN_VALUE;
		test.not_me = 1;

		if (!new_can_sacrifice_as_cost(player, 1, &test)){
			return 0;
		}

		return check_battlefield_for_special_card(player, card, player, CBFSC_GET_TOTAL_POW, &test);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		int p = instance->parent_controller, c = instance->parent_card;
		int pow = instance->targets[1].player;
		if (in_play(p, c)){
			pump_until_eot(p, c, p, c, pow, pow);
		}
	}

	return altar_extended(player, card, event, 0, TYPE_CREATURE, 0, SUBTYPE_ATOG, 0, 0, 0, 0, 0, -1, 0);
}

int card_auramancer(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		if( count_graveyard_by_type(player, TYPE_ENCHANTMENT) > 0 ){
			global_tutor(player, player, 2, TUTOR_HAND, 0, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}

int card_aven_shrine(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int id = get_id(instance->targets[1].player, instance->targets[1].card);
		int amount = count_graveyard_by_id(player, id)+count_graveyard_by_id(1-player, id);
		gain_life(instance->targets[1].player, amount);
	}

	return global_enchantment(player, card, event);
}

int card_aven_fisher(int player, int card, event_t event){
	/*
	  Aven Fisher |3|U
	  Creature - Bird Soldier 2/2
	  Flying (This creature can't be blocked except by creatures with flying or reach.)
	  When Aven Fisher dies, you may draw a card.
	*/
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_DUH) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_aven_flock(int player, int card, event_t event){
	/*
	  Aven Flock |4|W
	  Creature - Bird Soldier 2/3
	  Flying (This creature can't be blocked except by creatures with flying or reach.)
	  {W}: Aven Flock gets +0/+1 until end of turn.
	*/
	return generic_shade(player, card, event, 0, MANACOST_W(1), 0, 1, 0, 0);
}

int card_balancing_act(int player, int card, event_t event){
	if ( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		int my_p = count_subtype(player, TYPE_PERMANENT, -1);
		int his_p = count_subtype(1-player, TYPE_PERMANENT, -1);
		if( my_p != his_p ){
			if( my_p > his_p ){
				impose_sacrifice(player, card, player, my_p - his_p, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			else{
				impose_sacrifice(player, card, 1-player, his_p - my_p, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
		my_p = hand_count[player];
		his_p = hand_count[1-player];
		if( my_p != his_p ){
			if( my_p > his_p ){
				new_multidiscard(player, my_p - his_p, 0, player);
			}
			else{
				new_multidiscard(1-player, his_p - my_p, 0, player);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_barbarian_ring(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( ! paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 2, 0) &&
			has_threshold(player) && can_target(&td)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Generate mana\n Sac, Deal 2\n Cancel", 1);
		}
		if( choice == 0 ){
			damage_player(player, 1, player, card);
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
				if( spell_fizzled != 1  ){
					if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
						instance->info_slot = 1;
						kill_card(player, card, KILL_SACRIFICE);
					}
					else{
						untap_card_no_event(player, card);
					}
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			damage_creature_or_player(player, card, event, 2);
		}
	}
	else{
		return mana_producer(player, card, event);
	}
	return 0;
}

int card_bash_to_bits(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_ARTIFACT");
	}
	else if( event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE );
			}
			else{
				kill_card(player, card, KILL_DESTROY );
			}
  }
	else if( event == EVENT_GRAVEYARD_ABILITY ){
			if( has_mana_multi(player, 4, 0, 0, 0, 2, 0) ){
				return can_target(&td);
			}
	}
	else if( event == EVENT_PAY_FLASHBACK_COSTS ){
			charge_mana_multi(player, 4, 0, 0, 0, 2, 0);
			if( spell_fizzled != 1){
				return 1;
			}
	}
	return 0;
}

int card_battle_of_wits(int player, int card, event_t event)
{
  /* Battle of Wits	|3|U|U
   * Enchantment
   * At the beginning of your upkeep, if you have 200 or more cards in your library, you win the game. */

  if (trigger_condition == TRIGGER_UPKEEP && player == current_turn && affect_me(player, card)
	  && count_deck(player) >= 200 && upkeep_trigger(player, card, event))
	lose_the_game(1-player);

  if (event == EVENT_SHOULD_AI_PLAY && count_deck(player) >= 200)
	lose_the_game(1-player);

  return global_enchantment(player, card, event);
}

int card_battle_strain(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS){
	   int i;
	   for(i = 0; i < active_cards_count[1-current_turn]; i++){
		   if( in_play(1-current_turn, i) && is_what(1-current_turn, i, TYPE_CREATURE) && blocking(1-current_turn, i, event) ){
			   damage_player(1-current_turn, 1, player, card);
		   }
	   }
	}

	return global_enchantment(player, card, event);
}

int card_bearscape(int player, int card, event_t event){
	/* Bearscape	|1|G|G
	 * Enchantment
	 * |1|G, Exile two cards from your graveyard: Put a 2/2 |Sgreen Bear creature token onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && count_graveyard(player) > 1 ){
		return generic_activated_ability(player, card, event, 0, MANACOST_XG(1, 1), 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XG(1, 1)) ){
			char msg[100] = "Select a card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
				new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(instance->parent_controller, instance->parent_card, CARD_ID_BEAR, &token);
		token.pow = token.tou = 2;
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);
	}

	return global_enchantment(player, card, event);
}

int card_beast_attack(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_BEAST, &token);
			token.pow = 4;
			token.tou = 4;
			generate_token(&token);
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 2, 0, 0, 3, 0, 0);
}

int card_beloved_chaplain(int player, int card, event_t event){
	protection_from_creatures(player, card, event);
	return 0;
}

int card_bomb_squad(int player, int card, event_t event)
{
  /* Bomb Squad	|3|R
   * Creature - Dwarf 1/1
   * |T: Put a fuse counter on target creature.
   * At the beginning of your upkeep, put a fuse counter on each creature with a fuse counter on it.
   * Whenever a creature has four or more fuse counters on it, remove all fuse counters from it and destroy it. That creature deals 4 damage to its
   * controller. */

  int p, c, counters;

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	for (p = 0; p <= 1; ++p)
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (in_play(p, c) && is_what(p, c, TYPE_CREATURE) && count_counters(p, c, COUNTER_FUSE))
		  add_counter(p, c, COUNTER_FUSE);

  if (event == EVENT_STATIC_EFFECTS || event == EVENT_SHOULD_AI_PLAY)
	for (p = 0; p <= 1; ++p)
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (in_play(p, c) && is_what(p, c, TYPE_CREATURE) && (counters = count_counters(p, c, COUNTER_FUSE)))
		  {
			if (counters >= 4)
			  {
				remove_all_counters(p, c, COUNTER_FUSE);
				damage_player(p, 4, p, c);
				kill_card(p, c, KILL_DESTROY);
			  }
			else if (event == EVENT_SHOULD_AI_PLAY)
			  ai_modifier += (p == AI ? -16 : 16) * counters;
		  }

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_FUSE);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_braids_cabal_minion(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		impose_sacrifice(player, card, current_turn, 1, TYPE_CREATURE|TYPE_ARTIFACT|TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_burning_sands(int player, int card, event_t event)
{
  /* Burning Sands	|3|R|R	0x200d734
   * Enchantment
   * Whenever a creature dies, that creature's controller sacrifices a land. */

  card_instance_t* aff;
  if (event == EVENT_GRAVEYARD_FROM_PLAY
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && (aff = in_play(affected_card_controller, affected_card))
	  && (aff->kill_code > 0 && aff->kill_code < KILL_REMOVE)
	  && !is_humiliated(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->targets[affected_card_controller].player < 0)
		inst->targets[affected_card_controller].player = 0;
	  inst->targets[affected_card_controller].player++;
	}

  if (trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && affect_me(player, card) && player == reason_for_trigger_controller)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int dead[2] = { inst->targets[0].player, inst->targets[1].player };
	  if (dead[0] > 0 || dead[1] > 0)
		{
		  if (event == EVENT_TRIGGER)
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		  if (event == EVENT_RESOLVE_TRIGGER)
			{
			  inst->targets[0].player = inst->targets[1].player = 0;
			  /* Burning Sands's controller should be able to put the triggers on the stack in whatever order he likes, but there's no interface for it.  So
			   * just assume he orders them so that his opponent always sacrifices first. */
			  if (dead[1-player] > 0)
				impose_sacrifice(player, card, 1-player, dead[1-player], TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			  if (dead[player] > 0)
				impose_sacrifice(player, card, player, dead[player], TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		  if (event == EVENT_END_TRIGGER)	// Assume the trigger was countered.  (Though it should need a separate counter for each dead creature.)
			inst->targets[0].player = inst->targets[1].player = 0;
		}
	}

  return global_enchantment(player, card, event);
}

int card_cabal_patriarch(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 2, 1, 0, 0, 0, 0) ){
		if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) || count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
				choice = do_dialog(player, player, card, -1, -1, " Sac a creature\n Exile a creature card\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana_for_activated_ability(player, card, 2, 1, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				int good = 0;
				if( choice == 0 ){
					if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
						good = 1;
					}
				}
				if( choice == 1 ){
					char msg[100] = "Select a creature card to exile.";
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_CREATURE, msg);
					if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
						good = 1;
					}
				}
				if( good == 1 ){
					if( pick_target(&td, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
					}
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -2, -2);
		}
	}

	return 0;
}

int card_cabal_pit(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_B(2)) && can_use_activated_abilities(player, card) &&
			can_target(&td) && has_threshold(player) && can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac & weaken creature\n Cancel", ai_choice);
		}

		if( choice == 0){
			damage_player(player, 1, player, card);
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, MANACOST_B(1));
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
				untap_card_no_event(player, card);
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				if( valid_target(&td) ){
					pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -2, -2);
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

int card_cabal_shrine(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int id = get_id(instance->targets[1].player, instance->targets[1].card);
		int amount = count_graveyard_by_id(player, id)+count_graveyard_by_id(1-player, id);
		new_multidiscard(instance->targets[1].player, amount, 0, player);
	}

	return global_enchantment(player, card, event);
}

int card_call_of_the_herd(int player, int card, event_t event){
	/* Call of the Herd	|2|G
	 * Sorcery
	 * Put a 3/3 |Sgreen Elephant creature token onto the battlefield.
	 * Flashback |3|G */

	if(event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, 3,0, 0, 1, 0, 0) ){
		if( can_sorcery_be_played(player, event) ){
			return 1;
		}
	}
	else if( event == EVENT_PAY_FLASHBACK_COSTS ){
			charge_mana_multi(player, 3, 0, 0, 1, 0, 0);
			if( spell_fizzled != 1){
				return 1;
			}
	}
	else if( event == EVENT_CAN_CAST ){
			return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			generate_token_by_id(player, card, CARD_ID_ELEPHANT);
			if( get_flashback() ){
			   kill_card(player, card, KILL_REMOVE);
			}
			else{
				 kill_card(player, card, KILL_DESTROY);
			}
	}
	return 0;
}

int card_cantivore(int player, int card, event_t event){
	/* Cantivore	|1|W|W
	 * Creature - Lhurgoyf 100/100
	 * Vigilance
	 * ~'s power and toughness are each equal to the number of enchantment cards in all graveyards. */

	vigilance(player, card, event);

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_ENCHANTMENT);

	return goyf(player, card, event, &this_test);
}

// catalyst stone --> vanilla

int card_careful_study(int player, int card, event_t event){
	/*
	  Careful Study |U
	  Sorcery
	  Draw two cards, then discard two cards.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		multidiscard(player, 2, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_caustic_tar(int player, int card, event_t event){

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
			instance->targets[1] = instance->targets[0];
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		card_instance_t *trg = get_card_instance(t_player, t_card);

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
			if( validate_target(t_player, t_card, &td1, 0) ){
				lose_life(trg->targets[0].player, 3);
			}
		}
	}

	return 0;
}

int card_centaur_garden(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 0, 0, 0, 2, 0, 0) && can_use_activated_abilities(player, card) && can_target(&td) &&
			has_threshold(player) && can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac & pump creature\n Cancel", ai_choice);
		}

		if( choice == 0){
			damage_player(player, 1, player, card);
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
				untap_card_no_event(player, card);
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				if( valid_target(&td) ){
					pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 3, 3);
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

int card_cephalid_broker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			draw_cards(instance->targets[0].player, 2);
			new_multidiscard(instance->targets[0].player, 2, 0, player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_cephalid_coliseum(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.preferred_controller = player;
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t* instance = get_card_instance(player, card);

#define CAN_SAC	(!is_tapped(player, card) && has_mana_for_activated_ability(player, card, MANACOST_U(2))	\
				 && can_use_activated_abilities(player, card) && can_target(&td) && !is_animated_and_sick(player, card) && has_threshold(player))

	if (event == EVENT_CAN_ACTIVATE){
		int rval = mana_producer(player, card, event);
		if (rval){
			return rval;
		}
		return CAN_SAC;
	}
	else if (event == EVENT_ACTIVATE){
		int choice = 0;
		if (!paying_mana() && CAN_SAC){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac & draw / discard\n Cancel", 1);
		}

		if (choice == 0){
			damage_player(player, 1, player, card);
			return mana_producer(player, card, event);
		}

		if (choice == 1){
			tap_card(player, card);
			if (charge_mana_for_activated_ability(player, card, MANACOST_U(1)) && pick_target(&td, "TARGET_PLAYER")){
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
				untap_card_no_event(player, card);
			}
		}
	}
	else if (event == EVENT_RESOLVE_ACTIVATION){
			if (instance->info_slot > 0 && valid_target(&td)){
				draw_cards(instance->targets[0].player, 3);
				new_multidiscard(instance->targets[0].player, 3, 0, player);
			}
	}
	else{
		return mana_producer(player, card, event);
	}

	return 0;
#undef CAN_SAC
}

int card_cephalid_looter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			draw_cards(instance->targets[0].player, 1);
			new_multidiscard(instance->targets[0].player, 1, 0, player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_cephalid_retainer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities |= KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 0, 0, 2, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_cephalid_shrine(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int id = get_id(instance->targets[1].player, instance->targets[1].card);
		int amount = count_graveyard_by_id(player, id)+count_graveyard_by_id(1-player, id);
		charge_mana(instance->targets[1].player, COLOR_COLORLESS, amount);
		if( spell_fizzled == 1 ){
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
		}
	}

	return global_enchantment(player, card, event);
}

int card_chamber_of_manipulation(int player, int card, event_t event){

	/* Chamber of Manipulation	|2|U|U
	 * Enchantment - Aura
	 * Enchant land
	 * Enchanted land has "|T, Discard a card: Gain control of target creature until end of turn." */

	card_instance_t* instance;

	if (IS_CASTING(player, card, event)){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.preferred_controller = player;
		td.allowed_controller = player;

		instance = get_card_instance(player, card);

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
	}

	if( IS_ACTIVATING(event) && (instance = in_play(player, card)) && instance->damage_target_player != -1 ){

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.allowed_controller = 1-player;
		td1.preferred_controller = 1-player;

		int rval = attachment_granting_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_DISCARD, MANACOST0, 0, &td1, "TARGET_CREATURE");

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if (validate_arbitrary_target(&td1, instance->targets[0].player, instance->targets[0].card)){
				gain_control_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			}
		}

		return rval;
	}

	return 0;
}

int card_chance_encounter(int player, int card, event_t event){

	/* Chance Encounter	|2|R|R
	 * Enchantment
	 * Whenever you win a coin flip, put a luck counter on ~.
	 * At the beginning of your upkeep, if ~ has ten or more luck counters on it, you win the game. */

	// Coin-flip trigger is in flip_a_coin()

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		if( count_counters(player, card, COUNTER_LUCK) >= 10 ){
			lose_the_game(1-player);
		}
	}

	if (event == EVENT_SHOULD_AI_PLAY){
		int counters = count_counters(player, card, COUNTER_LUCK);
		if (player == AI){
			ai_modifier += counters * counters * counters;
		} else if (counters >= 7){
			ai_modifier -= counters * counters * counters;
		}
		if (counters >= 10){
			lose_the_game(1-player);
		}
	}

	return global_enchantment(player, card, event);
}

int card_charmed_pendant(int player, int card, event_t event){

	int *deck = deck_ptr[player];

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		deck[0] != -1
	  ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		card_ptr_t* c = cards_ptr[ cards_data[deck[0]].id ];
		// Unimplemented: Ruling 11/1/2005: If you flip over a hybrid card, you get one of either color mana, not both, for each mana symbol.
		produce_mana_tapped_multi(player, card, 0, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white);
		mill(player, 1);
	}

	return 0;
}

int card_chatter_of_squirrel(int player, int card, event_t event){
	/* Chatter of the Squirrel	|G
	 * Sorcery
	 * Put a 1/1 |Sgreen Squirrel creature token onto the battlefield.
	 * Flashback |1|G */

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			generate_token_by_id(player, card, CARD_ID_SQUIRREL);
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 1, 0, 0, 1, 0, 0);
}

int card_childhood_horror(int player, int card, event_t event){
	if( has_threshold(player)  ){
		modify_pt_and_abilities(player, card, event, 2, 2, 0);
		cannot_block(player, card, event);
	}
	return 0;
}

int card_chlorophant(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_DUH);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int cnters = 1;
		if( has_threshold(player) ){
			cnters++;
		}
		add_1_1_counters(player, card, cnters);
	}

	return 0;
}

int card_coffin_purge(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && (count_graveyard(player) > 0 || count_graveyard(1-player) > 0) ){
		return ! graveyard_has_shroud(2);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->targets[0].player = 1-player;
			if( count_graveyard(1-player) > 0 ){
				if( count_graveyard(player) > 0 ){
					if( ! pick_target(&td, "TARGET_PLAYER") ){
						return 0;
					}
				}
			}
			else{
				instance->targets[0].player = player;
			}
			char msg[100] = "Select a card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			if( new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_VALUE, &this_test, 1) == -1 ){
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
			if( selected != -1 ){
				rfg_card_from_grave(instance->targets[0].player, selected);
			}
			kill_card(player, card, get_flashback() ? KILL_REMOVE : KILL_DESTROY);
	}
	return do_flashback(player, card, event, 0, 1, 0, 0, 0, 0);
}

int card_cognivore(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_INSTANT | TYPE_INTERRUPT);
	this_test.type_flag = F1_NO_CREATURE;

	return goyf(player, card, event, &this_test);
}

int card_concentrate(int player, int card, event_t event){
	/*
	  Concentrate |2|U|U
	  Sorcery
	  Draw three cards.
	*/

	if(event == EVENT_RESOLVE_SPELL){
		draw_cards(player, 3);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_confessor(int player, int card, event_t event){

	if( discard_trigger(player, card, event, 2, RESOLVE_TRIGGER_DUH, 0) ){
		gain_life(player, 1);
	}

	return 0;
}

int card_crypt_creeper(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && (count_graveyard(player) > 0 || count_graveyard(1-player) > 0 ) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if( event == EVENT_ACTIVATE){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				instance->targets[0].player = 1-player;
				if(  count_graveyard(1-player) > 0 ){
					if( count_graveyard(player) > 0 ){
						pick_target(&td, "TARGET_PLAYER");
					}
				}
				else{
					instance->targets[0].player = player;
				}

				int selected = select_a_card(player, instance->targets[0].player, 2, 0, 1, -1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				if( selected != -1 ){
					instance->targets[1].player = selected;
					const int *grave = get_grave(player);
					instance->targets[1].card = grave[selected];
					kill_card(player, card, KILL_SACRIFICE);
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION){
			int selected = instance->targets[1].player;
			const int *grave = get_grave(player);
			if( instance->targets[1].card == grave[selected] ){
				rfg_card_from_grave(instance->targets[0].player, selected);
			}
	}

	return 0;
}

int card_crystal_quarry(int player, int card, event_t event){


	if( event == EVENT_ACTIVATE ){
		int choice = do_dialog(player, player, card, -1, -1, " Produce 1\n Produce WUBRG\n Cancel", 0);

		if( choice == 0){
			produce_mana_tapped(player, card, COLOR_COLORLESS, 1);
		}

		if( choice == 1 ){
			card_instance_t *instance = get_card_instance( player, card );
			instance->state |= STATE_TAPPED;
			charge_mana(player, COLOR_COLORLESS, 5);
			if( spell_fizzled != 1 ){
				produce_mana_tapped_multi(player, card, 0, 1, 1, 1, 1, 1);
			}
			else{
				instance->state &= ~STATE_TAPPED;
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}
	else if (event == EVENT_COUNT_MANA && affect_me(player, card) && !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card) ){
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}
	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_cultural_exchanges(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;
	td.illegal_abilities = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( would_validate_arbitrary_target(&td2, player, -1) && would_validate_arbitrary_target(&td2, 1-player, -1) ){
			return basic_spell(player, card, event);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = player;
		instance->targets[0].card = -1;
		instance->targets[1].player = 1-player;
		instance->targets[1].card = -1;
		instance->number_of_targets = 2;

		int max_targ = MIN(count_subtype(player, TYPE_CREATURE, -1), count_subtype(1-player, TYPE_CREATURE, -1));
		int trgs = 2;
		while( can_target(&td) && trgs < 10 && (trgs-2 < max_targ) ){
				if( new_pick_target(&td1, "Select target creature you control.", trgs, GS_LITERAL_PROMPT) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		int i;
		for(i=2; i<trgs; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		}
		int count = 0;
		while( can_target(&td1) && count < trgs-2 ){
				if( new_pick_target(&td, "Select target creature opponent controls.", count+trgs, GS_LITERAL_PROMPT) ){
					state_untargettable(instance->targets[count+trgs].player, instance->targets[count+trgs].card, 1);
					count++;
				}
				else{
					break;
				}
		}
		for(i=trgs; i<trgs+count; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		}
		instance->info_slot = trgs-2;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td2, 0) && validate_target(player, card, &td2, 1) ){
			int i;
			for(i=0; i<instance->info_slot; i++){
				if( validate_target(player, card, &td1, i+2) && validate_target(player, card, &td, i+instance->info_slot+2) ){
					exchange_control_of_target_permanents(player, card, instance->targets[i+2].player, instance->targets[i+2].card,
															instance->targets[i+instance->info_slot+2].player, instance->targets[i+instance->info_slot+2].card);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_darkwater_catacombs(int player, int card, event_t event){
	return od_land(player, card, event, COLOR_BLACK, COLOR_BLUE);
}

int card_darkwater_egg(int player, int card, event_t event){
	return od_egg(player, card, event, COLOR_BLACK, COLOR_BLUE);
}

int card_decimate(int player, int card, event_t event){

	target_definition_t td_artifact;
	default_target_definition(player, card, &td_artifact, TYPE_ARTIFACT );

	target_definition_t td_creature;
	default_target_definition(player, card, &td_creature, TYPE_CREATURE );

	target_definition_t td_enchantment;
	default_target_definition(player, card, &td_enchantment, TYPE_ENCHANTMENT );

	target_definition_t td_land;
	default_target_definition(player, card, &td_land, TYPE_LAND );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td_artifact) && can_target(&td_creature) && can_target(&td_enchantment) ){
		return can_target(&td_land);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->number_of_targets = 0;
			if (pick_target(&td_artifact, "TARGET_ARTIFACT")
				&& new_pick_target(&td_creature, "TARGET_CREATURE", 1, 1)
				&& new_pick_target(&td_enchantment, "TARGET_ENCHANTMENT", 2, 1)){
				new_pick_target(&td_land, "TARGET_LAND", 3, 1);
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td_artifact, 0) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			if( validate_target(player, card, &td_creature, 1) ){
				kill_card(instance->targets[1].player, instance->targets[1].card, KILL_DESTROY);
			}
			if( validate_target(player, card, &td_enchantment, 2) ){
				kill_card(instance->targets[2].player, instance->targets[2].card, KILL_DESTROY);
			}
			if( validate_target(player, card, &td_land, 3) ){
				kill_card(instance->targets[3].player, instance->targets[3].card, KILL_DESTROY);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int ability_decompose(int player, int card, event_t event)
{
  target_definition_t td;
  base_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_ACTIVATE)
	return can_target(&td);

  if (event == EVENT_ACTIVATE
	  && pick_target(&td, "TARGET_GRAVEYARD"))
	{
	  instance->number_of_targets = 0;
	  select_multiple_cards_from_graveyard(player, instance->targets[0].player, 0, player == instance->targets[0].player ? AI_MIN_VALUE : AI_MAX_VALUE,
										   NULL, 3, &instance->targets[1]);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  int i, any_exiled = 0, num_targets = 0;
	  for (i = 1; i <= 3; ++i)
		if (instance->targets[i].player != -1)
		  {
			++num_targets;
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, i);
			if (selected != -1)
			  {
				rfg_card_from_grave(instance->targets[0].player, selected);
				any_exiled = 1;
			  }
		  }

	  if (num_targets > 0 && !any_exiled)
		spell_fizzled = 1;
	}

  return 0;
}

int card_decompose(int player, int card, event_t event)
{
  /* Decompose	|1|B
   * Sorcery
   * Exile up to three target cards from a single graveyard. */

  if (!IS_CASTING(player, card, event))
	return 0;

  if (event == EVENT_CAN_CAST)
	return ability_decompose(player, card, EVENT_CAN_ACTIVATE);
  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	return ability_decompose(player, card, EVENT_ACTIVATE);
  if (event == EVENT_RESOLVE_SPELL)
	{
	  ability_decompose(player, card, EVENT_RESOLVE_ACTIVATION);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_dedicated_martyr(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 3);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_delaying_shield(int player, int card, event_t event){

	/* Delaying Shield	|3|W
	 * Enchantment
	 * If damage would be dealt to you, put that many delay counters on ~ instead.
	 * At the beginning of your upkeep, remove all delay counters from ~. For each delay counter removed this way, you lose 1 life unless you pay |1|W. */

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 ){
			if( source->damage_target_player == player && source->damage_target_card == -1 ){
				add_counters(player, card, COUNTER_DELAY, source->info_slot);
				source->info_slot = 0;
			}
		}
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int count = count_counters(player, card, COUNTER_DELAY);
		int paid = 0;
		while( count > 0 ){
				if( has_mana_multi(player, 1, 0, 0, 0, 0, 1) ){
					charge_mana_multi(player, 1, 0, 0, 0, 0, 1);
					if( spell_fizzled != 1 ){
						paid++;
					}
				}
				else{
					break;
				}
				count--;
		}
		int ltl = count_counters(player, card, COUNTER_DELAY) - paid;
		lose_life(player, ltl);
		remove_all_counters(player, card, COUNTER_DELAY);
	}

	return global_enchantment(player, card, event);
}

int card_dematerialize(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PERMANENT");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 0) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 5, 0, 2, 0, 0, 0);
}

int card_demolish(int player, int card, event_t event ){
	/*
	  Demolish |3|R
	  Sorcery
	  Destroy target artifact or land.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_LAND);

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target artifact or land.", 1, NULL);
}

static int demoralize_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	minimum_blockers(instance->targets[0].player, instance->targets[0].card, event, 2);

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_demoralize(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( has_threshold(player) ){
			pump_subtype_until_eot(player, card, 2, -1, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
		else{
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[player]-1;
				while( count > -1 ){
						if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
							create_targetted_legacy_effect(player, card, &demoralize_legacy, i, count);
						}
						count--;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_deserted_temple(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_LAND);
	td1.preferred_controller = player;
	if( player == AI ){
		td1.required_state = TARGET_STATE_TAPPED;
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_CAN_ACTIVATE || (event == EVENT_COUNT_MANA && affect_me(player, card)) ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && can_target(&td1) && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
			ai_choice = 1;
		}

		if( ai_choice == 1){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Untap target land\n Cancel", ai_choice);
		}
		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			instance->state |= STATE_TAPPED;
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td1, "TARGET_LAND") ){
				instance->info_slot = 66;
				instance->number_of_targets = 1;
			}
			else{
				instance->state &= ~STATE_TAPPED;
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
			if( valid_target(&td1) ){
				untap_card( instance->targets[0].player, instance->targets[0].card );
			}
			parent->info_slot = 0;
		}
		else{
			return mana_producer(player, card, event);
		}
	}
	return 0;
}

int card_devoted_caretaker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0,
									KEYWORD_PROT_INSTANTS + KEYWORD_PROT_SORCERIES, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_NONSICK+GAA_UNTAPPED+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 1, 0, &td, "TARGET_CREATURE");
}

int card_diligent_farmhand(int player, int card, event_t event){

	/* Diligent Farmhand	|G
	 * Creature - Human Druid 1/1
	 * |1|G, Sacrifice ~: Search your library for a basic land card and put that card onto the battlefield tapped. Then shuffle your library.
	 * If ~ is in a graveyard, effects from spells named Muscle Burst count it as a card named Muscle Burst. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 1, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_dirty_wererat(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( has_threshold(player) ){
		cannot_block(player, card, event);
		modify_pt_and_abilities(player, card, event, 2, 2, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION && can_regenerate(player, instance->parent_card) ){
		regenerate_target(player, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD+GAA_REGENERATION, 0, 1, 0, 0, 0, 0, 0, 0, 0);
}

int card_divine_sacrament(int player, int card, event_t event){

	if (event == EVENT_POWER || event == EVENT_TOUGHNESS){
		int amt = has_threshold(player) ? 2 : 1;
		boost_creature_by_color(player, card, event, COLOR_TEST_WHITE, amt, amt, 0, BCT_INCLUDE_SELF);
	}

	return global_enchantment(player, card, event);
}

// druid lyrist --> elvish lirist

// dwarvern grunt --> vanilla

int card_dwarven_recruiter(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int dwarfs[100];
		int tutored = 0;
		test_definition_t this_test;
		new_default_test_definition(&this_test, 0, "Select a Dwarf card.");
		this_test.subtype = SUBTYPE_DWARF;
		while( 1 ){
			int result = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GET_ID, 0, AI_MAX_VALUE, &this_test);
				if( result != -1 ){
					dwarfs[tutored] = get_internal_card_id_from_csv_id(result);
					tutored++;
				}
				else{
					break;
				}
		}
		shuffle(player);
		while( tutored > 0 ){
				int selected = show_deck( player, dwarfs, tutored, "Select a card to put on top of deck.", 0, 0x7375B0 );
				if( selected != -1 ){
					int card_added = add_card_to_hand(player, dwarfs[selected]);
					put_on_top_of_deck(player, card_added);
					if( selected < tutored-1 ){
						int k;
						for(k=selected; k<tutored; k++){
							dwarfs[k] = dwarfs[k+1];
						}
					}
					tutored--;
				}
		}
	}

	return 0;
}

int card_dwarven_shrine(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int id = get_id(instance->targets[1].player, instance->targets[1].card);
		int amount = count_graveyard_by_id(player, id)+count_graveyard_by_id(1-player, id);
		damage_player(instance->targets[1].player, 2*amount, player, card);
	}

	return global_enchantment(player, card, event);
}

int card_earnest_fellowship(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		int clr = get_color(affected_card_controller, affected_card);
		int i;
		for(i=1; i<6; i++){
			if( clr & (1<<i) ){
				event_result |= (1<<(10+i));
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_earth_rift(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_LAND");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 0) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 5, 0, 0, 0, 2, 0);
}

int card_elephants_ambush(int player, int card, event_t event){
	/* Elephant Ambush	|2|G|G
	 * Instant
	 * Put a 3/3 |Sgreen Elephant creature token onto the battlefield.
	 * Flashback |6|G|G */

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			generate_token_by_id(player, card, CARD_ID_ELEPHANT);
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 6, 0, 0, 2, 0, 0);
}

static int engulfing_flames_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( eot_trigger(player, card, event) ){
		remove_status(instance->targets[0].player, instance->targets[0].card, STATUS_CANNOT_REGENERATE);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_engulfing_flames(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 0) ){
				add_status(instance->targets[0].player, instance->targets[0].card, STATUS_CANNOT_REGENERATE);
				create_targetted_legacy_effect(player, card, &engulfing_flames_legacy, instance->targets[0].player, instance->targets[0].card);
				damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
			}
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 3, 0, 0, 0, 1, 0);
}

int card_entomb2(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int *deck = deck_ptr[player];
			if( deck[0] != -1 ){
				char msg[100] = "Select a card to put into your graveyard.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				this_test.no_shuffle = 1;
				int selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_GOOD_TO_PUT_IN_GRAVE, -1, &this_test);
				if( player == AI && selected == -1 ){
					selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_MAX_CMC, -1, &this_test);
				}
				if( selected != -1 ){
					int card_added = add_card_to_hand(player, deck[selected]);
					remove_card_from_deck(player, selected);
					int choice = 1;
					if( get_id(player, card_added) == CARD_ID_NARCOMOEBA ){
						choice = 0;
						if( ! duh_mode(player) ){
							choice = do_dialog(player, player, card_added, -1, -1, " Put into play\n Put into your grave", 0);
						}
					}
					if( choice == 0 ){
						put_into_play(player, card_added);
					}
					else{
						kill_card(player, card_added, KILL_DESTROY);
					}
				}
				shuffle(player);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_epicenter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( has_threshold(player) ){
			return 1;
		}
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( ! has_threshold(player) ){
				pick_target(&td, "TARGET_PLAYER");
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( ! has_threshold(player) ){
				if( instance->targets[0].player != -1 && valid_target(&td) ){
					impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				}
			}
			else{
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_LAND);
				int i;
				for(i=0; i<2; i++){
					if( can_sacrifice(player, i, 1, TYPE_LAND, 0) ){
						new_manipulate_all(player, card, i, &this_test, KILL_SACRIFICE);
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_escape_artist(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	unblockable(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		bounce_permanent(player, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, 0, 0, 1, 0, 0, 0, 0, 0, 0);
}

int card_execute(int player, int card, event_t event){
	/*
	  Execute |2|B
	  Instant
	  Destroy target white creature. It can't be regenerated.
	  Draw a card.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target white creature.", 1, NULL);
}

int card_extract2(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
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
				if( count_deck(instance->targets[0].player) > 0 ){
					char msg[100] = "Select a card to exile.";
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_ANY, msg);
					new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_firebolt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 0) ){
				damage_creature_or_player(player, card, event, 2);
			}
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 4, 0, 0, 0, 1, 0);
}

// flame burst --> kindle

int card_frightcrawler(int player, int card, event_t event){
	fear(player, card, event);
	if( has_threshold(player)  ){
		modify_pt_and_abilities(player, card, event, 2, 2, 0);
		cannot_block(player, card, event);
	}
	return 0;
}

int card_ghastly_demise(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.toughness_requirement = count_graveyard(player) | TARGET_PT_LESSER_OR_EQUAL;
	td.illegal_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 0) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_gorilla_titan(int player, int card, event_t event){
	if(event == EVENT_POWER || event == EVENT_TOUGHNESS){
		if( affect_me(player, card) && count_graveyard(player) == 0 && ! is_humiliated(player, card) ){
			event_result += 4;
		}
	}
	return 0;
}

int card_gravestorm(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int will_draw = 1;
		if( count_graveyard(1-player) > 0 ){
			char msg[100] = "Select a card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			if( new_global_tutor(1-player, 1-player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
				will_draw = 0;
			}
		}
		if( will_draw == 1 ){
			draw_cards(player, 1);
		}
	}

	return global_enchantment(player, card, event);
}

int card_ground_seal(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_hallowed_healer(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
			int prev = 2;
			if( has_threshold(player) ){
				prev+=2;
			}
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( target->info_slot > prev ){
				target->info_slot-=prev;
			}
			else{
				target->info_slot = 0;
			}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_DAMAGE_PREVENTION, 0, 0, 0, 0, 0, 0, 0,
									&td, "TARGET_DAMAGE");
}

int card_haunting_echoes(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int target = instance->targets[0].player;
				int choice = 0;
				if( player == HUMAN ){
					choice = do_dialog(player, player, card, -1, -1, " Remove All Matches\n Manually Pick Cards", 0);
				}

				const int *grave = get_grave(target);
				int *deck = deck_ptr[target];
				int *rfg = rfg_ptr[target];
				int i, j;
				if( choice == 0 ){
					for(i=count_graveyard(target)-1;i>=0;i--){
						for(j=count_deck(target)-1;j>=0;j--){
							if( deck[j] == grave[i] && ! is_basic_land_by_id( cards_data[ grave[i] ].id ) ){
								rfg[ count_rfg(target) ] = deck[j];
								remove_card_from_deck(target, j);
							}
						}
					}
					if( player == HUMAN ){
						show_deck( HUMAN, deck_ptr[target], count_deck(target), "Cards Remaining in Deck", 0, 0x7375B0 );
					}
				}
				else{
					int selected = 9;
					while( selected > -1 ){
						selected = show_deck( HUMAN, deck_ptr[target], count_deck(target), "Pick a card to remove", 0, 0x7375B0 );
						if( selected > -1 ){
							for(i=count_graveyard(target)-1;i>=0;i--){
								if( deck[selected] == grave[i] && ! is_basic_land_by_id( cards_data[ grave[i] ].id ) ){
									rfg[ count_rfg(target) ] = deck[selected];
									remove_card_from_deck(target, selected);
								}
							}
						}
					}
				}
				for(i=count_graveyard(target)-1;i>=0;i--){
					if( ! is_basic_land_by_id( cards_data[ grave[i] ].id ) ){
						rfg_card_from_grave(target, i);
					}
				}
			}
			kill_card( player, card, KILL_DESTROY );
	}
	return 0;
}

int card_holistic_wisdom(int player, int card, event_t event){

	char msg[100] = "Select a card to exile.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, msg);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
		if( hand_count[player] > 0 && count_graveyard(player) > 0  ){
			return ! graveyard_has_shroud(2);
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			this_test.type = get_type(player, selected);
			if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
				rfg_card_in_hand(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1  ){
			const int *grave = get_grave(player);
			add_card_to_hand(player, grave[selected]);
			remove_card_from_grave(player, selected);
		}
	}

	return global_enchantment(player, card, event);
}

int card_infected_vermin(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_multi(player, 2, 1, 0, 0, 0, 0) ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_threshold(player) && has_mana_for_activated_ability(player, card, 3, 1, 0, 0, 0, 0) ){
			choice = do_dialog(player, player, card, -1, -1, " 1 Damage to all\n 3 Damages to all\n Cancel", 1);
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana_for_activated_ability(player, card, 2+choice, 1, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 1+(choice*2);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_damage_all(player, card, 2, instance->info_slot, NDA_PLAYER_TOO, &this_test);
	}

	return 0;
}

int card_innocent_blood(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_ivy_elemental(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		instance->targets[1].card = x_value;
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			 if( instance->targets[1].card > 0 ){
				 add_1_1_counters(player, card, instance->targets[1].card);
			 }
			 instance->targets[8].card = 999;
	}
	else if( instance->targets[8].card != 999 && event == EVENT_TOUGHNESS && affect_me(player, card) ){
			 event_result++;
	}

	return 0;
}

int card_kamahls_desire(int player, int card, event_t event){
	int pump = 0;
	if( has_threshold(player) ){
		pump = 3;
	}

	return generic_aura(player, card, event, player, pump, 0, KEYWORD_FIRST_STRIKE, 0, 0, 0, 0);
}

int card_kamahl_pit_fighter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	check_legend_rule(player, card, event);

	haste(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 3);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0,
													&td, "TARGET_CREATURE_OR_PLAYER");
}

int card_kirtars_desire(int player, int card, event_t event){

	effect_cannot_attack(player, card, event);

	int pump = 0;
	if( has_threshold(player) ){
		pump |= SP_KEYWORD_CANNOT_BLOCK;
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, pump, 0, 0, 0);
}

int card_kirtars_wrath(int player, int card, event_t event){
	/* Kirtar's Wrath	|4|W|W
	 * Sorcery
	 * Destroy all creatures. They can't be regenerated.
	 * Threshold - If seven or more cards are in your graveyard, instead destroy all creatures, then put two 1/1 |Swhite Spirit creature tokens with flying onto the battlefield. Creatures destroyed this way can't be regenerated. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_manipulate_all(player, card, 2, &this_test, KILL_BURY);
			if( has_threshold(player) ){
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_SPIRIT, &token);
				token.qty = 2;
				token.key_plus = KEYWORD_FLYING;
				token.color_forced = COLOR_TEST_WHITE;
				generate_token(&token);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_krosan_avenger(int player, int card, event_t event){
	if( has_threshold(player) ){
		return regeneration(player, card, event, 1, 0, 0, 1, 0, 0);
	}
	return 0;
}

int card_krosan_beast(int player, int card, event_t event){
	if( has_threshold(player) ){
		modify_pt_and_abilities(player, card, event, 7, 7, 0);
	}
	return 0;
}

int card_laquatus_creativity(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = hand_count[instance->targets[0].player];
				draw_cards(instance->targets[0].player, amount);
				new_multidiscard(instance->targets[0].player, amount, 0, player);
			}
			kill_card( player, card, KILL_DESTROY );
	}
	return 0;
}

int card_last_rites(int player, int card, event_t event){

	/* Last Rites	|2|B
	 * Sorcery
	 * Discard any number of cards. Target player reveals his or her hand, then you choose a nonland card from it for each card discarded this way. That player
	 * discards those cards. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				char msg[100] = "Select a card to discard.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				int amount = 0;
				while( hand_count[player] > 0 ){
						int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
						if( selected != -1  ){
							discard_card(player, selected);
							amount++;
							if( player == AI && amount > 1 ){
								break;
							}
						}
						else{
							break;
						}
				}
				while( hand_count[instance->targets[0].player] > 0 && amount > 0 ){
						strcpy(msg, "Select a nonland card.");
						new_default_test_definition(&this_test, TYPE_LAND, msg);
						this_test.type_flag = 1;

						ec_definition_t this_definition;
						default_ec_definition(instance->targets[0].player, player, &this_definition);
						this_definition.effect = EC_DISCARD;

						new_effect_coercion(&this_definition, &this_test);
						amount--;
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_liutenant_kirtar(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 1, 0, 0, 0, 0, 1, 0, &td, "TARGET_CREATURE");
}

int card_life_burst(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				gain_life(instance->targets[0].player, 4);
				int amount = 4 * count_graveyard_by_id(2, get_id(player, card));
				gain_life(instance->targets[0].player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_lithatog(int player, int card, event_t event){
	/* Lithatog	|1|R|G
	 * Creature - Atog 1/2
	 * Sacrifice an artifact: ~ gets +1/+1 until end of turn.
	 * Sacrifice a land: ~ gets +1/+1 until end of turn. */
	return generic_husk(player, card, event, TYPE_LAND | TYPE_ARTIFACT, 1, 1, 0, 0);
}

int card_luminous_guardian(int player, int card, event_t event)
{
  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  int choice = DIALOG(player, card, event,
						  DLG_AUTOCHOOSE_IF_1,
						  "+0/+1", has_mana_for_activated_ability(player, card, MANACOST_W(1)), 1,
						  "Block an additional creature", has_mana_for_activated_ability(player, card, MANACOST_X(2)), 0);	// Never activate for the AI, since it doesn't ever multiblock unless forced to by Blaze of Glory :(

	  if (event == EVENT_CAN_ACTIVATE)
		return choice && can_use_activated_abilities(player, card);

	  if (event == EVENT_ACTIVATE)
		{
		  if (choice == 1)
			charge_mana_for_activated_ability(player, card, MANACOST_W(1));
		  else if (choice == 2)
			charge_mana_for_activated_ability(player, card, MANACOST_X(2));
		}

	  if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  if (choice == 1)
			alternate_legacy_text(1, instance->parent_controller, pump_ability_until_eot(player, card, instance->parent_controller, instance->parent_card, 0, 1, 0, 0));
		  else if (choice == 2)
			alternate_legacy_text(2, instance->parent_controller, can_block_additional_until_eot(player, card, instance->parent_controller, instance->parent_card, 1));
		}
	}

  return 0;
}

int card_magma_vein(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) ){
		return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
		if( spell_fizzled != 1 ){
			if( ! sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		this_test.keyword_flag = 1;
		new_damage_all(player, card, 2, 1, 0, &this_test);
	}

	return global_enchantment(player, card, event);
}

int card_magnivore(int player, int card, event_t event){
	/* Magnivore	|2|R|R
	 * Creature - Lhurgoyf 100/100
	 * Haste
	 * ~'s power and toughness are each equal to the number of sorcery cards in all graveyards. */

	haste(player, card, event);
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_SORCERY);
	return goyf(player, card, event, &this_test);
}

int card_malevolent_awakening(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && count_graveyard_by_type(player, TYPE_CREATURE) &&
		has_mana_for_activated_ability(player, card, 1, 2, 0, 0, 0, 0) && ! graveyard_has_shroud(2)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 2, 0, 0, 0, 0);
		if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_select_target_from_grave(player, card, player, SFG_CANNOT_CANCEL, AI_MAX_VALUE, &this_test, 0);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			const int *grave = get_grave(player);
			add_card_to_hand(player, grave[selected]);
			remove_card_from_grave(player, selected);
		}
	}

	return global_enchantment(player, card, event);
}

int card_master_apothecary(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_abilities = 0;
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_CLERIC;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td) && can_target(&td1) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return 0x63;
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td1, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_CLERIC) ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
					if( pick_target(&td, "TARGET_DAMAGE") ){
						instance->number_of_targets = 1;
					}
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( target->info_slot > 2 ){
				target->info_slot-=2;
			}
			else{
				target->info_slot = 0;
			}
	}
	return 0;
}

int card_millikin(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			return can_produce_mana(player, card);
		}
	}

	else if(event == EVENT_ACTIVATE ){
			mill(player, 1);
			return mana_producer(player, card, event);
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_mind_burst(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				int amount = 1 + count_graveyard_by_id(2, get_id(player, card));
				new_multidiscard(instance->targets[0].player, amount, 0, player);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mindslicer(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		APNAP(p, {new_discard_all(p, player);};);
	}

	return 0;
}

int card_minotaur_explorer(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		int choice = 1;
		if( hand_count[player] > 0 ){
			choice = do_dialog(player, player, card, -1, -1, " Discard\n Sacrifice", 0);
		}
		if (choice == 0){
			discard(player, DISC_RANDOM, player);
		} else {
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return 0;
}

int card_mirari2(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_SPELL);
	this_test.type_flag = F1_NO_CREATURE;

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		player == reason_for_trigger_controller && trigger_cause_controller == player &&
		! check_special_flags(trigger_cause_controller, trigger_cause, SF_NOT_CAST)
	  ){

		int trig = 0;

		if( new_make_test_in_play(trigger_cause_controller, trigger_cause, -1, &this_test) ){
			trig = 1;
		}

		if( ! has_mana(player, COLOR_COLORLESS, 3) ){
			trig = 0;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 1+player;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					charge_mana(player, COLOR_COLORLESS, 3);
					if( spell_fizzled != 1 ){
						copy_spell_from_stack(player, trigger_cause_controller, trigger_cause);
					}
			}
		}
	}
	return 0;
}

int card_molten_influence(int player, int card, event_t event){

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_INSTANT | TYPE_INTERRUPT | TYPE_SORCERY);

	if (event == EVENT_RESOLVE_SPELL){
		if (counterspell_validate(player, card, &td, 0)){
			card_instance_t* instance = get_card_instance(player, card);
			int ai_choice = 0;
			if (life[instance->targets[0].player]-4 < 6){
				ai_choice = 1;
			}
			int choice = do_dialog(instance->targets[0].player, player, card, -1, -1, " Take 4 damage\n Let Molten Influence counter the spell", ai_choice);
			if (choice == 0){
				damage_player(instance->targets[0].player, 4, player, card);
			}
			else{
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
		return 0;
	} else {
		return counterspell(player, card, event, &td, 0);
	}
}

int card_moments_peace(int player, int card, event_t event){

	if( event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, 2, 0, 0, 1, 0, 0) ){
		return 1;
	}
	else if( event == EVENT_PAY_FLASHBACK_COSTS ){
			charge_mana_multi(player, 2, 0, 0, 1, 0, 0);
			if( spell_fizzled != 1){
				return 1;
			}
	}
	else if( event == EVENT_CAN_CAST ){
			return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
			fog_effect(player, card);
			if( get_flashback() ){
			   kill_card(player, card, KILL_REMOVE);
			}
			else{
				 kill_card(player, card, KILL_DESTROY);
			}
	}
	return 0;
}

int card_mortivore(int player, int card, event_t event){
	/* Mortivore	|2|B|B
	 * Creature - Lhurgoyf 100/100
	 * ~'s power and toughness are each equal to the number of creature cards in all graveyards.
	 * |B: Regenerate ~. */

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);

	goyf(player, card, event, &this_test);

	return regeneration(player, card, event, 0, 1, 0, 0, 0, 0);
}

int card_mossfire_egg(int player, int card, event_t event){
	return od_egg(player, card, event, COLOR_GREEN, COLOR_RED);
}

int card_mossfire_valley(int player, int card, event_t event){
	return od_land(player, card, event, COLOR_GREEN, COLOR_RED);
}

int card_muscle_burst(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				int amount = 3 + count_graveyard_by_id(2, get_id(player, card)) + count_graveyard_by_id(2, CARD_ID_DILIGENT_FARMHAND);
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, amount, amount);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mystic_crusader(int player, int card, event_t event){
	if( has_threshold(player)  ){
		modify_pt_and_abilities(player, card, event, 1, 1, KEYWORD_FLYING);
	}
	return 0;
}

int card_mystic_enforcer(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 1) ){ return 0; }

	if( has_threshold(player) && affect_me(player, card) ){
		if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
			event_result += 3;
		}
		else if( event == EVENT_ABILITIES ){
			event_result |= KEYWORD_FLYING;
		}
	}
	return 0;
}

int card_mystic_penitent(int player, int card, event_t event){
	vigilance(player, card, event);
	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_ABILITIES) && affect_me(player, card) && has_threshold(player)){
		modify_pt_and_abilities(player, card, event, 1, 1, KEYWORD_FLYING);
	}
	return 0;
}

int card_mystic_visionary(int player, int card, event_t event){
	if( has_threshold(player)  ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FLYING);
	}
	return 0;
}

// mystic visionary, mystic zealot --> mystic crusader

int card_nantuko_disciple(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 1, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_nantuko_elder(int player, int card, event_t event){
	return mana_producing_creature_multi(player, card, event, 12, 1, 0, 0, 1, 0, 0);
}

int card_nantuko_mentor(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = get_power(instance->targets[0].player, instance->targets[0].card);
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, amount, amount);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 0, 1, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_nantuko_shrine(int player, int card, event_t event){
	/* Nantuko Shrine	|1|G|G
	 * Enchantment
	 * Whenever a player casts a spell, that player puts X 1/1 |Sgreen Squirrel creature tokens onto the battlefield, where X is the number of cards in all graveyards with the same name as that spell. */

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int id = get_id(instance->targets[1].player, instance->targets[1].card);
		int amount = count_graveyard_by_id(player, id)+count_graveyard_by_id(1-player, id);

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SQUIRREL, &token);
		token.t_player = instance->targets[1].player;
		token.qty = amount;
		generate_token(&token);
	}

	return global_enchantment(player, card, event);
}

int card_need_for_speed(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
			sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) &&
			pick_target(&td, "TARGET_CREATURE")
		  ){
			instance->number_of_targets = 1;
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td)  ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
		}
	}

	return global_enchantment(player, card, event);
}

int card_nefarious_lich(int player, int card, event_t event ){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player){
				int cd = count_graveyard(player)-1;
				while( damage->info_slot > 0 && cd > -1 ){
						damage->info_slot--;
						rfg_card_from_grave(player, cd);
						cd--;
				}
				if( damage->info_slot > 0 ){
					lose_the_game(player);
				}
			}
		}
	}

	if( leaves_play(player, card, event) ){
		lose_the_game(player);
	}

	return global_enchantment(player, card, event);
}

int card_new_frontiers(int player, int card, event_t event){

	/* New Frontiers	|X|G
	 * Sorcery
	 * Each player may search his or her library for up to X basic land cards and put them onto the battlefield tapped. Then each player who searched his or her
	 * library this way shuffles it. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			int amount = instance->info_slot;
			if( amount > 0 ){
				char options[100];
				scnprintf(options, 100, "X is %d.\n Search\n Decline", amount);
				int p;
				for (p = 0; p <= 1; ++p){
					if (do_dialog(p, player, card, -1, -1, options, 0) == 0){
						tutor_basic_lands(p, TUTOR_PLAY_TAPPED, amount);
					}
				}
			}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_nimble_mongoose(int player, int card, event_t event){
  if(event == EVENT_POWER || event == EVENT_TOUGHNESS){
	if( affect_me(player, card) && has_threshold(player) ){
	  event_result += 2;
	}
  }
  return 0;
}

int card_nomad_decoy(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1)
	  ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_threshold(player) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 2) ){
			choice = do_dialog(player, player, card, -1, -1, " Tap 1 creature\n Tap 2 creatures\n Cancel", 1);
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1+choice);
			if( spell_fizzled != 1 ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					instance->info_slot = 1;
					if( choice == 1 ){
						state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
						if( new_pick_target(&td, "TARGET_CREATURE", 1, 1) ){
							instance->number_of_targets = 2;
							instance->info_slot = 2;
							tap_card(player, card);
						}
						state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
					}
					else{
						tap_card(player, card);
					}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
	}

	return 0;
}

int card_nomad_stadium(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 2) && can_use_activated_abilities(player, card) &&
			has_threshold(player) && can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Gain 4 life\n Cancel", ai_choice);
		}

		if( choice == 0){
			damage_player(player, 1, player, card);
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1);
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
				untap_card_no_event(player, card);
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				gain_life(player, 4);
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

int card_nut_collector(int player, int card, event_t event){
	/* Nut Collector	|5|G
	 * Creature - Human Druid 1/1
	 * At the beginning of your upkeep, you may put a 1/1 |Sgreen Squirrel creature token onto the battlefield.
	 * Threshold - Squirrel creatures get +2/+2 as long as seven or more cards are in your graveyard. */

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		generate_token_by_id(player, card, CARD_ID_SQUIRREL);
	}

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
		&& has_threshold(player)){
		boost_creature_type(player, card, event, SUBTYPE_SQUIRREL, 2, 2, 0, BCT_INCLUDE_SELF);
	}

	return 0;
}

int card_overeager_apprentice(int player, int card, event_t event, int color){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( can_produce_mana(player, card) && hand_count[player] > 0 ){
			return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard");
		int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
		if( selected != -1 ){
			discard_card(player, selected);
			produce_mana(player, COLOR_BLACK, 3);
			kill_card(player, card, KILL_SACRIFICE);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			declare_mana_available(player, COLOR_BLACK, 3);
		}
	}

	return 0;
}

int card_patrol_hound(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, KEYWORD_FIRST_STRIKE, 0);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_patron_wizard(int player, int card, event_t event){

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.allowed_controller = player;
	td2.preferred_controller = player;
	td2.illegal_abilities = 0;
	td2.illegal_state = TARGET_STATE_TAPPED;
	td2.required_subtype = SUBTYPE_WIZARD;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td2) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return card_spiketail_hatchling(player, card, event);
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td2, "TARGET_CREATURE") ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
				instance->targets[0].player = card_on_stack_controller;
				instance->targets[0].card = card_on_stack;
				instance->number_of_targets = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			counterspell_resolve_unless_pay_x(player, card, NULL, 0, 1);
	}
	return 0;
}

int card_peek(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
		if( valid_target(&td) ){
			reveal_target_player_hand(instance->targets[0].player);
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_petrified_field(int player, int card, event_t event){

	char msg[100] = "Select a land card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, msg);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){

		int choice = 0;
		if( ! paying_mana() && count_graveyard_by_type(player, TYPE_LAND) > 0 && count_graveyard_by_type(player, TYPE_LAND) > 0 &&
			! graveyard_has_shroud(2) && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Sac and return a land card to your hand\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
			tap_card(player, card);
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
					instance->info_slot = 1;
					kill_card(player, card, KILL_SACRIFICE);
				}
				else{
					untap_card_no_event(player, card);
					spell_fizzled = 1;
				}
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
			const int *grave = get_grave(player);
			int selected = validate_target_from_grave(player, card, player, 0);
			if( selected != -1 && grave[selected] == instance->targets[0].card ){
				add_card_to_hand(player, grave[selected]);
				remove_card_from_grave(player, selected);
			}
		}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_phantatog(int player, int card, event_t event){

	/* Phantatog	|1|W|U
	 * Creature - Atog 1/2
	 * Sacrifice an enchantment: ~ gets +1/+1 until end of turn.
	 * Discard a card: ~ gets +1/+1 until end of turn. */

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ENCHANTMENT, "");

		int num_can_sac = max_can_sacrifice_as_cost(player, card, &test);
		int num_can_discard = hand_count[player];
		int num_times_can_activate = MAX(0, num_can_sac) + MAX(0, num_can_discard);
		if (num_times_can_activate <= 0){
			return 0;
		}

		return generic_shade_amt_can_pump(player, card, 1, 0, MANACOST0, num_times_can_activate);
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			if( hand_count[player] > 0 || can_sacrifice_type_as_cost(player, 1, TYPE_ENCHANTMENT) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int choice = 0;
			if( hand_count[player] > 0 ){
				if( can_sacrifice_type_as_cost(player, 1, TYPE_ENCHANTMENT) ){
					choice = do_dialog(player, player, card, -1, -1, " Discard\n Sac an enchantment\n Cancel", 1);
				}
			}
			if( choice == 0 ){
				discard(player, 0, player);
			}
			else if( choice == 1 ){
				controller_sacrifices_a_permanent(player, card, TYPE_ENCHANTMENT, SAC_AS_COST);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int p = instance->parent_controller, c = instance->parent_card;
		if (in_play(p, c)){
			pump_until_eot_merge_previous(p, c, p, c, 1, 1);
		}
	}

	return 0;
}

int card_pianna_nomad_captain(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // Whenever ~ attacks, attacking creatures get +1/+1 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  pump_creatures_until_eot(player, card, current_turn, 0, 1,1, 0,0, &test);
	}

  return 0;
}

int card_primal_frenzy(int player, int card, event_t event){
	/*
	  Primal Frenzy |G
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature has trample.
	*/
	return generic_aura(player, card, event, player, 0, 0, KEYWORD_TRAMPLE, 0, 0, 0, 0);
}

int card_psychatog(int player, int card, event_t event){

	/* Psychatog	|1|U|B
	 * Creature - Atog 1/2
	 * Discard a card: ~ gets +1/+1 until end of turn.
	 * Exile two cards from your graveyard: ~ gets +1/+1 until end of turn. */

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST){
		int num_can_discard = hand_count[player];
		int num_times_can_exile = count_graveyard(player) / 2;
		int num_times_can_activate = MAX(0, num_can_discard) + MAX(0, num_times_can_exile);
		if (num_times_can_activate <= 0){
			return 0;
		}

		return generic_shade_amt_can_pump(player, card, 1, 0, MANACOST0, num_times_can_activate);
	}

	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( hand_count[player] > 0 || count_graveyard(player) > 1 ){
				return 1;
			}
		}
	}
	else if( event == EVENT_ACTIVATE ){
			instance->number_of_targets = 0;
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				int choice = 0;
				if( count_graveyard(player) > 1 ){
					if( hand_count[player] > 0 ){
						choice = do_dialog(player, player, card, -1, -1, " Exile from graveyard\n Discard\n Cancel", 0);
					}
				}
				else{
					choice = 1;
				}
				if( choice == 0 ){
					if (select_multiple_cards_from_graveyard(player, player, 1, AI_MIN_VALUE, NULL, 2, &instance->targets[0])){
						rfg_card_from_grave(player, instance->targets[0].player);
						rfg_card_from_grave(player, instance->targets[1].player);
					}
				}
				else if( choice == 1 ){
					target_definition_t td;
					base_target_definition(player, card, &td, 0);
					td.zone = TARGET_ZONE_HAND;

					if (pick_target(&td, "PROMPT_DISCARDACARD")){
						discard_card(player, instance->targets[0].card);
					}
					ai_modifier += (player == AI) ? -48 : 48;
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			int p = instance->parent_controller, c = instance->parent_card;
			if (in_play(p, c)){
				pump_until_eot_merge_previous(p, c, p, c, 1, 1);
			}
	}

	return 0;
}

int card_puppeteer(int player, int card, event_t event)
{
  // 0x4110d0

  // |U, |T: You may tap or untap target creature.
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = ANYBODY;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	twiddle(player, card, 0);

  int rval = generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_U(1), 0, &td, "TARGET_CREATURE");

  if (event == EVENT_ACTIVATE && player == AI && cancel != 1)
	ai_modifier_twiddle(player, card, 0);

  return rval;
}

int card_ray_of_distortion(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "DISENCHANT");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( validate_target(player, card, &td, 0) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 4, 0, 0, 0, 0, 2);
}

int card_reckless_charge(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE );
  td.allowed_controller = 2;
  td.preferred_controller = player;
  td.allow_cancel = 0;

  card_instance_t *instance = get_card_instance(player, card);

  if( event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, 2, 0, 0, 0, 1, 0) && can_target(&td) ){
	  if( can_sorcery_be_played(player, event) ){
		  return 1;
	  }
	}
	else if( event == EVENT_PAY_FLASHBACK_COSTS ){
			charge_mana_multi(player, 2, 0, 0, 0, 1, 0);
			if( spell_fizzled != 1){
				return 1;
			}
	}
	else if( event == EVENT_CAN_CAST && can_target(&td) ){
			return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if(event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
			   pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3,0, 0, SP_KEYWORD_HASTE);
			}

			if( get_flashback() ){
			   kill_card(player, card, KILL_REMOVE);
			}
			else{
				 kill_card(player, card, KILL_DESTROY);
			}
	}
	return 0;
}

int card_recoup(int player, int card, event_t event ){

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_SORCERY) > 0 ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		char msg[100] = "Select a sorcery card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SORCERY, msg);
		if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave(player, card, player, 0);
		create_spell_has_flashback_legacy(player, card, selected, 0);
		if( get_flashback() ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return do_flashback(player, card, event, 3, 0, 0, 0, 1, 0);
}

int card_repentant_vampire(int player, int card, event_t event){

	/* Repentant Vampire	|3|B|B
	 * Creature - Vampire 3/3
	 * Flying
	 * Whenever a creature dealt damage by ~ this turn dies, put a +1/+1 counter on ~.
	 * Threshold - As long as seven or more cards are in your graveyard, ~ is |Swhite and has "|T: Destroy target |Sblack creature." */

	if( sengir_vampire_trigger(player, card, event, 2) ){
		card_instance_t* instance = get_card_instance(player, card);
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	if (!IS_GAA_EVENT(event) && event != EVENT_SET_COLOR){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	if( has_threshold(player) ){
		if (event == EVENT_SET_COLOR && affect_me(player, card)){
			event_result = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		}
		return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
	}
	return 0;
}

int card_rites_of_initiation(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int amount = choose_a_number(player, "Discard how many cards?", hand_count[player]/2);
			if( amount > hand_count[player] ){
				amount = hand_count[player];
			}
			multidiscard(player, amount, 1);
			pump_subtype_until_eot(player, card, player, -1, amount, 0, 0, 0);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_rites_of_refusal(int player, int card, event_t event){

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

	if(event == EVENT_RESOLVE_SPELL ){
		char msg[100] = "Select a card to discard.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		int amount = 0;
		while( hand_count[player] > 0 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
				if( selected != -1  ){
					discard_card(player, selected);
					amount++;
					if( player == AI && amount > 1 ){
						break;
					}
				}
				else{
					break;
				}
		}
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, 3*amount);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_rites_of_spring(int player, int card, event_t event){

	/* Rites of Spring	|1|G
	 * Sorcery
	 * Discard any number of cards. Search your library for that many basic land cards, reveal those cards, and put them into your hand. Then shuffle your
	 * library. */

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard.");
		int amount = 0;
		while( hand_count[player] > 0 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
				if( selected != -1  ){
					discard_card(player, selected);
					amount++;
					if( IS_AI(player) && amount > 1 ){
						break;
					}
				}
				else{
					break;
				}
		}

		if (amount == 0){
			shuffle(player);
		} else {
			tutor_basic_lands(player, TUTOR_HAND, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_roar_of_the_wurm(int player, int card, event_t event){
	/* Roar of the Wurm	|6|G
	 * Sorcery
	 * Put a 6/6 |Sgreen Wurm creature token onto the battlefield.
	 * Flashback |3|G */

	if( event == EVENT_CAN_CAST ){
			return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
			generate_token_by_id(player, card, CARD_ID_WURM);
			if( get_flashback() ){
			   kill_card(player, card, KILL_REMOVE);
			}
			else{
				 kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 3, 0, 0, 1, 0, 0);
}

int card_rotting_giant(int player, int card, event_t event)
{
  // Whenever ~ attacks or blocks, sacrifice it unless you exile a card from your graveyard.
  if (declare_attackers_trigger(player, card, event, 0, player, card)
	  || (blocking(player, card, event) && !is_humiliated(player, card)))
	{
	  test_definition_t this_test;
	  new_default_test_definition(&this_test, 0, "Select a card to exile.");
	  if (get_grave(player)[0] == -1
		  || new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) == -1)
		kill_card(player, card, KILL_SACRIFICE);
	}

  return 0;
}

int card_sacred_rites(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		char msg[100] = "Select a card to discard.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		int amount = 0;
		while( hand_count[player] > 0 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
				if( selected != -1  ){
					discard_card(player, selected);
					amount++;
					if( player == AI && amount > 1 ){
						break;
					}
				}
				else{
					break;
				}
		}
		pump_subtype_until_eot(player, card, player, -1, 0, amount, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sadistic_hypnotist(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			new_multidiscard(instance->targets[0].player, 2, 0, player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE+GAA_CAN_TARGET+GAA_CAN_SORCERY_BE_PLAYED,
													0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_sarcatog(int player, int card, event_t event){

	/* Sarcatog	|1|B|R
	 * Creature - Atog 1/2
	 * Exile two cards from your graveyard: ~ gets +1/+1 until end of turn.
	 * Sacrifice an artifact: ~ gets +1/+1 until end of turn. */

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ARTIFACT, "");
		test.not_me = 1;

		int num_can_sac = max_can_sacrifice_as_cost(player, card, &test);
		int num_times_can_exile = count_graveyard(player) / 2;
		int num_times_can_activate = MAX(0, num_can_sac) + MAX(0, num_times_can_exile);
		if (num_times_can_activate <= 0){
			return 0;
		}

		return generic_shade_amt_can_pump(player, card, 1, 0, MANACOST0, num_times_can_activate);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( count_graveyard(player) > 1 ){
				return 1;
			}
			return can_sacrifice_type_as_cost(player, 1, TYPE_ARTIFACT);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int choice = 0;
			if( count_graveyard(player) > 1 ){
				if( can_sacrifice_type_as_cost(player, 1, TYPE_ARTIFACT) ){
					choice = do_dialog(player, player, card, -1, -1, " Exile from graveyard\n Sac an artifact\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 0 ){
				char msg[100] = "Select a card to exile.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) != -1 ){
					new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test);
				}
				else{
					spell_fizzled = 1;
				}
			}
			else if( choice == 1 ){
				controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, SAC_AS_COST);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		int p = instance->parent_controller, c = instance->parent_card;
		if (in_play(p, c)){
			pump_until_eot_merge_previous(p, c, p, c, 1, 1);
		}
	}

	return 0;
}

int card_setons_desire(int player, int card, event_t event){
	int pump = 0;
	if( has_threshold(player) ){
		pump = SP_KEYWORD_LURE;
	}

	return generic_aura(player, card, event, player, 2, 2, 0, pump, 0, 0, 0);
}

int card_seton_krosan_protector(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	return tap_a_permanent_you_control_for_mana(player, card, event, TYPE_PERMANENT, SUBTYPE_DRUID, COLOR_GREEN, 1);
}

int card_shadowblood_ridge(int player, int card, event_t event){
	return od_land(player, card, event, COLOR_BLACK, COLOR_RED);
}

int card_shadowblood_egg(int player, int card, event_t event){
	return od_egg(player, card, event, COLOR_BLACK, COLOR_RED);
}

int card_shadowmage_infiltrator(int player, int card, event_t event){

	fear(player, card, event);

	return card_stealer_of_secrets(player, card, event);
}

int card_shelter2(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE );
  td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if(event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
				int key = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, key, 0);
				draw_cards(player, 1);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

static int shifty_doppelganger_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	haste(instance->targets[0].player, instance->targets[0].card, event);

	if( eot_trigger(player, card, event) ){
		state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
		if( check_rfg(player, CARD_ID_SHIFTY_DOPPELGANGER) ){
			int card_added = add_card_to_hand(player, get_internal_card_id_from_csv_id(CARD_ID_SHIFTY_DOPPELGANGER));
			remove_card_from_rfg(player, CARD_ID_SHIFTY_DOPPELGANGER);
			put_into_play(player, card_added);
		}
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
	}

	return 0;
}

int card_shifty_doppelganger(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 0) ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 3, 0, 1, 0, 0, 0);
		if( spell_fizzled != 1 ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a creature to put into play.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		int result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		if( result > -1 ){
			create_targetted_legacy_effect(player, card, &shifty_doppelganger_legacy, player, result);
		}
	}

	return 0;
}

int card_shower_of_coals(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int pl[2] = {0, 0};
			int trgs = 0;
			while( trgs < 3 && can_target(&td) ){
					if( new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", trgs, 0) ){
						if( instance->targets[trgs].card != -1 ){
							state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
							trgs++;
						}
						else{
							if( pl[instance->targets[trgs].player] != 1 ){
								pl[instance->targets[trgs].player] = 1;
								trgs++;
							}
							else{
								break;
							}
						}
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<trgs; i++){
				if( instance->targets[i].card != -1 ){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
				}
			}
			if( trgs < 1 ){
				spell_fizzled = 1;
			}
			else{
				instance->info_slot = trgs;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			int i;
			int dmg = 2;
			if( has_threshold(player) ){
				dmg = 4;
			}
			for(i=0;i<instance->info_slot; i++){
				if( validate_target(player, card, &td, i) ){
					damage_creature(instance->targets[i].player, instance->targets[i].card, dmg, player, card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_skeletal_scrying(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( count_graveyard(player) > 0 && has_mana(player, COLOR_COLORLESS, 1) ){
			return basic_spell(player, card, event);
		}
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = 0;
			set_special_flags2(player, card, SF2_X_SPELL);
			int max = count_graveyard(player);
			while( ! has_mana(player, COLOR_COLORLESS, max) ){
					max--;
			}
			if( player == AI ){
				while( life[player]-max < 6 ){
						max--;
				}
				if( max < 1 ){
					ai_modifier-=250;
				}
			}

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");
			int trc = select_multiple_cards_from_graveyard(player, player, 0, AI_MIN_VALUE, &this_test, max, &instance->targets[0]);
			charge_mana(player, COLOR_COLORLESS, trc);
			if( spell_fizzled != 1 ){
				int i;
				for(i=0; i<trc; i++){
					rfg_card_from_grave(player, instance->targets[i].player);
				}
				instance->info_slot = trc;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, instance->info_slot);
		lose_life(player, instance->info_slot);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_skycloud_expanse(int player, int card, event_t event){
	return od_land(player, card, event, COLOR_BLUE, COLOR_WHITE);
}

int card_skycloud_eGG(int player, int card, event_t event){
	return od_egg(player, card, event, COLOR_BLUE, COLOR_WHITE);
}

int card_soulcatcher(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}
	return 0;
}

int card_spark_mage(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL+DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
		}
	}
	return 0;
}

int card_squirrel_mob(int player, int card, event_t event){
	if( (event == EVENT_POWER ||  event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		int total = count_subtype(2, TYPE_PERMANENT, SUBTYPE_SQUIRREL);
		if (total > 0 && has_subtype(player, card, SUBTYPE_SQUIRREL)){
			--total;
		}
		event_result += total;
	}
	return 0;
}

int card_squirrel_nest(int player, int card, event_t event){
	/* Squirrel Nest	|1|G|G
	 * Enchantment - Aura
	 * Enchant land
	 * Enchanted land has "|T: Put a 1/1 |Sgreen Squirrel creature token onto the battlefield." */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			ai_modifier+=200;
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
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}

		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SQUIRREL, &token);
			token.t_player = t_player;
			generate_token(&token);
		}
	}

	return 0;
}

int card_stalking_bloodsucker(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, 2, 2);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, 1, 1, 0, 0, 0, 0, 0, 0, 0);
}

int card_standstill(int player, int card, event_t event){

	if(event == EVENT_CAN_CAST){
		return 1;
	}

	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){

	   int trig = 1;

	   if( is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
		  trig = 0;
	   }

	   if( trig > 0 ){
		  if(event == EVENT_TRIGGER){
			 event_result |= RESOLVE_TRIGGER_MANDATORY;
		  }
		  else if(event == EVENT_RESOLVE_TRIGGER){
				  draw_cards(1-trigger_cause_controller, 3);
				  kill_card(player, card, KILL_SACRIFICE);
		  }
	   }
	}

  return global_enchantment(player, card, event);
}

int card_stone_tongue_basilisk(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if (trigger_condition == TRIGGER_MUST_BLOCK && has_threshold(player)){
			everybody_must_block_me(player, card, event);
		}

		if (event == EVENT_CHANGE_TYPE && affect_me(player, card) ){
			card_instance_t* instance = get_card_instance(player, card);
			instance->destroys_if_blocked |= DIFB_DESTROYS_UNPROTECTED;
		}

		for_each_creature_damaged_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE, &lowland_basilisk_effect, player, card);
	}

	return 0;
}

int card_sungrass_prairie(int player, int card, event_t event){
	return od_land(player, card, event, COLOR_GREEN, COLOR_WHITE);
}

int card_sungrass_egg(int player, int card, event_t event){
	return od_egg(player, card, event, COLOR_GREEN, COLOR_WHITE);
}

int card_sylvan_might(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td) ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2, KEYWORD_TRAMPLE, 0);
			}
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 2, 0, 0, 2, 0, 0);
}

int card_syncopate(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		// That - sign is there for a reason.
		counterspell_resolve_unless_pay_x(player, card, NULL, -KILL_REMOVE, instance->info_slot);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL | GS_X_SPELL, NULL, NULL, 1, NULL);
}


int card_tainted_pact(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int ids[available_slots];
		int i;
		for(i=0; i<available_slots; i++)
			ids[i] = 0;
		int *deck = deck_ptr[player];
		int count = 0;
		int revealed[count_deck(player)];
		int ai_max = internal_rand(10)+1;
		while(deck[0] != -1 ){
				revealed[count] = deck[0];
				if( player == HUMAN ){
					if( show_deck( HUMAN, revealed, count+1, "Click on a card to stop", 0, 0x7375B0 ) != -1 ){
						add_card_to_hand(player, deck[0]);
						remove_card_from_deck(player, 0);
						break;
					}
				}
				else{
					if( count+1 == ai_max ){
						show_deck( 1-player, revealed, count+1, "Opponent exiled these cards.", 0, 0x7375B0 );
						remove_card_from_deck(player, 0);
						add_card_to_hand(player, deck[0]);
						break;
					}
				}
				ids[cards_data[deck[0]].id]++;
				if( ids[cards_data[deck[0]].id] > 1 ){
					if( player == AI ){
						show_deck( HUMAN, revealed, count+1, "Opponent exiled two cards with the same name.", 0, 0x7375B0 );
					}
					rfg_card_in_deck(player, 0);
					break;
				}
				rfg_card_in_deck(player, 0);
				count++;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_terravore(int player, int card, event_t event){
	/* Terravore	|1|G|G
	 * Creature - Lhurgoyf 100/100
	 * Trample
	 * ~'s power and toughness are each equal to the number of land cards in all graveyards. */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card)){
		event_result += count_graveyard_by_type(ANYBODY, TYPE_LAND);
	}
	return 0;
}

int card_thaumatog(int player, int card, event_t event){
	/* Thaumatog	|1|G|W
	 * Creature - Atog 1/2
	 * Sacrifice a land: ~ gets +1/+1 until end of turn.
	 * Sacrifice an enchantment: ~ gets +1/+1 until end of turn. */
	return generic_husk(player, card, event, TYPE_LAND | TYPE_ENCHANTMENT, 1, 1, 0, 0);
}

int card_think_tank(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			if( show_deck( player, deck, 1, "Put this card into your graveyard ?", 0, 0x7375B0 ) != -1 ){
				mill(player, 1);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_thought_devourer(int player, int card, event_t event){
	if( event == EVENT_MAX_HAND_SIZE && current_turn == player ){
		event_result-=4;
	}
	return 0;
}

int card_thought_eater(int player, int card, event_t event){
	if( event == EVENT_MAX_HAND_SIZE && current_turn == player ){
		event_result-=3;
	}
	return 0;
}

int card_thought_nibbler(int player, int card, event_t event){
	if( event == EVENT_MAX_HAND_SIZE && current_turn == player ){
		event_result-=2;
	}
	return 0;
}

int card_time_stretch(int player, int card, event_t event)
{
  /* Time Stretch	|8|U|U
   * Sorcery
   * Target player takes two extra turns after this one. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.preferred_controller = player;

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	pick_target(&td, "TARGET_PLAYER");

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td))
		{
			if( instance->targets[0].player == player ){
				time_walk_effect(player, card);
				time_walk_effect(player, card);
			}
			else{
				int fake = add_card_to_hand(instance->targets[0].player, get_original_internal_card_id(player, card));
				time_walk_effect(instance->targets[0].player, fake);
				time_walk_effect(instance->targets[0].player, fake);
				obliterate_card(instance->targets[0].player, fake);
			}
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_tireless_tribe(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 4);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_traumatize(int player, int card, event_t event){

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
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				mill( instance->targets[0].player, count_deck(instance->targets[0].player) / 2 );
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_unifying_theory(int player, int card, event_t event){

	/* Unifying Theory	|1|U
	 * Enchantment
	 * Whenever a player casts a spell, that player may pay |2. If the player does, he or she draws a card. */

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == trigger_cause_controller ){
		int trig = 0;
		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			trig = 1;
		}

		if( trig == 1 && ! has_mana(trigger_cause_controller, COLOR_COLORLESS, 2) ){
			trig = 0;
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_AI(trigger_cause_controller);
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if (charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, trigger_cause_controller, COLOR_COLORLESS, 2)){
						draw_a_card(trigger_cause_controller);
					}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_upheaval(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if(event == EVENT_RESOLVE_SPELL){
		int p;
		// do enchantments first, so that auras get bounced
		for( p = 0; p < 2; p++){
			int count = 0;
			while(count < active_cards_count[p]){
				card_data_t* card_d = get_card_data(p, count);
				if( card_d->type & TYPE_ENCHANTMENT && in_play(p, count)){
					bounce_permanent( p, count );
				}
				count++;
			}
		}

		for( p = 0; p < 2; p++){
			int count = 0;
			while(count < active_cards_count[p]){
				card_data_t* card_d = get_card_data(p, count);
				if( card_d->type & TYPE_PERMANENT && in_play(p, count)){
					bounce_permanent( p, count );
				}
				count++;
			}
		}
		kill_card(player, card, KILL_SACRIFICE);
	}
	return 0;
}

int card_vampiric_dragon(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( sengir_vampire_trigger(player, card, event, 2) ){
		add_1_1_counters(player, card, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_verdant_succession(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_token(affected_card_controller, affected_card) &&
			(get_color(affected_card_controller, affected_card) & COLOR_TEST_GREEN)
		  ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4 ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				int pos = instance->targets[11].player;
				if( pos < 11 ){
					instance->targets[pos].player = affected_card_controller;
					instance->targets[pos].card = affected->original_internal_card_id;
					instance->targets[11].player++;
				}
			}
		}
	}

	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
		affect_me(player, card )
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0; i<2; i++){
					int k;
					int dead_creatures[100];
					int dcc = 0;
					for(k=0; k<instance->targets[11].player; k++){
						int t_player = instance->targets[k].player;
						if( t_player == i ){
							dead_creatures[dcc] = instance->targets[k].card;
							dcc++;
						}
					}
					while( dcc > 0 ){
							int selected = show_deck( i, dead_creatures, dcc, "Select a creature to tutor from deck", 0, 0x7375B0 );
							if( selected != -1 ){
								char buffer2[100];
								card_ptr_t* c_me = cards_ptr[ cards_data[dead_creatures[selected]].id ];
								scnprintf(buffer2, 100, "Search for %s.", c_me->name);
								test_definition_t this_test;
								new_default_test_definition(&this_test, TYPE_CREATURE, buffer2);
								this_test.id = cards_data[dead_creatures[selected]].id;
								new_global_tutor(i, i, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
								for(k=selected; k<dcc; k++){
									dead_creatures[k] = dead_creatures[k+1];
								}
								dcc--;
							}
							else{
								break;
							}
					}
				}
				instance->targets[11].player = 0;
		}
   }

	return global_enchantment(player, card, event);
}

int card_volley_of_boulders(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int trgs = 0;
			while( trgs < 6 ){
					if( new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", trgs, 1) ){
						trgs++;
					}
			}
			instance->info_slot = trgs;
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			divide_damage(player, card, &td);
			if( get_flashback() ){
				kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 0, 0, 0, 0, 6, 0);
}

int card_wayward_angel(int player, int card, event_t event){

	if( has_threshold(player) ){
		modify_pt_and_abilities(player, card, event, 3, 3, KEYWORD_TRAMPLE);
		if( event == EVENT_SET_COLOR && affect_me(player, card) ){
			event_result = COLOR_TEST_BLACK;
		}
		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}

int card_werebear(int player, int card, event_t event){
  if(event == EVENT_POWER || event == EVENT_TOUGHNESS){
	if( affect_me(player, card) && has_threshold(player) ){
	  event_result += 3;
	}
  }
  return mana_producing_creature(player, card, event, has_threshold(player) ? 0 : 24, COLOR_GREEN, 1);
}

int card_wild_mongrel(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_DISCARD, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int choice = 0;
			if( player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Discard & Pump\n Discard, Pump & Change color \n Pass", 0);
			}
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				discard(player, 0, player);
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, card, player, instance->parent_card, 1, 1);
		if( instance->info_slot == 67 ){
			change_color(player, card, player, instance->parent_card, 1<<choose_a_color(player, 0), CHANGE_COLOR_SET|CHANGE_COLOR_END_AT_EOT|CHANGE_COLOR_NO_SLEIGHT);
		}
	}

	return 0;
}

int card_words_of_wisdom(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			draw_cards(player, 2);
			draw_cards(1-player, 1);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_zombie_infestation(int player, int card, event_t event){
	/* Zombie Infestation	|1|B
	 * Enchantment
	 * Discard two cards: Put a 2/2 |Sblack Zombie creature token onto the battlefield. */

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && hand_count[player] > 1 ){
		return has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			multidiscard(player, 2, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_ZOMBIE);
	}

	return global_enchantment(player, card, event);
}

int card_zoologist(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			show_deck( player, deck, 1, "Here's the first card of deck", 0, 0x7375B0 );
			if( is_what(-1, deck[0], TYPE_CREATURE) ){
				if( ! check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE) ){
					put_into_play_a_card_from_deck(player, player, 0);
				}
			}
			else{
				mill(player, 1);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 3, 0, 0, 1, 0, 0, 0, 0, 0);
}


