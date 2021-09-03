// -*- c-basic-offset:2 -*-
// Low-level event, trigger, and card function handling.

#include "manalink.h"

// These are fairly common here, but shouldn't be used elsewhere
#define push_affected_card_stack	EXE_FN(void, 0x435C80, void)
#define pop_affected_card_stack		EXE_FN(void, 0x435CD0, void)

// A frontend for call_card_fn() where both address and instance should be computed from {player,card}.
int call_card_function(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  typedef int (*CardFunction)(int, int, event_t);
  CardFunction fn = (CardFunction)(cards_data[instance->internal_card_id].code_pointer);
  return call_card_fn(fn, get_card_instance(player, card), player, card, event);
}

// A frontend for call_card_fn() where address should be computed from instance.
int call_card_function_i(card_instance_t* instance, int player, int card, event_t event)
{
  typedef int (*CardFunction)(int, int, event_t);
  CardFunction fn = (CardFunction)(cards_data[instance->internal_card_id].code_pointer);
  return call_card_fn(fn, instance, player, card, event);
}

int call_card_fn_impl(void* address, card_instance_t* instance, int player, int card, event_t event);	// never call directly
// Puts instance into esi, then calls address(player, card, event).
int call_card_fn(void* address, card_instance_t* instance, int player, int card, event_t event)
{
  int old_trigger_condition = trigger_condition;
  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER && event != EVENT_END_TRIGGER)
	trigger_condition = -1;

  int rval = call_card_fn_impl(address, instance, player, card, event);

  trigger_condition = old_trigger_condition;

  return rval;
}

extern int timestamp_card[500];
extern int timestamp_player[500];
void dispatch_event_raw(event_t event)
{
  // 0x435a40

  minimize_nondraining_mana();	// Needs to happen quite frequently.  This'll do it.

  // Begin additions
  // These two are because something calls this without stashing them when it really should; http://www.slightlymagic.net/forum/viewtopic.php?t=13665 is a symptom.
  int old_affected_card_controller = affected_card_controller;
  int old_affected_card = affected_card;
  /* And this is an optimization; compare e.g. upkeep_trigger_ability_mode(), which just checks trigger_condition before calling expensive things like
   * count_upkeeps(), even though it'll only respond to EVENT_TRIGGER and EVENT_RESOLVE_TRIGGER.  When called from dispatch_event() or similar, it's already set
   * to -1, but plenty of things call dispatch_event_raw instead.  Shame on them. */
  int old_trigger_condition = trigger_condition;
  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER && event != EVENT_END_TRIGGER)
	trigger_condition = -1;
  // End additions

  //if (ai_is_speculating!=1)logg("dispatch_event_raw", "%s%s%s for (%d,%d)=%s%s", trigger_condition <= 0 ? "" : trigger_t_str(trigger_condition), trigger_condition <= 0 ? "" : "/", event_t_str(event), affected_card_controller, affected_card, get_name(affected_card_controller, affected_card));

  // Begin additions
  // In the exe version, cards attacking cards were tapped in the same loop as the event was dispatched.  Instead, do it before dispatching.
  // If this seems out-of-place here, well, it did to me too; but I'm not going to try to shoehorn it into this function's callers.
  unsigned int ts;
  unsigned char attackers[151];
  if (event == EVENT_DECLARE_ATTACKERS)
	{
	  memset(attackers, 0xff, sizeof(attackers));
	  number_of_attackers_declared = 0;
	  for (ts = 0; ts < 500 && timestamp_player[ts] != -1; ++ts)
		{
		  int player = timestamp_player[ts];
		  int card = timestamp_card[ts];

		  card_instance_t* instance;
		  if (player == current_turn && (instance = in_play(player, card)) && instance->timestamp == ts
			  && (instance->state & STATE_ATTACKING))
			{
			  attackers[number_of_attackers_declared++] = card;
			  if (!(instance->state & STATE_TAPPED)
				  && !(get_special_abilities_by_instance(instance) & SP_KEYWORD_VIGILANCE))
				tap_card(player, card);
			}
		}
	}

  if (event == EVENT_CAN_SKIP_TURN)	// First event dispatched every turn, used only by skip-turn effects and housekeeping.
	enable_xtrigger_flags = 0;
  // End additions

  for (ts = 0; ts < 500 && timestamp_player[ts] != -1; ++ts)
	{
	  int player = timestamp_player[ts];
	  int card = timestamp_card[ts];

	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->timestamp == ts
		  && instance->internal_card_id != -1
		  && !(instance->state & STATE_OUBLIETTED)
		  && (instance->state & (STATE_INVISIBLE | STATE_IN_PLAY)))
		{
		  typedef int (*CardFunction)(int, int, event_t);
		  CardFunction fn = (CardFunction)(cards_data[instance->internal_card_id].code_pointer);
		  call_card_fn(fn, instance, player, card, event);
		  // Begin removals
		  // if event == EVENT_DECLARE_ATTACKERS, then tap the card if it's attacking, untapped, and doesn't have vigilance; then resolve damage cards and run regeneration/graveyard triggers
		  // End removals
		}
	}

  if (event == EVENT_DECLARE_ATTACKERS && number_of_attackers_declared > 0)
	{
	  // Begin additions
	  dispatch_xtrigger2(current_turn, XTRIGGER_ATTACKING, "attacking", 0, 0, 0);

	  /* These were run after each card tapped.  That was both too many times *and* not enough, since they didn't run for non-attacking cards handling
	   * EVENT_DECLARE_ATTACKERS. */
	  EXE_FN(void, 0x477070, void)();	// resolve_damage_cards_and_prevent_damage();
	  EXE_FN(void, 0x477a90, void)();	// regenerate_or_graveyard_triggers();
	  // End additions

	  recalculate_all_cards_in_play();
	}

  // Begin additions
  affected_card_controller = old_affected_card_controller;
  affected_card = old_affected_card;
  trigger_condition = old_trigger_condition;

  if (event == EVENT_TAPPED_TO_PLAY_ABILITY || event == EVENT_PLAY_ABILITY
	  || (event == EVENT_CAST_SPELL && !check_state(affected_card_controller, affected_card, STATE_IN_PLAY)))	// actually cast, not put_into_play
	recalculate_all_cards_in_play();
  // End additions
}

