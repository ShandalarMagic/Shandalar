#include "manalink.h"

/* Cards that were not coded since they have clones already:
1) Arc Runner -> Ball Lightning
2) Canyon Minotaur -> Hill Giant
3) Fiery Hellhound -> Shivan Dragon
4) Goblin Piker -> Hill Giant
5) Maniac Vandal -> Viridian Shaman
6) Reverberate -> Fork (too lazy for to code the miniscule difference)
7) Vulshok Berserker -> Raging Goblin
8) Stone Golem -> Stonework Puma
9) Autumn's Veil -> skipped (impossible)
10) Back to Nature -> Tranquility
11) Cudgel Troll -> Wall of Brambles
12) Duskdale Wurm -> Craw Wurm
13) Garruk's Companion -> Craw Wurm
14) Obstinate Baloth -> Staunch Defenders
15) Plummet -> Wing Snare
16) Runeclaw Bear -> Grizzly Bear
17) Sylvan Ranger -> Civic Wayfinder
18) Wall of Vines -> Wall of Wood
19) Yavimaya Wurm -> Craw Wurm
20) Aether Adept -> Man O war
21) Armored Cancrix -> Aie Elemental
22) Cloud Elemental -> Welkin Tern
23) Jaces Ingenuity -> Concentrate
24) Leyline of Anticipation -> skipped (impossible)
25) Maritime Guard -> Air Elemental
26) Mind Control -> Control Magic
27) Redirect -> skipped (impossible)
28) Scroll Thief -> Thieving Magpie
*/


/* WHITE CARDS */

//Ajani's Mantra
int card_ajanis_mantra(int player, int card, event_t event){
	/* Ajani's Mantra	|1|W
	 * Enchantment
	 * At the beginning of your upkeep, you may gain 1 life. */

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_DUH);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		gain_life( player, 1 );
	}

	return global_enchantment(player, card, event);
}

//Ajani's Pridemate
int card_ajanis_pridemate(int player, int card, event_t event)
{
  if (trigger_gain_life(player, card, event, player, RESOLVE_TRIGGER_DUH))
	add_1_1_counter(player, card);

  return 0;
}

//Angelic Arbiter
int card_angelic_arbiter(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( (event == EVENT_CLEANUP || event == EVENT_CAST_SPELL) && affect_me(player, card) ){
		instance->targets[1].card = 0;
	}

	if( current_turn != player ){
		//CHECK IF OPPONENT IS ATTACKING
		if (event == EVENT_DECLARE_ATTACKERS && current_turn == 1-player && count_attackers(current_turn) > 0){
			instance->targets[1].card |= 1;
		}
		//INCREASING THE COST IF OPPONENT ATTACKED THIS TURN
		if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == 1-player && ! is_humiliated(player, card) ){
			if( ! is_what(affected_card_controller, affected_card, TYPE_LAND) && (instance->targets[1].card & 1) ){
				infinite_casting_cost();
			}
		}

		//CHECK IF OPPONENT IS CASTING A SPELL
		if( !(instance->targets[1].card & 2) && specific_spell_played(player, card, event, 1-player, 2, TYPE_LAND, 1, 0, 0, 0, 0, 0, 0, -1, 0) ){
			instance->targets[1].card |= 2;
		}

		//MAKING IT IMPOSSIBLE TO ATTACK IF SPELL WAS PLAYED THIS TURN
		if( (instance->targets[1].card & 2) && ! is_humiliated(player, card) ){
			nobody_can_attack(player, card, event, 1-player);
		}
	}


	return 0;
}

int card_armored_ascension(int player, int card, event_t event)
{
  /* Armored Ascension	|3|W
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets +1/+1 for each |H2Plains you control and has flying. */

  card_instance_t* instance;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_ABILITIES)
	  && (instance = in_play(player, card))
	  && ! is_humiliated(player, card)
	  && affect_me(instance->damage_target_player, instance->damage_target_card))
	{
	  if (event == EVENT_ABILITIES)
		event_result |= KEYWORD_FLYING;
	  else
		event_result += count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_PLAINS));
	}

  return vanilla_aura(player, card, event, player);
}

//Celestial Purge
int card_celestial_purge(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK) | get_sleighted_color_test(player, card, COLOR_TEST_RED);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE );
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						get_sleighted_color_text2(player, card, "Select a %s or %s creature card.", COLOR_TEST_BLACK, COLOR_TEST_RED), 1, NULL);
}

//Excommunicate
int card_excommunicate(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

//Goldenglow Moth
int card_goldenglow_moth(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS && get_card_instance(player, card)->blocking < 255 && ! is_humiliated(player, card) ){
		if( do_dialog(player, player, card, -1, -1, " Gain life\n Pass", 0) == 0 ){
			gain_life(player, 4 );
		}
	}

	return 0;
}

//Infantry Veteran
int card_infantry_veteran(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION){
		if( valid_target( &td ) ){
			pump_until_eot(player, instance->parent_card, player, instance->targets[0].card, 1, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target attacking creature.");
}


//Inspired Charge
int card_inspired_charge(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		pump_subtype_until_eot(player, card, player, -1, 2, 1, 0, 0);
		kill_card(player, card, KILL_DESTROY );
	}

	return basic_spell(player, card, event);
}

int card_knight_exemplar(int player, int card, event_t event)
{
  /* Knight Exemplar	|1|W|W
   * Creature - Human Knight 2/2
   * First strike
   * Other Knight creatures you control get +1/+1 and are indestructible. */

  boost_subtype(player, card, event, SUBTYPE_KNIGHT, 1,1, 0,SP_KEYWORD_INDESTRUCTIBLE, BCT_CONTROLLER_ONLY);
  return 0;
}

//Leyline of Sanctity
int card_leyline_of_sanctity(int player, int card, event_t event){
	give_hexproof_to_player(player, card, event);
	return global_enchantment(player, card, event);
}

int card_mighty_leap(int player, int card, event_t event){
	return vanilla_instant_pump(player, card, event, player, player, 2, 2, KEYWORD_FLYING, 0);
}

//Roc Egg
int card_roc_egg(int player, int card, event_t event){
	/* Roc Egg	|2|W
	 * Creature - Bird 0/3
	 * Defender
	 * When ~ dies, put a 3/3 |Swhite Bird creature token with flying onto the battlefield. */

	if( graveyard_from_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_ROC);
	}
	return 0;
}


//Safe Passage
static int effect_safe_passage(int player, int card, event_t event){

	// prevent any damage that would be dealt to me
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card && ! check_state(affected_card_controller, affected_card, STATE_CANNOT_TARGET) ){
			if( damage->damage_target_player == player && damage->info_slot > 0 ){
				if( damage->targets[4].player == -1 && damage->targets[4].card == -1 ){
					damage->info_slot = 0;
				}
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_SACRIFICE );
	}

	return 0;
}

int card_safe_passage(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, &effect_safe_passage);
		kill_card(player, card, KILL_DESTROY );
	}

	return basic_spell(player, card, event);
}

