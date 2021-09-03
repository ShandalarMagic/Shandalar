#include "manalink.h"

int tapped_for_mana[COLOR_ARTIFACT + 1] = { 0 };

char always_prompt_for_color = 0;

/* Color or colors activately chosen to tap for.  Won't be set if the color is overridden by e.g. Contamination.  Only set for some of the functions here;
 * inspect them before inspecting this. */
color_test_t chosen_colors = 0;

/* Forwards to either declare_mana_available() or declare_mana_available_hex(), depending on whether more than one bit is set in colors.
 * declare_mana_available() is always preferable to declare_mana_available_hex(), which has limited slots and is much slower in has_mana(), charge_mana(), etc.
 * Calling declare_mana_available() directly instead of this is preferable too when the combination of colors is known. */
void declare_mana_available_maybe_hex(int player, color_test_t colors, int amount)
{
  if (num_bits_set(colors) == 1)
	declare_mana_available(player, single_color_test_bit_to_color(colors), amount);
  else
	declare_mana_available_hex(player, colors, amount);
}


void declare_mana_available_any_combination_of_colors(int player, color_test_t available, int amount)
{
  /***************************************************************************************************************************************************************
   *    A simple declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, count) is equivalent to "This can produce [count] of any one color among             *
   * COLOR_TEST_ANY_COLORED", which of course isn't accurate here - available = COLOR_TEST_BLACK|COLOR_TEST_BLUE and amount = 2 should be able to pay for |U|B.  *
   *    On the other hand, we don't want to do for (i = 0; i < count; ++i) declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, 1) either, since           *
   * declare_mana_available_hex() can only store the result of 50 calls per player.  Multiple copies of a card like this can easily run out.                     *
   *    As a compromise between extremes, call declare_mana_available_hex() up to seven times (one for each bit set in available), or amount (if that's lower),  *
   * distributing the amounts of mana evenly.  This is accurate, but still uses more slots than I'm really comfortable with.  On the plus side, I'm not aware of *
   * any effects like this with more than five bits set, and choose_a_color_exe() can't deal with COLOR_TEST_ARTIFACT in any case.                               *
   *    If this still turns out to be a problem, expanding the number of slots available to declare_mana_available_hex() is feasible, though still a fairly      *
   * significant project.                                                                                                                                        *
   ***************************************************************************************************************************************************************/
  if (amount > 0)
	{
	  unsigned int i, num_bits = num_bits_set(available & (COLOR_TEST_ANY | COLOR_TEST_ARTIFACT));
	  for (i = 0; i < num_bits; ++i)
		if ((amount + i) >= num_bits)
		  declare_mana_available_hex(player, available, (amount + i) / num_bits);
	}
}

int mana_producer(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  switch (event)
	{
	  case EVENT_CHANGE_TYPE:
		if (affect_me(player, card) && instance->targets[12].card != -1 && instance->parent_card < 0)
		  event_result = instance->targets[12].card;
		break;

	  case EVENT_RESOLVE_SPELL:
		play_land_sound_effect(player, card);
		instance->targets[13].player = get_id(player, card);
		break;

	  case EVENT_CAST_SPELL:
		if (affect_me(player, card))
		  ai_modifier += 100;
		break;

	  case EVENT_CAN_ACTIVATE:
		return CAN_TAP_FOR_MANA(player, card);

	  case EVENT_ACTIVATE:
		if (can_produce_mana(player, card))
		  {
			if (check_special_flags2(player, card, SF2_CONTAMINATION)
				&& is_what(player, card, TYPE_LAND))
			  produce_mana_tapped(player, card, COLOR_BLACK, 1);
			else
			  {
				// I'd think this should be ->mana_color, but match what the exe does
				int colors = instance->card_color & (COLOR_TEST_ANY | COLOR_TEST_ARTIFACT);
				if (num_bits_set(colors) == 1)
				  produce_mana_tapped(player, card, single_color_test_bit_to_color(colors), 1);	// suppress the dialog announcing which color the AI chose
				else
				  produce_mana_tapped_all_one_color(player, card, instance->card_color, 1);
			  }
		  }
		else
		  cancel = 1;
		break;

	  case EVENT_COUNT_MANA:
		if (affect_me(player, card) && CAN_TAP_FOR_MANA(player, card)){
			if (check_special_flags2(player, card, SF2_CONTAMINATION) && is_what(player, card, TYPE_LAND)){
				declare_mana_available(player, COLOR_BLACK, 1);
			}
			else{
				int clr = instance->card_color;
				if( check_special_flags2(player, card, SF2_QUARUM_TRENCH_GNOMES) & (clr & COLOR_TEST_WHITE) ){
					clr &= ~COLOR_TEST_WHITE;
				}
				if( check_special_flags2(player, card, SF2_DEEP_WATER)){
					clr = COLOR_TEST_BLUE;
				}
				declare_mana_available_maybe_hex(player, clr & (COLOR_TEST_ANY | COLOR_TEST_ARTIFACT), 1);
			}
		}
		break;

	  default:;	// avoid warning
	}

  return 0;
}

int mana_producer_tapped(int player, int card, event_t event)
{
  comes_into_play_tapped(player, card, event);
  return mana_producer(player, card, event);
}

void produce_mana_multi(int player, int cless, int black, int blue, int green, int red, int white){

	produce_mana(player, COLOR_COLORLESS, cless);
	produce_mana(player, COLOR_BLACK, black);
	produce_mana(player, COLOR_BLUE, blue);
	produce_mana(player, COLOR_GREEN, green);
	produce_mana(player, COLOR_RED, red);
	produce_mana(player, COLOR_WHITE, white);
}

// Converts from a color_test_t with exactly one bit set to a color_t.
int single_color_test_bit_to_color(color_test_t col){
	return (col & COLOR_TEST_BLACK ? COLOR_BLACK
			: col & COLOR_TEST_BLUE ? COLOR_BLUE
			: col & COLOR_TEST_GREEN ? COLOR_GREEN
			: col & COLOR_TEST_RED ? COLOR_RED
			: col & COLOR_TEST_WHITE ? COLOR_WHITE
			: col & COLOR_TEST_ARTIFACT ? COLOR_ARTIFACT
			: COLOR_COLORLESS);
}

/* Produces the specified color/amount of mana (modified by Contamination if the card is a land) and taps card.  Only produces mana if card is < 0, though it'll
 * still set tapped_for_mana_color, so you probably don't want to do this. */
void produce_mana_tapped(int player, int card, color_t color, int amount){
	if (check_special_flags2(player, card, SF2_CONTAMINATION)
		&& card >= 0
		&& is_what(player, card, TYPE_LAND)){
		produce_mana(player, COLOR_BLACK, 1);
		tapped_for_mana_color = COLOR_BLACK;
	}
	else if (check_special_flags2(player, card, SF2_QUARUM_TRENCH_GNOMES)
		&& card >= 0 && color == COLOR_WHITE
		&& is_what(player, card, TYPE_LAND)){
		produce_mana(player, COLOR_COLORLESS, 1);
		tapped_for_mana_color = COLOR_COLORLESS;
	}
	else if (check_special_flags2(player, card, SF2_DEEP_WATER)
		&& card >= 0 && is_what(player, card, TYPE_LAND)){
		produce_mana(player, COLOR_BLUE, 1);
		tapped_for_mana_color = COLOR_BLUE;
	}
	else {
		// If trying to tap for 0 or less mana, still take notice of it (e.g., to trigger Overabundance), but don't add the mana
		if (amount > 0){
			produce_mana(player, color, amount);
		}
		color_t c;
		for (c = COLOR_COLORLESS; c <= COLOR_ARTIFACT; ++c){
			tapped_for_mana[c] = 0;
		}
		tapped_for_mana[color] = amount >= 0 ? amount : 0;
		tapped_for_mana_color = amount == 1 ? color : 0x100;	// Special value that tells mana flare to inspect tapped_for_mana[]
	}

	if (card >= 0){
		/* Not tap_card() - that 1. clears tapped_for_mana_color, and 2. sends an extra EVENT_TAP_CARD, which is also sent after activation (by the function
		 * that sends EVENT_ACTIVATE, in lieu of EVENT_PLAY_ABILITY if the activated card becomes tapped).  A good way to check 2 is with Psychic Venom. */
		get_card_instance(player, card)->state |= STATE_TAPPED;
	}
}

