#include "manalink.h"

// Functions
static void store_mana_paid_to_play_this(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			card_instance_t *instance = get_card_instance(player, card);
			instance->targets[3].card = 0;
			int i;
			for(i=0; i<7; i++)
				instance->targets[3].card += mana_paid[i];
		}
	}
}

// Cards

// black
int card_baleful_force(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		draw_cards(player, 1);
		lose_life(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_curse_of_shallow_graves(int player, int card, event_t event)
{
	card_instance_t* instance = in_play(player, card);
	if (instance && declare_attackers_trigger(player, card, event, DAT_ATTACKS_PLAYER, 1-instance->targets[4].player, -1) &&
		instance->targets[1].player != 66
	  ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		token.action = TOKEN_ACTION_TAPPED;
		generate_token(&token);
		instance->targets[1].player = 66;
	}

	if( end_of_combat_trigger(player, card, event, 2) )
		instance->targets[1].player = 0;

	// Enchant player
	return curse(player, card, event);
}

int card_tempt_with_immortality(int player, int card, event_t event){
	/* Tempt with Immortality	|4|B
	 * Sorcery
	 * Tempting offer - Return a creature card from your graveyard to the battlefield. Each opponent may return a creature card from his or her graveyard to the battlefield. For each player who does, return a creature card from your graveyard to the battlefield. */

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
		if( new_global_tutor(1-player, 1-player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test) != -1 ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
				new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
			}
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_price_of_knowledge(int player, int card, event_t event){

	if( event == EVENT_MAX_HAND_SIZE ){
		event_result = 99;
	}

	upkeep_trigger_ability(player, card, event, 1-player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( hand_count[1-player] > 0 ){
			damage_player(1-player, hand_count[1-player], player, card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_fell_shepherd(int player, int card, event_t event)
{
  /* Fell Shepherd	|5|B|B
   * Creature - Avatar 8/6
   * Whenever ~ deals combat damage to a player, you may return to your hand all creature cards that were put into your graveyard from the battlefield this
   * turn.
   * |B, Sacrifice another creature: Target creature gets -2/-2 until end of turn. */

  if (damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER|DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_TRIGGER_OPTIONAL))
	return_all_dead_this_turn_to_hand(player, TYPE_CREATURE);

  if (!IS_ACTIVATING(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  card_instance_t* instance = get_card_instance(player, card);

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select another creature to sacrifice.");
  test.not_me = 1;

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_ACTIVATE(player, card, MANACOST_B(1)) && new_can_sacrifice_as_cost(player, card, &test);

  if (event == EVENT_ACTIVATE)
	{
	  int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
	  if (!sac)
		{
		  cancel = 1;
		  return 0;
		}

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
	  else
		{
		  cancel = 1;
		  state_untargettable(BYTE2(sac), BYTE3(sac), 0);
		}
	}

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -2, -2);

  return 0;
}

int card_hooded_horror(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( count_subtype(1-current_turn, TYPE_CREATURE, -1) >= count_subtype(current_turn, TYPE_CREATURE, -1) ){
				event_result = 1;
			}
		}
	}

	return 0;
}

int card_ophiomancer(int player, int card, event_t event){
	/* Ophiomancer	|2|B
	 * Creature - Human Shaman 2/2
	 * At the beginning of each upkeep, if you control no Snakes, put a 1/1 |Sblack Snake creature token with deathtouch onto the battlefield. */

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( count_subtype(player, TYPE_PERMANENT, SUBTYPE_SNAKE) < 1 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SNAKE, &token);
			token.pow = 1;
			token.tou = 1;
			token.color_forced = COLOR_TEST_BLACK;
			token.s_key_plus = SP_KEYWORD_DEATHTOUCH;
			generate_token(&token);
		}
	}

	return 0;
}

int card_toxic_deluge(int player, int card, event_t event){
	/* Toxic Deluge	|2|B
	 * Sorcery
	 * As an additional cost to cast ~, pay X life.
	 * All creatures get -X/-X until end of turn. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( basic_spell(player, card, event) ){
			return can_pay_life_as_cost_for_spells_or_activated_abilities(player, 1);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! is_token(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			int ltp = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_MAX_TOU, &this_test);
			while( life[player]-ltp < 6 ){
					ltp--;
			}
			if( player == HUMAN ){
				ltp = choose_a_number(player, "Pay how much life?", life[player]);
			}
			if( ltp < 1 || ltp > life[player] ){
				spell_fizzled = 1;
			}
			else{
				lose_life(player, ltp);
				instance->info_slot = ltp;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {pump_subtype_until_eot(player, card, p, -1, -instance->info_slot, -instance->info_slot, 0, 0);};);
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

// blue
int card_curse_of_inertia(int player, int card, event_t event)
{
	// Whenever a player attacks enchanted player with one or more creatures, that attacking player may tap or untap target permanent of his or her choice.
	card_instance_t* instance = in_play(player, card);
	if (instance && declare_attackers_trigger(player, card, event, DAT_ATTACKS_PLAYER, 1-instance->targets[4].player, -1)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.preferred_controller = current_turn;
		td.who_chooses = current_turn;

		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			instance->number_of_targets = 1;
			int choice = 1;
			if( current_turn == HUMAN ){
				choice = do_dialog(current_turn, current_turn, card, -1, -1, " Tap\n Untap", 0);
			}
			if( choice == 0 ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	// Enchant player
	return curse(player, card, event);
}

int card_diviner_spirit(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_REPORT_DAMAGE_DEALT) ){
		draw_cards(player, instance->targets[16].player);
		draw_cards(1-player, instance->targets[16].player);
		instance->targets[16].player = 0;
	}

	return 0;
}

int card_djinn_of_infinite_deceits(int player, int card, event_t event){

	target_definition_t td_any_nonlegend;
	default_target_definition(player, card, &td_any_nonlegend, TYPE_CREATURE);
	td_any_nonlegend.allowed_controller = 2;
	td_any_nonlegend.preferred_controller = 2;
	td_any_nonlegend.required_subtype = SUBTYPE_LEGEND;
	td_any_nonlegend.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	target_definition_t td_opp_nonlegend;
	default_target_definition(player, card, &td_opp_nonlegend, TYPE_CREATURE);
	td_opp_nonlegend.allowed_controller = 1-player;
	td_opp_nonlegend.required_subtype = SUBTYPE_LEGEND;
	td_opp_nonlegend.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	target_definition_t td_own_nonlegend;
	default_target_definition(player, card, &td_own_nonlegend, TYPE_CREATURE);
	td_own_nonlegend.allowed_controller = player;
	td_own_nonlegend.preferred_controller = player;
	td_own_nonlegend.required_subtype = SUBTYPE_LEGEND;
	td_own_nonlegend.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	target_definition_t *td, *td1;
	if (player == AI){
		td = &td_opp_nonlegend;
		td1 = &td_own_nonlegend;
	} else {
		td = td1 = &td_any_nonlegend;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && (current_phase <= PHASE_MAIN1 || current_phase >= PHASE_MAIN2) && can_target(td) && can_target(td1) ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
			&& pick_target(td, "TARGET_NONLEGENDARY_CREATURE")
			&& new_pick_target(td1, "TARGET_NONLEGENDARY_CREATURE", 1, 1)
		  ){
			instance->number_of_targets = 2;
			tap_card(player, card);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, td, 0) && validate_target(player, card, td1, 1) ){
			exchange_control_of_target_permanents(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
												  instance->targets[1].player, instance->targets[1].card);
		}
	}

	return 0;
}

// illusionist's gambit --> skipped

int card_order_of_succession(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.who_chooses = 1-player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.preferred_controller = 1-player;
	td1.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( select_target(player, card, &td1, "Select target creature opponent controls.", &(instance->targets[0])) ){
			if( ! select_target(player, card, &td, "Select target creature opponent controls.", &(instance->targets[1])) ){
				spell_fizzled = 1;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 1) && validate_target(player, card, &td1, 0) ){
			exchange_control_of_target_permanents(player, card, instance->targets[0].player, instance->targets[0].card,
													instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_true_name_nemesis(int player, int card, event_t event){

	hexproof(player, card, event);

	unblockable(player, card, event);

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player && damage->damage_source_player != player &&
				damage->info_slot > 0
			  ){
				damage->info_slot = 0 ;
			}
		}
	}
	return 0;
}

int card_tempt_with_reflections(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;
	td. allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			token_generation_t token;
			copy_token_definition(player, card, &token, instance->targets[0].player, instance->targets[0].card);
			generate_token(&token);
			int choice = do_dialog(1-player, instance->targets[0].player, instance->targets[0].card, -1, -1, " Get a copy of this creature\n Pass", 0);
			if( choice == 0 ){
				token.t_player = 1-player;
				generate_token(&token);
				token.t_player = player;
				generate_token(&token);
			}
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_tidal_force(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			instance->number_of_targets = 1;
			int choice = 0;
			if( player == HUMAN ){
				choice = do_dialog(player, player, card, -1, -1, " Tap\n Untap", 0);
			}
			if( choice == 0 ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

// green
int card_bane_of_progress(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT | TYPE_ENCHANTMENT);
		this_test.type_flag = F1_NO_PWALKER;

		int result = new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);

		add_1_1_counters(player, card, result);

	}

	return 0;
}

int card_curse_of_predation(int player, int card, event_t event)
{
  // Whenever a creature attacks enchanted player, put a +1/+1 counter on it.
  card_instance_t* instance = in_play(player, card);
  int amt;
  /* Look at original targets[0] rather than preserved targets[4], since the former doesn't change on this card but DAT_TRACK overwrites the latter.
   * (instance->damage_target_player/card seem like they'd work just fine.) */
  if (instance && (amt = declare_attackers_trigger(player, card, event, DAT_ATTACKS_PLAYER | DAT_TRACK, 1-instance->targets[0].player, -1)))
	{
	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
	  for (--amt; amt >= 0; --amt)
		if (in_play(current_turn, attackers[amt]))
		  add_1_1_counter(current_turn, attackers[amt]);
	}

  // Enchant player
  return curse(player, card, event);
}

int card_naya_soulbeast(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && ! check_special_flags(player, card, SF_NOT_CAST) &&
		! is_token(player, card)
	  ){
		int i;
		instance->info_slot = 0;
		for(i=0; i<2; i++){
			int *deck = deck_ptr[i];
			if( deck[0] != -1 ){
				if( i == HUMAN ){
					show_deck( HUMAN, deck, 1, "Here's the top card of your deck", 0, 0x7375B0 );
				}
				else{
					show_deck( HUMAN, deck, 1, "Here's the top card of your opponent's deck", 0, 0x7375B0 );
				}
				instance->info_slot += get_cmc_by_internal_id(deck[0]);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		add_1_1_counters(player, card, instance->info_slot);
	}

	return 0;
}

// primal vigor --> doubling season

int card_restore2(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.illegal_abilities = 0;

	char msg[100] = "Select a land card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, msg);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && (count_graveyard_by_type(player, TYPE_LAND) > 0 || count_graveyard_by_type(1-player, TYPE_LAND)) ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = 1-player;
		if( count_graveyard_by_type(1-player, TYPE_LAND) > 0 ){
			if( count_graveyard_by_type(player, TYPE_LAND) > 0 ){
				pick_target(&td, "TARGET_PLAYER");
			}
		}
		else{
			instance->targets[0].player = player;
		}
		if( new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_CMC, &this_test, 1) == -1 ){
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_SPELL){
		int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_spawning_grounds(int player, int card, event_t event){
	/* Spawning Grounds	|6|G|G
	 * Enchantment - Aura
	 * Enchant land
	 * Enchanted land has "|T: Put a 5/5 |Sgreen Beast creature token with trample onto the battlefield." */

	if (!IS_CASTING(player, card, event) && !IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, LAND);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
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

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(instance->damage_target_player, instance->damage_target_card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}
		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(instance->damage_target_player, instance->damage_target_card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_BEAST, &token);
			token.pow = 5;
			token.tou = 5;
			token.key_plus = KEYWORD_TRAMPLE;
			generate_token(&token);
		}
	}

	return 0;
}

int card_tempt_with_discovery(int player, int card, event_t event){
	/* Tempt with Discovery	|3|G
	 * Sorcery
	 * Tempting offer - Search your library for a land card and put it onto the battlefield. Each opponent may search his or her library for a land card and put it onto the battlefield. For each opponent who searches a library this way, search your library for a land card and put it onto the battlefield. Then each player who searched a library this way shuffles it. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		if( new_global_tutor(1-player, 1-player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test) != -1 ){
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

// red
int card_curse_of_chaos(int player, int card, event_t event ){

	/* Whenever a player attacks enchanted player with one or more creatures, that attacking player may discard a card. If the player does, he or she draws a card. */
	card_instance_t* instance = in_play(player, card);
	if (instance && declare_attackers_trigger(player, card, event, DAT_ATTACKS_PLAYER, 1-instance->targets[4].player, -1)){
		test_definition_t this_test;
		new_default_test_definition(&this_test, 0, "Select a card to discard.");

		int selected = new_select_a_card(current_turn, current_turn, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
		if( selected != -1 ){
			discard_card(current_turn, selected);
			draw_cards(current_turn, 1);
		}
	}

	// Enchant player
	return curse(player, card, event);
}

int card_from_the_ashes(int player, int card, event_t event){

	/* From the Ashes	|3|R
	 * Sorcery
	 * Destroy all nonbasic lands. For each land destroyed this way, its controller may search his or her library for a basic land card and put it onto the
	 * battlefield. Then each player who searched his or her library this way shuffles it. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "");
		this_test.subtype = SUBTYPE_BASIC;
		this_test.subtype_flag = DOESNT_MATCH;

		int r[2];
		APNAP(p, r[p] = new_manipulate_all(player, card, p, &this_test, KILL_DESTROY););
		APNAP(p, {
			if( r[p] > 0 && do_dialog(p, player, card, -1, -1, " Search\n Decline", 0) == 0){
				tutor_basic_lands(p, TUTOR_PLAY, r[p]);
			}
		});

		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_sudden_demise(int player, int card, event_t event){

	/* Sudden Demise	|X|R
	 * Sorcery
	 * Choose a color. ~ deals X damage to each creature of the chosen color. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
		instance->targets[0].player = 1<<choose_a_color(player, get_deck_color(player, 1-player));
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot > 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.color = instance->targets[0].player;
			new_damage_all(player, card, 2, instance->info_slot, 0, &this_test);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_tempt_with_vengeance(int player, int card, event_t event){
	/* Tempt with Vengeance	|X|R
	 * Sorcery
	 * Tempting offer - Put X 1/1 |Sred Elemental creature tokens with haste onto the battlefield. Each opponent may put X 1/1 |Sred Elemental creature tokens with haste onto the battlefield. For each player who does, put X 1/1 |Sred Elemental creature tokens with haste onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot > 0 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
			token.pow = 1;
			token.tou = 1;
			token.s_key_plus = SP_KEYWORD_HASTE;
			token.color_forced = COLOR_TEST_RED;
			token.qty = instance->info_slot;

			generate_token(&token);
			char choice[100];
			scnprintf(choice, 100, "Get %d 1/1 Elemental token%s", instance->info_slot, instance->info_slot == 1 ? "" : "s");
			if (DIALOG(player, card, EVENT_ACTIVATE, DLG_NO_STORAGE, DLG_RANDOM, DLG_WHO_CHOOSES(1-player),
					   choice, 1, 2,
					   "Decline", 1, 1) == 1
			   ){
				token.t_player = 1-player;
				generate_token(&token);
				token.t_player = player;
				generate_token(&token);
			}
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_terra_ravager(int player, int card, event_t event)
{
  // Whenever ~ attacks, it gets +X/+0 until end of turn, where X is the number of lands defending player controls.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	pump_until_eot(player, card, player, card, count_subtype(1-player, TYPE_LAND, -1), 0);

  if (event == EVENT_POW_BOOST && current_turn == player && current_phase <= PHASE_DECLARE_ATTACKERS)
	return count_subtype(1-player, TYPE_LAND, -1);

  return 0;
}

// widespread panic --> doubling season

int card_witch_hunt(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(player, 4, player, card);
	}

	if( current_turn == player && eot_trigger(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;

		if( valid_target(&td) ){
			int fake = add_card_to_hand(1-player, instance->internal_card_id);
			gain_control(1-player, fake, player, card);
			obliterate_card(1-player, fake);
		}
	}

	return global_enchantment(player, card, event);
}


// white
int card_act_of_authority(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
		if( pick_target(&td, "DISENCHANT") ){
			instance->number_of_targets = 1;
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( can_target(&td) && pick_target(&td, "DISENCHANT") ){
			instance->number_of_targets = 1;
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			int fake = add_card_to_hand(1-player, instance->internal_card_id);
			gain_control(1-player, fake, player, card);
			obliterate_card(1-player, fake);
		}
	}

	return global_enchantment(player, card, event);
}

int card_angel_of_finality(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			int count = count_graveyard(instance->targets[0].player)-1;
			while( count > -1 ){
					rfg_card_from_grave(instance->targets[0].player, count);
					count--;
			}
		}
	}
	return 0;
}

int card_curse_of_the_forsaken(int player, int card, event_t event)
{
  // Whenever a creature attacks enchanted player, its controller gains 1 life.
  card_instance_t* instance = in_play(player, card);
  if (instance && declare_attackers_trigger(player, card, event, DAT_ATTACKS_PLAYER|DAT_SEPARATE_TRIGGERS, 1-instance->targets[4].player, -1))
	gain_life(current_turn, 1);

  // Enchant player
  return curse(player, card, event);
}

int card_darksteel_mutation_insect(int player, int card, event_t event ){
	indestructible(player, card, event);
	return 0;
}

int card_darksteel_mutation(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_CHANGE_TYPE && affect_me(p, c) ){
			instance->targets[1].card = event_result;
			if( instance->targets[1].player == -1 ){
				instance->targets[1].player = get_internal_card_id_from_csv_id(CARD_ID_DARKSTEEL_MUTATION_INSECT);
			}
			event_result = instance->targets[1].player;
		}

		if( event == EVENT_SET_COLOR && affect_me(p, c) && instance->targets[1].card > -1 ){
			event_result |= cards_data[instance->targets[1].card].color;
		}

	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

// mystic barrier --> skipped


int card_serene_master(int player, int card, event_t event ){
	/* Serene Master	|1|W
	 * Creature - Human Monk 0/2
	 * Whenever ~ blocks, exchange its power and the power of target creature it's blocking until end of combat. */

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn != player && blocking(player, card, event) ){
		int his_pow = get_power(1-player, instance->blocking);
		int my_pow = get_power(player, card);
		set_pt_and_abilities_until_eot(player, card, player, card, his_pow, -1, 0, 0, 1);
		set_pt_and_abilities_until_eot(player, card, 1-player, instance->blocking, my_pow, -1, 0, 0, 1);
	}

	return 0;
}

int card_tempt_with_glory(int player, int card, event_t event)
{
  /* Tempt with Glory	|5|W
   * Sorcery
   * Tempting offer - Put a +1/+1 counter on each creature you control. Each opponent may put a +1/+1 counter on each creature he or she controls. For each
   * opponent who does, put a +1/+1 counter on each creature you control. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  manipulate_type(player, card, player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));

	  if (DIALOG(player, card, EVENT_ACTIVATE,
				 DLG_WHO_CHOOSES(1-player), DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL,
				 "Add +1/+1 counters", 1, count_subtype(1-player, TYPE_CREATURE, -1),
				 "Pass",               1, count_subtype(  player, TYPE_CREATURE, -1)) == 1)
		{
		  manipulate_type(player, card, 1-player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
		  manipulate_type(player, card,   player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
		}

	  kill_card(player, card, KILL_SACRIFICE);
	}

  return 0;
}

int card_unexpectedly_absent(int player, int card, event_t event){

	/* Unexpectedly Absent	|X|W|W
	 * Instant
	 * Put target nonland permanent into its owner's library just beneath the top X cards of that library. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PERMANENT") ){
			instance->info_slot = x_value;
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_permanent_under_the_first_x_card_of_its_owners_library(instance->targets[0].player, instance->targets[0].card, instance->info_slot);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

// gold
int card_derevi_empyrial_tactician(int player, int card, event_t event ){

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ||
		subtype_deals_damage(player, card, event, player, -1, DDBM_TRIGGER_OPTIONAL|DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_OPPONENT)
	  ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		int i, times_triggered = trigger_condition == TRIGGER_DEAL_DAMAGE ? instance->targets[1].card : 1;
		for (i = 0; i < times_triggered; ++i){
			if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
				instance->number_of_targets = 1;
				int choice = 1;
				if( player == HUMAN ){
					choice = do_dialog(player, player, card, -1, -1, " Tap\n Untap", 0);
				}
				if( choice == 0 ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
				}
				else{
					untap_card(instance->targets[0].player, instance->targets[0].card);
				}
			} else {
				break;
			}
		}
	}
	return 0;
}

int card_gahiji_honored_one(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // Whenever a creature attacks one of your opponents or a planeswalker an opponent controls, that creature gets +2/+0 until end of turn.
  int amt;
  if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, player, -1)))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
	  for (--amt; amt >= 0; --amt)
		if (in_play(current_turn, attackers[amt]))
		  pump_until_eot(player, card, current_turn, attackers[amt], 2, 0);
	}

  return 0;
}

int card_jeleva_nephalias_scourge(int player, int card, event_t event ){
	/*
	  Jeleva, Nephalia's Scourge English |1|U|B|R
	  Legendary Creature - Vampire Wizard 1/3
	  Flying
	  When Jeleva, Nephalia's Scourge enters the battlefield, each player exiles the top X cards of his or her library,
	  where X is the amount of mana spent to cast Jeleva.
	  Whenever Jeleva attacks, you may cast an instant or sorcery card exiled with it without paying its mana cost.
	*/

	card_instance_t *instance = get_card_instance(player, card);

	store_mana_paid_to_play_this(player, card, event);

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		int amount = instance->targets[3].card;
		if( amount ){
			int i, leg = 0, idx = 0;
			for(i=0; i<2; i++){
				int p = i == 0 ? player : 1-player;
				int *deck = deck_ptr[p];
				int to_remove = MIN(count_deck(p), amount);
				if( to_remove > 0 ){
					show_deck( player, deck, to_remove, "Cards exiled by Jeleva", 0, 0x7375B0 );
					while( to_remove ){
							if( is_what(-1, deck[0], TYPE_SPELL) && ! is_what(-1, deck[0], TYPE_CREATURE) ){
								exiledby_remember(player, card, p, deck[0], &leg, &idx);
								instance->info_slot++;
							}
							rfg_card_in_deck(p, 0);
							to_remove--;
					}
				}
			}
		}
	}

	int ai_mode = instance->info_slot ? RESOLVE_TRIGGER_MANDATORY : 0;
	if (declare_attackers_trigger(player, card, event, (player == AI ? ai_mode : RESOLVE_TRIGGER_OPTIONAL), player, card)){
		if( exiledby_choose(player, card, CARD_ID_JELEVA_NEPHALIAS_SCOURGE, EXBY_FIRST_FOUND | EXBY_TEST_CAN_CAST, EVENT_CAN_CAST, "spell", 1) ){
			int rval = exiledby_choose(player, card, CARD_ID_JELEVA_NEPHALIAS_SCOURGE, EXBY_CHOOSE | EXBY_MAX_VALUE | EXBY_TEST_CAN_CAST, EVENT_CAN_CAST, "spell", 1);
			int* loc = (int*)rval;
			if( loc ){
				int *loc2 = exiledby_find(player, card, *loc, NULL, NULL);
				if( *loc2 > -1 ){
					int p = *loc2 & 0x80000000;
					int iid = *loc2 & ~0x80000000;
					*loc2 = -1;
					if( remove_card_from_rfg(p, cards_data[iid].id) ){
						play_card_in_exile_for_free(player, p, cards_data[iid].id);
					}
					instance->info_slot--;
				}
			}
		}
	}

	return 0;
}

int card_marath_will_of_the_wild(int player, int card, event_t event ){
	/* Marath, Will of the Wild	|R|G|W
	 * Legendary Creature - Elemental Beast 0/0
	 * ~ enters the battlefield with a number of +1/+1 counters on it equal to the amount of mana spent to cast it.
	 * |X, Remove X +1/+1 counters from Marath: Choose one -
	 * * Put X +1/+1 counters on target creature. X can't be 0.
	 * * Marath deals X damage to target creature or player. X can't be 0.
	 * * Put an X/X |Sgreen Elemental creature token onto the battlefield. X can't be 0. */

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	store_mana_paid_to_play_this(player, card, event);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, (instance->targets[3].card > 0 ? instance->targets[3].card : 0));

	if( IS_GAA_EVENT(event) ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.preferred_controller = player;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		enum{
			CHOICE_PASS_COUNTERS = 1,
			CHOICE_DAMAGE,
			CHOICE_ELEMENTAL,
		};

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(player, card, event, 0, MANACOST_X(1), GVC_COUNTER(COUNTER_P1_P1), NULL, NULL);
		}

		if( event == EVENT_ACTIVATE ){
			instance->targets[1].card = instance->targets[1].player = instance->number_of_targets = 0;
			int max = count_1_1_counters(player, card);
			while( ! has_mana(player, COLOR_ANY, max) && max > 0 ){
					max--;
			}
			int amount = max;
			if( get_toughness(player, card) <= amount ){
				amount--;
			}
			if( player == HUMAN ){
				amount = choose_a_number(player, "Remove how many counters?", max);
				if( amount > count_1_1_counters(player, card) || ! has_mana_for_activated_ability(player, card, amount, 0, 0, 0, 0, 0) )
					amount = 0;
			}
			if( amount > 0 ){
				if( charge_mana_for_activated_ability(player, card, amount, 0, 0, 0, 0, 0) ){
					int choice = DIALOG(player, card, EVENT_ACTIVATE, DLG_RANDOM, DLG_NO_STORAGE,
										"Pass +1/+1 counters", can_target(&td1), 8,
										"Damage creature or player", can_target(&td2), 10,
										"Generate an Elemental", 1, 7);
					if( ! choice ){
						spell_fizzled = 1;
						return 0;
					}
					switch( choice ){
							case CHOICE_PASS_COUNTERS:
							{
								if( pick_target(&td1, "TARGET_CREATURE") ){
									remove_1_1_counters(player, card, amount);
								}
							}
							break;
							case CHOICE_DAMAGE:
							{
								if( pick_target(&td2, "TARGET_CREATURE_OR_PLAYER") ){
									remove_1_1_counters(player, card, amount);
								}
							}
							break;
							case CHOICE_ELEMENTAL:
								remove_1_1_counters(player, card, amount);
								break;
					}
					instance->targets[1].card = amount;
					instance->targets[1].player = choice;
				}
			}
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[1].player == CHOICE_PASS_COUNTERS && valid_target(&td1) ){
				add_1_1_counters(instance->targets[0].player, instance->targets[0].card, instance->targets[1].card);
			}
			if( instance->targets[1].player == CHOICE_DAMAGE && valid_target(&td2) ){
				damage_creature_or_player(player, card, event, instance->targets[1].card);
			}
			if( instance->targets[1].player == CHOICE_ELEMENTAL ){
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
				token.pow = instance->targets[1].card;
				token.tou = instance->targets[1].card;
				token.color_forced = COLOR_TEST_GREEN;
				generate_token(&token);
			}
		}
	}

	return 0;
}

