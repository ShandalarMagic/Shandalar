#include "manalink.h"

//-------- GLOBAL FUNCTIONS

static int coldsnap_crusaders(int player, int card, event_t event, int mana_color){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		int c[5] = {0, 0, 0, 0, 0};
		c[mana_color-1]++;
		if( has_mana_for_activated_ability(player, card, 0, c[0], c[1], c[2], c[3], c[4]) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		int c[5] = {0, 0, 0, 0, 0};
		c[mana_color-1]+=2;
		if( has_mana_for_activated_ability(player, card, 0, c[0], c[1], c[2], c[3], c[4]) ){
			if( current_phase > PHASE_DECLARE_BLOCKERS || check_for_ability(player, card, KEYWORD_FLYING) ){
				ai_choice = 1;
			}
			choice = do_dialog(player, player, card, -1, -1, " Give flying\n +1/+0 until end of turn\n Cancel", ai_choice);
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			c[mana_color-1] = 0;
			c[mana_color-1]+=(choice+1);
			charge_mana_for_activated_ability(player, card, 0, c[0], c[1], c[2], c[3], c[4]);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( instance->info_slot == 66){
			pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, KEYWORD_FLYING, 0);
		}

		if( instance->info_slot == 67){
			pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, 0);
		}
	}

	return 0;
}

static int martyr(int player, int card, event_t event, int cless, test_definition_t *this_test, target_definition_t *td, const char *prompt){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana_for_activated_ability(player, card, cless, 0, 0, 0, 0, 0) &&
		can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		if( get_id(player, card) == CARD_ID_MARTYR_OF_FROST ){
			return card_spiketail_hatchling(player, card, event);
		}
		if( get_id(player, card) == CARD_ID_MARTYR_OF_BONES ){
			if( count_graveyard(2) > 0 ){
				return ! graveyard_has_shroud(2);
			}
			else{
				return 0;
			}
		}
		if( td != NULL ){
			return can_target(td);
		}
		return 1;
	}

	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, cless, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				if( get_id(player, card) == CARD_ID_MARTYR_OF_FROST ){
					instance->targets[0].player = card_on_stack_controller;
					instance->targets[0].card = card_on_stack;
					instance->targets[1].player = reveal_cards_from_your_hand(player, card, this_test);
					kill_card(player, card, KILL_SACRIFICE);
				}
				else if( td != NULL ){
						load_text(0, prompt);
						if( ! select_target(td->player, td->card, td, text_lines[0], NULL) ){
							spell_fizzled = 1;
						}
						else{
							if( get_id(player, card) == CARD_ID_MARTYR_OF_BONES ){
								instance->number_of_targets = 1;
								if( count_graveyard(instance->targets[0].player) < 1 ){
									spell_fizzled = 1;
								}
								else{
									int result = reveal_cards_from_your_hand(player, card, this_test);
									int sel = 0;
									int cg = count_graveyard(instance->targets[0].player);
									int swap[cg];
									int i;
									for (i = 0; i < cg; i++){
										swap[i] = -1;
									}

									test_definition_t this_test2;
									new_default_test_definition(&this_test2, 0, "Select a card to exile.");
									this_test2.id = 904;
									this_test2.id_flag = 1;
									while( sel < result && sel < 8 && cg > 0){
											int selected = new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_VALUE,
																						&this_test2, sel+2);
											if( selected != -1 ){
												swap[selected] = turn_card_in_grave_face_down(instance->targets[0].player, selected);
												sel++;
												cg--;
											}
											else{
												break;
											}
									}
									for (i = 0; i < cg; i++){
										if( swap[i] != -1 ){
											turn_card_in_grave_face_up(instance->targets[0].player, i, swap[i]);
										}
									}
									for(i=2; i<sel+2; i++){
										int k;
										for(k=i+1; k<sel+2; k++){
											if( instance->targets[i].player > instance->targets[k].player ){
												SWAP(instance->targets[i], instance->targets[k]);	// struct copy
											}
										}
									}
									if( sel > 0 ){
										instance->targets[1].player = result+2;
										kill_card(player, card, KILL_SACRIFICE);
									}
								}
							}
							else{
								instance->targets[1].player = reveal_cards_from_your_hand(player, card, this_test);
								kill_card(player, card, KILL_SACRIFICE);
							}
						}
				}
				else{
					instance->targets[1].player = reveal_cards_from_your_hand(player, card, this_test);
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
	}

	return 0;
}

static void ripple(int player, int card){
	if( ! check_special_flags(player, card, SF_NOT_CAST) ){
		int max = 4;
		int *deck = deck_ptr[player];
		if( max > count_deck(player) ){
			max = count_deck(player);
		}
		if( max > 0){
			char buffer[100];
			card_ptr_t* c = cards_ptr[ get_id(player, card)  ];
			scnprintf(buffer, 100, "Cards revealed by %s\n", c->name);
			show_deck(HUMAN, deck, max, buffer, 0, 0x7375B0 );
			int i;
			for(i=max-1; i > -1; i--){
				if( cards_data[deck[i]].id == get_id(player, card) ){
					char buffer2[100];
					card_ptr_t* c2 = cards_ptr[ cards_data[deck[i]].id  ];
					scnprintf(buffer2, 100, " Play %s\n Pass", c2->name);
					int choice = do_dialog(player, player, card, -1, -1, buffer2, 0);
					if( choice == 0 ){
						play_card_in_deck_for_free(player, player, i);
						max--;
					}
				}
			}
			if( max > 0 ){
				put_top_x_on_bottom(player, player, max);
			}
		}
	}
}

static int casting_coldsnap_pitchspell(int player, int card, event_t event, test_definition_t *this_test){

	card_instance_t *instance = get_card_instance(player, card);

	if( played_for_free(player, card) || is_token(player, card) || instance->info_slot != 1){
		return 1;
	}

	int choice = 0;
	if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
		choice = do_dialog(player, player, card, -1, -1, " RFG two cards in hand\n Cast normally\n Cancel", 0);
	}
	if( choice == 0 ){
		int selected[2] = {-1, -1};
		int h_array[2][100];
		int ha_count = 0;
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && i != card ){
				h_array[0][ha_count] = get_original_internal_card_id(player, i);
				h_array[1][ha_count] = i;
				ha_count++;
			}
		}
		for(i=0; i<2; i++){
			selected[i] = select_card_from_zone(player, player, h_array[0], ha_count, 0, AI_MIN_VALUE, -1, this_test);
			if( selected[i] != -1 ){
				h_array[0][selected[i]] = get_internal_card_id_from_csv_id(CARD_ID_RULES_ENGINE);
			}
			else{
				break;
			}
		}
		if( selected[0] == -1 || selected[1] == -1 ){
			return 0;
		}
		else{
			for(i=0; i<2; i++){
				rfg_card_in_hand(player, h_array[1][selected[i]]);
			}
			return 1;
		}
	}
	if( choice == 1 ){
		charge_mana_from_id(player, card, event, get_id(player, card));
		if( spell_fizzled != 1 ){
			return 1;
		}
	}
	return 0;
}

static void cost_mod_for_coldsnap_pitchspell(int player, int card, event_t event, test_definition_t *this_test){
	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, this_test) > 2 ){
			null_casting_cost(player, card);
			get_card_instance(player, card)->info_slot = 1;
		}
		else{
			get_card_instance(player, card)->info_slot = 0;
		}
	}
}

static int coldsnap_pitchspell(int player, int card, event_t event, test_definition_t *this_test){

	cost_mod_for_coldsnap_pitchspell(player, card, event, this_test);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! casting_coldsnap_pitchspell(player, card, event, this_test) ){
			spell_fizzled = 1;
		}
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}


//-------- CARDS

