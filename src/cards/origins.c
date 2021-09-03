// -*- c-basic-offset:2 -*-
#include "manalink.h"

/***** Functions *****/

static int is_renowned(int player, int card)
{
  return check_special_flags3(player, card, SF3_RENOWNED);
}

static void renown(int player, int card, event_t event, int n)
{
  /* 702.111. Renown
   *
   * 702.111a Renown is a triggered ability. "Renown N" means "When this creature deals combat damage to a player, if it isn't renowned, put N +1/+1 counters on
   * it and it becomes renowned."
   *
   * 702.111b Renowned is a designation that has no rules meaning other than to act as a marker that the renown ability and other spells and abilities can
   * identify. Only permanents can be or become renowned. Once a permanent becomes renowned, it stays renowned until it leaves the battlefield. Renowned is
   * neither an ability nor part of the permanent's copiable values.
   *
   * 702.111c If a creature has multiple instances of renown, each triggers separately. The first such ability to resolve will cause the creature to become
   * renowned, and subsequent abilities will have no effect. (See rule 603.4) */

  if ((event == EVENT_DEAL_DAMAGE || trigger_condition == TRIGGER_DEAL_DAMAGE)
	  && !is_renowned(player, card)
	  && has_combat_damage_been_inflicted_to_a_player(player, card, event))
	{
	  set_special_flags3(player, card, SF3_RENOWNED);
	  add_1_1_counters(player, card, n);
	  dispatch_xtrigger2(player, XTRIGGER_BECAME_RENOWNED, "became renowned", 0, player, card);
	}
}

static void
return_to_bf_transformed_impl(int player, int card)
{
  // These won't have been set yet.  (Though the actual values on the card that they're redundant to will have been.)
  card_instance_t* inst = get_card_instance(player, card);
  inst->targets[13].player = inst->targets[13].card = get_id(player, card);

  true_transform(player, card);

  // Update id before putting back on the battlefield, so the correct version of the card gets EVENT_RESOLVE_SPELL
  inst->internal_card_id = inst->targets[12].card;
}

static void exile_and_return_to_bf_transformed(int player, int card)
{
  blink_effect(player, card, &return_to_bf_transformed_impl);
}

static int spell_mastery(int player)
{
  // Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, ...

  // Equivalent to count_graveyard_by_type(player, TYPE_INSTANT|TYPE_SORCERY) > 2, but stops after finding the second card.
  int i, count = 0;
  const int* gy = get_grave(player);
  for (i = 0; i < 500 && gy[i] != -1; ++i)
	if (is_what(-1, gy[i], TYPE_INSTANT|TYPE_SORCERY)
		&& ++count >= 2)
	  return 1;
  return 0;
}

/***** Cards *****/

/*** White ***/

int card_akroan_jailer(int player, int card, event_t event)
{
  /* Akroan Jailer	|W	0x200dc16
   * Creature - Human Soldier 1/1
   * |2|W, |T: Tap target creature. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  tap_card(inst->targets[0].player, inst->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_XW(2,1), 0, &td, "TARGET_CREATURE");
}

int card_ampryn_tactician(int player, int card, event_t event)
{
  /* Ampryn Tactician	|2|W|W	0x200dc1b
   * Creature - Human Soldier 3/3
   * When ~ enters the battlefield, creatures you control get +1/+1 until end of turn. */

  if (comes_into_play(player, card, event))
	pump_creatures_until_eot(player, card, player, 0, 1,1, 0,0, NULL);

  return 0;
}

/* Anointer of Champions	|W	=>m11.c:card_infantry_veteran
 * Creature - Human Cleric 1/1
 * |T: Target attacking creature gets +1/+1 until end of turn. */

int card_archangel_of_tithes(int player, int card, event_t event)
{
  /* Archangel of Tithes	|1|W|W|W	0x200e1d9
   * Creature - Angel 3/5
   * Flying
   * As long as ~ is untapped, creatures can't attack you or a planeswalker you control unless their controller pays |1 for each of those creatures.
   * As long as ~ is attacking, creatures can't block unless their controller pays |1 for each of those creatures. */

  if (!is_tapped(player, card))
	tax_attack(player, card, event);

  if (trigger_condition == TRIGGER_PAY_TO_BLOCK && affect_me(player, card) && reason_for_trigger_controller == 1-current_turn
	  && !forbid_attack && check_state(player, card, STATE_ATTACKING) && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		{
		  if (!has_mana(1-current_turn, COLOR_ANY, 1))
			forbid_attack = 1;
		  else
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		}
	  else if (event == EVENT_RESOLVE_TRIGGER
			   && !charge_mana_while_resolving(player, card, event, 1-current_turn, COLOR_COLORLESS, 1))
		{
		  forbid_attack = 1;
		  cancel = 0;
		}
	  if (forbid_attack)
		remove_state(trigger_cause_controller, trigger_cause, STATE_UNKNOWN8000);
	}

  return 0;
}

/* Auramancer	|2|W	=>odissey.c:card_auramancer
 * Creature - Human Wizard 2/2
 * When ~ enters the battlefield, you may return target enchantment card from your graveyard to your hand. */

/* Aven Battle Priest	|5|W	=>portal_1_2_3k.c:20056e2
 * Creature - Bird Cleric 3/3
 * Flying
 * When ~ enters the battlefield, you gain 3 life. */

int card_blessed_spirits(int player, int card, event_t event)
{
  /* Blessed Spirits	|2|W	0x200dc20
   * Creature - Spirit 2/2
   * Flying
   * Whenever you cast an enchantment spell, put a +1/+1 counter on ~. */

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ENCHANTMENT,MATCH, 0,0, 0,0, 0,0, -1,0))
	add_1_1_counter(player, card);

  return 0;
}

/* Celestial Flare	|W|W	=>m14.c:card_celestial_flare
 * Instant
 * Target player sacrifices an attacking or blocking creature. */

/* Charging Griffin	|3|W	=>m12.c:card_benalish_veteran
 * Creature - Griffin 2/2
 * Flying
 * Whenever ~ attacks, it gets +1/+1 until end of turn. */

int card_cleric_of_the_forward_order(int player, int card, event_t event)
{
  /* Cleric of the Forward Order	|1|W	0x200dc25
   * Creature - Human Cleric 2/2
   * When ~ enters the battlefield, you gain 2 life for each creature you control named ~. */

  if (comes_into_play(player, card, event))
	gain_life(player, 2 * count_cards_by_id(player, get_id(player, card)));

  return 0;
}

int card_consuls_lieutenant(int player, int card, event_t event)
{
  /* Consul's Lieutenant	|W|W	0x200dc2a
   * Creature - Human Soldier 2/1
   * First strike
   * Renown 1
   * Whenever ~ attacks, if it's renowned, other attacking creatures you control get +1/+1 until end of turn. */

  renown(player, card, event, 1);

  if ((event == EVENT_DECLARE_ATTACKERS || xtrigger_condition() == XTRIGGER_ATTACKING)
	  && is_renowned(player, card)
	  && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	{
	  test_definition_t test;
	  default_test_definition(&test, TYPE_CREATURE);
	  test.state = STATE_ATTACKING;
	  test.not_me = 1;

	  pump_creatures_until_eot(player, card, player, 0, 1,1, 0, 0, &test);
	}

  return 0;
}

/* Enlightened Ascetic	|1|W	=>m11.c:card_war_priest_of_thune
 * Creature - Cat Monk 1/1
 * When ~ enters the battlefield, you may destroy target enchantment. */

int card_enshrouding_mist(int player, int card, event_t event)
{
  /* Enshrouding Mist	|W	0x200dc2f
   * Instant
   * Target creature gets +1/+1 until end of turn. Prevent all damage that would be dealt to it this turn. If it's renowned, untap it. */

  if (event == EVENT_CHECK_PUMP && has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id))
	{
	  pumpable_power[player] += 1;
	  pumpable_toughness[player] += 99;	// since preventing all damage
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
		  get_card_instance(player, prevent_all_damage_to_target(player, card, inst->targets[0].player, inst->targets[0].card, 1))->token_status |= STATUS_INVISIBLE_FX;
		  pump_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 1,1);
		  if (is_renowned(inst->targets[0].player, inst->targets[0].card))
			untap_card(inst->targets[0].player, inst->targets[0].card);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_gideons_phalanx(int player, int card, event_t event)
{
  /* Gideon's Phalanx	|5|W|W	0x200dc34
   * Instant
   * Put four 2/2 |Swhite Knight creature tokens with vigilance onto the battlefield.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, creatures you control gain indestructible until end of turn. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_KNIGHT, &token);
	  token.s_key_plus = SP_KEYWORD_VIGILANCE;
	  token.qty = 4;
	  generate_token(&token);

	  if (spell_mastery(player))
		pump_creatures_until_eot(player, card, player, 0, 0,0, 0,SP_KEYWORD_INDESTRUCTIBLE, NULL);

	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_grasp_of_the_hieromancer(int player, int card, event_t event)
{
  /* Grasp of the Hieromancer	|1|W	0x200dc39
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +1/+1 and has "Whenever this creature attacks, tap target creature defending player controls." */

  card_instance_t* inst;
  int t_player, t_card;
  if ((event == EVENT_DECLARE_ATTACKERS || xtrigger_condition() == XTRIGGER_ATTACKING)
	  && (inst = in_play(player, card))
	  && (t_player = inst->damage_target_player) >= 0
	  && (t_card = inst->damage_target_card) >= 0
	  && !is_humiliated(t_player, t_card)
	  && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, t_player, t_card))
	{
	  target_definition_t td;
	  default_target_definition(t_player, t_card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_next_target_arbitrary(&td, "TARGET_CREATURE_OPPONENT_CONTROLS", player, card))
		tap_card(inst->targets[0].player, inst->targets[0].card);
	}

  return generic_aura(player, card, event, player, 1,1, 0,0, 0,0,0);
}

/* Hallowed Moonlight	|1|W	0x000000
 * Instant
 * Until end of turn, if a creature would enter the battlefield and it wasn't cast, exile it instead.
 * Draw a card. */

int card_healing_hands(int player, int card, event_t event)
{
  /* Healing Hands	|2|W	0x200dc3e
   * Sorcery
   * Target player gains 4 life.
   * Draw a card. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  gain_life(get_card_instance(player, card)->targets[0].player, 4);
		  draw_a_card(player);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_heavy_infantry(int player, int card, event_t event)
{
  /* Heavy Infantry	|4|W	0x200dc43
   * Creature - Human Soldier 3/4
   * When ~ enters the battlefield, tap target creature an opponent controls. */

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  tap_card(inst->targets[0].player, inst->targets[0].card);
		}
	}

  return 0;
}

int card_hixus_prison_warden(int player, int card, event_t event)
{
  /* Hixus, Prison Warden	|3|W|W	0x200e1e8
   * Legendary Creature - Human Soldier 4/4
   * Flash
   * Whenever a creature deals combat damage to you, if ~ entered the battlefield this turn, exile that creature until ~ leaves the battlefield. */

  card_instance_t* inst, *damage = combat_damage_being_dealt(event);
  if (damage
	  && damage->damage_target_card == -1 && damage->damage_target_player == player && !damage_is_to_planeswalker(damage)
	  && (damage->targets[3].player & TYPE_CREATURE)	// probably redundant to "combat damage"
	  && (inst = in_play(player, card)) && !is_humiliated(player, card)
	  && inst->info_slot == turn_count
	  && inst->eot_toughness < 40)	// targets[1] through targets[10], 8 bytes each, 2 bytes per creature
	{
	  unsigned char* creatures = (unsigned char*)(&inst->targets[1].player);
	  creatures[2 * inst->eot_toughness] = damage->damage_source_player;
	  creatures[2 * inst->eot_toughness + 1] = damage->damage_source_card;
	  inst->eot_toughness++;
	}

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player
	  && (inst = get_card_instance(player, card))->eot_toughness > 0)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;
	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  unsigned char* creatures = (unsigned char*)(&inst->targets[1].player);
		  unsigned int i;
		  for (i = 0; i < inst->eot_toughness; ++i)
			if (in_play(creatures[2 * i], creatures[2 * i + 1]))
			  obliviation(player, card, creatures[2 * i], creatures[2 * i + 1]);

		  inst->eot_toughness = 0;
		}
	}

  return_from_oblivion(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	get_card_instance(player, card)->info_slot = turn_count;

  return flash(player, card, event);
}

int card_knight_of_the_pilgrims_road(int player, int card, event_t event)
{
  /* Knight of the Pilgrim's Road	|2|W	0x200dc48
   * Creature - Human Knight 3/2
   * Renown 1 */
  renown(player, card, event, 1);
  return 0;
}

/* Knight of the White Orchid	|W|W	=>shards_of_alara.c:card_knight_of_the_white_orchid
 * Creature - Human Knight 2/2
 * First strike
 * When ~ enters the battlefield, if an opponent controls more lands than you, you may search your library for |Ha Plains card, put it onto the battlefield, then shuffle your library. */

/* Knightly Valor	|4|W	=>return_to_ravnica.c:card_knightly_valor
 * Enchantment - Aura
 * Enchant creature
 * When ~ enters the battlefield, put a 2/2 |Swhite Knight creature token with vigilance onto the battlefield.
 * Enchanted creature gets +2/+2 and has vigilance. */

int card_kytheon_hero_of_akros(int player, int card, event_t event)
{
  /* Kytheon, Hero of Akros	|W	0x200e1de
   * Legendary Creature - Human Soldier 2/1
   * At end of combat, if ~ and at least two other creatures attacked this combat, exile Kytheon, then return him to the battlefield transformed under his owner's control.
   * |2|W: ~ gains indestructible until end of turn. */

  double_faced_card(player, card, event);
  check_legend_rule(player, card, event);

  if (event == EVENT_DECLARE_ATTACKERS)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  inst->info_slot = (inst->state & STATE_ATTACKING) && number_of_attackers_declared >= 3 ? CARD_ID_KYTHEON_HERO_OF_AKROS : 0;
	}
  if (event == EVENT_CLEANUP)
	get_card_instance(player, card)->info_slot = 0;

  if (trigger_condition == TRIGGER_END_COMBAT && affect_me(player, card) && reason_for_trigger_controller == player
	  && get_card_instance(player, card)->info_slot == CARD_ID_KYTHEON_HERO_OF_AKROS)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;
	  if (event == EVENT_RESOLVE_TRIGGER)
		exile_and_return_to_bf_transformed(player, card);
	}

  return generic_shade(player, card, event, 0, MANACOST_XW(2,1), 0,0, 0,SP_KEYWORD_INDESTRUCTIBLE);
}
int fx_attack_gideon_battle_forged(int player, int card, event_t event)
{
  // This effect's address is referenced in planeswalker.c to autochoose the attacked planewalker

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_BEGIN_TURN && current_turn == inst->damage_target_player)
	inst->targets[3].player = CARD_ID_GIDEON_BATTLE_FORGED;

  if (inst->targets[3].player != CARD_ID_GIDEON_BATTLE_FORGED)	// Not controller's next turn yet
	return 0;

  if (event == EVENT_STATIC_EFFECTS && !in_play(inst->damage_source_player, inst->damage_source_card))
	kill_card(player, card, KILL_REMOVE);

  if (event == EVENT_MUST_ATTACK
	  && in_play(inst->damage_source_player, inst->damage_source_card)
	  && is_planeswalker(inst->damage_source_player, inst->damage_source_card)
	  && inst->damage_source_player != inst->damage_target_player)
	attack_if_able(inst->damage_target_player, inst->damage_target_card, event);

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
static int fx_indestructible_until_your_next_turn(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (affect_me(inst->damage_target_player, inst->damage_target_card))
		indestructible(inst->damage_target_player, inst->damage_target_card, event);
	}

  if (event == EVENT_BEGIN_TURN && current_turn == player)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
