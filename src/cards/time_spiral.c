#include "manalink.h"

// Functions
int generic_suspend_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if (instance->targets[7].player == 42){	// Prevent reentrance while resolving
		return 0;
	}

	if( (current_turn == instance->targets[6].player || instance->targets[9].card == CARD_ID_CURSE_OF_THE_CABAL) &&
		upkeep_trigger(player, card, event)
	  ){
		int total = count_upkeeps(current_turn);
		if( instance->targets[9].card == CARD_ID_CURSE_OF_THE_CABAL ){
			for (; total > 0 && count_counters(player, card, COUNTER_TIME) > 0; --total){
					if (instance->targets[6].card == 0){
						instance->targets[6].card = 1;	// suspended during a previous upkeep
						continue;
					}
					int ai_choice = 1;
					if( count_counters(player, card, COUNTER_TIME) < 2 && instance->targets[6].player != player ){
						ai_choice = 0;
					}
					int choice = ai_choice;
					if( choice == 0 || player != AI ){
						choice = do_dialog(current_turn, player, card, -1, -1, " Add 2 time counters to Curse of the Cabal\n Pass", ai_choice);
					}
					if( choice == 0 ){
						if( sacrifice(player, card, current_turn, 0, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
							add_counters(player, card, COUNTER_TIME, 2);
						}
					}
					if( current_turn == instance->targets[6].player ){
						remove_counter(player, card, COUNTER_TIME);
					}
			}
		}
		else{
			for (; total > 0 && count_counters(player, card, COUNTER_TIME) > 0; --total){
					if (instance->targets[6].card == 0){
						instance->targets[6].card = 1;	// suspended during a previous upkeep
						continue;
					}
					remove_counter(player, card, COUNTER_TIME);
			}
		}
	}
	if( instance->targets[9].card == CARD_ID_NIHILITH ){
		if (when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(player, card, event, 1-player, RESOLVE_TRIGGER_AI(player), NULL)){
			remove_counter(player, card, COUNTER_TIME);
		}

		enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
		if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player && trigger_cause_controller == 1-player){
			if (event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_AI(player);
			}
			if (event == EVENT_RESOLVE_TRIGGER){
				remove_counters(player, card, COUNTER_TIME, num_cards_milled);
			}
		}
	}
	if( instance->targets[9].card == CARD_ID_DEEP_SEA_KRAKEN &&
		specific_spell_played(player, card, event, 1-player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		remove_counter(player, card, COUNTER_TIME);
	}
	if( instance->targets[9].card == CARD_ID_PARDIC_DRAGON &&
		specific_spell_played(player, card, event, 1-player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		int choice = 0;
		if( ! duh_mode(1-player) ){
			choice = do_dialog(1-player, player, card, -1, -1, " Put a time counter on Pardic Dragon\n Pass", 0);
		}
		if( choice == 0 ){
			add_counter(player, card, COUNTER_TIME);
		}
	}
	if( event == EVENT_STATIC_EFFECTS && count_counters(player, card, COUNTER_TIME) < 1){
		instance->targets[7].player = 42;	// Prevent reentrance while resolving
		if( check_rfg(instance->targets[6].player, instance->targets[9].card) ){
			int crd = play_card_in_exile_for_free(instance->targets[6].player, instance->targets[6].player, instance->targets[9].card);
			if( is_what(instance->targets[6].player, crd, TYPE_CREATURE) ){
				give_haste(instance->targets[6].player, crd);
			}
			if (instance->targets[9].card == CARD_ID_RIFTMARKED_KNIGHT){
			  token_generation_t token;
			  default_token_definition(player, card, CARD_ID_KNIGHT, &token);
			  token.key_plus = get_sleighted_protection(player, card, KEYWORD_PROT_WHITE);	// I suppose it could be cast normally, then Sleight of Minded, then Delayed?
			  token.s_key_plus = SP_KEYWORD_FLANKING | SP_KEYWORD_HASTE;
			  token.color_forced = COLOR_TEST_BLACK;
			  generate_token(&token);
			}
		}
		kill_card(player, card, KILL_REMOVE);
		recalculate_all_cards_in_play();	// since this is an effect (kill_card_guts() calls this for permanents)
	}

	if (instance->targets[6].card == 0 && current_phase != PHASE_BEGIN_UPKEEP){
		instance->targets[6].card = 1;
	}
	return 0;
}

int is_suspend_legacy(int player, int card){
	if( is_what(player, card, TYPE_EFFECT) ){
		return get_card_instance(player, card)->info_slot == (int)generic_suspend_legacy ? 1 : 0;
	}
	return 0;
}

int suspend_a_card(int player, int card, int t_player, int t_card, int turns){
	int csvid;
	if( t_player != -1 ){
		csvid = get_id(t_player, t_card);
	}
	else{
		csvid = cards_data[t_card].id;
	}
	if( csvid != CARD_ID_GREATER_GARGADON ){
		int sus = alternate_legacy_text(1, player, create_legacy_effect(player, card, &generic_suspend_legacy));
		card_instance_t *suspended = get_card_instance(player, sus);
		suspended->targets[6].card = 0;	// prevent cards suspended during an upkeep trigger from immediately losing time counters
		suspended->targets[6].player = t_player != -1 ? t_player : player;
		suspended->targets[7].player = 0;
		suspended->targets[7].card = get_id(player, card);
		add_counters(player, sus, COUNTER_TIME, turns);
		suspended->targets[9].card = csvid;
	}
	else{
		int card_added = add_card_to_hand(player, get_internal_card_id_from_csv_id(CARD_ID_GREATER_GARGADON_SUSPENDED));
		add_counters(player, card_added, COUNTER_TIME, turns);
		put_into_play(player, card_added);
	}
	if( t_player != -1 ){
		kill_card(t_player, t_card, KILL_REMOVE);
	}
	return 0;
}

int suspend(int player, int card, event_t event, int turns, int colorless, int black, int blue, int green, int red, int white, target_definition_t* td, const char* prompt){
	if( event == EVENT_MODIFY_COST ){
		card_instance_t* instance = get_card_instance(player, card);
		int csvid = get_id(player, card);
		int cless = colorless;
		if( cless == -1 ){
			cless = 0;
		}
		if (csvid == CARD_ID_DETRITIVORE ||
			csvid == CARD_ID_BENALISH_COMMANDER ||
			csvid == CARD_ID_FUNGAL_BEHEMOTH ||
			csvid == CARD_ID_ROILING_HORROR
		  ){
			cless = 1; //Cannot be suspended with X = 0
		}
		if( has_mana_multi(player, cless, black, blue, green, red, white) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if (event == EVENT_CAN_CAST){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->info_slot && !played_for_free(player, card) && !is_token(player, card)){
			return 1;
		}
		return !td || can_target(td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		card_instance_t* instance = get_card_instance(player, card);
		enum{
			CHOICE_NORMAL = 1,
			CHOICE_SUSPEND
		} choice = CHOICE_NORMAL;
		int ff = played_for_free(player, card) | is_token(player, card);
		if( ! ff && instance->info_slot == 1){
			int cless = colorless;
			if (cless == -1){
				cless = 0;
			}
			int csvid = get_id(player, card);
			choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_AUTOCHOOSE_IF_1,
							"Play normally", has_mana_to_cast_id(player, event, csvid) && (!td || can_target(td)), 2,
							"Suspend", has_mana_multi(player, cless, black, blue, green, red, white), 1);
		}
		if(choice == CHOICE_NORMAL){
			if(instance->info_slot == 1 && ! ff ){
				if( ! charge_mana_from_id(player, card, event, get_id(player, card)) ){
					spell_fizzled = 1;
					return 0;
				}
			}
			if (td){
				instance->number_of_targets = 0;
				pick_target(td, prompt);
			}
		}
		else if (choice == CHOICE_SUSPEND){
				int csvid = get_id(player, card);
				int cless = colorless;
				if( cless == -1 ){
					cless = 0;
				}
				if (charge_mana_multi(player, cless, black, blue, green, red, white)){
					if (csvid == CARD_ID_DETRITIVORE ||
						csvid == CARD_ID_BENALISH_COMMANDER ||
						csvid == CARD_ID_FUNGAL_BEHEMOTH ||
						csvid == CARD_ID_ROILING_HORROR
					   ){
						charge_mana(player, COLOR_COLORLESS, -1);
						if( x_value == 0 ){ //Cannot be suspended with X = 0
							spell_fizzled = 1;
							return 0;
						}
					}
					if (turns == -1){
						turns = x_value;
					}
					suspend_a_card(player, card, player, card, turns);
					land_can_be_played &= ~LCBP_SPELL_BEING_PLAYED;
				}
		}
		else {
			spell_fizzled = 1;
		}
	}

	return 0;
}

static int suspend_only(int player, int card, event_t event, int turns, int colorless, int black, int blue, int green, int red, int white)
{
	card_instance_t *instance = get_card_instance(player, card);

  if (event == EVENT_MODIFY_COST && !played_for_free(player, card))
	{
		if( has_mana_multi(player, colorless, black, blue, green, red, white) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
			infinite_casting_cost();
		}
	}

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_CAST_SPELL && affect_me(player, card) && !played_for_free(player, card) && !is_token(player, card))
	{
		if( instance->info_slot == 1 )
			charge_mana_multi(player, colorless, black, blue, green, red, white);
		if( spell_fizzled != 1 ){
			suspend_a_card(player, card, player, card, turns);
			land_can_be_played &= ~LCBP_SPELL_BEING_PLAYED;
		}
	}

  return 0;
}

void holy_nimbus(int player, int card, event_t event, int cost)
{
	if (land_can_be_played & LCBP_REGENERATION){
		card_instance_t* instance = get_card_instance(player, card);

		if( instance->info_slot == 1 ){
			return;
		}

		if( instance->targets[1].player != 66 && instance->kill_code == KILL_DESTROY && can_regenerate(player, card) ){
			int choice = 0;
			if( has_mana(1-player, COLOR_COLORLESS, cost) ){
				choice = do_dialog(1-player, player, card, -1, -1, " Let it regenerate\n Negate this ability until EOT", 1);
			}
			if (choice == 1 && charge_mana_while_resolving(player, card, EVENT_ACTIVATE, 1-player, COLOR_COLORLESS, cost)){
				instance->targets[1].player = 66;
			} else {
				cancel = 0;
				regenerate_target(player, card);
			}
		}

		instance->info_slot = 0;
	}

	if( event == EVENT_CLEANUP ){
		get_card_instance(player, card)->targets[1].player = 0;
	}
}

// Cards
int card_academy_ruins(int player, int card, event_t event){
  check_legend_rule(player, card, event);

  return recycling_land(player, card, event, TYPE_ARTIFACT, 1, 0, 1, 0, 0, 0);
}

int card_aether_web(int player, int card, event_t event){
  return generic_aura(player, card, event, player, 1, 1, KEYWORD_REACH, SP_KEYWORD_SHADOW_HOSER, 0, 0, 0);
}

int card_aetherflame_wall(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[16].card < 0 ){
		instance->targets[16].card = 0;
	}

	if( ! (instance->targets[16].card & SP_KEYWORD_SHADOW_HOSER) ){
		instance->targets[16].card |= SP_KEYWORD_SHADOW_HOSER;
	}

	return generic_shade(player, card, event, 0, MANACOST_R(1), 1, 0, 0, 0);
}

int card_amrou_seekers(int player, int card, event_t event){

	if(event == EVENT_BLOCK_LEGALITY ){
		if( (player == attacking_card_controller && card == attacking_card) ){
			if( ! is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) &&
				!( get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_WHITE))
				  ){
					event_result = 1;
			}
		}
	}

	return 0;
}

int card_ancestral_vision(int player, int card, event_t event)
{
  /* Ancestral Vision	""
   * Sorcery
   * Suspend 4-|U
   * Target player draws three cards. */

  suspend_only(player, card, event, 4, MANACOST_U(1));

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE );
  td.zone = TARGET_ZONE_PLAYERS;
  td.allow_cancel = 0;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_CAST)
	{
	  if (instance->info_slot && !played_for_free(player, card) && !is_token(player, card))
		return 1;

	  return can_target(&td);
	}

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && get_card_instance(player, card)->internal_card_id != -1	// Don't target if was just suspended
	  && can_target(&td))
	pick_target(&td, "TARGET_PLAYER");

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		draw_cards(instance->targets[0].player, 3);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_ali_from_cairo(int player, int card, event_t event);
static int angels_grace_legacy(int player, int card, event_t event){

	// Prevents damage from reducing life below 1
	card_ali_from_cairo(player, card, event);

	// Delays life loss from reducing life below 1 or poison counters from increasing above 9 until this leaves play; suppresses draws on empty deck
	cannot_lose_the_game(player, card, event, player);

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_angels_grace(int player, int card, event_t event){

	cannot_be_countered(player, card, event);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int legacy = create_legacy_effect(player, card, &angels_grace_legacy);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[2].card = get_id(player, card);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ashcoat_bear(int player, int card, event_t event){
	/*
	  Ashcoat Bear English |1|G
	  Creature - Bear 2/2
	  Flash (You may cast this spell any time you could cast an instant.)
	*/
  return flash(player, card, event);
}

int card_aspect_of_the_mangoose(int player, int card, event_t event){

	immortal_enchantment(player, card, event);

	return generic_aura(player, card, event, player, 0, 0, KEYWORD_SHROUD, 0, 0, 0, 0 );
}

int card_assassinate(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_barbed_shocker(int player, int card, event_t event){

	haste(player, card, event);

	return card_shocker(player, card, event);
}

int card_basal_sliver(int player, int card, event_t event){

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SLIVER, 0, 0, 0, 0, 0, -1, 0) ){
			declare_mana_available(player, COLOR_BLACK, 2);
		}
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SLIVER, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( ! sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_SLIVER, 0, 0, 0, 0, 0, -1, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		produce_mana(player, COLOR_BLACK, 2);
	}

	return slivercycling(player, card, event);
}

int card_benalish_cavalry(int player, int card, event_t event){
  flanking(player, card, event);
  return 0;
}

int card_bonesplitter_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 2, 0, 0, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_bogardan_hellkite(int player, int card, event_t event){
	/* Bogardan Hellkite	|6|R|R
	 * Creature - Dragon 5/5
	 * Flash
	 * Flying
	 * When ~ enters the battlefield, it deals 5 damage divided as you choose among any number of target creatures and/or players. */

	if( comes_into_play(player, card, event) ){
		target_and_divide_damage(player, card, NULL, NULL, 5);
	}
	return flash(player, card, event);
}

int card_brine_elemental(int player, int card, event_t event){
	if( event == EVENT_TURNED_FACE_UP ){
		target_player_skips_next_untap(player, card, 1-player);
	}
	return morph(player, card, event, 5, 0, 2, 0, 0, 0);
}

int card_call_to_the_netherworld(int player, int card, event_t event){

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST0);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t test;
	new_default_test_definition(&test, TYPE_CREATURE, get_sleighted_color_text(player, card, "Select a %s creature card.", COLOR_TEST_BLACK));

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &test);
}


