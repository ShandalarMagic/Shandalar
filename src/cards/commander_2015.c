#include "manalink.h"

/***** Functions *****/

/***** Cards *****/

// White
int card_bastion_protector(int player, int card, event_t event){
	/* Bastion Protector	|2|W	0x200e31e
	 * Creature - Human Soldier 3/3
	 * Commander creatures you control get +2/+2 and have indestructible. */

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( in_play(affected_card_controller, affected_card) && affected_card_controller == player &&
			check_special_flags2(affected_card_controller, affected_card, SF2_COMMANDER) &&
			is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		  ){
			if( event == EVENT_POWER || event == EVENT_TOUGHNESS ){
				event_result +=2;
			}
			if( event == EVENT_ABILITIES ){
				special_abilities(affected_card_controller, affected_card, event, SP_KEYWORD_INDESTRUCTIBLE, player, card);
			}
		}
	}
	return 0;
}

int card_dawnbreak_reclaimer(int player, int card, event_t event){
	/* Dawnbreak Reclaimer	|4|W|W	0x200e323
	 * Creature - Angel 5/5
	 * Flying
	 * At the beginning of your end step, choose a creature card in an opponent's graveyard, then that player chooses a creature card in
	 *	your graveyard. You may return those cards to the battlefield under their owners' control. */

	if( current_turn == player && eot_trigger(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

		int selected[2] = {-1, -1};

		APNAP(p,
				{
					if( count_graveyard_by_type(1-p, TYPE_CREATURE) ){
						selected[p] = new_select_a_card(p, 1-p, TUTOR_FROM_GRAVE, 1, AI_MIN_CMC, -1, &this_test);
						int revealed[1] = {selected[p]};
						if( p == player ){
							show_deck(player, revealed, 1, "You selected this card.", 0, 0x7375B0 );
						}
						else{
							show_deck(player, revealed, 1, "Opponent selected this card.", 0, 0x7375B0 );
						}
					}
				};
		);

		int will_reanimate = selected[1-player] != -1 ? 20 : 0;
		if( will_reanimate && selected[player] != -1 ){
			if( get_cmc_by_internal_id(get_grave(1-player)[selected[player]]) >
				get_cmc_by_internal_id(get_grave(player)[selected[1-player]])
			  ){
				will_reanimate = 0;
			}
		}

		int choice = DIALOG(player, card, event, DLG_NO_STORAGE,
							"Reanimate the selected creatures",	(selected[1-player] != -1 && selected[player] != -1 ? 1 : 0), will_reanimate,
							"Pass",	1, 10);

		if( choice == 1 ){
			APNAP(p,{
						if( selected[p] != -1 ){
							reanimate_permanent(1-p, -1, 1-p, selected[p], REANIMATE_DEFAULT);
						}
					};
			);
		}
	}
	return 0;
}

int card_grasp_of_fate(int player, int card, event_t event){
	/* Grasp of Fate	|1|W|W	0x200e328 -->
	 * Enchantment
	 * When ~ enters the battlefield, for each opponent, exile up to one target nonland permanent that player controls until
	 *	~ leaves the battlefield. */

	return_from_oblivion(player, card, event);

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_PERMANENT);
		td.illegal_type = TYPE_LAND;
		td.allowed_controller = 1-player;

		if( can_target(&td) && new_pick_target(&td, "Select target nonland permanent.", 0, GS_LITERAL_PROMPT) ){
			card_instance_t *instance = get_card_instance( player, card );
			obliviation(player, card, instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 0;
		}
	}

	return global_enchantment(player, card, event);
}

/* Herald of the Host	|3|W|W	0x200c122 --> Serra Angel, as Myriad is useless in Manalink.
 * Creature - Angel 4/4
 * Flying, vigilance
 * Myriad */

int card_kalemnes_captain(int player, int card, event_t event){
	/* Kalemne's Captain	|3|W|W	0x200e32d
	 * Creature - Giant Soldier 5/5
	 * Vigilance
	 * |5|W|W: Monstrosity 3.
	 * When ~ becomes monstrous, exile all artifacts and enchantments. */

	if (event == EVENT_BECAME_MONSTROUS){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

		APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_REMOVE);};);
	}

	return monstrosity(player, card, event, MANACOST_XW(5, 2), 3);
}

int card_oreskos_explorer(int player, int card, event_t event){
	/* Oreskos Explorer	|1|W	0x200e332
	 * Creature - Cat Scout 2/2
	 * When ~ enters the battlefield, search your library for up to X |H1Plains cards, where X is the number of
	 *	players who control more lands than you. Reveal those cards, put them into your hand, then shuffle your library. */

	if( comes_into_play_mode(player, card, event, RESOLVE_TRIGGER_AI(player)) ){
		if( count_subtype(1-player, TYPE_LAND, -1) > count_subtype(player, TYPE_LAND, -1) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a Plains card.");
			this_test.subtype = SUBTYPE_PLAINS;

			new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

int card_righteous_confluence(int player, int card, event_t event){
	/* Righteous Confluence	|3|W|W	0x200e337
	 * Sorcery
	 * Choose three. You may choose the same mode more than once.
	 * * Put a 2/2 |Swhite Knight creature token with vigilance onto the battlefield.
	 * * Exile target enchantment.
	 * * You gain 5 life. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum
	{
		CHOICE_KNIGHT = 1,
		CHOICE_EXILE,
		CHOICE_GAIN_LIFE,
	};

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT );

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = instance->number_of_targets = 0;
		int choices[3] = {0, 0, 0};
		int i;
		for(i=0; i<3; i++){
			int priority_life = 20-life[player];
			priority_life-= (choices[0] & CHOICE_GAIN_LIFE ? 5 : 0);
			priority_life-= (choices[1] & CHOICE_GAIN_LIFE ? 5 : 0);

			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"Generate a Knight", 1, 5,
								"Exile an enchantment", can_target(&td), 10,
								"Gain 5 life", 1, priority_life);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			if( choice == CHOICE_KNIGHT || choice == CHOICE_GAIN_LIFE){
				choices[i] = choice;
			}
			if( choice == CHOICE_EXILE){
				if( new_pick_target(&td, "TARGET_ENCHANTMENT", -1, 0) ){
					choices[i] = choice;
					if( player == AI ){
						int tn = instance->number_of_targets-1;
						state_untargettable(instance->targets[tn].player, instance->targets[tn].card, 1);
					}
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
		}
		SET_BYTE0(instance->info_slot) |= choices[0];
		SET_BYTE1(instance->info_slot) |= choices[1];
		SET_BYTE2(instance->info_slot) |= choices[2];
		for(i=0; i<instance->number_of_targets; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int target_num = 0;
		int i;
		for(i=0; i<3; i++){
			int choice = BYTE0(instance->info_slot);
			if( i == 1 ){
				choice = BYTE1(instance->info_slot);
			}
			if( i == 2 ){
				choice = BYTE2(instance->info_slot);
			}
			if( choice & CHOICE_KNIGHT ){
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_KNIGHT, &token);
				token.pow = token.tou = 2;
				token.s_key_plus = SP_KEYWORD_VIGILANCE;
				token.color_forced = COLOR_TEST_WHITE;
				generate_token(&token);
			}
			if( choice & CHOICE_EXILE ){
				if( validate_target(player, card, &td, target_num) ){
					kill_card(instance->targets[target_num].player, instance->targets[target_num].card, KILL_REMOVE);
				}
				target_num++;
			}
			if( choice & CHOICE_GAIN_LIFE ){
				gain_life(player, 5);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_shielded_by_faith(int player, int card, event_t event){
	/* Shielded by Faith	|1|W|W	0x200e33c
	 * Enchantment - Aura
	 * Enchant creature
	 * Enchanted creature has indestructible.
	 * Whenever a creature enters the battlefield, you may attach ~ to that creature. */

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) && reason_for_trigger_controller == player ){
		card_instance_t *instance = get_card_instance( player, card );

		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( p > -1 ){
			if( !(trigger_cause_controller == player && trigger_cause == player) ){
				int ai_mode = RESOLVE_TRIGGER_OPTIONAL;
				if( player == AI ){
					ai_mode = trigger_cause_controller == player && get_cmc(trigger_cause_controller, trigger_cause) > get_cmc(p, c) ?
															RESOLVE_TRIGGER_MANDATORY : 0;
				}
				if( specific_cip(player, card, event, ANYBODY, ai_mode, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0)){
					instance->damage_target_player = instance->targets[1].player;
					instance->damage_target_card = instance->targets[1].card;
				}
			}
		}
	}

	return generic_aura(player, card, event, player, 0, 0, 0, SP_KEYWORD_INDESTRUCTIBLE, 0, 0, 0);
}

// Blue
/* AEthersnatch	|4|U|U	0x000000 --> Too much trouble. Waitlisted.
 * Instant
 * Gain control of target spell. You may choose new targets for it. */

/* Broodbirth Viper	|4|U	0x200456c --> Neurok Commando. As above, Myriad is worthless in Manalink.
 * Creature - Snake 3/3
 * Myriad
 * Whenever ~ deals combat damage to a player, you may draw a card. */

 static int effect_gigantoplasm(int player, int card, event_t event){
	if (effect_follows_control_of_attachment(player, card, event))
		return 0;

	card_instance_t* instance = get_card_instance(player, card);
	int p = instance->damage_target_player, c = instance->damage_target_card;
	if( p > -1 && c > -1 ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(p, c) && instance->targets[1].card > -1 ){
			if( check_status(player, card, STATUS_CONTROLLED) ){
				event_result = instance->targets[1].card;
			}
		}

		if( event == EVENT_CAN_ACTIVATE ){
			return granted_generic_activated_ability(player, card, p, c, event, 0, MANACOST_X(-1), 0, NULL, NULL);
		}

		if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(p, c, MANACOST0);
			if( spell_fizzled != 1 ){
				charge_mana(player, COLOR_ANY, -1);
				if( spell_fizzled != 1 ){
					instance->targets[1].player = x_value;
				}
			}
		}

		if( event == EVENT_RESOLVE_ACTIVATION ){
			get_card_instance(instance->parent_controller, instance->parent_card)->targets[1].card = instance->targets[1].player;
			disable_other_pt_setting_effects_attached_to_me(p, c);
			if( ! check_status(instance->parent_controller, instance->parent_card, STATUS_CONTROLLED) ){
				add_status(instance->parent_controller, instance->parent_card, STATUS_CONTROLLED);
			}
		}
	}

  return 0;
}