int card_gideon_battle_forged(int player, int card, event_t event)
{
  /* Gideon, Battle-Forged	""	0x200e1e3
   * Planeswalker - Gideon (3)
   * +2: Up to one target creature an opponent controls attacks ~ during its controller's next turn if able.
   * +1: Until your next turn, target creature gains indestructible. Untap that creature.
   * 0: Until end of turn, ~ becomes a 4/4 Human Soldier creature with indestructible that's still a planeswalker. Prevent all damage that would be dealt to him this turn. */

  if (IS_ACTIVATING(event))
	{
	  target_definition_t td_opponent_creature;
	  default_target_definition(player, card, &td_opponent_creature, TYPE_CREATURE);
	  td_opponent_creature.allowed_controller = 1-player;
	  td_opponent_creature.allow_cancel = 3;	// can cancel, can pick done

	  target_definition_t td_creature;
	  default_target_definition(player, card, &td_creature, TYPE_CREATURE);
	  td_creature.preferred_controller = player;

	  card_instance_t* inst = get_card_instance(player, card);

	  enum
	  {
		CHOICE_NETTLE = 1,
		CHOICE_INDESTRUCTIBLE,
		CHOICE_ANIMATE,
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Creature attacks",	1,							3,	+2,
						"Indestructible",	can_target(&td_creature),	2,	+1,
						"Become creature",	1,							1,	0);
	  if (event == EVENT_CAN_ACTIVATE && !choice)
		return 0;
	  if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_NETTLE:
			  inst->number_of_targets = 0;
			  if (can_target(&td_opponent_creature) && !pick_target(&td_opponent_creature, "TARGET_CREATURE_OPPONENT_CONTROLS"))
				cancel = inst->targets[0].card == -1;
			  break;

			case CHOICE_INDESTRUCTIBLE:
			  inst->number_of_targets = 0;
			  if (can_target(&td_creature))
				pick_target(&td_creature, "TARGET_CREATURE");

			case CHOICE_ANIMATE:
			  break;
		  }
	  if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_NETTLE:
			  if (inst->number_of_targets > 0 && valid_target(&td_opponent_creature))
				{
				  int p = inst->parent_controller, c = inst->parent_card;
				  if (in_play(p, c))
					alternate_legacy_text(1, p, create_targetted_legacy_effect(p, c, fx_attack_gideon_battle_forged,
																			   inst->targets[0].player, inst->targets[0].card));
				}
			  break;

			case CHOICE_INDESTRUCTIBLE:
			  if (valid_target(&td_creature))
				{
				  alternate_legacy_text(2, player, create_targetted_legacy_effect(player, card, fx_indestructible_until_your_next_turn,
																				  inst->targets[0].player, inst->targets[0].card));
				  untap_card(inst->targets[0].player, inst->targets[0].card);
				}
			  break;

			case CHOICE_ANIMATE:
			  ;int p = inst->parent_controller, c = inst->parent_card;
			  if (in_play(p, c))
				{
				  get_card_instance(p, prevent_all_damage_to_target(p, c, p, c, 1))->token_status |= STATUS_INVISIBLE_FX;
				  alternate_legacy_text(3, p, animate_self(p, c, 4,4, 0,SP_KEYWORD_INDESTRUCTIBLE, 0, 0));
				}
			  break;
		  }
	}

  has_subtype_if_animated_self(player, card, event, SUBTYPE_HUMAN);
  has_subtype_if_animated_self(player, card, event, SUBTYPE_SOLDIER);

  return planeswalker(player, card, event, 3);
}

int card_kytheons_irregulars(int player, int card, event_t event)
{
  /* Kytheon's Irregulars	|2|W|W	0x200dc4d
   * Creature - Human Soldier 4/3
   * Renown 1
   * |W|W: Tap target creature. */

  renown(player, card, event, 1);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  tap_card(inst->targets[0].player, inst->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_W(2), 0, &td, "TARGET_CREATURE");
}

int card_kytheons_tactics(int player, int card, event_t event)
{
  /* Kytheon's Tactics	|1|W|W	0x200dc52
   * Sorcery
   * Creatures you control get +2/+1 until end of turn.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, those creatures also gain vigilance until end of turn. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (!spell_mastery(player))
		pump_creatures_until_eot(player, card, player, 1, 2,1, 0,0, NULL);
	  else
		pump_creatures_until_eot(player, card, player, 2, 2,1, 0,SP_KEYWORD_VIGILANCE, NULL);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Mighty Leap	|1|W	=>m11.c:card_mighty_leap
 * Instant
 * Target creature gets +2/+2 and gains flying until end of turn. */

/* Murder Investigation	|1|W	=>gatecrash.c:card_murder_investigation
 * Enchantment - Aura
 * Enchant creature you control
 * When enchanted creature dies, put X 1/1 |Swhite Soldier creature tokens onto the battlefield, where X is its power. */

int card_patron_of_the_valiant(int player, int card, event_t event)
{
  /* Patron of the Valiant	|3|W|W	0x200dc57
   * Creature - Angel 4/4
   * Flying
   * When ~ enters the battlefield, put a +1/+1 counter on each creature you control with a +1/+1 counter on it. */

  if (comes_into_play(player, card, event))
	{
	  char marked[151] = {0};
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && count_1_1_counters(player, c) > 0 && is_what(player, c, TYPE_CREATURE))
		  marked[c] = 1;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (marked[c] && in_play(player, c))
		  add_1_1_counter(player, c);
	}

  return 0;
}

int card_relic_seeker(int player, int card, event_t event)
{
  /* Relic Seeker	|1|W	0x200dc5c
   * Creature - Human Soldier 2/2
   * Renown 1
   * When ~ becomes renowned, you may search your library for an Equipment card, reveal it, put it into your hand, then shuffle your library. */

  renown(player, card, event, 1);

  if (xtrigger_condition() == XTRIGGER_BECAME_RENOWNED && affect_me(player, card) && player == reason_for_trigger_controller
	  && trigger_cause == card && trigger_cause_controller == player && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_AI(player);
	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_ARTIFACT, "Select an Equipment card.");
		  test.subtype = SUBTYPE_EQUIPMENT;
		  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		}
	}

  return 0;
}

int card_sentinel_of_the_eternal_watch(int player, int card, event_t event)
{
  /* Sentinel of the Eternal Watch	|5|W	0x200dc61
   * Creature - Giant Soldier 4/6
   * Vigilance
   * At the beginning of combat on each opponent's turn, tap target creature that player controls. */

  vigilance(player, card, event);

  if (event == EVENT_MUST_ATTACK && current_turn == 1-player && !is_humiliated(player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  tap_card(inst->targets[0].player, inst->targets[0].card);
		}
	}

  return 0;
}

/* Sigil of the Empty Throne	|3|W|W	=>conflux.c:card_sigil_of_the_empty_throne
 * Enchantment
 * Whenever you cast an enchantment spell, put a 4/4 |Swhite Angel creature token with flying onto the battlefield. */

/* Stalwart Aven	|2|W	=>card_knight_of_the_pilgrims_road
 * Creature - Bird Soldier 1/3
 * Flying
 * Renown 1 */

/* Starfield of Nyx	|4|W	0x000000
 * Enchantment
 * At the beginning of your upkeep, you may return target enchantment card from your graveyard to the battlefield.
 * As long as you control five or more enchantments, each other non-Aura enchantment you control is a creature in addition to its other types and has base power and base toughness each equal to its converted mana cost. */

int card_suppression_bonds(int player, int card, event_t event)
{
  /* Suppression Bonds	|3|W	0x200dc66
   * Enchantment - Aura
   * Enchant nonland permanent
   * Enchanted permanent can't attack or block, and its activated abilities can't be activated. */

  // This will break if the aura is moved, but then, so will Arrest and the zillion other spells modeled on it.

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.illegal_type = TYPE_LAND;

  int rval = targeted_aura(player, card, event, &td, "TARGET_NONLAND_PERMANENT");

  card_instance_t* inst = in_play(player, card);
  if (inst && inst->damage_target_player >= 0)
	{
	  if (event == EVENT_RESOLVE_SPELL)
		disable_all_activated_abilities(inst->damage_target_player, inst->damage_target_card, 1);

	  cannot_attack(inst->damage_target_player, inst->damage_target_card, event);
	  cannot_block(inst->damage_target_player, inst->damage_target_card, event);
	  if (leaves_play(player, card, event))
		disable_all_activated_abilities(inst->damage_target_player, inst->damage_target_card, 0);
	}

  return rval;
}

int card_swift_reckoning(int player, int card, event_t event)
{
  /* Swift Reckoning	|1|W	0x200e1ed
   * Sorcery
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, you may cast ~ as though it had flash.
   * Destroy target tapped creature. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  if (event == EVENT_CAN_CAST && !can_sorcery_be_played(player, event) && !spell_mastery(player))
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
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "ROYAL_ASSASSIN", 1, NULL);	// "Select target tapped creature."
}

int card_topan_freeblade(int player, int card, event_t event)
{
  /* Topan Freeblade	|1|W	0x200dc6b
   * Creature - Human Soldier 2/2
   * Vigilance
   * Renown 1 */

  vigilance(player, card, event);
  renown(player, card, event, 1);
  return 0;
}

/* Totem-Guide Hartebeest	|4|W	=>rise_of_the_eldrazi.c:card_totem_guide_hartebeest
 * Creature - Antelope 2/5
 * When ~ enters the battlefield, you may search your library for an Aura card, reveal it, put it into your hand, then shuffle your library. */

static const char* target_is_unmarked_nonland(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return !((get_card_instance(player, card)->state & STATE_TARGETTED) || is_what(player, card, TYPE_LAND)) ? NULL : "AI prefers to pick nonredundant nonland";
}
static const char* target_is_marked_or_land(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return ((get_card_instance(player, card)->state & STATE_TARGETTED) || is_what(player, card, TYPE_LAND)) ? NULL : "AI prefers to pick redundant or land";
}
int card_tragic_arrogance(int player, int card, event_t event)
{
  /* Tragic Arrogance	|3|W|W	0x200e1f2
   * Sorcery
   * For each player, you choose from among the permanents that player controls an artifact, a creature, an enchantment, and a planeswalker. Then each player sacrifices all other nonland permanents he or she controls. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  type_t typs[4] = { TYPE_ARTIFACT, TYPE_CREATURE, TYPE_ENCHANTMENT, TARGET_TYPE_PLANESWALKER };
	  const char* prompts[2][4] =
		{
		  {
			"Select an artifact you control.",
			"Select a creature you control.",
			"Select an enchantment you control.",
			"Select a planeswalker you control."
		  },
		  {
			"Select an artifact an opponent controls.",
			"Select a creature an opponent controls.",
			"Select an enchantment an opponent controls.",
			"Select a planeswalker an opponent controls."
		  }
		};

	  target_definition_t td;
	  base_target_definition(player, card, &td, 0);
	  td.allow_cancel = 0;

	  target_definition_t td_unmarked_nonland;
	  base_target_definition(player, card, &td_unmarked_nonland, 0);
	  td_unmarked_nonland.extra = (int)target_is_unmarked_nonland;
	  td_unmarked_nonland.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td_unmarked_nonland.allowed_controller = td_unmarked_nonland.preferred_controller = 1-player;
	  td_unmarked_nonland.allow_cancel = 0;

	  target_definition_t td_marked_or_land;
	  base_target_definition(player, card, &td_marked_or_land, 0);
	  td_marked_or_land.extra = (int)target_is_marked_or_land;
	  td_marked_or_land.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	  td_marked_or_land.allowed_controller = td_marked_or_land.preferred_controller = player;
	  td_marked_or_land.allow_cancel = 0;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;

	  int i, p, t;
	  for (i = 0, p = player; i <= 1; ++i, p = 1-p)
		{
		  td.allowed_controller = td.preferred_controller = p;
		  target_definition_t* td_pref = p == player ? &td_unmarked_nonland : &td_marked_or_land;
		  for (t = 0; t < 4; ++t)
			{
			  td.required_type = td_pref->required_type = typs[t];
			  if (can_target(&td))
				{
				  pick_next_target_noload(IS_AI(player) && can_target(td_pref) ? td_pref : &td, prompts[i][t]);
				  add_state(inst->targets[0].player, inst->targets[0].card, STATE_TARGETTED);
				  inst->number_of_targets = 0;
				}
			}
		}

	  int can_cause[2] = { can_cause_sacrifice(player, 0), can_cause_sacrifice(player, 1) };

	  char marked[2][151] = {{0}};
	  int c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c))
			{
			  if (check_state(p, c, STATE_TARGETTED))
				remove_state(p, c, STATE_TARGETTED);
			  else if (can_cause[p] && !is_what(p, c, TYPE_LAND) && !check_special_flags2(p, c, SF2_CANNOT_BE_SACRIFICED))
				marked[p][c] = 1;
			}

	  for (i = 0, p = player; i <= 1; ++i, p = 1-p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (marked[p][c] && in_play(p, c))
			kill_card(p, c, KILL_SACRIFICE);
	}

  return basic_spell(player, card, event);
}

int card_valor_in_akros(int player, int card, event_t event)
{
  /* Valor in Akros	|3|W	0x200dc75
   * Enchantment
   * Whenever a creature enters the battlefield under your control, creatures you control get +1/+1 until end of turn. */

  if (specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0))
	pump_creatures_until_eot_merge_pt(player, card, player, 1,1, NULL);

  return global_enchantment(player, card, event);
}

/* Vryn Wingmare	|2|W	=>lorwyn.c:card_thorn_of_amethyst
 * Creature - Pegasus 2/1
 * Flying
 * Noncreature spells cost |1 more to cast. */

int card_war_oracle(int player, int card, event_t event)
{
  /* War Oracle	|2|W|W	0x200dc7a
   * Creature - Human Cleric 3/3
   * Lifelink
   * Renown 1 */

  lifelink(player, card, event);
  renown(player, card, event, 1);
  return 0;
}

/* Yoked Ox	|W	=>vanilla
 * Creature - Ox 0/4 */

/*** Blue ***/

int card_alhammarret_high_arbiter(int player, int card, event_t event)
{
  /* Alhammarret, High Arbiter	|5|U|U	0x200e224
   * Legendary Creature - Sphinx 5/5
   * Flying
   * As ~ enters the battlefield, each opponent reveals his or her hand. You choose the name of a nonland card revealed this way.
   * Your opponents can't cast spells with the chosen name. */

  card_instance_t* inst;
  if (event == EVENT_MODIFY_COST_GLOBAL)
	{
	  if ((affected_card_controller == -1 ? event_result : affected_card_controller) == 1-player
		  && (inst = in_play(player, card)) && !is_humiliated(player, card)
		  && inst->info_slot == (affected_card_controller == -1 ? affected_card : get_card_instance(affected_card_controller, affected_card)->internal_card_id)
		  && inst->eot_toughness == CARD_ID_ALHAMMARRET_HIGH_ARBITER)
		infinite_casting_cost();
	  return 0;
	}

  if (event == EVENT_RESOLVE_SPELL && !is_humiliated(player, card))
	{
	  ec_definition_t coerce;
	  default_ec_definition(1-player, player, &coerce);
	  coerce.effect = EC_SELECT_CARD;

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "Select a nonland card.");
	  test.type_flag = DOESNT_MATCH;
	  int sel = new_effect_coercion(&coerce, &test);
	  if (sel != -1)
		{
		  inst = get_card_instance(player, card);
		  inst->eot_toughness = CARD_ID_ALHAMMARRET_HIGH_ARBITER;
		  int iid = get_card_instance(1-player, sel)->internal_card_id;
		  inst->info_slot = iid;
		  create_card_name_legacy(player, card, cards_data[iid].id);
		}
	}

  return 0;
}

int card_anchor_to_the_aether(int player, int card, event_t event)
{
  /* Anchor to the AEther	|2|U	0x200dc7f
   * Sorcery
   * Put target creature on top of its owner's library. Scry 1. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  put_on_top_of_deck(inst->targets[0].player, inst->targets[0].card);
		  scry(player, 1);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_artificers_epiphany(int player, int card, event_t event)
{
  /* Artificer's Epiphany	|2|U	0x200dc84
   * Instant
   * Draw two cards. If you control no artifacts, discard a card. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int arts = count_permanents_by_type(player, TYPE_ARTIFACT) > 0;	// just in case a draw trigger adds an artifact
	  draw_cards(player, 2);
	  if (arts == 0)
		discard(player, 0, player);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_aspiring_aeronaut(int player, int card, event_t event)
{
  /* Aspiring Aeronaut	|3|U	0x200dc89
   * Creature - Human Artificer 1/2
   * Flying
   * When ~ enters the battlefield, put a 1/1 colorless Thopter artifact creature token with flying onto the battlefield. */

  if (comes_into_play(player, card, event))
	generate_token_by_id(player, card, CARD_ID_THOPTER);

  return 0;
}

/* Bone to Ash	|2|U|U	=>dark_ascension.c:card_bone_to_ash
 * Instant
 * Counter target creature spell.
 * Draw a card. */

int card_calculated_dismissal(int player, int card, event_t event)
{
  /* Calculated Dismissal	|2|U	0x200dc8e
   * Instant
   * Counter target spell unless its controller pays |3.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, scry 2. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (counterspell_resolve_unless_pay_x(player, card, NULL, 0, 3) > 0
		  && spell_mastery(player))
		scry(player, 2);
	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}

  return counterspell(player, card, event, NULL, 0);
}

int card_clash_of_wills(int player, int card, event_t event)
{
  /* Clash of Wills	|X|U	0x200dc93
   * Instant
   * Counter target spell unless its controller pays |X. */

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	get_card_instance(player, card)->info_slot = x_value;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  counterspell_resolve_unless_pay_x(player, card, NULL, 0, get_card_instance(player, card)->info_slot);
	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}

  return counterspell(player, card, event, NULL, 0);
}

/* Claustrophobia	|1|U|U	=>innistrad.c:card_claustrophobia
 * Enchantment - Aura
 * Enchant creature
 * When ~ enters the battlefield, tap enchanted creature.
 * Enchanted creature doesn't untap during its controller's untap step. */

/* Day's Undoing	|2|U	0x000000
 * Sorcery
 * Each player shuffles his or her hand and graveyard into his or her library, then draws seven cards. If it's your turn, end the turn. */

int card_deep_sea_terror(int player, int card, event_t event)
{
  /* Deep-Sea Terror	|4|U|U	0x200dc98
   * Creature - Serpent 6/6
   * ~ can't attack unless there are seven or more cards in your graveyard. */

  if (event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && !is_humiliated(player, card)
	  && !(count_graveyard(player) >= 7))
    event_result = 1;

  return 0;
}

/* Disciple of the Ring	|3|U|U	0x000000
 * Creature - Human Wizard 3/4
 * |1, Exile an instant or sorcery card from your graveyard: Choose one -
 * * Counter target noncreature spell unless its controller pays |2.
 * * ~ gets +1/+1 until end of turn.
 * * Tap target creature.
 * * Untap target creature. */

/* Disperse	|1|U	=>morningtide.c:card_disperse
 * Instant
 * Return target nonland permanent to its owner's hand. */

