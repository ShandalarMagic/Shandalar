#include "manalink.h"

// Functions
int hom_clockwork(int player, int card, event_t event, int amount)
{
  // 0x426620
  card_instance_t* instance = get_card_instance(player, card);

  // ~ enters the battlefield with [amount] +1/+0 counters on it.
  enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P0, amount);

  // At end of combat, if ~ attacked or blocked this combat, remove a +1/+0 counter from it.
  if ((event == EVENT_DECLARE_ATTACKERS && is_attacking(player, card)) || blocking(player, card, event))
	instance->targets[1].card = 66;

  if (instance->targets[1].card == 66 && end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  remove_counter(player, card, COUNTER_P1_P0);
	  instance->targets[1].card = 0;
	}

  /* |X, |T: Put up to X +1/+0 counters on ~. This ability can't cause the total number of +1/+0 counters on ~ to be greater than four. Activate this ability
   * only during your upkeep. */
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  int pp = instance->parent_controller;
	  int pc = instance->parent_card;
	  if (in_play(pp, pc))
		{
		  if (count_counters(pp, pc, COUNTER_P1_P0) >= amount)
			ai_modifier -= 256;
		  else
			{
			  add_counters(pp, pc, COUNTER_P1_P0, instance->info_slot);
			  int c = count_counters(pp, pc, COUNTER_P1_P0);
			  if (c > amount)
				remove_counters(pp, pc, COUNTER_P1_P0, c - amount);
			  else
				ai_modifier += 128;
			}
		}
	}

  if (event == EVENT_GET_SELECTED_CARD)
	{
	  int can_buy = amount - count_counters(player, card, COUNTER_P1_P0);
	  int doubling_effect = can_buy > 1 ? get_updated_counters_number(player, card, COUNTER_P1_P0, 1) : 1;
	  can_buy = MAX(0, can_buy);
	  if (doubling_effect > 1)
		can_buy = (can_buy + doubling_effect - 1) / doubling_effect;

	  int mana_avail = has_mana(player, COLOR_ANY, 1);
	  can_buy = MIN(can_buy, mana_avail);
	  displayed_x_value = can_buy;
	}

  int amt_to_charge = event == EVENT_ACTIVATE && IS_AI(player) ? displayed_x_value : -1;

  int rval = generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_ONLY_ON_UPKEEP|GAA_IN_YOUR_TURN, MANACOST_X(amt_to_charge), 0, NULL, NULL);

  if (event == EVENT_ACTIVATE && amt_to_charge != -1)
	instance->info_slot = amt_to_charge;

  if (rval && event == EVENT_CAN_ACTIVATE && IS_AI(player) && count_counters(player, card, COUNTER_P1_P0) <= amount / 2)
	EXE_DWORD(0x736808) |= 3;

  return rval;
}

static int homelands_triple_land(int player, int card, event_t event, color_test_t primary, color_test_t secondary)
{
  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  inst->info_slot = COLOR_TEST_COLORLESS;
	  play_land_sound_effect_force_color(player, card, COLOR_TEST_COLORLESS | primary | secondary);
	  count_mana();	// So it immediately goes back to proper colors when other mana sources are available
	}

  if (event == EVENT_CHANGE_TYPE)
	{
	  int avail = has_mana(player, COLOR_COLORLESS, 1);
	  inst->info_slot = COLOR_TEST_COLORLESS | (avail < 2 ? 0 : (primary | (avail < 3 ? 0 : secondary)));
	}

  if (event == EVENT_CAN_ACTIVATE
	  || (event == EVENT_COUNT_MANA && affect_me(player, card)))
	return inst->targets[1].player != 66 && mana_producer(player, card, event);

  if (event == EVENT_ACTIVATE)
	{
	  if (inst->targets[1].player == 66)
		return 0;

	  int avail = has_mana(player, COLOR_COLORLESS, 1);
	  int choice = COLOR_COLORLESS;
	  if (avail >= 2)
		choice = choose_a_color_exe(player, "What kind of mana?", 1, 0,
									COLOR_TEST_COLORLESS | primary | (avail < 3 ? 0 : secondary));

	  if (choice == -1)
		{
		  cancel = 1;
		  return 0;
		}

	  if (choice != COLOR_COLORLESS)
		{
		  inst->targets[1].player = 66;
		  charge_mana(player, COLOR_COLORLESS, (primary & (1<<choice)) ? 1 : 2);
		  inst->targets[1].player = 0;
		  if (cancel == 1)
			return 0;
		}

	  produce_mana_tapped(player, card, choice, 1);
	}

  return 0;
}

// Cards

/* Abbey Gargoyles	|2|W|W|W	=>invasion.c:card_galinas_knight
 * Creature - Gargoyle 3/4
 * Flying, protection from |Sred */

int card_abbey_matron(int player, int card, event_t event)
{
  /* Abbey Matron	|2|W	0x200d699
   * Creature - Human Cleric 1/3
   * |W, |T: ~ gets +0/+3 until end of turn. */
  return generic_shade_tap(player, card, event, MANACOST_W(1), 0,3);
}

/* AEther Storm	|3|U	0x000000
 * Enchantment
 * Creature spells can't be cast.
 * Pay 4 life: Destroy ~. It can't be regenerated. Any player may activate this ability. */

int card_alibans_tower(int player, int card, event_t event)
{
  /* Aliban's Tower	|1|R	0x200a44e
   * Instant
   * Target blocking creature gets +3/+1 until end of turn. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;
  td.required_state = TARGET_STATE_BLOCKING;

  if (event == EVENT_CHECK_PUMP && has_mana_to_cast_iid(player, EVENT_CAN_CAST, get_card_instance(player, card)->internal_card_id) && current_turn != player)
	{
	  pumpable_power[player] += 3;
	  pumpable_toughness[player] += 1;
	}

  return vanilla_pump(player, card, event, &td, 3, 1, 0, 0);
}

int card_ambush(int player, int card, event_t event)
{
  /* Ambush	|3|R	0x200d671
   * Instant
   * Blocking creatures gain first strike until end of turn. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.state = STATE_BLOCKING;
	  pump_creatures_until_eot(player, card, ANYBODY, 0, 0,0, KEYWORD_FIRST_STRIKE, 0, &test);
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_ambush_party(int player, int card, event_t event)
{
  /* Ambush Party	|4|R	0x200bb46
   * Creature - Human Rogue 3/1
   * First strike, haste */

  // The first vanilla card with haste, so it gets everything *else* that's vanilla-with-haste pointed at it too.

  haste(player, card, event);
  return 0;
}

int card_an_havva_constable(int player, int card, event_t event)
{
  /* An-Havva Constable	|1|G|G	0x200d676
   * Creature - Human 2/101
   * ~'s toughness is equal to 1 plus the number of |Sgreen creatures on the battlefield. */
  if (event == EVENT_TOUGHNESS && affect_me(player, card) && !is_humiliated(player, card))
	event_result += count_permanents_by_color(ANYBODY, TYPE_CREATURE, get_sleighted_color_test(player, card, COLOR_TEST_GREEN));
  return 0;
}

int card_an_havva_inn(int player, int card, event_t event)
{
  /* An-Havva Inn	|1|G|G	0x200d67b
   * Sorcery
   * You gain X plus 1 life, where X is the number of |Sgreen creatures on the battlefield. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  gain_life(player, 1 + count_permanents_by_color(ANYBODY, TYPE_CREATURE, get_sleighted_color_test(player, card, COLOR_TEST_GREEN)));
	  kill_card(player, card, KILL_DESTROY);
	}
  return basic_spell(player, card, event);
}

int card_an_havva_township(int player, int card, event_t event)
{
  /* An-Havva Township	""	0x200d680
   * Land
   * |T: Add |1 to your mana pool.
   * |1, |T: Add |G to your mana pool.
   * |2, |T: Add |R or |W to your mana pool. */
  return homelands_triple_land(player, card, event, COLOR_TEST_GREEN, COLOR_TEST_RED|COLOR_TEST_WHITE);
}

int card_an_zerrin_ruins(int player, int card, event_t event){
	/* An-Zerrin Ruins	|2|R|R	0x200a3db
	 * Enchantment
	 * As ~ enters the battlefield, choose a creature type.
	 * Creatures of the chosen type don't untap during their controllers' untap steps. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = select_a_subtype(player, card);
	}

	if( current_phase == PHASE_UNTAP && event == EVENT_UNTAP && ! is_humiliated(player, card) ){
		if( instance->info_slot > 0 && has_subtype(affected_card_controller, affected_card, instance->info_slot) ){
			card_instance_t *trg= get_card_instance(affected_card_controller, affected_card);
			trg->untap_status &= ~3;
		}
	}

	return global_enchantment(player, card, event);
}

int card_anaba_ancestor(int player, int card, event_t event){
	/* Anaba Ancestor	|1|R	0x200a3e0
	 * Creature - Minotaur Spirit 1/1
	 * |T: Another target Minotaur creature gets +1/+1 until end of turn. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_MINOTAUR;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_NOT_ME_AS_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									 &td, "Select another target Minotaur creature.");
}

/* Anaba Bodyguard	|3|R	=>vanilla
 * Creature - Minotaur 2/3
 * First strike */

int card_anaba_shaman(int player, int card, event_t event){
	/* Anaba Shaman	|3|R	0x200a3e5
	 * Creature - Minotaur Shaman 2/2
	 * |R, |T: ~ deals 1 damage to target creature or player. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(1), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_anaba_spirit_crafter(int player, int card, event_t event){
	/* Anaba Spirit Crafter	|2|R|R	0x2004bbb
	 * Creature - Minotaur Shaman 1/3
	 * Minotaur creatures get +1/+0. */

	// original code : 0x1201D44
	return boost_creature_type(player, card, event, SUBTYPE_MINOTAUR, 1, 0, 0, BCT_INCLUDE_SELF);
}

int card_apocalypse_chime(int player, int card, event_t event)
{
  /* Apocalypse Chime	|2	0x200d69e
   * Artifact
   * |2, |T, Sacrifice ~: Destroy all nontoken permanents originally printed in the Homelands expansion. They can't be regenerated. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  char bury[2][151] = {{0}};
	  int p, c, csvid;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && is_what(p, c, TYPE_PERMANENT) && !is_token(p, c) && (csvid = get_id(p, c))
			  // They're irritatingly scattered.
			  && ((csvid >= CARD_ID_ABBEY_GARGOYLES && csvid <= CARD_ID_WILLOW_PRIESTESS)
				  || (csvid >= CARD_ID_ABBEY_MATRON && csvid <= CARD_ID_WIZARDS_SCHOOL)
				  || csvid == CARD_ID_ANABA_SHAMAN
				  || csvid == CARD_ID_IHSANS_SHADE
				  || csvid == CARD_ID_MEMORY_LAPSE
				  || csvid == CARD_ID_MERCHANT_SCROLL))
			bury[p][c] = 1;
	  APNAP(pl,
			for (c = 0; c < active_cards_count[pl]; ++c)
			  if (bury[pl][c] && in_play(pl, c))
				kill_card(pl, c, KILL_BURY));
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL);
}

int card_autumn_willow(int player, int card, event_t event){
	/* Autumn Willow	|4|G|G	0x200acd2
	 * Legendary Creature - Avatar 4/4
	 * Shroud
	 * |G: Until end of turn, ~ can be the target of spells and abilities controlled by target player as though it didn't have shroud. */

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *parent = get_card_instance( player, instance->parent_card );
			if( parent->targets[1].player < 0 ){
				parent->targets[1].player = 0;
			}
			parent->targets[1].player |= (1<<instance->targets[0].player);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_G(1), 0, &td, "TARGET_PLAYER");
}