int card_candles_of_leng(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int *deck = deck_ptr[player];
		show_deck( player, deck, 1, "Here's the first card of deck.", 0, 0x7375B0 );
		if( is_id_in_grave(player, cards_data[deck[0]].id) ){
			mill(player, 1);
		}
		else{
			draw_cards(player, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 4, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_careful_consideration(int player, int card, event_t event){

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
			draw_cards(instance->targets[0].player, 4);
			int to_discard = 3;
			if( current_turn == player && (current_phase == PHASE_MAIN1 || current_phase == PHASE_MAIN2) ){
				to_discard = 2;
			}
			new_multidiscard(instance->targets[0].player, to_discard, 0, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_cavalry_master(int player, int card, event_t event){

	flanking(player, card, event);

	if( event == EVENT_DECLARE_BLOCKERS && ! is_humiliated(player, card) && current_turn == player ){
		int p = current_turn;
		int count = active_cards_count[p]-1;
		while( count > -1 ){
				if( in_play(p, count) && is_attacking(p, count) && check_for_special_ability(p, count, SP_KEYWORD_FLANKING) ){
					int c2 = active_cards_count[1-p]-1;
					while( c2 > -1 ){
							if( in_play(1-p, c2) && is_what(1-p, c2, TYPE_CREATURE) ){
								card_instance_t *this = get_card_instance(1-p, c2);
								if( this->blocking == count ){
									if( ! check_for_special_ability(1-p, c2, SP_KEYWORD_FLANKING) ){
										pump_until_eot(player, card, 1-p, c2, -1, -1);
									}
								}
							}
							c2--;
					}
				}
				count--;
		}
	}
	return 0;

}

int card_celestial_crusader(int player, int card, event_t event){

	cannot_be_countered(player, card, event);

	boost_creature_by_color(player, card, event, COLOR_TEST_WHITE, 1, 1, 0, 0);

	return flash(player, card, event);
}

static int chameleon_blur_legacy(int player, int card, event_t event ){
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 && source->damage_target_card == -1 ){
			if( is_what(source->damage_source_player, source->damage_source_card, TYPE_CREATURE) ){
				source->info_slot = 0;
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_chameleon_blur(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &chameleon_blur_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_children_of_korlis(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0) ){
			if( player != AI ){
				return 1;
			}
			else{
				if( get_trap_condition(player, TRAP_LIFE_LOST) > 0 ){
					return 1;
				}
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, get_trap_condition(player, TRAP_LIFE_LOST));
	}

	return 0;
}

int card_chromatic_star(int player, int card, event_t event)
{
  if (event == EVENT_CAN_ACTIVATE)
	return !is_tapped(player, card) && !is_animated_and_sick(player, card) && has_mana(player, COLOR_ANY, 1) && can_produce_mana(player, card);

  if (event == EVENT_ACTIVATE)
	{
	  tap_card(player, card);
	  charge_mana(player, COLOR_COLORLESS, 1);
	  if (spell_fizzled != 1 && produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, 1))
		kill_card(player, card, KILL_SACRIFICE);
	  else
		untap_card_no_event(player, card);
	}

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	draw_a_card(player);

  return 0;
}

int card_chronatog_totem(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_XU(1, 1), 0, NULL, NULL) ){
			return 1;
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 1;
		if( ! paying_mana() ){
			int abilities[3] = {mana_producer(player, card, EVENT_CAN_ACTIVATE), 0, 0};
			set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_TYPE_CHANGE, MANACOST_XU(1, 1), 0, NULL, NULL) ){
				abilities[1] = 1;
			}
			remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
			if( is_what(player, card, TYPE_CREATURE) ){
				if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_ONCE_PER_TURN, MANACOST0, 0, NULL, NULL) ){
					abilities[2] = 1;
				}
			}


			choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
							"Get mana", abilities[0], 5,
							"Animate", abilities[1], (current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS) || (current_turn != player && current_phase < PHASE_DECLARE_BLOCKERS) ? 10 : 1,
							"Pump", abilities[2], 5);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
		}
		if( choice == 1 ){
			return mana_producer(player, card, event);
		}
		if( choice == 2 ){
			set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
			generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_XU(1, 1), 0, NULL, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = choice;
			}
			remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		}
		if( choice == 3 ){
			generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST0, 0, NULL, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 2 ){
			add_a_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_ATOG);
			int legacy = artifact_animation(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
											1, 1, 2, 0, 0);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[6].player = COLOR_TEST_BLUE;
		}
		if( instance->info_slot == 3){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 3, 3);
			skip_next_turn(instance->parent_controller, instance->parent_card, player);
		}
	}
	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_chronosavant(int player, int card, event_t event){
	/* Chronosavant	|5|W
	 * Creature - Giant 5/5
	 * |1|W: Return ~ from your graveyard to the battlefield tapped. You skip your next turn. */

	if( event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, MANACOST_XW(1, 1))){
		return GA_RETURN_TO_PLAY_MODIFIED | GA_RETURN_TO_PLAY_WITH_EFFECT;
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XW(1, 1));
		if( spell_fizzled != 1){
			return GAPAID_REMOVE;
		}
	}

	if( event == EVENT_RETURN_TO_PLAY_FROM_GRAVE_MODIFIED ){
		add_state(player, card, STATE_TAPPED);
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		skip_next_turn(player, card, player);
	}

	return 0;
}

static const char* target_must_be_permanent_or_suspend(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_what(player, card, TYPE_PERMANENT))
	return NULL;
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->internal_card_id == LEGACY_EFFECT_CUSTOM && instance->info_slot == (int)generic_suspend_legacy)
	return NULL;
  if (cards_data[instance->internal_card_id].id == CARD_ID_GREATER_GARGADON_SUSPENDED)
	return NULL;
  return EXE_STR(0x728F6C);//",type"
}
static const char* target_must_be_permanent_or_suspend_and_have_counters(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  const char* perm_or_susp = target_must_be_permanent_or_suspend(who_chooses, player, card, targeting_player, targeting_card);
  if (perm_or_susp)
	return perm_or_susp;
  return count_counters(player, card, -1) ? NULL : "must have counters";
}
int card_clockspinning(int player, int card, event_t event)
{
  /* Clockspinning	|U
   * Instant
   * Buyback |3
   * Choose a counter on target permanent or suspended card. Remove that counter from that permanent or card or put another of those counters on it. */

  if (!IS_CASTING(player, card, event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT | TYPE_EFFECT );
  td.preferred_controller = ANYBODY;
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  if (IS_AI(player) && event != EVENT_RESOLVE_SPELL)
	/* Prevents the AI from considering addition or removal of counters from permanents without them (thus speculating more on which type of counter to add or
	 * remove from permanents that *do* have them).  However, it also prevents it from using this spell to kill a Skulking Ghost or activate another whenever-
	 * this-becomes-targeted trigger.  I think this is an acceptable tradeoff. */
	td.extra = (int)target_must_be_permanent_or_suspend_and_have_counters;
  else
	td.extra = (int)target_must_be_permanent_or_suspend;

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card) && pick_target(&td, "TARGET_CARD"))
	instance->info_slot = buyback(player, card, 3, 0, 0, 0, 0, 0);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  counter_t typ = choose_existing_counter_type(player, player, card, instance->targets[0].player, instance->targets[0].card,
													   CECT_ADD_OR_REMOVE, -1, -1);
		  int remove_priority = 1;
		  if (typ & (1<<9))
			{
			  remove_priority = 9;
			  typ &= ~(1<<9);
			}

		  if (typ != COUNTER_invalid)
			{
			  char prompt[100];
			  scnprintf(prompt, 100, "%s: %d", counter_names[typ], count_counters(instance->targets[0].player, instance->targets[0].card, typ));
			  if (DIALOG(player, card, EVENT_CAST_SPELL,
						 DLG_FULLCARD(instance->targets[0].player, instance->targets[0].card), DLG_HEADER(prompt), DLG_WHO_CHOOSES(player),
						 DLG_NO_STORAGE, DLG_RANDOM, DLG_NO_CANCEL,
						 "Add counter", 1, 3,
						 "Remove counter", 1, remove_priority) == 1)
				add_counter(instance->targets[0].player, instance->targets[0].card, typ);
			  else
				remove_counter(instance->targets[0].player, instance->targets[0].card, typ);
			}
		  if (instance->info_slot == 1)
			{
			  bounce_permanent(player, card);
			  return 0;
			}
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_clockwork_hydra(int player, int card, event_t event)
{
  // ~ enters the battlefield with four +1/+1 counters on it.
  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 4);

  // Whenever ~ attacks or blocks, remove a +1/+1 counter from it. If you do, ~ deals 1 damage to target creature or player.
  if ((declare_attackers_trigger(player, card, event, 0, player, card)
	   || (blocking(player, card, event) && !is_humiliated(player, card)))
	  && count_1_1_counters(player, card) > 0)
	{
	  remove_1_1_counter(player, card);

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
		damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
	}

  // |T: Put a +1/+1 counter on ~.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  add_1_1_counter(instance->parent_controller, instance->parent_card);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(0), 0, NULL, NULL);
}

int card_cloudchaser_kestrel(int player, int card, event_t event){

	target_definition_t td_permanent;
	default_target_definition(player, card, &td_permanent, TYPE_PERMANENT);

	target_definition_t td_enchantment;
	default_target_definition(player, card, &td_enchantment, TYPE_ENCHANTMENT);
	td_enchantment.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if (comes_into_play(player, card, event)){
		if (can_target(&td_enchantment) && pick_target(&td_enchantment, "TARGET_ENCHANTMENT")){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		if(valid_target(&td_permanent)){
			change_color(player, card, instance->targets[0].player, instance->targets[0].card, COLOR_TEST_WHITE, CHANGE_COLOR_SET|CHANGE_COLOR_END_AT_EOT);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_W(1), 0, &td_permanent, "TARGET_PERMANENT");
}


int card_coal_stoker(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = not_played_from_hand(player, card);
	}
	if( comes_into_play(player, card, event) && instance->info_slot != 1 ){
		produce_mana(player, COLOR_RED, 3);
	}

	return 0;
}

int card_conflagrate(int player, int card, event_t event){
	/* Conflagrate	|X|X|R
	 * Sorcery
	 * ~ deals X damage divided as you choose among any number of target creatures and/or players.
	 * Flashback-|R|R, Discard X cards. */

	if( ! IS_GS_EVENT(player, card, event) && event != EVENT_GRAVEYARD_ABILITY && event != EVENT_PAY_FLASHBACK_COSTS ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( (played_for_free(player, card) || is_token(player, card)) && ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) && ! get_flashback(player, card) ){
			return 0;
		}
		int trgs = 0;
		int targs[10][2];
		while( trgs < 10 ){
				if( get_flashback(player, card) && trgs+1 > hand_count[player] ){
					break;
				}
				if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) && ! has_mana(player, COLOR_COLORLESS, ((trgs+1)*2)) ){
					break;
				}
				if( trgs >= instance->info_slot && check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
					break;
				}
				if( new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", trgs, 0) ){
					if( instance->targets[trgs].card != -1 ){
						add_state(instance->targets[trgs].player, instance->targets[trgs].card, STATE_TARGETTED);
					}
					targs[trgs][0] = instance->targets[trgs].player;
					targs[trgs][1] = instance->targets[trgs].card;
					trgs++;
				}
				else{
					break;
				}
		}
		if( get_flashback(player, card) ){
			int original_hand[2][hand_count[player]];
			int ohc = 0;
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_hand(player, i) ){
					original_hand[0][ohc] = get_original_internal_card_id(player, i);
					original_hand[1][ohc] = i;
					ohc++;
				}
			}
			int to_discard[hand_count[player]];
			int tdc = 0;
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard.");
			while( ohc && tdc < instance->number_of_targets ){
					int selected = select_card_from_zone(player, player, original_hand[0], ohc, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 ){
						to_discard[tdc] = original_hand[1][selected];
						tdc++;
						for(i=selected; i<ohc; i++){
							original_hand[0][i] = original_hand[0][i+1];
							original_hand[1][i] = original_hand[1][i+1];
						}
						ohc--;
					}
					else{
						break;
					}
			}
			if( tdc == instance->number_of_targets ){
				for(i=0; i<tdc; i++){
					discard_card(player, to_discard[i]);
				}
				instance->info_slot = tdc;
			}
		}
		else{
			if( ! is_token(player, card) ){
				charge_mana(player, COLOR_COLORLESS, instance->number_of_targets * 2);
				if( spell_fizzled != 1 ){
					instance->info_slot = instance->number_of_targets;
				}
			}
		}
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( instance->targets[i].card != -1 ){
				remove_state(instance->targets[i].player, instance->targets[i].card, STATE_TARGETTED);
			}
			instance->targets[i].player = targs[i][0];
			instance->targets[i].card = targs[i][1];
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		divide_damage(player, card, &td);
		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( can_target(&td) && hand_count[player] > 0 ){
			return do_flashback(player, card, event, MANACOST_R(2));
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		return do_flashback(player, card, event, MANACOST_R(2));
	}

	return 0;
}


int card_coral_trickster(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_TURNED_FACE_UP ){
		if( can_target(&td) && pick_target(&td, "TARGET_PERMANENT") ){
			instance->number_of_targets = 1;
			if( is_tapped(instance->targets[0].player, instance->targets[0].card) ){
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return morph(player, card, event, 0, 0, 1, 0, 0, 0);
}

int card_crookclaw_transmuter(int player, int card, event_t event)
{
  // When ~ enters the battlefield, switch target creature's power and toughness until end of turn.
  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = ANYBODY;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		switch_power_and_toughness_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
	}

  // Flash
  return flash(player, card, event);
}

int card_curse_of_the_cabal(int player, int card, event_t event){

	/* Curse of the Cabal	|9|B
	 * Sorcery
	 * Target player sacrifices half the permanents he or she controls, rounded down.
	 * Suspend 2-|2|B|B
	 * At the beginning of each player's upkeep, if ~ is suspended, that player may sacrifice a permanent. If he or she does, put two time counters on ~. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			int amount = count_subtype(instance->targets[0].player, TYPE_PERMANENT, -1);
			amount/=2;
			impose_sacrifice(player, card, instance->targets[0].player, amount, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return suspend(player, card, event, 2, 2, 2, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_dark_withering(int player, int card, event_t event){

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_B(1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						get_sleighted_color_text(player, card, "Select target target non%s creature.", COLOR_TEST_BLACK), 1, NULL);
}

int card_deathspore_thallid(int player, int card, event_t event){

	/* Deathspore Thallid	|1|B
	 * Creature - Zombie Fungus 1/1
	 * At the beginning of your upkeep, put a spore counter on ~.
	 * Remove three spore counters from ~: Put a 1/1 |Sgreen Saproling creature token onto the battlefield.
	 * Sacrifice a Saproling: Target creature gets -1/-1 until end of turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		if( can_target(&td) && can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0)){
			return 1;
		}
		return can_make_saproling_from_fungus(player, card);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int choice = 0;
			if( can_make_saproling_from_fungus(player, card) ){
				if( can_target(&td) && can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Generate a Saproling\n Weaken a creature\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				saproling_from_fungus(player, card);
				instance->info_slot = 66+choice;
			}
			else if( choice == 1 ){
					if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0) ){
						if( pick_target(&td, "TARGET_CREATURE") ){
							instance->info_slot = 66+choice;
							instance->number_of_targets = 1;
						}
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

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	add_spore_counters(player, card, event);
	return 0;
}

int card_deep_sea_kraken(int player, int card, event_t event){
	/* Deep-Sea Kraken	|7|U|U|U
	 * Creature - Kraken 6/6
	 * ~ is unblockable.
	 * Suspend 9-|2|U
	 * Whenever an opponent casts a spell, if ~ is suspended, remove a time counter from it. */
	unblockable(player, card, event);
	return suspend(player, card, event, 9, 2, 0, 1, 0, 0, 0, NULL, NULL);
}

int card_demonic_collusion(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			if( hand_count[player] > 1 && ! is_token(player, card) ){
				choice = do_dialog(player, player, card, -1, -1, " Play normally\n Buyback", 1);
			}
			if( choice == 1 ){
				multidiscard(player, 2, 0);
			}
			instance->info_slot = choice;

	}

	else if(event == EVENT_RESOLVE_SPELL ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}

	return 0;
}

int card_draining_whelk(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		if( card_counterspell(player, card, EVENT_CAN_CAST) ){
			instance->info_slot = 1;
			return 0x63;
		}
		else{
			instance->info_slot = 0;
			return 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && current_turn == player){
		ai_modifier -= 10;
	}

	if( comes_into_play(player, card, event) ){
		if( instance->info_slot == 1 ){
			set_flags_when_spell_is_countered(player, card, card_on_stack_controller, card_on_stack);
			add_1_1_counters(player, card, get_cmc(card_on_stack_controller, card_on_stack));
			kill_card(card_on_stack_controller, card_on_stack, KILL_DESTROY);
		}
	}
	return 0;
}

