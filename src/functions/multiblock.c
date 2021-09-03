// -*- c-basic-offset:2 -*-

#include "manalink.h"

#define card_multiblocker	0x401010

static int is_multiblocker_card(int player, int card)
{
  return cards_at_7c7000[get_card_instance(player, card)->internal_card_id]->code_pointer == card_multiblocker;
}

static int is_multiblocker_card_copying(int player, int card, int orig_player, int orig_card)
{
  card_instance_t* instance = get_card_instance(player, card);
  return (cards_at_7c7000[instance->internal_card_id]->code_pointer == card_multiblocker
		  && instance->damage_source_player == orig_player && instance->damage_source_card == orig_card);
}

static int create_multiblocker(int player, int card, int num_blockable)
{
  card_instance_t* instance = get_card_instance(player, card);
  int iid;

  int already_a_multiblocker = is_multiblocker_card(player, card);

  if (already_a_multiblocker)	// reuse the type already created
	iid = instance->internal_card_id;
  else
	{	// based card blocking - create a new type
	  iid = create_a_card_type(instance->internal_card_id);

	  if (iid > 0)
		{
		  card_data_t* cd = cards_at_7c7000[iid];
		  cd->code_pointer = card_multiblocker;
		  cd->extra_ability = 0;
		  cd->id = 907;
		  cd->reserved2 = 0;
		}
	  else
		return 0;
	}

  int card_added = add_card_to_hand(player, iid);
  if (card_added != -1)
	{
	  card_instance_t* blocker_inst = get_card_instance(player, card_added);
	  blocker_inst->state = instance->state & ~STATE_BLOCKING;
	  --hand_count[player];
	  blocker_inst->regen_status = instance->regen_status;
	  blocker_inst->token_status = STATUS_SPECIAL_BLOCKER | STATUS_OBLITERATED;
	  blocker_inst->info_slot = -1;
	  blocker_inst->eot_toughness = num_blockable - 1;
	  if (already_a_multiblocker)
		{
		  blocker_inst->damage_source_player = instance->damage_source_player;
		  blocker_inst->damage_source_card = instance->damage_source_card;
		  blocker_inst->display_pic_csv_id = instance->display_pic_csv_id;
		  blocker_inst->display_pic_num = instance->display_pic_num;
		}
	  else
		{
		  blocker_inst->damage_source_player = player;
		  blocker_inst->damage_source_card = card;
		  blocker_inst->display_pic_csv_id = cards_at_7c7000[instance->internal_card_id]->id;
		  blocker_inst->display_pic_num = get_card_image_number(cards_at_7c7000[instance->internal_card_id]->id, player, card);
		}

	  return 1;
	}

  return 0;
}

void arbitrary_can_block_additional(event_t event, int num_additional)
{
  if (num_additional < 1)
	return;

  if (num_additional > 254)
	num_additional = 254;

  int player = affected_card_controller;
  int card = affected_card;

  if (is_multiblocker_card(player, card))
	return;

#if 0
  if (event == EVENT_CAN_MULTIBLOCK)
	event_result = 1;
#endif

  if (trigger_condition == TRIGGER_BLOCKER_CHOSEN
	  && reason_for_trigger_controller == player
	  && current_turn != player
	  && affect_me(trigger_cause_controller, trigger_cause))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		create_multiblocker(player, card, num_additional + 1);
	}
}

void creature_can_block_additional(int player, int card, event_t event, int num_additional)
{
  if (affect_me(player, card))
	arbitrary_can_block_additional(event, num_additional);
}

void attached_creature_can_block_additional(int player, int card, event_t event, int num_additional)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->damage_target_player >= 0 && instance->damage_target_card >= 0
	  && affect_me(instance->damage_target_player, instance->damage_target_card))
	arbitrary_can_block_additional(event, num_additional);
}

#if 0
static int has_attached_blaze_of_glory_legacy(int player, int card)
{
  int c;
  for (c = 0; c < active_cards_count[player]; ++c)
	{
	  card_instance_t* instance = get_card_instance(player, c);
	  if (instance->internal_card_id == EXE_DWORD(0x728368)	// Blaze of Glory legacy
		  && instance->damage_target_player == player
		  && instance->damage_target_card == card)
		return 1;
	}

  return 0;
}
#endif

