// -*- c-basic-offset:2 -*-
#include <limits.h>
#include <windows.h>

#include "manalink.h"

typedef struct
{
  const char* text;
  int legality;
  int ai_priority;
  int loyalty;
  signed char mana[8];
  target_definition_t* td;
  const char* tgt_prompt;
  enum
  {
	DF_TAP				= 1<<0,
	DF_LITERAL_PROMPT	= 1<<1,
	DF_99				= 1<<2,
  } flags;
} choice_t;
#define MAX_CHOICES	16	// Arbitrary, though already probably more than do_dialog() can handle

typedef enum
{
  RND_NO = 0,			// Always pick highest-valued choice
  RND_SPECULATE = 1,	// Speculate, using choice values as a guide.  DLG_RANDOM.
  RND_RANDOM = 2,		// Pick randomly, using choice values as a guide.  DLG_RANDOM_NO_SAVE.
} dlg_random_t;

static int
displayed_to_real(choice_t* choice, int choices, int omit_illegal, int displayed_value)
{
  if (omit_illegal)
	{
	  // convert the index of displayed options to index of all options
	  int i;
	  for (i = 0; i <= displayed_value && i < choices; ++i)
		if (!choice[i].legality)
		  ++displayed_value;
	}
  return displayed_value;
}

static int
real_to_displayed(choice_t* choice, int choices, int omit_illegal, int real_value)
{
  if (omit_illegal)
	{
	  int i = 0, displayed_idx = 0;
	  while (i < choices)
		{
		  if (choice[i].legality)
			{
			  --real_value;
			  if (real_value < 0)
				return displayed_idx;

			  ++displayed_idx;
			}

		  ++i;
		}

	  return displayed_idx;
	}

  return real_value;
}

static int
dialog_ai(int who_chooses, choice_t* choice, int choices, int omit_illegal, dlg_random_t random, int ai_choice1)
{
  int displayed_highest_idx = 0, real_highest_idx = 0;
  int highest_priority = INT_MIN;
  int total_legal_positive_priorities = 0;
  int i = 0, displayed_idx = 0;
  while (i < choices)
	{
	  if (choice[i].legality && displayed_idx != ai_choice1)
		{
		  if (choice[i].ai_priority > 0)
			total_legal_positive_priorities += choice[i].ai_priority;

		  if (choice[i].ai_priority > highest_priority)
			{
			  highest_priority = choice[i].ai_priority;
			  displayed_highest_idx = displayed_idx;
			  real_highest_idx = i;
			}

		  ++displayed_idx;
		}
	  else if (!omit_illegal)
		++displayed_idx;

	  ++i;
	}

  if (random == RND_NO)
	return displayed_highest_idx;

  if (highest_priority <= 0 || highest_priority >= 1000000)
	{
	  remember_ai_value(who_chooses, real_highest_idx);
	  return real_highest_idx;
	}

  if (ai_is_speculating != 1 && IS_AI(who_chooses) && random == RND_SPECULATE)		// i.e., really the AI player, not the AI speculating for human's move
	{
	  int value = remember_ai_value(who_chooses, 0);	// Just retrieve, since AI is definitely not speculating
	  // Sanity check, in case the remembered values got out of sync, perhaps by uncontrolled calls to internal_rand()
	  if (value < choices && choice[value].legality)
		{
		  displayed_idx = real_to_displayed(choice, choices, omit_illegal, value);
		  if (displayed_idx != ai_choice1)
			return displayed_idx;
		}

	  // Otherwise, fall through to true random choice.
	}

  i = displayed_idx = 0;
  int chosen = internal_rand(total_legal_positive_priorities);
  while (i < choices)
	{
	  if (choice[i].legality && displayed_idx != ai_choice1)
		{
		  if (choice[i].ai_priority > 0)
			{
			  chosen -= choice[i].ai_priority;
			  if (chosen < 0)
				{
				  if (ai_is_speculating == 1 && random == RND_SPECULATE)
					remember_ai_value(who_chooses, i);	// Just store, since AI is definitely speculating

				  return displayed_idx;
				}
			}

		  ++displayed_idx;
		}
	  else if (!omit_illegal)
		++displayed_idx;

	  ++i;
	}

  // shouldn't get here
  return displayed_highest_idx;
}

