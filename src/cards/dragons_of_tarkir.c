// -*- c-basic-offset:2 -*-
#include "manalink.h"

/***** Functions *****/

static int exploit_impl(int player, int card, event_t event, resolve_trigger_t mode)
{
  // Should not be called directly, only by exploit() and exploit_allow_ai().
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller
	  && can_sacrifice(player, player, 1, TYPE_CREATURE, 0)
	  && comes_into_play_mode(player, card, event, mode))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  return new_sacrifice(player, card, player, SAC_RETURN_CHOICE, &test);
	}

  return 0;
}

static int exploit(int player, int card, event_t event)
{
  /* 702.109. Exploit
   *
   * 702.109a Exploit is a triggered ability. "Exploit" means "When this creature enters the battlefield, you may sacrifice a creature."
   *
   * 702.109b A creature with exploit "exploits a creature" when the controller of the exploit ability sacrifices a creature as that ability resolves. */
  return exploit_impl(player, card, event, RESOLVE_TRIGGER_AI(player));
}

// Just like exploit(), but if allow_ai is zero, then the AI will decline
static int exploit_allow_ai(int player, int card, event_t event, int allow_ai)
{
  return exploit_impl(player, card, event, (allow_ai || !IS_AI(player)) ? RESOLVE_TRIGGER_AI(player) : RESOLVE_TRIGGER_OPTIONAL);
}

static int formidable(int player)
{
  // Returns true if creatures [player] controls have total power 8 or greater.
  card_instance_t* inst;
  int tot = 0, pow, c;
  for (c = 0; c < active_cards_count[player]; ++c)
	if ((inst = in_play(player, c)) && is_what(player, c, TYPE_CREATURE)
		&& (pow = get_power(player, c)) > 0	// Having a -1/3 Char-Rumbler on the bf doesn't count against you
		&& (tot += pow) >= 8)
	  return 1;

  return 0;
}

static int megamorph(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white)
{
  /* 702.36b Megamorph is a variant of the morph ability. "Megamorph [cost]" means "You may cast this card as a 2/2 face-down creature with no text, no name, no
   * subtypes, and no mana cost by paying {3} rather than paying its mana cost" and "As this permanent is turned face up, put a +1/+1 counter on it if its
   * megamorph cost was paid to turn it face up." A megamorph cost is a morph cost. */
  if (event == EVENT_UNMORPH)
	{
	  charge_mana_multi(player, cless, black, blue, green, red, white);
	  if (cancel != 1)
		add_1_1_counter(player, card);
	  return 0;
	}
  else
	return morph(player, card, event, cless, black, blue, green, red, white);
}

static int monument(int player, int card, event_t event, color_test_t cols, int x, int b, int u, int g, int r, int w)
{
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
						"Become Dragon",
							!paying_mana(),
							!(get_card_instance(player, card)->token_status & STATUS_LEGACY_TYPECHANGE),
							DLG_MANA(MANACOST6(x,b,u,g,r,w)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_ANIMATE)
		{
		  if (event == EVENT_RESOLVE_ACTIVATION)
			animate_self(player, card, 4,4, KEYWORD_FLYING,0, cols, 0);
		  return 0;
		}
	}

  has_subtype_if_animated_self(player, card, event, SUBTYPE_DRAGON);

  return mana_producer(player, card, event);
}

static void put_1_1_on_each_other_dragon_creature_you_control(int player, int card)
{
  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "");
  test.subtype = SUBTYPE_DRAGON;
  test.not_me = 1;
  new_manipulate_all(player, card, player, &test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
}

void raise_command_singleton_ai_priorities(event_t event, int* priority);	// in lorwyn.c

int rebound(int player, int card);	// in rise_of_the_eldrazi.c

// Must be called at toplevel, since it has its own event clause.
static int revealed_or_controlled_dragon(int player, int card, event_t event)
{
  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && !is_token(player, card) && !check_special_flags(player, card, SF_NOT_CAST))
	{
	  int has_dragon = check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DRAGON);
	  if (!(has_dragon && IS_AI(player)) && is_subtype_in_hand(player, SUBTYPE_DRAGON))
		{
		  test_definition_t test;
		  new_default_test_definition(&test, 0, get_subtype_text("Select %a card to reveal.", SUBTYPE_DRAGON));
		  test.subtype = SUBTYPE_DRAGON;
		  int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &test);
		  if (selected != -1)
			{
			  reveal_card(player, card, player, selected);
			  has_dragon = 1;
			}
		}
	  get_card_instance(player, card)->info_slot = has_dragon ? CARD_ID_DRACONIC_ROAR : 0;
	}

  return get_card_instance(player, card)->info_slot == CARD_ID_DRACONIC_ROAR;
}

/***** Cards *****/

/*** Colorless ***/

/* Scion of Ugin	|6	=>vanilla
 * Creature - Dragon Spirit 4/4
 * Flying */

/*** White ***/

int card_anafenza_kin_tree_spirit(int player, int card, event_t event)
{
  /* Anafenza, Kin-Tree Spirit	|W|W	0x200d946
   * Legendary Creature - Spirit Soldier 2/2
   * Whenever another nontoken creature enters the battlefield under your control, bolster 1. */

  check_legend_rule(player, card, event);

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;
	  test.type_flag = F1_NO_TOKEN;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		bolster(player, card, 1);
	}

  return 0;
}

int card_arashin_foremost(int player, int card, event_t event)
{
  /* Arashin Foremost	|1|W|W	0x200d94b
   * Creature - Human Warrior 2/2
   * Double strike
   * Whenever ~ enters the battlefield or attacks, another target Warrior creature you control gains double strike until end of turn. */

  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS
	  || trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.special = TARGET_SPECIAL_NOT_ME;
	  td.required_subtype = SUBTYPE_WARRIOR;
	  td.allowed_controller = td.preferred_controller = player;
	  td.allow_cancel = 0;

	  if (can_target(&td)
		  && (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card)
			  || comes_into_play(player, card, event))
		  && select_target(player, card, &td, "Select another target Warrior creature you control.", NULL))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 0,0, KEYWORD_DOUBLE_STRIKE,0);
		}
	}

  return 0;
}

int card_artful_maneuver(int player, int card, event_t event)
{
  /* Artful Maneuver	|1|W	0x200d950
   * Instant
   * Target creature gets +2/+2 until end of turn.
   * Rebound */

  int rval = vanilla_instant_pump(player, card, event, ANYBODY, player, 2,2, VANILLA_PUMP_DONT_KILL_CARD,0);
  if (rval && event == EVENT_RESOLVE_SPELL)
	alternate_legacy_text(2, player, rebound(player, card));

  return rval;
}

int card_aven_sunstriker(int player, int card, event_t event)
{
  /* Aven Sunstriker	|1|W|W	0x200d8bf
   * Creature - Bird Warrior 1/1
   * Flying
   * Double strike
   * Megamorph |4|W */
  return megamorph(player, card, event, MANACOST_XW(4, 1));
}

int card_aven_tactician(int player, int card, event_t event)
{
  /* Aven Tactician	|4|W	0x200d955
   * Creature - Bird Soldier 2/3
   * Flying
   * When ~ enters the battlefield, bolster 1. */

  if (comes_into_play(player, card, event))
	bolster(player, card, 1);

  return 0;
}

/* Battle Mastery	|2|W	=>lorwyn.c:card_battle_mastery
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature has double strike. */

/* Center Soul	|1|W	=>rise_of_the_eldrazi.c:card_emerge_unschated
 * Instant
 * Target creature you control gains protection from the color of your choice until end of turn.
 * Rebound */

/* Champion of Arashin	|3|W	=>m10.c:card_child_of_night
 * Creature - Hound Warrior 3/2
 * Lifelink */

/* Dragon Hunter	|W	0x000000
 * Creature - Human Warrior 2/1
 * Protection from Dragons
 * ~ can block Dragons as though it had reach. */

/* Dragon's Eye Sentry	|W	=>vanilla
 * Creature - Human Monk 1/3
 * Defender, first strike */

int card_dromoka_captain(int player, int card, event_t event)
{
  /* Dromoka Captain	|2|W	0x200d95a
   * Creature - Human Soldier 1/1
   * First strike
   * Whenever ~ attacks, bolster 1. */

  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	bolster(player, card, 1);

  return 0;
}

int card_dromoka_dunecaster(int player, int card, event_t event)
{
  /* Dromoka Dunecaster	|W	0x200d95f
   * Creature - Human Wizard 0/2
   * |1|W, |T: Tap target creature without flying. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.illegal_abilities |= KEYWORD_FLYING;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  tap_card(inst->targets[0].player, inst->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_XW(1,1), 0, &td, "FLOOD");	// "Select target creature without flying."
}

/* Dromoka Warrior	|1|W	=>vanilla
 * Creature - Human Warrior 3/1 */

int card_echoes_of_the_kin_tree(int player, int card, event_t event)
{
  /* Echoes of the Kin Tree	|1|W	0x200d964
   * Enchantment
   * |2|W: Bolster 1. */

  if (event == EVENT_SHOULD_AI_PLAY || event == EVENT_CAN_CAST)
	return global_enchantment(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	bolster(player, card, 1);
  return generic_activated_ability(player, card, event, 0, MANACOST_XW(2,1), 0, NULL, NULL);
}

int card_enduring_victory(int player, int card, event_t event)
{
  /* Enduring Victory	|4|W	0x200d969
   * Instant
   * Destroy target attacking or blocking creature. Bolster 1. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_state = TARGET_STATE_IN_COMBAT;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		  bolster(player, card, 1);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ATTACKING_BLOCKING_CREATURE", 1, NULL);
}

/* Fate Forgotten	|2|W	=>scars_of_mirrodin.c:card_revoke_existence
 * Instant
 * Exile target artifact or enchantment. */

int card_glaring_aegis(int player, int card, event_t event)
{
  /* Glaring Aegis	|W	0x200d96e
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield, tap target creature an opponent controls.
   * Enchanted creature gets +1/+3. */

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  tap_card(inst->targets[0].player, inst->targets[0].card);
		  inst->number_of_targets = 0;
		}
	}

  return generic_aura(player, card, event, player, 1,3, 0,0, 0,0,0);
}

int card_gleam_of_authority(int player, int card, event_t event)
{
  /* Gleam of Authority	|1|W	0x200d991
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +1/+1 for each +1/+1 counter on other creatures you control.
   * Enchanted creature has vigilance and "|W, |T: Bolster 1." */

  card_instance_t* inst;

  if (event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  inst = get_card_instance(player, card);
	  if (affect_me(inst->damage_target_player, inst->damage_target_card))
		{
		  // Total +1/+1 counters on all creatures you control
		  event_result += count_counters_by_counter_and_card_type(player, COUNTER_P1_P1, TYPE_CREATURE);
		  // ...but not on this creature, if it was counted
		  if (inst->damage_target_player == player)
			event_result -= count_counters(inst->damage_target_player, inst->damage_target_card, COUNTER_P1_P1);
		}
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	bolster(player, card, 1);
  else if (IS_GAA_EVENT(event) && (inst = in_play(player, card)) && inst->damage_target_player == player)
	return granted_generic_activated_ability(player, card, inst->damage_target_player, inst->damage_target_card, event,
											 GAA_UNTAPPED, MANACOST_W(1), 0, NULL, NULL);

  return generic_aura(player, card, event, player, 0,0, 0,SP_KEYWORD_VIGILANCE, 0,0,0);
}

int card_graceblade_artisan(int player, int card, event_t event)
{
  /* Graceblade Artisan	|2|W	0x200d973
   * Creature - Human Monk 2/3
   * ~ gets +2/+2 for each Aura attached to it. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card))
	event_result += 2 * count_auras_enchanting_me(player, card);

  return 0;
}

int card_great_teachers_decree(int player, int card, event_t event)
{
  /* Great Teacher's Decree	|3|W	0x200d978
   * Sorcery
   * Creatures you control get +2/+1 until end of turn.
   * Rebound */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  pump_creatures_until_eot(player, card, player, 1, 2,1, 0,0, NULL);
	  alternate_legacy_text(2, player, rebound(player, card));
	}

  return basic_spell(player, card, event);
}

int card_herald_of_dromoka(int player, int card, event_t event)
{
  /* Herald of Dromoka	|1|W	0x200d97d
   * Creature - Human Warrior 2/2
   * Vigilance
   * Other Warrior creatures you control have vigilance. */

  vigilance(player, card, event);
  boost_subtype(player, card, event, SUBTYPE_WARRIOR, 0,0, 0,SP_KEYWORD_VIGILANCE, BCT_CONTROLLER_ONLY);
  return 0;
}

int card_hidden_dragonslayer(int player, int card, event_t event)
{
  /* Hidden Dragonslayer	|1|W	0x200d982
   * Creature - Human Warrior 2/1
   * Lifelink
   * Megamorph |2|W
   * When ~ is turned face up, destroy target creature with power 4 or greater an opponent controls. */

  lifelink(player, card, event);

  if (event == EVENT_TURNED_FACE_UP)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.power_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && select_target(player, card, &td, "Select target creature with power 4 or greater an opponent controls.", NULL))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		}
	}

  return megamorph(player, card, event, MANACOST_XW(2,1));
}

int card_lightwalker(int player, int card, event_t event)
{
  /* Lightwalker	|1|W	0x200d987
   * Creature - Human Warrior 2/1
   * ~ has flying as long as it has a +1/+1 counter on it. */

  if (event == EVENT_ABILITIES && affect_me(player, card) && count_1_1_counters(player, card) > 0 && !is_humiliated(player, card))
	event_result |= KEYWORD_FLYING;

  return 0;
}

int card_misthoof_kirin(int player, int card, event_t event)
{
  /* Misthoof Kirin	|2|W	0x200d8c4
   * Creature - Kirin 2/1
   * Flying, vigilance
   * Megamorph |1|W */
  vigilance(player, card, event);
  return megamorph(player, card, event, MANACOST_XW(1,1));
}

int card_myth_realized(int player, int card, event_t event)
{
  /* Myth Realized	|W	0x200d98c
   * Enchantment
   * Whenever you cast a noncreature spell, put a lore counter on ~.
   * |2|W: Put a lore counter on ~.
   * |W: Until end of turn, ~ becomes a Monk Avatar creature in addition to its other types and gains "This creature's power and toughness are each equal to the number of lore counters on it." */

  if (prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	add_counter(player, card, COUNTER_LORE);

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  enum
	  {
		CHOICE_COUNTER = 1,
		CHOICE_ANIMATE
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Lore counter",		1,	1,												DLG_MANA(MANACOST_XW(2,1)),
						"Animate",			1,	is_what(player, card, TYPE_CREATURE) ? 1 : 0,	DLG_MANA(MANACOST_W(1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_COUNTER:
			  add_counter(player, card, COUNTER_LORE);
			  break;

			case CHOICE_ANIMATE:
			  animate_self(player, card, 0,0, 0,0, 0, 0);
			  break;
		  }
	}

  has_subtypes_if_animated_self(player, card, event, SUBTYPE_MONK, SUBTYPE_AVATAR);

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->token_status & STATUS_LEGACY_TYPECHANGE)
		event_result += count_counters(player, card, COUNTER_LORE);
	}

  return global_enchantment(player, card, event);
}

static void add_state_tapped(int player, int card)
{
  // not tap_card(), since that would send an EVENT_TAP_CARD event; the intent is to enter the bf tapped, not to enter the bf, then tap.
  get_card_instance(player, card)->state |= STATE_TAPPED;
}
int card_ojutai_exemplars(int player, int card, event_t event)
{
  /* Ojutai Exemplars	|2|W|W	0x200d99b
   * Creature - Human Monk 4/4
   * Whenever you cast a noncreature spell, choose one -
   * * Tap target creature.
   * * ~ gains first strike and lifelink until end of turn.
   * * Exile ~, then return it to the battlefield tapped under its owner's control. */

  if (prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allow_cancel = 0;

	  target_definition_t td_pref;
	  default_target_definition(player, card, &td_pref, TYPE_CREATURE);
	  td_pref.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  int ai_tap	= can_target(&td_pref) ? 3 : 1;
	  int ai_pump	= (check_for_ability(player, card, KEYWORD_FIRST_STRIKE) ? 0 : 1) + (check_for_special_ability(player, card, SP_KEYWORD_LIFELINK) ? 0 : 1);
	  ai_pump *= ai_pump;	// 4 if has neither ability, 1 if has one or the other, 0 if has neither
	  int ai_blink	= is_stolen(player, card) ? 0 : 2;

	  switch (DIALOG(player, card, EVENT_ACTIVATE,
					 DLG_RANDOM, DLG_NO_CANCEL, DLG_NO_STORAGE,
					 "Tap creature",			can_target(&td),	ai_tap,
					 "First strike, lifelink",	1,					ai_pump,
					 "Exile and return",		1,					ai_blink))
		{
		  case 1:
			;card_instance_t* inst = get_card_instance(player, card);
			pick_target(&td, "TARGET_CREATURE");
			inst->number_of_targets = 0;
			tap_card(inst->targets[0].player, inst->targets[0].card);
			break;

		  case 2:
			pump_ability_until_eot(player, card, player, card, 0,0, KEYWORD_FIRST_STRIKE,SP_KEYWORD_LIFELINK);
			break;

		  case 3:
			blink_effect(player, card, &add_state_tapped);
			break;
		}
	}

  return 0;
}

int card_orator_of_ojutai(int player, int card, event_t event)
{
  /* Orator of Ojutai	|1|W	0x200d9a5
   * Creature - Bird Monk 0/4
   * As an additional cost to cast ~, you may reveal a Dragon card from your hand.
   * Defender, flying
   * When ~ enters the battlefield, if you revealed a Dragon card or controlled a Dragon as you cast ~, draw a card. */

  if (revealed_or_controlled_dragon(player, card, event)	// must come first
	  && comes_into_play(player, card, event))
	draw_a_card(player);

  return 0;
}

/* Pacifism	|1|W	=>mirage.c:card_pacifism
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature can't attack or block. */

int card_profound_journey(int player, int card, event_t event)
{
  /* Profound Journey	|5|W|W	0x200d9c3
   * Sorcery
   * Return target permanent card from your graveyard to the battlefield.
   * Rebound */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_PERMANENT, "Select target permanent card.");

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int selected = validate_target_from_grave_source(player, card, player, 0);
	  if (selected != -1)
		{
		  reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
		  rebound(player, card);
		}
	  else
		kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &test);
}

