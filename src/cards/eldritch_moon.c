// -*- c-basic-offset:2 -*-
#include "manalink.h"

/***** Functions *****/

/***** Cards *****/

/*** Colorless ***/

/* Abundant Maw	|8	0x000000
 * Creature - Eldrazi Leech 6/4
 * Emerge |6|B
 * When you cast ~, target opponent loses 3 life and you gain 3 life. */

/* Decimator of the Provinces	|10	0x000000
 * Creature - Eldrazi Boar 7/7
 * Emerge |6|G|G|G
 * When you cast ~, creatures you control get +2/+2 and gain trample until end of turn.
 * Trample, haste */

/* Distended Mindbender	|8	0x000000
 * Creature - Eldrazi Insect 5/5
 * Emerge |5|B|B
 * When you cast ~, target opponent reveals his or her hand. You choose from it a nonland card with converted mana cost 3 or less and a card with converted mana cost 4 or greater. That player discards those cards. */

/* Drownyard Behemoth	|9	0x000000
 * Creature - Eldrazi Crab 5/7
 * Flash
 * Emerge |7|U
 * ~ has hexproof as long as it entered the battlefield this turn. */

/* Elder Deep-Fiend	|8	0x000000
 * Creature - Eldrazi Octopus 5/6
 * Flash
 * Emerge |5|U|U
 * When you cast ~, tap up to four target permanents. */

/* Emrakul, the Promised End	|13	0x000000
 * Legendary Creature - Eldrazi 13/13
 * ~ costs |1 less to cast for each card type among cards in your graveyard.
 * When you cast Emrakul, you gain control of target opponent during that player's next turn. After that turn, that player takes an extra turn.
 * Flying, trample, protection from instants */

/* Eternal Scourge	|3	0x000000
 * Creature - Eldrazi Horror 3/3
 * You may cast ~ from exile.
 * When ~ becomes the target of a spell or ability an opponent controls, exile ~. */

/* It of the Horrid Swarm	|8	0x000000
 * Creature - Eldrazi Insect 4/4
 * Emerge |6|G
 * When you cast ~, put two 1/1 |Sgreen Insect creature tokens onto the battlefield. */

/* Lashweed Lurker	|8	0x000000
 * Creature - Eldrazi Horror 5/4
 * Emerge |5|G|U
 * When you cast ~, you may put target nonland permanent on top of its owner's library. */

/* Mockery of Nature	|9	0x000000
 * Creature - Eldrazi Beast 6/5
 * Emerge |7|G
 * When you cast ~, you may destroy target artifact or enchantment. */

/* Vexing Scuttler	|8	0x000000
 * Creature - Eldrazi Crab 4/5
 * Emerge |6|U
 * When you cast ~, you may return target instant or sorcery card from your graveyard to your hand. */

/* Wretched Gryff	|7	0x000000
 * Creature - Eldrazi Hippogriff 3/4
 * Emerge |5|U
 * When you cast ~, draw a card.
 * Flying */

/*** White ***/

/* Blessed Alliance	|1|W	0x000000
 * Instant
 * Escalate |2
 * Choose one or more -
 * * Target player gains 4 life.
 * * Untap up to two target creatures.
 * * Target opponent sacrifices an attacking creature. */

/* Borrowed Grace	|2|W	0x000000
 * Instant
 * Escalate |1|W
 * Choose one or both -
 * * Creatures you control get +2/+0 until end of turn.
 * * Creatures you control get +0/+2 until end of turn. */

/* Bruna, the Fading Light	|5|W|W	0x000000
 * Legendary Creature - Angel Horror 5/7
 * When you cast ~, you may return target Angel or Human creature card from your graveyard to the battlefield.
 * Flying, vigilance
 * --MELD WITH--
 * Gisela, the Broken Blade	|2|W|W	0x000000
 * Legendary Creature - Angel Horror 4/3
 * Flying, first strike, lifelink
 * At the beginning of your end step, if you both own and control ~ and a creature named Bruna, the Fading Light, exile them, then meld them into Brisela, Voice of Nightmares.
 * --MELD TO--
 * Brisela, Voice of Nightmares	""	0x000000
 * Legendary Creature - Eldrazi Angel 9/10
 * Flying, first strike, vigilance, lifelink
 * Your opponents can't cast spells with converted mana cost 3 or less. */

int card_choking_restraints(int player, int card, event_t event)
{
  /* Choking Restraints	|2|W	0x20024b5
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature can't attack or block.
   * |3|W|W, Sacrifice ~: Exile enchanted creature. */

  if (IS_GAA_EVENT(event))
	{
	  if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  int p = inst->damage_target_player;
		  int c = inst->damage_target_card;
		  if (p >= 0 && in_play(p, c))
			kill_card(p, c, KILL_REMOVE);
		}

	  // Not GAA_REGENERATION, that checks if {player,card} can regenerate *itself*
	  return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_XW(3,2), 0, NULL, NULL);
	}

  return card_pacifism(player, card, event);
}

/* Collective Effort	|1|W|W	0x000000
 * Sorcery
 * Escalate-Tap an untapped creature you control.
 * Choose one or more -
 * * Destroy target creature with power 4 or greater.
 * * Destroy target enchantment.
 * * Put a +1/+1 counter on each creature target player controls. */

int card_courageous_outrider(int player, int card, event_t event)
{
  /* Courageous Outrider	|3|W	0x200a9e4
   * Creature - Human Scout 3/4
   * When ~ enters the battlefield, look at the top four cards of your library. You may reveal a Human card from among them and put it into your hand. Put the rest on the bottom of your library in any order. */

  if (comes_into_play(player, card, event))
	{
	  int to_reveal = count_deck(player);
	  to_reveal = MIN(4, to_reveal);

	  if (to_reveal > 0)
		{
		  test_definition_t test;
		  new_default_test_definition(&test, 0, "Select a Human card.");
		  test.subtype = SUBTYPE_HUMAN;
		  test.subtype_flag = MATCH;
		  test.create_minideck = to_reveal;
		  test.no_shuffle = 1;

		  int card_added = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		  if (card_added != 1)
			--to_reveal;
		  if (to_reveal > 0)
			put_top_x_on_bottom(player, player, to_reveal);
		}
	}

  return 0;
}

/* Dawn Gryff	|2|W	=>vanilla	0x401000
 * Creature - Hippogriff 2/2
 * Flying */

/* Deploy the Gatewatch	|4|W|W	0x000000
 * Sorcery
 * Look at the top seven cards of your library. Put up to two planeswalker cards from among them onto the battlefield. Put the rest on the bottom of your library in a random order. */

int card_desperate_sentry(int player, int card, event_t event)
{
  /* Desperate Sentry	|2|W	0x200e2ce
   * Creature - Human Soldier 1/2
   * When ~ dies, put a 3/2 colorless Eldrazi Horror creature token onto the battlefield.
   * Delirium - ~ gets +3/+0 as long as there are four or more card types among cards in your graveyard. */

  if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	generate_token_by_id(player, card, CARD_ID_ELDRAZI_HORROR);

  if (event == EVENT_POWER && affect_me(player, card) && delirium(player) && !is_humiliated(player, card))
	event_result += 3;

  return 0;
}