// As produce_mana_tapped(), but for two colors of mana.
void produce_mana_tapped2(int player, int card, color_t color1, int amount1, color_t color2, int amount2){
	if (color1 == color2){
		produce_mana_tapped(player, card, color1, amount1 + amount2);
		return;
	} else if (check_special_flags2(player, card, SF2_CONTAMINATION)
			   && card >= 0
			   && is_what(player, card, TYPE_LAND)){
		produce_mana(player, COLOR_BLACK, 1);
		tapped_for_mana_color = COLOR_BLACK;
	} else {
		// If trying to tap for 0 or less mana, still take notice of it (e.g., to trigger Overabundance), but don't add the mana
		if (amount1 > 0){
			produce_mana(player, color1, amount1);
		}
		if (amount2 > 0){
			produce_mana(player, color2, amount2);
		}
		color_t c;
		for (c = COLOR_COLORLESS; c <= COLOR_ARTIFACT; ++c){
			tapped_for_mana[c] = 0;
		}
		tapped_for_mana[color1] = amount1 >= 0 ? amount1 : 0;
		tapped_for_mana[color2] = amount2 >= 0 ? amount2 : 0;
		tapped_for_mana_color = 0x100;	// Special value that tells mana flare to inspect tapped_for_mana[]
	}

	if (card >= 0){
		/* Not tap_card() - that 1. clears tapped_for_mana_color, and 2. sends an extra EVENT_TAP_CARD, which is also sent after activation (by the function
		 * that sends EVENT_ACTIVATE, in lieu of EVENT_PLAY_ABILITY if the activated card becomes tapped).  A good way to check 2 is with Psychic Venom. */
		get_card_instance(player, card)->state |= STATE_TAPPED;
	}
}

// As produce_mana_tapped, but for arbitrary amounts of colored/unrestricted-colorless mana.
void produce_mana_tapped_multi(int player, int card, int colorless, int black, int blue, int green, int red, int white){
	if (check_special_flags2(player, card, SF2_CONTAMINATION)
		&& card >= 0
		&& is_what(player, card, TYPE_LAND)){
		produce_mana(player, COLOR_BLACK, 1);
		tapped_for_mana_color = COLOR_BLACK;
	} else {
		// If trying to tap for 0 or less mana, still take notice of it (e.g., to trigger Overabundance), but don't add the mana
		if (colorless > 0){
			produce_mana(player, COLOR_COLORLESS, colorless);
		}
		if (black > 0){
			produce_mana(player, COLOR_BLACK, black);
		}
		if (blue > 0){
			produce_mana(player, COLOR_BLUE, blue);
		}
		if (green > 0){
			produce_mana(player, COLOR_GREEN, green);
		}
		if (red > 0){
			produce_mana(player, COLOR_RED, red);
		}
		if (white > 0){
			produce_mana(player, COLOR_WHITE, white);
		}
		tapped_for_mana[COLOR_COLORLESS] = colorless >= 0 ? colorless : 0;
		tapped_for_mana[COLOR_BLACK] = black >= 0 ? black : 0;
		tapped_for_mana[COLOR_BLUE] = blue >= 0 ? blue : 0;
		tapped_for_mana[COLOR_GREEN] = green >= 0 ? green : 0;
		tapped_for_mana[COLOR_RED] = red >= 0 ? red : 0;
		tapped_for_mana[COLOR_WHITE] = white >= 0 ? white : 0;
		tapped_for_mana[COLOR_ARTIFACT] = 0;
		tapped_for_mana_color = 0x100;	// Special value that tells mana flare to inspect tapped_for_mana[]
	}

	if (card >= 0){
		/* Not tap_card() - that 1. clears tapped_for_mana_color, and 2. sends an extra EVENT_TAP_CARD, which is also sent after activation (by the function
		 * that sends EVENT_ACTIVATE, in lieu of EVENT_PLAY_ABILITY if the activated card becomes tapped).  A good way to check 2 is with Psychic Venom. */
		get_card_instance(player, card)->state |= STATE_TAPPED;
	}
}

static int choose_any_color_from_available(color_test_t available){
	int i;
	for (i = COLOR_COLORLESS; i <= COLOR_ARTIFACT; ++i){
		if (available & (1<<i)){
			return i;
		}
	}

	// no colored available
	return COLOR_COLORLESS;
}

// Chooses colors one at a time - call once for each num_mana, decreasing num_mana each time.
static int choose_combination_of_colors_of_mana_to_produce_onechoice(int* cols, int player, color_test_t available, int num_mana, const char* prompt){
	int i, highest = -1, num_available_needed = 0;
	color_test_t needed = 0;
	int charging[COLOR_ARTIFACT + 1] = { 0 };

	if (needed_mana_colors > 0){	// paying a cost
		// First, collect amounts needed
		for (i = COLOR_COLORLESS; i <= COLOR_ARTIFACT; ++i){
			if ((*charge_mana_addr_of_pay_mana_xbugrw)[i] == -1){
				charging[i] = 1000;
			} else {
				charging[i] = ((*charge_mana_addr_of_pay_mana_xbugrw)[i]	// amount not yet paid for
							   - cols[i]									// amount already in array
							   + (charge_mana_addr_of_pre_mana_pool ?		// this will be non-NULL only when handling EVENT_TAP_CARD from sources tapped within charge_mana()
								  (*charge_mana_addr_of_pre_mana_pool)[i] - mana_in_pool[player][i] : 0));	// amount added to mana pool by source just activated, but not yet applied to amount charged
			}
		}

		// Now, apply any excess colored mana to artifact and/or colorless cost if needed; otherwise, ignore excess.
		// If amount for any color is unsatisfied, record that.
		for (i = COLOR_COLORLESS; i <= COLOR_ARTIFACT; ++i){
			if (charging[i] < 0){
				if (charging[COLOR_ARTIFACT] > 0 && i != COLOR_ARTIFACT){
					int amt_to_apply = MIN(-charging[i], charging[COLOR_ARTIFACT]);
					charging[COLOR_ARTIFACT] -= amt_to_apply;
					charging[i] += amt_to_apply;
					if (charging[i] >= 0)
						continue;
					// otherwise, apply to colorless, too
				}
				if (charging[COLOR_COLORLESS] > 0 && i != COLOR_COLORLESS){
					int amt_to_apply = MIN(-charging[i], charging[COLOR_COLORLESS]);
					charging[COLOR_COLORLESS] -= amt_to_apply;
					charging[i] += amt_to_apply;
				}
			} else if (charging[i] > 0){
				needed |= 1<<i;

				if (i != COLOR_COLORLESS && i != COLOR_ARTIFACT && (available & (1<<i))){
					++num_available_needed;

					num_mana -= charging[i];

					if (highest == -1
						|| charging[i] > charging[highest]){
						highest = i;
					}
				}
			}
		}

		if (num_mana >= 0){	// Able to pay all the colored costs - return a color if there are any needed, or else anything if there's colorless needed, or else prompt
			if (highest >= 0){
				return highest;
			} else if (always_prompt_for_color){
				highest = choose_any_color_from_available(available);
				if ((charging[COLOR_COLORLESS] > 0 || charging[COLOR_ARTIFACT] > 0) && !always_prompt_for_color){
					return highest;
				} else {
					return choose_a_color_exe(player, prompt, 1, highest, available);
				}
			}
		}
	}

	if (num_available_needed == 1){	// paying a cost, and only one matching noncolorless is unsatisfied - autochoose that
		return highest;
	}

	if (highest == -1 && !always_prompt_for_color){	// no colored mana cost is unsatisfied, so just pick first available as default/ai choice
		highest = choose_any_color_from_available(available);

		if (needed_mana_colors > 0
			&& (charging[COLOR_COLORLESS] > 0 || charging[COLOR_ARTIFACT] > 0)){	// paying a cost with at least 1 colorless unsatisfied, so autochoose this
			return highest;
		}
	}

	// Either not paying a cost, or at least two needed colors are available.  AI chooses highest amount needed; player gets prompted.
	return choose_a_color_exe(player, prompt, 1, highest, available);
}

static void choose_combination_of_colors_of_mana_to_produce_into_arr(int* cols, int player, color_test_t available, int num_mana, const char* prompt){
	char buf[300];
	int num_left = num_mana;
	for (; num_left > 0; --num_left){
		if (prompt || player == 1 || ai_is_speculating == 1){
			buf[0] = 0;	// don't bother making a prompt that won't be seen
		} else {
			if (num_left == num_mana){
				if (num_left == 1){
					prompt = "What kind of mana?";
					buf[0] = 0;
				} else {
					sprintf(buf, "%d mana left", num_left);
				}
			} else {
				char* p = buf + sprintf(buf, "%d mana left (producing ", num_left);
				int i;
				if (cols[COLOR_COLORLESS] > 0){
					p += sprintf(p, "%d", cols[COLOR_COLORLESS]);
				}
				for (i = 0; i < cols[COLOR_WHITE]; ++i){
					*p++ = 'W';
				}
				for (i = 0; i < cols[COLOR_BLUE]; ++i){
					*p++ = 'U';
				}
				for (i = 0; i < cols[COLOR_BLACK]; ++i){
					*p++ = 'B';
				}
				for (i = 0; i < cols[COLOR_RED]; ++i){
					*p++ = 'R';
				}
				for (i = 0; i < cols[COLOR_GREEN]; ++i){
					*p++ = 'G';
				}
				for (i = 0; i < cols[COLOR_ARTIFACT]; ++i){
					*p++ = 'A';
				}
				*p++ = ')';
				*p = 0;
			}
		}
		int result = choose_combination_of_colors_of_mana_to_produce_onechoice(cols, player, available, num_left, prompt ? prompt : buf);
		if (cancel == 1 || result == -1){
			cancel = 1;
			return;
		}
		++cols[result];
		prompt = NULL;	// always generate a prompt for second and later choices
	}
}

