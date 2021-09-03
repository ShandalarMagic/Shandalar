#include "manalink.h"

// Global functions
int when_you_cast(int player, int card, event_t event){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! is_token(player, card) && ! check_special_flags(player, card, SF_NOT_CAST) ){
			return 1;
		}
	}
	return 0;
}

int annihilator(int player, int card, event_t event, int number)
{
  // Whenever this creature attacks, defending player sacrifices [number] permanents.
  if (declare_attackers_trigger(player, card, event, DAT_STORE_IN_INFO_SLOT, player, card))	// info_slot to avoid conflict with It That Betrays
	impose_sacrifice(player, card, 1-player, number, TYPE_PERMANENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);

  return 0;
}

/* Just the effects of leveling up, not activation.  l2sp/l3sp only work for special abilities that can just be set during EVENT_ABILITIES.  Currently only used
 * for deathtouch, indestructibility, lifelink, and vigilance, all of which are ok. */
static void level_up_effects(int player, int card, event_t event,
							 int l1power, int l1toughness,
							 int l2, int l2power, int l2toughness, keyword_t l2key, sp_keyword_t l2sp,
							 int l3, int l3power, int l3toughness, keyword_t l3key, sp_keyword_t l3sp)
{
  if (event == EVENT_POWER && affect_me(player, card) && l3power > l1power)
	{
	  int counters = count_counters(player, card, COUNTER_LEVEL);
	  if (counters >= l3)
		event_result += l3power - l1power;
	  else if (counters >= l2)
		event_result += l2power - l1power;
	}

  if (event == EVENT_TOUGHNESS && affect_me(player, card) && l3toughness > l1toughness)
	{
	  int counters = count_counters(player, card, COUNTER_LEVEL);
	  if (counters >= l3)
		event_result += l3toughness - l1toughness;
	  else if (counters >= l2)
		event_result += l2toughness - l1toughness;
	}

  if (event == EVENT_ABILITIES && affect_me(player, card) && (l3key || l3sp))
	{
	  int counters = count_counters(player, card, COUNTER_LEVEL);
	  if (counters >= l3)
		{
		  if (l3key & KEYWORD_BASIC_LANDWALK)
			l3key = get_hacked_walk(player, card, l3key);

		  if (l3key & KEYWORD_PROT_COLORED)
			l3key = get_sleighted_protection(player, card, l3key);

		  event_result |= l3key;

		  if (l3sp)
			special_abilities(player, card, event, l3sp, player, card);
		}
	  else if (counters >= l2)
		{
		  if (l2key & KEYWORD_BASIC_LANDWALK)
			l2key = get_hacked_walk(player, card, l2key);

		  if (l2key & KEYWORD_PROT_COLORED)
			l2key = get_sleighted_protection(player, card, l2key);

		  event_result |= l2key;

		  if (l2sp)
			special_abilities(player, card, event, l2sp, player, card);
		}
	}

  if (event == EVENT_SHOULD_AI_PLAY)
	{
	  /* Encourage AI to level up, particularly so that it crosses a level threshold, and preferably level up creatures the more counters they have past a level
	   * threshold.
	   *
	   * Each counter up to l2 is worth (1+number_of_counters) squared.  (+1 so that the first is worth 4, so the AI is willing to tap for it; it counts a
	   * tapped land as worth 1 less than an untapped one.)
	   *
	   * The counter putting it at l2 is worth an extra 24.
	   *
	   * Between l2 and l3, each counter is again worth (1+number_of_counters), but number_of_counters is counted from the l2 threshold, not from 0.
	   *
	   * The counter putting it at l3 is again worth an extra 24; counters past l3 aren't worth anything.
	   *
	   * Three examples:
	   * Enclave Cryptologist (l2 = 1, l3 = 3)
	   * Counters	Total modifier
	   * 0			0
	   * 1 (l2)		28 (2^2 + 24)
	   * 2			32 (l2bonus + 2^2)
	   * 3 (l3)		61 (l2bonus + 3^2 + 24)
	   *
	   * Student of Warfare (l2 = 2, l3 = 7)
	   * 0			0
	   * 1			4 (2^2)
	   * 2 (l2)		33 (3^2 + 24)
	   * 3			37 (l2bonus + 2^2)
	   * 4			42 (l2bonus + 3^2)
	   * 5			49 (l2bonus + 4^2)
	   * 6			58 (l2bonus + 5^2)
	   * 7 (l3)		93 (l2bonus + 6^2 + 24)
	   *
	   * Transcendent Master (l2 = 6, l3 = 12)
	   * 0			0
	   * 1			4
	   * 2			9
	   * 3			16
	   * 4			25
	   * 5			36
	   * 6 (l2)		73 (7^2 + 24)
	   * 7			77 (l2bonus + 2^2)
	   * 8			82 (l2bonus + 3^2)
	   * 9			89 (l2bonus + 4^2)
	   * 10			98 (l2bonus + 5^2)
	   * 11			109 (l2bonus + 6^2)
	   * 12			146 (l2bonus + 7^2 + 24)
	   */

	  int counters = count_counters(player, card, COUNTER_LEVEL);

	  if (!counters)
		return;

	  int mod = 0;

	  if (counters < l2)
		mod += (1+counters) * (1+counters);
	  else
		{
		  mod += (1+l2) * (1+l2) + 24;

		  if (counters > l2)
			{
			  if (counters >= l3)
				{
				  counters = l3;
				  mod += 24;
				}

			  int past_l2_plus_1 = counters - l2 + 1;
			  mod += past_l2_plus_1 * past_l2_plus_1;
			}
		}

	  if (player == AI)
		ai_modifier += mod;
	  else
		ai_modifier -= mod;
	}
}

// Level up, both effects and activation.
static int level_up(int player, int card, event_t event,
					int x, int b, int u, int g, int r, int w,
					int l1power, int l1toughness,
					int l2, int l2power, int l2toughness, keyword_t l2key, sp_keyword_t l2sp,
					int l3, int l3power, int l3toughness, keyword_t l3key, sp_keyword_t l3sp)
{
  level_up_effects(player, card, event,
				   l1power, l1toughness,
				   l2, l2power, l2toughness, l2key, l2sp,
				   l3, l3power, l3toughness, l3key, l3sp);

  if (event == EVENT_CAN_ACTIVATE)
	return CAN_ACTIVATE(player, card, MANACOST6(x,b,u,g,r,w)) && can_sorcery_be_played(player, event);

  if (event == EVENT_ACTIVATE)
	charge_mana_for_activated_ability(player, card, x,b,u,g,r,w);

  if (event == EVENT_RESOLVE_ACTIVATION)
	add_counter(player, card, COUNTER_LEVEL);

  return 0;
}

int totem_armor(int player, int card, event_t event, int pow, int tou, int key, int s_key){

	card_instance_t *instance = get_card_instance(player, card);
	int t_player = instance->damage_target_player;
	int t_card = instance->damage_target_card;

	if( in_play(player, card) && t_player != -1 ){
		if( instance->targets[7].player == 1 ){
			return 0;
		}
		instance->targets[7].player = 1;
		if( in_play(player, card) && is_creature_dead() && ( land_can_be_played & LCBP_REGENERATION) ){
			card_instance_t *enchanted = get_card_instance(t_player, t_card);
			if( enchanted->kill_code == KILL_DESTROY || enchanted->kill_code == KILL_BURY ){
				int is_tap = is_tapped(t_player, t_card);
				regenerate_target_exe(t_player, t_card);
				if( ! is_tap ){
					untap_card_no_event(t_player, t_card);
				}
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
		instance->targets[7].player = 0;

	}

	return generic_aura(player, card, event, player, pow, tou, key, s_key, 0, 0, 0);
}

static int rebound_legacy(int player, int card, event_t event)
{
  if (trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && current_turn == player && count_upkeeps(player) > 0)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  if (event == EVENT_TRIGGER)
		{
		  if (instance->targets[0].player == 1)
			event_result |= RESOLVE_TRIGGER_MANDATORY;
		  else
			{
			  instance->targets[0].player = 3;
			  event_result |= RESOLVE_TRIGGER_AI(player);
			}
		}

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  if (instance->targets[0].player == 1)
			instance->targets[0].player = 0;	// delay trigger to next upkeep
		  else
			{
			  if (check_rfg(player, instance->targets[0].card) && can_legally_play_csvid(player, instance->targets[0].card))
				play_card_in_exile_for_free(player, player, instance->targets[0].card);

			  kill_card(player, card, KILL_REMOVE);
			}
		}

	  if (event == EVENT_END_TRIGGER && instance->targets[0].player == 3)	// trigger declined
		kill_card(player, card, KILL_REMOVE);
	}

  return 0;
}

int rebound(int player, int card){

	if( !not_played_from_hand(player, card) ){
		int id = get_id(player, card);
		int legacy = create_legacy_effect(player, card, &rebound_legacy );
		card_instance_t *rebounded = get_card_instance(player, legacy);
		rebounded->targets[0].card = id;
		rebounded->targets[0].player = -1;
		if( current_phase == PHASE_UPKEEP ){
		   rebounded->targets[0].player = 1;
		}
		kill_card(player, card, KILL_REMOVE);
		return legacy;
	}
	else{
		 kill_card(player, card, KILL_DESTROY);
		 return -1;
	}
}

/************
* Colorless *
************/

int card_all_is_dust(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
		int p = 0;
		for( p = 0; p < 2; p++){
			int count = active_cards_count[p];
			while(count >= 0 ){
				card_data_t* card_d = get_card_data(p, count);
				if( in_play(p, count) && ! is_colorless(p, count) && (card_d->type & TYPE_PERMANENT) ){
					kill_card( p, count, KILL_SACRIFICE );
				}
				count--;
			}
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_artisan_of_kozilek(int player, int card, event_t event){
	annihilator(player, card, event, 2);
	if( when_you_cast(player, card, event) && count_graveyard_by_type(player, TYPE_CREATURE) > 0  ){
		global_tutor(player, player, 2, TUTOR_PLAY, 1, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	return 0;
}

int card_eldrazi_conscription(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 && in_play(instance->targets[0].player, instance->targets[0].card) ){
		annihilator(instance->damage_target_player, instance->damage_target_card, event, 2);
	}
	return generic_aura(player, card, event, player, 10, 10, KEYWORD_TRAMPLE, 0, 0, 0, 0);
}

int card_emrakul_the_aeons_torn(int player, int card, event_t event){

	/* Emrakul, the Aeons Torn	|15
	 * Legendary Creature - Eldrazi 15/15
	 * ~ can't be countered.
	 * When you cast ~, take an extra turn after this one.
	 * Flying, protection from colored spells, annihilator 6
	 * When ~ is put into a graveyard from anywhere, its owner shuffles his or her graveyard into his or her library. */

	// Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

	annihilator(player, card, event, 6);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		state_untargettable(player, card, 1);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		state_untargettable(player, card, 0);
	}

	if( when_you_cast(player, card, event) ){
		alternate_legacy_text(1, player, create_time_walk_legacy(player, card));
	}

	return 0;
}

int card_hand_of_emrakul(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	annihilator(player, card, event, 1);

	if( event == EVENT_MODIFY_COST ){
		if (can_sacrifice_as_cost(player, 4, TYPE_PERMANENT, MATCH, 0, 0, 0, 0, CARD_ID_ELDRAZI_SPAWN, MATCH, -1, 0)){
			instance->targets[1].player = 1;
			null_casting_cost(player, card);
		}
		else{
			instance->targets[1].player = 0;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) && instance->targets[1].player == 1 ){
			int choice = 0;
			if( count_cards_by_id(player, CARD_ID_ELDRAZI_SPAWN) > 3 ){
				if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
					choice = do_dialog(player, player, card, -1, -1, " Sac 4 Eldrazi Spawn\n Play normally\n Cancel", 0);
				}
			}
			if( choice == 0 ){
				if( sacrifice(player, card, player, 0, TYPE_PERMANENT, MATCH, 0, 0, 0, 0, CARD_ID_ELDRAZI_SPAWN, MATCH, -1, 0) ){
					impose_sacrifice(player, card, player, 3, TYPE_PERMANENT, MATCH, 0, 0, 0, 0, CARD_ID_ELDRAZI_SPAWN, MATCH, -1, 0);
				}
				else{
					spell_fizzled = 1;
				}
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, get_id(player, card));
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}

int card_it_that_betrays(int player, int card, event_t event){

	/* It That Betrays	|12
	 * Creature - Eldrazi 11/11
	 * Annihilator 2
	 * Whenever an opponent sacrifices a nontoken permanent, put that card onto the battlefield under your control. */

	card_instance_t *instance = get_card_instance(player, card);

	annihilator(player, card, event, 2);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		card_instance_t* affected = in_play(affected_card_controller, affected_card);
		if (affected && affected_card_controller == 1-player &&
			is_what(affected_card_controller, affected_card, TYPE_PERMANENT) && !is_token(affected_card_controller, affected_card) &&
			affected->kill_code == KILL_SACRIFICE && !check_special_flags(affected_card_controller, affected_card, SF_KILL_STATE_BASED_ACTION)
		   ){
			if( instance->targets[11].player < 0 ){
				instance->targets[11].player = 0;
			}
			int pos = instance->targets[11].player;
			if( pos < 10 ){
				instance->targets[pos].player = get_owner(affected_card_controller, affected_card);
				instance->targets[pos].card = get_id(affected_card_controller, affected_card);
				instance->targets[11].player++;
			}
		}
	}

	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && player == reason_for_trigger_controller && affect_me(player, card) &&
		 instance->targets[11].player > 0
	  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int i;
					for(i=0; i<instance->targets[11].player; i++){
						seek_grave_for_id_to_reanimate(player, card, instance->targets[i].player, instance->targets[i].card, REANIMATE_DEFAULT);
					}
					instance->targets[11].player = 0;
			}
	}

	return 0;
}

