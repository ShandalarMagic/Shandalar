#include "manalink.h"

// Global functions

int bushido(int player, int card, event_t event, int pump){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL && pump > 0 ){
		instance->targets[8].player = pump;
	}

	if( instance->targets[16].card < 0 ){
		instance->targets[16].card = 0;
	}

	if( ! (instance->targets[16].card & SP_KEYWORD_BUSHIDO) ){
		instance->targets[16].card |= SP_KEYWORD_BUSHIDO;
	}

	if( instance->targets[8].player > 0 && !is_humiliated(player, card) ){
		if( event == EVENT_DECLARE_BLOCKERS ){
			if( (instance->state & STATE_ATTACKING) && ! is_unblocked(player, card) ){
				pump_until_eot(player, card, player, card, instance->targets[8].player, instance->targets[8].player);
			}
			else if( current_turn != player && blocking(player, card, event) ){
				pump_until_eot(player, card, player, card, instance->targets[8].player, instance->targets[8].player);
			}
		}
		if( affect_me(player, card) && event == EVENT_CHECK_PUMP ){
			pumpable_power[player]+=instance->targets[8].player;
			pumpable_toughness[player]+=instance->targets[8].player;
		}
	}

	return 0;
}

static int get_zubera_count(void){
	int amount = get_trap_condition(HUMAN, TRAP_DEAD_ZUBERA_COUNT) + get_trap_condition(AI, TRAP_DEAD_ZUBERA_COUNT);
	return amount;
}

void splice_onto_arcane(int player, int card)
{
  // When cast, check the hand for splice spells.
  /* Storage allocation for all arcane spells:
   * targets[0]: Target of the original spell.
   * targets[1], [2], [3], [4], [5], [6]: Targets of spliced spell #1, #2, #3, #4, #5, and #6.
   * targets[7].player, [7].card, [8].player, [8].card, [9].player, [9].card: Low 3 bytes are the csvids of spliced-on spells #1, #2, #3, #4, #5, and #6
   *            respectively; high byte is the card indices of those spells.
   * targets[10]: unused
   * targets[11]-[19]: Used for global effects.
   * eot_toughness: Number of spliced spells.  (So splicing two spells results in eot_toughness == 2.)
   * info_slot: Reserved for spliced-onto spell's use.  In practice, can only be used with Arcane spells that can't be spliced (or else they'd overwrite the
   *            info_slot of the card they're spliced onto). */

  card_instance_t* instance = get_card_instance(player, card);
  if (IS_AI(player))
	return;

  struct
  {
	int csvid;
	uint8_t card;
  } options[151];

  typedef int (*CardFunction)(int, int, event_t);

  int i;
  int opt_count = 1;
  card_instance_t* inst;
  for (i = 0; i < active_cards_count[player]; ++i)
	if ((inst = in_hand(player, i)))
	  {
		CardFunction fn = (CardFunction)(cards_data[inst->internal_card_id].code_pointer);
		if (call_card_fn(fn, instance, player, card, EVENT_CAN_SPLICE))
		  {
			options[opt_count].csvid = get_id(player, i);
			options[opt_count].card = i;
			++opt_count;
		  }
	  }

  instance->eot_toughness = 0;
  int* data = &instance->targets[7].player;
  ASSERT(&data[5] == &instance->targets[9].card);

  int spliced = 0;
  while (spliced < 6 && opt_count > 1)
	{
	  char buffer[600];
	  int pos = scnprintf(buffer, 600, " Done\n");
	  for (i = 1; i < opt_count; ++i)
		pos += scnprintf(buffer + pos, 600 - pos, " Splice %s\n", cards_ptr[options[i].csvid]->name);

	  int choice = do_dialog(player, player, card, -1, -1, buffer, 0);
	  if (choice == 0)
		break;

	  hack_prepend_prompt = cards_ptr[options[choice].csvid]->name;

	  CardFunction fn = (CardFunction)(cards_data[get_internal_card_id_from_csv_id(options[choice].csvid)].code_pointer);
	  call_card_fn(fn, instance, player, card, EVENT_SPLICE);

	  hack_prepend_prompt = NULL;

	  if (spell_fizzled == 1)
		{
		  spell_fizzled = 0;
		  continue;
		}

	  instance->targets[spliced + 1] = instance->targets[0];	// struct copy
	  data[spliced] = options[choice].csvid;
	  SET_BYTE3(data[spliced]) = options[choice].card;
	  ++instance->eot_toughness;
	  ++spliced;

	  for (i = choice; i < opt_count; ++i)
		options[i] = options[i + 1];	// struct copy

	  --opt_count;

	  for (i = 1; i < opt_count; ++i)
		if (!((inst = in_hand(player, options[i].card))
			  && call_card_fn((CardFunction)(cards_data[inst->internal_card_id].code_pointer), instance, player, card, EVENT_CAN_SPLICE)))
		  {
			int j;
			for (j = i; j < opt_count; ++j)
			  options[j] = options[j + 1];	// struct copy

			--opt_count;
			--i;	// So we test the card that was just moved into this slot
		  }
	}

  instance->number_of_targets = 0;
}

void check_for_spliced_spells(int player, int card)
{
  card_instance_t* instance = get_card_instance(player, card);
  int* data = &instance->targets[7].player;
  int i, old_number_of_targets = instance->number_of_targets;
  for (i = 0; i < (int)instance->eot_toughness; ++i)
	if (data[i] != -1)
	  {
		// A hack to make the spliced spell see its target in targets[0].
		instance->targets[0] = instance->targets[i + 1];	// struct copy
		instance->number_of_targets = instance->targets[0].player >= 0 && instance->targets[0].card >= 0 ? 1 : 0;

		/* A truly horrible hack to make any effect cards created by the spliced spell have that spell's image and text, while still using the spliced-onto
		 * spell for target validation. */
		HackForceEffectChangeSource old_hack_force_effect_change_source = hack_force_effect_change_source;	// struct copy

		hack_force_effect_change_source.from.player = player;
		hack_force_effect_change_source.from.card = card;
		hack_force_effect_change_source.to.player = is_stolen(player, card) ? 1-player : player;
		hack_force_effect_change_source.to.card = BYTE3(data[i]);

		typedef int (*CardFunction)(int, int, event_t);
		CardFunction fn = (CardFunction)(cards_data[get_internal_card_id_from_csv_id(data[i] & 0x00FFFFFF)].code_pointer);
		call_card_fn(fn, instance, player, card, EVENT_RESOLVE_SPLICE);

		hack_force_effect_change_source = old_hack_force_effect_change_source;	// struct copy
	  }

  instance->number_of_targets = old_number_of_targets;
}

/* A handy frontend to Arcane instants and sorceries that also have Splice onto Arcane.  fn() is almost a normal card function; the only difference is that it
 * shouldn't call kill_card(player, card, KILL_DESTROY) during EVENT_RESOLVE_SPELL.  Even handier when called via the ARCANE_WITH_SPLICE macro.
 * If x == -10, then send EVENT_CAN_SPLICE and EVENT_SPLICE to the supplied function; it should respond only by checking for and charging the splice cost
 * respectively, returning 1 on success or 0 on failure.  This differs from the normal EVENT_CAN_SPLICE/EVENT_SPLICE behavior. */
int arcane_with_splice(int player, int card, int event, int (*fn)(int,int,event_t), int x, int b, int u, int g, int r, int w)
{
  switch (event)
	{
	  case EVENT_CAN_CAST:
		return fn(player, card, event);

	  case EVENT_CAN_SPLICE:
		if (x == -10 ? fn(player, card, EVENT_CAN_SPLICE) : has_mana_multi(player, x, b, u, g, r, w))
		  return fn(player, card, EVENT_CAN_CAST);
		else
		  return 0;

	  case EVENT_CAST_SPELL:
		if (!affect_me(player, card))
		  return 0;
		splice_onto_arcane(player, card);
		if (get_card_instance(player, card)->eot_toughness > 0)
		  hack_prepend_prompt = get_card_name_by_id(get_id(player, card));
		int cast_rval = fn(player, card, event);
		hack_prepend_prompt = NULL;
		return cast_rval;

	  case EVENT_SPLICE:
		if (!(x == -10 ? fn(player, card, EVENT_SPLICE) : charge_mana_multi(player, x, b, u, g, r, w)))
		  {
			spell_fizzled = 1;
			return 0;
		  }

		// Temporary assign affected_card* so we can use the usual EVENT_CAST_SPELL && affect_me(player, card) form in client code
		int old_affected_card_controller = affected_card_controller, old_affected_card = affected_card;

		affected_card_controller = player;
		affected_card = card;

		int splice_rval = fn(player, card, EVENT_CAST_SPELL);

		affected_card_controller = old_affected_card_controller;
		affected_card = old_affected_card;

		return splice_rval;

	  case EVENT_RESOLVE_SPELL:
		fn(player, card, event);
		check_for_spliced_spells(player, card);
		kill_card(player, card, KILL_DESTROY);
		return 0;

	  case EVENT_RESOLVE_SPLICE:
		return fn(player, card, EVENT_RESOLVE_SPELL);

	  default:
		return fn(player, card, event);
	}
}

// As arcane_with_splice, but can't be spliced itself.
int arcane(int player, int card, int event, int (*fn)(int,int,event_t))
{
  switch (event)
	{
	  case EVENT_CAN_CAST:
		return fn(player, card, event);

	  case EVENT_CAST_SPELL:
		if (!affect_me(player, card))
		  return 0;
		splice_onto_arcane(player, card);
		if (get_card_instance(player, card)->eot_toughness > 0)
		  hack_prepend_prompt = get_card_name_by_id(get_id(player, card));
		int cast_rval = fn(player, card, event);
		hack_prepend_prompt = NULL;
		return cast_rval;

	  case EVENT_RESOLVE_SPELL:
		fn(player, card, event);
		check_for_spliced_spells(player, card);
		kill_card(player, card, KILL_DESTROY);
		return 0;

	  default:
		return fn(player, card, event);
	}
}

int soulshift(int player, int card, event_t event, int cmc, int num){

	if( this_dies_trigger(player, card, event, 2) ){
		char msg[100];
		scnprintf(msg, 100, "Select a Spirit card with cmc %d or less.", cmc);
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		this_test.subtype = SUBTYPE_SPIRIT;
		this_test.subtype_flag = MATCH;
		this_test.cmc = cmc+1;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		for (; num > 0 && new_special_count_grave(player, &this_test); --num){
			if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test) == -1 ){
				break;
			}
		}
	}

	return 0;
}

int arcane_spirit_spell_trigger(int player, int card, event_t event, int trigger_mode){

	card_instance_t *instance = get_card_instance( player, card );

	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		player == trigger_cause_controller
	  ){
		int trig = 0;

		if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
			if( make_test_in_play(trigger_cause_controller, trigger_cause, -1, TYPE_ANY, 0, SUBTYPE_ARCANE, 0, 0, 0, 0, 0, -1, 0) ){
				trig = 1;
			}
			if( make_test_in_play(trigger_cause_controller, trigger_cause, -1, TYPE_ANY, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0) ){
				trig = 1;
			}
		}

		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				if (trigger_mode == RESOLVE_TRIGGER_DUH){
					trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
				}

				event_result |= trigger_mode;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					instance->targets[1].player = trigger_cause_controller;
					instance->targets[1].card = trigger_cause;
					return 1;
			}
		}
	}
	return 0;
}