static int effect_valkyrie(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(p, c) ){
			card_instance_t *affected = get_card_instance(p, c);
			if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE){
				instance->targets[11].player = 66;
				// Unattach
				instance->targets[0].player = instance->damage_target_player;
				instance->targets[0].card = instance->damage_target_card;
				instance->damage_target_player = -1;
				instance->damage_target_card = -1;
			}
		}
	}

	if( instance->targets[11].player == 66 && resolve_graveyard_trigger(player, card, event) == 1 ){
		int owner, position;
		if (find_in_owners_graveyard(instance->targets[0].player, instance->targets[0].card, &owner, &position)){
			reanimate_permanent(player, card, owner, position, REANIMATE_NO_CONTROL_LEGACY);
		}
		instance->targets[11].player = 0;
		kill_card(player, card, KILL_REMOVE);
	}

	if (event == EVENT_CLEANUP){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_adarkar_valkyrie(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	vigilance(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, instance->parent_card, &effect_valkyrie, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_NOT_ME_AS_TARGET, MANACOST_X(0), 0, &td, "TARGET_CREATURE");
}

int card_allosaurus_rider(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += landsofcolor_controlled[player][COLOR_ANY];
	}

	if( event != EVENT_MODIFY_COST && ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select a green card to exile");
	this_test.color = COLOR_TEST_GREEN;
	this_test.zone = TARGET_ZONE_HAND;
	this_test.id = CARD_ID_RULES_ENGINE;
	this_test.id_flag = 1;

	return coldsnap_pitchspell(player, card, event, &this_test);
}

int card_arctic_nishoba(int player, int card, event_t event)
{
  /* Arctic Nishoba	|5|G
   * Creature - Cat Warrior 6/6
   * Trample
   * Cumulative upkeep |G or |W
   * When ~ dies, you gain 2 life for each age counter on it. */

  cumulative_upkeep_hybrid(player, card, event, 1, COLOR_GREEN, COLOR_WHITE, 0);

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	gain_life(player, 2 * count_counters(player, card, COUNTER_AGE));

  return 0;
}

int card_arcum_dagsson(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.special = TARGET_SPECIAL_ARTIFACT_CREATURE;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) && can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
				char msg[100] = "Select a non-creature artifact.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);
				this_test.type_flag = F1_NO_CREATURE;
				new_global_tutor(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
			}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_bull_aurochs(int player, int card, event_t event)
{
  // Whenever ~ attacks, it gets +1/+0 until end of turn for each other attacking Aurochs.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t this_test;
	  default_test_definition(&this_test, TYPE_PERMANENT);
	  this_test.subtype = SUBTYPE_AUROCHS;
	  this_test.state = STATE_ATTACKING;
	  this_test.not_me = 1;

	  int amount = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
	  pump_until_eot(player, card, player, card, amount, 0);
	}

  return 0;
}

int card_aurochs_herd(int player, int card, event_t event){

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "Select an Aurochs card.");
		this_test.subtype = SUBTYPE_AUROCHS;

		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return card_bull_aurochs(player, card, event);
}

int card_balduvian_fallen(int player, int card, event_t event)
{
  /* Balduvian Fallen	|3|B
   * Creature - Zombie 3/5
   * Cumulative upkeep |1
   * Whenever ~'s cumulative upkeep is paid, it gets +1/+0 until end of turn for each |B or |R spent this way. */

  cumulative_upkeep(player, card, event, MANACOST_X(1));
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY && in_play(player, card)	// Cumulative upkeep was just successfully paid
	  && (mana_paid[COLOR_BLACK] || mana_paid[COLOR_RED]))
	pump_until_eot(player, card, player, card, mana_paid[COLOR_BLACK] + mana_paid[COLOR_RED], 0);

  return 0;
}

int card_balduvian_rage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_ATTACKING_CREATURE") ){
				instance->info_slot = x_value;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, instance->info_slot, 0);
				cantrip(player, card, 1);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_blizzard_specter(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.allowed_controller = 1-player;
  td.preferred_controller = 1-player;
  td.illegal_abilities = 0;
  td.who_chooses = 1-player;
  td.allow_cancel = 0;

  if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_OPPONENT) ){
	  int ai_choice = 1;
	  if( hand_count[1-player] < 1 && can_target(&td) ){
		  ai_choice = 0;
	  }

	  int choice = do_dialog(player, player, card, -1, -1, " Opponent bounces\n Opponent discard", ai_choice);

	  if( choice == 1 ){
		  discard(1-player, 0, player);
	  }
	  else if( choice == 0 ){

			   card_instance_t *instance = get_card_instance( player, card );

			   pick_target(&td, "TARGET_PERMANENT");

			   bounce_permanent(instance->targets[0].player, instance->targets[0].card);
	  }
  }

  return 0;
}

int card_boreal_druid(int player, int card, event_t event){
	return mana_producing_creature(player, card, event, 24, COLOR_COLORLESS, 1);
}

static int rtrue(int player, int card, int number_of_age_counters)
{
  return 1;
}
static int produce_mana_red_thunk(int player, int card, int number_of_age_counters)
{
  produce_mana(player, COLOR_RED, number_of_age_counters);
  return 1;
}
int card_braid_of_fire(int player, int card, event_t event)
{
  /* Braid of Fire	|1|R
   * Enchantment
   * Cumulative upkeep-Add |R to your mana pool. */

  cumulative_upkeep_general(player, card, event, rtrue, produce_mana_red_thunk);

  return global_enchantment(player, card, event);
}

// controvert --> counterspell

int card_controvert(int player, int card, event_t event){
	return card_counterspell(player, card, event);
}

int card_counterbalance(int player, int card, event_t event)
{
  // Whenever an opponent casts a spell, you may reveal the top card of your library. If you do, counter that spell if it has the same converted mana cost as the revealed card.
  if (trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller
	  && trigger_cause_controller == 1-player && !is_what(trigger_cause_controller, trigger_cause, TYPE_LAND)
	  && deck_ptr[player][0] != -1)
	{
	  if (event == EVENT_TRIGGER)
		{
		  if ((player == AI || ai_is_speculating == 1) && !check_state(trigger_cause_controller, trigger_cause, STATE_CANNOT_TARGET))
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		  else
			event_result |= RESOLVE_TRIGGER_OPTIONAL;
		}

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  reveal_card_iid(player, card, deck_ptr[player][0]);
		  if (get_cmc_by_internal_id(deck_ptr[player][0]) == get_cmc(trigger_cause_controller, trigger_cause)
			  && !check_state(trigger_cause_controller, trigger_cause, STATE_CANNOT_TARGET))
			kill_card(trigger_cause_controller, trigger_cause, KILL_SACRIFICE);
		}
	}

  return global_enchantment(player, card, event);
}

int card_coldsteel_heart(int player, int card, event_t event){

	/* Coldsteel Heart	|2
	 * Snow Artifact
	 * ~ enters the battlefield tapped.
	 * As ~ enters the battlefield, choose a color.
	 * |T: Add one mana of the chosen color to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t* instance = get_card_instance(player, card);
		instance->info_slot = 1 << choose_a_color(player, get_deck_color(player, player));
	}

	return mana_producer(player, card, event);
}

static int has_snow_mana_thunk(int player, int card, int number_of_age_counters)
{
  return has_snow_mana(player, number_of_age_counters, 0);
}
static int charge_snow_mana_while_processing(int player, int card, int number_of_age_counters)
{
  put_card_or_activation_onto_stack(player, card, EVENT_RESOLVE_TRIGGER, 0, 0);
  ldoubleclicked = spell_fizzled = 0;
  int rval = charge_snow_mana(player, card, number_of_age_counters, 0);
  obliterate_top_card_of_stack();
  return rval;
}
int card_cover_of_winter(int player, int card, event_t event){

	/* Cumulative upkeep |I
	 * If a creature would deal combat damage to you and/or one or more creatures you control, prevent X of that damage, where X is the number of age counters
	 * on ~.
	 * |I: Put an age counter on ~. */

	cumulative_upkeep_general(player, card, event, has_snow_mana_thunk, charge_snow_mana_while_processing);

	card_instance_t* damage = combat_damage_being_prevented(event);
	if( damage &&
		damage->damage_target_player == player &&
		(damage->targets[3].player & TYPE_CREATURE) &&	// probably redundant to status check
		!check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER)
	  ){
		if( damage->info_slot < count_counters(player, card, COUNTER_AGE) ){
			damage->info_slot = 0;
		}
		else{
			damage->info_slot-=count_counters(player, card, COUNTER_AGE);
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 0, 0);
		return can_use_activated_abilities(player, card) && has_snow_mana(player, 1, c1);
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 0, 0);
		charge_snow_mana(player, card, 1, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		add_counter(instance->parent_controller, instance->parent_card, COUNTER_AGE);
	}

	return global_enchantment(player, card, event);
}

int card_darien_king_of_kjeldor(int player, int card, event_t event){
	/* Darien, King of Kjeldor	|4|W|W
	 * Legendary Creature - Human Soldier 3/3
	 * Whenever you're dealt damage, you may put that many 1/1 |Swhite Soldier creature tokens onto the battlefield. */

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && instance->info_slot != current_phase ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player){
				generate_tokens_by_id(player, card, CARD_ID_SOLDIER, damage->info_slot);
				instance->info_slot = current_phase;
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		instance->info_slot =0 ;
	}

  return 0;
}