int card_kozilek_butcher_of_truth(int player, int card, event_t event){

	/* Kozilek, Butcher of Truth	|10
	 * Legendary Creature - Eldrazi 12/12
	 * When you cast ~, draw four cards.
	 * Annihilator 4
	 * When ~ is put into a graveyard from anywhere, its owner shuffles his or her graveyard into his or her library. */

	// Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

	check_legend_rule(player, card, event);
	annihilator(player, card, event, 4);
	if( when_you_cast(player, card, event) ){
		draw_cards(player, 4 );
	}
	return 0;
}

static const char* targets_a_permanent_you_control(int who_chooses, int player, int card)
{
  int i;
  card_instance_t* instance = get_card_instance(player, card);

  for (i = 0; i < instance->number_of_targets; ++i)
	if (instance->targets[0].player == who_chooses
		&& is_what(instance->targets[0].player, instance->targets[0].card, TYPE_PERMANENT))
	  return NULL;

  return "targets permanent you control";
}
static int targets_a_creature_you_control_with_power_ge_7(int who_chooses, int player, int card)
{
  int i;
  card_instance_t* instance = get_card_instance(player, card);

  for (i = 0; i < instance->number_of_targets; ++i)
	if (instance->targets[0].player == who_chooses
		&& is_what(instance->targets[0].player, instance->targets[0].card, TYPE_PERMANENT))
	  return 1;

  return 0;
}
int card_not_of_this_world(int player, int card, event_t event)
{
  target_definition_t td_effect;
  counter_activated_target_definition(player, card, &td_effect, 0);
  td_effect.extra = (int32_t)targets_a_permanent_you_control;
  td_effect.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

  target_definition_t td_spell;
  counterspell_target_definition(player, card, &td_spell, 0);
  td_spell.extra = (int32_t)targets_a_permanent_you_control;
  td_spell.special |= TARGET_SPECIAL_EXTRA_FUNCTION;

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_MODIFY_COST)
	{
	  if (instance->info_slot & 4)
		null_casting_cost(player, card);
	}
  else if (event == EVENT_CAN_CAST)
	{
	  if (counterspell(player, card, event, &td_spell, 0))
		{
		  instance->info_slot = 1;
		  if (targets_a_creature_you_control_with_power_ge_7(player, card_on_stack_controller, card_on_stack))
			instance->info_slot |= 4;
		  return 99;
		}
	  if (can_counter_activated_ability(player, card, event, &td_effect))
		{
		  instance->info_slot = 2;
		  if (targets_a_creature_you_control_with_power_ge_7(player, stack_cards[stack_size - 1].player, stack_cards[stack_size - 1].card))
			instance->info_slot |= 4;
		  return 99;
		}
	  instance->info_slot = 0;
	  return 0;
	}
  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  instance->number_of_targets = 0;
	  if (instance->info_slot & 1)
		counterspell(player, card, event, &td_spell, 0);
	  else if (instance->info_slot & 2)
		cast_counter_activated_ability(player, card, 0);
	}
  else if (event == EVENT_RESOLVE_SPELL)
	{
		if (instance->info_slot == 1){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			counterspell(player, card, event, &td_spell, 0);	// destroys card
		}
	  else if (instance->info_slot == 2 && resolve_counter_activated_ability(player, card, &td_effect, 0))
		kill_card(player, card, KILL_DESTROY);
	}
  else
	return counterspell(player, card, event, &td_spell, 0);

  return 0;
}

int card_pathrazer_of_ulamog(int player, int card, event_t event){
	annihilator(player, card, event, 3);
	minimum_blockers(player, card, event, 3);
	return 0;
}

int card_skittering_invasion(int player, int card, event_t event){
	/* Skittering Invasion	|7
	 * Tribal Sorcery - Eldrazi
	 * Put five 0/1 colorless Eldrazi Spawn creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your mana pool." */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SPAWN, 5);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_spawnsire_of_ulamog(int player, int card, event_t event){

	/* Spawnsire of Ulamog	|10
	 * Creature - Eldrazi 7/11
	 * Annihilator 1
	 * |4: Put two 0/1 colorless Eldrazi Spawn creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your mana pool."
	 * |2|0: Cast any number of Eldrazi cards you own from outside the game without paying their mana costs. */

	card_instance_t *instance = get_card_instance(player, card);

	annihilator(player, card, event, 1);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) ){
		return 1;
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) ){
			if( has_mana_for_activated_ability(player, card, 20, 0, 0, 0, 0, 0) ){
				choice = do_dialog(player, player, card, -1, -1, " Generate 2 Eldrazi Spawn\n It's Eldrazi Time !\n Cancel", 1);
			}
		}
		int cless = 4;
		if( choice == 1 ){
			cless+=16;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
		}
		else{
			charge_mana_for_activated_ability(player, card, cless, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->targets[1].player = 66+choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->targets[1].player == 66 ){
			generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SPAWN, 2);
		}
		if( instance->targets[1].player == 67 ){
#define RULES_ENGINE_AND_PLAY(player, csvid)	do {																							\
													update_rules_engine(check_card_for_rules_engine(get_internal_card_id_from_csv_id(csvid)));	\
													add_csvid_to_rfg(player, csvid);															\
													play_card_in_exile_for_free(player, player, csvid);											\
												} while (0)
			if( player == AI ){
				RULES_ENGINE_AND_PLAY(player, CARD_ID_ALL_IS_DUST);
				RULES_ENGINE_AND_PLAY(player, CARD_ID_KOZILEK_BUTCHER_OF_TRUTH);
				RULES_ENGINE_AND_PLAY(player, CARD_ID_ULAMOG_THE_INFINITE_GYRE);
				RULES_ENGINE_AND_PLAY(player, CARD_ID_EMRAKUL_THE_AEONS_TORN);
			}
			else{
				int result = card_from_list(player, 2, TYPE_ANY, 0, SUBTYPE_ELDRAZI, 0, COLOR_COLORLESS, 0, 0, 0, -1, 0);
				while( result != -1 ){
						RULES_ENGINE_AND_PLAY(player, result);
						result = card_from_list(player, 2, TYPE_ANY, 0, SUBTYPE_ELDRAZI, 0, COLOR_COLORLESS, 0, 0, 0, -1, 0);
				}
			}
#undef RULES_ENGINE_AND_PLAY
		}
	}

	return 0;
}

int card_ulamog_the_infinite_gyre(int player, int card, event_t event){

	/* Ulamog, the Infinite Gyre	|11
	 * Legendary Creature - Eldrazi 10/10
	 * When you cast ~, destroy target permanent.
	 * Annihilator 4
	 * ~ is indestructible.
	 * When ~ is put into a graveyard from anywhere, its owner shuffles his or her graveyard into his or her library. */

	// Last ability handled in special_mill() and raw_put_iid_on_top_of_graveyard_with_triggers().

	check_legend_rule(player, card, event);
	annihilator(player, card, event, 4);
	if( when_you_cast(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.allow_cancel = 0;
		if (can_target(&td) && pick_target(&td, "TARGET_PERMANENT")){
			card_instance_t* instance = get_card_instance(player, card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}
	indestructible(player, card, event);
	return 0;
}

int card_ulamogs_crusher(int player, int card, event_t event){
	annihilator(player, card, event, 2);
	return attack_if_able(player, card, event);
}

/********
* White *
********/

/* Affa Guard Hound	|2|W
 * Creature - Hound 2/2
 * Flash
 * When ~ enters the battlefield, target creature gets +0/+3 until end of turn. */

int card_caravan_escort(int player, int card, event_t event)
{
  /* Caravan Escort	|W
   * Creature - Human Knight 1/1
   * Level up |2
   * LEVEL 1-4	2/2
   * LEVEL 5+	5/5
   * First strike */

  return level_up(player, card, event, MANACOST_X(2),
				     1,1,
				  1, 2,2, 0,0,
				  5, 5,5, KEYWORD_FIRST_STRIKE,0);
}

int card_dawnglare_invoker(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, instance->targets[0].player, &this_test, ACT_TAP);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 8, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_deathless_angel(int player, int card, event_t event)
{
  /* Deathless Angel	|4|W|W
   * Creature - Angel 5/7
   * Flying
   * |W|W: Target creature gains indestructible until end of turn. */

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, 0,SP_KEYWORD_INDESTRUCTIBLE);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_W(2), 0, &td, "TARGET_CREATURE");
}

/* Demystify	|W => 0x408b70 (Onslaught)
 * Instant
 * Destroy target enchantment. */

/* Eland Umbra	|1|W
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature gets +0/+4.
 * Totem armor */

int card_emerge_unschated(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int kw = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, kw, 0);
			rebound(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int gideon_jura_force_attack(int player, int card, event_t event){
	// This effect's address is referenced in planeswalker.c to autochoose the attacked planewalker

	event_flags |= EA_FORCE_ATTACK;

	if (event == EVENT_MUST_ATTACK && current_turn == 1-player && !forbid_attack){
		card_instance_t* inst = get_card_instance(player, card);
		if (in_play(inst->damage_source_player, inst->damage_source_card)
			&& is_planeswalker(inst->damage_source_player, inst->damage_source_card)
			&& inst->damage_source_player != current_turn
		   ){
			all_must_attack_if_able(current_turn, event, -1);
		}
	}
	if( current_turn == 1-player && event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_gideon_jura(int player, int card, event_t event){

	/* Gideon Jura	|3|W|W
	 * Planeswalker - Gideon (6)
	 * +2: During target opponent's next turn, creatures that player controls attack ~ if able.
	 * -2: Destroy target tapped creature.
	 * 0: Until end of turn, ~ becomes a 6/6 Human Soldier creature that's still a planeswalker. Prevent all damage that would be dealt to him this turn. */

	card_instance_t *instance = get_card_instance(player, card);

	double_faced_card(player, card, event);

	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allow_cancel = 0;
		td.required_state = TARGET_STATE_TAPPED;


		enum {
			CHOICE_FORCE_ATTACK = 1,
			CHOICE_ANIMATE = 2,
			CHOICE_KILL = 3
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Make creatures attack", 1, count_counters(player, card, COUNTER_LOYALTY) < 6 ? 20 : 5, 2,
						"Kill tapped creature", can_target(&td), 10, -2,
						"Become creature", 1, count_counters(player, card, COUNTER_LOYALTY) >= 6 ? 15 : 5, 0);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;	// else fall through to planeswalker()
		}
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  switch (choice)
			{
			  case CHOICE_FORCE_ATTACK:
					break;

			  case CHOICE_ANIMATE:
					break;

			  case CHOICE_KILL:
					pick_target(&td, "TARGET_CREATURE");
					break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		switch (choice)
		  {
			case CHOICE_FORCE_ATTACK:
				;int p = instance->parent_controller, c = instance->parent_card;
				if (in_play(p, c)){
					create_legacy_effect(p, c, &gideon_jura_force_attack);
				}
				break;

			case CHOICE_ANIMATE:
				true_transform(player, instance->parent_card);
				break;

			case CHOICE_KILL:
				if (valid_target(&td)){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				}
				break;
		  }
	}

	return planeswalker(player, card, event, 6);
}

int card_gideon_jura_animated(int player, int card, event_t event){
	// prevent any damage that would be dealt to me
	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player && damage->info_slot > 0 ){
				damage->info_slot = 0;
			}
		}
	}

	// turn back into gideon EOT
	if( eot_trigger(player, card, event ) ){
		true_transform(player, card);
	}

	return planeswalker(player, card, event, 6);
}

/* Glory Seeker	|1|W => Vanilla
 * Creature - Human Soldier 2/2 */

/* Guard Duty	|W
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature has defender. */

/* Harmless Assault	|2|W|W
 * Instant
 * Prevent all combat damage that would be dealt this turn by attacking creatures. */

int card_hedron_field_purists(int player, int card, event_t event)
{
  /* Hedron-Field Purists	|2|W
   * Creature - Human Cleric 0/3
   * Level up |2|W
   * LEVEL 1-4	1/4
   * If a source would deal damage to you or a creature you control, prevent 1 of that damage.
   * LEVEL 5+	2/5
   * If a source would deal damage to you or a creature you control, prevent 2 of that damage. */

  card_instance_t* damage = damage_being_prevented(event);
  if (damage
	  && damage->damage_target_player == player && !damage_is_to_planeswalker(damage))
	{
	  int counters = count_counters(player, card, COUNTER_LEVEL);
	  if (counters >= 5)
		damage->info_slot = MAX(0, damage->info_slot - 2);
	  else if (counters >= 1)
		damage->info_slot--;
	}

  return level_up(player, card, event, MANACOST_XW(2,1),
				     0,3,
				  1, 1,4, 0,0,
				  5, 2,5, 0,0);
}

int card_hyena_umbra(int player, int card, event_t event){

	return totem_armor(player, card, event, 1, 1, KEYWORD_FIRST_STRIKE, 0);
}

int card_ikiral_outrider(int player, int card, event_t event)
{
  /* Ikiral Outrider	|1|W
   * Creature - Human Soldier 1/2
   * Level up |4
   * LEVEL 1-3	2/6
   * Vigilance
   * LEVEL 4+	3/10
   * Vigilance */

  // Need to value counters slightly more than usual, so the AI will be willing to pay |4 for the second one.
  if (event == EVENT_SHOULD_AI_PLAY && player == AI)
	{
	  int counters = count_counters(player, card, COUNTER_LEVEL);
	  counters = MIN(4, counters);
	  ai_modifier += counters;
	}

  return level_up(player, card, event, MANACOST_X(4),
				     1,2,
				  1, 2,6,  0,SP_KEYWORD_VIGILANCE,
				  4, 3,10, 0,SP_KEYWORD_VIGILANCE);
}