// dispatch_event() forces the initial value of event_result to 0; this lets you set it arbitrarily.
int dispatch_event_with_initial_event_result(int new_affected_card_controller, int new_affected_card, event_t event, int initial_event_result)
{
  // When the exe needs to do this, it inlines everything here.  Fugly.

  push_affected_card_stack();

  event_result = initial_event_result;
  affected_card_controller = new_affected_card_controller;
  affected_card = new_affected_card;
  attacking_card_controller = 1 - new_affected_card_controller;
  attacking_card = -1;

  int old_trigger_condition = trigger_condition;
  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER && event != EVENT_END_TRIGGER)
	trigger_condition = -1;

  dispatch_event_raw(event);

  int new_event_result = event_result;

  trigger_condition = old_trigger_condition;
  pop_affected_card_stack();

  return new_event_result;
}

/* dispatch_event() forces attacking_card_controller to 1-player and attacking_card to -1; this lets you set them explicitly.  Returns the value of event_result
 * as set by the cards' functions (before restoring event_result's previous value). */
int dispatch_event_with_attacker(int new_affected_card_controller, int new_affected_card, event_t event, int new_attacking_card_controller, int new_attacking_card)
{
  // When the exe needs to do this, it inlines everything here, too.  Still fugly.

  push_affected_card_stack();

  event_result = 0;
  affected_card_controller = new_affected_card_controller;
  affected_card = new_affected_card;
  attacking_card_controller = new_attacking_card_controller;
  attacking_card = new_attacking_card;

  int old_trigger_condition = trigger_condition;
  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER && event != EVENT_END_TRIGGER)
	trigger_condition = -1;

  dispatch_event_raw(event);

  int new_event_result = event_result;

  trigger_condition = old_trigger_condition;
  pop_affected_card_stack();

  return new_event_result;
}

/* Identical to dispatch_event_with_attacker(), but only sends the event to (new_affeced_card_controller,new_affected_card).  Differs from a bare
 * call_card_function() in that it preserves affected_card_controller, affected_card, trigger_condition, event_result, etc.  Returns the value of event_result
 * as set by the card function (before restoring event_result's previous value). */
int dispatch_event_with_attacker_to_one_card(int new_affected_card_controller, int new_affected_card, event_t event, int new_attacking_card_controller, int new_attacking_card)
{
  /* 0x435B50 is almost equivalent, but it returns what the card's function does instead of event_result; plus, has a lot of special-casing for specific events
   * that we don't need. */

  return dispatch_event_arbitrary_to_one_card(new_affected_card_controller, new_affected_card,
											  event,
											  new_affected_card_controller, new_affected_card,
											  new_attacking_card_controller, new_attacking_card);
}

int event_rval;
/* Identical to dispatch_event_with_attacker_to_one_card(), but sends the event to an arbitrary (player, card) pair which can differ from
 * (affected_card_controller, affected_card). */
int dispatch_event_arbitrary_to_one_card(int player, int card, event_t event, int new_affected_card_controller, int new_affected_card, int new_attacking_card_controller, int new_attacking_card)
{
  push_affected_card_stack();

  event_result = 0;
  affected_card_controller = new_affected_card_controller;
  affected_card = new_affected_card;
  attacking_card_controller = new_attacking_card_controller;
  attacking_card = new_attacking_card;

  int old_trigger_condition = trigger_condition;
  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER && event != EVENT_END_TRIGGER)
	trigger_condition = -1;

  event_rval = call_card_function(player, card, event);

  int new_event_result = event_result;

  trigger_condition = old_trigger_condition;
  pop_affected_card_stack();

  return new_event_result;
}

// Properly stashes globals then calls an arbitrary iid's card function for player/card (instead of that card's own one).  Returns what the function does.
int dispatch_event_to_single_card_overriding_function(int player, int card, event_t event, int iid)
{
  push_affected_card_stack();

  event_result = 0;
  affected_card_controller = player;
  affected_card = card;
  attacking_card_controller = 1-player;
  attacking_card = -1;

  int old_trigger_condition = trigger_condition;
  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER && event != EVENT_END_TRIGGER)
	trigger_condition = -1;

  typedef int (*CardFunction)(int, int, event_t);
  CardFunction fn = (CardFunction)(cards_data[iid].code_pointer);
  int rval = (*fn)(player, card, event);

  trigger_condition = old_trigger_condition;
  pop_affected_card_stack();

  return rval;
}


xtrigger_t xtrigger_impl_value_dont_use_directly = 0;
enable_xtrigger_flags_t enable_xtrigger_flags = 0;