int card_dark_depths(int player, int card, event_t event){

	/* Dark Depths	""
	 * Legendary Snow Land
	 * ~ enters the battlefield with ten ice counters on it.
	 * |3: Remove an ice counter from ~.
	 * When ~ has no ice counters on it, sacrifice it. If you do, put a legendary 20/20 |Sblack Avatar creature token with flying and indestructible named Marit Lage onto the battlefield. */

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if (event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
	}

	enters_the_battlefield_with_counters(player, card, event, COUNTER_ICE, 10);

	if (event == EVENT_STATIC_EFFECTS && count_counters(player, card, COUNTER_ICE) == 0 ){
		kill_card(player, card, KILL_SACRIFICE);
		generate_token_by_id(player, card, CARD_ID_MARIT_LAGE);
	}

	if (event == EVENT_RESOLVE_ACTIVATION ){
		remove_counter(instance->parent_controller, instance->parent_card, COUNTER_ICE);
	}

	return generic_activated_ability(player, card, event, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_marit_lage(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	indestructible(player, card, event);
	return 0;
}

static int target_for_deepfire_elemental(int player, int card, int mode){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_CREATURE);

	card_instance_t *instance= get_card_instance(player, card);

	int i;
	int trg = -1;
	int par = -1;
	for(i=0; i<2; i++){
		if( i == 1-player || player == HUMAN ){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && is_what(i, count, TYPE_ARTIFACT | TYPE_CREATURE) ){
						int cost = (get_cmc(i, count)*2)+1;
						if( has_mana_for_activated_ability(player, card, MANACOST_X(cost)) ){
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

int card_deepfire_elemental(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_CREATURE);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
		return target_for_deepfire_elemental(player, card, 0);
	}

	if( event == EVENT_ACTIVATE ){
		instance->targets[0].player = -1;
		instance->targets[0].card = -1;
		instance->number_of_targets = 0;
		if( player == HUMAN ){
			if( select_target(player, card, &td, "Select target creature or artifact", NULL) ){
				int cost = (2*get_cmc( instance->targets[0].player, instance->targets[0].card ))+1;
				if( ! has_mana_for_activated_ability(player, card, MANACOST_X(cost)) ){
					spell_fizzled = 1;
					return 0;
				}
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
		else if( player == AI ){
				int result = target_for_deepfire_elemental(player, card, 1);
				if( result != -1 ){
					instance->targets[0].player = 1-player;
					instance->targets[0].card = result;
					instance->number_of_targets = 1;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
		}
		if( spell_fizzled != 1 ){
			int cost = (2*get_cmc( instance->targets[0].player, instance->targets[0].card ))+1;
			charge_mana_for_activated_ability(player, card, cost, 0, 0, 0, 0, 0);
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
		}
	}
	return 0;
}

int card_diamond_faerie(int player, int card, event_t event){

	/* Diamond Faerie	|2|G|W|U
	 * Snow Creature - Faerie 3/3
	 * Flying
	 * |1|I: Snow creatures you control get +1/+1 until end of turn. */

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 0, 0);
		return can_use_activated_abilities(player, card) && has_snow_mana(player, 1, c1);
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 0, 0);
		charge_snow_mana(player, card, 1, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, instance->parent_card, player, SUBTYPE_SNOW, 1, 1, 0, 0);
	}

	return 0;
}

int card_disciple_of_tevesh_szat(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 4, 2, 0, 1, 0, 0) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			choice = do_dialog(player, player, card, -1, -1, " Give -1/-1\n Sac & give -6/-6\n Cancel", 1);
		}

		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			int c1 = get_cost_mod_for_activated_abilities(player, card, choice*4, 0, 0, 0, 0, 0);
			charge_mana_multi(player, c1, choice*2, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->state |= STATE_TAPPED;
				instance->number_of_targets = 1;
				instance->info_slot = 1+(5*choice);
				if( choice == 1 ){
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									-instance->info_slot, -instance->info_slot, 0, 0);
		}
	}

	return 0;
}

int card_earthen_goo(int player, int card, event_t event)
{
  /* Earthen Goo	|2|R
   * Creature - Ooze 2/2
   * Trample
   * Cumulative upkeep |R or |G
   * ~ gets +1/+1 for each age counter on it. */

  cumulative_upkeep_hybrid(player, card, event, 1, COLOR_RED, COLOR_GREEN, 0);

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card))
	event_result += count_counters(player, card, COUNTER_AGE);

  return 0;
}

int card_feast_of_flesh(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = count_graveyard_by_id(player, get_id(player, card))+1;
			damage_creature(instance->targets[0].player, instance->targets[0].card, amount, player, card);
			gain_life(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_field_marshal( int player, int card, event_t event){

	return boost_creature_type(player, card, event, SUBTYPE_SOLDIER, 1, 1, KEYWORD_FIRST_STRIKE, 0);
}

int card_frostweb_spider(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_DECLARE_BLOCKERS && blocking(player, card, event) ){
		if( check_for_ability(1-player, instance->blocking, KEYWORD_FLYING) ){
			instance->targets[1].player = 66;
		}
	}

	if( instance->targets[1].player == 66 && end_of_combat_trigger(player, card, event, 2) ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_frozen_solid(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
		if( damage_dealt_to_me_arbitrary(instance->damage_target_player, instance->damage_target_card, event, 0, player, card) ){
			kill_card(instance->damage_target_player, instance->damage_target_card, KILL_DESTROY);
		}
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_garza_zol_plague_queen(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	haste(player, card, event);

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		draw_cards(player, 1);
	}
	if( sengir_vampire_trigger(player, card, event, 2) ){
		add_1_1_counters(player, card, get_card_instance(player, card)->targets[11].card);
	}

	return 0;
}

int card_garzas_assassin(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_gelid_shackles(int player, int card, event_t event){

	/* Gelid Shackles	|W
	 * Snow Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature can't block, and its activated abilities can't be activated.
	 * |I: Enchanted creature gains defender until end of turn. */

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		cannot_block(instance->damage_target_player, instance->damage_target_card, event);
		if( event == EVENT_CAN_ACTIVATE ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 0, 0);
			return can_use_activated_abilities(player, card) && has_snow_mana(player, 1, c1);
		}

		if( event == EVENT_ACTIVATE ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 0, 0);
			charge_snow_mana(player, card, 1, c1);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_ability_until_eot(player, instance->parent_card, instance->damage_target_player, instance->damage_target_card, 0, 0, KEYWORD_DEFENDER, 0);
		}
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, GA_FORBID_ALL_ACTIVATED_ABILITIES);
}

int card_glacial_plating(int player, int card, event_t event)
{
  /* Glacial Plating	|2|W|W
   * Snow Enchantment - Aura
   * Enchant creature
   * Cumulative upkeep |I
   * Enchanted creature gets +3/+3 for each age counter on ~. */

  cumulative_upkeep_general(player, card, event, has_snow_mana_thunk, charge_snow_mana_while_processing);

  card_instance_t* instance;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && (instance = in_play(player, card)) && affect_me(instance->damage_target_player, instance->damage_target_card))
	event_result += 3 * count_counters(player, card, COUNTER_AGE);

  return vanilla_aura(player, card, event, player);
}

int card_grim_harvest(int player, int card, event_t event){
	return card_raise_dead(player, card, event);
}