int card_kabira_vindicator(int player, int card, event_t event)
{
  /* Kabira Vindicator	|3|W
   * Creature - Human Knight 2/4
   * Level up |2|W
   * LEVEL 2-4	3/6
   * Other creatures you control get +1/+1.
   * LEVEL 5+	4/8
   * Other creatures you control get +2/+2. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player)
	{
	  int counters = count_counters(player, card, COUNTER_LEVEL);
	  if (counters >= 5)
		boost_subtype(player, card, event, -1, 2,2, 0,0, BCT_CONTROLLER_ONLY);
	  else if (counters >= 2)
		boost_subtype(player, card, event, -1, 1,1, 0,0, BCT_CONTROLLER_ONLY);
	}

  return level_up(player, card, event, MANACOST_XW(2,1),
				     2,4,
				  2, 3,6, 0,0,
				  5, 4,8, 0,0);
}

int card_knight_of_cliffhaven(int player, int card, event_t event)
{
  /* Knight of Cliffhaven	|1|W
   * Creature - Kor Knight 2/2
   * Level up |3
   * LEVEL 1-3	2/3
   * Flying
   * LEVEL 4+	4/4
   * Flying, vigilance */

  return level_up(player, card, event, MANACOST_X(3),
				     2,2,
				  1, 2,3, KEYWORD_FLYING,0,
				  4, 4,4, KEYWORD_FLYING,SP_KEYWORD_VIGILANCE);
}

/* Kor Line-Slinger	|1|W
 * Creature - Kor Scout 0/1
 * |T: Tap target creature with power 3 or less. */

int card_kor_spiritdancer(int player, int card, event_t event)
{
  /* Kor Spiritdancer	|1|W
   * Creature - Kor Wizard 0/2
   * ~ gets +2/+2 for each Aura attached to it.
   * Whenever you cast an Aura spell, you may draw a card. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card))
	event_result += count_auras_enchanting_me(player, card) * 2;

  if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_ENCHANTMENT,MATCH, SUBTYPE_AURA,MATCH, 0,0, 0,0, -1,0))
	draw_a_card(player);

  return 0;
}

int card_lightmine_field(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( current_phase == PHASE_BEFORE_BLOCKING && instance->targets[1].player != 66 ){
		int amount = count_attackers(current_turn);
		if( amount > 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.state = STATE_ATTACKING;
			new_damage_all(player, card, current_turn, amount, 0, &this_test);
		}
		instance->targets[1].player = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return global_enchantment(player, card, event);
}

int card_linvala_keeper_of_silence(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, 1-player, &this_test, ACT_DISABLE_ALL_ACTIVATED_ABILITIES);
	}

	if( in_play(player, card) && event == EVENT_CAST_SPELL && affected_card_controller != player &&
		is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  ){
		disable_all_activated_abilities(affected_card_controller, affected_card, 1);
	}

	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		new_manipulate_all(player, card, 1-player, &this_test, ACT_ENABLE_ALL_ACTIVATED_ABILITIES);
	}

	return 0;
}

int card_lone_missionary(int player, int card, event_t event){
	// Also code for Teroh's Faithful
	return cip_lifegain(player, card, event, 4);
}

/* Luminous Wake	|2|W
 * Enchantment - Aura
 * Enchant creature
 * Whenever enchanted creature attacks or blocks, you gain 4 life. */

/* Makindi Griffin	|3|W => Vanilla
 * Creature - Griffin 2/4
 * Flying */

int card_mammoth_umbra(int player, int card, event_t event){

	return totem_armor(player, card, event, 3, 3, 0, SP_KEYWORD_VIGILANCE);
}

int card_near_death_experience(int player, int card, event_t event){

	/* Near-Death Experience	|2|W|W|W
	 * Enchantment
	 * At the beginning of your upkeep, if you have exactly 1 life, you win the game. */

	if( life[player] == 1 &&
		(event == EVENT_SHOULD_AI_PLAY ||
		 (current_turn == player && upkeep_trigger(player, card, event)))
	  ){
		lose_the_game(1-player);
	}

	return global_enchantment(player, card, event);
}

int card_nomads_assembly(int player, int card, event_t event){
	/* Nomads' Assembly	|4|W|W
	 * Sorcery
	 * Put a 1/1 |Swhite Kor Soldier creature token onto the battlefield for each creature you control.
	 * Rebound */

	if( event == EVENT_CAST_SPELL && affect_me(player, card) && player == AI ){
		if( count_subtype(player, TYPE_CREATURE, -1) < 2 ){
			ai_modifier-=25;
		}
	}

	if(event == EVENT_RESOLVE_SPELL){
		int total = count_subtype(player, TYPE_CREATURE, -1);
		generate_tokens_by_id(player, card, CARD_ID_KOR_SOLDIER, total);
		rebound(player, card);
	}

	return basic_spell(player, card, event);
}

int card_oust(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			if( valid_target(&td)  ){
			   int *deck = deck_ptr[instance->targets[0].player];
			   put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
			   int transit = deck[1];
			   deck[1] = deck[0];
			   deck[0] = transit;
			   gain_life(instance->targets[0].player, 3);
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_puncturing_light(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 3 | TARGET_PT_LESSER_OR_EQUAL;
	td.required_state = TARGET_STATE_IN_COMBAT;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

/* Repel the Darkness	|2|W
 * Instant
 * Tap up to two target creatures.
 * Draw a card. */

/* Smite	|W (Stronghold)
 * Instant
 * Destroy target blocked creature. */

int card_souls_attendant(int player, int card, event_t event)
{
  // Whenever another creature enters the battlefield, you may gain 1 life.
  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, ANYBODY, RESOLVE_TRIGGER_AI(player), &test))
		gain_life(player, 1);
	}

  return 0;
}

/* Soulbound Guardians	|4|W (vanilla)
 * Creature - Kor Spirit 4/5
 * Defender, flying */

/* Stalwart Shield-Bearers	|1|W
 * Creature - Human Soldier 0/3
 * Defender
 * Other creatures you control with defender get +0/+2. */

int card_student_of_warfare(int player, int card, event_t event)
{
  /* Student of Warfare	|W
   * Creature - Human Knight 1/1
   * Level up |W
   * LEVEL 2-6	3/3
   * First strike
   * LEVEL 7+	4/4
   * Double strike */

  return level_up(player, card, event, MANACOST_W(1),
				     1,1,
				  2, 3,3, KEYWORD_FIRST_STRIKE,0,
				  7, 4,4, KEYWORD_DOUBLE_STRIKE,0);
}

int card_survival_cache(int player, int card, event_t event){//UNUSEDCARD

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 2);
		if( life[player] > life[1-player] ){
			draw_cards(player, 1);
		}
		rebound(player, card);
	}

	return 0;
}

int card_time_of_heroes(int player, int card, event_t event)
{
  /* Time of Heroes	|1|W
   * Enchantment
   * Each creature you control with a level counter on it gets +2/+2. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
	  && count_counters(affected_card_controller, affected_card, COUNTER_LEVEL) > 0)
	event_result += 2;

  return global_enchantment(player, card, event);
}

int card_totem_guide_hartebeest(int player, int card, event_t event)
{
  /* Totem-Guide Hartebeest	|4|W
   * Creature - Antelope 2/5
   * When ~ enters the battlefield, you may search your library for an Aura card, reveal it, put it into your hand, then shuffle your library. */

  if (comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ENCHANTMENT, "Select an Aura card.");
	  test.subtype = SUBTYPE_AURA;
	  new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
	}

  return 0;
}

int card_transcendent_master(int player, int card, event_t event)
{
  /* Transcendent Master	|1|W|W
   * Creature - Human Cleric Avatar 3/3
   * Level up |1
   * LEVEL 6-11	6/6
   * Lifelink
   * LEVEL 12+	9/9
   * Lifelink
   * ~ is indestructible. */

  return level_up(player, card, event, MANACOST_X(1),
				     3,3,
				  6, 6,6, 0,SP_KEYWORD_LIFELINK,
				  12,9,9, 0,SP_KEYWORD_LIFELINK|SP_KEYWORD_INDESTRUCTIBLE);
}

/* Umbra Mystic	|2|W
 * Creature - Human Wizard 2/2
 * Auras attached to permanents you control have totem armor. */

/* Wall of Omens	|1|W => ravnica.c:Carven Caryatid
 * Creature - Wall 0/4
 * Defender
 * When ~ enters the battlefield, draw a card. */

/*******
* Blue *
*******/

/* Aura Finesse	|U
 * Instant
 * Attach target Aura you control to target creature.
 * Draw a card. */

/* Cast Through Time	|4|U|U|U
 * Enchantment
 * Instant and sorcery spells you control have rebound. */

int card_champions_drake(int player, int card, event_t event)
{
  /* Champion's Drake	|1|U
   * Creature - Drake 1/1
   * Flying
   * ~ gets +3/+3 as long as you control a creature with three or more level counters on it. */

  int c;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card))
	for (c = 0; c < active_cards_count[player]; ++c)
	  if (in_play(player, c) && is_what(player, c, TYPE_CREATURE) && count_counters(player, c, COUNTER_LEVEL) >= 3)
		{
		  event_result += 3;
		  break;
		}

  return 0;
}

int card_coralhelm_commander(int player, int card, event_t event)
{
  /* Coralhelm Commander	|U|U
   * Creature - Merfolk Soldier 2/2
   * Level up |1
   * LEVEL 2-3	3/3
   * Flying
   * LEVEL 4+	4/4
   * Flying
   * Other Merfolk creatures you control get +1/+1. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player && count_counters(player, card, COUNTER_LEVEL) >= 4)
	boost_subtype(player, card, event, SUBTYPE_MERFOLK, 1,1, 0,0, BCT_CONTROLLER_ONLY);

  return level_up(player, card, event, MANACOST_X(1),
				     2,2,
				  2, 3,3, KEYWORD_FLYING,0,
				  4, 4,4, KEYWORD_FLYING,0);
}

/* Crab Umbra	|U
 * Enchantment - Aura
 * Enchant creature
 * |2|U: Untap enchanted creature.
 * Totem armor */

int card_deprive(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);


	if( event == EVENT_CAN_CAST && card_on_stack_controller != -1 && card_on_stack != -1){
		if( ! check_state(card_on_stack_controller, card_on_stack, STATE_CANNOT_TARGET) ){
			return 0x63;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_LAND") ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_distortion_strike(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									1, 0, 0, SP_KEYWORD_UNBLOCKABLE);
			rebound(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_domestication(int player, int card, event_t event){

	/* Domestication	|2|U|U
	 * Enchantment - Aura
	 * Enchant creature
	 * You control enchanted creature.
	 * At the beginning of your end step, if enchanted creature's power is 4 or greater, sacrifice ~. */

	if( in_play(player, card) && current_turn == player && eot_trigger(player, card, event) ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->targets[0].player > -1 && get_power(instance->targets[0].player, instance->targets[0].card) > 3 ){
				kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return card_control_magic(player, card, event);
}

int card_dormant_gomazoa(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	does_not_untap(player, card, event);

	if (is_tapped(player, card) && any_becomes_target(player, card, event, player, 1, TYPE_NONE, -1, TYPE_NONEFFECT, ANYBODY, RESOLVE_TRIGGER_DUH)){
		untap_card(player, card);
	}

	return 0;
}

int card_drake_umbra(int player, int card, event_t event){

	return totem_armor(player, card, event, 3, 3, KEYWORD_FLYING, 0);
}

int card_echo_mage(int player, int card, event_t event){

	/* Echo Mage	|1|U|U
	 * Creature - Human Wizard 2/3
	 * Level up |1|U
	 * LEVEL 2-3	2/4
	 * |U|U, |T: Copy target instant or sorcery spell. You may choose new targets for the copy.
	 * LEVEL 4+	2/5
	 * |U|U, |T: Copy target instant or sorcery spell twice. You may choose new targets for the copies. */

	level_up_effects(player, card, event,
					    2,3,
					 2, 2,4, 0,0,
					 4, 2,5, 0,0);

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) && count_counters(player, card, COUNTER_LEVEL) >= 2 ){
			int result = activate_twincast(player, card, event, NULL, NULL);
			if( result && has_mana_for_activated_ability(player, card, MANACOST_U(2)) ){
				instance->info_slot = 66;
				return result;
			}
		}
		instance->info_slot = 0;
		if( can_sorcery_be_played(player, event) && has_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( instance->info_slot == 66 ){
			activate_twincast(player, card, event, NULL, NULL);
			if (spell_fizzled != 1 && charge_mana_for_activated_ability(player, card, MANACOST_U(2))){
				tap_card(player, card);
			}
		}
		else{
			charge_mana_for_activated_ability(player, card, 1, 0, 1, 0, 0, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			activate_twincast(player, card, event, NULL, NULL);
			if( count_counters_no_parent(player, card, COUNTER_LEVEL) >= 4 ){
				activate_twincast(player, card, event, NULL, NULL);
			}
		}
		else{
			add_counter(instance->parent_controller, instance->parent_card, COUNTER_LEVEL);
		}
	}

	return 0;
}

/* Eel Umbra	|1|U
 * Enchantment - Aura
 * Flash
 * Enchant creature
 * Enchanted creature gets +1/+1.
 * Totem armor */

int card_enclave_cryptologist(int player, int card, event_t event){

	/* Enclave Cryptologist	|U
	 * Creature - Merfolk Wizard 0/1
	 * Level up |1|U
	 * LEVEL 1-2	0/1
	 * |T: Draw a card, then discard a card.
	 * LEVEL 3+	0/1
	 * |T: Draw a card. */

	level_up_effects(player, card, event,
					    0,1,
					 1, 0,1, 0,0,
					 3, 0,1, 0,0);

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	int level = count_counters_no_parent(player, card, COUNTER_LEVEL);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_XU(1, 1), 0, NULL, NULL) && can_sorcery_be_played(player, event) ){
			return 1;
		}
		if( level > 0 && generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL) ){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		int can_use_nonlevel_ability1 = level > 0 && level < 3 && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST0, 0, NULL, NULL);
		int can_use_nonlevel_ability2 = level >= 3 && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);

		int choice = DIALOG(player, card, event,DLG_RANDOM, DLG_NO_STORAGE,
						"Level Up", has_mana_for_activated_ability(player, card, MANACOST_XU(1, 1)) && can_sorcery_be_played(player, event), 15-(level*5),
						"Draw & Discard", can_use_nonlevel_ability1, 5,
						"Draw 1 card", can_use_nonlevel_ability2, 15);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_for_activated_ability(player, card, MANACOST_XU(choice == 1, choice == 1)) ){
			if( choice > 1 ){
				tap_card(player, card);
			}
			instance->info_slot = 65+choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			add_counter(instance->parent_controller, instance->parent_card, COUNTER_LEVEL);
		}
		if( instance->info_slot > 66){
			draw_cards(player, 1);
			if( instance->info_slot == 67){
				discard(player, 0, player);
			}
		}
	}

	return 0;
}