static void dispatch_trigger_impl(int player, trigger_t trig, xtrigger_t xtrig, const char *prompt, int TENTATIVE_allow_response)
{
  // Original at 0x4371E0
#define TRIGGER_DEPTH	EXE_DWORD(0x785E60)

  ++TRIGGER_DEPTH;
  int orig_tcc = trigger_cause_controller;
  int orig_tc = trigger_cause;
  int old_life_gained = life_gained;
  int old_62C180 = EXE_DWORD(0x62C180);
  int old_reason_for_trigger_controller = reason_for_trigger_controller;
  int old_62BCE8 = EXE_DWORD(0x62BCE8);
  EXE_DWORD(0x62BCE8) = trig;
  reason_for_trigger_controller = player;
  int p, c;

  // Begin additions
  char processing[2][150];
  int processing_set = 0;
  if (TRIGGER_DEPTH != 1)
	{
	  processing_set = 1;
	  for (p = 0; p < 2; ++p)
		{
		  ASSERT(active_cards_count[p] <= 150);
		  for (c = 0; c < active_cards_count[p]; ++c)
			{
			  card_instance_t* instance = get_card_instance(p, c);
			  if (instance->internal_card_id == -1)
				processing[p][c] = 2;
			  else if (instance->state & STATE_PROCESSING)
				{
				  processing[p][c] = 1;
				  instance->state &= ~STATE_PROCESSING;
				}
			  else
				processing[p][c] = 0;

			  if (instance->state & STATE_IS_TRIGGERING)
				{
				  processing[p][c] |= 4;
				  instance->state &= ~STATE_IS_TRIGGERING;
				}
			}

		  for (c = active_cards_count[p]; c < 150; ++c)
			processing[p][c] = 2;
		}
	}
  // End additions

  unsigned int v8;
  do
	{
	  int old_620860 = EXE_DWORD(0x620860);
	  if (trace_mode & 2)
		EXE_DWORD(0x620860) = 1;
	  else if (player)
		EXE_DWORD(0x620860) = 2;
	  else
		EXE_DWORD(0x620860) = 1;

	  trigger_condition = trig;
	  xtrigger_impl_value_dont_use_directly = xtrig;
	  int old_60A554 = EXE_DWORD(0x60A554);
	  if (TENTATIVE_allow_response)
		EXE_DWORD(0x60A554) = TYPE_INSTANT | TYPE_INTERRUPT;
	  else
		EXE_DWORD(0x60A554) = 0;
	  cancel = 0;
	  EXE_DWORD(0x62852C) = 0;
	  EXE_DWORD(0x62C180) = 0;
	  v8 = EXE_FN(int, 0x475A30, int, const char*)(player, prompt);
	  EXE_DWORD(0x60A554) = old_60A554;
	  EXE_DWORD(0x60A53C) = old_60A554 & (TYPE_INSTANT | TYPE_INTERRUPT);
	  EXE_DWORD(0x620860) = old_620860;
	}
  while (((v8 < 1 ? 4 : 6) & EXE_DWORD(0x62C180))
		 || (TENTATIVE_allow_response && v8));

  // Begin additions
  // Not just dispatch_event(... EVENT_END_TRIGGER), since that'll clear trigger_condition
  // Set again, just in case something changed them and didn't restore
  trigger_condition = trig;
  xtrigger_impl_value_dont_use_directly = xtrig;
  reason_for_trigger_controller = player;
  trigger_cause = orig_tc;
  trigger_cause_controller = orig_tcc;
  int old_aff_cc = affected_card_controller, old_aff_c = affected_card, old_ev = event_result;
  card_instance_t* inst;
  for (p = 0; p < 2; ++p)
	for (c = 0; c < active_cards_count[p]; ++c)
	  if ((inst = in_play(p, c)))
		{
		  affected_card_controller = p;
		  affected_card = c;
		  call_card_function_i(inst, p, c, EVENT_END_TRIGGER);
		}
  affected_card_controller = old_aff_cc;
  affected_card = old_aff_c;
  event_result = old_ev;
  trigger_cause = orig_tc;
  trigger_cause_controller = orig_tcc;
  // End additions

  trigger_condition = -1;

  --TRIGGER_DEPTH;
  if (TRIGGER_DEPTH == 0)
	{
	  for (p = 0; p < 2; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  get_card_instance(p, c)->state &= ~(STATE_PROCESSING | STATE_IS_TRIGGERING);

	  if (!EXE_DWORD(0x7A31AC))
		{
		  EXE_DWORD(0x60A538) = 0;
		  if (ai_is_speculating != 1)
			EXE_DWORD(0x60A54C) = 0;
		}
	}
  // Begin additions
  else
	{
	  ASSERT(processing_set);
	  for (p = 0; p < 2; ++p)
		{
		  ASSERT(active_cards_count[p] <= 150);
		  for (c = 0; c < active_cards_count[p]; ++c)
			{
			  card_instance_t* instance = get_card_instance(p, c);

			  if (instance->internal_card_id == -1)	// Any newly-obliterated cards always get processing removed.
				instance->state &= ~(STATE_PROCESSING | STATE_IS_TRIGGERING);
			  else
				{
				  if (processing[p][c] & 4)	// Restore the previous is-triggering state
					{
					  instance->state |= STATE_IS_TRIGGERING;
					  processing[p][c] &= ~4;
					}

				  if (processing[p][c])		// Newly-created cards (processing[p][c] == 2) and cards that had processing before (processing[p][c] == 1)
					instance->state |= STATE_PROCESSING;
				  else						// Unprocessed cards that existed before and after get it removed.
					instance->state &= ~STATE_PROCESSING;
				}
			}
		}
	}
  // End additions

  EXE_DWORD(0x62C180) = old_62C180;
  reason_for_trigger_controller = old_reason_for_trigger_controller;
  EXE_DWORD(0x62BCE8) = old_62BCE8;
  life_gained = old_life_gained;

#undef TRIGGER_DEPTH
}

int dispatch_trigger(int player, trigger_t trig, const char *prompt, int TENTATIVE_allow_response)
{
  xtrigger_t old_xtrigger = xtrigger_impl_value_dont_use_directly;
  dispatch_trigger_impl(player, trig, 0, prompt, TENTATIVE_allow_response);
  xtrigger_impl_value_dont_use_directly = old_xtrigger;
  return 0;
}

int dispatch_trigger2(int player, trigger_t trig, const char *prompt, int TENTATIVE_allow_response, int new_trigger_cause_controller, int new_trigger_cause)
{
  int old_trigger_cause = trigger_cause_controller;
  int old_trigger_cause_controller = trigger_cause_controller;
  xtrigger_t old_xtrigger = xtrigger_impl_value_dont_use_directly;

  trigger_cause_controller = new_trigger_cause_controller;
  trigger_cause = new_trigger_cause;

  dispatch_trigger_impl(  player, trig, 0, prompt, TENTATIVE_allow_response);
  dispatch_trigger_impl(1-player, trig, 0, prompt, TENTATIVE_allow_response);

  trigger_cause_controller = old_trigger_cause;
  trigger_cause_controller = old_trigger_cause_controller;
  xtrigger_impl_value_dont_use_directly = old_xtrigger;

  return 0;
}

int dispatch_xtrigger2(int player, xtrigger_t xtrig, const char *prompt, int TENTATIVE_allow_response, int new_trigger_cause_controller, int new_trigger_cause)
{
  int old_trigger_cause = trigger_cause_controller;
  int old_trigger_cause_controller = trigger_cause_controller;
  xtrigger_t old_xtrigger = xtrigger_impl_value_dont_use_directly;

  trigger_cause_controller = new_trigger_cause_controller;
  trigger_cause = new_trigger_cause;

  dispatch_trigger_impl(  player, TRIGGER_XTRIGGER, xtrig, prompt, TENTATIVE_allow_response);
  dispatch_trigger_impl(1-player, TRIGGER_XTRIGGER, xtrig, prompt, TENTATIVE_allow_response);

  trigger_cause_controller = old_trigger_cause;
  trigger_cause_controller = old_trigger_cause_controller;
  xtrigger_impl_value_dont_use_directly = old_xtrigger;

  return 0;
}

void resolve_trigger(int player, int card, int TENTATIVE_reason_for_trigger_controller)
{
  // Original at 434800

  if (trace_mode & 2)
	{
	  char buf[500];
	  sprintf(buf, "%d: Player #%d is processing %s(%d).\n", EXE_DWORD(0x60EC40)++, player, cards_data[get_card_instance(player, card)->internal_card_id].name, card);
	  EXE_FN(void, 0x4A7D80, const char*)(buf);	// append_to_trace_txt()
	}
  EXE_DWORD(0x60E9F8) = 0;	// This is only ever written to, and only ever set to 0, but I'm leaving it in place just in case

  put_card_or_activation_onto_stack(player, card, EVENT_RESOLVE_TRIGGER, TENTATIVE_reason_for_trigger_controller, 0);
  if (cancel == 1)
	{
	  obliterate_top_card_of_stack();
	  EXE_DWORD(0x60E9F8) = 0;
	}
  else
	{
	  if (ai_is_speculating != 1)
		{
		  EXE_FN(void, 0x436700, void)();	// set_stack_damage_targets()
		  if (reason_for_trigger_controller != 0)
			{
			  load_text(0, "PROMPT_PROC1");
			  char buf[300];
			  sprintf(buf, text_lines[0], opponent_name);
			  EXE_FN(int, 0x471b60, int, int, int, int, const char*, int)(player, card, -1, -1, buf, 0);	// raw_do_dialog()
			}

		  int p, c;
		  for (p = 0; p <= 1; ++p)
			for (c = 0; c < active_cards_count[p]; ++c)
			  get_card_instance(p, c)->state &= ~STATE_IS_TRIGGERING;
		}

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->state |= STATE_PROCESSING | STATE_IS_TRIGGERING;	// STATE_IS_TRIGGERING added

	  // Begin additions
	  int old_tcond = trigger_condition;
	  int old_xtcond = xtrigger_impl_value_dont_use_directly;
	  int old_tcc = trigger_cause_controller;
	  int old_tc = trigger_cause;
	  // End additions

	  EXE_FN(void, 0x436740, void)();	// resolve_top_card_on_stack();

	  // Begin additions
	  trigger_condition = old_tcond;
	  xtrigger_impl_value_dont_use_directly = old_xtcond;
	  trigger_cause = old_tc;
	  trigger_cause_controller = old_tcc;
	  instance->state &= ~STATE_IS_TRIGGERING;
	  // End additions

	  EXE_DWORD(0x60E9F8) = 0;

	  // Begin additions
	  dispatch_event(player, card, EVENT_TRIGGER_RESOLVED);

	  trigger_condition = old_tcond;
	  xtrigger_impl_value_dont_use_directly = old_xtcond;
	  trigger_cause = old_tc;
	  trigger_cause_controller = old_tcc;
	  // End additions
	}
}

int get_abilities(int player, int card, event_t event, int new_attacking_card)
{
  // 0x4352d0

  card_instance_t* instance = get_card_instance(player, card);
  int iid = instance->internal_card_id;
  if (iid == activation_card)
	{
	  if (event == EVENT_CHANGE_TYPE)
		return iid;
	  else
		return get_abilities(instance->parent_controller, instance->parent_card, event, new_attacking_card);
	}

  if (EXE_DWORD(0x60A548))
	push_affected_card_stack();

  affected_card_controller = player;
  affected_card = card;

  if (iid == -1)
	{
	  iid = instance->backup_internal_card_id;
	  instance->regen_status &= ~KEYWORD_RECALC_ALL;
	}

  EXE_DWORD(0x73825C) = iid;
  attacking_card = new_attacking_card;

  int preliminary, recalc = 0;
  switch (event)
	{
	  case EVENT_POWER:
		if (instance->regen_status & KEYWORD_RECALC_POWER)
		  {
			instance->regen_status &= ~KEYWORD_RECALC_POWER;
			preliminary = cards_data[iid].power;
			if ((instance->state & (STATE_OUBLIETTED|STATE_IN_PLAY)) == STATE_IN_PLAY && preliminary > 0)
			  preliminary &= ~0x4000;
			preliminary += instance->counter_power;
			recalc = 1;
		  }
		else
		  event_result = instance->power;
		break;

	  case EVENT_TOUGHNESS:
		if (instance->regen_status & KEYWORD_RECALC_TOUGHNESS)
		  {
			instance->regen_status &= ~KEYWORD_RECALC_TOUGHNESS;
			preliminary = cards_data[iid].toughness;
			if ((instance->state & (STATE_OUBLIETTED|STATE_IN_PLAY)) == STATE_IN_PLAY && preliminary > 0)
			  preliminary &= ~0x4000;
			preliminary += instance->counter_toughness;
			recalc = 1;
		  }
		else
		  event_result = instance->toughness;
		break;

	  case EVENT_ABILITIES:
		if (instance->regen_status & KEYWORD_RECALC_ABILITIES)
		  {
			instance->regen_status &= ~KEYWORD_RECALC_ABILITIES;

			if (iid == damage_card)
			  {
				event_result = instance->regen_status;
				break;
			  }

			preliminary = ((instance->regen_status & (KEYWORD_RECALC_SET_COLOR|KEYWORD_RECALC_POWER|KEYWORD_RECALC_TOUGHNESS|KEYWORD_RECALC_CHANGE_TYPE))
						   | cards_data[iid].static_ability);
			if (preliminary & (KEYWORD_PROT_COLORED | KEYWORD_BASIC_LANDWALK))
			  {
				if (preliminary & KEYWORD_PROT_COLORED)
				  preliminary = (preliminary & ~KEYWORD_PROT_COLORED) | get_sleighted_protection(player, card, (preliminary & KEYWORD_PROT_COLORED));

				if (preliminary & KEYWORD_BASIC_LANDWALK)
				  preliminary = (preliminary & ~KEYWORD_BASIC_LANDWALK) | get_hacked_walk(player, card, (preliminary & KEYWORD_BASIC_LANDWALK));
			  }

			if (EXE_DWORD(0x60A548))
			  {
				int typ = cards_data[iid].type;
				if ((typ & TYPE_PERMANENT) && !(typ & TYPE_EFFECT))
				  {
					instance->targets[16].card = 0;
					instance->state &= ~STATE_VIGILANCE;
					instance->token_status &= ~(STATUS_WALL_CAN_ATTACK | STATUS_CANT_ATTACK);
					remove_special_flags(player, card, SF_MODULAR | SF_HEXPROOF_OVERRIDE);
					remove_special_flags2(player, card, SF2_THOUSAND_YEAR_ELIXIR);
				  }
			  }

			recalc = 1;
		  }
		else
		  event_result = instance->regen_status;
		break;

	  case EVENT_CHANGE_TYPE:
		if (instance->internal_card_id == -1)
		  event_result = -1;
		else if ((instance->regen_status & KEYWORD_RECALC_CHANGE_TYPE)
			&& (iid < EXE_DWORD(0x628c64)/*iid_legacy_data_card*/ || iid > EXE_DWORD(0x786d5c)/*iid_legacy_swap_power_toughness*/))
		  {
			instance->regen_status &= ~KEYWORD_RECALC_CHANGE_TYPE;
			instance->backup_internal_card_id = preliminary = instance->internal_card_id = instance->original_internal_card_id;
			instance->mana_color = cards_data[instance->internal_card_id].color;
			instance->destroys_if_blocked = 0;
			instance->token_status &= ~STATUS_LEGACY_TYPECHANGE;
			remove_special_flags2(player, card, SF2_ENCHANTED_EVENING | SF2_MYCOSYNTH_LATTICE | SF2_TEMPORARY_COPY_OF_TOKEN);
			recalc = 1;
		  }
		else
		  event_result = iid;
		break;

	  case EVENT_RECALC_DAMAGE:
		preliminary = instance->damage_on_card;
		recalc = 1;
		break;

	  case EVENT_SET_COLOR:
		if (instance->regen_status & KEYWORD_RECALC_SET_COLOR)
		  {
			instance->regen_status &= ~KEYWORD_RECALC_SET_COLOR;

			if (iid == damage_card)
			  {
				event_result = instance->color;
				break;
			  }

			if (instance->initial_color)
			  preliminary = instance->initial_color;
			else
			  {
				card_data_t* cd = &cards_data[iid], *unaltered_cd;
				// Lands and mana-producing artifacts, stupidly enough, keep their mana color in cd->color.  Actual color is colorless, except for Dryad Arbor.
				if (cd->type & TYPE_LAND)
				  preliminary = cards_data[iid].id == CARD_ID_DRYAD_ARBOR ? COLOR_TEST_GREEN : 0;
				else if ((cd->type & TYPE_ARTIFACT) && (cd->extra_ability & EA_MANA_SOURCE)
						 /* Only if the unaltered version of the card is both an artifact and a mana source, too; otherwise, it'll make e.g. a Llanowar Elves
						  * that has been turned into an artifact incorrectly colorless. */
						 && (unaltered_cd = &cards_data[get_internal_card_id_from_csv_id(cd->id)])
						 && (unaltered_cd->type & TYPE_ARTIFACT) && (unaltered_cd->extra_ability & EA_MANA_SOURCE))
				  preliminary = 0;
				else
				  preliminary = cd->color;
			  }

			preliminary &= COLOR_TEST_ANY_COLORED;	// to deal with cards that incorrectly set COLOR_COLORLESS or COLOR_ARTIFACT as color, not mana_color

			recalc = 1;
		  }
		else
		  event_result = instance->color;
		break;

	  default:
		preliminary = 0;
		recalc = 1;
		break;
	}

  if (recalc)
	{
	  event_result = preliminary;
	  if (EXE_DWORD(0x60A548))
		{
		  dispatch_event_raw(event);

		  if ((land_can_be_played & LCBP_NEED_EVENT_CHANGE_TYPE_SECOND_PASS) && event == EVENT_CHANGE_TYPE)
			{
			  land_can_be_played &= ~LCBP_NEED_EVENT_CHANGE_TYPE_SECOND_PASS;
			  instance->internal_card_id = event_result;
			  if (event_result != -1)
				instance->backup_internal_card_id = event_result;

			  land_can_be_played |= LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS;
			  dispatch_event_raw(event);
			  land_can_be_played &= ~LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS;
			}
		}

	  if (event == EVENT_ABILITIES && (player_bits[player] & PB_CANT_HAVE_OR_GAIN_ABILITIES_MASK))
		{
		  int typ = cards_data[iid].type;
		  if ((typ & TYPE_PERMANENT) && !(typ & TYPE_EFFECT))
			{
			  if (player_bits[player] & PB_CANT_HAVE_OR_GAIN_FIRST_STRIKE)
				event_result &= ~KEYWORD_FIRST_STRIKE;
			  if (player_bits[player] & PB_CANT_HAVE_OR_GAIN_FLYING)
				event_result &= ~KEYWORD_FLYING;
			  if (player_bits[player] & PB_CANT_HAVE_OR_GAIN_DEATHTOUCH)
				instance->targets[16].card &= ~SP_KEYWORD_DEATHTOUCH;
			  if (player_bits[player] & PB_CANT_HAVE_OR_GAIN_TRAMPLE)
				event_result &= ~KEYWORD_TRAMPLE;
			  if (player_bits[player] & PB_CANT_HAVE_OR_GAIN_SHROUD)
				event_result &= ~KEYWORD_SHROUD;
			  if (player_bits[player] & PB_CANT_HAVE_OR_GAIN_HEXPROOF)
				instance->targets[16].card &= ~SP_KEYWORD_HEXPROOF;
			}
		}

	  if (event == EVENT_POWER)
		{
		  if (event_result < 0)
			event_result = 0;

		  if (instance->token_status & STATUS_BERSERK)
			event_result *= 2;
		}
	}

  int saved_event_result = event_result;
  if (event == EVENT_TOUGHNESS
	  && (instance->state & (STATE_OUBLIETTED|STATE_INVISIBLE|STATE_IN_PLAY)) == STATE_IN_PLAY
	  && (cards_data[iid].type & TYPE_CREATURE)
	  && (event_result <= 0
		  || instance->damage_on_card >= event_result)
	  && !(instance->token_status & STATUS_CANNOT_BE_DESTROYED)
	  && EXE_DWORD(0x785E60) == 0	// replaces //&& trigger_condition == -1
	  && !(land_can_be_played & (LCBP_REGENERATION|LCBP_DAMAGE_PREVENTION)))
	{
	  if (event_result > 0)
		kill_card(player, card, KILL_DESTROY);
	  else
		kill_card(player, card, KILL_STATE_BASED_ACTION);

	  EXE_FN(int, 0x477a90, void)();	// regenerate_or_graveyard_triggers()
	}

  if (EXE_DWORD(0x60A548))
	pop_affected_card_stack();

  if (event != EVENT_CHANGE_TYPE)
	{
	  switch (event)
		{
		  case EVENT_POWER:		instance->power = saved_event_result;			break;
		  case EVENT_TOUGHNESS:	instance->toughness = saved_event_result;		break;
		  case EVENT_SET_COLOR:	instance->color = saved_event_result & COLOR_TEST_ANY_COLORED;	break;
		  case EVENT_ABILITIES:
			instance->regen_status = saved_event_result;
			// EVENT_CHANGE_TYPE is the canonical location for this, but deathtouch usually is only added during EVENT_ABILITIES, and sometimes can only be.
			if (instance->targets[16].card & SP_KEYWORD_DEATHTOUCH)
			  {
				int typ = cards_data[iid].type;
				if ((typ & TYPE_PERMANENT) && !(typ & TYPE_EFFECT))
				  instance->destroys_if_blocked |= DIFB_DESTROYS_UNPROTECTED;
			  }
			break;
		  default:				break;
		}
	  return saved_event_result;
	}

  instance->internal_card_id = saved_event_result;
  if (saved_event_result != -1)
	instance->backup_internal_card_id = saved_event_result;

  card_data_t* cd = &cards_data[saved_event_result];
  if (cd->extra_ability & EA_MANA_SOURCE)
	{
	  // There were many special cases here, both before and after the call to get_color_of_mana_produced_by_id(); all are handled by that now.
	  int csvid = cd->id;
	  int override = get_color_of_mana_produced_by_id(csvid, instance->info_slot, player);
	  if (override != -1)
		instance->mana_color = override;
	  else
		instance->mana_color = cards_ptr[csvid]->mana_source_colors;

	  instance->card_color = instance->mana_color;
#if 0
	  /* Sends EVENT_MODIFY_MANA_PROD to cards with cc[2] & 2 set.  The only card that has that and responds to EVENT_MODIFY_MANA_PROD is Fertile Ground, so I'm
	   * not going to bother translating this into C.  Can't be called directly because of its nonstandard calling convention. */
	  rval_in_eax = TENTATIVE_modify_mana_prod__returns_eax_and_ecx(player, card);
	  instance->mana_color &= rval_in_ecx;
	  instance->mana_color |= rval_in_eax;
#endif
	}

  // Makes Enchant Creature auras fall off of previously-animated cards that are no longer creatures
  if (instance->token_status & STATUS_ANIMATED && !(cd->type & TYPE_CREATURE))
	{
	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c <= active_cards_count[p]; ++c)
		  if (in_play(p, c))
			{
			  card_instance_t* inst = get_card_instance(p, c);
			  if (inst->damage_target_player == player && inst->damage_target_card == card
				  && (cards_data[inst->internal_card_id].type & TYPE_ENCHANTMENT)
				  && cards_ptr[cards_data[inst->internal_card_id].id]->subtype1 == 44)
				kill_card(p, c, KILL_STATE_BASED_ACTION);
			}
	}

  if (instance->state & STATE_IN_PLAY)
	event_flags |= cards_data[instance->internal_card_id].extra_ability & (EA_MARTYR|EA_SELECT_ATTACK|EA_SELECT_BLOCK|EA_LICH|EA_PAID_ATTACK|
																				 EA_PAID_BLOCK|EA_FORCE_ATTACK|EA_BEFORE_COMBAT|EA_DECLARE_ATTACK|
																				 EA_FELLWAR_STONE|EA_CONTROLLED);

  return saved_event_result;
}