int card_radiant_purge(int player, int card, event_t event)
{
  /* Radiant Purge	|1|W	0x200d9c8
   * Instant
   * Exile target multicolored creature or multicolored enchantment. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE|TYPE_ENCHANTMENT);
  td.preferred_controller = player;
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int)is_multicolored;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_REMOVE);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_LITERAL_PROMPT, &td,
					   "Select target multicolored creature or multicolored enchantment.", 1, NULL);
}

int card_resupply(int player, int card, event_t event)
{
  /* Resupply	|5|W	0x200d9a0
   * Instant
   * You gain 6 life.
   * Draw a card. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  gain_life(player, 6);
	  draw_a_card(player);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Sandcrafter Mage	|2|W	=>card_aven_tactician
 * Creature - Human Wizard 2/2
 * When ~ enters the battlefield, bolster 1. */

/* Sandstorm Charger	|4|W	=>card_aven_sunstriker
 * Creature - Beast 3/4
 * Megamorph |4|W */

int card_scale_blessing(int player, int card, event_t event)
{
  /* Scale Blessing	|3|W	0x200d9cd
   * Instant
   * Bolster 1, then put a +1/+1 counter on each creature you control with a +1/+1 counter on it. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  bolster(player, card, 1);
	  int c;
	  char has_counter[151] = {0};
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && is_what(player, c, TYPE_CREATURE) && count_1_1_counters(player, c) > 0)
		  has_counter[c] = 1;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (has_counter[c] && in_play(player, c))
		  add_1_1_counter(player, c);
	}

  return basic_spell(player, card, event);
}

int card_secure_the_wastes(int player, int card, event_t event)
{
  /* Secure the Wastes	|X|W	0x200d9d2
   * Instant
   * Put X 1/1 |Swhite Warrior creature tokens onto the battlefield. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_WARRIOR, &token);
	  token.color_forced = COLOR_TEST_WHITE;
	  token.qty = get_card_instance(player, card)->info_slot;
	  if (token.qty > 0)
		generate_token(&token);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_shieldhide_dragon(int player, int card, event_t event)
{
  /* Shieldhide Dragon	|5|W	0x200d9d7
   * Creature - Dragon 3/3
   * Flying, lifelink
   * Megamorph |5|W|W
   * When ~ is turned face up, put a +1/+1 counter on each other Dragon creature you control. */

  lifelink(player, card, event);

  if (event == EVENT_TURNED_FACE_UP)
	put_1_1_on_each_other_dragon_creature_you_control(player, card);

  return megamorph(player, card, event, MANACOST_XW(5,2));
}

int card_silkwrap(int player, int card, event_t event)
{
  /* Silkwrap	|1|W	0x200d9f0
   * Enchantment
   * When ~ enters the battlefield, exile target creature with converted mana cost 3 or less an opponent controls until ~ leaves the battlefield. */

  return_from_oblivion(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.extra = 3;
	  td.special = TARGET_SPECIAL_CMC_LESSER_OR_EQUAL;
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && select_target(player, card, &td, "Select target creature with converted mana cost 3 or less an opponent controls.", NULL))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  obliviation(player, card, inst->targets[0].player, inst->targets[0].card);
		}
	}

  return global_enchantment(player, card, event);
}

int card_strongarm_monk(int player, int card, event_t event)
{
  /* Strongarm Monk	|4|W	0x200d9f5
   * Creature - Human Monk 3/3
   * Whenever you cast a noncreature spell, creatures you control get +1/+1 until end of turn. */

  if (prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_creatures_until_eot_merge_pt(player, card, player, 1, 1, NULL);

  return 0;
}

int card_student_of_ojutai(int player, int card, event_t event)
{
  /* Student of Ojutai	|3|W	0x200d9fa
   * Creature - Human Monk 2/4
   * Whenever you cast a noncreature spell, you gain 2 life. */

  if (prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	gain_life(player, 2);

  return 0;
}

int card_sunscorch_regent(int player, int card, event_t event)
{
  /* Sunscorch Regent	|3|W|W	0x200d9ff
   * Creature - Dragon 4/3
   * Flying
   * Whenever an opponent casts a spell, put a +1/+1 counter on ~ and you gain 1 life. */

  if (specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, 0,0, 0,0, 0,0, 0,0, -1,0))
	{
	  add_1_1_counter(player, card);
	  gain_life(player, 1);
	}

  return 0;
}

int card_surge_of_righteousness(int player, int card, event_t event)
{
  /* Surge of Righteousness	|1|W	0x200da04
   * Instant
   * Destroy target |Sblack or |Sred creature that's attacking or blocking. You gain 2 life. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_state = TARGET_STATE_IN_COMBAT;
  td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK|COLOR_TEST_RED);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		  gain_life(player, 2);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_LITERAL_PROMPT, &td,
					   get_sleighted_color_text2(player, card, "Select target %s or %s creature that's attacking or blocking.", COLOR_BLACK, COLOR_RED),
					   1, NULL);
}

/* Territorial Roc	|1|W	=>vanilla
 * Creature - Bird 1/3
 * Flying */

/*** Blue ***/

/* Ancient Carp	|4|U	=>vanilla
 * Creature - Fish 2/5 */

int card_anticipate(int player, int card, event_t event)
{
  /* Anticipate	|1|U	0x200da09
   * Instant
   * Look at the top three cards of your library. Put one of them into your hand and the rest on the bottom of your library in any order. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  reveal_top_cards_of_library_and_choose_type(player, card, player, 3, 1, TUTOR_HAND,0, TUTOR_BOTTOM_OF_DECK,0, TYPE_ANY);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_belltoll_dragon(int player, int card, event_t event)
{
  /* Belltoll Dragon	|5|U	0x200d9dc
   * Creature - Dragon 3/3
   * Flying, hexproof
   * Megamorph |5|U|U
   * When ~ is turned face up, put a +1/+1 counter on each other Dragon creature you control. */

  hexproof(player, card, event);

  if (event == EVENT_TURNED_FACE_UP)
	put_1_1_on_each_other_dragon_creature_you_control(player, card);

  return megamorph(player, card, event, MANACOST_XU(5,2));
}

int card_blessed_reincarnation(int player, int card, event_t event)
{
  /* Blessed Reincarnation	|3|U	0x200da0e
   * Instant
   * Exile target creature an opponent controls. That player reveals cards from the top of his or her library until a creature card is revealed. The player puts that card onto the battlefield, then shuffles the rest into his or her library.
   * Rebound */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = 1-player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  metamorphosis(inst->targets[0].player, inst->targets[0].card, TYPE_CREATURE, KILL_REMOVE);
		  rebound(player, card);
		}
	  else
		kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OPPONENT_CONTROLS", 1, NULL);
}

int card_clone_legion(int player, int card, event_t event)
{
  /* Clone Legion	|7|U|U	0x200da13
   * Sorcery
   * For each creature target player controls, put a token onto the battlefield that's a copy of that creature. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.preferred_controller = ANYBODY;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  int c, p = get_card_instance(player, card)->targets[0].player;
		  char marked[151] = {0};
		  for (c = 0; c < active_cards_count[p]; ++c)
			if (in_play(p, c) && is_what(p, c, TYPE_CREATURE))
			  marked[c] = 1;

		  for (c = 0; c < active_cards_count[player]; ++c)
			if (marked[c])
			  copy_token(player, card, p, c);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

/* Contradict	|3|U|U	=>tempest.c:card_dismiss2
 * Instant
 * Counter target spell.
 * Draw a card. */

int card_dance_of_the_skywise(int player, int card, event_t event)
{
  /* Dance of the Skywise	|1|U	0x200da18
   * Instant
   * Until end of turn, target creature you control becomes a |Sblue Dragon Illusion with base power and toughness 4/4, loses all abilities, and gains flying. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  test_definition_t test;
		  new_default_test_definition(&test, 0, "");
		  test.power = test.toughness = 4;
		  test.subtype = SUBTYPE_DRAGON;	// This only has to be set to something > 0; it doesn't actually matter what
		  test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
		  test.keyword = KEYWORD_FLYING;

		  card_instance_t* inst = get_card_instance(player, card);
		  force_a_subtype(inst->targets[0].player, inst->targets[0].card, SUBTYPE_DRAGON);
		  add_a_subtype(inst->targets[0].player, inst->targets[0].card, SUBTYPE_ILLUSION);
		  humiliate_and_set_pt_abilities(player, card, inst->targets[0].player, inst->targets[0].card, 4, &test);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "ASHNODS_BATTLEGEAR", 1, NULL);	// "Select target creature you control."
}

int card_dirgur_nemesis(int player, int card, event_t event)
{
  /* Dirgur Nemesis	|5|U	0x200d8c9
   * Creature - Serpent 6/5
   * Defender
   * Megamorph |6|U */
  return megamorph(player, card, event, MANACOST_XU(6,1));
}

int card_dragonlords_prerogative(int player, int card, event_t event)
{
  /* Dragonlord's Prerogative	|4|U|U	0x200d9aa
   * Instant
   * As an additional cost to cast ~, you may reveal a Dragon card from your hand.
   * If you revealed a Dragon card or controlled a Dragon as you cast ~, ~ can't be countered.
   * Draw four cards. */

  if (revealed_or_controlled_dragon(player, card, event))	// must come first
	cannot_be_countered(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  draw_cards(player, 4);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_elusive_spellfist(int player, int card, event_t event)
{
  /* Elusive Spellfist	|1|U	0x200da1d
   * Creature - Human Monk 1/3
   * Whenever you cast a noncreature spell, ~ gets +1/+0 until end of turn and can't be blocked this turn. */

  if (prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_ability_until_eot(player, card, player, card, 1,0, 0,SP_KEYWORD_UNBLOCKABLE);

  return 0;
}

int card_encase_in_ice(int player, int card, event_t event)
{
  /* Encase in Ice	|1|U	0x200da22
   * Enchantment - Aura
   * Flash
   * Enchant |Sred or |Sgreen creature
   * When ~ enters the battlefield, tap enchanted creature.
   * Enchanted creature doesn't untap during its controller's untap step. */

  card_instance_t* inst = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_RED|COLOR_TEST_GREEN);

  if (inst->damage_target_player >= 0)
	{
	  if (comes_into_play(player, card, event))
		tap_card(inst->damage_target_player, inst->damage_target_card);

	  does_not_untap(inst->damage_target_player, inst->damage_target_card, event);

	  if (event == EVENT_STATIC_EFFECTS
		  && !(get_color(inst->damage_target_player, inst->damage_target_card) & td.required_color))
		kill_card(player, card, KILL_STATE_BASED_ACTION);
	}

  return targeted_aura(player, card, event, &td, get_sleighted_color_text2(player, card, "Select target %s or %s creature.", COLOR_RED, COLOR_GREEN));
}

int card_glint(int player, int card, event_t event)
{
  /* Glint	|1|U	0x200da27
   * Instant
   * Target creature you control gets +0/+3 and gains hexproof until end of turn. */

  return vanilla_instant_pump(player, card, event, player, player, 0,3, 0,SP_KEYWORD_HEXPROOF);
}

int card_gudul_lurker(int player, int card, event_t event)
{
  /* Gudul Lurker	|U	0x200d8ce
   * Creature - Salamander 1/1
   * ~ can't be blocked.
   * Megamorph |U */

  unblockable(player, card, event);
  return megamorph(player, card, event, MANACOST_U(1));
}

int card_gurmag_drowner(int player, int card, event_t event)
{
  /* Gurmag Drowner	|3|U	0x200da2c
   * Creature - Naga Wizard 2/4
   * Exploit
   * When ~ exploits a creature, look at the top four cards of your library. Put one of them into your hand and the rest into your graveyard. */

  if (exploit(player, card, event))
	reveal_top_cards_of_library_and_choose_type(player, card, player, 4, 1, TUTOR_HAND,0, TUTOR_GRAVE,0, TYPE_ANY);

  return 0;
}

/* Icefall Regent	|3|U|U	0x000000
 * Creature - Dragon 4/3
 * Flying
 * When ~ enters the battlefield, tap target creature an opponent controls. That creature doesn't untap during its controller's untap step for as long as you control ~.
 * Spells your opponents cast that target ~ cost |2 more to cast. */

int card_illusory_gains(int player, int card, event_t event)
{
  /* Illusory Gains	|3|U|U	0x200da31
   * Enchantment - Aura
   * Enchant creature
   * You control enchanted creature.
   * Whenever a creature enters the battlefield under an opponent's control, attach ~ to that creature. */

  if (specific_cip(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  inst->targets[0].player = trigger_cause_controller;
	  inst->targets[0].card = trigger_cause;
	  return call_card_function_i(inst, player, card, EVENT_RESOLVE_MOVING_AURA);
	}

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  return generic_stealing_aura(player, card, event, &td, "TARGET_CREATURE");
}

int card_learn_from_the_past(int player, int card, event_t event)
{
  /* Learn from the Past	|3|U	0x200da36
   * Instant
   * Target player shuffles his or her graveyard into his or her library.
   * Draw a card. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  reshuffle_grave_into_deck(get_card_instance(player, card)->targets[0].player, 0);
		  draw_a_card(player);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

/* Living Lore	|3|U	0x000000
 * Creature - Avatar 100/100
 * As ~ enters the battlefield, exile an instant or sorcery card from your graveyard.
 * ~'s power and toughness are each equal to the exiled card's converted mana cost.
 * Whenever ~ deals combat damage, you may sacrifice it. If you do, you may cast the exiled card without paying its mana cost. */

static int fx_exile_at_end_of_combat(int player, int card, event_t event)
{
  if (end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->damage_target_player >= 0)
		kill_card(inst->damage_target_player, inst->damage_target_card, KILL_REMOVE);
	}

  if (trigger_condition == TRIGGER_END_COMBAT && affect_me(player, card) && reason_for_trigger_controller == player
	  && event == EVENT_END_TRIGGER)	// If trigger is countered, it doesn't repeat next combat
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
int card_mirror_mockery(int player, int card, event_t event)
{
  /* Mirror Mockery	|1|U	0x200da3b
   * Enchantment - Aura
   * Enchant creature
   * Whenever enchanted creature attacks, you may put a token onto the battlefield that's a copy of that creature. Exile that token at end of combat. */

  card_instance_t* inst = in_play(player, card);
  if (inst && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), inst->damage_target_player, inst->damage_target_card))
	{
	  token_generation_t token;
	  copy_token_definition(player, card, &token, inst->damage_target_player, inst->damage_target_card);
	  token.legacy = 1;
	  token.special_code_for_legacy = &fx_exile_at_end_of_combat;
	  generate_token(&token);
	}

  return vanilla_aura(player, card, event, 1-player);
}

int card_monastery_loremaster(int player, int card, event_t event)
{
  /* Monastery Loremaster	|3|U	0x200da40
   * Creature - Djinn Wizard 3/2
   * Megamorph |5|U
   * When ~ is turned face up, return target noncreature, nonland card from your graveyard to your hand. */

  if (event == EVENT_TURNED_FACE_UP)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE | TYPE_LAND, "Select target noncreature, nonland card.");
	  test.type_flag = DOESNT_MATCH;
	  if (new_special_count_grave(player, &test) && !graveyard_has_shroud(player))
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &test);
	}

  return megamorph(player, card, event, MANACOST_XU(5,1));
}

int card_mystic_meditation(int player, int card, event_t event)
{
  /* Mystic Meditation	|3|U	0x200da45
   * Sorcery
   * Draw three cards. Then discard two cards unless you discard a creature card. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  draw_cards(player, 3);

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ARTIFACT, "Select a creature card to discard.");
	  test.zone = TARGET_ZONE_HAND;

	  int selected;
	  if (check_battlefield_for_special_card(player, card, player, 0, &test))
		selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &test);
	  else
		selected = -1;

	  if (selected != -1)
		discard_card(player, selected);
	  else
		multidiscard(player, 2, 0);

	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Negate	|1|U	=>morningtide.c:card_negate
 * Instant
 * Counter target noncreature spell. */

int card_ojutai_interceptor(int player, int card, event_t event)
{
  /* Ojutai Interceptor	|3|U	0x200d8d3
   * Creature - Bird Soldier 3/1
   * Flying
   * Megamorph |3|U */
  return megamorph(player, card, event, MANACOST_XU(3,1));
}

int card_ojutais_breath(int player, int card, event_t event)
{
  /* Ojutai's Breath	|2|U	0x200da4a
   * Instant
   * Tap target creature. It doesn't untap during its controller's next untap step.
   * Rebound */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  alternate_legacy_text(1, player, effect_frost_titan(player, card, inst->targets[0].player, inst->targets[0].card));
		  alternate_legacy_text(2, player, rebound(player, card));
		}
	  else
		kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_ojutais_summons(int player, int card, event_t event)
{
  /* Ojutai's Summons	|3|U|U	0x200da4f
   * Sorcery
   * Put a 2/2 |Sblue Djinn Monk creature token with flying onto the battlefield.
   * Rebound */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  generate_token_by_id(player, card, CARD_ID_DJINN_MONK);
	  rebound(player, card);
	}

  return basic_spell(player, card, event);
}

/* Palace Familiar	|1|U	=>scars_of_mirrodin.c:card_darkslick_drake
 * Creature - Bird 1/1
 * Flying
 * When ~ dies, draw a card. */

int card_profaner_of_the_dead(int player, int card, event_t event)
{
  /* Profaner of the Dead	|3|U	0x200da54
   * Creature - Naga Wizard 3/3
   * Exploit
   * When ~ exploits a creature, return to their owners' hands all creatures your opponents control with toughness less than the exploited creature's toughness. */

  int expl;
  if ((expl = exploit(player, card, event)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.toughness = get_toughness(BYTE2(expl), BYTE3(expl));
	  test.toughness_flag = F5_TOUGHNESS_LESSER_THAN_VALUE;
	  new_manipulate_all(player, card, 1-player, &test, ACT_BOUNCE);
	}

  return 0;
}

/* Qarsi Deceiver	|1|U	0x000000
 * Creature - Naga Wizard 0/4
 * |T: Add |1 to your mana pool. Spend this mana only to cast a face-down creature spell, pay a mana cost to turn a manifested creature face up, or pay a morph cost. */

int card_reduce_in_stature(int player, int card, event_t event)
{
  /* Reduce in Stature	|2|U	0x200da59
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature has base power and toughness 0/2. */

  if (event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  card_instance_t* inst = in_play(player, card);
	  if (inst && inst->damage_target_player >= 0 && affect_me(inst->damage_target_player, inst->damage_target_card))
		{
		  if (event == EVENT_POWER)
			event_result += 0 - get_base_power(inst->damage_target_player, inst->damage_target_card);
		  else
			event_result += 2 - get_base_toughness(inst->damage_target_player, inst->damage_target_card);
		}
	}

  return vanilla_aura(player, card, event, 1-player);
}

int card_shorecrasher_elemental(int player, int card, event_t event)
{
  /* Shorecrasher Elemental	|U|U|U	0x200da5e
   * Creature - Elemental 3/3
   * |U: Exile ~, then return it to the battlefield face down under its owner's control.
   * |1: ~ gets +1/-1 or -1/+1 until end of turn.
   * Megamorph |4|U */

  if (event == EVENT_POW_BOOST)
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_X(1),	1,-1);
  if (event == EVENT_TOU_BOOST)
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_X(1),	-1,1);

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  enum
	  {
		CHOICE_BLINK = 1,
		CHOICE_PUMP
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Exile and return",	1, 1,	DLG_MANA(MANACOST_U(1)),
						"+1/-1 or -1/+1",	1, 5,	DLG_MANA(MANACOST_X(1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  switch (choice)
			{
			  case CHOICE_BLINK:
				blink_effect(inst->parent_controller, inst->parent_card, &turn_face_down);
				break;

			  case CHOICE_PUMP:
				// 2/25/2015	You choose whether Shorecrasher Elemental gets +1/-1 or -1/+1 as the last activated ability resolves.
				if (in_play(inst->parent_controller, inst->parent_card))
				  {
					if (DIALOG(player, card, EVENT_ACTIVATE,
							   DLG_RANDOM, DLG_NO_CANCEL, DLG_NO_STORAGE,
							   "+1/-1",	1, 1,
							   "-1/+1",	1, 1) == 1)
					  pump_until_eot_merge_previous(player, card, inst->parent_controller, inst->parent_card, 1, -1);
					else
					  pump_until_eot_merge_previous(player, card, inst->parent_controller, inst->parent_card, -1, 1);
				  }
			}
		}
	}

  return megamorph(player, card, event, MANACOST_XU(4,1));
}

int card_sidisis_faithful(int player, int card, event_t event)
{
  /* Sidisi's Faithful	|U	0x200da63
   * Creature - Naga Wizard 0/4
   * Exploit
   * When ~ exploits a creature, return target creature to its owner's hand. */

  if (exploit(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  bounce_permanent(inst->targets[0].player, inst->targets[0].card);
		  inst->number_of_targets = 0;
		}
	}

  return 0;
}

int card_sight_beyond_sight(int player, int card, event_t event)
{
  /* Sight Beyond Sight	|3|U	0x200da68
   * Sorcery
   * Look at the top two cards of your library. Put one of them into your hand and the other on the bottom of your library.
   * Rebound */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  reveal_top_cards_of_library_and_choose_type(player, card, player, 2, 1, TUTOR_HAND,0, TUTOR_BOTTOM_OF_DECK,0, TYPE_ANY);
	  rebound(player, card);
	}

  return basic_spell(player, card, event);
}