/* Fleeting Distraction	|U => avacyn_restored.c
 * Instant
 * Target creature gets -1/-0 until end of turn.
 * Draw a card. */

/* Frostwind Invoker	|4|U
 * Creature - Merfolk Wizard 3/3
 * Flying
 * |8: Creatures you control gain flying until end of turn. */

int card_gravitational_shift(int player, int card, event_t event){

	if( event == EVENT_POWER && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		if( check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING) ){
			event_result+=2;
		}
		else{
			event_result-=2;
		}
	}

	return global_enchantment(player, card, event);
}

int card_guard_gomazoa(int player, int card, event_t event){

	card_instance_t* damage = combat_damage_being_prevented(event);
	if( damage && damage->damage_target_card == card && damage->damage_target_player == player ){
		damage->info_slot = 0;
	}

	return 0;
}

int card_hada_spy_patrol(int player, int card, event_t event)
{
  /* Hada Spy Patrol	|1|U
   * Creature - Human Rogue 1/1
   * Level up |2|U
   * LEVEL 1-2	2/2
   * ~ is unblockable.
   * LEVEL 3+	3/3
   * Shroud
   * ~ is unblockable. */

  return level_up(player, card, event, MANACOST_XU(2,1),
				     1,1,
				  1, 2,2, 0,             SP_KEYWORD_UNBLOCKABLE,
				  3, 3,3, KEYWORD_SHROUD,SP_KEYWORD_UNBLOCKABLE);
}

int card_halimar_wavewatch(int player, int card, event_t event)
{
  /* Halimar Wavewatch	|1|U
   * Creature - Merfolk Soldier 0/3
   * Level up |2
   * LEVEL 1-4	0/6
   * LEVEL 5+	6/6
   * |H2Islandwalk */

  return level_up(player, card, event, MANACOST_X(2),
				     0,3,
				  1, 0,6, 0,0,
				  5, 6,6, KEYWORD_ISLANDWALK,0);
}

/* Jwari Scuttler	|2|U => Vanilla
 * Creature - Crab 2/3 */

int card_lay_bare(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && card_on_stack_controller != -1 && card_on_stack != -1){
		if( ! check_state(card_on_stack_controller, card_on_stack, STATE_CANNOT_TARGET) ){
			return 0x63;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( spell_fizzled != 1 ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
		reveal_target_player_hand(instance->targets[0].player);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_lighthouse_chronologist(int player, int card, event_t event)
{
  /* Lighthouse Chronologist	|1|U
   * Creature - Human Wizard 1/3
   * Level up |U
   * LEVEL 4-6	2/4
   * LEVEL 7+	3/5
   * At the beginning of each end step, if it's not your turn, take an extra turn after this one. */

  if (trigger_condition == TRIGGER_EOT && current_turn != player && count_counters(player, card, COUNTER_LEVEL) >= 7 && eot_trigger(player, card, event))
	time_walk_effect(player, card);

  return level_up(player, card, event, MANACOST_U(1),
				     1,3,
				  4, 2,4, 0,0,
				  7, 3,5, 0,0);
}

/* Merfolk Observer	|1|U
 * Creature - Merfolk Rogue 2/1
 * When ~ enters the battlefield, look at the top card of target player's library. */

/* Merfolk Skyscout	|2|U|U
 * Creature - Merfolk Scout 2/3
 * Flying
 * Whenever ~ attacks or blocks, untap target permanent. */

/* Mnemonic Wall	|4|U => m13.c:Archaeomancer
 * Creature - Wall 0/4
 * Defender
 * When ~ enters the battlefield, you may return target instant or sorcery card from your graveyard to your hand. */

int card_narcolepsy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->damage_target_player > -1 ){
		if( ! is_tapped(instance->damage_target_player, instance->damage_target_card) && upkeep_trigger(player, card, event) ){
			tap_card( instance->damage_target_player, instance->damage_target_card );
		}
	}
	return generic_aura(player, card, event, 1-player, 0, 0, 0, 0, 0, 0, 0);
}

/* Phantasmal Abomination	|1|U|U (skulking ghost)
 * Creature - Illusion 5/5
 * Defender
 * When ~ becomes the target of a spell or ability, sacrifice it. */

/* Reality Spasm	|X|U|U
 * Instant
 * Choose one - Tap X target permanents; or untap X target permanents. */

int card_recurring_insight(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
		return would_valid_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = 1-player;
		instance->targets[0].card = -1;
		instance->number_of_targets = 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(player, hand_count[instance->targets[0].player]);
			rebound(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

/* Regress	|2|U => 0x42c6d0:Boomerang
 * Instant
 * Return target permanent to its owner's hand. */

int card_renegade_doppelganger(int player, int card, event_t event)
{
  /* Renegade Doppelganger	|1|U
   * Creature - Shapeshifter 0/1
   * Whenever another creature enters the battlefield under your control, you may have ~ become a copy of that creature until end of turn. */

  if (trigger_condition == TRIGGER_COMES_INTO_PLAY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.not_me = 1;

	  if (new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &test))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  shapeshift_target(player, card, player, card, instance->targets[1].player, instance->targets[1].card, SHAPESHIFT_UNTIL_EOT);
		}
	}

  return 0;

}

int card_sea_gate_oracle(int player, int card, event_t event)
{
  /* Sea Gate Oracle	|2|U
   * Creature - Human Wizard 1/3
   * When ~ enters the battlefield, look at the top two cards of your library. Put one of them into your hand and the other on the bottom of your library. */

  if (comes_into_play(player, card, event) && deck_ptr[player][0] != -1)
	reveal_top_cards_of_library_and_choose_type(player, card, player, 2, 1, TUTOR_HAND, 0, TUTOR_BOTTOM_OF_DECK, 0, TYPE_ANY);

  return 0;
}

int card_see_beyond(int player, int card, event_t event){

	/* See Beyond	|1|U
	 * Sorcery
	 * Draw two cards, then shuffle a card from your hand into your library. */

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL && affect_me(player, card) ){
			draw_cards(player, 2);

			target_definition_t td;
			base_target_definition(player, card, &td, 0);
			td.allow_cancel = 0;
			td.zone = TARGET_ZONE_HAND;
			td.allowed_controller = player;
			td.preferred_controller = player;

			if (can_target(&td) && pick_target(&td, "TARGET_CARD")){
				card_instance_t* instance = get_card_instance( player, card);
				shuffle_into_library(instance->targets[0].player, instance->targets[0].card);
			}

			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

/* Shared Discovery	|U
 * Sorcery
 * As an additional cost to cast ~, tap four untapped creatures you control.
 * Draw three cards. */

int card_skywatcher_adept(int player, int card, event_t event)
{
  /* Skywatcher Adept	|U
   * Creature - Merfolk Wizard 1/1
   * Level up |3
   * LEVEL 1-2	2/2
   * Flying
   * LEVEL 3+	4/2
   * Flying */

  return level_up(player, card, event, MANACOST_X(3),
				     1,1,
				  1, 2,2, KEYWORD_FLYING,0,
				  3, 4,2, KEYWORD_FLYING,0);
}

int card_sphinx_of_magosi(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		has_mana_for_activated_ability(player, card, 2, 0, 1, 0, 0, 0)
	  ){
		return 1;
	}

	else if(event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 2, 0, 1, 0, 0, 0);
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			draw_cards(player, 1);
			add_1_1_counter(player, instance->parent_card);
	}

	return 0;
}

int card_surrakar_spellblade(int player, int card, event_t event){

	/* Surrakar Spellblade	|1|U|U
	 * Creature - Surrakar 2/1
	 * Whenever you cast an instant or sorcery spell, you may put a charge counter on ~.
	 * Whenever ~ deals combat damage to a player, you may draw X cards, where X is the number of charge counters on it. */

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_SPELL, F1_NO_CREATURE, 0, 0, 0, 0, 0, 0, -1, 0) ){
		add_counter(player, card, COUNTER_CHARGE);
	}

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_TRIGGER_OPTIONAL) ){
		draw_cards(player, count_counters(player, card, COUNTER_CHARGE));
	}

	return 0;
}

int card_training_grounds(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		set_cost_mod_for_activated_abilities(player, -1, 2, &this_test);
	}

	if( event == EVENT_CAST_SPELL && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			set_cost_mod_for_activated_abilities(affected_card_controller, affected_card, 2, 0);
		}
	}


	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		remove_cost_mod_for_activated_abilities(player, -1, 2, &this_test);
	}

	return global_enchantment(player, card, event);
}

int card_unified_will(int player, int card, event_t event){//DISABLED

  if( event == EVENT_CAN_CAST ){
	 int result = card_counterspell(player, card, event);
	 if( result == 0 ){
		return 0;
	 }
	 else{
		  if( IS_AI(player) && player == AI && count_permanents_by_type(player, TYPE_CREATURE) <= count_permanents_by_type(1-player, TYPE_CREATURE) ){
			  return 0;
		  }
		  else{
			   return result;
		  }
	 }

  }

  else if( event == EVENT_RESOLVE_SPELL ){
		  if( count_permanents_by_type(player, TYPE_CREATURE) <= count_permanents_by_type(1-player, TYPE_CREATURE) ){
			  spell_fizzled = 1;
		  }
		  else{
			   return card_counterspell(player, card, event);
		  }
  }

  return card_counterspell(player, card, event);

}

int card_venerated_teacher(int player, int card, event_t event){

  /* Venerated Teacher	|2|U
   * Creature - Human Wizard 2/2
   * When ~ enters the battlefield, put two level counters on each creature you control with level up. */

  int c;
  card_instance_t* inst;
  if (comes_into_play(player, card, event))
	for (c = 0; c < active_cards_count[player]; ++c)
	  if ((inst = in_play(player, c)) && is_what(player, c, TYPE_CREATURE) && get_counter_type_by_id(cards_data[inst->internal_card_id].id) == COUNTER_LEVEL)
		add_counters(player, c, COUNTER_LEVEL, 2);

  return 0;
}

/********
* Black *
********/

static void destroy_self_at_end_of_combat_if_vs_le_1_power(int player, int card, int t_player, int t_card)
{
  if (get_power(t_player, t_card) <= 1)
	create_legacy_effect_exe(player, card, LEGACY_EFFECT_STONING, player, card);
}
int card_arrogant_bloodlord(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && !is_humiliated(player, card))
	instance->destroys_if_blocked |= DIFB_ASK_CARD;

  if (event == EVENT_CHECK_DESTROY_IF_BLOCKED && affect_me(player, card) && !is_humiliated(player, card)
	  && get_power(attacking_card_controller, attacking_card) <= 1)
	event_result |= 2;

  if (event == EVENT_DECLARE_BLOCKERS && !is_humiliated(player, card))
	{
	  if (player == current_turn && (instance->state & STATE_ATTACKING))
		for_each_creature_blocking_me(player, card, destroy_self_at_end_of_combat_if_vs_le_1_power, player, card);

	  if (player == 1-current_turn && (instance->state & STATE_BLOCKING))
		for_each_creature_blocked_by_me(player, card, destroy_self_at_end_of_combat_if_vs_le_1_power, player, card);
	}

  return 0;
}

/* Bala Ged Scorpion	|3|B
 * Creature - Scorpion 2/3
 * When ~ enters the battlefield, you may destroy target creature with power 1 or less. */

int card_baneful_omen(int player, int card, event_t event){

	if( eot_trigger_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player)) ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			show_deck(HUMAN, deck, 1, "Baneful Omen reveals...", 0, 0x7375B0 );
			lose_life(1-player, get_cmc_by_internal_id(deck[0]));
		}
	}

	return global_enchantment(player, card, event);
}

/* Bloodrite Invoker	|2|B
 * Creature - Vampire Shaman 3/1
 * |8: Target player loses 3 life and you gain 3 life. */

/* Bloodthrone Vampire	|1|B => onslaught.c:Nantuko Husk
 * Creature - Vampire 1/1
 * Sacrifice a creature: ~ gets +2/+2 until end of turn. */

/* Cadaver Imp	|1|B|B => portal_1_2_3k.c:Gravedigger
 * Creature - Imp 1/1
 * Flying
 * When ~ enters the battlefield, you may return target creature card from your graveyard to your hand. */