int card_dralnu_lich_lord(int player, int card, event_t event ){

	/* Dralnu, Lich Lord	|3|U|B
	 * Legendary Creature - Zombie Wizard 3/3
	 * If damage would be dealt to ~, sacrifice that many permanents instead.
	 * |T: Target instant or sorcery card in your graveyard gains flashback until end of turn. The flashback cost is equal to its mana cost. */

	check_legend_rule(player, card, event);

	if (IS_ACTIVATING(event)){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, "Select target instant or sorcery card.");

		if( event == EVENT_CAN_ACTIVATE ){
			if( generic_activated_ability(player, card, event, GAA_UNTAPPED, 0, 0, 0, 0, 0, 0, 0, NULL, NULL) ){
				if( new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(2) ){
					return 1;
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
				tap_card(player, card);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int selected = validate_target_from_grave(player, card, player, 0);
			create_spell_has_flashback_legacy(player, card, selected, 0);
		}
	}

	if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card)){
		get_card_instance(player, card)->destroys_if_blocked |= DIFB_ASK_CARD;
	}

	if (event == EVENT_CHECK_DESTROY_IF_BLOCKED && affect_me(player, card) && !is_humiliated(player, card)
		&& get_power(attacking_card_controller, attacking_card) >= 1){
		event_result |= 2;
	}


	if( !is_humiliated(player, card) && event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == player && damage->damage_target_card == card){
			if( damage->info_slot > 0 ){
				int amount = damage->info_slot;
				damage->info_slot = 0;
				impose_sacrifice(player, card, player, amount, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}

	return 0;
}

int card_dread_return(int player, int card, event_t event){
	/* Dread Return	|2|B|B
	 * Sorcery
	 * Return target creature card from your graveyard to the battlefield.
	 * Flashback-Sacrifice three creatures. */

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_GRAVEYARD_ABILITY){
		// can only play as sorcery
		if( can_sorcery_be_played(player, event) && count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(2) ){
			if( can_pay_flashback(player, instance->internal_card_id, event, 0, 0, 0, 0, 0, 0) ){
				return can_sacrifice_as_cost(player, 3, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ? GA_PLAYABLE_FROM_GRAVE : 0;
			}
		}
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
			if( pay_flashback(player, instance->internal_card_id, event, 0, 0, 0, 0, 0, 0) ){
			// sacrifice 3 creatures
			int i = 0;
			int sac = 0;
			for(i=0;i<3;i++){
				int mode = 0;
				if( i > 0 ){
					mode = 1;
				}
				if( sacrifice(player, card, player, mode, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					sac++;
				}
				else{
					break;
				}
			}
			if( sac == 0 ){
				spell_fizzled = 1;
			}
			else{
				return GAPAID_EXILE;
			}
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card");

	if(event == EVENT_RESOLVE_SPELL){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}

		if( get_flashback(player, card) ){
			kill_card(player, card, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &this_test);
}

int card_dream_stalker(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && can_target(&td1) ){
		if( pick_target(&td1, "TARGET_PERMANENT") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_drifter_il_dal(int player, int card, event_t event){
	shadow(player, card, event);
	basic_upkeep(player, card, event, 0, 0, 1, 0, 0, 0);
	return 0;
}

int card_durkwood_baloth(int player, int card, event_t event){
	/* Durkwood Baloth	|4|G|G
	 * Creature - Beast 5/5
	 * Suspend 5-|G */
	return suspend(player, card, event, 5, 0, 0, 0, 1, 0, 0, NULL, NULL);
}

int card_duskrider_peregrine(int player, int card, event_t event){
	/* Duskrider Peregrine	|5|W
	 * Creature - Bird 3/3
	 * Flying, protection from |Sblack
	 * Suspend 3-|1|W */
	return suspend(player, card, event, 3, MANACOST_XW(1,1), NULL, NULL);
}

int card_empty_the_warrens(int player, int card, event_t event){

	/* Empty the Warrens	|3|R
	 * Sorcery
	 * Put two 1/1 |Sred Goblin creature tokens onto the battlefield.
	 * Storm */

	if(event == EVENT_CAST_SPELL && affect_me(player, card)){
		if( ! check_special_flags(player, card, SF_NOT_CAST) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_GOBLIN, &token);
			token.pow = 1;
			token.tou = 1;
			token.color_forced = COLOR_TEST_RED;
			token.qty = 2*get_storm_count();
			generate_token(&token);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GOBLIN, &token);
		token.pow = 1;
		token.tou = 1;
		token.color_forced = COLOR_TEST_RED;
		token.qty = 2;
		generate_token(&token);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_endrek_sahr_master_breeder(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( specific_spell_played(player, card, event, player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t* instance = get_card_instance(player, card);
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_THRULL, &token);
		token.pow = token.tou = 1;
		token.qty = get_cmc(instance->targets[1].player, instance->targets[1].card);
		generate_token(&token);
	}

	if( event == EVENT_STATIC_EFFECTS && count_subtype(player, TYPE_PERMANENT, SUBTYPE_THRULL) > 6 ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_errant_doomsayer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.toughness_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_errant_ephemeron(int player, int card, event_t event){
	/* Errant Ephemeron	|6|U
	 * Creature - Illusion 4/4
	 * Flying
	 * Suspend 4-|1|U */
	return suspend(player, card, event, 4, 1, 0, 1, 0, 0, 0, NULL, NULL);
}

int card_eternity_snare(int player, int card, event_t event)
{
  /* Eternity Snare |5|U
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield, draw a card.
   * Enchanted creature doesn't untap during its controller's untap step. */

  if (comes_into_play(player, card, event))
	draw_cards(player, 1);

  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player, c = instance->damage_target_card;
  if (p >= 0 && c >= 0)
	does_not_untap(p, c, event);

  return disabling_aura(player, card, event);
}

int card_evangelize(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1 - player;
	td.preferred_controller = 1 - player;
	td.who_chooses = 1 - player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->info_slot = buyback(player, card, 2, 0, 0, 0, 0, 2);
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int resolved = 0;
			if( can_target(&td) && select_target(player, card, &td, "Select target creature to donate", &(instance->targets[0])) ){
				gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
				resolved = 1;
				if( instance->info_slot == 1 ){
					bounce_permanent(player, card);
				}
			}
			if( resolved == 0 || instance->info_slot == 0 ){
				kill_card(player, card, KILL_DESTROY);
			}
	}

	return 0;
}

int card_fathom_seer(int player, int card, event_t event)
{
  // Morph - Return two |H1Islands you control to their owner's hand.
  if (event == EVENT_CAN_UNMORPH)
	return count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND) >= 2;

  if (event == EVENT_UNMORPH)
	{
	  target_definition_t td;
	  base_target_definition(player, card, &td, TYPE_LAND);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.required_subtype = SUBTYPE_ISLAND;

	  if (pick_up_to_n_targets_noload(&td, "Select an Island you control.", 2) == 2)
		{
		  card_instance_t* instance = get_card_instance(player, card);

		  bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		  if (in_play(instance->targets[1].player, instance->targets[1].card))	// Some stupid Manalink non-stack-using trigger might've removed it
			bounce_permanent(instance->targets[1].player, instance->targets[1].card);

		  instance->number_of_targets = 0;
		}
	  else
		cancel = 1;

	  return 0;
	}

  // When ~ is turned face up, draw two cards.
  if (event == EVENT_TURNED_FACE_UP)
	draw_cards(player, 2);

  return morph(player, card, event, MANACOST_X(0));
}

int card_fertile_ground(int player, int card, event_t event)
{
  /* Fertile Ground	|1|G
   * Enchantment - Aura
   * Enchant land
   * Whenever enchanted land is tapped for mana, its controller adds one mana of any color to his or her mana pool. */
  return wild_growth_aura_all_one_color(player, card, event, -1, COLOR_TEST_ANY_COLORED, 1);
}

int firewake_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.preferred_controller = player;


		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( would_validate_arbitrary_target(&td2, instance->targets[2].player, instance->targets[2].card) ){
				pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[2].player, instance->targets[2].card, 2, 2);
			}
		}

		return shared_sliver_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_X(1), 0,
												&td2, "Select target Sliver to pump.");
	}

	return 0;
}

int card_firewake_sliver(int player, int card, event_t event){
	/*
	  Firewake Sliver |1|R|G
	  Creature - Sliver 1/1
	  All Sliver creatures have haste.
	  All Slivers have "{1}, Sacrifice this permanent: Target Sliver creature gets +2/+2 until end of turn."
	*/

	// All Sliver creatures have haste.
	boost_subtype(player, card, event, SUBTYPE_SLIVER, 0,0, 0,SP_KEYWORD_HASTE, BCT_INCLUDE_SELF);

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &firewake_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &firewake_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int card_flagstones_of_trokair(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( graveyard_from_play(player, card, event) ){
		global_tutor(player, player, 1, TUTOR_PLAY_TAPPED, 0, 1, TYPE_LAND, 0, SUBTYPE_PLAINS, 0, 0, 0, 0, 0, -1, 0);
	}

	return mana_producer(player, card, event);
}

int card_fledgling_mawcor(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_TURNED_FACE_UP ){
		instance->targets[1].player = 66;
	}

	if( instance->targets[1].player == 66 ){
		return card_prodigal_sorcerer(player, card, event);
	}

	return morph(player, card, event, 0, 0, 2, 0, 0, 0);
}

int card_flickering_spirit(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		blink_effect(instance->parent_controller, instance->parent_card, 0);
	}

	return generic_activated_ability(player, card, event, 0, 3, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_flowstone_channeler(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, -1, 0, SP_KEYWORD_HASTE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD+GAA_CAN_TARGET, 1, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_foriysian_interceptor(int player, int card, event_t event)
{
  creature_can_block_additional(player, card, event, 1);
  return flash(player, card, event);
}

int card_foriysian_totem(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 4, 0, 0, 0, 1, 0, 0, 0, 0) ){
			return 1;
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		int mode = (1<<2);
		if( mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			mode |= (1<<0);
		}
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_TYPE_CHANGE, 4, 0, 0, 0, 1, 0, 0, 0, 0) ){
			mode |=(1<<1);
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		int choice = 0;
		if( ! paying_mana() && ((mode & (1<<1)) || (mode & (1<<2))) ){
			char buffer[500];
			int pos = 0;
			int ai_choice = 1;
			if( mode & (1<<0) ){
				pos += scnprintf(buffer + pos, 500-pos, " Get mana\n", buffer);
			}
			if( mode & (1<<1) ){
				pos += scnprintf(buffer + pos, 500-pos, " Animate\n", buffer);
			}
			pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
			choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			while( !( (1<<choice) & mode) ){
					choice++;
			}
		}
		if( choice == 0 ){
			instance->info_slot = 66+choice;
			return mana_producer(player, card, event);
		}
		else if( choice == 1){
				set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
				charge_mana_multi(player, 4, 0, 0, 0, 1, 0);
				if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 4, 0, 0, 0, 1, 0, 0, 0, 0) ){
					instance->info_slot = 66+choice;
				}
				remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		}
		else if( choice == 2 ){
				instance->info_slot = 66+choice;
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( instance->targets[1].player == 66 ){
		creature_can_block_additional(player, card, event, 1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance(player, instance->parent_card);
		if( instance->info_slot == 66 ){
			return mana_producer(player, card, event);
		}
		if( instance->info_slot == 67 ){
			add_a_subtype(player, instance->parent_card, SUBTYPE_GIANT);
			int legacy = artifact_animation(player, instance->parent_card, player, instance->parent_card, 1, 4, 4, 0, 0);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[6].player = COLOR_TEST_RED;
			parent->targets[1].player = 66;
		}
	}
	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_fortify(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_SPELL ){
		int choice = do_dialog(player, player, card, -1, -1, " Pump power\n Pump toughness", current_turn == player ? 0 : 1);
		pump_subtype_until_eot(player, card, player, -1, 2*(choice == 0), 2*(choice == 1), 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_ali_from_cairo(int player, int card, event_t event);
int card_fortune_thief(int player, int card, event_t event){
	card_ali_from_cairo(player, card, event);
	return morph(player, card, event, 0, 0, 0, 0, 2, 0);
}

static int effect_fungus_sliver(int player, int card, event_t event){
	if (event == EVENT_AFTER_DAMAGE){
		card_instance_t* instance = get_card_instance(player, card);
		if (in_play(instance->damage_target_player, instance->damage_target_card)){
			add_1_1_counter(instance->damage_target_player, instance->damage_target_card);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	// just in case
	if (event == EVENT_CLEANUP && affect_me(player, card)){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_fungus_sliver(int player, int card, event_t event){

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card &&
			damage->damage_target_card != -1 && is_what(damage->damage_target_player, damage->damage_target_card, TYPE_CREATURE) &&
			has_subtype(damage->damage_target_player, damage->damage_target_card, SUBTYPE_SLIVER) &&
			(damage->info_slot > 0 ||
			 get_card_instance(damage->damage_source_player, damage->damage_source_card)->targets[16].player > 0)	// wither/infect damage
		  ){
			// Fungusaur ruling: 10/4/2004 If more than one creature damages it at one time, it only gets one counter.
			// Fungus Sliver has the same wording, so apply that here as well.

			// Search for existing legacies originating from this Fungus Sliver attached to target.
			int p, c;
			for (p = 0; p <= 1; ++p){
				for (c = 0; c < active_cards_count[player]; ++c){
					card_instance_t* instance = get_card_instance(p, c);
					if (instance->damage_source_card == card && instance->damage_source_player == player
						&& instance->damage_target_card == damage->damage_target_card && instance->damage_target_player == damage->damage_target_player
						&& instance->internal_card_id == LEGACY_EFFECT_CUSTOM && instance->info_slot == (int)effect_fungus_sliver){
						return 0;
					}
				}
			}
			create_targetted_legacy_effect(player, card, effect_fungus_sliver, damage->damage_target_player, damage->damage_target_card);
		}
	}

	return slivercycling(player, card, event);
}

int card_fury_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_DOUBLE_STRIKE, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_gauntlet_of_power(int player, int card, event_t event){
	/* Gauntlet of Power	|5
	 * Artifact
	 * As ~ enters the battlefield, choose a color.
	 * Creatures of the chosen color get +1/+1.
	 * Whenever a basic land is tapped for mana of the chosen color, its controller adds one mana of that color to his or her mana pool. */

	if (event != EVENT_RESOLVE_SPELL && event != EVENT_TAP_CARD && event != EVENT_COUNT_MANA && event != EVENT_POWER && event != EVENT_TOUGHNESS){
		return 0;
	}

	card_instance_t *instance= get_card_instance(player, card);

	if (event == EVENT_RESOLVE_SPELL){
		instance->info_slot = choose_a_color_and_show_legacy(player, card, player, -1);
		return 0;
	}

	int test_color = 1 << instance->info_slot;
	int subtype;
	if( instance->info_slot == COLOR_BLACK ){
		subtype = SUBTYPE_SWAMP;
	}
	else if( instance->info_slot == COLOR_BLUE ){
		subtype = SUBTYPE_ISLAND;
	}
	else if( instance->info_slot == COLOR_GREEN ){
		subtype = SUBTYPE_FOREST;
	}
	else if( instance->info_slot == COLOR_RED ){
		subtype = SUBTYPE_MOUNTAIN;
	}
	else if( instance->info_slot == COLOR_WHITE ){
		subtype = SUBTYPE_PLAINS;
	}
	else {
		/* No color chosen, because the enters-the-battlefield trigger was countered (e.g. by Stifle)
		 * or was prevented (e.g. by Torpor Orb, with a March of the Machines in play) */
		return 0;
	}

	// do the creature part
	if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
		int affected = get_color(affected_card_controller, affected_card);
		if( affected & test_color ){
			event_result++;
		}
		return 0;
	}

	// do the mana part
	if (has_subtype(affected_card_controller, affected_card, subtype) && is_basic_land(affected_card_controller, affected_card)){
		if (event == EVENT_TAP_CARD){
			if (tapped_for_mana_color == instance->info_slot
				|| (tapped_for_mana_color == 0x100 && tapped_for_mana[instance->info_slot] > 0)){
				produce_mana(affected_card_controller, instance->info_slot, 1);
			}
		}
		else if (event == EVENT_COUNT_MANA){
			if (!is_tapped(affected_card_controller, affected_card) && !is_animated_and_sick(affected_card_controller, affected_card)
				&& can_produce_mana(affected_card_controller, affected_card)){
				declare_mana_available(affected_card_controller, instance->info_slot, 1);
			}
		}
	}

	return 0;
}

int card_gemhide_sliver(int player, int card, event_t event){

	int result = permanents_you_control_can_tap_for_mana_all_one_color(player, card, event, TYPE_CREATURE, SUBTYPE_SLIVER, COLOR_TEST_ANY_COLORED, 1);
	if (event == EVENT_CAN_ACTIVATE){
		return result;
	}

	return slivercycling(player, card, event);
}

int card_gemstone_caverns(int player, int card, event_t event){

  /* Gemstone Caverns	""
   * Legendary Land
   * If ~ is in your opening hand and you're not playing first, you may begin the game with ~ on the battlefield with a luck counter on it. If you do, exile a
   * card from your hand.
   * |T: Add |1 to your mana pool. If ~ has a luck counter on it, instead add one mana of any color to your mana pool. */

  check_legend_rule(player, card, event);

  if (event == EVENT_CHANGE_TYPE)
	get_card_instance(player, card)->info_slot = count_counters(player, card, COUNTER_LUCK) > 0 ? COLOR_TEST_ANY_COLORED : COLOR_TEST_COLORLESS;

  return mana_producer(player, card, event);
}

int card_goblin_skycutter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			negate_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, KEYWORD_FLYING);
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_gorgon_recluse(int player, int card, event_t event)
{
	card_deathgazer(player, card, event);
	return madness(player, card, event, MANACOST_B(2));
}

int card_grapeshot(int player, int card, event_t event){

	/* Grapeshot	|1|R
	 * Sorcery
	 * ~ deals 1 damage to target creature or player.
	 * Storm */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
		if( spell_fizzled != 1 && ! check_special_flags(player, card, SF_NOT_CAST) ){
			int i;
			for(i=0;i<get_storm_count();i++){
				if( new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", 1, 0) ){
					damage_creature(instance->targets[1].player, instance->targets[1].card, 1, player, card);
				}
			}
			instance->number_of_targets = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_greater_gargadon_suspended(int player, int card, event_t event){

	/* Greater Gargadon	|9|R
	 * Creature - Beast 9/7
	 * Suspend 10-|R
	 * Sacrifice an artifact, creature, or land: Remove a time counter from ~. Activate this ability only if ~ is suspended. */

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		remove_counter(player, card, COUNTER_TIME);
	}

	if (IS_ACTIVATING(event)){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND, "");

		if (event == EVENT_CAN_ACTIVATE){
			return new_can_sacrifice_as_cost(player, card, &test);
		}

		if( event == EVENT_ACTIVATE ){
			new_sacrifice(player, card, player, 0, &test);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			remove_counter(instance->parent_controller, instance->parent_card, COUNTER_TIME);
		}
	}

	if( event == EVENT_STATIC_EFFECTS && count_counters(player, card, COUNTER_TIME) <= 0){
		int id = CARD_ID_GREATER_GARGADON;
		if( check_rfg(player, id) ){
			int crd = play_card_in_exile_for_free(player, player, id);
			remove_summoning_sickness(player, crd);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_greater_gargadon(int player, int card, event_t event){

	/* Greater Gargadon	|9|R
	 * Creature - Beast 9/7
	 * Suspend 10-|R
	 * Sacrifice an artifact, creature, or land: Remove a time counter from ~. Activate this ability only if ~ is suspended. */

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( has_mana(player, COLOR_RED, 1) ){
			card_ptr_t* c = cards_ptr[ get_id(player, card) ];
			COST_COLORLESS-=c->req_colorless;
			COST_RED-=c->req_red;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			int choice = 1;

			if( has_mana_multi(player, 9, 0, 0, 0, 1, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Play normally\n Suspend\n Do nothing", 0);
			}

			if( choice == 0 ){
				charge_mana_multi(player, 9, 0, 0, 0, 1, 0);
			}
			else if(choice == 1 ){
					charge_mana(player, COLOR_RED, 1);
					if( spell_fizzled != 1 ){
						int card_added = add_card_to_hand(player, get_internal_card_id_from_csv_id(CARD_ID_GREATER_GARGADON_SUSPENDED));
						add_counters(player, card_added, COUNTER_TIME, 10);
						put_into_play(player, card_added);
						add_card_to_rfg(player, instance->internal_card_id);
						obliterate_card(player, card);
						land_can_be_played &= ~LCBP_SPELL_BEING_PLAYED;
					}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}

int card_greenseeker(int player, int card, event_t event){

	/* Greenseeker	|G
	 * Creature - Elf Spellshaper 1/1
	 * |G, |T, Discard a card: Search your library for a basic land card, reveal it, and put it into your hand. Then shuffle your library. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		tutor_basic_lands(player, TUTOR_HAND, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD, 0, 0, 0, 1, 0, 0, 0, 0, 0);
}

// Griffin --> Vanilla

int card_griffin_guide(int player, int card, event_t event)
{
  /* Griffin Guide	|2|W
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +2/+2 and has flying.
   * When enchanted creature dies, put a 2/2 |Swhite Griffin creature token with flying onto the battlefield. */

  if (attached_creature_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	generate_token_by_id(player, card, CARD_ID_GRIFFIN);

  return generic_aura(player, card, event, player, 2, 2, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_ground_rift(int player, int card, event_t event){

	/* Ground Rift	|R
	 * Sorcery
	 * Target creature without flying can't block this turn.
	 * Storm */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities |= KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature without flying.", 1, NULL);
		if( spell_fizzled != 1 && ! check_special_flags(player, card, SF_NOT_CAST) ){
			int i;
			for(i=0;i<get_storm_count();i++){
				if( new_pick_target(&td, "Select target creature without flying.", 1, GS_LITERAL_PROMPT) ){
					pump_ability_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
				}
			}
			instance->number_of_targets = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_harmonic_sliver(int player, int card, event_t event){
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == player
	  ){
		int trig = 0;
		if( has_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_SLIVER)  ){
			trig = 1;
		}
		if( trig == 1 && check_battlefield_for_id(2, CARD_ID_TORPOR_ORB) ){
			trig = 0;
		}
		if( trig == 1 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int p = trigger_cause_controller;
					int c = trigger_cause;

					target_definition_t td;
					default_target_definition(p, c, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT );
					td.allow_cancel = 0;

					if (can_target(&td) && pick_target(&td, "DISENCHANT")){
						card_instance_t *instance = get_card_instance(p, c);
						kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
					}
				}
		}
	}
	return slivercycling(player, card, event);
}

int card_haunting_hymn(int player, int card, event_t event){

	/* Haunting Hymn	|4|B|B
	 * Instant
	 * Target player discards two cards. If you cast this spell during your main phase, that player discards four cards instead. */

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
	else if(event == EVENT_CAST_SPELL && affect_me(player, card)){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int to_discard = 2;
				if( current_turn == player && (current_phase == PHASE_MAIN1 || current_phase == PHASE_MAIN2) ){
					to_discard = 4;
				}
				new_multidiscard(instance->targets[0].player, to_discard, 0, player);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_herd_gnarr(int player, int card, event_t event){
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.not_me = 1;
	if( new_specific_cip(player, card, event, player, 2, &this_test) ){
		pump_until_eot(player, card, player, card, 2, 2);
	}
	return 0;
}

int card_hivestone(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_RESOLVE_SPELL ){
		int i = player;
		int count = 0;
		while( count < active_cards_count[i] ){
				if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
					add_a_subtype(i, count, SUBTYPE_SLIVER);
				}
				count++;
		}
	}
	if( specific_cip(player, card, event, player, 2, TYPE_CREATURE, 0, SUBTYPE_SLIVER, 1, 0, 0, 0, 0, -1, 0) ){
		add_a_subtype(instance->targets[1].player, instance->targets[1].card, SUBTYPE_SLIVER);
	}
	if( leaves_play(player, card, event) ){
		int i = player;
		int count = 0;
		while( count < active_cards_count[i] ){
				if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && get_added_subtype(i, count) == SUBTYPE_SLIVER ){
					reset_subtypes(i, count, 2);
				}
				count++;
		}
	}
	return 0;
}

int card_hypergenesis(int player, int card, event_t event)
{
  /* Hypergenesis	""
   * Sorcery
   * Suspend 3-|1|G|G
   * Starting with you, each player may put an artifact, creature, enchantment, or land card from his or her hand onto the battlefield. Repeat this process
   * until no one puts a card onto the battlefield. */

  if (event == EVENT_RESOLVE_SPELL)
	card_eureka(player, card, event);

  return suspend_only(player, card, event, 3, MANACOST_XG(1,2));
}

void damage_all_my_blockers(int player, int card, int dmg){
	int i;
	for(i=active_cards_count[1-player]-1; i>-1; i--){
		if( in_play(1-player, i) && is_what(1-player, i, TYPE_CREATURE) ){
			card_instance_t *this = get_card_instance(1-player, i);
			if( this->blocking == card ){
				damage_creature(1-player, i, dmg, player, card);
			}
		}
	}
}


int card_ib_halfheart_goblin_tactician(int player, int card, event_t event){
	check_legend_rule(player, card, event);

	if( current_turn == player && event == EVENT_DECLARE_BLOCKERS ){
		int count = active_cards_count[1-player]-1;
		while( count > -1  ){
				if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
					card_instance_t *this = get_card_instance(1-player, count);
					if( this->blocking < 255 && has_subtype(player, this->blocking, SUBTYPE_GOBLIN) && this->blocking != card ){
						damage_all_my_blockers(player, this->blocking, 4);
						kill_card(player, this->blocking, KILL_SACRIFICE);
					}
				}
				count--;
		}
	}
	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		return can_sacrifice_as_cost(player, 2, TYPE_LAND, 0, SUBTYPE_MOUNTAIN, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( sacrifice(player, card, player, 0, TYPE_LAND, 0, SUBTYPE_MOUNTAIN, 0, 0, 0, 0, 0, -1, 0) ){
				sacrifice(player, card, player, 1, TYPE_LAND, 0, SUBTYPE_MOUNTAIN, 0, 0, 0, 0, 0, -1, 0);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 2);
	}

	return 0;
}

int card_icatian_crier(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_tokens_by_id(player, card, CARD_ID_CITIZEN, 2);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD, 1, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_ith_high_arcanist(int player, int card, event_t event){

	/* Ith, High Arcanist	|5|W|U
	 * Legendary Creature - Human Wizard 3/5
	 * Vigilance
	 * |T: Untap target attacking creature. Prevent all combat damage that would be dealt to and dealt by that creature this turn.
	 * Suspend 4-|W|U */

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	suspend(player, card, event, 4, 0, 0, 1, 0, 0, 1, NULL, NULL);

	return card_maze_of_ith(player, card, event);
}

int card_ivory_giant(int player, int card, event_t event){

	/* Ivory Giant	|5|W|W
	 * Creature - Giant 3/4
	 * When ~ enters the battlefield, tap all non|Swhite creatures.
	 * Suspend 5-|W */

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		this_test.color_flag = 1;
		new_manipulate_all(player, card, 2, &this_test, ACT_TAP);
	}
	return suspend(player, card, event, 5, 0, 0, 0, 0, 0, 1, NULL, NULL);
}

static int ixidron_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[0].player != -1 && instance->targets[12].card != -1){
		if( event == EVENT_CHANGE_TYPE && affect_me(instance->targets[0].player, instance->targets[0].card) ){
			event_result = instance->targets[12].card;
		}
	}
	return 0;
}

int card_ixidron(int player, int card, event_t event){
	/* Ixidron	|3|U|U
	 * Creature - Illusion 100/100
	 * As ~ enters the battlefield, turn all other nontoken creatures face down.
	 * ~'s power and toughness are each equal to the number of face-down creatures on the battlefield. */

	if( event == EVENT_RESOLVE_SPELL ){
		int fdc_iid = get_internal_card_id_from_csv_id(CARD_ID_FACE_DOWN_CREATURE);
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && !(i==player && count == card) && ! is_token(player, card) ){
						if( get_id(i, count) != CARD_ID_FACE_DOWN_CREATURE ){
							int legacy = create_targetted_legacy_effect(player, card, &ixidron_legacy, i, count);
							card_instance_t *leg = get_card_instance(player, legacy);
							leg->token_status |= STATUS_INVISIBLE_FX;
							leg->targets[12].card = fdc_iid;
							turn_face_down(i, count);
						}
					}
					count--;
			}
		}
	}

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card)){
		event_result += count_cards_by_id(ANYBODY, CARD_ID_FACE_DOWN_CREATURE);
	}
	return 0;
}

int card_jaya_ballard_task_mage(int player, int card, event_t event){

	/* Jaya Ballard, Task Mage	|1|R|R
	 * Legendary Creature - Human Spellshaper 2/2
	 * |R, |T, Discard a card: Destroy target |Sblue permanent.
	 * |1|R, |T, Discard a card: ~ deals 3 damage to target creature or player. A creature dealt damage this way can't be regenerated this turn.
	 * |5|R|R, |T, Discard a card: ~ deals 6 damage to each creature and each player. */

	check_legend_rule(player, card, event);

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_color = COLOR_TEST_BLUE;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE &&
		!(can_use_activated_abilities(player, card)  && ! is_sick(player, card) && ! is_tapped(player, card) &&
		  hand_count[player] > 0)
	  ){
		return 0;
	}

	enum{
		CHOICE_KILL_BLUE_PERMANENT = 1,
		CHOICE_BOLT = 2,
		CHOICE_INFERNO = 3
	};
	unsigned int can_kill_blue_permanent = can_target(&td);
	unsigned int can_bolt = can_target(&td1);
	unsigned int can_inferno = 1;
	unsigned int inferno_value = life[player] > 6 && life[1-player] < 7 ? 10 : 0;
	int choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Destroy blue permanent", can_kill_blue_permanent, 5, DLG_MANA6(0, 0, 0, 0, 1, 0),
						"Deal 3 damage", can_bolt, 10, DLG_MANA6(1, 0, 0, 0, 1, 0),
						"Deal 6 damage to all", can_inferno, 5+inferno_value, DLG_MANA6(5, 0, 0, 0, 2, 0));

	if (event == EVENT_CAN_ACTIVATE){
		return choice;
	}
	else if (event == EVENT_ACTIVATE){
		if( choice == CHOICE_KILL_BLUE_PERMANENT ){
			if( pick_target(&td, "TARGET_PERMANENT") ){
				discard(player, 0, player);
				instance->number_of_targets = 1;
				tap_card(player, card);
			}
		}
		else if( choice == CHOICE_BOLT ){
			if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
				discard(player, 0, player);
				instance->number_of_targets = 1;
				tap_card(player, card);
			}
		}
		else if( choice == CHOICE_INFERNO ){
			discard(player, 0, player);
			tap_card(player, card);
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
		if( choice == CHOICE_KILL_BLUE_PERMANENT ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
			}
		}
		else if( choice == CHOICE_BOLT ){
			if( valid_target(&td1) ){
				int dc = damage_target0(player, card, 3);
				if( instance->targets[0].card != -1 ){
					get_card_instance(player, dc)->targets[3].card |= DMG_CANT_REGENERATE_IF_DEALT_DAMAGE_THIS_WAY;
				}
			}
		}
		else if( choice == CHOICE_INFERNO ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_damage_all(player, card, 2, 6, NDA_ALL_CREATURES+NDA_PLAYER_TOO, &this_test);
		}
	}
	return 0;
}

int card_jediths_dragoons(int player, int card, event_t event){
	vigilance(player, card, event);
	return cip_lifegain(player, card, event, 4);
}

static const char* target_must_be_permanent_or_suspend_and_have_time_counters(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  const char* perm_or_susp = target_must_be_permanent_or_suspend(who_chooses, player, card, targeting_player, targeting_card);
  if (perm_or_susp)
	return perm_or_susp;
  return count_counters(player, card, COUNTER_TIME) ? NULL : "must have counters";
}
int card_jhoiras_timebug(int player, int card, event_t event){

	/* Jhoira's Timebug	|2
	 * Artifact Creature - Insect 1/2
	 * |T: Choose target permanent you control or suspended card you own. If that permanent or card has a time counter on it, you may remove a time counter from
	 * it or put another time counter on it. */

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT | TYPE_EFFECT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	if (IS_AI(player))
	  td.extra = (int)target_must_be_permanent_or_suspend_and_have_time_counters;
	else
	  td.extra = (int)target_must_be_permanent_or_suspend;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if (valid_target(&td) && count_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_TIME)){
			int choice = do_dialog(player, player, card, -1, -1, " Add a time counter\n Remove a time counter", is_what(instance->targets[0].player, instance->targets[0].card, TYPE_EFFECT) ? 1 : 0);
			if( choice == 0 ){
				add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_TIME);
			}
			else{
				remove_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_TIME);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CARD");
}

int card_kaervek_the_merciless(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( can_target(&td1) && specific_spell_played(player, card, event, 1-player, 2, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
			instance->number_of_targets = 1;
			damage_creature_or_player(player, card, event, get_cmc(instance->targets[1].player, instance->targets[1].card));
		}
	}
	return 0;
}

int card_kher_keep(int player, int card, event_t event){
	/*
	  Kher Keep
	  Legendary Land
	  {T}: Add {1} to your mana pool.
	  {1}{R}, {T}: Put a 0/1 red Kobold creature token named Kobolds of Kher Keep onto the battlefield.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if (event == EVENT_ACTIVATE){
		instance->info_slot = 0;
		int choice = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_XR(2, 1), 0, NULL, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Generate a Kobolds of Kher Keep\n Cancel", 1);
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				if( charge_mana_for_activated_ability(player, card, MANACOST_XR(1, 1)) ){
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

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			generate_token_by_id(player, card, CARD_ID_KOBOLDS_OF_KHER_KEEP);
		}
	}

	return 0;
}


int card_knight_of_the_holy_nimbus(int player, int card, event_t event){

	flanking(player, card, event);

	holy_nimbus(player, card, event, 2);

	return 0;
}

int card_krosan_grip(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		add_state(player, card, STATE_CANNOT_TARGET);
		if( pick_target(&td, "DISENCHANT") ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		else{
			remove_state(player, card, STATE_CANNOT_TARGET);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		remove_state(player, card, STATE_CANNOT_TARGET);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_liege_of_the_pit(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		state_untargettable(player, card, 1);
		if( ! sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			damage_player(player, 7, player, card);
		}
		state_untargettable(player, card, 0);
	}

	return morph(player, card, event, 0, 4, 0, 0, 0, 0);
}

int card_lightning_axe(int player, int card, event_t event)
{
  /* Lightning Axe	|R
   * Instant
   * As an additional cost to cast ~, discard a card or pay |5.
   * ~ deals 5 damage to target creature. */

  if (event == EVENT_MODIFY_COST)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, 0, "");
	  test.not_me = 1;
	  test.zone = TARGET_ZONE_HAND;

	  card_instance_t* instance = get_card_instance(player, card);
	  if (!check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test))
		{
		  instance->info_slot = 5;
		  COST_COLORLESS += 5;
		}
	  else
		instance->info_slot = 0;
	}

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  if (played_for_free(player, card)	// no cost
		  || instance->info_slot == 5)	// already paid
		{
		  pick_target(&td, "TARGET_CREATURE");
		  return 0;
		}

	  enum
	  {
		CHOICE_CANCEL = 0,
		CHOICE_DISCARD = 1,
		CHOICE_MANA
	  } choice = DIALOG(player, card, EVENT_CAST_SPELL,
						DLG_RANDOM, DLG_NO_STORAGE, DLG_AUTOCHOOSE_IF_1,
						"Discard", hand_count[player] > 0, 1,
						"Pay 5", has_mana(player, COLOR_ANY, 5), 3);

	  switch (choice)
		{
		  case CHOICE_CANCEL:
			cancel = 1;	// Must be done manually, since DLG_NO_STORAGE set
			break;

		  case CHOICE_DISCARD:
			if (pick_target(&td, "TARGET_CREATURE"))
			  discard(player, 0, player);
			break;

		  case CHOICE_MANA:
			charge_mana(player, COLOR_COLORLESS, 5);
			if (cancel != 1)
			  pick_target(&td, "TARGET_CREATURE");
			break;
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, 5);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_lim_dul_the_necromancer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_ZOMBIE;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY && affected_card_controller == 1-player ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4 ){
				if( instance->targets[11].player < 1 ){
					instance->targets[11].player = 1;
				}
				int pos = instance->targets[11].player;
				if( pos < 10 ){
					instance->targets[pos].card = get_id(affected_card_controller, affected_card);
					instance->targets[11].player++;
				}
			}
		}
	}

	if( instance->targets[11].player > 1 && resolve_graveyard_trigger(player, card, event) == 1 ){
		while( instance->targets[11].player > 1 && has_mana_multi(player, 1, 1, 0, 0, 0, 0) ){
				char buffer[1500];
				int i;
				int pos = scnprintf(buffer, 1500, " Pass\n");
				for(i=1; i<instance->targets[11].player; i++){
					card_ptr_t* c_me = cards_ptr[ instance->targets[i].card ];
					pos += scnprintf(buffer+pos, 1500-pos, " Zombify %s\n", c_me->name);
				}
				int choice = do_dialog(player, player, card, -1, -1, buffer, 1);
				if( choice == 0 ){
					break;
				}
				else{
					const int *grave = get_grave(1-player);
					int count = count_graveyard(1-player)-1;
					while( count > -1 ){
							if( cards_data[grave[count]].id == instance->targets[choice].card ){
								charge_mana_multi(player, 1, 1, 0, 0, 0, 0);
								if( spell_fizzled != 1 ){
									int result = reanimate_permanent(player, card, 1-player, count, REANIMATE_DEFAULT);
									add_a_subtype(player, result, SUBTYPE_ZOMBIE);
									break;
								}
							}
							count--;
					}
					int k;
					for(k=choice; k<instance->targets[11].player; k++){
						instance->targets[k].card = instance->targets[k+1].card;
					}
					instance->targets[11].player--;
				}
		}
		instance->targets[11].player = 1;
	}
	if( land_can_be_played & LCBP_REGENERATION ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0) &&
			can_target(&td)
		  ){
			return is_subtype_dead(player, SUBTYPE_ZOMBIE, -1, 0);
		}
		else if( event == EVENT_ACTIVATE ){
				charge_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE_TO_REGEN") ){
					instance->number_of_targets = 1;
					if( ! can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
						spell_fizzled = 1;
					}
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				 regenerate_target(player, instance->parent_card);
		}
	}
	return 0;
}