static int myojin(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = not_played_from_hand(player, card);
	}

	if (!instance->info_slot){
		enters_the_battlefield_with_counters(player, card, event, COUNTER_DIVINITY, 1);
	}

	if( count_counters(player, card, COUNTER_DIVINITY) > 0 ){
		indestructible(player, card, event);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( count_counters(player, card, COUNTER_DIVINITY) > 0 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		remove_counter(player, card, COUNTER_DIVINITY);
	}

	return 0;
}

// cards

int card_akki_coalflinger(int player, int card, event_t event)
{
  // |R, |T: Attacking creatures gain first strike until end of turn.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "");
		test.state = STATE_ATTACKING;
		pump_creatures_until_eot(player, card, player, 0, 0,0, KEYWORD_FIRST_STRIKE,0, &test);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_R(1), 0, NULL, NULL);
}

int card_ashes_skin_zubera(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;

		if( would_validate_target(player, card, &td, 0) ){
			new_multidiscard(instance->targets[0].player, get_zubera_count(), 0, player);
		}
	}

	return 0;
}

int card_azami_lady_of_scrolls(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.required_subtype = SUBTYPE_WIZARD;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_PERMANENT") ){
			instance->number_of_targets = 1;
			if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_WIZARD) ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_azusa_lost_but_seeking(int player, int card, event_t event){
	// The effect is in "total_playable_lands" function in "functions.c"
	check_playable_lands(player);
	check_legend_rule(player, card, event);
	return 0;
}

int card_battle_mad_ronin(int player, int card, event_t event){
	attack_if_able(player, card, event);
	return bushido(player, card, event, 2);
}

int card_ben_ben_akki_hermit(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			target_definition_t td1;
			default_target_definition(player, instance->parent_card, &td1, TYPE_LAND);
			td1.allowed_controller = player;
			td1.illegal_abilities = 0;
			td1.required_subtype = SUBTYPE_MOUNTAIN;
			td1.illegal_state = TARGET_STATE_TAPPED;
			damage_creature(instance->targets[0].player, instance->targets[0].card, target_available(player, card, &td1), player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

ARCANE_WITH_SPLICE(card_blessed_breath, MANACOST_W(1)){
	// Target creature you control gains protection from the color of your choice until end of turn.
	// Splice onto Arcane |W
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = td.allowed_controller = player;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "ASHNODS_BATTLEGEAR");	// "Select target creature you control."
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0,
									   select_a_protection(player) | KEYWORD_RECALC_SET_COLOR, 0);
			}
	}
	return 0;
}

int card_blood_speaker(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		kill_card(player, card, KILL_SACRIFICE);
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, TYPE_PERMANENT, 0, SUBTYPE_DEMON, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_bloodthirsty_ogre(int player, int card, event_t event){

	/* |T: Put a devotion counter on ~.
	 * |T: Target creature gets -X/-X until end of turn, where X is the number of devotion counters on ~. Activate this ability only if you control a Demon. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	int can_activate = can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
						has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0);

	if( event == EVENT_CAN_ACTIVATE && can_activate ){
		if( player == AI ){
			if(count_counters(player, card, COUNTER_DEVOTION) > 0 && check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DEMON) ){
				return can_target(&td);
			}
		}
		else{
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 1;
		if( player != AI ){
			if( can_target(&td) && check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DEMON) &&
				has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
			  ){
				choice = do_dialog(player, player, card, -1, -1, " Add a counter\n Weaken a creature\n Cancel", 0);
			}
			else{
				choice = 0;
			}
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE")  ){
					instance->number_of_targets = 1;
					instance->info_slot = 1+choice;
					tap_card(player, card);
				}
		}
		else{
			instance->info_slot = 1+choice;
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			add_counter(instance->parent_controller, instance->parent_card, COUNTER_DEVOTION);
		}
		if( instance->info_slot == 2 && valid_target(&td) ){
			int amount = count_counters(instance->parent_controller, instance->parent_card, COUNTER_DEVOTION);
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -amount, -amount);
		}
	}

	if( player == AI && current_turn != player && can_activate && eot_trigger(player, card, event) ){
		tap_card(player, card);
		add_counter(player, card, COUNTER_DEVOTION);
	}

	return 0;
}

int card_brothers_yamazaki(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		int p;
		int kill_all = 0;
		int found = 0;
		for(p=0;p<2;p++){
			int c = 0;
			while(c < active_cards_count[p]){
					if( in_play(p, c) && get_id(p, c) == get_id(player, card) ){
						found++;
						if( found > 2 ){
							kill_all = 1;
							break;
						}
					}
					c++;
			}
		}
		if( kill_all == 1 ){
			manipulate_all(player, card, 1-player, 0, 0, 0, 0, 0, 0, get_id(player, card), 0, -1, 0, KILL_SACRIFICE);
			manipulate_all(player, card, player, 0, 0, 0, 0, 0, 0, get_id(player, card), 0, -1, 0, KILL_SACRIFICE);
		}
	}

	if ((event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS) &&
		affected_card != card && in_play(affected_card_controller, affected_card) &&
		get_id(affected_card_controller, affected_card) == get_id(player, card)
	  ){
		modify_pt_and_abilities(affected_card_controller, affected_card, event, 2, 2, 0);
		haste(affected_card_controller, affected_card, event);
	}

	return bushido(player, card, event, 1);
}

int card_budoka_gardener(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		if( count_permanents_by_type(player, TYPE_LAND) > 9 ){
			true_transform(player, instance->parent_card);
			verify_legend_rule(player, instance->parent_card, get_id(player, instance->parent_card));
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_dokai_weaver_of_life(int player, int card, event_t event){
	/* Dokai, Weaver of Life	|1|G (flipped)
	 * Legendary Creature - Human Monk 3/3
	 * |4|G|G, |T: Put an X/X |Sgreen Elemental creature token onto the battlefield, where X is the number of lands you control. */

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.pow = count_subtype(player, TYPE_LAND, -1);
		token.tou = count_subtype(player, TYPE_LAND, -1);
		token.color_forced = COLOR_TEST_GREEN;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XG(4,2), 0, NULL, NULL);
}

int card_burr_grafter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}

	return soulshift(player, card, event, 3, 1);
}

int card_bushi_tenderfoot(int player, int card, event_t event){
	double_faced_card(player, card, event);

	if( sengir_vampire_trigger(player, card, event, 2) ){
		true_transform(player, card);
		verify_legend_rule(player, card, get_id(player, card));
	}
	return 0;
}

int card_kenzo_the_hardhearted(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	return bushido(player, card, event, 2);
}

int card_cage_of_hands(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	int t_player = instance->targets[0].player;
	int t_card = instance->targets[0].card;

	if( in_play(player, card) && t_player > -1 ){
		cannot_attack(t_player, t_card, event);
		cannot_block(t_player, t_card, event);

		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1) ){
			return 1;
		}
		else if( event == EVENT_ACTIVATE ){
				 charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1);
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				bounce_permanent(player, instance->parent_card);
		}
	}

	return disabling_aura(player, card, event);
}

int card_call_to_glory(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_UNTAP);
		pump_subtype_until_eot(player, card, player, SUBTYPE_SAMURAI, 1, 1, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

ARCANE_WITH_SPLICE(card_candles_glow, MANACOST_XW(1,1)){
	// Prevent the next 3 damage that would be dealt to target creature or player this turn. You gain life equal to the damage prevented this way.
	// Splice onto Arcane |1|W
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				prevent_the_next_n_damage(player, card, instance->targets[0].player, instance->targets[0].card,
										  3, PREVENT_GAIN_LIFE, 0, 0);
			}
	}
	return 0;
}

// Cleanfall --> Tranquillity

int card_commune_with_nature(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		reveal_x_and_choose_a_card_type(player, card, 5, TYPE_CREATURE);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

ARCANE_WITH_SPLICE(card_consuming_vortex, MANACOST_XU(3,1)){
	// Return target creature to its owner's hand.
	// Splice onto Arcane |3|U
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
	}
	return 0;
}

// Cranial Extraction --> Memoricide

int card_cursed_ronin(int player, int card, event_t event){

	bushido(player, card, event, 1);

	return generic_shade(player, card, event, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0);
}

ARCANE_WITH_SPLICE(card_dampen_thought, MANACOST_XU(1,1)){
	// Target player puts the top four cards of his or her library into his or her graveyard.
	// Splice onto Arcane |1|U
	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				mill(instance->targets[0].player, 4);
			}
	}
	return 0;
}

ARCANE_WITH_SPLICE(card_desperate_ritual, MANACOST_XR(1,1)){
	// Add |R|R|R to your mana pool.
	// Splice onto Arcane |1|R
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		produce_mana(player, COLOR_RED, 3);
	}
	return 0;
}

int card_devoted_retainer(int player, int card, event_t event){
	return bushido(player, card, event, 1);
}

ARCANE(card_devouring_greed){
	// As an additional cost to cast ~, you may sacrifice any number of Spirits.
	// Target player loses 2 life plus 2 life for each Spirit sacrificed this way. You gain that much life.
	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int max = count_subtype(player, TYPE_PERMANENT, SUBTYPE_SPIRIT);
		int count = 0;
		while( count < max ){
				int cannot_cancel = 0;
				if( count > 0 ){
					cannot_cancel = 1;
				}
				count+=sacrifice(player, card, player, cannot_cancel, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
				if( count < max && player == HUMAN ){
					int choice = do_dialog(player, player, card, -1, -1, " Continue\n Stop", 0);
					if( choice == 1 ){
						break;
					}
				}
		}
		if( count > 0 ){
			td.allow_cancel = 0;
		}
		if( pick_target(&td, "TARGET_PLAYER") ){
			instance->info_slot = 2+(count * 2);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				lose_life(instance->targets[0].player, instance->info_slot);
				gain_life(player, instance->info_slot);
		   }
	}
	return 0;
}

int card_dosan_the_falling_leaf(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_MODIFY_COST_GLOBAL ){
		if( affected_card_controller != current_turn ){
			infinite_casting_cost();
		}
	}
	return 0;
}

int card_dripping_tongue_zubera(int player, int card, event_t event){
	/* Dripping-Tongue Zubera	|1|G
	 * Creature - Zubera Spirit 1/2
	 * When ~ dies, put a 1/1 colorless Spirit creature token onto the battlefield for each Zubera that died this turn. */

	if( this_dies_trigger(player, card, event, 2) ){
		generate_tokens_by_id(player, card, CARD_ID_SPIRIT, get_zubera_count());
	}

	return 0;
}