//Serra Ascendant
int card_serra_ascendant(int player, int card, event_t event){
	if ((event == EVENT_ABILITIES || event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) &&
		!is_humiliated(player, card) && in_play(player, card)
	   ){
		lifelink(player, card, event);
		if (life[player] >= 30){
			modify_pt_and_abilities(player, card, event, 5, 5, KEYWORD_FLYING);
		}
	}
	return 0;
}

//Silence
int card_silence(int player, int card, event_t event ){

	if( event == EVENT_RESOLVE_SPELL ){
		int spc = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//64
		card_instance_t *leg = get_card_instance(player, spc);
		leg->targets[2].player = 8;
		leg->targets[3].player = 1-player;
		leg->targets[2].card = get_id(player, card);
		create_card_name_legacy(player, spc, get_id(player, card));
		kill_card(player, card, KILL_DESTROY );
	}

	return basic_spell(player, card, event);
}

//Solemn Offering
int card_solemn_offering(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if(validate_target(player, card, &td, 0)){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			gain_life( player, 4 );
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "DISENCHANT", 1, NULL);
}

//Squadron Hawk
int card_squadron_hawk(int player, int card, event_t event){

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		char buffer[100];
		card_ptr_t* c = cards_ptr[ get_id(player, card) ];
		scnprintf(buffer, 100, "Select a card named %s.", c->name);

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, buffer);
		this_test.id = get_id(player, card);
		this_test.qty = 3;

		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 10+AI_FIRST_FOUND, &this_test);

		shuffle(player);
	}

	return 0;
}

//Sun Titan
int card_sun_titan(int player, int card, event_t event){

	vigilance(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a permanent with CMC 3 or less.");
		this_test.cmc = 4;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

		int can_tutor = new_special_count_grave(player, &this_test) > 0 && ! graveyard_has_shroud(player);

		if( comes_into_play_mode(player, card, event, can_tutor ? RESOLVE_TRIGGER_AI(player) : 0) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		}
	}

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a permanent with CMC 3 or less.");
		this_test.cmc = 4;
		this_test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;

		int can_tutor = new_special_count_grave(player, &this_test) > 0 && ! graveyard_has_shroud(player);

		if( can_tutor ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

//Tireless Missionaries
int card_tireless_missionaries(int player, int card, event_t event){
	if( comes_into_play(player, card, event) > 0 ){
		gain_life( player, 3 );
	}
	return 0;
}

int card_vengeful_archon(int player, int card, event_t event){
	/*
	  Vengeful Archon |4|W|W|W
	  Creature - Archon 7/7
	  Flying
	  {X}: Prevent the next X damage that would be dealt to you this turn.
	  If damage is prevented this way, Vengeful Archon deals that much damage to target player.
	*/

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.extra = damage_card;
	td.illegal_abilities = 0;
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_DAMAGE_PREVENTION | GAA_CAN_TARGET, MANACOST_X(1), 0, &td, NULL) ){
			if( can_target(&td1) ){
				return 99;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = instance->info_slot = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( pick_target(&td, "TARGET_DAMAGE") ){
				card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				int amount = dmg->info_slot;
				if( player != AI ){
					charge_mana(player, COLOR_COLORLESS, -1);
					if( spell_fizzled !=1 ){
						instance->info_slot = x_value;
					}
				}
				else{
					while( ! has_mana(player, COLOR_COLORLESS, amount) ){
							amount--;
					}
					charge_mana(player, COLOR_COLORLESS, amount);
					if( spell_fizzled !=1 ){
						instance->info_slot = amount;
					}
				}
				if( spell_fizzled != 1 ){
					new_pick_target(&td1, "TARGET_PLAYER", 1, 1);
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *dmg = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			int prevented = 0;
			while( instance->info_slot > 0 && dmg->info_slot > 0 ){
					instance->info_slot--;
					dmg->info_slot--;
					prevented++;
			}
			if( prevented > 0 && validate_target(player, card, &td1, 1) ){
				damage_player(instance->targets[1].player, prevented, instance->parent_controller, instance->parent_card);
			}
		}
	}

	return 0;
}

//War Priest of Thune
int card_war_priest_of_thune(int player, int card, event_t event){
	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_ENCHANTMENT);
		if(target_available(player, card, &td)){
			if( pick_target( &td, "TARGET_ENCHANTMENT") ){
				if(validate_target(player, card, &td, 0)){
					card_instance_t *instance = get_card_instance(player, card);
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
					instance->number_of_targets = 0;
				}
			}
		}
	}
	return 0;
}

/* BLACK CARDS */
//Blood Tithe
int card_blood_tithe(int player, int card, event_t event ){

	if( event == EVENT_RESOLVE_SPELL ){
		int result = lose_life(1-player, 3);
		gain_life( player, result );
		kill_card(player, card, KILL_DESTROY );
	}

	return basic_spell(player, card, event);
}

//Captivating Vampire
int card_captivating_vampire(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_abilities = 0;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_subtype = SUBTYPE_VAMPIRE;
	td.illegal_state = TARGET_STATE_TAPPED;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( target_available(player, card, &td) > 4 ){
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL);
		}
	}

	if( event == EVENT_ACTIVATE){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( tapsubtype_ability(player, card, 5, &td) ){
				if( pick_target(&td1, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			if( ! has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_VAMPIRE) ){
				add_a_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_VAMPIRE);
			}
			gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return boost_creature_type(player, card, event, SUBTYPE_VAMPIRE, 1, 1, 0, BCT_CONTROLLER_ONLY);
}

//Deathmark
int card_deathmark(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN) | get_sleighted_color_test(player, card, COLOR_TEST_WHITE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						get_sleighted_color_text2(player, card, "Select a %s or %s creature.", COLOR_TEST_GREEN, COLOR_TEST_WHITE), 1, NULL);
}

//Demon of Death's Gate
int card_demon_of_deaths_gate(int player, int card, event_t event){

	if( event == EVENT_MODIFY_COST ){
		int good = 0;
		if( can_sacrifice_as_cost(player, 3, TYPE_CREATURE, 0, 0, 0, COLOR_TEST_BLACK, 0, 0, 0, -1, 0) > 2 && can_pay_life(player, 6) ){
			good = 1;
		}
		if( good == 1 ){
			COST_COLORLESS -= 6;
			COST_BLACK -= 3;
		}
	}
	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.required_color = COLOR_TEST_BLACK;
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.illegal_abilities = 0;

			int choice = 1;
			int ai_choice = 1;
			if( target_available(player, card, &td) > 2 && can_pay_life(player, 6) ){
				if( player == AI && life[player]-6 > 5 ){
					ai_choice = 0;
				}
				choice = do_dialog(player, player, card, -1, -1, " Sac 3 Guys and lose 6 life\n Play normally\n Cancel", ai_choice);
			}

			if( choice == 0 ){
				test_definition_t test;
				new_default_test_definition(&test, TYPE_CREATURE, "Select a black creature to sacrifice.");
				test.color = COLOR_TEST_BLACK;
				int sacced[3];
				int sc = 0;
				while( sc < 3 ){
						int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
						if (!sac){
							cancel = 1;
							return 0;
						}
						sacced[sc] = BYTE3(sac);
						state_untargettable(BYTE2(sac), BYTE3(sac), 1);
						sc++;
				}
				int i;
				for(i=0; i<sc; i++){
					state_untargettable(player, sacced[i], 0);
				}
				if( sc == 3 ){
					for(i=0; i<sc; i++){
						kill_card(player, sacced[i], KILL_SACRIFICE);
					}
					lose_life(player, 6);
				}
			}
			else if( choice == 1 ){
					charge_mana_from_id(player, card, event, -1);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	return 0;
}


//Doom Blade
int card_doom_blade(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.illegal_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY );
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						get_sleighted_color_text(player, card, "Select a non%s or creature.", COLOR_TEST_BLACK), 1, NULL);
}