// Fixes a bug in the exe.  Do not call directly.
void event_activate_then_duplicate_into_stack(int player, int card, int event, int new_attacking_card_controller, int new_attacking_card)
{
  /* Normally, when a card is activated, 12 steps happen in activate() (at 0x434040) and two more happen elsewhere:
   * 1) If it's the AI playing the activation, do some unclear proprocessing, including (among much else) sending EVENT_GET_SELECTED_CARD to player/card and
   *    showing a dialog announcing the activation.
   * 2) an activation card is put onto the stack (put_card_or_activation_onto_stack() at 0x436550).  This involves:
   *   a) creating an activation card
   *   b) memcpy()ing the card as it currently exists into the activation card
   *   c) setting the activation card's internal_card_id (back to iid_activation_card), original_internal_card_id (to the creating card's internal_card_id),
   *      unknown0x14 and kill_code (both to 0), timestamp (back to what it was before the memcpy), and parent_controller and parent_card
   *   d) setting various stack variables (stack_size, stack_cards[], stack_damage_targets[], stack_phase[], and dword_628674[stack_size].
   *   For basic lands and for EVENT_RESOLVE_SPELL and EVENT_RESOLVE_TRIGGER, a, b, and c are skipped; a references to actual card is left on the stack.
   * 3) EVENT_UNKNOWN80 is dispatched, setting affected_card_controller/affected_card to player/card.  If event_result is set to nonzero, the top card of the
   *    stack is obliterated, and nothing else is done.  Nothing in the exe handles this event, and it seems dangerous to me - what if the AI keeps on trying
   *    to activate a card, and something responding to EVENT_UNKNOWN80 keeps cancelling it?
   * 4) Set tapped_for_mana_color = -1, and add an entry to the mana_spent stack (sub_430150)
   * 5) EVENT_ACTIVATE is dispatched to player/card (not the activation card) with dispatch_event_to_single_card() at 0x435B50 (almost the same as
   *    dispatch_event_with_attacker_to_one_card() above)
   * 6) If it's a network game and the remote player is activating, then (usually) show a dialog saying what he's doing.
   * 7) If cancel == 1, refund all mana spent since the mana_spent stack state was stored (sub_430200), obliterate the top card of the stack, and return.
   * 8) Remove the mana_stack entry (so cancelling afterwards won't refund mana spent so far) (sub_4301E0)
   * 9) Dispatch either EVENT_PLAY_ABILITY or both EVENT_TAP_CARD and EVENT_TAPPED_TO_PLAY_ABILITY, with affected_card_controller/affected_card == player/card.
   *10) Copy player/card's current state into the activation card again, preserving the same things as before; and set the activation card's display_pic_int_id
   *    to the original card's csvid and display_pic_num to 0.  (Digression: The original_internal_card_id saved when the activation_card was put on the stack
   *    is copied anew from player/card here, instead of preserving it; if player/card's internal_card_id is now -1 since it's no longer in play due to
   *    sacrificing or exiling or bouncing itself during activation or anything else that immediately gets rid of it, then it's pulled from player/card's
   *    original_internal_card.  This is why sacrifice-self abilities from Clone etc. get screwed up.)  Also set dword_628674[] and stack_damage_targets[] again
   *    here.  This happens in recopy_card_onto_stack().  If the top card on the stack wasn't an activation card (see the note at the end of step 2), then none
   *    of this except setting dword_628674[] and stack_damage_targets[] is done.
   *11) Play WAV_TAP if the card is flagged EA_MANA_SOURCE and tapped_for_mana_color != -1; otherwise, play WAV_FASTFX.
   *12) And call set_stack_damage_targets() at 0x436700, which updates stack_damage_targets[] for everything on the stack.  I suspect it's redundant.
   *13) Outside of activate(), (usually) allow response.
   *14) Eventually, resolve_top_card_on_stack() at 0x436740 is called.  This puts the stack down to the point it was at when this instance of activate() was
   *    called.
   *
   * When a mana source is activated during charge_mana(), an abbreviated sequence occurs:
   * 1) Send EVENT_CAN_ACTIVATE to the card, unless it's a land.  (I think that exception should be removed, but it's a problem for another day.)  On failure,
   *    do nothing else.
   * 2) Same as step 2 from above - activation card onto the stack.
   * 3) Store the colors of mana currently needed in needed_mana_colors.
   * 4) set tapped_for_mana_color = -1.
   * 5) Dispatch EVENT_ACTIVATE, just as in step 5 from above.
   * 6) (nothing)
   * 7) If cancel == 1, obliterate the top card of the stack, and continue onto the rest of charge_mana().
   * 8) (nothing)
   * 9) If the activating card is newly tapped, dispatch EVENT_TAP_CARD, otherwise do nothing.
   *10) (nothing)
   *11) Play WAV_TAP.
   *12) (nothing)
   *13) (nothing)
   *14) call resolve_top_card_on_stack() at 0x436740 and TENTATIVE_reassess_all_cards() at 0x472260.
   *
   * The sequence during autotap_mana_source() at 0x42FED0 is similar, though it at least always checks EVENT_CAN_ACTIVATE.
   *
   * The bug is that step 10 is missing - if a card sets its internal targets or info_slot during EVENT_ACTIVATE, and its EVENT_RESOLVE_ACTIVATION looks at
   * them, it'll see them as they were *before* EVENT_ACTIVATE was called.  This breaks mana sources that have non-mana abilities.  I'm currently seeing it with
   * Iceberg - if you activate it to add an ice counter (which takes place at resolution), and then cast something and pay for its mana with Iceberg's mana
   * ability, you get a free counter.  I'm mostly just amazed that it hasn't seemed to occur on lands with non-mana abilities.
   *
   * So what I'm doing is replacing the call to dispatch_event_to_single_card(player, card, EVENT_ACTIVATE, 1-player, -1) with a call here, ignoring the last
   * three parameters.  Here, we dispatch EVENT_ACTIVATE, then manually duplicate most of recopy_card_onto_stack() from step 10 in the full sequence. */

  dispatch_event_with_attacker_to_one_card(player, card, event, new_attacking_card_controller, new_attacking_card);

  // 0x436450
  int idx = stack_size - 1;
  card_instance_t* instance = get_card_instance(stack_cards[idx].player, stack_cards[idx].card);
  if (instance->internal_card_id == activation_card)
	{
	  int old_parent_card = instance->parent_card;
	  int old_parent_controller = instance->parent_controller;
	  uint32_t old_timestamp = instance->timestamp;
	  card_instance_t* parent_instance = get_card_instance(instance->parent_controller, instance->parent_card);

	  if (parent_instance->state & STATE_DONT_RECOPY_ONTO_STACK)
		parent_instance->state &= ~STATE_DONT_RECOPY_ONTO_STACK;
	  else
		{
		  STATIC_ASSERT(sizeof(card_instance_t) == 300, card_instance_t_size_differs_from_exe);
		  memcpy(instance, parent_instance, sizeof(card_instance_t));
		  instance->internal_card_id = activation_card;
		  instance->unknown0x14 = 0;
		  instance->kill_code = 0;
		  instance->state |= STATE_IN_PLAY;
		  instance->parent_controller = old_parent_controller;
		  instance->parent_card = old_parent_card;
		  instance->timestamp = old_timestamp;

		  if (parent_instance->internal_card_id != -1)
			instance->original_internal_card_id = parent_instance->internal_card_id;
		  // Begin additions
		  else
			instance->original_internal_card_id = parent_instance->backup_internal_card_id;
		  // End additions

		  // Begin removals
		  // if (instance->original_internal_card_id < iid_damage_card
		  //     || instance->original_internal_card_id >= iid_damage_card + 47)
		  //   {
		  //     instance->display_pic_int_id = cards_data[parent_instance->original_internal_card_id].id;
		  //     instance->display_pic_num = 0;
		  //   }
		  // End removals
		}

	  // Begin additions
	  instance->token_status &= ~STATUS_DYING;	// so kill_card() will reap it, even if the original card sacrificed itself as an activation card
	  // End additions
	}
  // Begin removals
  // if (ai_is_speculating != 1)
  //   dword_628674[idx] = arg1;	// 0 for EA_MANA_SOURCE cards that set tapped_for_mana_color; else 1
  // stack_damage_targets[idx].player = instance->damage_target_player;
  // stack_damage_targets[idx].card = instance->damage_target_card;
  // End removals
}

