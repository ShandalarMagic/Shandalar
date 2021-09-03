#include "manalink.h"

// Generic functions
int monolith(int player, int card, event_t event, int untap_cost){
	does_not_untap(player, card, event);

	card_instance_t *instance = get_card_instance( player, card);

	int active = ! is_tapped(player, card) && ! is_animated_and_sick(player, card ) && can_produce_mana(player, card);

	if( event == EVENT_COUNT_MANA && active && affect_me(player, card)){
		declare_mana_available(player, COLOR_COLORLESS, 3);
	}

	if(event == EVENT_CAN_ACTIVATE ){
		if( active == 1 || generic_activated_ability(player, card, event, 0, MANACOST_X(untap_cost), 0, NULL, NULL) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE){
		instance->info_slot = 0;
		if( active ){
			produce_mana_tapped(player, card, COLOR_COLORLESS, 3);
		}
		else{
			charge_mana_for_activated_ability(player, card, MANACOST_X(untap_cost));
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			untap_card( instance->parent_controller, instance->parent_card );
		}
	}

	if( player == AI && is_tapped(player, card) && current_turn != player &&
		generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_X(untap_cost), 0, NULL, NULL) &&
		eot_trigger(player, card, event)
	  ){
		charge_mana_for_activated_ability(player, card, MANACOST_X(untap_cost));
		if( spell_fizzled != 1 ){
			untap_card(player, card);
		}
	}

	return 0;
}

static int generic_x_spell_recoded(int player, int card, event_t event, int result){

	if( event == EVENT_CAN_CAST ){
		if( result > 0 && ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG) ){
			return result;
		}
	}
	else{
		return result;
	}
	return 0;
}

int card_generic_noncombat_1_mana_producing_creature(int player, int card, event_t event){
	// Birds of Paradise, Copper Myr, Gold Myr, Iron Myr, Leaden Myr, Silver Myr, Urborg Elf, Druid of the Anima, others
	if (event == EVENT_CAN_CAST){
		return 1;	// so it can be assigned to a creature with flash
	}

	int colors = get_card_instance(player, card)->mana_color & COLOR_TEST_ANY;
	if (!colors){
		return 0;
	} else if ((colors & (colors - 1)) == 0){	// exactly one bit set
		return mana_producing_creature(player, card, event, 24, single_color_test_bit_to_color(colors), 1);
	} else {
		return mana_producing_creature_all_one_color(player, card, event, 24, colors, 1);
	}
}

static int ward(int player, int card, event_t event, int keyword){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int prot = get_sleighted_protection(player, card, keyword);
		if( prot & KEYWORD_PROT_BLACK ){
			protection_from_black(instance->damage_target_player, instance->damage_target_card, event);
		}
		if( prot & KEYWORD_PROT_BLUE ){
			protection_from_blue(instance->damage_target_player, instance->damage_target_card, event);
		}
		if( prot & KEYWORD_PROT_GREEN ){
			protection_from_green(instance->damage_target_player, instance->damage_target_card, event);
		}
		if( prot & KEYWORD_PROT_RED ){
			protection_from_red(instance->damage_target_player, instance->damage_target_card, event);
		}
		if( prot & KEYWORD_PROT_WHITE ){
			protection_from_white(instance->damage_target_player, instance->damage_target_card, event);
		}
	}

	return generic_aura(player, card, event, player, 0, 0, get_sleighted_protection(player, card, keyword), 0, 0, 0, 0);
}