int card_consume_the_meek(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i = 0; i < 2; i++){
			int count = active_cards_count[i]-1;
			while( count > -1 ){
				if( in_play(i, count) && is_what(i, count, TYPE_CREATURE) && get_cmc(i, count) < 4 ){
					kill_card(i, count, KILL_BURY);
				}
				count--;
			}
		}
		kill_card(player, card, KILL_BURY);
	}
	return 0;
}

int card_consuming_vapors(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		if( player != AI  ){
			return can_target(&td);
		}
		else{
			instance->targets[0].player = 1-player;
			if( would_valid_target(&td) ){
				return can_target(&td1);
			}
		}
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) && can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
				int p = instance->targets[0].player;
				int trg = pick_creature_for_sacrifice(p, card, 1);
				if( trg != -1 ){
					gain_life(player, get_toughness(p, trg));
					kill_card(p, trg, KILL_SACRIFICE );
				}
			}
			rebound(player, card);
	}
	return 0;
}

/* Contaminated Ground	|1|B => gatecrash.c
 * Enchantment - Aura
 * Enchant land
 * Enchanted land is |Ha Swamp.
 * Whenever enchanted land becomes tapped, its controller loses 2 life. */

/* Corpsehatch	|3|B|B
 * Sorcery
 * Destroy target non|Sblack creature. Put two 0/1 colorless Eldrazi Spawn creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to
 * your mana pool." */

int card_curse_of_wizardry(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[0].player = 1<<choose_a_color_and_show_legacy(player, card, 1-player, -1);
	}

	if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, instance->targets[0].player, 0, 0, 0, -1, 0)){
		lose_life(instance->targets[1].player, 1);
	}

	return global_enchantment(player, card, event);
}

/* Death Cultist	|B
 * Creature - Human Wizard 1/1
 * Sacrifice ~: Target player loses 1 life and you gain 1 life. */

/* Demonic Appetite	|B
 * Enchantment - Aura
 * Enchant creature you control
 * Enchanted creature gets +3/+3.
 * At the beginning of your upkeep, sacrifice a creature. */

int card_drana_kalastrian_highborn(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_B(2), 0, &td, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			charge_mana_for_activated_ability(player, card, MANACOST_B(2));
			if( spell_fizzled != 1 ){
				charge_mana(player, COLOR_COLORLESS, -1);
				if( spell_fizzled != 1 ){
					instance->targets[1].card = x_value;
				}
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = instance->targets[1].card;
			if( amount > 0 ){
				pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, amount, 0);
				pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, -amount);
			}
		}
	}
	return 0;
}

/* Dread Drone	|4|B
 * Creature - Eldrazi Drone 4/1
 * When ~ enters the battlefield, put two 0/1 colorless Eldrazi Spawn creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your
 * mana pool." */

/* Escaped Null	|3|B
 * Creature - Zombie Berserker 1/2
 * Lifelink
 * Whenever ~ blocks or becomes blocked, it gets +5/+0 until end of turn. */

/* Essence Feed	|5|B
 * Sorcery
 * Target player loses 3 life. You gain 3 life and put three 0/1 colorless Eldrazi Spawn creature tokens onto the battlefield. They have "Sacrifice this
 * creature: Add |1 to your mana pool." */

/* Gloomhunter	|2|B (vanilla)
 * Creature - Bat 2/1
 * Flying */

int card_guul_draz_assassin(int player, int card, event_t event){

	/* Guul Draz Assassin	|B
	 * Creature - Vampire Assassin 1/1
	 * Level up |1|B
	 * LEVEL 2-3	2/2
	 * |B, |T: Target creature gets -2/-2 until end of turn.
	 * LEVEL 4+	4/4
	 * |B, |T: Target creature gets -4/-4 until end of turn. */

	level_up_effects(player, card, event,
					    1,1,
					 2, 2,2, 0,0,
					 4, 4,4, 0,0);

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance( player, card );

	int level = count_counters_no_parent(player, card, COUNTER_LEVEL);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0) && can_sorcery_be_played(player, event) ){
			return 1;
		}
		if( level > 1 && has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && ! is_tapped(player, card) && ! is_sick(player, card) &&
			can_target(&td)
		  ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0) && can_sorcery_be_played(player, event) ){
			if( level > 1 && has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && ! is_tapped(player, card) && ! is_sick(player, card) &&
				can_target(&td)
			  ){
				if( level > 3 ){
					choice = do_dialog(player, player, card, -1, -1, " Level up\n -4/-4 to target creature\n Cancel", 1);
				}
				else{
					choice = do_dialog(player, player, card, -1, -1, " Level up\n -2/-2 to target creature\n Cancel", 1);
				}
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 && charge_mana_for_activated_ability(player, card, 1, 1, 0, 0, 0, 0) ){
			instance->info_slot = 66+choice;
		}
		else if( choice == 1 && charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			instance->info_slot = 66+choice;
			tap_card(player, card);
			instance->number_of_targets = 1;
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION && affect_me(player, card) ){
			if( instance->info_slot == 66 ){
			   add_counter(instance->parent_controller, instance->parent_card, COUNTER_LEVEL);
			}
			if( instance->info_slot == 67 && valid_target(&td) ){
				int minus = -2;
				if( level > 3 ){
					minus = -4;
				}
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, minus, minus);
			}
	}
	return 0;
}

int card_hellcarver_demon(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.not_me = 1;
		new_manipulate_all(player, card, player, &this_test, KILL_SACRIFICE);
		discard_all(player);
		int amount = MIN(6, count_deck(player));
		if( amount > 0 ){
			int *deck = deck_ptr[player];
			show_deck( HUMAN, deck, amount, "Cards exiled by Hellcarver Demon", 0, 0x7375B0 );
			int count = amount-1;
			int demon[amount];
			int demon_count = 0;
			while( count > -1 ){
					if( ! is_what(-1, deck[count], TYPE_LAND) && can_legally_play_iid(player, deck[count]) ){
						demon[demon_count] = deck[count];
						demon_count++;
					}
					rfg_card_in_deck(player, count);
					count--;
			}
			while( demon_count > 0 ){
					test_definition_t this_test2;
					new_default_test_definition(&this_test2, TYPE_ANY, "Select a card to play for free");
					int selected = select_card_from_zone(player, player, demon, demon_count, 0, AI_MAX_CMC, -1, &this_test2);
					if( selected != -1 ){
						int csvid = cards_data[demon[selected]].id;
						int k;
						for(k=selected; k<demon_count; k++){
							demon[k] = demon[k+1];
						}
						demon_count--;
						play_card_in_exile_for_free(player, 1-player, csvid);
					}
					else{
						break;
					}
			}
		}
	}

	return 0;
}

/* Induce Despair	|2|B
 * Instant
 * As an additional cost to cast ~, reveal a creature card from your hand.
 * Target creature gets -X/-X until end of turn, where X is the revealed card's converted mana cost. */

int card_inquisition_of_kozilek(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				ec_definition_t ec;
				default_ec_definition(instance->targets[0].player, player, &ec);

				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_LAND, "Select a nonland card with CMC 3 or less.");
				this_test.type_flag = DOESNT_MATCH;
				this_test.cmc = 4;
				this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
				new_effect_coercion(&ec, &this_test);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

/* Last Kiss	|2|B (vicious hunger)
 * Instant
 * ~ deals 2 damage to target creature and you gain 2 life. */

int card_mortician_beetle(int player, int card, event_t event){
	/* Mortician Beetle	|B
	 * Creature - Insect 1/1
	 * Whenever a player sacrifices a creature, you may put a +1/+1 counter on ~. */

	if (whenever_a_player_sacrifices_a_permanent(player, card, event, ANYBODY, TYPE_CREATURE, RESOLVE_TRIGGER_AI(player))){
		add_1_1_counter(player, card);
	}

	return 0;
}

/* Nighthaze	|B
 * Sorcery
 * Target creature gains |H2swampwalk until end of turn.
 * Draw a card. */

int card_nirkana_cutthroat(int player, int card, event_t event)
{
  /* Nirkana Cutthroat	|2|B
   * Creature - Vampire Warrior 3/2
   * Level up |2|B
   * LEVEL 1-2	4/3
   * Deathtouch
   * LEVEL 3+	5/4
   * First strike, deathtouch */

  return level_up(player, card, event, MANACOST_XB(2,1),
				     3,2,
				  1, 4,3, 0,                   SP_KEYWORD_DEATHTOUCH,
				  3, 5,4, KEYWORD_FIRST_STRIKE,SP_KEYWORD_DEATHTOUCH);
}

int card_nirkana_revenant(int player, int card, event_t event){

	produce_mana_when_subtype_is_tapped(player, event, TYPE_PERMANENT, SUBTYPE_SWAMP, COLOR_BLACK, 1);

	return generic_shade(player, card, event, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 0);
}

int card_null_champion(int player, int card, event_t event)
{
  /* Null Champion	|1|B
   * Creature - Zombie Warrior 1/1
   * Level up |3
   * LEVEL 1-3	4/2
   * LEVEL 4+	7/3
   * |B: Regenerate ~. */

  if (IS_ACTIVATING(event) && (land_can_be_played & LCBP_REGENERATION) && count_counters_no_parent(player, card, COUNTER_LEVEL) >= 4)
	return regeneration(player, card, event, MANACOST_B(1));

  return level_up(player, card, event, MANACOST_X(3),
				     1,1,
				  1, 4,2, 0,0,
				  4, 7,3, KEYWORD_REGENERATION,0);
}

int card_pawn_of_ulamog(int player, int card, event_t event){
	/* Pawn of Ulamog	|1|B|B
	 * Creature - Vampire Shaman 2/2
	 * Whenever ~ or another nontoken creature you control dies, you may put a 0/1 colorless Eldrazi Spawn creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool." */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.type_flag = F1_NO_TOKEN;
		test.not_me = 1;

		count_for_gfp_ability(player, card, event, player, 0, &test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SPAWN, instance->targets[11].card);
		instance->targets[11].card = 0;
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		generate_token_by_id(player, card, CARD_ID_ELDRAZI_SPAWN);
	}

	return 0;
}

/* Perish the Thought	|2|B
 * Sorcery
 * Target opponent reveals his or her hand. You choose a card from it. That player shuffles that card into his or her library. */

