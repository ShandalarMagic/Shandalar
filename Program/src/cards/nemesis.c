#include "manalink.h"

// Functions
static int subtype_searcher(int player, int card, event_t event, int subtype){
	return generic_subtype_searcher(player, card, event, subtype, 3, 2);
}

static int mercenary_searcher(int player, int card, event_t event, int cless){
	return generic_subtype_searcher(player, card, event, SUBTYPE_MERCENARY, cless, 1);
}

int seal(int player, int card, event_t event, target_definition_t *td, const char *prompt ){

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, td, prompt);
	}

	if(event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 0, 0, 0, 0, 0, 0, 0, td, prompt);
	}

	return global_enchantment(player, card, event);
}

static int parallax(int player, int card, event_t event){

	fading(player, card, event, 5);

	return global_enchantment(player, card, event);
}

int nemesis_freespell(int player, int card, int event){
	if( ! played_for_free(player, card) && ! is_token(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->info_slot == 1 && player != AI && has_mana_to_cast_id(player, event, get_id(player, card)) ){
			int choice = do_dialog(player, player, card, -1, -1, " Play for free\n Play normally\n Cancel", 0);
			if( choice == 1 ){
				if( ! charge_mana_from_id(player, card, event, get_id(player, card)) ){
					return 0;
				}
			}
			else if( choice == 2 ){
					return 0;
			}
		}
	}
	return 1;
}


// Cards

int card_accumulated_knowledge(int player, int card, event_t event){

  if( event == EVENT_CAN_CAST ){
	 return 1;
  }
  else if( event == EVENT_RESOLVE_SPELL ){
		   int id = CARD_ID_ACCUMULATED_KNOWLEDGE;
		   int number = count_graveyard_by_id(player, id) + count_graveyard_by_id(1-player, id);
	 number++;
	 draw_cards(player, number);
	 kill_card(player, card, KILL_DESTROY);
  }

  return 0;
}

int card_aether_barrier(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, 2, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int sac = 1;
		if( has_mana(instance->targets[1].player, COLOR_COLORLESS, 1) ){
			charge_mana(instance->targets[1].player, COLOR_COLORLESS, 1);
			if( spell_fizzled != 1 ){
				sac = 0;
			}
		}
		if( sac == 1 ){
			impose_sacrifice(player, card, instance->targets[1].player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return global_enchantment(player, card, event);
}

int card_ancient_hydra(int player, int card, event_t event){

	/* Ancient Hydra	|4|R
	 * Creature - Hydra 5/1
	 * Fading 5
	 * |1, Remove a fade counter from ~: ~ deals 1 damage to target creature or player. */

	fading(player, card, event, 5);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(1), GVC_COUNTER(COUNTER_FADE), &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_arc_mage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 1, 0) && hand_count[player] > 0
	  ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 1, 0);
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			if( new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", 1, 1) ){
				tap_card(player, card);
				instance->number_of_targets = 2;
				discard(player, 0, player);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		divide_damage(player, card, &td);
	}

	return 0;
}

int card_ascendant_evincar(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	evincars(player, card, event, COLOR_TEST_BLACK);

	return 0;
}

int card_avenger_en_dal(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = get_toughness(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			gain_life(instance->targets[0].player, amount);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD+GAA_CAN_TARGET,
													2, 0, 0, 0, 0, 1, 0, &td, "TARGET_CREATURE");
}

int card_belbes_armor(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = instance->info_slot;
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -amount, amount);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, -1, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_belbes_portal(int player, int card, event_t event){

	/* Belbe's Portal	|5
	 * Artifact
	 * As ~ enters the battlefield, choose a creature type.
	 * |3, |T: You may put a creature card of the chosen type from your hand onto the battlefield. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[0].card = select_a_subtype(player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if (instance->targets[0].card <= 0){	// no creature type was chosen
			return 0;
		}

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, get_subtype_text("Select %a creature card.", instance->targets[0].card));
		this_test.subtype = instance->targets[0].card;
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 3, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_blastoderm(int player, int card, event_t event){
	/* Blastoderm	|2|G|G
	 * Creature - Beast 5/5
	 * Shroud
	 * Fading 3 */
	fading(player, card, event, 3);
	return 0;
}

int card_blinding_angel(int player, int card, event_t event)
{
  // Whenever ~ deals combat damage to a player, that player skips his or her next combat phase.
  if (damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT|DDBM_MUST_BE_COMBAT_DAMAGE))
	target_player_skips_his_next_attack_step(player, card, 1-player, 0);

  return 0;
}

int card_bola_warrior(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD+GAA_CAN_TARGET,
													0, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE");
}

int card_chieftain_en_dal(int player, int card, event_t event)
{
  // Whenever ~ attacks, attacking creatures gain first strike until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  pump_creatures_until_eot(player, card, current_turn, 0, 0,0, KEYWORD_FIRST_STRIKE,0, &test);
	}

	return 0;
}

// cloudskate --> blastoderm

int card_complex_automaton(int player, int card, event_t event){

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		if( count_subtype(player, TYPE_PERMANENT, -1) > 6 ){
			bounce_permanent(player, card);
		}
	}
	return 0;
}