static int blast(int player, int card, event_t event, int color){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_color = get_sleighted_color_test(player, card, 1<<color);

	target_definition_t td1;
	counterspell_target_definition(player, card, &td1, TYPE_ANY);
	td1.required_color = get_sleighted_color_test(player, card, 1<<color);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( counterspell(player, card, event, &td1, 0)  ){
			return 99;
		}
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, get_sleighted_color_text(player, card, "Select target %s permanent.", color), 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = 0;
		if( counterspell(player, card, EVENT_CAN_CAST, &td1, 0)  ){
			instance->info_slot = 66;
			return counterspell(player, card, event, &td1, 0);
		}
		else{
			generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, get_sleighted_color_text(player, card, "Select target %s permanent.", color), 1, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = 67;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int good = 1*(instance->info_slot == 66 && counterspell_validate(player, card, &td1, 0)) |
				   2*(instance->info_slot == 67 && valid_target(&td));
		if( good ){
			if( good & 1 ){
				real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			}
			if( good & 2 ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int lace(int player, int card, event_t event, int color_test){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	if( player == AI ){
		td.illegal_color = color_test;
	}

	target_definition_t td1;
	counterspell_target_definition(player, card, &td1, TYPE_ANY);
	if( player == AI ){
		td1.illegal_color = color_test;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = generic_spell(player, card, event, GS_COUNTERSPELL, &td1, NULL, 1, NULL);
		return result ? result : generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = generic_spell(player, card, EVENT_CAN_CAST, GS_COUNTERSPELL, &td1, NULL, 1, NULL);
		if( result ){
			instance->info_slot = 66;
			return generic_spell(player, card, event, GS_COUNTERSPELL, &td1, NULL, 1, NULL);
		}
		else{
			generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
			if( spell_fizzled != 1 ){
				instance->info_slot = 67;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( (instance->info_slot = 67 && valid_target(&td)) || (instance->info_slot = 66 && counterspell_validate(player, card, &td1, 0)) ){
			change_color(player, card, instance->targets[0].player, instance->targets[0].card, get_sleighted_color_test(player, card, color_test), CHANGE_COLOR_SET);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int dmg_source_color = 0;
static const char* colored_damage_source(int who_chooses, int player, int card){
	card_instance_t* dmg = get_card_instance(player, card);
	if ( dmg->internal_card_id == damage_card ){
		int clr = dmg->initial_color;
		if( in_play(dmg->damage_source_player, dmg->damage_source_card) ){
			clr = get_color(dmg->damage_source_player, dmg->damage_source_card);
		}
		if( clr & dmg_source_color ){
			return NULL;
		}
	}

	return EXE_STR(0x739060);//",color"
}

static int cop(int player, int card, event_t event, int color_test){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_EFFECT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION | TARGET_SPECIAL_DAMAGE_PLAYER;
	td.extra = (int32_t)colored_damage_source;
	dmg_source_color = get_sleighted_color_test(player, card, color_test);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI && (get_deck_color(player, 1-player) & color_test) ){
			ai_modifier+=25;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->info_slot = 0;
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_X(1), 0, &td, "TARGET_DAMAGE");
}

int cursed_permanent(int player, int card, event_t event, int dmg, target_definition_t *td, const char *prompt){
	card_instance_t* instance = get_card_instance(player, card);

	if ( instance->damage_target_player > -1 ){
		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			damage_player(instance->damage_target_player, dmg, player, card);
		}
		upkeep_trigger_ability(player, card, event, instance->damage_target_player);
	}

	return targeted_aura(player, card, event, td, prompt);
}

void landhome(int player, int card, event_t event, subtype_t subtype){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) ){
			if( ! check_battlefield_for_subtype(1-player, TYPE_PERMANENT, get_hacked_subtype(player, card, subtype)) ){
				event_result = 1;
			}
		}
		if( event == EVENT_ABILITIES && affect_me(player, card) ){
			if( ! check_battlefield_for_subtype(player, TYPE_PERMANENT, get_hacked_subtype(player, card, subtype)) ){
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}
}

int basic_landtype_killer(int player, int card, event_t event, int basic_land_type){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=5*(count_subtype(1-player, TYPE_PERMANENT, basic_land_type)-count_subtype(player, TYPE_PERMANENT, basic_land_type));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		default_test_definition(&test, TYPE_PERMANENT);
		test.subtype = get_hacked_subtype(player, card, basic_land_type);
		new_manipulate_all(player, card, ANYBODY, &test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static const char* can_be_untapped(int who_chooses, int player, int card)
{
	if( ! check_special_flags2(player, card, SF2_COULD_NOT_UNTAP) ){
		return NULL;
	}
	return "this permanent couldn't be untapped";
}

void untap_only_one_permanent_type(int player, int card, event_t event, int affected_player, int type){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && (current_turn == affected_player || affected_player == ANYBODY) &&
		(is_what(player, card, TYPE_EFFECT) || ! is_humiliated(player, card))
	  ){
		target_definition_t td;
		default_target_definition(player, card, &td, type);
		td.illegal_type = TYPE_ENCHANTMENT | TARGET_TYPE_PLANESWALKER;
		td.allowed_controller = current_turn;
		td.preferred_controller = current_turn;
		td.who_chooses = current_turn;
		td.required_state = TARGET_STATE_TAPPED;
		td.illegal_abilities = 0;
		td.allow_cancel = 0;
		td.extra = (int32_t)can_be_untapped;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

		char buffer[100];
		int pos = scnprintf(buffer, 100, "Select a");
		if( type == TYPE_LAND ){
			pos+=scnprintf(buffer+pos, 100-pos, " land");
		}
		if( type == TYPE_CREATURE ){
			pos+=scnprintf(buffer+pos, 100-pos, " creature");
		}
		if( type == TYPE_ARTIFACT ){
			pos+=scnprintf(buffer+pos, 100-pos, "n artifact");
		}
		pos+=scnprintf(buffer+pos, 100-pos, " to untap.");

		while( instance->info_slot < 1 && can_target(&td) ){
				if( select_target(player, card, &td, buffer, &(instance->targets[0])) ){
					instance->number_of_targets = 1;
					untap_card(instance->targets[0].player, instance->targets[0].card);
					instance->info_slot++;
				}
		}

		if( is_what(affected_card_controller, affected_card, type) ){
			get_card_instance(affected_card_controller, affected_card)->untap_status &= ~3;
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}
}

// Cards

// Air Elemental --> vanilla

int card_ancestral_recall(int player, int card, event_t event){

	/* Ancestral Recall	|U
	 * Instant
	 * Target player draws three cards. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			draw_cards(get_card_instance(player, card)->targets[0].player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_animate_artifact(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = player;

	return generic_animating_aura(player, card, event, &td, "TARGET_ARTIFACT", -1, -1, 0, 0);
}

int card_animate_dead(int player, int card, event_t event){
	// 0x4B5780
	// See also card_dance_of_the_dead() in ice_age.c and card_necromancy() in visions.c

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && instance->damage_target_player != -1 ){
		modify_pt_and_abilities(instance->damage_target_player, instance->damage_target_card, event, -1, 0, 0);

		if( leaves_play(player, card, event) ){
			kill_card(instance->damage_target_player, instance->damage_target_card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_CAN_CAST && any_in_graveyard_by_type(ANYBODY, TYPE_CREATURE) ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card.");

		select_target_from_either_grave(player, card, 0, AI_MAX_CMC, AI_MAX_CMC, &this_test, 0, 1);
	}
	if( event == EVENT_RESOLVE_SPELL){
		/* real_put_into_play removes STATE_INVISIBLE just after sending EVENT_RESOLVE_SPELL instead of just before; remove it now, so in_play() works right in
		 * any come-into-play effects.  (This might be a more reliable way of determining whether a card was played from hand.) */
		instance->state &= ~STATE_INVISIBLE;

		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_ATTACH_AS_AURA);
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int card_animate_wall(int player, int card, int event)
{
  // 0x4b63a0
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_ABILITIES
	  && affect_me(instance->damage_target_player, instance->damage_target_card)
	  && in_play(player, card) && in_play(affected_card_controller, affected_card)
	  && !is_humiliated(player, card))
	add_status(affected_card_controller, affected_card, STATUS_WALL_CAN_ATTACK);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;
  td.required_subtype = SUBTYPE_WALL;

  return targeted_aura(player, card, event, &td, "TARGET_WALL");
}

int card_ankh_of_mishra(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if( in_play(player, card) && specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		damage_player(instance->targets[1].player, 2, player, card);
	}

	return 0;
}

int card_armageddon(int player, int card, event_t event){
	// to insert

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		new_manipulate_all(player, card, ANYBODY, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_aspect_of_the_wolf(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
	}

	if( instance->damage_target_player > -1 && ! is_humiliated(player, card) ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		int total = count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_FOREST));
		if( event == EVENT_POWER && affect_me(p, c) ){
			event_result += (total-1)/2;
		}
		if( event == EVENT_TOUGHNESS && affect_me(p, c) ){
			event_result += (total+1)/2;
		}
		return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_AURA, &td, "TARGET_CREATURE", 1, NULL);
}

int card_bad_moon(int player, int card, event_t event){

	boost_creature_by_color(player, card, event, COLOR_TEST_BLACK, 1, 1, 0, 0);

	return global_enchantment(player, card, event);

}

// Badlands --> better leave it hardcoded for now.

int card_balance(int player, int card, event_t event){
	// to insert

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=2*(hand_count[1-player]-hand_count[player]);
		ai_modifier+=2*(count_subtype(1-player, TYPE_CREATURE, -1)-count_subtype(player, TYPE_CREATURE, -1));
		ai_modifier+=2*(count_subtype(1-player, TYPE_LAND, -1)-count_subtype(player, TYPE_LAND, -1));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int diff[2][3];
		int i;
		for(i=0; i<2; i++){
			diff[i][0] = count_subtype(i, TYPE_LAND, -1)-count_subtype(1-i, TYPE_LAND, -1);
		}
		if( diff[0][0] != 0 ){
			int sac_player = diff[0][0] > diff[1][0] ? 0 : 1;
			impose_sacrifice(player, card, sac_player, diff[sac_player][0], TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		for(i=0; i<2; i++){
			diff[i][1] = hand_count[i]-hand_count[1-i];
		}
		if( diff[0][1] != 0 ){
			int d_player = diff[0][1] > diff[1][1] ? 0 : 1;
			new_multidiscard(d_player, diff[d_player][1], 0, player);
		}
		for(i=0; i<2; i++){
			diff[i][2] = count_subtype(i, TYPE_CREATURE, -1)-count_subtype(1-i, TYPE_CREATURE, -1);
		}
		if( diff[0][2] != 0 ){
			int sac_player = diff[0][2] > diff[1][2] ? 0 : 1;
			impose_sacrifice(player, card, sac_player, diff[sac_player][2], TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_basalt_monolith(int player, int card, event_t event){
	return monolith(player, card, event, 3);
}

// Bayou -> hardcoded

// Benalish Hero --> vanilla

int effect_berserk(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 && instance->damage_target_card > -1 ){
		if( is_attacking(instance->damage_target_player, instance->damage_target_card) ){
			instance->targets[1].player = 66;
		}
	}

	if( instance->targets[1].player == 66 ){
		if( trigger_condition == TRIGGER_EOT && affect_me(player, card) && reason_for_trigger_controller == player ){
			remove_status(player, card, STATUS_INVISIBLE_FX);
			if( eot_trigger(player, card, event) ){
				kill_card(instance->damage_target_player, instance->damage_target_card, KILL_DESTROY);
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}
	else{
		if( event == EVENT_CLEANUP ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_berserk(int player, int card, event_t event){
	// to insert
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && !(current_phase <= PHASE_AFTER_BLOCKING) ){
		return 0;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									get_power(instance->targets[0].player, instance->targets[0].card), 0, KEYWORD_TRAMPLE, 0);
			int legacy = create_targetted_legacy_effect(player, card, &effect_berserk, instance->targets[0].player, instance->targets[0].card);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

// Birds of Paradise --> generic_noncombat_1_mana_producing_creature

int card_black_knight(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		protection_from_white(player, card, event);
	}
	return 0;
}

int card_black_lotus(int player, int card, event_t event){
	//0x420020
	return artifact_mana_all_one_color(player, card, event, 3, 1);
}

int card_black_vise(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=(5*(hand_count[1-player]-4));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		get_card_instance(player, card)->targets[0].player = 1-player;
	}

	if( get_card_instance(player, card)->targets[0].player > -1 ){
		upkeep_trigger_ability(player, card, event, get_card_instance(player, card)->targets[0].player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int af_player = get_card_instance(player, card)->targets[0].player;
			int amount = hand_count[af_player]-4;
			if( amount > 0 ){
				damage_player(af_player, amount, player, card);
			}
		}
	}

	return 0;
}

int card_black_ward(int player, int card, event_t event){
	return ward(player, card, event, KEYWORD_PROT_BLACK);
}

// blaze of glory --> Korath, please do this

int card_blessing(int player, int card, event_t event)
{
  // 0x4b9d20.  Since the function at 0x4af110 checks for it by address, 0x4b9d20 jumps here; don't change the address in ct_all.csv to this.

  /* Blessing	|W|W
   * Enchantment - Aura
   * Enchant creature
   * |W: Enchanted creature gets +1/+1 until end of turn. */

  if (event == EVENT_SHOULD_AI_PLAY)
	// Originally +/- (24 + 12*lands), and didn't check controller of enchanted creature.
	ai_modifier += (get_card_instance(player, card)->damage_target_player != player) ? 0 : (player == AI ? 12 : -12) * landsofcolor_controlled[player][COLOR_WHITE];

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (in_play(instance->damage_target_player, instance->damage_target_card))
		pump_until_eot_merge_previous(player, card, instance->damage_target_player, instance->damage_target_card, 1,1);
	}

  return vanilla_aura(player, card, event, player) || generic_activated_ability(player, card, event, 0, MANACOST_W(1), 0, NULL, NULL);
}

int card_blue_elemental_blast(int player, int card, event_t event){
	return blast(player, card, event, COLOR_RED);
}

int card_blue_ward(int player, int card, event_t event){
	return ward(player, card, event, KEYWORD_PROT_BLUE);
}

// Bog Wraith --> vanilla

int card_braingeyser(int player, int card, event_t event){
	// Original code : 0x41bb00
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			draw_cards(instance->targets[0].player, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_PLAYER", 1, NULL);
}

int card_burrowing(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 0, 0, get_hacked_walk(player, card, KEYWORD_MOUNTAINWALK), 0, 0, 0, 0);
}

// Camouflage --> leave it hardcoded for now

int card_castle(int player, int card, event_t event){
	if( in_play(player, card) && event == EVENT_TOUGHNESS && affected_card_controller == player && ! is_humiliated(player, card) &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_tapped(affected_card_controller, affected_card)
	  ){
		event_result+=2;
	}
	return global_enchantment(player, card, event);
}

// Celestial prism --> Korath, better if you do this

int channel_legacy(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		return can_pay_life(player, 1);
	}

	if( event == EVENT_ACTIVATE ){
		lose_life(player, 1);
		produce_mana(player, COLOR_COLORLESS, 1);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_pay_life(player, 1) ){
		declare_mana_available(player, COLOR_COLORLESS, 1);
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_channel(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && player == AI ){
		ai_modifier+=(2*(life[player]-10));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_activate(player, card, &channel_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_chaos_orb(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( in_play(player, instance->parent_card) ){
			int p = 1-player;
			int roll = internal_rand(100);
			if( roll < 5 ){
				p = player;
				roll+=50;
				roll+=internal_rand(50);
			}
			int killed = 0;
			int k;
			for(k=50; k<100; k+=10){
				if( roll > k ){
					killed++;
				}
			}
			if( killed > 0 ){
				int p_array[100];
				int pa_count = 0;
				for(k=0; k<active_cards_count[p]; k++){
					if( in_play(p, k) && is_what(p, k, TYPE_PERMANENT) ){
						p_array[pa_count] = k;
						pa_count++;
						if( pa_count > 99 ){
							break;
						}
					}
				}
				int rnd = internal_rand(pa_count);
				while( killed > 0 && rnd < pa_count ){
						if( ! is_token(p, p_array[rnd]) ){
							kill_card(p, p_array[rnd], KILL_DESTROY);
						}
						killed--;
				}
			}
			kill_card(player, instance->parent_card, KILL_DESTROY);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_chaoslace(int player, int card, event_t event){
	return lace(player, card, event, COLOR_TEST_RED);
}

int card_circle_of_protection_black(int player, int card, event_t event){
	return cop(player, card, event, COLOR_TEST_BLACK);
}

int card_circle_of_protection_blue(int player, int card, event_t event){
	return cop(player, card, event, COLOR_TEST_BLUE);
}

int card_circle_of_protection_green(int player, int card, event_t event){
	return cop(player, card, event, COLOR_TEST_GREEN);
}

int card_circle_of_protection_red(int player, int card, event_t event){
	return cop(player, card, event, COLOR_TEST_RED);
}

int card_circle_of_protection_white(int player, int card, event_t event){
	return cop(player, card, event, COLOR_TEST_WHITE);
}

int card_clockwork_beast(int player, int card, event_t event){
	// 0x4265e0
	return hom_clockwork(player, card, event, 7);
}

int card_clone(int player, int card, event_t event)
{
  // 0x451cf0

  /* Clone	|3|U
   * Creature - Shapeshifter 0/0
   * You may have ~ enter the battlefield as a copy of any creature on the battlefield. */

  enters_the_battlefield_as_copy_of_any_creature(player, card, event);

  return 0;
}

/* Cockatrice	|3|G|G => Thicket Basilisk
 * Creature - Cockatrice 2/4
 * Flying
 * Whenever ~ blocks or becomes blocked by a non-Wall creature, destroy that creature at end of combat. */

int card_consecrated_land(int player, int card, event_t event){

	if( get_card_instance(player, card)->damage_target_player > -1 && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		cannot_be_enchanted_granted(player, card, event, instance->damage_target_player, instance->damage_target_card);
		if( event == EVENT_ABILITIES && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			special_abilities(instance->damage_target_player, instance->damage_target_card, event, SP_KEYWORD_INDESTRUCTIBLE, player, card);
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_conservator(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int value = get_card_instance(instance->targets[0].player, instance->targets[0].card)->info_slot;
			get_card_instance(instance->targets[0].player, instance->targets[0].card)->info_slot = value < 2 ? 0 : value-2;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_DAMAGE");
}

// contract from below --> hardcoded, for now

int card_control_magic(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	return generic_stealing_aura(player, card, event, &td, "TARGET_CREATURE");
}

int card_conversion2(int player, int card, event_t event){
	basic_upkeep(player, card, event, 0, 0, 0, 0, 0, 2);
	change_lands_into_new_land_type(player, card, event, SUBTYPE_MOUNTAIN, MATCH, SUBTYPE_PLAINS);
	return global_enchantment(player, card, event);
}

int card_copper_tablet(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=5*(life[player]-life[1-player]);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(current_turn, 1, player, card);
	}
	upkeep_trigger_ability(player, card, event, ANYBODY);
	return 0;
}

int card_copy_artifact(int player, int card, event_t event)
{
  /* Copy Artifact	|1|U
   * Enchantment
   * You may have ~ enter the battlefield as a copy of any artifact on the battlefield, except it's an enchantment in addition to its other types. */

  target_definition_t td;
  if (event == EVENT_RESOLVE_SPELL)
	base_target_definition(player, card, &td, TYPE_ARTIFACT);

  enters_the_battlefield_as_copy_of_any(player, card, event, &td, "SELECT_AN_ARTIFACT");
  // type enchantment added in cloning()

  return global_enchantment(player, card, event);
}

int card_counterspell(int player, int card, event_t event){
	return counterspell(player, card, event, NULL, 0);
}

int card_craw_wurm(int player, int card, event_t event ){
	// original code : 00401000
	// This hack was used for the Wurm token when we had the 2000 card limit, not it's useless. This code pointer could be recycled.
	if( get_special_infos(player, card) == 66 ){
		modify_pt_and_abilities(player, card, event, 0, 2, 0);
	}

	return 0;
}

int card_creature_bond(int player, int card, event_t event){
	card_instance_t* instance = get_card_instance(player, card);

	if ( instance->damage_target_player > -1 && ! is_humiliated(player, card)){
		if( graveyard_from_play(instance->damage_target_player, instance->damage_target_card, event) ){
			damage_player(instance->damage_target_player, get_toughness(instance->damage_target_player, instance->damage_target_card), player, card);
		}
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

int card_crusade(int player, int card, event_t event){
	boost_creature_by_color(player, card, event, get_sleighted_color_test(player, card, COLOR_TEST_WHITE), 1, 1, 0, 0);
	return global_enchantment(player, card, event);
}

int card_crystal_rod(int player, int card, event_t event){
	return lifegaining_charm(player, card, event, 2, COLOR_TEST_BLUE, 1+player, 1);
}

int card_cursed_land(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	return cursed_permanent(player, card, event, 1, &td, "TARGET_LAND");
}

int cyclopean_tomb_effect(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_CHANGE_TYPE && affect_me(p, c) && count_counters(p, c, COUNTER_MIRE)){
			int lt = SUBTYPE_SWAMP;
			if( in_play(instance->targets[1].player, instance->targets[1].card) ){
				lt = get_hacked_subtype(instance->targets[1].player, instance->targets[1].card, SUBTYPE_SWAMP);
			}
			event_result = land_into_new_basic_land_type(event_result, lt);
		}
	}

	return 0;
}

int cyclopean_tomb_legacy(int player, int card, event_t event){
	if( current_turn == player ){
		int amount = upkeep_trigger(player, card, event);
		if( amount ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_PERMANENT);
			td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
			td.extra = COUNTER_MIRE;
			td.illegal_abilities = 0;
			td.allow_cancel = 0;

			card_instance_t *instance = get_card_instance(player, card);

			while( amount && can_target(&td)){
					if( select_target(player, card, &td, "Select a permanent with Mire counters", &(instance->targets[0])) ){
						instance->number_of_targets = 1;
						remove_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_MIRE,
										count_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_MIRE));
					}
					amount--;
			}
		}
	}
	return 0;
}

int card_cyclopean_tomb(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		create_legacy_effect(player, card, &cyclopean_tomb_legacy);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_SWAMP;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_MIRE);
			int legacy = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &cyclopean_tomb_effect,
														instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->token_status |= STATUS_INVISIBLE_FX;
			leg->targets[1].player = instance->parent_controller;
			leg->targets[1].card = instance->parent_card;
			leg->number_of_targets = 2;
		}
	}

	return generic_activated_ability(player, card, event, GAA_IN_YOUR_TURN | GAA_ONLY_ON_UPKEEP | GAA_CAN_TARGET | GAA_UNTAPPED | GAA_LITERAL_PROMPT,
									MANACOST_X(2), 0, &td, "Select target non-Swamp land.");
}

int card_dark_ritual(int player, int card, event_t event){
	// just the bare skeleton, probably needs AI hint, Korath please do it
	if( event == EVENT_CAN_CAST ){
		if( basic_spell(player, card, event) ){
			if( card_on_stack_controller > -1 && card_on_stack > -1 ){
				return 99;
			}
			return 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		produce_mana(player, COLOR_BLACK, 3);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// Darkpact --> hardcoded, for now

int card_death_ward(int player, int card, event_t event){
	// 0x004A3810

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_DESTROYED;
	td.preferred_controller = player;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET+GS_REGENERATION, &td, "TARGET_CREATURE", 1, NULL);
}

int card_deathgrip(int player, int card, event_t event ){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=5*count_subtype(1-player, TYPE_PERMANENT, SUBTYPE_FOREST);
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ANY);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if (counterspell_validate(player, card, &td, 0)){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK, MANACOST_B(2), 0, &td, NULL);
}

int card_deathlace(int player, int card, event_t event ){
	return lace(player, card, event, COLOR_TEST_BLACK);
}

// demonic attorney --> hardcoded for now

int card_demonic_hordes(int player, int card, event_t event)
{
  /* Demonic Hordes	|3|B|B|B
   * Creature - Demon 5/5
   * |T: Destroy target land.
   * At the beginning of your upkeep, unless you pay |B|B|B, tap ~ and sacrifice a land of an opponent's choice. */

  if (basic_upkeep_unpaid(player, card, event, MANACOST_B(3)))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  target_definition_t td;
	  base_target_definition(player, card, &td, TYPE_LAND);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.who_chooses = 1-player;
	  td.allow_cancel = 0;

	  tap_card(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "SACRIFICE_LAND"))
		{
		  instance->number_of_targets = 0;
		  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		}
	}

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);

  card_instance_t* instance = get_card_instance(player, card);

  if(event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_LAND");
}

int card_demonic_tutor(int player, int card, event_t event ){
	// original code : 0041D7C0

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=25;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select a card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_dingus_egg(int player, int card, event_t event ){

	if( ! is_humiliated(player, card) ){
		card_instance_t *instance= get_card_instance(player, card);
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
			if( is_what(affected_card_controller, affected_card, TYPE_LAND) ){
				card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
				if( affected->kill_code > 0 && affected->kill_code < 4 ){
					if( instance->targets[affected_card_controller].player < 0 ){
						instance->targets[affected_card_controller].player = 0;
					}
					instance->targets[affected_card_controller].player+=2;
				}
			}
		}

		if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
			affect_me(player, card ) && (instance->targets[0].player > 0 || instance->targets[1].player > 0)
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( instance->targets[player].player > 0 ){
						damage_player(player, instance->targets[player].player, player, card);
					}
					if( instance->targets[1-player].player > 0 ){
						damage_player(1-player, instance->targets[1-player].player, player, card);
					}
					instance->targets[0].player = instance->targets[1].player = 0;
			}
			else if (event == EVENT_END_TRIGGER){
					instance->targets[0].player = instance->targets[1].player = 0;
			}
		}
	}

	return 0;
}

int card_disenchant(int player, int card, event_t event ){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "DISENCHANT", 1, NULL);
}

int card_disintegrate(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( instance->targets[0].card != -1 ){
				cannot_regenerate_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
				exile_if_would_be_put_into_graveyard(player, card, instance->targets[0].player, instance->targets[0].card, 1);
			}
			damage_target0(player, card, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_disrupting_scepter(int player, int card, event_t event)
{
  /* Disrupting Scepter	|3
   * Artifact
   * |3, |T: Target player discards a card. Activate this ability only during your turn. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	discard(get_card_instance(player, card)->targets[0].player, 0, player);

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_IN_YOUR_TURN, MANACOST_X(3), 0, &td, "TARGET_PLAYER");
}

int card_dragon_whelp(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION || event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST ){
		return generic_shade(player, card, event, 0, MANACOST_R(1), 1, 0, 0, 0);
	}

	if( event == EVENT_ACTIVATE ){
		generic_activated_ability(player, card, event, 0, MANACOST_R(1), 0, NULL, NULL);
		if( spell_fizzled != 1 ){
			instance->info_slot++;
		}
	}

	if( eot_trigger(player, card, event) ){
		if( instance->info_slot > 3 && ! check_state(player, card, STATE_OUBLIETTED) && ! is_humiliated(player, card) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
		instance->info_slot = 0;
	}

	return 0;
}

int drain_life_legacy(int player, int card, event_t event){
	card_instance_t* instance = get_card_instance(player, card);
	if (event == EVENT_DEAL_DAMAGE){
		card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
		if (damage->internal_card_id == damage_card && damage->display_pic_csv_id == CARD_ID_DRAIN_LIFE
			&& damage->damage_target_card == instance->targets[0].card
			&& damage->damage_target_player == instance->targets[0].player
			&& damage->info_slot > 0
		   ){
			if( instance->targets[1].player < 0 ){
				instance->targets[1].player = 0;
			}
			int dd = MIN(damage->info_slot, life[damage->damage_target_player]);
			if( damage->damage_target_card != -1 ){
				dd = MIN(damage->info_slot, get_toughness(damage->damage_target_player, damage->damage_target_card));
			}
			instance->targets[1].player += dd;
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

	return 0;
}

int card_drain_life(int player, int card, event_t event){
	// Original code : 0x41E9B0

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST || event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			charge_mana(player, COLOR_BLACK, -1);
			if( spell_fizzled != 1 ){
				set_x_for_x_spells(player, card, event, instance->info_slot);
			}
		}
		if( spell_fizzled != 1 ){
			pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
		}
	}


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) && instance->info_slot > 0){
			int legacy = create_legacy_effect(player, card, &drain_life_legacy);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[0].player = instance->targets[0].player;
			leg->targets[0].card = instance->targets[0].card;
			damage_target0(player, card, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_drain_power(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int count;
			for( count = 0; count < active_cards_count[instance->targets[0].player]; count++){
				if( in_play(instance->targets[0].player, count) && is_what(instance->targets[0].player, count, TYPE_LAND) ){
					if( mana_producer(instance->targets[0].player, count, EVENT_CAN_ACTIVATE) ){
						call_card_function(instance->targets[0].player, count, EVENT_ACTIVATE);
					}
				}
			}
			int i;
			for (i = COLOR_COLORLESS; i <= COLOR_WHITE; ++i){
				int amount = mana_pool[instance->targets[0].player][i];
				mana_pool[instance->targets[0].player][i] = 0;
				produce_mana(player, i, amount);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_drudge_skeletons(int player, int card, event_t event){
	return regeneration(player, card, event, MANACOST_B(1));
}

int card_dwarven_demolition_team(int player, int card, event_t event)
{

	if (!IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_WALL;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_WALL");
}

int card_dwarven_warriors(int player, int card, event_t event)
{

	if (!IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

// Earth Elemental --> vanilla

int card_earthbind(int player, int card, int event){

  card_instance_t* instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	if( player == AI ){
		td.required_abilities = KEYWORD_FLYING;
	}

	if( event == EVENT_RESOLVE_SPELL && valid_target(&td) ){
		if( check_for_ability(instance->targets[0].player, instance->targets[0].card, KEYWORD_FLYING) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
			instance->targets[1].player = 66;
		}
	}

	if( instance->damage_target_player > -1 && instance->targets[1].player == 66 && ! is_humiliated(player, card) ){
		if (event == EVENT_ABILITIES){
			if (affect_me(instance->damage_target_player, instance->damage_target_card)){
				if( event_result & KEYWORD_FLYING ){
					event_result &= ~KEYWORD_FLYING;
				}
			}
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}


int card_earthquake(int player, int card, event_t event){
	// Original code : 0x41C7E0

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t* instance = get_card_instance(player, card);
		if( instance->info_slot > 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.keyword = KEYWORD_FLYING;
			this_test.keyword_flag = 1;
			new_damage_all(player, card, ANYBODY, instance->info_slot, NDA_PLAYER_TOO, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 1, NULL);
}

// Elvish Archer --> vanilla

int card_evil_presence(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	if( player == AI ){
		td.required_subtype = SUBTYPE_SWAMP;
		td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
	}

	card_instance_t* instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_CHANGE_TYPE && affect_me(p, c) ){
			event_result = land_into_new_basic_land_type(event_result, get_hacked_subtype(player, card, SUBTYPE_SWAMP));
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_false_orders(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_BLOCKING;

	card_instance_t* instance = get_card_instance(player, card);

	if( (event == EVENT_CAN_CAST && current_phase == PHASE_AFTER_BLOCKING) ||
		(event == EVENT_CAST_SPELL && affect_me(player, card)) ||
		event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CHANGE_TARGET
	  ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			remove_state(instance->targets[0].player, instance->targets[0].card, STATE_BLOCKING);
			remove_state(instance->targets[0].player, instance->targets[0].card, STATE_UNKNOWN8000);

			target_definition_t td1;
			default_target_definition(instance->targets[0].player, instance->targets[0].card, &td1, TYPE_CREATURE);
			td.required_state = TARGET_STATE_ATTACKING;

			if( select_target(player, card, &td1, "Select a new creature to block", &(instance->targets[1])) ){
				block(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_farmstead(int player, int card, event_t event){
  /* Farmstead	|W|W|W
   * Enchantment - Aura
   * Enchant land
   * Enchanted land has "At the beginning of your upkeep, you may pay |W|W. If you do, you gain 1 life." */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 && ! is_humiliated(player, card) ){
		int p = instance->damage_target_player;

		upkeep_trigger_ability_mode(player, card, event, player, has_mana(p, COLOR_WHITE, 2) ? RESOLVE_TRIGGER_AI(player) : 0);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			charge_mana(p, COLOR_WHITE, 2);
			if( spell_fizzled != 1 ){
				gain_life(p, 1);
			}
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_fastbond(int player, int card, event_t event)
{
  // 0x4316C0

  /* Fastbond	|G
   * Enchantment
   * You may play any number of additional lands on each of your turns.
   * Whenever you play a land, if it wasn't the first land you played this turn, ~ deals 1 damage to you. */

   if( ! is_humiliated(player, card) ){
		if (player == current_turn && (land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED) && current_phase >= PHASE_MAIN1 && current_phase <= PHASE_MAIN2
			&& in_play(player, card)
		   ){
			land_can_be_played &= ~LCBP_LAND_HAS_BEEN_PLAYED;
		}

		if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player
			&& trigger_cause_controller == player
			&& lands_played >= 1
			&& is_what(trigger_cause_controller, trigger_cause, TYPE_LAND)
			&& ! check_special_flags(trigger_cause_controller, trigger_cause, SF_NOT_CAST) //Bad name in this case, but it's the one used by "put_into_play"
			)
		{
			if (event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			if (event == EVENT_RESOLVE_TRIGGER){
				damage_player(player, 1, player, card);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_fear(int player, int card, event_t event)
{
  return generic_aura(player, card, event, player, 0, 0, 0, SP_KEYWORD_FEAR, 0, 0, 0);
}

int card_feedback(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
	return cursed_permanent(player, card, event, 1, &td, "TARGET_ENCHANTMENT");
}

int card_fire_elemental(int player, int card, event_t event ){
	// original code : 00401000
	// This hack was used for the Elemental token when we had the 2000 card limit, not it's useless. This code pointer could be recycled.

	if( get_special_infos(player, card) > 0 ){
		int amount = get_special_infos(player, card);
		if( event == EVENT_POWER && affect_me(player, card)  ){
			event_result+=(amount-5);
		}
		if( event == EVENT_TOUGHNESS && affect_me(player, card)  ){
			event_result+=(amount-4);
		}
	}

	return 0;
}

int card_fireball(int player, int card, event_t event){
	// Until the special window used for paying the manacost is decoded, let's leave it in this way.
	return generic_x_spell_recoded(player, card, event, card_fireball_exe(player, card, event));
}

int card_firebreathing(int player, int card, event_t event){
	/* Firebreathing	|R
	 * Enchantment - Aura
	 * Enchant creature
	 * |R: Enchanted creature gets +1/+0 until end of turn. */

	if (IS_GAA_EVENT(event)){
		card_instance_t* instance = get_card_instance(player, card);

		if( instance->damage_target_player > -1 ){
			if( event == EVENT_RESOLVE_ACTIVATION ){
				pump_until_eot_merge_previous(player, card,
											  instance->damage_target_player, instance->damage_target_card, 1, 0);
			}
			return generic_activated_ability(player, card, event, 0, MANACOST_R(1), 0, NULL, NULL);
		}
	}

	return vanilla_aura(player, card, event, player);
}

int card_flashfires(int player, int card, event_t event){
	return basic_landtype_killer(player, card, event, SUBTYPE_PLAINS);
}

int card_flight2(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 0, 0, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_fog(int player, int card, event_t event){
	// original code : 004A3700
	// also code for Holy Day and Darkness
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( current_turn == player || current_phase != PHASE_AFTER_BLOCKING || count_attackers(current_turn) < 1 ){
			ai_modifier-=25;
		}
	}


	if( event == EVENT_RESOLVE_SPELL ){
		fog_effect(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_force_of_nature(int player, int card, event_t event){
	if( ! is_humiliated(player, card) && basic_upkeep_unpaid(player, card, event, MANACOST_G(4)) ){
		damage_player(player, 8, player, card);
	}

	return 0;
}

static const char* combat_damage_from_an_unblocked_creature(int who_chooses, int player, int card)
{
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->internal_card_id == damage_card && (instance->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE)) ){
		if( is_what(instance->damage_source_player, instance->damage_source_card, TYPE_CREATURE) &&
			check_state(instance->damage_source_player, instance->damage_source_card, STATE_ATTACKING) &&
			is_unblocked(instance->damage_source_player, instance->damage_source_card)
		  ){
			return NULL;
		}
	}
	return "must be combat damage from an unblocked creature";
}

int card_forcefield(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_EFFECT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)combat_damage_from_an_unblocked_creature;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( dmg->info_slot > 1 ){
				dmg->info_slot = 1;
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, 1, 0, 0, 0, 0, 0, 0, &td, "TARGET_DAMAGE");
}

// forest --> better leave it hardcoded

int card_fork(int player, int card, event_t event)
{
  // original code : 0x4A28B0

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_SET_COLOR
	  && instance->targets[2].card != -1
	  && affect_me(player, instance->targets[2].card))
	event_result = COLOR_TEST_RED;

  int rval = twincast(player, card, event, NULL, &instance->targets[2].card);

  if (event == EVENT_RESOLVE_SPELL)
	kill_card(player, card, KILL_DESTROY);

  return rval;
}

// Frozen Shade --> Nantuko Shade

int card_fungusaur(int player, int card, event_t event){
  // 0x40aff0

  // Whenever ~ is dealt damage, put a +1/+1 counter on it.

	card_instance_t* damage = damage_being_dealt(event);
	if (damage && damage->damage_target_player == player && damage->damage_target_card == card)
		get_card_instance(player, card)->info_slot = 1;

	card_instance_t* instance = get_card_instance(player, card);
	if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player
		&& instance->info_slot && !instance->eot_toughness)
		{
		  if (event == EVENT_TRIGGER)
			event_result |= RESOLVE_TRIGGER_MANDATORY;

		  if (event == EVENT_RESOLVE_TRIGGER)
			instance->eot_toughness = 1;
		}

	if( ! is_humiliated(player, card) ){
	  if ((event == EVENT_AFTER_DAMAGE || event == EVENT_SHOULD_AI_PLAY)
		  && instance->eot_toughness == 1)
		{
		  add_1_1_counter(player, card);
		  instance->info_slot = instance->eot_toughness = 0;
		}
	}

  return 0;
}

int gaeas_liege_effect(int player, int card, event_t event){
	if (event == EVENT_CHANGE_TYPE){
		card_instance_t* instance = get_card_instance(player, card);
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if (affect_me(p, c)){
			event_result = land_into_new_basic_land_type(event_result, get_hacked_subtype(player, card, SUBTYPE_FOREST));
		}
	}
	if (event == EVENT_STATIC_EFFECTS){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->targets[1].player >= 0 && !in_play(instance->targets[1].player, instance->targets[1].card)){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int card_gaeas_liege(int player, int card, event_t event){
	/* Gaea's Liege	|3|G|G|G
	 * Creature - Avatar 100/100
	 * As long as ~ isn't attacking, its power and toughness are each equal to the number of |H1Forests you control. As long as ~ is attacking, its power and toughness are each equal to the number of |H1Forests defending player controls.
	 * |T: Target land becomes |Ha Forest until ~ leaves the battlefield. */

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && player != -1){
		event_result += count_subtype(card != -1 && is_attacking(player, card) ? 1-player : player,
									  TYPE_PERMANENT, get_hacked_subtype(player, card, SUBTYPE_FOREST));
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	if( IS_AI(player) && player == AI ){
		td.required_subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int legacy = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &gaeas_liege_effect,
														instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].player = instance->parent_controller;
			leg->targets[1].card = instance->parent_card;
			leg->number_of_targets = 2;
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td, "TARGET_LAND");
}

int card_gauntlet_of_might(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		if ((event == EVENT_COUNT_MANA || event == EVENT_TAP_CARD) && is_what(affected_card_controller, affected_card, TYPE_LAND)
			&& has_subtype(affected_card_controller, affected_card, get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN))){
			// See comments in card_mana_flare().

			if (!in_play(player, card)){
				return 0;
			}

			if (event == EVENT_COUNT_MANA){
				if (is_tapped(affected_card_controller, affected_card) || is_animated_and_sick(affected_card_controller, affected_card)
					|| !can_produce_mana(affected_card_controller, affected_card) ){
					return 0;
				}

				declare_mana_available(affected_card_controller, COLOR_RED, 1);
			} else {	// event == EVENT_TAP_CARD
				if (tapped_for_mana_color >= 0){
					/* Triggers even if the land produced no mana so long as it was tapped for a mana ability (such as a Tolarian Academy that somehow became a
					 * forest too, with no artifacts in play), by analogy with the ruling for Overabundance.  Differs from Mana Flare since that "adds one
					 * mana... of any type that land produced". */
					produce_mana(affected_card_controller, COLOR_RED, 1);
				}
			}
		}

		boost_creature_by_color(player, card, event, get_sleighted_color_test(player, card, COLOR_TEST_RED), 1, 1, 0, 0);
	}

	return 0;
}

int card_giant_growth(int player, int card, event_t event){
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 3, 3, 0, 0);
}

// Giant Spider --> vanilla

int card_glasses_of_urza(int player, int card, event_t event)
{

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	reveal_target_player_hand(get_card_instance(player, card)->targets[0].player);

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_PLAYER");
}

int card_gloom2(int player, int card, event_t event){
	if( ! is_humiliated(player, card) && in_play(player, card) ){
		if( event == EVENT_MODIFY_COST_GLOBAL && ! is_what(affected_card_controller, affected_card, TYPE_LAND) &&
			(get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_WHITE))
		  ){
			COST_COLORLESS+=3;
		}

		if( event == EVENT_RESOLVE_SPELL ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ENCHANTMENT);
			this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			set_cost_mod_for_activated_abilities(ANYBODY, -1, 4, &this_test);
		}

		if( event == EVENT_CAST_SPELL && ! affect_me(player, card) ){
			if( is_what(affected_card_controller, affected_card, TYPE_ENCHANTMENT) &&
				(get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_WHITE))
			  ){
				set_cost_mod_for_activated_abilities(affected_card_controller, affected_card, 4, NULL);
			}
		}
	}

	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		remove_cost_mod_for_activated_abilities(ANYBODY, -1, 4, &this_test);
	}

	return global_enchantment(player, card, event);
}

int card_goblin_balloon_brigade(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_R(1), 0, 0, KEYWORD_FLYING, 0);
}

int card_goblin_king(int player, int card, event_t event){
	boost_creature_type(player, card, event, SUBTYPE_GOBLIN, 1, 1, get_hacked_walk(player, card, KEYWORD_MOUNTAINWALK), 0);
	return 0;
}

int card_granite_gargoyle(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_R(1), 0, 1, 0, 0);
}

int card_grey_ogre(int player, int card, event_t event ){
	// original code : 00401000

	// This was the old hack for Ogre token when we had the 2000 card limit. Now it's unused so the code pointer could be recycled
	if( get_special_infos(player, card) == 66 ){
		modify_pt_and_abilities(player, card, event, 1, 1, 0);
	}

	return 0;
}

int card_green_ward(int player, int card, event_t event){
	return ward(player, card, event, KEYWORD_PROT_GREEN);
}

int card_guardian_angel(int player, int card, event_t event){
	// I don't really know how to help the AI for this, Korath, could you do it ?
	return generic_x_spell_recoded(player, card, event, card_guardian_angel_exe(player, card, event));
}

int card_healing_salve(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, 0);
	td1.extra = damage_card;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( can_target(&td1) ){
			return card_guardian_angel_exe(player, card, event);
		}
		else{
			return can_target(&td);
		}
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int choice = 0;
			if( can_target(&td) ){
				if( can_target(&td1) ){
					choice = do_dialog(player, player, card, -1, -1, " Gain 3 life\n Prevent 3 damage\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				if( pick_target(&td, "TARGET_PLAYER") ){
					instance->info_slot = 66;
				}
			}
			else if( choice == 1 ){
					if( pick_target(&td1, "TARGET_DAMAGE") ){
						instance->info_slot = 67;
					}
			}
			else{
				spell_fizzled = 1;
			}
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( instance->info_slot == 66 && valid_target(&td) ){
				gain_life(instance->targets[0].player, 3);
			}
			if( instance->info_slot == 67 ){
				if( valid_target(&td1) ){
					card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
					if( target->info_slot <= 3 ){
						target->info_slot = 0;
					}
					else{
						target->info_slot-=3;
					}
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_helm_of_chatzuk(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);
	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_BANDING, 0);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(1), 0, &td, "TARGET_CREATURE");
}

// Hill Giant --> vanilla

int card_holy_armor(int player, int card, event_t event)
{
  // 0x4b9910.  Since the function at 0x4af110 checks for it by address, 0x4b9910 jumps here; don't change the address in ct_all.csv to this.

  /* Holy Armor	|W
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +0/+2.
   * |W: Enchanted creature gets +0/+1 until end of turn. */

  if (event == EVENT_SHOULD_AI_PLAY)
	// Originally +/- 12*lands, and didn't check controller of enchanted creature.
	ai_modifier += (get_card_instance(player, card)->damage_target_player != player) ? 0 : (player == AI ? 6 : -6) * landsofcolor_controlled[player][COLOR_WHITE];

  if (event == EVENT_TOUGHNESS)
	{
	  card_instance_t* instance = in_play(player, card);
	  if (instance && affect_me(instance->damage_target_player, instance->damage_target_card) && !is_humiliated(player, card))
		event_result += 2;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (in_play(instance->damage_target_player, instance->damage_target_card))
		pump_until_eot_merge_previous(player, card, instance->damage_target_player, instance->damage_target_card, 0,1);
	}

  return vanilla_aura(player, card, event, player) || generic_activated_ability(player, card, event, 0, MANACOST_W(1), 0, NULL, NULL);
}

int card_holy_strenght(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 1, 2, 0, 0, 0, 0, 0);
}

int card_howl_from_beyond(int player, int card, event_t event){

	// 0x4a3310

	/* Enrage	|X|R
	 * Instant
	 * Target creature gets +X/+0 until end of turn. */

	/* Howl from Beyond	|X|B
	 * Instant
	 * Target creature gets +X/+0 until end of turn. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CHECK_PUMP ){
		if ( has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id) ){
			int mana_avail = has_mana(player, COLOR_ANY, 1) - get_cmc(player, card);
			if (mana_avail > 0){
				pumpable_power[player] += mana_avail;
			}
		}
	}

	if (event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, instance->info_slot, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_howling_mine(int player, int card, event_t event){

	if( event == EVENT_DRAW_PHASE && ! is_tapped(player, card) && ! is_humiliated(player, card) ){
		event_result++;
	}

	return 0;
}

// Hurloon Minotaur --> vanilla

int card_hurricane(int player, int card, event_t event){
	// 0x41c940

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		new_damage_all(player, card, ANYBODY, get_card_instance(player, card)->info_slot, NDA_PLAYER_TOO, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 1, NULL);
}

int card_hypnotic_specter(int player, int card, event_t event)
{
  // 0x4c4210

  /* Hypnotic Specter	|1|B|B
   * Creature - Specter 2/2
   * Flying
   * Whenever ~ deals damage to an opponent, that player discards a card at random. */

  whenever_i_deal_damage_to_a_player_he_discards_a_card(player, card, event, DDBM_MUST_DAMAGE_OPPONENT, 1);
  return 0;
}

int card_ice_storm(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	if (event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
}

int card_icy_manipulator(int player, int card, event_t event){

	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_LAND | TYPE_ARTIFACT);

	if( event == EVENT_ACTIVATE  ){
		if( (current_turn == player && current_phase < PHASE_DECLARE_BLOCKERS) ||
			(current_turn != player && current_phase < PHASE_DECLARE_ATTACKERS)
		  ){
			ai_modifier+=15;
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		tap_card(instance->targets[0].player, instance->targets[0].card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(1), 0, &td, "ICY_MANIPULATOR");
}

static int effect_illusionary_mask(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		if( (event == EVENT_TAP_CARD && is_attacking(instance->targets[0].player, instance->targets[0].card)) ||
			damage_dealt_by_me_arbitrary(instance->targets[0].player, instance->targets[0].card, event, 0, player, card) ||
			damage_dealt_to_me_arbitrary(instance->targets[0].player, instance->targets[0].card, event, 0, player, card)
		  ){
			instance->targets[1].player = 66;
			int i;
			for(i=0;i<2;i++){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && get_id(i, count) == CARD_ID_APHETTO_RUNECASTER ){
							draw_some_cards_if_you_want(i, count, i, 1);
						}
						count--;
				}
			}
			int (*ptFunction)(int, int, event_t) = (void*)cards_data[instance->targets[1].card].code_pointer;
			ptFunction(instance->targets[0].player, instance->targets[0].card, EVENT_TURNED_FACE_UP );
			verify_legend_rule(instance->targets[0].player, instance->targets[0].card, get_id(player, card));
			kill_card(player, card, KILL_REMOVE);
		}

		if( event == EVENT_CHANGE_TYPE && affect_me(instance->targets[0].player, instance->targets[0].card) && instance->targets[1].player != 66 ){
			event_result = instance->targets[1].card;
		}
	}

	return 0;
}

int hack_illusionary_mask_event = EVENT_CAN_ACTIVATE;
int card_illusionary_mask(int player, int card, event_t event){

	char msg[100] = "Choose a creature card to play face down.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);
	this_test.has_mana_to_pay_cmc = 2;	hack_illusionary_mask_event = event;
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_sorcery_be_played(player, event) ){
		return check_battlefield_for_special_card(player, card, player, 0, &this_test);
	}
	else if( event == EVENT_ACTIVATE ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_VALUE, -1, &this_test);
			if( selected != -1 ){
				charge_mana_from_id(player, selected, event, get_id(player, selected));
				if( spell_fizzled != 1 ){
					instance->targets[0].card = selected;
				}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *target = get_card_instance( player, instance->targets[0].card );
		int effect_card = create_targetted_legacy_effect(player, instance->parent_card, &effect_illusionary_mask, player, instance->targets[0].card);
		card_instance_t *leg = get_card_instance( player, effect_card );
		leg->targets[2].card = target->internal_card_id;
		leg->targets[1].card = get_internal_card_id_from_csv_id( CARD_ID_FACE_DOWN_CREATURE );
		leg->targets[3].card = get_id(player, instance->parent_card);
		put_into_play(player, instance->targets[0].card);
	}
	return 0;
}

int card_instill_energy(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  int rval = vanilla_aura(player, card, event, player);

  if (event == EVENT_CAST_SPELL && affect_me(player, card) && cancel != 1)	// after vanilla_aura()
	{
	  card_instance_t* tgt_inst = get_card_instance(instance->targets[0].player, instance->targets[0].card);
	  if ((tgt_inst->regen_status & KEYWORD_DEFENDER) && !(tgt_inst->token_status & STATUS_WALL_CAN_ATTACK))
		ai_modifier -= 48;
	  if (instance->targets[0].player == player
		  && (get_card_data(instance->targets[0].player, instance->targets[0].card)->extra_ability & (EA_ACT_ABILITY | EA_ACT_INTERRUPT | EA_MANA_SOURCE)))
		ai_modifier += 48;
	}

  int tp = instance->damage_target_player, tc = instance->damage_target_card;
  if (tp >= 0 && tc >= 0)
	{
	  card_instance_t* tgt_inst = get_card_instance(tp, tc);

	  /* Current Oracle wording has reverted to the original functionality, which only allows attacking and not tapping like the 5th-edition wording does.  I'm
	   * going to studiously ignore that rather than trying to hack it into combat selection and AI, or trying to only removing summoning sickness during
	   * Declare Attackers or such. */
	  if ((tgt_inst->state & STATE_SUMMON_SICK)
		  && ((event == EVENT_RESOLVE_SPELL && cancel != 1)	// after_vanilla_aura()
			  || (event == EVENT_CARDCONTROLLED && affect_me(tp, tc) && in_play(tp, tc))))
		{
		  tgt_inst->state &= ~STATE_SUMMON_SICK;
		  instance->targets[3].player = 1;	// For the restore-summoning-sickness hack below
		}

	  if (event == EVENT_RESOLVE_ACTIVATION && in_play(tp, tc))
		untap_card(tp, tc);

	  /* Hack (translated from the exe) to restore summoning sickness if this leaves play.  Only sort of works.  (Try casting two Instill Energies on the same
	   * creature and then Disenchanting one, for example.)
	   *
	   * A real solution would be to:
	   * 1. Set both STATE_SUMMON_SICK and some other bit when a creature enters play or is controlled and remove both at start of turn (maybe STATE_SICKNESS,
	   *    which I haven't seen the exe use so far)
	   * 2. Make this and Haste and so on only remove STATE_SUMMON_SICK and only during EVENT_ABILITIES, and add STATE_SUMMON_SICK at the start of
	   *    EVENT_ABILITIES if the other bit is set. */
	  if (trigger_condition == TRIGGER_LEAVE_PLAY && affect_me(player, card) && reason_for_trigger_controller == player
		  && trigger_cause_controller == player && trigger_cause == card && instance->targets[3].player == 1
		  && in_play(tp, tc))
		{
		  if (event == EVENT_TRIGGER)
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		  if (event == EVENT_RESOLVE_TRIGGER)
			tgt_inst->state |= STATE_SUMMON_SICK;
		}
	}

  if (event == EVENT_BEGIN_TURN && current_turn == instance->damage_target_player)
	instance->targets[3].player = 0;

  return rval || generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN|GAA_IN_YOUR_TURN, MANACOST_X(0), 0, NULL, NULL);
}

int card_invisibility(int player, int card, event_t event){
  // 0x431c40
  card_instance_t* instance = get_card_instance(player, card);

	if(event == EVENT_BLOCK_LEGALITY && attacking_card_controller == instance->damage_target_player && attacking_card == instance->damage_target_card &&
		! has_subtype(affected_card_controller, affected_card, SUBTYPE_WALL) && in_play(player, card) && !is_humiliated(player, card)
	  ){
		event_result = 1;
	}

	return vanilla_aura(player, card, event, player);
}

int card_ironclaw_orcs(int player, int card, event_t event){
	// to insert
	if( event == EVENT_BLOCK_LEGALITY && affect_me(player, card) && ! is_humiliated(player, card) &&
		get_power(attacking_card_controller, attacking_card) > 1
	  ){
		event_result = 1;
	}
	return 0;
}

int card_iron_star(int player, int card, event_t event){
	return lifegaining_charm(player, card, event, 2, COLOR_TEST_RED, 1+player, 1);
}

// ironroot trefolk --> vanilla

// island --> better leave it hardcoded

int card_island_sanctuary(int player, int card, event_t event)
{
  /* Island Sanctuary	|1|W
   * Enchantment
   * If you would draw a card during your draw step, instead you may skip that draw. If you do, until your next turn, you can't be attacked except by creatures
   * with flying and/or |H2islandwalk. */

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST)
		return 1;

	if (event == EVENT_CAST_SPELL && affect_me(player, card))
		if (!check_battlefield_for_id(player, get_id(player, card)))
		ai_modifier += 48;

	if( ! is_humiliated(player, card) ){

	  enum
	  {
		SANC_AI_WILL_ACTIVATE = 1<<0,
		SANC_AI_CHECKED_FOR_ATTACKERS = 1<<1,
		SANC_ACTIVATED_THIS_TURN = 1<<2
	  };

	  if (trigger_condition == TRIGGER_REPLACE_CARD_DRAW
		  && reason_for_trigger_controller == player
		  && affect_me(player, card)
		  && current_phase == PHASE_DRAW
		  && player == current_turn
		  && !suppress_draw)
		{
		  if (event == EVENT_TRIGGER)
			{
			  if (player == HUMAN)
				event_result |= RESOLVE_TRIGGER_OPTIONAL;
			  else if (!(instance->info_slot & SANC_ACTIVATED_THIS_TURN))
				{
				  if (!(instance->info_slot & SANC_AI_CHECKED_FOR_ATTACKERS))
					{
					  instance->info_slot |= SANC_AI_CHECKED_FOR_ATTACKERS;

					  keyword_t req_kw = get_hacked_walk(player, card, KEYWORD_ISLANDWALK) | KEYWORD_FLYING;
					  int c, found = 0;
					  for (c = 0; c < active_cards_count[HUMAN]; ++c)
						if (in_play(HUMAN, c)
							&& is_what(HUMAN, c, TYPE_CREATURE))
						  {
							card_instance_t* creat = get_card_instance(HUMAN, c);
							keyword_t abils = creat->regen_status;
							if (!(abils & req_kw)
								&& !((abils & KEYWORD_DEFENDER) && !(creat->token_status & STATUS_WALL_CAN_ATTACK)))
							  {
								found = 1;
								break;
							  }
						  }

					  if (found && !internal_rand(8 - hand_count[player]))
						instance->info_slot |= SANC_AI_WILL_ACTIVATE;
					}

				  if (instance->info_slot & SANC_AI_WILL_ACTIVATE)
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
			}

		  if (event == EVENT_RESOLVE_TRIGGER)
			{
			  instance->info_slot &= ~SANC_AI_WILL_ACTIVATE;
			  player_bits[player] |= 1;
			  int leg = create_legacy_effect_exe(player, card, LEGACY_EFFECT_GENERIC, -1, -1);
			  if (leg != -1)
				{
				  card_instance_t* legacy = get_card_instance(player, leg);
				  legacy->token_status |= STATUS_ISLAND_SANCTUARY | STATUS_PERMANENT;
				  legacy->info_slot = get_hacked_walk(player, card, KEYWORD_ISLANDWALK) | KEYWORD_FLYING;
				}
			  suppress_draw = 1;
			  instance->info_slot |= SANC_ACTIVATED_THIS_TURN;
			}
		}
	}

  if ((event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card))
	instance->info_slot = 0;

  return 0;
}

int card_ivory_cup(int player, int card, event_t event){
	return lifegaining_charm(player, card, event, 2, COLOR_TEST_WHITE, 1+player, 1);
}

int card_jade_monolith(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_EFFECT);
	td.special = TARGET_SPECIAL_DAMAGE_CREATURE;
	td.extra = damage_card;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			dmg->damage_target_player = player;
			dmg->damage_target_card = -1;
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST_X(1), 0, &td, "TARGET_DAMAGE");
}

