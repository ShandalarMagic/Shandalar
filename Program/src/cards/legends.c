#include "manalink.h"

// Functions

static int song(int player, int card, event_t event, int clr){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int trgs = 0;
		while( can_target(&td) ){
				if( new_pick_target(&td, "TARGET_CREATURE", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		if( trgs > 0 ){
			int i;
			for(i=0; i<trgs; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			instance->info_slot = trgs;
		}
		else{
			spell_fizzled = 1;
		}
	}


	if( event == EVENT_RESOLVE_SPELL ){
			int i;
			for(i=0; i<instance->info_slot; i++){
				change_color(player, card, instance->targets[i].player, instance->targets[i].card, clr, CHANGE_COLOR_SET|CHANGE_COLOR_END_AT_EOT);
			}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int mana_battery(int player, int card, event_t event, color_t color)
{
  // 0x423600

  /* |2, |T: Put a charge counter on ~.
   * |T, Remove any number of charge counters from ~: Add |C to your mana pool, then add an additional |C to your mana pool for each charge counter removed this
   * way. */

#define CAN_PRODUCE_SEP	(can_produce_mana(player, card))
#define CAN_PRODUCE		(CAN_TAP(player, card) && CAN_PRODUCE_SEP)
#define CAN_CHARGE_SEP(counters)	(can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(counters + 3)))
#define CAN_CHARGE(counters)		(CAN_TAP(player, card) && CAN_CHARGE_SEP(counters))

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (event == EVENT_CAN_ACTIVATE && !CAN_TAP(player, card))
		return 0;

	  if (event == EVENT_ACTIVATE)
		load_text(0, "MANABATTERY");

	  int num_counters = count_counters(player, card, COUNTER_CHARGE);

	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_CHARGE
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1,
						text_lines[0], CAN_PRODUCE_SEP, 2,
						text_lines[1], !paying_mana() && CAN_CHARGE_SEP(num_counters), (current_phase == PHASE_DISCARD && player != current_turn) ? 3 : 1);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_MANA:
			  if (num_counters == 0)
				{
				  produce_mana_tapped(player, card, color, 1);
				  break;
				}

			  int amt;
			  if (IS_AI(player))
				amt = recorded_rand(player, num_counters + 1);
			  else
				{
				  char prompt1[300], prompt2[600];
				  if (ai_is_speculating == 1)
					prompt2[0] = 0;
				  else
					{
					  load_text(0, "MANABATTERY");
					  scnprintf(prompt1, 300, text_lines[3], num_counters);
					  scnprintf(prompt2, 600, "%s\n\n%s", cards_ptr[get_id(player, card)]->full_name, prompt1);
					}
				  amt = choose_a_number(player, prompt2, 0);
				}

			  if (amt == -1)
				cancel = 1;
			  else
				{
				  amt = MIN(amt, num_counters);
				  remove_counters(player, card, COUNTER_CHARGE, amt);
				  produce_mana_tapped(player, card, color, amt + 1);
				}
			  break;

			case CHOICE_CHARGE:
			  ;card_instance_t* instance = get_card_instance(player, card);
			  instance->state |= STATE_TAPPED;
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(2)))
				tapped_for_mana_color = -1;
			  else
				untap_card_no_event(player, card);
			  break;
		  }
	  else if (choice == CHOICE_CHARGE)	// and event == EVENT_RESOLVE_ACTIVATION
		add_counter(player, card, COUNTER_CHARGE);
	}

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_PRODUCE)
	declare_mana_available(player, color, 1 + count_counters(player, card, COUNTER_CHARGE));

  if (event == EVENT_CAN_WASTE_MANA && CAN_CHARGE(count_counters(player, card, COUNTER_CHARGE)))
	event_result |= 1;

  if (event == EVENT_SHOULD_AI_PLAY && player == AI)
	ai_modifier += 12 * count_counters(player, card, COUNTER_CHARGE);

  return 0;
#undef CAN_PRODUCE_SEP
#undef CAN_PRODUCE
#undef CAN_CHARGE_SEP
#undef CAN_CHARGE
}

void landwalk_disabling_card(int player, int card, event_t event, player_bits_t landwalk_disabled){
	if( event == EVENT_ABILITIES ){
		player_bits[player] |= landwalk_disabled;
		player_bits[1-player] |= landwalk_disabled;
	}
}

static int leg_color_hoser(int player, int card, event_t event, int permanent_color, int land_subtype){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_color = 1<<get_sleighted_color(player, card, permanent_color);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_LAND);
	td1.required_subtype = get_hacked_subtype(player, card, land_subtype);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, 0, NULL, NULL, 0, NULL) ){
			return can_target(&td) || can_target(&td1);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = instance->number_of_targets = instance->info_slot = 0;
		if( can_target(&td) ){
			if( can_target(&td1) ){
				char buffer[100];
				int pos = 0;
				pos = scnprintf(buffer, 100, get_sleighted_color_text(player, card, " Kill a %s permanent\n", permanent_color));
				scnprintf(buffer + pos, 100-pos, get_hacked_land_text(player, card, " Bounce a %s\n Cancel", land_subtype));
				choice = do_dialog(player, player, card, -1, -1, buffer, 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			if( select_target(player, card, &td, get_sleighted_color_text(player, card, "Select target %s permanent", permanent_color), &(instance->targets[0])) ){
				instance->info_slot = 66+choice;
			}
			else{
				spell_fizzled = 1;
			}
		}
		else if( choice == 1 ){
				if( select_target(player, card, &td1, get_hacked_land_text(player, card, "Select target %s.", land_subtype), &(instance->targets[0])) ){
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

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}


// Cards
static void destroy_green_or_white_at_end_of_combat(int player, int card, int t_player, int t_card)
{
  if (get_color(t_player, t_card) & (get_sleighted_color_test(player, card, COLOR_TEST_GREEN)
									 | get_sleighted_color_test(player, card, COLOR_TEST_WHITE)))
	create_legacy_effect_exe(player, card, LEGACY_EFFECT_STONING, t_player, t_card);
}

int card_abomination(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card))
	instance->destroys_if_blocked |= DIFB_ASK_CARD;

  if (event == EVENT_CHECK_DESTROY_IF_BLOCKED && affect_me(player, card) && !is_humiliated(player, card)
	  && (get_color(attacking_card_controller, attacking_card) & (get_sleighted_color_test(player, card, COLOR_TEST_GREEN)
																  | get_sleighted_color_test(player, card, COLOR_TEST_WHITE))))
	event_result |= 1;

  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
	{
	  if (player == current_turn && (instance->state & STATE_ATTACKING))
		for_each_creature_blocking_me(player, card, destroy_green_or_white_at_end_of_combat, player, card);

	  if (player == 1-current_turn && (instance->state & STATE_BLOCKING))
		for_each_creature_blocked_by_me(player, card, destroy_green_or_white_at_end_of_combat, player, card);
	}

  return 0;
}

int card_acid_rain(int player, int card, event_t event){
	return basic_landtype_killer(player, card, event, SUBTYPE_FOREST);
}

int card_active_volcano(int player, int card, event_t event){
	return leg_color_hoser(player, card, event, COLOR_BLUE, SUBTYPE_ISLAND);
}

int card_adun_oakenshield(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, 0, 1, 0, 1, 1, 0, 0, NULL, NULL) ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(player)){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 1, 0, 1, 1, 0) ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);

			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
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
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
	}

	return 0;
}

// Adventurers' Guildhouse --> uncodeable

int card_aerathi_berserker(int player, int card, event_t event)
{
  // 0x415c90

  /* AErathi Berserker	|2|R|R|R
   * Creature - Human Berserker 2/4
   * Rampage 3 */

  rampage(player, card, event, 3);
  return 0;
}

static void becomes_green(int player, int card, int t_player, int t_card)
{
	change_color(player, card, t_player, t_card, get_sleighted_color_test(player, card, COLOR_TEST_GREEN), CHANGE_COLOR_SET);
}

int card_aisling_leprechaun(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);
	if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card)){
		if (player == current_turn && (instance->state & STATE_ATTACKING))
			for_each_creature_blocking_me(player, card, becomes_green, player, card);

		if (player == 1-current_turn && (instance->state & STATE_BLOCKING))
			for_each_creature_blocked_by_me(player, card, becomes_green, player, card);
	}

	return 0;
}

int card_akron_legionnaire(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_ATTACK_LEGALITY && affected_card_controller == player ){
			if( !(is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) ||
				get_id(affected_card_controller, affected_card) == CARD_ID_AKRON_LEGIONNAIRE)
			  ){
				event_result = 1;
			}
		}
	}

	return 0;
}

static int effect_carpet(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->damage_target_player == player && damage->damage_target_card == -1 && damage->info_slot > 0 ){
			if( damage->targets[4].player == -1 && damage->targets[4].card == -1 ){
				if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) &&
					! check_for_ability(damage->damage_source_player, damage->damage_source_card, KEYWORD_FLYING)
				  ){
					damage->info_slot = 0;
				}
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}


int card_al_abaras_carpet(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		create_legacy_effect(instance->parent_controller, instance->parent_card, &effect_carpet);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(5), 0, NULL, NULL);
}

int card_alabaster_potion(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.extra = damage_card;
	td1.required_type = 0;

	target_definition_t td2;
	default_target_definition(player, card, &td2, 0);
	td2.zone = TARGET_ZONE_PLAYERS;
	td2.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int rvalue = generic_spell(player, card, event, GS_DAMAGE_PREVENTION | GS_CAN_TARGET | GS_X_SPELL, &td1, NULL, 1, NULL);
		if( rvalue ){
			return 99;
		}
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td2, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = instance->number_of_targets = 0;
		if( (played_for_free(player, card) || is_token(player, card)) && ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->targets[1].player = 0;
		}
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->targets[1].player = x_value;
			int choice = 0;
			if( generic_spell(player, card, EVENT_CAN_CAST, GS_DAMAGE_PREVENTION | GS_CAN_TARGET | GS_X_SPELL, &td1, NULL, 1, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Gain X life\n Prevent X damage\n Cancel", 1);
			}
			if( choice == 2 ){
				spell_fizzled = 1;
				return 0;
			}
			instance->info_slot = 66+choice;
		}
		if( instance->info_slot == 66 ){
			pick_target(&td2, "TARGET_PLAYER");
		}
		if( instance->info_slot == 67 ){
			pick_target(&td1, "TARGET_DAMAGE");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == 66 && valid_target(&td2) ){
			gain_life(instance->targets[0].player, instance->targets[1].player);
		}
		if( instance->info_slot == 67 ){
			if( valid_target(&td1) ){
				card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				if( target->info_slot <= instance->targets[1].player ){
					target->info_slot = 0;
				}
				else{
					target->info_slot-=instance->targets[1].player;
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_alchors_tomb(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			change_color(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
						1<<choose_a_color(player, get_deck_color(player, 1-player)), CHANGE_COLOR_SET);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_PERMANENT");
}

static int effect_all_hallows_eve(int player, int card, event_t event)
{
  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  int iid = get_internal_card_id_from_csv_id(CARD_ID_ALL_HALLOWS_EVE);
	  int exiledpos = find_iid_in_rfg(player, iid);
	  if (exiledpos == -1)
		{
		  kill_card(player, card, KILL_REMOVE);
		  return -1;
		}

	  remove_counter(player, card, COUNTER_SCREAM);
	  if (!count_counters(player, card, COUNTER_SCREAM))
		{
			from_exile_to_graveyard(player, exiledpos);
			test_definition_t test;
			default_test_definition(&test, TYPE_CREATURE);
			APNAP(p, {new_reanimate_all(p, -1, p, &test, REANIMATE_DEFAULT);};);
			kill_card(player, card, KILL_REMOVE);
			return -1;
		}
	}

  return 0;
}

int card_all_hallows_eve(int player, int card, event_t event)
{
  /* All Hallow's Eve	|2|B|B
   * Sorcery
   * Exile ~ with two scream counters on it.
   * At the beginning of your upkeep, if ~ is exiled with a scream counter on it, remove a scream counter from it. If there are no more scream counters on it,
   * put it into your graveyard and each player returns all creature cards from his or her graveyard to the battlefield. */

  if (event == EVENT_CAN_CAST)
	return basic_spell(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int legacy = create_legacy_effect(player, card, &effect_all_hallows_eve);
	  add_counters(player, legacy, COUNTER_SCREAM, 2);
	  kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}

int card_amrou_kithkin(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_BLOCK_LEGALITY && attacking_card_controller == player && attacking_card == card ){
			if( get_power(affected_card_controller, affected_card) > 2 ){
				event_result = 1;
			}
		}
	}

	return 0;
}

int card_angelic_voices(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player ){
			int good = 1;
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) ){
					if( ! is_what(player, i, TYPE_ARTIFACT) && !(get_color(player, i) & get_sleighted_color_test(player, card, COLOR_TEST_WHITE)) ){
						good = 0;
						break;
					}
				}
			}
			event_result+=good;
		}
	}

	return global_enchantment(player, card, event);
}

int card_angus_mackenzie(int player, int card, event_t event){
	// original code : 00406250

	/* Angus Mackenzie	|G|W|U
	 * Legendary Creature - Human Cleric 2/2
	 * |G|W|U, |T: Prevent all combat damage that would be dealt this turn. Activate this ability only before the combat damage step. */

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && current_phase < PHASE_FIRST_STRIKE_DAMAGE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, 0, 0, 1, 1, 0, 1, 0, NULL,NULL);
	}

	if(event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, 0, 0, 1, 1, 0, 1, 0, NULL,NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		fog_effect(instance->parent_controller, instance->parent_card);
	}

	return 0;
}

int card_anti_magic_aura(int player, int card, event_t event){

	// "cannot be target of spell" approximated as "Shroud", for now
	card_instance_t* instance = get_card_instance(player, card);
	if( in_play(player, card) && instance->damage_target_player > -1 ){
		cannot_be_enchanted(instance->damage_target_player, instance->damage_target_card, event);
	}
	return generic_aura(player, card, event, player, 0, 0, KEYWORD_SHROUD, 0, 0, 0, 0);
}

int card_arboria(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  enchant_world(player, card, event);

  // Approximation: Creatures can't attack you or planeswalkers you control unless you're unsafe.  (Shouldn't affect planeswalkers.)
  if (event == EVENT_ATTACK_LEGALITY && instance->targets[1 - affected_card_controller].player <= 0)
	event_result = 1;

  // Reset "safeness" each turn
  if (event == EVENT_BEGIN_TURN)
	instance->targets[current_turn].player = 0;

  // Keep track of if a player played a spell on their turn
  if (trigger_condition == TRIGGER_SPELL_CAST && event == EVENT_END_TRIGGER
	  && affect_me(player, card) && trigger_cause_controller == current_turn)
	instance->targets[current_turn].player = 1;

  /* Approximation: Did a nontoken permanent enter the battlefield under a player's control?  (Should look at who put it on the battlefield, not whose control
   * it came under.) */
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && event == EVENT_END_TRIGGER
	  && affect_me(player, card) && trigger_cause_controller == current_turn
	  && is_what(trigger_cause_controller, trigger_cause, TYPE_PERMANENT)
	  && !is_token(trigger_cause_controller, trigger_cause))
	instance->targets[current_turn].player = 1;

  return global_enchantment(player, card, event);
}

int card_arcades_sabboth(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	basic_upkeep(player, card, event, 0, 0, 1, 1, 0, 1);

	if( event == EVENT_TOUGHNESS && affected_card_controller == player && ! is_tapped(affected_card_controller, affected_card) &&
		! is_attacking(affected_card_controller, affected_card) && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  ){
		event_result+=2;
	}

	return generic_shade(player, card, event, 0, MANACOST_W(1), 0, 1, 0, 0);
}

int card_arena_of_the_ancients(int player, int card, event_t event){

	/* Arena of the Ancients	|3
	 * Artifact
	 * Legendary creatures don't untap during their controllers' untap steps.
	 * When ~ enters the battlefield, tap all legendary creatures. */

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		if (!check_battlefield_for_id(ANYBODY, CARD_ID_LEYLINE_OF_SINGULARITY)){
			this_test.subtype = SUBTYPE_LEGEND;
		}
		new_manipulate_all(player, card, ANYBODY, &this_test, ACT_TAP);
	}

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && ! is_humiliated(player, card) &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
		(has_subtype(affected_card_controller, affected_card, SUBTYPE_LEGEND) ||
		 check_battlefield_for_id(ANYBODY, CARD_ID_LEYLINE_OF_SINGULARITY))
	  ){
		card_instance_t *instance= get_card_instance(affected_card_controller, affected_card);
		instance->untap_status &= ~3;
	}

	return 0;
}

int card_avoid_fate(int player, int card, event_t event){

	/* Avoid Fate	|G
	 * Instant
	 * Counter target instant or Aura spell that targets a permanent you control. */
	if(event == EVENT_CAN_CAST ){
		int result = card_counterspell(player, card, event);
		if( result > 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_INSTANT);

			test_definition_t this_test2;
			default_test_definition(&this_test2, TYPE_ENCHANTMENT);
			this_test2.subtype = SUBTYPE_AURA;

			if (new_make_test_in_play(card_on_stack_controller, card_on_stack, -1, &this_test) ||
				new_make_test_in_play(card_on_stack_controller, card_on_stack, -1, &this_test2)
			   ){
				card_instance_t *spell = get_card_instance(card_on_stack_controller, card_on_stack);
				int i = 0;
				for(i=0; i<spell->number_of_targets; i++){
					if( spell->targets[i].player == player && spell->targets[i].card != -1 ){
						return result;
					}
				}
			}
		}
	}
	if( (event == EVENT_CAST_SPELL && affect_me(player, card)) || event == EVENT_RESOLVE_SPELL ){
		return card_counterspell(player, card, event);
	}
	return 0;
}

int card_axelrod_gunnarson(int player, int card, event_t event){

	if( sengir_vampire_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);
		instance->number_of_targets = 0;

		// All the life gains and damage done are in separate 1-point packets.  Only target once as an interface convenience, though.
		int num = instance->targets[11].card;
		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
		} else {
			instance->number_of_targets = 0;
		}
		int i;
		for (i = 0; i < num; ++i){
			gain_life(player, 1);
			if (instance->number_of_targets == 1){
				damage_player(instance->targets[0].player, instance->targets[11].card, player, card);
			}
		}
	}

	return 0;
}