int card_living_end(int player, int card, event_t event)
{
	/* Living End	""
	 * Sorcery
	 * Suspend 3-|2|B|B
	 * Each player exiles all creature cards from his or her graveyard, then sacrifices all creatures he or she controls,
	 * then puts all cards he or she exiled
	 * this way onto the battlefield. */

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_reanimate_all(player, card, ANYBODY, &this_test, REANIMATE_SPECIAL_LIVING_DEAD);
		kill_card(player, card, KILL_DESTROY);
	}

	return suspend_only(player, card, event, 3, MANACOST_XB(2,2));
}

int card_lockets_of_yesterday(int player, int card, event_t event){
	const int *grave = get_grave(player);
	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player && grave[0] != -1 ){
		COST_COLORLESS-=count_graveyard_by_id(player, get_id(affected_card_controller, affected_card));
	}

	return 0;
}

int card_looter_il_kor(int player, int card, event_t event){
	shadow(player, card, event);
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		draw_a_card(player);
		discard(player, 0, player);
	}
	return 0;
}


int card_lotus_bloom(int player, int card, event_t event)
{
  /* Lotus Bloom	""
   * Artifact
   * Suspend 3-|0
   * |T, Sacrifice ~: Add three mana of any one color to your mana pool. */

  int rval = suspend_only(player, card, event, 3, MANACOST_X(0));

  return rval ? rval : card_black_lotus(player, card, event);
}

