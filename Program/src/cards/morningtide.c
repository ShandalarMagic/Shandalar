#include "manalink.h"

// Functions
static void cmc_mod_for_prowl(int player, int card, event_t event, int sub1, int sub2, int colorless, int black, int blue, int green, int red, int white)
{
  if (event == EVENT_MODIFY_COST)
	{
	  card_instance_t* instance = get_card_instance(player, card);
		int c1 = get_updated_casting_cost(player, card, -1, event, colorless);
	  if (has_mana_multi(player, c1, black, blue, green, red, white)
		  && (((sub1 == SUBTYPE_GOBLIN || sub2 == SUBTYPE_GOBLIN) && get_trap_condition(player, TRAP_PROWL_GOBLIN))
			  || ((sub1 == SUBTYPE_ROGUE || sub2 == SUBTYPE_ROGUE) && get_trap_condition(player, TRAP_PROWL_ROGUE))
			  || ((sub1 == SUBTYPE_FAERIE || sub2 == SUBTYPE_FAERIE) && get_trap_condition(player, TRAP_PROWL_FAERIE))))
		{
		  instance->targets[4].player = 66;
		  null_casting_cost(player, card);
		}
	  else
		instance->targets[4].player = 0;
	}
}

static void pay_prowl(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	card_instance_t *instance = get_card_instance(player, card);
	if( ! played_for_free(player, card) && ! is_token(player, card) && instance->targets[4].player == 66){
		int c1 = get_updated_casting_cost(player, card, -1, event, colorless);
		int choice = 0;
		if( has_mana_multi(player, c1, black, blue, green, red, white) ){
			if( has_mana_to_cast_iid(player, event, instance->internal_card_id) ){
				choice = do_dialog(player, player, card, -1, -1, " Pay Prowl cost\n Play the spell normally\n Cancel", 0);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			charge_mana_multi(player, c1, black, blue, green, red, white);
			if( spell_fizzled != 1 ){
				instance->info_slot = 1;
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

static int prowl(int player, int card, event_t event, int sub1, int sub2, int colorless, int black, int blue, int green, int red, int white){
	cmc_mod_for_prowl(player, card, event, sub1, sub2, colorless, black, blue, green, red, white);
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pay_prowl(player, card, event, colorless, black, blue, green, red, white);
	}
	return 0;
}

static int banneret(int player, int card, event_t event, int sub1, int sub2 ){
	if(event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		if( has_subtype(affected_card_controller, affected_card, sub1 ) ||
			has_subtype(affected_card_controller, affected_card, sub2 )
		  ){
			COST_COLORLESS--;
		}
	}
	return 0;
}

static int reinforce(int player, int card, event_t event, int amount_counter, int colorless, int black, int blue, int green, int red, int white){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		if( has_mana_multi(player, colorless, black, blue, green, red, white) ){
			return can_target(&td);
		}
	}
	else if( event == EVENT_ACTIVATE_FROM_HAND ){
			charge_mana_multi(player, colorless, black, blue, green, red, white);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				discard_card(player, card);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
			add_1_1_counters(instance->targets[0].player, instance->targets[0].card, amount_counter);
	}
	return 0;
}

static int kinship(int player, int card, event_t event){

	int *deck = deck_ptr[player];

	upkeep_trigger_ability_mode(player, card, event, player, deck[0] != -1 ? RESOLVE_TRIGGER_AI(player) : 0);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( deck[0] != -1 ){
			if( shares_creature_subtype(-1, deck[0], player, card) ){
				if (reveal_card_optional_iid(player, card, deck[0], NULL)){
					return 1;
				}
			}
			else if ( player == HUMAN ){
				reveal_card_iid(player, card, deck[0]);
			}
		}
	}

	return 0;
}


// Cards
int card_ambassador_oak(int player, int card, event_t event){
	/* Ambassador Oak	|3|G
	 * Creature - Treefolk Warrior 3/3
	 * When ~ enters the battlefield, put a 1/1 |Sgreen Elf Warrior creature token onto the battlefield. */

	if( comes_into_play(player, card, event) ){
		generate_token_by_id(player, card, CARD_ID_ELF_WARRIOR);
	}

	return 0;
}

int card_aunties_snitch(int player, int card, event_t event){
	return prowl(player, card, event, SUBTYPE_GOBLIN, SUBTYPE_ROGUE, MANACOST_XB(1, 1));
}

int card_bullyrush_banneret(int player, int card, event_t event){
	return banneret(player, card, event, SUBTYPE_SOLDIER, SUBTYPE_KITHKIN);
}

int card_battletide_alchemist(int player, int card, event_t event){
	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card == -1 && damage->info_slot > 0 ){
				int choice = 0;
				int amount = count_subtype(player, TYPE_PERMANENT, SUBTYPE_CLERIC);
				if( damage->damage_target_player != player ){
					choice = 1;
				}
				if( ! duh_mode(player) ){
					char buffer[100];
					scnprintf(buffer, 500, " Prevent %d damages\n Pass", amount);
					choice = do_dialog(player, player, card, -1, -1, buffer, 0);
				}
				if( choice == 0 ){
					if( damage->info_slot < amount ){
						damage->info_slot = 0;
					}
					else{
						damage->info_slot-=amount;
					}
				}
			}
		}
	}
	return 0;
}

int card_bitterblossom(int player, int card, event_t event){
	/* Bitterblossom	|1|B
	 * Tribal Enchantment - Faerie
	 * At the beginning of your upkeep, you lose 1 life and put a 1/1 |Sblack Faerie Rogue creature token with flying onto the battlefield. */

	//  original code : 004DEA12

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		generate_token_by_id(player, card, CARD_ID_FAERIE_ROGUE);
		lose_life(player, 1);
	}

	return global_enchantment(player, card, event);
}


int card_blightsoil_druid(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_pay_life(player, 1) ){
		return mana_producer(player, card, event);
	}

	else if( event == EVENT_ACTIVATE ){
			lose_life(player, 1);
			return mana_producer(player, card, event);
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

int card_boldwyr_heavyweights(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		if( do_dialog(1-player, player, card, -1, -1, " Tutor a creature\n Pass", duh_mode(player)) == 0 ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			new_global_tutor(1-player, 1-player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	return 0;
}

int card_borderland_behemoth(int player, int card, event_t event){

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		int result = 0;
		int count = 0;
		while( count < active_cards_count[player] ){
				if( count != card && in_play(player, count) && is_what(player, count, TYPE_PERMANENT) && has_subtype(player, count, SUBTYPE_GIANT) ){
					result+=4;
				}
				count++;
		}
		event_result+=result;
	}

	return 0;
}

int card_bosk_banneret(int player, int card, event_t event){

	return banneret(player, card, event, SUBTYPE_TREEFOLK, SUBTYPE_SHAMAN);
}

int card_bramblewood_paragon(int player, int card, event_t event){

	if( event == EVENT_ABILITIES && player == affected_card_controller){
		if( count_1_1_counters(affected_card_controller, affected_card) > 0 ){
			event_result |= KEYWORD_TRAMPLE;
		}
	}

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( trigger_cause_controller, trigger_cause ) &&
		reason_for_trigger_controller == affected_card_controller && trigger_cause_controller == player && trigger_cause != card
	  ){
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE ) &&
			has_creature_type(trigger_cause_controller, trigger_cause, SUBTYPE_WARRIOR)
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					add_1_1_counter(trigger_cause_controller, trigger_cause);
			}
		}
	}

	return 0;
}

int card_brightheart_banneret(int player, int card, event_t event){
	banneret(player, card, event, SUBTYPE_ELEMENTAL, SUBTYPE_WARRIOR);
	return reinforce(player, card, event, 1, 1, 0, 0, 0, 1, 0);
}

int card_chameleon_colossus(int player, int card, event_t event){

  /* Chameleon Colossus	|2|G|G
   * Creature - Shapeshifter 4/4
   * Changeling
   * Protection from |Sblack
   * |2|G|G: ~ gets +X/+X until end of turn, where X is its power. */

  changeling_switcher(player, card, event);

  return  card_yew_spirit(player, card, event); // Avacyn Restored
}