int card_ayesha_tanaka(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  check_legend_rule(player, card, event);

  target_definition_t td;
  counter_activated_target_definition(player, card, &td, TYPE_ARTIFACT);

  if (event == EVENT_CAN_ACTIVATE)
	{
	  return (can_counter_activated_ability(player, card, event, &td)
			  && !is_tapped(player, card)
			  && !is_sick(player, card)
			  && has_mana_for_activated_ability(player, card, MANACOST_X(0))) ? 99 : 0;
	}

  if (event == EVENT_ACTIVATE)
	{
	  instance->number_of_targets = 0;
	  if (cast_counter_activated_ability(player, card, 0))
		tap_card(player, card);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (validate_counter_activated_ability(player, card, &td, 0))
		{
		  ldoubleclicked = 0;
		  charge_mana_while_resolving(player, card, event, instance->targets[0].player, COLOR_WHITE, 1);
		  if (spell_fizzled == 1)
			raw_counter_activated_ability(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

// azure drake --> vanilla

static const char* damage_from_sorcery_source(int who_chooses, int player, int card){
	card_instance_t* dmg = get_card_instance(player, card);
	if ( dmg->internal_card_id == damage_card ){
		int good = dmg->targets[3].player & TYPE_SORCERY;
		if( in_play(dmg->damage_source_player, dmg->damage_source_card) ){
			good = is_what(dmg->damage_source_player, dmg->damage_source_card, TYPE_SORCERY);
		}
		if( good ){
			return NULL;
		}
	}

	return "must be damage from sorcery source";
}

int card_backdraft(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	// Approximation
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_EFFECT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)damage_from_sorcery_source;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			damage_player(target->damage_source_player, round_down_value(target->info_slot), player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td, "TARGET_DAMAGE", 1, NULL);
}

int card_backfire(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 && ! is_humiliated(player, card) ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if (event == EVENT_DEAL_DAMAGE){
			card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
			if (damage->internal_card_id == damage_card && damage->damage_source_card == c && damage->damage_source_player == p &&
				damage->damage_target_card == -1
			  ){
				if( damage->damage_target_player == player ){
					int amount = damage->info_slot;
					if( amount < 1 ){
						amount = get_card_instance(p, c)->targets[16].player;
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
					damage_player(p, instance->targets[1].player, player, card);
					instance->targets[1].player = 0;
			}
			else if (event == EVENT_END_TRIGGER){
					instance->targets[1].player = 0;
			}
		}
	}

	return disabling_aura(player, card, event);
}

// barbary apes --> vanilla

int card_barktooth_warbeard(int player, int card, event_t event){
	// Also code for Jasmine Boreal, Jedit Ojanen, Jerrard of the Closed Fist, Kasimir the Lone Wolf, Lady Orca, Ramirez DePietro, Sir Shandlar of Eberyn,
	// Sivitri Scarzam, The Lady of the Mountain, Tobias Andrion, Torsten Von Ursus,
	check_legend_rule(player, card, event);
	return 0;
}

int card_bartel_runeaxe(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	vigilance(player, card, event);
	return 0;
}

int card_beasts_of_bogardan(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		protection_from_red(player, card, event);
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.type_flag = F1_NO_TOKEN;
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			int good = check_battlefield_for_special_card(player, card, 1-player, 0, &this_test);
			event_result+=good;
		}
	}

	return 0;
}

int card_black_mana_battery(int player, int card, event_t event)
{
  // 0x4235E0
  return mana_battery(player, card, event, COLOR_BLACK);
}

int card_blazing_effigy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_DEAL_DAMAGE){
		card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
		if (damage->internal_card_id == damage_card && damage->damage_target_card == card && damage->damage_target_player == player &&
				damage->info_slot > 0
		  ){
			if( damage->display_pic_int_id == CARD_ID_BLAZING_EFFIGY ){
				if( instance->targets[1].player < 0 ){
					instance->targets[1].player = 0;
				}
				instance->targets[1].player+=damage->info_slot;
			}
		}
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			damage_target0(player, card, 3+(instance->targets[1].player > 0 ? instance->targets[1].player : 0));
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_blight2(int player, int card, int event){
	if( ! (IS_AURA_EVENT(player, card, event) || event == EVENT_TAP_CARD) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t* instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		if( event == EVENT_TAP_CARD && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			kill_card(instance->damage_target_player, instance->damage_target_card, KILL_DESTROY);
		}
	}
	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_blue_mana_battery(int player, int card, event_t event)
{
  // 0x423560
  return mana_battery(player, card, event, COLOR_BLUE);
}

int card_blood_lust(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CHECK_PUMP ){
		if ( has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id) ){
			if( instance->targets[0].player > -1 && instance->targets[0].card > -1 ){
				pumpable_power[player] += 4;
				pumpable_toughness[player] -= (get_toughness(instance->targets[0].player, instance->targets[0].card) > 4 ? 4 : get_toughness(instance->targets[0].player, instance->targets[0].card)-1);

			}
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if (event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		int tp = get_toughness(instance->targets[0].player, instance->targets[0].card) > 4 ? -4 : -(get_toughness(instance->targets[0].player, instance->targets[0].card)-1);
		pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 4, tp);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_boomerang(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
}

int card_boris_devilboon(int player, int card, event_t event){
	/* Boris Devilboon	|3|B|R
	 * Legendary Creature - Zombie Wizard 2/2
	 * |2|B|R, |T: Put a 1/1 |Sblack and |Sred Demon creature token named Minor Demon onto the battlefield. */

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_MINOR_DEMON);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XBR(2, 1, 1), 0, NULL, NULL);
}

int card_bronze_horse(int player, int card, event_t event){
	// Not 100% accurate, but oh well...
	if( ! is_humiliated(player, card) && event == EVENT_PREVENT_DAMAGE ){
		card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
		if (damage->internal_card_id == damage_card && damage->damage_target_card == card && damage->damage_target_player == player &&
			damage->info_slot > 0
		  ){
			if( (damage->targets[3].player & TYPE_SPELL) && !(damage->targets[3].player & TYPE_PERMANENT) && count_subtype(player, TYPE_CREATURE, -1) > 1 ){
				// targets[5]-targets[9] of the damage_card are a direct copy of the damage source first 5 targets
				int k;
				for(k=5; k<10; k++){
					if( damage->targets[k].player == player && damage->targets[k].card == card ){
						damage->info_slot = 0;
						break;
					}
				}
			}
		}
	}
	return 0;
}

static int brine_hag_set_pt_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 && graveyard_from_play(instance->targets[1].player, instance->targets[1].card, event) ){
		disable_other_pt_setting_effects_attached_to_me(instance->damage_target_player, instance->damage_target_card);
		remove_status(player, card, STATUS_INVISIBLE_FX);
		add_status(player, card, STATUS_CONTROLLED);
	}

	if( instance->damage_target_player > -1 && check_status(player, card, STATUS_CONTROLLED) ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_POWER && affect_me(p, c) ){
			event_result += (0-get_base_power(p, c));
		}
		if( event == EVENT_TOUGHNESS && affect_me(p, c) ){
			event_result += (2 - get_base_toughness(p, c));
		}
	}

	if( event == EVENT_CLEANUP && ! check_status(player, card, STATUS_CONTROLLED) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int create_brine_hag_legacy_effect(int player, int card, int t_player, int t_card){
	int leg = create_targetted_legacy_effect(player, card, &brine_hag_set_pt_legacy, t_player, t_card);
	if (leg != -1){
		card_instance_t* legacy = get_card_instance(player, leg);
		legacy->targets[1].player = player;
		legacy->targets[1].card = card;
		legacy->number_of_targets = 2;
		add_status(player, leg, STATUS_INVISIBLE_FX);
	}
	return leg;
}

int card_brine_hag(int player, int card, event_t event){

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->info_slot > 0 ){
			if( damage->damage_target_card == card && damage->damage_target_player == player){
				create_brine_hag_legacy_effect(player, card, damage->damage_source_player, damage->damage_source_card);
			}
		}
	}

	return 0;
}

int card_carrion_ants(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_X(1), 1, 1, 0, 0);
}

// cat warriors --> vanilla

// cathedral of serra --> uncodeable

int card_caverns_of_despair(int player, int card, event_t event){
	if( event == EVENT_ATTACK_LEGALITY && count_attackers(player) > 1 ){
		event_result = 1;
	}
	if( event == EVENT_BLOCK_LEGALITY && count_blockers(player, event) > 1 ){
		event_result = 1;
	}
	return enchant_world(player, card, event);
}

int card_chain_lightning(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		damage_target0(player, card, 3);
		if( has_mana(instance->targets[0].player, COLOR_RED, 2) && can_legally_play_iid(instance->targets[0].player, instance->internal_card_id) ){
			if( do_dialog(instance->targets[0].player, player, card, -1, -1, " Copy Chain Lightning\n Pass", 0) == 0 ){
				charge_mana(instance->targets[0].player, COLOR_RED, 2);
				if( spell_fizzled != 1 ){
					copy_spell(instance->targets[0].player, get_id(player, card));
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}


int card_chains_of_mephistopheles(int player, int card, event_t event){

	/* Chains of Mephistopheles	|1|B
	 * Enchantment
	 * If a player would draw a card except the first one he or she draws in his or her draw step each turn, that player discards a card instead. If the player
	 * discards a card this way, he or she draws a card. If the player doesn't discard a card this way, he or she puts the top card of his or her library into
	 * his or her graveyard. */


	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_humiliated(player, card) ){
		if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && instance->targets[1].player != 66 && !suppress_draw ){
			if( event == EVENT_TRIGGER){
				event_result |= 2u;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					if( instance->targets[8+reason_for_trigger_controller].player < 0 ){
						instance->targets[8+reason_for_trigger_controller].player = 0;
					}
					instance->targets[8+reason_for_trigger_controller].player++;
					if( instance->targets[8+reason_for_trigger_controller].player > 1 ){
						if( hand_count[reason_for_trigger_controller] > 0 ){
							discard(reason_for_trigger_controller, 0, player);
							instance->targets[1].player = 66;
							draw_cards(reason_for_trigger_controller, 1);
							instance->targets[1].player = 0;
						}
						else{
							mill(reason_for_trigger_controller, 1);
						}
						suppress_draw = 1;
					}
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[8].player = 0;
		instance->targets[9].player = 0;
	}

	return global_enchantment(player, card, event);
}

int card_chromium(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	basic_upkeep(player, card, event, 0, 1, 1, 0, 0, 1);

	rampage(player, card, event, 2);

	return 0;
}

int card_cleanse(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
		new_manipulate_all(player, card, ANYBODY, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_clergy_of_the_holy_nimbus(int player, int card, event_t event)
{
  // If ~ would be destroyed, regenerate it.
  // |1: ~ can't be regenerated this turn. Only any opponent may activate this ability.
  holy_nimbus(player, card, event, 1);
  return 0;
}

int card_cocoon(int player, int card, event_t event)
{
  /* Cocoon	|G
   * Enchantment - Aura
   * Enchant creature you control
   * Enchanted creature doesn't untap during your untap step if ~ has a pupa counter on it.
   * When ~ enters the battlefield, tap enchanted creature and put three pupa counters on ~.
   * At the beginning of your upkeep, remove a pupa counter from ~. If you can't, sacrifice it, put a +1/+1 counter on enchanted creature, and that creature gains flying. */

  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player, c = instance->damage_target_card;

  if (event == EVENT_UNTAP && affect_me(p, c) && count_counters(player, card, COUNTER_PUPA))
	does_not_untap(p, c, event);

  if (comes_into_play(player, card, event))
	{
	  add_counters(player, card, COUNTER_PUPA, 3);
	  tap_card(p, c);
	}

  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  if (count_counters(player, card, COUNTER_PUPA))
		remove_counter(player, card, COUNTER_PUPA);
	  else
		{
		  add_counter(p, c, COUNTER_P1_P1);
		  pump_ability_until_eot(player, card, p, c, 0,0, KEYWORD_FLYING,SP_KEYWORD_DOES_NOT_END_AT_EOT);
		  kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) )
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;

	return targeted_aura(player, card, event, &td, "ASHNODS_BATTLEGEAR");	// Select target creature you control
}

int card_concordant_crossroads(int player, int card, event_t event)
{
  // 0x416220

  /* Concordant Crossroads	|G
   * World Enchantment
   * All creatures have haste. */

  boost_subtype(player, card, event, -1, 0,0, 0,SP_KEYWORD_HASTE, BCT_INCLUDE_SELF);
  return enchant_world(player, card, event);
}

int card_cosmic_horror(int player, int card, event_t event){
	if( basic_upkeep_unpaid(player, card, event, MANACOST_XB(3, 3)) ){
		damage_player(player, 7, player, card);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_craw_giant(int player, int card, event_t event)
{
  // 0x415ca0

  /* Craw Giant	|3|G|G|G|G
   * Creature - Giant 6/4
   * Trample
   * Rampage 2 */

  /* Frost Giant	|3|R|R|R
   * Creature - Giant 4/4
   * Rampage 2 */

  /* Wolverine Pack	|2|G|G
   * Creature - Wolverine 2/4
   * Rampage 2 */

  rampage(player, card, event, 2);
  return 0;
}

int card_crevasse(int player, int card, event_t event){
	landwalk_disabling_card(player, card, event, PB_MOUNTAINWALK_DISABLED);
	return global_enchantment(player, card, event);
}

// crimson kobolds --> vanilla

int card_crimson_manticore(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		damage_target0(player, card, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_CREATURE");
}

// Crookshank Kobolds --> vanilla

int card_cyclopean_mummy(int player, int card, event_t event){
	if (event == EVENT_GRAVEYARD_FROM_PLAY && in_play(player, card) && affect_me(player, card) ){
		card_instance_t* instance = get_card_instance(player, card);
		if( instance->kill_code >= KILL_DESTROY && instance->kill_code <= KILL_SACRIFICE ){
			instance->kill_code = KILL_REMOVE;
		}
	}
	return 0;
}

int card_davenant_archer(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		damage_target0(player, card, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_dakkon_blackblade(int player, int card, event_t event){
	/* Dakkon Blackblade	|2|W|U|U|B
	 * Legendary Creature - Human Warrior 100/100
	 * ~'s power and toughness are each equal to the number of lands you control. */
	/* Molimo, Maro-Sorcerer	|4|G|G|G
	 * Legendary Creature - Elemental 100/100
	 * Trample
	 * ~'s power and toughness are each equal to the number of lands you control. */

	check_legend_rule(player, card, event);

	if( (event == EVENT_TOUGHNESS || event == EVENT_POWER) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result+=count_subtype(player, TYPE_LAND, -1);
	}

	return 0;
}

int card_deadfall(int player, int card, event_t event){
	landwalk_disabling_card(player, card, event, PB_FORESTWALK_DISABLED);
	return global_enchantment(player, card, event);
}

int card_demonic_torment(int player, int card, event_t event){

	card_instance_t* instance;

	if( (instance = in_play(player, card)) && instance->damage_target_player != -1 ){
		cannot_attack(instance->damage_target_player, instance->damage_target_card, event);

		card_instance_t* damage = combat_damage_being_prevented(event);
		if( damage &&
			damage->damage_source_player == instance->damage_target_player && damage->damage_source_card == instance->damage_target_card
		  ){
			damage->info_slot = 0;
		}
	}

	return vanilla_aura(player, card, event, 1-player);
}

int card_disharmony(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
			remove_from_combat(instance->targets[0].player, instance->targets[0].card);
			if( instance->targets[0].player != player ){
				gain_control_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target attacking creature.", 1, NULL);
}

int card_divine_intervention(int player, int card, event_t event)
{
  /* Divine Intervention	|6|W|W	0x200d667
   * Enchantment
   * ~ enters the battlefield with two intervention counters on it.
   * At the beginning of your upkeep, remove an intervention counter from ~.
   * When you remove the last intervention counter from ~, the game is a draw. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_INTERVENTION, 2);
  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	remove_counter(player, card, COUNTER_INTERVENTION);

  if (event == EVENT_STATIC_EFFECTS && in_play(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (count_counters(player, card, COUNTER_INTERVENTION) > 0)
		inst->info_slot = CARD_ID_DIVINE_INTERVENTION;
	  else if (inst->info_slot == CARD_ID_DIVINE_INTERVENTION)	// had counters previously; doesn't anymore
		{
		  if (is_humiliated(player, card))
			inst->info_slot = 0;	// so it doesn't trigger later when it stops being humiliated
		  else
			lose_the_game(ANYBODY);
		}
	}

  if (event == EVENT_SHOULD_AI_PLAY)
	ai_modifier += 24 * (life[HUMAN] - life[AI]);

  return global_enchantment(player, card, event);
}

int card_divine_transformation(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 3, 3, 0, 0, 0, 0, 0);
}

int card_divine_offering(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = get_cmc(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				gain_life(player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 1, NULL);
}

int card_dream_coat(int player, int card, event_t event){

	/* Dream Coat	|U
	 * Enchantment - Aura
	 * Enchant creature
	 * |0: Enchanted creature becomes the color or colors of your choice. Activate this ability only once each turn. */

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( instance->targets[1].card > -1 && event == EVENT_SET_COLOR && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result = instance->targets[1].card;
		}

		if( event == EVENT_CLEANUP ){
			instance->targets[2].player = 0;
		}

		if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST0, 0, NULL, NULL);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int clr = 1<<get_deck_color(player, 1-player);
			if( player != AI ){
				clr = 0;
				while( 1 ){
						int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE,
											"Black", !(clr & COLOR_TEST_BLACK), 1,
											"Blue", !(clr & COLOR_TEST_BLUE), 1,
											"Green", !(clr & COLOR_TEST_GREEN), 1,
											"Red", !(clr & COLOR_TEST_RED), 1,
											"White", !(clr & COLOR_TEST_WHITE), 1);
						if( choice ){
							clr |= (1<<choice);
						}
						else{
							break;
						}
				}
			}
			get_card_instance(instance->parent_controller, instance->parent_card)->targets[1].card = clr;
		}
	}

	return vanilla_aura(player, card, event, player);
}

int card_dwarven_song(int player, int card, event_t event){
	return song(player, card, event, COLOR_TEST_RED);
}

int card_elder_land_wurm(int player, int card, event_t event){

  /* Ruling for Elder Land Wurm: 10/4/2004: Another effect can give Elder Land Wurm defender after it has lost it. Blocking again will cause it to lose that
   * instance of defender too. */
  if (current_turn == 1-player && blocking(player, card, event) && !is_humiliated(player, card) && check_for_ability(player, card, KEYWORD_DEFENDER))
	{
	  // Probably should look for other instances of the this-loses-defender effect and get rid of them first to reduce clutter.
	  int leg = pump_ability_until_eot(player, card, player, card, 0, 0, KEYWORD_DEFENDER, SP_KEYWORD_DOES_NOT_END_AT_EOT);
	  if (leg != -1)
		get_card_instance(player, leg)->targets[4].player = 0;	// remove keyword

	}

  return 0;
}

int card_elder_spawn(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_ISLAND;
		if( new_can_sacrifice_as_cost(player, card, &this_test) ){
			int ai_choice = 0;
			if( (! can_attack(player, card) && life[player] > 11) ||
				check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test) < count_upkeeps(player)
			  ){
				ai_choice = 1;
			}
			int choice = do_dialog(player, player, card, -1, -1, " Pay upkeep\n Pass", ai_choice);
			if( choice == 0 ){
				if( new_sacrifice(player, card, player, 0, &this_test) ){
					kill--;
				}
			}
		}
		if( kill == 1 ){
			damage_player(player, 6, player, card);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_BLOCK_LEGALITY && attacking_card_controller == player && attacking_card == card && ! is_humiliated(player, card) ){
		if( get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_RED) ){
			event_result = 1;
		}
	}

	return 0;
}

int card_elven_riders(int player, int card, event_t event)
{
  // 0x4c41a0
  if (event == EVENT_BLOCK_LEGALITY
	  && attacking_card_controller == player && attacking_card == card
	  && !check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING)
	  && !has_subtype(affected_card_controller, affected_card, SUBTYPE_WALL)
	  && !is_humiliated(player, card))
	event_result = 1;

  return 0;
}

int card_emerald_dragonfly(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_G(2), 0, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_enchanted_being(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage
	  && damage->damage_target_card == card && damage->damage_target_player == player
	  && (damage->targets[3].player & TYPE_CREATURE) // probably redundant to status check
	  && is_enchanted(damage->damage_source_player, damage->damage_source_card)
	  && ! is_humiliated(player, card))
	damage->info_slot = 0;

  return 0;
}

static const char* ee_legal_target(int who_chooses, int player, int card){
	card_instance_t* instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		int type = get_type(instance->damage_target_player, instance->damage_target_card);
		if( type & (TYPE_LAND | TYPE_CREATURE) ){
			if( call_card_function(player, card, EVENT_CAN_MOVE_AURA) ){
				return NULL;
			}
		}
	}
	return "cannot move this aura";
}

int card_enchantment_alteration(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
	td.required_subtype = SUBTYPE_AURA;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int)ee_legal_target;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			set_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_ENCHANTMENT_ALTERATION);
			if( player == 0 ){
				set_special_flags3(instance->targets[0].player, instance->targets[0].card, SF3_MOVING_AURA_PLAYER0_SELECT_TARGET);
			}
			if( call_card_function(instance->targets[0].player, instance->targets[0].card, EVENT_CAN_MOVE_AURA) ){
				call_card_function(instance->targets[0].player, instance->targets[0].card, EVENT_MOVE_AURA);
				call_card_function(instance->targets[0].player, instance->targets[0].card, EVENT_RESOLVE_MOVING_AURA);
			}
			remove_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_ENCHANTMENT_ALTERATION);
			if( player == 0 ){
				remove_special_flags3(instance->targets[0].player, instance->targets[0].card, SF3_MOVING_AURA_PLAYER0_SELECT_TARGET);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target Aura enchanting a creature or a land", 1, NULL);
}

int card_energy_tap(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int cmc = get_cmc(instance->targets[0].player, instance->targets[0].card);
			tap_card(instance->targets[0].player, instance->targets[0].card);
			produce_mana(player, COLOR_COLORLESS, cmc);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target untapped creature you control.", 1, NULL);
}

int is_land_destroyer(int player, int card){
	return 0;
}

static const char* land_killer_spell(int who_chooses, int player, int card){
	if( is_land_destroyer(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		int k;
		for(k=0; k<instance->number_of_targets; k++){
			if( instance->targets[k].player == who_chooses && instance->targets[k].card != -1 ){
				if( is_what(instance->targets[k].player, instance->targets[k].card, TYPE_LAND) ){
					return NULL;
				}
			}
		}
	}
	return "must be a spell that directly destroy one or more of your lands";
}

int card_equinox(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( ! IS_GAA_EVENT(event)  ){
			return 0;
		}
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		target_definition_t td1;
		counterspell_target_definition(player, card, &td1, TYPE_SPELL);
		td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td1.extra = (int)land_killer_spell;

		if( event == EVENT_CAN_ACTIVATE ){
			card_instance_t *enc = get_card_instance(p, c);
			if( validate_target(p, c, &td1, 0) ){
				set_flags_when_spell_is_countered(player, card, enc->targets[0].player, enc->targets[0].card);
				kill_card(enc->targets[0].player, enc->targets[0].card, KILL_BURY);
			}
		}
		return granted_generic_activated_ability(player, card, p, c, event, GAA_UNTAPPED | GAA_SPELL_ON_STACK, MANACOST0, 0, &td1, "TARGET_SPELL");
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_SPELL ){
		return targeted_aura(player, card, event, &td, "TARGET_LAND");
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_AURA, &td, "TARGET_LAND", 1, NULL);
}