int card_silumgar_sorcerer(int player, int card, event_t event)
{
  /* Silumgar Sorcerer	|1|U|U	0x200da6d
   * Creature - Human Wizard 2/1
   * Flash
   * Flying
   * Exploit
   * When ~ exploits a creature, counter target creature spell. */

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_CAN_CAST)
	{
	  if (card_counterspell(player, card, EVENT_CAN_CAST))
		{
		  inst->info_slot = 1;
		  return 99;
		}
	  else
		{
		  inst->info_slot = 0;
		  return 1;
		}
	}

  if (event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI)
	ai_modifier -= 16;

  if (exploit_allow_ai(player, card, event, inst->info_slot)
	  && inst->info_slot == 1)
	real_counter_a_spell(player, card, card_on_stack_controller, card_on_stack);

  return 0;
}

int card_silumgar_spell_eater(int player, int card, event_t event)
{
  /* Silumgar Spell-Eater	|2|U	0x200e1f7
   * Creature - Naga Wizard 2/3
   * Megamorph |4|U
   * When ~ is turned face up, counter target spell unless its controller pays |3. */

  int rval = megamorph(player, card, event, MANACOST_XU(4,1));

  if (event != EVENT_CAN_UNMORPH && event != EVENT_UNMORPH && event != EVENT_TURNED_FACE_UP)
	return rval;

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_CAN_UNMORPH && rval
	  && (inst->targets[9].card = counterspell(player, card, event, NULL, 0)) == 99)
	return 99;

  if (event == EVENT_UNMORPH && cancel != 1)
	{
	  int countering = 0;

	  if (inst->targets[9].card == 99 && card_on_stack_controller != -1)
		{
		  counterspell(player, card, EVENT_CAST_SPELL, NULL, 0);
		  if (cancel != 1)
			countering = 1;
		}

	  if (!countering)
		{
		  cancel = 0;
		  inst->targets[9].card = 0;
		}
	  if (player == AI && (!countering || card_on_stack_controller == AI))
		ai_modifier -= 96;
	}

  if (event == EVENT_TURNED_FACE_UP)
	{
	  if (inst->targets[9].card == 99)
		counterspell_resolve_unless_pay_x(player, card, NULL, 0, 3);
	  cancel = 0;
	}

  return rval;
}

int card_silumgars_scorn(int player, int card, event_t event)
{
  /* Silumgar's Scorn	|U|U	0x200d9af
   * Instant
   * As an additional cost to cast ~, you may reveal a Dragon card from your hand.
   * Counter target spell unless its controller pays |1. If you revealed a Dragon card or controlled a Dragon as you cast ~, counter that spell instead. */

  if (!revealed_or_controlled_dragon(player, card, event)	// must come first
	  && event == EVENT_RESOLVE_SPELL)
	{
	  counterspell_resolve_unless_pay_x(player, card, NULL, 0, 1);
	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}
  else
	return counterspell(player, card, event, NULL, 0);
}

int card_skywise_teachings(int player, int card, event_t event)
{
  /* Skywise Teachings	|3|U	0x200da72
   * Enchantment
   * Whenever you cast a noncreature spell, you may pay |1|U. If you do, put a 2/2 |Sblue Djinn Monk creature token with flying onto the battlefield. */

  if (trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller
	  && has_mana_multi(player, MANACOST_XU(1,1))
	  && prowess_mode(player, card, event, RESOLVE_TRIGGER_AI(player))
	  && charge_mana_multi_while_resolving(player, card, event, player, MANACOST_XU(1,1)))
	generate_token_by_id(player, card, CARD_ID_DJINN_MONK);

  return global_enchantment(player, card, event);
}

int card_stratus_dancer(int player, int card, event_t event)
{
  /* Stratus Dancer	|1|U	0x200e1fc
   * Creature - Djinn Monk 2/1
   * Flying
   * Megamorph |1|U
   * When ~ is turned face up, counter target instant or sorcery spell. */

  int rval = megamorph(player, card, event, MANACOST_XU(1,1));

  if (event != EVENT_CAN_UNMORPH && event != EVENT_UNMORPH && event != EVENT_TURNED_FACE_UP)
	return rval;

  card_instance_t* inst = get_card_instance(player, card);

  target_definition_t td;
  counterspell_target_definition(player, card, &td, TYPE_INSTANT | TYPE_INTERRUPT | TYPE_SORCERY);

  if (event == EVENT_CAN_UNMORPH && rval
	  && (inst->targets[9].card = counterspell(player, card, event, &td, 0)) == 99)
	return 99;

  if (event == EVENT_UNMORPH && cancel != 1)
	{
	  int countering = 0;

	  if (inst->targets[9].card == 99 && card_on_stack_controller != -1)
		{
		  counterspell(player, card, EVENT_CAST_SPELL, &td, 0);
		  if (cancel != 1)
			countering = 1;
		}

	  if (!countering)
		{
		  cancel = 0;
		  inst->targets[9].card = 0;
		}
	  if (player == AI && (!countering || card_on_stack_controller == AI))
		ai_modifier -= 96;
	}

  if (event == EVENT_TURNED_FACE_UP)
	{
	  if (inst->targets[9].card == 99 && counterspell_validate(player, card, &td, 0))
		real_counter_a_spell(player, card, inst->targets[0].player, inst->targets[0].card);
	  cancel = 0;
	}

  return rval;
}

int card_taigams_strike(int player, int card, event_t event)
{
  /* Taigam's Strike	|3|U	0x200da77
   * Sorcery
   * Target creature gets +2/+0 until end of turn and can't be blocked this turn.
   * Rebound */

  int rval = vanilla_instant_pump(player, card, event, ANYBODY, player, 2,0, VANILLA_PUMP_DONT_KILL_CARD,SP_KEYWORD_UNBLOCKABLE);
  if (rval && event == EVENT_RESOLVE_SPELL)
	alternate_legacy_text(2, player, rebound(player, card));

  return rval;
}

/* Updraft Elemental	|2|U	=>vanilla
 * Creature - Elemental 1/4
 * Flying */

int card_void_squall(int player, int card, event_t event)
{
  /* Void Squall	|4|U	0x200da7c
   * Sorcery
   * Return target nonland permanent to its owner's hand.
   * Rebound */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.illegal_type = TYPE_LAND;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  bounce_permanent(inst->targets[0].player, inst->targets[0].card);
		  rebound(player, card);
		}
	  else
		kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_NONLAND_PERMANENT", 1, NULL);
}

int card_youthful_scholar(int player, int card, event_t event)
{
  /* Youthful Scholar	|3|U	0x200da81
   * Creature - Human Wizard 2/2
   * When ~ dies, draw two cards. */

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	draw_cards(player, 2);

  return 0;
}

int card_zephyr_scribe(int player, int card, event_t event)
{
  /* Zephyr Scribe	|2|U	0x200da86
   * Creature - Human Monk 2/1
   * |U, |T: Draw a card, then discard a card.
   * Whenever you cast a noncreature spell, untap ~. */

  if (prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	untap_card(player, card);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  draw_a_card(player);
	  discard(player, 0, player);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_U(1), 0, NULL, NULL);
}

/*** Black ***/

int card_acid_spewer_dragon(int player, int card, event_t event)
{
  /* Acid-Spewer Dragon	|5|B	0x200d9e1
   * Creature - Dragon 3/3
   * Flying, deathtouch
   * Megamorph |5|B|B
   * When ~ is turned face up, put a +1/+1 counter on each other Dragon creature you control. */

  deathtouch(player, card, event);

  if (event == EVENT_TURNED_FACE_UP)
	put_1_1_on_each_other_dragon_creature_you_control(player, card);

  return megamorph(player, card, event, MANACOST_XB(5,2));
}

int card_ambuscade_shaman(int player, int card, event_t event)
{
  /* Ambuscade Shaman	|2|B	0x200da8b
   * Creature - Orc Shaman 2/2
   * Whenever ~ or another creature enters the battlefield under your control, that creature gets +2/+2 until end of turn.
   * Dash |3|B */

  if (specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0))
	alternate_legacy_text(1, player, pump_until_eot(player, card, trigger_cause_controller, trigger_cause, 2,2));

  dash(player, card, event, MANACOST_XB(3,1));
  return 0;
}

int card_blood_chin_fanatic(int player, int card, event_t event)
{
  /* Blood-Chin Fanatic	|1|B|B	0x200da90
   * Creature - Orc Warrior 3/3
   * |1|B, Sacrifice another Warrior creature: Target player loses X life and you gain X life, where X is the sacrificed creature's power. */

  if (!IS_ACTIVATING(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.allow_cancel = 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select another Warrior creature to sacrifice.");
  test.subtype = SUBTYPE_WARRIOR;
  test.not_me = 1;

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_CAN_ACTIVATE)
	return (can_use_activated_abilities(player, card)
			&& has_mana_for_activated_ability(player, card, MANACOST_XB(1,1))
			&& can_target(&td)
			&& new_can_sacrifice_as_cost(player, card, &test));

  if (event == EVENT_ACTIVATE)
	{
	  int sac;
	  if (charge_mana_for_activated_ability(player, card, MANACOST_XB(1,1))
		  && (sac = new_sacrifice(player, card, player, SAC_RETURN_CHOICE, &test))
		  && pick_target(&td, "TARGET_PLAYER"))
		{
		  inst->number_of_targets = 1;
		  inst->info_slot = get_power(BYTE2(sac), BYTE3(sac));
		}
	  else
		cancel = 1;
	}

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  lose_life(inst->targets[0].player, inst->info_slot);
	  gain_life(player, inst->info_slot);
	}

  return 0;
}

int card_blood_chin_rager(int player, int card, event_t event)
{
  /* Blood-Chin Rager	|1|B	0x200da95
   * Creature - Human Warrior 2/2
   * Whenever ~ attacks, Warrior creatures you control gain menace until end of turn. */

  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_WARRIOR;
	  add_legacy_effect_to_all(player, card, &fx_menace, player, &test);
	}

  return 0;
}

int card_butchers_glee(int player, int card, event_t event)
{
  /* Butcher's Glee	|2|B	0x200da9a
   * Instant
   * Target creature gets +3/+0 and gains lifelink until end of turn. Regenerate it. */
  return vanilla_instant_pump(player, card, event, ANYBODY, player, 3,0, VANILLA_PUMP_REGENERATE,SP_KEYWORD_LIFELINK);
}

int card_coat_with_venom(int player, int card, event_t event)
{
  /* Coat with Venom	|B	0x200da9f
   * Instant
   * Target creature gets +1/+2 and gains deathtouch until end of turn. */
  return vanilla_instant_pump(player, card, event, ANYBODY, player, 1,2, 0,SP_KEYWORD_DEATHTOUCH);
}

int card_corpseweft(int player, int card, event_t event)
{
  /* Corpseweft	|2|B	0x200e201
   * Enchantment
   * |1|B, Exile one or more creature cards from your graveyard: Put an X/X |Sblack Zombie Horror creature token onto the battlefield tapped, where X is twice the number of cards exiled this way. */

  if (!IS_GAA_EVENT(event))
	return global_enchantment(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_ZOMBIE_HORROR, &token);
	  token.pow = token.tou = 2 * get_card_instance(player, card)->info_slot;
	  token.action = TOKEN_ACTION_TAPPED;
	  generate_token(&token);
	}

  int rval = generic_activated_ability(player, card, event, 0, MANACOST_XB(1,1), 0, NULL, NULL);

  if (event == EVENT_CAN_ACTIVATE && rval && !any_in_graveyard_by_type(player, TYPE_CREATURE))
	return 0;

  if (event == EVENT_ACTIVATE && cancel != 1)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");

	  int exiled = 0;
	  while (new_global_tutor(player, player, TUTOR_FROM_GRAVE_NOTARGET, TUTOR_RFG,
							  (IS_AI(player) && exiled == 0) ? 1 : 0,	// prevent AI from cancelling
							  AI_MIN_VALUE, &test) != -1)
		++exiled;

	  if (exiled == 0)
		cancel = 1;
	  else
		get_card_instance(player, card)->info_slot = exiled;
	}

  return rval;
}

int card_damnable_pact(int player, int card, event_t event)
{
  /* Damnable Pact	|X|B|B	0x200daa4
   * Sorcery
   * Target player draws X cards and loses X life. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.preferred_controller = ANYBODY;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  draw_cards(inst->targets[0].player, inst->info_slot);
		  lose_life(inst->targets[0].player, inst->info_slot);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_X_SPELL, &td, "TARGET_PLAYER", 1, NULL);
}

int card_deadly_wanderings(int player, int card, event_t event)
{
  /* Deadly Wanderings	|3|B|B	0x200daa9
   * Enchantment
   * As long as you control exactly one creature, that creature gets +2/+0 and has deathtouch and lifelink. */

  if ((event == EVENT_POWER || event == EVENT_ABILITIES)
	  && affected_card_controller == player
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && count_permanents_by_type(player, TYPE_CREATURE) == 1
	  && in_play(player, card) && !is_humiliated(player, card))
	{
	  if (event == EVENT_POWER)
		event_result += 2;
	  else	// EVENT_ABILITIES
		{
		  deathtouch(affected_card_controller, affected_card, event);
		  lifelink(affected_card_controller, affected_card, event);
		}
	}

  return global_enchantment(player, card, event);
}

/* Death Wind	|X|B	=>avacyn_restored.c:card_death_wind
 * Instant
 * Target creature gets -X/-X until end of turn. */

int card_deathbringer_regent(int player, int card, event_t event)
{
  /* Deathbringer Regent	|5|B|B	0x200daae
   * Creature - Dragon 5/6
   * Flying
   * When ~ enters the battlefield, if you cast it from your hand and there are five or more other creatures on the battlefield, destroy all other creatures. */

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	get_card_instance(player, card)->info_slot = !not_played_from_hand(player, card) ? CARD_ID_DEATHBRINGER_REGENT : 0;

  if (comes_into_play(player, card, event)
	  && get_card_instance(player, card)->info_slot == CARD_ID_DEATHBRINGER_REGENT)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;
	  if (check_battlefield_for_special_card(player, card, ANYBODY, CBFSC_GET_COUNT, &test) >= 5)
		new_manipulate_all(player, card, ANYBODY, &test, ACT_KILL_DESTROY);
	}

  return 0;
}

int card_defeat(int player, int card, event_t event)
{
  /* Defeat	|1|B	0x200dab3
   * Sorcery
   * Destroy target creature with power 2 or less. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.power_requirement = 2| TARGET_PT_LESSER_OR_EQUAL;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_LITERAL_PROMPT, &td, "Select target creature with power 2 or less.", 1, NULL);
}

/* Duress	|B	=>urza_saga.c:card_duress
 * Sorcery
 * Target opponent reveals his or her hand. You choose a noncreature, nonland card from it. That player discards that card. */

int card_dutiful_attendant(int player, int card, event_t event)
{
  /* Dutiful Attendant	|2|B	0x200dab8
   * Creature - Human Warrior 1/2
   * When ~ dies, return another target creature card from your graveyard to your hand. */

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select another target creature card.");
	  tutor_from_grave_to_hand_except_for_dying_card(player, card, 1, AI_MAX_CMC, &test);
	}

  return 0;
}

/* Flatten	|3|B	=>scars_of_mirrodin.c:card_grasp_of_darkness
 * Instant
 * Target creature gets -4/-4 until end of turn. */