int card_dark_triumph(int player, int card, event_t event){
	/*
	  Dark Triumph |4|B
	  Instant
	  If you control a Swamp, you may sacrifice a creature rather than pay Dark Triumph's mana cost.
	  Creatures you control get +2/+0 until end of turn.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_SWAMP) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			instance->info_slot = 1;
			null_casting_cost(player, card);
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
				choice = do_dialog(player, player, card, -1, -1, " Sac a creature\n Play normally\n Cancel", 0);
			}

			if( choice == 0 ){
				if( ! sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					spell_fizzled = 1;
				}
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 2, 0, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_daze(int player, int card, event_t event){
	/*
	  Daze |1|U
	  Instant
	  You may return an Island you control to its owner's hand rather than pay Daze's mana cost.
	  Counter target spell unless its controller pays {1}.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.required_subtype = SUBTYPE_ISLAND;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		if( target_available(player, card, &td) > 0 ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
				choice = do_dialog(player, player, card, -1, -1, " Bounce an Island\n Play normally\n Cancel", 0);
			}
			if( choice == 0 ){
				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_LAND);
				td.allowed_controller = player;
				td.preferred_controller = player;
				td.required_subtype = SUBTYPE_ISLAND;
				td.illegal_abilities = 0;

				if( new_pick_target(&td, "Select an Island to return to your hand.", 0, 1 | GS_LITERAL_PROMPT) ){
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);;
				}
				instance->number_of_targets = 0;
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
			}
		}
		if( spell_fizzled != 1 ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		return card_force_spike(player, card, event);
	}

	return 0;
}

int card_death_pit_offering(int player, int card, event_t event){
	/*
	  Death Pit Offering |2|B|B
	  Enchantment
	  When Death Pit Offering enters the battlefield, sacrifice all creatures you control.
	  Creatures you control get +2/+2.
	*/
	if( comes_into_play(player, card, event) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &test, KILL_SACRIFICE);
	}

	if( ! is_humiliated(player, card) ){
		boost_creature_type(player, card, event, -1, 2, 2, 0, BCT_CONTROLLER_ONLY);
	}

	return global_enchantment(player, card, event);
}

int card_defender_en_vec(int player, int card, event_t event){

	/* Defender en-Vec	|3|W
	 * Creature - Human Cleric 2/4
	 * Fading 4
	 * Remove a fade counter from ~: Prevent the next 2 damage that would be dealt to target creature or player this turn. */

	fading(player, card, event, 4);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.extra = damage_card;
	td1.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		if( target->info_slot > 2 ){
			target->info_slot -= 2;
		}
		else{
			target->info_slot = 0;
		}
	}
	return generic_activated_ability(player, card, event, GAA_DAMAGE_PREVENTION | GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_FADE), &td1, "TARGET_DAMAGE");
}

int card_defiant_falcon(int player, int card, event_t event)
{
  /* Amrou Scout	|1|W
   * Creature - Kithkin Rebel Scout 2/1
   * |4, |T: Search your library for a Rebel permanent card with converted mana cost 3 or less and put it onto the battlefield. Then shuffle your library. */

  /* Defiant Falcon	|1|W
   * Creature - Rebel Bird 1/1
   * Flying
   * |4, |T: Search your library for a Rebel permanent card with converted mana cost 3 or less and put it onto the battlefield. Then shuffle your library. */

  return generic_subtype_searcher(player, card, event, SUBTYPE_REBEL, 4, 0);
}

int card_defiant_vanguard(int player, int card, event_t event){

	/* Defiant Vanguard	|2|W
	 * Creature - Human Rebel 2/2
	 * When ~ blocks, at end of combat, destroy it and all creatures it blocked this turn.
	 * |5, |T: Search your library for a Rebel permanent card with converted mana cost 4 or less and put it onto the battlefield. Then shuffle your library. */

	if( current_turn != player && event == EVENT_DECLARE_BLOCKERS ){
		card_instance_t* instance = get_card_instance(player, card);
		if( instance->blocking < 255 ){
			create_targetted_legacy_effect(player, card, &die_at_end_of_combat, 1-player, instance->blocking);
			create_targetted_legacy_effect(player, card, &die_at_end_of_combat, player, card);
		}
	}

	return generic_subtype_searcher(player, card, event, SUBTYPE_REBEL, 5, 0);
}

int card_dominate(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return !check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG) && can_target(&td);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;

		charge_mana(player, COLOR_COLORLESS, -1);
		td.extra = instance->info_slot = x_value;
		td.special = TARGET_SPECIAL_CMC_LESSER_OR_EQUAL;

		if (!can_target(&td)){
			spell_fizzled = 1;
			ai_modifier -= 128;
		} else {
			char buf[128];
			sprintf(buf, "Select target creature with converted mana cost %d or less.", x_value);
			pick_next_target_noload(&td, buf);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		td.extra = instance->info_slot;
		td.special = TARGET_SPECIAL_CMC_LESSER_OR_EQUAL;
		if( valid_target(&td) ){
			gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ensnare(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, 2, &this_test, ACT_TAP);
		kill_card(player, card, KILL_DESTROY);
	}
	else{
		return card_gush(player, card, event);
	}

	return 0;
}

