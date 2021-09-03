#include "manalink.h"

// functions
static int is_artifact_creature(int player, int card){
	return is_what(player, card, TYPE_CREATURE) && is_what(player, card, TYPE_ARTIFACT);
}

enum{
	PUSIIPAT_TAPPED = 1,
};

static int kill_legacy_if_source_is_not_in_play_or_untapped(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[1].player > -1 ){
		int killme = 0;
		if( leaves_play(instance->targets[1].player, instance->targets[1].card, event) ){
			killme = 1;
		}
		if( instance->targets[3].player > 0 && (instance->targets[3].player & PUSIIPAT_TAPPED) &&
			! is_tapped(instance->targets[1].player, instance->targets[1].card)
		  ){
			killme = 1;
		}
		if( killme ){
			kill_card(instance->targets[2].player, instance->targets[2].card, KILL_REMOVE);
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

static int pump_until_source_is_in_play_and_tapped(int player, int card, int t_player, int t_card, int pow, int tou, int key, int s_key, int mode){
	pump_ability_t pump;
	default_pump_ability_definition(player, card, &pump, pow, tou, key, s_key);
	int legacy = pump_ability(player, card, t_player, t_card, &pump);
	card_instance_t* parent = get_card_instance(player, card);
	parent->targets[1].player = t_player;
	parent->targets[1].card = t_card;
	int l2 = create_targetted_legacy_effect(player, card, &kill_legacy_if_source_is_not_in_play_or_untapped, t_player, t_card);
	card_instance_t* leg = get_card_instance(player, l2);
	leg->targets[1].player = player;
	leg->targets[1].card = card;
	leg->targets[2].player = player;
	leg->targets[2].card = legacy;
	leg->targets[3].player = mode;
	add_status(player, l2, STATUS_INVISIBLE_FX);
	return legacy;
}

// cards

int card_amulet_of_kroog(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.extra = damage_card;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		if( dmg->info_slot > 0 ){
			dmg->info_slot--;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_X(2), 0, &td, "TARGET_DAMAGE");
}

int card_argivian_archaeologist(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card");

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_W(2), 0, NULL, NULL) ){
			if( count_graveyard_by_type(player, TYPE_ARTIFACT) ){
				return ! graveyard_has_shroud(2);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_W(2)) ){
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
	}

	return 0;
}

static const char* is_damaging_artifact_creature(int who_chooses, int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->internal_card_id == damage_card ){
		if( is_artifact_creature(instance->damage_target_player, instance->damage_target_card) ){
			return NULL;
		}
	}

	return "must be damage to an artifact creature";
}

int card_argivian_blacksmith(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_EFFECT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)is_damaging_artifact_creature;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		if( dmg->info_slot >= 2 ){
			dmg->info_slot-=2;
		}
		else{
			dmg->info_slot = 0;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST0, 0, &td, "TARGET_DAMAGE");
}

int card_argothian_pixies(int player, int card, event_t event){

	if( event == EVENT_BLOCK_LEGALITY && player == attacking_card_controller && card == attacking_card ){
		if( is_artifact_creature(affected_card_controller, affected_card) ){
			event_result = 1;
		}
	}

	card_instance_t* damage = damage_being_prevented(event);
	if(damage && damage->damage_target_card == card && damage->damage_target_player == player &&
		is_artifact_creature(-1, damage->targets[3].player)
	  ){
		damage->info_slot = 0;
	}
	return 0;
}

int card_argothian_treefolk(int player, int card, event_t event){

	card_instance_t* damage = damage_being_prevented(event);
	if(damage && damage->damage_target_card == card && damage->damage_target_player == player &&
		is_what(-1, damage->targets[3].player, TYPE_ARTIFACT)
	  ){
		damage->info_slot = 0;
	}
	return 0;
}

int card_armageddon_clock(int player, int card, event_t event)
{
  // 0x424040
  /* Armageddon Clock	|6
   * Artifact
   * At the beginning of your upkeep, put a doom counter on ~.
   * At the beginning of your draw step, ~ deals damage equal to the number of doom counters on it to each player.
   * |4: Remove a doom counter from ~. Any player may activate this ability but only during any upkeep step. */

  // At the beginning of your upkeep, put a doom counter on ~.
  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	add_counter(player, card, COUNTER_DOOM);

  // At the beginning of your draw step, ~ deals damage equal to the number of doom counters on it to each player.
  if ((trigger_condition == TRIGGER_DRAW_PHASE || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card)
	  && current_turn == player && reason_for_trigger_controller == player)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER || event == EVENT_SHOULD_AI_PLAY)
		{
		  int counters = count_counters(player, card, COUNTER_DOOM);
		  if (event == EVENT_SHOULD_AI_PLAY)
			++counters;

		  damage_player(current_turn, counters, player, card);
		  damage_player(1 - current_turn, counters, player, card);
		}
	}

  // |4: Remove a doom counter from ~. Any player may activate this ability but only during any upkeep step.
  if (event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_humiliated(player, card) )
	{
	  // I haven't the foggiest which part of this makes it activateable by the other player; or if the card is hardcoded somewhere I can't find.
	  int mana_needed = get_cost_mod_for_activated_abilities(player, card, MANACOST_X(4));

	  if (current_phase != PHASE_UPKEEP
		  || count_counters(player, card, COUNTER_DOOM) <= 0
		  || !has_mana(EXE_DWORD(0x60A534), COLOR_ANY, mana_needed)
		  || get_card_instance(player, card)->info_slot & 1)
		return 0;

	  if (HUMAN == EXE_DWORD(0x60A534) || (trace_mode & 2))
		return 1;

	  int should_ai_activate = life[AI] < life[HUMAN] || count_counters(player, card, COUNTER_DOOM) > life[AI];
	  if (should_ai_activate)
		EXE_DWORD(0x736808) |= 3;
	  return should_ai_activate;
	}

  if (event == EVENT_ACTIVATE)
	get_card_instance(player, card)->info_slot |= 1;

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  int mana_needed = get_cost_mod_for_activated_abilities(player, card, MANACOST_X(4));
	  card_instance_t* instance = get_card_instance(player, card);
	  card_instance_t* parent = in_play(instance->parent_controller, instance->parent_card);

	  if (parent)
		{
		  parent->info_slot = 0;

		  if (!IS_AI(EXE_DWORD(0x60A534))
			  || (life[AI] < life[HUMAN] || count_counters(player, card, COUNTER_DOOM) > life[AI]))
			{
			  if (charge_mana_while_resolving(instance->parent_controller, instance->parent_card,
											  EVENT_RESOLVE_ACTIVATION, EXE_DWORD(0x60A534), COLOR_COLORLESS, mana_needed))
				remove_counter(instance->parent_controller, instance->parent_card, COUNTER_DOOM);
			  else
				cancel = -1;
			}
		}
	}

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	ai_modifier += 24 * (life[AI] - life[HUMAN]);

  if (event == EVENT_STATIC_EFFECTS && stack_size == 0)
	get_card_instance(player, card)->info_slot = 0;	// just in case the ability was activated but countered

  return 0;
}