int card_gutless_ghoul(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 2);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_haakon_stromgald_scourge(int player, int card, event_t event){
	/* Haakon, Stromgald Scourge	|1|B|B
	 * Legendary Creature - Zombie Knight 3/3
	 * You may cast ~ from your graveyard, but not from anywhere else.
	 * As long as Haakon is on the battlefield, you may play Knight cards from your graveyard.
	 * When Haakon dies, you lose 2 life. */

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_MODIFY_COST && in_hand(player, card) ){
		infinite_casting_cost();
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		lose_life(player, 2);
	}

	if (event == EVENT_CAN_ACTIVATE){
		int i, sorc = can_sorcery_be_played(player, event);
		const int* grave = get_grave(player);
		for (i = 0; grave[i] != -1; ++i){
			if (has_subtype(-1, grave[i], SUBTYPE_KNIGHT) &&
				/* Instants, interrupts, cards with flash can be played at any time.  Otherwise, only when a sorcery is legal.  Check card_data_t::type directly
				 * instead of using is_what(), which strips TYPE_INSTANT from permanent types. */
				(sorc || cards_data[grave[i]].type & (TYPE_INSTANT | TYPE_INTERRUPT)) &&
				has_mana_to_cast_id(player, event, cards_data[grave[i]].id)
			   ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, 0, "Select a Knight card.");
		this_test.subtype = SUBTYPE_KNIGHT;

		int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, 1, -1, &this_test);
		if( selected == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		const int *grave = get_grave(player);
		if( !(cards_data[grave[selected]].type & (TYPE_INSTANT | TYPE_INTERRUPT)) &&
			!can_sorcery_be_played(player, event)
			){
			spell_fizzled = 1;
			return 0;
		}
		if( !has_mana_to_cast_id(player, event, cards_data[grave[selected]].id) ){
			spell_fizzled = 1;
			return 0;
		}
		charge_mana_from_id(player, -1, event, cards_data[grave[selected]].id);
		if( spell_fizzled != 1 ){
			play_card_in_grave_for_free(player, player, selected);
			cant_be_responded_to = 1;	// The animated knight will be respondable to, but the extra activation effect from Haakon won't be
		}
	}

	if( event == EVENT_GRAVEYARD_ABILITY ){
		int cless = get_updated_casting_cost(player, -1, instance->internal_card_id, event, 1);
		if( has_mana_multi(player, MANACOST_XB(cless, 2)) ){
			return GA_PLAYABLE_FROM_GRAVE;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		int cless = get_updated_casting_cost(player, -1, instance->internal_card_id, event, 1);
		charge_mana_multi(player, MANACOST_XB(cless, 2));
		if( spell_fizzled != 1 ){
			return GAPAID_REMOVE;
		}
	}

	return 0;
}

int card_heidar_rimewind_master(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	if (event == EVENT_CAN_ACTIVATE && count_subtype(player, TYPE_PERMANENT, SUBTYPE_SNOW) < 4){
		return 0;
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_PERMANENT");
}

static int opponent_controls_lands(int player, int card, int number_of_age_counters)
{
  int c;
  for (c = 0; c < active_cards_count[1-player]; ++c)
	if (in_play(1-player, c) && is_what(1-player, c, TYPE_LAND)
		&& --number_of_age_counters == 0)
	  return 1;
  return 0;
}
static int gain_control_of_lands(int player, int card, int number_of_age_counters)
{
  target_definition_t td;
  base_target_definition(player, card, &td, TYPE_LAND);
  td.allowed_controller = 1-player;
  td.preferred_controller = 1-player;

  char marked[2][151] = {{0}};
  if (mark_up_to_n_targets_noload(&td, "Select a land you don't control.", number_of_age_counters, marked) == number_of_age_counters)
	{
	  int c;
	  for (c = 0; c < active_cards_count[1-player]; ++c)
		if (marked[1-player][c] && in_play(1-player, c))
		  gain_control(player, card, 1-player, c);
	  return 1;
	}
  else
	return 0;
}
int card_herald_of_leshrac(int player, int card, event_t event)
{
  /* Herald of Leshrac	|6|B
   * Creature - Avatar 2/4
   * Flying
   * Cumulative upkeep-Gain control of a land you don't control.
   * ~ gets +1/+1 for each land you control but don't own.
   * When ~ leaves the battlefield, each player gains control of each land he or she owns that you control. */

  cumulative_upkeep_general(player, card, event, opponent_controls_lands, gain_control_of_lands);

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card))
	{
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && is_what(player, c, TYPE_LAND) && is_stolen(player, c))
		  ++event_result;
	}

  if (leaves_play(player, card, event))
	{
	  int card_added = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
	  get_back_your_permanents(1-player, card_added, TYPE_LAND);
	  obliterate_card(1-player, card_added);
	}

  return 0;
}

int card_hibernations_end(int player, int card, event_t event)
{
  /* Hibernation's End	|4|G
   * Enchantment
   * Cumulative upkeep |1
   * Whenever you pay ~'s cumulative upkeep, you may search your library for a creature card with converted mana cost equal to the number of age counters on ~
   * and put it onto the battlefield. If you do, shuffle your library. */

	cumulative_upkeep(player, card, event, MANACOST_X(1));

	if(event == EVENT_UPKEEP_TRIGGER_ABILITY && in_play(player, card)	// Cumulative upkeep was just successfully paid
	  && (IS_AI(player) || do_dialog(player, player, card, -1, -1, " Search for a creature\n Decline", 0) == 0))
	{
	  int counters = count_counters(player, card, COUNTER_AGE);
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  scnprintf(test.message, 100, "Select a creature card with CMC %d.", counters);
	  test.cmc = counters;
	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &test);
	}

  return global_enchantment(player, card, event);
}

int card_icefall(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND | TYPE_ARTIFACT);

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