int card_foul_renewal(int player, int card, event_t event)
{
  /* Foul Renewal	|3|B	0x200dabd
   * Instant
   * Return target creature card from your graveyard to your hand. Target creature gets -X/-X until end of turn, where X is the toughness of the card returned this way. */
  /* 2/25/2015	You must target a creature card in your graveyard and a creature on the battlefield to cast Foul Renewal. If the creature becomes an illegal
   * target before Foul Renewal resolves, you'll still return the creature card to your hand. If the creature card in your graveyard becomes an illegal target,
   * the creature on the battlefield will be unaffected as there is no "card returned this way." */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select target creature card.");

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_CAN_CAST)
	return count_graveyard_by_type(player, TYPE_CREATURE) > 0 && !graveyard_has_shroud(player) && can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &test, 1) != -1)
		{
		  pick_target(&td, "TARGET_CREATURE");
		  inst->info_slot = get_base_toughness_iid(player, get_grave(player)[inst->targets[1].player]);
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int selected = validate_target_from_grave_source(player, card, player, 1);
	  if (selected != -1)
		{
		  from_grave_to_hand(player, selected, TUTOR_HAND);
		  if (valid_target(&td))	// Per the ruling
			{
			  card_instance_t* inst = get_card_instance(player, card);
			  int x = -inst->info_slot;
			  pump_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, x, x);
			}
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_foul_tongue_invocation(int player, int card, event_t event)
{
  /* Foul-Tongue Invocation	|2|B	0x200d9b4
   * Instant
   * As an additional cost to cast ~, you may reveal a Dragon card from your hand.
   * Target player sacrifices a creature. If you revealed a Dragon card or controlled a Dragon as you cast ~, you gain 4 life. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  int revealed = revealed_or_controlled_dragon(player, card, event);	// must come first
  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  impose_sacrifice(player, card, inst->targets[0].player, 1, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0);
		  if (revealed)
			gain_life(player, 4);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_foul_tongue_shriek(int player, int card, event_t event)
{
  /* Foul-Tongue Shriek	|B	0x200dac2
   * Instant
   * Target opponent loses 1 life for each attacking creature you control. You gain that much life. */

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
		  int attackers = count_attackers(player);
		  lose_life(get_card_instance(player, card)->targets[0].player, attackers);
		  gain_life(player, attackers);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_OPPONENT", 1, NULL);
}

int bone_harvest_impl(int player, int card, event_t event, int draw_else_slowtrip);
int card_gravepurge(int player, int card, event_t event)
{
  /* Gravepurge	|2|B
   * Instant
   * Put any number of target creature cards from your graveyard on top of your library.
   * Draw a card. */
  return bone_harvest_impl(player, card, event, 1);
}

/* Hand of Silumgar	|1|B	=>m10.c:card_deadly_recluse
 * Creature - Human Warrior 2/1
 * Deathtouch */

/* Hedonist's Trove	|5|B|B	0x000000
 * Enchantment
 * When ~ enters the battlefield, exile all cards from target opponent's graveyard.
 * You may play land cards exiled with ~.
 * You may cast nonland cards exiled with ~. You can't cast more than one spell this way each turn. */

/* Kolaghan Skirmisher	|1|B	=>fate_reforged.c:card_aleshas_vanguard
 * Creature - Human Warrior 2/2
 * Dash |2|B */

int card_marang_river_skeleton(int player, int card, event_t event)
{
  /* Marang River Skeleton	|1|B	0x200d8d8
   * Creature - Skeleton 1/1
   * |B: Regenerate ~.
   * Megamorph |3|B */

  if (IS_GAA_EVENT(event))
	return regeneration(player, card, event, MANACOST_B(1));

  return megamorph(player, card, event, MANACOST_XB(3,1));
}

int card_marsh_hulk(int player, int card, event_t event)
{
  /* Marsh Hulk	|4|B|B	0x200d8dd
   * Creature - Zombie Ogre 4/6
   * Megamorph |6|B */
  return megamorph(player, card, event, MANACOST_XB(6,1));
}

/* Mind Rot	|2|B	=>portal_1_2_3k.c:card_mind_rot
 * Sorcery
 * Target player discards two cards. */

int card_minister_of_pain(int player, int card, event_t event)
{
  /* Minister of Pain	|2|B	0x200dac7
   * Creature - Human Shaman 2/3
   * Exploit
   * When ~ exploits a creature, creatures your opponents control get -1/-1 until end of turn. */

  if (exploit(player, card, event))
	pump_creatures_until_eot(player, card, 1-player, 0, -1,-1, 0,0, NULL);

  return 0;
}

int card_pitiless_horde(int player, int card, event_t event)
{
  /* Pitiless Horde	|2|B	0x200d8f1
   * Creature - Orc Berserker 5/3
   * At the beginning of your upkeep, you lose 2 life.
   * Dash |2|B|B */

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	lose_life(player, 2);

  dash(player, card, event, MANACOST_XB(2,2));
  return 0;
}

int card_qarsi_sadist(int player, int card, event_t event)
{
  /* Qarsi Sadist	|1|B	0x200dacc
   * Creature - Human Cleric 1/3
   * Exploit
   * When ~ exploits a creature, target opponent loses 2 life and you gain 2 life. */

  if (exploit(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, 0);
	  td.zone = TARGET_ZONE_PLAYERS;
	  if (would_validate_arbitrary_target(&td, 1-player, -1))
		{
		  lose_life(1-player, 2);
		  gain_life(player, 2);
		}
	}

  return 0;
}

int card_rakshasa_gravecaller(int player, int card, event_t event)
{
  /* Rakshasa Gravecaller	|4|B	0x200dad1
   * Creature - Cat Demon 3/6
   * Exploit
   * When ~ exploits a creature, put two 2/2 |Sblack Zombie creature tokens onto the battlefield. */

  if (exploit(player, card, event))
	generate_tokens_by_id(player, card, CARD_ID_ZOMBIE, 2);

  return 0;
}

int card_reckless_imp(int player, int card, event_t event)
{
  /* Reckless Imp	|2|B	0x200d8f6
   * Creature - Imp 2/2
   * Flying
   * ~ can't block.
   * Dash |1|B */

  cannot_block(player, card, event);
  dash(player, card, event, MANACOST_XB(1,1));
  return 0;
}

/* Risen Executioner	|2|B|B	0x000000
 * Creature - Zombie Warrior 4/3
 * ~ can't block.
 * Other Zombie creatures you control get +1/+1.
 * You may cast ~ from your graveyard if you pay |1 more to cast it for each other creature card in your graveyard. */

int card_self_inflicted_wound(int player, int card, event_t event)
{
  /* Self-Inflicted Wound	|1|B	0x200dad6
   * Sorcery
   * Target opponent sacrifices a |Sgreen or |Swhite creature. If that player does, he or she loses 2 life. */

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
		  new_default_test_definition(&test, TYPE_CREATURE,
									  get_sleighted_color_text2(player, card, "Select a %s or %s creature to sacrifice.", COLOR_GREEN, COLOR_WHITE));
		  test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN|COLOR_TEST_WHITE);

		  int tgt = get_card_instance(player, card)->targets[0].player;

		  if (new_sacrifice(player, card, tgt, SAC_CAUSED|SAC_NO_CANCEL, &test))
			lose_life(tgt, 2);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_OPPONENT", 1, NULL);
}

int card_shambling_goblin(int player, int card, event_t event)
{
  /* Shambling Goblin	|B	0x200dadb
   * Creature - Zombie Goblin 1/1
   * When ~ dies, target creature an opponent controls gets -1/-1 until end of turn. */

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  alternate_legacy_text(2, player, pump_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, -1,-1));
		}
	}

  return 0;
}

int card_sibsig_icebreakers(int player, int card, event_t event)
{
  /* Sibsig Icebreakers	|2|B	0x200dae0
   * Creature - Zombie 2/3
   * When ~ enters the battlefield, each player discards a card. */

  if (comes_into_play(player, card, event))
	APNAP(p, discard(p, 0, player));

  return 0;
}

int card_sidisi_undead_vizier(int player, int card, event_t event)
{
  /* Sidisi, Undead Vizier	|3|B|B	0x200dae5
   * Legendary Creature - Zombie Naga 4/6
   * Deathtouch
   * Exploit
   * When ~ exploits a creature, you may search your library for a card, put it into your hand, then shuffle your library. */

  check_legend_rule(player, card, event);
  deathtouch(player, card, event);

  if (exploit(player, card, event)
	  && (duh_mode(player)
		  || DIALOG(player, card, EVENT_ACTIVATE,
					DLG_NO_CANCEL,
					"Search library", 1, 1,
					"Decline", 1, 0) == 1))
	{
	  test_definition_t test;
	  default_test_definition(&test, TYPE_ANY);
	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &test);
	}

  return 0;
}

int card_silumgar_assassin(int player, int card, event_t event)
{
  /* Silumgar Assassin	|1|B	0x200daea
   * Creature - Human Assassin 2/1
   * Creatures with power greater than ~'s power can't block it.
   * Megamorph |2|B
   * When ~ is turned face up, destroy target creature with power 3 or less an opponent controls. */

  if (event == EVENT_BLOCK_LEGALITY && attacking_card == card && attacking_card_controller == player
	  && get_power(affected_card_controller, affected_card) > get_power(player, card)
	  && !is_humiliated(player, card))
	event_result = 1;

  if (event == EVENT_TURNED_FACE_UP)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.power_requirement = 3 | TARGET_PT_LESSER_OR_EQUAL;
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && select_target(player, card, &td, "Select target creature with power 3 or less an opponent controls.", NULL))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		}
	}

  return megamorph(player, card, event, MANACOST_XB(2,1));
}

int card_silumgar_butcher(int player, int card, event_t event)
{
  /* Silumgar Butcher	|4|B	0x200daef
   * Creature - Zombie Djinn 3/3
   * Exploit
   * When ~ exploits a creature, target creature gets -3/-3 until end of turn. */

  if (exploit(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  pump_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, -3,-3);
		  inst->number_of_targets = 0;
		}
	}

  return 0;
}

/* Ukud Cobra	|3|B	=>m10.c:card_deadly_recluse
 * Creature - Snake 2/5
 * Deathtouch */

/* Ultimate Price	|1|B	=>return_to_ravnica.c:card_ultimate_price
 * Instant
 * Destroy target monocolored creature. */

int card_virulent_plague(int player, int card, event_t event)
{
  /* Virulent Plague	|2|B	0x200daf4
   * Enchantment
   * Creature tokens get -2/-2. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && is_token(affected_card_controller, affected_card)
	  && in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card))
	event_result -= 2;

  return global_enchantment(player, card, event);
}

int card_vulturous_aven(int player, int card, event_t event)
{
  /* Vulturous Aven	|3|B	0x200daf9
   * Creature - Bird Shaman 2/3
   * Flying
   * Exploit
   * When ~ exploits a creature, you draw two cards and you lose 2 life. */

  if (exploit(player, card, event))
	{
	  draw_cards(player, 2);
	  lose_life(player, 2);
	}

  return 0;
}

/* Wandering Tombshell	|3|B	=>vanilla
 * Creature - Zombie Turtle 1/6 */

/*** Red ***/

int card_atarka_efreet(int player, int card, event_t event)
{
  /* Atarka Efreet	|3|R	0x200dafe
   * Creature - Efreet Shaman 5/1
   * Megamorph |2|R
   * When ~ is turned face up, it deals 1 damage to target creature or player. */

  if (event == EVENT_TURNED_FACE_UP)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
		{
		  get_card_instance(player, card)->number_of_targets = 0;
		  damage_target0(player, card, 1);
		}
	}

  return megamorph(player, card, event, MANACOST_XR(2,1));
}

int card_atarka_pummeler(int player, int card, event_t event)
{
  /* Atarka Pummeler	|4|R	0x200d90f
   * Creature - Ogre Warrior 4/5
   * Formidable - |3|R|R: Creatures you control gain menace until end of turn. Activate this ability only if creatures you control have total power 8 or greater. */

  if (event == EVENT_CAN_ACTIVATE)
	{
	  if (!formidable(player))
		return 0;
	  if (player == AI && IS_AI(player) && current_turn != player)
		return 0;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  add_legacy_effect_to_all(player, card, &fx_menace, player, &test);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_XR(3,2), 0, NULL, NULL);
}

int card_berserkes_onslaught(int player, int card, event_t event)
{
  /* Berserkers' Onslaught	|3|R|R	0x200db03
   * Enchantment
   * Attacking creatures you control have double strike. */

  card_instance_t* aff;
  if (event == EVENT_ABILITIES && affected_card_controller == player
	  && (aff = in_play(affected_card_controller, affected_card))
	  && aff->state & STATE_ATTACKING
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && in_play(player, card) && !is_humiliated(player, card))
	event_result |= KEYWORD_DOUBLE_STRIKE;

  return global_enchantment(player, card, event);
}

int card_commune_with_lava(int player, int card, event_t event)
{
  /* Commune with Lava	|X|R|R	0x200e279
   * Instant
   * Exile the top X cards of your library. Until the end of your next turn, you may play those cards. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int num = MIN(count_deck(player), inst->info_slot);
	  if (num > 0)
		{
		  // Irritatingly, have to exile first, then create play-from-exile effects
		  int iids[num];
		  memcpy(iids, deck_ptr[player], sizeof(iids));
		  rfg_top_n_cards_of_deck(player, num);
		  int i;
		  for (i = 0; i < num; ++i)
			create_may_play_card_from_exile_effect(player, card, player, cards_data[iids[i]].id, MPCFE_UNTIL_END_OF_YOUR_NEXT_TURN);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

int card_crater_elemental(int player, int card, event_t event)
{
  /* Crater Elemental	|2|R	0x200db08
   * Creature - Elemental 0/6
   * |R, |T, Sacrifice ~: ~ deals 4 damage to target creature.
   * Formidable - |2|R: ~ has base power 8 until end of turn. Activate this ability only if creatures you control have total power 8 or greater. */

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);

	  int can_damage = can_target(&td) && can_sacrifice_this_as_cost(player, card) && CAN_TAP(player, card);

	  enum
	  {
		CHOICE_DAMAGE = 1,
		CHOICE_BASE_POW
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"4 damage",		can_damage,			1,	DLG_MANA(MANACOST_R(1)),	DLG_TARGET(&td, "TARGET_CREATURE"),
						"Base power 8",	formidable(player),	1,	DLG_MANA(MANACOST_XR(2,1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_DAMAGE:
			  tap_card(player, card);
			  kill_card(player, card, KILL_SACRIFICE);
			  break;

			case CHOICE_BASE_POW:
			  break;
		  }
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_DAMAGE:
			  if (valid_target(&td))
				damage_target0(player, card, 4);
			  break;

			case CHOICE_BASE_POW:
			  ;card_instance_t* inst = get_card_instance(player, card);
			  if (in_play(inst->parent_controller, inst->parent_card))
				set_pt_and_abilities_until_eot(player, card, inst->parent_controller, inst->parent_card, 8,-1, 0,0, 0);
			  break;
		  }
	}

  return 0;
}

int card_descent_of_the_dragons(int player, int card, event_t event)
{
  /* Descent of the Dragons	|4|R|R	0x200e206
   * Sorcery
   * Destroy any number of target creatures. For each creature destroyed this way, its controller puts a 4/4 |Sred Dragon creature token with flying onto the battlefield. */

  if (event == EVENT_RESOLVE_GENERAL_EFFECT)
	{
	  card_instance_t* inst = get_card_instance(player, card);

      int num0 = LOWORD(inst->targets[10].player);	// 18 in Shandalar
      int num1 = HIWORD(inst->targets[10].player);
      if (num0 > 0)
		{
		  token_generation_t token;
		  default_token_definition(player, card, CARD_ID_DRAGON, &token);
		  token.pow = token.tou = 4;
		  token.qty = num0;
		  token.t_player = 0;
		  generate_token(&token);
		}
      if (num1 > 0)
		{
		  token_generation_t token;
		  default_token_definition(player, card, CARD_ID_DRAGON, &token);
		  token.pow = token.tou = 4;
		  token.qty = num1;
		  token.t_player = 1;
		  generate_token(&token);
		}
      return 0;
	}

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = ANYBODY;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  card_instance_t* leg = NULL;
	  int i;
	  for (i = 0; i < inst->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  {
			leg = legacy_permanents_destroyed_this_way_accumulate(player, card, leg, inst->targets[i].player, inst->targets[i].card);
			kill_card(inst->targets[i].player, inst->targets[i].card, KILL_DESTROY);
		  }

	  kill_card(player, card, KILL_DESTROY);
	}

  // "Any number" is "at most 11" because now not even non-permanents can use their entire targets array.  Nice going.
  return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 11, NULL);
}

int card_draconic_roar(int player, int card, event_t event)
{
  /* Draconic Roar	|1|R	0x200d9b9
   * Instant
   * As an additional cost to cast ~, you may reveal a Dragon card from your hand.
   * ~ deals 3 damage to target creature. If you revealed a Dragon card or controlled a Dragon as you cast ~, ~ deals 3 damage to that creature's controller. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  int revealed = revealed_or_controlled_dragon(player, card, event);	// must come first
  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  damage_target0(player, card, 3);
		  if (revealed)
			damage_player(get_card_instance(player, card)->targets[0].player, 3, player, card);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/* Dragon Fodder	|1|R	=>shards_of_alara.c:card_dragon_fodder
 * Sorcery
 * Put two 1/1 |Sred Goblin creature tokens onto the battlefield. */

int card_dragon_tempest(int player, int card, event_t event)
{
  /* Dragon Tempest	|1|R	0x200db0d
   * Enchantment
   * Whenever a creature with flying enters the battlefield under your control, it gains haste until end of turn.
   * Whenever a Dragon enters the battlefield under your control, it deals X damage to target creature or player, where X is the number of Dragons you control. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == player
	  && !check_for_cip_effects_removal(trigger_cause_controller, trigger_cause) && !is_humiliated(player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	  int flying = (is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE)
					&& check_for_ability(trigger_cause_controller, trigger_cause, KEYWORD_FLYING));
	  int dragon = has_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_DRAGON) && can_target(&td);

	  if (!flying && !dragon)
		return 0;

	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  if (flying)
			pump_ability_until_eot(player, card, trigger_cause_controller, trigger_cause, 0,0, 0,SP_KEYWORD_HASTE);
		  if (dragon && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
			{
			  damage_target0(player, card, count_subtype(player, TYPE_PERMANENT, SUBTYPE_DRAGON));
			  get_card_instance(player, card)->number_of_targets = 0;
			}
		  else
			cancel = 0;
		}
	}

  return global_enchantment(player, card, event);
}

int card_dragon_whisperer(int player, int card, event_t event)
{
  /* Dragon Whisperer	|R|R	0x200d914
   * Creature - Human Shaman 2/2
   * |R: ~ gains flying until end of turn.
   * |1|R: ~ gets +1/+0 until end of turn.
   * Formidable - |4|R|R: Put a 4/4 |Sred Dragon creature token with flying onto the battlefield. Activate this ability only if creatures you control have total power 8 or greater. */

  if (event == EVENT_POW_BOOST)
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_XR(1,1), 1,0);

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  enum
	  {
		CHOICE_FLYING = 1,
		CHOICE_POW,
		CHOICE_DRAGON
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Flying",		1,	check_for_ability(player, card, KEYWORD_FLYING) ? 2 : 0,	DLG_MANA(MANACOST_R(1)),
						"+1/+0",		1,	1,															DLG_MANA(MANACOST_XR(1,1)),
						"Dragon token",	formidable(player),	4,											DLG_MANA(MANACOST_XR(4,2)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  switch (choice)
			{
			  case CHOICE_FLYING:
				if (in_play(inst->parent_controller, inst->parent_card))
				  alternate_legacy_text(1, player, pump_ability_until_eot(player, card, inst->parent_controller, inst->parent_card, 0,0, KEYWORD_FLYING,0));
				break;

			  case CHOICE_POW:
				if (in_play(inst->parent_controller, inst->parent_card))
				  alternate_legacy_text(2, player, pump_until_eot_merge_previous(player, card, inst->parent_controller, inst->parent_card, 1,0));
				break;

			  case CHOICE_DRAGON:
                ;token_generation_t token;
                default_token_definition(player, card, CARD_ID_DRAGON, &token);
                token.pow = token.tou = 4;
                generate_token(&token);
				break;
		  }
		}
	}

  return 0;
}

int card_dragonlords_servant(int player, int card, event_t event)
{
  /* Dragonlord's Servant	|1|R	0x200db12
   * Creature - Goblin Shaman 1/3
   * Dragon spells you cast cost |1 less to cast. */

  if (event == EVENT_MODIFY_COST_GLOBAL
	  && (affected_card_controller == player
		  || (affected_card_controller == -1 && event_result == player))
	  && has_subtype(affected_card_controller, affected_card, SUBTYPE_DRAGON)
	  && !is_humiliated(player, card))
	COST_COLORLESS -= 1;

  return 0;
}

/* Hardened Berserker	|2|R	0x000000
 * Creature - Human Berserker 3/2
 * Whenever ~ attacks, the next spell you cast this turn costs |1 less to cast. */

int card_impact_tremors(int player, int card, event_t event)
{
  /* Impact Tremors	|1|R	0x200db17
   * Enchantment
   * Whenever a creature enters the battlefield under your control, ~ deals 1 damage to each opponent. */

  if (specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0))
	damage_player(1-player, 1, player, card);

  return global_enchantment(player, card, event);
}

int card_ire_shaman(int player, int card, event_t event)
{
  /* Ire Shaman	|1|R	0x200e27e
   * Creature - Orc Shaman 2/1
   * Menace
   * Megamorph |R
   * When ~ is turned face up, exile the top card of your library. Until end of turn, you may play that card. */

  menace(player, card, event);

  int iid;
  if (event == EVENT_TURNED_FACE_UP && (iid = deck_ptr[player][0]) != -1)
	{
	  rfg_top_card_of_deck(player);
	  create_may_play_card_from_exile_effect(player, card, player, cards_data[iid].id, MPCFE_UNTIL_EOT);
	}
  return megamorph(player, card, event, MANACOST_R(1));
}

/* Kindled Fury	|R	=>m13.c:card_kindled_fury
 * Instant
 * Target creature gets +1/+0 and gains first strike until end of turn. */

/* Kolaghan Aspirant	|1|R	=>avacyn_restored.c:card_somberwald_vigilante
 * Creature - Human Warrior 2/1
 * Whenever ~ becomes blocked by a creature, ~ deals 1 damage to that creature. */

