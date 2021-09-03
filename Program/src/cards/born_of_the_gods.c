// -*- c-basic-offset:2 -*-
#include "manalink.h"

/* Unimplemented:
 * Crypsis: needs generalized protection
 * Perplexing Chimera: needs deflection */

/* Approximations:
 * All five Archetypes: Many "X gains flying until end of turn" effects created while the Archetype is in play will start working if the Archetype leaves play
 *                      during that turn.  It's unlikely that all such will ever be fixed.
 * Ephara, God of the Polis: Doesn't count creatures that come into play after it unless Ephara is in play and not copying something else at the time.
 * Karametra, God of Harvests: Probably doesn't work with this ruling: If you cast a creature card with bestow for its bestow cost, it becomes an Aura spell and
 *                             not a creature spell. Karametra's last ability won't trigger."
 * Spirit of the Labyrinth: Replaces card draws with nothing instead of making them impossible, so you can make choices that make a player draw cards.  Compare
 *                          rulings for Tajuru Preserver.  Waiting for rulings.
 */

/****************
* Set mechanics *
****************/

int inspired(int player, int card, event_t event)
{
  // Whenever ~ becomes untapped, {...}
  return event == EVENT_UNTAP_CARD && affect_me(player, card) && !is_humiliated(player, card);
}

// Returns 1 if tribute is declined.  Stores local data in targets[6].card.
static int tribute_wasnt_paid(int player, int card, event_t event, int num_counters)
{
  /* As this creature enters the battlefield, an opponent of your choice may place {num_counters} +1/+1 counter on it.
   * When ~ enters the battlefield, if tribute wasn't paid, {...} */

  if (event == EVENT_RESOLVE_SPELL && num_counters > 0)
	{
	  char buf[100];
	  if (ai_is_speculating == 1)
		*buf = 0;
	  else if (num_counters == 1)
		strcpy(buf, "+1/+1 counter");
	  else
		sprintf(buf, "%d +1/+1 counters", num_counters);

	  if (DIALOG(player, card, EVENT_ACTIVATE,
				 DLG_WHO_CHOOSES(1-player), DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_RANDOM,
				 buf, 1, 1,
				 "No tribute", 1, 1) == 1)
		add_1_1_counters(player, card, num_counters);
	  else
		get_card_instance(player, card)->targets[6].card = 94;
	}

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY
	  && get_card_instance(player, card)->targets[6].card == 94
	  && comes_into_play(player, card, event))
	return 1;

  return 0;
}

static int effect_become_enchantment(int player, int card, event_t event)
{
  if (event == EVENT_CHANGE_TYPE)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  // Set continuously, just in case an Enchanted Evening card leaves play
	  if (instance->targets[1].player != 1)	// This effect is leaving play
		set_special_flags2(instance->damage_target_player, instance->damage_target_card, SF2_ENCHANTED_EVENING);
	}

  if (event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(player, card))	// no need for a full leaves_play() trigger for an effect card
	{
	  get_card_instance(player, card)->targets[1].player = 1;	// So don't reset
	  recalculate_all_cards_in_play();	// On the off chance something like Yavimaya Enchantress is counting enchantments.  Not automatically called for effect cards leaving play.
	}

  return 0;
}

// Can't be set by manipulating targets[] of effect_become_enchantment, since it has to be put onto tokens before they're put into play.
static int effect_become_enchantment_and_haste_and_remove_eot(int player, int card, event_t event)
{
  effect_become_enchantment(player, card, event);
  return haste_and_remove_eot(player, card, event);
}

static int archetype(int player, event_t event, keyword_t keyword, sp_keyword_t sp_keyword, player_bits_t cant_have_or_gain_bit)
{
 /* Approximation: Many "X gains flying until end of turn" effects created while the Archetype is in play will start working if the Archetype leaves play during
  * that turn.  It's unlikely that all such will ever be fixed. */

  if (event != EVENT_ABILITIES)
	return 0;

  player_bits[1-player] |= cant_have_or_gain_bit;

  if (!is_what(affected_card_controller, affected_card, TYPE_CREATURE))
	return 0;

  if (affected_card_controller == player)
	{
	  event_result |= keyword;

	  if (sp_keyword)
		{
		  card_instance_t* aff = get_card_instance(affected_card_controller, affected_card);
		  if (aff->targets[16].card < 0)
			aff->targets[16].card = 0;
		  aff->targets[16].card |= sp_keyword;
		}
	}
  else
	{
	  event_result &= ~keyword;

	  if (sp_keyword)
		{
		  card_instance_t* aff = get_card_instance(affected_card_controller, affected_card);
		  if (aff->targets[16].card < 0)
			aff->targets[16].card = 0;
		  aff->targets[16].card &= ~sp_keyword;
		}
	}

  return 0;
}

/********
* Cards *
********/

/********
* White *
********/

int card_acolytes_reward(int player, int card, event_t event)
{
  /* Acolyte's Reward |1|W
   * Instant
   * Prevent the next X damage that would be dealt to target creature this turn, where |X is your devotion to white. If damage is prevented this way, ~ deals
   * that much damage to target creature or player. */

  target_definition_t td_prev;
  default_target_definition(player, card, &td_prev, TYPE_CREATURE);
  td_prev.preferred_controller = player;

  target_definition_t td_cp;
  default_target_definition(player, card, &td_cp, TYPE_CREATURE);
  td_cp.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_CAN_CAST)
	return !(can_target(&td_prev) && can_target(&td_cp)) ? 0 : (land_can_be_played & LCBP_DAMAGE_PREVENTION) ? 99 : 1;

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (pick_target(&td_prev, "TARGET_CREATURE")
		  && new_pick_target(&td_cp, "TARGET_CREATURE_OR_PLAYER", 1, 1)
		  && player == AI)
		{
		  ai_modifier += (devotion(player, card, COLOR_WHITE, 0) - 2) * 24;

		  if (instance->targets[0].player != player)
			ai_modifier -= 96;

		  if (instance->targets[1].player == player)
			ai_modifier -= 96;
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td_prev))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  prevent_t flags = 0;
		  target_t redir = { -1, -1 };
		  if (validate_target(player, card, &td_cp, 1))
			{
			  flags = PREVENT_REDIRECT;
			  redir = instance->targets[1];	// struct copy
			}

		  prevent_the_next_n_damage(player, card, instance->targets[0].player, instance->targets[0].card,
									devotion(player, card, COLOR_WHITE, 0), flags, redir.player, redir.card);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_akroan_phalanx(int player, int card, event_t event)
{
  /* Akroan Phalanx |3|W
   * Creature - Human Soldier (3/3)
   * Vigilance
   * |2|R: Creatures you control get +1/+0 until end of turn. */

  vigilance(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	pump_creatures_until_eot_merge_pt(player, card, player, 1,0, NULL);

  return generic_activated_ability(player, card, event, 0, MANACOST_XR(2,1), 0, NULL, NULL);
}

/* Akroan Skyguard |1|W => theros.c:card_fabled_hero
 * Creature - Human Soldier (1/1)
 * Flying
 * Heroic - Whenever you cast a spell that targets ~, put a +1/+1 counter on ~. */

int card_archetype_of_courage(int player, int card, event_t event)
{
  /* Archetype of Courage |1|W|W
   * Enchantment Creature - Human Soldier (2/2)
   * Creatures you control have first strike.
   * Creatures your opponents control lose first strike and can't have or gain first strike. */

  if (event == EVENT_CAN_CAST)
	return 1;

  return archetype(player, event, KEYWORD_FIRST_STRIKE, 0, PB_CANT_HAVE_OR_GAIN_FIRST_STRIKE);
}

int card_brimaz_king_of_oreskos(int player, int card, event_t event)
{
  /* Brimaz, King of Oreskos |1|W|W
   * Legendary Creature - Cat Soldier (3/4)
   * Vigilance
   * Whenever ~ attacks, put a 1/1 white Cat Soldier creature token with vigilance onto the battlefield attacking.
   * Whenever ~ blocks a creature, put a 1/1 white Cat Soldier creature token with vigilance onto the battlefield blocking that creature. */

  check_legend_rule(player, card, event);

  vigilance(player, card, event);

  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_CAT_SOLDIER, &token);
	  token.pow = token.tou = 1;
	  token.s_key_plus = SP_KEYWORD_VIGILANCE;
	  token.action = TOKEN_ACTION_ATTACKING_UNTAPPED;
	  generate_token(&token);
	}

  if (blocking(player, card, event) && !is_humiliated(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_CAT_SOLDIER, &token);
	  token.pow = token.tou = 1;
	  token.s_key_plus = SP_KEYWORD_VIGILANCE;
	  token.action = TOKEN_ACTION_BLOCKING;
	  token.action_argument = instance->blocking;
	  generate_token(&token);
	}

  return 0;
}

int card_dawn_to_dusk(int player, int card, event_t event)
{
  /* Dawn to Dusk |2|W|W
   * Sorcery
   * Choose one or both - Return target enchantment card from your graveyard to your hand; and/or destroy target enchantment. */

  if (event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) || event == EVENT_RESOLVE_SPELL)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	  int can_gy_to_hand, can_destroy, can_destroy_opponents;
	  if (event == EVENT_RESOLVE_SPELL)
		can_gy_to_hand = can_destroy = can_destroy_opponents = 0;
	  else
		{
		  can_gy_to_hand = any_in_graveyard_by_type(player, TYPE_ENCHANTMENT) && !graveyard_has_shroud(player);
		  can_destroy = can_target(&td);
		  if (player != AI || !can_destroy)
			can_destroy_opponents = 0;
		  else
			{
			  target_definition_t td_opp;
			  default_target_definition(player, card, &td_opp, TYPE_ENCHANTMENT);
			  td.allowed_controller = 1-player;
			  can_destroy_opponents = can_target(&td_opp);
			}
		}

	  enum
	  {
		CHOICE_GY_TO_HAND = 1,
		CHOICE_DESTROY = 2,
		CHOICE_BOTH = 3,	// Fortuitously happens to equal CHOICE_GY_TO_HAND | CHOICE_DESTROY
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Return from graveyard", can_gy_to_hand, 5,
						"Destroy enchantment", can_destroy, can_destroy_opponents ? 5 : 1,
						"Both", can_gy_to_hand && can_destroy, can_destroy_opponents ? 25 : 3);

	  if (event == EVENT_CAN_CAST)
		return choice;
	  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  instance->number_of_targets = 0;

		  if (choice & CHOICE_GY_TO_HAND)
			{
			  test_definition_t test;
			  new_default_test_definition(&test, TYPE_ENCHANTMENT, "Select an enchantment card.");

			  if (new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &test, 1) == -1)
				{
				  spell_fizzled = 1;
				  return 0;
				}
			}

		  if (choice & CHOICE_DESTROY)
			pick_target(&td, "TARGET_ENCHANTMENT");
		}
	  else	// EVENT_RESOLVE_SPELL
		{
		  int selected;
		  if ((choice & CHOICE_GY_TO_HAND)
			  && (selected = validate_target_from_grave(player, card, player, 1)) != -1)
			from_grave_to_hand(player, selected, TUTOR_HAND);

		  if ((choice & CHOICE_DESTROY)
			  && valid_target(&td))
			{
			  card_instance_t* instance = get_card_instance(player, card);
			  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}

		  kill_card(player, card, KILL_DESTROY);
		}
	}

  return 0;
}

int card_eidolon_of_countless_battles(int player, int card, event_t event)
{
  /* Eidolon of Countless Battles |1|W|W
   * Enchantment Creature - Spirit (0/0)
   * Bestow |2|W|W
   * ~ and enchanted creature each get +1/+1 for each creature you control and +1/+1 for each Aura you control. */

  card_instance_t* instance;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && (affect_me(player, card)
		  || ((instance = get_card_instance(player, card)) && affect_me(instance->damage_target_player, instance->damage_target_card))))
	event_result += count_subtype(player, TYPE_CREATURE, -1) + count_subtype(player, TYPE_ENCHANTMENT, SUBTYPE_AURA);

  return generic_creature_with_bestow(player, card, event, MANACOST_XW(2,2), 0,0, 0, 0);
}

int card_elite_skirmisher(int player, int card, event_t event)
{
  /* Elite Skirmisher |2|W
   * Creature - Human Soldier (3/1)
   * Heroic - Whenever you cast a spell that targets ~, you may tap target creature. */

  if (heroic_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		tap_card(instance->targets[0].player, instance->targets[0].card);
	}

  return 0;
}

int card_epharas_radiance(int player, int card, event_t event)
{
  /* Ephara's Radiance |W
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature has "|1|W, |T: You gain 3 life." */

  if (event == EVENT_RESOLVE_ACTIVATION)
	gain_life(get_card_instance(player, card)->targets[9].player, 3);

  return aura_granting_activated_ability(player, card, event, player, GAA_UNTAPPED, MANACOST_XW(1,1), 0, NULL, NULL);
}

int card_excoriate(int player, int card, event_t event)
{
  /* Excoriate |3|W
   * Sorcery
   * Exile target tapped creature. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_state = TARGET_STATE_TAPPED;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (valid_target(&td))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "ROYAL_ASSASSIN", 1, NULL);	// "Select target tapped creature."
}

int card_fated_retribution(int player, int card, event_t event)
{
  /* Fated Retribution |4|W|W|W
   * Instant
   * Destroy all creatures and planeswalkers. If it's your turn, scry 2. */

  if (event == EVENT_CAN_CAST)
	return 1;

  // I'd put in an AI modifier so it prefers to cast on its own turn, but it doesn't do anything with scry anyway.

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  default_test_definition(&test, TYPE_CREATURE | TARGET_TYPE_PLANESWALKER);
	  new_manipulate_all(player, card, ANYBODY, &test, KILL_DESTROY);

	  if (player == current_turn)
		scry(player, 2);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_ghostblade_eidolon(int player, int card, event_t event)
{
  /* Ghostblade Eidolon |2|W
   * Enchantment Creature - Spirit (1/1)
   * Bestow |5|W
   * Double strike
   * Enchanted creature gets +1/+1 and has double strike. */

  return generic_creature_with_bestow(player, card, event, MANACOST_XW(5,1), 1,1, KEYWORD_DOUBLE_STRIKE, 0);
}

int card_glimpse_the_sun_god(int player, int card, event_t event)
{
  /* Glimpse the Sun God |X|W
   * Instant
   * Tap X target creatures. Scry 1. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int valid = 0;
	  card_instance_t* instance = get_card_instance(player, card);

	  target_definition_t td;
	  if (instance->number_of_targets == 0)
		++valid;	// If cast without targets, doesn't fizzle
	  else
		default_target_definition(player, card, &td, TYPE_CREATURE);

	  int i;
	  for (i = 0; i < instance->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  {
			++valid;
			tap_card(instance->targets[i].player, instance->targets[i].card);
		  }

	  if (valid)
		scry(player, 1);

	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}
  else
	return card_winter_blast_exe(player, card, event);	// Take advantage of MicroProse AI
}

int card_god_favored_general(int player, int card, event_t event)
{
  /* God-Favored General |1|W
   * Creature - Human Soldier (1/1)
   * Inspired - Whenever ~ becomes untapped, you may pay |2|W. If you do, put two 1/1 white Soldier enchantment creature tokens onto the battlefield. */

  if (inspired(player, card, event) && has_mana_multi(player, MANACOST_XW(2,1))
	  && DIALOG(player, card, EVENT_ACTIVATE, DLG_NO_STORAGE, "Generate Soldiers", 1, 1)
	  && charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_XW(2,1)))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_SOLDIER, &token);
	  token.special_flags2 = SF2_ENCHANTED_EVENING;
	  token.qty = 2;
	  generate_token(&token);
	}

  return 0;
}

/* Great Hart |3|W => vanilla
 * Creature - Elk (2/4) */

