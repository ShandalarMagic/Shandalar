#include "manalink.h"

//--- GENERIC FUNCTIONS ---

static void counters_on_upkeep(int player, int card, event_t event, int counter_type){

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_DUH);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, counter_type);
	}
}

static int generic_growing_enchantment(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_DUH);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_VERSE);
	}
	return global_enchantment(player, card, event);
}

static int urza_multimana_lands(int player, int card, event_t event, int sel_type, int clr){

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_produce_mana(player, card)
			&& (player != AI || count_subtype(player, sel_type, -1) > 0) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		produce_mana_tapped(player, card, clr, count_subtype(player, sel_type, -1));
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && can_produce_mana(player, card) ){
			declare_mana_available(player, clr, count_subtype(player, sel_type, -1));
		}
	}

	return 0;
}

void hidden_enchantment_trigger(int player, int card, event_t event, int type, test_definition_t *this_test, int subt){
	if( ai_is_speculating !=1 && trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		reason_for_trigger_controller == player && trigger_cause_controller == 1-player
	  ){

		int trig = 0;

		if( (type && is_what(trigger_cause_controller, trigger_cause, type)) ||
			(this_test != NULL && new_make_test_in_play(trigger_cause_controller, trigger_cause, -1, this_test))
		  ){
			trig = 1;
		}

		if( trig == 1 ){
			if( event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					add_a_subtype(player, card, subt);
					card_instance_t *instance = get_card_instance(player, card);
					if( get_id(player, card) == CARD_ID_HIDDEN_ANCIENTS ){
						instance->targets[12].card = get_internal_card_id_from_csv_id(CARD_ID_HIDDEN_ANCIENTS_ANIMATED);
					}
					else if( get_id(player, card) == CARD_ID_VEIL_OF_BIRDS ){
							instance->targets[12].card = get_internal_card_id_from_csv_id(CARD_ID_VEIL_OF_BIRDS_ANIMATED);
					}
					else if( get_id(player, card) == CARD_ID_HIDDEN_GIBBONS ){
							instance->targets[12].card = get_internal_card_id_from_csv_id(CARD_ID_HIDDEN_GIBBONS_ANIMATED);
					}
					else if( get_id(player, card) == CARD_ID_OPAL_CHAMPION ){
							instance->targets[12].card = get_internal_card_id_from_csv_id(CARD_ID_OPAL_CHAMPION_ANIMATED);
					}
					else{
						true_transform(player, card);
					}
			}
		}
	}
}

int hidden_enchantment(int player, int card, event_t event, int type, test_definition_t *this_test, int subt){

	double_faced_card(player, card, event);

	hidden_enchantment_trigger(player, card, event, type, this_test, subt);

	return global_enchantment(player, card, event);
}

static int us_paladin(int player, int card, event_t event, int clr){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = 1<<get_sleighted_color(player, card, clr);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_B(2), 0,
									&td, get_sleighted_color_text(player, card, "Select target %s creature", clr));
}

void immortal_enchantment(int player, int card, event_t event)
{
	int owner, position;
	if (check_special_flags(player, card, SF_CORRECTLY_RESOLVED) && ! is_token(player, card) &&
		this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) && find_in_owners_graveyard(player, card, &owner, &position)
	  ){
		const int *grave = get_grave(owner);
		add_card_to_hand(owner, grave[position]);
		remove_card_from_grave(owner, position);
	}
}

int us_cycling(int player, int card, event_t event){
	return cycling(player, card, event, MANACOST_X(2));
}


//--- CARDS ---

int card_abundance(int player, int card, event_t event){

	/* Abundance	|2|G|G
	 * Enchantment
	 * If you would draw a card, you may instead choose land or nonland and reveal cards from the top of your library until you reveal a card of the chosen
	 * kind. Put that card into your hand and put all other cards revealed this way on the bottom of your library in any order. */

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && !suppress_draw &&
		! is_humiliated(player, card)
	  ){
		if( event == EVENT_TRIGGER){
			event_result |= 2u;
		}
		else if( event == EVENT_RESOLVE_TRIGGER ){
				int *deck = deck_ptr[player];
				int choice = do_dialog(player, player, card, -1, -1, " Land\n Non-land\n Pass", 1);
				if( choice < 2 ){
					if( deck[0] != -1 ){
						int revealed = 0;
						int good = 0;
						while( deck[revealed] != -1 ){
								if( choice == 0 && (cards_data[deck[revealed]].type & TYPE_LAND) ){
									good = 1;
									break;
								}
								if( choice == 1 && !(cards_data[deck[revealed]].type & TYPE_LAND) ){
									good = 1;
									break;
								}
								revealed++;
						}
						show_deck( HUMAN, deck, revealed+1, "Cards revealed with Abundance", 0, 0x7375B0 );
						if( good == 1 ){
							add_card_to_hand(player, deck[revealed]);
							remove_card_from_deck(player, revealed);
							revealed--;
						}
						if( revealed > 0 ){
							put_top_x_on_bottom(player, player, revealed+1);
						}
					}
					suppress_draw = 1;
				}
		}
	}

	return global_enchantment(player, card, event);
}

int card_abyssal_horror(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( comes_into_play_mode(player, card, event, can_target(&td) ? RESOLVE_TRIGGER_MANDATORY : 0)){
			if (can_target(&td) && pick_target(&td, "TARGET_PLAYER")){
				new_multidiscard(instance->targets[0].player, 2, 0, player);
			}
		}
	}

	return 0;
}

int card_academy_researchers(int player, int card, event_t event){

	/* Academy Researchers	|1|U|U
	 * Creature - Human Wizard 2/2
	 * When ~ enters the battlefield, you may put an Aura card from your hand onto the battlefield attached to ~. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select an Aura card with enchant creature.");
		this_test.subtype = SUBTYPE_AURA_CREATURE;
		this_test.zone = TARGET_ZONE_HAND;
		if( comes_into_play_mode(player, card, event, check_battlefield_for_special_card(player, card, player, 0, &this_test) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			int result = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
			if( result > -1 ){
				put_into_play_aura_attached_to_target(player, result, player, card);
			}
		}
	}

	return 0;
}

int card_acidic_soil(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {damage_player(p, count_subtype(p, TYPE_LAND, -1), player, card);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_acridian(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XG(1, 1));

	return 0;
}

int card_albino_troll(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XG(1, 1));

	return regeneration(player, card, event, MANACOST_XG(1, 1));
}

int card_angelic_chorus(int player, int card, event_t event){

	/* Angelic Chorus	|3|W|W
	 * Enchantment
	 * Whenever a creature enters the battlefield under your control, you gain life equal to its toughness. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);

		if( new_specific_cip(player, card, event, player, 2, &this_test) ){
			card_instance_t* instance = get_card_instance(player, card);
			gain_life(player, get_toughness(instance->targets[1].player, instance->targets[1].card));
		}
	}

	return global_enchantment(player, card, event);
}

int card_angelic_page(int player, int card, event_t event){
	/*
	  Angelic Page |1|W
	  Creature - Angel Spirit 1/1
	  Flying
	  {T}: Target attacking or blocking creature gets +1/+1 until end of turn.
	*/
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0, &td,
									"Select target attacking or blocking creature.");
}

int card_annul(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}
	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	return counterspell(player, card, event, &td, 0);
}

int card_arc_lightning(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		int total = 0;
		while( total < 3 ){
				if( select_target(player, card, &td, "Select target creature or player.", &(instance->targets[total])) ){
					total++;
				}
				else{
					break;
				}
		}
		if( total != 3 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		divide_damage(player, card, &td);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_arcane_laboratory(int player, int card, event_t event){

	/* Arcane Laboratory	|2|U
	 * Enchantment
	 * Each player can't cast more than one spell each turn. */

	/* Eidolon of Rhetoric	|2|W
	 * Enchantment Creature - Spirit 1/4
	 * Each player can't cast more than one spell each turn. */

	/* Rule of Law	|2|W
	 * Enchantment
	 * Each player can't cast more than one spell each turn. */

	if( event == EVENT_MODIFY_COST_GLOBAL && ! is_humiliated(player, card) ){
		if( ! is_what(affected_card_controller, affected_card, TYPE_LAND) && get_specific_storm_count(affected_card_controller) > 0 ){
			infinite_casting_cost();
		}
	}

	return global_enchantment(player, card, event);
}

int card_argothian_elder(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if (pick_up_to_n_targets(&td, "TARGET_LAND", 2) == 2){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				untap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
	}

	return 0;
}

int card_argothian_enchantress(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		draw_a_card(player);
	}
	return 0;
}

int card_argothian_wurm(int player, int card, event_t event){

	if( comes_into_play(player, card, event) && count_permanents_by_type(1-player, TYPE_LAND) > 0 ){
		if( !IS_AI(1-player) ){
			if( controller_sacrifices_a_permanent(1-player, card, TYPE_LAND, 0) ){
				put_on_top_of_deck(player, card);
			}
		}
		else{
			 int clr = get_deck_color(player, 1-player);
			 if( !(clr & COLOR_TEST_BLACK) && !(clr & COLOR_TEST_BLUE) && !(clr & COLOR_TEST_WHITE) ){
				 controller_sacrifices_a_permanent(1-player, card, TYPE_LAND, SAC_NO_CANCEL);
				 put_on_top_of_deck(player, card);
			 }
		}
	}

	return 0;
}

int card_attunement(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 3);
		multidiscard(player, 4, 0);
	}

	return generic_activated_ability(player, card, event, GAA_BOUNCE_ME, MANACOST0, 0, NULL, NULL);
}

int card_back_to_basics(int player, int card, event_t event){
	if( event == EVENT_UNTAP && current_phase == PHASE_UNTAP && ! is_humiliated(player, card) ){
		card_instance_t *instance= get_card_instance(affected_card_controller, affected_card);
		card_data_t* card_d = &cards_data[ instance->internal_card_id ];
		if( ( card_d->type & TYPE_LAND ) && ! is_basic_land(affected_card_controller, affected_card) ){
			instance->untap_status &= ~3;
		}
	}
	return global_enchantment(player, card, event);
}

int card_barrins_codex(int player, int card, event_t event){

	/* Barrin's Codex	|4
	 * Artifact
	 * At the beginning of your upkeep, you may put a page counter on ~.
	 * |4, |T, Sacrifice ~: Draw X cards, where X is the number of page counters on ~. */

	card_instance_t *instance = get_card_instance( player, card );

	counters_on_upkeep(player, card, event, COUNTER_PAGE);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(4), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = count_counters(player, card, COUNTER_PAGE);
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(4), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, instance->info_slot);
	}

	return 0;
}

int card_barrin_master_wizard(int player, int card, event_t event){
	/*
	  Barrin, Master Wizard |1|U|U
	  Legendary Creature - Human Wizard 1/1
	  {2}, Sacrifice a permanent: Return target creature to its owner's hand.
	*/

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(2), 0, &td, NULL) ){
			return can_sacrifice_type_as_cost(player, 1, TYPE_PERMANENT);
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_PERMANENT, "Select a permanent to sacrifice.");
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac){
				cancel = 1;
				return 0;
			}
			instance->number_of_targets = 0;
			if( pick_target(&td, "TARGET_CREATURE") ){
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_bedlam(int player, int card, event_t event){

	/* Bedlam	|2|R|R
	 * Enchantment
	 * Creatures can't block. */

	if( event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		event_result = 1;
	}
	return global_enchantment(player, card, event);
}

static const char* is_land_or_nonblack_creature(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
	if( is_what(player, card, TYPE_LAND) ){
		return NULL;
	}
	if( is_what(player, card, TYPE_CREATURE) ){
		if( ! (get_color(player, card) & get_sleighted_color_test(targeting_player, targeting_card, COLOR_TEST_BLACK)) ){
			return NULL;
		}
	}
	return get_sleighted_color_text(targeting_player, targeting_card, "must be a land or a non%s creature.", COLOR_BLACK);
}

int card_befoul(int player, int card, event_t event){
	/*
	  Befoul |2|B|B
	  Sorcery
	  Destroy target land or nonblack creature. It can't be regenerated.
	*/

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)is_land_or_nonblack_creature;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						get_sleighted_color_text(player, card, "Slect target land or non%s creature.", COLOR_BLACK), 1, NULL);
}

