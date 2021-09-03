#include "manalink.h"

// ---- GLOBAL FUNCTIONS

// ---- CARDS
int card_academy_rector(int player, int card, event_t event)
{
  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player))
	  && exile_from_owners_graveyard(player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ENCHANTMENT, "Select an enchantment card.");
	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &test);
	}

  return 0;
}

int card_aether_sting(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, 1-player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		damage_player(1-player, 1, player, card);
	}

	return global_enchantment(player, card, event);
}


int card_apprentice_necromancer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_B(1), 0, NULL, NULL) ){
			return count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(player);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if(	charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 0) != -1 ){
				tap_card(player, card);
				kill_card(player, card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			int dead = reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			create_targetted_legacy_effect(player, card, &haste_and_sacrifice_eot, player, dead);
		}
	}

	return 0;
}

int card_attrition(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_B(1), 0,
									&td, get_sleighted_color_text(player, card, "Select target non%s creature.", COLOR_BLACK));
}

int card_aura_thief(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		new_manipulate_all(player, card, 1-player, &this_test, ACT_GAIN_CONTROL);
	}

	return 0;
}

int card_bloodshot_cyclops(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, instance->targets[2].player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET+GAA_SACRIFICE_CREATURE,
													0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_body_snatcher(int player, int card, event_t event){

	/* Body Snatcher	|2|B|B
	 * Creature - Minion 2/2
	 * When ~ enters the battlefield, exile it unless you discard a creature card.
	 * When ~ dies, exile ~ and return target creature card from your graveyard to the battlefield. */

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
	this_test.zone = TARGET_ZONE_HAND;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			ai_modifier -= 1000;
		}
	}

	if( comes_into_play(player, card, event) ){
		this_test.zone = TARGET_ZONE_HAND;
		int result = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
		if( result != -1 ){
			discard_card(player, result);
		}
		else{
			kill_card(player, card, KILL_REMOVE);
		}
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		exile_from_owners_graveyard(player, card);

		if (new_special_count_grave(player, &this_test) && !graveyard_has_shroud(player)){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_CMC, &this_test);
		}
	}

	return 0;
}