int card_earthshaker(int player, int card, event_t event){

	if( arcane_spirit_spell_trigger(player, card, event, 2) ){
		damage_all(player, card, player, 2, 0, 0, KEYWORD_FLYING, 1, 0, 0, 0, 0, 0, 0, -1, 0);
		damage_all(player, card, 1-player, 2, 0, 0, KEYWORD_FLYING, 1, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

ARCANE(card_eerie_procession){
	// Search your library for an Arcane card, reveal that card, and put it into your hand. Then shuffle your library.
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_ANY, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
	}
	return 0;
}

static int ec_damage_prevention(int player, int card){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.special = TARGET_SPECIAL_DAMAGE_LEGENDARY_CREATURE;
	td1.required_type = 0;

	if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 2) && can_use_activated_abilities(player, card) && can_target(&td1) &&
		(land_can_be_played & LCBP_DAMAGE_PREVENTION)
	  ){
		return 99;
	}
	return 0;
}

int card_eiganjo_castle(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.special = TARGET_SPECIAL_DAMAGE_LEGENDARY_CREATURE;
	td1.required_type = 0;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		return ec_damage_prevention(player, card) ? 99 : mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ec_damage_prevention(player, card) ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Protect a Legend\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1);
				if( spell_fizzled != 1 && pick_target(&td1, "TARGET_DAMAGE") ){
					instance->info_slot = 1;
				}
				else{
					remove_state(player, card, STATE_TAPPED);
					spell_fizzled = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				if( valid_target(&td1) ){
					card_instance_t *dmg = get_card_instance( instance->targets[0].player, instance->targets[0].card );
					if( dmg->info_slot <= 2 ){
						dmg->info_slot = 0;
					}
					else{
						dmg->info_slot -= 2;
					}
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

int card_eight_and_a_half_tails(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		int result = card_spiketail_hatchling(player, card, event);
		if( result && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			return result;
		}

		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			return can_target(&td1);
		}
	}
	else if( event == EVENT_ACTIVATE){
		// If there is a spell on the stack, assume that was the choice
		if( card_on_stack_controller > -1){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->targets[0].player = card_on_stack_controller;
				instance->targets[0].card = card_on_stack;
				instance->info_slot = 2;
			}
			return 0;
		}

		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && can_target(&td) ){
			int clr = get_sleighted_color(player, card, COLOR_WHITE);
			const char* color = (clr == COLOR_BLACK ? "BLACK"
								 : clr == COLOR_BLUE ? "BLUE"
								 : clr == COLOR_GREEN ? "GREEN"
								 : clr == COLOR_RED ? "RED"
								 : "white");
			char buf[128];
			sprintf(buf, " Gains Prot. from %s\n Becomes %s", color, color);
			choice = do_dialog(player, player, card, -1, -1, buf, 0);
		}
		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1);
			if( spell_fizzled != 1 ){
				if( pick_target(&td, "TARGET_PERMANENT") ){
					instance->number_of_targets = 1;
					instance->info_slot = 0;
				}
			}
		}
		else{
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				if( pick_target(&td1, "TARGET_PERMANENT") ){
					instance->number_of_targets = 1;
					instance->info_slot = 1;
				}
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 0 && valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_PROT_WHITE, 0 );
		}
		else if( (instance->info_slot == 1 && valid_target(&td1) )
				 || instance->info_slot == 2 ){
			change_color(player, card, instance->targets[0].player, instance->targets[0].card, COLOR_TEST_WHITE, CHANGE_COLOR_SET|CHANGE_COLOR_END_AT_EOT);
		}
	}
	return 0;
}

int card_ember_fist_zubera(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, get_zubera_count());
		}
	}

	return 0;
}

ARCANE(card_ethereal_haze){//UNUSEDCARD
	// Prevent all damage that would be dealt by creatures this turn.
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL ){
		return card_fog(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		fog_special(player, card, ANYBODY, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

ARCANE(card_eye_of_nowhere){
	// Return target permanent to its owner's hand.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PERMANENT");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
	}
	return 0;
}

ARCANE(card_feast_of_worms){
	// Destroy target land. If that land was legendary, its controller sacrifices another land.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_LAND");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			int sac = is_legendary(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			if( sac == 1 ){
				impose_sacrifice(player, card, instance->targets[0].player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}
	return 0;
}

int card_floating_dream_zubera(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		draw_cards(player, get_zubera_count());
	}

	return 0;
}

int card_forbidden_orchard(int player, int card, event_t event){
	/* Forbidden Orchard	""
	 * Land
	 * |T: Add one mana of any color to your mana pool.
	 * Whenever you tap ~ for mana, target opponent puts a 1/1 colorless Spirit creature token onto the battlefield. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIRIT, &token);
		token.t_player = 1-player;
		generate_token(&token);
	}
	return mana_producer(player, card, event);
}

int card_generals_kabuto(int player, int card, event_t event){

	if( is_equipping(player, card) ){
		card_instance_t *instance= get_card_instance(player, card);
		prevent_all_damage(instance->targets[8].player, instance->targets[8].card, event);
	}

	return vanilla_equipment(player, card, event, 2, 0, 0, KEYWORD_SHROUD, 0);
}

// Ghostly Prison --> Propaganda

static int test_not_in_array4(int iid, int arrptr, int player, int card)
{
  int* arr = (int*)arrptr;
  return iid != arr[0] && iid != arr[1] && iid != arr[2] && iid != arr[3];
}

int card_gifts_ungiven(int player, int card, event_t event){return 0;}
int card_gifts_ungiven2(int player, int card, event_t event)
{
  /* Search your library for four cards with different names and reveal them. Target opponent chooses two of those cards. Put the chosen cards into your
   * graveyard and the rest into your hand. Then shuffle your library. */
  if (event == EVENT_CAN_CAST)
	return opponent_is_valid_target(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	target_opponent(player, card);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (!opponent_is_valid_target(player, card))
		{
		  cancel = 1;
		  kill_card(player, card, KILL_DESTROY);
		  return 0;
		}

	  if (deck_ptr[player][0] == -1)
		{
		  kill_card(player, card, KILL_DESTROY);
		  return 0;
		}

	  int chosen[4] = {-1, -1, -1, -1};

	  test_definition_t test;
	  new_default_test_definition(&test, 0, "");
	  test.no_shuffle = 1;
	  test.value_for_special_selection_function = (int)chosen;
	  test.special_selection_function = test_not_in_array4;

	  int num_chosen;
	  for (num_chosen = 0; num_chosen < 4; ++num_chosen)
		{
			if (ai_is_speculating != 1)
				sprintf(test.message, "Select four cards with different names: #%d", num_chosen + 1);

			int pos = new_select_a_card(player, player, TUTOR_FROM_DECK, 0, AI_MAX_VALUE, -1, &test);
			if(pos == -1)
				break;
			chosen[num_chosen] = deck_ptr[player][pos];
			remove_card_from_deck(player, pos);
		}

	  /* Shuffle library first, since Manalink triggers won't wait until this is done resolving; we don't want any weird sequence of triggers caused by putting
	   * cards into the graveyard to have any chance of doing anything with the unshuffled top of the library. */
	  shuffle(player);

	  if (num_chosen <= 2)	// All go to graveyard from library.
		{
		  int to_mill = 0;
		  if (chosen[1] != -1)
			{
			  put_on_top_of_deck(player, add_card_to_hand(player, chosen[1]));
			  ++to_mill;
			}
		  if (chosen[0] != -1)
			{
			  put_on_top_of_deck(player, add_card_to_hand(player, chosen[0]));
			  ++to_mill;
			}

		  mill(player, to_mill);
		}
	  else
		{
		  new_default_test_definition(&test, 0, "Select a card to put into opponent's graveyard.");
		  int i;
		  for (i = 0; i < 2; ++i)
			{
			  int selected = select_card_from_zone(1-player, player, chosen, num_chosen, 1, AI_MAX_VALUE, -1, &test);
			  put_on_top_of_deck(player, add_card_to_hand(player, chosen[selected]));
			  // Move later choices down
			  for (; selected < 3; ++selected)
				chosen[selected] = chosen[selected + 1];
			  chosen[3] = -1;
			  --num_chosen;
			}

		  mill(player, 2);

		  for (i = 0; i < 4; ++i)
			if (chosen[i] != -1)
			  add_card_to_hand(player, chosen[i]);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

ARCANE_WITH_SPLICE(card_glacial_ray, MANACOST_XR(1,1)){
	// ~ deals 2 damage to target creature or player.
	// Splice onto Arcane |1|R
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, 2);
			}
	}
	return 0;
}

static int effect_glimpse_of_nature(int player, int card, event_t event){
	if(trigger_condition == TRIGGER_SPELL_CAST && player == affected_card_controller
	   && player == reason_for_trigger_controller && player == trigger_cause_controller && card == affected_card )
	{
		card_instance_t *instance = get_card_instance(trigger_cause_controller, trigger_cause);
		if( cards_data[ instance->internal_card_id].type & TYPE_CREATURE ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
				draw_a_card(player);
			}
		}
	}
	if( eot_trigger(player, card, event ) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}


int card_glimpse_of_nature(int player, int card, event_t event){

	if(event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI){
		int i;
		for(i=1; i<15; i++){
			if( has_mana(player, COLOR_ANY, i) ){
				ai_modifier+=5;
			}
			else{
				break;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &effect_glimpse_of_nature );
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_godo_bandit_warlord(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	card_instance_t* instance = get_card_instance(player, card);

	// When ~ enters the battlefield, you may search your library for an Equipment card and put it onto the battlefield. If you do, shuffle your library.
	if( comes_into_play(player, card, event) ){
		if( do_dialog(player, player, card, -1, -1, " Search for an Equipment\n Pass", 0) == 0 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an Equipment card.");
			this_test.subtype = SUBTYPE_EQUIPMENT;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	// Whenever ~ attacks for the first time each turn, untap it and all Samurai you control. After this phase, there is an additional combat phase.
	if (instance->info_slot == 0 && declare_attackers_trigger(player, card, event, 0, player, card)){

		instance->info_slot = 1;

		untap_card(player, card);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_SAMURAI;
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);

		create_legacy_effect(player, card, &finest_hour_legacy);
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}


	return 0;
}

int card_gutwrencher_oni(int player, int card, event_t event){

	if( current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		int trig = 1-check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_OGRE);
		if( trig == 1  ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					discard(player, 0, player);
			}
		}
	}
	return 0;
}

int card_hana_kami(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) && count_graveyard_by_subtype(player, SUBTYPE_ARCANE) > 0){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0);
			if( spell_fizzled != 1 ){
				int selected = select_a_card(player, player, 2, 0, 1, -1, TYPE_ANY, 0, SUBTYPE_ARCANE, 0, 0, 0, 0, 0, -1, 0);
				if( selected != -1 ){
					instance->targets[0].player = selected;
					const int *grave = get_grave(player);
					instance->targets[0].card = grave[selected];
					kill_card(player, card, KILL_SACRIFICE);
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION){
			int selected = instance->targets[0].player;
			const int *grave = get_grave(player);
			if( instance->targets[0].card == grave[selected] ){
				add_card_to_hand(player, grave[selected]);
				remove_card_from_grave(player, selected);
			}
	}

	return 0;
}

int card_he_who_hungers(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && can_sorcery_be_played(player, event) ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		if( would_valid_target(&td) ){
			return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
		}
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0)){
					instance->targets[0].player = 1-player;
					instance->targets[0].card = -1;
					instance->number_of_targets = 1;
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION){
			if( valid_target(&td) ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);

				ec_definition_t this_definition;
				default_ec_definition(instance->targets[0].player, player, &this_definition);
				new_effect_coercion(&this_definition, &this_test);
			}
	}

	return soulshift(player, card, event, 4, 1);
}

