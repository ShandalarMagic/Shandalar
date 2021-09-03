// -*- c-basic-offset:2 -*-
#include "manalink.h"

int card_UNUSED1(int player, int card, event_t event){return 0;}
int card_UNUSED2(int player, int card, event_t event){return 0;}

/***** Functions *****/

// Awaken.  See comments within awaken_tgt; also, requires the Modifies Casting Cost bit to be set.
#define IS_AWAKEN_EVENT(player, card, event)	(IS_GS_EVENT(player, card, event) || event == EVENT_MODIFY_COST || event == EVENT_QUERY_AWAKEN)

static int awaken_tgt(int player, int card, event_t event, int flags, target_definition_t* td, const char* prompt, int x, int b, int u, int g, int r, int w)
{
  /* 702.112. Awaken
   *
   * 702.112a Awaken appears on some instants and sorceries. It represents two abilities: a static ability that functions while the spell with awaken is on the
   * stack and a spell ability. "Awaken N - [cost]" means "You may pay [cost] rather than pay this spell's mana cost as you cast this spell" and "If this
   * spell's awaken cost was paid, put N +1/+1 counters on target land you control. That land becomes a 0/0 Elemental creature with haste. It's still a land."
   * Paying a spell's awaken cost follows the rules for paying alternative costs in rules 601.2b and 601.2f-h.
   *
   * 702.112b The controller of a spell with awaken chooses the target of the awaken spell ability only if that player chose to pay the spell's awaken
   * cost. Otherwise the spell is cast as if it didn't have that target. */

  /* Awaken is particularly awkward at resolution.  Its effects have to happen after the card's other effects (see the rulings for Rising Miasma, for example);
   * and if the card has no other targets and it's cast with awaken, its awaken target needs to successfully validate or the spell is countered.
   *
   * For a spell with awaken without other targets, the outline looks like:
   *  if (event == EVENT_RESOLVE_SPELL)
   *    {
   *      if (awaken_validate(player, card, 0))
   *        {
   *          other_effects();
   *          awaken_resolve(player, card, 0, number_of_counters);
   *        }
   *      kill_card(player, card, KILL_DESTROY);
   *    }
   *  return awaken(player, card, event, MANACOST_whatever());
   *
   * For a spell with awaken with other targets, the outline looks line:
   *  if (!IS_AWAKEN_EVENT(player, card, event))
   *    return 0;
   *
   *  target_definition_t td;
   *  default_target_definition(player, card, &td, TYPE_whatever);
   *
   *  if (event == EVENT_RESOLVE_SPELL)
   *    {
   *      if (valid_target(&td))
   *        other_effects();
   *      if (awaken_validate(player, card, 1))
   *        awaken_resolve(player, card, 1, number_of_counters);
   *      kill_card(player, card, KILL_DESTROY);
   *    }
   *  return awaken_tgt(player, card, event, GS_CAN_TARGET, &td, "TARGET_whatever", MANACOST_whatever()); */

  if (event == EVENT_QUERY_AWAKEN)
	return 1;

  if (event == EVENT_MODIFY_COST)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  x = get_updated_casting_cost(player, -1, inst->internal_card_id, event, x);
	  if (has_mana_multi(player, MAX(0, x), b, u, g, r, w))
		{
		  null_casting_cost(player, card);
		  inst->info_slot = CARD_ID_AWAKENING;
		}
	  else
		inst->info_slot = 0;
	}

  if (!IS_GS_EVENT(player, card, event) || event == EVENT_RESOLVE_SPELL)
	return 0;

  if (event == EVENT_CAN_CAST)
	return !td ? 1 : generic_spell(player, card, event, flags, td, prompt, 1, NULL);

  target_definition_t td_land;
  default_target_definition(player, card, &td_land, TYPE_LAND);
  td_land.allowed_controller = td_land.preferred_controller = player;

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->info_slot == CARD_ID_AWAKENING
		  && !is_token(player, card) && !check_special_flags(player, card, SF_NOT_CAST) && can_target(&td_land))
		switch (DIALOG(player, card, event,
					   DLG_RANDOM, DLG_NO_STORAGE,
					   "Pay mana cost",	1, 1,
					   "Pay awaken cost",	1, 3))
		  {
			case 1:
			  if (!played_for_free(player, card))
				charge_mana_from_id(player, card, event, get_id(player, card));
			  break;

			case 2:
			  x = get_updated_casting_cost(player, -1, inst->internal_card_id, event, x);
			  charge_mana_multi(player, MAX(0, x), b, u, g, r, w);
			  if (cancel == 1)
				return 0;
			  if (td)
				{
				  generic_spell(player, card, event, flags, td, prompt, 1, NULL);
				  if (cancel == 1)
					return 0;
				}
			  if (new_pick_target(&td_land, "TARGET_LAND", -1, 1))
				inst->info_slot = CARD_ID_AWAKENING + 1;
			  return 0;	// don't fall through to the generic_spell call below

			default:
			  cancel = 1;
			  break;
		  }
	}

  if (cancel == 1)
	return 0;
  else
	return generic_spell(player, card, event, flags, td, prompt, td ? 1 : 0, NULL);
}

static int awaken(int player, int card, event_t event, int x, int b, int u, int g, int r, int w)
{
  return awaken_tgt(player, card, event, 0, NULL, NULL, x,b,u,g,r,w);
}

static int awaken_validate(int player, int card, int target_idx)
{
  card_instance_t* inst = get_card_instance(player, card);
  if (inst->info_slot != CARD_ID_AWAKENING + 1)
	return 1;

  target_definition_t td_land;
  default_target_definition(player, card, &td_land, TYPE_LAND);
  td_land.allowed_controller = td_land.preferred_controller = player;

  return validate_target(player, card, &td_land, target_idx);
}

static int awaken_resolve(int player, int card, int target_idx, int number_of_counters)
{
  card_instance_t* inst = get_card_instance(player, card);
  /* Normally, awaken_validate() - which must always be called and return true before calling this - will ensure that the target is in play.  It's conceivable,
   * though, that the card's other effects will have removed it from the bf. */
  if (inst->info_slot != CARD_ID_AWAKENING + 1
	  || !in_play(inst->targets[target_idx].player, inst->targets[target_idx].card))
	return -1;

  add_a_subtype(inst->targets[target_idx].player, inst->targets[target_idx].card, SUBTYPE_ELEMENTAL);
  add_1_1_counters(inst->targets[target_idx].player, inst->targets[target_idx].card, number_of_counters);
  return animate_other(player, card, inst->targets[target_idx].player, inst->targets[target_idx].card, 0,0, 0,SP_KEYWORD_HASTE, 0, 1);
}

static int blighted_land(int player, int card, event_t event,
						 int x, int b, int u, int g, int r, int w,
						 const char* label, int ai,
						 target_definition_t* td, const char* prompt,
						 void (*impl)(int, int, target_definition_t*))
{
  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_SACRIFICE_FOR_EFFECT
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1, paying_mana() ? DLG_NO_DISPLAY_FOR_AI : DLG_NO_OP,
						"Produce mana",
							mana_producer(player, card, EVENT_CAN_ACTIVATE),
							paying_mana(),
						label,
							/* This calls generic_activated_ability() and charges mana manually instead of using a DLG_MANA() clause because both this and its
							 * mana ability have T in their costs */
							!paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE,
																		GAA_UNTAPPED|GAA_SACRIFICE_ME|(td ? GAA_CAN_TARGET : 0),
																		x+1,b,u,g,r,w, 0, td, prompt),
							landsofcolor_controlled[player][COLOR_ANY] - 6);
	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_SACRIFICE_FOR_EFFECT)
		{
		  if (event == EVENT_ACTIVATE)
			{
			  add_state(player, card, STATE_TAPPED);
			  charge_mana_for_activated_ability(player, card, x,b,u,g,r,w);
			  if (td && cancel != 1)
				{
				  card_instance_t* inst = get_card_instance(player, card);
				  if (td->zone == TARGET_ZONE_PLAYERS && td->allowed_controller == 1-player)
					{	// As usual, don't prompt for "target opponent", just do it.
					  inst->targets[0].player = 1-player;
					  inst->targets[0].card = -1;
					  inst->number_of_targets = 1;
					}
				  else
					{
					  inst->number_of_targets = 0;
					  pick_target(td, prompt);
					}
				}

			  if (cancel != 1)
				kill_card(player, card, KILL_SACRIFICE);
			  else
				remove_state(player, card, STATE_TAPPED);
			}
		  else if (event == EVENT_RESOLVE_ACTIVATION)
			(*impl)(player, card, td);

		  return 0;
		}
	  // else fall through to mana_producer below
	}

  return mana_producer(player, card, event);
}

// Requires the Modifies Casting Cost bit to be set.
static inline void converge(int player, int card, event_t event)
{
  store_num_colors_paid_in_info_slot(player, card, event);
}

static void ingest(int player, int card, event_t event)
{
  /* 702.114. Ingest
   *
   * 702.114a Ingest is a triggered ability. "Ingest" means "Whenever this creature deals combat damage to a player, that player exiles the top card of his or
   * her library."
   *
   * 702.114b If a creature has multiple instances of ingest, each triggers separately. */

  if (card != -1
	  && damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER|DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_TRACE_DAMAGED_PLAYERS|RESOLVE_TRIGGER_MANDATORY))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int p, times_damaged[2] = { BYTE0(inst->targets[1].player), BYTE1(inst->targets[1].player) };
	  inst->targets[1].player = 0;
	  for (p = 0; p <= 1; ++p)
		if (times_damaged[p] > 0)
		  rfg_top_n_cards_of_deck(p, times_damaged[p]);
	}
}

static int can_process(int who_processes, int num_cards)
{
  return count_rfg(1-who_processes) >= num_cards;
}

static int process(int who_processes, int num_cards)
{
  // You may put [num_cards] cards your opponents own from exile into their owners' graveyards.  If you do, [return nonzero]
  if (num_cards > 0 && can_process(who_processes, num_cards))
	{
	  test_definition_t test;
	  default_test_definition(&test, TYPE_ANY);
	  if (new_global_tutor(who_processes, 1-who_processes, TUTOR_FROM_RFG, TUTOR_GRAVE, 0, AI_MIN_VALUE, &test) != -1 && num_cards > 1)
		{
		  test.qty = num_cards - 1;
		  new_global_tutor(who_processes, 1-who_processes, TUTOR_FROM_RFG, TUTOR_GRAVE, 1, AI_MIN_VALUE, &test);
		  return 1;
		}
	}
  return 0;
}

static int process_as_cost(int who_processes, int num_cards, event_t event, int gaa_result)
{
  if (event == EVENT_CAN_ACTIVATE)
	return !gaa_result || !can_process(who_processes, num_cards) ? 0 : gaa_result;
  if (event == EVENT_ACTIVATE && cancel != 1)
	{
	  if (!process(who_processes, num_cards))
		cancel = 1;
	}
  return gaa_result;
}

static int rally(int player, int card, event_t event, resolve_trigger_t mode)
{
  // Rally - Whenever ~ or another Ally enters the battlefield under your control, [return true]

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == player)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "");
	  test.subtype = SUBTYPE_ALLY;
	  test.subtype_flag = MATCH;
	  test.not_me = 2;	// === pass the test if it's this card.  Only valid for new_specific_cip().

	  return new_specific_cip(player, card, event, player, mode, &test);
	}
  return 0;
}

static int test_isnt_land(int iid, int unused, int player, int card)
{
  return card >= 0 ? !is_what(player, card, TYPE_LAND) : !is_what(-1, iid, TYPE_LAND);
}

int when_you_cast(int player, int card, event_t event);

/***** Cards *****/

/*** Colorless ***/

int card_bane_of_bala_ged(int player, int card, event_t event)
{
  /* Bane of Bala Ged	|7	0x200de64
   * Creature - Eldrazi 7/5
   * Whenever ~ attacks, defending player exiles two permanents he or she controls. */

  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	{
	  target_definition_t td;
	  base_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.who_chooses = td.allowed_controller = td.preferred_controller = 1-current_turn;
	  td.allow_cancel = 0;

	  int i, chosen = pick_up_to_n_targets_noload(&td, "Select a permanent you control.", 2);
	  card_instance_t* inst = get_card_instance(player, card);
	  for (i = 0; i < chosen; ++i)
		kill_card(inst->targets[i].player, inst->targets[i].card, KILL_REMOVE);
	  inst->number_of_targets = 0;
	}

  return 0;
}

int card_blight_herder(int player, int card, event_t event)
{
  /* Blight Herder	|5	0x200de69
   * Creature - Eldrazi Processor 4/5
   * When you cast ~, you may put two cards your opponents own from exile into their owners' graveyards. If you do, put three 1/1 colorless Eldrazi Scion creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your mana pool." */

  if (when_you_cast(player, card, event) && process(player, 2))
	generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SCION, 3);

  return 0;
}

/* Breaker of Armies	|8	=>urza_destiny.c:card_taunting_elf
 * Creature - Eldrazi 10/8
 * All creatures able to block ~ do so. */

int card_conduit_of_ruin(int player, int card, event_t event)
{
  /* Conduit of Ruin	|6	0x200e2b5
   * Creature - Eldrazi 5/5
   * When you cast ~, you may search your library for a colorless creature card with converted mana cost 7 or greater, reveal it, then shuffle your library and put that card on top of it.
   * The first creature spell you cast each turn costs |2 less to cast. */

  if (event == EVENT_MODIFY_COST_GLOBAL)
	{
	  if ((affected_card_controller == -1 ? event_result : affected_card_controller) == player
		  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		  && get_stormcreature_count(player) == 0
		  && !is_humiliated(player, card))
		COST_COLORLESS -= 2;
	  return 0;
	}

  if (when_you_cast(player, card, event)
	  && do_dialog(player, player, card, -1, -1, " Search library\n Decline", 0) == 0)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select a colorless creature card with converted mana cost 7 or greater.");
	  test.color = COLOR_TEST_ANY_COLORED;
	  test.color_flag = DOESNT_MATCH;
	  test.cmc = 6;
	  test.cmc_flag = F5_CMC_GREATER_THAN_VALUE;

	  new_global_tutor(player, 1-player, TUTOR_FROM_DECK, TUTOR_DECK, 0, AI_MAX_VALUE, &test);
	}

  return 0;
}

/* Deathless Behemoth	|6	0x000000
 * Creature - Eldrazi 6/6
 * Vigilance
 * Sacrifice two Eldrazi Scions: Return ~ from your graveyard to your hand. Activate this ability only any time you could cast a sorcery. */

int card_desolation_twin(int player, int card, event_t event)
{
  /* Desolation Twin	|10	0x200de6e
   * Creature - Eldrazi 10/10
   * When you cast ~, put a 10/10 colorless Eldrazi creature token onto the battlefield. */

  if (when_you_cast(player, card, event))
	generate_token_by_id(player, card, CARD_ID_ELDRAZI);

  return 0;
}

/* Eldrazi Devastator	|8	=>vanilla
 * Creature - Eldrazi 8/9
 * Trample */

int card_endless_one(int player, int card, event_t event)
{
  /* Endless One	|X	0x200de73
   * Creature - Eldrazi 0/0
   * ~ enters the battlefield with X +1/+1 counters on it. */

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  set_special_flags2(player, card, SF2_X_SPELL);
	  if (played_for_free(player, card) || is_token(player, card))
		inst->info_slot = 0;
	  else
		{
		  charge_mana(player, COLOR_COLORLESS, -1);
		  inst->info_slot = x_value;
		}
	}

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, inst->info_slot);

  return 0;
}

/* Gruesome Slaughter	|6	0x000000
 * Sorcery
 * Until end of turn, colorless creatures you control gain "|T: This creature deals damage equal to its power to target creature." */

/* Kozilek's Channeler	|5	=>scars_of_mirrodin.c:card_palladium_myr
 * Creature - Eldrazi 4/4
 * |T: Add |2 to your mana pool. */

int card_oblivion_sower(int player, int card, event_t event)
{
  /* Oblivion Sower	|6	0x200de78
   * Creature - Eldrazi 5/8
   * When you cast ~, target opponent exiles the top four cards of his or her library, then you may put any number of land cards that player owns from exile onto the battlefield under your control. */

  if (when_you_cast(player, card, event) && opponent_is_valid_target(player, card))
	{
	  rfg_top_n_cards_of_deck(1-player, 4);

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "Select a land card.");

	  while (new_global_tutor(player, 1-player, TUTOR_FROM_RFG, TUTOR_PLAY, 0, AI_MAX_VALUE, &test) != -1)
		{}
	}

  return 0;
}

int card_ruin_processor(int player, int card, event_t event)
{
  /* Ruin Processor	|7	0x200de7d
   * Creature - Eldrazi Processor 7/8
   * When you cast ~, you may put a card an opponent owns from exile into that player's graveyard. If you do, you gain 5 life. */

  if (when_you_cast(player, card, event) && process(player, 1))
	gain_life(player, 5);

  return 0;
}

int card_scour_from_existence(int player, int card, event_t event)
{
  /* Scour from Existence	|7	0x200de82
   * Instant
   * Exile target permanent. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_REMOVE);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
}

int card_titans_presence(int player, int card, event_t event)
{
  /* Titan's Presence	|3	0x200e2bf
   * Instant
   * As an additional cost to cast ~, reveal a colorless creature card from your hand.
   * Exile target creature if its power is less than or equal to the revealed card's power. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select a colorless creature card.");
  test.color = COLOR_TEST_ANY_COLORED;
  test.color_flag = DOESNT_MATCH;
  test.zone = TARGET_ZONE_HAND;

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td)
		  && get_power(inst->targets[0].player, inst->targets[0].card) <= inst->info_slot)
		kill_card(inst->targets[0].player, inst->targets[0].card, KILL_REMOVE);

	  kill_card(player, card, KILL_DESTROY);
	}

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  /* Good thing that all the Manalink selection functions force the AI into predetermined strategies, none of which are relevant here, or else it might be
	   * able to figure out to pick a high power card through normal speculation. */
	  int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &test);
	  if (selected == -1)
		{
		  cancel = 1;
		  return 0;
		}
	  else
		{
		  reveal_card(player, card, player, selected);
		  inst->info_slot = get_base_power(player, selected);
		}
	}

  int rval = generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
  if (rval && event == EVENT_CAN_CAST && !check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test))
	return 0;

  return rval;
}

int card_ulamog_the_ceaseless_hunger(int player, int card, event_t event)
{
  /* Ulamog, the Ceaseless Hunger	|10	0x200de87
   * Legendary Creature - Eldrazi 10/10
   * When you cast ~, exile two target permanents.
   * Indestructible
   * Whenever ~ attacks, defending player exiles the top twenty cards of his or her library. */

  check_legend_rule(player, card, event);
  indestructible(player, card, event);

  if (when_you_cast(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.allow_cancel = 0;

	  int i, chosen = pick_up_to_n_targets(&td, "TARGET_PERMANENT", 2);
	  card_instance_t* inst = get_card_instance(player, card);
	  for (i = 0; i < chosen; ++i)
		kill_card(inst->targets[i].player, inst->targets[i].card, KILL_REMOVE);
	  inst->number_of_targets = 0;
	}

  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	rfg_top_n_cards_of_deck(1-current_turn, 20);

  return 0;
}

int card_ulamogs_despoiler(int player, int card, event_t event)
{
  /* Ulamog's Despoiler	|6	0x200de8c
   * Creature - Eldrazi Processor 5/5
   * As ~ enters the battlefield, you may put two cards your opponents own from exile into their owners' graveyards. If you do, ~ enters the battlefield with four +1/+1 counters on it. */

  card_instance_t* inst = get_card_instance(player, card);
  if (when_you_cast(player, card, event))
	inst->info_slot = process(player, 2) ? CARD_ID_ULAMOGS_DESPOILER : 0;

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, inst->info_slot == CARD_ID_ULAMOGS_DESPOILER ? 4 : 0);

  return 0;
}