int card_kolaghan_forerunners(int player, int card, event_t event)
{
  /* Kolaghan Forerunners	|2|R	0x200d8fb
   * Creature - Human Berserker 100/3
   * Trample
   * ~'s power is equal to the number of creatures you control.
   * Dash |2|R */

  if (event == EVENT_POWER && affect_me(player, card) && !is_humiliated(player, card) && player != -1)
	event_result += creature_count[player];

  dash(player, card, event, MANACOST_XR(2,1));
  return 0;
}

int card_kolaghan_stormsinger(int player, int card, event_t event)
{
  /* Kolaghan Stormsinger	|R	0x200db1c
   * Creature - Human Shaman 1/1
   * Haste
   * Megamorph |R
   * When ~ is turned face up, target creature gains haste until end of turn. */

  haste(player, card, event);

  if (event == EVENT_TURNED_FACE_UP)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 0,0, 0,SP_KEYWORD_HASTE);
		}
	}

  return megamorph(player, card, event, MANACOST_R(1));
}

int card_lightning_berserker(int player, int card, event_t event)
{
  /* Lightning Berserker	|R	0x200d900
   * Creature - Human Berserker 1/1
   * |R: ~ gets +1/+0 until end of turn.
   * Dash |R */

  dash(player, card, event, MANACOST_R(1));
  return generic_shade_merge_pt(player, card, event, 0, MANACOST_R(1), 1,0);
}

static int fx_haste_and_menace(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (affect_me(inst->damage_target_player, inst->damage_target_card))
		haste(inst->damage_target_player, inst->damage_target_card, event);
	}
  return fx_menace(player, card, event);
}
int card_lose_calm(int player, int card, event_t event)
{
  /* Lose Calm	|3|R	0x200db21
   * Sorcery
   * Gain control of target creature until end of turn. Untap that creature. It gains haste and menace until end of turn. */

  // This contains special-casing in aot_legacy() to add menace.

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);

		  if (inst->targets[0].player != player)
			effect_act_of_treason(player, card, inst->targets[0].player, inst->targets[0].card);
		  else
			{
			  create_targetted_legacy_effect(player, card, &fx_haste_and_menace, inst->targets[0].player, inst->targets[0].card);
			  untap_card(inst->targets[0].player, inst->targets[0].card);
			}
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/* Magmatic Chasm	|1|R	=>urza_saga.c:card_falter
 * Sorcery
 * Creatures without flying can't block this turn. */

/* Qal Sisma Behemoth	|2|R	0x000000
 * Creature - Ogre Warrior 5/5
 * ~ can't attack or block unless you pay |2. */

int card_rending_volley(int player, int card, event_t event)
{
  /* Rending Volley	|R	0x200db26
   * Instant
   * ~ can't be countered by spells or abilities.
   * ~ deals 4 damage to target |Swhite or |Sblue creature. */

  cannot_be_countered(player, card, event);

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE|COLOR_TEST_BLUE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, 4);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_LITERAL_PROMPT, &td,
					   get_sleighted_color_text2(player, card, "Select target %s or %s creature.", COLOR_WHITE, COLOR_BLUE),
					   1, NULL);
}

int card_roast(int player, int card, event_t event)
{
  /* Roast	|1|R	0x200d941
   * Sorcery
   * ~ deals 5 damage to target creature without flying. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.illegal_abilities |= KEYWORD_FLYING;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, 5);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "FLOOD", 1, NULL);	// "Select target creature without flying."
}

int card_sabertooth_outrider(int player, int card, event_t event)
{
  /* Sabertooth Outrider	|3|R	0x200d919
   * Creature - Human Warrior 4/2
   * Trample
   * Formidable - Whenever ~ attacks, if creatures you control have total power 8 or greater, ~ gains first strike until end of turn. */

  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && formidable(player)
	  && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	pump_ability_until_eot(player, card, player, card, 0,0, KEYWORD_FIRST_STRIKE,0);

  return 0;
}

int card_sarkhans_rage(int player, int card, event_t event)
{
  /* Sarkhan's Rage	|4|R	0x200d91e
   * Instant
   * ~ deals 5 damage to target creature or player. If you control no Dragons, ~ deals 2 damage to you. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  damage_target0(player, card, 5);
		  if (!check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DRAGON))
			damage_player(player, 2, player, card);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_sarkhans_triumph(int player, int card, event_t event)
{
  /* Sarkhan's Triumph	|2|R	0x200d923
   * Instant
   * Search your library for a Dragon creature card, reveal it, put it into your hand, then shuffle your library. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, TYPE_CREATURE,MATCH, SUBTYPE_DRAGON,MATCH, 0,0, 0,0, -1,0);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Screamreach Brawler	|2|R	=>fate_reforged.c:card_mardu_scout
 * Creature - Orc Berserker 2/3
 * Dash |1|R */

int card_seismic_rupture(int player, int card, event_t event)
{
  /* Seismic Rupture	|2|R	0x200db2b
   * Sorcery
   * ~ deals 2 damage to each creature without flying. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.keyword = KEYWORD_FLYING;
	  test.keyword_flag = DOESNT_MATCH;
	  new_damage_all(player, card, ANYBODY, 2, 0, &test);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_sprinting_warbrute(int player, int card, event_t event)
{
  /* Sprinting Warbrute	|4|R	0x200d905
   * Creature - Ogre Berserker 5/4
   * ~ attacks each turn if able.
   * Dash |3|R */

  attack_if_able(player, card, event);
  dash(player, card, event, MANACOST_XR(3,1));
  return 0;
}

int card_stormcrag_elemental(int player, int card, event_t event)
{
  /* Stormcrag Elemental	|5|R	0x200d8e2
   * Creature - Elemental 5/5
   * Trample
   * Megamorph |4|R|R */
  return megamorph(player, card, event, MANACOST_XR(4,2));
}

int card_stormwing_dragon(int player, int card, event_t event)
{
  /* Stormwing Dragon	|5|R	0x200d9e6
   * Creature - Dragon 3/3
   * Flying, first strike
   * Megamorph |5|R|R
   * When ~ is turned face up, put a +1/+1 counter on each other Dragon creature you control. */

  if (event == EVENT_TURNED_FACE_UP)
	put_1_1_on_each_other_dragon_creature_you_control(player, card);

  return megamorph(player, card, event, MANACOST_XR(5,2));
}

/* Summit Prowler	|2|R|R	=>vanilla
 * Creature - Yeti 4/3 */

int card_tail_slash(int player, int card, event_t event)
{
  /* Tail Slash	|2|R	0x200db30
   * Instant
   * Target creature you control deals damage equal to its power to target creature you don't control. */
  // 2/25/2015	If either creature is an illegal target as Tail Slash tries to resolve, the creature you control won't deal damage.

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
		   && pick_next_target_noload(&td_dont_control, "Select target creature you don't control.")))
	get_card_instance(player, card)->number_of_targets = 0;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  // See ruling.  (And note that the spell isn't actually countered unless both are invalid.)
	  if (valid_target(&td_control) && validate_target(player, card, &td_dont_control, 1))
		damage_creature(inst->targets[1].player, inst->targets[1].card,
						get_power(inst->targets[0].player, inst->targets[0].card),
						inst->targets[0].player, inst->targets[0].card);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/* Thunderbreak Regent	|2|R|R	0x000000
 * Creature - Dragon 4/4
 * Flying
 * Whenever a Dragon you control becomes the target of a spell or ability an opponent controls, ~ deals 3 damage to that player. */

/* Tormenting Voice	|1|R	=>khans_of_tarkir.c:card_tormenting_voice
 * Sorcery
 * As an additional cost to cast ~, discard a card.
 * Draw two cards. */

/* Twin Bolt	|1|R	=>rise_of_the_eldrazi.c:card_forked_bolt
 * Instant
 * ~ deals 2 damage divided as you choose among one or two target creatures and/or players. */

int card_vandalize(int player, int card, event_t event)
{
  /* Vandalize	|4|R	0x200db35
   * Sorcery
   * Choose one or both -
   * * Destroy target artifact.
   * * Destroy target land. */

  if (IS_CASTING(player, card, event))
	{
	  target_definition_t td_artifact;
	  default_target_definition(player, card, &td_artifact, TYPE_ARTIFACT);

	  target_definition_t td_land;
	  default_target_definition(player, card, &td_land, TYPE_LAND);

	  enum
	  {
		CHOICE_ARTIFACT = 1,
		CHOICE_LAND,
		CHOICE_BOTH
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Destroy artifact",	1, 3, DLG_TARGET(&td_artifact, "TARGET_ARTIFACT"),
						"Destroy land",		1, 2, DLG_TARGET(&td_land, "TARGET_LAND"),
						"Both",				can_target(&td_artifact) && can_target(&td_land), 7);

	  if (event == EVENT_CAN_CAST)
		return choice;
	  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
		{
		  if (choice == CHOICE_BOTH
			  && pick_target(&td_artifact, "TARGET_ARTIFACT"))
			new_pick_target(&td_land, "TARGET_LAND", 1, 1);
		  // otherwise done by DIALOG()
		}
	  else if (event == EVENT_RESOLVE_SPELL)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  switch (choice)
			{
			  case CHOICE_ARTIFACT:
			  case CHOICE_LAND:
				kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
				break;

			  case CHOICE_BOTH:
				if (valid_target(&td_artifact))
				  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
				if (validate_target(player, card, &td_land, 1))
				  kill_card(inst->targets[1].player, inst->targets[1].card, KILL_DESTROY);
				break;
			}
		  kill_card(player, card, KILL_DESTROY);
		}
	}

  return 0;
}

int card_volcanic_rush(int player, int card, event_t event)
{
  /* Volcanic Rush	|4|R	0x200db3a
   * Instant
   * Attacking creatures get +2/+0 and gain trample until end of turn. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  pump_creatures_until_eot(player, card, current_turn, 0, 2,0, KEYWORD_TRAMPLE,0, &test);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_volcanic_vision(int player, int card, event_t event)
{
  /* Volcanic Vision	|5|R|R	0x200db3f
   * Sorcery
   * Return target instant or sorcery card from your graveyard to your hand. ~ deals damage equal to that card's converted mana cost to each creature your opponents control. Exile ~. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_INSTANT|TYPE_SORCERY, "Select target instant or sorcery card.");

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int selected = validate_target_from_grave_source(player, card, player, 0);
	  if (selected != -1)
		{
		  int cmc = get_cmc_by_internal_id(get_grave(player)[selected]);
		  from_grave_to_hand(player, selected, TUTOR_HAND);

		  if (cmc > 0)
			new_damage_all(player, card, 1-player, cmc, NDA_ALL_CREATURES, NULL);

		  kill_card(player, card, KILL_REMOVE);
		}
	  else
		kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &test);
}

int card_warbringer(int player, int card, event_t event)
{
  /* Warbringer	|3|R	0x200db44
   * Creature - Orc Berserker 3/3
   * Dash costs you pay cost |2 less.
   * Dash |2|R */

  // First ability is hardcoded in dash().
  dash(player, card, event, MANACOST_XR(2,1));
  return 0;
}

int card_zurgo_bellstriker(int player, int card, event_t event)
{
  /* Zurgo Bellstriker	|R	0x200d90a
   * Legendary Creature - Orc Warrior 2/2
   * ~ can't block creatures with power 2 or greater.
   * Dash |1|R */

  check_legend_rule(player, card, event);

  if (event == EVENT_BLOCK_LEGALITY && affect_me(player, card) && !is_humiliated(player, card)
	  && get_power(attacking_card_controller, attacking_card) >= 2)
	event_result = 1;

  dash(player, card, event, MANACOST_XR(1,1));
  return 0;
}

/*** Green ***/

int card_aerie_bowmasters(int player, int card, event_t event)
{
  /* Aerie Bowmasters	|2|G|G	0x200d8e7
   * Creature - Hound Archer 3/4
   * Reach
   * Megamorph |5|G */
  return megamorph(player, card, event, MANACOST_XG(5,1));
}

int card_ainok_artillerist(int player, int card, event_t event)
{
  /* Ainok Artillerist	|2|G	0x2001006
   * Creature - Hound Archer 4/1
   * ~ has reach as long as it has a +1/+1 counter on it. */

  if (event == EVENT_ABILITIES && affect_me(player, card)
	  && count_1_1_counters(player, card) > 0 && !is_humiliated(player, card))
	event_result |= KEYWORD_REACH;

  return 0;
}

int card_ainok_survivalist(int player, int card, event_t event)
{
  /* Ainok Survivalist	|1|G	0x200100b
   * Creature - Hound Shaman 2/1
   * Megamorph |1|G
   * When ~ is turned face up, destroy target artifact or enchantment an opponent controls. */

  if (event == EVENT_TURNED_FACE_UP)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_ARTIFACT|TYPE_ENCHANTMENT);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && select_target(player, card, &td, "Select target artifact or enchantment an opponent controls.", NULL))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		}
	}

  return megamorph(player, card, event, MANACOST_XG(1,1));
}

int card_assault_formation(int player, int card, event_t event)
{
  /* Assault Formation	|1|G	0x2001010
   * Enchantment
   * Each creature you control assigns combat damage equal to its toughness rather than its power.
   * |G: Target creature with defender can attack this turn as though it didn't have defender.
   * |2|G: Creatures you control get +0/+1 until end of turn. */

  // First ability is hardcoded in get_attack_power().

  if (event == EVENT_CHECK_PUMP)
	pumpable_toughness[player] += generic_shade_amt_can_pump(player, card, 1, 0, MANACOST_XG(2,1), -1);

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.required_abilities = KEYWORD_DEFENDER;

	  enum
	  {
		CHOICE_DEFENDER = 1,
		CHOICE_PUMP
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Defender can attack",	1, 1,	DLG_MANA(MANACOST_G(1)),	DLG_LITERAL_TARGET(&td, "Select target creature with defender."),
						"+0/+1",				1, 2,	DLG_MANA(MANACOST_XG(2,1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  switch (choice)
			{
			  case CHOICE_DEFENDER:
				alternate_legacy_text(1, player,
									  create_targetted_legacy_effect(inst->parent_controller, inst->parent_card, &effect_defender_can_attack_until_eot,
																	 inst->targets[0].player, inst->targets[0].card));
				break;

			  case CHOICE_PUMP:
				pump_creatures_until_eot_merge_pt_alternate_legacy_text(player, card, player, 0,1, NULL, 2);
				break;
			}
		}
	}

  return global_enchantment(player, card, event);
}

int card_atarka_beastbreaker(int player, int card, event_t event)
{
  /* Atarka Beastbreaker	|1|G	0x200d928
   * Creature - Human Warrior 2/2
   * Formidable - |4|G: ~ gets +4/+4 until end of turn. Activate this ability only if creatures you control have total power 8 or greater. */

  if ((event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST || event == EVENT_CAN_ACTIVATE)
	  && !formidable(player))
	return 0;
  return generic_shade_merge_pt(player, card, event, 0, MANACOST_XG(4,1), 4,4);
}

int card_avatar_of_the_resolute(int player, int card, event_t event)
{
  /* Avatar of the Resolute	|G|G	0x2001015
   * Creature - Avatar 3/2
   * Reach, trample
   * ~ enters the battlefield with a +1/+1 counter on it for each other creature you control with a +1/+1 counter on it. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int c, n = 0;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && is_what(player, c, TYPE_CREATURE) && count_1_1_counters(player, c) > 0 && c != card)
		  ++n;
	  if (n > 0)
		{
		  ++hack_silent_counters;
		  add_1_1_counters(player, card, n);
		  --hack_silent_counters;
		}
	}

  return 0;
}

int card_circle_of_elders(int player, int card, event_t event)
{
  /* Circle of Elders	|2|G|G	0x200d92d
   * Creature - Human Shaman 2/4
   * Vigilance
   * Formidable - |T: Add |3 to your mana pool. Activate this ability only if creatures you control have total power 8 or greater. */

  vigilance(player, card, event);

  if ((event == EVENT_CAN_ACTIVATE || (event == EVENT_COUNT_MANA && affect_me(player, card)))
	  && !formidable(player))
	return 0;

  return mana_producing_creature(player, card, event, 0, COLOR_COLORLESS, 3);
}

int card_collected_company(int player, int card, event_t event)
{
  /* Collected Company	|3|G	0x200e20b
   * Instant
   * Look at the top six cards of your library. Put up to two creature cards with converted mana cost 3 or less from among them onto the battlefield. Put the rest on the bottom of your library in any order. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int chosen_iid[2] = {-1, -1};
	  int chosen_pos[2] = {-1, -1};

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card with converted mana cost 3 or less.");
	  test.cmc = 4;
	  test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

	  int depth = MIN(6, count_deck(player));
	  if (depth == 0)
		return 0;

	  int i;
	  for (i = 0; i < 2; ++i)
		{
		  int selected = select_card_from_zone(player, player, deck_ptr[player], depth, 0, AI_MAX_VALUE, -1, &test);
		  if (selected == -1)
			break;

		  chosen_iid[i] = deck_ptr[player][selected];
		  chosen_pos[i] = selected;
		  remove_card_from_deck(player, selected);
		  --depth;
		}

	  put_top_x_on_bottom(player, player, depth);

	  if (chosen_iid[0] >= 0)	// any chosen?
		{
		  if (check_battlefield_for_id(2, CARD_ID_GRAFDIGGERS_CAGE))	// leave them on top of library in original order
			{
			  if (chosen_iid[1] >= 0)
				{
				  if (chosen_pos[1] < chosen_pos[0])
					SWAP(chosen_iid[0], chosen_iid[1]);
				  put_iid_under_the_first_x_cards_of_library(player, chosen_iid[1], 0);
				}
			  put_iid_under_the_first_x_cards_of_library(player, chosen_iid[0], 0);
			  show_deck(HUMAN, deck_ptr[player], chosen_iid[1] >= 0 ? 2 : 1, "Revealed", 0, 0x7375b0);
			}
		  else
			{
			  put_into_play(player, add_card_to_hand(player, chosen_iid[0]));
			  if (chosen_iid[1] >= 0)
				put_into_play(player, add_card_to_hand(player, chosen_iid[1]));
			}
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Colossodon Yearling	|2|G	=>vanilla
 * Creature - Beast 2/4 */

/* Conifer Strider	|3|G	=>m12.c:card_aven_fleetwing
 * Creature - Elemental 5/1
 * Hexproof */

int card_deathmist_raptor(int player, int card, event_t event)
{
  /* Deathmist Raptor	|1|G|G	0x200e21a
   * Creature - Lizard Beast 3/3
   * Deathtouch
   * Whenever a permanent you control is turned face up, you may return ~ from your graveyard to the battlefield face up or face down.
   * Megamorph |4|G */

  deathtouch(player, card, event);
  // Second ability hacked into the horrible pseudotrigger in onslaught.c:check_for_turned_face_up_card_interactions()
  return megamorph(player, card, event, MANACOST_XG(4,1));
}

int card_den_protector(int player, int card, event_t event)
{
  /* Den Protector	|1|G	0x200101a
   * Creature - Human Warrior 2/1
   * Creatures with power less than ~'s power can't block it.
   * Megamorph |1|G
   * When ~ is turned face up, return target card from your graveyard to your hand. */

  if (event == EVENT_BLOCK_LEGALITY && attacking_card == card && attacking_card_controller == player
	  && get_power(affected_card_controller, affected_card) < get_power(player, card)
	  && !is_humiliated(player, card))
	event_result = 1;

  if (event == EVENT_TURNED_FACE_UP)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ANY, "Select target card.");
	  if (get_grave(player)[0] != -1 && !graveyard_has_shroud(player))
		new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &test);
	}

  return megamorph(player, card, event, MANACOST_XG(1,1));
}