int card_drogskol_shieldmate(int player, int card, event_t event)
{
  /* Drogskol Shieldmate	|2|W	0x200ec42
   * Creature - Spirit Soldier 2/3
   * Flash
   * When ~ enters the battlefield, other creatures you control get +0/+1 until end of turn. */

  if (comes_into_play(player, card, event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  pump_creatures_until_eot(player, card, player, 0, 0,1, 0,0, &test);
	}

  return flash(player, card, event);
}

/* Extricator of Sin	|2|W	0x000000
 * Creature - Human Cleric 0/3
 * When ~ enters the battlefield, you may sacrifice another permanent. If you do, put a 3/2 colorless Eldrazi Horror creature token onto the battlefield.
 * Delirium - At the beginning of your upkeep, if there are four or more card types among cards in your graveyard, transform ~.
 * --TRANSFORM--
 * Extricator of Flesh	""	0x000000
 * Creature - Eldrazi Horror 3/5
 * Eldrazi you control have vigilance.
 * |2, |T, Sacrifice a non-Eldrazi creature: Put a 3/2 colorless Eldrazi Horror creature token onto the battlefield. */

int card_faith_unbroken(int player, int card, event_t event)
{
  /* Faith Unbroken	|3|W	0x200ec47
   * Enchantment - Aura
   * Enchant creature you control
   * When ~ enters the battlefield, exile target creature an opponent controls until ~ leaves the battlefield.
   * Enchanted creature gets +2/+2. */

  return_from_oblivion(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  card_instance_t* inst = get_card_instance(player, card);

	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		obliviation(player, card, inst->targets[0].player, inst->targets[0].card);

	  inst->number_of_targets = 0;
	}

  // generic_aura() contains a bletcherous hack for "Enchant creature you control"
  return generic_aura(player, card, event, player, 2,2, 0,0, 0,0,0);
}

/* Faithbearer Paladin	|4|W	=>m10.c:card_child_of_night	0x200415c
 * Creature - Human Knight 3/4
 * Lifelink */

/* Fiend Binder	|3|W	=>m14.c:card_master_of_diversion	0x200aa8e
 * Creature - Human Soldier 3/2
 * Whenever ~ attacks, tap target creature defending player controls. */

int card_geist_of_the_lonely_vigil(int player, int card, event_t event)
{
  /* Geist of the Lonely Vigil	|1|W	0x200ec4c
   * Creature - Spirit Cleric 2/3
   * Defender, flying
   * Delirium - ~ can attack as though it didn't have defender as long as there are four or more card types among cards in your graveyard. */

  if (event == EVENT_ABILITIES && affect_me(player, card) && delirium(player) && !is_humiliated(player, card))
	add_status(player, card, STATUS_WALL_CAN_ATTACK);

  return 0;
}

/* Give No Ground	|3|W	0x000000
 * Instant
 * Target creature gets +2/+6 until end of turn and can block any number of creatures this turn. */

/* Guardian of Pilgrims	|1|W	=>lorwyn.c:card_kinsbaille_skirmisher	0x2007979
 * Creature - Spirit Cleric 2/2
 * When ~ enters the battlefield, target creature gets +1/+1 until end of turn. */

int card_ironclad_slayer(int player, int card, event_t event)
{
  /* Ironclad Slayer	|2|W	0x200ec51
   * Creature - Human Warrior 3/2
   * When ~ enters the battlefield, you may return target Aura or Equipment card from your graveyard to your hand. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, 0, "Select target Aura or Equipment card.");
	  test.subtype = SUBTYPE_AURA;
	  test.sub2 = SUBTYPE_EQUIPMENT;
	  test.subtype_flag = F2_MULTISUBTYPE;

	  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

  return 0;
}

/* Ironwright's Cleansing	|2|W	=>scars_of_mirrodin.c:card_revoke_existence	0x2004391
 * Sorcery
 * Exile target artifact or enchantment. */

int card_lone_rider(int player, int card, event_t event)
{
  /* Lone Rider	|1|W	0x200ec56
   * Creature - Human Knight 1/1
   * First strike, lifelink
   * At the beginning of the end step, if you gained 3 or more life this turn, transform ~. */

  double_faced_card(player, card, event);

  lifelink(player, card, event);

  if (trigger_condition == TRIGGER_EOT && affect_me(player, card)
	  && get_trap_condition(player, TRAP_LIFE_GAINED) >= 3
	  && eot_trigger(player, card, event))
	transform(player, card);

  return 0;
}
// --TRANSFORM--
/* It That Rides as One	""	=>m10.c:card_child_of_night	0x200415c
 * Creature - Eldrazi Horror 4/4
 * First strike, trample, lifelink */

int card_long_road_home(int player, int card, event_t event)
{
  /* Long Road Home	|1|W	0x200ec5b
   * Instant
   * Exile target creature. At the beginning of the next end step, return that card to the battlefield under its owner's control with a +1/+1 counter on it. */

  /* As with Otherworldly Journey (which is identical except that it's Arcane), the +1/+1 counter is implemented by a
   * hack in remove_until_eot(). */
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
		  remove_until_eot(player, card, inst->targets[0].player, inst->targets[0].card);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/* Lunarch Mantle	|1|W	0x000000
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature gets +2/+2 and has "|1, Sacrifice a permanent: This creature gains flying until end of turn." */

/* Peace of Mind	|1|W	=>exodus.c:card_peace_of_mind	0x20036c6
 * Enchantment
 * |W, Discard a card: You gain 3 life. */

int card_providence(int player, int card, event_t event)
{
  /* Providence	|5|W|W	0x200ec60
   * Sorcery
   * You may reveal this card from your opening hand. If you do, at the beginning of the first upkeep, your life total becomes 26.
   * Your life total becomes 26. */

  // First ability approximated with the Rules Engine.

  if (event == EVENT_RESOLVE_SPELL)
	{
	  set_life_total(player, 26);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_repel_the_abominable(int player, int card, event_t event)
{
  /* Repel the Abominable	|1|W	0x200ec65
   * Instant
   * Prevent all damage that would be dealt this turn by non-Human sources. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  fog_special2(player, card, ANYBODY, FOG_BY_ANY_TYPE | FOG_BY_NON_SUBTYPE, SUBTYPE_HUMAN);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_sanctifier_of_souls(int player, int card, event_t event)
{
  /* Sanctifier of Souls	|3|W	0x200ec6a
   * Creature - Human Cleric 2/3
   * Whenever another creature enters the battlefield under your control, ~ gets +1/+1 until end of turn.
   * |2|W, Exile a creature card from your graveyard: Put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		pump_until_eot_merge_previous(player, card, player, card, 1,1);
	}

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_ACTIVATE(player, card, MANACOST_XW(2,1)) && any_in_graveyard_by_type(player, TYPE_CREATURE);

  if (event == EVENT_ACTIVATE)
	{
	  charge_mana_for_activated_ability(player, card, MANACOST_XW(2,1));
	  if (cancel == 1)
		return 0;

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card to exile.");
	  int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MIN_VALUE, -1, &test);
	  if (selected == -1)
		{
		  cancel = 1;
		  return 0;
		}

	  remove_card_from_grave(player, selected);
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_SPIRIT, &token);
	  token.key_plus = KEYWORD_FLYING;
	  token.color_forced = COLOR_TEST_WHITE;
	  generate_token(&token);
	}

  return 0;
}

/* Selfless Spirit	|1|W	=>alara_reborn.c:card_dauntless_escort	0x20083ce
 * Creature - Spirit Cleric 2/1
 * Flying
 * Sacrifice ~: Creatures you control gain indestructible until end of turn. */

/* Sigarda's Aid	|W	0x000000
 * Enchantment
 * You may cast Aura and Equipment spells as though they had flash.
 * Whenever an Equipment enters the battlefield under your control, you may attach it to target creature you control. */

/* Sigardian Priest	|1|W	=>innistrad.c:card_avacynian_priest	0x2004823
 * Creature - Human Cleric 1/2
 * |1, |T: Tap target non-Human creature. */

int card_spectral_reserves(int player, int card, event_t event)
{
  /* Spectral Reserves	|3|W	0x200ec6f
   * Sorcery
   * Put two 1/1 |Swhite Spirit creature tokens with flying onto the battlefield. You gain 2 life. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  token_generation_t token;
	  default_token_definition(player, card, CARD_ID_SPIRIT, &token);
	  token.qty = 2;
	  token.key_plus = KEYWORD_FLYING;
	  token.color_forced = COLOR_TEST_WHITE;
	  generate_token(&token);

	  gain_life(player, 2);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_steadfast_cathar(int player, int card, event_t event)
{
  /* Steadfast Cathar	|1|W	0x200ec74
   * Creature - Human Soldier 2/1
   * Whenever ~ attacks, it gets +0/+2 until end of turn. */

  return when_attacks_pump_self(player, card, event, 0,2);
}

int card_subjugator_angel(int player, int card, event_t event)
{
  /* Subjugator Angel	|4|W|W	0x200ec88
   * Creature - Angel 4/3
   * Flying
   * When ~ enters the battlefield, tap all creatures your opponents control. */

  if (comes_into_play(player, card, event))
	manipulate_all(player, card, 1-player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_TAP);

  return 0;
}

int card_thalia_heretic_cathar(int player, int card, event_t event)
{
  /* Thalia, Heretic Cathar	|2|W	0x200ec8d
   * Legendary Creature - Human Soldier 3/2
   * First strike
   * Creatures and nonbasic lands your opponents control enter the battlefield tapped. */

  check_legend_rule(player, card, event);

  if (event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "");
	  test.subtype = SUBTYPE_BASIC;
	  test.subtype_flag = DOESNT_MATCH;

	  permanents_enters_battlefield_tapped(player, card, event, 1-player, TYPE_CREATURE, &test);
	}

  return 0;
}

int card_thalias_lancers(int player, int card, event_t event)
{
  /* Thalia's Lancers	|3|W|W	0x200ec92
   * Creature - Human Knight 4/4
   * First strike
   * When ~ enters the battlefield, you may search your library for a legendary card, reveal it, put it into your hand, then shuffle your library. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, 0, "Select a legendary card.");
	  test.subtype = SUBTYPE_LEGEND;
	  test.subtype_flag = MATCH;

	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

  return 0;
}

int card_thraben_standard_bearer(int player, int card, event_t event)
{
  /* Thraben Standard Bearer	|W	0x200ec97
   * Creature - Human Soldier 1/1
   * |1|W, |T, Discard a card: Put a 1/1 |Swhite Human Soldier creature token onto the battlefield. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	generate_token_by_id(player, card, CARD_ID_HUMAN_SOLDIER);

  return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_DISCARD, MANACOST_XW(1,1), 0, NULL, NULL);
}

/*** Blue ***/

/* Advanced Stitchwing	|3|U|U	0x000000
 * Creature - Zombie Horror 3/4
 * Flying
 * |2|U, Discard two cards: Return ~ from your graveyard to the battlefield tapped. */

int card_chilling_grasp(int player, int card, event_t event)
{
  /* Chilling Grasp	|2|U	0x200ec9c
   * Instant
   * Tap up to two target creatures. Those creatures don't untap during their controller's next untap step.
   * Madness |3|U */

  if (IS_GS_EVENT(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);

	  if (event == EVENT_RESOLVE_SPELL)
		{
		  card_instance_t* inst = get_card_instance(player, card);

		  int i;
		  for (i = 0; i < inst->number_of_targets; ++i)
			if (validate_target(player, card, &td, i))
			  does_not_untap_effect(player, card, inst->targets[i].player, inst->targets[i].card, EDNT_TAP_TARGET, 1);

		  kill_card(player, card, KILL_DESTROY);
		}

	  return generic_spell(player, card, event, GS_CAN_TARGET | GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
	}

  return madness(player, card, event, MANACOST_XU(3,1));
}

/* Coax from the Blind Eternities	|2|U	0x000000
 * Sorcery
 * You may choose an Eldrazi card you own from outside the game or in exile, reveal that card, and put it into your hand. */

/* Contingency Plan	|1|U	=>khans_of_tarkir.c:card_taigams_scheming	0x200d00e
 * Sorcery
 * Look at the top five cards of your library. Put any number of them into your graveyard and the rest back on top of your library in any order. */

/* Convolute	|2|U	=>ravnica.c:card_convolute	0x20066a5
 * Instant
 * Counter target spell unless its controller pays |4. */

/* Curious Homunculus	|1|U	0x000000
 * Creature - Homunculus 1/1
 * |T: Add |C to your mana pool. Spend this mana only to cast an instant or sorcery spell.
 * At the beginning of your upkeep, if there are three or more instant and/or sorcery cards in your graveyard, transform ~.
 * --TRANSFORM--
 * Voracious Reader	""	0x000000
 * Creature - Eldrazi Homunculus 3/4
 * Prowess
 * Instant and sorcery spells you cast cost |1 less to cast. */

int card_displace(int player, int card, event_t event)
{
  /* Displace	|2|U	0x200eca1
   * Instant
   * Exile up to two target creatures you control, then return those cards to the battlefield under their owner's control. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);

	  /* Yes, they should all be exiled, then all be returned, instead of interspersing the exiling and returning like
	   * this.  On the other hand, this makes any etb/ltb triggers they have see the other target like they should.
	   * Ghostly Flicker has a similar issue. */
	  int i;
	  for (i = 0; i < inst->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  blink_effect(inst->targets[i].player, inst->targets[i].card, 0);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET | GS_OPTIONAL_TARGET, &td, "ASHNODS_BATTLEGEAR", 2, NULL);
}

int card_docent_of_perfection(int player, int card, event_t event)
{
  /* Docent of Perfection	|3|U|U	0x200eca6
   * Creature - Insect Horror 5/4
   * Flying
   * Whenever you cast an instant or sorcery spell, put a 1/1 |Sblue Human Wizard creature token onto the battlefield. Then if you control three or more Wizards, transform ~. */

  double_faced_card(player, card, event);

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY,
							TYPE_SPELL,MATCH, 0,0, 0,0, 0,0, -1,0))
	{
	  generate_token_by_id(player, card, CARD_ID_HUMAN_WIZARD);
	  if (count_subtype(player, TYPE_PERMANENT, SUBTYPE_WIZARD) >= 3)
		transform(player, card);
	}

  return 0;
}
// --TRANSFORM--
int card_final_iteration(int player, int card, event_t event)
{
  /* Final Iteration	""	0x200ecab
   * Creature - Eldrazi Insect 6/5
   * Flying
   * Wizards you control get +2/+1 and have flying.
   * Whenever you cast an instant or sorcery spell, put a 1/1 |Sblue Human Wizard creature token onto the battlefield. */

  boost_creature_type(player, card, event, SUBTYPE_WIZARD, 2,1, KEYWORD_FLYING, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY,
							TYPE_SPELL,MATCH, 0,0, 0,0, 0,0, -1,0))
	generate_token_by_id(player, card, CARD_ID_HUMAN_WIZARD);

  return 0;
}

/* Drag Under	|2|U	=>invasion.c:card_repulse	0x200115a
 * Sorcery
 * Return target creature to its owner's hand.
 * Draw a card. */

int card_enlightened_maniac(int player, int card, event_t event)
{
  /* Enlightened Maniac	|3|U	0x200ecb0
   * Creature - Human 0/2
   * When ~ enters the battlefield, put a 3/2 colorless Eldrazi Horror creature token onto the battlefield. */

  if (comes_into_play(player, card, event))
	generate_token_by_id(player, card, CARD_ID_ELDRAZI_HORROR);

  return 0;
}

/* Exultant Cultist	|2|U	=>scars_of_mirrodin.c:card_darkslick_drake	0x2004201
 * Creature - Human Wizard 2/2
 * When ~ dies, draw a card. */

int card_fogwalker(int player, int card, event_t event)
{
  /* Fogwalker	|1|U	0x200ecb5
   * Creature - Spirit 1/3
   * Skulk
   * When ~ enters the battlefield, target creature an opponent controls doesn't untap during its controller's next untap step. */

  skulk(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;
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

  return 0;
}

/* Fortune's Favor	|3|U	0x000000
 * Instant
 * Target opponent looks at the top four cards of your library and separates them into a face-down pile and a face-up pile. Put one pile into your hand and the other into your graveyard. */

int card_geist_of_the_archives(int player, int card, event_t event)
{
  /* Geist of the Archives	|2|U	0x200ecba
   * Creature - Spirit 0/4
   * Defender
   * At the beginning of your upkeep, scry 1. */

  upkeep_trigger_ability(player, card, event, player);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	scry(player, 1);

  return 0;
}

int card_grizzled_angler(int player, int card, event_t event)
{
  /* Grizzled Angler	|2|U	0x200ecbf
   * Creature - Human 2/3
   * |T: Put the top two cards of your library into your graveyard. Then if there is a colorless creature card in your graveyard, transform ~. */

  double_faced_card(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  mill(player, 2);

	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.color = COLOR_TEST_ANY_COLORED;
	  test.color_flag = DOESNT_MATCH;

	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card)
		  && !check_special_flags3(inst->parent_controller, inst->parent_card, SF3_CARD_IS_FLIPPED)
		  && new_special_count_grave(player, &test))
		transform(inst->parent_controller, inst->parent_card);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}
// --TRANSFORM--
int card_grisly_anglerfish(int player, int card, event_t event)
{
  /* Grisly Anglerfish	""	0x200ecc4
   * Creature - Eldrazi Fish 4/5
   * |6: Creatures your opponents control attack this turn if able. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	pump_subtype_until_eot(player, card, 1-player, -1, 0,0, 0,SP_KEYWORD_MUST_ATTACK);

  return generic_activated_ability(player, card, event, GAA_NONE, MANACOST_X(6), 0, NULL, NULL);
}

/* Identity Thief	|2|U|U	0x000000
 * Creature - Shapeshifter 0/3
 * Whenever ~ attacks, you may exile another target nontoken creature. If you do, ~ becomes a copy of that creature until end of turn. Return the exiled card to the battlefield under its owner's control at the beginning of the next end step. */

/* Imprisoned in the Moon	|2|U	0x000000
 * Enchantment - Aura
 * Enchant creature, land, or planeswalker
 * Enchanted permanent is a colorless land with "|T: Add |C to your mana pool" and loses all other card types and abilities. */

int card_ingenious_skaab(int player, int card, event_t event)
{
  /* Ingenious Skaab	|2|U	0x200ecc9
   * Creature - Zombie Horror 2/3
   * Prowess
   * |U: ~ gets +1/-1 until end of turn. */

  prowess(player, card, event);

  return generic_shade_merge_pt(player, card, event, 0, MANACOST_U(1), 1,-1);
}

/* Laboratory Brute	|3|U	=>innistrad.c:card_armored_skaab	0x2004819
 * Creature - Zombie Horror 3/3
 * When ~ enters the battlefield, put the top four cards of your library into your graveyard. */

int card_lunar_force(int player, int card, event_t event)
{
  /* Lunar Force	|2|U	0x200ecce
   * Enchantment
   * When an opponent casts a spell, sacrifice ~ and counter that spell. */

  if (new_specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, NULL))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  target_t spl = inst->targets[1];	// struct copy
	  kill_card(player, card, KILL_SACRIFICE);
	  kill_card(spl.player, spl.card, KILL_SACRIFICE);
	}

  return global_enchantment(player, card, event);
}

int card_mausoleum_wanderer(int player, int card, event_t event)
{
  /* Mausoleum Wanderer	|U	0x200ecd3
   * Creature - Spirit 1/1
   * Flying
   * Whenever another Spirit enters the battlefield under your control, ~ gets +1/+1 until end of turn.
   * Sacrifice ~: Counter target instant or sorcery spell unless its controller pays |X, where X is ~'s power. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card))
	{
	  test_definition_t this_test;
	  new_default_test_definition(&this_test, TYPE_PERMANENT, "");
	  this_test.subtype = SUBTYPE_SPIRIT;
	  this_test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &this_test))
		pump_until_eot_merge_previous(player, card, player, card, 1,1);
	}

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  counterspell_target_definition(player, card, &td, TYPE_SPELL);

  if (event == EVENT_ACTIVATE)
	{
	  int pow = get_power(player, card);
	  if (pow <= 0 || has_mana(card_on_stack_controller, COLOR_COLORLESS, pow))
		ai_modifier -= 20;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	counterspell_resolve_unless_pay_x(player, card, &td, 0, get_power(player, card));

  return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME|GAA_SPELL_ON_STACK, MANACOST0, 0, &td, NULL);
}

/* Mind's Dilation	|5|U|U	0x000000
 * Enchantment
 * Whenever an opponent casts his or her first spell each turn, that player exiles the top card of his or her library. If it's a nonland card, you may cast it without paying its mana cost. */

int card_nebelgast_herald(int player, int card, event_t event)
{
  /* Nebelgast Herald	|2|U	0x200ecd8
   * Creature - Spirit 2/1
   * Flash
   * Flying
   * Whenever ~ or another Spirit enters the battlefield under your control, tap target creature an opponent controls. */
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card)
	  && reason_for_trigger_controller == player && trigger_cause_controller == player)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_PERMANENT, "");
	  test.subtype = SUBTYPE_SPIRIT;
	  test.subtype_flag = MATCH;
	  test.not_me = 2;	// === pass the test if it's this card.  Only valid for new_specific_cip().

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  if (can_target(&td)
		  && new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  if (pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
			tap_card(inst->targets[0].player, inst->targets[0].card);
		}
	}

  return flash(player, card, event);
}

int card_niblis_of_frost(int player, int card, event_t event)
{
  /* Niblis of Frost	|2|U|U	0x200ecdd
   * Creature - Spirit 3/3
   * Flying
   * Prowess
   * Whenever you cast an instant or sorcery spell, tap target creature an opponent controls. That creature doesn't untap during its controller's next untap step. */

  if (prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  alternate_legacy_text(1, player, pump_until_eot_merge_previous(player, card, player, card, 1, 1));
	  if (is_what(trigger_cause_controller, trigger_cause, TYPE_SPELL))
		{
		  target_definition_t td;
		  default_target_definition(player, card, &td, TYPE_CREATURE);
		  td.allowed_controller = 1-player;
		  td.allow_cancel = 0;

		  card_instance_t* inst = get_card_instance(player, card);
		  inst->number_of_targets = 0;
		  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
			alternate_legacy_text(2, player,
								  does_not_untap_effect(player, card,
														inst->targets[0].player, inst->targets[0].card, 0, 1));
		}
	}

  return 0;
}

int card_scour_the_laboratory(int player, int card, event_t event)
{
  /* Scour the Laboratory	|4|U|U	0x200ece2
   * Instant
   * Delirium - ~ costs |2 less to cast if there are four or more card types among cards in your graveyard.
   * Draw three cards. */

  if (event == EVENT_MODIFY_COST && delirium(player))
	COST_COLORLESS -= 2;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  draw_cards(player, 3);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_spontaneous_mutation(int player, int card, event_t event)
{
  /* Spontaneous Mutation	|U	0x200ece7
   * Enchantment - Aura
   * Flash
   * Enchant creature
   * Enchanted creature gets -X/-0, where X is the number of cards in your graveyard. */

  if (event == EVENT_CHECK_PUMP)
	return vanilla_instant_pump(player, card, event, ANYBODY, 1-player, -count_graveyard(player),0, 0,0);

  int pow;
  card_instance_t* inst;
  if (event == EVENT_POWER
	  && (inst = in_play(player, card))
	  && affect_me(inst->damage_target_player, inst->damage_target_card))
	pow = -count_graveyard(player);
  else
	pow = -1;

  return generic_aura(player, card, event, 1-player, pow,0, 0,0, 0,0,0);
}

/* Summary Dismissal	|2|U|U	0x000000
 * Instant
 * Exile all other spells and counter all abilities. */

int card_take_inventory(int player, int card, event_t event)
{
  /* Take Inventory	|1|U	0x200ecec
   * Sorcery
   * Draw a card, then draw cards equal to the number of cards named ~ in your graveyard. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  draw_cards(player, 1 + count_graveyard_by_id(player, CARD_ID_TAKE_INVENTORY));
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

/* Tattered Haunter	|1|U	=>m12.c:card_skywinder_drake	0x2004049
 * Creature - Spirit 2/1
 * Flying
 * ~ can block only creatures with flying. */

/* Turn Aside	|U	=>scars_of_mirrodin.c:card_turn_aside	0x2004431
 * Instant
 * Counter target spell that targets a permanent you control. */

/* Unsubstantiate	|1|U	0x000000
 * Instant
 * Return target spell or creature to its owner's hand. */

int card_wharf_infiltrator(int player, int card, event_t event)
{
  /* Wharf Infiltrator	|1|U	0x200ecf1
   * Creature - Human Horror 1/1
   * Skulk
   * Whenever ~ deals combat damage to a player, you may draw a card. If you do, discard a card.
   * Whenever you discard a creature card, you may pay |2. If you do, put a 3/2 colorless Eldrazi Horror creature token onto the battlefield. */

  skulk(player, card, event);

  if (damage_dealt_by_me(player, card, event,
						 DDBM_TRIGGER_OPTIONAL | DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_BE_COMBAT_DAMAGE)
	  && can_draw_cards_as_cost(player, 1))
	{
	  draw_a_card(player);
	  discard(player, 0, player);
	}

  if (trigger_condition == TRIGGER_DISCARD && affect_me(player, card)
	  && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE)
	  && discard_trigger(player, card, event, player, RESOLVE_TRIGGER_AI(player), 0)
	  && charge_mana_while_resolving(player, card, event, player, COLOR_COLORLESS, 2))
	generate_token_by_id(player, card, CARD_ID_ELDRAZI_HORROR);

  return 0;
}

/*** Black ***/

int card_boon_of_emrakul(int player, int card, event_t event)
{
  /* Boon of Emrakul	|2|B	0x200ecf6
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +3/-3. */

  return generic_aura(player, card, event, ANYBODY, 3,-3, 0,0, 0,0,0);
}

/* Borrowed Malevolence	|B	0x000000
 * Instant
 * Escalate |2
 * Choose one or both -
 * * Target creature gets +1/+1 until end of turn.
 * * Target creature gets -1/-1 until end of turn. */

int card_cemetery_recruitment(int player, int card, event_t event)
{
  /* Cemetery Recruitment	|1|B	0x200ecfb
   * Sorcery
   * Return target creature card from your graveyard to your hand. If it's a Zombie card, draw a card. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  test_definition_t test;
  new_default_test_definition(&test, TYPE_ARTIFACT, "Select target creature card.");

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int pos = validate_target_from_grave_source(player, card, player, 0);
	  if (pos != -1)
		{
		  int zombie = has_subtype(-1, get_grave(player)[pos], SUBTYPE_ZOMBIE);
		  from_grave_to_hand(player, pos, TUTOR_HAND);
		  if (zombie)
			draw_a_card(player);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &test);
}

int card_certain_death(int player, int card, event_t event)
{
  /* Certain Death	|5|B	0x200ed00
   * Sorcery
   * Destroy target creature. Its controller loses 2 life and you gain 2 life. */

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
		  lose_life(inst->targets[0].player, 2);
		  gain_life(player, 2);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/* Collective Brutality	|1|B	0x000000
 * Sorcery
 * Escalate-Discard a card.
 * Choose one or more -
 * * Target opponent reveals his or her hand. You choose an instant or sorcery card from it. That player discards that card.
 * * Target creature gets -2/-2 until end of turn.
 * * Target opponent loses 2 life and you gain 2 life. */

int card_cryptbreaker(int player, int card, event_t event)
{
  /* Cryptbreaker	|B	0x200ed05
   * Creature - Zombie 1/1
   * |1|B, |T, Discard a card: Put a 2/2 |Sblack Zombie creature token onto the battlefield.
   * Tap three untapped Zombies you control: You draw a card and you lose 1 life. */

  if (!IS_ACTIVATING(event))
	return 0;

  target_definition_t zombie;
  base_target_definition(player, card, &zombie, TYPE_PERMANENT);
  zombie.allowed_controller = zombie.preferred_controller = player;
  zombie.required_subtype = SUBTYPE_ZOMBIE;
  zombie.illegal_state = TARGET_STATE_TAPPED;

  int can_token = hand_count[player] >= 1;
  int can_draw = target_available(player, card, &zombie) >= 3;

  enum
  {
	CHOICE_TOKEN = 1,
	CHOICE_DRAW
  } choice = DIALOG(player, card, event,
					DLG_RANDOM,
					"Zombie token",			can_token,	2,	DLG_MANA(MANACOST_XB(1,1)), DLG_TAP,
					"Draw and lose life",	can_draw,	3,	DLG_MANA(MANACOST0));

  if (event == EVENT_CAN_ACTIVATE)
	return choice && can_use_activated_abilities(player, card);
  else if (event == EVENT_ACTIVATE)
	switch (choice)
	  {
		case CHOICE_TOKEN:
		  discard(player, 0, player);
		  break;

		case CHOICE_DRAW:
		  if (!tapsubtype_ability(player, card, 3, &zombie))
			cancel = 1;
		  break;
	  }
  else	// event == EVENT_ACTIVATION
	switch (choice)
	  {
		case CHOICE_TOKEN:
		  generate_token_by_id(player, card, CARD_ID_ZOMBIE);
		  break;

		case CHOICE_DRAW:
		  draw_a_card(player);
		  lose_life(player, 1);
		  break;
	  }

  return 0;
}

/* Dark Salvation	|X|X|B	0x000000
 * Sorcery
 * Target player puts X 2/2 |Sblack Zombie creature tokens onto the battlefield, then up to one target creature gets -1/-1 until end of turn for each Zombie that player controls. */

int card_dusk_feaster(int player, int card, event_t event)
{
  /* Dusk Feaster	|5|B|B	0x200ed0a
   * Creature - Vampire 4/5
   * Delirium - ~ costs |2 less to cast if there are four or more card types among cards in your graveyard.
   * Flying */

  if (event == EVENT_MODIFY_COST && delirium(player))
	COST_COLORLESS -= 2;

  return 0;
}

/* Gavony Unhallowed	|3|B	=>innistrad.c:card_unruly_mob	0x2004aad
 * Creature - Zombie 2/4
 * Whenever another creature you control dies, put a +1/+1 counter on ~. */

/* Graf Harvest	|B	0x000000
 * Enchantment
 * Zombies you control have menace.
 * |3|B, Exile a creature card from your graveyard: Put a 2/2 |Sblack Zombie creature token onto the battlefield. */

/* Graf Rats	|1|B	0x000000
 * Creature - Rat 2/1
 * At the beginning of combat on your turn, if you both own and control ~ and a creature named Midnight Scavengers, exile them, then meld them into Chittering Host.
 * --MELD WITH--
 * Midnight Scavengers	|4|B	0x000000
 * Creature - Human Rogue 3/3
 * When ~ enters the battlefield, you may return target creature card with converted mana cost 3 or less from your graveyard to your hand.
 * --MELD TO--
 * Chittering Host	""
 * Creature - Eldrazi Horror 5/6	0x000000
 * Haste
 * Menace
 * When ~ enters the battlefield, other creatures you control get +1/+0 and gain menace until end of turn. */

/* Haunted Dead	|3|B	0x000000
 * Creature - Zombie 2/2
 * When ~ enters the battlefield, put a 1/1 |Swhite Spirit creature token with flying onto the battlefield.
 * |1|B, Discard two cards: Return ~ from your graveyard to the battlefield tapped. */

/* Liliana, the Last Hope	|1|B|B	0x000000
 * Planeswalker - Liliana (3)
 * +1: Up to one target creature gets -2/-1 until your next turn.
 * -2: Put the top two cards of your library into your graveyard, then you may return a creature card from your graveyard to your hand.
 * -7: You get an emblem with "At the beginning of your end step, put X 2/2 |Sblack Zombie creature tokens onto the battlefield, where X is two plus the number of Zombies you control." */

/* Liliana, the Last Hope's Emblem	""	0x000000
 * Emblem - Liliana
 * At the beginning of your end step, put X 2/2 |Sblack Zombie creature tokens onto the battlefield, where X is two plus the number of Zombies you control. */

int card_lilianas_elite(int player, int card, event_t event)
{
  /* Liliana's Elite	|2|B	0x200ed0f
   * Creature - Zombie 1/1
   * ~ gets +1/+1 for each creature card in your graveyard. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card))
	event_result += count_graveyard_by_type(player, TYPE_CREATURE);

  return 0;
}

int card_markov_crusader(int player, int card, event_t event)
{
  /* Markov Crusader	|4|B	0x200ed14
   * Creature - Vampire Knight 4/3
   * Lifelink
   * ~ has haste as long as you control another Vampire. */

  lifelink(player, card, event);

  if (event == EVENT_ABILITIES && affect_me(player, card) && !is_humiliated(player, card))
	{
	  int c;
	  for (c = 0; c < active_cards_count[player]; ++c)
		if (in_play(player, c) && c != card && has_subtype(player, c, SUBTYPE_VAMPIRE))
		  {
			haste(player, card, event);
			break;
		  }
	}

  return 0;
}

/* Murder	|1|B|B	=>m13.c:card_murder2	0x20067ea
 * Instant
 * Destroy target creature. */

int card_noosegraf_mob(int player, int card, event_t event)
{
  /* Noosegraf Mob	|4|B|B	0x200ed19
   * Creature - Zombie 0/0
   * ~ enters the battlefield with five +1/+1 counters on it.
   * Whenever a player casts a spell, remove a +1/+1 counter from ~. If you do, put a 2/2 |Sblack Zombie creature token onto the battlefield. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 5);

  if (specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, 0,0, 0,0, 0,0, 0,0, -1,0)
	  && count_1_1_counters(player, card) >= 1)
	{
	  remove_1_1_counter(player, card);
	  generate_token_by_id(player, card, CARD_ID_ZOMBIE);
	}

  return 0;
}

/* Oath of Liliana	|2|B	0x000000
 * Legendary Enchantment
 * When ~ enters the battlefield, each opponent sacrifices a creature.
 * At the beginning of each end step, if a planeswalker entered the battlefield under your control this turn, put a 2/2 |Sblack Zombie creature token onto the battlefield. */

int card_olivias_dragoon(int player, int card, event_t event)
{
  /* Olivia's Dragoon	|1|B	0x200ed1e
   * Creature - Vampire Berserker 2/2
   * Discard a card: ~ gains flying until end of turn. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card))
		pump_ability_until_eot_no_repeat(player, card, inst->parent_controller, inst->parent_card, KEYWORD_FLYING,0);
	}

  return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST0, 0, NULL, NULL);
}

int card_prying_questions(int player, int card, event_t event)
{
  /* Prying Questions	|2|B	0x200ed23
   * Sorcery
   * Target opponent loses 3 life and puts a card from his or her hand on top of his or her library. */

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
		  int oppo = get_card_instance(player, card)->targets[0].player;

		  lose_life(oppo, 3);

		  if (hand_count[oppo] >= 1)
			{
			  test_definition_t test;
			  new_default_test_definition(&test, TYPE_ANY, "Select a card to put on top.");
			  new_global_tutor(oppo, oppo, TUTOR_FROM_HAND, TUTOR_DECK, 1, AI_MIN_VALUE, &test);
			}
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_OPPONENT", 1, NULL);
}

/* Rise from the Grave	|4|B	=>m11.c:card_rise_from_the_grave	0x2002735
 * Sorcery
 * Put target creature card from a graveyard onto the battlefield under your control. That creature is a |Sblack Zombie in addition to its other colors and types. */

/* Ruthless Disposal	|4|B	0x000000
 * Sorcery
 * As an additional cost to cast ~, discard a card and sacrifice a creature.
 * Two target creatures each get -13/-13 until end of turn. */

int card_skirsdag_supplicant(int player, int card, event_t event)
{
  /* Skirsdag Supplicant	|2|B	0x200ed28
   * Creature - Human Cleric 2/3
   * |B, |T, Discard a card: Each player loses 2 life. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  lose_life(current_turn, 2);
	  lose_life(1-current_turn, 2);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_DISCARD, MANACOST_B(1), 0, NULL, NULL);
}

int card_strange_augmentation(int player, int card, event_t event)
{
  /* Strange Augmentation	|B	0x200ed2d
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +1/+1.
   * Delirium - Enchanted creature gets an additional +2/+2 as long as there are four or more card types among cards in your graveyard. */

  int pt;
  card_instance_t* inst;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && (inst = in_play(player, card))
	  && affect_me(inst->damage_target_player, inst->damage_target_card)
	  && delirium(player))
	pt = 3;
  else
	pt = 1;

  return generic_aura(player, card, event, player, pt,pt, 0,0, 0,0,0);
}

int card_stromkirk_condemned(int player, int card, event_t event)
{
  /* Stromkirk Condemned	|B|B	0x200ed32
   * Creature - Vampire Horror 2/2
   * Discard a card: Vampires you control get +1/+1 until end of turn. Activate this ability only once each turn. */

  if (event == EVENT_CHECK_PUMP && card_stromkirk_condemned(player, card, EVENT_CAN_ACTIVATE))
	{
	  pumpable_power[player] += 1;
	  pumpable_toughness[player] += 1;
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, 0, "");
	  test.subtype = SUBTYPE_VAMPIRE;
	  test.subtype_flag = MATCH;

	  pump_creatures_until_eot_merge_pt(player, card, player, 1,1, &test);
	}

  return generic_activated_ability(player, card, event, GAA_DISCARD | GAA_ONCE_PER_TURN, MANACOST0, 0, NULL, NULL);
}

/* Succumb to Temptation	|1|B|B	=>fifth_dawn.c:card_nights_whispers	0x200cd39
 * Instant
 * You draw two cards and you lose 2 life. */

/* Thraben Foulbloods	|2|B	0x000000
 * Creature - Zombie Hound 3/2
 * Delirium - ~ gets +1/+1 and has menace as long as there are four or more card types among cards in your graveyard. */

int card_tree_of_perdition(int player, int card, event_t event)
{
  /* Tree of Perdition	|3|B	0x200ed37
   * Creature - Plant 0/13
   * Defender
   * |T: Exchange target opponent's life total with ~'s toughness. */

  // Deliberately not !is_humiliated() - the exchange is unaffected even if this card later loses its abilities
  if (event == EVENT_TOUGHNESS && affect_me(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (HIWORD(inst->targets[1].card) == CARD_ID_TREE_OF_PERDITION)
		event_result = SLOWORD(inst->targets[1].card);
	}

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.allowed_controller = 1-player;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  card_instance_t* parent = in_play(inst->parent_controller, inst->parent_card);
	  if (parent)
		{
		  int tgh = get_toughness(inst->parent_controller, inst->parent_card);
		  parent->targets[1].card = (CARD_ID_TREE_OF_PERDITION << 16) | LOWORD(life[inst->targets[0].player]);
		  set_life_total(inst->targets[0].player, tgh);
		}
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_ONLY_TARGET_OPPONENT,
								   MANACOST0, 0, &td, "TARGET_OPPONENT");
}

/* Vampire Cutthroat	|B	=>shadows_over_innistrad.c:card_farbog_revenant	0x200e931
 * Creature - Vampire Rogue 1/1
 * Skulk
 * Lifelink */

int card_voldaren_pariah(int player, int card, event_t event)
{
  /* Voldaren Pariah	|3|B|B	0x200ed41
   * Creature - Vampire Horror 3/3
   * Flying
   * Sacrifice three other creatures: Transform ~.
   * Madness |B|B|B */

  double_faced_card(player, card, event);

  if (IS_ACTIVATING(event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "Select another creature to sacrifice.");
	  test.not_me = 1;
	  test.qty = 3;

	  if (event == EVENT_CAN_ACTIVATE)
		return CAN_ACTIVATE0(player, card) && new_can_sacrifice_as_cost(player, card, &test);

	  if (event == EVENT_ACTIVATE && charge_mana_for_activated_ability(player, card, MANACOST0))
		new_sacrifice(player, card, player, SAC_ALL_OR_NONE|SAC_AS_COST, &test);

	  if (event == EVENT_RESOLVE_ACTIVATION)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  if (in_play(inst->parent_controller, inst->parent_card)
			  && !check_special_flags3(inst->parent_controller, inst->parent_card, SF3_CARD_IS_FLIPPED))
			transform(inst->parent_controller, inst->parent_card);
		}
	}

  return madness(player, card, event, MANACOST_B(3));
}
// --TRANSFORM--
int card_abolisher_of_bloodlines(int player, int card, event_t event)
{
  /* Abolisher of Bloodlines	""	0x200ed46
   * Creature - Eldrazi Vampire 6/5
   * Flying
   * When this creature transforms into ~, target opponent sacrifices three creatures. */

  if (event == EVENT_TRANSFORMED && target_opponent(player, card))
	impose_sacrifice(player, card, 1-player, 3, TYPE_CREATURE,MATCH, 0,0, 0,0, 0,0, -1,0);

  return 0;
}

/* Wailing Ghoul	|1|B	=>dark_ascension.c:card_screeching_skaab	0x2004e77
 * Creature - Zombie 1/3
 * When ~ enters the battlefield, put the top two cards of your library into your graveyard. */

/* Weirded Vampire	|3|B	=>shadows_over_innistrad.c:card_twins_of_maurer_estate	0x200e9cc
 * Creature - Vampire Horror 3/3
 * Madness |2|B */

int card_whispers_of_emrakul(int player, int card, event_t event)
{
  /* Whispers of Emrakul	|1|B	0x200ed4b
   * Sorcery
   * Target opponent discards a card at random.
   * Delirium - If there are four or more card types among cards in your graveyard, that player discards two cards at random instead. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.allowed_controller = 1-player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		new_multidiscard(get_card_instance(player, card)->targets[0].player,
						 delirium(player) ? 2 : 1, DISC_RANDOM, player);

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_OPPONENT", 1, NULL);
}

/*** Red ***/

int card_abandon_reason(int player, int card, event_t event)
{
  /* Abandon Reason	|2|R	0x200ed50
   * Instant
   * Up to two target creatures each get +1/+0 and gain first strike until end of turn.
   * Madness |1|R */

  if (event == EVENT_CHECK_PUMP)
	return vanilla_instant_pump(player, card, event, player, player, 1,0, KEYWORD_FIRST_STRIKE,0);

  if (IS_GS_EVENT(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;

	  if (event == EVENT_RESOLVE_SPELL)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  int i;
		  for (i = 0; i < inst->number_of_targets; ++i)
			if (validate_target(player, card, &td, i))
			  pump_ability_until_eot(player, card, inst->targets[i].player, inst->targets[i].card,
									 1,0, KEYWORD_FIRST_STRIKE,0);

		  kill_card(player, card, KILL_DESTROY);
		}

	  return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
	}

  return madness(player, card, event, MANACOST_XR(1,1));
}

int card_alchemists_greeting(int player, int card, event_t event)
{
  /* Alchemist's Greeting	|4|R	0x200ed55
   * Sorcery
   * ~ deals 4 damage to target creature.
   * Madness |1|R */

  if (IS_GS_EVENT(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);

	  if (event == EVENT_RESOLVE_SPELL)
		{
		  if (valid_target(&td))
			damage_target0(player, card, 4);

		  kill_card(player, card, KILL_DESTROY);
		}

	  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

  return madness(player, card, event, MANACOST_XR(1,1));
}

/* Assembled Alphas	|5|R	0x000000
 * Creature - Wolf 5/5
 * Whenever ~ blocks or becomes blocked by a creature, ~ deals 3 damage to that creature and 3 damage to that creature's controller. */

int card_bedlam_reveler(int player, int card, event_t event)
{
  /* Bedlam Reveler	|6|R|R	0x200ed5a
   * Creature - Devil Horror 3/4
   * ~ costs |1 less to cast for each instant and sorcery card in your graveyard.
   * Prowess
   * When ~ enters the battlefield, discard your hand, then draw three cards. */

  if (event == EVENT_MODIFY_COST)
	COST_COLORLESS -= count_graveyard_by_type(player, TYPE_SPELL);

  prowess(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  discard_all(player);
	  draw_cards(player, 3);
	}

  return 0;
}

int card_blood_mist(int player, int card, event_t event)
{
  /* Blood Mist	|3|R	0x200ed5f
   * Enchantment
   * At the beginning of combat on your turn, target creature you control gains double strike until end of turn. */

  if (event == EVENT_MUST_ATTACK && current_turn == player && !is_humiliated(player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = td.preferred_controller = player;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "ASHNODS_BATTLEGEAR"))	// "Select target creature you control"
		pump_ability_until_eot(player, card, inst->targets[0].player, inst->targets[0].card,
							   0,0, KEYWORD_DOUBLE_STRIKE,0);
	}

  return global_enchantment(player, card, event);
}

/* Bold Impaler	|R	=>battle_for_zendikar.c:card_lavastep_raider	0x200e08f
 * Creature - Vampire Knight 1/2
 * |2|R: ~ gets +2/+0 until end of turn. */

/* Borrowed Hostility	|R	0x000000
 * Instant
 * Escalate |3
 * Choose one or both -
 * * Target creature gets +3/+0 until end of turn.
 * * Target creature gains first strike until end of turn. */

/* Brazen Wolves	|2|R	=>portal_1_2_3k.c:card_lurking_nightstalker	0x200968e
 * Creature - Wolf 2/3
 * Whenever ~ attacks, it gets +2/+0 until end of turn. */

/* Collective Defiance	|1|R|R	0x000000
 * Sorcery
 * Escalate |1
 * Choose one or more -
 * * Target player discards all the cards in his or her hand, then draws that many cards.
 * * ~ deals 4 damage to target creature.
 * * ~ deals 3 damage to target opponent. */

static int fx_produce_mana_your_next_main_phase_this_turn(int player, int card, event_t event)
{
  /* player: who gets mana
   * targets[1].player: amt of colorless
   * targets[1].card: amt of black
   * targets[2].player: amt of blue
   * targets[2].card: amt of green
   * targets[3].player: amt of red
   * targets[3].card: amt of white */

  if (event == EVENT_PHASE_CHANGED
	  && current_turn == player
	  && (current_phase == PHASE_MAIN1 || current_phase == PHASE_MAIN2))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  produce_mana_multi(player,
						 inst->targets[1].player, inst->targets[1].card,
						 inst->targets[2].player, inst->targets[2].card,
						 inst->targets[3].player, inst->targets[3].card);
	  kill_card(player, card, KILL_REMOVE);
	}

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
void produce_mana_your_next_main_phase_this_turn(int player, int card, int x, int b, int u, int g, int r, int w)
{
  if (current_turn != player)
	return;
  int c = create_legacy_effect(player, card, &fx_produce_mana_your_next_main_phase_this_turn);
  if (c == -1)
	return;
  card_instance_t* fx = get_card_instance(player, c);
  fx->targets[1].player = x;
  fx->targets[1].card = b;
  fx->targets[2].player = u;
  fx->targets[2].card = g;
  fx->targets[3].player = r;
  fx->targets[3].card = w;
}

int card_conduit_of_storms(int player, int card, event_t event)
{
  /* Conduit of Storms	|2|R	0x200ed64
   * Creature - Werewolf Horror 2/3
   * Whenever ~ attacks, add |R to your mana pool at the beginning of your next main phase this turn.
   * |3|R|R: Transform ~. */

  double_faced_card(player, card, event);

  if (declare_attackers_trigger(player, card, event, 0, player, card))
	produce_mana_your_next_main_phase_this_turn(player, card, MANACOST_R(1));

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card)
		  && !check_special_flags3(inst->parent_controller, inst->parent_card, SF3_CARD_IS_FLIPPED))
		transform(inst->parent_controller, inst->parent_card);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_XR(3,2), 0, NULL, NULL);
}
// --TRANSFORM--
int card_conduit_of_emrakul(int player, int card, event_t event)
{
  /* Conduit of Emrakul	""	0x200ed69
   * Creature - Eldrazi Werewolf 5/4
   * Whenever ~ attacks, add |C|C to your mana pool at the beginning of your next main phase this turn. */

  if (declare_attackers_trigger(player, card, event, 0, player, card))
	produce_mana_your_next_main_phase_this_turn(player, card, MANACOST_X(2));

  return 0;
}