/* Griffin Dreamfinder |3|W|W => urza_saga.c:card_monk_idealist
 * Creature - Griffin (1/4)
 * When ~ enters the battlefield, return target enchantment card from your graveyard to your hand. */

int card_hero_of_iroas(int player, int card, event_t event)
{
  /* Hero of Iroas |1|W
   * Creature - Human Soldier (2/2)
   * Aura spells you cast cost |1 less to cast.
   * Heroic - Whenever you cast a spell that targets ~, put a +1/+1 counter on ~. */

  if (event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player
	  && has_subtype(affected_card_controller, affected_card, SUBTYPE_AURA) && !is_humiliated(player, card))
	{
	  --COST_COLORLESS;
	  return 0;
	}

  if (heroic(player, card, event))
	add_1_1_counter(player, card);

  return 0;
}

int card_hold_at_bay(int player, int card, event_t event)
{
  /* Hold at Bay |1|W
   * Instant
   * Prevent the next 7 damage that would be dealt to target creature or player this turn. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_CAN_CAST)
	return !can_target(&td) ? 0 : (land_can_be_played & LCBP_DAMAGE_PREVENTION) ? 99 : 1;

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (pick_target(&td, "TARGET_CREATURE_OR_PLAYER")
		  && player == AI && instance->targets[0].player != player)
		ai_modifier -= 96;
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  prevent_the_next_n_damage(player, card, instance->targets[0].player, instance->targets[0].card, 7, 0, 0, 0);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/* Loyal Pegasus |W => Mogg Flunkies
 * Creature - Pegasus (2/1)
 * Flying
 * ~ can't attack or block alone. */

int card_mortals_ardor(int player, int card, event_t event)
{
  /* Mortal's Ardor |W
   * Instant
   * Target creature gets +1/+1 and gains lifelink until end of turn. */

  return vanilla_instant_pump(player, card, event, ANYBODY, player, 1,1, 0,SP_KEYWORD_LIFELINK);
}

int card_nyxborn_shieldmate(int player, int card, event_t event)
{
  /* Nyxborn Shieldmate |W
   * Enchantment Creature - Human Soldier (1/2)
   * Bestow |2|W
   * Enchanted creature gets +1/+2. */

  return generic_creature_with_bestow(player, card, event, MANACOST_XW(2,1), 1,2, 0, 0);
}

int card_oreskos_sun_guide(int player, int card, event_t event)
{
  /* Oreskos Sun Guide |1|W
   * Creature - Cat Monk (2/2)
   * Inspired - Whenever ~ becomes untapped, you gain 2 life. */

  if (inspired(player, card, event))
	gain_life(player, 2);

  return 0;
}

int card_ornitharch(int player, int card, event_t event)
{
  /* Ornitharch |3|W|W
   * Creature - Archon (3/3)
   * Flying
   * Tribute 2
   * When ~ enters the battlefield, if tribute wasn't paid, put two 1/1 white Bird creature tokens with flying onto the battlefield. */

  if (tribute_wasnt_paid(player, card, event, 2))
	generate_tokens_by_id(player, card, CARD_ID_BIRD, 2);

  return 0;
}

int card_plea_for_guidance(int player, int card, event_t event)
{
  /* Plea for Guidance |5|W
   * Sorcery
   * Search your library for up to two enchantment cards, reveal them, and put them into your hand. Then shuffle your library. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t this_test;
	  new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select an enchantment card.");
	  this_test.qty = 2;
	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	  shuffle(player);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/* Revoke Existence |1|W => scars_of_mirrodin.c
 * Sorcery
 * Exile target artifact or enchantment. */

int card_silent_sentinel(int player, int card, event_t event)
{
  /* Silent Sentinel |5|W|W
   * Creature - Archon (4/6)
   * Flying
   * Whenever ~ attacks, you may return target enchantment card from your graveyard to the battlefield. */

  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && count_graveyard_by_type(player, TYPE_ENCHANTMENT) && !graveyard_has_shroud(player)
	  && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ENCHANTMENT, "Select target enchantment card.");
	  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &test);
	}

  return 0;
}

int card_spirit_of_the_labyrinth(int player, int card, event_t event)
{
  /* Spirit of the Labyrinth |1|W
   * Enchantment Creature - Spirit (3/1)
   * Each player can't draw more than one card each turn. */

  /* Approximation: Replaces card draws with nothing instead of making them impossible, so you can make choices that make a player draw cards.  Compare rulings
   * for Tajuru Preserver.  Waiting for rulings. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card)
	  && event == EVENT_TRIGGER && !suppress_draw && in_play(player, card) && cards_drawn_this_turn[reason_for_trigger_controller] > 0)
	suppress_draw = 1;	// Deliberately do this during EVENT_TRIGGER, not EVENT_RESOLVE_TRIGGER, so nothing else can replace the (nonexistent) draw

  return 0;
}

int card_sunbond(int player, int card, event_t event)
{
  /* Sunbond |3|W
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature has "Whenever you gain life, put that many +1/+1 counters on this creature." */

  card_instance_t* instance;
  int amt;
  if (trigger_condition == TRIGGER_GAIN_LIFE	// not needed; just avoid some processing in most calls to this function
	  && (instance = in_play(player, card))
	  && instance->damage_target_player >= 0 && instance->damage_target_card >= 0
	  && !is_humiliated(instance->damage_target_player, instance->damage_target_card)
	  // Trigger is placed on this card for clarity in the interface, even though it's really the enchanted creature triggering
	  && (amt = trigger_gain_life(player, card, event, instance->damage_target_player, RESOLVE_TRIGGER_MANDATORY)))
	add_1_1_counters(instance->damage_target_player, instance->damage_target_card, amt);

  return vanilla_aura(player, card, event, player);
}

int card_vanguard_of_brimaz(int player, int card, event_t event)
{
  /* Vanguard of Brimaz |W|W
   * Creature - Cat Soldier (2/2)
   * Vigilance
   * Heroic - Whenever you cast a spell that targets ~, put a 1/1 white Cat Soldier creature token with vigilance onto the battlefield. */

  vigilance(player, card, event);

  if (heroic(player, card, event))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_CAT_SOLDIER, &token);
	  token.s_key_plus = SP_KEYWORD_VIGILANCE;
	  generate_token(&token);
	}

  return 0;
}

/*******
* Blue *
********/

int card_aerie_worshippers(int player, int card, event_t event)
{
  /* Aerie Worshippers |3|U
   * Creature - Human Cleric (2/4)
   * Inspired - Whenever ~ becomes untapped, you may pay |2|U. If you do, put a 2/2 blue Bird enchantment creature token with flying onto the battlefield. */

  if (inspired(player, card, event) && has_mana_multi(player, MANACOST_XU(2,1))
	  && DIALOG(player, card, EVENT_ACTIVATE, DLG_NO_STORAGE, "Generate a Bird", 1, 1)
	  && charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_XU(2,1)))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_BIRD, &token);
	  token.special_flags2 = SF2_ENCHANTED_EVENING;
	  token.pow = 2;
	  token.tou = 2;
	  token.color_forced = COLOR_TEST_BLUE;
	  generate_token(&token);
	}

  return 0;
}

int card_arbiter_of_the_ideal(int player, int card, event_t event)
{
  /* Arbiter of the Ideal |4|U|U
   * Creature - Sphinx (4/5)
   * Flying
   * Inspired - Whenever ~ becomes untapped, reveal the top card of your library. If it's an artifact, creature, or land card, you may put it onto the
   * battlefield with a manifestation counter on it. It's an enchantment in addition to its other types. */

  if (inspired(player, card, event) && deck_ptr[player][0] != -1)
	{
	  if (!is_what(-1, deck_ptr[player][0], TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND)
		  || (is_what(-1, deck_ptr[player][0], TYPE_CREATURE) && check_battlefield_for_id(ANYBODY, CARD_ID_GRAFDIGGERS_CAGE)))
		reveal_card_iid(player, card, deck_ptr[player][0]);
	  else if (DIALOG(player, card, EVENT_ACTIVATE,
					  DLG_SMALLCARD_ID(deck_ptr[player][0]),
					  DLG_HEADER(player == HUMAN ? "You reveal:" : "Opponent reveals:"),
					  DLG_NO_STORAGE,
					  "Put into play", 1, 1))
		{
		  // put_into_play_a_card_from_deck() is the proper way to do this, but we need to make it already be an enchantment as it enters the battlefield.
		  int card_added = add_card_to_hand(player, deck_ptr[player][0]);
		  remove_card_from_deck(player, 0);
		  create_targetted_legacy_effect(player, card, &effect_become_enchantment, player, card_added);
		  set_special_flags2(player, card_added, SF2_ENCHANTED_EVENING);
		  ++hack_silent_counters;
		  add_counter(player, card_added, COUNTER_MANIFESTATION);
		  --hack_silent_counters;
		  put_into_play(player, card_added);
		}
	}

  return 0;
}

int card_archetype_of_imagination(int player, int card, event_t event)
{
  /* Archetype of Imagination |4|U|U
   * Enchantment Creature - Human Wizard (3/2)
   * Creatures you control have flying.
   * Creatures your opponents control lose flying and can't have or gain flying. */

  if (event == EVENT_CAN_CAST)
	return 1;

  return archetype(player, event, KEYWORD_FLYING, 0, PB_CANT_HAVE_OR_GAIN_FLYING);
}

int card_chorus_of_the_tides(int player, int card, event_t event)
{
  /* Chorus of the Tides |3|U
   * Flying
   * Heroic - Whenever you cast a spell that targets ~, scry 1. */

  if (heroic(player, card, event))
	scry(player, 1);

  return 0;
}

/* Crypsis |1|U
 * Instant
 * Target creature you control gains protection from creatures your opponents control until end of turn. Untap it. */

int card_deepwater_hypnotist(int player, int card, event_t event)
{
  /* Deepwater Hypnotist |1|U
   * Creature - Merfolk Wizard (2/1)
   * Inspired - Whenever ~ becomes untapped, target creature an opponent controls gets -3/-0 until end of turn. */

  if (inspired(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		pump_until_eot_merge_previous(player, card, instance->targets[0].player, instance->targets[0].card, -3,0);
	}

  return 0;
}

/* Divination |2|U => m14.c
 * Sorcery
 * Draw two cards. */

/* Eternity Snare |5|U => time_spiral.c
 * Enchantment - Aura
 * Enchant creature
 * When ~ enters the battlefield, draw a card.
 * Enchanted creature doesn't untap during its controller's untap step. */

int card_evanescent_intellect(int player, int card, event_t event)
{
  /* Evanescent Intellect |U
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature has "|1|U, |T: Target player puts the top three cards of his or her library into his or her graveyard." */

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  int rval = aura_granting_activated_ability(player, card, event, player, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_XU(1,1), 0, &td, "TARGET_PLAYER");

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (validate_arbitrary_target(&td, instance->targets[0].player, instance->targets[0].card))
		mill(instance->targets[0].player, 3);
	}

  return rval;
}

int card_fated_infatuation(int player, int card, event_t event)
{
  /* Fated Infatuation |U|U|U
   * Instant
   * Put a token onto the battlefield that's a copy of target creature you control. If it's your turn, scry 2. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = player;
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (valid_target(&td))
		{
		  copy_token(player, card, instance->targets[0].player, instance->targets[0].card);

		  if (player == current_turn)
			scry(player, 2);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "ASHNODS_BATTLEGEAR", 1, NULL);	// "Select target creature you control"
}

int card_flitterstep_eidolon(int player, int card, event_t event)
{
  /* Flitterstep Eidolon |1|U
   * Enchantment Creature - Spirit (1/1)
   * Bestow |5|U
   * ~ can't be blocked.
   * Enchanted creature gets +1/+1 and can't be blocked. */

  unblockable(player, card, event);

  return generic_creature_with_bestow(player, card, event, MANACOST_XU(5,1), 1,1, 0, SP_KEYWORD_UNBLOCKABLE);
}

int card_floodtide_serpent(int player, int card, event_t event)
{
  /* Floodtide Serpent |4|U
   * Creature - Serpent (4/4)
   * ~ can't attack unless you return an enchantment you control to its owner's hand. */

  if (trigger_condition == TRIGGER_PAY_TO_ATTACK && current_phase == PHASE_DECLARE_ATTACKERS
	  && affect_me(player, card) && reason_for_trigger_controller == player && forbid_attack == 0
	  && trigger_cause_controller == player && trigger_cause == card)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= 2;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  target_definition_t td;
		  default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
		  td.allowed_controller = player;
		  td.illegal_abilities = 0;

		  card_instance_t* instance = get_card_instance(player, card);
		  instance->number_of_targets = 0;
		  if (can_target(&td) && pick_target(&td, "TARGET_ENCHANTMENT"))
			{
			  instance->number_of_targets = 0;
			  bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
		  else
			forbid_attack = 1;
		}
	}

  return 0;
}

int card_kraken_of_the_straits(int player, int card, event_t event)
{
  /* Kraken of the Straits |5|U|U
   * Creature - Kraken (6/6)
   * Creatures with power less than the number of Islands you control can't block ~. */

  if (event == EVENT_BLOCK_LEGALITY
	  && player == attacking_card_controller && card == attacking_card
	  && get_power(affected_card_controller, affected_card) < basiclandtypes_controlled[player][get_hacked_color(player, card, COLOR_BLUE)])
	event_result = 1;

  return 0;
}

int card_meletis_astronomer(int player, int card, event_t event)
{
  /* Meletis Astronomer |1|U
   * Creature - Human Wizard (1/3)
   * Heroic - Whenever you cast a spell that targets ~, look at the top three cards of your library. You may reveal an enchantment card from among them and put
   * it into your hand. Put the rest on the bottom of your library in any order. */

  if (heroic(player, card, event))
	reveal_top_cards_of_library_and_choose_type(player, card, player, 3, 0, TUTOR_HAND, 1, TUTOR_BOTTOM_OF_DECK, 0, TYPE_ENCHANTMENT);

  return 0;
}

int card_mindreaver(int player, int card, event_t event)
{
  /* Mindreaver |U|U
   * Creature - Human Wizard (2/1)
   * Heroic - Whenever you cast a spell that targets ~, exile the top three cards of target player's library.
   * |U|U, Sacrifice ~: Counter target spell with the same name as a card exiled with ~. */

  if (heroic(player, card, event))
	{
	  target_definition_t td_player;
	  default_target_definition(player, card, &td_player, 0);
	  td_player.zone = TARGET_ZONE_PLAYERS;
	  td_player.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td_player) && pick_target(&td_player, "TARGET_PLAYER"))
		{
		  int i, tgtp = instance->targets[0].player, leg = 0, idx = 0;
		  for (i = 0; i < 3; ++i)
			if (deck_ptr[tgtp][0] != -1)
			  {
				exiledby_remember(player, card, tgtp, deck_ptr[tgtp][0], &leg, &idx);
				rfg_top_card_of_deck(tgtp);
			  }
		}
	}

#define ABILITY()				generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK|GAA_SACRIFICE_ME, MANACOST_U(2), 0, NULL, NULL)
#define FIND_IN_RFG(instance, spell)	exiledby_choose(player, card, cards_data[spell->internal_card_id].id, EXBY_FIRST_FOUND|EXBY_TEST_IID, instance->internal_card_id, NULL, 1)

  if (event == EVENT_CAN_ACTIVATE && ABILITY())
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  card_instance_t* spell = get_card_instance(card_on_stack_controller, card_on_stack);
	  if (FIND_IN_RFG(instance, spell))
		return 99;
	}

  if (event == EVENT_ACTIVATE)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  card_instance_t* spell = get_card_instance(card_on_stack_controller, card_on_stack);
	  if (FIND_IN_RFG(instance, spell))
		ABILITY();
	  else
		cancel = 1;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (counterspell_validate(player, card, NULL, 0))
		{
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

  return 0;

#undef FIND_IN_RFG
#undef ABILITY
}

