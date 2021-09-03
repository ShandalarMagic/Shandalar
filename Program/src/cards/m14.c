#include "manalink.h"

// General functions
static void make_angel(int player, int card, int qty){
	generate_tokens_by_id(player, card, CARD_ID_ANGEL, qty);
}

static int m14_staff(int player, int card, event_t event, int clr, int land_type){
	if( (trigger_condition == TRIGGER_SPELL_CAST &&
		 specific_spell_played(player, card, event, player, 2, 0, 0, 0, 0, get_sleighted_color_test(player, card, clr), 0, 0, 0, -1, 0)) ||
		(trigger_condition == TRIGGER_COMES_INTO_PLAY &&
		 specific_cip(player, card, event, player, 2, TYPE_LAND, 0, get_hacked_subtype(player, card, land_type), 0, 0, 0, 0, 0, -1, 0))
	  ){
		gain_life(player, 1);
	}
	return 0;
}

// Cards

// white
int card_ajanis_chosen(int player, int card, event_t event){

	/* Ajani's Chosen	|2|W|W
	 * Creature - Cat Soldier 3/3
	 * Whenever an enchantment enters the battlefield under your control, put a 2/2 |Swhite Cat creature token onto the battlefield. If that enchantment is an
	 * Aura, you may attach it to the token. */

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, player, 2, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_CAT, &token);
		token.pow = 2;
		token.tou = 2;
		token.keep_track_of_tokens_generated = 1;
		generate_token(&token);

		if (token.keep_track_of_tokens_generated > 0
			&& has_subtype(instance->targets[1].player, instance->targets[1].card, SUBTYPE_AURA_CREATURE)
			&& in_play(instance->targets[0].player, instance->targets[0].card)
			&& do_dialog(player, player, card, instance->targets[1].player, instance->targets[1].card, " Attach this Aura to the Cat\n Pass", 0) == 0){
				card_instance_t* aura = get_card_instance(instance->targets[1].player, instance->targets[1].card);
				aura->damage_target_player = instance->targets[0].player;
				aura->damage_target_card = instance->targets[0].card;
		}
	}

	return 0;
}

int card_angelic_accord(int player, int card, event_t event){
	/* Angelic Accord	|3|W
	 * Enchantment
	 * At the beginning of each end step, if you gained 4 or more life this turn, put a 4/4 |Swhite Angel creature token with flying onto the battlefield. */

	if( eot_trigger(player, card, event) && get_trap_condition(player, TRAP_LIFE_GAINED) > 3 ){
		make_angel(player, card, 1);
	}

	return global_enchantment(player, card, event);
}

// archangel of thune --> child of night

int card_banisher_priest(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	return_from_oblivion(player, card, event);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
			   obliviation(player, card, instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	if (event == EVENT_CAN_CAST){	// So it works with permanents with flash
		return 1;
	}
	return 0;
}

int card_bonescythe_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_DOUBLE_STRIKE, BCT_CONTROLLER_ONLY+BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_capashen_knight(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_XW(1, 1), 1, 1, 0, 0);
}

int card_celestial_flare(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
		if( valid_target(&td) && can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
			target_definition_t td1;
			default_target_definition(player, card, &td1, TYPE_CREATURE );
			td1.allowed_controller = instance->targets[0].player;
			td1.preferred_controller = instance->targets[0].player;
			td1.who_chooses = instance->targets[0].player;
			td1.required_state = TARGET_STATE_IN_COMBAT;
			td1.illegal_abilities = 0;
			if( can_target(&td1) && new_pick_target(&td1, "Select an attacking or blocking creature you control.", 1, GS_LITERAL_PROMPT) ){
				kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

// charging griffin --> benalish veteran

int card_dawnstrike_paladin(int player, int card, event_t event){

	vigilance(player, card, event);

	lifelink(player, card, event);

	return 0;
}

int card_devout_invocation(int player, int card, event_t event){
	/* Devout Invocation	|6|W
	 * Sorcery
	 * Tap any number of untapped creatures you control. Put a 4/4 |Swhite Angel creature token with flying onto the battlefield for each creature tapped this way. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_CREATURE );
		td1.allowed_controller = player;
		td1.preferred_controller = player;
		td1.illegal_abilities = 0;
		td1.illegal_state = TARGET_STATE_TAPPED;
		int tapped[100];
		int angels = 0;
		while( can_target(&td1) && new_pick_target(&td1, "Select a creature you control to tap.", 0, GS_LITERAL_PROMPT) ){
				instance->number_of_targets = 0;
				tapped[angels] = instance->targets[0].card;
				state_untargettable(player, tapped[angels], 1);
				angels++;
		}
		int i;
		for(i=0; i<angels; i++){
			tap_card(player, tapped[i]);
			state_untargettable(player, tapped[i], 0);
		}
		make_angel(player, card, angels);
		kill_card(player, card, KILL_DESTROY);
	}
	return basic_spell(player, card, event);
}

int card_fiendslayer_paladin(int player, int card, event_t event){
	lifelink(player, card, event);

	if( specific_spell_played(player, card, event, 1-player, 2, TYPE_SPELL, F1_NO_CREATURE, 0, 0, COLOR_TEST_BLACK | COLOR_TEST_RED, 0, 0, 0, -1, 0) ){
		card_instance_t* instance = get_card_instance(player, card);
		if (target_me(player, card, instance->targets[1].player, instance->targets[1].card)){
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
		}
	}
	return 0;
}

int card_hive_stirrings(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SLIVER, &token);
		token.qty = 2;
		token.pow = token.tou = 1;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}
	return basic_spell(player, card, event);
}

int card_imposing_sovereign(int player, int card, event_t event){

	/* Imposing Sovereign	|1|W
	 * Creature - Human 2/1
	 * Creatures your opponents control enter the battlefield tapped. */

	permanents_enters_battlefield_tapped(player, card, event, 1-player, TYPE_CREATURE, NULL);

	return 0;
}

int card_indestructibility(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		indestructible(instance->damage_target_player, instance->damage_target_card, event);
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	if( player == AI ){
		td.illegal_type = TYPE_LAND;
	}
	td.preferred_controller = player;

	return targeted_aura(player, card, event, &td, "TARGET_PERMANENT");
}

int card_master_of_diversion(int player, int card, event_t event)
{
  // Whenever ~ attacks, tap target creature defending player controls.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		tap_card(instance->targets[0].player, instance->targets[0].card);
	}

  return 0;
}

int card_path_of_bravery(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  // As long as your life total is greater than or equal to your starting life total, creatures you control get +1/+1.
  // This is badly incomplete.  Some progress toward fixing it in branch wip-challenge.
  if (event == EVENT_RESOLVE_SPELL)
	instance->targets[1].player = 20+(20*check_battlefield_for_id(player, CARD_ID_ELDER_DRAGON_HIGHLANDER));

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player
	  && life[player] >= instance->targets[1].player)
	++event_result;

  // Whenever one or more creatures you control attack, you gain life equal to the number of attacking creatures.
  int amt;
  if ((amt = declare_attackers_trigger(player, card, event, 0, player, -1)))
	gain_life(player, amt);

  return global_enchantment(player, card, event);
}

int card_sentinel_sliver(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES && affected_card_controller == player && ! is_humiliated(player, card)
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE) && has_subtype(affected_card_controller, affected_card, SUBTYPE_SLIVER))
	vigilance(affected_card_controller, affected_card, event);

  return slivercycling(player, card, event);
}