int card_jade_statue(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	// Approximation, or we won't be ever able to animate it before the attack declaration
	if(	beginning_of_combat(player, card, event, player, -1) ){
		if( can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
			int ai_choice = 0;
			if( is_sick(player, card) || is_tapped(player, card) ){
				ai_choice = 1;
			}
			int choice = do_dialog(player, player, card, -1, -1, " Animate Jade Idol\n Pass", ai_choice);
			if(choice == 0 && charge_mana_for_activated_ability(player, card, MANACOST_X(2)) ){
				set_special_flags(player, card, SF_TYPE_ALREADY_CHANGED);
				add_a_subtype(player, card, SUBTYPE_GOLEM);
				artifact_animation(player, card, player, card, 16, 3, 6, 0, 0);
				instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE;
				get_abilities(player, card, EVENT_CHANGE_TYPE, -1);
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE && ! is_what(player, card, TYPE_CREATURE) && current_turn != player ){
		if( current_phase == PHASE_BEFORE_BLOCKING || current_phase == PHASE_AFTER_BLOCKING ){
			return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_X(2), 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, MANACOST_X(2), 0, NULL, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		add_a_subtype(instance->parent_controller, instance->parent_card, SUBTYPE_GOLEM);
		artifact_animation(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 16, 3, 6, 0, 0);
	}

	return 0;
}

int card_jayemdae_tome(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(4), 0, NULL, NULL);
}

int card_juggernaut(int player, int card, event_t event){
	// original code : 00453C40

	if( ! is_humiliated(player, card) ){
		attack_if_able(player, card, event);

		if(event == EVENT_BLOCK_LEGALITY ){
			if( player == attacking_card_controller && card == attacking_card ){
				if( has_subtype(affected_card_controller, affected_card, SUBTYPE_WALL) ){
					event_result = 1;
				}
			}
		}
	}

   return 0;
}

int card_jump(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && affect_me(player, card)  ){
		if( current_phase < PHASE_DECLARE_BLOCKERS ){
			ai_modifier+=15;
		}
	}
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 0, 0, KEYWORD_FLYING, 0);
}