int card_brine_seer(int player, int card, event_t event){
	/* Brine Seer	|3|U
	 * Creature - Human Wizard 1/1
	 * |2|U, |T: Reveal any number of |Sblue cards in your hand. Counter target spell unless its controller pays |1 for each card revealed this way. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_BLUE));
	this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SPELL_ON_STACK, MANACOST_XU(2, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if(	charge_mana_for_activated_ability(player, card, MANACOST_XU(2, 1)) ){
			int amount = reveal_cards_from_your_hand(player, card, &this_test);
			counterspell(player, card, EVENT_CAST_SPELL, NULL, 0);
			instance->targets[1].card = amount;
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, instance->targets[1].card);
	}

	return 0;
}

static int bubbling_muck_effect(int player, int card, event_t event){
	produce_mana_when_subtype_is_tapped(2, event, TYPE_PERMANENT, SUBTYPE_SWAMP, COLOR_BLACK, 1);

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_bubbling_muck(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &bubbling_muck_effect);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_caltrops(int player, int card, event_t event)
{
  // Whenever a creature attacks, ~ deals 1 damage to it.
  int amt;
  if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, 2, -1)))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
	  for (--amt; amt >= 0; --amt)
		if (in_play(current_turn, attackers[amt]))
		  damage_creature(current_turn, attackers[amt], 1, player, card);
	}

  return 0;
}

int card_carnival_of_souls(int player, int card, event_t event){

	if( specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		produce_mana(player, COLOR_BLACK, 1);
		lose_life(player, 1);
	}

	return global_enchantment(player, card, event);
}

int card_cinder_seer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_RED));
	this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_RED);
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XR(2, 1), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if(	charge_mana_for_activated_ability(player, card, MANACOST_XR(2, 1)) ){
			if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				int amount = reveal_cards_from_your_hand(player, card, &this_test);
				instance->targets[1].card = amount;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && instance->targets[1].card > 0 ){
			damage_creature_or_player(player, card, event, instance->targets[1].card);
		}
	}

	return 0;
}

int card_covetous_dragon( int player, int card, event_t event){
	if( event == EVENT_STATIC_EFFECTS && ! is_humiliated(player, card) ){
		if( count_subtype(player, TYPE_ARTIFACT, -1) < 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return 0;
}

int card_donate(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 22) ){ return 0; }

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td1, NULL, 1, NULL) ){
			return can_target(&td);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td1, "TARGET_PLAYER") ){
			new_pick_target(&td, "Select target permanent you control.", 1, 1 | GS_LITERAL_PROMPT);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td1, 0) && validate_target(player, card, &td, 1) ){
			if( instance->targets[0].player != player ){
				give_control(player, card, instance->targets[1].player, instance->targets[1].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_elvish_piper(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);

		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_G(1), 0, NULL, NULL);
}

int card_emperor_crocodile(int player, int card, event_t event){
	/*
	  Emperor Crocodile |3|G
	  Creature - Crocodile 5/5
	  When you control no other creatures, sacrifice Emperor Crocodile.
	*/

	if( event == EVENT_STATIC_EFFECTS && ! is_humiliated(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		if( ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_eradicate(int player, int card, event_t event){

	/* Eradicate	|2|B|B
	 * Sorcery
	 * Exile target non|Sblack creature. Search its controller's graveyard, hand, and library for all cards with the same name as that creature and exile
	 * them. Then that player shuffles his or her library. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int id = get_id(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			lobotomy_effect(player, instance->targets[0].player, id, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, get_sleighted_color_text(player, card, "Select target non%s creature.", COLOR_BLACK), 1, NULL);
}

int card_extruder(int player, int card, event_t event){
	/*
	  Extruder
	  Artifact Creature - Juggernaut 4/3
	  Echo {4} (At the beginning of your upkeep, if this came under your control since the beginning of your last upkeep, sacrifice it unless you pay its echo cost.)
	  Sacrifice an artifact: Put a +1/+1 counter on target creature.
	*/

	echo(player, card, event, MANACOST_X(4));

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ARTIFACT, "Select an artifact to sacrifice.");
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

	if (event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_false_prophet(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, 2, &this_test, KILL_REMOVE);
	}

	return 0;
}

int card_fend_off(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return us_cycling(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			negate_combat_damage_this_turn(player, card, instance->targets[0].player, instance->targets[0].card, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_field_surgeon(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities = 0;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.extra = damage_card;
	td1.illegal_abilities = 0;
	td1.special = TARGET_SPECIAL_DAMAGE_CREATURE;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_target(&td) ){
			return generic_activated_ability(player, card, event, GAA_DAMAGE_PREVENTION | GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( new_pick_target(&td, "Select target creature to tap.", 0, 1 | GS_LITERAL_PROMPT) ){
				tap_card(instance->targets[0].player,  instance->targets[0].card);
				instance->number_of_targets = 0;
				pick_target(&td1, "TARGET_DAMAGE");
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td1, 0) ){
			card_instance_t *source = get_card_instance(instance->targets[0].player,  instance->targets[0].card);
			if( source->info_slot > 0 ){
				source->info_slot--;
			}
		}
	}

	return 0;
}

int card_flame_jet(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return us_cycling(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, 3, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_fledgling_osprey(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && affect_me(player, card) && is_enchanted(player, card)){
		event_result |= KEYWORD_FLYING;
	}

	return 0;
}

static const char* target_is_not_token(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  // As there's no way to require a given target to have more than one of the types in required_type, we can't just use TARGET_TYPE_TOKEN.
  return ! is_token(player, card) ? NULL : "can't target a token.";
}

int card_flicker2(int player, int card, event_t event){
	/* Flicker	|1|W
	 * Sorcery
	 * Exile target nontoken permanent, then return it to the battlefield under its owner's control. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_PERMANENT);
	td.preferred_controller = player;
	td.allow_cancel = 0;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int)target_is_not_token;
	if( player == AI ){
		td.required_state = TARGET_STATE_DESTROYED;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( player == AI ){
			return generic_spell(player, card, event, GS_CAN_TARGET | GS_REGENERATION, &td, NULL, 1, NULL);
		}
		else{
			return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI ){
			return generic_spell(player, card, event, GS_CAN_TARGET | GS_REGENERATION, &td, NULL, 1, NULL);
		}
		else{
			return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nontoken permanent.", 1, NULL);
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			if( player == AI ){
				regenerate_target(instance->targets[0].player, instance->targets[0].card);
			}
			blink_effect(instance->targets[0].player, instance->targets[0].card, 0);
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_fodder_cannon(int player, int card, event_t event){
	/*
	  Fodder Cannon |4
	  Artifact
	  {4}, {T}, Sacrifice a creature: Fodder Cannon deals 4 damage to target creature.
	*/
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		damage_target0(player, card, 4);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_CREATURE, MANACOST_X(4), 0, &td, "TARGET_CREATURE");
}

int card_gamekeeper(int player, int card, event_t event)
{
  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player))
	  && exile_from_owners_graveyard(player, card))
	{
	  int* deck = deck_ptr[player];
	  int z;
	  for (z = 0; deck[z] != -1 && z < 500; z++)
		if (cards_data[deck[z]].type & TYPE_CREATURE)
		  {
			int iid = deck[z];
			remove_card_from_deck(player, z);
			if (z > 0)
			  mill(player, z);

			put_into_play(player, add_card_to_hand(player, iid));
			return 0;
		  }

	  // Not found - mill them all!
	  mill(player, count_deck(player));
	}

  return 0;
}

int card_goblin_marshall(int player, int card, event_t event){
	/* Goblin Marshal	|4|R|R
	 * Creature - Goblin Warrior 3/3
	 * Echo |4|R|R
	 * When ~ enters the battlefield or dies, put two 1/1 |Sred Goblin creature tokens onto the battlefield. */

	echo(player, card, event, MANACOST_XR(4, 2));

	if( comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 2);
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		generate_tokens_by_id(player, card, CARD_ID_GOBLIN, 2);
	}

	return 0;
}

int card_heart_warden(int player, int card, event_t event){

	if( IS_GAA_EVENT(event) ){
		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_CAN_ACTIVATE ){
			if( mana_producer(player, card, event) ){
				return 1;
			}
			return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL);
		}

		if( event == EVENT_ACTIVATE ){
			int choice = 0;
			if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL) ){
				choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Sac & draw 1 card\n Cancel", 1);
			}

			if( choice == 0 ){
				return mana_producing_creature(player, card, event, 24, COLOR_GREEN, 1);
			}
			else if( choice == 1 ){
					if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
						instance->info_slot = 1;
						kill_card(player, card, KILL_SACRIFICE);
					}
			}
			else{
				spell_fizzled = 1;
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 1 ){
				draw_cards(player, 1);
			}
		}
	}
	else{
		return mana_producing_creature(player, card, event, 24, COLOR_GREEN, 1);
	}

	return 0;
}