int card_fanatical_devotion(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, GAA_REGENERATION | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
		if( result && can_sacrifice_type_as_cost(player, 1, TYPE_CREATURE) && count_subtype(player, TYPE_CREATURE, -1) > 1){
			return result;
		}
	}

	if( event == EVENT_ACTIVATE ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (!sac){
			cancel = 1;
			return 0;
		}
		if( new_pick_target(&td, "Select target creature to regenerate.", 0, 1 | GS_LITERAL_PROMPT) ){
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
		else{
			state_untargettable(BYTE2(sac), BYTE3(sac), 1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target( instance->targets[0].player, instance->targets[0].card );
		}
	}

	return 0;
}

int card_flame_rift(int player, int card, event_t event){
	/*
	  Flame Rift |1|R
	  Sorcery
	  Flame Rift deals 4 damage to each player.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		damage_player(player, 4, player, card);
		damage_player(1-player, 4, player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_flowstone_overseer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, -1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 0, 0, 0, 0, 2, 0, 0, &td, "TARGET_CREATURE");
}

int card_flowstone_slide(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		charge_mana(player, COLOR_COLORLESS, -1);
		if( spell_fizzled != 1 ){
			instance->info_slot = x_value;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = instance->info_slot;
		pump_subtype_until_eot(player, card, 2, -1, amount, -amount, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_flowstone_surge(int player, int card, event_t event){
	boost_creature_type(player, card, event, -1, 1, -1, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);
	return global_enchantment(player, card, event);
}

int card_jolting_merfolk(int player, int card, event_t event){

	/* Jolting Merfolk	|2|U|U
	 * Creature - Merfolk 2/2
	 * Fading 4
	 * Remove a fade counter from ~: Tap target creature. */

	fading(player, card, event, 4);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_FADE), &td, "TARGET_CREATURE");
}

int card_kill_switch(int player, int card, event_t event){

	/* Kill Switch	|3
	 * Artifact
	 * |2, |T: Tap all other artifacts. They don't untap during their controllers' untap steps for as long as ~ remains tapped. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		int pp = instance->parent_controller, pc = instance->parent_card;
		int i;
		for(i = 0; i < 2; i++){
			int count = active_cards_count[player]-1;
			while( count > -1){
					if( in_play(i, count) && is_what(i, count, TYPE_ARTIFACT) && !(i == pp && count == pc) ){
						tap_card(i, count);
						does_not_untap_until_im_tapped(pp, pc, i, count);
					}
					count--;
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_kor_haven(int player, int card, event_t event){
	/*
	  Kor Haven English
	  Legendary Land
	  {T}: Add {1} to your mana pool.
	  {1}{W}, {T}: Prevent all combat damage that would be dealt by target attacking creature this turn.
	*/
	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XW(2, 1), 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Prevent Damage\n Cancel", 1);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				if( charge_mana_for_activated_ability(player, card, MANACOST_XW(1, 1)) &&
					new_pick_target(&td, "DESERT", 0, 1)
				  ){
					instance->info_slot = 1;
				}
				else{
					remove_state(player, card, STATE_TAPPED);
				}
		}
		else if(choice == 2){
				spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( valid_target(&td) ){
				negate_combat_damage_this_turn(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0);
			}
		}
	}

	return 0;
}

int card_laccolith_grunt(int player, int card, event_t event){
	// also code for Laccolith Titan, Laccolith Warrior and Laccolith Whelp

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_CAN_ACTIVATE ){
		if( current_phase == PHASE_AFTER_BLOCKING && is_attacking(player, card) && ! is_unblocked(player, card) && instance->targets[1].player != 66 ){
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
		}
	}

	if( event == EVENT_ACTIVATE ){
		generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
		if( spell_fizzled != 1 ){
			instance->targets[1].player = 66;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, get_power(player, instance->parent_card), player, instance->parent_card);
		}
		negate_combat_damage_this_turn(player, instance->parent_card, player, instance->parent_card, 0);
	}

	return 0;
}

int card_lin_sivvi_defiant_hero(int player, int card, event_t event){

	/* Lin Sivvi, Defiant Hero	|1|W|W
	 * Legendary Creature - Human Rebel 1/3
	 * |X, |T: Search your library for a Rebel permanent card with converted mana cost X or less and put it onto the battlefield. Then shuffle your library.
	 * |3: Put target Rebel card from your graveyard on the bottom of your library. */

	test_definition_t this_test1;
	new_default_test_definition(&this_test1, 0, "Select target Rebel card.");
	this_test1.subtype = SUBTYPE_REBEL;

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[1].card = 0;
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_sick(player, card) && ! is_tapped(player, card) ){
			if( !(player == AI && instance->targets[1].card == -1) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				return 1;
			}
		}
		if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) && new_special_count_grave(player, &this_test1) > 0 &&
			! graveyard_has_shroud(2)
		  ){
			return 1;
		}
	}

	else if( event == EVENT_ACTIVATE ){
			int choice = 0;
			if( ! is_sick(player, card) && ! is_tapped(player, card) ){
				if( !(player == AI && instance->targets[1].card == -1) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
					if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) && new_special_count_grave(player, &this_test1) > 0 &&
						! graveyard_has_shroud(2)
						){
						choice = do_dialog(player, player, card, -1, -1, " Rebel search\n Rebel recycle\n Cancel", 0);
					}
				}
			}
			else{
				choice = 1;
			}

			if(choice == 0){
				charge_mana_for_activated_ability(player, card, -1, 0, 0, 0, 0, 0);
				if( spell_fizzled != 1 ){
					tap_card(player, card);
					instance->targets[2].card = x_value;
					instance->info_slot = 66+choice;
				}
			}

			else if(choice == 1){
					charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0);
					if( spell_fizzled != 1 ){
						if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test1, 0) != -1 ){
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

	else if( event == EVENT_RESOLVE_ACTIVATION ){

			if( instance->info_slot == 66){
				card_instance_t *parent = get_card_instance( player, instance->parent_card );
				int val = instance->targets[2].card;

				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_PERMANENT, "");
				snprintf(this_test.message, 100, "Select a Rebel permanent card with CMC %d or less.", val);
				this_test.subtype = SUBTYPE_REBEL;
				this_test.cmc = val+1;
				this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

				parent->targets[1].card = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
			}
			else if( instance->info_slot == 67){
					int selected = validate_target_from_grave(player, card, player, 0);
					if( selected != -1 ){
						from_graveyard_to_deck(player, selected, 2);
						card_instance_t *parent = get_card_instance( player, instance->parent_card );
						parent->targets[1].card = 0;
					}
			}
	}

	return 0;
}