int card_aysen_abbey(int player, int card, event_t event)
{
  /* Aysen Abbey	""	0x200d685
   * Land
   * |T: Add |1 to your mana pool.
   * |1, |T: Add |W to your mana pool.
   * |2, |T: Add |G or |U to your mana pool. */
  return homelands_triple_land(player, card, event, COLOR_TEST_WHITE, COLOR_TEST_GREEN|COLOR_TEST_BLUE);
}

int card_aysen_bureaucrats(int player, int card, event_t event){
	/* Aysen Bureaucrats	|1|W	0x200a3ea
	 * Creature - Human Advisor 1/1
	 * |T: Tap target creature with power 2 or less. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									 &td, "Select target creature with power 2 or less.");
}

int card_aysen_crusader(int player, int card, event_t event){
	/* Aysen Crusader	|2|W|W	0x200a3ef
	 * Creature - Human Knight 102/102
	 * ~'s power and toughness are each equal to 2 plus the number of Soldiers and Warriors you control. */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		int count = 0;
		int result = 0;
		while( count < active_cards_count[player] ){
				if( in_play(player, count) && (has_subtype(player, count, SUBTYPE_SOLDIER) || has_subtype(player, count, SUBTYPE_WARRIOR)) ){
					result++;
				}
				count++;
		}
		event_result+=result;
	}
	return 0;
}

int card_aysen_highway(int player, int card, event_t event)
{
  /* Aysen Highway	|3|W|W|W	0x200d6a3
   * Enchantment
   * |SWhite creatures have |H2plainswalk. */

  if (event == EVENT_ABILITIES && in_play(player, card) && in_play(affected_card_controller, affected_card) && !is_humiliated(player, card)
	  && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && (get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_WHITE)))
	event_result |= get_hacked_walk(player, card, KEYWORD_PLAINSWALK);

  return global_enchantment(player, card, event);
}

int card_bakis_curse(int player, int card, event_t event)
{
  /* Baki's Curse	|2|U|U	0x200d6a8
   * Sorcery
   * ~ deals 2 damage to each creature for each Aura attached to that creature. */
  if (event == EVENT_RESOLVE_SPELL)
	{
	  int p, c, dmg[2][151] = {{0}};
	  card_instance_t* aura;
	  /* Count auras attached to each creature, all in one pass.  In particular, don't call count_auras_enchanting_me() repeatedly, since it also loops over all
	   * cards. */
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if ((aura = in_play(p, c)) && is_what(p, c, TYPE_ENCHANTMENT) && has_subtype(p, c, SUBTYPE_AURA)
			  && aura->damage_target_card >= 0 && in_play(aura->damage_target_player, aura->damage_target_card)
			  && is_what(aura->damage_target_player, aura->damage_target_card, TYPE_CREATURE))
			dmg[aura->damage_target_player][aura->damage_target_card] += 2;

	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (dmg[p][c] >= 0 && in_play(p, c) && is_what(p, c, TYPE_CREATURE))
			damage_creature(p, c, dmg[p][c], player, card);

	  kill_card(player, card, KILL_DESTROY);
	}
  return basic_spell(player, card, event);
}

int card_baron_sengir(int player, int card, event_t event){
	/* Baron Sengir	|5|B|B|B	0x2002D20
	 * Legendary Creature - Vampire 5/5
	 * Flying
	 * Whenever a creature dealt damage by ~ this turn dies, put a +2/+2 counter on ~.
	 * |T: Regenerate another target Vampire. */

	check_legend_rule(player, card, event);

	if( sengir_vampire_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_counters(player, card, COUNTER_P2_P2, get_card_instance(player, card)->targets[11].card);
		get_card_instance(player, card)->targets[11].card = 0;
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;
	td.required_subtype = SUBTYPE_VAMPIRE;
	td.special = TARGET_SPECIAL_NOT_ME | (player == AI ? TARGET_SPECIAL_REGENERATION : 0);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_REGENERATION | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target Vampire to regenerate.");
}

int card_beast_walkers(int player, int card, event_t event)
{
  /* Beast Walkers	|1|W|W	0x200d6ad
   * Creature - Human Beast Soldier 2/2
   * |G: ~ gains banding until end of turn. */
  return generic_shade(player, card, event, 0, MANACOST_G(1), 0, 0, KEYWORD_BANDING, 0);
}

int card_black_carriage(int player, int card, event_t event)
{
  /* Black Carriage	|3|B|B	0x200a3f4
   * Creature - Horse 4/4
   * Trample
   * ~ doesn't untap during your untap step.
   * Sacrifice a creature: Untap ~. Activate this ability only during your upkeep. */

  does_not_untap(player, card, event);

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (in_play(inst->parent_controller, inst->parent_card))
		untap_card(inst->parent_controller, inst->parent_card);
	}

  int rval = generic_activated_ability(player, card, event,
									   GAA_IN_YOUR_TURN|GAA_ONLY_ON_UPKEEP|GAA_SACRIFICE_CREATURE|(IS_AI(player) ? GAA_NOT_ME_AS_TARGET : 0),
									   MANACOST0, 0, NULL, NULL);

  if (event == EVENT_CAN_ACTIVATE && rval && player == AI && IS_AI(player) && creature_count[player] >= 2 && is_tapped(player, card))
	EXE_DWORD(0x736808) |= 3;	// Force AI to activate. (Same as allowing it to.)

  return rval;
}

int card_broken_visage(int player, int card, event_t event){
	/* Broken Visage	|4|B	0x200a3f9
	 * Instant
	 * Destroy target nonartifact attacking creature. It can't be regenerated. Put a |Sblack Spirit creature token with that creature's power and toughness onto the battlefield. Sacrifice the token at the beginning of the next end step. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_state = TARGET_STATE_ATTACKING;
	td.illegal_type = TYPE_ARTIFACT;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_SPIRIT, &token);
			token.pow = get_power(instance->targets[0].player, instance->targets[0].card);
			token.tou = get_toughness(instance->targets[0].player, instance->targets[0].card);
			token.color_forced = COLOR_TEST_BLACK;
			token.special_infos = 67;
			generate_token(&token);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonartifact attacking creature.", 1, NULL);
}

int base_carapace(int player, int card, event_t event, int pow, int tgh)
{
  if (IS_GAA_EVENT(event))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int p = in_play(player, card) ? instance->damage_target_player : -1;
	  int c = instance->damage_target_card;
	  if (event == EVENT_RESOLVE_ACTIVATION && p >= 0 && can_be_regenerated(p, c))
		regenerate_target(p, c);

	  if (event == EVENT_CAN_ACTIVATE
		  && (p < 0
			  || !(land_can_be_played & LCBP_REGENERATION)
			  || !can_be_regenerated(p, c)))
		return 0;

	  // Not GAA_REGENERATION, that checks if {player,card} can regenerate *itself*
	  return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL) ? 99 : 0;
	}

  return generic_aura(player, card, event, player, pow,tgh, 0,0, 0, 0, 0);
}

int card_carapace(int player, int card, event_t event)
{
  /* Carapace	|G	0x200d6b2
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +0/+2.
   * Sacrifice ~: Regenerate enchanted creature. */
  return base_carapace(player, card, event, 0,2);
}

int card_castle_sengir(int player, int card, event_t event)
{
  /* Castle Sengir	""	0x200d68a
   * Land
   * |T: Add |1 to your mana pool.
   * |1, |T: Add |B to your mana pool.
   * |2, |T: Add |U or |R to your mana pool. */
  return homelands_triple_land(player, card, event, COLOR_TEST_BLACK, COLOR_TEST_BLUE|COLOR_TEST_RED);
}

/* Cemetery Gate	|2|B	=>vanilla
 * Creature - Wall 0/5
 * Defender
 * Protection from |Sblack */

/* Chain Stasis	|U	0x000000
 * Instant
 * You may tap or untap target creature. Then that creature's controller may pay |2|U. If the player does, he or she may copy this spell and may choose a new target for that copy. */