int card_gigantoplasm(int player, int card, event_t event)
{
	/* Gigantoplasm	|3|U	0x200e341
	 * Creature - Shapeshifter 0/0
	 * You may have ~ enter the battlefield as a copy of any creature on the battlefield except it gains
	 *	"|X: This creature has base power and toughness X/X." */

	if (enters_the_battlefield_as_copy_of_any_creature(player, card, event))
	{
	  set_legacy_image(player, CARD_ID_GIGANTOPLASM, create_targetted_legacy_activate(player, card, effect_gigantoplasm, player, card));
	}

	return 0;
}

/* Illusory Ambusher	|4|U	0x401000 --> vanilla, effect is on 'effect_damage'
 * Creature - Cat Illusion 4/1
 * Flash
 * Whenever ~ is dealt damage, draw that many cards. */

static int exile_at_end_of_combat(int player, int card, event_t event){

	if( end_of_combat_trigger(player, card, event, ANYBODY) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 ){
			kill_card(instance->damage_target_player, instance->damage_target_card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_mirror_match(int player, int card, event_t event)
{
	/* Mirror Match	|4|U|U	0x200e346
	 * Instant
	 * Cast ~ only during the declare blockers step.
	 * For each creature attacking you or a planeswalker you control, put a token that's a copy of that creature onto
	 *	the battlefield blocking that creature. Exile those tokens at end of combat. */

	if( event == EVENT_CAN_CAST && basic_spell(player, card, event) ){
		return current_phase == PHASE_AFTER_BLOCKING ? 1 : 0;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int c;
		for (c = 0; c < active_cards_count[1-player]; ++c){
			if (in_play(1-player, c) && is_what(1-player, c, TYPE_CREATURE) && (get_card_instance(1-player, c)->state & STATE_ATTACKING))
			{
				int csvid = get_id(1-player, c);
				int block_ = 1;
				if( check_status(1-player, c, STATUS_ANIMATED) ){
					if( !(cards_data[get_original_internal_card_id(1-player, c)].type & TYPE_CREATURE) ){
						//We're dealing with an noncreature animated permanent, so the token won't be animated and won't be able to block;
						csvid = cards_data[get_original_internal_card_id(1-player, c)].id;
						block_ = 0;
					}
				}
				token_generation_t token;
				default_token_definition(player, card, csvid, &token);
				if( block_ ){
					token.action = TOKEN_ACTION_BLOCKING;
					token.action_argument = c;
				}
				token.legacy = 1;
				token.special_code_for_legacy = &exile_at_end_of_combat;
				generate_token(&token);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mystic_confluence(int player, int card, event_t event){
	/* Mystic Confluence	|3|U|U	0x200e34b
	 * Instant
	 * Choose three. You may choose the same mode more than once.
	 * * Counter target spell unless its controller pays |3.
	 * * Return target creature to its owner's hand.
	 * * Draw a card. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum
	{
		CHOICE_COUNTERSPELL = 1,
		CHOICE_BOUNCE,
		CHOICE_DRAW,
	};

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		int result = counterspell(player, card, event, NULL, 0);
		return result ? result : 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = instance->number_of_targets = 0;
		int can_counter = counterspell(player, card, EVENT_CAN_CAST, NULL, 0);
		int choices[3] = {0, 0, 0};
		int i;
		int base_target = 0;
		for(i=0; i<3; i++){
			int priority_counter = 20;
			if( can_counter && player == AI ){
				int mana_to_pay = 0;
				if( choices[0] == CHOICE_COUNTERSPELL ){
					priority_counter-=5;
					mana_to_pay+=3;
				}
				if( choices[1] == CHOICE_COUNTERSPELL ){
					priority_counter-=5;
					mana_to_pay+=3;
				}
				if( ! has_mana(card_on_stack_controller, COLOR_ANY, mana_to_pay) ){
					priority_counter = 5;
				}
			}

			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"Mana Leak", can_counter, priority_counter,
								"Bounce a creature", can_target(&td), 10,
								"Draw a card", 1, 5);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			if( choice == CHOICE_COUNTERSPELL ){
				counterspell(player, card, event, NULL, base_target);
				choices[i] = choice;
				base_target++;
			}
			if( choice == CHOICE_BOUNCE){
				if( new_pick_target(&td, "TARGET_CREATURE", base_target, 0) ){
					choices[i] = choice;
					if( player == AI ){
						state_untargettable(instance->targets[base_target].player, instance->targets[base_target].card, 1);
					}
					base_target++;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			if( choice == CHOICE_DRAW ){
				choices[i] = choice;
			}
		}
		SET_BYTE0(instance->info_slot) |= choices[0];
		SET_BYTE1(instance->info_slot) |= choices[1];
		SET_BYTE2(instance->info_slot) |= choices[2];
		for(i=0; i<instance->number_of_targets; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int target_num = 0;
		int i;
		for(i=0; i<3; i++){
			int choice = BYTE0(instance->info_slot);
			if( i == 1 ){
				choice = BYTE1(instance->info_slot);
			}
			if( i == 2 ){
				choice = BYTE2(instance->info_slot);
			}
			if( choice & CHOICE_COUNTERSPELL ){
				counterspell_resolve_unless_pay_x(player, card, NULL, target_num, 3);
				target_num++;
			}
			if( choice & CHOICE_BOUNCE ){
				if( validate_target(player, card, &td, target_num) ){
					bounce_permanent(instance->targets[target_num].player, instance->targets[target_num].card);
				}
				target_num++;
			}
			if( choice & CHOICE_DRAW ){
				draw_cards(player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int synthetic_destiny_legacy(int player, int card, event_t event)
{
	if( eot_trigger(player, card, event) ){
		card_instance_t* instance = get_card_instance(player, card);
		if( instance->targets[0].player > 0 ){
			int *deck = deck_ptr[player];
			int found = 0;
			int break_point = -1;
			int i;
			for(i=0; i<500; i++){
				if( deck[i] != -1 ){
					if( is_what(-1, deck[i], TYPE_CREATURE) ){
						found++;
						if( found == instance->targets[0].player ){
							break_point = i;
							break;
						}
					}
				}
				else{
					break_point = i;
					break;
				}
			}

			if( break_point > -1 ){
				show_deck(player, deck, break_point+1, "Cards revealed by Synthetic Destiny", 0, 0x7375B0 );

				int added[instance->targets[0].player];
				int ac = 0;

				for(i=break_point; i>-1; i--){
					if( is_what(-1, deck[i], TYPE_CREATURE) ){
						added[ac] = add_card_to_hand(player, deck[i]);
						remove_card_from_deck(player, i);
						ac++;
					}
				}

				for(i=0; i<ac; i++){
					put_into_play(player, added[i]);
				}

				shuffle(player);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_synthetic_destiny(int player, int card, event_t event)
{
	/* Synthetic Destiny	|4|U|U	0x200e350
	 * Instant
	 * Exile all creatures you control. At the beginning of the next end step, reveal cards from the top of your library until
	 *	you reveal that many creature cards, put all creature cards revealed this way onto the battlefield, then shuffle the rest of the
	 *	revealed cards into your library. */

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int total_cmc[2] = {0, 0};
		APNAP(p,{
					int c;
					for(c=0; c<active_cards_count[p]; c++){
						if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) ){
							total_cmc[p] += get_cmc(p, c);
						}
					}
				};
		);
		ai_modifier += ((total_cmc[1-player]-total_cmc[player])*10);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		int result = new_manipulate_all(player, card, player, &this_test, KILL_REMOVE);

		int legacy = create_legacy_effect(player, card, &synthetic_destiny_legacy);
		card_instance_t* instance = get_card_instance(player, legacy);
		instance->targets[0].player = result;

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

// Black
int card_banshee_of_the_dread_choir(int player, int card, event_t event)
{
	/* Banshee of the Dread Choir	|3|B|B	0x200e355
	 * Creature - Spirit 4/4
	 * Myriad
	 * Whenever ~ deals combat damage to a player, that player discards a card. */

 // Myrid is worthless under Manalink
	whenever_i_deal_damage_to_a_player_he_discards_a_card(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE, 0);
	return 0;
}

int card_corpse_augur(int player, int card, event_t event)
{
	/* Corpse Augur	|3|B	0x200e35a
	 * Creature - Zombie Wizard 4/2
	 * When ~ dies, you draw X cards and you lose X life, where X is the number of creature cards in target player's graveyard. */

	if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			int amount = count_graveyard_by_type(instance->targets[0].player, TYPE_CREATURE);
			draw_cards(player, amount);
			lose_life(player, amount);
		}
	}

	return 0;
}

int card_daxos_torment(int player, int card, event_t event)
{
	/* Daxos's Torment	|3|B	0x200e35f
	 * Enchantment
	 * Constellation - Whenever ~ or another enchantment enters the battlefield under your control, ~ becomes a
	 *	5/5 Demon creature with flying and haste in addition to its other types until end of turn. */

	if( comes_into_play(player, card, event) || constellation(player, card, event) ){
		int legacy = turn_into_creature(player, card, player, card, 1, 5, 5);
		card_instance_t *instance = get_card_instance( player, legacy);
		instance->targets[8].player = KEYWORD_FLYING;
		instance->targets[8].card = SP_KEYWORD_HASTE;
	}

	return 0;
}

int card_deadly_tempest(int player, int card, event_t event)
{
	/* Deadly Tempest	|4|B|B	0x200e364
	 * Sorcery
	 * Destroy all creatures. Each player loses life equal to the number of creatures he or she controlled that were destroyed this way. */

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);

		APNAP(p,{
					int result = new_manipulate_all(player, card, player, &this_test, KILL_REMOVE);
					lose_life(p, result);
				};
		);

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_dread_summons(int player, int card, event_t event)
{
	/* Dread Summons	|X|B|B	0x200e369
	 * Sorcery
	 * Each player puts the top X cards of his or her library into his or her graveyard.
	 *	For each creature card put into a graveyard this way, you put a 2/2 |Sblack Zombie creature token onto the battlefield tapped. */

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p,{ special_mill(player, card, get_id(player, card), p, get_card_instance(player, card)->info_slot); };);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 1, NULL);
}

int card_scourge_of_nel_toth(int player, int card, event_t event)
{
	/* Scourge of Nel Toth	|5|B|B	0x200e36e
	 * Creature - Zombie Dragon 6/6
	 * Flying
	 * You may cast ~ from your graveyard by paying |B|B and sacrificing two creatures rather than paying its mana cost. */

	if( event == EVENT_GRAVEYARD_ABILITY && has_mana(player, COLOR_BLACK, 2) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.qty = 2;
		if( new_can_sacrifice_as_cost(player, card, &test) ){
			return GA_PLAYABLE_FROM_GRAVE;
		}
	}

	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "Select a creature to sacrifice.");
		int sac = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
		if (sac){
			int sac2 = new_sacrifice(player, card, player, SAC_JUST_MARK|SAC_AS_COST|SAC_RETURN_CHOICE, &test);
			if( sac2 ){
				charge_mana(player, COLOR_BLACK, 2);
				if( spell_fizzled != 1 ){
					kill_card(BYTE2(sac), BYTE3(sac), KILL_SACRIFICE);
					kill_card(BYTE2(sac2), BYTE3(sac2), KILL_SACRIFICE);
					return GAPAID_REMOVE;
				}
				else{
					state_untargettable(BYTE2(sac), BYTE3(sac), 0);
					state_untargettable(BYTE2(sac2), BYTE3(sac2), 0);
					spell_fizzled = 1;
					return 0;
				}
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

	return 0;
}

int card_thief_of_blood(int player, int card, event_t event)
{
	/* Thief of Blood	|4|B|B	0x200e373
	 * Creature - Vampire 1/1
	 * Flying
	 * As ~ enters the battlefield, remove all counters from all permanents. ~ enters the battlefield with a +1/+1 counter on it
	 *	for each counter removed this way. */

	if( event == EVENT_RESOLVE_SPELL ){
		int plus = 0;
		APNAP(p,{
					int c;
					for(c=active_cards_count[p]-1; c>-1; c--){
						if( in_play(p, c) && is_what(p, c, TYPE_PERMANENT) ){
							plus += count_counters(p, c, -1);
							remove_all_counters(p, c, -1);
						}
					}
				};
		);
		add_1_1_counters(player, card, plus);
	}

	return 0;
}

int card_wretched_confluence(int player, int card, event_t event)
{
	/* Wretched Confluence	|3|B|B	0x200e378
	 * Instant
	 * Choose three. You may choose the same mode more than once.
	 * * Target player draws a card and loses 1 life.
	 * * Target creature gets -2/-2 until end of turn.
	 * * Return target creature card from your graveyard to your hand. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum
	{
		CHOICE_DRAW = 1,
		CHOICE_M2_M2,
		CHOICE_RAISE_DEAD,
	};

	target_definition_t td;
	default_target_definition(player, card, &td, 0 );
	td.zone = TARGET_ZONE_PLAYERS;

	target_definition_t td2;
	default_target_definition(player, card, &td2, TYPE_CREATURE );

	test_definition_t test;
	new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td, NULL, 1, NULL) ){
			return 1;
		}
		if( generic_spell(player, card, event, GS_CAN_TARGET, &td2, NULL, 1, NULL) ){
			return 1;
		}
		if( generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &test) ){
			return 1;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = instance->number_of_targets = 0;
		int can_reanimate = generic_spell(player, card, EVENT_CAN_CAST, GS_GRAVE_RECYCLER, NULL, NULL, 1, &test);
		int choices[3] = {0, 0, 0};
		int i;
		int base_target = 0;
		for(i=0; i<3; i++){
			int priority_reanimate = 15;
			if( choices[0] == CHOICE_RAISE_DEAD || choices[1] == CHOICE_RAISE_DEAD ){
				priority_reanimate = 0;
			}

			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"Target player draw & lose 1", can_target(&td), 5,
								"-2/-2 to target creature", can_target(&td2), 10,
								"Raise Dead", can_reanimate, priority_reanimate);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			if( choice == CHOICE_DRAW ){
				if( new_pick_target(&td, "TARGET_PLAYER", base_target, 0) ){
					choices[i] = choice;
					base_target++;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			if( choice == CHOICE_M2_M2){
				if( new_pick_target(&td2, "TARGET_CREATURE", base_target, 0) ){
					choices[i] = choice;
					base_target++;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			if( choice == CHOICE_RAISE_DEAD ){
				if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &test, base_target) == -1 ){
					spell_fizzled = 1;
					return 0;
				}
			}
		}
		SET_BYTE0(instance->info_slot) |= choices[0];
		SET_BYTE1(instance->info_slot) |= choices[1];
		SET_BYTE2(instance->info_slot) |= choices[2];
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int target_num = 0;
		int i;
		for(i=0; i<3; i++){
			int choice = BYTE0(instance->info_slot);
			if( i == 1 ){
				choice = BYTE1(instance->info_slot);
			}
			if( i == 2 ){
				choice = BYTE2(instance->info_slot);
			}
			if( choice & CHOICE_DRAW ){
				if( validate_target(player, card, &td, target_num) ){
					draw_cards(instance->targets[target_num].player, 1);
					lose_life(instance->targets[target_num].player, 1);
				}
				target_num++;
			}
			if( choice & CHOICE_M2_M2 ){
				if( validate_target(player, card, &td2, target_num) ){
					pump_until_eot_merge_previous(player, card, instance->targets[target_num].player, instance->targets[target_num].card, -2, -2);
				}
				target_num++;
			}
			if( choice & CHOICE_RAISE_DEAD ){
				int selected = validate_target_from_grave_source(player, card, player, target_num);
				if( selected != -1 ){
					from_grave_to_hand(player, selected, TUTOR_HAND);
				}
				target_num++;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Red
static int awaken_the_sky_tyrant_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( p == -1 || c == -1 ){
		return 0;
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(p, c) ){
		if( get_card_instance(p, c)->kill_code == KILL_SACRIFICE ){
			instance->targets[11].player = 66;
		}

	}

	if( resolve_graveyard_trigger(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_DRAGON, &token);
		token.pow = token.tou = 5;
		token.key_plus = KEYWORD_FLYING;
		token.color_forced = COLOR_TEST_RED;
		generate_token(&token);

		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_awaken_the_sky_tyrant(int player, int card, event_t event)
{
	/* Awaken the Sky Tyrant	|3|R	0x200e37d
	 * Enchantment
	 * When a source an opponent controls deals damage to you, sacrifice ~. If you do, put a 5/5 |Sred Dragon creature token with flying onto the battlefield. */

	if( damage_dealt_to_me_arbitrary(player, card, event, 0, player, -1) ){
		if( ! check_special_flags2(player, card, SF2_CANNOT_BE_SACRIFICED) ){
			int legacy = create_legacy_effect(player, card, &awaken_the_sky_tyrant_legacy);
			card_instance_t *instance = get_card_instance( player, legacy );
			instance->targets[0].player = player;
			instance->targets[0].card = card;
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return global_enchantment(player, card, event);
}

int card_dream_pillager(int player, int card, event_t event)
{
	/* Dream Pillager	|5|R|R	0x200ee5e
	 * Creature - Dragon 4/4
	 * Flying
	 * Whenever ~ deals combat damage to a player, exile that many cards from the top of your library. Until end of turn, you may cast nonland cards exiled this way. */

	if( damage_dealt_by_me(player, card, event, DDBM_REPORT_DAMAGE_DEALT | DDBM_MUST_BE_COMBAT_DAMAGE | DDBM_MUST_DAMAGE_PLAYER) ){
		card_instance_t *instance = get_card_instance( player, card );

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_LAND);
		this_test.type_flag = DOESNT_MATCH;

		exile_top_x_card_you_may_play_them(player, card, instance->targets[16].player, &this_test, player, MPCFE_UNTIL_EOT);
	}

	return 0;
}

int card_fiery_confluence(int player, int card, event_t event)
{
	/* Fiery Confluence	|2|R|R	0x200e382
	 * Sorcery
	 * Choose three. You may choose the same mode more than once.
	 * * ~ deals 1 damage to each creature.
	 * * ~ deals 2 damage to each opponent.
	 * * Destroy target artifact. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum
	{
		CHOICE_DAMAGE_CREATURES = 1,
		CHOICE_DAMAGE_OPPONENT,
		CHOICE_KILL_ARTIFACT,
	};

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = instance->number_of_targets = 0;
		int choices[3] = {0, 0, 0};
		int i;
		int base_target = 0;
		for(i=0; i<3; i++){
			int priority_dc = 0;
			int priority_do = 40-(life[1-player]*2);
			int priority_ka = 12;
			int p, c;
			for(p=0; p<2; p++){
				for(c=0; c<active_cards_count[p]; c++){
					if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) ){
						int tou = get_toughness(p, c);
						if( p == player ){
							if( tou == 1 ){
								priority_dc -=5;
							}
							if( tou == 2 ){
								priority_dc -=4;
							}
							if( tou == 3 ){
								priority_dc -=3;
							}
						}
						else{
							if( tou == 1 ){
								priority_dc +=5;
							}
							if( tou == 2 ){
								priority_dc +=4;
							}
							if( tou == 3 ){
								priority_dc +=3;
							}
						}
					}
				}
			}

			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"1 damage to all creatures", 1, priority_dc,
								"2 damage to opponent", 1, priority_do,
								"Kill an artifact", can_target(&td), priority_ka);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			if( choice == CHOICE_DAMAGE_CREATURES || choice == CHOICE_DAMAGE_OPPONENT ){
				choices[i] = choice;
			}
			if( choice == CHOICE_KILL_ARTIFACT){
				if( new_pick_target(&td, "TARGET_ARTIFACT", base_target, 0) ){
					if( player == AI ){
						state_untargettable(instance->targets[base_target].player, instance->targets[base_target].card, 1);
					}
					choices[i] = choice;
					base_target++;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
		}
		SET_BYTE0(instance->info_slot) |= choices[0];
		SET_BYTE1(instance->info_slot) |= choices[1];
		SET_BYTE2(instance->info_slot) |= choices[2];
		for(i=0; i<instance->number_of_targets; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int target_num = 0;
		int i;
		for(i=0; i<3; i++){
			int choice = BYTE0(instance->info_slot);
			if( i == 1 ){
				choice = BYTE1(instance->info_slot);
			}
			if( i == 2 ){
				choice = BYTE2(instance->info_slot);
			}
			if( choice & CHOICE_DAMAGE_CREATURES ){
				APNAP(p,{ new_damage_all(player, card, p, 1, NDA_ALL_CREATURES, NULL); };);
			}
			if( choice & CHOICE_DAMAGE_OPPONENT  ){
				damage_player(1-player, 2, player, card);
			}
			if( choice & CHOICE_KILL_ARTIFACT){
				if( validate_target(player, card, &td, target_num) ){
					kill_card(instance->targets[target_num].player, instance->targets[target_num].card, KILL_DESTROY);
				}
				target_num++;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_magus_of_the_wheel(int player, int card, event_t event)
{
	/* Magus of the Wheel	|2|R	0x200e387
	 * Creature - Human Wizard 3/3
	 * |1|R, |T, Sacrifice ~: Each player discards his or her hand, then draws seven cards. */

	if( event == EVENT_RESOLVE_ACTIVATION ){
		APNAP(p,{
				discard_all(p);
				draw_cards(p, 7);
				};
		);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST_XR(1, 1), 0, NULL, NULL);
}

int card_meteor_blast(int player, int card, event_t event)
{
	/* Meteor Blast	|X|R|R|R	0x200e38c
	 * Sorcery
	 * ~ deals 4 damage to each of X target creatures and/or players. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		generic_spell(player, card, event, GS_X_SPELL, NULL, NULL, 0, NULL);
		return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", instance->info_slot, NULL);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				damage_creature(instance->targets[i].player, instance->targets[i].card, 4, player, card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mizzix_mastery(int player, int card, event_t event){
	/* Mizzix's Mastery	|3|R	0x200e391
	 * Sorcery
	 * Exile target card that's an instant or sorcery from your graveyard. For each card exiled this way, copy it, and you may cast the copy without paying its mana cost. Exile ~.
	 * Overload |5|R|R|R */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	char msg[100] = "Select a instant or sorcery card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_SORCERY | TYPE_INSTANT | TYPE_INTERRUPT, msg); //Some interrupts can be cast even if there's no spell on stack

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return instance->info_slot == 1 ? 1 : generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 1, &this_test);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( instance->info_slot == 1 && ! played_for_free(player, card) && ! is_token(player, card) ){
			int ai_choice = 0;
			if( ! graveyard_has_shroud(player) && new_special_count_grave(player, &this_test) < 2 ){
				ai_choice = 1;
			}
			int choice = do_dialog(player, player, card, -1, -1, " Pay Overload cost\n Play the spell normally\n Cancel", ai_choice);
			if( choice == 0 ){
				charge_mana_multi(player, MANACOST_XR(5, 3));
				if( spell_fizzled != 1 ){
					instance->info_slot |= 2;
				}
				else if( choice == 1 ){
						return charge_mana_from_id(player, card, EVENT_CAST_SPELL, get_id(player, card));
				}
				else{
					spell_fizzled = 1;
				}
			}
		}
		if( ! (instance->info_slot & 2) ){
			if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 0) == -1 ){
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int exiled[count_graveyard(player)];
		int ec = 0;
		if( ! (instance->info_slot & 2) ){
			int selected = validate_target_from_grave_source(player, card, player, 0);
			if( selected != -1 ){
				exiled[ec] = cards_data[get_grave(player)[selected]].id;
				rfg_card_from_grave(player, selected);
				ec++;
			}
		}
		else{
			int c;
			for(c=count_graveyard(player)-1; c>-1; c--){
				int iid = get_grave(player)[c];
				if( new_make_test(player, iid, -1, &this_test) ){
					exiled[ec] = cards_data[iid].id;
					rfg_card_from_grave(player, c);
					ec++;
				}
			}
		}
		int i;
		for(i=0; i<ec; i++){
			if( check_rfg(player, exiled[i]) ){
				play_card_in_exile_for_free(player, player, exiled[i]);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return overload(player, card, event, MANACOST_XR(5, 3));
}

int card_lightning_rager(int player, int card, event_t event){
	/* Lightning Rager |0	0x200e396
	 * Creature - Elemental 5/1
	 * Trample, Haste
	 * At the beginning of the end step, sacrifice Lightning Rager.
	 */

	if( ! is_humiliated(player, card) ){
		haste(player, card, event);
		if( ! check_state(player, card, STATE_OUBLIETTED) && eot_trigger(player, card, event) ){
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_token(player, card, event);
}

int card_rite_of_the_raging_storm(int player, int card, event_t event){
	/* Rite of the Raging Storm	|3|R|R	0x200e39b
	 * Enchantment
	 * Creatures named Lightning Rager can't attack you or planeswalkers you control.
	 * At the beginning of each player's upkeep, that player puts a 5/1 |Sred Elemental creature token named Lightning Rager onto the battlefield. It has trample, haste, and "At the beginning of the end step, sacrifice this creature." */

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_ATTACK_LEGALITY && get_id(affected_card_controller, affected_card) == CARD_ID_LIGHTNING_RAGER &&
			current_turn != player)
		{
			forbid_attack = 1;
		}

		upkeep_trigger_ability(player, card, event, ANYBODY);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_LIGHTNING_RAGER, &token);
			token.t_player = current_turn;
			generate_token(&token);
		}
	}

	return global_enchantment(player, card, event);
}

/* Warchief Giant	|3|R|R	0x200bb46 --> Raging Goblin, Myriad is worthless under Manalink
 * Creature - Giant Warrior 5/3
 * Haste
 * Myriad */

// Green

int card_arachnogenesis(int player, int card, event_t event){
	/* Arachnogenesis	|2|G	0x200e3a0
	 * Instant
	 * Put X 1/2 |Sgreen Spider creature tokens with reach onto the battlefield, where X is the number of creatures attacking you.
	 *	Prevent all combat damage that would be dealt this turn by non-Spider creatures. */

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( current_turn == player || current_phase != PHASE_AFTER_BLOCKING || count_attackers(current_turn) < 1 ){
			ai_modifier-=25;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int amount = 0;
		int c;
		for(c=0; c<active_cards_count[1-player]; c++){
			if( in_play(1-player, c) && is_what(1-player, c, TYPE_CREATURE) && check_state(1-player, c, STATE_ATTACKING) ){
				if( ! check_special_flags(1-player, c, SF_ATTACKING_PWALKER) ){
					amount++;
				}
			}
		}

		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SPIDER, &token);
		token.pow = 1;
		token.tou = 2;
		token.color_forced = COLOR_TEST_GREEN;
		token.key_plus = KEYWORD_REACH;
		token.qty = amount;
		generate_token(&token);

		fog_special2(player, card, ANYBODY, FOG_COMBAT_DAMAGE_ONLY | FOG_BY_NON_SUBTYPE, SUBTYPE_SPIDER);

		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_bloodspore_thrinax(int player, int card, event_t event){
	/* Bloodspore Thrinax	|2|G|G	0x200e3a5
	 * Creature - Lizard 2/2
	 * Devour 1
	 * Each other creature you control enters the battlefield with an additional X +1/+1 counters on it, where X is the number of +1/+1 counters on ~. */

	devour(player, card, event, 1);

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		if( (event == EVENT_CAST_SPELL || event == EVENT_ENTERING_THE_BATTLEFIELD_AS_CLONE) &&
			is_what(affected_card_controller, affected_card, TYPE_CREATURE))
		{
			enters_the_battlefield_with_counters(affected_card_controller, affected_card, event, COUNTER_P1_P1, count_counters(player, card, COUNTER_P1_P1));
		}
	}

	return 0;
}

/* Caller of the Pack	|5|G|G	0x401000 --> Vanilla, Myriad is worthless under Manalink
 * Creature - Beast 8/6
 * Trample
 * Myriad */

int card_centaur_vinecrasher(int player, int card, event_t event){
	/* Centaur Vinecrasher	|3|G	0x200e409
	 * Creature - Plant Centaur 1/1
	 * Trample
	 * ~ enters the battlefield with a number of +1/+1 counters on it equal to the number of land cards in all graveyards.
	 * Whenever a land card is put into a graveyard from anywhere, you may pay |G|G. If you do, return ~ from your graveyard to your hand. */

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, count_graveyard_by_type(player, TYPE_LAND)+count_graveyard_by_type(1-player, TYPE_LAND));

	return 0;
}

static void mark_ezuri_predation_token(token_generation_t* token, int beast, int number)
{
	if( number != 0 ){
		return;
	}
	add_status(token->t_player, beast, STATUS_PERMANENT);
}

static const char* check_for_state_targetted(int who_chooses, int player, int card)
{
	if( ! check_state(player, card, STATE_TARGETTED) ){
		return NULL;
	}
	return "already targetted by another Beast";
}

int card_ezuris_predation(int player, int card, event_t event){
	/* Ezuri's Predation	|5|G|G|G	0x200e3aa
	 * Sorcery
	 * For each creature your opponents control, put a 4/4 |Sgreen Beast creature token onto the battlefield.
	 *	Each of those Beasts fights a different one of those creatures. */

	if( event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BEAST, &token);
		token.pow = token.tou = 4;
		token.color_forced = COLOR_TEST_GREEN;
		token.qty = count_subtype(1-player, TYPE_CREATURE, -1);
		token.special_code_on_generation = &mark_ezuri_predation_token;
		generate_token(&token);

		int c;
		for(c=active_cards_count[player]-1; c>-1; c--){
			if( in_play(player, c) && get_id(player, c) == CARD_ID_BEAST && check_status(player, c, STATUS_PERMANENT) ){
				remove_status(player, c, STATUS_PERMANENT);

				target_definition_t td;
				default_target_definition(player, c, &td, TYPE_CREATURE);
				td.allowed_controller = 1-player;
				td.allow_cancel = 0;
				td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
				td.extra = (int)check_for_state_targetted;

				card_instance_t *instance = get_card_instance(player, c);

				if( can_target(&td) && new_pick_target(&td, "Select target creature your opponent controls.", 0, GS_LITERAL_PROMPT) ){
					add_state(instance->targets[0].player, instance->targets[0].card, STATE_TARGETTED);//So it won't be chosen a second time
					fight(player, card, instance->targets[0].player, instance->targets[0].card);
				}
				instance->number_of_targets = 0;
			}
		}

		for(c=active_cards_count[1-player]-1; c>-1; c--){
			if( in_play(1-player, c) && check_state(1-player, c, STATE_TARGETTED) ){
				remove_state(1-player, c, STATE_TARGETTED);
			}
		}

		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_great_oak_guardian(int player, int card, event_t event){
	/* Great Oak Guardian	|5|G	0x200e3af
	 * Creature - Treefolk 4/5
	 * Flash
	 * Reach
	 * When ~ enters the battlefield, creatures target player controls get +2/+2 until end of turn. Untap them. */

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;

		card_instance_t *instance = get_card_instance(player, card);

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			pump_creatures_until_eot(player, card, player, 0, 2, 2, 0, 0, NULL);

			test_definition_t test;
			default_test_definition(&test, TYPE_CREATURE);
			new_manipulate_all(player, card, player, &test, ACT_UNTAP);
		}
		instance->number_of_targets = 0;
	}

	return flash(player, card, event);
}

int card_pathbreaker_ibex(int player, int card, event_t event){
	/* Pathbreaker Ibex	|4|G|G	0x200e3b4
	 * Creature - Goat 3/3
	 * Whenever ~ attacks, creatures you control gain trample and get +X/+X until end of turn, where X is the greatest power among creatures you control. */

	if( declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, card) ){
		int amount = get_highest_power(player);
		pump_creatures_until_eot(player, card, player, 0, amount, amount, KEYWORD_TRAMPLE, 0, NULL);
	}

	return 0;
}