int card_karma(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=5*(count_subtype(1-player, TYPE_PERMANENT, SUBTYPE_SWAMP)-count_subtype(player, TYPE_PERMANENT, SUBTYPE_SWAMP));
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int amount = count_subtype(current_turn, TYPE_PERMANENT, get_hacked_subtype(player, card, SUBTYPE_SWAMP));
		if( amount ){
			damage_player(current_turn, amount, player, card);
		}
	}

	upkeep_trigger_ability(player, card, event, ANYBODY);

	return 0;
}

int card_keldon_warlord(int player, int card, event_t event){
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) && player != -1){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "");
		this_test.subtype = SUBTYPE_WALL;
		this_test.subtype_flag = DOESNT_MATCH;
		event_result+=check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
	}

	return 0;
}

int card_kormus_bell(int player, int card, event_t event){

	if( event == EVENT_CHANGE_TYPE ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_SWAMP);

		global_type_change(player, card, event, 2, TYPE_CREATURE, &this_test, 1, 1, 0, 0, 0);
	}

	return 0;
}

int card_kudzu(int player, int card, int event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t* instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 && ! is_humiliated(player, card) ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_TAP_CARD && affect_me(p, c) ){
			state_untargettable(p, c, 1);
			td.who_chooses = p;
			if(can_target(&td) && pick_target(&td, "TARGET_LAND") ){
				instance->damage_target_player = instance->targets[0].player;
				instance->damage_target_card = instance->targets[0].card;
			}
			state_untargettable(p, c, 0);
			kill_card(p, c, KILL_DESTROY);
		}
	}
	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_lance(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 0, 0, KEYWORD_FIRST_STRIKE, 0, 0, 0, 0);
}