int card_artifact_blast(int player, int card, event_t event){
	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ARTIFACT);
	return counterspell(player, card, event, &td, 0);
}

int card_artifact_possession(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( (event == EVENT_TAP_CARD || event == EVENT_PLAY_ABILITY) && affect_me(p, c) ){
			damage_player(p, 2, player, card);
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_ARTIFACT");
}

int card_artifact_ward(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 0, 0, KEYWORD_PROT_ARTIFACTS, 0, 0, 0, 0);
}

int card_ashnods_altar(int player, int card, event_t event){

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( can_produce_mana(player, card) && can_sacrifice_type_as_cost(player, 1, TYPE_CREATURE) ){
			declare_mana_available(player, COLOR_COLORLESS, 2);
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_produce_mana(player, card) && can_sacrifice_type_as_cost(player, 1, TYPE_CREATURE) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			produce_mana(player, COLOR_COLORLESS, 2);
		}
		else{
			spell_fizzled = 1;
		}
	}

	return 0;
}

int card_ashnods_battle_gear(int player, int card, event_t event){

	choose_to_untap(player, card, event);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if (IS_AI(player)){
		td.toughness_requirement = 3 | TARGET_PT_GREATER_OR_EQUAL;
	}
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_until_source_is_in_play_and_tapped(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
												2, -2, 0, 0, PUSIIPAT_TAPPED);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

int card_ashnods_transmogrant(int player, int card, event_t event){
	// original code : 0x044E240

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.illegal_type = TYPE_ARTIFACT;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if (valid_target(&td)){
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			turn_into_artifact(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

	event_t ev = event == EVENT_CHECK_PUMP ? EVENT_CAN_ACTIVATE : event;
	int rval = generic_activated_ability(player, card, ev, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET|GAA_SACRIFICE_ME, MANACOST_X(0), 0, &td, "TARGET_CREATURE");

	if (event == EVENT_CHECK_PUMP){
		if (rval){
			++pumpable_power[player];
			++pumpable_toughness[player];
		}
		rval = 0;
	}

	if (event == EVENT_ACTIVATE && cancel != 1){
		if (instance->targets[0].player != player){
			ai_modifier -= 24;
		}
		card_instance_t* target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		if ((target->regen_status & KEYWORD_DEFENDER) && !(target->token_status & STATUS_WALL_CAN_ATTACK)){
			ai_modifier -= 24;
		}
	}

	return rval;
}

int card_atog2(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( ! sacrifice(player, card, player, 0, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
	}
	return 0;
}

static void destroy_wall_at_end_of_combat(int player, int card, int t_player, int t_card)
{
  if (has_subtype(t_player, t_card, SUBTYPE_WALL))
	create_legacy_effect_exe(player, card, LEGACY_EFFECT_STONING, t_player, t_card);
}

int card_battering_ram(int player, int card, event_t event)
{
  //0x422cc0
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_ABILITIES && affect_me(player, card) && !is_humiliated(player, card))
	{
	  if (current_turn == player && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2)
		event_result |= KEYWORD_BANDING;
	}

  /* EVENT_ABILITIES usually isn't dispatched between the start of combat and cards being chosen as attachers, so also add banding at the phase change.  Also
   * make sure abilities are promptly recalculated after combat. */
  if (event == EVENT_PHASE_CHANGED && current_turn == player && (current_phase == PHASE_DECLARE_ATTACKERS || current_phase == PHASE_MAIN2))
	{
	  instance->regen_status |= KEYWORD_RECALC_ABILITIES;
	  if (current_phase == PHASE_DECLARE_ATTACKERS)
		get_abilities(player, card, EVENT_ABILITIES, -1);
	}

  if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card) && current_turn == player)
	instance->destroys_if_blocked |= DIFB_DESTROYS_WALLS;

  if (event == EVENT_DECLARE_BLOCKERS && player == current_turn && (instance->state & STATE_ATTACKING) && !is_humiliated(player, card))
	for_each_creature_blocking_me(player, card, destroy_wall_at_end_of_combat, player, card);

  return 0;
}

// bronze tablet --> hardcoded

int card_candelabra_of_tawnos(int player, int card, event_t event)
{
  // 0x422d90

  if (event == EVENT_GET_SELECTED_CARD)
	EXE_FN(void, 0x499010, int)(0);

  if (!IS_ACTIVATING(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);
  td.preferred_controller = ANYBODY;

  if (event == EVENT_CAN_ACTIVATE)
	{
	  if (EXE_DWORD(0x60A560) < 1)
		max_x_value = target_available(player, card, &td);

	  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(0), 0, NULL, NULL);
	}

  if (event == EVENT_ACTIVATE)
	{
	  if (player == AI)
		ai_modifier -= 24;

	  card_instance_t* instance = get_card_instance(player, card);

	  instance->state |= STATE_TAPPED;

	  int store_max_x = max_x_value;

	  max_x_value = target_available(player, card, &td);
	  max_x_value = MIN(max_x_value, 10);
	  charge_mana_for_activated_ability(player, card, MANACOST_X(-1));

	  max_x_value = store_max_x;

	  if (cancel != 1)
		{
		  instance->number_of_targets = 0;
		  td.preferred_controller = player;

		  char prompt[100];
		  *prompt = 0;

		  int i;
		  for (i = 0; i < x_value; ++i)
			{
			  if (ai_is_speculating != 1)
				{
				  load_text(0, "CANDLEABRA_OF_TAWNOS");
				  scnprintf(prompt, 100, text_lines[0], i + 1, x_value);
				}

			  if (!select_target(player, card, &td, prompt, &instance->targets[i]))
				{
				  cancel = 1;
				  break;
				}

			  get_card_instance(instance->targets[i].player, instance->targets[i].card)->state |= STATE_CANNOT_TARGET | STATE_TARGETTED;
			}

		  for (i = 0; i < instance->number_of_targets; ++i)
			get_card_instance(instance->targets[i].player, instance->targets[i].card)->state &= ~(STATE_CANNOT_TARGET | STATE_TARGETTED);
		}

	  if (cancel == 1)
		{
		  instance->number_of_targets = 0;
		  instance->state &= ~STATE_TAPPED;
		}
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  int i;
	  for (i = 0; i < instance->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  untap_card(instance->targets[i].player, instance->targets[i].card);

	  get_card_instance(instance->parent_controller, instance->parent_card)->number_of_targets = 0;
	}

  return 0;
}

static const char* damage_from_artifact_source(int who_chooses, int player, int card){
	card_instance_t* dmg = get_card_instance(player, card);
	if ( dmg->internal_card_id == damage_card ){
		int good = dmg->targets[3].player & TYPE_ARTIFACT;
		if( in_play(dmg->damage_source_player, dmg->damage_source_card) ){
			good = is_what(dmg->damage_source_player, dmg->damage_source_card, TYPE_ARTIFACT);
		}
		if( dmg->damage_target_card != -1 || dmg->damage_target_player != who_chooses ){
			good = 0;
		}
		if( good ){
			return NULL;
		}
	}

	return "must be damage from artifact source";
}

int card_circle_of_protection_artifacts(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event)  ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_EFFECT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)damage_from_artifact_source;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->info_slot = 0;
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_X(2), 0, &td, "TARGET_DAMAGE");
}