int card_displacement_wave(int player, int card, event_t event)
{
  /* Displacement Wave	|X|U|U	0x200dc9d
   * Sorcery
   * Return all nonland permanents with converted mana cost X or less to their owners' hands. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "");
	  test.type_flag = DOESNT_MATCH;
	  test.cmc = get_card_instance(player, card)->info_slot + 1;
	  test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
	  APNAP(p, new_manipulate_all(player, card, p, &test, ACT_BOUNCE));
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

/* Dreadwaters	|3|U	=>avacyn_restored.c:card_dreadwaters
 * Sorcery
 * Target player puts the top X cards of his or her library into his or her graveyard, where X is the number of lands you control. */

int card_faerie_miscreant(int player, int card, event_t event)
{
  /* Faerie Miscreant	|U	0x200dca2
   * Creature - Faerie Rogue 1/1
   * Flying
   * When ~ enters the battlefield, if you control another creature named ~, draw a card. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller
	  && trigger_cause == card && trigger_cause_controller == player)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.id = get_id(player, card);
	  test.id_flag = MATCH;
	  test.not_me = 1;
	  if (check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test)
		  && comes_into_play(player, card, event))
		draw_a_card(player);
	}

  return 0;
}

int card_harbinger_of_the_tides(int player, int card, event_t event)
{
  /* Harbinger of the Tides	|U|U	0x200e21f
   * Creature - Merfolk Wizard 2/2
   * You may cast ~ as though it had flash if you pay |2 more to cast it.
   * When ~ enters the battlefield, you may return target tapped creature an opponent controls to its owner's hand. */

	spell_can_be_cast_as_instant_by_paying_2(player, card, event);

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.required_state = TARGET_STATE_TAPPED;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_next_target_noload(&td, "Select target tapped creature an opponent controls."))
		bounce_permanent(inst->targets[0].player, inst->targets[0].card);
	}

  return flash(player, card, event);
}

int card_hydrolash(int player, int card, event_t event)
{
  /* Hydrolash	|2|U	0x200dca7
   * Instant
   * Attacking creatures get -2/-0 until end of turn.
   * Draw a card. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  default_test_definition(&test, TYPE_CREATURE);
	  test.state = STATE_ATTACKING;
	  pump_creatures_until_eot(player, card, current_turn, 0, -2,0, 0,0, &test);

	  draw_a_card(player);

	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Jace, Vryn's Prodigy	|1|U	0x000000
 * Legendary Creature - Human Wizard 0/2
 * |T: Draw a card, then discard a card. If there are five or more cards in your graveyard, exile ~, then return him to the battlefield transformed under his owner's control. */

/* Jace, Telepath Unbound	""	0x000000
 * Planeswalker - Jace (5)
 * +1: Up to one target creature gets -2/-0 until your next turn.
 * -3: You may cast target instant or sorcery card from your graveyard this turn. If that card would be put into your graveyard this turn, exile it instead.
 * -9: You get an emblem with "Whenever you cast a spell, target opponent puts the top five cards of his or her library into his or her graveyard." */

int card_jaces_sanctum(int player, int card, event_t event)
{
  /* Jace's Sanctum	|3|U	0x200dcac
   * Enchantment
   * Instant and sorcery spells you cast cost |1 less to cast.
   * Whenever you cast an instant or sorcery spell, scry 1. */

  if (event == EVENT_MODIFY_COST_GLOBAL)
	{
	  if ((affected_card_controller == -1 ? event_result : affected_card_controller) == player
		  && is_what(affected_card_controller, affected_card, TYPE_INSTANT|TYPE_SORCERY) && !is_humiliated(player, card))
		COST_COLORLESS -= 1;
	  return 0;
	}

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_INSTANT|TYPE_SORCERY,MATCH, 0,0, 0,0, 0,0, -1,0))
	scry(player, 1);

  return global_enchantment(player, card, event);
}

int card_jhessian_thief(int player, int card, event_t event)
{
  /* Jhessian Thief	|2|U	0x200dcb1
   * Creature - Human Rogue 1/3
   * Prowess
   * Whenever ~ deals combat damage to a player, draw a card. */

  prowess(player, card, event);
  int packets = has_combat_damage_been_inflicted_to_a_player(player, card, event);
  if (packets > 0)
	draw_cards(player, packets);

  return 0;
}

/* Maritime Guard	|1|U	=>vanilla
 * Creature - Merfolk Soldier 1/3 */

/* Mizzium Meddler	|2|U	0x000000
 * Creature - Vedalken Wizard 1/4
 * Flash
 * When ~ enters the battlefield, you may change a target of target spell or ability to ~. */

/* Negate	|1|U	=>morningtide.c:card_negate
 * Instant
 * Counter target noncreature spell. */

int card_nivix_barrier(int player, int card, event_t event)
{
  /* Nivix Barrier	|3|U	0x200dcb6
   * Creature - Illusion Wall 0/4
   * Flash
   * Defender
   * When ~ enters the battlefield, target attacking creature gets -4/-0 until end of turn. */

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.required_state = TARGET_STATE_ATTACKING;
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_ATTACKING_CREATURE"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  pump_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, -4,0);
		}
	}

  return flash(player, card, event);
}

static const char* target_targets_you(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  card_instance_t* inst = get_card_instance(player, card);
  int i;
  for (i = 0; i < inst->number_of_targets; ++i)
	if (inst->targets[i].card == -1 && inst->targets[i].player == targeting_player)
	  return NULL;
  return "targets you";
}
int card_psychic_rebuttal(int player, int card, event_t event)
{
  /* Psychic Rebuttal	|1|U	0x200e229
   * Instant
   * Counter target instant or sorcery spell that targets you.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, you may copy the spell countered this way. You may choose new targets for the copy. */

  target_definition_t td;
  counterspell_target_definition(player, card, &td, 0);
  td.extra = (int)(&target_targets_you);
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (counterspell_validate(player, card, &td, 0))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  int iid = get_card_instance(inst->targets[0].player, inst->targets[0].card)->internal_card_id;
		  real_counter_a_spell(player, card, inst->targets[0].player, inst->targets[0].card);
		  if (spell_mastery(player)
			  && DIALOG(player, card, EVENT_ACTIVATE,
						DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_SMALLCARD_ID(iid),
						"Copy",		1, 5,
						"Decline",	1, 1) == 1)
			copy_spell_from_stack(player, inst->targets[0].player, inst->targets[0].card);
		}
	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}
  else
	return counterspell(player, card, event, &td, 0);
}

/* Ringwarden Owl	|3|U|U	=>khans_of_tarkir.c:card_jeskai_windscout
 * Creature - Bird 3/3
 * Flying
 * Prowess */

/* Scrapskin Drake	|2|U	=>m12.c:card_skywinder_drake
 * Creature - Zombie Drake 2/3
 * Flying
 * ~ can block only creatures with flying. */

/* Screeching Skaab	|1|U	=>dark_ascension.c:card_screeching_skaab
 * Creature - Zombie 2/1
 * When ~ enters the battlefield, put the top two cards of your library into your graveyard. */

int card_send_to_sleep(int player, int card, event_t event)
{
  /* Send to Sleep	|1|U	0x200dcbb
   * Instant
   * Tap up to two target creatures.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, those creatures don't untap during their controllers' next untap steps. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int i, mastery = spell_mastery(player);
	  for (i = 0; i < inst->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  {
			if (mastery)
			  effect_frost_titan(player, card, inst->targets[i].player, inst->targets[i].card);
			else
			  tap_card(inst->targets[i].player, inst->targets[i].card);
		  }

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

/* Separatist Voidmage	|3|U	=>return_to_ravnica.c:card_voidwielder
 * Creature - Human Wizard 2/2
 * When ~ enters the battlefield, you may return target creature to its owner's hand. */

/* Sigiled Starfish	|1|U	=>journey_into_nyx.c:card_sigiled_starfish
 * Creature - Starfish 0/3
 * |T: Scry 1. */

/* Skaab Goliath	|5|U	=>innistrad.c:card_skaab_goliath
 * Creature - Zombie Giant 6/9
 * As an additional cost to cast ~, exile two creature cards from your graveyard.
 * Trample */

/* Soulblade Djinn	|3|U|U	=>dragons_of_tarkir.c:card_strongarm_monk
 * Creature - Djinn 4/3
 * Flying
 * Whenever you cast a noncreature spell, creatures you control get +1/+1 until end of turn. */

int card_sphinxs_tutelage(int player, int card, event_t event)
{
  /* Sphinx's Tutelage	|2|U	0x200e22e
   * Enchantment
   * Whenever you draw a card, target opponent puts the top two cards of his or her library into his or her graveyard. If they're both nonland cards that share a color, repeat this process.
   * |5|U: Draw a card, then discard a card. */

  if (trigger_condition == TRIGGER_CARD_DRAWN && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == player
	  && opponent_is_valid_target(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  const int* library = deck_ptr[1-player];
		  int to_mill, added_color = get_global_color_hack(1-player);
		  for (to_mill = 2;
			   to_mill <= 498
				 && library[to_mill-2] != -1 && library[to_mill-1] != -1
				 && !is_what(-1, library[to_mill-2], TYPE_LAND) && !is_what(-1, library[to_mill-1], TYPE_LAND)
				 && (added_color
					 || (get_color_by_internal_id_no_hack(library[to_mill-2])
						 & get_color_by_internal_id_no_hack(library[to_mill-1])
						 & COLOR_TEST_ANY_COLORED));
			   to_mill += 2)
			{}

		  mill(1-player, to_mill);
		}
	}

  if (!IS_GAA_EVENT(event))
	return global_enchantment(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  draw_a_card(player);
	  discard(player, 0, player);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_XU(5,1), 0, NULL, NULL);
}

/* Stratus Walk	|1|U	=>born_of_the_gods.c:card_stratus_walk
 * Enchantment - Aura
 * Enchant creature
 * When ~ enters the battlefield, draw a card.
 * Enchanted creature has flying.
 * Enchanted creature can block only creatures with flying. */

/* Talent of the Telepath	|2|U|U	0x000000
 * Sorcery
 * Target opponent reveals the top seven cards of his or her library. You may cast an instant or sorcery card from among them without paying its mana cost. Then that player puts the rest into his or her graveyard.
 * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, you may cast up to two revealed instant and/or sorcery cards instead of one. */

int card_thopter_spy_network(int player, int card, event_t event)
{
  /* Thopter Spy Network	|2|U|U	0x200e233
   * Enchantment
   * At the beginning of your upkeep, if you control an artifact, put a 1/1 colorless Thopter artifact creature token with flying onto the battlefield.
   * Whenever one or more artifact creatures you control deal combat damage to a player, draw a card. */
  /* 6/22/2015	The last ability will trigger, at most, once per combat damage step. However, if at least one artifact creature you control has first strike and
   * others don’t, or if an artifact creature you control has double strike, the ability could trigger twice per combat: once in each combat damage step. */
  // I'd have thought it could trigger twice if some damage was redirected (since it says "to a player" instead of "to one or more players"), but whatever.

  if (trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && reason_for_trigger_controller == player
	  && count_subtype(player, TYPE_ARTIFACT, -1) >= 0)
	upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	generate_token_by_id(player, card, CARD_ID_THOPTER);

  card_instance_t* damage = combat_damage_being_dealt(event);
  if (damage
	  && damage->damage_target_card == -1
	  && (damage->targets[3].player & (TYPE_ARTIFACT|TYPE_CREATURE)) == (TYPE_ARTIFACT|TYPE_CREATURE)
	  && !damage_is_to_planeswalker(damage)
	  && !is_humiliated(player, card))
	get_card_instance(player, card)->info_slot = CARD_ID_THOPTER_SPY_NETWORK;

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->info_slot == CARD_ID_THOPTER_SPY_NETWORK)
		{
		  if (event == EVENT_TRIGGER)
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		  if (event == EVENT_RESOLVE_TRIGGER)
			{
			  inst->info_slot = 0;
			  draw_a_card(player);
			}
		}
	}

  return global_enchantment(player, card, event);
}

/* Tower Geist	|3|U	=>dark_ascension.c:card_tower_geist
 * Creature - Spirit 2/2
 * Flying
 * When ~ enters the battlefield, look at the top two cards of your library. Put one of them into your hand and the other into your graveyard. */

/* Turn to Frog	|1|U	=>m12.c:card_turn_into_frog
 * Instant
 * Until end of turn, target creature loses all abilities and becomes a |Sblue Frog with base power and toughness 1/1. */

/* Watercourser	|2|U	=>m13.c:card_watercourser
 * Creature - Elemental 2/3
 * |U: ~ gets +1/-1 until end of turn. */

int card_whirler_rogue(int player, int card, event_t event)
{
  /* Whirler Rogue	|2|U|U	0x200dd1f
   * Creature - Human Rogue Artificer 2/2
   * When ~ enters the battlefield, put two 1/1 colorless Thopter artifact creature tokens with flying onto the battlefield.
   * Tap two untapped artifacts you control: Target creature can't be blocked this turn. */

  if (comes_into_play(player, card, event))
	generate_tokens_by_id(player, card, CARD_ID_THOPTER, 2);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td_artifact;
  base_target_definition(player, card, &td_artifact, TYPE_ARTIFACT);
  td_artifact.allowed_controller = td_artifact.preferred_controller = player;
  td_artifact.illegal_state = TARGET_STATE_TAPPED;

  target_definition_t td_creature;
  default_target_definition(player, card, &td_creature, TYPE_CREATURE);
  td_creature.preferred_controller = player;

  if (event == EVENT_ACTIVATE && !tapsubtype_ability(player, card, 2, &td_artifact))
	{
	  cancel = 1;
	  return 0;
	}

  int rval = generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td_creature, "TARGET_CREATURE");

  if (event == EVENT_CAN_ACTIVATE && rval && target_available(player, card, &td_artifact) < 2)
	return 0;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td_creature))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 0,0, 0,SP_KEYWORD_UNBLOCKABLE);
	}

  return rval;
}

/* Willbreaker	|3|U|U	0x000000
 * Creature - Human Wizard 2/3
 * Whenever a creature an opponent controls becomes the target of a spell or ability you control, gain control of that creature for as long as you control ~. */

/*** Black ***/

/* Blightcaster	|3|B	=>m14.c:card_blightcaster
 * Creature - Human Wizard 2/3
 * Whenever you cast an enchantment spell, you may have target creature get -2/-2 until end of turn. */

/* Catacomb Slug	|4|B	=>vanilla
 * Creature - Slug 2/6 */

/* Consecrated by Blood	|2|B|B	0x000000
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature gets +2/+2 and has flying and "Sacrifice two other creatures: Regenerate this creature." */

/* Cruel Revival	|4|B	=>onslaught.c:card_cruel_revival
 * Instant
 * Destroy target non-Zombie creature. It can't be regenerated. Return up to one target Zombie card from your graveyard to your hand. */

int card_dark_dabbling(int player, int card, event_t event)
{
  /* Dark Dabbling	|2|B	0x200e238
   * Instant
   * Regenerate target creature. Draw a card.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, also regenerate each other creature you control. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;
  if (event != EVENT_RESOLVE_SPELL)	// so it still validates even if the target's no longer being destroyed at resolution
	{
	  td.required_state = TARGET_STATE_DESTROYED;
	  if (IS_AI(player))
		td.special = TARGET_SPECIAL_REGENERATION;
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (valid_target(&td))
		{
		  int pt = inst->targets[0].player, ct = inst->targets[0].card;
		  if (can_regenerate(pt, ct) && get_card_instance(pt, ct)->kill_code == KILL_DESTROY)
			regenerate_target(pt, ct);

		  if (spell_mastery(player))
			{
			  int c;
			  card_instance_t* cr;
			  for (c = 0; c < active_cards_count[player]; ++c)
				if (!(pt == player && ct == c)
					&& (cr = in_play(player, c))
					&& (cr->kill_code == KILL_DESTROY)
					&& is_what(player, c, TYPE_CREATURE)
					&& can_regenerate(player, c))
				  regenerate_target(player, c);
			}

		  draw_a_card(player);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET | GS_REGENERATION, &td, "TARGET_CREATURE", 1, NULL);
}

int card_dark_petition(int player, int card, event_t event)
{
  /* Dark Petition	|3|B|B	0x200dcc0
   * Sorcery
   * Search your library for a card and put that card into your hand. Then shuffle your library.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, add |B|B|B to your mana pool. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  default_test_definition(&test, TYPE_ANY);
	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);

	  if (spell_mastery(player))
		produce_mana(player, COLOR_BLACK, 3);

	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_deadbridge_shaman(int player, int card, event_t event)
{
  /* Deadbridge Shaman	|2|B	0x200dcc5
   * Creature - Elf Shaman 3/1
   * When ~ dies, target opponent discards a card. */

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) && opponent_is_valid_target(player, card))
	discard(1-player, 0, player);

  return 0;
}