int card_heart_kami(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) && can_target(&td) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_ARTIFACT") ){
				int cost = get_cmc( instance->targets[0].player, instance->targets[0].card );
				if( has_mana(player, COLOR_COLORLESS, cost) ){
					charge_mana(player, COLOR_COLORLESS, cost);
					if( spell_fizzled != 1 ){
						kill_card(player, card, KILL_SACRIFICE);
					}
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
	}
	return 0;
}

ARCANE_WITH_SPLICE(card_hideous_laughter, MANACOST_XB(3,2)){
	// All creatures get -2/-2 until end of turn.
	// Splice onto Arcane |3|B|B
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_creatures_until_eot(player, card, ANYBODY, 0, -2,-2, 0,0, NULL);
	}
	return 0;
}

int card_hikari_twilight_guardian(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( arcane_spirit_spell_trigger(player, card, event, 1+player) ){
		remove_until_eot(player, card, player, card);
	}

	return 0;
}

int card_hinder(int player, int card, event_t event)
{
  /* Hinder	|1|U|U
   * Instant
   * Counter target spell. If that spell is countered this way, put that card on the top or bottom of its owner's library instead of into that player's
   * graveyard. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (counterspell_validate(player, card, NULL, 0))
		{
		  set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		  if (do_dialog(player, player, card, instance->targets[0].player, instance->targets[0].card, " Put on top of library\n Put on bottom of library", 1) == 0)
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		  else
			put_on_bottom_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}
  else
	return counterspell(player, card, event, NULL, 0);
}

int card_hisoka_minamo_sensei(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 2, 0, 1, 0, 0, 0) &&
		hand_count[player] > 0
	  ){
		instance->targets[1].player = card_on_stack_controller;
		instance->targets[1].card = card_on_stack;
		return card_spiketail_hatchling(player, card, event);
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 2, 0, 1, 0, 0, 0);
			if( spell_fizzled != 1  ){
				int selected = -1;
				if( player != AI ){
					selected = select_a_card(player, player, TUTOR_FROM_HAND, 0, 1, -1, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1,0);
				}
				else{
					int count = 0;
					while( count < active_cards_count[player] ){
							if( in_hand(player, count) && get_cmc(player, count) == get_cmc(instance->targets[1].player, instance->targets[1].card) ){
								if( get_base_value(player, count) >= get_base_value(instance->targets[1].player, instance->targets[1].card) ){
									selected = count;
									break;
								}
							}
							count++;
					}
				}
				if( selected != -1 ){
					instance->info_slot = get_cmc(player, selected);
					discard_card(player, selected);
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( get_cmc(instance->targets[1].player, instance->targets[1].card) == instance->info_slot ){
				set_flags_when_spell_is_countered(player, instance->parent_card, instance->targets[1].player, instance->targets[1].card);
				kill_card( instance->targets[1].player, instance->targets[1].card, KILL_DESTROY );
			}
	}
	return 0;
}

int card_honden_of_nights_reach (int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		if( valid_target(&td) ){
			new_multidiscard(1-player, count_subtype(player, TYPE_ENCHANTMENT, SUBTYPE_SHRINE), 0, player);
		}
	}

	return global_enchantment(player, card, event);
}

int card_honden_of_seeing_winds (int player, int card, event_t event){
	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		draw_cards(player, count_subtype(player, TYPE_ENCHANTMENT, SUBTYPE_SHRINE));
	}

	return global_enchantment(player, card, event);
}

int card_honden_of_lifes_web (int player, int card, event_t event){
	/* Honden of Life's Web	|4|G
	 * Legendary Enchantment - Shrine
	 * At the beginning of your upkeep, put a 1/1 colorless Spirit creature token onto the battlefield for each Shrine you control. */

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		generate_tokens_by_id(player, card, CARD_ID_SPIRIT, count_subtype(player, TYPE_ENCHANTMENT, SUBTYPE_SHRINE));
	}

	return global_enchantment(player, card, event);
}

int card_honden_of_infinite_rage(int player, int card, event_t event){
	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		card_instance_t* instance = get_card_instance(player, card);
		instance->number_of_targets = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_target0(player, card, count_subtype(player, TYPE_ENCHANTMENT, SUBTYPE_SHRINE));
		}
	}

	return global_enchantment(player, card, event);
}

int card_honden_of_cleansing_fire (int player, int card, event_t event){
	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		gain_life(player, 2*count_subtype(player, TYPE_ENCHANTMENT, SUBTYPE_SHRINE));
	}

	return global_enchantment(player, card, event);
}

int card_honor_worn_shaku(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.illegal_state = TARGET_STATE_TAPPED;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT );
	td1.preferred_controller = player;
	td1.required_subtype = SUBTYPE_LEGEND;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
			return 1;
		}
		if( is_tapped(player, card) && can_use_activated_abilities(player, card) && can_target(&td1) &&
			has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
		  ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE ){
		int ai_choice = 0;
		if( ! paying_mana() && is_tapped(player, card) && can_use_activated_abilities(player, card) &&
			can_target(&td1) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Untap\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_PERMANENT") ){
				instance->number_of_targets = 1;
				if( is_legendary(instance->targets[0].player, instance->targets[0].card) ){
					tap_card(instance->targets[0].player, instance->targets[0].card);
					instance->info_slot = 1;
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
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				untap_card(player, instance->parent_card);
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


int card_horobi_deaths_wail(int player, int card, event_t event){
  check_legend_rule(player, card, event);

  const target_t* targets = any_creature_becomes_target_of_spell_or_effect(player, card, event, ANYBODY);
  if (targets)
	for (; targets->player != -1; ++targets)
	  kill_card(targets->player, targets->card, KILL_DESTROY);

  return 0;
}

// Humble Budoka --> vanilla

int card_iname_death_aspect(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( do_dialog(player, player, card, -1, -1, " Spirit search\n Pass", count_deck(player) > 10 ? 0 : 1) == 0 ){
			int selected = global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GRAVE, 0, 4, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
			while( selected != -1 ){
					selected = global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GRAVE, 0, 4, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}
	return 0;
}

int card_iname_life_aspect(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) && special_count_grave(player, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0) > 0 ){
		int selected = global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, 4, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
		while( selected == -1 ){
				global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, 4, TYPE_PERMANENT, 0, SUBTYPE_SPIRIT, 0, 0, 0, 0, 0, -1, 0);
		}
	}
	return 0;
}

int card_indomitable_will(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 1, 2, 0, 0, 0, 0, 0);
}

int card_jade_idol(int player, int card, event_t event){

	if( arcane_spirit_spell_trigger(player, card, event, 2) ){
		add_a_subtype(player, card, SUBTYPE_SPIRIT);
		artifact_animation(player, card, player, card, 1, 4, 4, 0, 0);
	}

	return 0;
}

int card_journeyers_kite(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a basic land card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, msg);
		this_test.subtype = SUBTYPE_BASIC;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 3, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_jugan_rising_star(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		card_instance_t *instance= get_card_instance(player, card);

		state_untargettable(player, card, 1);

		if( can_target(&td) ){
			int count =  5;
			while( count > 0 ){
					if( pick_target(&td, "TARGET_CREATURE") ){
						if( target_available(player, card, &td) == 1 ){
							add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 5);
							count = 0;
						}
						else{
							add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
						}
						count--;
					}
					else{
						break;
					}
			}
		}
	}

	return 0;
}

// Jukai Messenger -> vanilla

static int junkyo_bell_effect(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	modify_pt_and_abilities(instance->targets[0].player, instance->targets[0].card, event, instance->targets[1].player, instance->targets[1].player, 0);

	if( eot_trigger(player, card, event) ){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
	}

	return 0;
}

int card_junkyo_bell(int player, int card, event_t event){

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.preferred_controller = player;

		card_instance_t *instance= get_card_instance(player, card);

		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			int legacy = create_targetted_legacy_effect(player, card, &junkyo_bell_effect, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg= get_card_instance(player, legacy);
			leg->targets[1].player = count_permanents_by_type(player, TYPE_CREATURE);
		}
	}

	return 0;
}

int card_jushi_apprentice(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
		if( hand_count[player] > 8 ){
			true_transform(player, instance->parent_card);
			verify_legend_rule(player, instance->parent_card, get_id(player, instance->parent_card));
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 2, 0, 1, 0, 0, 0, 0, 0, 0);
}

int card_tomoya_the_revealer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance= get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			draw_cards(instance->targets[0].player, hand_count[player]);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 3, 0, 2, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_kami_of_ancient_law(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_ENCHANTMENT");
}

int card_kami_of_fires_roar(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance= get_card_instance(player, card);

	if( can_target(&td) && arcane_spirit_spell_trigger(player, card, event, 2) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}

	return 0;
}

int card_kami_of_the_hunt(int player, int card, event_t event){
	if( arcane_spirit_spell_trigger(player, card, event, 2) ){
		pump_until_eot(player, card, player, card, 1, 1);
	}

	return 0;
}

int card_kami_of_waning_moon(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance= get_card_instance(player, card);

	if( can_target(&td) && arcane_spirit_spell_trigger(player, card, event, 2) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_FEAR);
		}
	}

	return 0;
}

int card_kashi_tribe_reaver(int player, int card, event_t event){

	freeze_when_damage(player, card, event);

	return regeneration(player, card, event, 1, 0, 0, 1, 0, 0);
}

int card_keiga(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	if( graveyard_from_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);
		instance->state |= STATE_CANNOT_TARGET;
		if( target_available(player, card, &td) ){
			pick_target(&td, "TARGET_CREATURE");
			gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_kiki_jiki_mirror_breaker(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 17) ){ return 0; }

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.required_subtype = SUBTYPE_LEGEND;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	card_instance_t *instance = get_card_instance(player, card);

	haste(player, card, event);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		if( ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td);
		}
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_NONLEGENDARY_CREATURE") ){
				instance->number_of_targets = 1;
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				token_generation_t token;
				copy_token_definition(player, card, &token, instance->targets[0].player, instance->targets[0].card);
				token.legacy = 1;
				token.special_code_for_legacy = &haste_and_sacrifice_eot;
				generate_token(&token);
			}
	}
	return 0;
}

int card_kiku_night_flower(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance= get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, get_power(instance->targets[0].player, instance->targets[0].card),
							instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, 2, 2, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

// Kitsune Blademaster --> Devoted Retainer

int card_kitsune_healer(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && can_target(&td) &&
		! is_tapped(player, card) && ! is_sick(player, card)
	  ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return 0x63;
		}
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_DAMAGE") ){
				tap_card(player, card);
				instance->number_of_targets = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( target->damage_target_card != -1 && is_legendary(target->damage_target_player, target->damage_target_card) ){
				target->info_slot = 0;
			}
			else{
				if( target->info_slot > 0 ){
					target->info_slot--;
				}
			}
	}
	return 0;
}

