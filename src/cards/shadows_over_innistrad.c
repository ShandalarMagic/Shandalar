#include "manalink.h"

/***** Functions *****/

int delirium(int player){
	// Delirium - if there are four or more card types among cards in your graveyard...
	return count_types_in_grave(player) >= 4 ? 1 : 0;
}

static void delirium_boost(int player, int card, event_t event, int pow, int tou, int key, int s_key){
	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER && affect_me(player, card) && pow && delirium(player) ){
			event_result += pow;
		}
		if( event == EVENT_TOUGHNESS && affect_me(player, card) && tou && delirium(player) ){
			event_result += tou;
		}
		if( event == EVENT_ABILITIES && affect_me(player, card) && (key || s_key) && delirium(player) ){
			event_result |= key;
			if( s_key ){
				special_abilities(player, card, event, s_key, player, card);
			}
		}
	}
}

void investigate(int player, int card, int t_player){
	/*
	  Put a colorless Clue artifact token onto the battlefield with "{2}, Sacrifice this artifact: Draw a card."
	*/
	token_generation_t token;
	default_token_definition(player, card, CARD_ID_CLUE, &token);
	token.t_player = t_player;
	generate_token(&token);

	//Special abilities linked to Investigate. Until no more cards of this type are created, there's no need to a dedicated EVENT
	int c;
	for(c=active_cards_count[t_player]; c>-1; c--){
		if( in_play(t_player, c) && ! is_humiliated(t_player, c) ){
			//Erdwal Illuminator - Whenever you investigate for the first time each turn, investigate an additional time.
			if( get_id(t_player, c) == CARD_ID_ERDWAL_ILLUMINATOR ){
				get_card_instance(t_player, c)->info_slot++;
				if( get_card_instance(t_player, c)->info_slot < 2 ){
					investigate(t_player, c, t_player);
				}
			}
		}
	}
}

static void make_spirit(int s_player, int s_card, int t_player, int qty, int sleight){
	token_generation_t token;
	default_token_definition(s_player, s_card, CARD_ID_SPIRIT, &token);
	token.t_player = t_player;
	token.qty = qty;
	token.pow = token.tou = 1;
	token.key_plus = KEYWORD_FLYING;
	token.color_forced = (sleight ? get_sleighted_color_test(s_player, s_card, COLOR_TEST_WHITE) : COLOR_TEST_WHITE);
	generate_token(&token);
}

static void make_zombie(int s_player, int s_card, int t_player, int qty, int sleight, int action){
	token_generation_t token;
	default_token_definition(s_player, s_card, CARD_ID_ZOMBIE, &token);
	token.t_player = t_player;
	token.pow = token.tou = 2;
	token.color_forced = (sleight ? get_sleighted_color_test(s_player, s_card, COLOR_TEST_BLACK) : COLOR_TEST_BLACK);
	token.qty = qty;
	token.action = action;
	generate_token(&token);
}

static void make_devil(int s_player, int s_card, int t_player, int qty, int sleight){
	token_generation_t token;
	default_token_definition(s_player, s_card, CARD_ID_DEVIL, &token);
	token.t_player = t_player;
	token.qty = qty;
	token.pow = token.tou = 1;
	token.color_forced = (sleight ? get_sleighted_color_test(s_player, s_card, COLOR_TEST_RED) : COLOR_TEST_RED);
	generate_token(&token);
}

static void make_wolf(int s_player, int s_card, int t_player, int qty, int sleight){
	token_generation_t token;
	default_token_definition(s_player, s_card, CARD_ID_WOLF, &token);
	token.t_player = t_player;
	token.qty = qty;
	token.pow = token.tou = 2;
	token.color_forced = (sleight ? get_sleighted_color_test(s_player, s_card,COLOR_TEST_GREEN) : COLOR_TEST_GREEN);
	generate_token(&token);
}

static void make_human_soldier(int s_player, int s_card, int t_player, int qty, int sleight){
	token_generation_t token;
	default_token_definition(s_player, s_card, CARD_ID_HUMAN_SOLDIER, &token);
	token.t_player = t_player;
	token.pow = token.tou = 1;
	token.color_forced = (sleight ? get_sleighted_color_test(s_player, s_card, COLOR_TEST_WHITE) : COLOR_TEST_WHITE);
	token.qty = qty;
	generate_token(&token);
}

void skulk(int player, int card, event_t event){
	// Skulk. (this creature can't be blocked by creatures with greater power.)

	if( event == EVENT_ABILITIES && affect_me(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance( player, card );
		if( instance->targets[16].card < 0 ){
			instance->targets[16].card = 0;
		}
		instance->targets[16].card |= SP_KEYWORD_SKULK;
	}

	if( in_play(player, card) && event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( attacking_card_controller == player && attacking_card == card ){
			if( get_power(affected_card_controller, affected_card) > get_power(attacking_card_controller, attacking_card) ){
				event_result = 1;
			}
		}
	}
}

static int whenever_a_clue_is_sacrificed(int player, int card, event_t event, int t_player, int trig_mode){

	if( event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) ){
		if( get_card_instance(affected_card_controller, affected_card)->kill_code == KILL_SACRIFICE &&
			get_id(affected_card_controller, affected_card) == CARD_ID_CLUE )
		{
			count_for_gfp_ability(player, card, event, t_player, TYPE_PERMANENT, NULL);
		}
	}

	return resolve_gfp_ability(player, card, event, trig_mode);
}

void soi_need_subtype_land(int player, int card, event_t event, subtype_t subtype, subtype_t sub2){
	if( event == EVENT_CAST_SPELL && affect_me(player, card) && !is_tapped(player, card) ){	// e.g. from Kismet
		int tapme = 1;
		if( is_subtype_in_hand(player, subtype) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY,
										get_hacked_land_text(player, card, "Select a %a or %a card to reveal", subtype, sub2));
			this_test.subtype = subtype;
			this_test.sub2 = sub2;
			this_test.subtype_flag = F2_MULTISUBTYPE;
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if (selected != -1){
				reveal_card(player, card, player, selected);
				tapme = 0;
			}
		}
		if( tapme ){
			get_card_instance(player, card)->state |= STATE_TAPPED;
		}
	}
}

/***** Cards *****/

/*** White ***/

int card_always_watching(int player, int card, event_t event){
	/* Always Watching	|1|W|W	0x200e72e
	 * Enchantment
	 * Nontoken creatures you control get +1/+1 and have vigilance. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
			if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
				! is_token(affected_card_controller, affected_card) )
			{
				event_result++;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_angel_of_deliverance(int player, int card, event_t event){
	/* Angel of Deliverance	|6|W|W	0x200e733
	 * Creature - Angel 6/6
	 * Flying
	 * Delirium - Whenever ~ deals damage, if there are four or more card types among cards in your graveyard,
	 * exile target creature an opponent controls. */

	store_attackers(player, card, event, 0, player, card, NULL);

	if (xtrigger_condition() == XTRIGGER_ATTACKING && affect_me(player, card) && reason_for_trigger_controller == player)
	{
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = td.allowed_controller = player;

		card_instance_t *instance = get_card_instance( player, card);

		instance->number_of_targets = 0;

		if( delirium(player) && can_target(&td) ){
			if( resolve_declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
				if( new_pick_target(&td, "Select target creature an opponent controls.", 0, GS_LITERAL_PROMPT) ){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
				}
			}
		}
	}

	return 0;
}

int card_angelic_purge(int player, int card, event_t event){
	/* Angelic Purge	|2|W	0x200e738
	 * Sorcery
	 * As an additional cost to cast ~, sacrifice a permanent.
	 * Exile target artifact, creature, or enchantment. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_ENCHANTMENT);

	test_definition_t test;
	new_default_test_definition(&test, TYPE_PERMANENT, "Select a permanent to sacrifice.");

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_CAN_CAST ){
		if( new_can_sacrifice_as_cost(player, card, &test) ){
			return generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (sac){
			generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
			if( spell_fizzled != 1 ){
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
				spell_fizzled = 1;
				return 0;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_apothecary_geist(int player, int card, event_t event){
	/* Apothecary Geist	|3|W	0x200e73d
	 * Creature - Spirit 2/3
	 * Flying
	 * When ~ enters the battlefield, if you control another Spirit, you gain 3 life. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_PERMANENT);
		test.subtype = SUBTYPE_SPIRIT;
		test.not_me = 1;
		if( comes_into_play(player, card, event) && check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &test) ){
			gain_life(player, 3);
		}
	}

	return 0;
}

int card_archangel_avacyn(int player, int card, event_t event){
	/* Archangel Avacyn	|3|W|W	0x200e742
	 * Legendary Creature - Angel 4/4
	 * Flash
	 * Flying, vigilance
	 * When ~ enters the battlefield, creatures you control gain indestructible until end of turn.
	 * When a non-Angel creature you control dies, transform ~ at the beginning of the next upkeep.*/

	check_legend_rule(player, card, event);

	double_faced_card(player, card, event);

	vigilance(player, card, event);

	if( comes_into_play(player, card, event) ){
		pump_creatures_until_eot(player, card, player, 1, 0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE, NULL);
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t test;
		default_test_definition(&test, TYPE_PERMANENT);
		test.subtype = SUBTYPE_ANGEL;
		test.subtype_flag = DOESNT_MATCH;
		count_for_gfp_ability(player, card, event, player, -1, &test);
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		get_card_instance(player, card)->info_slot = 66;
		if( current_phase == PHASE_UPKEEP ){
			remove_state(player, card, STATE_PROCESSING);
		}
	}

	if( get_card_instance(player, card)->info_slot == 66 ){
		if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
			get_card_instance(player, card)->info_slot = 0;
			if( upkeep_trigger(player, card, event) ){
				transform(player, card);
			}
		}
	}

	return flash(player, card, event);
}

int card_avacyn_the_purifier(int player, int card, event_t event){
 /* Avacyn, the Purifier	0x200e747
 * Legendary Creature - Angel 6/5
 * Flying
 * When this creature transforms into ~, it deals 3 damage to each other creature and each opponent. */

	check_legend_rule(player, card, event);

	if( event == EVENT_TRANSFORMED ){
		APNAP(p, {new_damage_all(player, card, p, 3, NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);};);
	}

	return 0;
}

int card_avacynian_missionaires(int player, int card, event_t event){
	/* Avacynian Missionaries	|3|W	0x200e74c
	 * Creature - Human Cleric 3/3
	 * At the beginning of your end step, if ~ is equipped, transform it.
	 */
	double_faced_card(player, card, event);

	return_from_oblivion(player, card, event);

	if( trigger_condition == TRIGGER_EOT && affect_me(player, card) ){
		if( is_equipped(player, card) && eot_trigger(player, card, event) ){
			transform(player, card);
		}
	}

	return 0;
}


int card_lunarch_inquisitors(int player, int card, event_t event){
	/*
	 * Lunarch Inquisitors	0x200e751
	 * Creature - Human Cleric 4/4
	 * When this creature transforms into ~, you may exile another target creature until ~ leaves the battlefield. */

	return_from_oblivion(player, card, event);

	if( event == EVENT_TRANSFORMED ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		if( can_target(&td) && new_pick_target(&td, "TARGET_CREATURE", 0, 0) ){
			card_instance_t *instance = get_card_instance( player, card);
			obliviation(player, card, instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_bound_by_moonsilver(int player, int card, event_t event){
	/* Bound by Moonsilver	|2|W	0x200e756
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature can't attack, block, or transform.
	 * Sacrifice another permanent: Attach ~ to target creature.
	 * Activate this ability only any time you could cast a sorcery and only once each turn. */

	// "can't transform" --> in "int can_transform"

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		test_definition_t test;
		default_test_definition(&test, TYPE_PERMANENT);
		test.not_me = 1;

		card_instance_t *instance = get_card_instance( player, card);

		if( event == EVENT_CAN_ACTIVATE ){
			if( generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_CAN_SORCERY_BE_PLAYED | GAA_ONCE_PER_TURN, MANACOST0, 0, &td, NULL) ){
				return new_can_sacrifice_as_cost(player, card, &test);
			}
		}

		if( event == EVENT_ACTIVATE ){
			int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if (sac){
				generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_ONCE_PER_TURN, MANACOST0, 0, &td, NULL);
				if( spell_fizzled != 1 ){
					kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
				}
				else{
					state_untargettable(BYTE2(sac), BYTE3(sac), 0);
					spell_fizzled = 1;
					return 0;
				}
			}
			else{
				spell_fizzled = 1;
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				attach_aura_to_target(instance->parent_controller, instance->parent_card, event,
										instance->targets[0].player, instance->targets[0].card);
			}
		}
	}

	return card_pacifism(player, card, event);
}

int card_bygone_bishop(int player, int card, event_t event){
	/* Bygone Bishop	|2|W	0x200e75b
	 * Creature - Spirit Cleric 2/3
	 * Flying
	 * Whenever you cast a creature spell with converted mana cost 3 or less, investigate. */

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.cmc = 4;
		test.cmc_flag = F5_CMC_LESSER_THAN_VALUE;
		if( new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test) ){
			investigate(player, card, player);
		}
	}

	return 0;
}

int card_cathars_companion(int player, int card, event_t event){
	/* Cathar's Companion	|2|W	0x200e760
	 * Creature - Hound 3/1
	 * Whenever you cast a noncreature spell, ~ gains indestructible until end of turn. */

	if( prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		pump_ability_until_eot(player, card, player, card, 0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
	}

	return 0;
}

/* Chaplain's Blessing	|W	--> Whitesun's Passage	0x200446d
 * Sorcery
 * You gain 5 life. */

int card_dauntless_cathar(int player, int card, event_t event){
	/* Dauntless Cathar	|2|W	0x200e765
	 * Creature - Human Soldier 3/2
	 * |1|W, Exile ~ from your graveyard: Put a 1/1 |Swhite Spirit creature token with flying onto the battlefield.
	 *		Activate this ability only any time you could cast a sorcery. */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XW(1, 1)) && can_sorcery_be_played(player, event) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_multi(player, MANACOST_XW(1, 1)) ){
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		make_spirit(player, card, player, 1, 0);
	}

	return 0;
}

int card_declaration_in_stone(int player, int card, event_t event){
	/* Declaration in Stone	|1|W	0x200e76a
	 * Sorcery
	 * Exile target creature and all other creatures its controller controls with the same name as that creature.
	 * That player investigates for each nontoken creature exiled this way. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int name = get_card_name(instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			int p = instance->targets[0].player;
			int c;
			for(c=active_cards_count[p]-1; c>-1; c--){
				if( in_play(p, c) && get_card_name(p, c) == name ){
					int can_investigate = ! is_token(p, c);
					kill_card(p, c, KILL_REMOVE);
					if( can_investigate ){
						investigate(player, card, p);
					}
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}


int card_descend_upon_the_sinful(int player, int card, event_t event){
	/* Descend upon the Sinful	|4|W|W	0x200e783
	 * Sorcery
	 * Exile all creatures.
	 * Delirium - Put a 4/4 |Swhite Angel creature token with flying onto the battlefield if there are four or more card types
	 *	among cards in your graveyard. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_REMOVE);};);

		if( delirium(player) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_ANGEL, &token);
			token.pow = token.tou = 4;
			token.key_plus = KEYWORD_FLYING;
			token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			generate_token(&token);
		}

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

/* Devilthorn Fox	|1|W --> vanilla	0x401000
 * Creature - Fox 3/1 */

int card_drogskol_cavalry(int player, int card, event_t event){
	/* Drogskol Cavalry	|5|W|W	0x200e76f
	 * Creature - Spirit Knight 4/4
	 * Flying
	 * Whenever another Spirit enters the battlefield under your control, you gain 2 life.
	 * |3|W: Put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_PERMANENT);
		test.subtype = SUBTYPE_SPIRIT;
		test.not_me = 1;
		if( new_specific_cip(player, card, event, player, -1, &test) ){
			gain_life(player, 2);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance(player, card);
		make_spirit(instance->parent_controller, instance->parent_card, instance->parent_controller, 1, 1);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XW(3, 1), 0, NULL, NULL);
}

int card_eerie_interlude(int player, int card, event_t event){
	/* Eerie Interlude	|2|W	0x200e774
	 * Instant
	 * Exile any number of target creatures you control.
	 * Return those cards to the battlefield under their owner's control at the beginning of the next end step. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				remove_until_eot(player, card, instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	//Max 10 targets or they will overlap with the reserved spots in the targeting array.
	return generic_spell(player, card, event, GS_OPTIONAL_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature you control.", 10, NULL);
}

int card_emissary_of_the_sleepless(int player, int card, event_t event){
	/* Emissary of the Sleepless	|4|W	0x200e779
	 * Creature - Spirit 2/4
	 * Flying
	 * When ~ enters the battlefield, if a creature died this turn, put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	if( comes_into_play(player, card, event) && morbid() ){
		make_spirit(player, card, player, 1, 1);
	}

	return 0;
}

/* Ethereal Guidance	|2|W --> Inspired Charge	0x20026a9
 * Sorcery
 * Creatures you control get +2/+1 until end of turn. */

int card_expose_evil(int player, int card, event_t event){
	/* Expose Evil	|1|W	0x200e77e
	 * Instant
	 * Tap up to two target creatures.
	 * Investigate. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				tap_card(instance->targets[i].player, instance->targets[i].card);
			}
		}
		investigate(player, card, player);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_gryffs_boon(int player, int card, event_t event){
	/* Gryff's Boon	|W	0x200e788
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +1/+0 and has flying.
	 * |3|W: Return ~ from your graveyard to the battlefield attached to target creature.
	 * Activate this ability only any time you could cast a sorcery. */

	if(event == EVENT_GRAVEYARD_ABILITY){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		if( has_mana_multi(player, MANACOST_XW(3, 1)) && can_sorcery_be_played(player, event) && can_target(&td) ){
			return GA_RETURN_TO_PLAY;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.illegal_abilities = 0;
		charge_mana_multi(player, MANACOST_XW(3, 1));
		if( spell_fizzled != 1 && new_pick_target(&td, "TARGET_CREATURE", 0, 1) ){
			return GAPAID_REMOVE;
		}
	}

	return generic_aura(player, card, event, player, 1, 0, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_hanweir_militia_captain(int player, int card, event_t event){
	/* Hanweir Militia Captain	|1|W	0x200e78d
	 * Creature - Human Soldier 2/2
	 * At the beginning of your upkeep, if you control four or more creatures, transform ~.
	 */

	double_faced_card(player, card, event);

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) && count_subtype(player, TYPE_CREATURE, -1) >= 4 )
	{
		upkeep_trigger_ability(player, card, event, player);
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY && count_subtype(player, TYPE_CREATURE, -1) >= 4 ){
		transform(player, card);
	}

	return 0;
}

int card_westvale_cult_leader(int player, int card, event_t event){
	/* Westvale Cult Leader		0x200e792
	 * Creature - Human Cleric 100/100
	 * ~'s power and toughness are each equal to the number of creatures you control.
	 * At the beginning of your end step, put a 1/1 |Swhite and |Sblack Human Cleric creature token onto the battlefield. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result += count_subtype(player, TYPE_CREATURE, -1);
		}
	}

	if( current_turn == player && eot_trigger(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_HUMAN_CLERIC, &token);
		token.pow = token.tou = 1;
		token.color_forced =	get_sleighted_color_test(player, card, COLOR_TEST_BLACK) |
								get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
		generate_token(&token);
	}

	return 0;
}

int card_hope_against_hope(int player, int card, event_t event){
	/* Hope Against Hope	|2|W	0x200e797
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +1/+1 for each creature you control.
	 * As long as enchanted creature is a Human, it has first strike. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance( player, card);
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( p > -1 ){
			if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(p, c) ){
				event_result += count_subtype(player, TYPE_CREATURE, -1);
			}
			if( event == EVENT_ABILITIES && affect_me(p, c) ){
				if( has_subtype(p, c, SUBTYPE_HUMAN) ){
					event_result |= KEYWORD_FIRST_STRIKE;
				}
			}
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

int card_humble_the_brute(int player, int card, event_t event){
	/* Humble the Brute	|4|W	0x200e79c
	 * Instant
	 * Destroy target creature with power 4 or greater.
	 * Investigate. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
			investigate(player, card, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature with power 4 or greater.", 1, NULL);
}

int card_inquisitors_ox(int player, int card, event_t event){
	/* Inquisitor's Ox	|3|W	0x200e7a1
	 * Creature - Ox 2/5
	 * Delirium - ~ gets +1/+0 and has vigilance as long as there are four or more card types among cards in your graveyard. */

	delirium_boost(player, card, event, 1, 0, 0, SP_KEYWORD_VIGILANCE);

	return 0;
}

/* Inspiring Captain	|3|W --> Ampryn Tactician	0x200dc1b
 * Creature - Human Knight 3/3
 * When ~ enters the battlefield, creatures you control get +1/+1 until end of turn. */

int card_militant_inquisitor(int player, int card, event_t event){
	/* Militant Inquisitor	|2|W	0x200e7ab
	 * Creature - Human Cleric 2/3
	 * ~ gets +1/+0 for each Equipment you control. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_POWER && affect_me(player, card) ){
			event_result += count_subtype(player, TYPE_PERMANENT, SUBTYPE_EQUIPMENT);
		}
	}

	return 0;
}

int card_moorland_drifter(int player, int card, event_t event){
	/* Moorland Drifter	|1|W	0x200e7a6
	 * Creature - Spirit 2/2
	 * Delirium - ~ has flying as long as there are four or more card types among cards in your graveyard. */

	delirium_boost(player, card, event, 0, 0, KEYWORD_FLYING, 0);

	return 0;
}

int card_nahiri_machinations(int player, int card, event_t event){
	/* Nahiri's Machinations	|1|W	0x200e7b0
	 * Enchantment
	 * At the beginning of combat on your turn, target creature you control gains indestructible until end of turn.
	 * |1|R: ~ deals 1 damage to target blocking creature. */

	if( beginning_of_combat(player, card, event, player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = td.preferred_controller = player;

		card_instance_t *instance = get_card_instance( player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) && new_pick_target(&td, "Select target creature you control.", 0, GS_LITERAL_PROMPT) ){
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
		}
	}


	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_state = TARGET_STATE_BLOCKING;

		if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td)){
			damage_target0(player, card, 1);
		}
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XR(1, 1), 0,
										&td, "Select target blocking creature.");
	}
	return global_enchantment(player, card, event);
}

int card_nearheath_chaplain(int player, int card, event_t event){
	/* Nearheath Chaplain	|3|W	0x200e7b5
	 * Creature - Human Cleric 3/1
	 * Lifelink
	 * |2|W, Exile ~ from your graveyard: Put two 1/1 |Swhite Spirit creature tokens with flying onto the battlefield.
	 *	Activate this ability only any time you could cast a sorcery. */

	lifelink(player, card, event);

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XW(2, 1)) && can_sorcery_be_played(player, event) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_multi(player, MANACOST_XW(2, 1)) ){
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		make_spirit(player, card, player, 2, 0);
	}

	return 0;
}

static int put_card_from_grave_on_bottom_of_deck(int player, int grave_pos){
	int *deck = deck_ptr[player];
	int cg = MAX(0, count_graveyard(player)-1);
	int iid = get_grave(player)[grave_pos];
	remove_card_from_grave(player, grave_pos);
	deck[cg] = iid;
	return iid;
}

int card_not_forgotten(int player, int card, event_t event){
	/* Not Forgotten	|1|W	0x200e7ba
	 * Sorcery
	 * Put target card from a graveyard on the top or bottom of its owner's library.
	 * Put a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put on bottom of deck.");

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return generic_spell(player, card, event, GS_GRAVE_RECYCLER_BOTH_GRAVES, NULL, NULL, 1, &this_test);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance( player, card );
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 0);
		if( selected != -1 ){
			put_card_from_grave_on_bottom_of_deck(instance->targets[0].player, selected);
			make_spirit(player, card, player, 1, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_odric_lunarch_marshal(int player, int card, event_t event){
	/* Odric, Lunarch Marshal	|3|W	0x200e7bf
	 * Legendary Creature - Human Soldier 3/3
	 * At the beginning of each combat, creatures you control gain first strike until end of turn if a creature you control has first strike.
	 *	The same is true for flying, deathtouch, double strike, haste, hexproof, indestructible, lifelink, menace, reach, skulk, trample, and
	 *	vigilance. */

	check_legend_rule(player, card, event);

	if( beginning_of_combat(player, card, event, ANYBODY, -1) ){
		int key_pump = 0;
		int key_mask = KEYWORD_FIRST_STRIKE | KEYWORD_FLYING | KEYWORD_DOUBLE_STRIKE | KEYWORD_REACH | KEYWORD_TRAMPLE;
		int s_key_pump = 0;
		int s_key_mask =	SP_KEYWORD_DEATHTOUCH | SP_KEYWORD_HASTE | SP_KEYWORD_HEXPROOF | SP_KEYWORD_INDESTRUCTIBLE |
							SP_KEYWORD_LIFELINK | SP_KEYWORD_MENACE | SP_KEYWORD_SKULK | SP_KEYWORD_VIGILANCE;

		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) ){
				key_pump |= (get_abilities(player, c, EVENT_ABILITIES, -1) & key_mask);
				s_key_pump |= (get_special_abilities(player, c) & s_key_mask);
			}
		}
		pump_creatures_until_eot(player, card, player, 0, 0, 0, key_pump, s_key_pump, NULL);
	}

	return 0;
}