int card_demonic_pact(int player, int card, event_t event)
{
  /* Demonic Pact	|2|B|B	0x200dcca
   * Enchantment
   * At the beginning of your upkeep, choose one that hasn't been chosen -
   * * ~ deals 4 damage to target creature or player and you gain 4 life.
   * * Target opponent discards two cards.
   * * Draw two cards.
   * * You lose the game. */
  /* 6/22/2015	The phrase "that hasn't been chosen" refers only to that specific Demonic Pact. If you control one and cast another one, you can choose any mode
   * for the second one the first time its ability triggers. */
  /* 6/22/2015	It doesn't matter who has chosen any particular mode. For example, say you control Demonic Pact and have chosen the first two modes. If an
   * opponent gains control of Demonic Pact, that player can choose only the third or fourth mode.*/
  /* 6/22/2015	In some very unusual situations, you may not be able to choose a mode, either because all modes have previously been chosen or the only
   * remaining modes require targets and there are no legal targets available. In this case, the ability is simply removed from the stack with no effect. */

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  card_instance_t* inst = get_card_instance(player, card);

	  target_definition_t td_creature_or_player;
	  default_target_definition(player, card, &td_creature_or_player, TYPE_CREATURE);
	  td_creature_or_player.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	  td_creature_or_player.allow_cancel = 0;

	  target_definition_t td_opponent;
	  default_target_definition(player, card, &td_opponent, 0);
	  td_opponent.allowed_controller = 1-player;
	  td_opponent.zone = TARGET_ZONE_PLAYERS;
	  td_opponent.allow_cancel = 0;

#define ALREADY(choice)	(inst->targets[choice+1].card == CARD_ID_DEMONIC_PACT)
	  enum
	  {
		CHOICE_DRAIN = 1,
		CHOICE_DISCARD,
		CHOICE_DRAW,
		CHOICE_LOSE
	  } choice = DIALOG(player, card, EVENT_ACTIVATE,
						DLG_RANDOM, DLG_NO_STORAGE, DLG_NO_CANCEL,
						"Drain life",	!ALREADY(CHOICE_DRAIN),		2,	DLG_TARGET(&td_creature_or_player, "TARGET_CREATURE_OR_PLAYER"),
						"Discard",		!ALREADY(CHOICE_DISCARD),	3,	DLG_TARGET(&td_opponent, "TARGET_OPPONENT"),
						"Draw",			!ALREADY(CHOICE_DRAW),		5,
						"Lose the game",!ALREADY(CHOICE_LOSE),		0);
#undef ALREADY

	  if (choice <= 0)
		return 0;

	  inst->targets[choice+1].card = CARD_ID_DEMONIC_PACT;
	  switch (choice)
		{
		  case CHOICE_DRAIN:
			damage_creature(inst->targets[0].player, inst->targets[0].card, 4, player, card);
			gain_life(player, 4);
			inst->number_of_targets = 0;
			break;

		  case CHOICE_DISCARD:
			new_multidiscard(inst->targets[0].player, 2, 0, player);
			inst->number_of_targets = 0;
			break;

		  case CHOICE_DRAW:
			draw_cards(player, 2);
			break;

		  case CHOICE_LOSE:
			lose_the_game(player);
			break;
		}
	}

  return global_enchantment(player, card, event);
}

/* Despoiler of Souls	|B|B	0x000000
 * Creature - Horror 3/1
 * ~ can't block.
 * |B|B, Exile two other creature cards from your graveyard: Return ~ from your graveyard to the battlefield. */

/* Erebos's Titan	|1|B|B|B	0x000000
 * Creature - Giant 5/5
 * As long as your opponents control no creatures, ~ has indestructible.
 * Whenever a creature card leaves an opponent's graveyard, you may discard a card. If you do, return ~ from your graveyard to your hand. */

int card_eyeblight_assassin(int player, int card, event_t event)
{
  /* Eyeblight Assassin	|2|B	0x200dccf
   * Creature - Elf Assassin 2/2
   * When ~ enters the battlefield, target creature an opponent controls gets -1/-1 until end of turn. */

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  pump_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, -1,-1);
		}
	}

  return 0;
}

int card_eyeblight_massacre(int player, int card, event_t event)
{
  /* Eyeblight Massacre	|2|B|B	0x200dcd4
   * Sorcery
   * Non-Elf creatures get -2/-2 until end of turn. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_ELF;
	  test.subtype_flag = DOESNT_MATCH;
	  pump_creatures_until_eot(player, card, ANYBODY, 0, -2,-2, 0,0, &test);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Fetid Imp	|1|B	=>morningtide.c:card_moonglove_changeling
 * Creature - Imp 1/2
 * Flying
 * |B: ~ gains deathtouch until end of turn. */

/* Fleshbag Marauder	|2|B	=>shards_of_alara.c:card_fleshbag_marauder
 * Creature - Zombie Warrior 3/1
 * When ~ enters the battlefield, each player sacrifices a creature. */

static const char* target_power_toughness_arent_equal(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  int pow = get_power(player, card);
  int tgh = get_toughness(player, card);
  return pow != tgh ? NULL : "power,toughness";
}
int card_gilt_leaf_winnower(int player, int card, event_t event)
{
  /* Gilt-Leaf Winnower	|3|B|B	0x200dcd9
   * Creature - Elf Warrior 4/3
   * Menace
   * When ~ enters the battlefield, you may destroy target non-Elf creature whose power and toughness aren't equal. */

  menace(player, card, event);

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.required_subtype = SUBTYPE_ELF;
	  td.extra = (int)target_power_toughness_arent_equal;
	  td.special = TARGET_SPECIAL_EXTRA_FUNCTION|TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	  if (can_target(&td) && new_pick_target(&td, "Select target non-Elf creature whose power and toughness aren't equal.", 0, GS_LITERAL_PROMPT))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		}
	}

  return 0;
}

/* Gnarlroot Trapper	|B	0x000000
 * Creature - Elf Druid 1/1
 * |T, Pay 1 life: Add |G to your mana pool. Spend this mana only to cast an Elf creature spell.
 * |T: Target attacking Elf you control gains deathtouch until end of turn. */

int card_graveblade_marauder(int player, int card, event_t event)
{
  /* Graveblade Marauder	|2|B	0x200dcde
   * Creature - Human Warrior 1/4
   * Deathtouch
   * Whenever ~ deals combat damage to a player, that player loses life equal to the number of creature cards in your graveyard. */

  deathtouch(player, card, event);

  if (damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER|DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_TRACE_DAMAGED_PLAYERS|RESOLVE_TRIGGER_MANDATORY))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int p, i, times_damaged[2] = { BYTE0(inst->targets[1].player), BYTE1(inst->targets[1].player) };
	  inst->targets[1].player = 0;
	  int creatures = count_graveyard_by_type(player, TYPE_CREATURE);
	  for (p = 0; p <= 1; ++p)
		for (i = 0; i < times_damaged[p]; ++i)
		  lose_life(p, creatures);
	}

  return 0;
}

int card_infernal_scarring(int player, int card, event_t event)
{
  /* Infernal Scarring	|1|B	0x200e251
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +2/+0 and has "When this creature dies, draw a card." */

  if (attached_creature_dies_trigger_for_controller(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	draw_a_card(player);

  return generic_aura(player, card, event, player, 2,0, 0,0, 0,0,0);
}

int card_infinite_obliteration(int player, int card, event_t event)
{
  /* Infinite Obliteration	|1|B|B	0x200e23d
   * Sorcery
   * Name a creature card. Search target opponent's graveyard, hand, and library for any number of cards with that name and exile them. Then that player shuffles his or her library. */

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
		  int iid;
		  if (!IS_AI(player))
			{
			  do
				{
				  iid = choose_a_card("Name a creature card.", -1, TYPE_CREATURE);
				} while (!is_what(-1, iid, TYPE_CREATURE) || !is_valid_card(cards_data[iid].id));
			}
		  else
			{
			  /* In keeping with the usual cheating ways here, pick the first creature card in target's library.  If none there, first in graveyard. */
			  iid = -1;
			  int i;
			  for (i = 0; i < 500 && deck_ptr[1-player][i] != -1; ++i)
				if (is_what(-1, deck_ptr[1-player][i], TYPE_CREATURE))
				  iid = deck_ptr[1-player][i];

			  if (iid == -1)
				{
				  const int* gy = get_grave(1-player);
				  for (i = 0; i < 500 && gy[i] != -1; ++i)
					if (is_what(-1, gy[i], TYPE_CREATURE))
					  iid = gy[i];

				  if (iid == -1)
					iid = get_internal_card_id_from_csv_id(CARD_ID_AIR_ELEMENTAL);
				}

			  DIALOG(player, card, EVENT_ACTIVATE,
					 DLG_NO_STORAGE, DLG_NO_CANCEL, DLG_SMALLCARD_ID(iid),
					 get_card_name_by_id(cards_data[iid].id), 1, 1);

			  if (player == AI)
				ai_modifier += 100;	// So it casts it at all
			}

		  lobotomy_effect(player, 1-player, cards_data[iid].id, 0);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_OPPONENT", 1, NULL);
}

int card_kothophed_soul_hoarder(int player, int card, event_t event)
{
  /* Kothophed, Soul Hoarder	|4|B|B	0x200dce3
   * Legendary Creature - Demon 6/6
   * Flying
   * Whenever a permanent owned by another player is put into a graveyard from the battlefield, you draw a card and you lose 1 life. */

  check_legend_rule(player, card, event);

  if (event == EVENT_GRAVEYARD_FROM_PLAY && get_owner(affected_card_controller, affected_card) != player)
	count_for_gfp_ability(player, card, event, ANYBODY, TYPE_PERMANENT, NULL);

  int num;
  for (num = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY); num > 0; --num)
	{
	  draw_a_card(player);
	  lose_life(player, 1);	// separate lose-life triggers, if anything's watching
	}

  return 0;
}

int card_languish(int player, int card, event_t event)
{
  /* Languish	|2|B|B	0x200dce8
   * Sorcery
   * All creatures get -4/-4 until end of turn. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  pump_creatures_until_eot(player, card, ANYBODY, 0, -4,-4, 0,0, NULL);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_liliana_heretical_healer(int player, int card, event_t event)
{
  /* Liliana, Heretical Healer	|1|B|B	0x200e256
   * Legendary Creature - Human Cleric 2/3
   * Lifelink
   * Whenever another nontoken creature you control dies, exile ~, then return her to the battlefield transformed under her owner's control. If you do, put a 2/2 |Sblack Zombie creature token onto the battlefield. */

  double_faced_card(player, card, event);
  check_legend_rule(player, card, event);
  lifelink(player, card, event);

  if (event == EVENT_GRAVEYARD_FROM_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.type_flag = F1_NO_TOKEN;
	  test.not_me = 1;
	  count_for_gfp_ability(player, card, event, player, 0, &test);
	}

  if (resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  exile_and_return_to_bf_transformed(player, card);
	  generate_token_by_id(player, card, CARD_ID_ZOMBIE);
	}

  return 0;
}
int card_liliana_defiant_necromancer(int player, int card, event_t event)
{
  /* Liliana, Defiant Necromancer	""	0x200e25b
   * Planeswalker - Liliana (3)
   * +2: Each player discards a card.
   * -X: Return target nonlegendary creature card with converted mana cost X from your graveyard to the battlefield.
   * -8: You get an emblem with "Whenever a creature dies, return it to the battlefield under your control at the beginning of the next end step." */

  if (IS_ACTIVATING(event))
	{
	  int loyalty = count_counters(player, card, COUNTER_LOYALTY);

	  int can_resurrect = 0;
	  test_definition_t test;
	  if (event != EVENT_RESOLVE_ACTIVATION)
		{
		  new_default_test_definition(&test, TYPE_CREATURE, "");
		  test.cmc = loyalty + 1;
		  test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		  test.subtype = SUBTYPE_LEGEND;
		  test.subtype_flag = DOESNT_MATCH;

		  can_resurrect = !graveyard_has_shroud(player) && new_special_count_grave(player, &test) > 0;
		}

	  int should_discard = hand_count[1-player] <= 0 ? 0 : (hand_count[player] <= 0 ? 4 : 2);

	  card_instance_t* inst = get_card_instance(player, card);

	  enum
	  {
		CHOICE_DISCARD = 1,
		CHOICE_RESURRECT,
		CHOICE_EMBLEM,
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Discard",					1,				should_discard,	+2,
						"Graveyard to battlefield",	can_resurrect,	1,				0,
						"Emblem",					1,				8,				-8);
	  if (event == EVENT_CAN_ACTIVATE && !choice)
		return 0;
	  if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_RESURRECT:
			  if (ai_is_speculating != 1)
				sprintf(test.message, "Select target nonlegendary creature card with converted mana cost %d or less.", loyalty);
			  int iid;
			  if ((iid = select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &test, 0)) != -1)
				{
				  int cost = -get_cmc_by_internal_id(iid);
				  SET_SBYTE1(inst->targets[9].player) = cost;
				  if (loyalty + cost <= 0)
					ai_modifier -= 256;
				}

			  break;

			case CHOICE_DISCARD:
			case CHOICE_EMBLEM:
			  break;
		  }
	  if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_DISCARD:
			  discard(player, 0, player);
			  discard(1-player, 0, player);
			  break;

			case CHOICE_RESURRECT:
			  ;int gy_pos = validate_target_from_grave_source(player, card, player, 0);
			  if (gy_pos != -1)
				reanimate_permanent(player, card, player, gy_pos, REANIMATE_DEFAULT);
			  break;

			case CHOICE_EMBLEM:
			  generate_token_by_id(player, card, CARD_ID_LILIANA_DEFIANT_NECROMANCER_EMBLEM);
			  break;
		  }
	}

  return planeswalker(player, card, event, 3);
}
static int fx_liliana_defiant_necromancer_emblem(int player, int card, event_t event)
{
  if (event == EVENT_SET_LEGACY_EFFECT_NAME)
	scnprintf(set_legacy_effect_name_addr, 52, cards_ptr[get_card_instance(player, card)->targets[1].player]->full_name);

  if (eot_trigger(player, card, event))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int owner, position;
	  if (find_in_owners_graveyard(inst->targets[0].player, inst->targets[0].card, &owner, &position))
		reanimate_permanent(player, card, owner, position, REANIMATE_DEFAULT);

	  kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}
int card_liliana_defiant_necromancer_emblem(int player, int card, event_t event)
{
  /* Liliana, Defiant Necromancer Emblem	""	0x200e260
   * Emblem
   * Whenever a creature dies, return it to the battlefield under your control at the beginning of the next end step. */

  card_instance_t* aff;
  if (event == EVENT_GRAVEYARD_FROM_PLAY
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && !is_token(affected_card_controller, affected_card)
	  && (aff = get_card_instance(affected_card_controller, affected_card))->kill_code != KILL_REMOVE
	  // You'd think with data space in card_instance_t at such a premium, you wouldn't need a *fourth* way of making the below effect
	  && !check_special_flags2(affected_card_controller, affected_card, SF2_WILL_BE_EXILED_IF_PUTTED_IN_GRAVE))
	{
	  card_instance_t* legacy = get_card_instance(player, create_targetted_legacy_effect(player, card, fx_liliana_defiant_necromancer_emblem, player, card));
	  legacy->targets[0].player = affected_card_controller;
	  legacy->targets[0].card = affected_card;
	  legacy->targets[1].player = cards_data[get_card_instance(affected_card_controller, affected_card)->original_internal_card_id].id;
	  legacy->eot_toughness = EVENT_SET_LEGACY_EFFECT_NAME;
	}

  return 0;
}

/* Macabre Waltz	|1|B	=>dissension.c:card_macabre_waltz
 * Sorcery
 * Return up to two target creature cards from your graveyard to your hand, then discard a card. */

int card_malakir_cullblade(int player, int card, event_t event)
{
  /* Malakir Cullblade	|1|B	0x200dced
   * Creature - Vampire Warrior 1/1
   * Whenever a creature an opponent controls dies, put a +1/+1 counter on ~. */

  count_for_gfp_ability(player, card, event, 1-player, TYPE_CREATURE, NULL);

  int num;
  for (num = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY); num > 0; --num)
	add_1_1_counter(player, card);	// separate add-a-counter triggers, if anything's watching

  return 0;
}

/* Nantuko Husk	|2|B	=>onslaught.c:card_nantuko_husk
 * Creature - Zombie Insect 2/2
 * Sacrifice a creature: ~ gets +2/+2 until end of turn. */

static void etb_with_2_1_1_counters(int player, int card)
{
  ++hack_silent_counters;
  add_1_1_counters(player, card, 2);
  --hack_silent_counters;
}
int card_necromantic_summons(int player, int card, event_t event)
{
  /* Necromantic Summons	|4|B	0x200dcf2
   * Sorcery
   * Put target creature card from a graveyard onto the battlefield under your control.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, that creature enters the battlefield with two additional +1/+1 counters on it. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Selet target creature card.");

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int gy_pos = validate_target_from_grave_source(player, card, inst->targets[0].player, 1);
	  if (gy_pos != -1)
		reanimate_permanent_with_function(player, card, inst->targets[0].player, gy_pos, REANIMATE_DEFAULT,
										  spell_mastery(player) ? &etb_with_2_1_1_counters : NULL);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_GRAVE_RECYCLER_BOTH_GRAVES, NULL, NULL, 0, &test);
}

/* Nightsnare	|3|B	0x000000
 * Sorcery
 * Target opponent reveals his or her hand. You may choose a nonland card from it. If you do, that player discards that card. If you don't, that player discards two cards. */

int card_priest_of_the_blood_rite(int player, int card, event_t event)
{
  /* Priest of the Blood Rite	|3|B|B	0x200dcf7
   * Creature - Human Cleric 2/2
   * When ~ enters the battlefield, put a 5/5 |Sblack Demon creature token with flying onto the battlefield.
   * At the beginning of your upkeep, you lose 2 life. */

  if (comes_into_play(player, card, event))
	generate_token_by_id(player, card, CARD_ID_DEMON);

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	lose_life(player, 2);

  return 0;
}

int card_rabid_bloodsucker(int player, int card, event_t event)
{
  /* Rabid Bloodsucker	|4|B	0x200dcfc
   * Creature - Vampire 3/2
   * Flying
   * When ~ enters the battlefield, each player loses 2 life. */

  if (comes_into_play(player, card, event))
	APNAP(p, lose_life(p, 2));

  return 0;
}

/* Read the Bones	|2|B	=>theros.c:card_read_the_bones
 * Sorcery
 * Scry 2, then draw two cards. You lose 2 life. */

int card_reave_soul(int player, int card, event_t event)
{
  /* Reave Soul	|1|B	0x200dd01
   * Sorcery
   * Destroy target creature with power 3 or less. */

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
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_LITERAL_PROMPT, &td, "Select target creature with power 3 or less.", 1, NULL);
}