//Grave Titan
int card_grave_titan(int player, int card, event_t event){
	/* Grave Titan	|4|B|B
	 * Creature - Giant 6/6
	 * Deathtouch
	 * Whenever ~ enters the battlefield or attacks, put two 2/2 |Sblack Zombie creature tokens onto the battlefield. */

	deathtouch(player, card, event);

	if (comes_into_play(player, card, event) || declare_attackers_trigger(player, card, event, 0, player, card)){
		generate_tokens_by_id(player, card, CARD_ID_ZOMBIE, 2);
	}

	return 0;
}


//Howling Banshee
int card_howling_banshee(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		APNAP(p, {lose_life(p, 3);};);
	}

	return 0;
}

int card_lilianas_specter(int player, int card, event_t event){
	/*
	  Liliana's Specter |1|B|B
	  Creature - Specter 2/1
	  Flying
	  When Liliana's Specter enters the battlefield, each opponent discards a card.
	*/
	if( comes_into_play(player, card, event) ){
		discard(1-player, 0, player);
	}

	return 0;
}

static int necrotic_plague_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		const int *grave = get_grave(instance->targets[1].player);
		int count = count_graveyard(instance->targets[1].player)-1;
		while( count > -1 ){
				if( cards_data[grave[count]].id == instance->targets[1].card ){
					int card_added = add_card_to_hand(instance->targets[1].player, grave[count]);
					set_special_flags3(instance->targets[1].player, card_added, SF3_REANIMATED);
					card_instance_t *this = get_card_instance(instance->targets[1].player, card_added);
					this->targets[0] = instance->targets[0];
					set_special_flags(instance->targets[1].player, card_added, SF_TARGETS_ALREADY_SET);
					put_into_play(instance->targets[1].player, card_added);
					break;
				}
				count--;
		}
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_necrotic_plague(int player, int card, event_t event){
	/*
	  Necrotic Plague |2|B|B
	  Enchantment - Aura
	  Enchant creature
	  Enchanted creature has "At the beginning of your upkeep, sacrifice this creature."
	  When enchanted creature dies, its controller chooses target creature one of his or her opponents controls.
	  Return Necrotic Plague from its owner's graveyard to the battlefield attached to that creature.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( instance->damage_target_player > -1 && in_play(player, card) && ! is_humiliated(player, card) ){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;
		if( this_dies_trigger(t_player, t_card, event, RESOLVE_TRIGGER_MANDATORY) ){
			target_definition_t td;
			default_target_definition(t_player, t_card, &td, TYPE_CREATURE );
			td.who_chooses = t_player;
			td.allowed_controller = 1-t_player;
			td.preferred_controller = 1 - t_player;
			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				int legacy = create_legacy_effect(player, card, &necrotic_plague_legacy);
				card_instance_t *leg = get_card_instance(player, legacy);
				leg->targets[0] = instance->targets[0];
				leg->targets[1].player = get_owner(player, card);
				leg->targets[1].card = get_original_id(player, card);
				leg->targets[11].player = 66;
			}
		}
		if( current_turn == t_player && upkeep_trigger(player, card, event) ){
			if( can_sacrifice(player, t_player, 1, TYPE_CREATURE, 0) ){
				kill_card(t_player, t_card, KILL_SACRIFICE);
			}
		}
	}

	return disabling_aura(player, card, event);
}

//Nightwing Shade
int card_nightwing_shade(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, MANACOST_XB(1, 1), 1, 1, 0, 0);
}

int card_phylactery_lich(int player, int card, event_t event)
{
  /* Phylactery Lich	|B|B|B
   * Creature - Zombie 5/5
   * As ~ enters the battlefield, put a phylactery counter on an artifact you control.
   * ~ is indestructible.
   * When you control no permanents with phylactery counters on them, sacrifice ~. */

  indestructible(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	{
	  target_definition_t td;
	  base_target_definition(player, card, &td, TYPE_ARTIFACT);
	  td.allowed_controller = player;
	  td.preferred_controller = player;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  if (can_target(&td) && pick_target(&td, "TARGET_ARTIFACT"))
		add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_PHYLACTERY);

	  instance->number_of_targets = 0;
	}

  if (event == EVENT_STATIC_EFFECTS && ! is_humiliated(player, card) && !has_any_counters(player, COUNTER_PHYLACTERY, TYPE_PERMANENT))
	kill_card(player, card, KILL_SACRIFICE);

  return 0;
}

int card_quag_sickness(int player, int card, event_t event)
{
  /* Quag Sickness	|2|B
   * Enchantment - Aura
   * Enchant creature
   * Enchanted creature gets -1/-1 for each |H2Swamp you control. */

  card_instance_t* instance;
  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS)
	  && (instance = in_play(player, card))
	  && affect_me(instance->damage_target_player, instance->damage_target_card))
	event_result -= count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_SWAMP));

  return disabling_aura(player, card, event);
}

int card_reassembling_skeleton(int player, int card, event_t event){
	if(event == EVENT_GRAVEYARD_ABILITY){
		if( has_mana_multi( player, MANACOST_XB(1, 1) ) ){
			return GA_RETURN_TO_PLAY_MODIFIED;
		}
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi( player, MANACOST_XB(1, 1) );
		if( spell_fizzled != 1){
			return GAPAID_REMOVE;
		}
	}
	if( event == EVENT_RETURN_TO_PLAY_FROM_GRAVE_MODIFIED ){
		add_state(player, card, STATE_TAPPED);
	}
	return 0;
}

int card_relentless_rats(int player, int card, event_t event){
	if(affect_me(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
			int result = count_cards_by_id( player, get_id(player, card) );
			result += count_cards_by_id( 1-player, get_id(player, card) );
			result--;

			event_result += result;
		}
	}
	return 0;
}

//Rise from the Grave
int card_rise_from_the_grave(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			reanimate_permanent(player, card, instance->targets[0].player, selected, REANIMATE_ADD_BLACK_ZOMBIE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER_BOTH_GRAVES, NULL, NULL, 1, &this_test);
}