int card_chandler(int player, int card, event_t event){
	/* Chandler	|4|R	0x200a3fe
	 * Legendary Creature - Human Rogue 3/3
	 * |R|R|R, |T: Destroy target artifact creature. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.special = TARGET_SPECIAL_ARTIFACT_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_LITERAL_PROMPT | GAA_CAN_TARGET, MANACOST_R(3), 0,
									&td, "Select target artifact creature.");
}

int card_clockwork_gnomes(int player, int card, event_t event)
{
  /* Clockwork Gnomes	|4	0x200d6b7
   * Artifact Creature - Gnome 2/2
   * |3, |T: Regenerate target artifact creature. */

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;
  td.required_state = TARGET_STATE_DESTROYED;
  td.special = TARGET_SPECIAL_ARTIFACT_CREATURE;
  if (player == AI)
	td.special |= TARGET_SPECIAL_REGENERATION;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (can_be_regenerated(inst->targets[0].player, inst->targets[0].card))
		regenerate_target(inst->targets[0].player, inst->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(3), 0, &td, "TARGET_ARTIFACTCREATURE") ? 99 : 0;
}

int card_clockwork_steed(int player, int card, event_t event){
	/* Clockwork Steed	|4	0x200a403
	 * Artifact Creature - Horse 0/3
	 * ~ enters the battlefield with four +1/+0 counters on it.
	 * ~ can't be blocked by artifact creatures.
	 * At end of combat, if ~ attacked or blocked this combat, remove a +1/+0 counter from it.
	 * |X, |T: Put up to X +1/+0 counters on ~. This ability can't cause the total number of +1/+0 counters on ~ to be greater than four. Activate this ability only during your upkeep. */

	if(event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) ){
				event_result = 1;
			}
		}
	}
	return hom_clockwork(player, card, event, 4);
}

int card_clockwork_swarm(int player, int card, event_t event){
	/* Clockwork Swarm	|4	0x200a408
	 * Artifact Creature - Insect 0/3
	 * ~ enters the battlefield with four +1/+0 counters on it.
	 * ~ can't be blocked by Walls.
	 * At end of combat, if ~ attacked or blocked this combat, remove a +1/+0 counter from it.
	 * |X, |T: Put up to X +1/+0 counters on ~. This ability can't cause the total number of +1/+0 counters on ~ to be greater than four. Activate this ability only during your upkeep. */

	if(event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( has_subtype(affected_card_controller, affected_card, SUBTYPE_WALL) ){
				event_result = 1;
			}
		}
	}
	return hom_clockwork(player, card, event, 4);
}

int card_coral_reef(int player, int card, event_t event)
{
  /* Coral Reef	|U|U	0x200d6bc
   * Enchantment
   * ~ enters the battlefield with four polyp counters on it.
   * Sacrifice |Han Island: Put two polyp counters on ~.
   * |U, Tap an untapped |Sblue creature you control, Remove a polyp counter from ~: Put a +0/+1 counter on target creature. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_POLYP, 4);

  if (IS_ACTIVATING(event))
	{
	  if (event == EVENT_CAN_ACTIVATE && !can_use_activated_abilities(player, card))
		return 0;

	  target_definition_t td_blue;
	  base_target_definition(player, card, &td_blue, TYPE_CREATURE);
	  td_blue.allowed_controller = player;
	  td_blue.preferred_controller = player;
	  td_blue.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);
	  td_blue.illegal_state = TARGET_STATE_TAPPED;

	  target_definition_t td_creature;
	  default_target_definition(player, card, &td_creature, TYPE_CREATURE);
	  td_creature.preferred_controller = player;

	  int polyp_counters = count_counters(player, card, COUNTER_POLYP);
	  int can_sac_island = can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0);
	  int priority_polyp = polyp_counters == 0 ? 1 : -1;
	  int can_pay_for_01 = can_target(&td_blue) && can_target(&td_creature) && polyp_counters >= 1;

	  enum
	  {
		CHOICE_POLYP = 1,
		CHOICE_01
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Add Polyp counter",	can_sac_island,	priority_polyp,
						"Add +0/+1 counter",	can_pay_for_01,	9,				DLG_MANA(MANACOST_U(1)));

	  switch (event)
		{
		  case EVENT_CAN_ACTIVATE:
			return choice;

		  case EVENT_ACTIVATE:
			switch (choice)
			  {
				case CHOICE_POLYP:
				  ;int island = pick_special_permanent_for_sacrifice(player, card, 0, TYPE_LAND, 0, SUBTYPE_ISLAND, 0, 0, 0, 0, 0, -1, 0);
				  if (island == -1)
					cancel = 1;
				  else
					kill_card(player, island, KILL_SACRIFICE);
				  break;

				case CHOICE_01:
				  ;card_instance_t* inst = get_card_instance(player, card);
				  inst->number_of_targets = 0;
				  if (!new_pick_target(&td_blue,
									   get_sleighted_color_text(player, card, "Select an untapped %s creature you control.", COLOR_BLUE),
									   0, GS_LITERAL_PROMPT|1))
					return 0;
				  target_t tapper = inst->targets[0];
				  inst->number_of_targets = 0;
				  if (!new_pick_target(&td_creature, "TARGET_CREATURE", 0, 1))
					return 0;
				  tap_card(tapper.player, tapper.card);
				  remove_counter(player, card, COUNTER_POLYP);
				  break;
			  }
			break;

		  case EVENT_RESOLVE_ACTIVATION:
			;card_instance_t* inst = get_card_instance(player, card);
			switch (choice)
			  {
				case CHOICE_POLYP:
				  if (in_play(inst->parent_controller, inst->parent_card))
					add_counters(inst->parent_controller, inst->parent_card, COUNTER_POLYP, 2);
				  break;

				case CHOICE_01:
				  if (in_play(inst->targets[0].player, inst->targets[0].card))
					add_counter(inst->targets[0].player, inst->targets[0].card, COUNTER_P0_P1);
				  break;
			  }

		  default:break;
		}
	}

  return global_enchantment(player, card, event);
}

int fx_dark_maze(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (!affect_me(inst->damage_target_player, inst->damage_target_card))
		return 0;
	  add_status(affected_card_controller, affected_card, STATUS_WALL_CAN_ATTACK);
	}
  if (eot_trigger(player, card, event))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  kill_card(inst->damage_target_player, inst->damage_target_card, KILL_REMOVE);
	}
  if (trigger_condition == TRIGGER_EOT && event == EVENT_END_TRIGGER && player == reason_for_trigger_controller)
	kill_card(player, card, KILL_REMOVE);	// presumeably the trigger was countered; don't run it again next turn
  return 0;
}
int card_dark_maze(int player, int card, event_t event)
{
  /* Dark Maze	|4|U	0x200d6c1
   * Creature - Wall 4/5
   * Defender
   * |0: ~ can attack this turn as though it didn't have defender. Exile it at the beginning of the next end step. */

  if (!IS_GAA_EVENT(event))
	return 0;

  card_instance_t* inst = get_card_instance(player, card);
  if (event == EVENT_RESOLVE_ACTIVATION && in_play(inst->parent_controller, inst->parent_card))
	create_targetted_legacy_effect(inst->parent_controller, inst->parent_card, &fx_dark_maze,
								   inst->parent_controller, inst->parent_card);

  if (event == EVENT_ACTIVATE && IS_AI(player)
	  && ((inst->token_status & STATUS_WALL_CAN_ATTACK)	// once is enough, thanks
		  || has_activation_on_stack(player, card)		// no, really, stop repeating yourself
		  || !(inst->regen_status & KEYWORD_DEFENDER)))	// no need
	{
	  if (ai_is_speculating == 1)
		cancel = 1;	// don't consider this branch
	  else
		ai_modifier -= 48;
	}

  return generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL);
}

/* Daughter of Autumn	|2|G|G	0x000000	[See also Hazduhr the Abbot below]
 * Legendary Creature - Avatar 2/4
 * |W: The next 1 damage that would be dealt to target |Swhite creature this turn is dealt to ~ instead. */

/* Death Speakers	|W	=>vanilla
 * Creature - Human Cleric 1/1
 * Protection from |Sblack */