int card_void_winnower(int player, int card, event_t event)
{
  /* Void Winnower	|9	0x200de91
   * Creature - Eldrazi 11/9
   * Your opponents can't cast spells with even converted mana costs.
   * Your opponents can't block with creatures with even converted mana costs. */

  if (event == EVENT_MODIFY_COST_GLOBAL)
	{
	  if ((affected_card_controller == -1 ? event_result : affected_card_controller) == 1-player
		  && !is_what(affected_card_controller, affected_card, TYPE_LAND) && get_cmc(affected_card_controller, affected_card) % 2 == 0
		  && !is_humiliated(player, card))
		infinite_casting_cost();
	  return 0;
	}

  if (event == EVENT_BLOCK_LEGALITY && affected_card_controller == 1-player && get_cmc(affected_card_controller, affected_card) % 2 == 0
	  && !is_humiliated(player, card))
	event_result = 1;

  return 0;
}

/*** White ***/

int card_angel_of_renewal(int player, int card, event_t event)
{
  /* Angel of Renewal	|5|W	0x200de96
   * Creature - Angel Ally 4/4
   * Flying
   * When ~ enters the battlefield, you gain 1 life for each creature you control. */

  if (comes_into_play(player, card, event))
	gain_life(player, count_subtype(player, TYPE_CREATURE, -1));

  return 0;
}

int card_angelic_gift(int player, int card, event_t event)
{
  /* Angelic Gift	|1|W	0x200de9b
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield, draw a card.
   * Enchanted creature has flying. */

  if (comes_into_play(player, card, event))
	draw_a_card(player);

  return generic_aura(player, card, event, player, 0,0, KEYWORD_FLYING,0, 0,0,0);
}

int card_cliffside_lookout(int player, int card, event_t event)
{
  /* Cliffside Lookout	|W	0x200dea0
   * Creature - Kor Scout Ally 1/1
   * |4|W: Creatures you control get +1/+1 until end of turn. */

  if (event == EVENT_CHECK_PUMP)
	{
	  int pump = generic_shade_amt_can_pump(player, card, 1, 0, MANACOST_XW(4,1), -1);
	  pumpable_power[player] += pump;
	  pumpable_toughness[player] += pump;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	pump_creatures_until_eot_merge_pt(player, card, player, 1,1, NULL);

  return generic_activated_ability(player, card, event, 0, MANACOST_XW(4,1), 0, NULL, NULL);
}

/* Courier Griffin	|3|W	=>stronghold.c:card_venerable_monk
 * Creature - Griffin 2/3
 * Flying
 * When ~ enters the battlefield, you gain 2 life. */

int card_emeria_shepherd(int player, int card, event_t event)
{
  /* Emeria Shepherd	|5|W|W	0x200dea5
   * Creature - Angel 4/4
   * Flying
   * Landfall - Whenever a land enters the battlefield under your control, you may return target nonland permanent card from your graveyard to your hand. If that land is |Ha Plains, you may return that nonland permanent card to the battlefield instead. */

  if (landfall_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "Select target nonland permanent card.");
	  test.special_selection_function = &test_isnt_land;

	  if (new_special_count_grave(player, &test) > 0 && !graveyard_has_shroud(player))
		{
		  tutor_t destination = TUTOR_HAND;
		  if (has_subtype(trigger_cause_controller, trigger_cause, get_hacked_subtype(player, card, SUBTYPE_PLAINS))
			  && DIALOG(player, card, EVENT_ACTIVATE,
						DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL,
						"Return to hand", 1, 1,
						"Return to battlefield", 1, 4) == 2)
			destination = TUTOR_PLAY;

		  new_global_tutor(player, player, TUTOR_FROM_GRAVE, destination, 0, AI_MAX_VALUE, &test);
		}
	}

  return 0;
}

int card_encircling_fissure(int player, int card, event_t event)
{
  /* Encircling Fissure	|2|W	0x200deaa
   * Instant
   * Prevent all combat damage that would be dealt this turn by creatures target opponent controls.
   * Awaken 2-|4|W */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, 0);
	  td.zone = TARGET_ZONE_PLAYERS;
	  td.allowed_controller = 1-player;

	  if (valid_target(&td))
		alternate_legacy_text(1, player, fog_special(player, card, 1-player, FOG_COMBAT_DAMAGE_ONLY));
	  if (awaken_validate(player, card, 1))
		alternate_legacy_text(2, player, awaken_resolve(player, card, 1, 2));
	  kill_card(player, card, KILL_DESTROY);
	}

  return awaken_tgt(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, NULL, NULL, MANACOST_XW(4,1));
}

/* Expedition Envoy	|W	=>vanilla
 * Creature - Human Scout Ally 2/1 */

/* Felidar Cub	|1|W	=>champions_of_kamigawa.c:card_kami_of_ancient_law
 * Creature - Cat Beast 2/2
 * Sacrifice ~: Destroy target enchantment. */

/* Felidar Sovereign	|4|W|W	=>zendikar.c:card_felidar_sovereign
 * Creature - Cat Beast 4/6
 * Vigilance, lifelink
 * At the beginning of your upkeep, if you have 40 or more life, you win the game. */

/* Fortified Rampart	|1|W	=>vanilla
 * Creature - Wall 0/6
 * Defender */

/* Ghostly Sentinel	|4|W	=>unlimited.c:card_serra_angel
 * Creature - Kor Spirit 3/3
 * Flying, vigilance */

int card_gideon_ally_of_zendikar(int player, int card, event_t event)
{
  /* Gideon, Ally of Zendikar	|2|W|W	0x200deaf
   * Planeswalker - Gideon (4)
   * +1: Until end of turn, ~ becomes a 5/5 Human Soldier Ally creature with indestructible that's still a planeswalker. Prevent all damage that would be dealt to him this turn.
   * 0: Put a 2/2 |Swhite Knight Ally creature token onto the battlefield.
   * -4: You get an emblem with "Creatures you control get +1/+1." */

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_ANIMATE = 1,
		CHOICE_TOKEN,
		CHOICE_EMBLEM
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Become creature",		1,	3,						+1,
						"Knight Ally token",	1,	2,						0,
						"Emblem",				1,	creature_count[player], -4);

	  if (event == EVENT_CAN_ACTIVATE && !choice)
		return 0;
	  if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_ANIMATE:
			  ;card_instance_t* inst = get_card_instance(player, card);
			  int p = inst->parent_controller, c = inst->parent_card;
			  if (in_play(p, c))
				{
				  get_card_instance(p, prevent_all_damage_to_target(p, c, p, c, 1))->token_status |= STATUS_INVISIBLE_FX;
				  animate_self(player, card, 5,5, 0,SP_KEYWORD_INDESTRUCTIBLE, 0, 0);
				}
			  break;

			case CHOICE_TOKEN:
			  generate_token_by_id(player, card, CARD_ID_KNIGHT_ALLY);
			  break;

			case CHOICE_EMBLEM:
			  generate_token_by_id(player, card, CARD_ID_GIDEON_ALLY_OF_ZENDIKAR_EMBLEM);
			  break;
		  }
	}

  has_subtype_if_animated_self(player, card, event, SUBTYPE_HUMAN);
  has_subtype_if_animated_self(player, card, event, SUBTYPE_SOLDIER);
  has_subtype_if_animated_self(player, card, event, SUBTYPE_ALLY);

  return planeswalker(player, card, event, 4);
}

/* Gideon, Ally of Zendikar Emblem	""	=>planar_chaos.c:card_gaeas_anthem
 * Emblem
 * Creatures you control get +1/+1. */

int card_gideons_reproach(int player, int card, event_t event)
{
  /* Gideon's Reproach	|1|W	0x200deb4
   * Instant
   * ~ deals 4 damage to target attacking or blocking creature. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_state = TARGET_STATE_IN_COMBAT;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, 4);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ATTACKING_BLOCKING_CREATURE", 1, NULL);
}

int card_hero_of_goma_fada(int player, int card, event_t event)
{
  /* Hero of Goma Fada	|4|W	0x200deb9
   * Creature - Human Knight Ally 4/3
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, creatures you control gain indestructible until end of turn. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_creatures_until_eot_no_repeat(player, card, player, 0, 0,SP_KEYWORD_INDESTRUCTIBLE, NULL);

  return 0;
}

/* Inspired Charge	|2|W|W	=>m11.c:card_inspired_charge
 * Instant
 * Creatures you control get +2/+1 until end of turn. */

/* Kitesail Scout	|W	=>vanilla
 * Creature - Kor Scout 1/1
 * Flying */

int card_kor_bladewhirl(int player, int card, event_t event)
{
  /* Kor Bladewhirl	|1|W	0x200debe
   * Creature - Kor Soldier Ally 2/2
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, creatures you control gain first strike until end of turn. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_creatures_until_eot_no_repeat(player, card, player, 0, KEYWORD_FIRST_STRIKE,0, NULL);

  return 0;
}

int card_kor_castigator(int player, int card, event_t event)
{
  /* Kor Castigator	|1|W	0x200dec3
   * Creature - Kor Wizard Ally 3/1
   * ~ can't be blocked by Eldrazi Scions. */

  if (event == EVENT_BLOCK_LEGALITY && attacking_card == card && attacking_card_controller == player && !is_humiliated(player, card)
	  && has_subtype(affected_card_controller, affected_card, SUBTYPE_ELDRAZI) && has_subtype(affected_card_controller, affected_card, SUBTYPE_SCION))
	event_result = 1;

  return 0;
}

int card_kor_entanglers(int player, int card, event_t event)
{
  /* Kor Entanglers	|4|W	0x200dec8
   * Creature - Kor Soldier Ally 3/4
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, tap target creature an opponent controls. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		tap_card(inst->targets[0].player, inst->targets[0].card);
	}

  return 0;
}

int card_lantern_scout(int player, int card, event_t event)
{
  /* Lantern Scout	|2|W	0x200decd
   * Creature - Human Scout Ally 3/2
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, creatures you control gain lifelink until end of turn. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_creatures_until_eot_no_repeat(player, card, player, 0, 0,SP_KEYWORD_LIFELINK, NULL);

  return 0;
}

static int fx_p2_p2_fog_me_from_colorless(int player, int card, event_t event)
{
  card_instance_t* inst;
  if (event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  inst = get_card_instance(player, card);
	  if (affect_me(inst->damage_target_player, inst->damage_target_card))
		event_result += 2;
	}

  card_instance_t* damage = damage_being_prevented(event);
  if (damage
	  && (inst = get_card_instance(player, card))
	  && damage->damage_target_card == inst->damage_target_card
	  && damage->damage_target_player == inst->damage_target_player
	  && !(damage->initial_color & COLOR_TEST_ANY_COLORED))
	damage->info_slot = 0;

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
int card_lithomancers_focus(int player, int card, event_t event)
{
  /* Lithomancer's Focus	|W	0x200ded2
   * Instant
   * Target creature gets +2/+2 until end of turn. Prevent all damage that would be dealt to that creature this turn by colorless sources. */

  if (event == EVENT_CHECK_PUMP && has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id))
	{
	  pumpable_power[player] += 2;
	  pumpable_toughness[player] += 2;
	}

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  create_targetted_legacy_effect(player, card, &fx_p2_p2_fog_me_from_colorless, inst->targets[0].player, inst->targets[0].card);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_makindi_patrol(int player, int card, event_t event)
{
  /* Makindi Patrol	|2|W	0x200ded7
   * Creature - Human Knight Ally 2/3
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, creatures you control gain vigilance until end of turn. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_creatures_until_eot_no_repeat(player, card, player, 0, 0,SP_KEYWORD_VIGILANCE, NULL);

  return 0;
}

/* Ondu Greathorn	|3|W	=>zendikar.c:card_steppe_lynx
 * Creature - Beast 2/3
 * First strike
 * Landfall - Whenever a land enters the battlefield under your control, ~ gets +2/+2 until end of turn. */

static int fx_when_creature_attacks_it_gains_lifelink(int player, int card, event_t event)
{
  int amt;
  if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, ANYBODY, -1)))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  unsigned char* attackers = (unsigned char*)(&inst->targets[2].player);
	  for (--amt; amt >= 0; --amt)
		if (in_play(current_turn, attackers[amt]))
		  alternate_legacy_text(1, player, pump_ability_until_eot(player, card, current_turn, attackers[amt], 0,0, 0,SP_KEYWORD_LIFELINK));
	}

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
int card_ondu_rising(int player, int card, event_t event)
{
  /* Ondu Rising	|1|W	0x200dedc
   * Sorcery
   * Whenever a creature attacks this turn, it gains lifelink until end of turn.
   * Awaken 4-|4|W */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (awaken_validate(player, card, 0))
		{
		  alternate_legacy_text(1, player, create_legacy_effect(player, card, &fx_when_creature_attacks_it_gains_lifelink));
		  alternate_legacy_text(2, player, awaken_resolve(player, card, 0, 4));
		}
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken(player, card, event, MANACOST_XW(4,1));
}

int card_planar_outburst(int player, int card, event_t event)
{
  /* Planar Outburst	|3|W|W	0x200dee1
   * Sorcery
   * Destroy all nonland creatures.
   * Awaken 4-|5|W|W|W */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (awaken_validate(player, card, 0))
		{
		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_CREATURE, "");
		  test.special_selection_function = &test_isnt_land;
		  new_manipulate_all(player, card, ANYBODY, &test, ACT_KILL_DESTROY);

		  awaken_resolve(player, card, 0, 4);
		}
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken(player, card, event, MANACOST_XW(5,3));
}

int card_quarantine_field(int player, int card, event_t event)
{
  /* Quarantine Field	|X|X|W|W	0x200dee6
   * Enchantment
   * ~ enters the battlefield with X isolation counters on it.
   * When ~ enters the battlefield, for each isolation counter on it, exile up to one target nonland permanent an opponent controls until ~ leaves the battlefield. */

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  set_special_flags2(player, card, SF2_X_SPELL);
	  if (played_for_free(player, card) || is_token(player, card))
		inst->info_slot = 0;
	  else
		inst->info_slot = charge_mana_for_double_x(player, COLOR_COLORLESS) / 2;
	}

  enters_the_battlefield_with_counters(player, card, event, COUNTER_ISOLATION, inst->info_slot);

  int isolation;
  if (comes_into_play(player, card, event) && (isolation = count_counters(player, card, COUNTER_ISOLATION)) > 0)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.illegal_type = TYPE_LAND;
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 2;

	  int i, count = pick_up_to_n_targets_noload(&td, "Select target nonland permanent an opponent controls.", isolation);
	  for (i = 0; i < count; ++i)
		obliviation(player, card, inst->targets[i].player, inst->targets[i].card);
	}

  return_from_oblivion(player, card, event);

  return global_enchantment(player, card, event);
}

int card_retreat_to_emeria(int player, int card, event_t event)
{
  /* Retreat to Emeria	|3|W	0x200deeb
   * Enchantment
   * Landfall - Whenever a land enters the battlefield under your control, choose one -
   * * Put a 1/1 |Swhite Kor Ally creature token onto the battlefield.
   * * Creatures you control get +1/+1 until end of turn. */

  if (landfall(player, card, event))
	{
	  if (DIALOG(player, card, EVENT_ACTIVATE,
				 DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL,
				 "Kor Ally token",	1,	3,
				 "+1/+1",			1,	creature_count[player]) == 1)
		generate_token_by_id(player, card, CARD_ID_KOR_ALLY);
	  else
		pump_creatures_until_eot_merge_pt(player, card, player, 1,1, NULL);
	}

  return global_enchantment(player, card, event);
}

int card_roils_retribution(int player, int card, event_t event)
{
  /* Roil's Retribution	|3|W|W	0x200def0
   * Instant
   * ~ deals 5 damage divided as you choose among any number of target attacking or blocking creatures. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_state = TARGET_STATE_IN_COMBAT;

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);

	  int i;
	  for (i = 0; i < 5; ++i)
		if (new_pick_target(&td, "TARGET_ATTACKING_BLOCKING_CREATURE", -1, 1))
		  add_state(inst->targets[i].player, inst->targets[i].card, STATE_TARGETTED);
		else
		  break;

	  for (i = 0; i < inst->number_of_targets; ++i)
		remove_state(inst->targets[i].player, inst->targets[i].card, STATE_TARGETTED);

	  if (cancel == 1)
		inst->number_of_targets = 0;
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  divide_damage(player, card, &td);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_serene_steward(int player, int card, event_t event)
{
  /* Serene Steward	|1|W	0x200def5
   * Creature - Human Cleric Ally 2/2
   * Whenever you gain life, you may pay |W. If you do, put a +1/+1 counter on target creature. */

  if (trigger_gain_life(player, card, event, player, RESOLVE_TRIGGER_AI(player))
	  && has_mana(player, COLOR_WHITE, 1))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  // charge mana last so because it isn't properly returned if cancel during target selection
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE") && charge_mana_while_resolving(player, card, event, player, COLOR_WHITE, 1))
		add_1_1_counter(inst->targets[0].player, inst->targets[0].card);
	}

  return 0;
}

/* Shadow Glider	|2|W	=>vanilla
 * Creature - Kor Soldier 2/2
 * Flying */