//Rotting Legion
int card_rotting_legion(int player, int card, event_t event){
	comes_into_play_tapped(player, card, event);
	return 0;
}


int card_sign_in_blood(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = life[player]-2 < 6 ? 1-player : player;

	card_instance_t *this_instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(this_instance->targets[0].player, 2);
			lose_life(this_instance->targets[0].player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

//Stabbing Pain
int card_stabbing_pain(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

//Viscera Seer
int card_viscera_seer(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		scry(player, 1);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST0, 0, NULL, NULL);
}

/* LANDS */

int m10_lands(int player, int card, event_t event, int sub1, int sub2){

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! check_state(player, card, STATE_TAPPED) ){
			int tap = 1;
			int i;
			for(i=0; i<active_cards_count[player]; i++){
				if( in_play(player, i) && (has_subtype(player, i, sub1) || has_subtype(player, i, sub2)) ){
					tap = 0;
					break;
				}
			}
			if( tap == 1 ){
				get_card_instance(player, card)->state |= STATE_TAPPED;	// avoid sending event
			}
		}
	}

	return mana_producer(player, card, event);
}


int card_dragonskull_summit(int player, int card, event_t event){
	return m10_lands(player, card, event, SUBTYPE_SWAMP, SUBTYPE_MOUNTAIN);
}

int card_drowned_catacomb(int player, int card, event_t event){
	return m10_lands(player, card, event, SUBTYPE_SWAMP, SUBTYPE_ISLAND);
}

int card_glacial_fortress(int player, int card, event_t event){
	return m10_lands(player, card, event, SUBTYPE_PLAINS, SUBTYPE_ISLAND);
}

int card_rootbound_crag(int player, int card, event_t event){
	return m10_lands(player, card, event, SUBTYPE_FOREST, SUBTYPE_MOUNTAIN);
}

int card_sunpetal_grove(int player, int card, event_t event){
	return m10_lands(player, card, event, SUBTYPE_FOREST, SUBTYPE_PLAINS);
}

int card_mystifying_maze(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) || event == EVENT_CAN_ACTIVATE ){
		return mana_producer(player, card, event);
	}

	card_instance_t *instance = get_card_instance(player, card);
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;
	td.allowed_controller = 1-player;
	td.preferred_controller = 1-player;

	if(event == EVENT_ACTIVATE ){
		int choice = instance->number_of_targets = instance->info_slot = 0;
		if( !paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_X(5), 0, &td, NULL) ){
			choice = do_dialog(player, player, card, -1, -1, " Get mana\n Exile an Attacker\n Cancel", 0);
		}
		if( choice == 0 ){
			return mana_producer(player, card, event);
		}
		else if( choice == 1 ){
				add_state(player, card, STATE_TAPPED);
				if( charge_mana_for_activated_ability(player, card, MANACOST_X(4)) &&
					new_pick_target(&td, "Select target creature your opponent controls", 0, 1 | GS_LITERAL_PROMPT)
				  ){
					tap_card(player, card);
					instance->info_slot = 1;
				}
				else{
					remove_state(player, card, STATE_TAPPED);
					spell_fizzled = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && can_target(&td) ){
			remove_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_terramorphic_expanse(int player, int card, event_t event){
	// Also code for Evolving Wilds

	if( event == EVENT_RESOLVE_SPELL ){
		play_land_sound_effect(player, card);
	}

	if( event == EVENT_ACTIVATE ){
		ai_modifier+=100;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
		this_test.subtype = SUBTYPE_BASIC;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

/* RED CARDS */
int card_ancient_hellkite(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) || ! is_attacking(player, card) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = opp;
	td.preferred_controller = opp;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 1, instance->parent_controller, instance->parent_card );
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_R(1), 0,
									&td, "Select target creature your opponent controls.");
}

int card_bloodcrazed_goblin(int player, int card, event_t event){
	if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && ! is_humiliated(player, card) && get_trap_condition(opp, TRAP_DAMAGE_TAKEN) <= 0 ){
		event_result = 1;
	}
	return 0;
}