/* Deranged Whelp	|1|R	= gatecrash.c:card_ripscale_predators
 * Creature - Wolf 2/1
 * Menace */

int card_distemper_of_the_blood(int player, int card, event_t event)
{
  /* Distemper of the Blood	|1|R	0x200ed3c
   * Sorcery
   * Target creature gets +2/+2 and gains trample until end of turn.
   * Madness |R */

  // Not an instant, but this is a less cumbersome API than vanilla_pump.  It won't ever see EVENT_CHECK_PUMP events.
  if (IS_GS_EVENT(player, card, event))
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 2,2, KEYWORD_TRAMPLE,0);

  return madness(player, card, event, MANACOST_R(1));
}

/* Falkenrath Reaver	|1|R	=>vanilla	0x401000
 * Creature - Vampire 2/2 */

int card_furyblade_vampire(int player, int card, event_t event)
{
  /* Furyblade Vampire	|1|R	0x200ed6e
   * Creature - Vampire Berserker 1/2
   * Trample
   * At the beginning of combat on your turn, you may discard a card. If you do, ~ gets +3/+0 until end of turn. */

  if (event == EVENT_MUST_ATTACK && current_turn == player && hand_count[player] >= 1 && !is_humiliated(player, card))
	{
	  test_definition_t this_test;
	  default_test_definition(&this_test, 0);
	  int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
	  if (selected != -1)
		{
		  discard_card(player, selected);
		  pump_until_eot_merge_previous(player, card, player, card, 3,0);
		}
	}

  return 0;
}