int card_kitsune_mystic(int player, int card, event_t event){
	double_faced_card(player, card, event);
	if( eot_trigger(player, card, event) && count_auras_enchanting_me(player, card) > 1 ){
		true_transform(player, card);
	}
	return 0;
}

static int check_targettable_auras_for_autumn_tail(int player, int card, int mode){
	int i;
	int par = -1;
	int result = -1;
	for(i=0; i<2; i++){
		if( player == HUMAN || (player == AI && i != player) ){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && has_subtype(i, count, SUBTYPE_AURA_CREATURE) ){
						card_instance_t *targ = get_card_instance( i, count );
						int (*ptFunction)(int, int, event_t) = (void*)cards_data[targ->internal_card_id].code_pointer;
						if( ptFunction(i, count, EVENT_CAN_MOVE_AURA) ){
							if( mode == 0 ){
								return 1;
							}
							if( mode == 1 && get_cmc(i, count) > par ){
								par = get_cmc(i, count);
								result = count;
							}
						}
					}
					count++;
			}
		}
	}
	return result;
}

int card_autumn_tail_kitsune_sage(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ENCHANTMENT);
	td1.required_subtype = SUBTYPE_AURA_CREATURE;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			check_targettable_auras_for_autumn_tail(player, card, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			int good = 1;
			if( player == HUMAN ){
				if( pick_target(&td1, "TARGET_ENCHANTMENT") ){
					instance->number_of_targets = 1;
					card_instance_t *targ = get_card_instance( instance->targets[0].player, instance->targets[0].card );
					int (*ptFunction)(int, int, event_t) = (void*)cards_data[targ->internal_card_id].code_pointer;
					if( ! ptFunction(instance->targets[0].player, instance->targets[0].card, EVENT_CAN_MOVE_AURA) ){
						good = 0;
					}
				}
			}
			else{
				int result = check_targettable_auras_for_autumn_tail(player, card, 1);
				if( result > -1 ){
					instance->targets[0].player = player;
					instance->targets[0].card = result;
					instance->number_of_targets = 1;
				}
				else{
					good = 0;
				}
			}
			if( good == 1 ){
				card_instance_t *targ = get_card_instance( instance->targets[0].player, instance->targets[0].card );
				int (*ptFunction)(int, int, event_t) = (void*)cards_data[targ->internal_card_id].code_pointer;
				if( ! ptFunction(instance->targets[0].player, instance->targets[0].card, EVENT_MOVE_AURA) ){
					spell_fizzled = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			card_instance_t *targ = get_card_instance( instance->targets[0].player, instance->targets[0].card );
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[targ->internal_card_id].code_pointer;
			ptFunction(instance->targets[0].player, instance->targets[0].card, EVENT_RESOLVE_MOVING_AURA);
		}
	}

	return 0;
}

int card_kodama_of_the_south_tree(int player, int card, event_t event){
	// Whenever you cast a Spirit or Arcane spell, each other creature you control gets +1/+1 and gains trample until end of turn.
	if( arcane_spirit_spell_trigger(player, card, event, 2) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "");
		test.not_me = 1;
		pump_creatures_until_eot(player, card, player, 0, 1,1, KEYWORD_TRAMPLE,0, &test);
	}

	return 0;
}

int card_kodama_of_the_north_tree(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	return 0;
}

ARCANE_WITH_SPLICE(card_kodamas_might, MANACOST_G(1)){
	// Target creature gets +2/+2 until end of turn.
	// Splice onto Arcane |G
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t* instance = get_card_instance(player, card);
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
			}
	}
	return 0;
}

ARCANE(card_kodamas_reach){
	/* Search your library for two basic land cards, reveal those cards, and put one onto the battlefield tapped and the other into your hand. Then shuffle your
	 * library. */
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		cultivate(player);
	}
	return 0;
}

int card_kokusho_the_evening_star(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( graveyard_from_play(player, card, event) ){
		int amount = lose_life(1-player, 5);
		gain_life(player, amount);
	}

	return 0;
}

int card_kondas_banner(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( is_equipping(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) &&
			! affect_me(instance->targets[8].player, instance->targets[8].card)
		  ){
			if( shares_creature_subtype(instance->targets[8].player, instance->targets[8].card, affected_card_controller, affected_card) ){
				event_result++;
			}
			if( get_color(instance->targets[8].player, instance->targets[8].card) & get_color(affected_card_controller, affected_card) ){
				event_result++;
			}
		}
	}
	else if( event == EVENT_ACTIVATE ){
			int new_cost = get_updated_equip_cost(player, card, 2);
			charge_mana( player, COLOR_COLORLESS, new_cost);
			if( spell_fizzled != 1){
				td.allow_cancel = 0;
				if( is_equipping(player, card) ){
					state_untargettable(instance->targets[8].player, instance->targets[8].card, 1);
				}
				if( select_target(player, card, &td, "Select Creature to Equip", NULL) ){
					instance->number_of_targets = 1;
					if( is_legendary(instance->targets[0].player, instance->targets[0].card) ){
						instance->targets[1].player = instance->targets[0].player;
						instance->targets[1].card = instance->targets[0].card;
					}
					else{
						if( is_equipping(player, card) ){
							state_untargettable(instance->targets[8].player, instance->targets[8].card, 0);
						}
						spell_fizzled = 1;
					}
				}
				if( is_equipping(player, card) ){
					state_untargettable(instance->targets[8].player, instance->targets[8].card, 0);
				}

			}
	}
	else{
		return basic_equipment(player, card, event, 2);
	}
	return 0;
}

int card_kondas_hatamoto(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[7].player = 0;
		int count = 0;
		while( count < active_cards_count[player] ){
				if( in_play(player, count) && has_subtype(player, count, SUBTYPE_SAMURAI) && is_legendary(player, count) ){
					instance->targets[7].player++;
				}
				count++;
		}
	}


	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == affected_card_controller && player == trigger_cause_controller
	  ){
		int trig = 0;
		if( make_test_in_play(trigger_cause_controller, trigger_cause, -1, TYPE_PERMANENT, 0, SUBTYPE_SAMURAI, 0, 0, 0, 0, 0, -1, 0) ){
			if( is_legendary(trigger_cause_controller, trigger_cause) ){
				trig = 1;
			}
		}
		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					instance->targets[7].player++;
			}
		}
	}

	if( trigger_condition == TRIGGER_LEAVE_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == affected_card_controller && player == trigger_cause_controller
	  ){
		int trig = 0;
		if( make_test_in_play(trigger_cause_controller, trigger_cause, -1, TYPE_PERMANENT, 0, SUBTYPE_SAMURAI, 0, 0, 0, 0, 0, -1, 0) ){
			if( is_legendary(trigger_cause_controller, trigger_cause) ){
				trig = 1;
			}
		}
		if( trig > 0 ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					instance->targets[7].player--;
			}
		}
	}

	if( instance->targets[7].player> 0 ){
		vigilance(player, card, event);
		modify_pt_and_abilities(player, card, event, 1, 2, 0);
	}

	return bushido(player, card, event, 1);
}

int card_konda_lord_of_eiganjo(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	vigilance(player, card, event);
	indestructible(player, card, event);
	return bushido(player, card, event, 5);
}

static int rfg_when_die(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(p, c) ){
			card_instance_t *affected = get_card_instance(p, c);
			if( affected->kill_code > 0 && affected->kill_code < 4){
				affected->kill_code = KILL_REMOVE;
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_kumano_master_yamabushi(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	check_legend_rule(player, card, event);

	damage_effects(player, card, event);

	if( instance->targets[1].player > 2 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		int i;
		for(i=2; i<instance->targets[1].player; i++){
			create_targetted_legacy_effect(player, card, &rfg_when_die, instance->targets[i].player, instance->targets[i].card);
		}
		instance->targets[1].player = 2;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td)  ){
				damage_creature_or_player(player, card, event, 1);
			}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_kuro_pit_lord(int player, int card, event_t event){

  card_instance_t *instance = get_card_instance( player, card );

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE );

	check_legend_rule(player, card, event);

	basic_upkeep(player, card, event, 0, 4, 0, 0, 0, 0);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td)  ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 1, &td, "TARGET_CREATURE");
}

ARCANE(card_lava_spike){
	// ~ deals 3 damage to target player.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				damage_player(instance->targets[0].player, 3, player, card);
			}
	}
	return 0;
}

// Lantern Kami --> vanilla

int card_long_forgotten_gohei(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( has_subtype(affected_card_controller, affected_card, SUBTYPE_ARCANE) ){
			COST_COLORLESS--;
		}
	}
	return boost_creature_type(player, card, event, SUBTYPE_SPIRIT, 1, 1, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
}

int card_marrow_gnawer(int player, int card, event_t event){
	/* Marrow-Gnawer	|3|B|B
	 * Legendary Creature - Rat Rogue 2/3
	 * Rat creatures have fear.
	 * |T, Sacrifice a Rat: Put X 1/1 |Sblack Rat creature tokens onto the battlefield, where X is the number of Rats you control. */

	check_legend_rule(player, card, event);

	if (event == EVENT_ABILITIES && has_subtype(affected_card_controller, affected_card, SUBTYPE_RAT) && !is_humiliated(player, card)){
		fear(affected_card_controller, affected_card, event);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_RAT, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
				sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_RAT, 0, 0, 0, 0, 0, -1, 0)
			  ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
				generate_tokens_by_id(player, card, CARD_ID_RAT, count_subtype(player, TYPE_PERMANENT, SUBTYPE_RAT));
	}

	return 0;
}

int card_matsu_tribe_decoy(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana_for_activated_ability(player, card, 2, 0, 0, 1, 0, 0) ){
		return can_target(&td);
	}

	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 1, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets=1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td)  ){
				target_must_block_me(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1);
			}
	}

	return freeze_when_damage(player, card, event);
}

int card_meloku_the_clouded_mirror(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(1), 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(1)) && new_pick_target(&td, "Choose a land to bounce", 0, 1 | GS_LITERAL_PROMPT) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card );
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ILLUSION, &token);
		token.pow = 1;
		token.tou = 1;
		generate_token(&token);
	}

	return 0;
}

int card_minamo_school_at_waters_edge(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_LEGEND;
	if (player == AI){
		td.required_state = TARGET_STATE_TAPPED;
		td.special = TARGET_SPECIAL_NOT_ME;
	}

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if (!paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_U(2)) && can_use_activated_abilities(player, card) && can_target(&td)){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Untap a Legend\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
			instance->number_of_targets = 0;
			instance->state |= STATE_TAPPED;
			if (charge_mana_for_activated_ability(player, card, MANACOST_U(1)) && pick_target(&td, "TARGET_LEGENDARY_PERMANENT")){
				instance->info_slot = 1;
			} else {
				untap_card_no_event(player, card);
			}
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				if(valid_target(&td) ){
					untap_card(instance->targets[0].player, instance->targets[0].card);
				}
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
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

int card_myojin_of_cleansing_fire(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		APNAP(p, {new_manipulate_all(instance->parent_controller, instance->parent_card, p, &this_test, KILL_DESTROY);};);
	}

	return myojin(player, card, event);
}