static int is_aura_or_equipment(int iid, int unused, int unused2, int unused3){

	if( has_subtype_by_id(cards_data[iid].id, SUBTYPE_EQUIPMENT) ||
		(is_what(-1, iid, TYPE_ENCHANTMENT) && has_subtype_by_id(cards_data[iid].id, SUBTYPE_AURA)) )
	{
		return 1;
	}

	return 0;
}

int card_open_the_armory(int player, int card, event_t event){
	/* Open the Armory	|1|W	0x200e7c4
	 * Sorcery
	 * Search your library for an Aura or Equipment card, reveal it, and put it into your hand. Then shuffle your library. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_PERMANENT, "Select an Aura or Equipment card.");
		test.special_selection_function = &is_aura_or_equipment;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_paranoid_parish_blade(int player, int card, event_t event){
	/* Paranoid Parish-Blade	|2|W	0x200e7c9
	 * Creature - Human Soldier 3/2
	 * Delirium - ~ gets +1/+0 and has first strike as long as there are four or more card types among cards in your graveyard. */
	delirium_boost(player, card, event, 1, 0, KEYWORD_FIRST_STRIKE, 0);
	return 0;
}

int card_pious_evangel(int player, int card, event_t event){
	/* Pious Evangel	|2|W	0x200e7ce
	 * Creature - Human Cleric 2/2
	 * Whenever ~ or another creature enters the battlefield under your control, you gain 1 life.
	 * |2, |T, Sacrifice another permanent: Transform ~.*/

	double_faced_card(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &this_test)){
			gain_life(player, 1);
		}
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t test;
	new_default_test_definition(&test, TYPE_PERMANENT, "Select another permanent to sacrifice.");
	test.not_me = 1;

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL) ){
			return new_can_sacrifice_as_cost(player, card, &test);
		}
	}

	if( event == EVENT_ACTIVATE ){
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (sac){
			generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL);
			if( spell_fizzled != 1 ){
				kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
			}
			else{
				state_untargettable(BYTE2(sac), BYTE3(sac), 0);
				return 0;
			}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance( player, card );
		if( ! check_special_flags3(instance->parent_controller, instance->parent_card, SF3_CARD_IS_FLIPPED) ){
			transform(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

static int wayward_disciple_effect(int player, int card, event_t event){

	//In case something is copying Wayward Disciple and then reverts to its original form.
	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( get_id(instance->damage_target_player, instance->damage_target_card) != CARD_ID_WAYWARD_DISCIPLE ){
				kill_card(player, card, KILL_REMOVE);
			}
		}
	}

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		card_instance_t *instance = get_card_instance(player, card);
		if( in_play(affected_card_controller, affected_card) ){
			int kill_code = get_card_instance(affected_card_controller, affected_card)->kill_code;
			if( kill_code ){
				if( instance->damage_target_player > -1 ){
					int p = instance->damage_target_player;
					int c = instance->damage_target_card;
					if( affect_me(p, c) ){
						//Will trigger also if this is not a creature when it goes into the graveyard;
						if( ! is_what(p, c, TYPE_CREATURE) ){
							count_for_gfp_ability(player, card, event, player, TYPE_PERMANENT, NULL);
						}
						instance->damage_target_player = instance->damage_target_card = -1;
						remove_status(player, card, STATUS_INVISIBLE_FX);
					}
				}
				count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);
			}
		}
	}

	if( ! check_status(player, card, STATUS_INVISIBLE_FX) ){
		int amount = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
		if( amount ){
			life_sucking(player, card, amount);
			kill_card(player, card, KILL_REMOVE);
		}
	}

	//Reset counts if the legacy is still invisible
	if( trigger_condition == TRIGGER_GRAVEYARD_FROM_PLAY && event == EVENT_END_TRIGGER ){
		get_card_instance(player, card)->targets[11].player = 0;
	}

	return 0;
}

int card_wayward_disciple(int player, int card, event_t event){
	/* Wayward Disciple	0x200e7d3
	 * Creature - Human Cleric 2/4
	 * Whenever ~ or another creature you control dies, target opponent loses 1 life and you gain 1 life. */

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		int found = 0;
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_EFFECT) ){
				card_instance_t *inst = get_card_instance(player, c);
				if( inst->info_slot == (int)wayward_disciple_effect ){
					if( (inst->damage_target_player == player && inst->damage_target_card == card) ||
						(inst->targets[0].player == player && inst->targets[0].card == card) )
					{
						found = 1;
						break;
					}
				}
			}
		}
		if( ! found ){
			int legacy = create_targetted_legacy_effect(player, card, &wayward_disciple_effect, player, card);
			card_instance_t *inst = get_card_instance(player, legacy);
			inst->targets[0].player = player;
			inst->targets[0].card = card;
			inst->number_of_targets = 1;
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);
	}

	if( get_card_instance(player, card)->kill_code <= 0 ){
		int amount = resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY);
		if( amount ){
			life_sucking(player, card, amount);
		}
	}

	return 0;
}

/* Puncturing Light	|1|W --> puncturing light	0x2008635
 * Instant
 * Destroy target attacking or blocking creature with power 3 or less. */

