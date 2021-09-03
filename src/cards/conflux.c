#include "manalink.h"

// Functions
static int basic_landcycling(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white)
{
  if (event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND)
	return cycling(player, card, event, colorless, black, blue, green, red, white);
  else if (event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1)
	tutor_basic_lands(player, TUTOR_HAND, 1);

  return 0;
}


// Cards

int card_absorb_vis(int player, int card, event_t event){

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
			lose_life(instance->targets[0].player, 4);
			gain_life(player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return basic_landcycling(player, card, event, 1, 1, 0, 0, 0, 0);
}

int card_apocalypse_hydra(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot > 4 ){
			instance->info_slot*=2;
		}
		add_1_1_counters(player, card, instance->info_slot);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_1_1_COUNTER, 1, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_armillary_sphere(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char buffer[500] = "Select a basic land card";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, buffer);
		this_test.subtype = SUBTYPE_BASIC;
		this_test.qty = 2;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);
		shuffle(player);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_ME, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_banefire(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = x_value;
			set_special_flags2(player, card, SF2_X_SPELL);
			if( instance->info_slot > 4 ){
				state_untargettable(player, card, 1);
			}
		}
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		state_untargettable(player, card, 0);
		if( valid_target(&td) ){
			if( instance->info_slot > 4 ){
				int legacy = create_legacy_effect(player, card, &my_damage_cannot_be_prevented);
				get_card_instance(player, legacy)->targets[0].player = player;
				get_card_instance(player, legacy)->targets[0].card = card;
			}
			damage_creature_or_player(player, card, event, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_beacon_behemoth(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 5 | TARGET_PT_GREATER_OR_EQUAL;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_VIGILANCE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_blood_tyrant(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int i;
		for(i=0; i<2; i++){
			int result = lose_life(i, 1);
			add_1_1_counters(player, card, result);
		}
	}

	return 0;
}

int card_bloodhall_ooze(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP && current_turn == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = COLOR_TEST_BLACK | COLOR_TEST_GREEN;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = COLOR_TEST_BLACK;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			add_1_1_counter(player, card);
		}
		this_test.color = COLOR_TEST_GREEN;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			add_1_1_counter(player, card);
		}
	}

	return 0;
}

int card_bone_saw(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 1, 1, 0, 0, 0);
}

int card_charnelhoard_wurm(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_TRIGGER_OPTIONAL) ){
		char buffer[500] = "Select a card to return to your hand.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, buffer);
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return 0;
}

int card_child_of_alara(int player, int card, event_t event){
	check_legend_rule(player, card, event);

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = DOESNT_MATCH;
		new_manipulate_all(player, card, 2, &this_test, KILL_BURY);
	}

	return 0;
}

int card_cliffrunner_behemoth(int player, int card, event_t event){

	if (event == EVENT_ABILITIES && affect_me(player, card)){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);

		this_test.color = COLOR_TEST_RED;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			haste(player, card, event);
		}
		this_test.color = COLOR_TEST_WHITE;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			lifelink(player, card, event);
		}
	}

	return 0;
}