int card_citanul_druid(int player, int card, event_t event)
{
  // 0x451c40

  // Whenever an opponent casts an artifact spell, put a +1/+1 counter on ~.
  test_definition_t test;
  new_default_test_definition(&test, TYPE_ARTIFACT, "");

  if (new_specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, &test))
	add_1_1_counter(player, card);

  return 0;
}

int card_clay_statue(int player, int card, event_t event){
	return regeneration(player, card, event, MANACOST_X(2));
}

int card_clockwork_avian(int player, int card, event_t event)
{
  // 0x426600
  return hom_clockwork(player, card, event, 4);
}

int card_colossus_of_sardia(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	does_not_untap(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		untap_card(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_IN_YOUR_TURN | GAA_ONLY_ON_UPKEEP, MANACOST_X(9), 0, NULL, NULL);
}

int card_coral_helm(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD_RANDOM | GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_CREATURE");
}

int card_crumble(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_ARTIFACT");
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = get_cmc(instance->targets[0].player, instance->targets[0].card);
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
				gain_life(instance->targets[0].player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_cursed_rack(int player, int card, event_t event){
	if( event == EVENT_MAX_HAND_SIZE && current_turn == 1-player && ! is_humiliated(player, card) ){
		event_result = 4;
	}
	return 0;
}

int card_damping_field(int player, int card, event_t event){
	untap_only_one_permanent_type(player, card, event, ANYBODY, TYPE_ARTIFACT);
	return global_enchantment(player, card, event);
}

int card_detonate(int player, int card, event_t event){
	return card_detonate_exe(player, card, event);
}

int card_drafnas_restoration(int player, int card, event_t event)
{
  // 0x41F560

  /* Drafna's Restoration	|U
   * Sorcery
   * Return any number of target artifact cards from target player's graveyard to the top of his or her library in any order. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.preferred_controller = player;

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  // Should target the artifacts here, too, but the exe doesn't and I can't be bothered to either.
  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	pick_target(&td, "TARGET_PLAYER");

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td) && !graveyard_has_shroud(instance->targets[0].player))
		{
		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_ARTIFACT, "Select target artifact card.");

		  int selected;
		  do
			{
			  selected = new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_GRAVE, TUTOR_DECK, 0, AI_MAX_VALUE, &test);
			} while (selected != -1 && !IS_AI(player));	// AI puts best artifact on top of own deck.
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_dragon_engine(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 2, 0, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_dwarven_weaponsmith(int player, int card, event_t event)
{
  // 0x4527e0

  // |T, Sacrifice an artifact: Put a +1/+1 counter on target creature. Activate this ability only during your upkeep.

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_ARTIFACT, "");

  if (event == EVENT_CAN_ACTIVATE)
	return (current_phase == PHASE_UPKEEP && current_turn == player
			&& !is_tapped(player, card) && !is_sick(player, card)
			&& can_target(&td)
			&& new_can_sacrifice_as_cost(player, card, &test)
			&& can_use_activated_abilities(player, card)
			&& has_mana_for_activated_ability(player, card, MANACOST_X(0)));

  if (event == EVENT_ACTIVATE)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (charge_mana_for_activated_ability(player, card, MANACOST_X(0))
		  && pick_target(&td, "TARGET_CREATURE"))
		new_sacrifice(player, card, player, 0, &test);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td))
		add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
	}

  return 0;
}

int card_energy_flux(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=5*(count_subtype(1-player, TYPE_ARTIFACT, -1)-count_subtype(player, TYPE_ARTIFACT, -1));
	}

	int upk;
	if (trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && !is_humiliated(player, card) && (upk = count_upkeeps(player)) > 0 &&
		count_subtype(current_turn, TYPE_ARTIFACT, -1) > 0
	  ){
		if (event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}

		if (event == EVENT_RESOLVE_TRIGGER){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ARTIFACT);
			int ai_best_choice = -1;
			while( upk ){
					int count = active_cards_count[current_turn]-1;
					while( count > -1 ){
							if( in_play(current_turn, count) && is_what(current_turn, count, TYPE_ARTIFACT) ){
								int kill_me = 1;
								add_state(current_turn, count, STATE_TARGETTED);
								if( has_mana(current_turn, COLOR_COLORLESS, 2) ){
									if( current_turn == AI && ai_best_choice == -1 ){
										ai_best_choice = check_battlefield_for_special_card(player, card, current_turn, CBFSC_AI_MAX_VALUE, &this_test);
									}
									if( current_turn == HUMAN || (current_turn == AI && count == ai_best_choice) ){
										int choice = 0;
										if( current_turn == HUMAN ){
											choice = do_dialog(current_turn, current_turn, count, -1, -1, " Pay upkeep\n Do not pay", 0);
										}
										if( choice == 0 ){
											charge_mana(current_turn, COLOR_COLORLESS, 2);
											if( spell_fizzled != 1 ){
												kill_me = 0;
												ai_best_choice = -1;
											}
										}
									}
								}
								remove_state(current_turn, count, STATE_TARGETTED);
								if( kill_me ){
									kill_card(current_turn, count, KILL_SACRIFICE);
								}
							}
							count--;
					}
					upk--;
			}
		}
	}

	return 0;
}

int card_feldons_cane(int player, int card, event_t event)
{
  // 0x420E70

  /* Feldon's Cane	|1
   * Artifact
   * |T, Exile ~: Shuffle your graveyard into your library. */

  int rval = generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
  if (event == EVENT_ACTIVATE)
	kill_card(player, card, KILL_REMOVE);

  if (event == EVENT_RESOLVE_ACTIVATION)
	reshuffle_grave_into_deck(player, 0);

  return rval;
}

int card_gaeas_avenger(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && player != -1){
		event_result+=count_subtype(1-player, TYPE_ARTIFACT, -1); // The "+1" is granted by the engine
	}

	return 0;
}

int card_gate_to_phyrexia(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=5*count_subtype(1-player, TYPE_ARTIFACT, -1);
	}

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_CAN_TARGET | GAA_ONCE_PER_TURN | GAA_ONLY_ON_UPKEEP,
									MANACOST0, 0, &td, "TARGET_ARTIFACT");
}