/* Returned Centaur	|3|B	=>theros.c:card_returned_centaur
 * Creature - Zombie Centaur 2/4
 * When ~ enters the battlefield, target player puts the top four cards of his or her library into his or her graveyard. */

/* Revenant	|4|B	=>stronghold.c:card_revenant
 * Creature - Spirit 100/100
 * Flying
 * ~'s power and toughness are each equal to the number of creature cards in your graveyard. */

int card_shadows_of_the_past(int player, int card, event_t event)
{
  /* Shadows of the Past	|1|B	0x200dd06
   * Enchantment
   * Whenever a creature dies, scry 1.
   * |4|B: Each opponent loses 2 life and you gain 2 life. Activate this ability only if there are four or more creature cards in your graveyard. */

  count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);

  int num;
  for (num = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY); num > 0; --num)
	scry(player, 1);

  if (IS_GAA_EVENT(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !(count_graveyard_by_type(player, TYPE_CREATURE) >= 4))
		return 0;

	  if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  lose_life(1-player, 2);
		  gain_life(player, 2);
		}

	  return generic_activated_ability(player, card, event, 0, MANACOST_XB(4,1), 0, NULL, NULL);
	}

  return global_enchantment(player, card, event);
}

/* Shambling Ghoul	|1|B	=>m11.c:card_rotting_legion
 * Creature - Zombie 2/3
 * ~ enters the battlefield tapped. */

int card_tainted_remedy(int player, int card, event_t event)
{
  /* Tainted Remedy	|2|B	0x200dd0b
   * Enchantment
   * If an opponent would gain life, that player loses that much life instead. */

  // Uses the usual hack in gain_life() instead of doing things right.
  return global_enchantment(player, card, event);
}

int card_thornbow_archer(int player, int card, event_t event)
{
  /* Thornbow Archer	|B	0x200dd10
   * Creature - Elf Archer 1/2
   * Whenever ~ attacks, each opponent who doesn't control an Elf loses 1 life. */

  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card)
	  && !check_battlefield_for_subtype(1-player, TYPE_PERMANENT, SUBTYPE_ELF))
	lose_life(1-player, 1);

  return 0;
}

/* Tormented Thoughts	|2|B	=>journey_into_nyx.c:card_tormented_thoughts
 * Sorcery
 * As an additional cost to cast ~, sacrifice a creature.
 * Target player discards a number of cards equal to the sacrificed creature's power. */

/* Touch of Moonglove	|B	0x000000
 * Instant
 * Target creature you control gets +1/+0 and gains deathtouch until end of turn. Whenever a creature dealt damage by that creature dies this turn, its controller loses 2 life. */

int card_undead_servant(int player, int card, event_t event)
{
  /* Undead Servant	|3|B	0x200dd15
   * Creature - Zombie 3/2
   * When ~ enters the battlefield, put a 2/2 |Sblack Zombie creature token onto the battlefield for each card named ~ in your graveyard. */

  if (comes_into_play(player, card, event))
	{
	  int n = count_graveyard_by_id(player, get_id(player, card));
	  if (n > 0)
		generate_tokens_by_id(player, card, CARD_ID_ZOMBIE, n);
	}

  return 0;
}

int card_unholy_hunger(int player, int card, event_t event)
{
  /* Unholy Hunger	|3|B|B	0x200dd1a
   * Instant
   * Destroy target creature.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, you gain 2 life. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  kill_card(inst->targets[0].player, inst->targets[0].card, KILL_DESTROY);
		  if (spell_mastery(player))
			gain_life(player, 2);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/* Weight of the Underworld	|3|B	=>born_of_the_gods.c:card_weight_of_the_underworld
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature gets -3/-2. */

/*** Red ***/

int card_abbot_of_keral_keep(int player, int card, event_t event)
{
  /* Abbot of Keral Keep	|1|R	0x200e283
   * Creature - Human Monk 2/1
   * Prowess
   * When ~ enters the battlefield, exile the top card of your library. Until end of turn, you may play that card. */

  prowess(player, card, event);

  int iid;
  if (comes_into_play(player, card, event) && (iid = deck_ptr[player][0]) != -1)
	{
	  rfg_top_card_of_deck(player);
	  create_may_play_card_from_exile_effect(player, card, player, cards_data[iid].id, MPCFE_UNTIL_EOT);
	}

  return 0;
}

static void block_deal2dam(int player, int card, int blocking_player, int blocking_card)
{
  damage_creature(blocking_player, blocking_card, 2, player, card);
}
int card_acolyte_of_the_inferno(int player, int card, event_t event)
{
  /* Acolyte of the Inferno	|2|R	0x200dd24
   * Creature - Human Monk 3/1
   * Renown 1
   * Whenever ~ becomes blocked by a creature, it deals 2 damage to that creature. */

  renown(player, card, event, 1);
  if (event == EVENT_DECLARE_BLOCKERS && current_turn == player && !is_humiliated(player, card))
	for_each_creature_blocking_me(player, card, block_deal2dam, player, card);
  return 0;
}

/* Act of Treason	|2|R	=>m10.c:card_act_of_treason
 * Sorcery
 * Gain control of target creature until end of turn. Untap that creature. It gains haste until end of turn. */

/* Akroan Sergeant	|2|R	=>card_knight_of_the_pilgrims_road
 * Creature - Human Soldier 2/2
 * First strike
 * Renown 1 */

/* Avaricious Dragon	|2|R|R	=>urza_saga.c:card_grafted_skullcap
 * Creature - Dragon 4/4
 * Flying
 * At the beginning of your draw step, draw an additional card.
 * At the beginning of your end step, discard your hand. */

/* Bellows Lizard	|R	=>return_to_ravnica.c:card_bellows_lizard
 * Creature - Lizard 1/1
 * |1|R: ~ gets +1/+0 until end of turn. */

/* Boggart Brute	|2|R	=>gatecrash.c:card_ripscale_predators
 * Creature - Goblin Warrior 3/2
 * Menace */

int card_call_of_the_full_moon(int player, int card, event_t event)
{
  /* Call of the Full Moon	|1|R	0x200e265
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +3/+2 and has trample.
   * At the beginning of each upkeep, if a player cast two or more spells last turn, sacrifice ~. */

  if (event == EVENT_PHASE_CHANGED && current_phase == PHASE_CLEANUP)	// immediately before EVENT_CLEANUP, during which storm counts become invalid.
	get_card_instance(player, card)->info_slot = ((get_specific_storm_count(player) >= 2 || get_specific_storm_count(1-player) >= 2)
												  ? CARD_ID_CALL_OF_THE_FULL_MOON : 0);

  if (trigger_condition == TRIGGER_UPKEEP && affect_me(player, card)
	  && get_card_instance(player, card)->info_slot == CARD_ID_CALL_OF_THE_FULL_MOON
	  && upkeep_trigger(player, card, event))
	kill_card(player, card, KILL_SACRIFICE);

  return generic_aura(player, card, event, player, 3,2, KEYWORD_TRAMPLE,0, 0,0,0);
}

int card_chandra_fire_of_kaladesh(int player, int card, event_t event)
{
  /* Chandra, Fire of Kaladesh	|1|R|R	0x200e26a
   * Legendary Creature - Human Shaman 2/2
   * Whenever you cast a |Sred spell, untap ~.
   * |T: ~ deals 1 damage to target player. If ~ has dealt 3 or more damage this turn, exile her, then return her to the battlefield transformed under her owner's control. */

  double_faced_card(player, card, event);
  check_legend_rule(player, card, event);

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, 0,0, 0,0, COLOR_TEST_RED,MATCH, 0,0, -1,0))
	untap_card(player, card);

  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && damage->damage_source_card == card && damage->damage_source_player == player)
	get_card_instance(player, card)->info_slot += damage->targets[16].player;

  if (event == EVENT_CLEANUP)
	get_card_instance(player, card)->info_slot = 0;

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int p = inst->parent_controller, c = inst->parent_card;
	  int dmg_this_turn = get_card_instance(p, c)->info_slot;
	  damage_target0(player, card, 1);
	  if (dmg_this_turn + 1 >= 3 && in_play(p, c))
		exile_and_return_to_bf_transformed(p, c);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_UNTAPPED, MANACOST0, 0, &td, "TARGET_PLAYER");
}
int card_chandra_roaring_flame(int player, int card, event_t event)
{
  /* Chandra, Roaring Flame	""	0x200e26f
   * Planeswalker - Chandra (4)
   * +1: ~ deals 2 damage to target player.
   * -2: ~ deals 2 damage to target creature.
   * -7: ~ deals 6 damage to each opponent. Each player dealt damage this way gets an emblem with "At the beginning of your upkeep, this emblem deals 3 damage to you." */

  if (IS_ACTIVATING(event))
	{
	  target_definition_t td_player;
	  default_target_definition(player, card, &td_player, 0);
	  td_player.zone = TARGET_ZONE_PLAYERS;

	  target_definition_t td_creature;
	  default_target_definition(player, card, &td_creature, TYPE_CREATURE);

	  enum
	  {
		CHOICE_PLAYER = 1,
		CHOICE_CREATURE,
		CHOICE_OPPONENT,
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"2 to player",		1,	2,	+1,	DLG_TARGET(&td_player, "TARGET_PLAYER"),
						"2 to creature",	1,	1,	-2,	DLG_TARGET(&td_creature, "TARGET_CREATURE"),
						"6 to opponent",	1,	4,	-7);
	  if (event == EVENT_CAN_ACTIVATE && !choice)
		return 0;
	  if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_PLAYER:
			  if (valid_target(&td_player))
				damage_target0(player, card, 2);
			  break;

			case CHOICE_CREATURE:
			  if (valid_target(&td_creature))
				damage_target0(player, card, 2);
			  break;

			case CHOICE_OPPONENT:
			{
				card_instance_t* inst = get_card_instance(player, card);
				int dmg_card = damage_player(1-player, 6, player, card);
				card_instance_t* dmg = get_card_instance(1-player, dmg_card);	// Weirdly, damage dealt to a player is "controlled" by that player
				ASSERT(dmg->damage_source_player == inst->parent_controller);
				ASSERT(dmg->damage_source_card == inst->parent_card);
				ASSERT(dmg->internal_card_id == damage_card);
				dmg->targets[10].card = CARD_ID_CHANDRA_ROARING_FLAME;
				//The emblem is generated inside of "effect_damage" in "damage_effects.c"
			}
			break;
		  }
	}
	return planeswalker(player, card, event, 4);
}
int card_chandra_roaring_flame_emblem(int player, int card, event_t event)
{
  /* Chandra, Roaring Flame Emblem	""	0x200e274
   * Emblem
   * At the beginning of your upkeep, this emblem deals 3 damage to you. */

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	damage_player(player, 3, player, card);

  return 0;
}

/* Chandra's Fury	|4|R	=>m13.c:card_chandras_fury
 * Instant
 * ~ deals 4 damage to target player and 1 damage to each creature that player controls. */

int card_chandras_ignition(int player, int card, event_t event)
{
  /* Chandra's Ignition	|3|R|R	0x200dd29
   * Sorcery
   * Target creature you control deals damage equal to its power to each other creature and each opponent. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  int pow = get_power(inst->targets[0].player, inst->targets[0].card);
		  damage_player(1-player, pow, inst->targets[0].player, inst->targets[0].card);
		  new_damage_all(inst->targets[0].player, inst->targets[0].card, ANYBODY, pow, NDA_NOT_TO_ME, NULL);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "ASHNODS_BATTLEGEAR", 1, NULL);	// "Select target creature you control."
}

/* Cobblebrute	|3|R	=>vanilla
 * Creature - Elemental 5/2 */

/* Demolish	|3|R	=>odissey.c:card_demolish
 * Sorcery
 * Destroy target artifact or land. */

/* Dragon Fodder	|1|R	=>shards_of_alara.c:card_dragon_fodder
 * Sorcery
 * Put two 1/1 |Sred Goblin creature tokens onto the battlefield. */

int card_embermaw_hellion(int player, int card, event_t event)
{
  /* Embermaw Hellion	|3|R|R	0x200e28d
   * Creature - Hellion 4/5
   * Trample
   * If another |Sred source you control would deal damage to a permanent or player, it deals that much damage plus 1 to that permanent or player instead. */

  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && damage->damage_source_player == player
	  && damage->damage_source_card != card
	  && (damage->initial_color & get_sleighted_color_test(player, card, COLOR_TEST_RED))
	  && !is_humiliated(player, card))
	damage->info_slot += 1;

  return 0;
}

int card_enthralling_victor(int player, int card, event_t event)
{
  /* Enthralling Victor	|3|R	0x200dd2e
   * Creature - Human Warrior 3/2
   * When ~ enters the battlefield, gain control of target creature an opponent controls with power 2 or less until end of turn. Untap that creature. It gains haste until end of turn. */

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;
	  td.allow_cancel = 0;

	  if (can_target(&td) && new_pick_target(&td, "Select target creature an opponent controls with power 2 or less.", 0, GS_LITERAL_PROMPT))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  effect_act_of_treason(player, card, inst->targets[0].player, inst->targets[0].card);
		}
	}

  return 0;
}

int card_exquisite_firecraft(int player, int card, event_t event)
{
  /* Exquisite Firecraft	|1|R|R	0x200dd33
   * Sorcery
   * ~ deals 4 damage to target creature or player.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, ~ can't be countered by spells or abilities. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  if (spell_mastery(player))
	cannot_be_countered(player, card, event);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, 4);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_fiery_impulse(int player, int card, event_t event)
{
  /* Fiery Impulse	|R	0x200dd3d
   * Instant
   * ~ deals 2 damage to target creature.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, ~ deals 3 damage to that creature instead. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, spell_mastery(player) ? 3 : 2);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_firefiend_elemental(int player, int card, event_t event)
{
  /* Firefiend Elemental	|3|R	0x200dd42
   * Creature - Elemental 3/2
   * Haste
   * Renown 1 */

  haste(player, card, event);
  renown(player, card, event, 1);
  return 0;
}

int card_flameshadow_conjuring(int player, int card, event_t event)
{
  /* Flameshadow Conjuring	|3|R	0x200dd47
   * Enchantment
   * Whenever a nontoken creature enters the battlefield under your control, you may pay |R. If you do, put a token onto the battlefield that's a copy of that creature. That token gains haste. Exile it at the beginning of the next end step. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && player == reason_for_trigger_controller)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.type_flag = F1_NO_TOKEN;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &test)
		  && has_mana(player, COLOR_RED, 1)
		  && charge_mana_while_resolving(player, card, event, player, COLOR_RED, 1))
		{
		  token_generation_t token;
		  copy_token_definition(player, card, &token, trigger_cause_controller, trigger_cause);
		  token.legacy = 1;
		  token.special_code_for_legacy = &haste_and_remove_eot;
		  generate_token(&token);
		}
	}

  return global_enchantment(player, card, event);
}

int card_ghirapur_aether_grid(int player, int card, event_t event)
{
  /* Ghirapur AEther Grid	|2|R	0x200dd4c
   * Enchantment
   * Tap two untapped artifacts you control: ~ deals 1 damage to target creature or player. */

  if (!IS_GAA_EVENT(event))
	return global_enchantment(player, card, event);

  target_definition_t td_artifact;
  base_target_definition(player, card, &td_artifact, TYPE_ARTIFACT);
  td_artifact.allowed_controller = td_artifact.preferred_controller = player;
  td_artifact.illegal_state = TARGET_STATE_TAPPED;

  target_definition_t td_creature_or_player;
  default_target_definition(player, card, &td_creature_or_player, TYPE_CREATURE);
  td_creature_or_player.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_ACTIVATE && !tapsubtype_ability(player, card, 2, &td_artifact))
	{
	  cancel = 1;
	  return 0;
	}

  int rval = generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td_creature_or_player, "TARGET_CREATURE_OR_PLAYER");

  if (event == EVENT_CAN_ACTIVATE && rval && target_available(player, card, &td_artifact) < 2)
	return 0;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td_creature_or_player))
	damage_target0(player, card, 1);

  return rval;
}

/* Ghirapur Gearcrafter	|2|R	=>card_aspiring_aeronaut
 * Creature - Human Artificer 2/1
 * When ~ enters the battlefield, put a 1/1 colorless Thopter artifact creature token with flying onto the battlefield. */

int card_goblin_glory_chaser(int player, int card, event_t event)
{
  /* Goblin Glory Chaser	|R	0x200dd51
   * Creature - Goblin Warrior 1/1
   * Renown 1
   * As long as ~ is renowned, it has menace. */

  if (is_renowned(player, card) && !is_humiliated(player, card))
    menace(player, card, event);

  return 0;
}