int card_cloak_and_dagger(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, player, 1+player, TYPE_CREATURE, 0, SUBTYPE_ROGUE, 0, 0, 0, 0, 0, -1, 0) ){
		 equip_target_creature(player, card, instance->targets[1].player, instance->targets[1].card);
	}

	return vanilla_equipment(player, card, event, 3, 2, 0, KEYWORD_SHROUD, 0);
}

int card_countryside_crusher(int player, int card, event_t event)
{
  /* Countryside Crusher	|1|R|R
   * Creature - Giant Warrior 3/3
   * At the beginning of your upkeep, reveal the top card of your library. If it's a land card, put it into your graveyard and repeat this process.
   * Whenever a land card is put into your graveyard from anywhere, put a +1/+1 counter on ~. */

  upkeep_trigger_ability(player, card, event, player);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	while (deck_ptr[player][0] != -1)
	  {
		reveal_card_iid(player, card, deck_ptr[player][0]);
		if (is_what(-1, deck_ptr[player][0], TYPE_LAND))
		  mill(player, 1);
		else
		  break;
	  }

  // from library
  enable_xtrigger_flags |= ENABLE_XTRIGGER_MILLED;
  if (xtrigger_condition() == XTRIGGER_MILLED && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == player && !is_humiliated(player, card))
	{
	  if (event != EVENT_TRIGGER && event != EVENT_RESOLVE_TRIGGER)
		return 0;

	  int i, num_lands = 0;
	  for (i = 0; i < num_cards_milled; ++i)
		if (is_what(-1, cards_milled[i].internal_card_id, TYPE_LAND))
		  {
			if (event == EVENT_TRIGGER)
			  {
				event_result |= RESOLVE_TRIGGER_MANDATORY;
				return 0;
			  }
			if (event == EVENT_RESOLVE_TRIGGER)
			  ++num_lands;
		  }
	  if (num_lands)
		add_1_1_counters(player, card, num_lands);
	}

  enable_xtrigger_flags |= ENABLE_XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY;
  if (xtrigger_condition() == XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_LAND, "");
	  if (when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(player, card, event, player, RESOLVE_TRIGGER_MANDATORY, &test))
		add_1_1_counter(player, card);
	}

  return 0;
}

int card_cream_of_the_crop(int player, int card, event_t event){

   if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( player, card ) &&
	   reason_for_trigger_controller == affected_card_controller && ! check_battlefield_for_id(2, CARD_ID_TORPOR_ORB)
	 ){
	   int trig = 0;

		if( trigger_cause_controller == player && is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE) &&
		   get_power(trigger_cause_controller, trigger_cause) > 0
		  ){
			trig = 1;
		}

		if( trig > 0 ){
		  if(event == EVENT_TRIGGER){
			 event_result |= 1+player;
		  }
		  else if(event == EVENT_RESOLVE_TRIGGER){
				  impulse_effect(player, get_power(trigger_cause_controller, trigger_cause), 1);
		  }
	   }
	}

	return global_enchantment(player, card, event);
}

