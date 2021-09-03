#include "manalink.h"

int count_upkeeps(int player){
	int i;
	int result = 1;
	int eh_flag = 0;
	for(i=0; i<2; i++){
		int count = 0;
		while( count < active_cards_count[i] ){
				if( in_play(i, count) && get_id(i, count) == CARD_ID_EON_HUB ){
					eh_flag = 1;
					result = 0;
					break;
				}
				if( in_play(i, count) && get_id(i, count) == CARD_ID_PARADOX_HAZE ){
					card_instance_t *instance = get_card_instance(i, count);
					if( instance->targets[0].player == player ){
						result++;
					}
				}
				if( in_play(i, count) && get_id(i, count) == CARD_ID_GIBBERING_DESCENT ){
					if( i == player && hand_count[i] < 1 ){
						result--;
					}
				}
				count++;
		}
		if( eh_flag == 1 ){
			break;
		}
	}
	if( result < 0 ){
		result = 0;
	}
	return result;
}


void echo(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){

	if( event == EVENT_RESOLVE_SPELL ){
		set_special_flags(player, card, SF_ECHO_TO_PAY);
	}

	if( check_special_flags(player, card, SF_ECHO_TO_PAY) && ! check_special_flags(player, card, SF_ECHO_PAID) && 
		current_turn == player && upkeep_trigger(player, card, event) 
	  ){
		set_special_flags(player, card, SF_ECHO_PAID);
		if( count_upkeeps(player) > 0 ){
			int kill = 1;
			if( has_mana_multi(player, colorless, black, blue, green, red, white) ){
				int choice = do_dialog(player, player, card, -1, -1, " Pay echo\n Decline", 0);
				if( choice == 0 ){
					if( ! check_battlefield_for_id(player, CARD_ID_THICK_SKINNED_GOBLIN) ){
						charge_mana_multi(player, colorless, black, blue, green, red, white);
					}
					if( spell_fizzled != 1 ){
						kill = 0;
					}
				}
			}

			if( kill == 1 ){
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}
}

void fading(int player, int card, event_t event, int fading_counters){

	/* 702.31a Fading is a keyword that represents two abilities. "Fading N" means "This permanent enters the battlefield with N fade counters on it" and "At
	 * the beginning of your upkeep, remove a fade counter from this permanent. If you can't, sacrifice the permanent." */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_FADE, fading_counters);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( count_counters(player, card, COUNTER_FADE) <= 0 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
		else{
			remove_counter(player, card, COUNTER_FADE);
		}
	}

}

// As basic_upkeep(), but allows a different card to trigger.
void basic_upkeep_arbitrary(int triggering_player, int triggering_card, int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white)
{
  upkeep_trigger_ability(triggering_player, triggering_card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  int smp, smc;
	  if (triggering_card == card && triggering_player == player)
		smp = smc = -1;
	  else
		{
		  smp = triggering_player;
		  smc = triggering_card;
		}

	  if (!(has_mana_multi(player, colorless, black, blue, green, red, white)
			&& DIALOG(triggering_player, triggering_card, EVENT_ACTIVATE,
					  DLG_FULLCARD(player, card), DLG_SMALLCARD(smp, smc), DLG_WHO_CHOOSES(player),
					  DLG_RANDOM, DLG_NO_STORAGE, DLG_AUTOCHOOSE_IF_1, DLG_NO_CANCEL,
					  "Pay upkeep", has_mana_multi(player, colorless, black, blue, green, red, white), 1,
					  "Decline", 1, 1) == 1
			&& charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, colorless, black, blue, green, red, white)))
		{
		  cancel = 0;
		  kill_card(player, card, KILL_SACRIFICE);
		}
	}
}

void basic_upkeep(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white)
{
  basic_upkeep_arbitrary(player, card, player, card, event, colorless, black, blue, green, red, white);
}

// As basic_upkeep(), but returns true if upkeep is unpaid instead of sacrificing.
int basic_upkeep_unpaid(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white)
{
  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  if (!(has_mana_multi(player, colorless, black, blue, green, red, white)
			&& DIALOG(player, card, EVENT_ACTIVATE,
					  DLG_FULLCARD(player, card), DLG_WHO_CHOOSES(player),
					  DLG_RANDOM, DLG_NO_STORAGE, DLG_AUTOCHOOSE_IF_1, DLG_NO_CANCEL,
					  "Pay upkeep", has_mana_multi(player, colorless, black, blue, green, red, white), 1,
					  "Decline", 1, 1) == 1
			&& charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, colorless, black, blue, green, red, white)))
		{
		  cancel = 0;
		  return 1;
		}
	}

  return 0;
}