int card_chandras_outrage(int player, int card, event_t event){

	/* Chandra's Outrage	|2|R|R
	 * Instant
	 * ~ deals 4 damage to target creature and 2 damage to that creature's controller. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
			damage_player(instance->targets[0].player, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_chandras_spitfire(int player, int card, event_t event)
{
  card_instance_t* damage = noncombat_damage_being_dealt(event);
  if (damage
	  && damage->damage_target_card == -1 && !check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER)
	  && damage->damage_target_player != player)
	++get_card_instance(player, card)->info_slot;

  if (trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->info_slot <= 0 || is_humiliated(player, card) )
		return 0;

	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  pump_until_eot(player, card, player, card, 3 * instance->info_slot, 0);
		  instance->info_slot = 0;
		}
	}

  return 0;
}

int card_combust(int player, int card, event_t event){

	cannot_be_countered(player, card, event);

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE) | get_sleighted_color_test(player, card, COLOR_TEST_WHITE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		state_untargettable(player, card, 0);
		if( valid_target(&td) ){
			int legacy = create_legacy_effect(player, card, &my_damage_cannot_be_prevented);
			get_card_instance(player, legacy)->targets[0].player = player;
			get_card_instance(player, legacy)->targets[0].card = card;
			damage_creature(instance->targets[0].player, instance->targets[0].card, 5, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						get_sleighted_color_text2(player, card, "Select a %s or %s creature.", COLOR_TEST_BLUE, COLOR_TEST_WHITE), 1, NULL);
}

int card_cyclops_gladiator( int player, int card, event_t event){
	/* Whenever ~ attacks, you may have it deal damage equal to its power to target creature defending player controls. If you do, that creature deals damage
	 * equal to its power to ~. */
	if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;

		card_instance_t* instance = get_card_instance(player, card);
		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS") ){
			int my_power = get_power(player, card);
			int their_power = get_power(instance->targets[0].player, instance->targets[0].card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, my_power, player, card);
			damage_creature(player, card, their_power, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_destructive_force(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance(player, card);
		int to_be_sacced[2][100];
		int tbsc[2] = {0, 0};
		APNAP(p, {
					// sac a land
					target_definition_t td;
					default_target_definition(player, card, &td, TYPE_LAND);
					td.illegal_abilities = 0;
					td.allowed_controller = p;
					td.preferred_controller = p;
					td.who_chooses = p;
					td.allow_cancel = 0;

					while( tbsc[p] < 5 && can_target(&td) ){
							instance->number_of_targets = 0;
							if( new_pick_target(&td, "Select a land you control.", 0, GS_LITERAL_PROMPT) ){
								to_be_sacced[p][tbsc[p]] = instance->targets[0].card;
								tbsc[p]++;
								state_untargettable(p, instance->targets[0].card, 1);
							}
					};
				};
		);

		APNAP(p, {
					int i;
					for(i=0; i<tbsc[p]; i++){
						kill_card(p, to_be_sacced[p][i], KILL_SACRIFICE);
					};
				};
		);

		APNAP(p, {new_damage_all(player, card, p, 5, NDA_ALL_CREATURES, NULL);};);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_earth_servant(int player, int card, event_t event){
	if( event == EVENT_TOUGHNESS && affect_me(player, card) && ! is_humiliated(player, card) ){
		event_result += count_subtype(player, TYPE_LAND, SUBTYPE_MOUNTAIN);
	}
	return 0;
}

int card_ember_hauler(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE );
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION  ){
		if( valid_target(&td1) ){
			damage_target0(player, card, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_X(1), 0, &td1, "TARGET_CREATURE_OR_PLAYER");
}

int card_fire_servant(int player, int card, event_t event)
{
  card_instance_t* damage = damage_being_dealt(event);
  if (damage
	  && damage->damage_source_player == player
	  && (damage->initial_color & get_sleighted_color_test(player, card, COLOR_TEST_RED))
	  && (damage->targets[3].player & TYPE_SPELL))
	damage->info_slot *= 2;

  return 0;
}

int card_fling(int player, int card, event_t event){

	/* Fling	|1|R
	 * Instant
	 * As an additional cost to cast ~, sacrifice a creature.
	 * ~ deals damage equal to the sacrificed creature's power to target creature or player. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET | GS_SAC_CREATURE_AS_COST, &td1, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (!sac){
			cancel = 1;
			return 0;
		}
		if( pick_target(&td1, "TARGET_CREATURE_OR_PLAYER") ){
			instance->info_slot = get_power(BYTE2(sac), BYTE3(sac));
			kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
		}
		else{
			state_untargettable(BYTE2(sac), BYTE3(sac), 1);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td1) ){
			damage_target0(player, card, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return 0;
}

int card_goblin_chieftain(int player, int card, event_t event)
{
  haste(player, card, event);

  // Other Goblin creatures you control get +1/+1 and have haste.
  boost_subtype(player, card, event, SUBTYPE_GOBLIN, 1,1, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY);

  return 0;
}

int card_hoarding_dragon(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an artifact card.");
		instance->targets[0].card = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
	}

	if( instance->targets[0].card > -1 && this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( check_rfg(player, instance->targets[0].card) ){
			add_card_to_hand(player, get_internal_card_id_from_csv_id( instance->targets[0].card ) );
		}
	}

	return 0;
}

static int effect_incite(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[0].card > -1 ){
		if( event == EVENT_SET_COLOR && affect_me(instance->targets[0].player, instance->targets[0].card) ){
			event_result = 1 << (COLOR_RED ) ;
		}
		attack_if_able(instance->targets[0].player, instance->targets[0].card, event);
	}
	if( eot_trigger(player, card, event ) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_incite(int player, int card, event_t event){

	/* Incite	|R
	 * Instant
	 * Target creature becomes |Sred until end of turn and attacks this turn if able. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, card, &effect_incite, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY );
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}


int card_inferno_titan(int player, int card, event_t event){
	/* Inferno Titan	|4|R|R
	 * Creature - Giant 6/6
	 * |R: ~ gets +1/+0 until end of turn.
	 * Whenever ~ enters the battlefield or attacks, it deals 3 damage divided as you choose among one, two, or three target creatures and/or players. */

	if (comes_into_play(player, card, event) || declare_attackers_trigger(player, card, event, 0, player, card)){
		target_and_divide_damage(player, card, NULL, NULL, 3);
	}

	return generic_shade_merge_pt(player, card, event, 0, MANACOST_R(1), 1, 0);
}

int card_leyline_of_punishment(int player, int card, event_t event){
	// tentative for 'damage_cannot_be_prevented'
	if( event == EVENT_PREVENT_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id ){
			state_untargettable(affected_card_controller, affected_card, 1);
		}
	}
	return global_enchantment(player, card, event);
}

int card_magma_phoenix(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		APNAP(p, {new_damage_all(player, card, p, 3, NDA_ALL_CREATURES | NDA_PLAYER_TOO, &this_test);};);
	}
	if( event == EVENT_GRAVEYARD_ABILITY && has_mana_multi(player, MANACOST_XR(3, 2)) ){
		return GA_RETURN_TO_HAND;
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XR(3, 2));
		if( spell_fizzled != 1 ){
			return GAPAID_REMOVE;
		}
	}
	return 0;
}

int card_pyretic_ritual(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		produce_mana(player, COLOR_RED, 3);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_thunder_strike(int player, int card, event_t event){
	return vanilla_instant_pump(player, card, event, player, player, 2, 0, KEYWORD_FIRST_STRIKE, 0);
}

int card_volcanic_strength(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 2, 2, get_hacked_walk(player, card, KEYWORD_MOUNTAINWALK), 0, 0, 0, 0);
}

int card_wild_evocation(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, current_turn);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( hand_count[current_turn] > 0 ){
			int p = current_turn;
			int p_hand[100];
			int ph_count = 0;
			int i;
			for(i=0;i<active_cards_count[p]; i++){
				if( in_hand(p, i)  ){
					card_instance_t *instance = get_card_instance(p, i);
					p_hand[ph_count] = instance->internal_card_id;
					ph_count++;
				}
			}
			int rnd = internal_rand(ph_count);
			for(i=0;i<active_cards_count[p]; i++){
				if( in_hand(p, i)  ){
					card_instance_t *instance = get_card_instance(p, i);
					if( instance->internal_card_id == p_hand[rnd] ){
						reveal_card(player, card, p, i);
						if( is_what(p, i, TYPE_LAND) ){
							put_into_play(p, i);
						}
						else{
							play_card_in_hand_for_free(p, i);
						}
					}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

/* ARTIFACTS */
int card_brittle_effigy(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_RFG_ME, MANACOST_X(4), 0, &td, "TARGET_CREATURE");
}

int card_crystal_ball(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		scry(player, 2);
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(1), 0, NULL, NULL);
}

int card_elixir_of_immortality(int player, int card, event_t event)
{
  /* Elixir of Immortality	|1
   * Artifact
   * |2, |T: You gain 5 life. Shuffle ~ and your graveyard into their owner's library. */

  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  gain_life(player, 5);

	  if (get_owner(instance->parent_controller, instance->parent_card) != player)
		shuffle_into_library(instance->parent_controller, instance->parent_card);
	  else
		put_on_top_of_deck(instance->parent_controller, instance->parent_card);

	  reshuffle_grave_into_deck(player, 0);
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
}

int effect_lose_defender_gain_flying_until_eot(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (event == EVENT_ABILITIES && affect_me(instance->damage_target_player, instance->damage_target_card))
	{
	  event_result |= KEYWORD_FLYING;
	  event_result &= ~KEYWORD_DEFENDER;
	}

  if ((event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card))
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_gargoyle_sentinel(int player, int card, event_t event)
{
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t *instance = get_card_instance(player, card);
	  create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, effect_lose_defender_gain_flying_until_eot,
									 instance->parent_controller, instance->parent_card);
	}

  return generic_activated_ability(player, card, event, 0, MANACOST_X(3), 0, NULL, NULL);
}