int card_declaration_of_naught(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( player != AI ){
			instance->targets[0].card = card_from_list(player, 3, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
		else{
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.type_flag = 1;
			instance->targets[0].card = new_global_tutor(player, 1-player, TUTOR_FROM_DECK, TUTOR_GET_ID, 0, AI_MAX_VALUE, &this_test);
		}
		create_card_name_legacy(player, card, instance->targets[0].card);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0) ){
		int result = card_spiketail_hatchling(player, card, event);
		if( result > 0 ){
			if( get_id(card_on_stack_controller, card_on_stack) == instance->targets[0].card ){
				return result;
			}
		}
	}

	if(event == EVENT_ACTIVATE  ){
		charge_mana_for_activated_ability(player, card, 0, 0, 1, 0, 0, 0);
		if( spell_fizzled != 1 ){
			instance->targets[1].player = card_on_stack_controller;
			instance->targets[1].card = card_on_stack;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		set_flags_when_spell_is_countered(player, instance->parent_card, instance->targets[1].player, instance->targets[1].card);
		kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
	}

	return global_enchantment(player, card, event);
}

int card_deglamer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			card_instance_t *instance = get_card_instance(player, card);
			shuffle_into_library(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ARTIFACT_ENCHANTMENT", 1, NULL);
}

int card_disperse(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.illegal_type = TYPE_LAND;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_PERMANENT");
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_distant_melody(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL){
		int selected = select_a_subtype(player, card);
		draw_cards(player, count_subtype(player, TYPE_PERMANENT, selected));
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_diviners_wand(int player, int card, event_t event){

	/* Diviner's Wand	|3
	 * Tribal Artifact - Wizard Equipment
	 * Equipped creature has "Whenever you draw a card, this creature gets +1/+1 and gains flying until end of turn" and "|4: Draw a card."
	 * Whenever a Wizard creature enters the battlefield, you may attach ~ to it.
	 * Equip |3 */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( is_equipping(player, card) && has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 3);
	}
	if( event == EVENT_ACTIVATE ){
			int choice = 0;
			if( can_activate_basic_equipment(player, card, event, 3) ){
				if( is_equipping(player, card) && has_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0) ){
					choice = do_dialog(player, player, card, -1, -1, " Change equipped creature\n Draw a card\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}

			if( choice == 0 ){
				activate_basic_equipment(player, card, 3);
				instance->info_slot = 66+choice;
			}
			else if( choice == 1 ){
					charge_mana_for_activated_ability(player, card, 4, 0, 0, 0, 0, 0);
					if( spell_fizzled != 1){
						instance->info_slot = 66+choice;
					}
			}
			else{
				spell_fizzled = 1;
			}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 ){
				resolve_activation_basic_equipment(player, card);
			}
			if( instance->info_slot == 67 ){
				draw_cards(player, 1);
			}
	}

	if (is_equipping(player, card) && card_drawn_trigger(player, card, event, player, RESOLVE_TRIGGER_MANDATORY)){
		pump_ability_until_eot(player, card, instance->targets[8].player, instance->targets[8].card, 1, 1, KEYWORD_FLYING, 0);
	}

	if( specific_cip(player, card, event, player, 1+player, TYPE_CREATURE, 0, SUBTYPE_WIZARD, 0, 0, 0, 0, 0, -1, 0) ){
		 equip_target_creature(player, card, instance->targets[1].player, instance->targets[1].card);
	}

	return 0;
}

int card_door_of_destinies(int player, int card, event_t event){

	/* Door of Destinies	|4
	 * Artifact
	 * As ~ enters the battlefield, choose a creature type.
	 * Whenever you cast a spell of the chosen type, put a charge counter on ~.
	 * Creatures you control of the chosen type get +1/+1 for each charge counter on ~. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL){
		instance->targets[0].player = select_a_subtype(player, card);
	}

	if( instance->targets[0].player >= 0 && specific_spell_played(player, card, event, player, 2, TYPE_ANY, 0, instance->targets[0].player, 0, 0, 0, 0, 0, -1, 0) ){
		add_counter(player, card, COUNTER_CHARGE);
	}

	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affected_card_controller == player ){
		if( has_subtype(affected_card_controller, affected_card, instance->targets[0].player) ){
			event_result += count_counters(player, card, COUNTER_CHARGE);
		}
	}

	return 0;
}

int card_earwig_squad(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && instance->info_slot == 1 ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		this_test.qty = 3;
		new_global_tutor(player, 1-player, TUTOR_FROM_DECK, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
		shuffle(1-player);
	}

	return prowl(player, card, event, SUBTYPE_GOBLIN, SUBTYPE_ROGUE, MANACOST_XB(2, 1));
}

int card_fendeep_summoner(int player, int card, event_t event){//UNUSEDCARD

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = SUBTYPE_SWAMP;
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0)
	  ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int total = 0;
			while( total < 2 && can_target(&td) ){
					if( select_target(player, card, &td, "Select target Swamp.", &(instance->targets[total])) ){
						if( has_subtype(instance->targets[total].player, instance->targets[total].card, SUBTYPE_SWAMP) ){
							state_untargettable(instance->targets[total].player, instance->targets[total].card, 1);
							total++;
						}
						else{
							break;
						}
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<total; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			if( total > 0 ){
				tap_card(player, card);
				instance->info_slot = total;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			if( validate_target(player, card, &td, i) ){
				add_a_subtype(instance->targets[i].player, instance->targets[i].card, SUBTYPE_TREEFOLK);
				land_animation2(player, instance->parent_card, instance->targets[i].player, instance->targets[i].card, 1, 3, 5, 0, 0, 0, 0);
			}
		}
	}

	return 0;
}

int card_fertilid(int player, int card, event_t event){

	enters_the_battlefield_with_counters(player, card, event, COUNTER_P1_P1, 2);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
			this_test.subtype = SUBTYPE_BASIC;
			new_global_tutor(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_DECK, TUTOR_PLAY_TAPPED, 0, AI_FIRST_FOUND, &this_test);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XG(1, 1), GVC_COUNTER(COUNTER_P1_P1), &td, "TARGET_PLAYER");
}

int card_festercreep(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		add_1_1_counter(player, card);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, card, 2, -1, -1, -1, 0, 0);
	}

	return generic_activated_ability(player, card, event, GAA_1_1_COUNTER, 1, 1, 0, 0, 0, 0, 0, 0, 0);
}

// giant warrior token --> rhino token

int card_feudkiller_verdict(int player, int card, event_t event){
	/* Feudkiller's Verdict	|4|W|W
	 * Tribal Sorcery - Giant
	 * You gain 10 life. Then if you have more life than an opponent, put a 5/5 |Swhite Giant Warrior creature token onto the battlefield. */

	if( event == EVENT_RESOLVE_SPELL){
		gain_life(player, 10);
		if( life[player] > life[1-player] ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_GIANT_WARRIOR, &token);
			token.pow = token.tou = 5;
			token.color_forced = COLOR_TEST_WHITE;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int forfend_effect(int player, int card, event_t event){

	if( event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);
		if( damage_card == source->internal_card_id && source->info_slot > 0 ){
			if( is_what(source->damage_target_player, source->damage_target_card, TYPE_CREATURE) ){
				source->info_slot = 0;
			}
		}
	}
	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_forfend(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL){
		create_legacy_effect(player, card, &forfend_effect);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_frogtosser_banneret(int player, int card, event_t event){
	haste(player, card, event);
	return banneret(player, card, event, SUBTYPE_GOBLIN, SUBTYPE_ROGUE);
}

// game trail changeling --> vanilla

int card_gilt_leaf_archdruid(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, RESOLVE_TRIGGER_CHECK_DECK_COUNT(player, 1), TYPE_ANY, 0, SUBTYPE_DRUID, 0, 0, 0, 0, 0, -1, 0) ){
		draw_cards(player, 1);
	}

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_DRUID;
	td1.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return target_available(player, card, &td1) > 6;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST0) && tapsubtype_ability(player, card, 7, &td1) ){
			instance->number_of_targets = 0;
			pick_target(&td, "TARGET_PLAYER");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			new_manipulate_all(player, card, instance->targets[0].player, &this_test, ACT_GAIN_CONTROL);
		}
	}

	return 0;
}

static int graceful_reprieve_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(p, c) ){
			card_instance_t *affected = get_card_instance(p, c);
			if( affected->kill_code > 0 && affected->kill_code < 4){
				instance->targets[11].player = get_original_id(p, c);
			}
		}
	}

	if( instance->targets[11].player > -1 && resolve_graveyard_trigger(player, card, event) == 1 ){
		seek_grave_for_id_to_reanimate(player, card, player, instance->targets[11].player, REANIMATE_DEFAULT);
		kill_card(player, card, KILL_REMOVE);
	}

	if( ! in_play(p, c) && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_graceful_reprieve(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		if( player == AI ){
			return card_death_ward(player, card, event);
		}
		else{
			return can_target(&td);
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( player == AI ){
			return card_death_ward(player, card, event);
		}
		else{
			pick_target(&td, "TARGET_PERMANENT");
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			int legacy = create_legacy_effect(player, card, &graceful_reprieve_legacy);
			card_instance_t *leg = get_card_instance( player, legacy );
			leg->targets[0] = instance->targets[0];
			leg->number_of_targets = 1;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_greatbow_doyen(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	boost_creature_type(player, card, event, SUBTYPE_ARCHER, 1, 1, 0, BCT_CONTROLLER_ONLY);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_source_player == player ){
				if( has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_ARCHER) ){
					int good = damage->info_slot;
					if( good < 1 ){
						card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
						if( trg->targets[16].player > 0 ){
							good = trg->targets[16].player;
						}
					}

					if( good > 0 ){
						if( instance->targets[1].player < 2 ){
							instance->targets[1].player = 2;
						}
						int pos = instance->targets[1].player;
						if( pos < 10 ){
							instance->targets[pos].player = damage->damage_target_player;
							instance->targets[pos].card = good;
							instance->targets[pos+1].card = damage->damage_source_card;
							instance->targets[1].player+=2;
						}
					}
				}
			}
		}
	}

	if( instance->targets[1].player > 2 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player
	  ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=2; i<instance->targets[1].player; i++){
					damage_player(instance->targets[i].player, instance->targets[i].card, player, instance->targets[i+1].card);
				}
				instance->targets[1].player = 2;
		}
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 2;
	}

	return 0;
}

int card_grimoire_thief(int player, int card, event_t event){
	/*
	  Grimoire Thief |U|U
	  Creature - Merfolk Rogue 2/2
	  Whenever Grimoire Thief becomes tapped, exile the top three cards of target opponent's library face down.
	  You may look at cards exiled with Grimoire Thief.
	  {U}, Sacrifice Grimoire Thief: Turn all cards exiled with Grimoire Thief face up. Counter all spells with those names.
	*/
	if (event == EVENT_TAP_CARD && affect_me(player, card) ){
		target_definition_t td_player;
		default_target_definition(player, card, &td_player, 0);
		td_player.zone = TARGET_ZONE_PLAYERS;

		if( would_validate_arbitrary_target(&td_player, 1-player, -1) ){
			int i, leg = 0, idx = 0;
			for (i = 0; i < 3; ++i){
				if (deck_ptr[1-player][0] != -1){
					exiledby_remember(player, card, 1-player, deck_ptr[1-player][0], &leg, &idx);
					rfg_top_card_of_deck(1-player);
				}
			}
		}
	}

#define ABILITY()	generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK|GAA_SACRIFICE_ME, MANACOST_U(1), 0, NULL, NULL)
#define FIND_IN_RFG(instance, spell)	exiledby_choose(player, card, cards_data[spell->internal_card_id].id, EXBY_FIRST_FOUND|EXBY_TEST_IID, instance->internal_card_id, NULL, 1)

	if (event == EVENT_CAN_ACTIVATE ){
		if( ABILITY()){
			card_instance_t* instance = get_card_instance(player, card);
			card_instance_t* spell = get_card_instance(card_on_stack_controller, card_on_stack);
			if (FIND_IN_RFG(instance, spell)){
				return 99;
			}
		}
		else if( player == HUMAN && exiledby_count(player, card, 1-player) ){
				return 1;
		}
	}

	if (event == EVENT_ACTIVATE){
		card_instance_t* instance = get_card_instance(player, card);
		card_instance_t* spell = get_card_instance(card_on_stack_controller, card_on_stack);
		if (FIND_IN_RFG(instance, spell)){
			ABILITY();
		}
		else{
			if( player == AI ){
				cancel = 1;
			}
			else{
				exiledby_choose(player, card, CARD_ID_GRIMOIRE_THIEF, EXBY_CHOOSE | EXBY_MAX_VALUE, 0, "any", 1);
				cancel = 1;
			}
		}
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

int card_heritage_druid(int player, int card, event_t event){
	// See also card_birchlore_rangers() in onslaught.c.

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.required_subtype = SUBTYPE_ELF;
	td.illegal_state = TARGET_STATE_TAPPED;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) && target_available(player, card, &td) > 2 ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		if( tapsubtype_ability(player, card, 3, &td) ){
			instance->number_of_targets = 3;
			// See comments in tap_a_permanent_you_control_for_mana(), which ideally should be generalized to handle this card.
			produce_mana(player, COLOR_GREEN, 3);
			tapped_for_mana_color = -2;
			/* EVENT_TAP_CARD already sent for all three elves.  It'll get sent twice for this if this happens to be one of the ones chosen; but it's not worth
			 * the effort to fix. */
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card) ){
		int count = target_available(player, card, &td);
		if (count > 2){
			declare_mana_available(player, COLOR_GREEN, 3 * (count / 3));
		}
	}

	if (event == EVENT_ATTACK_RATING && affect_me(player, card)){
		ai_defensive_modifier += 48;
	}
	if (event == EVENT_BLOCK_RATING && affect_me(player, card)){
		ai_defensive_modifier -= 48;
	}

	return 0;
}