int card_eternal_warrior(int player, int card, event_t event){

	return generic_aura(player, card, event, player, 0, 0, 0, SP_KEYWORD_VIGILANCE, 0, 0, 0);
}

int card_eureka(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		char msg[100] = "Choose a permanent to put into play.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, msg);

		int cancel0 = 0;
		int cancel1 = 0;
		while( cancel0 + cancel1 > -2){
				cancel0 = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
				cancel1 = new_global_tutor(1-player, 1-player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_evil_eye_of_orms_by_gore(int player, int card, event_t event){

	if( event == EVENT_ATTACK_LEGALITY && affected_card_controller == player && ! is_humiliated(player, card) &&
		! has_subtype(affected_card_controller, affected_card, SUBTYPE_EYE)
	  ){
		event_result = 1;
	}

	if( event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( ! has_subtype(affected_card_controller, affected_card, SUBTYPE_WALL) ){
				event_result = 1;
			}
		}
	}

	return 0;
}

int card_fallen_angel(int player, int card, event_t event){
	return generic_husk(player, card, event, TYPE_CREATURE, 2, 1, 0, 0);
}

int card_fallen_star(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		if( internal_rand(100) < 75 ){
			int affected_creatures = MIN(count_subtype(ANYBODY, TYPE_CREATURE, -1), internal_rand(4)+1);
			int found_creatures = 0;
			while( found_creatures < affected_creatures ){
					int p = 0;
					for(p=0; p<2; p++){
						int c;
						for(c=0; c<active_cards_count[p]; c++){
							if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) ){
								if( internal_rand(100)+1 > 50 ){
									add_state(p, c, STATE_TARGETTED);
									found_creatures++;
								}
							}
							if( found_creatures == affected_creatures ){
								break;
							}
						}
						if( found_creatures == affected_creatures ){
							break;
						}
					}
			}
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.state = STATE_TARGETTED;
			APNAP(p, {new_damage_all(player, card, p, 3, 0, &this_test);};);
			int p = 0;
			for(p=0; p<2; p++){
				int c;
				for(c=0; c<active_cards_count[p]; c++){
					if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) && check_state(p, c, STATE_TARGETTED) ){
						tap_card(p, c);
						remove_state(p, c, STATE_TARGETTED);
					}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static void tap_and_maze_of_ith(int player, int card, int t_player, int t_card){
	tap_card(t_player, t_card);
	maze_of_ith_effect(player, card, t_player, t_card);
}

int card_feint(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = STATE_ATTACKING;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			maze_of_ith_effect(player, card, instance->targets[0].player, instance->targets[0].card);
			for_each_creature_blocking_me(instance->targets[0].player, instance->targets[0].card, tap_and_maze_of_ith, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_field_of_dreams(int player, int card, event_t event)
{
  // Players play with the top card of their libraries revealed.

  if (event == EVENT_CAN_CAST)
	return 1;

  card_instance_t *instance = get_card_instance(player, card);
  int p;

  if (event == EVENT_RESOLVE_SPELL)
	for (p = 0; p <= 1; ++p)
	  {
		instance->targets[p].player = player;
		if (deck_ptr[p][0] == -1)
		  instance->targets[p].card = -1;
		else
		  instance->targets[p].card = create_card_name_legacy(player, card, cards_data[deck_ptr[p][0]].id);
	  }

  if (affect_me(player, card) && instance->targets[0].player >= 0)
	for (p = 0; p <= 1; ++p)
	  if (instance->targets[p].card == -1)
		{
		  if (deck_ptr[p][0] != -1)	// was empty, now has a card
			instance->targets[p].card = create_card_name_legacy(instance->targets[p].player, card, cards_data[deck_ptr[p][0]].id);
		}
	  else
		{
		  if (deck_ptr[p][0] == -1)	// had a card, now empty
			{
			  kill_card(instance->targets[p].player, instance->targets[p].card, KILL_REMOVE);
			  instance->targets[p].card = -1;
			}
		  else	// had a card, still does
			{
			  card_instance_t* legacy = get_card_instance(instance->targets[p].player, instance->targets[p].card);
			  int csvid = cards_data[deck_ptr[p][0]].id;
			  legacy->info_slot = -csvid - 1;	// update csvid if it's different
			}
		}

  return enchant_world(player, card, event);
}

int card_fire_sprites(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_G(1), 0, NULL, NULL) ){
			return can_produce_mana(player, card);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_G(1)) ){
			mana_producer(player, card, event);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card) ){
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_G(1), 0, NULL, NULL) ){
			mana_producer(player, card, event);
		}
	}

	return 0;
}

int firestorm_phoenix_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn != instance->targets[0].player && event == EVENT_CLEANUP ){
		remove_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_BLOOD);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_firestorm_phoenix(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST && count_counters(player, card, COUNTER_BLOOD) ){
		infinite_casting_cost();
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		remove_counter(player, card, COUNTER_BLOOD);
	}

	int owner, position;
	if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) && find_in_owners_graveyard(player, card, &owner, &position)){
		int iid = get_grave(owner)[position];
		obliterate_card_in_grave(owner, position);
		int card_added = add_card_to_hand(owner, iid);
		add_counter(player, card_added, COUNTER_BLOOD);
		int legacy = create_legacy_effect(player, card, &firestorm_phoenix_legacy);
		get_card_instance(player, legacy)->targets[0].player = player;
		get_card_instance(player, legacy)->targets[0].card = card_added;
		add_status(player, legacy, STATUS_INVISIBLE_FX);
	}

	return 0;
}

int card_flash_counter(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_INSTANT | TYPE_INTERRUPT);

	return counterspell(player, card, event, &td, 0);
}

int card_flash_flood(int player, int card, event_t event){
	return leg_color_hoser(player, card, event, COLOR_RED, SUBTYPE_MOUNTAIN);
}

int card_floral_spuzzem(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_humiliated(player, card) && event == EVENT_DECLARE_BLOCKERS && (instance->state & STATE_ATTACKING) && is_unblocked(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT);
		td.allowed_controller =  1-player;

		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_ARTIFACT") ){
			negate_combat_damage_this_turn(player, card, player, card, 0);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_force_spike(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_forethought_amulet(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player &&  damage->info_slot > 2 ){
				if( (damage->targets[3].player & TYPE_SPELL) && !(damage->targets[3].player & TYPE_CREATURE) ){
					damage->info_slot = 2;
				}
			}
		}
	}

	basic_upkeep(player, card, event, MANACOST_X(3));

	return 0;
}

int card_fortified_area(int player, int card, event_t event)
{
  // 0x4b8b90
  boost_creature_type(player, card, event, SUBTYPE_WALL, 1, 0, KEYWORD_BANDING, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
  return global_enchantment(player, card, event);
}

int card_gabriel_angelfire(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		int choice = do_dialog(player, player, card, -1, -1,
								" Gabriel gains flying\n Gabriel gains first strike\n Gabriel gains trample\n Gabriel gains rampage: 3", internal_rand(4));
		instance->info_slot = 66 + choice;
	}

	if(event == EVENT_ABILITIES && affect_me(player, card) && instance->info_slot < 69 && instance->info_slot > 65 ){
		if( instance->info_slot == 66){
			event_result |= KEYWORD_FLYING;
		}

		if( instance->info_slot == 67){
			event_result |= KEYWORD_FIRST_STRIKE;
		}

		if( instance->info_slot == 68){
			event_result |= KEYWORD_TRAMPLE;
		}
	}

	if( instance->info_slot == 69 ){
		rampage(player, card, event, 3);
	}

	return 0;
}

int card_gaseous_form(int player, int card, event_t event){
	card_instance_t* instance;

	if( (instance = in_play(player, card)) && instance->damage_target_player != -1 && ! is_humiliated(player, card) ){
		card_instance_t* damage = combat_damage_being_prevented(event);
		if( damage && ((damage->token_status & STATUS_COMBAT_DAMAGE) || (damage->token_status & STATUS_FIRST_STRIKE_DAMAGE)) ){
			if( (damage->damage_source_player == instance->damage_target_player && damage->damage_source_card == instance->damage_target_card) ||
				(damage->damage_target_player == instance->damage_target_player && damage->damage_target_card == instance->damage_target_card)
		      ){
				damage->info_slot = 0;
			}
		}
	}

	return vanilla_aura(player, card, event, 1-player);
}