void process_multiblock(int player, int card, uint16_t pow)
{
  // 0x4B3850
  card_instance_t* instance = get_card_instance(player, card);
  int c;

  if (is_multiblocker_card(player, card))
	{
	  int dsp = instance->damage_source_player;
	  int dsc = instance->damage_source_card;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (c != card && is_multiblocker_card_copying(player, c, dsp, dsc))
		  {
			card_instance_t* inst = get_card_instance(player, c);
			inst->power = pow;
			--inst->info_slot;
		  }
	  get_card_instance(dsp, dsc)->power = pow;
	}
  else
#if 0	// This doesn't actually seem to be necessary, since STATUS_SPECIAL_BLOCKER is also checked (it's set by sub_458430)
	if (dispatch_event(player, card, EVENT_CAN_MULTIBLOCK)
		|| has_attached_blaze_of_glory_legacy(player, card))
#endif
	{
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (is_multiblocker_card_copying(player, c, player, card))
		  {
			card_instance_t* inst = get_card_instance(player, c);
			inst->power = pow;
			--inst->info_slot;
		  }
	  instance->power = pow;
	}
}

static void count_and_reap_multiblockers(int player, int card, int force_reap_me)
{
  card_instance_t* instance = get_card_instance(player, card);
  int dsp = instance->damage_source_player;
  int dsc = instance->damage_source_card;
  if (instance->blocking == 255
	  || force_reap_me
	  || !in_play(1-player, instance->blocking))
	kill_card(player, card, KILL_BURY);

  int c, count = 0;
  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_play(player, c) && is_multiblocker_card_copying(player, c, dsp, dsc))	// a multiblocker copy of the same card as this
	  {
		card_instance_t* inst = get_card_instance(player, c);
		if (inst->blocking == 255
			|| !in_play(1-player, inst->blocking))
		  kill_card(player, c, KILL_BURY);
		else
		  ++count;
	  }

  ++count;	// for the original

  if (count == 1)	// Only the original blocking (and possibly not him)
	get_card_instance(dsp, dsc)->token_status &= ~STATUS_SPECIAL_BLOCKER;	// Don't prompt for amount of damage to deal to a single blocked creature, nor allow any to be left undealt
  else
	for (c = 0; c < active_cards_count[player]; ++c)
	  if (in_play(player, c) && is_multiblocker_card_copying(player, c, dsp, dsc))
		get_card_instance(player, c)->info_slot = count;
}

int card_multiblocker_hook(int player, int card, event_t event)
{
  /* Some of this was handled by card_palace_guard()/card_two_headed_giant_of_foriys() (such as destroying multiblockers if the original card is destroyed);
   * some of it's to better handle destroying non-blocking copies. */

  if (current_phase >= PHASE_MAIN2 && current_phase != PHASE_DAMAGE_PREVENTION)
	{
	  kill_card(player, card, KILL_BURY);
	  return 0;
	}

  card_instance_t* instance = get_card_instance(player, card);

  if (trigger_condition == TRIGGER_BLOCKER_CHOSEN
	  && affect_me(player, card)
	  && reason_for_trigger_controller == player
	  && current_turn != player
	  && affect_me(trigger_cause_controller, trigger_cause)
	  && instance->eot_toughness > 1)	// can make more copies
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		create_multiblocker(player, card, instance->eot_toughness);
	}

  if (event == EVENT_BLOCK_LEGALITY && affect_me(player, card))
	{
	  card_instance_t* aff;
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (c != card
			&& (aff = in_play(player, c))
			&& ((c == instance->damage_source_card && player == instance->damage_source_player)
				|| is_multiblocker_card_copying(player, c, instance->damage_source_player, instance->damage_source_card))
			&& (aff->blocking == attacking_card
				|| (aff->blocking != 255
					&& aff->blocking == get_card_instance(attacking_card_controller, attacking_card)->blocking)))
		  event_result = 1;

	  return 0;	// Don't fall through to the exe version, which just checks damage_source_player/card, not other multiblocker shadows
	}


  if (trigger_condition == TRIGGER_LEAVE_PLAY && event == EVENT_TRIGGER			// Creature this copy is blocking leaves play
	  && trigger_cause_controller == 1-player && trigger_cause == instance->blocking
	  && instance->info_slot != -1)	// Carefully avoid reaping before the "real" first count in EVENT_DECLARE_BLOCKERS
	count_and_reap_multiblockers(player, card, 1);
  else if (trigger_condition == TRIGGER_LEAVE_PLAY && event == EVENT_TRIGGER	// Creature this is a copy of leaves play
		   && trigger_cause_controller == instance->damage_source_player
		   && trigger_cause == instance->damage_source_card)
	count_and_reap_multiblockers(player, card, 1);	// OK to reap before the first count in EVENT_DECLARE_BLOCKERS, since all the other copies will destroy themselves too
  else if (event == EVENT_DECLARE_BLOCKERS)
	count_and_reap_multiblockers(player, card, 0);
  else if (event == EVENT_TRIGGER)	// The triggers this responds to aren't real, in-game triggers, so don't prompt like they were
	event = EVENT_RESOLVE_TRIGGER;

  return EXE_FN(int, 0x458430, int, int, event_t)(player, card, event);
}