/* Goblin Piledriver	|1|R	=>onslaught.c:card_goblin_piledriver
 * Creature - Goblin Warrior 1/2
 * Protection from |Sblue
 * Whenever ~ attacks, it gets +2/+0 until end of turn for each other attacking Goblin. */

int card_infectious_bloodlust(int player, int card, event_t event)
{
  /* Infectious Bloodlust	|1|R	0x200dd56
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +2/+1, has haste, and attacks each turn if able.
   * When enchanted creature dies, you may search your library for a card named ~, reveal it, put it into your hand, then shuffle your library. */

  if (attached_creature_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  // Although effect_this_dies tries reasonably hard to hide it, it's still an effect card; get_id() will return 903.
	  int csvid = get_card_instance(player, card)->display_pic_csv_id;
	  char prompt[100];
	  sprintf(prompt, "Select a card named %s.", cards_ptr[csvid]->full_name);

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ANY, prompt);
	  test.id = csvid;
	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &test);
	}

  return generic_aura(player, card, event, player, 2,1, 0,SP_KEYWORD_HASTE|SP_KEYWORD_MUST_ATTACK, 0,0,0);
}

/* Lightning Javelin	|3|R	born_of_the_gods.c:card_bolt_of_keranos
 * Sorcery
 * ~ deals 3 damage to target creature or player. Scry 1. */

int card_mage_ring_bully(int player, int card, event_t event)
{
  /* Mage-Ring Bully	|1|R	0x200dd5b
   * Creature - Human Warrior 2/2
   * Prowess
   * ~ attacks each turn if able. */

  prowess(player, card, event);
  attack_if_able(player, card, event);
  return 0;
}

int card_magmatic_insight(int player, int card, event_t event)
{
  /* Magmatic Insight	|R	0x200dd60
   * Sorcery
   * As an additional cost to cast ~, discard a land card.
   * Draw two cards. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_LAND, "Select a land card to discard.");
  test.zone = TARGET_ZONE_HAND;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  draw_cards(player, 2);
	  kill_card(player, card, KILL_DESTROY);
	}

  int rval = basic_spell(player, card, event);

  if (event == EVENT_CAN_CAST && rval && !check_battlefield_for_special_card(player, card, player, 0, &test))
	return 0;

  if (event == EVENT_CAST_SPELL && affect_me(player, card) && cancel != 1)
	{
	  int land = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &test);
	  if (land == -1)
		cancel = 1;
	  else
		discard_card(player, land);
	}

  return rval;
}

int card_molten_vortex(int player, int card, event_t event)
{
  /* Molten Vortex	|R	0x200dd65
   * Enchantment
   * |R, Discard a land card: ~ deals 2 damage to target creature or player. */

  if (!IS_GAA_EVENT(event))
	return global_enchantment(player, card, event);

  test_definition_t test_land;
  new_default_test_definition(&test_land, TYPE_LAND, "Select a land card to discard.");
  test_land.zone = TARGET_ZONE_HAND;

  target_definition_t td_creature_or_player;
  default_target_definition(player, card, &td_creature_or_player, TYPE_CREATURE);
  td_creature_or_player.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td_creature_or_player))
	damage_target0(player, card, 2);

  int rval = generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_R(1), 0, &td_creature_or_player, "TARGET_CREATURE_OR_PLAYER");

  if (event == EVENT_CAN_ACTIVATE && rval && !check_battlefield_for_special_card(player, card, player, 0, &test_land))
	return 0;

  if (event == EVENT_ACTIVATE && cancel != 1)
	{
	  int land = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &test_land);
	  if (land == -1)
		cancel = 1;
	  else
		discard_card(player, land);
	}

  return rval;
}

int card_pia_and_kiran_nalaar(int player, int card, event_t event)
{
  /* Pia and Kiran Nalaar	|2|R|R	0x200dd6a
   * Legendary Creature - Human Artificer 2/2
   * When ~ enters the battlefield, put two 1/1 colorless Thopter artifact creature tokens with flying onto the battlefield.
   * |2|R, Sacrifice an artifact: ~ deals 2 damage to target creature or player. */

  check_legend_rule(player, card, event);

  if (comes_into_play(player, card, event))
	generate_tokens_by_id(player, card, CARD_ID_THOPTER, 2);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td_creature_or_player;
  default_target_definition(player, card, &td_creature_or_player, TYPE_CREATURE);
  td_creature_or_player.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td_creature_or_player))
	damage_target0(player, card, 2);

  int rval = generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XR(2,1), 0, &td_creature_or_player, "TARGET_CREATURE_OR_PLAYER");

  if (event == EVENT_CAN_ACTIVATE && rval && !can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT,MATCH, 0,0, 0,0, 0,0, -1,0))
	return 0;

  if (event == EVENT_ACTIVATE && cancel != 1 && !controller_sacrifices_a_permanent(player, card, TYPE_ARTIFACT, 0))
	cancel = 1;

  return rval;
}

int card_prickleboar(int player, int card, event_t event)
{
  /* Prickleboar	|4|R	0x200dd6f
   * Creature - Boar 3/3
   * As long as it's your turn, ~ gets +2/+0 and has first strike. */

  if (event == EVENT_POWER && affect_me(player, card) && current_turn == player && !is_humiliated(player, card))
	event_result += 2;

  if (event == EVENT_ABILITIES && affect_me(player, card) && current_turn == player && !is_humiliated(player, card))
	event_result |= KEYWORD_FIRST_STRIKE;

  return 0;
}

int card_ravaging_blaze(int player, int card, event_t event)
{
  /* Ravaging Blaze	|X|R|R	0x200dd74
   * Instant
   * ~ deals X damage to target creature.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, ~ also deals X damage to that creature's controller. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  damage_target0(player, card, inst->info_slot);
		  if (spell_mastery(player))
			damage_player(inst->targets[0].player, inst->info_slot, player, card);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_X_SPELL, &td, "TARGET_CREATURE", 1, NULL);
}

int card_scab_clan_berserker(int player, int card, event_t event)
{
  /* Scab-Clan Berserker	|1|R|R	0x200dd79
   * Creature - Human Berserker 2/2
   * Haste
   * Renown 1
   * Whenever an opponent casts a noncreature spell, if ~ is renowned, ~ deals 2 damage to that player. */

  haste(player, card, event);
  renown(player, card, event, 1);
  if (is_renowned(player, card)
	  && specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE,DOESNT_MATCH, 0,0, 0,0, 0,0, -1,0))
	damage_player(1-player, 2, player, card);

  return 0;
}

int card_seismic_elemental(int player, int card, event_t event)
{
  /* Seismic Elemental	|3|R|R	0x200dd7e
   * Creature - Elemental 4/4
   * When ~ enters the battlefield, creatures without flying can't block this turn. */

  if (comes_into_play(player, card, event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.keyword = KEYWORD_FLYING;
	  test.keyword_flag = DOESNT_MATCH;
	  creatures_cannot_block(player, card, &test, 1);
	}

  return 0;
}

/* Skyraker Giant	|2|R|R	=>vanilla
 * Creature - Giant 4/3
 * Reach */

/* Smash to Smithereens	|1|R	=>shadowmoor.c:card_smash_to_smithereens
 * Instant
 * Destroy target artifact. ~ deals 3 damage to that artifact's controller. */

int card_subterranean_scout(int player, int card, event_t event)
{
  /* Subterranean Scout	|1|R	0x200dd83
   * Creature - Goblin Scout 2/1
   * When ~ enters the battlefield, target creature with power 2 or less can't be blocked this turn. */

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	  if (can_target(&td) && pick_target(&td, "TAWNOS_WAND"))	// "Select target creature with power 2 or less."
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 0,0, 0,SP_KEYWORD_UNBLOCKABLE);
		}
	}

  return 0;
}

int card_thopter_engineer(int player, int card, event_t event)
{
  /* Thopter Engineer	|2|R	0x200dd88
   * Creature - Human Artificer 1/3
   * When ~ enters the battlefield, put a 1/1 colorless Thopter artifact creature token with flying onto the battlefield.
   * Artifact creatures you control have haste. */

  if (comes_into_play(player, card, event))
	generate_token_by_id(player, card, CARD_ID_THOPTER);

  if (event == EVENT_ABILITIES && affected_card_controller == player
	  && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card))
	haste(affected_card_controller, affected_card, event);

  return 0;
}

/* Titan's Strength	|R	=>theros.c:card_titans_strength
 * Instant
 * Target creature gets +3/+1 until end of turn. Scry 1. */

/* Volcanic Rambler	|5|R	=>avacyn_restored.c:card_scalding_devil
 * Creature - Elemental 6/4
 * |2|R: ~ deals 1 damage to target player. */

/*** Green ***/

int card_aerial_volley(int player, int card, event_t event)
{
  /* Aerial Volley	|G	0x200dd8d
   * Instant
   * ~ deals 3 damage divided as you choose among one, two, or three target creatures with flying. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_abilities = KEYWORD_FLYING;

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);

	  int i;
	  for (i = 0; i < 3; ++i)
		if (new_pick_target(&td, "TARGET_FLYING", -1, 1))
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

int card_animists_awakening(int player, int card, event_t event)
{
  /* Animist's Awakening	|X|G	0x200e292
   * Sorcery
   * Reveal the top X cards of your library. Put all land cards from among them onto the battlefield tapped and the rest on the bottom of your library in a random order.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, untap those lands. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int mastery = spell_mastery(player);	// record now in case some trigger along the way adds or removes cards from graveyard

	  int num = MIN(count_deck(player), get_card_instance(player, card)->info_slot);
	  show_deck(HUMAN, deck_ptr[player], num, "Revealed", 0, 0x7375b0);

	  // remove cards from library in reverse order (so removing them doesn't affect later cards)
	  int lands[num], num_lands = 0, i;
	  for (i = num - 1; i >= 0; --i)
		if (is_what(-1, deck_ptr[player][i], TYPE_LAND))
		  {
			lands[num_lands++] = deck_ptr[player][i];
			remove_card_from_deck(player, i);
			--num;
		  }

	  // put rest on bottom
	  put_top_x_on_bottom_in_random_order(player, num);

	  // put revealed lands on bf (or back on library, preserving original order, if they can't move from library to bf)
	  int on_bf[num_lands], num_on_bf = 0;
	  int grafdigger = check_battlefield_for_id(ANYBODY, CARD_ID_GRAFDIGGERS_CAGE);
	  for (i = num_lands - 1; i >= 0; --i)
		if (grafdigger && is_what(-1, lands[i], TYPE_CREATURE))
		  raw_put_iid_on_top_of_deck(player, lands[i]);
		else
		  {
			int c = add_card_to_hand(player, lands[i]);
			on_bf[num_on_bf++] = c;
			get_card_instance(player, c)->state |= STATE_TAPPED;	// deliberately not tap_card(), since they don't become tapped
			put_into_play(player, c);
		  }

	  if (mastery)
		for (i = 0; i < num_on_bf; ++i)
		  if (in_play(player, on_bf[i]))
			untap_card(player, on_bf[i]);	// this time with the event, since they enter the bf tapped, then untap

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
}

/* Caustic Caterpillar	|G	=>darksteel.c:card_viridian_zealot
 * Creature - Insect 1/1
 * |1|G, Sacrifice ~: Destroy target artifact or enchantment. */

/* Conclave Naturalists	|4|G	=>m15.c:card_reclamation_sage
 * Creature - Dryad 4/4
 * When ~ enters the battlefield, you may destroy target artifact or enchantment. */

int card_dwynen_gilt_leaf_daen(int player, int card, event_t event)
{
  /* Dwynen, Gilt-Leaf Daen	|2|G|G	0x200dd92
   * Legendary Creature - Elf Warrior 3/4
   * Reach
   * Other Elf creatures you control get +1/+1.
   * Whenever ~ attacks, you gain 1 life for each attacking Elf you control. */

  check_legend_rule(player, card, event);
  boost_subtype(player, card, event, SUBTYPE_ELF, 1,1, 0,0, BCT_CONTROLLER_ONLY);
  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_ATTACKING;
	  test.subtype = SUBTYPE_ELF;
	  gain_life(player, check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &test));
	}

  return 0;
}

int card_dwynens_elite(int player, int card, event_t event)
{
  /* Dwynen's Elite	|1|G	0x200dd97
   * Creature - Elf Warrior 2/2
   * When ~ enters the battlefield, if you control another Elf, put a 1/1 |Sgreen Elf Warrior creature token onto the battlefield. */

  if (comes_into_play(player, card, event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "");
	  test.not_me = 1;
	  test.subtype = SUBTYPE_ELF;

	  if (check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test))
		generate_token_by_id(player, card, CARD_ID_ELF_WARRIOR);
	}

  return 0;
}

int card_elemental_bond(int player, int card, event_t event)
{
  /* Elemental Bond	|2|G	0x200dd9c
   * Enchantment
   * Whenever a creature with power 3 or greater enters the battlefield under your control, draw a card. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.power = 2;
	  test.power_flag = F5_POWER_GREATER_THAN_VALUE;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		draw_a_card(player);
	}

  return global_enchantment(player, card, event);
}

/* Elvish Visionary	|1|G	=>ravnica.c:card_carven_caryatid
 * Creature - Elf Shaman 1/1
 * When ~ enters the battlefield, draw a card. */

int card_evolutionary_leap(int player, int card, event_t event)
{
  /* Evolutionary Leap	|1|G	0x200dda1
   * Enchantment
   * |G, Sacrifice a creature: Reveal cards from the top of your library until you reveal a creature card. Put that card into your hand and the rest on the bottom of your library in a random order. */

  if (!IS_GAA_EVENT(event))
	return global_enchantment(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  // Of *course* there isn't a function encapsulating this sort of effect.  It's not like it's on dozens of cards or anything.

	  int* deck = deck_ptr[player];
	  int i, found = 0;
	  for (i = 0; i < 500 && deck[i] != -1; ++i)
		if (is_what(-1, deck[i], TYPE_CREATURE))
		  {
			found = 1;
			break;
		  }
	  if (!found)
		--i;
	  if (++i <= 0)
		return 0;

	  show_deck(HUMAN, deck, i, "Revealed", 0, (int)"OK");
	  if (found)
		{
		  --i;
		  add_card_to_hand(player, deck[i]);
		  remove_card_from_deck(player, i);
		}

	  if (i > 0)
		put_top_x_on_bottom_in_random_order(player, i);
	}

  return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST_G(1), 0, NULL, NULL);
}

/* Gaea's Revenge	|5|G|G	=>m11.c:card_gaeas_revenge
 * Creature - Elemental 8/5
 * ~ can't be countered.
 * Haste
 * ~ can't be the target of non|Sgreen spells or abilities from non|Sgreen sources. */

int card_gather_the_pack(int player, int card, event_t event)
{
  /* Gather the Pack	|1|G	0x200e297
   * Sorcery
   * Reveal the top five cards of your library. You may put a creature card from among them into your hand. Put the rest into your graveyard.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, put up to two creature cards from among the revealed cards into your hand instead of one. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");
	  if (spell_mastery(player))
		test.qty = 2;
	  reveal_top_cards_of_library_and_choose(player, card, player, 5, 0, TUTOR_HAND, 1, TUTOR_GRAVE, 1, &test);

	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_the_great_aurora(int player, int card, event_t event)
{
  /* The Great Aurora	|6|G|G|G	0x200e2a6
   * Sorcery
   * Each player shuffles all cards from his or her hand and all permanents he or she owns into his or her library, then draws that many cards. Each player may put any number of land cards from his or her hand onto the battlefield. Exile ~. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int c, num[2] = {0};
	  char marked[2][151] = {{0}};
	  APNAP(p,
			for (c = 0; c < active_cards_count[p]; ++c)
			  if (in_hand(p, c)
				  || (in_play(p, c) && is_what(p, c, TYPE_PERMANENT)))
				marked[p][c] = 1);
	  APNAP(p,
			for (c = 0; c < active_cards_count[p]; ++c)
			  if (marked[p][c] && get_card_instance(p, c)->internal_card_id != -1)	// sic
				{
				  ++num[get_owner(p, c)];
				  put_on_top_of_deck(p, c);
				});
	  APNAP(p, shuffle(p));
	  APNAP(p, draw_cards(p, num[p]));

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "Select a land card.");
	  APNAP(p,
			while (new_global_tutor(p, p, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_VALUE, &test) != -1)
			  {});

	  kill_card(player, card, KILL_REMOVE);
	}

  return basic_spell(player, card, event);
}

int card_herald_of_the_pantheon(int player, int card, event_t event)
{
  /* Herald of the Pantheon	|1|G	0x200dda6
   * Creature - Centaur Shaman 2/2
   * Enchantment spells you cast cost |1 less to cast.
   * Whenever you cast an enchantment spell, you gain 1 life. */

  if (event == EVENT_MODIFY_COST_GLOBAL)
	{
	  if ((affected_card_controller == -1 ? event_result : affected_card_controller) == player
		  && is_what(affected_card_controller, affected_card, TYPE_ENCHANTMENT) && !is_humiliated(player, card))
		COST_COLORLESS -= 1;
	  return 0;
	}

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_ENCHANTMENT,MATCH, 0,0, 0,0, 0,0, -1,0))
	gain_life(player, 1);

  return 0;
}

/* Hitchclaw Recluse	|2|G	=>vanilla
 * Creature - Spider 1/4
 * Reach */