/* Display of Dominance	|1|G	0x000000
 * Instant
 * Choose one -
 * * Destroy target |Sblue or |Sblack noncreature permanent.
 * * Permanents you control can't be the targets of |Sblue or |Sblack spells your opponents control this turn. */

int card_dragon_scarred_bear(int player, int card, event_t event)
{
  /* Dragon-Scarred Bear	|2|G	0x200d932
   * Creature - Bear 3/2
   * Formidable - |1|G: Regenerate ~. Activate this ability only if creatures you control have total power 8 or greater. */

  if (event == EVENT_CAN_ACTIVATE && !formidable(player))
	return 0;

  return regeneration(player, card, event, MANACOST_XG(1,1));
}

int card_dromokas_gift(int player, int card, event_t event)
{
  /* Dromoka's Gift	|4|G	0x200101f
   * Instant
   * Bolster 4. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  bolster(player, card, 4);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_epic_confrontation(int player, int card, event_t event)
{
  /* Epic Confrontation	|1|G	0x2001024
   * Sorcery
   * Target creature you control gets +1/+2 until end of turn. It fights target creature you don't control. */
  /* 2/25/2015	If the creature you control is an illegal target as Epic Confrontation tries to resolve, the creature you control won't get +1/+2. If that
   * creature is a legal target but the creature you don't control isn't, your creature will still get +1/+2. In both cases, neither creature will deal or be
   * dealt damage. */

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
	  // See ruling.  (And note that the spell isn't actually countered unless both are invalid.)
	  if (valid_target(&td_control))
		{
		  pump_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 1,2);

		  if (validate_target(player, card, &td_dont_control, 1))
			fight(inst->targets[0].player, inst->targets[0].card, inst->targets[1].player, inst->targets[1].card);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/* Explosive Vegetation	|3|G	=>onslaught.c:card_explosive_vegetation
 * Sorcery
 * Search your library for up to two basic land cards and put them onto the battlefield tapped. Then shuffle your library. */

static int fx_add_2_p1p1_at_eot(int player, int card, event_t event)
{
  if (eot_trigger(player, card, event))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  add_1_1_counters(inst->damage_target_player, inst->damage_target_card, 2);
	  kill_card(player, card, KILL_REMOVE);
	}
  return 0;
}
int card_foe_razer_regent(int player, int card, event_t event)
{
  /* Foe-Razer Regent	|5|G|G	0x2001029
   * Creature - Dragon 4/5
   * Flying
   * When ~ enters the battlefield, you may have it fight target creature you don't control.
   * Whenever a creature you control fights, put two +1/+1 counters on it at the beginning of the next end step. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  if (can_target(&td) && new_pick_target(&td, "Select target creature you don't control.", 0, GS_LITERAL_PROMPT))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  fight(player, card, inst->targets[0].player, inst->targets[0].card);
		}
	}

  if (event == EVENT_FIGHT && affected_card_controller == player && !is_humiliated(player, card))
	create_targetted_legacy_effect(player, card, &fx_add_2_p1p1_at_eot, affected_card_controller, affected_card);

  return 0;
}

int card_glade_watcher(int player, int card, event_t event)
{
  /* Glade Watcher	|1|G	0x200d937
   * Creature - Elemental 3/3
   * Defender
   * Formidable - |G: ~ can attack this turn as though it didn't have defender. Activate this ability only if creatures you control have total power 8 or greater. */

  if (event == EVENT_CAN_ACTIVATE && !formidable(player))
	return 0;

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card))
		create_targetted_legacy_effect(player, card, &effect_defender_can_attack_until_eot, inst->parent_controller, inst->parent_card);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_G(1), 0, NULL, NULL);
}

int card_guardian_shield_bearer(int player, int card, event_t event)
{
  /* Guardian Shield-Bearer	|1|G	0x200102e
   * Creature - Human Soldier 2/1
   * Megamorph |3|G
   * When ~ is turned face up, put a +1/+1 counter on another target creature you control. */

  if (event == EVENT_TURNED_FACE_UP)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.special = TARGET_SPECIAL_NOT_ME;
	  td.allowed_controller = player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_ANOTHER_CREATURE_CONTROL"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  add_1_1_counter(inst->targets[0].player, inst->targets[0].card);
		}
	}

  return megamorph(player, card, event, MANACOST_XG(3,1));
}

int card_herdchaser_dragon(int player, int card, event_t event)
{
  /* Herdchaser Dragon	|5|G	0x200d9eb
   * Creature - Dragon 3/3
   * Flying, trample
   * Megamorph |5|G|G
   * When ~ is turned face up, put a +1/+1 counter on each other Dragon creature you control. */

  if (event == EVENT_TURNED_FACE_UP)
	put_1_1_on_each_other_dragon_creature_you_control(player, card);

  return megamorph(player, card, event, MANACOST_XG(5,2));
}

int card_inspiring_call(int player, int card, event_t event)
{
  /* Inspiring Call	|2|G	0x20010b5
   * Instant
   * Draw a card for each creature you control with a +1/+1 counter on it. Those creatures gain indestructible until end of turn. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int c, n = 0;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && is_what(player, c, TYPE_CREATURE) && count_1_1_counters(player, c) > 0)
		  {
			pump_ability_until_eot(player, card, player, c, 0,0, 0,SP_KEYWORD_INDESTRUCTIBLE);
			++n;
		  }

	  draw_cards(player, n);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_lurking_arynx(int player, int card, event_t event)
{
  /* Lurking Arynx	|4|G	0x20010ba
   * Creature - Cat Beast 3/5
   * Formidable - |2|G: Target creature blocks ~ this turn if able. Activate this ability only if creatures you control have total power 8 or greater. */

  if (!IS_GAA_EVENT(event))
	return 0;

  if (event == EVENT_CAN_ACTIVATE && !formidable(player))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  target_must_block_me(player, card, inst->targets[0].player, inst->targets[0].card, 1);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XG(2,1), 0, &td, "TARGET_CREATURE");
}

/* Naturalize	|1|G	=>unlimited.c:card_disenchant
 * Instant
 * Destroy target artifact or enchantment. */

int card_obscuring_aether(int player, int card, event_t event)
{
  /* Obscuring AEther	|G	0x20010bf
   * Enchantment
   * Face-down creature spells you cast cost |1 less to cast.
   * |1|G: Turn ~ face down. */

  // First ability hardcoded in true_get_updated_casting_cost(); no need for a MODIFY_COST_GLOBAL handler.

  if (event == EVENT_SHOULD_AI_PLAY || event == EVENT_CAN_CAST)
	return global_enchantment(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  turn_face_down(inst->parent_controller, inst->parent_card);
	}

  can_exist_while_face_down(player, card, event);

  return generic_activated_ability(player, card, event, 0, MANACOST_XG(1,1), 0, NULL, NULL);
}

int card_pinion_feast(int player, int card, event_t event)
{
  /* Pinion Feast	|4|G	0x20010c4
   * Instant
   * Destroy target creature with flying. Bolster 2. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_abilities = KEYWORD_FLYING;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		  bolster(player, card, 2);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_FLYING", 1, NULL);
}

int card_press_the_advantage(int player, int card, event_t event)
{
  /* Press the Advantage	|2|G|G	0x20010c9
   * Instant
   * Up to two target creatures each get +2/+2 and gain trample until end of turn. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int i;
	  for (i = 0; i < inst->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  pump_ability_until_eot(player, card, inst->targets[i].player, inst->targets[i].card, 2,2, KEYWORD_TRAMPLE,0);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

// Can always look at opponent's face-down creatures - see http://www.slightlymagic.net/forum/viewtopic.php?t=13200
/* Revealing Wind	|2|G	=>unlimited.c:card_fog
 * Instant
 * Prevent all combat damage that would be dealt this turn. You may look at each face-down creature that's attacking or blocking. */

int card_salt_road_ambushers(int player, int card, event_t event)
{
  /* Salt Road Ambushers	|3|G	0x200e215
   * Creature - Hound Warrior 3/3
   * Whenever another permanent you control is turned face up, if it's a creature, put two +1/+1 counters on it.
   * Megamorph |3|G|G */

  // First ability hacked into the horrible pseudotrigger in onslaught.c:check_for_turned_face_up_card_interactions()
  return megamorph(player, card, event, MANACOST_XG(3,2));
}

int card_salt_road_quartermasters(int player, int card, event_t event)
{
  /* Salt Road Quartermasters	|2|G	0x20010d8
   * Creature - Human Soldier 1/1
   * ~ enters the battlefield with two +1/+1 counters on it.
   * |2|G, Remove a +1/+1 counter from ~: Put a +1/+1 counter on target creature. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  add_1_1_counter(inst->targets[0].player, inst->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_1_1_COUNTER, MANACOST_XG(2,1), 0, &td, "TARGET_CREATURE");
}

int card_sandsteppe_scavenger(int player, int card, event_t event)
{
  /* Sandsteppe Scavenger	|4|G	0x200db49
   * Creature - Hound Scout 2/2
   * When ~ enters the battlefield, bolster 2. */

  if (comes_into_play(player, card, event))
	bolster(player, card, 2);

  return 0;
}

int card_scaleguard_sentinels(int player, int card, event_t event)
{
  /* Scaleguard Sentinels	|G|G	0x200d9be
   * Creature - Human Soldier 2/3
   * As an additional cost to cast ~, you may reveal a Dragon card from your hand.
   * ~ enters the battlefield with a +1/+1 counter on it if you revealed a Dragon card or controlled a Dragon as you cast ~. */

  if (revealed_or_controlled_dragon(player, card, event))	// must come first
	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 1);
  return 0;
}

int card_segmented_krotiq(int player, int card, event_t event)
{
  /* Segmented Krotiq	|5|G	0x200d8ec
   * Creature - Insect 6/5
   * Megamorph |6|G */
  return megamorph(player, card, event, MANACOST_XG(6,1));
}

int card_servant_of_the_scale(int player, int card, event_t event)
{
  /* Servant of the Scale	|G	0x200db4e
   * Creature - Human Soldier 0/0
   * ~ enters the battlefield with a +1/+1 counter on it.
   * When ~ dies, put X +1/+1 counters on target creature you control, where X is the number of +1/+1 counters on ~. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 1);

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "ASHNODS_BATTLEGEAR"))	// "Select target creature you control"
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  add_1_1_counters(inst->targets[0].player, inst->targets[0].card, count_1_1_counters(player, card));
		}
	}

  return 0;
}

/* Shaman of Forgotten Ways	|2|G	0x000000
 * Creature - Human Shaman 2/3
 * |T: Add two mana in any combination of colors to your mana pool. Spend this mana only to cast creature spells.
 * Formidable - |9|G|G, |T: Each player's life total becomes the number of creatures he or she controls. Activate this ability only if creatures you control have total power 8 or greater. */

int card_shape_the_sands(int player, int card, event_t event)
{
  /* Shape the Sands	|G	0x200db53
   * Instant
   * Target creature gets +0/+5 and gains reach until end of turn. */
  return vanilla_instant_pump(player, card, event, ANYBODY, player, 0,5, KEYWORD_REACH,0);
}

int enchanted_land_has_T_add_n_mana_of_any_one_color(int player, int card, event_t event, int amt);

int card_sheltered_aerie(int player, int card, event_t event)
{
  /* Sheltered Aerie	|2|G	0x200db58
   * Enchantment - Aura
   * Enchant land
   * Enchanted land has "|T: Add two mana of any one color to your mana pool." */
  return enchanted_land_has_T_add_n_mana_of_any_one_color(player, card, event, 2);
}

int card_sight_of_the_scalelords(int player, int card, event_t event)
{
  /* Sight of the Scalelords	|4|G	0x200db5d
   * Enchantment
   * At the beginning of combat on your turn, creatures you control with toughness 4 or greater get +2/+2 and gain vigilance until end of turn. */

  // This is a horrid hack, but it works, even if an attack isn't explicitly declared
  if (event == EVENT_MUST_ATTACK && current_turn == player && !is_humiliated(player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.toughness = 3;
	  test.toughness_flag = F5_TOUGHNESS_GREATER_THAN_VALUE;

	  pump_creatures_until_eot(player, card, player, 0, 2,2, 0,SP_KEYWORD_VIGILANCE, &test);
	}

  return global_enchantment(player, card, event);
}

int card_stampeding_elk_herd(int player, int card, event_t event)
{
  /* Stampeding Elk Herd	|3|G|G	0x200d93c
   * Creature - Elk 5/5
   * Formidable - Whenever ~ attacks, if creatures you control have total power 8 or greater, creatures you control gain trample until end of turn. */

  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && formidable(player)
	  && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	pump_creatures_until_eot(player, card, player, 0, 0,0, KEYWORD_TRAMPLE,0, NULL);

  return 0;
}

int card_sunbringers_touch(int player, int card, event_t event)
{
  /* Sunbringer's Touch	|2|G|G	0x200db62
   * Sorcery
   * Bolster X, where X is the number of cards in your hand. Each creature you control with a +1/+1 counter on it gains trample until end of turn. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int c, h = hand_count[player];
	  if (h > 0)
		bolster(player, card, h);

	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && is_what(player, c, TYPE_CREATURE) && count_1_1_counters(player, c) > 0)
		  pump_ability_until_eot(player, card, player, c, 0,0, KEYWORD_TRAMPLE,0);

	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_surrak_the_hunt_caller(int player, int card, event_t event)
{
  /* Surrak, the Hunt Caller	|2|G|G	0x200db67
   * Legendary Creature - Human Warrior 5/4
   * Formidable - At the beginning of combat on your turn, if creatures you control have total power 8 or greater, target creature you control gains haste until end of turn. */

  check_legend_rule(player, card, event);

  // This is a horrid hack, but it works, even if an attack isn't explicitly declared
  if (event == EVENT_MUST_ATTACK && current_turn == player && formidable(player) && !is_humiliated(player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = td.preferred_controller = player;

	  if (can_target(&td) && pick_target(&td, "ASHNODS_BATTLEGEAR"))	// "Select target creature you control"
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 0,0, 0,SP_KEYWORD_HASTE);
		}
	}

  return 0;
}

int card_tread_upon(int player, int card, event_t event)
{
  /* Tread Upon	|1|G	0x200db6c
   * Instant
   * Target creature gets +2/+2 and gains trample until end of turn. */
  return vanilla_instant_pump(player, card, event, ANYBODY, player, 2,2, KEYWORD_TRAMPLE,0);
}

/*** Multi ***/

int card_arashin_sovereign(int player, int card, event_t event)
{
  /* Arashin Sovereign	|5|G|W	0x200db71
   * Creature - Dragon 6/6
   * Flying
   * When ~ dies, you may put it on the top or bottom of its owner's library. */

  int owner, position, pos;
  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player))
	  && find_in_owners_graveyard(player, card, &owner, &position)
	  && (pos = DIALOG(player, card, EVENT_ACTIVATE,
					   DLG_RANDOM, DLG_NO_STORAGE,
					   "Put on top",	1, 2,
					   "Put on bottom",	1, 1)))
	{
	  int iid = get_grave(owner)[position];
	  obliterate_card_in_grave(owner, position);
	  if (pos == 1)
		put_iid_under_the_first_x_cards_of_library(owner, iid, 0);
	  else
		put_iid_under_the_first_x_cards_of_library(owner, iid, 500);
	}

  return 0;
}

static int effect_no_effect_until_eot(int player, int card, event_t event)
{
  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);
  return 0;
}
int card_atarkas_command(int player, int card, event_t event)
{
  /* Atarka's Command	|R|G	0x200db76
   * Instant
   * Choose two -
   * * Your opponents can't gain life this turn.
   * * ~ deals 3 damage to each opponent.
   * * You may put a land card from your hand onto the battlefield.
   * * Creatures you control get +1/+1 and gain reach until end of turn. */

  if (!IS_CASTING(player, card, event))
	return 0;

  typedef enum
  {
	CHOICE_NO_GAIN_LIFE = 1,
	CHOICE_DAMAGE,
	CHOICE_LAND,
	CHOICE_PUMP
  } CommandChoices;

  test_definition_t test_land;
  new_default_test_definition(&test_land, TYPE_LAND, "Select a land card.");
  test_land.zone = TARGET_ZONE_HAND;

  int priority[5];
  if (!IS_AI(player) || event == EVENT_RESOLVE_SPELL)
	priority[CHOICE_NO_GAIN_LIFE] = priority[CHOICE_DAMAGE] = priority[CHOICE_LAND] = priority[CHOICE_PUMP] = 1;
  else
	{
	  priority[CHOICE_NO_GAIN_LIFE] = 1;
	  priority[CHOICE_DAMAGE] = 2;
	  priority[CHOICE_LAND] = check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test_land) ? 3 : -1;
	  priority[CHOICE_PUMP] = creature_count[player];
	}

  CommandChoices choices[2];
  choices[0] = DIALOG(player, card, event,
					  DLG_CHOOSE_TWO(&choices[1]), DLG_SORT_RESULTS, DLG_RANDOM,
					  "Can't gain life",		1, priority[CHOICE_NO_GAIN_LIFE],
					  "Deal 3 damage",			1, priority[CHOICE_DAMAGE],
					  "Land onto battlefield",	1, priority[CHOICE_LAND],
					  "+1/+1 and reach",		1, priority[CHOICE_PUMP]);

  if (event == EVENT_CAN_CAST)
	return choices[0];
  else if (event == EVENT_RESOLVE_SPELL)
	{
	  int choice;
	  for (choice = 0; choice < 2; ++choice)
		switch (choices[choice])
		  {
			case CHOICE_NO_GAIN_LIFE:
			  /* Appallingly, this isn't mediated by event in Manalink, but by a search on csvid.  So it's not any more appalling that I'm making that
			   * distinguish between Atarka's Command's different effect card types by looking at which text they display. */
			  alternate_legacy_text(1, player, create_legacy_effect(player, card, &effect_no_effect_until_eot));
			  break;

			case CHOICE_DAMAGE:
			  damage_player(1-player, 3, player, card);
			  break;

			case CHOICE_LAND:
			  new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &test_land);
			  break;

			case CHOICE_PUMP:
			  pump_creatures_until_eot(player, card, player, 2, 1,1, KEYWORD_REACH,0, NULL);
			  break;
		  }
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_boltwing_marauder(int player, int card, event_t event)
{
  /* Boltwing Marauder	|3|B|R	0x200db7b
   * Creature - Dragon 5/4
   * Flying
   * Whenever another creature enters the battlefield under your control, target creature gets +2/+0 until end of turn. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		{
		  target_definition_t td;
		  default_target_definition(player, card, &td, TYPE_CREATURE);
		  td.preferred_controller = player;
		  td.allow_cancel = 0;

		  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
			{
			  card_instance_t* inst = get_card_instance(player, card);
			  inst->number_of_targets = 0;
			  pump_until_eot_merge_previous(player, card, inst->targets[0].player, inst->targets[0].card, 2,0);
			}
		}
	}

  return 0;
}

int card_cunning_breezedancer(int player, int card, event_t event)
{
  /* Cunning Breezedancer	|4|W|U	0x200db80
   * Creature - Dragon 4/4
   * Flying
   * Whenever you cast a noncreature spell, ~ gets +2/+2 until end of turn. */

  if (prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	pump_until_eot_merge_previous(player, card, player, card, 2,2);

  return 0;
}

int card_dragonlord_atarka(int player, int card, event_t event)
{
  /* Dragonlord Atarka	|5|R|G	0x200db85
   * Legendary Creature - Elder Dragon 8/8
   * Flying, trample
   * When ~ enters the battlefield, it deals 5 damage divided as you choose among any number of target creatures and/or planeswalkers your opponents control. */

  check_legend_rule(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE|TARGET_TYPE_PLANESWALKER);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  target_and_divide_damage(player, card, &td, "Select target creature or planeswalker an opponent controls.", 5);
	}

  return 0;
}