int card_seraph_of_the_sword(int player, int card, event_t event){

	card_instance_t* damage = combat_damage_being_prevented(event);
	if (damage && damage->damage_target_card == card && damage->damage_target_player == player){
		damage->info_slot = 0 ;
	}

	return 0;
}

int card_soulmender(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 1);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_steelform_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 1, 0, BCT_INCLUDE_SELF+BCT_CONTROLLER_ONLY);
	return slivercycling(player, card, event);
}

int card_stonehorn_chanter(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_XW(5, 1), 0, 0, 0, SP_KEYWORD_LIFELINK+SP_KEYWORD_VIGILANCE);
}

// blue

static int colossal_whale_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( leaves_play(instance->targets[0].player, instance->targets[0].card, event) ){
		if( check_rfg(instance->targets[1].player, cards_data[instance->targets[1].card].id) ){
			int card_added = add_card_to_hand(instance->targets[1].player, instance->targets[1].card);
			remove_card_from_rfg(instance->targets[1].player, cards_data[instance->targets[1].card].id);
			put_into_play(instance->targets[1].player, card_added);
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_colossal_whale(int player, int card, event_t event){

	// Whenever ~ attacks, you may exile target creature defending player controls until ~ leaves the battlefield.
	if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		card_instance_t* instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS")){
			if( ! is_token(instance->targets[0].player, instance->targets[0].card) ){
				int legacy = create_legacy_effect(player, card, &colossal_whale_legacy);
				card_instance_t *leg = get_card_instance(player, legacy);
				leg->targets[0].player = player;
				leg->targets[0].card = card;
				leg->number_of_targets = 1;
				leg->targets[1].player = get_owner(instance->targets[0].player, instance->targets[0].card);
				leg->targets[1].card = get_original_internal_card_id(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
		}
	}

	return 0;
}

int card_dismiss_into_dream(int player, int card, event_t event)
{
  /*
	Dismiss into Dream |6|U
	Enchantment
	Each creature your opponents control is an Illusion in addition to its other types and has
	"When this creature becomes the target of a spell or ability, sacrifice it."
  */
  if (event == EVENT_RESOLVE_SPELL)
	{
	  int c;
	  for (c = active_cards_count[1-player] - 1; c >= 0; --c)
		if (in_play(1-player, c) && is_what(1-player, c, TYPE_CREATURE))
		  add_a_subtype(1-player, c, SUBTYPE_ILLUSION);
	}

  if (event == EVENT_CAST_SPELL)
	{
	  if (affected_card_controller == 1-player && is_what(affected_card_controller, affected_card, TYPE_CREATURE))
		add_a_subtype(affected_card_controller, affected_card, SUBTYPE_ILLUSION);
	}

  if (leaves_play(player, card, event))
	{
	  int c;
	  for (c = active_cards_count[1-player] - 1; c >= 0; --c)
		if (in_play(1-player, c) && is_what(1-player, c, TYPE_CREATURE))
		  reset_subtypes(1-player, c, 2);
	}

  const target_t* targets = any_creature_becomes_target_of_spell_or_effect(player, card, event, 1-player);
  if (targets)
	for (; targets->player != -1; ++targets)
	  kill_card(targets->player, targets->card, KILL_SACRIFICE);

  return global_enchantment(player, card, event);
}

int card_divination(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_elite_arcanist(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_INSTANT, "Select an instant card.");
		this_test.zone = TARGET_ZONE_HAND;

		if( check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			int result = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_VALUE, -1, &this_test);
			if( result > -1 ){
				instance->targets[1].card = get_original_internal_card_id(player, result);
				kill_card(player, result, KILL_REMOVE);
				create_card_name_legacy(player, card, cards_data[instance->targets[1].card].id);
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( instance->targets[1].card > -1 ){
			int result = can_legally_play_csvid(player, instance->targets[1].card);
			if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(get_cmc_by_internal_id(instance->targets[1].card)), 0, NULL, NULL) ){
				return result;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_X(get_cmc_by_internal_id(instance->targets[1].card))) ){
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		copy_spell(player, cards_data[instance->targets[1].card].id);
	}

	return 0;
}

int card_galerider_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_FLYING, BCT_CONTROLLER_ONLY+BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_glimpse_the_future(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		select_one_and_mill_the_rest(player, player, 3, TYPE_ANY);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_illusionary_armor(int player, int card, event_t event)
{
  if (kill_attachment_when_creature_is_targeted(player, card, event, KILL_SACRIFICE))
	return 0;

  return generic_aura(player, card, event, player, 4, 4, 0, 0, 0, 0, 0);
}

int card_jaces_mindseeker(int player, int card, event_t event)
{
  /* Jace's Mindseeker	|4|U|U
   * Creature - Fish Illusion 4/4
   * Flying
   * When ~ enters the battlefield, target opponent puts the top five cards of his or her library into his or her graveyard. You may cast an instant or sorcery
   * card from among them without paying its mana cost. */

  if (comes_into_play(player, card, event) && target_opponent(player, card))
	special_mill(player, card, CARD_ID_JACES_MINDSEEKER, 1-player, 5);

  return 0;
}

// messenger drake --> darkslick drake

// quicken --> skipped (impossible)

// seacost drake --> vanilla

static const char* nice_to_tap(int unused, int player, int card){
	if( get_cmc(player, card) > 3)
		return NULL;
	if( get_power(player, card) > 2)
		return NULL;
	return "unused, AI only";
}

int card_tidebinder_mage(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_color = COLOR_TEST_GREEN | COLOR_TEST_RED;
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;

		// A bit of AI helping
		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.required_color = COLOR_TEST_GREEN | COLOR_TEST_RED;
		td2.allowed_controller = 1-player;
		td2.allow_cancel = 0;
		td2.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td2.extra = (int32_t)nice_to_tap;

		if( can_target(&td) ){
			if( player == AI && can_target(&td2) ){
				pick_target(&td2, "TARGET_CREATURE");
			}
			else{
				pick_target(&td, "TARGET_CREATURE");
			}
			tap_card(instance->targets[0].player, instance->targets[0].card);
			int legacy = create_targetted_legacy_effect(player, card, &dungeon_geist_effect, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].player = player;
			leg->targets[1].card = card;
			leg->number_of_targets = 2;
		}
	}

	return 0;
}

int card_time_ebb(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td1, "TARGET_CREATURE", 1, NULL);
}