int card_blanchwood_armor(int player, int card, event_t event)
{
  /* Blanchwood Armor	|2|G
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +1/+1 for each |H2Forest you control. */

  card_instance_t* instance;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && (instance = in_play(player, card))
	  && affect_me(instance->damage_target_player, instance->damage_target_card))
	event_result += count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_FOREST));

  return vanilla_aura(player, card, event, 1-player);
}

int card_blasted_landscape(int player, int card, event_t event)
{
  /* Blasted Landscape	""
   * Land
   * |T: Add |1 to your mana pool.
   * Cycling |2 */

  if (in_play(player, card) && event != EVENT_RESOLVE_ACTIVATION_FROM_HAND)
	return mana_producer(player, card, event);
  else
	return us_cycling(player, card, event);
}

int card_blood_vassal(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return can_sacrifice_this_as_cost(player, card);
	}
	if( event == EVENT_ACTIVATE ){
		produce_mana(player, COLOR_BLACK, 2);
		kill_card(player, card, KILL_SACRIFICE);
	}
	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_sacrifice_this_as_cost(player, card) && can_use_activated_abilities(player, card) ){
		declare_mana_available(player, COLOR_BLACK, 2);
	}

	return 0;
}

int card_brilliant_halo(int player, int card, event_t event){

	immortal_enchantment(player, card, event);

	return generic_aura(player, card, event, player, 1, 2, 0, 0, 0, 0, 0);
}

int card_bulwark(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, hand_count[player]-hand_count[1-player] > 0 ? RESOLVE_TRIGGER_MANDATORY : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		int amount = hand_count[player]-hand_count[1-player];
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		if( amount > 0 && valid_target(&td) ){
			damage_player(1-player, amount, player, card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_catalog(int player, int card, event_t event){
	/*
	  Catalog |2|U
	  Instant
	  Draw two cards, then discard a card.
	*/

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		discard(player, 0, player);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_cathodion(int player, int card, event_t event){
	/*
	  Cathodion |3
	  Artifact Creature - Construct 3/3
	  When Cathodion dies, add {3} to your mana pool.
	*/

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		produce_mana(player, COLOR_COLORLESS, 3);
	}

	return 0;
}

int ability_decompose(int player, int card, event_t event);
int card_carrion_beetles(int player, int card, event_t event)
{
  /* Carrion Beetles	|B
   * Creature - Insect 1/1
   * |2|B, |T: Exile up to three target cards from a single graveyard. */

  if (event == EVENT_CAN_ACTIVATE)
	return ability_decompose(player, card, event) && CAN_ACTIVATE(player, card, MANACOST_XB(2,1)) && CAN_TAP(player, card);

  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST_XB(2,1)))
	{
	  ability_decompose(player, card, event);
	  tap_card(player, card);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	ability_decompose(player, card, event);

  return 0;
}

int card_catastrophe(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		int ai_choice = 0;
		if( count_subtype(player, TYPE_CREATURE, -1) > count_subtype(1-player, TYPE_CREATURE, -1) ){
			ai_choice = 1;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Kill all creatures\n Kill all lands", ai_choice);
		if( choice == 1 ){
			this_test.type = TYPE_LAND;
		}
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);

		kill_card(player, card, KILL_DESTROY);

	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_child_of_gaea(int player, int card, event_t event){

	basic_upkeep(player, card, event, MANACOST_G(2));

	return regeneration(player, card, event, MANACOST_XG(1, 1));
}

int card_chimeric_staff(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		artifact_animation(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1,
							instance->info_slot, instance->info_slot, 0, 0);
	}

	return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_X(-1), 0, NULL, NULL);
}

int card_citanul_flute(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100];
		scnprintf(msg, 100, " Select a creature card with CMC %d or less", instance->info_slot);
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.cmc = instance->info_slot+1;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, MANACOST_X(-1), 0, 0, 0);
}

int card_citanul_hierophants(int player, int card, event_t event){
	return permanents_you_control_can_tap_for_mana(player, card, event, TYPE_CREATURE, -1, COLOR_GREEN, 1);
}

int card_claws_of_gix(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_X(1), 0, NULL, NULL) ){
			return can_sacrifice_type_as_cost(player, 1, TYPE_PERMANENT);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
			if( ! controller_sacrifices_a_permanent(player, card, TYPE_PERMANENT, 0) ){
				spell_fizzled = 1;;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 1);
	}

	return 0;
}

int card_confiscate(int player, int card, event_t event){
	/*
	  Confiscate |4|U|U
	  Enchantment - Aura
	  Enchant permanent (Target a permanent as you cast this. This card enters the battlefield attached to that permanent.)
	  You control enchanted permanent.
	*/
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	return generic_stealing_aura(player, card, event, &td, "TARGET_PERMANENT");
}

int card_congregate(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.preferred_controller = player;
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life(instance->targets[0].player, count_subtype(2, TYPE_CREATURE, -1)*2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_contamination(int player, int card, event_t event){

	/* Contamination	|2|B
	 * Enchantment
	 * At the beginning of your upkeep, sacrifice ~ unless you sacrifice a creature.
	 * If a land is tapped for mana, it produces |B instead of any other type and amount. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if (!can_sacrifice_as_cost(player, 1, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0)
			|| !controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0)
		   ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		set_special_flags2(player, -1, SF2_CONTAMINATION);
	}

	if( event == EVENT_CAST_SPELL && is_what(affected_card_controller, affected_card, TYPE_LAND) ){
		set_special_flags2(affected_card_controller, affected_card, SF2_CONTAMINATION);
	}

	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		this_test.id = get_id(player, card);
		this_test.not_me = 1;
		if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			remove_special_flags2(player, -1, SF2_CONTAMINATION);
		}
	}

	return global_enchantment(player, card, event);
}

int card_copper_gnomes(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a artifact card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(4), 0, NULL, NULL);
}

int card_corrupt2(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = count_subtype(player, TYPE_LAND, SUBTYPE_SWAMP);
			damage_creature_or_player(player, card, event, amount);
			gain_life(player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_cradle_guard(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XG(1, 2));

	return 0;
}

int card_crater_hellion(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XR(4, 2));

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		APNAP(p, {new_damage_all(player, card, p, 4, 0, &this_test);};);
	}

	return 0;
}

int card_crosswinds(int player, int card, event_t event){
	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING)
		  ){
			event_result-=2;
		}
	}
	return global_enchantment(player, card, event);
}

int card_crystal_chimes(int player, int card, event_t event){

	/* Crystal Chimes	|3
	 * Artifact
	 * |3, |T, Sacrifice ~: Return all enchantment cards from your graveyard to your hand. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ENCHANTMENT, "");
		from_grave_to_hand_multiple(player, &test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_X(3), 0, NULL, NULL);
}

int card_curfew(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		APNAP(p,{
					td.allowed_controller = p;
					td.preferred_controller = p;
					td.illegal_abilities = 0;
					td.who_chooses = p;
					instance->number_of_targets = 0;
					if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
						bounce_permanent(instance->targets[0].player, instance->targets[0].card);
					}
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_dark_hatchling(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		td1.allow_cancel = 0;

		if (can_target(&td1) && new_pick_target(&td1, get_sleighted_color_text(player, card, "Select target non%s creature.", COLOR_BLACK), 0, GS_LITERAL_PROMPT)){
			card_instance_t* instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_darkest_hour(int player, int card, event_t event)
{
  if (event == EVENT_SET_COLOR && is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_humiliated(player, card)
	  && in_play(affected_card_controller, affected_card) && in_play(player, card))
	event_result = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

  return global_enchantment(player, card, event);
}

int card_despondency(int player, int card, event_t event){

	immortal_enchantment(player, card, event);

	return generic_aura(player, card, event, 1-player, -2, 0, 0, 0, 0, 0, 0);
}

int card_destructive_urge(int player, int card, event_t event)
{
  /* Destructive Urge	|1|R|R
   * Enchantment - Aura
   * Enchant creature
   * Whenever enchanted creature deals combat damage to a player, that player sacrifices a land. */

  if (attached_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRACE_DAMAGED_PLAYERS))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
	  instance->targets[1].player = 0;
	  APNAP(p, impose_sacrifice(player, card, p, times_damaged[p], TYPE_LAND,MATCH, 0,0, 0,0, 0,0, -1,0););
	}

  return vanilla_aura(player, card, event, player);
}