int card_ley_druid(int player, int card, event_t event)
{
  // 0x4ca670
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);
  td.preferred_controller = player;

  card_instance_t* instance = get_card_instance(player, card);

  // |T: Untap target land.
  int rval = generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_LAND");

  if (event == EVENT_ACTIVATE && player == AI && cancel != 1 && !is_tapped(instance->targets[0].player, instance->targets[0].card))
	ai_modifier -= 48;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	untap_card(instance->targets[0].player, instance->targets[0].card);

  return rval;
}

int card_library_of_leng(int player, int card, event_t event){
  // 0x40BE80

	if( ! is_humiliated(player, card) ){
		if (event == EVENT_MAX_HAND_SIZE && current_turn == player ){
			event_result = 1000;
		}

		if (discard_trigger(player, card, event, player, RESOLVE_TRIGGER_OPTIONAL, DISCARD_STILL_IN_HAND)){
			put_on_top_of_deck(trigger_cause_controller, trigger_cause);
			play_sound_effect(WAV_DISCARD);
			EXE_BYTE(0x786DD4) = 1;	// prevent normal card-to-graveyard effect
		}
	}

	return card_spellbook(player, card, event);
}

int card_lich(int player, int card, event_t event){

	/* Lich	|B|B|B|B
	 * Enchantment
	 * As ~ enters the battlefield, you lose life equal to your life total.
	 * You don't lose the game for having 0 or less life.
	 * If you would gain life, draw that many cards instead.
	 * Whenever you're dealt damage, sacrifice that many nontoken permanents. If you can't, you lose the game.
	 * When ~ is put into a graveyard from the battlefield, you lose the game. */

	/* How delightfully obfuscated.  That's the Life's a Lich challenge.  is_unlocked() will always return false here, since it cheerfully ignores the supplied
	 * parameter for most values (including this one).  Instead, it fetches it from player/card, and Lich isn't in the list. */
	if( event == EVENT_GRAVEYARD_FROM_PLAY && get_challenge4() == 2 && is_unlocked(player, card, event, 37) ){
		return 0;
	}
	return ! is_humiliated(player, card) ? card_lich_exe(player, card, event) : 0;
}

int card_lifeforce(int player, int card, event_t event ){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		ai_modifier+=5*count_subtype(1-player, TYPE_PERMANENT, SUBTYPE_SWAMP);
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ANY);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if (counterspell_validate(player, card, &td, 0)){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK, MANACOST_G(2), 0, &td, NULL);
}

int card_lifelace(int player, int card, event_t event ){
	return lace(player, card, event, COLOR_TEST_GREEN);
}

int card_lifetap(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI){
		ai_modifier+=5*count_subtype(1-player, TYPE_PERMANENT, SUBTYPE_FOREST);
	}

	if( event == EVENT_TAP_CARD && affected_card_controller == 1-player && ! is_humiliated(player, card) ){
		if( has_subtype(affected_card_controller, affected_card, get_hacked_subtype(player, card, SUBTYPE_FOREST)) ){
			gain_life(player, 1);
		}
	}

	return 0;
}

int card_lightning_bolt(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_living_lands(int player, int card, event_t event){
	/*
	  Living Lands |3|G
	  Enchantment
	  All Forests are 1/1 creatures that are still lands.
	*/
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_LAND);
	this_test.subtype = SUBTYPE_FOREST;

	global_type_change(player, card, event, ANYBODY, TYPE_CREATURE, &this_test, 1, 1, 0, 0, 0);

	return global_enchantment(player, card, event);
}

int card_living_wall(int player, int card, event_t event){
	/*
	  Living Wall |4
	  Artifact Creature - Wall 0/6
	  Defender (This creature can't attack.)
	  {1}: Regenerate Living Wall.
	*/

	return regeneration(player, card, event, MANACOST_X(1));
}

int card_lord_of_atlantis(int player, int card, event_t event){

	boost_creature_type(player, card, event, SUBTYPE_MERFOLK, 1, 1, get_hacked_walk(player, card, KEYWORD_ISLANDWALK), 0);

	return 0;
}

int card_living_artifact(int player, int card, event_t event){

	/* Living Artifact	|G
	 * Enchantment - Aura
	 * Enchant artifact
	 * Whenever you're dealt damage, put that many vitality counters on ~.
	 * At the beginning of your upkeep, you may remove a vitality counter from ~. If you do, you gain 1 life. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->targets[0].player != -1 && ! is_humiliated(player, card)){
		damage_effects(player, card, event);

		if (count_counters(player, card, COUNTER_VITALITY) > 0){
			upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

			if (event == EVENT_UPKEEP_TRIGGER_ABILITY){
				remove_counter(player, card, COUNTER_VITALITY);
				gain_life(player, 1);
			}
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_ARTIFACT");
}

int card_lord_of_the_pit(int player, int card, event_t event){
	upkeep_trigger_ability(player, card, event, player);

	if (event == EVENT_UPKEEP_TRIGGER_ABILITY){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select another creature to sacrifice.");
		test.not_me = 1;
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		int dmg = 7;
		if( sac){
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			dmg = 0;
		}
		if( dmg ){
			damage_player(player, 7, player, card);
		}
	}
	return 0;
}

int card_lure(int player, int card, event_t event)
{
  // 0x4b9050
  return generic_aura(player, card, event, player, 0,0, 0,SP_KEYWORD_LURE, 0,0,0);
}

static int ai_can_hack_card(int player, int card){
	int colors_to_hack = cards_ptr[get_id(player, card)]->hack_colors;
	if( ! check_special_flags3(player, card, SF3_HARDCODED_HACK_WORDS_REPLACED) ){
		set_special_flags3(player, card, SF3_HARDCODED_HACK_WORDS_REPLACED);
		cards_ptr[get_id(player, card)]->hack_colors = colors_to_hack = get_colors_to_hack_from_csvid(get_id(player, card));
	}
	return colors_to_hack;
}

int card_magical_hack(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		int result = counterspell(player, card, event, NULL, 0);
		if( result ){

			instance->info_slot = 1;
			return result;
		}
		instance->info_slot = 0;
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( instance->info_slot == 1 ){
			if( ! ai_can_hack_card(card_on_stack_controller, card_on_stack) ){
				ai_modifier-=100;
			}
			return counterspell(player, card, event, NULL, 0);
		}
		generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
		if( ! ai_can_hack_card(instance->targets[0].player, instance->targets[0].card) ){
			ai_modifier-=100;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int t_player = -1;
		int t_card = -1;
		if( instance->info_slot == 1 ){
			if( counterspell_validate(player, card, NULL, 0) ){
				t_player = instance->targets[0].player;
				t_card = instance->targets[0].card;
			}
		}
		else{
			if( valid_target(&td) ){
				t_player = instance->targets[0].player;
				t_card = instance->targets[0].card;
			}
		}
		if( t_player != -1 ){
			replace_all_instances_of_one_basic_land_type_word_with_another(t_player, t_card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Mahamothi Djinn --> vanilla

int card_mana_flare(int player, int card, event_t event){
	//0x4B8940; also code for Heartbeat of Spring, Zhur-Taa Ancient

	if( ! is_humiliated(player, card) ){
		switch (event){
			case EVENT_TAP_CARD:
				if (!is_what(affected_card_controller, affected_card, TYPE_LAND)){
					return 0;
				}

				if (!in_play(player, card)){	// Seems unnecessary, but it's explicitly checked in the exe version, and is harmless
					return 0;
				}

				produce_mana_of_any_type_tapped_for(player, card, 1);

				return 0;

			case EVENT_COUNT_MANA:
				if (!is_what(affected_card_controller, affected_card, TYPE_LAND)){
					return 0;
				}

				if (!in_play(player, card)){	// Seems unnecessary, but it's explicitly checked in the exe version, and is harmless
					return 0;
				}

				// A check for EA_MANA_SOURCE is conspicuous by its absence in the exe version.
				if (is_tapped(affected_card_controller, affected_card) || is_animated_and_sick(affected_card_controller, affected_card)
					|| !can_produce_mana(affected_card_controller, affected_card) ){
					return 0;
				}

				int num_colors = 0;
				color_t col;
				card_instance_t* aff_instance = get_card_instance(affected_card_controller, affected_card);
				for (col = COLOR_COLORLESS; col <= COLOR_ARTIFACT; ++col){
					if (aff_instance->card_color & (1 << col)){
						++num_colors;
					}
				}

				if (num_colors > 0){
					declare_mana_available_hex(affected_card_controller, aff_instance->card_color, 1);
				} else {
					declare_mana_available(affected_card_controller, single_color_test_bit_to_color(aff_instance->card_color), 1);
				}

				return 0;

			case EVENT_CAN_CAST:
				return 1;

			case EVENT_CAST_SPELL:
				if (affect_me(player, card)){
					ai_modifier += 48;
				}
				return 0;

			default:
				return 0;
		}
	}
	return 0;
}

int card_mana_short(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI){
		ai_modifier+=(25*current_turn != player);
		ai_modifier-=(25-(5*count_subtype(1-player, TYPE_LAND, -1)));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			new_manipulate_all(player, card, instance->targets[0].player, &this_test, ACT_TAP);
			int i;
			for (i = COLOR_COLORLESS; i <= COLOR_WHITE; ++i){
				mana_pool[instance->targets[0].player][i] = 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_mana_vault(int player, int card, event_t event)
{
  // 0x424800
	if( ! is_humiliated(player, card)  ){
	  static int hack_dont_tap_to_pay_for_other_vaults = 0;

	  // ~ doesn't untap during your untap step.
	  does_not_untap(player, card, event);

	  // At the beginning of your upkeep, you may pay |4. If you do, untap ~.
	  if (trigger_condition == TRIGGER_UPKEEP && affect_me(player, card)
		  && player == current_turn
		  && !is_humiliated(player, card))
		{
		  if (event == EVENT_TRIGGER
			  /* It makes no sense to use the current Paradox Haze hack here, since there's nothing to spend the mana on between triggers.
			   * When that correctly adds full upkeep phases, this should run during all of them as normal. */
			  && count_upkeeps(player) > 0)
			{
			  if (player == HUMAN || (trace_mode & 2))
				{
				  if (has_mana(player, COLOR_ANY, 4)	// not an activated ability
					  && (!duh_mode(player) || is_tapped(player, card)))
					event_result |= RESOLVE_TRIGGER_OPTIONAL;
				}
			  else if (is_tapped(player, card))
				{
				  int mana_req = 6;
				  if (life[player] <= 5 || internal_rand(2))
					mana_req = 4;

				  /* It's aggravating that we need to recount mana, but if we just prevent payment, an AI with a tapped Mana Vault, an untapped Mana Vault, and an
				   * untapped land will tap the land and then do nothing else. */
				  ++hack_dont_tap_to_pay_for_other_vaults;
				  count_mana();
				  int has_enough = has_mana(player, COLOR_ANY, mana_req);
				  --hack_dont_tap_to_pay_for_other_vaults;
				  count_mana();

				  if (has_enough)
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
			}

		  if (event == EVENT_RESOLVE_TRIGGER)
			{
			  ++hack_dont_tap_to_pay_for_other_vaults;
			  int paid = charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_COLORLESS, 4);
			  --hack_dont_tap_to_pay_for_other_vaults;

			  if (paid)
				{
				  get_card_instance(player, card)->info_slot = 36;	// Make AI reluctant to tap until his next turn
				  untap_card(player, card);
				}
			}
		}

	  // At the beginning of your draw step, if ~ is tapped, it deals 1 damage to you.
	  if ((trigger_condition == TRIGGER_DRAW_PHASE || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card) && reason_for_trigger_controller == player
		  && current_turn == player
		  && is_tapped(player, card)
		  && !is_humiliated(player, card))
		{
		  if (event == EVENT_TRIGGER)
			event_result |= RESOLVE_TRIGGER_MANDATORY;

		  if (event == EVENT_RESOLVE_TRIGGER || event == EVENT_SHOULD_AI_PLAY)
			damage_player(player, 1, player, card);
		}

#define CAN_PRODUCE_MANA	(!is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card)	\
							 && !(hack_dont_tap_to_pay_for_other_vaults && player == AI && !(trace_mode & 2)))

	  // |T: Add |3 to your mana pool.
	  if (event == EVENT_CAN_ACTIVATE)
		return CAN_PRODUCE_MANA;

	  if (event == EVENT_ACTIVATE)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  ai_modifier -= 12 + instance->info_slot;
		  produce_mana_tapped(player, card, COLOR_COLORLESS, 3);
		}

	  if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_PRODUCE_MANA)
		declare_mana_available(player, COLOR_COLORLESS, 3);

	  // AI
	  if (event == EVENT_BEGIN_TURN && current_turn == player)
		get_card_instance(player, card)->info_slot = 0;	// Hasn't been untapped since player's last turn.

	  if (event == EVENT_CAST_SPELL && affect_me(player, card))
		ai_modifier += 192 / (landsofcolor_controlled[1][COLOR_ANY] + 1);

	  if (event == EVENT_SHOULD_AI_PLAY
		  && landsofcolor_controlled[player][COLOR_ANY] < 4
		  && (get_card_instance(player, card)->state & STATE_TAPPED))
		life[player] -= 4 - landsofcolor_controlled[player][COLOR_ANY];
#undef CAN_PRODUCE_MANA
	}

  return 0;
}

int card_manabarbs(int player, int card, event_t event){
	if( in_play(player, card) && event == EVENT_TAP_CARD && tapped_for_mana_color >= 0
		&& is_what(affected_card_controller, affected_card, TYPE_LAND) && ! is_humiliated(player, card)
	  ){
		damage_player(affected_card_controller, 1, player, card);
	}
	return global_enchantment(player, card, event);
}

int card_meekstone(int player, int card, event_t event){
	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && ! is_humiliated(player, card) ){
		if( get_power(affected_card_controller, affected_card) > 2 ){
			get_card_instance(affected_card_controller, affected_card)->untap_status &= ~3;
		}
	}
	return 0;
}

// Merfolk of the Pearl Trident, Mesa Pegasus --> Vanilla