static int
prepare_dialog_prompt(char* buf, choice_t* choice, int choices, const char* header_txt, int omit_illegal, int no_cancel)
{
  int i, num_legal = 0;

  if (ai_is_speculating == 1)
	{
	  *buf = 0;

	  for (i = 0; i < choices; ++i)
		if (choice[i].legality)
		  ++num_legal;
	}
  else
	{
	  char *p = buf;

	  if (header_txt)
		p += scnprintf(p, 600 - (p-buf), "%s\n", header_txt);

	  for (i = 0; i < choices; ++i)
		if (choice[i].legality)
		  {
			p += scnprintf(p, 600 - (p-buf), " %s\n", choice[i].text);
			++num_legal;
		  }
		else if (!omit_illegal)
		  p += scnprintf(p, 600 - (p-buf), " _%s\n", choice[i].text);

	  if (!no_cancel)
		p += scnprintf(p, 600 - (p-buf), " %s", EXE_STR(0x7281A4));	// localized Cancel
	  else if (p > buf && *--p == '\n')	// strip the \n from the last choice (or possibly header text)
		*p = 0;

	  buf[599] = 0;
	}

  return num_legal;
}

int
dialog_impl(int player, int card, event_t event, ...)
{
  if (event != EVENT_CAN_CAST
	  && event != EVENT_CAST_SPELL
	  && event != EVENT_RESOLVE_SPELL
	  && event != EVENT_CAN_ACTIVATE
	  && event != EVENT_ACTIVATE
	  && event != EVENT_RESOLVE_ACTIVATION)
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  choice_t choice[MAX_CHOICES];
  int choices = 0;	// Number of elements in choice[]

  dlg_random_t random = RND_NO;
  int no_cancel = 0;
  int omit_illegal = 0;
  int* second_choice = NULL;
  int sort_results = 0;
  int fullcard_player = player;
  int fullcard_card = card;
  int smallcard_player = -1;
  int smallcard_card = -1;
  int who_chooses = player;
  const char* header_txt = NULL;
  int fullcard_id = -1;
  int fullcard_id_is_csv = 0;
  int no_storage = 0;
  int autochoose_if_1 = 0;
  int no_display_for_ai = 0;
  int planeswalker_loyalty = 0;
  int loyalty_counters = 0;
  int smallcard_id = -1;
  int smallcard_id_is_csv = 0;
  unsigned char* store_in = (unsigned char*)(&instance->info_slot);
  int any_99 = 0;
  int any_targets = 0;

  va_list args;
  va_start(args, event);

  char* arg;
  while ((arg = va_arg(args, char*)))
	if (*arg == DLGIMPL_CH)
	  switch ((DlgImplCh)arg[1])
		{
		  case DLGIMPL_CH:
		  case DLGIMPL_NO_OP_CH:
			break;

		  case DLGIMPL_RANDOM_CH:
			random = RND_SPECULATE;
			break;

		  case DLGIMPL_RANDOM_NO_SAVE_CH:
			random = RND_RANDOM;
			break;

		  case DLGIMPL_NO_CANCEL_CH:
			no_cancel = 1;
			break;

		  case DLGIMPL_OMIT_ILLEGAL_CH:
			omit_illegal = 1;
			break;

		  case DLGIMPL_CHOOSE_TWO_CH:
			second_choice = va_arg(args, int*);
			break;

		  case DLGIMPL_FULLCARD_CH:
			fullcard_player = va_arg(args, int);
			fullcard_card = va_arg(args, int);
			break;

		  case DLGIMPL_SMALLCARD_CH:
			smallcard_player = va_arg(args, int);
			smallcard_card = va_arg(args, int);
			break;

		  case DLGIMPL_WHO_CHOOSES_CH:
			who_chooses = va_arg(args, int);
			break;

		  case DLGIMPL_HEADER_CH:
			header_txt = va_arg(args, const char*);
			break;

		  case DLGIMPL_FULLCARD_ID_CH:
			fullcard_id = va_arg(args, int);
			fullcard_id_is_csv = 0;
			break;

		  case DLGIMPL_FULLCARD_CSVID_CH:
			fullcard_id = va_arg(args, int);
			fullcard_id_is_csv = 1;	// delay converting to an internal card id until we know that it's needed
			break;

		  case DLGIMPL_NO_STORAGE_CH:
			no_storage = 1;
			event = EVENT_CAST_SPELL;
			break;

		  case DLGIMPL_SORT_RESULTS_CH:
			sort_results = 1;
			break;

		  case DLGIMPL_AUTOCHOOSE_IF_1_CH:
			autochoose_if_1 = 1;
			break;

		  case DLGIMPL_PLANESWALKER_CH:
			planeswalker_loyalty = 1;
			loyalty_counters = count_counters(player, card, COUNTER_LOYALTY);
			break;

		  case DLGIMPL_MANA_CH:
			ASSERT(choices > 0 && "DLG_MANA can't precede the first choice");
			choice[choices-1].mana[COLOR_COLORLESS]	= va_arg(args, int);
			choice[choices-1].mana[COLOR_BLACK]		= va_arg(args, int);
			choice[choices-1].mana[COLOR_BLUE]		= va_arg(args, int);
			choice[choices-1].mana[COLOR_GREEN]		= va_arg(args, int);
			choice[choices-1].mana[COLOR_RED]		= va_arg(args, int);
			choice[choices-1].mana[COLOR_WHITE]		= va_arg(args, int);
			choice[choices-1].mana[COLOR_ARTIFACT]	= 0;
			choice[choices-1].mana[COLOR_ANY]		= 1;

			if (choice[choices-1].legality)
			  {
				if (event == EVENT_CAN_CAST || event == EVENT_CAST_SPELL)
				  {
					if (!has_mana_multi(player, choice[choices-1].mana[COLOR_COLORLESS], choice[choices-1].mana[COLOR_BLACK],
										choice[choices-1].mana[COLOR_BLUE], choice[choices-1].mana[COLOR_GREEN], choice[choices-1].mana[COLOR_RED],
										choice[choices-1].mana[COLOR_WHITE]))
					  choice[choices-1].legality = 0;
				  }
				else if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE)
				  {
					if (!has_mana_for_activated_ability(player, card, choice[choices-1].mana[COLOR_COLORLESS], choice[choices-1].mana[COLOR_BLACK],
														choice[choices-1].mana[COLOR_BLUE], choice[choices-1].mana[COLOR_GREEN],
														choice[choices-1].mana[COLOR_RED], choice[choices-1].mana[COLOR_WHITE]))
					  choice[choices-1].legality = 0;
				  }
			  }
			break;

		  case DLGIMPL_LITERAL_TARGET_CH:
			choice[choices - 1].flags |= DF_LITERAL_PROMPT;
			// and fall through
		  case DLGIMPL_TARGET_CH:
			any_targets = 1;
			ASSERT(choices > 0 && "DLG_TARGET can't precede the first choice");
			choice[choices - 1].td = va_arg(args, target_definition_t*);
			choice[choices - 1].tgt_prompt = va_arg(args, const char*);
			if (choice[choices-1].legality
				&& (event == EVENT_CAN_CAST || event == EVENT_CAST_SPELL || event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE)
				&& !can_target(choice[choices - 1].td))
			  choice[choices-1].legality = 0;
			break;

		  case DLGIMPL_TAP_CH:
			ASSERT(choices > 0 && "DLG_TAP can't precede the first choice");
			choice[choices - 1].flags |= DF_TAP;
			if (choice[choices-1].legality
				&& (event == EVENT_CAN_CAST || event == EVENT_CAST_SPELL || event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE)
				&& !CAN_TAP(player, card))
			  choice[choices-1].legality = 0;
			break;

		  case DLGIMPL_SMALLCARD_ID_CH:
			smallcard_id = va_arg(args, int);
			smallcard_id_is_csv = 0;
			break;

		  case DLGIMPL_SMALLCARD_CSVID_CH:
			smallcard_id = va_arg(args, int);
			smallcard_id_is_csv = 1;	// delay converting to an internal card id until we know that it's needed
			break;

		  case DLGIMPL_STORE_IN_CH:
			store_in = va_arg(args, unsigned char*);
			break;

		  case DLGIMPL_NO_DISPLAY_FOR_AI_CH:
			no_display_for_ai = 1;
			break;

		  case DLGIMPL_99_CH:
			ASSERT(choices > 0 && "DLG_99 can't precede the first choice");
			choice[choices - 1].flags |= DF_99;
			any_99 = 1;
			break;
		}
	else
	  {
		ASSERT(choices < MAX_CHOICES);
		choice[choices].text = arg;
		choice[choices].legality = va_arg(args, int);
		choice[choices].ai_priority = va_arg(args, int);
		if (planeswalker_loyalty)
		  {
			choice[choices].loyalty = va_arg(args, int);
			if (-choice[choices].loyalty > loyalty_counters)
			  choice[choices].legality = 0;
		  }
		memset(choice[choices].mana, 0, sizeof(choice[choices].mana));
		choice[choices].td = NULL;
		choice[choices].tgt_prompt = NULL;
		choice[choices].flags = 0;
		++choices;
	  }

  va_end(args);

  if (event == EVENT_CAN_CAST || event == EVENT_CAN_ACTIVATE)
	{
	  int i;
	  int num_ok = 0, num_ok_99 = 0, needed = second_choice ? 2 : 1;
	  for (i = 0; i < choices; ++i)
		if (choice[i].legality && (player != AI || choice[i].ai_priority > 0))
		  {
			++num_ok;
			if (choice[i].flags & DF_99)
			  ++num_ok_99;

			if (num_ok >= needed
				&& (!any_99
					|| num_ok_99 > 0))
			  break;
		  }

	  if (num_ok >= needed)
		return num_ok_99 ? 99 : 1;
	  else
		return 0;
	}

  if (event == EVENT_CAST_SPELL || event == EVENT_ACTIVATE)
	{
	  if (fullcard_id == -1 && instance->internal_card_id == -1)
		fullcard_id = instance->backup_internal_card_id;

	  if (fullcard_id_is_csv && (ai_is_speculating != 1))
		fullcard_id = get_internal_card_id_from_csv_id(fullcard_id);

	  if (smallcard_id_is_csv && (ai_is_speculating != 1))
		smallcard_id = get_internal_card_id_from_csv_id(smallcard_id);

	  int ai_choice1 = 0, ai_choice2 = 0;
	  if (IS_AI(who_chooses))
		{
		  ai_choice1 = dialog_ai(who_chooses, choice, choices, omit_illegal, random, -1);
		  if (second_choice)
			{
			  ai_choice2 = dialog_ai(who_chooses, choice, choices, omit_illegal, random, ai_choice1);
			  if (sort_results && ai_choice2 < ai_choice1)
				SWAP(ai_choice1, ai_choice2);
			}
		}

	  if (fullcard_id >= 0 && (ai_is_speculating != 1))
		{
		  fullcard_card = add_card_to_hand(fullcard_player, fullcard_id);
		  get_card_instance(fullcard_player, fullcard_card)->state |= STATE_INVISIBLE;
		  --hand_count[fullcard_player];
		}

	  if (smallcard_id >= 0 && (ai_is_speculating != 1))
		{
		  smallcard_player = player;
		  smallcard_card = add_card_to_hand(smallcard_player, smallcard_id);
		  get_card_instance(smallcard_player, smallcard_card)->state |= STATE_INVISIBLE;
		  --hand_count[smallcard_player];
		}

	  char buf[600];	// the most do_dialog() can handle
	  int num_legal = prepare_dialog_prompt(buf, choice, choices, header_txt, omit_illegal, no_cancel);
	  int chosen1 = 0;
	  if (num_legal == 1 && autochoose_if_1 && !IS_AI(who_chooses) && !second_choice)
		{
		  int i;
		  for (i = 0; i < choices; ++i)
			if (choice[i].legality)
			  {
				chosen1 = i;
				break;
			  }
		}
	  else
		{
		  if (num_legal < (second_choice ? 2 : 1) && choices > 0)
			goto cancelled;

		  if (no_display_for_ai && IS_AI(who_chooses))
			chosen1 = ai_choice1;
		  else
			chosen1 = do_dialog(who_chooses, fullcard_player, fullcard_card, smallcard_player, smallcard_card, buf, ai_choice1);

		  if (choices <= 0)
			goto cancelled;

		  chosen1 = displayed_to_real(choice, choices, omit_illegal, chosen1);
		}

	  if (chosen1 < 0 || chosen1 >= choices)
		goto cancelled;

	  ++chosen1;

	  if (second_choice)
		{
		  choice[chosen1 - 1].legality = 0;

		  if (prepare_dialog_prompt(buf, choice, choices, header_txt, omit_illegal, no_cancel) < 1)
			goto cancelled;

		  int chosen2;
		  if (no_display_for_ai && IS_AI(who_chooses))
			chosen2 = ai_choice2;
		  else
			chosen2 = do_dialog(who_chooses, fullcard_player, fullcard_card, smallcard_player, smallcard_card, buf, ai_choice2);

		  chosen2 = displayed_to_real(choice, choices, omit_illegal, chosen2);

		  if (chosen2 < 0 || chosen2 >= choices)
			goto cancelled;

		  ++chosen2;

		  if (sort_results && chosen2 < chosen1)
			SWAP(chosen1, chosen2);

		  int i, mana[8] = {0};
		  if (choice[chosen1-1].mana[COLOR_ANY])
			for (i = 0; i < 8; ++i)
			  mana[i] += choice[chosen1-1].mana[i];
		  if (choice[chosen2-1].mana[COLOR_ANY])
			for (i = 0; i < 8; ++i)
			  mana[i] += choice[chosen2-1].mana[i];
		  if (mana[COLOR_ANY] > 0
			  && !(event == EVENT_CAST_SPELL
				   ? charge_mana_multi(player, mana[COLOR_COLORLESS], mana[COLOR_BLACK], mana[COLOR_BLUE], mana[COLOR_GREEN], mana[COLOR_RED],
									   mana[COLOR_WHITE])
				   : charge_mana_for_activated_ability(player, card, mana[COLOR_COLORLESS], mana[COLOR_BLACK], mana[COLOR_BLUE], mana[COLOR_GREEN],
													   mana[COLOR_RED], mana[COLOR_WHITE])))
			{
			  if (no_storage)
				spell_fizzled = 0;
			  goto cancelled;
			}

		  if (!no_storage)
			{
			  store_in[0] = chosen1;
			  store_in[1] = chosen2;
			}

		  *second_choice = chosen2;
		}
	  else
		{
		  signed char* mana = &choice[chosen1-1].mana[0];
		  if ((mana[COLOR_ANY] || (planeswalker_loyalty && choice[chosen1-1].loyalty != 999))
			  && !(event == EVENT_CAST_SPELL
				   ? charge_mana_multi(player, mana[COLOR_COLORLESS], mana[COLOR_BLACK], mana[COLOR_BLUE], mana[COLOR_GREEN], mana[COLOR_RED],
									   mana[COLOR_WHITE])
				   : charge_mana_for_activated_ability(player, card, mana[COLOR_COLORLESS], mana[COLOR_BLACK], mana[COLOR_BLUE], mana[COLOR_GREEN],
													   mana[COLOR_RED], mana[COLOR_WHITE])))
			{
			  if (no_storage)
				spell_fizzled = 0;
			  goto cancelled;
			}

		  if (any_targets)
			{
			  instance->number_of_targets = 0;
			  if (choice[chosen1-1].td)
				{
				  if ((choice[chosen1-1].flags & DF_LITERAL_PROMPT)
					  ? !pick_next_target_noload(choice[chosen1-1].td, choice[chosen1-1].tgt_prompt)
					  : !pick_target(choice[chosen1-1].td, choice[chosen1-1].tgt_prompt))
					{
					  if (no_storage)
						spell_fizzled = 0;
					  goto cancelled;
					}
				}
			}

		  if (choice[chosen1-1].flags & DF_TAP)
			tap_card(player, card);

		  if (!no_storage)
			*store_in = chosen1;
		}

	  if (0)	// Accessed only via aborts from above
		{
		cancelled:
		  chosen1 = 0;
		  if (!no_storage)
			spell_fizzled = 1;
		}

	  if (planeswalker_loyalty)
		{
		  int cost = 0;
		  if (spell_fizzled != 1)
			{
			  cost = choice[chosen1 - 1].loyalty;

			  if (cost == 999)
				cost = 0;

			  if (cost + loyalty_counters <= 0)
				ai_modifier -= 256;
			  else if (chosen1 == choices && cost < 0)
				ai_modifier -= 24 * cost;	// bonus equal to the penalty planeswalker() will give for removing those counters
			}

		  SET_SBYTE1(instance->targets[9].player) = cost;
		}

	  if (fullcard_id >= 0 && (ai_is_speculating != 1))
		obliterate_card(fullcard_player, fullcard_card);

	  if (smallcard_id >= 0 && (ai_is_speculating != 1))
		obliterate_card(smallcard_player, smallcard_card);

	  return chosen1;
	}

  // Otherwise, event == EVENT_RESOLVE_SPELL || event == EVENT_RESOLVE_ACTIVATION.
  if (choice[store_in[0]-1].td && !valid_target(choice[store_in[0]-1].td))
	return 0;

  if (second_choice)
	*second_choice = store_in[1];

  return store_in[0];
}