int card_myojin_of_infinite_rage(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		manipulate_all(player, card, player, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
		manipulate_all(player, card, 1-player, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0, KILL_DESTROY);
	}
	return myojin(player, card, event);
}

int card_myojin_of_lifes_web(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		while( selected != -1 ){
				selected = global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}
	return myojin(player, card, event);
}

int card_myojin_of_nights_reach(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		new_discard_all(1-player, player);
	}
	return myojin(player, card, event);
}

int card_myojin_of_seeing_winds(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, count_permanents_by_type(player, TYPE_PERMANENT));
	}
	return myojin(player, card, event);
}

int card_nagao_bound_by_honor(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	// Whenever ~ attacks, Samurai creatures you control get +1/+1 until end of turn.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		pump_subtype_until_eot(player, card, player, SUBTYPE_SAMURAI, 1, 1, 0, 0);
	}
	return bushido(player, card, event, 1);
}

int card_natures_will(int player, int card, event_t event){

	card_instance_t* damage = combat_damage_being_dealt(event);
	if( damage &&
		damage->damage_source_player == player &&
		damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER) &&
		(damage->targets[3].player & TYPE_CREATURE)	// probably redundant to status check
	  ){
		get_card_instance(player, card)->info_slot = 1;
	}

	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player ){
			card_instance_t* instance = get_card_instance(player, card);
			if (instance->info_slot <= 0){
				return 0;
			}
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					manipulate_all(player, card, 1-player, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_TAP);
					manipulate_all(player, card, player, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_UNTAP);
					instance->info_slot = 0;
			}
	}

	return global_enchantment(player, card, event);
}

int card_nezumi_bone_reader(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			discard(instance->targets[0].player, 0, player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_SORCERY_BE_PLAYED+GAA_CAN_TARGET+GAA_SACRIFICE_CREATURE, 0, 1, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_nezumi_cutthroat(int player, int card, event_t event){
	fear(player, card, event);
	cannot_block(player, card, event);
	return 0;
}

int card_nezumi_graverobber(int player, int card, event_t event ){

	double_faced_card(player, card, event);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0) && can_use_activated_abilities(player, card) &&
		count_graveyard(1-player) > 0
	  ){
		return ! graveyard_has_shroud(2);
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				char buffer[100];
				scnprintf(buffer, 100, "Select an card to exile.");
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, buffer);
				int selected = new_select_a_card(player, 1-player, TUTOR_FROM_GRAVE, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					const int *grave = get_grave(1-player);
					instance->targets[0].player = selected;
					instance->targets[0].card = grave[selected];
				}
				else{
					spell_fizzled = 1;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			const int *grave = get_grave(1-player);
			int selected = instance->targets[0].player;
			if( grave[selected] == instance->targets[0].card ){
				rfg_card_from_grave(1-player, selected);
				if( grave[0] == -1 ){
					true_transform(player, instance->parent_card);
					verify_legend_rule(player, instance->parent_card, CARD_ID_NIGHTEYES_THE_DESECRATOR);
				}
			}
	}
	return 0;
}

int card_nighteyes_the_desecrator(int player, int card, event_t event){

	/* Nighteyes the Desecrator
	 * Legendary Creature - Rat Wizard
	 * 4/2
	 * |4|B: Put target creature card from a graveyard onto the battlefield under your control. */

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 4, 1, 0,0,0,0)  ){
		if( has_dead_creature(2) ){
			return ! graveyard_has_shroud(2);
		}
	}
	else if( event == EVENT_ACTIVATE ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select target creature card.");
			if (charge_mana_for_activated_ability(player, card, 4, 1, 0,0,0,0)){
				select_target_from_either_grave(player, card, 0, AI_MAX_CMC, AI_MAX_CMC, &test, 0, 1);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t* instance = get_card_instance(player, card);
			int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
			if( selected != -1 ){
				reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_DEFAULT);
			}
	}
	return 0;
}

int card_nezumi_shortfang(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if(event == EVENT_RESOLVE_ACTIVATION){
		discard( 1-player, 0, player);
		if( hand_count[1-player] < 1 ){
			true_transform(player, instance->parent_card);
			verify_legend_rule(player, instance->parent_card, CARD_ID_STABWHISKER_THE_ODIOUS);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 1, 1, 0, 0, 0, 0, 0, 0, 0);
}

int card_stabwhisker_the_odious(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, 1-player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int life_loss = 3;
		life_loss-=hand_count[1-player];
		if( life_loss > 0 ){
			lose_life(1-player, life_loss);
		}
	}

	return 0;
}

int card_night_dealings(int player, int card, event_t event){

	/* Whenever a source you control deals damage to another player, put that many theft counters on ~.
	 * |2|B|B, Remove X theft counters from ~: Search your library for a nonland card with converted mana cost X, reveal it, and put it into your hand. Then
	 * shuffle your library. */

	card_instance_t *instance = get_card_instance(player, card);

	damage_effects(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 2, 2, 0, 0, 0, 0 ) && count_counters(player, card, COUNTER_THEFT) > 0 ){
			return 1;
		}
	}
	else if(event == EVENT_ACTIVATE){
		charge_mana_for_activated_ability(player, card, 2, 2, 0, 0, 0, 0 );
		if( spell_fizzled != 1 ){
			int amount = count_counters(player, card, COUNTER_THEFT);
			if( player == HUMAN ){
				int number = choose_a_number(player, "Remove how many counters?", amount);
				if( number > 0 && number <= amount ){
					amount = number;
				}
				else{
					spell_fizzled = 1;
				}
			}
			instance->info_slot = amount;
			remove_counters(player, card, COUNTER_THEFT, amount);
		}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION){
		global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_LAND, 1, 0, 0, 0, 0, 0, 0, instance->info_slot, 0);
	}

	return global_enchantment(player, card, event);
}

int card_night_of_souls_betrayal(int player, int card, event_t event)
{
  /* Night of Souls' Betrayal	|2|B|B
   * Legendary Enchantment
   * All creatures get -1/-1. */

  check_legend_rule(player, card, event);
  boost_subtype(player, card, event, -1, -1,-1, 0,0, BCT_INCLUDE_SELF);
  return global_enchantment(player, card, event);
}

int card_no_dachi(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 3, 2, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_oathkeeper_takenos_daisho(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( is_equipping(player, card) ){
		int p = instance->targets[8].player;
		int c = instance->targets[8].card;

		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( affect_me(p, c) && has_subtype(p, c, SUBTYPE_SAMURAI) ){
				card_instance_t *affected = get_card_instance(p, c);
				if( affected->kill_code > 0 && affected->kill_code < 4){
					instance->targets[11].player = 66;
					instance->targets[11].card = get_id(p, c);
				}
			}
			else if( affect_me(player, card) ){
					kill_card(p, c, KILL_REMOVE);
			}
		}

		if( instance->targets[11].player == 66 && resolve_graveyard_trigger(player, card, event) == 1 ){
			const int *grave = get_grave(player);
			int count = count_graveyard(player)-1;
			while( count > -1 ){
					if( cards_data[grave[count]].id == instance->targets[11].card ){
						reanimate_permanent(player, card, player, count, REANIMATE_DEFAULT);
						break;
					}
					count--;
			}
			instance->targets[11].player = 0;
			instance->targets[11].card = -1;
		}
	}
	return vanilla_equipment(player, card, event, 2, 3, 1, 0, 0);
}

int card_okina_temple_to_the_grandfathers(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( (event == EVENT_COUNT_MANA && affect_me(player, card)) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_LEGEND;

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_G(2), 0, &td, "TARGET_CREATURE") ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Pump a Legend\n Cancel", 1);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			instance->number_of_targets = instance->info_slot = 0;
			instance->state |= STATE_TAPPED;
			if (charge_mana_for_activated_ability(player, card, MANACOST_G(1)) && pick_target(&td, "TARGET_LEGENDARY_CREATURE") ){
				instance->info_slot = 1;
				tap_card(player, card); // Now get the proper event sent
			}
			else{
				untap_card_no_event(player, card);
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot > 0 ){
			if( valid_target(&td) ){
				pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
			}
		}
		else{
			return mana_producer(player, card, event);
		}
	}

	return 0;
}

int card_oni_possession(int player, int card, event_t event ){
	card_instance_t *instance = get_card_instance( player, card );
	if( in_play(player, card) && instance->damage_target_player > -1 ){
		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}
	return generic_aura(player, card, event, player, 3, 3, KEYWORD_TRAMPLE, 0, SUBTYPE_DEMON, 0, 0);
}

int card_orochi_eggwatcher(int player, int card, event_t event){
	/* Orochi Eggwatcher	|2|G
	 * Creature - Snake Shaman 1/1
	 * |2|G, |T: Put a 1/1 |Sgreen Snake creature token onto the battlefield. If you control ten or more creatures, flip ~.
	 * --FLIP--
	 * Shidako, Broodmistress
	 * Legendary Creature - Snake Shaman
	 * |G, Sacrifice a creature: Target creature gets +3/+3 until end of turn.
	 * 3/3 */

	card_instance_t *instance= get_card_instance(player, card);

	double_faced_card(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_SNAKE);
		int i;
		int amount = 0;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) ){
				amount++;
			}
		}
		if( amount > 9 ){
			true_transform(player, instance->parent_card);
			verify_legend_rule(player, instance->parent_card, get_id(player, instance->parent_card));
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 2, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_petals_of_insight(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		int max = 3;
		int kill_me = 1;
		if( max > count_deck(player) ){
			max = count_deck(player);
		}
		if( max > 0 ){
			if( player == AI && max < 3 ){
				kill_me = 0;
			}
			else{
				int *deck = deck_ptr[player];
				if( show_deck(player, deck, max, "Click on a card to draw 3 cards", 0, 0x7375B0 ) != -1 ){
					draw_cards(player, 3);
				}
				else{
					kill_me = 0;
				}
			}
		}
		if( kill_me ==1 ){
			kill_card(player, card, KILL_DESTROY);
		}
		else{
			if( max > 1 ){
				put_top_x_on_bottom(player, player, max);
			}
			bounce_permanent(player, card);
		}
	}

	return 0;
}

int card_shidako_broodmistress(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 3, 3);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE, MANACOST_G(1), 0, &td, "TARGET_CREATURE");
}