int card_galvanic_bombardment(int player, int card, event_t event)
{
  /* Galvanic Bombardment	|R	0x200ed73
   * Instant
   * ~ deals X damage to target creature, where X is 2 plus the number of cards named ~ in your graveyard. */

  if (event == EVENT_CHECK_PUMP && can_play_iid(player, event, get_card_instance(player, card)->internal_card_id))
	pumpable_toughness[1-player] -= 2 + count_graveyard_by_id(player, get_id(player, card));

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		damage_target0(player, card, 2 + count_graveyard_by_id(player, get_id(player, card)));

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_harmless_offering(int player, int card, event_t event)
{
  /* Harmless Offering	|2|R	0x200ed78
   * Sorcery
   * Target opponent gains control of target permanent you control. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_PERMANENT);
  td.allowed_controller = td.preferred_controller = player;

  if (event == EVENT_CAN_CAST)
	return opponent_is_valid_target(player, card) && can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (target_opponent(player, card))
		new_pick_target(&td, "TARGET_PERMANENT_YOU_CONTROL", 1, 1);
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (opponent_is_valid_target(player, card) && validate_target(player, card, &td, 1))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  give_control(player, card, inst->targets[1].player, inst->targets[1].card);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/* Impetuous Devils	|2|R|R	0x000000
 * Creature - Devil 6/1
 * Trample, haste
 * When ~ attacks, up to one target creature defending player controls blocks it this combat if able.
 * At the beginning of the end step, sacrifice ~. */

/* Incendiary Flow	|1|R	=>champions_of_kamigawa.c:card_yamabushis_flame	0x2006173
 * Sorcery
 * ~ deals 3 damage to target creature or player. If a creature dealt damage this way would die this turn, exile it instead. */

int card_insatiable_gorgers(int player, int card, event_t event)
{
  /* Insatiable Gorgers	|2|R|R	0x200ed7d
   * Creature - Vampire Berserker 5/3
   * ~ attacks each combat if able.
   * Madness |3|R */

  if (event == EVENT_MUST_ATTACK && current_turn == player && !is_humiliated(player, card))
	attack_if_able(player, card, event);

  return madness(player, card, event, MANACOST_XR(3,1));
}

int card_make_mischief(int player, int card, event_t event)
{
  /* Make Mischief	|2|R	0x200ed82
   * Sorcery
   * ~ deals 1 damage to target creature or player. Put a 1/1 |Sred Devil creature token onto the battlefield. It has "When this creature dies, it deals 1 damage to target creature or player." */

  if (event == EVENT_CHECK_PUMP && can_play_iid(player, event, get_card_instance(player, card)->internal_card_id))
	pumpable_toughness[1-player] -= 1;

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  damage_target0(player, card, 1);
		  generate_token_by_id(player, card, CARD_ID_DEVIL);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

/* Mirrorwing Dragon	|3|R|R	0x000000
 * Creature - Dragon 4/5
 * Flying
 * Whenever a player casts an instant or sorcery spell that targets only ~, that player copies that spell for each other creature he or she controls that the spell could target. Each copy targets a different one of those creatures. */

/* Nahiri's Wrath	|2|R	0x000000
 * Sorcery
 * As an additional cost to cast ~, discard X cards.
 * ~ deals damage equal to the total converted mana cost of the discarded cards to each of up to X target creatures and/or planeswalkers. */

static int fx_otherworldly_outburst(int player, int card, event_t event)
{
  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_POWER && affect_me(inst->damage_target_player, inst->damage_target_card))
	event_result += 1;

  if (graveyard_from_play(inst->damage_target_player, inst->damage_target_card, event))
	{
	  generate_token_by_id(player, card, CARD_ID_ELDRAZI_HORROR);
	  kill_card(player, card, KILL_REMOVE);
	}

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
int card_otherworldly_outburst(int player, int card, event_t event)
{
  /* Otherworldly Outburst	|R	0x200ed87
   * Instant
   * Target creature gets +1/+0 until end of turn. When that creature dies this turn, put a 3/2 colorless Eldrazi Horror creature token onto the battlefield. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;

	  if (valid_target(&td))
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  create_targetted_legacy_effect(player, card, &fx_otherworldly_outburst,
										 inst->targets[0].player, inst->targets[0].card);
		}
	  kill_card(player, card, KILL_DESTROY);
	  return 0;
	}

  return vanilla_instant_pump(player, card, event, ANYBODY, player, 1,0, 0,0);
}

/* Prophetic Ravings	|R	0x000000
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature has haste and "|T, Discard a card: Draw a card." */

/* Savage Alliance	|2|R	0x000000
 * Instant
 * Escalate |1
 * Choose one or more -
 * * Creatures target player controls gain trample until end of turn.
 * * ~ deals 2 damage to target creature.
 * * ~ deals 1 damage to each creature target opponent controls. */

/* Shreds of Sanity	|2|R	0x000000
 * Sorcery
 * Return up to one target instant card and up to one target sorcery card from your graveyard to your hand, then discard a card. Exile ~. */

int card_smoldering_werewolf(int player, int card, event_t event)
{
  /* Smoldering Werewolf	|2|R|R	0x200ed8c
   * Creature - Werewolf Horror 3/2
   * When ~ enters the battlefield, it deals 1 damage to each of up to two target creatures.
   * |4|R|R: Transform ~. */

  double_faced_card(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allow_cancel = 3;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  pick_up_to_n_targets(&td, "TARGET_CREATURE", 2);
	  if (cancel == 1)
		inst->number_of_targets = 0;
	  else
		{
		  int i;
		  for (i = 0; i < inst->number_of_targets; ++i)
			damage_creature(inst->targets[i].player, inst->targets[i].card, 1, player, card);
		}
	}

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card)
		  && !check_special_flags3(inst->parent_controller, inst->parent_card, SF3_CARD_IS_FLIPPED))
		transform(inst->parent_controller, inst->parent_card);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_XR(4,2), 0, NULL, NULL);
}
// --TRANSFORM--
int card_erupting_dreadwolf(int player, int card, event_t event)
{
  /* Erupting Dreadwolf	""	0x200ed91
   * Creature - Eldrazi Werewolf 6/4
   * Whenever ~ attacks, it deals 2 damage to target creature or player. */

  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	  td.allow_cancel = 0;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
		damage_target0(player, card, 2);
	}

  return 0;
}