int card_goblin_artisans(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			if( counterspell(player, card, EVENT_CAN_CAST, &td, 0) ){
				if( ! check_special_flags2(card_on_stack_controller, card_on_stack, SF2_GOBLIN_ARTISANS) ){
					return 99;
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		ai_modifier+=5*(hand_count[player]-7);
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
			set_special_flags2(card_on_stack_controller, card_on_stack, SF2_GOBLIN_ARTISANS);
			tap_card(player, card);
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( counterspell_validate(player, card, &td, 0) ){
			if( flip_a_coin(instance->parent_controller, instance->parent_card) ){
				draw_cards(instance->parent_controller, 1);
			}
			else{
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
			}
		}
	}
	return 0;
}

int card_golgothian_sylex(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) && !is_token(i, count)){
						int csvid = get_id(i, count);
						if( csvid >= CARD_ID_AMULET_OF_KROOG && csvid < CARD_ID_YOTIAN_SOLDIER ){
							kill_card(i, count, KILL_SACRIFICE);
						}
					}
					count--;
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(1), 0, NULL, NULL);
}

int card_grapeshot_catapult(int player, int card, event_t event){

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		damage_target0(player, card, 1);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target creature with flying.");
}

int card_haunting_wind(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=5*(count_subtype(1-player, TYPE_ARTIFACT, -1)-count_subtype(player, TYPE_ARTIFACT, -1));
	}

	if( event == EVENT_PLAY_ABILITY && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) &&
		! is_tapped(affected_card_controller, affected_card)
	  ){
		damage_player(affected_card_controller, 1, player, card);
	}

	if( event == EVENT_TAP_CARD && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) ){
		damage_player(affected_card_controller, 1, player, card);
	}

	return 0;
}

int card_hurkyls_recall(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ARTIFACT);
			new_manipulate_all(player, card, get_card_instance(player, card)->targets[0].player, &this_test, ACT_BOUNCE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_ivory_tower(int player, int card, event_t event){

	if( !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
		current_turn == player
	  ){
		int count = count_upkeeps(player);
		if(event == EVENT_TRIGGER && count > 0 && hand_count[player] > 4 ){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				card_instance_t *instance= get_card_instance(player, card);
				card_data_t* card_d = &cards_data[ instance->internal_card_id ];
				int (*ptFunction)(int, int, event_t) = (void*)card_d->code_pointer;
				while( count > 0 ){
						ptFunction(player, card, EVENT_UPKEEP_TRIGGER_ABILITY );
						count--;
				}
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int amount = hand_count[player]-4;
		gain_life(player, amount);
	}

	return 0;
}

int card_jalum_tome(int player, int card, event_t event){

	if (event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
}

int card_martyrs_of_korlis(int player, int card, event_t event ){
	// original code : 0x453EB0

	if( event == EVENT_PREVENT_DAMAGE && ! is_tapped(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_target_player == player &&
				damage->info_slot > 0
			  ){
				if( damage_from_artifact_source(-1, affected_card_controller, affected_card) == NULL ){
					damage->damage_target_card = card;
				}
			}
		}
	}

	return 0;
}

int card_mightstone(int player, int card, event_t event ){
	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER && is_attacking(affected_card_controller, affected_card) ){
			event_result++;
		}
	}
	return 0;
}

int card_millstone(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			mill( instance->targets[0].player, 2);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td1, "TARGET_PLAYER");
}

int card_mishras_factory(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) ){
		if( instance->targets[12].card > -1 ){
			event_result = instance->targets[12].card;
		}
	}

	if (event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if( instance->targets[1].player != 66 ){
			return mana_producer(player, card, event);
		}
	}
	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		if (event == EVENT_CAN_ACTIVATE && instance->targets[1].player == 66){
			return 0;
		}
		if (paying_mana()){
			return mana_producer(player, card, event);
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.required_subtype = SUBTYPE_ASSEMBLY_WORKER;

		int can_generate_mana = (!is_tapped(player, card) && !is_animated_and_sick(player, card)
								 && instance->targets[1].player != 66 && can_produce_mana(player, card));
		int ai_priority_generate_mana = -1;

		int can_animate = can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(1));
		int ai_priority_animate = (is_what(player, card, TYPE_CREATURE)
								   || (current_turn == player && current_phase > PHASE_MAIN1)
								   || (current_turn != player && current_phase != PHASE_BEFORE_BLOCKING)) ? 0 : 2;

		int can_pump = (can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST0)
						&& !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_target(&td));
		int ai_priority_pump = current_phase == PHASE_AFTER_BLOCKING ? 3 : 1;

		int choice = DIALOG(player, card, event,
							"Generate mana", can_generate_mana, ai_priority_generate_mana,
							"Animate", can_animate, ai_priority_animate,
							"Pump an Assembly Worker", can_pump, ai_priority_pump);

		if (event == EVENT_CAN_ACTIVATE){
			return choice;
		}
		else if (event == EVENT_ACTIVATE){
			if (choice == 1){
				return mana_producer(player, card, event);
			}
			else if (choice == 2){
				if (player == AI){
					instance->targets[1].player = 66;
				}
				charge_mana_for_activated_ability(player, card, MANACOST_X(1));
				instance->targets[1].player = 0;
			}
			else if (choice == 3){
				instance->targets[1].player = 66;
				instance->number_of_targets = 0;
				if (charge_mana_for_activated_ability(player, card, MANACOST0) &&
					select_target(player, card, &td, "Select an Assembly Worker to pump", &(instance->targets[0]))
				  ){
					instance->number_of_targets = 1;
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
				instance->targets[1].player = 0;
			}
		}
		else {	// event == EVENT_RESOLVE_ACTIVATION
			if (choice == 2 ){
				get_card_instance(instance->parent_controller, instance->parent_card)->targets[12].card = get_internal_card_id_from_csv_id(CARD_ID_ASSEMBLY_WORKER);
				add_status(instance->parent_controller, instance->parent_card, STATUS_ANIMATED);
			}
			else if (choice == 3 && valid_target(&td)){
					pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[12].card = get_internal_card_id_from_csv_id(CARD_ID_MISHRAS_FACTORY);
	}
	return 0;
}

int card_assembly_worker(int player, int card, event_t event){
	if( get_special_infos(player, card) != 66 ){
		return card_mishras_factory(player, card, event);
	}
	return 0;
}

int card_mishras_war_machine(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( hand_count[player] > 0 ){
			int ai_choice = 0;
			if( (! can_attack(player, card) && life[player] > 5) || hand_count[player] < count_upkeeps(player)	){
				ai_choice = 1;
			}
			int choice = do_dialog(player, player, card, -1, -1, " Pay upkeep\n Pass", ai_choice);
			if( choice == 0 ){
				discard(player, 0, player);
				kill--;
			}
		}
		if( kill == 1 ){
			damage_player(player, 3, player, card);
			tap_card(player, card);
		}
	}
	return 0;
}