int card_gauntlets_of_chaos(int player, int card, event_t event){

	/* Gauntlets of Chaos	|5
	 * Artifact
	 * |5, Sacrifice ~: Exchange control of target artifact, creature, or land you control and target permanent an opponent controls that shares one of those
	 * types with it. If those permanents are exchanged this way, destroy all Auras attached to them. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND);
	td1.allowed_controller = player;
	td1.preferred_controller = player;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND);
	td2.allowed_controller = 1-player;
	td2.preferred_controller = 1-player;


	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(5), 0, &td1, "TARGET_PERMANENT") ){
			return can_target(&td2);
		}
	}

	if(event == EVENT_ACTIVATE){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(5)) ){
			if( new_pick_target(&td1, "Select target artifact, creature, or land you control.", 0, 1 | GS_LITERAL_PROMPT) ){
				td2.required_type = get_type(instance->targets[0].player, instance->targets[0].card) & (TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND);
				if( can_target(&td2) ){
					if( new_pick_target(&td2, "Select target artifact, creature, or land an opponent controls.", 1, 1 | GS_LITERAL_PROMPT) ){
						instance->info_slot = instance->internal_card_id;
						kill_card(player, card, KILL_SACRIFICE);
					}
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( validate_target(player, card, &td1, 0) &&  validate_target(player, card, &td2, 1) ){
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && has_subtype(i, count, SUBTYPE_AURA) ){
							card_instance_t *this = get_card_instance( i, count );
							if( (this->damage_target_player == instance->targets[0].player && this->damage_target_card == instance->targets[0].card) ||
								(this->damage_target_player == instance->targets[1].player && this->damage_target_card == instance->targets[1].card)
							  ){
								kill_card(i, count, KILL_DESTROY);
							}
						}
						count--;
				}
			}
			int fake = add_card_to_hand(player, instance->info_slot);
			add_state(player, fake, STATE_INVISIBLE);
			exchange_control_of_target_permanents(player, fake, instance->targets[0].player, instance->targets[0].card,
													instance->targets[1].player, instance->targets[1].card);
			obliterate_card(player, fake);
		}
	}

	return 0;
}

int card_ghosts_of_the_damned(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

static int giant_slug_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn == player && instance->damage_target_player > -1 && upkeep_trigger(player, card, event) ){
		int ai_choice = get_deck_color(player, 1-player)-1;
		int key = 1 << do_dialog(player, instance->damage_target_player, instance->damage_target_card, -1, -1, " Swampwalk\n Islandwalk\n Forestwalk\n Mountainwalk\n Plainswalk", ai_choice);
		pump_ability_until_eot(instance->damage_target_player, instance->damage_target_card, instance->damage_target_player, instance->damage_target_card,
						0, 0, key, 0);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_giant_slug(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &giant_slug_legacy,
										instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_X(5), 0, NULL, NULL);
}

int card_giant_strenght(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 2, 2, 0, 0, 0, 0, 0);
}

int card_giant_turtle(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player == 66 && ! is_humiliated(player, card) ){
		cannot_attack(player, card, event);
	}

	if( event == EVENT_DECLARE_ATTACKERS && is_attacking(player, card) && ! is_humiliated(player, card) ){
		instance->targets[1].player = 66;
	}

	if( current_turn == 1-player && event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}
	return 0;
}

static int glyph_of_delusion_wall_card = 0;
static const char* target_is_blocked_by_glyph_of_delusion_wall_card(int who_chooses, int player, int card)
{
	if (player == current_turn && is_blocking(glyph_of_delusion_wall_card, card)){
		return NULL;
	} else {
		return "not blocked by Wall";
	}
}
int card_glyph_of_delusion(int player, int card, event_t event){

	/* Glyph of Delusion	|U
	 * Instant
	 * Put X glyph counters on target creature that target Wall blocked this turn, where X is the power of that blocked creature. The creature gains "This
	 * creature doesn't untap during your untap step if it has a glyph counter on it" and "At the beginning of your upkeep, remove a glyph counter from this
	 * creature." */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td_wall;
	default_target_definition(player, card, &td_wall, TYPE_CREATURE);
	td_wall.preferred_controller = player;
	td_wall.required_state = TARGET_STATE_BLOCKING;
	td_wall.required_subtype = SUBTYPE_WALL;

	target_definition_t td_blocked_by_wall;
	default_target_definition(player, card, &td_blocked_by_wall, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td_wall);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if (pick_next_target_noload(&td_wall, "Select target blocking Wall.")){
			ASSERT(instance->targets[0].player == 1-current_turn);
			glyph_of_delusion_wall_card = instance->targets[0].card;

			// This part won't be validated on resolution, but it's supposed to be "blocked by target wall this turn" anyway.
			td_blocked_by_wall.extra = (int32_t)target_is_blocked_by_glyph_of_delusion_wall_card;
			td_blocked_by_wall.special = TARGET_SPECIAL_EXTRA_FUNCTION;

			if (!can_target(&td_blocked_by_wall)){
				spell_fizzled = 1;
			} else {
				pick_next_target_noload(&td_blocked_by_wall, "Select target creature blocked by target Wall.");
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td_wall) && validate_target(player, card, &td_blocked_by_wall, 1) ){
			add_counters(instance->targets[1].player, instance->targets[1].card, COUNTER_GLYPH,
						 get_power(instance->targets[1].player, instance->targets[1].card));
			gains_doesnt_untap_while_has_a_counter_and_remove_a_counter_at_upkeep(player, card,
																				  instance->targets[1].player, instance->targets[1].card, COUNTER_GLYPH);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int glyph_of_destruction_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].card > -1 ){
		if( current_phase < PHASE_MAIN2 ){
			modify_pt_and_abilities(instance->targets[0].player, instance->targets[0].card, event, 10, 0, 0);
		}
		if( event == EVENT_PREVENT_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card ){
				if( damage->damage_target_card == card && damage->damage_target_player == player &&
					damage->info_slot > 0
				  ){
					damage->info_slot = 0;
				}
			}
		}
		if( eot_trigger(player, card, event) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_glyph_of_destruction(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_BLOCKING;
	td.required_subtype = SUBTYPE_WALL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, &glyph_of_destruction_legacy, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target blocking Wall you control.", 1, NULL);
}

static int glyph_of_doom_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].card > -1 ){
		if( end_of_combat_trigger(player, card, event, 2) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

static void add_glyph_of_doom_legacy(int player, int card, int t_player, int t_card){
	create_targetted_legacy_effect(player, card, &glyph_of_doom_legacy, t_player, t_card);
}

int card_glyph_of_doom(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_BLOCKING;
	td.required_subtype = SUBTYPE_WALL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if (valid_target(&td)){
			for_each_creature_blocked_by_me(instance->targets[0].player, instance->targets[0].card, add_glyph_of_doom_legacy, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target blocking Wall you control.", 1, NULL);
}

static int glyph_of_life_legacy(int player, int card, event_t event)
{
  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && (damage->targets[3].player & TYPE_CREATURE)
	  && is_attacking(damage->damage_source_player, damage->damage_source_card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (damage->damage_target_card == instance->damage_target_card
		  && damage->damage_target_player == instance->damage_target_player)
		gain_life(player, damage->info_slot);
	}

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_glyph_of_life(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_BLOCKING;
	td.required_subtype = SUBTYPE_WALL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, &glyph_of_life_legacy, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target Wall.", 1, NULL);
}

static int glyph_of_reincarnation_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( end_of_combat_trigger(player, card, event, 2) ){
		instance->damage_target_player = instance->damage_target_card = -1;
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		if( count_graveyard_by_type(instance->targets[0].player, TYPE_CREATURE) > 0 ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			int selected = new_select_a_card(player, instance->targets[0].player, TUTOR_FROM_GRAVE, 1, AI_MIN_CMC, -1, &this_test);
			reanimate_permanent(instance->targets[0].player, -1, instance->targets[0].player, selected, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_glyph_of_reincarnation(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_BLOCKING;
	td.required_subtype = SUBTYPE_WALL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			char marked[2][151] = {{0}};

			mark_each_creature_blocked_by_me(instance->targets[0].player, instance->targets[0].card, marked);

			APNAP(p,
					{
						int c;
						for (c = 0; c < active_cards_count[p]; ++c){
							if (marked[p][c] && in_play(p, c)){
								create_targetted_legacy_effect(player, card, &glyph_of_reincarnation_legacy, p, c);
							}
						}
					}
			);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target blocking Wall", 1, NULL);
}

int card_gosta_dirk(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	landwalk_disabling_card(player, card, event, PB_ISLANDWALK_DISABLED);
	return 0;
}

int card_gravity_sphere(int player, int card, event_t event)
{
  // 0x416280

  /* Gravity Sphere	|2|R
   * World Enchantment
   * All creatures lose flying. */

  if (event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && in_play(player, card) && in_play(affected_card_controller, affected_card)
	  && !is_humiliated(player, card))
	event_result &= ~KEYWORD_FLYING;

  return enchant_world(player, card, event);
}

int card_great_defender(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CHECK_PUMP ){
		if ( has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id) ){
			if( instance->targets[0].player > -1 && instance->targets[0].card > -1 ){
				pumpable_toughness[player] += get_cmc(instance->targets[0].player, instance->targets[0].card);

			}
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if (event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, get_cmc(instance->targets[0].player, instance->targets[0].card));
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_great_wall(int player, int card, event_t event){
	landwalk_disabling_card(player, card, event, PB_PLAINSWALK_DISABLED);
	return global_enchantment(player, card, event);
}

static const char* red_black_damage_source(int who_chooses, int player, int card, int targeting_player, int targeting_card){
	card_instance_t* dmg = get_card_instance(player, card);
	if ( dmg->internal_card_id == damage_card ){
		int clr = dmg->initial_color;
		if( in_play(dmg->damage_source_player, dmg->damage_source_card) ){
			clr = get_color(dmg->damage_source_player, dmg->damage_source_card);
		}
		if( clr & (get_sleighted_color(targeting_player, targeting_card, COLOR_TEST_RED) |
					get_sleighted_color(targeting_player, targeting_card, COLOR_TEST_BLACK))
		  ){
			return NULL;
		}
	}

	return EXE_STR(0x739060);//",color"
}

int card_greater_realm_of_protection(int player, int card, event_t event){

	if( in_play(player, card) && IS_GAA_EVENT(event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_EFFECT);
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION | TARGET_SPECIAL_DAMAGE_PLAYER;
		td.extra = (int32_t)red_black_damage_source;

		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				get_card_instance(instance->targets[0].player, instance->targets[0].card)->info_slot = 0;
			}
		}

		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_XW(1, 1), 0, &td, "TARGET_DAMAGE");
	}

	return global_enchantment(player, card, event);
}

int card_greed(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}
	return generic_activated_ability(player, card, event, 0, MANACOST_B(1), 2, NULL, NULL);
}

int card_green_mana_battery(int player, int card, event_t event)
{
  // 0x4235A0
  return mana_battery(player, card, event, COLOR_GREEN);
}

int card_gwendlyn_di_corci(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			discard(instance->targets[0].player, DISC_RANDOM, player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_IN_YOUR_TURN, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_halfdane(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( !is_humiliated(player, card) && current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.special = TARGET_SPECIAL_NOT_ME;
		td.allow_cancel = 0;

		if( can_target(&td) ){
			upkeep_trigger_ability(player, card, event, player);
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.special = TARGET_SPECIAL_NOT_ME;
		td.allow_cancel = 0;

		if( can_target(&td) && new_pick_target(&td, "Select another target creature.", 0, GS_LITERAL_PROMPT) ){
			real_set_pt(player, card, player, card, get_power(instance->targets[0].player, instance->targets[0].card),
						get_toughness(instance->targets[0].player, instance->targets[0].card), 4);
		}
	}

	return 0;
}

static int hammerheim_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( instance->targets[0].player > -1 ){
		if( event == EVENT_ABILITIES && affect_me(instance->targets[0].player, instance->targets[0].card) ){
			int i;
			for(i=0; i<5; i++){
				if( event_result & (1<<i) ){
					event_result &= ~(1<<i);
				}
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_hammerheim(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = instance->number_of_targets = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Remove landwalk\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST0) && pick_target(&td, "TARGET_CREATURE") ){
				tap_card(player, card);
				instance->info_slot = 1;
			}
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
			parent->info_slot = 0;
			if( valid_target(&td) ){
				create_targetted_legacy_effect(player, instance->parent_card, &hammerheim_legacy, instance->targets[0].player, instance->targets[0].card);
			}
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

static int legacy_ht(int player, int card, event_t event){

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		int warriors = count_permanents_by_type(player, TYPE_LAND);
		generate_tokens_by_id(player, card, CARD_ID_SAND_WARRIOR, warriors);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_hazezon_tamar(int player, int card, event_t event){
	/* Hazezon Tamar	|4|R|G|W
	 * Legendary Creature - Human Warrior 2/4
	 * When ~ enters the battlefield, put X 1/1 Sand Warrior creature tokens that are |Sred, |Sgreen, and |Swhite onto the battlefield at the beginning of your next upkeep, where X is the number of lands you control at that time.
	 * When Hazezon leaves the battlefield, exile all Sand Warriors. */

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &legacy_ht );
	}

	if( leaves_play(player, card, event) ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && is_token(i, count) ){
						if( has_subtype(i, count, SUBTYPE_WARRIOR) && has_subtype(i, count, SUBTYPE_SAND) ){
							kill_card(i, count, KILL_REMOVE);
						}
					}
					count--;
			}
		}
	}

	return 0;
}

// headless horseman --> vanilla
int card_heavens_gate(int player, int card, event_t event){
	return song(player, card, event, COLOR_TEST_WHITE);
}

int card_hell_swarm(int player, int card, event_t event){

	if (event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, ANYBODY, -1, -1, 0, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_hells_caretaker(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_ONLY_ON_UPKEEP | GAA_SACRIFICE_CREATURE, MANACOST0, 0, NULL, NULL) ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
				return ! graveyard_has_shroud(player);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature to sacrifice.");

		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &this_test);
			if (!sac){
				cancel = 1;
				return 0;
			}

			get_card_instance(player, card)->number_of_targets = 0;
			int selected = select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 0);
			if( selected != -1 ){
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
				tap_card(player, card);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		}
	}

	return 0;
}

int card_hellfire(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_BLACK;
		this_test.color_flag = DOESNT_MATCH;
		int result = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
		int result2 = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test);
		ai_modifier+=((result2-result)*5);
		ai_modifier-=life[player]-(result+result2+3) < 6 ? 15 : 0;
	}

	if(event == EVENT_RESOLVE_SPELL){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.color = COLOR_TEST_BLACK;
		this_test.color_flag = DOESNT_MATCH;
		int result = new_manipulate_all(player, card, ANYBODY, &this_test, KILL_DESTROY);

		damage_player(player, 3+result, player, card);

		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

// holy day --> fog

int card_horn_of_deafening(int player, int card, event_t event){
	// original code : 00405F70

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		negate_combat_damage_this_turn(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

// hornet cobra

int card_horror_of_horrors(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select a %s to sacrifice.", SUBTYPE_SWAMP));
	this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_SWAMP);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
	td1.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td1.special = TARGET_SPECIAL_REGENERATION;
	}
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_REGENERATION, MANACOST0, 0, &td1, "TARGET_CREATURE");
		if( new_can_sacrifice_as_cost(player, card, &this_test) ){
			return result;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &this_test);
		if (!sac){
			cancel = 1;
			return 0;
		}
		if( new_pick_target(&td1, get_sleighted_color_text(player, card, "Select target %s creature to regenerate", COLOR_BLACK), 0, 1 | GS_LITERAL_PROMPT) ){
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
		else{
			state_untargettable(BYTE2(sac), BYTE3(sac), 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_hunding_gjornersen(int player, int card, event_t event){
	// also code for Marhault Elsdragon
	check_legend_rule(player, card, event);
	rampage(player, card, event, 1);
	return 0;
}

int card_hyperion_blacksmith(int player, int card, event_t event)
{
  // 0x429f90

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	// |T: You may tap or untap target artifact an opponent controls.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller = td.preferred_controller = 1-player;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
		twiddle(player, card, 0);

	int rval = generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "HYPERION_BLACKSMITH");

	if (event == EVENT_ACTIVATE && player == AI && cancel != 1)
		ai_modifier_twiddle(player, card, 0);

	return rval;
}

int card_ichneumon_druid(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAST_SPELL && affected_card_controller == 1-player && is_what(affected_card_controller, affected_card, TYPE_INSTANT) ){
		instance->info_slot++;
	}

	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == trigger_cause_controller && ! is_humiliated(player, card) ){
		if( trigger_cause_controller == 1-player && is_what(trigger_cause_controller, trigger_cause, TYPE_INSTANT) && instance->info_slot > 1 ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					damage_player(1-player, 4, player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL || event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_immolation(int player, int card, event_t event){
	return generic_aura(player, card, event, 1-player, 2, -2, 0, 0, 0, 0, 0);
}

static int imprison_ability(int player, int card){
	if( has_mana(player, COLOR_COLORLESS, 1) ){
		if( do_dialog(player, player, card, -1, -1, " Activate Imprision\n Pass", 0) == 0 ){
			charge_mana(player, COLOR_COLORLESS, 1);
			if( spell_fizzled != 1 ){
				return 1;
			}
		}
	}
	return 0;
}

int card_imprison(int player, int card, event_t event){
	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_DECLARE_ATTACKERS && is_attacking(p, c) ){
			if( imprison_ability(player, card) ){
				remove_state(p, c, STATE_ATTACKING);
				tap_card(p, c);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
		}

		if( blocking(p, c, event) ){
			if( imprison_ability(player, card) ){
				remove_state(p, c, STATE_BLOCKING);
				remove_state(p, c, STATE_UNKNOWN8000);
				tap_card(p, c);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
		}
		/* It's possibile to find the activation card of (p,c) and kill it in rensponse of EVENT_PLAY_ABILITY ?
		if( event == EVENT_PLAY_ABILITY && affect_me(p, c) ){
			if( imprison_ability(player, card) ){
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
		}
		*/

	}
	return disabling_aura(player, card, event);
}

int card_in_the_eye_of_chaos(int player, int card, event_t event){

	enchant_world(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		!is_humiliated(player, card) && !is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) &&
		is_what(trigger_cause_controller, trigger_cause, TYPE_INSTANT | TYPE_INTERRUPT)
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int kill = 1;
				if( has_mana(trigger_cause_controller, COLOR_COLORLESS, get_cmc(trigger_cause_controller, trigger_cause)) ){
					charge_mana(trigger_cause_controller, COLOR_COLORLESS, get_cmc(trigger_cause_controller, trigger_cause));
					if( spell_fizzled != 1 ){
						kill = 0;
					}
				}
				if( kill == 1 ){
					kill_card(trigger_cause_controller, trigger_cause, KILL_SACRIFICE);
				}
		}
	}
	return global_enchantment(player, card, event);
}

int card_indestructible_aura(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			prevent_all_damage_to_target(player, card, instance->targets[0].player, instance->targets[0].card, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

static void destroy_nonwall_at_end_of_combat(int player, int card, int t_player, int t_card)
{
  if (!has_subtype(t_player, t_card, SUBTYPE_WALL))
	create_legacy_effect_exe(player, card, LEGACY_EFFECT_STONING, t_player, t_card);
}

static void destroy_creature_at_end_of_combat(int player, int card, int t_player, int t_card)
{
  create_legacy_effect_exe(player, card, LEGACY_EFFECT_STONING, t_player, t_card);
}

int card_infernal_medusa(int player, int card, event_t event)
{
  // 0x4ca9e0
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card))
	instance->destroys_if_blocked |= (current_turn == player ? DIFB_DESTROYS_NONWALLS : DIFB_DESTROYS_ALL);

  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
	{
	  if (player == current_turn && (instance->state & STATE_ATTACKING))
		for_each_creature_blocking_me(player, card, destroy_nonwall_at_end_of_combat, player, card);

	  if (player == 1-current_turn && (instance->state & STATE_BLOCKING))
		for_each_creature_blocked_by_me(player, card, destroy_creature_at_end_of_combat, player, card);
	}

  return 0;
}

static int infinite_autority_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( end_of_combat_trigger(player, card, event, 2) ){
		instance->damage_target_player = instance->damage_target_card = -1; //Unattach the legacy or will be removed along the creature
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		instance->targets[2].card = 66;
	}

	if( eot_trigger(player, card, event) ){
		if( instance->targets[2].card == 66 && in_play(instance->targets[1].player, instance->targets[1].card) ){
			add_1_1_counters(instance->targets[1].player, instance->targets[1].card, 1);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

static void create_infinite_autority_legacy(int player, int card, int t_player, int t_card)
{
	if ( get_toughness(t_player, t_card) < 4 ){
		int legacy = create_targetted_legacy_effect(player, card, &infinite_autority_legacy, t_player, t_card);
		card_instance_t *instance = get_card_instance(player, card);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[1].player = instance->damage_target_player;
		leg->targets[1].card = instance->damage_target_card;
		instance->number_of_targets = 2;
	}
}

int card_infinite_autority(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( current_turn == player && is_attacking(p, c) ){
			for_each_creature_blocking_me(p, c, create_infinite_autority_legacy, player, card);
		}
		if( current_turn != player && blocking(p, c, event) ){
			for_each_creature_blocked_by_me(p, c, create_infinite_autority_legacy, player, card);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

static int get_permanents_color(int player, int type, int flag){

	int result = 0;
	int count = 0;

	while( count < active_cards_count[player] ){
			if( in_play(player, count) ){
				if( flag != is_what(player, count, type) ){
					result |= get_color(player, count);
				}
			}
			count++;
	}

	return result;
}

int card_invoke_prejudice(int player, int card, event_t event){

	/* Invoke Prejudice	|U|U|U|U
	 * Enchantment
	 * Whenever an opponent casts a creature spell that doesn't share a color with a creature you control, counter that spell unless its controller pays |X,
	 * where X is its converted mana cost. */

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		!is_humiliated(player, card) && trigger_cause_controller == 1-player && !is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) &&
		is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) &&
		!( get_color(trigger_cause_controller, trigger_cause) & get_permanents_color(player, TYPE_CREATURE, 0))
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int kill = 1;
				if( has_mana(1-player, COLOR_COLORLESS, get_cmc(trigger_cause_controller, trigger_cause)) ){
					charge_mana(1-player, COLOR_COLORLESS, get_cmc(trigger_cause_controller, trigger_cause));
					if( spell_fizzled != 1 ){
						kill = 0;
					}
				}
				if( kill == 1 ){
					real_counter_a_spell(player, card, trigger_cause_controller, trigger_cause);
				}
		}
	}

	return global_enchantment(player, card, event);
}

int card_ivory_guardians(int player, int card, event_t event){
	if( ! is_humiliated(player, card) && in_play(player, card) ){
		protection_from_red(player, card, event);
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && get_id(affected_card_controller, affected_card) == CARD_ID_IVORY_GUARDIANS ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.type_flag = F1_NO_TOKEN;
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_RED);
			if( check_battlefield_for_special_card(player, card, 1-affected_card_controller, 0, &this_test) ){
				event_result++;
			}
		}
	}

	return 0;
}

int card_jacques_le_vert(int player, int card, event_t event)
{
  /* Jacques le Vert	|1|R|G|W
   * Legendary Creature - Human Warrior 3/2
   * |SGreen creatures you control get +0/+2. */

	check_legend_rule(player, card, event);

	boost_creature_by_color(player, card, event, get_sleighted_color_test(player, card, COLOR_TEST_GREEN), 0, 2, 0, BCT_CONTROLLER_ONLY);

	return 0;
}

static int johan_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		if( ! in_play(instance->targets[1].player, instance->targets[1].card) || is_tapped(instance->targets[1].player, instance->targets[1].card) ){
			kill_card(instance->targets[2].player, instance->targets[2].card, KILL_REMOVE);
			kill_card(player, card, KILL_REMOVE);
		}
	}
	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_johan(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && (instance->info_slot & 2) && !is_humiliated(player, card) ){
		event_result = 1;
	}

	if( beginning_of_combat(player, card, event, player, -1) && ! is_tapped(player, card) ){
		int choice = do_dialog(player, player, card, -1, -1, " Give Vigilance to other creatures\n Pass", is_tapped(player, card) );
		if (choice == 0){
			instance->info_slot |= 2;
			int count = active_cards_count[player];
			while( count > -1 ){
					if( count != card && in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
						int legacy = pump_ability_until_eot(player, card, player, count, 0, 0, 0, SP_KEYWORD_VIGILANCE);
						if( legacy > -1 ){
							int l2 = create_targetted_legacy_effect(player, card, &johan_legacy, player, count);
							card_instance_t *leg = get_card_instance(player, l2);
							leg->targets[1].player = player;
							leg->targets[1].card = card;
							leg->targets[2].player = player;
							leg->targets[2].card = legacy;
							add_status(player, l2, STATUS_INVISIBLE_FX);
						}
					}
					count--;
			}
		}
	}

	return 0;
}

int card_jovial_evil(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			damage_player(instance->targets[0].player,
							check_battlefield_for_special_card(player, card, instance->targets[0].player, CBFSC_GET_COUNT, &this_test)*2,
							player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, NULL, NULL, 1, NULL);
}

static void juxtapose_effect(int player, int card, int sel_type){

	card_instance_t *instance = get_card_instance( player, card );

	if( count_permanents_by_type(player, sel_type) > 0 &&
		count_permanents_by_type(1-player, sel_type) > 0
	  ){
		int i;
		int max_cmc[2] = {-1, -1};
		int dup[2] = {0, 0};
		int trg[2] = {-1, -1};
		for(i=0; i<2; i++){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && is_what(i, count, sel_type) ){
						if( get_cmc(i, count) > max_cmc[i] ){
							max_cmc[i] = get_cmc(i, count);
							trg[i] = count;
							dup[i] = 0;
						}
						else if( get_cmc(i, count) == max_cmc[i] ){
								dup[i] = 1;
						}
					}
					count++;
			}
		}
		if( dup[0] > 0 || dup[1] > 0 ){
			i = 0;
			for(i=0; i<2; i++){
				if( dup[i] > 0 ){
					manipulate_all(player, card, i, sel_type, 0, 0, 0, 0, 0, 0, 0, max_cmc[i], 3, ACT_MAKE_UNTARGETTABLE);

					target_definition_t td;
					default_target_definition(player, card, &td, sel_type);
					td.illegal_abilities = 0;
					td.preferred_controller = i;
					td.allowed_controller = i;
					td.who_chooses = i;
					td.allow_cancel = 0;
					if( sel_type == TYPE_CREATURE ){
						if( pick_target(&td, "TARGET_CREATURE") ){
							trg[i] = instance->targets[0].card;
						}
					}
					else if( sel_type == TYPE_LAND ){
							if( pick_target(&td, "TARGET_LAND") ){
								trg[i] = instance->targets[0].card;
							}
					}
					else if( sel_type == TYPE_ENCHANTMENT ){
							if( pick_target(&td, "TARGET_ENCHANTMENT") ){
								trg[i] = instance->targets[0].card;
							}
					}
					else if( sel_type == TYPE_ARTIFACT ){
							if( pick_target(&td, "TARGET_ARTIFACT") ){
								trg[i] = instance->targets[0].card;
							}
					}
					else{
						if( pick_target(&td, "TARGET_PERMANENT") ){
							trg[i] = instance->targets[0].card;
						}
					}

					manipulate_all(player, card, i, sel_type, 0, 0, 0, 0, 0, 0, 0, max_cmc[i], 3, ACT_REMOVE_UNTARGETTABLE);
					dup[i] = 0;
				}
			}
		}

		exchange_control_of_target_permanents(player, card, player, trg[player], 1-player, trg[1-player]);
	}
}

int card_juxtapose(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			juxtapose_effect(player, card, TYPE_CREATURE);
			juxtapose_effect(player, card, TYPE_ARTIFACT);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_karakas(int player, int card, event_t event){
	// original code : 0x40D220

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_LEGEND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = instance->number_of_targets = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce W\n Bounce a Legend\n Do nothing", 1);
		}
		if( choice == 0 ){
			if (player == AI && !paying_mana()){
				ai_modifier -= 128;
			}
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
					pick_target(&td, "TARGET_LEGENDARY_CREATURE");
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( valid_target(&td)  ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

// keepers of the faith --> vanilla

int card_kei_takahashi(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;
	td.special = TARGET_SPECIAL_DAMAGE_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( dmg->info_slot < 2 ){
				dmg->info_slot = 0;
			}
			else{
				dmg->info_slot-=2;
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION_CREATURE, MANACOST0, 0, &td, "TARGET_DAMAGE");
}

int card_killer_bees(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_G(1), 1, 1, 0, 0);
}

int card_kismet(int player, int card, event_t event)
{
  // 0x4b56e0

  /* Kismet	|3|W
   * Enchantment
   * Artifacts, creatures, and lands played by your opponents enter the battlefield tapped. */

	permanents_enters_battlefield_tapped(player, card, event, 1-player, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND, NULL);

	return global_enchantment(player, card, event);
}

int card_knowledge_vault(int player, int card, event_t event)
{
  /* Knowledge Vault	|4
   * Artifact
   * |2, |T: Exile the top card of your library face down.
   * |0: Sacrifice ~. If you do, discard your hand, then put all cards exiled with ~ into their owner's hand.
   * When ~ leaves the battlefield, put all cards exiled with ~ into their owner's graveyard. */

  if (leaves_play(player, card, event))
	{
	  int leg = 0, idx = 0, *loc;
	  while ((loc = exiledby_find_any(player, card, &leg, &idx)))
		{
		  int owner = (*loc & 0x80000000) ? 1 : 0;
		  int iid = *loc & ~0x80000000;
		  *loc = -1;
		  int pos = add_card_to_rfg(owner, iid);
		  from_exile_to_graveyard(player, pos);
		}
	}

  if (!IS_GAA_EVENT(event))
	return 0;

  int cuaa = can_use_activated_abilities(player, card);

  enum
  {
	CHOICE_EXILE = 1,
	CHOICE_SACRIFICE
  } choice = DIALOG(player, card, event, DLG_RANDOM,
					"Exile the top card of your library", cuaa, 1, DLG_TAP, DLG_MANA(MANACOST_X(2)),
					"Sacrifice", cuaa && can_sacrifice_this_as_cost(player, card), 1, DLG_MANA(MANACOST_X(0)));

  if (event == EVENT_CAN_ACTIVATE)
	return choice;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_ACTIVATE)
	switch (choice)
	  {
		case CHOICE_EXILE:
		  if (player == AI && current_turn == 1-player && current_phase == PHASE_DISCARD)
			ai_modifier += 48;
		  break;

		case CHOICE_SACRIFICE:
		  ;int ai_mod = 0;
		  if (instance->targets[0].card != -1)
			{
			  ai_mod -= 12 * BYTE0(instance->targets[0].card);
			  ai_mod += 12 * BYTE1(instance->targets[0].card);
			}
		  ai_modifier += (player == AI) ? ai_mod : -ai_mod;
		  instance->targets[0].player = exiledby_detach(player, card);
		  kill_card(player, card, KILL_SACRIFICE);
		  break;
	  }

  if (event == EVENT_RESOLVE_ACTIVATION)
	switch (choice)
	  {
		case CHOICE_EXILE:
		  if (deck_ptr[player][0] != -1)
			{
			  exiledby_remember(instance->parent_controller, instance->parent_card, player, deck_ptr[player][0], NULL, NULL);
			  play_sound_effect(WAV_DESTROY);
			  obliterate_top_card_of_deck(player);

			  // Keep count of cards exiled by each player, for ai modifier
			  if (instance->targets[0].card == -1)
				instance->targets[0].card = 0;

			  if (player == 0)
				++SET_BYTE0(instance->targets[0].card);
			  else
				++SET_BYTE1(instance->targets[0].card);
			}
		  break;

		case CHOICE_SACRIFICE:
			discard_all(player);
		  int leg = 0, idx = 0, *loc;
		  while ((loc = exiledby_find_any(player-2, instance->targets[0].player, &leg, &idx)))
			{
			  int owner = (*loc & 0x80000000) ? 1 : 0;
			  int iid = *loc & ~0x80000000;
			  add_card_to_hand(owner, iid);
			  *loc = -1;
			}

		  exiledby_destroy_detached(player, instance->targets[0].player);
		  break;
	  }

  return 0;
}

int card_kobold_drill_sergeant(int player, int card, event_t event)
{
  boost_creature_type(player, card, event, SUBTYPE_KOBOLD, 0, 1, KEYWORD_TRAMPLE, BCT_CONTROLLER_ONLY);
  return 0;
}

int card_kobold_overlord(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_KOBOLD, 0, 0, KEYWORD_FIRST_STRIKE, BCT_CONTROLLER_ONLY);
	return 0;
}