int card_spreading_flames(int player, int card, event_t event)
{
  /* Spreading Flames	|6|R	0x200ed96
   * Instant
   * ~ deals 6 damage divided as you choose among any number of target creatures. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);

  if (event == EVENT_CAN_CAST)
	return can_target(&td);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);

	  int i;
	  for (i = 0; i < 6; ++i)
		if (new_pick_target(&td, "TARGET_CREATURE", -1, 1))
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

int card_stensia_banquet(int player, int card, event_t event)
{
  /* Stensia Banquet	|2|R	0x200ed9b
   * Sorcery
   * ~ deals damage to target opponent equal to the number of Vampires you control.
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
		  int dmg = count_subtype(player, TYPE_PERMANENT, SUBTYPE_VAMPIRE);
		  if (dmg > 0)
			damage_target0(player, card, dmg);

		  draw_a_card(player);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_OPPONENT", 1, NULL);
}

int card_stensia_innkeeper(int player, int card, event_t event)
{
  /* Stensia Innkeeper	|3|R	0x200eda0
   * Creature - Vampire 3/3
   * When ~ enters the battlefield, tap target land an opponent controls. That land doesn't untap during its controller's next untap step. */

  if (comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_LAND);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_next_target_noload(&td, "Select target land an opponent controls."))
		does_not_untap_effect(player, card, inst->targets[0].player, inst->targets[0].card, EDNT_TAP_TARGET, 1);
	}

  return 0;
}

