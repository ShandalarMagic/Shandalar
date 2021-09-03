#include "manalink.h"

static int action_on_card_impl(int player, int card, int p, int c, actions_t action, counter_t counter_type, int counter_num)
{
  switch (action)
	{
	  case ACT_KILL_DESTROY:
	  case ACT_KILL_BURY:
		if( ! check_for_special_ability(p, c, SP_KEYWORD_INDESTRUCTIBLE) ){
			kill_card(p, c, action);
			return 1;
		}
		return 0;
	  case ACT_KILL_SACRIFICE:
	  case ACT_KILL_REMOVE:
	  case ACT_KILL_STATE_BASED_ACTION:
		kill_card(p, c, action);
		return 1;

	  case ACT_BOUNCE:
		bounce_permanent(p, c);
		return 1;

	  case ACT_TAP:
		if (!is_tapped(p, c))
		  {
			tap_card(p, c);
			return 1;
		  }
		return 0;

	  case ACT_UNTAP:
		if (is_tapped(p, c))
		  {
			untap_card(p, c);
			return 1;
		  }
		return 0;

	  case ACT_RFG_UNTIL_EOT:
		remove_until_eot(player, card, p, c);
		return 1;
		break;

	  case ACT_PUT_ON_TOP:
		put_on_top_of_deck(p, c);
		return 1;

	  case ACT_PUT_ON_BOTTOM:
		put_on_bottom_of_deck(p, c);
		return 1;

	  case ACT_DISABLE_NONMANA_ACTIVATED_ABILITIES:
		disable_nonmana_activated_abilities(p, c, 1);
		return 1;

	  case ACT_ENABLE_NONMANA_ACTIVATED_ABILITIES:
		disable_nonmana_activated_abilities(p, c, 0);
		return 1;

	  case ACT_HUMILIATE:
		humiliate(player, card, p, c, 2);
		return 1;

	  case ACT_DE_HUMILIATE:
		humiliate(player, card, p, c, 0);
		return 1;

	  case ACT_MAKE_UNTARGETTABLE:
		state_untargettable(p, c, 1);
		return 1;

	  case ACT_REMOVE_UNTARGETTABLE:
		state_untargettable(p, c, 0);
		return 1;

	  case ACT_RESET_ADDED_SUBTYPE:
		reset_subtypes(p, c, 2);
		return 1;

	  case ACT_DISABLE_ALL_ACTIVATED_ABILITIES:
		disable_all_activated_abilities(p, c, 1);
		return 1;

	  case ACT_ENABLE_ALL_ACTIVATED_ABILITIES:
		disable_all_activated_abilities(p, c, 0);
		return 1;

	  case ACT_GET_COUNT:
		return 1;

	  case ACT_GAIN_CONTROL:
		gain_control(player, card, p, c);
		return 1;

	  case ACT_DOES_NOT_UNTAP:
		does_not_untap_effect(player, card, p, c, 0, 1);
		return 1;

	  case ACT_PHASE_OUT:
		phase_out(p, c);
		return 1;

	  case ACT_OF_TREASON:
		effect_act_of_treason(player, card, p, c);
		return 1;

	  case ACT_DETAIN:
		detain(player, card, p, c);
		return 1;

	  case ACT_PARAMETERIZED_ACTIONS_MASK:
		abort();

	  case ACT_ADD_COUNTERS_BASE:
		/* Number of counters added (or would have been added, if there wasn't a maximum of 255).  Doesn't account for Melira's Keepers, but then,
		 * nothing checks this return value anyway. */
		add_counters_predoubled(p, c, counter_type, counter_num);
		return counter_num;

	  case ACT_REMOVE_COUNTERS_BASE:
		;int curr = count_counters(p, c, counter_type);
		int amt_removed = MIN(counter_num, curr);
		remove_counters(p, c, counter_type, amt_removed);
		return amt_removed;
	}

  return 0;
}