int card_honored_hierarch(int player, int card, event_t event)
{
  /* Honored Hierarch	|G	0x200ddab
   * Creature - Human Druid 1/1
   * Renown 1
   * As long as ~ is renowned, it has vigilance and "|T: Add one mana of any color to your mana pool." */

  renown(player, card, event, 1);

  if (is_renowned(player, card))
	{
	  if ((event == EVENT_ABILITIES || event == EVENT_COUNT_MANA) && affect_me(player, card)
		  && !is_humiliated(player, card))
		{
		  vigilance(player, card, event);
		  get_card_instance(player, card)->info_slot = COLOR_TEST_ANY_COLORED;
		}
	  return mana_producing_creature_all_one_color(player, card, event, 0, COLOR_TEST_ANY_COLORED, 1);
	}
  else
	{
	  if ((event == EVENT_ABILITIES || event == EVENT_COUNT_MANA) && affect_me(player, card))
		get_card_instance(player, card)->info_slot = 0;
	  return 0;
	}
}

int card_joraga_invocation(int player, int card, event_t event)
{
  /* Joraga Invocation	|4|G|G	0x200ddb0
   * Sorcery
   * Each creature you control gets +3/+3 until end of turn and must be blocked this turn if able. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  pump_creatures_until_eot(player, card, player, 0, 3,3, 0,SP_KEYWORD_MUST_BE_BLOCKED, NULL);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Leaf Gilder	|1|G	=>m14.c:card_elvish_mystic
 * Creature - Elf Druid 2/1
 * |T: Add |G to your mana pool. */

/* Llanowar Empath	|3|G	=>future_sight.c:card_llanowar_empath
 * Creature - Elf Shaman 2/2
 * When ~ enters the battlefield, scry 2, then reveal the top card of your library. If it's a creature card, put it into your hand. */

int card_managorger_hydra(int player, int card, event_t event)
{
  /* Managorger Hydra	|2|G	0x200ddb5
   * Creature - Hydra 1/1
   * Trample
   * Whenever a player casts a spell, put a +1/+1 counter on ~. */

  if (specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, 0,0, 0,0, 0,0, 0,0, -1,0))
	add_1_1_counter(player, card);

  return 0;
}

int card_mantle_of_webs(int player, int card, event_t event)
{
  /* Mantle of Webs	|1|G	0x200ddba
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +1/+3 and has reach. */
  return generic_aura(player, card, event, player, 1,3, KEYWORD_REACH,0, 0,0,0);
}

/* Might of the Masses	|G	=>rise_of_the_eldrazi.c:card_might_of_the_masses
 * Instant
 * Target creature gets +1/+1 until end of turn for each creature you control. */

int card_nissa_vastwood_seer(int player, int card, event_t event)
{
  /* Nissa, Vastwood Seer	|2|G	0x200e2ab
   * Legendary Creature - Elf Scout 2/2
   * When ~ enters the battlefield, you may search your library for a basic |H2Forest card, reveal it, put it into your hand, then shuffle your library.
   * Whenever a land enters the battlefield under your control, if you control seven or more lands, exile ~, then return her to the battlefield transformed under her owner's control. */

  double_faced_card(player, card, event);
  check_legend_rule(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select a basic %s card.", SUBTYPE_FOREST));
	  test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
	  test.sub2 = SUBTYPE_BASIC;
	  test.subtype_flag = F2_MULTISUBTYPE_ALL;
	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &test);
	}

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player
	  && count_subtype(player, TYPE_LAND, -1) >= 7
	  && specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_LAND,MATCH, 0,0, 0,0, 0,0, -1,0))
	exile_and_return_to_bf_transformed(player, card);

  return 0;
}
int card_nissa_sage_animist(int player, int card, event_t event)
{
  /* Nissa, Sage Animist	""	0x200e2b0
   * Planeswalker - Nissa (3)
   * +1: Reveal the top card of your library. If it's a land card, put it onto the battlefield. Otherwise, put it into your hand.
   * -2: Put a legendary 4/4 |Sgreen Elemental creature token named Ashaya, the Awoken World onto the battlefield.
   * -7: Untap up to six target lands. They become 6/6 Elemental creatures. They're still lands. */

  if (IS_ACTIVATING(event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_LAND);
	  td.preferred_controller = player;
	  td.allow_cancel = 3;	// Cancel and Done

	  card_instance_t* inst = get_card_instance(player, card);

	  enum
	  {
		CHOICE_REVEAL = 1,
		CHOICE_ASHAYA,
		CHOICE_ELEMENTALS
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Reveal top card",			1,	1,	+1,
						"Ashaya, the Awoken World",	1,	1,	-2,
						"Lands to creatures",		1,	3,	-7);

	  if (event == EVENT_CAN_ACTIVATE && !choice)
		return 0;

	  if (event == EVENT_ACTIVATE && choice == CHOICE_ELEMENTALS)
		pick_up_to_n_targets(&td, "TARGET_LAND", 6);

	  if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_REVEAL:
			  ;int iid = deck_ptr[player][0];
			  if (iid != -1)
				{
				  reveal_card_iid(player, card, iid);
				  if (!is_what(-1, iid, TYPE_LAND))
					{
					  obliterate_top_card_of_deck(player);
					  add_card_to_hand(player, iid);
					}
				  else
					put_into_play_a_card_from_deck(player, player, 0);
				}
			  break;

			case CHOICE_ASHAYA:
			  generate_token_by_id(player, card, CARD_ID_ASHAYA_THE_AWOKEN_WORLD);
			  break;

			case CHOICE_ELEMENTALS:
			  // Animate first, then untap, since only the latter has associated triggers
			  ;int i;
			  for (i = 0; i < inst->number_of_targets; ++i)
				if (validate_target(player, card, &td, i))
				  {
					add_a_subtype(inst->targets[i].player, inst->targets[i].card, SUBTYPE_ELEMENTAL);
					animate_other(player, card, inst->targets[i].player, inst->targets[i].card, 6,6, 0,0, 0, 1);
				  }
				else
				  inst->targets[i].player = -1;

			  for (i = 0; i < inst->number_of_targets; ++i)
				if (inst->targets[i].player != -1 && in_play(inst->targets[i].player, inst->targets[i].card))
				  untap_card(inst->targets[i].player, inst->targets[i].card);

			  break;
		  }
	}

  return planeswalker(player, card, event, 3);
}

int card_nissas_pilgrimage(int player, int card, event_t event)
{
  /* Nissa's Pilgrimage	|2|G	0x200ddbf
   * Sorcery
   * Search your library for up to two basic |H2Forest cards, reveal those cards, and put one onto the battlefield tapped and the rest into your hand. Then shuffle your library.
   * Spell mastery - If there are two or more instant and/or sorcery cards in your graveyard, search your library for up to three basic |H2Forest cards instead of two. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int to_hand = spell_mastery(player) ? 2 : 1;

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, get_hacked_land_text(player, card, "Select a basic %s card to put on the battlefield tapped.", SUBTYPE_FOREST));
	  test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
	  test.sub2 = SUBTYPE_BASIC;
	  test.subtype_flag = F2_MULTISUBTYPE_ALL;
	  test.no_shuffle = 1;

	  if (new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &test) != -1)
		{
		  if (ai_is_speculating != 1)
			strcpy(test.message, get_hacked_land_text(player, card, "Select a basic %s card to put in your hand.", SUBTYPE_FOREST));
		  test.qty = to_hand;
		  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		}

	  shuffle(player);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_nissas_revelation(int player, int card, event_t event)
{
  /* Nissa's Revelation	|5|G|G	0x200ddc4
   * Sorcery
   * Scry 5, then reveal the top card of your library. If it's a creature card, you draw cards equal to its power and you gain life equal to its toughness. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  scry(player, 5);
	  int iid = deck_ptr[player][0];
	  if (iid != -1)
		{
		  reveal_card_iid(player, card, iid);
		  if (is_what(-1, iid, TYPE_CREATURE))
			{
			  int pow = get_base_power_iid(player, iid);
			  int tgh = get_base_toughness_iid(player, iid);
			  draw_cards(player, pow);
			  gain_life(player, tgh);
			}
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

/* Orchard Spirit	|2|G	=>innistrad.c:card_orchard_spirit
 * Creature - Spirit 2/2
 * ~ can't be blocked except by creatures with flying or reach. */

/* Outland Colossus	|3|G|G	0x000000
 * Creature - Giant 6/6
 * Renown 6
 * ~ can't be blocked by more than one creature. */

int card_pharikas_disciple(int player, int card, event_t event)
{
  /* Pharika's Disciple	|3|G	0x200ddce
   * Creature - Centaur Warrior 2/3
   * Deathtouch
   * Renown 1 */

  deathtouch(player, card, event);
  renown(player, card, event, 1);
  return 0;
}

/* Reclaim	|G	=>exodus.c:card_reclaim
 * Instant
 * Put target card from your graveyard on top of your library. */

int card_rhox_maulers(int player, int card, event_t event)
{
  /* Rhox Maulers	|4|G	0x200ddd3
   * Creature - Rhino Soldier 4/4
   * Trample
   * Renown 2 */

  renown(player, card, event, 2);
  return 0;
}

/* Skysnare Spider	|4|G|G	=>unlimited.c:card_serra_angel
 * Creature - Spider 6/6
 * Vigilance
 * Reach */

int card_somberwald_alpha(int player, int card, event_t event)
{
  /* Somberwald Alpha	|3|G	0x200ddc9
   * Creature - Wolf 3/2
   * Whenever a creature you control becomes blocked, it gets +1/+1 until end of turn.
   * |1|G: Target creature you control gains trample until end of turn. */

  if (event == EVENT_DECLARE_BLOCKERS && current_turn == player && !is_humiliated(player, card))
	{
	  card_instance_t* inst;
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if ((inst = in_play(player, c)) && (inst->state & STATE_ATTACKING) && is_what(player, c, TYPE_CREATURE)
			&& !is_unblocked(player, c))
		  alternate_legacy_text(1, player, pump_until_eot(player, card, player, c, 1,1));
	}

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = td.preferred_controller = player;

  return vanilla_creature_pumper(player, card, event, MANACOST_XG(1,1), 0, 0,0, KEYWORD_TRAMPLE,0, &td);
}

/* Sylvan Messenger	|3|G	=>apocalypse.c:card_sylvan_messenger
 * Creature - Elf 2/2
 * Trample
 * When ~ enters the battlefield, reveal the top four cards of your library. Put all Elf cards revealed this way into your hand and the rest on the bottom of your library in any order. */

/* Timberpack Wolf	|1|G	=>m13.c:card_timberpack_wolf
 * Creature - Wolf 2/2
 * ~ gets +1/+1 for each other creature you control named ~. */

/* Titanic Growth	|1|G	=>portal_1_2_3k.c:card_monstrous_growth
 * Instant
 * Target creature gets +4/+4 until end of turn. */

int card_undercity_troll(int player, int card, event_t event)
{
  /* Undercity Troll	|1|G	0x200ddd8
   * Creature - Troll 2/2
   * Renown 1
   * |2|G: Regenerate ~. */

  renown(player, card, event, 1);
  return regeneration(player, card, event, MANACOST_XG(2,1));
}

int card_valeron_wardens(int player, int card, event_t event)
{
  /* Valeron Wardens	|2|G	0x200dddd
   * Creature - Human Monk 1/3
   * Renown 2
   * Whenever a creature you control becomes renowned, draw a card. */

  renown(player, card, event, 2);

  if (xtrigger_condition() == XTRIGGER_BECAME_RENOWNED && affect_me(player, card) && player == reason_for_trigger_controller
	  && trigger_cause_controller == player && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;
	  if (event == EVENT_RESOLVE_TRIGGER)
		draw_a_card(player);
	}

  return 0;
}

/* Vastwood Gorger	|5|G	=>vanilla
 * Creature - Wurm 5/6 */

static int fx_fog_by_power_le_4(int player, int card, event_t event)
{
  card_instance_t* damage = combat_damage_being_prevented(event);
  if (damage
	  && (damage->targets[3].player & TYPE_CREATURE)	// probably redundant to combat damage check
	  && get_power(damage->damage_source_player, damage->damage_source_card) <= 4)
	damage->info_slot = 0;

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
int card_vine_snare(int player, int card, event_t event)
{
  /* Vine Snare	|2|G	0x200dde2
   * Instant
   * Prevent all combat damage that would be dealt this turn by creatures with power 4 or less. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  create_legacy_effect(player, card, &fx_fog_by_power_le_4);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_wild_instincts(int player, int card, event_t event)
{
  /* Wild Instincts	|3|G	0x200dde7
   * Sorcery
   * Target creature you control gets +2/+2 until end of turn. It fights target creature an opponent controls. */

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
	  if (valid_target(&td_control))
		{
		  pump_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 2,2);

		  if (validate_target(player, card, &td_dont_control, 1))
			fight(inst->targets[0].player, inst->targets[0].card, inst->targets[1].player, inst->targets[1].card);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_woodland_bellower(int player, int card, event_t event)
{
  /* Woodland Bellower	|4|G|G	0x200ddec
   * Creature - Beast 6/5
   * When ~ enters the battlefield, you may search your library for a nonlegendary |Sgreen creature card with converted mana cost 3 or less, put it onto the battlefield, then shuffle your library. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE,
								  get_sleighted_color_text(player, card,
														   "Select a nonlegendary %s creature card with converted mana cost 3 or less.", COLOR_GREEN));
	  test.cmc = 4;
	  test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
	  test.subtype = SUBTYPE_LEGEND;
	  test.subtype_flag = DOESNT_MATCH;
	  test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
	  test.color_flag = MATCH;

	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &test);
	}

  return 0;
}

/* Yeva's Forcemage	|2|G	=>m13.c:card_yeva_forcemage
 * Creature - Elf Shaman 2/2
 * When ~ enters the battlefield, target creature gets +2/+2 until end of turn. */

int card_zendikars_roil(int player, int card, event_t event)
{
  /* Zendikar's Roil	|3|G|G	0x200ddf1
   * Enchantment
   * Whenever a land enters the battlefield under your control, put a 2/2 |Sgreen Elemental creature token onto the battlefield. */

  if (specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_LAND,MATCH, 0,0, 0,0, 0,0, -1,0))
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
	  token.pow = token.tou = 2;
	  token.color_forced = COLOR_TEST_GREEN;
	  generate_token(&token);
	}

  return global_enchantment(player, card, event);
}

/*** Multi ***/

int card_blazing_hellhound(int player, int card, event_t event)
{
  /* Blazing Hellhound	|2|B|R	0x200ddf6
   * Creature - Elemental Hound 4/3
   * |1, Sacrifice another creature: ~ deals 1 damage to target creature or player. */

  if (!IS_ACTIVATING(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  card_instance_t* instance = get_card_instance(player, card);

  test_definition_t test;
  new_default_test_definition(&test, TYPE_CREATURE, "Select another creature to sacrifice.");
  test.not_me = 1;

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_ACTIVATE(player, card, MANACOST_X(1)) && new_can_sacrifice_as_cost(player, card, &test);

  if (event == EVENT_ACTIVATE)
	{
	  int sac;
	  if (!charge_mana_for_activated_ability(player, card, MANACOST_X(1))
		  || !(sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test)))
		{
		  cancel = 1;
		  return 0;
		}

	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
		kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
	  else
		{
		  cancel = 1;
		  state_untargettable(BYTE2(sac), BYTE3(sac), 0);
		}
	}

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	damage_target0(player, card, 1);

  return 0;
}

int card_blood_cursed_knight(int player, int card, event_t event)
{
  /* Blood-Cursed Knight	|1|W|B	0x200ddfb
   * Creature - Vampire Knight 3/2
   * As long as you control an enchantment, ~ gets +1/+1 and has lifelink. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_ABILITIES) && affect_me(player, card) && !is_humiliated(player, card)
	  && count_subtype(player, TYPE_ENCHANTMENT, -1) > 0)
	{
	  if (event == EVENT_ABILITIES)
		lifelink(player, card, event);
	  else
		event_result += 1;
	}

  return 0;
}

int card_bounding_krasis(int player, int card, event_t event)
{
  /* Bounding Krasis	|1|G|U	0x200de00
   * Creature - Fish Lizard 3/3
   * Flash
   * When ~ enters the battlefield, you may tap or untap target creature. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = ANYBODY;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		twiddle(player, card, 0);
	}

  return flash(player, card, event);
}

int card_citadel_castellan(int player, int card, event_t event)
{
  /* Citadel Castellan	|1|G|W	0x200de05
   * Creature - Human Knight 2/3
   * Vigilance
   * Renown 2 */

  vigilance(player, card, event);
  renown(player, card, event, 2);
  return 0;
}

/* Iroas's Champion	|1|R|W	=>vanilla
 * Creature - Human Soldier 2/2
 * Double strike */

int card_possessed_skaab(int player, int card, event_t event)
{
  /* Possessed Skaab	|3|U|B	0x200de0a
   * Creature - Zombie 3/2
   * When ~ enters the battlefield, return target instant, sorcery, or creature card from your graveyard to your hand.
   * If ~ would die, exile it instead. */

  if (comes_into_play(player, card, event)
	  && any_in_graveyard_by_type(player, TYPE_INSTANT|TYPE_SORCERY|TYPE_CREATURE) && !graveyard_has_shroud(player))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_INSTANT|TYPE_SORCERY|TYPE_CREATURE, "Select target instant, sorcery, or creature card.");
	  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &test);
	}

  /* Not foolproof, but reasonably ok.  The main inaccuracy will be that other cards looking for graveyard triggers that have earlier timestamps will still see
   * this as having its original kill_code.  (This is actually how Cyclopean Mummy was originally implemented, though its current wording is a trigger instead
   * of a replacement effect.)  The proper way to do this is with an XTRIGGER_REPLACE_KILL trigger as in Shandalar. */
  if (event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(player, card) && in_play(player, card) && !is_humiliated(player, card))
	get_card_instance(player, card)->kill_code = KILL_REMOVE;

  return 0;
}