int card_conflux(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			load_text(0, "COLORWORDS");
			test_definition_t this_test;
			int i;
			for(i=1; i<6; i++){
				char msg[100];
				scnprintf(msg, 100, " Select a %s card.", text_lines[i-1]);
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				this_test.color = 1<<i;
				this_test.no_shuffle = 1;
				new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			}
			shuffle(player);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_countersquall(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && card_on_stack_controller != -1 && card_on_stack != -1){
		if( ! check_state(card_on_stack_controller, card_on_stack, STATE_CANNOT_TARGET) ){
			if( ! is_what(card_on_stack_controller, card_on_stack, TYPE_CREATURE) ){
				return 0x63;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		lose_life(instance->targets[0].player, 2);
		set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_court_homunculus(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_ARTIFACT);
	this_test.not_me = 1;
	if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
		modify_pt_and_abilities(player, card, event, 1, 1, 0);
	}

	return 0;
}

int card_cumber_stone(int player, int card, event_t event){

	boost_creature_type(player, card, event, -1, -1, 0, 0, BCT_OPPONENT_ONLY);

	if (event == EVENT_CAN_CAST){	// So it can be reused with enchantments, flash permanents, etc.
		return 1;
	} else {
		return 0;
	}
}

int card_cylian_sunsinger(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.id = get_id(player, instance->parent_card);
		pump_creatures_until_eot(player, card, player, 0, 3, 3, 0, 0, &this_test);
	}

	return generic_activated_ability(player, card, event, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0);
}

int card_drag_down(int player, int card, event_t event){
	/* Drag Down	|2|B
	 * Instant
	 * Domain - Target creature gets -1/-1 until end of turn for each basic land type among lands you control. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = count_domain(player, card);
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -amount, -amount);
			}
			kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_dragonsoul_knight(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		force_a_subtype(player, instance->parent_card, SUBTYPE_DRAGON);
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 5, 3, KEYWORD_FLYING+KEYWORD_TRAMPLE, 0);
	}

	return generic_activated_ability(player, card, event, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0);
}

int card_dreadwing(int player, int card, event_t event){

	return generic_shade(player, card, event, 0, 1, 0, 1, 0, 1, 0, 3, 0, KEYWORD_FLYING, 0);
}

int card_esperzoa(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.allow_cancel = 0;

	return bounce_permanent_at_upkeep(player, card, event, &td);
}

int card_ethersworn_adjudicator(int player, int card, event_t event){
	/* Ethersworn Adjudicator	|4|U
	 * Artifact Creature - Vedalken Knight 4/4
	 * Flying
	 * |1|W|B, |T: Destroy target creature or enchantment.
	 * |2|U: Untap ~. */

	card_instance_t *instance = get_card_instance( player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ENCHANTMENT);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XU(2, 1), 0, NULL, NULL) ){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_XBW(1, 1, 1), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_XU(2, 1), 0, NULL, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_XBW(1, 1, 1), 0, &td, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Untap\n Kill creature or enchantment\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			charge_mana_for_activated_ability(player, card, MANACOST_XU(2, 1));
			if( spell_fizzled != 1 ){
				instance->info_slot = 66+choice;
			}
		}
		else if( choice == 1 ){
				charge_mana_for_activated_ability(player, card, MANACOST_XBW(1, 1, 1));
				if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE_ENCHANTMENT") ){
					tap_card(player, card);
					instance->info_slot = 66+choice;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			untap_card(player, instance->parent_card);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_exploding_borders(int player, int card, event_t event){//UNUSEDCARD

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td1);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td1, "TARGET_PLAYER");
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td1) ){
				global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, 4, TYPE_LAND, 0, SUBTYPE_BASIC, 0, 0, 0, 0, 0, -1, 0);
				int damage = count_domain(player, card);
				if( damage > 0 ){
					damage_player(instance->targets[0].player, damage, player, card);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_extractor_demon(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_LEAVE_PLAY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.zone = TARGET_ZONE_PLAYERS;

		if( affect_me( player, card) && !( trigger_cause == card && trigger_cause_controller == player) &&
			reason_for_trigger_controller == player && can_target(&td)
		  ){
			if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) && in_play(trigger_cause_controller, trigger_cause) ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_AI(player);
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						card_instance_t *instance = get_card_instance(player, card);
						instance->number_of_targets = 0;
						if( pick_target(&td, "TARGET_PLAYER") ){
							mill(instance->targets[0].player, 2);
						}
				}
			}
		}
	}

	return unearth(player, event, MANACOST_XB(2, 1));
}

int card_faerie_mechanist(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int amount = 3;
		if( count_deck(player) < amount ){
			amount = count_deck(player);
		}
		if( amount > 0 ){
			char msg[100] = "Select an artifact card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);
			this_test.create_minideck = amount;
			this_test.no_shuffle = 1;
			if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test) != -1 ){
				amount--;
			}
			if( amount > 0 ){
				put_top_x_on_bottom(player, player, amount);
			}
		}
	}

	return 0;
}

int card_fiery_fall(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = x_value;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 5, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return basic_landcycling(player, card, event, 1, 0, 0, 0, 1, 0);
}

int card_fleshformer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 2, 2, 0, SP_KEYWORD_FEAR);
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -2, -2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_IN_YOUR_TURN+GAA_CAN_TARGET, 0, 1, 1, 1, 1, 1, 0, &td, "TARGET_CREATURE");
}

int card_font_of_mythos(int player, int card, event_t event)
{
  if (event == EVENT_DRAW_PHASE)
	event_result += 2;
  return 0;
}

static const char* target_damaged_me_this_turn(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
	int good = who_chooses == 0 ? check_special_flags2(player, card, SF2_HAS_DAMAGED_PLAYER0) :
							check_special_flags2(player, card, SF2_HAS_DAMAGED_PLAYER1);
	return good ? NULL : "dealt damage to you this turn";
}

int card_giltspire_avenger(int player, int card, event_t event)
{
  exalted(player, card, event, 0, 0);

  // |T: Destroy target creature that dealt damage to you this turn.
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int)target_damaged_me_this_turn;

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_CREATURE");
}

int card_gleam_of_resistence(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 1, 2, 0, 0);
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, player, &this_test, ACT_UNTAP);
		kill_card(player, card, KILL_DESTROY);
	}
	return basic_landcycling(player, card, event, 1, 0, 0, 0, 0, 1);
}


int card_gluttonous_slime(int player, int card, event_t event){
	devour(player, card, event, 1);
	return flash(player, card, event);
}