// Returns 1 if {player,card} is attacking and unblocked.
int is_unblocked(int player, int card){
	card_instance_t* instance = get_card_instance(player, card);
	if (!(instance->state & STATE_ATTACKING)){
		return 0;
	}
	int count, band = instance->blocking;
	for (count = 0; count < active_cards_count[1-player]; ++count){
		if (in_play(1-player, count)){
			instance = get_card_instance(1-player, count);
			if (instance->blocking == card
				|| (band != 255 && instance->blocking == band)){
				return 0;
			}
		}
	}
	return 1;
}

// Returns the number of creatures player controls that are blocking anything.
int count_blockers(int player, int event){
  int result = 0;
  int count = 0;

  while( count < active_cards_count[player] ){
		if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) && blocking(player, count, event) ){
			result++;
		}
		count++;
  }
  return result;
}

// Returns nonzero if {player,card} is blocking anything.
int blocking(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS && current_turn != player && get_card_instance(player, card)->blocking < 255){
		return 1;
	}
	return 0;
}

// Returns count of creatures being blocked by (player, card), dealing properly with both banding and multiblocker creatures.
int count_creatures_this_is_blocking(int player, int card)
{
  int c, d, count = 0;
  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_play(player, c)
		&& (c == card
			|| is_multiblocker_card_copying(player, c, player, card)))
	  {
		card_instance_t* instance = get_card_instance(player, c);
		int directly_blocked = instance->blocking;
		if (directly_blocked == 255)
		  continue;

		card_instance_t* instance_directly_blocked = get_card_instance(1-player, directly_blocked);
		int band = instance_directly_blocked->blocking;
		if (band == 255)
		  count += in_play(1-player, directly_blocked) ? 1 : 0;
		else
		  {
			for (d = 0; d < active_cards_count[1-player]; ++d)
			  if (in_play(1-player, d)
				  && get_card_instance(1-player, d)->blocking == band)
				++count;
		  }
	  }

  return count;
}

// Returns number of creatures {player,card} is blocked by, dealing properly with banding.
int count_my_blockers(int player, int card)
{
  int band = get_card_instance(player, card)->blocking;
  if (band == 255)
	band = card;

  card_instance_t* inst;
  int c, result = 0;
  for (c = 0; c < active_cards_count[1-player]; ++c)
	if ((inst = in_play(1-player, c)) && is_what(1-player, c, TYPE_CREATURE) && inst->blocking == band)
	  ++result;

  return result;
}

// Preconditions: attack_card is controlled by current_turn; blocking_card is controlled by 1-current_turn; in_play(current_turn, attacking_card)
int is_blocking(int blocking_card, int attack_card)
{
  int c, attacker_band = get_card_instance(current_turn, attack_card)->blocking;
  card_instance_t* instance;

  for (c = 0; c < active_cards_count[1-current_turn]; ++c)
	if ((instance = in_play(1-current_turn, c))
		&& (c == blocking_card
			|| is_multiblocker_card_copying(1-current_turn, c, 1-current_turn, blocking_card)))
	  {
		int directly_blocked = instance->blocking;
		if (directly_blocked == 255)
		  continue;

		if (directly_blocked == attack_card)
		  return 1;

		if (attacker_band == 255)
		  continue;

		card_instance_t* instance_directly_blocked = get_card_instance(current_turn, directly_blocked);
		int blocked_band = instance_directly_blocked->blocking;
		if (blocked_band == attacker_band)
		  return 1;
	  }

  return 0;
}

// player1/card1 and player2/card2 are interchangeable.
int is_blocking_or_blocked_by(int player1, int card1, int player2, int card2)
{
  return (player1 != player2
		  && (current_turn == player1
			  ? in_play(player1, card1) && is_blocking(card2, card1)
			  : in_play(player2, card2) && is_blocking(card1, card2)));
}