int card_trained_condor(int player, int card, event_t event)
{
  // Not quite the same as Chasm Drake, annoyingly enough.

  // Whenever ~ attacks, another target creature you control gains flying until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.allow_cancel = 0;
	  td.special = TARGET_SPECIAL_NOT_ME;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;

	  if (can_target(&td))
		{
		  if (player == AI && !(td.illegal_abilities & KEYWORD_FLYING))
			{
			  td.illegal_abilities |= KEYWORD_FLYING;
			  if (!can_target(&td))
				td.illegal_abilities &= ~KEYWORD_FLYING;
			}

		  if (pick_target(&td, "TARGET_ANOTHER_CREATURE_CONTROL"))
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
		}
	}

  return 0;
}

int card_warden_of_evos_isle(int player, int card, event_t event){

	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player && ! is_humiliated(player, card) ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			card_instance_t *instance = get_card_instance( affected_card_controller, affected_card );
			if( cards_data[instance->internal_card_id].static_ability & KEYWORD_FLYING ){
				COST_COLORLESS--;
			}
		}
	}

	return 0;
}

int card_windreader_sphinx(int player, int card, event_t event)
{
  // Whenever a creature with flying attacks, you may draw a card.
  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.keyword = KEYWORD_FLYING;

	  if (declare_attackers_trigger_test(player, card, event, RESOLVE_TRIGGER_AI(player) | DAT_SEPARATE_TRIGGERS, 2, -1, &test))
		draw_a_card(player);
	}

  return 0;
}

int card_zephyr_charge(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_FLYING, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XU(1, 1), 0, &td, "TARGET_CREATURE");
}

//Black

// accursed spirit --> bladetusk boar

int card_arteficers_hex(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			card_instance_t *eq = get_card_instance(instance->damage_target_player, instance->damage_target_card);
			kill_card(eq->targets[8].player, eq->targets[8].card, KILL_DESTROY);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.required_subtype = SUBTYPE_EQUIPMENT;

	return targeted_aura(player, card, event, &td, "TARGET_EQUIPMENT");
}

int card_blightcaster(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, 2, TYPE_ENCHANTMENT, F1_NO_PWALKER, 0, 0, 0, 0, 0, 0, -1, 0) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -2, -2);
			}
		}
	}

	return 0;
}

int card_blood_bairn(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_NOT_ME_AS_TARGET, MANACOST0, 0, NULL, NULL);
}

int card_bogbrew_witch(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		if( player == HUMAN ){
			new_default_test_definition(&this_test, TYPE_ANY, "Select a Newt or Cauldron card.");
			this_test.id = CARD_ID_FESTERING_NEWT;
			this_test.id2 = CARD_ID_BUBBLING_CAULDRON;
			this_test.id_flag = F4_DOUBLE_ID_TUTOR;
		}
		else{
			default_test_definition(&this_test, TYPE_ANY);
			int mode = 1*check_battlefield_for_id(player, CARD_ID_FESTERING_NEWT) + 2*check_battlefield_for_id(player, CARD_ID_BUBBLING_CAULDRON);
			if( mode == 3 || mode == 2 ){
				this_test.id = CARD_ID_FESTERING_NEWT;
			}
			else{
				this_test.id = CARD_ID_BUBBLING_CAULDRON;
			}
		}
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
}

int card_corpse_hauler(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_XB(2, 1), 0, NULL, NULL) ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(player) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_XB(2, 1)) ){
			char msg[100] = "Select a creature card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
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
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
	}

	return 0;
}

int card_dark_prophecy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		draw_cards(player, instance->targets[11].card);
		lose_life(player, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

// deathgaze gorgon --> deadly recluse

int card_festering_newt(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				int minus = -1;
				if( check_battlefield_for_id(player, CARD_ID_BOGBREW_WITCH) ){
					minus = -4;
				}
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, minus, minus);
			}
		}
	}

	return 0;
}

int card_gnawing_zombie(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 1);
			gain_life(player, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_CAN_TARGET, MANACOST_XB(1, 1), 0, &td, "TARGET_PLAYER");
}