int card_mishras_workshop(int player, int card, event_t event)
{
  //0x47b5e0
  if (event == EVENT_COUNT_MANA && affect_me(player, card)
	  && !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card))
	declare_mana_available(player, COLOR_ARTIFACT, 3);

  if (event == EVENT_CAN_ACTIVATE)
	return !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card);

  if (event == EVENT_ACTIVATE)
	produce_mana_tapped(player, card, COLOR_ARTIFACT, 3);

  if (event == EVENT_RESOLVE_SPELL)
	play_land_sound_effect(player, card);

	return 0;
}

static const char* permanent_you_control_and_own(int who_chooses, int player, int card){
	if( who_chooses == AI ){
		if( check_state(player, card, STATE_OWNED_BY_OPPONENT) ){
			return NULL;
		}
	}
	if( who_chooses == HUMAN ){
		if( ! check_state(player, card, STATE_OWNED_BY_OPPONENT) ){
			return NULL;
		}
	}

	return "must be a permanent you both own and control";
}

int card_obelisk_of_undoing(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT );
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td1.extra = (int32_t)permanent_you_control_and_own;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			bounce_permanent( instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_CAN_TARGET, MANACOST_X(6), 0, &td1, "TARGET_PERMANENT");
}

int card_onulet(int player, int card, event_t event){

	if( graveyard_from_play(player, card, event) ){
		gain_life(player, 2);
	}

	return 0;
}

int card_orcish_mechanic(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td1, "TARGET_CREATURE_OR_PLAYER") ){
			return can_sacrifice_type_as_cost(player, 1, TYPE_ARTIFACT);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact to sacrifice.");
			if( new_sacrifice(player, card, player, 0, &this_test) ){
				if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
					instance->number_of_targets = 1;
					tap_card(player, card);
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			damage_target0(player, card, 2);
		}
	}

	return 0;
}

// Ornithopter --> vanilla

int card_phyrexian_gremlins(int player, int card, event_t event){
	// original code : 0x4549D0

	choose_to_untap(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
			parent->targets[1] = instance->targets[0];
			does_not_untap_until_im_tapped(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_ARTIFACT");
}

static int is_monolith(int id){
	if( id == CARD_ID_MANA_VAULT ||
		id == CARD_ID_GRIM_MONOLITH ||
		id == CARD_ID_BASALT_MONOLITH
	  ){
		return 1;
	}
	return 0;
}

int card_power_artifact(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		if( leaves_play(player, card, event) ){
			remove_cost_mod_for_activated_abilities(instance->damage_target_player, instance->damage_target_card, 2, 0);
		}
	}
	if( event == EVENT_CAN_CAST ){
		if( player != AI ){
			return can_target(&td);
		}
		else{
			int count = 0;
			while( count < active_cards_count[player] ){
					if( in_play(player, count) && is_monolith(get_id(player, count)) ){
						instance->targets[0].player = player;
						instance->targets[0].card = count;
						instance->number_of_targets = 1;
						if( would_valid_target(&td) ){
							return 1;
						}
					}
					count++;
			}
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player != AI ){
			pick_target(&td, "TARGET_ARTIFACT");
		}
		else{
			instance->targets[0].player = -1;
			instance->targets[0].card = -1;
			instance->number_of_targets = 0;
			int count = 0;
			while( count < active_cards_count[player] ){
					if( in_play(player, count) && is_monolith(get_id(player, count)) ){
						instance->targets[0].player = player;
						instance->targets[0].card = count;
						instance->number_of_targets = 1;
						if( would_valid_target(&td) ){
							ai_modifier+=100;
							break;
						}
					}
					count++;
			}
			if( instance->targets[0].player == -1 ){
				spell_fizzled = 1;
			}
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			instance->damage_target_player = instance->targets[0].player;
			instance->damage_target_card = instance->targets[0].card;
			set_cost_mod_for_activated_abilities(instance->targets[0].player, instance->targets[0].card, 2, 0);
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_CAN_MOVE_AURA || event == EVENT_MOVE_AURA || event == EVENT_RESOLVE_MOVING_AURA ){
		return targeted_aura(player, card, event, &td, "TARGET_ARTIFACT");
	}

	return 0;
}

int card_powerleech(int player, int card, event_t event){
	// original code : 0x4327D0

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=5*count_subtype(1-player, TYPE_ARTIFACT, -1);
	}


	if( event == EVENT_PLAY_ABILITY && affected_card_controller == 1-player &&
		is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) &&
		! is_tapped(affected_card_controller, affected_card)
	  ){
		gain_life(player, 1);
	}

	if( event == EVENT_TAP_CARD && affected_card_controller == 1-player &&
		is_what(affected_card_controller, affected_card, TYPE_ARTIFACT)
	  ){
		gain_life(player, 1);
	}

	return 0;
}

int card_priest_of_yawgmoth(int player, int card, event_t event){
	// I really don't know how to hint the AI about the amount of mana produced by this, Korath please take care of this

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_type_as_cost(player, 1, TYPE_ARTIFACT);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ARTIFACT, "Select an artifact to sacrifice.");
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (!sac){
				cancel = 1;
				return 0;
			}
			tap_card(player, card);
			int cmc = get_cmc(BYTE2(sac), BYTE3(sac));
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			produce_mana(player, COLOR_BLACK, cmc);
		}
	}

	return 0;
}