int card_candelabra_of_tawnos(int player, int card, event_t event);
int card_magus_of_the_candelabra(int player, int card, event_t event)
{
  if (event == EVENT_ATTACK_RATING && affect_me(player, card))
	ai_defensive_modifier += 24 / (landsofcolor_controlled[player][COLOR_ANY] + 2);
  else if (event == EVENT_BLOCK_RATING && affect_me(player, card))
	ai_defensive_modifier -= 96 / (landsofcolor_controlled[player][COLOR_ANY] + 2);
  else
	return card_candelabra_of_tawnos(player, card, event);

  return 0;
}

int card_magus_of_the_jar(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		return card_memory_jar(player, card, event);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_mirror_universe(int player, int card, event_t event);
int card_magus_of_the_mirror(int player, int card, event_t event){
	return card_mirror_universe(player, card, event);
}

int card_magus_of_the_scroll(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	if( event == EVENT_CAN_ACTIVATE && hand_count[player] > 0 ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td1, "TARGET_CREATURE_OR_PLAYER");
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td1) ){
				if( hand_count[player] == 0 ){ return 0; }
				char msg[100] = "Select a card to name.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
				if( player == AI ){
					do_dialog(HUMAN, player, card, player, selected, "AI Names...", 0);
				}
				int sel2 = get_random_card_in_hand(player);
				reveal_card(player, card, player, sel2);
				if( get_id(player, sel2) == get_id(player, selected) ){
					damage_creature_or_player(player, card, event, 2);
				}
			}
	}
	return 0;
}

int card_mangara_of_corondor(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			if( in_play(player, instance->parent_card)  ){
				kill_card(player, instance->parent_card, KILL_REMOVE);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PERMANENT");
}

int card_might_of_old_krosa(int player, int card, event_t event){

	/* Might of Old Krosa	|G
	 * Instant
	 * Target creature gets +2/+2 until end of turn. If you cast this spell during your main phase, that creature gets +4/+4 until end of turn instead. */

	int pump = 2;
	if( current_turn == player && (current_phase == PHASE_MAIN1 || current_phase == PHASE_MAIN2) ){
		pump = 4;
	}
	return vanilla_instant_pump(player, card, event, ANYBODY, player, pump, pump, 0, 0);
}

int card_might_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 2, 2, 0, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int mindlash_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			APNAP(p1, {discard(p1, 0, player);};);
		}

		return shared_sliver_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(1), 0, NULL, NULL);
	}

	return 0;
}

int card_mindlash_sliver(int player, int card, event_t event){
	/*
	  Mindlash Sliver |B
	  Creature - Sliver 1/1,
	  All Slivers have "{1}, Sacrifice this permanent: Each player discards a card."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &mindlash_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &mindlash_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}


int card_mishra_artificer_prodigy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		tutor_card_with_the_same_name(instance->targets[1].player, instance->targets[1].card, player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0);
	}
	return 0;
}

int card_mogg_war_marshal(int player, int card, event_t event){

	echo(player, card, event, 1, 0, 0, 0, 1, 0);
	if( comes_into_play(player, card, event) || graveyard_from_play(player, card, event)){
		generate_token_by_id(player, card, CARD_ID_GOBLIN);
	}
	return 0;
}

int card_molder2(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td1)){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td1, "DISENCHANT") ){
			int amount = get_cmc(instance->targets[0].player, instance->targets[0].card);
			charge_mana(player, COLOR_COLORLESS, amount);
			if( spell_fizzled != 1 ){
				instance->info_slot = amount;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			gain_life(player, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_momentary_blink(int player, int card, event_t event){

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
			if( get_flashback(player, card) ){
				instance->info_slot = 66;
			}
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
				blink_effect(instance->targets[0].player, instance->targets[0].card, 0);
			}
			if( instance->info_slot == 66){
				kill_card(player, card, KILL_REMOVE );
			}
			else{
				kill_card(player, card, KILL_DESTROY );
			}
	}
	return do_flashback(player, card, event, 3, 0, 1, 0, 0, 0);
}

int card_mwonvuli_acid_moss(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_LAND);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td1, "TARGET_LAND");
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, 1, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int is_instant_or_has_flash(int iid, int has_teferi, int unused1, int unused2){
	/* Approximation, but it works as the current implementation of "flash" requires that the card has TYPE_INSTANT or TYPE_INTERRUPT plus its other types.
	 * Deliberately don't use is_what(), which ignores the TYPE_INSTANT bit for cards with any permanent types. */
	return cards_data[iid].type & (TYPE_INSTANT | TYPE_INTERRUPT | (has_teferi ? TYPE_CREATURE : 0));
}

int card_mystical_teachings(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select an instant or a card with flash.");
			this_test.special_selection_function = &is_instant_or_has_flash;
			this_test.value_for_special_selection_function = check_battlefield_for_id(player, CARD_ID_TEFERI_MAGE_OF_ZHALFIR);
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			if( get_flashback(player, card) ){
			   kill_card(player, card, KILL_REMOVE);
			}
			else{
				 kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, MANACOST_XB(5, 1));
}

int card_nantuko_shaman(int player, int card, event_t event){

	/* Nantuko Shaman	|2|G
	 * Creature - Insect Shaman 3/2
	 * When ~ enters the battlefield, if you control no tapped lands, draw a card.
	 * Suspend 1-|2|G|G */

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.state = STATE_TAPPED;
		if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			draw_cards(player, 1);
		}
	}
	return suspend(player, card, event, 1, MANACOST_XG(2, 2), NULL, NULL);
}

int card_nether_traitor(int player, int card, event_t event){
	haste(player, card, event);
	shadow(player, card, event);
	return 0;
}

int card_nightshade_assassin(int player, int card, event_t event){

	if(comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player))){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.allow_cancel = 0;

		if (can_target(&td1)){
			int amount = reveal_any_number_of_cards_of_selected_color(player, card, COLOR_TEST_BLACK);
			if (amount > 0 && pick_target(&td1, "TARGET_CREATURE")){
				card_instance_t* instance = get_card_instance(player, card);
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -amount, -amount);
			}
		}
	}
	return madness(player, card, event, MANACOST_XB(1, 1));
}

int card_norin_the_wary(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // When a player casts a spell or a creature attacks, exile ~. Return it to the battlefield under its owner's control at the beginning of the next end step.
  if (new_specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, NULL)
	  || declare_attackers_trigger(player, card, event, 0, 2, -1))
	remove_until_eot(player, card, player, card);

  return 0;
}

int card_opaline_sliver(int player, int card, event_t event)
{
  const target_t* targets = any_becomes_target(player, card, event, player, 0, TYPE_PERMANENT, SUBTYPE_SLIVER, TYPE_NONEFFECT, 1-player, RESOLVE_TRIGGER_AI(player));
  if (targets)
	{
	  int num = 0;
	  for (; targets->player != -1; ++targets)
		++num;

	  if (num == 1)
		draw_cards(player, 1);
	  else
		draw_up_to_n_cards(player, num);
	}

  return slivercycling(player, card, event);
}

int card_outrider_en_kor(int player, int card, event_t event){
	flanking(player, card, event);
	return en_kor_damage_prevention_ability(player, card, event);
}

int card_paradise_plume(int player, int card, event_t event){

	/* Paradise Plume	|4
	 * Artifact
	 * As ~ enters the battlefield, choose a color.
	 * Whenever a player casts a spell of the chosen color, you may gain 1 life.
	 * |T: Add one mana of the chosen color to your mana pool. */

	card_instance_t *instance = get_card_instance( player, card );

	if( specific_spell_played(player, card, event, 2, 1+player, TYPE_ANY, 0, 0, 0, instance->info_slot, 0, 0, 0, -1, 0) ){
		gain_life(player, 1);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = 1<<choose_a_color_and_show_legacy(player, card, player, -1);
	}

	return mana_producer(player, card, event);
}

int card_paradox_haze(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;
	return enchant_player(player, card, event, &td);
}

int card_pardic_dragon(int player, int card, event_t event){
	/* Pardic Dragon	|4|R|R
	 * Creature - Dragon 4/4
	 * Flying
	 * |R: ~ gets +1/+0 until end of turn.
	 * Suspend 2-|R|R
	 * Whenever an opponent casts a spell, if ~ is suspended, that player may put a time counter on ~. */
	suspend(player, card, event, 2, 0, 0, 0, 0, 2, 0, NULL, NULL);
	return generic_shade(player, card, event, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0);
}

int card_pendelhaven_elder(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.power = 1;
		this_test.toughness = 1;
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && new_make_test_in_play(player, count, -1, &this_test) ){
					pump_until_eot(player, instance->parent_card, player, count, 1, 2);
				}
				count--;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_pentarch_paladin(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	flanking(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[2].player = 1 << choose_a_color_and_show_legacy(player, card, 1-player, -1);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	if( instance->targets[2].player > -1 ){
		td.required_color = instance->targets[2].player;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_W(2), 0, &td, "TARGET_PERMANENT");
}

int card_penumbra_spider(int player, int card, event_t event){
	/* Penumbra Spider	|2|G|G
	 * Creature - Spider 2/4
	 * Reach
	 * When ~ dies, put a 2/4 |Sblack Spider creature token with reach onto the battlefield. */

	if( graveyard_from_play(player, card, event)){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIDER, &token);
		token.pow = 2;
		token.tou = 4;
		token.color_forced = COLOR_TEST_BLACK;
		generate_token(&token);
	}
	return 0;
}

int card_phantom_wurm(int player, int card, event_t event){
	/* Phantom Wurm	|4|G|G
	 * Creature - Wurm Spirit 2/0
	 * ~ enters the battlefield with four +1/+1 counters on it.
	 * If damage would be dealt to ~, prevent that damage. Remove a +1/+1 counter from ~. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 4);

	phantom_effect(player, card, event, 0);

	return 0;
}

int card_phthisis(int player, int card, event_t event)
{
  /* Phthisis	|3|B|B|B|B
   * Sorcery
   * Destroy target creature. Its controller loses life equal to its power plus its toughness.
   * Suspend 5-|1|B */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  int amount = (get_power(instance->targets[0].player, instance->targets[0].card) +
						get_toughness(instance->targets[0].player, instance->targets[0].card));
		  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		  lose_life(instance->targets[0].player, amount);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return suspend(player, card, event, 5, MANACOST_XB(1,1), &td, "TARGET_CREATURE");
}

int card_phyrexian_totem(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_XB(2, 1), 0, NULL, NULL) ){
			return 1;
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;

		enum
		{
			CHOICE_MANA = 1,
			CHOICE_ANIMATE
		};

		int can_produce = mana_producer(player, card, EVENT_CAN_ACTIVATE);
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		int can_animate = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_TYPE_CHANGE, MANACOST_XB(2, 1), 0, NULL, NULL);
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);

		int choice = DIALOG(player, card, event, DLG_RANDOM,
							"Get mana", can_produce, 5,
							"Animate", can_animate, 15);

		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}

		if( choice == CHOICE_MANA ){
			instance->info_slot = CHOICE_MANA;
			return mana_producer(player, card, event);
		}
		if( choice == CHOICE_ANIMATE ){
			set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
			generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_XB(2, 1), 0, NULL, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = CHOICE_ANIMATE;
			}
			remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		enum
		{
			CHOICE_MANA = 1,
			CHOICE_ANIMATE
		};

		if( instance->info_slot == CHOICE_MANA ){
			return mana_producer(player, card, event);
		}

		if( instance->info_slot == CHOICE_ANIMATE ){
			add_a_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_HORROR);
			int legacy = artifact_animation(instance->parent_controller, instance->parent_card,
											instance->parent_controller, instance->parent_card, 1, 5, 5, KEYWORD_TRAMPLE, 0);
			card_instance_t *leg = get_card_instance(instance->parent_controller, legacy);
			leg->targets[6].player = COLOR_TEST_BLACK;
			get_card_instance(instance->parent_controller, instance->parent_card)->targets[1].player = 66;
		}
	}

	if( instance->targets[1].player == 66 ){
		true_phyrexian_negator(player, card, event);
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_pit_keeper(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && count_graveyard_by_type(player, TYPE_CREATURE) > 3 ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

int card_plague_sliver(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int count = count_subtype(current_turn, TYPE_PERMANENT, SUBTYPE_SLIVER);
		if( count > 0 ){
			damage_player(current_turn, count, player, card);
		}
	}
	return slivercycling(player, card, event);
}

int card_plated_pegasus(int player, int card, event_t event){

	card_benevolent_unicorn(player, card, event);

	return flash(player, card, event);
}

int card_primal_forcemage(int player, int card, event_t event){
	if( trigger_cause_controller == player && trigger_condition == TRIGGER_COMES_INTO_PLAY &&
		affect_me( player, card ) && reason_for_trigger_controller == player && trigger_cause != card
	  ){
		int trig = 0;
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			trig = 1;
		}

		if( trig == 1 && check_battlefield_for_id(2, CARD_ID_TORPOR_ORB) ){
			trig = 0;
		}
		if( trig == 1 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					pump_until_eot(player, card, trigger_cause_controller, trigger_cause, 3, 3);
			}
		}
	}
	return 0;
}

int card_prismatic_lens(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = COLOR_TEST_COLORLESS;
		play_land_sound_effect_force_color(player, card, COLOR_TEST_ANY_COLORED);
		count_mana();	// So it immediately goes back to ANY_COLORED when played with exactly one mana source
	}

	if (event == EVENT_CHANGE_TYPE){
		if( has_mana(player, COLOR_COLORLESS, 2) ){
			instance->info_slot = COLOR_TEST_ANY_COLORED;
		}
		else{
			instance->info_slot = COLOR_TEST_COLORLESS;
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return instance->targets[1].player != 66 && mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		return instance->targets[1].player != 66 && mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		if (instance->targets[1].player == 66){
			return 0;
		}
		int choice = COLOR_COLORLESS;
		if( has_mana(player, COLOR_COLORLESS, 2) ){
			choice = choose_a_color_exe(player, "What kind of mana?", 1, 0, COLOR_TEST_ANY);
		}
		if( choice == -1 ){
			cancel = 1;
		} else {
			if( choice != COLOR_COLORLESS){
				instance->targets[1].player = 66;
				charge_mana(player, COLOR_COLORLESS, 1);
				instance->targets[1].player = 0;
				if( cancel == 1 ){
					return 0;
				}
			}
			produce_mana_tapped(player, card, choice, 1);
		}
	}

	return 0;
}