const char* target_is_creature_or_aura(int who_chooses, int player, int card)
{
  if (is_what(player, card, TYPE_CREATURE) || has_subtype(player, card, SUBTYPE_AURA))
	return NULL;
  else
	return EXE_STR(0x73964C);	// ",subtype"
}

int card_nullify(int player, int card, event_t event)
{
  /* Nullify |U|U
   * Instant
   * Counter target creature or Aura spell. */

  target_definition_t td;
  counterspell_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ENCHANTMENT);
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int32_t)target_is_creature_or_aura;

  return counterspell(player, card, event, &td, 0);
}

int card_nyxborn_triton(int player, int card, event_t event)
{
  /* Nyxborn Triton |2|U
   * Enchantment Creature - Merfolk (2/3)
   * Bestow |4|U
   * Enchanted creature gets +2/+3. */

  return generic_creature_with_bestow(player, card, event, MANACOST_XU(4,1), 2,3, 0, 0);
}

int card_oracles_insight(int player, int card, event_t event)
{
  /* Oracle's Insight |3|U
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature has "|T: Scry 1, then draw a card." */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  int p = get_card_instance(player, card)->targets[9].player;
	  scry(p, 1);
	  draw_cards(p, 1);
	}

  return aura_granting_activated_ability(player, card, event, player, GAA_UNTAPPED, MANACOST_X(0), 0, NULL, NULL);
}

/* Perplexing Chimera |4|U
 * Enchantment Creature - Chimera (3/3)
 * Whenever an opponent casts a spell, you may exchange control of ~ and that spell. If you do, you may choose new targets for the spell. (If the spell becomes
 * a permanent, you control that permanent.) */

static int effect_retraction_helix(int player, int card, event_t event)
{
  if (event == EVENT_CLEANUP)
	{
	  kill_card(player, card, event);
	  return 0;
	}

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.illegal_type = TYPE_LAND;

  int rval = attachment_granting_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_NONLAND_PERMANENT");

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (validate_arbitrary_target(&td, instance->targets[0].player, instance->targets[0].card))
		bounce_permanent(instance->targets[0].player, instance->targets[0].card);
	}

  return rval;
}

int card_retraction_helix(int player, int card, event_t event)
{
  /* Retraction Helix |U
   * Instant
   * Until end of turn, target creature gains "|T: Return target nonland permanent to its owner's hand." */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  create_targetted_legacy_activate(player, card, effect_retraction_helix, instance->targets[0].player, instance->targets[0].card);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_siren_of_the_fanged_coast(int player, int card, event_t event)
{
  /* Siren of the Fanged Coast |3|U|U
   * Creature - Siren (1/1)
   * Flying
   * Tribute 3
   * When ~ enters the battlefield, if tribute wasn't paid, gain control of target creature. */

  if (tribute_wasnt_paid(player, card, event, 3))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
	}

  return 0;
}

int card_sphinxs_disciple(int player, int card, event_t event)
{
  /* Sphinx's Disciple |3|U|U
   * Creature - Human Wizard (2/2)
   * Flying
   * Inspired - Whenever ~ becomes untapped, draw a card. */

  if (inspired(player, card, event))
	draw_cards(player, 1);

  return 0;
}

int card_stratus_walk(int player, int card, event_t event)
{
  /* Stratus Walk |1|U
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield, draw a card.
   * Enchanted creature has flying.
   * Enchanted creature can block only creatures with flying. */

  if (comes_into_play(player, card, event))
	draw_cards(player, 1);

  card_instance_t* instance;
  if (event == EVENT_BLOCK_LEGALITY
	  && (instance = in_play(player, card))
	  && affect_me(instance->damage_target_player, instance->damage_target_card)
	  && !(get_abilities(attacking_card_controller, attacking_card, EVENT_ABILITIES, -1) & KEYWORD_FLYING))
	event_result = 1;

  return generic_aura(player, card, event, player, 0,0, KEYWORD_FLYING,0, 0,0,0);
}

int card_sudden_storm(int player, int card, event_t event)
{
  /* Sudden Storm |3|U
   * Instant
   * Tap up to two target creatures. Those creatures don't untap during their controllers' next untap steps. Scry 1. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int i, valid;

	  for (i = valid = 0; i < instance->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  {
			++valid;
			does_not_untap_effect(player, card, instance->targets[i].player, instance->targets[i].card, EDNT_TAP_TARGET, 1);
		  }

	  if (valid || instance->number_of_targets == 0)
		scry(player, 1);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_thassas_rebuff(int player, int card, event_t event)
{
  /* Thassa's Rebuff |1|U
   * Instant
   * Counter target spell unless its controller pays |X, where |X is your devotion to blue. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  counterspell_resolve_unless_pay_x(player, card, NULL, 0, devotion(player, card, COLOR_BLUE, 0));
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 0, NULL);
}

int card_tromokratis(int player, int card, event_t event)
{
  /* Tromokratis |5|U|U
   * Legendary Creature - Kraken (8/8)
   * ~ has hexproof unless it's attacking or blocking.
   * ~ can't be blocked unless all creatures defending player controls block it. (If any creature that player controls doesn't block this creature, it can't be
   * blocked.) */

  check_legend_rule(player, card, event);

  // Force recomputation of abilities in blocking step, since it'll lose hexproof
  if (event == EVENT_DECLARE_BLOCKERS)
	{
	  get_card_instance(player, card)->regen_status |= KEYWORD_RECALC_ABILITIES;
	  get_abilities(player, card, EVENT_ABILITIES, -1);
	}

  // Removing the creature it's blocking from combat doesn't make it stop being a blocking creature, either in Manalink or modern rules.
  if (event == EVENT_ABILITIES && affect_me(player, card) && !(get_card_instance(player, card)->state & (STATE_ATTACKING|STATE_BLOCKING)))
	hexproof(player, card, event);

  /* Blocking legality.  This is a bit complex due to multiblockers, and because Manalink checks blocking selections as they're made intead of allowing a full
   * declaration and then checking it for legality.
   *
   * We check legality in two passes.
   *
   * First pass: For each real creature defending player controls, mark it is_creature, and if it can block excluding this ability (or is already blocking
   * this), mark it can_block as well.  For each multiblock shadow that can block, mark its source card's has_multiblocker and set its multiblocker_idx to point
   * at the shadow.
   *
   * Second pass: If there's any entry marked is_creature but neither can_block nor has_multiblocker, then it's a failure.
   *
   * Then when a creature is actually selected to block this, reconstruct the first pass above; then force everything else with can_block to block, and for
   * everything else with has_multiblocker, force their multiblocker_idx to block. */

  if ((event == EVENT_BLOCK_LEGALITY && attacking_card_controller == player && attacking_card == card && event_result != 1)
	  || (trigger_condition == TRIGGER_BLOCKER_CHOSEN && (event == EVENT_TRIGGER || event == EVENT_RESOLVE_TRIGGER) && reason_for_trigger_controller == player
		  && affect_me(player, card) && current_turn == player && is_blocking(trigger_cause, card)))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (instance->targets[2].player == 17)	// We're already either checking legality recursively, or forcing everything to block.
		return 0;

	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;
	  else
		{
		  instance->targets[2].player = 17;	// Prevent reentrance

		  int band = instance->blocking == 255 ? card : instance->blocking;

		  struct
		  {
			uint8_t is_creature;
			uint8_t can_block;
			uint8_t has_multiblocker;
			uint8_t multiblocker_idx;
		  } marked[151] = {{0,0,0,0}};

		  card_instance_t* aff;
		  int c, p = 1-player;
		  // First pass.
		  for (c = 0; c < active_cards_count[p]; ++c)
			if ((aff = in_play(p, c)))
			  {
				if (cards_data[aff->internal_card_id].code_pointer == 0x401010)
				  {
					if (aff->damage_source_player == p && is_legal_block(p, c, player, card))
					  {
						marked[aff->damage_source_card].has_multiblocker = 1;
						marked[aff->damage_source_card].multiblocker_idx = c;
					  }
				  }
				else if (is_what(p, c, TYPE_CREATURE))
				  {
					marked[c].is_creature = 1;
					if (aff->blocking == band)
					  marked[c].can_block = 2;	// already blocking
					else
					  marked[c].can_block = is_legal_block(p, c, player, card) ? 1 : 0;	// can block
				  }
			  }

		  if (event == EVENT_BLOCK_LEGALITY)
			{
			  // Second pass.
			  for (c = 0; c < active_cards_count[p]; ++c)
				if (marked[c].is_creature && !marked[c].can_block && !marked[c].has_multiblocker)
				  {
					event_result = 1;
					break;
				  }
			}
		  else	// resolving TRIGGER_BLOCKER_CHOSEN
			{
			  marked[trigger_cause].can_block = 2;	// It just blocked.

			  for (c = 0; c < active_cards_count[p]; ++c)
				if (marked[c].is_creature)
				  {
					if (marked[c].can_block == 1)
					  block(p, c, player, card);
					else if (marked[c].has_multiblocker)
					  block(p, marked[c].multiblocker_idx, player, card);
				  }
			}

		  // Allow reentrance
		  instance->targets[2].player = 0;
		}
	}

  return 0;
}

int card_vortex_elemental(int player, int card, event_t event)
{
  /* Vortex Elemental |U
   * Creature - Elemental (0/1)
   * |U: Put ~ and each creature blocking or blocked by it on top of their owners' libraries, then those players shuffle their libraries.
   * |3|U|U: Target creature blocks ~ this turn if able. */

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  int ai_shuffle = (current_turn == player
						? ((instance->state & STATE_ATTACKING) && !is_unblocked(player, card))
						: ((instance->state & STATE_BLOCKING) && instance->blocking < 255 && in_play(1-player, instance->blocking))) ? 10 : 1;

	  int ai_force_block = (current_turn == player && current_phase < PHASE_DECLARE_BLOCKERS) ? 5 : 0;

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);

	  enum
	  {
		CHOICE_SHUFFLE = 1,
		CHOICE_FORCE_BLOCK = 2
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Shuffle into library", 1, ai_shuffle, DLG_MANA(MANACOST_U(1)),
						"Block if able", can_target(&td), ai_force_block, DLG_MANA(MANACOST_XU(3,2)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice && can_use_activated_abilities(player, card);
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_SHUFFLE:
			  if (ai_shuffle >= 10)
				ai_modifier += 64;
			  break;

			case CHOICE_FORCE_BLOCK:
			  instance->number_of_targets = 0;
			  pick_target(&td, "TARGET_CREATURE");
			  break;
		  }
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_SHUFFLE:
			  if (in_play(instance->parent_controller, instance->parent_card))
				{
				  char marked[2][151] = {{0}};
				  char should_shuffle[2] = {0};

				  marked[instance->parent_controller][instance->parent_card] = 1;
				  should_shuffle[get_owner(instance->parent_controller, instance->parent_card)] = 1;

				  if (instance->parent_controller == current_turn)
					mark_each_creature_blocking_me(instance->parent_controller, instance->parent_card, marked);
				  else
					mark_each_creature_blocked_by_me(instance->parent_controller, instance->parent_card, marked);

				  APNAP(p,
						{
						  int c;
						  for (c = 0; c < active_cards_count[p]; ++c)
							if (marked[p][c]
								&& in_play(p, c))
							  {
								should_shuffle[get_owner(p, c)] = 1;
								// Since leave-play triggers will (erroneously) run as each creature leaves play, this may have already left play :(
								put_on_top_of_deck(p, c);
							  }
						});
				  APNAP(p, { if (should_shuffle[p]) shuffle(p); });
				}
			  break;

			case CHOICE_FORCE_BLOCK:
			  if (valid_target(&td))
				target_must_block_me(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1);
			  break;
		  }
	}

  return 0;
}

int card_whelming_wave(int player, int card, event_t event)
{
  /* Whelming Wave |2|U|U
   * Sorcery
   * Return all creatures to their owners' hands except for Krakens, Leviathans, Octopuses and Serpents. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int p, c;
	  int marked[2][151] = {{0}};

	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_CREATURE)
			  && !has_subtype(p, c, SUBTYPE_KRAKEN)
			  && !has_subtype(p, c, SUBTYPE_LEVIATHAN)
			  && !has_subtype(p, c, SUBTYPE_OCTOPUS)
			  && !has_subtype(p, c, SUBTYPE_SERPENT))
			marked[p][c] = 1;

	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (marked[p][c])
			bounce_permanent(p, c);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/********
* Black *
********/

int card_archetype_of_finality(int player, int card, event_t event)
{
  /* Archetype of Finality |4|B|B
   * Enchantment Creature - Gorgon (2/3)
   * Creatures you control have deathtouch.
   * Creatures your opponents control lose deathtouch and can't have or gain deathtouch. */

  if (event == EVENT_CAN_CAST)
	return 1;

  return archetype(player, event, 0, SP_KEYWORD_DEATHTOUCH, PB_CANT_HAVE_OR_GAIN_DEATHTOUCH);
}

int card_ashioks_adept(int player, int card, event_t event)
{
  /* Ashiok's Adept |2|B
   * Creature - Human Wizard (1/3)
   * Heroic - Whenever you cast a spell that targets ~, each opponent discards a card. */

  if (hand_count[1-player] >= 1 && heroic(player, card, event))
	discard(1-player, 0, player);

  return 0;
}

int card_asphyxiate(int player, int card, event_t event)
{
  /* Asphyxiate |1|B|B
   * Sorcery
   * Destroy target untapped creature. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.illegal_state = TARGET_STATE_TAPPED;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (valid_target(&td))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_UNTAPPED_CREATURE", 1, NULL);
}

int card_bile_blight(int player, int card, event_t event)
{
  /* Bile Blight |B|B
   * Instant
   * Target creature and all creatures with the same name as that creature get -3/-3 until end of turn. */

  return echoing_pump(player, card, event, -3, -3);
}

int card_black_oak_of_odunos(int player, int card, event_t event)
{
  /* Black Oak of Odunos |2|B
   * Creature - Zombie Treefolk (0/5)
   * Defender
   * |B, Tap another untapped creature you control: ~ gets +1/+1 until end of turn. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.illegal_abilities = 0;
  td.illegal_state = TARGET_STATE_TAPPED;
  td.special = TARGET_SPECIAL_NOT_ME;

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->parent_controller, c = instance->parent_card;
	  if (in_play(p, c))
		pump_until_eot_merge_previous(p, c, p, c, 1,1);
	}

  int rval = generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_LITERAL_PROMPT, MANACOST_B(1), 0, &td, "Select another untapped creature you control.");

  if (event == EVENT_ACTIVATE && cancel != 1)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  tap_card(instance->targets[0].player, instance->targets[0].card);
	  instance->number_of_targets = 0;	// Prevents when-this-becomes-targeted triggers
	}

  return rval;
}

int card_champion_of_stray_souls(int player, int card, event_t event)
{
  /* Champion of Stray Souls |4|B|B
   * Creature - Skeleton Warrior (4/4)
   * |3|B|B, |T, Sacrifice X other creatures: Return |X target creatures from your graveyard to the battlefield.
   * |5|B|B: Put ~ on top of your library from your graveyard. */

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_XB(3,2))
	  && !is_tapped(player, card) && !is_sick(player, card))	// Don't check gy for creatures/shroud or bf for saccable creatures, since X can be 0
	return 1;

  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST_XB(3,2)))
	{
	  tap_card(player, card);

	  test_definition_t test_sac;
	  new_default_test_definition(&test_sac, TYPE_CREATURE, "Select another creature to sacrifice.");
	  test_sac.not_me = 1;

	  int max_cards = count_graveyard_by_type(player, TYPE_CREATURE);
	  int max_sac = max_can_sacrifice_as_cost(player, card, &test_sac);
	  max_cards = MIN(max_cards, max_sac);
	  max_cards = MIN(max_cards, 10);
	  if (max_cards <= 0 || graveyard_has_shroud(player))
		{
		  ai_modifier -= 48;
		  instance->info_slot = 0;
		  return 0;
		}

	  char buf[100];
	  if (ai_is_speculating == 1)
		*buf = 0;
	  else if (max_cards == 1)
		strcpy(buf, "Select target creature card.");
	  else
		sprintf(buf, "Select up to %d target creature cards.", max_cards);

	  test_definition_t test_animate;
	  new_default_test_definition(&test_animate, TYPE_CREATURE, buf);

	  instance->info_slot = select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, &test_animate, max_cards, &instance->targets[0]);

	  int i;
	  for (i = 0; i < instance->info_slot; ++i)
		if (!new_sacrifice(player, card, player, 1, &test_sac))
		  {
			spell_fizzled = 1;
			break;
		  }
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (instance->info_slot <= 0 || graveyard_has_shroud(player))
		return 0;

	  // Move out of graveyard first, then animate one at a time, to prevent strange interactions with Gravedigger, Angel of Glory's Rise, etc.
	  int i, selected, dead[10];
	  for (i = 0; i < 10; ++i)
		if (i < instance->info_slot && (selected = validate_target_from_grave(player, card, player, i)) != -1)
		  {
			dead[i] = get_grave(player)[selected];
			obliterate_card_in_grave(player, selected);
		  }
		else
		  dead[i] = -1;

	  for (i = 0; i < 10; ++i)
		if (dead[i] != -1)
		  {
			int pos = raw_put_iid_on_top_of_graveyard(player, dead[i]);
			increase_trap_condition(player, TRAP_CARDS_TO_GY_FROM_ANYWHERE, -1);	// Since not really putting in graveyard
			reanimate_permanent(player, card, player, pos, REANIMATE_DEFAULT);
		  }
	}

  if (event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, MANACOST_XB(5,2)))
	return GA_PUT_ON_TOP_OF_DECK;

	if (event == EVENT_PAY_FLASHBACK_COSTS){
		charge_mana_multi(player, MANACOST_XB(5,2));
		if( spell_fizzled != 1 ){
			return GAPAID_REMOVE;
		}
	}

  return 0;
}