int card_hunting_triad(int player, int card, event_t event){
	/* Hunting Triad	|3|G
	 * Tribal Sorcery - Elf
	 * Put three 1/1 |Sgreen Elf Warrior creature tokens onto the battlefield.
	 * Reinforce 3-|3|G */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_ELF_WARRIOR, 3);
		kill_card(player, card, KILL_DESTROY);
	}

	return reinforce(player, card, event, 3, 3, 0, 0, 1, 0, 0);
}

int card_idyllic_tutor(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, 1, TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_ink_dissolver(int player, int card, event_t event){
	if (kinship(player, card, event)){
		mill(1-player, 3);
	}
	return 0;
}

int card_inspired_sprite(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, SUBTYPE_WIZARD, 0, 0, 0, 0, 0, -1, 0) ){
		untap_card(player, card);
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card)  ){
		return has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			tap_card(player, card);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
		discard(player, 0, player);
	}

	return flash(player, card, event);
}

static int kinsbaille_borderguard_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(p, c) ){
			card_instance_t *affected = get_card_instance(p, c);
			if( affected->kill_code > 0 && affected->kill_code < 4){
				instance->targets[11].player = count_1_1_counters(p, c);
			}
		}
	}

	if( instance->targets[11].player > 0 && resolve_graveyard_trigger(player, card, event) == 1 ){
		generate_tokens_by_id(player, card, CARD_ID_KITHKIN_SOLDIER, instance->targets[11].player);
		kill_card(player, card, KILL_REMOVE);
	}

	if( ! in_play(p, c) && eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_kinsbaile_borderguard(int player, int card, event_t event){
	/* Kinsbaile Borderguard	|1|W|W
	 * Creature - Kithkin Soldier 1/1
	 * ~ enters the battlefield with a +1/+1 counter on it for each other Kithkin you control.
	 * When ~ dies, put a 1/1 |Swhite Kithkin Soldier creature token onto the battlefield for each counter on it. */

	if( event == EVENT_RESOLVE_SPELL ){
		add_1_1_counters(player, card, count_subtype(player, TYPE_PERMANENT, SUBTYPE_KITHKIN)-1);
		int legacy = create_legacy_effect(player, card, &kinsbaille_borderguard_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[0].card = card;
		leg->targets[0].player = player;
		leg->number_of_targets = 1;
		add_status(player, legacy, STATUS_INVISIBLE_FX);
	}

	return 0;
}

int card_kinsbaile_cavalier(int player, int card, event_t event){

	/* Kinsbaile Cavalier	|3|W
	 * Creature - Kithkin Knight 2/2
	 * Knight creatures you control have double strike. */

	boost_subtype(player, card, event, SUBTYPE_KNIGHT, 0,0, KEYWORD_DOUBLE_STRIKE,0, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

	return 0;
}

int card_kithkin_zephyrnaut(int player, int card, event_t event){
	if (kinship(player, card, event)){
		pump_ability_until_eot(player, card, player, card, 2, 2, KEYWORD_FLYING, SP_KEYWORD_VIGILANCE);
	}
	return 0;
}

int card_knowledge_exploitation(int player, int card, event_t event){

	/* Knowledge Exploitation	|5|U|U
	 * Tribal Sorcery - Rogue
	 * Prowl |3|U
	 * Search target opponent's library for an instant or sorcery card. You may cast that card without paying its mana cost. Then that player shuffles his or
	 * her library. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	cmc_mod_for_prowl(player, card, event, SUBTYPE_ROGUE, -1, MANACOST_XU(3, 1));

	if( event == EVENT_CAN_CAST ){
		return opponent_is_valid_target(player, card);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pay_prowl(player, card, event, MANACOST_XU(3, 1));
		if( spell_fizzled != 1 ){
			target_opponent(player, card);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_SPELL, "Select an instant or sorcery card to play for free.");
			int selected = new_select_a_card(player, 1-player, TUTOR_FROM_DECK, 0, AI_MAX_CMC, -1, &this_test);
			if( selected != -1 && can_legally_play_iid(player, deck_ptr[1-player][selected]) ){
				cant_be_responded_to = 1;	// before playing the card - once this effect starts resolving, it's too late to respod to whatever card gets chosen
				play_card_in_deck_for_free(player, 1-player, selected);
			}
			shuffle(1-player);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_latchkey_faerie(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( comes_into_play(player, card, event) && instance->info_slot == 1 ){
		draw_cards(player, 1);
	}

	return prowl(player, card, event, SUBTYPE_FAERIE, SUBTYPE_ROGUE, MANACOST_XU(2, 1));
}


int card_leafcrown_elder(int player, int card, event_t event){

	if (kinship(player, card, event)){
		int *deck = deck_ptr[player];
		card_ptr_t* c = cards_ptr[ cards_data[deck[0]].id ];
		char buffer[100];
		snprintf(buffer, 100, " Play %s\n Pass", c->name );
		int choice = do_dialog(player, player, card, -1, -1, buffer, 0);
		if( choice == 0 ){
			play_card_in_deck_for_free(player, player, 0);
		}
	}

	return 0;
}

int card_lightning_crafter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	champion(player, card, event, SUBTYPE_GOBLIN, SUBTYPE_SHAMAN);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 3);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0,
													&td, "TARGET_CREATURE_OR_PLAYER");
}

int card_lys_alana_bowmaster(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance( player, card );

	if( can_target(&td) && specific_spell_played(player, card, event, player, 1+player, TYPE_ANY, 0, SUBTYPE_ELF, 0, 0, 0, 0, 0, -1, 0) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, card);
		}
	}

	return 0;
}

int card_maralen_of_the_mornsong(int player, int card, event_t event){

	/* Maralen of the Mornsong	|1|B|B
	 * Legendary Creature - Elf Wizard 2/3
	 * Players can't draw cards.
	 * At the beginning of each player's draw step, that player loses 3 life, searches his or her library for a card, puts it into his or her hand, then
	 * shuffles his or her library. */

	check_legend_rule(player, card, event);

	if( trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && event == EVENT_TRIGGER ){
		suppress_draw = 1;	// Deliberately do this during EVENT_TRIGGER, not EVENT_RESOLVE_TRIGGER, so nothing else can replace the (nonexistent) draw
	}

	if( event == EVENT_DRAW_PHASE ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		new_global_tutor(current_turn, current_turn, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_CMC, &this_test);
		lose_life(current_turn, 3);
	}

	return 0;
}

int card_meadowboon(int player, int card, event_t event)
{
  /* Meadowboon	|2|W|W
   * Creature - Elemental 3/3
   * When ~ leaves the battlefield, put a +1/+1 counter on each creature target player controls.
   * Evoke |3|W */

  if (comes_into_play(player, card, event) && evoked(player, card))
	kill_card(player, card, KILL_SACRIFICE);

  if (leaves_play(player, card, event))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, 0);
	  td.zone = TARGET_ZONE_PLAYERS;
	  td.allow_cancel = 0;

	  card_instance_t* instance = get_card_instance(player, card);
	  instance->number_of_targets = 0;
	  if (can_target(&td) && pick_target(&td, "TARGET_PLAYER"))
		manipulate_type(player, card, instance->targets[0].player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}

  return evoke(player, card, event, MANACOST_XW(3,1));
}

int card_merrow_witsniper(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;

	card_instance_t *instance= get_card_instance(player, card);

	if( comes_into_play(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			mill(instance->targets[0].player, 1);
		}
	}

	return 0;
}

// mind shatter --> mind twist

int card_moonglove_changeling(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
}

int card_morsel_theft(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	cmc_mod_for_prowl(player, card, event, SUBTYPE_ROGUE, -1, MANACOST_XB(1, 1));

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pay_prowl(player, card, event, MANACOST_XB(1, 1));
		if( spell_fizzled != 1 ){
			pick_target(&td, "TARGET_PLAYER");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			lose_life(instance->targets[0].player, 3);
			gain_life(player, 3);
			if( instance->info_slot == 1 ){
				draw_cards(player, 1);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_mosquito_guard(int player, int card, event_t event){
	return reinforce(player, card, event, 1, 1, 0, 0, 0, 0, 1);
}

int card_mothdust_changeling(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.illegal_state = TARGET_STATE_TAPPED;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) && pick_target(&td, "TARGET_CREATURE") ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, KEYWORD_FLYING, 0);
	}

	return 0;
}

int card_murmuring_bosk(int player, int card, event_t event){

	if( event == EVENT_ACTIVATE ){
		int black_mana_before = mana_pool[COLOR_BLACK+8*player];
		int white_mana_before = mana_pool[COLOR_WHITE+8*player];
		mana_producer(player, card, event);
		if( black_mana_before < mana_pool[COLOR_BLACK+8*player] || white_mana_before < mana_pool[COLOR_WHITE+8*player]){
			damage_player(player, 1, player, card);
		}
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) && !is_tapped(player, card) ){	// e.g. from Kismet
			int tapme = 1;
			if( is_subtype_in_hand(player, SUBTYPE_TREEFOLK) ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select Treefolk card to reveal.");
				this_test.subtype = SUBTYPE_TREEFOLK;
				int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
				if (selected != -1){
					reveal_card(player, card, player, selected);
					tapme = 0;
				}
			}
			if( tapme ){
				add_state(player, card, STATE_TAPPED);
			}
	}
	else{
		return mana_producer(player, card, event);
	}
	return 0;
}

int card_mutavault(int player, int card, event_t event){
	return manland_normal(player, card, event, 1, 0, 0, 0, 0, 0);
}

int card_mutavault_animated(int player, int card, event_t event){
	changeling_switcher(player, card, event);
	return manland_animated(player, card, event, 1, 0, 0, 0, 0, 0);
}

int card_negate(int player, int card, event_t event){
	card_instance_t *instance= get_card_instance(player, card);
	if(event == EVENT_CAN_CAST ){
		int result =  card_counterspell(player, card, event);
		if( result > 0 && ! is_what(card_on_stack_controller, card_on_stack, TYPE_CREATURE) ){
			return result;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			return card_counterspell(player, card, event);
	}
	else{
		return card_counterspell(player, card, event);
	}
	return 0;
}

static int nevermaker_legacy(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.illegal_type = TYPE_LAND;
	td.allow_cancel = 0;

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( leaves_play(p, c, event) && can_target(&td) ){
		if( new_pick_target(&td, "TARGET_PERMANENT", 1, 0) ){
			put_on_top_of_deck(instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_nevermaker(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		create_my_legacy(player, card, &nevermaker_legacy);
	}

	if (comes_into_play(player, card, event) && evoked(player, card)){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return evoke(player, card, event, 3, 0, 1, 0, 0, 0);
}

int card_noggin_whack(int player, int card, event_t event){

	/* Noggin Whack	|2|B|B
	 * Tribal Sorcery - Rogue
	 * Prowl |1|B
	 * Target player reveals three cards from his or her hand. You choose two of them. That player discards those cards. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	cmc_mod_for_prowl(player, card, event, SUBTYPE_ROGUE, -1, MANACOST_XB(1, 1));

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pay_prowl(player, card, event, MANACOST_XB(1, 1));
		if( spell_fizzled != 1 ){
			pick_target(&td, "TARGET_PLAYER");
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, 0);

			ec_definition_t this_definition;
			default_ec_definition(instance->targets[0].player, player, &this_definition);
			this_definition.cards_to_reveal = 3;
			this_definition.qty = 2;
			new_effect_coercion(&this_definition, &this_test);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}


int card_notorious_throng(int player, int card, event_t event){
	/* Notorious Throng	|3|U
	 * Tribal Sorcery - Rogue
	 * Prowl |5|U
	 * Put X 1/1 |Sblack Faerie Rogue creature tokens with flying onto the battlefield, where X is the damage dealt to your opponents this turn. If ~'s prowl cost was paid, take an extra turn after this one. */

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_tokens_by_id(player, card, CARD_ID_FAERIE_ROGUE, get_trap_condition(1-player, TRAP_DAMAGE_TAKEN));
		if( instance->info_slot == 1 ){
			return card_time_walk(player, card, event);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return prowl(player, card, event, SUBTYPE_ROGUE, -1, MANACOST_XU(5, 1));
}

int card_obsidian_battle_axe(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( specific_cip(player, card, event, player, 1+player, TYPE_CREATURE, 0, SUBTYPE_WARRIOR, 0, 0, 0, 0, 0, -1, 0) ){
		 equip_target_creature(player, card, instance->targets[1].player, instance->targets[1].card);
	}

	return vanilla_equipment(player, card, event, 3, 2, 1, 0, SP_KEYWORD_HASTE);
}

static int offalsnout_legacy(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.allow_cancel = 0;
	td.illegal_abilities = 0;

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( leaves_play(p, c, event) && ! graveyard_has_shroud(2) ){
		instance->targets[1].player = 1-player;
		if( count_graveyard(1-player) > 0  ){
			if( count_graveyard(player) > 0 && player != AI ){
				new_pick_target(&td, "TARGET_PLAYER", 1, 0);
			}
		}
		else{
			if( count_graveyard(player) > 0 && player != AI ){
				instance->targets[1].player = player;
			}
			else{
				instance->targets[1].player = -1;
			}
		}
		if( instance->targets[1].player != -1 ){
			char msg[100] = "Select a card to exile.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_ANY, msg);
			new_global_tutor(player, instance->targets[1].player, TUTOR_FROM_GRAVE, TUTOR_RFG, 1, AI_MAX_VALUE, &this_test);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_offalsnout(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		create_my_legacy(player, card, &offalsnout_legacy);
	}

	if (comes_into_play(player, card, event) && evoked(player, card)){
		kill_card(player, card, KILL_SACRIFICE);
	}

	evoke(player, card, event, 0, 1, 0, 0, 0, 0);

	return flash(player, card, event);
}

int card_oonas_blackguard(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( trigger_cause_controller, trigger_cause ) &&
		reason_for_trigger_controller == affected_card_controller && trigger_cause_controller == player && trigger_cause != card
	  ){
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE ) &&
			has_creature_type(trigger_cause_controller, trigger_cause, SUBTYPE_ROGUE)
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					add_1_1_counter(trigger_cause_controller, trigger_cause);
			}
		}
	}

	damage_effects(player, card, event);

		if( instance->targets[1].player > 0  && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
			reason_for_trigger_controller == player ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					new_multidiscard(1-player, instance->targets[1].player, 0, player);
					instance->targets[1].player = 0;
			}
		}

	return 0;
}

int card_orchard_warden(int player, int card, event_t event){

	/* Orchard Warden	|4|G|G
	 * Creature - Treefolk Shaman 4/6
	 * Whenever another Treefolk creature enters the battlefield under your control, you may gain life equal to that creature's toughness. */

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.subtype = SUBTYPE_TREEFOLK;
	this_test.not_me = 1;

	if( new_specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), &this_test) ){
		card_instance_t *instance = get_card_instance(player, card);
		gain_life(player, get_toughness(instance->targets[1].player, instance->targets[1].card));
	}

	return 0;
}

int card_preeminent_captain(int player, int card, event_t event)
{
  // Whenever ~ attacks, you may put a Soldier creature card from your hand onto the battlefield tapped and attacking.
  if (declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_AI(player), player, card))
	{
	  test_definition_t this_test;
	  new_default_test_definition(&this_test, TYPE_CREATURE, "Select a Soldier card to put into play attacking.");
	  this_test.subtype = SUBTYPE_SOLDIER;
	  new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY_ATTACKING, 0, AI_MAX_CMC, &this_test);
	}

  return 0;
}