int card_grim_return(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		// Approximation
		if( ! graveyard_has_shroud(HUMAN) && has_type_dead_this_turn_in_grave(HUMAN, &this_test) ){
			return 1;
		}
		if( ! graveyard_has_shroud(AI) && has_type_dead_this_turn_in_grave(AI, &this_test) ){
			return 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( choose_a_dead_this_turn(player, card, 2, 0, AI_MAX_CMC, &this_test, 0) == -1 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
		int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_lifebane_zombie(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	intimidate(player, card, event);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;

		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		if( valid_target(&td) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE,
										get_sleighted_color_text2(player, card, "Select a %s or %s creature card.", COLOR_GREEN, COLOR_WHITE));
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN | COLOR_TEST_WHITE);

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			this_definition.effect = EC_RFG;

			new_effect_coercion(&this_definition, &this_test);
		}
	}

	return 0;
}

int card_lilianas_reaver(int player, int card, event_t event){

	deathtouch(player, card, event);

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		discard(1-player, 0, player);

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		token.action = TOKEN_ACTION_TAPPED;
		generate_token(&token);
	}

	return 0;
}

int card_liturgy_of_blood(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			produce_mana(player, COLOR_BLACK, 3);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

// minotaur abomination --> vanilla

int card_rise_of_the_dark_realms(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		APNAP(p, {new_reanimate_all(player, card, p, &this_test, REANIMATE_ALL_UNDER_CASTERS_CONTROL);};);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_shadowborn_apostle(int player, int card, event_t event){

	if (event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL) ){
			return can_sacrifice_as_cost(player, 6, TYPE_CREATURE, MATCH, 0, 0, 0, 0, CARD_ID_SHADOWBORN_APOSTLE, MATCH, -1, 0);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if (charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			int sacced[6];
			int sc = 0;
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ANY, "Select a Shadowborn Apostle to sacrifice.");
			test.id = CARD_ID_SHADOWBORN_APOSTLE;
			while( sc < 6 ){
					int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
					if (!sac){
						break;
					}
					sacced[sc] = BYTE3(sac);
					sc++;
			}
			int i;
			for(i=0; i<sc; i++){
				if( sc == 6 ){
					kill_card(player, sacced[i], KILL_SACRIFICE);
				}
				else{
					state_untargettable(player, sacced[i], 1);
				}
			}
			if( sc != 6 ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test2;
		new_default_test_definition(&this_test2, TYPE_CREATURE, "Select a Demon creature card.");
		this_test2.subtype = SUBTYPE_DEMON;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test2);
	}

	return 0;
}

int card_shadowborn_demon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_subtype = SUBTYPE_DEMON;
		td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
		td.allow_cancel = 0;

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( new_pick_target(&td, "Select target non-Demon creature.", 0, GS_LITERAL_PROMPT)){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		instance->number_of_targets = 0;
		if( count_graveyard_by_type(player, TYPE_CREATURE) < 6 ){
			impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	return 0;
}

int card_syphon_sliver(int player, int card, event_t event)
{
  /* Syphon Sliver	|2|B
   * Creature - Sliver 2/2
   * Sliver creatures you control have lifelink. */
  boost_subtype(player, card, event, SUBTYPE_SLIVER, 0,0, 0,SP_KEYWORD_LIFELINK, BCT_INCLUDE_SELF | BCT_CONTROLLER_ONLY);

  return slivercycling(player, card, event);
}

static int tenacious_dead_death_effect(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(p, c) ){
			card_instance_t *affected = get_card_instance(p, c);
			if( affected->kill_code > 0 && affected->kill_code < 4){
				instance->targets[11].player = 66;
				instance->targets[1].player = get_owner(p, c);
				instance->targets[1].card = cards_data[get_original_internal_card_id(p, c)].id;
				remove_status(player, card, STATUS_INVISIBLE_FX);
			}
		}
	}

	if( has_mana_multi(player, MANACOST_XB(1, 1)) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		charge_mana_multi(player, MANACOST_XB(1, 1));
		if( spell_fizzled != 1){
			seek_grave_for_id_to_reanimate(player, -1, instance->targets[1].player, instance->targets[1].card, REANIMATE_TAP | REANIMATE_UNDER_OWNER_CONTROL	);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	if( ! in_play(p, c) && event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_tenacious_dead(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int legacy = create_my_legacy(player, card, &tenacious_dead_death_effect);
		add_status(player, legacy, STATUS_INVISIBLE_FX);
	}

	return 0;
}

// undead minotaur --> vanilla

int card_vampire_warlord(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && can_regenerate(player, instance->parent_card) ){
		regenerate_target(player, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_REGENERATION|GAA_SACRIFICE_CREATURE|GAA_NOT_ME_AS_TARGET, MANACOST0, 0, NULL, NULL);
}

int card_xathrid_necromancer(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_HUMAN;
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, player, 0, &this_test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		token.qty = instance->targets[11].card;
		token.action = TOKEN_ACTION_TAPPED;
		generate_token(&token);
		instance->targets[11].card = 0;
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		token.action = TOKEN_ACTION_TAPPED;
		generate_token(&token);
	}

	return 0;
}

// red

int card_academy_rider(int player, int card, event_t event){

	intimidate(player, card, event);

	if( hand_count[player] > 0 && damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		int choice = do_dialog(player, player, card, -1, -1, " Discard & draw\n Pass", 0);
		if( choice == 0 ){
			discard(player, 0, player);
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_awaken_the_ancient(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		add_a_subtype(instance->damage_target_player, instance->damage_target_card, SUBTYPE_GIANT);
	}

	if( leaves_play(player, card, event) ){
		reset_subtypes(instance->damage_target_player, instance->damage_target_card, 2);
	}

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_SET_COLOR && affect_me(p, c) ){
			event_result |= get_sleighted_color_test(player, card, COLOR_TEST_RED);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, LAND);
	td.preferred_controller = player;
	td.required_subtype = get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN);

	return generic_animating_aura(player, card, event, &td, get_hacked_land_text(player, card, "Select target %s.", SUBTYPE_MOUNTAIN), 7, 7, 0, SP_KEYWORD_HASTE);
}

int card_barrage_of_expendables(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_battle_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 2, 0, 0, BCT_CONTROLLER_ONLY+BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_blur_sliver(int player, int card, event_t event){
	// Sliver creatures you control have haste.
	boost_subtype(player, card, event, SUBTYPE_SLIVER, 0,0, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

	return slivercycling(player, card, event);
}

int card_burning_earth(int player, int card, event_t event){
	if( event == EVENT_TAP_CARD && tapped_for_mana_color >= 0 && ! is_humiliated(player, card)
		&& is_what(affected_card_controller, affected_card, TYPE_LAND) && ! is_basic_land(affected_card_controller, affected_card) ){
		damage_player(affected_card_controller, 1, player, card);
	}
	return global_enchantment(player, card, event);
}

int card_chandra_pyromaster(int player, int card, event_t event){

	/* Chandra, Pyromaster	|2|R|R
	 * Planeswalker - Chandra (4)
	 * +1: ~ deals 1 damage to target player and 1 damage to up to one target creature that player controls. That creature can't block this turn.
	 * 0: Exile the top card of your library. You may play it this turn.
	 * -7: Exile the top ten cards of your library. Choose an instant or sorcery card exiled this way and copy it three times.
	 *     You may cast the copies without paying their mana costs. */

	if (IS_ACTIVATING(event)){

		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		if( instance->targets[0].player > -1 && instance->number_of_targets > 0 ){
			td2.allowed_controller = instance->targets[0].player;
			td2.preferred_controller = instance->targets[0].player;
		}

		enum{
			CHOICE_PING_PLAYER = 1,
			CHOICE_REMOVE,
			CHOICE_COPY3
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Ping player & creature", can_target(&td), 10, 1,
						"Exile top card", 1, 8, 0,
						"Exile 10 & copy 3 times", 1, 15, -7);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
			instance->number_of_targets = 0;
			switch (choice)
			{
				case CHOICE_PING_PLAYER :
				{
					if( pick_target(&td, "TARGET_PLAYER") ){
						if( can_target(&td2) ){
							new_pick_target(&td2, "Select a creature controller by the targetted player.", 1, GS_LITERAL_PROMPT);
						}
					}
					break;
				}

				case CHOICE_REMOVE:
				case CHOICE_COPY3:
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		{
			case CHOICE_PING_PLAYER:
			{
				if( valid_target(&td)){
					damage_player(instance->targets[0].player, 1, instance->parent_controller, instance->parent_card);
					if( validate_target(player, card, &td2, 1)){
						pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[1].player, instance->targets[1].card,
												0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
						damage_creature(instance->targets[1].player, instance->targets[1].card, 1, instance->parent_controller, instance->parent_card);
					}
				}
			}
			break;

			case CHOICE_REMOVE:
			{
				int csvid = cards_data[deck_ptr[player][0]].id;
				rfg_top_card_of_deck(player);
				create_may_play_card_from_exile_effect(instance->parent_controller, instance->parent_card, player, csvid, MPCFE_UNTIL_EOT);
			}
			break;

			case CHOICE_COPY3:
			{
				int count = MIN(10, count_deck(player));
				if( count ){
					int *deck = deck_ptr[player];
					show_deck( player, deck, count, "Chandra will exile these cards.", 0, 0x7375B0 );
					int reveal_array[count];
					int rac = 0;
					int i;
					for(i=0; i<count; i++){
						if( is_what(-1, deck[0], TYPE_SPELL) && can_legally_play_iid(player, deck[0]) ){
							reveal_array[rac] = deck[0];
							rac++;
						}
						rfg_top_card_of_deck(player);
					}
					if( rac ){
						char msg[100] = "Select an instant or sorcery card.";
						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_SPELL, msg);
						this_test.type_flag = F1_NO_CREATURE;
						int selected = select_card_from_zone(player, player, reveal_array, rac, 0, AI_MAX_CMC, -1, &this_test);
						if( selected != -1 ){
							for(i=0; i<3; i++){
								copy_spell(player, cards_data[reveal_array[selected]].id);
							}
						}
					}
				}
			}
			break;
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_cyclops_tyrant(int player, int card, event_t event){
	intimidate(player, card, event);
	if( event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( attacking_card_controller != -1 && get_power(attacking_card_controller, attacking_card) < 3 ){
			event_result = 1;
		}
	}
	return 0;
}

int card_dragon_egg(int player, int card, event_t event){
	/* Dragon Egg	|2|R
	 * Creature - Dragon 0/2
	 * Defender
	 * When ~ dies, put a 2/2 |Sred Dragon creature token with flying onto the battlefield. It has "|R: This creature gets +1/+0 until end of turn." */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_DRAGON, &token);
		token.pow = token.tou = 2;
		token.special_infos = 67;
		generate_token(&token);
	}
	return 0;
}

int card_fleshpulper_giant(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.toughness_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
			   kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

int card_goblin_diplomats(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		pump_creatures_until_eot(instance->parent_controller, instance->parent_card, current_turn, 0, 0, 0, 0, SP_KEYWORD_MUST_ATTACK, &this_test);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_marauding_maulhorn(int player, int card, event_t event){
	if (event == EVENT_MUST_ATTACK && current_turn == player && ! is_humiliated(player, card) &&
		!check_battlefield_for_id(player, CARD_ID_ADVOCATE_OF_THE_BEAST)
	  ){
		attack_if_able(player, card, event);
	}
	return 0;
}

int card_lightning_talons(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 3, 0, KEYWORD_FIRST_STRIKE, 0, 0, 0, 0);
}

int card_mindsparker(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, TYPE_SPELL, F1_NO_CREATURE, 0, 0,
								get_sleighted_color_test(player, card, COLOR_TEST_BLUE) | get_sleighted_color_test(player, card, COLOR_TEST_WHITE), 0,
								0, 0, -1, 0)
	  ){
		damage_player(instance->targets[1].player, 2, player, card);
	}

	return 0;
}

int card_molten_birth(int player, int card, event_t event){
	/* Molten Birth	|1|R|R
	 * Sorcery
	 * Put two 1/1 |Sred Elemental creature tokens onto the battlefield. Then flip a coin. If you win the flip, return ~ to its owner's hand. */

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.qty = 2;
		token.pow = token.tou = 1;
		token.color_forced = COLOR_TEST_RED;
		generate_token(&token);
		if( flip_a_coin(player, card) ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return basic_spell(player, card, event);
}

int card_ogre_battledriver(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;

		card_instance_t *instance = get_card_instance(player, card);

		if( new_specific_cip(player, card, event, player, 2, &this_test) ){
			pump_ability_until_eot(player, card, instance->targets[1].player, instance->targets[1].card, 2, 0, 0, SP_KEYWORD_HASTE);
		}
	}

	return 0;
}

// Regathan Firecat --> VANILLA

int card_scourge_of_valkas(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		card_instance_t *instance = get_card_instance(player, card);

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_DRAGON;

		instance->number_of_targets = 0;

		if( new_specific_cip(player, card, event, player, 2, &this_test) ){
			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
				instance->number_of_targets = 1;
				damage_creature_or_player(player, card, event, count_subtype(player, TYPE_PERMANENT, SUBTYPE_DRAGON));
			}
		}
	}

	return generic_shade(player, card, event, 0, MANACOST_R(1), 1, 0, 0, 0);
}

// seismic stomp --> falter

int card_striking_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_FIRST_STRIKE, BCT_CONTROLLER_ONLY+BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_thorncaster_sliver(int player, int card, event_t event)
{
  // Sliver creatures you control have "Whenever this creature attacks, it deals 1 damage to target creature or player."
  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_SLIVER;

	  int amt;
	  if ((amt = declare_attackers_trigger_test(player, card, event, DAT_TRACK, 2, -1, &test)))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		  for (--amt; amt >= 0; --amt)
			{
			  target_definition_t td;
			  default_target_definition(current_turn, attackers[amt], &td, TYPE_CREATURE);
			  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
			  td.allow_cancel = 0;

			  card_instance_t* sliver = get_card_instance(current_turn, attackers[amt]);
			  sliver->number_of_targets = 0;
			  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
				damage_creature(sliver->targets[0].player, sliver->targets[0].card, 1, current_turn, attackers[amt]);
			}
		}
	}

  return slivercycling(player, card, event);
}