int card_skullwinder(int player, int card, event_t event){
	/* Skullwinder	|2|G	0x200e3b9
	 * Creature - Snake 1/3
	 * Deathtouch
	 * When ~ enters the battlefield, return target card from your graveyard to your hand, then choose an opponent.
	 *	That player returns a card from his or her graveyard to his or her hand. */

	deathtouch(player, card, event);

	if( comes_into_play(player, card, event) ){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select a card to return to your hand.");

		if( ! graveyard_has_shroud(player) && count_graveyard(player) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &test);
			if( count_graveyard(1-player) ){
				new_global_tutor(1-player, 1-player, TUTOR_FROM_GRAVE, TUTOR_HAND, 1, AI_MAX_VALUE, &test);
			}
		}
	}

	return 0;
}

int card_verdant_confluence(int player, int card, event_t event){
	/* Verdant Confluence	|4|G|G	0x000000
	 * Sorcery
	 * Choose three. You may choose the same mode more than once.
	 * * Put two +1/+1 counters on target creature.
	 * * Return target permanent card from your graveyard to your hand.
	 * * Search your library for a basic land card, put it onto the battlefield tapped, then shuffle your library. */

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	enum
	{
		CHOICE_P1_P1_COUNTER = 1,
		CHOICE_REGROWTH,
		CHOICE_TUTOR_BASIC_LAND,
	};

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );

	test_definition_t test;
	new_default_test_definition(&test, TYPE_PERMANENT, "Select a permanent card.");

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return basic_spell(player, card, event);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = instance->number_of_targets = 0;
		int can_reanimate = generic_spell(player, card, EVENT_CAN_CAST, GS_GRAVE_RECYCLER, NULL, NULL, 1, &test);
		int choices[3] = {0, 0, 0};
		int i;
		int base_target = 0;
		for(i=0; i<3; i++){
			int priority_reanimate = 15;
			if( choices[0] == CHOICE_REGROWTH || choices[1] == CHOICE_REGROWTH ){
				priority_reanimate = 0;
			}

			int choice = DIALOG(player, card, event, DLG_NO_STORAGE, DLG_RANDOM,
								"2 +1/+1 counters on target creature", can_target(&td), 10,
								"Return a permanent from grave to hand", can_reanimate, priority_reanimate,
								"Tutor basic land tapped", 1, 5);

			if( ! choice ){
				spell_fizzled = 1;
				return 0;
			}
			if( choice == CHOICE_P1_P1_COUNTER ){
				if( new_pick_target(&td, "TARGET_CREATURE", base_target, 0) ){
					choices[i] = choice;
					base_target++;
				}
				else{
					spell_fizzled = 1;
					return 0;
				}
			}
			if( choice == CHOICE_REGROWTH ){
				if( select_target_from_grave_source(player, card, player, 0, AI_MAX_VALUE, &test, base_target) == -1 ){
					spell_fizzled = 1;
					return 0;
				}
			}
			if( CHOICE_TUTOR_BASIC_LAND){
				choices[i] = choice;
			}
		}
		SET_BYTE0(instance->info_slot) |= choices[0];
		SET_BYTE1(instance->info_slot) |= choices[1];
		SET_BYTE2(instance->info_slot) |= choices[2];
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int target_num = 0;
		int i;
		for(i=0; i<3; i++){
			int choice = BYTE0(instance->info_slot);
			if( i == 1 ){
				choice = BYTE1(instance->info_slot);
			}
			if( i == 2 ){
				choice = BYTE2(instance->info_slot);
			}
			if( choice & CHOICE_P1_P1_COUNTER ){
				if( validate_target(player, card, &td, target_num) ){
					add_1_1_counters(instance->targets[target_num].player, instance->targets[target_num].card, 2);
				}
				target_num++;
			}
			if( choice & CHOICE_REGROWTH ){
				int selected = validate_target_from_grave_source(player, card, player, target_num);
				if( selected != -1 ){
					from_grave_to_hand(player, selected, TUTOR_HAND);
				}
				target_num++;
			}
			if( choice & CHOICE_TUTOR_BASIC_LAND ){
				tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Gold

int card_anya_merciless_angel(int player, int card, event_t event){
	/* Anya, Merciless Angel	|3|R|W	0x200e3c3
	 * Legendary Creature - Angel 4/4
	 * Flying
	 * ~ gets +3/+3 for each opponent whose life total is less than half his or her starting life total.
	 * As long as an opponent's life total is less than half his or her starting life total, ~ has indestructible. */

	check_legend_rule(player, card, event);

	if( ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			if( life[1-player] < get_starting_life_total(1-player) / 2 ){
				event_result += 3;
			}
		}
		if( event == EVENT_ABILITIES && affect_me(player, card) ){
			if( life[1-player] < get_starting_life_total(1-player) / 2 ){
				indestructible(player, card, event);
			}
		}
	}

	return 0;
}

int card_arjun_the_shifting_flame(int player, int card, event_t event){
	/* Arjun, the Shifting Flame	|4|U|R	0x200e3c8
	 * Legendary Creature - Sphinx Wizard 5/5
	 * Flying
	 * Whenever you cast a spell, put the cards in your hand on the bottom of your library in any order, then draw that many cards. */

	check_legend_rule(player, card, event);

	if( new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, NULL) ){
		int amount = hand_count[player];

		test_definition_t test;
		new_default_test_definition(&test, TYPE_ANY, "Select a card to put on bottom of deck");

		while( hand_count[player] ){
				new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_BOTTOM_OF_DECK, 1, AI_FIRST_FOUND, &test);
		}

		draw_cards(player, amount);
	}

	return 0;
}