int card_claim_of_erebos(int player, int card, event_t event)
{
  /* Claim of Erebos |1|B
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature has "|1|B, |T: Target player loses 2 life." */

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  int rval = aura_granting_activated_ability(player, card, event, player, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_XB(1,1), 0, &td, "TARGET_PLAYER");

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (validate_arbitrary_target(&td, instance->targets[0].player, instance->targets[0].card))
		lose_life(instance->targets[0].player, 2);
	}

  return rval;
}

int card_drown_in_sorrow(int player, int card, event_t event)
{
  /* Drown in Sorrow |1|B|B
   * Sorcery
   * All creatures get -2/-2 until end of turn. Scry 1. */

	if (event == EVENT_CAN_CAST)
	  return 1;

	if (event == EVENT_RESOLVE_SPELL)
	  {
		pump_creatures_until_eot(player, card, ANYBODY, 0, -2,-2, 0,0, NULL);
		scry(player, 1);
		kill_card(player, card, KILL_DESTROY);
	  }

	return 0;
}

int card_eater_of_hope(int player, int card, event_t event)
{
  /* Eater of Hope |5|B|B
   * Creature - Demon (6/4)
   * Flying
   * |B, Sacrifice another creature: Regenerate ~.
   * |2|B, Sacrifice two other creatures: Destroy target creature. */

  if (!IS_ACTIVATING(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allow_cancel = 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select another creature to sacrifice.");
  test.not_me = 1;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card))
	{
	  if (land_can_be_played & LCBP_REGENERATION)
		return (can_regenerate(player, card) && has_mana_for_activated_ability(player, card, MANACOST_B(1))
				&& new_can_sacrifice_as_cost(player, card, &test)) ? 99 : 0;
	  else
		{
		  test.qty = 2;
		  return can_target(&td) && has_mana_for_activated_ability(player, card, MANACOST_XB(2,1)) && new_can_sacrifice_as_cost(player, card, &test);
		}
	}

  if (event == EVENT_ACTIVATE)
	{
	  instance->info_slot = 0;
	  if (land_can_be_played & LCBP_REGENERATION)
		{
		  if (charge_mana_for_activated_ability(player, card, MANACOST_B(1)) && new_sacrifice(player, card, player, 0, &test))
			instance->info_slot = 1;

		  instance->number_of_targets = 0;
		}
	  else
		{
		  instance->number_of_targets = 0;
		  if (charge_mana_for_activated_ability(player, card, MANACOST_XB(2,1))
			  && new_sacrifice(player, card, player, 0, &test)
			  && new_sacrifice(player, card, player, SAC_NO_CANCEL, &test)
			  && can_target(&td)	// It's possible, though unlikely, that there may no longer be any legal targets (e.g. if this has shroud)
			  && pick_target(&td, "TARGET_CREATURE"))
			{
			  instance->number_of_targets = 1;
			  instance->info_slot = 2;
			}
		}
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (instance->info_slot == 1)
		regenerate_target(instance->parent_controller, instance->parent_card);
	  if (instance->info_slot == 2 && valid_target(&td))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

  return 0;
}

int card_eye_gouge(int player, int card, event_t event)
{
  /* Eye Gouge |B
   * Instant
   * Target creature gets -1/-1 until end of turn. If it's a Cyclops, destroy it. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.illegal_state = TARGET_STATE_TAPPED;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (valid_target(&td))
		{
		  pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		  if (in_play(instance->targets[0].player, instance->targets[0].card)
			  && has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_CYCLOPS))
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_fate_unraveler(int player, int card, event_t event)
{
  /* Fate Unraveler |3|B
   * Enchantment Creature - Hag (3/4)
   * Whenever an opponent draws a card, ~ deals 1 damage to that player. */

  /* Underworld Dreams	|B|B|B
   * Enchantment
   * Whenever an opponent draws a card, ~ deals 1 damage to him or her. */

  if (card_drawn_trigger(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY))
	damage_player(1-player, 1, player, card);

  return global_enchantment(player, card, event);
}

static int effect_indestructible(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);
  indestructible(instance->damage_target_player, instance->damage_target_card, event);
  return 0;
}

int card_fated_return(int player, int card, event_t event)
{
  /* Fated Return |4|B|B|B
   * Instant
   * Put target creature card from a graveyard onto the battlefield under your control. It gains indestructible. If it's your turn, scry 2. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	if( event == EVENT_RESOLVE_SPELL){
		card_instance_t *instance = get_card_instance( player, card );
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			reanimate_permanent_with_effect(player, card, instance->targets[0].player, selected, REANIMATE_DEFAULT, &effect_indestructible);
			if (player == current_turn){
				scry(player, 2);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER_BOTH_GRAVES, NULL, NULL, 0, &this_test);
}

int card_felhide_brawler(int player, int card, event_t event)
{
  /* Felhide Brawler |1|B
   * Creature - Minotaur (2/2)
   * ~ can't block unless you control another Minotaur. */

  if (event == EVENT_BLOCK_LEGALITY && affect_me(player, card))
	{
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (c != card
			&& in_play(player, c)
			&& has_subtype(player, c, SUBTYPE_MINOTAUR)
			// Just in case there's some weird Tribal Instant/Sorcery - Minotaur card printed that's somehow castable during EVENT_BLOCK_LEGALITY:
			&& is_what(player, c, TYPE_PERMANENT))
		  return 0;

	  // otherwise
	  event_result = 1;
	}

  return 0;
}

int card_forlorn_pseudamma(int player, int card, event_t event)
{
  /* Forlorn Pseudamma |3|B
   * Creature - Zombie (2/1)
   * Intimidate
   * Inspired - Whenever ~ becomes untapped, you may pay |2|B. If you do, put a 2/2 black Zombie enchantment creature token onto the battlefield. */

  if (event == EVENT_ABILITIES && affect_me(player, card))
	intimidate(player, card, event);

  if (inspired(player, card, event) && has_mana_multi(player, MANACOST_XB(2,1))
	  && DIALOG(player, card, EVENT_ACTIVATE, DLG_NO_STORAGE, "Generate a Zombie", 1, 1)
	  && charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_XB(2,1)))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
	  token.special_flags2 = SF2_ENCHANTED_EVENING;
	  generate_token(&token);
	}

  return 0;
}

int card_forsaken_drifters(int player, int card, event_t event)
{
  /* Forsaken Drifters |3|B
   * Creature - Zombie (4/2)
   * When ~, dies, put the top four cards of your library into your graveyard. */

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	mill(player, 4);

  return 0;
}

int card_gild(int player, int card, event_t event)
{
  /* Gild |3|B
   * Sorcery
   * Exile target creature. Put a colorless artifact token named Gold onto the battlefield. It has "Sacrifice this artifact: Add one mana of any color to your
   * mana pool." */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (valid_target(&td))
		{
		  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		  generate_token_by_id(player, card, CARD_ID_GOLD);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_grisly_transformation(int player, int card, event_t event)
{
  /* Grisly Transformation |2|B
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield, draw a card.
   * Enchanted creature has intimidate. */

  if (comes_into_play(player, card, event))
	draw_cards(player, 1);

  return generic_aura(player, card, event, player, 0,0, 0,SP_KEYWORD_INTIMIDATE, 0,0,0);
}

int card_herald_of_torment(int player, int card, event_t event)
{
  /* Herald of Torment |1|B|B
   * Enchantment Creature - Demon (3/3)
   * Bestow |3|B|B
   * Flying
   * At the beginning of your upkeep, you lose 1 life.
   * Enchanted creature gets +3/+3 and has flying. */

  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	lose_life(player, 1);

  return generic_creature_with_bestow(player, card, event, MANACOST_XB(3,2), 3, 3, KEYWORD_FLYING, 0);
}

int card_marshmist_titan(int player, int card, event_t event)
{
  /* Marshmist Titan |6|B
   * Creature - Giant (4/5)
   * ~ costs |X less to cast, where X is your devotion to black. */

  if (event == EVENT_MODIFY_COST)
	COST_COLORLESS -= devotion(player, card, COLOR_BLACK, 0);

  return 0;
}

/* Necrobite |2|B => avacyn_restored.c
 * Instant
 * Target creature gains deathtouch until end of turn. Regenerate it. */

int card_nyxborn_eidolon(int player, int card, event_t event)
{
  /* Nyxborn Eidolon |1|B
   * Enchantment Creature - Spirit (2/1)
   * Bestow |4|B
   * Enchanted creature gets +2/+1. */

  return generic_creature_with_bestow(player, card, event, MANACOST_XB(4,1), 2,1, 0, 0);
}

int card_odunos_river_trawler(int player, int card, event_t event)
{
  /* Odunos River Trawler |2|B
   * Creature - Zombie (2/2)
   * When ~ enters the battlefield, return target enchantment creature card from your graveyard to your hand.
   * |W, Sacrifice ~: Return target enchantment creature card from your graveyard to your hand. */

#define DECL_TEST(test)	test_definition_t test;																							\
						new_default_test_definition(&test, TYPE_ENCHANTMENT|TYPE_CREATURE, "Select target enchantment creature card.");	\
						test.type_flag = F1_MATCH_ALL

  if (comes_into_play(player, card, event))
	{
	  DECL_TEST(test);
	  if (new_special_count_grave(player, &test) > 0)
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &test);
	}

  if (event == EVENT_CAN_ACTIVATE)
	{
	  if (!has_mana_for_activated_ability(player, card, MANACOST_W(1))
		  || !can_sacrifice_this_as_cost(player, card)
		  || !can_use_activated_abilities(player, card)
		  || graveyard_has_shroud(player))
		return 0;

	  DECL_TEST(test);
	  if (new_special_count_grave(player, &test) > 0)
		return 1;
	}

  if (event == EVENT_ACTIVATE)
	{
	  DECL_TEST(test);
	  if (charge_mana_for_activated_ability(player, card, MANACOST_W(1))
		  && new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &test, 0) != -1)
		kill_card(player, card, KILL_SACRIFICE);
	  else
		spell_fizzled = 1;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  int selected;
	  if ((selected = validate_target_from_grave(player, card, player, 0)) != -1)
		from_grave_to_hand(player, selected, TUTOR_HAND);
	  else
		spell_fizzled = 1;
	}

  return 0;
#undef DECL_TEST
}

int card_pain_seer(int player, int card, event_t event)
{
  /* Pain Seer |1|B
   * Creature - Human Wizard (2/2)
   * Inspired - Whenever ~ becomes untapped, reveal the top card of your library and put that card into your hand. You lose life equal to that card's converted
   * mana cost. */

  if (inspired(player, card, event))
	dark_confidant_effect(player, card, player);

  return 0;
}