int card_nekusar_the_mindrazer(int player, int card, event_t event){

	/* Nekusar, the Mindrazer	|2|U|B|R
	 * Legendary Creature - Zombie Wizard 2/4
	 * At the beginning of each player's draw step, that player draws an additional card.
	 * Whenever an opponent draws a card, ~ deals 1 damage to that player. */

	check_legend_rule(player, card, event);

	if( event == EVENT_DRAW_PHASE ){
		event_result++;
	}

	if (card_drawn_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY)){
		damage_player(1-player, 1, player, card);
	}

	return 0;
}

int card_oloro_ageless_ascetic(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		gain_life(player, 2);
	}

	return 0;
}

int card_prossh_skyraider_of_kher(int player, int card, event_t event){
	/* Prossh, Skyraider of Kher	|3|B|R|G
	 * Legendary Creature - Dragon 5/5
	 * Flying
	 * When you cast ~, put X 0/1 |Sred Kobold creature tokens named Kobolds of Kher Keep onto the battlefield, where X is the amount of mana spent to cast Prossh.
	 * Sacrifice another creature: Prossh gets +1/+0 until end of turn. */

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			store_mana_paid_to_play_this(player, card, event);
		}
		if( instance->targets[3].card > 0 && ! check_special_flags(player, card, SF_NOT_CAST) && ! is_token(player, card) ){
			generate_tokens_by_id(player, card, CARD_ID_KOBOLDS_OF_KHER_KEEP, instance->targets[3].card);
		}
	}

	if( event == EVENT_CAN_ACTIVATE && count_subtype(player, TYPE_CREATURE, -1) > 1 ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			state_untargettable(player, card, 1);
			if( ! sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
			state_untargettable(player, card, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, 0);
	}

	return 0;
}