int card_primal_clay(int player, int card, event_t event)
{
  //0x4210a0

  /* Primal Clay	|4
   * Artifact Creature - Shapeshifter 100/100
   * As ~ enters the battlefield, it becomes your choice of a 3/3 artifact creature, a 2/2 artifact creature with flying, or a 1/6 Wall artifact creature with
   * defender in addition to its other types. */

  // Also Primal Plasma, which doesn't add subtype Wall.
  /* Primal Plasma	|3|U
   * Creature - Elemental Shapeshifter 100/100
   * As ~ enters the battlefield, it becomes your choice of a 3/3 creature, a 2/2 creature with flying, or a 1/6 creature with defender. */

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  instance->state |= STATE_OUBLIETTED;

	  int new_iid = create_a_card_type(instance->internal_card_id);
	  card_data_t* cd = &cards_data[new_iid];

	  char prompt[600];
	  if (ai_is_speculating != 1)
		{
		  load_text(0, "PRIMAL_CLAY");
		  sprintf(prompt, "%s\n %s\n %s\n %s", text_lines[0], text_lines[get_id(player, card) == CARD_ID_PRIMAL_CLAY ? 1 : 4], text_lines[2], text_lines[3]);
		}

	  switch (do_dialog(player, player, card, -1, -1, prompt, 1))
		{
		  case 0:
			cd->power = 1;
			cd->toughness = 6;
			cd->static_ability = KEYWORD_DEFENDER;
			if (get_id(player, card) == CARD_ID_PRIMAL_CLAY)
			  add_a_subtype(player, card, SUBTYPE_WALL);
			break;

		  case 1:
			cd->power = cd->toughness = 2;
			cd->static_ability = KEYWORD_FLYING;
			break;

		  case 2:
			cd->power = cd->toughness = 3;
			cd->static_ability = 0;
			break;
		}

	  instance->state &= ~STATE_OUBLIETTED;

	  instance->backup_internal_card_id = instance->dummy3 = instance->internal_card_id = new_iid;
	  instance->regen_status |= KEYWORD_RECALC_ABILITIES|KEYWORD_RECALC_POWER|KEYWORD_RECALC_TOUGHNESS|KEYWORD_RECALC_CHANGE_TYPE;
	}

  if (event == EVENT_CHANGE_TYPE
	  && !(land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS)
	  && affect_me(player, card)
	  && (in_play(player, card) || (instance->state & STATE_INVISIBLE)))
	event_result = instance->dummy3;

  return 0;
}

int card_rakalite(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->info_slot > 0 && eot_trigger(player, card, event) ){
		instance->info_slot = 0;
		bounce_permanent(player, card);
	}

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.extra = damage_card;
	td.illegal_abilities = 0;

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_X(2), 0, &td, "TARGET_DAMAGE");
	}

	if( event == EVENT_ACTIVATE ){
		generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_X(2), 0, &td, "TARGET_DAMAGE");
		if( spell_fizzled != 1 ){
			instance->info_slot++;
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		if( dmg->info_slot > 0 ){
			dmg->info_slot--;
		}
	}

	return 0;
}

int card_reconstruction(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	char msg[100] = "Select an Artifact card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ARTIFACT, msg);

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &this_test);
}

int card_reverse_polarity(int player, int card, event_t event)
{
  // 0x475260

  // You gain X life, where X is twice the damage dealt to you so far this turn by artifacts.
  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  gain_life(player, 2 * get_trap_condition(player, TRAP_ARTIFACT_DAMAGE_TAKEN));
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_rocket_launcher(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->info_slot > 0 && eot_trigger(player, card, event) ){
		instance->info_slot = 0;
		kill_card(player, card, KILL_DESTROY);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		set_special_flags(player, card, SF_CORRECTLY_RESOLVED);
	}

	if( event == EVENT_CLEANUP ){
		remove_special_flags(player, card, SF_CORRECTLY_RESOLVED);
	}

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_ACTIVATE && ! check_special_flags(player, card, SF_CORRECTLY_RESOLVED) ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(2), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_X(2), 0, &td, "TARGET_CREATURE_OR_PLAYER");
		if( spell_fizzled != 1 ){
			instance->info_slot++;
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		damage_target0(player, card, 1);
	}

	return 0;
}

int card_sage_of_lat_nam(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			return can_sacrifice_type_as_cost(player, 1, TYPE_ARTIFACT);
		}
	}
	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ARTIFACT);
			if( new_sacrifice(player, card, player, 0, &this_test) ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}
	return 0;
}

int card_shapeshifter2(int player, int card, event_t event){
	/* Shapeshifter	|6
	 * Artifact Creature - Shapeshifter 100/207
	 * As ~ enters the battlefield, choose a number between 0 and 7.
	 * At the beginning of your upkeep, you may choose a number between 0 and 7.
	 * ~'s power is equal to the last chosen number and its toughness is equal to 7 minus that number. */

	if (card == -1){
		// While not on the bf, no number has been chosen; so power is 0 and toughness is 7.
		if (event == EVENT_TOUGHNESS && affect_me(player, card)){
			event_result += 7;
		}
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_POWER && affect_me(player, card) && instance->targets[1].player > -1 ){
		event_result+=instance->targets[1].player;
	}

	if( event == EVENT_TOUGHNESS && affect_me(player, card) && instance->targets[1].card > -1 ){
		event_result+=instance->targets[1].card;
	}

	if( event == EVENT_RESOLVE_SPELL || event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( player == HUMAN ){
			int result = -1;
			while( result == -1 ){
					result = choose_a_number(player, "Choose a number between 0 and 7 for Power", 0);
					if( result < 0 || result > 7 ){
						result = -1;
					}
			}
			instance->targets[1].player = result;
			instance->targets[1].card = 7-result;
		}
		else{
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			int max_pow = check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_MAX_POW, &this_test);
			int my_tou = max_pow+1;
			if( my_tou > 7 ){
				my_tou = 1;
			}
			instance->targets[1].player = 7-my_tou;
			instance->targets[1].card = my_tou;
		}
	}

	int upk;
	if (trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && !is_humiliated(player, card) && (upk = count_upkeeps(player)) > 0){
		if (event == EVENT_TRIGGER){
			event_result |= player == AI ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
		}

		if (event == EVENT_RESOLVE_TRIGGER){
			call_card_function(player, card, EVENT_UPKEEP_TRIGGER_ABILITY);
		}
	}
	return 0;
}