int card_reaper_of_the_flight_moonsilver(int player, int card, event_t event){
	/* Reaper of Flight Moonsilver	|3|W|W	0x200e7d8
	 * Creature - Angel 3/3
	 * Flying
	 * Delirium - Sacrifice another creature: ~ gets +2/+1 until end of turn.
	 * Activate this ability only if there are four or more card types among cards in your graveyard. */

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
	{
		int pump = event == EVENT_POW_BOOST ? 2 : 1;

		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select another creature card to sacrifice.");
		test.not_me = 1;

		int num_can_sac = max_can_sacrifice_as_cost(player, card, &test);
		if (num_can_sac <= 0)
			return 0;

		return generic_shade_amt_can_pump(player, card, pump, 0, MANACOST0, num_can_sac);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t test;
	new_default_test_definition(&test, TYPE_CREATURE, "Select another creature card to sacrifice.");
	test.not_me = 1;

	if( event == EVENT_CAN_ACTIVATE ){
		if( delirium(player) && generic_activated_ability(player, card, event, 0, MANACOST0, 0, NULL, NULL) ){
			return new_can_sacrifice_as_cost(player, card, &test);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( ! new_sacrifice(player, card, player, SAC_AS_COST, &test) ){
			spell_fizzled = 1;
			return 0;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		return generic_husk(player, card, event, TYPE_CREATURE, 2, 1, 0, 0);
	}

	return 0;
}

int card_silverstrike(int player, int card, event_t event){
	/* Silverstrike	|3|W	0x200e7dd
	 * Instant
	 * Destroy target attacking creature. You gain 3 life. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			gain_life(player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target attacking creature.", 1, NULL);
}


int card_spectral_shepherd(int player, int card, event_t event){
	/* Spectral Shepherd	|2|W	0x200e7e2
	 * Creature - Spirit 2/2
	 * Flying
	 * |1|U: Return target Spirit you control to its owner's hand. */

	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_SPIRIT;
	td.allowed_controller = td.preferred_controller = player;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		bounce_permanent(instance->targets[0].player, instance->targets[0].card);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XU(1, 1), 0,
									&td, "Select target Spirit you control.");
}

int card_stern_constable(int player, int card, event_t event){
	/* Stern Constable	|W	200e7e7
	 * Creature - Human Soldier 1/1
	 * |T, Discard a card: Tap target creature. */

	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		tap_card(instance->targets[0].player, instance->targets[0].card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_DISCARD, MANACOST0, 0, &td, "TARGET_CREATURE");
}

int card_strenght_of_arms(int player, int card, event_t event){
	/* Strength of Arms	|W	0x200e7ec
	 * Instant
	 * Target creature gets +2/+2 until end of turn. If you control an Equipment, put a 1/1 |Swhite Human Soldier
	 * creature token onto the battlefield. */

	if (!IS_GS_EVENT(player, card, event)  ){
		return vanilla_instant_pump(player, card, event, ANYBODY, player, 2, 2, 0, 0);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if (event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card );

			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);

			if( count_subtype(player, TYPE_PERMANENT, SUBTYPE_EQUIPMENT) ){
				make_human_soldier(player, card, player, 1, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_survive_the_night(int player, int card, event_t event){
	/* Survive the Night	|2|W	0x200e7f1
	 * Instant
	 * Target creature gets +1/+0 and gains indestructible until end of turn.
	 * Investigate. */

	if (!IS_GS_EVENT(player, card, event)  ){
		return vanilla_instant_pump(player, card, event, ANYBODY, player, 1, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	if (event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card );

			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);

			investigate(player, card, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_tenacity(int player, int card, event_t event)
{
	/* Tenacity	|3|W	0x200e7f6
	 * Instant
	 * Creatures you control get +1/+1 and gain lifelink until end of turn. Untap those creatures. */

	if( event == EVENT_RESOLVE_SPELL ){
		pump_ability_t pump;
		default_pump_ability_definition(player, card, &pump, 1, 1, 0, 0);
		pump.paue_flags = PAUE_END_AT_EOT | PAUE_UNTAP;
		pump_ability_by_test(player, card, player, &pump, NULL);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_thalias_liutenant(int player, int card, event_t event)
{
	/* Thalia's Lieutenant	|1|W	0x200e7fb
	 * Creature - Human Soldier 1/1
	 * When ~ enters the battlefield, put a +1/+1 counter on each other Human you control.
	 * Whenever another Human enters the battlefield under your control, put a +1/+1 counter on ~. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_PERMANENT);
		test.not_me = 1;
		test.subtype = SUBTYPE_HUMAN;
		if( comes_into_play(player, card, event) ){
			new_manipulate_all(player, card, player, &test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
		}
		if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test) ){
			add_1_1_counter(player, card);
		}
	}

	return 0;
}

int card_thraben_inspector(int player, int card, event_t event){
	/* Thraben Inspector	|W	0x200e800
	 * Creature - Human Soldier 1/2
	 * When ~ enters the battlefield, investigate. */

	if( comes_into_play(player, card, event) ){
		investigate(player, card, player);
	}

	return 0;
}

int card_topplegeist(int player, int card, event_t event){
	/* Topplegeist	|W	0x200e805
	 * Creature - Spirit 1/1
	 * Flying
	 * When ~ enters the battlefield, tap target creature an opponent controls.
	 * Delirium - At the beginning of each opponent's upkeep, if there are four or more card types among cards in your graveyard,
	 *	tap target creature that player controls. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = td.preferred_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( new_pick_target(&td, "Select target creature an opponent controls.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance(player, card);
				tap_card(instance->targets[0].player, instance->targets[0].card);
				instance->number_of_targets = 0;
			}
		}
	}

	if( current_turn != player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = td.preferred_controller = 1-player;
		td.allow_cancel = 0;

		upkeep_trigger_ability_mode(player, card, event, 1-player, ((can_target(&td) && delirium(player)) ? RESOLVE_TRIGGER_MANDATORY : 0));
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = td.preferred_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) ){
			if( new_pick_target(&td, "Select target creature an opponent controls.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance(player, card);
				tap_card(instance->targets[0].player, instance->targets[0].card);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

int card_town_gossipmonger(int player, int card, event_t event){
	/* Town Gossipmonger	|W	0x200e80a
	 * Creature - Human 1/1
	 * |T, Tap an untapped creature you control: Transform ~.*/

	double_faced_card(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.allowed_controller = td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.special = TARGET_SPECIAL_NOT_ME;
	td.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST0, 0, &td, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			if( new_pick_target(&td, "Select another untapped creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( ! check_special_flags3(instance->parent_controller, instance->parent_card, SF3_CARD_IS_FLIPPED) ){
			transform(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}


int card_incited_rabble(int player, int card, event_t event){
 /* Incited Rabble	0x200e80f
 * Creature - Human 2/3
 * ~ attacks each combat if able.
 * |2: ~ gets +1/+0 until end of turn. */
	attack_if_able(player, card, event);
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_X(2), 1, 0);
}

/* Unruly Mob	|1|W	0x2004aad
 * Creature - Human 1/1
 * Whenever another creature you control dies, put a +1/+1 counter on ~. */

int card_vessel_of_ephemera(int player, int card, event_t event){
	/* Vessel of Ephemera	|1|W	0x200e814
	 * Enchantment
	 * |2|W, Sacrifice ~: Put two 1/1 |Swhite Spirit creature tokens with flying onto the battlefield. */

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		make_spirit(player, card, player, 2, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_XW(2, 1), 0, NULL, NULL);
}

/*** Blue ***/

int card_aberrant_researcher(int player, int card, event_t event){
	/* Aberrant Researcher	|3|U	0x200e819
	 * Creature - Human Insect 3/2
	 * Flying
	 * At the beginning of your upkeep, put the top card of your library into your graveyard.
	 * If it's an instant or sorcery card, transform ~.*/

	double_faced_card(player, card, event);

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			int iid = deck[0];
			mill(player, 1);
			if( is_what(-1, iid, TYPE_SPELL) ){
				transform(player, card);
			}
		}
	}

	return 0;
}

/*
 * Perfected Form --> vanilla	0x401000
 * Creature - Insect Horror 5/4
 * Flying */

/* Broken Concentration	|1|U|U --> It will never worl as inteded under Manalink. Skipped.
 * Instant
 * Counter target spell.
 * Madness |3|U */

/* Catalog	|2|U	0x200cba9
 * Instant
 * Draw two cards, then discard a card. */

int card_compelling_deterrence(int player, int card, event_t event){
	/* Compelling Deterrence	|1|U	0x200e81e
	 * Instant
	 * Return target nonland permanent to its owner's hand.
	 * Then that player discards a card if you control a Zombie. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.subtype = SUBTYPE_ZOMBIE;

			if( check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &this_test) ){
				discard(instance->targets[0].player, 0, player);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonland permanent.", 1, NULL);
}


int card_confirm_suspicions(int player, int card, event_t event){
	/* Confirm Suspicions	|3|U|U	0x200e823
	 * Instant
	 * Counter target spell.
	 * Investigate three times. */

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
			card_instance_t *instance = get_card_instance( player, card );

			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);

			investigate(player, card, player);
			investigate(player, card, player);
			investigate(player, card, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return counterspell(player, card, event, NULL, 0);
}

int card_daring_sleuth(int player, int card, event_t event){
	/* Daring Sleuth	|1|U	0x200e828
	 * Creature - Human Rogue 2/1
	 * When you sacrifice a Clue, transform ~.*/

	double_faced_card(player, card, event);

	int amount = whenever_a_clue_is_sacrificed(player, card, event, player, RESOLVE_TRIGGER_MANDATORY);
	if( amount ){
		transform(player, card);
		get_card_instance(player, card)->targets[11].card = 0;
	}

	return 0;
}

int card_bearer_of_overwhelming_truths(int player, int card, event_t event){
	/*
	 * Bearer of Overwhelming Truths	0x200e82d
	 * Creature - Human Wizard 3/2
	 * Prowess
	 * Whenever ~ deals combat damage to a player, investigate. */

	prowess(player, card, event);

	if( damage_dealt_by_me_arbitrary(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_MUST_BE_COMBAT_DAMAGE, player, card) ){
		investigate(player, card, player);
	}

	return 0;
}

int card_deny_existence(int player, int card, event_t event){
	/* Deny Existence	|2|U	0x200e832
	 * Instant
	 * Counter target creature spell. If that spell is countered this way, exile it instead of putting it into its owner's graveyard. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, &td, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, &td, 0) ){
			card_instance_t *instance = get_card_instance(player, card);
			if( manage_counterspell_linked_hacks(player, card, instance->targets[0].player, instance->targets[0].card) != KILL_REMOVE ){
				put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
				rfg_top_card_of_deck(instance->targets[0].player);
			}
			else{
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


/* Drownyard Explorers	|3|U --> Thraben Inspector	0x200e800
 * Creature - Human Wizard 2/4
 * When ~ enters the battlefield, investigate. */

int card_drunau_corpse_trawler(int player, int card, event_t event){
	/* Drunau Corpse Trawler	|3|U	0x200e837
	 * Creature - Zombie 1/1
	 * When ~ enters the battlefield, put a 2/2 |Sblack Zombie creature token onto the battlefield.
	 * |2|B: Target Zombie gains deathtouch until end of turn. */

	if( comes_into_play(player, card, event) ){
		make_zombie(player, card, player, 1, 1, 0);
	}

	if( ! IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_ZOMBIE;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, 0, SP_KEYWORD_DEATHTOUCH);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XB(2, 1), 0,
									&td, "Select target Zombie.");
}

int card_engulf_the_shore(int player, int card, event_t event){
	/* Engulf the Shore	|3|U	0x200e83c
	 * Instant
	 * Return to their owners' hands all creatures with toughness less than or equal to the number of |H1Islands you control. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.toughness = count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_ISLAND))+1;
		test.toughness_flag = F5_TOUGHNESS_LESSER_THAN_VALUE;
		APNAP(p, {new_manipulate_all(player, card, p, &test, ACT_BOUNCE);};);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_epiphany_at_the_drownyard(int player, int card, event_t event){
	/* Epiphany at the Drownyard	|X|U	0x200e841
	 * Instant
	 * Reveal the top X plus one cards of your library and separate them into two piles.
	 * An opponent chooses one of those piles. Put that pile into your hand and the other into your graveyard. */

	if (event == EVENT_CAST_SPELL && affect_me(player, card) ){
		ai_modifier += 48;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = get_card_instance(player, card)->info_slot;
		effect_fof(player, 1-player, amount + 1, TUTOR_GRAVE);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 1, NULL);
}

int card_erdwal_illuminator(int player, int card, event_t event){
	/* Erdwal Illuminator	|1|U	0x200e846
	 * Creature - Spirit 1/3
	 * Flying
	 * Whenever you investigate for the first time each turn, investigate an additional time. */

	//The rest of the effect is in "investigate()"

	if( event == EVENT_CLEANUP ){
		get_card_instance(player, card)->info_slot = 0;
	}

	return 0;
}

static void p1_p1_if_spirit(int player, int card){
	if( has_subtype(player, card, SUBTYPE_SPIRIT) ){
		add_1_1_counter(player, card);
	}
}

int card_essence_flux(int player, int card, event_t event){
	/* Essence Flux	|U	0x200e850
	 * Instant
	 * Exile target creature you control, then return that card to the battlefield under its owner's control.
	 If it's a Spirit, put a +1/+1 counter on it. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			blink_effect(instance->targets[0].player, instance->targets[0].card, &p1_p1_if_spirit);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature you control.", 1, NULL);
}


int card_fleeting_memories(int player, int card, event_t event){
	/* Fleeting Memories	|2|U	0x200e855
	 * Enchantment
	 * When ~ enters the battlefield, investigate.
	 * Whenever you sacrifice a Clue, target player puts the top three cards of his or her library into his or her graveyard. */

	if( comes_into_play(player, card, event) ){
		investigate(player, card, player);
	}

	int amount = whenever_a_clue_is_sacrificed(player, card, event, player, RESOLVE_TRIGGER_MANDATORY);
	if( amount ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card);

		instance->number_of_targets = 0;

		if( can_target(&td) ){
			int i;
			for(i=0; i<amount; i++){
				instance->number_of_targets = 0;
				if( new_pick_target(&td, "TARGET_PLAYER", 0, 0) ){
					mill(instance->targets[0].player, 3);
				}
			}
		}

		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_forgotten_creation(int player, int card, event_t event){
	/* Forgotten Creation	|3|U	0x200e85a
	 * Creature - Zombie Horror 3/3
	 * Skulk
	 * At the beginning of your upkeep, you may discard all the cards in your hand. If you do, draw that many cards. */

	skulk(player, card, event);

	if( current_turn == player && upkeep_trigger_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int amount = hand_count[player];
		new_discard_all(player, player);
		draw_cards(player, amount);
	}

	return 0;
}

int card_furtive_homunculus(int player, int card, event_t event){
	/* Furtive Homunculus	|1|U	0x200e85f
	 * Creature - Homunculus 2/1
	 * Skulk */
	skulk(player, card, event);
	return 0;
}

int card_geralfs_masterpiece(int player, int card, event_t event){
	/* Geralf's Masterpiece	|3|U|U	0x200e864
	 * Creature - Zombie Horror 7/7
	 * Flying
	 * ~ gets -1/-1 for each card in your hand.
	 * |3|U, Discard three cards: Return ~ from your graveyard to the battlefield tapped. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result -= hand_count[player];
		}
	}

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XU(3, 1)) ){
			if( hand_count[player] >= 3 ){
				return GA_RETURN_TO_PLAY_MODIFIED;
			}
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XU(3, 1));
		if( spell_fizzled != 1 ){
			new_multidiscard(player, 3, 0, player);
			return GAPAID_REMOVE;
		}
	}

	if( event == EVENT_RETURN_TO_PLAY_FROM_GRAVE_MODIFIED ){
		get_card_instance(player, card)->state |= STATE_TAPPED;
	}

	return 0;
}

int card_ghostly_wings(int player, int card, event_t event){
	/* Ghostly Wings	|1|U	0x200e869
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +1/+1 and has flying.
	 * Discard a card: Return enchanted creature to its owner's hand. */

	if( get_card_instance(player, card)->damage_target_player > -1 ){
		card_instance_t* instance = get_card_instance(player, card);
		int p = instance->damage_target_player, c = instance->damage_target_card;

		if( event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST0, 0, NULL, NULL);
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			bounce_permanent(p, c);
		}
	}

	return generic_aura(player, card, event, player, 1, 1, KEYWORD_FLYING, 0, 0, 0, 0);
}

int card_gone_missing(int player, int card, event_t event){
	/* Gone Missing	|4|U	0x200e86e
	 * Sorcery
	 * Put target permanent on top of its owner's library.
	 * Investigate. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
			investigate(player, card, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_invasive_surgery(int player, int card, event_t event){
	/* Invasive Surgery	|U	0x200e873
	 * Instant
	 * Counter target sorcery spell.
	 * Delirium - If there are four or more card types among cards in your graveyard, search the graveyard, hand, and library
	 * of that spell's controller for any number of cards with the same name as that spell, exile those cards,
	 * then that player shuffles his or her library. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_SORCERY);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int csvid = get_id(instance->targets[0].player, instance->targets[0].card);
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
			if( delirium(player) ){
				lobotomy_effect(player, instance->targets[0].player, csvid, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, &td, "TARGET_SPELL", 1, NULL);
}

int card_jace_unraveler_of_secrets(int player, int card, event_t event){
	/* Jace, Unraveler of Secrets	|3|U|U	0x200e878
	 * Planeswalker - Jace (5)
	 * +1: Scry 1, then draw a card.
	 * -2: Return target creature to its owner's hand.
	 * -8: You get an emblem with "Whenever an opponent casts his or her first spell each turn, counter that spell." */

	if( IS_GAA_EVENT(event) ){
		enum
		{
			CHOICE_SCRY = 1,
			CHOICE_BOUNCE,
			CHOICE_EMBLEM
		};

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance( player, card);

		if( event == EVENT_ACTIVATE ){
			instance->info_slot = instance->number_of_targets = 0;
			int priority[3] = {5, 1, 20};
			priority[1]+= 3*(count_subtype(1-player, TYPE_CREATURE, -1)-count_subtype(player, TYPE_CREATURE, -1));
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
								"Scry & draw",			1, priority[0],	+1,
								"Bounce a creature",	can_target(&td), priority[1],	-2,
								"Emblem",				1, priority[2],	-8);
			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			if( choice == CHOICE_BOUNCE ){
				if( ! new_pick_target(&td, "TARGET_CREATURE", 0, 1) ){
					return 0;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == CHOICE_SCRY ){
				scry(player, 1),
				draw_cards(player, 1);
			}
			if( instance->info_slot == CHOICE_BOUNCE ){
				if( valid_target(&td) ){
					bounce_permanent(instance->targets[0].player, instance->targets[0].card);
				}
			}
			if( instance->info_slot == CHOICE_EMBLEM ){
				generate_reserved_token_by_id(player, CARD_ID_JACE_UNRAVELER_OF_SECRETS_EMBLEM);
			}
		}
	}

	return planeswalker(player, card, event, 5);
}

int card_jace_unraveler_of_secrets_emblem(int player, int card, event_t event){
	/* Jace, Unraveler of Secrets's Emblem	0x200e87d
	 * Emblem - Jace
	 * Whenever an opponent casts his or her first spell each turn, counter that spell. */

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && get_specific_storm_count(1-player) == 1 ){
		if( new_specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, NULL) ){
			real_counter_a_spell(player, card, trigger_cause_controller, trigger_cause);
		}
	}

	return 0;
}

int card_jaces_scrutiny(int player, int card, event_t event){
	/* Jace's Scrutiny	|1|U	0x200e882
	 * Instant
	 * Target creature gets -4/-0 until end of turn.
	 * Investigate. */

	if( IS_GS_EVENT(player, card, event) && event != EVENT_RESOLVE_SPELL){
		return vanilla_instant_pump(player, card, event, ANYBODY, 1-player, -4, 0, 0, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance( player, card);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -4, 0);
			investigate(player, card, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_just_the_wind(int player, int card, event_t event){
	/* Just the Wind	|1|U	0x200e887
	 * Instant
	 * Return target creature to its owner's hand.
	 * Madness |U */

	if( IS_GS_EVENT(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance( player, card);

		if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			}
			kill_card(player, card, KILL_DESTROY);
		}
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
	}

	return madness(player, card, event, MANACOST_U(1));
}

int card_lamplight_of_selhoff(int player, int card, event_t event){
	/* Lamplighter of Selhoff	|4|U	0x200e88c
	 * Creature - Zombie Horror 3/5
	 * When ~ enters the battlefield, if you control another Zombie, you may draw a card. If you do, discard a card. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_ZOMBIE;
		this_test.not_me = 1;

		// intervening-if clause - must be checked both during EVENT_TRIGGER and EVENT_RESOLVE_TRIGGER
		if( check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &this_test) &&
			comes_into_play_mode(player, card, event, IS_AI(player) ? RESOLVE_TRIGGER_MANDATORY : RESOLVE_TRIGGER_OPTIONAL) &&
			can_draw_cards_as_cost(player, 1)
			){
			draw_cards(player, 1);
			discard(player, 0, player);
		}
	}

	return 0;
}

int card_manic_scribe(int player, int card, event_t event){
	/* Manic Scribe	|1|U	0x200e891
	 * Creature - Human Wizard 0/3
	 * When ~ enters the battlefield, each opponent puts the top three cards of his or her library into his or her graveyard.
	 * Delirium - At the beginning of each opponent's upkeep, if there are four or more card types among cards in your graveyard,
	 * that player puts the top three cards of his or her library into his or her graveyard. */

	if( comes_into_play(player, card, event) ){
		mill(1-player, 3);
	}

	upkeep_trigger_ability_mode(player, card, event, 1-player, (delirium(player) ? RESOLVE_TRIGGER_MANDATORY : 0));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		mill(1-player, 3);
	}

	return 0;
}

int card_nagging_thoughts(int player, int card, event_t event){
	/* Nagging Thoughts	|1|U	0x200e896
	 * Sorcery
	 * Look at the top two cards of your library. Put one of them into your hand and the other into your graveyard.
	 * Madness |1|U */

	if( IS_GS_EVENT(player, card, event) ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int cd = count_deck(player);
		if( cd ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, "Select a card to add to your hand");
			this_test.create_minideck = MIN(cd, 2);
			this_test.no_shuffle = 1;
			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 1, AI_MAX_VALUE, &this_test);
			cd--;
			if( cd ){
				mill(player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return madness(player, card, event, MANACOST_XU(1, 1));
}

int card_nephalia_moondrakes(int player, int card, event_t event){
	/* Nephalia Moondrakes	|5|U|U	0x200e89b
	 * Creature - Drake 5/5
	 * Flying
	 * When ~ enters the battlefield, target creature gains flying until end of turn.
	 * |4|U|U, Exile ~ from your graveyard: Creatures you control gain flying until end of turn. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.preferred_controller = player;
		td.allow_cancel = 0;

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( new_pick_target(&td, "TARGET_CREATURE", 0, 0) ){
				card_instance_t *instance = get_card_instance(player, card);
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, KEYWORD_FLYING, 0);
				instance->number_of_targets = 0;
			}
		}
	}

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XU(4, 2)) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XU(4, 2));
		if( spell_fizzled != 1 ){
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		pump_creatures_until_eot(player, card, player, 0, 0, 0, KEYWORD_FLYING, 0, NULL);
	}

	return 0;
}


/* Niblis of Dusk	|2|U --> Jeskai Windscout	0x200ce74
 * Creature - Spirit 2/1
 * Flying
 * Prowess */

int card_ongoing_investigation(int player, int card, event_t event){
	/* Ongoing Investigation	|1|U	0x200e8a0
	 * Enchantment
	 * Whenever one or more creatures you control deal combat damage to a player, investigate.
	 * |1|G, Exile a creature card from your graveyard: Investigate. You gain 2 life. */

	check_damage_test(player, -1, event, DDBM_MUST_DAMAGE_OPPONENT | DDBM_MUST_BE_COMBAT_DAMAGE, player, card, TYPE_CREATURE, NULL);

	if( resolve_damage_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		investigate(player, card, player);
	}

	if( IS_GAA_EVENT(event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to exile.");

		if( event == EVENT_CAN_ACTIVATE ){
			if( generic_activated_ability(player, card, event, 0, MANACOST_XG(1, 1), 0, NULL, NULL) ){
				return new_special_count_grave(player, &this_test);
			}
		}

		if( event == EVENT_ACTIVATE ){
			if( generic_activated_ability(player, card, event, 0, MANACOST_XG(1, 1), 0, NULL, NULL) ){
				if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MIN_VALUE, &this_test) == -1 ){
					spell_fizzled = 1;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *instance = get_card_instance( player, card );
			investigate(instance->parent_controller, instance->parent_card, player);
			gain_life(player, 2);
		}
	}

	return global_enchantment(player, card, event);
}

int card_pieces_of_the_puzzle(int player, int card, event_t event){
	/* Pieces of the Puzzle	|2|U	0x200e8a5
	 * Sorcery
	 * Reveal the top five cards of your library. Put up to two instant and/or sorcery cards from among them into your hand and the rest
	 * into your graveyard. */

	if( event == EVENT_RESOLVE_SPELL ){
		int cd = count_deck(player);
		if( cd > 0 ){
			int to_mill = MIN(cd, 5);
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_SPELL, "Select an instant and/or sorcery card.");
			this_test.create_minideck = to_mill;
			this_test.no_shuffle = 1;
			this_test.qty = MIN(cd, 2);
			to_mill -= new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			if( to_mill ){
				mill(player, to_mill);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_pore_over_the_pages(int player, int card, event_t event){
	/* Pore Over the Pages	|3|U|U	0x200e8aa
	 * Sorcery
	 * Draw three cards, untap up to two lands, then discard a card. */

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 3);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND );
		td.preferred_controller = player;

		card_instance_t *instance = get_card_instance( player, card );

		int i;
		for(i=0; i<2; i++){
			instance->number_of_targets = 0;
			if( new_pick_target(&td, "TARGET_LAND", 0, 0) ){
				untap_card(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				break;
			}
		}

		discard(player, 0, player);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_press_for_answers(int player, int card, event_t event){
	/* Press for Answers	|1|U	0x200e8af
	 * Sorcery
	 * Tap target creature. It doesn't untap during its controller's next untap step.
	 * Investigate. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
			investigate(player, card, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_rattlechains(int player, int card, event_t event){
	/* Rattlechains	|1|U	0x200e8b4
	 * Creature - Spirit 2/1
	 * Flash
	 * Flying
	 * When ~ enters the battlefield, target Spirit gains hexproof until end of turn.
	 * You may cast Spirit spells as though they had flash. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.preferred_controller = player;
		td.required_subtype = SUBTYPE_SPIRIT;
		td.allow_cancel = 0;

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( new_pick_target(&td, "Select target Spirit.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance(player, card);
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HEXPROOF);
				instance->number_of_targets = 0;
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		int i;
		int result = 0;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && ! is_what(player, i, TYPE_LAND) && has_subtype(player, i, SUBTYPE_SPIRIT) &&
				has_mana_to_cast_iid(player, event, get_card_instance(player, i)->internal_card_id)
			  ){
				int tr = can_legally_play_card_in_hand(player, i);
				if( tr ){
					if( tr == 99 ){
						return 99;
					}
					result = tr;
				}
			}
		}
		return result;
	}

	if( event == EVENT_ACTIVATE ){
		int playable[2][hand_count[player]];
		int pc = 0;
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && ! is_what(player, i, TYPE_LAND) && has_subtype(player, i, SUBTYPE_SPIRIT) &&
				has_mana_to_cast_iid(player, event, get_card_instance(player, i)->internal_card_id)
			  ){
				if( can_legally_play_card_in_hand(player, i) ){
					playable[0][pc] = get_card_instance(player, i)->internal_card_id;
					playable[1][pc] = i;
					pc++;
				}
			}
		}

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a Spirit card to play.");

		int selected = select_card_from_zone(player, player, playable[0], pc, 0, AI_MAX_VALUE, -1, &this_test);
		if( selected != -1 ){
			if( charge_mana_from_id(player, -1, event, cards_data[playable[0][selected]].id) ){
				play_card_in_hand_for_free(player, playable[1][selected]);
				cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
			}
			else{
				spell_fizzled = 1;
				return 0;
			}
		}
		else{
			spell_fizzled = 1;
			return 0;
		}
	}

	return flash(player, card, event);
}

/* Reckless Scholar	|2|U	0x2001c7c
 * Creature - Human Wizard 2/1
 * |T: Target player draws a card, then discards a card. */

int card_rise_from_the_tides(int player, int card, event_t event){
	/* Rise from the Tides	|5|U	0x200e8b9
	 * Sorcery
	 * Put a 2/2 |Sblack Zombie creature token onto the battlefield tapped for each instant and sorcery card in your graveyard. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_SPELL);

		make_zombie(player, card, player, new_special_count_grave(player, &this_test), 1, TOKEN_ACTION_TAPPED);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

/* Seagraf Skaab	|1|U --> vanilla	0x401000
 * Creature - Zombie 1/3 */

int card_silburling_snapper(int player, int card, event_t event){
	/* Silburlind Snapper	|5|U	0x200e8be
	 * Creature - Turtle 6/6
	 * ~ can't attack unless you've cast a noncreature spell this turn. */

	if( in_play(player, card) && ! is_humiliated(player, card) && event == EVENT_ATTACK_LEGALITY && affect_me(player, card) ){
		if( get_specific_storm_count(player) <= get_stormcreature_count(player) ){
			event_result = 1;
		}
	}

	return 0;
}

/* Silent Observer	|3|U --> vanilla	0x401000
 * Creature - Spirit 1/5
 * Flying */

/* Sleep Paralysis	|3|U --> Claustrophobia	0x200485f
 * Enchantment - Aura
 * Enchant creature
 * When ~ enters the battlefield, tap enchanted creature.
 * Enchanted creature doesn't untap during its controller's untap step. */

int card_startled_awake(int player, int card, event_t event){
	/* Startled Awake	|2|U|U	0x200e8c3
	 * Sorcery
	 * Target opponent puts the top thirteen cards of his or her library into his or her graveyard.
	 * |3|U|U: Put ~ from your graveyard onto the battlefield transformed. Activate this ability only any time you could cast a sorcery.*/

	double_faced_card(player, card, event);

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XU(3, 2)) && can_sorcery_be_played(player, event) ){
			return GA_RETURN_TO_PLAY_MODIFIED;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XU(3, 2));
		if( spell_fizzled != 1 ){
			return GAPAID_REMOVE;
		}
	}

	if( event == EVENT_RETURN_TO_PLAY_FROM_GRAVE_MODIFIED ){
		transform(player, card);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 13);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_persistent_nightmare(int player, int card, event_t event){
	/* Persistent Nightmare	0x200e8c8
	 * Creature - Nightmare 1/1
	 * Skulk
	 * When ~ deals combat damage to a player, return it to its owner's hand. */

	skulk(player, card, event);

	if( damage_dealt_by_me_arbitrary(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER, player, card) ){
		bounce_permanent(player, card);
	}

	return 0;
}

int card_stitched_mangler(int player, int card, event_t event){
	/* Stitched Mangler	|2|U	0x200e8cd
	 * Creature - Zombie Horror 2/3
	 * ~ enters the battlefield tapped.
	 * When ~ enters the battlefield, tap target creature an opponent controls.
	 * That creature doesn't untap during its controller's next untap step. */

	comes_into_play_tapped(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = td.preferred_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( new_pick_target(&td, "Select target creature an opponent controls.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance(player, card);
				effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

int card_stitchwing_skaab(int player, int card, event_t event){
	/* Stitchwing Skaab	|3|U	0x200e8d2
	 * Creature - Zombie Horror 3/1
	 * Flying
	 * |1|U, Discard two cards: Return ~ from your graveyard to the battlefield tapped. */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XU(1, 1)) && hand_count[player] >= 2 ){
			return GA_RETURN_TO_PLAY_MODIFIED;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XU(1, 1));
		if( spell_fizzled != 1 ){
			new_multidiscard(player, 2, 0, player);
			return GAPAID_REMOVE;
		}
	}

	if( event == EVENT_RETURN_TO_PLAY_FROM_GRAVE_MODIFIED ){
		add_state(player, card, STATE_TAPPED);
	}

	return 0;
}

/* Stormrider Spirit	|4|U	// == Ashcoat Bear
 * Creature - Spirit 3/3
 * Flash
 * Flying */

int card_thing_in_the_ice(int player, int card, event_t event){
	/* Thing in the Ice	|1|U	0x200e8d7
	 * Creature - Horror 0/4
	 * Defender
	 * ~ enters the battlefield with four ice counters on it.
	 * Whenever you cast an instant or sorcery spell, remove an ice counter from ~.
	 * Then if it has no ice counters on it, transform it.*/

	double_faced_card(player, card, event);

	enters_the_battlefield_with_counters(player, card, event, COUNTER_ICE, 4);

	if( prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( count_counters(player, card, COUNTER_ICE) ){
			remove_counter(player, card, COUNTER_ICE);
		}
		if( ! count_counters(player, card, COUNTER_ICE) ){
			transform(player, card);
		}
	}

	return 0;
}

int card_awoken_horror(int player, int card, event_t event){
	/* Awoken Horror	0x200e8dc
	 * Creature - Kraken Horror 7/8
	 * When this creature transforms into ~, return all non-Horror creatures to their owners' hands. */

	if( event == EVENT_TRANSFORMED ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.subtype = SUBTYPE_HORROR;
		test.subtype_flag = DOESNT_MATCH;
		APNAP(p, {new_manipulate_all(player, card, p, &test, ACT_BOUNCE);};);
	}

	return 0;
}

int card_trail_of_evidence(int player, int card, event_t event){
	/* Trail of Evidence	|2|U	0x200e8e1
	 * Enchantment
	 * Whenever you cast an instant or sorcery spell, investigate. */

	if( prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		investigate(player, card, player);
	}

	return global_enchantment(player, card, event);
}

int card_uninvited_geist(int player, int card, event_t event){
	/* Uninvited Geist	|2|U	0x200e8e6
	 * Creature - Spirit 2/2
	 * Skulk
	 * When ~ deals combat damage to a player, transform it.*/

	double_faced_card(player, card, event);

	skulk(player, card, event);

	if( damage_dealt_by_me_arbitrary(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER, player, card) ){
		transform(player, card);
	}

	return 0;
}

/* Unimpeded Trespasser --> Latch Seeker	0x2005b0b
 * Creature - Spirit 3/3
 * ~ can't be blocked. */

int card_vessel_of_paramnesia(int player, int card, event_t event){
	/* Vessel of Paramnesia	|1|U	0x200e8eb
	 * Enchantment
	 * |U, Sacrifice ~: Target player puts the top three cards of his or her library into his or her graveyard. Draw a card. */

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			mill(instance->targets[0].player, 3);
			draw_cards(player, 1);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_U(1), 0, &td, "TARGET_PLAYER");
}


int card_welcome_to_the_fold(int player, int card, event_t event){
	/* Welcome to the Fold	|2|U|U	0x200e8f0
	 * Sorcery
	 * Madness |X|U|U
	 * Gain control of target creature if its toughness is 2 or less.
	 * If ~'s madness cost was paid, instead gain control of that creature if its toughness is X or less. */

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XU(-1, 2));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int valid = 1;
			if( check_special_flags3(player, card, SF3_MADNESS_COST_PAID) ){
				if( get_toughness(instance->targets[0].player, instance->targets[0].card) > instance->info_slot ){
					valid = 0;
				}
			}
			else{
				if( get_toughness(instance->targets[0].player, instance->targets[0].card) > 2 ){
					valid = 0;
				}
			}
			if( valid ){
				gain_control_permanently(player, instance->targets[0].player, instance->targets[0].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

/*** Black ***/

/* Accursed Witch	|3|B --> Skipped both as we can't modify the cost in EVENT_CAST_SPELL
 * Creature - Human Shaman 4/2
 * Spells your opponents cast that target ~ cost |1 less to cast.
 * When ~ dies, return it to the battlefield transformed under your control attached to target opponent.
 * --TRANSFORM--
 * Infectious Curse	""(B)
 * Enchantment - Aura Curse
 * Enchant player
 * Spells you cast that target enchanted player cost |1 less to cast.
 * At the beginning of enchanted player's upkeep, that player loses 1 life and you gain 1 life. */

int card_alms_of_the_vein(int player, int card, event_t event){
	/* Alms of the Vein	|2|B	0x200e8f5
	 * Sorcery
	 * Target opponent loses 3 life and you gain 3 life.
	 * Madness |B */

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_B(1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 3);
			gain_life(player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, "TARGET_PLAYER", 1, NULL);
}

int card_asylum_visitor(int player, int card, event_t event){
	/* Asylum Visitor	|1|B	0x200e8fa
	 * Creature - Vampire Wizard 3/1
	 * At the beginning of each player's upkeep, if that player has no cards in hand, you draw a card and you lose 1 life.
	 * Madness |1|B */

	upkeep_trigger_ability(player, card, event, ANYBODY);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( hand_count[current_turn] == 0 ){
			lose_life(player, 1);
			draw_cards(player, 1);
		}
	}

	return madness(player, card, event, MANACOST_XB(1, 1));
}

int card_behind_the_scenes(int player, int card, event_t event){
	/* Behind the Scenes	|2|B	0x200e8ff
	 * Enchantment
	 * Creatures you control have skulk.
	 * |4|W: Creatures you control get +1/+1 until end of turn. */

	if( event == EVENT_ABILITIES && in_play(player, card) && ! is_humiliated(player, card) ){
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			card_instance_t *instance = get_card_instance(affected_card_controller, affected_card);
			if( instance->targets[16].card < 0 ){
				instance->targets[16].card = 0;
			}
			instance->targets[16].card |= SP_KEYWORD_SKULK;
		}
	}

	if( in_play(player, card) && event == EVENT_BLOCK_LEGALITY && ! is_humiliated(player, card) ){
		if( attacking_card_controller == player ){
			if( get_power(affected_card_controller, affected_card) > get_power(attacking_card_controller, attacking_card) ){
				event_result = 1;
			}
		}
	}

	if( leaves_play(player, card, event) ){
		int c;
		for(c=0; c < active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) ){
				remove_special_ability(player, c, SP_KEYWORD_SKULK);
			}
		}
	}

	if( IS_GAA_EVENT(event) ){
		if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *instance = get_card_instance( player, card );
			pump_creatures_until_eot(instance->parent_controller, instance->parent_card, player, 0, 1, 1, 0, 0, NULL);
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_XW(4, 1), 0, NULL, NULL);
	}

	return global_enchantment(player, card, event);
}


int card_behold_the_beyond(int player, int card, event_t event ){
	/* Behold the Beyond	|5|B|B	0x200e904
	 * Sorcery
	 * Discard your hand. Search your library for three cards and put those cards into your hand. Then shuffle your library. */

	if( event == EVENT_RESOLVE_SPELL ){
		discard_all(player);

		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select a card.");
		test.qty = 3;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_biting_rain(int player, int card, event_t event ){
	/* Biting Rain	|2|B|B	0x200e909
	 * Sorcery
	 * All creatures get -2/-2 until end of turn.
	 * Madness |2|B */

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XB(2, 1));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		APNAP(p, {pump_creatures_until_eot(player, card, p, 0, -2, -2, 0, 0, NULL);};);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_call_the_bloodline(int player, int card, event_t event ){
	/* Call the Bloodline	|1|B	0x200e90e
	 * Enchantment
	 * |1, Discard a card: Put a 1/1 |Sblack Vampire Knight creature token with lifelink onto the battlefield.
	 * Activate this ability only once each turn. */

	if( IS_GAA_EVENT(event) ){
		if( event == EVENT_RESOLVE_ACTIVATION ){
			card_instance_t *instance = get_card_instance( player, card );

			token_generation_t token;
			default_token_definition(instance->parent_controller, instance->parent_card, CARD_ID_VAMPIRE_KNIGHT, &token);
			token.pow = token.tou = 1;
			token.s_key_plus = SP_KEYWORD_LIFELINK;
			token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);
			generate_token(&token);
		}
		return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN | GAA_DISCARD, MANACOST_X(1), 0, NULL, NULL);
	}

	return global_enchantment(player, card, event);
}

int card_creeping_dread(int player, int card, event_t event ){
	/* Creeping Dread	|3|B	0x200e913
	 * Enchantment
	 * At the beginning of your upkeep, each player discards a card.
	 * Each opponent who discarded a card that shares a card type with the card you discarded loses 3 life. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int crds[2] = {-1, -1};
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select a card to discard");
		APNAP(p, {crds[p] = new_select_a_card(player, player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &test);};);
		int ll = (get_type(player, crds[player]) & get_type(1-player, crds[1-player])) ? 1 : 0;
		APNAP(p, {new_discard_card(p, crds[p], player, 0);};);
		if( ll ){
			lose_life(1-player, 3);
		}
	}

	return global_enchantment(player, card, event);
}


int card_crow_of_dark_tidings(int player, int card, event_t event ){
	/* Crow of Dark Tidings	|2|B	0x200e918
	 * Creature - Zombie Bird 2/1
	 * Flying
	 * When ~ enters the battlefield or dies, put the top two cards of your library into your graveyard. */

	if( comes_into_play(player, card, event) || this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		mill(player, 2);
	}

	return 0;
}

/* Dead Weight	|B	=>innistrad.c:card_dead_weight()
 * Enchantment - Aura
 * Enchant creature
 * Enchanted creature gets -2/-2. */

int card_diregraf_colossus(int player, int card, event_t event ){
	/* Diregraf Colossus	|2|B	0x200e91d
	 * Creature - Zombie Giant 2/2
	 * ~ enters the battlefield with a +1/+1 counter on it for each Zombie card in your graveyard.
	 * Whenever you cast a Zombie spell, put a 2/2 |Sblack Zombie creature token onto the battlefield tapped. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, count_graveyard_by_subtype(player, SUBTYPE_ZOMBIE));

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		this_test.subtype = SUBTYPE_ZOMBIE;

		if( new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &this_test) ){
			make_zombie(player, card, player, 1, 1, TOKEN_ACTION_TAPPED);
		}
	}

	return 0;
}

int card_elusive_tormentor(int player, int card, event_t event ){
	/* Elusive Tormentor	|2|B|B	0x200e922
	 * Creature - Vampire Wizard 4/4
	 * |1, Discard a card: Transform ~.*/

	double_faced_card(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance( player, card );
		// See: http://magic.wizards.com/en/articles/archive/feature/shadows-over-innistrad-mechanics
		if( ! check_special_flags3(instance->parent_controller, instance->parent_card, SF3_CARD_IS_FLIPPED) ){
			transform(instance->parent_controller, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_X(1), 0, NULL, NULL);
}

int card_insidious_mist(int player, int card, event_t event ){
 /* Insidious Mist	0x200e927
 * Creature - Elemental 0/1
 * Hexproof, indestructible
 * ~ can't block and can't be blocked.
 * Whenever ~ attacks and isn't blocked, you may pay |2|B. If you do, transform it. */

	hexproof(player, card, event);

	indestructible(player, card, event);

	unblockable(player, card, event);

	cannot_block(player, card, event);

	double_faced_card(player, card, event);

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( is_attacking(player, card) && event == EVENT_DECLARE_BLOCKERS && is_unblocked(player, card) ){
			if( has_mana_multi(player, MANACOST_XB(2, 1)) ){
				charge_mana_multi(player, MANACOST_XB(2, 1));
				if( spell_fizzled != 1 ){
					transform(player, card);
				}
			}
		}
	}

	return 0;
}

int card_ever_after(int player, int card, event_t event){
	/* Ever After	|4|B|B	0x200e92c
	 * Sorcery
	 * Return up to two target creature cards from your graveyard to the battlefield.
	 * Each of those creatures is a |Sblack Zombie in addition to its other colors and types. Put ~ on the bottom of its owner's library. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<get_card_instance(player, card)->info_slot; i++){
			int selected = validate_target_from_grave(player, card, player, i);
			if( selected != -1 ){
				reanimate_permanent(player, card, player, selected, REANIMATE_ADD_BLACK_ZOMBIE);
			}
		}
		deck_ptr[player][count_deck(player)] = get_original_internal_card_id(player, card);
		obliterate_card(player, card);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER | GS_OPTIONAL_TARGET, NULL, NULL, 2, &this_test);
}

int card_farbog_revenant(int player, int card, event_t event){
	/* Farbog Revenant	|2|B	0x200e931
	 * Creature - Spirit 1/3
	 * Skulk
	 * Lifelink */
	skulk(player, card, event);
	lifelink(player, card, event);
	return 0;
}

int card_from_under_the_floorboards(int player, int card, event_t event){
	/* From Under the Floorboards	|3|B|B	0x200e936
	 * Sorcery
	 * Madness |X|B|B
	 * Put three 2/2 |Sblack Zombie creature tokens onto the battlefield tapped and you gain 3 life.
	 * If ~'s madness cost was paid, instead put X of those tokens onto the battlefield tapped and you gain X life. */

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XB(-1, 2));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}


	if( event == EVENT_RESOLVE_SPELL ){
		int amount = check_special_flags3(player, card, SF3_MADNESS_COST_PAID) ? get_card_instance(player, card)->info_slot : 3;
		make_zombie(player, card, player, amount, 1, TOKEN_ACTION_TAPPED);
		gain_life(player, amount);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}


int card_ghoulcallers_accomplice(int player, int card, event_t event){
	/* Ghoulcaller's Accomplice	|1|B	0x200e93b
	 * Creature - Human Rogue 2/2
	 * |3|B, Exile ~ from your graveyard: Put a 2/2 |Sblack Zombie creature token onto the battlefield.
	 * Activate this ability only any time you could cast a sorcery. */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XB(3, 1)) && can_sorcery_be_played(player, event) ){
			return GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		if( charge_mana_multi(player, MANACOST_XB(3, 1)) ){
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		make_zombie(player, card, player, 1, 0, 0);
	}

	return 0;
}

int card_ghoulsteed(int player, int card, event_t event){
	/* Ghoulsteed	|4|B	0x200e940
	 * Creature - Zombie Horse 4/4
	 * |2|B, Discard two cards: Return ~ from your graveyard to the battlefield tapped. */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XB(2, 1)) && hand_count[player] >= 2 ){
			return GA_RETURN_TO_PLAY_MODIFIED;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XB(2, 1));
		if( spell_fizzled != 1 ){
			new_multidiscard(player, 2, 0, player);
			return GAPAID_REMOVE;
		}
	}

	if( event == EVENT_RETURN_TO_PLAY_FROM_GRAVE_MODIFIED ){
		add_state(player, card, STATE_TAPPED);
	}

	return 0;
}

int card_gisas_bidding(int player, int card, event_t event){
	/* Gisa's Bidding	|2|B|B	0x200e945
	 * Sorcery
	 * Put two 2/2 |Sblack Zombie creature tokens onto the battlefield.
	 * Madness |2|B */
	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XB(2, 1));
	}

	if( event == EVENT_RESOLVE_SPELL ){
		make_zombie(player, card, player, 2, 1, 0);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_grotesque_mutation(int player, int card, event_t event){
	/* Grotesque Mutation	|1|B	0x200e94a
	 * Instant
	 * Target creature gets +3/+1 and gains lifelink until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 3, 1, 0, SP_KEYWORD_LIFELINK);
}

int card_heir_of_falkenrath(int player, int card, event_t event){
	/* Heir of Falkenrath	|1|B	0x200e94f
	 * Creature - Vampire 2/1
	 * Discard a card: Transform ~. Activate this ability only once each turn.*/

	double_faced_card(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance( player, card );
		// See: http://magic.wizards.com/en/articles/archive/feature/shadows-over-innistrad-mechanics
		if( ! check_special_flags3(instance->parent_controller, instance->parent_card, SF3_CARD_IS_FLIPPED) ){
			transform(instance->parent_controller, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD | GAA_ONCE_PER_TURN, MANACOST0, 0, NULL, NULL);
}

 /* Heir to the Night	""(B) --> vanilla	0x401000
 * Creature - Vampire Berserker 3/2
 * Flying */

int card_hound_of_the_farbogs(int player, int card, event_t event){
	/* Hound of the Farbogs	|4|B	0x200e954
	 * Creature - Zombie Hound 5/3
	 * Delirium - ~ has menace as long as there are four or more card types among cards in your graveyard. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( delirium(player) ){
			menace(player, card, event);
		}
		else{
			remove_special_ability(player, card, SP_KEYWORD_MENACE);
		}
	}

	return 0;
}

int card_indulgent_aristocrat(int player, int card, event_t event){
	/* Indulgent Aristocrat	|B	0x200e959
	 * Creature - Vampire 1/1
	 * Lifelink
	 * |2, Sacrifice a creature: Put a +1/+1 counter on each Vampire you control. */

	lifelink(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_VAMPIRE;

		card_instance_t *instance = get_card_instance(player, card);

		new_manipulate_all(instance->parent_controller, instance->parent_card, player, &this_test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_CREATURE, MANACOST0, 0, NULL, NULL);
}

int card_kindly_stranger(int player, int card, event_t event){
	/* Kindly Stranger	|2|B	0x200e95e
	 * Creature - Human 2/3
	 * Delirium - |2|B: Transform ~. Activate this ability only if there are four or more card types among cards in your graveyard.*/

	double_faced_card(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( ! delirium(player) ){
			return 0;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance( player, card );
		// See: http://magic.wizards.com/en/articles/archive/feature/shadows-over-innistrad-mechanics
		if( ! check_special_flags3(instance->parent_controller, instance->parent_card, SF3_CARD_IS_FLIPPED) ){
			transform(instance->parent_controller, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XB(2, 1), 0, NULL, NULL);
}

int card_demon_possessed_witch(int player, int card, event_t event){
 /* Demon-Possessed Witch	0x200e963
 * Creature - Human Shaman 4/3
 * When this creature transforms into ~, you may destroy target creature. */

	if( event == EVENT_TRANSFORMED ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		card_instance_t *instance = get_card_instance( player, card);

		if( can_target(&td) && new_pick_target(&td, "TARGET_CREATURE", 0, 0) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

int card_lilianas_indignation(int player, int card, event_t event){
	/* Liliana's Indignation	|X|B	0x200e968
	 * Sorcery
	 * Put the top X cards of your library into your graveyard.
	 * Target player loses 2 life for each creature card put into your graveyard this way. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			special_mill(player, card, get_id(player, card), player, instance->info_slot);
			int ll = 0;
			int i;
			for (i = 0; i< num_cards_milled; i++){
				if( cards_milled[i].source != -1 && is_what(-1,  cards_milled[i].internal_card_id, TYPE_CREATURE) ){
					ll+=2;
				}
			}
			lose_life(instance->targets[0].player, ll);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

/* Macabre Waltz	|1|B	=>dissension.c:card_macabre_waltz()
 * Sorcery
 * Return up to two target creature cards from your graveyard to your hand, then discard a card. */

int card_markov_dreadknight(int player, int card, event_t event){
	/* Markov Dreadknight	|3|B|B	0x200e96d
	 * Creature - Vampire Knight 3/3
	 * Flying
	 * |2|B, Discard a card: Put two +1/+1 counters on ~. */

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
	{
		int cless = get_cost_mod_for_activated_abilities(player, card, MANACOST_XB(2, 1));
		if( ! has_mana_multi(player, MANACOST_XB(cless, 1)) || hand_count[player] < 1 ){
			return 0;
		}
		int i = 1;
		while( has_mana_multi(player, MANACOST_XB(cless*i, i)) && hand_count[player] <= i ){
				i++;
		}
		return i*2;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance( player, card );
		add_1_1_counters(instance->parent_controller, instance->parent_card, 2);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_XB(2, 1), 0, NULL, NULL);
}

int card_merciless_resolve(int player, int card, event_t event){
	/* Merciless Resolve	|2|B	0x200e972
	 * Instant
	 * As an additional cost to cast ~, sacrifice a creature or land.
	 * Draw two cards. */

	if( event == EVENT_CAN_CAST ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND | TYPE_CREATURE);
		if( new_can_sacrifice_as_cost(player, card, &this_test) ){
			return basic_spell(player, card, event);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! controller_sacrifices_a_permanent(player, card, TYPE_CREATURE | TYPE_LAND, 0) ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mindwrack_demon(int player, int card, event_t event){
	/* Mindwrack Demon	|2|B|B	0x200e977
	 * Creature - Demon 4/5
	 * Flying, trample
	 * When ~ enters the battlefield, put the top four cards of your library into your graveyard.
	 * Delirium - At the beginning of your upkeep, you lose 4 life unless there are four or more card types among cards in your graveyard. */

	if( comes_into_play(player, card, event) ){
		mill(player, 4);
	}

	if( current_turn == player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		if( ! delirium(player) ){
			upkeep_trigger_ability(player, card, event, player);
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( ! delirium(player) ){
			lose_life(player, 4);
		}
	}

	return 0;
}

int card_morkrut_necropod(int player, int card, event_t event){
	/* Morkrut Necropod	|5|B	0x200e97c
	 * Creature - Slug Horror 7/7
	 * Menace
	 * Whenever ~ attacks or blocks, sacrifice another creature or land. */

	menace(player, card, event);

	if( ! is_humiliated(player, card) ){
		if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) || blocking(player, card, event) ){
			state_untargettable(player, card, 1);
			controller_sacrifices_a_permanent(player, card, TYPE_CREATURE | TYPE_LAND, SAC_NO_CANCEL);
			state_untargettable(player, card, 0);
		}
	}

	return 0;
}

int card_murderous_colpulsion(int player, int card, event_t event){
	/* Murderous Compulsion	|1|B	0x200e981
	 * Sorcery
	 * Destroy target tapped creature.
	 * Madness |1|B */

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XB(1, 1));
	}

	return card_assassinate(player, card, event);
}

int card_olivias_bloodsworn(int player, int card, event_t event){
	/* Olivia's Bloodsworn	|1|B	0x200e986
	 * Creature - Vampire Soldier 2/1
	 * Flying
	 * ~ can't block.
	 * |R: Target Vampire gains haste until end of turn. */

	cannot_block(player, card, event);

	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_VAMPIRE;
	td.preferred_controller = player;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, 0, SP_KEYWORD_HASTE);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_R(1), 0,
									&td, "Select target Vampire you control.");
}

int card_pale_rider_of_trostad(int player, int card, event_t event){
	/* Pale Rider of Trostad	|1|B	0x200e98b
	 * Creature - Spirit 3/3
	 * Skulk
	 * When ~ enters the battlefield, discard a card. */

	skulk(player, card, event);

	if( comes_into_play(player, card, event) ){
		discard(player, 0, player);
	}

	return 0;
}

int card_pick_the_brain(int player, int card, event_t event){
	/* Pick the Brain	|2|B	0x200e990
	 * Sorcery
	 * Target opponent reveals his or her hand. You choose a nonland card from it and exile that card.
	 * Delirium - If there are four or more card types among cards in your graveyard,
	 * search that player's graveyard, hand, and library for any number of cards with the same name as the exiled card,
	 * exile those cards, then that player shuffles his or her library. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) && hand_count[1-player] > 0 ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a nonland card to exile.");
			this_test.type_flag = DOESNT_MATCH;
			int selected = new_select_a_card(player, 1-player, TUTOR_FROM_HAND, 1, AI_MAX_VALUE, -1, &this_test);
			int csvid = get_id(1-player, selected);
			rfg_card_in_hand(1-player, selected);
			if( delirium(player) ){
				lobotomy_effect(player, 1-player, csvid, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_ONLY_TARGET_OPPONENT, &td, NULL, 1, NULL);
}

int card_rancid_rats(int player, int card, event_t event){
	/* Rancid Rats	|1|B	0x200e995
	 * Creature - Zombie Rat 1/1
	 * Skulk
	 * Deathtouch */
	skulk(player, card, event);
	deathtouch(player, card, event);
	return 0;
}

int card_relentless_dead(int player, int card, event_t event){
	/* Relentless Dead	|B|B	0x200e99a
	 * Creature - Zombie 2/2
	 * Menace
	 * When ~ dies, you may pay |B. If you do, return it to its owner's hand.
	 * When ~ dies, you may pay |X. If you do, return another target Zombie creature card with converted mana cost X
	 * from your graveyard to the battlefield. */

	menace(player, card, event);

	int owner, position;
	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		int abilities[4] = {0, 0, 0, 1};
		enum
		{
			CHOICE_REANIMATE_ME,
			CHOICE_REANIMATE_OTHER,
			CHOICE_REANIMATE_BOTH,
			CHOICE_DO_NOTHING
		};
		if( has_mana(player, COLOR_BLACK, 1) && find_in_owners_graveyard(player, card, &owner, &position) ){
			abilities[0] = 1;
		}
		int ai_choice = -1;
		if( ! graveyard_has_shroud(player) ){
			int i;
			int par = -1;
			int iid = -1;
			if( position > -1 ){
				iid = turn_card_in_grave_face_down(player, position);
			}
			for(i = count_graveyard(player)-1; i>-1; i--){
				if( is_what(-1, get_grave(player)[i], TYPE_CREATURE) &&
					has_subtype_by_id(cards_data[get_grave(player)[i]].id, SUBTYPE_ZOMBIE) )
				{
					int cmc = get_cmc_by_internal_id(get_grave(player)[i]);
					if( player == HUMAN ){
						if( has_mana(player, COLOR_ANY, cmc) ){
							abilities[1] = 1;
							break;
						}
					}
					else{
						if( (abilities[0] && has_mana_multi(player, MANACOST_XB(cmc, 1))) ||
							(! abilities[0] && has_mana_multi(player, MANACOST_X(cmc))) )
						{
							if( cmc > par ){
								par = cmc;
								ai_choice = i;
								abilities[1] = 1;
							}
						}
					}
				}
			}
			if( iid > -1 ){
				turn_card_in_grave_face_up(player, position, iid);
			}
		}
		if( abilities[0] && abilities[1] ){
			abilities[2] = 1;
		}
		int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_NO_CANCEL,
							"Return this to hand", abilities[0], 5,
							"Reanimate zombie", abilities[1], 10,
							"Do both", abilities[2], 15,
							"Do nothing", abilities[3], 1);

		if( choice == CHOICE_REANIMATE_ME || choice == CHOICE_REANIMATE_BOTH){
			charge_mana(player, COLOR_BLACK, 1);
			if( spell_fizzled != 1 ){
				reanimate_permanent(owner, -1, owner, position, REANIMATE_DEFAULT);
			}
			else{
				spell_fizzled = 0;
			}
		}

		if( choice == CHOICE_REANIMATE_OTHER || choice == CHOICE_REANIMATE_BOTH){
			if( player == HUMAN ){
				charge_mana(player, COLOR_ANY, -1);
				if( spell_fizzled != 1 ){
					char buffer[500];
					scnprintf(buffer, 500, "Select target creature card with CMC %d", x_value);

					test_definition_t test;
					new_default_test_definition(&test, TYPE_CREATURE, buffer);
					test.cmc = x_value;
					new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_MAX_VALUE, &test);
				}
			}
			else{
				if( ai_choice > -1 ){
					charge_mana(player, COLOR_ANY, get_cmc_by_internal_id(get_grave(player)[ai_choice]));
					if( spell_fizzled != 1 ){
						reanimate_permanent(player, card, player, ai_choice, REANIMATE_DEFAULT);
					}
				}
			}
		}
	}

	return 0;
}

int card_rottenheart_ghoul(int player, int card, event_t event){
	/* Rottenheart Ghoul	|3|B	0x200e99f
	 * Creature - Zombie 2/4
	 * When ~ dies, target player discards a card. */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			card_instance_t *instance = get_card_instance( player, card);
			discard(instance->targets[0].player, 0, player);
		}
	}

	return 0;
}

int card_sanitarium_skeleton(int player, int card, event_t event){
	/* Sanitarium Skeleton	|B	0x200e9a4
	 * Creature - Skeleton 1/2
	 * |2|B: Return ~ from your graveyard to your hand. */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana_multi(player, MANACOST_XB(2, 1)) ){
			return GA_RETURN_TO_HAND;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XB(2, 1));
		if( spell_fizzled != 1 ){
			return GAPAID_REMOVE;
		}
	}

	return 0;
}


int card_shamble_back(int player, int card, event_t event){
	/* Shamble Back	|B	0x200e9a9
	 * Sorcery
	 * Exile target creature card from a graveyard. Put a 2/2 |Sblack Zombie creature token onto the battlefield. You gain 2 life. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card to exile.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
		if( selected != -1 ){
			rfg_card_from_grave(instance->targets[0].player, selected);
			make_zombie(player, card, player, 1, 1, 0);
			gain_life(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER_BOTH_GRAVES, NULL, NULL, 1, &this_test);
}

int card_sinister_concoction(int player, int card, event_t event){
	/* Sinister Concoction	|B	0x200e9ae
	 * Enchantment
	 * |B, Pay 1 life, Put the top card of your library into your graveyard, Discard a card, Sacrifice ~: Destroy target creature. */

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		if( event == EVENT_CAN_ACTIVATE ){
			return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_SACRIFICE_ME | GAA_DISCARD, MANACOST_B(1), 1, &td, NULL);
		}

		if( event == EVENT_ACTIVATE ){
			generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_DISCARD, MANACOST_B(1), 1, &td, "TARGET_CREATURE");
			if( spell_fizzled != 1 ){
				mill(player, 1);
				kill_card(player, card, KILL_SACRIFICE);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			if( valid_target(&td) ){
				card_instance_t *instance = get_card_instance( player, card );
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}

	}

	return global_enchantment(player, card, event);
}

int card_stallion_of_ashmouth(int player, int card, event_t event){
	/* Stallion of Ashmouth	|3|B	0x200e9b3
	 * Creature - Nightmare Horse 3/3
	 * Delirium - |1|B: ~ gets +1/+1 until end of turn.
	 * Activate this ability only if there are four or more card types among cards in your graveyard. */

	int rval = generic_shade_merge_pt(player, card, event, 0, MANACOST_XB(1, 1), 1, 1);
	if( event == EVENT_CAN_ACTIVATE && rval && !delirium(player)){
		rval = 0;
	}
	return rval;
}

int card_stromkirk_mentor(int player, int card, event_t event){
	/* Stromkirk Mentor	|3|B	0x200e9b8
	 * Creature - Vampire Soldier 4/2
	 * When ~ enters the battlefield, put a +1/+1 counter on another target Vampire you control. */

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.preferred_controller = td.allowed_controller = player;
		td.allow_cancel = 0;
		td.required_subtype = SUBTYPE_VAMPIRE;
		td.special = TARGET_SPECIAL_NOT_ME;

		if( can_target(&td) && new_pick_target(&td, "Select another target Vampire you control.", 0, GS_LITERAL_PROMPT) ){
			card_instance_t *instance = get_card_instance( player, card);
			add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
	}

	return 0;
}

/* Throttle	|4|B	0x20042a6
 * Instant
 * Target creature gets -4/-4 until end of turn. */

int card_to_the_slaughter(int player, int card, event_t event){
	/* To the Slaughter	|2|B	0x200e9bd
	 * Instant
	 * Target player sacrifices a creature or planeswalker.
	 * Delirium - If there are four or more card types among cards in your graveyard, instead that player sacrifices a creature
	 *	and a planeswalker. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( delirium(player) ){
				test_definition_t test;
				new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
				new_sacrifice(player, card, instance->targets[0].player, SAC_NO_CANCEL | SAC_CAUSED, &test);

				new_default_test_definition(&test, TARGET_TYPE_PLANESWALKER, "Select a planeswalker to sacrifice.");
				new_sacrifice(player, card, instance->targets[0].player, SAC_NO_CANCEL | SAC_CAUSED, &test);
			}
			else{
				test_definition_t test;
				new_default_test_definition(&test, TYPE_CREATURE | TARGET_TYPE_PLANESWALKER,
											"Select a creature or planeswalker to sacrifice.");
				new_sacrifice(player, card, instance->targets[0].player, SAC_NO_CANCEL | SAC_CAUSED, &test);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_tooth_collector(int player, int card, event_t event){
	/* Tooth Collector	|2|B	0x200e9c2
	 * Creature - Human Rogue 3/2
	 * When ~ enters the battlefield, target creature an opponent controls gets -1/-1 until end of turn.
	 * Delirium - At the beginning of each opponent's upkeep, if there are four or more card types among cards in your graveyard,
	 *	target creature that player controls gets -1/-1 until end of turn. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = td.preferred_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( new_pick_target(&td, "Select target creature an opponent controls.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance(player, card);
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
				instance->number_of_targets = 0;
			}
		}
	}

	if( current_turn != player && trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = td.preferred_controller = 1-player;
		td.allow_cancel = 0;

		upkeep_trigger_ability_mode(player, card, event, 1-player, ((can_target(&td) && delirium(player)) ? RESOLVE_TRIGGER_MANDATORY : 0));
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = td.preferred_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) ){
			if( new_pick_target(&td, "Select target creature an opponent controls.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance(player, card);
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, -1, -1);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

int card_triskaidekaphobia(int player, int card, event_t event){
	/* Triskaidekaphobia	|3|B	0x200e9c7
	 * Enchantment
	 * At the beginning of your upkeep, choose one -
	 * * Each player with exactly 13 life loses the game, then each player gains 1 life.
	 * * Each player with exactly 13 life loses the game, then each player loses 1 life. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( life[player] == 13 || life[1-player] == 13 ){
			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"13 means death + gains life", 1, 5,
								"13 means death + lose life", 1, 5);

			if( life[player] == life[1-player] ){
				//Game is a tie
			}
			else{
				if( life[player] == 13 ){
					lose_the_game(player);
				}
				if( life[1-player] == 13 ){
					lose_the_game(1-player);
				}
				if( choice == 1 ){
					APNAP(p, {gain_life(p, 1);};);
				}
				if( choice == 2){
					APNAP(p, {lose_life(p, 1);};);
				}
			}
		}
	}

	return 0;
}

int card_twins_of_maurer_estate(int player, int card, event_t event){
	/* Twins of Maurer Estate	|4|B	0x200e9cc
	 * Creature - Vampire 3/5
	 * Madness |2|B */
	return madness(player, card, event, MANACOST_XB(2, 1));
}

/* Vampire Noble	|2|B --> vanilla	0x401000
 * Creature - Vampire 3/2 */

int card_vessel_of_malignity(int player, int card, event_t event){
	/* Vessel of Malignity	|1|B	0x200e9d1
	 * Enchantment
	 * |1|B, Sacrifice ~: Target opponent exiles two cards from his or her hand.
	 * Activate this ability only any time you could cast a sorcery. */

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_ANY, "Select a card to exile.");
			int count = 0;
			while( count < 2 && hand_count[instance->targets[0].player] ){
					new_global_tutor(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, TUTOR_RFG, 1, AI_MIN_VALUE, &test);
					count++;
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_ONLY_TARGET_OPPONENT | GAA_CAN_SORCERY_BE_PLAYED,
									MANACOST_XB(1, 1), 0, &td, NULL);
}


/*** Red ***/

int card_avacyns_judgment(int player, int card, event_t event){
	/* Avacyn's Judgment	|1|R	0x200e9d6
	 * Sorcery
	 * Madness |X|R
	 * ~ deals 2 damage divided as you choose among any number of target creatures and/or players.
	 * If ~'s madness cost was paid, it deals X damage divided as you choose among those creatures and/or players instead. */

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XR(-1, 1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance( player, card);
		instance->number_of_targets = 0;
		int dmg = check_special_flags3(player, card, SF3_MADNESS_COST_PAID) ? instance->info_slot : 2;
		int i;
		for(i=0; i<dmg; i++){
			char buffer[100];
			scnprintf(buffer, 100, "Select target creature or player (%d of %d).", i+1, dmg);
			new_pick_target(&td, buffer, i, 1 | GS_LITERAL_PROMPT);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance( player, card);
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				damage_creature(instance->targets[i].player, instance->targets[i].card, 1, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_bloodmad_vampire(int player, int card, event_t event){
	/* Bloodmad Vampire	|2|R	0x200e9db
	 * Creature - Vampire Berserker 4/1
	 * Whenever ~ deals combat damage to a player, put a +1/+1 counter on it.
	 * Madness |1|R */

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XR(1, 1));
	}

	return card_slith_predator(player, card, event);
}

/* Breakneck Rider	|1|R|R --> Gatstaf Shepherd	0x20048f0
 * Creature - Human Scout Werewolf 3/3
 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/

int card_neck_breaker(int player, int card, event_t event){
 /* Neck Breaker	0x200e9e0
 * Creature - Werewolf 4/3
 * Attacking creatures you control get +1/+0 and have trample.
 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

	werewolf_moon_phases(player, card, event);

	if( in_play(player, card) && ! is_humiliated(player, card) && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && is_attacking(affected_card_controller, affected_card) ){
			if( event == EVENT_POWER ){
				event_result++;
			}
			if( event == EVENT_ABILITIES ){
				event_result |= KEYWORD_TRAMPLE;
			}
		}
	}

	return 0;
}

int card_burn_from_within(int player, int card, event_t event){
	/* Burn from Within	|X|R	0x200e9e5
	 * Sorcery
	 * ~ deals X damage to target creature or player. If a creature is dealt damage this way,
	 * it loses indestructible until end of turn. If that creature would die this turn, exile it instead. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( instance->targets[0].card != -1 ){
				exile_if_would_be_put_into_graveyard(player, card, instance->targets[0].player, instance->targets[0].card, 1);
			}
			damage_target0(player, card, instance->info_slot);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_X_SPELL, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

/* Convicted Killer	|2|R --> Gatstaf Shepherd	0x20048f0
 * Creature - Human Werewolf 2/2
 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/

/* Branded Howler --> Howlpack of Estwald	0x2004945
 * Creature - Werewolf 4/4
 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

int card_dance_with_devils(int player, int card, event_t event){
	/* Dance with Devils	|3|R	0x
	 * Instant
	 * Put two 1/1 |Sred Devil creature tokens onto the battlefield.
	 * They have "When this creature dies, it deals 1 damage to target creature or player." */

	if( event == EVENT_RESOLVE_SPELL ){
		make_devil(player, card, player, 2, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_devils_playground(int player, int card, event_t event){
	/* Devils' Playground	|4|R|R	0x200e9ef
	 * Sorcery
	 * Put four 1/1 |Sred Devil creature tokens onto the battlefield.
	 * They have "When this creature dies, it deals 1 damage to target creature or player." */

	if( event == EVENT_RESOLVE_SPELL ){
		make_devil(player, card, player, 4, 1);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_dissension_in_the_ranks(int player, int card, event_t event){
	/* Dissension in the Ranks	|3|R|R	0x200e9f4
	 * Instant
	 * Target blocking creature fights another target blocking creature. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_BLOCKING;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td, 1) ){
			fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target blocking creature.", 2, NULL);
}

int card_dual_shot(int player, int card, event_t event){
	/* Dual Shot	|R	0x200e9f9
	 * Instant
	 * ~ deals 1 damage to each of up to two target creatures. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if(event == EVENT_RESOLVE_SPELL ){
		card_instance_t *instance = get_card_instance( player, card);
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				damage_creature(instance->targets[i].player, instance->targets[i].card, 1, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_ember_eye_wolf(int player, int card, event_t event){
	/* Ember-Eye Wolf	|1|R	0x200e9fe
	 * Creature - Wolf 1/2
	 * Haste
	 * |1|R: ~ gets +2/+0 until end of turn. */
	haste(player, card, event);
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_XR(1, 1), 2, 0);
}

/* Falkenrath Gorger	|R --> vanilla	0x401000 (the real effect is in "discard_card_impl.")
 * Creature - Vampire Berserker 2/1
 * Each Vampire creature card you own that isn't on the battlefield has madness. The madness cost is equal to its mana cost. */

/* Fiery Temper	|1|R|R	=>torment.c:card_fiery_temper()
 * Instant
 * ~ deals 3 damage to target creature or player.
 * Madness |R */

int card_flameblade_angel(int player, int card, event_t event){
	/* Flameblade Angel	|4|R|R	0x200ea03
	 * Creature - Angel 4/4
	 * Flying
	 * Whenever a source an opponent controls deals damage to you or a permanent you control,
	 * you may have ~ deal 1 damage to that source's controller. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		card_instance_t* instance, *damage = damage_being_dealt(event);
		if( damage &&  damage->damage_source_player == 1-player &&
			damage->damage_target_player == player )
		{
			get_card_instance(player, card)->info_slot++;
		}

		if(trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player
		  && (instance = get_card_instance(player, card))->info_slot > 0)
		{
			if (event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}

			if (event == EVENT_RESOLVE_TRIGGER)
			{
				int i;
				for(i=0; i<instance->info_slot; i++){
					damage_player(1-player, 1, player, card);
				}
				instance->info_slot = 0;
			}
		}
	}

	return 0;
}

/* Gatstaf Arsonists	|4|R --> Gatstaf Shepherd	0x20048f0
 * Creature - Human Werewolf 5/4
 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/

int card_gatstaf_ravagers(int player, int card, event_t event){
 /* Gatstaf Ravagers	0x200ea08
 * Creature - Werewolf 6/5
 * Menace
 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */
	menace(player, card, event);
	werewolf_moon_phases(player, card, event);
	return 0;
}

int card_geier_reach_bandit(int player, int card, event_t event){
	/* Geier Reach Bandit	|2|R	0x200ea0d
	 * Creature - Human Rogue Werewolf 3/2
	 * Haste
	 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/
	haste(player, card, event);
	double_faced_card(player, card, event);
	human_moon_phases(player, card, event);
	return 0;
}

int card_vildin_pack_alpha(int player, int card, event_t event){
 /* Vildin-Pack Alpha	0x200ea12
 * Creature - Werewolf 4/3
 * Whenever a Werewolf enters the battlefield under your control, you may transform it.
 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

	werewolf_moon_phases(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.subtype = SUBTYPE_WEREWOLF;
		if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &this_test) ){
			transform(trigger_cause_controller, trigger_cause);
		}
	}

	return 0;
}

int card_geistblast(int player, int card, event_t event){
	/* Geistblast	|2|R	0x200ea17
	 * Instant
	 * ~ deals 2 damage to target creature or player.
	 * |2|U, Exile ~ from your graveyard: Copy target instant or sorcery spell you control. You may choose new targets for the copy. */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		target_definition_t td;
		counterspell_target_definition(player, card, &td, TYPE_SPELL);
		td.allowed_controller = td.preferred_controller = player;

		if( can_target(&td) && has_mana_multi(player, MANACOST_XU(2, 1)) ){
			return GA_INTERRUPT_ABILITY | GA_BASIC;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi(player, MANACOST_XU(2, 1));
		if( spell_fizzled != 1 ){
			card_instance_t* instance = get_card_instance(player, card);
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
			return GAPAID_EXILE;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATED_GRAVEYARD_ABILITY ){
		target_definition_t td;
		counterspell_target_definition(player, card, &td, TYPE_SPELL);
		td.allowed_controller = td.preferred_controller = player;

		if( counterspell_validate(player, card, &td, 0) ){
			card_instance_t* instance = get_card_instance(player, card);
			copy_spell_from_stack(player, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return card_shock(player, card, event);
}


int card_gibbering_fiend(int player, int card, event_t event){
	/* Gibbering Fiend	|1|R	0x200ea1c
	 * Creature - Devil 2/1
	 * When ~ enters the battlefield, it deals 1 damage to each opponent.
	 * Delirium - At the beginning of each opponent's upkeep, if there are four or more card types among cards in your graveyard,
	 * ~ deals 1 damage to that player. */

	if( comes_into_play(player, card, event) ){
		damage_player(1-player, 1, player, card);
	}

	upkeep_trigger_ability_mode(player, card, event, 1-player, (delirium(player) ? RESOLVE_TRIGGER_MANDATORY : 0));

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(1-player, 1, player, card);
	}

	return 0;
}

int card_goldnight_castigator(int player, int card, event_t event){
	/* Goldnight Castigator	|2|R|R	0x200ea21
	 * Creature - Angel 4/9
	 * Flying, haste
	 * If a source would deal damage to you, it deals double that damage to you instead.
	 * If a source would deal damage to ~, it deals double that damage to ~ instead. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		card_instance_t* damage = damage_being_dealt(event);
		if( damage && damage->info_slot > 0 && damage->damage_target_player == player ){
			if( damage->damage_target_card == -1 || damage->damage_target_card == card ){
				damage->info_slot *= 2;
			}
		}
	}

	return 0;
}

int card_harness_the_storm(int player, int card, event_t event){
	/* Harness the Storm	|2|R	0x200ea26
	 * Enchantment
	 * Whenever you cast an instant or sorcery spell from your hand, you may cast target card with the same name
	 * as that spell from your graveyard. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) ){
			if( ! not_played_from_hand(player, card) ){
				int name = get_card_name(player, card);

				char buffer[500];
				scnprintf(buffer, 500, "Select a card name %s.", cards_ptr[name]->name);
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, buffer);
				this_test.id = name;

				if( new_special_count_grave(player, &this_test) ){
					if( new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_AI(player), NULL) ){
						int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_FIRST_FOUND, -1, &this_test);
						if( selected != -1 ){
							play_card_in_grave_for_free(player, player, selected);
						}
					}
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_howlpack_wolf(int player, int card, event_t event){
	/* Howlpack Wolf	|2|R	0x200ea2b
	 * Creature - Wolf 3/3
	 * ~ can't block unless you control another Wolf or Werewolf. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_BLOCK_LEGALITY && affect_me(player, card) ){
			int can_block_ = 0;
			int c;
			for(c=0; c<active_cards_count[player]; c++){
				if( in_play(player, c) && is_what(player, c, TYPE_PERMANENT) &&
					(has_subtype(player, c, SUBTYPE_WOLF) || has_subtype(player, c, SUBTYPE_WEREWOLF)) )
				{
					can_block_ = 1;
					break;
				}
			}
			if( ! can_block_ ){
				event_result = 1;
			}
		}
	}

	return 0;
}

/* Hulking Devil	|3|R --> vanilla	0x401000
 * Creature - Devil 5/2 */

int card_incorrigible_youths(int player, int card, event_t event){
	/* Incorrigible Youths	|3|R|R	0x200ea30
	 * Creature - Vampire 4/3
	 * Haste
	 * Madness |2|R */
	haste(player, card, event);
	return madness(player, card, event, MANACOST_XR(2, 1));
}

/* Inner Struggle	|3|R --> Repentance	0x200a070
 * Instant
 * Target creature deals damage to itself equal to its power. */

int card_insolent_neonate(int player, int card, event_t event){
	/* Insolent Neonate	|R	0x200ea35
	 * Creature - Vampire 1/1
	 * Menace
	 * Discard a card, Sacrifice ~: Draw a card. */

	menace(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_DISCARD, 0, MANACOST0, NULL, NULL);
}

static void damage_my_blockers(int player, int card, event_t event, int dmg){

	if( ! is_humiliated(player, card) ){
		if( current_turn == player && is_attacking(player, card) && event == EVENT_DECLARE_BLOCKERS ){
			int count = active_cards_count[1-player]-1;
			while( count > -1 ){
					if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
						card_instance_t *this = get_card_instance(1-player, count);
						if( this->blocking == card ){
							damage_creature(1-player, count, dmg, player, card);
						}
					}
					count--;
			}
		}

		if( current_turn != player && event == EVENT_DECLARE_BLOCKERS ){
			card_instance_t *instance = get_card_instance(player, card);
			if( instance->blocking < 255 ){
				damage_creature(1-player, instance->blocking, dmg, player, card);
			}
		}
	}

}

int card_kessig_forgemaster(int player, int card, event_t event){
	/* Kessig Forgemaster	|1|R	0x200ea3a
	 * Creature - Human Shaman Werewolf 2/1
	 * Whenever ~ blocks or becomes blocked by a creature, ~ deals 1 damage to that creature.
	 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/
	double_faced_card(player, card, event);
	human_moon_phases(player, card, event);
	damage_my_blockers(player, card, event, 1);
	return 0;
}

int card_flameheart_werewolf(int player, int card, event_t event){
	/* Flameheart Werewolf	0x200ea3f
	 * Creature - Werewolf 3/2
	 * Whenever ~ blocks or becomes blocked by a creature, ~ deals 2 damage to that creature.
	 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */
	werewolf_moon_phases(player, card, event);
	damage_my_blockers(player, card, event, 2);
	return 0;
}

/* Lightning Axe	|R	=>time_spiral.c:card_lightning_axe()
 * Instant
 * As an additional cost to cast ~, discard a card or pay |5.
 * ~ deals 5 damage to target creature. */

/* Mad Prophet	|3|R	=>avacyn_restored.c:card_mad_prophet()
 * Creature - Human Shaman 2/2
 * Haste
 * |T, Discard a card: Draw a card. */

/* Magmatic Chasm	|1|R --> Falter	0x20038ce
 * Sorcery
 * Creatures without flying can't block this turn. */

int card_malevolent_whispers(int player, int card, event_t event){
	/* Malevolent Whispers	|3|R
	 * Sorcery
	 * Gain control of target creature until end of turn. Untap that creature. It gets +2/+0 and gains haste until end of turn.
	 * Madness |3|R */

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XR(3, 1));
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t* instance = get_card_instance(player, card);
		if( valid_target(&td) ){
			effect_act_of_treason_and_modify_pt_or_abilities(player, card, instance->targets[0].player, instance->targets[0].card, 2,0, 0,SP_KEYWORD_HASTE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_pyre_hound(int player, int card, event_t event){
	/* Pyre Hound	|3|R	0x200ea49
	 * Creature - Elemental Hound 2/3
	 * Trample
	 * Whenever you cast an instant or sorcery spell, put a +1/+1 counter on ~. */

	if( prowess_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_ravenous_bloodseeker(int player, int card, event_t event){
	/* Ravenous Bloodseeker	|1|R	0x200ea4e
	 * Creature - Vampire Berserker 1/3
	 * Discard a card: ~ gets +2/-2 until end of turn. */

	if (event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST){
		int amt = 0;
		if( ! has_mana_for_activated_ability(player, card, MANACOST0) || hand_count[player] < 1 ){
			return 0;
		}
		int cless = get_cost_mod_for_activated_abilities(player, card, MANACOST0);
		while( has_mana_for_activated_ability(player, card, MANACOST_X(cless * amt)) && hand_count[player] <= amt ){
				amt++;
		}
		return event == EVENT_POW_BOOST ? amt*2 : amt*-2;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		return generic_shade_merge_pt(player, card, event, 0, MANACOST0, 2, -2);
	}

	return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST0, 0, NULL, NULL);
}

int card_reduce_to_ashes(int player, int card, event_t event){
	/* Reduce to Ashes	|4|R	0x200ea53
	 * Sorcery
	 * ~ deals 5 damage to target creature. If that creature would die this turn, exile it instead. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			card_instance_t* instance = get_card_instance(player, card);
			exile_if_would_be_put_into_graveyard(player, card, instance->targets[0].player, instance->targets[0].card, 1);
			damage_target0(player, card, 5);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_rush_of_adrenaline(int player, int card, event_t event){
	/* Rush of Adrenaline	|R	0x200ea58
	 * Instant
	 * Target creature gets +2/+1 and gains trample until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 2, 1, KEYWORD_TRAMPLE, 0);
}

/* Sanguinary Mage	|1|R --> Jeskai Windscout	0x200ce74
 * Creature - Vampire Wizard 1/3
 * Prowess */

int card_scourge_wolf(int player, int card, event_t event){
	/* Scourge Wolf	|R|R	0x200ea5d
	 * Creature - Wolf Horror 2/2
	 * First strike
	 * Delirium - ~ has double strike as long as there are four or more card types among cards in your graveyard. */

	if( in_play(player, card) && ! is_humiliated(player, card) && event == EVENT_ABILITIES && affect_me(player, card) ){
		if( delirium(player) ){
			event_result |= KEYWORD_DOUBLE_STRIKE;
		}
	}

	return 0;
}

int card_senseles_rage(int player, int card, event_t event){
	/* Senseless Rage	|1|R	0x
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +2/+2.
	 * Madness |1|R */

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XR(1, 1));
	}

	return generic_aura(player, card, event, player, 2, 2, 0, 0, 0, 0, 0);
}

int card_sin_prodder(int player, int card, event_t event){
	/* Sin Prodder	|2|R	0x200ea67
	 * Creature - Devil 3/2
	 * Menace
	 * At the beginning of your upkeep, reveal the top card of your library.
	 *	Any opponent may have you put that card into your graveyard. If a player does, ~ deals damage to that player equal
	 *	to that card's converted mana cost. Otherwise, put that card into your hand. */

	menace(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 && ai_is_speculating != 1){
			int priorities[2] = {10, 1};
			if( life[1-player]-get_cmc_by_internal_id(deck[0]) < 6 ){
				priorities[0] = 1;
				priorities[1] = 10;
			}
			int choice = DIALOG(player, card, event, DLG_FULLCARD(player, card),
								DLG_SMALLCARD_ID(deck[0]), DLG_NO_CANCEL, DLG_WHO_CHOOSES(1-player),
								"Put this card into opponent's graveyard", priorities[0], 1,
								"Decline", priorities[1], 1);

			if( choice == 1 ){
				int dmg = get_cmc_by_internal_id(deck[0]);
				mill(player, 1);
				damage_player(1-player, dmg, player, card);
			}

			if( choice == 2 ){
				add_card_to_hand(player, deck[0]);
			}
		}
	}

	return 0;
}

static int skin_invasion_legacy(int player, int card, event_t event){

	if( resolve_graveyard_trigger(player, card, event) ){
		card_instance_t* instance = get_card_instance(player, card);
		int owner, position;
		if( find_in_owners_graveyard(instance->targets[0].player, instance->targets[0].card, &owner, &position) ){
			reanimate_permanent(owner, -1, owner, position, REANIMATE_RETURN_TO_PLAY_TRANSFORMED);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_skin_invasion(int player, int card, event_t event){
	/* Skin Invasion	|R	0x200ea6c
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature attacks each combat if able.
	 * When enchanted creature dies, return ~ to the battlefield transformed under your control.*/

	double_faced_card(player, card, event);

	if( in_play(player, card) && ! is_humiliated(player, card) && get_card_instance(player, card)->damage_target_player > -1 ){
		card_instance_t* instance = get_card_instance(player, card);
		if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(instance->damage_target_player, instance->damage_target_card) ){
			instance->targets[11].player = 0;
			count_for_gfp_ability(player, card, event, ANYBODY, TYPE_ANY, NULL);
			if( instance->targets[11].player > 0 ){
				int legacy = create_legacy_effect(player, card, &skin_invasion_legacy);
				get_card_instance(player, legacy)->targets[0].player = player;
				get_card_instance(player, legacy)->targets[0].card = card;
				get_card_instance(player, legacy)->number_of_targets = 1;
			}
		}
	}

	return generic_aura(player, card, event, 1-player, 0, 0, 0, SP_KEYWORD_MUST_ATTACK, 0, 0, 0);
}

/* Skin Shedder --> vanilla	0x401000
 * Creature - Insect Horror 3/4 */

int card_spiteful_motives(int player, int card, event_t event){
	/* Spiteful Motives	|3|R	0x200ea71
	 * Enchantment - Aura
	 * Flash
	 * Enchant creature
	 * Enchanted creature gets +3/+0 and has first strike. */
	return generic_aura(player, card, event, player, 3, 0, KEYWORD_FIRST_STRIKE, 0, 0, 0, 0);
}

int card_stensia_masquerade(int player, int card, event_t event){
	/* Stensia Masquerade	|2|R	0x200ea76
	 * Enchantment
	 * Attacking creatures you control have first strike.
	 * Whenever a Vampire you control deals combat damage to a player, put a +1/+1 counter on it.
	 * Madness |2|R */

	if( event == EVENT_CAN_CAST || event == EVENT_SHOULD_AI_PLAY ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_CAN_PAY_MADNESS_COST || event == EVENT_PAY_MADNESS_COST ){
		return madness(player, card, event, MANACOST_XR(2, 1));
	}

	if( in_play(player, card) && ! is_humiliated(player, card) && affected_card_controller == player ){
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && is_attacking(affected_card_controller, affected_card) ){
			if( event == EVENT_ABILITIES ){
				event_result |= KEYWORD_FIRST_STRIKE;
			}
		}
	}

	return card_rakish_heir(player, card, event);
}

int card_structural_distortion(int player, int card, event_t event){
	/* Structural Distortion	|3|R	0x200ea7b
	 * Sorcery
	 * Exile target artifact or land. ~ deals 2 damage to that permanent's controller. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_LAND);

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			damage_player(instance->targets[0].player, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td,
						"Select target artifact or land.", 1, NULL);
}

/* Tormenting Voice	|1|R	=>khans_of_tarkir.c:card_tormenting_voice()
 * Sorcery
 * As an additional cost to cast ~, discard a card.
 * Draw two cards. */

static const char* is_an_attacking_wolf_or_werewolf(int who_chooses, int player, int card)
{
	if( is_attacking(player, card) && (has_subtype(player, card, SUBTYPE_WEREWOLF) || has_subtype(player, card, SUBTYPE_WOLF)) ){
		return NULL;
	}
	return "must be an attacking Wolf or Werewolf.";
}

int card_ulrichs_kindred(int player, int card, event_t event){
	/* Ulrich's Kindred	|2|R	0x200ea80
	 * Creature - Wolf 3/2
	 * Trample
	 * |3|G: Target attacking Wolf or Werewolf gains indestructible until end of turn. */

	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.extra = (int32_t)is_an_attacking_wolf_or_werewolf;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.preferred_controller = player;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XG(3, 1), 0,
									&td, "Select target attacking Wolf or Werewolf.");
}

int card_uncaged_fury(int player, int card, event_t event){
	/* Uncaged Fury	|2|R	0x200ea85
	 * Instant
	 * Target creature gets +1/+1 and gains double strike until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 1, 1, KEYWORD_DOUBLE_STRIKE, 0);
}

int card_vessel_of_volatility(int player, int card, event_t event){
	/* Vessel of Volatility	|1|R	0x200ea8a
	 * Enchantment
	 * |1|R, Sacrifice ~: Add |R|R|R|R to your mana pool. */

	if( event == EVENT_CAN_ACTIVATE ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->targets[0].player != 66 ){
			if( can_produce_mana(player, card) && can_sacrifice_this_as_cost(player, card) ){
				return has_mana_for_activated_ability(player, card, MANACOST_XR(1, 1));
			}
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->targets[0].player != 66 ){
			if( can_produce_mana(player, card) && can_sacrifice_this_as_cost(player, card) &&
				has_mana_for_activated_ability(player, card, MANACOST_XR(1, 1)) )
			{
				declare_mana_available(player, COLOR_RED, 4);
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		card_instance_t *instance = get_card_instance(player, card);
		instance->targets[0].player = 66;
		generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_XR(1, 1), 0, NULL, NULL);
		instance->targets[0].player = 0;
		if( spell_fizzled != 1 ){
			produce_mana(player, COLOR_RED, 4);
		}
	}

	return global_enchantment(player, card, event);
}

/* Village Messenger	|R --> Gatstaf Shepherd	0x20048f0
 * Creature - Human Werewolf 1/1
 * Haste
 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/

 /* Moonrise Intruder --> Gatstaf Ravagers	0x200ea08
 * Creature - Werewolf 2/2
 * Menace
 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

/* Voldaren Duelist	|3|R --> Fervent Cathar	0x2005df4
 * Creature - Vampire Warrior 3/2
 * Haste
 * When ~ enters the battlefield, target creature can't block this turn. */

static const char* is_creature_or_pwalker(int who_chooses, int player, int card)
{
	if( is_what(player, card, TYPE_CREATURE) || is_planeswalker(player, card) ){
		return NULL;
	}
	return "must be a creature or planeswalker.";
}

int card_wolf_of_devils_breach(int player, int card, event_t event){
	/* Wolf of Devil's Breach	|3|R|R	0x200ea8f
	 * Creature - Elemental Wolf 5/5
	 * Whenever ~ attacks, you may pay |1|R and discard a card.
	 * If you do, ~ deals damage to target creature or planeswalker equal to the discarded card's converted mana cost. */

	store_attackers(player, card, event, RESOLVE_TRIGGER_OPTIONAL, player, card, NULL);

	if (xtrigger_condition() == XTRIGGER_ATTACKING && affect_me(player, card) && reason_for_trigger_controller == player)
	{
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.extra = (int32_t)is_creature_or_pwalker;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

		card_instance_t *instance = get_card_instance(player, card);

		instance->number_of_targets = 0;

		if( has_mana_multi(player, MANACOST_XR(1, 1)) && hand_count[player] > 0 && can_target(&td) ){
			if( resolve_declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
				charge_mana_multi(player, MANACOST_XR(1, 1));
				if( spell_fizzled != 1 ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard.");
					int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_CMC, -1, &this_test);
					if( selected != -1 && new_pick_target(&td, "Select target creature or planeswalker.", 0, GS_LITERAL_PROMPT) ){
						new_discard_card(player, selected, player, 0);
						damage_creature(instance->targets[0].player, instance->targets[0].card, 5, player, card);
					}
				}
			}
		}
	}

	return 0;
}

/*** Green ***/

int card_aim_high(int player, int card, event_t event){
	/* Aim High	|1|G	0x200ea94
	 * Instant
	 * Untap target creature. It gets +2/+2 and gains reach until end of turn. */

	if(event == EVENT_CHECK_PUMP ){
		return vanilla_instant_pump(player, card, event, ANYBODY, player, 2, 2, 0, 0);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			untap_card(instance->targets[0].player, instance->targets[0].card);
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card,
									2, 2, KEYWORD_REACH, 0);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_autumnal_gloom(int player, int card, event_t event){
	/* Autumnal Gloom	|2|G	0x200ea99
	 * Enchantment
	 * |B: Put the top card of your library into your graveyard.
	 * Delirium - At the beginning of your end step, if there are four or more card types among cards in your graveyard, transform ~.*/

	double_faced_card(player, card, event);

	if( IS_GAA_EVENT(event) ){
		if( event == EVENT_RESOLVE_ACTIVATION ){
			mill(player, 1);
		}
		return generic_activated_ability(player, card, event, 0, MANACOST_B(1), 0, NULL, NULL);
	}

	if( current_turn == player && trigger_condition == TRIGGER_EOT && affect_me(player, card) ){
		if( delirium(player) && eot_trigger(player, card, event) ){
			transform(player, card);
		}
	}

	return global_enchantment(player, card, event);
}

/* Ancient of the Equinox --> Aven Fleetwing	0x2004021
 * Creature - Treefolk 4/4
 * Trample, hexproof */

int card_briarbridge_patrol(int player, int card, event_t event){
	/* Briarbridge Patrol	|3|G	0x200ea9e
	 * Creature - Human Warrior 3/3
	 * Whenever ~ deals damage to one or more creatures, investigate.
	 * At the beginning of each end step, if you sacrificed three or more Clues this turn,
	 * you may put a creature card from your hand onto the battlefield. */

	if( damage_dealt_by_me_arbitrary(player, card, event, DDBM_MUST_DAMAGE_CREATURE | DDBM_MUST_BE_COMBAT_DAMAGE, player, card) ){
		investigate(player, card, player);
	}

	if( current_turn == player && trigger_condition == TRIGGER_EOT && affect_me(player, card) ){
		if( get_trap_condition(player, TRAP_CLUES_SACRIFICED) >= 3 &&
			eot_trigger_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player)) )
		{
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to put onto the battlefield.");
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	return 0;
}

int card_byway_courier(int player, int card, event_t event){
	/* Byway Courier	|2|G	0x200eaa3
	 * Creature - Human Scout 3/2
	 * When ~ dies, investigate. */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		investigate(player, card, player);
	}

	return 0;
}

int card_clip_wings(int player, int card, event_t event){
	/* Clip Wings	|1|G	0x200eaa8
	 * Instant
	 * Each opponent sacrifices a creature with flying. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature with flying.");
		this_test.keyword = KEYWORD_FLYING;
		new_sacrifice(player, card, 1-player, SAC_CAUSED|SAC_NO_CANCEL, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_confront_the_unknown(int player, int card, event_t event){
	/* Confront the Unknown	|G	0x200eaad
	 * Instant
	 * Investigate, then target creature gets +1/+1 until end of turn for each Clue you control. */

	if(event == EVENT_CHECK_PUMP ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_PERMANENT);
		this_test.id = CARD_ID_CLUE;
		int amount = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
		return vanilla_instant_pump(player, card, event, ANYBODY, player, amount+1, amount+1, 0, 0);
	}

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			investigate(player, card, player);

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.id = CARD_ID_CLUE;
			int amount = check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test);
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, amount, amount);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

static void make_insect(int s_player, int s_card, int t_player, int qty, int sleight){
	token_generation_t token;
	default_token_definition(s_player, s_card, CARD_ID_INSECT, &token);
	token.t_player = t_player;
	token.qty = qty;
	token.pow = token.tou = 1;
	token.color_forced = (sleight ? get_sleighted_color_test(s_player, s_card, COLOR_TEST_GREEN) : COLOR_TEST_GREEN);
	generate_token(&token);
}

int card_crawling_sensation(int player, int card, event_t event){
	/* Crawling Sensation	|2|G	0x200eab2
	 * Enchantment
	 * At the beginning of your upkeep, you may put the top two cards of your library into your graveyard.
	 * Whenever one or more land cards are put into your graveyard from anywhere for the first time each turn,
	 * put a 1/1 |Sgreen Insect creature token onto the battlefield. */

	upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));

	if (event == EVENT_UPKEEP_TRIGGER_ABILITY){
		mill(player, 2);
	}

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->targets[1].player != 66 ){
			int result = whenever_type_is_put_into_graveyard_from_anywhere(player, card, event, player, TYPE_LAND, NULL);
			if( result ){
				make_insect(player, card, player, 1, 1);
				instance->targets[1].player = 66;
			}
		}
	}

	if( event == EVENT_CLEANUP ){
		get_card_instance(player, card)->targets[1].player = 0;
	}

	return global_enchantment(player, card, event);
}

int card_cryptolith_rite(int player, int card, event_t event){
	/* Cryptolith Rite	|1|G	0x200eab7
	 * Enchantment
	 * Creatures you control have "|T: Add one mana of any color to your mana pool." */

	int result = permanents_you_control_can_tap_for_mana_all_one_color(player, card, event, TYPE_CREATURE, 0, COLOR_TEST_ANY_COLORED, 1);
	if (event == EVENT_CAN_ACTIVATE){
		return result;
	}

	return global_enchantment(player, card, event);
}

int card_cult_of_the_waxing_moon(int player, int card, event_t event){
	/* Cult of the Waxing Moon	|4|G	0x200eabc
	 * Creature - Human Shaman 5/4
	 * Whenever a permanent you control transforms into a non-Human creature, put a 2/2 |Sgreen Wolf creature token onto the battlefield. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_ANOTHER_PERMANENT_HAS_TRANSFORMED && affected_card_controller == player ){
			if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
				! has_subtype(affected_card_controller, affected_card, SUBTYPE_HUMAN) )
			{
				make_wolf(player, card, player, 1, 1);
			}
		}
	}

	return 0;
}

int card_deathcap_cultivator(int player, int card, event_t event){
	/* Deathcap Cultivator	|1|G	0x200eac1
	 * Creature - Human Druid 2/1
	 * |T: Add |B or |G to your mana pool.
	 * Delirium - ~ has deathtouch as long as there are four or more card types among cards in your graveyard. */

	if( event == EVENT_ABILITIES && affected_card_controller == player && in_play(player, card) && ! is_humiliated(player, card) ){
		if( delirium(player) ){
			special_abilities(player, card, event, SP_KEYWORD_DEATHTOUCH, player, card);
		}
	}

	return mana_producing_creature_all_one_color(player, card, event, 5, COLOR_TEST_BLACK | COLOR_TEST_GREEN, 1);
}

int card_duskwatch_recruiter(int player, int card, event_t event){
	/* Duskwatch Recruiter	|1|G	0x200eac6
	 * Creature - Human Warrior Werewolf 2/2
	 * |2|G: Look at the top three cards of your library. You may reveal a creature card from among them and put it into your hand.
	 * Put the rest on the bottom of your library in any order.
	 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/

	double_faced_card(player, card, event);

	human_moon_phases(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( count_deck(player) ){
			int amount = MIN(3, count_deck(player));

			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to reveal.");
			this_test.create_minideck = amount;
			this_test.no_shuffle = 1;

			int selected = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);

			if( selected != -1 ){
				card_instance_t *instance = get_card_instance( player, card);
				reveal_card(instance->parent_controller, instance->parent_card, player, selected);
				amount--;
				put_top_x_on_bottom(player, player, amount);
			}
		}
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XG(2, 1), 0, NULL, NULL);
}

int card_krallenhorde_howler(int player, int card, event_t event){
	/* Krallenhorde Howler	0x200eacb
	 * Creature - Werewolf 3/3
	 * Creature spells you cast cost |1 less to cast.
	 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

	werewolf_moon_phases(player, card, event);

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player &&
			is_what(affected_card_controller, affected_card, TYPE_CREATURE) )
		{
			COST_COLORLESS--;
		}
	}

	return 0;
}

int card_equestrial_skill(int player, int card, event_t event){
	/* Equestrian Skill	|3|G	0x200ead0
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature gets +3/+3.
	 * As long as enchanted creature is a Human, it has trample. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( get_card_instance(player, card)->damage_target_player > -1 ){
			card_instance_t *instance = get_card_instance( player, card);
			int p = instance->damage_target_player;
			int c = instance->damage_target_card;
			if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(p, c) ){
				event_result += 3;
			}
			if( event == EVENT_ABILITIES && affect_me(p, c) && has_subtype(p, c, SUBTYPE_HUMAN) ){
				event_result |= KEYWORD_TRAMPLE;
			}
		}
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

int card_fork_in_the_road(int player, int card, event_t event){
	/* Fork in the Road	|1|G	0x200ead5
	 * Sorcery
	 * Search your library for up to two basic land cards and reveal them. Put one into your hand and the other into your graveyard.
	 * Then shuffle your library. */

	if(event == EVENT_RESOLVE_SPELL ){
		char buffer[100];
		scnprintf(buffer, 100, "Select a basic land card to add to your hand.");
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, buffer);
		this_test.subtype = SUBTYPE_BASIC;
		this_test.no_shuffle = 1;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_FIRST_FOUND, &this_test);

		strcpy(buffer, "Select a basic land card to put into the graveyard.");
		new_default_test_definition(&this_test, TYPE_LAND, buffer);
		this_test.subtype = SUBTYPE_BASIC;
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_GRAVE, 0, AI_FIRST_FOUND, &this_test);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

/* Gloomwidow	|2|G --> card_skywinder_drake()	0x2004049
 * Creature - Spider 3/3
 * Reach
 * ~ can block only creatures with flying. */

int card_graf_mole(int player, int card, event_t event){
	/* Graf Mole	|2|G	0x
	 * Creature - Mole Beast 2/4
	 * Whenever you sacrifice a Clue, you gain 3 life. */

	int amount = whenever_a_clue_is_sacrificed(player, card, event, player, RESOLVE_TRIGGER_MANDATORY);
	if( amount ){
		gain_life(player, amount*3);
		get_card_instance( player, card)->targets[11].card = 0;
	}

	return 0;
}

int card_groundskeeper(int player, int card, event_t event){
	/* Groundskeeper	|G	0x200eadf
	 * Creature - Human Druid 1/1
	 * |1|G: Return target basic land card from your graveyard to your hand. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_LAND, "Select target basic land card.");
	this_test.subtype = SUBTYPE_BASIC;

	if( event == EVENT_CAN_ACTIVATE ){
		if( new_special_count_grave(player, &this_test) && ! graveyard_has_shroud(player) ){
			return generic_activated_ability(player, card, event, 0, MANACOST_XG(1, 1), 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		generic_activated_ability(player, card, event, 0, MANACOST_XG(1, 1), 0, NULL, NULL);
		if( spell_fizzled != 1 ){
			if( select_target_from_grave_source(player, card, player, 0, AI_FIRST_FOUND, &this_test, 0) == -1 ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave_source(player, card, player, 0);
		if( spell_fizzled != 1 ){
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
	}

	return 0;
}

int card_hermit_of_the_natterknolls(int player, int card, event_t event){
	/* Hermit of the Natterknolls	|2|G	0x200eae4
	 * Creature - Human Werewolf 2/3
	 * Whenever an opponent casts a spell during your turn, draw a card.
	 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/

	double_faced_card(player, card, event);

	human_moon_phases(player, card, event);

	if( current_turn == player && new_specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, NULL) ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_lone_wolf_of_the_natterknolls(int player, int card, event_t event){
	/* Lone Wolf of the Natterknolls	0x200eae9
	 * Creature - Werewolf 3/5
	 * Whenever an opponent casts a spell during your turn, draw two cards.
	 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

	werewolf_moon_phases(player, card, event);

	if( current_turn == player && new_specific_spell_played(player, card, event, 1-player, RESOLVE_TRIGGER_MANDATORY, NULL) ){
		draw_cards(player, 2);
	}

	return 0;
}

/* Hinterland Logger	|1|G --> Gatstaf Shepherd	0x20048f0
 * Creature - Human Werewolf 2/1
 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/

/* Timber Shredder --> Howlpack of Estwald	0x2004945
 * Creature - Werewolf 4/2
 * Trample
 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

int card_howlpack_resurgence(int player, int card, event_t event){
	/* Howlpack Resurgence	|2|G	0x200eaee
	 * Enchantment
	 * Flash
	 * Each creature you control that's a Wolf or a Werewolf gets +1/+1 and has trample. */

	if( event == EVENT_CAN_CAST || event == EVENT_SHOULD_AI_PLAY ){
		return global_enchantment(player, card, event);
	}

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_ABILITIES) &&
			affected_card_controller == player &&
			is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			(	has_subtype(affected_card_controller, affected_card, SUBTYPE_WOLF) ||
				has_subtype(affected_card_controller, affected_card, SUBTYPE_WEREWOLF)
			) )
		{
			switch( event ){
					case EVENT_POWER:
					case EVENT_TOUGHNESS:
						event_result++;
						break;

					case EVENT_ABILITIES:
						event_result |= KEYWORD_TRAMPLE;
						break;

					default:
						break;
			}
		}
	}

	return flash(player, card, event);
}

int card_inexorable_blob(int player, int card, event_t event){
	/* Inexorable Blob	|2|G	0x200eaf3
	 * Creature - Ooze 3/3
	 * Delirium - Whenever ~ attacks, if there are four or more card types among cards in your graveyard,
	 * put a 3/3 |Sgreen Ooze creature token onto the battlefield tapped and attacking. */

	store_attackers(player, card, event, 0, player, card, NULL);

	if (xtrigger_condition() == XTRIGGER_ATTACKING && affect_me(player, card) && reason_for_trigger_controller == player)
	{
		if( delirium(player) && resolve_declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_OOZE, &token);
			token.pow = token.tou = 3;
			token.action = TOKEN_ACTION_ATTACKING;
			token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
			generate_token(&token);
		}
	}

	return 0;
}

int card_intrepid_provisioners(int player, int card, event_t event){
	/* Intrepid Provisioner	|3|G	0x200eaf8
	 * Creature - Human Scout 3/3
	 * Trample
	 * When ~ enters the battlefield, another target Human you control gets +2/+2 until end of turn. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.allowed_controller = td.preferred_controller = player;
		td.required_subtype = SUBTYPE_HUMAN;
		td.special = TARGET_SPECIAL_NOT_ME;
		td.allow_cancel = 0;

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( new_pick_target(&td, "Select target Human you control.", 0, GS_LITERAL_PROMPT) ){
				card_instance_t *instance = get_card_instance(player, card);
				pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

int card_kessig_dire_swine(int player, int card, event_t event){
	/* Kessig Dire Swine	|4|G|G	0x200eafd
	 * Creature - Boar Horror 6/6
	 * Delirium - ~ has trample as long as there are four or more card types among cards in your graveyard. */
	delirium_boost(player, card, event, 0, 0, KEYWORD_TRAMPLE, 0);
	return 0;
}

int card_lambholt_pacifist(int player, int card, event_t event){
	/* Lambholt Pacifist	|1|G	0x200ec24
	 * Creature - Human Shaman Werewolf 3/3
	 * ~ can't attack unless you control a creature with power 4 or greater.
	 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/

	double_faced_card(player, card, event);

	human_moon_phases(player, card, event);

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.power = 3;
			this_test.power_flag = F5_POWER_GREATER_THAN_VALUE;
			if( ! check_battlefield_for_special_card(player, card, player, CBFSC_CHECK_ONLY, &this_test) ){
				event_result = 1;
			}
		}
	}

	return 0;
}

/* Lambholt Butcher --> Howlpack of Estwald	0x2004945
 * Creature - Werewolf 4/4
 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

/* Loam Dryad	|G --> Springleaf Drum	0x2001222
 * Creature - Dryad Horror 1/2
 * |T, Tap an untapped creature you control: Add one mana of any color to your mana pool. */

int card_might_beyond_reason(int player, int card, event_t event){
	/* Might Beyond Reason	|3|G	0x200eb02
	 * Instant
	 * Put two +1/+1 counters on target creature.
	 * Delirium - Put three +1/+1 counters on that creature instead if there are four or more card types among cards in your graveyard. */

	if( ! IS_GS_EVENT(player, card, event)  ){
		return vanilla_instant_pump(player, card, event, ANYBODY, player, (delirium(player) ? 3 : 2), (delirium(player) ? 3 : 2), 0, 0);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, (delirium(player) ? 3 : 2));
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_moldgraf_scavenger(int player, int card, event_t event){
	/* Moldgraf Scavenger	|1|G	0x200eb07
	 * Creature - Fungus 0/4
	 * Delirium - ~ gets +3/+0 as long as there are four or more card types among cards in your graveyard. */
	delirium_boost(player, card, event, 3, 0, 0, 0);
	return 0;
}

int card_moonlight_hunt(int player, int card, event_t event){
	/* Moonlight Hunt	|1|G	0x200eb0c
	 * Instant
	 * Choose target creature you don't control. Each creature you control that's a Wolf or a Werewolf
	 * deals damage equal to its power to that creature. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int c;
			for(c=active_cards_count[player]-1; c>-1; c--){
				if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) &&
					( has_subtype(player, c, SUBTYPE_WOLF) || has_subtype(player, c, SUBTYPE_WEREWOLF) ) )
				{
					damage_creature(instance->targets[0].player, instance->targets[0].card, get_power(player, c), player, c);
				}
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_obsessive_skinner(int player, int card, event_t event){
	/* Obsessive Skinner	|1|G	0x200eb11
	 * Creature - Human Rogue 1/1
	 * When ~ enters the battlefield, put a +1/+1 counter on target creature.
	 * Delirium - At the beginning of each opponent's upkeep, if there are four or more card types among cards in your graveyard,
	 * put a +1/+1 counter on target creature. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.preferred_controller = player;
		td.allow_cancel = 0;

		if( can_target(&td) && comes_into_play(player, card, event) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				card_instance_t *instance = get_card_instance(player, card);
				add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				instance->number_of_targets = 0;
			}
		}
	}

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.preferred_controller = player;
		td.allow_cancel = 0;

		upkeep_trigger_ability_mode(player, card, event, 1-player, ((can_target(&td) && delirium(player)) ? RESOLVE_TRIGGER_MANDATORY : 0));
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.preferred_controller = player;
		td.allow_cancel = 0;

		if( can_target(&td) ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				card_instance_t *instance = get_card_instance(player, card);
				add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
				instance->number_of_targets = 0;
			}
		}
	}

	return 0;
}

int card_pack_guardian(int player, int card, event_t event){
	/* Pack Guardian	|2|G|G	0x200eb16
	 * Creature - Wolf Spirit 4/3
	 * Flash
	 * When ~ enters the battlefield, you may discard a land card. If you do, put a 2/2 |Sgreen Wolf creature token onto the battlefield. */

	if( hand_count[player] > 0 && comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land card to discard.");
		int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
		if( selected != -1 ){
			new_discard_card(player, selected, player, 0);
			make_wolf(player, card, player, 1, 1);
		}
	}

	return flash(player, card, event);
}

int card_quilled_wolf(int player, int card, event_t event){
	/* Quilled Wolf	|1|G	0x200eb1b
	 * Creature - Wolf 2/2
	 * |5|G: ~ gets +4/+4 until end of turn. */
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_XG(5, 1), 4, 4);
}

/* Rabid Bite	|1|G --> Tail Slash	0x200db30
 * Sorcery
 * Target creature you control deals damage equal to its power to target creature you don't control. */

int card_root_out(int player, int card, event_t event ){
	/* Root Out	|2|G	0x200eb20
	 * Sorcery
	 * Destroy target artifact or enchantment.
	 * Investigate. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			investigate(player, card, player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "DISENCHANT", 1, NULL);
}

int card_sage_of_ancient_lore(int player, int card, event_t event ){
	/* Sage of Ancient Lore	|4|G	0x200eb25
	 * Creature - Human Shaman Werewolf 100/100
	 * ~'s power and toughness are each equal to the number of cards in your hand.
	 * When ~ enters the battlefield, draw a card.
	 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/

	double_faced_card(player, card, event);

	human_moon_phases(player, card, event);

	if( comes_into_play(player, card, event) ){
		draw_cards(player, 1);
	}

	if( player >= 0 && card >=0 && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result += hand_count[player];
		}
	}

	return 0;
}


int card_werewolf_of_ancient_hunger(int player, int card, event_t event ){
	/* Werewolf of Ancient Hunger	0x200eb2a
	 * Creature - Werewolf 100/100
	 * Vigilance, trample
	 * ~'s power and toughness are each equal to the total number of cards in all players' hands.
	 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

	werewolf_moon_phases(player, card, event);

	vigilance(player, card, event);

	if( player >= 0 && card >=0 && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result += hand_count[player];
			event_result += hand_count[1-player];
		}
	}

	return 0;
}

int card_seasons_past(int player, int card, event_t event){
	/* Seasons Past	|4|G|G	0x200eb2f
	 * Sorcery
	 * Return any number of cards with different converted mana costs from your graveyard to your hand.
	 * Put ~ on the bottom of its owner's library. */

	if(event == EVENT_RESOLVE_SPELL ){
		int costs[16];
		int i;
		for(i=0; i<16; i++){
			costs[i] = 0;
		}
		while( 1 ){
				int count = 0;
				int grave_clone[count_graveyard(player)];
				while( count < count_graveyard(player) ){
						if( ! costs[get_cmc_by_internal_id(get_grave(player)[count])] ){
							grave_clone[count] = get_grave(player)[count];
						}
						else{
							grave_clone[count] = iid_draw_a_card;
						}
						count++;
				}
				if( ! count ){
					break;
				}

				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to return to your hand.");
				this_test.id = cards_data[iid_draw_a_card].id;
				this_test.id_flag = DOESNT_MATCH;
				int selected = select_card_from_zone(player, player, grave_clone, count, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					costs[get_cmc_by_internal_id(grave_clone[selected])] = 1;
					add_card_to_hand(player, get_grave(player)[selected]);
					remove_card_from_grave(player, selected);
				}
				else{
					break;
				}
		}
		deck_ptr[player][count_deck(player)] = get_original_internal_card_id(player, card);
		obliterate_card(player, card);
	}

	return basic_spell(player, card, event);
}

int card_second_harvest(int player, int card, event_t event){
	/* Second Harvest	|2|G|G	0x200eb34
	 * Instant
	 * For each token you control, put a token onto the battlefield that's a copy of that permanent. */

	if( event == EVENT_RESOLVE_SPELL ){
		copy_all_tokens(player, card, TYPE_CREATURE, NULL);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_silverful_partisan(int player, int card, event_t event){
	/* Silverfur Partisan	|2|G	0x200eb39
	 * Creature - Wolf Warrior 2/2
	 * Trample
	 * Whenever a Wolf or Werewolf you control becomes the target of an instant or sorcery spell,
	 * put a 2/2 |Sgreen Wolf creature token onto the battlefield. */

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && reason_for_trigger_controller == player ){
		card_instance_t* instance = get_card_instance(trigger_cause_controller, trigger_cause);
		int good = 0;
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( instance->targets[i].player == player && instance->targets[i].card != -1 ){
				if( has_subtype(instance->targets[i].player, instance->targets[i].card, SUBTYPE_WOLF) ||
					has_subtype(instance->targets[i].player, instance->targets[i].card, SUBTYPE_WEREWOLF) )
				{
					good = 1;
					break;
				}
			}
		}

		if( good ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_SPELL);
			if( new_specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, &this_test) ){
				make_wolf(player, card, player, 1, 1);
			}
		}
	}

	return 0;
}

/* Solitary Hunter	|3|G --> Gatstaf Shepherd	0x20048f0
 * Creature - Human Warrior Werewolf 3/4
 * At the beginning of each upkeep, if no spells were cast last turn, transform ~.*/

/* One of the Pack --> Howlpack of Estwald	0x2004945
 * Creature - Werewolf 5/6
 * At the beginning of each upkeep, if a player cast two or more spells last turn, transform ~. */

int card_soul_swallower(int player, int card, event_t event){
	/* Soul Swallower	|2|G|G	0x200eb3e
	 * Creature - Wurm 3/3
	 * Trample
	 * Delirium - At the beginning of your upkeep, if there are four or more card types among cards in your graveyard,
	 * put three +1/+1 counters on ~. */

	if( trigger_condition == TRIGGER_UPKEEP && affect_me(player, card) ){
		upkeep_trigger_ability_mode(player, card, event, player, (delirium(player) ? RESOLVE_TRIGGER_MANDATORY : 0));
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( delirium(player) ){
			add_1_1_counters(player, card, 3);
		}
	}

	return 0;
}

/* Stoic Builder	|2|G --> Cartographer	0x2003662
 * Creature - Human 2/3
 * When ~ enters the battlefield, you may return target land card from your graveyard to your hand. */

/* Thornhide Wolves	|4|G --> vanilla	0x401000
 * Creature - Wolf 4/5 */

int card_tireless_tracker(int player, int card, event_t event){
	/* Tireless Tracker	|2|G	0x200eb43
	 * Creature - Human Scout 3/2
	 * Whenever a land enters the battlefield under your control, investigate.
	 * Whenever you sacrifice a Clue, put a +1/+1 counter on ~. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &this_test) ){
			investigate(player, card, player);
		}
	}

	int amount = whenever_a_clue_is_sacrificed(player, card, event, player, RESOLVE_TRIGGER_MANDATORY);
	if( amount ){
		add_1_1_counters(player, card, amount);
		get_card_instance( player, card)->targets[11].card = 0;
	}

	return 0;
}

int card_traverse_the_ulvenwald(int player, int card, event_t event){
	/* Traverse the Ulvenwald	|G	0x200eb48
	 * Sorcery
	 * Search your library for a basic land card, reveal it, put it into your hand, then shuffle your library.
	 * Delirium - If there are four or more card types among cards in your graveyard, instead search your library
	 * for a creature or land card, reveal it, put it into your hand, then shuffle your library. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		if( delirium(player) ){
			new_default_test_definition(&this_test, TYPE_LAND | TYPE_CREATURE, "Select a creature or land card.");
		}
		else{
			new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
			this_test.subtype = SUBTYPE_BASIC;
		}
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_ulvenwald_hydra(int player, int card, event_t event){
	/* Ulvenwald Hydra	|4|G|G	0x200eb4d
	 * Creature - Hydra 100/100
	 * Reach
	 * ~'s power and toughness are each equal to the number of lands you control.
	 * When ~ enters the battlefield, you may search your library for a land card, put it onto the battlefield tapped,
	 * then shuffle your library. */

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_MAX_VALUE, &this_test);
	}

	if( player >= 0 && card >=0 && ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result += count_subtype(player, TYPE_LAND, -1);
		}
	}

	return 0;
}

int card_ulvenwald_mysteries(int player, int card, event_t event){
	/* Ulvenwald Mysteries	|2|G	0x200eb52
	 * Enchantment
	 * Whenever a nontoken creature you control dies, investigate.
	 * Whenever you sacrifice a Clue, put a 1/1 |Swhite Human Soldier creature token onto the battlefield. */

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		card_instance_t* instance = get_card_instance(player, card);
		if( instance->targets[1].player < 0 ){
			instance->targets[1].player = 0;
		}
		if( instance->targets[1].card < 0 ){
			instance->targets[1].card = 0;
		}
		// instance->targets[1].player --> "Whenever a nontoken creature you control dies..."
		int prev = instance->targets[11].player;
		if( ! is_token(affected_card_controller, affected_card) ){
			count_for_gfp_ability(player, card, event, player, TYPE_CREATURE, NULL);
		}
		if( instance->targets[11].player > prev ){
			instance->targets[1].player++;
		}

		// instance->targets[1].card --> "Whenever you sacrifice a Clue..."
		prev = instance->targets[11].player;
		if( get_card_instance(affected_card_controller, affected_card)->kill_code == KILL_SACRIFICE &&
			get_id(affected_card_controller, affected_card) == CARD_ID_CLUE )
		{
			count_for_gfp_ability(player, card, event, player, TYPE_PERMANENT, NULL);
		}
		if( instance->targets[11].player > prev ){
			instance->targets[1].card++;
		}
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t* instance = get_card_instance(player, card);
		int i;
		for(i=0; i<instance->targets[1].player; i++){
			investigate(player, card, player);
		}

		make_human_soldier(player, card, player, instance->targets[1].card, 1);

		instance->targets[1].player = instance->targets[1].card = 0;
		instance->targets[11].card = 0;
	}

	return global_enchantment(player, card, event);
}

int card_vessel_of_nascency(int player, int card, event_t event){
	/* Vessel of Nascency	|G	0x200eb57
	 * Enchantment
	 * |1|G, Sacrifice ~: Reveal the top four cards of your library. You may put an artifact, creature, enchantment, land,
	 * or planeswalker card from among them into your hand. Put the rest into your graveyard. */

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( count_deck(player) ){
			int qty = MIN(4, count_deck(player));
			reveal_top_cards_of_library(player, qty);

			test_definition_t test;
			new_default_test_definition(&test, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_ENCHANTMENT | TYPE_LAND | TARGET_TYPE_PLANESWALKER,
										"Select an artifact, creature, enchantment, land, or planeswalker card.");
			test.create_minideck = qty;
			test.no_shuffle = 1;
			int selected = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
			if( selected != -1 ){
				qty--;
				if( qty ){
					mill(player, qty);
				}
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_XG(1, 1), 0, NULL, NULL);
}

int card_veteran_cathar(int player, int card, event_t event){
	/* Veteran Cathar	|1|G	0x200eb5c
	 * Creature - Human Soldier 2/2
	 * |3|W: Target Human gains double strike until end of turn. */

	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.required_subtype = SUBTYPE_HUMAN;

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		card_instance_t *instance = get_card_instance(player, card);
		pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
								0, 0, KEYWORD_DOUBLE_STRIKE, 0);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XW(3, 1), 0, &td, "Select target Human.");
}