int card_dragonlord_dromoka(int player, int card, event_t event)
{
  /* Dragonlord Dromoka	|4|G|W	0x200db8a
   * Legendary Creature - Elder Dragon 5/7
   * ~ can't be countered.
   * Flying, lifelink
   * Your opponents can't cast spells during your turn. */

  if (event == EVENT_MODIFY_COST_GLOBAL)
	{
	  if ((affected_card_controller == -1 ? event_result : affected_card_controller) == 1-player
		  && current_turn == player
		  && !is_what(affected_card_controller, affected_card, TYPE_LAND) && !is_humiliated(player, card))
		infinite_casting_cost();
	  return 0;
	}

  check_legend_rule(player, card, event);
  cannot_be_countered(player, card, event);
  lifelink(player, card, event);

  return 0;
}

int card_dragonlord_kolaghan(int player, int card, event_t event)
{
  /* Dragonlord Kolaghan	|4|B|R	0x200db8f
   * Legendary Creature - Elder Dragon 6/5
   * Flying, haste
   * Other creatures you control have haste.
   * Whenever an opponent casts a creature or planeswalker spell with the same name as a card in his or her graveyard, that player loses 10 life. */

  check_legend_rule(player, card, event);
  haste(player, card, event);
  boost_subtype(player, card, event, -1, 0,0, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY);

  if (specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE|TARGET_TYPE_PLANESWALKER,MATCH, 0,0, 0,0, 0,0, -1,0))
	{
	  // get_card_instance(trigger_cause_controller, trigger_cause)->internal_card_id might not be the canonical one.  This always is.
	  int iid = get_internal_card_id_from_csv_id(get_id(trigger_cause_controller, trigger_cause));

	  const int* gy = get_grave(1-player);
	  int i;
	  for (i = 0; i < 500 && gy[i] != -1; ++i)
		if (gy[i] == iid)
		  {
			lose_life(1-player, 10);
			break;
		  }
	}

  return 0;
}

int card_dragonlord_ojutai(int player, int card, event_t event)
{
  /* Dragonlord Ojutai	|3|W|U	0x200db94
   * Legendary Creature - Elder Dragon 5/4
   * Flying
   * ~ has hexproof as long as it's untapped.
   * Whenever ~ deals combat damage to a player, look at the top three cards of your library. Put one of them into your hand and the rest on the bottom of your library in any order. */

  check_legend_rule(player, card, event);

  if (event == EVENT_ABILITIES && affect_me(player, card) && !is_tapped(player, card))
	hexproof(player, card, event);

  if (has_combat_damage_been_inflicted_to_a_player(player, card, event))
	reveal_top_cards_of_library_and_choose_type(player, card, player, 3, 1, TUTOR_HAND,0, TUTOR_BOTTOM_OF_DECK,0, TYPE_ANY);

  return 0;
}

int card_dragonlord_silumgar(int player, int card, event_t event)
{
  /* Dragonlord Silumgar	|4|U|B	0x200db99
   * Legendary Creature - Elder Dragon 3/5
   * Flying, deathtouch
   * When ~ enters the battlefield, gain control of target creature or planeswalker for as long as you control ~. */

  check_legend_rule(player, card, event);
  deathtouch(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE|TARGET_TYPE_PLANESWALKER);

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLANESWALKER"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  gain_control_until_source_is_in_play_and_tapped(player, card, inst->targets[0].player, inst->targets[0].card, GCUS_CONTROLLED);
		}
	}

  return 0;
}

int card_dromokas_command(int player, int card, event_t event)
{
  /* Dromoka's Command	|G|W	0x200db9e
   * Instant
   * Choose two -
   * * Prevent all damage target instant or sorcery spell would deal this turn.
   * * Target player sacrifices an enchantment.
   * * Put a +1/+1 counter on target creature.
   * * Target creature you control fights target creature you don't control. */

  if (!IS_CASTING(player, card, event))
	return 0;

  typedef enum
  {
	CHOICE_PREVENT = 1,
	CHOICE_SAC,
	CHOICE_COUNTER,
	CHOICE_FIGHT
  } CommandChoices;

  target_definition_t td_spell;
  base_target_definition(player, card, &td_spell, TYPE_INSTANT|TYPE_SORCERY);
  td_spell.zone = TARGET_ZONE_ON_STACK;

  target_definition_t td_player;
  default_target_definition(player, card, &td_player, 0);
  td_player.zone = TARGET_ZONE_PLAYERS;

  target_definition_t td_creature;
  default_target_definition(player, card, &td_creature, TYPE_CREATURE);
  target_definition_t td_control = td_creature;	// struct copy
  target_definition_t td_dont_control = td_creature;	// struct copy;

  td_creature.preferred_controller = player;
  td_control.allowed_controller = td_control.preferred_controller = player;
  td_dont_control.allowed_controller = td_control.preferred_controller = 1-player;

  int can[5], priority[5];
  if (event == EVENT_RESOLVE_SPELL)
	{
	  can[CHOICE_PREVENT] = can[CHOICE_SAC] = can[CHOICE_COUNTER] = can[CHOICE_FIGHT] = 1;
	  priority[CHOICE_PREVENT] = priority[CHOICE_SAC] = priority[CHOICE_COUNTER] = priority[CHOICE_FIGHT] = 1;
	}
  else
	{
	  can[CHOICE_PREVENT] = can_target(&td_spell);
	  can[CHOICE_SAC] = can_target(&td_player);
	  can[CHOICE_COUNTER] = can_target(&td_creature);
	  int can_target_controlled = can_target(&td_control);
	  can[CHOICE_FIGHT] = can_target_controlled && can_target(&td_dont_control);

	  if (!IS_AI(player))
		priority[CHOICE_PREVENT] = priority[CHOICE_SAC] = priority[CHOICE_COUNTER] = priority[CHOICE_FIGHT] = 1;
	  else
		{
		  priority[CHOICE_PREVENT] = can[CHOICE_PREVENT] ? 3 : -100;
		  if (!can[CHOICE_SAC])
			priority[CHOICE_SAC] = -100;
		  else
			{
			  priority[CHOICE_SAC] = 0;
			  int c;
			  for (c = 0; c < active_cards_count[1-player]; ++c)
				if (in_play(1-player, c) && is_what(1-player, c, TYPE_ENCHANTMENT))
				  {
					priority[CHOICE_SAC] = 6;
					break;
				  }
			}

		  priority[CHOICE_COUNTER] = !can[CHOICE_COUNTER] ? -100 : (can_target_controlled ? 5 : -2);
		  priority[CHOICE_FIGHT] = can[CHOICE_FIGHT] ? 4 : -100;

		  raise_command_singleton_ai_priorities(event, priority);
		}
	}

  card_instance_t* inst = get_card_instance(player, card);

  // have to do all the targeting manually, since DIALOG() can't handle that with DLG_CHOOSE_TWO
  int choice;
  CommandChoices choices[2];
  choices[0] = DIALOG(player, card, event,
					  DLG_CHOOSE_TWO(&choices[1]), DLG_SORT_RESULTS, DLG_RANDOM,
					  "Prevent damage",			can[CHOICE_PREVENT],	priority[CHOICE_PREVENT],
					  "Sacrifice enchantment",	can[CHOICE_SAC],		priority[CHOICE_SAC],
					  "+1/+1 counter",			can[CHOICE_COUNTER],	priority[CHOICE_COUNTER],
					  "Fight",					can[CHOICE_FIGHT],		priority[CHOICE_FIGHT]);

  if (event == EVENT_CAN_CAST)
	return choices[0];
  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  inst->number_of_targets = 0;
	  for (choice = 0; choice < 2; ++choice)
		switch (choices[choice])
		  {
			case CHOICE_PREVENT:
			  if (!pick_next_target_noload(&td_spell, "Select target instant or sorcery spell."))
				return 0;
			  break;

			case CHOICE_SAC:
			  if (!new_pick_target(&td_player, "TARGET_PLAYER", -1, 1))
				return 0;
			  break;

			case CHOICE_COUNTER:
			  if (!new_pick_target(&td_creature, "TARGET_CREATURE", -1, 1))
				return 0;
			  break;

			case CHOICE_FIGHT:
			  if (!new_pick_target(&td_control, "ASHNODS_BATTLEGEAR", -1, 1)	// "Select target creature you control."
				  || !pick_next_target_noload(&td_dont_control, "Select target creature you don't control."))
				return 0;
			  break;
		  }
	}
  else if (event == EVENT_RESOLVE_SPELL)
	{
	  // All choices are targeted, so we don't have to worry about the spell being countered because all targets chosen for other choices are invalid
	  int n = -1;
	  for (choice = 0; choice < 2; ++choice)
		switch (choices[choice])
		  {
			case CHOICE_PREVENT:
			  if (validate_target(player, card, &td_spell, ++n))
				negate_damage_this_turn(player, card, inst->targets[n].player, inst->targets[n].card, 0);
			  break;

			case CHOICE_SAC:
			  if (validate_target(player, card, &td_player, ++n))
				impose_sacrifice(player, card, inst->targets[n].player, 1, TYPE_ENCHANTMENT,MATCH, 0,0, 0,0, 0,0, -1,0);
			  break;

			case CHOICE_COUNTER:
			  if (validate_target(player, card, &td_creature, ++n))
				add_1_1_counter(inst->targets[n].player, inst->targets[n].card);
			  break;

			case CHOICE_FIGHT:
			  if (validate_target(player, card, &td_control, ++n)
				  && validate_target(player, card, &td_dont_control, ++n))
				fight(inst->targets[n-1].player, inst->targets[n-1].card,
					  inst->targets[n].player, inst->targets[n].card);
			  break;
		  }
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_enduring_scalelord(int player, int card, event_t event)
{
  /* Enduring Scalelord	|4|G|W	0x200dbad
   * Creature - Dragon 4/4
   * Flying
   * Whenever one or more +1/+1 counters are placed on another creature you control, you may put a +1/+1 counter on ~. */
  enable_xtrigger_flags |= ENABLE_XTRIGGER_1_1_COUNTERS;
  if (xtrigger_condition() == XTRIGGER_1_1_COUNTERS && affect_me(player, card) && player == reason_for_trigger_controller
	  && trigger_cause_controller == player && trigger_cause != card && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		{
		  if (!(IS_AI(player) && count_1_1_counters(player, card) >= 200))	// quite enough
			event_result |= RESOLVE_TRIGGER_AI(player);
		}
	  if (event == EVENT_RESOLVE_TRIGGER)
		add_1_1_counter(player, card);
	}

  return 0;
}

int card_harbinger_of_the_hunt(int player, int card, event_t event)
{
  /* Harbinger of the Hunt	|3|R|G	0x200dbb2
   * Creature - Dragon 5/3
   * Flying
   * |2|R: ~ deals 1 damage to each creature without flying.
   * |2|G: ~ deals 1 damage to each other creature with flying. */

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  enum
	  {
		CHOICE_WITHOUT_FLYING = 1,
		CHOICE_OTHERS_WITH_FLYING
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Without flying",		1, 1, DLG_MANA(MANACOST_XR(2,1)),
						"Others with flying",	1, 1, DLG_MANA(MANACOST_XG(2,1)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  test_definition_t test;
		  default_test_definition(&test, TYPE_CREATURE);
		  test.keyword = KEYWORD_FLYING;

		  switch (choice)
			{
			  case CHOICE_WITHOUT_FLYING:
				test.keyword_flag = DOESNT_MATCH;
				break;

			  case CHOICE_OTHERS_WITH_FLYING:
				test.keyword_flag = MATCH;
				test.not_me = 1;
				break;
			}
		  new_damage_all(player, card, ANYBODY, 1, 0, &test);
		}
	}

  return 0;
}

int card_kolaghans_command(int player, int card, event_t event)
{
  /* Kolaghan's Command	|1|B|R	0x200dbb7
   * Instant
   * Choose two -
   * * Return target creature card from your graveyard to your hand.
   * * Target player discards a card.
   * * Destroy target artifact.
   * * ~ deals 2 damage to target creature or player. */

  if (!IS_CASTING(player, card, event))
	return 0;

  typedef enum
  {
	CHOICE_RAISE_DEAD = 1,
	CHOICE_DISCARD,
	CHOICE_ARTIFACT,
	CHOICE_DAMAGE
  } CommandChoices;

  test_definition_t test_raise_dead;
  default_test_definition(&test_raise_dead, TYPE_CREATURE);

  target_definition_t td_player;
  default_target_definition(player, card, &td_player, 0);
  td_player.zone = TARGET_ZONE_PLAYERS;

  target_definition_t td_artifact;
  default_target_definition(player, card, &td_artifact, TYPE_ARTIFACT);

  target_definition_t td_creature_or_player;
  default_target_definition(player, card, &td_creature_or_player, TYPE_CREATURE);
  td_creature_or_player.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  int can[5], priority[5];
  if (event == EVENT_RESOLVE_SPELL)
	{
	  can[CHOICE_RAISE_DEAD] = can[CHOICE_DISCARD] = can[CHOICE_ARTIFACT] = can[CHOICE_DAMAGE] = 1;
	  priority[CHOICE_RAISE_DEAD] = priority[CHOICE_DISCARD] = priority[CHOICE_ARTIFACT] = priority[CHOICE_DAMAGE] = 1;
	}
  else
	{
	  can[CHOICE_RAISE_DEAD] = new_special_count_grave(player, &test_raise_dead) > 0 && !graveyard_has_shroud(player);
	  can[CHOICE_DISCARD] = can_target(&td_player);
	  can[CHOICE_ARTIFACT] = can_target(&td_artifact);
	  can[CHOICE_DAMAGE] = can_target(&td_creature_or_player);

	  if (!IS_AI(player))
		priority[CHOICE_RAISE_DEAD] = priority[CHOICE_DISCARD] = priority[CHOICE_ARTIFACT] = priority[CHOICE_DAMAGE] = 1;
	  else
		{
		  priority[CHOICE_RAISE_DEAD] = can[CHOICE_RAISE_DEAD] ? 3 : -100;
		  priority[CHOICE_DISCARD] = can[CHOICE_DISCARD] ? (hand_count[1-player] > 0 ? 4 : 0) : -100;
		  priority[CHOICE_ARTIFACT] = can[CHOICE_ARTIFACT] ? 6 : -100;
		  priority[CHOICE_DAMAGE] = can[CHOICE_DAMAGE] ? 4 : -100;

		  raise_command_singleton_ai_priorities(event, priority);
		}
	}

  card_instance_t* inst = get_card_instance(player, card);

  // have to do all the targeting manually, since DIALOG() can't handle that with DLG_CHOOSE_TWO
  int choice;
  CommandChoices choices[2];
  choices[0] = DIALOG(player, card, event,
					  DLG_CHOOSE_TWO(&choices[1]), DLG_SORT_RESULTS, DLG_RANDOM,
					  "Graveyard to hand",	can[CHOICE_RAISE_DEAD],	priority[CHOICE_RAISE_DEAD],
					  "Discard",			can[CHOICE_DISCARD],	priority[CHOICE_DISCARD],
					  "Destroy artifact",	can[CHOICE_ARTIFACT],	priority[CHOICE_ARTIFACT],
					  "Deal 2 damage",		can[CHOICE_DAMAGE],		priority[CHOICE_DAMAGE]);

  if (event == EVENT_CAN_CAST)
	return choices[0];
  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  for (choice = 0; choice < 2; ++choice)
		switch (choices[choice])
		  {
			case CHOICE_RAISE_DEAD:
			  if (select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &test_raise_dead, 3) == -1)
				{
				  cancel = 1;
				  return 0;
				}
			  break;

			case CHOICE_DISCARD:
			  if (!new_pick_target(&td_player, "TARGET_PLAYER", inst->number_of_targets, 1))
				return 0;
			  break;

			case CHOICE_ARTIFACT:
			  if (!new_pick_target(&td_artifact, "TARGET_ARTIFACT", inst->number_of_targets, 1))
				return 0;
			  break;

			case CHOICE_DAMAGE:
			  if (!new_pick_target(&td_creature_or_player, "TARGET_CREATURE_OR_PLAYER", inst->number_of_targets, 1))
				return 0;
			  break;
		  }
	}
  else if (event == EVENT_RESOLVE_SPELL)
	{
	  // All choices are targeted, so we don't have to worry about the spell being countered because all targets chosen for other choices are invalid
	  int n = -1;
	  for (choice = 0; choice < 2; ++choice)
		switch (choices[choice])
		  {
			case CHOICE_RAISE_DEAD:
			  ;int selected = validate_target_from_grave_source(player, card, player, 3);
			  if (selected != -1)
				from_grave_to_hand(player, selected, TUTOR_HAND);
			  break;

			case CHOICE_DISCARD:
			  if (validate_target(player, card, &td_player, ++n))
				discard(inst->targets[n].player, 0, player);
			  break;

			case CHOICE_ARTIFACT:
			  if (validate_target(player, card, &td_artifact, ++n))
				kill_card(inst->targets[n].player, inst->targets[n].card, KILL_DESTROY);
			  break;

			case CHOICE_DAMAGE:
			  if (validate_target(player, card, &td_creature_or_player, ++n))
				damage_creature(inst->targets[n].player, inst->targets[n].card, 2, player, card);
			  break;
		  }
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/* Narset Transcendent	|2|W|U	0x000000
 * Planeswalker - Narset (6)
 * +1: Look at the top card of your library. If it's a noncreature, nonland card, you may reveal it and put it into your hand.
 * -2: When you cast your next instant or sorcery spell from your hand this turn, it gains rebound.
 * -9: You get an emblem with "Your opponents can't cast noncreature spells." */

int card_necromaster_dragon(int player, int card, event_t event)
{
  /* Necromaster Dragon	|3|U|B	0x2007604
   * Creature - Dragon 4/4
   * Flying
   * Whenever ~ deals combat damage to a player, you may pay |2. If you do, put a 2/2 |Sblack Zombie creature token onto the battlefield and each opponent puts the top two cards of his or her library into his or her graveyard. */

  if (damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRIGGER_OPTIONAL)
	  && has_mana(player, COLOR_COLORLESS, 2)
	  && charge_mana_while_resolving(player, card, event, player, COLOR_COLORLESS, 2))
	{
	  generate_token_by_id(player, card, CARD_ID_ZOMBIE);
	  mill(1-player, 2);
	}

  return 0;
}