int card_orochi_hatchery(int player, int card, event_t event){

	/* Orochi Hatchery	|X|X
	 * Artifact
	 * ~ enters the battlefield with X charge counters on it.
	 * |5, |T: Put a 1/1 |Sgreen Snake creature token onto the battlefield for each charge counter on ~. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = charge_mana_for_double_x(player, COLOR_ARTIFACT);
		instance->info_slot = result/2;
	}

	enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, instance->info_slot);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_tokens_by_id(player, card, CARD_ID_SNAKE, count_counters(instance->parent_controller, instance->parent_card, COUNTER_CHARGE));
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 5, 0, 0, 0, 0, 0, 0, 0,0);
}

int card_orochi_leafcaller(int player, int card, event_t event){

 //   card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_mana(player, COLOR_GREEN, 1) ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE ){
		charge_mana(player, COLOR_GREEN, 1);
		if( spell_fizzled != 1 ){
			return mana_producer(player, card, event);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_orochi_ranger(int player, int card, event_t event){
	return freeze_when_damage(player, card, event);
}

// orochi sustainer --> llanowar elves

int card_otherworldly_journey(int player, int card, event_t event){
	// the +1/+1 effect is embedded in "remove_until_eot" function

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

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
			if( player == AI ){
				return card_death_ward(player, card, event);
			}
			else{
				pick_target(&td, "TARGET_CREATURE");
			}
	}
	else if( event == EVENT_RESOLVE_SPELL){
			if( valid_target(&td) ){
				remove_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY );
  }
  return 0;
}

int card_pain_kami(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) && can_target(&td) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0);
			if( spell_fizzled != 1 ){
				charge_mana(player, COLOR_COLORLESS, -1);
				if( spell_fizzled != 1 && x_value > 0 ){
					instance->info_slot = x_value;
					if( pick_target(&td, "TARGET_CREATURE") ){
						kill_card(player, card, KILL_SACRIFICE);
					}
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot, player, card);
	}
	return 0;
}

int card_painwracker_oni(int player, int card, event_t event){

	fear(player, card, event);

	if( current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		int trig = 1-check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_OGRE);
		if( trig == 1  ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}
	return 0;
}

ARCANE(card_part_the_veil)
{
  /* Part the Veil	|3|U
   * Instant - Arcane
   * Return all creatures you control to their owner's hand. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	manipulate_type(player, card, player, TYPE_CREATURE, ACT_BOUNCE);

  return 0;
}

ARCANE(card_peer_through_depths){
	/* Look at the top five cards of your library. You may reveal an instant or sorcery card from among them and put it into your hand. Put the rest on the
	 * bottom of your library in any order. */
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		reveal_x_and_choose_a_card_type(player, card, 5, TYPE_SORCERY | TYPE_INSTANT);
	}
	return 0;
}

int card_pious_kitsune(int player, int card, event_t event){

	/* At the beginning of your upkeep, put a devotion counter on ~. Then if a creature named Eight-and-a-Half-Tails is on the battlefield, you gain 1 life for
	 * each devotion counter on ~.
	 * |T, Remove a devotion counter from ~: You gain 1 life. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_DEVOTION);
		if( check_battlefield_for_id(ANYBODY, CARD_ID_EIGHT_AND_A_HALF_TAILS) ){
			gain_life(player, count_counters(player, card, COUNTER_DEVOTION));
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, GVC_COUNTER(COUNTER_DEVOTION), NULL, NULL);
}

ARCANE_WITH_SPLICE(card_psychic_puppetry, MANACOST_U(1))
{
  // You may tap or untap target permanent.
  // Splice onto Arcane |U
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT );

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card) && pick_target(&td, "TARGET_PERMANENT"))
	ai_modifier_twiddle(player, card, 0);

  if (event == EVENT_RESOLVE_SPELL && valid_target(&td))
	twiddle(player, card, 0);

  return 0;
}

ARCANE(card_quiet_purity){
	// Destroy target enchantment.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ENCHANTMENT");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
	}
	return 0;
}

ARCANE(card_reach_through_mists){
	// Draw a card.
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 1);
	}
	return 0;
}

ARCANE(card_rend_flesh){
	// Destroy target non-Spirit creature.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_SPIRIT;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
	}
	return 0;
}

int card_ronin_houndmaster(int player, int card, event_t event){
	haste(player, card, event);
	return bushido(player, card, event, 1);
}

int card_rootrunner(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE  && can_use_activated_abilities(player, card) && can_target(&td) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 2, 0, 0)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 0, 0, 2, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_LAND") ){
				kill_card(player, card, KILL_SACRIFICE);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
	}
	return soulshift(player, card, event, 3, 1);
}

int card_ryusei(int player, int card, event_t event){
	check_legend_rule(player, card, event);

	if( graveyard_from_play(player, card, event) ){
		damage_all(player, card, player, 5, 0, 0, KEYWORD_FLYING, 1, 0, 0, 0, 0, 0, 0, -1, 0);
		damage_all(player, card, 1-player, 5, 0, 0, KEYWORD_FLYING, 1, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	return 0;
}

int card_sachi_daughter_of_seshiro(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	int result = permanents_you_control_can_tap_for_mana(player, card, event, TYPE_PERMANENT, SUBTYPE_SHAMAN, COLOR_GREEN, 2);
	if (event == EVENT_CAN_ACTIVATE){
		return result;
	}

	return boost_creature_type(player, card, event, SUBTYPE_SNAKE, 0, 1, 0, BCT_CONTROLLER_ONLY);
}

int card_sakura_tribe_elder(int player, int card, event_t event){
	/*
	  Sakura-Tribe Elder |1|G
	  Creature - Snake Shaman 1/1
	  Sacrifice Sakura-Tribe Elder: Search your library for a basic land card, put that card onto the battlefield tapped, then shuffle your library.
	*/
	if( event == EVENT_RESOLVE_ACTIVATION ){
		tutor_basic_land(player, 1, 1);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}

int card_samurai_enforcers(int player, int card, event_t event){
	return bushido(player, card, event, 2);
}

int card_samurai_of_the_pale_courtain(int player, int card, event_t event)
{
  /* Samurai of the Pale Curtain	|W|W
   * Creature - Fox Samurai 2/2
   * Bushido 1
   * If a permanent would be put into a graveyard, exile it instead. */

  if (event == EVENT_GRAVEYARD_FROM_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "");
	  if_a_card_would_be_put_into_graveyard_from_play_exile_it_instead(player, card, event, ANYBODY, &test);
	}

	return bushido(player, card, event, 1);
}

int card_seizan_perverter_of_truth(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(current_turn, 2);
		draw_cards(current_turn, 2);
	}

	return 0;
}

static int training_counter(int player, int card, event_t event){
	card_instance_t *instance= get_card_instance(player, card);
	return bushido(instance->targets[0].player, instance->targets[0].card, event, 0);
}

int card_sensei_golden_tail(int player, int card, event_t event){

	/* Sensei Golden-Tail	|1|W
	 * Legendary Creature - Fox Samurai 2/1
	 * Bushido 1
	 * |1|W, |T: Put a training counter on target creature. That creature gains bushido 1 and becomes a Samurai in addition to its other creature
	 * types. Activate this ability only any time you could cast a sorcery. */

	check_legend_rule(player, card, event);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance= get_card_instance(player, card);

	int can_activate = can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_activate && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1) ){
		return can_target(&td);
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				tap_card(player, card);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_TRAINING);	// does nothing, per rulings
				add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_SAMURAI);
				card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				target->targets[8].player++;
				if( ! check_for_special_ability(instance->targets[0].player, instance->targets[0].card, SP_KEYWORD_BUSHIDO) ){
					create_targetted_legacy_effect(player, instance->parent_card, &training_counter, instance->targets[0].player, instance->targets[0].card);
				}
			}
	}

	return bushido(player, card, event, 1);
}

int card_senseis_divining_top(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		int good = 0;
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			good = 1;
		}
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
			good = 1;
		}
		return good;
	}
	else if(event == EVENT_ACTIVATE){
			int choice = 0;
			if( has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
				if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					char buffer[52];
					snprintf(buffer, 52, " Rearrange top 3 cards.\n Draw a card and flip Top\n Cancel" );
					choice = do_dialog(player, player, card, -1, -1, buffer, choice);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0){
				if( charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
					instance->info_slot = choice+1;
				}
			}
			else if( choice == 1 ){
					if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
						tap_card( player, card);
						instance->info_slot = choice+1;
					}
			}
			else{
				spell_fizzled = 1;
			}

	}
	else if(event == EVENT_RESOLVE_ACTIVATION && spell_fizzled != 1){
		if ( instance->info_slot == 1){
			rearrange_top_x(player, player, 3);
		}
		if ( instance->info_slot == 2){
			draw_a_card(player);
			if( in_play(instance->parent_controller, instance->parent_card) ){
				put_on_top_of_deck(instance->parent_controller, instance->parent_card);
			}
		}
	}
	return 0;
}

int card_seshiro_the_anointed(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( subtype_deals_damage(player, card, event, player, SUBTYPE_SNAKE, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_PLAYER) ){
		draw_some_cards_if_you_want(player, card, player, instance->targets[1].card);
		instance->targets[1].card = 0;
	}

	return boost_creature_type(player, card, event, SUBTYPE_SNAKE, 2, 2, 0, BCT_CONTROLLER_ONLY);
}

int card_shimatsu_the_bloodcloaked(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  // As ~ enters the battlefield, sacrifice any number of permanents. Shimatsu enters the battlefield with that many +1/+1 counters on it.
  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "");
	  test.not_me = 1;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->state |= STATE_OUBLIETTED;	// so it isn't destroyed after each sacrifice

	  int n = 0;
	  while (new_sacrifice(player, card, player, 0, &test))
		++n;

	  instance->state &= ~STATE_OUBLIETTED;
	  add_1_1_counters(player, card, n);
	}

  return 0;
}

int card_shinka_the_bloodsoaked_keep(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_LEGEND;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_R(2)) && can_use_activated_abilities(player, card) && can_target(&td) ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Give First Strike to a Legend\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			instance->number_of_targets = 0;
			instance->state |= STATE_TAPPED;
			if (charge_mana_for_activated_ability(player, card, MANACOST_R(1)) && pick_target(&td, "TARGET_LEGENDARY_CREATURE") ){
					instance->info_slot = 1;
			} else {
				untap_card_no_event(player, card);
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				if(valid_target(&td) ){
					pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
											0, 0, KEYWORD_FIRST_STRIKE, 0);
				}
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
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

int card_shisato_whispering_hunter(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		impose_sacrifice(player, card, player, 1, TYPE_PERMANENT, 0, SUBTYPE_SNAKE, 0, 0, 0, 0, 0, -1, 0);
	}

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		target_player_skips_next_untap(player, card, 1-player);
	}

	return 0;
}

int card_shizo_deaths_storehouse(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_LEGEND;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, MANACOST_B(2))  && can_use_activated_abilities(player, card) && can_target(&td) ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Give Fear to a Legend\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			instance->number_of_targets = 0;
			instance->state |= STATE_TAPPED;
			if (charge_mana_for_activated_ability(player, card, MANACOST_B(1)) && pick_target(&td, "TARGET_LEGENDARY_CREATURE") ){
				instance->info_slot = 1;
			} else {
				untap_card_no_event(player, card);
			}
		}

		if (choice == 2){
			cancel = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				if(valid_target(&td) ){
					pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
											0, 0, 0, SP_KEYWORD_FEAR);
				}
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
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