int card_watcher_in_the_web(int player, int card, event_t event){
	/* Watcher in the Web	|4|G	0x200eb61
	 * Creature - Spider 2/5
	 * Reach
	 * ~ can block an additional seven creatures each combat. */
	creature_can_block_additional(player, card, event, 7);
	return 0;
}


int card_weirding_wood(int player, int card, event_t event){
	/* Weirding Wood	|2|G	0x200eb66
	 * Enchantment - Aura
	 * Enchant land
	 * When ~ enters the battlefield, investigate.
	 * Enchanted land has "|T: Add two mana of any one color to your mana pool." */

	if (comes_into_play(player, card, event)){
		investigate(player, card, player);
	}

	return enchanted_land_has_T_add_n_mana_of_any_one_color(player, card, event, 2);
}

/*** Multi ***/

int card_altered_ego(int player, int card, event_t event){
	/* Altered Ego	|X|2|G|U	0x200eb6b
	 * Creature - Shapeshifter 0/0
	 * ~ can't be countered.
	 * You may have ~ enter the battlefield as a copy of any creature on the battlefield,
	 * except it enters with X additional +1/+1 counters on it. */

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( ! played_for_free(player, card) && ! is_token(player, card) ){
			charge_mana(player, COLOR_ANY, -1);
			if( spell_fizzled != 1 ){
				set_x_for_x_spells(player, card, event, x_value);
				add_1_1_counters(player, card, get_card_instance(player, card)->info_slot);
			}
		}
	}

	return card_clone(player, card, event);
}