int current_trigger_or_event_is_forced(void)
{
  // 0x476F10

  /* I'm not positive what this function's exact effects are, but if a trigger that's not listed here runs, and the player has a stop on the current phase, he's
   * always prompted to select "Done" even if there's only one (forced) trigger handler, or none at all. */

  uint8_t ctoe = EXE_DWORD(0x62bce8) & 0xff;	// TENTATIVE_current_trigger_or_event
  if (ctoe == EVENT_UNKNOWN8E)
	return must_attack ? 1 : 0;

  if (ctoe != EVENT_CAN_SKIP_TURN
	  && ctoe != 107	//0x6B or -148 - I can't find any uses.
	  && ctoe != EVENT_CAST_SPELL
	  && ctoe != EVENT_ACTIVATE
	  && ctoe != EVENT_DEAL_DAMAGE
	  && ctoe != 111	//0x6F or -144 - I can't find any uses.
	  && ctoe != EVENT_REGENERATE
	  && ctoe != EVENT_RESOLVE_SPELL
	  && ctoe != EVENT_RESOLVE_ACTIVATION
	  && ctoe != EVENT_CAN_ACTIVATE
	  && ctoe != EVENT_CAN_CAST
	  && ctoe != 117	//0x75 or -138 - I can't find any uses.
	  && ctoe != 118	//0x76 or -137 - I can't find any uses.
	  && ctoe != EVENT_GRAVEYARD_FROM_PLAY
	  && ctoe != EVENT_BLOCK_LEGALITY
	  && ctoe != EVENT_ATTACK_LEGALITY
	  && ctoe != 122	//0x7A or -133 - I can't find any uses.
	  && ctoe != 123	//0x7B or -132 - I can't find any uses.
	  && ctoe != 124	//0x7C or -131 - I can't find any uses.
	  && ctoe != EVENT_TRIGGER
	  && ctoe != EVENT_RESOLVE_TRIGGER
	  && ctoe != EVENT_COUNT_MANA
	  && ctoe != EVENT_UNKNOWN80
	  && ctoe != EVENT_TAP_CARD
	  && ctoe != EVENT_UNTAP
	  && ctoe != EVENT_UNTAP_CARD
	  && ctoe != EVENT_SET_UNTAP_COST
	  && ctoe != EVENT_SETUP_UPKEEP_COSTS
	  && ctoe != EVENT_UPKEEP_COSTS_UNPAID
	  && ctoe != EVENT_CHECK_UPK_PAYMENT
	  && ctoe != EVENT_CHECK_UNTAP_PAYMENT
	  && ctoe != EVENT_MUST_ATTACK
	  && ctoe != EVENT_UNKNOWN8E
	  && ctoe != EVENT_SHOULD_AI_PLAY
	  && ctoe != 200	//0xC8 or -55 - I didn't even attempt to search.
	  && ctoe != TRIGGER_UPKEEP
	  && ctoe != TRIGGER_DURING_UPKEEP
	  && ctoe != TRIGGER_END_UPKEEP
	  && ctoe != TRIGGER_END_COMBAT
	  && ctoe != TRIGGER_EOT
	  && ctoe != TRIGGER_DRAW_PHASE
	  && ctoe != TRIGGER_REPLACE_CARD_DRAW
	  && ctoe != TRIGGER_CARD_DRAWN
	  && ctoe != TRIGGER_DISCARD
	  && ctoe != TRIGGER_TAP_CARD
	  && ctoe != TRIGGER_SPELL_CAST
	  && ctoe != TRIGGER_LEAVE_PLAY
	  && ctoe != TRIGGER_GRAVEYARD_FROM_PLAY
	  && ctoe != TRIGGER_GRAVEYARD_ORDER
	  && ctoe != TRIGGER_DEAL_DAMAGE
	  && ctoe != TRIGGER_BOUNCE_PERMANENT
	  && ctoe != TRIGGER_MUST_ATTACK
	  && ctoe != TRIGGER_COMES_INTO_PLAY
	  && ctoe != TRIGGER_PAY_TO_ATTACK
	  && ctoe != TRIGGER_XTRIGGER
	  )
	return 1;

  return 0;
}