int card_didgeridoo(int player, int card, event_t event){
	/* Didgeridoo	|1	0x2002D25
	 * Artifact
	 * |3: You may put a Minotaur permanent card from your hand onto the battlefield. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	char msg[100] = "Select a Minotaur permanent card.";
	test_definition_t this_test2;
	new_default_test_definition(&this_test2, TYPE_PERMANENT, msg);
	this_test2.subtype = SUBTYPE_MINOTAUR;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test2);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_X(3), 0, NULL, NULL);
}

int card_drudge_spell(int player, int card, event_t event){
	/* Drudge Spell	|B|B	0x2002D2A
	 * Enchantment
	 * |B, Exile two creature cards from your graveyard: Put a 1/1 |Sblack Skeleton creature token onto the battlefield. It has "|B: Regenerate this creature."
	 * When ~ leaves the battlefield, destroy all Skeleton tokens. They can't be regenerated. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->info_slot = internal_rand(99);
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL) ){
			return count_graveyard_by_type(player, TYPE_CREATURE) > 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			test_definition_t this_test2;
			new_default_test_definition(&this_test2, TYPE_CREATURE, "Select a creature card to exile.");

			if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test2) != -1 ){
				new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test2);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_SKELETON);
	}

	if( leaves_play(player, card, event) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_PERMANENT, "");
		test.subtype = SUBTYPE_SKELETON;
		test.type_flag = F1_IS_TOKEN;
		new_manipulate_all(player, card, ANYBODY, &test, KILL_BURY);
	}

	return global_enchantment(player, card, event);
}

int card_dry_spell(int player, int card, event_t event){
	/* Dry Spell	|1|B	0x200a412
	 * Sorcery
	 * ~ deals 1 damage to each creature and each player. */

	if( event == EVENT_RESOLVE_SPELL ){
		new_damage_all(player, card, ANYBODY, 1, NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_dwarven_pony(int player, int card, event_t event)
{
	/* Dwarven Pony	|R	0x200b51f
	 * Creature - Horse 1/1
	 * |1|R, |T: Target Dwarf creature gains |H2mountainwalk until end of turn. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_subtype = SUBTYPE_DWARF;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
							 0, 0, get_hacked_walk(player, card, KEYWORD_MOUNTAINWALK), 0);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_UNTAPPED, MANACOST_XR(1, 1), 0, &td, "TARGET_DWARF_CREATURE");
}

static int
fx_dwarven_sea_clan(int player, int card, event_t event)
{
  if ((trigger_condition == TRIGGER_END_COMBAT || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER || event == EVENT_SHOULD_AI_PLAY)
		{
		  card_instance_t* inst = get_card_instance(player, card);
		  if (inst->damage_target_player != -1)
			damage_creature(inst->damage_target_player, inst->damage_target_card, 2,
							inst->damage_source_player, inst->damage_source_card);
		  kill_card(player, card, KILL_REMOVE);
		}
	}

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);
  return 0;
}
static const char*
target_controls_island(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
  return (basiclandtypes_controlled[player][get_hacked_color(targeting_player, targeting_card, COLOR_BLUE)] >= 0
		  ? NULL : get_hacked_land_text(targeting_player, targeting_card, "controls %a", SUBTYPE_ISLAND));
}
int card_dwarven_sea_clan(int player, int card, event_t event)
{
  /* Dwarven Sea Clan	|2|R	0x200d6c6
   * Creature - Dwarf 1/1
   * |T: Choose target attacking or blocking creature whose controller controls |Han Island. ~ deals 2 damage to that creature at end of combat. Activate this ability only before the end of combat step. */
  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_state = TARGET_STATE_IN_COMBAT;
  td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
  td.extra = (int)target_controls_island;

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  create_targetted_legacy_effect(inst->parent_controller, inst->parent_card, &fx_dwarven_sea_clan,
									 inst->targets[0].player, inst->targets[0].card);
	}

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_UNTAPPED, MANACOST0, 0, &td, "TARGET_ATTACKING_BLOCKING_CREATURE");
}

/* Dwarven Trader	|R	=>vanilla
 * Creature - Dwarf 1/1 */

/* Ebony Rhino	|7	=>vanilla
 * Artifact Creature - Rhino 4/5
 * Trample */

int card_eron_the_relentless(int player, int card, event_t event){
	/* Eron the Relentless	|3|R|R	0x200a417
	 * Legendary Creature - Human Rogue 5/2
	 * Haste
	 * |R|R|R: Regenerate ~. */

	check_legend_rule(player, card, event);
	haste(player, card, event);
	return regeneration(player, card, event, 0, 0, 0, 0, 3, 0);
}

int card_evaporate(int player, int card, event_t event)
{
  /* Evaporate	|2|R	0x200d6cb
   * Sorcery
   * ~ deals 1 damage to each |Swhite and/or |Sblue creature. */
  if (event == EVENT_RESOLVE_SPELL)
	{
	  damage_all(player, card, ANYBODY, 1, 0, 0, 0,0, 0,0, get_sleighted_color_test(player, card, COLOR_TEST_BLUE|COLOR_TEST_WHITE),1, 0,0, -1,0);
	  kill_card(player, card, KILL_DESTROY);
	}
  return basic_spell(player, card, event);
}

int card_faerie_noble(int player, int card, event_t event)
{
	/* Faerie Noble	|2|G	0x200a41c
	 * Creature - Faerie 1/2
	 * Flying
	 * Other Faerie creatures you control get +0/+1.
	 * |T: Other Faerie creatures you control get +1/+0 until end of turn. */

  // Other Faerie creatures you control get +0/+1.
  boost_creature_type(player, card, event, SUBTYPE_FAERIE, 0, 1, 0, BCT_CONTROLLER_ONLY);

  // |T: Other Faerie creatures you control get +1/+0 until end of turn.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;
	  test.subtype = SUBTYPE_FAERIE;

	  card_instance_t* instance = get_card_instance(player, card);
	  pump_creatures_until_eot(instance->parent_controller, instance->parent_card, player, 0, 1,0, 0,0, &test);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(0), 0, NULL, NULL);
}

int card_feast_of_the_unicorn(int player, int card, event_t event){
	/* Feast of the Unicorn	|3|B	0x200a421
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +4/+0. */

	return generic_aura(player, card, event, player, 4, 0, 0, 0, 0, 0, 0);
}

int card_ferozs_ban(int player, int card, event_t event){
	/* Feroz's Ban	|6	0x200a426
	 * Artifact
	 * Creature spells cost |2 more to cast. */

	if( event == EVENT_MODIFY_COST_GLOBAL && is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_humiliated(player, card) ){
		COST_COLORLESS+=2;
	}
	return 0;
}

int card_folk_of_an_havva(int player, int card, event_t event)
{
  /* Folk of An-Havva	|G	0x200d6d0
   * Creature - Human 1/1
   * Whenever ~ blocks, it gets +2/+0 until end of turn. */
  if (event == EVENT_POW_BOOST && current_turn != player && current_phase <= PHASE_DECLARE_BLOCKERS)
	event_result += 2;
  if (blocking(player, card, event) && !is_humiliated(player, card))
	pump_until_eot(player, card, player, card, 2, 0);
  return 0;
}

int card_forget(int player, int card, event_t event){
	/* Forget	|U|U	0x200a42b
	 * Sorcery
	 * Target player discards two cards, then draws as many cards as he or she discarded this way. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = 2;
			if( amount > hand_count[instance->targets[0].player] ){
				amount = hand_count[instance->targets[0].player];
			}
			new_multidiscard(instance->targets[0].player, 2, 0, player);
			draw_cards(instance->targets[0].player, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_funeral_march(int player, int card, event_t event)
{
  /* Funeral March	|1|B|B	0x200d6da
   * Enchantment - Aura
   * Enchant creature
   * When enchanted creature leaves the battlefield, its controller sacrifices a creature. */

  card_instance_t* inst = get_card_instance(player, card);
  if (other_leaves_play(player, card, inst->damage_target_player, inst->damage_target_card, event))
	{
	  card_instance_t* ench = in_play(inst->damage_target_player, inst->damage_target_card);
	  if (ench)
		ench->state |= STATE_CANNOT_TARGET;	// Since TRIGGER_LEAVE_PLAY is, in most cases, sent with that card still on the battlefield
	  impose_sacrifice(player, card, inst->damage_target_player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

  return vanilla_aura(player, card, event, 1-player);
}

static void first_strike_if_white(int player, int card, int t_player, int t_card)
{
  if (get_color(t_player, t_card) & get_sleighted_color_test(player, card, COLOR_TEST_WHITE))
	pump_ability_until_eot(player, card, player, card, 0,0, KEYWORD_FIRST_STRIKE,0);
}
int card_ghost_hounds(int player, int card, event_t event)
{
  /* Ghost Hounds	|1|B	0x200d6df
   * Creature - Hound Spirit 1/1
   * Vigilance
   * Whenever ~ blocks or becomes blocked by a |Swhite creature, ~ gains first strike until end of turn. */

  vigilance(player, card, event);

  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);

	  if (inst->state & STATE_ATTACKING)
		for_each_creature_blocking_me(player, card, first_strike_if_white, player, card);

	  if (inst->state & STATE_BLOCKING)
		for_each_creature_blocked_by_me(player, card, first_strike_if_white, player, card);
	}

  return 0;
}

/* Giant Albatross	|1|U	0x000000
 * Creature - Bird 1/1
 * Flying
 * When ~ dies, you may pay |1|U. If you do, for each creature that dealt damage to ~ this turn, destroy that creature unless its controller pays 2 life. A creature destroyed this way can't be regenerated. */

static int giant_oyster_effect(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[1].player > -1 ){
		if( event == EVENT_DRAW_PHASE && current_turn == instance->targets[1].player && is_tapped(instance->targets[1].player, instance->targets[1].card) ){
			add_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card, 1);
		}
		if( event == EVENT_UNTAP ){
			if( affect_me(instance->targets[1].player, instance->targets[1].card) ){
				int amount = count_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card);
				remove_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_M1_M1, amount);
			}
			if( current_phase == PHASE_UNTAP && affect_me(instance->targets[0].player, instance->targets[0].card) ){
				does_not_untap(instance->targets[0].player, instance->targets[0].card, event);
			}
		}
		if( leaves_play(instance->targets[1].player, instance->targets[1].card, event) ){
			int amount = count_minus1_minus1_counters(instance->targets[0].player, instance->targets[0].card);
			remove_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_M1_M1, amount);
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

int card_giant_oyster(int player, int card, event_t event){
	/* Giant Oyster	|2|U|U	0x000000:200a430
	 * Creature - Oyster 0/3
	 * You may choose not to untap ~ during your untap step.
	 * |T: For as long as ~ remains tapped, target tapped creature doesn't untap during its controller's untap step, and at the beginning of each of your draw steps, put a -1/-1 counter on that creature. When ~ leaves the battlefield or becomes untapped, remove all -1/-1 counters from the creature. */

	choose_to_untap(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int legacy = create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &giant_oyster_effect,
														instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].player = player;
			leg->targets[1].card = instance->parent_card;
			card_instance_t *parent = get_card_instance(player, instance->parent_card);
			parent->targets[1] = instance->targets[0];
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_grandmother_sengir(int player, int card, event_t event){
	/* Grandmother Sengir	|4|B	0x2002D2F
	 * Legendary Creature - Human Wizard 3/3
	 * |1|B, |T: Target creature gets -1/-1 until end of turn. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, -1, -1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_XB(1, 1), 0, &td, "TARGET_CREATURE");
}

static int greater_werewolf_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_M0_M2);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

static void attach_greater_werewolf_legacy(int player, int card, int t_player, int t_card)
{
	create_targetted_legacy_effect(player, card, &greater_werewolf_legacy, t_player, t_card);
}

int card_greater_werewolf(int player, int card, event_t event){
	/* Greater Werewolf	|4|B	0x2002D34
	 * Creature - Werewolf 2/4
	 * At end of combat, put a -0/-2 counter on each creature blocking or blocked by ~. */

	if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card)){
		card_instance_t *instance = get_card_instance( player, card);
		if (player == current_turn && (instance->state & STATE_ATTACKING)){
			for_each_creature_blocking_me(player, card, attach_greater_werewolf_legacy, player, card);
		}
		if (player == 1-current_turn && (instance->state & STATE_BLOCKING)){
			for_each_creature_blocked_by_me(player, card, attach_greater_werewolf_legacy, player, card);
		}
	}

	return 0;
}