/* Preconditions: (player,card) is blocking; fn is a function with signature void fn(int arg1, int arg2, int blocked_controller, int blocked_card)
 * fn() will be called with arguments arg1, arg2, blocked_controller, blocked_card for each (blocked_controller,blocked_card) blocked by (player,card). */
void for_each_creature_blocked_by_me(int player, int card, void (*fn)(int, int, int, int), int arg1, int arg2)
{
  int c, d;
  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_play(player, c)
		&& (c == card
			|| is_multiblocker_card_copying(player, c, player, card)))
	  {
		card_instance_t* instance = get_card_instance(player, c);
		int directly_blocked = instance->blocking;
		if (directly_blocked == 255)
		  continue;

		card_instance_t* instance_directly_blocked = get_card_instance(1-player, directly_blocked);
		int band = instance_directly_blocked->blocking;
		if (band == 255)
		  {
			if (in_play(1-player, directly_blocked))
			  fn(arg1, arg2, 1-player, directly_blocked);
		  }
		else
		  {
			for (d = 0; d < active_cards_count[1-player]; ++d)
			  if (in_play(1-player, d)
				  && get_card_instance(1-player, d)->blocking == band)
				fn(arg1, arg2, 1-player, d);
		  }
	  }
}

/* A simple interface to for_each_creature_blocked_by_me() handling a common case.  marked should be a char array[2][151] initialized to 0; each element of
 * marked[blocked_controller][blocked_card] will be set to 1. */
void mark_each_creature_blocked_by_me(int player, int card, char (*marked)[151])
{
  int c, d;
  for (c = 0; c < active_cards_count[player]; ++c)
	if (in_play(player, c)
		&& (c == card
			|| is_multiblocker_card_copying(player, c, player, card)))
	  {
		card_instance_t* instance = get_card_instance(player, c);
		int directly_blocked = instance->blocking;
		if (directly_blocked == 255)
		  continue;

		card_instance_t* instance_directly_blocked = get_card_instance(1-player, directly_blocked);
		int band = instance_directly_blocked->blocking;
		if (band == 255)
		  {
			if (in_play(1-player, directly_blocked))
			  marked[1-player][directly_blocked] = 1;
		  }
		else
		  {
			for (d = 0; d < active_cards_count[1-player]; ++d)
			  if (in_play(1-player, d)
				  && get_card_instance(1-player, d)->blocking == band)
				marked[1-player][d] = 1;
		  }
	  }
}

/* Preconditions: (player,card) is attacking; fn is a function with signature void fn(int arg1, int arg2, int blocking_controller, int blocking_card)
 * fn() will be called with arguments arg1, arg2, blocking_controller, blocking_card for each (blocking_controller,blocking_card) blocking (player,card). */
void for_each_creature_blocking_me(int player, int card, void (*fn)(int, int, int, int), int arg1, int arg2)
{
  int c, band = get_card_instance(player, card)->blocking;
  if (band == 255)
	band = card;

  for (c = 0; c < active_cards_count[1-player]; ++c)
	{
	  card_instance_t* blocking_inst = get_card_instance(1-player, c);
	  if (blocking_inst->blocking == band && in_play(1-player, c))
		{
		  if (is_multiblocker_card(1-player, c))
			{
			  int dsc = blocking_inst->damage_source_card;
			  if (in_play(1-player, dsc))
				fn(arg1, arg2, 1-player, dsc);
			}
		  else
			fn(arg1, arg2, 1-player, c);
		}
	}
}

/* A simple interface to for_each_creature_blocking_me() handling a common case.  marked should be a char array[2][151] initialized to 0; each element of
 * marked[blocking_controller][blocking_card] will be set to 1. */
void mark_each_creature_blocking_me(int player, int card, char (*marked)[151])
{
  int c, band = get_card_instance(player, card)->blocking;
  if (band == 255)
	band = card;

  for (c = 0; c < active_cards_count[1-player]; ++c)
	{
	  card_instance_t* blocking_inst = get_card_instance(1-player, c);
	  if (blocking_inst->blocking == band && in_play(1-player, c))
		{
		  if (is_multiblocker_card(1-player, c))
			{
			  int dsc = blocking_inst->damage_source_card;
			  if (in_play(1-player, dsc))
				marked[1-player][dsc] = 1;
			}
		  else
			marked[1-player][c] = 1;
		}
	}
}