/* As produce_mana_tapped(), but num_mana of any combination of the colors set in available.  If prompt is NULL, one will be constructed on-the-fly for each
 * choose_a_color dialog, showing amount of mana left and colors chosen so far.  Cancellable. */
int produce_mana_tapped_any_combination_of_colors(int player, int card, color_test_t available, int num_mana, const char* prompt){
	chosen_colors = 0;
	cancel = 0;
	if (check_special_flags2(player, card, SF2_CONTAMINATION)
		&& card >= 0
		&& is_what(player, card, TYPE_LAND)){
		produce_mana(player, COLOR_BLACK, 1);
		tapped_for_mana_color = COLOR_BLACK;
	} else {
		int cols[COLOR_ARTIFACT + 1] = { 0 };
		choose_combination_of_colors_of_mana_to_produce_into_arr(cols, player, available, num_mana, prompt);
		if (cancel == 1){
			return 0;
		} else {
			int i;
			for (i = COLOR_COLORLESS; i <= COLOR_ARTIFACT; ++i){
				if (cols[i] > 0){
					if( i == COLOR_WHITE && check_special_flags2(player, card, SF2_QUARUM_TRENCH_GNOMES) ){
						produce_mana(player, COLOR_COLORLESS, cols[COLOR_COLORLESS]);
						chosen_colors |= 1 << COLOR_COLORLESS;
					}
					else{
						produce_mana(player, i, cols[i]);
						chosen_colors |= 1 << i;
					}
				}
				tapped_for_mana[i] = MAX(0, cols[i]);
			}
			tapped_for_mana_color = 0x100;	// Special value that tells mana flare to inspect tapped_for_mana[]
		}
	}

	if (card >= 0){
		/* Not tap_card() - that 1. clears tapped_for_mana_color, and 2. sends an extra EVENT_TAP_CARD, which is also sent after activation (by the function
		 * that sends EVENT_ACTIVATE, in lieu of EVENT_PLAY_ABILITY if the activated card becomes tapped).  A good way to check 2 is with Psychic Venom. */
		get_card_instance(player, card)->state |= STATE_TAPPED;
	}

	return 1;
}

/* As produce_mana_tapped_any_combination_of_colors(), but doesn't tap card, account for Contamination, or set tapped_for_mana_color.  Cancellable. */
int produce_mana_any_combination_of_colors(int player, color_test_t available, int num_mana, const char* prompt){
	chosen_colors = 0;
	cancel = 0;
	int cols[COLOR_ARTIFACT + 1] = { 0 };
	choose_combination_of_colors_of_mana_to_produce_into_arr(cols, player, available, num_mana, prompt);
	if (cancel == 1){
		return 0;
	} else {
		int i;
		for (i = COLOR_COLORLESS; i <= COLOR_ARTIFACT; ++i){
			if (cols[i] > 0){
				produce_mana(player, i, cols[i]);
				chosen_colors |= 1 << i;
			}
		}
		return 1;
	}
}

static int choose_mana_all_one_color(int player, color_test_t available, int num_mana, const char* prompt, color_test_t default_color){
	if (num_mana <= 0){
		return choose_any_color_from_available(available);
	}

	int i, highest = -1, num_available_needed = 0;
	color_test_t needed = 0;
	int charging[COLOR_ARTIFACT + 1] = { 0 };

	if (needed_mana_colors > 0){	// paying a cost
		// First, collect amounts needed
		for (i = COLOR_COLORLESS; i <= COLOR_ARTIFACT; ++i){
			if ((*charge_mana_addr_of_pay_mana_xbugrw)[i] == -1){
				charging[i] = 1000;
			} else {
				charging[i] = ((*charge_mana_addr_of_pay_mana_xbugrw)[i]	// amount not yet paid for
							   + (charge_mana_addr_of_pre_mana_pool ?		// this will be non-NULL only when handling EVENT_TAP_CARD from sources tapped within charge_mana()
								  (*charge_mana_addr_of_pre_mana_pool)[i] - mana_in_pool[player][i] : 0));	// amount added to mana pool by source just activated, but not yet applied to amount charged
			}
		}

		// Now, apply any excess colored mana to artifact and/or colorless cost if needed; otherwise, ignore excess.
		// If amount for any color is unsatisfied, record that.
		for (i = COLOR_COLORLESS; i <= COLOR_ARTIFACT; ++i){
			if (charging[i] < 0){
				if (charging[COLOR_ARTIFACT] > 0 && i != COLOR_ARTIFACT){
					int amt_to_apply = MIN(-charging[i], charging[COLOR_ARTIFACT]);
					charging[COLOR_ARTIFACT] -= amt_to_apply;
					charging[i] += amt_to_apply;
					if (charging[i] >= 0)
						continue;
					// otherwise, apply to colorless, too
				}
				if (charging[COLOR_COLORLESS] > 0 && i != COLOR_COLORLESS){
					int amt_to_apply = MIN(-charging[i], charging[COLOR_COLORLESS]);
					charging[COLOR_COLORLESS] -= amt_to_apply;
					charging[i] += amt_to_apply;
				}
			} else if (charging[i] > 0){
				needed |= 1<<i;

				if (i != COLOR_COLORLESS && i != COLOR_ARTIFACT && (available & (1<<i))){
					++num_available_needed;

					num_mana -= charging[i];

					if (highest == -1
						|| charging[i] > charging[highest]){
						highest = i;
					}
				}
			}
		}

		if (num_available_needed == 1 && num_mana >= 0){	// Able to pay all the colored costs - return a color if there are any needed, or else anything if there's colorless needed, or else prompt
			if (highest >= 0){
				return highest;
			} else {
				if (available & default_color){
					highest = choose_any_color_from_available(available & default_color);
				} else {
					highest = choose_any_color_from_available(available);
				}
				if ((charging[COLOR_COLORLESS] > 0 || charging[COLOR_ARTIFACT] > 0) && !always_prompt_for_color){
					return highest;
				} else {
					return choose_a_color_exe(player, prompt, 1, highest, available);
				}
			}
		}
	}

	if (num_available_needed == 1){	// paying a cost, and only one matching noncolorless is unsatisfied - autochoose that
		return highest;
	}

	if (highest == -1 && !always_prompt_for_color){	// no colored mana cost is unsatisfied, so just pick first available as default/ai choice
		if (available & default_color){
			highest = choose_any_color_from_available(available & default_color);
		} else {
			highest = choose_any_color_from_available(available);
		}

		if (needed_mana_colors > 0
			&& (MAX(0, charging[COLOR_COLORLESS]) + MAX(0, charging[COLOR_ARTIFACT]) >= num_mana)){	// paying a cost with at least more colorless unsatisfied than this produces so autochoose this
			return highest;
		}
	}

	// Either not paying a cost, or at least two needed colors are available, or mana will be left over.  AI chooses highest amount needed; player gets prompted.
	return choose_a_color_exe(player, prompt, 1, highest, available);
}

// As produce_mana_tapped_all_one_color(), but if no specific color is needed and default_color is produceable, produce that.  Cancellable.
int produce_mana_tapped_all_one_color_with_default(int player, int card, color_test_t available, int num_mana, color_test_t default_color){
	chosen_colors = 0;
	cancel = 0;
	if (check_special_flags2(player, card, SF2_CONTAMINATION)
		&& card >= 0
		&& is_what(player, card, TYPE_LAND)){
		produce_mana(player, COLOR_BLACK, 1);
		tapped_for_mana_color = COLOR_BLACK;
	} else {
		int choice = choose_mana_all_one_color(player, available, num_mana, "What kind of mana?", default_color);
		if (cancel == 1 || choice == -1){
			cancel = 1;
			return 0;
		} else {
			// If trying to tap for 0 or less mana, still take notice of it (e.g., to trigger Overabundance), but don't add the mana
			if (num_mana > 0){
				produce_mana(player, choice, num_mana);
				chosen_colors |= 1 << choice;
			}

			color_t c;
			for (c = COLOR_COLORLESS; c <= COLOR_ARTIFACT; ++c){
				tapped_for_mana[c] = 0;
			}
			tapped_for_mana[choice] = num_mana;
			tapped_for_mana_color = num_mana == 1 ? choice : 0x100;	// Special value that tells mana flare to inspect tapped_for_mana[]
			if (player == AI && !(trace_mode & 2) && ai_is_speculating != 1){
				if (choice == COLOR_COLORLESS){
					choice = COLOR_ARTIFACT;	// "to produce colorless mana"
				}
				load_text(0, "FELLWAR_STONE");
				do_dialog(player, player, card, -1, -1, text_lines[choice >= 1 && choice <= 6 ? choice : 6], 0);
			}
		}
	}

	if (card >= 0){
		/* Not tap_card() - that 1. clears tapped_for_mana_color, and 2. sends an extra EVENT_TAP_CARD, which is also sent after activation (by the function
		 * that sends EVENT_ACTIVATE, in lieu of EVENT_PLAY_ABILITY if the activated card becomes tapped).  A good way to check 2 is with Psychic Venom. */
		get_card_instance(player, card)->state |= STATE_TAPPED;
	}
	return 1;
}