// goblin outlander --> vanilla

int card_goblin_razerunners(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0) ){
		return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 1, 0);
		if( spell_fizzled != 1 && ! sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		add_1_1_counter(player, instance->parent_card);
	}

	if( trigger_condition == TRIGGER_EOT && current_turn == player){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		if( eot_trigger_mode(player, card, event, player, count_1_1_counters(player, card) > 0 && can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				damage_player(instance->targets[0].player, count_1_1_counters(player, card), player, card);
			}
		}
	}

	return 0;
}

static int grixis_slavedriver_death_effect(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( trigger_condition == TRIGGER_LEAVE_PLAY ){
		if( affect_me( player, card) && trigger_cause == c && trigger_cause_controller == p
			&& reason_for_trigger_controller == player )
		{
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					generate_token_by_id(player, card, CARD_ID_ZOMBIE);
					kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	return 0;
}

int card_grixis_slavedriver(int player, int card, event_t event){
	/* Grixis Slavedriver	|5|B
	 * Creature - Zombie Giant 4/4
	 * When ~ leaves the battlefield, put a 2/2 |Sblack Zombie creature token onto the battlefield.
	 * Unearth |3|B */

	if( event == EVENT_RESOLVE_SPELL ){
		create_my_legacy(player, card, &grixis_slavedriver_death_effect);
	}

	return unearth(player, event, 3, 1, 0, 0, 0, 0);
}

int card_hellkite_hatchling(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	devour(player, card, event, 1);

	if( instance->targets[1].player == 66 ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_FLYING+KEYWORD_TRAMPLE);
	}

	return 0;
}

int card_hellspark_elemental(int player, int card, event_t event){
	card_ball_lightning(player, card, event);
	return unearth(player, event, 1, 0, 0, 0, 1, 0);
}

int card_kaleidostone(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		has_mana(player, COLOR_COLORLESS, 5)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		ai_modifier -= 24;

		card_instance_t* instance = get_card_instance(player, card);
		instance->state |= STATE_TAPPED;
		charge_mana(player, COLOR_COLORLESS, 5);
		if( spell_fizzled == 1 ){
			instance->state &= ~STATE_TAPPED;
		} else {
			produce_mana_tapped_multi(player, card, 0, 1, 1, 1, 1, 1);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
			has_mana(player, COLOR_COLORLESS, 5) && can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			int i;
			for(i=1; i<6; i++){
				declare_mana_available(player, i, 1);
			}
		}
	}

	return 0;
}

int card_kederekt_parasite(int player, int card, event_t event){

	/* Kederekt Parasite	|B
	 * Creature - Horror 1/1
	 * Whenever an opponent draws a card, if you control a |Sred permanent, you may have ~ deal 1 damage to that player. */

	if( trigger_condition == TRIGGER_CARD_DRAWN && affect_me(player, card) && reason_for_trigger_controller == player &&
		trigger_cause_controller == 1-player && in_play(player, card) && !is_humiliated(player, card)
	  ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_RED);

		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			if( event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_AI(player);
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					damage_player(1-player, 1, player, card);
			}
		}
	}

	return 0;
}

static const char* target_is_forest_or_plains(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return ((has_subtype(player, card, get_hacked_subtype(targeting_player, targeting_card, SUBTYPE_FOREST))
		   || has_subtype(player, card, get_hacked_subtype(targeting_player, targeting_card, SUBTYPE_PLAINS)))
		  ? NULL
		  : EXE_STR(0x73964C));	// ",subtype"
}
int card_knight_of_the_reliquary(int player, int card, event_t event){

	/* Knight of the Reliquary	|1|G|W
	 * Creature - Human Knight 2/2
	 * ~ gets +1/+1 for each land card in your graveyard.
	 * |T, Sacrifice |Ha Forest or |H2Plains: Search your library for a land card, put it onto the battlefield, then shuffle your library. */

	if( event == EVENT_CAN_ACTIVATE ){
		return (CAN_ACTIVATE0(player, card)
				&& CAN_TAP(player, card)
				&& (basiclandtypes_controlled[player][get_hacked_color(player, card, COLOR_GREEN)]
					|| basiclandtypes_controlled[player][get_hacked_color(player, card, COLOR_WHITE)])
				&& can_sacrifice_type_as_cost(player, 1, TYPE_LAND));
	}
	else if( event == EVENT_ACTIVATE ){
			target_definition_t td;
			base_target_definition(player, card, &td, TYPE_LAND);
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.extra = (int)target_is_forest_or_plains;
			td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

			card_instance_t* instance = get_card_instance(player, card);

			instance->number_of_targets = 0;
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) &&
				pick_next_target_noload(&td, get_hacked_land_text(player, card, "Select %a or %s to sacrifice.", SUBTYPE_FOREST, SUBTYPE_PLAINS))
			  ){
				tap_card(player, card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_LAND, "Select a land card.");
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &test);
	}
	else if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result += count_graveyard_by_type(player, TYPE_LAND );
	}
	return 0;
}