int card_young_pyromancer(int player, int card, event_t event){
	/* Young Pyromancer	|1|R
	 * Creature - Human Shaman 2/1
	 * Whenever you cast an instant or sorcery spell, put a 1/1 |Sred Elemental creature token onto the battlefield. */

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_SPELL, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.pow = token.tou = 1;
		token.color_forced = COLOR_TEST_RED;
		generate_token(&token);
	}

	return 0;
}

// green
int card_advocate_of_the_beast(int player, int card, event_t event){

	if(trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player && current_turn == player &&
		! is_humiliated(player, card)
	  ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.required_subtype = SUBTYPE_BEAST;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( pick_target(&td, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
					}
			}
		}
	}

	return 0;
}

int card_elvish_mystic(int player, int card, event_t event){
	//0x4c9900; also code for Llanowar Elves, Fyndhorn Elves, Leaf Gilder, Orochi Sustainer, Vine Trellis
	return mana_producing_creature(player, card, event, 24, COLOR_GREEN, 1);
}

int card_enlarge(int player, int card, event_t event){
	/* Enlarge	|3|G|G
	 * Sorcery
	 * Target creature gets +7/+7 and gains trample until end of turn. It must be blocked this turn if able. */
	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	return vanilla_pump(player, card, event, &td, 7, 7, KEYWORD_TRAMPLE, SP_KEYWORD_MUST_BE_BLOCKED);
}