int card_into_the_north(int player, int card, event_t event){

	char msg[100] = "Select a Snow land.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, msg);
	this_test.subtype = SUBTYPE_SNOW;

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_jesters_scepter(int player, int card, event_t event){

	/* Jester's Scepter	|3
	 * Artifact
	 * When ~ enters the battlefield, exile the top five cards of target player's library face down. You may look at those cards for as long as they remain
	 * exiled.
	 * |2, |T, Put a card exiled with ~ into its owner's graveyard: Counter target spell if it has the same name as that card. */

#pragma message "They're currently exiled face-up, so it can't possibly work in mutiplayer."

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			int leg = 0, idx = 0;
			int max = MIN(5, count_deck(instance->targets[0].player));
			while( max ){
				exiledby_remember(player, card, instance->targets[0].player, deck_ptr[instance->targets[0].player][0], &leg, &idx);
				rfg_card_in_deck(instance->targets[0].player, 0);
				max--;
			}
			instance->targets[1] = instance->targets[0];
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		int leg = 0;
		int idx = 0;
		if( exiledby_find_any(player, card, &leg, &idx) != NULL ){
			int result = generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SPELL_ON_STACK, MANACOST_X(2), 0, NULL, NULL);
			if( result ){
				if( player != AI ){
					return result;
				}
				else{
					int needle = get_original_internal_card_id(card_on_stack_controller, card_on_stack);
					needle |= (instance->targets[1].player == 1 ? 0x80000000 : 0);
					if( exiledby_find(player, card, needle, &leg, &idx) != NULL ){
						return result;
					}
				}
			}
			else{
				if( player == HUMAN ){
					return 1;
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( player == HUMAN ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SPELL_ON_STACK, MANACOST_X(2), 0, NULL, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Jester's Scepter ability\n Show exiled cards\n Cancel", 0);
			}
			else{
				choice = 1;
			}
		}
		if( choice == 1 ){
			exiledby_choose(player, card, CARD_ID_JESTERS_SCEPTER, EXBY_CHOOSE, 0, "", 1);
			spell_fizzled = 1;
			return 0;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == 0 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
				int iid = -1;
				if( player == HUMAN ){
					int rval = exiledby_choose(player, card, CARD_ID_JESTERS_SCEPTER, EXBY_CHOOSE, 0, "", 1);
					int* loc = (int*)rval;
					if( loc ){
						int *loc2 = exiledby_find(player, card, *loc, NULL, NULL);
						iid = *loc2 & ~0x80000000;
						*loc2 = -1;
					}
				}
				else{
					int leg = 0;
					int idx = 0;
					int* loc;
					while ((loc = exiledby_find_any(player, card, &leg, &idx)) != NULL){
							int iid2 = *loc & ~0x80000000;
							if( iid2 == get_original_internal_card_id(card_on_stack_controller, card_on_stack) ){
								*loc = -1;
								iid = iid2;
								break;
							}
							idx++;
					}
				}
				if( iid != -1 ){
					int pos = find_iid_in_rfg(instance->targets[1].player, iid);
					if( pos > -1 ){
						from_exile_to_graveyard(instance->targets[1].player, pos);
					}
					instance->targets[0].player = card_on_stack_controller;
					instance->targets[0].card = card_on_stack;
					instance->number_of_targets = 1;
					instance->info_slot = iid;
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( get_id(card_on_stack_controller, card_on_stack) == cards_data[instance->info_slot].id ){
			real_counter_a_spell(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_jokulmorder(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		if( can_sacrifice(player, player, 5, TYPE_LAND, 0)
			&& sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)){
			impose_sacrifice(player, card, player, 4, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	does_not_untap(player, card, event);

	if( specific_cip(player, card, event, player, RESOLVE_TRIGGER_DUH, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	return 0;
}

static int can_pay_jotun_grunt_upkeep(int player, int card, int number_of_age_counters)
{
  int gy[2] = { count_graveyard(0) / 2, count_graveyard(1) / 2 };
  return gy[0] + gy[1] >= number_of_age_counters;
}
static int pay_jotun_grunt_upkeep(int player, int card, int number_of_age_counters)
{
  int gy[2] = { count_graveyard(0) / 2, count_graveyard(1) / 2 };

  card_instance_t* instance = get_card_instance(player, card);

  test_definition_t test;
  default_test_definition(&test, 0);

  while (number_of_age_counters > 0 && gy[0] > 0 && gy[1] > 0)
	if (select_target_from_either_grave(player, card, SFG_NOTARGET, AI_MIN_VALUE, AI_MAX_VALUE, &test, 0, 1) != -1)
	  {
		from_graveyard_to_deck(instance->targets[0].player, instance->targets[1].player, 2);
		new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_GRAVE_NOTARGET, TUTOR_BOTTOM_OF_DECK, 1,
						 instance->targets[0].player == player ? AI_MIN_VALUE : AI_MAX_VALUE, &test);
		--number_of_age_counters;
		--gy[instance->targets[0].player];
	  }

  if (number_of_age_counters > 0)
	{
	  int who = gy[0] > 0 ? 0 : 1;
	  for (number_of_age_counters *= 2; number_of_age_counters > 0; --number_of_age_counters)
		new_global_tutor(player, who, TUTOR_FROM_GRAVE_NOTARGET, TUTOR_BOTTOM_OF_DECK, 1, who == player ? AI_MIN_VALUE : AI_MAX_VALUE, &test);
	}

  return 1;
}
int card_jotun_grunt(int player, int card, event_t event)
{
  /* Jotun Grunt	|1|W
   * Creature - Giant Soldier 4/4
   * Cumulative upkeep-Put two cards from a single graveyard on the bottom of their owner's library. */
  cumulative_upkeep_general(player, card, event, can_pay_jotun_grunt_upkeep, pay_jotun_grunt_upkeep);
  return 0;
}

int card_jotun_owl_keeper(int player, int card, event_t event)
{
  /* Jotun Owl Keeper	|2|W
   * Creature - Giant 3/3
   * Cumulative upkeep |W or |U
   * When ~ dies, put a 1/1 |Swhite Bird creature token with flying onto the battlefield for each age counter on it. */

  cumulative_upkeep_hybrid(player, card, event, 1, COLOR_WHITE, COLOR_BLUE, 0);

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	generate_tokens_by_id(player, card, CARD_ID_BIRD, count_counters(player, card, COUNTER_AGE));

  return 0;
}

int card_juniper_order_ranger(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  add_1_1_counter(instance->targets[1].player, instance->targets[1].card);
		  add_1_1_counter(player, card);
		}
	}

  return 0;
}

static int flip_coins(int player, int card, int number_of_age_counters)
{
  for (; number_of_age_counters > 0; --number_of_age_counters)
	flip_a_coin(player, card);
  return 1;
}
int card_karplusan_minotaur(int player, int card, event_t event)
{
  /* Karplusan Minotaur	|2|R|R
   * Creature - Minotaur Warrior 3/3
   * Cumulative upkeep-Flip a coin.
   * Whenever you win a coin flip, ~ deals 1 damage to target creature or player.
   * Whenever you lose a coin flip, ~ deals 1 damage to target creature or player of an opponent's choice. */

  // Coin flip triggers are in flip_a_coin() itself.

  cumulative_upkeep_general(player, card, event, rtrue, flip_coins);

  return 0;
}

int card_karplusan_wolverine(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DECLARE_BLOCKERS ){
		int good = 0;
		if( current_turn == player ){
			if( is_attacking(player, card) ){
				int count = active_cards_count[1-player]-1;
				while( count > -1 ){
						if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
							card_instance_t *this = get_card_instance(1-player, count);
							if( this->blocking == card ){
								good = 1;
								break;
							}
						}
						count--;
				}
			}
		}
		if( good == 1 ){
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				instance->number_of_targets = 1;
				damage_creature_or_player(player, card, event, 1);
			}
		}
	}

	return 0;
}

int card_kjeldoran_javelineer(int player, int card, event_t event){

	/* Kjeldoran Javelineer	|W
	 * Creature - Human Soldier 1/2
	 * Cumulative upkeep |1
	 * |T: ~ deals damage equal to the number of age counters on it to target attacking or blocking creature. */

	cumulative_upkeep(player, card, event, MANACOST_X(1));

	if (!IS_GAA_EVENT(event)){
	  return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			damage_target0(player, card, count_counters(player, card, COUNTER_AGE));
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_CREATURE");
}

int card_kjeldoran_war_cry(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = count_graveyard_by_id(player, get_id(player, card))+1;
		pump_subtype_until_eot(player, card, player, -1, amount, amount, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_krovikan_rot(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_krovikan_whispers(int player, int card, event_t event)
{
  /* Krovikan Whispers	|3|U
   * Enchantment - Aura
   * Enchant creature
   * Cumulative upkeep |U or |B
   * You control enchanted creature.
   * When ~ is put into a graveyard from the battlefield, you lose 2 life for each age counter on it. */

  cumulative_upkeep_hybrid(player, card, event, 1, COLOR_BLUE, COLOR_BLACK, 0);

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	lose_life(player, 2 * count_counters(player, card, COUNTER_AGE));

  return card_control_magic(player, card, event);
}

int card_lightning_serpent(int player, int card, event_t event){

	/* Lightning Serpent	|X|R
	 * Creature - Elemental Serpent 2/1
	 * Trample, haste
	 * ~ enters the battlefield with X +1/+0 counters on it.
	 * At the beginning of the end step, sacrifice ~. */

	card_instance_t* instance = get_card_instance(player, card);

	haste(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P0, instance->info_slot);

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_lovisa_coldeyes(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // Each creature that's a Barbarian, a Warrior, or a Berserker gets +2/+2 and has haste.
  if ((event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && (has_subtype(affected_card_controller, affected_card, SUBTYPE_BARBARIAN)
		  || has_subtype(affected_card_controller, affected_card, SUBTYPE_BERSERKER)
		  || has_subtype(affected_card_controller, affected_card, SUBTYPE_WARRIOR)))
	{
	  if (event == EVENT_ABILITIES)
		haste(affected_card_controller, affected_card, event);
	  else
		event_result += 2;
	}

  return 0;
}

int card_magmatic_core(int player, int card, event_t event){

	/* Magmatic Core	|2|R|R
	 * Enchantment
	 * Cumulative upkeep |1
	 * At the beginning of your end step, ~ deals X damage divided as you choose among any number of target creatures, where X is the number of age counters on
	 * it. */

	cumulative_upkeep(player, card, event, 1, 0, 0, 0, 0, 0);

	if( count_counters(player, card, COUNTER_AGE) > 0 && current_turn == player && eot_trigger(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		target_and_divide_damage(player, card, &td, "TARGET_CREATURE", count_counters(player, card, COUNTER_AGE));
	}

	return global_enchantment(player, card, event);
}

int card_martyr_of_the_ashes(int player, int card, event_t event){

	char buffer[100];
	scnprintf(buffer, 100, "Select a red card");
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, buffer);
	this_test.color = COLOR_TEST_RED;
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		test_definition_t this_test2;
		default_test_definition(&this_test2, TYPE_CREATURE);
		this_test2.keyword = KEYWORD_FLYING;
		this_test2.keyword_flag = 1;
		new_damage_all(player, card, 2, instance->targets[1].player, 0, &this_test2);
	}

	return martyr(player, card, event, 2, &this_test, NULL, 0);
}

int card_martyr_of_bones(int player, int card, event_t event){

	char buffer[100];
	scnprintf(buffer, 100, "Select a black card");
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, buffer);
	this_test.color = COLOR_TEST_BLACK;
	this_test.zone = TARGET_ZONE_HAND;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		int i;
		for(i=instance->targets[1].player-1; i>1; i--){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, i);
			if( selected != -1 ){
				rfg_card_from_grave(instance->targets[0].player, selected);
			}
		}
	}

	return martyr(player, card, event, 1, &this_test, &td, "TARGET_PLAYER");
}

int card_martyr_of_frost(int player, int card, event_t event){

	char buffer[100];
	scnprintf(buffer, 100, "Select a blue card");
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, buffer);
	this_test.color = COLOR_TEST_BLUE;
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		charge_mana(instance->targets[0].player, COLOR_COLORLESS, instance->targets[1].player);
		if( spell_fizzled == 1 ){
			set_flags_when_spell_is_countered(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		}
	}

	return martyr(player, card, event, 2, &this_test, NULL, 0);
}

int card_martyr_of_sands(int player, int card, event_t event){

	char buffer[100];
	scnprintf(buffer, 100, "Select a white card");
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, buffer);
	this_test.color = COLOR_TEST_WHITE;
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		gain_life(player, instance->targets[1].player*3);
	}

	return martyr(player, card, event, 1, &this_test, NULL, 0);
}

int card_martyr_of_spores(int player, int card, event_t event){

	char buffer[100];
	scnprintf(buffer, 100, "Select a green card");
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, buffer);
	this_test.color = COLOR_TEST_GREEN;
	this_test.zone = TARGET_ZONE_HAND;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].player);
		}
	}

	return martyr(player, card, event, 1, &this_test, &td, "TARGET_CREATURE");
}