int card_sanguimancy(int player, int card, event_t event)
{
  /* Sanguimancy |4|B
   * Sorcery
   * You draw X cards and you lose X life, where X is your devotion to black. */

  if (event == EVENT_CAN_CAST)
	return 1;


  if (event == EVENT_RESOLVE_SPELL)
	{
	  int dev = devotion(player, card, COLOR_BLACK, 0);
	  if (dev > 0)
		{
		  draw_cards(player, dev);
		  lose_life(player, dev);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_servant_of_tymaret(int player, int card, event_t event)
{
  /* Servant of Tymaret |2|B
   * Creature - Zombie (1/3)
   * Inspired - Whenever ~ becomes untapped, each opponent loses 1 life. You gain life equal to the life lost this way.
   * |2|B: Regenerate ~. */

  if (inspired(player, card, event))
	gain_life(player, lose_life(1-player, 1));

  return regeneration(player, card, event, MANACOST_XB(2,1));
}

int card_shrike_harpy(int player, int card, event_t event)
{
  /* Shrike Harpy |3|B|B
   * Creature - Harpy (2/2)
   * Flying
   * Tribute 2
   * When ~ enters the battlefield, if tribute wasn't paid, target opponent sacrifices a creature. */

  if (tribute_wasnt_paid(player, card, event, 2) && target_opponent(player, card))
	player_sacrifices_a_permanent(player, card, get_card_instance(player, card)->targets[0].player, TYPE_CREATURE, SAC_NO_CANCEL);

  return 0;
}

int card_spiteful_returned(int player, int card, event_t event)
{
  /* Spiteful Returned |1|B
   * Enchantment Creature - Zombie (1/1)
   * Bestow |3|B
   * Whenever ~ or enchanted creature attacks, defending player loses 2 life.
   * Enchanted creature gets +1/+1. */

  card_instance_t* instance = in_play(player, card);
  if (declare_attackers_trigger(player, card, event, 0, player, card)
	  || (instance && declare_attackers_trigger(player, card, event, 0, instance->damage_target_player, instance->damage_target_card)))
	lose_life(1-current_turn, 2);

  return generic_creature_with_bestow(player, card, event, MANACOST_XB(3,1), 1,1, 0, 0);
}

int card_warchanter_of_mogis(int player, int card, event_t event)
{
  /* Warchanter of Mogis |3|B|B
   * Creature - Minotaur Shaman (3/3)
   * Inspired - Whenever ~ becomes untapped, target creature you control gains intimidate until end of turn. */

  if (inspired(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;
	  td.allowed_controller = player;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "ASHNODS_BATTLEGEAR"))	// "Select target creature you control."
		pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0,0, 0, SP_KEYWORD_INTIMIDATE);
	}

  return 0;
}

int card_weight_of_the_underworld(int player, int card, event_t event)
{
  /* Weight of the Underworld |3|B
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets -3/-2. */

  return generic_aura(player, card, event, 1-player, -3,-2, 0,0, 0,0,0);
}

/******
* Red *
******/

int card_akroan_conscriptor(int player, int card, event_t event)
{
  /* Akroan Conscriptor |4|R
   * Creature - Human Shaman (3/2)
   * Heroic - Whenever you cast a spell that targets ~, gain control of another target creature until end of turn. Untap that creature. It gains haste until end
   * of turn. */

  if (heroic(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.special = TARGET_SPECIAL_NOT_ME;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_ANOTHER_CREATURE"))
		effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
	}

  return 0;
}

int card_archetype_of_aggression(int player, int card, event_t event)
{
  /* Archetype of Aggression |1|R|R
   * Enchantment Creature - Human Warrior (3/2)
   * Creatures you control have trample.
   * Creatures your opponents control lose trample and can't have or gain trample. */

  if (event == EVENT_CAN_CAST)
	return 1;

  return archetype(player, event, KEYWORD_TRAMPLE, 0, PB_CANT_HAVE_OR_GAIN_TRAMPLE);
}

int card_bolt_of_keranos(int player, int card, event_t event)
{
  /* Bolt of Keranos |1|R|R
   * Sorcery
   * ~ deals 3 damage to target creature or player. Scry 1. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (valid_target(&td))
		{
		  damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
		  scry(player, 1);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

/* Cyclops of One-Eyed Pass |2|R|R => vanilla
 * Creature - Cyclops (5/2) */

int card_epiphany_storm(int player, int card, event_t event)
{
  /* Epiphany Storm |R
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature has "|R, |T, Discard a card: Draw a card." */

  if (event == EVENT_RESOLVE_ACTIVATION)
	draw_cards(get_card_instance(player, card)->targets[9].player, 1);

  return aura_granting_activated_ability(player, card, event, player, GAA_UNTAPPED|GAA_DISCARD, MANACOST_R(1), 0, NULL, NULL);
}

int card_everflame_eidolon(int player, int card, event_t event)
{
  /* Everflame Eidolon |1|R
   * Enchantment Creature - Spirit (1/1)
   * Bestow |2|R
   * |R: ~ gets +1/+0 until end of turn. If it's an Aura, enchanted creature gets +1/+0 until end of turn instead.
   * Enchanted creature gets +1/+1. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = instance->parent_controller, c = instance->parent_card, tp, tc;

	  card_instance_t* parent = get_card_instance(p, c);
	  if (parent->damage_target_player >= 0)
		{
		  tp = parent->damage_target_player;
		  tc = parent->damage_target_card;
		}
	  else
		{
		  tp = p;
		  tc = c;
		}

	  if (in_play(tp, tc))
		pump_until_eot_merge_previous(p, c, tp, tc, 1,0);

	  return 0;
	}

  return generic_creature_with_bestow(player, card, event, MANACOST_XR(2,1), 1,1, 0, 0) || generic_shade_merge_pt(player, card, event, 0, MANACOST_R(1), 1,0);
}

int card_fall_of_the_hammer(int player, int card, event_t event)
{
  /* Fall of the Hammer |1|R
   * Instant
   * Target creature you control deals damage equal to its power to another target creature. */

  if (!(event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) || event == EVENT_RESOLVE_SPELL))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td_control;
  default_target_definition(player, card, &td_control, TYPE_CREATURE);
  td_control.allowed_controller = td_control.preferred_controller = player;

  target_definition_t td_another;
  default_target_definition(player, card, &td_another, TYPE_CREATURE);

  if (event == EVENT_CAN_CAST)
	{
	  if (ai_is_speculating == 1 || (player == AI && !(trace_mode & 2)))
		{
		  td_another.allowed_controller = 1-player;
		  return can_target(&td_control) && can_target(&td_another);
		}
	  else
		return can_target(&td_control) && target_available(player, card, &td_another) >= 2;
	}

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  instance->number_of_targets = 0;
	  if (pick_target(&td_control, "ASHNODS_BATTLEGEAR"))	// "Select target creature you control."
		{
		  state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
		  new_pick_target(&td_another, "TARGET_ANOTHER_CREATURE", 1, 0);
		  state_untargettable(instance->targets[0].player, instance->targets[0].card, 0);
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int p0 = instance->targets[0].player;
	  int c0 = instance->targets[0].card;

	  int p1 = instance->targets[1].player;
	  int c1 = instance->targets[1].card;

	  if (valid_target(&td_control)
		  && validate_target(player, card, &td_another, 1)
		  && !(p0 == p1 && c0 == c1))	// Though how it would've gotten here, I dunno
		damage_creature(p1, c1, get_power(p0, c0), p0, c0);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_fated_conflagration(int player, int card, event_t event)
{
  /* Fated Conflagration |1|R|R|R
   * Instant
   * ~ deals 5 damage to target creature or planewalker. If it's your turn, scry 2. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE | TARGET_TYPE_PLANESWALKER);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (valid_target(&td))
		{
		  damage_creature(instance->targets[0].player, instance->targets[0].card, 5, player, card);

		  if (player == current_turn)
			scry(player, 2);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLANESWALKER", 1, NULL);
}

int card_fearsome_temper(int player, int card, event_t event)
{
  /* Fearsome Temper |2|R
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +2/+2 and has "|2|R: Target creature can't block this creature this turn." */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  int rval = aura_granting_activated_ability(player, card, event, player, GAA_CAN_TARGET, MANACOST_XR(2,1), 0, &td, "TARGET_CREATURE");

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (validate_arbitrary_target(&td, instance->targets[0].player, instance->targets[0].card))
		creature1_cant_block_creature2_until_eot(instance->parent_controller, instance->parent_card,
												 instance->targets[0].player, instance->targets[0].card,
												 instance->targets[9].player, instance->targets[9].card);
	}

  if (event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  card_instance_t* instance = in_play(player, card);
	  if (instance && affect_me(instance->damage_target_player, instance->damage_target_card))
		event_result += 2;
	}

  return rval;
}

int card_felhide_spiritbinder(int player, int card, event_t event)
{
  /* Felhide Spiritbinder |3|R
   * Creature - Minotaur Shaman (3/4)
   * Inspired - Whenever ~ becomes untapped, you may pay |1|R. If you do, put a token onto the battlefield that's a copy of another target creature except it's
   * an enchantment in addition to its other types. It gains haste. Exile it at the beginning of the next end step. */

  if (inspired(player, card, event) && has_mana_multi(player, MANACOST_XR(1,1)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.special = TARGET_SPECIAL_NOT_ME;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td)
		  && DIALOG(player, card, EVENT_ACTIVATE, DLG_NO_STORAGE, "Duplicate a creature", 1, 1)
		  && charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_XR(1,1))
		  && pick_target(&td, "TARGET_ANOTHER_CREATURE"))
		{
		  token_generation_t token;
		  copy_token_definition(player, card, &token, instance->targets[0].player, instance->targets[0].card);
		  // If this is copying something that's a true token, this robustly makes it an enchantment
		  token.special_flags2 |= SF2_ENCHANTED_EVENING;
		  token.legacy = 1;
		  // If this is copying something that's not a true token, this makes it an enchantment; but further copies won't be
		  token.special_code_for_legacy = &effect_become_enchantment_and_haste_and_remove_eot;
		  generate_token(&token);
		}
	}

  return 0;
}

int card_flame_wreathed_phoenix(int player, int card, event_t event)
{
  /* Flame-Wreathed Phoenix |2|R|R
   * Creature - Phoenix (3/3)
   * Flying
   * Tribute 2
   * When ~ enters the battlefield, if tribute wasn't paid, it gains haste and "When this creature dies, return it to its owner's hand." */

  card_instance_t* instance = get_card_instance(player, card);

  if (tribute_wasnt_paid(player, card, event, 2))
	{
	  ai_modifier -= 24;	// To represent the eventual loss of card advantage.
	  instance->targets[4].player = 17;	// Stored in targets, not info slot, since that's not copied during this_dies_trigger() (called by Endless Cockroaches)
	  create_targetted_legacy_effect(player, card, &empty, player, card);	// make visually distinct
	}

  if (instance->targets[4].player == 17)
	{
	  if (event == EVENT_ABILITIES && affect_me(player, card))
		haste(player, card, event);

	  card_endless_cockroaches(player, card, event);
	}

  return 0;
}

int card_forgestoker_dragon(int player, int card, event_t event)
{
  /* Forgestoker Dragon |4|R|R
   * Creature - Dragon (5/4)
   * Flying
   * |1|R: ~ deals 1 damage to target creature. That creature can't block this combat. Activate this ability only if ~ is attacking. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_CAN_ACTIVATE && !is_attacking(player, card))
	return 0;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  int leg = pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
	  if (leg >= 0)
		{
		  card_instance_t* legacy = get_card_instance(instance->parent_controller, leg);
		  legacy->targets[3].player = PAUE_END_AT_END_OF_COMBAT;
		}

	  damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, card);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(1,1), 0, &td, "TARGET_CREATURE");
}

/* Impetuous Sunchaser |1|R => scars_of_mirrodin.c:card_reckless_brute
 * Creature - Human Soldier (1/1)
 * Flying, haste
 * ~ attacks each turn if able. */

int card_kragma_butcher(int player, int card, event_t event)
{
  /* Kragma Butcher |2|R
   * Creature - Minotaur Warrior (2/3)
   * Inspired - Whenever ~ becomes untapped, it gets +2/+0 until end of turn. */

  if (inspired(player, card, event))
	pump_until_eot_merge_previous(player, card, player, card, 2,0);

  return 0;
}

int effect_flame_fusillade(int player, int card, event_t event);
int card_lightning_volley(int player, int card, event_t event)
{
  /* Lightning Volley |3|R
   * Instant
   * Until end of turn, creatures you control gain "|T: This creature deals 1 damage to target creature or player." */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int leg = create_legacy_activate(player, card, &effect_flame_fusillade);
	  if (leg >= 0)
		get_card_instance(player, leg)->targets[2].player = TYPE_CREATURE;

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_nyxborn_rollicker(int player, int card, event_t event)
{
  /* Nyxborn Rollicker |R
   * Enchantment Creature - Satyr (1/1)
   * Bestow |1|R
   * Enchanted creature gets +1/+1. */

  return generic_creature_with_bestow(player, card, event, MANACOST_XR(1,1), 1,1, 0, 0);
}

int card_oracle_of_bones(int player, int card, event_t event)
{
  /* Oracle of Bones |2|R|R
   * Creature - Minotaur Shaman (3/1)
   * Haste
   * Tribute 2
   * When ~ enters the battlefield, if tribute wasn't paid, you may cast an instant or sorcery card from your hand without paying its mana cost. */

  if (event == EVENT_ABILITIES && affect_me(player, card))
	haste(player, card, event);

  if (tribute_wasnt_paid(player, card, event, 2))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_INSTANT|TYPE_SORCERY);
	  td.zone = TARGET_ZONE_HAND;
	  td.illegal_abilities = 0;
	  td.allowed_controller = player;
	  td.preferred_controller = player;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;

	  if (can_target(&td) && pick_next_target_noload(&td, "Select an instant or sorcery card from your hand."))
		{
		  instance->number_of_targets = 0;
		  play_card_in_hand_for_free(instance->targets[0].player, instance->targets[0].card);
		}
	}

  return 0;
}

int card_pharagax_giant(int player, int card, event_t event)
{
  /* Pharagax Giant |4|R
   * Creature - Giant (3/3)
   * Tribute 2
   * When ~ enters the battlefield, if tribute wasn't paid, ~ deals 5 damage to each opponent. */

  if (tribute_wasnt_paid(player, card, event, 2))
	damage_player(1-player, 5, player, card);

  return 0;
}

int card_pinnacle_of_rage(int player, int card, event_t event)
{
  /* Pinnacle of Rage |4|R|R
   * Sorcery
   * ~ deals 3 damage to each of two target creatures and/or players. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int i;

	  for (i = 0; i < 2; ++i)
		if (validate_target(player, card, &td, i))
		  damage_creature(instance->targets[i].player, instance->targets[i].card, 3, player, card);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 2, NULL);
}

/* Reckless Reveler |1|R => dark_ascension.c:card_torch_fiend
 * Creature - Satyr (2/1)
 * |R, Sacrifice ~: Destroy target artifact. */

/* Rise to the Challenge |1|R => m11.c:card_thunder_strike
 * Instant
 * Target creature gets +2/+0 and gains first strike until end of turn. */

int card_satyr_firedancer(int player, int card, event_t event)
{
  /* Satyr Firedancer |1|R
   * Enchantment Creature - Satyr (1/1)
   * Whenever an instant or sorcery spell you control deals damage to an opponent, ~ deals that much damage to target creature that player controls. */

  if (event == EVENT_CAN_CAST)
	return 1;

  /* This is probably overkill; it's difficult to get an instant or sorcery to damage your opponent multiple times at once without it being a bug (e.g.,
   * Electrolyze should deal one packet of 2 damage, not two packets of 1, if it's only damaging one target), and even harder to get multiple instants or
   * sorceries to damage simultaneously. */
  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && damage->damage_source_player == player
	  && (damage->targets[3].player & (TYPE_INSTANT|TYPE_SORCERY))
	  && damage->damage_target_player != player && damage->damage_target_card == -1 && !damage_is_to_planeswalker(damage))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (instance->info_slot < 10)
		instance->targets[instance->info_slot++].card = damage->info_slot;
	}

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->info_slot <= 0)
		return 0;

	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  target_definition_t td;
		  default_target_definition(player, card, &td, TYPE_CREATURE);
		  td.allowed_controller = 1-player;
		  td.allow_cancel = 0;

		  int amts[10];

		  instance->number_of_targets = 0;
		  int i;
		  for (i = 0; i < instance->info_slot && can_target(&td); ++i)
			{
			  int amt = instance->targets[i].card;

			  char prompt[200];
			  if (ai_is_speculating == 1)
				prompt[0] = 0;
			  else
				{
				  load_text(0, "TARGET_CREATURE_OPPONENT_CONTROLS");
				  scnprintf(prompt, 200, "%s %s (%d): %s", EXE_STR(0x738F34) /*PROMPT_ACTION[2]*/, cards_ptr[get_id(player, card)]->full_name, amt, text_lines[0]);
				}

			  if (pick_next_target_noload(&td, prompt))
				amts[instance->number_of_targets - 1] = amt;
			}

		  instance->info_slot = 0;

		  for (i = 0; i < instance->number_of_targets; ++i)
			damage_creature(instance->targets[i].player, instance->targets[i].card, amts[i], player, card);
		}
	}

  return 0;
}

int card_satyr_nyx_smith(int player, int card, event_t event)
{
  /* Satyr Nyx-Smith |2|R
   * Creature - Satyr Shaman (2/1)
   * Haste
   * Inspired - Whenever ~ becomes untapped, you may pay |2|R. If you do, put a 3/1 red Elemental enchantment creature token with haste onto the battlefield. */

  if (event == EVENT_ABILITIES && affect_me(player, card))
	haste(player, card, event);

  if (inspired(player, card, event) && has_mana_multi(player, MANACOST_XR(2,1))
	  && DIALOG(player, card, EVENT_ACTIVATE, DLG_NO_STORAGE, "Generate an Elemental", 1, 1)
	  && charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_XR(2,1)))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
	  token.special_flags2 = SF2_ENCHANTED_EVENING;
	  token.pow = 3;
	  token.tou = 1;
	  token.s_key_plus = SP_KEYWORD_HASTE;
	  generate_token(&token);
	}

  return 0;
}