int card_daxos_the_returned(int player, int card, event_t event){
	/* Daxos the Returned	|1|W|B	0x200e3cd
	 * Legendary Creature - Zombie Soldier 2/2
	 * Whenever you cast an enchantment spell, you get an experience counter.
	 * |1|W|B: Put a |Swhite and |Sblack Spirit enchantment creature token onto the battlefield.
	 *	It has "This creature's power and toughness are each equal to the number of experience counters you have." */

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_ENCHANTMENT);
		if( new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test) ){
			add_experience(player, 1);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_BIRD, &token);
		token.special_flags2 = SF2_ENCHANTED_EVENING;
		token.special_infos = 66;
		generate_token(&token);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XWB(1, 1, 1), 0, NULL, NULL);
}

int card_ezuri_claw_of_progress(int player, int card, event_t event){
	/* Ezuri, Claw of Progress	|2|G|U	0x200e3d2
	 * Legendary Creature - Elf Warrior 3/3
	 * Whenever a creature with power 2 or less enters the battlefield under your control, you get an experience counter.
	 * At the beginning of combat on your turn, put X +1/+1 counters on another target creature you control,
	 *	where X is the number of experience counters you have. */

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.power = 3;
		test.power_flag = F5_POWER_LESSER_THAN_VALUE;
		if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test) ){
			add_experience(player, 1);
		}
	}

	if( beginning_of_combat(player, card, event, player, card) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.special = TARGET_SPECIAL_NOT_ME;
		td.allowed_controller = td.preferred_controller = player;

		card_instance_t *inst = get_card_instance(player, card);

		inst->number_of_targets = 0;

		if( can_target(&td) && new_pick_target(&td, "Select another target creature you control.", 0, GS_LITERAL_PROMPT) ){
			add_1_1_counters(inst->targets[0].player, inst->targets[0].card, EXPERIENCE_COUNTERS(player));
		}
	}

	return 0;
}