static int diabolic_servitude_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(instance->targets[0].player, instance->targets[0].card) &&
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->kill_code > 0
		  ){
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->kill_code = KILL_REMOVE;
			instance->damage_target_player = instance->damage_target_card = -1;
			instance->targets[11].player = 66;
		}

		if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
			bounce_permanent(instance->targets[1].player, instance->targets[1].card);
			kill_card(player, card, KILL_REMOVE);
		}

		if( leaves_play(instance->targets[1].player, instance->targets[1].card, event) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_diabolic_servitude(int player, int card, event_t event){
	/*
	  Diabolic Servitude |3|B
	  Enchantment
	  When Diabolic Servitude enters the battlefield, return target creature card from your graveyard to the battlefield.
	  When the creature put onto the battlefield with Diabolic Servitude dies, exile it and return Diabolic Servitude to its owner's hand.
	  When Diabolic Servitude leaves the battlefield, exile the creature put onto the battlefield with Diabolic Servitude.
	*/
	if( comes_into_play(player, card, event) ){
		char buffer[100] = "Select a creature card";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, buffer);

		if( count_graveyard_by_type(player, TYPE_CREATURE) && ! graveyard_has_shroud(player) ){
			int reanimated = new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
			if( reanimated > -1 ){
				int legacy = create_targetted_legacy_effect(player, card, &diabolic_servitude_effect, player, reanimated);
				card_instance_t *instance = get_card_instance(player, legacy);
				instance->targets[1].player = player;
				instance->targets[1].card = card;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_discordant_dirge(int player, int card, event_t event){

	/* Discordant Dirge	|3|B|B
	 * Enchantment
	 * At the beginning of your upkeep, you may put a verse counter on ~.
	 * |B, Sacrifice ~: Look at target opponent's hand and choose up to X cards from it, where X is the number of verse counters on ~. That player discards
	 * those cards. */

	if( ! IS_GAA_EVENT(event) ){
		return generic_growing_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);


	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_B(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, MANACOST_B(1));
		if( spell_fizzled != 1 ){
			instance->info_slot = count_counters(player, card, COUNTER_VERSE);
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			ec_definition_t ec;
			default_ec_definition(instance->targets[0].player, player, &ec);
			ec.qty = instance->info_slot;

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);
			new_effect_coercion(&ec, &this_test);
		}
	}

	return 0;
}

int card_disruptive_student(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, &td, 0, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SPELL_ON_STACK, MANACOST0, 0, &td, NULL);
}

int card_dragons_blood(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_CREATURE");
}

int card_drifting_djinn(int player, int card, event_t event){

	basic_upkeep(player, card, event, MANACOST_XU(1, 1));

	return us_cycling(player, card, event);
}

int card_drifting_meadow(int player, int card, event_t event)
{
  /* Drifting Meadow	""
   * Land
   * ~ enters the battlefield tapped.
   * |T: Add |W to your mana pool.
   * Cycling |2 */

  // Also Polluted Mire, Remote Isle, Slippery Karst, Smoldering Crater

  comes_into_play_tapped(player, card, event);

  if (in_play(player, card) && event != EVENT_RESOLVE_ACTIVATION_FROM_HAND)
	return mana_producer(player, card, event);
  else
	return us_cycling(player, card, event);
}

int card_duress(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			ec_definition_t ec;
			default_ec_definition(instance->targets[0].player, player, &ec);

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_LAND, "Select a nonland, noncreature card.");
			this_test.type_flag = DOESNT_MATCH;
			new_effect_coercion(&ec, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_eastern_paladin(int player, int card, event_t event){
	/*
	  Eastern Paladin |2|B|B
	  Creature - Zombie Knight 3/3
	  {B}{B}, {T}: Destroy target green creature.
	*/
  return us_paladin(player, card, event, COLOR_GREEN);
}

int card_elite_archers(int player, int card, event_t event){
	/*
	  Elite Archers |5|W
	  Creature - Human Soldier Archer 3/3
	  {T}: Elite Archers deals 3 damage to target attacking or blocking creature.
	*/
	if( !IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		damage_target0(player, card, 3);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0 | GAA_LITERAL_PROMPT, 0,
									&td, "Select target attacking or blocking creature.");
}

int card_elvish_herder(int player, int card, event_t event){

	if( !IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_TRAMPLE, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_G(1), 0, &td, "TARGET_CREATURE");
}

int card_elvish_lyrist(int player, int card, event_t event){

	if( !IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_G(1), 0, &td, "TARGET_ENCHANTMENT");
}

int card_endless_wurm(int player, int card, event_t event)
{
  /* Endless Wurm	|3|G|G
   * Creature - Wurm 9/9
   * Trample
   * At the beginning of your upkeep, sacrifice ~ unless you sacrifice an enchantment. */

  upkeep_trigger_ability(player, card, event, player);

	if (event == EVENT_UPKEEP_TRIGGER_ABILITY && !controller_sacrifices_a_permanent(player, card, TYPE_ENCHANTMENT, 0)){
		kill_card(player, card, KILL_SACRIFICE);
		return -1;
	}

	return 0;
}

int card_energy_field(int player, int card, event_t event)
{
  /* Energy Field	|1|U
   * Enchantment
   * Prevent all damage that would be dealt to you by sources you don't control.
   * When a card is put into your graveyard from anywhere, sacrifice ~. */

  if (when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, NULL))
	kill_card(player, card, KILL_SACRIFICE);

  enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
  if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == player && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		kill_card(player, card, KILL_SACRIFICE);
	}

  card_instance_t* damage = damage_being_prevented(event);
  if (damage
	  && damage->damage_source_player == 1-player
	  && damage->damage_target_card == -1 && damage->damage_target_player == player && !damage_is_to_planeswalker(damage))
	damage->info_slot = 0;

  return global_enchantment(player, card, event);
}

static int effect_exhaustion(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && affected_card_controller == 1-player ){
		if( is_what(affected_card_controller, affected_card, TYPE_LAND) ||
			is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		  ){
			card_instance_t *this= get_card_instance(affected_card_controller, affected_card);
			this->untap_status &= ~3;
		}
	}

	if( eot_trigger(player, card, event) ){
		instance->targets[1].card--;
		if( instance->targets[1].card < 1 ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_exhaustion(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_effect(player, card, &effect_exhaustion);
		card_instance_t *instance = get_card_instance(player, legacy);
		if( current_turn == player ){
			instance->targets[1].card = 2;
		}
		else{
			 instance->targets[1].card = 3;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_exhume(int player, int card, event_t event){
	if(event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
					if( count_graveyard_by_type(p, TYPE_CREATURE) > 0 ){
						char buffer[100];
						scnprintf(buffer, 100, "Select a creature card.");
						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
						new_global_tutor(p, p, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
					}
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_exploration(int player, int card, event_t event){
	check_playable_lands(player);
	return global_enchantment(player, card, event);
}

int card_expunge(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.illegal_type = TYPE_ARTIFACT;
		td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

		if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			}
			kill_card(player, card, KILL_DESTROY);
		}

		return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
							get_sleighted_color_text(player, card, "Select target non%s, nonartifact creature.", COLOR_BLACK), 1, NULL);
	}

	return us_cycling(player, card, event);
}

int card_faith_healer(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_type_as_cost(player, 1, TYPE_ENCHANTMENT);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int sac = controller_sacrifices_a_permanent(player, card, TYPE_ENCHANTMENT, SAC_RETURN_CHOICE);
			if (sac){
				instance->info_slot = get_cmc(BYTE2(sac), BYTE3(sac));
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, instance->info_slot);
	}

	return 0;
}

int card_falter(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		this_test.keyword_flag = DOESNT_MATCH;
		creatures_cannot_block(player, card, &this_test, 1);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_fecundity(int player, int card, event_t event){
	/*
	  Fecundity |2|G
	  Enchantment
	  Whenever a creature dies, that creature's controller may draw a card.
	*/
	if( ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance( player, card);
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
			if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
				card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
				if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE ){
					if( instance->targets[affected_card_controller].player < 0 ){
						instance->targets[affected_card_controller].player = 0;
					}
					instance->targets[affected_card_controller].player++;
				}
			}
		}

		if( (instance->targets[0].player || instance->targets[1].player) && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY &&
			player == reason_for_trigger_controller && affect_me(player, card )
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int i;
					for(i=0; i<2; i++){
						if( instance->targets[i].player > 0 ){
							draw_some_cards_if_you_want(player, card, i, instance->targets[i].player);
							instance->targets[i].player = 0;
						}
					}
			}
		}
	}

	return global_enchantment(player, card, event);
}

// Fault line code is the same of Earthquake

int card_fiery_mantle(int player, int card, event_t event){

	immortal_enchantment(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;
		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
		}

		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_ability_until_eot(player, instance->parent_card, t_player, t_card, 1, 0, 0, 0);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_fire_ants(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		this_test.keyword_flag = 1;
		this_test.not_me = 1;
		new_damage_all(instance->parent_controller, instance->parent_card, 2, 1, 0, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_flesh_reaver(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if (event == EVENT_DEAL_DAMAGE){
			card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
			if (damage->internal_card_id == damage_card && damage->damage_source_card == card && damage->damage_source_player == player ){
				int good = 1;
				if( damage->damage_target_player == player ){
					good = 0;
				}
				if( damage->damage_target_card == -1 ){
					if( damage->targets[4].player > -1 || damage->targets[4].card > -1 ){
						good = 0;
					}
				}
				if( good ){
					int amount = damage->info_slot;
					if( amount < 1 ){
						amount = instance->targets[16].player;
					}
					if( amount > 0 ){
						if( instance->targets[1].player < 0 ){
							instance->targets[1].player = 0;
						}
						instance->targets[1].player += amount;
					}
				}
			}
		}
		if( instance->targets[1].player > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == player
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					damage_player(player, instance->targets[1].player, player, card);
					instance->targets[1].player = 0;
			}
			else if (event == EVENT_END_TRIGGER){
					instance->targets[1].player = 0;
			}
		}
	}

	return 0;
}

// There is no Fluctuator code. The card is only checked by "cycling" and "landcycling" functions in "functions.c".


int card_fog_bank(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage
	  && ((damage->damage_target_card == card && damage->damage_target_player == player)
		  || (damage->damage_source_card == card && damage->damage_source_player == player)))
	damage->info_slot = 0;

  return 0;
}