int card_kobold_taskmaster(int player, int card, event_t event)
{
  boost_creature_type(player, card, event, SUBTYPE_KOBOLD, 1, 0, 0, BCT_CONTROLLER_ONLY);
  return 0;
}

// kobold of kher keep --> vanilla

static int kry_shield_damage_prevention(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_source_card == instance->targets[0].card && damage->damage_source_player == instance->targets[0].player &&
				damage->info_slot > 0
			  ){
				int prevent = 1;
				if( instance->targets[1].player > 0 && (instance->targets[1].player & 1) &&
					!(damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE))
				  ){
					prevent = 0;
				}
				if( prevent ){
					damage->info_slot = 0;
				}
			}
		}
	}
	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

static void kry_shield_effect(int player, int card, int t_player, int t_card, int mode){
	int l1 = pump_ability_until_eot(player, card, t_player, t_card, 0, get_cmc(t_player, t_card), 0, 0);
	get_card_instance(player, l1)->targets[1].player = mode;
	int legacy = create_targetted_legacy_effect(player, card, &kry_shield_damage_prevention, t_player, t_card);
	add_status(player, legacy, STATUS_INVISIBLE_FX);
}

int card_kry_shield(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kry_shield_effect(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

int card_lady_caleria(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_lady_evangela(int player, int card, event_t event)
{
  // original code : 0040CD80

  /* Lady Evangela	|W|U|B
   * Legendary Creature - Human Cleric 1/2
   * |W|B, |T: Prevent all combat damage that would be dealt by target creature this turn. */

  check_legend_rule(player, card, event);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  negate_combat_damage_this_turn(player, card, instance->targets[0].player, instance->targets[0].card, 0);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_WB(1,1), 0, &td, "TARGET_CREATURE");
}

int card_land_equilibrium(int player, int card, event_t event){
	if( ! is_humiliated(player, card) && trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player){
		if( count_subtype(player, TYPE_LAND, -1)+1 == count_subtype(1-player, TYPE_LAND, -1) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			if( new_specific_cip(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, &this_test) ){
				impose_sacrifice(player, card, 1-player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}
	return global_enchantment(player, card, event);
}

int card_land_tax(int player, int card, event_t event)
{
	if (landsofcolor_controlled[player][COLOR_ANY] < landsofcolor_controlled[1-player][COLOR_ANY]){
		upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));
	}

	if (event == EVENT_UPKEEP_TRIGGER_ABILITY){
		tutor_basic_lands(player, TUTOR_HAND, 3);
		shuffle(player);
	}

	return global_enchantment(player, card, event);
}


static int effect_lands_edge(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player != -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.zone = TARGET_ZONE_PLAYERS;
		td.illegal_abilities = get_protections_from(p, c);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.zone = TARGET_ZONE_HAND;
		if( player == AI ){
			this_test.type = TYPE_LAND;
		}

		if( event == EVENT_CAN_ACTIVATE ){
			int cless = get_cost_mod_for_activated_abilities(p, c, 0, 0, 0, 0, 0, 0);
			if( has_mana(player, COLOR_COLORLESS, cless) && can_use_activated_abilities(p, c) && hand_count[player] > 0 ){
				if( player == AI ){
					if( can_target(&td) && check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
						return 1;
					}
				}
				else{
					return 1;
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			int cless = get_cost_mod_for_activated_abilities(p, c, 0, 0, 0, 0, 0, 0);
			charge_mana(player, COLOR_COLORLESS, cless);
			if( spell_fizzled != 1 ){
				if( player == AI ){
					int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 && pick_target(&td, "TARGET_PLAYER") ){
						instance->number_of_targets = 1;
						instance->targets[2].player = 66;
						discard_card(player, selected);
					}
				}
				else{
					int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
					if( selected != -1 ){
						int result = is_what(player, selected, TYPE_LAND) ? 66 : 0;
						if( result ){
							if( pick_target(&td, "TARGET_PLAYER") ){
								instance->number_of_targets = 1;
								instance->targets[2].player = result;
								discard_card(player, selected);
							}
						}
						else{
							instance->targets[2].player = result;
							discard_card(player, selected);
						}
					}
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->targets[2].player == 66 && valid_target(&td) ){
				damage_player(instance->targets[0].player, 2, p, c);
			}
		}

		if( leaves_play(p, c, event) ){
			kill_card(player, card, KILL_REMOVE);
		}

	}

	return 0;
}

int card_lands_edge(int player, int card, event_t event ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.zone = TARGET_ZONE_HAND;
	if( player == AI ){
		this_test.type = TYPE_LAND;
	}
	card_instance_t *instance = get_card_instance(player, card);

	enchant_world(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		int fake = add_card_to_hand(1-player, instance->internal_card_id);
		int legacy = create_legacy_activate(1-player, fake, &effect_lands_edge);
		card_instance_t *leg = get_card_instance(1-player, legacy);
		leg->targets[1].player = player;
		leg->targets[1].card = card;
		obliterate_card(1-player, fake);
		hand_count[1-player]--;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( player == AI ){
			if( generic_activated_ability(player, card, event, GAA_DISCARD, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER") ){
				if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
					return 1;
				}
			}
		}
		else{
			return generic_activated_ability(player, card, event, GAA_DISCARD, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( player == AI ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 && pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				instance->targets[2].player = 66;
				discard_card(player, selected);
			}
		}
		else{
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				int result = is_what(player, selected, TYPE_LAND) ? 66 : 0;
				if( result ){
					if( pick_target(&td, "TARGET_PLAYER") ){
						instance->number_of_targets = 1;
						instance->targets[2].player = result;
						discard_card(player, selected);
					}
				}
				else{
					instance->targets[2].player = result;
					discard_card(player, selected);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[2].player == 66 && valid_target(&td) ){
			damage_player(instance->targets[0].player, 2, player, instance->parent_card);
		}
	}

	return global_enchantment(player, card, event);
}

static const char* is_blocking_lw(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_blocking(card, targeting_card))
	return NULL;
  else
	return "must be blocking Lesser Werewolf";
}

static const char* lw_is_blocking_him(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_blocking(targeting_card, card))
	return NULL;
  else
	return "must be blocked by Lessere Werewolf";
}

int card_lesser_werewolf(int player, int card, event_t event){

	/* Lesser Werewolf	|3|B
	 * Creature - Werewolf 2/4
	 * |B: If ~'s power is 1 or more, it gets -1/-0 until end of turn and put a -0/-1 counter on target creature blocking or blocked by ~. Activate this ability
	 * only during the declare blockers step. */

	card_instance_t *instance = get_card_instance(player, card);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller =  1-player;
	td.preferred_controller = 1-player;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)is_blocking_lw;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller =  1-player;
	td1.preferred_controller = 1-player;
	td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td1.extra = (int32_t)lw_is_blocking_him;

	if( event == EVENT_CAN_ACTIVATE ){
		if( current_phase == PHASE_AFTER_BLOCKING && generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL) ){
			if( current_turn == player ){
				return can_target(&td);
			}
			else{
				return can_target(&td1);
			}
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( player == AI && get_power(player, card) < 1 ){
			ai_modifier-=25;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			if( current_turn == player ){
				if( select_target(player, card, &td, "Select target creature blocking Lesser Werewolf", &(instance->targets[0])) ){
					instance->number_of_targets = 1;
				}
				else{
					spell_fizzled = 1;
				}
			}
			else{
				if( select_target(player, card, &td1, "Select target creature blocked by Lesser Werewolf", &(instance->targets[0])) ){
					instance->number_of_targets = 1;
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( ((current_turn == player && valid_target(&td)) || (current_turn != player && valid_target(&td1))) &&
			get_power(instance->parent_controller, instance->parent_card) > 0
		  ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, -1, 0);
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_M0_M1);
		}
	}

	return 0;
}

int card_life_chisel(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, instance->targets[1].card);
	}
	return altar_basic(player, card, event, 24, TYPE_CREATURE);
}

static int effect_life_matrix(int player, int card, event_t event)
{
  if (effect_follows_control_of_attachment(player, card, event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player, c = instance->damage_target_card;

  if (event == EVENT_ABILITIES && affect_me(p, c) && count_counters(p, c, COUNTER_MATRIX))
	event_result |= KEYWORD_REGENERATION;

  if (land_can_be_played & LCBP_REGENERATION)
	{
	  if (event == EVENT_CAN_ACTIVATE)
		return CAN_ACTIVATE0(p, c) && count_counters(p, c, COUNTER_MATRIX) && can_regenerate(p, c) ? 99 : 0;

	  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(p, c, MANACOST0))
		remove_counter(p, c, COUNTER_MATRIX);

	  if (event == EVENT_RESOLVE_ACTIVATION)
		regenerate_target(p, c);
	}

  return 0;
}
int card_life_matrix(int player, int card, event_t event){

  /* Life Matrix	|4
   * Artifact
   * |4, |T: Put a matrix counter on target creature and that creature gains "Remove a matrix counter from this creature: Regenerate this creature." Activate
   * this ability only during your upkeep. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_MATRIX);

	  // Make sure no effect already attached
	  card_instance_t* inst;
	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if ((inst = in_play(p, c)) && inst->internal_card_id == LEGACY_EFFECT_ACTIVATED && inst->info_slot == (int)effect_life_matrix
			  && inst->damage_target_card == instance->targets[0].card && inst->damage_target_player == instance->targets[0].player)
			return 0;

	  create_targetted_legacy_activate(player, card, &effect_life_matrix, instance->targets[0].player, instance->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_ONLY_ON_UPKEEP|GAA_IN_YOUR_TURN, MANACOST_X(4), 0, &td, "TARGET_CREATURE");
}

int card_lifeblood(int player, int card, event_t event){
	/* Lifeblood	|2|W|W
	 * Enchantment
	 * Whenever |Ha Mountain an opponent controls becomes tapped, you gain 1 life. */
	if( event == EVENT_TAP_CARD && affected_card_controller == 1-player ){
		if( has_subtype(affected_card_controller, affected_card, get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN)) ){
			gain_life(player, 1);
		}
	}

	return global_enchantment(player, card, event);
}

int card_living_plane(int player, int card, event_t event){

	if( event == EVENT_CHANGE_TYPE ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);

		global_type_change(player, card, event, 2, TYPE_CREATURE, &this_test, 1, 1, 0, 0, 0);
	}

	enchant_world(player, card, event);

	return global_enchantment(player, card, event);
}

int card_livonya_silone(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_BLOCK_LEGALITY && !(player_bits[player] & PB_NONSTANDARD_LANDWALK_DISABLED) && ! is_humiliated(player, card) ){
		if( attacking_card_controller == player && attacking_card == card ){
			if( instance->targets[1].player != 66 ){
				int count = 0;
				while( count < active_cards_count[1-player] ){
						if( in_play(1-player, count) && is_what(1-player, count, TYPE_LAND) && is_legendary(1-player, count) ){
							instance->targets[1].player = 66;
							break;
						}
						count++;
				}
			}

			if( instance->targets[1].player == 66 ){
				event_result = 1;
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_lord_magnus(int player, int card, event_t event){
	/* Lord Magnus	|3|G|W|W	0x200d66c
	 * Legendary Creature - Human Druid 4/3
	 * First strike
	 * Creatures with |H2plainswalk can be blocked as though they didn't have |H2plainswalk.
	 * Creatures with |H2forestwalk can be blocked as though they didn't have |H2forestwalk. */

	landwalk_disabling_card(player, card, event, PB_PLAINSWALK_DISABLED | PB_FORESTWALK_DISABLED);
	check_legend_rule(player, card, event);
	return 0;
}

// lost soul --> vanilla

int effect_mana_drain(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( current_turn == player && current_phase == instance->targets[0].player ){
		if( instance->targets[1].card == CARD_ID_CHANCELLOR_OF_THE_TANGLE ){
				produce_mana(player, COLOR_GREEN, 1);
		}
		else {
			produce_mana(player, COLOR_COLORLESS, instance->targets[0].card);
		}
		instance->targets[0].player = -1;
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_mana_drain(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = get_cmc(card_on_stack_controller, card_on_stack);
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0)  ){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);

			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);

			int legacy_card = create_legacy_effect(player, card, &effect_mana_drain );
			card_instance_t *legacy = get_card_instance(player, legacy_card);
			legacy->targets[0].card = instance->info_slot;
			if( current_turn == player && current_phase < PHASE_MAIN2 ){
				legacy->targets[0].player = PHASE_MAIN2;
			}
			else{
				legacy->targets[0].player = PHASE_MAIN1;
			}
			legacy->targets[1].card = get_id(player, card);
		}

		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_mana_matrix(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player &&
			is_what(affected_card_controller, affected_card, TYPE_INSTANT | TYPE_INTERRUPT | TYPE_ENCHANTMENT)
		  ){
			COST_COLORLESS-=2;
		}
	}
	return 0;
}

int card_marble_priest(int player, int card, event_t event)
{
  // All Walls able to block ~ do so.
  subtype_must_block_me(player, card, event, SUBTYPE_WALL, "Choose defending Wall");

  // Prevent all combat damage that would be dealt to ~ by Walls.
  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage
	  && damage->damage_target_player == player && damage->damage_target_card == card
	  && has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_WALL))
	damage->info_slot = 0;

  return 0;
}

int card_master_of_the_hunt(int player, int card, event_t event){
	/* Master of the Hunt	|2|G|G
	 * Creature - Human 2/2
	 * |2|G|G: Put a 1/1 |Sgreen Wolf creature token named Wolves of the Hunt onto the battlefield. It has "bands with other creatures named Wolves of the Hunt." */

	if( event == EVENT_RESOLVE_ACTIVATION){
		generate_token_by_id(player, card, CARD_ID_WOLVES_OF_THE_HUNT);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XG(2, 2), 0, NULL, NULL);
}

static void exchange_life_totals(int player, int t_player){
	if( player == t_player ){
		return;
	}
	int my_life = life[player];
	if( check_battlefield_for_id(player, CARD_ID_LICH) ){
		my_life = 0;
	}
	int his_life = life[t_player];
	if( check_battlefield_for_id(t_player, CARD_ID_LICH) ){
		his_life = 0;
	}
	set_life_total(player, his_life);
	set_life_total(t_player, my_life);
}

int card_mirror_universe(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td)  ){
			exchange_life_totals(player, 1-player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_ONLY_TARGET_OPPONENT | GAA_SACRIFICE_ME | GAA_ONLY_ON_UPKEEP |GAA_IN_YOUR_TURN,
									MANACOST0, 0, &td, "TARGET_PLAYER");
}

int card_moat(int player, int card, event_t event){

	if (event == EVENT_ATTACK_LEGALITY && !(get_card_instance(affected_card_controller, affected_card)->regen_status & KEYWORD_FLYING) &&
		! is_humiliated(player, card))
		++event_result;

	return global_enchantment(player, card, event);
}

int card_mold_demon(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_SWAMP;
		this_test.qty = 2;

		if( ! new_can_sacrifice(player, card, player, &this_test) ){
			ai_modifier-=50;
		}
	}

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_SWAMP;
		this_test.qty = 2;

		int kill_me = 1;
		if( new_can_sacrifice(player, card, player, &this_test) ){
			if( do_dialog(player, player, card, -1, -1, " Sac two Swamps\n Sacrifice Mold Demon", 0) == 0 ){
				if( new_sacrifice(player, card, player, 0, &this_test) ){
					impose_sacrifice(player, card, player, 1, TYPE_PERMANENT, 0, SUBTYPE_SWAMP, 0, 0, 0, 0, 0, -1, 0);
					kill_me = 0;
				}
			}
		}
		if( kill_me ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

// moss monster --> vanilla

// mountain stronghold --> undoable

// mountain yeti --> vanilla

int card_nebuchadnezzar(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && hand_count[instance->targets[0].player] > 0 ){
			int t_player = instance->targets[0].player;
			int *deck = deck_ptr[t_player];
			int id = cards_data[deck[internal_rand(count_deck(t_player)-1)]].id;
			if( player == HUMAN ){
				id = card_from_list(player, 3, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
			if( player == AI ){
				card_ptr_t* c = cards_ptr[ id ];
				char buffer[100];
				scnprintf(buffer, 100, "Opponent named : %s", c->name);
				do_dialog(HUMAN, player, card, -1, -1, buffer, 0);
			}
			int howmany = MIN(instance->info_slot, hand_count[t_player]);
			int initial_hand[2][50];
			int initial_index = 0;
			int ai_hand[2][50];
			int hand_index = 0;
			int count = 0;
			while( count < active_cards_count[t_player] ){
					if( in_hand(t_player, count) ){
						card_instance_t *crd = get_card_instance(t_player, count);
						initial_hand[0][initial_index] = crd->internal_card_id;
						initial_hand[1][initial_index] = count;
						initial_index++;
					}
					count++;
			}
			while( hand_index < howmany && initial_index > -1 ){
					int rnd = initial_index;
					if( howmany < hand_count[t_player] ){
						rnd = internal_rand(initial_index);
					}
					ai_hand[0][hand_index] = initial_hand[0][rnd];
					ai_hand[1][hand_index] = initial_hand[1][rnd];
					int k;
					for(k=rnd; k<initial_index; k++){
						initial_hand[0][k] = initial_hand[0][k+1];
						initial_hand[1][k] = initial_hand[1][k+1];
					}
					hand_index++;
					initial_index--;
			}
			show_deck( HUMAN, ai_hand[0], hand_index, "Target revealed these cards", 0, 0x7375B0 );
			int i;
			for(i=0; i<hand_index; i++){
				if( cards_data[ai_hand[0][i]].id == id ){
					new_discard_card(t_player, ai_hand[1][i], player, 0);
				}
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_IN_YOUR_TURN | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST_X(-1), 0, &td, "TARGET_PLAYER");
}

int card_nether_void(int player, int card, event_t event){

	/* Nether Void	|3|B
	 * World Enchantment
	 * Whenever a player casts a spell, counter it unless its controller pays |3. */

	if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		card_instance_t *instance = get_card_instance(player, card);
		int nether_effect = 1;
		if( has_mana(instance->targets[1].player, COLOR_COLORLESS, 3) ){
			charge_mana(instance->targets[1].player, COLOR_COLORLESS, 3);
			if( spell_fizzled != 1 ){
				nether_effect = 0;
			}
		}
		if( nether_effect ){
			real_counter_a_spell(player, card, instance->targets[1].player, instance->targets[1].card);
		}
	}

	return enchant_world(player, card, event);
}

int card_nicol_bolas2(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	basic_upkeep(player, card, event, 0, 1, 1, 0, 1, 0);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT) ){
		new_discard_all(1-player, player);
	}

	return 0;
}

int card_north_star(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL) ){
			int result = 0;
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_hand(player, i) ){
					int can_play_this_spell = can_legally_play_iid(player, get_card_instance(player, i)->internal_card_id);
					if( can_play_this_spell ){
						int mana_needed = get_updated_casting_cost(player, i, -1, EVENT_CAST_SPELL, get_cmc(player, i));
						if( has_mana_for_activated_ability(player, card, MANACOST_X(4+mana_needed)) ){
							if( can_play_this_spell == 99 ){
								return 99;
							}
							else{
								if( is_what(player, i, TYPE_INSTANT) || can_sorcery_be_played(player, event) ){
									result = 1;
								}
							}
						}
					}
				}
			}
			return result;
		}
	}

	if(event == EVENT_ACTIVATE ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select a card to play with North Star.");

		int pwns[2][hand_count[player]];
		int pwnsc = 0;
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) ){
				int can_play_this_spell = can_legally_play_iid(player, get_card_instance(player, i)->internal_card_id);
				if( can_play_this_spell && (is_what(player, i, TYPE_INSTANT | TYPE_INTERRUPT) || can_sorcery_be_played(player, event))){
					int mana_needed = get_updated_casting_cost(player, i, -1, EVENT_CAST_SPELL, get_cmc(player, i));
					if( has_mana_for_activated_ability(player, card, MANACOST_X(4+mana_needed)) ){
						pwns[0][pwnsc] = get_card_instance(player, i)->internal_card_id;
						pwns[1][pwnsc] = i;
						pwnsc++;
					}
				}
			}
		}
		int selected = select_card_from_zone(player, player, pwns[0], pwnsc, 0, AI_MAX_VALUE, -1, &test);
		if( selected != -1 ){
			int mana_needed = get_updated_casting_cost(player, pwns[1][selected], -1, EVENT_CAST_SPELL, get_cmc(player, pwns[1][selected]));
			if( charge_mana_for_activated_ability(player, card, MANACOST_X(4+mana_needed)) ){
				tap_card(player, card);
				play_card_in_hand_for_free(player, pwns[1][selected]);
				cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
			}
			else {
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}

static const char* is_damaging_me(int who_chooses, int player, int card){
	card_instance_t* dmg = get_card_instance(player, card);
	if ( dmg->internal_card_id == damage_card ){
		if( dmg->damage_target_player == who_chooses && dmg->damage_target_card == -1 ){
			return NULL;
		}
	}

	return "must be damage dealt to you";
}

int card_nova_pentacle(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_EFFECT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)is_damaging_me;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.who_chooses = 1-player;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE){
		int result = generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_DAMAGE_PREVENTION, MANACOST_X(3), 0, &td, "TARGET_DAMAGE");
		if( result && can_target(&td1)  ){
			return 0x63;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(3)) ){
			if( pick_target(&td, "TARGET_DAMAGE") ){
				if( new_pick_target(&td1, "TARGET_CREATURE", 1, 1) ){
					tap_card(player, card);
				}
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			target->damage_target_player = instance->targets[1].player;
			target->damage_target_card = instance->targets[1].card;
		}
	}
	return 0;
}