int psionic_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;


		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( would_validate_arbitrary_target(&td2, instance->targets[2].player, instance->targets[2].card) ){
				damage_creature(instance->targets[2].player, instance->targets[2].card, 2, instance->targets[0].player, instance->targets[0].card);
				damage_creature(instance->targets[0].player, instance->targets[0].card, 3, instance->targets[0].player, instance->targets[0].card);
			}
		}

		return shared_sliver_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0,
												&td2, "TARGET_CREATURE_OR_PLAYER");
	}

	return 0;
}

int card_psionic_sliver(int player, int card, event_t event){
	/*
	  Psionic Sliver |4|U
	  Creature - Sliver 2/2
	  All Sliver creatures have "{T}: This creature deals 2 damage to target creature or player and 3 damage to itself."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &psionic_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &psionic_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int card_psychotic_episode(int player, int card, event_t event){

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
			reveal_target_player_hand(instance->targets[0].player);
			int *deck = deck_ptr[instance->targets[0].player];
			show_deck( player, deck, 1, "Here's the first card of target player", 0, 0x7375B0 );
			int ai_choice = 0;
			if( hand_count[instance->targets[0].player] < 1 ){
				ai_choice = 0;
			}
			int choice = do_dialog(player, player, card, -1, -1, " From hand\n From deck", ai_choice);
			if( choice == 0  ){
				ec_definition_t ec;
				default_ec_definition(instance->targets[0].player, player, &ec);
				ec.effect = EC_PUT_ON_BOTTOM;

				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);
				new_effect_coercion(&ec, &this_test);
			}
			else{
				put_top_x_on_bottom(player, instance->targets[0].player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_pull_from_eternity(int player, int card, event_t event){

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
			if( count_rfg(instance->targets[0].player) > 0 ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);
				new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_RFG, TUTOR_GRAVE, 0, AI_MAX_VALUE, &this_test);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_pulmonic_sliver(int player, int card, event_t event)
{
  /* Pulmonic Sliver	|3|W|W
   * Creature - Sliver 3/3
   * All Sliver creatures have flying.
   * All Slivers have "If this permanent would be put into a graveyard, you may put it on top of its owner's library instead." */

  card_instance_t* aff;
  if (event == EVENT_GRAVEYARD_FROM_PLAY && in_play(player, card) && !is_humiliated(player, card)
	  && (aff = in_play(affected_card_controller, affected_card))
	  && aff->kill_code > 0 && aff->kill_code < KILL_REMOVE
	  && has_subtype(affected_card_controller, affected_card, SUBTYPE_SLIVER)
	  && !is_humiliated(affected_card_controller, affected_card)
	  && DIALOG(player, card, EVENT_ACTIVATE,
				DLG_WHO_CHOOSES(affected_card_controller), DLG_SMALLCARD(affected_card_controller, affected_card),
				DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_RANDOM,
				"Put on top of owner's library", 1, 1,
				"Pass", 1, 1) == 1)
	put_on_top_of_deck(affected_card_controller, affected_card);

  boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_FLYING, BCT_INCLUDE_SELF);

  return slivercycling(player, card, event);
}

int quilled_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.required_state = TARGET_STATE_IN_COMBAT;


		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( would_validate_arbitrary_target(&td2, instance->targets[2].player, instance->targets[2].card) ){
				damage_creature(instance->targets[2].player, instance->targets[2].card, 1, instance->targets[0].player, instance->targets[0].card);
			}
		}

		return shared_sliver_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
												&td2, "Select target attacking or blocking creature.");
	}

	return 0;
}

int card_quilled_sliver(int player, int card, event_t event){
	/*
	  Quilled Sliver |W
	  Creature - Sliver 1/1
	  All Slivers have "{T}: This permanent deals 1 damage to target attacking or blocking creature."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &quilled_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &quilled_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int card_reiterate(int player, int card, event_t event)
{
  // original code : 004DDEEF

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	instance->info_slot = buyback(player, card, 3, 0, 0, 0, 0, 0);

  int rval = twincast(player, card, event, NULL, &instance->targets[2].card);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (instance->info_slot == 1 && instance->targets[2].card != -1)	// i.e. it wasn't countered by having no valid targets
		bounce_permanent(player, card);
	  else
		kill_card(player, card, KILL_DESTROY);
	}

  return rval;
}

int card_restore_balance(int player, int card, event_t event)
{
  /* Restore Balance	""
   * Sorcery
   * Suspend 6-|W
   * Each player chooses a number of lands he or she controls equal to the number of lands controlled by the player who controls the fewest, then sacrifices the
   * rest. Players sacrifice creatures and discard cards the same way. */

  if (event == EVENT_RESOLVE_SPELL)
	card_balance(player, card, event);

  return suspend_only(player, card, event, 6, MANACOST_W(1));
}

int card_return_to_dust(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "DISENCHANT") ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			if( can_target(&td) && current_turn == player && (current_phase == PHASE_MAIN1 || current_phase == PHASE_MAIN2) ){
				select_target(player, card, &td, "Select target artifact or enchantment.", &(instance->targets[1]));
			}
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_REMOVE);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rift_bolt(int player, int card, event_t event)
{
  /* Rift Bolt	|2|R
   * Sorcery
   * ~ deals 3 damage to target creature or player.
   * Suspend 1-|R */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td))
		damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);

	  kill_card(player, card, KILL_DESTROY);
	}

  return suspend(player, card, event, 1, MANACOST_R(1), &td, "TARGET_CREATURE_OR_PLAYER");
}

// By jatill
int card_riftwing_cloudskate(int player, int card, event_t event){
	/* Riftwing Cloudskate	|3|U|U
	 * Creature - Illusion 2/2
	 * Flying
	 * When ~ enters the battlefield, return target permanent to its owner's hand.
	 * Suspend 3-|1|U */
	card_instance_t *instance = get_card_instance(player, card);
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;
		if( can_target(&td) ){
			pick_target(&td, "TARGET_PERMANENT");
			bounce_permanent( instance->targets[0].player, instance->targets[0].card );
		}
	}
	return suspend(player, card, event, 3, 1, 0, 1, 0, 0, 0, NULL, NULL);
}

static int saffi_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(p, c) ){
			card_instance_t *affected = get_card_instance(p, c);
			if( affected->kill_code > 0 && affected->kill_code < 4){
				instance->targets[11].player = 66;
			}
		}
	}

	if( instance->targets[11].player == 66 && resolve_graveyard_trigger(player, card, event) == 1 ){
		seek_grave_for_id_to_reanimate(player, card, player, instance->targets[1].card, REANIMATE_DEFAULT);
		kill_card(player, card, KILL_REMOVE);
		instance->targets[11].player = 0;
	}

	if( ! in_play(p, c) && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}


int card_saffi_eriksdotter(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int legacy = create_legacy_effect(player, card, &saffi_legacy);
			card_instance_t *this = get_card_instance(player, legacy);
			this->targets[0] = instance->targets[0];
			this->number_of_targets = 1;
			this->targets[1].card = get_original_id(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_sage_of_epityr(int player, int card, event_t event){//UNUSEDCARD
	if( comes_into_play(player, card, event) > 0 ){
		rearrange_top_x(player, player, 4);
	}
	return 0;
}

int card_sangrophage(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int to_tap = 1;

		if( can_pay_life(player, 2) ){
			int ai_choice = 0;
			if(life[player] < 7){
			   ai_choice = 1;
			}

			int choice = do_dialog(player, player, card, -1, -1, " Pay 2 life\n Tap this", ai_choice);
			if( choice == 0){
				lose_life(player, 2);
				to_tap = 0;
			}
		}

		if( to_tap == 1 ){
			tap_card(player, card);
		}
	}

	return 0;
}

static int sarpadian_empires_legacy(int player, int card, event_t event){

	if (event == EVENT_SET_LEGACY_EFFECT_NAME ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			char clr[10];
			scnprintf(clr, 10, "%s", cards_ptr[get_card_instance(instance->damage_target_player, instance->damage_target_card)->targets[2].player]->name);
			char subt[20];
			scnprintf(subt, 20, "%s", raw_get_subtype_text(get_card_instance(instance->damage_target_player, instance->damage_target_card)->targets[2].card));
			scnprintf(set_legacy_effect_name_addr, 51, "%s %s", clr, subt);
		}
	}

	return 0;
}

int card_sarpadian_empires_vol_vii(int player, int card, event_t event){

	/* Sarpadian Empires, Vol. VII	|3
	 * Artifact
	 * As ~ enters the battlefield, choose |Swhite Citizen, |Sblue Camarid, |Sblack Thrull, |Sred Goblin, or |Sgreen Saproling.
	 * |3, |T: Put a 1/1 creature token of the chosen color and type onto the battlefield. */


	if( event == EVENT_RESOLVE_SPELL ){
		int clrs[5] = {COLOR_TEST_BLACK, COLOR_TEST_BLUE, COLOR_TEST_GREEN, COLOR_TEST_RED, COLOR_TEST_WHITE};
		int ids[5] = {CARD_ID_THRULL, CARD_ID_CAMARID, CARD_ID_SAPROLING, CARD_ID_GOBLIN, CARD_ID_CITIZEN};
		int subs[5] = {SUBTYPE_THRULL, SUBTYPE_CAMARID, SUBTYPE_SAPROLING, SUBTYPE_GOBLIN, SUBTYPE_CITIZEN};
		int choice = do_dialog(player, player, card, -1, -1, " Black Thrull\n Blue Camarid\n Green Saproling\n Red Goblin\n White Citizen", internal_rand(5));
		card_instance_t *instance = get_card_instance(player, card);
		instance->targets[1].player = clrs[choice];
		instance->targets[2].player = CARD_ID_BLACK+choice;
		instance->targets[1].card = ids[choice];
		instance->targets[2].card = subs[choice];
		int legacy_card = create_targetted_legacy_effect(player, card, &sarpadian_empires_legacy, player, card);
		get_card_instance(player, legacy_card)->eot_toughness = EVENT_SET_LEGACY_EFFECT_NAME;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		token_generation_t token;
		default_token_definition(player, card, instance->targets[1].card, &token);
		token.color_forced = instance->targets[1].player;
		token.pow = token.tou = 1;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL);
}

int card_savage_thallid(int player, int card, event_t event){

	/* Savage Thallid	|3|G|G
	 * Creature - Fungus 5/2
	 * At the beginning of your upkeep, put a spore counter on ~.
	 * Remove three spore counters from ~: Put a 1/1 |Sgreen Saproling creature token onto the battlefield.
	 * Sacrifice a Saproling: Regenerate target Fungus. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allow_cancel = 0;
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;
	td.required_subtype = SUBTYPE_FUNGUS;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		if( (land_can_be_played & LCBP_REGENERATION) && can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0) &&
			can_target(&td)
		  ){
			return 0x63;
		}
		else{
			return can_make_saproling_from_fungus(player, card);
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			if( (land_can_be_played & LCBP_REGENERATION) && can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0) &&
				can_target(&td)
			  ){
				if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0) ){
					instance->number_of_targets = 0;
					if (pick_next_target_noload(&td, "Select target Fungus.")){
						instance->info_slot = 67;
					}
				}
				else{
					spell_fizzled = 1;
				}
			}
			else{
				saproling_from_fungus(player, card);
				instance->info_slot = 66;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	add_spore_counters(player, card, event);
	return 0;
}

int card_scion_of_the_ur_dragon(int player, int card, event_t event)
{
  /* Scion of the Ur-Dragon	|W|U|B|R|G
   * Legendary Creature - Dragon Avatar 4/4
   * Flying
   * |2: Search your library for a Dragon permanent card and put it into your graveyard. If you do, ~ becomes a copy of that card until end of turn. Then
   * shuffle your library. */

  check_legend_rule(player, card, event);

  if (event == EVENT_CAN_ACTIVATE)
	return can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(2));

  if (event == EVENT_ACTIVATE)
	charge_mana_for_activated_ability(player, card, MANACOST_X(2));

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int pp = instance->parent_controller;
	  int pc = instance->parent_card;

	  test_definition_t this_test;
	  default_test_definition(&this_test, TYPE_PERMANENT);
	  this_test.subtype = SUBTYPE_DRAGON;

	  int result = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GRAVE, 0, AI_MAX_CMC, &this_test);

	  if (result != -1 && in_play(pp, pc))
		shapeshift_target(pp, pc, pp, pc, player, result, SHAPESHIFT_UNTIL_EOT);
	}

  return 0;
}

int screeching_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td2;
		default_target_definition(player, card, &td2, 0);
		td2.zone = TARGET_ZONE_PLAYERS;


		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( would_validate_arbitrary_target(&td2, instance->targets[2].player, instance->targets[2].card) ){
				mill(instance->targets[2].player, 1);
			}
		}

		return shared_sliver_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td2, "TARGET_PLAYER");
	}

	return 0;
}

int card_screeching_sliver(int player, int card, event_t event){
	/*
	  Screeching Sliver |U
	  Creature - Sliver 1/1
	  All Slivers have "{T}: Target player puts the top card of his or her library into his or her graveyard."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &screeching_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &screeching_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int card_scryb_ranger(int player, int card, event_t event){

	if (flash(player, card, event)){
		return 1;
	}

	return card_quirion_ranger(player, card, event);
}

int card_search_for_tomorrow(int player, int card, event_t event){

	/* Search for Tomorrow	|2|G
	 * Sorcery
	 * Search your library for a basic land card and put it onto the battlefield. Then shuffle your library.
	 * Suspend 2-|G */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, 4, TYPE_LAND, 0, SUBTYPE_BASIC, 0, 0, 0, 0, 0, -1, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return suspend(player, card, event, 2, 0, 0, 0, 1, 0, 0, NULL, NULL);
}