static void hunting_moa_counter(int player, int card, int illegal_abil){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( illegal_abil > 0 ){
		td.illegal_abilities = illegal_abil;
	}
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t* instance = get_card_instance(player, card);

	if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
		add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
	}
}

int card_hunting_moa(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XG(2, 1));

	if( comes_into_play(player, card, event) || this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		hunting_moa_counter(player, card, get_protections_from(player, card));
	}

	return 0;
}

int card_impatience(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		instance->targets[current_turn+1].player = get_specific_storm_count(current_turn);
	}

	if( event == EVENT_CAST_SPELL && ! affect_me(player, card) && ! is_what(affected_card_controller, affected_card, TYPE_LAND) &&
		affected_card_controller == current_turn
	  ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->targets[current_turn+1].player < 0 ){
			instance->targets[current_turn+1].player = 0;
		}
		instance->targets[current_turn+1].player++;
	}

	if(trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		int trigs = instance->targets[current_turn+1].player < 1 ? 1 : 0;
		instance->targets[current_turn+1].player = 0;
		if( eot_trigger_mode(player, card, event, ANYBODY, (trigs ? RESOLVE_TRIGGER_MANDATORY : 0)) ){
			damage_player(current_turn, 2, player, card);
		}
	}
	return global_enchantment(player, card, event);
}


int card_iridescent_drake(int player, int card, event_t event){

	/* Iridescent Drake	|3|U
	 * Creature - Drake 2/2
	 * Flying
	 * When ~ enters the battlefield, put target Aura card from a graveyard onto the battlefield under your control attached to ~. */

	if( comes_into_play(player, card, event) && ! graveyard_has_shroud(2) && any_in_graveyard_by_subtype(ANYBODY, SUBTYPE_AURA_CREATURE) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;
		td.illegal_abilities = 0;

		card_instance_t *instance = get_card_instance(player, card);

		instance->targets[0].player = 1-player;
		if( any_in_graveyard_by_subtype(1-player, SUBTYPE_AURA_CREATURE) ){
			if( any_in_graveyard_by_subtype(player, SUBTYPE_AURA_CREATURE) ){
				pick_target(&td, "TARGET_PLAYER");
			}
		}
		else{
			instance->targets[0].player = player;
		}

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select target Aura card with enchant creature.");
		this_test.subtype = SUBTYPE_AURA_CREATURE;
		int card_added = new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_CMC, &this_test);
		if (instance->targets[0].player != player){
			get_card_instance(player, card_added)->state ^= STATE_OWNED_BY_OPPONENT;
		}
		put_into_play_aura_attached_to_target(player, card_added, player, card);
	}

	return 0;
}

int card_ivy_seer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_GREEN));
	this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XG(2, 1), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if(	charge_mana_for_activated_ability(player, card, MANACOST_XG(2, 1)) ){
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE")){
				int amount = reveal_cards_from_your_hand(player, card, &this_test);
				instance->targets[1].card = amount;
				instance->number_of_targets = 1;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = instance->targets[1].card;
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, amount, amount);
		}
	}

	return 0;
}