int card_prickly_boggart( int player, int card, event_t event){
	fear(player, card, event);
	return 0;
}

int card_pyroclast_consul( int player, int card, event_t event){
	if (kinship(player, card, event)){
		new_damage_all(player, card, ANYBODY, 2, 0, NULL);
	}
	return 0;
}

int card_rage_forger(int player, int card, event_t event)
{
  // When ~ enters the battlefield, put a +1/+1 counter on each other Shaman creature you control.
  if (comes_into_play(player, card, event))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_SHAMAN;
	  test.not_me = 1;
	  new_manipulate_all(player, card, player, &test, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));
	}

  // Whenever a creature you control with a +1/+1 counter on it attacks, you may have that creature deal 1 damage to target player.
  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.power = 0;
	  test.power_flag = F5_POWER_1_1_COUNTERS_GREATER_THAN_VALUE;

	  int amt;
	  if ((amt = declare_attackers_trigger_test(player, card, event, RESOLVE_TRIGGER_AI(player) | DAT_TRACK, player, -1, &test)))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		  for (--amt; amt >= 0; --amt)
			if (in_play(current_turn, attackers[amt]))
			  {
				target_definition_t td;
				default_target_definition(current_turn, attackers[amt], &td, 0);
				td.zone = TARGET_ZONE_PLAYERS;

				instance->number_of_targets = 0;
				if (can_target(&td) && pick_next_target_arbitrary(&td, "TARGET_PLAYER", player, card))
				  damage_creature(instance->targets[0].player, instance->targets[0].card, 1, current_turn, attackers[amt]);
			  }
		}
	}

  return 0;
}