static actions_t cook_action(int player, actions_t raw_action, counter_t* counter_type, int* counter_num)
{
  switch (raw_action & ACT_PARAMETERIZED_ACTIONS_MASK)
	{
	  case 0:
		return raw_action;

	  case ACT_ADD_COUNTERS_BASE:
		*counter_type = BYTE2(raw_action);
		*counter_num = SLOWORD(raw_action);
		if (*counter_num <= 0)
		  return -1;	// Don't bother continuing; trying to add 0 or a negative number

		*counter_num = get_updated_counters_number(player, -1, *counter_type, *counter_num);
		if (*counter_num <= 0)
		  return -2;	// Reduced to 0 by get_updated_counters_number(); but won't necessarily be true for the other player

		return ACT_ADD_COUNTERS_BASE;

	  case ACT_REMOVE_COUNTERS_BASE:
		*counter_type = BYTE2(raw_action);
		*counter_num = SLOWORD(raw_action);
		if (*counter_num <= 0)
		  return -1;	// Don't bother continuing; trying to remove 0 or a negative number

		return ACT_REMOVE_COUNTERS_BASE;

	  default:
		ASSERT(!"Unknown parameterized action");
		return -1;
	}
}

int action_on_card(int player, int card, int t_player, int t_card, actions_t raw_action)
{
  counter_t counter_type = COUNTER_invalid;
  int counter_num = 0;
  actions_t action = cook_action(t_player, raw_action, &counter_type, &counter_num);
  if (action == (actions_t)-1 || action == (actions_t)-2)
	return 0;

  return action_on_card_impl(player, card, t_player, t_card, action, counter_type, counter_num);
}

int action_on_target(int player, int card, unsigned int target_number, actions_t raw_action)
{
  card_instance_t* instance = get_card_instance(player, card);
  ASSERT((instance->targets[target_number].player & ~1) == 0);	// i.e., it's either 0 or 1
  return action_on_card(player, card, instance->targets[target_number].player, instance->targets[target_number].card, raw_action);
}

int new_manipulate_all(int player, int card, int t_player, test_definition_t* this_test, actions_t raw_action)
{
	int result = 0;
	int marked[2][100];
	int mc[2] = {0, 0};
	
	// First step: mark the cards which passes the test.
	int i, c;
	for (i = 0; i < 2; i++){
		int p = i == 0 ? current_turn : 1-current_turn;
		if (t_player == ANYBODY || p == t_player){
			counter_t counter_type = COUNTER_invalid;
			int counter_num = 0;
			actions_t action = cook_action(p, raw_action, &counter_type, &counter_num);
			if (action == (actions_t)-1)		// action invalid for both players
				return result;
			else if (action == (actions_t)-2)	// action invalid for this player, maybe not for the other
				continue;
		  
			for (c = active_cards_count[p] - 1; c >= 0; --c){
				if(((this_test->zone == TARGET_ZONE_IN_PLAY && in_play(p, c) && is_what(p, c, TYPE_PERMANENT) && !check_state(p, c, STATE_CANNOT_TARGET))
					|| (this_test->zone == TARGET_ZONE_HAND && in_hand(p, c)))
					&& new_make_test_in_play(p, c, -1, this_test)
					&& (this_test->not_me == 0 || !(player == p && c == card))
				  ){
					marked[p][mc[p]] = c;
					mc[p]++;
				}
			}
		}
	}

	// Second step: do the appropriate 'action' on marked cards, if legal.
	for (i = 0; i < 2; i++){
		int p = i == 0 ? current_turn : 1-current_turn;
		if (t_player == ANYBODY || p == t_player){
			counter_t counter_type = COUNTER_invalid;
			int counter_num = 0;
			actions_t action = cook_action(p, raw_action, &counter_type, &counter_num);
			if( action != (actions_t)-2 ){
				for(c=0; c<mc[p]; c++){
					result += action_on_card_impl(player, card, p, marked[p][c], action, counter_type, counter_num);
				}
			}
		}
	}

	return result;
}