// As produce_mana_tapped(), but num_mana all of the same color (chosen from those set in available).  Cancellable.
int produce_mana_tapped_all_one_color(int player, int card, color_test_t available, int num_mana)
{
  return produce_mana_tapped_all_one_color_with_default(player, card, available, num_mana, COLOR_TEST_COLORLESS);
}

// As produce_mana_tapped_x_all_one_color(), but doesn't tap card, account for Contamination, or set tapped_for_mana_color.  Cancellable.
int produce_mana_all_one_color(int player, color_test_t available, int num_mana){
	chosen_colors = 0;
	if (num_mana > 0){
		int choice = choose_mana_all_one_color(player, available, num_mana, "What kind of mana?", COLOR_TEST_COLORLESS);
		if (cancel == 1 || choice == -1){
			cancel = 1;
			return 0;
		} else {
			produce_mana(player, choice, num_mana);
			chosen_colors |= 1 << choice;
		}
	}
	return 1;
}

/* Produces amount of mana in any combination of the types put into tapped_for_mana_color/tapped_for_mana[].  This is the back-end function of Mana Flare and
 * similar effects. */
void produce_mana_of_any_type_tapped_for(int player, int card, int amount){
	chosen_colors = 0;
	switch (tapped_for_mana_color){
		case 0x100:{	// Tapped for zero mana, or more than one (not necessarily of more than one color).
						// Nothing in the exe except Mana Flare compares tapped_for_mana_color to anything but -1, so using this value is safe
			color_test_t colors = 0;
			color_t col, first_found = 0x100;
			int num_found = 0;
			for (col = COLOR_COLORLESS; col < COLOR_ARTIFACT; ++col){
				if (tapped_for_mana[col] > 0){
					++num_found;
					colors |= (1 << col);
					if (first_found == 0x100)
						first_found = col;
				}
			}

			// 9/16/2007 ruling on Mana Flare: Does not copy any restrictions on the mana, such as with Mishra's Workshop or Pillar of the Paruns.
			if (tapped_for_mana[COLOR_ARTIFACT] > 0 && !(colors & COLOR_TEST_COLORLESS)){
				++num_found;
				colors |= COLOR_TEST_COLORLESS;
				if (first_found == 0x100)
					first_found = COLOR_COLORLESS;
			}

			if (num_found == 0){		// Tapped for mana, but produced 0
				break;
			} else if (num_found == 1){	// Tapped for one color of mana
				produce_mana(affected_card_controller, first_found, amount);
				chosen_colors |= 1 << first_found;
			} else {					// Tapped for more than one color of mana
				char prompt[300];
				sprintf(prompt, "%s (%s): What kind of mana?",
						cards_ptr[cards_at_7c7000[get_card_instance(player, card)->internal_card_id]->id]->full_name,
						cards_ptr[cards_at_7c7000[get_card_instance(affected_card_controller, affected_card)->internal_card_id]->id]->full_name);

				produce_mana_any_combination_of_colors(player, colors, amount, prompt);
				if (cancel == 1){
					// Too late to cancel tapping the card, so have to produce the mana; but let cancel propagate through, so as to cancel further autotapping
					produce_mana(affected_card_controller, first_found, amount);
					chosen_colors |= 1 << first_found;
				}
			}
			break;
		}

		case COLOR_ARTIFACT:
			// 9/16/2007 ruling on Mana Flare: Does not copy any restrictions on the mana, such as with Mishra's Workshop or Pillar of the Paruns.
			produce_mana(affected_card_controller, COLOR_COLORLESS, amount);
			chosen_colors = 1 << COLOR_COLORLESS;
			break;

		case COLOR_COLORLESS:
		case COLOR_BLACK:
		case COLOR_BLUE:
		case COLOR_GREEN:
		case COLOR_RED:
		case COLOR_WHITE:
			// Produced exactly one mana
			produce_mana(affected_card_controller, tapped_for_mana_color, amount);
			chosen_colors = 1 << tapped_for_mana_color;
			break;
	}
}

/* Produces amount of color mana when allowed_player taps a card of type and subtype for mana.  allowed_player may be 2 to work for both players; as usual,
 * subtype may be 0 or -1 to ignore subtype.  This is the back-end function of High Tide and similar effects. */
void produce_mana_when_subtype_is_tapped(int allowed_player, event_t event, int type, int subtype, color_t color, int amount){
	if (event == EVENT_TAP_CARD && tapped_for_mana_color >= 0
		&& (allowed_player == 2 || allowed_player == affected_card_controller)
		&& ((type & TYPE_PERMANENT) || is_what(affected_card_controller, affected_card, type))
		&& (subtype <= 0 || has_subtype(affected_card_controller, affected_card, subtype))){
		produce_mana(affected_card_controller, color, amount);
	}

	if (event == EVENT_COUNT_MANA
		&& (allowed_player == 2 || allowed_player == affected_card_controller)
		&& !is_tapped(affected_card_controller, affected_card) && !is_animated_and_sick(affected_card_controller, affected_card)
		&& can_produce_mana(affected_card_controller, affected_card)
		&& ((type & TYPE_PERMANENT) || is_what(affected_card_controller, affected_card, type))
		&& (subtype <= 0 || has_subtype(affected_card_controller, affected_card, subtype))){
		declare_mana_available(affected_card_controller, color, amount);
	}
}

typedef enum {
	PM_FIXED,
	PM_ALL_ONE,
	PM_ANY_COMBO
} produce_mana_t;

static int wga_backend(int player, int card, event_t event, int subtype, produce_mana_t what_to_produce, int colors, int amount){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	if (subtype > 0){
		td.required_subtype = subtype;
	}

	card_instance_t *instance = get_card_instance(player, card);

	switch (event){
		case EVENT_CAN_CAST:
			return can_target(&td);

		case EVENT_CAST_SPELL:
			if (affect_me(player, card)){
				char buffer[100];
				scnprintf(buffer, 100, "Select target %s.", subtype > 0 ? get_hacked_land_text(player, card, "%s", subtype) : "land");
				if (new_pick_target(&td, buffer, 0, 1 | GS_LITERAL_PROMPT)){
					if (player == AI && instance->targets[0].player == player){
						ai_modifier += 30;
						// Wild Growth: ai_modifier += 12 * total_power_of_creatures_by_color[player][COLOR_GREEN], which doesn't make much sense.
					}
				} else {
					cancel = 1;
				}
			}
			break;

		case EVENT_RESOLVE_SPELL:
			if (valid_target(&td)){
				instance->damage_target_player = instance->targets[0].player;
				instance->damage_target_card = instance->targets[0].card;
			} else {
				kill_card(player, card, KILL_BURY);
				spell_fizzled = 1;
			}
			instance->number_of_targets = 0;
			break;

		case EVENT_TAP_CARD:
			if (instance->damage_target_card != -1 && affect_me(instance->damage_target_player, instance->damage_target_card)
				&& tapped_for_mana_color >= 0 && amount > 0){
				switch (what_to_produce){
					case PM_FIXED:		produce_mana(instance->damage_target_player, colors, amount);									break;
					case PM_ALL_ONE:	produce_mana_all_one_color(instance->damage_target_player, colors, amount);						break;
					case PM_ANY_COMBO:	produce_mana_any_combination_of_colors(instance->damage_target_player, colors, amount, NULL);	break;
				}
			}
			break;

		case EVENT_COUNT_MANA:
			if (instance->damage_target_card != -1 && affect_me(instance->damage_target_player, instance->damage_target_card)
				&& !is_tapped(instance->damage_target_player, instance->damage_target_card)
				&& !is_animated_and_sick(instance->damage_target_player, instance->damage_target_card)
				&& can_produce_mana(instance->damage_target_player, instance->damage_target_card)
				&& amount > 0){
				switch (what_to_produce){
					case PM_FIXED:		declare_mana_available(instance->damage_target_player, colors, amount);		break;
					case PM_ALL_ONE:	declare_mana_available_maybe_hex(instance->damage_target_player, colors, amount);	break;
					case PM_ANY_COMBO:	declare_mana_available_any_combination_of_colors(instance->damage_target_player, colors, amount);	break;
				}
			}
			break;

		default: break;
	}
	return 0;
}