/* Stromkirk Occultist	|2|R	0x000000
 * Creature - Vampire Horror 3/2
 * Trample
 * Whenever ~ deals combat damage to a player, exile the top card of your library. Until end of turn, you may play that card.
 * Madness |1|R */

int card_thermo_alchemist(int player, int card, event_t event)
{
  /* Thermo-Alchemist	|1|R	0x200eda5
   * Creature - Human Shaman 0/3
   * Defender
   * |T: ~ deals 1 damage to each opponent.
   * Whenever you cast an instant or sorcery spell, untap ~. */

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY,
							TYPE_SPELL,MATCH, 0,0, 0,0, 0,0, -1,0))
	untap_card(player, card);

  if (event == EVENT_RESOLVE_ACTIVATION)
	damage_creature(1-player, -1, 1, player, card);

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_vildin_pack_outcast(int player, int card, event_t event)
{
  /* Vildin-Pack Outcast	|4|R	0x200edaa
   * Creature - Werewolf Horror 4/4
   * Trample
   * |R: ~ gets +1/-1 until end of turn.
   * |5|R|R: Transform ~. */

  double_faced_card(player, card, event);

  if (event == EVENT_POW_BOOST)
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_R(1), 1,-1);

  if (!IS_ACTIVATING(event))
	return 0;

  enum
  {
	CHOICE_PUMP = 1,
	CHOICE_TRANSFORM
  } choice = DIALOG(player, card, event,
					DLG_RANDOM,
					"+1/-1",	1,	1,	DLG_MANA(MANACOST_R(1)),
					"Transform",1,	4,	DLG_MANA(MANACOST_XR(5,2)));

  if (event == EVENT_CAN_ACTIVATE)
	return choice && can_use_activated_abilities(player, card);
  else if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card))
		switch (choice)
		  {
			case CHOICE_PUMP:
			  pump_until_eot_merge_previous(player, card, player, card, 1,-1);
			  break;

			case CHOICE_TRANSFORM:
			  if (!check_special_flags3(inst->parent_controller, inst->parent_card, SF3_CARD_IS_FLIPPED))
				transform(inst->parent_controller, inst->parent_card);
			  break;
		  }
	}

  return 0;
}
// --TRANSFORM--
/* Dronepack Kindred	""	=>legends.c:card_wall_of_opposition	0x200c519
 * Creature - Eldrazi Werewolf 5/7
 * Trample
 * |1: ~ gets +1/+0 until end of turn. */

int card_weaver_of_lightning(int player, int card, event_t event)
{
  /* Weaver of Lightning	|2|R	0x200edaf
   * Creature - Human Shaman 1/4
   * Reach
   * Whenever you cast an instant or sorcery spell, ~ deals 1 damage to target creature an opponent controls. */

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY,
							TYPE_SPELL,MATCH, 0,0, 0,0, 0,0, -1,0))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  get_card_instance(player, card)->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS"))
		damage_target0(player, card, 1);
	}

  return 0;
}

/*** Green ***/

int card_backwoods_survivalists(int player, int card, event_t event)
{
  /* Backwoods Survivalists	|3|G	0x200edb4
   * Creature - Human Warrior 4/3
   * Delirium - ~ gets +1/+1 and has trample as long as there are four or more card types among cards in your graveyard. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card)
	  && delirium(player) && !is_humiliated(player, card))
	event_result += 1;

  if (event == EVENT_ABILITIES && affect_me(player, card) && delirium(player) && !is_humiliated(player, card))
	event_result |= KEYWORD_TRAMPLE;

  return 0;
}

int card_bloodbriar(int player, int card, event_t event)
{
  /* Bloodbriar	|2|G	0x200edb9
   * Creature - Plant Elemental 2/3
   * Whenever you sacrifice another permanent, put a +1/+1 counter on ~. */

  if (event == EVENT_GRAVEYARD_FROM_PLAY
	  && !affect_me(player, card)
	  && get_card_instance(affected_card_controller, affected_card)->kill_code == KILL_SACRIFICE)
	count_for_gfp_ability(player, card, event, player, TYPE_PERMANENT, NULL);

  int n = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
  if (n > 0)
	add_1_1_counters(player, card, n);

  return 0;
}

int card_clear_shot(int player, int card, event_t event)
{
  /* Clear Shot	|2|G	0x200edbe
   * Instant
   * Target creature you control gets +1/+1 until end of turn. It deals damage equal to its power to target creature you don't control. */

  if (event == EVENT_CHECK_PUMP && can_play_iid(player, event, get_card_instance(player, card)->internal_card_id))
	{
	  pumpable_power[player] += 1;
	  pumpable_toughness[player] += 1;
	}

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td_control;
  default_target_definition(player, card, &td_control, TYPE_CREATURE);
  td_control.allowed_controller = td_control.preferred_controller = player;

  target_definition_t td_dont;
  default_target_definition(player, card, &td_dont, TYPE_CREATURE);
  td_dont.allowed_controller = td_dont.preferred_controller = 1-player;

  if (event == EVENT_CAN_CAST)
	return can_target(&td_control) && can_target(&td_dont);

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  inst->number_of_targets = 0;
	  if (new_pick_target(&td_control, "ASHNODS_BATTLEGEAR", 0, 1))
		new_pick_target(&td_dont, "Select target creature you don't control.", 1, 1 | GS_LITERAL_PROMPT);
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (validate_target(player, card, &td_control, 0))
		{
		  pump_until_eot(player, card, inst->targets[0].player, inst->targets[0].card, 1,1);
		  if (validate_target(player, card, &td_dont, 1))
			damage_creature(inst->targets[1].player, inst->targets[1].card,
							get_power(inst->targets[0].player, inst->targets[0].card),
							inst->targets[0].player, inst->targets[0].card);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

/* Crop Sigil	|G	0x000000
 * Enchantment
 * At the beginning of your upkeep, you may put the top card of your library into your graveyard.
 * Delirium - |2|G, Sacrifice ~: Return up to one target creature card and up to one target land card from your graveyard to your hand. Activate this ability only if there are four or more card types among cards in your graveyard. */

int card_crossroads_consecrator(int player, int card, event_t event)
{
  /* Crossroads Consecrator	|G	0x200edc3
   * Creature - Human Cleric 1/2
   * |G, |T: Target attacking Human gets +1/+1 until end of turn. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_subtype = SUBTYPE_HUMAN;
  td.required_state = TARGET_STATE_ATTACKING;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  pump_until_eot_merge_previous(player, card, inst->targets[0].player, inst->targets[0].card, 1,1);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT,
								   MANACOST_G(1), 0, &td, "Select target attacking Human.");
}

/* Eldritch Evolution	|1|G|G	0x000000
 * Sorcery
 * As an additional cost to cast ~, sacrifice a creature.
 * Search your library for a creature card with converted mana cost X or less, where X is 2 plus the sacrificed creature's converted mana cost. Put that card onto the battlefield, then shuffle your library. Exile ~. */

/* Emrakul's Evangel	|2|G	0x000000
 * Creature - Human Horror 3/2
 * |T, Sacrifice ~ and any number of other non-Eldrazi creatures: Put a 3/2 colorless Eldrazi Horror creature token onto the battlefield for each creature sacrificed this way. */

int card_emrakuls_influence(int player, int card, event_t event)
{
  /* Emrakul's Influence	|2|G|G	0x200edc8
   * Enchantment
   * Whenever you cast an Eldrazi creature spell with converted mana cost 7 or greater, draw two cards. */

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE,MATCH,
							SUBTYPE_ELDRAZI,MATCH, 0,0, 0,0, 6,F5_CMC_GREATER_THAN_VALUE))
	draw_cards(player, 2);

  return global_enchantment(player, card, event);
}

/* Foul Emissary	|2|G	0x000000
 * Creature - Human Horror 1/1
 * When ~ enters the battlefield, look at the top four cards of your library. You may reveal a creature card from among them and put it into your hand. Put the rest on the bottom of your library in any order.
 * When you sacrifice ~ while casting a spell with emerge, put a 3/2 colorless Eldrazi Horror creature token onto the battlefield. */

int card_gnarlwood_dryad(int player, int card, event_t event)
{
  /* Gnarlwood Dryad	|G	0x200edcd
   * Creature - Dryad Horror 1/1
   * Deathtouch
   * Delirium - ~ gets +2/+2 as long as there are four or more card types among cards in your graveyard. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card)
	  && delirium(player) && !is_humiliated(player, card))
	event_result += 2;

  return 0;
}

int card_grapple_with_the_past(int player, int card, event_t event)
{
  /* Grapple with the Past	|1|G	0x200edd2
   * Instant
   * Put the top three cards of your library into your graveyard, then you may return a creature or land card from your graveyard to your hand. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  mill(player, 3);

	  if (any_in_graveyard_by_type(player, TYPE_CREATURE | TYPE_LAND))
		{
		  test_definition_t test;
		  new_default_test_definition(&test, TYPE_CREATURE | TYPE_LAND, "Select a creature or land card.");
		  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

/* Hamlet Captain	|1|G	=>innistrad.c:card_hamlet_captain	0x2003f1d
 * Creature - Human Warrior 2/2
 * Whenever ~ attacks or blocks, other Humans you control get +1/+1 until end of turn. */

int card_ishkanah_grafwidow(int player, int card, event_t event)
{
  /* Ishkanah, Grafwidow	|4|G	0x200edd7
   * Legendary Creature - Spider 3/5
   * Reach
   * Delirium - When ~ enters the battlefield, if there are four or more card types among cards in your graveyard, put three 1/2 |Sgreen Spider creature tokens with reach onto the battlefield.
   * |6|B: Target opponent loses 1 life for each Spider you control. */

  check_legend_rule(player, card, event);

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card)
	  && card == trigger_cause && player == trigger_cause_controller
	  && delirium(player)
	  && comes_into_play(player, card, event))
	generate_tokens_by_id(player, card, CARD_ID_SPIDER, 3);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.allowed_controller = 1-player;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  lose_life(inst->targets[0].player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_SPIDER));
	}

  return generic_activated_ability(player, card, event, GAA_CAN_ONLY_TARGET_OPPONENT,
								   MANACOST_XB(6,1), 0, &td, "TARGET_OPPONENT");
}