int card_kalemne_disciple_of_iroas(int player, int card, event_t event){
	/* Kalemne, Disciple of Iroas	|2|R|W	0x200e3d7
	 * Legendary Creature - Giant Soldier 3/3
	 * Double strike, vigilance
	 * Whenever you cast a creature spell with converted mana cost 5 or greater, you get an experience counter.
	 * ~ gets +1/+1 for each experience counter you have. */

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_CREATURE);
		test.cmc = 4;
		test.cmc_flag = F5_CMC_GREATER_THAN_VALUE;
		if( new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test) ){
			add_experience(player, 1);
		}
	}

	if( ! is_humiliated(player, card) ){
		if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
			event_result += EXPERIENCE_COUNTERS(player);
		}
	}

	return 0;
}


int card_karlov_of_the_ghost_council(int player, int card, event_t event){
	/* Karlov of the Ghost Council	|W|B	0x200e3dc
	 * Legendary Creature - Spirit Advisor 2/2
	 * Whenever you gain life, put two +1/+1 counters on ~.
	 * |W|B, Remove six +1/+1 counters from ~: Exile target creature. */

	if( ! is_humiliated(player, card) && trigger_gain_life(player, card, event, player, RESOLVE_TRIGGER_MANDATORY) ){
		add_1_1_counters(player, card, 2);
	}

	check_legend_rule(player, card, event);

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *inst = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		kill_card(inst->targets[0].player, inst->targets[0].card, KILL_REMOVE);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_BW(1, 1), GVC_COUNTERS(COUNTER_P1_P1, 6), &td, "TARGET_CREATURE");
}