int card_reclusive_artificer(int player, int card, event_t event)
{
  /* Reclusive Artificer	|2|U|R	0x200de0f
   * Creature - Human Artificer 2/3
   * Haste
   * When ~ enters the battlefield, you may have it deal damage to target creature equal to the number of artifacts you control. */

  haste(player, card, event);

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		damage_target0(player, card, count_subtype(player, TYPE_ARTIFACT, -1));
	}

  return 0;
}

int card_shaman_of_the_pack(int player, int card, event_t event)
{
  /* Shaman of the Pack	|1|B|G	0x200de14
   * Creature - Elf Shaman 3/2
   * When ~ enters the battlefield, target opponent loses life equal to the number of Elves you control. */

  if (comes_into_play(player, card, event) && opponent_is_valid_target(player, card))
	lose_life(1-player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_ELF));

  return 0;
}

int card_thunderclap_wyvern(int player, int card, event_t event)
{
  /* Thunderclap Wyvern	|2|W|U	0x200de19
   * Creature - Drake 2/3
   * Flash
   * Flying
   * Other creatures you control with flying get +1/+1. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && affected_card_controller == player && affected_card != card
	  && in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card)
	  && check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING))
	event_result += 1;

  return flash(player, card, event);
}

int card_zendikar_incarnate(int player, int card, event_t event)
{
  /* Zendikar Incarnate	|2|R|G	0x200de1e
   * Creature - Elemental 100/4
   * ~'s power is equal to the number of lands you control. */

  if (event == EVENT_POWER && affect_me(player, card) && !is_humiliated(player, card) && player != -1)
	// Irritatingly, can't just use landsofcolor_controlled[player][COLOR_ANY], since it won't have been updated yet.
	event_result += count_subtype(player, TYPE_LAND, -1);

  return 0;
}

/*** Artifact ***/

int card_alchemists_vial(int player, int card, event_t event)
{
  /* Alchemist's Vial	|2	0x200de23
   * Artifact
   * When ~ enters the battlefield, draw a card.
   * |1, |T, Sacrifice ~: Target creature can't attack or block this turn. */

  if (comes_into_play(player, card, event))
	draw_a_card(player);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int leg = pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 0,0, 0,SP_KEYWORD_CANNOT_BLOCK);
	  get_card_instance(player, leg)->targets[3].player |= PAUE_CANT_ATTACK;
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET|GAA_SACRIFICE_ME, MANACOST_X(1), 0, &td, "TARGET_CREATURE");
}

/* Alhammarret's Archive	|5	0x000000
 * Legendary Artifact
 * If you would gain life, you gain twice that much life instead.
 * If you would draw a card except the first one you draw in each of your draw steps, draw two cards instead. */

/* Angel's Tomb	|3	=>avacyn_restored.c:card_angels_tomb
 * Artifact
 * Whenever a creature enters the battlefield under your control, you may have ~ become a 3/3 |Swhite Angel artifact creature with flying until end of turn. */

/* Bonded Construct	|1	0x000000	[see http://www.slightlymagic.net/forum/viewtopic.php?t=12717 before attempting to do this]
 * Artifact Creature - Construct 2/1
 * ~ can't attack alone. */

/* Brawler's Plate	|3	=>m15.c:card_brawlers_plate
 * Artifact - Equipment
 * Equipped creature gets +2/+2 and has trample.
 * Equip |4 */

int card_chief_of_the_foundry(int player, int card, event_t event)
{
  /* Chief of the Foundry	|3	0x200de28
   * Artifact Creature - Construct 2/3
   * Other artifact creatures you control get +1/+1. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && affected_card_controller == player && affected_card != card
	  && in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card)
	  && is_what(affected_card_controller, affected_card, TYPE_ARTIFACT))	// non-creatures won't get these events anyway
	event_result += 1;

  return 0;
}

/* Gold-Forged Sentinel	|6	=>vanilla
 * Artifact Creature - Chimera 4/4
 * Flying */

int card_guardian_automaton(int player, int card, event_t event)
{
  /* Guardian Automaton	|4	0x200de2d
   * Artifact Creature - Construct 3/3
   * When ~ dies, you gain 3 life. */

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	gain_life(player, 3);

  return 0;
}

/* Guardians of Meletis	|3	=>vanilla
 * Artifact Creature - Golem 0/6
 * Defender */

int card_hangarback_walker(int player, int card, event_t event)
{
  /* Hangarback Walker	|X|X	0x200de32
   * Artifact Creature - Construct 0/0
   * ~ enters the battlefield with X +1/+1 counters on it.
   * When ~ dies, put a 1/1 colorless Thopter artifact creature token with flying onto the battlefield for each +1/+1 counter on ~.
   * |1, |T: Put a +1/+1 counter on ~. */

  card_instance_t* inst = get_card_instance(player, card);

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	generate_tokens_by_id(player, card, CARD_ID_THOPTER, count_1_1_counters(player, card));

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	inst->info_slot = played_for_free(player, card) || is_token(player, card) ? 0 : charge_mana_for_double_x(player, COLOR_ARTIFACT) / 2;

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, inst->info_slot);

  if (event == EVENT_RESOLVE_ACTIVATION && in_play(inst->parent_controller, inst->parent_card))
	add_1_1_counter(inst->parent_controller, inst->parent_card);

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(1), 0, NULL, NULL);
}

int card_helm_of_the_gods(int player, int card, event_t event)
{
  /* Helm of the Gods	|1	0x200de37
   * Artifact - Equipment
   * Equipped creature gets +1/+1 for each enchantment you control.
   * Equip |1 */

  int ench = 0;
  if (event == EVENT_POWER || event == EVENT_TOUGHNESS)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (affect_me(inst->targets[8].player, inst->targets[8].card))
		ench = count_subtype(player, TYPE_ENCHANTMENT, -1);
	}

  return vanilla_equipment(player, card, event, 1, ench,ench, 0,0);
}

/* Jayemdae Tome	|4	=>unlimited.c:card_jayemdae_tome
 * Artifact
 * |4, |T: Draw a card. */

int card_mage_ring_responder(int player, int card, event_t event)
{
  /* Mage-Ring Responder	|7	0x200de3c
   * Artifact Creature - Golem 7/7
   * ~ doesn't untap during your untap step.
   * |7: Untap ~.
   * Whenever ~ attacks, it deals 7 damage to target creature defending player controls. */

  does_not_untap(player, card, event);

  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		damage_target0(player, card, 7);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card))
		untap_card(inst->parent_controller, inst->parent_card);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_X(7), 0, NULL, NULL);
}

/* Meteorite	|5	=>m15.c:card_meteorite
 * Artifact
 * When ~ enters the battlefield, it deals 2 damage to target creature or player.
 * |T: Add one mana of any color to your mana pool. */

int card_orbs_of_warding(int player, int card, event_t event)
{
  /* Orbs of Warding	|5	0x200de41
   * Artifact
   * You have hexproof.
   * If a creature would deal damage to you, prevent 1 of that damage. */

  give_hexproof_to_player(player, card, event);

  card_instance_t* damage = damage_being_prevented(event);
  if (damage
	  && (damage->targets[3].player & TYPE_CREATURE)
	  && damage->damage_target_card == -1 && damage->damage_target_player == player && damage->targets[4].player == -1
	  && in_play(player, card) && !is_humiliated(player, card))
	damage->info_slot -= 1;

  return 0;
}

int card_prism_ring(int player, int card, event_t event)
{
  /* Prism Ring	|1	0x200de46
   * Artifact
   * As ~ enters the battlefield, choose a color.
   * Whenever you cast a spell of the chosen color, you gain 1 life. */

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_RESOLVE_SPELL)
	inst->info_slot = 1 << choose_a_color_and_show_legacy(player, card, player, -1);

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, 0,0, 0,0, inst->info_slot,MATCH, 0,0, -1,0))
	gain_life(player, 1);

  return 0;
}

/* Pyromancer's Goggles	|5	0x000000
 * Legendary Artifact
 * |T: Add |R to your mana pool. When that mana is spent to cast a |Sred instant or sorcery spell, copy that spell and you may choose new targets for the copy. */

int card_ramroller(int player, int card, event_t event)
{
  /* Ramroller	|3	0x200de4b
   * Artifact Creature - Juggernaut 2/3
   * ~ attacks each turn if able.
   * ~ gets +2/+0 as long as you control another artifact. */

  if (!is_humiliated(player, card))
	{
	  attack_if_able(player, card, event);

	  if (event == EVENT_POWER && affect_me(player, card))
		{
		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_ARTIFACT, "");
		  test.not_me = 1;
		  if (check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test))
			event_result += 2;
		}
	}

  return 0;
}

/* Runed Servitor	|2	=>conspiracy.c:card_runed_servitor
 * Artifact Creature - Construct 2/2
 * When ~ dies, each player draws a card. */

int card_sigil_of_valor(int player, int card, event_t event)
{
  /* Sigil of Valor	|2	0x200de50
   * Artifact - Equipment
   * Whenever equipped creature attacks alone, it gets +1/+1 until end of turn for each other creature you control.
   * Equip |1 */

  card_instance_t* inst = get_card_instance(player, card);
  if (inst->targets[8].card >= 0
	  && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY|DAT_ATTACKS_ALONE, inst->targets[8].player, inst->targets[8].card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;
	  int p = inst->targets[8].player, c = inst->targets[8].card;
	  int others = check_battlefield_for_special_card(p, c, player, CBFSC_GET_COUNT, &test);
	  if (others > 0)
		pump_until_eot(player, card, p, c, others,others);
	}

  return basic_equipment(player, card, event, 1);
}

int card_sword_of_the_animist(int player, int card, event_t event)
{
  /* Sword of the Animist	|2	0x200de55
   * Legendary Artifact - Equipment
   * Equipped creature gets +1/+1.
   * Whenever equipped creature attacks, you may search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library.
   * Equip |2 */

  check_legend_rule(player, card, event);

  card_instance_t* inst = get_card_instance(player, card);
  if (inst->targets[8].card >= 0
	  && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), inst->targets[8].player, inst->targets[8].card))
	tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 1);

  return vanilla_equipment(player, card, event, 2, 1,1, 0,0);
}

int card_throwing_knife(int player, int card, event_t event)
{
  /* Throwing Knife	|2	0x200de5a
   * Artifact - Equipment
   * Equipped creature gets +2/+0.
   * Whenever equipped creature attacks, you may sacrifice ~. If you do, ~ deals 2 damage to target creature or player.
   * Equip |2 */

  card_instance_t* inst = get_card_instance(player, card);
  if (inst->targets[8].card >= 0
	  && declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), inst->targets[8].player, inst->targets[8].card)
	  && can_sacrifice_this_as_cost(player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
		{
		  kill_card(player, card, KILL_SACRIFICE);
		  damage_target0(player, card, 2);
		}
	}

  return vanilla_equipment(player, card, event, 2, 2,0, 0,0);
}

/* Veteran's Sidearm	|2	=>mirrodin.c:card_leonin_scimitar
 * Artifact - Equipment
 * Equipped creature gets +1/+1.
 * Equip |1 */

/* War Horn	|3	=>unlimited.c:card_orcish_oriflamme
 * Artifact
 * Attacking creatures you control get +1/+0. */

/*** Land ***/

/* Battlefield Forge	""	=>functions.c:generic_painland
 * Land
 * |T: Add |1 to your mana pool.
 * |T: Add |R or |W to your mana pool. ~ deals 1 damage to you. */

/* Caves of Koilos	""	=>functions.c:generic_painland
 * Land
 * |T: Add |1 to your mana pool.
 * |T: Add |W or |B to your mana pool. ~ deals 1 damage to you. */

/* Evolving Wilds	""	=>m11.c:card_terramorphic_expanse
 * Land
 * |T, Sacrifice ~: Search your library for a basic land card and put it onto the battlefield tapped. Then shuffle your library. */

int card_foundry_of_the_consuls(int player, int card, event_t event)
{
  /* Foundry of the Consuls	""	0x200de5f
   * Land
   * |T: Add |1 to your mana pool.
   * |5, |T, Sacrifice ~: Put two 1/1 colorless Thopter artifact creature tokens with flying onto the battlefield. */

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_TOKENS
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1, DLG_NO_DISPLAY_FOR_AI,
						"Produce mana",
							mana_producer(player, card, EVENT_CAN_ACTIVATE),
							paying_mana(),
						"Thopter tokens",
							/* This calls generic_activated_ability() and charges mana manually instead of using a DLG_MANA() clause because both this and its
							 * mana ability have T in their costs */
							!paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED|GAA_SACRIFICE_ME,
																		MANACOST_X(6), 0, NULL, NULL),
							landsofcolor_controlled[player][COLOR_ANY] - 4);
	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_TOKENS)
		{
		  if (event == EVENT_ACTIVATE)
			{
			  add_state(player, card, STATE_TAPPED);
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(5)))
				kill_card(player, card, KILL_SACRIFICE);
			  else
				remove_state(player, card, STATE_TAPPED);
			}
		  else if (event == EVENT_RESOLVE_ACTIVATION)
			generate_tokens_by_id(player, card, CARD_ID_THOPTER, 2);

		  return 0;
		}
	  // else fall through to mana_producer below
	}

  return mana_producer(player, card, event);
}

/* Llanowar Wastes	""	=>functions.c:generic_painland
 * Land
 * |T: Add |1 to your mana pool.
 * |T: Add |B or |G to your mana pool. ~ deals 1 damage to you. */

/* Mage-Ring Network	""	0x000000
 * Land
 * |T: Add |1 to your mana pool.
 * |1, |T: Put a storage counter on ~.
 * |T, Remove X storage counters from ~: Add |X to your mana pool. */

/* Rogue's Passage	""	=>return_to_ravnica.c:card_rogues_passage
 * Land
 * |T: Add |1 to your mana pool.
 * |4, |T: Target creature can't be blocked this turn. */

/* Shivan Reef	""	=>functions.c:generic_painland
 * Land
 * |T: Add |1 to your mana pool.
 * |T: Add |U or |R to your mana pool. ~ deals 1 damage to you. */

/* Yavimaya Coast	""	=>functions.c:generic_painland
 * Land
 * |T: Add |1 to your mana pool.
 * |T: Add |G or |U to your mana pool. ~ deals 1 damage to you. */

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

/*** Bonus ***/

/* Aegis Angel	|4|W|W	=>m12.c:card_aegis_angel
 * Creature - Angel 5/5
 * Flying
 * When ~ enters the battlefield, another target permanent gains indestructible for as long as you control ~. */

/* Divine Verdict	|3|W	=>m10.c:card_divine_verdict
 * Instant
 * Destroy target attacking or blocking creature. */

/* Eagle of the Watch	|2|W	=>unlimited.c:card_serra_angel
 * Creature - Bird 2/1
 * Flying, vigilance */

/* Serra Angel	|3|W|W	=>unlimited.c:card_serra_angel
 * Creature - Angel 4/4
 * Flying
 * Vigilance */

/* Into the Void	|3|U	=>avacyn_restored.c:card_into_the_void
 * Sorcery
 * Return up to two target creatures to their owners' hands. */

/* Mahamoti Djinn	|4|U|U	=>vanilla
 * Creature - Djinn 5/6
 * Flying */

/* Weave Fate	|3|U	=>m14.c:card_divination
 * Instant
 * Draw two cards. */

/* Flesh to Dust	|3|B|B	=>m15.c:card_flesh_to_dust
 * Instant
 * Destroy target creature. It can't be regenerated. */

/* Mind Rot	|2|B	=>portal_1_2_3k.c:card_mind_rot
 * Sorcery
 * Target player discards two cards. */

/* Nightmare	|5|B	=>unlimited.c:card_nightmare2
 * Creature - Nightmare Horse 100/100
 * Flying
 * ~'s power and toughness are each equal to the number of |H1Swamps you control. */

/* Sengir Vampire	|3|B|B	=>unlimited.c:card_sengir_vampire
 * Creature - Vampire 4/4
 * Flying
 * Whenever a creature dealt damage by ~ this turn dies, put a +1/+1 counter on ~. */

/* Fiery Hellhound	|1|R|R	=>fifth_dawn.c:card_furnace_whelp
 * Creature - Elemental Hound 2/2
 * |R: ~ gets +1/+0 until end of turn. */

/* Shivan Dragon	|4|R|R	=>fifth_dawn.c:card_furnace_whelp
 * Creature - Dragon 5/5
 * Flying
 * |R: ~ gets +1/+0 until end of turn. */

/* Plummet	|1|G	=>urza_legacy.c:card_wing_snare
 * Instant
 * Destroy target creature with flying. */

/* Prized Unicorn	|3|G	=>urza_destiny.c:card_taunting_elf
 * Creature - Unicorn 2/2
 * All creatures able to block ~ do so. */

/* Terra Stomper	|3|G|G|G	=>zendikar.c:card_terra_stomper
 * Creature - Beast 8/8
 * ~ can't be countered.
 * Trample */