int card_gaeas_bounty(int player, int card, event_t event){

	/* Gaea's Bounty	|2|G
	 * Sorcery
	 * Search your library for up to two |H2Forest cards, reveal those cards, and put them into your hand. Then shuffle your library. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card.", SUBTYPE_FOREST));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		this_test.qty = 2;

		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		shuffle(player);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_gaeas_cradle(int player, int card, event_t event){
	// original code : 0x4E56D7
	return urza_multimana_lands(player, card, event, TYPE_CREATURE, COLOR_GREEN);
}

int card_gaeas_embrace(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;
		if( event == EVENT_CAN_ACTIVATE ){
			if( generic_activated_ability(t_player, t_card, event, GAA_REGENERATION, MANACOST_G(1), 0, NULL, NULL) ){
				if( can_regenerate(t_player, t_card) ){
					return 99;
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(t_player, t_card, event, GAA_REGENERATION, MANACOST_G(1), 0, NULL, NULL);
		}

		if( event == EVENT_RESOLVE_ACTIVATION && can_regenerate(t_player, t_card) ){
			regenerate_target(t_player, t_card);
		}
	}

	return generic_aura(player, card, event, player, 3, 3, KEYWORD_TRAMPLE, 0, 0, 0, 0);
}

int card_gamble(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		char msg[100] = "Select a card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		discard(player, DISC_RANDOM, player);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_gilded_drake(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;

		card_instance_t *instance = get_card_instance(player, card);

		int kill = 1;

		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				exchange_control_of_target_permanents(player, card, player, card, instance->targets[0].player, instance->targets[0].card);
				kill = 0;
			}
		}

		if( kill == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}

	}

	return 0;
}

int card_goblin_lackey (int player, int card, event_t event){
	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		char buffer[100];
		scnprintf(buffer, 100, "Select a Goblin card");

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, buffer);
		this_test.subtype = SUBTYPE_GOBLIN;
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	return 0;
}

int card_goblin_matron( int player, int card, event_t event){
	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a Goblin card.");
		this_test.subtype = SUBTYPE_GOBLIN;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}
	return 0;
}

int card_goblin_offensive(int player, int card, event_t event){
	/* Goblin Offensive	|X|1|R|R
	 * Sorcery
	 * Put X 1/1 |Sred Goblin creature tokens onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 ){
			instance->info_slot = x_value;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, instance->info_slot);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_goblin_patrol(int player, int card, event_t event){

	echo(player, card, event, MANACOST_R(1));

	return 0;
}

int card_goblin_war_buggy(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XR(1, 1));

	haste(player, card, event);

	return 0;
}

int card_grafted_skullcap(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		if(event == EVENT_DRAW_PHASE && current_turn == player){
			event_result++;
		}
		if( current_turn == player && eot_trigger(player, card, event) ){
			discard_all(player);
		}
	}
	return 0;
}

int card_great_whale(int player, int card, event_t event){
	if( comes_into_play(player, card, event) > 0 ){
		untap_lands(player, card, 7);
	}
	return 0;
}

int card_greater_good(int player, int card, event_t event){
	/*
	  Greater Good |2|G|G
	  Enchantment
	  Sacrifice a creature: Draw cards equal to the sacrificed creature's power, then discard three cards.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST0, 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (!sac){
			cancel = 1;
			return 0;
		}
		instance->info_slot = get_power(BYTE2(sac), BYTE3(sac));
		kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, instance->info_slot);
		multidiscard(player, 3, 0);
	}

	return global_enchantment(player, card, event);
 }

int card_greener_pastures(int player, int card, event_t event){
	/* Greener Pastures	|2|G
	 * Enchantment
	 * At the beginning of each player's upkeep, if that player controls more lands than each other player, the player puts a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( count_subtype(current_turn, TYPE_LAND, -1) > count_subtype(1-current_turn, TYPE_LAND, -1) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SAPROLING, &token);
			token.t_player = current_turn;
			generate_token(&token);
		}
	}

	return global_enchantment(player, card, event);
}

int card_herald_of_serra(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XW(2, 2));

	vigilance(player, card, event);

	return 0;
}

int card_hermetic_study(int player, int card, event_t event){

	card_instance_t* instance;
	if( (instance = in_play(player, card)) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		if( IS_GAA_EVENT(event) ){
			target_definition_t td1;
			default_target_definition(t_player, t_card, &td1, TYPE_CREATURE);
			td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

			if( event == EVENT_CAN_ACTIVATE ){
				return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td1, "TARGET_CREATURE_OR_PLAYER");
			}

			if( event == EVENT_ACTIVATE ){
				return generic_activated_ability(t_player, t_card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, MANACOST0, 0, &td1, "TARGET_CREATURE_OR_PLAYER");
			}

			if( event == EVENT_RESOLVE_ACTIVATION ){
				if( validate_target(t_player, t_card, &td1, 0) ){
					damage_target0(t_player, t_card, 1);
				}
			}
		}
	}

	return vanilla_aura(player, card, event, player);
}

int card_hibernation2(int player, int card, event_t event){
	/*
	  Hibernation |2|U
	  Instant
	  Return all green permanents to their owners' hands.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
		new_manipulate_all(player, card, ANYBODY, &this_test, ACT_BOUNCE);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

// all "hidden" animated enchantments --> vanilla

int card_hidden_ancients(int player, int card, event_t event){
	return hidden_enchantment(player, card, event, TYPE_ENCHANTMENT, NULL, SUBTYPE_TREEFOLK);
}

int card_hidden_guerrillas(int player, int card, event_t event){
	return hidden_enchantment(player, card, event, TYPE_ARTIFACT, NULL, SUBTYPE_SOLDIER);
}

int card_hidden_herd(int player, int card, event_t event){
	double_faced_card(player, card, event);
	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.subtype = SUBTYPE_BASIC;
		this_test.subtype_flag = DOESNT_MATCH;
		hidden_enchantment_trigger(player, card, event, 0, &this_test, SUBTYPE_BEAST );
	}
	return global_enchantment(player, card, event);
}

int card_hidden_predators(int player, int card, event_t event){
	double_faced_card(player, card, event);
	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.power = 3;
		this_test.power_flag = 2;
		hidden_enchantment_trigger(player, card, event, 0, &this_test, SUBTYPE_BEAST );
	}
	return global_enchantment(player, card, event);
}

int card_hidden_spider(int player, int card, event_t event){
	double_faced_card(player, card, event);
	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		hidden_enchantment_trigger(player, card, event, 0, &this_test, SUBTYPE_SPIDER);
	}
	return global_enchantment(player, card, event);
}

int card_hidden_stag(int player, int card, event_t event){
	return hidden_enchantment(player, card, event, TYPE_LAND, NULL, SUBTYPE_BEAST);
}

int card_hidden_stag_animated(int player, int card, event_t event, test_definition_t *this_test){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) &&
		reason_for_trigger_controller == player && trigger_cause_controller == player ){

		int trig = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			trig = 1;
		}

		if( trig == 1 ){
			if( event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					true_transform(player, card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_horseshoe_crab( int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION ){
		untap_card(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_U(1), 0, NULL, NULL);
}

int card_humble(int player, int card, event_t event){
	/*
	  Humble |1|W (same code as Ovinize)
	  Instant
	  Target creature loses all abilities and becomes 0/1 until end of turn.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t hc;
			default_test_definition(&hc, 0);
			hc.power = 0;
			hc.toughness = 1;
			humiliate_and_set_pt_abilities(player, card, instance->targets[0].player, instance->targets[0].card, 4, &hc);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_hush(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return us_cycling(player, card, event);
}

int card_ill_gotten_gains(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
					discard_all(p);
					int amount = MIN(3, count_graveyard(p));

					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_ANY, "Select a card to return to your hand.");
					int z = 0;
					while( z < amount ){
							if( new_global_tutor(p, p, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test) != -1 ){
								z++;
							}
							else{
								break;
							}
					}
				};
		);
		kill_card(player, card, KILL_REMOVE);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_imaginary_pet(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, hand_count[player] > 0 ? RESOLVE_TRIGGER_MANDATORY : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		bounce_permanent(player, card);
	}

	return 0;
}

int card_intrepid_hero(int player, int card, event_t event){

	if( !IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target creature with Power 4 or more.");
}

int card_karn_silver_golem(int player, int card, event_t event){

	if( ! is_unlocked(player, card, event, 20) ){ return 0; }

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_DECLARE_BLOCKERS ){
	   if( (instance->state & STATE_ATTACKING) && ! is_unblocked(player, card) ){
		  pump_until_eot(player, card, player, card, -4, 4);
	   }
	   else if( current_turn != player && blocking(player, card, event) ){
			   pump_until_eot(player, card, player, card, -4, 4);
	   }
	}

	if( !IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = player;
	td.illegal_type = TYPE_CREATURE;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int pt = get_cmc(instance->targets[0].player, instance->targets[0].card);
			artifact_animation(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, pt, pt, 0, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_ARTIFACT");
}

int card_launch(int player, int card, event_t event){

	immortal_enchantment(player, card, event);

	return generic_aura(player, card, event, player, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_lay_waste(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND );

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			}
			kill_card(player, card, KILL_DESTROY);
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
	}

	return us_cycling(player, card, event);
}

static int lifeline_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( eot_trigger(player, card, event) ){
		seek_grave_for_id_to_reanimate(instance->targets[0].player, -1, instance->targets[0].player, instance->targets[0].card, REANIMATE_DEFAULT);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_lifeline(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( is_humiliated(player, card) ){
		return 0;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( in_play(affected_card_controller, affected_card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.type_flag = F1_NO_TOKEN;
			if( new_make_test_in_play(affected_card_controller, affected_card, -1, &this_test) ){
				card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
				if( affected->kill_code > 0 && affected->kill_code < 4 ){
					if( instance->targets[11].player < 0 ){
						instance->targets[11].player = 0;
					}
					int pos = instance->targets[11].player;
					if( pos < 11 ){
						instance->targets[pos].player = get_owner(affected_card_controller, affected_card);
						instance->targets[pos].card = cards_data[get_original_internal_card_id(affected_card_controller, affected_card)].id;
						instance->targets[11].player++;
					}
				}
			}
		}
	}

	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
		affect_me(player, card )
	  ){
		if( count_subtype(2, TYPE_CREATURE, -1) > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int i;
					for(i=0; i<instance->targets[11].player; i++){
						int legacy = create_legacy_effect(player, card, &lifeline_legacy);
						card_instance_t *leg = get_card_instance(player, legacy);
						leg->targets[0] = instance->targets[i];
					}
					instance->targets[11].player = 0;
			}
		}
	}

	return 0;
}

int card_lightning_dragon(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XR(2, 2));

	return generic_shade(player, card, event, 0, MANACOST_R(1), 1, 0, 0, 0);
}

int card_lilting_refrain(int player, int card, event_t event){

	/* Lilting Refrain	|1|U
	 * Enchantment
	 * At the beginning of your upkeep, you may put a verse counter on ~.
	 * Sacrifice ~: Counter target spell unless its controller pays |X, where X is the number of verse counters on ~. */

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_VERSE);
	}

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);

	if(event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_SPELL_ON_STACK, MANACOST0, 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = count_counters(player, card, COUNTER_VERSE);
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_SPELL_ON_STACK, MANACOST0, 0, &td, NULL);
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, &td, 0, instance->info_slot);
	}

	return 0;
 }


int card_lotus_blossom(int player, int card, event_t event){

	/* Lotus Blossom	|2
	 * Artifact
	 * At the beginning of your upkeep, you may put a petal counter on ~.
	 * |T, Sacrifice ~: Add X mana of any one color to your mana pool, where X is the number of petal counters on ~. */

	counters_on_upkeep(player, card, event, COUNTER_PETAL);

	int counters = count_counters(player, card, COUNTER_PETAL);

	if (event == EVENT_CAN_ACTIVATE && IS_AI(player) && counters == 0){
		return 0;
	}

	return artifact_mana_all_one_color(player, card, event, counters, 1);
}

int card_lull(int player, int card, event_t event){
	if( event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND || event == EVENT_RESOLVE_ACTIVATION_FROM_HAND ){
		return us_cycling(player, card, event);
	}
	return card_fog(player, card, event);
}

int card_lurking_evil(int player, int card, event_t event){

   card_instance_t *instance = get_card_instance(player, card);

   double_faced_card(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST0, 1, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			lose_life(player, round_up_value(life[player]));
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		true_transform(instance->parent_controller, instance->parent_card);
	}

   return global_enchantment(player, card, event);
}

int card_meltdown(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		this_test.cmc = instance->info_slot+1;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_metrognome(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GNOME, &token);
		token.pow = token.tou = 1;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL);
}

int card_midsummer_revel(int player, int card, event_t event){

	/* Midsummer Revel	|3|G|G
	 * Enchantment
	 * At the beginning of your upkeep, you may put a verse counter on ~.
	 * |G, Sacrifice ~: Put X 3/3 |Sgreen Beast creature tokens onto the battlefield, where X is the number of verse counters on ~. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_G(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_G(1)) ){
			instance->info_slot = count_counters(player, card, COUNTER_VERSE);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_tokens_by_id(player, card, CARD_ID_BEAST, instance->info_slot);
	}

	return generic_growing_enchantment(player, card, event);
}


int card_mishras_helix(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE  ){
		instance->number_of_targets = 0;
		int trgs = 0;
		while( can_target(&td) && has_mana_for_activated_ability(player, card, MANACOST_X(trgs+1)) ){
				if( new_pick_target(&td, "TARGET_LAND", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		int i;
		for(i=0; i<trgs; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		}
		if( trgs > 0 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(trgs)) ){
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
	}

	return 0;
}

int card_monk_idealist(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		char msg[100] = "Select an enchantment card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT, msg);
		if( new_special_count_grave(player, &this_test) > 0 && ! graveyard_has_shroud(2) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

int card_monk_realist(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_ENCHANTMENT")){
			card_instance_t* instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_morphling(int player, int card, event_t event)
{
  // |U: Untap ~.
  // |U: ~ gains flying until end of turn.
  // |U: ~ gains shroud until end of turn.
  // |1: ~ gets +1/-1 until end of turn.
  // |1: ~ gets -1/+1 until end of turn.

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  card_instance_t* instance = get_card_instance(player, card);

	  if (event == EVENT_ACTIVATE)
		load_text(0, "MORPHLING");

	  if (stack_size == 0)
		instance->targets[4].player = 0;

	  int ai_priority_untap = (!is_tapped(player, card) ? -1
							   : instance->targets[4].player & 1 ? 1
							   : 10);
	  int ai_priority_flying = (check_for_ability(player, card, KEYWORD_FLYING) ? -1
								: instance->targets[4].player & 2 ? 1
								: 10);
	  int ai_priority_shroud = (check_for_ability(player, card, KEYWORD_SHROUD) ? -1
								: instance->targets[4].player & 4 ? 5	// It *could* make sense to gain shroud while a gain-shroud activation is on the stack
								: 10);

	  enum
	  {
		CHOICE_UNTAP = 1,
		CHOICE_FLYING,
		CHOICE_SHROUD,
		CHOICE_POWER,
		CHOICE_TOUGHNESS
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						text_lines[4]/*untap*/,	1, ai_priority_untap,	DLG_MANA(MANACOST_U(1)),
						text_lines[3]/*flying*/,1, ai_priority_flying,	DLG_MANA(MANACOST_U(1)),
						text_lines[2]/*shroud*/,1, ai_priority_shroud,	DLG_MANA(MANACOST_U(1)),
						text_lines[0]/*+1/-1*/,	1, 5,					DLG_MANA(MANACOST_X(1)),
						text_lines[1]/*-1/+1*/,	1, 5,					DLG_MANA(MANACOST_X(1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_UNTAP:
			  instance->targets[4].player |= 1;	// Discourage reactivation while an untap activation is on the stack
			  break;

			case CHOICE_FLYING:
			  instance->targets[4].player |= 2;	// Discourage reactivation while a flying activation is on the stack
			  break;

			case CHOICE_SHROUD:
			  instance->targets[4].player |= 4;	// Discourage reactivation while a shroud activation is on the stack
			  break;

			default:break;
		  }
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  player = instance->parent_controller;
		  card = instance->parent_card;
		  if (!in_play(player, card))
			return 0;

		  switch (choice)
			{
			  case CHOICE_UNTAP:
				untap_card(player, card);
				break;

			  case CHOICE_FLYING:
				alternate_legacy_text(1, player, pump_ability_until_eot(player, card, player, card, 0,0, KEYWORD_FLYING,0));
				break;

			  case CHOICE_SHROUD:
				alternate_legacy_text(2, player, pump_ability_until_eot(player, card, player, card, 0,0, KEYWORD_SHROUD,0));
				break;

			  case CHOICE_POWER:
				alternate_legacy_text(3, player, pump_until_eot_merge_previous(player, card, player, card, 1,-1));
				break;

			  case CHOICE_TOUGHNESS:
				// Can't merge for -x/+x
				alternate_legacy_text(4, player, pump_until_eot(player, card, player, card, -1,1));
				break;
			}
		}
	}

  if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
	return has_mana(player, COLOR_ANY, 1);

  return 0;
}