int card_garruk_caller_emblem(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
	}
	return 0;
}

int card_garruk_caller_of_beasts(int player, int card, event_t event){

	/* Garruk, Caller of Beasts	|4|G|G
	 * Planeswalker - Garruk (4)
	 * +1: Reveal the top five cards of your library. Put all creature cards revealed this way into your hand and the rest on the bottom of your library in any
	 * order.
	 * -3: You may put a |Sgreen creature card from your hand onto the battlefield.
	 * -7: You get an emblem with "Whenever you cast a creature spell, you may search your library for a creature card, put it onto the battlefield, then
	 * shuffle your library." */

	if (IS_ACTIVATING(event)){

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a green creature card.");
		this_test.color = COLOR_TEST_GREEN;
		this_test.zone = TARGET_ZONE_HAND;

		int priority_green_crit = 0;
		if( event == EVENT_ACTIVATE ){
			this_test.cmc = 4;
			this_test.cmc_flag	= F5_CMC_GREATER_THAN_VALUE;
			priority_green_crit = check_battlefield_for_special_card(player, card, player, 0, &this_test) ? 15 : 0;
			priority_green_crit += ((count_counters(player, card, COUNTER_LOYALTY)*5)-15);
			this_test.cmc = -1;
			this_test.cmc_flag	= 0;
		}

		enum{
			CHOICE_REVEAL5 = 1,
			CHOICE_GREEN_CRIT,
			CHOICE_EMBLEM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Look at the top 5", 1, 10, 1,
						"Put into play a green creature", 1, priority_green_crit, -3,
						"Emblem", 1, 20, -7);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		{
			case CHOICE_REVEAL5:
				lead_the_stampede_effect(player, CARD_ID_GARRUK_CALLER_OF_BEASTS, 5);
				break;

			case CHOICE_GREEN_CRIT:
				new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
				break;

			case CHOICE_EMBLEM:
				generate_token_by_id(player, card, CARD_ID_GARRUKS_EMBLEM);
				break;
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_groundshaker_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 0, 0, KEYWORD_TRAMPLE, BCT_CONTROLLER_ONLY+BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_hunt_the_weak(int player, int card, event_t event){
	/* Hunt the Weak	|3|G
	 * Sorcery
	 * Put a +1/+1 counter on target creature you control. Then that creature fights target creature you don't control. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td1, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( new_pick_target(&td, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
			new_pick_target(&td1, "Select target creature your opponent controls.", 1, 1 | GS_LITERAL_PROMPT);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 1) ){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			if( validate_target(player, card, &td1, 0) ){
				fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_into_the_wilds(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			if( player != AI ){
				show_deck( player, deck, 1, "Card revealed by Into the Wilds", 0, 0x7375B0 );
			}
			if( is_what(-1, deck[0], TYPE_LAND) ){
				put_into_play_a_card_from_deck(player, player, 0);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_kalonian_hydra(int player, int card, event_t event){

	// ~ enters the battlefield with four +1/+1 counters on it.
	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 4);

	// Whenever ~ attacks, double the number of +1/+1 counters on each creature you control.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					add_1_1_counters(player, count, count_1_1_counters(player, count));
				}
				count--;
		}
	}

	return 0;
}

// kaloniant tusker --> vanilla

int card_manaweft_sliver(int player, int card, event_t event){

	int result = permanents_you_control_can_tap_for_mana_all_one_color(player, card, event, TYPE_CREATURE, SUBTYPE_SLIVER, COLOR_TEST_ANY_COLORED, 1);
	if (event == EVENT_CAN_ACTIVATE){
		return result;
	}

	return slivercycling(player, card, event);
}

int card_megantic_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 3, 3, 0, BCT_CONTROLLER_ONLY+BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_oath_of_the_ancient_wood(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && new_specific_cip(player, card, event, player, 2, &this_test) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				instance->number_of_targets = 1;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_predatory_sliver(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_SLIVER, 1, 1, 0, BCT_CONTROLLER_ONLY+BCT_INCLUDE_SELF);
	return slivercycling(player, card, event);
}

int card_primeval_bounty(int player, int card, event_t event){
	/* Primeval Bounty	|5|G
	 * Enchantment
	 * Whenever you cast a creature spell, put a 3/3 |Sgreen Beast creature token onto the battlefield.
	 * Whenever you cast a noncreature spell, put three +1/+1 counters on target creature you control.
	 * Whenever a land enters the battlefield under your control, you gain 3 life. */

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller ){
		if( player == trigger_cause_controller ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.allow_cancel = 0;

			instance->number_of_targets = 0;

			int trig = 1;
			if( is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) || is_what(trigger_cause_controller, trigger_cause, TYPE_EFFECT ) ){
				trig = 0;
			}
			if( trig == 1 && ! is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) ){
				if( can_target(&td) ){
					trig = 2;
				}
				else{
					trig = 0;
				}
			}

			if( trig > 0 ){
				if(event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				else if(event == EVENT_RESOLVE_TRIGGER){
						if( trig == 1 ){
							token_generation_t token;
							default_token_definition(player, card, CARD_ID_BEAST, &token);
							token.pow = token.tou = 3;
							token.color_forced = COLOR_TEST_GREEN;
							generate_token(&token);
						}
						if( trig == 2 ){
							if( pick_target(&td, "TARGET_CREATURE") ){
								add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 3);
							}
						}
				}
			}
		}
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &this_test) ){
			gain_life(player, 3);
		}
	}

	return global_enchantment(player, card, event);
}