int card_shatterstorm(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		new_manipulate_all(player, card, ANYBODY, &this_test, KILL_BURY);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_staff_of_zegon(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -2, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_CREATURE");
}

int card_strip_mine(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	if (event == EVENT_ACTIVATE){
		instance->info_slot = 0;
		int choice = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST0, 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Destroy a land\n Cancel", 1);
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				if( charge_mana_for_activated_ability(player, card, MANACOST0) &&
					new_pick_target(&td, "TARGET_LAND", 0, 1)
				  ){
					instance->info_slot = 1;
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

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && validate_target(player, card, &td, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}


int card_su_chi(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		produce_mana(player, COLOR_COLORLESS, 4);
	}
	return 0;
}

int card_tablet_of_epityr(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_ARTIFACT, 0);
	}

	if( has_mana(player, COLOR_COLORLESS, 1) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int i;
		for(i=0; i<instance->targets[11].card; i++){
			if( ! has_mana(player, COLOR_COLORLESS, 1) ){
				break;
			}
			charge_mana(player, COLOR_COLORLESS, 1);
			if( spell_fizzled != 1 ){
				gain_life(player, 1);
			}
		}
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_tawnos_wand(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

int card_tawnoss_coffin(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	// straight from "choose_to_untap", with the noted differences
	if (event == EVENT_UNTAP && affect_me(player, card)){
		instance->untap_status &= ~2;
	}

	if (current_phase == PHASE_UNTAP && affect_me(player, card)){
		if (event == EVENT_TRIGGER){
			if (instance->untap_status & 1){
				if (!(instance->untap_status & 2)
					&& !(cards_data[instance->internal_card_id].type & types_that_dont_untap)){
					if ((player != 1 || (trace_mode & 2)) && ai_is_speculating != 1){
						event_result |= 1;	// Human: untapping is optional
					}
					else{
						/* AI : leave tapped if CSVID contained in "instance->targets[1].card" is found in "instance->targets[1].player"'s Exile zone
						and instance->targets[1].player != player */
						if (instance->targets[1].player > -1 ){
							if( instance->targets[1].player == player ){
								instance->targets[1].player = -1;
								event_result |= 2;
							}
							else{
								if( ! check_rfg(instance->targets[1].player, instance->targets[1].card)){
									instance->targets[1].player = -1;
									event_result |= 2;
								}
							}
						}
					}
				}
			}
		}
		else if (event == EVENT_RESOLVE_TRIGGER){
				instance->untap_status |= 2;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( ! is_token(instance->targets[0].player, instance->targets[0].card) ){
				card_instance_t *parent = get_card_instance(player, instance->parent_card);
				parent->targets[1].player = get_owner(instance->targets[0].player, instance->targets[0].card);
				parent->targets[1].card = cards_data[get_original_id(instance->targets[0].player, instance->targets[0].card)].id;
			}
			exile_permanent_and_auras_attached(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
				EPAAC_TAWNOSS_COFFIN | EPAAC_RETURN_TO_PLAY_TAPPED );
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_tawnos_weaponry(int player, int card, event_t event){
	choose_to_untap(player, card, event);

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_until_source_is_in_play_and_tapped(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
												1, 1, 0, 0, PUSIIPAT_TAPPED);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

int card_tetravite(int player, int card, event_t event){
	// original code: 00421B70

	/* Tetravite	""
	 * Artifact Creature - Tetravite 1/1
	 * Flying
	 * ~ can't be enchanted. */

	if( get_special_infos(player, card) > 0 ){
		cannot_be_enchanted(player, card, event);
	}

	return 0;
}

static int tetra_code = 0;
static const char* is_my_tetravite(int unused, int player, int card){
	if( get_id(player, card) == CARD_ID_TETRAVITE && get_special_infos(player, card) == tetra_code ){
		return NULL;
	}
	return "this Tetravite was generated by another Tetravus.";
}

int card_tetravus(int player, int card, event_t event){
	//  original code : 00421590

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int ids = internal_rand(100);
		set_special_infos(player, card, ids);
	}

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 3);

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int tetravites = 0;
		int i;
		int choice;

		if( count_1_1_counters(player, card) > 0 ){
			choice = do_dialog(player, player, card, -1, -1,
							   " Release some Tetravites (Auto)\n Release some Tetravites (Manual)\n Pass", 0);
			int number = 0;

			if( choice == 0 ){
				number = count_1_1_counters(player, card);
			}

			if( choice == 1 ){
				number = choose_a_number(player, "Remove how many counters?", count_1_1_counters(player, card));
				if( number > count_1_1_counters(player, card) ){
					number = count_1_1_counters(player, card);
				}
			}

			if( number > 0 ){
				for(i=0; i<number;i++){
					remove_1_1_counter(player, card);
				}
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_TETRAVITE, &token);
				token.qty = number;
				token.special_infos = get_special_infos(player, card);
				generate_token(&token);
			}
		}

		for(i =0; i<2; i++){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && get_id(i, count) == CARD_ID_TETRAVITE ){
						if( get_special_infos(player, card) == get_special_infos(i, count) ){
							tetravites++;
						}
					}
					count++;
			}
		}

		if( tetravites > 0 ){
			int ai_choice = 2;
			for( i = 0; i < active_cards_count[player]; i++){
				if( in_play(player, i) && ( get_id(player, i) == CARD_ID_DOUBLING_SEASON ||
											get_id(player, i) == CARD_ID_PARALLEL_LIVES ||
											get_id(player, i) == CARD_ID_SELESNYA_LOFT_GARDENS ||
											get_id(player, i) == CARD_ID_PRIMAL_VIGOR
										   )
				  ){
					ai_choice = 0;
					break;
				}
			}

			choice = do_dialog(player, player, card, -1, -1, " Dock some Tetravites (Auto)\n Dock some Tetravites (Manual)\n Pass", ai_choice);
			if( choice == 0 ){
				i=0;
				for(i =0; i<2; i++){
					int count = 0;
					while( count < active_cards_count[i] ){
							if( in_play(i, count) && get_id(i, count) == CARD_ID_TETRAVITE ){
								if( get_special_infos(player, card) == get_special_infos(i, count) ){
									kill_card(i, count, KILL_SACRIFICE);
									add_1_1_counter(player, card);
								}
							}
							count++;
					}
				}
			}

			if( choice == 1 ){
				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_CREATURE );
				td.illegal_abilities = 0;
				td.extra = (int32_t)is_my_tetravite;
				td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
				tetra_code = get_special_infos(player, card);

				while( can_target(&td) ){
						if( pick_target(&td, "TETRAVITE") ){
							kill_card(instance->targets[0].player,instance->targets[0].card, KILL_SACRIFICE);
							add_1_1_counter(player, card);
							tetravites--;
						}
						else{
							break;
						}
				}
			}

		}
	}

	return 0;
}

int card_the_rack(int player, int card, event_t event){
	// 0x420650
	/*
	  The Rack |1
	  Artifact
	  As The Rack enters the battlefield, choose an opponent.
	  At the beginning of the chosen player's upkeep, The Rack deals X damage to that player,
		where X is 3 minus the number of cards in his or her hand.
	*/
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=(5*(7-hand_count[1-player]));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		get_card_instance(player, card)->targets[0].player = 1-player;
	}

	if( get_card_instance(player, card)->targets[0].player > -1 ){
		upkeep_trigger_ability(player, card, event, get_card_instance(player, card)->targets[0].player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int af_player = get_card_instance(player, card)->targets[0].player;
			int amount = 3-hand_count[af_player];
			if( amount > 0 ){
				damage_player(af_player, amount, player, card);
			}
		}
	}

	return 0;
}