int card_no_rest_for_the_wicked(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( ! get_dead_count(player, TYPE_CREATURE | GDC_NONTOKEN) ){
			ai_modifier-=30;
		}
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		return_all_dead_this_turn_to_hand(player, TYPE_CREATURE | GDC_NONTOKEN);
	}

	return global_enchantment(player, card, event);
}

int card_noetic_scales(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int count = active_cards_count[current_turn]-1;
		int amount = hand_count[current_turn];
		while( count > -1 ){
			   if( in_play(current_turn, count) && is_what(current_turn, count, TYPE_CREATURE) &&
				   get_power(current_turn, count) > amount
				 ){
				   bounce_permanent(current_turn, count);
			   }
			   count--;
		}
	}

	return 0;
}

int card_okk(int player, int card, event_t event){
	/*
	  Okk |1|R
	  Creature - Goblin 4/4
	  Okk can't attack unless a creature with greater power also attacks.
	  Okk can't block unless a creature with greater power also blocks.
	*/
	if( ! is_humiliated(player, card) ){
		if (event == EVENT_ATTACK_LEGALITY && affect_me(player, card)){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.illegal_abilities = 0;
			td.required_state = TARGET_STATE_ATTACKING;
			td.power_requirement = (get_power(player, card)+1) | TARGET_PT_GREATER_OR_EQUAL;

			if (!can_target(&td))
				event_result = 1;
		}
		if (event == EVENT_BLOCK_LEGALITY && affect_me(player, card)){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.illegal_abilities = 0;
			td.required_state = TARGET_STATE_BLOCKING;
			td.power_requirement = (get_power(player, card)+1) | TARGET_PT_GREATER_OR_EQUAL;

			if (!can_target(&td))
				event_result = 1;
		}
	}
	return 0;
}

int card_opal_acrolith(int player, int card, event_t event){
	return hidden_enchantment(player, card, event, TYPE_CREATURE, NULL, SUBTYPE_SOLDIER);
}

int card_opal_acrolith_animated(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		true_transform(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST0, 0, NULL, NULL);
}

int card_opal_archangel(int player, int card, event_t event){
	return hidden_enchantment(player, card, event, TYPE_CREATURE, NULL, SUBTYPE_ANGEL);
}

int card_opal_gargoyle(int player, int card, event_t event){
	return hidden_enchantment(player, card, event, TYPE_CREATURE, NULL, SUBTYPE_GARGOYLE);
}

int card_opal_titan(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && ! is_humiliated(player, card) &&
		reason_for_trigger_controller == player && trigger_cause_controller == 1-player ){

		int trig = 0;
		int clr = 0;

		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
			trig = 1;
			clr = get_color(trigger_cause_controller, trigger_cause);
		}


		if( trig == 1 ){
			if( event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					instance->targets[1].player = 0;
					int i;
					for(i=1; i<6; i++){
						if( (clr & (1<<i)) ){
							instance->targets[1].player |= 1<<(10+i);
						}
					}
					add_a_subtype(player, card, SUBTYPE_GIANT);
					true_transform(player, card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_opal_titan_animated(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 )
		modify_pt_and_abilities(player, card, event, 0, 0, instance->targets[1].player);

	return 0;
}

int card_oppression(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t *instance = get_card_instance(player, card);
		discard(instance->targets[1].player, 0, player);
	}

	return global_enchantment(player, card, event);
}

int card_order_of_yawgmoth(int player, int card, event_t event)
{
  // 0x1202812

  /* Order of Yawgmoth	|2|B|B
   * Creature - Zombie Knight 2/2
   * Fear
   * Whenever ~ deals damage to a player, that player discards a card. */

  fear(player, card, event);

  whenever_i_deal_damage_to_a_player_he_discards_a_card(player, card, event, 0, 0);

  return 0;
}


int pariah_effect(int player, int card, event_t event, int dest_player, int dest_card){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_player == player && damage->damage_target_card == card && damage->info_slot > 0 ){
				damage->damage_target_player = dest_player;
				damage->damage_target_card = dest_card;
			}
		}
	}

	return 0;
}

int card_pariah(int player, int card, event_t event){
	/* Pariah	|2|W
	 * Enchantment - Aura
	 * Enchant creature
	 * All damage that would be dealt to you is dealt to enchanted creature instead. */
	card_instance_t* instance = in_play(player, card);
	if( instance && instance->damage_target_player > -1 ){
		pariah_effect(player, -1, event, instance->damage_target_player, instance->damage_target_card);
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

int card_peregrine_drake(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		untap_lands(player, card, 5);
	}
	return 0;
}

int card_persecute(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int clr = 1<<choose_a_color(player, get_deck_color(player, instance->targets[0].player));
			int t_player = instance->targets[0].player;
			int hand_array[50];
			int hand_index = 0;
			int count = active_cards_count[t_player];
			while( count > -1 ){
				   if( in_hand(t_player, count) ){
					   card_instance_t *this = get_card_instance(t_player, count);
					   hand_array[hand_index] = this->internal_card_id;
					   hand_index++;
				   }
				   count--;
			}
			show_deck(HUMAN, hand_array, hand_index, "Target player's hand", 0, 0x7375B0 );

			count = active_cards_count[t_player];
			while( count > -1 ){
				   if( in_hand(t_player, count) && (get_color(t_player, count) & clr) ){
					   discard_card(t_player, count);
				   }
				   count--;
			}
		}
		kill_card(player, card, KILL_DESTROY);

	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_phyrexian_colossus(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	minimum_blockers(player, card, event, 3);

	does_not_untap(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		untap_card(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST0, 8, NULL, NULL);
}

int card_phyrexian_processor(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && (life[AI] < 6 || ! can_pay_life(player, 1)) ){
			ai_modifier -= 1000;
		}
	}

	if( comes_into_play(player, card, event) && can_pay_life(player, 1) ){
		int number = 4 + internal_rand(6);
		while( life[AI] - number < 6 ){
				number--;
		}
		if( player == HUMAN ){
			 number = choose_a_number(player, "Pay how much life?", life[player]-1);
		}
		lose_life(player, number);
		instance->targets[1].player = number;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_MINION, &token);
		token.pow = instance->targets[1].player;
		token.tou = instance->targets[1].player;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL);
}

int card_phyrexian_tower(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

#define CAN_PRODUCE_MANA(player, card)	(!is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card))
#define CAN_PRODUCE_BB(player, card)	can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)

  if ((event == EVENT_CHANGE_TYPE && affect_me(player, card) && in_play(player, card))
	  || event == EVENT_RESOLVE_SPELL)	// mostly so the right land sound plays
	{
	  color_test_t cols = CAN_PRODUCE_BB(player, card) ? COLOR_TEST_BLACK|COLOR_TEST_COLORLESS : COLOR_TEST_COLORLESS;

	  get_card_instance(player, card)->info_slot = cols;

	  if (event == EVENT_RESOLVE_SPELL)
		play_land_sound_effect_force_color(player, card, cols);
	}

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_PRODUCE_MANA(player, card);

  if (event == EVENT_ACTIVATE)
	{
	  int choice = 0;
	  if (CAN_PRODUCE_BB(player, card))
		choice = do_dialog(player, player, card, -1, -1, " Add 1\n Sac a creature and add BB\n Cancel", 1);

	  if (choice == 0)
		produce_mana_tapped(player, card, COLOR_COLORLESS, 1);
	  else if (choice == 1 && controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, 0))
		produce_mana_tapped(player, card, COLOR_BLACK, 2);
	  else
		spell_fizzled = 1;
	}

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_PRODUCE_MANA(player, card))
	{
	  if (CAN_PRODUCE_BB(player, card))
		declare_mana_available(player, COLOR_BLACK, 2);
	  else
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}

  return 0;
#undef CAN_PRODUCE_MANA
#undef CAN_PRODUCE_BB
}

int card_planar_birth(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
					int count = count_graveyard(p)-1;
					const int *grave = get_grave(p);
					while( count > -1 ){
							if( is_basic_land_by_id(cards_data[grave[count]].id) ){
								reanimate_permanent(p, -1, p, count, REANIMATE_TAP);
							}
							count--;
					}
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_planar_void(int player, int card, event_t event)
{
  /* Planar Void	|B
   * Enchantment
   * Whenever another card is put into a graveyard from anywhere, exile that card. */

  // From library
  enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
  if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  int i, p = trigger_cause_controller;
		  for (i = 0; i < num_cards_milled; ++i)
			if (event == EVENT_RESOLVE_TRIGGER && cards_milled[i].position != -1)
			  {
				int pos = find_in_graveyard_by_source(p, cards_milled[i].source, cards_milled[i].position);
				if (pos != -1)
				  rfg_card_from_grave(p, pos);
				cards_milled[i].position = -1;	// No longer in graveyard, so keep any other triggers from looking
			  }
		}
	}

  if (when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, NULL)
	  && gy_from_anywhere_pos != -1)
	{
	  if (BYTE0(gy_from_anywhere_source) == card && (BYTE1(gy_from_anywhere_source) & 1) == player)
		return 0;	// Can't trigger for self.  (It won't reliably get an opportunity to, though.)

	  int p = trigger_cause_controller;
	  int pos = find_in_graveyard_by_source(p, gy_from_anywhere_source, gy_from_anywhere_pos);
	  if (pos != -1)	// already removed, but hadn't been recorded
		rfg_card_from_grave(p, pos);
	  gy_from_anywhere_pos = -1;	// No longer in graveyard, so keep any other triggers from looking
	}

  return global_enchantment(player, card, event);
}

// polluted mire --> drifting_meadow

int card_pouncing_jaguar(int player, int card, event_t event){

	echo(player, card, event, MANACOST_G(1));

	return 0;
}

int card_priest_of_gix(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		produce_mana(player, COLOR_BLACK, 3);
	}

	return 0;
}