// rootwalla --> basking rootwalla

// rumbling baloth --> vanilla

static int savage_summonings_effect(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && ! affect_me(player, card) && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			state_untargettable(affected_card_controller, affected_card, 1);
		}
	}
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == affected_card_controller
	  ){
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) && check_state(trigger_cause_controller, trigger_cause, STATE_CANNOT_TARGET) ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					state_untargettable(trigger_cause_controller, trigger_cause, 0);
					add_1_1_counter(trigger_cause_controller, trigger_cause);
					kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_savage_summoning(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( current_phase < PHASE_DECLARE_ATTACKERS ){
			ai_modifier+=25;
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &savage_summonings_effect);
		kill_card(player, card, KILL_DESTROY);
	}

	cannot_be_countered(player, card, event);

	return basic_spell(player, card, event);
}

int card_sporemound(int player, int card, event_t event){
	/* Sporemound	|3|G|G
	 * Creature - Fungus 3/3
	 * Whenever a land enters the battlefield under your control, put a 1/1 |Sgreen Saproling creature token onto the battlefield. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		if( new_specific_cip(player, card, event, player, 2, &this_test) ){
			generate_token_by_id(player, card, CARD_ID_SAPROLING);
		}
	}

	return 0;
}

int card_vastwood_hydra(int player, int card, event_t event, int intitial_counters){

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		int amount = count_1_1_counters(player, card);
		while( amount > 0 && can_target(&td) ){
				if( select_target(player, card, &td, "Select target creature", &(instance->targets[0])) ){
					add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
					instance->number_of_targets = 1;
					amount--;
				}
				else{
					break;
				}
		}
	}

	return card_ivy_elemental(player, card, event);
}

int card_voracious_wurm(int player, int card, event_t event, int intitial_counters){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, get_trap_condition(player, TRAP_LIFE_GAINED));

	return 0;
}

int card_witchstalker(int player, int card, event_t event){

	hexproof(player, card, event);

	if( current_turn == player && specific_spell_played(player, card, event, 1-player, 2, TYPE_SPELL, F1_NO_CREATURE, 0, 0,
														get_sleighted_color_test(player, card, COLOR_TEST_BLUE) |
														get_sleighted_color_test(player, card, COLOR_TEST_BLACK),
														0, 0, 0, -1, 0)
	  ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_woodborn_behemoth(int player, int card, event_t event){

	if( is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			if( count_subtype(player, TYPE_LAND, -1) > 7 ){
				event_result+=4;
			}
		}

		if( event == EVENT_ABILITIES && affect_me(player, card) ){
			if( count_subtype(player, TYPE_LAND, -1) > 7 ){
				event_result |= KEYWORD_TRAMPLE;
			}
		}
	}

	return 0;
}

// artifacts
int card_bubbling_cauldron(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_CREATURE | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		if(charge_mana_for_activated_ability(player, card, MANACOST_X(1)) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
			if( player == AI && check_battlefield_for_id(player, CARD_ID_FESTERING_NEWT) ){
				test.id = CARD_ID_FESTERING_NEWT;
			}
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac){
				cancel = 1;
				return 0;
			}
			instance->info_slot = get_id(BYTE2(sac), BYTE3(sac)) == CARD_ID_FESTERING_NEWT ? 67 : 66;
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			gain_life(player, 4);
		}
		if( instance->info_slot == 67 ){
			int result = lose_life(1-player, 4);
			gain_life(player, result);
		}
	}

	return 0;
}

static int effect_lose_defender_gain_trample(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (event == EVENT_ABILITIES && affect_me(instance->damage_target_player, instance->damage_target_card))
	{
	  event_result |= KEYWORD_TRAMPLE;
	  event_result &= ~KEYWORD_DEFENDER;
	}

  return 0;
}

int card_guardian_of_the_ages(int player, int card, event_t event, int intitial_counters)
{
  // When a creature attacks you or a planeswalker you control, if ~ has defender, it loses defender and gains trample.
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && check_for_ability(player, card, KEYWORD_DEFENDER)
	  && declare_attackers_trigger(player, card, event, DAT_SEPARATE_TRIGGERS, 1-player, -1))
	create_targetted_legacy_effect(player, card, effect_lose_defender_gain_trample, player, card);

  return 0;
}

int card_haunted_plate_mail(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	// |0: Until end of turn, ~ becomes a 4/4 Spirit artifact creature that's no longer an Equipment.
	//	Activate this ability only if you control no creatures.
	// Equip |4
	if( event == EVENT_CAN_ACTIVATE ){
		if( !check_battlefield_for_subtype(player, TYPE_CREATURE, -1) &&
			generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST0, 0, NULL, NULL)
		  ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 4);
	}
	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		if( !check_battlefield_for_subtype(player, TYPE_CREATURE, -1) &&
			generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST0, 0, NULL, NULL)
		  ){
			instance->info_slot = 66;
		}
		else if (activate_basic_equipment(player, card, 4)){
				instance->info_slot = 67;
		}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			artifact_animation(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 1, 4, 4, 0, 0);
		}
		if( instance->info_slot == 67 ){
			resolve_activation_basic_equipment(player, card);
		}
	}
	// Equipped creature gets +4/+4.
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && is_equipping(player, card) ){
		return vanilla_equipment(player, card, event, 0, 4, 4, 0, 0);
	}
	return 0;
}

int card_pyromancers_gauntlet(int player, int card, event_t event)
{
  /* If a |Sred instant or sorcery spell you control or a |Sred planeswalker you control would deal damage to a permanent or player, it deals that much damage
   * plus 2 to that permanent or player instead. */
  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && (damage->initial_color & get_sleighted_color_test(player, card, COLOR_TEST_RED))
	  && (damage->targets[3].player & (TYPE_SPELL | TARGET_TYPE_PLANESWALKER)))
	damage->info_slot += 2;

  return 0;
}