int card_knotvine_mystic(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		has_mana(player, COLOR_COLORLESS, 1)
	  ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		card_instance_t* instance = get_card_instance(player, card);
		instance->state |= STATE_TAPPED;
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled == 1 ){
			instance->state &= ~STATE_TAPPED;
		} else {
			produce_mana_tapped_multi(player, card, 0, 0, 0, 1, 1, 1);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
			has_mana(player, COLOR_COLORLESS, 1)
		  ){
			int i;
			for(i=3; i<6; i++){
				declare_mana_available(player, i, 1);
			}
		}
	}

	return 0;
}

// Lapse of Certainty --> memory lapse

int card_maelstrom_archangel (int player, int card, event_t event){
	if( has_combat_damage_been_inflicted_to_a_player(player, card, event) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ANY);
		td.illegal_type = TYPE_LAND;
		td.zone = TARGET_ZONE_HAND;
		td.illegal_abilities = 0;
		td.allowed_controller = player;
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance( player, card);

		if( select_target(player, card, &td, "Choose a card to play for free", NULL) ){
			play_card_in_hand_for_free(player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_magister_sphinx(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;
		select_target(player, card, &td, "Choose a target player", NULL);
		card_instance_t *instance = get_card_instance(player, card);
		set_life_total(instance->targets[0].player, 10);
	}
	return 0;
}

int card_malfegor(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		int amount = hand_count[player];
		discard_all(player);
		impose_sacrifice(player, card, 1-player, amount, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	return 0;
}

int card_mark_of_asylum(int player, int card, event_t event){

	card_instance_t* damage = noncombat_damage_being_prevented(event);
	if (damage && damage->damage_target_player == player && damage->damage_target_card != -1){
		damage->info_slot = 0;
	}
	return global_enchantment(player, card, event);
}

int card_martial_coup(int player, int card, event_t event){
	/* Martial Coup	|X|W|W
	 * Sorcery
	 * Put X 1/1 |Swhite Soldier creature tokens onto the battlefield. If X is 5 or more, destroy all other creatures. */

	card_instance_t *instance = get_card_instance( player, card);
	if(event == EVENT_CAN_CAST ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card)){
		instance->info_slot = x_value;
	}
	if(event == EVENT_RESOLVE_SPELL){
		int p;
		int i;
		if( instance->info_slot > 4 ){
			for( p = 0; p < 2; p++){
				int count = 0;
				while(count < active_cards_count[p]){
					card_data_t* card_d = get_card_data(p, count);
					if((card_d->type & TYPE_CREATURE) && in_play(p, count)){
						kill_card( p, count, KILL_DESTROY );
					}
					count++;
				}
			}
		}
		for (i=0; i< instance->info_slot; i++){
			generate_token_by_id(player, card, CARD_ID_SOLDIER);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_master_transmuter(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( is_sick(player, card) || is_tapped(player, card) ){
			return 0;
		}
		if( target_available(player, card, &td) && has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
			return 1;
		}
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0);
			if( spell_fizzled != 1 ){
				if( ! select_target(player, card, &td, "Select an Artifact you control", NULL) ){
					spell_fizzled = 1;
				}
				else{
					tap_card(player, card);
					instance->number_of_targets = 1;
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			char msg[100] = "Select an artifact card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}
	return 0;
}

int card_matca_rioters(int player, int card, event_t event){
	/*
	  Matca Rioters |2|G
	  Creature - Human Warrior x/x
	  Domain - Matca Rioters's power and toughness are each equal to the number of basic land types among lands you control.
	*/
	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += count_domain(player, card);
	}
	return 0;
}

int card_meglonoth(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	vigilance(player, card, event);

	if( event == EVENT_DECLARE_BLOCKERS && current_turn != player ){
		if( instance->blocking < 255 ){
			damage_player(1-player, get_power(player, card), player, card);
		}
	}

	return 0;
}


int card_might_of_alara_unlocked(int player, int card, event_t event){
	if( event == EVENT_CHECK_PUMP && has_mana(player, COLOR_GREEN, 1) ){
		int count = count_domain(player, card);
		pumpable_power[player] += count;
		pumpable_toughness[player] += count;
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int count = count_domain(player, card);
			card_instance_t *instance = get_card_instance(player, card);
			pump_until_eot( player, card, instance->targets[0].player, instance->targets[0].card, count, count );
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}
int card_might_of_alara(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 24) ){ return 0; }
	return card_might_of_alara_unlocked(player, card, event);
}

int card_mirror_sigil_sergeant(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_UPKEEP && current_turn == player && get_special_infos(player, card) != 88 ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			token_generation_t token;
			default_token_definition(player, card, get_id(player, card), &token);
			token.special_infos = 88;
			generate_token(&token);
		}
	}

	if (event == EVENT_CLEANUP && affect_me(player, card)){
		set_special_infos(player, card, 0);
	}

	return 0;
}

// nacatl outlander --> vanilla

int card_nicol_bolas_planeswalker(int player, int card, event_t event){

	/* Nicol Bolas, Planeswalker	|4|U|B|B|R
	 * Planeswalker - Bolas (5)
	 * +3: Destroy target noncreature permanent.
	 * -2: Gain control of target creature.
	 * -9: ~ deals 7 damage to target player. That player discards seven cards, then sacrifices seven permanents. */

	if (IS_ACTIVATING(event)){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_CREATURE;

		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		if( player == AI ){
			td1.allowed_controller = 1-player;
			td1.preferred_controller = 1-player;
		}

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_PERMANENT);
		td2.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		enum{
			CHOICE_KILL_NONCREATURE = 1,
			CHOICE_STEAL_CREATURE,
			CHOICE_CRUEL_ULTIMATUM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Destroy noncreature", can_target(&td), 10, 3, DLG_LITERAL_TARGET(&td, "Select target noncreature permanent"),
						"Gain control", can_target(&td1), 8, -2, DLG_TARGET(&td1, "TARGET_CREATURE"),
						"7 damage, discard 7, sac 7", can_target(&td2), 20, -9, DLG_TARGET(&td2, "TARGET_PLAYER"));

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		{
			case CHOICE_KILL_NONCREATURE:
			{
				if( valid_target(&td) ){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				}
			}
			break;

			case CHOICE_STEAL_CREATURE:
			{
				if( valid_target(&td1) ){
					gain_control(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
				}
			}
			break;

			case CHOICE_CRUEL_ULTIMATUM:
			{
				if( valid_target(&td2) ){
					damage_player(instance->targets[0].player, 7, player, card);
					multidiscard(instance->targets[0].player, 7, 0);
					impose_sacrifice(player, card, instance->targets[0].player, 7, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				}
			}
			break;
		}
	}

	return planeswalker(player, card, event, 5);
}

int card_noble_hierarch( int player, int card, event_t event){
	exalted(player, card, event, 0, 0);
	return card_generic_noncombat_1_mana_producing_creature(player, card, event);
}

int card_nyxathid  (int player, int card, event_t event){
	if(event == EVENT_POWER || event == EVENT_TOUGHNESS){
		if(affect_me(player, card ) ){
			event_result -= hand_count[1-player];
		}
	}
	return 0;
}

int card_obelisk_of_alara2(int player, int card, event_t event){

	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td_player;
		default_target_definition(player, card, &td_player, TYPE_CREATURE);
		td_player.zone = TARGET_ZONE_PLAYERS;

		target_definition_t td_opp_creature;
		default_target_definition(player, card, &td_opp_creature, TYPE_CREATURE);

		target_definition_t td_own_creature;
		default_target_definition(player, card, &td_own_creature, TYPE_CREATURE);
		td_own_creature.preferred_controller = player;

		target_definition_t td_own_creature_attacking;
		default_target_definition(player, card, &td_own_creature, TYPE_CREATURE);
		td_own_creature_attacking.required_state = TARGET_STATE_ATTACKING;
		td_own_creature_attacking.preferred_controller = player;

		if (event == EVENT_CAN_ACTIVATE
			&& (!can_use_activated_abilities(player, card) || is_tapped(player, card) || is_animated_and_sick(player, card))){
			return 0;
		}

		enum
		{
			CHOICE_GAIN_LIFE = 1,
			CHOICE_DRAW_DISCARD,
			CHOICE_WEAKEN_CREATURE,
			CHOICE_DAMAGE_PLAYER,
			CHOICE_PUMP_CREATURE,
		} choice = DIALOG(player, card, event,
						  "Gain life", 1,
						  life[player] >= 6 ? 1	// Worse choice if I have 6 or more life
						  : 8,					// Otherwise, only outweighed by red, and then only if opponent has 3 or less life.
						  DLG_MANA(MANACOST_XW(1,1)),

						  "Draw & discard", 1,
						  4,	// Just behind black.
						  DLG_MANA(MANACOST_XU(1,1)),

						  "Weaken creature", can_target(&td_opp_creature),
						  5,	// First choice unless (red: opponent <= 6 life; white: self <= 5 life; or green: have an attacking creature after blocking)
						  DLG_MANA(MANACOST_XB(1,1)),

						  "Damage player", can_target(&td_player),
						  life[1-player] > 6 ? 2			// Second-worse choice if opponent has more than six life
						  : life[1-player] > 3 ? 7		// Best choice if opponent has three or less life
						  : 9,							// Otherwise, only outweighed by white, and then only if I have 5 or less life.
						  DLG_MANA(MANACOST_XR(1,1)),

						  "Pump creature", can_target(&td_own_creature),
						  player == AI && current_turn == AI && current_phase == PHASE_AFTER_BLOCKING && can_target(&td_own_creature_attacking) ? 6
								// Pulls in front of black and blue if it's my turn, it's the post-block phase, and I have an attacking creature
						  : 3,	// Otherwise, worse than black or blue, but better than red or white (unless those are modified by a player with low life)
						  DLG_MANA(MANACOST_XG(1,1))
						  );

		if (event == EVENT_CAN_ACTIVATE){
			return choice;
		} else if (event == EVENT_ACTIVATE){
			instance->number_of_targets = 0;
			switch (choice){
				case CHOICE_GAIN_LIFE:
				case CHOICE_DRAW_DISCARD:
					break;

				case CHOICE_WEAKEN_CREATURE:
					pick_target(&td_opp_creature, "TARGET_CREATURE");
					break;

				case CHOICE_DAMAGE_PLAYER:
					pick_target(&td_player, "TARGET_PLAYER");
					break;

				case CHOICE_PUMP_CREATURE:
					if (player == AI && current_turn == AI && current_phase == PHASE_AFTER_BLOCKING && can_target(&td_own_creature_attacking)){
						pick_target(&td_own_creature_attacking, "TARGET_CREATURE");
					} else {
						pick_target(&td_own_creature, "TARGET_CREATURE");
					}
					break;
			}

			if (spell_fizzled != 1){
				tap_card(player, card);
			}
		} else if (event == EVENT_RESOLVE_ACTIVATION){
			switch (choice){
				case CHOICE_GAIN_LIFE:
					gain_life(player, 5);
					break;
				case CHOICE_DRAW_DISCARD:
					draw_cards(player, 1);
					discard(player, 0, 0);
					break;
				case CHOICE_WEAKEN_CREATURE:
					if (valid_target(&td_opp_creature)){
						pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -2, -2);
					}
					break;
				case CHOICE_DAMAGE_PLAYER:
					if (valid_target(&td_player)){
						damage_player(instance->targets[0].player, 3, player, instance->parent_card);
					}
					break;
				case CHOICE_PUMP_CREATURE:
					if (valid_target(&td_own_creature)){
						pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 4, 4);
					}
					break;
			}
		}
	}

	return 0;
}