int card_priest_of_titania(int player, int card, event_t event){
	int count = 0;
	if (event == EVENT_COUNT_MANA || event == EVENT_ACTIVATE || (event == EVENT_CAN_ACTIVATE && player == AI)){	// Only count if it'll actually be needed
		count = count_subtype(2, TYPE_PERMANENT, SUBTYPE_ELF);
	}
	return mana_producing_creature(player, card, event, 48, COLOR_GREEN, count);
}

int card_purging_scythe(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int i;
		int t_player = -1;
		int t_card = -1;
		int doubles_count = 1;
		int min_t = 100;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
				   if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
					   if( get_toughness(i, count) < min_t ){
						   t_player = i;
						   t_card = count;
						   min_t = get_toughness(i, count);
						   doubles_count = 1;
						   instance->targets[doubles_count].player = t_player;
						   instance->targets[doubles_count].card = t_card;
					   }
					   else if( get_toughness(i, count) == min_t ){
								doubles_count++;
								instance->targets[doubles_count].player = i;
								instance->targets[doubles_count].card = count;
					   }


				   }
				   count--;
			}
		}

		if( doubles_count > 1 ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.illegal_abilities = 0;
			td.toughness_requirement = min_t;
			td.allow_cancel = 0;

			instance->number_of_targets = 0;
			char msg[100];
			scnprintf(msg, 100, "Select target creature with toughness %d.", min_t);
			if( new_pick_target(&td, msg, 0, GS_LITERAL_PROMPT) ){
				t_player = instance->targets[0].player;
				t_card = instance->targets[0].card;
			}
		}

		if( t_player != -1 && t_card != -1 ){
			damage_creature(t_player, t_card, 2, player, card);
		}
	}

	return 0;
}

int card_rain_of_filth(int player, int card, event_t event){
	/* Rain of Filth	|B
	 * Instant
	 * Until end of turn, lands you control gain "Sacrifice this land: Add |B to your mana pool." */

	if( event == EVENT_RESOLVE_SPELL ){
		generate_token_by_id(player, card, CARD_ID_RAIN_OF_FILTH_EFFECT);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_rain_of_filth_effect(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
	   return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_ACTIVATE ){
		if( ! controller_sacrifices_a_permanent(player, card, TYPE_LAND, 0) ){
			spell_fizzled = 1;
			return 0;
		}
		produce_mana(player, COLOR_BLACK, 1 );
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			declare_mana_available(player, COLOR_BLACK, 1);
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_rain_of_salt(int player, int card, event_t event){
	/*
	  Rain of Salt |4|R|R
	  Sorcery
	  Destroy two target lands.
	*/
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 2, NULL);
}

int card_raze(int player, int card, event_t event)
{
  /* Raze	|R
   * Sorcery
   * As an additional cost to cast ~, sacrifice a land.
   * Destroy target land. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_CAST)
	{
	  if (!can_sacrifice_type_as_cost(player, 1, TYPE_LAND))
		return 0;

	  // Either other player must have a targettable land, or player must have at least two lands and at least one must be targettable.
	  td.allowed_controller = 1-player;
	  if (can_target(&td))
		return 1;

	  td.allowed_controller = player;
	  return can_target(&td) && basiclandtypes_controlled[player][COLOR_ANY] >= 2;
	}

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  if (is_token(player, card))
		pick_target(&td, "TARGET_LAND");
	  else
		{
		  int sac = controller_sacrifices_a_permanent(player, card, TYPE_LAND, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE);
		  if (!sac)
			return 0;

		  int sac_p = BYTE2(sac), sac_c = BYTE3(sac);

		  instance->number_of_targets = 0;
		  if (can_target(&td) && pick_target(&td, "TARGET_LAND"))
			kill_card(sac_p, sac_c, KILL_SACRIFICE);
		  else
			{
			  state_untargettable(sac_p, sac_c, 0);
			  cancel = 1;
			}
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_recantation(int player, int card, event_t event){
	/* Recantation	|3|U|U
	 * Enchantment
	 * At the beginning of your upkeep, you may put a verse counter on ~.
	 * |U, Sacrifice ~: Return up to X target permanents to their owners' hands, where X is the number of verse counters on ~. */

	if( ! IS_GAA_EVENT(event) ){
		return generic_growing_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_U(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		ai_modifier+=(count_counters(player, card, COUNTER_VERSE) < 2 ? -5 : 0);
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_U(1)) ){
			int max = count_counters(player, card, COUNTER_VERSE);
			int tg = 0;
			while( tg < max && can_target(&td) ){
					if( new_pick_target(&td, "TARGET_PERMANENT", tg, 0) ){
						state_untargettable(instance->targets[tg].player, instance->targets[tg].card, 1);
						tg++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				action_on_target(player, card, i, ACT_BOUNCE);
			}
		}
	}

	return 0;
}

int card_redeem(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				prevent_the_next_n_damage(player, card, instance->targets[i].player, instance->targets[i].card, 0, PREVENT_INFINITE, 0, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 2, NULL);
}

int card_reflexes(int player, int card, event_t event){
	/*
	  Reflexes |R
	  Enchantment - Aura, R
	  Enchant creature (Target a creature as you cast this. This card enters the battlefield attached to that creature.)
	  Enchanted creature has first strike. (It deals combat damage before creatures without first strike.)
	*/
	return generic_aura(player, card, event, player, 0, 0, KEYWORD_FIRST_STRIKE, 0, 0, 0, 0);
}

int card_rejuvenate(int player, int card, event_t event){

	if(event == EVENT_CAN_CAST){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
		gain_life(player, 6);
		kill_card(player, card, KILL_DESTROY);
	}

	return us_cycling(player, card, event);
}

int card_remembrance(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	count_for_gfp_ability_and_store_values(player, card, event, player, TYPE_CREATURE, NULL, GFPC_TRACK_DEAD_CREATURES | GFPC_EXTRA_SKIP_TOKENS, 0);

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int added[20];
		int ac = 0;
		int k;
		for(k=0; k<10; k++){
			if( instance->targets[k].player != -1 ){
				int result = seek_grave_for_id_to_reanimate(player, -1, player, cards_data[instance->targets[k].player].id, REANIMATEXTRA_LEAVE_IN_GRAVEYARD);
				if( result != -1 ){
					remove_card_from_grave(player, result);
					added[ac] = instance->targets[k].player;
					ac++;
				}
				instance->targets[k].player = -1;
			}
			if( instance->targets[k].card != -1 ){
				int result = seek_grave_for_id_to_reanimate(player, -1, player, cards_data[instance->targets[k].card].id, REANIMATEXTRA_LEAVE_IN_GRAVEYARD);
				if( result != -1 ){
					remove_card_from_grave(player, result);
					added[ac] = instance->targets[k].card;
					ac++;
				}
			}
			instance->targets[k].card = -1;
		}
		if( ac ){
			int i;
			for(i=0;i<ac;i++){
				char prompt[100];
				scnprintf(prompt, 600, "Select a card named %s", cards_ptr[added[i]]->name);
				test_definition_t test;
				new_default_test_definition(&test, TYPE_CREATURE, prompt);
				test.id = cards_data[added[i]].id;
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &test);
			}
		}
		instance->targets[11].player = 0;
	}

	return global_enchantment(player, card, event);
}

// remote isle --> drifting meadow

int card_reprocess(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;

		card_instance_t *instance= get_card_instance(player, card);

		int crds = 0;
		while( can_target(&td) ){
				instance->number_of_targets = 0;
				if( pick_target(&td, "TARGET_PERMANENT") ){
					kill_card(player, instance->targets[0].card, KILL_SACRIFICE);
					crds++;
				}
				else{
					break;
				}
		}

		if( crds > 0 ){
			draw_cards(player, crds);
		}

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_rescind(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT );

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY);
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
	}

	return us_cycling(player, card, event);
}

int card_rewind(int player, int card, event_t event){
	// Original code : 0x412A70

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		untap_lands(player, card, 4);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 0, NULL);
}

int card_rumbling_crescendo(int player, int card, event_t event){
	/* Rumbling Crescendo	|3|R|R
	 * Enchantment
	 * At the beginning of your upkeep, you may put a verse counter on ~.
	 * |R, Sacrifice ~: Destroy up to X target lands, where X is the number of verse counters on ~. */

	if( ! IS_GAA_EVENT(event) ){
		return generic_growing_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_R(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		ai_modifier+=(count_counters(player, card, COUNTER_VERSE) < 2 ? -5 : 0);
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_R(1)) ){
			int max = count_counters(player, card, COUNTER_VERSE);
			int tg = 0;
			while( tg < max && can_target(&td) ){
					if( new_pick_target(&td, "TARGET_LAND", tg, 0) ){
						state_untargettable(instance->targets[tg].player, instance->targets[tg].card, 1);
						tg++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				action_on_target(player, card, i, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_sanctum_guardian(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			target->info_slot = 0;
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST0, 0, &td, "TARGET_DAMAGE");
}

int card_scoria_wurm(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( ! flip_a_coin(player, card) ){
			bounce_permanent(player, card);
		}
	}

	return 0;
}

int card_scrap2(int player, int card, event_t event){

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT );

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			}
			kill_card(player, card, KILL_DESTROY);
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 1, NULL);
	}

	return us_cycling(player, card, event);
}

int card_seasoned_marshal(int player, int card, event_t event)
{
  // 0x412fb0

  /* Seasoned Marshal	|2|W|W
   * Creature - Human Soldier 2/2
   * Whenever ~ attacks, you may tap target creature. */

  /* Niblis of the Urn	|1|W
   * Creature - Spirit 1/1
   * Flying
   * Whenever ~ attacks, you may tap target creature. */

  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		tap_card(instance->targets[0].player, instance->targets[0].card);
	}

  return 0;
}