int card_sheer_drop(int player, int card, event_t event)
{
  /* Sheer Drop	|2|W	0x200defa
   * Sorcery
   * Destroy target tapped creature.
   * Awaken 3-|5|W */

  if (!IS_AWAKEN_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_state = TARGET_STATE_TAPPED;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		}
	  if (awaken_validate(player, card, 1))
		awaken_resolve(player, card, 1, 3);
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken_tgt(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", MANACOST_XW(5,1));
}

/* Smite the Monstrous	|3|W	=>innistrad.c:card_smite_the_monstrous
 * Instant
 * Destroy target creature with power 4 or greater. */

/* Stasis Snare	|1|W|W	=>m14.c:card_banisher_priest
 * Enchantment
 * Flash
 * When ~ enters the battlefield, exile target creature an opponent controls until ~ leaves the battlefield. */

int card_stone_haven_medic(int player, int card, event_t event)
{
  /* Stone Haven Medic	|1|W	0x200deff
   * Creature - Kor Cleric 1/3
   * |W, |T: You gain 1 life. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	gain_life(player, 1);

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_W(1), 0, NULL, NULL);
}

int card_tandem_tactics(int player, int card, event_t event)
{
  /* Tandem Tactics	|1|W	0x200df04
   * Instant
   * Up to two target creatures each get +1/+2 until end of turn. You gain 2 life. */

  if (event == EVENT_CHECK_PUMP && has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id))
	{
	  pumpable_power[player] += 1;
	  pumpable_toughness[player] += 2;
	}

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int i, num_valid = 0;
	  for (i = 0; i < inst->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  {
			++num_valid;
			pump_until_eot(player, card, inst->targets[i].player, inst->targets[i].card, 1,2);
		  }

	  if (num_valid > 0 || inst->number_of_targets == 0)
		gain_life(player, 2);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_unified_front(int player, int card, event_t event)
{
  /* Unified Front	|3|W	0x200df09
   * Sorcery
   * Converge - Put a 1/1 |Swhite Kor Ally creature token onto the battlefield for each color of mana spent to cast ~. */

  converge(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  generate_tokens_by_id(player, card, CARD_ID_KOR_ALLY, get_card_instance(player, card)->info_slot);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/*** Blue ***/

int card_adverse_conditions(int player, int card, event_t event)
{
  /* Adverse Conditions	|3|U	0x200df0e
   * Instant
   * Devoid
   * Tap up to two target creatures. Those creatures don't untap during their controller's next untap step. Put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool." */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int i;
	  for (i = 0; i < inst->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  effect_frost_titan(player, card, inst->targets[0].player, inst->targets[0].card);

	  generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_benthic_infiltrator(int player, int card, event_t event)
{
  /* Benthic Infiltrator	|2|U	0x200df13
   * Creature - Eldrazi Drone 1/4
   * Devoid
   * Ingest
   * ~ can't be blocked. */

  ingest(player, card, event);
  unblockable(player, card, event);
  return 0;
}

int card_cryptic_cruiser(int player, int card, event_t event)
{
  /* Cryptic Cruiser	|3|U	0x200df18
   * Creature - Eldrazi Processor 3/3
   * Devoid
   * |2|U, Put a card an opponent owns from exile into that player's graveyard: Tap target creature. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  tap_card(inst->targets[0].player, inst->targets[0].card);
	}

  return process_as_cost(player, 1, event,
						 generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XU(2,1), 0, &td, "TARGET_CREATURE"));
}

int card_drowner_of_hope(int player, int card, event_t event)
{
  /* Drowner of Hope	|5|U	0x200df1d
   * Creature - Eldrazi 5/5
   * Devoid
   * When ~ enters the battlefield, put two 1/1 colorless Eldrazi Scion creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your mana pool."
   * Sacrifice an Eldrazi Scion: Tap target creature. */

  if (comes_into_play(player, card, event))
	generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SCION, 2);

  if (!IS_GAA_EVENT(event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_PERMANENT, "Select an Eldrazi Scion to sacrifice.");
  test.subtype = SUBTYPE_ELDRAZI;
  test.sub2 = SUBTYPE_SCION;
  test.subtype_flag = F2_MULTISUBTYPE_ALL;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_CAN_ACTIVATE)
	return (generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE")
			&& new_can_sacrifice_as_cost(player, card, &test));

  if (event == EVENT_ACTIVATE)
	{
	  if (!charge_mana_for_activated_ability(player, card, MANACOST0))
		{
		  cancel = 1;
		  return 0;
		}
	  int sac = new_sacrifice(player, card, player, SAC_AS_COST|SAC_JUST_MARK|SAC_RETURN_CHOICE, &test);
	  if (!sac)
		{
		  cancel = 1;
		  return 0;
		}
	  /* If the scion was the only targetable available target, make sure it still can be targeted.  What a wreck.  This works just fine with the *actual*
	   * conventions of the game, where targets are always chosen first, rather than the Manalink conventions where costs are always paid first. */
	  remove_state(BYTE2(sac), BYTE3(sac), STATE_CANNOT_TARGET);
	  add_state(BYTE2(sac), BYTE3(sac), STATE_TARGETTED);
	  get_card_instance(player, card)->number_of_targets = 0;
	  if (!pick_target(&td, "TARGET_CREATURE"))
		{
		  remove_state(BYTE2(sac), BYTE3(sac), STATE_TARGETTED);
		  cancel = 1;
		  return 0;
		}
	  else
		kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
	}

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  tap_card(inst->targets[0].player, inst->targets[0].card);
	}

  return 0;
}

int card_eldrazi_skyspawner(int player, int card, event_t event)
{
  /* Eldrazi Skyspawner	|2|U	0x200df22
   * Creature - Eldrazi Drone 2/1
   * Devoid
   * Flying
   * When ~ enters the battlefield, put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool." */

  if (comes_into_play(player, card, event))
	generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);

  return 0;
}

int card_horribly_awry(int player, int card, event_t event)
{
  /* Horribly Awry	|1|U	0x200df27
   * Instant
   * Devoid
   * Counter target creature spell with converted mana cost 4 or less. If that spell is countered this way, exile it instead of putting it into its owner's graveyard. */

  target_definition_t td;
  counterspell_target_definition(player, card, &td, TYPE_CREATURE);
  td.extra = 4;
  td.special |= TARGET_SPECIAL_CMC_LESSER_OR_EQUAL;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (counterspell_validate(player, card, &td, 0))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  set_flags_when_spell_is_countered(player, card, inst->targets[0].player, inst->targets[0].card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_REMOVE);
		}
	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}

  return counterspell(player, card, event, &td, 0);
}

/* Incubator Drone	|3|U	=>card_eldrazi_skyspawner
 * Creature - Eldrazi Drone 2/3
 * Devoid
 * When ~ enters the battlefield, put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool." */

int card_mist_intruder(int player, int card, event_t event)
{
  /* Mist Intruder	|1|U	0x200df2c
   * Creature - Eldrazi Drone 1/2
   * Devoid
   * Flying
   * Ingest */
  ingest(player, card, event);
  return 0;
}

int card_murk_strider(int player, int card, event_t event)
{
  /* Murk Strider	|3|U	0x200df31
   * Creature - Eldrazi Processor 3/2
   * Devoid
   * When ~ enters the battlefield, you may put a card an opponent owns from exile into that player's graveyard. If you do, return target creature to its owner's hand. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE") && process(player, 1))
		bounce_permanent(inst->targets[0].player, inst->targets[0].card);
	}

  return 0;
}

int card_oracle_of_dust(int player, int card, event_t event)
{
  /* Oracle of Dust	|4|U	0x200df36
   * Creature - Eldrazi Processor 3/5
   * Devoid
   * |2, Put a card an opponent owns from exile into that player's graveyard: Draw a card, then discard a card. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  draw_a_card(player);
	  discard(player, 0, player);
	}

  return process_as_cost(player, 1, event,
						 generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL));
}

int card_ruination_guide(int player, int card, event_t event)
{
  /* Ruination Guide	|2|U	0x200df3b
   * Creature - Eldrazi Drone 3/2
   * Devoid
   * Ingest
   * Other colorless creatures you control get +1/+0. */

  ingest(player, card, event);

  if (event == EVENT_POWER
	  && affected_card_controller == player && affected_card != card
	  && in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card)
	  && !(get_color(affected_card_controller, affected_card) & COLOR_TEST_ANY_COLORED))
	event_result += 1;

  return 0;
}

int card_salvage_drone(int player, int card, event_t event)
{
  /* Salvage Drone	|U	0x200df40
   * Creature - Eldrazi Drone 1/1
   * Devoid
   * Ingest
   * When ~ dies, you may draw a card. If you do, discard a card. */

  ingest(player, card, event);

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player))
	  && can_draw_cards_as_cost(player, 1))
	{
	  draw_a_card(player);
	  discard(player, 0, player);
	}

  return 0;
}

int card_spell_shrivel(int player, int card, event_t event)
{
  /* Spell Shrivel	|2|U	0x200df45
   * Instant
   * Devoid
   * Counter target spell unless its controller pays |4. If that spell is countered this way, exile it instead of putting it into its owner's graveyard. */

  target_definition_t td;
  counterspell_target_definition(player, card, &td, 0);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  // That - sign is there for a reason
	  counterspell_resolve_unless_pay_x(player, card, NULL, -KILL_REMOVE, 4);
	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}
  else
	return counterspell(player, card, event, NULL, 0);
}

int card_tide_drifter(int player, int card, event_t event)
{
  /* Tide Drifter	|1|U	0x200df4a
   * Creature - Eldrazi Drone 0/5
   * Devoid
   * Other colorless creatures you control get +0/+1. */

  if (event == EVENT_TOUGHNESS
	  && affected_card_controller == player && affected_card != card
	  && in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card)
	  && !(get_color(affected_card_controller, affected_card) & COLOR_TEST_ANY_COLORED))
	event_result += 1;

  return 0;
}

int card_ulamogs_reclaimer(int player, int card, event_t event)
{
  /* Ulamog's Reclaimer	|4|U	0x200df4f
   * Creature - Eldrazi Processor 2/5
   * Devoid
   * When ~ enters the battlefield, you may put a card an opponent owns from exile into that player's graveyard. If you do, return target instant or sorcery card from your graveyard to your hand. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player))
	  && count_graveyard_by_type(player, TYPE_INSTANT|TYPE_SORCERY) > 0 && !graveyard_has_shroud(player)
	  && process(player, 1))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_INSTANT|TYPE_SORCERY, "Select target instant or sorcery card.");
	  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &test);
	}

  return 0;
}

/* Anticipate	|1|U	=>dragons_of_tarkir.c:card_anticipate
 * Instant
 * Look at the top three cards of your library. Put one of them into your hand and the rest on the bottom of your library in any order. */

int card_brilliant_spectrum(int player, int card, event_t event)
{
  /* Brilliant Spectrum	|3|U	0x200df54
   * Sorcery
   * Converge - Draw X cards, where X is the number of colors of mana spent to cast ~. Then discard two cards. */

  converge(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  draw_cards(player, get_card_instance(player, card)->info_slot);
	  new_multidiscard(player, 2, 0, player);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Cloud Manta	|3|U	=>vanilla
 * Creature - Fish 3/2
 * Flying */

int card_clutch_of_currents(int player, int card, event_t event)
{
  /* Clutch of Currents	|U	0x200df59
   * Sorcery
   * Return target creature to its owner's hand.
   * Awaken 3-|4|U */

  if (!IS_AWAKEN_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  bounce_permanent(inst->targets[0].player, inst->targets[0].card);
		}
	  if (awaken_validate(player, card, 1))
		awaken_resolve(player, card, 1, 3);
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken_tgt(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", MANACOST_XU(4,1));
}

int card_coastal_discovery(int player, int card, event_t event)
{
  /* Coastal Discovery	|3|U	0x200df5e
   * Sorcery
   * Draw two cards.
   * Awaken 4-|5|U */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (awaken_validate(player, card, 0))
		{
		  draw_cards(player, 2);
		  awaken_resolve(player, card, 0, 4);
		}
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken(player, card, event, MANACOST_XU(5,1));
}

int card_coralhelm_guide(int player, int card, event_t event)
{
  /* Coralhelm Guide	|1|U	0x200df63
   * Creature - Merfolk Scout Ally 2/1
   * |4|U: Target creature can't be blocked this turn. */

  return vanilla_creature_pumper(player, card, event, MANACOST_XU(4,1), 0, 0,0, 0,SP_KEYWORD_UNBLOCKABLE, NULL);
}

/* Dampening Pulse	|3|U	=>conflux.c:card_cumber_stone
 * Enchantment
 * Creatures your opponents control get -1/-0. */

/* Dispel	|U	=>legends.c:card_flash_counter
 * Instant
 * Counter target instant spell. */

int card_exert_influence(int player, int card, event_t event)
{
  /* Exert Influence	|4|U	0x200df68
   * Sorcery
   * Converge - Gain control of target creature if its power is less than or equal to the number of colors of mana spent to cast ~. */

  converge(player, card, event);

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (valid_target(&td) && get_power(inst->targets[0].player, inst->targets[0].card) <= inst->info_slot)
		gain_control(player, card, inst->targets[0].player, inst->targets[0].card);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_guardian_of_tazeem(int player, int card, event_t event)
{
  /* Guardian of Tazeem	|3|U|U	0x200df6d
   * Creature - Sphinx 4/5
   * Flying
   * Landfall - Whenever a land enters the battlefield under your control, tap target creature an opponent controls. If that land is |Han Island, that creature doesn't untap during its controller's next untap step. */

  if (landfall(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;

	  int island = has_subtype(trigger_cause_controller, trigger_cause, get_hacked_subtype(player, card, SUBTYPE_ISLAND));

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		{
		  if (island)
			effect_frost_titan(player, card, inst->targets[0].player, inst->targets[0].card);
		  else
			tap_card(inst->targets[0].player, inst->targets[0].card);
		}
	}

  return 0;
}

static int test_has_awaken(int iid, int unused, int player, int card)
{
  typedef int (*WhyIsItStoredAsAnUint32T)(int, int, event_t);
  return call_card_fn((WhyIsItStoredAsAnUint32T)(cards_data[iid].code_pointer),
					  get_card_instance(0, 0),	// Since broken asm cards look at it, and they haven't necessarily all been removed
					  0, 0, EVENT_QUERY_AWAKEN);
}
int card_halimar_tidecaller(int player, int card, event_t event)
{
  /* Halimar Tidecaller	|2|U	0x200df72
   * Creature - Human Wizard Ally 2/3
   * When ~ enters the battlefield, you may return target card with awaken from your graveyard to your hand.
   * Land creatures you control have flying. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  /* Awaken is defined in rule 702.112a to only be on instant and sorcery cards, so take advantage of that to limit the number of cards to send
	   * EVENT_QUERY_AWAKEN to directly.  If later sets redefine it to work on permanents, this'll need revisiting. */
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_INSTANT|TYPE_SORCERY, "Select target card with awaken.");
	  test.special_selection_function = &test_has_awaken;

	  if (new_special_count_grave(player, &test) > 0 && !graveyard_has_shroud(player))
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

  if (event == EVENT_ABILITIES
	  && affected_card_controller == player
	  && in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card)
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE) && is_what(affected_card_controller, affected_card, TYPE_LAND))
	event_result |= KEYWORD_FLYING;

  return 0;
}

int card_part_the_waterveil(int player, int card, event_t event)
{
  /* Part the Waterveil	|4|U|U	0x200df77
   * Sorcery
   * Take an extra turn after this one. Exile ~.
   * Awaken 6-|6|U|U|U */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (awaken_validate(player, card, 0))
		{
		  alternate_legacy_text(1, player, time_walk_effect(player, card));
		  alternate_legacy_text(2, player, awaken_resolve(player, card, 0, 6));
		  kill_card(player, card, KILL_REMOVE);	// Should, stricly speaking, happen before the awaken effect
		}
	  else
		kill_card(player, card, KILL_DESTROY);
	}
  return awaken(player, card, event, MANACOST_XU(6,3));
}

int card_prism_array(int player, int card, event_t event)
{
  /* Prism Array	|4|U	0x200df7c
   * Enchantment
   * Converge - ~ enters the battlefield with a crystal counter on it for each color of mana spent to cast it.
   * Remove a crystal counter from ~: Tap target creature.
   * |W|U|B|R|G: Scry 3. */

  card_instance_t* inst = get_card_instance(player, card);

  converge(player, card, event);

  enters_the_battlefield_with_counters(player, card, event, COUNTER_CRYSTAL, inst->info_slot);

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  /* The card was gutted to unusability during development, and the AI's not going to play this card well no matter what we do (even before considering that
	   * it makes no use of scry), so I'm not going to bother with trying to encourage it to pick untapped creatures or whatever. */

	  enum
	  {
		CHOICE_TAP = 1,
		CHOICE_SCRY
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Tap creature",	count_counters(player, card, COUNTER_CRYSTAL) > 0,	5,	DLG_MANA(MANACOST0), DLG_TARGET(&td, "TARGET_CREATURE"),
						"Scry 3",		1,													1,	DLG_MANA6(0,1,1,1,1,1));
	  // The AI doesn't get any direct benefit from scrying, but it might trigger Flamespeaker Adept or Knowledge and Power.

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE && choice == CHOICE_TAP && cancel != 1)
		remove_counter(player, card, COUNTER_CRYSTAL);
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_TAP:
			  if (valid_target(&td))
				tap_card(inst->targets[0].player, inst->targets[0].card);
			  break;

			case CHOICE_SCRY:
			  scry(player, 3);
			  break;
		  }
	}

  return global_enchantment(player, card, event);
}

int card_retreat_to_coralhelm(int player, int card, event_t event)
{
  /* Retreat to Coralhelm	|2|U	0x200df81
   * Enchantment
   * Landfall - Whenever a land enters the battlefield under your control, choose one -
   * * You may tap or untap target creature.
   * * Scry 1. */

  if (landfall(player, card, event))
	{
	  /* The way it's supposed to happen:
	   * 1. Land enters bf.
	   * 2. Next time a player gets priority, controller chooses whether to scry, or to tap/untap, and if the latter, he picks the target creature.  Trigger goes on the stack.
	   * 3. Players can put more effects on the stack.
	   * 4. Triggered ability resolves.  If scry was chosen, controller scries; otherwise, he chooses whether to tap, untap, or do nothing.
	   *
	   * Since there's no step 3 in Manalink, the whole is almost equivalent to immediately choosing whether to tap, to untap, to scry, or to do nothing all
	   * during step 2, and that would be a better interface besides.  However, if there's not targettable creature, you must choose to scry.  So the clumsy
	   * version wins. */

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = ANYBODY;

	  if (DIALOG(player, card, EVENT_ACTIVATE,
				 DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL,
				 "Tap or untap",	1,	5,	DLG_TARGET(&td, "TARGET_CREATURE"),
				 "Scry 1",			1,	1) == 1)
		twiddle(player, card, 0);
	  else
		scry(player, 1);
	}

  return global_enchantment(player, card, event);
}

int card_roilmages_trick(int player, int card, event_t event)
{
  /* Roilmage's Trick	|3|U	0x200df86
   * Instant
   * Converge - Creatures your opponents control get -X/-0 until end of turn, where X is the number of colors of mana spent to cast ~.
   * Draw a card. */

  converge(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  // Merging the effects won't do anything, but this lets the effect cards show the power mod instead of just "-X/-0".
	  pump_creatures_until_eot_merge_pt(player, card, player, -inst->info_slot,0, NULL);
	  draw_a_card(player);
	  kill_card(player, card, KILL_DESTROY);
	}
  return basic_spell(player, card, event);
}

int card_rush_of_ice(int player, int card, event_t event)
{
  /* Rush of Ice	|U	0x200df8b
   * Sorcery
   * Tap target creature. It doesn't untap during its controller's next untap step.
   * Awaken 3-|4|U */

  if (!IS_AWAKEN_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  alternate_legacy_text(1, player, effect_frost_titan(player, card, inst->targets[0].player, inst->targets[0].card));
		}
	  if (awaken_validate(player, card, 1))
		alternate_legacy_text(2, player, awaken_resolve(player, card, 1, 3));
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken_tgt(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", MANACOST_XU(4,1));
}

int card_scatter_to_the_winds(int player, int card, event_t event)
{
  /* Scatter to the Winds	|1|U|U	0x200e2ba
   * Instant
   * Counter target spell.
   * Awaken 3-|4|U|U */

  if (!IS_AWAKEN_EVENT(player, card, event))
	return counterspell(player, card, event, NULL, 0);

  target_definition_t td;
  counterspell_target_definition(player, card, &td, 0);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (counterspell_validate(player, card, &td, 0))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  real_counter_a_spell(player, card, inst->targets[0].player, inst->targets[0].card);
		}

	  if (awaken_validate(player, card, 1))
		awaken_resolve(player, card, 1, 3);

	  kill_card(player, card, KILL_DESTROY);
	}

  return awaken_tgt(player, card, event, GS_COUNTERSPELL, &td, NULL, MANACOST_XU(4,2));
}

int card_tightening_coils(int player, int card, event_t event)
{
  /* Tightening Coils	|1|U	0x200df90
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets -6/-0 and loses flying. */

  if (event == EVENT_ABILITIES)
	{
	  card_instance_t* inst = in_play(player, card);
	  if (inst && affect_me(inst->damage_target_player, inst->damage_target_card))
		event_result &= ~KEYWORD_FLYING;
	}

  return generic_aura(player, card, event, 1-player, -6,0, 0,0, 0,0,0);
}