int card_mouth_of_ronom(int player, int card, event_t event){

	/* Mouth of Ronom	""
	 * Snow Land
	 * |T: Add |1 to your mana pool.
	 * |4|I, |T, Sacrifice ~: ~ deals 4 damage to target creature. */

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_COUNT_MANA && !(is_tapped(player, card)) && affect_me(player, card) ){
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 2;
	td.preferred_controller = 1-player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_ACTIVATE){
		if (!CAN_TAP(player, card)){
			return 0;
		}
		if (can_produce_mana(player, card)){
			return 1;
		}
		int c1 = get_cost_mod_for_activated_abilities(player, card, 4, 0, 0, 0, 0, 0);
		return can_use_activated_abilities(player, card) && has_snow_mana(player, 1, c1) && can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int c1 = get_cost_mod_for_activated_abilities(player, card, 4, 0, 0, 0, 0, 0);
		if( !paying_mana() && can_use_activated_abilities(player, card) && can_target(&td) && has_snow_mana(player, 1, c1) ){
			choice = do_dialog(player, player, card, -1, -1, " Generate 1\n Sac, Deal 4\n Cancel", 1);
		}
		instance->info_slot = choice;
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		if( choice == 1 ){
			if( ! charge_snow_mana(player, card, 1, c1) ){
				spell_fizzled = 1;
			}
			else{
				if( pick_target(&td, "TARGET_CREATURE") ){
					tap_card(player, card);
					kill_card(player, card, KILL_SACRIFICE);
				}
			}
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 1 ){
				if( valid_target(&td) ){
					damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, instance->parent_card);
				}
			}
	}

	return 0;
}

int card_ohran_viper(int player, int card, event_t event){
	// original code : 012053FC

	/* Ohran Viper	|1|G|G
	 * Snow Creature - Snake 1/3
	 * Whenever ~ deals combat damage to a creature, destroy that creature at end of combat.
	 * Whenever ~ deals combat damage to a player, you may draw a card. */

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRIGGER_OPTIONAL) ){
		draw_a_card(player);
	}

	if( ! is_humiliated(player, card) ){
		if (event == EVENT_CHANGE_TYPE && affect_me(player, card) ){
			card_instance_t* instance = get_card_instance(player, card);
			instance->destroys_if_blocked |= DIFB_DESTROYS_UNPROTECTED;
		}

		for_each_creature_damaged_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE, &lowland_basilisk_effect, player, card);
	}

	return 0;
}

int card_perilous_researches(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			draw_cards(player, 2);
			sacrifice(player, card, player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_phobian_phantasm(int player, int card, event_t event){

  cumulative_upkeep(player, card, event, 0, 1, 0, 0, 0, 0);

  fear(player, card, event);

  return 0;
}

int card_phyrexian_etchings(int player, int card, event_t event){

  /* Phyrexian Etchings	|B|B|B
   * Enchantment
   * Cumulative upkeep |B
   * At the beginning of your end step, draw a card for each age counter on ~.
   * When ~ is put into a graveyard from the battlefield, you lose 2 life for each age counter on it. */

  cumulative_upkeep(player, card, event, 0, 1, 0, 0, 0, 0);

  if (current_turn == player && eot_trigger(player, card, event))
	draw_cards(player, count_counters(player, card, COUNTER_AGE));

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	lose_life(player, 2 * count_counters(player, card, COUNTER_AGE));

  return global_enchantment(player, card, event);
}

int card_phyrexian_ironfoot(int player, int card, event_t event){

	/* Phyrexian Ironfoot	|3
	 * Snow Artifact Creature - Construct 3/4
	 * ~ doesn't untap during your untap step.
	 * |1|I: Untap ~. */

	does_not_untap(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 0, 0);
		return can_use_activated_abilities(player, card) && has_snow_mana(player, 1, c1);
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 0, 0);
		charge_snow_mana(player, card, 1, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		untap_card(player, instance->parent_card);
	}

	return 0;
}

static int can_sacrifice_creatures(int player, int card, int number_of_age_counters)
{
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.qty = number_of_age_counters;

	return new_can_sacrifice(player, card, player, &this_test);
}

static int sacrifice_creatures(int player, int card, int number_of_age_counters)
{
  return impose_sacrifice(player, card, player, number_of_age_counters, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0) == number_of_age_counters;
}
int card_phyrexian_soulgorger(int player, int card, event_t event)
{
  /* Phyrexian Soulgorger	|3
   * Snow Artifact Creature - Construct 8/8
   * Cumulative upkeep-Sacrifice a creature. */
  cumulative_upkeep_general(player, card, event, can_sacrifice_creatures, sacrifice_creatures);
  return 0;
}

int card_resize(int player, int card, event_t event){//UNUSEDCARD
	/* Resize	|1|G
	 * Instant
	 * Target creature gets +3/+3 until end of turn.
	 * Recover |1|G */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 3, 3, 0, 0);
}

static int effect_rt(int player, int card, event_t event){

  card_instance_t *instance = get_card_instance(player, card);

  if(event == EVENT_BLOCK_LEGALITY ){
	 if( instance->targets[0].player == attacking_card_controller && instance->targets[0].card == attacking_card ){
		 if( ! has_subtype(affected_card_controller, affected_card, SUBTYPE_SNOW) ){
			 event_result = 1;
		 }
	 }
  }

  if( eot_trigger(player, card, event) ){
	  kill_card(player, card, KILL_REMOVE);
  }

  return 0;
}

int card_rime_transfusion(int player, int card, event_t event){

	/* Rime Transfusion	|1|B
	 * Snow Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +2/+1 and has "|I: This creature can't be blocked this turn except by snow creatures." */

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(t_player, t_card) ){
			int c1 = get_cost_mod_for_activated_abilities(t_player, t_card, 0, 0, 0, 0, 0, 0);
			if( has_snow_mana(player, 1, c1) ){
				return 1;
			}
		}

		if( event == EVENT_ACTIVATE ){
			int c1 = get_cost_mod_for_activated_abilities(t_player, t_card, 0, 0, 0, 0, 0, 0);
			charge_snow_mana(t_player, t_card, 1, c1);
		}

		if( event == EVENT_RESOLVE_ACTIVATION){
			create_targetted_legacy_effect(player, instance->parent_card, &effect_rt, t_player, t_card);
		}
	}
	return generic_aura(player, card, event, player, 2,1, 0,0, 0,0,0);
}

int card_rimebound_dead(int player, int card, event_t event){

	/* Rimebound Dead	|B
	 * Snow Creature - Skeleton 1/1
	 * |I: Regenerate ~. */

	if( land_can_be_played & LCBP_REGENERATION ){
		if( event == EVENT_CAN_ACTIVATE ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 0, 0);
			if (can_use_activated_abilities(player, card) && has_snow_mana(player, 1, c1)){
				return can_regenerate(player, card);
			}
		}
		else if( event == EVENT_ACTIVATE ){
				int c1 = get_cost_mod_for_activated_abilities(player, card, 0, 0, 0, 0, 0, 0);
				charge_snow_mana(player, card, 1, c1);
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				card_instance_t *instance = get_card_instance( player, card );
				if ( can_regenerate(instance->parent_controller, instance->parent_card) ){
					regenerate_target(instance->parent_controller, instance->parent_card);
				}
		}
	}
	return 0;
}

int card_rimefeather_owl(int player, int card, event_t event){

	/* Rimefeather Owl	|5|U|U
	 * Snow Creature - Bird 100/100
	 * Flying
	 * ~'s power and toughness are each equal to the number of snow permanents on the battlefield.
	 * |1|I: Put an ice counter on target permanent.
	 * Permanents with ice counters on them are snow. */

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		event_result += count_subtype(ANYBODY, TYPE_PERMANENT, SUBTYPE_SNOW);
	}

	// Snow supertype added by a hack in has_subtype(), rather than add_a_subtype(), so that it works on ice counters added by other cards.

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 0, 0);
		if( has_snow_mana(player, 1, c1) ){
			return can_target(&td1);
		}
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 0, 0);
		if( charge_snow_mana(player, card, 1, c1) ){
			if( pick_target(&td1, "TARGET_PERMANENT") ){
				instance->number_of_targets = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td1) ){
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_ICE);
		}
	}

	return 0;
}