int card_jasmine_seer(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XW(2, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_WHITE));
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		this_test.zone = TARGET_ZONE_HAND;

		if(	charge_mana_for_activated_ability(player, card, MANACOST_XW(2, 1)) ){
			if( spell_fizzled != 1 ){
				int amount = reveal_cards_from_your_hand(player, card, &this_test);
				instance->targets[1].card = amount;
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, instance->targets[1].card*2);
	}

	return 0;
}

int card_junk_diver(int player, int card, event_t event)
{
  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ARTIFACT, "Select an artifact card.");
	  tutor_from_grave_to_hand_except_for_dying_card(player, card, 1, AI_MAX_CMC, &test);
	}

  return 0;
}

int card_keldon_champion(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XR(2, 2));

	haste(player, card, event);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER")){
			damage_player(instance->targets[0].player, 3, player, card);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_keldon_vandals(int player, int card, event_t event){

	echo(player, card, event, MANACOST_XR(2, 1));

	if (comes_into_play(player, card, event)){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_ARTIFACT);
		td1.allow_cancel = 0;

		if (can_target(&td1) && pick_target(&td1, "TARGET_ARTIFACT")){
			card_instance_t* instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_marker_beetle(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.preferred_controller = player;
		td1.allow_cancel = 0;

		card_instance_t *instance= get_card_instance(player, card);
		if( can_target(&td1) && pick_target(&td1, "TARGET_CREATURE") ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL);
}

int card_master_healer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		if( target->info_slot > 4 ){
			target->info_slot-=4;
		}
		else{
			target->info_slot = 0;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST0, 0, &td, "TARGET_DAMAGE");
}

int card_masticore(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( hand_count[player] > 0 ){
			int ai_choice = 0;
			if( (!can_use_activated_abilities(player, card) && !can_attack(player, card)) ||
				hand_count[player] < count_upkeeps(player)	// In second and subsequent upkeeps, this compares handcount after discard from prior upkeeps against total number of upkeeps, not number remaining
			 ){
				ai_choice = 1;
			}
			int choice = do_dialog(player, player, card, -1, -1, " Pay upkeep\n Pass", ai_choice);
			if( choice == 0 ){
				discard(player, 0, player);
				kill--;
			}
		}
		if( kill == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST_X(2), 0, NULL, NULL) ){
			return 99;
		}
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(2), 0, &td, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
			int choice = 67;
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_REGENERATION, MANACOST_X(2), 0, NULL, NULL) ){
				choice = 66;
			}
			else{
				pick_target(&td, "TARGET_CREATURE");
			}
			instance->info_slot = choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && can_regenerate(instance->parent_controller, instance->parent_card) > 0 ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 67 && valid_target(&td) ){
			damage_target0(player, card, 1);
		}
	}
	return 0;
}

int card_mental_discipline(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_XU(1, 1), 0, NULL, NULL);
}

int card_metalworker(int player, int card, event_t event){

	if( IS_GAA_EVENT(event) || (event == EVENT_COUNT_MANA && affect_me(player, card)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");
		this_test.zone = TARGET_ZONE_HAND;

		if( event == EVENT_COUNT_MANA && affect_me(player, card) && mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			declare_mana_available(player, COLOR_COLORLESS, 2 * check_battlefield_for_special_card(player, card, player, 0, &this_test));
		}

		if(event == EVENT_CAN_ACTIVATE ){
			return mana_producer(player, card, event);
		}

		if( event == EVENT_ACTIVATE ){
			int result = reveal_cards_from_your_hand(player, card, &this_test);
			produce_mana_tapped(player, card, COLOR_COLORLESS, 2*result);
		}
	}

	return 0;
}

int card_metathran_elite(int player, int card, event_t event){

	if(event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( attacking_card_controller == player && attacking_card == card ){
			if( is_enchanted(player, card) ){
				event_result = 1;
			}
		}
	}

	return 0;
}

int card_multanis_decree(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		int result = new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		gain_life(player, result*2);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_nightshade_seer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_BLACK));
	this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XB(2, 1), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if(	charge_mana_for_activated_ability(player, card, MANACOST_XB(2, 1)) ){
			if( pick_target(&td, "TARGET_CREATURE")){
				tap_card(player, card);
				int amount = reveal_cards_from_your_hand(player, card, &this_test);
				instance->targets[1].card = amount;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = instance->targets[1].card;
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -amount, -amount);
		}
	}

	return 0;
}