/* Kessig Prowler	|G	0x000000
 * Creature - Werewolf Horror 2/1
 * |4|G: Transform ~.
 * --TRANSFORM--
 * Sinuous Predator	""	= conspiracy.c:card_charging_rhino (and unimplemented Stalking Tiger, Ironhoof Ox, Norwood Riders), though it works poorly
 * Creature - Eldrazi Werewolf 4/4
 * ~ can't be blocked by more than one creature. */

int card_noose_constrictor(int player, int card, event_t event)
{
  /* Noose Constrictor	|1|G	0x200ee09
   * Creature - Snake 2/2
   * Reach
   * Discard a card: ~ gets +1/+1 until end of turn. */

  if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
	return hand_count[player];

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card))
		pump_until_eot_merge_previous(player, card, inst->parent_controller, inst->parent_card, 1,1);
	}

  return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST0, 0, NULL, NULL);
}

/* Permeating Mass	|G	0x000000
 * Creature - Spirit 1/3
 * Whenever ~ deals combat damage to a creature, that creature becomes a copy of ~. */

/* Prey Upon	|G	=>innistrad.c:card_prey_upon	0x20049db
 * Sorcery
 * Target creature you control fights target creature you don't control. */

/* Primal Druid	|1|G	=>mirrodin_besieged.c:card_viridian_emissary	0x2002ba4
 * Creature - Human Druid 0/3
 * When ~ dies, you may search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library. */

int card_shrill_howler(int player, int card, event_t event)
{
  /* Shrill Howler	|2|G	0x200eddc
   * Creature - Werewolf Horror 3/1
   * Creatures with power less than ~'s power can't block it.
   * |5|G: Transform ~. */

  double_faced_card(player, card, event);

  if (event == EVENT_BLOCK_LEGALITY
	  && card == attacking_card && player == attacking_card_controller
	  && get_power(affected_card_controller, affected_card) < get_power(player, card)
	  && !is_humiliated(player, card))
	event_result = 1;

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card)
		  && !check_special_flags3(inst->parent_controller, inst->parent_card, SF3_CARD_IS_FLIPPED))
		transform(inst->parent_controller, inst->parent_card);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_XG(5,1), 0, NULL, NULL);
}
// --TRANSFORM--
int card_howling_chorus(int player, int card, event_t event)
{
  /* Howling Chorus	""	0x0x200ede1
   * Creature - Eldrazi Werewolf 3/5
   * Creatures with power less than ~'s power can't block it.
   * Whenever ~ deals combat damage to a player, put a 3/2 colorless Eldrazi Horror creature token onto the battlefield. */

  if (event == EVENT_BLOCK_LEGALITY
	  && card == attacking_card && player == attacking_card_controller
	  && get_power(affected_card_controller, affected_card) < get_power(player, card)
	  && !is_humiliated(player, card))
	event_result = 1;

  if (damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_BE_COMBAT_DAMAGE))
	generate_token_by_id(player, card, CARD_ID_ELDRAZI_HORROR);

  return 0;
}

int card_somberwald_stag(int player, int card, event_t event)
{
  /* Somberwald Stag	|3|G|G	0x200ede6
   * Creature - Elk 4/3
   * When ~ enters the battlefield, you may have it fight target creature you don't control. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = td.preferred_controller = 1-player;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_next_target_noload(&td, "Select target creature you don't control."))
		fight(player, card, inst->targets[0].player, inst->targets[0].card);
	}

  return 0;
}

int card_spirit_of_the_hunt(int player, int card, event_t event)
{
  /* Spirit of the Hunt	|1|G|G	0x200edeb
   * Creature - Wolf Spirit 3/3
   * Flash
   * When ~ enters the battlefield, each other creature you control that's a Wolf or a Werewolf gets +0/+3 until end of turn. */

  if (comes_into_play(player, card, event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_WOLF;
	  test.sub2 = SUBTYPE_WEREWOLF;
	  test.subtype_flag = F2_MULTISUBTYPE;

	  pump_creatures_until_eot(player, card, player, 0, 0,3, 0,0, &test);
	}

  return flash(player, card, event);
}

int card_splendid_reclamation(int player, int card, event_t event)
{
  /* Splendid Reclamation	|3|G	0x200edf0
   * Sorcery
   * Return all land cards from your graveyard to the battlefield tapped. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  reanimate_all(player, card, player, TYPE_LAND,MATCH, 0,0, 0,0, 0,0, -1,0, REANIMATE_TAP);
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

/* Springsage Ritual	|3|G	=>m11.c:card_solemn_offering	0x2002767
 * Instant
 * Destroy target artifact or enchantment. You gain 4 life. */

/* Swift Spinner	|3|G	=>time_spiral.c:card_ashcoat_bear	0x200cb63
 * Creature - Spider 2/3
 * Flash
 * Reach */

/* Tangleclaw Werewolf	|2|G|G	0x000000
 * Creature - Werewolf Horror 2/4
 * ~ can block an additional creature each combat.
 * |6|G: Transform ~.
 * --TRANSFORM--
 * Fibrous Entangler	""	0x000000
 * Creature - Eldrazi Werewolf 4/6
 * Vigilance
 * ~ must be blocked if able.
 * ~ can block an additional creature each combat. */

int card_ulvenwald_captive(int player, int card, event_t event)
{
  /* Ulvenwald Captive	|1|G	0x200edf5
   * Creature - Werewolf Horror 1/2
   * Defender
   * |T: Add |G to your mana pool.
   * |5|G|G: Transform ~. */

  double_faced_card(player, card, event);

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_TRANSFORM
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1, DLG_NO_DISPLAY_FOR_AI,
						"Mana",
							mana_producer(player, card, EVENT_CAN_ACTIVATE),
							paying_mana(),
						"Transform",
							!paying_mana() && can_use_activated_abilities(player, card),
							3,
							DLG_MANA(MANACOST_XG(5,2)));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_TRANSFORM)
		{
		  if (event == EVENT_RESOLVE_ACTIVATION)
			{
			  card_instance_t* inst = get_card_instance(player, card);
			  if (in_play(inst->parent_controller, inst->parent_card)
				  && !check_special_flags3(inst->parent_controller, inst->parent_card, SF3_CARD_IS_FLIPPED))
				transform(inst->parent_controller, inst->parent_card);
			}
		  return 0;
		}
	}

  return mana_producer(player, card, event);
}
// --TRANSFORM--
/* Ulvenwald Abomination	""	=>scars_of_mirrodin.c:card_palladium_myr	0x200acf5
 * Creature - Eldrazi Werewolf 4/6
 * |T: Add |C|C to your mana pool. */

int card_ulvenwald_observer(int player, int card, event_t event)
{
  /* Ulvenwald Observer	|4|G|G	0x200edfa
   * Creature - Treefolk 6/6
   * Whenever a creature you control with toughness 4 or greater dies, draw a card. */

  if (event == EVENT_GRAVEYARD_FROM_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.toughness = 3;
	  test.toughness_flag = F5_TOUGHNESS_GREATER_THAN_VALUE;

	  count_for_gfp_ability(player, card, event, player, 0, &test);
	}

  int n = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
  if (n > 0)
	draw_cards(player, n);

  return 0;
}

/* Waxing Moon	|1|G	0x000000
 * Instant
 * Transform up to one target Werewolf you control. Creatures you control gain trample until end of turn. */

int card_wolfkin_bond(int player, int card, event_t event)
{
  /* Wolfkin Bond	|4|G	0x200edff
   * Enchantment - Aura
   * Enchant creature
   * When ~ enters the battlefield, put a 2/2 |Sgreen Wolf creature token onto the battlefield.
   * Enchanted creature gets +2/+2. */

  if (comes_into_play(player, card, event))
	generate_token_by_id(player, card, CARD_ID_WOLF);

  return generic_aura(player, card, event, player, 2,2, 0,0, 0,0,0);
}

int card_woodcutters_grit(int player, int card, event_t event)
{
  /* Woodcutter's Grit	|2|G	0x200ee04
   * Instant
   * Target creature you control gets +3/+3 and gains hexproof until end of turn. */

  return vanilla_instant_pump(player, card, event, ANYBODY, player, 3,3, 0,SP_KEYWORD_HEXPROOF);
}

/* Woodland Patrol	|2|G	=>unlimited.c:card_serra_angel	0x200c122
 * Creature - Human Scout 3/2
 * Vigilance */

/*** Multi ***/

int card_bloodhall_priest(int player, int card, event_t event)
{
  /* Bloodhall Priest	|2|B|R	0x200ee0e
   * Creature - Vampire Cleric 4/4
   * Whenever ~ enters the battlefield or attacks, if you have no cards in hand, ~ deals 2 damage to target creature or player.
   * Madness |1|B|R */

  if ((hand_count[player] == 0 || event == EVENT_CLEANUP)
	  && (comes_into_play(player, card, event)
		  || declare_attackers_trigger(player, card, event, 0, player, card)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	  td.allow_cancel = 0;

	  get_card_instance(player, card)->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER"))
		damage_target0(player, card, 2);
	}

  return madness(player, card, event, MANACOST_XBR(1,1,1));
}

int card_campaign_of_vengeance(int player, int card, event_t event)
{
  /* Campaign of Vengeance	|3|W|B	0x200ee13
   * Enchantment
   * Whenever a creature you control attacks, defending player loses 1 life and you gain 1 life. */

  int attackers = declare_attackers_trigger(player, card, event, 0, player, -1);
  if (attackers)
	for (; attackers > 0; --attackers)	// separate life/gain-life event for each trigger
	  {
		lose_life(1-player, 1);
		gain_life(player, 1);
	  }

  return global_enchantment(player, card, event);
}

/* Gisa and Geralf	|2|U|B	0x000000
 * Legendary Creature - Human Wizard 4/4
 * When ~ enters the battlefield, put the top four cards of your library into your graveyard.
 * During each of your turns, you may cast a Zombie creature card from your graveyard. */

int card_grim_flayer(int player, int card, event_t event)
{
  /* Grim Flayer	|B|G	0x200ee18
   * Creature - Human Warrior 2/2
   * Trample
   * Whenever ~ deals combat damage to a player, look at the top three cards of your library. Put any number of them into your graveyard and the rest back on top of your library in any order.
   * Delirium - ~ gets +2/+2 as long as there are four or more card types among cards in your graveyard. */

  if (damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_BE_COMBAT_DAMAGE))
	reveal_top_cards_of_library_and_choose_type(player, card, player, 3, 0, TUTOR_GRAVE, 0, TUTOR_DECK, 0, TYPE_ANY);

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card)
	  && delirium(player) && !is_humiliated(player, card))
	event_result += 2;

  return 0;
}