int card_ugins_insight(int player, int card, event_t event)
{
  /* Ugin's Insight	|3|U|U	0x200df95
   * Sorcery
   * Scry X, where X is the highest converted mana cost among permanents you control, then draw three cards. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  scry(player, get_highest_cmc(player, TYPE_PERMANENT));
	  draw_cards(player, 3);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Wave-Wing Elemental	|5|U	=>zendikar.c:card_steppe_lynx
 * Creature - Elemental 3/4
 * Flying
 * Landfall - Whenever a land enters the battlefield under your control, ~ gets +2/+2 until end of turn. */

int card_windrider_patrol(int player, int card, event_t event)
{
  /* Windrider Patrol	|3|U|U	0x200df9a
   * Creature - Merfolk Wizard 4/3
   * Flying
   * Whenever ~ deals combat damage to a player, scry 2. */

  int packets = has_combat_damage_been_inflicted_to_a_player(player, card, event);
  for (; packets > 0; --packets)
	scry(player, 2);

  return 0;
}

/*** Black ***/

int card_complete_disregard(int player, int card, event_t event)
{
  /* Complete Disregard	|2|B	0x200df9f
   * Instant
   * Devoid
   * Exile target creature with power 3 or less. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.power_requirement = 3 | TARGET_PT_LESSER_OR_EQUAL;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_REMOVE);
		}
	  kill_card(player, card, KILL_DESTROY);
	}
  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/* Culling Drone	|1|B	=>card_mist_intruder
 * Creature - Eldrazi Drone 2/2
 * Devoid
 * Ingest */

int card_dominator_drone(int player, int card, event_t event)
{
  /* Dominator Drone	|2|B	0x200dfa4
   * Creature - Eldrazi Drone 3/2
   * Devoid
   * Ingest
   * When ~ enters the battlefield, if you control another colorless creature, each opponent loses 2 life. */

  ingest(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.color = COLOR_TEST_ANY_COLORED;
	  test.color_flag = DOESNT_MATCH;
	  test.not_me = 1;

	  if (check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test))
		lose_life(1-player, 2);
	}

  return 0;
}

int card_grave_birthing(int player, int card, event_t event)
{
  /* Grave Birthing	|2|B	0x200dfa9
   * Instant
   * Devoid
   * Target opponent exiles a card from his or her graveyard. You put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool."
   * Draw a card. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.allowed_controller = 1-player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  test_definition_t test;
		  default_test_definition(&test, TYPE_ANY);

		  int oppo = get_card_instance(player, card)->targets[0].player;
		  if (get_grave(oppo)[0] != -1)
			new_global_tutor(oppo, oppo, TUTOR_FROM_GRAVE_NOTARGET, TUTOR_RFG, 1, AI_MIN_VALUE, &test);

		  generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);
		  draw_a_card(player);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_OPPONENT", 1, NULL);
}

int card_grip_of_desolation(int player, int card, event_t event)
{
  /* Grip of Desolation	|4|B|B	0x200dfae
   * Instant
   * Devoid
   * Exile target creature and target land. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td_creature;
  default_target_definition(player, card, &td_creature, TYPE_CREATURE);

  target_definition_t td_land;
  default_target_definition(player, card, &td_land, TYPE_LAND);

  if (event == EVENT_CAN_CAST)
	return can_target(&td_creature) && can_target(&td_land);

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && !(pick_target(&td_creature, "TARGET_CREATURE")
		   && new_pick_target(&td_land, "TARGET_LAND", 1, 1)))
	get_card_instance(player, card)->number_of_targets = 0;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);

	  if (valid_target(&td_creature))
		kill_card(inst->targets[0].player, inst->targets[0].card, KILL_REMOVE);

	  if (validate_target(player, card, &td_land, 1))
		kill_card(inst->targets[1].player, inst->targets[1].card, KILL_REMOVE);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_mind_raker(int player, int card, event_t event)
{
  /* Mind Raker	|3|B	0x200dfb3
   * Creature - Eldrazi Processor 3/3
   * Devoid
   * When ~ enters the battlefield, you may put a card an opponent owns from exile into that player's graveyard. If you do, each opponent discards a card. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player))
	  && process(player, 1))
	discard(1-player, 0, player);

  return 0;
}

int card_silent_skimmer(int player, int card, event_t event)
{
  /* Silent Skimmer	|3|B	0x200dfb8
   * Creature - Eldrazi Drone 0/4
   * Devoid
   * Flying
   * Whenever ~ attacks, defending player loses 2 life. */

  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	lose_life(1-current_turn, 2);

  return 0;
}

int card_skitterskin(int player, int card, event_t event)
{
  /* Skitterskin	|3|B	0x200dfbd
   * Creature - Eldrazi Drone 4/3
   * Devoid
   * ~ can't block.
   * |1|B: Regenerate ~. Activate this ability only if you control another colorless creature. */

  cannot_block(player, card, event);

  int rval = regeneration(player, card, event, MANACOST_XB(1,1));
  if (rval && event == EVENT_CAN_ACTIVATE)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.color = COLOR_TEST_ANY_COLORED;
	  test.color_flag = DOESNT_MATCH;
	  test.not_me = 1;

	  if (!check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test))
		return 0;
	}
  return rval;
}

int card_sludge_crawler(int player, int card, event_t event)
{
  /* Sludge Crawler	|B	0x200dfc2
   * Creature - Eldrazi Drone 1/1
   * Devoid
   * Ingest
   * |2: ~ gets +1/+1 until end of turn. */

  ingest(player, card, event);

  return generic_shade_merge_pt(player, card, event, 0, MANACOST_X(2), 1,1);
}

int card_smothering_abomination(int player, int card, event_t event)
{
  /* Smothering Abomination	|2|B|B	0x200dfc7
   * Creature - Eldrazi 4/3
   * Devoid
   * Flying
   * At the beginning of your upkeep, sacrifice a creature.
   * Whenever you sacrifice a creature, draw a card. */

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	controller_sacrifices_a_permanent(player, card, TYPE_CREATURE, SAC_NO_CANCEL);

  if (whenever_a_player_sacrifices_a_permanent(player, card, event, player, TYPE_CREATURE, RESOLVE_TRIGGER_MANDATORY))
	draw_a_card(player);

  return 0;
}

int card_swarm_surge(int player, int card, event_t event)
{
  /* Swarm Surge	|2|B	0x200dfcc
   * Sorcery
   * Devoid
   * Creatures you control get +2/+0 until end of turn. Colorless creatures you control also gain first strike until end of turn. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && is_what(player, c, TYPE_CREATURE))
		  {
			if (get_color(player, c) & COLOR_TEST_ANY_COLORED)
			  alternate_legacy_text(1, player, pump_until_eot(player, card, player, c, 2,0));
			else
			  alternate_legacy_text(2, player, pump_ability_until_eot(player, card, player, c, 2,0, KEYWORD_FIRST_STRIKE,0));
		  }
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_transgress_the_mind(int player, int card, event_t event)
{
  /* Transgress the Mind	|1|B	0x200dfd1
   * Sorcery
   * Devoid
   * Target player reveals his or her hand. You choose a card from it with converted mana cost 3 or greater and exile that card. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  ec_definition_t coerce;
		  default_ec_definition(get_card_instance(player, card)->targets[0].player, player, &coerce);
		  coerce.effect = EC_RFG;

		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_ANY, "Select a card with converted mana cost 3 or greater.");
		  test.cmc = 2;
		  test.cmc_flag = F5_CMC_GREATER_THAN_VALUE;
		  new_effect_coercion(&coerce, &test);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_wasteland_strangler(int player, int card, event_t event)
{
  /* Wasteland Strangler	|2|B	0x200dfd6
   * Creature - Eldrazi Processor 3/2
   * Devoid
   * When ~ enters the battlefield, you may put a card an opponent owns from exile into that player's graveyard. If you do, target creature gets -3/-3 until end of turn. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE") && process(player, 1))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  pump_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, -3,-3);
		}
	}

  return 0;
}

/* Altar's Reap	|1|B	=>innistrad.c:card_altars_reap
 * Instant
 * As an additional cost to cast ~, sacrifice a creature.
 * Draw two cards. */

int card_bloodbond_vampire(int player, int card, event_t event)
{
  /* Bloodbond Vampire	|2|B|B	0x200dfdb
   * Creature - Vampire Shaman Ally 3/3
   * Whenever you gain life, put a +1/+1 counter on ~. */

  if (trigger_gain_life(player, card, event, player, RESOLVE_TRIGGER_MANDATORY))
	add_1_1_counter(player, card);

  return 0;
}

/* Bone Splinters	|B	=>shards_of_alara.c:card_bone_splinters
 * Sorcery
 * As an additional cost to cast ~, sacrifice a creature.
 * Destroy target creature. */

int card_carrier_thrall(int player, int card, event_t event)
{
  /* Carrier Thrall	|1|B	0x200dfe0
   * Creature - Vampire 2/1
   * When ~ dies, put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool." */

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);

  return 0;
}

/* Defiant Bloodlord	|5|B|B	=>vanilla with a horrid hack in gain_life() - it and Sanguine Bond could stand a rewrite with proper triggers
 * Creature - Vampire 4/5
 * Flying
 * Whenever you gain life, target opponent loses that much life. */

int card_demons_grasp(int player, int card, event_t event)
{
  /* Demon's Grasp	|4|B	0x200dfe5
   * Sorcery
   * Target creature gets -5/-5 until end of turn. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  return vanilla_pump(player, card, event, &td, -5,-5, 0,0);
}

int card_drana_liberator_of_malakir(int player, int card, event_t event)
{
  /* Drana, Liberator of Malakir	|1|B|B	0x200dfea
   * Legendary Creature - Vampire Ally 2/3
   * Flying, first strike
   * Whenever ~ deals combat damage to a player, put a +1/+1 counter on each attacking creature you control. */

  check_legend_rule(player, card, event);

  int packets = has_combat_damage_been_inflicted_to_a_player(player, card, event);
  if (packets > 0)
	{
	  char marked[151] = {0};
	  card_instance_t* inst;
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if ((inst = in_play(player, c)) && (inst->state & STATE_ATTACKING) && is_what(player, c, TYPE_CREATURE))
		  marked[c] = 1;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (marked[c] && in_play(player, c))
		  add_1_1_counter(player, packets);
	}

  return 0;
}

/* Dutiful Return	|3|B	=>mirrodin_besieged.c:card_morbid_plunder
 * Sorcery
 * Return up to two target creature cards from your graveyard to your hand. */

int card_geyserfield_stalker(int player, int card, event_t event)
{
  /* Geyserfield Stalker	|4|B	0x200dfef
   * Creature - Elemental 3/2
   * Menace
   * Landfall - Whenever a land enters the battlefield under your control, ~ gets +2/+2 until end of turn. */

  minimum_blockers(player, card, event, 2);

  if (landfall(player, card, event))
	pump_until_eot_merge_previous(player, card, player, card, 2,2);

  return 0;
}

int card_guul_draz_overseer(int player, int card, event_t event)
{
  /* Guul Draz Overseer	|4|B|B	0x200dff4
   * Creature - Vampire 3/4
   * Flying
   * Landfall - Whenever a land enters the battlefield under your control, other creatures you control get +1/+0 until end of turn. If that land is |Ha Swamp, those creatures get +2/+0 until end of turn instead. */

  if (landfall(player, card, event))
	{
	  int swamp = has_subtype(trigger_cause_controller, trigger_cause, get_hacked_subtype(player, card, SUBTYPE_SWAMP));

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  pump_creatures_until_eot_merge_pt(player, card, player, swamp ? 2 : 1, 0, &test);
	}

  return 0;
}

int card_hagra_sharpshooter(int player, int card, event_t event)
{
  /* Hagra Sharpshooter	|2|B	0x200dff9
   * Creature - Human Assassin Ally 2/2
   * |4|B: Target creature gets -1/-1 until end of turn. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  pump_until_eot_merge_previous(player, card, inst->targets[0].player, inst->targets[0].card, -1,-1);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XB(4,1), 0, &td, "TARGET_CREATURE");
}

int card_kalastria_healer(int player, int card, event_t event)
{
  /* Kalastria Healer	|1|B	0x200dffe
   * Creature - Vampire Cleric Ally 1/2
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, each opponent loses 1 life and you gain 1 life. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  lose_life(1-player, 1);
	  gain_life(player, 1);
	}

  return 0;
}

int card_kalastria_nightwatch(int player, int card, event_t event)
{
  /* Kalastria Nightwatch	|4|B	0x200e003
   * Creature - Vampire Warrior Ally 4/5
   * Whenever you gain life, ~ gains flying until end of turn. */

  if (trigger_gain_life(player, card, event, player, RESOLVE_TRIGGER_MANDATORY))
	pump_ability_until_eot_no_repeat(player, card, player, card, KEYWORD_FLYING,0);

  return 0;
}

int card_malakir_familiar(int player, int card, event_t event)
{
  /* Malakir Familiar	|2|B	0x200e008
   * Creature - Bat 2/1
   * Flying, deathtouch
   * Whenever you gain life, ~ gets +1/+1 until end of turn. */

  deathtouch(player, card, event);

  if (trigger_gain_life(player, card, event, player, RESOLVE_TRIGGER_MANDATORY))
	pump_until_eot_merge_previous(player, card, player, card, 1,1);

  return 0;
}

int card_mires_malice(int player, int card, event_t event)
{
  /* Mire's Malice	|3|B	0x200e00d
   * Sorcery
   * Target opponent discards two cards.
   * Awaken 3-|5|B */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, 0);
	  td.zone = TARGET_ZONE_PLAYERS;
	  td.allowed_controller = 1-player;

	  if (valid_target(&td))
		new_multidiscard(1-player, 2, 0, player);
	  if (awaken_validate(player, card, 1))
		awaken_resolve(player, card, 1, 3);
	  kill_card(player, card, KILL_DESTROY);
	}

  return awaken_tgt(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, NULL, NULL, MANACOST_XB(5,1));
}

int card_nirkana_assassin(int player, int card, event_t event)
{
  /* Nirkana Assassin	|2|B	0x200e012
   * Creature - Vampire Assassin Ally 2/3
   * Whenever you gain life, ~ gains deathtouch until end of turn. */

  if (trigger_gain_life(player, card, event, player, RESOLVE_TRIGGER_MANDATORY))
	pump_ability_until_eot_no_repeat(player, card, player, card, 0,SP_KEYWORD_DEATHTOUCH);

  return 0;
}

int card_ob_nixilis_reignited(int player, int card, event_t event)
{
  /* Ob Nixilis Reignited	|3|B|B	0x200e017
   * Planeswalker - Nixilis (5)
   * +1: You draw a card and you lose 1 life.
   * -3: Destroy target creature.
   * -8: Target opponent gets an emblem with "Whenever a player draws a card, you lose 2 life." */

  if (IS_ACTIVATING(event))
	{
	  target_definition_t td_creature;
	  default_target_definition(player, card, &td_creature, TYPE_CREATURE);

	  enum
	  {
		CHOICE_DRAW_LIFE = 1,
		CHOICE_DESTROY,
		CHOICE_EMBLEM
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Draw",				1,	3,	+1,
						"Destroy creature",	1,	1,	-3,	DLG_TARGET(&td_creature, "TARGET_CREATURE"),
						"Emblem",			opponent_is_valid_target(player, card),	20,	-8);

	  if (event == EVENT_CAN_ACTIVATE && !choice)
		return 0;
	  if (event == EVENT_ACTIVATE && choice == CHOICE_EMBLEM)	// set target manually instead of DLG_TARGET(), so the player isn't uselessly prompted
		target_opponent(player, card);
	  if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  switch (choice)
			{
			  case CHOICE_DRAW_LIFE:
				draw_a_card(player);
				lose_life(player, 1);
				break;

			  case CHOICE_DESTROY:
				kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
				break;

			  case CHOICE_EMBLEM:
				;token_generation_t token;
				default_token_definition(player, card, CARD_ID_OB_NIXILIS_REIGNITED_EMBLEM, &token);
				token.t_player = inst->targets[0].player;
				generate_token(&token);
				break;
			}
		}
	}

  return planeswalker(player, card, event, 5);
}

int card_ob_nixilis_reignited_emblem(int player, int card, event_t event)
{
  /* Ob Nixilis Reignited Emblem	""	0x200e01c
   * Emblem
   * Whenever a player draws a card, you lose 2 life. */

  if (card_drawn_trigger(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY))
	lose_life(player, 2);

  return 0;
}

int card_painful_truths(int player, int card, event_t event)
{
  /* Painful Truths	|2|B	0x200e021
   * Sorcery
   * Converge - You draw X cards and you lose X life, where X is the number of colors of mana spent to cast ~. */

  converge(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int x = get_card_instance(player, card)->info_slot;
	  draw_cards(player, x);
	  lose_life(player, x);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_retreat_to_hagra(int player, int card, event_t event)
{
  /* Retreat to Hagra	|2|B	0x200e026
   * Enchantment
   * Landfall - Whenever a land enters the battlefield under your control, choose one -
   * * Target creature gets +1/+0 and gains deathtouch until end of turn.
   * * Each opponent loses 1 life and you gain 1 life. */

  if (landfall(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;
	  td.allow_cancel = 0;

	  if (DIALOG(player, card, EVENT_ACTIVATE,
				 DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_AUTOCHOOSE_IF_1,
				 "+1/+0 and deathtouch",	1,	1,	DLG_TARGET(&td, "TARGET_CREATURE"),
				 "Drain life",				1,	1) == 1)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 1,0, 0,SP_KEYWORD_DEATHTOUCH);
		}
	  else
		{
		  lose_life(1-player, 1);
		  gain_life(player, 1);
		}
	}

  return global_enchantment(player, card, event);
}

int card_rising_miasma(int player, int card, event_t event)
{
  /* Rising Miasma	|3|B	0x200e02b
   * Sorcery
   * All creatures get -2/-2 until end of turn.
   * Awaken 3-|5|B|B */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (awaken_validate(player, card, 0))
		{
		  pump_creatures_until_eot(player, card, ANYBODY, 1, -2,-2, 0,0, NULL);
		  alternate_legacy_text(2, player, awaken_resolve(player, card, 0, 3));
		}
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken(player, card, event, MANACOST_XB(5,2));
}

int card_ruinous_path(int player, int card, event_t event)
{
  /* Ruinous Path	|1|B|B	0x200e030
   * Sorcery
   * Destroy target creature or planeswalker.
   * Awaken 4-|5|B|B */

  if (!IS_AWAKEN_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE|TARGET_TYPE_PLANESWALKER);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		}
	  if (awaken_validate(player, card, 1))
		awaken_resolve(player, card, 1, 4);
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken_tgt(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLANESWALKER", MANACOST_XB(5,2));
}

int card_vampiric_rites(int player, int card, event_t event)
{
  /* Vampiric Rites	|B	0x200e035
   * Enchantment
   * |1|B, Sacrifice a creature: You gain 1 life and draw a card. */

  if (!IS_GAA_EVENT(event))
	return global_enchantment(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  gain_life(player, 1);
	  draw_a_card(player);
	}

  return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST_XB(1,1), 0, NULL, NULL);
}

int card_voracious_null(int player, int card, event_t event)
{
  /* Voracious Null	|2|B	0x200e03a
   * Creature - Zombie 2/2
   * |1|B, Sacrifice another creature: Put two +1/+1 counters on ~. Activate this ability only any time you could cast a sorcery. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card))
		add_1_1_counters(inst->parent_controller, inst->parent_card, 2);
	}

  return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE|GAA_NOT_ME_AS_TARGET|GAA_CAN_SORCERY_BE_PLAYED,
								   MANACOST_XB(1,1), 0, NULL, NULL);
}

int card_zulaport_cutthroat(int player, int card, event_t event)
{
  /* Zulaport Cutthroat	|1|B	0x200e03f
   * Creature - Human Rogue Ally 1/1
   * Whenever ~ or another creature you control dies, each opponent loses 1 life and you gain 1 life. */

  /* This card's function is also that of its effect card.  That's perhaps a bit too clever for its own good.  Don't emulate unless the function does nothing
   * else except implement a "Whenever [this card] or [something else] dies, ..." trigger. */
  if (event == EVENT_GRAVEYARD_FROM_PLAY)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->internal_card_id != LEGACY_EFFECT_CUSTOM)
		{
		  count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);

		  if (affect_me(player, card))
			{
			  card_instance_t* legacy = get_card_instance(player, create_legacy_effect(player, card, card_zulaport_cutthroat));
			  legacy->targets[11].player = inst->targets[11].player;
			  inst->targets[1].player = CARD_ID_ZULAPORT_CUTTHROAT;	// prevent main card from triggering
			}
		}
	  else if (!affect_me(inst->damage_source_player, inst->damage_source_card))
		count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);
	}

  if (trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && affect_me(player, card) && player == reason_for_trigger_controller)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (event == EVENT_TRIGGER)
		{
		  if (inst->targets[11].player > 0	// deaths recorded
			  && inst->targets[1].player != CARD_ID_ZULAPORT_CUTTHROAT)	// hasn't handed off to an effect card
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  int num = MAX(0, inst->targets[11].player);
		  inst->targets[11].player = 0;
		  for (; num > 0; --num)	// so things triggering on losing or gaining life see multiple instances
			{
			  lose_life(1-player, 1);
			  gain_life(player, 1);
			}
		}
	  if (event == EVENT_END_TRIGGER
		  && get_card_instance(player, card)->internal_card_id == LEGACY_EFFECT_CUSTOM)
		kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}