#if 0
// Example usage:
int is_spell_on_stack(void){return 1;}
int can_target_permanent(void){return 1;}
int can_target_opponents_permanent(void){return 1;}
int cards_in_ai_deck(void){return 1;}
int tap_all_creatures(void){return 1;}
int
card_cryptic_command2(int player, int card, event_t event)
{
  // Choose two - Counter target spell; or return target permanent to its owner's hand; or tap all creatures your opponents control; or draw a card.

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_CAST || event == EVENT_CAST_SPELL || event == EVENT_RESOLVE_SPELL)	// Perhaps add a #define IS_CASTING(event)	((event) == EVENT_CAN_CAST || (event) == ...) etc. macro, and one for activation?
	{
	  // Making the choices into symbolic constants like this isn't necessary, but I think it's clearer.
#define CHOICE_COUNTER	1
#define CHOICE_BOUNCE	2
#define CHOICE_TAP		3
#define CHOICE_DRAW		4

	  int results[2];
	  results[0] = DIALOG(player, card, event,
						  DLG_CHOOSE_TWO(&results[1]),
						  "Counter target spell", is_spell_on_stack(), 100,	// ai's favorite choice by far, if it's legal.
						  "Bounce target permanent", can_target_permanent(), can_target_opponents_permanent() ? 30 : -20,	// An iffy choice if can target an opponent's permanent, and a legal but actively harmful one if can only target his own.  If the only legal choices for the ai include targeting his own permanent, this will return 0 for EVENT_CAN_CAST.
						  "Tap creatures", 1, 5,	// Always legal, and always very low priority.
						  "Draw a card", 1, cards_in_ai_deck() > 10 ? 50 : -100);	// Always legal.  Usually a pretty good choice, but ruinous if 10 or fewer cards in ai's deck.

	  if (results[0] == 0 && results[1] == 0)
		return 0;

	  if (event == EVENT_CAN_CAST)
		return 1;
	  else if (event == EVENT_CAST_SPELL)
		{
		  int i;
		  instance->number_of_targets = 0;
		  for (i = 0; i <= 1; ++i)
			{
			  switch (results[i])
				{
				  case CHOICE_COUNTER:
					instance->targets[instance->number_of_targets].player = card_on_stack_controller;
					instance->targets[instance->number_of_targets].card = card_on_stack;
					++instance->number_of_targets;
					break;
				  case CHOICE_BOUNCE:
					select_target(player, card, /*td*/NULL, "Choose a target to bounce.", &instance->targets[instance->number_of_targets]);
					++instance->number_of_targets;	// only if select_target() doesn't; I don't remember.
					break;
				  case CHOICE_TAP:		break;	// Untargeted
				  case CHOICE_DRAW:		break;	// Untargeted
				}
			  if (cancel == 1)
				return 0;
			}
		}
	  else	// event == EVENT_RESOLVE_SPELL
		{
		  int i, fizzled[2] = { 0, 0 };
		  for (i = 0; i <= 1; ++i)
			{
			  int tgt = (instance->number_of_targets == 2 ? i : 0);	// if two targets, then first is in 0 and second is in 1.  If only one target, it's always in 0.
			  switch (results[i])
				{
				  case CHOICE_COUNTER:	counterspell(player, card, event, NULL, 0);	break;
				  case CHOICE_BOUNCE:
					if (validate_target(player, card, /*td*/NULL, tgt))
					  bounce_permanent(instance->targets[tgt].player, instance->targets[tgt].card);
					else
					  fizzled[i] = 1;
					break;
				  case CHOICE_TAP:		tap_all_creatures();	break;
				  case CHOICE_DRAW:		draw_a_card(player);	break;
				  case 0:				fizzled[i] = 1;			break;
				}
			  if (cancel == 1)
				{
				  fizzled[i] = 1;
				  cancel = 0;
				}
			}
		  if (fizzled[0] == 1 && fizzled[1] == 1)
			cancel = 1;
		}

#undef CHOICE_COUNTER
#undef CHOICE_BOUNCE
#undef CHOICE_TAP
#undef CHOICE_DRAW
	}

  return 0;
}
#endif

#define option_Layout		EXE_DWORD(0x787260)
#define hwnd_FullCardClass	EXE_PTR_VOID(0x715ca0)

INT_PTR __stdcall dlgproc_do_dialog_hook(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  if (msg == (option_Layout == 2 ? WM_RBUTTONDBLCLK : WM_RBUTTONDOWN))
	{
	  SendMessageA(hwnd_FullCardClass, WM_COMMAND, 1, 0);
	  return 0;
	}

  return EXE_STDCALL_FN(INT_PTR, 0x4aa090, HWND, UINT, WPARAM, LPARAM)(hwnd, msg, wparam, lparam);	// dlgproc_do_dialog()
}