int card_mind_twist(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			new_multidiscard(instance->targets[0].player, instance->info_slot, DISC_RANDOM, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_PLAYER", 1, NULL);
}

// Mons's Goblin Raiders --> vanilla

// Mountain --> hardcoded, for now

int card_mox_emerald(int player, int card, event_t event)
{
  // Mox Emerald: 0x41ff80
  // Mox Jet: 0x41ffa0
  // Mox Pearl: 0x41ffc0
  // Mox Ruby: 0x41ffe0
  // Mox Sapphire: 0x420000

  // And can be used for any non-sacrificing artifact mana source that produces one mana of any combination of colors.
  return artifact_mana_all_one_color(player, card, event, 1, 0);
}

int card_natural_selection(int player, int card, event_t event){
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
			rearrange_top_x(instance->targets[0].player, player, 3);
			int choice = do_dialog(player, player, card, -1, -1, " Reshuffle target player's deck\n Pass", instance->targets[0].player == player ? 1 : 0);
			if( ! choice ){
				shuffle(instance->targets[0].player);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_PLAYER", 1, NULL);
}

int card_nshadow(int player, int card, event_t event){
	// Nether Shadow --> Raging Goblin. The extra code the EXE version used is obsolete as we're handling the graveyard ability through Rules Engine
	haste(player, card, event);

  if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
	 int position = 0;
	 const int *graveyard = get_grave(player);
	 int victims = 0;
	 int count = count_graveyard(player) - 1;
	 while( count > -1 ){
		   if( cards_data[  graveyard[count] ].id == CARD_ID_NETHER_SHADOW ){
			  position = count;
		   }
		   count--;
	 }

	 count = count_graveyard(player) - 1;
	 while( count > position ){
		   if( (cards_data[ graveyard[count] ].type & TYPE_CREATURE) ){
			  victims++;
		   }
		   count--;
	 }

	 if( victims >= 3){
		int choice = do_dialog(player, player, card, -1, -1," Return Nether Shadow\n Do not return Nether Shadow\n", 0);
		if( choice == 0 ){
		   put_into_play(player, card);
		   return -1;
		}
	 }
	 return -2;
  }

 return 0;
}

static int effect_nettling_imp(int player, int card, event_t event)
{
  // 0x457b10
  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player, c = instance->damage_target_card;

  if (p >= 0 && c >= 0)
	attack_if_able(instance->damage_target_player, instance->damage_target_card, event);

  if ((event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card))
	{
	  if (p >= 0 && c >= 0
		  && in_play(p, c)
		  && !(get_card_instance(p, c)->state & (STATE_SUMMON_SICK | STATE_ATTACKED)))
		kill_card(p, c, KILL_DESTROY);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_nettling_imp(int player, int card, event_t event)
{
  // 0x454170
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = 1-player;
  td.required_subtype = SUBTYPE_WALL;
  td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;
  td.illegal_state = TARGET_STATE_SUMMONING_SICK;

  if (event == EVENT_ACTIVATE && player == AI)
	ai_modifier += 48;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		card_instance_t* instance = get_card_instance(player, card);
		int fake = add_card_to_hand(1-player, get_original_internal_card_id(instance->parent_controller, instance->parent_card));
		create_targetted_legacy_effect(1-player, fake, &effect_nettling_imp, instance->targets[0].player, instance->targets[0].card);
		obliterate_card(1-player, fake);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_BEFORE_ATTACKERS|GAA_IN_OPPONENT_TURN, MANACOST_X(0), 0, &td, "ERHNAM_DJINN");
}

int card_nevinyrrals_disk(int player, int card, event_t event){
	// 0x425A40
	comes_into_play_tapped(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_ENCHANTMENT);
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 1, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_nightmare2(int player, int card, event_t event){
	/* Nightmare	|5|B
	 * Creature - Nightmare Horse 100/100
	 * Flying
	 * ~'s power and toughness are each equal to the number of |H1Swamps you control. */
	/* Squelching Leeches	|2|B|B
	 * Creature - Leech 100/100
	 * ~'s power and toughness are each equal to the number of |H1Swamps you control. */

	// original code : 0x4C5450
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) && player != -1){
		event_result+=count_subtype(player, TYPE_PERMANENT, get_hacked_subtype(player, card, SUBTYPE_SWAMP));
	}
	return 0;
}

int northern_southern_paladin(int player, int card, event_t event, color_t color)
{
  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.required_color = 1 << get_sleighted_color(player, card, color);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

  const char* prompt = event == EVENT_ACTIVATE ? get_sleighted_color_text(player, card, "Select target %s permanent.", color) : "";

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_LITERAL_PROMPT, MANACOST_W(2), 0, &td, prompt);
}

int card_northern_paladin(int player, int card, event_t event)
{
  // 0x4c6a70

  /* Northern Paladin	|2|W|W
   * Creature - Human Knight 3/3
   * |W|W, |T: Destroy target |Sblack permanent. */

  return northern_southern_paladin(player, card, event, COLOR_BLACK);
}

// obsianus golem --> vanilla

int card_orcish_artillery(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t* instance = get_card_instance(player, card);
	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		damage_target0(player, card, 2);
		damage_player(instance->parent_controller, 3, instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_orcish_oriflamme(int player, int card, event_t event){
	if( event == EVENT_POWER && affected_card_controller == player && is_attacking(affected_card_controller, affected_card) && ! is_humiliated(player, card) ){
		event_result++;
	}
	return global_enchantment(player, card, event);
}

int card_paralyze(int player, int card, event_t event)
{
  // 0x4bac90
  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player, c = instance->damage_target_card;

  if (p >= 0 && c >= 0 && ! is_humiliated(player, card) ){
	  if (comes_into_play(player, card, event))
		tap_card(p, c);

	  does_not_untap(p, c, event);

	  // Can't make it an optional trigger, because activator (usually) doesn't control the card.
	  if (can_use_activated_abilities(player, card))
		upkeep_trigger_ability(player, card, event, p);

	  // not has_mana_for_activated_ability() or charge_mana_for_activated_ability() here, since they both require the controller to be activating

	  if (event == EVENT_UPKEEP_TRIGGER_ABILITY
		  && (!duh_mode(p)	// Only ask if tapped/not-enough-mana if duh mode is off
			  || (is_tapped(p, c)
				  && has_mana(p, COLOR_COLORLESS, 4))))	// AI choosing - only if enchantee is tapped, and can pay mana
		if ((p != HUMAN		// AI choosing for itself
			 || do_dialog(instance->damage_target_player, player, card, p, c, " Pay to untap\n Pass", 0) == 0)	// Human choosing, or AI speculating
			&& charge_mana_while_resolving(player, card, EVENT_RESOLVE_ACTIVATION, p, COLOR_COLORLESS, 4))
		  untap_card(p, c);
	}

  return disabling_aura(player, card, event);
}

// pearled unicorn --> vanilla
static const char* is_damaging_attached_creature(int who_chooses, int player, int card, int targeting_player, int targeting_card){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->internal_card_id == damage_card && instance->damage_target_player == targeting_player && instance->damage_target_card == targeting_card &&
		instance->info_slot > 0
	  ){
		return NULL;
	}
	return "must damage Personal Incarnation";
}

int card_personal_incarnation(int player, int card, event_t event){

	if( graveyard_from_play(player, card, event) ){
		int amount = round_up_value(life[get_owner(player, card)]);
		lose_life(get_owner(player, card), amount);
	}

	if( ! IS_GAA_EVENT(event) || get_owner(player, card) != player ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_EFFECT);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int)is_damaging_attached_creature;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			dmg->damage_target_player = instance->parent_controller;
			dmg->damage_target_card = -1;
		}
	}

	return generic_activated_ability(player, card, event, GAA_DAMAGE_PREVENTION | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_DAMAGE");
}

int pestilence_impl(int player, int card, event_t event, int sac_at_eot_if_no_creatures, color_t color)
{
	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST)
		return 1;

  // At the beginning of the end step, if no creatures are on the battlefield, sacrifice ~.
  if (sac_at_eot_if_no_creatures && eot_trigger(player, card, event))
	{
	  test_definition_t test;
	  default_test_definition(&test, TYPE_CREATURE);
	  if (!check_battlefield_for_subtype(ANYBODY, TYPE_CREATURE, -1))
		kill_card(player, card, KILL_SACRIFICE);
	}

  // |B: ~ deals 1 damage to each creature and each player.
  if (event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card))
	return has_mana_for_activated_ability(player, card, MANACOST_CLR(color, 1));

  if (event == EVENT_ACTIVATE)
	charge_mana_for_activated_ability(player, card, MANACOST_CLR(color, 1));

  if (event == EVENT_RESOLVE_ACTIVATION)
	new_damage_all(instance->parent_controller, instance->parent_card, ANYBODY, 1, NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);

  // AI.  Unchanged from exe version.
  if (event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI && check_battlefield_for_id(player, get_id(player, card)))
	ai_modifier -= 96;

  if (event == EVENT_SHOULD_AI_PLAY && in_play(player, card))
	{
	  int b_avail = MIN(life[player], landsofcolor_controlled[player][color]);
	  int rel_creatures_with_toughness[16] = {0};

	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_CREATURE))
			{
			  int tgh = get_card_instance(p, c)->toughness;
			  if (tgh <= b_avail && tgh < 16)
				{
				  // Should probably put a check for protection here.
				  if (p == player)
					--rel_creatures_with_toughness[tgh];
				  else
					++rel_creatures_with_toughness[tgh];
				}
			}

	  int i, mod = 0, most = 0;
	  for (i = 0; i < 16; ++i)
		{
		  mod += 24 * (i + 1) * rel_creatures_with_toughness[i];
		  most = MAX(most, mod);
		}

	  if (player == HUMAN)
		ai_modifier -= most;
	  else
		ai_modifier += most;

	  if (life[1 - player] < life[player] && landsofcolor_controlled[player][color])
		{
		  mod = 24 - (life[1 - player] / landsofcolor_controlled[player][color]);
		  mod = MAX(mod, 1);

		  if (player == HUMAN)
			ai_modifier -= 24 * mod;
		  else
			ai_modifier += 24 * mod;
		}
	}

  return 0;
}

int card_pestilence(int player, int card, event_t event)
{
  return pestilence_impl(player, card, event, 1, COLOR_BLACK);
}

int card_phantasmal_forces(int player, int card, event_t event){
	basic_upkeep(player, card, event, MANACOST_U(1));
	return 0;
}

int card_phantasmal_terrain(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) || event == EVENT_CAN_MOVE_AURA ||
		event == EVENT_MOVE_AURA || event == EVENT_RESOLVE_MOVING_AURA
	  ){
		return targeted_aura(player, card, event, &td, "TARGET_LAND");
	}


	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int blt[5] = {SUBTYPE_SWAMP, SUBTYPE_ISLAND, SUBTYPE_FOREST, SUBTYPE_MOUNTAIN, SUBTYPE_PLAINS};
			instance->targets[1].player = blt[do_dialog(player, player, card, -1, -1, " Swamp\n Island\n Forest\n Mountain\n Plains", 0)];
		}
		return targeted_aura(player, card, event, &td, "TARGET_LAND");
	}

	if( instance->damage_target_player > -1 && instance->targets[1].player > -1 && ! is_humiliated(player, card) ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_CHANGE_TYPE && affect_me(p, c) ){
			event_result = land_into_new_basic_land_type(event_result, get_hacked_subtype(player, card, instance->targets[1].player));
		}
	}

	return 0;


}

// phantom monster --> vanilla

int card_pirate_ship(int player, int card, event_t event){
	landhome(player, card, event, get_hacked_subtype(player, card, SUBTYPE_ISLAND));
	return card_prodigal_sorcerer(player, card, event);
}

int card_plague_rats(int player, int card, event_t event){
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result+=count_cards_by_id(ANYBODY, CARD_ID_PLAGUE_RATS);
	}
	return 0;
}

// plains, plateau --> hardcoded, for now

int card_power_leak(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t* instance = get_card_instance(player, card);

	if ( instance->damage_target_player > -1 ){
		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int dmg = 2;
			if( IS_AI(player) ){
				int mana_to_charge = 2;
				while( ! has_mana(instance->damage_target_player, COLOR_COLORLESS, mana_to_charge) ){
						mana_to_charge--;
				}
				charge_mana(instance->damage_target_player, COLOR_COLORLESS, mana_to_charge);
				if( spell_fizzled != 1 ){
					dmg-=mana_to_charge;
				}
			}
			else{
				if (charge_mana_while_resolving(player, card, event, instance->damage_target_player, COLOR_COLORLESS, -1)){
					dmg-=x_value;
				}
			}
			if( dmg > 0 ){
				damage_player(instance->damage_target_player, dmg, player, card);
			}
		}
		upkeep_trigger_ability(player, card, event, instance->damage_target_player);
	}

	return targeted_aura(player, card, event, &td, "TARGET_ENCHANTMENT");
}

int card_power_surge(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CLEANUP ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "");
		this_test.state = STATE_TAPPED;
		this_test.state_flag = DOESNT_MATCH;
		instance->targets[0].player = check_battlefield_for_special_card(player, card, 1-current_turn, CBFSC_GET_COUNT, &this_test);
	}
	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(current_turn, instance->targets[0].player, player, card);
	}
	if( instance->targets[0].player > 0 ){
		upkeep_trigger_ability(player, card, event, ANYBODY);
	}
	return global_enchantment(player, card, event);
}