/*** Red ***/

int card_barrage_tyrant(int player, int card, event_t event)
{
  /* Barrage Tyrant	|4|R	0x200e044
   * Creature - Eldrazi 5/3
   * Devoid
   * |2|R, Sacrifice another colorless creature: ~ deals damage equal to the sacrificed creature's power to target creature or player. */

  if (!IS_GAA_EVENT(event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select another colorless creature to sacrifice.");
  test.color = COLOR_TEST_ANY_COLORED;
  test.color_flag = DOESNT_MATCH;
  test.not_me = 1;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_CAN_ACTIVATE)
	return (generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(2,1), 0, &td, "TARGET_CREATURE_OR_PLANESWALKER")
			&& new_can_sacrifice_as_cost(player, card, &test));

  if (event == EVENT_ACTIVATE)
	{
	  if (!charge_mana_for_activated_ability(player, card, MANACOST_XR(2,1)))
		{
		  cancel = 1;
		  return 0;
		}
	  int sac = new_sacrifice(player, card, player, SAC_AS_COST|SAC_JUST_MARK|SAC_RETURN_CHOICE, &test);
	  if (!sac)
		{
		  cancel = 1;
		  return 0;
		}
	  /* If the sacrificed creature was the only available target, make sure it still can be targeted.  What a wreck.  This works just fine with the *actual*
	   * conventions of the game, where targets are always chosen first, rather than the Manalink conventions where costs are always paid first. */
	  remove_state(BYTE2(sac), BYTE3(sac), STATE_CANNOT_TARGET);
	  add_state(BYTE2(sac), BYTE3(sac), STATE_TARGETTED);
	  get_card_instance(player, card)->number_of_targets = 0;
	  if (!pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
		{
		  remove_state(BYTE2(sac), BYTE3(sac), STATE_TARGETTED);
		  cancel = 1;
		  return 0;
		}
	  else
		{
		  get_card_instance(player, card)->info_slot = get_power(BYTE2(sac), BYTE3(sac));
		  kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
	}

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	damage_target0(player, card, get_card_instance(player, card)->info_slot);

  return 0;
}

int card_crumble_to_dust(int player, int card, event_t event)
{
  /* Crumble to Dust	|3|R	0x200e049
   * Sorcery
   * Devoid
   * Exile target nonbasic land. Search its controller's graveyard, hand, and library for any number of cards with the same name as that land and exile them. Then that player shuffles his or her library. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);
  td.required_subtype = SUBTYPE_NONBASIC;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  int csvid = get_id(inst->targets[0].player, inst->targets[0].card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_REMOVE);
		  lobotomy_effect(player, 1-player, csvid, 0);
		}
	  kill_card(player, card, KILL_DESTROY);
	}
  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_NONBASIC_LAND", 1, NULL);
}

int card_kozileks_sentinel(int player, int card, event_t event)
{
  /* Kozilek's Sentinel	|1|R	0x200e04e
   * Creature - Eldrazi Drone 1/4
   * Devoid
   * Whenever you cast a colorless spell, ~ gets +1/+0 until end of turn. */

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, 0,0, 0,0, COLOR_TEST_ANY_COLORED,DOESNT_MATCH, 0,0, -1,0))
	pump_until_eot_merge_previous(player, card, player, card, 1,0);

  return 0;
}

int card_molten_nursery(int player, int card, event_t event)
{
  /* Molten Nursery	|2|R	0x200e053
   * Enchantment
   * Devoid
   * Whenever you cast a colorless spell, ~ deals 1 damage to target creature or player. */

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, 0,0, 0,0, COLOR_TEST_ANY_COLORED,DOESNT_MATCH, 0,0, -1,0))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	  get_card_instance(player, card)->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
		damage_target0(player, card, 1);
	}

  return global_enchantment(player, card, event);
}

int card_nettle_drone(int player, int card, event_t event)
{
  /* Nettle Drone	|2|R	0x200e058
   * Creature - Eldrazi Drone 3/1
   * Devoid
   * |T: ~ deals 1 damage to each opponent.
   * Whenever you cast a colorless spell, untap ~. */

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, 0,0, 0,0, COLOR_TEST_ANY_COLORED,DOESNT_MATCH, 0,0, -1,0))
	untap_card(player, card);

  if (event == EVENT_RESOLVE_ACTIVATION)
	damage_player(1-player, 1, player, card);

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_processor_assault(int player, int card, event_t event)
{
  /* Processor Assault	|1|R	0x200e05d
   * Sorcery
   * Devoid
   * As an additional cost to cast ~, put a card an opponent owns from exile into that player's graveyard.
   * ~ deals 5 damage to target creature. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, 5);
	  kill_card(player, card, KILL_DESTROY);
	}

  int rval = generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
  if (rval && event == EVENT_CAN_CAST && !can_process(player, 1))
	return 0;

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && !is_token(player, card) && !check_special_flags(player, card, SF_NOT_CAST)
	  && !process(player, 1))
	{
	  cancel = 1;
	  return 0;
	}
  return rval;
}

int card_serpentine_spike(int player, int card, event_t event)
{
  /* Serpentine Spike	|5|R|R	0x200e062
   * Sorcery
   * Devoid
   * ~ deals 2 damage to target creature, 3 damage to another target creature, and 4 damage to a third target creature. If a creature dealt damage this way would die this turn, exile it instead. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int i;
	  for (i = 0; i < 3; ++i)
		if (validate_target(player, card, &td, i))
		  {
			exile_if_would_be_put_into_graveyard(player, card, inst->targets[i].player, inst->targets[i].card, 1);
			damage_creature(inst->targets[i].player, inst->targets[i].card, i + 2, player, card);
		  }
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 3, NULL);
}

/* Touch of the Void	|2|R	=>champions_of_kamigawa.c:card_yamabushis_flame
 * Sorcery
 * Devoid
 * ~ deals 3 damage to target creature or player. If a creature dealt damage this way would die this turn, exile it instead. */

/* Turn Against	|4|R	=>m10.c:card_act_of_treason
 * Instant
 * Devoid
 * Gain control of target creature until end of turn. Untap that creature. It gains haste until end of turn. */

/* Vestige of Emrakul	|3|R	=>vanilla
 * Creature - Eldrazi Drone 3/4
 * Devoid
 * Trample */

int card_vile_aggregate(int player, int card, event_t event)
{
  /* Vile Aggregate	|2|R	0x200e067
   * Creature - Eldrazi Drone 100/5
   * Devoid
   * ~'s power is equal to the number of colorless creatures you control.
   * Trample
   * Ingest */

  ingest(player, card, event);

  if (event == EVENT_POWER && affect_me(player, card) && !is_humiliated(player, card) && player != -1)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.color = COLOR_TEST_ANY_COLORED;
	  test.color_flag = DOESNT_MATCH;

	  event_result += check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &test);
	}

  return 0;
}

int card_akoum_firebird(int player, int card, event_t event)
{
  /* Akoum Firebird	|2|R|R	0x200e06c
   * Creature - Phoenix 3/3
   * Flying, haste
   * ~ attacks each turn if able.
   * Landfall - Whenever a land enters the battlefield under your control, you may pay |4|R|R. If you do, return ~ from your graveyard to the battlefield. */

  // The graveyard-to-battlefield ability is handled with the usual hacks in the Rules Engine.
  haste(player, card, event);
  attack_if_able(player, card, event);
  return 0;
}

int card_akoum_hellkite(int player, int card, event_t event)
{
  /* Akoum Hellkite	|4|R|R	0x200e071
   * Creature - Dragon 4/4
   * Flying
   * Landfall - Whenever a land enters the battlefield under your control, ~ deals 1 damage to target creature or player. If that land is |Ha Mountain, ~ deals 2 damage to that creature or player instead. */

  if (landfall(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	  int mountain = has_subtype(trigger_cause_controller, trigger_cause, get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN));

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
		damage_target0(player, card, mountain ? 2 : 1);
	}

  return 0;
}

int card_akoum_stonewaker(int player, int card, event_t event)
{
  /* Akoum Stonewaker	|1|R	0x200e076
   * Creature - Human Shaman 2/1
   * Landfall - Whenever a land enters the battlefield under your control, you may pay |2|R. If you do, put a 3/1 |Sred Elemental creature token with trample and haste onto the battlefield. Exile that token at the beginning of the next end step. */

  if (landfall_mode(player, card, event, RESOLVE_TRIGGER_AI(player))
	  && has_mana_multi(player, MANACOST_XR(2,1))
	  && charge_mana_multi_while_resolving(player, card, event, player, MANACOST_XR(2,1)))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
	  token.pow = 3;
	  token.tou = 1;
	  token.special_infos = 66;
	  token.key_plus = KEYWORD_TRAMPLE;
	  token.s_key_plus = SP_KEYWORD_HASTE;
	  generate_token(&token);
	}

  return 0;
}

int card_belligerent_whiptail(int player, int card, event_t event)
{
  /* Belligerent Whiptail	|3|R	0x200e07b
   * Creature - Wurm 4/2
   * Landfall - Whenever a land enters the battlefield under your control, ~ gains first strike until end of turn. */

  if (landfall(player, card, event))
	pump_ability_until_eot_no_repeat(player, card, player, card, KEYWORD_FIRST_STRIKE,0);

  return 0;
}

int card_boiling_earth(int player, int card, event_t event)
{
  /* Boiling Earth	|1|R	0x200e080
   * Sorcery
   * ~ deals 1 damage to each creature your opponents control.
   * Awaken 4-|6|R */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (awaken_validate(player, card, 0))
		{
		  new_damage_all(player, card, 1-player, 1, 0, NULL);
		  awaken_resolve(player, card, 0, 4);
		}
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken(player, card, event, MANACOST_XR(6,1));
}

int card_chasm_guide(int player, int card, event_t event)
{
  /* Chasm Guide	|3|R	0x200e085
   * Creature - Goblin Scout Ally 3/2
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, creatures you control gain haste until end of turn. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_creatures_until_eot_no_repeat(player, card, player, 0, 0,SP_KEYWORD_HASTE, NULL);

  return 0;
}

/* Dragonmaster Outcast	|R	=>worldwake.c:card_dragonmaster_outcast
 * Creature - Human Shaman 1/1
 * At the beginning of your upkeep, if you control six or more lands, put a 5/5 |Sred Dragon creature token with flying onto the battlefield. */

int card_firemantle_mage(int player, int card, event_t event)
{
  /* Firemantle Mage	|2|R	0x200e08a
   * Creature - Human Shaman Ally 2/2
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, creatures you control gain menace until end of turn. */

  int c;
  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	for (c = 0; c < active_cards_count[player]; ++c)
	  if (in_play(player, c) && is_what(player, c, TYPE_CREATURE))
		create_targetted_legacy_effect_no_repeat(player, card, fx_menace, player, c);

  return 0;
}

/* Goblin War Paint	|1|R	=>zendikar.c:card_goblin_war_paint
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature gets +2/+2 and has haste. */

int card_lavastep_raider(int player, int card, event_t event)
{
  /* Lavastep Raider	|R	0x200e08f
   * Creature - Goblin Warrior 1/2
   * |2|R: ~ gets +2/+0 until end of turn. */

  return generic_shade_merge_pt(player, card, event, 0, MANACOST_XR(2,1), 2,0);
}

/* Makindi Sliderunner	|1|R	=>zendikar.c:card_hedron_scrabbler
 * Creature - Beast 2/1
 * Trample
 * Landfall - Whenever a land enters the battlefield under your control, ~ gets +1/+1 until end of turn. */

int card_ondu_champion(int player, int card, event_t event)
{
  /* Ondu Champion	|2|R|R	0x200e094
   * Creature - Minotaur Warrior Ally 4/3
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, creatures you control gain trample until end of turn. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_creatures_until_eot_no_repeat(player, card, player, 0, KEYWORD_TRAMPLE,0, NULL);

  return 0;
}

int card_outnumber(int player, int card, event_t event)
{
  /* Outnumber	|R	0x200e099
   * Instant
   * ~ deals damage to target creature equal to the number of creatures you control. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, count_subtype(player, TYPE_CREATURE, -1));
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_radiant_flames(int player, int card, event_t event)
{
  /* Radiant Flames	|2|R	0x200e09e
   * Sorcery
   * Converge - ~ deals X damage to each creature, where X is the number of colors of mana spent to cast ~. */

  converge(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  new_damage_all(player, card, ANYBODY, get_card_instance(player, card)->info_slot, 0, NULL);
	  kill_card(player, card, KILL_DESTROY);
	}
  return basic_spell(player, card, event);
}

int card_reckless_cohort(int player, int card, event_t event)
{
  /* Reckless Cohort	|1|R	0x200e0a3
   * Creature - Human Warrior Ally 2/2
   * ~ attacks each combat if able unless you control another Ally. */

  if (event == EVENT_MUST_ATTACK && current_turn == player)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "");
	  test.subtype = SUBTYPE_ALLY;
	  test.not_me = 1;

	  if (!check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test))
		attack_if_able(player, card, event);
	}

  return 0;
}

int card_retreat_to_valakut(int player, int card, event_t event)
{
  /* Retreat to Valakut	|2|R	0x200e0a8
   * Enchantment
   * Landfall - Whenever a land enters the battlefield under your control, choose one -
   * * Target creature gets +2/+0 until end of turn.
   * * Target creature can't block this turn. */

  if (landfall(player, card, event))
	{
	  target_definition_t td_pump;
	  default_target_definition(player, card, &td_pump, TYPE_CREATURE);
	  td_pump.preferred_controller = player;
	  td_pump.allow_cancel = 0;

	  target_definition_t td_blck;
	  default_target_definition(player, card, &td_blck, TYPE_CREATURE);
	  td_blck.allow_cancel = 0;

	  card_instance_t* inst = get_card_instance(player, card);
	  if (DIALOG(player, card, EVENT_ACTIVATE,
				 DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL,
				 "+2/+0",		1,	1,	DLG_TARGET(&td_pump, "TARGET_CREATURE"),
				 "Can't block",	1,	1,	DLG_TARGET(&td_blck, "TARGET_CREATURE")) == 1)
		pump_until_eot_merge_previous(player, card, inst->targets[0].player, inst->targets[0].card, 2,0);
	  else
		pump_ability_until_eot_no_repeat(player, card, inst->targets[0].player, inst->targets[0].card, 0,SP_KEYWORD_CANNOT_BLOCK);
	}

  return global_enchantment(player, card, event);
}

/* Rolling Thunder	|X|R|R	=>tempest.c:card_rolling_thunder
 * Sorcery
 * ~ deals X damage divided as you choose among any number of target creatures and/or players. */

/* Shatterskull Recruit	|3|R|R	=>gatecrash.c:card_ripscale_predators
 * Creature - Giant Warrior Ally 4/4
 * Menace */

/* Stonefury	|3|R|R	=>gatecrash.c:card_ground_assault
 * Instant
 * ~ deals damage to target creature equal to the number of lands you control. */

/* Sure Strike	|1|R	=>zendikar.c:card_slaughter_cry
 * Instant
 * Target creature gets +3/+0 and gains first strike until end of turn. */

int card_tunneling_geopede(int player, int card, event_t event)
{
  /* Tunneling Geopede	|2|R	0x200e0ad
   * Creature - Insect 3/2
   * Landfall - Whenever a land enters the battlefield under your control, ~ deals 1 damage to each opponent. */

  if (landfall(player, card, event))
	damage_player(1-player, 1, player, card);

  return 0;
}

int card_valakut_invoker(int player, int card, event_t event)
{
  /* Valakut Invoker	|2|R	0x200e0b2
   * Creature - Human Shaman 2/3
   * |8: ~ deals 3 damage to target creature or player. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	damage_target0(player, card, 3);

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(8), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

/* Valakut Predator	|2|R	=>zendikar.c:card_steppe_lynx
 * Creature - Elemental 2/2
 * Landfall - Whenever a land enters the battlefield under your control, ~ gets +2/+2 until end of turn. */

/* Volcanic Upheaval	|3|R	=>unlimited.c:card_ice_storm
 * Instant
 * Destroy target land. */

/* Zada, Hedron Grinder	|3|R	0x000000
 * Legendary Creature - Goblin Ally 3/3
 * Whenever you cast an instant or sorcery spell that targets only ~, copy that spell for each other creature you control that the spell could target. Each copy targets a different one of those creatures. */

/*** Green ***/

/* Blisterpod	|G	=>card_carrier_thrall
 * Creature - Eldrazi Drone 1/1
 * Devoid
 * When ~ dies, put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool." */

int card_brood_monitor(int player, int card, event_t event)
{
  /* Brood Monitor	|4|G|G	0x200e0bc
   * Creature - Eldrazi Drone 3/3
   * Devoid
   * When ~ enters the battlefield, put three 1/1 colorless Eldrazi Scion creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your mana pool." */

  if (comes_into_play(player, card, event))
	generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SCION, 3);

  return 0;
}

int card_call_the_scions(int player, int card, event_t event)
{
  /* Call the Scions	|2|G	0x200e0c1
   * Sorcery
   * Devoid
   * Put two 1/1 colorless Eldrazi Scion creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your mana pool." */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SCION, 2);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_eyeless_watcher(int player, int card, event_t event)
{
  /* Eyeless Watcher	|3|G	0x200e0c6
   * Creature - Eldrazi Drone 1/1
   * Devoid
   * When ~ enters the battlefield, put two 1/1 colorless Eldrazi Scion creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your mana pool." */

  if (comes_into_play(player, card, event))
	generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SCION, 2);

  return 0;
}

int card_from_beyond(int player, int card, event_t event)
{
  /* From Beyond	|3|G	0x200e0e4
   * Enchantment
   * Devoid
   * At the beginning of your upkeep, put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool."
   * |1|G, Sacrifice ~: Search your library for an Eldrazi card, reveal it, put it into your hand, then shuffle your library. */

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);

  if (!IS_GAA_EVENT(event))
	return global_enchantment(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ANY, "Select an Eldrazi card.");
	  test.subtype = SUBTYPE_ELDRAZI;

	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

  return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_XG(1,1), 0, NULL, NULL);
}

int card_unnatural_aggression(int player, int card, event_t event)
{
  /* Unnatural Aggression	|2|G	0x200e0cb
   * Instant
   * Devoid
   * Target creature you control fights target creature an opponent controls. If the creature an opponent controls would die this turn, exile it instead. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td_control;
  default_target_definition(player, card, &td_control, TYPE_CREATURE);
  td_control.allowed_controller = td_control.preferred_controller = player;

  target_definition_t td_dont_control;
  default_target_definition(player, card, &td_dont_control, TYPE_CREATURE);
  td_dont_control.allowed_controller = td_dont_control.preferred_controller = 1-player;

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_CAN_CAST)
	return generic_spell(player, card, event, GS_CAN_TARGET, &td_control, NULL, 1, NULL) && can_target(&td_dont_control);

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && !(pick_target(&td_control, "ASHNODS_BATTLEGEAR")	// "Select target creature you control"
		   && new_pick_target(&td_dont_control, "TARGET_CREATURE_OPPONENT_CONTROLS", 1, 1)))
	inst->number_of_targets = 0;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (validate_target(player, card, &td_dont_control, 1))
		{
		  exile_if_would_be_put_into_graveyard(player, card, inst->targets[1].player, inst->targets[1].card, 1);

		  if (valid_target(&td_control))
			fight(inst->targets[0].player, inst->targets[0].card, inst->targets[1].player, inst->targets[1].card);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_void_attendant(int player, int card, event_t event)
{
  /* Void Attendant	|2|G	0x200e0d0
   * Creature - Eldrazi Processor 2/3
   * Devoid
   * |1|G, Put a card an opponent owns from exile into that player's graveyard: Put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool." */

  if (event == EVENT_RESOLVE_ACTIVATION)
	generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);

  return process_as_cost(player, 1, event,
						 generic_activated_ability(player, card, event, 0, MANACOST_XG(1,1), 0, NULL, NULL));
}