int card_reach_of_branches(int player, int card, event_t event){

	/* Reach of Branches	|4|G
	 * Tribal Instant - Treefolk
	 * Put a 2/5 |Sgreen Treefolk Shaman creature token onto the battlefield.
	 * Whenever |Ha Forest enters the battlefield under your control, you may return ~ from your graveyard to your hand. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		generate_token_by_id(player, card, CARD_ID_TREEFOLK_SHAMAN);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_redeem_the_lost(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int keyword = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, keyword, 0);
			}
			if( clash(player, card) ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}

	return 0;
}

int card_release_the_ants(int player, int card, event_t event){

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
			if( valid_target(&td) ){
				damage_creature_or_player(player, card, event, 1);
			}
			if( clash(player, card) ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}

	return 0;
}

int card_research_the_deep(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			draw_cards(player, 1);
			if( clash(player, card) ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}

	return 0;
}

int card_reveillark(int player, int card, event_t event){

	if( leaves_play(player, card, event) == 1  ){
		int i;
		for(i=0; i<2; i++){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_CREATURE);
			this_test.power = 3;
			this_test.power_flag = 3;
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		}
	}

	if (comes_into_play(player, card, event) && evoked(player, card)){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return evoke(player, card, event, 5, 0, 0, 0, 0, 1);
}

int card_revive_the_fallen(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;
	td.illegal_abilities = 0;

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && (count_graveyard_by_type(player, TYPE_CREATURE) > 0 || count_graveyard_by_type(1-player, TYPE_CREATURE)) ){
		return ! graveyard_has_shroud(2);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[0].player = 1-player;
		if( count_graveyard_by_type(1-player, TYPE_CREATURE) > 0 ){
			if( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
				new_pick_target(&td, "TARGET_PLAYER", 0, 1);
			}
		}
		else{
			instance->targets[0].player = player;
		}

		int selected = new_select_target_from_grave(player, card, instance->targets[0].player, 0, AI_MAX_CMC, &this_test, 1);
		if( selected == -1 ){
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			int selected = validate_target_from_grave(player, card, instance->targets[0].player, 1);
			if( selected != -1){
				from_grave_to_hand(instance->targets[0].player, selected, TUTOR_HAND);
			}
			if( clash(player, card) ){
				bounce_permanent(player, card);
			}
			else{
				kill_card(player, card, KILL_DESTROY);
			}
	}

	return 0;
}

int card_rhys_the_exiled(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	// Whenever ~ attacks, you gain 1 life for each Elf you control.
	if (declare_attackers_trigger(player, card, event, 0, player, card)){
		gain_life(player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_ELF));
	}

	// |B, Sacrifice an Elf: Regenerate ~.
	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST_B(1), 0, NULL, NULL) ){
			if( can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_ELF, 0, 0, 0, 0, 0, -1, 0) ){
				return 99;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_B(1)) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_PERMANENT);
			this_test.subtype = SUBTYPE_ELF;
			int result = new_sacrifice(player, card, player, SAC_JUST_MARK | SAC_RETURN_CHOICE, &this_test);
			if( result ){
				int s_player = BYTE2(result);
				int s_card = BYTE3(result);
				if( s_player == player && s_card == card ){
					state_untargettable(s_player, s_card, 0);
					spell_fizzled = 1;
					return 0;
				}
				kill_card(s_player, s_card, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance( player, card);
		if( can_regenerate(instance->parent_controller, instance->parent_card) ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_rustic_clachan(int player, int card, event_t event){

	/* Rustic Clachan	""
	 * Land
	 * As ~ enters the battlefield, you may reveal a Kithkin card from your hand. If you don't, ~ enters the battlefield tapped.
	 * |T: Add |W to your mana pool.
	 * Reinforce 1-|1|W */

	lorwyn_need_subtype_land(player, card, event, SUBTYPE_KITHKIN);

	if (!IS_ACTIVATING_FROM_HAND(event)){
		if (in_play(player, card)){
			return mana_producer(player, card, event);
		} else {
			return 0;
		}
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE_FROM_HAND  ){
		if( has_mana_multi(player, 1, 0, 0, 0, 0, 1) ){
			return can_target(&td);
		}
	}
	if( event == EVENT_ACTIVATE_FROM_HAND ){
			charge_mana_multi(player, 1, 0, 0, 0, 0, 1);
			if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
				discard_card(player, card);
			}
	}
	if( event == EVENT_RESOLVE_ACTIVATION_FROM_HAND && spell_fizzled != 1 ){
		add_1_1_counters(instance->targets[0].player, instance->targets[0].card, 1);
	}

	return 0;
}