int card_power_sink(int player, int card, event_t event){
	/* Power Sink	|X|U
	 * Instant
	 * Counter target spell unless its controller pays |X. If he or she doesn't, that player taps all lands with mana abilities he or she controls and empties his or her mana pool. */

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		if( counterspell_resolve_unless_pay_x(player, card, NULL, 0, instance->info_slot) == 1 ){ //AKA target validated but X wasn't paid
			int i;
			for (i = 0; i<active_cards_count[instance->targets[0].player]; ++i){
				if( in_play(instance->targets[0].player, i) && is_what(instance->targets[0].player, i, TYPE_LAND) &&
					is_mana_producer_land(instance->targets[0].player, i)
				  ){
					tap_card(instance->targets[0].player, i);
				}
			}
			for (i = COLOR_COLORLESS; i <= COLOR_ANY; ++i){	// mana_pool[player][COLOR_ANY] is total mana in pool
				mana_pool[instance->targets[0].player][i] = 0;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL | GS_COUNTERSPELL, NULL, NULL, 1, NULL);
}

int card_prodigal_sorcerer(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		damage_target0(player, card, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_psionic_blast(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if (event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 4);
			damage_player(player, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_psychic_venom(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t* instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 && ! is_humiliated(player, card) ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_TAP_CARD && affect_me(p, c) ){
			damage_player(p, 2, player, card);
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_purelace(int player, int card, event_t event){
	return lace(player, card, event, COLOR_TEST_WHITE);
}

// Raging River --> hardcoded, for now. Or Korath, you could do this if you want

int card_raise_dead(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) == -1 ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_red_elemental_blast(int player, int card, event_t event){
	return blast(player, card, event, COLOR_BLUE);
}

int card_red_ward(int player, int card, event_t event){
	return ward(player, card, event, KEYWORD_PROT_RED);
}

int card_regeneration(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if ( instance->damage_target_player > -1 && ! is_humiliated(player, card) && IS_GAA_EVENT(event) ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( can_regenerate(p, c) ){
				regenerate_target(p, c);
			}
		}
		return granted_generic_activated_ability(player, card, p, c, event, GAA_REGENERATION, MANACOST_G(1), 0, NULL, NULL);
	}

	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_regrowth(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_ANY);
	this_test.ai_selection_mode = AI_MAX_VALUE;

	if ( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
}

/* Resurrection	|2|W|W => portal_1_2_3k.c:Breath of Life
 * Sorcery
 * Return target creature card from your graveyard to the battlefield. */

int card_reverse_damage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;
	td.extra = damage_card;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			gain_life(player, target->info_slot);
			target->info_slot = 0;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_DAMAGE_PREVENTION | GS_CAN_TARGET, &td, "TARGET_DAMAGE", 1, NULL);
}

int card_righteousness(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_BLOCKING;

	if (event == EVENT_CHECK_PUMP ){
		if (!has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id) || current_turn == player){
			return 0;
		}
		pumpable_power[player] += 7;
		pumpable_toughness[player] += 7;
	}

	return vanilla_pump(player, card, event, &td, 7, 7, 0, 0);
}

// Roc of Kher Ridges --> vanilla

static const char* target_must_be_damage_targeting_this_rock_hydra(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  card_instance_t* instance = get_card_instance(player, card);
  return (instance->internal_card_id == damage_card && instance->damage_target_player == targeting_player && instance->damage_target_card == targeting_card
		  ? NULL
		  : text_lines[1]);	// can only work because initial targeting uses text_lines[0] of same source
}

int card_rock_hydra(int player, int card, event_t event)
{
  // 00454ed0

  /* ~ enters the battlefield with X +1/+1 counters on it.
   * For each 1 damage that would be dealt to ~, if it has a +1/+1 counter on it, remove a +1/+1 counter from it and prevent that 1 damage.
   * |R: Prevent the next 1 damage that would be dealt to ~ this turn.
   * |R|R|R: Put a +1/+1 counter on ~. Activate this ability only during your upkeep. */

  card_instance_t* instance = get_card_instance(player, card);
  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  instance->info_slot = x_value;
	  if (player == AI)
		ai_modifier += 24 * (x_value - 3);
	}

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, instance->info_slot);

  if (event == EVENT_CAN_ACTIVATE)
	{
	  if (!can_use_activated_abilities(player, card))
		return 0;

	  if ((land_can_be_played & LCBP_DAMAGE_PREVENTION)
		  && has_mana_for_activated_ability(player, card, MANACOST_R(1))
		  && count_1_1_counters(player, card) > 0
		  && EXE_FN(int, 0x458920, int, int, int)(player, card, -1))	// is there a damage card of any source type targeting me
		return 99;

	  if (current_phase != PHASE_UPKEEP
		  || player != current_turn
		  || player != EXE_DWORD(0x60A534)
		  || !has_mana_for_activated_ability(player, card, MANACOST_R(3)))
		return 0;

	  if (IS_AI(player) && hand_count[AI] - landsofcolor_controlled[AI][COLOR_RED] == 3)
		EXE_DWORD(0x736808) |= 3u;

	  return 1;
	}

  if (event == EVENT_ACTIVATE)
	{
	  instance->number_of_targets = 0;
	  if (land_can_be_played & LCBP_DAMAGE_PREVENTION)
		{
		  if (!charge_mana_for_activated_ability(player, card, MANACOST_R(1)))
			return 0;

		  target_definition_t td;
		  base_target_definition(player, card, &td, 0);
		  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		  td.extra = (int)target_must_be_damage_targeting_this_rock_hydra;

		  instance->number_of_targets = 0;
		  pick_target(&td, "ROCK_HYDRA");
		}
	  else if (charge_mana_for_activated_ability(player, card, MANACOST_R(3)))
		{
		  instance->targets[0].player = player;
		  instance->targets[0].card = card;
		  instance->number_of_targets = 1;
		}
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* parent = in_play(instance->parent_controller, instance->parent_card);
	  if (parent)
		{
		  if (land_can_be_played & LCBP_DAMAGE_PREVENTION)
			{
			  target_definition_t td;
			  base_target_definition(instance->parent_controller, instance->parent_card, &td, 0);
			  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
			  td.extra = (int)target_must_be_damage_targeting_this_rock_hydra;

			  if (valid_target(&td))
				{
				  card_instance_t* damage = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				  if (damage->info_slot > 0)
					--damage->info_slot;
				}
			  else
				cancel = 1;
			}
		  else
			add_1_1_counter(instance->parent_controller, instance->parent_card);

		  parent->number_of_targets = 0;
		}
	}

  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && damage->damage_target_card == card && damage->damage_target_player == player)
	{
	  int num_counters = count_1_1_counters(player, card);
	  num_counters = MIN(num_counters, damage->info_slot);

	  if (num_counters > 0)
		{
		  remove_1_1_counters(player, card, num_counters);
		  damage->info_slot -= num_counters;
		}
	}

  return 0;
}

int card_rod_of_ruin(int player, int card, event_t event){
	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		damage_target0(player, card, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_royal_assassin(int player, int card, event_t event){

	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		ai_defensive_modifier += 60;
	}

	if (event == EVENT_BLOCK_RATING && affect_me(player, card)){
		ai_defensive_modifier -= 60;
	}

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_sacrifice(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_SAC_CREATURE_AS_COST, NULL, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (!sac){
			cancel = 1;
			return 0;
		}
		instance->info_slot = get_cmc(BYTE2(sac), BYTE3(sac));
		kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
	}


	if ( event == EVENT_RESOLVE_SPELL ){
		produce_mana(player, COLOR_BLACK, instance->info_slot);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_SAC_CREATURE_AS_COST, NULL, NULL, 1, NULL);
}

int card_samite_healer(int player, int card, event_t event){
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

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DAMAGE_PREVENTION, MANACOST0, 0, &td, "TARGET_DAMAGE");
}

// savannah --> hardcoded, for now

// savannah lions, scathe zombies --> vanilla

int card_scavenging_ghoul(int player, int card, event_t event)
{
  // 0x4c93b0

  // At the beginning of each end step, put a corpse counter on ~ for each creature that died this turn.
  if (creatures_dead_this_turn > 0 && (event == EVENT_SHOULD_AI_PLAY || eot_trigger(player, card, event)))
	  add_counters(player, card, COUNTER_CORPSE, creatures_dead_this_turn);

  // Remove a corpse counter from ~: Regenerate ~.
  if (land_can_be_played & LCBP_REGENERATION)
	{
	  if (event == EVENT_CAN_ACTIVATE && count_counters(player, card, COUNTER_CORPSE) <= 0)
		return 0;

	  int rval = regeneration(player, card, event, MANACOST_X(0));
	  if (event == EVENT_ACTIVATE && cancel != 1)
		remove_counter(player, card, COUNTER_CORPSE);

	  return rval;
	}

  return 0;
}

// scrubland --> hardcoded, for now

// scryb sprites --> vanilla

int card_sea_serpent(int player, int card, event_t event){
	if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( ! check_battlefield_for_subtype(1-player, TYPE_PERMANENT, get_hacked_subtype(player, card, SUBTYPE_ISLAND)) ){
			event_result = 1;
		}
	}
	landhome(player, card, event, SUBTYPE_ISLAND);
	return 0;
}

int card_sedge_troll(int player, int card, event_t event){
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result+=check_battlefield_for_subtype(player, TYPE_PERMANENT, get_hacked_subtype(player, card, SUBTYPE_SWAMP)) ? 1 : 0;
	}
	return regeneration(player, card, event, MANACOST_B(1));
}

int card_sengir_vampire(int player, int card, event_t event){
	if( sengir_vampire_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counter(player, card);
	}
	return 0;
}

int card_serra_angel(int player, int card, event_t event){
	vigilance(player, card, event);
	return 0;
}

// shanodin dryads --> vanilla

int card_shatter(int player, int card, event_t event ){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}


	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT", 1, NULL);
}

/* Shivan Dragon	|4|R|R => fifth_dawn.c:Furnace Whelp
 * Creature - Dragon 5/5
 * Flying
 * |R: ~ gets +1/+0 until end of turn. */

int card_simulacrum(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( player == AI ){
			if( get_trap_condition(player, TRAP_DAMAGE_TAKEN) > 0 ){
				return can_target(&td);
			}
		}
		else{
			return can_target(&td);
		}
	}
	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int amount = get_trap_condition(player, TRAP_DAMAGE_TAKEN);
				damage_creature(instance->targets[0].player, instance->targets[0].card, amount, player, card);
				gain_life(player, amount);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// sinkhole --> ice storm

static int effect_sirens_call(int player, int card, event_t event)
{
  // 0x4a22d0
  all_must_attack_if_able(player, event, -1);

	if ((event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card)){
	  /* Ruling: 10/4/2004 It will require creatures with Haste to attack since they are able, but it won't destroy them if they don't for some reason.
	   * Not implementable, since haste removes summoning sickness without a trace.  (Instill Energy keeps track of whether it removes summoning sickness, in
	   * case the aura is removed.) */
		int c = active_cards_count[current_turn]-1;
		while( c > -1 ){
			if (in_play(current_turn, c) && is_what(current_turn, c, TYPE_CREATURE) && !has_subtype(current_turn, c, SUBTYPE_WALL)
				&& !(get_card_instance(current_turn, c)->state & (STATE_SUMMON_SICK | STATE_ATTACKED))
			  ){
				kill_card(current_turn, c, KILL_DESTROY);
			}
			c--;
		}
		kill_card(player, card, KILL_DESTROY);
	}
  return 0;
}

int card_sirens_call(int player, int card, event_t event)
{
	// 0x4a2bf0
	if( event == EVENT_CAN_CAST ){
		if( basic_spell(player, card, event) ){
			return current_turn != player && current_phase < PHASE_DECLARE_ATTACKERS;
		}
	}

	if (event == EVENT_RESOLVE_SPELL){
		int fake = add_card_to_hand(1-player, get_original_internal_card_id(player, card));
		create_legacy_effect(1-player, fake, &effect_sirens_call);
		obliterate_card(1-player, fake);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int ai_can_sleight_card(int player, int card){
	int colors_to_sleight = cards_ptr[get_id(player, card)]->sleight_color;
	if( ! check_special_flags3(player, card, SF3_HARDCODED_SLEIGHT_WORDS_REPLACED) ){
		set_special_flags3(player, card, SF3_HARDCODED_SLEIGHT_WORDS_REPLACED);
		cards_ptr[get_id(player, card)]->sleight_color = colors_to_sleight = get_colors_to_sleight_from_text(get_id(player, card));
	}
	return colors_to_sleight;
}

int card_sleight_of_mind(int player, int card, event_t event){
/*
Sleight of Mind |U
Instant
Change the text of target spell or permanent by replacing all instances of one color word with another.
(For example, you may change "target black spell" to "target blue spell." This effect lasts indefinitely.)
*/
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		int result = counterspell(player, card, event, NULL, 0);
		if( result ){
			instance->info_slot = 1;
			return result;
		}
		instance->info_slot = 0;
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( instance->info_slot == 1 ){
			if( ! ai_can_sleight_card(card_on_stack_controller, card_on_stack) ){
				ai_modifier-=100;
			}
			return counterspell(player, card, event, NULL, 0);
		}
		generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
		if( ! ai_can_sleight_card(instance->targets[0].player, instance->targets[0].card) ){
			ai_modifier-=100;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int t_player = -1;
		int t_card = -1;
		if( instance->info_slot == 1 ){
			if( counterspell_validate(player, card, NULL, 0) ){
				t_player = instance->targets[0].player;
				t_card = instance->targets[0].card;
			}
		}
		else{
			if( valid_target(&td) ){
				t_player = instance->targets[0].player;
				t_card = instance->targets[0].card;
			}
		}
		if( t_player != -1 ){
			replace_all_instances_of_one_color_word_with_another(t_player, t_card);
		}
		kill_card(player, card, KILL_DESTROY);
//		return card_sleight_of_mind_exe(player, card, event);
	}

	return 0;
}

int card_smoke(int player, int card, event_t event){
	untap_only_one_permanent_type(player, card, event, ANYBODY, TYPE_CREATURE);
	return global_enchantment(player, card, event);
}

int card_sol_ring(int player, int card, event_t event)
{
  //0x424b30

  /* Sol Ring	|1
   * Artifact
   * |T: Add |2 to your mana pool. */

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	ai_modifier += 192 / (landsofcolor_controlled[1][COLOR_ANY] + 1);

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_TAP_FOR_MANA(player, card);

  if (event == EVENT_ACTIVATE)
	{
	  ai_modifier -= 12;
	  produce_mana_tapped(player, card, COLOR_COLORLESS, 2);
	}

  if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_TAP_FOR_MANA(player, card))
	declare_mana_available(player, COLOR_COLORLESS, 2);

  return 0;
}

int card_soul_net(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, 0);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int i;
		for(i=0; i<instance->targets[11].card; i++){
			if( ! has_mana(player, COLOR_COLORLESS, 1) ){
				break;
			}
			charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_COLORLESS, 1);
			if( spell_fizzled == 1 ){
				break;
			} else {
				gain_life(player, 1);
			}
		}
		instance->targets[11].card = 0;
	}

	return 0;
}

int card_spell_blast(int player, int card, event_t event){

	// 0x40a740

	/* Spell Blast	|X|U
	 * Instant
	 * Counter target spell with converted mana cost X. */

#if 1
	return generic_x_spell_recoded(player, card, event, card_spell_blast_exe(player, card, event));
#else
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAN_CAST ){
		int result = counterspell(player, card, event, NULL, 0);
		if( result ){
			if( player == HUMAN ){
				return result;
			}
			else{
				if( has_mana_multi(player, get_cmc(card_on_stack_controller, card_on_stack), 0, 1, 0, 0, 0) ){
					return result;
				}
			}
		}
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int amount = -1;
		if( player == AI ){
			amount = get_cmc(card_on_stack_controller, card_on_stack);
		}
		charge_mana(player, COLOR_COLORLESS, amount);
		if( spell_fizzled != 1 ){
			instance->info_slot = x_value;
			instance->targets[1].player = get_cmc(card_on_stack_controller, card_on_stack);
			set_special_flags2(player, card, SF2_X_SPELL);
			return counterspell(player, card, event, NULL, 0);
		}
	}
	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->info_slot == instance->targets[1].player ){
			return counterspell(player, card, event, NULL, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
#endif
}

int card_stasis(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	basic_upkeep(player, card, event, MANACOST_U(1));

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && ! is_humiliated(player, card) ){
		if( is_tapped(affected_card_controller, affected_card) && is_what(affected_card_controller, affected_card, TYPE_LAND) ){
			get_card_instance(affected_card_controller, affected_card)->untap_status &= ~3;
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return global_enchantment(player, card, event);
}

int card_steal_artifact(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	return generic_stealing_aura(player, card, event, &td, "TARGET_ARTIFACT");
}

int card_stone_giant(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.toughness_requirement = (get_power(player, card)-1) | TARGET_PT_LESSER_OR_EQUAL;
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int legacy = pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
												0, 0, KEYWORD_FLYING, SP_KEYWORD_DIE_AT_EOT);
			get_card_instance( player, legacy)->targets[3].card = KILL_DESTROY;
		}
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td, "TARGET_CREATURE");
}

// stone rain --> ice storm