int card_massacre2(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_SWAMP) && check_battlefield_for_subtype(1-player, TYPE_LAND, SUBTYPE_PLAINS) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! nemesis_freespell(player, card, event) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, 2, -1, -2, -2, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mind_slash(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT+GAA_CAN_SORCERY_BE_PLAYED+GAA_SACRIFICE_CREATURE,
										0, 1, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
	}
	else if( event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT+GAA_CAN_SORCERY_BE_PLAYED+GAA_SACRIFICE_CREATURE,
											0, 1, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);

				ec_definition_t this_definition;
				default_ec_definition(instance->targets[0].player, player, &this_definition);
				new_effect_coercion(&this_definition, &this_test);
			}
	}
	return global_enchantment(player, card, event);
}

int card_mogg_salvage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN) && check_battlefield_for_subtype(1-player, TYPE_LAND, SUBTYPE_ISLAND) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! nemesis_freespell(player, card, event) ){
			spell_fizzled = 1;
		}
		else{
			pick_target(&td, "TARGET_ARTIFACT");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_moggcatcher(int player, int card, event_t event){
	return subtype_searcher(player, card, event, SUBTYPE_GOBLIN);
}

int card_murderous_betrayal(int player, int card, event_t event){
	/*
	  Murderous Betrayal |B|B|B
	  Enchantment
	  {B}{B}, Pay half your life, rounded up: Destroy target nonblack creature. It can't be regenerated.
	*/
	if( event == EVENT_SHOULD_AI_PLAY ){
		ai_modifier += (player == HUMAN) ? -8 : 8;
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_B(2), (life[player]+1)/2, &td,
									"Select target nonblack creature.");
}

// nesting wurm --> squadron hawk

int card_netter_en_dal(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, effect_cannot_attack_until_eot,
										   instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_DISCARD|GAA_CAN_TARGET, MANACOST_W(1), 0, &td, "TARGET_CREATURE");
}

int card_oracles_attendants(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;
	td.special = TARGET_SPECIAL_DAMAGE_CREATURE;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *source = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		source->damage_target_player = player;
		source->damage_target_card = instance->parent_card;
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_DAMAGE_PREVENTION_CREATURE,
													0, 0, 0, 0, 0, 0, 0, &td, "TARGET_DAMAGE");
}

int card_overlaid_terrain(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		new_manipulate_all(player, card, player, &this_test, KILL_SACRIFICE);
	}

	int result = permanents_you_control_can_tap_for_mana_all_one_color(player, card, event, TYPE_LAND, -1, COLOR_TEST_ANY_COLORED, 2);
	if (event == EVENT_CAN_ACTIVATE){
		return result;
	}

	return global_enchantment(player, card, event);
}

int card_pack_hunt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			char buffer[100];
			card_ptr_t* c = cards_ptr[ get_id(instance->targets[0].player, instance->targets[0].card) ];
			scnprintf(buffer, 100, "Select a card named %s.", c->name);

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_PERMANENT, buffer);
			this_test.id = get_id(instance->targets[0].player, instance->targets[0].card);
			this_test.qty = 3;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
			shuffle(player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_parallax_inhibitor(int player, int card, event_t event){

	/* Parallax Inhibitor	|2
	 * Artifact
	 * |1, |T, Sacrifice ~: Put a fade counter on each permanent with fading you control. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_PERMANENT) &&
					get_counter_type_by_id(get_id(player, count)) == COUNTER_FADE
				  ){
					add_counter(player, count, COUNTER_FADE);
				}
				count--;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_ME, MANACOST_X(1), 0, NULL, NULL);
}

int card_parallax_nexus(int player, int card, event_t event){

	/* Parallax Nexus	|2|B
	 * Enchantment
	 * Fading 5
	 * Remove a fade counter from ~: Target opponent exiles a card from his or her hand. Activate this ability only any time you could cast a sorcery.
	 * When ~ leaves the battlefield, each player returns to his or her hand all cards he or she owns exiled with ~. */

	card_instance_t *instance = get_card_instance( player, card );

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST0, GVC_COUNTER(COUNTER_FADE), &td, NULL);
		}
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				test_definition_t this_test;
				default_test_definition(&this_test, TYPE_ANY);

				ec_definition_t this_definition;
				default_ec_definition(instance->targets[0].player, instance->targets[0].player, &this_definition);
				this_definition.ai_selection_mode = AI_MIN_VALUE;
				this_definition.effect = EC_RFG;
				int id = new_effect_coercion(&this_definition, &this_test);
				if( id > -1 && in_play(player, instance->parent_card) ){
					card_instance_t *parent = get_card_instance( player, instance->parent_card);
					if( parent->info_slot < 1 ){
						parent->info_slot = 1;
					}
					int pos = parent->info_slot;
					parent->targets[pos].player = instance->targets[0].player;
					parent->targets[pos].card = id;
					parent->info_slot++;
					create_card_name_legacy(player, instance->parent_card, id);
				}
			}
		}
	}

	if( leaves_play(player, card, event) ){
		int i;
		for(i=1; i<instance->info_slot; i++){
			if( instance->targets[i].player != -1 ){
				if( check_rfg(instance->targets[i].player, instance->targets[i].card) ){
					int to_add = get_internal_card_id_from_csv_id(instance->targets[i].card);
					add_card_to_hand(instance->targets[i].player, to_add);
					remove_card_from_rfg(instance->targets[i].player, instance->targets[i].card);
				}
			}
		}
	}

	return parallax(player, card, event);
}