static int titanias_song_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CHANGE_TYPE ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		this_test.type_flag = F1_NO_CREATURE;

		global_type_change(player, card, event, 2, TYPE_CREATURE, &this_test, 16384, 16384, 0, 0, 0);
	}

	if( event == EVENT_CAST_SPELL && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) &&
		! is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_what(affected_card_controller, affected_card, TYPE_EFFECT)
	  ){
		humiliate(player, card, affected_card_controller, affected_card, 1);
	}

	if( instance->targets[0].player > -1 && ! in_play(instance->targets[0].player, instance->targets[0].card) ){
		if( instance->token_status & STATUS_INVISIBLE_FX){
			remove_status(player, card, STATUS_INVISIBLE_FX);
		}
		if( eot_trigger(player, card, event) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ARTIFACT);
			new_manipulate_all(player, card, player, &this_test, ACT_DE_HUMILIATE);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_titanias_song(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_ARTIFACT) && ! is_what(i, count, TYPE_CREATURE)){
						humiliate(player, card, i, count, 1);
					}
					count--;
			}
		}

		int legacy = create_legacy_effect(player, card, &titanias_song_legacy);
		get_card_instance(player, legacy)->targets[0].player = player;
		get_card_instance(player, legacy)->targets[0].card = card;
		get_card_instance(player, legacy)->number_of_targets = 1;
		get_card_instance(player, legacy)->token_status |= STATUS_INVISIBLE_FX;
	}

	return global_enchantment(player, card, event);
}

int card_transmute_artifact(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an Artifact card");
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &this_test);
		if( sac ){
			int amount = get_cmc(BYTE2(sac), BYTE3(sac));
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			if( player == AI ){
				int count = 1;
				while( has_mana(player, COLOR_COLORLESS, count) ){
						count++;
				}
				this_test.cmc = amount+count+1;
				this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
			}
			int card_added = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			if( card_added != -1 ){
				int pip = 1;
				int diff = get_cmc(player, card_added)-amount;
				if( diff > 0 ){
					charge_mana(player, COLOR_COLORLESS, diff);
					pip = 1-spell_fizzled;
				}
				if( pip ){
					put_into_play(player, card_added);
				}
				else{
					put_on_top_of_deck(player, card_added);
					mill(player, 1);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_triskelion(int player, int card, event_t event){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 3);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_1_1_COUNTER | GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_urzas_avenger(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL);
		return player == AI ? result && get_toughness(player, card) > 1 : result;
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( player == HUMAN ){
				int kw[4] = {KEYWORD_BANDING, KEYWORD_FLYING, KEYWORD_FIRST_STRIKE, KEYWORD_TRAMPLE};
				int choice = do_dialog(player, player, card, -1, -1, " Get Banding\n Get Flying\n Get First Strike\n Get Trample\n Cancel", 0);
				if( choice == 4){
					spell_fizzled = 1;
				}
				else{
					instance->targets[1].player = kw[choice];
				}
			}
			else{
				int kw[4] = {KEYWORD_FLYING, KEYWORD_TRAMPLE, KEYWORD_FIRST_STRIKE, KEYWORD_BANDING};
				if( instance->targets[1].card < 0 ){
					instance->targets[1].card = 0;
				}
				int choice = 0;
				while( instance->targets[1].card & kw[choice] ){
						choice++;
				}
				if( choice > 3 ){
					spell_fizzled = 1;
				}
				else{
					instance->targets[1].player = kw[choice];
					instance->targets[1].card |= kw[choice];
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card,
								-1, -1, instance->targets[1].player, 0);
	}

	return 0;
}

int card_urzas_chalice(int player, int card, event_t event){

	if( has_mana(player, COLOR_COLORLESS, 1) &&
		specific_spell_played(player, card, event, 2, 1+player, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
	  ){
		charge_mana(player, COLOR_COLORLESS, 1);
		if( spell_fizzled != 1 ){
			gain_life(player, 1);
		}
	}

	return 0;
}

static int urzatron_land(int player, int card, event_t event, int mana_amt)
{
  if (event == EVENT_COUNT_MANA && affect_me(player, card)
	  && !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card))
	declare_mana_available(player, COLOR_COLORLESS, urzatron_parts(player) == 7 ? mana_amt : 1);

  if (event == EVENT_CAN_ACTIVATE)
	return !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card);

  if (event == EVENT_ACTIVATE)
	produce_mana_tapped(player, card, COLOR_COLORLESS, urzatron_parts(player) == 7 ? mana_amt : 1);

  if (event == EVENT_RESOLVE_SPELL)
	play_land_sound_effect(player, card);

  return 0;
}

int card_urzas_tower(int player, int card, event_t event)
{
  //0x4a91e0
  return urzatron_land(player, card, event, 3);
}

int card_urzas_mine_card_urzas_power_plant(int player, int card, event_t event)
{
  //0x4a90b0
  return urzatron_land(player, card, event, 2);
}

int card_urzas_miter(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);

		if( event == EVENT_GRAVEYARD_FROM_PLAY && affected_card_controller == player ){
			if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
			if( is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) ){
				card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
				if( affected->kill_code == KILL_DESTROY || affected->kill_code == KILL_BURY ){
					if( instance->targets[11].player < 0 ){
						instance->targets[11].player = 0;
					}
					instance->targets[11].player++;
				}
			}
		}

		if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
			affect_me(player, card )
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int count = 0;
					while( count < instance->targets[11].player && has_mana(player, COLOR_COLORLESS, 3) ){
							if( do_dialog(player, player, card, -1, -1, " Draw a card\n Pass", count_deck(player) < 10) == 0 ){
								charge_mana(player, COLOR_COLORLESS, 3);
								if( spell_fizzled != 1 ){
									draw_cards(player, 1);
								}
							}
							count++;
					}
					instance->targets[11].player = 0;
			}
		}
	}
	return 0;
}

// wall of spears --> vanilla

int card_weakstone(int player, int card, event_t event ){
	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER && is_attacking(affected_card_controller, affected_card) ){
			event_result--;
		}
	}
	return 0;
}

int card_xenic_poltergeist(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );
	td.preferred_controller = player;
	td.illegal_type = TYPE_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int pt = get_cmc(instance->targets[0].player, instance->targets[0].card);
			int legacy = turn_into_creature(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
											8, pt, pt);
			get_card_instance(player, legacy)->targets[4].player = instance->parent_controller;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target noncreature artifact.");
}

int card_yawgmoths_demon(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT);
		if( new_can_sacrifice(player, card, 1, &this_test) ){
			int ai_choice = 0;
			if( (! can_attack(player, card) && life[player] > 5) || count_subtype(player, TYPE_ARTIFACT, -1) < count_upkeeps(player) ){
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
			damage_player(player, 2, player, card);
			tap_card(player, card);
		}
	}
	return 0;
}

// yotian solider --> serra angel