int card_scouring_sands(int player, int card, event_t event)
{
  /* Scouring Sands |1|R
   * Sorcery
   * ~ deals 1 damage to each creature your opponents control. Scry 1. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  new_damage_all(player, card, 1-player, 1, NDA_ALL_CREATURES, NULL);
	  scry(player, 1);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

static int effect_searing_blood(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  if (graveyard_from_play(instance->damage_target_player, instance->damage_target_card, event))
	{
	  damage_player(instance->damage_target_player, 3, instance->damage_source_player, instance->damage_source_card);
	  kill_card(player, card, KILL_REMOVE);
	}

  if ((event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card))
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_searing_blood(int player, int card, event_t event)
{
  /* Searing Blood |R|R
   * Instant
   * ~ deals 2 damage to target creature. When that creature dies this turn, ~ deals 3 damage to that creature's controller. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);

		  damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		  create_targetted_legacy_effect(player, card, &effect_searing_blood, instance->targets[0].player, instance->targets[0].card);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_stormcaller_of_keranos(int player, int card, event_t event)
{
  /* Stormcaller of Keranos |2|R
   * Creature - Human Shaman (2/2)
   * Haste
   * |1|U: Scry 1. */

  if (event == EVENT_ABILITIES && affect_me(player, card))
	haste(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	scry(player, 1);

  return generic_activated_ability(player, card, event, 0, MANACOST_XU(1,1), 0, NULL, NULL);
}

int card_thunder_brute(int player, int card, event_t event)
{
  /* Thunder Brute |4|R|R
   * Creature - Cyclops (5/5)
   * Trample
   * Tribute 3
   * When ~ enters the battlefield, if tribute wasn't paid, it gains haste until end of turn. */

  if (tribute_wasnt_paid(player, card, event, 3))
	pump_ability_until_eot(player, card, player, card, 0,0, 0, SP_KEYWORD_HASTE);

  return 0;
}

int card_thunderous_might(int player, int card, event_t event)
{
  /* Thunderous Might |1|R
   * Enchantment - Aura
   * Enchant creature
   * Whenever enchanted creature attacks, it gets +X/+0 until end of turn, where X is your devotion to red. */

  card_instance_t* instance = in_play(player, card);
  if (instance && declare_attackers_trigger(player, card, event, 0, instance->damage_target_player, instance->damage_target_card))
	pump_until_eot(player, card, instance->damage_target_player, instance->damage_target_card, devotion(player, card, COLOR_RED, 0), 0);

  return vanilla_aura(player, card, event, player);
}

int card_whims_of_the_fates(int player, int card, event_t event)
{
  /* Whims of the Fates |5|R
   * Sorcery
   * Starting with you, each player separates all permanents he or she controls into three piles. Then each player chooses one of his or her piles at random and
   * sacrifices those permanents. (Piles can be empty.) */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  char marked[2][151] = {{0}};
	  int pl, c;
	  for (pl = 0; pl <= 1; ++pl)
		{
		  int p = pl ? 1-player: player;
		  if (p == AI || ai_is_speculating == 1)
			{
			  card_instance_t* inst;
			  // AI: randomly assign
			  for (c = 0; c < active_cards_count[p]; ++c)
				if ((inst = in_play(p, c)) && is_what(p, c, TYPE_PERMANENT))
				  {
					// Auras enchanting own permanents: put them in the same pile as their permanent in the second pass
					if (inst->damage_target_player == p && inst->damage_target_card >= 0
						&& has_subtype(p, c, SUBTYPE_AURA)
						&& in_play(inst->damage_target_player, inst->damage_target_card)
						&& is_what(inst->damage_target_player, inst->damage_target_card, TYPE_PERMANENT))
					  marked[p][c] = 42;
					else
					  marked[p][c] = internal_rand(3);
				  }
				else
				  marked[p][c] = 17;

			  for (c = 0; c < active_cards_count[p]; ++c)
				{
				  if (marked[p][c] == 42)
					{
					  int tgt = c;
					  while (marked[p][tgt] == 42)
						tgt = get_card_instance(p, tgt)->damage_target_card;

					  marked[p][c] = marked[p][tgt];
					}

				  inst = get_card_instance(p, c);
				  switch (marked[p][c])
					{
					  case 0:
						inst->state &= ~(STATE_CANNOT_TARGET|STATE_TARGETTED);
						break;

					  case 1:
						inst->state |= STATE_CANNOT_TARGET;
						inst->state &= ~STATE_TARGETTED;
						break;

					  case 2:
						inst->state &= ~STATE_CANNOT_TARGET;
						inst->state |= STATE_TARGETTED;
						break;
					}
				}
			}
		  else
			{
			  target_definition_t td;
			  base_target_definition(player, card, &td, TYPE_PERMANENT);
			  td.who_chooses = p;
			  td.allowed_controller = p;
			  td.allow_cancel = 3;

			  card_instance_t* inst;

			restart:
			  for (c = 0; c < active_cards_count[p]; ++c)
				if ((inst = in_play(p, c)) && is_what(p, c, TYPE_PERMANENT))
				  {
					inst->state &= ~(STATE_CANNOT_TARGET|STATE_TARGETTED);
					marked[p][c] = 0;
				  }
				else
				  marked[p][c] = 17;

			  int pile;
			  for (pile = 1; pile <= 2; ++pile)
				{
				  target_t tgt;
				  char prompt[200];
				  sprintf(prompt, "Select permanents for pile #%d.", pile);
				  while (select_target(player, card-1000, &td, prompt, &tgt))
					{
					  /* We know that it was either pile or 0, since otherwise it would be untargettable (whether from not being an in-play permanent or having
					   * been marked STATE_CANT_TARGET) */
					  ASSERT(marked[tgt.player][tgt.card] == 0 || marked[tgt.player][tgt.card] == pile);

					  marked[tgt.player][tgt.card] ^= pile;
					  get_card_instance(tgt.player, tgt.card)->state ^= STATE_TARGETTED;
					}

				  if (tgt.card == -1)	// pushed cancel, not done
					goto restart;

				  if (pile == 1)
					for (c = 0; c < active_cards_count[p]; ++c)
					  if (marked[p][c] == pile)
						{
						  inst = get_card_instance(p, c);
						  inst->state &= ~STATE_TARGETTED;
						  inst->state |= STATE_CANNOT_TARGET;
						}
				}
			}
		}

	  for (pl = 0; pl <= 1; ++pl)
		for (c = 0; c < active_cards_count[pl]; ++c)
		  if (marked[pl][c] == 1 || marked[pl][c] == 2)
			get_card_instance(pl, c)->state &= ~(STATE_CANNOT_TARGET|STATE_TARGETTED);

	  APNAP(p,
			{
			  int sac = internal_rand(3);
			  if (new_can_sacrifice(player, card, p, NULL))
				for (c = 0; c < active_cards_count[p]; ++c)
				  if (marked[p][c] == sac)
					kill_card(p, c, KILL_SACRIFICE);
			});

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/********
* Green *
********/

int card_archetype_of_endurance(int player, int card, event_t event)
{
  /* Archetype of Endurance |6|G|G
   * Enchantment Creature - Boar (6/5)
   * Creatures you control have hexproof.
   * Creatures your opponents control lose hexproof and can't have or gain hexproof. */

  if (event == EVENT_CAN_CAST)
	return 1;

  return archetype(player, event, 0, SP_KEYWORD_HEXPROOF, PB_CANT_HAVE_OR_GAIN_HEXPROOF);
}

int card_aspect_of_hydra(int player, int card, event_t event)
{
  /* Aspect of Hydra |G
   * Instant
   * Target creature gets +X/+X until end of turn, where X is your devotion to green. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  int dev = devotion(player, card, COLOR_GREEN, 0);
		  pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, dev, dev);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/* Charging Badger |G => vanilla
 * Creature - Badger (1/1)
 * Trample */

int card_courser_of_kruphix(int player, int card, event_t event)
{
  /* Courser of Kruphix |1|G|G
   * Enchantment Creature - Centaur (2/4)
   * Play with the top card of your library revealed.
   * You may play the top card of your library if it's a land card.
   * Whenever a land enters the battlefield under your control, you gain 1 life. */

  if (event == EVENT_CAN_CAST)
	return 1;

  reveal_top_card(player, card, event);

  if (event == EVENT_CAN_ACTIVATE)
	return (deck_ptr[player][0] != -1
			&& !(land_can_be_played & LCBP_LAND_HAS_BEEN_PLAYED)
			&& can_sorcery_be_played(player, event)
			&& is_what(-1, deck_ptr[player][0], TYPE_LAND)
			&& !is_humiliated(player, card));

  if (event == EVENT_ACTIVATE)
	{
	  play_card_in_deck_for_free(player, player, 0);
	  cant_be_responded_to = 1;
	}

  if (!is_humiliated(player, card) && landfall(player, card, event))
	gain_life(player, 1);

  return 0;
}

int card_culling_mark(int player, int card, event_t event)
{
  /* Culling Mark |2|G
   * Sorcery
   * Target creature blocks this turn if able. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0,0, 0, SP_KEYWORD_MUST_BLOCK);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && (current_turn == player || current_phase >= EVENT_DECLARE_BLOCKERS))
	ai_modifier -= 96;

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_fated_intervention(int player, int card, event_t event)
{
  /* Fated Intervention |2|G|G|G
   * Instant
   * Put two 3/3 green Centaur enchantment creature tokens onto the battlefield. If it's your turn, scry 2. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_CENTAUR, &token);
	  token.special_flags2 = SF2_ENCHANTED_EVENING;
	  token.qty = 2;
	  generate_token(&token);

	  if (player == current_turn)
		scry(player, 2);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_graverobber_spider(int player, int card, event_t event)
{
  /* Graverobber Spider |3|G
   * Creature - Spider (2/4)
   * Reach
   * |3|B: ~ gets +X/+X until end of turn, where X is the number of creature cards in your graveyard. Activate this ability only once each turn. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (in_play(instance->parent_controller, instance->parent_card))
		{
		  int amt = count_graveyard_by_type(player, TYPE_CREATURE);
		  if (amt > 0)
			pump_until_eot(player, card, instance->parent_controller, instance->parent_card, amt, amt);
		}
	}

  return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST_XB(3,1), 0, NULL, NULL);
}

int card_hero_of_leina_tower(int player, int card, event_t event)
{
  /* Hero of Leina Tower |G
   * Creature - Human Warrior (1/1)
   * Heroic - Whenever you cast a spell that targets ~, you may pay X. If you do, put X +1/+1 counters on ~. */

  if (heroic_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  int old_x_value = x_value;	// just in case
	  charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, COLOR_COLORLESS, -1);
	  int counters = x_value;
	  x_value = old_x_value;

	  if (cancel != 1 && counters > 0)
		add_1_1_counters(player, card, counters);
	}

  return 0;
}

static int effect_hunters_prowess(int player, int card, event_t event)
{
  // Not quite the same as similar effects, since this grants an ability to the creature which should be removed by humiliation.

  card_instance_t* instance = get_card_instance(player, card);

	if ( !is_humiliated(instance->targets[0].player, instance->targets[0].card) &&
		 damage_dealt_by_me_arbitrary(instance->targets[0].player, instance->targets[0].card, event,
									DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_REPORT_DAMAGE_DEALT, player, card)

	){
	  draw_cards(player, instance->targets[16].player);
	  instance->targets[16].player = 0;
	}

	if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_hunters_prowess(int player, int card, event_t event)
{
  /* Hunter's Prowess |4|G
   * Sorcery
   * Until end of turn, target creature gets +3/+3 and gains trample and "Whenever this creature deals combat damage to a player, draw that many cards." */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 3, KEYWORD_TRAMPLE, 0);
			int legacy = create_legacy_effect(player, card, &effect_hunters_prowess);
			get_card_instance(player, legacy)->targets[0] = instance->targets[0];
			get_card_instance(player, legacy)->number_of_targets = 1;
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_karametras_favor(int player, int card, event_t event)
{
  /* Karametra's Favor |1|G
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield, draw a card.
   * Enchanted creature has "|T: Add one mana of any color to your mana pool." */

  if (comes_into_play(player, card, event))
	draw_a_card(player);

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || (event == EVENT_COUNT_MANA && affect_me(player, card)))
	{
	  card_instance_t* instance = in_play(player, card);
	  if (!instance || instance->damage_target_player < 0 || instance->damage_target_card < 0)
		return 0;

	  if (event == EVENT_ACTIVATE)
		{
		  if (produce_mana_tapped_all_one_color(instance->damage_target_player, instance->damage_target_card, COLOR_TEST_ANY_COLORED, 1))
			// tap_card() would do this, but clear tapped_for_mana_color first.
			dispatch_event(instance->damage_target_player, instance->damage_target_card, EVENT_TAP_CARD);
		}
	  else if (!is_tapped(instance->damage_target_player, instance->damage_target_card)
			   && !is_sick(instance->damage_target_player, instance->damage_target_card)
			   && can_produce_mana(instance->damage_target_player, instance->damage_target_card))
		{
		  if (event == EVENT_CAN_ACTIVATE)
			return 1;

		  if (event == EVENT_COUNT_MANA)
			declare_mana_available_hex(instance->damage_target_player, COLOR_TEST_ANY_COLORED, 1);
		}

	  return 0;
	}

  return vanilla_aura(player, card, event, player);
}

int card_mischief_and_mayhem(int player, int card, event_t event)
{
  /* Mischief and Mayhem |4|G
   * Sorcery
   * Up to two target creatures each get +4/+4 until end of turn. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int i;
	  for (i = 0; i < instance->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  pump_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 4, 4);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_mortals_resolve(int player, int card, event_t event)
{
  /* Mortal's Resolve |1|G
   * Instant
   * Target creature gets +1/+1 and gains indestructible until end of turn. */

  return vanilla_instant_pump(player, card, event, ANYBODY, player, 1,1, 0,SP_KEYWORD_INDESTRUCTIBLE);
}

int card_nessian_demolok(int player, int card, event_t event)
{
  /* Nessian Demlock |3|G|G
   * Creature - Beast (3/3)
   * Tribute 3
   * When ~ enters the battlefield, if tribute wasn't paid, destroy target noncreature permanent. */

  if (tribute_wasnt_paid(player, card, event, 3))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.illegal_type = TYPE_CREATURE;

	  card_instance_t* instance = get_card_instance(player, card);

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_NONCREATURE_PERMANENT"))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

  return 0;
}

int card_nessian_wilds_ravager(int player, int card, event_t event)
{
  /* Nessian Wilds Ravager |4|G|G
   * Creature - Hydra (6/6)
   * Tribute 6
   * When ~ enters the battlefield, if tribute wasn't paid, you may have ~ fight another target creature. */

  if (tribute_wasnt_paid(player, card, event, 6))
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.special = TARGET_SPECIAL_NOT_ME;

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_ANOTHER_CREATURE"))
		fight(player, card, instance->targets[0].player, instance->targets[0].card);
	}

  return 0;
}

int card_noble_quarry(int player, int card, event_t event)
{
  /* Noble Quarry |2|G
   * Enchantment Creature - Unicorn (1/1)
   * Bestow |5|G
   * All creatures able to block ~ or enchanted creature do so.
   * Enchanted creature gets +1/+1. */

  everybody_must_block_me(player, card, event);

  return generic_creature_with_bestow(player, card, event, MANACOST_XG(5,1), 1,1, 0,SP_KEYWORD_LURE);
}

int card_nyxborn_wolf(int player, int card, event_t event)
{
  /* Nyxborn Wolf |2|G
   * Enchantment Creature - Wolf (3/1)
   * Bestow |4|G
   * Enchanted creature gets +3/+1. */

  return generic_creature_with_bestow(player, card, event, MANACOST_XG(4,1), 3,1, 0, 0);
}

int card_peregrination(int player, int card, event_t event)
{
  /* Peregrination |3|G
   * Sorcery
   * Search your library for up to two basic land cards, reveal those cards, and put one onto the battlefield tapped and the other into your hand. Shuffle your
   * library, then scry 1. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  cultivate(player);
	  scry(player, 1);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_pheres_band_raiders(int player, int card, event_t event)
{
  /* Pheres-Band Raiders |5|G
   * Creature - Centaur Warrior (5/5)
   * Inspired - Whenever ~ becomes untapped, you may pay |2|G. If you do, put a 3/3 green Centaur enchantment creature token onto the battlefield. */

  if (inspired(player, card, event) && has_mana_multi(player, MANACOST_XG(2,1))
	  && DIALOG(player, card, EVENT_ACTIVATE, DLG_NO_STORAGE, "Generate a Centaur", 1, 1)
	  && charge_mana_multi_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, player, MANACOST_XG(2,1)))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_CENTAUR, &token);
	  token.special_flags2 = SF2_ENCHANTED_EVENING;
	  generate_token(&token);
	}

  return 0;
}