int sedge_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			kill_card(player, card, KILL_REMOVE);
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_state = TARGET_STATE_DESTROYED;
		td.preferred_controller = player;
		td.required_subtype = SUBTYPE_SLIVER;
		td.extra = (int32_t)sliver_to_regenerate;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td.illegal_abilities = 0;

		if( event == EVENT_CAN_ACTIVATE && in_play(p, c) ){
			if( land_can_be_played & LCBP_REGENERATION ){
				int pp, cc;
				for(pp = 0; pp<2; pp++){
					for(cc=0; cc < active_cards_count[pp]; cc++){
						if( !(pp == p && cc == c) && in_play(pp, cc) && is_what(pp, cc, TYPE_PERMANENT) && has_subtype(pp, cc, SUBTYPE_SLIVER) ){
							int result = regeneration(pp, cc, EVENT_CAN_ACTIVATE, MANACOST_B(1));
							if( result ){
								return result;
							}
						}
					}
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			if( new_pick_target(&td, "Select a Sliver to regenerate.", 0, 1 | GS_LITERAL_PROMPT) ){
				instance->number_of_targets = 1;
				if( can_use_activated_abilities(instance->targets[0].player, instance->targets[0].card) ){
					charge_mana_for_activated_ability(instance->targets[0].player, instance->targets[0].card, MANACOST_B(1));
				}
				else{
					spell_fizzled = 1;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_sedge_sliver(int player, int card, event_t event){
	/*
	  Sedge Sliver |2|R
	  Creature - Sliver 2/2
	  All Sliver creatures have "This creature gets +1/+1 as long as you control a Swamp."
	  All Slivers have "{B}: Regenerate this permanent."
	*/

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && ! is_humiliated(player, card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && has_subtype(affected_card_controller, affected_card, SUBTYPE_SLIVER) ){
			if( check_battlefield_for_subtype(affected_card_controller, TYPE_LAND, SUBTYPE_SWAMP) ){
				event_result++;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &sedge_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &sedge_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}
	return slivercycling(player, card, event);
}

int card_sengir_nosferatu(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BAT, &token);
		token.tou = 2;
		token.special_infos = 66;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_RFG_ME, 1, 1, 0, 0, 0, 0, 0, 0, 0);
}

int card_serra_avenger(int player, int card, event_t event)
{
  if (event == EVENT_MODIFY_COST && affect_me(player, card)
	  && current_turn == player && get_specific_turn_count(player) <= 3)
	infinite_casting_cost();

  vigilance(player, card, event);
  return 0;
}

int card_shadow_sliver(int player, int card, event_t event){

	if (event == EVENT_ABILITIES && in_play(player, card) && !is_humiliated(player, card)
		&& has_creature_type(affected_card_controller, affected_card, SUBTYPE_SLIVER)){
		shadow(affected_card_controller, affected_card, event);
	}

	return slivercycling(player, card, event);
}

int card_sidewinder_sliver(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && in_play(affected_card_controller, affected_card) &&
		has_subtype(affected_card_controller, affected_card, SUBTYPE_SLIVER)
	  ){
		flanking(affected_card_controller, affected_card, event);
	}

	if( event == EVENT_DECLARE_BLOCKERS ){
		int p = current_turn;
		int count = active_cards_count[p]-1;
		while( count > -1 ){
				if( in_play(p, count) && has_subtype(p, count, SUBTYPE_SLIVER) && is_attacking(p, count) &&
					check_for_special_ability(p, count, SP_KEYWORD_FLANKING)
				  ){
					int c2 = active_cards_count[1-p]-1;
					while( c2 > -1 ){
							if( in_play(1-p, c2) && is_what(1-p, c2, TYPE_CREATURE) ){
								card_instance_t *this = get_card_instance(1-p, c2);
								if( this->blocking == count ){
									if( ! check_for_special_ability(1-p, c2, SP_KEYWORD_FLANKING) ){
										pump_until_eot(player, card, 1-p, c2, -1, -1);
									}
								}
							}
							c2--;
					}
				}
				count--;
		}
	}

	return slivercycling(player, card, event);
}

int card_smallpox(int player, int card, event_t event)
{
  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  APNAP(p, { lose_life(p, 1); });
	  APNAP(p, { discard(p, 0, player); });
	  APNAP(p, { impose_sacrifice(player, card, p, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0); });
	  APNAP(p, { impose_sacrifice(player, card, p, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0); });
	  kill_card(player, card, KILL_DESTROY);
	}
  return 0;
}

int card_snapback(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	al_pitchspell(player, card, event, COLOR_TEST_BLUE, 0);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( casting_al_pitchspell(player, card, event, COLOR_TEST_BLUE, 0) ){
				pick_target(&td1, "TARGET_CREATURE");
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_spectral_force(int player, int card, event_t event)
{
  // Whenever ~ attacks, if defending player controls no |Sblack permanents, it doesn't untap during your next untap step.
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && count_permanents_by_color(1-current_turn, TYPE_PERMANENT, get_sleighted_color(player, card, COLOR_TEST_BLACK)) == 0
	  && declare_attackers_trigger(player, card, event, 0, player, card))
		does_not_untap_effect(player, card, player, card, EDNT_CHECK_ORIGINAL_CONTROLLER, 1);

  return 0;
}

int card_spell_burst(int player, int card, event_t event){
	// original code : 0x4DE16F

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = counterspell(player, card, event, NULL, 0);
		if( result > 0 ){
			if( has_mana(player, COLOR_COLORLESS, get_cmc(card_on_stack_controller, card_on_stack)) ){
				return result;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		charge_mana(player, COLOR_COLORLESS, get_cmc(card_on_stack_controller, card_on_stack));
		if( spell_fizzled != 1 ){
			instance->info_slot = buyback(player, card, MANACOST_X(3));
			return counterspell(player, card, event, NULL, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int resolved = 0;
		if (counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			resolved++;
		}
		if( instance->info_slot == 1 && resolved){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_spiketail_drakeling(int player, int card, event_t event){
	/* Spiketail Drakeling	|1|U|U
	 * Creature - Drake 2/2
	 * Flying
	 * Sacrifice ~: Counter target spell unless its controller pays |2. */

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, &td, 0, 2);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_SPELL_ON_STACK, 0, 0, 0, 0, 0, 0, 0, &td, NULL);
}

int card_spinneret_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_REACH, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_sporesower_thallid(int player, int card, event_t event){

	/* Sporesower Thallid	|2|G|G
	 * Creature - Fungus 4/4
	 * At the beginning of your upkeep, put a spore counter on each Fungus you control.
	 * Remove three spore counters from ~: Put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_PERMANENT, "");
		test.subtype = SUBTYPE_FUNGUS;
		new_manipulate_all(player, card, player, &test, ACT_ADD_COUNTERS(COUNTER_SPORE, 1));
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		return can_make_saproling_from_fungus(player, card);
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			saproling_from_fungus(player, card);
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
	}

	return 0;
}

int card_sprite_noble(int player, int card, event_t event){

	if( event == EVENT_TOUGHNESS && affected_card_controller == player && affected_card != card &&
		in_play(affected_card_controller, affected_card) && check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING)
	  ){
		event_result++;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		player = instance->parent_controller;
		card = instance->parent_card;

		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( count != card && in_play(player, count) && check_for_ability(player, count, KEYWORD_FLYING) ){
					pump_until_eot(player, card, player, count, 1, 0);
				}
				count--;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_stonebrow_krosan_hero(int player, int card, event_t event)
{
  /* Stonebrow, Krosan Hero	|3|R|G
   * Legendary Creature - Centaur Warrior 4/4
   * Trample
   * Whenever a creature you control with trample attacks, it gets +2/+2 until end of turn. */

  check_legend_rule(player, card, event);

  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.keyword = KEYWORD_TRAMPLE;

	  int amt;
	  if ((amt = declare_attackers_trigger_test(player, card, event, DAT_TRACK, player, -1, &test)))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		  for (--amt; amt >= 0; --amt)
			if (in_play(current_turn, attackers[amt]))
			  pump_until_eot(player, card, current_turn, attackers[amt], 2, 2);
		}
	}

  return 0;
}

int card_stonewood_invocation(int player, int card, event_t event){
	/* Stonewood Invocation	|3|G
	 * Instant
	 * Split second
	 * Target creature gets +5/+5 and gains shroud until end of turn. */

	cannot_be_countered(player, card, event);

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 5, 5, KEYWORD_SHROUD, 0);
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		  kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_strangling_soot(int player, int card, event_t event){
	/* Strangling Soot	|2|B
	 * Instant
	 * Destroy target creature with toughness 3 or less.
	 * Flashback |5|R */

	if (IS_CASTING(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.toughness_requirement = 3 | TARGET_PT_LESSER_OR_EQUAL;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_CAST ){
			return can_target(&td);
		}

		else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
				pick_target(&td, "TARGET_CREATURE");
		}

		else if( event == EVENT_RESOLVE_SPELL ){
				if( valid_target(&td) ){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				}
				if( get_flashback(player, card) ){
				   kill_card(player, card, KILL_REMOVE);
				}
				else{
					 kill_card(player, card, KILL_DESTROY);
				}
		}
	}
	return do_flashback(player, card, event, MANACOST_XR(5,1));
}

int card_strength_in_numbers(int player, int card, event_t event){
	/* Strength in Numbers	|1|G
	 * Instant
	 * Until end of turn, target creature gains trample and gets +X/+X, where X is the number of attacking creatures. */
	int amt = (event == EVENT_CHECK_PUMP || event == EVENT_RESOLVE_SPELL) ? count_attackers(current_turn) : 0;	// Likely won't be any use during EVENT_CHECK_PUMP.  Oh well.
	return vanilla_instant_pump(player, card, event, ANYBODY, player, amt, amt, KEYWORD_TRAMPLE, 0);
}

int card_stronghold_overseer(int player, int card, event_t event){
	shadow(player, card, event);
	if(event == EVENT_RESOLVE_ACTIVATION){
		int p;
		for( p = 0; p < 2; p++){
			int count = 0;
			while(count < active_cards_count[p]){
				card_data_t* card_d = get_card_data(p, count);
				if((card_d->type & TYPE_CREATURE) && in_play(p, count)){
					if( check_for_special_ability(p, count, SP_KEYWORD_SHADOW) ){
						pump_until_eot(player, card, p, count, 1, 0);
					}
					else{
						pump_until_eot(player, card, p, count, -1, 0);
					}
				}
				count++;
			}
		}
	}
	return generic_activated_ability(player, card, event, 0, 0, 2, 0, 0, 0, 0, 0, 0, 0);
}

int card_stuffy_doll(int player, int card, event_t event){

	indestructible(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;
		if( pick_target(&td, "TARGET_PLAYER") ){
			instance->targets[1] = instance->targets[0];
		}
		instance->number_of_targets = 0;
	}

	if( damage_dealt_to_me_arbitrary(player, card, event, 0, player, card) && instance->targets[1].player > -1){
		damage_player(instance->targets[1].player, instance->targets[7].card, player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		damage_creature(instance->parent_controller, instance->parent_card, 1, instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_subterranean_shambler(int player, int card, event_t event){
	echo(player, card, event, 1, 0, 0, 0, 1, 0);
	if( comes_into_play(player, card, event) || graveyard_from_play(player, card, event)){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		this_test.keyword_flag = 1;
		new_damage_all(player, card, 2, 1, 0, &this_test);
	}
	return 0;
}

int card_sudden_death(int player, int card, event_t event){
	/* Sudden Death	|1|B|B
	 * Instant
	 * Split second
	 * Target creature gets -4/-4 until end of turn. */

	cannot_be_countered(player, card, event);

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -4, -4);
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		  kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sudden_shock(int player, int card, event_t event){
	/* Sudden Shock	|1|R
	 * Instant
	 * Split second
	 * ~ deals 2 damage to target creature or player. */

	cannot_be_countered(player, card, event);

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				damage_creature_or_player(player, card, event, 2);
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		  kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sudden_spoiling(int player, int card, event_t event){
	/*
	  Sudden Spoiling |1|B|B
	  Instant
	  Split second (As long as this spell is on the stack, players can't cast spells or activate abilities that aren't mana abilities.)
	  Creatures target player controls become 0/2 and lose all abilities until end of turn.
	*/

	cannot_be_countered(player, card, event);

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			int p = instance->targets[0].player;

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			test_definition_t hc;
			default_test_definition(&hc, 0);
			hc.power = 0;
			hc.toughness = 2;

			int c;
			for (c = active_cards_count[p]-1; c > -1; c--){
				if (in_play(p, c) && is_what(p, c, TYPE_CREATURE)){
					humiliate_and_set_pt_abilities(player, card, p, c, 4, &hc);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sulfurous_blast(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = 2;
		if( current_turn == player && (current_phase == PHASE_MAIN1 || current_phase == PHASE_MAIN2) ){
			amount = 3;
		}
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_damage_all(player, card, 2, amount, NDA_ALL_CREATURES+NDA_PLAYER_TOO, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static const char* is_saprofite(int who_chooses, int player, int card){
	if( has_subtype(player, card, SUBTYPE_INSECT) || has_subtype(player, card, SUBTYPE_RAT) || has_subtype(player, card, SUBTYPE_SPIDER) ||
		has_subtype(player, card, SUBTYPE_SQUIRREL)
	  ){
		return NULL;
	}
	else{
		return "must be Insect, Rat, Spider, or Squirrel";
	}
}

int card_swarmyard(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.preferred_controller = player;
	td.extra = (int32_t)is_saprofite;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special |= TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

#define CAN_REGEN	(generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_REGENERATION|GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, NULL))
#define CAN_MANA	(mana_producer(player, card, EVENT_CAN_ACTIVATE))

	if( event == EVENT_CAN_ACTIVATE ){
		if (CAN_REGEN){
			return 99;
		}
		return CAN_MANA;
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		instance->info_slot = 0;
		if(! paying_mana() && CAN_REGEN){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Regenerate\n Cancel", 1);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		if( choice == 1){
			instance->number_of_targets = 0;
			if( charge_mana_for_activated_ability(player, card, MANACOST0) &&
				new_pick_target(&td, "Select target Insect, Rat, Spider, or Squirrel.", 0, 1 | GS_LITERAL_PROMPT)
			  ){
				instance->info_slot = 1;
				tap_card(player, card);
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( can_be_regenerated(instance->targets[0].player, instance->targets[0].card) ){
				regenerate_target(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
#undef CAN_REGEN
#undef CAN_MANA
}

int card_teferi_mage_of_zhalfir(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_HAND;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_target(&td) ){
		return check_playable_permanents(player, TYPE_CREATURE, 1);
	}

	if( event == EVENT_ACTIVATE){
		instance->targets[0].card =  -1;
		if( player == AI ){
			instance->targets[0].card = check_playable_permanents(player, TYPE_CREATURE, 0);
		}
		else{
			 if( pick_target(&td, "TARGET_CREATURE") ){
				 instance->number_of_targets = 1;
			 }
		}
		if( instance->targets[0].card != -1 ){
			int id = get_id(player, instance->targets[0].card);
			if( has_mana_to_cast_id(player, event, id) ){
				card_instance_t *this = get_card_instance(player, instance->targets[0].card);
				if( can_legally_play_iid(player, this->internal_card_id) ){
					charge_mana_from_id(player, instance->targets[0].card, event, id);
					if( spell_fizzled != 1 ){
						play_card_in_hand_for_free(player, instance->targets[0].card);
						cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
					}
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

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller != player ){
		int effect = 0;
		if( current_turn != 1-player ){
			effect = 1;
		}
		else{
			 if( current_phase != PHASE_MAIN1 && current_phase != PHASE_MAIN2 ){
				 effect = 1;
			 }
		}

		if( effect == 1 ){
			infinite_casting_cost();
		}
	}

	return flash(player, card, event);
}

int telekinetik_sliver_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) ){
			add_status(player, card, STATUS_INVISIBLE_FX);
		}

		if( ! in_play(p, c) ){
			if( event == EVENT_CAN_ACTIVATE ){
				return 0;
			}

			if( event == EVENT_CLEANUP ){
				kill_card(player, card, KILL_REMOVE);
			}
		}

		if( is_humiliated(p, c) ){
			return 0;
		}

		if( ! IS_GAA_EVENT(event) ){
			return 0;
		}

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_PERMANENT);


		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( would_validate_arbitrary_target(&td2, instance->targets[2].player, instance->targets[2].card) ){
				tap_card(instance->targets[2].player, instance->targets[2].card);
			}
		}

		return shared_sliver_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td2, "TARGET_PERMANENT");
	}

	return 0;
}

int card_telekinetik_sliver(int player, int card, event_t event){
	/*
	  Telekinetic Sliver |2|U|U
	  Creature - Sliver 2/2
	  All Slivers have "{T}: Tap target permanent."
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_activate(player, card, &telekinetik_sliver_shared_ability);
		card_instance_t *instance = get_card_instance(player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
		legacy = create_legacy_activate(1-player, fake, &telekinetik_sliver_shared_ability);
		instance = get_card_instance(1-player, legacy);
		instance->targets[1].player = player;
		instance->targets[1].card = card;
		instance->number_of_targets = 1;
		obliterate_card(1-player, fake);
	}

	return slivercycling(player, card, event);
}

int card_temporal_isolation(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_PREVENT_DAMAGE ){
			card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
			if( damage_card == source->internal_card_id  && source->damage_source_player == instance->damage_target_player &&
				source->damage_source_card == instance->damage_target_card
			  ){
				source->info_slot = 0;
			}
		}
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, SP_KEYWORD_SHADOW, 0, 0, 0);
}

int card_tendrils_of_corruption(int player, int card, event_t event){

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
				int amount = count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP);
				damage_creature(instance->targets[0].player, instance->targets[0].card, amount, player, card);
				gain_life(player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_thallid_germinator(int player, int card, event_t event){

	/* Thallid Germinator	|2|G
	 * Creature - Fungus 2/2
	 * At the beginning of your upkeep, put a spore counter on ~.
	 * Remove three spore counters from ~: Put a 1/1 |Sgreen Saproling creature token onto the battlefield.
	 * Sacrifice a Saproling: Target creature gets +1/+1 until end of turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		if( can_target(&td) && can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0) ){
			return 1;
		}
		return can_make_saproling_from_fungus(player, card);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int choice = 0;
			if( can_make_saproling_from_fungus(player, card) ){
				if( can_target(&td) && can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Generate a Saproling\n Pump a creature\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				saproling_from_fungus(player, card);
				instance->info_slot = 66+choice;
			}
			else if( choice == 1 ){
					if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_SAPROLING, 0, 0, 0, 0, 0, -1, 0) ){
						if( pick_target(&td, "TARGET_CREATURE") ){
							instance->info_slot = 66+choice;
							instance->number_of_targets = 1;
						}
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

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	add_spore_counters(player, card, event);
	return 0;
}

int card_thelon_of_havenwood(int player, int card, event_t event){

	/* Thelon of Havenwood
	 * |G|G
	 * Legendary Creature - Elf Druid 2/2
	 * Each Fungus creature gets +1/+1 for each spore counter on it.
	 * |B|G, Exile a Fungus card from a graveyard: Put a spore counter on each Fungus on the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if(event == EVENT_POWER || event == EVENT_TOUGHNESS){
		if( has_subtype(affected_card_controller, affected_card, SUBTYPE_FUNGUS) && in_play(affected_card_controller, affected_card) ){
			event_result += count_counters(affected_card_controller, affected_card, COUNTER_SPORE);
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		return (has_mana_for_activated_ability(player, card, MANACOST_BG(1,1))
				&& any_in_graveyard_by_subtype(ANYBODY, SUBTYPE_FUNGUS)
				&& !graveyard_has_shroud(ANYBODY));
	}

	else if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 1, 0, 1, 0, 0);
			if( spell_fizzled != 1){
				instance->targets[0].player = 1-player;
				if( any_in_graveyard_by_subtype(1-player, SUBTYPE_FUNGUS)){
					if( any_in_graveyard_by_subtype(player, SUBTYPE_FUNGUS)){
						pick_target(&td, "TARGET_PLAYER");
					}
				}
				else{
					instance->targets[0].player = player;
				}

				int selected = select_a_card(player, instance->targets[0].player, TUTOR_FROM_GRAVE, 0, 1, -1, TYPE_PERMANENT, 0, SUBTYPE_FUNGUS, 0,
											 0, 0, 0, 0, -1, 0);
				if( selected != -1 ){
					remove_card_from_grave(instance->targets[0].player, selected);
				}
				else{
					spell_fizzled = 1;
				}
			}
	}

	else if(event == EVENT_RESOLVE_ACTIVATION){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_PERMANENT, "");
		test.subtype = SUBTYPE_FUNGUS;
		new_manipulate_all(player, card, ANYBODY, &test, ACT_ADD_COUNTERS(COUNTER_SPORE, 1));
	}

	return 0;
}

int card_thelonite_hermit(int player, int card, event_t event){
	/* Thelonite Hermit	|3|G
	 * Creature - Elf Shaman 1/1
	 * Saproling creatures get +1/+1.
	 * Morph |3|G|G
	 * When ~ is turned face up, put four 1/1 |Sgreen Saproling creature tokens onto the battlefield. */

	if(event == EVENT_TURNED_FACE_UP ){
		generate_tokens_by_id(player, card, CARD_ID_SAPROLING, 4);
	}
	boost_creature_type(player, card, event, SUBTYPE_SAPROLING, 1, 1, 0, BCT_INCLUDE_SELF);
	return morph(player, card, event, 3, 0, 0, 2, 0, 0);
}

int card_thick_skinned_goblin(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, KEYWORD_PROT_RED, 0);
	}

	return generic_activated_ability(player, card, event, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
}

int card_think_twice(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
			return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL){
			draw_cards(player, 1);
			if( get_flashback(player, card) ){
			   kill_card(player, card, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return do_flashback(player, card, event, 2, 0, 1, 0, 0, 0);
}

int card_thunder_totem(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 1, 0, 0, 0, 0, 2, 0, 0, 0) ){
			return 1;
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		int mode = (1<<2);
		if( mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			mode |= (1<<0);
		}
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_TYPE_CHANGE, 1, 0, 0, 0, 0, 2, 0, 0, 0) ){
			mode |=(1<<1);
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		int choice = 0;
		if( ! paying_mana() && (mode & (1<<1)) ){
			char buffer[500];
			int pos = 0;
			int ai_choice = 1;
			if( mode & (1<<0) ){
				pos += scnprintf(buffer + pos, 500-pos, " Get mana\n", buffer);
			}
			if( mode & (1<<1) ){
				pos += scnprintf(buffer + pos, 500-pos, " Animate\n", buffer);
			}
			pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
			choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			while( !( (1<<choice) & mode) ){
					choice++;
			}
		}
		if( choice == 0 ){
			instance->info_slot = 66+choice;
			return mana_producer(player, card, event);
		}
		else if( choice == 1){
				set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
				if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 1, 0, 0, 0, 0, 2, 0, 0, 0) ){
					instance->info_slot = 66+choice;
				}
				remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			return mana_producer(player, card, event);
		}
		if( instance->info_slot == 67 ){
			add_a_subtype(player, instance->parent_card, SUBTYPE_ELEMENTAL);
			int legacy = artifact_animation(player, instance->parent_card, player, instance->parent_card, 1, 2, 2, KEYWORD_FLYING+KEYWORD_FIRST_STRIKE, 0);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[6].player = COLOR_TEST_WHITE;
		}
	}
	return 0;
}

int card_tivadar_of_thorn(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_GOBLIN;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if (comes_into_play(player, card, event) && can_target(&td)){
		instance->number_of_targets = 0;
		if (pick_next_target_noload(&td, "Select target Goblin.")){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_trespasser_il_vec(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card);
	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, 0, SP_KEYWORD_SHADOW);
	}
	return generic_activated_ability(player, card, event, GAA_DISCARD, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_triskelavus(int player, int card, event_t event){
	/* Triskelavus	|7
	 * Artifact Creature - Construct 1/1
	 * Flying
	 * ~ enters the battlefield with three +1/+1 counters on it.
	 * |1, Remove a +1/+1 counter from ~: Put a 1/1 colorless Triskelavite artifact creature token with flying onto the battlefield. It has "Sacrifice this creature: This creature deals 1 damage to target creature or player." */

	if( event == EVENT_RESOLVE_SPELL ){
		add_1_1_counters(player, card, 3);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_TRISKELAVITE);
	}

	return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_trump_the_domains(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, count_domain(player, card), count_domain(player, card), KEYWORD_TRAMPLE, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_unyaro_bees(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_activated_abilities(player, card) ){
			if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		int mode = (1<<2);
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
			mode |= (1<<0);
		}
		if( has_mana_for_activated_ability(player, card, 3, 0, 0, 1, 0, 0) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			mode |=(1<<1);
		}
		int choice = 0;
		if( (mode & (1<<0)) && (mode & (1<<1)) ){
			int ai_choice = 0;
			if( mode & (1<<1) && current_phase != PHASE_AFTER_BLOCKING ){
				ai_choice = 1;
			}
			choice = do_dialog(player, player, card, -1, -1, " Pump\n Sac & damage\n Cancel", ai_choice);
		}
		while( !( (1<<choice) & mode) ){
				choice++;
		}
		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1){
				instance->targets[3].player = 66;
				charge_mana_for_activated_ability(player, card, 3, 0, 0, 1, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					instance->info_slot = 66+choice;
					kill_card(player, card, KILL_SACRIFICE);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, 1);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
	}

	return 0;
}

int card_urborg_syphon_mage(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		int result = lose_life(1-player, 2);
		gain_life(player, result);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD, 2, 1, 0, 0, 0, 0, 0, 0, 0);
}

int card_urzas_factory(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card)  ){
		if( ! is_tapped(player, card) && can_produce_mana(player, card) ){
			declare_mana_available(player, COLOR_COLORLESS, 1);
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_use_activated_abilities(player, card) ){
			if( has_mana_for_activated_ability(player, card, 8, 0, 0, 0, 0, 0) ){
				return 1;
			}
		}
		if( ! is_tapped(player, card) ){
			return can_produce_mana(player, card);
		}
	}

	if(event == EVENT_ACTIVATE ){
		int mode = (1<<0)+(1<<2);
		if( can_use_activated_abilities(player, card) ){
			if( has_mana_for_activated_ability(player, card, 8, 0, 0, 0, 0, 0) ){
				mode |=(1<<1);
			}
		}
		int choice = 0;
		if( ! paying_mana() && (mode & (1<<1)) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Generate an Assembly Worker\n Cancel", 1);
		}
		while( !( (1<<choice) & mode) ){
				choice++;
		}
		if( choice == 0 ){
			instance->info_slot = 66+choice;
			return mana_producer(player, card, event);
		}
		else if( choice == 1){
				tap_card(player, card);
				charge_mana_for_activated_ability(player, card, 7, 0, 0, 0, 0, 0);
				if( spell_fizzled != 1 ){
					instance->info_slot = 66+choice;
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
		if( instance->info_slot == 66 ){
			return mana_producer(player, card, event);
		}
		if( instance->info_slot == 67 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ASSEMBLY_WORKER, &token);
			token.special_infos = 66;
			generate_token(&token);

			card_instance_t *parent = get_card_instance(player, instance->parent_card);
			parent->info_slot = 0;
		}
	}

	return 0;
}

int card_vampiric_sliver(int player, int card, event_t event, int trig_mode){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->info_slot > 0 ){
			if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) &&
				has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_SLIVER)
			  ){
				if( instance->targets[0].player < 10 ){
					if( instance->targets[0].player < 1 ){
						instance->targets[0].player = 1;
					}
					int pos = instance->targets[0].player;
					instance->targets[pos].player = damage->damage_target_player;
					instance->targets[pos].card = damage->damage_target_card;
					instance->targets[0].player++;
				}
			}
		}
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4 ){
				if( instance->targets[0].player > 1 ){
					int i;
					for(i=1; i<instance->targets[0].player; i++){
						if( instance->targets[i].player == affected_card_controller &&
							instance->targets[i].card == affected_card
						  ){
							if( instance->targets[0].card < 0 ){
								instance->targets[0].card = 0;
							}
							instance->targets[0].card |= (1<<i);
						}
					}
				}
			}
		}
	}

	if( instance->targets[0].player > 1 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
		affect_me(player, card )
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= trig_mode;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=1; i<instance->targets[0].player; i++){
					if(instance->targets[0].card & (1<<i) ){
						add_1_1_counter(instance->targets[i].player, instance->targets[i].card);
					}
				}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[0].player = 1;
	}

	return slivercycling(player, card, event);
}

int card_verdant_embrace(int player, int card, event_t event, int trig_mode){
	/* Verdant Embrace	|3|G|G
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +3/+3 and has "At the beginning of each upkeep, put a 1/1 |Sgreen Saproling creature token onto the battlefield." */

	card_instance_t *instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player != -1 ){

		upkeep_trigger_ability(player, card, event, 2);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SAPROLING, &token);
			token.t_player = instance->damage_target_player;
			token.qty = count_upkeeps(current_turn);
			generate_token(&token);
		}

	}
	return generic_aura(player, card, event, player, 3, 3, 0, 0, 0, 0, 0);
}