/* Beastcaller Savant	|1|G	0x000000
 * Creature - Elf Shaman Ally 1/1
 * Haste
 * |T: Add one mana of any color to your mana pool. Spend this mana only to cast a creature spell. */

/* Broodhunter Wurm	|3|G	=>vanilla
 * Creature - Wurm 4/3 */

int card_earthen_arms(int player, int card, event_t event)
{
  /* Earthen Arms	|1|G	0x200e0d5
   * Sorcery
   * Put two +1/+1 counters on target permanent.
   * Awaken 4-|6|G */

  if (!IS_AWAKEN_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  add_1_1_counters(inst->targets[0].player, inst->targets[0].card, 2);
		}
	  if (awaken_validate(player, card, 1))
		awaken_resolve(player, card, 1, 4);
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken_tgt(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", MANACOST_XG(6,1));
}

/* Giant Mantis	|3|G	=>vanilla
 * Creature - Insect 2/4
 * Reach */

int card_greenwarden_of_murasa(int player, int card, event_t event)
{
  /* Greenwarden of Murasa	|4|G|G	0x200e0da
   * Creature - Elemental 5/4
   * When ~ enters the battlefield, you may return target card from your graveyard to your hand.
   * When ~ dies, you may exile it. If you do, return target card from your graveyard to your hand. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player))
	  || (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player))
		  && exile_from_owners_graveyard(player, card)))
	{
	  test_definition_t test;
	  default_test_definition(&test, TYPE_ANY);
	  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

  return 0;
}

int card_infuse_with_the_elements(int player, int card, event_t event)
{
  /* Infuse with the Elements	|3|G	0x200e0df
   * Instant
   * Converge - Put X +1/+1 counters on target creature, where X is the number of colors of mana spent to cast ~. That creature gains trample until end of turn. */

  converge(player, card, event);

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  add_1_1_counters(inst->targets[0].player, inst->targets[0].card, inst->info_slot);
		  pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 0,0, KEYWORD_TRAMPLE,0);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_jaddi_offshoot(int player, int card, event_t event)
{
  /* Jaddi Offshoot	|G	0x200e0e9
   * Creature - Plant 0/3
   * Defender
   * Landfall - Whenever a land enters the battlefield under your control, you gain 1 life. */

  if (landfall(player, card, event))
	gain_life(player, 1);

  return 0;
}

int card_generic_combat_1_mana_producing_creature(int player, int card, event_t event)
{
  /* Lifespring Druid	|2|G	0x200e0ee
   * Creature - Elf Druid 2/1
   * |T: Add one mana of any color to your mana pool. */
  // Also: Alloy Myr, Llanowar Dead, Sisters of the Flame

  if (event == EVENT_CAN_CAST)
	return 1;	// so it can be assigned to a creature with flash

  int colors = get_card_instance(player, card)->mana_color & COLOR_TEST_ANY;
  if (!colors)
	return 0;
  else if ((colors & (colors - 1)) == 0)	// exactly one bit set
	return mana_producing_creature(player, card, event, 0, single_color_test_bit_to_color(colors), 1);
  else
	return mana_producing_creature_all_one_color(player, card, event, 0, colors, 1);
}

int card_murasa_ranger(int player, int card, event_t event)
{
  /* Murasa Ranger	|3|G	0x2008c11
   * Creature - Human Warrior 3/3
   * Landfall - Whenever a land enters the battlefield under your control, you may pay |3|G. If you do, put two +1/+1 counters on ~. */

  if (landfall_mode(player, card, event, RESOLVE_TRIGGER_AI(player))
	  && has_mana_multi(player, MANACOST_XG(3,1))
	  && charge_mana_multi_while_resolving(player, card, event, player, MANACOST_XG(3,1)))
	add_1_1_counters(player, card, 2);

  return 0;
}

/* Natural Connection	|2|G	=>mirage.c:card_rampant_growth
 * Instant
 * Search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library. */

int card_nissas_renewal(int player, int card, event_t event)
{
  /* Nissa's Renewal	|5|G	0x200410c
   * Sorcery
   * Search your library for up to three basic land cards, put them onto the battlefield tapped, then shuffle your library. You gain 7 life. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 3);
	  gain_life(player, 7);
	  kill_card(player, card, KILL_DESTROY);
	}
  return basic_spell(player, card, event);
}

int card_oran_rief_hydra(int player, int card, event_t event)
{
  /* Oran-Rief Hydra	|4|G|G	0x2008144
   * Creature - Hydra 5/5
   * Trample
   * Landfall - Whenever a land enters the battlefield under your control, put a +1/+1 counter on ~. If that land is |Ha Forest, put two +1/+1 counters on ~ instead. */

  if (landfall(player, card, event))
	add_1_1_counters(player, card, has_subtype(trigger_cause_controller, trigger_cause, get_hacked_subtype(player, card, SUBTYPE_FOREST)) ? 2 : 1);

  return 0;
}

int card_oran_rief_invoker(int player, int card, event_t event)
{
  /* Oran-Rief Invoker	|1|G	0x200cf5a
   * Creature - Human Shaman 2/2
   * |8: ~ gets +5/+5 and gains trample until end of turn. */

  return generic_shade(player, card, event, 0, MANACOST_X(8), 5,5, KEYWORD_TRAMPLE,0);
}

/* Plated Crusher	|4|G|G|G	m12.c:card_aven_fleetwing
 * Creature - Beast 7/6
 * Trample, hexproof */

/* Plummet	|1|G	=>urza_legacy.c:card_wing_snare
 * Instant
 * Destroy target creature with flying. */

/* Reclaiming Vines	|2|G|G	=>visions.c:card_creeping_mold
 * Sorcery
 * Destroy target artifact, enchantment, or land. */

int card_retreat_to_kazandu(int player, int card, event_t event)
{
  /* Retreat to Kazandu	|2|G	0x200cf5f
   * Enchantment
   * Landfall - Whenever a land enters the battlefield under your control, choose one -
   * * Put a +1/+1 counter on target creature.
   * * You gain 2 life. */

  if (landfall(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;
	  td.allow_cancel = 0;

	  if (DIALOG(player, card, EVENT_ACTIVATE,
				 DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_AUTOCHOOSE_IF_1,
				 "+1/+1 counter",	1,	3,	DLG_TARGET(&td, "TARGET_CREATURE"),
				 "Gain life",		1,	1) == 1)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  add_1_1_counter(inst->targets[0].player, inst->targets[0].card);
		}
	  else
		gain_life(player, 2);
	}

  return global_enchantment(player, card, event);
}

/* Rot Shambler	|1|G	=>innistrad.c:card_unruly_mob
 * Creature - Fungus 1/1
 * Whenever another creature you control dies, put a +1/+1 counter on ~. */

/* Scythe Leopard	|G	=>zendikar.c:card_hedron_scrabbler
 * Creature - Cat 1/1
 * Landfall - Whenever a land enters the battlefield under your control, ~ gets +1/+1 until end of turn. */

int card_seek_the_wilds(int player, int card, event_t event)
{
  /* Seek the Wilds	|1|G	0x200e0f3
   * Sorcery
   * Look at the top four cards of your library. You may reveal a creature or land card from among them and put it into your hand. Put the rest on the bottom of your library in any order. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  reveal_top_cards_of_library_and_choose_type(player, card, player, 4, 0, TUTOR_HAND,1, TUTOR_BOTTOM_OF_DECK,0, TYPE_CREATURE|TYPE_LAND);
	  kill_card(player, card, KILL_DESTROY);
	}
  return basic_spell(player, card, event);
}

/* Snapping Gnarlid	|1|G	=>zendikar.c:card_hedron_scrabbler
 * Creature - Beast 2/2
 * Landfall - Whenever a land enters the battlefield under your control, ~ gets +1/+1 until end of turn. */

int card_swell_of_growth(int player, int card, event_t event)
{
  /* Swell of Growth	|1|G	0x200e0f8
   * Instant
   * Target creature gets +2/+2 until end of turn. You may put a land card from your hand onto the battlefield. */

  int rval = vanilla_instant_pump(player, card, event, ANYBODY, player, 2,2, VANILLA_PUMP_DONT_KILL_CARD,0);
  if (rval && event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "Select a land card.");
	  new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &test);
	  kill_card(player, card, KILL_DESTROY);
	}
  return rval;
}

/* Sylvan Scrying	|1|G	=>mirrodin.c:card_sylvan_scrying
 * Sorcery
 * Search your library for a land card, reveal it, and put it into your hand. Then shuffle your library. */

int card_tajuru_beastmaster(int player, int card, event_t event)
{
  /* Tajuru Beastmaster	|5|G	0x200e0fd
   * Creature - Elf Warrior Ally 5/5
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, creatures you control get +1/+1 until end of turn. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_creatures_until_eot_merge_pt(player, card, player, 1,1, NULL);

  return 0;
}

int card_tajuru_stalwart(int player, int card, event_t event)
{
  /* Tajuru Stalwart	|2|G	0x200e102
   * Creature - Elf Scout Ally 0/1
   * Converge - ~ enters the battlefield with a +1/+1 counter on it for each color of mana spent to cast it. */

  converge(player, card, event);
  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, get_card_instance(player, card)->info_slot);
  return 0;
}

int card_tajuru_warcaller(int player, int card, event_t event)
{
  /* Tajuru Warcaller	|3|G|G	0x200e107
   * Creature - Elf Warrior Ally 2/1
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, creatures you control get +2/+2 until end of turn. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_creatures_until_eot_merge_pt(player, card, player, 2,2, NULL);

  return 0;
}

/* Territorial Baloth	|4|G	=>zendikar.c:card_steppe_lynx
 * Creature - Beast 4/4
 * Landfall - Whenever a land enters the battlefield under your control, ~ gets +2/+2 until end of turn. */

int card_undergrowth_champion(int player, int card, event_t event)
{
  /* Undergrowth Champion	|1|G|G	0x200e10c
   * Creature - Elemental 2/2
   * If damage would be dealt to ~ while it has a +1/+1 counter on it, prevent that damage and remove a +1/+1 counter from ~.
   * Landfall - Whenever a land enters the battlefield under your control, put a +1/+1 counter on ~. */

  phantom_effect(player, card, event, 1);
  if (landfall(player, card, event))
	add_1_1_counter(player, card);

  return 0;
}

int card_woodland_wanderer(int player, int card, event_t event)
{
  /* Woodland Wanderer	|3|G	0x200e111
   * Creature - Elemental 2/2
   * Vigilance, trample
   * Converge - ~ enters the battlefield with a +1/+1 counter on it for each color of mana spent to cast it. */

  vigilance(player, card, event);
  converge(player, card, event);
  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, get_card_instance(player, card)->info_slot);
  return 0;
}

/*** Multi ***/

int card_brood_butcher(int player, int card, event_t event)
{
  /* Brood Butcher	|3|B|G	0x200e116
   * Creature - Eldrazi Drone 3/3
   * Devoid
   * When ~ enters the battlefield, put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool."
   * |B|G, Sacrifice a creature: Target creature gets -2/-2 until end of turn. */

  if (comes_into_play(player, card, event))
	generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  pump_until_eot_merge_previous(player, card, inst->targets[0].player, inst->targets[0].card, -2,-2);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_SACRIFICE_CREATURE, MANACOST_BG(1,1), 0, &td, "TARGET_CREATURE");
}

static const char* target_is_creature_or_not_on_bf(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return in_play(player, card) && !is_what(player, card, TYPE_CREATURE) ? "type" : NULL;
}
int card_brutal_expulsion(int player, int card, event_t event)
{
  /* Brutal Expulsion	|2|U|R	0x200e11b
   * Instant
   * Devoid
   * Choose one or both -
   * * Return target spell or creature to its owner's hand.
   * * ~ deals 2 damage to target creature or planeswalker. If that permanent would be put into a graveyard this turn, exile it instead. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td_spell_or_creature;
  default_target_definition(player, card, &td_spell_or_creature, 0);
  td_spell_or_creature.illegal_type = TYPE_EFFECT;
  td_spell_or_creature.zone = TARGET_ZONE_IN_PLAY|TARGET_ZONE_ON_STACK;
  td_spell_or_creature.extra = (int)(&target_is_creature_or_not_on_bf);

  target_definition_t td_creature_or_planeswalker;
  default_target_definition(player, card, &td_creature_or_planeswalker, TYPE_CREATURE|TARGET_TYPE_PLANESWALKER);

  typedef enum
  {
	CHOICE_BOUNCE = 1,
	CHOICE_DAMAGE = 2,
	CHOICE_BOTH = 3	// == CHOICE_BOUNCE|CHOICE_DAMAGE
  } Choices;

  int can[CHOICE_BOTH+1];
  can[CHOICE_BOUNCE] = can_target(&td_spell_or_creature);
  can[CHOICE_DAMAGE] = can_target(&td_creature_or_planeswalker);
  can[CHOICE_BOTH] = can[CHOICE_BOUNCE] && can[CHOICE_DAMAGE];

  Choices choice = DIALOG(player, card, event,
						  DLG_RANDOM,
						  "Return to hand",	can[CHOICE_BOUNCE],	1,
						  "Deal damage",	can[CHOICE_DAMAGE],	1,
						  "Both",			can[CHOICE_BOTH],	3);

  if (event == EVENT_CAN_CAST)
	return choice;

  card_instance_t* inst = get_card_instance(player, card);
  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  inst->number_of_targets = 0;

	  if ((choice & CHOICE_BOUNCE)
		  && !pick_next_target_noload(&td_spell_or_creature, "Select target spell or creature"))
		choice &= ~CHOICE_DAMAGE;

	  if (choice & CHOICE_DAMAGE)
		new_pick_target(&td_creature_or_planeswalker, "TARGET_CREATURE_OR_PLANESWALKER", -1, 1);

	  if (cancel == 1)
		inst->number_of_targets = 0;
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int tgt = -1;

	  if ((choice & CHOICE_BOUNCE)
		  && validate_target(player, card, &td_spell_or_creature, ++tgt))
		bounce_permanent(inst->targets[tgt].player, inst->targets[tgt].card);

	  if ((choice & CHOICE_DAMAGE)
		  && validate_target(player, card, &td_creature_or_planeswalker, ++tgt))
		{
		  exile_if_would_be_put_into_graveyard(player, card, inst->targets[tgt].player, inst->targets[tgt].card, 1);
		  damage_creature(inst->targets[tgt].player, inst->targets[tgt].card, 2, player, card);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_catacomb_sifter(int player, int card, event_t event)
{
  /* Catacomb Sifter	|1|B|G	0x200e120
   * Creature - Eldrazi Drone 2/3
   * Devoid
   * When ~ enters the battlefield, put a 1/1 colorless Eldrazi Scion creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool."
   * Whenever another creature you control dies, scry 1. */

  if (comes_into_play(player, card, event))
	generate_token_by_id(player, card, CARD_ID_ELDRAZI_SCION);

  count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);

  int num;
  for (num = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY); num > 0; --num)
	scry(player, 1);

  return 0;
}

int card_dust_stalker(int player, int card, event_t event)
{
  /* Dust Stalker	|2|B|R	0x200e125
   * Creature - Eldrazi 5/3
   * Devoid
   * Haste
   * At the beginning of each end step, if you control no other colorless creatures, return ~ to its owner's hand. */

  haste(player, card, event);

  if (trigger_condition == TRIGGER_EOT && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.color = COLOR_TEST_ANY_COLORED;
	  test.color_flag = DOESNT_MATCH;
	  test.not_me = 1;

	  if (!check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test)
		  && eot_trigger(player, card, event))
		bounce_permanent(player, card);
	}

  return 0;
}

int card_fathom_feeder(int player, int card, event_t event)
{
  /* Fathom Feeder	|U|B	0x200e12a
   * Creature - Eldrazi Drone 1/1
   * Devoid
   * Deathtouch
   * Ingest
   * |3|U|B: Draw a card. Each opponent exiles the top card of his or her library. */

  deathtouch(player, card, event);
  ingest(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  draw_a_card(player);
	  rfg_top_card_of_deck(1-player);
	}
  return generic_activated_ability(player, card, event, 0, MANACOST_XUB(3,1,1), 0, NULL, NULL);
}

int card_forerunner_of_slaughter(int player, int card, event_t event)
{
  /* Forerunner of Slaughter	|B|R	0x200e12f
   * Creature - Eldrazi Drone 3/2
   * Devoid
   * |1: Target colorless creature gains haste until end of turn. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.illegal_color = COLOR_TEST_ANY_COLORED;
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 0,0, 0,SP_KEYWORD_HASTE);
	}
  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GS_LITERAL_PROMPT, MANACOST_X(1), 0, &td, "Select target colorless creature.");
}

int card_herald_of_kozilek(int player, int card, event_t event)
{
  /* Herald of Kozilek	|1|U|R	0x200e134
   * Creature - Eldrazi Drone 2/4
   * Devoid
   * Colorless spells you cast cost |1 less to cast. */

  if (event == EVENT_MODIFY_COST_GLOBAL
	  && !(get_color_real_or_iid(event_result, affected_card_controller, affected_card) & COLOR_TEST_ANY_COLORED)
	  && !is_humiliated(player, card))
	COST_COLORLESS -= 1;

  return 0;
}

int card_sire_of_stagnation(int player, int card, event_t event)
{
  /* Sire of Stagnation	|4|U|B	0x200e139
   * Creature - Eldrazi 5/7
   * Devoid
   * Whenever a land enters the battlefield under an opponent's control, that player exiles the top two cards of his or her library and you draw two cards. */

  if (specific_cip(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, TYPE_LAND,MATCH, 0,0, 0,0, 0, 0, -1,0))
	{
	  rfg_top_n_cards_of_deck(1-current_turn, 2);
	  draw_cards(player, 2);
	}

  return 0;
}

int card_ulamogs_nullifier(int player, int card, event_t event)
{
  /* Ulamog's Nullifier	|2|U|B	0x200e13e
   * Creature - Eldrazi Processor 2/3
   * Devoid
   * Flash
   * Flying
   * When ~ enters the battlefield, you may put two cards your opponents own from exile into their owners' graveyards. If you do, counter target spell. */

  if (!IS_CASTING(player, card, event))	// sic
	return 0;

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_CAN_CAST)
	return (inst->info_slot = (counterspell(player, card, EVENT_CAN_CAST, NULL, 0) && can_process(player, 2) ? 99 : 1));

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  if (inst->info_slot == 99 && card_on_stack_controller != -1)
		{
		  inst->targets[0].player = card_on_stack_controller;
		  inst->targets[0].card = card_on_stack;
		  inst->number_of_targets = 1;
		}
	  else
		{
		  inst->info_slot = 1;
		  if (player == AI)
			ai_modifier -= 36;
		}
	}

  if (comes_into_play(player, card, event) && inst->info_slot == 99)
	{
	  if (counterspell_validate(player, card, NULL, 0) && process(player, 2))
		real_counter_a_spell(player, card, inst->targets[0].player, inst->targets[0].card);
	  inst->number_of_targets = 0;
	}

  return 0;
}