/* Hazduhr the Abbot	|3|W|W	0x000000	[see also Daughter of Autumn above]
 * Legendary Creature - Human Cleric 2/5
 * |X, |T: The next X damage that would be dealt this turn to target |Swhite creature you control is dealt to ~ instead. */

int card_headstone(int player, int card, event_t event)
{
  /* Headstone	|1|B	0x200d6e4
   * Instant
   * Exile target card from a graveyard.
   * Draw a card at the beginning of the next turn's upkeep. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int selected = validate_target_from_grave_source(player, card, inst->targets[0].player, 1);
	  if (selected != -1)
		{
		  rfg_card_from_grave(inst->targets[0].player, selected);
		  cantrip(player, card, 1);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  test_definition_t test;
  new_default_test_definition(&test, TYPE_ANY, "Select target card to exile.");
  return generic_spell(player, card, event, GS_GRAVE_RECYCLER_BOTH_GRAVES, NULL, NULL, 1, &test);
}

int card_heart_wolf(int player, int card, event_t event)
{
	/* Heart Wolf	|3|R	0x200b524
	 * Creature - Wolf 2/2
	 * First strike
	 * |T: Target Dwarf creature gets +2/+0 and gains first strike until end of turn. When that creature leaves the battlefield this turn, sacrifice ~. Activate this ability only during combat. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.required_subtype = SUBTYPE_DWARF;

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  int leg = pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
										   2, 0, KEYWORD_FIRST_STRIKE, 0);
		  if (leg != -1)
			{
			  card_instance_t* legacy = get_card_instance(player, leg);
			  legacy->targets[3].player = PAUE_KILL_SOURCE_IF_LEAVES_BATTLEFIELD;
			  legacy->targets[3].card = KILL_SACRIFICE;
			}
		}
	}

  if (event == EVENT_CAN_ACTIVATE && !(current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2))
	return 0;

  return generic_activated_ability(player, card, event, GAA_CAN_TARGET|GAA_UNTAPPED, MANACOST_X(0), 0, &td, "TARGET_DWARF_CREATURE");
}

int card_hungry_mist(int player, int card, event_t event){
	/* Hungry Mist	|2|G|G	0x200a435
	 * Creature - Elemental 6/2
	 * At the beginning of your upkeep, sacrifice ~ unless you pay |G|G. */

	basic_upkeep(player, card, event, MANACOST_G(2));
	return 0;
}

int card_ihsans_shade(int player, int card, event_t event){
	/* Ihsan's Shade	|3|B|B|B	0x200accd
	 * Legendary Creature - Shade Knight 5/5
	 * Protection from |Swhite */

	check_legend_rule(player, card, event);
	return 0;
}

int card_irini_sengir(int player, int card, event_t event)
{
  /* Irini Sengir	|2|B|B	0x200cb4f
   * Legendary Creature - Vampire Dwarf 2/2
   * |SGreen enchantment spells and |Swhite enchantment spells cost |2 more to cast. */

  if (event == EVENT_MODIFY_COST_GLOBAL)
	{
	  if (is_what(affected_card_controller, affected_card, TYPE_ENCHANTMENT)
		  && (get_color_real_or_iid(event_result, affected_card_controller, affected_card)
			  & get_sleighted_color_test(player, card, COLOR_TEST_WHITE | COLOR_TEST_GREEN))
		  && !is_humiliated(player, card))
		COST_COLORLESS += 2;
	  return 0;
	}

  check_legend_rule(player, card, event);
  return 0;
}

int card_ironclaw_curse(int player, int card, event_t event)
{
  /* Ironclaw Curse	|R	0x200d6e9
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets -0/-1.
   * Enchanted creature can't block creatures with power equal to or greater than the enchanted creature's toughness. */

  if (event == EVENT_BLOCK_LEGALITY)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (inst->damage_target_player < 0 || !affect_me(inst->damage_target_player, inst->damage_target_card))
		return 0;
	  if (get_power(attacking_card_controller, attacking_card) >= get_toughness(inst->damage_target_player, inst->damage_target_card))
		event_result = 1;
	}

  return generic_aura(player, card, event, 1-player, 0,-1, 0,0, 0, 0, 0);
}

int fx_change_landtype(int player, int card, event_t event)
{
  // changes to the basic landtype set by the color in targets[1].player.  COLOR_BLACK => SUBTYPE_SWAMP, etc.
  if (event == EVENT_CHANGE_TYPE && !(land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (!affect_me(inst->damage_target_player, inst->damage_target_card))
		return 0;
	  event_result = inst->targets[1].player - 1;	// Since iids 0-4 are conveniently Swamp, Island, Forest, Mountain, Plains
	}

  if (event == EVENT_CLEANUP && !(get_card_instance(player, card)->token_status & STATUS_PERMANENT))
	kill_card(player, card, KILL_REMOVE);
  return 0;
}
int card_jinx(int player, int card, event_t event)
{
  /* Jinx	|1|U	0x200d6ee
   * Instant
   * Target land becomes the basic land type of your choice until end of turn.
   * Draw a card at the beginning of the next turn's upkeep. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  card_instance_t* inst = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  int color = choose_a_land(player, COLOR_BLACK + recorded_rand(player, COLOR_WHITE - COLOR_BLACK + 1));
		  int leg = create_targetted_legacy_effect(player, card, &fx_change_landtype, inst->targets[0].player, inst->targets[0].card);
		  card_instance_t* legacy = get_card_instance(player, leg);
		  legacy->targets[1].player = color;
		  legacy->eot_toughness = 1;
		  get_card_instance(player, cantrip(player, card, 1))->eot_toughness = 2;
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
}

int card_joven(int player, int card, event_t event){
	/* Joven	|3|R|R	0x200a43a
	 * Legendary Creature - Human Rogue 3/3
	 * |R|R|R, |T: Destroy target noncreature artifact. */

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.illegal_type = TYPE_CREATURE;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_LITERAL_PROMPT | GAA_CAN_TARGET, MANACOST_R(3), 0,
									&td, "Select target noncreature artifact");
}

static int effect_jovens_ferrets(int player, int card, event_t event)
{
  if (end_of_combat_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  /* Ruling 10/1/2008: The second ability will trigger only if Joven's Ferrets is still on the battlefield at end of combat. If it's been dealt lethal
	   * combat damage and destroyed during that combat (or has left the battlefield during that combat by any other means), its ability won't trigger and the
	   * creatures that blocked it won't be affected. */
	  if (in_play(instance->damage_source_player, instance->damage_source_card))
		  alternate_legacy_text(2, instance->damage_source_player,
								does_not_untap_effect(instance->damage_source_player, instance->damage_source_card,
													  instance->damage_target_player, instance->damage_target_card,
													  EDNT_TAP_TARGET, 1));
	  // And don't remove the effect now, in case there's another combat this turn.
	}

  if (event == EVENT_CLEANUP)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_jovens_ferrets(int player, int card, event_t event)
{
	/* Joven's Ferrets	|G	0x200a43f
	 * Creature - Ferret 1/1
	 * Whenever ~ attacks, it gets +0/+2 until end of turn.
	 * At end of combat, tap all creatures that blocked ~ this turn. They don't untap during their controller's next untap step. */

  // Whenever ~ attacks, it gets +0/+2 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	alternate_legacy_text(1, player, pump_until_eot(player, card, player, card, 0, 2));

	// At end of combat, tap all creatures that blocked ~ this turn. They don't untap during their controller's next untap step.
  if (event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card))
	{
	  char marked[2][151] = {{0}};
	  mark_each_creature_blocking_me(player, card, marked);
	  int c;
	  for (c = 0; c < active_cards_count[1-player]; ++c)
		if (marked[1-player][c])
		  alternate_legacy_text(2, player, create_targetted_legacy_effect(player, card, effect_jovens_ferrets, 1-player, c));
	}

  return 0;
}

static int effect_cant_be_blocked_except_by_walls(int player, int card, event_t event){
  if (event == EVENT_BLOCK_LEGALITY)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->damage_target_card >= 0
		  && attacking_card_controller == instance->damage_target_player && attacking_card == instance->damage_target_card
		  && !has_subtype(affected_card_controller, affected_card, SUBTYPE_WALL))
		event_result = 1;
	}

  if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_jovens_tools(int player, int card, event_t event){
	/* Joven's Tools	|6	0x200a444
	 * Artifact
	 * |4, |T: Target creature can't be blocked this turn except by Walls. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player ;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &effect_cant_be_blocked_except_by_walls,
											instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(4), 0, &td, "TARGET_CREATURE");
}

int card_koskun_falls(int player, int card, event_t event){
	/* Koskun Falls	|2|B|B	0x200a449
	 * World Enchantment
	 * At the beginning of your upkeep, sacrifice ~ unless you tap an untapped creature you control.
	 * Creatures can't attack you unless their controller pays |2 for each creature he or she controls that's attacking you. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		base_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.illegal_state = TARGET_STATE_TAPPED;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance( player, card );
			tap_card(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		} else {
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	tax_attack(player, card, event);

	return enchant_world(player, card, event);
}

int card_koskun_keep(int player, int card, event_t event)
{
  /* Koskun Keep	""	0x200d68f
   * Land
   * |T: Add |1 to your mana pool.
   * |1, |T: Add |R to your mana pool.
   * |2, |T: Add |B or |G to your mana pool. */
  return homelands_triple_land(player, card, event, COLOR_TEST_RED, COLOR_TEST_BLACK|COLOR_TEST_GREEN);
}

/* Labyrinth Minotaur	|3|U	=>m11.c:card_wall_of_frost
 * Creature - Minotaur 1/4
 * Whenever ~ blocks a creature, that creature doesn't untap during its controller's next untap step. */