// An enchant land that produces a specific amount of color of mana when enchanted land is tapped for mana.
int wild_growth_aura(int player, int card, event_t event, int subtype, color_t color, int amount){
	return wga_backend(player, card, event, subtype, PM_ALL_ONE, 1 << color, amount);
}

// As wild_growth_aura(), but produces num_mana of any one color set in available.
int wild_growth_aura_all_one_color(int player, int card, event_t event, int subtype, color_test_t available, int num_mana){
	return wga_backend(player, card, event, subtype, PM_ALL_ONE, available, num_mana);
}

// As wild_growth_aura(), but produces num_mana of any combination of colors set in available.
int wild_growth_aura_any_combination_of_colors(int player, int card, event_t event, int subtype, color_test_t available, int num_mana){
	return wga_backend(player, card, event, subtype, PM_ANY_COMBO, available, num_mana);
}

/* A generic creature that taps to produce mana, all of a single constant color.  If reluctance_to_fight is nonzero, it will tend not to attack or block if its
 * controller controls few lands of matching color.  Higher numbers are more reluctant; Llanowar Elves and Birds of Paradise use 24. */
int mana_producing_creature(int player, int card, event_t event, int reluctance_to_fight, color_t color, int amount){
	if (event == EVENT_CAN_ACTIVATE){
		return !is_tapped(player, card) && !is_sick(player, card) && can_produce_mana(player, card) && (player != AI || amount > 0);
	} else if (event == EVENT_ACTIVATE){
		produce_mana_tapped(player, card, color, amount);
	} else if (event == EVENT_COUNT_MANA){
		if (affect_me(player, card) && mana_producing_creature(player, card, EVENT_CAN_ACTIVATE, reluctance_to_fight, color, amount)){
			declare_mana_available(player, color, amount);
		}
	} else if (reluctance_to_fight && (event == EVENT_ATTACK_RATING || event == EVENT_BLOCK_RATING) && affect_me(player, card)){
		if (color == COLOR_COLORLESS || color == COLOR_ARTIFACT){
			color = COLOR_ANY;	// count all lands, not just colorless ones
		}
		if (event == EVENT_ATTACK_RATING){
			ai_defensive_modifier += reluctance_to_fight / (landsofcolor_controlled[player][color] + 2);
		} else {	// EVENT_BLOCK_RATING
			ai_defensive_modifier -= 4*reluctance_to_fight / (landsofcolor_controlled[player][color] + 2);
		}
	}
	return 0;
}

// As mana_producing_creature(), but produces multiple colors.  reluctance_to_fight goes by total number of lands.
int mana_producing_creature_multi(int player, int card, event_t event, int reluctance_to_fight, int colorless, int black, int blue, int green, int red, int white){
	if (event == EVENT_CAN_ACTIVATE){
		return !is_tapped(player, card) && !is_sick(player, card) && can_produce_mana(player, card)
			&& (player != AI || colorless > 0 || black > 0 || blue > 0 || green > 0 || red > 0 || white > 0);
	} else if (event == EVENT_ACTIVATE){
		produce_mana_tapped_multi(player, card, colorless, black, blue, green, red, white);
	} else if (event == EVENT_COUNT_MANA){
		if (affect_me(player, card) && mana_producing_creature_multi(player, card, EVENT_CAN_ACTIVATE, reluctance_to_fight, colorless, black, blue, green, red, white)){
			declare_mana_available(player, COLOR_COLORLESS, colorless);
			declare_mana_available(player, COLOR_BLACK, black);
			declare_mana_available(player, COLOR_BLUE, blue);
			declare_mana_available(player, COLOR_GREEN, green);
			declare_mana_available(player, COLOR_RED, red);
			declare_mana_available(player, COLOR_WHITE, white);
		}
	} else if (reluctance_to_fight && (event == EVENT_ATTACK_RATING || event == EVENT_BLOCK_RATING) && affect_me(player, card)){
		if (event == EVENT_ATTACK_RATING){
			ai_defensive_modifier += reluctance_to_fight / (landsofcolor_controlled[player][COLOR_ANY] + 2);
		} else {	// EVENT_BLOCK_RATING
			ai_defensive_modifier -= 4*reluctance_to_fight / (landsofcolor_controlled[player][COLOR_ANY] + 2);
		}
	}
	return 0;
}

// As mana_producing_creature(), but produces num_mana all of a single color in available.  reluctance_to_fight goes by total number of lands.
int mana_producing_creature_all_one_color(int player, int card, event_t event, int reluctance_to_fight, color_test_t available, int num_mana){
	if (event == EVENT_CAN_ACTIVATE){
		return !is_tapped(player, card) && !is_sick(player, card) && can_produce_mana(player, card) && (player != AI || num_mana > 0);
	} else if (event == EVENT_ACTIVATE){
		produce_mana_tapped_all_one_color(player, card, available, num_mana);
	} else if (event == EVENT_COUNT_MANA){
		if (affect_me(player, card) && mana_producing_creature_all_one_color(player, card, EVENT_CAN_ACTIVATE, reluctance_to_fight, available, num_mana)){
			declare_mana_available_maybe_hex(player, available, num_mana);
		}
	} else if (reluctance_to_fight && (event == EVENT_ATTACK_RATING || event == EVENT_BLOCK_RATING) && affect_me(player, card)){
		if (event == EVENT_ATTACK_RATING){
			ai_defensive_modifier += reluctance_to_fight / (landsofcolor_controlled[player][COLOR_ANY] + 2);
		} else {	// EVENT_BLOCK_RATING
			ai_defensive_modifier -= 4*reluctance_to_fight / (landsofcolor_controlled[player][COLOR_ANY] + 2);
		}
	}
	return 0;
}

// A generic artifact that taps to produce mana of its mana_color.  If sac is nonzero, then sacrifices itself when activated.
int artifact_mana_all_one_color(int player, int card, event_t event, int amount, int sac){
	if (event == EVENT_CAN_ACTIVATE){
		return !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card)
			&& (!sac || can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0));
	} else if (event == EVENT_ACTIVATE){
		ai_modifier -= sac ? 36 : 12;

		produce_mana_tapped_all_one_color(player, card, get_card_instance(player, card)->mana_color, amount);
		if (cancel != 1 && sac){
			kill_card(player, card, KILL_SACRIFICE);
		}
	} else if (event == EVENT_COUNT_MANA){
		if (affect_me(player, card) && artifact_mana_all_one_color(player, card, EVENT_CAN_ACTIVATE, amount, sac)){
			declare_mana_available_maybe_hex(player, get_card_instance(player, card)->mana_color, amount);
		}
	}
	return 0;
}

int two_mana_land(int player, int card, event_t event, color_t color1, color_t color2){
	// Original asm at 0x120058D, for what it's worth.  It includes responses to EVENT_ATTACK_RATING and EVENT_BLOCK_RATING copied from Llanowar Elves.

	switch (event){
		case EVENT_RESOLVE_SPELL:
			play_land_sound_effect(player, card);
			return 0;

		case EVENT_CAN_ACTIVATE:
			return !is_tapped(player, card) && !is_animated_and_sick(player, card) && can_produce_mana(player, card);

		case EVENT_ACTIVATE:{
			/* I'm not sure if these are truly necessary, though it's in the ManalinkEx.dll asm for this function and in most (all?) of the card functions for
			 * mana producers in the exe.  On the other hand, its omission from most C-coded mana producers hasn't resulted in any obvious bugs.  It's possible
			 * that it impacts the AI, though. */
			declare_mana_available(player, color2, -1);
			declare_mana_available(player, color1, -1);

			produce_mana_tapped2(player, card, color1, 1, color2, 1);
			return 0;
		}

		case EVENT_COUNT_MANA:{
			if (!affect_me(player, card) || !two_mana_land(player, card, EVENT_CAN_ACTIVATE, color1, color2)){
				return 0;
			}

			if (check_special_flags2(player, card, SF2_CONTAMINATION)){
				declare_mana_available(player, COLOR_BLACK, 1);
			} else {
				declare_mana_available(player, color2, 1);
				declare_mana_available(player, color1, 1);
			}
			return 0;
		}

		default:
			return 0;
	}
}

int sac_land(int player, int card, event_t event, int base_color, int color1, int color2){
	if( event == EVENT_RESOLVE_SPELL ){
		get_card_instance(player, card)->info_slot = (1<<base_color) | (1<<color1) | (1<<color2);
	}
	else if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( !(player == AI || ai_is_speculating == 1 || ldoubleclicked) ){
			choice = do_dialog(player, player, card, -1, -1, " 1 Mana Ability\n Sac Ability\n Cancel", 0);
		}
		if( choice == 1 ){
			produce_mana_tapped2(player, card, color1, 1, color2, 1);
			kill_card(player, card, KILL_SACRIFICE);
		}
		else if(choice == 2){
			spell_fizzled = 1;
		}
		else{
			return mana_producer_fixed(player, card, event, base_color);
		}
		return 0;
	}

	return mana_producer_fixed(player, card, event, base_color);
}