int card_osai_vultures(int player, int card, event_t event)
{
  // 0x4c6cd0

  // At the beginning of each end step, if a creature died this turn, put a carrion counter on ~.
  if (creatures_dead_this_turn > 0 && (event == EVENT_SHOULD_AI_PLAY || eot_trigger(player, card, event)))
	add_counter(player, card, COUNTER_CARRION);

  // Remove two carrion counters from ~: ~ gets +1/+1 until end of turn.
  if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
	return count_counters(player, card, COUNTER_CARRION) / 2;

  if (event == EVENT_CAN_ACTIVATE && count_counters(player, card, COUNTER_CARRION) < 2)	// Else fall through to generic_shade
	return 0;

  int rval = generic_shade_merge_pt(player, card, event, 0, MANACOST_X(0), 1,1);
  if (event == EVENT_ACTIVATE && cancel != 1)
	remove_counters(player, card, COUNTER_CARRION, 2);

  return rval;
}

int card_palladia_mors(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	basic_upkeep(player, card, event, 0, 0, 0, 1, 1, 1);

	return 0;
}

int card_part_water(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int trgs = 0;
		instance->number_of_targets = 0;
		while( has_mana(player, COLOR_COLORLESS, (trgs+1)*2) && can_target(&td) ){
				if( new_pick_target(&td, "TARGET_CREATURE", trgs, 0) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		if( trgs > 0 ){
			int i;
			for(i=0; i<trgs; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			charge_mana(player, COLOR_COLORLESS, trgs*2);
			if( spell_fizzled != 1 ){
				instance->info_slot = trgs;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card,
										0, 0, get_hacked_walk(player, card, KEYWORD_ISLANDWALK), 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_pavel_maliki(int player, int card, event_t event)
{
  /* Pavel Maliki	|4|B|R
   * Legendary Creature - Human 5/3
   * |B|R: ~ gets +1/+0 until end of turn. */

  check_legend_rule(player, card, event);

  return generic_shade_merge_pt(player, card, event, 0, MANACOST_BR(1,1), 1, 0);
}

int card_pendelhaven(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.power_requirement = 1;
	td.toughness_requirement = 1;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int choice = instance->info_slot = instance->number_of_targets = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Pump an 1/1\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST0) &&
				new_pick_target(&td, "Select target creature with P/T 1.", 0, 1 | GS_LITERAL_PROMPT)
			  ){
				tap_card(player, card);
				instance->info_slot = 1;
			}
		}

		if( choice == 2 ){
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			if( valid_target(&td) ){
				pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 2);
			}
		}
	}

	return 0;
}

int card_petra_sphinx(int player, int card, event_t event)
{
  // 0x414d30

  /* Petra Sphinx	|2|W|W|W
   * Creature - Sphinx 3/4
   * |T: Target player names a card, then reveals the top card of his or her library. If that card is the named card, that player puts it into his or her
   * hand. If it isn't, the player puts it into his or her graveyard. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  int p = get_card_instance(player, card)->targets[0].player;
	  int chosen, on_top = deck_ptr[p][0];
	  if (!IS_AI(p))
		chosen = choose_a_card("Choose a card", -1, -1);
	  else
		{
		  chosen = get_internal_card_id_from_csv_id(CARD_ID_LABORATORY_MANIAC);	// as good a default as any
		  if (on_top != -1)
			{
			  // This has to be deterministic.  Same algorithm as petra_sphinx_helper() at 0x414d30; no doubt it could be improved.
			  target_t counts[501];
			  memset(counts, -1, sizeof(counts));

			  int i, j, highest_count = 1;
			  for (i = 0; deck_ptr[p][i] != -1; ++i)
				for (j = 0; ; ++j)
				  if (counts[j].card == deck_ptr[p][i])
					{
					  ++counts[j].player;
					  highest_count = MAX(highest_count, counts[j].player);
					  break;
					}
				  else if (counts[j].card == -1)
					{
					  counts[j].card = deck_ptr[p][i];
					  counts[j].player = 1;
					  break;
					}

			  // Now pick the topmost card in the deck whose count is at least tied for the highest
			  for (i = 0; i < 501; ++i)
				if (counts[i].player == highest_count)
				  {
					chosen = counts[i].card;
					break;
				  }
			}
		}

	  if (p == AI || (trace_mode & 2))
		look_at_iid(player, card, chosen, "chooses:");

	  if (on_top != -1)
		{
		  reveal_card_iid(player, card, on_top);
		  if (on_top == chosen)
			add_card_to_hand(p, on_top);
		  else
			mill(p, 1);
		}
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_PLAYER");
}

int card_pit_scorpion(int player, int card, event_t event){
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_TRACE_DAMAGED_PLAYERS) ){
		card_instance_t *instance = get_card_instance(player, card);
		poison_counters[0]+= BYTE0(instance->targets[1].player) ? 1 : 0;
		poison_counters[1]+= BYTE1(instance->targets[1].player) ? 1 : 0;
	}
	return 0;
}

int card_pixie_queen(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.illegal_abilities = KEYWORD_FLYING;
	}
	td.preferred_controller = player;


	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, KEYWORD_FLYING, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_G(3), 0, &td, "TARGET_CREATURE");
}

int card_planar_gate(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		if( event == EVENT_MODIFY_COST_GLOBAL && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			COST_COLORLESS-=2;
		}
	}
	return 0;
}

int card_pradesh_gypsies(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.illegal_abilities = KEYWORD_FLYING;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -2, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XG(1, 1), 0, &td, "TARGET_CREATURE");
}

int card_presence_of_the_master(int player, int card, event_t event){

	/* Presence of the Master	|3|W
	 * Enchantment
	 * Whenever a player casts an enchantment spell, counter it. */

	if( event == EVENT_CAST_SPELL && ! affect_me(player, card) ){
		int p = affected_card_controller;
		int c = affected_card;
		card_data_t* card_d = get_card_data(p, c);
		if( ( card_d->type & TYPE_ENCHANTMENT) && card_d->cc[2] != 9  ){
			real_counter_a_spell(player, card, p, c);
			land_can_be_played &= ~LCBP_SPELL_BEING_PLAYED;
		}
	}
	return global_enchantment(player, card, event);
}

int card_primordial_ooze(int player, int card, event_t event){

	attack_if_able(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_1_1_counter(player, card);
		charge_mana(player, COLOR_COLORLESS, count_1_1_counters(player, card));
		if( spell_fizzled == 1  ){
			tap_card(player, card);
			damage_player(player, count_1_1_counters(player, card), player, card);
		}
	}

	return 0;
}

int card_princess_lucrezia(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	return mana_producing_creature(player, card, event, 0, COLOR_BLUE, 1);
}

int card_psionic_entity(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE && player == AI ){
		if( get_power(player, card) < 3  ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
			if( ! would_validate_target(player, card, &td, 0) || life[1-player] > 8 ){
				ai_modifier-=35;
			}
			instance->targets[0].player = -1;
			instance->targets[0].card = -1;
			instance->number_of_targets = 0;
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		damage_target0(player, card, 2);
		damage_creature(instance->parent_controller, instance->parent_card, 3, instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_psychic_purge(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int puppet_master_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 && instance->targets[1].player > -1 ){
		int pm_player = instance->targets[0].player;
		int pm_card = instance->targets[0].card;

		int enc_player = instance->targets[1].player;
		int enc_card = instance->targets[1].card;

		if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(enc_player, enc_card) ){
			card_instance_t *enchanted = get_card_instance(enc_player, enc_card);
			if( enchanted->kill_code > 0 && enchanted->kill_code != KILL_REMOVE &&
				! check_special_flags2(enc_player, enc_card, SF2_WILL_BE_EXILED_IF_PUTTED_IN_GRAVE)
			  ){
				instance->targets[2].player = get_owner(enc_player, enc_card);
				instance->targets[2].card = cards_data[get_original_internal_card_id(enc_player, enc_card)].id;
				instance->targets[3].player = get_owner(pm_player, pm_card);
				instance->targets[3].card = cards_data[get_original_internal_card_id(pm_player, pm_card)].id;
				remove_status(player, card, STATUS_INVISIBLE_FX);
			}
		}

		if( instance->targets[2].player > -1 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
			affect_me(player, card )
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int result = seek_grave_for_id_to_reanimate(instance->targets[2].player, -1, instance->targets[2].player,
																instance->targets[2].card, REANIMATEXTRA_RETURN_TO_HAND2);
					if( result > -1 ){
						if( has_mana(player, COLOR_BLUE, 3) && do_dialog(player, player, card, -1, -1, " Return Puppet Master\n Pass", 0) == 0 ){
							charge_mana(player, COLOR_BLUE, 3);
							if( spell_fizzled != 1 ){
								seek_grave_for_id_to_reanimate(instance->targets[3].player, -1, instance->targets[3].player,
																instance->targets[3].card, REANIMATEXTRA_RETURN_TO_HAND2);
							}
						}
					}
					kill_card(player, card, KILL_REMOVE);
			}
			else if (event == EVENT_END_TRIGGER){
					instance->targets[2].player = 0;
			}
		}
		if( event == EVENT_CLEANUP && ! in_play(pm_player, pm_card) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_puppet_master(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		int leg = create_legacy_effect(player, card, &puppet_master_legacy);
		add_status(player, leg, STATUS_INVISIBLE_FX);
		card_instance_t *l1 = get_card_instance(player, leg);
		l1->targets[0].player = player;
		l1->targets[0].card = card;
		l1->targets[1].player = instance->damage_target_player;
		l1->targets[1].card = instance->damage_target_card;
		instance->number_of_targets = 2;
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_pyrotechnics(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		int i;
		for(i=0; i<4; i++ ){
			if( new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", i, 1) ){
				if( instance->targets[i].card != -1 ){
					add_state(instance->targets[i].player, instance->targets[i].card, STATE_TARGETTED);
				}
			}
			else{
				break;
			}
		}
		for(i=0; i<instance->number_of_targets; i++ ){
			remove_state(instance->targets[i].player, instance->targets[i].card, STATE_TARGETTED);
		}

		if( instance->number_of_targets < 4 ){
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		divide_damage(player, card, &td);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_quagmire(int player, int card, event_t event){
	landwalk_disabling_card(player, card, event, PB_SWAMPWALK_DISABLED);
	return global_enchantment(player, card, event);
}


static const char* not_already_disabled_by_qtg(int who_chooses, int player, int card){
	if( ! check_special_flags2(player, card, SF2_QUARUM_TRENCH_GNOMES) ){
		return NULL;
	}

	return "AI helper function";
}

int card_quarum_trench_gnomes(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_PLAINS;
	if( player == AI ){
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td.extra = (int32_t)not_already_disabled_by_qtg;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE && player == AI ){
		ai_modifier+=15;
	}

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		set_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_QUARUM_TRENCH_GNOMES);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_LAND");
}

int card_rabid_wombat(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		vigilance(player, card, event);
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result+=(count_auras_enchanting_me(player, card)*2);
		}
	}
	return 0;
}

int card_radjan_spirit(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.required_abilities = KEYWORD_FLYING;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		negate_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, KEYWORD_FLYING);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

// raging bull --> vanilla

int card_ragnar(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_DESTROYED;
	td.preferred_controller = player;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_REGENERATION, 0, 0, 1, 1, 0, 1, 0, &td, "TARGET_CREATURE");
}

int card_ramses_overdark(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ENCHANTED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

static int rapid_fire_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].card > -1 ){
		modify_pt_and_abilities(instance->targets[0].player, instance->targets[0].card, event, 0, 0, KEYWORD_FIRST_STRIKE);
		rampage(instance->targets[0].player, instance->targets[0].card, event, 2);
	}
	if (event == EVENT_CLEANUP){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_rapid_fire(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && current_phase < PHASE_DECLARE_BLOCKERS ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, &rapid_fire_legacy, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rasputin_dreamweaver(int player, int card, event_t event){

	/* Rasputin Dreamweaver	|4|W|U
	 * Legendary Creature - Human Wizard 4/1
	 * ~ enters the battlefield with seven dream counters on it.
	 * Remove a dream counter from Rasputin: Add |1 to your mana pool.
	 * Remove a dream counter from Rasputin: Prevent the next 1 damage that would be dealt to Rasputin this turn.
	 * At the beginning of your upkeep, if Rasputin started the turn untapped, put a dream counter on it.
	 * ~ can't have more than seven dream counters on it. */

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance( player, card);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_DREAM, 7);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( instance->info_slot == 65 && count_counters(player, card, COUNTER_DREAM) < 7 ){
			add_counter(player, card, COUNTER_DREAM);
			while (count_counters(player, card, COUNTER_DREAM) > 7){
				remove_counter(player, card, COUNTER_DREAM);
			}
		}
		instance->info_slot = 0;
	}

	if( event == EVENT_CAN_ACTIVATE && count_counters(player, card, COUNTER_DREAM) > 0 ){
		if( can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return 1;
		}
		return can_produce_mana(player, card);
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;

		int choice = 0;
		if( ! paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Damage shield\n Do nothing", 1);
		}
		if( choice == 0 ){
			remove_counter(player, card, COUNTER_DREAM);
			produce_mana(player, COLOR_COLORLESS, 1);
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					remove_counter(player, card, COUNTER_DREAM);
					instance->info_slot = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			prevent_the_next_n_damage(instance->parent_controller, instance->parent_card,
									  instance->parent_controller, instance->parent_card,
									  1, 0, 0, 0);
		}
	}

	if( event == EVENT_BEGIN_TURN ){
		instance->info_slot = is_tapped(player, card) ? 0 : 65;
	}

	if (event == EVENT_STATIC_EFFECTS){
		while (count_counters(player, card, COUNTER_DREAM) > 7){
			remove_counter(player, card, COUNTER_DREAM);
		}
	}

	return 0;
}

// Rebirth --> hardcoded, for now

int card_recall(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_CAN_CAST ){
		if( player != AI ){
			if( has_mana(player, COLOR_COLORLESS, 2) ){
				return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
			}
		}
		else{
			if( has_mana(player, COLOR_COLORLESS, 2) && count_graveyard(player) > 0 ){
				return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
			}
		}
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( !IS_AI(player) ){
				charge_mana(player, COLOR_COLORLESS, -1);
				if( spell_fizzled != 1 ){
					instance->info_slot = x_value/2;
				}
			}
			else{
				int hand[60];
				int grv[60];
				int hand_index = 0;
				int grv_index = 0;
				int count = 0;
				while( count < active_cards_count[player] ){
						if( in_hand(player, count) ){
							hand[hand_index] = get_base_value(player, count);
							hand_index++;
						}
						count++;
				}
				const int *grave = get_grave(player);
				count = 0;
				while( grave[count] != -1 ){
						grv[grv_index] = get_base_value(-1, grave[count]);
						grv_index++;
						count++;
				}
				int to_regrow = 0;
				int i;
				int k;
				for(i=0; i<hand_index; i++){
					for(k=0; k<grv_index; i++){
						if( grv[k] > hand[i] ){
							to_regrow++;
							grv[k] = -1;
							break;
						}
					}
				}
				while( ! has_mana(player, COLOR_COLORLESS, to_regrow*2) ){
						to_regrow--;
				}
				charge_mana(player, COLOR_COLORLESS, to_regrow*2);
				if( spell_fizzled != 1 ){
					instance->info_slot = to_regrow;
				}
			}
	}
	else if(event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			int cards = instance->info_slot;
			if( cards > hand_count[player] ){
				cards = hand_count[player];
			}
			int i = 0;
			for( i = 0; i < cards; i++){
				discard(player, 0, player);
			}

			for( i = 0; i < cards; i++){
				global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, 1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}

			kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}

int card_red_mana_battery(int player, int card, event_t event)
{
  // 0x423580
  return mana_battery(player, card, event, COLOR_RED);
}

static int reincarnation_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( graveyard_from_play(instance->targets[0].player, instance->targets[0].card, event) ){
		int owner = get_owner(instance->targets[0].player, instance->targets[0].card);
		if( count_graveyard_by_type(owner, TYPE_CREATURE) > 0 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
			int selected = new_select_a_card(player, owner, TUTOR_FROM_GRAVE, 1, instance->targets[0].player == player ? AI_MAX_CMC : AI_MIN_CMC, -1, &this_test);
			reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_UNDER_OWNER_CONTROL);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_reincarnation(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, &reincarnation_legacy, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_relic_barrier(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		tap_card(instance->targets[0].player, instance->targets[0].card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_ARTIFACT");
}

int card_relic_bind2(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( event == EVENT_TAP_CARD && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			instance->number_of_targets = 0;

			target_definition_t td2;
			default_target_definition(player, card, &td2, 0);
			td2.zone = TARGET_ZONE_PLAYERS;
			td2.allow_cancel = 0;

			target_definition_t td3;
			default_target_definition(player, card, &td3, TYPE_ARTIFACT);
			td3.zone = TARGET_ZONE_PLAYERS;
			td3.preferred_controller = player;
			td3.allow_cancel = 0;

			int choice = 0;
			if( can_target(&td2) ){
				if( can_target(&td3) ){
					int ai_choice = 0;
					if( life[player] < 6 && would_validate_arbitrary_target(&td3, player, -1) ){
						ai_choice = 1;
					}
					choice = do_dialog(player, player, card, player, card, " Deal 1 damage\n Target gain 1 life", ai_choice);
				}
			}
			else{
				choice = 1;
			}
			if( choice == 0 && pick_target(&td2, "TARGET_PLAYER") ){
				damage_player(instance->targets[0].player, 1, player, card);
			}
			if( choice == 1 && pick_target(&td3, "TARGET_PLAYER") ){
				gain_life(instance->targets[0].player, 1);
			}
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = 1-player;
	td.allowed_controller = 1-player;

	return targeted_aura(player, card, event, &td, "TARGET_ARTIFACT");
}

static int good_enchantment_for_remove_enchantments(int player, int t_player, int t_card){
	if( player == t_player && ! is_stolen(t_player, t_card) ){
		if( ! has_subtype(t_player, t_card, SUBTYPE_AURA) ){
			return 1;
		}
		else{
			card_instance_t* instance = get_card_instance(t_player, t_card);
			if( instance->damage_target_player == player ){
				return 1;
			}
			else{
				if( is_attacking(instance->damage_target_player, instance->damage_target_card) ){
					return 1;
				}
			}
		}
	}
	return 0;
}

static int bad_enchantment_for_remove_enchantments(int player, int t_player, int t_card){
	if( player != t_player && has_subtype(t_player, t_card, SUBTYPE_AURA) ){
		card_instance_t* instance = get_card_instance(t_player, t_card);
		if( instance->damage_target_player == player ){
			return 1;
		}
		else{
			if( is_attacking(instance->damage_target_player, instance->damage_target_card) ){
				return 1;
			}
		}
	}
	return 0;
}

int card_remove_enchantments(int player, int card, event_t event){
	if (event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_ENCHANTMENT) ){
						if( good_enchantment_for_remove_enchantments(player, i, count) ){
							bounce_permanent(i, count);
						}
					}
					count--;
			}
		}
		for(i=0; i<2; i++){
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_ENCHANTMENT) ){
						if( i == player || (i!= player && bad_enchantment_for_remove_enchantments(player, i, count)) ){
							kill_card(i, count, KILL_DESTROY);
						}
					}
					count--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_remove_soul(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_CREATURE);

	return counterspell(player, card, event, &td, 0);
}

int card_reset(int player, int card, event_t event)
{
  // 0x42d0a0

  // Cast ~ only during an opponent's turn after his or her upkeep step.
  if (event == EVENT_CAN_CAST && player != current_turn && current_phase >= PHASE_DRAW && current_phase <= PHASE_CLEANUP)
	return basic_spell(player, card, event);

  // Untap all lands you control.
  if (event == EVENT_RESOLVE_SPELL)
	{
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && is_what(player, c, TYPE_LAND))
		  untap_card(player, c);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_revelation(int player, int card, event_t event)
{
  enchant_world(player, card, event);

  if (event == EVENT_STATIC_EFFECTS)
	{
	  player_bits[0] |= PB_HAND_REVEALED;
	  player_bits[1] |= PB_HAND_REVEALED;
	}

  return global_enchantment(player, card, event);
}

int card_reverberation(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}
	// Approximation
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)damage_from_sorcery_source;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			target->damage_target_player = target->damage_source_player;
			target->damage_target_card = -1;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_DAMAGE_PREVENTION, &td, "TARGET_DAMAGE", 1, NULL);
}