int card_anguish_unmaking(int player, int card, event_t event){
	/* Anguished Unmaking	|1|W|B	0x200eb70
	 * Instant
	 * Exile target nonland permanent. You lose 3 life. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			lose_life(player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target nonland permanent.", 1, NULL);
}

int card_arlinn_kord(int player, int card, event_t event){
	/* Arlinn Kord	|2|R|G	0x200eb75
	 * Planeswalker - Arlinn (3)
	 * +1: Until end of turn, up to one target creature gets +2/+2 and gains vigilance and haste.
	 * 0: Put a 2/2 |Sgreen Wolf creature token onto the battlefield. Transform ~.*/

	double_faced_card(player, card, event);

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.preferred_controller = player;
		td.allow_cancel = 3;	// Both done and cancel buttons

		card_instance_t *instance = get_card_instance( player, card);

		enum
		{
			CHOICE_PUMP = 1,
			CHOICE_WOLF,
		};

		if( event == EVENT_ACTIVATE ){
			instance->info_slot = instance->number_of_targets = 0;
			int priority[2] = {5, (! count_subtype(player, TYPE_CREATURE, -1) ? 10 : 1) };
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
								"Pump creature",		1,	priority[0],	+1,
								"Wolf & Transform",		1,	priority[1],	0);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}

			if( choice == CHOICE_PUMP ){
				instance->number_of_targets = 0;
				new_pick_target(&td, "TARGET_CREATURE", 0, 0);
				if (instance->targets[0].card == -1){
					cancel = 1;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int pl = instance->parent_controller;
			int ca = instance->parent_card;

			if( instance->info_slot == CHOICE_PUMP ){
				if( instance->number_of_targets && valid_target(&td) ){
					pump_ability_until_eot(pl, ca, instance->targets[0].player, instance->targets[0].card,
											2, 2, 0, SP_KEYWORD_VIGILANCE | SP_KEYWORD_HASTE);
				}
			}

			if( instance->info_slot == CHOICE_WOLF ){
				make_wolf(pl, ca, player, 1, 1);
				transform(pl, ca);
			}
		}
	}

	return planeswalker(player, card, event, 3);
}