int card_serra_advocate(int player, int card, event_t event){
	/*
	  Serra Advocate |3|W
	  Creature - Angel 2/2
	  Flying
	  {T}: Target attacking or blocking creature gets +2/+2 until end of turn.
	*/
	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;
	td.preferred_controller = player;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_serra_avatar(int player, int card, event_t event)
{
  /* Serra Avatar	|4|W|W|W
   * Creature - Avatar 100/100
   * ~'s power and toughness are each equal to your life total.
   * When ~ is put into a graveyard from anywhere, shuffle it into its owner's library. */

  // Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && player != -1)
	event_result += life[player];

  return 0;
}

int card_serras_embrace(int player, int card, event_t event){
	/*
	  Serra's Embrace |2|W|W
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature gets +2/+2 and has flying and vigilance.
	*/
	return generic_aura(player, card, event, player, 2, 2, KEYWORD_FLYING, SP_KEYWORD_VIGILANCE, 0, 0, 0);
}

int card_serras_liturgy(int player, int card, event_t event){
	/* Serra's Liturgy	|2|W|W
	 * Enchantment
	 * At the beginning of your upkeep, you may put a verse counter on ~.
	 * |W, Sacrifice ~: Destroy up to X target artifacts and/or enchantments, where X is the number of verse counters on ~. */

	if( ! IS_GAA_EVENT(event) ){
		return generic_growing_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT | TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_W(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		ai_modifier+=(count_counters(player, card, COUNTER_VERSE) < 2 ? -5 : 0);
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_W(1)) ){
			int max = count_counters(player, card, COUNTER_VERSE);
			int tg = 0;
			while( tg < max && can_target(&td) ){
					if( new_pick_target(&td, "DISENCHANT", tg, 0) ){
						state_untargettable(instance->targets[tg].player, instance->targets[tg].card, 1);
						tg++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				action_on_target(player, card, i, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_serra_sanctum(int player, int card, event_t event){
	return urza_multimana_lands(player, card, event, TYPE_ENCHANTMENT, COLOR_WHITE);
}

int card_shivs_embrace(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;
		if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, 0, MANACOST_R(1), 0, NULL, NULL);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, t_player, t_card, 1, 0, 0, 0);
		}
	}

	return generic_aura(player, card, event, player, 2, 2, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_shivan_gorge(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = 0;
		if( !paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_XR(3, 1)) ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Damage opponent\n Do nothing", 1);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				add_status(player, card, STATE_TAPPED);
				if( charge_mana_for_activated_ability(player, card, MANACOST_XR(2, 1)) ){
					instance->info_slot = 1;
					tap_card(player, card);
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
			damage_player(1-player, 1, instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_shivan_hellkite(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		damage_creature_or_player(player, card, event, 1);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(1, 1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_shivan_raptor(int player, int card, event_t event){
	haste(player, card, event);
	echo(player, card, event, MANACOST_XR(2, 1));
	return 0;
}

int card_show_and_tell(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "Choose an artifact, creature, enchantment, or land.");
		this_test.type_flag = F1_NO_PWALKER;
		int selected[2] = {-1, -1};
		APNAP(p, {selected[p] = new_select_a_card(p, p, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);};);
		APNAP(p,{
					if( selected[p] != -1 ){
						put_into_play(p, selected[p]);
					}
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_skirge_familiar(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_CAN_ACTIVATE && hand_count[player] > 0 ){
		   return can_produce_mana(player, card);
		}

		if( event == EVENT_ACTIVATE ){
			discard(player, 0, player);
			produce_mana(player, COLOR_BLACK, 1);
		}

		if( event == EVENT_COUNT_MANA && affect_me(player, card) && hand_count[player] > 0 ){
			declare_mana_available(player, COLOR_BLACK, 1);
		}
	}

	return 0;
}

int card_skittering_skirge(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

// slippery karst --> drifting meadow

int card_smokestack(int player, int card, event_t event){

	/* Smokestack	|4
	 * Artifact
	 * At the beginning of your upkeep, you may put a soot counter on ~.
	 * At the beginning of each player's upkeep, that player sacrifices a permanent for each soot counter on ~. */

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		impose_sacrifice(player, card, current_turn, count_counters(player, card, COUNTER_SOOT), TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		if( current_turn == player ){
			int choice = do_dialog(player, player, card, -1, -1, " Add soot counter\n Pass", 0);
			if( choice == 0 ){
				add_counter(player, card, COUNTER_SOOT);
			}
		}
	}

	return 0;
}

// smoldering crater --> drifting meadow

int card_sneak_attack(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( IS_GAA_EVENT(event) ){
		if(event == EVENT_RESOLVE_ACTIVATION ){
			char msg[100] = "Select a creature card,";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			int result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
			if( result > -1 ){
				create_targetted_legacy_effect(player, instance->parent_card, &haste_and_sacrifice_eot, player, result);
			}
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_R(1), 0, NULL, NULL);
	}

	return global_enchantment(player, card, event);
}

static int somnophore_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	does_not_untap(instance->targets[0].player, instance->targets[0].card, event);

	if( leaves_play(instance->targets[1].player, instance->targets[1].card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_somnophore(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		instance->number_of_targets = 0;
		if( can_target(&td) && new_pick_target(&td, "Select target creature defending player controls.", 0, GS_LITERAL_PROMPT) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			int legacy = create_targetted_legacy_effect(player, card, &somnophore_effect, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].player = player;
			leg->targets[1].card = card;
			leg->number_of_targets = 2;
		}
	}

	return 0;
}

int card_songstitcher(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			negate_combat_damage_this_turn(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XW(1, 1), 0, &td1, "Select target creature with flying.");
}

int card_spined_fluke(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && count_subtype(player, TYPE_CREATURE, -1) < 1 ){
			ai_modifier-=1000;
		}
	}

	if( comes_into_play(player, card, event) ){
		impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return regeneration(player, card, event, MANACOST_B(1));
}

int card_spire_owl(int player, int card, event_t event){
	/*
	  Spire Owl |1|U
	  Creature - Bird 1/1
	  Flying
	  When Spire Owl enters the battlefield, look at the top four cards of your library, then put them back in any order.
	*/
	if( comes_into_play(player, card, event) ){
		rearrange_top_x(player, player, 4);
	}

	return 0;
}

int card_spreading_algae(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	immortal_enchantment(player, card, event);

	if( IS_AURA_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND );
		td.required_subtype = get_hacked_subtype(player, card, SUBTYPE_SWAMP);

		if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if (instance->damage_target_player >= 0 && instance->damage_target_card >= 0){
				instance->targets[0].player = instance->damage_target_player;
				instance->targets[0].card = instance->damage_target_card;
				instance->number_of_targets = 1;
			}
			else if (! new_pick_target(&td, get_hacked_land_text(player, card, "Select target %s.", SUBTYPE_SWAMP), 0, 1 | GS_LITERAL_PROMPT) ){
					return 0;
			}
			if (player == AI && !(trace_mode & 2)){
				ai_modifier += (instance->targets[0].player == td.preferred_controller) ? 24 : -24;
			}

		}

		if( !(event == EVENT_CAST_SPELL && affect_me(player, card)) ){
			return targeted_aura(player, card, event, &td, NULL);
		}

	}

	if( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( trigger_condition == TRIGGER_TAP_CARD && affect_me(player, card) && reason_for_trigger_controller == player ){
			if( trigger_cause_controller == p && trigger_cause == c ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						kill_card(p, c, KILL_DESTROY);
				}
			}
		}
	}

	return 0;

}

int card_steam_blast(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {new_damage_all(player, card, p, 2, NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_stern_proctor(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_ARTIFACT_ENCHANTMENT")){
			card_instance_t* instance = get_card_instance(player, card);
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_stroke_of_genius(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags(player, card, SF2_X_SPELL);
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			charge_mana(player, COLOR_COLORLESS, -1);
			if( spell_fizzled != 1 ){
				instance->info_slot = x_value;
			}
		}
		if( spell_fizzled != 1 ){
			pick_target(&td, "TARGET_PLAYER");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(instance->targets[0].player, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sunder2(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL && affect_me (player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, ACT_BOUNCE);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_symbiosis(int player, int card, event_t event){

	if( event == EVENT_CHECK_PUMP ){
		return vanilla_instant_pump(player, card, event, ANYBODY, player, 2, 2, 0, 0);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				pump_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 2, 2);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_tainted_aether(int player, int card, event_t event){

	if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t *instance = get_card_instance(player, card);
		impose_sacrifice(player, card, instance->targets[1].player, 1, TYPE_CREATURE | TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return global_enchantment(player, card, event);
}

int card_telepathy(int player, int card, event_t event)
{
  if (event == EVENT_STATIC_EFFECTS)
	player_bits[1-player] |= PB_HAND_REVEALED;

  return global_enchantment(player, card, event);
}

static int temporal_aperture_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card);
	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	if( instance->targets[0].player > -1 && deck_ptr[player][0] != instance->targets[0].player ){
		return 0;
	}
	if(event == EVENT_SET_LEGACY_EFFECT_NAME && instance->targets[0].player > -1 ){
		scnprintf(set_legacy_effect_name_addr, 51, "%s", cards_ptr[cards_data[instance->targets[0].player].id]->name);
	}
	if( instance->targets[0].card > -1 && event == EVENT_CAN_ACTIVATE ){
		int rval = can_legally_play_iid(player, instance->targets[0].card);
		if( rval == 99 && (cards_data[instance->targets[0].card].type & (TYPE_INSTANT | TYPE_INTERRUPT)) ){
			return 99;
		}
		if( rval && !(cards_data[instance->targets[0].card].type & (TYPE_INSTANT | TYPE_INTERRUPT)) && ! can_sorcery_be_played(player, event) ){
			rval = 0;
		}
		return rval;
	}
	if( event == EVENT_ACTIVATE ){
		char buffer[500];
		card_ptr_t* c = cards_ptr[ cards_data[instance->targets[0].card].id ];
		scnprintf(buffer, 500, " Play %s\n Cancel", c->name);
		int choice = do_dialog(player, player, card, -1, -1, buffer, 1);
		if( choice == 0 ){
			play_card_in_deck_for_free(player, player, 0);
			cant_be_responded_to = 1;
			kill_card(player, card, KILL_REMOVE);
		}
		if( choice == 1 ){
			spell_fizzled = 1;
		}
	}
	return 0;
}

int card_temporal_aperture(int player, int card, event_t event){
	/*
	  Temporal Aperture |2
	  Artifact,
	  {5}, {T}: Shuffle your library, then reveal the top card. Until end of turn, for as long as that card remains on top of your library,
	  play with the top card of your library revealed and you may play that card without paying its mana cost. (If it has X in its mana cost, X is 0.)
	*/
	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_ACTIVATE && player == AI ){
		// Check for other Temporal Aperture legacies
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_EFFECT) ){
				card_instance_t *eff = get_card_instance( player, c);
				if( eff->info_slot == (int)temporal_aperture_legacy ){
					ai_modifier-=100;
					break;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		shuffle(player);
		if( deck_ptr[player][0] != -1 ){
			int legacy = create_legacy_activate(instance->parent_controller, instance->parent_card, &temporal_aperture_legacy);
			card_instance_t *parent = get_card_instance(instance->parent_controller, legacy);
			parent->targets[0].card = deck_ptr[player][0];
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(5), 0, NULL, NULL);
}

int card_thran_quarry(int player, int card, event_t event)
{
  /* Thran Quarry	""
   * Land
   * At the beginning of the end step, if you control no creatures, sacrifice ~.
   * |T: Add one mana of any color to your mana pool. */

  if (trigger_condition == TRIGGER_EOT && affect_me(player, card) && reason_for_trigger_controller == player
	  && count_permanents_by_type(player, TYPE_CREATURE) <= 0 && eot_trigger(player, card, event))
	kill_card(player, card, KILL_SACRIFICE);

  return mana_producer(player, card, event);
}

int card_time_spiral(int player, int card, event_t event){
	// Original code :    0x4DB7A0

	if(event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
				reshuffle_hand_into_deck(p, 1);
				reshuffle_grave_into_deck(p, 0);
				draw_cards(p, 7);
			};
			);
		untap_lands(player, card, 6);
		kill_card(player, card, KILL_REMOVE);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_titanias_boon(int player, int card, event_t event)
{
	/* Titania's Boon	|3|G
	 * Sorcery
	 * Put a +1/+1 counter on each creature you control. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  manipulate_type(player, card, player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	  kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_titanias_chosen(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, COLOR_TEST_GREEN, 0, 0, 0, -1, 0) ){
		add_1_1_counter(player, card);
	}
	return 0;
}

int card_tolarian_academy(int player, int card, event_t event){
	// original code : 0x4E56A6
	return urza_multimana_lands(player, card, event, TYPE_ARTIFACT, COLOR_BLUE);
}

int card_torch_song(int player, int card, event_t event){

	/* Torch Song	|2|R
	 * Enchantment
	 * At the beginning of your upkeep, you may put a verse counter on ~.
	 * |2|R, Sacrifice ~: ~ deals X damage to target creature or player, where X is the number of verse counters on ~. */

	if( ! IS_GAA_EVENT(event) ){
		return generic_growing_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, MANACOST_XR(2, 1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
	}
	if( event == EVENT_ACTIVATE ){
		instance->info_slot = count_counters(player, card, COUNTER_VERSE);
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, MANACOST_XR(2, 1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, instance->info_slot);
		}
	}

	return global_enchantment(player, card, event);
}

int card_turnabout(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int types[3] = {TYPE_LAND, TYPE_CREATURE, TYPE_ARTIFACT};
			int ct[3] = {count_subtype(instance->targets[0].player, TYPE_LAND, -1), count_subtype(instance->targets[0].player, TYPE_CREATURE, -1),
						count_subtype(instance->targets[0].player, TYPE_ARTIFACT, -1)};

			int max = 0;
			int ai_choice = 0;
			int i;
			for(i=0; i<3; i++){
				if( ct[i] > max ){
					max = ct[i];
					ai_choice = i;
				}
			}
			int choice = do_dialog(player, player, card, -1, -1, " Land\n Creature \n Artifact", ai_choice);
			int c2 = do_dialog(player, player, card, -1, -1, " Tap\n Untap", instance->targets[0].player == player ? 1 : 0);
			int action = c2 == 0 ? ACT_TAP : ACT_UNTAP;

			manipulate_all(player, card, instance->targets[0].player, types[choice], 0, 0, 0, 0, 0, 0, 0, -1, 0, action);

		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_umbilicus(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allowed_controller = current_turn;
		td.preferred_controller = current_turn;
		td.illegal_abilities = 0;
		td.who_chooses = current_turn;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;
		int choice = 0;
		if( can_pay_life(player, 2) ){
			int ai_choice = life[player]-2 < 6 ? 1 : 0;
			choice = do_dialog(current_turn, player, card, -1, -1, " Pay 2 life\n Bounce permanent", ai_choice);
		}
		if( choice == 0 ){
			lose_life(current_turn, 2);
		}
		if( choice == 1 ){
			if( pick_target(&td, "TARGET_PERMANENT") ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_urzas_armor(int player, int card, event_t event){

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_PREVENT_DAMAGE ){
			card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card && damage->damage_target_player == player &&
				damage->damage_target_card == -1 && damage->info_slot > 0 && damage->targets[4].player == -1
			  ){
				damage->info_slot--;
			}
		}
	}
	return 0;
}

int card_vampiric_embrace(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( sengir_vampire_trigger(instance->damage_target_player, instance->damage_target_card, event, 2) ){
			card_instance_t *ec = get_card_instance(instance->damage_target_player, instance->damage_target_card);
			add_1_1_counters(instance->damage_target_player, instance->damage_target_card, ec->targets[11].card);
			ec->targets[11].card = 0;
		}
	}

	return generic_aura(player, card, event, player, 2, 2, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_vebulid(int player, int card, event_t event)
{
  // ~ enters the battlefield with a +1/+1 counter on it.
  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 1);

  // At the beginning of your upkeep, you may put a +1/+1 counter on ~.
  upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	add_1_1_counter(player, card);

  // When ~ attacks or blocks, destroy it at end of combat.
  if (declare_attackers_trigger(player, card, event, 0, player, card)
	  || (blocking(player, card, event) && !is_humiliated(player, card)))
	create_targetted_legacy_effect(player, card, die_at_end_of_combat, player, card);

  return 0;
}

int card_veil_of_birds(int player, int card, event_t event){
	return hidden_enchantment(player, card, event, TYPE_ANY, NULL, SUBTYPE_BIRD);
}

int card_veiled_sentry(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		instance->info_slot = get_cmc(instance->targets[1].player, instance->targets[1].player);
		add_a_subtype(player, card, SUBTYPE_ILLUSION);
		instance->targets[12].card = get_internal_card_id_from_csv_id(CARD_ID_VEILED_SENTRY_ANIMATED);
	}

	return global_enchantment(player, card, event);
}

int card_veiled_sentry_animated(int player, int card, event_t event){
	/* Veiled Sentry Animated	|U
	 * Creature - Illusion 100/100
	 * Veiled Sentry becomes an Illusion creature with power and toughness each equal to that spell's converted mana cost. */
	if (card == -1){
		return 0;	// Can't imagine how the animated version would get inspected while not on the bf, but whatever
	}
	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card)){
		event_result += get_card_instance(player, card)->info_slot;
	}
	return 0;
}

int card_vernal_bloom(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if ((event == EVENT_COUNT_MANA || event == EVENT_TAP_CARD) && is_what(affected_card_controller, affected_card, TYPE_LAND)
			&& has_subtype(affected_card_controller, affected_card, SUBTYPE_FOREST)){
			// See comments in card_mana_flare().

			if (!in_play(player, card)){
				return 0;
			}

			if (event == EVENT_COUNT_MANA){
				if (is_tapped(affected_card_controller, affected_card) || is_animated_and_sick(affected_card_controller, affected_card)
					|| !can_produce_mana(affected_card_controller, affected_card) ){
					return 0;
				}

				declare_mana_available(affected_card_controller, COLOR_GREEN, 1);
			} else {	// event == EVENT_TAP_CARD
				if (tapped_for_mana_color >= 0){
					/* Triggers even if the land produced no mana so long as it was tapped for a mana ability (such as a Tolarian Academy that somehow became a
					 * forest too, with no artifacts in play), by analogy with the ruling for Overabundance.  Differs from Mana Flare since that "adds one
					 * mana... of any type that land produced". */
					produce_mana(affected_card_controller, COLOR_GREEN, 1);
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_victimize(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return (count_graveyard_by_type (player, TYPE_CREATURE) >= 2
				&& !graveyard_has_shroud(player)
				&& can_sacrifice(player, player, 1, TYPE_CREATURE, 0));
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select 2 creature cards.");
		card_instance_t* instance = get_card_instance(player, card);
		select_multiple_cards_from_graveyard(player, player, -1, AI_MAX_VALUE, &this_test, 2, &instance->targets[4]);
	}

	else if(event == EVENT_RESOLVE_SPELL ){
		// If both target cards are illegal at resolution, the spell fizzles; if just one is, the other is still affected.
		int i, num_validated = 0;
		for (i = 4; i <= 5; ++i){
			int selected = validate_target_from_grave(player, card, player, i);
			if (selected != -1){
				++num_validated;
			}
		}
		if (num_validated <= 0){
			spell_fizzled = 1;
		} else if (sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)){
			// We have to revalidate, since the cards might have gone away because we sacrificed something like Emrakul, the Aeons Torn
			for (i = 4; i <= 5; ++i){
				int selected = validate_target_from_grave(player, card, player, i);
				if (selected != -1){
					reanimate_permanent(player, card, player, selected, REANIMATE_TAP);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_vile_requiem(int player, int card, event_t event){
	/* Vile Requiem	|2|B|B
	 * Enchantment
	 * At the beginning of your upkeep, you may put a verse counter on ~.
	 * |1|B, Sacrifice ~: Destroy up to X target non|Sblack creatures, where X is the number of verse counters on ~. They can't be regenerated. */

	if( ! IS_GAA_EVENT(event) ){
		return generic_growing_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_XB(1, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		ai_modifier+=(count_counters(player, card, COUNTER_VERSE) < 2 ? -5 : 0);
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_XB(1, 1)) ){
			int max = count_counters(player, card, COUNTER_VERSE);
			int tg = 0;
			while( tg < max && can_target(&td) ){
					if( new_pick_target(&td, "Select target non black creature.", tg, GS_LITERAL_PROMPT) ){
						state_untargettable(instance->targets[tg].player, instance->targets[tg].card, 1);
						tg++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<instance->number_of_targets; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				action_on_target(player, card, i, KILL_BURY);
			}
		}
	}

	return 0;
}

int card_voltaic_key(int player, int card, event_t event ){
	// original code : 0x1200000

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, "TARGET_ARTIFACT");
}

int card_war_dance(int player, int card, event_t event){

	/* War Dance	|G
	 * Enchantment
	 * At the beginning of your upkeep, you may put a verse counter on ~.
	 * Sacrifice ~: Target creature gets +X/+X until end of turn, where X is the number of verse counters on ~. */

	if( ! IS_GAA_EVENT(event) ){
		return generic_growing_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST0, 0, &td, "TARGET_CREATURE");
	}
	if( event == EVENT_ACTIVATE ){
		instance->info_slot = count_counters(player, card, COUNTER_VERSE);
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST0, 0, &td, "TARGET_CREATURE");
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, instance->info_slot, instance->info_slot);
		}
	}

	return global_enchantment(player, card, event);
}

int card_western_paladin(int player, int card, event_t event){
	/*
	  Western Paladin |2|B|B
	  Creature - Zombie Knight 3/3
	  {B}{B}, {T}: Destroy target white creature.
	*/
  return us_paladin(player, card, event, COLOR_WHITE);
}

int card_whetstone(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		APNAP(p, {mill(p, 2);};);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_X(3), 0, NULL, NULL);
}

int card_whirlwind2(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_wild_dogs(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND || (event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1) ){
		return us_cycling(player, card, event);
	}
	return card_ghazban_ogre(player, card, event);
}

int card_wildfire(int player, int card, event_t event){

	/* Wildfire	|4|R|R
	 * Sorcery
	 * Each player sacrifices four lands. ~ deals 4 damage to each creature. */

	if(event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {impose_sacrifice(player, card, p, 4, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);};);
		APNAP(p, {new_damage_all(player, card, p, 4, NDA_ALL_CREATURES, NULL);};);
		kill_card(player, card, KILL_REMOVE);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_windfall(int player, int card, event_t event){
	/*
	  Windfall |2|U
	  Sorcery
	  Each player discards his or her hand, then draws cards equal to the greatest number of cards a player discarded this way.
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		int ctd = MAX(hand_count[player], hand_count[1-player]);
		APNAP(p,{
				new_discard_all(p, player);
				draw_cards(p, ctd);
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_witch_engine(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		produce_mana_tapped(player, card, COLOR_BLACK, 4);
		give_control(player, card, player, card);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
		declare_mana_available(player, COLOR_BLACK, 4);;
	}

	return 0;
}

int card_wizard_mentor(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			bounce_permanent(instance->parent_controller, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td1, "TARGET_CREATURE");
}

int card_worn_powerstone(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	return card_sol_ring(player, card, event);
}

int card_worship(int player, int card, event_t event)
{
  if (event == EVENT_CAN_CAST)
	return 1;

  card_instance_t* damage;
  if (event == EVENT_DAMAGE_REDUCTION
	  && event_result < 1
	  && (damage = get_card_instance(affected_card_controller, affected_card))
	  && damage->damage_target_player == player && damage->damage_target_card == -1
	  && creature_count[player] > 0)
	event_result = MIN(1, life[player]);	// If life already below 1, then neither increase nor decrease it

  if (event == EVENT_SHOULD_AI_PLAY && in_play(player, card) && creature_count[player] > 0)
	{
	  if (player == AI)
		ai_modifier += 480;
	  else
		ai_modifier -= 480;
	}

  return 0;
}

static int effect_yawgmoths_will(int player, int card, event_t event){

	if_a_card_would_be_put_into_graveyard_from_anywhere_exile_it_instead(player, card, event, player, NULL);

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return can_activate_to_play_cards_from_graveyard(player, card, event, TYPE_ANY);
}

int card_yawgmoths_will(int player, int card, event_t event)
{
  /* Yawgmoth's Will	|2|B
   * Sorcery
   * Until end of turn, you may play cards from your graveyard.
   * If a card would be put into your graveyard from anywhere this turn, exile that card instead. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  create_legacy_activate(player, card, &effect_yawgmoths_will);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_zephid_embrace(int player, int card, event_t event ){
	return generic_aura(player, card, event, player, 2, 2, KEYWORD_FLYING | KEYWORD_SHROUD, 0, 0, 0, 0);
}