// As cumulative_upkeep(), but allows a different card to trigger.
int cumulative_upkeep_arbitrary(int triggering_player, int triggering_card, int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white)
{
  /* 702.23a Cumulative upkeep is a triggered ability that imposes an increasing cost on a permanent. "Cumulative upkeep [cost]" means "At the beginning of
   * your upkeep, if this permanent is on the battlefield, put an age counter on this permanent. Then you may pay [cost] for each age counter on it. If you
   * don't, sacrifice it." If [cost] has choices associated with it, each choice is made separately for each age counter, then either the entire set of costs
   * is paid, or none of them is paid. Partial payments aren't allowed.
   *
   * Example: A creature has "Cumulative upkeep {W} or {U}" and two age counters on it. When its ability next triggers and resolves, the creature's controller
   * puts an age counter on it and then may pay {W}{W}{W}, {W}{W}{U}, {W}{U}{U}, or {U}{U}{U} to keep the creature on the battlefield.
   *
   * Example: A creature has "Cumulative upkeep -- Sacrifice a creature" and one age counter on it. When its ability next triggers and resolves, its
   * controller can't choose the same creature to sacrifice twice. Either two different creatures must be sacrificed, or the creature with cumulative upkeep
   * must be sacrificed.
   *
   * 702.23b If a permanent has multiple instances of cumulative upkeep, each triggers separately. However, the age counters are not connected to any
   * particular ability; each cumulative upkeep ability will count the total number of age counters on the permanent at the time that ability resolves.
   *
   * Example: A creature has two instances of "Cumulative upkeep -- Pay 1 life." The creature has no age counters, and both cumulative upkeep abilities
   * trigger. When the first ability resolves, the controller adds a counter and then chooses to pay 1 life. When the second ability resolves, the controller
   * adds another counter and then chooses to pay an additional 2 life. */

  upkeep_trigger_ability(triggering_player, triggering_card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  /* A few cards (including Balduvian Fallen and Hibernation's End) detect a succesfully-paid cumulative upkeep by checking
	   * event==EVENT_UPKEEP_TRIGGER_ABILITY && in_play(player, card) after the cumulative_upkeep() call, despite this being an implementation detail; find and
	   * fix them if it changes .*/

	  int smp, smc;
	  if (triggering_card == card && triggering_player == player)
		smp = smc = -1;
	  else
		{
		  smp = triggering_player;
		  smc = triggering_card;
		}

	  add_counter(player, card, COUNTER_AGE);
	  int amount = count_counters(player, card, COUNTER_AGE);
	  if (!(DIALOG(triggering_player, triggering_card, EVENT_ACTIVATE,
				   DLG_FULLCARD(player, card), DLG_SMALLCARD(smp, smc), DLG_WHO_CHOOSES(player),
				   DLG_RANDOM, DLG_NO_STORAGE, DLG_AUTOCHOOSE_IF_1, DLG_NO_CANCEL,
				   "Pay cumulative upkeep", has_mana_multi(player, colorless*amount, black*amount, blue*amount, green*amount, red*amount, white*amount), 1,
				   "Decline", 1, 1) == 1
			&& charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, colorless*amount, black*amount, blue*amount,
												 green*amount, red*amount, white*amount)))
		{
		  cancel = 0;
		  kill_card(player, card, KILL_SACRIFICE);
		  return 1;
		}
	}

  return 0;
}

// Returns 1 if upkeep is unpaid.
int cumulative_upkeep(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white)
{
  return cumulative_upkeep_arbitrary(player, card, player, card, event, colorless, black, blue, green, red, white);
}

// As cumulative_upkeep(), but charges hybrid mana.
int cumulative_upkeep_hybrid(int player, int card, event_t event, int amt_colored, color_t first_color, color_t second_color, int amt_colorless)
{
  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  add_counter(player, card, COUNTER_AGE);
	  int amount = count_counters(player, card, COUNTER_AGE);
	  if (!(DIALOG(player, card, EVENT_ACTIVATE,
				   DLG_RANDOM, DLG_NO_STORAGE, DLG_AUTOCHOOSE_IF_1, DLG_NO_CANCEL,
				   "Pay cumulative upkeep", has_mana_hybrid(player, amt_colored*amount, first_color, second_color, amt_colorless*amount), 1,
				   "Decline", 1, 1) == 1
			&& charge_mana_hybrid_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, amt_colored*amount, first_color, second_color, amt_colorless*amount)))
		{
		  cancel = 0;
		  kill_card(player, card, KILL_SACRIFICE);
		  return 1;
		}
	}

  return 0;
}

/* As cumulative_upkeep(), but arbitrary functions instead of mana costs.  Both can_pay_fn() and pay_fn() should have signature int fn(int player, int card, int
 * number_of_age_counters), and return nonzero if upkeep (can be/is) paid. */
int cumulative_upkeep_general(int player, int card, event_t event, int (*can_pay_fn)(int, int, int), int (*pay_fn)(int, int, int))
{
  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  add_counter(player, card, COUNTER_AGE);
	  int amount = count_counters(player, card, COUNTER_AGE);
	  if (!(DIALOG(player, card, EVENT_ACTIVATE,
				   DLG_RANDOM, DLG_NO_STORAGE, DLG_AUTOCHOOSE_IF_1, DLG_NO_CANCEL,
				   "Pay cumulative upkeep", (!can_pay_fn || (can_pay_fn)(player, card, amount)), 1,
				   "Decline", 1, 1) == 1
			&& (!pay_fn || (pay_fn)(player, card, amount))))
		{
		  cancel = 0;
		  kill_card(player, card, KILL_SACRIFICE);
		  return 1;
		}
	}

  return 0;
}