int sac_land_tapped(int player, int card, event_t event, int base_color, int color1, int color2)
{
  comes_into_play_tapped(player, card, event);
  return sac_land(player, card, event, base_color, color1, color2);
}

/* Implements "X you control have 'T: Add Y to your mana pool.'", where Y is a specific color and amount of mana.  Set subtype to -1 if just checking for a
 * type, as usual. (Example: Sachi, Daughter of Seshiro - X is Shamans, and Y is |G|G.) */
// A reasonable next step is to implement permanents_anyone_controls_can_tap_for_mana(), using a scheme similar to Iff-Biff Efreet.
int permanents_you_control_can_tap_for_mana(int player, int card, event_t event, type_t type, int32_t subtype, color_t color, int amount){
	if (event == EVENT_CAN_ACTIVATE){
		return check_for_untapped_nonsick_subtype(player, type, subtype);
	}

	if (event == EVENT_ACTIVATE ){

		target_definition_t td;
		default_target_definition(player, card, &td, type);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_state = TARGET_STATE_TAPPED | TARGET_STATE_SUMMONING_SICK;
		td.illegal_abilities = 0;
		if (subtype > 0){
			td.required_subtype = subtype;
		}

		if( pick_target(&td, type == TYPE_CREATURE ? "TARGET_CREATURE" : type == TYPE_LAND ? "TARGET_LAND" : "TARGET_PERMANENT") ){
			card_instance_t* instance = get_card_instance(player, card);
			instance->number_of_targets = 1;
			produce_mana_tapped(instance->targets[0].player, instance->targets[0].card, color, amount);
			if (!(instance->targets[0].player == player && instance->targets[0].card == card)){
				/* tap_card() would do this, but clear tapped_for_mana_color first.
				 * And if the card is tapping itself, the event will be dispatched later anyway, so don't double up here. */
				dispatch_event(instance->targets[0].player, instance->targets[0].card, EVENT_TAP_CARD);
			}
		} else {
			cancel = 1;
		}
	}

	if (event == EVENT_COUNT_MANA && affect_me(player, card)){
		// This is inaccurate when one of the tappable permanents can itself produce mana, since it will declare mana too.  There's no good solution.
		int count = amount * count_untapped_nonsick_subtype(player, type, subtype);
		if (count > 0){
			declare_mana_available(player, color, count);
		}
	}

	return 0;
}

// As permanents_you_control_can_tap_for_mana(), but choice of colors set in available.  (Example: Overlaid Terrain - Lands, and 2 of COLOR_TEST_ANY_COLORED.)
int permanents_you_control_can_tap_for_mana_all_one_color(int player, int card, event_t event, type_t type, int32_t subtype, color_test_t available, int num_mana){
	if ((available & (COLOR_TEST_ANY | COLOR_TEST_ARTIFACT)) == 0){
		// Sanity check - no valid mana bits set
		return 0;
	}
	// Not 'can_use_activated_abilities(player, card)' as this is usually a continuous ability, and cannot be disabled by cards like Pithing Needle and such
	if (event == EVENT_CAN_ACTIVATE && ! is_humiliated(player, card) ){
		return check_for_untapped_nonsick_subtype(player, type, subtype);
	}

	if (event == EVENT_ACTIVATE ){

		target_definition_t td;
		default_target_definition(player, card, &td, type);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_state = TARGET_STATE_TAPPED | TARGET_STATE_SUMMONING_SICK;
		td.illegal_abilities = 0;
		if (subtype > 0){
			td.required_subtype = subtype;
		}

		if( pick_target(&td, type == TYPE_CREATURE ? "TARGET_CREATURE" : type == TYPE_LAND ? "TARGET_LAND" : "TARGET_PERMANENT") ){
			card_instance_t* instance = get_card_instance(player, card);
			instance->number_of_targets = 1;
			produce_mana_tapped_all_one_color(instance->targets[0].player, instance->targets[0].card, available, num_mana);
			if (!(instance->targets[0].player == player && instance->targets[0].card == card)){
				/* tap_card() would do this, but clear tapped_for_mana_color first.
				 * And if the card is tapping itself, the event will be dispatched later anyway, so don't double up here. */
				dispatch_event(instance->targets[0].player, instance->targets[0].card, EVENT_TAP_CARD);
			}
		} else {
			cancel = 1;
		}
	}

	if (event == EVENT_COUNT_MANA && affect_me(player, card)){
		/* As with permanents_you_control_can_tap_for_mana() above, this is inaccurate when one of the tappable permanents can itself produce mana, since it
		 * will declare mana too.  There's no good solution. */
		int count = count_untapped_nonsick_subtype(player, type, subtype) * num_mana;
		declare_mana_available_any_combination_of_colors(player, available, count);
	}

	return 0;
}

/* Implements "Tap an untapped X you control: Add Y to your mana pool.", where Y is a specific color and amount of mana. (Example: Seton, Krosan Protector - X
 * is Druid and Y is |G.)  Differs from permanents_you_control_can_tap_for_mana() in that 1. this is correct, not an approximation; and 2. it can tap
 * summoning-sick creatures. */
int tap_a_permanent_you_control_for_mana(int player, int card, event_t event, type_t type, int32_t subtype, color_t color, int amount){
	if (event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)){
		return count_untapped_subtype(player, type, subtype) > 0;
	}

	if (event == EVENT_ACTIVATE ){

		target_definition_t td;
		default_target_definition(player, card, &td, type);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_state = TARGET_STATE_TAPPED;
		td.illegal_abilities = 0;
		if (subtype > 0){
			td.required_subtype = subtype;
		}

		if( pick_target(&td, type == TYPE_CREATURE ? "TARGET_CREATURE" : type == TYPE_LAND ? "TARGET_LAND" : "TARGET_PERMANENT") ){
			card_instance_t* instance = get_card_instance(player, card);
			instance->number_of_targets = 1;
			// 106.10. To "tap a permanent for mana" is to activate a mana ability of that permanent that includes the {T} symbol in its activation cost. See rule 605, "Mana Abilities."
			produce_mana(player, color, amount);
			tapped_for_mana_color = -2;	// Still a mana ability, so still can't be responded to, though
			if (instance->targets[0].player == player && instance->targets[0].card == card){
				// EVENT_TAP_CARD will be dispatched later
				instance->state |= STATE_TAPPED;
			} 
			else {
				get_card_instance(instance->targets[0].player, instance->targets[0].card)->state |= STATE_TAPPED;
				dispatch_event(instance->targets[0].player, instance->targets[0].card, EVENT_TAP_CARD);
			}
		} 
		else {
			cancel = 1;
		}
	}

	if (event == EVENT_COUNT_MANA && affect_me(player, card)){
		// This is inaccurate when one of the tappable permanents can itself produce mana, since it will declare mana too.  There's no good solution.
		int count = amount * count_untapped_subtype(player, type, subtype);
		if (count > 0){
			declare_mana_available(player, color, count);
		}
	}

	return 0;
}

// This will only be correct for lands that are in play; info_slot isn't generally set while a land's still in hand
int get_colors_of_mana_land_could_produce_ignoring_costs(int player, int card){
	card_data_t* card_d = get_card_data(player, card);
	if (!(card_d->extra_ability & EA_MANA_SOURCE)){
		return 0;
	}
	int id = card_d->id;
	if (id == CARD_ID_VIVID_CREEK || id == CARD_ID_VIVID_CRAG || id == CARD_ID_VIVID_MEADOW || id == CARD_ID_VIVID_MARSH || id == CARD_ID_VIVID_GROVE
		|| id == CARD_ID_TENDO_ICE_BRIDGE || id == CARD_ID_MIRRODINS_CORE || id == CARD_ID_SHIMMERING_GROTTO || id == CARD_ID_CAVERN_OF_SOULS || id == CARD_ID_UNKNOWN_SHORES){
		/* Per 5/1/2008 Reflecting Pool ruling: Reflecting Pool checks the effects of all mana-producing abilities of lands you control, but it doesn't check
		 * their costs. For example, Vivid Crag says "{T}, Remove a charge counter from Vivid Crag: Add one mana of any color to your mana pool." If you control
		 * Vivid Crag and Reflecting Pool, you can tap Reflecting Pool for any color of mana. It doesn't matter whether Vivid Crag has a charge counter on it,
		 * and it doesn't matter whether it's untapped. */
		return COLOR_TEST_ANY_COLORED;
	} else if (id == CARD_ID_COMMAND_TOWER || id == CARD_ID_ANCIENT_SPRING || id == CARD_ID_GEOTHERMAL_CREVICE || id == CARD_ID_IRRIGATION_DITCH
			   || id == CARD_ID_SULFUR_VENT || id == CARD_ID_TINDER_FARM || id == CARD_ID_GEMSTONE_CAVERNS || id == CARD_ID_NIMBUS_MAZE
			   || id == CARD_ID_RIVER_OF_TEARS || id == CARD_ID_TAINTED_FIELD || id == CARD_ID_TAINTED_ISLE || id == CARD_ID_TAINTED_PEAK
			   || id == CARD_ID_TAINTED_WOOD || id == CARD_ID_GEM_BAZAAR || id == CARD_ID_NYKTHOS_SHRINE_TO_NYX || id == CARD_ID_URBORG_TOMB_OF_YAWGMOTH ||
			   id == CARD_ID_OPAL_PALACE || id == CARD_ID_COMMANDERS_SPHERE
	  ){
		card_instance_t* instance = get_card_instance(player, card);
		return instance->info_slot;
	} else if (id == CARD_ID_REFLECTING_POOL || id == CARD_ID_EXOTIC_ORCHARD){
		/* We won't get here for Reflecting Pool/Fellwar Stone/Sylvok Explorer/Exotic Orchard, nor for the displayed mana color for Squandered Resources,
		 * but will when activating Squandered Resources */
		card_instance_t* instance = get_card_instance(player, card);
		return instance->mana_color;
	} else if (id == CARD_ID_CAVERN_OF_SOULS || id == CARD_ID_SPRINGJACK_PASTURE){
		return COLOR_TEST_ANY;
	} else {	// Explicitly including the filter lands and Homelands triple lands, again per the 5/1/2008 Reflecting Pool ruling
		return card_d->color;
	}
}