int card_leaping_lizard(int player, int card, event_t event)
{
  /* Leaping Lizard	|1|G|G	0x200d6f3
   * Creature - Lizard 2/3
   * |1|G: ~ gets -0/-1 and gains flying until end of turn. */
  return generic_shade(player, card, event, 0, MANACOST_XG(1, 1), 0,-1, KEYWORD_FLYING,0);
}

int card_leeches(int player, int card, event_t event){
	/* Leeches	|1|W|W	0x200a453
	 * Sorcery
	 * Target player loses all poison counters. ~ deals that much damage to that player. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = POISON_COUNTERS(instance->targets[0].player);
			raw_set_poison(instance->targets[0].player, 0);
			damage_player(instance->targets[0].player, amount, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

static void give_first_strike(int player, int card, int t_player, int t_card)
{
  pump_ability_until_eot(player, card, t_player, t_card, 0,0, KEYWORD_FIRST_STRIKE,0);
}
int card_mammoth_harness(int player, int card, event_t event)
{
  /* Mammoth Harness	|3|G	0x200d6f8
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature loses flying.
   * Whenever enchanted creature blocks or becomes blocked by a creature, the other creature gains first strike until end of turn. */

  if (event == EVENT_ABILITIES)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (!affect_me(inst->damage_target_player, inst->damage_target_card))
		return 0;
	  event_result &= ~KEYWORD_FLYING;
	}

  if (event == EVENT_DECLARE_BLOCKERS)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  card_instance_t* ench = get_card_instance(inst->damage_target_player, inst->damage_target_card);

	  if (ench->state & STATE_ATTACKING)
		for_each_creature_blocking_me(inst->damage_target_player, inst->damage_target_card, give_first_strike, player, card);

	  if (ench->state & STATE_BLOCKING)
		for_each_creature_blocked_by_me(inst->damage_target_player, inst->damage_target_card, give_first_strike, player, card);
	}

  return vanilla_aura(player, card, event, 1-player);
}

int card_marjhan(int player, int card, event_t event){
	/* Marjhan	|5|U|U	0x2002D39
	 * Creature - Leviathan 8/8
	 * ~ doesn't untap during your untap step.
	 * |U|U, Sacrifice a creature: Untap ~. Activate this ability only during your upkeep.
	 * ~ can't attack unless defending player controls |Han Island.
	 * |U|U: ~ gets -1/-0 until end of turn and deals 1 damage to target attacking creature without flying.
	 * When you control no |H1Islands, sacrifice ~. */

	does_not_untap(player, card, event);

	islandhome(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities |= KEYWORD_FLYING;
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( player == AI && current_turn == player && upkeep_trigger(player, card, event) && count_upkeeps(player) &&
		is_tapped(player, card) && count_subtype(1-player, TYPE_LAND, SUBTYPE_ISLAND) > 0
	  ){
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_CREATURE, MANACOST_U(2), 0, NULL, NULL) &&
			count_subtype(player, TYPE_CREATURE, -1)
		  ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_U(2)) ){
				state_untargettable(player, card, 1);
				if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					untap_card(player, card);
				}
				state_untargettable(player, card, 0);
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if( player != AI &&
			generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_ONLY_ON_UPKEEP, MANACOST_U(2), 0, NULL, NULL)
		  ){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_U(2), 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		if( player != AI &&
			generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_CREATURE | GAA_ONLY_ON_UPKEEP, MANACOST_U(2), 0, NULL, NULL)
		  ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_U(2), 0, &td, NULL) ){
				choice = do_dialog(player, player, card, player, card, " Untap Marjhan\n Damage a creature\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, 0, 0, 2, 0, 0, 0) ){
			instance->number_of_targets = 0;
			if( choice == 0 ){
				if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
					instance->info_slot = 66+choice;
				}
				else{
					spell_fizzled = 1;
				}
			}
			if( choice == 1 ){
				if( new_pick_target(&td, "Select target attacking creature with flying.", 0, 1 | GS_LITERAL_PROMPT) ){
					instance->info_slot = 66+choice;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			untap_card(instance->parent_controller, instance->parent_card);
		}

		if( instance->info_slot == 67 ){
			if( valid_target(&td) ){
				pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, -1, 0);
				damage_creature(instance->targets[0].player, instance->targets[0].card, 1, player, instance->parent_card);
			}
		}
	}

	return 0;
}