int card_arlinn_embraced_by_the_moon(int player, int card, event_t event){
	/* Arlinn, Embraced by the Moon		0x200eb7a
	 * Planeswalker - Arlinn
	 * +1: Creatures you control get +1/+1 and gain trample until end of turn.
	 * -1: ~ deals 3 damage to target creature or player. Transform ~.
	 * -6: You get an emblem with "Creatures you control have haste and '|T: This creature deals damage equal to its power to target creature or player.'" */

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

		card_instance_t *instance = get_card_instance( player, card);

		enum
		{
			CHOICE_PUMP_ALL = 1,
			CHOICE_DAMAGE,
			CHOICE_EMBLEM
		};

		if( event == EVENT_ACTIVATE ){
			instance->info_slot = instance->number_of_targets = 0;
			int priority[3] = {5, (! count_subtype(player, TYPE_CREATURE, -1) ? 10 : 1), 15};
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
								"Pump creatures",		1,					priority[0],	+1,
								"Damage & Transform",	can_target(&td),	priority[1],	-1,
								"Emblem",				1,					priority[2],	-6);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}

			if( choice == CHOICE_DAMAGE ){
				if( ! new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", 0, 1) ){
					return 0;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int pl = instance->parent_controller;
			int ca = instance->parent_card;

			if( instance->info_slot == CHOICE_PUMP_ALL ){
				pump_creatures_until_eot(pl, ca, player, 0, 1, 1, KEYWORD_TRAMPLE, 0, NULL);
			}

			if( instance->info_slot == CHOICE_DAMAGE ){
				if( valid_target(&td) ){
					damage_target0(player, card, 3);
					transform(pl, ca);
				}
			}

			if( instance->info_slot == CHOICE_EMBLEM ){
				generate_reserved_token_by_id(player, CARD_ID_ARLINN_EMBRACED_BY_THE_MOON_EMBLEM);
			}
		}
	}

	return planeswalker(player, card, event, 3);
}