int card_paleoloth(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.power = 4;
	this_test.power_flag = 2;
	this_test.not_me = 1;
	if( new_specific_cip(player, card, event, player, 1+player, &this_test) && ! graveyard_has_shroud(2) ){
		test_definition_t this_test2;
		default_test_definition(&this_test2, TYPE_CREATURE);
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test2);
	}
	return 0;
}

int card_paragon_of_the_amesha(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		force_a_subtype(player, instance->parent_card, SUBTYPE_ANGEL);
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 3, 3, KEYWORD_FLYING, SP_KEYWORD_LIFELINK);
	}

	return generic_activated_ability(player, card, event, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0);
}

int card_parasitic_strix(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = COLOR_TEST_BLACK;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.zone = TARGET_ZONE_PLAYERS;
			td.allow_cancel = 0;
			select_target(player, card, &td, "Choose a target player", NULL);
			card_instance_t *instance = get_card_instance(player, card);
			lose_life(instance->targets[0].player, 2);
			gain_life(player, 2);
		}
	}
	return 0;
}

int card_path_to_exile(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card)){
		pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
				int choice = 0;
				if( instance->targets[0].player != AI ){
					choice = do_dialog(instance->targets[0].player, player, card, -1, -1, " Search a basic land\n Pass", 0);
				}
				if( choice == 0 ){
					char msg[100] = "Select a basic land card.";
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_LAND,msg);
					this_test.subtype = SUBTYPE_BASIC;
					new_global_tutor(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &this_test);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_progenitus(int player, int card, event_t event)
{
  /* Progenitus	|W|W|U|U|B|B|R|R|G|G
   * Legendary Creature - Hydra Avatar 10/10
   * Protection from everything
   * If ~ would be put into a graveyard from anywhere, reveal ~ and shuffle it into its owner's library instead. */

  // Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

  check_legend_rule(player, card, event);
  unblockable(player, card, event);
  return 0;
}

int card_rakka_mar(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	haste(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.pow = 3;
		token.tou = 1;
		token.s_key_plus = SP_KEYWORD_HASTE;
		generate_token(&token);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 1, 0, 0, 0, 0);
}

int card_reliquiary_tower(int player, int card, event_t event){
	if( current_turn == player && event == EVENT_MAX_HAND_SIZE ){
		event_result+=99;
	}
	return mana_producer(player, card, event);
}

int card_rotting_rats(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		discard(player, 0, 0);
		discard(1-player, 0, 0);
	}

	return unearth(player, event, 1, 1, 0, 0, 0, 0);
}