int card_memory_lapse(int player, int card, event_t event){
	/* Memory Lapse	|1|U	0x200ccc6
	 * Instant
	 * Counter target spell. If that spell is countered this way, put it on top of its owner's library instead of into that player's graveyard. */

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			card_instance_t *instance = get_card_instance(player, card);
			if( manage_counterspell_linked_hacks(player, card, instance->targets[0].player, instance->targets[0].card) != KILL_REMOVE ){
				put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_merchant_scroll(int player, int card, event_t event){
	/* Merchant Scroll	|1|U	0x2003F95
	 * Sorcery
	 * Search your library for a |Sblue instant card, reveal that card, and put it into your hand. Then shuffle your library. */
	// original code : 0040EA00

	if( event == EVENT_RESOLVE_SPELL ){
		char msg[100] = "Select an Instant card.";
		test_definition_t this_test2;
		new_default_test_definition(&this_test2, TYPE_INSTANT | TYPE_INTERRUPT, msg);
		this_test2.type_flag = F1_NO_CREATURE;
		this_test2.color = COLOR_TEST_BLUE;

		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test2);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

/* Mesa Falcon	|1|W	=>mirage.c:card_pearl_dragon
 * Creature - Bird 1/1
 * Flying
 * |1|W: ~ gets +0/+1 until end of turn. */

int card_mystic_decree(int player, int card, event_t event, int amount){
	/* Mystic Decree	|2|U|U	0x200a458
	 * World Enchantment
	 * All creatures lose flying and |H2islandwalk. */

	if( event == EVENT_ABILITIES && (event_result & (KEYWORD_FLYING | KEYWORD_BASIC_LANDWALK)) &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE) && ! is_humiliated(player, card)
	  ){
		event_result &= ~(KEYWORD_FLYING | get_hacked_walk(player, card, KEYWORD_ISLANDWALK));
	}

	return enchant_world(player, card, event);
}

/* Narwhal	|2|U|U	=>invasion.c:card_galinas_knight
 * Creature - Whale 2/2
 * First strike, protection from |Sred */

int card_orcish_mine(int player, int card, event_t event)
{
  /* Orcish Mine	|1|R|R	0x200d6fd
   * Enchantment - Aura
   * Enchant land
   * ~ enters the battlefield with three ore counters on it.
   * At the beginning of your upkeep or whenever enchanted land becomes tapped, remove an ore counter from ~.
   * When the last ore counter is removed from ~, destroy enchanted land and ~ deals 2 damage to that land's controller. */

  enters_the_battlefield_with_counters(player, card, event, COUNTER_ORE, 3);

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	remove_counter(player, card, COUNTER_ORE);

  if (event == EVENT_TAP_CARD)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (affect_me(inst->damage_target_player, inst->damage_target_card) && !is_humiliated(player, card))
		remove_counter(player, card, COUNTER_ORE);
	}

  if (event == EVENT_STATIC_EFFECTS && in_play(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  if (count_counters(player, card, COUNTER_ORE) > 0)
		inst->info_slot = CARD_ID_ORCISH_MINE;
	  else if (inst->info_slot == CARD_ID_ORCISH_MINE)	// had counters previously; doesn't anymore
		{
		  if (is_humiliated(player, card))
			inst->info_slot = 0;	// so it doesn't trigger later when it stops being humiliated
		  else
			{
			  damage_player(inst->damage_target_player, 2, player, card);
			  kill_card(inst->damage_target_player, inst->damage_target_card, KILL_DESTROY);
			}
		}
	}

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);
  td.preferred_controller = 1-player;
  return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_primal_order(int player, int card, event_t event){
	/* Primal Order	|2|G|G	0x2002D43
	 * Enchantment
	 * At the beginning of each player's upkeep, ~ deals damage to that player equal to the number of nonbasic lands he or she controls. */

	upkeep_trigger_ability(player, card, event, 2);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int dmg = count_subtype(current_turn, TYPE_LAND, -1)-count_subtype(current_turn, TYPE_LAND, SUBTYPE_BASIC);
		if( dmg > 0 ){
			damage_player(current_turn, dmg, player, card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_prophecy(int player, int card, event_t event)
{
  /* Prophecy	|W	0x200d702
   * Sorcery
   * Reveal the top card of target opponent's library. If it's a land, you gain 1 life. Then that player shuffles his or her library.
   * Draw a card at the beginning of the next turn's upkeep. */

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
		  int tgt = get_card_instance(player, card)->targets[0].player;
		  int iid = deck_ptr[tgt][0];
		  if (iid != -1)
			{
			  reveal_card_iid(player, card, iid);
			  if (is_what(-1, iid, TYPE_LAND))
				gain_life(player, 1);
			}
		  shuffle(tgt);	// even if empty - rule 701.16e
		  cantrip(player, card, 1);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_OPPONENT", 1, NULL);
}

static void set_val_if_color(int color_test, int val, int t_player, int t_card)
{
  if (get_color(t_player, t_card) & color_test)
	*((int*)val) = 1;
}
int card_rashka_the_slayer(int player, int card, event_t event)
{
  /* Rashka the Slayer	|3|W|W	0x200d707
   * Legendary Creature - Human Archer 3/3
   * Reach
   * Whenever ~ blocks one or more |Sblack creatures, ~ gets +1/+2 until end of turn. */

  check_legend_rule(player, card, event);

  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int color_test_black = 1<<get_sleighted_color(player, card, COLOR_BLACK);
	  int blocking_black = 0;
	  if (inst->state & STATE_BLOCKING)
		for_each_creature_blocked_by_me(player, card, set_val_if_color, color_test_black, (int)(&blocking_black));
	  if (blocking_black)
		pump_until_eot(player, card, player, card, 1,2);
	}

  return 0;
}

int card_reef_pirates(int player, int card, event_t event)
{
	/* Reef Pirates	|1|U|U	0x200b529
	 * Creature - Zombie Pirate 2/2
	 * Whenever ~ deals damage to an opponent, that player puts the top card of his or her library into his or her graveyard. */

  if (damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT))
	mill(1-player, 1);

  return 0;
}

int card_renewal(int player, int card, event_t event)
{
  /* Renewal	|2|G	0x200d70c
   * Sorcery
   * As an additional cost to cast ~, sacrifice a land.
   * Search your library for a basic land card and put that card onto the battlefield. Then shuffle your library.
   * Draw a card at the beginning of the next turn's upkeep. */

  if (event == EVENT_CAN_CAST && basic_spell(player, card, event))
	return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);

  if (event == EVENT_CAST_SPELL && affect_me(player, card)
	  && !sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0))
	cancel = 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  tutor_basic_land(player, 1, 0);
	  cantrip(player, card, 1);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_retribution(int player, int card, event_t event){
	/* Retribution	|2|R|R	0x2002D48
	 * Sorcery
	 * Choose two target creatures an opponent controls. That player chooses and sacrifices one of those creatures. Put a -1/-1 counter on the other. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL){
		int i;
		int trgs[2] = {0, 0};
		for(i=0; i<2; i++){
			if( validate_target(player, card, &td, i) ){
				trgs[i] = 1;
			}
		}

		if( trgs[0] + trgs[1] > 0 ){
			if( trgs[0] + trgs[1] > 1 ){
				int sacced = 0;
				i = 0;
				while( ! sacced ){
						if( trgs[i] ){
							if( do_dialog(instance->targets[i].player, instance->targets[i].player, instance->targets[i].card, -1, -1, " Sac this creature\n Pass", 0) == 0 ){
								if( can_sacrifice(player, instance->targets[i].player, 1, TYPE_CREATURE, 0) ){
									kill_card(instance->targets[i].player, instance->targets[i].card, KILL_SACRIFICE);
								}
								trgs[i] |= 2;
								sacced = 1;
							}
						}
						i++;
						if( i > 1 ){
							i = 0;
						}
				}
				for(i=0; i<2; i++){
					if( !(trgs[i] & 2) ){
						add_counter(instance->targets[i].player, instance->targets[i].card, COUNTER_M1_M1);
						break;
					}
				}
			}
			else{
				for(i=0; i<2; i++){
					if(	trgs[i] ){
						if( can_sacrifice(player, instance->targets[i].player, 1, TYPE_CREATURE, 0) ){
							kill_card(instance->targets[i].player, instance->targets[i].card, KILL_SACRIFICE);
						}
						break;
					}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature opponent controls.", 2, NULL);
}

int card_reveka_wizard_savant(int player, int card, event_t event){
	/* Reveka, Wizard Savant	|2|U|U	0x200a45d
	 * Legendary Creature - Dwarf Wizard 0/1
	 * |T: ~ deals 2 damage to target creature or player and doesn't untap during your next untap step. */

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 2);
			if (in_play(instance->parent_controller, instance->parent_card) ){
			  does_not_untap_effect(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 
									EDNT_CHECK_ORIGINAL_CONTROLLER, 1);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_root_spider(int player, int card, event_t event)
{
  /* Root Spider	|3|G	0x200d6d5
   * Creature - Spider 2/2
   * Whenever ~ blocks, it gets +1/+0 and gains first strike until end of turn. */
  if (event == EVENT_POW_BOOST && current_turn != player && current_phase <= PHASE_DECLARE_BLOCKERS)
	event_result += 1;
  if (blocking(player, card, event) && !is_humiliated(player, card))
	pump_ability_until_eot(player, card, player, card, 1,0, KEYWORD_FIRST_STRIKE,0);
  return 0;
}

int card_roots(int player, int card, event_t event)
{
  /* Roots	|3|G	0x200a462
   * Enchantment - Aura
   * Enchant creature without flying
   * When ~ enters the battlefield, tap enchanted creature.
   * Enchanted creature doesn't untap during its controller's untap step. */

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = 1-player;
  td.illegal_abilities = KEYWORD_FLYING;

  card_instance_t* inst = get_card_instance(player, card);

  if (comes_into_play(player, card, event))
	tap_card(inst->damage_target_player, inst->damage_target_card);

  if (inst->damage_target_player >= 0)
	{
	  does_not_untap(inst->damage_target_player, inst->damage_target_card, event);
	  if (event == EVENT_STATIC_EFFECTS && !check_for_ability(inst->damage_target_player, inst->damage_target_card, KEYWORD_FLYING))
		kill_card(player, card, KILL_STATE_BASED_ACTION);
	}

  return targeted_aura(player, card, event, &td, "FLOOD");	// Select target creature without flying.
}

int card_roterothopter(int player, int card, event_t event, int amount)
{
  /* Roterothopter	|1	0x200a467
   * Artifact Creature - Thopter 0/2
   * Flying
   * |2: ~ gets +1/+0 until end of turn. Activate this ability no more than twice each turn. */

  int rval = generic_shade_merge_pt(player, card, event, 0, MANACOST_X(2), 1,0);

  card_instance_t* inst = get_card_instance(player, card);

  if (event == EVENT_CAN_ACTIVATE && rval && inst->info_slot >= 2)
	return 0;

  if (event == EVENT_ACTIVATE && cancel != 1)
	inst->info_slot = MAX(inst->info_slot + 1, 1);

  if (event == EVENT_POW_BOOST)
	rval = MIN(rval, 2 - inst->info_slot);

  if (event == EVENT_CLEANUP)
	inst->info_slot = 0;

  return rval;
}

int card_rysorian_badger(int player, int card, event_t event){
	/* Rysorian Badger	|2|G	0x000000:200a46c
	 * Creature - Badger 2/2
	 * Whenever ~ attacks and isn't blocked, you may exile up to two target creature cards from defending player's graveyard. If you do, you gain 1 life for each card exiled this way and ~ assigns no combat damage this turn. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && instance->targets[1].player < 1 && current_phase == PHASE_AFTER_BLOCKING && is_attacking(player, card) &&
		is_unblocked(player, card) && count_graveyard_by_type(1-player, TYPE_CREATURE) > 0 && ! graveyard_has_shroud(2)
	  ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		if( instance->targets[1].player < 0 ){
			instance->targets[1].player = 0;
		}
		instance->targets[1].player++;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		char msg[100] = "Select a creature card.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, msg);

		int count = MIN(count_graveyard_by_type(1-player, TYPE_CREATURE), 2);
		while( count > 0 ){
				int selected = new_global_tutor(player, 1-player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
				if( selected != 1 ){
					gain_life(player, 1);
				}
				count--;
		}
		negate_combat_damage_this_turn(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 0);
	}

	return 0;
}

/* Samite Alchemist	|3|W	0x000000
 * Creature - Human Cleric 0/2
 * |W|W, |T: Prevent the next 4 damage that would be dealt this turn to target creature you control. Tap that creature. It doesn't untap during your next untap step. */

/* Sea Sprite	|1|U	=>vanilla
 * Creature - Faerie 1/1
 * Flying, protection from |Sred */

int card_sea_troll(int player, int card, event_t event)
{
  /* Sea Troll	|2|U	0x200d71b
   * Creature - Troll 2/1
   * |U: Regenerate ~. Activate this ability only if ~ blocked or was blocked by a |Sblue creature this turn. */

  if (event == EVENT_DECLARE_BLOCKERS)
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int color_test_blue = 1<<get_sleighted_color(player, card, COLOR_BLUE);
	  int blocking_blue = 0;
	  if (inst->state & STATE_ATTACKING)
		for_each_creature_blocking_me(player, card, set_val_if_color, color_test_blue, (int)(&blocking_blue));
	  if (inst->state & STATE_BLOCKING)
		for_each_creature_blocked_by_me(player, card, set_val_if_color, color_test_blue, (int)(&blocking_blue));
	  if (blocking_blue)
		inst->info_slot = CARD_ID_SEA_TROLL;
	}

  if (event == EVENT_CLEANUP)
	get_card_instance(player, card)->info_slot = 0;

  if (event == EVENT_CAN_ACTIVATE && get_card_instance(player, card)->info_slot != CARD_ID_SEA_TROLL)
	return 0;

  return regeneration(player, card, event, MANACOST_U(1));
}

int card_sengir_autocrat(int player, int card, event_t event){
	/* Sengir Autocrat	|3|B	0x2002D4D
	 * Creature - Human 2/2
	 * When ~ enters the battlefield, put three 0/1 |Sblack Serf creature tokens onto the battlefield.
	 * When ~ leaves the battlefield, exile all Serf tokens. */

	if( comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_SERF, 3);
	}

	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "");
		this_test.subtype = SUBTYPE_SERF;
		new_manipulate_all(player, card, 2, &this_test, KILL_REMOVE);
	}

	return 0;
}

int card_sengir_bats(int player, int card, event_t event){
	/* Sengir Bats	|1|B|B	0x2007456
	 * Creature - Bat 1/2
	 * Flying
	 * Whenever a creature dealt damage by ~ this turn dies, put a +1/+1 counter on ~. */
	// also code for Sengir Vampire
	if( sengir_vampire_trigger(player, card, event, 2) ){
		add_1_1_counters(player, card, get_card_instance(player, card)->targets[11].card);
	}
	return 0;
}

int card_serra_aviary(int player, int card, event_t event){
	/* Serra Aviary	|3|W	0x200a471
	 * World Enchantment
	 * Creatures with flying get +1/+1. */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING) &&
		! is_humiliated(player, card)
	  ){
		event_result++;
	}
	return enchant_world(player, card, event);
}

/* Serra Bestiary	|W|W	0x000000
 * Enchantment - Aura
 * Enchant creature
 * At the beginning of your upkeep, sacrifice ~ unless you pay |W|W.
 * Enchanted creature can't attack or block, and its activated abilities with |T in their costs can't be activated. */