static const char* can_activate(int who_chooses, int player, int card)
{
	if( can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST0) ){
		return NULL;
	}
	return "this creature can't use activated abilities.";
}

int card_arlinn_embraced_by_the_moon_emblem(int player, int card, event_t event){
	/* Arlinn Kord's Emblem	0x200eb7f
	 * Emblem - Arlinn
	 * Creatures you control have haste and "|T: This creature deals damage equal to its power to target creature or player." */

	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.extra = (int32_t)can_activate;
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE);
	td2.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_CREATURE) ){
				if( granted_generic_activated_ability(player, card, player, c, event, GAA_UNTAPPED | GAA_CAN_TARGET, 0, MANACOST0,
														&td2, NULL) )
				{
					return 1;
				}
			}
		}
	}


	if( event == EVENT_ACTIVATE  ){
		instance->number_of_targets = 0;
		if( new_pick_target(&td, "Select an untapped creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
			granted_generic_activated_ability(player, card, instance->targets[0].player, instance->targets[0].card, event,
												GAA_UNTAPPED | GAA_CAN_TARGET, 0, MANACOST0, &td2, "TARGET_CREATURE_OR_PLAYER");
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *crit = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		if( validate_target(crit->targets[0].player, crit->targets[0].card, &td2, 0) ){
			damage_target0(crit->targets[0].player, crit->targets[0].card,
							get_power(crit->targets[0].player, crit->targets[0].card));
		}
	}

	return 0;
}

int card_fevered_visions(int player, int card, event_t event){
	/* Fevered Visions	|1|U|R	0x200eb84
	 * Enchantment
	 * At the beginning of each player's end step, that player draws a card. If the player is your opponent and
	 * has four or more cards in hand, ~ deals 2 damage to him or her. */

	if( eot_trigger(player, card, event) ){
		draw_cards(current_turn, 1);
		if( current_turn != player && hand_count[current_turn] >= 4 ){
			damage_player(current_turn, 2, player, card);
		}
	}

	return global_enchantment(player, card, event);
}

int card_the_gitrog_monster(int player, int card, event_t event){
	/* The Gitrog Monster	|3|B|G	0x200eb89
	 * Legendary Creature - Frog Horror 6/6
	 * Deathtouch
	 * At the beginning of your upkeep, sacrifice ~ unless you sacrifice a land.
	 * You may play an additional land on each of your turns.
	 * Whenever one or more land cards are put into your graveyard from anywhere, draw a card. */

	check_legend_rule(player, card, event);

	deathtouch(player, card, event);

	check_playable_lands(player);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "Select a land to sacrifice.");

		int sac = 1;
		if( new_sacrifice(player, card, player, 0, &this_test) ){
			sac = 0;
		}
		if( sac ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		int result = whenever_type_is_put_into_graveyard_from_anywhere(player, card, event, player, TYPE_LAND, NULL);
		if( result ){
			draw_cards(player, 1);
		}
	}

	return 0;
}

int card_invocation_of_saint_traft(int player, int card, event_t event){
	/* Invocation of Saint Traft	|1|W|U	0x200eb8e
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature has "Whenever this creature attacks, put a 4/4 |Swhite Angel creature token with flying onto
	 * the battlefield tapped and attacking. Exile that token at end of combat." */

	if( in_play(player, card) && ! is_humiliated(player, card) && get_card_instance(player, card)->damage_target_player > -1 ){
		card_instance_t *instance = get_card_instance(player, card);
		saint_traft_ability(instance->damage_target_player, instance->damage_target_card, event,
							instance->damage_target_player, instance->damage_target_card);
	}

	if( ! IS_AURA_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	return targeted_aura(player, card, event, &td, "TARGET_CREATURE");
}

static const char* is_enchantment_or_tapped_artifact_creature(int who_chooses, int player, int card)
{
	if( is_what(player, card, TYPE_ENCHANTMENT) || (is_tapped(player, card) && is_what(player, card, TYPE_CREATURE | TYPE_ARTIFACT))  ){
		return NULL;
	}
	return "must be an enchantment, tapped artifact, or tapped creature.";
}

static int haste_and_bounce_eot(int player, int card, event_t event){

	if( get_card_instance(player, card)->damage_target_player > -1 ){
		card_instance_t *instance = get_card_instance(player, card);
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_ABILITIES && affect_me(p, c) ){
			haste(p, c, event);
		}

		if( eot_trigger(player, card, event) ){
			bounce_permanent(p, c);
		}
	}

	return 0;
}

int card_nahiri_the_harbinger(int player, int card, event_t event){
	/* Nahiri, the Harbinger	|2|R|W	0x200eb93
	 * Planeswalker - Nahiri (4)
	 * +2: You may discard a card. If you do, draw a card.
	 * -2: Exile target enchantment, tapped artifact, or tapped creature.
	 * -8: Search your library for an artifact or creature card, put it onto the battlefield, then shuffle your library.
	 * It gains haste. Return it to your hand at the beginning of the next end step. */

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.extra = (int32_t)is_enchantment_or_tapped_artifact_creature;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

		card_instance_t *instance = get_card_instance( player, card);

		enum
		{
			CHOICE_DISCARD_DRAW = 1,
			CHOICE_EXILE,
			CHOICE_TUTOR
		};

		if( event == EVENT_ACTIVATE ){
			instance->info_slot = instance->number_of_targets = 0;

			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
								"Discard & draw",		1,					5,	+2,
								"Exile permanent",		can_target(&td),	10,	-2,
								"Tutor",				1,					15, -8);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}

			if( choice == CHOICE_EXILE ){
				if( ! new_pick_target(&td, "Select target enchantment, tapped artifact, or tapped creature.", 0, 1 | GS_LITERAL_PROMPT) ){
					return 0;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int pl = instance->parent_controller;
			int ca = instance->parent_card;

			if( instance->info_slot == CHOICE_DISCARD_DRAW ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to discard.");
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
				if( selected != -1 ){
					new_discard_card(player, selected, player, 0);
					draw_cards(player, 1);
				}
			}

			if( instance->info_slot == CHOICE_EXILE ){
				if( valid_target(&td) ){
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
				}
			}

			if( instance->info_slot == CHOICE_TUTOR ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_ARTIFACT, "Select an artifact or creature card.");
				int selected = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
				if( selected != -1 ){
					create_targetted_legacy_effect(pl, ca, &haste_and_bounce_eot, player, selected);
				}
			}
		}
	}

	return planeswalker(player, card, event, 4);
}

static int olivia_mobilized_for_war_legacy(int player, int card, event_t event){

	if( get_card_instance(player, card)->damage_target_player > -1 ){
		card_instance_t *instance = get_card_instance(player, card);
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_ABILITIES && affect_me(p, c) && instance->targets[1].player != 66 ){
			haste(p, c, event);
		}

		if( event == EVENT_CLEANUP ){
			instance->targets[1].player = 66;
		}
	}

	return 0;
}

int card_olivia_mobilized_for_war(int player, int card, event_t event){
	/* Olivia, Mobilized for War	|1|B|R	0x200eb98
	 * Legendary Creature - Vampire Knight 3/3
	 * Flying
	 * Whenever another creature enters the battlefield under your control, you may discard a card.
	 * If you do, put a +1/+1 counter on that creature, it gains haste until end of turn, and it becomes a Vampire in addition
	 * to its other types. */

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.not_me = 1;

		if( hand_count[player] && new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &test) ){
			test_definition_t test2;
			new_default_test_definition(&test2, TYPE_ANY, "Select a card to discard.");
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &test2);
			if( selected != -1 ){
				new_discard_card(player, selected, player, 0);
				add_1_1_counter(trigger_cause_controller, trigger_cause);
				add_a_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_VAMPIRE);
				create_targetted_legacy_effect(player, card, &olivia_mobilized_for_war_legacy, trigger_cause_controller, trigger_cause);
			}
		}
	}

	return 0;
}

/* Prized Amalgam	|1|U|B --> vanilla	0x401000 (the effect is in "static_graveyard_abilities")
 * Creature - Zombie 3/3
 * Whenever a creature enters the battlefield, if it entered from your graveyard or you cast it from your graveyard,
 * return ~ from your graveyard to the battlefield tapped at the beginning of the next end step. */

int card_sigarda_herons_grace(int player, int card, event_t event){
	/* Sigarda, Heron's Grace	|3|G|W	0x200eb9d
	 * Legendary Creature - Angel 4/5
	 * Flying
	 * You and Humans you control have hexproof.
	 * |2, Exile a card from your graveyard: Put a 1/1 |Swhite Human Soldier creature token onto the battlefield. */

	check_legend_rule(player, card, event);

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( event == EVENT_ABILITIES && affected_card_controller == player &&
			has_subtype(affected_card_controller, affected_card, SUBTYPE_HUMAN) )
		{
			hexproof(affected_card_controller, affected_card, event);
		}

		player_bits[player] |= PB_PLAYER_HAS_HEXPROOF;
	}

	if( IS_GAA_EVENT(event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a card to exile.");

		if( event == EVENT_CAN_ACTIVATE ){
			if( generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL) ){
				return new_special_count_grave(player, &this_test);
			}
		}

		if( event == EVENT_ACTIVATE ){
			if( generic_activated_ability(player, card, event, 0, MANACOST_X(2), 0, NULL, NULL) ){
				if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MIN_VALUE, &this_test) == -1 ){
					spell_fizzled = 1;
				}
			}
		}

		if( event == EVENT_CAN_ACTIVATE ){
			card_instance_t *instance = get_card_instance( player, card );
			make_human_soldier(instance->parent_controller, instance->parent_card, player, 1, 1);
		}
	}

	return 0;
}

int card_sorin_grim_nemesis(int player, int card, event_t event){
	/* Sorin, Grim Nemesis	|4|W|B	0x200eba2
	 * Planeswalker - Sorin (6)
	 * +1: Reveal the top card of your library and put that card into your hand. Each opponent loses life equal to its converted mana cost.
	 * -X: ~ deals X damage to target creature or planeswalker and you gain X life.
	 * -9: Put a number of 1/1 |Sblack Vampire Knight creature tokens with lifelink onto the battlefield equal to the highest life total among all players. */

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.extra = (int32_t)is_creature_or_pwalker;
		td.special = TARGET_SPECIAL_EXTRA_FUNCTION;

		card_instance_t *instance = get_card_instance( player, card);

		enum
		{
			CHOICE_REVEAL = 1,
			CHOICE_DAMAGE,
			CHOICE_TOKENS
		};

		if( event == EVENT_ACTIVATE ){
			instance->info_slot = instance->number_of_targets = instance->targets[1].player = 0;
			int priority_damage = life[player] < 6 ? 10 : 1;
			int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_PLANESWALKER,
								"Reveal & damage",					1,					5,					+1,
								"Damage creature or pwalker",		can_target(&td),	priority_damage,	0,
								"Make Vampire Knights",				1,					15,					-9);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}

			if( choice == CHOICE_DAMAGE ){
				if( new_pick_target(&td, "Select target creature or planeswalker.", 0, 1 | GS_LITERAL_PROMPT) ){
					int ctr =	is_what(instance->targets[0].player, instance->targets[0].card, TYPE_CREATURE) ?
								get_toughness(instance->targets[0].player, instance->targets[0].card) :
								count_counters(instance->targets[0].player, instance->targets[0].card, COUNTER_LOYALTY);
					if( player == HUMAN ){
						ctr = choose_a_number(player, "How many counters you'll remove ?", ctr);
					}
					if( ctr < 0 || ctr > count_counters(player, card, COUNTER_LOYALTY) ){
						spell_fizzled = 1;
						return 0;
					}
					remove_counters(player, card, COUNTER_LOYALTY, ctr);
					instance->targets[1].player = ctr;
				}
				else{
					return 0;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int pl = instance->parent_controller;
			int ca = instance->parent_card;

			if( instance->info_slot == CHOICE_REVEAL ){
				if( deck_ptr[player][0] != -1 ){
					reveal_card_iid(pl, ca, deck_ptr[player][0]);
					int cmc = get_cmc_by_internal_id(deck_ptr[player][0]);
					add_card_to_hand(player, deck_ptr[player][0]);
					remove_card_from_deck(player, 0);
					lose_life(1-player, cmc);
				}
			}

			if( instance->info_slot == CHOICE_DAMAGE ){
				if( valid_target(&td) ){
					damage_creature(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, pl, ca);
					gain_life(player, instance->targets[1].player);
				}
			}

			if( instance->info_slot == CHOICE_TOKENS ){
				int amount = life[player] > life[1-player] ? life[player] : life[1-player];

				token_generation_t token;
				default_token_definition(player, card, CARD_ID_VAMPIRE_KNIGHT, &token);
				token.qty = amount;
				token.pow = token.tou = 1;
				token.s_key_plus = SP_KEYWORD_LIFELINK;
				token.color_forced = get_sleighted_color_test(pl, ca, COLOR_TEST_BLACK);
				generate_token(&token);
			}
		}
	}

	return planeswalker(player, card, event, 6);
}

/*** Artifact ***/