int card_roon_of_the_hidden_realm(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  if (event == EVENT_ABILITIES && affect_me(player, card))
	vigilance(player, card, event);

  // |2, |T: Exile another target creature. Return that card to the battlefield under its owner's control at the beginning of the next end step.
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.special = TARGET_SPECIAL_NOT_ME;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  remove_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_ANOTHER_CREATURE");
}

int card_sydri_galvanic_genius(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // |U: Target noncreature artifact becomes an artifact creature with power and toughness each equal to its converted mana cost until end of turn.
  // |W|B: Target artifact creature gains deathtouch and lifelink until end of turn.
  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  target_definition_t td_noncreature_artifact;
	  default_target_definition(player, card, &td_noncreature_artifact, TYPE_ARTIFACT);
	  td_noncreature_artifact.illegal_type = TYPE_CREATURE;

	  target_definition_t td_artifact_creature;
	  default_target_definition(player, card, &td_artifact_creature, TYPE_CREATURE);
	  td_artifact_creature.special = TARGET_SPECIAL_ARTIFACT_CREATURE;

	  enum
	  {
		CHOICE_ANIMATE = 1,
		CHOICE_DEATHTOUCH_LIFELINK
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Animate artifact", 1, 2, DLG_MANA(MANACOST_U(1)), DLG_TARGET(&td_noncreature_artifact, "XENIC_POLTERGEIST"),	/*"Select target non-creature artifact."*/
						"Deathtouch and lifelink", 1, 1, DLG_MANA(MANACOST_WB(1,1)), DLG_TARGET(&td_artifact_creature, "TARGET_ARTIFACTCREATURE"));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  int pp = instance->parent_controller, pc = instance->parent_card;
		  int tp = instance->targets[0].player, tc = instance->targets[0].card;

		  switch (choice)
			{
			  case CHOICE_ANIMATE:
				;int cmc = get_cmc(tp, tc);
				artifact_animation(pp, pc, tp, tc, 1, cmc,cmc, 0,0);
			  break;

			case CHOICE_DEATHTOUCH_LIFELINK:
			  pump_ability_until_eot(pp, pc, tp, tc, 0,0, 0,SP_KEYWORD_DEATHTOUCH|SP_KEYWORD_LIFELINK);
			  break;
		  }
		}
	}

  return 0;
}