int card_pheres_band_tromper(int player, int card, event_t event)
{
  /* Pheres-Band Tromper |3|G
   * Creature - Centaur Warrior (3/3)
   * Inspired - Whenever ~ becomes untapped, put a +1/+1 counter on it. */

  if (inspired(player, card, event))
	add_1_1_counter(player, card);

  return 0;
}

int card_raised_by_wolves(int player, int card, event_t event)
{
  /* Raised by Wolves |3|G|G
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield, put two 2/2 green Wolf creature tokens onto the battlefield.
   * Enchanted creature gets +1/+1 for each Wolf you control. */

  if (comes_into_play(player, card, event))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_WOLF, &token);
	  token.qty = 2;
	  generate_token(&token);
	}

  if (event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card) && in_play(player, card))
		event_result += count_subtype(player, TYPE_PERMANENT, SUBTYPE_WOLF);
	}

  return vanilla_aura(player, card, event, player);
}

int card_satyr_wayfinder(int player, int card, event_t event)
{
  /* Satyr Wayfinder |1|G
   * Creature - Satyr (1/1)
   * When ~ enters the battlefield, reveal the top four cards of your library. You may put a land card from among them into your hand. Put the rest into your
   * graveyard. */

  if (comes_into_play(player, card, event))
	reveal_top_cards_of_library_and_choose_type(player, card, player, 4, 0, TUTOR_HAND, 1, TUTOR_GRAVE, 1, TYPE_LAND);

  return 0;
}

int card_scourge_of_skola_vale(int player, int card, event_t event)
{
  /* Scourge of Skola Vale |2|G
   * Creature - Hydra (0/0)
   * Trample
   * ~ enters the battlefield with two +1/+1 counters on it.
   * |T, Sacrifice another creature: Put a number of +1/+1 counters on ~ equal to the sacrificed creature's toughness. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (in_play(instance->parent_controller, instance->parent_card))
		add_1_1_counters(instance->parent_controller, instance->parent_card, instance->targets[2].card);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_CREATURE|GAA_NOT_ME_AS_TARGET, MANACOST_X(0), 0, NULL, NULL);
}

/* Setessan Oathsworn |1|G|G => theros.c:card_staunch_hearted_warrior
 * Creature - Satyr Warrior (1/1)
 * Heroic - Whenever you cast a spell that targets ~, put two +1/+1 counters on ~. */

int card_setessan_starbreaker(int player, int card, event_t event)
{
  /* Setessan Starbreaker |3|G
   * Creature - Human Warrior (2/1)
   * When ~ enters the battlefield, you may destroy target Aura. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
	  td.required_subtype = SUBTYPE_AURA;

	  card_instance_t* instance = get_card_instance(player, card);

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_next_target_noload(&td, "Select target Aura."))
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

  return 0;
}

int card_skyreaping(int player, int card, event_t event)
{
  /* Skyreaping |1|G
   * Sorcery
   * ~ deals damage to each creature with flying equal to your devotion to green. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.keyword = KEYWORD_FLYING;

	  int amount = devotion(player, card, COLOR_GREEN, 0);
	  new_damage_all(player, card, ANYBODY, amount, 0, &test);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_snake_of_the_golden_grove(int player, int card, event_t event)
{
  /* Snake of the Golden Grove |4|G
   * Creature - Snake (4/4)
   * Tribute 3
   * When ~ enters the battlefield, if tribute wasn't paid, you gain 4 life. */

  if (tribute_wasnt_paid(player, card, event, 3))
	gain_life(player, 4);

  return 0;
}

/* Swordwise Centaur |G|G => vanilla
 * Creature - Centaur Warrior (3/2) */

/* Unravel the AEther |1|G => morningtide.c:card_deglamer
 * Instant
 * Choose target artifact or enchantment. Its owner shuffles it into his or her library. */

/*************
* Multicolor *
*************/

int card_chromanticore(int player, int card, event_t event)
{
  /* Chromanticore |W|U|B|R|G
   * Enchantment Creature - Manticore (4/4)
   * Bestow |2|W|U|B|R|G
   * Flying, first strike, vigilance, trample, lifelink
   * Enchanted creature gets +4/+4 and has flying, first strike, vigilance, trample and lifelink. */

  vigilance(player, card, event);
  lifelink(player, card, event);

  return generic_creature_with_bestow(player, card, event,
									  2,1,1,1,1,1,	// mana cost
									  4, 4,	// p/t
									  KEYWORD_FLYING | KEYWORD_FIRST_STRIKE | KEYWORD_TRAMPLE,
									  SP_KEYWORD_VIGILANCE | SP_KEYWORD_LIFELINK);
}

int card_ephara_god_of_the_polis(int player, int card, event_t event)
{
  /* Ephara, God of the Polis |2|W|U
   * Legendary Enchantment Creature - God (6/5)
   * Indestructible
   * As long as your devotion to white and blue is less than seven, ~ isn't a creature.
   * At the beginning of each upkeep, if you had another creature enter the battlefield under your control last turn, draw a card. */

  /* Approximation: Except for creatures that enter the battlefield before ~ on the same turn it's played, only counts creatures coming into play while ~ is in
   * play (or on the stack) and not copying something else. */

  /* Targets:
   * [1].player: Have any creatures other than this entered the battlefield this turn.
   * [1].card: Have any creatures other than this entered the battlefield last turn. */

  card_instance_t* instance = get_card_instance(player, card);

  check_legend_rule(player, card, event);

  indestructible(player, card, event);

  generic_creature_with_devotion(player, card, event, COLOR_WHITE, COLOR_BLUE, 7);

  if (event == EVENT_CAN_CAST)
	return 1;

  // Any other creatures already played this turn?
  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	instance->targets[1].player = get_trap_condition(player, TRAP_CREATURES_PLAYED) > 0 ? 1 : 0;

  // Any creatures entering the battlefield while this is in play or on the stack?
  // Inlined here rather than specific_cip(), since we have to keep track even if this is currently humiliated or there's a Torpor Orb in play.
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player
	  && event == EVENT_END_TRIGGER	// Trigger doesn't need to be visible or ordered
	  && trigger_cause_controller == player && trigger_cause != card
	  && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE))
	instance->targets[1].player = 1;

  if (event == EVENT_BEGIN_TURN)
	{
	  instance->targets[1].card = instance->targets[1].player;	// creatures last turn
	  instance->targets[1].player = 0;	// none so far this turn
	}

  if (instance->targets[1].card > 0)
	upkeep_trigger_ability(player, card, event, ANYBODY);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	draw_cards(player, 1);

  return 0;
}

int card_epharas_enlightenment(int player, int card, event_t event)
{
  /* Ephara's Enlightenment |1|W|U
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield, put a +1/+1 counter on enchanted creature.
   * Enchanted creature has flying.
   * Whenever a creature enters the battlefield under your control, you may return ~ to its owner's hand. */

  card_instance_t* instance = get_card_instance(player, card);

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  if (comes_into_play(player, card, event)
		  && instance->damage_target_player >= 0 && instance->damage_target_card >= 0
		  && in_play(instance->damage_target_player, instance->damage_target_card))
		add_1_1_counter(instance->damage_target_player, instance->damage_target_card);

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");

	  int trigger_mode = is_stolen(player, card) ? RESOLVE_TRIGGER_OPTIONAL : RESOLVE_TRIGGER_AI(player);

	  if (new_specific_cip(player, card, event, player, trigger_mode, &test))
		bounce_permanent(player, card);
	}

  if (event == EVENT_ABILITIES && affect_me(instance->damage_target_player, instance->damage_target_card) && in_play(player, card))
	event_result |= KEYWORD_FLYING;

  return vanilla_aura(player, card, event, player);
}

int card_fanatic_of_xenagos(int player, int card, event_t event)
{
  /* Fanatic of Xenagos |1|R|G
   * Creature - Centaur Warrior (3/3)
   * Trample
   * Tribute 1
   * When ~ enters the battlefield, if tribute wasn't paid, it gets +1/+1 and gains haste until end of turn. */

  if (tribute_wasnt_paid(player, card, event, 1))
	pump_ability_until_eot(player, card, player, card, 1,1, 0, SP_KEYWORD_HASTE);

  return 0;
}

int card_karametra_god_of_harvests(int player, int card, event_t event)
{
  /* Karametra, God of Harvests |3|G|W
   * Legendary Enchantment Creature - God (6/7)
   * Indestructible
   * As long as your devotion to green and white is less than seven, ~ isn't a creature.
   * Whenever you cast a creature spell, you may search your library for a Forest or Plains card, put it onto the battlefield tapped, then shuffle your
   * library. */

  /* Approximation: Probably doesn't work with this ruling: If you cast a creature card with bestow for its bestow cost, it becomes an Aura spell and not a
   * creature spell. Karametra's last ability won't trigger." */

  check_legend_rule(player, card, event);

  indestructible(player, card, event);

  generic_creature_with_devotion(player, card, event, COLOR_GREEN, COLOR_WHITE, 7);

  if (event == EVENT_CAN_CAST)
	return 1;

  if (trigger_condition == TRIGGER_SPELL_CAST)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");

	  if (new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), &test))
		{
		  new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select %a or %s card.", SUBTYPE_FOREST, SUBTYPE_PLAINS));
		  test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		  test.sub2 = get_hacked_subtype(player, card, SUBTYPE_PLAINS);
		  test.subtype_flag = F2_MULTISUBTYPE;

		  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &test);
		}
	}

  return 0;
}

static int effect_prevent_damage_to_or_from_until_my_next_turn(int player, int card, event_t event)
{
  card_instance_t* damage = damage_being_prevented(event), *instance;
  if (damage
	  && (instance = get_card_instance(player, card))
	  && ((damage->damage_source_card == instance->damage_target_card && damage->damage_source_player == instance->damage_target_player)
		  || (damage->damage_target_card == instance->damage_target_card && damage->damage_target_player == instance->damage_target_player)))
	damage->info_slot = 0;

  if (event == EVENT_BEGIN_TURN && current_turn == player)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_kiora_the_crashing_wave(int player, int card, event_t event)
{
  /* Kiora, the Crashing Wave |2|G|U
   * Planeswalker - Kiora (2)
   * +1: Until your next turn, prevent all damage that would be dealt to and dealt by target permanent an opponent controls.
   * -1: Draw a card. You may play an additional land this turn.
   * -5: You get an emblem with "At the beginning of your end step, put a 9/9 blue Kraken creature token onto the battlefield." */

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int loyalty = count_counters(player, card, COUNTER_LOYALTY);

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.allowed_controller = 1-player;

	  enum
	  {
		CHOICE_PREVENT_DAMAGE = 1,
		CHOICE_DRAW_AND_LAND = 2,
		CHOICE_KRAKEN_EMBLEM = 3
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Prevent damage", can_target(&td),	MAX(1, 10 - loyalty),	1,
						"Draw and play land", 1,			loyalty,				-1,
						"Kraken emblem", 1,					(loyalty - 4) * 5,		-5);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  if (choice == CHOICE_PREVENT_DAMAGE)
			pick_target(&td, "TARGET_PERMANENT");
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		{
		  switch (choice)
			{
			  case CHOICE_PREVENT_DAMAGE:
				if (valid_target(&td))
				  {
					alternate_legacy_text(1, instance->parent_controller,
										  create_targetted_legacy_effect(player, card, &effect_prevent_damage_to_or_from_until_my_next_turn,
																		 instance->targets[0].player, instance->targets[0].card));
					if (IS_AI(player) && is_what(instance->targets[0].player, instance->targets[0].card, TYPE_CREATURE))
					  ai_modifier += get_power(instance->targets[0].player, instance->targets[0].card) * 12;
				  }
				break;

			  case CHOICE_DRAW_AND_LAND:
				draw_cards(player, 1);
				alternate_legacy_text(2, instance->parent_controller, create_legacy_effect(player, card, &check_playable_lands_legacy));
				break;

			  case CHOICE_KRAKEN_EMBLEM:
				{
				  int i, tok = generate_reserved_token_by_id(player, CARD_ID_KIORAS_EMBLEM);
				  if (tok != -1)
					{
					  /* Copy sleight flags for eventual use modifying the emblem's generated krakens' color, though I suspect generate_token_by_id() should be
					   * made to do it itself */
					  card_instance_t* emblem = get_card_instance(player, tok);
					  emblem->token_status |= instance->token_status & (STATUS_SLEIGHTED | STATUS_HACKED);
					  for (i = 0; i < 6; ++i)
						{
						  emblem->color_id[i] = instance->color_id[i];
						  emblem->hack_mode[i] = instance->hack_mode[i];
						}
					}
				  break;
				}
			}
		}
	}

  return planeswalker(player, card, event, 2);
}

int card_kioras_emblem(int player, int card, event_t event)
{
	// At the beginning of your end step, put a 9/9 blue Kraken creature token onto the battlefield.
	if (current_turn == player && eot_trigger(player, card, event)){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_KRAKEN, &token);
		token.pow = 9;
		token.tou = 9;
		token.color_forced = COLOR_TEST_BLUE;
		generate_token(&token);
	}

	return 0;
}

int card_kioras_follower(int player, int card, event_t event)
{
  /* Kiora's Follower |G|U
   * Creature - Merfolk (2/2)
   * |T: Untap another target permanent. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.special = TARGET_SPECIAL_NOT_ME;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  untap_card(instance->targets[0].player, instance->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), 0, &td, "TARGET_ANOTHER_PERMANENT");
}

int card_mogis_god_of_slaughter(int player, int card, event_t event)
{
  /* Mogis, God of Slaughter |2|B|R
   * Legendary Enchantment Creature - God
   * Indestructible
   * As long as your devotion to black and red is less than seven, ~ isn't a creature.
   * At the beginning of each opponent's upkeep, ~ deals 2 damage to that player unless he or she sacrifices a creature. */

  check_legend_rule(player, card, event);

  indestructible(player, card, event);

  generic_creature_with_devotion(player, card, event, COLOR_BLACK, COLOR_RED, 7);

  if (event == EVENT_CAN_CAST)
	return 1;

  upkeep_trigger_ability(player, card, event, 1-player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  if (DIALOG(player, card, EVENT_ACTIVATE,
				 DLG_WHO_CHOOSES(1-player), DLG_RANDOM, DLG_NO_CANCEL, DLG_NO_STORAGE,
				 "Sacrifice a creature", can_sacrifice(player, 1-player, 1, TYPE_CREATURE, 0), 3,
				 "Take 2 damage", 1, 5) == 1)
		impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0,0,0,0,0,0,0, -1, 0);
	  else
		damage_player(1-player, 2, player, card);
	}

  return 0;
}