int card_rimescale_dragon(int player, int card, event_t event){

	/* Rimescale Dragon	|5|R|R
	 * Snow Creature - Dragon 5/5
	 * Flying
	 * |2|I: Tap target creature and put an ice counter on it.
	 * Creatures with ice counters on them don't untap during their controllers' untap steps. */

	if (current_phase == PHASE_UNTAP && event == EVENT_UNTAP &&
		count_counters(affected_card_controller, affected_card, COUNTER_ICE) && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	   ){
		get_card_instance(affected_card_controller, affected_card)->untap_status &= ~3;
	}

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 2, 0, 0, 0, 0, 0);
		if( has_snow_mana(player, 1, c1) ){
			return can_target(&td1);
		}
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 2, 0, 0, 0, 0, 0);
		if( charge_snow_mana(player, card, 1, c1) && can_target(&td1) && pick_target(&td1, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
		} else {
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td1) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_ICE);
		}
	}

	return 0;
}

int card_rimewind_taskmage(int player, int card, event_t event)
{
  // |1, |T: You may tap or untap target permanent. Activate this ability only if you control four or more snow permanents.
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);

  if (event == EVENT_CAN_ACTIVATE && count_subtype(player, TYPE_PERMANENT, SUBTYPE_SNOW) < 4)
	return 0;

  int rval = generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(1), 0, &td, "TARGET_PERMANENT");

  if (event == EVENT_ACTIVATE && player == AI && cancel != 1)
	ai_modifier_twiddle(player, card, 0);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	twiddle(player, card, 0);

  return rval;
}