int card_jinxed_idol(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY || event == EVENT_SHOULD_AI_PLAY ){
		damage_player(player, 2, player, card);
	}

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) && player == instance->parent_controller){
		give_control_of_self(instance->parent_controller, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE | GAA_CAN_ONLY_TARGET_OPPONENT, MANACOST0, 0, &td, "TARGET_PLAYER");
}

int card_platinum_angel(int player, int card, event_t event){
	// 004C48A0

	if( player == AI ){
		cannot_block(player, card, event);
	}

	cannot_lose_the_game(player, card, event, player);

	return 0;
}

int card_sorcerers_strongbox(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int flip = flip_a_coin(player, instance->parent_card);
		if( flip == 1 ){
			kill_card(instance->parent_controller, instance->parent_card, KILL_SACRIFICE);
			draw_cards(player, 3);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
}

int card_steel_overseer(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int count = active_cards_count[player]-1;
		while(count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) && is_what(player, count, TYPE_ARTIFACT) ){
					add_1_1_counter(player, count);
				}
				count--;
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_sword_of_vengeance(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 3, 2, 0,  KEYWORD_TRAMPLE | KEYWORD_FIRST_STRIKE, SP_KEYWORD_VIGILANCE | SP_KEYWORD_HASTE);
}

int card_temple_bell(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_a_card(player);
		draw_a_card(1-player);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
}

int card_warlords_axe(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 4, 3, 1, 0, 0);
}

int card_whispersilk_cloak(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 2, 0, 0, KEYWORD_SHROUD, SP_KEYWORD_UNBLOCKABLE);
}

/* GREEN CARDS */
int card_acidic_slime(int player, int card, event_t event){

	deathtouch(player, card, event);

	if( comes_into_play(player, card, event) ){
		card_instance_t *instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.required_type = TYPE_ARTIFACT | TYPE_ENCHANTMENT | TYPE_LAND;
		td.allowed_controller = 2;
		td.preferred_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) && new_pick_target(&td, "Select target artifact, enchantment or land.", 0, GS_LITERAL_PROMPT) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_brindle_boar(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 4);
	}
	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
}

int card_cultivate(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		cultivate(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_dryads_favor(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 2, 2, get_hacked_walk(player, card, KEYWORD_FORESTWALK), 0, 0, 0, 0);
}

int card_elvish_archdruid(int player, int card, event_t event){

	/* Elvish Archdruid	|1|G|G
	 * Creature - Elf Druid 2/2
	 * Other Elf creatures you control get +1/+1.
	 * |T: Add |G to your mana pool for each Elf you control. */

	boost_subtype(player, card, event, SUBTYPE_ELF, 1,1, 0,0, BCT_CONTROLLER_ONLY);

	int count = 0;
	if (event == EVENT_COUNT_MANA || event == EVENT_ACTIVATE || (event == EVENT_CAN_ACTIVATE && player == AI)){	// Only count if it'll actually be needed
		count = count_subtype(player, TYPE_PERMANENT, SUBTYPE_ELF);
	}
	return mana_producing_creature(player, card, event, 36, COLOR_GREEN, count);
}

int card_fauna_shaman(int player, int card, event_t event){

	/* Fauna Shaman	|1|G
	 * Creature - Elf Shaman 2/2
	 * |G, |T, Discard a creature card: Search your library for a creature card, reveal it, and put it into your hand. Then shuffle your library. */

	if (!IS_ACTIVATING(event)){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
	this_test.zone = TARGET_ZONE_HAND;

	if(event == EVENT_CAN_ACTIVATE && generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_G(1), 0, NULL, NULL) &&
		check_battlefield_for_special_card(player, card, player, 0, &this_test)
	   ){
		if( player == AI ){
			if( new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_GOOD_TO_PUT_IN_GRAVE, -1, &this_test) > -1 ){
				return 1;
			}
		}
		else{
			return 1;
		}
	}
	else if(event == EVENT_ACTIVATE){
			charge_mana_for_activated_ability(player, card, MANACOST_G(1));
			if( spell_fizzled != 1 ){
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_GOOD_TO_PUT_IN_GRAVE, -1, &this_test);
				if( selected != -1 ){
					tap_card(player, card);
					discard_card(player, selected);
				}
				else{
					 spell_fizzled = 1;
				}
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION){
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
	}
	return 0;
}

int card_gaeas_revenge(int player, int card, event_t event){
	cannot_be_countered(player, card, event);
	haste(player, card, event);
	/*
	// fizzle any non-green spells or abilities coming at me
	int p;
	int i;
	for( p = 0; p < 2; p++){
		int count = 0;
		while(count < active_cards_count[p]){
			card_instance_t *this_instance = get_card_instance(p, count);
			for(i=0;i<this_instance->number_of_targets;i++){
				if( this_instance->targets[i].player == player && this_instance->targets[i].card == card ){
					if( ! ( get_color(p, count) & COLOR_TEST_GREEN ) ){
						this_instance->targets[i].player =-1;
						this_instance->targets[i].card =-1;
					}
				}
			}
			count++;
		}
	}
	*/
	return 0;
}

int card_garruks_packleader(int player, int card, event_t event){
	if(trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
		reason_for_trigger_controller == player && trigger_cause_controller == player && trigger_cause != card
	  ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.power = 2;
		this_test.power_flag = F5_POWER_GREATER_THAN_VALUE;

		if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_CHECK_DECK_COUNT(player, 1), &this_test) ){
			draw_cards(player, 1);
		}
	}
	return 0;
}

int card_hornet_sting(int player, int card, event_t event){

	/* Hornet Sting	|G
	 * Instant
	 * ~ deals 1 damage to target creature or player. */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_target0(player, card, 1);
		}
		kill_card(player, card, KILL_DESTROY );
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_hunters_feast(int player, int card, event_t event)
{
  /* Hunters' Feast	|3|G
   * Sorcery
   * Any number of target players each gain 6 life. */

	if (event == EVENT_CAN_CAST)
		return 1;

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;
  td.preferred_controller = player;
  td.allow_cancel = 3;	// Both Done and Cancel buttons

  card_instance_t* instance = get_card_instance(player, card);

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  instance->number_of_targets = 0;
	  while (can_target(&td))
		{
		  if (!new_pick_target(&td, "TARGET_PLAYER", -1, 0))
			{
			  if (instance->targets[instance->number_of_targets].card == -1)	// picked cancel, not done
				cancel = 1;
			  return 0;
			}

		  if (td.allowed_controller == ANYBODY)	// was first choice
			{
			  if (IS_AI(player) && instance->targets[0].player == player)
				break;	// AI never targets opponent
			  else
				td.allowed_controller = td.preferred_controller = 1 - instance->targets[0].player;
			}
		  else
			break;	// was second choice
		}
	}

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int i;
	  for (i = 0; i < instance->number_of_targets; ++i)
		if (validate_target(player, card, &td, i))
		  gain_life(instance->targets[i].player, 6);

	  kill_card(player, card, KILL_DESTROY );
	}

  return 0;
}