int card_brain_in_a_jar(int player, int card, event_t event){
	/* Brain in a Jar	|2	0x200eba7
	 * Artifact
	 * |1, |T: Put a charge counter on ~, then you may cast an instant or sorcery card with converted mana cost equal
	 * to the number of charge counters on ~ from your hand without paying its mana cost.
	 * |3, |T, Remove X charge counters from ~: Scry X. */

	if( IS_GAA_EVENT(event) ){
		card_instance_t *instance = get_card_instance( player, card);

		enum
		{
			CHOICE_COUNTER = 1,
			CHOICE_SCRY,
		};

		if( event == EVENT_ACTIVATE ){
			instance->info_slot = instance->targets[1].player = 0;
			int can_scry = has_mana_for_activated_ability(player, card, MANACOST_X(3)) && count_counters(player, card, COUNTER_CHARGE) ? 1 : 0;
			int priority_counter = 1;
			int priority_scry = 1;

			int cmc[16];
			int c;
			for(c=0; c<16; c++){
				cmc[c] = 0;
			}
			for(c=0; c<active_cards_count[player]; c++){
				if( in_hand(player, c) && is_what(player, c, TYPE_SPELL) ){
					cmc[get_cmc(player, c)]++;
				}
			}
			int counters_now = count_counters(player, card, COUNTER_CHARGE);
			for(c=1; c<counters_now; c++){
				priority_scry = cmc[MAX(0, counters_now-c)]*(counters_now-c);
			}
			for(c=counters_now+1; c<16; c++){
				priority_counter = cmc[c]*(c-counters_now);
			}

			int counters_to_remove = 0;
			int par = 0;
			for(c=counters_now; c>0; c--){
				if( cmc[c] > par ){
					counters_to_remove = c;
					par = cmc[c];
				}
			}

			int choice = DIALOG(player, card, event, DLG_RANDOM,
								"Add counter & play spell",	1, priority_counter,
								"Remove counters & scry", can_scry,	priority_scry);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}

			if( charge_mana_for_activated_ability(player, card, MANACOST_X((choice == CHOICE_COUNTER ? 1 : 3))) ){
				if( choice == CHOICE_SCRY ){
					if( player == HUMAN ){
						counters_to_remove = choose_a_number(player, "How many counters you'll remove?", counters_to_remove);
					}
					if( counters_to_remove > 0 && counters_to_remove <= counters_now ){
						remove_counters(player, card, COUNTER_CHARGE, counters_to_remove);
						instance->targets[11].player = counters_to_remove;
					}
					else{
						spell_fizzled = 1;
						return 0;
					}
				}
				tap_card(player, card);
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			int pl = instance->parent_controller;
			int ca = instance->parent_card;

			if( instance->info_slot == CHOICE_COUNTER ){
				add_counter(pl, ca, COUNTER_CHARGE);

				char buffer[500];
				scnprintf(buffer, 500, "Select a instant and sorcery card with CMC %d.", count_counters(pl, ca, COUNTER_CHARGE));
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_SPELL, buffer);
				this_test.cmc = count_counters(pl, ca, COUNTER_CHARGE);
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MAX_VALUE, -1, &this_test);
				if( selected != -1 ){
					play_card_in_hand_for_free(player, selected);
				}
			}

			if( instance->info_slot == CHOICE_SCRY ){
				scry(player, instance->targets[1].player);
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(1), 0, NULL, NULL);
}

int card_corrupted_grafstone(int player, int card, event_t event){
	/* Corrupted Grafstone	|2	0x200ebac
	 * Artifact
	 * ~ enters the battlefield tapped.
	 * |T: Choose a color of a card in your graveyard. Add one mana of that color to your mana pool. */

	comes_into_play_tapped(player, card, event);

	if (event == EVENT_CHANGE_TYPE && affect_me(player, card)){
		card_instance_t* instance = get_card_instance(player, card);
		int base_color = 0;
		int hack = get_global_color_hack(player);
		int count = 0;
		while( get_grave(player)[count] != -1 ){
				base_color |= (get_color_by_internal_id_no_hack(get_grave(player)[count]) | hack);
				count++;
		}
		instance->mana_color = instance->card_color = base_color;
	}

	if( IS_GAA_EVENT(event) || (event == EVENT_COUNT_MANA && affect_me(player, card)) ){
		card_instance_t* instance = get_card_instance(player, card);
		if(instance->mana_color == 0 || instance->mana_color == COLOR_TEST_COLORLESS ){	// Can't produce mana, but can still tap for 0
			if (event == EVENT_CAN_ACTIVATE && player == AI){
				return 0;
			}
			if (event == EVENT_ACTIVATE){
				if (can_produce_mana(player, card)){
					produce_mana_tapped(player, card, COLOR_COLORLESS, 0);
					return 0;
				}
			}
			if (event == EVENT_COUNT_MANA){
				return 0;
			}
		}
	}

	return mana_producer(player, card, event);
}

int card_epitaph_golem(int player, int card, event_t event){
	/* Epitaph Golem	|5	0x200ebb1
	 * Artifact Creature - Golem 3/5
	 * |2: Put target card from your graveyard on the bottom of your library. */

	if( IS_GAA_EVENT(event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put on bottom of deck.");

		if( event == EVENT_CAN_ACTIVATE ){
			if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL) ){
				return new_special_count_grave(player, &this_test);
			}
		}

		if( event == EVENT_ACTIVATE ){
			if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(2), 0, NULL, NULL) ){
				if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &this_test, 0) != -1 ){
					tap_card(player, card);
				}
				else{
					spell_fizzled = 1;
				}
			}
		}

		if( event == EVENT_CAN_ACTIVATE ){
			int selected = validate_target_from_grave_source(player, card, player, 0);
			if( selected != -1 ){
				deck_ptr[player][count_deck(player)] = get_grave(player)[selected];
				remove_card_from_grave(player, selected);
			}
		}
	}

	return 0;
}

int card_explosive_apparatus(int player, int card, event_t event){
	/* Explosive Apparatus	|1	0x200ebb6
	 * Artifact
	 * |3, |T, Sacrifice ~: ~ deals 2 damage to target creature or player. */

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_target0(player, card, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET | GAA_SACRIFICE_ME, MANACOST_X(3), 0, &td, "TARGET_CREATURE_OR_PLAYER");
}


int card_harvest_hand(int player, int card, event_t event){
	/* Harvest Hand	|3	0x200ebbb
	 * Artifact Creature - Scarecrow 2/2
	 * When ~ dies, return it to the battlefield transformed under your control.*/

	double_faced_card(player, card, event);

	 nice_creature_to_sacrifice(player, card);

	int owner, position;
	if (this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) && find_in_owners_graveyard(player, card, &owner, &position)){
		reanimate_permanent(owner, -1, owner, position, REANIMATE_RETURN_TO_PLAY_TRANSFORMED);	// -1 for card doesn't abort since no legacy is added
	}
	return 0;
}

int card_scrounged_scythe(int player, int card, event_t event){
	/* Scrounged Scythe	0x200ebc0
	 * Artifact - Equipment
	 * Equipped creature gets +1/+1.
	 * As long as equipped creature is a Human, it has menace.
	 * Equip |2 */

	if( is_equipping(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			int p = instance->damage_target_player;
			int c = instance->damage_target_card;
			if( has_subtype(p, c, SUBTYPE_HUMAN) ){
				menace(p, c, event);
			}
		}
	}
	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = td.preferred_controller = player;
		if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			if( is_equipping(instance->parent_controller, instance->parent_card) ){
				card_instance_t *equip = get_card_instance(instance->parent_controller, instance->parent_card);
				remove_special_ability(equip->damage_target_player, equip->damage_target_card, SP_KEYWORD_MENACE);
			}
		}
	}
	return vanilla_equipment(player, card, event, 2, 1, 1, 0, 0);
}

int card_haunted_cloak(int player, int card, event_t event){
	/* Haunted Cloak	|3	0x200ebc5
	 * Artifact - Equipment
	 * Equipped creature has vigilance, trample, and haste.
	 * Equip |1 */
	return vanilla_equipment(player, card, event, 1, 0, 0, KEYWORD_TRAMPLE, SP_KEYWORD_VIGILANCE | SP_KEYWORD_HASTE);
}

int card_magnifying_glass(int player, int card, event_t event){
	/* Magnifying Glass	|3	0x200ebca
	 * Artifact
	 * |T: Add |C to your mana pool.
	 * |4, |T: Investigate. */

	if( ! IS_GAA_EVENT(event) ){
		return mana_producer(player, card, event);
	}

	enum
	{
		CHOICE_MANA = 1,
		CHOICE_INVESTIGATE,
	};

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(5), 0, NULL, NULL) ){
			return 1;
		}
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->info_slot = CHOICE_MANA;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(5), 0, NULL, NULL) ){
			int choice = DIALOG(player, card, event, DLG_RANDOM,
								"Produce mana",	1, 1,
								"Investigate", 1, 10);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}

			if( choice == CHOICE_MANA ){
				return mana_producer(player, card, event);
			}
			if( choice == CHOICE_INVESTIGATE ){
				add_state(player, card, STATE_TAPPED);
				if( ! charge_mana_for_activated_ability(player, card,  MANACOST_X(4)) ){
					remove_state(player, card, STATE_TAPPED);
				}
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->info_slot == CHOICE_INVESTIGATE ){
			investigate(instance->parent_controller, instance->parent_card, player);
		}
	}

	return 0;
}

int card_murderers_axe(int player, int card, event_t event){
	/* Murderer's Axe	|4	0x200ec29
	 * Artifact - Equipment
	 * Equipped creature gets +2/+2.
	 * Equip-Discard a card. */

	if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
		return vanilla_equipment(player, card, event, 0, 2, 2, 0, 0);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( hand_count[player] ){
			return can_activate_basic_equipment(player, card, event, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		activate_basic_equipment(player, card, 0);
		if( spell_fizzled != 1 ){
			discard(player, 0, player);
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		resolve_activation_basic_equipment(player, card);
	}

	return 0;
}

int card_neglected_heirloom(int player, int card, event_t event){
	/* Neglected Heirloom	|1	0x200ebcf
	 * Artifact - Equipment
	 * Equipped creature gets +1/+1.
	 * When equipped creature transforms, transform ~.
	 * Equip |1 */

	double_faced_card(player, card, event);

	if( is_equipping(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( event == EVENT_ANOTHER_PERMANENT_HAS_TRANSFORMED &&
				affect_me(instance->damage_target_player, instance->damage_target_card) )
			{
				transform(player, card);
			}
		}
	}

	return vanilla_equipment(player, card, event, 1, 1, 1, 0, 0);
}

int card_ashmouth_blade(int player, int card, event_t event){
	/* Ashmouth Blade	0x200ebd4
	 * Artifact - Equipment
	 * Equipped creature gets +3/+3 and has first strike.
	 * Equip |3 */
	return vanilla_equipment(player, card, event, 3, 3, 3, KEYWORD_FIRST_STRIKE, 0);
}

/* Runaway Carriage	|4 --> Fog Elemental	0x20032e3
 * Artifact Creature - Construct 5/6
 * Trample
 * When ~ attacks or blocks, sacrifice it at end of combat. */

int card_shard_of_broken_glass(int player, int card, event_t event){
	/* Shard of Broken Glass	|1	0x200ebd9
	 * Artifact - Equipment
	 * Equipped creature gets +1/+0.
	 * Whenever equipped creature attacks, you may put the top two cards of your library into your graveyard.
	 * Equip |1 */

	if( is_equipping(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player),
											instance->damage_target_player, instance->damage_target_card) )
			{
				mill(player, 2);
			}
		}
	}

	return vanilla_equipment(player, card, event, 1, 1, 0, 0, 0);
}

int card_skeleton_key(int player, int card, event_t event){
	/* Skeleton Key	|1	0x200ebde
	 * Artifact - Equipment
	 * Equipped creature has skulk.
	 * Whenever equipped creature deals combat damage to a player, you may draw a card. If you do, discard a card.
	 * Equip |2 */

	if( is_equipping(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			int p = instance->damage_target_player;
			int c = instance->damage_target_card;
			if (damage_dealt_by_me_arbitrary(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_TRACE_DAMAGED_PLAYERS |
											RESOLVE_TRIGGER_AI(player), p, c) )
			{
				int times_damaged = BYTE0(instance->targets[1].player) + BYTE1(instance->targets[1].player);
				int i;
				for(i=0; i<times_damaged; i++){
					draw_cards(player, 1);
					discard(player, 0, player);
				}
			}
		}
	}

	if( IS_GAA_EVENT(event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = td.preferred_controller = player;
		if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			if( is_equipping(instance->parent_controller, instance->parent_card) ){
				card_instance_t *equip = get_card_instance(instance->parent_controller, instance->parent_card);
				remove_special_ability(equip->damage_target_player, equip->damage_target_card, SP_KEYWORD_SKULK);
			}
		}
	}

	return vanilla_equipment(player, card, event, 2, 0, 0, 0, SP_KEYWORD_SKULK);
}

int card_slayers_plate(int player, int card, event_t event){
	/* Slayer's Plate	|3	0x200ebe3
	 * Artifact - Equipment
	 * Equipped creature gets +4/+2.
	 * Whenever equipped creature dies, if it was a Human, put a 1/1 |Swhite Spirit creature token with flying onto the battlefield.
	 * Equip |3 */

	if( is_equipping(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			int p = instance->damage_target_player;
			int c = instance->damage_target_card;
			if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(p, c) && has_subtype(p, c, SUBTYPE_HUMAN) ){
				count_for_gfp_ability(player, card, event, player, TYPE_PERMANENT, NULL);
			}
		}
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		get_card_instance(player, card)->targets[11].card = 0;
		make_spirit(player, card, player, 1, 1);
	}

	return vanilla_equipment(player, card, event, 3, 4, 2, 0, 0);
}

int card_tamiyos_journal(int player, int card, event_t event){
	/* Tamiyo's Journal	|5	0x200ebe8
	 * Legendary Artifact
	 * At the beginning of your upkeep, investigate.
	 * |T, Sacrifice three Clues: Search your library for a card and put that card into your hand. Then shuffle your library. */

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		investigate(player, card, player);
	}

	if (!IS_GAA_EVENT(event)  ){
		return 0;
	}

	test_definition_t test;
	new_default_test_definition(&test, TYPE_PERMANENT, "Select a Clue to sacrifice.");
	test.id = CARD_ID_CLUE;
	test.qty = 3;

	if( event == EVENT_CAN_ACTIVATE ){
		if( new_can_sacrifice_as_cost(player, card, &test) ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST0, 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			int sacced[3] = {-1, -1, -1};
			int sc = 0;
			int i;
			for(i=0; i<3; i++){
				sacced[i] = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
				if( sacced[i] == -1 ){
					break;
				}
				else{
					sc++;
				}
			}
			for(i=0; i<3; i++){
				if( sacced[i] != -1 ){
					if( sc == 3 ){
						kill_card(BYTE2(sacced[i]), BYTE3(sacced[i]), KILL_SACRIFICE);
					}
					else{
						state_untargettable(BYTE2(sacced[i]), BYTE3(sacced[i]), 0);
					}
				}
			}
			if( sc == 3 ){
				tap_card(player, card);
			}
			if( sc != 3 ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		test_definition_t test2;
		new_default_test_definition(&test2, TYPE_ANY, "Select a card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &test2);
	}

	return 0;
}

int card_thraben_gargoyle(int player, int card, event_t event){
	/* Thraben Gargoyle	|1	0x200ebed
	 * Artifact Creature - Gargoyle 2/2
	 * Defender
	 * |6: Transform ~.*/

	double_faced_card(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *instance = get_card_instance( player, card );
		if( ! check_special_flags3(instance->parent_controller, instance->parent_card, SF3_CARD_IS_FLIPPED) ){
			transform(instance->parent_controller, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_X(6), 0, NULL, NULL);
}

 /* Stonewing Antagonizer --> vanilla	0x401000
 * Artifact Creature - Gargoyle Horror 4/2
 * Flying */

int card_true_faith_censer(int player, int card, event_t event){
	/* True-Faith Censer	|2	0x200ebf2
	 * Artifact - Equipment
	 * Equipped creature gets +1/+1 and has vigilance.
	 * As long as equipped creature is a Human, it gets an additional +1/+0.
	 * Equip |2 */

	if( is_equipping(player, card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			if( event == EVENT_POWER && affect_me(instance->damage_target_player, instance->damage_target_card) &&
				has_subtype(instance->damage_target_player, instance->damage_target_card, SUBTYPE_HUMAN) )
			{
				event_result++;;
			}
		}
	}

	return vanilla_equipment(player, card, event, 2, 1, 1, 0, SP_KEYWORD_VIGILANCE);
}

/* Wicker Witch	|3 --> vanilla	0x401000
 * Artifact Creature - Scarecrow 3/1 */

/* Wild-Field Scarecrow	|3 --> Armillary Sphere	0x20082a7
 * Artifact Creature - Scarecrow 1/4
 * Defender
 * |2, Sacrifice ~: Search your library for up to two basic land cards, reveal them,
 * and put them into your hand. Then shuffle your library. */

/*** Land ***/

int card_choked_estuary(int player, int card, event_t event){
	/* Choked Estuary	0x200ebf7
	 * Land
	 * As ~ enters the battlefield, you may reveal |Han Island or |H2Swamp card from your hand.
	 * If you don't, ~ enters the battlefield tapped.
	 * |T: Add |U or |B to your mana pool. */
	soi_need_subtype_land(player, card, event, SUBTYPE_ISLAND, SUBTYPE_SWAMP);
	return 0;
}

int card_drownyard_temple(int player, int card, event_t event){
	/* Drownyard Temple	0x200ebfc
	 * Land
	 * |T: Add |C to your mana pool.
	 * |3: Return ~ from your graveyard to the battlefield tapped. */

	if( event == EVENT_GRAVEYARD_ABILITY ){
		if( has_mana(player, COLOR_ANY, 3) ){
			return GA_RETURN_TO_PLAY_MODIFIED;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana(player, COLOR_ANY, 3);
		if( spell_fizzled != 1 ){
			return GAPAID_REMOVE;
		}
	}

	if( event == EVENT_RETURN_TO_PLAY_FROM_GRAVE_MODIFIED ){
		add_state(player, card, STATE_TAPPED);
	}

	return mana_producer(player, card, event);
}

int card_foreboding_ruins(int player, int card, event_t event){
	/* Foreboding Ruins	0x200ec01
	 * Land
	 * As ~ enters the battlefield, you may reveal |Ha Swamp or |H2Mountain card from your hand.
	 * If you don't, ~ enters the battlefield tapped.
	 * |T: Add |B or |R to your mana pool. */
	soi_need_subtype_land(player, card, event, SUBTYPE_SWAMP, SUBTYPE_MOUNTAIN);
	return 0;
}

/* Forsaken Sanctuary	""	// == mana_producer_tapped	0x2002d57
 * Land
 * ~ enters the battlefield tapped.
 * |T: Add |W or |B to your mana pool. */

int card_fortified_village(int player, int card, event_t event){
	/* Fortified Village	0x200ec06
	 * Land
	 * As ~ enters the battlefield, you may reveal |Ha Forest or |H2Plains card from your hand.
	 * If you don't, ~ enters the battlefield tapped.
	 * |T: Add |G or |W to your mana pool. */
	soi_need_subtype_land(player, card, event, SUBTYPE_FOREST, SUBTYPE_PLAINS);
	return 0;
}

/* Foul Orchard	""	// == mana_producer_tapped 0x2002d57
 * Land
 * ~ enters the battlefield tapped.
 * |T: Add |B or |G to your mana pool. */

int card_game_trail(int player, int card, event_t event){
	/* Game Trail	0x200ec0b
	 * Land
	 * As ~ enters the battlefield, you may reveal |Ha Mountain or |H2Forest card from your hand.
	 * If you don't, ~ enters the battlefield tapped.
	 * |T: Add |R or |G to your mana pool. */
	soi_need_subtype_land(player, card, event, SUBTYPE_FOREST, SUBTYPE_MOUNTAIN);
	return 0;
}

/* Highland Lake	""	// == mana_producer_tapped  0x2002d57
 * Land
 * ~ enters the battlefield tapped.
 * |T: Add |U or |R to your mana pool. */

int card_port_town(int player, int card, event_t event){
	/* Port Town	0x200ec10
	 * Land
	 * As ~ enters the battlefield, you may reveal |Ha Plains or |H2Island card from your hand.
	 * If you don't, ~ enters the battlefield tapped.
	 * |T: Add |W or |U to your mana pool. */
	soi_need_subtype_land(player, card, event, SUBTYPE_PLAINS, SUBTYPE_ISLAND);
	return 0;
}


/* Stone Quarry	""	// == mana_producer_tapped 0x2002d57
 * Land
 * ~ enters the battlefield tapped.
 * |T: Add |R or |W to your mana pool. */

/* Warped Landscape --> Terminal Moraine	0x2008ae5
 * Land
 * |T: Add |C to your mana pool.
 * |2, |T, Sacrifice ~: Search your library for a basic land card and put it onto the battlefield tapped. Then shuffle your library. */

int card_westvale_abbey(int player, int card, event_t event){
	/* Westvale Abbey	0x200ec15
	 * Land
	 * |T: Add |C to your mana pool.
	 * |5, |T, Pay 1 life: Put a 1/1 |Swhite and |Sblack Human Cleric creature token onto the battlefield.
	 * |5, |T, Sacrifice five creatures: Transform ~, then untap it.*/

	double_faced_card(player, card, event);

	if( ! IS_GAA_EVENT(event) ){
		return mana_producer(player, card, event);
	}

	enum
	{
		CHOICE_MANA = 1,
		CHOICE_CLERIC,
		CHOICE_TRANSFORM
	};

	test_definition_t test;
	new_default_test_definition(&test, TYPE_CREATURE, "Select a creture to sacrifice.");

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_X(6), 0, NULL, NULL) ){
			if( can_pay_life(player, 1) ){
				return 1;
			}
			test.qty = 5;
			if( new_can_sacrifice_as_cost(player, card, &test) ){
				return 1;
			}
		}
		return mana_producer(player, card, event);
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->info_slot = CHOICE_MANA;
		if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_X(6), 0, NULL, NULL) ){
			test.qty = 5;
			int choice = DIALOG(player, card, event, DLG_RANDOM,
								"Produce mana",	1, 1,
								"Generate an Human Cleric", can_pay_life(player, 1), 5,
								"Transform", new_can_sacrifice_as_cost(player, card, &test), 15);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}

			if( choice == CHOICE_MANA ){
				return mana_producer(player, card, event);
			}
			if( choice == CHOICE_CLERIC || choice == CHOICE_TRANSFORM ){
				add_state(player, card, STATE_TAPPED);
				if( ! charge_mana_for_activated_ability(player, card,  MANACOST_X(5)) ){
					remove_state(player, card, STATE_TAPPED);
					return 0;
				}
				if( choice == CHOICE_CLERIC ){
					lose_life(player, 1);
				}
				if( choice == CHOICE_TRANSFORM ){
					test.qty = 1;
					int sacced[5] = {-1, -1, -1, -1, -1};
					int sc = 0;
					int i;
					for(i=0; i<5; i++){
						sacced[i] = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
						if( sacced[i] == -1 ){
							break;
						}
						else{
							sc++;
						}
					}
					for(i=0; i<5; i++){
						if( sacced[i] != -1 ){
							if( sc == 5 ){
								kill_card(BYTE2(sacced[i]), BYTE3(sacced[i]), KILL_SACRIFICE);
							}
							else{
								state_untargettable(BYTE2(sacced[i]), BYTE3(sacced[i]), 0);
							}
						}
					}
					if( sc != 5 ){
						spell_fizzled = 1;
						remove_state(player, card, STATE_TAPPED);
					}
				}
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->info_slot == CHOICE_CLERIC ){
			token_generation_t token;
			default_token_definition(instance->parent_controller, instance->parent_card, CARD_ID_HUMAN_CLERIC, &token);
			token.pow = token.tou = 1;
			token.color_forced = get_sleighted_color_test(player, card, COLOR_TEST_BLACK) |
								get_sleighted_color_test(player, card, COLOR_TEST_WHITE);
			generate_token(&token);
		}
		if( instance->info_slot == CHOICE_TRANSFORM ){
			transform(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_ormendahl_profane_prince(int player, int card, event_t event){
 /* Ormendahl, Profane Prince	0x200ec1a
 * Legendary Creature - Demon 9/7
 * Flying, lifelink, indestructible, haste */
	check_legend_rule(player, card, event);
	special_abilities(player, card, event, SP_KEYWORD_LIFELINK | SP_KEYWORD_INDESTRUCTIBLE | SP_KEYWORD_HASTE, player, card);
	return 0;
}

/* Woodland Stream	""	// == mana_producer_tapped 0x2002d57
 * Land
 * ~ enters the battlefield tapped.
 * |T: Add |G or |U to your mana pool. */

/* Plains	""	=>magic.exe:card_plains()
 * Basic Land - |H2Plains */

/* Island	""	=>magic.exe:card_island()
 * Basic Land - |H2Island */

/* Swamp	""	=>magic.exe:card_swamp()
 * Basic Land - |H2Swamp */

/* Mountain	""	=>magic.exe:card_mountain()
 * Basic Land - |H2Mountain */

/* Forest	""	=>magic.exe:card_forest()
 * Basic Land - |H2Forest */

/*** Tokens ***/

int card_clue(int player, int card, event_t event){
	// Clue |2	0x200e84b	Could be used for Slinking Skirge and Brass Secretary
	// Artifact (Token)
	// |2, Sacrifice this: Draw a card.

	if (event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, MANACOST_X(2), 0, NULL, NULL);
}

int card_devil_token(int player, int card, event_t event){
	/* Devil	0x200ec1f
	 * Creature - Devil 1/1
	 * When this creature dies, it deals 1 damage to target creature or player. */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_target_creature_or_player(instance->targets[0].player, instance->targets[0].card, 1, player, card);
		}
	}

	return generic_token(player, card, event);
}

/* Human Cleric	""(WB) --> Beast token	0x2001d0d
 * Creature - Human Cleric 1/1 */

/* Human Soldier	""(W) --> Beast token	0x2001d0d
 * Creature - Human Soldier 1/1 */

/* Vampire Knight	""(B) --> Beast token	0x2001d0d
 * Creature - Vampire Knight 1/1
 * Lifelink */