int card_phenax_god_of_deception(int player, int card, event_t event)
{
  /* Phenax, God of Deception |3|U|B
   * Legendary Enchantment Creature - God (4/7)
   * Indestructible
   * As long as your devotion to blue and black is less than seven, ~ isn't a creature.
   * Creatures you control have "|T: Target player puts the top X cards of his or her library into his or her graveyard, where X is this creature's
   * toughness." */

  check_legend_rule(player, card, event);

  indestructible(player, card, event);

  generic_creature_with_devotion(player, card, event, COLOR_BLUE, COLOR_BLACK, 7);

  if (event == EVENT_CAN_CAST)
	return 1;

#define DECL_TGT_CREATURE(td_creature)												\
  target_definition_t td_creature;													\
  default_target_definition(player, card, &td_creature, TYPE_CREATURE);				\
  td_creature.allowed_controller = player;											\
  td_creature.preferred_controller = player;										\
  td_creature.illegal_state = TARGET_STATE_TAPPED | TARGET_STATE_SUMMONING_SICK;	\
  td_creature.illegal_abilities = 0

#define DECL_TGT_PLAYER(player, card, td_player)			\
  target_definition_t td_player;							\
  default_target_definition(player, card, &td_player, 0);	\
  td_player.zone = TARGET_ZONE_PLAYERS

  if (event == EVENT_CAN_ACTIVATE && !is_humiliated(player, card))
	{
	  DECL_TGT_PLAYER(player, card, td_player);

	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && is_what(player, c, TYPE_CREATURE) && !is_tapped(player, c) && !is_sick(player, c) && can_use_activated_abilities(player, c))
		  {
			td_player.card = c;
			td_player.illegal_abilities = get_protections_from(player, c);

			if (can_target(&td_player))
			  return 1;
		  }
	}

  if (event == EVENT_ACTIVATE)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  DECL_TGT_CREATURE(td_creature);

	  instance->number_of_targets = 0;
	  if (pick_next_target_noload(&td_creature, "Select a creature you control."))
		{
		  int p = instance->targets[0].player;
		  int c = instance->targets[0].card;
		  instance->number_of_targets = 0;

		  instance->targets[1] = instance->targets[0];	// struct copy

		  DECL_TGT_PLAYER(p, c, td_player);
		  if (can_use_activated_abilities(p, c) && can_target(&td_player) && charge_mana_for_activated_ability(p, c, MANACOST_X(0)))
			{
			  load_text(0, "TARGET_PLAYER");
			  if (select_target(p, c, &td_player, text_lines[0], &instance->targets[0]))
				{
				  tap_card(p, c);
				  return 0;
				}
			}
		}

	  spell_fizzled = 1;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  DECL_TGT_PLAYER(instance->targets[1].player, instance->targets[1].card, td_player);

	  if (validate_arbitrary_target(&td_player, instance->targets[0].player, instance->targets[0].card))
		mill(instance->targets[0].player, get_toughness(instance->targets[1].player, instance->targets[1].card));

	  get_card_instance(instance->parent_controller, instance->parent_card)->number_of_targets = 0;
	}

  return 0;
#undef DECL_TGT_CREATURE
#undef DECL_TGT_PLAYER
}

int card_ragemonger(int player, int card, event_t event)
{
  /* Ragemonger |1|B|R
   * Creature - Minotaur Shaman (2/3)
   * Minotaur spells you cast cost |B|R less to cast. This effect reduces only the amount of colored mana you pay. */

  if (event == EVENT_MODIFY_COST_GLOBAL
	  && affected_card_controller == player
	  && has_subtype(affected_card_controller, affected_card, SUBTYPE_MINOTAUR))
	{
	  --COST_RED;
	  --COST_BLACK;
	}

  return 1;
}

int card_reap_what_is_sown(int player, int card, event_t event)
{
  /* Reap What Is Sown |1|G|W
   * Instant
   * Put a +1/+1 counter on each of up to three target creatures. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int i;
	  for (i = 0; i < instance->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  add_1_1_counter(instance->targets[i].player, instance->targets[i].card);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 3, NULL);
}

int card_siren_of_the_silent_song(int player, int card, event_t event)
{
  /* Siren of the Silent Song |1|U|B
   * Creature - Zombie Siren (2/1)
   * Flying
   * Inspired - Whenever ~ becomes untapped, each opponent discards a card and puts the top card of his or her library into his or her graveyard. */

  if (inspired(player, card, event))
	{
	  discard(1-player, 0, player);
	  mill(1-player, 1);
	}

  return 0;
}

int card_xenagos_god_of_revels(int player, int card, event_t event)
{
  /* Xenagos, God of Revels |3|R|G
   * Legendary Enchantment Creature - God (6/5)
   * Indestructible
   * As long as your devotion to red and green is less than seven, ~ isn't a creature.
   * At the beginning of combat on your turn, another target creature you control gains haste and gets +X/+X until end of turn, where X is that creature's
   * power. */

  check_legend_rule(player, card, event);

  indestructible(player, card, event);

  generic_creature_with_devotion(player, card, event, COLOR_RED, COLOR_GREEN, 7);

	if (event == EVENT_CAN_CAST){
		return 1;
	}

	if (beginning_of_combat(player, card, event, player, -1) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = td.preferred_controller = player;
		td.special = TARGET_SPECIAL_NOT_ME;

		card_instance_t *instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if (can_target(&td) && pick_target(&td, "TARGET_ANOTHER_CREATURE_CONTROL")){
			int pow = get_power(instance->targets[0].player, instance->targets[0].card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, pow,pow, 0, SP_KEYWORD_HASTE);
		}
	}

	return 0;
}

/***********
* Artifact *
***********/

int card_astral_cornucopia(int player, int card, event_t event)
{
  /* Astral Cornucopia |X|X|X
   * Artifact
   * ~ enters the battlefield with X charge counters on it.
   * |T: Choose a color. Add one mana of that color to your mana pool for each charge counter on ~. */

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && !played_for_free(player, card) && !is_token(player, card))
	{
	  if (player == AI && !has_mana(player, COLOR_ARTIFACT, 3))
		ai_modifier -= 256;

	  int xxx = charge_mana_for_multi_x(player, COLOR_ARTIFACT, 3) / 3;
	  ai_modifier += 24 * xxx;

	  get_card_instance(player, card)->info_slot = xxx;
	}

  enters_the_battlefield_with_counters(player, card, event, COUNTER_CHARGE, get_card_instance(player, card)->info_slot);

  return artifact_mana_all_one_color(player, card, event,
									 (event == EVENT_ACTIVATE || event == EVENT_COUNT_MANA) ? count_counters(player, card, COUNTER_CHARGE) : 0, 0);
}

int card_gorgons_head(int player, int card, event_t event)
{
  /* Gorgon's Head |1
   * Artifact - Equipment
   * Equipped creature has deathtouch.
   * Equip |2 */

  return vanilla_equipment(player, card, event, 2, 0,0, 0,SP_KEYWORD_DEATHTOUCH);
}

int card_heroes_podium(int player, int card, event_t event)
{
  /* Heroes' Podium |5
   * Legendary Artifact
   * Each legendary creature you control gets +1/+1 for each other legendary creature you control.
   * |X, |T: Look at the top X cards of your library. You may reveal a legendary creature card from among them and put it into your hand. Put the rest on the
   * bottom of your library in a random order. */

  check_legend_rule(player, card, event);

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)	// Probably redundant
	  && in_play(player, card) && !is_humiliated(player, card))
	{
	  // I'd have expected is_legendary() to take care of this.  Preferably with reasonable caching.
	  if (check_battlefield_for_id(ANYBODY, CARD_ID_LEYLINE_OF_SINGULARITY)
		  && (is_legendary(affected_card_controller, affected_card)	// all creatures that are naturally legendary
			  || !is_what(affected_card_controller, affected_card, TYPE_LAND)))	// and all nonland creatures
		{
		  int c, other_legends = 0;
		  for (c = 0; c < active_cards_count[player]; ++c)
			if (in_play(player, c)
				&& c != affected_card
				&& is_what(player, c, TYPE_CREATURE)
				&& (is_legendary(player, c)
					|| !is_what(player, c, TYPE_LAND)))
			  ++other_legends;

		  event_result += other_legends;
		}
	  else if (is_legendary(affected_card_controller, affected_card))
		event_result += count_subtype(player, TYPE_CREATURE, SUBTYPE_LEGEND) - 1;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select a legendary creature card.");
	  test.subtype = SUBTYPE_LEGEND;

	  int num_cards = get_card_instance(player, card)->info_slot;

	  reveal_top_cards_of_library_and_choose(player, card, player, num_cards, 0, TUTOR_HAND, 1, TUTOR_BOTTOM_OF_DECK_RANDOM, 0, &test);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(-1), 0, NULL, NULL);
}

int card_pillar_of_war(int player, int card, event_t event)
{
  /* Pillar of War |3
   * Artifact Creature - Golem (3/3)
   * Defender
   * As long as ~ is enchanted, it can attack as though it didn't have defender. */

  if (event == EVENT_ABILITIES && affect_me(player, card) && is_enchanted(player, card) && !is_humiliated(player, card))
	add_status(affected_card_controller, affected_card, STATUS_WALL_CAN_ATTACK);

  return 0;
}

int card_siren_song_lyre(int player, int card, event_t event)
{
  /* Siren Song Lyre |2
   * Artifact - Equipment
   * Equipped creature has "|2, |T: Tap target creature."
   * Equip |2 */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  int rval = equipment_granting_activated_ability(player, card, event, "Tap target creature", 2,
												  GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(2), 0, &td, "TARGET_CREATURE");

  if (rval && event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (validate_arbitrary_target(&td, instance->targets[0].player, instance->targets[0].card))
		tap_card(instance->targets[0].player, instance->targets[0].card);

	  return 0;
	}

  return rval;
}

/* Springleaf Drum |1 => lorwyn.c
 * Artifact
 * |T, Tap an untapped creature you control: Add one mana of any color to your mana pool. */

/*******
* Land *
*******/

/* Temple of Enlightenment => New Benalia
 * Land
 * ~ enters the battlefield tapped.
 * When ~ enters the battlefield, scry 1.
 * |T: Add |W or |U to your mana pool. */

/* Temple of Malice => New Benalia
 * Land
 * ~ enters the battlefield tapped.
 * When ~ enters the battlefield, scry 1.
 * |T: Add |B or |R to your mana pool. */

/* Temple of Plenty => New Benalia
 * Land
 * ~ enters the battlefield tapped.
 * When ~ enters the battlefield, scry 1.
 * |T: Add |G or |W to your mana pool. */

/**********************************
* Battle the Horde challenge deck *
**********************************/
/*
10 Minotaur Goreseeker
15 Minotaur Younghorn
4 Mogis's Chosen
10 Phoberos Reaver
4 Reckless Minotaur
2 Consuming Rage
2 Descend on the Prey
2 Intervention of Keranos
2 Touch of the Horned God
2 Unquenchable Fury
1 Altar of Mogis
1 Massacre Totem
2 Plundered Statue
2 Refreshing Elixir
1 Vitality Salve

The Horde has no life.
Instead, for each 1 damage one card from the Horde's deck is put into the graveyard.
You win when the Horde has no cards left in the deck and no more creatures on the battlefield.
*/

int card_battle_the_horde(int player, int card, event_t event){

	if( upkeep_trigger(player, card, event) ){
		int *deck = deck_ptr[player];
		int amount = 1 + count_subtype(player, TYPE_ARTIFACT, SUBTYPE_HORDE_ARTIFACT);
		while( amount ){
			if( deck[0] != -1 ){
				play_card_in_deck_for_free(player, player, 0);
			}
			else{
				if( ! count_subtype(player, TYPE_CREATURE, -1) ){
					lose_the_game(player);
				}
			}
			amount--;
		}
	}

	if (trigger_condition == TRIGGER_REPLACE_CARD_DRAW
		&& reason_for_trigger_controller == player
		&& affect_me(player, card)
		&& !suppress_draw
	   ){
		if (event == EVENT_TRIGGER){
			event_result = RESOLVE_TRIGGER_MANDATORY;
		}
		else if (event == EVENT_RESOLVE_TRIGGER){
				suppress_draw = 1;
		}
	}

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage = get_card_instance(affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && damage->damage_target_player == player &&
			damage->damage_target_card == -1
		  ){
			mill(player, damage->info_slot);
			damage->info_slot = 0;
		}
	}

	if( eot_trigger(player, card, event) ){
		int *deck = deck_ptr[player];
		if( deck[0] == -1 ){
			if( ! count_subtype(player, TYPE_CREATURE, -1) ){
				lose_the_game(player);
			}
		}
	}

	return vanguard_card(player, card, event, 0, 20, 12);
}

int card_vitality_salve(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( count_graveyard_by_type(1-player, TYPE_CREATURE) ){
		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");
		  new_global_tutor(1-player, 1-player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &test);
		}
	}
	return 0;
}

int card_refreshing_elixir(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		gain_life(1-player, 5);
	}
	return 0;
}

int card_plundered_statue(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		draw_cards(1-player, 1);
	}
	return 0;
}

int card_massacre_totem(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		mill(player, 7);
	}
	return 0;
}

int card_altar_of_mogis(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		impose_sacrifice(player, card, player, 2, TYPE_PERMANENT, 0, SUBTYPE_MINOTAUR, 0, 0, 0, 0, 0, -1, 0);
	}
	return 0;
}

int unquenchable_fury_legacy(int player, int card, event_t event){
	if( event == EVENT_DECLARE_BLOCKERS ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.subtype = SUBTYPE_MINOTAUR;
		test.state = STATE_ATTACKING;

		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_play(player, i) && new_make_test_in_play(player, i, -1, &test) ){
				minimum_blockers(player, i, event, 2);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_unquenchable_fury(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &unquenchable_fury_legacy);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int touch_of_the_horned_god_legacy(int player, int card, event_t event){
	card_instance_t *instance= get_card_instance(player, card);
	int amt;
	if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, player, -1))){
		unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		for (--amt; amt >= 0; --amt){
			if (in_play(current_turn, attackers[amt]) ){
				pump_ability_until_eot(player, card, current_turn, attackers[amt], 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_touch_of_the_horned_god(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &touch_of_the_horned_god_legacy);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int intervention_of_keranos_legacy(int player, int card, event_t event){

	if( beginning_of_combat(player, card, event, player, -1) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		new_damage_all(player, card, ANYBODY, 3, NDA_ALL_CREATURES, &test);
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_intervention_of_keranos(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &intervention_of_keranos_legacy);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int descend_of_the_prey_legacy(int player, int card, event_t event){
	card_instance_t *instance= get_card_instance(player, card);
	int amt;
	if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, player, -1))){
		unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		for (--amt; amt >= 0; --amt){
			if (in_play(current_turn, attackers[amt]) ){
				pump_ability_until_eot(player, card, current_turn, attackers[amt], 0, 0, KEYWORD_FIRST_STRIKE, SP_KEYWORD_MUST_BE_BLOCKED);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_descend_of_the_prey(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &descend_of_the_prey_legacy);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int consuming_rage_legacy(int player, int card, event_t event){
	card_instance_t *instance= get_card_instance(player, card);
	int amt;
	if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK, player, -1))){
		unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		for (--amt; amt >= 0; --amt){
			if (in_play(current_turn, attackers[amt]) ){
				pump_ability_t pump;
				default_pump_ability_definition(player, card, &pump, 2, 0, 0, 0);
				pump.paue_flags = PAUE_END_AT_EOT | PAUE_REMOVE_TARGET_AT_EOT;
				pump.eot_removal_method = KILL_DESTROY;
				pump_ability(player, card, current_turn, attackers[amt], &pump);
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_consuming_rage(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &consuming_rage_legacy);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_reckless_minotaur(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		haste(player, card, event);
		attack_if_able(player, card, event);
		if( eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_DESTROY);
		}
	}
	return 0;
}

int card_phoberos_reaver(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		haste(player, card, event);
		attack_if_able(player, card, event);
	}
	return 0;
}

int card_mogis_chosen(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		haste(player, card, event);
		comes_into_play_tapped(player, card, event);
	}
	return 0;
}

// Minotaur Younghorn / Minotaur Goreseeker --> Phoberos Reaver