int card_sage_of_fables(int player, int card, event_t event){

	/* Sage of Fables	|2|U
	 * Creature - Merfolk Wizard 2/2
	 * Each other Wizard creature you control enters the battlefield with an additional +1/+1 counter on it.
	 * |2, Remove a +1/+1 counter from a creature you control: Draw a card. */

	target_definition_t td;
	base_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.special = TARGET_SPECIAL_REQUIRES_COUNTER;
	td.extra = COUNTER_P1_P1;

	card_instance_t *instance = get_card_instance(player, card);

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me( trigger_cause_controller, trigger_cause ) &&
		reason_for_trigger_controller == affected_card_controller && trigger_cause_controller == player && trigger_cause != card
	  ){
		if( is_what(trigger_cause_controller, trigger_cause, TYPE_CREATURE ) &&
			has_creature_type(trigger_cause_controller, trigger_cause, SUBTYPE_WIZARD)
		  ){
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					add_1_1_counter(trigger_cause_controller, trigger_cause);
			}
		}
	}

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td) ){
		return has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 2, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 && pick_target(&td, "TARGET_CREATURE") ){
			instance->number_of_targets = 1;
			if( count_1_1_counters(instance->targets[0].player, instance->targets[0].card) > 0 ){
				remove_1_1_counter(instance->targets[0].player, instance->targets[0].card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return 0;
}

int card_sages_dousing(int player, int card, event_t event){

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
			instance->number_of_targets = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if (counterspell_resolve_unless_pay_x(player, card, NULL, 0, 3) && check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_WIZARD)){
			draw_cards(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_scapeshift(int player, int card, event_t event){

	if(event == EVENT_CAN_CAST){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_LAND );
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.illegal_abilities = 0;

			card_instance_t *instance = get_card_instance(player, card);

			int total = 0;
			int stop = 0;

			while( can_target(&td) && stop != 1){
					if( pick_target(&td, "TARGET_LAND") ){
						instance->number_of_targets = 1;
						kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
						total++;
					}
					else{
						stop = 1;
					}
			}

			if( total > 0 ){
				tutor_lands(player, TUTOR_PLAY_TAPPED, total);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_scarblade_elite(int player, int card, event_t event){

	test_definition_t this_test;
	new_default_test_definition(&this_test, 0, "Select an Assassin card to exile.");
	this_test.subtype = SUBTYPE_ASSASSIN;

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( new_special_count_grave(player, &this_test) > 0 && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return can_target(&td);
		}
	}

	if(event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_GRAVE, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 && pick_target(&td, "TARGET_CREATURE") ){
				rfg_card_from_grave(player, selected);
				instance->number_of_targets = 1;
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_sensation_gorger(int player, int card, event_t event){
	if (kinship(player, card, event)){
		APNAP(p,{
				new_discard_all(p, player);
				draw_cards(p, 4);
				};
		);
	}
	return 0;
}

int card_shard_volley(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			pick_target(&td, "TARGET_CREATURE_OR_PLAYER");
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_shared_animosity(int player, int card, event_t event){

	if( ! is_humiliated(player, card) && (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS) ){
		if(	declare_attackers_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY, player, -1) ){
			int count = active_cards_count[player]-1;
			while( count > -1 ){
				if( in_play(player, count) && is_what(player, count, TYPE_CREATURE) && is_attacking(player, count) ){
					int k;
					int result = 0;
					for(k=0; k<active_cards_count[player]; k++){
						if( count != k && in_play(player, k) && is_what(player, k, TYPE_CREATURE) && is_attacking(player, k) &&
							shares_creature_subtype(player, count, player, k)
						  ){
							result++;
						}
					}
					if( result > 0 ){
						pump_until_eot(player, card, player, count, result, 0);
					}
				}
				count--;
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_shinewed(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT );

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		add_1_1_counters(player, card, 1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_1_1_COUNTER+GAA_CAN_TARGET, 1, 0, 0, 0, 0, 1, 0, &td, "TARGET_ENCHANTMENT");
}

int card_sigil_tracer(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_PERMANENT);
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.required_subtype = SUBTYPE_WIZARD;
	td1.preferred_controller = player;
	td1.allowed_controller = player;
	td1.illegal_abilities = 0;

	target_definition_t td2;
	counterspell_target_definition(player, card, &td2, TYPE_SPELL);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		int result = generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK, MANACOST_XU(1, 1), 0, &td2, NULL);
		if( result && target_available(player, card, &td1) > 1 ){
			return result;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_XU(1, 1)) ){
			if( tapsubtype_ability(player, card, 2, &td1) ){
				activate_twincast(player, card, event, &td2, NULL);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		activate_twincast(player, card, event, &td2, NULL);
	}

	return 0;
}

static int slithermuse_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( leaves_play(p, c, event) ){
		int amount = hand_count[1-player]-hand_count[player];
		if( amount > 0 ){
			draw_cards(player, amount);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_slithermuse(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		create_my_legacy(player, card, &slithermuse_legacy);
	}

	if (comes_into_play(player, card, event) && evoked(player, card)){
		kill_card(player, card, KILL_SACRIFICE);
	}

	evoke(player, card, event, 3, 0, 1, 0, 0, 0);

	return flash(player, card, event);
}

static int spitebellows_legacy(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	int p = instance->targets[0].player;
	int c = instance->targets[0].card;

	if( p > -1 && leaves_play(p, c, event) ){
		target_definition_t td1;
		default_target_definition(p, c, &td1, TYPE_CREATURE);
		td1.allow_cancel = 0;
		td1.special = TARGET_SPECIAL_NOT_ME;

		if(can_target(&td1) && new_pick_target(&td1, "Select another target creature.", 1, GS_LITERAL_PROMPT) ){
			card_instance_t *orig = get_card_instance(p, c);
			damage_creature(orig->targets[1].player, orig->targets[1].card, 6, p, c);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_spitebellows(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		create_my_legacy(player, card, &spitebellows_legacy);
	}

	if (comes_into_play(player, card, event) && evoked(player, card)){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return evoke(player, card, event, MANACOST_XR(1, 2));
}

int card_squeaking_pie_grubfellows(int player, int card, event_t event){
	if (kinship(player, card, event)){
		discard(1-player, 0, player);
	}
	return 0;
}

int card_stenchskipper(int player, int card, event_t event){
	if( eot_trigger(player, card, event) && ! check_battlefield_for_subtype(player, TYPE_PERMANENT, SUBTYPE_GOBLIN) ){
		kill_card(player, card, KILL_SACRIFICE);
	}
	return 0;
}

int card_stinkdrinker_bandit(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn == player && current_phase == PHASE_AFTER_BLOCKING && instance->targets[1].player != 66  ){
		int count = active_cards_count[player];
		while( count > -1 ){
				if( in_play(player, count) && is_attacking(player, count) && is_unblocked(player, count) && has_subtype(player, count, SUBTYPE_ROGUE) ){
					pump_until_eot(player, card, player, count, 2, 1);
				}
				count--;
		}
		instance->targets[1].player = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}

	return prowl(player, card, event, SUBTYPE_GOBLIN, SUBTYPE_ROGUE, MANACOST_XB(1, 1));
}

int card_stonehewer_giant(int player, int card, event_t event){

	vigilance(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ARTIFACT, "Select an Equipment card.");
		this_test.subtype = SUBTYPE_EQUIPMENT;
		int equip = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		if( equip != -1 ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE );
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.illegal_abilities = 0;
			td.allow_cancel = 0;
			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				equip_target_creature(player, equip, player, instance->targets[0].card);
			}
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 1, 0, 0, 0, 0, 1, 0, 0, 0);
}

int card_stonybrook_banneret(int player, int card, event_t event){

	return banneret(player, card, event, SUBTYPE_MERFOLK, SUBTYPE_WIZARD);
}

// merfolk wizard --> rhino token.

int card_stonybrook_schoolmaster(int player, int card, event_t event){
	/* Stonybrook Schoolmaster	|2|W
	 * Creature - Merfolk Wizard 1/2
	 * Whenever ~ becomes tapped, you may put a 1/1 |Sblue Merfolk Wizard creature token onto the battlefield. */

	if( event == EVENT_TAP_CARD && affect_me(player, card) ){
		int choice = player == HUMAN ? do_dialog(player, player, card, -1, -1, " Generate a Merfolk Wizard\n Pass", 0) : 0;
		if( choice == 0 ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_MERFOLK_WIZARD, &token);
			token.pow = token.tou = 1;
			token.color_forced = COLOR_TEST_BLUE;
			generate_token(&token);
		}
	}
	return 0;
}

// supreme exemplar --> nova chaser

int card_tauren_mauler(int player, int card, event_t event){

	if( specific_spell_played(player, card, event, 1-player, 1+duh_mode(player), TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_thieves_fortune(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}
	if( event == EVENT_RESOLVE_SPELL ){
		impulse_effect(player, 4, 0);
		kill_card(player, card, KILL_DESTROY);
	}
	return prowl(player, card, event, SUBTYPE_ROGUE, -1, MANACOST_U(1));
}

int card_thornbite_staff(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td1;
	if( event == EVENT_RESOLVE_ACTIVATION
		? (instance->targets[8].player >= 0 && instance->targets[8].card >= 0)	// equipped creature may have left play, so is_equipping() is too strict
		: is_equipping(player, card)
	  ){
		default_target_definition(instance->targets[8].player, instance->targets[8].card, &td1, TYPE_CREATURE);
		td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	} else {
		default_target_definition(player, card, &td1, TYPE_CREATURE);
		td1.illegal_type = TYPE_CREATURE;	// ensure failure
	}

#define ABILITY(event)	generic_activated_ability(instance->targets[8].player, instance->targets[8].card, event,					\
												  GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(2), 0, &td1, "TARGET_CREATURE_OR_PLAYER")

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		return (can_activate_basic_equipment(player, card, event, 4) ||
				(is_equipping(player, card) && ABILITY(event)));
	}
	if( event == EVENT_ACTIVATE ){
			int choice = 0;
			if( can_activate_basic_equipment(player, card, event, 4) ){
				if( is_equipping(player, card) && ABILITY(EVENT_CAN_ACTIVATE) ){
					choice = do_dialog(player, player, card, -1, -1, " Change equipped creature\n Damage target\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}

			instance->info_slot = 66+choice;
			instance->number_of_targets = 0;
			if( choice == 0 ){
				activate_basic_equipment(player, card, 4);
			}
			else if( choice == 1 ){
					if (charge_mana_for_activated_ability(instance->targets[8].player, instance->targets[8].card, MANACOST_X(2))){
						card_instance_t* equipped = get_card_instance(instance->targets[8].player, instance->targets[8].card);
						int old_num_tgts = equipped->number_of_targets;	// since select_target() overwrites it
						load_text(0, "TARGET_CREATURE_OR_PLAYER");
						if (select_target(instance->targets[8].player, instance->targets[8].card, &td1, text_lines[0], &instance->targets[0])){
							equipped->number_of_targets = old_num_tgts;
							instance->number_of_targets = 1;
							tap_card(instance->targets[8].player, instance->targets[8].card);
							return 0;
						}
						equipped->number_of_targets = old_num_tgts;
					}
					spell_fizzled = 1;
			}
			else{
				spell_fizzled = 1;
			}
	}
	if(event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 ){
				resolve_activation_basic_equipment(player, card);
			}
			if( instance->info_slot == 67 && validate_arbitrary_target(&td1, instance->targets[0].player, instance->targets[0].card) ){
				 damage_creature(instance->targets[0].player, instance->targets[0].card, 1, instance->targets[8].player, instance->targets[8].card);
			}
	}

	if( specific_cip(player, card, event, player, 1+player, TYPE_CREATURE, 0, SUBTYPE_SHAMAN, 0, 0, 0, 0, 0, -1, 0) ){
		 equip_target_creature(player, card, instance->targets[1].player, instance->targets[1].card);
	}

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		count_for_gfp_ability(player, card, event, ANYBODY, TYPE_CREATURE, NULL);
	}

	if( is_equipping(player, card) && is_tapped(instance->targets[8].player, instance->targets[8].card) &&
		resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY)
	  ){
		untap_card(instance->targets[8].player, instance->targets[8].card);
	}

	return 0;
#undef ABILITY
}

int card_titans_revenge(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int valid = 0;
		if( valid_target(&td) ){
			damage_target0(player, card, instance->info_slot);
			valid = 1;
		}
		if( valid && clash(player, card) ){
			bounce_permanent(player, card);
		}
		else{
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_unstoppable_ash(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( current_turn == player && current_phase == PHASE_AFTER_BLOCKING && instance->targets[1].player != 66 ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
				if( in_play(player, count) && is_attacking(player, count) && ! is_unblocked(player, count) ){
					pump_until_eot(player, card, player, count, 0, 5);
				}
				count--;
		}
		instance->targets[1].player = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->targets[1].player = 0;
	}
	return champion(player, card, event, SUBTYPE_TREEFOLK, SUBTYPE_WARRIOR);
}

int card_vendilion_clique(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( comes_into_play(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE );
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance( player, card );

		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			char msg[100] = "Select a nonland card.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_LAND, msg);
			this_test.type_flag = 1;

			int selected = new_select_a_card(player, instance->targets[0].player, TUTOR_FROM_HAND, 0, AI_MAX_VALUE, -1, &this_test);
			if( selected != -1 ){
				put_on_top_of_deck(instance->targets[0].player, selected);
				put_top_card_of_deck_to_bottom(instance->targets[0].player);
				draw_cards(instance->targets[0].player, 1);
			}
		}

	}

	return flash(player, card, event);
}

int card_vengeful_firebrand(int player, int card, event_t event){
	if( is_sick(player, card) && count_graveyard_by_subtype(player, SUBTYPE_WARRIOR) > 0 ){
		give_haste(player, card);
	}
	return generic_shade(player, card, event, 0, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0);
}

int card_veterans_armaments(int player, int card, event_t event)
{
  if (event == EVENT_ACTIVATE && !is_equipping(player, card))
	ai_modifier += 30;

  card_instance_t* instance;

  // Equipped creature has "Whenever this creature attacks or blocks, it gets +1/+1 until end of turn for each attacking creature."
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS || event == EVENT_DECLARE_BLOCKERS)
	  && (instance = in_play(player, card))
	  && (xtrigger_condition() == XTRIGGER_ATTACKING	// resolving the trigger
		  || (is_equipping(player, card)
			  && !is_humiliated(player, card)
			  && !is_humiliated(instance->targets[8].player, instance->targets[8].card)))
	  && (declare_attackers_trigger(player, card, event, DAT_TRACK, instance->targets[8].player, instance->targets[8].card)
		  || blocking(instance->targets[8].player, instance->targets[8].card, event)))
	{
	  int p, c;
	  if (xtrigger_condition() == XTRIGGER_ATTACKING)	// apply to the creature it was equipping when attack was declared
		{
		  p = current_turn;
		  c = BYTE0(instance->targets[2].player);
		  if (!in_play(p, c))
			return 0;
		}
	  else	// Blocking, so it's the same creature currently equipped
		{
		  p = instance->targets[8].player;
		  c = instance->targets[8].card;
		}

	  int amount = count_attackers(current_turn);
	  pump_until_eot(player, card, p, c, amount, amount);
	}


  // Whenever a Soldier creature enters the battlefield, you may attach ~ to it.
  if (specific_cip(player, card, event, player, RESOLVE_TRIGGER_AI(player), TYPE_CREATURE, 0, SUBTYPE_SOLDIER, 0, 0, 0, 0, 0, -1, 0))
	{
	  instance = get_card_instance(player, card);
	  equip_target_creature(player, card, instance->targets[1].player, instance->targets[1].card);
	}

  // Equip |2
  return basic_equipment(player, card, event, 2);
}

int card_walker_of_the_grove(int player, int card, event_t event){
	/* Walker of the Grove	|6|G|G
	 * Creature - Elemental 7/7
	 * When ~ leaves the battlefield, put a 4/4 |Sgreen Elemental creature token onto the battlefield.
	 * Evoke |4|G */

	if (comes_into_play(player, card, event) && evoked(player, card)){
		kill_card(player, card, KILL_SACRIFICE);
	}

	if (leaves_play(player, card, event)){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.color_forced = COLOR_TEST_GREEN;
		token.pow = 4;
		token.tou = 4;
		generate_token(&token);
	}

	return evoke(player, card, event, 4, 0, 0, 1, 0, 0);
}

int card_wandering_graybeard(int player, int card, event_t event){
	if (kinship(player, card, event)){
		gain_life(player, 4);
	}
	return 0;
}

// warren's weirdings --> diabolic edict

int card_weirding_shaman(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 3, 1, 0, 0, 0, 0) ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 3, 1, 0, 0, 0, 0);
		if( spell_fizzled != 1 && ! sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0)  ){
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GOBLIN_ROGUE, &token);
		token.qty = 2;
		generate_token(&token);
	}

	return 0;
}

int card_wolf_skull_shaman(int player, int card, event_t event){
	/* Wolf-Skull Shaman	|1|G
	 * Creature - Elf Shaman 2/2
	 * Kinship - At the beginning of your upkeep, you may look at the top card of your library. If it shares a creature type with ~, you may reveal it. If you do, put a 2/2 |Sgreen Wolf creature token onto the battlefield. */

	if (kinship(player, card, event)){
		generate_token_by_id(player, card, CARD_ID_WOLF);
	}
	return 0;
}