int card_angelic_captain(int player, int card, event_t event)
{
  /* Angelic Captain	|3|R|W	0x200e143
   * Creature - Angel Ally 4/3
   * Flying
   * Whenever ~ attacks, it gets +1/+1 until end of turn for each other attacking Ally. */

  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_ALLY;
	  test.state = STATE_ATTACKING;
	  test.not_me = 1;

	  int allies = check_battlefield_for_special_card(player, card, current_turn, CBFSC_GET_COUNT, &test);
	  if (allies > 0)
		pump_until_eot(player, card, player, card, allies, allies);
	}
  return 0;
}

int card_bring_to_light(int player, int card, event_t event)
{
  /* Bring to Light	|3|G|U	0x200e288
   * Sorcery
   * Converge - Search your library for a creature, instant, or sorcery card with converted mana cost less than or equal to the number of colors of mana spent to cast ~, exile that card, then shuffle your library. You may cast that card without paying its mana cost. */

  converge(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int cmc = get_card_instance(player, card)->info_slot;
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE|TYPE_INSTANT|TYPE_SORCERY, "");
	  test.cmc = cmc + 1;
	  test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
	  if (ai_is_speculating != 1)
		sprintf(test.message, "Select a creature, instant, or sorcery card with converted mana cost less %d or less.", cmc);

	  int csvid = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MAX_VALUE, &test);
	  int iid = get_internal_card_id_from_csv_id(csvid);
	  if (csvid != -1
		  && !is_what(-1, iid, TYPE_LAND)
		  && can_legally_play_iid(player, iid)
		  && DIALOG(player, card, EVENT_CAST_SPELL,
					DLG_NO_CANCEL, DLG_NO_STORAGE, DLG_SMALLCARD_CSVID(csvid),
					"Cast", 1, 1,
					"Decline", 1, 0) == 1)
		play_card_in_exile_for_free(player, player, csvid);

	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_dranas_emissary(int player, int card, event_t event)
{
  /* Drana's Emissary	|1|W|B	0x200e148
   * Creature - Vampire Cleric Ally 2/2
   * Flying
   * At the beginning of your upkeep, each opponent loses 1 life and you gain 1 life. */

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  lose_life(1-player, 1);
	  gain_life(player, 1);
	}

  return 0;
}

/* Grove Rumbler	|2|R|G	=>zendikar.c:card_steppe_lynx
 * Creature - Elemental 3/3
 * Trample
 * Landfall - Whenever a land enters the battlefield under your control, ~ gets +2/+2 until end of turn. */

int card_grovetender_druids(int player, int card, event_t event)
{
  /* Grovetender Druids	|2|G|W	0x200e14d
   * Creature - Elf Druid Ally 3/3
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, you may pay |1. If you do, put a 1/1 |Sgreen Plant creature token onto the battlefield. */

  if (rally(player, card, event, RESOLVE_TRIGGER_AI(player))
	  && has_mana(player, COLOR_COLORLESS, 1)
	  && charge_mana_while_resolving(player, card, event, player, COLOR_COLORLESS, 1))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_PLANT, &token);
	  token.pow = token.tou = 1;
	  generate_token(&token);
	}

  return 0;
}

int card_kiora_master_of_the_depths(int player, int card, event_t event)
{
  /* Kiora, Master of the Depths	|2|G|U	0x200e29c
   * Planeswalker - Kiora (4)
   * +1: Untap up to one target creature and up to one target land.
   * -2: Reveal the top four cards of your library. You may put a creature card and/or a land card from among them into your hand. Put the rest into your graveyard.
   * -8: You get an emblem with "Whenever a creature enters the battlefield under your control, you may have it fight target creature." Then put three 8/8 |Sblue Octopus creature tokens onto the battlefield. */

  if (IS_ACTIVATING(event))
	{
	  card_instance_t* inst = get_card_instance(player, card);

	  target_definition_t td_creature;
	  default_target_definition(player, card, &td_creature, TYPE_CREATURE);
	  target_definition_t td_land;
	  default_target_definition(player, card, &td_land, TYPE_LAND);
	  td_creature.preferred_controller = td_land.preferred_controller = player;
	  td_creature.allow_cancel = td_land.allow_cancel = 3;	// Cancel and Done buttons
	  if (IS_AI(player))
		td_creature.illegal_state = td_land.illegal_state = TARGET_STATE_TAPPED;

	  enum
	  {
		CHOICE_UNTAP = 1,
		CHOICE_REVEAL,
		CHOICE_EMBLEM,
		CHOICE_UNTAP_ONLY_LAND	// A fake choice so we can validate against the correct target definition
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Untap",						1,	3,	+1,
						"Reveal library",				1,	1,	-2,
						"Emblem and Octopus tokens",	1,	10,	-8);

	  if (event == EVENT_CAN_ACTIVATE && !choice)
		return 0;

	  if (event == EVENT_ACTIVATE && choice == CHOICE_UNTAP)
		{
		  inst->number_of_targets = 0;

		  if (can_target(&td_creature))
			{
			  load_text(0, "TARGET_CREATURE");
			  if (!select_target(player, card, &td_creature, text_lines[0], &inst->targets[0]))
				{
				  if (inst->targets[0].card == -1)
					cancel = 1;
				  else
					inst->info_slot = CHOICE_UNTAP_ONLY_LAND;
				}
			}

		  if (cancel != 1 && can_target(&td_land))
			{
			  load_text(0, "TARGET_LAND");
			  int loc = inst->number_of_targets;
			  if (!select_target(player, card, &td_creature, text_lines[0], &inst->targets[loc])
				  && inst->targets[loc].card == -1)
				cancel = 1;
			}
		}

	  if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_UNTAP:
			  if (valid_target(&td_creature))
				untap_card(inst->targets[0].player, inst->targets[0].card);
			  if (inst->number_of_targets >= 2 && validate_target(player, card, &td_land, 1))
				untap_card(inst->targets[1].player, inst->targets[1].card);
			  break;

			case CHOICE_UNTAP_ONLY_LAND:
			  if (inst->number_of_targets >= 1 && valid_target(&td_land))
				untap_card(inst->targets[0].player, inst->targets[0].card);
			  break;

			case CHOICE_REVEAL:
			  if (deck_ptr[player][0] != -1)
				{
				  show_deck(HUMAN, deck_ptr[player], 4, "Revealed", 0, 0x7375b0);

				  int sz = MIN(4, count_deck(player));

				  test_definition_t test;
				  new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");
				  test.no_shuffle = 2;
				  test.create_minideck = sz;

				  if (new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test) != -1)
					--sz;

				  if (deck_ptr[player][0] != -1)
					{
					  new_default_test_definition(&test, TYPE_LAND, "Select a land card.");
					  test.no_shuffle = 2;
					  test.create_minideck = sz;

					  if (new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test) != -1)
						--sz;
					}

				  mill(player, sz);
				}
			  break;

			case CHOICE_EMBLEM:
			  generate_token_by_id(player, card, CARD_ID_KIORA_MASTER_OF_THE_DEPTHS_EMBLEM);
			  generate_tokens_by_id(player, card, CARD_ID_OCTOPUS, 3);
			  break;
		  }
	}

  return planeswalker(player, card, event, 4);
}
int card_kiora_master_of_the_depths_emblem(int player, int card, event_t event)
{
  /* Kiora, Master of the Depths Emblem	""	0x200e2a1
   * Emblem
   * Whenever a creature enters the battlefield under your control, you may have it fight target creature. */

  if (specific_cip(player, card, event, 1-player, RESOLVE_TRIGGER_AI(player), TYPE_CREATURE,MATCH, 0,0, 0,0, 0, 0, -1,0))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = 1-player;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		fight(trigger_cause_controller, trigger_cause, inst->targets[0].player, inst->targets[0].card);
	}

  return 0;
}

int card_march_from_the_tomb(int player, int card, event_t event)
{
  /* March from the Tomb	|3|W|B	0x200e152
   * Sorcery
   * Return any number of target Ally creature cards with total converted mana cost 8 or less from your graveyard to the battlefield. */

  if (event == EVENT_CAN_CAST)
	return 1;	// "any number" includes zero
  if (!IS_CASTING(player, card, event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select target Ally creature card with converted mana cost 8 or less.");
  test.subtype = SUBTYPE_ALLY;
  test.cmc = 9;
  test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  inst->info_slot = 0;	// Store number of targets here, since Manalink can't display graveyard targets properly

	  if (graveyard_has_shroud(player))
		return 0;	// No legal targets, so cast with none

	  int iid;
	  while (inst->info_slot < 10	// There haven't been any Ally creatures with cmc 0 printed, so the 10 reamining usable targets are enough
			 && (iid = select_target_from_grave_source(player, card, player, SFG_NO_SPELL_FIZZLED, AI_MAX_VALUE, &test, inst->info_slot)) != -1)
		{
		  ++inst->info_slot;
		  test.cmc -= get_cmc_by_internal_id(iid);
		  if (ai_is_speculating != 1)
			sprintf(test.message, "Select target Ally creature card with converted mana cost %d or less.", test.cmc);
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int i, gy_pos;
	  for (i = 0; i < inst->info_slot; ++i)
		if ((gy_pos = validate_target_from_grave_source(player, card, player, i)) != -1)
		  reanimate_permanent(player, card, player, gy_pos, REANIMATE_DEFAULT);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_munda_ambush_leader(int player, int card, event_t event)
{
  /* Munda, Ambush Leader	|2|R|W	0x200e157
   * Legendary Creature - Kor Ally 3/4
   * Haste
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, you may look at the top four cards of your library. If you do, reveal any number of Ally cards from among them, then put those cards on top of your library in any order and the rest on the bottom in any order. */

  check_legend_rule(player, card, event);
  haste(player, card, event);

  if (rally(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ANY, "Select an Ally card to put on top.");
	  test.subtype = SUBTYPE_ALLY;

	  int depth = MIN(4, count_deck(player));
	  if (depth == 0)
		return 0;

	  int to_reveal[4], num_to_reveal = 0;

	  int selected;
	  while (depth > 0
			 && (selected = select_card_from_zone(player, player, deck_ptr[player], depth, 0, AI_MIN_VALUE, -1, &test)) != -1)
		{
		  int iid = deck_ptr[player][selected];
		  to_reveal[num_to_reveal++] = iid;

		  remove_card_from_deck(player, selected);
		  --depth;
		  put_iid_under_the_first_x_cards_of_library(player, iid, depth);
		}

	  put_top_x_on_bottom(player, player, depth);

	  if (num_to_reveal > 0 && player == AI)
		{
		  // Randomize order, since the order to put them on top of the library isn't revealed
		  int i;
		  for (i = num_to_reveal - 1; i >= 1; --i)
			{
			  int r = internal_rand(i + 1);
			  if (r != i)
				SWAP(to_reveal[i], to_reveal[r]);
			}

		  show_deck(HUMAN, to_reveal, num_to_reveal, "Revealed", 0, 0x7375b0);
		}
	}

  return 0;
}

int card_noyan_dar_roil_shaper(int player, int card, event_t event)
{
  /* Noyan Dar, Roil Shaper	|3|W|U	0x200e15c
   * Legendary Creature - Merfolk Ally 4/4
   * Whenever you cast an instant or sorcery spell, you may put three +1/+1 counters on target land you control. If you do, that land becomes a 0/0 Elemental creature with haste that's still a land. */

  check_legend_rule(player, card, event);

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_INSTANT|TYPE_SORCERY,MATCH, 0,0, 0,0, 0,0, -1,0))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_LAND);
	  td.allowed_controller = td.preferred_controller = player;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && new_pick_target(&td, "Select target land you control", -1, 0))
		{
		  /* Adding the +1/+1 counters is a cost per rule 117.12.  There isn't currently anything that could prevent it, I don't think; maybe some sort of
		   * hell-Johnny combination that somehow gave an already-animated land Melira's Keepers' ability.  But it doesn't hurt to check. */
		  int old_counters = count_1_1_counters(inst->targets[0].player, inst->targets[0].card);
		  add_1_1_counters(inst->targets[0].player, inst->targets[0].card, 3);
		  if (count_1_1_counters(inst->targets[0].player, inst->targets[0].card) > old_counters)
			{
			  add_a_subtype(inst->targets[0].player, inst->targets[0].card, SUBTYPE_ELEMENTAL);
			  animate_other(player, card, inst->targets[0].player, inst->targets[0].card, 0,0, 0,SP_KEYWORD_HASTE, 0, 1);
			}
		}
	}

  return 0;
}

static int omnath_death_trigger(int player, int card, event_t event)
{
  /* This function implements the "Whenever [this card] or [something else] dies, ..." trigger on both Omnath and the effect card he makes when he dies.  Don't
   * try to shoehorn it directly into a card's function unless the card does nothing else. */
  if (event == EVENT_GRAVEYARD_FROM_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "");
	  test.subtype = SUBTYPE_ELEMENTAL;

	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->internal_card_id != LEGACY_EFFECT_CUSTOM)
		{
		  count_for_gfp_ability(player, card, event, player, 0, &test);

		  if (affect_me(player, card))
			{
			  card_instance_t* legacy = get_card_instance(player, create_legacy_effect(player, card, omnath_death_trigger));
			  legacy->targets[11].player = inst->targets[11].player;
			  inst->targets[1].player = CARD_ID_OMNATH_LOCUS_OF_RAGE;	// prevent main card from triggering
			}
		}
	  else if (!affect_me(inst->damage_source_player, inst->damage_source_card))
		count_for_gfp_ability(player, card, event, player, 0, &test);
	}

  if (trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && affect_me(player, card) && player == reason_for_trigger_controller)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (event == EVENT_TRIGGER)
		{
		  if (inst->targets[11].player > 0	// deaths recorded
			  && inst->targets[1].player != CARD_ID_OMNATH_LOCUS_OF_RAGE)	// hasn't handed off to an effect card
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  int num = MAX(0, inst->targets[11].player);
		  inst->targets[11].player = 0;
		  if (num > 0)
			{
			  target_definition_t td;
			  default_target_definition(player, card, &td, TYPE_CREATURE);
			  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

			  for (; num > 0; --num)
				{
				  inst->number_of_targets = 0;
				  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
					damage_target0(player, card, 3);
				  else
					break;
				}
			}
		}
	  if (event == EVENT_END_TRIGGER
		  && get_card_instance(player, card)->internal_card_id == LEGACY_EFFECT_CUSTOM)
		kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}
int card_omnath_locus_of_rage(int player, int card, event_t event)
{
  /* Omnath, Locus of Rage	|3|R|R|G|G	0x200e161
   * Legendary Creature - Elemental 5/5
   * Landfall - Whenever a land enters the battlefield under your control, put a 5/5 |Sred and |Sgreen Elemental creature token onto the battlefield.
   * Whenever ~ or another Elemental you control dies, ~ deals 3 damage to target creature or player. */

  check_legend_rule(player, card, event);

  if (landfall(player, card, event))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
	  token.pow = token.tou = 5;
	  token.color_forced = COLOR_TEST_RED|COLOR_TEST_GREEN;
	  generate_token(&token);
	}

  return omnath_death_trigger(player, card, event);
}

int card_resolute_blademaster(int player, int card, event_t event)
{
  /* Resolute Blademaster	|3|R|W	0x200e166
   * Creature - Human Soldier Ally 2/2
   * Rally - Whenever ~ or another Ally enters the battlefield under your control, creatures you control gain double strike until end of turn. */

  if (rally(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_creatures_until_eot_no_repeat(player, card, player, 0, KEYWORD_DOUBLE_STRIKE,0, NULL);

  return 0;
}

int card_roil_spout(int player, int card, event_t event)
{
  /* Roil Spout	|1|W|U	0x200e16b
   * Sorcery
   * Put target creature on top of its owner's library.
   * Awaken 4-|4|W|U */

  if (!IS_AWAKEN_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  put_on_top_of_deck(inst->targets[0].player, inst->targets[0].card);
		}
	  if (awaken_validate(player, card, 1))
		awaken_resolve(player, card, 1, 4);
	  kill_card(player, card, KILL_DESTROY);
	}
  return awaken_tgt(player, card, event, GS_CAN_TARGET, &td, "TARGET_", MANACOST_XWU(4,1,1));
}

int card_skyrider_elf(int player, int card, event_t event)
{
  /* Skyrider Elf	|X|G|U	0x200e170
   * Creature - Elf Warrior Ally 0/0
   * Flying
   * Converge - ~ enters the battlefield with a +1/+1 counter on it for each color of mana spent to cast it. */

  converge(player, card, event);
  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  inst->targets[1].player = inst->info_slot;	// number of colors paid
	  inst->info_slot = x_value;	// will still be valid; needs to be here for e.g. Spell Blast
	  set_special_flags2(player, card, SF2_X_SPELL);
	}

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, get_card_instance(player, card)->targets[1].player);

  return 0;
}

int card_veteran_warleader(int player, int card, event_t event)
{
  /* Veteran Warleader	|1|G|W	0x200e175
   * Creature - Human Soldier Ally 100/100
   * ~'s power and toughness are each equal to the number of creatures you control.
   * Tap another untapped Ally you control: ~ gains your choice of first strike, vigilance, or trample until end of turn. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card))
	event_result += creature_count[player];

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  base_target_definition(player, card, &td, TYPE_PERMANENT);
  td.required_subtype = SUBTYPE_ALLY;
  td.illegal_state = TARGET_STATE_TAPPED;
  td.special = TARGET_SPECIAL_NOT_ME;

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int p = inst->parent_controller, c = inst->parent_card;
	  if (!in_play(p, c))
		return 0;

	  typedef enum
	  {
		CHOICE_FIRST_STRIKE = 1,
		CHOICE_VIGILANCE,
		CHOICE_TRAMPLE,
	  } Choices;

	  int already[CHOICE_TRAMPLE+1];
	  already[CHOICE_FIRST_STRIKE] = check_for_ability(player, card, KEYWORD_FIRST_STRIKE);
	  already[CHOICE_VIGILANCE] = check_for_special_ability(player, card, SP_KEYWORD_VIGILANCE);
	  already[CHOICE_TRAMPLE] = check_for_ability(player, card, KEYWORD_TRAMPLE);
	  int vigilance_timing = current_turn == player && current_phase < PHASE_DECLARE_ATTACKERS;

	  Choices choice = DIALOG(player, card, event,
							  DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL,
							  "First strike",	1,	already[CHOICE_FIRST_STRIKE] ? 0 : 100,
							  "Vigilance",		1,	already[CHOICE_VIGILANCE] ? 0 : (!vigilance_timing ? 1 : 100),
							  "Trample",		1,	already[CHOICE_TRAMPLE] ? 0 : 100);

	  int kw = 0, skw = 0;
	  switch (choice)
		{
		  case CHOICE_FIRST_STRIKE:	kw = KEYWORD_FIRST_STRIKE;	break;
		  case CHOICE_VIGILANCE:	skw = SP_KEYWORD_VIGILANCE;	break;
		  case CHOICE_TRAMPLE:		kw = KEYWORD_TRAMPLE;		break;
		}
	  if (kw || skw)
		alternate_legacy_text(choice, p, pump_ability_until_eot(p, c, p, c, 0,0, kw,skw));
	}

  int rval = generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_LITERAL_PROMPT, MANACOST0, 0, &td, "Select another untapped Ally you control.");
  if (event == EVENT_ACTIVATE && cancel != 1)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  tap_card(inst->targets[0].player, inst->targets[0].card);
	  inst->number_of_targets = 0;	// No when-this-becomes-targeted triggers
	}

  return rval;
}

/*** Artifact ***/

int card_aligned_hedron_network(int player, int card, event_t event)
{
  /* Aligned Hedron Network	|4	0x200e17a
   * Artifact
   * When ~ enters the battlefield, exile all creatures with power 5 or greater until ~ leaves the battlefield. */
  /* 8/25/2015  In some very rare situations, Aligned Hedron Network may enter the battlefield as a creature with power 5 or greater. If this happens, Aligned
   * Hedron Network will exile itself along with other creatures with power 5 or greater. Those cards will immediately return to the battlefield. If this causes
   * a loop with Aligned Hedron Network continually exiling and returning itself, the game will be a draw unless a player breaks the loop somehow. */

  if (comes_into_play(player, card, event))
	{
	  char marked[2][151] = {{0}};
	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_CREATURE) && get_power(p, c) >= 5)
			marked[p][c] = 1;

	  if (marked[player][card])
		{
		  // per ruling, and there's no opportunity to break the loop in Manalink
		  lose_the_game(ANYBODY);
		  marked[player][card] = 0;	// won't get here except when speculating
		}

	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (marked[p][c] && in_play(p, c))
			obliviation(player, card, p, c);
	}

  return_from_oblivion(player, card, event);

  return 0;
}