int card_stream_of_life(int player, int card, event_t event){
	// Original code : 0x41c300
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier+=(life[player]-20);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			gain_life(instance->targets[0].player, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_PLAYER", 1, NULL);
}

// sunglasses of urza --> hardcoded, for now
// swamp --> hardcoded, for now

int card_swords_to_plowshares(int player, int card, event_t event){
	// original code : 004A3750
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			gain_life(instance->targets[0].player, get_power(instance->targets[0].player, instance->targets[0].card));
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

// taiga --> hardcoded, for now

int card_terror(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_type = TYPE_ARTIFACT;
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target target nonartifact, nonblack creature.", 1, NULL);
}

int card_the_hive(int player, int card, event_t event){
	/* The Hive	|5
	 * Artifact
	 * |5, |T: Put a 1/1 colorless Insect artifact creature token with flying named Wasp onto the battlefield. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_WASP);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, 5, 0, 0, 0, 0, 0, 0, 0, 0);
}

static void destroy_nonwall_at_end_of_combat(int player, int card, int t_player, int t_card)
{
  if (!has_subtype(t_player, t_card, SUBTYPE_WALL))
	create_legacy_effect_exe(player, card, LEGACY_EFFECT_STONING, t_player, t_card);
}

int card_thicket_basilisk(int player, int card, event_t event)
{
  // 0x4ca9e0
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card))
	instance->destroys_if_blocked |= DIFB_DESTROYS_NONWALLS;

  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
	{
	  if (player == current_turn && (instance->state & STATE_ATTACKING))
		for_each_creature_blocking_me(player, card, destroy_nonwall_at_end_of_combat, player, card);

	  if (player == 1-current_turn && (instance->state & STATE_BLOCKING))
		for_each_creature_blocked_by_me(player, card, destroy_nonwall_at_end_of_combat, player, card);
	}

  return 0;
}

int card_thoughtlace(int player, int card, event_t event){
	return lace(player, card, event, COLOR_TEST_BLUE);
}

int card_throne_of_bone(int player, int card, event_t event){
	return lifegaining_charm(player, card, event, 2, COLOR_TEST_BLACK, 1+player, 1);
}

// timber wolves --> vanilla

int card_time_vault(int player, int card, event_t event)
{
  // 0x420280

	card_instance_t *instance = get_card_instance( player, card );
  // ~ enters the battlefield tapped.
  comes_into_play_tapped(player, card, event);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	ai_modifier += 24;

  // ~ doesn't untap during your untap step.
  does_not_untap(player, card, event);

  // If you would begin your turn while ~ is tapped, you may skip that turn instead. If you do, untap ~.
  if (event == EVENT_CAN_SKIP_TURN
	  && current_turn == player
	  && is_tapped(player, card)
	  && !this_turn_is_being_skipped()	// Added
	  && ! is_humiliated(player, card))
	{
	  char prompt[600];
	  load_text(0, "TIME_VAULT");
	  scnprintf(prompt, 600, " %s\n %s", text_lines[0], text_lines[1]);
	  if (do_dialog(player, player, card, -1, -1, prompt, internal_rand(5) == 0))
		{
		  land_can_be_played |= LCBP_SKIP_TURN;
		  untap_card(player, card);
		}
	}

  // |T: Take an extra turn after this one.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (!internal_rand(5))
		ai_modifier += 48;

	  time_walk_effect(instance->parent_controller, instance->parent_card);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(0), 0, NULL, NULL);
}

int card_time_walk(int player, int card, event_t event ){
	// original code : 0041B710

	if( event == EVENT_RESOLVE_SPELL ){
		time_walk_effect(player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

// Does everything Timetwister does except kill the card at resolution.
int resolve_timetwister(int player, int card, event_t event)
{
  // Each player shuffles his or her hand and graveyard into his or her library, then draws seven cards.

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	ai_modifier += 48 - 24 * hand_count[player];

  if (event == EVENT_RESOLVE_SPELL)
	{
	  /* Try to minimize impact of draw and shuffle triggers, which happen within shuffle() and draw_cards() instead of going onto the stack and resolving after
	   * this is complete */
	  APNAP(p,
			{
			  reshuffle_hand_into_deck(p, 1);
			  reshuffle_grave_into_deck(p, 1);
			});

	  APNAP(p, shuffle(p););

	  /* If both players run out of cards during this draw, the game should be drawn, not lost by whichever happened to draw first.  The exe version of Wheel of
	   * Fortune almost handles this correctly (it doesn't account for draw replacement effects - if both players have fewer than 7 cards, but one has an Island
	   * Sanctuary, for example, it'll still be a drawn game), but the exe version of Timetwister doesn't try.  I'm not going to either. */
	  APNAP(p, draw_cards(p, 7));
	}

  return 0;
}

int card_timetwister(int player, int card, event_t event)
{
  // 0x41C030

  /* Timetwister	|2|U
   * Sorcery
   * Each player shuffles his or her hand and graveyard into his or her library, then draws seven cards. */

  int rval = resolve_timetwister(player, card, event);
  if (event == EVENT_RESOLVE_SPELL)
	kill_card(player, card, KILL_DESTROY);

  return rval;
}

int card_tranquillity(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ENCHANTMENT);
		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// tropical island --> hardcoded, for now

int card_tsunami(int player, int card, event_t event){
	return basic_landtype_killer(player, card, event, SUBTYPE_ISLAND);
}

// tundra --> hardcoded, for now

int card_tunnel(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_WALL;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_WALL", 1, NULL);
}

int card_twiddle(int player, int card, event_t event)
{
  // 0x4a2fc0

  // You may tap or untap target artifact, creature, or land.
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_ARTIFACT|TYPE_CREATURE|TYPE_LAND);
  td.preferred_controller = ANYBODY;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		twiddle(player, card, 0);

	  kill_card(player, card, KILL_DESTROY);
	}

  int rval = generic_spell(player, card, event, GS_CAN_TARGET, &td, "TWIDDLE", 1, NULL);

  if (event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI && cancel != 1)
	ai_modifier_twiddle(player, card, 0);

  return rval;
}

int card_two_headed_giant_of_foriys(int player, int card, event_t event)
{
	// 4559F0
	creature_can_block_additional(player, card, event, 1);
	return 0;
}

// underground sea --> hardcoded, for now

int card_unholy_strenght(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 2, 1, 0, 0, 0, 0, 0);
}

int card_unsummon(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_uthden_troll(int player, int card, event_t event){
	return regeneration(player, card, event, MANACOST_R(1));
}

int card_verduran_enchantress(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_OPTIONAL, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		draw_cards(player, 1);
	}
	return 0;
}

static int effect_vesuvan_doppelganger(int player, int card, event_t event){
	if (effect_follows_control_of_attachment(player, card, event))
		return 0;

	card_instance_t* instance = get_card_instance(player, card);
	int p = instance->damage_target_player, c = instance->damage_target_card;
	if( p > -1 && c > -1 ){
		if( ! is_humiliated(p, c) ){
			upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

			if (event == EVENT_UPKEEP_TRIGGER_ABILITY){

				target_definition_t td;
				default_target_definition(p, c, &td, TYPE_CREATURE);
				td.preferred_controller = ANYBODY;

				instance->number_of_targets = 0;
				load_text(0, "TARGET_CREATURE");
				if (select_target(p, c-1000, &td, text_lines[0], &instance->targets[0])){
					if (instance->targets[0].card == c && instance->targets[0].player == p)	// picked self
						return 0;

					cloning_and_verify_legend(p, c, instance->targets[0].player, instance->targets[0].card);

					// Should usually be redundant to the setting in the assignment in card_vesuvan_doppelganger(), but needed to overwrite forced color from tokens.
					get_card_instance(p, c)->initial_color = COLOR_TEST_BLUE;
				}
				else{
					cancel = 1;
				}
			}
		}

	}

  return 0;
}

int card_vesuvan_doppelganger(int player, int card, event_t event)
{
  /* Vesuvan Doppelganger	|3|U|U
   * Creature - Shapeshifter 0/0
   * You may have ~ enter the battlefield as a copy of any creature on the battlefield except it doesn't copy that creature's color and it gains "At the
   * beginning of your upkeep, you may have this creature become a copy of target creature except it doesn't copy that creature's color. If you do, this
   * creature gains this ability." */

  /* Like the original Clone, Vesuvan Doppelganger's csvid is special-cased in the exe to check EVENT_CAN_CAST like it was an instant, sorcery, or enchantment.
   * It's also special-cased in get_displayable_cardname_from_player_card() to always show its own name, but that doesn't work.  (Just as well.) */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (enters_the_battlefield_as_copy_of_any_creature(player, card, event))
	{
	  get_card_instance(player, card)->initial_color = COLOR_TEST_BLUE;
	  set_legacy_image(player, CARD_ID_VESUVAN_DOPPELGANGER, create_targetted_legacy_effect(player, card, effect_vesuvan_doppelganger, player, card));
	}

  return 0;
}

int card_veteran_bodyguard(int player, int card, event_t event){
	if( ! is_tapped(player, card) && event == EVENT_PREVENT_DAMAGE && (current_phase == PHASE_FIRST_STRIKE_DAMAGE ||
		current_phase == PHASE_NORMAL_COMBAT_DAMAGE) && ! is_humiliated(player, card)
	  ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == player &&
			damage->damage_target_card == -1
		  ){
			if( is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE) &&
				is_unblocked(damage->damage_source_player, damage->damage_source_card)
			  ){
				damage->damage_target_player = player;
				damage->damage_target_card = card;
			}
		}
	}
	return 0;
}

int card_volcanic_eruption(int player, int card, event_t event){
	// 0x41c4a0
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( (played_for_free(player, card) || is_token(player, card)) && ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			return 0;
		}
		int trgs = 0;
		while( can_target(&td) ){
				if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) && ! has_mana(player, COLOR_COLORLESS, trgs+1) ){
					break;
				}
				if( check_special_flags2(player, card, SF2_COPIED_FROM_STACK) && trgs == instance->info_slot ){
					break;
				}
				if( new_pick_target(&td, get_hacked_land_text(player, card, "Select target %s.", SUBTYPE_MOUNTAIN), trgs, GS_LITERAL_PROMPT) ){
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
		if( trgs == 0 ){
			spell_fizzled = 1;
			return 0;
		}
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			charge_mana(player, COLOR_COLORLESS, trgs);
		}
		if( spell_fizzled != 1 ){
			instance->info_slot = trgs;
		}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int i;
			int dmg = 0;
			for(i=0; i<instance->number_of_targets; i++){
				if( validate_target(player, card, &td, i) ){
					kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
					dmg++;
				}
			}
			if( dmg > 0 ){
				APNAP(p, {new_damage_all(player, card, p, dmg, NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);};);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

// volcanin island --> hardcoded, for now

/* Wall of Air	|1|U|U => vanilla
 * Creature - Wall 1/5
 * Defender, flying */

/* Wall of Bone	|2|B => Drudge Skeletons
 * Creature - Skeleton Wall 1/4
 * Defender
 * |B: Regenerate ~. */

int card_wall_of_brambles(int player, int card, event_t event){
	/* Wall of Brambles	|2|G
	 * Creature - Plant Wall 2/3
	 * Defender
	 * |G: Regenerate ~. */
	return regeneration(player, card, event, MANACOST_G(1));
}

/* Wall of Fire	|1|R|R => fifth_dawn.c:Furnace Whelp
 * Creature - Wall 0/5
 * Defender
 * |R: ~ gets +1/+0 until end of turn. */

/* Wall of Ice	|2|G => vanilla
 * Creature - Wall 0/7
 * Defender */

/* Wall of Stone	|1|R|R => vanilla
 * Creature - Wall 0/8
 * Defender */

/* Wall of Swords	|3|W => vanilla
 * Creature - Wall 3/5
 * Defender, flying */

int card_wall_of_water(int player, int card, event_t event){
	/* Wall of Water	|1|U|U
	 * Creature - Wall 0/5
	 * Defender
	 * |U: ~ gets +1/+0 until end of turn. */
	return generic_shade(player, card, event, 0, MANACOST_U(1), 1, 0, 0, 0);
}

/* Wall of Wood	|G => vanilla
 * Creature - Wall 0/3
 * Defender */

int card_wanderlust(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	return cursed_permanent(player, card, event, 1, &td, "TARGET_CREATURE");
}

// war mammoth --> vanilla

int card_warp_artifact(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	return cursed_permanent(player, card, event, 1, &td, "TARGET_ARTIFACT");
}

// water elemental --> vanilla

int card_weakness(int player, int card, event_t event){
	return generic_aura(player, card, event, 1-player, -2, -1, 0, 0, 0, 0, 0);
}

int card_web(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 0, 2, KEYWORD_REACH, 0, 0, 0, 0);
}

int card_wheel_of_fortune(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI ){
			ai_modifier+=5*(hand_count[1-player]-hand_count[player]);
			if( count_deck(player) < 10 ){
				ai_modifier-=50;
			}
			if( count_deck(1-player) <= 7 ){
				ai_modifier+=50;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{
				discard_all(p);
				draw_cards(p, 7);
				};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_white_knight(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		protection_from_black(player, card, event);
	}
	return 0;
}


int card_white_ward(int player, int card, event_t event){
	return ward(player, card, event, KEYWORD_PROT_WHITE);
}

int card_wild_growth(int player, int card, event_t event){
	return wild_growth_aura(player, card, event, -1, COLOR_GREEN, 1);
}

// will-'o-the-wisp --> drudge skeletons

int card_winter_orb(int player, int card, event_t event){
	untap_only_one_permanent_type(player, card, event, ANYBODY, TYPE_LAND);
	return 0;
}

int card_wooden_sphere(int player, int card, event_t event){
	return lifegaining_charm(player, card, event, 2, COLOR_TEST_GREEN, 1+player, 1);
}

// word of command --> uncodeable

int card_wrath_of_god(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, ANYBODY, &this_test, KILL_BURY);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static const char* zombie_to_regenerate(int who_chooses, int player, int card)
{
	if(	can_use_activated_abilities(player, card) ){
		if( who_chooses == HUMAN || (who_chooses == AI && can_regenerate(player, card)) ){
			return NULL;
		}
	}
	return "this Zombie cannot regenerate.";
}

int zombie_master_shared_ability(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[1].player > -1 ){
		int p = instance->targets[1].player;
		int c = instance->targets[1].card;

		if( leaves_play(p, c, event) || get_id(p, c) != CARD_ID_ZOMBIE_MASTER ){
			kill_card(player, card, KILL_REMOVE);
		}

		if( ! IS_GAA_EVENT(event) || is_humiliated(p, c) ){
			return 0;
		}

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_state = TARGET_STATE_DESTROYED;
		td.preferred_controller = player;
		td.required_subtype = SUBTYPE_ZOMBIE;
		td.extra = (int32_t)zombie_to_regenerate;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		td.illegal_abilities = 0;

		if( event == EVENT_CAN_ACTIVATE ){
			if( land_can_be_played & LCBP_REGENERATION ){
				int pp, cc;
				for(pp = 0; pp<2; pp++){
					for(cc=0; cc < active_cards_count[pp]; cc++){
						if( !(pp == p && cc == c) && in_play(pp, cc) && is_what(pp, cc, TYPE_PERMANENT) && has_subtype(pp, cc, SUBTYPE_ZOMBIE) ){
							int result = granted_generic_activated_ability(player, card, pp, cc, event, GAA_REGENERATION, MANACOST_B(1), 0, NULL, NULL);
							if( result ){
								return result;
							}
						}
					}
				}
			}
		}

		if( event == EVENT_ACTIVATE ){
			state_untargettable(p, c, 1);
			instance->number_of_targets = 1;
			if( new_pick_target(&td, "Select a Zombie to regenerate.", 0, 1 | GS_LITERAL_PROMPT) ){
				if( can_use_activated_abilities(instance->targets[0].player, instance->targets[0].card) ){
					charge_mana_for_activated_ability(instance->targets[0].player, instance->targets[0].card, MANACOST_B(1));
				}
				else{
					spell_fizzled = 1;
				}
			}
			state_untargettable(p, c, 0);
		}

		if( event == EVENT_RESOLVE_ACTIVATION && validate_target(player, card, &td, 0) &&
			can_regenerate(instance->targets[0].player, instance->targets[0].card)
		  ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_zombie_master(int player, int card, event_t event){//not actually used; see card_zombie_master2 in recoded_cards.c
	if( event == EVENT_ABILITIES && affect_me(player, card )){
		return 0;
	}
	return card_zombie_master_exe(player, card, event);
}

static void create_zombie_master_legacies(int player, int card){
	int legacy = create_legacy_activate(player, card, &zombie_master_shared_ability);
	card_instance_t *instance = get_card_instance(player, legacy);
	instance->targets[1].player = player;
	instance->targets[1].card = card;
	instance->number_of_targets = 1;
	int fake = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
	legacy = create_legacy_activate(1-player, fake, &zombie_master_shared_ability);
	instance = get_card_instance(1-player, legacy);
	instance->targets[1].player = player;
	instance->targets[1].card = card;
	instance->number_of_targets = 1;
	obliterate_card(1-player, fake);
}

int card_zombie_master2(int player, int card, event_t event){
	/*
	  Zombie Master |1|B|B
	  Creature - Zombie 2/3
	  Other Zombie creatures have swampwalk.
	  Other Zombies have "{B}: Regenerate this permanent."
	*/

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) ){
		//Create the legacies for the fake ability here, in case something already in play is transformed into a Zombie Master.
		int i;
		int found = 0;
		for(i=active_cards_count[player]-1; i>-1; i--){
			if( in_play(player, i) && is_what(player, i, TYPE_EFFECT) ){
				if( get_card_instance(player, i)->display_pic_csv_id == CARD_ID_ZOMBIE_MASTER ){
					found = 1;
					break;
				}
			}
		}
		if( ! found ){
			create_zombie_master_legacies(player, card);
		}
	}

	if( event == EVENT_ABILITIES && ! is_humiliated(player, card) ){
		if( has_creature_type(affected_card_controller, affected_card, SUBTYPE_ZOMBIE) && ! affect_me(player, card) ){
			event_result |= (get_hacked_walk(player, card, KEYWORD_SWAMPWALK) | KEYWORD_REGENERATION);
		}
	}

	return 0;
}
