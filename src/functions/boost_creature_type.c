#include "manalink.h"

// Doesn't work for special keywords that need to be active other than during EVENT_ABILITIES.
void boost_subtype(int player, int card, event_t event, subtype_t subtype, int power, int toughness, int abilities, int sp_abilities, bct_t flags)
{
  // use subtype = -1 for boosting all the creatures
  if ((event == EVENT_POWER && power)
	  || (event == EVENT_TOUGHNESS && toughness)
	  || (event == EVENT_ABILITIES && (abilities || sp_abilities)))
	{
	  if (!in_play(player, card) || !in_play(affected_card_controller, affected_card))
		return;

	  if (!is_what(affected_card_controller, affected_card, TYPE_CREATURE))
		return;

	  if (is_humiliated(player, card))
		return;

	  if ((flags & BCT_CONTROLLER_ONLY) && affected_card_controller != player)
		return;

	  if ((flags & BCT_OPPONENT_ONLY) && affected_card_controller == player)
		return;

	  if (!(flags & BCT_INCLUDE_SELF) && affect_me(player, card))
		return;

	  if ((int)subtype != -1 && !has_creature_type(affected_card_controller, affected_card, subtype))
		return;

	  if (event == EVENT_POWER)
		event_result += power;
	  else if (event == EVENT_TOUGHNESS)
		event_result += toughness;
	  else if (event == EVENT_ABILITIES)
		{
		  event_result |= abilities;
		  if (sp_abilities)
			special_abilities(affected_card_controller, affected_card, event, sp_abilities, player, card);
		}
	}
}

int boost_creature_type(int player, int card, event_t event, subtype_t subtype, int power, int toughness, int abilities, bct_t flags)
{
  boost_subtype(player, card, event, subtype, power, toughness, abilities, 0, flags);
  return 0;
}

int boost_creature_by_color(int player, int card, event_t event, int clr, int power, int toughness, int abilities, bct_t flags){

	if (event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_ABILITIES){
		if ((flags & BCT_CONTROLLER_ONLY) && affected_card_controller != player)
			return 0;

		if ((flags & BCT_OPPONENT_ONLY) && affected_card_controller == player)
			return 0;

		if (!(flags & BCT_INCLUDE_SELF) && affect_me(player, card))
			return 0;

		if (!in_play(player, card) || !in_play(affected_card_controller, affected_card))
			return 0;

		if (!is_what(affected_card_controller, affected_card, TYPE_CREATURE))
			return 0;

		if (is_humiliated(player, card))
			return 0;

		clr &= COLOR_TEST_ANY_COLORED;
		if (!(flags & BCT_NO_SLEIGHT))
			clr = get_sleighted_color_test(player, card, clr);

		if (!(get_color(affected_card_controller, affected_card) & clr))
			return 0;

		if (event == EVENT_POWER)
			event_result += power;
		else if (event == EVENT_TOUGHNESS)
			event_result += toughness;
		else if(event == EVENT_ABILITIES)
			event_result |= abilities;
	}

	return 0;
}