int card_kaseto_orochi_archmage(int player, int card, event_t event){
	/* Kaseto, Orochi Archmage	|1|G|U	0x200e3e1
	 * Legendary Creature - Snake Wizard 2/2
	 * |G|U: Target creature can't be blocked this turn. If that creature is a Snake, it gets +2/+2 until end of turn. */

	check_legend_rule(player, card, event);

	if (!IS_GAA_EVENT(event))
		return 0;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *inst = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
		int pump = has_subtype(inst->targets[0].player, inst->targets[0].card, SUBTYPE_SNAKE) ? 2 : 0;
		pump_ability_until_eot(inst->parent_controller, inst->parent_card, inst->targets[0].player, inst->targets[0].card, pump, pump, 0, SP_KEYWORD_UNBLOCKABLE);
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_UG(1, 1), 0, &td, "TARGET_CREATURE");
}

static void counters_from_mazirek(int player, int card){
	card_instance_t *inst = get_card_instance(player, card);
	int amount = inst->targets[11].card;
	inst->targets[11].card = 0;

	test_definition_t test;
	default_test_definition(&test, TYPE_CREATURE);

	int i;
	for(i=0; i<amount; i++){
		new_manipulate_all(player, card, player, &test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}
}

static int was_sacrificed(int player, int card){
	return get_card_instance(player, card)->kill_code == KILL_SACRIFICE ? 1 : 0;
}

static void count_for_mazirek_ability(int player, int card, event_t event){
	if( event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) ){
		card_instance_t *instance = get_card_instance(player, card);
		if( instance->damage_target_player > -1 && is_what(player, card, TYPE_EFFECT) ){
			if( affect_me(instance->damage_target_player, instance->damage_target_card) ){
				instance->damage_target_player = instance->damage_target_card = -1;
				remove_status(player, card, STATUS_INVISIBLE_FX);
			}
			return;
		}
		if( ! check_status(player, card, STATUS_INVISIBLE_FX) ){
			count_for_gfp_ability_and_store_values_extra(player, card, event, ANYBODY, TYPE_CREATURE, was_sacrificed, 0, 0);
		}
	}
}

