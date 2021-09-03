// -*- c-basic-offset:2 -*-
// Keyword abilities, and abilities handled analagously to them.

#include "manalink.h"

/***********************
* Removal of abilities *
***********************/

/* Uses bits 0-11 of all cards' targets[17].card to indicate non-mana activated abilities removed (0-3), all activated abilities removed (4-7), and all
 * abilities removed (8-11).  Just in case one of these types of removal is imposed 16 times. */

int can_use_activated_abilities(int player, int card)
{
  card_instance_t* instance = get_card_instance(player, card);
  return instance->targets[17].card <= 0 || !(instance->targets[17].card & 0xFFF);
}

int can_use_activated_abilities_by_instance(card_instance_t* instance)
{
  return instance->targets[17].card <= 0 || !(instance->targets[17].card & 0xFFF);
}

int can_produce_mana(int player, int card)
{
	card_instance_t* instance = get_card_instance(player, card);
	int result = 1;
	if( instance->targets[17].card > 0 && (instance->targets[17].card & 0xFF0) ){ // Check for abilities disabled / "humiliation"
		result = 0;
	}
	if( result == 1 && player == AI ){
		if( instance->targets[14].card != -1 && (instance->targets[14].card & SF_MANA_PRODUCER_DISABLED_FOR_AI) ){
			result = 0;
		}
	}
	return result;
}

int is_humiliated_by_instance(card_instance_t* instance)
{
  return !(instance->targets[17].card <= 0 || !(instance->targets[17].card & 0xF00));
}

int is_humiliated(int player, int card)
{
  if (player == -1 || card == -1)
	return 0;
  else
	return is_humiliated_by_instance(get_card_instance(player, card));
}

void disable_nonmana_activated_abilities(int player, int card, int mode)
{
  // mode = 0: Reactivate activated abilities.
  // mode = 1: Disable activated abilities.

  card_instance_t* instance = get_card_instance(player, card);

  if (instance->targets[17].card < 0)
	instance->targets[17].card = 0;

  uint32_t u = instance->targets[17].card & 0xF;

  if (mode)
	{
	  if (u != 0xF)	// counter not maxed
		u += 0x1;
	}
  else
	{
	  if (u != 0)	// any counters to remove
		u -= 0x1;
	}

  instance->targets[17].card &= ~0xF;
  instance->targets[17].card |= u;
}

void disable_all_activated_abilities(int player, int card, int mode)
{
  // mode = 0: Reactivate all activated abilities
  // mode = 1: Shutdown all activated abilities

  card_instance_t* instance = get_card_instance(player, card);

  if (instance->targets[17].card < 0)
	instance->targets[17].card = 0;

  uint32_t u = instance->targets[17].card & 0xF0;

  if (mode)
	{
	  if (u != 0xF0)	// counter not maxed
		u += 0x10;
	}
  else
	{
	  if (u != 0)	// any counters to remove
		u -= 0x10;
	}

  instance->targets[17].card &= ~0xF0;
  instance->targets[17].card |= u;
}

static void real_humiliation(int t_player, int t_card, int mode){
	card_instance_t* instance = get_card_instance(t_player, t_card);

	if (instance->targets[17].card < 0){
		instance->targets[17].card = 0;
	}

	uint32_t u = instance->targets[17].card & 0xF00;

	if (mode){
		if (u != 0xF00){	// counter not maxed
			u += 0x100;
		}
	}
	else{
		if (u != 0){	// any counters to remove
			u -= 0x100;
		}
	}

	instance->targets[17].card &= ~0xF00;
	instance->targets[17].card |= u;
}