int card_hedron_archive(int player, int card, event_t event)
{
  /* Hedron Archive	|4	0x200e17f
   * Artifact
   * |T: Add |2 to your mana pool.
   * |2, |T, Sacrifice ~: Draw two cards. */

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_DRAW
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1, paying_mana() ? DLG_NO_DISPLAY_FOR_AI : DLG_NO_OP,
						"Produce mana",
							mana_producer(player, card, EVENT_CAN_ACTIVATE),
							paying_mana(),
						"Draw",
							/* This calls generic_activated_ability() and charges mana manually instead of using a DLG_MANA() clause because both this and its
							 * mana ability have T in their costs */
							!paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED|GAA_SACRIFICE_ME,
																		MANACOST_X(4), 0, NULL, NULL),
							landsofcolor_controlled[player][COLOR_ANY] - 6);
	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_DRAW)
		{
		  if (event == EVENT_ACTIVATE)
			{
			  add_state(player, card, STATE_TAPPED);
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(2)))
				kill_card(player, card, KILL_SACRIFICE);
			  else
				remove_state(player, card, STATE_TAPPED);
			}
		  else if (event == EVENT_RESOLVE_ACTIVATION)
			draw_cards(player, 2);

		  return 0;
		}
	  // else fall through to card_sol_ring below
	}

  return card_sol_ring(player, card, event);
}

static void mark_if_blocked_by_colorless(int any_ptr, int unused, int blocking_controller, int blocking_card)
{
  int* any = (int*)any_ptr;
  if (!*any && !(get_color(blocking_controller, blocking_card) & COLOR_TEST_ANY_COLORED))
	*any = 1;
}
int card_hedron_blade(int player, int card, event_t event)
{
  /* Hedron Blade	|1	0x200e184
   * Artifact - Equipment
   * Equipped creature gets +1/+1.
   * Whenever equipped creature becomes blocked by one or more colorless creatures, it gains deathtouch until end of turn.
   * Equip |2 */

  if (event == EVENT_DECLARE_BLOCKERS)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->damage_target_player == current_turn
		  && (get_card_instance(inst->damage_target_player, inst->damage_target_card)->state & STATE_ATTACKING))
		{
		  int any = 0;
		  for_each_creature_blocking_me(inst->damage_target_player, inst->damage_target_card,
										mark_if_blocked_by_colorless, (int)(&any), 0);
		  if (any)
			pump_ability_until_eot(player, card, inst->damage_target_player, inst->damage_target_card, 0,0, 0,SP_KEYWORD_DEATHTOUCH);
		}
	}

  return vanilla_equipment(player, card, event, 2, 1,1, 0,0);
}

int card_pathway_arrows(int player, int card, event_t event)
{
  /* Pathway Arrows	|1	0x200e189
   * Artifact - Equipment
   * Equipped creature has "|2, |T: This creature deals 1 damage to target creature. If a colorless creature is dealt damage this way, tap it."
   * Equip |2 */

  if (!IS_ACTIVATING(event))
	return 0;

  card_instance_t* inst = get_card_instance(player, card);

#define ACTIVATE(ev)	granted_generic_activated_ability(player, card, inst->targets[8].player, inst->targets[8].card, ev,	\
														  GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE")

  typedef enum
  {
	CHOICE_EQUIP = 1,
	CHOICE_DAMAGE,
  } Choices;

  int can_equip = can_activate_basic_equipment(player, card, event, 2);

  target_definition_t td;

  int can_damage;
  if (event == EVENT_RESOLVE_ACTIVATION)
	can_damage = 1;
  else
	{
	  can_damage = is_equipping(player, card) && inst->targets[8].player == player;
	  if (can_damage)
		{
		  default_target_definition(inst->targets[8].player, inst->targets[8].card, &td, TYPE_CREATURE);
		  can_damage = ACTIVATE(EVENT_CAN_ACTIVATE);
		}
	}

  Choices choice = DIALOG(player, card, event,
						  DLG_RANDOM,
						  "Equip",			can_equip,	1,
						  "Deal damage",	can_damage,	10);

  if (event == EVENT_CAN_ACTIVATE)
	return choice;

  if (event == EVENT_ACTIVATE)
	switch (choice)
	  {
		case CHOICE_EQUIP:
		  activate_basic_equipment(player, card, 2);
		  break;

		case CHOICE_DAMAGE:
		  ACTIVATE(event);
		  break;
	  }

  if (event == EVENT_RESOLVE_ACTIVATION)
	switch (choice)
	  {
		case CHOICE_EQUIP:
		  resolve_activation_basic_equipment(player, card);
		  break;

		case CHOICE_DAMAGE:
		  inst->parent_controller = inst->targets[8].player;	// since it's the creature doing the targeting
		  inst->parent_card = inst->targets[8].card;
		  default_target_definition(player, card, &td, TYPE_CREATURE);	// after reassigning parent
		  if (valid_target(&td))
			{
			  damage_creature(inst->targets[0].player, inst->targets[0].card, 1, inst->targets[8].player, inst->targets[8].card);
			  if (!(get_color(inst->targets[0].player, inst->targets[0].card) & COLOR_TEST_ANY_COLORED))
				tap_card(inst->targets[0].player, inst->targets[0].card);
			}
		  break;
	  }

  return 0;
#undef ACTIVATE
}

/* Pilgrim's Eye	|3	=>worldwake.c:card_pilgrims_eye
 * Artifact Creature - Thopter 1/1
 * Flying
 * When ~ enters the battlefield, you may search your library for a basic land card, reveal it, put it into your hand, then shuffle your library. */

int card_slab_hammer(int player, int card, event_t event)
{
  /* Slab Hammer	|2	0x200e18e
   * Artifact - Equipment
   * Whenever equipped creature attacks, you may return a land you control to its owner's hand. If you do, the creature gets +2/+2 until end of turn.
   * Equip |2 */

  card_instance_t* inst = get_card_instance(player, card);
  if (inst->targets[8].card >= 0
	  && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), inst->targets[8].player, inst->targets[8].card))
	{
	  target_definition_t td;
	  base_target_definition(player, card, &td, TYPE_LAND);
	  td.allowed_controller = player;

	  int eqp = inst->targets[8].player;
	  int eqc = inst->targets[8].card;

	  target_t land;
	  if (can_target(&td) && select_target(player, card-1000, &td, "Select a land you control.", &land))
		{
		  bounce_permanent(land.player, land.card);
		  if (in_play(eqp, eqc))	// Just in case, say, it was equipping an animated land, and you chose to bounce that land
			pump_until_eot(player, card, player, card, 2,2);
		}
	}

  return basic_equipment(player, card, event, 2);
}

/*** Land ***/

/* Ally Encampment	""	0x000000
 * Land
 * |T: Add |1 to your mana pool.
 * |T: Add one mana of any color to your mana pool. Spend this mana only to cast an Ally spell.
 * |1, |T, Sacrifice ~: Return target Ally you control to its owner's hand. */

static void blighted_cataract_impl(int player, int card, target_definition_t* td)
{
  draw_cards(player, 2);
}
int card_blighted_cataract(int player, int card, event_t event)
{
  /* Blighted Cataract	""	0x200e193
   * Land
   * |T: Add |1 to your mana pool.
   * |5|U, |T, Sacrifice ~: Draw two cards. */
  return blighted_land(player, card, event, MANACOST_XU(5,1), "Draw",1, NULL,NULL, blighted_cataract_impl);
}

static void blighted_fen_impl(int player, int card, target_definition_t* td)
{
  if (valid_target(td))
	impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0);
}
int card_blighted_fen(int player, int card, event_t event)
{
  /* Blighted Fen	""	0x200e198
   * Land
   * |T: Add |1 to your mana pool.
   * |4|B, |T, Sacrifice ~: Target opponent sacrifices a creature. */

  target_definition_t td;
  if (IS_ACTIVATING(event))
	{
	  default_target_definition(player, card, &td, 0);
	  td.zone = TARGET_ZONE_PLAYERS;
	  td.allowed_controller = 1-player;
	}
  return blighted_land(player, card, event, MANACOST_XB(4,1), "Opponent sacrifices",creature_count[1-player] > 0, &td,"TARGET_OPPONENT", blighted_fen_impl);
}

static void blighted_gorge_impl(int player, int card, target_definition_t* td)
{
  if (valid_target(td))
	damage_target0(player, card, 2);
}
int card_blighted_gorge(int player, int card, event_t event)
{
  /* Blighted Gorge	""	0x200e19d
   * Land
   * |T: Add |1 to your mana pool.
   * |4|R, |T, Sacrifice ~: ~ deals 2 damage to target creature or player. */

  target_definition_t td;
  if (IS_ACTIVATING(event))
	default_target_definition(player, card, &td, TYPE_CREATURE);
  return blighted_land(player, card, event, MANACOST_XR(4,1), "Deal damage",1, &td,"TARGET_CREATURE_OR_PLAYER", blighted_gorge_impl);
}

static void blighted_steppe_impl(int player, int card, target_definition_t* td)
{
  gain_life(player, 2 * count_subtype(player, TYPE_CREATURE, -1));
}
int card_blighted_steppe(int player, int card, event_t event)
{
  /* Blighted Steppe	""	0x200e1a2
   * Land
   * |T: Add |1 to your mana pool.
   * |3|W, |T, Sacrifice ~: You gain 2 life for each creature you control. */
  return blighted_land(player, card, event, MANACOST_XW(3,1), "Gain life",creature_count[player] > 0, NULL,NULL, blighted_steppe_impl);
}

static void blighted_woodland_impl(int player, int card, target_definition_t* td)
{
  tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 2);
}
int card_blighted_woodland(int player, int card, event_t event)
{
  /* Blighted Woodland	""	0x200e1a7
   * Land
   * |T: Add |1 to your mana pool.
   * |3|G, |T, Sacrifice ~: Search your library for up to two basic land cards and put them onto the battlefield tapped. Then shuffle your library. */
  return blighted_land(player, card, event, MANACOST_XG(3,1), "Search for lands",1, NULL,NULL, blighted_woodland_impl);
}

int card_tangoland(int player, int card, event_t event)
{
  /* Canopy Vista	""	0x200e0b7
   * Land - |H2Forest |H2Plains
   * ~ enters the battlefield tapped unless you control two or more basic lands. */

  // inlines comes_into_play_tapped()
  if ((event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) && affect_me(player, card)
	  && !check_special_flags2(player, card, SF2_FACE_DOWN_DUE_TO_MANIFEST))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "");
	  test.subtype = SUBTYPE_BASIC;
	  test.not_me = 1;	/* It's not logically on the battlefield yet.  (Though this won't matter unless something somehow turns it into a basic land without
						 * removing this replacement effect.) */

	  if (check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &test) < 2)
		get_card_instance(player, card)->state |= STATE_TAPPED;	// avoid sending event
	}

  return mana_producer(player, card, event);
}

/* Cinder Glade	""	=>card_tangoland
 * Land - |H2Mountain |H2Forest
 * ~ enters the battlefield tapped unless you control two or more basic lands. */

/* Evolving Wilds	""	=>m11.c:card_terramorphic_expanse
 * Land
 * |T, Sacrifice ~: Search your library for a basic land card and put it onto the battlefield tapped. Then shuffle your library. */

int card_fertile_thicket(int player, int card, event_t event)
{
  /* Fertile Thicket	""	0x200e1b1
   * Land
   * ~ enters the battlefield tapped.
   * When ~ enters the battlefield, you may look at the top five cards of your library. If you do, reveal up to one basic land card from among them, then put that card on top of your library and the rest on the bottom in any order.
   * |T: Add |G to your mana pool. */

  comes_into_play_tapped(player, card, event);

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "Select a basic land card.");
	  test.subtype = SUBTYPE_BASIC;
	  reveal_top_cards_of_library_and_choose(player, card, player, 5, 0, TUTOR_DECK, 1, TUTOR_BOTTOM_OF_DECK, 0, &test);
	}

  return mana_producer(player, card, event);
}

int card_looming_spires(int player, int card, event_t event)
{
  /* Looming Spires	""	0x200e1b6
   * Land
   * ~ enters the battlefield tapped.
   * When ~ enters the battlefield, target creature gets +1/+1 and gains first strike until end of turn.
   * |T: Add |R to your mana pool. */

  comes_into_play_tapped(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 1,1, KEYWORD_FIRST_STRIKE,0);
	}

  return mana_producer(player, card, event);
}

int card_lumbering_falls(int player, int card, event_t event)
{
  /* Lumbering Falls	""	0x200e1cf
   * Land
   * ~ enters the battlefield tapped.
   * |T: Add |G or |U to your mana pool.
   * |2|G|U: ~ becomes a 3/3 |Sgreen and |Sblue Elemental creature with hexproof until end of turn. It's still a land. */

  comes_into_play_tapped(player, card, event);

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_ANIMATE
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1, DLG_NO_DISPLAY_FOR_AI,
						"Produce mana",
							mana_producer(player, card, EVENT_CAN_ACTIVATE),
							paying_mana(),
						"Become Elemental",
							!paying_mana(),
							!(get_card_instance(player, card)->token_status & STATUS_LEGACY_TYPECHANGE),
						DLG_MANA(MANACOST_XGU(2,1,1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_ANIMATE)
		{
		  if (event == EVENT_RESOLVE_ACTIVATION)
			animate_self(player, card, 3,3, 0,SP_KEYWORD_HEXPROOF, COLOR_TEST_GREEN|COLOR_TEST_BLUE, 0);
		  return 0;
		}
	}

  has_subtype_if_animated_self(player, card, event, SUBTYPE_ELEMENTAL);

  return mana_producer(player, card, event);
}

int card_mortuary_mire(int player, int card, event_t event)
{
  /* Mortuary Mire	""	0x200e1bb
   * Land
   * ~ enters the battlefield tapped.
   * When ~ enters the battlefield, you may put target creature card from your graveyard on top of your library.
   * |T: Add |B to your mana pool. */

  comes_into_play_tapped(player, card, event);

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select target creature card.");
	  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_DECK, 0, AI_MAX_VALUE, &test);
	}

  return mana_producer(player, card, event);
}

/* Prairie Stream	""	=>card_tangoland
 * Land - |H2Plains |H2Island
 * ~ enters the battlefield tapped unless you control two or more basic lands. */

int card_sanctum_of_ugin(int player, int card, event_t event)
{
  /* Sanctum of Ugin	""	0x200e1c0
   * Land
   * |T: Add |1 to your mana pool.
   * Whenever you cast a colorless spell with converted mana cost 7 or greater, you may sacrifice ~. If you do, search your library for a colorless creature card, reveal it, put it into your hand, then shuffle your library. */

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), 0,0, 0,0, COLOR_TEST_ANY_COLORED,DOESNT_MATCH, 0,0, 6,F5_CMC_GREATER_THAN_VALUE)
	  && can_sacrifice_this_as_cost(player, card))	// Yes, it's a cost.  Rule 117.12.
	{
	  kill_card(player, card, KILL_SACRIFICE);

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select a colorless creature card.");
	  test.color = COLOR_TEST_ANY_COLORED;
	  test.color_flag = DOESNT_MATCH;

	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

  return mana_producer(player, card, event);
}

int card_sandstone_bridge(int player, int card, event_t event)
{
  /* Sandstone Bridge	""	0x200e1c5
   * Land
   * ~ enters the battlefield tapped.
   * When ~ enters the battlefield, target creature gets +1/+1 and gains vigilance until end of turn.
   * |T: Add |W to your mana pool. */

  comes_into_play_tapped(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 1,1, 0,SP_KEYWORD_VIGILANCE);
	}

  return mana_producer(player, card, event);
}

int card_shambling_vent(int player, int card, event_t event)
{
  /* Shambling Vent	""	0x200e1d4
   * Land
   * ~ enters the battlefield tapped.
   * |T: Add |W or |B to your mana pool.
   * |1|W|B: ~ becomes a 2/3 |Swhite and |Sblack Elemental creature with lifelink until end of turn. It's still a land. */

  comes_into_play_tapped(player, card, event);

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_ANIMATE
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1, DLG_NO_DISPLAY_FOR_AI,
						"Produce mana",
							mana_producer(player, card, EVENT_CAN_ACTIVATE),
							paying_mana(),
						"Become Elemental",
							!paying_mana(),
							!(get_card_instance(player, card)->token_status & STATUS_LEGACY_TYPECHANGE),
						DLG_MANA(MANACOST_XWB(1,1,1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_ANIMATE)
		{
		  if (event == EVENT_RESOLVE_ACTIVATION)
			animate_self(player, card, 2,3, 0,SP_KEYWORD_LIFELINK, COLOR_TEST_WHITE|COLOR_TEST_BLACK, 0);
		  return 0;
		}
	}

  has_subtype_if_animated_self(player, card, event, SUBTYPE_ELEMENTAL);

  return mana_producer(player, card, event);
}

/* Shrine of the Forsaken Gods	""	0x000000
 * Land
 * |T: Add |1 to your mana pool.
 * |T: Add |2 to your mana pool. Spend this mana only to cast colorless spells. Activate this ability only if you control seven or more lands. */

int card_skyline_cascade(int player, int card, event_t event)
{
  /* Skyline Cascade	""	0x200e1ca
   * Land
   * ~ enters the battlefield tapped.
   * When ~ enters the battlefield, target creature an opponent controls doesn't untap during its controller's next untap step.
   * |T: Add |U to your mana pool. */

  comes_into_play_tapped(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  if (IS_AI(player))
		{
		  td.required_state = TARGET_STATE_TAPPED;
		  if (!can_target(&td))
			td.required_state = 0;
		}

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		does_not_untap_effect(player, card, inst->targets[0].player, inst->targets[0].card, 0, 1);
	}

  return mana_producer(player, card, event);
}

/* Smoldering Marsh	""	=>card_tangoland
 * Land - |H2Swamp |H2Mountain
 * ~ enters the battlefield tapped unless you control two or more basic lands. */

static void spawning_bed_impl(int player, int card, target_definition_t* td)
{
  generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SCION, 3);
}
int card_spawning_bed(int player, int card, event_t event)
{
  /* Spawning Bed	""	0x200e1ac
   * Land
   * |T: Add |1 to your mana pool.
   * |6, |T, Sacrifice ~: Put three 1/1 colorless Eldrazi Scion creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your mana pool." */
  return blighted_land(player, card, event, MANACOST_X(6), "Eldrazi Scion tokens",0, NULL,NULL, spawning_bed_impl);
}

/* Sunken Hollow	""	=>card_tangoland
 * Land - |H2Island |H2Swamp
 * ~ enters the battlefield tapped unless you control two or more basic lands. */

/* Plains	""	=>magic.exe:card_plains
 * Basic Land - |H2Plains */

/* Island	""	=>magic.exe:card_island
 * Basic Land - |H2Island */

/* Swamp	""	=>magic.exe:card_swamp
 * Basic Land - |H2Swamp */

/* Mountain	""	=>magic.exe:card_mountain
 * Basic Land - |H2Mountain */

/* Forest	""	=>magic.exe:card_forest
 * Basic Land - |H2Forest */