int get_color_of_mana_produced_by_id(int csvid, int info_slot, int player){
	/* Used to determine card_instance_t::mana_color.
	 *
	 * info_slot will be -1 when this is called during card creation.  The actual card_instance_t's info_slot will be 0, however, and if it happens to be a creature,
	 * it'll get EVENT_CHANGE_TYPE messages.  Otherwise, the special cases below do not apply during card creation, only in response to EVENT_CHANGE_TYPE.
	 *
	 * Anything that returns -1 from here gets their card_instance_t::mana_color set to card_ptr_t::mana_source_colors.
	 *
	 * Most of what's left are cards whose produced mana colors vary. */
	switch (csvid){
		case CARD_ID_CAVERN_OF_SOULS:
		case CARD_ID_CHANNEL:				//(hardcoded wrong)
		case CARD_ID_DOUBLING_CUBE:			//should vary
		case CARD_ID_ELEMENTAL_RESONANCE:	//should vary
			return COLOR_TEST_COLORLESS;

		// varying

		case CARD_ID_CASCADE_BLUFFS:
		case CARD_ID_COMMAND_TOWER:			// not safe to search for edh card if this is drawn during game startup :(
		case CARD_ID_FETID_HEATH:
		case CARD_ID_FIRE_LIT_THICKET:
		case CARD_ID_FLOODED_GROVE:
		case CARD_ID_GENERIC_ANIMATED_LAND:
		case CARD_ID_GRAVEN_CAIRNS:
		case CARD_ID_MYSTIC_GATE:
		case CARD_ID_OPAL_PALACE:			// not safe to search for edh card if this is drawn during game startup :(
		case CARD_ID_RUGGED_PRAIRIE:
		case CARD_ID_SUNKEN_RUINS:
		case CARD_ID_TWILIGHT_MIRE:
		case CARD_ID_WOODED_BASTION:
		case CARD_ID_COMMANDERS_SPHERE:		// not safe to search for edh card if this is drawn during game startup :(
		case CARD_ID_BLOOM_TENDER:			//should vary
		case CARD_ID_AN_HAVVA_TOWNSHIP:
		case CARD_ID_AYSEN_ABBEY:
		case CARD_ID_CASTLE_SENGIR:
		case CARD_ID_KOSKUN_KEEP:
		case CARD_ID_WIZARDS_SCHOOL:
			return info_slot;	// even when it's -1

		case CARD_ID_CHROME_MOX:
		case CARD_ID_HONORED_HIERARCH:
		case CARD_ID_REALMWRIGHT:
			return info_slot >= 0 ? info_slot : 0;

		case CARD_ID_PRISMATIC_LENS:
		case CARD_ID_SHIMMERING_GROTTO:
		case CARD_ID_UNKNOWN_SHORES:
			return info_slot >= 0 ? info_slot : COLOR_TEST_COLORLESS | COLOR_TEST_ANY_COLORED;

		case CARD_ID_COLDSTEEL_HEART:
		case CARD_ID_GEM_BAZAAR:
		case CARD_ID_PARADISE_PLUME:
		case CARD_ID_UTOPIA_SPRAWL:
			return info_slot >= 0 ? info_slot : COLOR_TEST_ANY_COLORED;

		case CARD_ID_GEMSTONE_CAVERNS:
			return info_slot >= 0 ? info_slot : COLOR_TEST_COLORLESS;

		case CARD_ID_MIRRODINS_CORE:
		case CARD_ID_SOL_GRAIL:
		case CARD_ID_VERDANT_HAVEN:
			return info_slot >= 0 ? info_slot : COLOR_TEST_ANY_COLORED;

		case CARD_ID_PHYREXIAN_TOWER:
			return info_slot >= 0 ? info_slot : COLOR_TEST_COLORLESS | COLOR_TEST_BLACK;

		case CARD_ID_NIMBUS_MAZE:
			return info_slot >= 0 ? info_slot : COLOR_TEST_COLORLESS | COLOR_TEST_WHITE | COLOR_TEST_BLUE;

		case CARD_ID_RIVER_OF_TEARS:
			return info_slot >= 0 ? info_slot : COLOR_TEST_BLUE | COLOR_TEST_BLACK;

		case CARD_ID_TAINTED_FIELD:
			return info_slot >= 0 ? info_slot : COLOR_TEST_COLORLESS | COLOR_TEST_WHITE | COLOR_TEST_BLACK;

		case CARD_ID_TAINTED_ISLE:
			return info_slot >= 0 ? info_slot : COLOR_TEST_COLORLESS | COLOR_TEST_BLUE | COLOR_TEST_BLACK;

		case CARD_ID_TAINTED_PEAK:
			return info_slot >= 0 ? info_slot : COLOR_TEST_COLORLESS | COLOR_TEST_BLACK | COLOR_TEST_RED;

		case CARD_ID_TAINTED_WOOD:
			return info_slot >= 0 ? info_slot : COLOR_TEST_COLORLESS | COLOR_TEST_BLACK | COLOR_TEST_GREEN;

		case CARD_ID_TENDO_ICE_BRIDGE:
			return info_slot >= 0 ? info_slot : COLOR_TEST_COLORLESS | COLOR_TEST_ANY_COLORED;

		case CARD_ID_NYKTHOS_SHRINE_TO_NYX:
			return info_slot >= 0 ? info_slot : COLOR_TEST_COLORLESS | COLOR_TEST_ANY_COLORED;

		case CARD_ID_VIVID_CRAG:
			return info_slot >= 0 ? info_slot : COLOR_TEST_RED;

		case CARD_ID_STORMTIDE_LEVIATHAN:
			return info_slot >= 0 ? info_slot : COLOR_TEST_BLUE;

		case CARD_ID_VIVID_CREEK:
			return info_slot >= 0 ? info_slot : COLOR_TEST_BLUE;

		case CARD_ID_VIVID_GROVE:
		case CARD_ID_QUIRION_ELVES:
			return info_slot >= 0 ? info_slot : COLOR_TEST_GREEN;

		case CARD_ID_URBORG_TOMB_OF_YAWGMOTH:
			return info_slot >= 0 ? info_slot : COLOR_TEST_BLACK;

		case CARD_ID_VIVID_MARSH:
			return info_slot >= 0 ? info_slot : COLOR_TEST_BLACK;

		case CARD_ID_VIVID_MEADOW:
			return info_slot >= 0 ? info_slot : COLOR_TEST_WHITE;

		case CARD_ID_SPRINGJACK_PASTURE:
			return info_slot >= 0 && (info_slot & (1<<30)) ? COLOR_TEST_ANY : COLOR_TEST_COLORLESS;

		case CARD_ID_EXOTIC_ORCHARD:
		case CARD_ID_FELLWAR_STONE:
		case CARD_ID_SYLVOK_EXPLORER:
			player = 1 - player;
			// and fall through
		case CARD_ID_REFLECTING_POOL:
		case CARD_ID_SQUANDERED_RESOURCES:
		case CARD_ID_STAR_COMPASS:{
			/* These should really be computed during EVENT_COUNT_MANA, not here.  This is called after EVENT_CHANGE_TYPE is dispatched to this card; other
			 * cards might be queued to change type but not yet have done so. */
			if (info_slot == -1){
				return COLOR_TEST_ANY_COLORED;
			}

			// check the colors for all lands we have in play
			int count, color = 0;
			for (count = 0; count < active_cards_count[player] && (color & 0x3F) != 0x3F; ++count){
				card_data_t* card_d = get_card_data(player, count);
				if ((card_d->type & TYPE_LAND) && in_play(player, count) && card_d->id != CARD_ID_REFLECTING_POOL){
					if( csvid != CARD_ID_STAR_COMPASS || (csvid == CARD_ID_STAR_COMPASS && has_subtype(player, count, SUBTYPE_BASIC)) ){
						color |= get_colors_of_mana_land_could_produce_ignoring_costs(player, count);
					}
				}
			}
			if (csvid == CARD_ID_REFLECTING_POOL || csvid == CARD_ID_SQUANDERED_RESOURCES){
				if (color & COLOR_TEST_ARTIFACT){
					color &= ~COLOR_TEST_ARTIFACT;
					color |= COLOR_TEST_COLORLESS;
				}
			} else {
				color &= ~(COLOR_TEST_COLORLESS | COLOR_TEST_ARTIFACT);
			}
			return color;
		}

		default:
			return -1;
	}
}

