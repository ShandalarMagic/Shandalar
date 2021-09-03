#include "manalink.h"

// Cards - Portal
int card_balance_of_power(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = hand_count[instance->targets[0].player]-hand_count[player];
			if( amount > 0 ){
				draw_cards(player, amount);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

// bee sting --> shock

int card_blaze(int player, int card, event_t event){
	// Original code : 0x4049B0

	return generic_x_spell(player, card, event, TARGET_ZONE_CREATURE_OR_PLAYER, 0, 1);
}

int card_breath_of_life(int player, int card, event_t event){
	// 0x41f9f0

	/* Breath of Life	|3|W
	 * Sorcery
	 * Return target creature card from your graveyard to the battlefield. */

	/* False Defeat	|3|W
	 * Sorcery
	 * Return target creature card from your graveyard to the battlefield. */

	/* Resurrection	|2|W|W
	 * Sorcery
	 * Return target creature card from your graveyard to the battlefield. */

	/* Zombify	|3|B
	 * Sorcery
	 * Return target creature card from your graveyard to the battlefield. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card.");
	this_test.ai_selection_mode = AI_MAX_CMC;

	if ( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &this_test);
}

int card_charging_paladin(int player, int card, event_t event){
	/* Charging Paladin	|2|W	0x200ec79
	 * Creature - Human Knight 2/2
	 * Whenever ~ attacks, it gets +0/+3 until end of turn. */

	return when_attacks_pump_self(player, card, event, 0,3);
}

// cloud dragon --> cloud sprite

// cloud pirates --> cloud sprite

int card_command_of_unsummoning(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && current_turn != player && current_phase == PHASE_BEFORE_BLOCKING){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			instance->info_slot = 1;
			if( can_target(&td) ){
				if( new_pick_target(&td, "TARGET_CREATURE", 1, 0) ){
					instance->info_slot++;
				}
			}
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				bounce_permanent(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cruel_fate(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		return would_valid_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = 1-player;
		instance->number_of_targets = 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int t_player = instance->targets[0].player;
			int number = 5;
			int *deck = deck_ptr[t_player];
			if( number > count_deck(t_player) ){
				 number = count_deck(t_player);
			}
			if( number > 0 ){
				int selected = -1;
				while( selected == -1 ){
						selected = show_deck( player, deck, number, "Pick a card", 0, 0x7375B0 );
				}
				int card_added = add_card_to_hand(t_player, deck[selected]);
				remove_card_from_deck(t_player, selected);
				put_on_top_of_deck(t_player, card_added);
				mill(t_player, 1);
				number--;
				if( number > 0 ){
					rearrange_top_x(t_player, player, number);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cruel_tutor(int player, int card, event_t event){
	// Also code for Vampiric Tutor

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		lose_life(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int deep_wood_legacy(int player, int card, event_t event)
{
	card_instance_t* damage = damage_being_prevented(event);
	if (damage
		&& (damage->targets[3].player & TYPE_CREATURE)
		&& is_attacking(damage->damage_source_player, damage->damage_source_card)
		&& damage->damage_target_player == player && damage->damage_target_card == -1 && !damage_is_to_planeswalker(damage))
		damage->info_slot = 0;

	if (event == EVENT_CLEANUP)
		kill_card(player, card, KILL_REMOVE);

	return 0;
}

int card_deep_wood(int player, int card, event_t event){

	// Cast ~ only during the declare attackers step and only if you've been attacked this step.
	// Prevent all damage that would be dealt to you this turn by attacking creatures.
	if( event == EVENT_CAN_CAST && current_turn != player && current_phase == PHASE_BEFORE_BLOCKING){
		int c, p = current_turn;
		for (c = 0; c < active_cards_count[p]; ++c){
			if (in_play_and_attacking(p, c) && !check_special_flags(p, c, SF_ATTACKING_PWALKER)){
				return 1;
			}
		}
		return 0;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &deep_wood_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_deja_vu(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_SORCERY) > 0 ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		char msg[100] = "Select a sorcery card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SORCERY, msg);
		int selected = new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0);
		if( selected == -1 ){
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
	}


	return 0;
}

int card_devastation2(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE | TYPE_LAND);
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}


	return 0;
}

int card_dread_reaper(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		lose_life(player, 5);
	}

	return 0;
}

int card_ebon_dragon(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		instance->targets[0].player = 1-player;
		instance->number_of_targets = 1;
		if( valid_target(&td) && hand_count[1-player] > 0 ){
			int choice = 0;
			if( player != AI ){
				choice = do_dialog(player, player, card, -1, -1, " Opponent discard\n Pass", 0);
			}
			if( choice == 0 ){
				discard(1-player, 0, player);
			}
		}
	}

	return 0;
}

int card_endless_cockroaches(int player, int card, event_t event)
{
	int owner, position;
	if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) && find_in_owners_graveyard(player, card, &owner, &position))
		{
			int iid = get_grave(owner)[position];
			obliterate_card_in_grave(owner, position);
			add_card_to_hand(owner, iid);
		}

	return 0;
}

int card_final_strike(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1 - player;
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = pick_creature_for_sacrifice(player, card, 0);
		if( result > -1 ){
			instance->info_slot = get_power(player, result);
			kill_card(player, result, KILL_SACRIFICE);
			instance->targets[0].player = 1 - player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, instance->info_slot, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_fire_dragon(int player, int card, event_t event)
{
	/* Fire Dragon	|6|R|R|R
	 * Creature - Dragon 6/6
	 * Flying
	 * When ~ enters the battlefield, it deals damage equal to the number of |H1Mountains you control to target creature. */
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	int amt = (trigger_condition == TRIGGER_COMES_INTO_PLAY && event == EVENT_RESOLVE_TRIGGER) ? count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN)) : 0;
	return cip_damage_creature(player, card, event, &td, "TARGET_CREATURE", amt);
}

int card_fire_imp(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	return cip_damage_creature(player, card, event, &td, "TARGET_CREATURE", 2);
}

int card_gift_of_estates(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( count_subtype(player, TYPE_LAND, -1) < count_subtype(1-player, TYPE_LAND, -1) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_PLAINS));
			this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_PLAINS);
			this_test.qty = 3;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			shuffle(player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_gravedigger(int player, int card, event_t event)
{
  // 0x4e3e90

  /* Gravedigger	|3|B
   * Creature - Zombie 2/2
   * When ~ enters the battlefield, you may return target creature card from your graveyard to your hand. */

  /* Cadaver Imp	|1|B|B
   * Creature - Imp 1/1
   * Flying
   * When ~ enters the battlefield, you may return target creature card from your graveyard to your hand. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select target creature card.");
	  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

  return 0;
}

static int harsh_justice_legacy(int player, int card, event_t event ){
	card_instance_t *instance = get_card_instance( player, card );
	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id ){
			if( source->damage_source_player == instance->targets[0].player && source->damage_source_card == instance->targets[0].card ){
				if( source->damage_target_player == instance->targets[1].player && source->damage_target_card == -1 ){
					int good = source->info_slot;
					if( good < 1 ){
						card_instance_t *trg = get_card_instance( instance->targets[0].player, instance->targets[0].card );
						good = trg->targets[16].player;
					}
					instance->targets[1].card = good;
				}
			}
		}
	}

	if( instance->targets[1].card > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				damage_player(instance->targets[0].player, instance->targets[1].card, instance->targets[0].player, instance->targets[0].card);
				instance->targets[1].card = 0;
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_harsh_justice(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;
	td.illegal_abilities = 0;

	if( event == EVENT_CAN_CAST && current_turn != player && current_phase == PHASE_BEFORE_BLOCKING){
		return can_target(&td);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int count = active_cards_count[current_turn]-1;
		while( count > -1 ){
				if( in_play(current_turn, count) && is_attacking(current_turn, count) ){
					int legacy = create_targetted_legacy_effect(player, card, &harsh_justice_legacy, current_turn, count);
					card_instance_t *instance = get_card_instance( player, legacy );
					instance->targets[1].player = player;
				}
				count--;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_jungle_lion(int player, int card, event_t event){
	cannot_block(player, card, event);
	return 0;
}

int card_kings_assassin(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_BEFORE_ATTACKERS|GAA_IN_YOUR_TURN,
									0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

static int last_chance_legacy(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);
	if( current_turn == player && eot_trigger(player, card, event) ){
		instance->targets[1].player--;
		if( instance->targets[1].player < 1 ){
			lose_the_game(player);
		}
	}
	return 0;
}

int card_last_chance(int player, int card, event_t event ){
	// also code for final fortune

	if( event == EVENT_RESOLVE_SPELL ){
		if( ! check_battlefield_for_id(1-player, CARD_ID_STRANGLEHOLD) ){
			int legacy = create_legacy_effect(player, card, &last_chance_legacy);
			card_instance_t *instance = get_card_instance(player, legacy);
			instance->targets[1].player = 2;
			return card_time_walk(player, card, event);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	else{
		return card_time_walk(player, card, event);
	}
	return 0;
}

int card_lava_axe(int player, int card, event_t event)
{
	/* Lava Axe	|4|R
	 * Sorcery
	 * ~ deals 5 damage to target player. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, 5);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

// mercenary knight --> hidden horror

int card_mind_rot(int player, int card, event_t event)
{
  /* Mind Rot	|2|B
   * Sorcery
   * Target player discards two cards. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		new_multidiscard(get_card_instance(player, card)->targets[0].player, 2, 0, player);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_monstrous_growth(int player, int card, event_t event){
	/*
	  Monstrous Growth |1|G
	  Sorcery
	  Target creature gets +4/+4 until end of turn.
	*/
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 4, 4, 0, 0);
}

int card_needle_storm(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		new_damage_all(player, card, 2, 4, 0, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}


	return 0;
}

int card_omen2(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		rearrange_top_x(player, player, 3);
		int choice = do_dialog(player, player, card, -1, -1, " Shuffle\n Pass", 1);
		if( choice == 0 ){
			shuffle(player);
		}
		draw_cards(player, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// owl familiar --> merfolk traders

int card_personal_tutor(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, 1, TYPE_SORCERY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// pillaging horde --> balduvian horde

int card_plant_elemental(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		if( ! sacrifice(player, card, player, 0, TYPE_LAND, 0, get_hacked_subtype(player, card, SUBTYPE_FOREST), 0, 0, 0, 0, 0, -1, 0) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_primeval_force(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int sac = 1;
		int forest = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		if( count_subtype(player, TYPE_LAND, forest) > 2 ){
			if( sacrifice(player, card, player, 0, TYPE_LAND, 0, forest, 0, 0, 0, 0, 0, -1, 0) ){
				impose_sacrifice(player, card, player, 2, TYPE_LAND, 0, forest, 0, 0, 0, 0, 0, -1, 0);
				sac = 0;
			}
		}
		if( sac == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_sacred_nectar(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			gain_life(player, 4);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_serpent_assassin(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

// spiritual guardian --> lone missionary

int card_sylvan_tutor(int player, int card, event_t event){
	// Also code of Worldly tutor

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_temporary_truce(int player, int card, event_t event){
	/* Temporary Truce	|1|W
	 * Sorcery
	 * Each player may draw up to two cards. For each card less than two a player draws this way, that player gains 2 life. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int amount = 2;
			if( !IS_AI(i) ){
				amount = choose_a_number(i, "Draw how many cards?", 2);
				if( amount > 2 || amount < 0 ){
					amount = 2;
				}
			}
			else{
				while( life[i] < 6 && amount > 1 ){
					amount--;
				}
				if( count_deck(i) <= amount ){
					amount = 0;
				}
			}
			draw_cards(i, amount);
			gain_life(i, 4-(amount*2));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_thing_from_the_deep(int player, int card, event_t event)
{
	// Whenever ~ attacks, sacrifice it unless you sacrifice |Han Island.
	if (declare_attackers_trigger(player, card, event, 0, player, card)
		&& !player_sacrifices_a_hacked_land(player, card, player, SUBTYPE_ISLAND, 0))
		kill_card(player, card, KILL_SACRIFICE);

	return 0;
}

int card_wicked_pact(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && target_available(player, card, &td) > 1){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			new_pick_target(&td, "TARGET_CREATURE", 1, 1);
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		int result = 0;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
				result++;
			}
		}
		if( result > 0 ){
			lose_life(player, 5);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_wood_elves(int player, int card, event_t event){
	/*
	  Wood Elves |2|G
	  Creature - Elf Scout 1/1
	  When Wood Elves enters the battlefield, search your library for a Forest card and put that card onto the battlefield. Then shuffle your library.
	*/
	if( comes_into_play(player, card, event) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_LAND, "Select a Forest card.");
		test.subtype = SUBTYPE_FOREST;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &test);
	}

	return 0;
}

// Cards - Portal 2

int card_abyssal_nightstalker(int player, int card, event_t event){

	if (event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) && is_unblocked(player, card)){
		discard(1-player, 0, player);
	}

	return 0;
}

// alaborn grenadier --> serra angel

// alaborn musketeer -> vanilla


int card_alaborn_zealot(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS && blocking(player, card, event) ){
		card_instance_t *instance = get_card_instance( player, card );
		kill_card(1-player, instance->blocking, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_alluring_scent(int player, int card, event_t event){
	// alluring scent
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_LURE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ancient_craving(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 3);
		lose_life(player, 3);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_angel_of_mercy(int player, int card, event_t event){
	return cip_lifegain(player, card, event, 3);
}

// brimstone dragon --> raging goblin

// brutal nightstalker --> ebon dragon

int card_cruel_edict(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		instance->number_of_targets = 1;
		return would_valid_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = 1-player;
		instance->number_of_targets = 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dakmor_plague(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		new_damage_all(player, card, 2, 3, NDA_PLAYER_TOO | NDA_ALL_CREATURES, NULL);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dakmor_sorceress(int player, int card, event_t event){

	if( event == EVENT_POWER && affect_me(player, card) ){
		event_result+=count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_SWAMP));
	}

	return 0;
}

// deathcoil wurm --> thorn elemental

int card_denizen_of_the_deep(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		new_manipulate_all(player, card, player, &this_test, ACT_BOUNCE);
	}

	return 0;
}

// false summoning --> remove soul

int card_festival_of_trokin(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, count_subtype(player, TYPE_CREATURE, -1)*2);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_foul_spirit(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		impose_sacrifice(player, card, player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_goblin_firestarter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET+GAA_BEFORE_ATTACKERS+GAA_IN_YOUR_TURN,
									 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_goblin_general(int player, int card, event_t event)
{
	// Whenever ~ attacks, Goblin creatures you control get +1/+1 until end of turn.
	if (declare_attackers_trigger(player, card, event, 0, player, card))
		pump_subtype_until_eot(player, card, player, SUBTYPE_GOBLIN, 1,1, 0,0);

	if ((event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST) && current_turn == player && current_phase <= PHASE_DECLARE_ATTACKERS)
		return 1;

	return 0;
}

int card_goblin_lore(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
		draw_cards(player, 4);
		multidiscard(player, 3, DISC_RANDOM);
		kill_card(player, card, KILL_BURY);
	}
	return 0;
}

int card_lurking_nightstalker(int player, int card, event_t event)
{
	/* Borderland Marauder	|1|R	0x200968e
	 * Creature - Human Warrior 1/2
	 * Whenever ~ attacks, it gets +2/+0 until end of turn. */
	/* Brazen Wolves	|2|R
	 * Creature - Wolf 2/3
	 * Whenever ~ attacks, it gets +2/+0 until end of turn. */
	/* Charging Bandits	|4|B
	 * Creature - Human Rogue 3/3
	 * Whenever ~ attacks, it gets +2/+0 until end of turn. */
	/* Hollow Dogs	|4|B
	 * Creature - Zombie Hound 3/3
	 * Whenever ~ attacks, it gets +2/+0 until end of turn. */
	/* Lurking Nightstalker	|B|B
	 * Creature - Nightstalker 1/1
	 * Whenever ~ attacks, it gets +2/+0 until end of turn. */
	/* Ravenous Skirge	|2|B
	 * Creature - Imp 1/1
	 * Flying
	 * Whenever ~ attacks, it gets +2/+0 until end of turn. */
	/* Vicious Kavu	|1|B|R
	 * Creature - Kavu 2/2
	 * Whenever ~ attacks, it gets +2/+0 until end of turn. */
	/* Wei Ambush Force	|1|B
	 * Creature - Human Soldier 1/1
	 * Whenever ~ attacks, it gets +2/+0 until end of turn. */

	return when_attacks_pump_self(player, card, event, 2,0);
}

int card_mystic_denial(int player, int card, event_t event){
	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_CREATURE | TYPE_SORCERY);

	return counterspell(player, card, event, &td, 0);
}

int card_nightstalker_engine(int player, int card, event_t event){
	/* Nightstalker Engine	|4|B
	 * Creature - Nightstalker 100/3
	 * ~'s power is equal to the number of creature cards in your graveyard. */

	if( event == EVENT_POWER && affect_me(player, card) && player != -1){
		event_result += count_graveyard_by_type(player, TYPE_CREATURE);
	}

	return 0;
}

int card_norwood_priestess(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, get_sleighted_color_text(player, card, "Select a %s creature card.", COLOR_GREEN));
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_BEFORE_ATTACKERS|GAA_IN_YOUR_TURN, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_ogre_arsonist(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allow_cancel = 0;

	if(target_available(player, card, &td) && comes_into_play(player, card, event) ){
		select_target(player, card, &td, "Select target land", NULL);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}
	return 0;
}

int card_predatory_nightstalker(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		if( comes_into_play_mode(player, card, event, would_validate_arbitrary_target(&td, 1-player, -1) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}

int card_prowling_nightstalker(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY ){
		if( attacking_card_controller == player && attacking_card == card && !(get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_BLACK)) ){
			event_result = 1;
		}
	}

	return 0;
}

// raiding nightstalker --> vanilla

int card_rain_of_daggers(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		instance->number_of_targets = 1;
		return would_valid_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = 1-player;
		instance->number_of_targets = 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			int life_loss = new_manipulate_all(player, card, 1-player, &this_test, KILL_DESTROY)*2;
			lose_life(player, life_loss);
		}
		kill_card(player, card, KILL_DESTROY);

	}

	return 0;
}

int card_return_of_the_nightstalkers(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		reanimate_all(player, card, player, TYPE_PERMANENT, 0, SUBTYPE_NIGHTSTALKER, 0, 0, 0, 0, 0, -1, 0, REANIMATE_DEFAULT);
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_SWAMP);
		new_manipulate_all(player, card, player, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);

	}

	return 0;
}

int card_righteous_charge(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 2, 2, 0, 0);
		kill_card(player, card, KILL_DESTROY);

	}

	return 0;
}

int card_righteous_fury(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_TAPPED;
		int life_gain = new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY)*2;
		gain_life(player, life_gain);
		kill_card(player, card, KILL_DESTROY);

	}

	return 0;
}

int card_sea_drake(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		int i;
		for(i = 0; i < 2; i++ ){
			if( can_target(&td) ){
				pick_target(&td, "TARGET_LAND");
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_swarm_of_rats(int player, int card, event_t event){
	/* Swarm of Rats	|1|B
	 * Creature - Rat 100/1
	 * ~'s power is equal to the number of Rats you control. */

	if( event == EVENT_POWER && affect_me(player, card) && ! is_humiliated(player, card) && player != -1){
		event_result += count_subtype(player, TYPE_PERMANENT, SUBTYPE_RAT);
	}

	return 0;
}

int card_sylvan_yeti(int player, int card, event_t event)
{
  /* Sylvan Yeti	|2|G|G
   * Creature - Yeti 100/4
   * ~'s power is equal to the number of cards in your hand. */

  if (event == EVENT_POWER && affect_me(player, card) && !is_humiliated(player, card) && player != -1)
	event_result += hand_count[player];

  return 0;
}

int card_temple_acolyte(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		gain_life(player, 3);
	}

	return 0;
}

// temporal manipulation --> time walk

int card_vampiric_spirit(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		lose_life(player, 4);
	}

	return 0;
}

// Portal 3 Kingdoms

// Cards

// alert shu infantry

// ambition's cost --> ancient craving

int card_borrowing_the_east_wind(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
				if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && check_for_special_ability(i, count, SP_KEYWORD_HORSEMANSHIP) ){
					damage_creature(i, count, instance->info_slot, player, card);
				}
				count--;
			}
			damage_player(i, instance->info_slot, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_burning_of_xinye(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td_player;
	default_target_definition(player, card, &td_player, 0);
	td_player.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td_player) ){
			int i;
			for(i=0; i<2; i++){
				int p = i == 0 ? player : 1-player;

				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_LAND);
				td.allowed_controller = p;
				td.who_chooses = p;
				td.illegal_abilities = 0;
				td.allow_cancel = 0;

				int selected = instance->number_of_targets = 0;
				while( selected < 4 && can_target(&td) ){
					if( new_pick_target(&td, "Select a land you control.", selected, GS_LITERAL_PROMPT) ){
						state_untargettable(instance->targets[selected].player, instance->targets[selected].card, 1);
						selected++;
					}
				}

				int t;
				for(t=0; t<instance->number_of_targets; t++){
					action_on_target(player, card, t, KILL_DESTROY);
				}
			}

			APNAP(p, { new_damage_all(player, card, p, 4, NDA_ALL_CREATURES, NULL); };);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td_player, NULL, 1, NULL);
}

int card_cao_cao(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			new_multidiscard(instance->targets[0].player, 2, 0, player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_ONLY_TARGET_OPPONENT|GAA_BEFORE_ATTACKERS|GAA_IN_YOUR_TURN,
									 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_cao_ren_wei_commander(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	horsemanship2(player, card, event);

	if( comes_into_play(player, card, event) ){
		lose_life(player, 3);
	}

	return 0;
}

// Capture of Jingzhou --> time walk

// control of the cour --> goblin lore

// corrupt cour official --> ravenous rats

int card_diaochan_artful_beauty(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = 1 - player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.who_chooses = 1 - player;
	td1.preferred_controller = player;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS &&
			has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
			){
			return target_available(player, card, &td) >= 2;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			if (player == AI){
				do_dialog(HUMAN, player, card, instance->targets[0].player, instance->targets[0].card, "", 0);
			}
			if( new_pick_target(&td1, "TARGET_CREATURE", 1, 0) ){
				tap_card(player, card);
				if (player != AI){
					do_dialog(HUMAN, player, card, instance->targets[1].player, instance->targets[1].card, "", 0);
				}
			}
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for (i = 0; i < 2; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_dong_zhou_the_tyrant(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) && can_target(&td1) ){
		if( pick_target(&td1, "TARGET_CREATURE") ){
			damage_player(instance->targets[0].player, get_power(instance->targets[0].player, instance->targets[0].card),
						  instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

// false defeat --> breath of life

// famine --> dakmor plague

// fire hambush --> volcanic hammer

// fire bowman --> goblin firestarter

int card_guan_yus_1000_li_march(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_TAPPED;
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);

	}

	return 0;
}

int card_guan_yu_sainted_warrior(int player, int card, event_t event){
	// Guan Yu, Sainted Warrior
	check_legend_rule(player, card, event);

	horsemanship2(player, card, event);

	// Owner makes the choice of whether to activate, which is why the card's oracle text isn't "When ~ dies, ..."
	int owner, position;
	if (this_dies_trigger_for_owner(player, card, event, RESOLVE_TRIGGER_AI(get_owner(player, card)))
		&& find_in_owners_graveyard(player, card, &owner, &position))
		from_graveyard_to_deck(owner, position, 3);

	return 0;
}

// heavy fog --> deep wood

int card_hua_tou(int player, int card, event_t event){
	/* Hua Tuo, Honored Physician	|1|G|G
	 * Legendary Creature - Human 1/2
	 * |T: Put target creature card from your graveyard on top of your library. Activate this ability only during your turn, before attackers are declared. */

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, MANACOST0) ){
			if( current_phase < PHASE_DECLARE_ATTACKERS && current_turn == player){
				if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
					return ! graveyard_has_shroud(2);
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			if( new_select_target_from_grave(player, card, player, 0, AI_MAX_CMC, &this_test, 0) != -1 ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			from_graveyard_to_deck(player, selected, 1);
		}

	}

	return 0;
}

int card_hunting_cheetah(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT|DDBM_TRIGGER_OPTIONAL) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_FOREST));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

int card_imperial_edict(int player, int card, event_t event){

	target_definition_t td_player;
	default_target_definition(player, card, &td_player, TYPE_CREATURE);
	td_player.zone = TARGET_ZONE_PLAYERS;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		return would_valid_target(&td_player);
	}

	if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
	}

	if (event == EVENT_RESOLVE_SPELL){
		if (valid_target(&td_player)){
			target_definition_t td_creature;
			default_target_definition(player, card, &td_creature, TYPE_CREATURE);
			td_creature.allowed_controller = 1 - player;
			td_creature.preferred_controller = 1 - player;
			td_creature.who_chooses = 1 - player;
			td_creature.illegal_abilities = 0;
			td_creature.allow_cancel = 0;

			instance->number_of_targets = 0;
			if (can_target(&td_creature) && pick_target(&td_creature, "TARGET_CREATURE")){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_imperial_recruiter(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		char msg[100] = "Select a creature with Power 2 or less";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.power = 3;
		this_test.power_flag = 3;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}
	return 0;
}

int card_kongming_sleeping_dragon(int player, int card, event_t event)
{
	/* Kongming, "Sleeping Dragon"	|2|W|W
	 * Legendary Creature - Human Advisor 2/2
	 * Other creatures you control get +1/+1. */

  check_legend_rule(player, card, event);
  boost_subtype(player, card, event, -1, 1,1, 0,0, BCT_CONTROLLER_ONLY);
  return 0;
}

int card_kongmings_contraptions(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, instance->parent_card);
		}
	}

	if (event == EVENT_CAN_ACTIVATE && current_phase != PHASE_BEFORE_BLOCKING){
		return 0;
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_IN_OPPONENT_TURN,
									0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_lady_sun(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			bounce_permanent(player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_NOT_ME_AS_TARGET|GAA_BEFORE_ATTACKERS|GAA_IN_YOUR_TURN,
									0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_lady_zhurong_warrior_queen(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	horsemanship2(player, card, event);

	return 0;
}

int card_liu_bei_lord_of_shu(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	horsemanship2(player, card, event);

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		if( check_battlefield_for_id(player, CARD_ID_GUAN_YU_SAINTED_WARRIOR) || check_battlefield_for_id(player, CARD_ID_ZHANG_FEI_FIERCE_WARRIOR) ){
			event_result+=2;
		}
	}

	return 0;
}

int card_loyal_retainers(int player, int card, event_t event){
	/* Loyal Retainers	|2|W
	 * Creature - Human Advisor 1/1
	 * Sacrifice ~: Return target legendary creature card from your graveyard to the battlefield. Activate this ability only during your turn, before attackers are declared. */

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( current_phase < PHASE_DECLARE_ATTACKERS && current_turn == player){
			if( any_in_graveyard_by_subtype(player, SUBTYPE_LEGEND) > 0 ){
				return ! graveyard_has_shroud(2);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select target legendary creature card.");
		this_test.subtype = SUBTYPE_LEGEND;
		int selected = new_select_target_from_grave(player, card, player, 0, AI_MAX_CMC, &this_test, 0);
		if( selected == -1 ){
			spell_fizzled = 1;
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}

	}

	return 0;
}

int card_lu_bu_master_at_arms(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	horsemanship2(player, card, event);

	haste(player, card, event);

	return 0;
}

// lu meng wu general --> lady zhurong

int card_lu_su_wu_advisor(int player, int card, event_t event){//UNUSEDCARD

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_BEFORE_ATTACKERS|GAA_IN_YOUR_TURN, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_lu_xun_scholar_general(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	horsemanship2(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_TRIGGER_OPTIONAL+DDBM_MUST_DAMAGE_OPPONENT) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_ma_chao_western_warrior(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	horsemanship2(player, card, event);

	// Whenever ~ attacks alone, it's unblockable this combat.
	if (declare_attackers_trigger(player, card, event, DAT_ATTACKS_ALONE, player, card)){
		int leg = pump_ability_until_eot(player, card, player, card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		if (leg != -1){
			card_instance_t* legacy = get_card_instance(player, leg);
			legacy->targets[3].player = PAUE_END_AT_END_OF_COMBAT;
		}
	}

	return 0;
}

int card_meng_huo_barbarian_king(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
		&& affected_card_controller == player && affected_card != card
		&& in_play(player, card)
		&& (get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_GREEN))){
		event_result += 1;
	}

	return 0;
}

// mountain bandit --> raging goblin

int card_overwhelming_forces(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		instance->number_of_targets = 1;
		return would_valid_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->targets[0].player = 1-player;
			instance->number_of_targets = 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_CREATURE);
				int cd = new_manipulate_all(player, card, 1-player, &this_test, KILL_DESTROY);
				draw_cards(player, cd);
			}
			kill_card(player, card, KILL_DESTROY);

	}

	return 0;
}

int card_pang_tong_young_phoenix(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_BEFORE_ATTACKERS|GAA_IN_YOUR_TURN,
									0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

// Peach Garden Oath --> Festival of Trokin

// Preemptive Strike --> Remove Soul

// Ravaging Horde --> Ogre Arsonist

int card_riding_red_hare(int player, int card, event_t event){
	/* Riding Red Hare	|2|W
	 * Sorcery
	 * Target creature gets +3/+3 and gains horsemanship until end of turn. */
	if (!IS_CASTING(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	return vanilla_pump(player, card, event, &td, 3, 3, 0, SP_KEYWORD_HORSEMANSHIP);
}

int card_riding_the_dilu_horse(int player, int card, event_t event){
	/* Riding the Dilu Horse	|2|G
	 * Sorcery
	 * Target creature gets +2/+2 and gains horsemanship. */
	if (!IS_CASTING(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	return vanilla_pump(player, card, event, &td, 2, 2, 0, SP_KEYWORD_HORSEMANSHIP | SP_KEYWORD_DOES_NOT_END_AT_EOT);
}

int card_rolling_earthquake(int player, int card, event_t event)
{
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && !check_for_special_ability(i, count, SP_KEYWORD_HORSEMANSHIP) ){
						damage_creature(i, count, instance->info_slot, player, card);
					}
					count--;
				}
				damage_player(i, instance->info_slot, player, card);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// sage's knowledge --> deja vu

int card_shu_cavalry(int player, int card, event_t event){
	horsemanship2(player, card, event);
	return 0;
}

// shu elite companions --> shu cavalry

// shu foot soldiers --> vanilla

int card_shu_general(int player, int card, event_t event){
	vigilance(player, card, event);
	horsemanship2(player, card, event);
	return 0;
}

// shu grain caravan --> lone missionary

int card_sima_yi_wei_field_general(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	return card_dakmor_sorceress(player, card, event);
}

int card_sun_ce_young_conquerer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	horsemanship2(player, card, event);

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_sun_quan_lord_of_wu(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if (event == EVENT_ABILITIES
		&& in_play(player, card)
		&& affected_card_controller == player
		&& !is_humiliated(player, card)
		&& is_what(affected_card_controller, affected_card, TYPE_CREATURE)){
		horsemanship2(affected_card_controller, affected_card, event);
	}

	return 0;
}

// warrior's oath --> last chance

// wei ambush force --> lurking nightstalker

int card_wei_assassins(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		instance->targets[0].player = 1-player;
		instance->number_of_targets = 1;
		if( valid_target(&td) ){
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_CREATURE);
			td1.allowed_controller = 1-player;
			td1.preferred_controller = 1-player;
			td1.who_chooses = 1-player;
			td1.illegal_abilities = 0;

			if( can_target(&td1) && pick_target(&td1, "TARGET_CREATURE") ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

// wei elite companions --> shu cavalry

// wei infantry --> vanilla

int card_wei_night_riders(int player, int card, event_t event){

	horsemanship2(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT) ){
		discard(1-player, 0, player);
	}

	return 0;
}

// wei scout --> shu cavalry

// wei strike force --> shu cavalry

// wolf pack -->thorn elemental

// wu elite cavalry --> shu cavalry

// wu infantry --> vanilla

// wu light cavalry -->  shu cavalry

int card_xiahou_dun_the_one_eyed(int player, int card, event_t event){
	/* Xiahou Dun, the One-Eyed	|2|B|B
	 * Legendary Creature - Human Soldier 3/2
	 * Horsemanship
	 * Sacrifice ~: Return target |Sblack card from your graveyard to your hand. Activate this ability only during your turn, before attackers are declared. */

	horsemanship2(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( current_phase < PHASE_DECLARE_ATTACKERS && current_turn == player){
			if( count_graveyard_by_color(player, get_sleighted_color_test(player, card, COLOR_TEST_BLACK)) > 0 && ! graveyard_has_shroud(2) ){
				return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_BLACK));
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

		if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
	}

	return 0;
}

int card_xun_yu_wei_advisor(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_BEFORE_ATTACKERS|GAA_IN_YOUR_TURN,
									0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_zhang_fei_fierce_warrior(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	vigilance(player, card, event);
	horsemanship2(player, card, event);
	return 0;
}

int card_zhang_he_wei_general(int player, int card, event_t event)
{
	check_legend_rule(player, card, event);

	horsemanship2(player, card, event);

	// Whenever ~ attacks, each other creature you control gets +1/+0 until end of turn.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "");
		this_test.not_me = 1;
		pump_creatures_until_eot(player, card, player, 0, 1,0, 0,0, &this_test);
	}

	return 0;
}

int card_zhang_liao_hero_of_hefei(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT) ){
		discard(1-player, 0, player);
	}

	return 0;
}

int card_zhuge_jin_wu_strategist(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_BEFORE_ATTACKERS|GAA_IN_YOUR_TURN,
									0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_zodiac_dragon(int player, int card, event_t event)
{
  // Owner makes the choice of whether to activate, which is why the card's oracle text isn't "When ~ dies, ..."
  int owner, position;
  if (this_dies_trigger_for_owner(player, card, event, RESOLVE_TRIGGER_AI(get_owner(player, card)))
	  && find_in_owners_graveyard(player, card, &owner, &position))
	{
	  int iid = get_grave(owner)[position];
	  remove_card_from_grave(owner, position);
	  add_card_to_hand(owner, iid);
	}

  return 0;
}

// cards - Starter

int card_dakmor_ghoul(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		instance->targets[0].player = 1-player;
		instance->number_of_targets = 1;
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 2);
			gain_life(player, 2);
		}
	}

	return 0;
}

int card_devout_monk(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		gain_life(player, 1);
	}

   return 0;
}

int card_goblin_settler(int player, int card, event_t event){
	/*
	  Goblin Settler |3|R
	  Creature - Goblin 1/1
	  When Goblin Settler enters the battlefield, destroy target land.
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.allow_cancel = 0;


		if( can_target(&td) && pick_target(&td, "TARGET_LAND") ){
			action_on_target(player, card, 0, KILL_DESTROY);
			get_card_instance(player, card)->number_of_targets = 0;
		}
	}

	return 0;
}

int card_grim_tutor(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			lose_life(player, 3);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// loyal sentry -> alaborn_zealot

// pride of lions --> thorn elemental

int card_sleight_of_hand(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
		int *deck = deck_ptr[player];
		int selected = show_deck( player, deck_ptr[player], 2, "Pick a card to put in hand", 0, 0x7375B0 );
		if( selected == -1 ){ selected = 0; }
		add_card_to_hand(player, deck[selected] );
		remove_card_from_deck( player, selected );
		put_top_card_of_deck_to_bottom(player);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_thunder_dragon(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		this_test.keyword_flag = 1;
		new_damage_all(player, card, 2, 3, 0, &this_test);
	}

	return 0;
}

int card_tidings(int player, int card, event_t event){
	/*
	  Tidings |3|U|U
	  Sorcery
	  Draw four cards.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 4);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

// veteran cavalier --> serra angel




