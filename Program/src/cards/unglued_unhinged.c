#include "manalink.h"


static const char* target_has_reminder_text(int who_chooses, int player, int card)
{
  card_ptr_t* cp = cards_ptr[get_id(player, card)];
  return cp->rules_text && strchr(cp->rules_text, '(') ? NULL : "no reminder text";
}

int card_duh(int player, int card, event_t event)
{
  /* Duh |B
   * Instant
   * Destroy target creature with reminder text. (Reminder text is any italicized text in parentheses that explains rules you already know.) */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int32_t)target_has_reminder_text;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_LITERAL_PROMPT, &td, "Select target creature with reminder text.", 1, NULL);
}

int card_jack_in_the_mox(int player, int card, event_t event){
	if( event == EVENT_ACTIVATE ){
		tap_card(player, card);
		int roll = internal_rand(6);
		if( roll == 0 ){
			lose_life(player, 5);
			kill_card(player, card, KILL_SACRIFICE);
			spell_fizzled = 1;
			return 0;
		}
		else if( roll == 1 ){
			produce_mana(player, COLOR_WHITE, 1);
		}
		else if( roll == 2 ){
			produce_mana(player, COLOR_BLUE, 1);
		}
		else if( roll == 3 ){
			produce_mana(player, COLOR_BLACK, 1);
		}
		else if( roll == 4 ){
			produce_mana(player, COLOR_RED, 1);
		}
		else if( roll == 5 ){
			produce_mana(player, COLOR_GREEN, 1);
		}
		spell_fizzled = 1;
		return 0;
	}
	int result = mana_producer(player, card, event);
	return result;
}