int card_serra_inquisitors(int player, int card, event_t event)
{
  /* Serra Inquisitors	|4|W	0x200d711
   * Creature - Human Cleric 3/3
   * Whenever ~ blocks or becomes blocked by one or more |Sblack creatures, ~ gets +2/+0 until end of turn. */

  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
	{
	  card_instance_t* inst = get_card_instance(player, card);
	  int color_test_black = 1<<get_sleighted_color(player, card, COLOR_BLACK);
	  int blocking_black = 0;
	  if (inst->state & STATE_ATTACKING)
		for_each_creature_blocking_me(player, card, set_val_if_color, color_test_black, (int)(&blocking_black));
	  if (inst->state & STATE_BLOCKING)
		for_each_creature_blocked_by_me(player, card, set_val_if_color, color_test_black, (int)(&blocking_black));
	  if (blocking_black)
		pump_until_eot(player, card, player, card, 2,0);
	}

  return 0;
}

int card_serra_paladin(int player, int card, event_t event)
{
  /* Serra Paladin	|2|W|W	0x200a476
   * Creature - Human Knight 2/2
   * |T: Prevent the next 1 damage that would be dealt to target creature or player this turn.
   * |1|W|W, |T: Target creature gains vigilance until end of turn. */

  if (!IS_GAA_EVENT(event))
	return 0;

  if (land_can_be_played & LCBP_DAMAGE_PREVENTION)
	return card_samite_healer(player, card, event);

  return vanilla_creature_pumper(player, card, event, MANACOST_XW(1, 2), GAA_UNTAPPED, 0,0, 0,SP_KEYWORD_VIGILANCE, NULL);
}

int card_serrated_arrows(int player, int card, event_t event){
	/* Serrated Arrows	|4	0x200181C
	 * Artifact
	 * ~ enters the battlefield with three arrowhead counters on it.
	 * At the beginning of your upkeep, if there are no arrowhead counters on ~, sacrifice it.
	 * |T, Remove an arrowhead counter from ~: Put a -1/-1 counter on target creature. */

	if (event == EVENT_SHOULD_AI_PLAY){
		ai_modifier += (player == AI ? 1 : -1) * count_counters(player, card, COUNTER_ARROWHEAD);
	}

	enters_the_battlefield_with_counters(player, card, event, COUNTER_ARROWHEAD, 3);

	if( trigger_condition == TRIGGER_UPKEEP && current_turn == player && count_counters(player, card, COUNTER_ARROWHEAD) <= 0 &&
		upkeep_trigger(player, card, event)
	  ){
	   kill_card(player, card, KILL_SACRIFICE);
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( validate_target(player, card, &td, 0) ){
			add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_M1_M1);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(0), GVC_COUNTERS(COUNTER_ARROWHEAD, 1), &td, "TARGET_CREATURE");
}

/* Shrink	|G	=>m13.c:card_hydrosurge
 * Instant
 * Target creature gets -5/-0 until end of turn. */

int card_soraya_the_falconer(int player, int card, event_t event){
	/* Soraya the Falconer	|1|W|W	0x20011A0
	 * Legendary Creature - Human 2/2
	 * Bird creatures get +1/+1.
	 * |1|W: Target Bird creature gains banding until end of turn. */

	check_legend_rule(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	boost_creature_type(player, card, event, SUBTYPE_BIRD, 1, 1, 0, BCT_INCLUDE_SELF);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_BIRD;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_BANDING, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XW(1, 1), 0, &td, "Select target Bird creature.");
}

int card_spectral_bears(int player, int card, event_t event){
	/* Spectral Bears	|1|G	0x200a480
	 * Creature - Bear Spirit 3/3
	 * Whenever ~ attacks, if defending player controls no |Sblack nontoken permanents, it doesn't untap during your next untap step. */

	if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "");
		this_test.type_flag = F1_NO_TOKEN;
		this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

		if (!check_battlefield_for_special_card(player, card, 1-current_turn, 0, &this_test) &&
			declare_attackers_trigger(player, card, event, 0, player, card)
		   ){
			does_not_untap_effect(player, card, player, card, EDNT_CHECK_ORIGINAL_CONTROLLER, 1);
		}
	}

	return 0;
}

/* Timmerian Fiends	|1|B|B	0x000000
 * Creature - Horror 1/1
 * Remove ~ from your deck before playing if you're not playing for ante.
 * |B|B|B, Sacrifice ~: The owner of target artifact may ante the top card of his or her library. If that player doesn't, exchange ownership of that artifact and ~. Put the artifact card into your graveyard and ~ from anywhere into that player's graveyard. This change in ownership is permanent. */

int card_torture(int player, int card, event_t event)
{
	/* Torture	|B	0x200b52e
	 * Enchantment - Aura
	 * Enchant creature
	 * |1|B: Put a -1/-1 counter on enchanted creature. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  add_minus1_minus1_counters(instance->damage_target_player, instance->damage_target_card, 1);
	}

  return disabling_aura(player, card, event) || generic_activated_ability(player, card, event, 0, MANACOST_XB(1,1), 0, NULL, NULL);
}

int card_trade_caravan(int player, int card, event_t event){
	/* Trade Caravan	|W	0x200a485
	 * Creature - Human Nomad 1/1
	 * At the beginning of your upkeep, put a currency counter on ~.
	 * Remove two currency counters from ~: Untap target basic land. Activate this ability only during an opponent's upkeep. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		add_counter(player, card, COUNTER_CURRENCY);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_BASIC;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_CAN_ACTIVATE && current_phase == PHASE_UPKEEP && current_turn != player ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, GVC_COUNTERS(COUNTER_CURRENCY, 2), &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_BASICLAND") ){
			instance->number_of_targets = 1;
			remove_counters(player, card, COUNTER_CURRENCY, 2);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

/* Truce	|2|W	=>portal_1_2_3k.c:card_temporary_truce
 * Instant
 * Each player may draw up to two cards. For each card less than two a player draws this way, that player gains 2 life. */

int card_veldrane_of_sengir(int player, int card, event_t event)
{
  /* Veldrane of Sengir	|5|B|B	0x200d716
   * Legendary Creature - Human Rogue 5/5
   * |1|B|B: ~ gets -3/-0 and gains |H2forestwalk until end of turn. */
  check_legend_rule(player, card, event);
  return generic_shade(player, card, event, 0, MANACOST_XB(1, 2), -3,0, KEYWORD_FORESTWALK,0);
}

int card_wall_of_kelp(int player, int card, event_t event){
	/* Wall of Kelp	|U|U	0x200173B
	 * Creature - Plant Wall 0/3
	 * Defender
	 * |U|U, |T: Put a 0/1 |Sblue Plant Wall creature token with defender named Kelp onto the battlefield. */

	if(event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_KELP);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_U(2), 0, NULL, NULL);
}

/* Willow Faerie	|1|G	=>vanilla
 * Creature - Faerie 1/2
 * Flying */

int card_willow_priestess(int player, int card, event_t event){
	/* Willow Priestess	|2|G|G	0x2002D52
	 * Creature - Faerie Druid 2/2
	 * |T: You may put a Faerie permanent card from your hand onto the battlefield.
	 * |2|G: Target |Sgreen creature gains protection from |Sblack until end of turn. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.required_color = get_sleighted_color(player, card, COLOR_TEST_GREEN);

	char msg[100] = "Select a Faerie permanent card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_PERMANENT, msg);
	this_test.subtype = SUBTYPE_FAERIE;
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XG(2, 1), 0, &td1, NULL) ){
			return 1;
		}
		int rvalue = generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		if( player == AI && ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
			rvalue = 0;
		}
		return rvalue;
	}

	if( event == EVENT_ACTIVATE){
		int choice = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XG(2, 1), 0, &td1, NULL) ){
			if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
				if( player == HUMAN || (player == AI && check_battlefield_for_special_card(player, card, player, 0, &this_test)) ){
					int ai_choice = 1;
					if( current_phase > PHASE_DECLARE_BLOCKERS ){
						ai_choice = 0;
					}
					choice = do_dialog(player, player, card, -1, -1, " Give Protection\n Put a Faerie into play\n Cancel", ai_choice);
				}
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		else{
			if( charge_mana_for_activated_ability(player, card, MANACOST_XG(choice == 0 ? 2 : 0, choice == 0 ? 1 : 0)) ){
				if( choice == 0 ){
					if( new_pick_target(&td1, get_sleighted_color_text(player, card, "Select target %s creature.", COLOR_GREEN), 0, 1 | GS_LITERAL_PROMPT) ){
						instance->number_of_targets = 1;
						instance->info_slot = 66+choice;
					}
				}
				else if( choice == 1 ){
						tap_card(player, card);
						instance->info_slot = 66+choice;
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		if( instance->info_slot == 66 && valid_target(&td1)  ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, get_sleighted_protection(instance->parent_controller, instance->parent_card, KEYWORD_PROT_BLACK), 0);
		}
		if( instance->info_slot == 67 && hand_count[player] > 0 ){
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	return 0;
}

int card_winter_sky(int player, int card, event_t event)
{
  /* Winter Sky	|R	0x200d720
   * Sorcery
   * Flip a coin. If you win the flip, ~ deals 1 damage to each creature and each player. If you lose the flip, each player draws a card. */

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (flip_a_coin(player, card))
		new_damage_all(player, card, ANYBODY, 1, NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);
	  else
		APNAP(p, draw_a_card(p));
	  kill_card(player, card, KILL_DESTROY);
	}

  return basic_spell(player, card, event);
}

int card_wizards_school(int player, int card, event_t event)
{
  /* Wizards' School	""	0x200d694
   * Land
   * |T: Add |1 to your mana pool.
   * |1, |T: Add |U to your mana pool.
   * |2, |T: Add |W or |B to your mana pool. */
  return homelands_triple_land(player, card, event, COLOR_TEST_BLUE, COLOR_TEST_WHITE|COLOR_TEST_BLACK);
}