int card_shattergang_brothers(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0) &&
			can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0) &&
			can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0) &&
			can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int abils[3] = {can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) && has_mana_for_activated_ability(player, card, MANACOST_XB(2, 1)),
						can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) && has_mana_for_activated_ability(player, card, MANACOST_XR(2, 1)),
						can_sacrifice_as_cost(player, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) && has_mana_for_activated_ability(player, card, MANACOST_XG(2, 1))};

		int priorities[3] = {	count_subtype(1-player, TYPE_CREATURE, -1) ? 5+(5*(count_subtype(player, TYPE_CREATURE, -1)-count_subtype(1-player, TYPE_CREATURE, -1))) : 0,
								count_subtype(1-player, TYPE_ARTIFACT, -1) ? 5+(5*(count_subtype(player, TYPE_ARTIFACT, -1)-count_subtype(1-player, TYPE_ARTIFACT, -1))) : 0,
								count_subtype(1-player, TYPE_ENCHANTMENT, -1) ? 5+(5*(count_subtype(player, TYPE_ENCHANTMENT, -1)-count_subtype(1-player, TYPE_ENCHANTMENT, -1))) : 0};

		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
							"Sac a creature", abils[0], priorities[0],
							"Sac an artifact", abils[1], priorities[1],
							"Sac an enchantment", abils[2], priorities[2]);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}

		if( charge_mana_for_activated_ability(player, card, 2, choice == 1, 0, choice == 3, choice == 2, 0) ){
			int types[3] = { TYPE_CREATURE, TYPE_ARTIFACT, TYPE_ENCHANTMENT };
			if( sacrifice(player, card, player, 0, types[choice-1], 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				instance->info_slot = choice;
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int types[3] = { TYPE_CREATURE, TYPE_ARTIFACT, TYPE_ENCHANTMENT };
		impose_sacrifice(player, instance->parent_card, 1-player, 1, types[instance->info_slot-1], 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

// artifacts

int card_eye_of_doom(int player, int card, event_t event){

	/* Eye of Doom	|4
	 * Artifact
	 * When ~ enters the battlefield, each player chooses a nonland permanent and puts a doom counter on it.
	 * |2, |T, Sacrifice ~: Destroy each permanent with a doom counter on it. */

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		APNAP(p, {
			td.preferred_controller = 1-p;
			td.who_chooses = p;
			if( can_target(&td) && pick_target(&td, "TARGET_NONLAND_PERMANENT") ){
				instance->number_of_targets = 0;
				add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_DOOM);
			}
		});
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i, count;
		for(i=0; i<2; i++ ){
			for (count = active_cards_count[i] - 1; count > -1; --count){
				if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) && count_counters(i, count, COUNTER_DOOM) > 0 ){
					kill_card(i, count, KILL_DESTROY);
				}
			}
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_ME, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_surveyors_scope(int player, int card, event_t event){

	char msg[100] = "Select a basic land card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, msg);
	this_test.subtype = SUBTYPE_BASIC;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( count_subtype(1-player, TYPE_LAND, -1) - count_subtype(player, TYPE_LAND, -1) > 1 ){
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_RFG_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

// lands
int card_opal_palace(int player, int card, event_t event){
	/* Opal Palace	""
	 * Land
	 * |T: Add |C to your mana pool.
	 * |1, |T: Add to your mana pool one mana of any color in your commander's color identity. If you spend this mana to cast your commander, it enters the battlefield with a number of additional +1/+1 counters on it equal to the number of times it's been cast from the command zone this game. */

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int count = 0;
		while( count < active_cards_count[player] ){
				if( in_play(player, count) && get_id(player, count) == CARD_ID_ELDER_DRAGON_HIGHLANDER ){
					card_instance_t *this = get_card_instance(player, count);
					int fake = this->info_slot;
					instance->info_slot = cards_data[fake].color;
					instance->info_slot |= get_color_from_remainder_text(cards_data[fake].id);
					break;
				}
				count++;
		}
		if( event == EVENT_RESOLVE_SPELL ){
			play_land_sound_effect_force_color(player, card, instance->info_slot);
		}
	}
	if( event == EVENT_COUNT_MANA && affect_me(player, card) && !is_tapped(player, card) && !is_animated_and_sick(player, card)){
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}

	if( event == EVENT_CAN_ACTIVATE && !(is_tapped(player, card)) ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana(player, COLOR_COLORLESS, 2) && player != AI && instance->info_slot > 0 ){
			choice = do_dialog(player, player, card, -1, -1, " Add colorless mana\n Add colored mana\n Cancel", 0);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			instance->state |= STATE_TAPPED;
			charge_mana(player, COLOR_COLORLESS, 1);
			if( spell_fizzled == 1){
				instance->state &= ~STATE_TAPPED;
				return 0;
			}
			produce_mana_tapped_all_one_color(player, card, instance->info_slot, 1);
			if( spell_fizzled == 1){
				instance->state &= ~STATE_TAPPED;
				return 0;
			}
		}
	}

	return 0;
}