int card_pestilence_demon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_RESOLVE_ACTIVATION ){
		damage_all(player, instance->parent_card, player, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		damage_all(player, instance->parent_card, 1-player, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return generic_activated_ability(player, card, event, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0);
}

/* Repay in Kind	|5|B|B
 * Sorcery
 * Each player's life total becomes the lowest life total among all players. */

/* Shrivel	|1|B => 0x40ff70:Nausea
 * Sorcery
 * All creatures get -1/-1 until end of turn. */

/* Skeletal Wurm	|7|B (drudge skeletons)
 * Creature - Skeleton Wurm 7/6
 * |B: Regenerate ~. */

/* Suffer the Past	|X|B
 * Instant
 * Exile X target cards from target player's graveyard. For each card exiled this way, that player loses 1 life and you gain 1 life. */

int card_thought_gorger(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		add_1_1_counters(player, card, hand_count[player]);
		discard_all(player);
	}

	if( leaves_play(player, card, event) ){
		draw_cards(player, count_1_1_counters(player, card));
	}

	return 0;
}

/* Vendetta	|B => mercadian_masques.c
 * Instant
 * Destroy target non|Sblack creature. It can't be regenerated. You lose life equal to that creature's toughness. */

int card_virulent_swipe(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 0, 0, SP_KEYWORD_DEATHTOUCH);
			rebound(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

/* Zof Shade	|3|B (generic_shade)
 * Creature - Shade 2/2
 * |2|B: ~ gets +2/+2 until end of turn. */

int card_zulaport_enforcer(int player, int card, event_t event)
{
  /* Zulaport Enforcer	|B
   * Creature - Human Warrior 1/1
   * Level up |4
   * LEVEL 1-2	3/3
   * LEVEL 3+	5/5
   * ~ can't be blocked except by |Sblack creatures. */

  // Need to value counters slightly more than usual, so the AI will be willing to pay |4 for the second one.
  if (event == EVENT_SHOULD_AI_PLAY && player == AI)
	{
	  int counters = count_counters(player, card, COUNTER_LEVEL);
	  counters = MIN(4, counters);
	  ai_modifier += counters;
	}

  if (event == EVENT_BLOCK_LEGALITY
	  && attacking_card_controller == player && attacking_card == card
	  && count_counters(player, card, COUNTER_LEVEL) >= 3
	  && !(get_color(affected_card_controller, affected_card) & get_sleighted_color_test(player, card, COLOR_TEST_BLACK)))
	event_result = 1;

  return level_up(player, card, event, MANACOST_X(4),
				     1,1,
				  1, 3,3, 0,0,
				  3, 5,5, 0,0);
}

/******
* Red *
******/

/* Akoum Boulderfoot	|4|R|R (sparkmage apprentice)
 * Creature - Giant Warrior 4/5
 * When ~ enters the battlefield, it deals 1 damage to target creature or player. */

/* Battle Rampart	|2|R
 * Creature - Wall 1/3
 * Defender
 * |T: Target creature gains haste until end of turn. */

/* Battle-Rattle Shaman	|3|R
 * Creature - Goblin Shaman 2/2
 * At the beginning of combat on your turn, you may have target creature get +2/+0 until end of turn. */

int card_brimstone_mage(int player, int card, event_t event)
{
  /* Brimstone Mage	|2|R
   * Creature - Human Shaman 2/2
   * Level up |3|R
   * LEVEL 1-2	2/3
   * |T: ~ deals 1 damage to target creature or player.
   * LEVEL 3+	2/4
   * |T: ~ deals 3 damage to target creature or player. */

  level_up_effects(player, card, event,
				      2,2,
				   1, 2,3, 0,0,
				   3, 2,4, 0,0);

  if (IS_ACTIVATING(event))
	{
	  int level = count_counters_no_parent(player, card, COUNTER_LEVEL);

	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	  char damage_prompt[100];
	  if (ai_is_speculating == 1)
		damage_prompt[0] = 0;
	  else
		scnprintf(damage_prompt, 100, "Deal %d damage", level >= 3 ? 3 : 1);

	  int duh_damage = level >= 3 && !IS_AI(player) && duh_mode(player);

	  enum
	  {
		CHOICE_LEVEL_UP = 1,
		CHOICE_DAMAGE
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						(level == 0 || duh_damage) ? DLG_AUTOCHOOSE_IF_1 : DLG_NO_OP,
						"Level up",
							can_sorcery_be_played(player, event) && !duh_damage,
							level < 3 ? 3 : -1,
							DLG_MANA(MANACOST_XR(3,1)),
						damage_prompt,
							level >= 1,
							1,
							DLG_TAP, DLG_TARGET(&td, "TARGET_CREATURE_OR_PLAYER"));

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		switch (choice)
		  {
			case CHOICE_LEVEL_UP:
			  add_counter(player, card, COUNTER_LEVEL);
			  break;

			case CHOICE_DAMAGE:
			  damage_target0(player, card, level >= 3 ? 3 : 1);
			  break;
		  }
	}

  // The most straightforward parts of the AI from card_prodigal_sorcerer() in the exe, which is quite extensive.

  // Let AI know that it can damage blockers when considering what to attack with
  if (event == EVENT_CHECK_PUMP && CAN_TAP(player, card) && CAN_ACTIVATE0(player, card))
	{
	  int level = count_counters(player, card, COUNTER_LEVEL);
	  if (level >= 3)
		pumpable_toughness[1 - player] -= 3;
	  else if (level >= 1)
		pumpable_toughness[1 - player] -= 1;
	}

  if (event == EVENT_SHOULD_AI_PLAY && player == AI)
	{
	  // Need to value counters slightly more than usual, so the AI will be willing to pay |4 for the second one.
	  int level = count_counters(player, card, COUNTER_LEVEL);
	  level = MIN(3, level);
	  ai_modifier += level;

	  // Discourage AI from tapping during its own turn if it could tap to deal damage.
	  if (level > 0 && current_turn == player && CAN_TAP(player, card) && CAN_ACTIVATE0(player, card))
		ai_modifier += level >= 3 ? 72 : 24;
	}

  /* Discourage AI from attacking with this if it could tap to deal damage.  Prodigal Sorcerer also discourages blocking by subtracking 48 from
   * ai_defensive_modifier during EVENT_BLOCK_RATING; I'm omitting it for this. */
  if (event == EVENT_ATTACK_RATING && affect_me(player, card) && CAN_ACTIVATE0(player, card)
	  && count_counters(player, card, COUNTER_LEVEL) > 0)
	ai_defensive_modifier += 48;

  return 0;
}

/* Brood Birthing	|1|R
 * Sorcery
 * If you control an Eldrazi Spawn, put three 0/1 colorless Eldrazi Spawn creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to
 * your mana pool." Otherwise, put one of those tokens onto the battlefield. */

int card_conquering_manticore(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_CREATURE")  ){
			effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_devastating_summons(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);
  td.allowed_controller = player;
  td.preferred_controller = player;
  td.illegal_abilities = 0;
  td.allow_cancel = 0;

  card_instance_t *instance = get_card_instance(player, card);

  if( event == EVENT_CAN_CAST){
	 return 1;
  }

  if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
	 int result = 0;

	 if( IS_AI(player) && player == AI ){
		if( life[1-player]/2 <= count_permanents_by_type(player, TYPE_LAND) ){

		   int i;
		   for( i= active_cards_count[player]-1; i > -1; i--){
			   if( in_play(player, i) && is_what(player, i, TYPE_LAND) ){
				  kill_card(player, i, KILL_SACRIFICE);
				  result++;
				  if( result >= life[1-player]/2 ){
					 break;
				  }
			   }
		   }

		}
		else{

		   int i;
		   for( i= active_cards_count[player]-1; i > -1; i--){
			   if( in_play(player, i) && is_what(player, i, TYPE_LAND) && is_tapped(player, i) ){
				  kill_card(player, i, KILL_SACRIFICE);
				  result++;
			   }
		   }
		}

	 }
	 else{
		  while( can_target(&td) ){
				pick_target(&td, "TARGET_LAND");
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
				result++;
				if( can_target(&td) ){
					int choice = do_dialog(player, player, card, -1, -1, " Continue\n Stop", 0);
					if( choice == 1 ){
						break;
					}
				}
		  }
	 }
	 instance->info_slot = result;
  }

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.qty = 2;
		token.pow = instance->info_slot;
		token.tou = instance->info_slot;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_disaster_radius(int player, int card, event_t event){

	char buffer[100] = " Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, buffer);
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return check_battlefield_for_special_card(player, card, player, 0, &this_test);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
		if( selected != -1 ){
			reveal_card(player, card, player, selected);
			instance->info_slot = get_cmc(player, selected);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		new_damage_all(player, card, 1-player, instance->info_slot, 0, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

/* Emrakul's Hatcher	|4|R
 * Creature - Eldrazi Drone 3/3
 * When ~ enters the battlefield, put three 0/1 colorless Eldrazi Spawn creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your
 * mana pool." */

int card_explosive_revelation(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int *deck = deck_ptr[player];
			if( deck[0] != -1 ){
				int count = 0;
				int dmg = 0;
				while( deck[count] != -1 ){
						if( ! is_what(-1, deck[count], TYPE_LAND) ){
							dmg = get_cmc_by_internal_id(deck[count]);
							break;
						}
						count++;
				}
				count++;
				show_deck( player, deck, count, "Cards revealed by Explosive Revelation,", 0, 0x7375B0 );
				if( dmg > 0 ){
					damage_creature_or_player(player, card, event, dmg);
					add_card_to_hand(player, deck[count-1]);
					remove_card_from_deck(player, count-1);
					count--;
				}
				if( count > 0 ){
					put_top_x_on_bottom(player, player, count);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

/* Fissure Vent	|3|R|R
 * Sorcery
 * Choose one or both - Destroy target artifact; and/or destroy target nonbasic land. */

int card_flame_slash(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card)){
			pick_target(&td, "TARGET_CREATURE");
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
			}
			kill_card(player, card, KILL_DESTROY );
	}
	return 0;
}

int card_forked_bolt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", 1, 1);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		divide_damage(player, card, &td);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_goblin_arsonist(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return 0;
}

/* Goblin Tunneler	|1|R => 0x4c73d0:Dwarven Warriors
 * Creature - Goblin Rogue 1/1
 * |T: Target creature with power 2 or less is unblockable this turn. */

/* Grotag Siege-Runner	|1|R
 * Creature - Goblin Rogue 2/1
 * |R, Sacrifice ~: Destroy target creature with defender. ~ deals 2 damage to that creature's controller. */

/* Heat Ray	|X|R
 * Instant
 * ~ deals X damage to target creature. */

int card_hellion_eruption(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		int amount = new_manipulate_all(player, card, player, &this_test, KILL_SACRIFICE);

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_HELLION, &token);
		token.qty = amount;
		token.pow = 4;
		token.tou = 4;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_kargan_dragonlord(int player, int card, event_t event)
{
  /* Kargan Dragonlord	|R|R
   * Creature - Human Warrior 2/2
   * Level up |R
   * LEVEL 4-7	4/4
   * Flying
   * LEVEL 8+	8/8
   * Flying, trample
   * |R: ~ gets +1/+0 until end of turn. */

  if (IS_SHADE_EVENT(event) && count_counters_no_parent(player, card, COUNTER_LEVEL) >= 8)
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_R(1), 1,0);

  return level_up(player, card, event, MANACOST_R(1),
				     2,2,
				  4, 4,4, KEYWORD_FLYING,0,
				  8, 8,8, KEYWORD_FLYING|KEYWORD_TRAMPLE,0);
}

int card_kiln_fiend(int player, int card, event_t event){
	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_SPELL, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		pump_until_eot(player, card, player, card, 3, 0);
	}
	return 0;
}

/* Lagac Lizard	|3|R (vanilla)
 * Creature - Lizard 3/3 */

/* Lavafume Invoker	|2|R
 * Creature - Goblin Shaman 2/2
 * |8: Creatures you control get +3/+0 until end of turn. */

int card_lord_of_shatterskull_pass(int player, int card, event_t event)
{
  /* Lord of Shatterskull Pass	|3|R
   * Creature - Minotaur Shaman 3/3
   * Level up |1|R
   * LEVEL 1-5	6/6
   * LEVEL 6+	6/6
   * Whenever ~ attacks, it deals 6 damage to each creature defending player controls. */

  if ((event == EVENT_DECLARE_ATTACKERS || (xtrigger_condition() == XTRIGGER_ATTACKING && affect_me(player, card)))
	  && count_counters(player, card, COUNTER_LEVEL) >= 6 && declare_attackers_trigger(player, card, event, 0, player, card))
	new_damage_all(player, card, 1-player, 6, 0, NULL);

  return level_up(player, card, event, MANACOST_XR(1,1),
				     3,3,
				  1, 6,6, 0,0,
				  6, 6,6, 0,0);
}

/* Lust for War	|2|R
 * Enchantment - Aura
 * Enchant creature
 * Whenever enchanted creature becomes tapped, ~ deals 3 damage to that creature's controller.
 * Enchanted creature attacks each turn if able. */

int card_magmaw(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) &&
		can_target(&td)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_LAND, 1, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if (charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0)
			&& sacrifice(player, card, player, 0, TYPE_LAND, 1, 0, 0, 0, 0, 0, 0, -1, 0)
			&& pick_target(&td, "TARGET_CREATURE_OR_PLAYER")){
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
		}
	}

	return 0;
}

/* Ogre Sentry	|1|R (vanilla)
 * Creature - Ogre Warrior 3/3
 * Defender */

int card_rage_nimbus(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_ACTIVATE){
		return (can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_XR(1, 1))
				&& (player == HUMAN || current_turn == HUMAN));
	}

	if(event == EVENT_ACTIVATE ){
		if (charge_mana_for_activated_ability(player, card, MANACOST_XR(1, 1)) && pick_target(&td, "TARGET_CREATURE")){
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_MUST_ATTACK);
		}
	}

	return 0;
}

int card_raid_bombardment(int player, int card, event_t event)
{
  // Whenever a creature you control with power 2 or less attacks, ~ deals 1 damage to defending player.
  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.power = 3;
	  test.power_flag = F5_POWER_LESSER_THAN_VALUE;

	  if (declare_attackers_trigger_test(player, card, event, DAT_SEPARATE_TRIGGERS, player, -1, &test))
		damage_player(1-current_turn, 1, player, card);
	}

  return global_enchantment(player, card, event);
}

int card_rapacious_one(int player, int card, event_t event){
	/* Rapacious One	|5|R
	 * Creature - Eldrazi Drone 5/4
	 * Trample
	 * Whenever ~ deals combat damage to a player, put that many 0/1 colorless Eldrazi Spawn creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your mana pool." */

	card_instance_t *instance = get_card_instance(player, card);
	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_OPPONENT+DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_REPORT_DAMAGE_DEALT) ){
		generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SPAWN, instance->targets[16].player);
		instance->targets[16].player = 0;
	}

	return 0;
}

/* Soulsurge Elemental	|3|R
 * Creature - Elemental 100/1
 * First strike
 * ~'s power is equal to the number of creatures you control. */

/* Spawning Breath	|1|R
 * Instant
 * ~ deals 1 damage to target creature or player. Put a 0/1 colorless Eldrazi Spawn creature token onto the battlefield. It has "Sacrifice this creature: Add |1
 * to your mana pool." */

int card_splinter_twin(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( in_play(player, card) && instance->damage_target_player > -1 ){
			int t_player = instance->damage_target_player;
			int t_card = instance->damage_target_card;

			if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(t_player, t_card) && ! is_sick(t_player, t_card) ){
				return has_mana_for_activated_ability(t_player, t_card, 0, 0, 0, 0, 0, 0);
			}
			else if( event == EVENT_ACTIVATE ){
					if( charge_mana_for_activated_ability(t_player, t_card, 0, 0, 0, 0, 0, 0) ){
						tap_card(t_player, t_card);
					}
			}
			else if( event == EVENT_RESOLVE_ACTIVATION ){
					token_generation_t token;
					copy_token_definition(t_player, t_card, &token, t_player, t_card);
					token.legacy = 1;
					token.special_code_for_legacy = &haste_and_remove_eot;
					generate_token(&token);
			}
	}
	return generic_aura(player, card, event, player, 0, 0, 0, 0, 0, 0, 0);
}

int card_staggershock(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td)  ){
				damage_creature_or_player(player, card, event, 2);
				rebound(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}
	return 0;
}