int card_ojutais_command(int player, int card, event_t event)
{
  /* Ojutai's Command	|2|W|U	0x200dbbc
   * Instant
   * Choose two -
   * * Return target creature card with converted mana cost 2 or less from your graveyard to the battlefield.
   * * You gain 4 life.
   * * Counter target creature spell.
   * * Draw a card. */

  if (!IS_CASTING(player, card, event))
	return 0;

  typedef enum
  {
	CHOICE_INVALID_TARGET = 0,
	CHOICE_RESURRECT = 1,
	CHOICE_GAIN_LIFE,
	CHOICE_COUNTER,
	CHOICE_DRAW
  } CommandChoices;

  target_definition_t td_spell;
  counterspell_target_definition(player, card, &td_spell, TYPE_CREATURE);

  int can_counter_spell = counterspell(player, card, EVENT_CAN_CAST, &td_spell, -1);

  if (event == EVENT_CAN_CAST && !IS_AI(player))
	return can_counter_spell ? 99 : 1;	// always castable (choices 2 and 4)

  test_definition_t test_resurrect;
  default_test_definition(&test_resurrect, TYPE_CREATURE);
  test_resurrect.cmc = 3;
  test_resurrect.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

  int can[5], priority[5];
  if (event == EVENT_RESOLVE_SPELL)
	{
	  can[CHOICE_RESURRECT] = can[CHOICE_GAIN_LIFE] = can[CHOICE_COUNTER] = can[CHOICE_DRAW] = 1;
	  priority[CHOICE_RESURRECT] = priority[CHOICE_GAIN_LIFE] = priority[CHOICE_COUNTER] = priority[CHOICE_DRAW] = 1;
	}
  else
	{
	  can[CHOICE_RESURRECT] = new_special_count_grave(player, &test_resurrect) > 0 && !graveyard_has_shroud(player);
	  can[CHOICE_GAIN_LIFE] = 1;
	  can[CHOICE_COUNTER] = can_counter_spell;
	  can[CHOICE_DRAW] = 1;

	  if (!IS_AI(player))
		priority[CHOICE_RESURRECT] = priority[CHOICE_GAIN_LIFE] = priority[CHOICE_COUNTER] = priority[CHOICE_DRAW] = 1;
	  else
		{
		  priority[CHOICE_RESURRECT] = can[CHOICE_RESURRECT] ? 30 : -100;

		  priority[CHOICE_GAIN_LIFE] = 700/(life[player]+6) - life[player]*life[player]/4;
		  priority[CHOICE_GAIN_LIFE] = MAX(priority[CHOICE_GAIN_LIFE], 1);

		  priority[CHOICE_COUNTER] = can[CHOICE_COUNTER] ? 1000000 : -100;	// If this is being cast in the interrupt window, the AI's got to pick this.

		  priority[CHOICE_DRAW] = 5 * (count_deck(player) - 5);
		  priority[CHOICE_DRAW] = MIN(priority[CHOICE_DRAW], 50);

		  raise_command_singleton_ai_priorities(event, priority);
		}
	}

  // have to do all the targeting manually, since DIALOG() can't handle that with DLG_CHOOSE_TWO
  int choice;
  CommandChoices choices[2];
  choices[0] = DIALOG(player, card, event,
					  DLG_CHOOSE_TWO(&choices[1]), DLG_SORT_RESULTS, DLG_RANDOM,
					  "Graveyard to battlefield",	can[CHOICE_RESURRECT],	priority[CHOICE_RESURRECT],
					  "Gain 4 life",				can[CHOICE_GAIN_LIFE],	priority[CHOICE_GAIN_LIFE],
					  "Counter creature spell",		can[CHOICE_COUNTER],	priority[CHOICE_COUNTER],
					  "Draw",						can[CHOICE_DRAW],		priority[CHOICE_DRAW]);

  if (event == EVENT_CAN_CAST)
	return (choices[0] <= 0 ? 0
			: can_counter_spell ? 99
			: 1);
  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  for (choice = 0; choice < 2; ++choice)
		switch (choices[choice])
		  {
			case CHOICE_RESURRECT:
			  if (select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &test_resurrect, 3) == -1)
				{
				  cancel = 1;
				  return 0;
				}
			  break;

			case CHOICE_COUNTER:
			  counterspell(player, card, event, &td_spell, 0);	// always in slot 0
			  if (cancel == 1)
				return 0;
			  break;

			case CHOICE_GAIN_LIFE:
			case CHOICE_DRAW:
			case CHOICE_INVALID_TARGET:
			  break;
		  }
	}
  else if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  // Validate
	  int tgts = 0, valid_tgts = 0;
	  for (choice = 0; choice < 2; ++choice)
		switch (choices[choice])
		  {
			case CHOICE_RESURRECT:
			  ++tgts;
			  if ((inst->targets[2].player = validate_target_from_grave_source(player, card, player, 3)) != -1)
				++valid_tgts;
			  else
				choices[choice] = CHOICE_INVALID_TARGET;
			  break;

			case CHOICE_COUNTER:
			  ++tgts;
			  if (counterspell_validate(player, card, &td_spell, 0))	// always in slot 0
				++valid_tgts;
			  else
				choices[choice] = CHOICE_INVALID_TARGET;
			  break;

			case CHOICE_GAIN_LIFE:
			case CHOICE_DRAW:
			case CHOICE_INVALID_TARGET:
			  break;
		  }

	  if (tgts > 0 && valid_tgts == 0)
		cancel = 1;
	  else
		for (choice = 0; choice < 2; ++choice)
		  switch (choices[choice])
			{
			  case CHOICE_INVALID_TARGET:
				break;

			  case CHOICE_RESURRECT:
				reanimate_permanent(player, card, player, inst->targets[2].player, REANIMATE_DEFAULT);
				break;

			  case CHOICE_GAIN_LIFE:
				gain_life(player, 4);
				break;

			  case CHOICE_COUNTER:
				kill_card(inst->targets[0].player, inst->targets[0].card, KILL_BURY);	// always in slot 0
				break;

			  case CHOICE_DRAW:
				draw_a_card(player);
				break;
			}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_pristine_skywise(int player, int card, event_t event)
{
  /* Pristine Skywise	|4|W|U	0x200dbc1
   * Creature - Dragon 6/4
   * Flying
   * Whenever you cast a noncreature spell, untap ~. It gains protection from the color of your choice until end of turn. */

  if (prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  pump_ability_until_eot(player, card, player, card, 0,0, select_a_protection(player)|KEYWORD_RECALC_SET_COLOR,0);
	  untap_card(player, card);
	}

  return 0;
}

int card_ruthless_deathfang(int player, int card, event_t event)
{
  /* Ruthless Deathfang	|4|U|B	0x200dbc6
   * Creature - Dragon 4/4
   * Flying
   * Whenever you sacrifice a creature, target opponent sacrifices a creature. */

  if (whenever_a_player_sacrifices_a_permanent(player, card, event, player, TYPE_CREATURE, RESOLVE_TRIGGER_MANDATORY)
	  && opponent_is_valid_target(player, card))
	impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0);

  return 0;
}

int card_sarkhan_unbroken(int player, int card, event_t event)
{
  /* Sarkhan Unbroken	|2|G|U|R	0x200dbcb
   * Planeswalker - Sarkhan (4)
   * +1: Draw a card, then add one mana of any color to your mana pool.
   * -2: Put a 4/4 |Sred Dragon creature token with flying onto the battlefield.
   * -8: Search your library for any number of Dragon creature cards and put them onto the battlefield. Then shuffle your library. */

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_DRAW_MANA = 1,
		CHOICE_TOKEN,
		CHOICE_SEARCH
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Draw and mana",		1,	1,	+1,
						"Dragon token",			1,	2,	-2,
						"Search for Dragons",	1,	5,	-8);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_DRAW_MANA:
			  // Since this is a loyalty ability, rule 605.1a makes this not be a mana ability; so it happens at resolution
			  draw_a_card(player);
			  do
				{
				  cancel = 0;
				  produce_mana_all_one_color(player, COLOR_TEST_ANY_COLORED, 1);
				} while (cancel == 1);
			  break;

			case CHOICE_TOKEN:
			  ;token_generation_t token;
			  default_token_definition(player, card, CARD_ID_DRAGON, &token);
			  token.pow = token.tou = 4;
			  generate_token(&token);
			  break;

			case CHOICE_SEARCH:
			  ;test_definition_t test;
			  new_default_test_definition(&test, TYPE_CREATURE, "Select a Dragon creature card.");
			  test.subtype = SUBTYPE_DRAGON;
			  test.no_shuffle = 1;

			  while (new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &test) != -1)
				{}

			  shuffle(player);
			  break;
		  }
	}

  return planeswalker(player, card, event, 4);
}

int card_savage_ventmaw(int player, int card, event_t event)
{
  /* Savage Ventmaw	|4|R|G	0x200dbdf
   * Creature - Dragon 4/4
   * Flying
   * Whenever ~ attacks, add |R|R|R|G|G|G to your mana pool. Until end of turn, this mana doesn't empty from your mana pool as steps and phases end. */

  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	{
	  produce_mana(player, COLOR_RED, 3);
	  mana_doesnt_drain_until_eot(player, COLOR_RED, 3);
	  produce_mana(player, COLOR_GREEN, 3);
	  mana_doesnt_drain_until_eot(player, COLOR_GREEN, 3);
	}

  return 0;
}

int card_silumgars_command(int player, int card, event_t event)
{
  /* Silumgar's Command	|3|U|B	0x200dbe4
   * Instant
   * Choose two -
   * * Counter target noncreature spell.
   * * Return target permanent to its owner's hand.
   * * Target creature gets -3/-3 until end of turn.
   * * Destroy target planeswalker. */

  if (!IS_CASTING(player, card, event))
	return 0;

  typedef enum
  {
	CHOICE_COUNTER = 1,
	CHOICE_BOUNCE,
	CHOICE_M3_M3,
	CHOICE_PLANESWALKER
  } CommandChoices;

  target_definition_t td_spell;
  counterspell_target_definition(player, card, &td_spell, TYPE_NONEFFECT);
  td_spell.illegal_type = TYPE_CREATURE;

  target_definition_t td_permanent;
  default_target_definition(player, card, &td_permanent, TYPE_PERMANENT);

  target_definition_t td_creature;
  default_target_definition(player, card, &td_creature, TYPE_CREATURE);

  target_definition_t td_planeswalker;
  default_target_definition(player, card, &td_planeswalker, TARGET_TYPE_PLANESWALKER);

  int can_counter_spell = counterspell(player, card, EVENT_CAN_CAST, &td_spell, -1);

  int can[5];
  if (event == EVENT_RESOLVE_SPELL)
	can[CHOICE_COUNTER] = can[CHOICE_BOUNCE] = can[CHOICE_M3_M3] = can[CHOICE_PLANESWALKER] = 1;
  else
	{
	  can[CHOICE_COUNTER] = can_counter_spell;
	  can[CHOICE_BOUNCE] = can_target(&td_permanent);
	  can[CHOICE_M3_M3] = can_target(&td_creature);
	  can[CHOICE_PLANESWALKER] = can_target(&td_planeswalker);
	}

  card_instance_t* inst = get_card_instance(player, card);

  // have to do all the targeting manually, since DIALOG() can't handle that with DLG_CHOOSE_TWO
  int choice;
  CommandChoices choices[2];
  choices[0] = DIALOG(player, card, event,
					  DLG_CHOOSE_TWO(&choices[1]), DLG_SORT_RESULTS, DLG_RANDOM,
					  "Counter noncreature spell",	can[CHOICE_COUNTER],		1000000,	// If cast as an interrupt, AI has to pick this
					  "Permanent to hand",			can[CHOICE_BOUNCE],			3,
					  "-3/-3",						can[CHOICE_M3_M3],			3,
					  "Destroy planeswalker",		can[CHOICE_PLANESWALKER],	7);

  if (event == EVENT_CAN_CAST)
	return (choices[0] <= 0 ? 0
			: can_counter_spell ? 99
			: 1);
  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  for (choice = 0; choice < 2; ++choice)
		switch (choices[choice])
		  {
			case CHOICE_COUNTER:
			  counterspell(player, card, event, &td_spell, inst->number_of_targets);
			  if (cancel == 1)
				return 0;
			  break;

			case CHOICE_BOUNCE:
			  if (!new_pick_target(&td_permanent, "TARGET_PERMANENT", inst->number_of_targets, 1))
				return 0;
			  break;

			case CHOICE_M3_M3:
			  if (!new_pick_target(&td_creature, "TARGET_CREATURE", inst->number_of_targets, 1))
				return 0;
			  break;

			case CHOICE_PLANESWALKER:
			  if (!new_pick_target(&td_planeswalker, "TARGET_PLANESWALKER", inst->number_of_targets, 1))
				return 0;
			  break;
		  }
	}
  else if (event == EVENT_RESOLVE_SPELL)
	{
	  // All choices are targeted, so we don't have to worry about the spell being countered because all targets chosen for other choices are invalid
	  int n = -1;
	  for (choice = 0; choice < 2; ++choice)
		switch (choices[choice])
		  {
			case CHOICE_COUNTER:
			  if (counterspell_validate(player, card, &td_spell, ++n))
				kill_card(inst->targets[n].player, inst->targets[n].card, KILL_BURY);
			  break;

			case CHOICE_BOUNCE:
			  if (validate_target(player, card, &td_permanent, ++n))
				bounce_permanent(inst->targets[n].player, inst->targets[n].card);
			  break;

			case CHOICE_M3_M3:
			  if (validate_target(player, card, &td_creature, ++n))
				pump_until_eot(player, card, inst->targets[n].player, inst->targets[n].card, -3,-3);
			  break;

			case CHOICE_PLANESWALKER:
			  if (validate_target(player, card, &td_planeswalker, ++n))
				kill_card(inst->targets[n].player, inst->targets[n].card, KILL_DESTROY);
			  break;
		  }

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/* Swift Warkite	|4|B|R	0x000000
 * Creature - Dragon 4/4
 * Flying
 * When ~ enters the battlefield, you may put a creature card with converted mana cost 3 or less from your hand or graveyard onto the battlefield. That creature gains haste. Return it to your hand at the beginning of the next end step. */

/*** Artifact ***/

int card_ancestral_statue(int player, int card, event_t event)
{
  /* Ancestral Statue	|4	0x200dc02
   * Artifact Creature - Golem 3/4
   * When ~ enters the battlefield, return a nonland permanent you control to its owner's hand. */

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.illegal_type = TYPE_LAND;
	  td.allowed_controller = td.preferred_controller = player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_NONLAND_PERMANENT"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  bounce_permanent(inst->targets[0].player, inst->targets[0].card);
		}
	}

  return 0;
}

int card_atarka_monument(int player, int card, event_t event)
{
  /* Atarka Monument	|3	0x200dbe9
   * Artifact
   * |T: Add |R or |G to your mana pool.
   * |4|R|G: ~ becomes a 4/4 |Sred and |Sgreen Dragon artifact creature with flying until end of turn. */
  return monument(player, card, event, COLOR_TEST_RED|COLOR_TEST_GREEN, MANACOST_XRG(4,1,1));
}

/* Custodian of the Trove	|3	=>m11.c:card_rotting_legion
 * Artifact Creature - Golem 2/5
 * Defender
 * ~ enters the battlefield tapped. */

int card_dragonloft_idol(int player, int card, event_t event)
{
  /* Dragonloft Idol	|4	0x200dc07
   * Artifact Creature - Gargoyle 3/3
   * As long as you control a Dragon, ~ gets +1/+1 and has flying and trample. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_ABILITIES) && affect_me(player, card)
	  && check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_DRAGON) && !is_humiliated(player, card))
	{
	  if (event == EVENT_ABILITIES)
		event_result |= KEYWORD_FLYING | KEYWORD_TRAMPLE;
	  else
		event_result += 1;
	}

  return 0;
}

int card_dromoka_monument(int player, int card, event_t event)
{
  /* Dromoka Monument	|3	0x200dbee
   * Artifact
   * |T: Add |G or |W to your mana pool.
   * |4|G|W: ~ becomes a 4/4 |Sgreen and |Swhite Dragon artifact creature with flying until end of turn. */
  return monument(player, card, event, COLOR_TEST_GREEN|COLOR_TEST_WHITE, MANACOST_XGW(4,1,1));
}

int card_gate_smasher(int player, int card, event_t event)
{
  /* Gate Smasher	|3	0x200dc0c
   * Artifact - Equipment
   * ~ can be attached only to a creature with toughness 4 or greater.
   * Equipped creature gets +3/+0 and has trample.
   * Equip |3 */

  return vanilla_equipment(player, card, event, 3, 3,0, KEYWORD_TRAMPLE, 0);
}

/* Keeper of the Lens	|1	=>vanilla (see http://www.slightlymagic.net/forum/viewtopic.php?t=13200)
 * Artifact Creature - Golem 1/2
 * You may look at face-down creatures you don't control. */

int card_kolaghan_monument(int player, int card, event_t event)
{
  /* Kolaghan Monument	|3	0x200dbf3
   * Artifact
   * |T: Add |B or |R to your mana pool.
   * |4|B|R: ~ becomes a 4/4 |Sblack and |Sred Dragon artifact creature with flying until end of turn. */
  return monument(player, card, event, COLOR_TEST_BLACK|COLOR_TEST_RED, MANACOST_XBR(4,1,1));
}

int card_ojutai_monument(int player, int card, event_t event)
{
  /* Ojutai Monument	|3	0x200dbf8
   * Artifact
   * |T: Add |W or |U to your mana pool.
   * |4|W|U: ~ becomes a 4/4 |Swhite and |Sblue Dragon artifact creature with flying until end of turn. */
  return monument(player, card, event, COLOR_TEST_WHITE|COLOR_TEST_BLUE, MANACOST_XWU(4,1,1));
}

int card_silumgar_monument(int player, int card, event_t event)
{
  /* Silumgar Monument	|3	0x200dbfd
   * Artifact
   * |T: Add |U or |B to your mana pool.
   * |4|U|B: ~ becomes a 4/4 |Sblue and |Sblack Dragon artifact creature with flying until end of turn. */
  return monument(player, card, event, COLOR_TEST_BLUE|COLOR_TEST_BLACK, MANACOST_XUB(4,1,1));
}

/* Spidersilk Net	|0	=>zendikar.c:card_spidersilk_net
 * Artifact - Equipment
 * Equipped creature gets +0/+2 and has reach.
 * Equip |2 */

/* Stormrider Rig	|2	=>planechase.c:card_sai_of_the_shinobi
 * Artifact - Equipment
 * Equipped creature gets +1/+1.
 * Whenever a creature enters the battlefield under your control, you may attach ~ to it.
 * Equip |2 */

int card_tapestry_of_the_ages(int player, int card, event_t event)
{
  /* Tapestry of the Ages	|4	0x200e210
   * Artifact
   * |2, |T: Draw a card. Activate this ability only if you've cast a noncreature spell this turn. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	draw_cards(player, 1);

  int rval = generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);

  if (event == EVENT_CAN_ACTIVATE && rval && get_specific_storm_count(player) <= get_stormcreature_count(player))
	return 0;

  return rval;
}

int card_vial_of_dragonfire(int player, int card, event_t event)
{
  /* Vial of Dragonfire	|2	0x200dc11
   * Artifact
   * |2, |T, Sacrifice ~: ~ deals 2 damage to target creature. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	damage_target0(player, card, 2);

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_SACRIFICE_ME, MANACOST_X(2), 0, &td, "TARGET_CREATURE");
}

/*** Land ***/

/* Evolving Wilds	""	=>m11.c:card_terramorphic_expanse
 * Land
 * |T, Sacrifice ~: Search your library for a basic land card and put it onto the battlefield tapped. Then shuffle your library. */

/* Haven of the Spirit Dragon	""	0x000000
 * Land
 * |T: Add |1 to your mana pool.
 * |T: Add one mana of any color to your mana pool. Spend this mana only to cast a Dragon creature spell.
 * |2, |T, Sacrifice ~: Return target Dragon creature card or Ugin planeswalker card from your graveyard to your hand. */

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