// rupture spire --> transguild promenade

int card_salvage_slasher(int player, int card, event_t event){
	if( event == EVENT_POWER && affect_me(player, card) ){
		event_result += count_graveyard_by_type(player, TYPE_ARTIFACT );
	}
	return 0;
}

int card_scarland_thrinax(int player, int card, event_t event){
	/* Scarland Thrinax	|B|R|G
	 * Creature - Lizard 2/2
	 * Sacrifice a creature: Put a +1/+1 counter on ~. */
	/* Bloodflow Connoisseur	|2|B
	 * Creature - Vampire 1/1
	 * Sacrifice a creature: Put a +1/+1 counter on ~. */
	return generic_husk(player, card, event, TYPE_CREATURE, 101, 101, 0, 0);
}

int card_scattershot_archer(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		new_damage_all(player, instance->parent_card, 2, 1, 0, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_scepter_of_dominance(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 1, 0, &td, "TARGET_CREATURE");
}

int card_scepter_of_fugue( int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_NONE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			discard( instance->targets[0].player, 0, 0);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_IN_YOUR_TURN, 1, 1, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_scepter_of_insight( int player, int card, event_t event){

	if(event == EVENT_RESOLVE_ACTIVATION){
		draw_cards(player, 1);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 3, 0, 1, 0, 0, 0, 0, 0, 0);
}

int card_sedraxis_alchemist(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.color = COLOR_TEST_BLUE;
		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_PERMANENT);
			td.illegal_type = TYPE_LAND;
			td.allow_cancel = 0;
			select_target(player, card, &td, "Choose target nonland permanent", NULL);
			card_instance_t *instance = get_card_instance(player, card);
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_shambling_remains(int player, int card, event_t event){
	cannot_block(player, card, event);
	return unearth(player, event, 0, 1, 0, 0, 1, 0);
}

int card_shard_convergence(int player, int card, event_t event)
{
  if (event == EVENT_CAN_CAST)
	return 1;

  /* Search your library for a Plains card, an Island card, a Swamp card, and a Mountain card. Reveal those cards and put them into your hand. Then shuffle your
   * library. */
  if (event == EVENT_RESOLVE_SPELL)
	{
	  subtype_t subtypes[4] = { SUBTYPE_PLAINS, SUBTYPE_ISLAND, SUBTYPE_SWAMP, SUBTYPE_MOUNTAIN };
	  int i;
	  for (i = 0; i < 4; ++i)
		{
		  test_definition_t this_test;
		  new_default_test_definition(&this_test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a card", subtypes[i]));
		  this_test.subtype = get_hacked_subtype(player, card, subtypes[i]);
		  if (i != 3)
			this_test.no_shuffle = 1;
		  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_sigil_of_the_empty_throne(int player, int card, event_t event){
	/* Sigil of the Empty Throne	|3|W|W
	 * Enchantment
	 * Whenever you cast an enchantment spell, put a 4/4 |Swhite Angel creature token with flying onto the battlefield. */

	if( specific_spell_played(player, card, event, player, 2, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		generate_token_by_id(player, card, CARD_ID_ANGEL);
	}

	return global_enchantment(player, card, event);
}


int card_sludge_strider(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_ARTIFACT);
	this_test.not_me = 1;

	if( has_mana(player, COLOR_COLORLESS, 1) ){
		if( new_specific_cip(player, card, event, player, 1+player, &this_test) ){
			charge_mana(player, COLOR_COLORLESS, 1);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
				instance->number_of_targets = 1;
				lose_life(instance->targets[0].player, 1);
				gain_life(player, 1);
			}
		}
	}

	if( trigger_condition == TRIGGER_LEAVE_PLAY ){
		if( affect_me( player, card) && reason_for_trigger_controller == player ){
			if( new_make_test_in_play(trigger_cause_controller, trigger_cause, -1, &this_test) ){
				if(event == EVENT_TRIGGER){
					event_result |= 1+player;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						charge_mana(player, COLOR_COLORLESS, 1);
						if( spell_fizzled != 1 && pick_target(&td, "TARGET_PLAYER") ){
							instance->number_of_targets = 1;
							lose_life(instance->targets[0].player, 1);
							gain_life(player, 1);
						}
				}
			}
		}
	}

	return 0;
}

int card_souls_majesty(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card)){
		pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				draw_cards(player, get_power(instance->targets[0].player, instance->targets[0].card));
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sphinx_summoner(int player, int card, event_t event){
	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		char msg[100] = "Select an artifact creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);
		this_test.special_selection_function = &is_artifact_creature_by_internal_id;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}
	return 0;
}

int card_spore_burst(int player, int card, event_t event){
	/* Spore Burst	|3|G
	 * Sorcery
	 * Domain - Put a 1/1 |Sgreen Saproling creature token onto the battlefield for each basic land type among lands you control. */

	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			generate_tokens_by_id(player, card, CARD_ID_SAPROLING, count_domain(player, card));
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sylvan_bounty(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life(instance->targets[0].player, 8);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return basic_landcycling(player, card, event, 1, 0, 0, 1, 0, 0);
}


int card_telemin_performance(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		return would_valid_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int *deck = deck_ptr[instance->targets[0].player];
				if( deck[0] != -1 ){
					int count = 0;
					int good = 0;
					while( deck[count] != -1 ){
							if( is_what(-1, deck[count], TYPE_CREATURE) ){
								good = 1;
								break;
							}
							count++;
					}
					show_deck(player, deck, count+1, "Cards revealed by Telemin Performance.", 0, 0x7375B0 );
					show_deck(1-player, deck, count+1, "Cards revealed by Telemin Performance.", 0, 0x7375B0 );
					if( good == 1 ){
						int card_added = add_card_to_hand(player, deck[count]);
						remove_card_from_deck(instance->targets[0].player, count);
						put_into_play(player, card_added);
						card_instance_t *stolen = get_card_instance( player, card_added);
						stolen->state |= STATE_OWNED_BY_OPPONENT;
					}
					else{
						count++;
					}
					if( count > 0 ){
						mill(instance->targets[0].player, count);
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_thornling(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
		return 1;
	}
	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) ){
			choice = do_dialog(player, player, card, -1, -1, " Haste\n Trample\n Indestructible\n +1/-1\n -1/+1\n Cancel", 2);
		}
		else{
			choice = do_dialog(player, player, card, -1, -1, " +1/-1\n -1/+1\n Cancel", 3);
			choice += 3;
		}

		if( choice == 5 ){
			spell_fizzled = 1;
		}
		else{
			instance->info_slot = choice;
			if( choice == 3 || choice == 4 ){
				charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			}
			else{
				charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
			}
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		int choice = instance->info_slot;
		if( choice == 0 ){
			pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, 0, SP_KEYWORD_HASTE );
		}
		else if( choice == 1 ){
			pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, KEYWORD_TRAMPLE, 0 );
		}
		else if( choice == 2 ){
			pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
		}
		else if (choice == 3 ){
			pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, -1);
		}
		else if (choice == 4 ){
			pump_until_eot(player, instance->parent_card, player, instance->parent_card, -1, 1);
		}
	}

	return 0;
}

int card_traumatic_visions(int player, int card, event_t event)
{
  int cyc = basic_landcycling(player, card, event, MANACOST_XU(1,1));
  return cyc ? cyc : card_counterspell(player, card, event);
}

int card_tukatongue_thallid(int player, int card, event_t event){
	/* Tukatongue Thallid	|G
	 * Creature - Fungus 1/1
	 * When ~ dies, put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	if( in_play(player, card) && graveyard_from_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_SAPROLING);
	}
	return 0;
}

// valeron outlander --> vanilla

// vedalken outlander --> esper cormorants


int card_voices_from_the_void(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				multidiscard(instance->targets[0].player, count_domain(player, card), 0);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_volcanic_fallout(int player, int card, event_t event){
	if(event == EVENT_CAST_SPELL && affect_me(player, card)){
		card_instance_t *instance = get_card_instance(player, card);
		instance->state |= STATE_CANNOT_TARGET;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		damage_player(HUMAN, 2, player, card);
		damage_player(AI, 2, player, card);
	}
	return card_pyroclasm(player, card, event);
}

int card_voracious_dragon(int player, int card, event_t event){
	/* Voracious Dragon	|3|R|R
	 * Creature - Dragon 4/4
	 * Flying
	 * Devour 1
	 * When ~ enters the battlefield, it deals damage to target creature or player equal to twice the number of Goblins it devoured. */

	devour(player, card, event, 1);

	if( comes_into_play(player, card, event) ){
		// Yes, it still targets even if it didn't devour any goblins.
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_target0(player, card, get_card_instance(player, card)->targets[1].player*2);
		}
	}
	return 0;
}

int card_wall_of_reverence(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_EOT && current_turn == player){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;

		if( eot_trigger_mode(player, card, event, player, can_target(&td) ? RESOLVE_TRIGGER_AI(player) : 0) ){
			if( pick_target(&td, "ASHNODS_BATTLEGEAR") ){	// "Select target creature you control."
				card_instance_t *instance = get_card_instance( player, card );
				instance->number_of_targets = 1;
				gain_life(player, get_power(instance->targets[0].player, instance->targets[0].card) );
			}
		}
	}

	return 0;
}

int card_worldly_counsel(int player, int card, event_t event){

	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			impulse_effect(player, count_domain(player, card), 0);
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// zombie outlander --> vanilla