// righteous avengers --> vanilla

static const char* is_instant_or_aura_targetting_one_of_my_permanents(int who_chooses, int player, int card){

	if( is_what(player, card, TYPE_INSTANT | TYPE_INTERRUPT) || ( is_what(player, card, TYPE_ENCHANTMENT) && has_subtype(player, card, SUBTYPE_AURA)) ){
		card_instance_t *instance = get_card_instance(player, card);
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( instance->targets[i].player == who_chooses && instance->targets[i].card != -1 ){
				return NULL;
			}
		}
	}

	return "must be an instant or an aura targetting a permanent you control";
}

int card_ring_of_immortals(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ANY);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)is_instant_or_aura_targetting_one_of_my_permanents;

	/* Ring of Immortals	|5
	 * Artifact
	 * |3, |T: Counter target instant or Aura spell that targets a permanent you control. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( counterspell_validate(player, card, &td, 0) ){
			real_counter_a_spell(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK | GAA_UNTAPPED, MANACOST_X(3), 0, &td, "TARGET_SPELL");
}

int card_riven_turnbull(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	return mana_producing_creature(player, card, event, 0, COLOR_BLACK, 1);
}

int card_rohgahh_of_kher_keep(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		int count = count_upkeeps(player);
		int sc = count;
		while( count > 0 ){
				if( has_mana(player, COLOR_RED, 3) ){
					charge_mana(player, COLOR_RED, 3);
					if( spell_fizzled != 1 ){
						sc--;
					}
					else{
						break;
					}
				}
				else{
					break;
				}
				count--;
		}
		if( sc > 0 ){
			tap_card(player, card);
			int fake = add_card_to_hand(1-player, instance->internal_card_id);
			int i = active_cards_count[player];
			while( i > -1 ){
					if( in_play(player, i) && get_id(player, i) == CARD_ID_KOBOLDS_OF_KHER_KEEP ){
						tap_card(player, i);
						gain_control(1-player, fake, player, i);
					}
					i--;
			}
			gain_control(1-player, fake, player, card);
			obliterate_card(1-player, fake);
		}
	}

	if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
		if( affected_card_controller == player && get_id(affected_card_controller, affected_card) == CARD_ID_KOBOLDS_OF_KHER_KEEP ){
			event_result += 2;
		}
	}

	return 0;
}

int card_rubinia_soulsinger(int player, int card, event_t event){

	/* Rubinia Soulsinger	|2|G|W|U
	 * Legendary Creature - Faerie 2/3
	 * You may choose not to untap ~ during your untap step.
	 * |T: Gain control of target creature for as long as you control ~ and ~ remains tapped. */

	check_legend_rule(player, card, event);

	choose_to_untap(player, card, event);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if (IS_AI(player)){
		td.allowed_controller = 1-player;
	}

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		card_instance_t* instance = get_card_instance(player, card);
		gain_control_until_source_is_in_play_and_tapped(player, card, instance->targets[0].player, instance->targets[0].card, GCUS_CONTROLLED | GCUS_TAPPED);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_CREATURE");
}

int card_rust2(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	if (event == EVENT_CAN_CAST){
		return can_counter_activated_ability(player, card, event, &td);
	}

	if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		get_card_instance(player, card)->number_of_targets = 0;
		return cast_counter_activated_ability(player, card, 0);
	}

	if (event == EVENT_RESOLVE_SPELL){
		resolve_counter_activated_ability(player, card, &td, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sea_kings_blessing(int player, int card, event_t event){
	return song(player, card, event, COLOR_TEST_BLUE);
}

// seafarer's quay --> impossible

int card_seeker2(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( event == EVENT_BLOCK_LEGALITY && attacking_card_controller == instance->damage_target_player && attacking_card == instance->damage_target_card ){
			if( ! is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) &&
				!(get_color(affected_card_controller, affected_card) & COLOR_TEST_WHITE)
			  ){
				event_result = 1;
			}
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

// segovian leviathan --> vanilla

static const char* is_blocking_s(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_blocking(card, targeting_card))
	return NULL;
  else
	return "must be blocking Sentinel";
}

static const char* s_is_blocking_him(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  if (is_blocking(targeting_card, card))
	return NULL;
  else
	return "must be blocked by Sentinel";
}

int card_sentinel(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_TOUGHNESS && affect_me(player, card) && instance->targets[1].player > -1 ){
		event_result+=instance->targets[1].player;
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller =  1-player;
	td.preferred_controller = 1-player;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)is_blocking_s;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller =  1-player;
	td1.preferred_controller = 1-player;
	td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td1.extra = (int32_t)s_is_blocking_him;

	if( event == EVENT_CAN_ACTIVATE ){
		if( current_phase == PHASE_AFTER_BLOCKING && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			if( current_turn == player ){
				return can_target(&td);
			}
			else{
				return can_target(&td1);
			}
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( current_turn == player ){
				if( select_target(player, card, &td, "Select target creature blocking Sentinel", &(instance->targets[0])) ){
					instance->number_of_targets = 1;
				}
				else{
					spell_fizzled = 1;
				}
			}
			else{
				if( select_target(player, card, &td1, "Select target creature blocked by Sentinel", &(instance->targets[0])) ){
					instance->number_of_targets = 1;
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( (current_turn == player && valid_target(&td)) || (current_turn != player && valid_target(&td1)) ){
			card_instance_t *parent = get_card_instance(player, instance->parent_card);
			parent->targets[1].player = get_power(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

// poison snake --> pit scorpion

int card_serpent_generator(int player, int card, event_t event){
	/* Serpent Generator	|6
	 * Artifact
	 * |4, |T: Put a 1/1 colorless Snake artifact creature token onto the battlefield. It has "Whenever this creature deals damage to a player, that player gets a poison counter." */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_POISON_SNAKE);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL);
}

int card_shelkin_brownie(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.required_abilities = KEYWORD_BANDING;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		negate_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, KEYWORD_BANDING);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_shield_wall(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL){
		pump_subtype_until_eot(player, card, player, -1, 0, 2, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int effect_sns(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == instance->targets[2].player &&
			damage->damage_target_card == -1 && ! check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER) &&
			damage->info_slot > 0 && damage->damage_source_player == instance->targets[0].player && damage->damage_source_card == instance->targets[0].card
		  ){
			damage->damage_target_player = instance->targets[1].player;
			damage->damage_target_card = instance->targets[1].card;
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_shimian_night_stalker(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			int leg = create_targetted_legacy_effect(player, card, &effect_sns, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *hag = get_card_instance(player, leg);
			hag->targets[1].player = instance->parent_controller;
			hag->targets[1].card = instance->parent_card;
			hag->targets[2].player = player;
			hag->number_of_targets = 2;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_B(1), 0, &td, "TARGET_CREATURE");
}

static int silhouette_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == instance->targets[0].card && damage->damage_target_player == instance->targets[0].player &&
				damage->info_slot > 0 && !(damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE))
			  ){
				// targets[5]-targets[9] of the damage_card are a direct copy of the damage source first 5 targets
				int k;
				for(k=5; k<10; k++){
					if( damage->targets[k].player == instance->targets[0].player && damage->targets[k].card == instance->targets[0].card ){
						damage->info_slot = 0;
						break;
					}
				}
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_silhouette(int player, int card, event_t event){
	// Approximation
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, &silhouette_effect, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_sol_kanar_the_swamp_king(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( specific_spell_played(player, card, event, 2, 2, TYPE_ANY, 0, 0, 0, COLOR_TEST_BLACK, 0, 0, 0, -1, 0) ){
		gain_life(player, 1);
	}

	return 0;
}

int card_spectral_cloak(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player != -1 && ! is_tapped(instance->damage_target_player, instance->damage_target_card) ){
		modify_pt_and_abilities(instance->damage_target_player, instance->damage_target_card, event, 0, 0, KEYWORD_SHROUD);
	}

	return vanilla_aura(player, card, event, player);
}

int card_spinal_villain(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

void spirit_link_effect(int player, int card, event_t event, int who_gains){

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

		if( damage_card != source->internal_card_id || source->info_slot <= 0 ||
			source->damage_source_player != player || source->damage_source_card != card
		  ){
			return;
		}

		else{
			gain_life(who_gains, source->info_slot);
		}
	}
}

int card_spirit_link(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		spirit_link_effect(instance->damage_target_player, instance->damage_target_card, event, player);
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_spirit_shackle(int player, int card, event_t event)
{
  /* Spirit Shackle	|B|B
   * Enchantment - Aura
   * Enchant creature
   * Whenever enchanted creature becomes tapped, put a -0/-2 counter on it. */

  if (event == EVENT_TAP_CARD)
	{
	  card_instance_t* instance = in_play(player, card);
	  if (instance && affect_me(instance->damage_target_player, instance->damage_target_card))
		add_counter(instance->damage_target_player, instance->damage_target_card, COUNTER_M0_M2);
	}

  return disabling_aura(player, card, event);
}

int card_spiritual_sanctury(int player, int card, event_t event){
	/* Spiritual Sanctuary	|2|W|W
	 * Enchantment
	 * At the beginning of each player's upkeep, if that player controls |Ha Plains, he or she gains 1 life. */

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		int cp = check_battlefield_for_subtype(current_turn, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_PLAINS));
		upkeep_trigger_ability_mode(player, card, event, ANYBODY, cp ? RESOLVE_TRIGGER_MANDATORY : 0);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		gain_life(current_turn, 1);
	}

	return global_enchantment(player, card, event);
}

int card_stangg(int player, int card, event_t event){
	/* Stangg	|4|R|G
	 * Legendary Creature - Human Warrior 3/4
	 * When ~ enters the battlefield, put a legendary 3/4 |Sred and |Sgreen Human Warrior creature token named ~ Twin onto the battlefield. When ~ leaves the battlefield, exile that token. When that token leaves the battlefield, sacrifice ~. */

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_STANGG_TWIN);
	}

	if( leaves_play(player, card, event) == 1  ){
		manipulate_all(player, card, player, 0, 0, 0, 0, 0, 0, CARD_ID_STANGG_TWIN, 0, -1, 0, KILL_REMOVE);
		manipulate_all(player, card, 1-player,0, 0, 0, 0, 0, 0, CARD_ID_STANGG_TWIN, 0, -1, 0, KILL_REMOVE);
	}

  return 0;
}

int card_stangg_twin(int player, int card, event_t event){
	check_legend_rule(player, card, event);

	if( leaves_play(player, card, event) == 1  ){
		manipulate_all(player, card, player, 0, 0, 0, 0, 0, 0, CARD_ID_STANGG, 0, -1, 0, KILL_SACRIFICE);
		manipulate_all(player, card, 1-player,0, 0, 0, 0, 0, 0, CARD_ID_STANGG, 0, -1, 0, KILL_SACRIFICE);
	}

	return 0;
}

int card_storm_seeker(int player, int card, event_t event)
{
  /* Storm Seeker	|3|G
   * Instant
   * ~ deals damage equal to the number of cards in target player's hand to that player. */

  /* Sudden Impact	|3|R
   * Instant
   * ~ deals damage equal to the number of cards in target player's hand to that player. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, hand_count[get_card_instance(player, card)->targets[0].player]);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_storm_world(int player, int card, event_t event)
{
  // 0x4160a0

  /* Storm World	|R
   * World Enchantment
   * At the beginning of each player's upkeep, ~ deals X damage to that player, where X is 4 minus the number of cards in his or her hand. */

  upkeep_trigger_ability(player, card, event, ANYBODY);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  int damage = 4 - hand_count[current_turn];
	  if (damage > 0)
		damage_player(current_turn, damage, player, card);
	}

  return enchant_world(player, card, event);
}

int card_subdue(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	/* Prevent all combat damage that would be dealt by target creature this turn. That creature gets +0/+X until end of turn, where X is its converted mana
	 * cost. */
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kry_shield_effect(player, card, instance->targets[0].player, instance->targets[0].card, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_sunastian_falconer(int player, int card, event_t event){
	//0x4069b0
	check_legend_rule(player, card, event);
	return mana_producing_creature(player, card, event, 0, COLOR_COLORLESS, 2);
}

int card_sword_of_the_ages(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
	}

	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				int total_damage = 0;
				while( can_target(&td1)  ){
						if( new_pick_target(&td1, "LORD_OF_THE_PIT", 0, 0) ){
							total_damage += get_power(instance->targets[0].player, instance->targets[0].card);
							instance->number_of_targets = 1;
							kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
						}
						else{
							break;
						}
				}
				if( total_damage > 0){
					if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
						tap_card(player, card);
						instance->info_slot = total_damage;
						kill_card(player, card, KILL_REMOVE);
					}
				}
			}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, instance->info_slot);
		}
	}

	return 0;
}

int card_sylvan_library(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( current_turn == player && event == EVENT_DRAW_PHASE ){
			if( do_dialog(player, player, card, -1, -1, " Activate Sylvan Library\n Pass", count_deck(player) > 10 ? 0 : 1) == 0 ){
				get_card_instance(player, card)->info_slot = 1;
				event_result+=2;
			}
		}
		if (event == EVENT_PHASE_CHANGED && current_phase == PHASE_MAIN1 && !(instance->info_slot & 2) && in_play(player, card) && (instance->info_slot & 1) ){
			if( current_turn == player ){
				instance->info_slot |= 2;	// prevent being asked multiple times
				int dtt[2][100];
				int dtt_count = 0;
				int i;
				for(i=0; i<active_cards_count[player]; i++){
					if( in_hand(player, i) && check_state(player, i, STATE_JUST_DRAWED) ){
						dtt[0][dtt_count] = get_card_instance(player, i)->internal_card_id;
						dtt[1][dtt_count] = i;
						dtt_count++;
					}
				}
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put on top of deck");
				int pb = 0;
				while( pb < 2 && dtt_count > 0){
						int selected = select_card_from_zone(player, player, dtt[0], dtt_count, 1-can_pay_life(player, 4), AI_MIN_VALUE, -1, &this_test);
						if( selected != -1 ){
							put_on_top_of_deck(player, dtt[1][selected]);
							int k;
							for(k=selected; k<dtt_count; k++){
								dtt[0][k] = dtt[0][k+1];
								dtt[1][k] = dtt[1][k+1];
							}
							dtt_count--;
						}
						else{
							lose_life(player, 4);
						}
						pb++;
				}
			}
		}

		if(event == EVENT_CLEANUP ){
			instance->info_slot = 0;
		}
	}

	return global_enchantment(player, card, event);
}


int card_sylvan_paradise(int player, int card, event_t event){
	return song(player, card, event, COLOR_TEST_GREEN);
}