static int mazirek_effect(int player, int card, event_t event){

	if( get_card_instance(player, card)->damage_target_player > -1 && effect_follows_control_of_attachment(player, card, event) ){
		return 0;
	}

	count_for_mazirek_ability(player, card, event);

	if( ! check_status(player, card, STATUS_INVISIBLE_FX) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		counters_from_mazirek(player, card);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_mazirek_kraul_death_priest(int player, int card, event_t event){
	/* Mazirek, Kraul Death Priest	|3|B|G	0x200e3e6
	 * Legendary Creature - Insect Shaman 2/2
	 * Flying
	 * Whenever a player sacrifices another permanent, put a +1/+1 counter on each creature you control. */

	check_legend_rule(player, card, event);

	if( event == EVENT_STATIC_EFFECTS && affect_me(player, card) ){
		int found = 0;
		int c;
		for(c=0; c<active_cards_count[player]; c++){
			if( in_play(player, c) && is_what(player, c, TYPE_EFFECT) ){
				card_instance_t *inst = get_card_instance(player, c);
				if( inst->info_slot == (int)mazirek_effect ){
					found = 1;
					break;
				}
			}
		}
		if( ! found ){
			int legacy = create_targetted_legacy_effect(player, card, &mazirek_effect, player, card);
			add_status(player, legacy, STATUS_INVISIBLE_FX);
		}
	}

	count_for_mazirek_ability(player, card, event);

	if( get_card_instance(player, card)->kill_code <= 0 && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		counters_from_mazirek(player, card);
	}

	return 0;
}

int card_meren_of_clan_nel_toth(int player, int card, event_t event){
	/* Meren of Clan Nel Toth	|2|B|G	0x200e3eb
	 * Legendary Creature - Human Shaman 3/4
	 * Whenever another creature you control dies, you get an experience counter.
	 * At the beginning of your end step, choose target creature card in your graveyard.
	 *	If that card's converted mana cost is less than or equal to the number of experience counters you have, return it to the battlefield.
	 *	Otherwise, put it into your hand. */

	check_legend_rule(player, card, event);

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( ! affect_me(player, card) ){
			count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
		}
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		card_instance_t *inst = get_card_instance(player, card);
		int amount = inst->targets[11].card;
		inst->targets[11].card = 0;
		add_experience(player, amount);
	}

	if( current_turn == player && eot_trigger(player, card, event) ){
		if( ! graveyard_has_shroud(player) && count_graveyard_by_type(player, TYPE_CREATURE) ){
			test_definition_t test;
			new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");
			if( player == AI ){
				test.cmc = EXPERIENCE_COUNTERS(player);
				int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MAX_VALUE, -1, &test);
				if( selected != -1 ){
					reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
				}
				else{
					new_default_test_definition(&test, TYPE_CREATURE, "Select a creature card.");
					new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &test);
				}
			}
			else{
				int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 1, AI_MAX_VALUE, -1, &test);
				if( get_cmc_by_internal_id(get_grave(player)[selected]) <= EXPERIENCE_COUNTERS(player) ){
					reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
				}
				else{
					from_grave_to_hand(player, selected, TUTOR_HAND);
				}
			}
		}
	}

	return 0;
}

int card_mizzix_of_the_izmagus(int player, int card, event_t event){
	/* Mizzix of the Izmagnus	|2|U|R	0x200e3f0
	 * Legendary Creature - Goblin Wizard 2/2
	 * Whenever you cast an instant or sorcery spell with converted mana cost greater than the number of experience counters you have,
	 *	you get an experience counter.
	 * Instant and sorcery spells you cast cost |1 less to cast for each experience counter you have. */

	if( event == EVENT_MODIFY_COST_GLOBAL ){
		if( (affected_card_controller == -1 ? event_result : affected_card_controller) == player &&
			in_play(player, card) && ! is_humiliated(player, card) &&
			is_what(affected_card_controller, affected_card, TYPE_SORCERY | TYPE_INSTANT | TYPE_INTERRUPT)
		  ){
			COST_COLORLESS -= EXPERIENCE_COUNTERS(player);
		}
		return 0;
	}

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) ){
		test_definition_t test;
		default_test_definition(&test, TYPE_SORCERY | TYPE_INSTANT | TYPE_INTERRUPT);
		test.cmc = EXPERIENCE_COUNTERS(player)-1;
		test.cmc_flag = F5_CMC_GREATER_THAN_VALUE;
		if( new_specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test) ){
			add_experience(player, 1);
		}
	}

	return 0;
}