int card_sift_through_sands(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		discard(player, 0, player);
		if( get_trap_condition(player, TRAP_PEER_THROUGH_DEPTHS) && get_trap_condition(player, TRAP_REACH_THROUGH_MISTS) ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, 4, 0, 0, 0, 0, 0, 0, CARD_ID_THE_UNSPEAKABLE, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_silent_chant_zubera(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		gain_life(player, get_zubera_count()*2);
	}

	return 0;
}

int card_soratami_cloudskater(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance= get_card_instance(player, card);


	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) ){
		return can_target(&td);
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_LAND") ){
				instance->number_of_targets = 1;
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			draw_cards(player, 1);
			discard(player, 0, player);
	}

	return 0;
}

int card_soratami_savant(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) && can_target(&td) ){
		int result = card_spiketail_hatchling(player, card, event);
		if( result > 0 ){
			instance->targets[1].player = card_on_stack_controller;
			instance->targets[1].card = card_on_stack;
			return result;
		}
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_LAND") ){
				instance->number_of_targets = 1;
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			counterspell_resolve_unless_pay_x(player, card, NULL, 1, 3);
	}

	return 0;
}

int card_soratami_seer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance= get_card_instance(player, card);


	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) && target_available(player, card, &td) > 1 ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_LAND") ){
				instance->number_of_targets = 1;
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				td.allow_cancel = 0;
				pick_target(&td, "TARGET_LAND");
				instance->number_of_targets = 1;
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int amount = hand_count[player];
			discard_all(player);
			draw_cards(player, amount);
	}

	return 0;
}

int card_sosuke_son_of_seshiro(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( subtype_deals_damage(player, card, event, player, SUBTYPE_WARRIOR, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_CREATURE+DDBM_TRACE_DAMAGED_CREATURES) ){
		int i;
		for(i=0; i<instance->targets[1].card; i++){
			if( in_play(instance->targets[i+2].player, instance->targets[i+2].card) ){
				create_targetted_legacy_effect(player, card, &die_at_end_of_combat, instance->targets[i+2].player, instance->targets[i+2].card);
			}
		}
		instance->targets[1].card = 0;
	}

	return boost_creature_type(player, card, event, SUBTYPE_SNAKE, 1, 0, 0, BCT_CONTROLLER_ONLY);
}

int card_soulblast(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
						instance->info_slot+=get_power(player, count);
						kill_card(player, count, KILL_SACRIFICE);
					}
					count--;
			}

		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, instance->info_slot);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

ARCANE_WITH_SPLICE(card_soulless_revival, MANACOST_XB(1,1)){
	// Return target creature card from your graveyard to your hand.
	// Splice onto Arcane |1|B
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return any_in_graveyard_by_type(player, TYPE_CREATURE) && !graveyard_has_shroud(player);
	}
	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int selected = select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_CMC, -1, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0);
			if( selected != -1 ){
				const int *grave = get_grave(player);
				instance->targets[0].player = selected;
				instance->targets[0].card = grave[selected];
			}
			else{
				spell_fizzled = 1;
			}
	}
	if(event == EVENT_RESOLVE_SPELL){
			int selected = instance->targets[0].player;
			const int *grave = get_grave(player);

			if( instance->targets[0].card == grave[selected] ){
				add_card_to_hand(player, grave[selected]);
				remove_card_from_grave(player, selected);
			}
	}

	return 0;
}

ARCANE_WITH_SPLICE(card_strange_inversion, MANACOST_XR(1,1))
{
  // Switch target creature's power and toughness until end of turn.
  // Splice onto Arcane |1|R
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = ANYBODY;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	pick_target(&td, "TARGET_CREATURE");

  if (event == EVENT_RESOLVE_SPELL && valid_target(&td))
	switch_power_and_toughness_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);

  return 0;
}

// not in card_id ??
int card_swallowing_plague(int player, int card, event_t event){

	return generic_x_spell(player, card, event, TARGET_ZONE_IN_PLAY, COLOR_COLORLESS, 9);
}

int card_takeno_samurai_general(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( affected_card_controller == player && affected_card != card && has_subtype(affected_card_controller, affected_card, SUBTYPE_SAMURAI) &&
		check_for_special_ability(affected_card_controller, affected_card, SP_KEYWORD_BUSHIDO)
	  ){
		card_instance_t *instance = get_card_instance(affected_card_controller, affected_card);
		modify_pt_and_abilities(affected_card_controller, affected_card, event,instance->targets[8].player, instance->targets[8].player, 0);
	}

	return bushido(player, card, event, 2);
}

int card_tatsumasa_the_dragon_fang(int player, int card, event_t event){

	/* Tatsumasa, the Dragon's Fang	|6
	 * Legendary Artifact - Equipment
	 * Equipped creature gets +5/+5.
	 * |6, Exile ~: Put a 5/5 |Sblue Dragon Spirit creature token with flying onto the battlefield. Return ~ to the battlefield under its owner's control when
	 * that token dies.
	 * Equip |3 */

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if (is_equipping(player, card)){
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, 5, 5, 0);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 6, 0, 0, 0, 0, 0)  ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 3);
	}
	else if( event == EVENT_ACTIVATE ){
			int new_cost = get_updated_equip_cost(player, card, 3);
			int choice = 0;
			if( has_mana(player, COLOR_COLORLESS, new_cost) ){
				if( has_mana_for_activated_ability(player, card, 6, 0, 0, 0, 0, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Equip\n Summon the Dragon Spirit\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}
			instance->info_slot = choice;
			if( choice == 2  ){
				spell_fizzled = 1;
			}
			else if( choice == 1 ){
				if (charge_mana_for_activated_ability(player, card, MANACOST_X(6))){
					kill_card(player, card, KILL_REMOVE);
				}
			}
			else{
				activate_basic_equipment(player, card, 3);
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 1 ){
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_DRAGON_SPIRIT, &token);
				token.pow = token.tou = 5;
				token.key_plus = KEYWORD_FLYING;
				token.color_forced = COLOR_TEST_BLUE;
				generate_token(&token);
			}
			else{
				resolve_activation_basic_equipment(player, card);
			}
	}
	return 0;
}

int card_tenza_godos_maul(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( is_equipping(player, card) ){
		if( is_legendary(instance->targets[8].player, instance->targets[8].card)){
			modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event,2, 2, 0);
		}
		if( get_color(instance->targets[8].player, instance->targets[8].card) & COLOR_TEST_RED ){
			modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event,0, 0, KEYWORD_TRAMPLE);
		}
	}
	return vanilla_equipment(player, card, event, 1, 1, 1, 0, 0);
}

int card_the_unspeakable(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){
		global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, 1, TYPE_ANY, 0, SUBTYPE_ARCANE, 0, 0, 0, 0, 0, -1, 0);

	}
	return 0;
}

int card_thief_of_hope(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( arcane_spirit_spell_trigger(player, card, event, 2) ){
		instance->targets[0].player = 1-player;
		if( valid_target(&td)  ){
			lose_life(instance->targets[0].player, 1);
			gain_life(player, 1);
		}
	}

	return soulshift(player, card, event, 2, 1);
}

ARCANE_WITH_SPLICE(card_through_the_breach, MANACOST_XR(2,2)){
	/* You may put a creature card from your hand onto the battlefield. That creature gains haste. Sacrifice that creature at the beginning of the next end
	 * step. */
	// Splice onto Arcane |2|R|R
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int result = global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		if( result > -1 ){
			create_targetted_legacy_effect(player, card, &haste_and_sacrifice_eot, player, result);
		}
	}
	return 0;
}

int card_time_of_need(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_CREATURE, 0, SUBTYPE_LEGEND, 0, 0, 0, 0, 0, -1, 0);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_uyo_silent_prophet(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.preferred_controller = player;
	td.allowed_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance= get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, MANACOST_X(2)) && target_available(player, card, &td) >= 2 ){
		return activate_twincast(player, card, event, NULL, NULL);
	}
	else if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		activate_twincast(player, card, event, NULL, NULL);
		if (spell_fizzled != 1
			&& charge_mana_for_activated_ability(player, card, MANACOST_X(2))
			&& new_pick_target(&td, "TARGET_LAND", 1, 1)){
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
			new_pick_target(&td, "TARGET_LAND", 2, 1);
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
			if (spell_fizzled != 1){
				bounce_permanent(instance->targets[1].player, instance->targets[1].card);
				bounce_permanent(instance->targets[2].player, instance->targets[2].card);
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
		activate_twincast(player, card, event, NULL, NULL);
	}

	return 0;
}

int card_villainous_ogre(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	cannot_block(player, card, event);

	if( land_can_be_played & LCBP_REGENERATION ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  &&
			has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && instance->kill_code == KILL_DESTROY &&
			!( instance->token_status & STATUS_CANNOT_REGENERATE ) && check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DEMON)
		  ){
			return 0x63;
		}
		else if( event == EVENT_ACTIVATE ){
				charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) ;
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				 regenerate_target(player, instance->parent_card);
		}
	}
	return 0;
}

ARCANE(card_waking_nightmare){
	// Target player discards two cards.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				new_multidiscard(instance->targets[0].player, 2, 0, player);
			}
	}
	return 0;
}

ARCANE_WITH_SPLICE(card_wear_away, MANACOST_XG(3,1)){
	// Destroy target artifact or enchantment.
	// Splice onto Arcane |3|G
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "DISENCHANT");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance(player, card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
	}
	return 0;
}

int card_wicked_akuba(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance= get_card_instance(player, card);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT) ){
		instance->targets[7].player = 1;
	}

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && instance->targets[7].player == 1 ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		return would_valid_target(&td);
	}
	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				lose_life(instance->targets[0].player, 1);
			}
	}
	if( event == EVENT_CLEANUP  ){
		instance->targets[7].player = 0;
	}

	return 0;
}

int card_yamabushis_flame(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td)  ){
				if( instance->targets[0].card > -1 ){
					create_targetted_legacy_effect(player, card, &rfg_when_die, instance->targets[0].player, instance->targets[0].card);
				}
				damage_creature_or_player(player, card, event, 3);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_yamabushis_storm(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
						create_targetted_legacy_effect(player, card, &rfg_when_die, i, count);
						damage_creature(i, count, 1, player, card);
					}
					count--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_yosei(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
			target_player_skips_next_untap(player, card, instance->targets[0].player);

			instance->targets[2] = instance->targets[0];

			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_PERMANENT);
			td1.allowed_controller = instance->targets[2].player;
			td1.preferred_controller = instance->targets[2].player;
			td1.illegal_state = TARGET_STATE_TAPPED;

			int count = 0;
			while( count < 5 && can_target(&td1) ){
					if( pick_target(&td1, "TARGET_PERMANENT") ){
						instance->number_of_targets = 1;
						tap_card( instance->targets[0].player, instance->targets[0].card);
						count++;
					}
					else{
						break;
					}
			}
		}
	}

	return 0;
}

int card_zozu_the_punisher(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( specific_cip(player, card, event, 2, 2, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		damage_player(instance->targets[1].player, 2, player, card);
	}

	return 0;
}
