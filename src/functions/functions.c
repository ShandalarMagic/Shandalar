#include "manalink.h"
#include <windows.h>

int discard_controller = 0;

unsigned int num_bits_set(unsigned int v){
  // From Bit Twiddling Hacks, http://graphics.stanford.edu/~seander/bithacks.html#CountBitsSetParallel
  v = v - ((v >> 1) & 0x55555555);                              // reuse input as temporary
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);               // temp
  return (((v + (v >> 4)) & 0xF0F0F0F) * 0x1010101) >> 24;      // count
}

int get_attack_power(int player, int card){
	card_instance_t* inst;
	int i;
	for(i=0; i<2; i++){
		int count;
		for( count = 0; count < active_cards_count[i]; ++count ){
				if( (inst = in_play(i, count)) ){
					switch(cards_data[inst->internal_card_id].id){
						case CARD_ID_ASSAULT_FORMATION:
						case CARD_ID_THE_GREAT_FOREST:
							if(i != player){
								break;
							}
							// else fall through
						case CARD_ID_DORAN_THE_SIEGE_TOWER:
							if(!is_humiliated(i, count)){
								return get_toughness(player, card);
							}
							break;
					}
				}
		}
	}
	return get_power(player, card);
}

int get_discard_controller(void){
	return discard_controller;
}


void set_discard_flag(int player, int card){
	raw_set_poison(HUMAN, 3);
   // raw_set_poison(AI, 1+card);
   // life[AI] = 20+event;
   life[AI] = reason_for_trigger_controller;
   life[HUMAN] = card_on_stack_controller;
	discard_controller = player;
	return;
}

int duh_mode(int player){
	return IS_AI(player) || get_setting(SETTING_DUH_MODE);
}

int dealt_damage_to_player(int player, int card, int parent_player, int parent_card, int event){
	//card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		card_instance_t *parent_instance = get_card_instance(parent_player, parent_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->damage_source_player == player ){
				if(  damage->damage_source_card == card && damage->info_slot > 0 ){
					parent_instance->targets[16].player = damage->info_slot;
				}
			}
		}
	}

	card_instance_t *parent_instance = get_card_instance(parent_player, parent_card);
	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(parent_player, parent_card) &&
		reason_for_trigger_controller == parent_player && parent_instance->targets[16].player > 0 ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				parent_instance->targets[16].player = 0;
				return 1;
		}
	}
	return 0;
}

int is_valid_card(int id){
	if( id < 0 || id > (available_slots-1) ){
		return 0;
	}
	if( card_coded[id/8] & (1<<(id%8)) ){
		int crd = get_internal_card_id_from_csv_id( id );
		card_ptr_t* c = cards_ptr[ id ];
		// no tokens or vanguard cardss
		if( cards_data[crd].cc[2] != 3 && c->expansion != 4096 && cards_data[crd].type != TYPE_EFFECT ){
			// ante cards
			if( id != CARD_ID_CONTRACT_FROM_BELOW && id != CARD_ID_DARKPACT && id != CARD_ID_BRONZE_TABLET &&
				id != CARD_ID_JEWELED_BIRD && id != CARD_ID_DEMONIC_ATTORNEY && id != CARD_ID_REBIRTH &&
				id != CARD_ID_TEMPEST_EFREET
			  ){
				// take out the rules engine and a 'dummy' card
				if( id != 916 && id != CARD_ID_RULES_ENGINE && id != CARD_ID_DEADBOX ){
					return 1;
				}
			}
		}
   }
   return 0;
}

int has_combat_damage_been_inflicted_to_opponent(int player, int card, event_t event){
	return damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_OPPONENT);
}

int has_combat_damage_been_inflicted_to_a_player(int player, int card, event_t event){
	return damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER);
}

int deathtouch_until_eot(int player, int card, event_t event){

	if (event == EVENT_ABILITIES){
		card_instance_t *instance = get_card_instance( player, card );
		deathtouch(instance->targets[0].player, instance->targets[0].card, event);
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int get_sum(int player, type_t type){
	int p = player;
	int count = 0;
	int sum = 0;
	while (count < active_cards_count[p]){
		if (in_play(p, count) && is_what(p, count, type)){
			sum += count;	// This doesn't do what you probably think it does.
		}
		count++;
	}
	return sum;
}

int get_hand_sum(int player, type_t type){
	int p = player;
	int count = 0;
	int sum = 0;
	while (count < active_cards_count[p]){
		if (in_hand(p, count) && is_what(p, count, type)){
			sum += count;	// This doesn't do what you probably think it does.
		}
		count++;
	}
	return sum;
}

const char* get_card_name_by_id(int csvid)
{
  return cards_ptr[csvid]->name;
}

const char* get_card_name_by_instance(card_instance_t* instance)
{
  return cards_ptr[cards_data[instance->internal_card_id].id]->name;
}

const char* get_card_or_subtype_name(int subtype_or_csvid)
{
  if (subtype_or_csvid < 0)
	return cards_ptr[-(subtype_or_csvid + 1)]->name;
  else if (subtype_or_csvid <= 232)	// Should be the number in Text.res - 1 (since indexed from 0).  Good to a maximum of 249 before it needs splitting.  If this number is changed, then grep for it and adjacent numberals.  There's currently four occurrences in has_creature_type.c.
	return strs_hunting_subtypenames[subtype_or_csvid];
#if 0
  else if (subtype_or_csvid <= 499)
	return strs_hunting_subtypenames_2[subtype_or_csvid - 250];	// extern char strs_hunting_subtypenames_2[250][300], to be initialized in initialize_ability_tooltip_names()
#endif
  else
	return "";
}

static int id_updater_for_reveal_top_card_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		card_instance_t *leg = get_card_instance(instance->damage_target_player, instance->damage_target_card);
		if( leg->damage_target_player > -1 ){
			int* deck = deck_ptr[leg->damage_target_player];
			if( deck[0] != -1 ){
				leg->info_slot = -cards_data[deck[0]].id-1;
			}
		}
	}
	return 0;
}

int reveal_top_card(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int* deck = deck_ptr[player];
		if( deck[0] != -1 ){
			card_data_t* card_d = &cards_data[ deck[0] ];
			int legacy = create_card_name_legacy(player, card, card_d->id );
			int leg2 = create_targetted_legacy_effect(player, card, &id_updater_for_reveal_top_card_legacy, player, legacy);
			add_status(player, leg2, STATUS_INVISIBLE_FX);
		}
	}

	return 0;
}

int get_random_card(void){
	int i;
	int crd = -1;
	for(i=0; i<100; i++){
		crd+=100*internal_rand(16);
	}
	if( crd > (available_slots-1) ){
		crd -= (available_slots-1);
	}

	if( is_valid_card(crd) ){
		return crd;
	}
	else{
		while(1){
			crd++;
			if( is_valid_card(crd) ){
				return crd;
			}
		}
	}
	return -1;
}

// Can a card with this iid be played (other than mana cost and timing restrictions)?
int can_legally_play_iid(int player, int internal_card_id)
{
  card_data_t* card_d = &cards_data[internal_card_id];

  // Lands can only be played once per turn, and only during your turn.
  if ((card_d->type & TYPE_LAND)
	  && ((land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED) || (current_turn != player)))
	return 0;

  // Instants, interrupts, sorceries, and enchantments (including permanents with flash and planeswalkers) must return true from EVENT_CAN_CAST.
  if (card_d->type & (TYPE_INSTANT | TYPE_INTERRUPT | TYPE_SORCERY | TYPE_ENCHANTMENT))	// deliberately not is_what(), so flash cards returning 99 are correct
	{
	  /* Calling code pointers directly is DANGEROUS; exe and especially asm cards make assumptions about what's in each register.  One of the things needed is
	   * a valid card instance, so we have to create a card.  Besides, it's needed so targeting works right; and some especially misbehaving C cards set values
	   * on the card during EVENT_CAN_CAST. */
	  int card_added = add_card_to_hand(player, internal_card_id);
	  card_instance_t* instance = get_card_instance(player, card_added);
	  instance->state |= STATE_INVISIBLE;	--hand_count[player];
	  int rval = call_card_fn((void*)(card_d->code_pointer), instance, player, card_added, EVENT_CAN_CAST);
	  obliterate_card_and_recycle(player, card_added);
	  return rval;
	}

  // A (non-flash) creature, artifact, or land (when one hasn't been played, and it's your turn).
  return 1;
}

// Can a card with this iid be played (other than mana cost)?
int can_legally_play_iid_now(int player, int internal_card_id, event_t event)
{
  if ((cards_data[internal_card_id].type & (TYPE_INSTANT | TYPE_INTERRUPT))	// deliberately not is_what(), to include permanents with flash
	  || can_sorcery_be_played(player, event))
	return can_legally_play_iid(player, internal_card_id);
  else
	return 0;
}

// Can a card with this csvid be played (other than mana cost and timing restrictions)?
int can_legally_play_csvid(int player, int csvid)
{
  return can_legally_play_iid(player, get_internal_card_id_from_csv_id(csvid));
}

// Can a card with this csvid be played (other than mana cost)?
int can_legally_play_csvid_now(int player, int csvid, event_t event)
{
  return can_legally_play_iid_now(player, get_internal_card_id_from_csv_id(csvid), event);
}

int can_legally_play_card_in_hand(int player, int card)
{
  card_data_t* card_d = &cards_data[get_card_instance(player, card)->internal_card_id];

  // Lands can only be played once per turn, and only during your turn.
  if ((card_d->type & TYPE_LAND)
	  && ((land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED) || (current_turn != player)))
	return 0;

  // Instants, interrupts, sorceries, and enchantments (including permanents with flash and planeswalkers) must return true from EVENT_CAN_CAST.
  if (card_d->type & (TYPE_INSTANT | TYPE_INTERRUPT | TYPE_SORCERY | TYPE_ENCHANTMENT))	// deliberately not is_what(), so flash cards returning 99 are correct
	{
		return call_card_function(player, card, EVENT_CAN_CAST);
	}

  // A (non-flash) creature, artifact, or land (when one hasn't been played, and it's your turn).
  return 1;
}

static int is_aura(int player, int card ){
	if( has_subtype(player, card, SUBTYPE_AURA) ){
		return 1;
	}
	return 0;
}

/* Exiles {t_player,t_card} and then returns it to the battlefield under its owner's control.  If before_etb is non-null, call that function with the card just
 * before it re-enters the battlefield. */
void blink_effect(int t_player, int t_card, void (*before_etb)(int player, int card))
{
	if( in_play(t_player, t_card) ){
		int token = is_token(t_player, t_card);
		int iid = get_original_internal_card_id(t_player, t_card);
		int owner = get_owner(t_player, t_card);
		kill_card(t_player, t_card, KILL_REMOVE);
		if( ! token && is_what(-1, iid, TYPE_PERMANENT) ){
			int card_added = add_card_to_hand(owner, iid);
			int good = 1;
			if( is_aura(owner, card_added) ){
				good = call_card_function(owner, card_added, EVENT_CAN_CAST);
				if( good ){
					set_special_flags3(owner, card_added, SF3_IN_PLAY_DIRECTLY);
				}
			}
			if( good ){
				remove_card_from_rfg(owner, cards_data[iid].id);
				if (before_etb){
					(*before_etb)(owner, card_added);
				}
				put_into_play(owner, card_added);
			}
			else{
				obliterate_card(owner, card_added);
			}
		}
	}
}

static int remove_until_eot_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if (instance->targets[1].card && eot_trigger(player, card, event)){
		if( check_rfg(instance->targets[0].player, cards_data[instance->targets[0].card].id) ){
			int new_card = instance->targets[0].card;
			int card_added = add_card_to_hand(instance->targets[0].player, new_card);
			int good = 1;
			if( is_aura(instance->targets[0].player, card_added) ){
				good = call_card_function(instance->targets[0].player, card_added, EVENT_CAN_CAST);
				if( good ){
					set_special_flags3(instance->targets[0].player, card_added, SF3_IN_PLAY_DIRECTLY);
				}
			}
			if( good ){
				remove_card_from_rfg(instance->targets[0].player, cards_data[instance->targets[0].card].id);
				if( instance->targets[1].player & 1 ){
					add_1_1_counter(instance->targets[0].player, card_added);
				}
				put_into_play(instance->targets[0].player, card_added);
				if( instance->display_pic_csv_id == CARD_ID_VANISH_INTO_MEMORY ){
					multidiscard(player, get_toughness(instance->targets[0].player, card_added), 0);
				}
			}
			else{
				obliterate_card(instance->targets[0].player, card_added);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}

	// EVENT_CLEANUP is sent to all cards immediately before any TRIGGER_EOTs are
	if (event == EVENT_CLEANUP){
		instance->targets[1].card = 1;
	}
	return 0;
}

int remove_until_eot(int player, int card, int t_player, int t_card){
	int legacy = -1;
	if( in_play(t_player, t_card) ){
		int token = is_token(t_player, t_card);
		int iid = get_original_internal_card_id(t_player, t_card);
		int owner = get_owner(t_player, t_card);
		kill_card(t_player, t_card, KILL_REMOVE);
		if( ! token && is_what(-1, iid, TYPE_PERMANENT) ){
			legacy = create_legacy_effect(player, card, &remove_until_eot_legacy);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[0].player = owner;
			leg->targets[0].card = iid;
			leg->targets[1].player = 0;
			switch (get_id(player, card)){
				case CARD_ID_LONG_ROAD_HOME:
				case CARD_ID_OTHERWORLDLY_JOURNEY:
					leg->targets[1].player = 1;
					break;
				default:
					break;
			}
			// Prevent remove-until-eot effects added during an eot effect from ending until next turn
			leg->targets[1].card = current_phase != PHASE_CLEANUP;
		}
	}
	return legacy;
}

void obliviation(int player, int card, int targ_player, int targ_card)
{
  if (!is_token(targ_player, targ_card) && in_play(targ_player, targ_card))
	{
	  int iid = get_original_internal_card_id(targ_player, targ_card);
	  card_instance_t* legacy = get_card_instance(player, create_card_name_legacy(player, card, cards_data[iid].id));
	  legacy->targets[7].player = get_owner(targ_player, targ_card);
	  legacy->targets[7].card = iid;
	}

  kill_card(targ_player, targ_card, KILL_REMOVE);
}

void return_from_oblivion(int player, int card, event_t event){
	if (leaves_play(player, card, event)){
		card_instance_t* inst;
		int p, c, iid, owner;
		for (p = 0; p <= 1; ++p){
			for (c = 0; c < active_cards_count[p]; ++c){
				if ((inst = get_card_instance(p, c))
					&& inst->internal_card_id == LEGACY_EFFECT_ASWAN
					&& inst->damage_target_card == card && inst->damage_target_player == player
					&& (iid = inst->targets[7].card) >= 0 && (owner = inst->targets[7].player) >= 0)
				{
					kill_card(p, c, KILL_REMOVE);
					int card_added = add_card_to_hand(owner, iid);
					int good = 1;
					if( is_aura(owner, card_added) ){
						good = call_card_function(owner, card_added, EVENT_CAN_CAST);
						if( good ){
							set_special_flags3(owner, card_added, SF3_IN_PLAY_DIRECTLY);
						}
					}
					if( good ){
						remove_card_from_rfg(owner, cards_data[iid].id);
						put_into_play(owner, card_added);
					}
					else{
						obliterate_card(owner, card_added);
					}
				}
			}
		}
	}
}

int check_for_cip_effects_removal(int player, int card){
	if (is_what(player, card, TYPE_CREATURE) ){
		int i, k;
		for(i=0; i<2; i++){
			for(k=0; k<active_cards_count[i]; k++){
				if( in_play(i, k) && ! is_humiliated(i, k) ){
					if( get_id(i, k) == CARD_ID_TORPOR_ORB || get_id(i, k) == CARD_ID_HUSHWING_GRYFF ){
						return 1;
					}
				}
			}
		}
	}
	return 0;
}

int comes_into_play_mode(int player, int card, event_t event, resolve_trigger_t trigger_mode)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller
	  && player == trigger_cause_controller && card == trigger_cause
	  && !is_humiliated(player, card) && ! check_for_cip_effects_removal(player, card))
	{
	  if (event == EVENT_TRIGGER)
		{
		  if (trigger_mode == RESOLVE_TRIGGER_DUH)
			trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

		  event_result |= trigger_mode;
		}
	  else if (event == EVENT_RESOLVE_TRIGGER)
		return 1;
	}
  return 0;
}

int comes_into_play(int player, int card, event_t event)
{
  return comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY);
}

void comes_into_play_tapped(int player, int card, event_t event)
{
  // inlined in card_generic_som_tapland() and card_tangoland()
  // PS.  That means if you're going to put some random misplaced hack here, you should do it there too.
  if ((event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card)
	  && !check_special_flags2(player, card, SF2_FACE_DOWN_DUE_TO_MANIFEST))
	get_card_instance(player, card)->state |= STATE_TAPPED;	// avoid sending event
}

int leaves_play(int player, int card, event_t event){
	if( trigger_condition == TRIGGER_LEAVE_PLAY ){
		if( affect_me( player, card) && trigger_cause == card && trigger_cause_controller == player
			&& reason_for_trigger_controller == player )
		{
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
				return 1;
			}
		}
	}
	return 0;
}

// As leaves_play(), but allows a card ({triggering_player,triggering_card}) to watch for a different card ({t_player,t_card}) leaving play, not itself.
int other_leaves_play(int triggering_player, int triggering_card, int t_player, int t_card, event_t event)
{
  if (trigger_condition == TRIGGER_LEAVE_PLAY
	  && affect_me(triggering_player, triggering_card)
	  && trigger_cause == t_card && trigger_cause_controller == t_player
	  && reason_for_trigger_controller == triggering_player)
	{
	  if(event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;
	  else if (event == EVENT_RESOLVE_TRIGGER)
		return 1;
	}
  return 0;
}

int is_sick(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if( (instance->state & STATE_SUMMON_SICK) ){
		if( is_what(player, card, TYPE_CREATURE) && ! check_special_flags2(player, card, SF2_THOUSAND_YEAR_ELIXIR) ){
			return 1;
		}
	}
	return 0;
}

int is_animated_and_sick(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if( (instance->state & STATE_SUMMON_SICK) ){
		if( is_what(player, card, TYPE_CREATURE) && ! check_special_flags2(player, card, SF2_THOUSAND_YEAR_ELIXIR) ){
			return 1;
		}
	}
	return 0;
}

extern int hack_allow_sorcery_if_rules_engine_is_only_effect;
int can_sorcery_be_played(int player, event_t event){
	// Must be your own main phase
	if(current_turn != player || (current_phase != PHASE_MAIN1 && current_phase != PHASE_MAIN2)){
		return 0;
	}

	// If the stack is empty, good
	if (stack_size <= 0){
		return 1;
	}

	if (stack_size == 1){
		/* During EVENT_CAST_SPELL and EVENT_ACTIVATE, the card that's being cast or effect that's being activated is already on the stack.  If it's the only
		 * one on the stack, however, a sorcery was legal to play when it was put there. */
		if (event == EVENT_CAST_SPELL || event == EVENT_ACTIVATE){
			return 1;
		}

		/* A rules engine activation is a special case, since it'll indirectly call other card functions that themselves will call this with EVENT_CAN_ACTIVATE
		 * or similar instead of the original EVENT_ACTIVATE.  So we set hack_allow_sorcery_if_rules_engine_is_only_effect at the start of card_rules_engine()'s
		 * EVENT_CAN_ACTIVATE and clear it at the end. */
		if (hack_allow_sorcery_if_rules_engine_is_only_effect){
			card_instance_t* instance = get_card_instance(stack_cards[0].player, stack_cards[0].card);
			if (cards_data[instance->original_internal_card_id].id == CARD_ID_RULES_ENGINE){
				return 1;
			}
		}
	}

	// Nope, there's actually a spell or effect on the stack.
	return 0;
}

int count_subtype_in_hand(int player, subtype_t type){

  int i;
  int count = 0;

  for(i=0; i < active_cards_count[player]; i++){
	  if( in_hand(player, i) && has_subtype(player, i, type) ){
		 count++;
	  }
  }

  return count;
}

int tap_card(int player, int card)
{
  // 0x4a3170
  card_instance_t* instance = get_card_instance(player, card);
  if (!(instance->state & STATE_TAPPED))
	{
	  instance->state |= STATE_TAPPED;
	  // Exe version only cleared tapped_for_mana_color if the tapping card had EA_MANA_SOURCE set.
	  tapped_for_mana_color = -1;
	  dispatch_event(player, card, EVENT_TAP_CARD);
	}
  return 0;
}

void untap_card(int player, int card)
{
  // inlined in untap_phase()
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->state & STATE_TAPPED)
	{
	  instance->state &= ~STATE_TAPPED;
	  if (player_bits[player] & PB_SEND_EVENT_UNTAP_CARD_TO_ALL)
		dispatch_event(player, card, EVENT_UNTAP_CARD);
	  else
		dispatch_event_with_attacker_to_one_card(player, card, EVENT_UNTAP_CARD, 1-player, -1);
	}
}

/* Don't send EVENT_UNTAP_CARD events.  Use only if this card is untapping only as an implementation detail, e.g. it was tapped to prevent it from tapping to
 * generate mana for its own activation cost, and the player cancelled during mana charging. */
void untap_card_no_event(int player, int card)
{
  get_card_instance(player, card)->state &= ~STATE_TAPPED;
}

int get_special_flags(int player, int card, int type_of_special_flag){
	card_instance_t *instance = get_card_instance(player, card);
	if( type_of_special_flag == 1 ){
		return instance->targets[14].card;
	}
	if( type_of_special_flag == 2 ){
		return instance->targets[17].player;
	}
	if( type_of_special_flag == 3 ){
		return instance->targets[15].card;
	}
	return -1;
}

int check_special_flags(int player, int card, int value){
	card_instance_t *instance = get_card_instance( player, card );
	if (instance->targets[14].card == -1){
		return 0;
	} else {
		return instance->targets[14].card & value;
	}
}

void set_special_flags(int player, int card, int value){
	if( card != -1 ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->targets[14].card == -1 ){
			instance->targets[14].card = 0;
		}
		instance->targets[14].card |= value;
	}
	else{
		int i;
		for(i=0; i<2; i++){
			if( i == player || player == 2 ){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) ){
							card_instance_t *instance = get_card_instance( i, count );
							if( instance->targets[14].card == -1 ){
								instance->targets[14].card = 0;
							}
							instance->targets[14].card |= value;
						}
						count--;
				}
			}
		}
	}
}

void remove_special_flags(int player, int card, int value){
	if( card != -1 ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->targets[14].card != -1 && (instance->targets[14].card & value) ){
			instance->targets[14].card &= ~value;
		}
	}
	else{
		int i;
		for(i=0; i<2; i++){
			if( i == player || player == 2 ){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) ){
							card_instance_t *instance = get_card_instance( i, count );
							if( instance->targets[14].card != -1 && (instance->targets[14].card & value) ){
								instance->targets[14].card &= ~value;
							}
						}
						count--;
				}
			}
		}
	}
}

int check_special_flags2(int player, int card, int value){
	card_instance_t *instance = get_card_instance( player, card );
	if (instance->targets[17].player == -1){
		return 0;
	} else {
		return instance->targets[17].player & value;
	}
}

void set_special_flags2(int player, int card, int value){
	if( card != -1 ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->targets[17].player == -1 ){
			instance->targets[17].player = 0;
		}
		instance->targets[17].player |= value;
	}
	else{
		int i;
		for(i=0; i<2; i++){
			if( i == player || player == 2 ){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) ){
							card_instance_t *instance = get_card_instance( i, count );
							if( instance->targets[17].player == -1 ){
								instance->targets[17].player = 0;
							}
							instance->targets[17].player |= value;
						}
						count--;
				}
			}
		}
	}
}

void remove_special_flags2(int player, int card, int value){
	if( card != -1 ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->targets[17].player != -1 && (instance->targets[17].player & value) ){
			instance->targets[17].player &= ~value;
		}
	}
	else{
		int i;
		for(i=0; i<2; i++){
			if( i == player || player == 2 ){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) ){
							card_instance_t *instance = get_card_instance( i, count );
							if( instance->targets[17].player != -1 && (instance->targets[17].player & value) ){
								instance->targets[17].player &= ~value;
							}
						}
						count--;
				}
			}
		}
	}
}

int check_special_flags3(int player, int card, int value){
	card_instance_t *instance = get_card_instance( player, card );
	if (instance->targets[15].card == -1){
		return 0;
	} else {
		return instance->targets[15].card & value;
	}
}

void set_special_flags3(int player, int card, int value){
	if( card != -1 ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->targets[15].card == -1 ){
			instance->targets[15].card = 0;
		}
		instance->targets[15].card |= value;
	}
	else{
		int i;
		for(i=0; i<2; i++){
			if( i == player || player == 2 ){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) ){
							card_instance_t *instance = get_card_instance( i, count );
							if( instance->targets[15].card == -1 ){
								instance->targets[15].card = 0;
							}
							instance->targets[15].card |= value;
						}
						count--;
				}
			}
		}
	}
}

void remove_special_flags3(int player, int card, int value){
	if( card != -1 ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->targets[15].card != -1 && (instance->targets[15].card & value) ){
			instance->targets[15].card &= ~value;
		}
	}
	else{
		int i;
		for(i=0; i<2; i++){
			if( i == player || player == 2 ){
				int count = active_cards_count[i]-1;
				while( count > -1 ){
						if( in_play(i, count) ){
							card_instance_t *instance = get_card_instance( i, count );
							if( instance->targets[15].card != -1 && (instance->targets[15].card & value) ){
								instance->targets[15].card &= ~value;
							}
						}
						count--;
				}
			}
		}
	}
}

static int effect_pump_until_eot(int player, int card, event_t event)
{
  if (event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card))
		event_result += event == EVENT_POWER ? instance->counter_power : instance->counter_toughness;
	}

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int pump_until_eot(int player, int card, int t_player, int t_card, int power, int toughness){
	int result = create_targetted_legacy_effect(player, card, effect_pump_until_eot, t_player, t_card);
	card_instance_t* effect = get_card_instance(player, result);
	effect->counter_power = power;
	effect->counter_toughness = toughness;
	return result;
}

/* If there's another pump_until_eot effect card originating from player/card attached to t_player/t_card, controlled by player, then add the power/toughness
 * specified here to that effect and return it.  (If you've manually fiddled with the effect's other target settings, you're on your own; they're not checked
 * for.)  Otherwise, create a new effect and return that instead.  In either case, set the effect's counter_power and counter_toughness so they can be seen by
 * |n in the effect text - which must specifically have |n's in it or this'll be hopelessly confusing. */
int pump_until_eot_merge_previous(int player, int card, int t_player, int t_card, int power, int toughness)
{
  /* I'd have liked to use pump_ability_until_eot() and allow an ability and/or sp_ability to be passed to this, and still combine if they matched a previous
   * effect card exactly (after being sleighted, etc.), but that won't work properly; abilities have to be added *and removed* in timestamp order. */
  int src_player, src_card;
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->internal_card_id == activation_card)
	{
	  src_player = instance->parent_controller;
	  src_card = instance->parent_card;
	}
  else
	{
	  src_player = player;
	  src_card = card;
	}
  int c;
  for (c = 0; c < active_cards_count[player]; ++c)
	if ((instance = in_play(player, c))
		&& instance->internal_card_id == LEGACY_EFFECT_CUSTOM && instance->info_slot == (int)effect_pump_until_eot
		&& instance->damage_source_player == src_player && instance->damage_source_card == src_card
		&& instance->damage_target_player == t_player && instance->damage_target_card == t_card)
	  {
		instance->counter_power += power;
		instance->counter_toughness += toughness;
		return c;
	  }

  return pump_until_eot(src_player, src_card, t_player, t_card, power, toughness);
}

void default_pump_ability_definition(int player, int card, pump_ability_t *pump, int pow, int tou, int key, int skey){
	pump->source_p = player;
	pump->source_c = card;
	pump->pow_mod = pow;
	pump->tou_mod = tou;
	pump->key_mod = key;
	pump->skey_mod = skey;
	pump->paue_flags = 0;
	pump->can_block_additional_creatures = 0;
	pump->eot_removal_method = 0;
}

int legacy_effect_pump_ability_until_eot(int player, int card, event_t event ){
	// targets[0] = permanent this is attached to (also in card_instance_t::damage_target_player/card)
	// targets[1].player = ability modifier;
	// targets[1].card = special ability modifier;
	// counter_power = power modifier;
	// counter_toughness = toughness modifier;
	// targets[3].player: paue_3player_t flags (see 'manalink.h')
	// targets[3].card = kill method if in special ability modifier is present "SP_KEYWORD_DIE_AT_EOT";
	// targets[4].player: if 1, then add abilities in targets[1].player; else remove them.  Don't use for something that could be removed by
	//                    PB_CANT_HAVE_OR_GAIN_ABILITIES_MASK.  Prefer negate_ability_until_eot() unless you're doing something else too.
	// targets[4].card: &1: spirit link benefitting player 0
	//                  &2: spirit link benefitting player 1
	//                  &(1|2): spirit link benefitting controller (instead of either of above)
	// targets[5].player: if > 0, then creature can block this many additional creatures
	// targets[5].card: if > -1, extra function to return (see Berserk on unlimited.c)

	card_instance_t *instance = get_card_instance(player, card);
	if( affect_me(instance->targets[0].player, instance->targets[0].card ) ){
		if( event == EVENT_POWER ){
			event_result += instance->counter_power;
		}
		else if( event == EVENT_TOUGHNESS ){
				event_result += instance->counter_toughness;
		}
		else if( event == EVENT_ABILITIES ){
				if( instance->targets[4].player == 1 ){
					event_result |= instance->targets[1].player;
				}
				else{
					event_result &= ~instance->targets[1].player;
				}
		}
	}

	if (instance->targets[1].player == -1){
		instance->targets[1].player = 0;
	}
	if (instance->targets[1].card == -1){
		instance->targets[1].card = 0;
	}

	// fake keywords
	if (instance->targets[1].card){
		special_abilities(instance->targets[0].player, instance->targets[0].card, event, instance->targets[1].card, player, card);
	}

	if (instance->targets[4].card > 0){
		if ((instance->targets[4].card & 3) == 3){
			spirit_link_effect(instance->targets[0].player, instance->targets[0].card, event, instance->targets[0].player);
		} else {
			if (instance->targets[4].card & 1){
				spirit_link_effect(instance->targets[0].player, instance->targets[0].card, event, 0);
			}
			if (instance->targets[4].card & 2){
				spirit_link_effect(instance->targets[0].player, instance->targets[0].card, event, 1);
			}
		}
	}

	if (instance->targets[3].player > 0){
		if (instance->targets[3].player & PAUE_CANT_ATTACK){
			cannot_attack(instance->targets[0].player, instance->targets[0].card, event);
		}
		if ((instance->targets[3].player & PAUE_KILL_SOURCE_IF_LEAVES_BATTLEFIELD)
			&& in_play(instance->damage_source_player, instance->damage_source_card)
			&& leaves_play(instance->targets[0].player, instance->targets[0].card, event)){
			kill_card(instance->damage_source_player, instance->damage_source_card,
					  instance->targets[3].card >= KILL_BURY && instance->targets[3].card <= KILL_REMOVE ? instance->targets[3].card : KILL_SACRIFICE);
		}
	}

	if (instance->targets[5].player > 0){
		event_flags |= EA_SELECT_BLOCK;
		attached_creature_can_block_additional(player, card, event, instance->targets[5].player);
	}

	int end_effect = 0;
	if( (instance->targets[3].player & PAUE_END_AT_EOT) ){
		if( !((instance->targets[1].card & SP_KEYWORD_DIE_AT_EOT) || (instance->targets[3].player & PAUE_REMOVE_TARGET_AT_EOT)) ){
			if( event == EVENT_CLEANUP ){
				end_effect = 1;
			}
		}
		else{
			if( eot_trigger(player, card, event) ){
				end_effect = 1;
			}
		}
	}
	if( (instance->targets[3].player & PAUE_END_AT_END_OF_COMBAT ) && end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		end_effect = 1;
	}
	if( ((instance->targets[3].player & PAUE_END_IF_SOURCE_UNTAP ) || (instance->targets[3].player & PAUE_END_IF_SOURCE_LEAVES_PLAY )) &&
		leaves_play(instance->targets[6].player, instance->targets[6].card, event)
	  ){
		end_effect = 1;
	}
	if( (instance->targets[3].player & PAUE_END_IF_SOURCE_UNTAP ) ){
		player_bits[instance->targets[6].player] |= PB_SEND_EVENT_UNTAP_CARD_TO_ALL	;
		if( event == EVENT_UNTAP_CARD && affect_me(instance->targets[6].player, instance->targets[6].card) ){
			end_effect = 1;
		}
	}
	if( (instance->targets[3].player & PAUE_END_AT_THE_BEGINNING_OF_YOUR_NEXT_TURN) && current_turn != player && eot_trigger(player, card, event) ){
		end_effect = 1;
	}

	if( end_effect ){
		if( instance->targets[3].player & PAUE_DE_HUMILIATE_AT_EOT ){
			humiliate(player, card, instance->targets[0].player, instance->targets[0].card, 0);
		}
		if( instance->targets[3].player & PAUE_ENABLE_ALL_ACTIVATED_ABILITIES_AT_EOT ){
			disable_all_activated_abilities(instance->targets[0].player, instance->targets[0].card, 0);
		}
		if( instance->targets[3].player & PAUE_ENABLE_NONMANA_ACTIVATED_ABILITIES_AT_EOT ){
			disable_nonmana_activated_abilities(instance->targets[0].player, instance->targets[0].card, 0);
		}
		if( instance->targets[3].player & PAUE_RESET_SUBTYPES_AT_EOT ){
			reset_subtypes(instance->targets[0].player, instance->targets[0].card, 2);
		}
		if( (instance->targets[1].card & SP_KEYWORD_DIE_AT_EOT) || (instance->targets[3].player & PAUE_REMOVE_TARGET_AT_EOT) ){
			if( instance->targets[3].card != ACT_BOUNCE ){
				int kill_mode = instance->targets[3].card;
				kill_card(instance->targets[0].player, instance->targets[0].card, kill_mode);
			}
			else{
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}

	if( instance->targets[5].card > -1 ){
		return instance->targets[5].card;
	}

	return 0;
}

static void get_sleighted_hacked_suppressed_ability(int player, int card, int t_player, int* p_ability, int* p_sp_ability)
{
  int ability = *p_ability;
  int sp_ability = *p_sp_ability;

  // If any color protections or landwalk abilities set, or KEYWORD_RECALC_SET_COLOR, then account for hack/sleight
  if (ability & (KEYWORD_PROT_COLORED|KEYWORD_BASIC_LANDWALK|KEYWORD_RECALC_SET_COLOR))
	{
	  // KEYWORD_RECALC_SET_COLOR is our flag to *not* touch protections or landwalks due to hack or sleight, typically where a choice is prompted for or the ability is copied
	  if (ability & KEYWORD_RECALC_SET_COLOR)
		ability &= ~KEYWORD_RECALC_SET_COLOR;
	  else
		{
		  if (ability & KEYWORD_PROT_COLORED)
			{
			  uint32_t col, orig = ability & KEYWORD_PROT_COLORED;
			  ability &= ~KEYWORD_PROT_COLORED;
			  for (col = COLOR_BLACK; col <= COLOR_WHITE; ++col)
				if (orig & (KEYWORD_PROT_BLACK << (col - 1)))
				  ability |= KEYWORD_PROT_BLACK << (get_sleighted_color(player, card, col) - 1);
			}

		  if (ability & KEYWORD_BASIC_LANDWALK)
			{
			  uint32_t col, orig = ability & KEYWORD_BASIC_LANDWALK;
			  ability &= ~KEYWORD_BASIC_LANDWALK;
			  for (col = COLOR_BLACK; col <= COLOR_WHITE; ++col)
				if (orig & (KEYWORD_SWAMPWALK << (col - 1)))
				  ability |= KEYWORD_SWAMPWALK << (get_hacked_color(player, card, col) - 1);
			}
		}
	}

  if (player_bits[t_player] & PB_CANT_HAVE_OR_GAIN_ABILITIES_MASK)
	{
	  if (player_bits[t_player] & PB_CANT_HAVE_OR_GAIN_FIRST_STRIKE)
		ability &= ~KEYWORD_FIRST_STRIKE;
	  if (player_bits[t_player] & PB_CANT_HAVE_OR_GAIN_FLYING)
		ability &= ~KEYWORD_FLYING;
	  if (player_bits[t_player] & PB_CANT_HAVE_OR_GAIN_DEATHTOUCH)
		sp_ability &= ~SP_KEYWORD_DEATHTOUCH;
	  if (player_bits[t_player] & PB_CANT_HAVE_OR_GAIN_TRAMPLE)
		ability &= ~KEYWORD_TRAMPLE;
	  if (player_bits[t_player] & PB_CANT_HAVE_OR_GAIN_SHROUD)
		ability &= ~KEYWORD_SHROUD;
	  if (player_bits[t_player] & PB_CANT_HAVE_OR_GAIN_HEXPROOF)
		sp_ability &= ~SP_KEYWORD_HEXPROOF;
	}

  *p_ability = ability;
  *p_sp_ability = sp_ability;
}

static int pump_ability_until_eot_with_precomputed_sleight_hack_suppress(int player, int card, int t_player, int t_card, pump_ability_t *pump){
	if( pump->paue_flags & PAUE_UNTAP ){
		untap_card(t_player, t_card);
	}
	int legacy_card = create_targetted_legacy_effect(player, card, &legacy_effect_pump_ability_until_eot, t_player, t_card);
	card_instance_t *legacy_instance = get_card_instance(player, legacy_card);
	legacy_instance->targets[0].player = t_player;
	legacy_instance->targets[0].card = t_card;
	legacy_instance->number_of_targets = 1;
	legacy_instance->targets[1].player = pump->key_mod;
	legacy_instance->targets[1].card = pump->skey_mod;
	legacy_instance->counter_power = pump->pow_mod;
	legacy_instance->counter_toughness = pump->tou_mod;
	legacy_instance->targets[3].player = pump->paue_flags;
	legacy_instance->targets[3].card = pump->eot_removal_method;
	legacy_instance->targets[4].player = 1;
	legacy_instance->targets[5].player = pump->can_block_additional_creatures;
	legacy_instance->targets[6].player = pump->source_p;
	legacy_instance->targets[6].card = pump->source_c;
	return legacy_card;
}

int pump_ability(int player, int card, int t_player, int t_card, pump_ability_t *pump){
	if (pump->key_mod || pump->skey_mod){
		get_sleighted_hacked_suppressed_ability(player, card, t_player, &pump->key_mod, &pump->skey_mod);
	}
	return pump_ability_until_eot_with_precomputed_sleight_hack_suppress(player, card, t_player, t_card, pump);
}

int pump_ability_until_eot(int player, int card, int t_player, int t_card, int power, int toughness, int ability, int sp_ability)
{
	pump_ability_t pump;
	default_pump_ability_definition(player, card, &pump, power, toughness, ability, sp_ability);
	if( !(sp_ability & SP_KEYWORD_DOES_NOT_END_AT_EOT) ){
		pump.paue_flags = PAUE_END_AT_EOT;
	}
	return pump_ability(player, card, t_player, t_card, &pump);
}

/* Just like pump_ability_until_eot(), but can't pump power or toughness, and if the creature already has an effect card from {player,card} and has all the
 * named keywords set, only gives the existing effect a new timestamp instead of making a new one. */
int pump_ability_until_eot_no_repeat(int player, int card, int t_player, int t_card, int ability, int sp_ability)
{
  get_sleighted_hacked_suppressed_ability(player, card, t_player, &ability, &sp_ability);

  card_instance_t* inst = get_card_instance(t_player, t_card);
  int eff;
  if ((inst->regen_status & ability) == (uint32_t)ability
	  && (get_special_abilities_by_instance(inst) & sp_ability) == sp_ability
	  && (eff = find_repeat_legacy_effect(player, card, legacy_effect_pump_ability_until_eot, t_player, t_card)) != -1)
	return eff;

  pump_ability_t pump;
  default_pump_ability_definition(player, card, &pump, 0, 0, ability, sp_ability);
  if (!(sp_ability & SP_KEYWORD_DOES_NOT_END_AT_EOT))
	pump.paue_flags = PAUE_END_AT_EOT;
  return pump_ability_until_eot_with_precomputed_sleight_hack_suppress(player, card, t_player, t_card, &pump);
}

void pump_ability_by_test(int player, int card, int t_player, pump_ability_t *pump, test_definition_t *this_test){
	if (pump->key_mod || pump->skey_mod){
		get_sleighted_hacked_suppressed_ability(player, card, t_player, &pump->key_mod, &pump->skey_mod);
	}
	int i = 0;
	for(i=0; i<2; i++){
		int p = i == 0 ? current_turn : 1-current_turn;
		if( t_player == ANYBODY || t_player == p ){
			int c = active_cards_count[p]-1;
			while( c > -1 ){
					if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) && (this_test == NULL || new_make_test_in_play(p, c, -1, this_test)) ){
						if( this_test == NULL || this_test->not_me == 0 || (this_test->not_me == 1 && !(p == player && c == card)) ){
							pump_ability_until_eot_with_precomputed_sleight_hack_suppress(player, card, p, c, pump);
						}
					}
					c--;
			}
		}
	}
}

int can_block_additional_until_eot(int player, int card, int t_player, int t_card, int num_additional){
	int legacy_card = create_targetted_legacy_effect(player, card, &legacy_effect_pump_ability_until_eot, t_player, t_card);
	card_instance_t *legacy_instance = get_card_instance(player, legacy_card);
	legacy_instance->targets[0].player = t_player;
	legacy_instance->targets[0].card = t_card;
	legacy_instance->targets[1].player = 0;
	legacy_instance->targets[1].card = 0;
	legacy_instance->counter_power = 0;
	legacy_instance->counter_toughness = 0;
	legacy_instance->targets[4].player = 1;
	legacy_instance->targets[5].player = num_additional;
	return legacy_card;
}

void pump_creatures_until_eot(int player, int card, int t_player, int alternate_legacy_text_number, int power, int toughness, int ability, int sp_ability, test_definition_t *this_test /* optional */){

	int i, count;
	int test_score = this_test ? new_get_test_score(this_test) : 0;
	card_instance_t* instance = get_card_instance(player, card);
	if (instance->internal_card_id == activation_card){
		player = instance->parent_controller;
		card = instance->parent_card;
	}

	for(i=0; i<2; i++){
		if( t_player==2 || i==t_player ){
			int shs_ability = ability, shs_sp_ability = sp_ability;
			if (ability || sp_ability){
				get_sleighted_hacked_suppressed_ability(player, card, i, &shs_ability, &shs_sp_ability);
			}
			pump_ability_t pump;
			default_pump_ability_definition(player, card, &pump, power, toughness, shs_ability, shs_sp_ability);
			pump.paue_flags = PAUE_END_AT_EOT;

			for (count = active_cards_count[i]-1; count >= 0; --count){
				if( (instance = in_play(i, count)) && is_what(i, count, TYPE_CREATURE) &&
					( !this_test ||
					  (( this_test->not_me == 0 || (this_test->not_me == 1 && !(i == player && count == card)) ) &&
					   new_make_test_in_play(i, count, test_score, this_test)
					) ) &&
					!(instance->state & STATE_CANNOT_TARGET)
				  ){
					int legacy = pump_ability_until_eot_with_precomputed_sleight_hack_suppress(player, card, i, count, &pump);
					if (alternate_legacy_text_number > 0){
						alternate_legacy_text(alternate_legacy_text_number, player, legacy);
					}
				}
			}
		}
	}
}

/* Just like pump_creatures_until_eot(), but can't pump power or toughness, and if the creature already has an effect card from {player,card} and has all the
 * named keywords set, only gives the existing effect a new timestamp instead of making a new one. */
void pump_creatures_until_eot_no_repeat(int player, int card, int t_player, int alternate_legacy_text_number, int ability, int sp_ability, test_definition_t *this_test /* optional */){

	int i, count;
	int test_score = this_test ? new_get_test_score(this_test) : 0;

	card_instance_t* instance = get_card_instance(player, card);
	if (instance->internal_card_id == activation_card){
		player = instance->parent_controller;
		card = instance->parent_card;
	}

	for(i=0; i<2; i++){
		if( t_player==2 || i==t_player ){
			int shs_ability = ability, shs_sp_ability = sp_ability;
			if (ability || sp_ability){
				get_sleighted_hacked_suppressed_ability(player, card, i, &shs_ability, &shs_sp_ability);
			}
			pump_ability_t pump;
			default_pump_ability_definition(player, card, &pump, 0, 0, shs_ability, shs_sp_ability);
			pump.paue_flags = PAUE_END_AT_EOT;

			for (count = active_cards_count[i]-1; count >= 0; --count){
				if( (instance = in_play(i, count)) && is_what(i, count, TYPE_CREATURE) &&
					( !this_test ||
					  (( this_test->not_me == 0 || (this_test->not_me == 1 && !(i == player && count == card)) ) &&
					   new_make_test_in_play(i, count, test_score, this_test)
					) ) &&
					!(instance->state & STATE_CANNOT_TARGET)
				  ){
					if ((instance->regen_status & shs_ability) == (uint32_t)shs_ability
						&& (get_special_abilities_by_instance(instance) & shs_sp_ability) == shs_sp_ability
						&& find_repeat_legacy_effect(player, card, legacy_effect_pump_ability_until_eot, i, count) != -1
					   ){
						continue;
					}

					int legacy = pump_ability_until_eot_with_precomputed_sleight_hack_suppress(player, card, i, count, &pump);
					if (alternate_legacy_text_number > 0){
						alternate_legacy_text(alternate_legacy_text_number, player, legacy);
					}
				}
			}
		}
	}
}

// Just like pump_creatures_until_eot_merge_pt(), but allows an alternate legacy text number.
void pump_creatures_until_eot_merge_pt_alternate_legacy_text(int player, int card, int t_player, int power, int toughness, test_definition_t *this_test /* optional */, int alt_text_number)
{
  int p, c;
  int test_score = this_test ? new_get_test_score(this_test) : 0;
  card_instance_t* instance;
  for (p = 0; p < 2; ++p)
	if (t_player == ANYBODY || t_player == p)
	  for (c = active_cards_count[p]-1; c >= 0; --c)
		if ((instance = in_play(p, c))
			&& is_what(p, c, TYPE_CREATURE)
			&& (!this_test
				|| ((this_test->not_me == 0 || (this_test->not_me == 1 && !(p == player && c == card)))
					&& (new_make_test_in_play(p, c, test_score, this_test))))
			&& !(instance->state & STATE_CANNOT_TARGET))
		  {
			int leg = pump_until_eot_merge_previous(player, card, p, c, power, toughness);
			if (alt_text_number)
			  alternate_legacy_text(alt_text_number, player, leg);
		  }
}

/* Just like pump_creatures_until_eot(), but can't pump abilities, and sends its effect through pump_until_eot_merge_previous() so they combine into a single
 * effect card.  Requires that the card's effect text uses |n substitutions. */
void pump_creatures_until_eot_merge_pt(int player, int card, int t_player, int power, int toughness, test_definition_t *this_test /* optional */)
{
  pump_creatures_until_eot_merge_pt_alternate_legacy_text(player, card, t_player, power, toughness, this_test, 0);
}

int pump_subtype_until_eot(int player, int card, int t_player, int subtype, int power, int toughness, int ability, int sp_ability ){
	// subtype = -1 --> pump all creatures.  But you should just call pump_creatures_until_eot() instead then.

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "");
	if( subtype > 0 ){
		this_test.subtype = subtype;
	}
	pump_creatures_until_eot(player, card, t_player, 0, power, toughness, ability, sp_ability, &this_test);
	return 0;
}

void pump_color_until_eot(int player, int card, int t_player, int clr, int power, int toughness, int ability, int sp_ability ){
	// clr = -1 --> pump all creatures

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	if( clr > 0 ){
		this_test.color = clr;
	}
	pump_creatures_until_eot(player, card, t_player, 0, power, toughness, ability, sp_ability, &this_test);
}

static int legacy_switch_power_and_toughness(int player, int card, event_t event)
{
  // targets[0] = permanent this is attached to (also in card_instance_t::damage_target_player/card)
  // targets[7].player = pending switch

  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player, c = instance->damage_target_card;
  card_instance_t* aff = get_card_instance(p, c);

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(p, c))
	{
	  // Mark for revisit
	  instance->targets[7].player = 1;

	  // Make sure both power and toughness have been calculated
	  get_abilities(p, c, EVENT_POWER, -1);
	  get_abilities(p, c, EVENT_TOUGHNESS, -1);

	  return 0;
	}

  if (event == EVENT_CLEANUP && affect_me(player, card))
	{
	  aff->regen_status |= KEYWORD_RECALC_POWER | KEYWORD_RECALC_TOUGHNESS;
	  kill_card(player, card, KILL_REMOVE);
	  return 0;
	}

  // Any other event
  if (instance->targets[7].player == 1)
	{
	  instance->targets[7].player = 0;

	  SWAP(aff->power, aff->toughness);
	}

  return 0;
}

int switch_power_and_toughness_until_eot(int player, int card, int t_player, int t_card)
{
  return create_targetted_legacy_effect(player, card, &legacy_switch_power_and_toughness, t_player, t_card);
}

int count_shrines( int player, int card ){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
	td.required_subtype = SUBTYPE_SHRINE;
	td.allowed_controller = player;
	td.preferred_controller = player;
	return target_available(player, card, &td);
}

int count_subtype(int what_player, int type, int subtype)
{
	int p, c, count = 0;
	for (p = 0; p <= 1; ++p)
		if (what_player == ANYBODY || what_player == p)
			for (c = 0; c < active_cards_count[p]; ++c)
				if (in_play(p, c) && is_what(p, c, type) && ! is_what(p, c, TYPE_EFFECT) && (subtype == -1 || has_subtype(p, c, subtype)))
					++count;
	return count;
}

int check_battlefield_for_subtype(int what_player, int type, int subtype)
{
  int p, c;
  for (p = 0; p <= 1; ++p)
	if (what_player == ANYBODY || what_player == p)
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (in_play(p, c) && is_what(p, c, type) && (subtype == -1 || has_subtype(p, c, subtype)))
		  return 1;
  return 0;
}

int pick_card_from_deck(int player){

	int selected = -1;

	if( player != AI ){
		selected = show_deck( player, deck_ptr[player], 500, "Pick a card", 0, 0x7375B0 );
	}
	else{
		 int i;
		 int *deck = deck_ptr[player];
		 int max_value = -1;
		 int count = count_deck(player);
		 int pos = internal_rand(count);
		 for(i=pos; i<count; i++){
			 card_ptr_t* c = cards_ptr[ cards_data[deck[i]].id ];
			 if( c->ai_base_value > max_value ){
				 max_value = c->ai_base_value;
				 selected = i;
			 }
		 }
	}

	return selected;
}

// Deprecated.  Used only by pattern_of_rebirth_tutor() (called from Pattern of Rebirth at 0x1203847).
int tutor(int player, int type, int (*func_ptr)(int, int),  int tutor_location  ){
	int card_added = -1;
	int i=0;
	while(i++ < 50){
		int selected = pick_card_from_deck(player);
		if( selected && selected != -1){
			int *deck = deck_ptr[player];
			if (is_what(-1, deck[selected], type)){
				card_added = add_card_to_hand(player, deck[selected] );
				remove_card_from_deck( player, selected );
				if( func_ptr(player, card_added) ){
					if( tutor_location == TUTOR_PLAY ){
						put_into_play(player, card_added);
					}
					else if( tutor_location == TUTOR_GRAVE ){
						kill_card(player, card_added, KILL_DESTROY);
					}
					break;
				}
				else{
					put_on_top_of_deck(player, card_added);
				}
			}
		}
		else{
			break;
		}
	}
	shuffle(player);
	if( tutor_location == TUTOR_DECK ){
		put_on_top_of_deck(player, card_added);
	}
	return card_added;
}

int legacy_effect_activated(int player, int card, event_t event){
	card_instance_t* instance = get_card_instance(player, card);
	if (instance->info_slot > 4096){
		return ((int (*)(int, int, event_t))instance->info_slot)(player, card, event);
	} else {
		// Original Nafs Asp legagy

		if (event == EVENT_CAN_ACTIVATE){
			return has_mana(player, COLOR_ANY, 1);
		} else if (event == EVENT_ACTIVATE){
			if (affect_me(player, card) && has_mana(player, COLOR_ANY, 1)){
				charge_mana(player, COLOR_COLORLESS, 1);
			}
		}
		else if (event == EVENT_RESOLVE_ACTIVATION){
			kill_card(player, instance->parent_card, KILL_REMOVE);
		}
		else if ((trigger_condition == TRIGGER_DRAW_PHASE || event == EVENT_SHOULD_AI_PLAY)
				 && affect_me(player, card)
				 && current_turn == player
				 && reason_for_trigger_controller == player
				 && get_card_instance(player, card)->info_slot == 0){
			if (event == EVENT_TRIGGER){
				event_result |= 2;
			} else if (event == EVENT_RESOLVE_TRIGGER || event == EVENT_SHOULD_AI_PLAY){
				load_text(0, "NAFS_ASP");
				do_dialog(player, player, card, -1, -1, text_lines[0], 0);
				lose_life(player, 1);
				kill_card(player, card, KILL_BURY);
			}
		} else if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY){
			if (ai_is_speculating == 1){
				--life[player];
			}
			instance->info_slot = 0;
		}
		return 0;
	}
}

int create_legacy_effect(int player, int card, int (*func_ptr)(int, int, event_t) ){
	int result = create_legacy_effect_exe(player, card, LEGACY_EFFECT_CUSTOM, -1, -1);
	card_instance_t *instance = get_card_instance(player, result);
	instance->info_slot = (int) func_ptr;
	return result;
}

int create_legacy_activate(int player, int card, int (*func_ptr)(int, int, event_t) ){
	int result = create_legacy_effect_exe(player, card, LEGACY_EFFECT_ACTIVATED, -1, -1);
	card_instance_t *instance = get_card_instance(player, result);
	instance->info_slot = (int)func_ptr;
	return result;
}

int create_targetted_legacy_activate(int player, int card, int (*func_ptr)(int, int, event_t), int t_player, int t_card){
	int result = create_legacy_effect_exe(player, card, LEGACY_EFFECT_ACTIVATED, t_player, t_card);
	card_instance_t *instance = get_card_instance(player, result);
	instance->info_slot = (int)func_ptr;
	return result;
}

int create_targetted_legacy_effect(int player, int card, int (*func_ptr)(int, int, event_t), int target_player_id, int target_card ){
	int result = create_legacy_effect_exe(player, card, LEGACY_EFFECT_CUSTOM, target_player_id, target_card);
	card_instance_t *instance = get_card_instance(player, result);
	instance->info_slot =  (int) func_ptr;
	instance->targets[0].player = target_player_id;
	instance->targets[0].card = target_card;
	instance->number_of_targets = 1;
	return result;
}

/* If there's already an effect card attached to {target_player_id,target_card} with func_ptr created by {player,card}, give it a new timestamp and return it;
 * else return -1. */
int find_repeat_legacy_effect(int player, int card, int (*func_ptr)(int, int, event_t), int target_player_id, int target_card){
	card_instance_t* inst;
	int eff;
	for (eff = 0; eff < active_cards_count[player]; ++eff){
		if( (inst = in_play(player, eff))
			&& inst->internal_card_id == LEGACY_EFFECT_CUSTOM && inst->info_slot == (int)func_ptr
			&& inst->damage_source_player == player && inst->damage_source_card == card
			&& inst->damage_target_player == target_player_id && inst->damage_target_card == target_card
		  ){
			new_timestamp(player, eff);
			return eff;
		}
	}
	return -1;
}

/* Just like create_targetted_legacy_effect(), but if there's an effect from the same source with the same func_ptr already affecting the card, just gives that
 * a new timestamp instead. */
int create_targetted_legacy_effect_no_repeat(int player, int card, int (*func_ptr)(int, int, event_t), int target_player_id, int target_card){
	int eff = find_repeat_legacy_effect(player, card, func_ptr, target_player_id, target_card);
	if (eff != -1){
		return eff;
	} else {
		return create_targetted_legacy_effect(player, card, func_ptr, target_player_id, target_card);
	}
}

// Creates a legacy effect from (player,card) under control of 1-player.
int create_legacy_effect_for_opponent(int player, int card, int (*func_ptr)(int, int, event_t), int t_player, int t_card)
{
  int card_added = add_card_to_hand(1-player, get_card_instance(player, card)->internal_card_id);
  ASSERT(card_added != -1);
  card_instance_t* temp = get_card_instance(1-player, card_added);

  // Make sure it's neither in hand nor on battlefield when it gets obliterated
  temp->state = (temp->state | STATE_INVISIBLE) & ~STATE_IN_PLAY;
  --hand_count[1-player];

  /* So all the values on the created legacy are correct - create_legacy_effect copies card_color, mana_color, color, STATUS_SLEIGHTED, STATUS_HACKED,
   * color_id[0..5], and hack_mode[0..5] from (player,card); and sets legacy->initial_color to (player,card)->color. */
  memcpy(temp, get_card_instance(player, card), sizeof(card_instance_t));

  // Still neither in hand nor on battlefield
  temp->state = (temp->state | STATE_INVISIBLE) & ~STATE_IN_PLAY;

  int legacy = create_legacy_effect(1-player, card_added, func_ptr);
  obliterate_card(1-player, card_added);

  // Fixup created legacy to point to original card as source
  card_instance_t* leg = get_card_instance(1-player, legacy);
  leg->damage_source_player = player;
  leg->damage_source_card = card;

  // And same art version
  leg->display_pic_num = get_card_image_number(leg->display_pic_csv_id, player, card);

  // Redundant attachment in targets[0], per create_targetted_legacy_effect()
  if (t_player >= 0)
	{
	  leg->targets[0].player = t_player;
	  leg->targets[0].card = t_card;
	  leg->number_of_targets = 1;
	}

  return legacy;
}

/* If this effect is controlled by a different player than the card it's attached to, then transfer control and ownership of the effect.  Calling effect should
 * return 0 if this returns true. */
int effect_follows_control_of_attachment(int player, int card, int event)
{
  if (event == EVENT_STATIC_EFFECTS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->damage_target_player, c = instance->damage_target_card;

	  if (p != player)
		{
		  // Transfer control and ownership of effect
		  int leg = create_targetted_legacy_effect(p, 0, &empty, p, c);
		  card_instance_t* legacy = get_card_instance(p, leg);
		  int old_timestamp = legacy->timestamp;
		  memcpy(legacy, instance, sizeof(card_instance_t));
		  legacy->timestamp = old_timestamp;
		  legacy->state ^= STATE_OWNED_BY_OPPONENT;
		  kill_card(player, card, KILL_REMOVE);
		  return 1;
		}
	}
  return 0;
}


int create_my_legacy(int player, int card, int (*func_ptr)(int, int, event_t)){
	int result = create_legacy_effect_exe(player, card, LEGACY_EFFECT_CUSTOM, -1, -1);
	card_instance_t *instance = get_card_instance(player, result);
	instance->info_slot =  (int) func_ptr;
	instance->targets[0].player = player;
	instance->targets[0].card = card;
	instance->number_of_targets = 1;
	// add_status(player, result, STATUS_INVISIBLE_FX);
	return result;
}

void add_legacy_effect_to_all(int player, int card, int (*func_ptr)(int, int, event_t), int t_player, test_definition_t *this_test){
	int i;
	for(i=0; i<2; i++){
		if( i == t_player || t_player == 2 ){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
					if( in_play(i, count) && new_make_test_in_play(i, count, -1, this_test) ){
						int result = create_legacy_effect_exe(player, card, LEGACY_EFFECT_CUSTOM, i, count);
						card_instance_t *instance = get_card_instance(player, result);
						instance->info_slot =  (int) func_ptr;
						instance->targets[0].player = i;
						instance->targets[0].card = count;
					}
					count--;
			}
		}
	}
}

int resolve_graveyard_trigger(int player, int card, event_t event){
	card_instance_t *instance= get_card_instance(player, card);
	int good = 0;
	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller){
		if( affect_me(player, card ) ){
			if( instance->targets[11].player > 0 ){
				good = 1;
			}
		}
	}

	if( good == 1 ){
		if(event == EVENT_TRIGGER){
			//Make all trigges mandatoy for now
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			return 1;
		}
	}
	return 0;
}

void does_not_untap(int player, int card, event_t event){
	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && affect_me(player, card) ){
		card_instance_t *instance= get_card_instance(player, card);
		instance->untap_status &= ~3;
	}
}

static int effect_does_not_untap(int player, int card, event_t event ){
	/* targets[0]: Permanent this is attached to (also in card_instance_t::damage_target_player/card)
	 * targets[1]: Source of effect.
	 * targets[2].player: Number of untap steps to skip.
	 * targets[2].card: mode, per ednt_flags_t. */

	card_instance_t *instance = get_card_instance(player, card);

	int flags = instance->targets[2].card;

	int t_player = instance->damage_target_player;
	int t_card = instance->damage_target_card;

	if( flags && (flags & EDNT_TAP_AT_END_OF_COMBAT) ){
		if( end_of_combat_trigger(player, card, event, 2) ){
			tap_card(t_player, t_card);
			instance->targets[2].card &= ~EDNT_TAP_AT_END_OF_COMBAT;
		}
	}

	if( ! flags || (flags && (flags & EDNT_CHECK_ORIGINAL_CONTROLLER) && current_turn == instance->targets[3].player) ||
		(flags && (flags & (EDNT_REMAIN_TAPPED_UNTIL_SOURCE_LEAVES_PLAY | EDNT_WONT_UNTAP_ANYMORE)))
	  ){
		does_not_untap(t_player, t_card, event);
	}

	if (event == EVENT_END_OF_UNTAP_STEP && affected_card_controller == t_player ){
		if( ! flags || (flags && !(flags & (EDNT_REMAIN_TAPPED_UNTIL_SOURCE_LEAVES_PLAY | EDNT_WONT_UNTAP_ANYMORE))) ){
			if( instance->targets[2].player > 1 ){
				--instance->targets[2].player;
			}
			else{
				remove_special_flags2(t_player, t_card, SF2_COULD_NOT_UNTAP);
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if ( flags && (flags & EDNT_REMAIN_TAPPED_UNTIL_SOURCE_LEAVES_PLAY) && instance->targets[2].player > -1 ){
		if( leaves_play(instance->targets[1].player, instance->targets[1].card, event) ){
			remove_special_flags2(t_player, t_card, SF2_COULD_NOT_UNTAP);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int does_not_untap_effect(int player, int card, int t_player, int t_card, int mode, int turns){
	if( mode & EDNT_TAP_TARGET ){
		tap_card(t_player, t_card);
	}
	int legacy = create_targetted_legacy_effect(player, card, &effect_does_not_untap, t_player, t_card);
	set_special_flags2(t_player, t_card, SF2_COULD_NOT_UNTAP);
	card_instance_t *instance = get_card_instance( player, legacy );

	instance->targets[1].player = player;
	instance->targets[1].card = card;
	instance->number_of_targets = 2;

	instance->targets[2].card = mode;
	instance->targets[2].player = turns;

	/*
	5/1/2007: "Your next untap..." refers to the player who controls [this permanent] when the triggered ability resolves.
	9/22/2011: If another player controls [this permanent] during your next untap step, the part of the effect that keeps it
				from untapping does nothing. It won't try to apply during a future untap step.
	*/
	instance->targets[3].player = t_player;

	return legacy;
}

static int dnuimt_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 && instance->targets[1].player > -1){
		if( in_play(instance->targets[1].player, instance->targets[1].card) && is_tapped(instance->targets[1].player, instance->targets[1].card) ){
			does_not_untap(instance->damage_target_player, instance->damage_target_card, event);
		}
		else{
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

int does_not_untap_until_im_tapped(int player, int card, int t_player, int t_card){
	int legacy = create_targetted_legacy_effect(player, card, &dnuimt_legacy, t_player, t_card);
	card_instance_t *leg = get_card_instance(player, legacy);
	leg->targets[1].player = player;
	leg->targets[1].card = card;
	leg->number_of_targets = 2;
	return legacy;
}

static int ability_doesnt_untap_while_has_a_counter_and_remove_a_counter_at_upkeep(int player, int card, event_t event)
{
  /* This [permanent] doesn't untap during your untap step if it has a [type] counter on it.
   * At the beginning of your upkeep, remove a [type] counter from this [permanent]. */

  /* These abilities don't go away if the [permanent] has no [type] counters on it (and something unrelated might put more on it, making it stop tapping again);
   * but they do go away permanently if the [permanent] loses all abilities, even if it only loses abilities until end of turn. */

  /* Local data:
   * targets[1].card: counter type */

  card_instance_t* instance = get_card_instance(player, card);
  int counter_type = instance->targets[1].card;

  if (event == EVENT_UNTAP && count_counters(instance->damage_target_player, instance->damage_target_card, counter_type))
	does_not_untap(instance->damage_target_player, instance->damage_target_card, event);

  if (trigger_condition == TRIGGER_UPKEEP && affect_me(player, card)
	  && count_counters(instance->damage_target_player, instance->damage_target_card, counter_type))	// avoid annoying popup if no counters
	upkeep_trigger_ability(player, card, event, instance->damage_target_player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	remove_counter(instance->damage_target_player, instance->damage_target_card, counter_type);

#if 0
  /* As written, this fires if the card is currently humiliated when the abilities are gained.  That's correct for continuous humiliation effects like Humility,
   * but not one-shot ones like Humble. */
  if (event == EVENT_STATIC_EFFECTS && is_humiliated(instance->damage_target_player, instance->damage_target_card))
	kill_card(player, card, KILL_REMOVE);
#endif

  return 0;
}
void gains_doesnt_untap_while_has_a_counter_and_remove_a_counter_at_upkeep(int src_player, int src_card, int t_player, int t_card, counter_t counter_type)
{
  int legacy = create_targetted_legacy_effect(src_player, src_card, ability_doesnt_untap_while_has_a_counter_and_remove_a_counter_at_upkeep, t_player, t_card);
  get_card_instance(src_player, legacy)->targets[1].card = counter_type;
}

int count_cards_by_id(int player, int id){
	int total = 0;
	int i;
	for(i=0; i<2; i++){
		if( player == 2 || i == player ){
			int count = 0;
			while(count < active_cards_count[i]){
					if( in_play(i, count) && ! is_humiliated(i, count) && get_id(i, count) == id ){
						total++;
					}
					count++;
			}
		}
	}
	return total;
}

int select_card_from_graveyard(int player, int card, int whose_grave, int type ){
	int card_added = -1;
	while(1){
		int selected = show_deck( player, get_grave(whose_grave), 500, "Pick a card", 0, 0x7375B0 );
		if( selected != -1){
			const int *grave = get_grave(whose_grave);
			if (is_what(-1, grave[selected], type)){
				card_added =  add_card_to_hand(player, grave[selected] );

				card_instance_t *instance = get_card_instance(player, card);
				instance->targets[0].player = player;
				instance->targets[1].player = whose_grave;
				instance->targets[0].card = card_added;
				instance->number_of_targets = 1;
				instance->info_slot = selected;

				card_instance_t *target = get_card_instance(player, card_added);
				target->state |= STATE_INVISIBLE;
				--hand_count[player];

				return card_added;
			}
		}
		else{
			break;
		}
	}
	return card_added;
}

int has_mana_multi(int player, int colorless, int black, int blue, int green, int red, int white){
	MANA_COLORLESS =	MAX(0, colorless);
	MANA_BLACK =		MAX(0, black);
	MANA_BLUE =			MAX(0, blue);
	MANA_GREEN =		MAX(0, green);
	MANA_RED =			MAX(0, red);
	MANA_WHITE =		MAX(0, white);
	MANA_ARTIFACT =		0;
	MANA_UNKNOWN =		0;
	return check_mana_multi(player);
}

int has_mana_multi_a(int player, int colorless, int black, int blue, int green, int red, int white)
{
  colorless =	MAX(0, colorless);
  black =		MAX(0, black);
  blue =		MAX(0, blue);
  green =		MAX(0, green);
  red =			MAX(0, red);
  white =		MAX(0, white);

  // check_mana_multi() doesn't apply colorless mana to artifact costs, unfortunately.
  if (black == 0 && blue == 0 && green == 0 && red == 0 && white == 0)
	return has_mana(player, COLOR_ARTIFACT, colorless) ? 1 : 0;
  else
	{
	  int artifact = 0;
	  for (; colorless >= 0; --colorless, ++artifact)
		{
		  MANA_COLORLESS =	colorless;
		  MANA_BLACK =		black;
		  MANA_BLUE =			blue;
		  MANA_GREEN =		green;
		  MANA_RED =			red;
		  MANA_WHITE =		white;
		  MANA_ARTIFACT =		artifact;
		  MANA_UNKNOWN =		0;
		  if (check_mana_multi(player))
			return 1;
		}

	  return 0;
	}
}

int charge_mana_multi(int player, int colorless, int black, int blue, int green, int red, int white){
	PAY_MANA_COLORLESS =	colorless;
	PAY_MANA_BLACK =		black;
	PAY_MANA_BLUE =			blue;
	PAY_MANA_GREEN =		green;
	PAY_MANA_RED =			red;
	PAY_MANA_WHITE =		white;
	charge_mana(player, 0, 0);
	if( spell_fizzled == 1 ){
		return 0;
	}
	return 1;
}

int charge_mana_multi_a(int player, int colorless, int black, int blue, int green, int red, int white, int artifact){
	PAY_MANA_COLORLESS =	colorless;
	PAY_MANA_BLACK =		black;
	PAY_MANA_BLUE =			blue;
	PAY_MANA_GREEN =		green;
	PAY_MANA_RED =			red;
	PAY_MANA_WHITE =		white;
	PAY_MANA_ARTIFACT =		artifact;
	charge_mana(player, 0, 0);
	if( spell_fizzled == 1 ){
		return 0;
	}
	return 1;
}

/* These variants should be called instead of charge_mana()/charge_mana_multi() during EVENT_RESOLVE_TRIGGER, EVENT_RESOLVE_SPELL, and RESOLVE_ACTIVATION, so
 * the prompt is correct. */
int charge_mana_while_resolving(int player, int card, event_t event, int charged_player, color_t color, int amount)
{
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	event = EVENT_RESOLVE_TRIGGER;
  put_card_or_activation_onto_stack(player, card, event, 0, 0);
  ldoubleclicked = spell_fizzled = 0;
  charge_mana(charged_player, color, amount);
  obliterate_top_card_of_stack();
  return spell_fizzled == 1 ? 0 : 1;
}

int charge_mana_while_resolving_csvid(int csvid, event_t event, int charged_player, color_t color, int amount)
{
  int card_added = add_card_to_hand(charged_player, get_internal_card_id_from_csv_id(csvid));
  card_instance_t* instance = get_card_instance(charged_player, card_added);
  instance->state |= STATE_INVISIBLE;
  --hand_count[charged_player];

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	event = EVENT_RESOLVE_TRIGGER;
  put_card_or_activation_onto_stack(charged_player, card_added, event, 0, 0);
  ldoubleclicked = spell_fizzled = 0;
  charge_mana(charged_player, color, amount);
  obliterate_top_card_of_stack();

  obliterate_card(charged_player, card_added);
  return spell_fizzled == 1 ? 0 : 1;
}

int charge_mana_multi_while_resolving(int player, int card, event_t event, int charged_player, int colorless, int black, int blue, int green, int red, int white)
{
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	event = EVENT_RESOLVE_TRIGGER;
  put_card_or_activation_onto_stack(player, card, event, 0, 0);
  ldoubleclicked = spell_fizzled = 0;
  int result = charge_mana_multi(charged_player, colorless, black, blue, green, red, white);
  obliterate_top_card_of_stack();
  return result;
}

int charge_mana_multi_while_resolving_csvid(int csvid, event_t event, int charged_player, int colorless, int black, int blue, int green, int red, int white)
{
  int card_added = add_card_to_hand(charged_player, get_internal_card_id_from_csv_id(csvid));
  card_instance_t* instance = get_card_instance(charged_player, card_added);
  instance->state |= STATE_INVISIBLE;
  --hand_count[charged_player];

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	event = EVENT_RESOLVE_TRIGGER;
  put_card_or_activation_onto_stack(charged_player, card_added, event, 0, 0);
  ldoubleclicked = spell_fizzled = 0;
  int result = charge_mana_multi(charged_player, colorless, black, blue, green, red, white);
  obliterate_top_card_of_stack();

  obliterate_card(charged_player, card_added);
  return result;
}

int charge_mana_hybrid_while_resolving(int player, int card, event_t event, int amt_colored, color_t first_color, color_t second_color, int amt_colorless)
{
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	event = EVENT_RESOLVE_TRIGGER;
  put_card_or_activation_onto_stack(player, card, event, 0, 0);
  ldoubleclicked = spell_fizzled = 0;
  int result = charge_mana_hybrid(player, card, amt_colored, first_color, second_color, amt_colorless);
  obliterate_top_card_of_stack();
  return result;
}

// An unusual case - the card is worded to be an activated, not a triggered ability, but we're charging for it during resolution.
int charge_mana_for_activated_ability_while_resolving(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white)
{
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	event = EVENT_RESOLVE_TRIGGER;
  put_card_or_activation_onto_stack(player, card, event, 0, 0);
  ldoubleclicked = spell_fizzled = 0;
  int result = charge_mana_for_activated_ability(player, card, colorless, black, blue, green, red, white);
  obliterate_top_card_of_stack();
  return result;
}

int is_basic_land(int player, int card){
	int id = -1;
	if( player != -1 ){
		id = get_id(player, card);
	}
	else{
		 id = cards_data[card].id;
	}
	return is_basic_land_by_id( id );
}

int is_basic_land_by_id(int id){
	if( id == CARD_ID_FOREST || id == CARD_ID_ISLAND || id == CARD_ID_SWAMP || id == CARD_ID_PLAINS ||
		id == CARD_ID_MOUNTAIN
	  ){
		return 1;
	}
	return 0;
}

int eot_trigger(int player, int card, event_t event){
	if(trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player &&
		(is_what(player, card, TYPE_EFFECT) || ! is_humiliated(player, card)) ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			return 1;
		}
	}
	return 0;
}

int eot_trigger_mode(int player, int card, event_t event, int t_player, resolve_trigger_t trig_mode){
	if( (current_turn == t_player || t_player == ANYBODY) && trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player &&
		(is_what(player, card, TYPE_EFFECT) || ! is_humiliated(player, card))
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= (trig_mode == RESOLVE_TRIGGER_DUH ? (duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL) : trig_mode);
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				return 1;
		}
	}
	return 0;
}

void all_must_attack_if_able(int player, event_t event, int subtype)
{
	event_flags |= EA_FORCE_ATTACK;	// just in case it's not set in ct_all.csv - it won't be for effect cards

	if( event == EVENT_MUST_ATTACK && current_turn == player && !forbid_attack ){
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) && (subtype < 0 || has_subtype(player, c, subtype)) ){
				attack_if_able(player, c, event);
			}
		}
	}
}

int cycling(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		colorless -= (2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR));
		colorless += (2 * count_cards_by_id(player, CARD_ID_SUPPRESSION_FIELD));

		if( colorless < 0 ){
			colorless = 0;
		}
		if( has_mana_multi(player, colorless, black, blue, green, red, white) ){
			return 1;
		}
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			colorless -= (2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR));
			colorless += (2 * count_cards_by_id(player, CARD_ID_SUPPRESSION_FIELD));

			 if( colorless < 0 ){
				 colorless = 0;
			 }

		charge_mana_multi(player, colorless, black, blue, green, red, white);
		if( spell_fizzled != 1 ){
			discard_card(player, card);
			return 2;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			draw_a_card(player);
	}
	return 0;
}

int cycling_hybrid(int player, int card, event_t event, int amt_colored, color_t first_color, color_t second_color, int amt_colorless)
{
  if (event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND)
	{
	  amt_colorless += 2 * count_cards_by_id(player, CARD_ID_SUPPRESSION_FIELD);
	  if (amt_colorless > 0)
		{
		  amt_colorless -= 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		  amt_colorless = MAX(0, amt_colorless);
		}
	}

  if (event == EVENT_CAN_ACTIVATE_FROM_HAND)
	return has_mana_hybrid(player, amt_colored, first_color, second_color, amt_colorless) ? 1 : 0;

  if (event == EVENT_ACTIVATE_FROM_HAND && charge_mana_hybrid(player, card, amt_colored, first_color, second_color, amt_colorless))
	{
	  discard_card(player, card);
	  return 2;
	}

  if (event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1)
	draw_a_card(player);

  return 0;
}

// Keeps the cycling abilities with odd activation costs in close proximity to normal cycling implementation.
int cycling_special_cost(int player, int card, event_t event, int mode)
{
  // Mode == 1: sacrifice a land
  // Mode == 2: pay 2 life

  int amt_colorless = 0;
  if (event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND)
	{
	  amt_colorless += 2 * count_cards_by_id(player, CARD_ID_SUPPRESSION_FIELD);
	  if (amt_colorless > 0)
		{
		  amt_colorless -= 2 * count_cards_by_id(player, CARD_ID_FLUCTUATOR);
		  amt_colorless = MAX(0, amt_colorless);
		}
	}

  if (event == EVENT_CAN_ACTIVATE_FROM_HAND)
	{
	  if (amt_colorless > 0 && !has_mana(player, COLOR_ANY, amt_colorless))
		return 0;

	  switch (mode)
		{
		  case 1:	return can_sacrifice_type_as_cost(player, 1, TYPE_LAND) ? 1 : 0;
		  case 2:	return can_pay_life(player, 2) ? 1 : 0;
		}

	  return 1;
	}

  if (event == EVENT_ACTIVATE_FROM_HAND)
	{
	  if (amt_colorless > 0)
		{
		  charge_mana(player, COLOR_COLORLESS, amt_colorless);
		  if (cancel == 1)
			return 0;
		}

	  switch (mode)
		{
		  case 1:
			if (!controller_sacrifices_a_permanent(player, card, TYPE_LAND, 0))
			  {
				cancel = 1;
				return 0;
			  }
			break;

		  case 2:
			lose_life(player, 2);
			break;
		}

	  discard_card(player, card);
	  return 2;
	}

  if (event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1)
	draw_a_card(player);

  return 0;
}

int choose_a_land(int player, int ai_choice){
	int result = -1;
	while(result == -1){
		result = choose_a_color_exe(player, "Choose a basic land type", 0, ai_choice, COLOR_TEST_ANY_COLORED);
	}
	return result;
}

int choose_a_color(int player, int ai_choice){
	int result = -1;
	while(result == -1){
		result = choose_a_color_exe(player, "Choose a color", 1, ai_choice, COLOR_TEST_ANY_COLORED);
	}
	return result;
}

int choose_a_color_from(int player, int ai_choice, color_test_t available_colors){
	int result = -1;
	while(result == -1){
		result = choose_a_color_exe(player, "Choose a color", 1, ai_choice, available_colors);
	}
	return result;
}

int count_permanents_by_type(int player, type_t type)
{
  int p, c, count = 0;
  for (p = 0; p <= 1; ++p)
	if (player == ANYBODY || player == p)
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (in_play(p, c) && is_what(p, c, type) && !is_what(p, c, TYPE_EFFECT))
		  ++count;
  return count;
}

void menace(int player, int card, event_t event)
{
	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->targets[16].card < 0 ){
			instance->targets[16].card = 0;
		}
		instance->targets[16].card |= SP_KEYWORD_MENACE;
	}

	if( event == EVENT_DECLARE_BLOCKERS && ! is_humiliated(player, card) ){
		minimum_blockers(player, card, event, 2);
	}
}

int fx_menace(int player, int card, event_t event)
{
	if( get_card_instance(player, card)->damage_target_player > -1 ){
		card_instance_t *instance = get_card_instance( player, card );

		special_abilities(instance->damage_target_player, instance->damage_target_card, event, SP_KEYWORD_MENACE, player, card);

		if (event == EVENT_CLEANUP && is_what(player, card, TYPE_EFFECT) ){
			card_instance_t* inst = get_card_instance(player, card);
			remove_special_ability(inst->damage_target_player, inst->damage_target_card, SP_KEYWORD_MENACE);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

void minimum_blockers(int player, int card, int event, int min){
	if( event == EVENT_DECLARE_BLOCKERS ){
		// If there are fewer than 3 creatures blocking me, then no one is
		// blocking me
		int block_count = 0;
		int count = 0;
		while(count < active_cards_count[1-player]){
			if(in_play(1-player, count) ){
				card_instance_t *instance = get_card_instance( 1-player, count);
				if( instance->blocking == card ){
					block_count++;
				}
			}
			count++;
		}

		if( block_count < min ){
			count = 0;
			while(count < active_cards_count[1-player]){
				if(in_play(1-player, count) ){
					card_instance_t *instance = get_card_instance( 1-player, count);
					if( instance->blocking == card ){
						instance->blocking = 255;
						instance->state &= ~(STATE_BLOCKED|STATE_BLOCKING);
					}
				}
				count++;
			}
		}
	}
}

int upkeep_trigger_mode_impl(int player, int card, event_t event, resolve_trigger_t trigger_mode, int no_check)
{
	int upk = 0;
	if (trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && (is_what(player, card, TYPE_EFFECT) || ! is_humiliated(player, card)) &&
		(no_check || (upk = count_upkeeps(current_turn) > 0) )
	){
		if (event == EVENT_TRIGGER){
			if (trigger_mode == RESOLVE_TRIGGER_DUH){
				trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
			}
			event_result |= trigger_mode;
		}

		if (event == EVENT_RESOLVE_TRIGGER){
			if( no_check ){
				return 1;
			}
			return upk;
		}
	}

	return 0;
}

int upkeep_trigger_mode(int player, int card, event_t event, resolve_trigger_t trigger_mode)
{
	return upkeep_trigger_mode_impl(player, card, event, trigger_mode, 0);
}

int upkeep_trigger_no_check(int player, int card, event_t event)
{
	//This won't check for cards that skips or adds Upkeep Phases, so it should be done by the card calling this functions.
	//Useful for cards that create an effect card that will trigger on next Upkeep.
	//Example of usage: "cantrip" in "ice_age.c"
	return upkeep_trigger_mode_impl(player, card, event, RESOLVE_TRIGGER_MANDATORY, 1);
}

int upkeep_trigger(int player, int card, event_t event)
{
  return upkeep_trigger_mode_impl(player, card, event, RESOLVE_TRIGGER_MANDATORY, 0);
}

int global_enchantment(int player, int card, event_t event){
	if( event == EVENT_SHOULD_AI_PLAY ){
		ai_modifier += (player == HUMAN) ? -8 : 8;
	}
	else if( event == EVENT_CAN_CAST ){
		return 1;
	}
	return 0;
}

int enchant_world(int player, int card, event_t event)
{
  if (event == EVENT_SHOULD_AI_PLAY)
	ai_modifier += (player == HUMAN) ? -8 : 8;

  if (event == EVENT_CAN_CAST)
	return 1;

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == player && trigger_cause == card)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  int p, c;
		  card_instance_t* inst;
		  for (p = 0; p <= 1; ++p)
			for (c = 0; c < active_cards_count[p]; ++c)
			  if ((inst = in_play(p, c)) && !(c == card && p == player) && cards_data[inst->internal_card_id].subtype == SUB_ENCHANT_WORLD)
				kill_card(p, c, KILL_STATE_BASED_ACTION);
		}
	}

  return 0;
}

int get_global_color_hack(int player){

	int result = 0;
	int i;
	int z;
	unsigned int timestamp = 0;

	for(z=0; z<2; z++){
		for(i=0; i < active_cards_count[z]; i++){
			if( in_play(z,i) ){
				if( get_id(z,i) == CARD_ID_PAINTERS_SERVANT ){
					card_instance_t *instance = get_card_instance(z,i);
					if( instance->targets[1].card > -1 && instance->timestamp > timestamp ){
						timestamp = instance->timestamp;
						result |= instance->targets[1].card;
					}
				}
				/* This works properly only with Painter's Servant.  Celestial Dawn doesn't change the color of lands, Mycosynth Lattice sets
				 * COLOR_TEST_COLORLESS instead of 0 (the former never occurs in actual cards' color, either in card_instance_t or card_data_t, only
				 * mana_color), and both need to override the color, not just add to it.  They both also break the optimization in Grindstone: Celestial Dawn
				 * since it's never accounted for (Grindstone calls this function with player==2 then only calls get_color_by_internal_id_no_hack(), when the
				 * nonlands should still be white), and Mycosinth Lattice because, since it returns nonzero from this, forces a whole-library mill instead of
				 * forcing it to always only be the two initial cards. */
				if( z == player && get_id(z,i) == CARD_ID_CELESTIAL_DAWN ){
					card_instance_t *instance = get_card_instance(z,i);
					if( instance->timestamp > timestamp ){
						timestamp = instance->timestamp;
						result = COLOR_TEST_WHITE;
					}
				}
				if( get_id(z,i) == CARD_ID_MYCOSYNTH_LATTICE ){
					card_instance_t *instance = get_card_instance(z,i);
					if( instance->timestamp > timestamp ){
						timestamp = instance->timestamp;
						result = COLOR_TEST_COLORLESS;
					}
				}
			}
		}
	}

	return result;
}

static int split_cards_color_table(int player, int card){
	int csvid = player != -1 ? get_id(player, card) : cards_data[card].id;
	int half_played = get_card_instance(player, card)->info_slot;
	int result = 0;
	if( half_played ){
		switch( csvid ){
				// Invasion
				case CARD_ID_STAND_DELIVER: //Not yet coded
					result = (half_played & 1) ? COLOR_TEST_WHITE : COLOR_TEST_BLUE;
					break;
				case CARD_ID_SPITE_MALICE:
					result = (half_played & 1) ? COLOR_TEST_BLUE : COLOR_TEST_BLACK;
					break;
				case CARD_ID_PAIN_SUFFERING: //Not yet coded
					result = (half_played & 1) ? COLOR_TEST_BLACK : COLOR_TEST_RED;
					break;
				case CARD_ID_ASSAULT_BATTERY:
					result = (half_played & 1) ? COLOR_TEST_RED : COLOR_TEST_GREEN;
					break;
				case CARD_ID_WAX_WANE:
					result = (half_played & 1) ? COLOR_TEST_GREEN : COLOR_TEST_WHITE;
					break;
				// Apocalypse
				case CARD_ID_FIRE_ICE:
					result = (half_played & 1) ? COLOR_TEST_RED : COLOR_TEST_BLUE;
					break;
				case CARD_ID_ILLUSION_REALITY: //Not yet coded
					result = (half_played & 1) ? COLOR_TEST_BLUE : COLOR_TEST_GREEN;
					break;
				case CARD_ID_LIFE_DEATH:
					result = (half_played & 1) ? COLOR_TEST_GREEN : COLOR_TEST_BLACK;
					break;
				case CARD_ID_NIGHT_DAY: //Not yet coded
					result = (half_played & 1) ? COLOR_TEST_BLACK : COLOR_TEST_WHITE;
					break;
				case CARD_ID_ORDER_CHAOS:
					result = (half_played & 1) ? COLOR_TEST_WHITE : COLOR_TEST_RED;
					break;
				// Dissension
				case CARD_ID_BOUND_DETERMINED:
					result = (half_played & 1) ? COLOR_TEST_BLACK | COLOR_TEST_GREEN : COLOR_TEST_BLUE | COLOR_TEST_GREEN;
					break;
				case CARD_ID_CRIME_PUNISHMENT:
					result = (half_played & 1) ? COLOR_TEST_BLACK | COLOR_TEST_WHITE : COLOR_TEST_BLACK | COLOR_TEST_GREEN;
					break;
				case CARD_ID_HIDE_SEEK:
					result = (half_played & 1) ? COLOR_TEST_RED | COLOR_TEST_WHITE : COLOR_TEST_BLACK | COLOR_TEST_WHITE;
					break;
				case CARD_ID_HIT_RUN:
					result = (half_played & 1) ? COLOR_TEST_BLACK |  COLOR_TEST_RED : COLOR_TEST_GREEN | COLOR_TEST_RED;
					break;
				case CARD_ID_ODDS_ENDS:
					result = (half_played & 1) ? COLOR_TEST_BLUE | COLOR_TEST_RED : COLOR_TEST_RED | COLOR_TEST_WHITE;
					break;
				case CARD_ID_PURE_SIMPLE:
					result = (half_played & 1) ? COLOR_TEST_GREEN | COLOR_TEST_RED : COLOR_TEST_GREEN | COLOR_TEST_WHITE;
					break;
				case CARD_ID_RESEARCH_DEVELOPMENT:
					result = (half_played & 1) ? COLOR_TEST_BLUE | COLOR_TEST_GREEN : COLOR_TEST_BLUE | COLOR_TEST_RED;
					break;
				case CARD_ID_RISE_FALL:
					result = (half_played & 1) ? COLOR_TEST_BLACK | COLOR_TEST_BLUE : COLOR_TEST_BLACK | COLOR_TEST_RED;
					break;
				case CARD_ID_SUPPLY_DEMAND:
					result = (half_played & 1) ?COLOR_TEST_GREEN | COLOR_TEST_WHITE : COLOR_TEST_BLUE | COLOR_TEST_WHITE;
					break;
				case CARD_ID_TRIAL_ERROR:
					result = (half_played & 1) ? COLOR_TEST_BLUE | COLOR_TEST_WHITE : COLOR_TEST_BLACK | COLOR_TEST_BLUE;
					break;
				// Dragon's Maze
				case CARD_ID_ALIVE_WELL:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_GREEN | COLOR_TEST_WHITE : ((half_played & 1) ? COLOR_TEST_GREEN : COLOR_TEST_WHITE);
					break;
				case CARD_ID_ARMED_DANGEROUS:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_GREEN | COLOR_TEST_RED : ((half_played & 1) ? COLOR_TEST_RED : COLOR_TEST_GREEN);
					break;
				case CARD_ID_BECK_CALL:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLUE | COLOR_TEST_GREEN | COLOR_TEST_WHITE :
												((half_played & 1) ? COLOR_TEST_BLUE | COLOR_TEST_GREEN : COLOR_TEST_BLUE | COLOR_TEST_WHITE);
					break;
				case CARD_ID_BREAKING_ENTERING:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLUE | COLOR_TEST_BLACK | COLOR_TEST_RED :
												((half_played & 1) ? COLOR_TEST_BLUE | COLOR_TEST_BLACK : COLOR_TEST_BLACK | COLOR_TEST_RED);
					break;
				case CARD_ID_CATCH_RELEASE:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLUE | COLOR_TEST_RED | COLOR_TEST_WHITE :
												((half_played & 1) ? COLOR_TEST_BLUE | COLOR_TEST_RED : COLOR_TEST_RED | COLOR_TEST_WHITE);
					break;
				case CARD_ID_DOWN_DIRTY:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_GREEN | COLOR_TEST_BLACK : ((half_played & 1) ? COLOR_TEST_BLACK : COLOR_TEST_GREEN);
					break;
				case CARD_ID_FAR_AWAY:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLACK | COLOR_TEST_BLUE : ((half_played & 1) ? COLOR_TEST_BLUE : COLOR_TEST_BLACK);
					break;
				case CARD_ID_FLESH_BLOOD:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLACK | COLOR_TEST_GREEN | COLOR_TEST_RED :
												((half_played & 1) ? COLOR_TEST_BLACK | COLOR_TEST_GREEN : COLOR_TEST_GREEN | COLOR_TEST_RED);
					break;
				case CARD_ID_GIVE_TAKE:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLUE | COLOR_TEST_GREEN : ((half_played & 1) ? COLOR_TEST_GREEN : COLOR_TEST_BLUE);
					break;
				case CARD_ID_PROFIT_LOSS:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLACK | COLOR_TEST_WHITE : ((half_played & 1) ? COLOR_TEST_WHITE : COLOR_TEST_BLACK);
					break;
				case CARD_ID_PROTECT_SERVE:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLUE | COLOR_TEST_WHITE : ((half_played & 1) ? COLOR_TEST_WHITE : COLOR_TEST_BLUE);
					break;
				case CARD_ID_READY_WILLING:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLACK | COLOR_TEST_GREEN | COLOR_TEST_WHITE :
												((half_played & 1) ? COLOR_TEST_GREEN | COLOR_TEST_WHITE : COLOR_TEST_BLACK | COLOR_TEST_WHITE);
					break;
				case CARD_ID_TOIL_TROUBLE:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLACK | COLOR_TEST_RED : ((half_played & 1) ? COLOR_TEST_BLACK : COLOR_TEST_RED);
					break;
				case CARD_ID_TURN_BURN:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_BLUE | COLOR_TEST_RED : ((half_played & 1) ? COLOR_TEST_BLUE : COLOR_TEST_RED);
					break;
				case CARD_ID_WEAR_TEAR:
					result = (half_played & 1) && (half_played & 2) ? COLOR_TEST_WHITE | COLOR_TEST_RED : ((half_played & 1) ? COLOR_TEST_RED : COLOR_TEST_WHITE);
					break;
				default:
					break;
		}
	}
	return result;
}

int get_color(int player, int card){
	int result = get_abilities(player, card, EVENT_SET_COLOR, -1);
	int result_split_card = split_cards_color_table(player, card);
	if( result_split_card ){
		result = result_split_card | get_global_color_hack(player);
	}
	return result;
}

int get_color_by_internal_id_no_hack(int internal_id){

	int clr = cards_data[internal_id].color;

	if( cards_data[internal_id].type == TYPE_LAND ){
		clr = cards_data[internal_id].id == CARD_ID_DRYAD_ARBOR ? COLOR_TEST_GREEN : 0;
	}

	if ((cards_data[internal_id].type & TYPE_ARTIFACT) && cards_data[internal_id].cc[0] == 0){	// No colored mana in casting cost, so not a colored artifact but a mana-producing one
		clr = 0;
	}

	clr &= COLOR_TEST_ANY_COLORED;	// Strip out COLOR_TEST_COLORLESS and COLOR_TEST_ARTIFACT, for mana-producing cards that happen to produce those

	return clr;
}

// Use this only for checking few cards in a row, otherwise store get_global_color_hack in a variable and use get_color_by_internal_id_no_hack
int get_color_by_internal_id(int player, int internal_id){

	int clr = get_color_by_internal_id_no_hack(internal_id);

	clr |= get_global_color_hack(player);

	return clr;
}

// A shim between get_color() and get_color_by_internal_id().
int get_color_real_or_iid(int player_or_owner, int player_or_negone, int card_or_iid)
{
  // That player=-1 card=iid convention has always been a terrible idea.
  return (player_or_negone >= 0
		  ? get_color(player_or_negone, card_or_iid)					// a concrete player/card pair.
		  : get_color_by_internal_id(player_or_owner, card_or_iid));	// a card not-in-play
}

int is_creature_dead(void){
	int p=0;
	for(p=0;p<2;p++){
		int c=0;
		for(c=0;c< active_cards_count[p];c++){
			card_data_t* card_d = get_card_data(p, c);
			if((card_d->type & TYPE_CREATURE) && in_play(p, c)){
				if( can_regenerate(p, c) ){
					return 0x63;
				}
			}
		}
	}
	return 0;
}

int pick_creature_to_regen(target_definition_t td){
	if( pick_target(&td, "TARGET_CREATURE_TO_REGEN") ){
		card_instance_t *instance = get_card_instance(td.player, td.card);
		while( spell_fizzled != 1 ){
			card_instance_t *creature = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if( creature->kill_code == KILL_DESTROY && !(creature->token_status & STATUS_CANNOT_REGENERATE) ){
				break;
			}
			pick_target(&td, "TARGET_CREATURE_TO_REGEN");
		}
		if( spell_fizzled != 1 ){
			return 1;
		}
	}
	return 0;
}

static int split_cards_cmc_table_by_id(int csvid, int half_played){
	// For putting things simple, we'll always assume that the CMC we get from any other zone aside from STATE_IN_PLAY is the lesser.
	int result = 0;
	switch( csvid ){
				// Invasion
			case CARD_ID_STAND_DELIVER: //Not yet coded
				{
					if( (half_played & 1) ){
						result+=1;
					}
					if( (half_played & 2) ){
						result+=3;
					}
					if( ! half_played ){
						result = 1;
					}
				}
				break;
			case CARD_ID_SPITE_MALICE:
				result = 4;
				break;
			case CARD_ID_PAIN_SUFFERING: //Not yet coded
				{
					if( (half_played & 1) ){
						result+=1;
					}
					if( (half_played & 2) ){
						result+=4;
					}
					if( ! half_played ){
						result = 1;
					}
				}
				break;
			case CARD_ID_ASSAULT_BATTERY:
				{
					if( (half_played & 1) ){
						result+=1;
					}
					if( (half_played & 2) ){
						result+=4;
					}
					if( ! half_played ){
						result = 1;
					}
				}
				break;
			case CARD_ID_WAX_WANE:
				result = 1;
				break;
			// Apocalypse
			case CARD_ID_FIRE_ICE:
				result = 2;
				break;
			case CARD_ID_ILLUSION_REALITY: //Not yet coded
				{
					if( (half_played & 1) ){
						result+=1;
					}
					if( (half_played & 2) ){
						result+=3;
					}
					if( ! half_played ){
						result = 1;
					}
				}
				break;
			case CARD_ID_LIFE_DEATH:
				{
					if( (half_played & 1) ){
						result+=1;
					}
					if( (half_played & 2) ){
						result+=2;
					}
					if( ! half_played ){
						result = 1;
					}
				}
				break;
			case CARD_ID_NIGHT_DAY: //Not yet coded
				{
					if( (half_played & 1) ){
						result+=1;
					}
					if( (half_played & 2) ){
						result+=3;
					}
					if( ! half_played ){
						result = 1;
					}
				}
				break;
			case CARD_ID_ORDER_CHAOS:
				{
					if( (half_played & 1) ){
						result+=3;
					}
					if( (half_played & 2) ){
						result+=4;
					}
					if( ! half_played ){
						result = 3;
					}
				}
				break;
			// Dissension
			case CARD_ID_BOUND_DETERMINED:
				{
					if( (half_played & 1) ){
						result+=5;
					}
					if( (half_played & 2) ){
						result+=2;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_CRIME_PUNISHMENT:
				{
					if( (half_played & 1) ){
						result+=5;
					}
					if( (half_played & 2) ){
						result+=2;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_HIDE_SEEK:
				result = 2;
				break;
			case CARD_ID_HIT_RUN:
				{
					if( (half_played & 1) ){
						result+=3;
					}
					if( (half_played & 2) ){
						result+=5;
					}
					if( ! half_played ){
						result = 3;
					}
				}
				break;
			case CARD_ID_ODDS_ENDS:
				{
					if( (half_played & 1) ){
						result+=2;
					}
					if( (half_played & 2) ){
						result+=5;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_PURE_SIMPLE:
				result = 3;
				break;
			case CARD_ID_RESEARCH_DEVELOPMENT:
				{
					if( (half_played & 1) ){
						result+=2;
					}
					if( (half_played & 2) ){
						result+=5;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_RISE_FALL:
				result = 2;
				break;
			case CARD_ID_SUPPLY_DEMAND:
				{
					if( (half_played & 1) ){
						result+=2;
					}
					if( (half_played & 2) ){
						result+=3;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_TRIAL_ERROR:
				result = 2;
				break;
			// Planar Chaos
			case CARD_ID_BOOM_BUST:
			case CARD_ID_ROUGH_TUMBLE:
				{
					if( (half_played & 1) ){
						result+=2;
					}
					if( (half_played & 2) ){
						result+=6;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_DEAD_GONE:
				{
					if( (half_played & 1) ){
						result+=1;
					}
					if( (half_played & 2) ){
						result+=3;
					}
					if( ! half_played ){
						result = 1;
					}
				}
				break;
			// Dragon's Maze
			case CARD_ID_ALIVE_WELL:
				{
					if( (half_played & 1) ){
						result+=1;
					}
					if( (half_played & 2) ){
						result+=4;
					}
					if( ! half_played ){
						result = 1;
					}
				}
				break;
			case CARD_ID_ARMED_DANGEROUS:
				{
					if( (half_played & 1) ){
						result+=2;
					}
					if( (half_played & 2) ){
						result+=4;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_BECK_CALL:
				{
					if( (half_played & 1) ){
						result+=2;
					}
					if( (half_played & 2) ){
						result+=6;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_BREAKING_ENTERING:
				{
					if( (half_played & 1) ){
						result+=2;
					}
					if( (half_played & 2) ){
						result+=6;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_CATCH_RELEASE:
				{
					if( (half_played & 1) ){
						result+=3;
					}
					if( (half_played & 2) ){
						result+=6;
					}
					if( ! half_played ){
						result = 3;
					}
				}
				break;
			case CARD_ID_DOWN_DIRTY:
				{
					if( (half_played & 1) ){
						result+=4;
					}
					if( (half_played & 2) ){
						result+=3;
					}
					if( ! half_played ){
						result = 3;
					}
				}
				break;
			case CARD_ID_FAR_AWAY:
				{
					if( (half_played & 1) ){
						result+=2;
					}
					if( (half_played & 2) ){
						result+=3;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_FLESH_BLOOD:
				{
					if( (half_played & 1) ){
						result+=5;
					}
					if( (half_played & 2) ){
						result+=2;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_GIVE_TAKE:
				result = 3;
				break;
			case CARD_ID_PROFIT_LOSS:
				{
					if( (half_played & 1) ){
						result+=2;
					}
					if( (half_played & 2) ){
						result+=3;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_PROTECT_SERVE:
				{
					if( (half_played & 1) ){
						result+=3;
					}
					if( (half_played & 2) ){
						result+=2;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_READY_WILLING:
				result = 3;
				break;
			case CARD_ID_TOIL_TROUBLE:
				result = 3;
				break;
			case CARD_ID_TURN_BURN:
				{
					if( (half_played & 1) ){
						result+=3;
					}
					if( (half_played & 2) ){
						result+=2;
					}
					if( ! half_played ){
						result = 2;
					}
				}
				break;
			case CARD_ID_WEAR_TEAR:
				{
					if( (half_played & 1) ){
						result+=2;
					}
					if( (half_played & 2) ){
						result+=1;
					}
					if( ! half_played ){
						result = 1;
					}
				}
				break;
			default:
				break;
	}
	return result;
}

static int split_cards_cmc_table(int player, int card){
	// For putting things simple, we'll always assume that the CMC we get from any other zone aside from STATE_IN_PLAY is the lesser.
	int csvid = player != -1 ? get_id(player, card) : cards_data[card].id;
	int half_played = player != -1 ? get_card_instance(player, card)->info_slot : 0;
	return split_cards_cmc_table_by_id(csvid, half_played);
}

int get_cmc_by_id(int id){

	if( id == CARD_ID_LOTUS_BLOOM ){
	   return 0;
	}

	card_ptr_t* c = cards_ptr[ id ];

	int result = c->req_black + c->req_blue + c->req_green + c->req_red + c->req_white;
	int cless = c->req_colorless;

	if( cless == 40 ){
		cless = 0;
	}
	result+=cless;
	int result_split_card = split_cards_cmc_table_by_id(id, 0);
	return result_split_card ? result_split_card : result;
}

int get_cmc(int player, int card){
	if (player == -1){
		return get_cmc_by_internal_id(card);
	}
	card_instance_t *instance = get_card_instance(player, card);
	if( is_token(player, card) ){
		return 0;
	}
	int iid = instance->internal_card_id;
	if (iid == -1){
		iid = instance->backup_internal_card_id;
	}
/* From SOI "Double-Faced Cards Rules Changes" : "Under the new rules, the converted mana cost of the back face of a DFC is based on
	the mana cost of the front face. (Previously, because the back faces lacked mana costs, their converted mana costs were all 0.)
*/
	if( player != -1 && in_play(player, card) && check_special_flags3(player, card, SF3_CARD_IS_FLIPPED) ){
		iid = get_original_internal_card_id(player, card);
	}
	int result = get_cmc_by_internal_id(iid);
	if(is_x_spell(player, card) && (instance->state & STATE_INVISIBLE)){
		result += instance->info_slot;
	}
	int result_split_card = split_cards_cmc_table(player, card);
	if( result_split_card ){
		// Take in account split cards with X in the casting cost, which is stored in "targets[1].card" instead on "info_slot"
		int csvid = get_id(player, card);
		int half_played = instance->info_slot;
		switch( csvid ){
			case CARD_ID_CRIME_PUNISHMENT:
				{
					if( (half_played & 2) ){
						result_split_card+= (instance->targets[1].card > 0 ? instance->targets[1].card : 0);
					}
				}
				break;
			case CARD_ID_SUPPLY_DEMAND:
				{
					if( (half_played & 1) ){
						result_split_card+= (instance->targets[1].card > 0 ? instance->targets[1].card : 0);
					}
				}
				break;
			default:
			break;
		}
	}
	return result_split_card ? result_split_card : result;
}

int get_cmc_by_internal_id(int id){
	card_data_t* card_d = &cards_data[ id  ];
	int cmc = card_d->cc[0] + card_d->cc[1];
	if( card_d->cc[1] > 16 ){
		cmc = card_d->cc[0];
	}

	if( card_d->id == CARD_ID_LOTUS_BLOOM ){
	   return 0;
	}

	int result_split_card = split_cards_cmc_table(-1, id);
	return result_split_card ? result_split_card : cmc;
}

int paying_mana(void){
	return !unknown62BCEC || needed_mana_colors;
}

// A permanent legacy effect that does nothing.
int empty(int player, int card, event_t event){
	/* (While this sometimes has an address assigned in ManalinkEh.asm, it's not referenced by ct_all.csv or the exe; it's used as a placeholder when an address
	 * is freed.) */
	return 0;
}

// A legacy effect that does nothing except get removed at end of turn.
int empty_until_eot(int player, int card, event_t event)
{
  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int legacy_change_color(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( instance->targets[1].player > 0 ){
		if( event == EVENT_SET_COLOR && affect_me(p, c) ){
			if( instance->targets[1].player & CHANGE_COLOR_ADD ){
				event_result |= instance->targets[1].card;
			}
			if( instance->targets[1].player & CHANGE_COLOR_SET ){
				event_result = instance->targets[1].card;
			}
		}
		if( (instance->targets[1].player & CHANGE_COLOR_END_AT_EOT) && eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	return 0;
}

void change_color(int player, int card, int t_player, int t_card, color_test_t new_color, change_color_t mode){
	// mode = 1 -> Add
	// mode = 2 -> Set
	// mode +4 -> end at eot
	// mode +8 -> don't play sound (e.g., this is being added to the affected card as it's being created)
	int legacy = create_targetted_legacy_effect(player, card, &legacy_change_color, t_player, t_card);
	card_instance_t *instance = get_card_instance(player, legacy);
	instance->targets[1].player = mode;
	if (!(mode & CHANGE_COLOR_NO_SLEIGHT)){
		new_color = get_sleighted_color_test(player, card, new_color);
	}
	instance->targets[1].card = new_color;
	if (!(mode & CHANGE_COLOR_NO_SOUND)){
		play_sound_effect(WAV_CHANGEC);
	}
}

int effect_unearth(int player, int card, event_t event ){
	card_instance_t *instance = get_card_instance(player, card);
	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	haste(p, c, event);

	if (instance->targets[1].player < 0 && in_play(p, c)){
		instance->targets[1].player = 0;
		set_special_flags(p, c, SF_UNEARTH);	// In case it was missed by the caller, or caller couldn't access the legacy directly (as in token generation).
	}

	if( leaves_play(p, c, event) ){
		add_card_to_rfg(get_owner(p, c), get_original_internal_card_id(p, c));
	}
	if( instance->targets[1].card != 66 && eot_trigger(player, card, event) ){
		instance->targets[1].card = 66;
		if( ! check_state(p, c, STATE_OUBLIETTED) ){
			kill_card(p, c, KILL_REMOVE);
		}
	}
	return 0;
}

static int haste_and_exile_your_upkeep(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		haste(instance->damage_target_player, instance->damage_target_card, event);
	}

	if( current_turn == instance->damage_target_player && upkeep_trigger(player, card, event) ){
		kill_card(instance->damage_target_player, instance->damage_target_card, KILL_REMOVE);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

static int reanimate_permanent_impl(int player, int card, int target_graveyard, int selected, reanimate_mode_t action, counter_t counter_type, int (*effect_fn)(int, int, event_t), void (*before_etb)(int, int)){
	// player = who get the reanimate card
	// target_graveyard = the player who owns the graveyard in which the cards to reanimate is

	if( selected == -1 ){
		return -1;
	}

	if (action < 0){
		action = 0;
	}

	const int *grave = get_grave(target_graveyard);
	int ca_player = player;
	int card_added = (action & REANIMATE_SPECIAL_R_ALL) ? selected : add_card_to_hand(ca_player, grave[selected] );

	if( !(action & REANIMATE_SPECIAL_R_ALL) ){
		remove_card_from_grave(target_graveyard, selected);
	}

	if( !(action & REANIMATE_RETURN_TO_HAND) ){
		if( is_what(-1, grave[selected], TYPE_CREATURE) && check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE) ){
			return -1;
		}
	}

	if ((action & REANIMATE_ATTACH_AS_AURA) && player >= 0 && card >= 0){
		/* Set damage_target_player/card before put_into_play(), so that
		 * 1. It displays as attached while prompting for the creature's come-into-play effects
		 * 2. If the creature's come-into-play effects destroy itself or the aura, the other gets sacrificed (if player/card is Animate Dead or similar) */
		set_special_flags(player, card, SF_TARGETS_ALREADY_SET);
		attach_aura_to_target(player, card, EVENT_RESOLVE_SPELL, ca_player, card_added);
		/*
		card_instance_t* instance = get_card_instance(player, card);
		instance->damage_target_player = instance->targets[0].player = ca_player;
		instance->damage_target_card = instance->targets[0].card = card_added;
		*/
	}

	int effect_src = card >= 0 ? card : card_added;

	if (counter_type != COUNTER_invalid){
		++hack_silent_counters;
		add_counter(ca_player, card_added, counter_type);
		--hack_silent_counters;
	}

	if (action & REANIMATE_TAP){
		get_card_instance(ca_player, card_added)->state |= STATE_TAPPED;	// not tap_card() to avoid the extra EVENT_TAP_CARD
	}

	if (action & REANIMATE_PLUS1_PLUS1_COUNTER){
		add_1_1_counter(ca_player, card_added);
	}

	if (action & REANIMATE_MINUS1_MINUS1_COUNTER){
		add_minus1_minus1_counters(ca_player, card_added, 1);
	}

	if (effect_fn){
		create_targetted_legacy_effect(player, effect_src, effect_fn, ca_player, card_added);
	}

	if (action == REANIMATE_UNEARTH){
		convert_to_token(ca_player, card_added);
		set_special_flags(ca_player, card_added, SF_UNEARTH);
		set_special_flags2(ca_player, card_added, SF2_WILL_BE_EXILED_IF_PUTTED_IN_GRAVE);
	}

	// Grimoire of the Dead, Dread Slaver, Rise From the Grave
	if (action & REANIMATE_ADD_BLACK_ZOMBIE){
		change_color(player, effect_src, ca_player, card_added, COLOR_TEST_BLACK, CHANGE_COLOR_ADD|CHANGE_COLOR_NO_SOUND);
		add_a_subtype(ca_player, card_added, SUBTYPE_ZOMBIE);
	}

	if (action & REANIMATE_HASTE_AND_EXILE_AT_EOT){
		create_targetted_legacy_effect(player, effect_src, &haste_and_remove_eot, ca_player, card_added);
	}

	if (action & REANIMATE_HASTE_AND_EXILE_AT_YOUR_UPKEEP){
		create_targetted_legacy_effect(player, effect_src, &haste_and_exile_your_upkeep, ca_player, card_added);
	}

	if (action & REANIMATE_HASTE_UNTIL_EOT){
		pump_ability_until_eot(player, effect_src, ca_player, card_added, 0, 0, 0, SP_KEYWORD_HASTE);
	}


	if (action & REANIMATE_RETURN_TO_PLAY_TRANSFORMED){
		transform(ca_player, card_added);
	}

	if( player != target_graveyard ){
		if( player == HUMAN ){
			add_state(ca_player, card_added, STATE_OWNED_BY_OPPONENT);
		}
		else{
			remove_state(ca_player, card_added, STATE_OWNED_BY_OPPONENT);
		}
	}

	if (before_etb){
	  (*before_etb)(ca_player, card_added);
	}

	if(!(action & (REANIMATE_NO_CONTROL_LEGACY |
					REANIMATE_UNEARTH |
					REANIMATE_ADD_BLACK_ZOMBIE |
					REANIMATE_HASTE_AND_EXILE_AT_EOT |
					REANIMATE_ATTACH_AS_AURA |
					REANIMATE_RETURN_TO_HAND)
		)
		&& !effect_fn
	  ){
		if( effect_src != -1 && ((player != target_graveyard) || get_owner(ca_player, card_added) != player) ){
			create_targetted_legacy_effect(player, effect_src, &empty, ca_player, card_added);
		}
	}

	if (!(action & REANIMATE_RETURN_TO_HAND)){
		set_special_flags3(ca_player, card_added, SF3_REANIMATED);
		put_into_play(ca_player, card_added);
		if( action == REANIMATE_UNEARTH && in_play(ca_player, card_added) ){
			create_targetted_legacy_effect(player, effect_src, &effect_unearth, ca_player, card_added);
		}
	}

	return card_added;
}

int reanimate_permanent(int player, int card, int target_graveyard, int selected, reanimate_mode_t action){
  return reanimate_permanent_impl(player, card, target_graveyard, selected, action, COUNTER_invalid, NULL, NULL);
}

// Just like reanimate_permanent(), but attach an effect with function effect_fn to the animated permanent before putting it into play.
int reanimate_permanent_with_effect(int player, int card, int target_graveyard, int selected, reanimate_mode_t action, int (*effect_fn)(int, int, event_t)){
  return reanimate_permanent_impl(player, card, target_graveyard, selected, action, COUNTER_invalid, effect_fn, NULL);
}

// Just like reanimate_permanent_with_effect(), but also add a counter to it before putting it into play.
int reanimate_permanent_with_counter_and_effect(int player, int card, int target_graveyard, int selected, reanimate_mode_t action, counter_t counter_type, int (*effect_fn)(int, int, event_t))
{
  return reanimate_permanent_impl(player, card, target_graveyard, selected, action, counter_type, effect_fn, NULL);
}

// Just like reanimate_permanent(), but call an artibtrary function on {player,card} just before it's put on the battlefield.
int reanimate_permanent_with_function(int player, int card, int target_graveyard, int selected, reanimate_mode_t action, void (*before_etb)(int, int)){
  return reanimate_permanent_impl(player, card, target_graveyard, selected, action, COUNTER_invalid, NULL, before_etb);
}

static int new_reanimate_all_impl(int player, int card, int targ_player, test_definition_t *this_test, reanimate_mode_t action,
									counter_t counter_type, int (*effect_fn)(int, int, event_t), void (*before_etb)(int, int)){
	int csvid = card == -1 ? -1 : get_id(player, card);
	int result = 0;
	int i;
	int grafdiggers_cage_flag = check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE);
	int to_reanimate[2][500];
	int trc[2] = {0, 0};

	// To prevent unwanted interactions, global reanimation will be done in 3 steps.
	// First step: removing from graveyards all cards that match "test" and storing in the players arrays
	for (i = 0; i < 2; ++i){
		if (targ_player == i || targ_player == ANYBODY){
			int count;
			int lower_limit = (i == targ_player && csvid == CARD_ID_CLEANSING_MEDITATION) ? get_card_instance(player, card)->info_slot : -1;

			const int* grave = get_grave(i);
			for (count = count_graveyard(i)-1; count > lower_limit; --count){
				if (is_what(-1, grave[count], TYPE_PERMANENT) && new_make_test(i, grave[count], -1, this_test)){
					if( ! is_what(-1, grave[count], TYPE_CREATURE) || (is_what(-1, grave[count], TYPE_CREATURE) && ! grafdiggers_cage_flag) ){
						if( trc[i] < 500 ){
							to_reanimate[i][trc[i]] = grave[count];
							trc[i]++;
							if( action & REANIMATE_SPECIAL_LIVING_DEAD ){
								rfg_card_from_grave(i, count);
							}
							else{
								remove_card_from_grave(i, count);
							}
							result++;
						}
					}
				}
			}
		}
	}

	// Intermediate step: if REANIMATE_SPECIAL_LIVING_DEAD is used, each player will sacrifice all his creatures.
	// As now, there is no card that says "When THIS dies, DO SOMETHING with target exiled card",
	// so we're pretty saved from unwanted interactions
	if( action & REANIMATE_SPECIAL_LIVING_DEAD ){
		test_definition_t this_test2;
		default_test_definition(&this_test2, TYPE_CREATURE);
		APNAP(p, {new_manipulate_all(player, card, p, &this_test2, KILL_SACRIFICE);});
	}

	// Second step: adding to player's hands all the card from the player's arrays
	// If (action & REANIMATE_ALL_UNDER_CASTERS_CONTROL), they will be all added to PLAYER's hand and their "state" will be set accordingly.
	if( action & REANIMATE_ALL_UNDER_CASTERS_CONTROL ){
		for(i=0; i<trc[player]; i++){
			int good = 1;
			if( action & REANIMATE_SPECIAL_LIVING_DEAD ){
				if( ! check_rfg(player, cards_data[to_reanimate[player][i]].id) ){
					good = 0;
				}
			}
			if( good ){
				if( action & REANIMATE_SPECIAL_LIVING_DEAD ){
					remove_card_from_rfg(player, cards_data[to_reanimate[player][i]].id);
				}
				int card_added = add_card_to_hand(player, to_reanimate[player][i]);
				to_reanimate[player][i] = card_added;
			}
		}
		int ntrc = trc[player];
		for(i=0; i<trc[1-player]; i++){
			int good = 1;
			if( action & REANIMATE_SPECIAL_LIVING_DEAD ){
				if( ! check_rfg(1-player, cards_data[to_reanimate[1-player][i]].id) ){
					good = 0;
				}
			}
			if( good ){
				if( action & REANIMATE_SPECIAL_LIVING_DEAD ){
					remove_card_from_rfg(player, cards_data[to_reanimate[1-player][i]].id);
				}
				int card_added = add_card_to_hand(player, to_reanimate[1-player][i]);
				get_card_instance(player, card_added)->state ^= STATE_OWNED_BY_OPPONENT;
				to_reanimate[player][ntrc] = card_added;
				ntrc++;
			}
		}
		trc[player] = ntrc;
		trc[1-player] = 0;
	}
	else{
		for (i = 0; i < 2; ++i){
			int k;
			for(k=0; k<trc[i]; k++){
				int good = 1;
				if( action & REANIMATE_SPECIAL_LIVING_DEAD ){
					if( ! check_rfg(i, cards_data[to_reanimate[i][k]].id) ){
						good = 0;
					}
				}
				if( good ){
					if( action & REANIMATE_SPECIAL_LIVING_DEAD ){
						remove_card_from_rfg(i, cards_data[to_reanimate[i][k]].id);
					}
					int card_added = add_card_to_hand(i, to_reanimate[i][k]);
					to_reanimate[i][k] = card_added;
				}
			}
		}
	}

	// Thirs step: calling "reanimate_permanent" with REANIMATE_SPECIAL_R_ALL so it won't try to remove the cards from graveyards again
	// "reanimate_permanent" will set flags, effects and interactions properly and put the card into play.
	action |= REANIMATE_SPECIAL_R_ALL;
	if( action & REANIMATE_ALL_UNDER_CASTERS_CONTROL ){
		int k;
		for(k=0; k<trc[player]; k++){
			reanimate_permanent(player, card, player, to_reanimate[player][k], action);
		}
	}
	else{
		APNAP(p,{
					int k;
					for(k=0; k<trc[p]; k++){
						reanimate_permanent(p, -1, p, to_reanimate[p][k], action);
					}
				};
		);
	}

	return result;
}

int new_reanimate_all(int player, int card, int targ_player, test_definition_t *this_test, reanimate_mode_t action){
	return new_reanimate_all_impl(player, card, targ_player, this_test, action, 0, NULL, NULL);
}

void reanimate_all(int player, int card, int targ_player, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5, reanimate_mode_t action)
{
	test_definition_t test;
	new_default_test_definition(&test, type, "");
	test.type_flag = flag1;
	test.subtype = subtype;
	test.subtype_flag = flag2;
	test.color = color;
	test.color_flag = flag3;
	test.id = id;
	test.id_flag = flag4;
	test.cmc = cc;
	test.cmc_flag = flag5;
	new_reanimate_all(player, card, targ_player, &test, action);
}

// did this card just go to a graveyard from play?
int graveyard_from_play(int player, int card, event_t event){
	if(event == EVENT_GRAVEYARD_FROM_PLAY){
		if( affect_me( player, card) && in_play(player, card) ){
			card_instance_t *instance = get_card_instance( player, card);
			if(instance->kill_code != KILL_REMOVE && instance->kill_code != 0 ){
				return 1;
			}
		}
	}
	return 0;
}

// Should be called only before entering or as enters the battlefield
void cloning(int player, int card, int t_player, int t_card){
	card_instance_t *instance = get_card_instance(player, card);
	if( t_player == -1 ){
		instance->targets[12].card = t_card;
	}
	else{
		card_instance_t *target = get_card_instance(t_player, t_card);
		int iid = target->internal_card_id;
		if (iid == -1)
		  iid = target->backup_internal_card_id;

		switch (get_id(player, card)){
			case CARD_ID_PHYREXIAN_METAMORPH:
				if (!(cards_data[iid].type & TYPE_ARTIFACT)){	// not is_what() - deliberately ignore SF2_MYCOSYNTH_LATTICE
					iid = create_a_card_type(iid);
					cards_data[iid].type |= TYPE_ARTIFACT;
				}
				break;

			case CARD_ID_COPY_ARTIFACT:
				if (!(cards_data[iid].type & TYPE_ENCHANTMENT) || is_planeswalker(-1, iid)){	// not is_what() - deliberately ignore SF2_ENCHANTED_EVENING
					iid = create_a_card_type(iid);
					cards_data[iid].type |= TYPE_ENCHANTMENT;
				}
				break;
		}
		if( target->targets[13].player > -1 && target->targets[13].card > -1 ){
			if( t_player != -1 && check_status(t_player, t_card, STATUS_ANIMATED) ){
				instance->targets[13].player = cards_data[get_original_internal_card_id(t_player, t_card)].id;
				instance->targets[13].card = cards_data[get_original_internal_card_id(t_player, t_card)].id+1;
			}
			else{
				instance->targets[13].player = target->targets[13].player;
				instance->targets[13].card = target->targets[13].card;
			}
		}
		if( target->targets[12].player > -1 ){
			if( t_player != -1 && check_status(t_player, t_card, STATUS_ANIMATED) ){
				instance->targets[12].player = cards_data[get_original_internal_card_id(t_player, t_card)].id;
			}
			else{
				instance->targets[12].player = target->targets[12].player;
			}
		}
		if( t_player != -1 && check_status(t_player, t_card, STATUS_ANIMATED) ){
			instance->targets[12].card = get_original_internal_card_id(t_player, t_card);
		}
		else{
			instance->targets[12].card = iid;
		}
		instance->initial_color = 0;
		copy_token_characteristics_for_clone(player, card, t_player, t_card);
	}
}

// Should be called if changing types after already being in play.
void cloning_and_verify_legend(int player, int card, int t_player, int t_card)
{
  cloning(player, card, t_player, t_card);

  // See also flip_card(), which should parallel this.
  card_instance_t* instance = get_card_instance(player, card);
  instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE;
  get_abilities(player, card, EVENT_CHANGE_TYPE, -1);
  if (in_play(player, card))
	verify_legend_rule(player, card, get_id(player, card));
}

void cloning_card(int player, int card, event_t event)
{
  if (event == EVENT_CHANGE_TYPE && affect_me(player, card)
	  && !(land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->targets[12].card != -1)
		event_result = instance->targets[12].card;
	}
}

// t_player and t_card only need to be initialized during EVENT_RESOLVE_SPELL.
void enters_the_battlefield_as_copy_of(int player, int card, event_t event, int t_player, int t_card)
{
  cloning_card(player, card, event);

  if (event == EVENT_RESOLVE_SPELL && t_card >= 0)
	{
	  cloning(player, card, t_player, t_card);

	  card_instance_t* instance = get_card_instance(player, card);

	  instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE;
	  get_abilities(player, card, EVENT_CHANGE_TYPE, -1);

	  call_card_function_i(instance, player, card, EVENT_RESOLVE_SPELL);
	  dispatch_event(player, card, EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE);
	}
}

// As enters_the_battlefield_as_copy_of(), but prompts for a target.  Returns nonzero if copies.  td only needs to be initialized during EVENT_RESOLVE_SPELL.
int enters_the_battlefield_as_copy_of_any(int player, int card, event_t event, target_definition_t* td, const char* prompt)
{
  target_t tgt = { -1, -1 };
  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->state |= STATE_OUBLIETTED;
	  int ok = pick_target(td, prompt);
	  instance->state &= ~STATE_OUBLIETTED;
	  if (ok)
		{
		  instance->number_of_targets = 0;
		  if (instance->targets[0].card != card || instance->targets[0].player != player)	// didn't pick self
			tgt = instance->targets[0];	// struct copy
		}
	  else
		cancel = 0;
	}

  enters_the_battlefield_as_copy_of(player, card, event, tgt.player, tgt.card);

  return tgt.card >= 0;
}

// Common case of enters_the_battlefield_as_copy_of_any().
int enters_the_battlefield_as_copy_of_any_creature(int player, int card, event_t event)
{
  target_definition_t td;
  if (event == EVENT_RESOLVE_SPELL)
	{
	  base_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = ANYBODY;
	}
  return enters_the_battlefield_as_copy_of_any(player, card, event, &td, "SELECT_A_CREATURE");
}

static int shapeshift_target_effect(int player, int card, event_t event)
{
  /* Local storage:
   * targets[0]: Redundant copy of {instance->damage_target_player, instance->damage_target_card}
   * targets[1].player: mode
   * targets[1].card: internal_card_id to change into
   * targets[2].player: effect to add, if SHAPESHIFT_EFFECT_WHEN_REMOVE is set
   * targets[4]-targets[8]: token characteristic-setting effects
   * targets[10].player: previous special_infos (when copying a token, it has to set them on the card being copied to; this is used to restore them afterward)
   */
  card_instance_t* instance = get_card_instance(player, card);
  int t_player = instance->damage_target_player, t_card = instance->damage_target_card;

  int mode = instance->targets[1].player;
  int iid = instance->targets[1].card;
  int effect_fn = instance->targets[2].player;
  int counter_type = BYTE3(mode);

  if (event == EVENT_CHANGE_TYPE && affect_me(t_player, t_card)
	  && iid != -1 && !(land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS))
	{
	  event_result = iid;
	  if (mode & SHAPESHIFT_IMPL_COPYING_TOKEN)
		set_special_flags2(t_player, t_card, SF2_TEMPORARY_COPY_OF_TOKEN);
	}

  if (mode & SHAPESHIFT_IMPL_COPYING_TOKEN)
	token_characteristic_setting_effects(player, card, event, t_player, t_card);

  if (((mode & SHAPESHIFT_UNTIL_EOT) && eot_trigger(player, card, event))
	  || (event == EVENT_STATIC_EFFECTS && (mode & SHAPESHIFT_IMPL_COUNTER_FLAG)
		  && !count_counters(t_player, t_card, counter_type)))
	{
	  if ((mode & SHAPESHIFT_EFFECT_WHEN_REMOVE) && effect_fn != -1)
		create_targetted_legacy_activate(instance->damage_source_player, instance->damage_source_card,
										 (int (*)(int, int, event_t))effect_fn,
										 t_player, t_card);

	  if ((mode & SHAPESHIFT_IMPL_COPYING_TOKEN)
		  && !check_special_flags2(player, card, SF2_TEMPORARY_COPY_OF_TOKEN))	// ie, another shapeshift effect isn't overwriting it too
		set_special_infos(t_player, t_card, instance->targets[10].player);

	  kill_card(player, card, KILL_REMOVE);

	  card_instance_t* aff = get_card_instance(t_player, t_card);
	  aff->regen_status |= KEYWORD_RECALC_CHANGE_TYPE;
	  get_abilities(t_player, t_card, EVENT_CHANGE_TYPE, -1);
	  if (in_play(t_player, t_card))
		verify_legend_rule(t_player, t_card, get_id(t_player, t_card));
	}

  return 0;
}

static int real_shapeshift_target(int src_player, int src_card, int t_player, int t_card, int turn_into_player, int turn_into_card, int mode)
{
  int legacy = create_targetted_legacy_effect(src_player, src_card, &shapeshift_target_effect, t_player, t_card);
  card_instance_t* leg = get_card_instance(src_player, legacy);
  leg->targets[0].player = t_player;
  leg->targets[0].card = t_card;
  leg->targets[1].player = mode & ~SHAPESHIFT_IMPL_COPYING_TOKEN;
  leg->targets[10].player = 0;

  if (turn_into_player == -1)
	leg->targets[1].card = turn_into_card;
  else
	{
	  card_instance_t* turn_into = get_card_instance(turn_into_player, turn_into_card);
	  int iid = turn_into->internal_card_id;
	  if (iid == -1)
		iid = turn_into->backup_internal_card_id;

	  leg->targets[1].card = iid;

	  if (copy_token_characteristics_for_clone(src_player, legacy, turn_into_player, turn_into_card))
		{
		  leg->targets[1].player |= SHAPESHIFT_IMPL_COPYING_TOKEN;
		  leg->targets[10].player = get_special_infos(t_player, t_card);
		  if (check_special_flags2(t_player, t_card, SF2_TEMPORARY_COPY_OF_TOKEN))
			{
			  /* Another shapeshift effect is already overwriting the token.  Make it so this effect is the only one setting the token's characteristic values,
			   * and so we restore the original special_infos, not the one temporarily set by the first effect. */
			  card_instance_t* inst;
			  int p, c;
			  for (p = 0; p <= 1; ++p)
				for (c = 0; c < active_cards_count[p]; ++c)
				  if ((inst = in_play(p, c)) && inst->internal_card_id == LEGACY_EFFECT_CUSTOM && inst->info_slot == (int)shapeshift_target_effect
					  && inst->damage_target_player == p && inst->damage_target_card == c
					  && (inst->targets[1].player & SHAPESHIFT_IMPL_COPYING_TOKEN)
					  && !check_special_flags2(p, c, SF2_TEMPORARY_COPY_OF_TOKEN))
					{
					  set_special_flags2(p, c, SF2_TEMPORARY_COPY_OF_TOKEN);
					  leg->targets[10].player = get_special_infos(p, c);
					  /* There *should* be only one unflagged effect, since currently all shapeshift effects that copy another permanent end at end of turn, but
					   * continue searching just in case. */
					}
			}
		  set_special_infos(t_player, t_card, get_special_infos(src_player, legacy));
		}
	}

  return legacy;
}

// Shapeshifts a single permanent.
int shapeshift_target(int src_player, int src_card, int t_player, int t_card, int turn_into_player, int turn_into_card, int mode)
{
  int legacy = real_shapeshift_target(src_player, src_card, t_player, t_card, turn_into_player, turn_into_card, mode);

  if (in_play(t_player, t_card))
	verify_legend_rule(t_player, t_card, get_id(t_player, t_card));

  return legacy;
}

// Shapeshifts all cards matching test and controlled by t_player (or ANYBODY).
void shapeshift_all(int src_player, int src_card, int t_player, test_definition_t* test, int turn_into_player, int turn_into_card, int mode)
{
  int test_score = new_get_test_score(test);

  int num_affected = 0;
  char marked[2][151] = {{0}};
  int is_legend = is_legendary(turn_into_player, turn_into_card);

  int p, c, t;
  for (p = 0; p <= 1; ++p)
	if (p == t_player || t_player == ANYBODY)
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (in_play(p, c)
			&& !(test->not_me == 1 && c == src_card && p == src_player)
			&& new_make_test_in_play(p, c, test_score, test))
		  {
			marked[p][c] = 1;
			++num_affected;
		  }

  for (t = 0; t <= 1; ++t)
	{
	  p = t == 0 ? current_turn : 1-current_turn;
	  if (p == t_player || t_player == ANYBODY)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (marked[p][c] && in_play(p, c))
			real_shapeshift_target(src_player, src_card, p, c, turn_into_player, turn_into_card, mode);
	}

  if (num_affected > 0 && is_legend)
	for (p = 0; p <= 1; ++p)
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (marked[p][c] && in_play(p, c) && is_legendary(p, c))
		  {
			verify_legend_rule(p, c, get_id(p, c));
			goto break2;	// Only need to call once, since they'll all be the same id
		  }
 break2:;
}

void land_animation(int player, int card){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[13].player == instance->targets[13].card ){
		int id = instance->targets[13].player;
		id++;
		instance->targets[12].card = get_internal_card_id_from_csv_id(id);
		instance->targets[13].card = id;
	}
	else{
		int id = instance->targets[13].player;
		instance->targets[12].card = get_internal_card_id_from_csv_id(id);
		instance->targets[13].card = id;
	}
}

int manland_shared(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_ACTIVATE ){
		int result = 0;
		set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, colorless, black, blue, green, red, white, 0, 0, 0) ){
			if( get_id(player, card) != CARD_ID_NANTUKO_MONASTERY || has_threshold(player) ){
				result =  1;
			}
		}
		remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		if( result ){
			return 1;
		}
		return mana_producer(player, card, event);
	}
	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int choice = 0;
		if( mana_producer(player, card, EVENT_CAN_ACTIVATE) ){
			set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
			if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_TYPE_CHANGE, colorless, black, blue, green, red, white, 0, 0, 0) ){
				if( get_id(player, card) != CARD_ID_NANTUKO_MONASTERY || has_threshold(player) ){
					choice = do_dialog(player, player, card, -1, -1, " Generate Mana\n Animate\n Cancel", 1);
				}
			}
			remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		}
		else{
			choice = 1;
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				set_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
				if( generic_activated_ability(player, card, event, GAA_TYPE_CHANGE, colorless, black, blue, green, red, white, 0, 0, 0) ){
					instance->info_slot = 66;
				}
				remove_special_flags(player, card, SF_MANA_PRODUCER_DISABLED_FOR_AI);
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			if (instance->info_slot == 66 && instance->targets[13].player == instance->targets[13].card){
				add_status(player, instance->parent_card, STATUS_ANIMATED);
				true_transform(player, instance->parent_card);
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

int manland_animated(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	card_instance_t *instance = get_card_instance(player, card);
	if( get_id(player, card) != CARD_ID_STALKING_STONES_ANIMATED && eot_trigger(player, card, event ) ){
		remove_status(player, card, STATUS_ANIMATED);
		remove_special_flags(player, card, SF_TYPE_ALREADY_CHANGED);
		true_transform(player, card);
	}
	else if( event == EVENT_SET_COLOR && affect_me(player, card) ){
			event_result = cards_data[ instance->internal_card_id ].color ;
	}
	return manland_shared(player, card, event, colorless, black, blue, green, red, white);
}

int manland_normal(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	double_faced_card(player, card, event);
	return manland_shared(player, card, event, colorless, black, blue, green, red, white);
}

int cannot_be_countered(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->state |= STATE_CANNOT_TARGET;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			instance->state &= ~STATE_CANNOT_TARGET;
	}
	return 0;
}

int is_colorless(int player, int card){
	if( get_color(player, card) < 2 ){
		return 1;
	}
	return 0;
}

int is_target(int player, int card, int controller_of_effect)
{
  int i, j;
  for (i = 0; i < stack_size; ++i)
	if (stack_cards[i].player == controller_of_effect || controller_of_effect == 2)
	  {
		card_instance_t* instance = get_card_instance(stack_cards[i].player, stack_cards[i].card);
		for (j = 0; j < instance->number_of_targets; ++j)
		  if (instance->targets[j].player == player && instance->targets[j].card == card)
			return 1;
	  }
  return 0;
}

int get_cost_for_propaganda_like_effect(int player, int card){
	int csvid = get_id(player, card);
	int tax = 0, annex_count = 0;
	switch( csvid ){
			case CARD_ID_SPHERE_OF_SAFETY:
				tax = count_subtype(player, TYPE_ENCHANTMENT, -1);
				break;

			case CARD_ID_PROPAGANDA:
			case CARD_ID_ELEPHANT_GRASS:
			case CARD_ID_GHOSTLY_PRISON:
			case CARD_ID_WINDBORN_MUSE:
			case CARD_ID_KOSKUN_FALLS:
				tax = 2;
				break;

			case CARD_ID_ARCHANGEL_OF_TITHES:
			{
				if( ! is_tapped(player, card) ){
					tax = 1;
				}
				break;
			}

			case CARD_ID_COLLECTIVE_RESTRAINT:
				tax = count_domain(player, card);
				break;

			case CARD_ID_NORNS_ANNEX:
				annex_count = 1;
				break;

			case CARD_ID_WAR_TAX:
			{
					card_instance_t *this = get_card_instance(player, card);
					if( this->targets[1].player > 0 ){
						tax = this->targets[1].player;
					}
					break;
			}
			default:
				break;
	}

	int result = 0;
	SET_BYTE0(result)+=tax;
	SET_BYTE1(result)+=annex_count;
	return result;
}

void tax_attack(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_PAY_TO_ATTACK && current_phase == PHASE_DECLARE_ATTACKERS && current_turn == 1-player &&
		forbid_attack == 0 && ! is_humiliated(player, card)
	  ){
		if( affect_me( trigger_cause_controller, trigger_cause ) && reason_for_trigger_controller == affected_card_controller ){
			int result = get_cost_for_propaganda_like_effect(player, card);
			int tax = BYTE0(result);
			int annex_count = BYTE1(result);

			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER){
					if( annex_count == 0 ){
						if( has_mana(1-player, COLOR_ANY, tax) ){
							int choice = do_dialog(1-player, player, card, -1, -1, " Pay to attack\n Don't Pay\n", 0);
							if( choice == 0 ){
								charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, 1-player, COLOR_COLORLESS, tax);
								if( spell_fizzled != 1 ){
									return;
								}
							}
						}
					}
					else{
						if( has_phyrexian_mana(1-player, MANACOST_XW(tax, annex_count), 0)){
							int choice = do_dialog(1-player, player, card, -1, -1, " Pay to attack\n Don't Pay\n", 0);
							if( choice == 0 ){
								if( charge_phyrexian_mana(1-player, card, event, MANACOST_XW(tax, annex_count), 0) ){
									return;
								}
							}
						}
					}
					forbid_attack = 1;
			}
		}
	}
}

/* This doesn't actually skip the step, it just reduces number of
   draws by 99.  Should be good enough 99& of the time. */
int skip_your_draw_step(int player, event_t event){
	if(event == EVENT_DRAW_PHASE && player==current_turn){
		event_result-=99;
	}
	return 1;
}

int get_colors_to_sleight_from_text(int csvid)
{
	char *txt = cards_ptr[csvid]->rules_text;
	int colors_to_sleight = 0;
	if (txt){
		while ((txt = strstr(txt, "|S")))
		{
			txt += 2;
			int c;
			for (c = COLOR_BLACK; c <= COLOR_WHITE; ++c)
			{
				int i;
				for (i = 0; i < num_words_to_sleight; ++i){
					int l = strlen(strs_words_to_sleight[c][i]);
					if (!strncmp(txt, strs_words_to_sleight[c][i], l))
					{
						txt += l;
						colors_to_sleight |= 1 << c;
						goto outer_continue;
					}
				}
			}
			outer_continue:;
		}
	}
	return colors_to_sleight;
}


static int get_colors_to_hack_from_rules_text(int csvid)
{
	char *txt = cards_ptr[csvid]->rules_text;
	int colors_to_sleight = 0;
	if (txt){
		while ((txt = strstr(txt, "|H")))
		{
			txt += 2;
			int c;
			for (c = COLOR_BLACK; c <= COLOR_WHITE; ++c)
			{
				int i;
				for (i = 0; i < num_words_to_hack; ++i){
					int l = strlen(strs_words_to_hack[c][i]);
					if (!strncmp(txt, strs_words_to_hack[c][i], l))
					{
						txt += l;
						colors_to_sleight |= 1 << c;
						goto outer_continue;
					}
				}
			}
			outer_continue:;
		}
	}
	return colors_to_sleight;
}

static int get_colors_to_hack_from_type_text(int csvid)
{
	const char *txt = cards_ptr[csvid]->type_text;
	int colors_to_sleight = 0;
	if (txt){
		while ((txt = strstr(txt, "|H")))
		{
			txt += 2;
			int c;
			for (c = COLOR_BLACK; c <= COLOR_WHITE; ++c)
			{
				int i;
				for (i = 0; i < num_words_to_hack; ++i){
					int l = strlen(strs_words_to_hack[c][i]);
					if (!strncmp(txt, strs_words_to_hack[c][i], l))
					{
						txt += l;
						colors_to_sleight |= 1 << c;
						goto outer_continue;
					}
				}
			}
			outer_continue:;
		}
	}
	return colors_to_sleight;
}

int get_colors_to_hack_from_csvid(int csvid){
	int result = get_colors_to_hack_from_type_text(csvid);
	result |= get_colors_to_hack_from_rules_text(csvid);
	return result;
}


int get_hacked_color(int player, int card, int color)
{
	// 0x004A65A0
  card_instance_t *v3; // esi
  int result; // eax

  v3 = get_card_instance(player, card);
  result = v3->hack_mode[color];
  if (!v3->hack_mode[color])
    result = color;
  return result;
}

int get_sleighted_color(int player, int card, int color)
{
	// 004A65D0
  card_instance_t *instance; // esi
  color_t result; // eax

  instance = get_card_instance(player, card);
  result = instance->color_id[color];
  if (!instance->color_id[color])
    result = color;
  return result;
}

void replace_all_instances_of_one_color_word_with_another(int player, int card){
	int options[6] = {0, 1, 1, 1, 1, 1};
	int i;
	if( player == AI ){
		for(i=COLOR_BLACK; i<=COLOR_WHITE; i++){
			options[i] = 0;
		}
		int colors_to_sleight = cards_ptr[get_id(player, card)]->sleight_color;
		if( ! check_special_flags3(player, card, SF3_HARDCODED_SLEIGHT_WORDS_REPLACED) ){
			set_special_flags3(player, card, SF3_HARDCODED_SLEIGHT_WORDS_REPLACED);
			cards_ptr[get_id(player, card)]->sleight_color = colors_to_sleight = get_colors_to_sleight_from_text(get_id(player, card));
		}
		for(i=COLOR_BLACK; i<=COLOR_WHITE; i++){
			if( colors_to_sleight & (1<<i) ){
				if( ! get_card_instance(player, card)->color_id[i] ){
					options[i] = 1;
				}
			}
		}
		for(i=COLOR_BLACK; i<=COLOR_WHITE; i++){
			if( get_card_instance(player, card)->color_id[i] ){
				options[i] = 1;
			}
		}
	}
	int orig_color = DIALOG(player, card, EVENT_RESOLVE_SPELL, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_OMIT_ILLEGAL,
							"From Black...", 	options[1], 1,
							"From Blue...", 	options[2], 1,
							"From Green...", 	options[3], 1,
							"From Red...", 		options[4], 1,
							"From White...", 	options[5], 1);

	// 10/4/2004: It can't change a word to the same word. It must be a different word.
	for(i=COLOR_BLACK; i<=COLOR_WHITE; i++){
		options[i] = 0;
	}
	options[orig_color] = 1;

	int s_color = DIALOG(player, card, EVENT_RESOLVE_SPELL, DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_OMIT_ILLEGAL,
						"...to Black", 	!options[1], ((orig_color == COLOR_WHITE || orig_color == COLOR_GREEN) ? 10 : 1),
						"...to Blue", 	!options[2], ((orig_color == COLOR_RED || orig_color == COLOR_GREEN) ? 10 : 1),
						"...to Green", 	!options[3], ((orig_color == COLOR_BLACK || orig_color == COLOR_BLUE) ? 10 : 1),
						"...to Red", 	!options[4], ((orig_color == COLOR_BLUE || orig_color == COLOR_WHITE) ? 10 : 1),
						"...to White", 	!options[5], ((orig_color == COLOR_RED || orig_color == COLOR_BLACK) ? 10 : 1));

	get_card_instance(player, card)->color_id[orig_color] = s_color;
}

void replace_all_instances_of_one_basic_land_type_word_with_another(int player, int card){

	int options[6] = {0, 1, 1, 1, 1, 1};
	int i;
	if( player == AI ){
		for(i=COLOR_BLACK; i<=COLOR_WHITE; i++){
			options[i] = 0;
		}
		int colors_to_hack = cards_ptr[get_id(player, card)]->hack_colors;
		if( ! check_special_flags3(player, card, SF3_HARDCODED_HACK_WORDS_REPLACED) ){
			set_special_flags3(player, card, SF3_HARDCODED_HACK_WORDS_REPLACED);
			cards_ptr[get_id(player, card)]->hack_colors = colors_to_hack = get_colors_to_hack_from_csvid(get_id(player, card));
		}
		for(i=COLOR_BLACK; i<=COLOR_WHITE; i++){
			if( colors_to_hack & (1<<i) ){
				if( ! get_card_instance(player, card)->hack_mode[i] ){
					options[i] = 1;
				}
			}
		}
		for(i=COLOR_BLACK; i<=COLOR_WHITE; i++){
			if( get_card_instance(player, card)->hack_mode[i] ){
				options[i] = 1;
			}
		}
	}
	int orig_color = DIALOG(player, card, EVENT_RESOLVE_SPELL, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_OMIT_ILLEGAL,
							"From Swamp...", 	options[1], 1,
							"From Island...", 	options[2], 1,
							"From Forest...", 	options[3], 1,
							"From Mountain...", options[4], 1,
							"From Plain...", 	options[5], 1);

	// 10/4/2004: It can't change a word to the same word. It must be a different word.
	for(i=COLOR_BLACK; i<=COLOR_WHITE; i++){
		options[i] = 0;
	}
	options[orig_color] = 1;

	int s_color = DIALOG(player, card, EVENT_RESOLVE_SPELL, DLG_NO_STORAGE, DLG_RANDOM, DLG_NO_CANCEL, DLG_OMIT_ILLEGAL,
						"...to Swamp", 		!options[1], ((orig_color == COLOR_WHITE || orig_color == COLOR_GREEN) ? 10 : 1),
						"...to Island", 	!options[2], ((orig_color == COLOR_RED || orig_color == COLOR_GREEN) ? 10 : 1),
						"...to Forest", 	!options[3], ((orig_color == COLOR_BLACK || orig_color == COLOR_BLUE) ? 10 : 1),
						"...to Mountain", 	!options[4], ((orig_color == COLOR_BLUE || orig_color == COLOR_WHITE) ? 10 : 1),
						"...to Plain", 		!options[5], ((orig_color == COLOR_RED || orig_color == COLOR_BLACK) ? 10 : 1));

	get_card_instance(player, card)->hack_mode[orig_color] = s_color;
}

// fmt should have exactly one %s in it to receive the color text.  Returns a pointer to a static buffer.
const char* get_sleighted_color_text(int player, int card, const char* fmt, int orig_color_t){
	if (ai_is_speculating == 1){
		return "";
	}

	static char buf[500];

	static const char* words[][2] = {
		{ "colorless",	"COLORLESS" },
		{ "black",		"BLACK" },
		{ "blue",		"BLUE" },
		{ "green",		"GREEN" },
		{ "red",		"RED" },
		{ "white",		"WHITE" },
		{ "artifact",	"ARTIFACT" },
		{ "any",		"ANY" }
	};

	int sleighted = get_sleighted_color(player, card, orig_color_t);

	sprintf(buf, fmt, words[sleighted][sleighted == orig_color_t ? 0 : 1]);

	return buf;
}

// As get_sleighted_color_text(), but fmt should have exactly two %s in it.
const char* get_sleighted_color_text2(int player, int card, const char* fmt, int orig_color_t_1, int orig_color_t_2){
	if (ai_is_speculating == 1){
		return "";
	}

	static char buf[500];

	static const char* words[][2] = {
		{ "colorless",	"COLORLESS" },
		{ "black",		"BLACK" },
		{ "blue",		"BLUE" },
		{ "green",		"GREEN" },
		{ "red",		"RED" },
		{ "white",		"WHITE" },
		{ "artifact",	"ARTIFACT" },
		{ "any",		"ANY" }
	};

	int sleighted1 = get_sleighted_color(player, card, orig_color_t_1);
	int sleighted2 = get_sleighted_color(player, card, orig_color_t_2);

	sprintf(buf, fmt, words[sleighted1][sleighted1 == orig_color_t_1 ? 0 : 1], words[sleighted2][sleighted2 == orig_color_t_2 ? 0 : 1]);

	return buf;
}

int get_sleighted_color_test(int player, int card, int orig_color_test){
	if (card < 0 || player < 0){
		return orig_color_test;
	}
	switch (orig_color_test){
		// The common cases - exactly one color and nothing else
		case COLOR_TEST_BLACK:	orig_color_test = COLOR_BLACK;	break;
		case COLOR_TEST_BLUE:	orig_color_test = COLOR_BLUE;	break;
		case COLOR_TEST_GREEN:	orig_color_test = COLOR_GREEN;	break;
		case COLOR_TEST_RED:	orig_color_test = COLOR_RED;	break;
		case COLOR_TEST_WHITE:	orig_color_test = COLOR_WHITE;	break;
		default:{
			if (orig_color_test & COLOR_TEST_ANY_COLORED){
				// The general case - multiple colors, no colors, one or more colors plus other "colors" like COLOR_TEST_ARTIFACT or COLOR_TEST_COLORLESS, etc.
				uint32_t col, new_color_test = 0;
				for (col = COLOR_BLACK; col <= COLOR_WHITE; ++col){
					if (orig_color_test & (COLOR_TEST_BLACK << (col - 1))){
						new_color_test |= COLOR_TEST_BLACK << (get_sleighted_color(player, card, col) - 1);
					}
				}
				return (orig_color_test & ~COLOR_TEST_ANY_COLORED) | new_color_test;
			}
			return orig_color_test;
		}
	}
	return COLOR_TEST_BLACK << (get_sleighted_color(player, card, orig_color_test) - 1);
}

int get_sleighted_protection(int player, int card, int orig_prot){
	switch (orig_prot){
		// The common cases - exactly one color and nothing else
		case KEYWORD_PROT_BLACK:	orig_prot = COLOR_BLACK;	break;
		case KEYWORD_PROT_BLUE:		orig_prot = COLOR_BLUE;		break;
		case KEYWORD_PROT_GREEN:	orig_prot = COLOR_GREEN;	break;
		case KEYWORD_PROT_RED:		orig_prot = COLOR_RED;		break;
		case KEYWORD_PROT_WHITE:	orig_prot = COLOR_WHITE;	break;
		default:{
			if (orig_prot & KEYWORD_PROT_COLORED){
				// The general case - multiple protections, no protections, one or more protections plus other keywords, etc.
				uint32_t col, new_prot = 0;
				for (col = COLOR_BLACK; col <= COLOR_WHITE; ++col){
					if (orig_prot & (KEYWORD_PROT_BLACK << (col - 1))){
						new_prot |= KEYWORD_PROT_BLACK << (get_sleighted_color(player, card, col) - 1);
					}
				}
				return (orig_prot & ~KEYWORD_PROT_COLORED) | new_prot;
			}
			return orig_prot;
		}
	}
	return KEYWORD_PROT_BLACK << (get_sleighted_color(player, card, orig_prot) - 1);
}

/* fmt should have exactly one %s, %a, or %l in it to receive the land text for each subtype provided as a variable argument.  (%a will insert a "a " or "an "
 * before the land text as approriate; %l will make unhacked land text all lowercase, appropriate for e.g. a landwalk ability rather than a type name.)  Returns
 * a pointer to a static buffer.  Each variable argument can be a SUBTYPE_*, HARDCODED_SUBTYPE_*, or COLOR_*. */
const char* get_hacked_land_text(int player, int card, const char* fmt, ...){
	static char buf[500];
	if (ai_is_speculating == 1){
		*buf = 0;
		return buf;
	}

	static const char* words[][3] = {
		{ "land",		"Land",		"LAND" },
		{ "swamp",		"Swamp",	"SWAMP" },
		{ "island",		"Island",	"ISLAND" },
		{ "forest",		"Forest",	"FOREST" },
		{ "mountain",	"Mountain",	"MOUNTAIN" },
		{ "plains",		"Plains",	"PLAINS" },
		{ "land",		"Land",		"LAND" },
		{ "land",		"Land",		"LAND" },
	};

	va_list args;
	va_start(args, fmt);

	const char *p = fmt;
	char *q = buf;
	for (; (*q = *p); ++p, ++q){
		if (*p == '%' && (*(p + 1) == 's' || *(p + 1) == 'a' || *(p + 1) == 'l')){

			int hacked, orig_color_t, orig_subtype = va_arg(args, int);

			switch (orig_subtype){
				case HARDCODED_SUBTYPE_SWAMP:	case SUBTYPE_SWAMP:		case COLOR_BLACK:	orig_color_t = COLOR_BLACK;	break;
				case HARDCODED_SUBTYPE_ISLAND:	case SUBTYPE_ISLAND:	case COLOR_BLUE:	orig_color_t = COLOR_BLUE;	break;
				case HARDCODED_SUBTYPE_FOREST:	case SUBTYPE_FOREST:	case COLOR_GREEN:	orig_color_t = COLOR_GREEN;	break;
				case HARDCODED_SUBTYPE_MOUNTAIN:case SUBTYPE_MOUNTAIN:	case COLOR_RED:		orig_color_t = COLOR_RED;	break;
				case HARDCODED_SUBTYPE_PLAINS:	case SUBTYPE_PLAINS:	case COLOR_WHITE:	orig_color_t = COLOR_WHITE;	break;
				default:
					hacked = orig_color_t = 0;
					goto skip_hack;
			}
			hacked = (player >= 0 && card >= 0) ? get_hacked_color(player, card, orig_color_t) : orig_color_t;

		skip_hack:
			++p;
			int article = *p == 'a';
			int lowercase = *p == 'l';

			if (article){
				*q++ = 'a';
				if (hacked == COLOR_BLUE){
					*q++ = 'n';
				}
				*q++ = ' ';
			}
			if (hacked != orig_color_t){
				q += sprintf(q, "%s", words[hacked][2]) - 1;
			} else if (lowercase){
				q += sprintf(q, "%s", words[hacked][0]) - 1;
			} else {
				q += sprintf(q, "%s", words[hacked][1]) - 1;
			}
		}
	}

	va_end(args);

	return buf;
}

int get_hacked_walk(int player, int card, int orig_walk){
	switch (orig_walk){
		// The common cases - exactly one landwalk ability and nothing else
		case KEYWORD_SWAMPWALK:		orig_walk = COLOR_BLACK;	break;
		case KEYWORD_ISLANDWALK:	orig_walk = COLOR_BLUE;		break;
		case KEYWORD_FORESTWALK:	orig_walk = COLOR_GREEN;	break;
		case KEYWORD_MOUNTAINWALK:	orig_walk = COLOR_RED;		break;
		case KEYWORD_PLAINSWALK:	orig_walk = COLOR_WHITE;	break;
		default:{
			if (orig_walk & KEYWORD_BASIC_LANDWALK){
				// The general case - multiple landwalk abilities, no landwalk abilities, one or more landwalk abilities plus other keywords, etc.
				uint32_t col, new_walk = 0;
				for (col = COLOR_BLACK; col <= COLOR_WHITE; ++col){
					if (orig_walk & (KEYWORD_SWAMPWALK << (col - 1))){
						new_walk |= KEYWORD_SWAMPWALK << (get_hacked_color(player, card, col) - 1);
					}
				}
				return (orig_walk & ~KEYWORD_BASIC_LANDWALK) | new_walk;
			}
			return orig_walk;
		}
	}
	return KEYWORD_SWAMPWALK << (get_hacked_color(player, card, orig_walk) - 1);
}

int get_hacked_subtype(int player, int card, int orig_subtype){
	if (card < 0 || player < 0){
		return orig_subtype;
	}

	static int subtypes[][4] = {
		{ 0, 0, 0, 0 },
		{ SUBTYPE_SWAMP,	HARDCODED_SUBTYPE_SWAMP,	COLOR_BLACK,	CARD_ID_SWAMP },
		{ SUBTYPE_ISLAND,	HARDCODED_SUBTYPE_ISLAND,	COLOR_BLUE,		CARD_ID_ISLAND },
		{ SUBTYPE_FOREST,	HARDCODED_SUBTYPE_FOREST,	COLOR_GREEN,	CARD_ID_FOREST },
		{ SUBTYPE_MOUNTAIN,	HARDCODED_SUBTYPE_MOUNTAIN,	COLOR_RED,		CARD_ID_MOUNTAIN },
		{ SUBTYPE_PLAINS,	HARDCODED_SUBTYPE_PLAINS,	COLOR_WHITE,	CARD_ID_PLAINS }
	};

	int column, orig_color_t;

	switch (orig_subtype){
		case SUBTYPE_SWAMP:				column = 0;	orig_color_t = COLOR_BLACK;	break;
		case HARDCODED_SUBTYPE_SWAMP:	column = 1;	orig_color_t = COLOR_BLACK;	break;
		case COLOR_BLACK:				column = 2;	orig_color_t = COLOR_BLACK;	break;
		case CARD_ID_SWAMP:				column = 3;	orig_color_t = COLOR_BLACK;	break;

		case SUBTYPE_ISLAND:			column = 0;	orig_color_t = COLOR_BLUE;	break;
		case HARDCODED_SUBTYPE_ISLAND:	column = 1;	orig_color_t = COLOR_BLUE;	break;
		case COLOR_BLUE:				column = 2;	orig_color_t = COLOR_BLUE;	break;
		case CARD_ID_ISLAND:			column = 3;	orig_color_t = COLOR_BLUE;	break;

		case SUBTYPE_FOREST:			column = 0;	orig_color_t = COLOR_GREEN;	break;
		case HARDCODED_SUBTYPE_FOREST:	column = 1;	orig_color_t = COLOR_GREEN;	break;
		case COLOR_GREEN:				column = 2;	orig_color_t = COLOR_GREEN;	break;
		case CARD_ID_FOREST:			column = 3;	orig_color_t = COLOR_GREEN;	break;

		case SUBTYPE_MOUNTAIN:			column = 0;	orig_color_t = COLOR_RED;	break;
		case HARDCODED_SUBTYPE_MOUNTAIN:column = 1;	orig_color_t = COLOR_RED;	break;
		case COLOR_RED:					column = 2;	orig_color_t = COLOR_RED;	break;
		case CARD_ID_MOUNTAIN:			column = 3;	orig_color_t = COLOR_RED;	break;

		case SUBTYPE_PLAINS:			column = 0;	orig_color_t = COLOR_WHITE;	break;
		case HARDCODED_SUBTYPE_PLAINS:	column = 1;	orig_color_t = COLOR_WHITE;	break;
		case COLOR_WHITE:				column = 2;	orig_color_t = COLOR_WHITE;	break;
		case CARD_ID_PLAINS:			column = 3;	orig_color_t = COLOR_WHITE;	break;

		default:
			return orig_subtype;
	}

	return subtypes[get_hacked_color(player, card, orig_color_t)][column];
}

void rampage(int player, int card, event_t event, int number)
{
  // 0x415cb0
  if (event == EVENT_DECLARE_BLOCKERS)
	{
	  int blockers = count_my_blockers(player, card);
	  if (blockers > 1)
		{
		  number *= (blockers - 1);
		  pump_until_eot(player, card, player, card, number, number);
		}
	}
}

// Just like get_type(), but the iid is already known (and possibly may not be easily determinable from just player/card, for instance if it's left play)
type_t get_type_with_iid(int player, int card, int iid){
	type_t typ = cards_data[iid].type;
	if ((typ & (TYPE_INSTANT | TYPE_INTERRUPT)) && (typ & (TYPE_PERMANENT | TYPE_SORCERY))){
		typ &= ~(TYPE_INSTANT | TYPE_INTERRUPT);	// Permanents with flash and sorceries that can be cast at instant speed are instants only as an implementation detail
	}
	if (typ & TYPE_INTERRUPT){
		typ |= TYPE_INSTANT;	// Interrupts are considered instants (but not vice-versa)
	}
	if (is_planeswalker(player, card)){
		typ &= ~TYPE_ENCHANTMENT;
		typ |= TARGET_TYPE_PLANESWALKER;
	}
	if (player != -1 && (typ & (TYPE_PERMANENT | TARGET_TYPE_PLANESWALKER))){
		int flags2 = check_special_flags2(player, card, SF2_ENCHANTED_EVENING | SF2_MYCOSYNTH_LATTICE);
		if (flags2 & SF2_ENCHANTED_EVENING){
			typ |= TYPE_ENCHANTMENT;
		}
		if (flags2 & SF2_MYCOSYNTH_LATTICE){
			typ |= TYPE_ARTIFACT;
		}
	}
	return typ;
}

type_t get_type(int player, int card)
{
  int iid;
  if (player < 0)
	iid = card;
  else
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  iid = instance->internal_card_id;
	  if (iid < 0)
		iid = instance->backup_internal_card_id;
	}
  return get_type_with_iid(player, card, iid);
}

int is_what(int player, int card, int test_type){
	// Equivalent to (test_type & get_type(player, card)) ? 1 : 0, but avoids unnecessary calls to check_special_flags() and is_planeswalker().
	int typ;
	if( player == -1 ){
		typ = cards_data[card].type;
	}
	else{
		typ = get_card_data(player, card)->type;
	}

	if ((test_type & TARGET_TYPE_PLANESWALKER) && is_planeswalker(player, card)){
		return 1;
	}

	if (test_type & TYPE_ENCHANTMENT){
		if (player != -1 && (typ & TYPE_PERMANENT) && check_special_flags2(player, card, SF2_ENCHANTED_EVENING)){
			typ |= TYPE_ENCHANTMENT;
		}
		else if ((test_type & TYPE_PERMANENT) != TYPE_PERMANENT && is_planeswalker(player, card)){
				return 0;
		}
	}

	if (test_type & TYPE_ARTIFACT){
		if (player != -1 && (typ & TYPE_PERMANENT) && check_special_flags2(player, card, SF2_MYCOSYNTH_LATTICE)){
			typ |= TYPE_ARTIFACT;
		}
	}

	if (typ & TYPE_INTERRUPT){
		typ |= TYPE_INSTANT;	// Consider interrupts as instants (but don't consider instants as interrupts, so TYPE_INTERRUPT can still be tested for)
	}

	if ((typ & (TYPE_INSTANT | TYPE_INTERRUPT)) && (typ & (TYPE_PERMANENT | TYPE_SORCERY))){
		typ &= ~(TYPE_INSTANT | TYPE_INTERRUPT);	// Permanents with flash and sorceries that can be cast at instant speed are instants only as an implementation detail
	}

	return typ & test_type ? 1 : 0;
}

int color_to_color_test(int color){

  return (1 << color);
}

int keyword_to_color(int keyword){

	int i;
	for(i=0; i<5; i++){
		if( (1 << (11+i)) == keyword ){ // Get color from keyword
			return i+1;
		}
	}
	return 0;
}

int select_a_color(int player){

  int ai_choice = 0;

  if( player == AI ){
	 ai_choice = get_deck_color(player, 1-player);
  }

  int color = choose_a_color(player, ai_choice);
  int keyword = color_to_color_test(color);

  return keyword;
}

static int actually_has_a_card_in_hand(int player){
	int c;
	for (c = 0; c < active_cards_count[player]; ++c){
		if (in_hand(player, c)){
			return 1;
		}
	}
	return 0;
}

int count_permanents_by_color(int player, int selected_type, int selected_color){
	if (player == ANYBODY){
		return (count_permanents_by_color(0, selected_type, selected_color)
				+ count_permanents_by_color(1, selected_type, selected_color));
	}

	int count, result = 0;

	for (count = 0; count < active_cards_count[player]; ++count){
		if (is_what(player, count, selected_type)
			&& in_play(player, count)
			&& (get_color(player, count) & selected_color)){
			result++;
		}
	}

	return result;
}

int is_token(int player, int card){

	card_instance_t *affected = get_card_instance(player, card);

	if( (affected->token_status & STATUS_TOKEN) && ! check_special_flags(player, card, SF_UNEARTH) ){
		return 1;
	}

	return 0;
}

int pick_card_from_graveyard(int player, int target_graveyard, const char * message){

	return show_deck( player, get_grave(target_graveyard), 500, message, 0, 0x7375B0 );
}

int changeling_switcher(int player, int card, event_t event){

   if( event == EVENT_ABILITIES && affect_me(player, card) ){
	   event_result |= KEYWORD_PROT_INTERRUPTS;
   }

   return 0;
}

int generic_creature_with_activated_tapsubtype_ability(int player, int card, event_t event, subtype_t subtype, int howmany, int illegalstate1, int illegalstate2){
  card_instance_t *instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = player;
  td.preferred_controller = player;
  td.required_subtype = subtype;
  td.illegal_abilities = 0;
  td.illegal_state = illegalstate1 | illegalstate2;
  td.allow_cancel = 0;

  target_definition_t td1;
  default_target_definition(player, card, &td1, TYPE_CREATURE);
  td1.allowed_controller = player;
  td1.preferred_controller = player;
  td1.illegal_abilities = 0;
  td1.allow_cancel = 0;
  td1.illegal_state = illegalstate1 | illegalstate2;
  td1.required_abilities = KEYWORD_PROT_INTERRUPTS;

  if( event == EVENT_CAN_ACTIVATE &&
	  (target_available(player, card, &td) + target_available(player, card, &td1) >= howmany)   ){
	return 1;
  }

  if( event == EVENT_ACTIVATE){
	 int stop = 0;
	 int test = 0;
	 while( stop != 1 ){
		   if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			  tap_card(instance->targets[0].player, instance->targets[0].card);
			  test++;
		   }
		   else if( can_target(&td1) && pick_target(&td1, "TARGET_CREATURE") ){
				   tap_card(instance->targets[0].player, instance->targets[0].card);
				   test++;

		   }

		   if( test == howmany ){
			  stop = 1;
		   }

	}

  }

  return 0;
 }

int specific_spell_played(int player, int card, event_t event, int t_player, int trigger_mode, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		!is_humiliated(player, card) && (t_player == ANYBODY || trigger_cause_controller == t_player) &&
		!is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) && ! is_token(trigger_cause_controller, trigger_cause) &&
		make_test_in_play(trigger_cause_controller, trigger_cause, -1, type, flag1, subtype, flag2, color, flag3, id, flag4, cc, flag5)
	  ){
		if(event == EVENT_TRIGGER){
			if (trigger_mode == RESOLVE_TRIGGER_DUH){
				trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
			}
			event_result |= trigger_mode;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			card_instance_t* instance = get_card_instance(player, card);
			instance->targets[1].player = trigger_cause_controller;
			instance->targets[1].card = trigger_cause;
			return 1;
		}
	}
	return 0;
}

int new_specific_spell_played(int player, int card, event_t event, int t_player, int trigger_mode, test_definition_t* this_test/*optional*/){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
		!is_humiliated(player, card) && (t_player == ANYBODY || trigger_cause_controller == t_player) &&
		!is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) && ! is_token(trigger_cause_controller, trigger_cause) &&
		(!this_test || new_make_test_in_play(trigger_cause_controller, trigger_cause, -1, this_test))
	  ){
		if(event == EVENT_TRIGGER){
			if (trigger_mode == RESOLVE_TRIGGER_DUH){
				trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
			}
			event_result |= trigger_mode;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			card_instance_t* instance = get_card_instance(player, card);
			instance->targets[1].player = trigger_cause_controller;
			instance->targets[1].card = trigger_cause;
			return 1;
		}
	}
	return 0;
}

int specific_cip(int player, int card, event_t event, int t_player, int trigger_mode, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5)
{
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, type, "");
	  test.type_flag = flag1;
	  test.subtype = subtype;
	  test.subtype_flag = flag2;
	  test.color = color;
	  test.color_flag = flag3;
	  test.id = id;
	  test.id_flag = flag4;
	  test.cmc = cc;
	  test.cmc_flag = flag5;
	  return new_specific_cip(player, card, event, t_player, trigger_mode, &test);
	}
  else
	return 0;
}

int new_specific_cip(int player, int card, event_t event, int t_player, int trigger_mode, test_definition_t *this_test){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == affected_card_controller && !is_humiliated(player, card)

	  ){
		if( t_player != ANYBODY && trigger_cause_controller != t_player ){
			return 0;
		}

		if( this_test->not_me == 1 && trigger_cause_controller == player && trigger_cause == card ){
			return 0;
		}

		if( !new_make_test_in_play(trigger_cause_controller, trigger_cause, -1, this_test)
			&& !(this_test->not_me == 2 && trigger_cause_controller == player && trigger_cause == card)){
			return 0;
		}

		if( check_for_cip_effects_removal(trigger_cause_controller, trigger_cause) ){
			return 0;
		}

		if(event == EVENT_TRIGGER){
			if (trigger_mode == RESOLVE_TRIGGER_DUH){
				trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
			}
			event_result |= trigger_mode;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				card_instance_t* instance = get_card_instance(player, card);
				instance->targets[1].player = trigger_cause_controller;
				instance->targets[1].card = trigger_cause;
				get_card_instance(trigger_cause_controller, trigger_cause)->regen_status |= KEYWORD_RECALC_POWER|KEYWORD_RECALC_TOUGHNESS;
				return 1;
		}
	}
	return 0;
}

int metamorphosis(int player, int card, type_t selected_type, kill_t kill_code){

	kill_card(player, card, kill_code);

	int *deck = deck_ptr[player];
	int z;
	int card_added = -1;
	for(z=0; z < count_deck(player); z++){
		if (is_what(-1, deck[z], selected_type)){
			show_deck(HUMAN, deck, z+1, "These cards were revealed", 0, 0x7375B0 );
			if( ! check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE) ){
			   card_added = add_card_to_hand(player, deck[z]);
			   remove_card_from_deck(player, z);
			   put_into_play(player, card_added);
			}
			break;
		}
	}
	shuffle(player);

	return card_added;
}

int impulse_effect(int player, int amount, int whattodo){
	//  whattodo = 0 --> Impulse-like effect (add the selected card to hand and put the rest on bottom)
	//  whattodo = 1 --> Impulse-like effect, but put the selected card on the top of library
	//  whattodo = 2 --> add the selected card to hand and leave the rest on the top.

	int *deck = deck_ptr[player];
	int card_added = -1;
	if( count_deck(player) < amount ){
		amount = count_deck(player);
	}
	if (amount <= 0){
		return 0;
	}
	if( deck[0] != -1 ){
		char msg[100] = "Select a card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		this_test.create_minideck = amount;
		this_test.no_shuffle = 1;

		int selected = new_select_a_card(player, player, TUTOR_FROM_DECK, 1, AI_MAX_VALUE, -1, &this_test);
		card_added = add_card_to_hand(player, deck[selected]);
		remove_card_from_deck(player, selected);
		amount--;

		if( amount > 0 && whattodo != 2){
			put_top_x_on_bottom(player, player, amount);
		}

		if( whattodo == 1){
			put_on_top_of_deck(player, card_added);
		}
	}

	return card_added;
}

int count_free_mana(int player, int type){

	int count = active_cards_count[player]-1;
	int free_mana = 0;

	while( count > -1 ){
		  if( in_play(player, count) ){
			  if( get_id(player, count) == CARD_ID_SEMBLANCE_ANVIL ){
				  card_instance_t *leg = get_card_instance(player, count);
				  if( leg->info_slot & type ){
					  free_mana+=2;
				  }
			  }
			  if( get_id(player, count) == CARD_ID_HELM_OF_AWAKENING ){
				  free_mana+=1;
			  }
		  }
		  count--;
	}

  return free_mana;
}

int is_attacking(int player, int card){
  card_instance_t *instance = get_card_instance(player, card);
  if( instance->state & STATE_ATTACKING ){
	 return 1;
  }

  return 0;
}

card_instance_t* in_play_and_attacking(int player, int card)
{
  card_instance_t* instance = in_play(player, card);
  return instance && (instance->state & STATE_ATTACKING) ? instance : NULL;
}

int count_attackers(int player)
{
  int c, result = 0;
  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_play_and_attacking(player, c))
	  ++result;

  return result;
}

int count_attackers_non_planeswalker(int player)
{
  int c, result = 0;
  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_play_and_attacking(player, c) && !check_special_flags(player, c, SF_ATTACKING_PWALKER))
	  ++result;

  return result;
}

/* If player == whose_deck or player is an AI (actual AI, or AI speculating for a human player), then returns a color_t of the most common color in whose_deck's
 * library.  If player is a human and whose_deck isn't his, then always returns COLOR_BLACK.  If player is -1, then returns a color_test_t of the two most
 * common colors in whose_deck's library. */
int get_deck_color(int player, int whose_deck){
	if (player >= 0 && !IS_AI(player) && player != whose_deck){
		return COLOR_BLACK;	// don't give away the AI's colors
	}
	int *deck = deck_ptr[whose_deck];
	int count = count_deck(whose_deck)-1;
	int clrs[5] = {0, 0, 0, 0, 0};
	while( count > -1 ){
			int color = cards_data[ deck[count] ].color;
			if( (color & COLOR_TEST_ANY_COLORED) && ! is_what(-1, deck[count], TYPE_LAND) ){
				int i;
				for (i = COLOR_BLACK; i <= COLOR_WHITE; i++){
					if( color & (1 << i) ){
						clrs[i-1]++;
					}
				}
			}
			count--;
	}
	if( player != -1 ){
		int ai_choice = 1;
		int i;
		int par = -1;
		for (i = COLOR_BLACK; i <= COLOR_WHITE; i++){
			if( clrs[i - 1] > par ){
				par = clrs[i - 1];
				ai_choice = i;
			}
		}
		return ai_choice;
	}
	else{
		int clrs_res[2] = {0, 0};
		int par = -1;
		int i;
		for (i = COLOR_BLACK; i <= COLOR_WHITE; i++){
			if( clrs[i - 1] > par ){
				par = clrs[i - 1];
				clrs_res[0] = i;
			}
		}
		par = -1;
		for (i = COLOR_BLACK; i <= COLOR_WHITE; i++){
			if( clrs[i - 1] > par && i != clrs_res[0] ){
				par = clrs[i - 1];
				clrs_res[1] = i;
			}
		}
		return (1 << clrs_res[0]) | (1 << clrs_res[1]);
	}
}

int select_a_protection(int player){

  int ai_choice = 0;

  if( player == AI ){
	 ai_choice = get_deck_color(player, 1-player);
  }

  int color = choose_a_color(player, ai_choice);
  int keyword = KEYWORD_PROT_BLACK;

  if( color == 2 ){
	 keyword = KEYWORD_PROT_BLUE;
  }
  else if( color == 3 ){
		  keyword = KEYWORD_PROT_GREEN;
  }
  else if( color == 4 ){
		  keyword = KEYWORD_PROT_RED;
  }
  else if( color == 5 ){
		  keyword = KEYWORD_PROT_WHITE;
  }

  return keyword;
}

int sacrifice_at_end_of_combat(int player, int card, event_t event)
{
	if (end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)){
		card_instance_t* instance = get_card_instance(player, card);
		if( ! check_state(instance->damage_target_player, instance->damage_target_card, STATE_OUBLIETTED) ){
			kill_card(instance->damage_target_player, instance->damage_target_card, KILL_SACRIFICE);
		}
		kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}

int remove_at_eot(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( eot_trigger(player, card, event) ){
		int p = instance->targets[0].player;
		int c = instance->targets[0].card;
		if( ! check_state(p, c, STATE_OUBLIETTED) ){
			kill_card(p, c, KILL_REMOVE);
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}
	return 0;
}

int haste_and_remove_eot(int player, int card, event_t event ){
	card_instance_t *instance = get_card_instance(player, card);
	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( p > -1 ){
		haste(p, c, event);
		if( eot_trigger(player, card, event) ){
			if( ! check_state(p, c, STATE_OUBLIETTED) ){
				kill_card(p, c, KILL_REMOVE);
			}
			else{
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}

	return 0;
}

int haste_and_sacrifice_eot(int player, int card, event_t event ){
	card_instance_t *instance = get_card_instance(player, card);
	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( p > -1 ){
		haste(p, c, event);
		if( check_state(p, c, STATE_OUBLIETTED) ){
			kill_card(player, card, KILL_REMOVE);
		}
		if( eot_trigger(player, card, event) ){
			kill_card(p, c, KILL_SACRIFICE);
		}
	}


	return 0;
}

int no_combat_damage_this_turn(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_prevented(event);

  if (damage)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (damage->damage_source_card == instance->targets[0].card && damage->damage_source_player == instance->targets[0].player)
		damage->info_slot = 0;
	}
  else if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

void negate_combat_damage_this_turn(int player, int card, int t_player, int t_card, int unused){
	create_targetted_legacy_effect(player, card, &no_combat_damage_this_turn, t_player, t_card);
}

int total_playable_lands(int player)
{
  int p, c, total = 1;
  card_instance_t* inst;

  for (p = 0; p < 2; p++)
	for (c = 0; c < active_cards_count[p]; ++c)
	  if (in_play(p, c))
		{
		  if (p != player)
			{
			  if (get_id(p, c) == CARD_ID_RITES_OF_FLOURISHING)
				++total;
			}
		  else
			switch (get_id(p, c))
			  {
				case CARD_ID_EXPLORATION:
				case CARD_ID_ORACLE_OF_MUL_DAYA:
				case CARD_ID_RITES_OF_FLOURISHING:
				case CARD_ID_MINA_AND_DENN_WILDBORN:
				case CARD_ID_THE_GITROG_MONSTER:
				  total++;
				  break;
				case CARD_ID_AZUSA_LOST_BUT_SEEKING:
				  total += 2;
				  break;
				case 903:
				  inst = get_card_instance(p, c);
				  if (inst->internal_card_id == LEGACY_EFFECT_CUSTOM && inst->info_slot == (int)check_playable_lands_legacy)
					switch (inst->display_pic_csv_id)
					  {
						case CARD_ID_EXPLORE:
						case CARD_ID_URBAN_EVOLUTION:
						case CARD_ID_KIORA_THE_CRASHING_WAVE:
						  total++;
						  break;
						case CARD_ID_JOURNEY_OF_DISCOVERY:
						  total += 2;
						  break;
						case CARD_ID_SUMMER_BLOOM:
						  total += 3;
						  break;
					  }
				  break;
			  }
		}

  if (total == 1){
	  player_bits[player] &= ~PB_COUNT_TOTAL_PLAYABLE_LANDS;
  }

  return total;
}

void check_playable_lands(int player)
{
	player_bits[player] |= PB_COUNT_TOTAL_PLAYABLE_LANDS;
}

int check_playable_lands_legacy(int player, int card, event_t event){

	player_bits[player] |= PB_COUNT_TOTAL_PLAYABLE_LANDS;

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int generic_painland(int player, int card, event_t event)
{
  /* |T: Add |1 to your mana pool.
   * |T: Add |C1 or |C2 to your mana pool. ~ deals 1 damage to you. */

  if (event == EVENT_CAN_ACTIVATE)
	{
	  if (IS_AI(player) && life[player] <= 1 && !(get_card_instance(player, card)->mana_color & COLOR_TEST_COLORLESS))
		return 0;
	}

  if (event == EVENT_ACTIVATE)
	{
	  color_test_t colors = get_card_instance(player, card)->mana_color;
	  if (IS_AI(player) && life[player] <= 1 && (colors & COLOR_TEST_COLORLESS))
		produce_mana_tapped_all_one_color(player, card, COLOR_TEST_COLORLESS, 1);
	  else
		{
		  produce_mana_tapped_all_one_color(player, card, colors, 1);
		  if (chosen_colors & ~COLOR_TEST_COLORLESS)
			damage_player(player, 1, player, card);
		}
	}
  else if (event == EVENT_COUNT_MANA && affect_me(player, card))
	{
	  if (CAN_TAP_FOR_MANA(player, card))
		{
		  if (check_special_flags2(player, card, SF2_CONTAMINATION)
			  && is_what(player, card, TYPE_LAND))
			declare_mana_available(player, COLOR_BLACK, 1);
		  else
			{
			  color_test_t colors = get_card_instance(player, card)->mana_color;
			  if (IS_AI(player) && life[player] <= 1)
				{
				  if (colors & COLOR_TEST_COLORLESS)
					declare_mana_available(player, COLOR_COLORLESS, 1);
				}
			  else
				declare_mana_available_hex(player, colors, 1);
			}
		}
	}
  else
	return mana_producer(player, card, event);

  return 0;
}

int effect_defender_can_attack_until_eot(int player, int card, event_t event){
	if (event == EVENT_ABILITIES){
		card_instance_t* instance = get_card_instance(player, card);
		if (instance->damage_target_player >= 0){
			get_card_instance(instance->damage_target_player, instance->damage_target_card)->token_status |= STATUS_WALL_CAN_ATTACK;
		}
	}
	if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

// Also suitable as part of an enchantment
int effect_cannot_attack(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES && in_play(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  cannot_attack(instance->damage_target_player, instance->damage_target_card, event);
	}

	return 0;
}

int effect_cannot_attack_until_eot(int player, int card, event_t event)
{
  effect_cannot_attack(player, card, event);

  if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int cannot_attack(int player, int card, event_t event){

	if (event == EVENT_ABILITIES && affect_me(player, card)
		&& player >= 0 && card >= 0){	// mainly for effect_cannot_attack() above, so it doesn't do anything if not attached
		card_instance_t *instance = get_card_instance(player, card);
		instance->token_status |= STATUS_CANT_ATTACK;
	}
	return 0;
}

void enters_the_battlefield_with_counters(int player, int card, event_t event, counter_t type, int number)
{
	if( (event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card) &&
		! check_special_flags2(player, card, SF2_FACE_DOWN_DUE_TO_MANIFEST)
	  ){
		++hack_silent_counters;
		add_counters(player, card, type, number);
		--hack_silent_counters;
	}
}

int count_nonbasic_lands(int player){

	int result = 0;
	int i;
	for(i=0; i<2; i++){
		if( player == 2 || i == player ){
			int count = 0;
			while( count < active_cards_count[i] ){
				   if( in_play(i, count) && is_what(i, count, TYPE_LAND) && ! is_basic_land(i, count) ){
					   result++;
				   }
				   count++;
			}
		}
	}

	return result;
}

int control_nonbasic_land(int player){

	int count = 0;
	while( count < active_cards_count[player] ){
		   if( in_play(player, count) && is_what(player, count, TYPE_LAND) && ! is_basic_land(player, count) ){
			   return 1;
		   }
		   count++;
	}

	return 0;
}

int control_basic_land(int player){
	if( player != 2 ){
		int count = 0;
		while( count < active_cards_count[player] ){
				if( in_play(player, count) && is_what(player, count, TYPE_LAND) && is_basic_land(player, count) ){
					return 1;
				}
				count++;
		}
		return 0;
	}
	else{
		int i;
		for(i=0; i<2; i++){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && is_what(i, count, TYPE_LAND) && is_basic_land(i, count) ){
						return 1;
					}
					count++;
			}
		}
		return 0;
	}
	return 0;
}

int check_base_value_for_ai(int card, int conf_base_value, int mode){
	// Mode = 1 -> Select the greatest
	// Mode = -1 -> Select the lowest

	if( card < 0 ){
		return 0;
	}

	int base_value = get_base_value(-1, cards_data[card].id);
	if( mode == 1 && base_value > conf_base_value ){
		return 1;
	}
	if( mode == -1 && base_value < conf_base_value ){
		return 1;
	}
	return 0;
}

int player_reveals_x_and_discard(int t_player, int who_choses, int howmany, int effect, int ai_selection_mode, int type, int flag1,
					int subtype, int flag2, int color, int flag3, int id, int flag4, int cmc, int flag5
  ){
	// The function will return the CSV ID of the discarded card, unless otherwise noted.
	// effect = 1 -> Discard
	// effect = 2 -> Exile
	// effect = 4 -> Used for Karn Liberated (AI will chosse the less AI Base value among non-permanents).
	// effect = 8 -> Select a card to play from opponent's hand. Will return the CARD value, not the CSV ID.
	// effect = 16 -> Select a creature or a planeswalker (Despise)

	test_definition_t this_test;
	default_test_definition(&this_test, type);
	this_test.type_flag = flag1;
	this_test.subtype = subtype;
	this_test.subtype_flag = flag2;
	this_test.color = color;
	this_test.color_flag = flag3;
	this_test.id = id;
	this_test.id_flag = flag4;
	this_test.cmc = cmc;
	this_test.cmc_flag = flag5;

	test_definition_t this_test2;
	default_test_definition(&this_test2, TYPE_ANY);

	int count = 0;
	int initial_index = 0;
	int hand_index = 0;
	int initial_hand[50];
	int ai_hand[50];

	if( howmany > hand_count[t_player] ){
		howmany = hand_count[t_player];
	}
	while( count < active_cards_count[t_player] ){
		   if( in_hand(t_player, count) ){
			   card_instance_t *crd = get_card_instance(t_player, count);
			   initial_hand[initial_index] = crd->internal_card_id;
			   initial_index++;
		   }
		   count++;
	}

	count = 0;
	int selected = -1;
	while( count < howmany ){
			selected = -1;
			selected = select_card_from_zone(t_player, t_player, initial_hand, initial_index, 1, AI_MIN_VALUE, -1, &this_test2);
			if( selected != -1 ){
				ai_hand[hand_index] = initial_hand[selected];
				hand_index++;
				int i;
				for(i=selected; i<(initial_index-1); i++){
					initial_hand[i] = initial_hand[i+1];
				}
				initial_index--;
				count++;
			}
	}

	int result = 0;
	selected = select_card_from_zone(who_choses, t_player, ai_hand, hand_index, 1, AI_MAX_VALUE, -1, &this_test);

	if( selected == -1 ){
		return -1;
	}

	count = active_cards_count[t_player]-1;
	while( count > -1 ){
		   if( in_hand(t_player, count) ){
			   card_instance_t *crd = get_card_instance(t_player, count);
			   if( crd->internal_card_id == ai_hand[selected] ){
				   result = get_id(t_player, count);
				   if( effect & 1 ){
					   new_discard_card(t_player, count, who_choses, 0);
				   }
				   if( effect & 2 ){
						rfg_card_in_hand(t_player, count);
				   }
				   if( effect & 32 ){
						put_on_top_of_deck(t_player, count);
				   }
				   if( effect & 4 || effect & 64 ){
					   result = count;
				   }
				   break;
			   }
		   }
		   count--;
	}

	return result;
}

static int discard_card_by_int_id(int player, int int_crd_id, int effect, test_definition_t *this_test, int source_of_effect){
	int result = -1;
	int count = active_cards_count[player]-1;
	while( count > -1 ){
			if( in_hand(player, count) ){
				card_instance_t *crd = get_card_instance(player, count);
				int good = 0;
				if( int_crd_id > -1 && crd->internal_card_id == int_crd_id ){
					good = 1;
				}
				if( int_crd_id == -1 && new_make_test_in_play(player, count, -1, this_test) ){
					good = 1;
				}
				if( good == 1 ){
					result = get_id(player, count);
					if( (effect & EC_KARN_LIBERATED) || (effect & EC_SELECT_CARD) || (effect & EC_GERRARDS_VERDICT) ){
						result = count;
					}
					if( effect & EC_DISCARD ){
						new_discard_card(player, count, source_of_effect, 0);
					}
					if( effect & EC_RFG ){
						rfg_card_in_hand(player, count);
					}
					if( effect & EC_PUT_ON_TOP ){
						put_on_top_of_deck(player, count);
					}
					if( effect & EC_PUT_ON_BOTTOM ){
						put_on_bottom_of_deck(player, count);
					}
					if( int_crd_id > -1 ){
						break;
					}
				}
			}
			count--;
	}
	return result;
}

void default_ec_definition(int t_player, int who_choses, ec_definition_t *this_definition){
	this_definition->target_player = t_player;
	this_definition->who_choses = who_choses;
	this_definition->effect = EC_DISCARD;
	this_definition->qty = 1;
	this_definition->cards_to_reveal = 0;
	this_definition->ai_selection_mode = AI_MAX_VALUE;
	this_definition->special_selection_function = NULL;
}

int new_effect_coercion(ec_definition_t *this_definition, test_definition_t *this_test){
	int initial_index = 0;
	int ai_index = 0;
	int initial_hand[50];
	int ai_hand[50];

	test_definition_t test_default;
	if (!this_test){
		default_test_definition(&test_default, TYPE_ANY);
		this_test = &test_default;
	}

	if( this_definition->cards_to_reveal > 0 ){
		if( this_definition->cards_to_reveal > hand_count[this_definition->target_player] ){
			this_definition->cards_to_reveal = hand_count[this_definition->target_player];
		}

		int count = 0;
		while( count < active_cards_count[this_definition->target_player] ){
			   if( in_hand(this_definition->target_player, count) ){
				   initial_hand[initial_index] = get_original_internal_card_id(this_definition->target_player, count);
				   initial_index++;
			   }
			   count++;
		}

		test_definition_t this_test2;
		new_default_test_definition(&this_test2, TYPE_ANY, "Select a card to reveal.");

		while( ai_index < this_definition->cards_to_reveal){
				int selected = select_card_from_zone(this_definition->target_player, this_definition->target_player, initial_hand, initial_index, 1,
													AI_MIN_VALUE, -1, &this_test2);
				if( selected != -1 ){
					ai_hand[ai_index] = initial_hand[selected];
					ai_index++;
					int i;
					for(i=selected; i<(initial_index-1); i++){
						initial_hand[i] = initial_hand[i+1];
					}
					initial_index--;
				}
		}
	}
	int result = -1;
	if( !(this_definition->effect & EC_ALL_WHICH_MATCH_CRITERIA) ){

		int i;
		for (i = 0; i < this_definition->qty; ++i){
			int iid = -1;

			if( this_definition->cards_to_reveal < 1){
				if (hand_count[this_definition->target_player] <= 0){
					break;
				}
				this_test->zone = TARGET_ZONE_HAND;
				int no_cancel = 0;
				if( check_battlefield_for_special_card(-1, -1, this_definition->target_player, 0, this_test) ){
					no_cancel = 1;
				}

				int selected = new_select_a_card(this_definition->who_choses, this_definition->target_player, TUTOR_FROM_HAND, no_cancel,
												 this_definition->ai_selection_mode, -1, this_test);
				if( selected == -1 ){
					break;
				}
				iid = get_original_internal_card_id(this_definition->target_player, selected);
			}
			else{
				if (ai_index <= 0){
					break;
				}
				int no_cancel = 0;
				int j;
				for (j = 0; j < ai_index; ++j){
					if (new_make_test(this_definition->target_player, ai_hand[j], -1, this_test)){
						no_cancel = 1;
						break;
					}
				}
				int selected = select_card_from_zone(this_definition->who_choses, this_definition->target_player, ai_hand, ai_index, no_cancel,
													 this_definition->ai_selection_mode, -1, this_test);
				if( selected == -1 ){
					break;
				}
				iid = ai_hand[selected];
				--ai_index;
				for (j = selected; j < ai_index; ++j){
					ai_hand[j] = ai_hand[j + 1];
				}
			}
			if( iid != -1 ){
				result = discard_card_by_int_id(this_definition->target_player, iid, this_definition->effect, 0, this_definition->who_choses);
			}
		}
	}
	else{
		if( this_definition->cards_to_reveal < 1){
			reveal_target_player_hand(this_definition->target_player);
			discard_card_by_int_id(this_definition->target_player, -1, this_definition->effect, this_test, this_definition->who_choses);
		}
		else{
			show_deck( this_definition->who_choses, ai_hand, ai_index, "Target player revealed these.", 0, 0x7375B0 );
			int k;
			for(k=0; k<ai_index; k++){
				if( new_make_test(this_definition->target_player, ai_hand[k], -1, this_test) ){
					discard_card_by_int_id(this_definition->target_player, ai_hand[k], EC_DISCARD, NULL, this_definition->who_choses);
				}
			}
		}
	}

	return result;
}

int check_battlefield_for_id(int player, int id){

	if( id < 0 ){
		return 0;
	}

	int i;
	for(i=0; i<2; i++){
		if( player == i || player == 2 ){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && get_id(i, count) == id && ! is_humiliated(i, count) ){
						// this should be right 99% of the cases, since an "humiliated" card lose all abilities
						return 1;
					}
					count++;
			}
		}
	}

	return 0;
}

int locate_id(int player, int id){

	int i;
	for(i=0; i<2; i++){
		if( player == 2 || i == player ){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && get_id(i, count) == id ){
						return count;
					}
					count++;
			}
		}
	}
	return -1;
}

int check_battlefield_for_targettable_id(int player, int card, int t_player, int id, int mode){

	if( id == -1 ){
		return 0;
	}

	if( t_player < 2 ){
		int count = 0;
		while( count < active_cards_count[t_player] ){
			   if( in_play(t_player, count) && get_id(t_player, count) == id ){
					if( ! is_protected_from_me(player, card, t_player, count) ){
						if( mode == 0 ){
							return 1;
						}
						if( mode == 1 ){
							return count;
						}
					}
			   }
			   count++;
		}
	}

	else if( player == 2 ){
			 int i;
			 for(i=0; i<2; i++){
				 int count = 0;
				 while( count < active_cards_count[i] ){
						if( in_play(i, count) && get_id(i, count) == id ){
							if( ! is_protected_from_me(player, card, i, count) ){
								if( mode == 0 ){
									return 1;
								}
								if( mode == 1 ){
									return count;
								}
							}
						}
						count++;
				 }
			 }
	}

	return 0;
}

void lobotomy_effect(int player, int t_player, int csvid, int mode){
	// mode & 1 --> Must find all copies of the CSVID
	// mode & 2 --> Do not shuffle at the end of the effect
	test_definition_t this_test;
	new_default_test_definition(&this_test, 0, "");
	this_test.id = csvid;
	this_test.no_shuffle = 1;

	if (ai_is_speculating != 1){
		scnprintf(this_test.message, 100, "Hand: Select a card named %s to exile.", cards_ptr[csvid]->full_name);
	}
	while( hand_count[t_player] > 0 && new_global_tutor(player, t_player, TUTOR_FROM_HAND, TUTOR_RFG, 0, AI_FIRST_FOUND, &this_test) != -1 ){
	}

	if (ai_is_speculating != 1){
		scnprintf(this_test.message, 100, "Graveyard: Select a card named %s to exile.", cards_ptr[csvid]->full_name);
	}
	while( new_special_count_grave(t_player, &this_test) > 0 &&
		   new_global_tutor(player, t_player, TUTOR_FROM_GRAVE, TUTOR_RFG, (mode & 1) ? 1 : 0, AI_FIRST_FOUND, &this_test) != -1
		 ){
	}

	if (ai_is_speculating != 1){
		scnprintf(this_test.message, 100, "Library: Select a card named %s to exile.", cards_ptr[csvid]->full_name);
	}
	while( deck_ptr[t_player][0] != -1 && new_global_tutor(player, t_player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_FIRST_FOUND, &this_test) != -1 ){
	}

	/* Shuffle even if the library is now empty - 701.16f If an effect causes a player to shuffle a library containing zero or one cards, abilities that trigger
	 * when a library is shuffled will still trigger. */
	if( !( mode & 2) ){
		shuffle(t_player);
	}
}

int is_mana_producer_land(int player, int card){
	int crd = card;
	if( player !=-1 ){
		card_instance_t *instance = get_card_instance(player, card);
		crd = instance->internal_card_id;
	}
	if( is_what(-1, crd, TYPE_LAND) && (cards_data[crd].extra_ability & EA_MANA_SOURCE) ){
		return 1;
	}
	return 0;
}

int is_planeswalker(int player, int card){

	if( player == -1 ){
		card_data_t* card_d = &cards_data[card];
		if( (card_d->type & TYPE_ENCHANTMENT) && card_d->cc[2] == 9  ){
			return 1;
		}
	}
	else{
		card_data_t* card_d = get_card_data(player, card);
		if( (card_d->type & TYPE_ENCHANTMENT) && card_d->cc[2] == 9  ){
			return 1;
		}
	}

  return 0;
}

void make_all_untargettable(void){

	 int i;
	 for(i=0; i<2; i++){
		 int count = 0;
		 while( count < active_cards_count[i] ){
				if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
					card_instance_t *this = get_card_instance(i, count);
					this->state |= STATE_CANNOT_TARGET;
				}
				count++;
		 }
	 }
}

void remove_untargettable_from_all(void){

	 int i;
	 for(i=0; i<2; i++){
		 int count = 0;
		 while( count < active_cards_count[i] ){
				if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) ){
					card_instance_t *this = get_card_instance(i, count);
					if( this->state & STATE_CANNOT_TARGET ){
						this->state  &= ~STATE_CANNOT_TARGET;
					}
				}
				count++;
		 }
	 }
}

void nice_creature_to_sacrifice(int player, int card){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[16].card < 0 ){
		instance->targets[16].card = 0;
	}

	if( !is_humiliated(player, card) ){
		if( ! (instance->targets[16].card & SP_KEYWORD_CANNON_FODDER) ){
			instance->targets[16].card |= SP_KEYWORD_CANNON_FODDER;
		}
	}
}

static int effect_prevent_the_next_n_damage(int player, int card, event_t event){

	// instance->counter_toughness -> max damage to prevent (accessible in effect text as |n)
	// bitfields for instance->targets[2].player
	//  1 -> add a +1/+1 counter for each damage prevented
	//  2 -> Gain life equal to damage prevented
	//  4 -> redirect damage to targets[1]
	//  8 -> no limit to damage prevented

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *instance = get_card_instance(player, card);

		int t_player = instance->targets[0].player;
		int t_card = instance->targets[0].card;

		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == t_player &&
			damage->damage_target_card == t_card && damage->info_slot > 0 && ! check_state(affected_card_controller, affected_card, STATE_CANNOT_TARGET)
		  ){
			int amount;
			if( instance->targets[2].player & PREVENT_INFINITE ){
				amount = damage->info_slot;
			} else {
				amount = MIN(damage->info_slot, instance->counter_toughness);
				amount = MAX(0, amount);
			}

			if( amount > 0 ){
				damage->info_slot -= amount;
				if( !(instance->targets[2].player & PREVENT_INFINITE) ){
					instance->counter_toughness -= amount;
				}
				if( (instance->targets[2].player & PREVENT_ADD_1_1_COUNTER) && damage->damage_target_card > -1 ){
					add_1_1_counters(damage->damage_target_player, damage->damage_target_card, amount);
				}
				if( (instance->targets[2].player & PREVENT_GAIN_LIFE) ){
					gain_life(player, amount);
				}
				if( (instance->targets[2].player & PREVENT_REDIRECT) &&
					instance->targets[1].player >= 0 &&
					(instance->targets[1].card == -1 || in_play(instance->targets[1].player, instance->targets[1].card))
				  ){
					damage_creature(instance->targets[1].player, instance->targets[1].card, amount,
											damage->damage_source_player, damage->damage_source_card);
				}
			}

			if( instance->counter_toughness <= 0 && !(instance->targets[2].player & PREVENT_INFINITE) ){
				kill_card(player, card, KILL_REMOVE);
			} else {
				/* Force the effect card to redisplay, so the new number is seen in the title.
				 *
				 * The best way would be to find its hwnd and call InvalidateRect(hwnd, 0, 0), but we don't know how the canonical way to do that yet.
				 *
				 * Instead, we have to do something to make the engine see the card as having changed.  The function at 0x48c9d0 seems to determine what
				 * qualifies.  We can't even just pick one bit that gets looked at and toggle it, since this might get called twice (or some other even number
				 * of times) between redisplays.
				 *
				 * Annoyingly enough, changing info_slot is usually the least harmful way to do it, but that's where the effect function is stored in
				 * LEGACY_EFFECT_CUSTOM.
				 *
				 * The second least-harmful way is to manipulate untap_status: any change forces a redisplay, but the exe otherwise only ever inspects or alters
				 * bits 1 and 2 of byte0.  We'll increment bits 3-7, leaving bytes 1-3 and bits 1-2 of byte0 untouched. */
				uint8_t val = BYTE0(instance->untap_status) & (0xFF & ~3);
				if (val >= 252){
					val = 0;
				} else {
					val += 4;
				}
				SET_BYTE0(instance->untap_status) = val | (BYTE0(instance->untap_status) & 3);
			}
		}
	}

	if (event == EVENT_SET_LEGACY_EFFECT_NAME){	// Since | codes don't get interpreted in effect card names, where we need this one most
		card_instance_t* instance = get_card_instance(player, card);
		snprintf(set_legacy_effect_name_addr, 51, "%s (%d)", get_card_name_by_id(instance->display_pic_csv_id), instance->counter_toughness);
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

void prevent_the_next_n_damage(int player, int card, int t_player, int t_card, int amount, prevent_t flags, int redir_to_player, int redir_to_card)
{
  if (amount <= 0 && !(flags & PREVENT_INFINITE))
	return;

  int leg, opponent = 0;
  if (t_card != -1)
	leg = create_targetted_legacy_effect(player, card, &effect_prevent_the_next_n_damage, t_player, t_card);
  else if (player == t_player)
	leg = create_legacy_effect(player, card, &effect_prevent_the_next_n_damage);
  else
	{
	  // So it shows up in the opponent's battlefield
	  leg = create_legacy_effect_for_opponent(player, card, &effect_prevent_the_next_n_damage, -1, -1);
	  player = 1-player;
	  opponent = 1;
	}

  if (leg == -1)
	return;

  card_instance_t* legacy = get_card_instance(player, leg);

  // If we're targeting a player, these won't have been set already
  legacy->targets[0].player = t_player;
  legacy->targets[0].card = t_card;
  legacy->number_of_targets = 1;

  if (opponent)
	legacy->state ^= STATE_OWNED_BY_OPPONENT;

  if (flags & PREVENT_REDIRECT)
	{
	  if (redir_to_player < 0 || redir_to_player > 1
		  || redir_to_card < -1 || redir_to_card >= 150)
		flags &= ~PREVENT_REDIRECT;
	  else
		{
		  legacy->number_of_targets = 2;
		  legacy->targets[1].player = redir_to_player;
		  legacy->targets[1].card = redir_to_card;
		}
	}

  legacy->targets[2].player = flags;
  if (!(flags & PREVENT_INFINITE))
	{
	  legacy->eot_toughness = EVENT_SET_LEGACY_EFFECT_NAME;
	  legacy->counter_toughness = amount;
	}

  /* If this was cast while there were damage cards on the battlefield, redispatch EVENT_PREVENT_DAMAGE to it to make sure it resolves immediately.
   * Compare resolve_damage_cards_and_prevent_damage() at 0x477070. */
  if (land_can_be_played & LCBP_DAMAGE_PREVENTION)
	{
	  card_instance_t* inst;
	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if ((inst = in_play(p, c))
			  && inst->internal_card_id == damage_card
			  && !(inst->state & STATE_TAPPED))
			{
			  dispatch_event_arbitrary_to_one_card(player, leg, EVENT_PREVENT_DAMAGE, p, c, 1-p, -1);
			  if (legacy->internal_card_id == -1)	// removed itself
				return;
			}
	}
}

void reveal_target_player_hand(int player ){

	 int cards_array[60];
	 int revealed = 0;
	 int i;
	 for(i=0;i<active_cards_count[player]; i++){
		 if( in_hand(player, i) ){
			 card_instance_t *rev_card = get_card_instance(player, i);
			 cards_array[revealed] = rev_card->internal_card_id;
			 revealed++;
		 }
	 }

	 if( revealed > 0){
		 show_deck( 1-player, cards_array, revealed, "Cards revealed", 0, 0x7375B0 );
	 }
}

int get_most_common_cmc_nonland(int player)
{
  int c, cmc[17] = {0};

  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_play(player, c) && is_what(player, c, TYPE_PERMANENT) && !is_what(player, c, TYPE_LAND))
	  cmc[get_cmc(player, c)]++;

  int i, result = -1, par = 0;
  for (i = 0; i < 17; ++i)
	if (cmc[i] > par)
	  {
		par = cmc[i];
		result = i;
	  }

  return result;
}

int get_most_common_cmc_in_hand(int player, type_t type)
{
  int c, cmc[17] = {0};

  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_hand(player, c) && is_what(player, c, type))
	  cmc[get_cmc(player, c)]++;

  int i, result = -1, par = 0;
  for (i = 0; i < 17; ++i)
	if (cmc[i] > par)
	  {
		par = cmc[i];
		result = i;
	  }

  return result;
}

int get_highest_cmc(int player, type_t type){
	int p, c, result = 0;
	for(p=0; p<2; p++){
		if( player == p || player == ANYBODY ){
			for (c = 0; c < active_cards_count[p]; ++c){
				if (in_play(p, c) && is_what(p, c, type)){
					int cmc = get_cmc(p, c);
					if (cmc > result){
						result = cmc;
					}
				}
			}
		}
	}
	return result;
}

int get_highest_cmc_nonland(int player)
{
  int c, result = 0;
  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_play(player, c) && is_what(player, c, TYPE_PERMANENT) && !is_what(player, c, TYPE_LAND))
	  {
		int cmc = get_cmc(player, c);
		if (cmc > result)
		  result = cmc;
	  }

  return result;
}

int symbiotic_creature(int player, int card, event_t event, token_generation_t *token){

	nice_creature_to_sacrifice(player, card);

	if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)){
		generate_token(token);
	}

	return 0;
}

// AI leaves tapped if in_play(targets[1])
void choose_to_untap(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_COULD_NOT_UNTAP);
	}

	if( instance->targets[1].player > -1 && ! in_play(instance->targets[1].player, instance->targets[1].card) ){
		instance->targets[1].player = -1;
	}

	if (event == EVENT_UNTAP && affect_me(player, card) && is_tapped(player, card)){
		instance->untap_status &= ~2;
	}

	if (current_phase == PHASE_UNTAP && affect_me(player, card)){
		if (event == EVENT_TRIGGER){
			if (instance->untap_status & 1){
				if (!(instance->untap_status & 2)
					&& !(cards_data[instance->internal_card_id].type & types_that_dont_untap)){
					if ((player != 1 || (trace_mode & 2)) && ai_is_speculating != 1){
						event_result |= 1;	// Human: untapping is optional
					} else if (instance->targets[1].player > -1 && in_play(instance->targets[1].player, instance->targets[1].card)){
						;	// AI: leave tapped
					} else {
						event_result |= 2;	// AI: untap
					}
				}
			}
		} else if (event == EVENT_RESOLVE_TRIGGER){
			instance->untap_status |= 2;
		}
	}
}

// Call as if (choosing_to_untap(player, card, event)) { return should_ai_untap; }.  AI will leave tapped if that return value is 0, otherwise will untap.
int choosing_to_untap(int player, int card, event_t event)
{
  if (event == EVENT_UNTAP && affect_me(player, card))
	get_card_instance(player, card)->untap_status &= ~2;

  if (current_phase == PHASE_UNTAP && affect_me(player, card))
	{
	  if (event == EVENT_TRIGGER)
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  if ((instance->untap_status & 3) == 1
			  && !(cards_data[instance->internal_card_id].type & types_that_dont_untap))
			{
			  if (!IS_AI(player))
				event_result |= 1;	// Human: untapping is optional
			  else if (call_card_function_i(instance, player, card, EVENT_CHOOSING_TO_UNTAP))
				event_result |= 2;	// AI: untap if return nonzero from response
			}
		}
	  else if (event == EVENT_RESOLVE_TRIGGER)
		get_card_instance(player, card)->untap_status |= 2;
	}

  return event == EVENT_CHOOSING_TO_UNTAP;
}

void add_state(int player, int card, int st){
	if( card != -1 ){
		card_instance_t *instance = get_card_instance(player, card);
		instance->state |= st;
	}
	else{
		int i;
		for(i=0; i<2; i++){
			if( player == 2 || i == player ){
				int count = 0;
				while( count < active_cards_count[i] ){
						if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
							card_instance_t *instance = get_card_instance(i, count);
							instance->state |= st;
						}
						count++;
				}
			}
		}
	}
}

void remove_state(int player, int card, int st){
	if( card != -1 ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->state & st ){
			instance->state &= ~st;
		}
	}
	else{
		int i;
		for(i=0; i<2; i++){
			if( player == 2 || i == player ){
				int count = 0;
				while( count < active_cards_count[i] ){
						if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) ){
							card_instance_t *instance = get_card_instance(i, count);
							if( instance->state & st ){
								instance->state &= ~st;
							}
						}
						count++;
				}
			}
		}
	}
}

void add_status(int player, int card, int st){
	card_instance_t *instance = get_card_instance(player, card);
	instance->token_status |= st;
}

void remove_status(int player, int card, int st){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->token_status & st ){
		instance->token_status &= ~st;
	}
}

void state_untargettable(int player, int card, int mode){
	// mode = 1 --> Give
	// mode = 0 --> Remove

	card_instance_t *instance = get_card_instance(player, card);

	if( mode == 0 && ( instance->state & STATE_CANNOT_TARGET) ){
		instance->state &= ~STATE_CANNOT_TARGET;
	}

	if( mode == 1 ){
		instance->state |= STATE_CANNOT_TARGET;
	}
}

int pick_targets_for_multidamage(int player, int card, int max_targets, int damages, target_definition_t *td, const char *prompt){

	card_instance_t *instance = get_card_instance(player, card);

	load_text(0, prompt);
	int trgts = 0;
	if( player != AI ){
		int players[2] = {0, 0};
		while( trgts < max_targets && can_target(td) ){
				if( select_target(player, card, td, text_lines[0], &(instance->targets[trgts])) ){
					if( instance->targets[trgts].card == -1 ){
						if( players[instance->targets[trgts].player] != 1 ){
							players[instance->targets[trgts].player] = 1;
							trgts++;
						}
						else{
							if( td->allow_cancel != 0 ){
								break;
							}
						}
					}
					else{
						state_untargettable(instance->targets[trgts].player, instance->targets[trgts].card, 1);
						trgts++;
					}
				}
				else{
					if( td->allow_cancel != 0 ){
						break;
					}
				}
		}
		int i;
		for(i=0; i<trgts; i++){
			if( instance->targets[i].card != -1 ){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
		}
	}
	else{
		if( td->zone == TARGET_ZONE_CREATURE_OR_PLAYER ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			if( would_validate_target(player, card, td, 0) ){
				trgts++;
			}
		}
		int count = 0;
		while( count < active_cards_count[1-player] && trgts < max_targets ){
				if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
					if( ! is_protected_from_me(player, card, 1-player, count) &&
						get_toughness(1-player, count) <= damages
					  ){
						instance->targets[trgts].player = 1-player;
						instance->targets[trgts].card = count;
						trgts++;
					}
				}
				count++;
		}

		if( trgts < max_targets && td->allow_cancel == 0 ){
			count = 0;
			while( count < active_cards_count[1-player] && trgts < max_targets ){
					if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
						if( ! is_protected_from_me(player, card, 1-player, count) &&
							get_toughness(1-player, count) > damages
						  ){
							instance->targets[trgts].player = 1-player;
							instance->targets[trgts].card = count;
							trgts++;
						}
					}
					count++;
			}
		}

		if( trgts < max_targets && td->allow_cancel == 0 ){
			count = 0;
			while( count < active_cards_count[player] && trgts < max_targets ){
					if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
						if( ! is_protected_from_me(player, card, player, count) &&
							get_toughness(player, count) > damages
						  ){
							instance->targets[trgts+1].player = player;
							instance->targets[trgts+1].card = count;
							trgts++;
						}
					}
					count++;
			}
		}

		if( trgts < max_targets && td->allow_cancel == 0 ){
			count = 0;
			while( count < active_cards_count[player] && trgts < max_targets ){
					if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
						if( ! is_protected_from_me(player, card, player, count) ){
							instance->targets[trgts+1].player = player;
							instance->targets[trgts+1].card = count;
							trgts++;
						}
					}
					count++;
			}
		}
	}
	return trgts;
}

void multidamage(int player, int card, int num_targets, int damages, target_definition_t *td){
	// requires the definition of targets via the "pick_targets_for_multidamage" function

	if( num_targets < 1 ){
		return;
	}

	card_instance_t *instance = get_card_instance(player, card);

	int i;
	for(i=0; i<num_targets; i++){
		if( validate_target(player, card, td, i) ){
			damage_creature(instance->targets[i].player, instance->targets[i].card, damages, player, card);
		}
	}
}

void nobody_can_attack(int player, int card, event_t event, int t_player)
{
  if (event == EVENT_ATTACK_LEGALITY
	  && (affected_card_controller == t_player || t_player == ANYBODY))
	event_result = 1;
}

static int effect_nobody_can_attack_until_eot(int player, int card, event_t event)
{
  /* Local storage:
   * damage_target_player/damage_target_card: this card can always still attack.
   * targets[1].player: the player who can't attack.
   * targets[1].card: &1: ends at end of turn.
   *                  &2: ends after target player's next declare attackers step or second main phase.
   *                  &4: ends after target player's next declare attackers step; during discard or cleanup, changes to &2.  Suitable for assigning during combat.
   *                  &8: ends at start of controller's next turn
   */

  event_flags |= EA_DECLARE_ATTACK;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_ATTACK_LEGALITY
	  && (affected_card_controller == instance->targets[1].player || instance->targets[1].player == ANYBODY)
	  && !affect_me(instance->damage_target_player, instance->damage_target_card))
	event_result = 1;

  if (trigger_condition == TRIGGER_LEAVE_PLAY
	  && trigger_cause_controller == instance->damage_target_player && trigger_cause == instance->damage_target_card)
	instance->damage_target_player = instance->damage_target_card = -1;	// detach (immediately)

  if ((event == EVENT_CLEANUP && (instance->targets[1].card & 1))
	  || (event == EVENT_PHASE_CHANGED && (instance->targets[1].card & 2)
		  && current_turn == instance->targets[1].player && (current_phase == PHASE_MAIN2 || current_phase == PHASE_BEFORE_BLOCKING))
	  || (event == EVENT_PHASE_CHANGED && (instance->targets[1].card & 4)
		  && current_turn == instance->targets[1].player && current_phase == PHASE_BEFORE_BLOCKING)
	  || (event == EVENT_BEGIN_TURN && current_turn == player && (instance->targets[1].card & 8)))
	{
	  instance->targets[1].card = 0;
	  kill_card(player, card, KILL_REMOVE);
	}

  if ((instance->targets[1].card & 4) && current_phase > PHASE_MAIN2 && current_turn == instance->targets[1].player)
	{
	  instance->targets[1].card &= ~4;
	  instance->targets[1].card |= 2;
	}

  return 0;
}

/* Only except_player/except_card, and creatures not controlled by t_player, can attack this turn.  t_player can be 2 for anybody, and except_player/except_card
 * can be -1 to not provide an exception. */
void nobody_can_attack_until_eot(int player, int card, int t_player, int except_player, int except_card)
{
  int leg = (except_card < 0 ? create_legacy_effect(player, card, effect_nobody_can_attack_until_eot)
			 : create_targetted_legacy_effect(player, card, effect_nobody_can_attack_until_eot, except_player, except_card));

  if (leg != -1)
	{
	  card_instance_t* legacy = get_card_instance(player, leg);
	  legacy->targets[1].player = t_player;
	  legacy->targets[1].card = 1;
	}
}

void target_player_skips_his_next_attack_step(int player, int card, int t_player, int this_turn_only)
{
  int leg = create_legacy_effect(player, card, effect_nobody_can_attack_until_eot);

  if (leg != -1)
	{
	  card_instance_t* legacy = get_card_instance(player, leg);
	  legacy->targets[1].player = t_player;
	  legacy->targets[1].card = (current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2) ? 4 : 2;
	  if (this_turn_only)
		legacy->targets[1].card |= 1;
	}
}

void nobody_can_attack_until_your_next_turn(int player, int card, int t_player)
{
  int leg = create_legacy_effect(player, card, effect_nobody_can_attack_until_eot);

  if (leg != -1)
	{
	  // Not sure whether it's better or worse to always set this here than to return the legacy and let the caller muck with it.
	  alternate_legacy_text(2, player, leg);

	  card_instance_t* legacy = get_card_instance(player, leg);
	  legacy->targets[1].player = t_player;
	  legacy->targets[1].card = 8;
	}
}

int count_planeswalkers(int player, int mode){

	int result = 0;
	int count = 0;
	while( count < active_cards_count[player] ){
			if( in_play(player, count) && is_planeswalker(player, count) ){
				if( mode == 1 ){
					return 1;
				}
				result++;
			}
			count++;
	}

	return result;
}

int is_legendary(int player, int card){
	// use player = -1 if is a value from deck/graveyard/rfg

	int value = 0;
	if( player != -1 ){
		if (get_original_id(player, card) == CARD_ID_SAKASHIMA_THE_IMPOSTOR){
			return 1;
		}
		card_data_t* card_d = get_card_data(player, card);
		value = card_d->subtype;
	}
	else{
		 value = cards_data[card].subtype;
	}

	if( value == 0x0e || value == 0x0f ){
		return 1;
	}

	return 0;
}

int fog_effect_legacy(int player, int card, event_t event ){
	/*
	 * targets[0].player: prevent damage to this player and his creatures (or by this player's creatures, if
	 *                    FOG_BY_CONTROLLED_BY_PLAYER is set)
	 * targets[0].card:   a fog_special_flags_t
	 * targets[1].player: param value given to fog_special2()
	 * targets[1].card:   csvid of the effect's source.  Redundant to display_pic_csv_id.
	 */
	card_instance_t *instance = get_card_instance(player,card);
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 && ! check_state(affected_card_controller, affected_card, STATE_CANNOT_TARGET) ){
			fog_special_flags_t flags = instance->targets[0].card;
			if( (flags & FOG_BY_ANY_TYPE) || is_what(source->damage_source_player, source->damage_source_card, TYPE_CREATURE) ){
				if( instance->targets[0].player == ANYBODY ||
					instance->targets[0].player == ((flags & FOG_BY_CONTROLLED_BY_PLAYER)
													? source->damage_source_player
													: source->damage_target_player)
				  ){
					int good = 1;
					if( flags & FOG_COMBAT_DAMAGE_ONLY ){
						if( !(source->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE)) ){
							good = 0;
						}
					}
					if( flags & FOG_UNBLOCKED_CREATURES_ONLY ){
						if( ! is_unblocked(source->damage_source_player, source->damage_source_card) ){
							good = 0;
						}
					}
					if( flags & FOG_CREATURES_WITHOUT_TRAMPLE ){
						if( check_for_ability(source->damage_source_player, source->damage_source_card, KEYWORD_TRAMPLE) ){
							good = 0;
						}
					}
					if( flags & FOG_COLOR_MASK ){
						int clr = source->initial_color;
						if( in_play(source->damage_source_player, source->damage_source_card) ){
							clr = get_color(source->damage_source_player, source->damage_source_card);
						}
						if( flags & FOG_INVERSE_COLOR_CHECK ){
							if( clr & flags ){
								good = 0;
							}
						}
						else{
							if( !(clr & flags) ){
								good = 0;
							}
						}
					}
					if( flags & FOG_BY_NON_SUBTYPE ){
						if( instance->targets[1].player > 0 &&
							has_subtype(source->damage_source_player, source->damage_source_card,
										instance->targets[1].player) ){
							good = 0;
						}
					}
					if( good == 1 ){
						source->info_slot = 0;
					}
				}
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int fog_special2(int player, int card, int t_player, fog_special_flags_t flags, int param){
	 // set the "fog effects" flag for Planeswalkers
	 // ...which is entirely wrong if any flags at all are set, but I'm not going to bother fixing it.
	 int i;
	 for(i=0; i<2; i++){
		 int count = 0;
		 while( count < active_cards_count[i] ){
				if( in_play(i, count) && is_planeswalker(i, count) ){
					set_special_flags(i, count, SF_WONT_RECEIVE_DAMAGE);
				}
				count++;
		}
	}

	int legacy = create_legacy_effect(player, card, &fog_effect_legacy);
	card_instance_t *instance= get_card_instance(player,legacy);
	instance->targets[0].player = t_player;
	instance->targets[0].card = flags;
	instance->targets[1].player = param;
	instance->targets[1].card = get_id(player, card);
	return legacy;
}

int fog_special(int player, int card, int t_player, int flags){
	return fog_special2(player, card, t_player, flags, 0);
}

int fog_effect(int player, int card){
	return fog_special2(player, card, ANYBODY, FOG_COMBAT_DAMAGE_ONLY, 0);
}

int colors_shared(int clr1, int clr2){
	return clr1 & clr2 & COLOR_TEST_ANY_COLORED;
}

int has_my_colors(int player, int card, int t_player, int t_card ){

	int clrs[2];
	if( player != -1 ){
		clrs[0] = get_color(player, card);
	}
	else{
		clrs[0] = cards_data[card].color;
	}
	if( t_player != -1 ){
		clrs[1] = get_color(t_player, t_card);
	}
	else{
		clrs[1] = cards_data[t_card].color;
	}

	return colors_shared(clrs[0], clrs[1]);
}

void modify_pt_and_abilities(int player, int card, event_t event, int p_mod, int t_mod, int a_mod ){

	if( event == EVENT_POWER && affect_me(player, card) ){
		if( p_mod > 100 ){
			event_result = (p_mod-100);
		}
		else{
			event_result += p_mod;
		}
	}

	if( event == EVENT_TOUGHNESS && affect_me(player, card) ){
		if( t_mod > 100 ){
			event_result = (t_mod-100);
		}
		else{
			event_result += t_mod;
		}
	}

	if( event == EVENT_ABILITIES && affect_me(player, card) ){
		event_result |= a_mod;
	}
}

int get_cost_mod_for_activated_abilities(int player, int card, int orig_cost, int black, int blue, int green, int red, int white){
	// uses "targets[17].card" as storage
	// bit 12-15 --> -1 colorless mana (min 1) x bit set
	// bit 16-19 --> -2 colorless mana (min 1) x bit set
	// bit 20-23 --> +2 colorless mana x bit set
	// bit 24-27 --> +3 colorless mana x bit set
	card_instance_t *instance = get_card_instance(player, card);
	int result = orig_cost;
	if( result < 0 ){
		result = 0;
	}
	if( instance->targets[17].card > (1<<11) ){
		int i;
		for(i=20; i<24; i++){
			if( instance->targets[17].card & (1<<i) ){
				result+=2;
			}
		}
		for(i=24; i<28; i++){
			if( instance->targets[17].card & (1<<i) ){
				result+=3;
			}
		}
		if( result > 0 ){
			for(i=12; i<16; i++){
				if( instance->targets[17].card & (1<<i) ){
					result--;
				}
			}
			for(i=16; i<20; i++){
				if( instance->targets[17].card & (1<<i) ){
					result -= 2;
				}
			}
			if( result < 0 ){
				result = 0;
			}
			if (!result && !black && !blue && !green && !red && !white && orig_cost > 0){
				result = 1;
			}
		}
	}
	if( result == 0 && orig_cost < 0 ){
		return orig_cost;
	}
	return result;
}

void real_set_cost_mod_for_activated_abilities(int player, int card, int mode){
	card_instance_t *instance = get_card_instance(player, card);
	int i;
	if( instance->targets[17].card < 0 ){
		instance->targets[17].card = 0;
	}
	for(i=12+((mode-1)*4); i<12+(mode*4); i++){
		if( !( instance->targets[17].card & (1<<i)) ){
			instance->targets[17].card |= (1<<i);
			break;
		}
	}
}

void set_cost_mod_for_activated_abilities(int player, int card, int mode, test_definition_t *this_test){
	// mode = 1 --> -1 colorless mana (min 1)
	// mode = 2 --> -2 colorless mana (min 1)
	// mode = 3 --> +2 colorless mana
	// mode = 4 --> +3 colorless mana
	if( card > -1 ){
		real_set_cost_mod_for_activated_abilities(player, card, mode);
	}
	else{
		int i;
		int test = new_get_test_score(this_test);
		for(i=0; i<2; i++){
			if( i == player || player == 2 ){
				int count = 0;
				while( count < active_cards_count[i] ){
						if( in_play(i, count) && new_make_test_in_play(i, count, test, this_test) ){
							real_set_cost_mod_for_activated_abilities(i, count, mode);
						}
						count++;
				}
			}
		}
	}
}

void real_remove_cost_mod_for_activated_abilities(int player, int card, int mode){
	// mode = 1 --> -1 colorless mana (min 1)
	// mode = 2 --> -2 colorless mana (min 1)
	// mode = 3 --> +2 colorless mana
	// mode = 4 --> +3 colorless mana
	card_instance_t *instance = get_card_instance(player, card);
	int i;
	if( instance->targets[17].card < 0 ){
		instance->targets[17].card = 0;
	}
	for(i=(12+(mode*4))-1; i>(12+((mode-1)*4))-1; i--){
		if( instance->targets[17].card & (1<<i) ){
			instance->targets[17].card &= ~ (1<<i);
			break;
		}
	}
}

void remove_cost_mod_for_activated_abilities(int player, int card, int mode, test_definition_t *this_test){
	// mode = 1 --> -1 colorless mana (min 1)
	// mode = 2 --> -2 colorless mana (min 1)
	// mode = 3 --> +2 colorless mana
	// mode = 4 --> +3 colorless mana
	if( card > -1 ){
		real_remove_cost_mod_for_activated_abilities(player, card, mode);
	}
	else{
		int i;
		int test = new_get_test_score(this_test);
		for(i=0; i<2; i++){
			if( i == player || player == 2 ){
				int count = 0;
				while( count < active_cards_count[i] ){
						if( in_play(i, count) && new_make_test_in_play(i, count, test, this_test) ){
							real_remove_cost_mod_for_activated_abilities(i, count, mode);
						}
						count++;
				}
			}
		}
	}
}

void phantom_effect(int player, int card, event_t event, int requires_counter){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card &&
			damage->damage_target_card == card && damage->damage_target_player == player && damage->info_slot > 0 &&
			(!requires_counter || count_1_1_counters(player, card) > 0)
          ){
			damage->info_slot = 0;
			remove_1_1_counter(player, card);
		}
	}
}

int pick_target_by_type(int player, int card, int t_type, int must_select){

	target_definition_t td;
	default_target_definition(player, card, &td, t_type);
	td.allow_cancel = 1-must_select;

	int result = 0;

	if( t_type == TYPE_CREATURE ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			result = 1;
		}
	}
	else if( t_type == TYPE_LAND ){
			if( pick_target(&td, "TARGET_LAND") ){
				result = 1;
			}
	}
	else if( t_type == TYPE_ENCHANTMENT ){
			if( pick_target(&td, "TARGET_ENCHANTMENT") ){
				result = 1;
			}
	}
	else if( t_type == TYPE_ARTIFACT ){
			if( pick_target(&td, "TARGET_ARTIFACT") ){
				result = 1;
			}
	}
	else{
		if( pick_target(&td, "TARGET_PERMANENT") ){
			result = 1;
		}
	}
	return result;
}

int no_damage_this_turn(int player, int card, event_t event ){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id &&
			source->info_slot > 0 && source->damage_source_player == instance->targets[0].player &&
			source->damage_source_card == instance->targets[0].card
		  ){
			source->info_slot = 0;
		}
	}

	// If effect would end while damage cards are still resolving, instead detach this effect and make it leave play when they're all gone
	if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(player, card)
		&& instance->damage_target_player >= 0
		&& (land_can_be_played & LCBP_PENDING_DAMAGE_CARDS)
      ){
		instance->damage_target_player = instance->damage_target_card = -1;
		event_result = 1;
	}
	if( event == EVENT_AFTER_DAMAGE
		&& instance->damage_target_player == -1
		&& !(land_can_be_played & LCBP_PENDING_DAMAGE_CARDS)
	  ){
		kill_card(player, card, KILL_REMOVE);
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

void negate_damage_this_turn(int player, int card, int t_player, int t_card, int override ){
	create_targetted_legacy_effect(player, card, &no_damage_this_turn, t_player, t_card);
}

int cip_damage_creature(int player, int card, event_t event, target_definition_t *td1, const char *prompt, int amount){

	if( comes_into_play(player, card, event) ){

		if( amount == -1 ){
			if( player != AI ){
				amount = choose_a_number(player, "Deal how much damage?", life[player]);
			}
			else{
				amount = life[player]-6;
				if( amount < 0 ){
					amount = 0;
				}
			}
		}

		if( get_id(player, card) == CARD_ID_VOLCANO_HELLION && amount > 0 ){
			int legacy = create_legacy_effect(player, card, &my_damage_cannot_be_prevented);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[0].player = player;
			leg->targets[0].card = card;
		}
		load_text(0, prompt);
		card_instance_t *instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if (can_target(td1) && select_target(player, card, td1, text_lines[0], &(instance->targets[0]))){
			damage_target0(player, card, amount);

			if (get_id(player, card) == CARD_ID_VOLCANO_HELLION){
				damage_player(player, amount, player, card);
			}
		}
	}

	return 0;
}

int get_base_value( int player, int card ){
	int id = -1;
	if( player == -1 ){
		id = cards_data[card].id;
	}
	else{
		id = get_id(player, card);
	}
	card_ptr_t* c = cards_ptr[ id ];
	/*
	if( c->ai_base_value < 1 ){
		return my_base_value_by_id(id);
	}
	*/
	return c->ai_base_value;
}

// Selects total_damage targets, then deals 1 damage to each for each time it was targeted.  Returns number of different targets selected.
int target_and_divide_damage(int player, int card, target_definition_t* td, const char* prompt, int total_damage)
{
  target_definition_t td_default;
  if (!td)
	{
	  default_target_definition(player, card, &td_default, TYPE_CREATURE);
	  td_default.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	  td_default.allow_cancel = 0;
	  td = &td_default;
	}

  if (!can_target(td))
	return 0;

  if (!prompt)
	prompt = "TARGET_CREATURE_OR_PLAYER";

  int mode = 0;
  if (prompt[0] && prompt[1] >= 'a' && prompt[1] <= 'z')	// at least 2 characters, and 2nd is lowercase
	mode = GS_LITERAL_PROMPT;

  card_instance_t* inst = get_card_instance(player, card);
  inst->number_of_targets = 0;

  int i, failed = 0;;
  for (i = 0; i < total_damage; ++i)
	if (!new_pick_target(td, prompt, i, mode))
	  {
		failed = 1;
		break;
	  }
	else if (inst->targets[i].card >= 0)
	  get_card_instance(inst->targets[i].player, inst->targets[i].card)->state |= STATE_TARGETTED;

  for (i = 0; i < inst->number_of_targets; ++i)
	if (inst->targets[i].card >= 0)
	  get_card_instance(inst->targets[i].player, inst->targets[i].card)->state &= ~STATE_TARGETTED;

  if (failed)
	{
	  inst->number_of_targets = 0;
	  return 0;
	}

  inst->number_of_targets = total_damage;
  int rval = divide_damage(player, card, NULL);
  inst->number_of_targets = 0;

  return rval;
}

// Deals 1 damage to each of {player,card}'s targets for each time it was damaged.
int divide_damage(int player, int card, target_definition_t* td)
{
  card_instance_t* inst = get_card_instance(player, card);

  int i, validated = 0;
  for (i = 0; i < inst->number_of_targets; ++i)
	if (inst->targets[i].player >= 0)
	  {
		// Find any repeats of that target, so we deal the full amount of damage in one packet
		int j, dmg = 1;
		for (j = i + 1; j < inst->number_of_targets; ++j)
		  if (inst->targets[j].card == inst->targets[i].card
			  && inst->targets[j].player == inst->targets[i].player)
			{
			  inst->targets[j].player = inst->targets[j].card = -1;	// mark so it won't be processed again
			  ++dmg;
			}

		if (!td || validate_target(player, card, td, i))
		  {
			++validated;
			damage_creature(inst->targets[i].player, inst->targets[i].card, dmg, player, card);
		  }
		else
		  inst->targets[i].player = inst->targets[i].card = -1;
	  }

  return validated;
}

int check_playable_permanents(int player, int p_type, int mode){
	int count = 0;
	int max_value = -1;
	int result = -1;
	while( count < active_cards_count[player] ){
			if( in_hand(player, count) && is_what(player, count, p_type) ){
				int id = get_id(player, count);
				card_ptr_t* c = cards_ptr[ id ];
				if( has_mana_multi(player, c->req_colorless, c->req_black, c->req_blue, c->req_green,
					c->req_red, c->req_white)
				  ){
					card_instance_t *instance = get_card_instance(player, count);
					if( can_legally_play_iid(player, instance->internal_card_id) ){
						if( mode == 1 ){
							return 1;
						}
						if( c->ai_base_value > max_value ){
							max_value = c->ai_base_value;
							result = count;
						}
					}
				}
			}
			count++;
	}
	if( mode == 1 ){
		result = 0;
	}

	return result;
}

/* Returns the number of times {player,card} can activate for the given life and mana cost, multiplied by pump.  If limit is >= 0, it's an additional constraint
 * on the maximum number of times it can activate (e.g., to deal with a cost involving a sacrifice). */
int generic_shade_amt_can_pump(int player, int card, int pump, int life_to_pay, int cless, int black, int blue, int green, int red, int white, int limit)
{
  if (pump <= 0 || !can_use_activated_abilities(player, card))
	return 0;

  int amt;

  cless = get_cost_mod_for_activated_abilities(player, card, cless, black, blue, green, red, white);

  switch (!!cless + !!black + !!blue + !!green + !!red + !!white)
	{
	  case 0:	// No mana requirement.  Sure hope there's a life requirement or limit is set.
		amt = life[player] / life_to_pay;
		break;

	  case 1:	// Exactly one color of mana
		if (cless)
		  amt = has_mana(player, COLOR_ANY, 1) / cless;
		else if (black)
		  amt = has_mana(player, COLOR_BLACK, 1) / black;
		else if (blue)
		  amt = has_mana(player, COLOR_BLUE, 1) / blue;
		else if (green)
		  amt = has_mana(player, COLOR_GREEN, 1) / green;
		else if (red)
		  amt = has_mana(player, COLOR_RED, 1) / red;
		else // if (white)
		  amt = has_mana(player, COLOR_WHITE, 1) / white;
		break;

	  default:	// Two or more colors of mana.  Have to do this the hard way.
		// First, establish an upper bound.
		// Can't be more than (total mana available) / (total mana per activation)
		;int upper_bound = has_mana(player, COLOR_ANY, 1) / (cless + black + blue + green + red + white);

		// Can't be more than (total mana of a given color available) / (mana of that color per activation)
#define CHECK_COLOR(chg, col) do						\
		  {												\
			if (chg)									\
			  {											\
				int c = has_mana(player, col, 1) / chg;	\
				upper_bound = MIN(upper_bound, c);		\
			  }											\
		  } while (0)

		CHECK_COLOR(black, COLOR_BLACK);
		CHECK_COLOR(blue, COLOR_BLUE);
		CHECK_COLOR(green, COLOR_GREEN);
		CHECK_COLOR(red, COLOR_RED);
		CHECK_COLOR(white, COLOR_WHITE);
#undef CHECK_COLOR

		if (upper_bound <= 0)
		  return 0;

		// Might still be less than that.  Consider an activation cost of |R|G, with one Taiga and eight Swamps in play.

		int lower_bound = 0;

		while (upper_bound != lower_bound)
		  {
			int mid = (upper_bound + lower_bound + 1) / 2;
			if (has_mana_multi(player, cless*mid, black*mid, blue*mid, green*mid, red*mid, white*mid))
			  lower_bound = mid;
			else
			  upper_bound = mid - 1;
		  }

		if (upper_bound <= 0)
		  return 0;

		amt = upper_bound * pump;

		break;
	}

  if (life_to_pay > 0)
	{
	  if (!can_pay_life(player, 1))
		return 0;

	  int amt_life = (life[player] - 1) / life_to_pay;
	  if (amt_life <= 0)
		return 0;
	  else if (amt_life < amt)
		amt = amt_life;
	}

  if (limit >= 0 && amt > limit)
	amt = limit;

  return amt * pump;
}

// If p_pump >= 100, adds p_pump-100 +1/+1 counters, doesn't add power, and ignores t_pump.
int generic_shade(int player, int card, event_t event, int life_to_pay, int cless, int black, int blue, int green, int red, int white, int p_pump, int t_pump, int k_pump, int sp_key_pump)
{
  if (event == EVENT_CAN_ACTIVATE
	  && can_use_activated_abilities(player, card)
	  && has_mana_for_activated_ability(player, card, cless, black, blue, green, red, white)
	  && can_pay_life(player, life_to_pay))
	return 1;

  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, cless, black, blue, green, red, white))
	lose_life(player, life_to_pay);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->parent_controller, c = instance->parent_card;
	  if (in_play(p, c))
		{
		  if (p_pump < 100)
			pump_ability_until_eot(p, c, p, c, p_pump, t_pump, k_pump, sp_key_pump);
		  else
			{
			  int value = p_pump-100;
			  add_1_1_counters(p, c, value);

			  if (k_pump || sp_key_pump)
				  pump_ability_until_eot(p, c, p, c, 0, 0, k_pump, sp_key_pump);
			}
		}
	}

  if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
	{
	  int pump;
	  if (p_pump >= 100)
		pump = p_pump - 100;
	  else
		pump = event == EVENT_POW_BOOST ? p_pump : t_pump;

	  return generic_shade_amt_can_pump(player, card, pump, life_to_pay, cless, black, blue, green, red, white, -1);
	}

  return 0;
}

/* Just like generic_shade(), but can't pump abilities or add counters, and sends its effect through pump_until_eot_merge_previous() so they combine into a
 * single effect card.  Requires that the card's effect text uses |n substitutions. */
int generic_shade_merge_pt(int player, int card, event_t event, int life_to_pay, int cless, int black, int blue, int green, int red, int white, int p_pump, int t_pump)
{
  if (event == EVENT_CAN_ACTIVATE
	  && can_use_activated_abilities(player, card)
	  && has_mana_for_activated_ability(player, card, cless, black, blue, green, red, white)
	  && can_pay_life(player, life_to_pay))
	return 1;

  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, cless, black, blue, green, red, white))
	lose_life(player, life_to_pay);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->parent_controller, c = instance->parent_card;
	  if (in_play(p, c))
		alternate_legacy_text(1, p, pump_until_eot_merge_previous(p, c, p, c, p_pump, t_pump));
	}

  if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
	return generic_shade_amt_can_pump(player, card, event == EVENT_POW_BOOST ? p_pump : t_pump, life_to_pay, cless, black, blue, green, red, white, -1);

  return 0;
}

// If p_pump >= 100, adds p_pump-100 +1/+1 counters, doesn't add power, and ignores t_pump.
int generic_husk(int player, int card, event_t event, int type, int p_pump, int t_pump, int k_pump, int sp_key_pump)
{
  /* Creature
   * Sacrifice a [card type]: ~ gets +[p_pump]/+[t_pump] and gains [k_pump] and [sp_key_pump] until end of turn. */

  if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
	{
	  int pump = event == EVENT_POW_BOOST ? p_pump : t_pump;
	  if (p_pump >= 100)
		pump = p_pump - 100;

	  if (pump <= 0)
		return 0;

	  test_definition_t test;
	  new_default_test_definition(&test, type, "");
	  test.not_me = 1;

	  int num_can_sac = max_can_sacrifice_as_cost(player, card, &test);
	  if (num_can_sac <= 0)
		return 0;

	  return generic_shade_amt_can_pump(player, card, pump, 0, MANACOST0, num_can_sac);
	}

  if (!IS_ACTIVATING(event))
	return 0;

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_ACTIVATE0(player, card) && can_sacrifice_type_as_cost(player, 1, type);

  if (event == EVENT_ACTIVATE
	  && charge_mana_for_activated_ability(player, card, MANACOST0))
	{
	  int sac = controller_sacrifices_a_permanent(player, card, type, SAC_AS_COST | SAC_RETURN_CHOICE);
	  if (!sac)
		cancel = 1;
	  else if (player == AI && BYTE3(sac) == card && BYTE2(sac) == player)
		ai_modifier -= 96;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t *instance = get_card_instance(player, card);
	  int p = instance->parent_controller, c = instance->parent_card;

	  if (in_play(p, c))
		{
		  if (p_pump < 100)
			{
			  if (!k_pump && !sp_key_pump	// can't merge if adding any keywords
				  && (p_pump > 0			// the legal combinations for merging
					  || (p_pump == 0 && t_pump > 0)
					  || (p_pump < 0 && t_pump == 0)))
				pump_until_eot_merge_previous(p, c, p, c, p_pump, t_pump);
			  else
				pump_ability_until_eot(p, c, p, c, p_pump, t_pump, k_pump, sp_key_pump);
			}
		  else
			{
			  add_1_1_counters(p, c, p_pump - 100);

			  if (k_pump || sp_key_pump)
				pump_ability_until_eot(p, c, p, c, 0, 0, k_pump, sp_key_pump);
			}
		}
	}

	return 0;
}

// Like generic_shade, but includes T in activation cost.  Only supports modifications to power or toughness, due to the limited card pool.
int generic_shade_tap(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white, int power, int toughness)
{
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->parent_controller, c = instance->parent_card;
	  if (in_play(p, c))
		pump_ability_until_eot(p, c, p, c, power, toughness, 0, 0);
	}

  if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
	{
	  int pump = event == EVENT_POW_BOOST ? power : toughness;
	  if (pump > 0
		  && generic_shade_tap(player, card, EVENT_CAN_ACTIVATE, cless, black, blue, green, red, white, power, toughness))
		return pump;
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, cless, black, blue, green, red, white, 0, NULL, NULL);
}

int when_attacks_pump_self(int player, int card, event_t event, int power, int toughness)
{
  if (declare_attackers_trigger(player, card, event, 0, player, card))
    pump_until_eot_merge_previous(player, card, player, card, power, toughness);

  if (event == EVENT_POW_BOOST && current_turn == player && current_phase <= PHASE_DECLARE_ATTACKERS
	  && power > 0 && !is_humiliated(player, card))
	return power;

  if (event == EVENT_TOU_BOOST && current_turn == player && current_phase <= PHASE_DECLARE_ATTACKERS
	  && toughness > 0 && !is_humiliated(player, card))
	return toughness;

  return 0;
}

// Only needs to be called during EVENT_GRAVEYARD_FROM_PLAY and TRIGGER_GRAVEYARD_FROM_PLAY.  (More to the point, only need to initialize test then.)
int store_and_trigger_graveyard_from_play(int player, int card, int event, int t_player, int trigger_mode, test_definition_t* test, int triggering_player, int triggering_card)
{
  card_instance_t* aff;
  if (event == EVENT_GRAVEYARD_FROM_PLAY && (affected_card_controller == t_player || t_player == ANYBODY)
	  && (aff = in_play(affected_card_controller, affected_card))
	  && !(test->not_me == 1 && affect_me(player, card))
	  && aff->kill_code > 0 && aff->kill_code < KILL_REMOVE
	  && new_make_test_in_play(affected_card_controller, affected_card, -1, test))
	{
	  card_instance_t* instance = get_card_instance(triggering_player, triggering_card);

	  if (instance->targets[11].player < 0)
		instance->targets[11].player = 0;

	  if (instance->targets[11].player < 10)
		{
		  instance->targets[instance->targets[11].player].player = affected_card_controller;
		  instance->targets[instance->targets[11].player].card = affected_card;
		  instance->targets[11].player++;
		}
	}

  if (trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && triggering_player == reason_for_trigger_controller && affect_me(triggering_player, triggering_card))
	{
	  card_instance_t* instance = get_card_instance(triggering_player, triggering_card);
	  if (instance->targets[11].player > 0)
		{
		  if (event == EVENT_TRIGGER)
			{
			  if (trigger_mode == RESOLVE_TRIGGER_DUH)
				trigger_mode = duh_mode(triggering_player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
			  event_result |= trigger_mode;
			}

		  if (event == EVENT_RESOLVE_TRIGGER)
			{
			  int num = instance->targets[11].player;
			  instance->targets[11].player = 0;
			  return num;
			}

		  if (event == EVENT_END_TRIGGER)
			instance->targets[11].player = 0;
		}
	}

  return 0;
}

static int is_dying(int player, int card){
	if( ! in_play(player, card) ){
		return 0;
	}
	if( check_special_flags2(player, card, SF2_WILL_BE_EXILED_IF_PUTTED_IN_GRAVE) ){
		return 0;
	}
	int kill_code = get_card_instance(player, card)->kill_code;
	if( kill_code < KILL_DESTROY || kill_code > KILL_SACRIFICE ){
		return 0;
	}
	return 1;
}

static void count_for_gfp_ability_and_store_values_impl(int player, int card, int event, int t_player, int type/*optional*/,
														test_definition_t *this_test/*optional*/,
														int(*extra_check)(int player, int card)/*optional*/,
														int mode, int storage_location)
{

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( ! is_dying(affected_card_controller, affected_card) ){
			return;
		}
		if( t_player != 2 && affected_card_controller != t_player){
			return;
		}
		if( this_test && this_test->not_me == 1 && affect_me(player, card) ){
			return;
		}
		if( type && ! is_what(affected_card_controller, affected_card, type) ){
			return;
		}
		if( this_test && ! new_make_test_in_play(affected_card_controller, affected_card, -1, this_test) ){
			return;
		}
		if( extra_check != NULL && ! extra_check(affected_card_controller, affected_card) ){
			return;
		}

		card_instance_t* instance = get_card_instance(player, card);
		if( instance->targets[11].player < 0 ){
			instance->targets[11].player = 0;
		}
		if( ! is_token(affected_card_controller, affected_card) || !(mode & GFPC_EXTRA_SKIP_TOKENS) ){
			if( mode ){
				if( !(mode & GFPC_TRACK_DEAD_CREATURES) ){
					if( instance->targets[storage_location].player < 0){
						instance->targets[storage_location].player = 0;
					}
					if( mode & GFPC_STORE_POWER){ // CARD_ID_KRESH_THE_BLOODBRAIDED
						instance->targets[storage_location].player += get_power(affected_card_controller, affected_card);
					}
					if( mode & GFPC_STORE_TOUGHNESS ){
						instance->targets[storage_location].player += get_toughness(affected_card_controller, affected_card);
					}
					if( mode & GFPC_STORE_CMC  ){
						instance->targets[storage_location].player += get_cmc(affected_card_controller, affected_card);
					}
					if( mode & GFPC_STORE_COLORS){
						instance->targets[storage_location].player |= get_color(affected_card_controller, affected_card);
					}
				}
				else{
					int pos = storage_location+instance->targets[11].player;
					if( pos < 10 ){
						if( t_player == ANYBODY ){
							instance->targets[pos].player = get_owner(affected_card_controller, affected_card);
							instance->targets[pos].card = get_original_internal_card_id(affected_card_controller, affected_card);
						}
						else{
							while( pos < 10 ){
									if( instance->targets[pos].player == -1 ){
										instance->targets[pos].player = get_original_internal_card_id(affected_card_controller, affected_card);
										break;
									}
									if( instance->targets[pos].card == -1 ){
										instance->targets[pos].card = get_original_internal_card_id(affected_card_controller, affected_card);
										break;
									}
									pos++;
							}
						}
					}
				}
			}
			instance->targets[11].player++;
			instance->targets[11].card = 0;
		}
	}
}

void count_for_gfp_ability_and_store_values_extra(int player, int card, int event, int t_player, int type, int(*extra_check)(int player, int card), int mode, int storage_location){
	count_for_gfp_ability_and_store_values_impl(player, card, event, t_player, type, NULL, extra_check, mode, storage_location);
}

void count_for_gfp_ability_and_store_values(int player, int card, int event, int t_player, int type, test_definition_t *this_test/*optional*/, int mode, int storage_location){
	count_for_gfp_ability_and_store_values_impl(player, card, event, t_player, type, this_test, NULL, mode, storage_location);
}

void count_for_gfp_ability(int player, int card, int event, int t_player, int type, test_definition_t *this_test/*optional*/){
	count_for_gfp_ability_and_store_values_impl(player, card, event, t_player, type, this_test, NULL, 0, 0);
}

int resolve_gfp_ability(int player, int card, event_t event, resolve_trigger_t trig_mode){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
		affect_me(player, card )
	  ){
		if(event == EVENT_TRIGGER){
			if (trig_mode == RESOLVE_TRIGGER_DUH){
				trig_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
			}
			event_result |= trig_mode;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int num = instance->targets[11].player;
				instance->targets[11].card = num;	// Squander this data for all cards to maintain backward compatibility
				instance->targets[11].player = 0;
				return num;
		}
		else if (event == EVENT_END_TRIGGER){
				instance->targets[11].player = 0;
		}
	}
	return 0;
}

int count_graveyard_from_play(int player, int card, int event, int trg, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){

	int all_players = 0;
	int not_me = 0;
	int opt_trigger = 0;
	if( trg & 2 ){
		all_players = 1;
	}
	if( trg & 4 ){
		trg-=4;
		not_me = 1;
	}
	if( trg & 8 ){
		trg-=8;
		opt_trigger = 1;
	}

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_GRAVEYARD_FROM_PLAY && (affected_card_controller == trg || all_players == 1) ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( affect_me(player, card) && not_me == 1 ){ return 0; }
		if( make_test_in_play(affected_card_controller, affected_card, -1, type, flag1, subtype, flag2,
								color, flag3, id, flag4, cc, flag5)
		  ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4 ){
				if( instance->targets[11].player < 0 ){
					instance->targets[11].player = 0;
				}
				instance->targets[11].player++;
				if( get_id(player, card) == CARD_ID_KRESH_THE_BLOODBRAIDED || get_id(player, card) == CARD_ID_GRAND_OSSUARY){
					if( instance->targets[1].player < 0 ){
						instance->targets[1].player = 0;
					}
					instance->targets[1].player+=get_power(affected_card_controller, affected_card);
				}
				if( instance->targets[0].card == CARD_ID_SHOWSTOPPER ){
					if( instance->targets[1].player < 2 ){
						instance->targets[1].player = 2;
					}
					int pos = instance->targets[1].player;
					if( pos < 18 ){
						instance->targets[pos].player = get_protections_from(affected_card_controller, affected_card);
					}
				}
			}
		}
	}

	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
		affect_me(player, card )
	  ){
		if(event == EVENT_TRIGGER){
			if( opt_trigger == 1 ){
				event_result |= 1+player;
			}
			else{
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				instance->targets[11].card = instance->targets[11].player;
				instance->targets[11].player = 0;
				return 1;
		}
		else if (event == EVENT_END_TRIGGER){
				instance->targets[11].player = 0;
		}
	}
	return 0;
}

void remove_summoning_sickness(int player, int card ){
	card_instance_t *this = get_card_instance(player, card);
	if( this->state & STATE_SUMMON_SICK ){
		this->state &= ~STATE_SUMMON_SICK;
	}
}

int dont_account_for_trinisphere = 0;
int havengul_lich_hack = 0;
// exactly one of card and iid must be >= 0.  Player must always be valid.
// this could return even negative values, used by Buyback and similar costs modifier.
int true_get_updated_casting_cost(int player, int card, int iid, event_t event, int orig_cless){
	// Seriously?
	/* Plus, this can't account for things like Edgewalker ("Cleric spells you cast cost |W|B less to cast.") or Jade Leech ("Green spells you cast cost G more
	 * to cast.") */
	ASSERT(player == 0 || player == 1 || !"get_updated_casting_cost() needs a real player");
	ASSERT((card < 0) != (iid < 0) && "get_updated_casting_cost() needs exactly one of card and iid to be valid");
	if (iid < 0){
		iid = get_card_instance(player, card)->internal_card_id;
	}
	if( cards_data[iid].type & TYPE_LAND ){
		return 0;
	}
	int csvid = cards_data[iid].id;
	int cless = orig_cless;

	card_ptr_t* c = cards_ptr[ csvid ];
	if( cless == -1 ){
		cless = c->req_colorless;
	}
	if( cless > 15 || cless == -1 ){
		cless = 0;
	}

	int trinisphere_flag = 0;
	int clr = get_color_by_internal_id(player, iid);
	int i;
	int checked_pl = 0;
	for(i=0; i<2; i++){
		card_instance_t* instance;
		int count = 0;
		while(count < active_cards_count[i] ){
			if( (instance = in_play(i, count)) && (is_what(i, count, TYPE_EFFECT) || ! is_humiliated(i, count)) ){
				int this_csvid = get_id(i, count);
				switch( this_csvid ){
						case CARD_ID_SPECIAL_EFFECT:
						{
							int flags = instance->targets[0].card;
							if( (flags & SE_CSVID_CANT_BE_PLAYED) || (flags & SE_TYPE_CANT_BE_PLAYED) ){
								if( player == instance->targets[0].player || instance->targets[0].player == ANYBODY ){
									if( flags & SE_CSVID_CANT_BE_PLAYED ){
										if( cards_data[iid].id == instance->targets[1].player ){
											return 99;
										}
									}
									if( (flags & SE_TYPE_CANT_BE_PLAYED) && instance->targets[1].player > -1 ){
										if( is_what(-1, iid, instance->targets[1].player) ){
											return 99;
										}
									}
								}
							}
							break;
						}

						case CARD_ID_ANGELIC_ARBITER:
						{
							if( instance->targets[1].card > 0 && (instance->targets[1].card & 1) && i != player ){
								return 99;
							}
							break;
						}

						case CARD_ID_ANIMAR_SOUL_OF_ELEMENTS:
						{
							if( i == player && is_what(-1, iid, TYPE_CREATURE) ){
								cless-=count_1_1_counters(i, count);
							}
							break;
						}

						case CARD_ID_ARCANE_LABORATORY:
						case CARD_ID_RULE_OF_LAW:
						{
							if( instance->targets[1].player > 0 && (instance->targets[1].player & (1+player)) ){
								return 99;
							}
							break;
						}

						case CARD_ID_ARCANE_MELEE:
						{
							if( ! is_what(-1, iid, TYPE_CREATURE) && is_what(-1, iid, TYPE_SPELL) ){
								cless-=2;
							}
							break;
						}

						case CARD_ID_AURA_OF_SILENCE:
						{
							if( i != player && is_what(-1, iid, TYPE_ARTIFACT | TYPE_ENCHANTMENT) ){
								cless+=2;
							}
							break;
						}

						case CARD_ID_BALLYRUSH_BANNERET:
						{
							if( i == player && (has_subtype_by_id(csvid, SUBTYPE_SOLDIER) || has_subtype_by_id(csvid, SUBTYPE_KITHKIN)) ){
								cless--;
							}
							break;
						}

						case CARD_ID_BASANDRA_BATTLE_SERAPH:
						{
							if( current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 ){
								return 99;
							}
							break;
						}

						case CARD_ID_BOSK_BANNERET:
						{
							if( i == player && (has_subtype_by_id(csvid, SUBTYPE_TREEFOLK) || has_subtype_by_id(csvid, SUBTYPE_SHAMAN)) ){
								cless--;
							}
							break;
						}

						case CARD_ID_BRAND_OF_ILL_OMEN:
						{
							if( player == instance->damage_target_player && is_what(-1, iid, TYPE_CREATURE)){
								return 99;
							}
							break;
						}

						case CARD_ID_BRIGHTHEARTH_BANNERET:
						{
							if( i == player && (has_subtype_by_id(csvid, SUBTYPE_ELEMENTAL) || has_subtype_by_id(csvid, SUBTYPE_WARRIOR)) ){
								cless--;
							}
							break;
						}

						case CARD_ID_CENTAUR_OMENREADER:
						{
							if( is_tapped(i, count) && i == player && is_what(-1, iid, TYPE_CREATURE) ){
								cless-=2;
							}
							break;
						}

						case CARD_ID_CLOUD_KEY:
						{
							if( instance->targets[1].card > 0 && is_what(-1, iid, instance->targets[1].card) ){
								cless--;
							}
							break;
						}

						case CARD_ID_COUNCIL_OF_THE_ABSOLUTE:
						{
							if( get_card_instance(i, count)->targets[1].card == csvid){
								if( i == player ){
									cless-=2;
								}
								else{
									return 99;
								}
							}
							break;
						}

						case CARD_ID_CURSE_OF_EXHAUSTION:
						{
							if( player == instance->targets[0].player && instance->targets[1].player == 66 ){
								return 49;
							}
							break;
						}

						case CARD_ID_DEFENSE_GRID:
						{
							if( current_turn != player ){
								cless+=3;
							}
							break;
						}

						case CARD_ID_DRAGONSPEAKER_SHAMAN:
						{
							if( i == player && has_subtype_by_id(csvid, SUBTYPE_DRAGON) ){
								cless-=2;
							}
							break;
						}

						case CARD_ID_ELDER_DRAGON_HIGHLANDER:
						{
							if( instance->targets[5].card == iid){
								cless += count_counters(i, count, COUNTER_ENERGY);
							}
							break;
						}

						case CARD_ID_EMERALD_MEDALLION:
						{
							if( i == player && (clr & COLOR_TEST_GREEN) ){
								cless--;
							}
							break;
						}

						case CARD_ID_ETHERIUM_SCULPTOR:
						{
							if( is_what(-1, iid, TYPE_ARTIFACT) && i == player ){
								cless--;
							}
							break;
						}

						case CARD_ID_ETHERSWORN_CANONIST:
						{
							if( ! is_what(-1, iid, TYPE_ARTIFACT) ){
								if( instance->targets[1].player > 0 && (instance->targets[1].player & (1+player)) ){
									return 99;
								}
							}
							break;
						}

						case CARD_ID_EXCLUSION_RITUAL:
						{
							if( instance->targets[1].card == csvid ){
								return 99;
							}
							break;
						}

						case CARD_ID_EYE_OF_UGIN:
						{
							if( i == player && has_subtype_by_id(csvid, SUBTYPE_ELDRAZI) && !(clr & COLOR_TEST_ANY_COLORED) ){
								cless-=2;
							}
							break;
						}

						case CARD_ID_FROGTOSSER_BANNERET:
						{
							if( i == player && (has_subtype_by_id(csvid, SUBTYPE_GOBLIN) || has_subtype_by_id(csvid, SUBTYPE_ROGUE)) ){
								cless--;
							}
							break;
						}

						case CARD_ID_GADDOCK_TEEG:
						{
							if( ! is_what(-1, iid, TYPE_CREATURE) ){
								if( get_cmc_by_id(csvid) > 3 ){
									return 99;
								}
							}
							break;
						}

						case CARD_ID_GLOOM:
						{
							if( (clr & COLOR_TEST_WHITE)  ){
								cless+=3;
							}
							break;
						}

						case CARD_ID_GOBLIN_ELECTROMANCER:
						{
							if( i == player && ! is_what(-1, iid, TYPE_CREATURE) && is_what(-1, iid, TYPE_SPELL) ){
								cless--;
							}
							break;
						}

						case CARD_ID_GOBLIN_WARCHIEF:
						{
							if( i == player && has_subtype_by_id(csvid, SUBTYPE_GOBLIN) ){
								cless--;
							}
							break;
						}

						case CARD_ID_GRAND_ABOLISHER:
						{
							if( i != player && current_turn == i ){
								return 99;
							}
							break;
						}

						case CARD_ID_GRAND_ARBITER_AUGUSTIN_IV:
						{
							if( i == player ){
								cless--;
							}
							else{
								cless++;
							}
							break;
						}

						case CARD_ID_GRID_MONITOR:
						case CARD_ID_STEEL_GOLEM:
						{
							if( i == player && is_what(-1, iid, TYPE_CREATURE) ){
								return 49;
							}
							break;
						}

						case CARD_ID_HEARTLESS_SUMMONING:
						{
							if( i == player && is_what(-1, iid, TYPE_CREATURE) ){
								cless-=2;
							}
							break;
						}

						case CARD_ID_HELM_OF_AWAKENING:
							cless--;
							break;

						case CARD_ID_HERALD_OF_WAR:
						{
							if( i == player && (has_subtype_by_id(csvid, SUBTYPE_ANGEL) || has_subtype_by_id(csvid, SUBTYPE_HUMAN)) ){
								cless-=count_1_1_counters(i, count);
							}
							break;
						}

						case CARD_ID_HERO_OF_IROAS:
						{
							if( i == player && has_subtype(-1, iid, SUBTYPE_AURA) && ! is_humiliated(i, count) ){
								--cless;
							}
							break;
						}

						case CARD_ID_IN_THE_EYE_OF_CHAOS:
						{
							if( is_what(-1, iid, TYPE_INSTANT) && ! is_what(-1, iid, TYPE_CREATURE) ){
								cless+=get_cmc_by_id(csvid);
							}
							break;
						}

						case CARD_ID_IONA_SHIELD_OF_EMERIA:
						{
							if( i == 1-player && ! is_what(-1, iid, TYPE_LAND) ){
								if( clr & instance->targets[10].card ){
									return 49;
								}
							}
							break;
						}

						/* Semi-general case.  Checks against player need to be handled both here and specially in the
						 * card function; and the card function shouldn't do anything with affected_card_controller
						 * except check it with affect_me() or pass it on to functions that explicitly understand the
						 * (-1,iid) convention.  Usually this means checking EVENT_MODIFY_COST_GLOBAL first and
						 * explicitly returning at the end.
						 * Simply checking affected_card_controller == player in the card function WILL FAIL.
						 * Calling most functions that look at affected_card_controller WILL CRASH. */

						// Spells your opponents cast cost more/cost less/can't be played
						case CARD_ID_ALHAMMARRET_HIGH_ARBITER:
						case CARD_ID_DRAGONLORD_DROMOKA:
						case CARD_ID_VOID_WINNOWER:
							if (player == i){
								break;
							}
							checked_pl = 1;
							// and fall through

						// Spells you cast cost more/cost less/can't be played.  See above for usage.
						case CARD_ID_ARCHENEMY:
						case CARD_ID_CONDUIT_OF_RUIN:
						case CARD_ID_DRAGONLORDS_SERVANT:
						case CARD_ID_GEIST_FUELED_SCARECROW:
						case CARD_ID_HERALD_OF_KOZILEK:
						case CARD_ID_HERALD_OF_THE_PANTHEON:
						case CARD_ID_JACES_SANCTUM:
						case CARD_ID_KRALLENHORDE_HOWLER:
						case CARD_ID_MIZZIX_OF_THE_IZMAGNUS:
						case CARD_ID_PLANAR_GATE:
						case CARD_ID_SEAL_OF_THE_GUILDPACT:
							if (player != i && !checked_pl){
								break;
							}
							checked_pl = 1;
							// and fall through

						// Spells anybody casts cost more/cost less/can't be played.  See above for usage.
						case CARD_ID_FEROZS_BAN:
						case CARD_ID_IRINI_SENGIR:
						{
							int old_cost_colorless = COST_COLORLESS;
							int old_affected_card_controller = affected_card_controller;
							int old_affected_card = affected_card;
							int old_event_result = event_result;

							event_result = player;
							if (card >= 0){
								affected_card_controller = player;
								affected_card = card;
							} else {
								affected_card_controller = -1;
								affected_card = iid;
							}

							call_card_function(i, count, EVENT_MODIFY_COST_GLOBAL);

							affected_card_controller = old_affected_card_controller;
							affected_card = old_affected_card;
							event_result = old_event_result;

							cless += COST_COLORLESS - old_cost_colorless;
							COST_COLORLESS = old_cost_colorless;
							break;
						}

						case CARD_ID_JET_MEDALLION:
						{
							if( i == player && (clr & COLOR_TEST_BLACK) ){
								cless--;
							}
							break;
						}

						case CARD_ID_LOCKET_OF_YESTERDAYS:
						{
							if( i == player ){
								cless-=count_graveyard_by_id(player, csvid);
							}
							break;
						}

						case CARD_ID_LODESTONE_GOLEM:
						{
							if( ! is_what(-1, iid, TYPE_ARTIFACT) ){
								cless++;
							}
							break;
						}

						case CARD_ID_MANA_MATRIX:
						{
							if( i == player && ( is_what(-1, iid, TYPE_INSTANT) || is_what(-1, iid, TYPE_ENCHANTMENT) ) ){
								cless-=2;
							}
							break;
						}

						case CARD_ID_MEDDLING_MAGE:
						case CARD_ID_NEVERMORE:
						{
							if( instance->targets[1].card == csvid ){
								return 49;
							}
							break;
						}

						case CARD_ID_MYCOSYNTH_GOLEM:
						{
							if( is_what(-1, iid, TYPE_ARTIFACT) && is_what(-1, iid, TYPE_CREATURE) ){
								cless-=count_subtype(player, TYPE_ARTIFACT, -1);
							}
							break;
						}

						case CARD_ID_NIGHTSCAPE_FAMILIAR:
						{
							if( i == player && (clr & get_sleighted_color_test(i, count, COLOR_TEST_BLUE | COLOR_TEST_RED)) ){
								cless--;
							}
							break;
						}

						case CARD_ID_OMNISCIENCE:
						{
							if( i == player){
								cless-=c->req_colorless;
							}
							break;
						}

						case CARD_ID_PEARL_MEDALLION:
						{
							if( i == player && (clr & COLOR_TEST_WHITE) ){
								cless--;
							}
							break;
						}

						case CARD_ID_RAKDOS_LORD_OF_RIOTS:
						{
							if( i == player && is_what(-1, iid, TYPE_CREATURE) ){
								cless-=get_trap_condition(1-player, TRAP_LIFE_LOST);
							}
							break;
						}

						case CARD_ID_ROOFTOP_STORM:
						{
							if( i == player && has_subtype_by_id(csvid, SUBTYPE_ZOMBIE) ){
								cless-=c->req_colorless;
							}
							break;
						}

						case CARD_ID_RUBY_MEDALLION:
						{
							if( i == player && (clr & COLOR_TEST_RED) ){
								cless--;
							}
							break;
						}

						case CARD_ID_SAPPHIRE_MEDALLION:
						{
							if( i == player && (clr & COLOR_TEST_BLUE) ){
								cless--;
							}
							break;
						}

						case CARD_ID_SEMBLANCE_ANVIL:
						{
							if( i == player ){
								if (is_what(-1, iid, instance->info_slot)){
									cless -= 2;
								}
							}
							break;
						}

						case CARD_ID_SPHERE_OF_RESISTANCE:
						{
							cless++;
							break;
						}

						case CARD_ID_STINKDRINKER_DAREDEVIL:
						{
							if( i == player && has_subtype_by_id(csvid, SUBTYPE_GIANT) ){
								cless-=2;
							}
							break;
						}

						case CARD_ID_STONE_CALENDAR:
						{
							if( i == player ){
								cless--;
							}
							break;
						}

						case CARD_ID_STONYBROOK_BANNERET:
						{
							if( i == player && (has_subtype_by_id(csvid, SUBTYPE_MERFOLK) || has_subtype_by_id(csvid, SUBTYPE_WIZARD)) ){
								cless--;
							}
							break;
						}

						case CARD_ID_STORMSCAPE_FAMILIAR:
						{
							if( i == player && (clr & get_sleighted_color_test(i, count, COLOR_TEST_WHITE | COLOR_TEST_BLACK)) ){
								cless--;
							}
							break;
						}

						case CARD_ID_SUNSCAPE_FAMILIAR:
						{
							if( i == player && (clr & get_sleighted_color_test(i, count, COLOR_TEST_GREEN | COLOR_TEST_BLUE)) ){
								cless--;
							}
							break;
						}

						case CARD_ID_TEFERI_MAGE_OF_ZHALFIR:
						{
							if( i != player && ! can_sorcery_be_played(player, event) ){
								return 49;
							}
							break;
						}

						case CARD_ID_THALIA_GUARDIAN_OF_THRABEN:
						{
							if( i != player && ! is_what(-1, iid, TYPE_CREATURE) ){
								cless++;
							}
							break;
						}

						case CARD_ID_THORNSCAPE_FAMILIAR:
						{
							if( i == player && (clr & get_sleighted_color_test(i, count, COLOR_TEST_RED | COLOR_TEST_WHITE)) ){
								cless--;
							}
							break;
						}

						case CARD_ID_THUNDERSCAPE_FAMILIAR:
						{
							if( i == player && (clr & get_sleighted_color_test(i, count, COLOR_TEST_BLACK | COLOR_TEST_GREEN)) ){
								cless--;
							}
							break;
						}

						case CARD_ID_TRINISPHERE:
							trinisphere_flag = 1;
							break;

						case CARD_ID_THORN_OF_AMETHYST:
						case CARD_ID_VRYN_WINGMARE:
						{
							if( ! is_what(-1, iid, TYPE_CREATURE) ){
								cless++;
							}
							break;
						}

						case CARD_ID_UNDEAD_WARCHIEF:
						{
							if( i == player && has_subtype_by_id(csvid, SUBTYPE_ZOMBIE) ){
								cless--;
							}
							break;
						}

						case CARD_ID_VOIDSTONE_GARGOYLE:
						{
							if( instance->targets[1].card == csvid ){
								return 99;
							}
							break;
						}

						case CARD_ID_WARD_OF_BONES:
						{
							if( i != player ){
								if( instance->targets[1].card == 66 && is_what(-1, iid, TYPE_CREATURE) ){
									return 49;
								}
								else if( instance->targets[2].card == 66 && is_what(-1, iid, TYPE_ARTIFACT) ){
										return 49;
								}
								else if( instance->targets[3].card == 66 && is_what(-1, iid, TYPE_ENCHANTMENT) ){
										return 49;
								}
							}
							break;
						}

						case CARD_ID_OBSCURING_ETHER:
						case CARD_ID_DREAM_CHISEL:
						{
							if( i == player && cards_data[iid].id == CARD_ID_FACE_DOWN_CREATURE ){
								--cless;
							}
							break;
						}

						default:
							break;
				}
			}
			count++;
		}
	}

	if( trinisphere_flag == 1 && cless < 3 && dont_account_for_trinisphere <= 0 ){
		if( orig_cless == -1 ){
			int total_cmc = c->req_black + c->req_blue + c->req_green + c->req_red + c->req_white;
			if( total_cmc < 3 ){
				cless = 3-total_cmc;
			}
		}
		else{
			cless = 3-orig_cless;
		}
	}
	cless+=havengul_lich_hack;

	return cless;
}

// This is a frontend and prevent the return of negative values when charging mana
int get_updated_casting_cost(int player, int card, int iid, event_t event, int cless){
	return MAX(0, true_get_updated_casting_cost(player, card, iid, event, cless));
}

int is_phyrexian(int id){
	if( id == CARD_ID_ACT_OF_AGGRESSION ||
		id == CARD_ID_APOSTLES_BLESSING ||
		id == CARD_ID_BIRTHING_POD ||
		id == CARD_ID_CATHEDRAL_MEMBRANE ||
		id == CARD_ID_CORROSIVE_GALE ||
		id == CARD_ID_DISMEMBER ||
		id == CARD_ID_GITAXIAN_PROBE ||
		id == CARD_ID_GUT_SHOT ||
		id == CARD_ID_MARROW_SHARDS ||
		id == CARD_ID_MENTAL_MISSTEP ||
		id == CARD_ID_MOLTENSTEEL_DRAGON ||
		id == CARD_ID_MUTAGENIC_GROWTH ||
		id == CARD_ID_NORNS_ANNEX ||
		id == CARD_ID_NOXIOUS_REVIVAL ||
		id == CARD_ID_PHYREXIAN_METAMORPH ||
		id == CARD_ID_PITH_DRILLER ||
		id == CARD_ID_PORCELAIN_LEGIONNAIRE ||
		id == CARD_ID_POSTMORTEM_LUNGE ||
		id == CARD_ID_RAGE_EXTRACTOR ||
		id == CARD_ID_RUTHLESS_INVASION ||
		id == CARD_ID_SLASH_PANTHER ||
		id == CARD_ID_SPINED_THOPTER ||
		id == CARD_ID_SURGICAL_EXTRACTION ||
		id == CARD_ID_TEZZERETS_GAMBIT ||
		id == CARD_ID_THUNDERING_TANADON ||
		id == CARD_ID_VAULT_SKIRGE
	  ){
		return 1;
	}
	return 0;
}

int is_id_in_grave(int player, int id){
	const int *grave = get_grave(player);
	int count = count_graveyard(player)-1;
	while( count > -1 ){
			if( cards_data[grave[count]].id == id ){
				return 1;
			}
			count--;
	}
	return 0;
}

int is_id_in_hand(int player, int id){
	int count = 0;
	while( count < active_cards_count[player] ){
			if( in_hand(player, count) && get_id(player, count) == id ){
				return 1;
			}
			count++;
	}
	return 0;
}

#define C(ch) ((ch) == 'B' || (ch) == 'U' || (ch) == 'G' || (ch) == 'R' || (ch) == 'W')
// These are fragile, but better than what we had before.
int is_monocolor_hybrid(card_ptr_t* cp)
{
  // All existing monocolor hybrid casting costs start with "|2C", where C is a color; no other cards do.
  const char* c = cp->mana_cost_text;
  return c && c[0] == '|' && c[1] == '2' && C(c[2]);
}

int is_normal_hybrid(card_ptr_t* cp)
{
  // All existing normal hybrid casting costs end with "Cx", where C is a color and x is any character; no other cards do.
  const char* c = cp->mana_cost_text;
  int len;
  return c && (len = strlen(c)) >= 3 && C(c[len-2]);
}

int is_mixed_hybrid(card_ptr_t* cp)
{
  // All existing mixed hybrid/non-hybrid casting costs end with "|xx|x", where x is any character; no other cards do.
  const char* c = cp->mana_cost_text;
  int len;
  return c && (len = strlen(c)) >= 5 && c[len-2] == '|' && c[len-5] == '|';
}
#undef C


#define USE_HOPELESSLY_BROKEN 1

#ifdef USE_HOPELESSLY_BROKEN
int has_mana_to_cast_iid(int player, event_t event, int iid)
{
  /****************************************************************************************************************
  * Any further special cases for specific cards are unacceptable in this function and will be reverted on sight. *
  ****************************************************************************************************************/
  int csvid = cards_data[iid].id;
  if(csvid==CARD_ID_REAPER_KING)return can_cast_reaper_king(player,-1,event);

  card_ptr_t* cp = cards_ptr[csvid];
  int orig_cless = cp->req_colorless;
  if (orig_cless == 40)
	orig_cless = 0;

  int cless = get_updated_casting_cost(player, -1, iid, event, orig_cless);
  if(csvid==CARD_ID_KARADOR_GHOST_CHIEFTAIN){cless-=count_graveyard_by_type(player,TYPE_CREATURE);if(cless<0)cless=0;}
  int is_artifact = cards_data[iid].type & TYPE_ARTIFACT;
  if (is_phyrexian(csvid))
	return has_phyrexian_mana(player, cless, cp->req_black, cp->req_blue, cp->req_green, cp->req_red, cp->req_white, is_artifact);
  else if (is_normal_hybrid(cp))
	{
	  int c1 = get_colors_for_hybrid(-1, iid, 0, 0);
	  int c2 = get_colors_for_hybrid(-1, iid, c1, 0);
	  return has_mana_hybrid(player, get_number_of_hybrid_mana(-1, iid, 0, 0), c1, c2, cless);
	}
  else if (is_monocolor_hybrid(cp))
	{
	  int c1 = get_color_for_monocolored_hybrid(-1, iid);
	  return has_mana_for_monocolor_hybrid(player, get_number_of_monocolored_hybrid_mana(-1, iid, 0), c1,  0);
	}
#if 0
  else if (is_mixed_hybrid(cp))
	{
	  // I can't bring myself to care.
	}
#endif
  else if (is_artifact)
	return has_mana_multi_a(player, cless, cp->req_black, cp->req_blue, cp->req_green, cp->req_red, cp->req_white);
  else
	return has_mana_multi(player, cless, cp->req_black, cp->req_blue, cp->req_green, cp->req_red, cp->req_white);
}

int charge_mana_from_id(int player/*to_charge*/, int card/*to_charge_for - optional; set to -1 to omit*/, event_t event, int csvid)
{
  /****************************************************************************************************************
  * Any further special cases for specific cards are unacceptable in this function and will be reverted on sight. *
  ****************************************************************************************************************/

  if (csvid < 0)
	csvid = get_id(player, card);

  int iid;
  if (card == -1)
	iid = get_internal_card_id_from_csv_id(csvid);
  else
	{
	  iid = get_card_instance(player, card)->internal_card_id;
	  ASSERT(cards_data[iid].id == csvid);
	}

  if(csvid==CARD_ID_REAPER_KING){int y=card>-1?card:add_card_to_hand(player,iid);int z=casting_reaper_king(player,y,event);if(card<0)obliterate_card(player,y);return z;}

  card_ptr_t* cp = cards_ptr[csvid];
  int orig_cless = cp->req_colorless;
  if (orig_cless == 40)
	orig_cless = 0;

  int is_artifact = cards_data[iid].type & TYPE_ARTIFACT;
  int rval;
  int card_added = -1;

#define ADD_CARD()													\
  if (card < 0)														\
	{																\
	  card = card_added = add_card_to_hand(player, iid);			\
	  get_card_instance(player, card)->state |= STATE_INVISIBLE;	\
	  --hand_count[player];											\
	}

  if (is_phyrexian(csvid))
	{
	  // This both accounts for changed casting cost itself, and needs a real card.
	  ADD_CARD();
	  rval = charge_phyrexian_mana(player, card, event, orig_cless, cp->req_black, cp->req_blue, cp->req_green, cp->req_red, cp->req_white, is_artifact);
	}
  else
	{
	  int cless;
	  if (card >= 0)
		cless = get_updated_casting_cost(player, card, -1, event, orig_cless);
	  else
		cless = get_updated_casting_cost(player, -1, iid, event, orig_cless);

	  if (is_normal_hybrid(cp))
		{
		  ADD_CARD();
		  int c1 = get_colors_for_hybrid(player, card, 0, 0);
		  int c2 = get_colors_for_hybrid(player, card, c1, 0);
		  rval = charge_mana_hybrid(player, card, get_number_of_hybrid_mana(player, card, 0, 0), c1, c2, cless);
		}
	  else if (is_monocolor_hybrid(cp))
		{
		  ADD_CARD();
		  int c1 = get_color_for_monocolored_hybrid(player, card);
		  rval = charge_mana_for_monocolor_hybrid(player, card, get_number_of_monocolored_hybrid_mana(player, card, 0), c1, cless);
		}
#if 0
	  else if (is_mixed_hybrid(cp))
		{
		  // I still can't bring myself to care.
		}
#endif
	  else
		{
		  if (csvid==CARD_ID_KARADOR_GHOST_CHIEFTAIN){cless-=count_graveyard_by_type(player,TYPE_CREATURE);if(cless<0)cless=0;}

		  int charge_x = 0;
		  if (cards_data[iid].cc[1] == 255)	// a more reliable check than orig_cless == 40; cards that naughtily charge X themselves set orig_cless == 40 so X appears in displayed mana cost but cc[1] == 0 so it's not charged by the engine
			{
			  if (cless == 0)
				cless = -1;	// can just do it in the initial mana call
			  else
				charge_x = 1;
			}

		  rval = charge_mana_multi_a(player, is_artifact ? 0 : cless, cp->req_black, cp->req_blue, cp->req_green, cp->req_red, cp->req_white, is_artifact ? cless : 0);

		  if (rval && charge_x)
			rval = charge_mana_multi_a(player, is_artifact ? 0 : -1, 0,0,0,0,0, is_artifact ? -1 : 0);
		}
	}

  if (card_added >= 0)
	obliterate_card(player, card_added);

  return rval;
}
#elif defined(USE_EXE_VERSION)
/* These are almost right, but incompatible with the way alternative costs are defined - which is to force mana cost to either 0 or infinite during cost
 * modification, then charge for real during EVENT_CAST_SPELL.  Which is unqueryable and unsustainable.  Not to mention that they then try to call *this* if you
 * choose to decline the alternative costs.
 *
 * The right way is to use cost modification only for *cost modification*, and then have something else that's called for alternative costs.
 *
 * Besides which, at least half of the calls to this are followed by play_spell_for_free().  Such calls should just put the card on the stack normally and
 * charge normally. */
int has_mana_to_cast_iid(int player, event_t event, int iid)
{
  int added = add_card_to_hand(player, iid);
  card_instance_t* inst = get_card_instance(player, added);
  inst->state |= STATE_INVISIBLE;
  --hand_count[player];
  int rval = EXE_FN(int, 0x402930, int, int, int)(player, 0, added);
  obliterate_card_and_recycle(player, added);
  return rval;
}

//global _charge_mana_to_cast
//_charge_mana_to_cast:
//	push	ebp
//	mov	ebp, esp
//	push	edi
//	push	dword [ebp+0x10]	; card - param3
//	push	dword [ebp+0x0c]	; player - param2
//	mov	edi, [ebp+0x08]		; card_data_t* - param1
//	call	0x402680
//	add	esp, 2*4		; pop 2 params
//	pop	edi
//	leave
//	ret

int charge_mana_from_id(int player, int card, event_t event, int csvid)
{
  int iid = get_internal_card_id_from_csv_id(csvid);
  int added = add_card_to_hand(player, iid);
  card_instance_t* inst = get_card_instance(player, added);
  inst->state |= STATE_INVISIBLE;
  --hand_count[player];
  EXE_FN(int, 0x402930, int, int, int)(player, 0, added);
  int rval = charge_mana_multi(player, COST_COLORLESS, COST_BLACK, COST_BLUE, COST_GREEN, COST_RED, COST_WHITE);
  obliterate_card(player, added);
  return rval;
}
#endif

int has_mana_to_cast_id(int player, event_t event, int csvid){
	return has_mana_to_cast_iid(player, event, get_internal_card_id_from_csv_id(csvid));
}

void remove_id_from_grave(int player, int csvid, remove_id_from_grave_t mode){
	const int *grave = get_grave(player);
	int count = count_graveyard(player)-1;
	while( count > -1 ){
			if( cards_data[grave[count]].id == csvid ){
				if( mode & RIFG_OBLITERATE ){
					remove_card_from_grave(player, count);
				}
				else{
					rfg_card_from_grave(player, count);
				}
				if( !(mode & RIFG_REMOVE_ALL) ){
					break;
				}
			}
			count--;
	}
}

int already_dead(int player, int card){
	if( check_special_flags(player, card, SF_LETHAL_DAMAGE_DESTROY) ||
		check_special_flags(player, card, SF_LETHAL_DAMAGE_BURY) ||
		check_special_flags(player, card, SF_LETHAL_DAMAGE_EXILE) ||
		get_card_instance(player, card)->damage_on_card >= get_toughness(player, card)
	  ){
		return 1;
	}
	return 0;
}

void gain_life(int player, int amount){
	int false_cure_flag = 0;
	int life_loss_flag = 0;
	if( amount <= 0 ){
		return;
	}
	int original_amount = amount;

	// First search for card / effect that modify the amount of life gained
	int i;
	for(i=0; i<2; i++){
		int count = active_cards_count[i]-1;
		while( count > -1 ){
			if( in_play(i, count) ){
				if( is_what(i, count, TYPE_PERMANENT) ){
					int id = get_id(i, count);
					if( id == CARD_ID_FORSAKEN_WASTES || id == CARD_ID_LEYLINE_OF_PUNISHMENT ||
						id == CARD_ID_EVERLASTING_TORMENT || id == CARD_ID_SULFURIC_VORTEX || id == CARD_ID_HAVOC_FESTIVAL ||
						id == CARD_ID_WITCH_HUNT
						){
						return;
					}
					if( i == player && (id == CARD_ID_BOON_REFLECTION || id == CARD_ID_RHOX_FAITHMENDER) ){
						amount *=2;
					}
					if( id == CARD_ID_RAIN_OF_GORE || (id == CARD_ID_TAINTED_REMEDY && i != player) ){
						life_loss_flag = 1;	// only gets replaced once, even if there's multiple copies
					}
					if( i != player && (id == CARD_ID_EREBOS_GOD_OF_THE_DEAD || id == CARD_ID_EREBOS_GOD_OF_THE_DEAD_INCARNATE)){
						return;
					}
					if( i == player && id == CARD_ID_PLATINUM_EMPERION ){
						return;
					}
				}
				else if( is_what(i, count, TYPE_EFFECT) ){
					card_instance_t *instance = get_card_instance(i, count);
					if( instance->internal_card_id == LEGACY_EFFECT_CUSTOM ){
						switch (instance->display_pic_csv_id){
							case CARD_ID_FALSE_CURE:
								false_cure_flag++;
								break;
							case CARD_ID_FLAMES_OF_THE_BLOOD_HAND:
							case CARD_ID_STIGMA_LASHER:
								if (instance->targets[3].player == player){
									return;
								}
								break;
							case CARD_ID_SKULLCRACK:
								return;
							case CARD_ID_ATARKAS_COMMAND:
								if (i != player && BYTE0(instance->eot_toughness) == 1){
									return;
								}
								break;
						}
					}
				}
			}
			if( amount == 0 ){
				return;
			}
			count--;
		}
	}

	if (life_loss_flag){
		/* Even if the player has a Boon Reflection - the affected player chooses the order of replacement effects, so presumeably applies the life-losing
		 * replacement effect first instead of the doubling one */
		lose_life(player, original_amount);
		return;
	}

	// Then, if amount > 0, search for cards that uses this quantity
	// All of these are better done by triggering on TRIGGER_GAIN_LIFE.

	int at_count = 0;
	int nl_count = count_cards_by_id(player, CARD_ID_NEFARIOUS_LICH);
	if( nl_count > 0 && amount > 0){
		draw_cards(player, amount*nl_count);
		amount = 0;
	}

	if( amount > 0 ){
		i = 0;
		for(i=0; i<2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
				if( in_play(i, count) ){
					if( is_what(i, count, TYPE_PERMANENT) && ! is_humiliated(i, count) ){
						int id = get_id(i, count);
						if( i == player && id == CARD_ID_AGELESS_ENTITY ){
							add_1_1_counters(i, count, amount);
						}
						if( i != player && id == CARD_ID_KAVU_PREDATOR ){
							add_1_1_counters(i, count, amount);
						}
						if( i == player && id == CARD_ID_WELL_OF_LOST_DREAMS &&
							has_mana(i, COLOR_COLORLESS, 1)
							){
							int choice = do_dialog(i, i, count, -1, -1, " Activate the Well\n Pass", 0);
							if( choice == 0 ){
								int old_max_x_value = max_x_value;
								int old_x_value = x_value;
								max_x_value = amount;
								if (charge_mana_while_resolving(i, count, EVENT_RESOLVE_TRIGGER, i, COLOR_COLORLESS, -1)){
									draw_cards(i, x_value);
								}
								max_x_value = old_max_x_value;
								x_value = old_x_value;
							}
						}
						if( i == player && id == CARD_ID_OLORO_AGELESS_ASCETIC &&
							has_mana(i, COLOR_COLORLESS, 1)
							){
							int choice = do_dialog(i, i, count, -1, -1, " Activate Oloro\n Pass", 0);
							if( choice == 0 && charge_mana_while_resolving(i, count, EVENT_RESOLVE_TRIGGER, i, COLOR_COLORLESS, 1)){
								draw_cards(player, 1);
								lose_life(1-player, 1);
							}
						}
						if( i == player && id == CARD_ID_DROGSKOL_REAVER ){
							draw_cards(i, 1);
						}
						if( i == player && id == CARD_ID_CRADLE_OF_VITALITY &&
							has_mana_multi(i, 1, 0, 0, 0, 0, 1)
							){
							target_definition_t td;
							default_target_definition(i, count, &td, TYPE_CREATURE);
							td.preferred_controller = player;
							if( player == AI ){
								td.allowed_controller = player;
							}
							td.allow_cancel = 0;
							if( can_target(&td) ){
								int choice = do_dialog(i, i, count, -1, -1, " Activate Cradle of Vitality\n Pass", 0);
								if( choice == 0 ){
									charge_mana_multi(i, 1, 0, 0, 0, 0, 1);
									if( spell_fizzled != 1 ){
										card_instance_t *instance = get_card_instance(i, count);
										pick_target(&td, "TARGET_CREATURE");
										add_1_1_counters(instance->targets[0].player, instance->targets[0].card, amount);
									}
								}
							}
						}
						if( i == player && id == CARD_ID_SEARING_MEDITATION &&
							has_mana(i, COLOR_COLORLESS, 2)
							){
							target_definition_t td;
							default_target_definition(i, count, &td, TYPE_CREATURE);
							td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
							td.allow_cancel = 0;
							if( can_target(&td) ){
								int choice = do_dialog(i, i, count, -1, -1, " Activate Searing Meditation\n Pass", 0);
								if( choice == 0 ){
									charge_mana(i, COLOR_COLORLESS, 2);
									if( spell_fizzled != 1 ){
										card_instance_t *instance = get_card_instance(i, count);
										pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
										damage_creature(instance->targets[0].player, instance->targets[0].card, 2, i, count);
									}
								}
							}
						}
						if( i == player && (id == CARD_ID_SANGUINE_BOND || id == CARD_ID_DEFIANT_BLOODLORD) ){
							lose_life(1-player, amount);
						}
						if( i == player && id == CARD_ID_ARCHANGEL_OF_THUNE ){
							at_count++;
						}
					}
					else{
						card_instance_t *eff = get_card_instance(i, count);
						if( player == eff->targets[1].player && eff->targets[2].card == CARD_ID_VIZKOPA_GUILDMAGE ){
							lose_life(1-eff->targets[1].player, amount);
						}
					}
				}
				count--;
			}
		}

		if( at_count > 0 ){
			int count = active_cards_count[player]-1;
			while( count > -1 ){
					if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
						if( ! already_dead(player, count) && get_card_instance(player, count)->damage_on_card < get_toughness(player, count) ){
							if( current_phase == PHASE_NORMAL_COMBAT_DAMAGE || current_phase == PHASE_FIRST_STRIKE_DAMAGE ){
								set_special_flags3(player, count, SF3_ARCHANGEL_OF_THUNE_COUNTER);
								get_card_instance(player, count)->eot_toughness = at_count;
							}
							else{
								add_counters(player, count, COUNTER_P1_P1, at_count);
							}
						}
					}
					count--;
			}
		}

		// do the code for punishing fire
		int p = player;
		const int *grave = get_grave(1-p);
		i = count_graveyard(1-p) -1;
		while( i > -1 ){
			if( cards_data[grave[i]].id == CARD_ID_PUNISHING_FIRE && has_mana(1-p, COLOR_RED, 1) ){
				int card_added = add_card_to_hand(1-p, grave[i] );
				int choice = do_dialog(1-p, 1-p, card_added, -1, -1, " Get back Punishing Fire\n Pass", 0);
				if( choice == 0 ){
					charge_mana(1-p, COLOR_RED, 1);
					if( spell_fizzled != 1 ){
						rfg_card_from_grave(1-p, i);
					}
					else{
						obliterate_card(1-p, card_added);
					}
				}
				else{
					obliterate_card(1-p, card_added);
				}
			}
			i--;
		}
	}

	real_gain_life(player, amount);
	increase_trap_condition(player, TRAP_LIFE_GAINED, amount);
	if( false_cure_flag > 0 ){
		lose_life(player, (2*false_cure_flag)*amount);
	}
}

/* Returns the amount of life gained, which will always be at least 1.  EA_LICH must be set in event_flags for the trigger to be dispatched (so a card with
 * Flags: Lich set in ct_all.csv must have been on the battlefield this turn, or set it manually each turn during EVENT_CAN_SKIP_TURN). */
int trigger_gain_life(int player, int card, event_t event, int who_gains, resolve_trigger_t trigger_mode)
{
  int amt;
  if (trigger_condition == TRIGGER_GAIN_LIFE && reason_for_trigger_controller == player && (amt = EXE_DWORD(0x73B6AC)) > 0
	  && (who_gains == ANYBODY || who_gains == trigger_cause_controller) && affect_me(player, card) && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		{
		  if (trigger_mode == RESOLVE_TRIGGER_DUH)
			trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

		  event_result |= trigger_mode;
		}

	  if (event == EVENT_RESOLVE_TRIGGER)
		return amt;
	}

  return 0;
}

int check_state(int player, int card, int state ){
	card_instance_t *this = get_card_instance(player, card);
	if( this->state & state ){
		return 1;
	}
	return 0;
}

int check_status(int player, int card, int status ){
	card_instance_t *this = get_card_instance(player, card);
	if( this->token_status & status ){
		return 1;
	}
	return 0;
}

void kill_my_legacy(int player, int card){
	int id = get_id(player, card);
	int i;
	for(i=0; i<2; i++){
		int count = active_cards_count[i]-1;
		while( count > -1 ){
				if( is_what(i, count, TYPE_EFFECT) ){
					card_instance_t *this = get_card_instance( i, count );
					if( this->targets[2].card == id ){
						kill_card(i, count, KILL_REMOVE);
					}
				}
				count--;
		}
	}
}

void untap_lands(int player, int card, int num_lands){
	int i =0;
	card_instance_t *instance = get_card_instance( player, card );
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	while(i++ < num_lands){
		if( can_target(&td) ){
			if ( select_target(player, card, &td, "Choose a land to untap", NULL) ){
				instance->number_of_targets = 1;
				untap_card( instance->targets[0].player, instance->targets[0].card);
			}
			else{
				i = num_lands;
			}
		}
	}
}

int until_eot_legacy(int player, int card, event_t event){

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int can_block_me(int blocked_player, int blocked_card, int blocking_player, int blocking_card){
	return is_legal_block(blocking_player, blocking_card, blocked_player, blocked_card);
}

// Makes player/card block player_to_block/card_to_block.  Doesn't check legality.
void block(int player, int card, int player_to_block, int card_to_block)
{
  // This is inlined in generate_token(), since some of it happens before the token's put into play and some of it after.
  // Also inlined in human_assign_blockers, where STATE_UNKNOWN8000 isn't set.

  if (player == player_to_block)
	return;

  card_instance_t* to_block = get_card_instance(player_to_block, card_to_block);
  card_instance_t* blocker = get_card_instance(player, card);

  blocker->blocking = to_block->blocking == 255 ? card_to_block : to_block->blocking;
  blocker->state |= STATE_UNKNOWN8000|STATE_BLOCKING;

  play_sound_effect(WAV_BLOCK2);

  if (event_flags & EA_SELECT_BLOCK)
	dispatch_trigger2(current_turn, TRIGGER_BLOCKER_CHOSEN, EXE_STR(0x790074)/*PROMPT_BLOCKERSELECTION[0]*/, 0, player, card);
}

int player_flips_a_coin(int player, int card, int who_flips){
	int result = 0;
	int choice = get_id(player, card) == CARD_ID_MANA_CLASH ? FC_TAILS : FC_HEAD;
	if( get_id(player, card) != CARD_ID_RAL_ZAREK && get_id(player, card) != CARD_ID_MANA_CLASH &&
		get_id(player, card) != CARD_ID_PANDORAS_BOX
	  ){
		choice = do_dialog(who_flips, player, card, -1, -1, " Heads\n Tails", internal_rand(100) > 49 ? FC_TAILS : FC_HEAD);
	}
	int rounds = 2*count_cards_by_id(player, CARD_ID_KRARKS_THUMB);
	if( ! rounds ){
		rounds = 1;
	}
	char buffer[500];
	int count = 0;
	int cf[rounds];
	int pos = 0;
	while( count < rounds ){
			cf[count] = coin_flip(who_flips, "", 1);
			if( rounds > 1 ){
				pos += scnprintf(buffer + pos, 500-pos, " Choose");
				if( cf[count] == FC_HEAD ){
					pos += scnprintf(buffer + pos, 500-pos, " Heads\n");
				}
				else{
					pos += scnprintf(buffer + pos, 500-pos, " Tails\n");
				}
			}
			count++;
	}
	if( rounds == 1 ){
		result = choice == cf[0];
	}
	else{
		int ai_choice = 0;
		count = 0;
		while( count < rounds ){
				if( cf[count] == choice ){
					ai_choice = count;
					break;
				}
				count++;
		}
		int choice2 = do_dialog(who_flips, player, card, -1, -1, buffer, ai_choice);
		if( choice == cf[choice2] ){
			result = 1;
		}
	}
	count = active_cards_count[who_flips]-1;
	while( count > -1 ){
			if( in_play(who_flips, count) && get_id(who_flips, count) == CARD_ID_KARPLUSAN_MINOTAUR ){
				target_definition_t td;
				default_target_definition(who_flips, count, &td, TYPE_CREATURE);
				td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
				if( result == 0 ){
					td.who_chooses = 1-who_flips;
					td.preferred_controller = who_flips;
				}
				td.allow_cancel = 0;
				card_instance_t *instance = get_card_instance(who_flips, count);
				if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
					instance->number_of_targets = 1;
					damage_creature(instance->targets[0].player, instance->targets[0].card, 1, who_flips, count);
				}
			}
			count--;
	}
	if( result == 1 ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, 0, "");
		this_test.id = CARD_ID_CHANCE_ENCOUNTER;
		new_manipulate_all(player, card, who_flips, &this_test, ACT_ADD_COUNTERS(COUNTER_LUCK, 1));
	}
	return result;
}

int flip_a_coin(int player, int card){
	return player_flips_a_coin(player, card, player);
}

/* Can be used with sorceries and with any target restrictions, but the AI can't fully understand the power/toughness modifications.
 * td only needs to be set if IS_CASTING(player, card, event). */
int vanilla_pump(int player, int card, event_t event, target_definition_t *td, int power, int toughness, int keyword, int sp_keyword){

	int hack_side_effect = keyword & KEYWORD_RECALC_ALL;

	if (hack_side_effect == VANILLA_PUMP_REGENERATE || hack_side_effect == VANILLA_PUMP_REGENERATE_DONT_KILL_CARD){
		target_definition_t td_regen;
		memcpy(&td_regen, td, sizeof(target_definition_t));
		td_regen.required_state |= TARGET_STATE_DESTROYED;
		if (player == AI){
			td_regen.special |= TARGET_SPECIAL_REGENERATION;
		}

		if (event == EVENT_CAN_CAST && can_target(&td_regen)){
			return 99;
		}

		if (event == EVENT_CAST_SPELL && affect_me(player, card) && can_target(&td_regen)){
			get_card_instance(player, card)->number_of_targets = 0;
			pick_target(&td_regen, "TARGET_CREATURE");
			return 0;
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(td) ){
			card_instance_t* instance = get_card_instance(player, card);
			keyword &= ~KEYWORD_RECALC_ALL;
			int leg;
			if (keyword || sp_keyword){
				leg = pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, power, toughness, keyword, sp_keyword);
			} else {
				leg = pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, power, toughness);
			}
			switch (hack_side_effect){
				case VANILLA_PUMP_DRAW_A_CARD:	draw_a_card(player);	break;
				case VANILLA_PUMP_SCRY_1:		scry(player, 1);		break;
				case VANILLA_PUMP_CANTRIP:
					alternate_legacy_text(1, player, leg);
					alternate_legacy_text(2, player, cantrip(player, card, 1));
					break;
				case VANILLA_PUMP_DONT_KILL_CARD:
					alternate_legacy_text(1, player, leg);
					return 1;
				case VANILLA_PUMP_REGENERATE:
				case VANILLA_PUMP_REGENERATE_DONT_KILL_CARD:
					alternate_legacy_text(1, player, leg);
					alternate_legacy_text(2, player, regenerate_or_shield(player, card, instance->targets[0].player, instance->targets[0].card));
					if (hack_side_effect == VANILLA_PUMP_REGENERATE_DONT_KILL_CARD){
						return 1;
					}
					break;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, td, "TARGET_CREATURE", 1, NULL);
}

/* A more restricted version of vanilla_pump(), usable only for instants and only when there are no restrictions on targeting except for whose creature it can
 * target; but this lets us teach the AI about the power/toughness modifications. */
int vanilla_instant_pump(int player, int card, event_t event, int allowed_controller, int preferred_controller, int power, int toughness, int keyword, int sp_keyword)
{
  if (event == EVENT_CHECK_PUMP && (power || toughness || (sp_keyword & SP_KEYWORD_INDESTRUCTIBLE)))
	{
	  if (!has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id))
		return 0;

	  if (sp_keyword & SP_KEYWORD_INDESTRUCTIBLE)
		toughness = 99;

	  if (preferred_controller == 1-player || preferred_controller == ANYBODY)
		{
		  if (power < 0)
			pumpable_power[1-player] += power;
		  if (toughness < 0)
			pumpable_toughness[1-player] += toughness;
		}
	  if (preferred_controller == player || preferred_controller == ANYBODY)
		{
		  if (power > 0)
			pumpable_power[player] += power;
		  if (toughness > 0)
			pumpable_toughness[player] += toughness;
		}
	}

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = allowed_controller;
  td.preferred_controller = preferred_controller;

  return vanilla_pump(player, card, event, &td, power, toughness, keyword, sp_keyword);
}

void shuffle_for_exe(int whose_computer_shuffles, int whose_library)
{
  if (ai_is_speculating != 1)
	{
	  play_sound_effect(WAV_SHUFFLE);
	  EXE_FN(void, 0x438940, int)(whose_library);	// shuffle_animation()
	}

  if ((trace_mode & 2)
	  && whose_computer_shuffles != 0)
	{
	  EXE_FN(void, 0x479cc0, int)(whose_library);	// receive_deck_order_from_network()
	  return;
	}

  // Shuffle ourselves, since the function doing the actual reorder in Magic.exe has a nonstandard calling convention

  int i, sz = 0;
  // First, ensure there are no "gaps" of an invalid iid of -1.
  int scratch[500];
  for (i = 0; i < 500; ++i)
	if (deck_ptr[whose_library][i] != -1)
	  scratch[sz++] = deck_ptr[whose_library][i];
  // Initialize the remaining elements to -1
  for (i = sz; i < 500; ++i)
	scratch[i] = -1;

  // Fisher-Yates shuffle
  for (i = sz - 1; i >= 1; --i)
	{
	  int j = internal_rand(i + 1);	// This introduces bias, since internal_rand() is a cruddy prng that restricts its range using mod, but no worse than what was there before.
	  if (i != j)
		SWAP(scratch[i], scratch[j]);
	}

  // Put it back.
  memcpy(deck_ptr[whose_library], scratch, sizeof(scratch));

  // And tell the other computer.
  if (trace_mode & 2)
	EXE_FN(void, 0x479c60, int)(whose_library);	// send_deck_order_over_network()
}

void shuffle(int player){
	shuffle_for_exe(player, player);

	int i;
	for(i=0; i<2; i++){
		int count = active_cards_count[i]-1;
		while( count > -1 ){
				if( in_play(i, count) ){
					if( is_what(i, count, TYPE_PERMANENT) ){
						if( get_id(i, count) == CARD_ID_PSYCHOGENIC_PROBE ){
							damage_player(player, 2, i, count);
						}
						if( get_id(i, count) == CARD_ID_WIDESPREAD_PANIC ){
							if( hand_count[player] > 0 ){
								char msg[100] = "Select a card to put on top.";
								test_definition_t this_test;
								new_default_test_definition(&this_test, TYPE_ANY, msg);
								int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
								put_on_top_of_deck(player, selected);
							}
						}
						if( i != player && get_id(i, count) == CARD_ID_COSIS_TRICKSTER ){
							int choice = 0;
							if( ! duh_mode(i) ){
								choice = do_dialog(i, i, count, -1, -1, " Add +1/+1 counter\n Pass", 0);
							}
							if( choice == 0 ){
								add_1_1_counter(i, count);
							}
						}
					}
				}
				count--;
		}
	}
}

int count_creatures_by_power(int player, int pow, int mode){
	// Flags for mode
	// 0 -> equal
	// 1 -> more
	// 2 -> less
	int count = 0;
	int result = 0;
	while( count < active_cards_count[player] ){
			if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
				int his_pow = get_power(player, count);
				if( mode == 0 && his_pow == pow ){
					result++;
				}
				if( mode == 1 && his_pow > pow ){
					result++;
				}
				if( mode == 2 && his_pow < pow ){
					result++;
				}
			}
			count++;
	}
	return result;
}

int count_creatures_by_toughness(int player, int pow, int mode){
	// Flags for mode
	// 0 -> equal
	// 1 -> more
	// 2 -> less
	int count = 0;
	int result = 0;
	while( count < active_cards_count[player] ){
			if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) ){
				int his_pow = get_toughness(player, count);
				if( mode == 0 && his_pow == pow ){
					result++;
				}
				if( mode == 1 && his_pow > pow ){
					result++;
				}
				if( mode == 2 && his_pow < pow ){
					result++;
				}
			}
			count++;
	}
	return result;
}

int vanilla_creature_pumper(int player, int card, event_t event, int req_cless, int req_black, int req_blue,
							int req_green, int req_red, int req_white, int flags,
							int pow, int tou, int key, int s_key, target_definition_t *td
  ){

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td_default;
	if (!td){
		default_target_definition(player, card, &td_default, TYPE_CREATURE);
		td_default.preferred_controller = player;
		td = &td_default;
	}

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(td) ){
		card_instance_t* instance = get_card_instance(player, card);
		pump_ability_until_eot(player, card,
							   instance->targets[0].player, instance->targets[0].card, pow, tou, key, s_key);
	}

	return generic_activated_ability(player, card, event, flags | GAA_CAN_TARGET,
									 req_cless, req_black, req_blue, req_green, req_red, req_white,
									 0, td, "TARGET_CREATURE");
}

int real_count_colors(int clr){
	int result = 0;
	int i;
	for(i=1; i<6; i++){
		if( clr & (1 << i) ){
			result++;
		}
	}
	return result;
}

int count_colors(int player, int card){

	int clr;
	if( player != -1 ){
		clr = get_color(player, card);
	}
	else{
		clr = cards_data[card].color;
		if (cards_data[card].type == TYPE_LAND){
			return cards_data[card].id == CARD_ID_DRYAD_ARBOR ? 1 : 0;
		}
	}
	return real_count_colors(clr);
}

int evaluate_colors(int color1, int color2){
	if( color2 != COLOR_TEST_COLORLESS ){
		int i;
		for(i=1; i<6; i++){
			if( !(color1 & (1<<i)) && (color2 & (1<<i)) ){
				return 0;
			}
		}
	}
	return 1;
}

int end_of_combat_trigger(int player, int card, event_t event, int trigger_mode){

	if( trigger_condition == TRIGGER_END_COMBAT && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		if( event == EVENT_TRIGGER){
			event_result |= trigger_mode;
		}
		else if( event == EVENT_RESOLVE_TRIGGER ){
				return 1;
		}
	}
	return 0;
}

int discard_trigger(int player, int card, event_t event, int t_player, int trigger_mode, int discard_trigger_mode){

	if( trigger_condition == TRIGGER_DISCARD && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		if( t_player != 2 && t_player != trigger_cause_controller ){
			return 0;
		}
		if( (discard_trigger_mode & DISCARD_NO_NORMAL) && current_phase > PHASE_MAIN2 ){
			return 0;
		}
		if ((discard_trigger_mode & DISCARD_STILL_IN_HAND) && !in_hand(trigger_cause_controller, trigger_cause)){
			return 0;
		}

		if(event == EVENT_TRIGGER){
			if (trigger_mode == RESOLVE_TRIGGER_DUH){
				trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
			}
			event_result |= trigger_mode;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			return 1;
		}
	}

	return 0;
}

void set_life_total(int player, int amount){
	if( life[player] < amount ){
		int lg = amount-life[player];
		gain_life(player, lg);
	}
	else if( life[player] > amount ){
			int ll = life[player]-amount;
			lose_life(player, ll);
	}
}

int lifegaining_charm(int player, int card, event_t event, int affected_player, int clr, int trigger_mode, int cless){

	if( specific_spell_played(player, card, event, affected_player, trigger_mode, TYPE_ANY, 0, 0, 0, clr, 0, 0, 0, -1, 0) ){
		charge_mana(player, COLOR_COLORLESS, cless);
		if( spell_fizzled != 1 ){
			gain_life(player, 1);
		}
	}

	return 0;
}

int cip_lifegain(int player, int card, event_t event, int amount){

	if( comes_into_play(player, card, event) ){
		gain_life(player, amount);
	}

	return 0;
}

static void cannot_lose_the_game_impl(int player, int card, event_t event, int who_cant_lose, int for_life, int for_poison, int for_draw_on_empty_deck){

	/* Local data usage:
	 * targets[5].player: Amount of life loss being hidden from the engine
	 * targets[5].card: Amount of poison counters being hidden from the engine
	 * targets[6].player: who who_cant_lose was the last time this was called
	 * targets[6].card: set to 17 once this begins leaving play
	 */

	// Everything here (except for card drawing) is deliberately checked continuously, not just during state-based effects or similar.

	card_instance_t* instance = in_play(player, card);
	// Make sure in play and initialized
	if (!instance){
		return;
	}

	if (instance->targets[5].player < 0 || instance->targets[5].card < 0){
		instance->targets[5].player = 0;
		instance->targets[5].card = 0;
		instance->targets[6].player = who_cant_lose;
	}

	// Restore lost life, poison counters
	int restore_life_poison = 0;
	if (instance->targets[6].player != who_cant_lose){
		// Preventing death from a different player than before - restore lost life/poison for old player before doing anything else
		restore_life_poison = 1;
	}
	if (leaves_play(player, card, event)){	// Call even if somehow changing control at the same time
		restore_life_poison = 1;
		// Prevent further suppression of life loss or poison counters by this card as it leaves play
		instance->targets[6].card = 17;
	}
	if (restore_life_poison){
		if (for_poison){
			raw_set_poison(instance->targets[6].player, POISON_COUNTERS(instance->targets[6].player) + instance->targets[5].card);
		}
		instance->targets[5].card = 0;

		if (for_life){
			life[instance->targets[6].player] -= instance->targets[5].player;
		}
		instance->targets[5].player = 0;

		instance->targets[6].player = who_cant_lose;
	}

	// If we are at 0 cards and we would draw a card, instead do nothing
	if (trigger_condition == TRIGGER_REPLACE_CARD_DRAW
		&& deck_ptr[who_cant_lose][0] == -1
		&& reason_for_trigger_controller == who_cant_lose
		&& affect_me(player, card)
		&& for_draw_on_empty_deck
		&& !suppress_draw
	   ){
		if (event == EVENT_TRIGGER){
			event_result = RESOLVE_TRIGGER_MANDATORY;
		} else if (event == EVENT_RESOLVE_TRIGGER){
			suppress_draw = 1;
		}
	}

	// Store changes of life that reduce to below 1
	if (for_life){
		if (life[who_cant_lose] < 1 && instance->targets[6].card != 17){
			int diff = 1 - life[who_cant_lose];
			life[who_cant_lose] += diff;
			instance->targets[5].player += diff;
		} else if (life[who_cant_lose] > 1 && instance->targets[5].player > 0){
			int diff = MIN(life[who_cant_lose] - 1, instance->targets[5].player);
			life[who_cant_lose] -= diff;
			instance->targets[5].player -= diff;
		}
	}

	// Store changes of poison counter that increase above 9
	if (for_poison){
		if (POISON_COUNTERS(who_cant_lose) > 9 && instance->targets[6].card != 17){
			int diff = POISON_COUNTERS(who_cant_lose) - 9;
			raw_set_poison(who_cant_lose, 9);
			instance->targets[5].card += diff;
		} else if (POISON_COUNTERS(who_cant_lose) < 9 && instance->targets[5].card > 0){
			int diff = MIN(9 - POISON_COUNTERS(who_cant_lose), instance->targets[5].card);
			raw_set_poison(who_cant_lose, POISON_COUNTERS(who_cant_lose) + diff);
			instance->targets[5].card -= diff;
		}
	}
}

void cannot_lose_the_game(int player, int card, event_t event, int who_cant_lose){
	cannot_lose_the_game_impl(player, card, event, who_cant_lose, 1, 1, 1);
}

void cannot_lose_the_game_for_having_less_than_0_life(int player, int card, event_t event, int who_cant_lose){
	cannot_lose_the_game_impl(player, card, event, who_cant_lose, 1, 0, 0);
}

int damage_redirection(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	// The damage directed to this target...
	int s_player = instance->targets[0].player;
	int s_card = instance->targets[0].card;

	// ...Will be redirected to this target
	int t_player = instance->targets[1].player;
	int t_card = instance->targets[1].card;

	// If "instance->targets[2].player" is set to "1", the effect end at EOT


	if( event == EVENT_PREVENT_DAMAGE && (current_phase == PHASE_FIRST_STRIKE_DAMAGE ||
		current_phase == PHASE_NORMAL_COMBAT_DAMAGE)
	  ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == s_player &&
			damage->damage_target_card == s_card && damage->info_slot > 0
		  ){
			int good = 1;
			if( t_card != -1 && ! in_play(t_player, t_card) ){
				good = 0;
			}
			if( good == 1 ){
				damage->damage_target_player = t_player;
				damage->damage_target_card = t_card;
			}
		}
	}

	if( eot_trigger(player, card, event) && instance->targets[2].player == 1 ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int generic_x_spell(int player, int card, event_t event, int zone, int mana_color, int effect){
	// Bitfields for effect
	// 1 --> Damage target
	// 8 --> Gain life if target for other effects is valid (Consume Spirit)
	// 8192 --> Pump target creature -X/-X
	// 16384 --> Mill target player

	// Use mana_color = COLOR_COLORLESS if X can be paid with any kind of mana

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = zone;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( can_target(&td) ){
			return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){

		if (!is_token(player, card)){
			if( mana_color > 0 ){
				charge_mana(player, mana_color, -1);
				if( spell_fizzled != 1 ){
					instance->info_slot = x_value;
				}
			}
			else{
				instance->info_slot = x_value;
			}
		}

		if( spell_fizzled != 1 ){
			if( zone == TARGET_ZONE_IN_PLAY ){
				pick_target(&td, "TARGET_CREATURE");
			}
			if( zone == TARGET_ZONE_PLAYERS ){
				pick_target(&td, "TARGET_PLAYER");
			}
			if( zone == TARGET_ZONE_CREATURE_OR_PLAYER ){
				pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
			}
			if( get_id(player, card) == CARD_ID_FANNING_THE_FLAMES && has_mana(player, COLOR_COLORLESS, 3) ){
				int choice = do_dialog(player, player, card, player, card, " Buyback\n Pass", 0);
				if( choice == 0 ){
					charge_mana(player, COLOR_COLORLESS, 3);
					if( spell_fizzled != 1 ){
						instance->targets[8].card = get_id(player, card);;
					}
				}
			}
		}

		set_special_flags2(player, card, SF2_X_SPELL);
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int amt = instance->info_slot;

			if( valid_target(&td)  ){
				if( effect & 1 ){	// used by: Blaze/Fanning the Flames/Volcanic Geyser, Consume Spirit, Death Grasp, Swallowing Plague
					damage_creature_or_player(player, card, event, amt);
				}
				if( effect & 8 ){	// used by: Consume Spirit, Death Grasp, Psychic Drain, Swallowing Plague
					gain_life(player, amt);
				}
				if( effect & 8192 ){	// used by: Death Wind
					pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -amt, -amt);
				}
				if( effect & 16384 ){	// used by: Psychic Drain
					mill(instance->targets[0].player, amt);
				}
			}

			if( instance->targets[8].card == CARD_ID_FANNING_THE_FLAMES ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return 0;
}

int steal_permanent_from_target_opponent_deck(int player, int card, event_t event, test_definition_t *this_test){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			new_global_tutor(player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

static int timewalk_legacy(int player, int card, event_t event){

	card_instance_t *inst = get_card_instance( player, card );

	if (inst->token_status & STATUS_TIMEWALK)
    {
		if (event == EVENT_CAN_SKIP_TURN && time_walk_flag == -1)
			time_walk_flag = player;

/* This gets checked in the wrong order - EVENT_CLEANUP is set in order by card index, not timestamp,
	and the timewalk effect with the *newest* timestamp needs to be the one that takes precedence
*/
		if (event == EVENT_CLEANUP && !dword_4EF1B8)
        {
			dword_4EF1B8 |= 1 << player;
			inst->token_status &= ~STATUS_PERMANENT;

        }
    }

    if( !(inst->token_status & STATUS_PERMANENT) && (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY) ){
		kill_card(player, card, KILL_REMOVE);
    }

	return 0;
}

static int create_time_walk_legacy(int player, int card){
	if (time_walk_flag == -1){
		int p, c;
		for (p = 0; p < 2; ++p){
			for (c = 0; c < active_cards_count[p]; ++c){
				card_instance_t* instance = get_card_instance(p, c);
				if (instance->info_slot == (int)timewalk_legacy && (instance->token_status & STATUS_TIMEWALK)){
					goto found;
				}
			}
		}
		// not found:
		time_walk_flag = player;
	}
 found:;
	int legacy_card = create_legacy_effect(player, card, &timewalk_legacy);
	card_instance_t* legacy_inst = get_card_instance(player, legacy_card);
	legacy_inst->token_status |= STATUS_TIMEWALK | STATUS_PERMANENT;
	ai_modifier += (player == AI ? 96 : -96);

	return legacy_card;
}
/* For reference:
int card_time_walk(int player, int card, event_t event){
	if (event == EVENT_CAN_CAST){
		return 1;
	} else if (event == EVENT_CAST_SPELL){
		if (affect_me(player, card)){
			ai_modifier += 96;
		}
	} else if (event == EVENT_RESOLVE_SPELL){
		create_time_walk_legacy(player, card);
		kill_card(player, card, KILL_BURY);
	}
	return 0;
}
*/

static int check_for_extra_turn_suppression(int player){
	int i;
	for(i=0; i<2; i++){
		int c;
		for(c=0; c<active_cards_count[i]; c++){
			if( in_play(i, c) && ! is_humiliated(i, c) && get_card_instance(i, c)->kill_code == 0){
				int id = get_id(i, c);
				if( id == CARD_ID_UGINS_NEXUS ){
					return 1;
				}
				if( id == CARD_ID_STRANGLEHOLD && i != player ){
					return 1;
				}
			}
		}
	}
	return 0;
}

int time_walk_effect(int player, int card){
	int legacy = -1;
	if( ! check_for_extra_turn_suppression(player) ){
		int i;
		for(i=0; i<active_cards_count[player]; i++ ){
			if( in_play(player, i) && get_id(player, i) == CARD_ID_MEDOMAI_THE_AGELESS ){
				card_instance_t *instance = get_card_instance(player, i);
				if( instance->targets[1].player < 0 ){
					instance->targets[1].player = 0;
				}
				instance->targets[1].player+=2;
			}
		}
		legacy = create_time_walk_legacy(player, card);
	}
	return legacy;
}

static int skip_next_turn_legacy(int player, int card, event_t event)
{
  /* Targets:
   * [0].player: player whose turn is to be skipped */
  if (event == EVENT_CAN_SKIP_TURN)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (current_turn == instance->targets[0].player && !(land_can_be_played & LCBP_SKIP_TURN))
		{
		  land_can_be_played |= LCBP_SKIP_TURN;
		  kill_card(player, card, KILL_REMOVE);
		}
	}

  return 0;
}

int skip_next_turn(int player, int card, int t_player){
	int legacy = create_legacy_effect(player, card, &skip_next_turn_legacy);
	card_instance_t *instance = get_card_instance(player, legacy);
	instance->targets[0].player = t_player;
	return legacy;
}

// Returns true if the turn about to be started will be skipped instead.  Accurate only during EVENT_CAN_SKIP_TURN (the first event of each turn).
int this_turn_is_being_skipped(void)
{
  if (land_can_be_played & LCBP_SKIP_TURN)	// A skip-turn effect has already resolved.
	return 1;

  /* Look for a skip_next_turn_legacy targeting the current player.  The point of this is to find a skip-your-next-turn effect that will resolve this turn, but
   * hasn't yet (perhaps because you have a tapped Time Vault that gets checked before the effect card).  No need to check for other legacies if this is itself
   * being called from a legacy, though; we can just make the first one seen trigger.  Hence it just checks for LCBP_SKIP_TURN directly. */
  int p, c;
  for (p = 0; p <= 1; ++p)
	for (c = 0; c < active_cards_count[p]; ++c)
	  if (in_play(p, c))
		{
		  card_instance_t* inst = get_card_instance(p, c);
		  if (inst->internal_card_id == LEGACY_EFFECT_CUSTOM && inst->info_slot == (int)skip_next_turn_legacy && inst->targets[0].player == current_turn)
			return 1;
		}

  return 0;
}

enum
	{
		CPL_PAY_LIFE_FOR_SPELLS_OR_ACTIVATED_ABILITY = 1<<0,
	};

int can_pay_life_impl(int player, int amount, int flags){

	int i;
	for(i=0; i<2; i++){
		int count = 0;
		while( count < active_cards_count[i] ){
				if( in_play(i, count) ){
					int id = get_id(i, count);
					if( i == player && id == CARD_ID_PLATINUM_EMPERION && ! is_humiliated(i, count) ){
						return 0;
					}
					if( (flags & CPL_PAY_LIFE_FOR_SPELLS_OR_ACTIVATED_ABILITY) && id == CARD_ID_ANGEL_OF_JUBILATION &&
						! is_humiliated(i, count)
					  ){
						return 0;
					}
				}
				count++;
		}
	}
	if( life[player] >= amount ){
		return 1;
	}

	return 0;
}

int can_pay_life_as_cost_for_spells_or_activated_abilities(int player, int amount){
	return can_pay_life_impl(player, amount, CPL_PAY_LIFE_FOR_SPELLS_OR_ACTIVATED_ABILITY);
}

int can_pay_life(int player, int amount){
	return can_pay_life_impl(player, amount, 0);
}

int lose_life(int player, int amount)
{
  if (amount <= 0)
	return 0;
  if (player == ANYBODY){
	  int l = lose_life(current_turn, amount);
	  l += lose_life(1-current_turn, amount);
	  return l;
  } else if (player != 0 && player != 1){
	  return 0;
  }

  /* Poor man's trigger.  Mark cards we're interested in that were in play at start of life loss, then resolve them all.  Can't just resolve them as we go
   * through, because Oath of Lim-Dul and Lich's Tomb could let you sacrifice something that should be triggering simultaneously.  (Come to think of it, the
   * engine's normal triggers probably wouldn't work right for those, either.) */

  int c, marked[2][151] = {{0}}, any_player = 0, any_opp = 0;
  card_instance_t* instance;

  for (c = 0; c < active_cards_count[player]; ++c)
	if ((instance = in_play(player, c)))
	  {
		int csvid = cards_data[instance->internal_card_id].id;
		if (csvid == CARD_ID_TRANSCENDENCE
			|| csvid == CARD_ID_OATH_OF_LIM_DUL
			|| csvid == CARD_ID_LICHS_TOMB)
		  {
			marked[player][c] = csvid;
			any_player = 1;
		  }
		if (csvid == CARD_ID_PLATINUM_EMPERION)
			return 0;
	  }

  for (c = 0; c < active_cards_count[1-player]; ++c)
	if ((instance = in_play(1-player, c)))
	  {
		int csvid = cards_data[instance->internal_card_id].id;
		if (csvid == CARD_ID_MINDCRANK
			|| csvid == CARD_ID_EXQUISITE_BLOOD)
		  {
			marked[1-player][c] = csvid;
			any_opp = 1;
		  }
	  }

  increase_trap_condition(player, TRAP_LIFE_LOST, amount);
  life[player] -= amount;
  if (amount > 0)
	play_sound_effect(WAV_LIFELOSS);

  // Deliberately do not check in-play again

  if (any_player)
	for (c = 0; c < active_cards_count[player]; ++c)
	  switch (marked[player][c])
		{
		  case CARD_ID_TRANSCENDENCE:
			gain_life(player, 2 * amount);
			break;

		  case CARD_ID_OATH_OF_LIM_DUL:
			oath_of_lim_dul_lifeloss_trigger(player, c, amount);
			break;

		  case CARD_ID_LICHS_TOMB:
			impose_sacrifice(player, c, player, amount, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			break;
		}

  if (any_opp)
	for (c = 0; c < active_cards_count[1-player]; ++c)
	  switch (marked[1-player][c])
		{
		  case CARD_ID_MINDCRANK:
			mill(player, amount);
			break;

		  case CARD_ID_EXQUISITE_BLOOD:
			gain_life(1 - player, amount);
			break;
		}

  return amount;
}

void store_attackers(int player, int card, event_t event, declare_attackers_trigger_t mode, int attacker_player, int attacker_card, test_definition_t* test)
{
  if (event == EVENT_DECLARE_ATTACKERS && !is_humiliated(player, card))
	{
	  if (attacker_player != current_turn && attacker_player != ANYBODY)
		return;

	  if (number_of_attackers_declared == 0)
		return;

	  card_instance_t* instance = get_card_instance(player, card);
	  int32_t* storage = ((mode & DAT_STORE_IN_INFO_SLOT) ? &instance->info_slot
						  : (mode & DAT_STORE_IN_TARGETS_3) ? &instance->targets[3].player
						  : &instance->targets[1].player);
	  if (*storage < 0)
		*storage = 0;

	  if ((mode & DAT_ATTACKS_ALONE) && number_of_attackers_declared != 1)
		return;

	  if ((mode & DAT_ATTACKS_WITH_3_OR_MORE) && number_of_attackers_declared < 3)
		return;

	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);

	  int c, begin, last;
	  if (attacker_card >= 0)
		begin = last = attacker_card;
	  else
		{
		  begin = 0;
		  last = active_cards_count[current_turn] - 1;
		}

	  for (c = begin; c <= last; ++c)
		if (in_play_and_attacking(current_turn, c)
			&& !((mode & DAT_ATTACKS_PLAYER) && check_special_flags(current_turn, c, SF_ATTACKING_PWALKER))
			&& !((mode & DAT_ATTACKS_PLANESWALKER) && !check_special_flags(current_turn, c, SF_ATTACKING_PWALKER))
			&& (!test
				|| ((test->not_me == 0 || (test->not_me == 1 && !(current_turn == player && c == card)))
					&& new_make_test_in_play(current_turn, c, -1, test))))
		  {
			if ((mode & DAT_TRACK) && *storage < 64)
			  attackers[*storage] = c;

			++*storage;
		  }
	}

	if( event == EVENT_CLEANUP ){
		card_instance_t* instance = get_card_instance(player, card);
		int32_t* storage = ((mode & DAT_STORE_IN_INFO_SLOT) ? &instance->info_slot
							: (mode & DAT_STORE_IN_TARGETS_3) ? &instance->targets[3].player
							: &instance->targets[1].player);
		*storage = 0;
	}
}

int resolve_declare_attackers_trigger(int player, int card, event_t event, declare_attackers_trigger_t mode){
  // Stores an internal count of attackers in (player,card)->targets[1].player; returns that amount.
  if (xtrigger_condition() == XTRIGGER_ATTACKING && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int32_t* storage = ((mode & DAT_STORE_IN_INFO_SLOT) ? &instance->info_slot
						  : (mode & DAT_STORE_IN_TARGETS_3) ? &instance->targets[3].player
						  : &instance->targets[1].player);
	  if (*storage <= 0)
		return 0;

	  if (event == EVENT_TRIGGER)
		{
		  int trigger_mode = mode & ((1<<4) - 1);

		  if (trigger_mode == RESOLVE_TRIGGER_DUH)
			trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
		  else if (trigger_mode == 0)
			trigger_mode = RESOLVE_TRIGGER_MANDATORY;

		  event_result |= trigger_mode;
		}

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  if ((mode & DAT_SEPARATE_TRIGGERS) && *storage > 1)
			{
			  instance->state &= ~STATE_PROCESSING;
			  --*storage;
			  return 1;
			}
		  else
			{
			  int amt = *storage;
			  *storage = 0;
			  return amt;
			}
		}

	  if (event == EVENT_END_TRIGGER)
		*storage = 0;
	}

  return 0;
}

// Stores an internal count of attackers in (player,card)->targets[1].player; returns that amount.
int declare_attackers_trigger(int player, int card, event_t event, declare_attackers_trigger_t mode, int attacker_player, int attacker_card)
{
	store_attackers(player, card, event, mode, attacker_player, attacker_card, NULL);
	return resolve_declare_attackers_trigger(player, card, event, mode);
}

// As declare_attackers_trigger, but attacking cards must also pass test.
int declare_attackers_trigger_test(int player, int card, event_t event, declare_attackers_trigger_t mode, int attacker_player, int attacker_card, test_definition_t* test)
{
	store_attackers(player, card, event, mode, attacker_player, attacker_card, test);
	return resolve_declare_attackers_trigger(player, card, event, mode);
}

int pick_target_nonbasic_land(int player, int card, int cannot_cancel)
{
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);
  td.required_subtype = SUBTYPE_NONBASIC;
  td.allow_cancel = cannot_cancel ? 0 : 1;

  card_instance_t *instance = get_card_instance(player, card);

  instance->number_of_targets = 0;

  if (can_target(&td) && pick_target(&td, "TARGET_NONBASIC_LAND"))
	{
	  if (instance->targets[0].player == player)
		ai_modifier -= 128;

	  return 1;
	}

  cancel = 1;
  return 0;
}

extern int hack_xx;
// A generalization of charge_mana_for_double_x().  num_x==2 => charge XX.  num_x==3 => charge XXX.  etc.  Returns total mana spent, not what X is.
int charge_mana_for_multi_x(int player, int color, int num_x)
{
  if (player == HUMAN && ai_is_speculating != 1)
	{
	  int old_hack_xx = hack_xx;
	  hack_xx = num_x;
	  charge_mana(player, color, -1);
	  hack_xx = old_hack_xx;
	  if (spell_fizzled != 1)
		return x_value;
	}
  else
	{
	  int mana_avail = has_mana(player, color, 1);
	  mana_avail /= num_x;

	  if (mana_avail <= 0)
		{
		  spell_fizzled = 1;
		  return 0;
		}

	  mana_avail *= num_x;
	  charge_mana(player, color, mana_avail);
	  if (spell_fizzled != 1)
		return mana_avail;
	}
  return 0;
}

// Charges XX of color.  Returns total mana spent, not what X is.
int charge_mana_for_double_x(int player, int color)
{
  return charge_mana_for_multi_x(player, color, 2);
}

void check_damage_test(int player, int card, event_t event, ddbm_flags_t mode, int triggering_player, int triggering_card, int type /*optional*/,
						test_definition_t *test/*optional*/){
	if (event == EVENT_DEAL_DAMAGE){
		card_instance_t* instance = get_card_instance(triggering_player, triggering_card);
		card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
		if (damage->internal_card_id == damage_card
			&& (damage->damage_source_card == card || card == -1)
			&& (test == NULL || new_make_test_in_play(damage->damage_source_player, damage->damage_source_card, -1, test))
			&& (type == -1 || is_what(damage->damage_source_player, damage->damage_source_card, type))
			&& damage->damage_source_player == player
			&& damage->info_slot > 0
		   ){

			if (mode & (DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_DAMAGE_PLANESWALKER | DDBM_MUST_DAMAGE_CREATURE)){
				if (damage_is_to_planeswalker(damage)){
					if (!(mode & DDBM_MUST_DAMAGE_PLANESWALKER)){
						return;
					}
				} else if (damage->damage_target_card != -1){
					if (!(mode & DDBM_MUST_DAMAGE_CREATURE)){
						return;
					}
				} else {
					if (!(mode & DDBM_MUST_DAMAGE_PLAYER)){
						return;
					}
				}
			}

			if ((mode & DDBM_MUST_DAMAGE_OPPONENT)
				&& (damage->damage_target_card != -1 || damage->damage_target_player != 1-triggering_player || damage_is_to_planeswalker(damage))){
				return;
			}
			if ((mode & DDBM_MUST_BE_COMBAT_DAMAGE)
				&& !(damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE))){
				return;
			}

			if (is_humiliated(triggering_player, triggering_card)){
				return;
			}

			if( mode & DDBM_TRACE_DAMAGING_SOURCES ){
				if( instance->info_slot < 0 ){
					instance->info_slot = 0;
				}
				int pos = instance->info_slot;
				if( pos < 10 ){
					instance->targets[pos].player = damage->damage_source_player;
					instance->targets[pos].card = damage->damage_source_card;
					instance->info_slot++;
				}
			}
			else{
				int count_pos = mode & DDBM_STORE_IN_TARGETS_9 ? 9 : 8;
				if (instance->targets[count_pos].player < 0){
					instance->targets[count_pos].player = 0;
				}
				++instance->targets[count_pos].player;
				if (mode & DDBM_TRACE_DAMAGED_PLAYERS){
					if(instance->targets[1].player == -1){
						instance->targets[1].player = 0;
					}
					if (damage->damage_target_player == 1){
						++SET_BYTE1(instance->targets[1].player);
					} else {
						++SET_BYTE0(instance->targets[1].player);
					}
				}

				if (mode & DDBM_REPORT_DAMAGE_DEALT){
					if (instance->targets[16].player < 0){
						instance->targets[16].player = 0;
					}
					instance->targets[16].player += damage->info_slot;
				}
			}
		}
	}
}

void check_damage(int player, int card, event_t event, ddbm_flags_t mode, int triggering_player, int triggering_card){
	check_damage_test(player, card, event, mode, triggering_player, triggering_card, -1, NULL);
}

int resolve_damage_trigger(int player, int card, event_t event, ddbm_flags_t mode, int triggering_player, int triggering_card){
	// DDBM_TRACE_DAMAGING_SOURCES must be used here too if it's used in "check_damage_test" or this either won't work or will glitch
	if( trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(triggering_player, triggering_card) &&
		reason_for_trigger_controller == triggering_player ){

		card_instance_t* instance = get_card_instance(triggering_player, triggering_card);
		int count_pos = mode & DDBM_STORE_IN_TARGETS_9 ? 9 : 8;
		if( mode & DDBM_TRACE_DAMAGING_SOURCES ){
			if( instance->info_slot <= 0 ){
				return 0;
			}
		}
		else{
			if (instance->targets[count_pos].player <= 0){
				return 0;
			}
		}

		if(event == EVENT_TRIGGER){
			if( mode & DDBM_TRIGGER_OPTIONAL ){
				event_result |= RESOLVE_TRIGGER_AI(triggering_player);
			}
			else{
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int packets_dealt = instance->targets[count_pos].player;
				if( mode & DDBM_TRACE_DAMAGING_SOURCES ){
					packets_dealt = instance->info_slot;
					instance->info_slot = 0;
				}
				else{
					instance->targets[count_pos].player = 0;
				}
				if( !(mode & DDBM_REPORT_DAMAGE_DEALT) ){
					instance->targets[16].player = 0;
				}
				return packets_dealt;
		}
		else if (event == EVENT_END_TRIGGER){
				instance->targets[16].player = 0;
				if( mode & DDBM_TRACE_DAMAGING_SOURCES ){
					instance->info_slot = 0;
				}
				else{
					instance->targets[count_pos].player = 0;
				}
				if (mode & DDBM_TRACE_DAMAGED_PLAYERS){
					instance->targets[1].player = 0;
				}
		}
	}

	return 0;
}

/* Just like damage_dealt_by_me(), except can specify a different triggering_player and triggering_card that highlights for the trigger and which gets its
 * targets[1,8,16].player overwritten instead of player/card. */
int damage_dealt_by_me_arbitrary(int player, int card, event_t event, ddbm_flags_t mode, int triggering_player, int triggering_card){

	if (mode & DDBM_NOT_ME){
		return 0;
	}

	ASSERT(!(mode & DDBM_TRACE_DAMAGED_CREATURES) && "unimplemented for damage_dealt_by_me_arbitrary");
	/* If that does become implemented, at least half the calls to this with DDBM_MUST_DAMAGE_PLAYER and to has_combat_damage_been_inflicted_to_a_player()
	 * should be re-examined - many of their cards say something like "Whenever ~ deals combat damage to a player, that player does something/do something
	 * to that player", and they all still assume "that player" is always opponent */

	check_damage(player, card, event, mode, triggering_player, triggering_card);

	return resolve_damage_trigger(player, card, event, mode, triggering_player, triggering_card);
}

// Overwrites instance->targets[8].player, instance->targets[16].player, and for some modes instance->targets[1].player.
int damage_dealt_by_me(int player, int card, event_t event, ddbm_flags_t mode){
	return damage_dealt_by_me_arbitrary(player, card, event, mode, player, card);
}

// A common case.  Uses targets[1], [8], [16].player, as usual.  DDBM_MUST_DAMAGE_PLAYER and DDBM_TRACE_DAMAGED_PLAYERS are added to mode.
void whenever_i_deal_damage_to_a_player_he_discards_a_card(int player, int card, int event, ddbm_flags_t mode, int discard_is_random)
{
  if (damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_TRACE_DAMAGED_PLAYERS | mode))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
	  instance->targets[1].player = 0;
	  new_multidiscard(0, times_damaged[0], discard_is_random, player);
	  new_multidiscard(1, times_damaged[1], discard_is_random, player);
	}
}

// A common case of damage_dealt_by_me_arbitrary(), this triggers on damage dealt by the card (not necessarily a creature) {player,card} is attached to.
int attached_creature_deals_damage(int player, int card, event_t event, ddbm_flags_t mode)
{
  card_instance_t* instance;
  if (IS_DDBM_EVENT(event) && (instance = in_play(player, card)))
	{
	  int p = instance->damage_target_player, c = instance->damage_target_card;
	  return damage_dealt_by_me_arbitrary(p, c, event, mode, player, card);
	}
  return 0;
}

// A common case of damage_dealt_by_me_arbitrary(), this triggers on damage dealt by the card {player,card} is equipping.
int equipped_creature_deals_damage(int player, int card, event_t event, ddbm_flags_t mode)
{
  return attached_creature_deals_damage(player, card, event, mode | DDBM_STORE_IN_TARGETS_9);
}

/* Sets triggering_player/card's targets[7].player to number of times damaged and targets[7].card to total amount of damage.  Uses targets[6] internally.  Most
 * DDBM_* flags unimplemented. */
int damage_dealt_to_me_arbitrary(int player, int card, event_t event, ddbm_flags_t mode, int triggering_player, int triggering_card)
{
  ASSERT(!(mode & DDBM_MUST_DAMAGE_OPPONENT) && "unimplemented for damage_dealt_to_me_arbitrary");
  ASSERT(!(mode & DDBM_REPORT_DAMAGE_DEALT) && "unimplemented for damage_dealt_to_me_arbitrary");
  ASSERT(!(mode & DDBM_MUST_DAMAGE_PLAYER) && "unimplemented for damage_dealt_to_me_arbitrary");
  ASSERT(!(mode & DDBM_MUST_DAMAGE_CREATURE) && "unimplemented for damage_dealt_to_me_arbitrary");
  ASSERT(!(mode & DDBM_MUST_DAMAGE_PLANESWALKER) && "unimplemented for damage_dealt_to_me_arbitrary");
  // Explicitly ignore DDBM_NOT_ME
  ASSERT(!(mode & DDBM_TRACE_DAMAGED_CREATURES) && "unimplemented for damage_dealt_to_me_arbitrary");
  ASSERT(!(mode & DDBM_TRACE_DAMAGED_PLAYERS) && "unimplemented for damage_dealt_to_me_arbitrary");
  ASSERT(!(mode & DDBM_STORE_IN_TARGETS_9) && "unimplemented for damage_dealt_to_me_arbitrary");

  card_instance_t* damage = damage_being_dealt(event);
  if (damage && damage->damage_target_card == card && damage->damage_target_player == player)
	{
	  if ((mode & DDBM_MUST_BE_COMBAT_DAMAGE)
		  && !(damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE)))
		return 0;

	  card_instance_t* instance = get_card_instance(triggering_player, triggering_card);
	  if (instance->targets[6].player < 0)
		instance->targets[6].player = 1;
	  else
		++instance->targets[6].player;

	  if (instance->targets[6].card < 0)
		instance->targets[6].card = damage->info_slot;
	  else
		instance->targets[6].card += damage->info_slot;
	}

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(triggering_player, triggering_card)
	  && reason_for_trigger_controller == triggering_player)
	{
	  card_instance_t* instance = get_card_instance(triggering_player, triggering_card);
	  if (instance->targets[6].player <= 0)
		return 0;

	  if (event == EVENT_TRIGGER)
		{
		  if (mode & DDBM_TRIGGER_OPTIONAL)
			event_result |= RESOLVE_TRIGGER_AI(triggering_player);
		  else
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
	  else if(event == EVENT_RESOLVE_TRIGGER)
		{
		  instance->targets[7] = instance->targets[6];	// struct copy
		  instance->targets[6].player = 0;
		  instance->targets[6].card = 0;
		  return 1;
		}
	  else if (event == EVENT_END_TRIGGER)
		{
		  instance->targets[6].player = 0;
		  instance->targets[6].card = 0;
		}
	}

  return 0;
}

#if 0
/* Returns total damage dealt in instance->targets[1].card, but gets very confused if targets[16].player (for replaced damage) is set.  Number of times damaged
 * in instance->info_slot - 2, but maxes at 10.  Overwrites all targets [1]-[9], but uses [1].player internally and nothing looks at what it writes to [2]-[9].
 * Caller is expected to clear info_slot if it does inspect them.  Should therefore never be used unless it's the triggering card being damaged.  Also isn't
 * usable with any of the damage_dealt_by_me() variants on the same card.  Doesn't appear fit for use except for the bare fact of damage having been dealt;
 * deprecating. */
int damage_dealt_to_me(int player, int card, event_t event, int mode){
	// Bitfields for mode
	// 1 --> Optional trigger
	// 2 --> Mandatory trigger

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player ){
					int good = 0;
					if( damage->info_slot > 0 ){
						good = damage->info_slot;
					}
					else{
						card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
						if( trg->targets[16].player > 0 ){
							good = trg->targets[16].player;
						}
					}

					if( good > 0 ){
						if( instance->info_slot < 2 ){
							instance->info_slot = 2;
						}
						if( instance->info_slot < 10 ){
							instance->targets[instance->info_slot].player = damage->damage_source_player;
							instance->targets[instance->info_slot].card = damage->damage_source_card;
							instance->info_slot++;
						}
						if( instance->targets[1].player < 0 ){
							instance->targets[1].player = 0;
						}
						instance->targets[1].player+=good;
					}
			}
		}
	}

	if( instance->targets[1].player > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			if( mode & 1 ){
				event_result |= 1+player;
			}
			else{
				event_result |= 2;
			}
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				instance->targets[1].card = instance->targets[1].player;
				instance->targets[1].player = 0;
				return 1;
		}
		else if(event == EVENT_END_TRIGGER){
			instance->targets[1].player = 0;
		}
	}

	return 0;
}
#endif

int sengir_vampire_trigger(int player, int card, event_t event, int trig_mode){

	card_instance_t *instance = get_card_instance( player, card );

	if( is_humiliated(player, card) ){
		return 0;
	}

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->info_slot > 0 ){
			if( damage->damage_source_card == card && damage->damage_source_player == player &&
				damage->damage_target_card != -1
			  ){
				if( instance->targets[1].player < 10 ){
					int good = 1;
					if( instance->targets[1].player > 2 ){
						int i;
						for(i=2; i<instance->targets[1].player; i++){
							if( damage->damage_target_player == instance->targets[i].player &&
								damage->damage_target_card == instance->targets[i].card
							  ){
								good = 0;
								break;
							}
						}
					}
					if( good == 1 ){
						if( instance->targets[1].player < 2 ){
							instance->targets[1].player = 2;
						}
						int pos = instance->targets[1].player;
						instance->targets[pos].player = damage->damage_target_player;
						instance->targets[pos].card = damage->damage_target_card;
						instance->targets[1].player++;
					}
				}
			}
		}
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( ! in_play(affected_card_controller, affected_card) ){ return 0; }
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
			if( affected->kill_code > 0 && affected->kill_code < 4 ){
				if( instance->targets[1].player > 2 ){
					int i;
					for(i=2; i<instance->targets[1].player; i++){
						if( instance->targets[i].player == affected_card_controller &&
							instance->targets[i].card == affected_card
						  ){
							if( instance->targets[11].player < 0 ){
								instance->targets[11].player = 0;
							}
							instance->targets[11].player++;
							instance->targets[i].player = get_owner(instance->targets[i].player, instance->targets[i].card);
							instance->targets[i].card = get_id(affected_card_controller, affected_card);
						}
					}
				}
			}
		}
	}

	if( instance->targets[11].player > 0 && trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller &&
		affect_me(player, card )
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= trig_mode;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				instance->targets[11].card = instance->targets[11].player;
				instance->targets[11].player = 0;
				return 1;
		} else if (event == EVENT_END_TRIGGER){
			instance->targets[11].player = 0;
		}
	}

	if( current_phase == PHASE_UNTAP ){
		instance->targets[1].player = 2;
	}

	return 0;
}

int has_dead_creature(int player){
	int i;
	for(i=0; i<2; i++){
		if(i==player || player == 2 ){
			int count = 0;
			const int *grave = get_grave(i);
			while( grave[count] != -1 ){
					if( (cards_data[grave[count]].type & TYPE_CREATURE) ){
						return 1;
					}
					count++;
			}
		}
	}
	return 0;
}

int target_player_skips_untap(int player, int card, event_t event){
	// Skip-untap effect handled in engine.c:untap_phase()
	return 0;
}

int target_player_skips_next_untap(int player, int card, int t_player ){
	int legacy = create_legacy_effect(player, card, &target_player_skips_untap);
	card_instance_t *instance = get_card_instance(player, legacy);
	instance->targets[0].player = t_player;
	instance->targets[0].card = 1;
	return legacy;
}

int charge_mana_for_activated_ability(int player, int card, int cless, int black, int blue, int green, int red, int white){
	if( check_special_flags2(player, card, SF2_MYCOSYNTH_LATTICE) ){
		cless+=(black+blue+green+red+white);
		black = blue = green = red = white = 0;
	}
	int c1 = get_cost_mod_for_activated_abilities(player, card, cless, black, blue, green, red, white);
	if( check_special_flags2(player, card, SF2_CELESTIAL_DAWN) ){
		white+=(black+blue+green+red);
		black = blue = green = red = 0;
	}
	if( cless == -1 && c1 != cless ){
		charge_mana(player, COLOR_COLORLESS, c1);
		if( spell_fizzled != 1 ){
			charge_mana_multi(player, -1, black, blue, green, red, white);
			if( spell_fizzled != 1 ){
				return 1;
			}
		}
	}
	else{
		if( c1 == 0 && black == 0 && blue == 0 && green == 0 && red == 0 && white == 0 ){
			return 1;
		}
		charge_mana_multi(player, c1, black, blue, green, red, white);
		if( spell_fizzled != 1 ){
			return 1;
		}
	}
	return 0;
}

int has_mana_for_activated_ability(int player, int card, int cless, int black, int blue, int green, int red, int white){
	// This will break if any of these are -1 to indicate {X}
	if( check_special_flags2(player, card, SF2_MYCOSYNTH_LATTICE) ){
		cless+=(black+blue+green+red+white);
		black = blue = green = red = white = 0;
	}
	int c1 = get_cost_mod_for_activated_abilities(player, card, cless, black, blue, green, red, white);
	if( check_special_flags2(player, card, SF2_CELESTIAL_DAWN) ){
		white+=(black+blue+green+red);
		black = blue = green = red = 0;
	}
	if( c1 == 0 && black == 0 && blue == 0 && green == 0 && red == 0 && white == 0 ){
		return 1;
	}
	if( has_mana_multi(player, c1, black, blue, green, red, white) ){
		return 1;
	}
	return 0;
}

int generic_activated_ability(int player, int card, event_t event, int mode, int cless, int black, int blue, int green, int red, int white,
							  uint32_t variable_costs, target_definition_t *td, const char *prompt)
{
  return granted_generic_activated_ability(player, card, player, card, event, mode, cless, black, blue, green, red, white, variable_costs, td, prompt);
}

/* Uses targets[9] to store the player/card it was attached to at the time it was activated.  Should be called *before* the EVENT_RESOLVE_ACTIVATION handler if
 * td is provided. */
int attachment_granting_activated_ability(int player, int card, event_t event, int mode,
										  int cless, int black, int blue, int green, int red, int white,
										  uint32_t variable_costs, target_definition_t *td, const char *prompt)
{
  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION || event == EVENT_CLEANUP)
	{
	  card_instance_t* instance = in_play(player, card);
	  if (!instance)
		return 0;

	  if (event == EVENT_CAN_ACTIVATE
		  && (instance->damage_target_card < 0 || instance->damage_target_player < 0
			  || !in_play(instance->damage_target_player, instance->damage_target_card)))
		return 0;

	  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE)
		{
		  // Store attached-to player/card continuously up until activation, since they get set to -1/-1 in the attached card if the attachment leaves play
		  instance->targets[9].player = instance->damage_target_player;
		  instance->targets[9].card = instance->damage_target_card;
		}

	  if (instance->targets[9].player < 0 || instance->targets[9].card < 0)	// not yet attached
		  return 0;

	  if (td)	// So we don't have to jump through hoops defining the target def; but it does require that this is called before validation.
		{
		  td->player = instance->targets[9].player;
		  td->card = instance->targets[9].card;
		  if (td->illegal_abilities & KEYWORD_SHROUD)	// A fairly reliable indicator that it was assigned through get_protections_from(), instead of e.g. being an untargeted choice of a nonflying creature
			td->illegal_abilities = get_protections_from(instance->targets[9].player, instance->targets[9].card);
		}

	  return granted_generic_activated_ability(player, card, instance->targets[9].player, instance->targets[9].card, event, mode,
											   cless, black, blue, green, red, white, variable_costs, td, prompt);
	}

  return 0;
}

/* Uses targets[9] to store the player/card it was attached to at the time it was activated and info_slot to store the choice made.  Returns 1 during
 * EVENT_RESOLVE_ACTIVATION if the activated ability was selected, 0 if it was equipped. */
int equipment_granting_activated_ability(int player, int card, event_t event, const char* dialog_option, int equip_cost, int mode,
										 int cless, int black, int blue, int green, int red, int white,
										 uint32_t variable_costs, target_definition_t *td, const char *prompt)
{
  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION || event == EVENT_CLEANUP)
	{
#define GRANTED(event)	attachment_granting_activated_ability(player, card, event, mode, cless, black, blue, green, red, white, variable_costs, td, prompt)

	  card_instance_t* instance = get_card_instance(player, card);

	  enum
	  {
		CHOICE_ABILITY = 1,
		CHOICE_EQUIP
	  } choice;

	  // If not equipping, no dialog
	  if (event != EVENT_RESOLVE_ACTIVATION && !is_equipping(player, card))
		{
		  if (event == EVENT_ACTIVATE)
			instance->info_slot = CHOICE_EQUIP;

		  return basic_equipment(player, card, event, equip_cost);
		}

	  choice = DIALOG(player, card, event,
					  dialog_option, GRANTED(EVENT_CAN_ACTIVATE), 2,
					  "Equip", can_activate_basic_equipment(player, card, event, equip_cost), 1);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_ABILITY:
			  GRANTED(event);
			  break;

			case CHOICE_EQUIP:
			  activate_basic_equipment(player, card, equip_cost);
			  break;
		  }
	  else	// EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_ABILITY:
			  GRANTED(event);
			  return 1;

			case CHOICE_EQUIP:
			  resolve_activation_basic_equipment(player, card);
			  break;
		  }
	}

  return 0;
#undef GRANTED
}

int hack_what_was_sacced_for_gaa = 0;	// valid only immediately after calling generic_activated_ability with event == EVENT_ACTIVATE.
int granted_generic_activated_ability(int granting_player, int granting_card, int granted_to_player, int granted_to_card, event_t event, int mode,
									  int cless, int black, int blue, int green, int red, int white,
									  uint32_t variable_costs, target_definition_t *td, const char *prompt)
{
	if (event == EVENT_CLEANUP){
		card_instance_t* granting_instance = get_card_instance(granting_player, granting_card);
		if (affect_me(granting_player, granting_card) && (mode & GAA_ONCE_PER_TURN)){
			granting_instance->targets[2].player = 0;
		}
		return 0;	// granted_to_player/granted_to_card may not yet be valid
	}

	if( event == EVENT_CAN_ACTIVATE ){
		card_instance_t* granting_instance = get_card_instance(granting_player, granting_card);
		card_instance_t* granted_to_instance = get_card_instance(granted_to_player, granted_to_card);
		if( (mode & GAA_UNTAPPED) &&
			(is_tapped(granted_to_player, granted_to_card) || check_special_flags2(granted_to_player, granted_to_card, SF2_KATABATIC_WINDS))
		  ){
			return 0;
		}
		if( mode & (GAA_TAPPED | GAA_UNTAPPED) ){
			if( is_what(granted_to_player, granted_to_card, TYPE_CREATURE) && is_sick(granted_to_player, granted_to_card) ){
				return 0;
			}
		}
		if (mode & GAA_CAN_TARGET){
			int make_untargettable = (mode & GAA_NOT_ME_AS_TARGET) && !(granted_to_instance->state & STATE_CANNOT_TARGET);	// In particular, don't remove STATE_CANNOT_TARGET later if it was set before this
			if (make_untargettable){
				state_untargettable(granted_to_player, granted_to_card, 1);
			}
			int count = can_target(td);
			if (make_untargettable){
				state_untargettable(granted_to_player, granted_to_card, 0);
			}
			if (!count){
				return 0;
			}
		}
		if( (mode & GAA_CAN_SORCERY_BE_PLAYED) && ! can_sorcery_be_played(granted_to_player, event) ){
			return 0;
		}
		if( ((mode & GAA_DISCARD) || (mode & GAA_DISCARD_RANDOM)) && hand_count[granted_to_player] < 1 ){
			return 0;
		}
		if( (mode & GAA_1_1_COUNTER) && count_1_1_counters(granted_to_player, granted_to_card) < 1 ){
			return 0;
		}
		if( (mode & GAA_MINUS1_MINUS1_COUNTER) && count_minus1_minus1_counters(granted_to_player, granted_to_card) < 1  ){
			return 0;
		}
		if (BYTE3(variable_costs) == 1 && count_counters(granted_to_player, granted_to_card, BYTE2(variable_costs)) < BYTE1(variable_costs)){
			return 0;
		}
		if( mode & GAA_SPELL_ON_STACK ){
			if (!counterspell(granted_to_player, granted_to_card, EVENT_CAN_CAST, td, -1)){
				return 0;
			}
		}
		if ((mode & GAA_ONCE_PER_TURN) && granting_instance->targets[2].player > 0 && (granting_instance->targets[2].player & (1 << granted_to_player))){
			return 0;
		}

		if( (mode & GAA_ONLY_ON_UPKEEP) && (current_phase != PHASE_UPKEEP || count_upkeeps(granted_to_player) < 1) ){
			return 0;
		}

		if( (mode & GAA_IN_YOUR_TURN) && current_turn != granted_to_player ){
			return 0;
		}

		if( (mode & GAA_IN_OPPONENT_TURN) && current_turn == granted_to_player ){
			return 0;
		}

		if( (mode & GAA_TAPPED) && ! is_tapped(granted_to_player, granted_to_card) ){
			return 0;
		}

		if( (mode & GAA_SACRIFICE_CREATURE) && ! can_sacrifice_as_cost(granted_to_player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return 0;
		}
		if( (mode & GAA_SACRIFICE_ME) ){
			if (!can_sacrifice_this_as_cost(granted_to_player, granted_to_card)){
				return 0;
			}
		}
		if( ((mode & GAA_SACRIFICE_ME) || (mode & GAA_SACRIFICE_CREATURE)) && (mode & GAA_NOT_ME_AS_TARGET) && count_subtype(granted_to_player, TYPE_CREATURE, -1) < 2){
			return 0;
		}
		if( (mode & GAA_DECK_SEARCHER) && IS_AI(granted_to_player) && granting_instance->targets[2].card == 1 ){	// previously failed to choose
			return 0;
		}
		if( SBYTE0(variable_costs) > 0 && ! can_pay_life_as_cost_for_spells_or_activated_abilities(granted_to_player, SBYTE0(variable_costs)) ){
			return 0;
		}
		if( mode & GAA_CAN_ONLY_TARGET_OPPONENT ){
			target_definition_t td_default, *td_used = td;
			if (!td_used){
				default_target_definition(granted_to_player, granted_to_card, &td_default, 0);
				td_default.zone = TARGET_ZONE_PLAYERS;
				td_default.allowed_controller = 1-granted_to_player;
				td_used = &td_default;
			}
			if( !would_validate_arbitrary_target(td_used, 1-granted_to_player, -1) ){
				return 0;
			}
		}
		if( mode & GAA_BEFORE_ATTACKERS ){
			if( current_phase > PHASE_MAIN1 ){
				return 0;
			}
		}
		if( mode & GAA_REGENERATION ){
			if( ! ( land_can_be_played & LCBP_REGENERATION) ){
				return 0;
			}
			if( td == NULL && granted_to_instance->kill_code != KILL_DESTROY){
				return 0;
			}
			if( td == NULL && granted_to_player == AI && !can_regenerate(granted_to_player, granted_to_card) ){
				return 0;
			}
		}
		if( (mode & GAA_TYPE_CHANGE) && granted_to_player == AI &&
			(check_special_flags(granted_to_player, granted_to_card, SF_TYPE_ALREADY_CHANGED) || is_what(granted_to_player, granted_to_card, TYPE_CREATURE))
		  ){	// when is the flag ever not redundant to the type check?
			return 0;
		}
		if( ! has_mana_for_activated_ability(granted_to_player, granted_to_card, cless, black, blue, green, red, white) ){
			return 0;
		}

		if (!can_use_activated_abilities(granted_to_player, granted_to_card) || is_humiliated(granting_player, granting_card)){
			return 0;
		}

		if( mode & (GAA_REGENERATION | GAA_DAMAGE_PREVENTION | GAA_DAMAGE_PREVENTION_PLAYER | GAA_DAMAGE_PREVENTION_CREATURE | GAA_DAMAGE_PREVENTION_ME
					| GAA_SPELL_ON_STACK) ){
			return 99;
		} else {
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		hack_what_was_sacced_for_gaa = 0;
		card_instance_t* granting_instance = get_card_instance(granting_player, granting_card);
		if( charge_mana_for_activated_ability(granted_to_player, granted_to_card, cless == -2 ? 0 : cless, black, blue, green, red, white) ){
			if (cless == -2){
				granting_instance->info_slot = charge_mana_for_double_x(granted_to_player, COLOR_COLORLESS) / 2;
			}
			else if( cless == -1 || cless == -2 || black == -1 || blue == -1 || green == -1 || red == -1 || white == -1 ){
					granting_instance->info_slot = x_value;
			}
			if( mode & GAA_NOT_ME_AS_TARGET ){
				state_untargettable(granted_to_player, granted_to_card, 1);
			}
			int sac = 0;
			if( (mode & GAA_SACRIFICE_CREATURE) ){
				test_definition_t test;
				new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
				sac = new_sacrifice(granted_to_player, granted_to_card, granted_to_player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
				if (!sac){
					cancel = 1;
					goto failure;
				}
			}
			if( mode & GAA_CAN_TARGET ){
				if (!(mode & GAA_LITERAL_PROMPT)){
					load_text(0, prompt);
					prompt = text_lines[0];
				}
				card_instance_t* targeting_instance = get_card_instance(td->player, td->card);	// Unfortunately, this isn't necessarily the same as *either* granting_instance or granted_to_instance.
				int old_number_of_targets = targeting_instance->number_of_targets;	// Since select_target() changes it
				target_t tgt;
				int result = select_target(td->player, td->card, td, prompt, &tgt);
				targeting_instance->number_of_targets = old_number_of_targets;
				if( ! result ){
					goto failure;
				}
				if( (mode & GAA_REGENERATION) && granted_to_player == AI && !can_regenerate(tgt.player, tgt.card) ){
					goto failure;
				}
				if( (mode & GAA_DAMAGE_PREVENTION_PLAYER) ){
					card_instance_t *trg = get_card_instance(tgt.player, tgt.card);
					if( trg->damage_target_card != -1 ){
						goto failure;
					}
				}
				if( (mode & GAA_DAMAGE_PREVENTION_CREATURE) ){
					card_instance_t *trg = get_card_instance(tgt.player, tgt.card);
					if( trg->damage_target_card == -1 ){
						goto failure;
					}
				}
				if( (mode & GAA_DAMAGE_PREVENTION_ME) ){
					card_instance_t *trg = get_card_instance(tgt.player, tgt.card);
					if( trg->damage_target_card != -1 || trg->damage_target_player != granted_to_player ){
						goto failure;
					}
				}
				granting_instance->targets[0] = tgt;	// struct copy
				granting_instance->number_of_targets = 1;
			}
			if( mode & GAA_CAN_ONLY_TARGET_OPPONENT ){
				target_definition_t td_default, *td_used = td;
				if (!td_used){
					default_target_definition(granted_to_player, granted_to_card, &td_default, 0);
					td_default.zone = TARGET_ZONE_PLAYERS;
					td_default.allowed_controller = 1-granted_to_player;
					td_used = &td_default;
				}
				if (!validate_arbitrary_target(td_used, 1-granted_to_player, -1)){
					goto failure;
				}
				granting_instance->targets[0].player = 1-granted_to_player;
				granting_instance->targets[0].card = -1;
				granting_instance->number_of_targets = 1;
			}

			// Targeting successful
			if( mode & GAA_SPELL_ON_STACK ){
				if (card_on_stack_controller == -1){
					goto failure;
				}
				granting_instance->targets[0].player = card_on_stack_controller;
				granting_instance->targets[0].card = card_on_stack;
				granting_instance->number_of_targets = 1;
			}
			if( mode & GAA_UNTAPPED ){
				tap_card(granted_to_player, granted_to_card);
			}
			if( mode & GAA_TAPPED ){
				untap_card(granted_to_player, granted_to_card);
			}
			if( SBYTE0(variable_costs) > 0 ){
				lose_life(granted_to_player, SBYTE0(variable_costs));
			}
			if( mode & GAA_SACRIFICE_ME ){
				kill_card(granted_to_player, granted_to_card, KILL_SACRIFICE);
			}
			if( mode & GAA_DISCARD ){
				discard(granted_to_player, 0, granting_player);
			}
			if( mode & GAA_DISCARD_RANDOM ){
				discard(granted_to_player, DISC_RANDOM, granting_player);
			}
			if( mode & GAA_1_1_COUNTER ){
				remove_1_1_counter(granted_to_player, granted_to_card);
			}
			if( mode & GAA_MINUS1_MINUS1_COUNTER ){
				remove_counter(granted_to_player, granted_to_card, COUNTER_M1_M1);
			}
			if (BYTE3(variable_costs) == 1){
				remove_counters(granted_to_player, granted_to_card, BYTE2(variable_costs), BYTE1(variable_costs));
			}
			if( mode & GAA_ONCE_PER_TURN ){
				if (granting_instance->targets[2].player < 0){
					granting_instance->targets[2].player = 0;
				}
				granting_instance->targets[2].player |= 1 << granted_to_player;
			}
			if( mode & GAA_NOT_ME_AS_TARGET ){
				state_untargettable(granted_to_player, granted_to_card, 0);
			}
			if( mode & GAA_RFG_ME ){
				kill_card(granted_to_player, granted_to_card, KILL_REMOVE);
			}
			if( mode & GAA_BOUNCE_ME ){
				bounce_permanent(granted_to_player, granted_to_card);
			}
			if( (mode & GAA_TYPE_CHANGE) && granted_to_player == AI ){
				set_special_flags(granted_to_player, granted_to_card, SF_TYPE_ALREADY_CHANGED);
			}
			if( mode & GAA_SACRIFICE_CREATURE ){
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
			hack_what_was_sacced_for_gaa = sac;
			return 1;

		failure:
			if( mode & GAA_NOT_ME_AS_TARGET ){
				state_untargettable(granted_to_player, granted_to_card, 0);
			}
			if( mode & GAA_SACRIFICE_CREATURE ){
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
			}
			spell_fizzled = 1;
			hack_what_was_sacced_for_gaa = 0;
			return 0;
		}
	}

	return 0;
}

int prevent_all_damage(int player, int card, event_t event){

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
	return 0;
}

static int prevent_all_damage_to_target_legacy(int player, int card, event_t event){
	// prevent all the damage dealt to (targets[0].player, targets[0].card)
	// if (targets[1].player, targets[1].card) is set, will only prevent the damage did by it to (targets[0].player, targets[0].card)
	// if bit 0 of targets[2].player is set, effect will end at EOT
	// bit 1 to 6 of targets[2].player : colors to check against the damage_card

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == instance->targets[0].card && damage->damage_target_player == instance->targets[0].player &&
				damage->info_slot > 0
			  ){
				int good = 1;
				if( instance->targets[1].player > -1 &&
					!(damage->damage_source_card == instance->targets[1].card && damage->damage_source_player == instance->targets[1].player)
				  ){
					good = 0;
				}
				if( instance->targets[2].player > 1 ){
					int clr = in_play(damage->damage_source_player, damage->damage_source_card) ?
								get_color(damage->damage_source_player, damage->damage_source_card) : damage->initial_color;
					if( !(clr & instance->targets[2].player) ){
						good = 0;
					}
				}
				if( good ){
					damage->info_slot = 0;
				}
			}
		}
	}

	if( instance->targets[2].player > 0 && (instance->targets[2].player & 1) && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int prevent_all_damage_to_target(int player, int card, int t_player, int t_card, int mode){
	int legacy = -1;
	if( t_card != -1 ){
		legacy = create_targetted_legacy_effect(player, card, &prevent_all_damage_to_target_legacy, t_player, t_card);
	}
	else{
		legacy = create_legacy_effect(player, card, &prevent_all_damage_to_target_legacy);
	}
	card_instance_t *instance = get_card_instance(player, legacy);
	if( t_card == -1 ){
		instance->targets[0].player = t_player;
		instance->targets[0].card = t_card;
	}
	instance->targets[2].player = mode;
	return legacy;
}

int pick_target_permanent(target_definition_t *td){
	int type = td->required_type;

	if( type == TYPE_CREATURE ){
		if( pick_target(td, "TARGET_CREATURE") ){
			return 1;
		}
	}
	else if( type == TYPE_ARTIFACT ){
		if( pick_target(td, "TARGET_ARTIFACT") ){
			return 1;
		}
	}

	else if( type == TYPE_ENCHANTMENT ){
		if( pick_target(td, "TARGET_ENCHANTMENT") ){
			return 1;
		}
	}

	else if( type == TYPE_LAND ){
		if( pick_target(td, "TARGET_LAND") ){
			return 1;
		}
	}
	else{
		if( pick_target(td, "TARGET_PERMANENT") ){
			return 1;
		}
	}
	return 0;
}

int bounce_permanent_at_upkeep(int player, int card, event_t event, target_definition_t *td){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn == player && !is_humiliated(player, card) && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		if(event == EVENT_TRIGGER && can_target(td) ){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int count = count_upkeeps(player);
				while( count > 0 && can_target(td) ){
						if( pick_target_permanent(td) ){
							instance->number_of_targets = 1;
							bounce_permanent(instance->targets[0].player, instance->targets[0].card);
						}
						count--;
				}
		}
	}

	return 0;
}

void get_back_your_permanents(int player, int card, type_t typ)
{
  // First, destroy any effect cards controlling permanents stolen from player.
  card_instance_t* inst;
  int p, c;
  for (p = 0; p <= 1; ++p)
	for (c = 0; c < active_cards_count[1-player]; ++c)
	  if ((inst = in_play(p, c)) && inst->internal_card_id == LEGACY_EFFECT_ALADDIN && inst->damage_target_player == 1-player
		  && is_stolen(inst->damage_target_player, inst->damage_target_card) && is_what(inst->damage_target_player, inst->damage_target_card, typ))
		kill_card(p, c, KILL_REMOVE);

  // Anything left over.
  for (c = active_cards_count[1-player] - 1; c >= 0; --c)
	if (in_play(1-player, c) && is_what(1-player, c, typ) && is_stolen(1-player, c))
	  gain_control(player, card, 1-player, c);
}

int card_from_list(int player, int mode, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){
	// bitflags for mode :
	// 1 --> must return a correct value
	// 2 --> Will return a CSVID
	// 4 --> Will return an Internal Card Id
	int result = -1;
	if( ai_is_speculating != 1 ){
			while( 1 ){
				int cfl = choose_a_card("Choose a card", -1, -1);
				if( make_test(player, cfl, -1, type, flag1, subtype, flag2, color, flag3, id, flag4, cc, flag5) ){
					if( mode & 2 ){
						result = cards_data[cfl].id;
						break;
					}
					if( mode & 4 ){
						result = cfl;
						break;
					}
				}
				else{
					if( !(mode & 1)  ){
						break;
					}
				}
			}
	}
	return result;
}

int name_a_card(int player, int ai_player, test_definition_t *test, int mode){
	// bitflags for mode :
	// 1 --> must return a correct value
	// 2 --> Will return a CSVID
	// 4 --> Will return an Internal Card Id
	if( player == HUMAN ){
		return card_from_list(player, mode, test->type, test->type_flag, test->subtype, test->subtype_flag, test->color, test->color_flag, test->id, test->id_flag, test->cmc, test->cmc_flag);
	}
	else{
		int result = new_select_a_card(player, ai_player, TUTOR_FROM_DECK, AI_RANDOM, 0, -1, test);
		if( result != -1 ){
			return (mode & 2) ? cards_data[deck_ptr[ai_player][result]].id : deck_ptr[ai_player][result];
		}
		result = new_select_a_card(player, ai_player, TUTOR_FROM_GRAVE, AI_RANDOM, 0, -1, test);
		if( result != -1 ){
			return (mode & 2) ? cards_data[get_grave(ai_player)[result]].id : get_grave(ai_player)[result];
		}
		result = new_select_a_card(player, ai_player, TUTOR_FROM_RFG, AI_RANDOM, 0, -1, test);
		if( result != -1 ){
			return (mode & 2) ? cards_data[rfg_ptr[ai_player][result]].id : rfg_ptr[ai_player][result];
		}
	}
	return -1;
}

int id_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[2].player > 0 ){
		if( (instance->targets[2].player & 1) && eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

int create_id_legacy(int player, int card, int t_player, int t_card, int mode){
	int legacy = -1;
	if( t_card != -1 ){
		legacy = create_targetted_legacy_effect(player, card, &id_legacy, t_player, t_card);
	}
	else{
		legacy = create_legacy_effect(player, card, &id_legacy);
	}
	card_instance_t *leg = get_card_instance(player, legacy);
	leg->targets[1].player = player;
	leg->targets[1].card = card;
	if( t_card == -1 ){
		leg->targets[0].player = t_player;
	}
	leg->targets[2].player = mode;
	leg->targets[2].card = get_id(player, card);
	leg->targets[3].card = get_id(player, card);
	return legacy;
}

int check_for_id_legacy(int player, int id){
	int i;
	for(i=0; i<2; i++){
		if( i == player || player == 2 ){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && is_what(i, count, TYPE_EFFECT) ){
						card_instance_t *instance = get_card_instance(i, count);
						if( instance->targets[2].card == id || instance->targets[3].card == id ){
							return count;
						}
					}
					count++;
			}
		}
	}
	return -1;
}

int exile_if_would_be_put_into_graveyard_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance( player, card );
	if( instance->targets[0].player > -1 ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(instance->targets[0].player, instance->targets[0].card) ){
			card_instance_t *this = get_card_instance( instance->targets[0].player, instance->targets[0].card );
			if( this->kill_code > 0 && this->kill_code < KILL_REMOVE ){
				this->kill_code = KILL_REMOVE;
			}
		}
		if( instance->targets[1].player > 0 && (instance->targets[1].player & 1) && eot_trigger(player, card, event) ){
			remove_special_flags2(instance->targets[0].player, instance->targets[0].card, SF2_WILL_BE_EXILED_IF_PUTTED_IN_GRAVE);
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

void exile_if_would_be_put_into_graveyard(int player, int card, int t_player, int t_card, int mode){
	if( t_player > -1 && t_card > -1 ){
		set_special_flags2(t_player, t_card, SF2_WILL_BE_EXILED_IF_PUTTED_IN_GRAVE);
		int legacy = create_targetted_legacy_effect(player, card, &exile_if_would_be_put_into_graveyard_legacy, t_player, t_card);
		card_instance_t *instance = get_card_instance( player, legacy );
		instance->targets[1].player = mode;
	}
}

void null_casting_cost(int player, int card){
	card_ptr_t* c = cards_ptr[ get_id(player, card) ];
	COST_COLORLESS-=get_updated_casting_cost(player, card, -1, EVENT_MODIFY_COST, -1);
	COST_BLACK-=c->req_black;
	COST_BLUE-=c->req_blue;
	COST_GREEN-=c->req_green;
	COST_RED-=c->req_red;
	COST_WHITE-=c->req_white;
}

void infinite_casting_cost(void){
	COST_COLORLESS = COST_BLACK = COST_BLUE = COST_GREEN = COST_RED = COST_WHITE = 100;
}

int die_at_end_of_combat(int player, int card, event_t event){

	if( end_of_combat_trigger(player, card, event, 2) ){
		card_instance_t *instance = get_card_instance(player, card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return 0;
}

int reveal_any_number_of_cards_of_selected_color(int player, int card, int selected_color){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.zone = TARGET_ZONE_HAND;
	td.illegal_abilities = 0;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_color = selected_color;

	card_instance_t *instance = get_card_instance(player, card);


	int revealed = 0;
	int cards_array[ 500 ];
	int choice = do_dialog(player, player, card, -1, -1, " Auto mode\n Manual mode", 0);

	if( choice == 1){
		int stop = 0;
		while( can_target(&td) && stop != 1){
			   if( pick_target(&td, "TARGET_CARD") ){
				   if( valid_target(&td) ){
					   card_instance_t *rev_card = get_card_instance(player, instance->targets[0].card);
					   cards_array[revealed] = rev_card->internal_card_id;
					   instance->targets[revealed+1].card = instance->targets[0].card;
					   rev_card->state |= STATE_CANNOT_TARGET;
					   revealed++;
				   }
			   }
			   else{
					stop = 1;
			   }
		}

		if( revealed > 0){
			int i;
			for(i=0;i<revealed; i++){
				card_instance_t *crd = get_card_instance(player, instance->targets[i+1].card);
				crd->state &= ~STATE_CANNOT_TARGET;
			}
		}
	}

	if( choice == 0){
		int i;
		for(i=0;i<active_cards_count[player]; i++){
			if( in_hand(player, i) && ( get_color(player, i) & selected_color) ){
				card_instance_t *rev_card = get_card_instance(player, i);
				cards_array[revealed] = rev_card->internal_card_id;
				revealed++;
			}
		}
	}

	if( revealed > 0){
		show_deck( player, cards_array, revealed, "Cards revealed", 0, 0x7375B0 );
	}

	return revealed;
}

int reveal_cards_from_hand(int player, int card, test_definition_t *this_test){

	this_test->id = CARD_ID_RULES_ENGINE;
	this_test->id_flag = 1;

	int ha[100];
	int hac = 0;
	int rev[100];
	int revealed = 0;

	int count = 0;
	while( count != -1 ){
			if( in_hand(player, count) ){
				card_instance_t *crd = get_card_instance(player, count);
				ha[hac] = crd->internal_card_id;
				hac++;
			}
			count++;
	}
	if( hac > 0 ){
		int choice = 0;
		if( player == HUMAN ){
			choice = do_dialog(player, player, card, -1, -1, " Auto mode\n Manual mode", 0);
		}
		if( choice == 1){
			while( 1 ){
					int selected = show_deck( HUMAN, ha, hac, "Select a card to reveal.", 0, 0x7375B0 );
					if( selected != -1 ){
						if( new_make_test(player, ha[selected], -1, this_test) ){
							rev[revealed] = ha[selected];
							revealed++;
							ha[selected] = get_internal_card_id_from_csv_id(CARD_ID_RULES_ENGINE);
						}
						else{
							break;
						}
					}
					else{
						break;
					}
			}
		}
		else{
			count = 0;
			while( count < hac ){
					if( new_make_test(player, ha[count], -1, this_test) ){
						rev[revealed] = ha[count];
						revealed++;
					}
					count++;
			}
		}
		if( revealed > 0){
			show_deck( HUMAN, rev, revealed, "Cards revealed", 0, 0x7375B0 );
		}
	}
	return revealed;
}

// source_player/source_card are the source of the effect.  If they're unknown, put the revealed player/card there and use -1,-1 for revealed_player, revealed_card.
void reveal_card(int source_player, int source_card, int revealed_player, int revealed_card)
{
  if (ai_is_speculating != 1)
	{
	  load_text(0, "REVEALS");
	  DIALOG(source_player, source_card, EVENT_ACTIVATE,
			 DLG_FULLCARD(source_player, source_card),
			 DLG_SMALLCARD(revealed_player, revealed_card),
			 DLG_MSG(text_lines[0]));
	  //DLG_WHO_CHOOSES(HUMAN),
	  //EXE_STR(0x78F0A8), 1, 1);	//localized "OK"
	}
}

void reveal_card_iid(int source_player, int source_card, int revealed_iid)
{
  if (ai_is_speculating != 1 && revealed_iid != -1)
	{
	  load_text(0, "REVEALS");
	  DIALOG(source_player, source_card, EVENT_ACTIVATE,
			 DLG_FULLCARD(source_player, source_card),
			 DLG_SMALLCARD_ID(revealed_iid),
			 DLG_MSG(text_lines[0]));
	  //DLG_WHO_CHOOSES(HUMAN),
	  //EXE_STR(0x78F0A8), 1, 1);	//localized "OK"
	}
}

// Identical to reveal_card_iid, but arbitrary text so you can pretend it's not a reveal
void look_at_iid(int source_player, int source_card, int revealed_iid, const char* prompt)
{
  if (ai_is_speculating != 1 && revealed_iid != -1)
	DIALOG(source_player, source_card, EVENT_ACTIVATE,
		   DLG_FULLCARD(source_player, source_card),
		   DLG_SMALLCARD_ID(revealed_iid),
		   DLG_MSG(prompt));
		   //DLG_WHO_CHOOSES(HUMAN),
		   //EXE_STR(0x78F0A8), 1, 1);	//localized "OK"
}

// Gives the human a choice whether to reveal a card or not.  The AI always does.  Returns 1 if revealed.
int reveal_card_optional_iid(int source_player, int source_card, int revealed_iid, const char* prompt/*optional*/)
{
  if (ai_is_speculating != 1 && revealed_iid != -1)
	{
	  if (source_player == AI)
		{
		  reveal_card_iid(source_player, source_card, revealed_iid);
		  return 1;
		}

	  return 1 == DIALOG(source_player, source_card, EVENT_ACTIVATE,
						 DLG_FULLCARD(source_player, source_card),
						 DLG_SMALLCARD_ID(revealed_iid),
						 DLG_NO_CANCEL,
						 prompt ? prompt : "Reveal", 1, 1,
						 "Pass", 1, 1);
	}
  else
	return 0;
}

int enchant_player(int player, int card, event_t event, target_definition_t *td){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_SHOULD_AI_PLAY ){
		return should_ai_play(player, card);
	}

	if( event == EVENT_CAN_CAST ){
		return can_target(td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags(player, card, SF_TARGETS_ALREADY_SET) ){
			load_text(0, "TARGET_PLAYER");
			int result = select_target(td->player, td->card, td, text_lines[0], NULL);
			if( ! result ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( ! valid_target(td) ){
			kill_card(player, card, KILL_SACRIFICE);
		}
		else{
			instance->targets[4] = instance->targets[0];
		}
	}

	return 0;
}

int evincars(int player, int card, event_t event, color_test_t color)
{
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && !affect_me(player, card)
	  && in_play(player, card) && in_play(affected_card_controller, affected_card)
	  && !is_humiliated(player, card))
	{
	  if (get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, color))
		++event_result;
	  else
		--event_result;
	}

  return 0;
}

int count_defenders(int player)
{
  int p, c, rval = 0;
  for (p = 0; p < 2; ++p)
	if (p == player || player == ANYBODY)
	  for (c = 0; c < active_cards_count[p]; ++c)
		if (in_play(p, c) && is_what(p, c, TYPE_CREATURE) && check_for_ability(p, c, KEYWORD_DEFENDER))
		  ++rval;

  return rval;
}

int get_power_sum(int player){

	int result = 0;
	int i;
	for(i=0; i<2; i++){
		if( i == player || player == 2 ){
			int count = 0;
			while( count < active_cards_count[i] ){
				   if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) ){
					   result+= get_power(i, count);
				   }
				   count++;
			}
		}
	}

	return result;
}

void set_pt_and_abilities(int player, int card, event_t event, int pow, int tou, int key, int s_key){

		if( event == EVENT_POWER && affect_me(player, card) ){
			event_result += pow - get_base_power(player, card);
		}
		if( event == EVENT_TOUGHNESS && affect_me(player, card) ){
			event_result += tou - get_base_toughness(player, card);
		}
		if( event == EVENT_ABILITIES && affect_me(player, card) ){
				event_result |= key;
		}
		if( s_key > 0 ){
			special_abilities(player, card, event, s_key, player, card);
		}
}

int get_random_card_in_hand(int player){
	if( hand_count[player] <= 0 ){
		return -1;
	}

	if ((trace_mode & 2) && current_turn == 1){
		EXE_FN(int, 0x4A70C0, int, int)(1, 24);	// receive_network_packet(1, 24)
		return EXE_DWORD(0x628C10);
	}

	int hnd[151], hnd_count = 0, c;
	hnd[0] = -1;	// in case hand_count[] is wrong and none are found
	for (c = 0; c < active_cards_count[player]; ++c){
		if (in_hand(player, c)){
			hnd[hnd_count++] = c;
		}
	}

	int rnd = hnd_count > 1 ? internal_rand(hnd_count) : 0;

	if (trace_mode & 2){	// current_turn will be 0
		EXE_DWORD(0x628C10) = hnd[rnd];
		EXE_BYTE(0x628C0C) = 24;
		EXE_FN(int, 0x4A6BC0, int, int)(0, 24);	// send_network_packet(0, 24);
	}

	return hnd[rnd];
}

int reveal_cards_from_your_hand(int player, int card, test_definition_t *this_test){

	int choice = 0;
	int result = 0;
	int revealed[100];
	if( player != AI ){
		choice = do_dialog(player, player, card, -1, -1, " Auto mode\n Manual mode", 0);
	}
	if( choice == 0 ){
		int i;
		for(i=0;i<active_cards_count[player]; i++){
			if( in_hand(player, i) && new_make_test_in_play(player, i, -1, this_test) ){
				card_instance_t *rev_card = get_card_instance(player, i);
				revealed[result] = rev_card->internal_card_id;
				result++;
			}
		}
	}
	else{
		int your_hand[100];
		int hand_index = 0;
		int i;
		for(i=0;i<active_cards_count[player]; i++){
			if( in_hand(player, i) ){
				card_instance_t *rev_card = get_card_instance(player, i);
				your_hand[hand_index] = rev_card->internal_card_id;
				hand_index++;
			}
		}
		int to_show = hand_index;
		while( result < hand_index ){
				int selected = show_deck( HUMAN, your_hand, to_show, "Choose a card to reveal", 0, 0x7375B0 );
				if( selected != -1 && new_make_test(player, your_hand[selected], -1, this_test) ){
					revealed[result] = your_hand[selected];
					int k;
					for(k=selected; k<hand_index; k++){
						your_hand[k] = your_hand[k+1];
					}
					to_show--;
					result++;
				}
				else{
					break;
				}
		}
	}
	if( result > 0 ){
		show_deck( HUMAN, revealed, result, "Player revealed these cards", 0, 0x7375B0 );
	}
	return result;
}

int get_highest_power(int player ){

	 int result = -1;
	 int i;

	 for(i=0;i<active_cards_count[player]; i++){
		 if( in_play(player, i) && is_what(player, i, TYPE_CREATURE) ){
			 int x = get_power(player, i);
			 if(  x > result ){
				 result = x;
			 }
		 }
	 }

	 return result;
}

int can_cast_creature_from_grave(int player, int mode, event_t event){
	const int *grave = get_grave(player);
	if( grave[0] == -1 ){
		if( mode == 1 ){
			return 0;
		}
		if( mode == 2 ){
			return -1;
		}
	}
	int count = 0;
	int count_g = count_graveyard(player);
	int max_value = 0;
	int result = 0;
	if( mode == 2 ){
		result = -1;
	}

	while( count < count_g ){
			if( is_what(-1, grave[count], TYPE_CREATURE) ){
				if( !(mode & 4) ||
					((mode & 4) && ((cards_data[grave[count]].color & COLOR_TEST_BLACK) || (cards_data[grave[count]].color & COLOR_TEST_BLUE) ||
									(cards_data[grave[count]].color & COLOR_TEST_RED)))
				  ){
					int id = cards_data[grave[count]].id;
					card_ptr_t* c = cards_ptr[ id ];
					if( has_mana_to_cast_id(player, event, id) ){
						if( mode == 1 ){
							result = 1;
							break;
						}
						if( c->ai_base_value > max_value ){
							max_value = c->ai_base_value;
							result = count;
						}
					}
				}
			}
			count++;
	}
	return result;
}

static int creatures_cannot_block_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_BLOCK_LEGALITY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		if( instance->targets[0].player > 0 ){
			this_test.subtype = instance->targets[0].player;
		}
		if( instance->targets[0].card > -1 ){
			this_test.subtype_flag = instance->targets[0].card;
		}
		if( instance->targets[1].player > -1 ){
			this_test.color = instance->targets[1].player;
		}
		if( instance->targets[1].card > -1 ){
			this_test.color_flag = instance->targets[1].card;
		}
		if( instance->targets[2].player > 0 ){
			this_test.id = instance->targets[2].player;
		}
		if( instance->targets[2].card > -1 ){
			this_test.id_flag = instance->targets[2].card;
		}
		if( instance->targets[3].player > -1 ){
			this_test.cmc = instance->targets[3].player;
		}
		if( instance->targets[3].card > -1 ){
			this_test.cmc_flag = instance->targets[3].card;
		}
		if( instance->targets[4].player > 0 ){
			this_test.keyword = instance->targets[4].player;
		}
		if( instance->targets[4].card > -1 ){
			this_test.keyword_flag = instance->targets[4].card;
		}
		if( instance->targets[5].player > -1 ){
			this_test.power = instance->targets[5].player;
		}
		if( instance->targets[5].card > -1 ){
			this_test.power_flag = instance->targets[5].card;
		}
		if( instance->targets[6].player > -1 ){
			this_test.toughness = instance->targets[6].player;
		}
		if( instance->targets[6].card > -1 ){
			this_test.toughness_flag = instance->targets[6].card;
		}
		if( instance->targets[7].player > 0 ){
			this_test.state = instance->targets[7].player;
		}
		if( instance->targets[7].card > -1 ){
			this_test.state_flag = instance->targets[7].card;
		}
		if( new_make_test_in_play(affected_card_controller, affected_card, -1, &this_test) ){
			event_result = 1;
		}
	}

	if( instance->targets[8].player > 0 && (instance->targets[8].player & 1) && event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int creatures_cannot_block(int player, int card, test_definition_t *this_test, int mode){
	int legacy = create_legacy_effect(player, card, &creatures_cannot_block_legacy);
	card_instance_t *instance = get_card_instance(player, legacy);
	if( this_test != NULL ){
		instance->targets[0].player = this_test->subtype;
		instance->targets[0].card = this_test->subtype_flag;
		instance->targets[1].player = this_test->color;
		instance->targets[1].card = this_test->color_flag;
		instance->targets[2].player = this_test->id;
		instance->targets[2].card = this_test->id_flag;
		instance->targets[3].player = this_test->cmc;
		instance->targets[3].card = this_test->cmc_flag;
		instance->targets[4].player = this_test->keyword;
		instance->targets[4].card = this_test->keyword_flag;
		instance->targets[5].player = this_test->power;
		instance->targets[5].card = this_test->power_flag;
		instance->targets[6].player = this_test->toughness;
		instance->targets[6].card = this_test->toughness_flag;
		instance->targets[7].player = this_test->state;
		instance->targets[7].card = this_test->state_flag;
	}
	instance->targets[8].player = mode;
	return legacy;
}

static int effect_creature1_cant_block_creature2_until_eot(int player, int card, event_t event)
{
  if (event == EVENT_BLOCK_LEGALITY)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card)
		  && attacking_card_controller == instance->targets[0].player && attacking_card == instance->targets[0].card)
		event_result = 1;
	}

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

void creature1_cant_block_creature2_until_eot(int src_player, int src_card, int t_player, int t_card, int unblockable_player, int unblockable_card)
{
  int leg = create_targetted_legacy_effect(src_player, src_card, effect_creature1_cant_block_creature2_until_eot, t_player, t_card);
  if (leg >= 0)
	{
	  card_instance_t* legacy = get_card_instance(src_player, leg);
	  legacy->targets[0].player = unblockable_player;
	  legacy->targets[0].card = unblockable_card;
	  legacy->number_of_targets = 1;
	}
}

int can_cast_spell_from_grave(int player, int mode){
	const int *grave = get_grave(player);
	if( grave[0] == -1 ){
		if( mode == 1 ){
			return 0;
		}
		if( mode == 2 ){
			return -1;
		}
	}
	int count = 0;
	int count_g = count_graveyard(player);
	int max_value = 0;
	int result = 0;
	if( mode == 2 ){
		result = -1;
	}

	while( count < count_g ){
			if( (is_what(-1, grave[count], TYPE_SORCERY) ||
				is_what(-1, grave[count], TYPE_INSTANT) ||
				is_what(-1, grave[count], TYPE_INTERRUPT)) &&
				! is_what(-1, grave[count], TYPE_CREATURE)
			  ){
				int id = cards_data[grave[count]].id;
				card_ptr_t* c = cards_ptr[ id ];
				if( has_mana_multi(player, c->req_colorless, c->req_black, c->req_blue, c->req_green,
					c->req_red, c->req_white)
				  ){
					if( can_legally_play_iid(player, grave[count]) ){
						if( mode == 1 ){
							result = 1;
							break;
						}
						if( c->ai_base_value > max_value ){
							max_value = c->ai_base_value;
							result = count;
						}
					}
				}
			}
			count++;
	}
	return result;
}

int is_stolen(int player, int card){
	if( player == HUMAN && check_state(player, card, STATE_OWNED_BY_OPPONENT) ){
		return 1;
	}
	if( player == AI &&  ! check_state(player, card, STATE_OWNED_BY_OPPONENT) ){
		return 1;
	}
	return 0;
}

int get_owner(int player, int card)
{
  return check_state(player, card, STATE_OWNED_BY_OPPONENT) ? 1 : 0;
}

void mana_into_string(int cless, int black, int blue, int green, int red, int white, test_definition_t *this_test){
	char buffer[100];
	int k;
	int pos = 0;
	if( cless != 0 ){
		if( cless == -1 || cless == 40 ){
			pos +=scnprintf(buffer+pos, 100-pos, "X");
		}
		else{
			pos +=scnprintf(buffer+pos, 100-pos, "%d", cless);
		}
	}
	for(k=0; k<black; k++){
		pos +=scnprintf(buffer+pos, 100-pos, "B");
	}
	for(k=0; k<blue; k++){
		pos +=scnprintf(buffer+pos, 100-pos, "U");
	}
	for(k=0; k<green; k++){
		pos +=scnprintf(buffer+pos, 100-pos, "G");
	}
	for(k=0; k<red; k++){
		pos +=scnprintf(buffer+pos, 100-pos, "R");
	}
	for(k=0; k<white; k++){
		pos +=scnprintf(buffer+pos, 100-pos, "W");
	}
	strcpy(this_test->message, buffer);
}

static int can_lose_the_game(int player){
	int i;
	for(i=0; i<2; i++){
		int count = 0;
		while( count < active_cards_count[i] ){
				if( in_play(i, count)  ){
					if( is_what(i, count, TYPE_PERMANENT) ){
						int id = get_id(i, count);
						if( id == CARD_ID_PLATINUM_ANGEL && i == player && !is_humiliated(i, count) ){
							return 0;
						}
						if( id == CARD_ID_ABYSSAL_PERSECUTOR && i != player && !is_humiliated(i, count) ){
							return 0;
						}
					}
					else if( is_what(i, count, TYPE_EFFECT) ){
						card_instance_t *instance = get_card_instance(i, count);
						if( instance->display_pic_csv_id == CARD_ID_ANGELS_GRACE && i == player ){
							return 0;
						}
					}
				}
				count++;
		}
	}
	return 1;
}

void lose_the_game(int player){
	if (player == ANYBODY){	// draw
		if (ai_is_speculating == 1){
			life[0] = life[1] = -99;
		} else {
			real_lose_the_game(ANYBODY);
		}
	} else if (can_lose_the_game(player)){
		if (ai_is_speculating == 1){
			life[player] = -99;
			life[1-player] = 20;
		} else {
			real_lose_the_game(player);
		}
	}
}

int card_generic_animated_land(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[5].player > 0 ){
		instance->display_pic_csv_id = instance->targets[5].player;
	}
	if( instance->targets[5].card > 0 ){
		instance->display_pic_num = instance->targets[5].card;
	}
	if( event == EVENT_POWER && affect_me(player, card) && instance->targets[7].player > 0 ){
		event_result+=(instance->targets[7].player-1);
	}
	if( event == EVENT_TOUGHNESS && affect_me(player, card) && instance->targets[7].card > 0 ){
		event_result+=(instance->targets[7].card-1);
	}
	if( event == EVENT_ABILITIES && affect_me(player, card) && instance->targets[8].player > 0 ){
		event_result |= instance->targets[8].player;
	}
	if( event == EVENT_SET_COLOR && affect_me(player, card) && instance->targets[6].player > 0 ){
		event_result = instance->targets[6].player;
	}
	if( instance->targets[8].card > 0 ){
		special_abilities(player, card, event, instance->targets[8].card, player, card);
	}
	int result = 0;
	if( instance->targets[6].card > -1 ){
		int (*ptFunction)(int, int, event_t) = (void*)instance->targets[6].card;
		result = ptFunction(player, card, event);
	}
	return result;
}

int real_land_animation2(int player, int card, int t_player, int t_card, int mode, int pow, int tou, int key, int s_key, int clr){
	// bitfields for mode
	// 1--> ends at eor
	// 2 --> ends if (player, card) leaves play

	int legacy = turn_into_creature(player, card, t_player, t_card, mode, pow, tou);
	card_instance_t *leg = get_card_instance(player, legacy);
	leg->targets[1].player = mode;
	//leg->targets[1].card = l1;
	leg->targets[2].card = get_id(player, card);
	leg->targets[4].player = player;
	leg->targets[4].card = card;
	leg->targets[6].player = clr;
	leg->targets[8].player = key;
	leg->targets[8].card = s_key;
	return legacy;
}

static int global_type_animation_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){

		test_definition_t this_test;
		default_test_definition(&this_test, instance->targets[4].player);
		this_test.type_flag = instance->targets[4].card;
		this_test.subtype = instance->targets[5].player;
		this_test.subtype_flag = instance->targets[5].card;
		this_test.color = instance->targets[6].player;
		this_test.color_flag = instance->targets[6].card;
		this_test.cmc = instance->targets[7].player;
		this_test.cmc_flag = instance->targets[7].card;

		global_type_change(player, card, event, instance->targets[0].player, TYPE_CREATURE, &this_test,
							instance->targets[1].player, instance->targets[1].card, instance->targets[2].player, instance->targets[3].card,
							instance->targets[3].player);

		if( instance->targets[0].card > -1 && (instance->targets[0].card & 1) ){
			if( eot_trigger(player, card, event) ){
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	return 0;
}

int land_animation2(int player, int card, int t_player, int t_card, int mode, int pow, int tou, int key, int s_key, int clr, test_definition_t *this_test){
	int legacy = -1;
	if( t_card != -1 ){
		legacy = real_land_animation2(player, card, t_player, t_card, mode, pow, tou, key, s_key, clr);
	}
	else{
		legacy = create_legacy_effect(player, card, &global_type_animation_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[0].player = t_player;
		leg->targets[0].card = mode;
		leg->targets[1].player = pow;
		leg->targets[1].card = tou;
		leg->targets[2].player = key;
		leg->targets[2].card = s_key;	// unused
		leg->targets[3].player = clr;
		leg->targets[3].card = -1;
		leg->targets[4].player = this_test->type;
		leg->targets[4].card = this_test->type_flag;
		leg->targets[5].player = this_test->subtype;
		leg->targets[5].card = this_test->subtype_flag;
		leg->targets[6].player = this_test->color;
		leg->targets[6].player = this_test->color_flag;
		leg->targets[7].card = this_test->cmc_flag;
		leg->targets[7].card = this_test->cmc_flag;
	}
	return legacy;
}

int counterspell(int player, int card, event_t event, target_definition_t* td /*optional*/, int target_num){
	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAN_CAST){
		if (card_on_stack_controller != -1){
			target_definition_t td_default;
			if (!td){
				counterspell_target_definition(player, card, &td_default, 0);
				td = &td_default;
			}
			if (target_num < 0){
				target_t old_target = instance->targets[0];	// struct copy
				instance->targets[0].player = card_on_stack_controller;
				instance->targets[0].card = card_on_stack;

				int result = would_validate_target(player, card, td, 0);

				instance->targets[0] = old_target;	// struct copy

				return result ? 99 : 0;
			} else {
				instance->targets[target_num].player = card_on_stack_controller;
				instance->targets[target_num].card = card_on_stack;
				if (instance->number_of_targets <= target_num){
					instance->number_of_targets = target_num + 1;
				}

				if (would_validate_target(player, card, td, target_num)){
					return 99;
				}
			}
		}
	}
	else if (event == EVENT_CAST_SPELL && affect_me(player, card)){
		if (card_on_stack_controller == -1){
			cancel = 1;
		}
		else {
			instance->targets[target_num].player = card_on_stack_controller;
			instance->targets[target_num].card = card_on_stack;
			if (instance->number_of_targets <= target_num){
				instance->number_of_targets = target_num + 1;
			}
		}
		ai_modifier -= 36;
	}
	else if (event == EVENT_CAN_COUNTER && raw_mana_available[player][COLOR_BLUE] >= 2){
		ai_modifier += 24;
	}
	else if (event == EVENT_RESOLVE_SPELL) {
		if (counterspell_validate(player, card, td, target_num)){
			real_counter_a_spell(player, card, instance->targets[target_num].player, instance->targets[target_num].card);
		}
		instance->number_of_targets = 0;
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int counterspell_validate(int player, int card, target_definition_t* td /*optional*/, int target_num){
	int valid;
	if (td){
		int old_preferred_controller = td->preferred_controller;
		td->preferred_controller = td->allowed_controller;
		valid = validate_target(player, card, td, target_num);
		td->preferred_controller = old_preferred_controller;
	} else {
		target_definition_t td_default;
		counterspell_target_definition(player, card, &td_default, 0);
		td_default.preferred_controller = 2;
		valid = validate_target(player, card, &td_default, target_num);
	}

	card_instance_t* instance = get_card_instance(player, card);

	if (!valid || !(get_card_instance(instance->targets[target_num].player, instance->targets[target_num].card)->state & STATE_INVISIBLE)){
		cancel = 1;
		return 0;
	}
	return 1;
}

int can_counter_activated_ability(int player, int card, event_t event, target_definition_t* td /*optional*/)
{
  target_definition_t td_default;
  if (!td)
	{
	  counter_activated_target_definition(player, card, &td_default, 0);
	  td = &td_default;
	}

  int offset = (event == EVENT_ACTIVATE || event == EVENT_CAST_SPELL) ? 2 : 1;	// during these events, *this* card/effect is on the stack, too
  if (stack_size >= offset
	  && would_validate_arbitrary_target(td, stack_cards[stack_size - offset].player, stack_cards[stack_size - offset].card)
	  && stack_data[stack_size - offset].generating_event != EVENT_RESOLVE_TRIGGER       // Redundant for now, since the engine doesn't let you respond to triggers with interrupts OR fast effects.
	  && cards_data[stack_data[stack_size - offset].internal_card_id].id != 904)         // "Draw a card" effect in draw phase
	return 99;
  else
	return 0;
}

int cast_counter_activated_ability(int player, int card, int target_pos)
{
  if (stack_size >= 2)  // since this will be on the stack by now too
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  instance->targets[target_pos] = stack_cards[stack_size - 2];	// struct copy
	  ++instance->number_of_targets;

	  ai_modifier += 100;   // This is the smallest for which I could get the AI to cast it (against my fetchland).  Counterspell uses -36.
	  return 1;
	}
  else
	{
	  spell_fizzled = 1;
	  return 0;
	}
}

int validate_counter_activated_ability(int player, int card, target_definition_t* td /*optional*/, int target_pos)
{
  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td_default;
  if (!td)
	{
	  counter_activated_target_definition(player, card, &td_default, 0);
	  td = &td_default;
	}

  int old_preferred_controller = td->preferred_controller;
  td->preferred_controller = td->allowed_controller;

  int rval = 0;
  if (validate_arbitrary_target(td, instance->targets[target_pos].player, instance->targets[target_pos].card))
	{
	  card_instance_t* tgt = get_card_instance(instance->targets[target_pos].player, instance->targets[target_pos].card);

	  if ((tgt->state & STATE_JUST_CAST)    // not STATE_INVISIBLE, since this is at instant speed, not interrupt
		  && stack_data[stack_size - 1].generating_event != EVENT_RESOLVE_TRIGGER
		  && cards_data[stack_data[stack_size - 1].internal_card_id].id != 904)
		rval = 1;
	}

  td->preferred_controller = old_preferred_controller;
  return rval;
}

void raw_counter_activated_ability(int tgt_player, int tgt_card)
{
  get_card_instance(tgt_player, tgt_card)->internal_card_id = -1;         // kill_card(), like counterspell uses, works for some effects (like Prodigal Sorcerer's), but not others (like fetchlands'), probably because this is at instant speed.  This always works.
}

int resolve_counter_activated_ability(int player, int card, target_definition_t* td /*optional*/, int target_pos)
{
  if (validate_counter_activated_ability(player, card, td, target_pos))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  raw_counter_activated_ability(instance->targets[0].player, instance->targets[0].card);
	  return 1;
	}
  else
	return 0;
}

int twincast(int player, int card, int event, target_definition_t* td /*optional*/, int* ret_copy /*optional*/)
{
  target_definition_t td_default;
  if (!td)
	{
	  td = &td_default;
	  counterspell_target_definition(player, card, td, TYPE_INSTANT | TYPE_SORCERY);
	  td->preferred_controller = 2;
	}

  if (event == EVENT_CAN_CAST || event == EVENT_CAST_SPELL)
	return counterspell(player, card, event, td, 0);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (counterspell_validate(player, card, td, 0))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  int copy = copy_spell_from_stack(player, instance->targets[0].player, instance->targets[0].card);
		  if (ret_copy)
			*ret_copy = copy;
		}
	}

  return 0;
}

int activate_twincast(int player, int card, int event, target_definition_t* td /*optional*/, int* ret_copy /*optional*/)
{
  if (event == EVENT_CAN_ACTIVATE)
	return twincast(player, card, EVENT_CAN_CAST, td, ret_copy);

  if (event == EVENT_ACTIVATE)
	return twincast(player, card, EVENT_CAST_SPELL, td, ret_copy);

  if (event == EVENT_RESOLVE_ACTIVATION)
	return twincast(player, card, EVENT_RESOLVE_SPELL, td, ret_copy);

  return 0;
}

int type_change_legacy(int player, int card, event_t event){
	// Compare XmorgrantFX at 0x457FA0

	/* Targets:
	 * 0: permanent this is attached to (also in card_instance_t::damage_target_player/card)
	 * 1.player: &1: kill legacy at end of turn (or attached card, if it's chimeric coils)
	 *           &2: kill legacy if targets[4] not in play
	 *           &4: kill legacy if attached card doesn't have any counters of type BYTE0(targets[7].player)
	 *           &8: kill legacy at targets[4].player next upkeep (Xenic Poltergeist)
	 *           &16: kill legacy at end of combat (Jade Statue)
	 * 2.card: csvid of source of effect (also in card_instance_t::display_pic_csv_id)
	 * 4: if 1.player&2 and this isn't in play, kill legacy
	 * 6.player: forces color to this
	 * 7.player & 0xFF: if 1.player&4, kill legacy if attached card doesn't have any counters of this type
	 * 8.player: abilities to add
	 * 8.card: sp_abilities to add */

	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[0].player > -1 ){
		int p = instance->targets[0].player;
		int c = instance->targets[0].card;
		int destroyed = 0;

		if (event == EVENT_CHANGE_TYPE && affect_me(instance->targets[0].player, instance->targets[0].card) ){
			event_result = instance->dummy3;
		}
		if (event == EVENT_STATIC_EFFECTS && instance->targets[1].player > 0){
			if ((instance->targets[1].player & 2) &&
				(instance->targets[4].player > -1 && !in_play(instance->targets[4].player, instance->targets[4].card))
			   ){
				destroyed = 1;
			}
			if ((instance->targets[1].player & 4) && !count_counters(p, c, BYTE0(instance->targets[7].player))){
				destroyed = 1;
			}
		}
		if( event == EVENT_ABILITIES && affect_me(p, c) && instance->targets[8].player > 0 ){
			event_result |= instance->targets[8].player;
		}
		if( event == EVENT_SET_COLOR && affect_me(p, c) && instance->targets[6].player > 0 ){
			event_result = instance->targets[6].player;
		}
		if( instance->targets[2].card == CARD_ID_GENJU_OF_THE_FIELDS ){
			spirit_link_effect(p, c, event, p);
		}
		if( instance->targets[8].card > 0 ){
			special_abilities(p, c, event, instance->targets[8].card, player, card);
		}
		if( instance->targets[1].player > 0 && (instance->targets[1].player & 1) && eot_trigger(player, card, event) ){
			if( instance->targets[2].card == CARD_ID_CHIMERIC_COILS ){
				destroyed = 0;	// Just in case something above also set it
				kill_card(p, c, KILL_SACRIFICE);
			}
			else{
				destroyed = 1;
			}
		}

		if( instance->targets[1].player > 0 && (instance->targets[1].player & 8) && current_turn == instance->targets[4].player &&
			upkeep_trigger(player, card, event)
		  ){
			destroyed = 1;
		}

		if( instance->targets[1].player > 0 && (instance->targets[1].player & 16) && end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY)){
			destroyed = 1;
		}

		if (destroyed){
			remove_special_flags(p, c, SF_TYPE_ALREADY_CHANGED);
			reset_subtypes(p, c, 2);
			kill_card(player, card, KILL_REMOVE);
			recalculate_all_cards_in_play();	// since this is an effect (kill_card_guts() calls this for permanents)
		}
	}

	return 0;
}

int turn_into_artifact(int player, int card, int t_player, int t_card, int mode){
	int legacy = -1;
	if( t_player != -1 && t_card != -1 ){
		card_instance_t *instance = get_card_instance(t_player, t_card);
		int newtype = create_a_card_type(instance->internal_card_id);
		cards_at_7c7000[newtype]->type |= (cards_data[instance->internal_card_id].type | TYPE_ARTIFACT);
		legacy = create_targetted_legacy_effect(player, card, &type_change_legacy, t_player, t_card);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->dummy3 = newtype;
		leg->token_status |= STATUS_PERMANENT;
		leg->targets[1].player = mode;
	}
	return legacy;
}

int turn_into_creature(int player, int card, int t_player, int t_card, int mode, int pow, int tou){
	int legacy = -1;
	if( t_player != -1 && t_card != -1 ){
		card_instance_t *instance = get_card_instance(t_player, t_card);
		int newtype = create_a_card_type(instance->internal_card_id);
		cards_at_7c7000[newtype]->type |= (cards_data[instance->internal_card_id].type | TYPE_CREATURE);
		cards_at_7c7000[newtype]->power = pow != -1 ? pow : get_cmc(t_player, t_card);
		cards_at_7c7000[newtype]->toughness = tou != -1 ? tou : get_cmc(t_player, t_card);
		legacy = create_targetted_legacy_effect(player, card, &type_change_legacy, t_player, t_card);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->dummy3 = newtype;
		leg->token_status |= STATUS_PERMANENT;
		leg->targets[1].player = mode;
	}
	return legacy;
}

int artifact_animation(int player, int card, int t_player, int t_card, int mode, int pow, int tou, int abilities, int sp_abilities){
	// pow / tou = -1 sets the corresponding parameter to the CMC of (t_player, t_card)
	/* Flags
	 &1: kill legacy at end of turn (or attached card, if it's chimeric coils)
	 &2: kill legacy if targets[4] not in play
	 &4: kill legacy if attached card doesn't have any counters of type BYTE0(targets[7].player)
	 &8: kill legacy at targets[4].player next upkeep (Xenic Poltergeist)
	 &16: kill legacy at end of combat (Jade Statue)
	*/
	int legacy_card = turn_into_creature(player, card, t_player, t_card, mode, pow, tou);
	card_instance_t *legacy_instance = get_card_instance(player, legacy_card);
	legacy_instance->targets[2].card = get_id(player, card);
	legacy_instance->targets[8].player = abilities;
	legacy_instance->targets[8].card = sp_abilities;
	return legacy_card;
}

int is_hardcoded_card(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if( cards_data[instance->internal_card_id].code_pointer < 0x2001006 ){
		return 1;
	}
	return 0;
}

int prevent_damage_until_eot_legacy(int player, int card, event_t event){
	// instance->targets[0] = source of damage
	// instance->targets[1].player = max amount of damage to prevent. If left to "-1", all damage did by source will be prevented until EOT.

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->info_slot > 0 ){
			if( damage->damage_target_player == instance->targets[0].player && damage->damage_target_card == instance->targets[0].card ){
				if( instance->targets[1].player == -1 ){
					damage->info_slot = 0;
				}
				else{
					if( damage->info_slot > instance->targets[1].player ){
						damage->info_slot -= instance->targets[1].player;
						instance->targets[1].player = 0;
					}
					else{
						instance->targets[1].player-=damage->info_slot;
						damage->info_slot = 0;
					}
				}
			}
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int prevent_damage_until_eot(int player, int card, int t_player, int t_card, int max_dmg){
	// t_player, t_card must be a damage card
	card_instance_t *trg = get_card_instance(t_player, t_card);
	if( max_dmg == -1 ){
		trg->info_slot = 0;
	}
	else{
		if( trg->info_slot > max_dmg ){
			trg->info_slot -= max_dmg;
			max_dmg = 0;
		}
		else{
			max_dmg-=trg->info_slot;
			trg->info_slot = 0;
		}
	}
	int legacy = -1;
	if( max_dmg == -1 || max_dmg > 0 ){
		legacy = create_legacy_effect(player, card, &prevent_damage_until_eot_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[0].player = trg->damage_source_player;
		leg->targets[0].card = trg->damage_source_card;
		leg->targets[1].player = max_dmg;
		leg->targets[2].card = get_id(player, card);
	}
	return legacy;
}

void get_an_additional_combat_phase(void){
	current_phase = PHASE_DECLARE_ATTACKERS;
	land_can_be_played &= ~TENTATIVE_LCBP_DURING_COMBAT;
}

// title is a writeable buffer containing the initial text.
// For card titles, it'll be 52 bytes long.
// For fullcard text, 400 bytes; title will be 0x6089b4.
void legacy_name(char* title, int player, int card)	// 0x200aa02
{
	card_instance_t* instance = get_displayed_card_instance(player, card);
	int event = ((int)title == 0x6089b4) ? EVENT_SET_LEGACY_EFFECT_TEXT : EVENT_SET_LEGACY_EFFECT_NAME;
	// These aren't technically the same as EVENT_SET_LEGACY_EFFECT_NAME and EVENT_SET_LEGACY_EFFECT_TEXT, but the numbers are chosen to coincide
	int call_fn = instance->eot_toughness & event;
	int alternative = BYTE0(instance->eot_toughness);

	if (call_fn
		&& (instance->internal_card_id == LEGACY_EFFECT_CUSTOM
			|| instance->internal_card_id == LEGACY_EFFECT_ACTIVATED)){
		typedef int (*CardFunction)(int, int, event_t);
		set_legacy_effect_name_addr = title;
		(*(CardFunction)(instance->info_slot))(player, card, event);
		set_legacy_effect_name_addr[event == EVENT_SET_LEGACY_EFFECT_NAME ? 51 : 399] = 0;
	}

	if (alternative > 0 && alternative <= 9){	// highest currently used is 4
		// The following is the equivalent of copy_string_alternative_by_pipe_hash_digit()
		const char *p = title;
		while (1){
			while (*p && strncmp(p, "|#", 2)){
				++p;
			}

			if (!*p){
				return;	// never found a matching |# code
			}

			p += 2;
			if (atoi(p) == alternative){
				char tmp[400];
				char* q = tmp;
				++p;
				while (*p && strncmp(p, "|#", 2)){
					*q++ = *p++;
				}
				*q = 0;
				strcpy(title, tmp);
				return;
			}
		}
	}
}

// This has to replace all variants |n in buf, in-place.
void effect_card_text_replace_pipe_n(char* buf, int pow, int tgh)
{
  /* Mingw doesn't come with posix regex.h, and using c++11's std::regex and std::string (which it requires) increases object size by a full sixth.
   * That's unacceptable if we only need it for this function, so suffer through doing it manually. */

  char* begin_repl = strstr(buf, "|n");
  if (!begin_repl)	// No |n to replace
	return;

  // We may have to start replacing earlier than this.  The furthest we have to go back is four bytes, for "+0/+|n".
  begin_repl -= 4;
  if (begin_repl < buf)
	begin_repl = buf;

  /* We have to match "[+-]?\|n/", "/[+-]?\|n", and "[+-]?n" to deal with existing data.  Replace is complicated slightly by 0's, which
   * have to be properly signed to match the other value. */
  char* p = begin_repl;

  char replacement[400];	// size of the original buffer
  char* q = replacement;
  while (*p)
	{
	  // Power
	  if (!strncmp(p, "+|n/", 4)
		  || !strncmp(p, "-|n/", 4)
		  || !strncmp(p, "|n/", 3))
		{
		  // move p to the / (so it'll match the signed toughness pattern in the next iteration)
		  if (*p == '|')
			p += 2;
		  else
			p += 3;

		  if (pow == 0 && tgh < 0)
			q += sprintf(q, "-0");
		  else
			q += sprintf(q, "%+d", pow);
		}
	  // Toughness
	  else if (!strncmp(p, "/+|n", 4)
			   || !strncmp(p, "/-|n", 4)
			   || !strncmp(p, "/|n", 3))
		{
		  // move p past the end
		  if (*(p+1) == '|')
			p += 3;
		  else
			p += 4;

		  if (tgh == 0 && pow < 0)
			q += sprintf(q, "/-0");
		  else
			q += sprintf(q, "/%+d", tgh);
		}
	  /* The irritating handful of effects that just have a |n by itself to represent a number, and shove that number in counter_toughness.  These get an
	   * unsigned 0 and positive numbers, which is why /[+-]|n has to be handled separately above. */
	  else if (!strncmp(p, "|n", 2))
		{
		  p += 2;
		  q += sprintf(q, "%d", tgh);
		}
	  // Non-matching
	  else
		*q++ = *p++;
	}
  *q = 0;

  strcpy(begin_repl, replacement);
}

/* Flags an effect card to use alternate text, set off with |#1, |#2, etc. tags in its text.
 * Works only on csvid 903 effects (which includes both LEGACY_EFFECT_CUSTOM and LEGACY_EFFECT_GENERIC) other than LEGACY_EFFECT_PUMP.  So it's nonfunctional
 * with create_card_name_legacy().
 * Returns legacycard again. */
int alternate_legacy_text(int textnum, int player, int legacycard){
	if (legacycard >= 0){
		card_instance_t* instance = get_card_instance(player, legacycard);
		ASSERT(cards_data[instance->internal_card_id].id == 903 && "alternate_legacy_text only works on csvid 903 effects");
		ASSERT(instance->internal_card_id != LEGACY_EFFECT_PUMP && "alternate_legacy_text doesn't work on LEGACY_EFFECT_PUMP");
		instance->eot_toughness &= ~(EVENT_SET_LEGACY_EFFECT_NAME | EVENT_SET_LEGACY_EFFECT_TEXT);
		instance->eot_toughness |= textnum;
	}
	return legacycard;
}

// Sets {player,legacycard}'s image (and therefore title and text) to be csvid.  Works only on effect cards.  Returns legacycard again.
int set_legacy_image(int player, int csvid, int legacycard)
{
  if (legacycard >= 0)
	{
	  card_instance_t* leg = get_card_instance(player, legacycard);
	  leg->display_pic_csv_id = csvid;
	  leg->display_pic_num = get_card_image_number(csvid, leg->damage_source_player, leg->damage_source_card);
	}
  return legacycard;
}

int create_card_name_legacy(int player, int card, int id){
	int legacy_card = create_legacy_effect_exe(player, card, LEGACY_EFFECT_ASWAN, player, card );
	card_instance_t *legacy = get_card_instance(player, legacy_card);
	legacy->info_slot = -id - 1;
	return legacy_card;
}

static int subtype_name_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if (event == EVENT_SET_LEGACY_EFFECT_NAME && instance->targets[0].player > -1 ){
		scnprintf(set_legacy_effect_name_addr, 51, "%s", raw_get_subtype_text(instance->targets[0].player));
	}
	if( instance->targets[0].card == 2 && event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int create_subtype_name_legacy(int player, int card, int subtype){
	int legacy_card = create_targetted_legacy_effect(player, card, &subtype_name_legacy, player, card);
	get_card_instance(player, legacy_card)->targets[0].player = subtype;
	get_card_instance(player, legacy_card)->eot_toughness = EVENT_SET_LEGACY_EFFECT_NAME;
	return legacy_card;
}

int create_hardcoded_subtype_name_legacy(int player, int card, int subtype){
	int legacy_card = create_legacy_effect_exe(player, card, LEGACY_EFFECT_ASWAN, player, card );
	card_instance_t *legacy = get_card_instance(player, legacy_card);
	legacy->info_slot = subtype;
	return legacy_card;
}

void set_x_for_x_spells(int player, int card, event_t event, int value){
	card_instance_t *instance = get_card_instance(player, card);
	int value_before_x = 0;
	if( cards_data[instance->internal_card_id].cc[1] <= 15 ){
		value_before_x += cards_data[instance->internal_card_id].cc[1];
	}
	int diff = value_before_x - true_get_updated_casting_cost(player, card, -1, event, -1);
	instance->info_slot = value;
	if( diff < 0 ){
		instance->info_slot -= diff;
	}
}

int generic_spell(int player, int card, event_t event, int flags, target_definition_t *td, const char *prompt, int max_targets, test_definition_t *this_test){

	card_instance_t *instance = get_card_instance(player, card);

	// straight from "targeted_aura"
	if( flags & GS_AURA ){
		if (event == EVENT_GET_SELECTED_CARD){
			*(int*)(0x78FA30) = instance->damage_target_card | (instance->damage_target_player << 8);
		}
	}

	if(event == EVENT_CAN_CAST ){
		if( ! flags ){
			return 1;
		}
		if( flags & GS_CAN_TARGET ){
			if( check_special_flags2(player, card, SF2_SPELLSKITE) ){
				if( ! would_validate_target(player, card, td, 9) ){
					return 0;
				}
			}
			if( check_special_flags2(player, card, SF2_QUICKSILVER_DRAGON) ){
				td->zone = TARGET_ZONE_IN_PLAY;
				td->required_type = TYPE_CREATURE;
			}
			if( check_special_flags2(player, card, SF2_REFLECTING_MIRROR) ){
				td->zone = TARGET_ZONE_PLAYERS;
			}
			if( check_special_flags2(player, card, SF2_REBOUND) ){
				td->zone = TARGET_ZONE_PLAYERS;
			}
			if( !(flags & GS_OPTIONAL_TARGET) ){
				if( flags & GS_ONE_OR_MULTIPLE_TARGETS ){
					if( ! can_target(td) ){
						return 0;
					}
				}
				else{
					if( (flags & GS_AURA) && td->illegal_abilities > 0 ){
						if( target_available(player, card, td) == 1 ){
							//Dealing with the only two cases of "can't be the target of Aura spells"
							int p, c;
							for(p=0; p<2; p++){
								for(c=0; c<active_cards_count[c]; c++){
									if( in_play(p, c) && is_what(p, c, TYPE_PERMANENT) ){
										if( would_validate_arbitrary_target(td, p, c) ){
											if( ! is_humiliated(p, c) ){
												int csvid = get_id(p, c);
												if( csvid == CARD_ID_TETSUO_UMEZAWA || csvid == CARD_ID_BARTEL_RUNEAXE ){
													return 0;
												}
											}
										}
									}
								}
							}
						}
					}
					if( max_targets == 1 ? ! can_target(td) : (target_available(td->player, td->card, td) < max_targets) ){
						return 0;
					}
				}
			}
		}
		if( (flags & GS_X_SPELL) && check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG) ){
			return 0;
		}
		if( (flags & GS_GRAVE_RECYCLER) || (flags & GS_GRAVE_RECYCLER_OPP_GRAVE) || (flags & GS_GRAVE_RECYCLER_BOTH_GRAVES) ){
			int t_player = player;
			if( flags & GS_GRAVE_RECYCLER_OPP_GRAVE ){
				t_player = 1-player;
			}
			if( flags & GS_GRAVE_RECYCLER_BOTH_GRAVES ){
				t_player = ANYBODY;
			}
			int result = 0;
			int p;
			for(p=0; p<2; p++){
				if( p == t_player || t_player == ANYBODY ){
					if( new_special_count_grave(p, this_test) > 0 && ! graveyard_has_shroud(p) ){
						result = 1;
						break;
					}
				}
			}
			if( ! result ){
				return 0;
			}
		}
		if( (flags & GS_REGENERATION) && !(land_can_be_played & LCBP_REGENERATION) ){
			return 0;
		}
		if( (flags & GS_DAMAGE_PREVENTION) && !(land_can_be_played & LCBP_DAMAGE_PREVENTION) ){
			return 0;
		}
		if( flags & GS_CAN_ONLY_TARGET_OPPONENT ){
			int valid;
			if (td){
				instance->targets[0].player = 1-player;
				instance->targets[0].card = -1;
				instance->number_of_targets = 1;
				valid = would_validate_target(player, card, td, 0);
				instance->number_of_targets = 0;
			} else {
				valid = opponent_is_valid_target(player, card);
			}
			if (!valid){
				return 0;
			}
		}
		if (flags & GS_COUNTERSPELL){
			if (!counterspell(player, card, event, td, -1)){
				return 0;
			}
		}
		if (flags & GS_SAC_CREATURE_AS_COST){
			if( ! can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				return 0;
			}
		}

		return (flags & (GS_REGENERATION|GS_COUNTERSPELL|GS_DAMAGE_PREVENTION)) ? 99 : 1;
	}

	if( (event == EVENT_CAST_SPELL && affect_me(player, card)) || event == EVENT_CHANGE_TARGET ){
		if( !flags || check_special_flags2(player, card, SF2_FACE_DOWN_DUE_TO_MANIFEST) ){
			return 0;
		}
		// straight from "targeted_aura"
		if( flags & GS_AURA ){
			if (player == AI && !(trace_mode & 2)){
				ai_modifier += (instance->targets[0].player == td->preferred_controller) ? 24 : -24;
			}
		}
		if( event == EVENT_CHANGE_TARGET ){
			if( check_special_flags2(player, card, SF2_SPELLSKITE) ){
				int t_array[instance->number_of_targets];
				int tac = 0;
				int i;
				for(i=0; i<instance->number_of_targets; i++){
					if( instance->targets[i].card != -1 ){
						t_array[tac] = get_card_instance(instance->targets[i].player, instance->targets[i].card)->internal_card_id;
						tac++;
					}
					else{
						if( instance->targets[i].player == player ){
							t_array[tac] = get_internal_card_id_from_csv_id(CARD_ID_RULES_ENGINE);
							tac++;
						}
						else{
							t_array[tac] = get_internal_card_id_from_csv_id(CARD_ID_DEADBOX);
							tac++;
						}
					}
				}
				return show_deck( player, t_array, tac, " Select the target to change (Rules Engine = you, Deadbox = opponent)", 0, 0x7375B0 );
			}
			else{
				if( check_special_flags2(player, card, SF2_QUICKSILVER_DRAGON) ){
					td->zone = TARGET_ZONE_IN_PLAY;
					td->required_type = TYPE_CREATURE;
				}
				if( check_special_flags2(player, card, SF2_REFLECTING_MIRROR) ){
					td->zone = TARGET_ZONE_PLAYERS;
				}
				if( check_special_flags2(player, card, SF2_REBOUND) )
					td->zone = TARGET_ZONE_PLAYERS;
				if( instance->targets[0].card != -1 && !(flags & (GS_GRAVE_RECYCLER | GS_COUNTERSPELL) ) )
					state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
			}
		}
		int base_target = 0;
		if( event == EVENT_CHANGE_TARGET ){
			base_target = 9;
		}
		if( flags & GS_X_SPELL ){
			if (!is_token(player, card)){
				set_x_for_x_spells(player, card, event, x_value);
			}
			set_special_flags2(player, card, SF2_X_SPELL);
			/*
			x_value = 0; For avoiding problem with spells like Green Sun's Zenith and creatures with "X" in the casting cost.
			*/
		}

		int sac = 0;
		if( flags & GS_SAC_CREATURE_AS_COST  ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
			sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if( ! sac ){
				cancel = 1;
				goto epilogue;
			}
		}

		// Normal targeting block
		int normal_targeting = (flags & (GS_CAN_TARGET | GS_OPTIONAL_TARGET)) ? 1 : 0;
		if( check_special_flags(player, card, SF_TARGETS_ALREADY_SET) ){
			normal_targeting = 0;
		}
		if( flags & (GS_GRAVE_RECYCLER | GS_GRAVE_RECYCLER_OPP_GRAVE | GS_GRAVE_RECYCLER_BOTH_GRAVES) ){
			normal_targeting = 0;
		}
		if( normal_targeting ){
			if( check_special_flags2(player, card, SF2_MISDIRECTION) && event == EVENT_CHANGE_TARGET){
				td->allow_cancel = 0;
			}
			instance->number_of_targets = 0;
			int trgs = base_target;
			int targeted_player[2] = {0};
			while( can_target(td) && trgs < (max_targets+base_target) ){
					const char* use_prompt;
					if (flags & GS_LITERAL_PROMPT){
						use_prompt = prompt;
					}
					else {
						load_text(0, prompt);
						use_prompt = text_lines[0];
					}
					if( select_target(player, card, td, use_prompt, &(instance->targets[trgs])) ){
						if( instance->targets[trgs].card != -1 ){
							int valid = 1;
							//Dealing with the only two cases of "can't be the target of Aura spells"
							if( ! is_humiliated(instance->targets[trgs].player, instance->targets[trgs].card) ){
								int csvid = get_id(instance->targets[trgs].player, instance->targets[trgs].card);
								if( csvid == CARD_ID_TETSUO_UMEZAWA || csvid == CARD_ID_BARTEL_RUNEAXE ){
									char illegal_tgt[200];
									sprintf(illegal_tgt, EXE_STR(0x73940C)/*"Illegal target (%s)."*/, EXE_STR(0x785E65)/*"can't target this"*/);
									display_error_message(illegal_tgt);
									--instance->number_of_targets;
									valid = 0;
								}
							}
							if( valid ){
								if( !(flags & GS_ONE_OR_MULTIPLE_TARGETS) ){
									state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
								}
								else{
									add_state(instance->targets[trgs].player, instance->targets[trgs].card, STATE_TARGETTED);
								}
								trgs++;
							}
						}
						else{
							if(targeted_player[instance->targets[trgs].player]) {
								if( !(flags & GS_ONE_OR_MULTIPLE_TARGETS) ) {
									--instance->number_of_targets;
									char illegal_tgt[200];
									sprintf(illegal_tgt, EXE_STR(0x73940C)/*"Illegal target (%s)."*/, EXE_STR(0x785E65)/*"can't target this"*/);
									display_error_message(illegal_tgt);
									continue;
								}
								else{
									trgs++;
								}
							}
							else{
								targeted_player[instance->targets[trgs].player] = 1;
								trgs++;
							}
						}
					}
					else{
						break;
					}
			}
			int i;
			for(i=base_target; i<trgs; i++){
				if( instance->targets[i].card != -1 ){
					state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
					remove_state(instance->targets[i].player, instance->targets[i].card, STATE_TARGETTED);
				}
			}
			if (!(flags & GS_OPTIONAL_TARGET) ){
				if( !(flags & GS_ONE_OR_MULTIPLE_TARGETS) ){
					if( trgs < (max_targets+base_target) ){
						spell_fizzled = 1;
					}
				}
				else{
					if( trgs == base_target ){ // AKA zero targets selected
						spell_fizzled = 1;
					}
				}
			}
		}
		// End of normal targeting block

		if( flags & GS_CAN_ONLY_TARGET_OPPONENT ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = -1;
			instance->number_of_targets = 1;
		}

		if( flags & GS_COUNTERSPELL ){
			if( event == EVENT_CHANGE_TARGET ){
				// in the current Manalink, Counterspell-like cards could only change target by targetting themselves
				// Which is an invalid target for plenty of them.  Consider Remove Soul.
				instance->targets[9].player = player;
				instance->targets[9].card = card;
			}
			else{
				counterspell(player, card, event, td, 0);
				if (cancel == 1){
					goto epilogue;
				}
			}
		}

		//Targeting cards in grave
		if( flags & (GS_GRAVE_RECYCLER | GS_GRAVE_RECYCLER_OPP_GRAVE | GS_GRAVE_RECYCLER_BOTH_GRAVES) ){
			int t_player = player;
			if( flags & GS_GRAVE_RECYCLER_OPP_GRAVE ){
				t_player = 1-player;
			}
			if( flags & GS_GRAVE_RECYCLER_BOTH_GRAVES ){
				t_player = ANYBODY;
			}
			if( max_targets > 1 ){
				if( t_player == ANYBODY ){
					t_player = ANYBODY;
					target_definition_t td9;
					default_target_definition(player, card, &td9, 0);
					td9.zone = TARGET_ZONE_PLAYERS;
					td9.illegal_abilities = 0;
					if( pick_target(&td9, "TARGET_PLAYER") ){
						t_player = instance->targets[0].player;
						instance->targets[0].player = instance->targets[0].card = -1;
						instance->number_of_targets = 0;
					}
					else{
						return 0;
					}
				}
				int must_select_all = (!(flags & GS_OPTIONAL_TARGET) ? 1 : 0);
				int result = select_multiple_cards_from_graveyard(player, t_player, must_select_all, this_test->ai_selection_mode, this_test,
																	max_targets, &instance->targets[0]);
				if( spell_fizzled == 1 ){
					return 0;
				}
				if( !(flags & GS_X_SPELL) && (flags & GS_OPTIONAL_TARGET)){
					instance->info_slot = result;
				}
			}
			else{
				if( t_player != ANYBODY ){
					int result = select_target_from_grave_source(player, card, t_player, 0, this_test->ai_selection_mode, this_test, 0);
					if( result == -1 ){
						cancel = 1;
						return 0;
					}
				}
				else{
					int result = select_target_from_either_grave(player, card, 0, this_test->ai_selection_mode, this_test->ai_selection_mode, this_test, 0, 1);
					if( result == -1 ){
						cancel = 1;
						return 0;
					}
				}
			}
		}
		//End of targeting cards in grave

		if( event == EVENT_CHANGE_TARGET ){
			if( instance->targets[0].card != -1 && !(flags & (GS_GRAVE_RECYCLER | GS_COUNTERSPELL) ) ){
				state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
			}
		}

	epilogue:
		if( sac ){
			if (cancel == 1){
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
			}
			else {
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
		}
	}

	if( event == EVENT_CAN_CHANGE_TARGET ){
		if( (flags & GS_GRAVE_RECYCLER) || (flags & GS_GRAVE_RECYCLER_OPP_GRAVE) || (flags & GS_GRAVE_RECYCLER_BOTH_GRAVES) ){ // Disabled, for now.
			return 0;
		}
		if( instance->targets[0].card != -1 && !(flags & (GS_GRAVE_RECYCLER | GS_COUNTERSPELL) ) ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
		}
		int value = 0;
		if( flags & GS_COUNTERSPELL ){ // in the current Manalink, Counterspell-like cards could only change target by targetting themselves
			if( ! check_state(player, card, STATE_CANNOT_TARGET) ){
				return 1;
			}
		}
		else{
			value = generic_spell(player, card, EVENT_CAN_CAST, flags, td, prompt, max_targets, this_test);
		}
		if( instance->targets[0].card != -1 && !(flags & (GS_GRAVE_RECYCLER | GS_COUNTERSPELL) ) ){
			state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
		return value;
	}

	return 0;
}

int basic_spell(int player, int card, event_t event){
	//A spell that has no restriction such as targets, life to pay and so on, always returning 1 in EVENT_CAN_CAST and doing absolutely nothing in EVENT_CAST_SPELL
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}


int is_x_spell(int player, int card){
	card_data_t* card_d;
	if( player != -1 ){
		card_d = get_card_data(player, card);
		if( card_d->cc[1] == 255 ){
			return 1;
		}
	}
	else{
		if( cards_data[card].cc[1] == 255 ){
			return 1;
		}
	}
	return check_special_flags2(player, card, SF2_X_SPELL);
}

void fight(int p1, int c1, int p2, int c2){
	/* 701.10. Fight
	 *
	 * 701.10a A spell or ability may instruct a creature to fight another creature or it may instruct two creatures to fight each other. Each of those
	 * creatures deals damage equal to its power to the other creature.
	 *
	 * 701.10b If a creature instructed to fight is no longer on the battlefield or is no longer a creature, no damage is dealt. If a creature is an illegal
	 * target for a resolving spell or ability that instructs it to fight, no damage is dealt.
	 *
	 * 701.10c If a creature fights itself, it deals damage equal to its power to itself twice.
	 *
	 * 701.10d The damage dealt when a creature fights isn't combat damage. */

	card_instance_t* i1 = in_play(p1, c1);
	if (!i1 || !is_what(p1, c1, TYPE_CREATURE)){
		return;
	}
	card_instance_t* i2 = in_play(p2, c2);
	if (!i2 || !is_what(p2, c2, TYPE_CREATURE)){
		return;
	}

	i1->regen_status |= KEYWORD_RECALC_POWER;
	int pow1 = get_abilities(p1, c1, EVENT_POWER, -1);

	i2->regen_status |= KEYWORD_RECALC_POWER;
	int pow2 = get_abilities(p2, c2, EVENT_POWER, -1);

	play_sound_effect(WAV_ASWANJAG);

	damage_creature(p2, c2, pow1, p1, c1);
	damage_creature(p1, c1, pow2, p2, c2);

	dispatch_event(p1, c1, EVENT_FIGHT);
	dispatch_event(p2, c2, EVENT_FIGHT);
}

void upkeep_trigger_ability_mode(int player, int card, event_t event, int whose_upkeep, resolve_trigger_t trigger_mode){
	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) &&
		(whose_upkeep == current_turn || whose_upkeep == 2) &&
		!is_humiliated(player, card)
	  ){
		int count = count_upkeeps(current_turn);
		if(event == EVENT_TRIGGER && count > 0){
			if (trigger_mode == RESOLVE_TRIGGER_DUH){
				trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;
			}
			event_result |= trigger_mode;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			while( count > 0 && call_card_function(player, card, EVENT_UPKEEP_TRIGGER_ABILITY) != -1){
				count--;
			}
		}
	}
}

void upkeep_trigger_ability(int player, int card, event_t event, int whose_upkeep){
	upkeep_trigger_ability_mode(player, card, event, whose_upkeep, RESOLVE_TRIGGER_MANDATORY);
}

// Untap all cards with type types controlled by player during his opponent's untap step.  Stores temporary data in where_to_store.
void untap_permanents_during_opponents_untap(int player, int card, type_t types, int* where_to_store){
	if (current_phase == PHASE_UNTAP && current_turn == 1-player){
		if (*where_to_store != 77){
			manipulate_all(player, card, player, types, MATCH, 0, 0, 0, 0, 0, 0, -1, 0, ACT_UNTAP);
			*where_to_store = 77;
		}
	} else if (*where_to_store == 77){
		*where_to_store = 0;
	}
}

void set_flags_when_spell_is_countered(int player, int card, int c_player, int c_card){
	if( is_what(c_player, c_card, TYPE_CREATURE) && player != c_player ){
		increase_trap_condition(c_player, TRAP_SUMMONING_TRAP, 1);
	}
}

void damage_target_creature_or_player(int to_damage_player, int to_damage_card, int dmg_amount, int dmg_source_player, int dmg_source_card){
	if( to_damage_card != -1 ){
		damage_creature(to_damage_player, to_damage_card, dmg_amount, dmg_source_player, dmg_source_card);
	}
	else{
		damage_player(to_damage_player, dmg_amount, dmg_source_player, dmg_source_card);
	}
}

void true_transform(int player, int card){

	card_instance_t *instance = get_card_instance(player, card);

	int id = instance->targets[13].player;
	if( instance->targets[13].player == instance->targets[13].card ){
		id++;
	}
	instance->targets[12].card = get_internal_card_id_from_csv_id(id);
	instance->targets[13].card = id;
}

int effect_this_dies(int player, int card, event_t event)
{
  /* Local storage:
   * BYTE3(eot_toughness): trigger mode, or 0 if this is already triggering
   * targets[] and all counters: same as source card */
  if (trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller && affect_me(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (event == EVENT_TRIGGER)
		{
		  resolve_trigger_t trigger_mode = BYTE3(instance->eot_toughness);
		  if (trigger_mode == RESOLVE_TRIGGER_DUH)
			trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

		  event_result |= trigger_mode;
		}
	  else if (event == EVENT_RESOLVE_TRIGGER && BYTE3(instance->eot_toughness))
		{
		  SET_BYTE3(instance->eot_toughness) = 0;	// prevent reentrance
		  call_card_fn((void*)(cards_data[get_internal_card_id_from_csv_id(instance->display_pic_csv_id)].code_pointer),
					   instance, player, card, EVENT_RESOLVE_THIS_DIES_TRIGGER);
		  kill_card(player, card, KILL_REMOVE);
		}
	  else if (event == EVENT_END_TRIGGER)
		kill_card(player, card, KILL_REMOVE);
	}
  return 0;
}

static int this_dies_trigger_impl(int player, int card, event_t event, resolve_trigger_t trigger_mode, int who_gets_trigger, int attached)
{
  if (event == EVENT_GRAVEYARD_FROM_PLAY)
	{
	  int t_player, t_card;
	  card_instance_t* instance, *t_inst;

	  if (attached)
		{
		  instance = in_play(player, card);

		  if (!instance || instance->damage_target_player < 0 || instance->damage_target_card < 0)
			return 0;

		  t_player = instance->damage_target_player;
		  t_card = instance->damage_target_card;

		  t_inst = in_play(instance->damage_target_player, instance->damage_target_card);
		}
	  else
		{
		  t_player = player;
		  t_card = card;
		  instance = t_inst = in_play(player, card);
		}

	  if (!t_inst || !affect_me(t_player, t_card) || t_inst->kill_code == KILL_REMOVE
		  || check_special_flags2(t_player, t_card, SF2_WILL_BE_EXILED_IF_PUTTED_IN_GRAVE))
		return 0;

	  int legacy;
	  if (player == who_gets_trigger)
		legacy = create_legacy_effect(player, card, effect_this_dies);
	  else
		{
		  ASSERT(who_gets_trigger == 1-player);
		  legacy = create_legacy_effect_for_opponent(player, card, effect_this_dies, -1, -1);
		}

	  ASSERT(legacy != -1);
	  card_instance_t* leg = get_card_instance(who_gets_trigger, legacy);
	  if (get_owner(t_player, t_card) == 0)
		leg->state &= ~STATE_OWNED_BY_OPPONENT;
	  else
		leg->state |= STATE_OWNED_BY_OPPONENT;

	  memcpy(leg->targets, instance->targets, sizeof(instance->targets));
	  leg->number_of_targets = instance->number_of_targets;
	  leg->eot_toughness = 1;	// Always show the first alternate text if the text has alternates
	  SET_BYTE3(leg->eot_toughness) = trigger_mode;

	  // for e.g. Vexing Sphinx
	  leg->special_counters = instance->special_counters;
	  leg->special_counter_type = instance->special_counter_type;
	  leg->counters2 = instance->counters2;
	  leg->counters3 = instance->counters3;
	  leg->counters4 = instance->counters4;
	  leg->counters = instance->counters;
	  leg->counters5 = instance->counters5;
	  leg->counters_m1m1 = instance->counters_m1m1;

	  leg->damage_target_player = instance->damage_target_player;
	  leg->damage_target_card = instance->damage_target_card;

	  // Store last known power/toughness
	  if (is_what(player, card, TYPE_CREATURE))
		{
		  leg->power = get_power(player, card);
		  leg->toughness = get_toughness(player, card);
		}
	}

  return event == EVENT_RESOLVE_THIS_DIES_TRIGGER;
}

int this_dies_trigger(int player, int card, event_t event, resolve_trigger_t trigger_mode)
{
  return this_dies_trigger_impl(player, card, event, trigger_mode, player, 0);
}

/* Same as this_dies_trigger(), but the card's owner, not its controller, gets the legacy effect if card is stolen as it dies.  In particular, when this returns
 * true, player will be the owner since it's the legacy effect, not the original card, at that point. */
int this_dies_trigger_for_owner(int player, int card, event_t event, resolve_trigger_t trigger_mode)
{
  return this_dies_trigger_impl(player, card, event, trigger_mode, get_owner(player, card), 0);
}

int attached_creature_dies_trigger(int player, int card, event_t event, resolve_trigger_t trigger_mode)
{
  return this_dies_trigger_impl(player, card, event, trigger_mode, player, 1);
}

// controller of attached creature, not of {player,card}, gets the trigger
int attached_creature_dies_trigger_for_controller(int player, int card, event_t event, resolve_trigger_t trigger_mode)
{
  return this_dies_trigger_impl(player, card, event, trigger_mode, get_card_instance(player, card)->damage_target_player, 1);
}

// exiledby.c is more flexible
int exile_permanent_and_remember_it(int player, int card, int t_player, int t_card, int store_loc){
	card_instance_t *instance = get_card_instance( player, card );
	int result = -1;
	if( instance->targets[store_loc].card < store_loc+1 ){
		instance->targets[store_loc].card = store_loc+1;
	}
	int max_pos = is_what(player, card, TYPE_EFFECT) ? 18 : 10;
	int pos = instance->targets[store_loc].card;
	if( pos < max_pos && ! is_token(t_player, t_card) ){
		instance->targets[pos].player = is_stolen(t_player, t_card) ? 1-t_player : t_player;
		instance->targets[pos].card = get_original_internal_card_id(t_player, t_card);
		if( max_pos == 10 ){
			create_card_name_legacy(player, card, get_id(t_player, t_card));
		}
		instance->targets[store_loc].card++;
		result = instance->targets[pos].card;
	}
	kill_card(t_player, t_card, KILL_REMOVE);
	return result;
}

static int remove_untargettable_legacy(int player, int card, event_t event){
	card_instance_t* instance = get_card_instance(player, card);
	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( trigger_cause_controller == instance->targets[0].player && check_state(trigger_cause_controller, trigger_cause, STATE_CANNOT_TARGET) ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					state_untargettable(trigger_cause_controller, trigger_cause, 0);
					kill_card(player, card, KILL_REMOVE);
			}
		}
	}
	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

void type_uncounterable(int player, int card, event_t event, int t_player, int type, test_definition_t *this_test /*optional*/){
	if( is_what(player, card, TYPE_PERMANENT) && is_humiliated(player, card) ){
		return;
	}
	if( event == EVENT_CAST_SPELL && !affect_me(player, card) && !is_what(affected_card_controller, affected_card, TYPE_LAND) && in_play(player, card)){
		if( t_player == affected_card_controller || t_player == ANYBODY ){
			if( this_test != NULL ){
				if( new_make_test(player, get_card_instance(affected_card_controller, affected_card)->internal_card_id, -1, this_test) ){
					state_untargettable(affected_card_controller, affected_card, 1);
				}
			}
			if( type ){
				if( is_what(affected_card_controller, affected_card, type) ){
					state_untargettable(affected_card_controller, affected_card, 1);
				}
			}
			if( this_test == NULL && ! type ){
				state_untargettable(affected_card_controller, affected_card, 1);
			}
		}
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int legacy = create_legacy_effect(player, card, &remove_untargettable_legacy);
		get_card_instance(player, legacy)->targets[0].player = player;
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		if( (trigger_cause_controller == t_player || t_player == ANYBODY) && check_state(trigger_cause_controller, trigger_cause, STATE_CANNOT_TARGET) ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					state_untargettable(trigger_cause_controller, trigger_cause, 0);
			}
		}
	}
}

int my_damage_cannot_be_prevented(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *instance = get_card_instance(player, card);

		int t_player = is_what(player, card, TYPE_EFFECT) ? instance->targets[0].player : player;
		int t_card = is_what(player, card, TYPE_EFFECT) ? instance->targets[0].card : card;

		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->damage_source_player == t_player && source->damage_source_card == t_card ){
			state_untargettable(affected_card_controller, affected_card, 1);
		}
	}

	if( event == EVENT_CLEANUP && is_what(player, card, TYPE_EFFECT) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;

}

int damage_cannot_be_prevented_until_eot_legacy(int player, int card, event_t event){
	// tentative for 'damage_cannot_be_prevented'
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id ){
			state_untargettable(affected_card_controller, affected_card, 1);
		}
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int damage_cannot_be_prevented_until_eot(int player, int card){
	return create_legacy_effect(player, card, &damage_cannot_be_prevented_until_eot_legacy);
}

int is_artifact_creature_by_internal_id(int iid, int unused1, int unused2, int unused3){
	return is_what(-1, iid, TYPE_CREATURE) && is_what(-1, iid, TYPE_ARTIFACT) ? 1 : 0;
}

void global_type_change(int player, int card, event_t event, int t_player, int added_type, test_definition_t *this_test, int pow, int tou, int key, int subtype, int clr){
	if (event == EVENT_CHANGE_TYPE && in_play(player, card) ){
		if( affected_card_controller == t_player ||t_player == 2 ){
			if( ! is_what(affected_card_controller, affected_card, TYPE_EFFECT) &&
				new_make_test_in_play(affected_card_controller, affected_card, -1, this_test)
			  ){
				if (!(land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS)){
					// allow effects that change type to or from forest to be fully accounted for before animating
					land_can_be_played |= LCBP_NEED_EVENT_CHANGE_TYPE_SECOND_PASS;
				}
				else{
					int newtype = *(int*)(0x7a5380);	// first dynamic iid
					int end = newtype + 16;

					for (; newtype < end; ++newtype){
						if (cards_data[newtype].id == cards_data[event_result].id ){
							if( new_make_test(player, newtype, -1, this_test) ){
								event_result = newtype;
								goto found;
							}
						}
					}

					newtype = create_a_card_type(event_result);
					if (newtype != -1){
						cards_data[newtype].type |= added_type;
						if( pow == 16384 )
							pow = get_cmc(affected_card_controller, affected_card);
						cards_data[newtype].power = pow;
						if( tou == 16384 )
							tou = get_cmc(affected_card_controller, affected_card);
						cards_data[newtype].toughness = tou;
						cards_data[newtype].static_ability = key;
						event_result = newtype;
					}

					found:
						get_card_instance(affected_card_controller, affected_card)->token_status |= STATUS_ANIMATED;
						if (subtype > 0 && ! has_subtype(affected_card_controller, affected_card, subtype)){
							add_a_subtype(affected_card_controller, affected_card, subtype);
						}
				}
			}
		}
	}

	if (event == EVENT_SET_COLOR && in_play(player, card) && clr > 0){
		if( affected_card_controller == t_player ||t_player == 2 ){
			if( new_make_test_in_play(affected_card_controller, affected_card, -1, this_test) ){
				event_result = get_sleighted_color_test(player, card, clr);
			}
		}
	}

	if (leaves_play(player, card, event) && subtype > 0){
		int p, c;
		for (p = 0; p < 2; ++p){
			for (c = 0; c < active_cards_count[p]; ++c){
				if (in_play(p, c) && new_make_test_in_play(p, c, -1, this_test) ){
					reset_subtypes(p, c, 2);
				}
			}
		}
	}
}

/* Translates type into text, writing at most maxlen characters into buf (including terminating NUL).  Returns buf. */
char* type_text(char* buf, int maxlen, type_t type, type_text_t flags)
{
  if (ai_is_speculating == 1){
	*buf = 0;
	return buf;
  }

  const char* types[17] = {0};
  int num_types = 0;

  load_text(0, "TYPE");

  if (!(flags & TYPETEXT_NO_ABBREV))
	{
	  // Abbreviate "instant interrupt" into "instant"
	  if ((type & (TYPE_INSTANT|TYPE_INTERRUPT)) == (TYPE_INSTANT|TYPE_INTERRUPT))
		type &= ~TYPE_INTERRUPT;

	  // Abbreviate "artifact creature enchantment instant land sorcery" (and possibly "planeswalker") into "card"
	  if ((type & (TYPE_PERMANENT | TYPE_INSTANT | TYPE_SORCERY)) == (TYPE_PERMANENT | TYPE_INSTANT | TYPE_SORCERY))	// Note TYPE_INTERRUPT is already gone
		{
		  type &= ~(TYPE_NONEFFECT | TARGET_TYPE_PLANESWALKER);
		  types[num_types++] = text_lines[2];
		}

	  // Abbreviate "artifact creature enchantment land" (and possibly "planeswalker") into "permanent"
	  if ((type & TYPE_PERMANENT) == TYPE_PERMANENT)
		{
		  type &= ~(TYPE_PERMANENT | TARGET_TYPE_PLANESWALKER);
		  types[num_types++] = text_lines[9];
		}
	}

#define TYPE(typ, idx)	if (type & typ)	types[num_types++] = text_lines[idx]
  TYPE(TYPE_ARTIFACT, 1);
  TYPE(TYPE_CREATURE, 3);
  TYPE(TYPE_ENCHANTMENT, 5);
  TYPE(TYPE_INSTANT, 6);
  TYPE(TYPE_INTERRUPT, 7);
  TYPE(TYPE_LAND, 8);
  TYPE(TARGET_TYPE_PLANESWALKER, 12);
  TYPE(TYPE_SORCERY, 14);

  /* Sort even though those types are added alphabetically, in case "card" or "permanent" were added to the front of the array, or the text is localized (though
   * "non" and "or" below won't be) */
  sort_ArrayConstCharPtr(types, num_types);

  // "Effect" always gets added after real types
  TYPE(TYPE_EFFECT, 4);

  // If nothing so far, add a "None"
  if (num_types == 0)
	types[num_types++] = text_lines[0];

  const char* non_txt = (flags & TYPETEXT_INVERT) ? "non" : "";

  if (num_types == 1)
	scnprintf(buf, maxlen, "%s%s", non_txt, types[0]);
  else
	{
	  int i;
	  char* p = buf;
	  int end = num_types - ((flags & TYPETEXT_INVERT) ? 1 : 2);
	  for (i = 0; i < end; ++i)
		p += scnprintf(p, maxlen - (p - buf), "%s%s, ", non_txt, types[i]);

	  if (flags & TYPETEXT_INVERT)
		scnprintf(p, maxlen - (p - buf), "%s%s", non_txt, types[num_types - 1]);
	  else
		scnprintf(p, maxlen - (p - buf), "%s or %s", types[num_types - 2], types[num_types - 1]);
	}

  return buf;
#undef TYPE
}

void untap_only_1_permanent_per_upkeep(int player, int card, event_t event, int t_player, int type){
	target_definition_t td;
	default_target_definition(player, card, &td, type);
	td.required_state = TARGET_STATE_TAPPED;
	td.illegal_abilities = 0;
	td.allowed_controller = t_player != 2 ? t_player : current_turn;
	td.preferred_controller = t_player != 2 ? t_player : current_turn;
	td.who_chooses = t_player != 2 ? t_player : current_turn;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	upkeep_trigger_ability(player, card, event, t_player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		char prompt[200];
		if (ai_is_speculating == 1){
			prompt[0] = 0;
		} else {
			char typetext[100];
			scnprintf(prompt, 200, "PROCESSING %s: Select %s to untap.", cards_ptr[get_id(player, card)]->full_name, type_text(typetext, 100, type, 0));
		}
		instance->number_of_targets = 0;
		if( can_target(&td) && pick_next_target_noload(&td, prompt) ){
			instance->number_of_targets = 0;
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && (current_turn == t_player || t_player == 2) && is_what(affected_card_controller, affected_card, type) ){
		card_instance_t *instance2= get_card_instance(affected_card_controller, affected_card);
		instance2->untap_status &= ~3;
	}
}

// Resolve a Twiddle-like effect.
void twiddle(int player, int card, int tgtnum)
{
  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->targets[tgtnum].player;
  int c = instance->targets[tgtnum].card;

  load_text(0, "TWIDDLE");
  switch (DIALOG(player, card, EVENT_ACTIVATE,
				 DLG_RANDOM,
				 DLG_NO_STORAGE,
				 DLG_SMALLCARD(p, c),
				 text_lines[1]/*Tap.*/, 1,		p == AI ? 1 : 3,
				 text_lines[2]/*Untap.*/, 1,	p == AI ? 3 : 1))
	{
	  case 1:
		tap_card(p, c);
		break;

	  case 2:
		untap_card(p, c);
		break;
	}
}

// Change ai_modifier for a Twiddle-like effect.  Should happen in EVENT_ACTIVATE or EVENT_CAST_SPELL after targetting.
void ai_modifier_twiddle(int player, int card, int tgtnum)
{
  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->targets[tgtnum].player;
  int c = instance->targets[tgtnum].card;

  if (!is_what(p, c, TYPE_CREATURE)
	  && (!(get_card_data(p, c)->extra_ability & (EA_ACT_ABILITY | EA_ACT_INTERRUPT))	// No nonmana activated abilities
		  || !is_what(p, c, TYPE_CREATURE|TYPE_ARTIFACT|TYPE_LAND)))	// mono-type enchantment or planeswalker
	ai_modifier -= 24;

  if (!(p == AI) != !(is_tapped(p, c))	// AI's tapped permanent, or human's untapped
	  || (player == p && card == c))
	ai_modifier -= 96;
}

void remove_from_combat(int player, int card)
{
  card_instance_t* instance = get_card_instance(player, card);

  instance->blocking = -1;

  if (instance->state & STATE_ATTACKING)
	instance->state |= STATE_ATTACKED;

  if (instance->state & STATE_BLOCKING)
	instance->state |= STATE_BLOCKED;

  instance->state &= ~(STATE_ATTACKING | STATE_BLOCKING);
}

static int get_pt_for_creature_with_variable_pt(int player, int iid, event_t event)
{
  int curr = event == EVENT_POWER ? cards_data[iid].power : cards_data[iid].toughness;
  if ((curr > 0 && (curr & 0x4000)))
	{
	  curr &= ~0x4000;

	  int old_affected_card_controller = affected_card_controller;
	  int old_affected_card = affected_card;
	  int old_event_result = event_result;

	  affected_card_controller = player;
	  affected_card = -1;
	  event_result = curr;
	  call_card_fn((void*)cards_data[iid].code_pointer, NULL, player, -1, event);
	  curr = event_result;

	  affected_card_controller = old_affected_card_controller;
	  affected_card = old_affected_card;
	  event_result = old_event_result;
	}
  return curr;
}

int get_base_power(int player, int card)
{
	card_instance_t* instance = get_card_instance(player, card);
	int iid = instance->internal_card_id;
	if (iid == -1)
		iid = instance->backup_internal_card_id;
	return get_pt_for_creature_with_variable_pt(player, iid, EVENT_POWER);
}

int get_base_power_iid(int owner, int iid){
	return get_pt_for_creature_with_variable_pt(owner, iid, EVENT_POWER);
}

int get_base_toughness(int player, int card)
{
	card_instance_t* instance = get_card_instance(player, card);
	int iid = instance->internal_card_id;
	if (iid == -1)
		iid = instance->backup_internal_card_id;
	return get_pt_for_creature_with_variable_pt(player, iid, EVENT_TOUGHNESS);
}

int get_base_toughness_iid(int owner, int iid)
{
	return get_pt_for_creature_with_variable_pt(owner, iid, EVENT_TOUGHNESS);
}

static event_t hack_current_event = 0;	// There's a couple exe variables that look like they always hold the current event (or maybe trigger), but I'm not 100% certain of any of them.
static const char* has_mana_to_cast_target(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return has_mana_to_cast_id(who_chooses, hack_current_event, get_id(player, card)) ? NULL : "not enough mana";
}
int can_play_cards_as_though_they_had_flash(int player, int card, event_t event, target_definition_t* td, const char* prompt, int literal)
{
  if (event != EVENT_CAN_ACTIVATE && event != EVENT_ACTIVATE)
	return 0;

  if ((IS_AI(player) || event == EVENT_CAN_ACTIVATE) && td->extra == -1)
	{
	  td->special |= TARGET_SPECIAL_EXTRA_FUNCTION;
	  td->extra = (int)has_mana_to_cast_target;
	}

  hack_current_event = event;

  if (event == EVENT_CAN_ACTIVATE)
	return can_target(td);

  if (event == EVENT_ACTIVATE)
	{
	  if (!literal)
		{
		  load_text(0, prompt);
		  prompt = text_lines[0];
		}

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (pick_next_target_noload(td, prompt)
		  && charge_mana_from_id(player, instance->targets[0].card, event, -1))
		{
		  instance->number_of_targets = 0;
		  play_card_in_hand_for_free(instance->targets[0].player, instance->targets[0].card);
		  cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
		}
	}

  return 0;

}

// See effect_werewolf_ransacker() in dark_ascension.c for intended usage.
int effect_put_into_a_graveyard_this_way(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_GRAVEYARD_FROM_PLAY)
	{
	  if (affect_me(instance->damage_target_player, instance->damage_target_card))	// detach, so this isn't immediately buried
		instance->damage_target_player = instance->damage_target_card = -1;
	}

  if (event == EVENT_STATIC_EFFECTS
	  || (trigger_condition == TRIGGER_LEAVE_PLAY
		  && affect_me(player, card) && reason_for_trigger_controller == player
		  && trigger_cause_controller == instance->targets[0].player && trigger_cause == instance->targets[0].card))
	{
	  if (instance->targets[0].player < 0 || instance->targets[0].card < 0 || instance->targets[3].player == 42)
		{
		  kill_card(player, card, KILL_REMOVE);
		  return 0;
		}

	  // May need to hack put_on_top_of_deck(), oubliette_helper_5(), and bounce_permanent() to clear STATUS_DYING and/or kill_code before triggering.
	  card_instance_t* aff = get_card_instance(instance->targets[0].player, instance->targets[0].card);
	  if (!(aff->token_status & STATUS_DYING)
		  || (aff->kill_code != KILL_DESTROY && aff->kill_code != KILL_BURY))
		{
		  kill_card(player, card, KILL_REMOVE);
		  return 0;
		}

	  if (event == EVENT_TRIGGER)
		return RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  instance->targets[3].player = 42;
		  return 1;
		}
	}

  if (event == EVENT_PHASE_CHANGED || event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

// If test is NULL, then it defaults to all non-token cards.  Only needs to be called (and thus test only needs to be set) during EVENT_GRAVEYARD_FROM_PLAY.
void if_a_card_would_be_put_into_graveyard_from_play_exile_it_instead(int player, int card, int event, int t_player, test_definition_t* test /*optional*/)
{
  card_instance_t* aff;
  if (event == EVENT_GRAVEYARD_FROM_PLAY && in_play(player, card) && !is_humiliated(player, card)
	  && (affected_card_controller == t_player || t_player == ANYBODY)
	  && (aff = in_play(affected_card_controller, affected_card))
	  && aff->kill_code > 0 && aff->kill_code < KILL_REMOVE
	  && (test ? new_make_test_in_play(affected_card_controller, affected_card, -1, test) : !is_token(affected_card_controller, affected_card)))
	aff->kill_code = KILL_REMOVE;
}

/* Must set replace_milled as the very last thing after this returns true if anything is done with the card.  If test is NULL, then it defaults to all cards.
 * Only needs to be called (and thus test only needs to be set) if xtrigger_condition() == XTRIGGER_REPLACE_MILL. */
int if_a_card_would_be_put_into_graveyard_from_library_do_something_instead(int player, int card, int event, int t_player, resolve_trigger_t trigger_mode, test_definition_t* test /*optional*/)
{
  enable_xtrigger_flags |= ENABLE_XTRIGGER_REPLACE_MILL;

  if (xtrigger_condition() == XTRIGGER_REPLACE_MILL && affect_me(player, card) && reason_for_trigger_controller == player && !replace_milled
	  && (trigger_cause_controller == t_player || t_player == ANYBODY) && !is_humiliated(player, card)
	  && (!test || new_make_test(trigger_cause_controller, deck_ptr[trigger_cause_controller][trigger_cause], -1, test)))
	{
	  if (event == EVENT_TRIGGER)
		{
		  if (trigger_mode == RESOLVE_TRIGGER_DUH)
			trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

		  event_result |= trigger_mode;
		}
	  if (event == EVENT_RESOLVE_TRIGGER)
		return 1;
	}

  return 0;
}

/* Must set replace_milled as the very last thing after this returns true if anything is done with the card.  If test is NULL, then it defaults to all cards.
 * Only needs to be called (and thus test only needs to be set) if xtrigger_condition() == XTRIGGER_REPLACE_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY. */
int if_a_card_would_be_put_into_graveyard_from_anywhere_but_library_do_something_instead(int player, int card, int event, int t_player, resolve_trigger_t trigger_mode, test_definition_t* test /*optional*/)
{
  enable_xtrigger_flags |= ENABLE_XTRIGGER_REPLACE_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY;

  if (xtrigger_condition() == XTRIGGER_REPLACE_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY && affect_me(player, card) && reason_for_trigger_controller == player
	  && !replace_milled
	  && (trigger_cause_controller == t_player || t_player == ANYBODY) && !is_humiliated(player, card)
	  && (!test || new_make_test(trigger_cause_controller, trigger_cause, -1, test)))
	{
	  if (event == EVENT_TRIGGER)
		{
		  if (trigger_mode == RESOLVE_TRIGGER_DUH)
			trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

		  event_result |= trigger_mode;
		}
	  if (event == EVENT_RESOLVE_TRIGGER)
		return 1;
	}

  return 0;
}

/* A front end to XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY.  trigger_cause_controller (whose graveyard), trigger_cause (iid of card), gy_from_anywhere_pos
 * (tentative position), and gy_from_anywhere_source (canonical graveyard_source[][] value) are all still valid.  Only needs to be called (and thus test only
 * needs to be set) if xtrigger_condition() == XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY. */
int when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(int player, int card, int event, int t_player, resolve_trigger_t trigger_mode, test_definition_t* test /*optional*/)
{
  enable_xtrigger_flags |= ENABLE_XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY;

  if (xtrigger_condition() == XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY && affect_me(player, card) && reason_for_trigger_controller == player
	  && (trigger_cause_controller == t_player || t_player == ANYBODY) && !is_humiliated(player, card)
	  && (!test || new_make_test(trigger_cause_controller, trigger_cause, -1, test)))
	{
	  if (event == EVENT_TRIGGER)
		{
		  if (trigger_mode == RESOLVE_TRIGGER_DUH)
			trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

		  event_result |= trigger_mode;
		}
	  if (event == EVENT_RESOLVE_TRIGGER)
		return 1;
	}

  return 0;
}

/* A fairly common case.  If test is NULL, then it defaults to all non-token cards.  Only needs to be called (and thus test only needs to be set) during
 * EVENT_GRAVEYARD_FROM_PLAY, XTRIGGER_REPLACE_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY, and XTRIGGER_REPLACE_MILL. */
void if_a_card_would_be_put_into_graveyard_from_anywhere_exile_it_instead(int player, int card, int event, int t_player, test_definition_t* test /*optional*/)
{
  if (if_a_card_would_be_put_into_graveyard_from_library_do_something_instead(player, card, event, t_player, RESOLVE_TRIGGER_MANDATORY, test))
	{
	  rfg_top_card_of_deck(trigger_cause_controller);
	  replace_milled = 1;
	}
  if (if_a_card_would_be_put_into_graveyard_from_anywhere_but_library_do_something_instead(player, card, event, t_player, RESOLVE_TRIGGER_MANDATORY, test))
	{
	  add_card_to_rfg(trigger_cause_controller, trigger_cause);
	  play_sound_effect(WAV_DESTROY);
	  replace_milled = 1;
	}
  if_a_card_would_be_put_into_graveyard_from_play_exile_it_instead(player, card, event, t_player, test);
}

int can_play_iid(int player, int event, int iid){
	if( can_legally_play_iid(player, iid) && has_mana_to_cast_iid(player, event, iid) )
		return 1;
	return 0;
}

void cannot_attack_alone(int player, int card, event_t event){
  if (event == EVENT_ATTACK_LEGALITY && affect_me(player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.illegal_abilities = 0;
	  td.required_state = TARGET_STATE_ATTACKING;

	  if (!can_target(&td))
		event_result = 1;
	}
}

void cannot_block_alone(int player, int card, event_t event){
  if ( event == EVENT_BLOCK_LEGALITY && affect_me(player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.illegal_abilities = 0;
	  td.required_state = TARGET_STATE_BLOCKING;

	  if (!can_target(&td))
		event_result = 1;
	}
}

int nightveil_specter_like_ability(int player, int card, event_t event, unsigned int flags, unsigned int cards_to_exile, unsigned int csvid_to_display){

	int exby_flags = (flags & NSLA_MUST_PAY_MANACOST_OF_EXILED_CARDS) ? EXBY_TEST_HAS_MANA_TO_CAST : 0;

	int ddbm_flags = (flags & NSLA_EXILE_ONLY_WITH_COMBAT_DAMAGE) ? DDBM_MUST_BE_COMBAT_DAMAGE : 0;
	ddbm_flags |= (flags & NSLA_EXILE_ONLY_WHEN_DAMAGING_PLAYER) ? DDBM_MUST_DAMAGE_PLAYER : 0;

	if( damage_dealt_by_me(player, card, event, ddbm_flags) ){
		int t_player = 1-player;
		if( flags & NSLA_EXILE_FROM_PLAYERS_DECK ){
			t_player = player;
		}
		unsigned int count = 0;
		while( count < cards_to_exile && deck_ptr[t_player][0] != -1){
				exiledby_remember(player, card, t_player, deck_ptr[t_player][0], NULL, NULL);
				rfg_card_in_deck(t_player, 0);
				count++;
		}
	}

  if (event == EVENT_CAN_ACTIVATE)
	{
	  int can_cast = exiledby_choose(player, card, csvid_to_display, EXBY_CAN_CAST|EXBY_TEST_CAN_CAST|exby_flags, event, NULL, 1);
	  if (IS_AI(player) || can_cast)
		return can_cast;
	  else
		return 1;	// human can always list which cards have been exiled
	}

  if (event == EVENT_ACTIVATE)
	{
	  if (!IS_AI(player))
		{
		  int can_cast = exiledby_choose(player, card, csvid_to_display, EXBY_CAN_CAST|EXBY_TEST_CAN_CAST|exby_flags, event, NULL, 1);
		  if (DIALOG(player, card, event, DLG_NO_STORAGE, DLG_NO_CANCEL,
					 "Show exiled cards", 1, 1,
					 "Play an exiled card", can_cast, 1) == 1)
			{
			  exiledby_choose(player, card, csvid_to_display, EXBY_CHOOSE, 0, NULL, 1);
			  cancel = 1;
			  return 0;
			}
		}

	  int rval = exiledby_choose(player, card, csvid_to_display, EXBY_CHOOSE|EXBY_TEST_CAN_CAST|exby_flags, event, NULL, 1);
	  int* loc = (int*)rval;
	  if (!loc)
		spell_fizzled = 1;
	  else
		{
			int raw = *loc;
			int t_player = *loc & 0x80000000 ? 1 : 0;
			int iid = *loc & ~0x80000000;
			int csvid = cards_data[iid].id;
			*loc = -1;
			if( (exby_flags & EXBY_TEST_HAS_MANA_TO_CAST) ){
				charge_mana_from_id(player, -1, event, csvid);
			}
			if( cancel != 1 && play_card_in_exile_for_free(player, t_player, csvid) != -1){
				cant_be_responded_to = 1;	// response to the spell allowed by put_card_on_stack(), but none for this activation
			}
			else{
				*loc = raw;
				cancel = 1;
			}
		}
	}

	if( (flags & NSLA_REMOVE_STORED_CARDS_AT_EOT) && event == EVENT_CLEANUP ){
		int id = exiledby_detach(player, card);
		exiledby_destroy_detached(player, id);
	}

	return 0;
}

void prevent_all_my_damage(int player, int card, event_t event, int flags){
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 ){
			if( source->damage_source_card == card && source->damage_source_player == player ){
				int nd = 1;
				if( (flags & DDBM_MUST_BE_COMBAT_DAMAGE) && !(source->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE)) ){
					nd = 0;
				}
				if( (flags & DDBM_MUST_DAMAGE_PLAYER) &&
					(source->damage_target_card != -1 || check_special_flags(source->damage_source_player, source->damage_source_card, SF_ATTACKING_PWALKER))
				  ){
					nd = 0;
				}
				if( (flags & DDBM_MUST_DAMAGE_OPPONENT) && (source->damage_target_card != -1 || source->damage_target_player != 1-player) ){
					nd = 0;
				}
				if( nd ){
					source->info_slot = 0;
				}
			}
		}
	}
}

static int prevent_all_damage_dealt_by_target_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 ){
		prevent_all_my_damage(instance->damage_target_player, instance->damage_target_card, event, instance->targets[1].player);
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int prevent_all_damage_dealt_by_target(int player, int card, int t_player, int t_card, int ddbm_flags){
	int legacy = create_targetted_legacy_effect(player, card, &prevent_all_damage_dealt_by_target_legacy, t_player, t_card);
	get_card_instance(player, legacy)->targets[1].player = ddbm_flags;
	return legacy;
}

int card_drawn_trigger(int player, int card, event_t event, int who_draws, resolve_trigger_t trigger_mode)
{
  if (trigger_condition == TRIGGER_CARD_DRAWN && affect_me(player, card) && reason_for_trigger_controller == player
	  && (trigger_cause_controller == who_draws || who_draws == ANYBODY)
	  && in_play(player, card) && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		{
		  if (trigger_mode == RESOLVE_TRIGGER_DUH)
			trigger_mode = duh_mode(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL;

		  event_result |= trigger_mode;
		}

	  if (event == EVENT_RESOLVE_TRIGGER)
		return 1;
	}

  return 0;
}

int beginning_of_phase(int player, int card, event_t event, int aff_player, int aff_card, int phase){
	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_PHASE_CHANGED && current_phase == phase && instance->info_slot == 0 && in_play(player, card)){
		if( (aff_player == ANYBODY || current_turn == aff_player) && (aff_card == -1 || aff_card == card) ){
			instance->info_slot = 1;	// prevent being asked multiple times
			return 1;
		}
	}

	if(event == EVENT_PHASE_CHANGED && current_phase != phase){
		instance->info_slot = 0;
	}

	return 0;
}

int beginning_of_combat(int player, int card, event_t event, int aff_player, int aff_card){
	return beginning_of_phase(player, card, event, player, card, PHASE_DECLARE_ATTACKERS);
}

void permanents_enters_battlefield_tapped(int player, int card, event_t event, int t_player, int type, test_definition_t *test){
	/* If this changes the events it looks at, please grep for its uses - at least one card filters by the same events
	 * so it doesn't construct a test_definition_t every single time its function is called. */
	if( (event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && ! affect_me(player, card) ){
		if( is_what(player, card, TYPE_EFFECT) || (in_play(player, card) && ! is_humiliated(player, card)) ){
			if( affected_card_controller == t_player || t_player == ANYBODY ){
				if( (type && is_what(affected_card_controller, affected_card, type)) ||
					(test != NULL && new_make_test_in_play(affected_card_controller, affected_card, -1, test))
				  ){
					if( ! check_state(affected_card_controller, affected_card, STATE_TAPPED) ){
						add_state(affected_card_controller, affected_card, STATE_TAPPED);
						if( spell_fizzled == 1 ){
							remove_state(affected_card_controller, affected_card, STATE_TAPPED);
						}
					}
				}
			}
		}
	}
}

void popup(const char* title, const char* fmt, ...)
{
  char buf[8000];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 8000, fmt, args);
  va_end(args);

  MessageBox(0, buf, title, MB_ICONERROR|MB_TASKMODAL);
}

int recycling_land(int player, int card, event_t event, int recycling_type, int cless, int black, int blue, int green, int red, int white){

	char msg[100] = "Select a card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, recycling_type, msg);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && has_mana_for_activated_ability(player, card, cless+1, black, blue, green, red, white) &&
			count_graveyard_by_type(player, recycling_type) > 0
		  ){
			ai_choice = 1;
		}

		if( ai_choice == 1){
			choice = do_dialog(player, player, card, -1, -1, " Add 1\n Recycle\n Cancel", ai_choice);
		}

		if( choice == 0){
			return mana_producer(player, card, event);
		}

		else if( choice == 1 ){
				tap_card(player, card);
				if( charge_mana_for_activated_ability(player, card, cless, black, blue, green, red, white) ){
					if( new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
						instance->info_slot = 1;
					}
					else{
						untap_card_no_event(player, card);
						spell_fizzled = 1;
					}
				}
				else{
					untap_card_no_event(player, card);
				}
		}

		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot > 0 ){
				int selected = validate_target_from_grave(player, card, player, 0);
				if( selected != -1 ){
					from_graveyard_to_deck(player, selected, 1);
				}
				card_instance_t *parent = get_card_instance( instance->parent_controller, instance->parent_card );
				parent->info_slot = 0;
			}
	}
	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_fake_card(int player, int card, event_t event){


	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			 life[player] = get_color(player, instance->parent_card);
	}

	return 0;
}

int insurrection_effect(int player, int card){

	manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_UNTAP);
	pump_subtype_until_eot(player, card, player, -1, 0, 0, 0, SP_KEYWORD_HASTE);

	int count = active_cards_count[1-player]-1;
	int result = 0;
	while( count > -1 ){
			if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
				effect_act_of_treason(player, card, 1-player, count);
				result++;
			}
			count--;
	}
	return result;
}

static int default_tutor3(int player, int card){
	return 1;
}

int tutor(int, int, int (*func_ptr)(int, int),  int);	// deprecated
int pattern_of_rebirth_tutor (int player){
	return tutor(player, TYPE_CREATURE, &default_tutor3, TUTOR_PLAY );
}

static int is_karoo(int csvid){

	switch( csvid ){
			case (CARD_ID_AZORIUS_CHANCERY):
			case (CARD_ID_BOROS_GARRISON):
			case (CARD_ID_DIMIR_AQUEDUCT):
			case (CARD_ID_GOLGARI_ROT_FARM):
			case (CARD_ID_GRUUL_TURF):
			case (CARD_ID_IZZET_BOILERWORKS):
			case (CARD_ID_NOGGLE_BRIDGEBREAKER):
			case (CARD_ID_ORZHOV_BASILICA):
			case (CARD_ID_RAKDOS_CARNARIUM):
			case (CARD_ID_SELESNYA_SANCTUARY):
			case (CARD_ID_SIMIC_GROWTH_CHAMBER):
				return 1;
				break;
			default:
				return 0;
	}
}

static const char* is_not_karoo(int who_chooses, int player, int card){
	if( ! is_karoo(get_id(player, card)) ){
		return NULL;
	}
	return "AI helper function";
}

int karoo(int player, int card, event_t event, int color1, int color2, int ai_card){
	/*
	if (player == AI && !(trace_mode & 2)){
		if (event == EVENT_CHANGE_TYPE && affect_me(player, card)){
			event_result = get_internal_card_id_from_csv_id(ai_card);
		}
		return 0;
	}
	*/
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI){
		target_definition_t td;
		base_target_definition(player, card, &td, TYPE_LAND);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.allow_cancel = 0;
		td.required_state = TARGET_STATE_TAPPED;
		td.extra = (int32_t)is_not_karoo;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

		if( can_target(&td) ){
			ai_modifier+=50;
		}
		else{
			ai_modifier-=50;
		}
	}

	comes_into_play_tapped(player, card, event);

	if (comes_into_play(player, card, event)){
		int land_to_bounce = -1;
		if( player == HUMAN ){
			target_definition_t td;
			base_target_definition(player, card, &td, TYPE_LAND);
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.allow_cancel = 0;
			td.illegal_abilities = 0;

			card_instance_t* instance = get_card_instance(player, card);
			instance->number_of_targets = 0;
			if (can_target(&td) && select_target(player, card, &td, "Choose a land to bounce", NULL)){
				land_to_bounce = instance->targets[0].card;
			}
		}
		else{
			int c;
			for(c = 0; c < active_cards_count[player]; c++){
				if( in_play(player, c) && is_tapped(player, c) && is_what(player, c, TYPE_LAND) ){
					if( ! is_karoo(get_id(player, c)) ){
						land_to_bounce = c;
						break;
					}
				}
			}
			if( land_to_bounce == -1 ){
				for(c = 0; c < active_cards_count[player]; c++){
					if( in_play(player, c) && is_what(player, c, TYPE_LAND) ){
						if( ! is_karoo(get_id(player, c)) ){
							land_to_bounce = c;
							break;
						}
					}
				}
			}
			if( land_to_bounce == -1 ){
				for(c = 0; c < active_cards_count[player]; c++){
					if( c != card && in_play(player, c) && is_what(player, c, TYPE_LAND) ){
						land_to_bounce = c;
						break;
					}
				}
			}
			if( land_to_bounce == -1 ){
				land_to_bounce = card;
			}
		}
		bounce_permanent(player, land_to_bounce);
	}

	return two_mana_land(player, card, event, color1, color2);
}

void play_land_sound_effect(int player, int card){
	card_data_t* cd = get_card_data(player, card);
	play_land_sound_effect_force_color(player, card, cd->color);
}

void play_land_sound_effect_force_color(int player, int card, int colors){
	if (!is_what(player, card, TYPE_LAND)){
		// For e.g. Magus of the Coffers and other cards call lands' functions
		return;
	}

	colors &= COLOR_TEST_ANY_COLORED;	// so e.g. COLOR_TEST_WHITE below also includes COLOR_TEST_WHITE|COLOR_TEST_COLORLESS, for Karoo; and COLOR_TEST_ARTIFACT is treated like COLOR_TEST_COLORLESS.

	wav_t wav1 = -1, wav2 = -1;
	switch (colors){
		// colorless
		case 0:					wav1 = WAV_GREY;	break;

		// one color
		case COLOR_TEST_BLACK:	wav1 = WAV_BLACK;	break;
		case COLOR_TEST_BLUE:	wav1 = WAV_BLUE;	break;
		case COLOR_TEST_GREEN:	wav1 = WAV_GREEN;	break;
		case COLOR_TEST_RED:	wav1 = WAV_RED;		break;
		case COLOR_TEST_WHITE:	wav1 = WAV_WHITE;	break;

		// two colors
		case COLOR_TEST_WHITE|COLOR_TEST_BLUE:	wav1 = WAV_WHITEBLUE;	break;
		case COLOR_TEST_BLUE|COLOR_TEST_BLACK:	wav1 = WAV_BLUEBLACK;	break;
		case COLOR_TEST_BLACK|COLOR_TEST_RED:	wav1 = WAV_BLACKRED;	break;
		case COLOR_TEST_RED|COLOR_TEST_GREEN:	wav1 = WAV_REDGREEN;	break;
		case COLOR_TEST_GREEN|COLOR_TEST_WHITE:	wav1 = WAV_GREENWHITE;	break;

		case COLOR_TEST_WHITE|COLOR_TEST_BLACK:	wav1 = WAV_WHITEBLACK;	break;
		case COLOR_TEST_BLACK|COLOR_TEST_GREEN:	wav1 = WAV_BLACKGREEN;	break;
		case COLOR_TEST_GREEN|COLOR_TEST_BLUE:	wav1 = WAV_GREENBLUE;	break;
		case COLOR_TEST_BLUE|COLOR_TEST_RED:	wav1 = WAV_BLUERED;		break;
		case COLOR_TEST_RED|COLOR_TEST_WHITE:	wav1 = WAV_REDWHITE;	break;

		// three colors - bit of a hack
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_WHITE|COLOR_TEST_BLUE):	wav1 = WAV_RED;		wav2 = WAV_BLACKGREEN;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_BLUE|COLOR_TEST_BLACK):	wav1 = WAV_GREEN;	wav2 = WAV_REDWHITE;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_BLACK|COLOR_TEST_RED):	wav1 = WAV_WHITE;	wav2 = WAV_GREENBLUE;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_RED|COLOR_TEST_GREEN):	wav1 = WAV_BLUE;	wav2 = WAV_WHITEBLACK;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_GREEN|COLOR_TEST_WHITE):	wav1 = WAV_BLACK;	wav2 = WAV_BLUERED;		break;

		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_WHITE|COLOR_TEST_BLACK):	wav1 = WAV_BLUE;	wav2 = WAV_REDGREEN;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_BLACK|COLOR_TEST_GREEN):	wav1 = WAV_RED;		wav2 = WAV_WHITEBLUE;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_GREEN|COLOR_TEST_BLUE):	wav1 = WAV_WHITE;	wav2 = WAV_BLACKRED;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_BLUE|COLOR_TEST_RED):	wav1 = WAV_BLACK;	wav2 = WAV_GREENWHITE;	break;
		case COLOR_TEST_ANY_COLORED & ~(COLOR_TEST_RED|COLOR_TEST_WHITE):	wav1 = WAV_GREEN;	wav2 = WAV_BLUEBLACK;	break;


		// four colors - slightly less of a hack.
		// (While there aren't any naturally four-color lands that I know of, these can still be played by Reflecting Pool, Exotic Orchard, etc.)
		case COLOR_TEST_ANY_COLORED & ~COLOR_TEST_BLACK:	wav1 = WAV_REDGREEN;	wav2 = WAV_WHITEBLUE;	break;
		case COLOR_TEST_ANY_COLORED & ~COLOR_TEST_BLUE:		wav1 = WAV_BLACKRED;	wav2 = WAV_GREENWHITE;	break;
		case COLOR_TEST_ANY_COLORED & ~COLOR_TEST_GREEN:	wav1 = WAV_WHITEBLUE;	wav2 = WAV_BLACKRED;	break;
		case COLOR_TEST_ANY_COLORED & ~COLOR_TEST_RED:		wav1 = WAV_GREENWHITE;	wav2 = WAV_BLUEBLACK;	break;
		case COLOR_TEST_ANY_COLORED & ~COLOR_TEST_WHITE:	wav1 = WAV_BLUEBLACK;	wav2 = WAV_REDGREEN;	break;

		// five colors - going with gembazar, even though City of Brass is silent.  (It's better than polkamix?)
		case COLOR_TEST_ANY_COLORED:			wav1 = WAV_GEMBAZAR;	break;
	}

	if (wav1 != (wav_t)-1){
		play_sound_effect(wav1);
	}
	if (wav2 != (wav_t)-1){
		play_sound_effect(wav2);
	}
}

void for_each_creature_damaged_by_me(int player, int card, event_t event, int flags, void (*fn)(int, int, int, int), int arg1, int arg2){
	card_instance_t *instance = get_card_instance(player, card);
	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t* dmg = get_card_instance(affected_card_controller, affected_card);
		if( dmg->internal_card_id == damage_card && dmg->damage_source_player == player && dmg->damage_source_card == card ){
			if( dmg->info_slot > 0 || get_card_instance(dmg->damage_source_player, dmg->damage_source_card)->targets[16].player > 0 ){
				if( dmg->damage_target_card > -1 ){
					int good = 1;
					if( (flags & DDBM_MUST_BE_COMBAT_DAMAGE) && !(dmg->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE)) ){
						good = 0;
					}
					if( good ){
						int pos = instance->info_slot;
						if( pos < 10 ){
							instance->targets[pos].player = dmg->damage_target_player;
							instance->targets[pos].card = dmg->damage_target_card;
							instance->info_slot++;
						}
					}
				}
			}
		}
	}
	if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=0; i<instance->info_slot; i++){
					if( in_play(instance->targets[i].player, instance->targets[i].card) ){
						fn(arg1, arg2, instance->targets[i].player, instance->targets[i].card);
					}
				}
				instance->info_slot = 0;
		}
	}
}

unsigned int round_down_value(unsigned int orig_value){
	unsigned int amount = orig_value/2;
	if( amount*2 != orig_value ){
		amount = (orig_value-1)/2;
	}
	return amount;
}

unsigned int round_up_value(unsigned int orig_value){
	unsigned int amount = orig_value/2;
	if( amount*2 != orig_value ){
		amount = (orig_value+1)/2;
	}
	return amount;
}

void no_more_than_x_creatures_can_attack(int player, int card, event_t event, int t_player, int max_value){
	if( current_turn == t_player || t_player == ANYBODY ){
		card_instance_t* aff = get_card_instance(affected_card_controller, affected_card);
		if (aff->state & STATE_ATTACKING){	// Already attacking
			return;
		}

		int num_attacking = 0;
		int c;
		for (c = 0; c < active_cards_count[affected_card_controller]; ++c){
			if ((aff = in_play(affected_card_controller, c)) && (aff->state & STATE_ATTACKING) && ++num_attacking >= max_value){
				event_result = 1;
				break;
			}
		}
	}
}

void no_more_than_x_creatures_can_block(int player, int card, event_t event, int t_player, int max_value){
	if( current_turn != t_player || t_player == ANYBODY ){
		card_instance_t* aff = get_card_instance(affected_card_controller, affected_card);
		if (aff->state & STATE_BLOCKING){	// Already blocking
			return;
		}

		int num_blockers = 0;
		int c;
		for (c = 0; c < active_cards_count[affected_card_controller]; ++c){
			if ((aff = in_play(affected_card_controller, c)) && (aff->state & STATE_BLOCKING) && ++num_blockers >= max_value){
				event_result = 1;
				break;
			}
		}
	}
}

int choose_a_color_and_show_legacy(int player, int card, int ai_t_player, int ret_location){
	//	return chosen color
	//	store the legacy in "targets[ret_location].card" if "ret_location" > -1.
	int clr = choose_a_color(player, get_deck_color(player, ai_t_player));
	int legacy = create_card_name_legacy(player, card, CARD_ID_BLACK+(clr-1));
	if( ret_location > -1 ){
		get_card_instance(player, card)->targets[ret_location].card = legacy;
	}
	return clr;
}

static int discard_trigger_delayed_ability(int player, int card, event_t event){
	if( eot_trigger(player, card, event) ){
		card_instance_t *instance = get_card_instance( player, card );
		int p = instance->targets[0].player;
		int csvid = instance->targets[0].card;
		int mode = instance->targets[1].player;
		if( p > -1 && csvid > -1 ){
			int count = count_graveyard(p)-1;
			while( count > -1 ){
					if( cards_data[get_grave(p)[count]].id == csvid ){
						if( mode & 1 ){
							add_card_to_hand(p, get_grave(p)[count]);
							remove_card_from_deck(p, count);
						}
						if( mode & 2 ){
							reanimate_permanent(p, -1, p, count, REANIMATE_PLUS1_PLUS1_COUNTER);
						}
						break;
					}
					count--;
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_madness_effect(int player, int card, event_t event){
	//Madness ruling changed, so this is no more used.

	int iid = get_card_instance(player, card)->targets[1].card;

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		instance->targets[1].player = current_phase;
	}

	if( get_card_instance(player, card)->targets[1].player > -1 ){
		card_instance_t *instance = get_card_instance(player, card);

		if( current_phase != instance->targets[1].player ){
			if( iid > -1 ){
				int pos = find_iid_in_rfg(player, iid);
				if( pos > -1 ){
					from_exile_to_graveyard(player, pos);
				}
			}
			kill_card(player, card, KILL_SACRIFICE);
		}

		if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player &&
		    instance->targets[2].card != 66 )
		{

			int trig = 0;

			if( ! is_what(trigger_cause_controller, trigger_cause, TYPE_LAND) ){
				trig = 1;
			}


			if( trig == 1 ){
				if( event == EVENT_TRIGGER){
					event_result |= RESOLVE_TRIGGER_MANDATORY;
				}
				else if( event == EVENT_RESOLVE_TRIGGER ){
						if( iid > -1 ){
							int pos = find_iid_in_rfg(player, iid);
							if( pos > -1 ){
								from_exile_to_graveyard(player, pos);
							}
						}
						kill_card(player, card, KILL_SACRIFICE);
				}
			}
		}

		if( event == EVENT_CAN_ACTIVATE ){
			int result = 0;
			if( iid > -1 && instance->targets[2].card != 66 ){
				int fake = add_card_to_hand(player, iid);
				add_state(player, fake, STATE_INVISIBLE);
				hand_count[player]--;
				if( call_card_function(player, fake, EVENT_CAN_PAY_MADNESS_COST) ){
					if( is_what(player, fake, TYPE_PERMANENT) ){
						result = 1;
					}
					else{
						result = call_card_function(player, fake, EVENT_CAN_CAST);
					}
				}
				obliterate_card(player, fake);
			}
			return result;
		}

		if( event == EVENT_ACTIVATE ){
			if( iid > -1 ){
				int fake = add_card_to_hand(player, iid);
				add_state(player, fake, STATE_INVISIBLE);
				hand_count[player]--;
				if( call_card_function(player, fake, EVENT_PAY_MADNESS_COST) ){
					instance->targets[2].card = 66;
					remove_state(player, fake, STATE_INVISIBLE);
					play_card_in_hand_for_free(player, fake);
					cant_be_responded_to = 1;
				}
				else{
					obliterate_card(player, fake);
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			kill_card(instance->parent_controller, instance->parent_card, KILL_SACRIFICE);
		}
	}

	return 0;
}

int madness(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	if( event == EVENT_CAN_PAY_MADNESS_COST ){
		int cless = get_updated_casting_cost(player, card, -1, EVENT_CAN_CAST, colorless);
		if( has_mana_multi(player, cless, black, blue, green, red, white) ){
			if( is_what(player, card, TYPE_CREATURE | TYPE_ARTIFACT | TYPE_LAND) ){
				return 1;
			}
			if( is_what(player, card, TYPE_ENCHANTMENT) && ! has_subtype(player, card, SUBTYPE_AURA) ){
				return 1;
			}
			return call_card_function(player, card, EVENT_CAN_CAST);
		}
	}

	if( event == EVENT_PAY_MADNESS_COST ){
		int cless = get_updated_casting_cost(player, card, -1, EVENT_CAST_SPELL, colorless);
		charge_mana_multi(player, cless, black, blue, green, red, white);
		if( spell_fizzled != 1 && colorless == -1 ){
			charge_mana(player, COLOR_ANY, -1);
		}
		if( spell_fizzled != 1 ){
			if( colorless == -1 ){
				set_x_for_x_spells(player, card, event, x_value);
			}
			set_special_flags(player, card, SF3_MADNESS_COST_PAID);
			return 1;
		}
	}

	return 0;
}

void discard_card_impl(int player, int card, int player_who_controls_effect, int flags){
  int v2; // ST14_4
  int v3; // ST10_4

  v2 = trigger_cause_controller;
  v3 = trigger_cause;
  trigger_cause_controller = player;
  trigger_cause = card;
  byte_786DD4 = 0;
  dispatch_trigger_twice_once_with_each_player_as_reason(
														 player,
														 TRIGGER_DISCARD,
														 str_Draw_a_card,
														 0);
  trigger_cause = v3;
  trigger_cause_controller = v2;
  if (byte_786DD4 != 1)
	{
		//Begin addition
		int csvid = get_id(player, card);
		int quagnoth_flag = 0;
		int madness_flag = 0;
		int falkenrath_gorger_flag = check_battlefield_for_id(player, CARD_ID_FALKENRATH_GORGER);
		if( cards_data[get_card_instance(player, card)->internal_card_id].cc[2] == 11 || csvid == CARD_ID_ICHOR_SLICK ||
			(falkenrath_gorger_flag && has_subtype(player, card, SUBTYPE_VAMPIRE)) )
		{
			/* "Madness - If you discard this card, discard it into exile.
			 * When you do, cast it for its madness cost or put it into your graveyard."
			 * We're just faking it, until some cards with "when THIS is put into exile" appears.
			 */
			enum
			{
				MADNESS_MODE_STANDARD = 1,
				MADNESS_MODE_FALKENRATH_GORGER
			};

			int mode = MADNESS_MODE_STANDARD;
			int options[2] ={	call_card_function(player, card, EVENT_CAN_PAY_MADNESS_COST),
								(falkenrath_gorger_flag && has_subtype(player, card, SUBTYPE_VAMPIRE) &&
								has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_original_internal_card_id(player, card)))
							};
			mode = DIALOG(player, card, EVENT_CAST_SPELL, DLG_NO_STORAGE, DLG_NO_CANCEL,
							"Pay normal Madness cost", options[0], 10,
							"Use Falkenrath Goger's Madness", options[1], 10,
							"Decline", 1, 1);

			if( mode == MADNESS_MODE_STANDARD ){
				if( call_card_function(player, card, EVENT_PAY_MADNESS_COST) ){
					madness_flag = 1;
					play_card_in_hand_madness(player, card);
				}
			}
			if( mode == MADNESS_MODE_FALKENRATH_GORGER ){
				if( charge_mana_from_id(player, -1, EVENT_CAST_SPELL, get_id(player, card)) ){
					madness_flag = 1;
					play_card_in_hand_madness(player, card);
				}
			}
		}
		if( player != player_who_controls_effect ){
			switch( csvid ){
					case CARD_ID_DODECAPOD: //Replacement: it goes directly from hand to play
					{
						add_1_1_counters(player, card, 2);
						put_into_play(player, card);
					}
					return;

					case CARD_ID_LOXODON_SMITER://Replacement: it goes directly from hand to play
					case CARD_ID_OBSTINATE_BALOTH://Replacement: it goes directly from hand to play
					case CARD_ID_WILT_LEAF_LIEGE://Replacement: it goes directly from hand to play
					{
						put_into_play(player, card);
						return;
					}

					case CARD_ID_GUERRILLA_TACTICS:
					{
						target_definition_t td;
						default_target_definition(player, card, &td, TYPE_CREATURE);
						td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
						td.allow_cancel = 0;

						card_instance_t *instance = get_card_instance(player, card);

						if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
							damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
						}
						break;
					}

					case CARD_ID_MANGARAS_BLESSING:
					{
						gain_life(player, 2);
						int legacy = create_legacy_effect(player, card, &discard_trigger_delayed_ability);
						card_instance_t *instance = get_card_instance( player, legacy);
						instance->targets[0].player = player;
						instance->targets[0].card = csvid;
						instance->targets[1].player = 1 ; //Return to hand at eot.
					}
					break;

					case CARD_ID_METROGNOME:
					{
						token_generation_t token;
						default_token_definition(player, card, CARD_ID_GNOME, &token);
						token.qty = 4;
						token.pow = token.tou = 1;
						generate_token(&token);
					}
					break;

					case CARD_ID_PSYCHIC_PURGE:
						lose_life(player_who_controls_effect, 5);
						break;

					case CARD_ID_PURE_INTENTIONS:
					{
						int legacy = create_legacy_effect(player, card, &discard_trigger_delayed_ability);
						card_instance_t *instance = get_card_instance( player, legacy);
						instance->targets[0].player = player;
						instance->targets[0].card = csvid;
						instance->targets[1].player = 1 ; //Return to hand at eot.
					}
					break;

					case CARD_ID_SAND_GOLEM:
					{
						int legacy = create_legacy_effect(player, card, &discard_trigger_delayed_ability);
						card_instance_t *instance = get_card_instance( player, legacy);
						instance->targets[0].player = player;
						instance->targets[0].card = csvid;
						instance->targets[1].player = 2 ; //Return to play with a +1/+1 counter at eot.
					}
					break;

					case CARD_ID_QUAGNOTH:
						quagnoth_flag = 1;
						break;

					default:
						break;
			}
		}
		//End addition

		dispatch_event_to_single_card(player, card, EVENT_DISCARD, 1 - player, -1);
		if( ! madness_flag ){ //If Madness was paid, the card is no more in hand, so callig this block will surely screw things up.
			put_card_in_graveyard(player, card);
			get_card_instance(player, card)->internal_card_id = -1;
			call_sub_437E20_unless_ai_is_speculating();
			play_sound_effect(WAV_DISCARD);
			--hand_count[player];
		}

		//Begin addition
		if( quagnoth_flag ){
			seek_grave_for_id_to_reanimate(player, -1, player, csvid, REANIMATE_DEFAULT);
		}
		//End addition
	}
}

void discard_card(int player, int card){
	discard_card_impl(player, card, player, 0);
}

void new_discard_card(int player, int card, int player_who_controls_effect, int flags){
	discard_card_impl(player, card, player_who_controls_effect, flags);
}

void discard(int player, int flags, int player_who_controls_effect)
{
	int random = flags & DISC_RANDOM;
	//  int result; // eax
  card_instance_t *v3; // esi
  card_instance_t *v4; // esi
  const char *v5; // [sp-8h] [bp-4D8h]
  signed int v6; // [sp+4h] [bp-4CCh]
  signed int card; // [sp+8h] [bp-4C8h]
  //  target_t ret_tgt; // [sp+Ch] [bp-4C4h]
  int c; // [sp+4C8h] [bp-8h]
  int v10; // [sp+4CCh] [bp-4h]

  //  result = player;
  if (hand_count[player] > 0)
	{
	  if (player != 1
		  || trace_mode & 2
		  || (dword_60A4B0 + hand_count[1] > 0))
		{
		  if ((player && !(trace_mode & 2)) || ai_is_speculating == 1 || random)
			{
			  v10 = 0;
			  if (trace_mode & 2 && player == 1)
				{
				  TENTATIVE_wait_for_network_result(1, 20);
				  card = dblword_628C10;
				  v10 = dblword_628C10 != -1;
				}
			  else
				{
				  v6 = 0;
				  do
					{
					  card = internal_rand(active_cards_count[player]);
					  v3 = get_card_instance(player, card);
					  if (v3->internal_card_id != -1
						  && !(v3->state & (STATE_INVISIBLE|STATE_IN_PLAY)))
						v10 = 1;
					  if (v10)
						break;
					  ++v6;
					}
				  while (v6 < 999);
				  if (!v10)
					{
					  for (c = 0; active_cards_count[player] > c; ++c)
						{
						  v4 = get_card_instance(player, c);
						  if (v4->internal_card_id != -1
							  && !(v4->state & (STATE_INVISIBLE|STATE_IN_PLAY)))
							{
							  v10 = 1;
							  card = c;
							  break;
							}
						}
					}
				  if (trace_mode & 2 && !player)
					{
					  if (!v10)
						card = -1;
					  dblword_628C10 = card;
					  byte_628C0C = 20;
					  TENTATIVE_send_network_result(0, 20);
					}
				}
			}
		  else
			{
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard.");
				card = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);

				/*

			  load_text(0, "PROMPT_DISCARDACARD");
			  C_real_select_target(
								   player,
								   player,
								   player,
								   TARGET_ZONE_HAND,
								   0,
								   0,
								   0,
								   0,
								   0,
								   0,
								   -1,
								   SUB_invalid,
								   -1,
								   -1,
								   0,
								   0,
								   0,
								   (const char *)text_lines,
								   0,
								   &ret_tgt);
			  card = ret_tgt.card;
			  */
			}
		  if (!(trace_mode & 2) && player == 1 && ai_is_speculating != 1)
			{
			  load_text(0, "PROMPT_DISCARDACARD");
			  if (random)
				v5 = text_lines[1];
			  else
				v5 = text_lines[2];
			  do_dialog(1, 1, card, -1, -1, v5, 0);
			}
		  // result = discard_card(player, card);
		  new_discard_card(player, card, player_who_controls_effect, flags);
		}
	}
  //  return result;
}

void new_multidiscard(int player, int quantity, int flags, int player_who_controls_effect){
	while( quantity ){
			discard(player, flags, player_who_controls_effect);
			quantity--;
	}
}

void multidiscard(int player, int quantity, int random){
	new_multidiscard(player, quantity, random ? DISC_RANDOM : 0, player);
}

void new_discard_all(int player, int player_who_controls_effect){
	int p;
	for (p = 0; p < 2; p++){
		if (p == player || player == 2){
			while (hand_count[p] > 0){
				if (actually_has_a_card_in_hand(p)){
					discard(p, 0, player_who_controls_effect);
				} else {
					if (ai_is_speculating != 1){
						popup("discard_all()", "Couldn't find card in hand for %s player\nhand_count[%d] still %d", p == 0 ? "human" : "ai", p, hand_count[p]);
					}
					hand_count[p] = 0;
				}
			}
		}
	}
}

void discard_all(int player){
	new_discard_all(player, player);
}

// Returns the first activation card currently on the stack controlled by player that was activated for {player,card}, or NULL if not found.
card_instance_t* has_activation_on_stack(int player, int card)
{
  int c;
  card_instance_t* inst;
  for (c = 0; c < active_cards_count[player]; ++c)
	if ((inst = get_card_instance(player, c))
		&& inst->internal_card_id == activation_card
		&& inst->parent_card == card && inst->parent_controller == player)
	  return inst;
  return NULL;
}

// Resets {player,card}'s timestamp to now.
void new_timestamp(int player, int card)
{
  EXE_FN(void, 0x4373b0, int, int)(player, card);	// set_timestamp()
  EXE_FN(void, 0x437400, void)();	// compact_timestamps()
}

// Uses {player, card}->targets[11].player.
int
whenever_a_player_sacrifices_a_permanent(int player, int card, event_t event, int who_sacs, type_t typ, resolve_trigger_t mode)
{
  card_instance_t* inst, *aff;
  if (event == EVENT_GRAVEYARD_FROM_PLAY
	  && (affected_card_controller == who_sacs || who_sacs == ANYBODY)
	  && ((aff = in_play(affected_card_controller, affected_card)))
	  && is_what(affected_card_controller, affected_card, typ)
	  && aff->kill_code == KILL_SACRIFICE && !check_special_flags(affected_card_controller, affected_card, SF_KILL_STATE_BASED_ACTION)
	  && (inst = in_play(player, card)) && !is_humiliated(player, card))
	{
	  if (inst->targets[11].player < 0)
		inst->targets[11].player = 0;
	  ++inst->targets[11].player;
	}

  if (trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && affect_me(player, card) && player == reason_for_trigger_controller
	  && (inst = in_play(player, card)) && inst->targets[11].player > 0)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= mode;
	  else if (event == EVENT_RESOLVE_TRIGGER)
		{
		  if (--inst->targets[11].player > 0)
			inst->state &= ~STATE_PROCESSING;	// trigger again
		  return 1;
		}
	  else if (event == EVENT_END_TRIGGER)
		inst->targets[11].player = 0;
	}

  return 0;
}

static int find_target(card_instance_t* inst, int player, int card)
{
  int i;
  for (i = inst->number_of_targets - 1; i >= 0; --i)
    if (inst->targets[i].card == card && inst->targets[i].player == player)
      break;
  return i;
}
static int fx_permanents_destroyed_this_way_accumulate_condition_lambda(int player, int card)
{
  if (BYTE1(get_card_instance(player, card)->eot_toughness) & 2)	// Already mid-resolution
	return 0;

  if (card == trigger_cause && player == trigger_cause_controller)	// This effect!
	return 0;

  card_instance_t* ins = get_card_instance(player, card);
  if (ins->number_of_targets == 0)	// no targets left in play
	return 1;

  card_instance_t* tgt;
  int i;
  for (i = 0; i < ins->number_of_targets; ++i)
	if ((tgt = in_play(ins->targets[i].player, ins->targets[i].card))	// a target that's still in play
		&& (tgt->token_status & STATUS_DYING)	// and dying
		&& tgt->kill_code == KILL_DESTROY)	// due to destruction
	  return 0;

  return 1;
}
static int fx_permanents_destroyed_this_way_accumulate_triggering(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && affect_me(player, card) && player == reason_for_trigger_controller
	  && fx_permanents_destroyed_this_way_accumulate_condition_lambda(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;
	  if (event == EVENT_RESOLVE_TRIGGER)
		return 1;
	}
  return 0;
}
static int fx_permanents_destroyed_this_way_accumulate(int player, int card, event_t event)
{
  card_instance_t* inst = get_card_instance(player, card);

  if (trigger_condition == TRIGGER_LEAVE_PLAY && affect_me(player, card) && player == reason_for_trigger_controller
      && event == EVENT_TRIGGER
      && !(BYTE1(get_card_instance(player, card)->eot_toughness) & 2))	// Already mid-resolution
    {
      int tgt = find_target(inst, trigger_cause_controller, trigger_cause);
      if (tgt < 0)	// Already recorded this one
		return 0;

      inst->targets[tgt] = inst->targets[inst->number_of_targets - 1];	// struct copy
      --inst->number_of_targets;

      if (get_card_instance(trigger_cause_controller, trigger_cause)->kill_code == KILL_DESTROY)
		{
		  uint16_t count = HIWORD(inst->eot_toughness);
		  ++count;
		  SET_BYTE2(inst->eot_toughness) = BYTE0(count);
		  SET_BYTE3(inst->eot_toughness) = BYTE1(count);

		  if (inst->targets[10].card != 0xdeadcad)
			{
			  inst->targets[10].card = 0xdeadcad;
			  inst->targets[10].player = 0;
			}

		  if (trigger_cause_controller == 0)
			{
			  count = LOWORD(inst->targets[10].player);
			  ++count;
			  SET_BYTE0(inst->targets[10].player) = BYTE0(count);
			  SET_BYTE1(inst->targets[10].player) = BYTE1(count);
			}
		  else if (trigger_cause_controller == 1)	// it had *better* be
			{
			  count = HIWORD(inst->targets[10].player);
			  ++count;
			  SET_BYTE2(inst->targets[10].player) = BYTE0(count);
			  SET_BYTE3(inst->targets[10].player) = BYTE1(count);
			}
		}
    }

  if (fx_permanents_destroyed_this_way_accumulate_triggering(player, card, event)
      || event == EVENT_PHASE_CHANGED || event == EVENT_CLEANUP)
    {
      SET_BYTE1(inst->eot_toughness) |= 2;	// prevent re-entry
      if (BYTE1(inst->eot_toughness) & 1)	// Another effect was created along with this one to hold more targets
		{
		  card_instance_t* other;
		  int c;
		  for (c = 0; c < active_cards_count[player]; ++c)
			if ((other = in_play(player, c))
				&& other->damage_source_card == inst->damage_source_card && other->damage_source_player == inst->damage_source_player
				&& other->internal_card_id == inst->internal_card_id && other->info_slot == inst->info_slot
				&& c != card)
			  {
				SET_BYTE2(other->eot_toughness) += BYTE2(inst->eot_toughness);
				SET_BYTE3(other->eot_toughness) += BYTE3(inst->eot_toughness);
				SET_BYTE2(inst->eot_toughness) = 0;
				SET_BYTE3(inst->eot_toughness) = 0;

				if (inst->targets[10].card == 0xdeadcad)
				  {
					if (other->targets[10].card != 0xdeadcad)
					  {
						other->targets[10].card = 0xdeadcad;
						other->targets[10].player = 0;
					  }
					if (other->number_of_targets >= 11)	// Urk.
					  --other->number_of_targets;

					int count = LOWORD(inst->targets[10].player);
					count += LOWORD(other->targets[10].player);
					SET_BYTE0(other->targets[10].player) = BYTE0(count);
					SET_BYTE1(other->targets[10].player) = BYTE1(count);

					count = HIWORD(inst->targets[10].player);
					count += HIWORD(other->targets[10].player);
					SET_BYTE2(other->targets[10].player) = BYTE0(count);
					SET_BYTE3(other->targets[10].player) = BYTE1(count);

					inst->targets[10].player = 0;
				  }

				kill_card(player, card, KILL_REMOVE);
				return 0;
			  }
		}

      if (inst->targets[10].card != 0xdeadcad)
		{
		  inst->targets[10].card = 0xdeadcad;
		  inst->targets[10].player = 0;
		}

      int csvid = inst->display_pic_csv_id;
	  int iid = get_internal_card_id_from_csv_id(csvid);
      call_card_fn((void*)cards_data[iid].code_pointer, inst, player, card, EVENT_RESOLVE_GENERAL_EFFECT);
      kill_card(player, card, KILL_REMOVE);
    }

  return 0;
}
/* Call once for each target.  previous_effect should be NULL on the first call, else the return value of the previous call to this function.  When the last
 * target is destroyed (not counting ones that stop being destroyed for whatever reason), then the original card's function will be sent
 * EVENT_RESOLVE_GENERAL_EFFECT, with player/card set to one of the effect cards created.  The number of targets that were destroyed will be in
 * HIWORD(effect->eot_toughness).  The number of those that were controlled by player 0 will be in LOWORD(effect->targets[10].player), and the number that were
 * controlled by player 1 will be in HIWORD(effect->targets[10].player). */
card_instance_t* legacy_permanents_destroyed_this_way_accumulate(int src_player, int src_card, card_instance_t* previous_effect, int tgt_player, int tgt_card)
{
  // It is obscene that not even effect cards can use their full targets array.

  if (previous_effect == NULL)
	previous_effect = get_card_instance(src_player, create_legacy_effect(src_player, src_card, fx_permanents_destroyed_this_way_accumulate));
  else if (previous_effect->number_of_targets == 18)
    {
      SET_BYTE1(previous_effect->eot_toughness) |= 1;
      previous_effect = get_card_instance(src_player, create_legacy_effect(src_player, src_card, fx_permanents_destroyed_this_way_accumulate));
      SET_BYTE1(previous_effect->eot_toughness) |= 1;
    }

  if (tgt_player >= 0)
	{
	  previous_effect->targets[previous_effect->number_of_targets].player = tgt_player;
	  previous_effect->targets[previous_effect->number_of_targets++].card = tgt_card;
	}
  return previous_effect;
}

int get_card_name(int player, int card){
	int csvid = player == -1 ? cards_data[card].id : get_id(player, card);
	if( player != -1 ){
		// Special cases for cards in play
		if( cards_data[get_card_instance(player, card)->original_internal_card_id].id == CARD_ID_SAKASHIMA_THE_IMPOSTOR ){
			return CARD_ID_SAKASHIMA_THE_IMPOSTOR;
		}
		if( cards_data[get_card_instance(player, card)->original_internal_card_id].id == CARD_ID_LAZAV_DIMIR_MASTERMIND ){
			return CARD_ID_LAZAV_DIMIR_MASTERMIND;
		}
		if( get_id(player, card) == CARD_ID_DARKSTEEL_MUTATION_INSECT ){
			return cards_data[get_card_instance(player, card)->targets[1].card].id;
		}
	}
	if( csvid == CARD_ID_FACE_DOWN_CREATURE){ //Face-down cards have no name
		return -1;
	}
	return csvid;
}

int has_same_name(int player, int card, int player2, int card2){
	int id1 = get_card_name(player, card);
	int id2 = get_card_name(player2, card2);
	if( id1 != -1 && id2 != -1 && id1 == id2 ){
		return 1;
	}
	return 0;
}

void damage_creature_and_all_the_others_with_the_same_name(int player, int card, int t_player, int t_card, int dmg){
	int id = get_card_name(t_player, t_card);
	if( id != -1 ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.id = id;
		APNAP(p, {new_damage_all(player, card, p, dmg, 0, &this_test);};);
	}
	else{//The creature has no name, probably a face-down card
		damage_creature(t_player, t_card, dmg, player, card);
	}
}

int tutor_card_with_the_same_name(int player, int card, int who_tutors, int t_player, int tutor_from, int tutor_to, int must_tutor){
	int id = get_card_name(player, card);
	//Special hack, see: http://www.slightlymagic.net/forum/viewtopic.php?f=86&t=17522#p195229
	if( id == CARD_ID_SPLINTER_TOKEN ){
		id = CARD_ID_SPLINTER;
	}
	char msg[100];
	if( id == -1 ){ // A card with no name, probably a face-down card.
		scnprintf(msg, 100, "You won't be able to tutor any card as you're searching for a card with no name.");
	}
	else{
		scnprintf(msg, 100, "Select a card named %s", cards_ptr[ id ]->name);
	}
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, msg);
	this_test.id = id;
	int result = new_global_tutor(who_tutors, t_player, tutor_from, tutor_to, must_tutor, AI_FIRST_FOUND, &this_test);
	return result;
}

int exile_top_x_card_you_may_play_them(int player, int card, int howmany, test_definition_t *test, int t_player, mpcfe_mode_t flags){
	int amount = MIN( howmany, count_deck(t_player) );
	int result = 0;
	if( amount ){
		char buffer[100];
		int pos = scnprintf(buffer, 100, "Cards exiled");
		if( card != -1 ){
			scnprintf(buffer + pos, 100-pos, " by %s", cards_ptr[get_id(player, card)]->name);
		}
		int *deck = deck_ptr[t_player];
		show_deck( player, deck, amount, buffer, 0, 0x7375B0 );
		int count = 0;
		while( deck[0] != -1 && count < amount ){
				int iid = deck_ptr[player][0];
				if (iid != -1){
					rfg_top_card_of_deck(player);
					if( new_make_test(t_player, iid, -1, test) ){
						create_may_play_card_from_exile_effect(player, card, t_player, cards_data[iid].id, flags);
						result++;
					}
				}
				count++;
		}
	}
	return result;
}

void life_sucking(int player, int card, int amount){
	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	int t_player = 1-player;

	if( ! would_validate_arbitrary_target(&td, t_player, -1) ){
		t_player = player;
		if( ! would_validate_arbitrary_target(&td, t_player, -1) ){
			return;
		}
	}

	lose_life(t_player, amount);
	gain_life(player, amount);
}