int manipulate_all(int player, int card, int t_player, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5, actions_t action){

	test_definition_t this_test;
	new_default_test_definition(&this_test, type, "");
	this_test.type = type;
	this_test.type_flag = flag1;
	this_test.subtype = subtype;
	this_test.subtype_flag = flag2;
	this_test.color = color;
	this_test.color_flag = flag3;
	this_test.id = id;
	this_test.id_flag = flag4;
	this_test.cmc = cc;
	this_test.cmc_flag = flag5;
	return new_manipulate_all(player, card, t_player, &this_test, action);
}

int manipulate_type(int player, int card, int t_player, type_t type, actions_t action)
{
  test_definition_t this_test;
  new_default_test_definition(&this_test, type, "");
  return new_manipulate_all(player, card, t_player, &this_test, action);
}

int manipulate_auras_enchanting_target(int player, int card, int t_player, int t_card, test_definition_t *this_test, actions_t raw_action){
  // If the auras will be exiled and then returned to play attached to the original source under some condition, use "exile_permanent_and_auras_attached" instead
  int p, c;
  int result = 0;
  for (p = 0; p < 2; p++)
	{
	  counter_t counter_type = COUNTER_invalid;
	  int counter_num = 0;
	  actions_t action = cook_action(p, raw_action, &counter_type, &counter_num);
	  if (action == (actions_t)-1)		// action invalid for both players
		return result;
	  else if (action == (actions_t)-2)	// action invalid for this player, maybe not for the other
		continue;

	  card_instance_t* aura;

	  for (c = active_cards_count[p] - 1; c >= 0; --c)
		if ((aura = in_play(p, c))
			&& aura->damage_target_player == t_player && aura->damage_target_card == t_card
			&& is_what(p, c, TYPE_ENCHANTMENT) && has_subtype(p, c, SUBTYPE_AURA)
			&& (!this_test
				|| (new_make_test_in_play(p, c, -1, this_test)
					&& (this_test->not_me == 0 || !(player == p && c == card)))))
		  result += action_on_card_impl(player, card, p, c, action, counter_type, counter_num);
	}

  return result;
}

int new_damage_all(int player, int card, int targ_player, int dmg, int mode, test_definition_t *this_test){
	int score = this_test ? new_get_test_score(this_test) : 0;
	int result = 0;
	int i, n, count;
	for (n = 0; n <= 1; ++n){
		i = (n == 0 ? current_turn : 1-current_turn);
		if( targ_player == 2 || i == targ_player ){
			for( count = active_cards_count[i]-1; count >= 0; --count ){
				if( in_play(i, count) &&
					is_what(i, count, TYPE_CREATURE) &&
					( ( mode & NDA_ALL_CREATURES ) || !this_test || new_make_test_in_play(i, count, score, this_test) ) &&
					( card != count || player != i || !((mode & NDA_NOT_TO_ME) || (this_test && this_test->not_me == 1)) )
				  ){
					if( mode & NDA_EXILE_IF_FATALLY_DAMAGED ){
						exile_if_would_be_put_into_graveyard(player, card, i, count, 1);
					}
					int dmg_card = damage_creature(i, count, dmg, player, card);
					if ((mode & NDA_CANT_REGENERATE_IF_DEALT_DAMAGE_THIS_WAY) && dmg_card != -1){
						card_instance_t* damage = get_card_instance(player, dmg_card);
						damage->targets[3].card |= DMG_CANT_REGENERATE_IF_DEALT_DAMAGE_THIS_WAY;
					}
					result+=dmg;
				}
			}
			if( mode & NDA_PLAYER_TOO ){
				damage_player(i, dmg, player, card);
			}
		}
	}
	return result;
}

void damage_all(int p, int c, int controller, int amount_damage, int all, int player_too, int keyword, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){
	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.subtype = subtype;
	this_test.subtype_flag = flag2;
	this_test.color = color;
	this_test.color_flag = flag3;
	this_test.id = id;
	this_test.id_flag = flag4;
	this_test.cmc = cc;
	this_test.cmc_flag = flag5;
	this_test.keyword = keyword;
	this_test.keyword_flag = flag1;
	new_damage_all(p, c, controller, amount_damage, (1*all)+(2*player_too), &this_test);
}