int card_rite_of_flame(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	if(event == EVENT_RESOLVE_SPELL ){
		int count = count_graveyard_by_id(HUMAN, CARD_ID_RITE_OF_FLAME);
		count += count_graveyard_by_id(AI, CARD_ID_RITE_OF_FLAME);
		produce_mana(player, COLOR_RED, 2 + count );
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_ronom_unicorn(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td1) ){
			kill_card( instance->targets[0].player,instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_ENCHANTMENT");
}

int card_rune_snag(int player, int card, event_t event){
	/* Rune Snag	|1|U
	 * Instant
	 * Counter target spell unless its controller pays |2 plus an additional |2 for each card named ~ in each graveyard. */

	if( event == EVENT_RESOLVE_SPELL ){
			int amount = 1 + count_graveyard_by_id(player, CARD_ID_RUNE_SNAG);
			amount += count_graveyard_by_id(1-player, CARD_ID_RUNE_SNAG);
			amount *= 2;

			counterspell_resolve_unless_pay_x(player, card, NULL, 0, amount);

			kill_card(player, card, KILL_DESTROY);
	}
	else{
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_scrying_sheets(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if(  can_use_activated_abilities(player, card) ){
			int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 0, 0);
			if( has_snow_mana(player, 1, c1) ){
				choice = do_dialog(player, player, card, -1, -1, " Generate 1\n Snow Scry\n Cancel", 1);
			}
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				tap_card(player, card);
				int c1 = get_cost_mod_for_activated_abilities(player, card, 1, 0, 0, 0, 0, 0);
				if( ! charge_snow_mana(player, card, 1, c1) ){
					spell_fizzled = 1;
					untap_card_no_event(player, card);
				}
				else{
					instance->info_slot = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *parent = get_card_instance(instance->parent_controller, instance->parent_card);
			if( instance->info_slot == 1){
				int *deck = deck_ptr[player];
				show_deck( player, deck, 1, "This is the first card of your library", 0, 0x7375B0 );
				if( has_subtype_by_id(cards_data[deck[0]].id, SUBTYPE_SNOW) ){
					add_card_to_hand(player, deck[0]);
					remove_card_from_deck(player, 0);
				}
				parent->info_slot = 0;
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

int card_sek_kuar_death_keeper(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.type_flag = F1_NO_TOKEN;
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && affect_me(player, card) && reason_for_trigger_controller == player &&
		instance->kill_code < KILL_DESTROY
	  ){
		if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_GRAVEBORN, &token);
			token.pow = 3;
			token.tou = 1;
			token.qty = instance->targets[11].card;
			generate_token(&token);
			instance->targets[11].card = 0;
		}
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( instance->targets[11].player > 0 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_GRAVEBORN, &token);
			token.pow = 3;
			token.tou = 1;
			token.qty = instance->targets[11].player;
			generate_token(&token);
		}
	}

	return 0;
}

int card_shape_of_the_wiitigo(int player, int card, event_t event)
{
  card_instance_t* instance = in_play(player, card);

  // Approximation: Checks whether any creature it was attached to has attacked or blocked since last upkeep, not the one it's currently attached to.

  if (instance && instance->damage_target_card >= 0)
	{
	  // When ~ enters the battlefield, put six +1/+1 counters on enchanted creature.
	  if (comes_into_play(player, card, event) && in_play(instance->damage_target_player, instance->damage_target_card))
		add_1_1_counters(instance->damage_target_player, instance->damage_target_card, 6);

	  /* At the beginning of your upkeep, put a +1/+1 counter on enchanted creature if it attacked or blocked since your last upkeep. Otherwise, remove a +1/+1
	   * counter from it. */
	  upkeep_trigger_ability(player, card, event, player);

	  if (event == EVENT_UPKEEP_TRIGGER_ABILITY && in_play(instance->damage_target_player, instance->damage_target_card))
		{
		  if (instance->targets[1].player == 66)
			{
			  add_1_1_counter(instance->damage_target_player, instance->damage_target_card);
			  instance->targets[1].player = 0;
			}
		  else
			remove_1_1_counter(instance->damage_target_player, instance->damage_target_card);
		}

	  card_instance_t* aff;
	  if ((event == EVENT_DECLARE_ATTACKERS && (aff = in_play(instance->damage_target_player, instance->damage_target_card)) && (aff->state & STATE_ATTACKING))
		  || (event == EVENT_DECLARE_BLOCKERS && in_play(instance->damage_target_player, instance->damage_target_card) && blocking(player, card, event)))
		instance->targets[1].player = 66;
	}

  // Enchant creature
  return vanilla_aura(player, card, event, player);
}

static int can_put_p1_p1_counter_on_opp(int player, int card, int number_of_age_counters)
{
  target_definition_t td;
  base_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = td.preferred_controller = 1-player;
  td.extra = get_internal_card_id_from_csv_id(CARD_ID_MELIRAS_KEEPERS);
  td.special = TARGET_SPECIAL_EXTRA_NOT_IID;
  return can_target(&td);
}
static int put_p1_p1_counter_on_opp(int player, int card, int number_of_age_counters)
{
  target_definition_t td;
  base_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = td.preferred_controller = 1-player;
  td.extra = get_internal_card_id_from_csv_id(CARD_ID_MELIRAS_KEEPERS);
  td.special = TARGET_SPECIAL_EXTRA_NOT_IID;
  td.allow_cancel = 0;

  int i;
  card_instance_t* instance = get_card_instance(player, card);
  instance->number_of_targets = 0;
  for (i = 0; i < number_of_age_counters; ++i)
	if (can_target(&td) && pick_next_target_noload(&td, "Select a creature an opponent controls."))
	  {
		add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		instance->number_of_targets = 0;
	  }
	else	// Perhaps the only creature has a trigger like Phyrexian Devourer's, and checks it continuously instead of only during EVENT_STATIC_EFFECTS.
	  return 0;

  return 1;
}
int card_sheltering_ancient(int player, int card, event_t event)
{
  /* Sheltering Ancient	|1|G
   * Creature - Treefolk 5/5
   * Trample
   * Cumulative upkeep-Put a +1/+1 counter on a creature an opponent controls. */
  cumulative_upkeep_general(player, card, event, can_put_p1_p1_counter_on_opp, put_p1_p1_counter_on_opp);
  return 0;
}

int card_skred(int player, int card, event_t event)
{
  /* Skred	|R
   * Instant
   * ~ deals damage to target creature equal to the number of snow permanents you control. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, count_snow_permanents(player, TYPE_PERMANENT, 0));

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_soul_spike(int player, int card, event_t event){

	if( event != EVENT_MODIFY_COST && ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select a black card to exile");
	this_test.color = COLOR_TEST_BLACK;
	this_test.zone = TARGET_ZONE_HAND;
	this_test.id = CARD_ID_RULES_ENGINE;
	this_test.id_flag = 1;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	cost_mod_for_coldsnap_pitchspell(player, card, event, &this_test);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td1, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! casting_coldsnap_pitchspell(player, card, event, &this_test) ){
			spell_fizzled = 1;
			return 0;
		}
		pick_target(&td1, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
			gain_life(player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sound_the_call(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WOLF, &token);
		token.pow = 1;
		token.tou = 1;
		token.special_infos = 66;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_squall_drifter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 1, 0, &td, "TARGET_CREATURE");
}

int card_stalking_yeti(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
			int my_power = get_power(player, card);
			int his_power = get_power(instance->targets[0].player, instance->targets[0].card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, my_power, player, card);
			damage_creature(player, card, his_power, instance->targets[0].player, instance->targets[0].card);
		}
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_sorcery_be_played(player, event) ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 2, 0, 0, 0, 0, 0);
		if( has_snow_mana(player, 1, c1) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int c1 = get_cost_mod_for_activated_abilities(player, card, 2, 0, 0, 0, 0, 0);
		charge_snow_mana(player, card, 1, c1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		bounce_permanent(player, instance->parent_card);
	}

	return 0;
}

int card_stromgald_crusader(int player, int card, event_t event){

	return coldsnap_crusaders(player, card, event, COLOR_BLACK);
}

int card_suns_bounty(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 4);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sunscour(int player, int card, event_t event){
	if( event != EVENT_MODIFY_COST && ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select a white card to exile");
	this_test.color = COLOR_TEST_WHITE;
	this_test.zone = TARGET_ZONE_HAND;
	this_test.id = CARD_ID_RULES_ENGINE;
	this_test.id_flag = 1;

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test2;
		default_test_definition(&this_test2, TYPE_CREATURE);
		new_manipulate_all(player, card, 2, &this_test2, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return coldsnap_pitchspell(player, card, event, &this_test);
}

int card_surging_aether(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PERMANENT") ){
			ripple(player, card);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_surging_dementia(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			ripple(player, card);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			discard(instance->targets[0].player, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_surging_flame(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			ripple(player, card);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_surging_might(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			ripple(player, card);
		}
	}
	else{
		return generic_aura(player, card, event, player, 2, 2, 0, 0, 0, 0, 0);
	}

	return 0;
}

int card_surging_sentinels(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && spell_fizzled != 1){
		ripple(player, card);
	}

	return 0;
}

int card_tamanoa(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( source->damage_source_player != player ){
			return 0;
		}

		if( damage_card != source->internal_card_id ){
			return 0;
		}

		if( source->info_slot <= 0 ){
			return 0;
		}

		if( ! in_play( source->damage_source_player, source->damage_source_card ) ){
			gain_life(player, source->info_slot);
		}

		else{
			card_instance_t *damage_source = get_card_instance(source->damage_source_player, source->damage_source_card);
			if (!(cards_data[damage_source->internal_card_id].type & TYPE_CREATURE)){
				gain_life(player, source->info_slot);
			}
		}
	}

	return 0;

}

int card_thermopod(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_activated_abilities(player, card) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return 1;
		}
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return has_snow_mana(player, 1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->info_slot = 0;
		int choice = 0;
		if( can_use_activated_abilities(player, card) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST0, 0, NULL, NULL) && has_snow_mana(player, 1, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Give Haste\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				produce_mana(player, COLOR_RED, 1);
			}
			else{
				spell_fizzled = 1;
			}
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
					if( charge_snow_mana(player, card, 1, 0) ){
						get_card_instance(player, card)->info_slot = 1;
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
		card_instance_t *instance= get_card_instance(player, card);
		if( instance->info_slot == 1 ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
									0, 0, 0, SP_KEYWORD_HASTE);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_use_activated_abilities(player, card) &&
		can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		declare_mana_available(player, COLOR_RED, 1);
	}

	return 0;
}

int card_thrummingstone(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( specific_spell_played(player, card, event, player, 2, TYPE_EFFECT, 1, 0, 0, 0, 0, 0, 0, -1, 0) ){
		ripple(instance->targets[1].player, instance->targets[1].card);
	}

	return 0;
}

int card_vanish_into_memory(int player, int card, event_t event){

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
			draw_cards(player, get_power(instance->targets[0].player, instance->targets[0].card));
			remove_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int can_discard_n(int player, int card, int number_of_age_counters)
{
  return hand_count[player] >= number_of_age_counters;
}
static int multidiscard_thunk(int player, int card, int number_of_age_counters)
{
  multidiscard(player, number_of_age_counters, 0);
  return 1;
}
int card_vexing_sphinx(int player, int card, event_t event)
{
  /* Vexing Sphinx	|1|U|U
   * Creature - Sphinx 4/4
   * Flying
   * Cumulative upkeep-Discard a card.
   * When ~ dies, draw a card for each age counter on it. */

  cumulative_upkeep_general(player, card, event, can_discard_n, multidiscard_thunk);

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	draw_cards(player, count_counters(player, card, COUNTER_AGE));

  return 0;
}

static int opp_can_gain_life_as_cost(int player, int card, int number_of_age_counters)
{
  return !check_battlefield_for_id(1-player, CARD_ID_PLATINUM_EMPERION);
}
static int opp_gains_life(int player, int card, int number_of_age_counters)
{
  gain_life(1-player, number_of_age_counters);
  return 1;
}
int card_wall_of_shards(int player, int card, event_t event)
{
  /* Wall of Shards	|1|W
   * Snow Creature - Wall 1/8
   * Defender, flying
   * Cumulative upkeep-An opponent gains 1 life. */
  cumulative_upkeep_general(player, card, event, opp_can_gain_life_as_cost, opp_gains_life);
  return 0;
}

int card_white_shield_crusader(int player, int card, event_t event){

	return coldsnap_crusaders(player, card, event, COLOR_WHITE);
}

int card_void_maw(int player, int card, event_t event){

	/* Void Maw	|4|B|B
	 * Creature - Horror 4/5
	 * Trample
	 * If another creature would die, exile it instead.
	 * Put a card exiled with ~ into its owner's graveyard: ~ gets +2/+2 until end of turn. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( !affect_me(player, card) && in_play(affected_card_controller, affected_card) && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE){
				int leg = 0, idx = 0;
				if (!is_token(affected_card_controller, affected_card)){
					exiledby_remember(player, card, get_owner(affected_card_controller, affected_card),
										get_original_internal_card_id(affected_card_controller, affected_card), &leg, &idx);
				}
				affected->kill_code = KILL_REMOVE;
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return exiledby_choose(player, card, CARD_ID_VOID_MAW, EXBY_FIRST_FOUND, 0, "Creature", 1);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int rval = exiledby_choose(player, card, CARD_ID_VOID_MAW, EXBY_CHOOSE, 0, "Creature", 1);
			int* loc = (int*)rval;
			if( loc ){
				int *loc2 = exiledby_find(player, card, *loc, NULL, NULL);
				int owner = *loc2 & 0x80000000;
				int iid = *loc2 & ~0x80000000;
				int pos = find_iid_in_rfg(owner, iid);
				if( pos > -1 ){
					from_exile_to_graveyard(owner, pos);
				}
				*loc2 = -1;
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

int card_wilderness_elemental(int player, int card, event_t event){
	/* Wilderness Elemental	|1|R|G
	 * Creature - Elemental 100/3
	 * Trample
	 * ~'s power is equal to the number of nonbasic lands your opponents control. */

	if( event == EVENT_POWER && affect_me(player, card) && player != -1){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.subtype = SUBTYPE_BASIC;
		this_test.subtype_flag = DOESNT_MATCH;
		event_result += check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test);
	}

	return 0;
}

int card_woolly_razorback(int player, int card, event_t event){

	/* Woolly Razorback	|2|W|W
	 * Creature - Boar Beast 7/7
	 * ~ enters the battlefield with three ice counters on it.
	 * As long as ~ has an ice counter on it, prevent all combat damage it would deal and it has defender.
	 * Whenever ~ blocks, remove an ice counter from it. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_ICE, 3);

	if (event == EVENT_ABILITIES && affect_me(player, card) && count_counters(player, card, COUNTER_ICE) > 0){
		event_result |= KEYWORD_DEFENDER;
	}

	card_instance_t* damage = combat_damage_being_prevented(event);
	if (damage &&
		damage->damage_source_player == player && damage->damage_source_card == card &&
		count_counters(player, card, COUNTER_ICE) > 0
	   ){
		damage->info_slot = 0;
	}

	if( event == EVENT_DECLARE_BLOCKERS && count_counters(player, card, COUNTER_ICE) > 0 && blocking(player, card, event) ){
		remove_counter(player, card, COUNTER_ICE);
	}

	return 0;
}

int card_zur_the_enchanter(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	/* Whenever ~ attacks, you may search your library for an enchantment card with converted mana cost 3 or less and put it onto the battlefield. If you do,
	 * shuffle your library. */
	if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card)){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select an enchantment with CMC 3 or less.");
		this_test.cmc = 4;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