int card_ring_of_three_wishes(int player, int card, event_t event){

	/* Ring of Three Wishes	|5
	 * Artifact
	 * ~ enters the battlefield with three wish counters on it.
	 * |5, |T, Remove a wish counter from ~: Search your library for a card and put that card into your hand. Then shuffle your library. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_WISH, 3);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(5), GVC_COUNTER(COUNTER_WISH), NULL, NULL);
}

int card_staff_of_the_death_magus(int player, int card, event_t event){
	/* Staff of the Death Magus	|3
	 * Artifact
	 * Whenever you cast a |Sblack spell or |Ha Swamp enters the battlefield under your control, you gain 1 life. */
	return m14_staff(player, card, event, COLOR_TEST_BLACK, SUBTYPE_SWAMP);
}

int card_staff_of_the_flame_magus(int player, int card, event_t event){
	/* Staff of the Flame Magus	|3
	 * Artifact
	 * Whenever you cast a |Sred spell or |Ha Mountain enters the battlefield under your control, you gain 1 life. */
	return m14_staff(player, card, event, COLOR_TEST_RED, SUBTYPE_MOUNTAIN);
}

int card_staff_of_the_mind_magus(int player, int card, event_t event){
	/* Staff of the Mind Magus	|3
	 * Artifact
	 * Whenever you cast a |Sblue spell or |Han Island enters the battlefield under your control, you gain 1 life. */
	return m14_staff(player, card, event, COLOR_TEST_BLUE, SUBTYPE_ISLAND);
}

int card_staff_of_the_sun_magus(int player, int card, event_t event){
	/* Staff of the Sun Magus	|3
	 * Artifact
	 * Whenever you cast a |Swhite spell or |Ha Plains enters the battlefield under your control, you gain 1 life. */
	return m14_staff(player, card, event, COLOR_TEST_WHITE, SUBTYPE_PLAINS);
}

int card_staff_of_the_wild_magus(int player, int card, event_t event){
	/* Staff of the Wild Magus	|3
	 * Artifact
	 * Whenever you cast a |Sgreen spell or |Ha Forest enters the battlefield under your control, you gain 1 life. */
	return m14_staff(player, card, event, COLOR_TEST_GREEN, SUBTYPE_FOREST);
}

int card_vial_of_poison(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
		}
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

// lands

int card_encroaching_wastes(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_NONBASIC;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_X(5), 0, NULL, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Produce mana\n Kill a nonbasic\n Do nothing", 1);
		}
		if( choice == 0 ){
				return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				if (charge_mana_for_activated_ability(player, card, MANACOST_X(4)) && pick_target_nonbasic_land(player, card, 0)){
					instance->info_slot = 1;
					tap_card(player, card);
					kill_card(player, card, KILL_SACRIFICE);
				}
				else{
					remove_state(player, card, STATE_TAPPED);
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
	}

	return 0;
}