int syphon_soul_legacy(int player, int card, event_t event){
	card_instance_t* instance = get_card_instance(player, card);
	if (event == EVENT_DEAL_DAMAGE){
		card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
		if (damage->internal_card_id == damage_card && damage->display_pic_int_id == CARD_ID_SYPHON_SOUL
			&& damage->info_slot > 0
		   ){
			if( instance->targets[1].player < 0 ){
				instance->targets[1].player = 0;
			}
			instance->targets[1].player += damage->info_slot;
		}
	}

	if( instance->targets[1].player > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				gain_life(player, instance->targets[1].player);
				kill_card(player, card, KILL_REMOVE);
		}
		else if (event == EVENT_END_TRIGGER){
				instance->targets[1].player = 0;
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_syphon_soul(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_legacy_effect(player, card, &syphon_soul_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[0].player = player;
		leg->targets[0].card = card;
		damage_player(1-player, 2, player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int takklemaggot_legacy(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		int p = instance->targets[0].player;
		int c = instance->targets[0].card;

		if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(p, c) ){
			if( get_card_instance(p, c)->kill_code > 0 && get_card_instance(p, c)->kill_code != KILL_REMOVE ){
				instance->targets[11].player = 66;
				instance->targets[1].player = get_owner(p, c);
			}
		}

		if( instance->targets[11].player == 66 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
			affect_me(player, card)
		  ){
			if (event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if (event == EVENT_RESOLVE_TRIGGER){
					int tak_player = instance->targets[1].player;
					int crit_player = instance->targets[2].player;
					const int *grave = get_grave(tak_player);
					int count = count_graveyard(tak_player);
					while( count > -1 ){
							if( cards_data[grave[count]].id == CARD_ID_TAKKLEMAGGOT ){
								int card_added = add_card_to_hand(tak_player, grave[count]);
								get_card_instance(tak_player, card_added)->targets[1].player = crit_player;
								get_card_instance(tak_player, card_added)->targets[1].card = -1;
								remove_card_from_grave(tak_player, count);
								put_into_play(tak_player, card_added);
								break;
							}
							count--;
					}
					kill_card(player, card, KILL_REMOVE);
			}
			else if (event == EVENT_END_TRIGGER){
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}
	return 0;
}

int card_takklemaggot(int player, int card, event_t event){

	/* Takklemaggot	|2|B|B
	 * Enchantment - Aura
	 * Enchant creature
	 * At the beginning of the upkeep of enchanted creature's controller, put a -0/-1 counter on that creature.
	 * When enchanted creature dies, that creature's controller chooses a creature that ~ could enchant. If he or she does, return ~ to the battlefield under
	 * your control attached to that creature. If he or she doesn't, return ~ to the battlefield under your control as a non-Aura enchantment. It loses "enchant
	 * creature" and gains "At the beginning of that player's upkeep, ~ deals 1 damage to him or her." */

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			if( c != -1 ){
				add_counter(p, c, COUNTER_M0_M1);
			}
			else{
				damage_player(p, 1, player, card);
			}
		}
		upkeep_trigger_ability(player, card, event, p);

		if( c > -1 ){
			if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(p, c) ){
				if( get_card_instance(p, c)->kill_code > 0 && get_card_instance(p, c)->kill_code != KILL_REMOVE ){
					int legacy = create_legacy_effect(player, card, &takklemaggot_legacy);
					card_instance_t *leg = get_card_instance(player, legacy);
					leg->targets[0].player = player;
					leg->targets[0].card = card;
					leg->targets[2].player = p;
					leg->number_of_targets = 1;
				}
			}
		}
	}

	if( event == EVENT_CAN_MOVE_AURA || event == EVENT_MOVE_AURA || event == EVENT_RESOLVE_MOVING_AURA ){
		return disabling_aura(player, card, event);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL  && affect_me(player, card) ){
		if (player == AI && !(trace_mode & 2)){
			ai_modifier += (instance->targets[0].player == td.preferred_controller) ? 24 : -24;
		}
		if( instance->targets[1].player == -1 ){
			pick_target(&td, "TARGET_CREATURE");
		}
		else{
			td.preferred_controller = 1-instance->targets[1].player;
			td.who_chooses = instance->targets[1].player;
			td.illegal_abilities = 0;
			td.allow_cancel = 0;
			if( can_target(&td) ){
				pick_target(&td, "TARGET_CREATURE");
			}
			else{
				instance->targets[0] = instance->targets[1];
				instance->number_of_targets = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->targets[0].card != -1 ){
			if( valid_target(&td) ){
				attach_aura_to_target(player, card, event, instance->targets[0].player, instance->targets[0].card);
			}
			else{
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
		else{
			instance->damage_target_player = instance->targets[0].player;
			instance->damage_target_card = -1;
		}
	}

	return 0;
}

static int effect_telekinesis(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP ){
		if(affect_me(instance->targets[0].player, instance->targets[0].card) ){
			if( instance->targets[2].card > 0 ){
				card_instance_t *this = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				this->untap_status &= ~3;
				instance->targets[2].card--;
			}
			else{
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( event == EVENT_DEAL_DAMAGE && instance->targets[2].card == 3){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->damage_source_player == instance->targets[0].player &&
			damage->damage_source_card == instance->targets[0].card && damage->info_slot > 0 ){
			damage->info_slot = 0;
		}
	}

	if( eot_trigger(player, card, event) ){
		instance->targets[2].card--;
	}

	return 0;
}

int card_telekinesis(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			int reincarnated = create_targetted_legacy_effect(player, card, &effect_telekinesis, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *reinc = get_card_instance(player, reincarnated);
			reinc->targets[2].card = 3;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_teleport2(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && current_phase == PHASE_BEFORE_BLOCKING){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// tempest efreet --> undoable

int card_tetsuo_umezawa(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_TAPPED_OR_BLOCKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, 0, 2, 1, 0, 1, 0, 0,
									&td, "Select target tapped or blocking creature.");
}

// At the beginning of each player's upkeep, destroy target nonartifact creature that player controls of his or her choice. It can't be regenerated.
void abyss_trigger(int player, int card, event_t event)
{
  upkeep_trigger_ability(player, card, event, ANYBODY);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.illegal_type = TYPE_ARTIFACT;
	  td.who_chooses = current_turn;
	  td.allowed_controller = current_turn;
	  td.preferred_controller = current_turn;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_next_target_noload(&td, "Select target nonartifact creature you control."))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
	}
}

int card_the_abyss(int player, int card, event_t event)
{
  // 0x416140

  /* The Abyss	|3|B
   * World Enchantment
   * At the beginning of each player's upkeep, destroy target nonartifact creature that player controls of his or her choice. It can't be regenerated. */

  abyss_trigger(player, card, event);
  return enchant_world(player, card, event);
}

int card_the_brute(int player, int card, event_t event){
	card_instance_t* instance = get_card_instance(player, card);

	if ( instance->damage_target_player > -1 ){
		if( IS_GAA_EVENT(event) ){
			int p = instance->damage_target_player;
			int c = instance->damage_target_card;
			if( event == EVENT_RESOLVE_ACTIVATION ){
				if( can_regenerate(p, c) ){
					regenerate_target(p, c);
				}
			}
			return granted_generic_activated_ability(player, card, p, c, event, GAA_REGENERATION, MANACOST_R(3), 0, NULL, NULL);
		}
	}

	return generic_aura(player, card, event, player, 1, 0, 0, 0, 0, 0, 0);
}

int card_the_tabernacle_at_pendrell_vale(int player, int card, event_t event){
	if (event == EVENT_RESOLVE_SPELL){
		play_land_sound_effect(player, card);
	}

	check_legend_rule(player, card, event);

	pendrell_effect(player, card, event, KILL_DESTROY);

	return 0;
}

int card_the_wretched(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn == player && event == EVENT_DECLARE_BLOCKERS ){
		int count = 0;
		instance->info_slot = 0;
		while( count < active_cards_count[1-player] ){
				if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
					if(is_blocking(count, card) ){
						instance->targets[instance->info_slot].card = count;
						instance->info_slot++;
					}
				}
				count++;
		}
	}

	if( instance->info_slot > 0 && end_of_combat_trigger(player, card, event, 2) ){
		int amount = instance->info_slot;
		int i;
		for(i = 0; i < amount; i++ ){
			if( in_play( 1-player, instance->targets[i].card) ){
				gain_control_until_source_is_in_play_and_tapped(player, card, 1-player, instance->targets[i].card, GCUS_CONTROLLED);
			}
		}
		instance->info_slot = 0;
	}

	return 0;
}

// thunder spirit --> vanilla

static int time_elemental_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );
	if( event == EVENT_SHOULD_AI_PLAY || end_of_combat_trigger(player, card, event, 2) ){
		damage_player(player, 5, player, card);
		if (in_play(instance->targets[0].player, instance->targets[0].card) && player == instance->targets[0].player){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}
	return 0;
}

int card_time_elemental(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_state = TARGET_STATE_ENCHANTED;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	if( ((event == EVENT_DECLARE_ATTACKERS && is_attacking(player, card))
		 || (current_turn != player && blocking(player, card, event)))
		&& !is_humiliated(player, card) ){
		int leg = create_legacy_effect(player, card, &time_elemental_legacy);
		card_instance_t* legacy = get_card_instance(player, leg);
		legacy->targets[0].player = player;
		legacy->targets[0].card = card;
	}

	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		ai_defensive_modifier += 120;
	}
	if (event == EVENT_BLOCK_RATING && affect_me(player, card)){
		ai_defensive_modifier -= 120;
	}
	// Try even harder to keep this from blocking - tell the AI that anything that blocks or is blocked by this will kill it
	if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card)){
		instance->destroys_if_blocked |= DIFB_ASK_CARD;
	}
	if (event == EVENT_CHECK_DESTROY_IF_BLOCKED && affect_me(player, card) && !is_humiliated(player, card)){
		event_result |= 2;
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XU(2, 2), 0, &td, "TIME_ELEMENTAL");
}

static int tolaria_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( instance->targets[0].player > -1 ){
		if( event == EVENT_ABILITIES && affect_me(instance->targets[0].player, instance->targets[0].card) ){
			if( event_result & KEYWORD_BANDING ){
				event_result &= ~KEYWORD_BANDING;
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_tolaria(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = instance->number_of_targets = 0;
		if( ! paying_mana() && current_phase == PHASE_UPKEEP &&
			generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL)
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Remove Banding\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST0) && pick_target(&td, "TARGET_CREATURE") ){
				tap_card(player, card);
				instance->info_slot = 1;
			}
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			if( valid_target(&td) ){
				create_targetted_legacy_effect(player, instance->parent_card, &tolaria_legacy, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return 0;
}

int card_tor_wauki(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	return card_heavy_ballista(player, card, event);
}

int card_touch_of_darkness(int player, int card, event_t event){
	return song(player, card, event, COLOR_TEST_BLACK);
}

int card_transmutation(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	// Switch target creature's power and toughness until end of turn.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = ANYBODY;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_SPELL){
		if (valid_target(&td))
			switch_power_and_toughness_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_triassic_egg(int player, int card, event_t event){

	/* Triassic Egg	|4
	 * Artifact
	 * |3, |T: Put a hatchling counter on ~.
	 * Sacrifice ~: Choose one - You may put a creature card from your hand onto the battlefield; or return target creature card from your graveyard to the
	 * battlefield. Activate this ability only if two or more hatchling counters are on ~. */

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( player != AI && generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL) ){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

		int choice = 0;
		if( player != AI && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL) ){
				choice = do_dialog(player, player, card, player, card, " Add counter\n Open the Egg\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(choice == 0 ? 3 : 0)) ){
			if( choice == 0 ){
				tap_card(player, card);
				instance->info_slot = 66+choice;
			}
			if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					this_test.zone = TARGET_ZONE_HAND;
					int zn = 0;
					if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
						if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(2) ){
							zn = do_dialog(player, player, card, player, card, " Tutor from hand\n Tutor from grave\n Cancel", 1);
						}
					}
					else{
						zn = 1;
					}
					if( zn == 0 ){
						instance->info_slot = 66+choice;
						kill_card(player, card, KILL_SACRIFICE);
					}
					else if( zn == 1 ){
							if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 0) != -1 ){
								instance->info_slot = 66+choice+zn;
								kill_card(player, card, KILL_SACRIFICE);
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
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			add_counter(instance->parent_controller, instance->parent_card, COUNTER_HATCHLING);
		}
		if( instance->info_slot == 67 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
		if( instance->info_slot == 68 ){
			int selected = validate_target_from_grave_source(player, card, player, 0);
			if( selected != -1  ){
				reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			}
		}
	}

	if( player == AI && current_turn != player && eot_trigger(player, card, event) ){
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL) ){
			charge_mana_for_activated_ability(player, card, MANACOST_X(3));
			if( spell_fizzled != 1 ){
				tap_card(player, card);
				add_counter(player, card, COUNTER_HATCHLING);
			}
		}
	}

	return 0;
}

// tundra wolves

int card_tuknir_deathlock(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_GR(1, 1), 0, &td, "TARGET_CREATURE");
}

int card_typhoon(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		damage_player(1-player, count_subtype(1-player, TYPE_PERMANENT, get_hacked_subtype(player, card, SUBTYPE_ISLAND)), player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 1, NULL);
}

int card_undertow(int player, int card, event_t event){
	landwalk_disabling_card(player, card, event, PB_ISLANDWALK_DISABLED);
	return global_enchantment(player, card, event);
}


int card_underworld_dreams(int player, int card, event_t event){
	if (card_drawn_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY)){
		damage_player(1-player, 1, player, card);
	}

	return global_enchantment(player, card, event);
}

//unholy citadel --> impossible

int card_untamed_wilds(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		tutor_basic_land(player, 1, 0);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_ur_drago(int player, int card, event_t event){
	// completed
	check_legend_rule(player, card, event);
	landwalk_disabling_card(player, card, event, PB_SWAMPWALK_DISABLED);
	return 0;
}

static int urborg_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( instance->targets[0].player > -1 ){
		if( event == EVENT_ABILITIES && affect_me(instance->targets[0].player, instance->targets[0].card) ){
			if( instance->targets[1].player == 66 && (event_result & KEYWORD_FIRST_STRIKE) ){
				event_result &= ~KEYWORD_FIRST_STRIKE;
			}
			if( instance->targets[1].player == 67 && (event_result & KEYWORD_BASIC_LANDWALK) ){
				event_result &= ~get_hacked_walk(player, card, KEYWORD_SWAMPWALK);
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_urborg(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = instance->number_of_targets = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Remove ability\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			if( charge_mana_for_activated_ability(player, card, MANACOST0) && pick_target(&td, "TARGET_CREATURE") ){
				tap_card(player, card);
				instance->info_slot = 1;
			}
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
				if( valid_target(&td) ){
					int choice = 0;
					int ai_choice = 0;
					keyword_t walk = get_hacked_walk(player, card, KEYWORD_SWAMPWALK);
					if( check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_FIRST_STRIKE) ){
						if( check_for_ability(instance->targets[0].player, instance->targets[0].card, walk) ){
							if( current_phase < PHASE_DECLARE_BLOCKERS ){
								ai_choice = 0;
							}
							char buf[128];
							sprintf(buf, " Remove first strike\n Remove %swalk",
									walk == KEYWORD_SWAMPWALK ? "swamp"
									: walk == KEYWORD_ISLANDWALK ? "ISLAND"
									: walk == KEYWORD_FORESTWALK ? "FOREST"
									: walk == KEYWORD_MOUNTAINWALK ? "MOUNTAIN"
									: "PLAINS");
							choice = do_dialog(player, instance->targets[0].player, instance->targets[0].card, -1, -1, buf, ai_choice);
						}
					}
					else{
						choice = 1;
					}
					int legacy = create_targetted_legacy_effect(player, instance->parent_card, &urborg_legacy,
																instance->targets[0].player, instance->targets[0].card);
					card_instance_t *leg = get_card_instance( player, legacy );
					leg->targets[1].player = 66+choice;
				}
		}
	}

	return 0;
}

int card_vampiric_bats(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && instance->info_slot < 2 ){
		return generic_shade(player, card, event, 0, MANACOST_B(1), 1, 0, 0, 0);
	}

	if( event == EVENT_ACTIVATE && instance->info_slot < 2 ){
		generic_shade(player, card, event, 0, MANACOST_B(1), 1, 0, 0, 0);
		if( spell_fizzled != 1 ){
			instance->info_slot++;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		return generic_shade(player, card, event, 0, MANACOST_B(1), 1, 0, 0, 0);
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_vaevictis_asmadi(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	basic_upkeep(player, card, event, 0, 1, 0, 1, 1, 0);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) ){
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
			return 1;
		}
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
		if( spell_fizzled == 1 ){
			spell_fizzled = 0;
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
			if( spell_fizzled == 1 ){
				spell_fizzled = 0;
				charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, 0);
	}

	return 0;
}

int card_venarian_gold(int player, int card, event_t event)
{
  /* Venarian Gold	|X|U|U
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield, tap enchanted creature and put X sleep counters on it.
   * Enchanted creature doesn't untap during its controller's untap step if it has a sleep counter on it.
   * At the beginning of the upkeep of enchanted creature's controller, remove a sleep counter from that creature. */

  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player, c = instance->damage_target_card;

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	instance->info_slot = x_value;

  if (comes_into_play(player, card, event))
	{
	  add_counters(p, c, COUNTER_SLEEP, instance->info_slot);
	  tap_card(p, c);
	}

  if (event == EVENT_UNTAP && affect_me(p, c) && count_counters(p, c, COUNTER_SLEEP))
	does_not_untap(p, c, event);

  upkeep_trigger_ability(player, card, event, p);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	remove_counter(p, c, COUNTER_SLEEP);

  return disabling_aura(player, card, event);
}

int card_visions2(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[player];
			show_deck( player, deck, 5, "Cards revealed by Visions", 0, 0x7375B0 );
			int choice = do_dialog(player, player, card, -1, -1, " Reshuffle target player's deck\n Pass", instance->targets[0].player == player ? 1 : 0);
			if( ! choice ){
				shuffle(instance->targets[0].player);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_PLAYER", 1, NULL);
}

int card_voodoo_doll( int player, int card, event_t event){

	/* Voodoo Doll	|6
	 * Artifact
	 * At the beginning of your upkeep, put a pin counter on ~.
	 * At the beginning of your end step, if ~ is untapped, destroy ~ and it deals damage to you equal to the number of pin counters on it.
	 * |X|X, |T: ~ deals damage equal to the number of pin counters on it to target creature or player. X is the number of pin counters on ~. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_PIN);
	}

	if( player == current_turn && !is_tapped(player, card) && eot_trigger(player, card, event) ){
		int dam = count_counters(player, card, COUNTER_PIN);
		if (dam > 0){
			damage_player(player, dam, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td1) ){
			damage_target0(player, card, count_counters(player, card, COUNTER_PIN));
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(2 * count_counters(player, card, COUNTER_PIN)), 0, &td1, "TARGET_CREATURE_OR_PLAYER");
}

// walking dead --> drudge skeletons

// wall of caltrops --> impossible

static int effect_cant_attack_during_controllers_next_turn(int player, int card, event_t event)
{
  // 0x4a0c10
  effect_cannot_attack(player, card, event);

  if ((event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (instance->damage_target_player < 0 || instance->damage_target_card < 0
		  || !in_play(instance->damage_target_player, instance->damage_target_card)
		  || (current_turn == instance->damage_target_player
			  && --get_card_instance(player, card)->targets[1].player <= 0))
		kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}

static void add_effect_cant_attack_during_controllers_next_turn(int player, int card, int t_player, int t_card)
{
  int leg = create_targetted_legacy_effect(player, card, effect_cant_attack_during_controllers_next_turn, t_player, t_card);
  if (leg != -1)
	{
	  card_instance_t* legacy = get_card_instance(player, leg);
	  legacy->targets[1].player = (current_turn == t_player) ? 2 : 1;
	}
}

int card_wall_of_dust(int player, int card, event_t event)
{
  //0x4cab80
  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card)
	  && player == 1-current_turn && (get_card_instance(player, card)->state & STATE_BLOCKING))
	for_each_creature_blocked_by_me(player, card, add_effect_cant_attack_during_controllers_next_turn, player, card);

  return 0;
}

// wall of earth, wall of heat, wall of light --> vanilla

int card_wall_of_opposition(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_X(1), 1, 0, 0, 0);
}

int card_wall_of_putrid_flesh(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->info_slot > 0 ){
			if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) &&
				is_enchanted(damage->damage_source_player, damage->damage_source_card)
			  ){
				damage->info_slot = 0;
			}
		}
	}

	return 0;
}

int card_wall_of_shadows(int player, int card, event_t event)
{
  // Prevent all damage that would be dealt to ~ by creatures it's blocking.
  card_instance_t* damage = damage_being_prevented(event);
  if (damage
	  && damage->damage_target_player == player && damage->damage_target_card == card
	  && (damage->targets[3].player & TYPE_CREATURE)
	  && damage->damage_source_player == current_turn
	  && player != current_turn
	  && in_play(damage->damage_source_player, damage->damage_source_card)
	  && is_blocking(card, damage->damage_source_card)
	  && !is_humiliated(player, card))
	damage->info_slot = 0;

  // ~ can't be the target of spells that can target only Walls or of abilities that can target only Walls.
  // Approximated by a hack in validate_target_impl().  (Which should maybe move to is_protected_from() once that's supported in targeting.)

  return 0;
}

int card_wall_of_tombstones(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_TOUGHNESS && affect_me(player, card) && instance->targets[1].player > -1 ){
			event_result+=instance->targets[1].player;
		}

		if( current_turn == player && upkeep_trigger(player, card, event) ){
			instance->targets[1].player = count_graveyard_by_type(player, TYPE_CREATURE);
		}
	}

	return 0;
}

int card_wall_of_vapor(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->info_slot > 0 ){
			if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) &&
				instance->blocking == damage->damage_source_card && damage->damage_source_player == 1-player
			  ){
				damage->info_slot = 0;
			}
		}
	}

	return 0;
}

int card_wall_of_wonder(int player, int card, event_t event)
{
  //0x42b670
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 4, -4);
	  int leg = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, effect_defender_can_attack_until_eot,
											   instance->parent_controller, instance->parent_card);
	  if (leg != -1)
		get_card_instance(instance->parent_controller, leg)->token_status = STATUS_INVISIBLE_FX;
	}

  if (event == EVENT_POW_BOOST && has_mana_for_activated_ability(player, card, MANACOST_XU(2, 2)))
	return 4;

  return generic_activated_ability(player, card, event, 0, MANACOST_XU(2, 2), 0, NULL, NULL);
}

int card_whirling_dervish(int player, int card, event_t event){
	// whirling dervish
	card_instance_t *instance = get_card_instance( player, card );

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT) ){
		instance->targets[1].player = 66;
	}
	if( instance->targets[1].player == 66 && eot_trigger(player, card, event) && ! is_humiliated(player, card) ){
		add_1_1_counter(player, card);
		instance->targets[1].player = 0;
	}

	return 0;
}

int card_white_mana_battery(int player, int card, event_t event)
{
  // 0x4235C0
  return mana_battery(player, card, event, COLOR_WHITE);
}

int card_willow_satyr(int player, int card, event_t event){
	// original code : 0x407680

	/* Willow Satyr	|2|G|G
	 * Creature - Satyr 1/1
	 * You may choose not to untap ~ during your untap step.
	 * |T: Gain control of target legendary creature for as long as you control ~ and ~ remains tapped. */

	choose_to_untap(player, card, event);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_LEGEND;
	if (IS_AI(player)){
		td.allowed_controller = 1-player;
	}

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		card_instance_t* instance = get_card_instance(player, card);
		gain_control_until_source_is_in_play_and_tapped(player, card, instance->targets[0].player, instance->targets[0].card, GCUS_CONTROLLED | GCUS_TAPPED);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_LEGENDARY_CREATURE");
}

int card_winds_of_change(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int result = reshuffle_hand_into_deck(i, 0);
			draw_cards(i, result);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_winter_blast(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	card_instance_t* instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int max = check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ? instance->number_of_targets : 10;
		instance->number_of_targets = 0;
		int tg = 0;
		while( can_target(&td) && tg < max && (check_special_flags2(player, card, SF2_COPIED_FROM_STACK) || has_mana(player, COLOR_COLORLESS, tg+1)) ){
				if( new_pick_target(&td, "TARGET_CREATURE", tg, 0) ){
					state_untargettable(instance->targets[tg].player, instance->targets[tg].card, 1);
					tg++;
				}
				else{
					break;
				}
		}
		int i;
		for(i=0; i<tg; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		}
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			charge_mana(player, COLOR_COLORLESS, tg);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
				if( check_for_ability(instance->targets[i].player, instance->targets[i].card, KEYWORD_FLYING) ){
					damage_creature(instance->targets[i].player, instance->targets[i].card, 2, player, card);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_wood_elemental(int player, int card, event_t event)
{
  /* Wood Elemental	|3|G
   * Creature - Elemental 100/100
   * As ~ enters the battlefield, sacrifice any number of untapped |H1Forests.
   * ~'s power and toughness are each equal to the number of |H1Forests sacrificed as it entered the battlefield. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->state |= STATE_OUBLIETTED;

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select an untapped %s.", SUBTYPE_FOREST));
	  test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
	  test.state = STATE_TAPPED;
	  test.state_flag = DOESNT_MATCH;
	  test.qty = 500;

	  int sacced = new_sacrifice(player, card, player, SAC_DONE, &test);
	  instance->info_slot = sacced;
	  instance->regen_status |= KEYWORD_RECALC_POWER | KEYWORD_RECALC_TOUGHNESS;
	  instance->state &= ~STATE_OUBLIETTED;
	}

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card))
	{
	  if (card == -1)	// no forests sacrificed yet if not on the bf
		return 0;

	  event_result += get_card_instance(player, card)->info_slot;
	}

  return 0;
}

int card_xira_arien(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			draw_cards(instance->targets[0].player, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 1, 0, 1, 1, 0, 0, &td, "TARGET_PLAYER");
}

// zephyr falcon --> serra angel