int card_leyline_of_vitality(int player, int card, event_t event){
	if( ! is_humiliated(player, card) ){
		if( specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			gain_life(player, 1);
		}

		if( event == EVENT_TOUGHNESS && affected_card_controller == player ){
			event_result++;
		}
	}
	return global_enchantment(player, card, event);
}


// This is the version for Limited
int card_mitotic_slime(int player, int card, event_t event){

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_OOZE, &token);
		token.qty = 2;
		token.pow = token.tou = 2;
		token.special_infos = 67;
		generate_token(&token);
	}

	return 0;
}

int card_natures_spiral(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a card to return to your hand.");

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
}

int card_overwhelming_stampede(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		int count = 0;
		int max = 0;
		while(count < active_cards_count[player]){
			card_data_t* card_d = get_card_data(player, count);
			if((card_d->type & TYPE_CREATURE) && in_play(player, count)){
				int power = get_power(player, count);
				if( power > max ){
					max = power;
				}
			}
			count++;
		}
		pump_subtype_until_eot(player, card, player, -1, max, max, KEYWORD_TRAMPLE, 0);

		kill_card(player, card, KILL_DESTROY );
	}

	return basic_spell(player, card, event);
}

int card_primal_cocoon(int player, int card, event_t event){
	card_instance_t* instance = in_play(player, card);
	if (instance && instance->damage_target_player >= 0){
		int t_player = instance->damage_target_player;
		int t_card = instance->damage_target_card;

		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			add_1_1_counters(t_player, t_card, 1);
		}

		if (declare_attackers_trigger(player, card, event, 0, t_player, t_card) || blocking(t_player, t_card, event)){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	// Enchant creature
	return vanilla_aura(player, card, event, player);
}

int card_primeval_titan(int player, int card, event_t event)
{
	/* Whenever ~ enters the battlefield or attacks, you may search your library for up to two land cards, put them onto the battlefield tapped, then shuffle
	 * your library. */
	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ||
		declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card)
	  ){
		tutor_lands(player, TUTOR_PLAY_TAPPED, 2);
		shuffle(player);
	}

	return 0;
}

int card_protean_hydra(int player, int card, event_t event){

	/* Protean Hydra	|X|G
	 * Creature - Hydra 0/0
	 * ~ enters the battlefield with X +1/+1 counters on it.
	 * If damage would be dealt to ~, prevent that damage and remove that many +1/+1 counters from it.
	 * Whenever a +1/+1 counter is removed from ~, put two +1/+1 counters on it at the beginning of the next end step. */

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		set_special_flags2(player, card, SF2_X_SPELL);
		instance->info_slot = x_value;
	}
	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, instance->info_slot);

	if( event == EVENT_DEAL_DAMAGE && ! is_humiliated(player, card) ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == card && damage->damage_target_player == player && damage->info_slot > 0 ){
				int removed = count_counters(player, card, COUNTER_P1_P1);
				if( damage->info_slot < removed){
					removed = damage->info_slot;
				}
				instance->eot_toughness += removed;
				damage->info_slot = 0;
				remove_1_1_counters(player, card, removed);
			}
		}
	}

	if( instance->eot_toughness > 0 && eot_trigger(player, card, event) ){
		add_1_1_counters(player, card, 2 * instance->eot_toughness);
		instance->eot_toughness = 0;
	}
	return 0;
}

// BLUE CARDS

int card_air_servant(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XU(2, 1), 0,
									&td, "Select target creature with flying.");
}

int card_augury_owl(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		scry(player, 3);
	}
	return 0;
}

int card_call_to_mind(int player, int card, event_t event){

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_SPELL, "Select a instant or sorcery spell.");

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( selected != -1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
}

int card_conundrum_sphinx(int player, int card, event_t event){
	if( ! is_unlocked(player, card, event, 8) ){ return 0; }

	/* Whenever ~ attacks, each player names a card. Then each player reveals the top card of his or her library. If the card a player revealed is the card he
	 * or she named, that player puts it into his or her hand. If it's not, that player puts it on the bottom of his or her library. */
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		// each player names a card
		int i;
		for(i=0;i<2;i++){
			int p = player;
			int card_id = -1;
			if(i == 1 ){ p = opp; };
			int *deck = deck_ptr[p];
			if( p == HUMAN ){
				if( ai_is_speculating != 1 ){
					card_id = choose_a_card("Conundrum Sphinx: Name a card", -1, -1);
				}
			}
			else{
				int r = life[AI]-5;
				if( life[AI] > 20 ){
					r = 15;
				}
				else if( life[AI] < 6 ){
					r = 1;
				}
				card_id = deck[ internal_rand(r) ];
			}

			// show what was revealed
			int top_card = add_card_to_hand(p, deck[0]);
			char buffer[100];
			if( p == HUMAN ){
				snprintf(buffer, 100, "You reveal..." );
			}
			else{
				card_data_t* card_d = &cards_data[ card_id ];
				card_ptr_t* c1 = cards_ptr[ card_d->id ];
				snprintf(buffer, 100, "AI guesses %s and reveals...", c1->full_name );
			}

			do_dialog(HUMAN, player, card, p, top_card, buffer, 0);

			if( deck[0] == card_id ){
				obliterate_top_card_of_deck(p);
			}
			else{
				obliterate_card(p, top_card);
				put_top_card_of_deck_to_bottom(p);
			}
		}
	}
	return 0;
}

int card_diminish(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if(event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance= get_card_instance(player, card);
		set_pt_and_abilities_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 1, 0, 0, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_flashfreeze(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_RED) | get_sleighted_color_test(player, card, COLOR_TEST_GREEN);

	return counterspell(player, card, event, &td, 0);
}

int effect_frost_titan(int player, int card, int t_player, int t_card){
	if( in_play(t_player, t_card) ){
		return does_not_untap_effect(player, card, t_player, t_card, EDNT_TAP_TARGET, 1);
	} else {
		return -1;
	}
}

int card_frost_titan(int player, int card, event_t event)
{
  /* Frost Titan	|4|U|U
   * Creature - Giant 6/6
   * Whenever ~ becomes the target of a spell or ability an opponent controls, counter that spell or ability unless its controller pays |2.
   * Whenever ~ enters the battlefield or attacks, tap target permanent. It doesn't untap during its controller's next untap step. */

  if (comes_into_play(player, card, event) || declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_PERMANENT);
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_PERMANENT"))
		effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
	}

  const target_t* targeter;
  if (!is_humiliated(player, card)
	  && (targeter = becomes_target_of_spell(player, card, event, player, card, 1-player, RESOLVE_TRIGGER_MANDATORY)))
	{
	  int p = targeter->player, c = targeter->card;
	  if (has_mana(p, COLOR_ANY, 2)
		  && do_dialog(p, player, card, p, c, " Pay 2\n Decline", 0) == 0)
		{
		  ldoubleclicked = 0;
		  charge_mana(p, COLOR_COLORLESS, 2);
		  if (spell_fizzled != 1)
			return 0;
		}

	  kill_card(p, c, KILL_SACRIFICE);
	}

  return 0;
}