int card_surreal_memoir(int player, int card, event_t event){

	char msg[100] = "Select a sorcery or instant card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_INSTANT, msg);
	this_test.type_flag = F1_NO_CREATURE;

	if( event == EVENT_CAN_CAST ){
		if( new_special_count_grave(player, &this_test) ){
			return ! graveyard_has_shroud(2);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		tutor_random_permanent_from_grave(player, card, player, TUTOR_HAND, TYPE_INSTANT, 1, REANIMATE_DEFAULT);
		rebound(player, card);
	}

	return 0;
}

int card_traitorous_instinct(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int legacy = effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[2].player = 2;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_tuktuk_the_explorer(int player, int card, event_t event){
	/* Tuktuk the Explorer	|2|R
	 * Legendary Creature - Goblin 1/1
	 * Haste
	 * When ~ dies, put a legendary 5/5 colorless Goblin Golem artifact creature token named Tuktuk the Returned onto the battlefield. */

	check_legend_rule(player, card, event);

	haste(player, card, event);

	if( graveyard_from_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_TUKTUK_THE_RETURNED);
	}

	return 0;
}

int card_tuktuk_the_returned(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	return 0;
}

int card_valakut_fireboar(int player, int card, event_t event)
{
  // Whenever ~ attacks, switch its power and toughness until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	switch_power_and_toughness_until_eot(player, card, player, card);

  return 0;
}

/* Vent Sentinel	|3|R
 * Creature - Elemental 2/4
 * Defender
 * |1|R, |T: ~ deals damage to target player equal to the number of creatures with defender you control. */

static int waw_legacy(int player, int card, event_t event){
	if( end_of_combat_trigger(player, card, event, 2) ){
		relentless_assault_effect(player, card);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}
int card_world_at_war(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			create_legacy_effect(player, card, waw_legacy);
			rebound(player, card);
	}

   return 0;
}

/* Wrap in Flames	|3|R
 * Sorcery
 * ~ deals 1 damage to each of up to three target creatures. Those creatures can't block this turn. */

/********
* Green *
********/

int card_ancient_stirrings(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL){
			int amount = 5;
			if( amount > count_deck(player) ){
				amount = count_deck(player);
			}
			if( amount > 0 ){
				char msg[100] = "Select a colorless card.";
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, msg);
				this_test.color = COLOR_TEST_ANY_COLORED;
				this_test.color_flag = DOESNT_MATCH;
				this_test.create_minideck = amount;
				this_test.no_shuffle = 1;
				int selected = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
				if( selected >= 0 ){
					reveal_card(player, card, player, selected);
					amount--;
				}
				if( amount > 0 ){
					put_top_x_on_bottom(player, player, amount);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_aura_gnarlid(int player, int card, event_t event)
{
  /* Aura Gnarlid	|2|G
   * Creature - Beast 2/2
   * Creatures with power less than ~'s power can't block it.
   * ~ gets +1/+1 for each Aura on the battlefield. */

  if (event == EVENT_BLOCK_LEGALITY
	  && player == attacking_card_controller && card == attacking_card
	  && get_power(affected_card_controller, affected_card) < get_power(player, card))
	event_result = 1;

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card))
	event_result += count_subtype(ANYBODY, TYPE_ENCHANTMENT, SUBTYPE_AURA);

  return 0;
}

int card_awakening_zone(int player, int card, event_t event){
	/* Awakening Zone	|2|G
	 * Enchantment
	 * At the beginning of your upkeep, you may put a 0/1 colorless Eldrazi Spawn creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool." */

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		generate_token_by_id(player, card, CARD_ID_ELDRAZI_SPAWN);
	}

	return global_enchantment(player, card, event);
}

int card_bear_umbra(int player, int card, event_t event)
{
  // Enchanted creature gets +2/+2 and has "Whenever this creature attacks, untap all lands you control."
  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  card_instance_t* instance = in_play(player, card);
	  if (instance && instance->damage_target_player >= 0
		  && !is_humiliated(instance->damage_target_player, instance->damage_target_card)
		  && declare_attackers_trigger(player, card, event, 0, instance->damage_target_player, instance->damage_target_card))
		manipulate_all(player, card, player, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_UNTAP);
	}

  return totem_armor(player, card, event, 2, 2, 0, 0);
}

int card_beastbreaker_of_bala_ged(int player, int card, event_t event)
{
  /* Beastbreaker of Bala Ged	|1|G
   * Creature - Human Warrior 2/2
   * Level up |2|G
   * LEVEL 1-3	4/4
   * LEVEL 4+	6/6
   * Trample */

  return level_up(player, card, event, MANACOST_XG(2,1),
				     2,2,
				  1, 4,4, 0,0,
				  4, 6,6, KEYWORD_TRAMPLE,0);
}

int card_boar_umbra(int player, int card, event_t event){

	return totem_armor(player, card, event, 3, 3, 0, 0);
}

int card_bramblesnap(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.illegal_state = TARGET_STATE_TAPPED;

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		return can_target(&td);
	}

	else if(event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
				instance->number_of_targets = 1;
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			if( in_play(instance->parent_controller, instance->parent_card) ){
				pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, 1);
			}
	}

	return 0;
}

int card_broodwarden(int player, int card, event_t event){
	if( player == affected_card_controller && in_play(player, card)&& has_creature_type(player, affected_card, SUBTYPE_SPAWN) && has_creature_type(player, affected_card, SUBTYPE_ELDRAZI) ){
		if( event == EVENT_POWER ){
			event_result += 2;
		}
		else if( event == EVENT_TOUGHNESS ){
			event_result += 1;
		}
	}
	return 0;
}

/* Daggerback Basilisk	|2|G (deadly recluse)
 * Creature - Basilisk 2/2
 * Deathtouch */

int card_gelatinous_genesis(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 1, NULL);
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			set_special_flags2(player, card, SF2_X_SPELL);
			int angels = charge_mana_for_double_x(player, COLOR_COLORLESS);
			if( spell_fizzled != 1 ){
				instance->info_slot = angels/2;
			}
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_OOZE, &token);
		token.qty = instance->info_slot;
		token.pow = instance->info_slot;
		token.tou = instance->info_slot;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_gigantomancer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			set_pt_and_abilities_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 7, 7, 0, 0, 0);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_X(1), 0, &td, "TARGET_CREATURE");
}

/* Gravity Well	|1|G|G
 * Enchantment
 * Whenever a creature with flying attacks, it loses flying until end of turn. */

int card_growth_spasm(int player, int card, event_t event){
	/* Growth Spasm	|2|G
	 * Sorcery
	 * Search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library. Put a 0/1 colorless Eldrazi Spawn creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool." */

	if( event == EVENT_RESOLVE_SPELL ){
		generate_token_by_id(player, card, CARD_ID_ELDRAZI_SPAWN);
	}
	return card_rampant_growth(player, card, event);
}

/* Haze Frog	|3|G|G
 * Creature - Frog 2/1
 * Flash
 * When ~ enters the battlefield, prevent all combat damage that other creatures would deal this turn. */

/* Irresistible Prey	|G
 * Sorcery
 * Target creature must be blocked this turn if able.
 * Draw a card. */

/* Jaddi Lifestrider	|4|G
 * Creature - Elemental 2/8
 * When ~ enters the battlefield, you may tap any number of untapped creatures you control. You gain 2 life for each creature tapped this way. */

int card_joraga_treespeaker(int player, int card, event_t event){
	/* Joraga Treespeaker	|G
	 * Creature - Elf Druid 1/1
	 * Level up |1|G
	 * LEVEL 1-4	1/2
	 * |T: Add |G|G to your mana pool.
	 * LEVEL 5+	1/4
	 * Elves you control have "|T: Add |G|G to your mana pool." */

	level_up_effects(player, card, event,
					    1,1,
					 1, 1,2, 0,0,
					 5, 1,4, 0,0);

	if (!IS_ACTIVATING(event) && !(event == EVENT_COUNT_MANA && affect_me(player, card))){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	int level = count_counters(player, card, COUNTER_LEVEL);

#define TAP_ELF(evnt)	(permanents_you_control_can_tap_for_mana(player, card, (evnt), TYPE_PERMANENT, SUBTYPE_ELF, COLOR_GREEN, 2))

#define CAN_LEVEL_UP	(!paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) && can_sorcery_be_played(player, event))
#define CAN_GET_MANA	(level > 0 && !is_tapped(player, card) && !is_sick(player, card) && can_produce_mana(player, card))
#define CAN_TAP_ELF		(level > 4 && TAP_ELF(EVENT_CAN_ACTIVATE))
#define CHOICE_LEVEL	(1<<0)
#define CHOICE_MANA		(1<<1)
#define CHOICE_ELF		(1<<2)

	if( event == EVENT_CAN_ACTIVATE ){
		return CAN_LEVEL_UP || CAN_GET_MANA || CAN_TAP_ELF;
	}

	if( event == EVENT_ACTIVATE ){
		instance->info_slot = 0;

		int choice = (CAN_LEVEL_UP ? CHOICE_LEVEL : 0) | (CAN_GET_MANA ? CHOICE_MANA : 0) | (CAN_TAP_ELF ? CHOICE_ELF : 0);

		switch (num_bits_set(choice)){
			case 0:
				return 0;
			case 1:
				break;
			default:{
				char buffer[500];
				int pos = 0;
				int ai_choice = -1;
				buffer[pos++] = ' ';
				if (choice & CHOICE_LEVEL){
					ai_choice = 0;
				} else {
					buffer[pos++] = '_';
				}
				pos += scnprintf(buffer + pos, 500-pos, "Level up\n ", buffer);
				if (choice & CHOICE_MANA){
					ai_choice = 1;
				} else {
					buffer[pos++] = '_';
				}
				pos += scnprintf(buffer + pos, 500-pos, "Tap for GG\n ", buffer);
				if (choice & CHOICE_ELF){
					if (ai_choice != 1){
						ai_choice = 2;
					}
				} else {
					buffer[pos++] = '_';
				}
				pos += scnprintf(buffer + pos, 500-pos, "Tap an Elf for GG\n Cancel", buffer);

				choice = 1 << do_dialog(player, player, card, -1, -1, buffer, ai_choice);
			}
		}

		instance->info_slot = 0;
		if( choice == CHOICE_LEVEL && charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) ){
			instance->info_slot = 66;
		}
		else if( choice == CHOICE_MANA ){
			produce_mana_tapped(player, card, COLOR_GREEN, 2);
		}
		else if( choice == CHOICE_ELF ){
			TAP_ELF(event);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			add_counter(instance->parent_controller, instance->parent_card, COUNTER_LEVEL);
		}
	}

	if( level > 0 && event == EVENT_COUNT_MANA && affect_me(player, card) ){
		if (level > 4){
			TAP_ELF(event);
		} else if (CAN_GET_MANA){
			declare_mana_available(player, COLOR_GREEN, 2);
		}
	}

	return 0;
#undef TAP_ELF
#undef CAN_LEVEL_UP
#undef CAN_GET_MANA
#undef CAN_TAP_ELF
#undef CHOICE_LEVEL
#undef CHOICE_MANA
#undef CHOICE_ELF
}

int card_kazandu_tuskcaller(int player, int card, event_t event){

	/* Kazandu Tuskcaller	|1|G
	 * Creature - Human Shaman 1/1
	 * Level up |1|G
	 * LEVEL 2-5	1/1
	 * |T: Put a 3/3 |Sgreen Elephant creature token onto the battlefield.
	 * LEVEL 6+	1/1
	 * |T: Put two 3/3 |Sgreen Elephant creature tokens onto the battlefield. */

	level_up_effects(player, card, event,
					    1,1,
					 2, 1,1, 0,0,
					 6, 1,1, 0,0);

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	card_instance_t *instance = get_card_instance( player, card );

	int level = count_counters_no_parent(player, card, COUNTER_LEVEL);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) && can_sorcery_be_played(player, event) ){
			return 1;
		}
		if( level > 1 && ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return 1;
		}
	}
	if( event == EVENT_ACTIVATE && affect_me(player, card) ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) && can_sorcery_be_played(player, event) ){
			if( level > 1 && ! is_tapped(player, card) && ! is_sick(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
				if( level < 6){
					choice = do_dialog(player, player, card, -1, -1, " Level up\n Generate a 3/3 Elephant token\n Cancel", 1);
				}
				else{
					choice = do_dialog(player, player, card, -1, -1, " Level up\n Generate 2 3/3 Elephant tokens\n Cancel", 1);
				}
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 && charge_mana_for_activated_ability(player, card, 1, 0, 0, 1, 0, 0) ){
			instance->info_slot = 66+choice;
		}
		else if( choice == 1 && charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			instance->info_slot = 66+choice;
			tap_card(player, card);
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION && affect_me(player, card) ){
			if( instance->info_slot == 66 ){
			   add_counter(instance->parent_controller, instance->parent_card, COUNTER_LEVEL);
			}
			if( instance->info_slot == 67 ){
				int amount = 1;
				if( level >= 6 ){
					amount++;
				}
				generate_tokens_by_id(player, card, CARD_ID_ELEPHANT, amount);
			}
	}
	return 0;
}

int card_khalni_hydra(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_color = COLOR_TEST_GREEN;
	td.illegal_abilities = 0;

	if( event == EVENT_MODIFY_COST ){
		COST_GREEN -= target_available(player, card, &td);
	}

	return 0;
}

int card_kozileks_predator(int player, int card, event_t event){
	/* Kozilek's Predator	|3|G
	 * Creature - Eldrazi Drone 3/3
	 * When ~ enters the battlefield, put two 0/1 colorless Eldrazi Spawn creature tokens onto the battlefield. They have "Sacrifice this creature: Add |1 to your mana pool." */

	if( comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_ELDRAZI_SPAWN, 2);
	}
	return 0;
}

/* Leaf Arrow	|G
 * Instant
 * ~ deals 3 damage to target creature with flying. */

/* Living Destiny	|3|G
 * Instant
 * As an additional cost to cast ~, reveal a creature card from your hand.
 * You gain life equal to the revealed card's converted mana cost. */

int card_might_of_the_masses(int player, int card, event_t event)
{
  /* Might of the Masses	|G
   * Instant
   * Target creature gets +1/+1 until end of turn for each creature you control. */
  int amt = (event == EVENT_CHECK_PUMP || event == EVENT_RESOLVE_SPELL) ? count_subtype(player, TYPE_CREATURE, -1) : 0;
  return vanilla_instant_pump(player, card, event, ANYBODY, player, amt, amt, 0, 0);
}