int card_parallax_wave(int player, int card, event_t event){

	/* Parallax Wave	|2|W|W
	 * Enchantment
	 * Fading 5
	 * Remove a fade counter from ~: Exile target creature.
	 * When ~ leaves the battlefield, each player returns to the battlefield all cards he or she owns exiled with ~. */

	card_instance_t *instance = get_card_instance( player, card );

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.preferred_controller = player;
		td1.required_state = TARGET_STATE_DESTROYED;

		if( event == EVENT_CAN_ACTIVATE ){
			if( player == AI ){
				int result = generic_activated_ability(player, card, event, GAA_REGENERATION | GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_FADE),
														&td1, "TARGET_CREATURE");
				if( result ){
					return result;
				}
			}
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_FADE), &td, "TARGET_CREATURE");
		}

		if( event == EVENT_ACTIVATE ){
			if( player == AI ){
				int result = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_REGENERATION | GAA_CAN_TARGET, MANACOST0,
														GVC_COUNTER(COUNTER_FADE), &td1, "TARGET_CREATURE");
				if( result ){
					return generic_activated_ability(player, card, event, GAA_REGENERATION | GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_FADE),
													&td1, "TARGET_CREATURE");
				}
			}
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_FADE), &td, "TARGET_CREATURE");
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				if( player == AI && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
					regenerate_target(instance->targets[0].player, instance->targets[0].card);
				}
				int is_tok = is_token(instance->targets[0].player, instance->targets[0].card);
				int id = get_original_internal_card_id(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
				if( ! is_tok && in_play(player, instance->parent_card) ){
					card_instance_t *parent = get_card_instance( player, instance->parent_card);
					if( parent->info_slot < 1 ){
						parent->info_slot = 1;
					}
					int pos = parent->info_slot;
					parent->targets[pos].player = instance->targets[0].player;
					if( is_stolen(instance->targets[0].player, instance->targets[0].card) ){
						parent->targets[pos].player = 1-instance->targets[0].player;
					}
					parent->targets[pos].card = id;
					parent->info_slot++;
					create_card_name_legacy(player, instance->parent_card, cards_data[id].id);
				}
			}
		}
	}

	if( leaves_play(player, card, event) ){
		int i;
		for(i=1; i<instance->info_slot; i++){
			if( instance->targets[i].player != -1 ){
				if( check_rfg(instance->targets[i].player, cards_data[instance->targets[i].card].id ) ){
					int card_added = add_card_to_hand(instance->targets[i].player, instance->targets[i].card);
					remove_card_from_rfg(instance->targets[i].player, cards_data[instance->targets[i].card].id );
					put_into_play(instance->targets[i].player, card_added);
				}
			}
		}
	}

	return parallax(player, card, event);
}

int card_parallax_tide(int player, int card, event_t event){

	/* Parallax Tide	|2|U|U
	 * Enchantment
	 * Fading 5
	 * Remove a fade counter from ~: Exile target land.
	 * When ~ leaves the battlefield, each player returns to the battlefield all cards he or she owns exiled with ~. */

	card_instance_t *instance = get_card_instance( player, card );

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);

		if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, GVC_COUNTER(COUNTER_FADE), &td, "TARGET_LAND");
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				int is_tok = is_token(instance->targets[0].player, instance->targets[0].card);
				int id = get_original_internal_card_id(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
				if( ! is_tok && in_play(player, instance->parent_card) ){
					card_instance_t *parent = get_card_instance( player, instance->parent_card);
					if( parent->info_slot < 1 ){
						parent->info_slot = 1;
					}
					int pos = parent->info_slot;
					parent->targets[pos].player = instance->targets[0].player;
					if( is_stolen(instance->targets[0].player, instance->targets[0].card) ){
						parent->targets[pos].player = 1-instance->targets[0].player;
					}
					parent->targets[pos].card = id;
					parent->info_slot++;
					create_card_name_legacy(player, instance->parent_card, cards_data[id].id);
				}
			}
		}
	}

	if( leaves_play(player, card, event) ){
		return card_parallax_wave(player, card, event);
	}

	return parallax(player, card, event);
}