int card_opalescence(int player, int card, event_t event){

	/* Opalescence	|2|W|W
	 * Enchantment
	 * Each other non-Aura enchantment is a creature with power and toughness each equal to its converted mana cost. It's still an enchantment. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = 0;
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_ENCHANTMENT) && ! is_what(i, count, TYPE_CREATURE) && ! has_subtype(i, count, SUBTYPE_AURA) ){
						if( !(i == player && count == card) ){
							card_instance_t *lnd = get_card_instance(i, count);
							int k;
							int mnt = 1;
							for(k=0; k<instance->info_slot; k++){
								if( instance->targets[k].player == lnd->internal_card_id ){
									mnt = 0;
									break;
								}
							}
							if( mnt == 1 ){
								int pos = instance->info_slot;
								if( pos < 16 ){
									int newtype = create_a_card_type(lnd->internal_card_id);
									cards_at_7c7000[newtype]->type |= (cards_data[lnd->internal_card_id].type | TYPE_CREATURE);
									cards_at_7c7000[newtype]->power = get_cmc(i, count);
									cards_at_7c7000[newtype]->toughness = get_cmc(i, count);
									instance->targets[pos].player = lnd->internal_card_id;
									instance->targets[pos].card = newtype;
									instance->info_slot++;
								}
							}

						}
					}
					count--;
			}
		}
	}

	if( event == EVENT_CHANGE_TYPE && is_what(affected_card_controller, affected_card, TYPE_ENCHANTMENT) &&
		! has_subtype(affected_card_controller, affected_card, SUBTYPE_AURA)
	  ){
		card_instance_t *lnd = get_card_instance(affected_card_controller, affected_card);
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( lnd->internal_card_id == instance->targets[i].player ){
				event_result = instance->targets[i].card;
				break;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && is_what(affected_card_controller, affected_card, TYPE_ENCHANTMENT) && ! affect_me(player, card) &&
		! is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! has_subtype(affected_card_controller, affected_card, SUBTYPE_AURA)
		){
			card_instance_t *lnd = get_card_instance(affected_card_controller, affected_card);
			int k;
			int mnt = 1;
			for(k=0; k<instance->info_slot; k++){
				if( instance->targets[k].player == lnd->internal_card_id ){
					mnt = 0;
					break;
				}
			}
			if( mnt == 1 ){
				int pos = instance->info_slot;
				if( pos < 16 ){
					int newtype = create_a_card_type(lnd->internal_card_id);
					cards_at_7c7000[newtype]->type |= (cards_data[lnd->internal_card_id].type | TYPE_CREATURE);
					cards_at_7c7000[newtype]->power = get_cmc(affected_card_controller, affected_card);
					cards_at_7c7000[newtype]->toughness = get_cmc(affected_card_controller, affected_card);
					instance->targets[pos].player = lnd->internal_card_id;
					instance->targets[pos].card = newtype;

					instance->info_slot++;
				}
			}

	}

	return global_enchantment(player, card, event);
}

int card_opposition(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.illegal_state = TARGET_STATE_TAPPED;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE | TYPE_LAND | TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td1, NULL) ){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if (charge_mana_for_activated_ability(player, card, MANACOST0) && pick_next_target_noload(&td, "Select an untapped creature you control.")){
			target_t to_tap = instance->targets[0];	// struct copy
			instance->number_of_targets = 0;
			if (pick_target(&td1, "ICY_MANIPULATOR")){
				tap_card(to_tap.player, to_tap.card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			tap_card( instance->targets[0].player, instance->targets[0].card );
		}
	}
	return global_enchantment(player, card, event);
}

int card_pattern_of_rebirth(int player, int card, event_t event){
	/* Pattern of Rebirth	|3|G
	 * Enchantment - Aura
	 * Enchant creature
	 * When enchanted creature dies, that creature's controller may search his or her library for a creature card and put that card onto the battlefield. If that player does, he or she shuffles his or her library. */

	int controller = get_card_instance(player, card)->damage_target_player;
	if( attached_creature_dies_trigger_for_controller(player, card, event, RESOLVE_TRIGGER_AI(controller)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		new_global_tutor(controller, controller, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int true_phyrexian_negator(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	// An approximation - tells the AI that this will be destroyed if it blocks or is blocked by a creature with at least 1 power.
	if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card)){
		instance->destroys_if_blocked |= DIFB_ASK_CARD;
	}
	if (event == EVENT_CHECK_DESTROY_IF_BLOCKED && affect_me(player, card) && !is_humiliated(player, card)
		&& get_power(attacking_card_controller, attacking_card) >= 1){
		event_result |= 2;
	}

	if( damage_dealt_to_me_arbitrary(player, card, event, 0, player, card) ){
		impose_sacrifice(player, card, player, instance->targets[7].card, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_phyrexian_negator(int player, int card, event_t event){
	if( in_play(player, card) && ! is_unlocked(player, card, event, 32) ){
		kill_card(player, card, KILL_SACRIFICE);
		return 0;
	}
	return true_phyrexian_negator(player, card, event);
}

int card_plow_under(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				put_on_top_of_deck(instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 2, NULL);
}

int card_powder_keg(int player, int card, event_t event)
{
  /* Powder Keg	|2
   * Artifact
   * At the beginning of your upkeep, you may put a fuse counter on ~.
   * |T, Sacrifice ~: Destroy each artifact and creature with converted mana cost equal to the number of fuse counters on ~. */

  if (trigger_condition == TRIGGER_UPKEEP && affect_me(player, card))
	{
	  int mode;
	  if (!IS_AI(player))
		mode = RESOLVE_TRIGGER_OPTIONAL;
	  else if (count_counters(player, card, COUNTER_FUSE) >= get_highest_cmc(1-player, TYPE_CREATURE|TYPE_ARTIFACT))
		mode = 0;
	  else
		mode = RESOLVE_TRIGGER_MANDATORY;

	  upkeep_trigger_ability_mode(player, card, event, player, mode);
	}

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	add_counter(player, card, COUNTER_FUSE);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE | TYPE_ARTIFACT, "");
	  test.cmc = count_counters(player, card, COUNTER_FUSE);
	  new_manipulate_all(player, card, ANYBODY, &test, KILL_DESTROY);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_ME, MANACOST_X(0), 0, NULL, NULL);
}

int card_quash(int player, int card, event_t event){

	/* Quash	|2|U|U
	 * Instant
	 * Counter target instant or sorcery spell. Search its controller's graveyard, hand, and library for all cards with the same name as that spell and exile
	 * them. Then that player shuffles his or her library. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_SPELL);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int id = get_id(instance->targets[0].player, instance->targets[0].card);
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			lobotomy_effect(player, instance->targets[0].player, id, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, &td, NULL, 1, NULL);
}

int card_decompose(int player, int card, event_t event);
int card_rapid_decay(int player, int card, event_t event)
{
  /* Rapid Decay	|1|B
   * Instant
   * Exile up to three target cards from a single graveyard.
   * Cycling |2 */

  if (card_decompose(player, card, event))
	return 1;

  return cycling(player, card, event, MANACOST_X(2));
}

int card_ravenous_rats(int player, int card, event_t event){
	/*
	  Ravenous Rats |1|B
	  Creature - Rat 1/1
	  When Ravenous Rats enters the battlefield, target opponent discards a card.
	*/
	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		if( would_validate_arbitrary_target(&td, 1-player, -1) && hand_count[1-player] ){
			discard(1-player, 0, player);
		}
		get_card_instance(player, card)->number_of_targets = 0;
	}

	return 0;
}


int card_rayne_academy_chancellor(int player, int card, event_t event)
{
  const target_t* targets = any_becomes_target(player, card, event, player, 1, TYPE_PERMANENT, -1, TYPE_EFFECT|TYPE_NONEFFECT, 1-player, RESOLVE_TRIGGER_AI(player));
  if (targets)
	{
	  int num = 0;
	  for (; targets->player != -1; ++targets)
		++num;

	  if (is_enchanted(player, card))
		num *= 2;

	  if (num == 1)
		draw_cards(player, 1);
	  else
		draw_up_to_n_cards(player, num);
	}

  return 0;
}

int card_reckless_abandon(int player, int card, event_t event){
	/* Reckless Abandon	|R
	 * Sorcery
	 * As an additional cost to cast ~, sacrifice a creature.
	 * ~ deals 4 damage to target creature or player. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_SAC_CREATURE_AS_COST,
						 &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_repercussion(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_DEAL_DAMAGE ){
			card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
			if( damage->internal_card_id == damage_card && damage->damage_target_card != -1){
				int dmg = damage->info_slot;
				if( dmg < 1 ){
					card_instance_t *target = get_card_instance(damage->damage_source_player, damage->damage_source_card);
					dmg = target->targets[16].player;
				}
				if( dmg > 0 ){
					if( instance->info_slot < 0 ){
						instance->info_slot = 0;
					}
					instance->targets[instance->info_slot].player = damage->damage_target_player;
					instance->targets[instance->info_slot].card = damage->info_slot;
					instance->info_slot++;
				}
			}
		}

		if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == affected_card_controller ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int i;
					for(i=0; i<instance->info_slot; i++){
						damage_player(instance->targets[i].player, instance->targets[i].card, player, card);
					}
					instance->info_slot = 0;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_reliquiary_monk(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);
		if( can_target(&td) && pick_target(&td, "TARGET_ARTIFACT_ENCHANTMENT")){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_replenish(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		default_test_definition(&test, TYPE_ENCHANTMENT);
		new_reanimate_all(player, card, player, &test, REANIMATE_DEFAULT);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_rofellos_gift(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		char msg[100] = "Select an enchantment card";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ENCHANTMENT, msg);

		char msg2[100] = "Select an enchantment card";
		test_definition_t this_test2;
		new_default_test_definition(&this_test2, TYPE_ANY, msg2);
		this_test2.color = COLOR_TEST_GREEN;

		int amount = reveal_cards_from_your_hand(player, card, &this_test2);
		while( amount > 0 ){
				new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
				amount--;
		}

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_rofellos_llanowar_emissary(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	int count = 0;
	if (event == EVENT_COUNT_MANA || event == EVENT_ACTIVATE || (event == EVENT_CAN_ACTIVATE && player == AI)){	// Only count if it'll actually be needed
		count = count_subtype(player, TYPE_LAND, SUBTYPE_FOREST);
	}

	return mana_producing_creature(player, card, event, 48, COLOR_GREEN, count);
}

int card_scour(int player, int card, event_t event){

	/* Scour	|2|W|W
	 * Instant
	 * Exile target enchantment. Search its controller's graveyard, hand, and library for all cards with the same name as that enchantment and exile them. Then
	 * that player shuffles his or her library. */

	if( ! IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int id = get_id(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			lobotomy_effect(player, instance->targets[0].player, id, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ENCHANTMENT", 1, NULL);
}

int card_scent_of_brine(int player, int card, event_t event){
	/* Scent of Brine	|1|U
	 * Instant
	 * Reveal any number of |Sblue cards in your hand. Counter target spell unless its controller pays |1 for each card revealed this way. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_BLUE));
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
		this_test.zone = TARGET_ZONE_HAND;
		int amount = reveal_cards_from_your_hand(player, card, &this_test);

		counterspell_resolve_unless_pay_x(player, card, NULL, 0, amount);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 1, NULL);
}

int card_scent_of_cinder(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_RED));
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_RED);
			this_test.zone = TARGET_ZONE_HAND;
			int amount = reveal_cards_from_your_hand(player, card, &this_test);
			damage_creature_or_player(player, card, event, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER" , 1, NULL);
}

int card_scent_of_ivy(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_GREEN));
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
			this_test.zone = TARGET_ZONE_HAND;
			int amount = reveal_cards_from_your_hand(player, card, &this_test);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, amount, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE" , 1, NULL);
}

int card_scent_of_jasmine(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		char buffer[100];
		scnprintf(buffer, 100, "Select a white card");
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_WHITE));
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		this_test.zone = TARGET_ZONE_HAND;
		int amount = reveal_cards_from_your_hand(player, card, &this_test);
		gain_life(player, amount*2);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_scent_of_nightshade(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, get_sleighted_color_text(player, card, "Select a %s card.", COLOR_BLACK));
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
			this_test.zone = TARGET_ZONE_HAND;
			int amount = reveal_cards_from_your_hand(player, card, &this_test);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -amount, -amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_sigil_of_sleep(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		if( attached_creature_deals_damage(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_TRACE_DAMAGED_PLAYERS) ){
			int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
			instance->targets[1].player = 0;
			int i, d;
			for(i=0; i<2; i++){
				int p = i == 0 ? player : 1-player;
				for(d=0; d<times_damaged[p]; d++){
					target_definition_t td;
					default_target_definition(player, card, &td, TYPE_CREATURE);
					td.allowed_controller = p;
					td.preferred_controller = p;
					td.allow_cancel = 0;
					if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						bounce_permanent(instance->targets[0].player, instance->targets[0].card);
					}
				}
			}
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

// Skittering horror code is the same for Skittering Skirge in urza_saga.c

int card_solidarity2(int player, int card, event_t event){
	/*
	  Solidarity |3|W
	  Instant
	  Creatures you control get +0/+5 until end of turn.
	*/
	if(event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 0, 5, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_soul_feast(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 4);
			gain_life(player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_sowing_salt(int player, int card, event_t event){

	/* Sowing Salt	|2|R|R
	 * Sorcery
	 * Exile target nonbasic land. Search its controller's graveyard, hand, and library for all cards with the same name as that land and exile them. Then that player shuffles his or her library. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_NONBASIC;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int id = get_id(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			lobotomy_effect(player, instance->targets[0].player, id, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonbasic land.", 1, NULL);
}

int card_splinter2(int player, int card, event_t event){

	/* Splinter	|2|G|G
	 * Sorcery
	 * Exile target artifact. Search its controller's graveyard, hand, and library for all cards with the same name as that artifact and exile them. Then that
	 * player shuffles his or her library. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int id = get_id(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			lobotomy_effect(player, instance->targets[0].player, id, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 1, NULL);
}

int card_taunting_elf(int player, int card, event_t event){

	return everybody_must_block_me(player, card, event);
}

int card_temporal_adept(int player, int card, event_t event){

	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		ai_defensive_modifier += 60;
	}

	if (event == EVENT_BLOCK_RATING && affect_me(player, card)){
		ai_defensive_modifier -= 60;
	}

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance(player, card);


	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_U(3), 0, &td, "TARGET_PERMANENT");
}

int card_tethered_griffin(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( count_subtype(player, TYPE_ENCHANTMENT, -1) < 1 ){
			ai_modifier-=1000;
		}
	}

	if( event == EVENT_STATIC_EFFECTS && ! is_humiliated(player, card) && count_subtype(player, TYPE_ENCHANTMENT, -1) < 1 ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_thieving_magpie(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_thorn_elemental(int player, int card, event_t event)
{
  /* Thorn Elemental	|5|G|G
   * Creature - Elemental 7/7
   * You may have ~ assign its combat damage as though it weren't blocked. */

  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage
	  && damage->damage_source_card == card && damage->damage_source_player == player
	  && damage->damage_target_card != -1
	  && get_card_instance(player, card)->targets[1].player == 1)
	{
	  damage->damage_target_player = 1-player;
	  damage->damage_target_card = -1;
	}

  if (event == EVENT_DECLARE_BLOCKERS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->targets[1].player = 0;
	  if ((instance->state & STATE_ATTACKING) && !is_unblocked(player, card))
		instance->targets[1].player = do_dialog(player, player, card, -1, -1, " Deal damage normally\n Ignore blocker", 1);
	}

  return 0;
}

int card_thran_dynamo(int player, int card, event_t event){

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			declare_mana_available(player, COLOR_COLORLESS, 3);
		}
	}

	if(event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	if(event == EVENT_ACTIVATE){
		produce_mana_tapped(player, card, COLOR_COLORLESS, 3);
	}

	return 0;
}

int card_thran_foundry(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			int *deck = deck_ptr[instance->targets[0].player];
			int count_d = count_deck(instance->targets[0].player);
			int count = count_graveyard(instance->targets[0].player)-1;
			while( count > -1 ){
					deck[count_d] = get_grave(instance->targets[0].player)[count];
					remove_card_from_grave(instance->targets[0].player, count);
					count_d++;
					count--;
			}
			shuffle(instance->targets[0].player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_RFG_ME | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, "TARGET_PLAYER");
}

int card_thran_golem(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_ABILITIES && affect_me(player, card) ){
			if( is_enchanted(player, card) ){
				event_result |= KEYWORD_FIRST_STRIKE;
				event_result |= KEYWORD_TRAMPLE;
				event_result |= KEYWORD_FLYING;
			}
		}
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			if( is_enchanted(player, card) ){
				event_result +=2;
			}
		}
	}

	return 0;
}

int card_treachery(int player, int card, event_t event){
	// Original code :0x4DB760

	if( comes_into_play(player, card, event) ){
		untap_lands(player, card, 5);
	}

	return card_control_magic(player, card, event);
}

int card_trumpet_blast(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_ATTACKING;
		pump_creatures_until_eot(player, card, current_turn, 0, 2, 0, 0, 0, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_twisted_experiment(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 3, -1, 0, 0, 0, 0, 0);
}

int card_urzas_incubator(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = select_a_subtype(player, card);
	}

	if( instance->info_slot > 0 && event == EVENT_MODIFY_COST_GLOBAL && has_subtype(affected_card_controller, affected_card, instance->info_slot) &&
		! is_humiliated(player, card)
	  ){
	   COST_COLORLESS-=2;
	}

	return 0;
}

int card_wake_of_destruction(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	return echoing_spell(player, card, event, &td, "TARGET_LAND", KILL_DESTROY);
}

int card_yavimaya_elder(int player, int card, event_t event){

	if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player))){
		tutor_basic_lands(player, TUTOR_HAND, 2);
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL);
}

int card_yavimaya_enchantress(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result +=count_subtype(2, TYPE_ENCHANTMENT, -1);
	}

	return 0;
}

int card_yavimaya_hollow(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_DESTROYED;
	td.preferred_controller = player;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_REGENERATION | GAA_CAN_TARGET, MANACOST_G(1), 0, &td, NULL);
		if( result ){
			return result;
		}
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = instance->number_of_targets = instance->info_slot = 0;
		if( ! paying_mana() &&
			generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_REGENERATION | GAA_CAN_TARGET, MANACOST_G(1), 0, &td, "TARGET_CREATURE")
		  ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Regenerate creature\n Cancel", 1);
		}

		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST_G(1)) && pick_target(&td, "TARGET_CREATURE") ){
					instance->info_slot = 1;
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
				regenerate_target( instance->targets[0].player, instance->targets[0].card );
			}
		}
	}

	return 0;
}

int card_yawgmoths_bargain(int player, int card, event_t event){

	skip_your_draw_step(player, event);

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if(event == EVENT_RESOLVE_ACTIVATION){
		draw_a_card(player);
	}
	return generic_activated_ability(player, card, event, 0, MANACOST0, 1, NULL, NULL);
}