int card_herons_grace_champion(int player, int card, event_t event)
{
  /* Heron's Grace Champion	|2|G|W	0x200ee1d
   * Creature - Human Knight 3/3
   * Flash
   * Lifelink
   * When ~ enters the battlefield, other Humans you control get +1/+1 and gain lifelink until end of turn. */

  lifelink(player, card, event);

  if (comes_into_play(player, card, event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_HUMAN;
	  test.not_me = 1;

	  pump_creatures_until_eot(player, card, player, 0, 1,1, 0,SP_KEYWORD_LIFELINK, &test);
	}

  return flash(player, card, event);
}

/* Mercurial Geists	|2|U|R	=>rise_of_the_eldrazi.c:card_kiln_fiend	0x2004b0c
 * Creature - Spirit 1/3
 * Flying
 * Whenever you cast an instant or sorcery spell, ~ gets +3/+0 until end of turn. */

static int fx_creatures_power_le_2_cant_block(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES && get_power(affected_card_controller, affected_card) < 2)
	cannot_block(affected_card_controller, affected_card, event);

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
int card_mournwillow(int player, int card, event_t event)
{
  /* Mournwillow	|1|B|G	0x200ee22
   * Creature - Plant Skeleton 3/2
   * Haste
   * Delirium - When ~ enters the battlefield, if there are four or more card types among cards in your graveyard, creatures with power 2 or less can't block this turn. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card)
	  && card == trigger_cause && player == trigger_cause_controller
	  && delirium(player)
	  && comes_into_play(player, card, event))
	create_legacy_effect(player, card, &fx_creatures_power_le_2_cant_block);

  return 0;
}

/* Ride Down	|R|W	=>khans_of_tarkir.c:card_ride_down	0x200d180
 * Instant
 * Destroy target blocking creature. Creatures that were blocked by that creature this combat gain trample until end of turn. */

/* Spell Queller	|1|W|U	0x000000
 * Creature - Spirit 2/3
 * Flash
 * Flying
 * When ~ enters the battlefield, exile target spell with converted mana cost 4 or less.
 * When ~ leaves the battlefield, the exiled card's owner may cast that card without paying its mana cost. */

/* Tamiyo, Field Researcher	|1|G|W|U	0x000000
 * Planeswalker - Tamiyo (4)
 * +1: Choose up to two target creatures. Until your next turn, whenever either of those creatures deals combat damage, you draw a card.
 * -2: Tap up to two target nonland permanents. They don't untap during their controller's next untap step.
 * -7: Draw three cards. You get an emblem with "You may cast nonland cards from your hand without paying their mana costs." */

/* Tamiyo, Field Researcher's Emblem	""	0x000000
 * Emblem - Tamiyo
 * You may cast nonland cards from your hand without paying their mana costs. */

int card_ulrich_of_the_krallenhorde(int player, int card, event_t event)
{
  /* Ulrich of the Krallenhorde	|3|R|G	0x200ee27
   * Legendary Creature - Human Werewolf 4/4
   * Whenever this creature enters the battlefield or transforms into ~, target creature gets +4/+4 until end of turn.
   * At the beginning of each upkeep, if no spells were cast last turn, transform ~. */

  double_faced_card(player, card, event);
  human_moon_phases(player, card, event);
  check_legend_rule(player, card, event);

  if (event == EVENT_TRANSFORMED
	  || comes_into_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.preferred_controller = player;
	  td.allow_cancel = 0;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_CREATURE"))
		pump_until_eot_merge_previous(player, card, inst->targets[0].player, inst->targets[0].card, 4,4);
	}

  return 0;
}
// --TRANSFORM--
int card_ulrich_uncontested_alpha(int player, int card, event_t event)
{
  /* Ulrich, Uncontested Alpha	""(RG)	0x200ee2c
   * Legendary Creature - Werewolf 6/6
   * Whenever this creature transforms into ~, you may have it fight target non-Werewolf creature you don't control.
   * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

  check_legend_rule(player, card, event);
  werewolf_moon_phases(player, card, event);

  if (event == EVENT_TRANSFORMED)
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.required_subtype = SUBTYPE_WEREWOLF;
	  td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	  card_instance_t* inst = get_card_instance(player, card);
	  inst->number_of_targets = 0;
	  if (can_target(&td) && pick_next_target_noload(&td, "Select target non-Werewolf creature you don't control."))
		fight(player, card, inst->targets[0].player, inst->targets[0].card);
	}

  return 0;
}

/*** Artifact ***/

/* Cathar's Shield	|0	=>scars_of_mirrodin.c:card_accorder_shield	0x200416b
 * Artifact - Equipment
 * Equipped creature gets +0/+3 and has vigilance.
 * Equip |3 */

int card_cryptolith_fragment(int player, int card, event_t event)
{
  /* Cryptolith Fragment	|3	0x200ee31
   * Artifact
   * ~ enters the battlefield tapped.
   * |T: Add one mana of any color to your mana pool. Each player loses 1 life.
   * At the beginning of your upkeep, if each player has 10 or less life, transform ~. */

  double_faced_card(player, card, event);
  comes_into_play_tapped(player, card, event);

  if ((event == EVENT_UPKEEP_TRIGGER_ABILITY || trigger_condition == TRIGGER_UPKEEP)
	  && life[0] <= 10 && life[1] <= 10)
	{
	  upkeep_trigger_ability(player, card, event, player);

	  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
		transform(player, card);
	}

  int rval = mana_producer(player, card, event);
  if (event == EVENT_ACTIVATE && cancel != 1)
	APNAP(p, lose_life(p, 1));
  return rval;
}
// --TRANSFORM--
int card_aurora_of_emrakul(int player, int card, event_t event)
{
  /* Aurora of Emrakul	""	0x200ee36
   * Creature - Eldrazi Reflection 1/4
   * Flying, deathtouch
   * Whenever ~ attacks, each opponent loses 3 life. */

  deathtouch(player, card, event);

  if (declare_attackers_trigger(player, card, event, 0, player, card))
	lose_life(1-current_turn, 3);

  return 0;
}

int card_cultists_staff(int player, int card, event_t event)
{
  /* Cultist's Staff	|2	200ee3b
   * Artifact - Equipment
   * Equipped creature gets +2/+2.
   * Equip |3 */

  return vanilla_equipment(player, card, event, 3, 2,2, 0,0);
}

/* Field Creeper	|2	=>vanilla	0x401000
 * Artifact Creature - Scarecrow 2/1 */

int card_geist_fueled_scarecrow(int player, int card, event_t event)
{
  /* Geist-Fueled Scarecrow	|4	0x200ee40
   * Artifact Creature - Scarecrow 4/4
   * Creature spells you cast cost |1 more to cast. */

  if (event == EVENT_MODIFY_COST_GLOBAL)
	{
	  if ((affected_card_controller == -1 ? event_result : affected_card_controller) == player
		  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		  && in_play(player, card) && !is_humiliated(player, card))
		COST_COLORLESS += 1;

	  return 0;	// explicit return at end of EVENT_MODIFY_COST_GLOBAL
	}

  return 0;
}

int card_lupine_prototype(int player, int card, event_t event)
{
  /* Lupine Prototype	|2	0x200ee45
   * Artifact Creature - Wolf Construct 5/5
   * ~ can't attack or block unless a player has no cards in hand. */

  if (event == EVENT_ATTACK_LEGALITY && affect_me(player, card)
	  && (hand_count[0] == 0 || hand_count[1] == 0)
	  && !is_humiliated(player, card))
	event_result = 1;

  if (event == EVENT_ABILITIES && affect_me(player, card)
	  && (hand_count[0] == 0 || hand_count[1] == 0)
	  && !is_humiliated(player, card) && in_play(player, card))
	cannot_block(player, card, event);

  return 0;
}

/* Slayer's Cleaver	|3	0x000000
 * Artifact - Equipment
 * Equipped creature gets +3/+1 and must be blocked by an Eldrazi if able.
 * Equip |4 */

/* Soul Separator	|3	0x000000
 * Artifact
 * |5, |T, Sacrifice ~: Exile target creature card from your graveyard. Put a token onto the battlefield that's a copy of that card except it's 1/1, it's a Spirit in addition to its other types, and it has flying. Put a |Sblack Zombie creature token onto the battlefield with power equal to that card's power and toughness equal to that card's toughness. */

int card_stitchers_graft(int player, int card, event_t event)
{
  /* Stitcher's Graft	|1	0x200ee4a
   * Artifact - Equipment
   * Equipped creature gets +3/+3.
   * Whenever equipped creature attacks, it doesn't untap during its controller's next untap step.
   * Whenever ~ becomes unattached from a permanent, sacrifice that permanent.
   * Equip |2 */

  card_instance_t* inst = get_card_instance(player, card);
  if (declare_attackers_trigger(player, card, event, 0, inst->targets[8].player, inst->targets[8].card))
	does_not_untap_effect(player, card, inst->targets[8].player, inst->targets[8].card, 0, 1);

  // graft effect hard-coded in unattach()

  return vanilla_equipment(player, card, event, 2, 3,3, 0,0);
}

/* Terrarion	|1	(unimplemented reprint from Ravnica: City of Guilds)
 * Artifact
 * ~ enters the battlefield tapped.
 * |2, |T, Sacrifice ~: Add two mana in any combination of colors to your mana pool.
 * When ~ is put into a graveyard from the battlefield, draw a card. */

/* Thirsting Axe	|3	0x000000
 * Artifact - Equipment
 * Equipped creature gets +4/+0.
 * At the beginning of your end step, if equipped creature didn't deal combat damage to a creature this turn, sacrifice it.
 * Equip |2 */

/*** Land ***/

int card_geier_reach_sanitarium(int player, int card, event_t event)
{
  /* Geier Reach Sanitarium	""	0x200ee4f
   * Legendary Land
   * |T: Add |C to your mana pool.
   * |2, |T: Each player draws a card, then discards a card. */

  check_legend_rule(player, card, event);

  if (IS_ACTIVATING(event))
	{
	  enum
	  {
		CHOICE_MANA = 1,
		CHOICE_LOOT
	  } choice = DIALOG(player, card, event,
						DLG_AUTOCHOOSE_IF_1, DLG_NO_DISPLAY_FOR_AI,
						"Mana",
							mana_producer(player, card, EVENT_CAN_ACTIVATE),
							paying_mana(),
						"Draw and discard",
							/* This calls generic_activated_ability() and charges mana manually instead of using a
							 * DLG_MANA() clause because both this and its mana ability have T in their costs */
							!paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE,
																		GAA_UNTAPPED, MANACOST_X(3), 0, NULL, NULL),
							hand_count[AI] - 2*hand_count[HUMAN] + landsofcolor_controlled[AI][COLOR_ANY]);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (choice == 0)	// cancelled
		return 0;
	  else if (choice == CHOICE_LOOT)
		{
		  if (event == EVENT_ACTIVATE)
			{
			  add_state(player, card, STATE_TAPPED);
			  if (!charge_mana_for_activated_ability(player, card, MANACOST_X(2)))
				remove_state(player, card, STATE_TAPPED);
			}
		  else if (event == EVENT_RESOLVE_ACTIVATION)
			APNAP(p,
				  draw_a_card(p);
				  discard(p, 0, player));

		  return 0;
		}
	  // else fall through to mana_producer below
	}

  return mana_producer(player, card, event);
}

/* Hanweir Battlements	""	0x000000
 * Land
 * |T: Add |C to your mana pool.
 * |R, |T: Target creature gains haste until end of turn.
 * |3|R|R, |T: If you both own and control ~ and a creature named Hanweir Garrison, exile them, then meld them into Hanweir, the Writhing Township.
 * --MELD WITH--
 * Hanweir Garrison	|2|R	0x000000
 * Creature - Human Soldier 2/3
 * Whenever ~ attacks, put two 1/1 |Sred Human creature tokens onto the battlefield tapped and attacking.
 * --MELD TO--
 * Hanweir, the Writhing Township	""	0x000000
 * Legendary Creature - Eldrazi Ooze 7/4
 * Trample, haste
 * Whenever ~ attacks, put two 3/2 colorless Eldrazi Horror creature tokens onto the battlefield tapped and attacking. */

/* Nephalia Academy	""	0x000000
 * Land
 * If a spell or ability an opponent controls causes you to discard a card, you may reveal that card and put it on top of your library instead of putting it anywhere else.
 * |T: Add |C to your mana pool. */

/*** Tokens ***/

/* Eldrazi Horror	""	=>vanilla	0x401000
 * Creature - Eldrazi Horror 3/2 */

/* Human Wizard	""	=>vanilla	0x401000
 * Creature - Human Wizard 1/1 */