int set_pt_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );

	if( instance->damage_target_player > -1 && check_status(player, card, STATUS_CONTROLLED) ){
		if( event == EVENT_POWER && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result += instance->counter_power;
		}
		if( event == EVENT_TOUGHNESS && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			event_result += instance->counter_toughness;
		}
	}

	if( instance->targets[1].player > -1 ){
		if( (instance->targets[1].player & 1) && event == EVENT_CLEANUP ){
			reactivate_the_most_recent_pt_setting_effects_attached_to_me(instance->damage_target_player, instance->damage_target_card);
			kill_card(player, card, KILL_REMOVE);
		}
		if( (instance->targets[1].player & 2) && instance->targets[2].player > -1 &&
			leaves_play(instance->targets[2].player, instance->targets[2].card, event)
		 ){
			reactivate_the_most_recent_pt_setting_effects_attached_to_me(instance->damage_target_player, instance->damage_target_card);
			kill_card(player, card, KILL_REMOVE);
		}
		if( (instance->targets[1].player & 4) && current_turn == instance->damage_target_player && upkeep_trigger(player, card, event) ){
			reactivate_the_most_recent_pt_setting_effects_attached_to_me(instance->damage_target_player, instance->damage_target_card);
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

static int is_pt_effect(int player, int card){
	card_instance_t* inst = get_card_instance(player, card);
	if( is_what(player, card, TYPE_EFFECT) && (inst->info_slot == (int)set_pt_legacy || inst->display_pic_csv_id == CARD_ID_GIGANTOPLASM) ){
		return 1;
	}
	return 0;
}

static int effect_on_all_pt_setting_effects_attached_to_me(int player, int card, int effect){
// effect = 0 --> check only
// effect = 1 -> enable the "set_pt_legacy" with the highest timestamp (AKA the most recent)
// effect = -1 -> disable all "set_pt_legacy"
	int result = 0;
	int legacies[100][3];
	int lc = 0;
	card_instance_t* inst;
	int p, c;
	for (p = 0; p <= 1; ++p){
		for (c = active_cards_count[p]; c > -1; c--){
			if ( (inst = in_play(p, c)) && inst->damage_target_player == player && inst->damage_target_card == card ){
				if( is_pt_effect(p, c) ){
					if( effect == 1 && ! check_status(p, c, STATUS_CONTROLLED) ){
						legacies[lc][0] = inst->timestamp;
						legacies[lc][1] = p;
						legacies[lc][2] = c;
						lc++;
						result = 1;
					}
					if( effect == -1 ){
						remove_status(p, c, STATUS_CONTROLLED);
						result = 1;
					}
				}
			}
		}
	}
	if( lc ){
		int max_timestamp = 0;
		int id_of_legacy_to_activate = 0;
		int i;
		for(i=0; i<lc; i++){
			if( legacies[i][0] > max_timestamp ){
				max_timestamp = legacies[i][0];
			}
			id_of_legacy_to_activate = i;
		}
		add_status(legacies[id_of_legacy_to_activate][1], legacies[id_of_legacy_to_activate][2], STATUS_CONTROLLED);
	}
	return result;
}

void disable_other_pt_setting_effects_attached_to_me(int t_player, int t_card){
	effect_on_all_pt_setting_effects_attached_to_me(t_player, t_card, -1);
}

void reactivate_the_most_recent_pt_setting_effects_attached_to_me(int t_player, int t_card){
	effect_on_all_pt_setting_effects_attached_to_me(t_player, t_card, 1);
}

int real_set_pt(int player, int card, int t_player, int t_card, int pow, int tou, int mode){
	disable_other_pt_setting_effects_attached_to_me(t_player, t_card);
	int leg = create_targetted_legacy_effect(player, card, &set_pt_legacy, t_player, t_card);
	if (leg != -1){
		card_instance_t* legacy = get_card_instance(player, leg);
		legacy->token_status |= STATUS_CONTROLLED;

		legacy->counter_power = pow > -1 ? pow - get_base_power(t_player, t_card) : 0;
		legacy->counter_toughness = tou > -1 ? tou - get_base_toughness(t_player, t_card) : 0;
		legacy->targets[1].player = mode;
	}
	return leg;
}

int set_pt_and_abilities_until_eot(int player, int card, int t_player, int t_card, int pow, int tou, int key, int s_key, int mode ){
	// pow = -1 : leave power unchanged
	// tou = -1 : leave toughness unchanged
	if( t_player != ANYBODY && t_card != -1 ){
		real_set_pt(player, card, t_player, t_card, pow, tou, 1);
		int legacy = -1;
		if( key || s_key ){
			legacy = pump_ability_until_eot(player, card, t_player, t_card, 0, 0, key, s_key);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
		return legacy;
	}
	int i;
	for(i=0; i<2; i++){
		if( i == t_player || t_player == ANYBODY ){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
						int good = 1;
						if( ( mode & 1) && ! (is_attacking(i, count) && is_unblocked(i, count)) ){
							good = 0;
						}
						if( good == 1 ){
							real_set_pt(player, card, i, count, pow, tou, 1);
							if( key || s_key ){
								int legacy = pump_ability_until_eot(player, card, i, count, 0, 0, key, s_key);
								add_status(player, legacy, STATUS_INVISIBLE_FX);
							}
						}
					}
					count--;
			}
		}
	}
	return -1;
}

int humility_legacy(int player, int card, event_t event){
	// See also tempest.c:card_humility()
	card_instance_t* instance = get_card_instance(player, card);

	int p = instance->damage_target_player;
	int c = instance->damage_target_card;

	if( p > -1 ){
		if( instance->token_status & STATUS_CONTROLLED ){
			if( affect_me(p, c) ){
				int pow = instance->targets[1].player;
				int tgh = instance->targets[1].card;
				int color = instance->targets[2].card;
				int abils = instance->targets[2].player;
				switch (event){
					case EVENT_POWER:
						event_result += (pow > -1 ? pow : 0) - get_base_power(p, c);
						break;

					case EVENT_TOUGHNESS:
						event_result += (tgh > -1 ? tgh : 1) - get_base_toughness(p, c);
						break;

					case EVENT_ABILITIES:
						event_result &= (KEYWORD_RECALC_CHANGE_TYPE|KEYWORD_RECALC_TOUGHNESS|KEYWORD_RECALC_POWER|KEYWORD_RECALC_ABILITIES|KEYWORD_RECALC_SET_COLOR);
						card_instance_t* aff = get_card_instance(p, c);
						aff->state &= ~STATE_VIGILANCE;
						// Probably appropriate to remove more, but these are the only ones that give icons, so meh.
						if (aff->targets[16].card != -1)
							aff->targets[16].card &= ~(SP_KEYWORD_FEAR|SP_KEYWORD_INTIMIDATE|SP_KEYWORD_UNBLOCKABLE|SP_KEYWORD_DEATHTOUCH|SP_KEYWORD_LIFELINK
													   |SP_KEYWORD_INDESTRUCTIBLE|SP_KEYWORD_SHADOW|SP_KEYWORD_HEXPROOF|SP_KEYWORD_HORSEMANSHIP
													   |SP_KEYWORD_VIGILANCE);
						event_result |= abils;
						break;

					case EVENT_SET_COLOR:
						if (color)
						event_result = color;
						break;

					default:;	// avoid warning
				}
			}
		}
		if( event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY ){
			int mode = instance->targets[3].player;
			if( mode > -1 && (mode & (1<<2)) ){
				if( instance->targets[3].card > -1 ){
					reset_subtypes(p, c, 3);
				}
				real_humiliation(p, c, 0);
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	return 0;
}
static void set_humiliation_legacy(int player, int card, int mode, test_definition_t *hc){
	card_instance_t *instance = get_card_instance(player, card);
	instance->targets[1].player = hc != NULL ? hc->power : 1;
	instance->targets[1].card = hc != NULL ? hc->toughness : 1;
	instance->targets[2].player = hc != NULL ? hc->keyword : 0;
	instance->targets[2].card = hc != NULL ? hc->color : 0;
	instance->targets[3].player = mode;
	instance->targets[3].card = hc != NULL ? hc->subtype : 0;
}

int humiliate_and_set_pt_abilities(int player, int card, int t_player, int t_card, int mode, test_definition_t *hc){
	// mode = 0: Reactivate all abilities.
	// mode = 1: Remove all abilities.
	// mode = 2: Remove all abilities, and attach and return a legacy that sets power/toughness; removes keywords; maybe sets color;
	// mode = 4: humiliation ends at eot
	int legacy = -1;

	if (mode){
		real_humiliation(t_player, t_card, 1);
		if( mode & ((1<<1) | (1<<2)) ){
			disable_other_pt_setting_effects_attached_to_me(t_player, t_card);

			int p, c;
			for (p = 0; p <= 1; ++p){
				for (c = 0; c < active_cards_count[p]; ++c){
					card_instance_t* inst = get_card_instance(p, c);
					if (inst->internal_card_id == LEGACY_EFFECT_CUSTOM
						&& (inst->info_slot == (int)humility_legacy || inst->info_slot == (int)set_pt_legacy)
						&& (inst->state & (STATE_OUBLIETTED|STATE_IN_PLAY)) == STATE_IN_PLAY
						&& inst->damage_target_player == t_player
						&& inst->damage_target_card == t_card
					  ){
						inst->token_status &= ~STATUS_CONTROLLED;
					}
				}

			}

			legacy = create_targetted_legacy_effect(player, card, &humility_legacy, t_player, t_card);
			add_status(player, legacy, STATUS_CONTROLLED);
			set_humiliation_legacy(player, legacy, mode, hc);
		}
	}
	else{
		real_humiliation(t_player, t_card, 0);
	}
	return legacy;
}

int humiliate(int src_player, int src_card, int t_player, int t_card, int mode){
	return humiliate_and_set_pt_abilities(src_player, src_card, t_player, t_card, mode, NULL);
}

/****************************
* Abilities in sp_keyword_t *
****************************/

// static int block_if_able(int player, int card, event_t event);
static void lure_effect(int player, int card, event_t event, int luring_player, int luring_card, int all_must_block, int subtype, const char* prompt);

void special_abilities(int player, int card, event_t event, int sp_abilities, int source_player, int source_card){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[16].card < 0 ){
		instance->targets[16].card = 0;
	}

	if( sp_abilities & SP_KEYWORD_FEAR ){
		fear(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_LIFELINK ){
		lifelink(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_HASTE ){
		haste(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_VIGILANCE ){
		vigilance(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_UNBLOCKABLE ){
		unblockable(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_CANNOT_BLOCK ){
		cannot_block(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_SHADOW ){
		shadow(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_DEATHTOUCH ){
		deathtouch(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_MUST_ATTACK ){
		attack_if_able(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_INDESTRUCTIBLE){
		indestructible(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_MUST_BE_BLOCKED){
		lure_effect(source_player, source_card, event, player, card, 0, -1, NULL);
	}
	if (sp_abilities & SP_KEYWORD_FLANKING){
		granted_flanking(source_player, source_card, player, card, event);
	}
	if (sp_abilities & SP_KEYWORD_WITHER){
		wither(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_LURE){
		lure_effect(source_player, source_card, event, player, card, 1, -1, NULL);
	}

	if( sp_abilities & SP_KEYWORD_HEXPROOF){
		hexproof(player, card, event);
	}

	if( sp_abilities & SP_KEYWORD_INTIMIDATE){
		intimidate(player, card, event);
	}
	if( sp_abilities & SP_KEYWORD_MUST_BLOCK ){
		block_if_able(player, card, event);
	}
	if (sp_abilities & SP_KEYWORD_SHADOW_HOSER){
		instance->targets[16].card |= SP_KEYWORD_SHADOW_HOSER;
	}

	if( sp_abilities & SP_KEYWORD_MENACE ){
		menace(player, card, event);
	}

	if( sp_abilities & SP_KEYWORD_SKULK ){
		skulk(player, card, event);
	}

	if( sp_abilities & SP_KEYWORD_HORSEMANSHIP ){
		horsemanship2(player, card, event);
	}
}

// Single-bit abilities

#define SP_ABILITY_SUPPRESSABLE(fn_name, bit, suppress_bit)			\
  void fn_name(int player, int card, event_t event)					\
{																	\
  if (event == EVENT_ABILITIES && affect_me(player, card))			\
	{																\
	  card_instance_t* instance = get_card_instance(player, card);	\
																	\
	  if (instance->targets[16].card == -1)							\
		instance->targets[16].card = 0;								\
																	\
	  if (player_bits[player] & suppress_bit)						\
		instance->targets[16].card &= ~bit;							\
	  else															\
		instance->targets[16].card |= bit;							\
	}																\
}

#define SP_ABILITY(fn_name, bit)									\
void fn_name(int player, int card, event_t event)					\
{																	\
  if (event == EVENT_ABILITIES && affect_me(player, card))			\
	{																\
	  card_instance_t* instance = get_card_instance(player, card);	\
																	\
	  if (instance->targets[16].card == -1)							\
		instance->targets[16].card = 0;								\
																	\
	  instance->targets[16].card |= bit;							\
	}																\
}

SP_ABILITY(cannot_block, SP_KEYWORD_CANNOT_BLOCK);
SP_ABILITY_SUPPRESSABLE(deathtouch, SP_KEYWORD_DEATHTOUCH, PB_CANT_HAVE_OR_GAIN_DEATHTOUCH);
SP_ABILITY(fear, SP_KEYWORD_FEAR);
SP_ABILITY_SUPPRESSABLE(hexproof, SP_KEYWORD_HEXPROOF, PB_CANT_HAVE_OR_GAIN_HEXPROOF);
SP_ABILITY(horsemanship2, SP_KEYWORD_HORSEMANSHIP);
SP_ABILITY(indestructible, SP_KEYWORD_INDESTRUCTIBLE);
SP_ABILITY(intimidate, SP_KEYWORD_INTIMIDATE);
SP_ABILITY(lifelink, SP_KEYWORD_LIFELINK);
SP_ABILITY(rfg_when_damage, SP_KEYWORD_RFG_WHEN_DAMAGE);
SP_ABILITY(shadow, SP_KEYWORD_SHADOW);
SP_ABILITY(unblockable, SP_KEYWORD_UNBLOCKABLE);
SP_ABILITY(wither, SP_KEYWORD_WITHER);

#undef SP_ABILITY
#undef SP_ABILITY_SUPPRESSABLE

// Usually pump_ability_until_eot() is better, but this can go on non-creatures too (e.g. Boros Charm).
int indestructible_until_eot(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card))
		indestructible(instance->damage_target_player, instance->damage_target_card, event);
	}

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

// Protection
void protection_from_creatures(int player, int card, event_t event)
{
  unblockable(player, card, event);

  card_instance_t* damage = damage_being_prevented(event);
  if (damage
	  && damage->damage_target_card == card && damage->damage_target_player == player
	  && (damage->targets[3].player & TYPE_CREATURE))
	damage->info_slot = 0;
}

// Regeneration

int can_be_regenerated(int player, int card)	// by something else.
{
  card_instance_t* instance = in_play(player, card);

  return ((!instance	// not in play
		   || instance->kill_code != KILL_DESTROY	// not being destroyed (or is being buried)
		   || (instance->token_status & STATUS_CANNOT_REGENERATE))
		  ? 0 : 99);
}

int can_regenerate(int player, int card){
	if ((land_can_be_played & LCBP_REGENERATION)	// This seems actively unhelpful - it prevents regeneration_shield_legacy() from working, for instance.
		&& can_be_regenerated(player, card)
		&& can_use_activated_abilities(player, card)){
		return 99;
	} else {
		return 0;
	}
}

int regenerate_target(int player, int card){
	if( player != -1 && card != -1 && in_play(player, card) ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->kill_code == KILL_DESTROY && ! (instance->token_status & STATUS_CANNOT_REGENERATE) ){
			regenerate_target_exe(player, card);
		}
	}
	return 0;
}

int regeneration(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		if( can_regenerate(instance->parent_controller, instance->parent_card) ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_REGENERATION, cless, black, blue, green, red, white, 0, NULL, NULL);
}

int regeneration_shield_legacy(int player, int card, event_t event){
	/* targets[0]: Permanent this is attached to (also in card_instance_t::damage_target_player/card) */

	card_instance_t *instance = get_card_instance(player, card);

	if (instance->targets[0].player >= 0){
		if (instance->targets[1].player < 0){
			instance->targets[1].player = 0;
		}
		if (event == EVENT_ABILITIES && affect_me(instance->targets[0].player, instance->targets[0].card)){
			event_result |= KEYWORD_REGENERATION;	// mostly cosmetic, though its presence affects the AI
		}

		static int regeneration_from_shield_in_progress = 0;	// May not be necessary
		if (can_be_regenerated(instance->targets[0].player, instance->targets[0].card)
			&& !regeneration_from_shield_in_progress){
			++regeneration_from_shield_in_progress;
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
			kill_card(player, card, KILL_REMOVE);
			--regeneration_from_shield_in_progress;
		}
	}

	if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int regenerate_or_shield(int player, int card, int t_player, int t_card){
	if ((player == card && t_player == t_card) ? can_regenerate(player, card) : can_be_regenerated(player, card)){
		regenerate_target(t_player, t_card);
		return -1;
	} else {
		return create_targetted_legacy_effect(player, card, &regeneration_shield_legacy, t_player, t_card);
	}
}

static int legacy_cannot_regenerate_until_eot(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( eot_trigger(player, card, event) ){
		remove_status(instance->targets[0].player, instance->targets[0].card, STATUS_CANNOT_REGENERATE);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int cannot_regenerate_until_eot(int player, int card, int t_player, int t_card){
	add_status(t_player, t_card, STATUS_CANNOT_REGENERATE);
	int leg = create_targetted_legacy_effect(player, card, &legacy_cannot_regenerate_until_eot, t_player, t_card);
	int leg2 = create_legacy_effect_exe(player, card, LEGACY_EFFECT_GENERIC, t_player, t_card);
	if (leg2 != -1){
		// exe looks, not for STATUS_CANNOT_REGENERATE on a card trying to regenerate, but a LEGACY_EFFECT_GENERIC with STATUS_CANNOT_REGENERATE attached to the card
		card_instance_t* legacy = get_card_instance(player, leg2);
		legacy->token_status |= STATUS_CANNOT_REGENERATE | STATUS_INVISIBLE_FX;
		// will be removed at eot since STATUS_PERMANENT isn't set
	}
	return leg;
}

// Other abilities

int attack_if_able(int player, int card, event_t event){
	if (event == EVENT_MUST_ATTACK && current_turn == player && ! forbid_attack ){
		card_instance_t* attacker = get_card_instance(player, card);
		if (!(attacker->state & STATE_UNKNOWN8000)){
			attacker->state |= STATE_UNKNOWN8000;
			if (can_attack(player, card)){
				must_attack = 1;
			}
		}
	}
	return 0;
}

static int can_block_target_player, can_block_target_card;
static const char* can_block_target(int who_chooses, int player, int card)
{
  if (can_block_me(player, card, can_block_target_player, can_block_target_card))
	return NULL;
  else
	return EXE_STR(0x5b8580);	// localized "Illegal block." from PROMPT_DEFENDWHOM
}

int can_block(int player, int card){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = 1-player;
	td.allowed_controller = 1-player;
	td.allow_cancel = 0;
	td.required_state = TARGET_STATE_ATTACKING;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)can_block_target;
	can_block_target_player = player;
	can_block_target_card = card;
	return can_target(&td);
}

void select_blocker(int player, int card, int player_who_chooses_blockers){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = 1-player;
	td.allowed_controller = 1-player;
	td.allow_cancel = 0;
	td.who_chooses = player_who_chooses_blockers;
	td.required_state = TARGET_STATE_ATTACKING;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int32_t)can_block_target;
	can_block_target_player = player;
	can_block_target_card = card;

	card_instance_t *instance = get_card_instance(player, card);
	while( instance->blocking >= 255 && can_target(&td) ){
			instance->number_of_targets = 0;
			if( pick_target(&td, "PROMPT_DEFENDWHOM") ){
				block(player, card, 1-player, instance->targets[0].card);
			}
	}
}

int block_if_able(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[16].card < 0 ){
		instance->targets[16].card = 0;
	}

	if( ! (instance->targets[16].card & SP_KEYWORD_MUST_BLOCK) ){
		instance->targets[16].card |= SP_KEYWORD_MUST_BLOCK;
	}

	if( current_turn != player && event == EVENT_DECLARE_BLOCKERS ){
		select_blocker(player, card, player);
	}

	return 0;
}

const char* target_must_be_able_to_block_me(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return get_card_instance(player, card)->blocking == 255 && is_legal_block(player, card, targeting_player, targeting_card) ? NULL : EXE_STR(0x5b8580);	// localized "Illegal block." from PROMPT_DEFENDWHOM
}

static void lure_effect(int player, int card, event_t event, int luring_player, int luring_card, int all_must_block, int subtype, const char* prompt)
{
  card_instance_t* luring;
  int who_chooses;
  if (trigger_condition == TRIGGER_MUST_BLOCK
	  && (event == EVENT_TRIGGER || event == EVENT_RESOLVE_TRIGGER)
	  && affect_me(player, card)
	  && luring_player == current_turn
	  && reason_for_trigger_controller == (who_chooses = (event_flags & EF_ATTACKER_CHOOSES_BLOCKERS) ? current_turn : 1 - current_turn)
	  && (luring = in_play(luring_player, luring_card))
	  && (luring->state & STATE_ATTACKING)
	  && (all_must_block || is_unblocked(luring_player, luring_card)))
	{
	  target_definition_t td;
	  default_target_definition(luring_player, luring_card, &td, 0);
	  td.allowed_controller = 1-luring_player;
	  td.preferred_controller = 1-luring_player;
	  td.who_chooses = who_chooses;
	  td.required_subtype = subtype >= 0 ? subtype : -1;
	  td.illegal_abilities = 0;
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION | TARGET_SPECIAL_ALLOW_MULTIBLOCKER;
	  td.extra = (int32_t)target_must_be_able_to_block_me;

	  if (!can_target(&td))
		return;

	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  char fullprompt[200];
		  if (ai_is_speculating == 1)
			*fullprompt = 0;
		  else
			{
			  if (!(prompt && *prompt))
				{
				  load_text(0, "PROMPT_CHOOSEBLOCKERS");
				  prompt = text_lines[0];
				}
			  int csvid;
			  if (is_what(player, card, TYPE_EFFECT))
				csvid = get_card_instance(player, card)->display_pic_csv_id;
			  else
				csvid = get_id(player, card);
			  scnprintf(fullprompt, 200, "%s: %s", cards_ptr[csvid]->full_name, prompt);
			}

		  target_t tgt;
		  int old_number_of_targets = luring->number_of_targets;	// Since select_target() overwrites it
		  trigger_condition = -1;	// Otherwise other lure effects on the battlefield trigger from within select_target(), perplexingly enough

		  int result = select_target(luring_player, luring_card, &td, fullprompt, &tgt);

		  trigger_condition = TRIGGER_MUST_BLOCK;
		  luring->number_of_targets = old_number_of_targets;

		  if (result)
			{
			  block(tgt.player, tgt.card, luring_player, luring_card);
			  if (all_must_block && can_target(&td))
				// More triggers left; but let the defending player activate it one at a time so he can choose blocking in the case of multiple Lures.
				get_card_instance(player, card)->state &= ~STATE_PROCESSING;
			}
		  else
			get_card_instance(player, card)->state &= ~STATE_PROCESSING;
		}
	}
}

int everybody_must_block_me(int player, int card, event_t event)
{
  lure_effect(player, card, event, player, card, 1, -1, NULL);
  return 0;
}

void subtype_must_block_me(int player, int card, event_t event, subtype_t subtype, const char* prompt)
{
  lure_effect(player, card, event, player, card, 1, subtype, prompt);
}

int must_be_blocked(int player, int card, event_t event)
{
  lure_effect(player, card, event, player, card, 0, -1, NULL);
  return 0;
}

static int target_must_block_me_legacy(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_MUST_BLOCK
	  && (event == EVENT_TRIGGER || event == EVENT_RESOLVE_TRIGGER)
	  && affect_me(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card), *luring, *lured;

	  int luring_player = instance->damage_source_player;
	  int luring_card = instance->damage_source_card;

	  int lured_player = instance->damage_target_player;
	  int lured_card = instance->damage_target_card;

	  if (luring_player == current_turn
		  && reason_for_trigger_controller == ((event_flags & EF_ATTACKER_CHOOSES_BLOCKERS) ? current_turn : 1 - current_turn)
		  && (luring = in_play(luring_player, luring_card))
		  && (luring->state & STATE_ATTACKING)

		  && luring_player != lured_player
		  && (lured = in_play(lured_player, lured_card))
		  && lured->blocking == 255
		  && is_legal_block(lured_player, lured_card, luring_player, luring_card))
		{
		  if (event == EVENT_TRIGGER)
			event_result |= RESOLVE_TRIGGER_MANDATORY;

		  if (event == EVENT_RESOLVE_TRIGGER)
			block(lured_player, lured_card, luring_player, luring_card);
		}
	}

  if (event == EVENT_CLEANUP && (get_card_instance(player, card)->targets[2].player & 1))
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int target_must_block_me(int player, int card, int t_player, int t_card, int mode){
	// bitfields_for_mode
	// 1 --> Effect ends at eot
	int legacy_card = create_targetted_legacy_effect(player, card, &target_must_block_me_legacy, t_player, t_card);
	card_instance_t *legacy_instance = get_card_instance(player, legacy_card);
	legacy_instance->targets[2].player = mode;
	return legacy_card;
}

int exalted(int player, int card, event_t event, int ability, int sp_ability)
{
  if (declare_attackers_trigger(player, card, event, DAT_ATTACKS_ALONE|DAT_TRACK, player, -1))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int attacker = BYTE0(instance->targets[2].player);
	  if (in_play(player, attacker))
		pump_ability_until_eot(player, card, player, attacker, 1, 1, ability, sp_ability);

	  return attacker;
	}

  return -1;
}

int check_for_flanking_removal(int player, int card){
	if( ! check_state(player, card, STATE_ATTACKING | STATE_ATTACKED) ){
		return 0;
	}
	int c;
	for(c=0; c<active_cards_count[1-player]; c++){
		if( in_play(1-player, c) && get_id(1-player, c) == CARD_ID_BARBED_FOLIAGE && ! is_humiliated(1-player, c) ){
			return 1;
		}
	}
	return 0;
}

void granted_flanking(int s_player, int s_card, int t_player, int t_card, event_t event)
{
  if (event == EVENT_ABILITIES && affect_me(t_player, t_card))
	{
		if( ! check_for_flanking_removal(t_player, t_card) ){
			card_instance_t* instance = get_card_instance(t_player, t_card);
			if (instance->targets[16].card < 0){
				instance->targets[16].card = 0;
			}
			instance->targets[16].card |= SP_KEYWORD_FLANKING;
		}
	}

  if (event == EVENT_DECLARE_BLOCKERS && is_attacking(t_player, t_card) &&
	check_for_special_ability(t_player, t_card, SP_KEYWORD_FLANKING) )
	{
	  char marked[2][151] = {{0}};
	  mark_each_creature_blocking_me(t_player, t_card, marked);
	  int c;
	  for (c = 0; c < active_cards_count[1-current_turn]; ++c)
		if (marked[1-current_turn][c])
		  alternate_legacy_text(1, s_player, pump_until_eot(s_player, s_card, 1-current_turn, c, -1, -1));
	}
}

void flanking(int player, int card, event_t event)
{
  granted_flanking(player, card, player, card, event);
}

int flash(int player, int card, event_t event){
	// Must also be set as type instant
	if (event == EVENT_CAN_CAST){
		return 1;
	} else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		ai_modifier -= 16;
	}
	return 0;
}

int freeze_when_damage(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[16].card < 0 ){
		instance->targets[16].card = 0;
	}

	instance->targets[16].card |= SP_KEYWORD_FREEZE_WHEN_DAMAGE;

	if( !is_humiliated(player, card) ){
		damage_effects(player, card, event);

		if( instance->targets[1].player > 2 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == player
		  ){
			int i;
			for(i=2; i<instance->targets[1].player; i++){
				effect_frost_titan(player, card, instance->targets[i].player, instance->targets[i].card);
			}
			instance->targets[1].player = 2;
		}

		if( event == EVENT_CLEANUP ){
			instance->targets[1].player = 2;
		}
	}
	return 0;
}

void give_haste(int player, int card)	// Avoid!
{
  card_instance_t* instance = in_play(player, card);
  if (instance)
	instance->state &= ~STATE_SUMMON_SICK;
}

void haste(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES && affect_me(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (instance->targets[16].card == -1)
		instance->targets[16].card = 0;

	  instance->targets[16].card |= SP_KEYWORD_HASTE;

	  if (!is_humiliated(player, card))
		instance->state &= ~STATE_SUMMON_SICK;
	}
}

void persist(int player, int card, event_t event)
{
  if (count_minus1_minus1_counters(player, card) <= 0)
	{
	  nice_creature_to_sacrifice(player, card);

	  int owner, position;
	  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)
		  && find_in_owners_graveyard(player, card, &owner, &position))
		reanimate_permanent(owner, -1, owner, position, REANIMATE_MINUS1_MINUS1_COUNTER);	// -1 for card doesn't abort since no legacy is added
	}
}

void vigilance(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES && affect_me(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (instance->targets[16].card == -1)
		instance->targets[16].card = 0;

	  instance->targets[16].card |= SP_KEYWORD_VIGILANCE;

	  if (!is_humiliated(player, card))
		instance->state |= STATE_VIGILANCE;	// Can't quite be omitted yet.
	}
}

/*************************
* Detection of abilities *
*************************/

int check_for_ability(int player, int card, int keyword){
	if( card != -1 ){
		if( get_abilities(player, card, EVENT_ABILITIES, -1) & keyword ){
			return 1;
		}
	}
	else{
		int count = 0;
		while( count < active_cards_count[player] ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
					if( get_abilities(player, count, EVENT_ABILITIES, -1) & keyword ){
						return 1;
					}
				}
				count++;
		}
	}
	return 0;
}

int get_special_abilities_by_instance(card_instance_t* instance)
{
  if (is_humiliated_by_instance(instance))
	return 0;

  if (instance->targets[16].card < 0)
	instance->targets[16].card = 0;

  if (instance->state & STATE_VIGILANCE)
	instance->targets[16].card |= SP_KEYWORD_VIGILANCE;

  int keyword = instance->targets[16].card;


 // Obsolete
//  if ((instance->targets[14].card & SF_FLANKING_REMOVED) && instance->targets[14].card != -1)
//	keyword &= ~SP_KEYWORD_FLANKING;

  return keyword;
}

int get_special_abilities(int player, int card){
	return get_special_abilities_by_instance(get_card_instance(player, card));
}

int check_for_special_ability(int player, int card, int keyword)
{
  return get_special_abilities_by_instance(get_card_instance(player, card)) & keyword;
}

void remove_special_ability(int player, int card, int sa)
{
	int s_keyword = get_card_instance(player, card)->targets[16].card;
	if( s_keyword > -1 && (s_keyword & sa) ){
		get_card_instance(player, card)->targets[16].card &= ~sa;
	}
}

void set_special_abilities(int player, int card, int type, int value){
	if( card != -1 ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->targets[16].card < 0 ){
			instance->targets[16].card = 0;
		}
		instance->targets[16].card |= value;
	}
	else{
		int i;
		for(i=0; i<2; i++){
			if( i == player || player == 2 ){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && is_what(i, count, type) ){
							card_instance_t *instance = get_card_instance( i, count );
							if( instance->targets[16].card < 0 ){
								instance->targets[16].card = 0;
							}
							instance->targets[16].card |= value;
						}
						count--;
				}
			}
		}
	}
}

int legacy_negate_ability_and_special_ability(int player, int card, event_t event){

	if (event == EVENT_ABILITIES && check_status(player, card, STATUS_CONTROLLED) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if (affect_me(instance->damage_target_player, instance->damage_target_card)){
				if( instance->targets[1].player > -1 ){
					event_result &= ~instance->targets[1].player;
				}
				if( instance->targets[1].card > -1 ){
					remove_special_ability(player, card, instance->targets[1].card);
				}
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->targets[1].player > -1 && (instance->targets[1].player & 1) ){
			effect_on_all_negate_ability_and_special_ability_legacy_attached_to_me(instance->damage_target_player, instance->damage_target_card, 1);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int effect_on_all_negate_ability_and_special_ability_legacy_attached_to_me(int player, int card, int effect){
// effect = 0 --> check only
// effect = 1 -> enable the "control legacy" with the highest timestamp (AKA the most recent)
// effect = -1 -> disable all "control legacies"
	int result = 0;
	int legacies[100][3];
	int lc = 0;
	card_instance_t* inst;
	int p, c;
	for (p = 0; p <= 1; ++p){
		for (c = active_cards_count[p]; c > -1; c--){
			if ( (inst = in_play(p, c)) && inst->damage_target_player == player && inst->damage_target_card == card ){
				if( is_what(p, c, TYPE_EFFECT) && inst->info_slot == (int)legacy_negate_ability_and_special_ability ){
					if( effect == 1 && ! check_status(p, c, STATUS_CONTROLLED) ){
						legacies[lc][0] = inst->timestamp;
						legacies[lc][1] = p;
						legacies[lc][2] = c;
						lc++;
						result = 1;
					}
					if( effect == -1 ){
						remove_status(p, c, STATUS_CONTROLLED);
						result = 1;
					}
				}
			}
		}
	}
	if( lc ){
		int max_timestamp = 0;
		int id_of_legacy_to_activate = 0;
		int i;
		for(i=0; i<lc; i++){
			if( legacies[i][0] > max_timestamp ){
				max_timestamp = legacies[i][0];
			}
			id_of_legacy_to_activate = i;
		}
		add_status(legacies[id_of_legacy_to_activate][1], legacies[id_of_legacy_to_activate][2], STATUS_CONTROLLED);
	}
	return result;
}

static int create_negate_ability_and_special_ability_until_eot(int player, int card, int t_player, int t_card, keyword_t keyword,
	int sp_ability)
{
	effect_on_all_negate_ability_and_special_ability_legacy_attached_to_me(t_player, t_card, -1);
	int legacy = create_targetted_legacy_effect(player, card, &legacy_negate_ability_and_special_ability, t_player, t_card);
	card_instance_t *leg = get_card_instance(player, legacy);
	leg->targets[1].player = keyword;
	leg->targets[1].card = sp_ability;
	leg->targets[2].player = 1;
	return legacy;
}

/* Affects all of t_player's creatures if t_card == -1, and all creatures if t_player == 2 as well.  t_player == 2, t_card != -1 is undefined behavior.  Returns
 * created legacy if t_player != 2 and t_card != -1 (i.e., a single creature), otherwise -1. */
static int negate_ability_and_special_ability_until_eot_impl(int player, int card, int t_player, int t_card, keyword_t keyword, int s_keyword){
	int i;
	for(i=0; i<2; i++){
		if( i == t_player || t_player == ANYBODY ){
			if( t_card != -1 ){
				return create_negate_ability_and_special_ability_until_eot(player, card, t_player, t_card, keyword, s_keyword);
			}
			else{
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
							create_negate_ability_and_special_ability_until_eot(player, card, i, count, keyword, s_keyword);
						}
						count--;
				}
			}
		}
	}
	return -1;
}

int negate_ability_until_eot(int player, int card, int t_player, int t_card, keyword_t keyword){
	return negate_ability_and_special_ability_until_eot_impl(player, card, t_player, t_card, keyword, 0);
}

int negate_ability_and_special_ability_until_eot(int player, int card, int t_player, int t_card, keyword_t keyword, int s_keyword){
	return negate_ability_and_special_ability_until_eot_impl(player, card, t_player, t_card, keyword, s_keyword);
}