int card_momentous_fall(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int result = pick_creature_for_sacrifice(player, card, 0);
		if( result > -1 ){
			instance->targets[1].player = get_power(player, result);
			instance->targets[1].card = get_toughness(player, result);
			kill_card(player, result, KILL_SACRIFICE);
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, instance->targets[1].player);
		gain_life(player, instance->targets[1].card);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mul_daya_channelers(int player, int card, event_t event){

	reveal_top_card(player, card, event);

	int *deck = deck_ptr[player];

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		if( deck[0] != -1 && is_what(-1, deck[0], TYPE_CREATURE) ){
			event_result+=3;
		}
	}

	if( event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && ! is_sick(player, card) && deck[0] != -1 && is_what(-1, deck[0], TYPE_LAND) ){
		return can_produce_mana(player, card);
	}

	if( event == EVENT_ACTIVATE ){
		produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, 2);
	}

	if( event == EVENT_COUNT_MANA && ! is_tapped(player, card) && ! is_sick(player, card) &&
		deck[0] != -1 && is_what(-1, deck[0], TYPE_LAND) && affect_me(player, card)
	  ){
		declare_mana_available_hex(player, COLOR_TEST_ANY_COLORED, 2);
	}

	return 0;
}

/* Naturalize	|1|G => 0x4a2f10:Disenchant
 * Instant
 * Destroy target artifact or enchantment. */

/* Nema Siltlurker	|4|G (vnailla)
 * Creature - Lizard 3/5 */

int card_nest_invader(int player, int card, event_t event){
	/* Nest Invader	|1|G
	 * Creature - Eldrazi Drone 2/2
	 * When ~ enters the battlefield, put a 0/1 colorless Eldrazi Spawn creature token onto the battlefield. It has "Sacrifice this creature: Add |1 to your mana pool." */

	if( comes_into_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_ELDRAZI_SPAWN);
	}
	return 0;
}

/* Ondu Giant	|3|G (farhaven elf)
 * Creature - Giant Druid 2/4
 * When ~ enters the battlefield, you may search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library. */

int card_overgrown_battlement(int player, int card, event_t event){
	int count = 0;
	if (event == EVENT_COUNT_MANA || event == EVENT_ACTIVATE || (event == EVENT_CAN_ACTIVATE && player == AI)){	// Only count if it'll actually be needed
		count = count_defenders(player);
	}
	return mana_producing_creature(player, card, event, 24, COLOR_GREEN, count);
}

int card_pelakka_wurm(int player, int card, event_t event){

	/* Pelakka Wurm	|4|G|G|G
	 * Creature - Wurm 7/7
	 * Trample
	 * When ~ enters the battlefield, you gain 7 life.
	 * When ~ dies, draw a card. */

	if (comes_into_play(player, card, event)){
		gain_life(player, 7);
	}

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_snake_umbra(int player, int card, event_t event)
{
  /* Snake Umbra	|2|G
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +1/+1 and has "Whenever this creature deals damage to an opponent, you may draw a card."
   * Totem armor */

  if (IS_DDBM_EVENT(event))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int packets;
	  if (instance->damage_target_card >= 0 && !is_humiliated(instance->damage_target_player, instance->damage_target_card)
		  && (packets = equipped_creature_deals_damage(player, card, event, DDBM_MUST_DAMAGE_OPPONENT|DDBM_TRIGGER_OPTIONAL)))
		draw_cards(player, packets);
	}

  return totem_armor(player, card, event, 1, 1, 0, 0);
}

int card_spider_umbra(int player, int card, event_t event){

	return totem_armor(player, card, event, 1, 1, KEYWORD_REACH, 0);
}

/* Sporecap Spider	|2|G (vanilla)
 * Creature - Spider 1/5
 * Reach */

/* Stomper Cub	|3|G|G (vanilla)
 * Creature - Beast 5/3
 * Trample */

/* Tajuru Preserver	|1|G => Vanilla plus hack in sacrifices.c
 * Creature - Elf Shaman 2/1
 * Spells and abilities your opponents control can't cause you to sacrifice permanents. */

int card_vengevine(int player, int card, event_t event){
	haste(player, card, event);
	return 0;
}

/* Wildheart Invoker	|2|G|G
 * Creature - Elf Shaman 4/3
 * |8: Target creature gets +5/+5 and gains trample until end of turn. */

/*************
* Multicolor *
*************/

int card_sarkhan_the_mad(int player, int card, event_t event){

	/* Sarkhan the Mad	|3|B|R
	 * Planeswalker - Sarkhan (7)
	 * 0: Reveal the top card of your library and put it into your hand. ~ deals damage to himself equal to that card's converted mana cost.
	 * -2: Target creature's controller sacrifices it, then that player puts a 5/5 |Sred Dragon creature token with flying onto the battlefield.
	 * -4: Each Dragon creature you control deals damage equal to its power to target player. */

	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		target_definition_t td2;
		default_target_definition(player, card, &td2, TYPE_CREATURE);
		td2.preferred_controller = player;
		if( player == AI ){
			td2.power_requirement = 4 | TARGET_PT_LESSER_OR_EQUAL;
			td2.toughness_requirement = 4 | TARGET_PT_LESSER_OR_EQUAL;
		}

		target_definition_t td3;
		default_target_definition(player, card, &td3, TYPE_CREATURE);
		if( player == AI ){
			td3.power_requirement = 5 | TARGET_PT_GREATER_OR_EQUAL;
			td3.toughness_requirement = 5 | TARGET_PT_GREATER_OR_EQUAL;
		}

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_DRAGON;

		card_instance_t *instance = get_card_instance(player, card);

		int priority_you_sac = target_available(player, card, &td2) * 3;
		int priority_opp_sac = target_available(player, card, &td3) * 3;
		int can_sac = player == HUMAN ? can_target(&td2) : ((priority_you_sac && can_sacrifice(player, player, 1, TYPE_CREATURE, 0)) ||
															(priority_opp_sac && can_sacrifice(player, 1-player, 1, TYPE_CREATURE, 0)));
		int priority_dragon_power = check_battlefield_for_special_card(player, card, player, CBFSC_GET_TOTAL_POW, &this_test) * 2;

		enum {
			CHOICE_REVEAL = 1,
			CHOICE_SAC_TO_DRAGON,
			CHOICE_DRAGON_POWER
		}
		choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
						"Reveal & damage Sarkhan", 1, 10, 0,
						"Impose sacrifice", can_sac, priority_opp_sac > priority_you_sac ? priority_opp_sac : priority_you_sac, -2,
						"Dragon Power", 1, priority_dragon_power, -4);
		if (event == EVENT_CAN_ACTIVATE){
			if (!choice)
				return 0;	// else fall through to planeswalker()
		}

		else if (event == EVENT_ACTIVATE){
				instance->number_of_targets = 0;
				if( choice == CHOICE_SAC_TO_DRAGON ){
					if( player == AI ){
						if( priority_opp_sac > priority_you_sac ){
							pick_target(&td3, "TARGET_CREATURE");
						}
						else{
							pick_target(&td2, "TARGET_CREATURE");
						}
					}
					else{
						pick_target(&td, "TARGET_CREATURE");
					}
				}
		}

		else{	// event == EVENT_RESOLVE_ACTIVATION
			if( choice == CHOICE_REVEAL ){
				int *deck = deck_ptr[player];
				if( deck[0] != -1 ){
					show_deck( 1-player, deck, 1, "Sarkhan the Mad revealed...", 0, 0x7375B0 );
					int card_added = add_card_to_hand(player, deck[0]);
					remove_card_from_deck(player, 0);
					remove_counters(instance->parent_controller, instance->parent_card, COUNTER_LOYALTY, get_cmc(player, card_added));
				}
			}
			else if( choice == CHOICE_SAC_TO_DRAGON ){
					if( valid_target(&td) && can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
						kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
						generate_token_by_id(player, card, CARD_ID_DRAGON);
					}
			}
			else if( choice == CHOICE_DRAGON_POWER ){
					int c;
					for(c=active_cards_count[player]-1; c > -1; c--){
						if( in_play(player, c) && new_make_test_in_play(player, c, -1, &this_test) ){
							target_definition_t td4;
							default_target_definition(player, c, &td4, 0);
							td4.zone = TARGET_ZONE_PLAYERS;
							td4.allow_cancel = 0;
							if( can_target(&td4) && pick_target(&td4, "TARGET_PLAYER") ){
								damage_target0(player, c, get_power(player, c));
								get_card_instance(player, c)->number_of_targets = 0;
							}
						}
					}
			}
		}
	}
	return planeswalker(player, card, event, 7);
}

/***********
* Artifact *
***********/

/* Angelheart Vial	|5
 * Artifact
 * Whenever you're dealt damage, you may put that many charge counters on ~.
 * |2, |T, Remove four charge counters from ~: You gain 2 life and draw a card. */

int card_dreamstone_hedron(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		return can_produce_mana(player, card);
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( ! paying_mana() && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 6, 0, 0, 0, 0, 0) &&
			can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0)
		  ){
			ai_choice = 1;
		}
		if( ai_choice == 1 ){
			choice =  do_dialog(player, player, card, -1, -1, " Get mana\n Sac & draw 3 card\n Cancel", 0);
		}
		if( choice == 0 ){
			produce_mana_tapped(player, card, COLOR_COLORLESS, 3);
		}
		else if( choice == 1 ){
			instance->state |= STATE_TAPPED;
			charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
				kill_card(player, card, KILL_SACRIFICE);
			} else {
				instance->state &= ~STATE_TAPPED;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			draw_cards(player, 3);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
		if( can_produce_mana(player, card) ){
			declare_mana_available(player, COLOR_COLORLESS, 3);
		}
	}

	return 0;
}

/* Enatu Golem	|6
 * Artifact Creature - Golem 3/5
 * When ~ dies, you gain 4 life. */

int card_hedron_matrix(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( in_play(player, card) && is_equipping(player, card) ){
		int amount = get_cmc(instance->targets[8].player, instance->targets[8].card);
		modify_pt_and_abilities(instance->targets[8].player, instance->targets[8].card, event, amount, amount, 0);
	}

	return basic_equipment(player, card, event, 4);
}

int card_keening_stone(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int amount = count_graveyard(instance->targets[0].player);
			mill(instance->targets[0].player, amount);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 5, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int card_ogres_cleaver(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 5, 5, 0, 0, 0);
}

int card_pennon_blade(int player, int card, event_t event)
{
  /* Pennon Blade	|3
   * Artifact - Equipment
   * Equipped creature gets +1/+1 for each creature you control.
   * Equip |4 */

  card_instance_t* instance;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && (instance = in_play(player, card))
	  && affect_me(instance->damage_target_player, instance->damage_target_card))
	event_result += count_subtype(player, TYPE_CREATURE, -1);

  return basic_equipment(player, card, event, 4);
}

int card_prophetic_prism(int player, int card, event_t event)
{
  // When ~ enters the battlefield, draw a card.
  if (comes_into_play(player, card, event))
	draw_a_card(player);

  // |1, |T: Add one mana of any color to your mana pool.
  if (event == EVENT_CAN_ACTIVATE)
	return (can_produce_mana(player, card) && !is_tapped(player, card) && !is_animated_and_sick(player, card)
			&& has_mana(player, COLOR_COLORLESS, 1) && get_card_instance(player, card)->targets[1].player != 66);

  if (event == EVENT_ACTIVATE)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  instance->targets[1].player = 66;
	  charge_mana(player, COLOR_COLORLESS, 1);
	  instance->targets[1].player = 0;
	  if (spell_fizzled != 1)
		produce_mana_tapped_all_one_color(player, card, COLOR_TEST_ANY_COLORED, 1);
	}

  return 0;
}

/* Reinforced Bulwark	|3
 * Artifact Creature - Wall 0/4
 * Defender
 * |T: Prevent the next 1 damage that would be dealt to you this turn. */

/* Runed Servitor	|2
 * Artifact Creature - Construct 2/2
 * When ~ dies, each player draws a card. */

int card_sphinx_bone_wand(int player, int card, event_t event){

	/* Sphinx-Bone Wand	|7
	 * Artifact
	 * Whenever you cast an instant or sorcery spell, you may put a charge counter on ~. If you do, ~ deals damage equal to the number of charge counters on it
	 * to target creature or player. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if (specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_SPELL,0, 0,0, 0,0, 0,0, -1,0)){
		add_counter(player, card, COUNTER_CHARGE);
		instance->number_of_targets = 0;
		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER")){
			damage_creature(instance->targets[0].player, instance->targets[0].card, count_counters(player, card, COUNTER_CHARGE), player, card);
		}
	}

	return 0;
}

int card_warmongers_chariot(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (event == EVENT_ABILITIES && is_equipping(player, card) && affect_me(instance->targets[8].player, instance->targets[8].card))
	get_card_instance(instance->targets[8].player, instance->targets[8].card)->token_status |= STATUS_WALL_CAN_ATTACK;

  return vanilla_equipment(player, card, event, 3, 2, 2, 0, 0);
}

/*******
* Land *
*******/

int card_eldrazi_temple(int player, int card, event_t event){
	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if(has_subtype(affected_card_controller, affected_card, SUBTYPE_ELDRAZI )
			&& is_colorless(affected_card_controller, affected_card)
			&& ! is_tapped(player, card) )
		{
			COST_COLORLESS--;
		}
	}
	return mana_producer(player, card, event);
}

/* Evolving Wilds	"" => m11.c:Terramorphic Expanse
 * Land
 * |T, Sacrifice ~: Search your library for a basic land card and put it onto the battlefield tapped. Then shuffle your library. */