int card_plague_witch(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD+GAA_CAN_TARGET,
													0, 1, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_predator_flagship(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) && can_target(&td) ){
			return 1;
		}
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && has_mana_for_activated_ability(player, card, 5, 0, 0, 0, 0, 0) &&
			can_target(&td1)
		  ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0) && can_target(&td) ){
			if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && has_mana_for_activated_ability(player, card, 5, 0, 0, 0, 0, 0) &&
				can_target(&td1)
			  ){
				choice = do_dialog(player, player, card, -1, -1, " Give Flying\n Kill a creature with Flying\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, 2+(3*choice), 0, 0, 0, 0, 0) ){
				if( choice == 0 ){
					if( pick_target(&td, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						instance->info_slot = 66+choice;
					}
				}
				else if( choice == 1 ){
						if( pick_target(&td1, "TARGET_CREATURE") ){
							instance->number_of_targets = 1;
							instance->info_slot = 66+choice;
							tap_card(player, card);
						}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
		}
		if( instance->info_slot == 67 && valid_target(&td1) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

// rackling --> the rack

int card_raths_edge(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, 5, 0, 0, 0, 0, 0) && can_use_activated_abilities(player, card) && can_target(&td) &&
			can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			ai_choice = 1;
		}
		int choice = 0;
		if( ai_choice == 1 ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac a land & damage\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		if( choice == 1 ){
			tap_card(player, card);
			charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 && sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					instance->number_of_targets = 1;
					instance->info_slot = 1;
				}
			}
			else{
				 untap_card_no_event(player, card);
			}
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
				if( valid_target(&td) ){
					damage_creature_or_player(player, card, event, 1);
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

int card_rathi_assassin(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;
	td.required_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		int result = 0;
		if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) ){
			result = 1;
			if( player == AI && instance->targets[2].card == -1 ){
				result = 0;
			}
		}
		if( has_mana_for_activated_ability(player, card, 1, 2, 0, 0, 0, 0) && can_target(&td) ){
			result = 1;
		}
		return result;
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) && (player != AI || (player == AI && instance->targets[2].card == -1)) ){
			if( has_mana_for_activated_ability(player, card, 1, 2, 0, 0, 0, 0) && can_target(&td) ){
				choice = do_dialog(player, player, card, -1, -1, " Mercenary search\n Kill creature\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
				tap_card(player, card);
			}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, 1, 2, 0, 0, 0, 0);
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					instance->info_slot = 66+choice;
					tap_card(player, card);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			card_instance_t *parent = get_card_instance(player, instance->parent_card);

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a Mercenary card with CMC 3 or less.");
			this_test.subtype = SUBTYPE_MERCENARY;
			this_test.cmc = 4;
			this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
			parent->targets[2].card = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_rathi_fiend(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		lose_life(player, 3);
		lose_life(1-player, 3);
	}

	return mercenary_searcher(player, card, event, 3);
}

int card_rathi_intimidator(int player, int card, event_t event){
	fear(player, card, event);
	return mercenary_searcher(player, card, event, 2);
}

int card_refreshing_rain(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_FOREST) && check_battlefield_for_subtype(1-player, TYPE_LAND, SUBTYPE_SWAMP) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! nemesis_freespell(player, card, event) ){
			spell_fizzled = 1;
		}
		else{
			pick_target(&td, "TARGET_PLAYER");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life(instance->targets[0].player, 6);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_reverent_silence(int player, int card, event_t event){

	test_definition_t this_test2;
	default_test_definition(&this_test2, TYPE_LAND);
	this_test2.subtype = SUBTYPE_FOREST;	// not hackable in time to matter

	if( event == EVENT_MODIFY_COST && check_battlefield_for_special_card(player, card, player, 0, &this_test2) ){
		null_casting_cost(player, card);
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = 0;
		if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
			choice = do_dialog(player, player, card, -1, -1, " Make opponent gain 6 life\n Play normally\n Cancel", 1);
		}
		if( choice == 0 ){
			gain_life(1-player, 6);
		}
		else if( choice == 1 ){
				charge_mana_from_id(player, card, event, get_id(player, card));
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		this_test.type_flag = F1_NO_PWALKER;
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_rhox(int player, int card, event_t event){
	card_thorn_elemental(player, card, event);
	return regeneration(player, card, event, 2, 0, 0, 1, 0, 0);
}

int card_rising_waters(int player, int card, event_t event){
	untap_only_1_permanent_per_upkeep(player, card, event, 2, TYPE_LAND);
	return global_enchantment(player, card, event);
}

int card_rupture(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int result = pick_creature_for_sacrifice(player, card, 0);
		if( result != -1 ){
			int pow = get_power(player, result);
			kill_card(player, result, KILL_SACRIFICE);
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			this_test.keyword_flag = 1;
			new_damage_all(player, card, 2, pow, NDA_PLAYER_TOO, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_saproling_burst(int player, int card, event_t event){

	/* Saproling Burst	|4|G
	 * Enchantment
	 * Fading 7
	 * Remove a fade counter from ~: Put a |Sgreen Saproling creature token onto the battlefield. It has "This creature's power and toughness are each equal to
	 * the number of fade counters on ~."
	 * When ~ leaves the battlefield, destroy all tokens put onto the battlefield with ~. They can't be regenerated. */

	if (global_enchantment(player, card, event)){
		return 1;
	}

	fading(player, card, event, 7);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
			token_generation_t token;
			default_token_definition(player, instance->parent_card, CARD_ID_SAPROLING, &token);
			token.pow = token.tou = count_counters(player, card, COUNTER_FADE);
			token.special_infos = 66;
			generate_token(&token);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST0, GVC_COUNTER(COUNTER_FADE), NULL, NULL);
}

static int effect_saproling_cluster(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player != -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( event == EVENT_CAN_ACTIVATE ){
			if( can_use_activated_abilities(p, c) && hand_count[player] > 0 ){
				int cless = get_cost_mod_for_activated_abilities(p, c, 1, 0, 0, 0, 0, 0);
				if( has_mana(player, COLOR_COLORLESS, cless) ){
					return 1;
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			int cless = get_cost_mod_for_activated_abilities(p, c, 1, 0, 0, 0, 0, 0);
			charge_mana(player, COLOR_COLORLESS, cless);
			if( spell_fizzled != 1 ){
				discard(player, 0, player);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
		}

		if( leaves_play(p, c, event) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_saproling_cluster(int player, int card, event_t event ){//UNUSEDCARD
	/* Saproling Cluster	|1|G
	 * Enchantment
	 * |1, Discard a card: Put a 1/1 |Sgreen Saproling creature token onto the battlefield. Any player may activate this ability. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST )
		return 1;

	if( event == EVENT_SHOULD_AI_PLAY )
		return should_ai_play(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int fake = add_card_to_hand(1-player, instance->internal_card_id);
		int legacy = create_legacy_activate(1-player, fake, &effect_saproling_cluster);
		card_instance_t *leg = get_card_instance(1-player, legacy);
		leg->targets[1].player = player;
		leg->targets[1].card = card;
		obliterate_card(1-player, fake);
		hand_count[1-player]--;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_SAPROLING);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_seahunter(int player, int card, event_t event){
	/* Seahunter	|2|U|U
	 * Creature - Human Mercenary 2/2
	 * |3, |T: Search your library for a Merfolk permanent card and put it onto the battlefield. Then shuffle your library. */
	return subtype_searcher(player, card, event, SUBTYPE_MERFOLK);
}

// seal of cleansing --> seal of the primordium

int card_seal_of_doom(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = COLOR_TEST_BLACK;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return seal(player, card, event, &td, "TARGET_CREATURE");
}

int card_seal_of_fire(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
	}

	return seal(player, card, event, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_seal_of_removal(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return seal(player, card, event, &td, "TARGET_CREATURE");
}

int card_seal_of_strength(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 3);
		}
	}

	return seal(player, card, event, &td, "TARGET_CREATURE");
}

// shrieking mogg --> thundermare

static int sivvis_rouse_legacy(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 ){
				if( source->damage_target_player == player && source->damage_target_card != -1 ){
					source->info_slot = 0;
				}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_sivvis_rouse(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_PLAINS) && check_battlefield_for_subtype(1-player, TYPE_LAND, SUBTYPE_MOUNTAIN) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! nemesis_freespell(player, card, event) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &sivvis_rouse_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int sivvis_valor_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 ){
			if( source->damage_target_player == instance->targets[0].player && source->damage_target_card == instance->targets[0].card ){
				source->damage_target_player = player;
				source->damage_target_card = -1;
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_sivvis_valor(int player, int card, event_t event){
	/*
	  Sivvi's Valor |2|W
	  Instant
	  If you control a Plains, you may tap an untapped creature you control rather than pay Sivvi's Valor's mana cost.
	  All damage that would be dealt to target creature this turn is dealt to you instead.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_MODIFY_COST ){
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.allowed_controller = player;
		td2.preferred_controller = player;
		td2.illegal_abilities = 0;
		td2.illegal_state = TARGET_STATE_TAPPED;

		if( check_battlefield_for_subtype(player, TYPE_LAND, SUBTYPE_PLAINS) && can_target(&td2) ){
			instance->info_slot = 1;
			null_casting_cost(player, card);
		}
		else{
			instance->info_slot = 0;
		}
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->info_slot == 1 ){
			int choice = 0;
			if( has_mana_to_cast_iid(player, get_card_instance(player, card)->internal_card_id, event) ){
				choice = do_dialog(player, player, card, -1, -1, " Tap a creature you control\n Play normally\n Cancel", 0);
			}
			if( choice == 0 ){
				target_definition_t td2;
				default_target_definition(player, card, &td2, TYPE_CREATURE);
				td2.allowed_controller = player;
				td2.preferred_controller = player;
				td2.illegal_abilities = 0;
				td2.illegal_state = TARGET_STATE_TAPPED;
				if( new_pick_target(&td2, "Select a creature you control to tap.", 0, 1 | GS_LITERAL_PROMPT) ){
					action_on_target(player, card, 0, ACT_TAP);
					td.allow_cancel = 0;
					instance->number_of_targets = 0;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int legacy = create_legacy_effect(player, card, &sivvis_valor_legacy);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[0] = instance->targets[0];
			leg->number_of_targets = 1;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	if( event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	return 0;
}

int card_skyshroud_behemoth(int player, int card, event_t event){
	/* Skyshroud Behemoth	|5|G|G
	 * Creature - Beast 10/10
	 * Fading 2
	 * ~ enters the battlefield tapped. */
	comes_into_play_tapped(player, card, event);
	fading(player, card, event, 2);
	return 0;
}

int card_skyshroud_claim(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select a %s card.", SUBTYPE_FOREST));
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		this_test.qty = 2;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_skyshroud_poacher(int player, int card, event_t event){
	return subtype_searcher(player, card, event, SUBTYPE_ELF);
}

int card_skyshroud_ridgeback(int player, int card, event_t event){
	/* Skyshroud Ridgeback	|G
	 * Creature - Beast 2/3
	 * Fading 2 */
	fading(player, card, event, 2);
	return 0;
}

// skyshroud sentinel --> squadron hawk

int card_sneaky_homunculus(int player, int card, event_t event){
	/*
	  Sneaky Homunculus |1|U
	  Creature - Homunculus Illusion 1/1
	  Sneaky Homunculus can't block or be blocked by creatures with power 2 or greater.
	*/
	if( ! is_humiliated(player, card) ){
		if( event == EVENT_BLOCK_LEGALITY ){
			if( affect_me(player, card) ){
				if( get_power(attacking_card_controller, attacking_card) > 1 ){
					event_result = 1;
				}
			}
			if( player == attacking_card_controller && card == attacking_card ){
				if( get_power(affected_card_controller, affected_card) > 1 ){
					event_result = 1;
				}
			}
		}
	}
	return 0;
}

int card_spiritual_asylum(int player, int card, event_t event)
{
	if( in_play(player, card) && ! is_humiliated(player, card) ){
		// Creatures and lands you control have shroud.
		if (event == EVENT_ABILITIES && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE | TYPE_LAND))
			event_result |= KEYWORD_SHROUD;

		// When a creature you control attacks, sacrifice ~.
		// Simplification: only check once per attack.
		if (declare_attackers_trigger(player, card, event, 0, player, -1))
			kill_card(player, card, KILL_SACRIFICE);
	}

	return global_enchantment(player, card, event);
}

int card_stampede_driver(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, instance->parent_card, player, -1, 1, 1, KEYWORD_TRAMPLE, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD, 1, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_stronghold_biologist(int player, int card, event_t event){

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		set_flags_when_spell_is_countered(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD+GAA_SPELL_ON_STACK,
													0, 0, 2, 0, 0, 0, 0, &td, NULL);
}

int card_stronghold_machinist(int player, int card, event_t event){

	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);
	td.illegal_type = TYPE_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		set_flags_when_spell_is_countered(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD+GAA_SPELL_ON_STACK,
													0, 0, 2, 0, 0, 0, 0, &td, NULL);
}

int card_tangle_wire(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 15) ){ return 0; }

	/* Tangle Wire	|3
	 * Artifact
	 * Fading 4
	 * At the beginning of each player's upkeep, that player taps an untapped artifact, creature, or land he or she controls for each fade counter on ~. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_FADE, 4);

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( current_turn == player ){
			if( count_counters(player, card, COUNTER_FADE) <= 0 ){
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				remove_counter(player, card, COUNTER_FADE);
			}
		}
		int to_tap = count_counters(player, card, COUNTER_FADE);
		if( to_tap > 0 ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND );
			td.illegal_state = TARGET_STATE_TAPPED;
			td.allowed_controller = current_turn;
			td.preferred_controller = current_turn;
			td.who_chooses = current_turn;
			td.illegal_abilities = 0;
			td.allow_cancel = 0;

			int i;
			for( i=0;i<to_tap;i++){
				if( target_available(player, card, &td) ){
					select_target(player, card, &td, "Choose a card to tap", NULL);
					instance->number_of_targets = 1;
					int target = instance->targets[0].card;
					tap_card( current_turn, target );
				}
			}
		}
	}

	return 0;
}

int card_trickster_mage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int choice = 0;
			if( is_tapped(instance->targets[0].player, instance->targets[0].card) ){
				choice = 1;
			}
			if( player != AI ){
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

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DISCARD+GAA_CAN_TARGET,
													0, 0, 1, 0, 0, 0, 0, &td, "TWIDDLE");
}

int card_vicious_hunger(int player, int card, event_t event){
	// Also code for Douse in Gloom and Sorin't Thirst

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
			gain_life(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// viseling --> black vise

int card_volrath_the_fallen(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_HAND;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0) ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to discard.");
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 ){
				instance->info_slot = get_cmc(player, selected);
				discard_card(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, instance->info_slot, instance->info_slot);
	}

	return 0;
}

int card_wild_mammoth(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[1].card = -1;
	}

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		if( count_subtype(player, TYPE_CREATURE, -1) < count_subtype(1-player, TYPE_CREATURE, -1) ){
			if( instance->targets[1].card > -1 ){
				kill_card(player, instance->targets[1].card, KILL_REMOVE);
				instance->targets[1].card = -1;
			}
			else{
				give_control(player, card, player, card);
			}
		}
	}

	return 0;
}

int card_woodripper(int player, int card, event_t event){

	/* Woodripper	|3|G|G
	 * Creature - Beast 4/6
	 * Fading 3
	 * |1, Remove a fade counter from ~: Destroy target artifact. */

	fading(player, card, event, 3);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(1), GVC_COUNTER(COUNTER_FADE), &td, "TARGET_ARTIFACT");
}