// Artifacts
/* Blade of Selves	|2	0x000000 --> Skipped, Myriad is worthless under Manalink
 * Artifact - Equipment
 * Equipped creature has myriad.
 * Equip |4 */

int card_sandstone_oracle(int player, int card, event_t event){
	/* Sandstone Oracle	|7	0x200e3f5
	 * Artifact Creature - Sphinx 4/4
	 * Flying
	 * When ~ enters the battlefield, choose an opponent. If that player has more cards in hand than you, draw cards equal to the difference. */

	if( comes_into_play(player, card, event) ){
		if( hand_count[1-player] > hand_count[player] ){
			draw_cards(player, hand_count[1-player] - hand_count[player]);
		}
	}

	return 0;
}

int card_scytheclaw(int player, int card, event_t event){
	/* Scytheclaw	|5	0x200e3fa
	 * Artifact - Equipment
	 * Living weapon
	 * Equipped creature gets +1/+1.
	 * Whenever equipped creature deals combat damage to a player, that player loses half his or her life, rounded up.
	 * Equip |3 */

	if( is_equipping(player, card) ){
		if (equipped_creature_deals_damage(player, card, event, DDBM_MUST_DAMAGE_PLAYER | DDBM_TRACE_DAMAGED_PLAYERS | DDBM_MUST_BE_COMBAT_DAMAGE))
		{
			card_instance_t* instance = get_card_instance(player, card);
			int times_damaged[2] = { BYTE0(instance->targets[1].player), BYTE1(instance->targets[1].player) };
			instance->targets[1].player = 0;
			APNAP(p,{
						if( times_damaged[p] ){
							lose_life(p, round_up_value(life[p]));
						}
					};
			);
		}
		return vanilla_equipment(player, card, event, 3, 1, 1, 0, 0);
	}

	return living_weapon(player, card, event, 3);
}

int card_seal_of_the_guildpact(int player, int card, event_t event){
	/* Seal of the Guildpact	|5	0x200ee63
	 * Artifact
	 * As ~ enters the battlefield, choose two colors.
	 * Each spell you cast costs |1 less to cast for each of the chosen colors it is. */

	if( event == EVENT_RESOLVE_SPELL ){
		card_instance_t* instance = get_card_instance(player, card);
		// AI part
		instance->targets[0].player = get_deck_color(-1, player);
		// If "get_deck_color" reports only 1 color, AI will choose the following in the "color pie" as second
		int i;
		for(i=1; i<6; i++){
			if( instance->targets[0].player == (1<<i) ){
				instance->targets[0].player |= (1<<(i+1));
				break;
			}
		}
		if( player == HUMAN ){
			instance->targets[0].player = 1<<choose_a_color(player, COLOR_BLACK);
			while( 1 ){
					int selected = 1<<choose_a_color(player, COLOR_BLACK);
					if( ! (instance->targets[0].player & selected) ){
						break;
					}
			}
		}
		for(i=1; i<6; i++){
			if( instance->targets[0].player & (1<<i) ){
				create_card_name_legacy(player, card, CARD_ID_BLACK + (i-1));
			}
		}
	}

	if( event == EVENT_MODIFY_COST_GLOBAL ){
		card_instance_t* instance = get_card_instance(player, card);
		if( (affected_card_controller == -1 ? event_result : affected_card_controller) == player &&
			instance->targets[0].player != -1 &&
			in_play(player, card) && ! is_humiliated(player, card)
		  ){
			int i;
			for(i=1; i<6; i++){
				if( (instance->targets[0].player & (1<<i)) &&
					(get_color(affected_card_controller, affected_card) & (1<<i))
				  ){
					COST_COLORLESS--;
				}
			}
		}
		return 0;
	}

	return 0;
}

int card_thought_vessel(int player, int card, event_t event){
	/* Thought Vessel	|2	0x200e3ff
	 * Artifact
	 * You have no maximum hand size.
	 * |T: Add |1 to your mana pool. */

	if( ! is_humiliated(player, card) && current_turn == player && event == EVENT_MAX_HAND_SIZE ){
		event_result = 1000;
	}

	return mana_producer(player, card, event);
}

// Land

static int locate_edh_card(int player){
	int i;
	for(i=0; i<active_cards_count[player]; i++){
		if( in_play(player, i) && get_id(player, i) == CARD_ID_ELDER_DRAGON_HIGHLANDER ){
			return i;
		}
	}
	return -1;
}

static int is_commander_in_command_zone(int player){
	enum{
			COMM_IN_COMMAND_ZONE 	= 0,
			COMM_IN_PLAY 			= 1,
			COMM_MISSING_IN_ACTION 	= 2
	};

	int edh_card = locate_edh_card(player);
	if( edh_card != -1 ){
		card_instance_t *instance = get_card_instance(player, edh_card);
		if( instance->targets[3].player > -1 ){
			return (instance->targets[3].player & COMM_IN_COMMAND_ZONE) ? 1 : 0;
		}
	}

	return 0;
}

static int count_energy_card_on_commander_card(int player){

	int edh_card = locate_edh_card(player);
	if( edh_card != -1 ){
		return count_counters(player, edh_card, COUNTER_ENERGY);
	}

	return 0;
}

static int put_commander_from_command_zone_to(int player, int where){
	enum{
			COMM_IN_COMMAND_ZONE 	= 0,
			COMM_IN_PLAY 			= 1,
			COMM_MISSING_IN_ACTION 	= 2
	};

	int edh_card = locate_edh_card(player);
	int commander_card = -1;
	if( edh_card != -1 ){
		card_instance_t *instance = get_card_instance(player, edh_card);
		if( instance->targets[3].player > -1 ){
			if( instance->targets[3].player & COMM_IN_COMMAND_ZONE ){
				remove_card_from_rfg(player, cards_data[instance->info_slot].id);
				instance->targets[3].player = COMM_MISSING_IN_ACTION; //So the "commander legacy" will be attached
				commander_card = add_card_to_hand(player, instance->info_slot);
				if( where == TUTOR_PLAY_TAPPED ){
					add_state(player, commander_card, STATE_TAPPED);
				}
				if( where == TUTOR_PLAY_TAPPED || where == TUTOR_PLAY){
					set_special_flags2(player, commander_card, SF2_COMMANDER);
					put_into_play(player, commander_card);
				}
			}
		}
	}

	return commander_card;
}

int card_command_beacon(int player, int card, event_t event){
	/* Command Beacon	""	0x200e404
	 * Land
	 * |T: Add |1 to your mana pool.
	 * |T, Sacrifice ~: Put your commander into your hand from the command zone. */


	if( ! IS_GAA_EVENT(event) ){
		return mana_producer(player, card, event);
	}

	enum
	{
		CHOICE_MANA = 1,
		CHOICE_SAC
	};

	if( event == EVENT_CAN_ACTIVATE ){
		int result = mana_producer(player, card, event);
		if( ! result ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->info_slot = 0;
		int can_produce_mana_ = mana_producer(player, card, EVENT_CAN_ACTIVATE);
		int can_sacrifice_ = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
		int priority_sac = 0;
		if( is_commander_in_command_zone(player) && count_energy_card_on_commander_card(player) > 0 ){
			priority_sac = 10;
		}
		int choice = DIALOG(player, card, event, DLG_RANDOM,
							"Produce mana",	can_produce_mana_, 5,
							"Sac & put Commander in hand", can_sacrifice_, priority_sac);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		if( choice == CHOICE_MANA ){
			return mana_producer(player, card, event);
		}
		if( choice == CHOICE_SAC ){
			tap_card(player, card);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( get_card_instance(player, card)->info_slot == CHOICE_SAC ){
			put_commander_from_command_zone_to(player, TUTOR_HAND);
		}
	}

	return 0;
}

// And a bunch of reprints.