// Stores last-hacked color_test in info_slot.  No avoiding it; it's needed for get_color_of_mana_produced_by_id(), which doesn't get a card ref.
int all_lands_are_basiclandtype(int player, int card, int event, int whose_lands, int land_color, int land_subtype)
{
#if 0
  if(event == EVENT_MODIFY_MANA_PROD && is_what(affected_card_controller, affected_card, TYPE_LAND)){
	/* This needs cc[2] & 2 set to even be called, and all it does is change card_instance_t::mana_color, i.e. what color the mana stripe is.  The event works
	 * by setting dword_55CCEB0 and dword_55CCEB4, not return value or event_result; dword_55CCEB0 is |ed with the normal result and dword_55CCEB4 is &ed with
	 * it.  It won't actually change what color of mana that's produced.  The only card that successfully uses this is Fertile Ground, though Wild Growth would
	 * respond to the event to set add a green manastripe to its land if the cc[2] bit was set.
	 *
	 * Responding to EVENT_SET_COLOR can change card_instance_t::color, which is how e.g. Thoughtlace works.
	 *
	 * To make a land actually tappable for a new color, we'd have to set card_instance_t::card_color, which no event lets you do directly.  And even then it
	 * would only work for lands that work by calling mana_producer_tapped(), which isn't many of the exe ones. */
	event_result |= 1 << get_hacked_color(player, card, land_color);
  }
#endif

  // Added/changed subtypes in general should really be cleared at the start of EVENT_CHANGE_TYPE, then added during it.

  int remove_subtypes = 0, add_subtypes = 0;

  if (event == EVENT_CAST_SPELL
	  && in_play(player, card)
	  && (affected_card_controller == whose_lands || whose_lands == ANYBODY)
	  && is_what(affected_card_controller, affected_card, TYPE_LAND))
	add_a_subtype(affected_card_controller, affected_card, get_hacked_subtype(player, card, land_subtype));

  if (event == EVENT_CHANGE_TYPE
	  && in_play(player, card)
	  && (affected_card_controller == whose_lands || whose_lands == ANYBODY))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int col = 1 << get_hacked_color(player, card, land_color);
	  if (instance->info_slot != col)
		{
		  add_subtypes = 1;
		  instance->info_slot = col;
		}
	}

  if (leaves_play(player, card, event))
	remove_subtypes = 1;

  if (add_subtypes)
	{
	  int p, c;
	  int subt = get_hacked_subtype(player, card, land_subtype);
	  for (p = 0; p < 2; ++p)
		if (p == whose_lands || whose_lands == ANYBODY)
		  for (c = 0; c < active_cards_count[p]; ++c)
			if (in_play(p, c) && is_what(p, c, TYPE_LAND))
			  add_a_subtype(p, c, subt);

	  get_card_instance(player, card)->info_slot = 1 << get_hacked_color(player, card, land_color);
	}

  if (remove_subtypes)
	{
	  test_definition_t this_test;
	  default_test_definition(&this_test, TYPE_LAND);
	  new_manipulate_all(player, card, whose_lands, &this_test, ACT_RESET_ADDED_SUBTYPE);
	}

  if (player == whose_lands || whose_lands == ANYBODY)
	return permanents_you_control_can_tap_for_mana(player, card, event, TYPE_LAND, -1, get_hacked_color(player, card, land_color), 1);
  else
	return 0;
}

void minimize_nondraining_mana(void)
{
  int p, c, nondraining;
  for (p = 0; p <= 1; ++p)
	for (c = 0; c < 7; ++c)
	  if ((nondraining = (mana_doesnt_drain_from_pool[p][c] & MANADRAIN_AMT_MASK))
		  && mana_in_pool[p][c] < nondraining)
		mana_doesnt_drain_from_pool[p][c] = mana_in_pool[p][c] | (mana_doesnt_drain_from_pool[p][c] & ~MANADRAIN_AMT_MASK);
}

/* A specific amount of a specific color of mana already in player's mana pool doesn't drain until end of turn.  Unlike MANADRAIN_DOESNT_DRAIN and
 * MANADRAIN_BECOMES_COLORLESS, this should be called immediately after adding the mana to the pool, not during EVENT_MANA_POOL_DRAINING. */
void mana_doesnt_drain_until_eot(int player, color_t color, int amt)
{
  if (amt < 0)
	return;

  int nondraining = mana_doesnt_drain_from_pool[player][color] & MANADRAIN_AMT_MASK;

  nondraining += amt;
  nondraining = MIN(nondraining, MANADRAIN_AMT_MASK);
  nondraining = MIN(nondraining, mana_in_pool[player][color]);

  mana_doesnt_drain_from_pool[player][color] &= ~MANADRAIN_AMT_MASK;
  mana_doesnt_drain_from_pool[player][color] |= nondraining;
}

int finalize_activation(int player, int card);
void mana_burn(void)
{
  // 0x43A060

  if (!(trace_mode & 2))
	{
	restart:;
	  int c;
	  card_instance_t* instance;
	  if (mana_in_pool[AI][7] && current_phase > PHASE_NORMAL_COMBAT_DAMAGE
		  && active_cards_count[AI] < 150)
		for (c = 0; c < active_cards_count[AI]; ++c)
		  if ((instance = in_play(AI, c))
			  && dispatch_event_with_attacker_to_one_card(AI, c, EVENT_CAN_WASTE_MANA, 1-AI, -1))
			{
			  if (EXE_FN(int, 0x434040, int, int, int)(AI, AI, c))	// activate(AI, AI, c)
				finalize_activation(AI, c);
			  goto restart;
			}
	}

  int p, c;
  for (p = 0; p <= 1; ++p)
	for (c = 0; c < 7; ++c)
	  mana_doesnt_drain_from_pool[p][c] &= MANADRAIN_AMT_MASK;
  dispatch_event(0, 0, EVENT_MANA_POOL_DRAINING);

  for (p = 0; p < 2; ++p)
	{
	  int amt_drained = 0, amt_left = 0;
	  int clr;
	  if (mana_in_pool[p][COLOR_ANY] > 0)
		for (clr = COLOR_ARTIFACT; clr >= COLOR_COLORLESS; --clr)
		  if (mana_doesnt_drain_from_pool[p][clr] & MANADRAIN_DOESNT_DRAIN)
			amt_left += mana_in_pool[p][clr];
		  else
			{
			  uint8_t nondraining;
			  if ((nondraining = (mana_doesnt_drain_from_pool[p][clr] & MANADRAIN_AMT_MASK)))
				{
				  if (nondraining > mana_in_pool[p][clr])
					{	// more flagged as not-draining than is actually in pool
					  nondraining = mana_in_pool[p][clr];
					  mana_doesnt_drain_from_pool[p][clr] &= ~MANADRAIN_AMT_MASK;
					  mana_doesnt_drain_from_pool[p][clr] |= nondraining;
					}
				  mana_in_pool[p][clr] -= nondraining;
				}

			  if (mana_doesnt_drain_from_pool[p][clr] & MANADRAIN_BECOMES_COLORLESS)
				{
				  amt_left += mana_in_pool[p][clr];
				  mana_in_pool[p][COLOR_COLORLESS] += mana_in_pool[p][clr];
				}
			  else
				amt_drained += mana_in_pool[p][clr];

			  mana_in_pool[p][clr] = nondraining;
			  amt_left += nondraining;
			}

	  mana_in_pool[p][COLOR_ANY] = amt_left;

	  int setting_mana_burn;
	  if (amt_drained > 0
		  && (setting_mana_burn = get_setting(SETTING_MANA_BURN))
		  && !(setting_mana_burn == 2 && IS_AI(p)))
		{
		  if (ai_is_speculating != 1)
			{
			  play_sound_effect(WAV_MANABURN);
			  EXE_FN(void, 0x470250, int, int)(p, amt_drained);	// display mana burn dialog
			}
		  life[p] -= amt_drained;
		}
	}
}