int card_harbor_serpent(int player, int card, event_t event)
{
  if (event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && ! is_humiliated(player, card) )
	{
	  int island = get_hacked_color(player, card, COLOR_BLUE);
	  if (basiclandtypes_controlled[0][island] + basiclandtypes_controlled[1][island] < 5)
		event_result = 1;
	}

  return 0;
}

int card_ice_cage(int player, int card, event_t event)
{
  if (kill_attachment_when_creature_is_targeted(player, card, event, KILL_DESTROY))
	return 0;

  return card_arrest(player, card, event);
}

int card_jaces_erasure(int player, int card, event_t event){
	/* Jace's Erasure	|1|U
	 * Enchantment
	 * Whenever you draw a card, you may have target player put the top card of his or her library into his or her graveyard. */

	if( card_drawn_trigger(player, card, event, player, RESOLVE_TRIGGER_DUH) ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			instance->number_of_targets = 1;
			mill(instance->targets[0].player, 1);
		}
	}
	return global_enchantment(player, card, event);
}

int card_mass_polymorph(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		int p = player;
		int count = active_cards_count[p];
		int creatures = 0;
		while(count >= 0 ){
			card_data_t* card_d = get_card_data(p, count);
			if( in_play(p, count) && (card_d->type & TYPE_CREATURE) ){
				kill_card( p, count, KILL_REMOVE );
				creatures++;
			}
			count--;
		}

		// find that many creatures from the deck
		int *deck = deck_ptr[player];
		int i=0;
		int found_count =0;
		int creatures_found[count_deck(player)];
		while( found_count < creatures && deck[i] != -1 ){
				card_data_t* card_d = &cards_data[ deck[i] ];
				if( card_d->type & TYPE_CREATURE ){
					creatures_found[found_count] = i;
					found_count++;
				}
				i++;
		}

		show_deck( HUMAN, deck_ptr[player], i+1, "These cards were revealed", 0, 0x7375B0 );
		if( found_count > 0 ){
			// remove the creatures from the deck
			for(i=found_count-1;i>=0;i--){
				int idx = creatures_found[i];
				creatures_found[i] = deck[ idx ];
				remove_card_from_deck(player, idx);
			}

			// put the creatures into play
			for(i=0;i<found_count;i++){
				int card_added = add_card_to_hand(player, creatures_found[i]);
				put_into_play(player, card_added);
			}
		}
		shuffle(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_merfolk_sovereign(int player, int card, event_t event){

	boost_creature_type(player, card, event, SUBTYPE_MERFOLK, 1, 1, 0, BCT_CONTROLLER_ONLY);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_MERFOLK;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST0, 0,
									&td, "Select target Merfolk creature.");
}

int card_merfolk_spy(int player, int card, event_t event){

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_BE_COMBAT_DAMAGE) ){
		if(hand_count[opp]==0){
			return 0;
		}
		int r =  internal_rand( hand_count[opp] );
		int cards_array[ 1 ];
		int i=0;
		int hand_index = 0;
		for(i=0;i<active_cards_count[opp]; i++){
			card_instance_t *instance = get_card_instance(opp, i);
			if( ! ( instance->state & STATE_INVISIBLE ) && ! ( instance->state & STATE_IN_PLAY )  ){
				int id = instance->internal_card_id;
				if( id > -1 ){
					if( hand_index == r ){
						cards_array[0] = id;
					}
					hand_index++;
				}
			}
		}
		show_deck( player, cards_array, 1, "Card Revealed at Random", 0, 0x7375B0 );
		card_instance_t *instance = get_card_instance(player, card);
		instance->damage_source_player = -1;
		return 0;
	}

	return 0;

}

int card_phantom_beast(int player, int card, event_t event){
	return card_skulking_ghost(player, card, event);
}

int card_preordain(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		scry(player, 2);
		draw_a_card(player);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_sleep(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int p = instance->targets[0].player;
			int count = active_cards_count[p] -1;
			while(count >= 0 ){
					if( in_play(p, count) && is_what(p, count, TYPE_CREATURE) ){
						effect_frost_titan(player, card, p, count);
					}
					count--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_stormtide_leviathan(int player, int card, event_t event){
	// creatures without flying or islandwalk cannot attack
	if( event == EVENT_ATTACK_LEGALITY && ! is_humiliated(player, card) ){
		int abilities = get_abilities(affected_card_controller, affected_card, EVENT_ABILITIES, -1);
		if (!(abilities & KEYWORD_FLYING) && !(abilities & get_hacked_walk(player, card, KEYWORD_ISLANDWALK))){
			event_result = 1;
		}
	}

	return all_lands_are_basiclandtype(player, card, event, 2, COLOR_BLUE, SUBTYPE_ISLAND);
}

int card_tome_scour(int player, int card, event_t event){
	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 5);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_time_reversal(int player, int card, event_t event)
{
  /* Time Reversal	|3|U|U
   * Sorcery
   * Each player shuffles his or her hand and graveyard into his or her library, then draws seven cards. Exile ~. */

  int rval = resolve_timetwister(player, card, event);
  if (event == EVENT_RESOLVE_SPELL)
	kill_card(player, card, KILL_REMOVE);

  return rval;
}

int card_wall_of_frost(int player, int card, event_t event){
	if( event == EVENT_DECLARE_BLOCKERS && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->blocking < 255 ){
			does_not_untap_effect(player, card, opp, instance->blocking, 0, 1);
		}
	}
	return 0;
}

int card_water_servant(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, 0, MANACOST_U(1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_U(1)) ){
			int ai_choice = 1;
			if( is_attacking(player, card) && current_phase == PHASE_AFTER_BLOCKING && is_unblocked(player, card) ){
				ai_choice = 0;
			}
			int choice = do_dialog(player, player, card, -1, -1, " +1/-1\n -1/+1\n Cancel", ai_choice);
			if( choice == 2 ){
				spell_fizzled = 1;
			}
			else{
				instance->info_slot = choice;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 0 ){
			pump_until_eot(player, card, player, instance->parent_card, 1, -1);
		}
		else{
			pump_until_eot(player, card, player, instance->parent_card, -1, 1);
		}
	}
	return 0;
}


int card_dark_tutelage(int player, int card, event_t event){

	if(trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && current_turn==player ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
			int *deck = deck_ptr[player];
			int card_added = add_card_to_hand(player, deck[0] );
			remove_card_from_deck( player, 0 );
			int cmc = get_cmc(player, card_added);
			lose_life(player, cmc);
			reveal_card(player, card, player, card_added);
		}
	}

	return global_enchantment(player, card, event);
}