int card_vesuva(int player, int card, event_t event)
{
  /* Vesuva	""
   * Land
   * You may have ~ enter the battlefield tapped as a copy of any land on the battlefield. */

  target_definition_t td;
  if (event == EVENT_RESOLVE_SPELL)
	{
	  base_target_definition(player, card, &td, TYPE_LAND);
	  td.allowed_controller = ANYBODY;
	  td.preferred_controller = player;
	}

  if (enters_the_battlefield_as_copy_of_any(player, card, event, &td, "SELECT_A_LAND"))
	get_card_instance(player, card)->state |= STATE_TAPPED;	// avoid sending event
  else if (event == EVENT_RESOLVE_SPELL)
	play_land_sound_effect(player, card);	// Resolving as itself

  return 0;
}

static int effect_vesuvan_shapeshifter(int player, int card, event_t event)
{
  if (effect_follows_control_of_attachment(player, card, event))
	return 0;

  if (current_turn == player && upkeep_trigger_mode(player, card, event, RESOLVE_TRIGGER_OPTIONAL))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  turn_face_down(instance->damage_target_player, instance->damage_target_card);
	  kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}
int card_vesuvan_shapeshifter(int player, int card, event_t event)
{
  /* Vesuvan Shapeshifter	|3|U|U
   * Creature - Shapeshifter 0/0
   * As ~ enters the battlefield or is turned face up, you may choose another creature on the battlefield. If you do, until ~ is turned face down, it becomes a
   * copy of that creature and gains "At the beginning of your upkeep, you may turn this creature face down."
   * Morph |1|U */

  target_definition_t td;
  if (event == EVENT_RESOLVE_SPELL || event == EVENT_TURNED_FACE_UP)
	{
	  base_target_definition(player, card, &td, TYPE_CREATURE);
	  td.special = TARGET_SPECIAL_NOT_ME;
	}

  if (enters_the_battlefield_as_copy_of_any(player, card, event, &td, "SELECT_ANOTHER_CREATURE"))
	set_legacy_image(player, CARD_ID_VESUVAN_SHAPESHIFTER, create_targetted_legacy_activate(player, card, &effect_vesuvan_shapeshifter, player, card));

  if (event == EVENT_TURNED_FACE_UP)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (pick_target(&td, "SELECT_ANOTHER_CREATURE"))
		{
		  instance->number_of_targets = 0;

		  cloning(player, card, instance->targets[0].player, instance->targets[0].card);	// legend rule will be verified by flip_card()
		  set_legacy_image(player, CARD_ID_VESUVAN_SHAPESHIFTER, create_targetted_legacy_activate(player, card, &effect_vesuvan_shapeshifter, player, card));

		  dispatch_event_to_single_card_overriding_function(player, card, EVENT_TURNED_FACE_UP, instance->targets[12].card);
		}
	}

  return morph(player, card, event, MANACOST_XU(1,1));
}

int card_volcanic_awakening(int player, int card, event_t event){

	/* Volcanic Awakening	|4|R|R
	 * Sorcery
	 * Destroy target land.
	 * Storm */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
		if( spell_fizzled != 1 && ! check_special_flags(player, card, SF_NOT_CAST) ){
			int i;
			for(i=0;i<get_storm_count();i++){
				if( new_pick_target(&td, "TARGET_LAND", 1, 0) ){
					kill_card(instance->targets[1].player, instance->targets[1].card, KILL_DESTROY);
				}
			}
			instance->number_of_targets = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_walk_the_aeons(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if (player == AI){
		td.allowed_controller = player;
	}
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if (!pick_target(&td, "TARGET_PLAYER")){
			return 0;
		}
		int tgt_player = instance->targets[0].player;

		if( ! is_token(player, card) && can_sacrifice_as_cost(player, 3, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0) ){
			int choice = do_dialog(player, player, card, -1, -1," Play normally\n Buyback", 1);
			if( choice == 1 ){
				if( sacrifice(player, card, player, 0, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0) ){
					impose_sacrifice(player, card, player, 2, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0);
				}
				else{
					spell_fizzled = 1;
				}
			}
			instance->info_slot = choice;
		}

		instance->number_of_targets = 1;
		instance->targets[0].player = tgt_player;
		instance->targets[0].card = -1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( instance->targets[0].player == player ){
				time_walk_effect(player, card);
			}
			else{
				int fake = add_card_to_hand(instance->targets[0].player, get_original_internal_card_id(player, card));
				time_walk_effect(instance->targets[0].player, fake);
				obliterate_card(instance->targets[0].player, fake);
			}
			if( instance->info_slot == 1 ){
				bounce_permanent(player, card);
			} else {
				kill_card(player, card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_watcher_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 2, 0, BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_weathered_bodyguards(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage
	  && damage->damage_target_player == player
	  && damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER)
	  && is_unblocked(damage->damage_source_player, damage->damage_source_card)
	  && !is_tapped(player, card))
	damage->damage_target_card = card;

  return morph(player, card, event, 3, 0, 0, 0, 0, 1);
}

int card_weatherseed_totem(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 2, 0, 0, 3, 0, 0, 0, 0, 0) ){
			return 1;
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE ){
		int mode = (1<<2);
		if( mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			mode |= (1<<0);
		}
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_TYPE_CHANGE, 2, 0, 0, 3, 0, 0, 0, 0, 0) ){
			mode |=(1<<1);
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		int choice = 0;
		if( ! paying_mana() && (mode & (1<<1)) ){
			char buffer[500];
			int pos = 0;
			int ai_choice = 1;
			if( mode & (1<<0) ){
				pos += scnprintf(buffer + pos, 500-pos, " Get mana\n", buffer);
			}
			if( mode & (1<<1) ){
				pos += scnprintf(buffer + pos, 500-pos, " Animate\n", buffer);
			}
			pos += scnprintf(buffer + pos, 500-pos, " Cancel", buffer);
			choice = do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			while( !( (1<<choice) & mode) ){
					choice++;
			}
		}
		if( choice == 0 ){
			instance->info_slot = 66+choice;
			return mana_producer(player, card, event);
		}
		else if( choice == 1){
				set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
				if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, 2, 0, 0, 3, 0, 0, 0, 0, 0) ){
					instance->info_slot = 66+choice;
				}
				remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( instance->targets[1].player == 66 ){
		card_endless_cockroaches(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *parent = get_card_instance(player, instance->parent_card);
		if( instance->info_slot == 66 ){
			return mana_producer(player, card, event);
		}
		if( instance->info_slot == 67 ){
			add_a_subtype(player, instance->parent_card, SUBTYPE_TREEFOLK);
			int legacy = artifact_animation(player, instance->parent_card, player, instance->parent_card, 1, 5, 3, KEYWORD_TRAMPLE, 0);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[6].player = COLOR_TEST_GREEN;
			parent->targets[1].player = 66;
		}
	}
	if( event == EVENT_CLEANUP && affect_me(player, card) ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_wheel_of_fate(int player, int card, event_t event)
{
  /* Wheel of Fate	""
   * Sorcery
   * Suspend 4-|1|R
   * Each player discards his or her hand, then draws seven cards. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  APNAP(p, { new_discard_all(p, player); });
	  APNAP(p, { draw_cards(p, 7); });
	  kill_card(player, card, KILL_DESTROY);
	}

  return suspend_only(player, card, event, 4, MANACOST_XR(1,1));
}

int card_wipe_away(int player, int card, event_t event){
	/* Wipe Away	|1|U|U
	 * Instant
	 * Split second
	 * Return target permanent to its owner's hand. */

	cannot_be_countered(player, card, event);

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_PERMANENT") ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		  kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_word_of_seizing(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	cannot_be_countered(player, card, event);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( pick_target(&td, "TARGET_PERMANENT") ){
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_wurmcalling(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = x_value;
			set_special_flags2(player, card, SF2_X_SPELL);
			instance->targets[0].player = buyback(player, card, 2, 0, 0, 1, 0, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_WURM, &token);
		token.pow = instance->info_slot;
		token.tou = instance->info_slot;
		generate_token(&token);

		if( instance->targets[0].player == 1 ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_yavimaya_dryad(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		if (basiclandtypes_controlled[1-player][get_hacked_color(player, card, COLOR_GREEN)] > 0){
			td.preferred_controller = player;
		}
		else{
			td.preferred_controller = 1 - player;
		}

		if (can_target(&td) && pick_target(&td, "TARGET_PLAYER")){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select a %s card.", SUBTYPE_FOREST));
			test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
			int selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, (instance->targets[0].player == player ? AI_MAX_VALUE : AI_MIN_VALUE), -1,
											&test);
			if (selected != -1){
				int *deck = deck_ptr[player];
				int card_added = add_card_to_hand(instance->targets[0].player, deck[selected]);
				add_state(instance->targets[0].player, card_added, STATE_TAPPED);
				if (player != instance->targets[0].player){
					get_card_instance(instance->targets[0].player, card_added)->state ^= STATE_OWNED_BY_OPPONENT;
				}
				remove_card_from_deck(player, selected);
				put_into_play(instance->targets[0].player, card_added);
			}
			shuffle(player);
		}
	}

	return 0;
}

