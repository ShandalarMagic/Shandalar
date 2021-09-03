#include <limits.h>
#include "manalink.h"

// Functions


static void bestow(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_MODIFY_COST ){
		int cless = get_updated_casting_cost(player, -1, get_internal_card_id_from_csv_id(get_id(player, card) + 1), event, colorless);
		if( has_mana_multi(player, cless, black, blue, green, red, white) && can_target(&td) ){
			null_casting_cost(player, card);
			instance->info_slot = 1;
		} else {
			instance->info_slot = 0;
		}
	}
}

static int pay_bestow(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white){

	card_instance_t *instance = get_card_instance(player, card);

	if( played_for_free(player, card) || is_token(player, card) ){
		return 0;
	}
	int double_x = 0;
	if( colorless == -2 ){
		double_x = 1;
		colorless = 0;
	}
	int choice = 0;
	int cless = get_updated_casting_cost(player, -1, get_internal_card_id_from_csv_id(get_id(player, card) + 1), event, colorless);
	if( has_mana_multi(player, cless, black, blue, green, red, white) ){
		if( has_mana_to_cast_id(player, event, get_id(player, card)) ){
			choice = do_dialog(player, player, card, -1, -1, " Pay Bestow cost\n Play the spell normally\n Cancel", 0);
		}
	}
	else{
		choice = 1;
	}
	if( choice == 0 ){
		charge_mana_multi(player, cless, black, blue, green, red, white);
		if( spell_fizzled != 1 ){
			if( double_x ){
				instance->targets[1].player = charge_mana_for_double_x(player, COLOR_COLORLESS);
				if( spell_fizzled == 1 ){
					return 0;
				}
			}
			return 2;
		}
	}
	else if( choice == 1){
			if( charge_mana_from_id(player, card, event, get_id(player, card)) ){
				return 1;
			}
	}
	else{
		spell_fizzled = 1;
	}
	return 0;
}

int creature_with_targeted_bestow(int player, int card, event_t event, target_definition_t *td, int colorless, int black, int blue, int green, int red, int white,
								  int pow, int tou, int key, int s_key
  ){
	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	bestow(player, card, event, colorless, black, blue, green, red, white);

	if(event == EVENT_CAN_SKIP_TURN){
	   instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE|KEYWORD_RECALC_ABILITIES;
	   get_abilities(player, card, EVENT_CHANGE_TYPE, -1);
	   get_abilities(player, card, EVENT_ABILITIES, -1);
	}

	if( event == EVENT_CHANGE_TYPE && affect_me(player, card) && instance->targets[12].card != -1 ){
		event_result = instance->targets[12].card;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[13].player = get_id(player, card);
		instance->targets[13].card = get_id(player, card);
		int result = 0;
		if( instance->info_slot == 1 ){
			result = pay_bestow(player, card, event, colorless, black, blue, green, red, white);
		}
		else{
			result = 1;
		}
		if( result == 0 ){
			spell_fizzled = 1;
		}
		else if( result == 2 ){
				if( pick_target(td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					instance->info_slot = 66;
					true_transform(player, card);
					instance->backup_internal_card_id = instance->internal_card_id = instance->targets[12].card;	// Since it won't get EVENT_CHANGE_TYPE events while on the stack
				}
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( instance->number_of_targets > 0 ){
			if( valid_target(td) ){
				instance->damage_target_player = instance->targets[0].player;
				instance->damage_target_card = instance->targets[0].card;
			} else {
				true_transform(player, card);
			}
		}
	}

	if (in_play(player, card) && instance->damage_target_player > -1 ){
		modify_pt_and_abilities(instance->damage_target_player, instance->damage_target_card, event, pow, tou, key);
		special_abilities(instance->damage_target_player, instance->damage_target_card, event, s_key, player, card);
		if( other_leaves_play(player, card, instance->damage_target_player, instance->damage_target_card, event) && instance->info_slot == 66){
			instance->info_slot = 2;
			instance->damage_target_player = instance->damage_target_card = -1;
			true_transform(player, card);
		}
	}

	return 0;
}

int generic_creature_with_bestow(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white,
								int pow, int tou, int key, int s_key
  ){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	return creature_with_targeted_bestow(player, card, event, &td, colorless, black, blue, green, red, white, pow, tou, key, s_key);
}

// Same as heroic, but not necessarily RESOLVE_TRIGGER_MANDATORY.
int heroic_mode(int player, int card, event_t event, resolve_trigger_t trigger_mode){

	if( in_play(player, card) && ! is_humiliated(player, card) ){
		const target_t* tgt;
		if ((tgt = becomes_target_of_spell(player, card, event, player, card, player, trigger_mode))){
			// (Does anything with heroic actually rely on targets[1] being set like specific_cip() does?)
			get_card_instance(player, card)->targets[1] = *tgt;	// struct copy
			return 1;
		}
	}

	return 0;
}

int heroic(int player, int card, event_t event){
	return heroic_mode(player, card, event, RESOLVE_TRIGGER_MANDATORY);
}

// Exactly the same as chroma(player, -1, clr1, clr2), except sleights clr1 and clr2 unless card is -1.
int devotion(int player, int card, int clr1, int clr2)
{
  if (card >= 0)
	{
	  if (clr1)
		clr1 = get_sleighted_color(player, card, clr1);

	  if (clr2)
		clr2 = get_sleighted_color(player, card, clr2);
	}

  return chroma(player, -1, clr1, clr2);
}

void generic_creature_with_devotion(int player, int card, event_t event, int dev_color1, int dev_color2, int amount){

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_CAN_SKIP_TURN){
		instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE|KEYWORD_RECALC_ABILITIES;
		get_abilities(player, card, EVENT_CHANGE_TYPE, -1);
		get_abilities(player, card, EVENT_ABILITIES, -1);
	}

	if (event == EVENT_RESOLVE_SPELL && instance->targets[12].card != -1){
		/* During EVENT_RESOLVE_SPELL, STATE_INVISIBLE will still be set if this was put into play rather than cast, which would prevent devotion() from
		 * counting this card; so temporarily remove */
		int was_invisible = instance->state & STATE_INVISIBLE;
		int dev = devotion(player, card, dev_color1, dev_color2);
		instance->state |= was_invisible;
		if (dev >= amount && instance->internal_card_id != instance->targets[12].card){
			/* We can only possibly get away with changing internal_card_id during EVENT_RESOLVE_SPELL because all gods have the same function for both forms.
			 * It might still break things; cards shouldn't change type except during EVENT_CHANGE_TYPE. */
			instance->backup_internal_card_id = instance->internal_card_id = instance->targets[12].card;
			instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE|KEYWORD_RECALC_ABILITIES;
			if (!was_invisible){
				// Was cast.  Play the creature sound in addition to the enchantment sound already playing.
				play_sound_effect(WAV_SUMMON);
			}
		}
	}

	if (event == EVENT_CHANGE_TYPE && affect_me(player, card) && in_play(player, card) && instance->targets[12].card != -1){
		int dev = devotion(player, card, dev_color1, dev_color2);
		if (dev >= amount && instance->internal_card_id != instance->targets[12].card){
			event_result = instance->targets[12].card;
			instance->regen_status |= KEYWORD_RECALC_CHANGE_TYPE|KEYWORD_RECALC_ABILITIES;
		}
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->targets[13].player = get_id(player, card);
		instance->targets[13].card = instance->targets[13].player+1;
		instance->targets[12].card = get_internal_card_id_from_csv_id(get_id(player, card)+1);
	}
}

int is_monstrous(int player, int card){
	if( check_special_flags2(player, card, SF2_MONSTROUS) ){
		return 1;
	}
	return 0;
}

int monstrosity(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white, int c_amount){

	if (event == EVENT_ACTIVATE
		|| (event == EVENT_CAN_ACTIVATE && !is_monstrous(player, card))
	   ){
		int flags = IS_AI(player) ? GAA_ONCE_PER_TURN : 0;
		return generic_activated_ability(player, card, event, flags, colorless, black,  blue, green, red, white, 0, 0, 0);
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);
		if (in_play(player, instance->parent_card) && !is_monstrous(player, instance->parent_card)){
			set_special_flags2(player, instance->parent_card, SF2_MONSTROUS);
			add_1_1_counters(player, instance->parent_card, c_amount);
			call_card_function(player, instance->parent_card, EVENT_BECAME_MONSTROUS);
		}
	}

	if ((event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)
		&& !is_monstrous(player, card)
	   ){
		if (colorless == -2){
			/* Both Monstrosity X cards, as it happens, have cost XXC.
			 * This assumes that all mana sources produce an equal amount of mana no matter which colors they're tapped for; so far as I know, the only
			 * exception is Nykthos, Shrine to Nyx. */
			colorless = get_cost_mod_for_activated_abilities(player, card, 0, black, blue, green, red, white);
			if (!has_mana_multi(player, colorless, black, blue, green, red, white)){
				return 0;
			}
			int total_mana = has_mana(player, COLOR_ANY, -1);
			total_mana -= (colorless + black + blue + green + red + white);
			if (total_mana < 2){
				return 0;
			} else {
				return total_mana / 2;
			}
		} else if (CAN_ACTIVATE(player, card, MANACOST6(colorless, black, blue, green, red, white))){
			return c_amount;
		}
	}

	return 0;
}

static int ordeal(int player, int card, event_t event)
{
  card_instance_t* instance;
  // Whenever enchanted creature attacks, put a +1/+1 counter on it. Then if it has three or more +1/+1 counters on it, sacrifice ~.
  if ((xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	  && (instance = in_play(player, card))
	  && instance->damage_target_player >= 0
	  && declare_attackers_trigger(player, card, event, 0, instance->damage_target_player, instance->damage_target_card))
	{
	  add_1_1_counter(instance->damage_target_player, instance->damage_target_card);
	  if (count_1_1_counters(instance->damage_target_player, instance->damage_target_card) >= 3)
		kill_card(player, card, KILL_SACRIFICE);
	}

  // When you sacrifice ~, [...]
  if (event == EVENT_GRAVEYARD_FROM_PLAY && affect_me(player, card)
	  && (instance = in_play(player, card))
	  && instance->kill_code == KILL_SACRIFICE && !check_special_flags(affected_card_controller, affected_card, SF_KILL_STATE_BASED_ACTION))
	instance->targets[11].player = 66;

  return vanilla_aura(player, card, event, player);
}

// Cards

// White
int card_battlewise_valor(int player, int card, event_t event){
	/* Battlewise Valor	|1|W
	 * Instant
	 * Target creature gets +2/+2 until end of turn. Scry 1. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 2, 2, VANILLA_PUMP_SCRY_1, 0);
}

int card_cavalry_pegasus(int player, int card, event_t event)
{
  // Whenever ~ attacks, each attacking Human gains flying until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t this_test;
	  default_test_definition(&this_test, TYPE_CREATURE);
	  this_test.subtype = SUBTYPE_HUMAN;
	  this_test.state = STATE_ATTACKING;
	  pump_creatures_until_eot(player, card, player, 0, 0, 0, KEYWORD_FLYING, 0, &this_test);
	}

  return 0;
}

int card_celestial_archon(int player, int card, event_t event){
	return generic_creature_with_bestow(player, card, event, 5, 0, 0, 0, 0, 2, 4, 4, KEYWORD_FLYING+KEYWORD_FIRST_STRIKE, 0);
}

int card_chained_to_the_rocks(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_subtype = get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	return_from_oblivion(player, card, event);

	if( event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( can_target(&td1) ){
			ai_modifier+=50;
		}
		pick_target(&td, "TARGET_LAND");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			instance->damage_target_player = instance->targets[0].player;
			instance->damage_target_card = instance->targets[0].card;
		}
		else{
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if (comes_into_play(player, card, event)){
		if (instance->damage_target_player >= 0 && can_target(&td1)){
			if (new_pick_target(&td1, "TARGET_CREATURE", 1, 0)){
				obliviation(player, card, instance->targets[1].player, instance->targets[1].card);
			}
		}
	}

	return 0;
}

int card_chosen_by_heliod(int player, int card, event_t event){
	if (comes_into_play(player, card, event)){
		draw_cards(player, 1);
	}

	return generic_aura(player, card, event, player, 0, 2, 0, 0, 0, 0, 0);
}

int card_coastline_chimera(int player, int card, event_t event)
{
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  can_block_additional_until_eot(player, instance->parent_card, player, instance->parent_card, 1);
	}

  return generic_activated_ability(player, card, event, GAA_NONE, MANACOST_XW(1,1), 0, NULL, NULL);
}

int card_dauntless_onslaught(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int i, successes = 0;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				pump_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 2, 2);
				++successes;
			}
		}
		if (successes == 0 && instance->number_of_targets > 0){
			spell_fizzled = 1;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

static const char* target_noncombat_damage(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
	card_instance_t* inst = get_card_instance(player, card);
	if (inst->internal_card_id != damage_card){
		return ",card type";
	}
	if (!(inst->token_status & (STATUS_COMBAT_DAMAGE | STATUS_FIRST_STRIKE_DAMAGE))){
		return ",combat damage";
	}
	return NULL;
}
int card_decorated_griffin(int player, int card, event_t event){
	/* Decorated Griffin	|4|W
	 * Creature - Griffin 2/3
	 * Flying
	 * |1|W: Prevent the next 1 combat damage that would be dealt to you this turn. */

	if (!IS_GAA_EVENT(event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_NONE);
	td.preferred_controller = ANYBODY;	// damage cards attached to a player are internally controlled by that player
	td.extra = (int)target_noncombat_damage;
	td.illegal_abilities = 0;
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER | TARGET_SPECIAL_EXTRA_FUNCTION;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
		if( target->info_slot > 0 ){
			target->info_slot--;
		}
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_DAMAGE_PREVENTION_ME, 1, 0, 0, 0, 0, 1, 0, &td, "TARGET_DAMAGE");
}

int card_elspeth_suns_champion(int player, int card, event_t event){

	/* Elspeth, Sun's Champion	|4|W|W
	 * Planeswalker - Elspeth (4)
	 * +1: Put three 1/1 |Swhite Soldier creature tokens onto the battlefield.
	 * -3: Destroy all creatures with power 4 or greater.
	 * -7: You get an emblem with "Creatures you control get +2/+2 and have flying." */

	if (IS_ACTIVATING(event)){

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.power = 3;
		this_test.power_flag = 2;

		int priority_kill = 0;
		int priority_emblem = 0;
		if( event == EVENT_ACTIVATE ){
			priority_kill = (check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test)-
							check_battlefield_for_special_card(player, card, player, CBFSC_GET_COUNT, &this_test))*4;
			priority_kill += ((count_counters(player, card, COUNTER_LOYALTY)*4)-12);
			if( ! check_battlefield_for_id(player, CARD_ID_ELSPETH_SUNS_CHAMPION_EMBLEM) ){
				priority_emblem = 5*count_subtype(player, TYPE_CREATURE, -1);
				priority_emblem += ((count_counters(player, card, COUNTER_LOYALTY)*4)-21);
			}
		}

		enum{
			CHOICE_SOLDIERS = 1,
			CHOICE_KILL_MIGHTIES,
			CHOICE_EMBLEM
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Generate 3 Soldiers", 1, 10, 1,
						"Kill all creatures with POW >= 4", 1, priority_kill, -3,
						"Emblem", 1, priority_emblem, -7);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
			switch (choice)
			{
				case CHOICE_SOLDIERS:
					generate_tokens_by_id(player, card, CARD_ID_SOLDIER, 3);
					break;

				case CHOICE_KILL_MIGHTIES:
					APNAP(p, {new_manipulate_all(player, card, p, &this_test, KILL_DESTROY);};);
					break;

				case CHOICE_EMBLEM:
					generate_token_by_id(player, card, CARD_ID_ELSPETH_SUNS_CHAMPION_EMBLEM);
					break;
			}
		}
	}

	return planeswalker(player, card, event, 4);
}

int card_elspeth_suns_champion_emblem(int player, int card, event_t event){
	/* Elspeth, Sun's Champion Emblem	""
	 * Emblem
	 * Creatures you control get +2/+2 and have flying. */

	boost_subtype(player, card, event, -1, 2,2, KEYWORD_FLYING,0, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);
	return 0;
}

int card_epharas_warden(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 3 | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_evangel_of_heliod(int player, int card, event_t event){
	/* Evangel of Heliod	|4|W|W
	 * Creature - Human Cleric 1/3
	 * When ~ enters the battlefield, put a number of 1/1 |Swhite Soldier creature tokens onto the battlefield equal to your devotion to |Swhite. */

	if( comes_into_play(player, card, event) ){
		generate_tokens_by_id(player, card, CARD_ID_SOLDIER, devotion(player, card, COLOR_WHITE, 0));
	}

	return 0;
}

int card_fabled_hero(int player, int card, event_t event){

	/* Akroan Skyguard	|1|W
	 * Creature - Human Soldier 1/1
	 * Flying
	 * Heroic - Whenever you cast a spell that targets ~, put a +1/+1 counter on ~. */

	/* Fabled Hero	|1|W|W
	 * Creature - Human Soldier 2/2
	 * Double strike
	 * Heroic - Whenever you cast a spell that targets ~, put a +1/+1 counter on ~. */

	/* Lagonna-Band Trailblazer	|W
	 * Creature - Centaur Scout 0/4
	 * Heroic - Whenever you cast a spell that targets ~, put a +1/+1 counter on ~. */

	/* Satyr Hoplite	|R
	 * Creature - Satyr Soldier 1/1
	 * Heroic - Whenever you cast a spell that targets ~, put a +1/+1 counter on ~. */

	/* War-Wing Siren	|2|U
	 * Creature - Siren Soldier 1/3
	 * Flying
	 * Heroic - Whenever you cast a spell that targets ~, put a +1/+1 counter on ~. */

	/* Wingsteed Rider	|1|W|W
	 * Creature - Human Knight 2/2
	 * Flying
	 * Heroic - Whenever you cast a spell that targets ~, put a +1/+1 counter on ~. */

	if( heroic(player, card, event) ){
		add_1_1_counter(player, card);
	}

	return 0;
}

int card_favored_hoplite(int player, int card, event_t event){

	if( heroic(player, card, event) ){
		add_1_1_counter(player, card);
		prevent_the_next_n_damage(player, card, player, card, 0, PREVENT_INFINITE, 0, 0);
	}

	return 0;
}

static int gift_of_immortality_legacy(int legacy_p, int legacy_c, event_t event){

	card_instance_t* legacy_instance = get_card_instance(legacy_p, legacy_c);

	int aura_p = legacy_instance->targets[0].player;
	int aura_c = legacy_instance->targets[0].card;

	// Track most recent enchanted creature
	card_instance_t* aura_instance = get_card_instance(aura_p, aura_c);
	if (in_play(aura_p, aura_c) && aura_instance->damage_target_player >= 0){
		legacy_instance->targets[1].player = aura_instance->damage_target_player;
		legacy_instance->targets[1].card = aura_instance->damage_target_card;
	}

	int enchanted_p = legacy_instance->targets[1].player;
	int enchanted_c = legacy_instance->targets[1].card;

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		if( affect_me(enchanted_p, enchanted_c) ){
			card_instance_t *affected = get_card_instance(enchanted_p, enchanted_c);
			if( is_token(enchanted_p, enchanted_c) ){
				kill_card(legacy_p, legacy_c, KILL_REMOVE);
			}
			else if( affected->kill_code > 0 && affected->kill_code < KILL_REMOVE){
					legacy_instance->targets[11].player = 2 | get_owner(enchanted_p, enchanted_c);
					legacy_instance->targets[11].card = cards_data[get_original_internal_card_id(enchanted_p, enchanted_c)].id;
			}
		}
	}

	if( legacy_instance->targets[11].player > -1 && (legacy_instance->targets[11].player & 2) && resolve_graveyard_trigger(legacy_p, legacy_c, event) ){
		int owner = legacy_instance->targets[11].player &= ~2;
		int result = seek_grave_for_id_to_reanimate(legacy_p, legacy_c, owner, legacy_instance->targets[11].card, REANIMATEXTRA_RETURN_TO_HAND2);
		if (result >= 0){
			legacy_instance->targets[2].player = owner;
			legacy_instance->targets[2].card = result;
			legacy_instance->number_of_targets = 3;
			put_into_play(owner, result);
		}
		legacy_instance->targets[11].player = -1;
	}

	int resurrected_p = legacy_instance->targets[2].player;
	int resurrected_c = legacy_instance->targets[2].card;

	if (resurrected_p >= 0 && in_play(resurrected_p, resurrected_c) && eot_trigger(legacy_p, legacy_c, event) ){
		if( legacy_instance->targets[2].player > -1 ){
			int result = seek_grave_for_id_to_reanimate(legacy_p, legacy_c, legacy_p, CARD_ID_GIFT_OF_IMMORTALITY, REANIMATEXTRA_RETURN_TO_HAND2);
			if (result >= 0){
				put_into_play_aura_attached_to_target(legacy_p, result, resurrected_p, resurrected_c);
				kill_card(legacy_p, legacy_c, KILL_REMOVE);
			}
		}
		legacy_instance->targets[2].player = -1;
	}
	else if (resurrected_p < 0 && legacy_instance->targets[11].player == -1 && !in_play(aura_p, aura_c)){
			kill_card(legacy_p, legacy_c, KILL_REMOVE);
	}
	return 0;
}

int card_gift_of_immortality(int player, int card, event_t event){

	int rval = vanilla_aura(player, card, event, player);

	if (event == EVENT_RESOLVE_SPELL && spell_fizzled != 1){	// must come after the vanilla_aura() call, and in the same call to this function
		int legacy = create_legacy_effect(player, card, &gift_of_immortality_legacy);
		card_instance_t *leg = get_card_instance(player, legacy);
		leg->targets[0].player = player;
		leg->targets[0].card = card;
		leg->targets[1] = get_card_instance(player, card)->targets[0];	// struct copy
		leg->number_of_targets = 2;
	}

	return rval;
}

int card_glare_of_heresy(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT );
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PERMANENT", 1, NULL);
}

int card_gods_willing(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int kw = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, kw, 0);
			scry(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_heliod_god_of_the_sun(int player, int card, event_t event){
	/* Heliod, God of the Sun	|3|W
	 * Legendary Enchantment Creature - God 0/0
	 * Indestructible
	 * As long as your devotion to |Swhite is less than five, Heliod isn't a creature.
	 * Other creatures you control have vigilance.
	 * |2|W|W: Put a 2/1 |Swhite Cleric enchantment creature token onto the battlefield. */

	check_legend_rule(player, card, event);

	indestructible(player, card, event);

	generic_creature_with_devotion(player, card, event, get_sleighted_color(player, card, COLOR_WHITE), 0, 5);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_CLERIC, &token);
		token.pow = 2;
		token.tou = 1;
		token.color_forced = COLOR_TEST_WHITE;
		token.special_flags2 = SF2_ENCHANTED_EVENING;
		generate_token(&token);
	}

	if (event == EVENT_ABILITIES
		&& affected_card_controller == player
		&& affected_card != card
		&& is_what(affected_card_controller, affected_card, TYPE_CREATURE)){
		vigilance(affected_card_controller, affected_card, event);
	}

	return generic_activated_ability(player, card, event, 0, 2, 0, 0, 0, 0, 2, 0, 0, 0);
}

int card_heliods_emissary(int player, int card, event_t event)
{
  // Whenever ~ or enchanted creature attacks, tap target creature an opponent controls.
  card_instance_t* instance = in_play(player, card);
  if (declare_attackers_trigger(player, card, event, 0, player, card)
	  || (instance && declare_attackers_trigger(player, card, event, 0, instance->damage_target_player, instance->damage_target_card)))
	{
	  target_definition_t td;
	  default_target_definition(player, card, &td, TYPE_CREATURE);
	  td.allowed_controller = 1-player;
	  td.allow_cancel = 0;

	  instance->number_of_targets = 1;
	  if (can_target(&td) && new_pick_target(&td, "TARGET_CREATURE_OPPONENT_CONTROLS", 1, 0))
		tap_card(instance->targets[1].player, instance->targets[1].card);
	}

  // Bestow |6|W
  return generic_creature_with_bestow(player, card, event, MANACOST_XW(6,1), 3,3, 0,0);
}

int card_hopeful_eidolon(int player, int card, event_t event){

	lifelink(player, card, event);

	return generic_creature_with_bestow(player, card, event, 3, 0, 0, 0, 0, 1, 1, 1, 0, SP_KEYWORD_LIFELINK);
}

int card_hundred_handed_one(int player, int card, event_t event){
	vigilance(player, card, event);
	if( is_monstrous(player, card) ){
		modify_pt_and_abilities(player, card, event, 0, 0, KEYWORD_REACH);
		creature_can_block_additional(player, card, event, 99);
	}
	return monstrosity(player, card, event, 3, 0, 0, 0, 0, 3, 3);
}

int card_lagonna_band_elder(int player, int card, event_t event){
	if( comes_into_play(player, card, event) && count_subtype(player, TYPE_ENCHANTMENT, -1) > 0 ){
		gain_life(player, 3);
	}
	return 0;
}

int card_last_breath(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 2 | TARGET_PT_LESSER_OR_EQUAL;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			gain_life(instance->targets[0].player, 4);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_leonin_snarecaster(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_CREATURE") ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return 0;
}

int card_observant_alseid(int player, int card, event_t event){

	vigilance(player, card, event);

	return generic_creature_with_bestow(player, card, event, 4, 0, 0, 0, 0, 1, 2, 2, 0, SP_KEYWORD_VIGILANCE);
}

int card_ordeal_of_heliod(int player, int card, event_t event){

	if (resolve_graveyard_trigger(player, card, event)){
		get_card_instance(player, card)->targets[11].player = -1;
		gain_life(player, 10);
	}

	return ordeal(player, card, event);
}

int card_phalanx_leader(int player, int card, event_t event)
{
  /* Phalanx Leader	|W|W
   * Creature - Human Soldier 1/1
   * Heroic - Whenever you cast a spell that targets ~, put a +1/+1 counter on each creature you control. */

  if (heroic(player, card, event))
	manipulate_type(player, card, player, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_P1_P1, 1));

  return 0;
}

int card_ray_of_dissolution(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			gain_life(player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_ENCHANTMENT", 1, NULL);
}

int card_scholar_of_athreos(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int result = lose_life(1-player, 1);
		gain_life(player, result);
	}

	return generic_activated_ability(player, card, event, 0, 2, 1, 0, 0, 0, 0, 0, 0, 0);
}

int card_setessan_battle_priest(int player, int card, event_t event){

	if( heroic(player, card, event) ){
		gain_life(player, 2);
	}

	return 0;
}

int card_setessan_griffin(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, 2, 2);
	}

	return generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, 2, 0, 0, 2, 0, 0, 0, 0, 0);
}

// Silent Artisan --> vanilla

int card_soldier_of_the_pantheon(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_ANY);
	this_test.color_flag = F3_MULTICOLORED;

	if( new_specific_spell_played(player, card, event, 1-player, 2, &this_test) ){
		gain_life(player, 1);
	}

	protection_from_multicolored(player, card, event);

	return 0;
}

static const char* target_damaged_me_this_turn(int who_chooses, int player, int card, int targeting_player, int targeting_card)
{
	int good = who_chooses == 0 ? check_special_flags2(player, card, SF2_HAS_DAMAGED_PLAYER0) :
							check_special_flags2(player, card, SF2_HAS_DAMAGED_PLAYER1);
	return good ? NULL : "dealt damage to you this turn";
}

int card_spear_of_heliod(int player, int card, event_t event)
{
	check_legend_rule(player, card, event);

	if (event == EVENT_CAN_CAST)
		return 1;

	// Creatures you control get +1/+1.
	boost_creature_type(player, card, event, -1, 1, 1, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	// |1|W|W, |T: Destroy target creature that dealt damage to you this turn.
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td.extra = (int)target_damaged_me_this_turn;

	if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (valid_target(&td))
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_XW(1,2), 0, &td, "TARGET_CREATURE");
}

// Travelling philosopher --> vanilla

int card_vanquish_the_foul(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.power_requirement = 4 | TARGET_PT_GREATER_OR_EQUAL;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			scry(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

// wingsteed rider --> fabled hero

// yoked ox --> vanilla

// Blue

int card_aqueous_form(int player, int card, event_t event)
{
  // Whenever enchanted creature attacks, scry 1.
  card_instance_t* instance = in_play(player, card);
  if (instance && declare_attackers_trigger(player, card, event, 0, instance->damage_target_player, instance->damage_target_card))
	scry(player, 1);

  // Enchanted creature can't be blocked.
  return generic_aura(player, card, event, player, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE, 0, 0, 0);
}

static int effect_artisan_of_forms(int player, int card, event_t event)
{
  if (effect_follows_control_of_attachment(player, card, event))
	return 0;

  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player, c = instance->damage_target_card;

  if (becomes_target_of_spell(player, card, event, p, c, player, RESOLVE_TRIGGER_AI(player)))
	{
	  target_definition_t td;
	  default_target_definition(p, c, &td, TYPE_CREATURE);
	  td.preferred_controller = ANYBODY;

	  instance->number_of_targets = 0;
	  load_text(0, "TARGET_CREATURE");
	  if (select_target(p, c-1000, &td, text_lines[0], &instance->targets[0]))
		{
		  if (instance->targets[0].card == c && instance->targets[0].player == p)	// picked self
			return 0;

		  cloning_and_verify_legend(p, c, instance->targets[0].player, instance->targets[0].card);
		}
	}

  return 0;
}
int card_artisan_of_forms(int player, int card, event_t event)
{
  /* Artisan of Forms	|1|U
   * Creature - Human Wizard 1/1
   * Heroic - Whenever you cast a spell that targets ~, you may have ~ become a copy of target creature and gain this ability. */

  cloning_card(player, card, event);

  if (event == EVENT_RESOLVE_SPELL)
	create_targetted_legacy_effect(player, card, &effect_artisan_of_forms, player, card);

  return 0;
}

// benthic giant --> aven_fleetwing

int card_bident_of_thassa(int player, int card, event_t event){

	/* Bident of Thassa	|2|U|U
	 * Legendary Enchantment Artifact
	 * Whenever a creature you control deals combat damage to a player, you may draw a card.
	 * |1|U, |T: Creatures your opponents control attack this turn if able. */

	check_legend_rule(player, card, event);

	if (event == EVENT_CAN_CAST){
		return 1;
	}

	if (subtype_deals_damage(player, card, event, player, -1, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER|DDBM_TRIGGER_OPTIONAL)){
		draw_some_cards_if_you_want(player, card, player, get_card_instance(player, card)->targets[1].card);
	}

	if( event == EVENT_ACTIVATE && current_turn == player ){
		ai_modifier-=100;
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		pump_subtype_until_eot(player, card, 1-player, -1, 0, 0, 0, SP_KEYWORD_MUST_ATTACK);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 1, 0, 1, 0, 0, 0, 0, 0, 0);
}

int card_breaching_hippocamp(int player, int card, event_t event){

	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = player;
		td.preferred_controller = player;

		state_untargettable(player, card, 1);
		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
			card_instance_t* instance = get_card_instance(player, card);
			untap_card(instance->targets[0].player, instance->targets[0].card);
		}
		state_untargettable(player, card, 0);
	}

	return flash(player, card, event);
}

// Coastline Chimera --> multiblock.c

int card_crackling_triton(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_ME, 2, 0, 0, 0, 1, 0, 0, &td, "TARGET_CREATURE_OR_PLAYER");
}

int card_curse_of_the_swine(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST && can_target(&td) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}

	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int trgs = 0;
			while( can_target(&td) && has_mana(player, COLOR_COLORLESS, trgs + 1) && trgs < 19){
					if( new_pick_target(&td, "TARGET_CREATURE", trgs, 0) ){
						state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
						trgs++;
					}
					else{
						break;
					}
			}
			int i;
			for(i=0; i<trgs; i++){
				state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			}
			charge_mana(player, COLOR_COLORLESS, trgs);
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			int i, num[2] = {0};
			for (i = 0; i < instance->number_of_targets; i++){
				if( validate_target(player, card, &td, i) ){
					kill_card(instance->targets[i].player, instance->targets[i].card, KILL_REMOVE);
					++num[instance->targets[i].player];
				}
			}

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_BOAR, &token);
			token.pow = 2;
			token.tou = 2;
			for (i = 0; i < 2; ++i){
				if (num[i] > 0){
					token.t_player = i;
					token.qty = num[i];
					generate_token(&token);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_dissolve(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		if (counterspell_validate(player, card, 0, 0)){
			card_instance_t *instance = get_card_instance(player, card);
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
			scry(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 0, NULL);
}

int card_fate_foretold(int player, int card, event_t event){
	/* Fate Foretold	|1|U
	 * Enchantment - Aura
	 * Enchant creature
	 * When ~ enters the battlefield, draw a card.
	 * When enchanted creature dies, its controller draws a card. */

	int rval = vanilla_aura(player, card, event, player);

	if (comes_into_play(player, card, event)){
		draw_cards(player, 1);
	}

	if (attached_creature_dies_trigger_for_controller(player, card, event, RESOLVE_TRIGGER_MANDATORY)){
		draw_cards(get_card_instance(player, card)->damage_target_player, 1);
	}

	return rval;
}

int card_gainsay(int player, int card, event_t event){
	target_definition_t td;
	counterspell_target_definition(player, card, &td, 0);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);

	return counterspell(player, card, event, &td, 0);
}

int card_horizon_scholar(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		scry(player, 2);
	}
	return 0;
}

int card_lost_in_a_labyrinth(int player, int card, event_t event){
	/* Lost in a Labyrinth	|U
	 * Instant
	 * Target creature gets -3/-0 until end of turn. Scry 1. */
	return vanilla_instant_pump(player, card, event, ANYBODY, 1-player, -3, 0, VANILLA_PUMP_SCRY_1, 0);
}

int card_master_of_waves(int player, int card, event_t event){
	/* Master of Waves	|3|U
	 * Creature - Merfolk Wizard 2/1
	 * Protection from |Sred
	 * Elemental creatures you control get +1/+1.
	 * When ~ enters the battlefield, put a number of 1/0 |Sblue Elemental creature tokens onto the battlefield equal to your devotion to |Sblue. */

	boost_creature_type(player, card, event, SUBTYPE_ELEMENTAL, 1, 1, 0, BCT_CONTROLLER_ONLY | BCT_INCLUDE_SELF);

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_ELEMENTAL, &token);
		token.qty = devotion(player, card, COLOR_BLUE, 0);
		token.pow = 1;
		token.tou = 0;
		token.color_forced = COLOR_TEST_BLUE;
		generate_token(&token);
	}

	return 0;
}

int card_meletis_charlatan(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td_fork;
	counterspell_target_definition(player, card, &td_fork, TYPE_INSTANT|TYPE_SORCERY);
	td_fork.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( counterspell_validate(player, card, &td_fork, 0) ){
			copy_spell_from_stack(instance->targets[0].player, instance->targets[0].player, instance->targets[0].card);
		}
	}


	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_NONSICK | GAA_SPELL_ON_STACK, 2, 0, 1, 0, 0, 0, 0, &td_fork, 0);
}

int card_nimbus_naiad(int player, int card, event_t event){
	return generic_creature_with_bestow(player, card, event, 4, 0, 1, 0, 0, 0, 2, 2, KEYWORD_FLYING, 0);
}

// omenspeaker --> horizon scholar

int card_ordeal_of_thassa(int player, int card, event_t event){

	if (resolve_graveyard_trigger(player, card, event)){
		get_card_instance(player, card)->targets[11].player = -1;
		draw_cards(player, 2);
	}

	return ordeal(player, card, event);
}

int card_prescient_chimera(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_SPELL);

	if( new_specific_spell_played(player, card, event, player, 2, &this_test) ){
		scry(player, 1);
	}

	return 0;
}

int card_prognostic_sphinx(int player, int card, event_t event)
{
  // Whenever ~ attacks, scry 3.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	scry(player, 3);

  // Discard a card: ~ gains hexproof until end of turn. Tap it.
  if (event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  pump_ability_until_eot(player, card, instance->parent_controller, instance->parent_card, 0,0, 0,SP_KEYWORD_HEXPROOF);
	  tap_card(instance->parent_controller, instance->parent_card);
	}

  return generic_activated_ability(player, card, event, GAA_DISCARD, MANACOST_X(0), 0, NULL, NULL);
}

int card_sea_gods_revenge(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		int i, good = 0;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				good++;
				bounce_permanent(instance->targets[i].player, instance->targets[i].card);
			}
		}
		if( good || instance->number_of_targets == 0){
			scry(player, 1);
		} else {
			spell_fizzled = 1;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 3, NULL);
}

int sealock_monster_legacy(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	if ( instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_CAN_ACTIVATE && instance->damage_target_player == player ){ //Or you will be able to tap opponent's lands for mana.
			return mana_producer(p, c, event);
		}
		if( event == EVENT_ACTIVATE ){
			tap_card(p, c);
			int clr = check_special_flags2(p, c, SF2_CONTAMINATION) ? COLOR_BLACK : COLOR_BLUE;
			produce_mana(player, clr, 1);
		}
	}
	return 0;
}

int card_sealock_monster(int player, int card, event_t event){

	if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && ! is_humiliated(player, card) ){
		if (!basiclandtypes_controlled[1-player][get_hacked_color(player, card, COLOR_BLUE)]){
			event_result = 1;
		}
	}

	if( event == EVENT_BECAME_MONSTROUS ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);

		if( can_target(&td) && pick_target(&td, "TARGET_LAND") ){
			card_instance_t* instance = get_card_instance(player, card);

			instance->number_of_targets = 1;
			add_a_subtype(instance->targets[0].player, instance->targets[0].card, get_hacked_subtype(player, card, SUBTYPE_ISLAND));
			create_targetted_legacy_activate(player, card, &sealock_monster_legacy, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return monstrosity(player, card, event, MANACOST_XU(5, 2), 3);
}

int card_shipbreaker_kraken(int player, int card, event_t event){

	if( event == EVENT_BECAME_MONSTROUS ){
		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);

		instance->number_of_targets = 0;
		int trgs;
		for (trgs = 0; trgs < 4 && can_target(&td) && new_pick_target(&td, "TARGET_CREATURE", trgs, 0); ++trgs){
			state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
		}
		for (trgs = 0; trgs < instance->number_of_targets; ++trgs){
			state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 0);
			does_not_untap_effect(player, card, instance->targets[trgs].player, instance->targets[trgs].card,
								  EDNT_TAP_TARGET|EDNT_REMAIN_TAPPED_UNTIL_SOURCE_LEAVES_PLAY, 0);
		}
	}

	return monstrosity(player, card, event, MANACOST_XU(6, 2), 4);
}

int card_stymied_hopes(int player, int card, event_t event){
	/* Stymied Hopes	|1|U
	 * Instant
	 * Counter target spell unless its controller pays |1. Scry 1. */

	if( event == EVENT_RESOLVE_SPELL ){
		if (counterspell_resolve_unless_pay_x(player, card, NULL, 0, 1)){
			scry(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_COUNTERSPELL, NULL, NULL, 0, NULL);
}

int card_swan_song(int player, int card, event_t event){
	/* Swan Song	|U
	 * Instant
	 * Counter target enchantment, instant, or sorcery spell. Its controller puts a 2/2 |Sblue Bird creature token with flying onto the battlefield. */

	target_definition_t td;
	counterspell_target_definition(player, card, &td, TYPE_ENCHANTMENT | TYPE_SPELL);

	if (event == EVENT_RESOLVE_SPELL){
		if (counterspell_validate(player, card, &td, 0)){
			card_instance_t* instance = get_card_instance(player, card);
			set_flags_when_spell_is_countered(player, card, instance->targets[0].player, instance->targets[0].card);
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);

			token_generation_t token;
			default_token_definition(player, card, CARD_ID_BIRD, &token);
			token.t_player = instance->targets[0].player;
			token.pow = 2;
			token.tou = 2;
			token.color_forced = COLOR_TEST_BLUE;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
		return 0;
	} else {
		return counterspell(player, card, event, &td, 0);
	}
}

int card_thassa_god_of_the_sea(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	indestructible(player, card, event);

	generic_creature_with_devotion(player, card, event, get_sleighted_color(player, card, COLOR_BLUE), 0, 5);

	upkeep_trigger_ability(player, card, event, player);

	if (event == EVENT_UPKEEP_TRIGGER_ABILITY){
		scry(player, 1);
	}

	if( event == EVENT_ACTIVATE ){
		if( current_turn == player && current_phase < PHASE_DECLARE_BLOCKERS && can_target(&td) ){
			ai_modifier+=15;
		}
		else{
			ai_modifier-=50;
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 1, 0, 1, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_thassas_bounty(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			draw_cards(player, 3);
			mill(instance->targets[0].player, 3);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_thassas_emissary(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER) ||
		(instance->damage_target_player > -1 &&
		 damage_dealt_by_me_arbitrary(instance->damage_target_player, instance->damage_target_card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER, player, card) )
	  ){
		draw_cards(player, 1);
	}

	return generic_creature_with_bestow(player, card, event, 5, 0, 1, 0, 0, 0, 3, 3, 0, 0);
}

int card_triton_fortune_hunter(int player, int card, event_t event){

	if( heroic(player, card, event) ){
		draw_cards(player, 1);
	}

	return 0;
}

// Triton Shorethief --> vanilla

static int triton_tactics_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		modify_pt_and_abilities(instance->targets[0].player, instance->targets[0].card, event, 0, 3, 0);
		if (event == EVENT_DECLARE_BLOCKERS && current_turn != player && instance->targets[1].player != 66){
			card_instance_t* blocker = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			if (blocker->blocking != 255){
				card_instance_t* blocked = get_card_instance(1-player, blocker->blocking);
				if (blocked->blocking == 255){	// attacking alone
					does_not_untap_effect(player, card, 1-player, blocker->blocking, EDNT_TAP_AT_END_OF_COMBAT, 1);
				} else {
					int c;
					for (c = 0; c < active_cards_count[1-player]; ++c){	// attacking in a band
						if (in_play(1-player, c) && get_card_instance(1-player, c)->blocking == blocker->blocking){
							does_not_untap_effect(player, card, 1-player, c, EDNT_TAP_AT_END_OF_COMBAT, 1);
						}
					}
				}
				instance->targets[1].player = 66;
			}
		}
	}

	if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_triton_tactics(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				untap_card(instance->targets[i].player, instance->targets[i].card);
				create_targetted_legacy_effect(player, card, &triton_tactics_legacy, instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

// vaporkin --> cloud sprite

int card_voyages_end(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			bounce_permanent(instance->targets[0].player, instance->targets[0].card);
			scry(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_wavecrash_triton(int player, int card, event_t event){

	if( heroic(player, card, event) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1 - player;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t *instance = get_card_instance(player, card);
			instance->number_of_targets = 1;
			effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

// Black
int card_abhorrent_overlord(int player, int card, event_t event){
	/* Abhorrent Overlord	|5|B|B
	 * Creature - Demon 6/6
	 * Flying
	 * When ~ enters the battlefield, put a number of 1/1 |Sblack Harpy creature tokens with flying onto the battlefield equal to your devotion to |Sblack.
	 * At the beginning of your upkeep, sacrifice a creature. */

	upkeep_trigger_ability(player, card, event, player);

	if (event == EVENT_UPKEEP_TRIGGER_ABILITY){
		impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( comes_into_play(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_HARPY, &token);
		token.qty = devotion(player, card, COLOR_BLACK, 0);
		token.color_forced = COLOR_TEST_BLACK;
		token.key_plus = KEYWORD_FLYING;
		generate_token(&token);
	}

	return 0;
}

int card_agent_of_the_fates(int player, int card, event_t event){

	deathtouch(player, card, event);

	if( heroic(player, card, event) ){
		impose_sacrifice(player, card, 1-player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return 0;
}

int card_asphodel_wanderer(int player, int card, event_t event){
	return regeneration(player, card, event, 2, 1, 0, 0, 0, 0);
}

int card_baleful_eidolon(int player, int card, event_t event){
	deathtouch(player, card, event);
	return generic_creature_with_bestow(player, card, event, 4, 1, 0, 0, 0, 0, 1, 1, 0, SP_KEYWORD_DEATHTOUCH);
}

int card_blood_toll_harpy(int player, int card, event_t event){
	if( comes_into_play(player, card, event) ){
		lose_life(player, 1);
		lose_life(1-player, 1);
	}
	return 0;
}

int card_boon_of_erebos(int player, int card, event_t event)
{
	/* Boon of Erebos	|B
	 * Instant
	 * Target creature gets +2/+0 until end of turn. Regenerate it. You lose 2 life. */

	int rval = vanilla_instant_pump(player, card, event, ANYBODY, player, 2,0, VANILLA_PUMP_REGENERATE_DONT_KILL_CARD,0);
	if (rval && event == EVENT_RESOLVE_SPELL){
		lose_life(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}
	return rval;
}

int card_cavern_lampad(int player, int card, event_t event){
	intimidate(player, card, event);
	return generic_creature_with_bestow(player, card, event, 5, 1, 0, 0, 0, 0, 2, 2, 0, SP_KEYWORD_INTIMIDATE);
}

int card_cutthroat_maneuver(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 1, 1, 0, SP_KEYWORD_LIFELINK);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_dark_betrayal(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLACK);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_disciple_of_phenax(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_PLAYER") ){
			ec_definition_t ec;
			default_ec_definition(instance->targets[0].player, player, &ec);
			ec.cards_to_reveal = devotion(player, card, COLOR_BLACK, 0);

			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_ANY);

			new_effect_coercion(&ec, &this_test);
		}
	}
	return 0;
}

int card_erebos_god_of_the_dead(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	indestructible(player, card, event);

	generic_creature_with_devotion(player, card, event, get_sleighted_color(player, card, COLOR_BLACK), 0, 5);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, 0, 1, 1, 0, 0, 0, 0, 2, 0, 0);
}

int card_ereboss_emissary(int player, int card, event_t event){

	/* Erebos's Emissary	|3|B
	 * Enchantment Creature - Snake 3/3
	 * Bestow |5|B
	 * Discard a creature card: ~ gets +2/+2 until end of turn. If ~ is an Aura, enchanted creature gets +2/+2 until end of turn instead.
	 * Enchanted creature gets +3/+3. */

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");
	this_test.zone = TARGET_ZONE_HAND;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			return check_battlefield_for_special_card(player, card, player, 0, &this_test);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 0) ){
			int selected = new_select_a_card(player, player, TUTOR_FROM_HAND, 0, AI_MIN_VALUE, -1, &this_test);
			if( selected != -1 ){
				discard_card(player, selected);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( has_subtype(player, instance->parent_card, SUBTYPE_AURA) && instance->damage_target_player > -1 ){
			pump_until_eot(player, card, instance->damage_target_player, instance->damage_target_card, 2, 2);
		}
		else{
			pump_until_eot(player, card, player, instance->parent_card, 2, 2);
		}
	}

	return generic_creature_with_bestow(player, card, event, 5, 1, 0, 0, 0, 0, 3, 3, 0, 0);
}

// fellhide minotaur --> vanilla

int card_fleshmad_steed(int player, int card, event_t event){
	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if( in_play(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		tap_card(player, card);
	}
	return 0;
}

int card_gray_merchant_of_asphodel(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		int result = lose_life(1-player, devotion(player, card, COLOR_BLACK, 0));
		gain_life(player, result);
	}

	return 0;
}

// hero's downfall --> dreadbore

int card_hythonia_the_cruel(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	deathtouch(player, card, event);

	if( event == EVENT_BECAME_MONSTROUS ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.subtype = SUBTYPE_GORGON;
		this_test.subtype_flag = DOESNT_MATCH;

		new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
	}

	return monstrosity(player, card, event, 6, 2, 0, 0, 0, 0, 3);
}

// insatiable harpy --> child of night

int card_keepsake_gorgon(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.required_subtype = SUBTYPE_GORGON;
	td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

	card_instance_t *instance = get_card_instance(player, card);

	deathtouch(player, card, event);

	if( event == EVENT_BECAME_MONSTROUS ){
		instance->number_of_targets = 0;
		if (can_target(&td) && select_target(player, card, &td, "Select target non-Gorgon creature.", &(instance->targets[0]))){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return monstrosity(player, card, event, 5, 2, 0, 0, 0, 0, 1);
}

int card_lash_of_the_whip(int player, int card, event_t event){
	/* Lash of the Whip	|4|B
	 * Instant
	 * Target creature gets -4/-4 until end of turn. */
	return vanilla_instant_pump(player, card, event, ANYBODY, 1-player, -4, -4, 0, 0);
}

int card_loathsome_catoblepas(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( this_dies_trigger(player, card, event, 2) ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;

		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			alternate_legacy_text(1, instance->damage_source_player,
								  pump_ability_until_eot(instance->damage_source_player, instance->damage_source_card,
														 instance->targets[0].player, instance->targets[0].card, -3, -3, 0, 0));
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		alternate_legacy_text(2, instance->parent_controller,
							  pump_ability_until_eot(player, card, instance->parent_controller, instance->parent_card, 0, 0, 0, SP_KEYWORD_MUST_BE_BLOCKED));
	}

	return generic_activated_ability(player, card, event, 0, 2, 0, 0, 1, 0, 0, 0, 0, 0);
}

// march of the returned --> morbid plunder

int card_mogiss_marauder(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		int max = devotion(player, card, COLOR_BLACK, 0);
		int trgs = 0;
		while( trgs < max && can_target(&td) ){
				if( select_target(player, card, &td, " Select target creature.", &(instance->targets[trgs])) ){
					state_untargettable(instance->targets[trgs].player, instance->targets[trgs].card, 1);
					trgs++;
				}
				else{
					break;
				}
		}
		int i;
		for(i=0; i<trgs; i++){
			state_untargettable(instance->targets[i].player, instance->targets[i].card, 0);
			pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 0, 0, 0, SP_KEYWORD_HASTE+SP_KEYWORD_INTIMIDATE);
		}
	}
	return 0;
}

int card_nighthowler(int player, int card, event_t event){
	if( (event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) ){
		event_result+=count_graveyard_by_type(2, TYPE_CREATURE);
	}
	return generic_creature_with_bestow(player, card, event, 2, 2, 0, 0, 0, 0,
										count_graveyard_by_type(2, TYPE_CREATURE), count_graveyard_by_type(2, TYPE_CREATURE), 0, 0);
}

int card_ordeal_of_erebos(int player, int card, event_t event){

	if (resolve_graveyard_trigger(player, card, event)){
		card_instance_t *instance = get_card_instance(player, card);
		instance->targets[11].player = -1;

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_PLAYER") ){
			new_multidiscard(instance->targets[0].player, 2, 0, player);
		}
		// Avoid confusing generic_aura()
		instance->targets[0].player = instance->damage_target_player;
		instance->targets[0].card = instance->damage_target_card;
	}

	return ordeal(player, card, event);
}

// Pharika's Cure --> vicious hunger

int card_read_the_bones(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		scry(player, 2);
		draw_cards(player, 2);
		lose_life(player, 2);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int rescue_from_the_underworld_legacy(int player, int card, event_t event){
	if (current_turn == player && upkeep_trigger(player, card, event) && count_upkeeps(player) > 0){
		card_instance_t* leg = get_card_instance(player, card);

		// First the targeted creature
		int selected = validate_target_from_grave(player, card, player, 0);
		if (selected != -1){
			reanimate_permanent(player, card, player, selected, REANIMATE_DEFAULT);
			if (leg->targets[2].player == player
				&& leg->targets[0].player < leg->targets[1].player){
				// This will almost always be true, since the targeted creature was already in the grave before the sacrifice
				--leg->targets[1].player;
			}
		}

		if (leg->targets[1].player >= 0){
			selected = validate_target_from_grave(player, card, leg->targets[2].player, 1);
			if (selected != -1){
				int card_maybe_added = card;
				if (leg->targets[2].player != player){
					// reanimate_permanent() will create an empty effect card attached to cards animated from opponent's graveyard; make sure it's not blank
					card_maybe_added = add_card_to_hand(player, get_internal_card_id_from_csv_id(CARD_ID_RESCUE_FROM_THE_UNDERWORLD));
					get_card_instance(player, card_maybe_added)->state |= STATE_INVISIBLE;
					--hand_count[player];
				}
				reanimate_permanent(player, card_maybe_added, leg->targets[2].player, selected, REANIMATE_DEFAULT);
				if (card_maybe_added != card){
					obliterate_card(player, card_maybe_added);
				}
			}
		}

		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_rescue_from_the_underworld(int player, int card, event_t event){

	/* Rescue from the Underworld	|4|B
	 * Instant
	 * As an additional cost to cast ~, sacrifice a creature.
	 * Choose target creature card in your graveyard. Return that card and the sacrificed card to the battlefield under your control at the beginning of your
	 * next upkeep. Exile ~. */

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, "Select target creature card.");
	this_test.ai_selection_mode = AI_MAX_CMC;

	card_instance_t *instance = get_card_instance( player, card );

	if(event == EVENT_CAN_CAST ){
		if( can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return generic_spell(player, card, event, GS_GRAVE_RECYCLER, NULL, NULL, 0, &this_test);
		}
	}

	if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
		if( select_target_from_grave_source(player, card, player, 0, AI_MAX_CMC, &this_test, 1) != -1 ){
			int selected = pick_creature_for_sacrifice(player, card, 0);
			instance->number_of_targets = 0;
			if( selected != -1 ){
				card_instance_t* sac = get_card_instance(player, selected);
				instance->targets[2].player = (sac->state & STATE_OWNED_BY_OPPONENT) ? 1 : 0;
				instance->targets[2].card = cards_data[sac->original_internal_card_id].id;
				kill_card(player, selected, KILL_SACRIFICE);
				return 0;
			}
		}
		spell_fizzled = 1;
	}

	if(event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, player, 1);
		if( selected != -1 ){
			int legacy = create_legacy_effect(player, card, &rescue_from_the_underworld_legacy);
			card_instance_t *leg = get_card_instance( player, legacy );
			leg->targets[0].player = selected;
			leg->targets[0].card = get_grave(player)[selected];

			selected = seek_grave_for_id_to_reanimate(player, card, instance->targets[2].player, instance->targets[2].card, REANIMATEXTRA_LEAVE_IN_GRAVEYARD);
			if (selected == -1){
				leg->targets[1].player = leg->targets[1].card = -1;
			} else {
				leg->targets[1].player = selected;
				leg->targets[1].card = get_grave(instance->targets[2].player)[selected];
				leg->targets[2].player = instance->targets[2].player;
			}
			kill_card(player, card, KILL_REMOVE);
		} else {
			kill_card(player, card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_returned_centaur(int player, int card, event_t event){
	if (comes_into_play(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, 0);
		td.zone = TARGET_ZONE_PLAYERS;
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_PLAYER")){
			card_instance_t* instance = get_card_instance(player, card);
			mill(instance->targets[0].player, 4);
		}
	}
	return 0;
}

int card_returned_phalanx(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_ACTIVATION ){
		card_instance_t* instance = get_card_instance(player, card);
		create_targetted_legacy_effect(player, instance->parent_card, &effect_defender_can_attack_until_eot, player, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XU(1, 1), 0, 0, 0);
}

int card_scourgemark(int player, int card, event_t event){
	if (comes_into_play(player, card, event)){
		draw_cards(player, 1);
	}

	return generic_aura(player, card, event, player, 1, 0, 0, 0, 0, 0, 0);
}

int card_sip_of_hemlock(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			lose_life(instance->targets[0].player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_tormented_hero(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	if( heroic(player, card, event) ){
		int result = lose_life(1-player, 1);
		gain_life(player, result);
	}

	return 0;
}

int card_vipers_kiss(int player, int card, event_t event){
	return generic_aura(player, card, event, 1 - player, -1, -1, 0, 0, 0, 0, GA_FORBID_ALL_ACTIVATED_ABILITIES);
}

int card_whip_of_erebos(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	damage_effects(player, card, event);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_CAN_ACTIVATE ){
		if (generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_SORCERY_BE_PLAYED, 2, 2, 0, 0, 0, 0, 0, 0, 0)
			&& count_graveyard_by_type(player, TYPE_CREATURE) > 0
			&& !graveyard_has_shroud(2)){
			return 1;
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, 2, 2, 0, 0, 0, 0) ){
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card.");

			if( new_select_target_from_grave(player, card, player, 0, AI_MAX_CMC, &this_test, 0) != -1 ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int selected = validate_target_from_grave(player, card, player, 0);
		if( selected != -1 ){
			card_instance_t* instance = get_card_instance(player, card);
			reanimate_permanent(player, instance->parent_card, player, selected, REANIMATE_UNEARTH);
		}
	}

	if (event == EVENT_ABILITIES
		&& affected_card_controller == player
		&& is_what(affected_card_controller, affected_card, TYPE_CREATURE)
		&& in_play(player, card) && !is_humiliated(player, card)
	   ){
		lifelink(affected_card_controller, affected_card, event);
	}

	return global_enchantment(player, card, event);
}

// Red

int card_akroan_crusader(int player, int card, event_t event){
	/* Akroan Crusader	|R
	 * Creature - Human Soldier 1/1
	 * Heroic - Whenever you cast a spell that targets ~, put a 1/1 |Sred Soldier creature token with haste onto the battlefield. */

	if( heroic(player, card, event) ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SOLDIER, &token);
		token.pow = 1;
		token.tou = 1;
		token.s_key_plus = SP_KEYWORD_HASTE;
		token.color_forced = COLOR_TEST_RED;
		generate_token(&token);
	}

	return 0;
}

int card_anger_of_the_gods(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		new_damage_all(player, card, 2, 3, NDA_EXILE_IF_FATALLY_DAMAGED | NDA_ALL_CREATURES, NULL);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_arena_athlete(int player, int card, event_t event){
	if (heroic(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.allow_cancel = 0;

		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
			card_instance_t* instance = get_card_instance(player, card);

			instance->number_of_targets = 1;
			pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_CANNOT_BLOCK);
		}
	}

	return 0;
}

// borderland minotaur --> vanilla

int card_boulderfall(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_CAN_CAST ){
		return can_target(&td);
	}

	else if(event == EVENT_CAST_SPELL && affect_me(player, card) ){
			instance->number_of_targets = 0;
			int trgs;
			for (trgs = 0; trgs < 5; ++trgs){
				if (!new_pick_target(&td, "TARGET_CREATURE_OR_PLAYER", trgs, 1)){
					return 0;
				}
			}
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			divide_damage(player, card, &td);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_coordinated_assault(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				pump_ability_until_eot(player, card, instance->targets[i].player, instance->targets[i].card, 1, 0, KEYWORD_FIRST_STRIKE, 0);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

int card_deathbellow_raider(int player, int card, event_t event){
	attack_if_able(player, card, event);
	return regeneration(player, card, event, 2, 1, 0, 0, 0, 0);
}

int card_dragon_mantle(int player, int card, event_t event){
	/* Dragon Mantle	|R
	 * Enchantment - Aura
	 * Enchant creature
	 * When ~ enters the battlefield, draw a card.
	 * Enchanted creature has "|R: This creature gets +1/+0 until end of turn." */

	if (IS_GAA_EVENT(event)){
		card_instance_t *instance = get_card_instance(player, card);

		if( instance->damage_target_player > -1 ){
			int t_player = instance->damage_target_player;
			int t_card = instance->damage_target_card;
			if( event == EVENT_RESOLVE_ACTIVATION ){
				pump_until_eot_merge_previous(player, card, t_player, t_card, 1, 0);
			}
			return granted_generic_activated_ability(player, card, t_player, t_card, event, 0,
													 MANACOST_R(1), 0, NULL, NULL);
		}
	}

	if (comes_into_play(player, card, event)){
		draw_cards(player, 1);
	}

	return vanilla_aura(player, card, event, player);
}

int card_ember_swallower(int player, int card, event_t event){

	if( event == EVENT_BECAME_MONSTROUS ){
		impose_sacrifice(player, card, player, 3, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		impose_sacrifice(player, card, 1-player, 3, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	return monstrosity(player, card, event, 5, 0, 0, 0, 2, 0, 3);
}

int card_fanatic_of_mogis(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		damage_player(1-player, devotion(player, card, COLOR_RED, 0), player, card);
	}

	return 0;
}

int card_firedrinker_satyr(int player, int card, event_t event){
	if (damage_dealt_to_me_arbitrary(player, card, event, 0, player, card)){
		card_instance_t* instance = get_card_instance(player, card);
		damage_player(player, instance->targets[7].card, player, card);
	}

	if (event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);
		pump_until_eot(player, instance->parent_card, player, instance->parent_card, 1, 0);
		damage_player(player, 1, player, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, 0, MANACOST_XR(1,1), 0, NULL, NULL);
}

int card_flamespeaker_adept(int player, int card, event_t event)
{
  /* Flamespeaker Adept	|2|R
   * Creature - Human Shaman 2/3
   * Whenever you scry, ~ gets +2/+0 and gains first strike until end of turn. */

  if (xtrigger_condition() == XTRIGGER_SCRY && affect_me(player, card) && reason_for_trigger_controller == player
	  && trigger_cause_controller == player && !is_humiliated(player, card))
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		pump_ability_until_eot(player, card, player, card, 2, 0, KEYWORD_FIRST_STRIKE, 0);
	}

  return 0;
}

int card_hammer_of_purphoros(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	boost_subtype(player, card, event, -1, 0,0, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, MANACOST_XR(2, 1), 0, 0 ,0) ){
			return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		get_card_instance(player, card)->number_of_targets = 0;
		if( charge_mana_for_activated_ability(player, card, MANACOST_XR(2, 1)) ){
			if( sacrifice(player, card, player, 0, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GOLEM, &token);
		token.special_flags2 = SF2_ENCHANTED_EVENING;
		generate_token(&token);
	}

	return global_enchantment(player, card, event);
}

int card_ill_tempered_cyclops(int player, int card, event_t event){
	return monstrosity(player, card, event, 5, 0, 0, 0, 1, 0, 3);
}

int card_labyrinth_champion(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( heroic(player, card, event) && can_target(&td) ){
		if( pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			instance->number_of_targets = 1;
			damage_creature_or_player(player, card, event, 2);
		}
	}

	return 0;
}

// lightning striker --> lighting bolt

int card_messengers_speed(int player, int card, event_t event){
	return generic_aura(player, card, event, player, 0, 0, KEYWORD_TRAMPLE, SP_KEYWORD_HASTE, 0, 0, 0);
}

int card_minotaur_skullcleaver(int player, int card, event_t event){
	haste(player, card, event);
	if( comes_into_play(player, card, event) ){
		pump_until_eot(player, card, player, card, 2, 0);
	}
	return 0;
}

int card_ordeal_of_purphoros(int player, int card, event_t event){//UNUSEDCARD

	if (resolve_graveyard_trigger(player, card, event)){
		card_instance_t* instance = get_card_instance(player, card);
		instance->targets[11].player = -1;

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;
		td.allow_cancel = 0;

		instance->number_of_targets = 0;
		if( can_target(&td) && pick_target(&td, "TARGET_CREATURE_OR_PLAYER") ){
			damage_creature_or_player(player, card, event, 3);
		}
		// Avoid confusing generic_aura()
		instance->targets[0].player = instance->damage_target_player;
		instance->targets[0].card = instance->damage_target_card;
	}

	return ordeal(player, card, event);
}

int card_peak_eruption(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.required_subtype = get_hacked_subtype(player, card, SUBTYPE_MOUNTAIN);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			damage_player(instance->targets[0].player, 3, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_LAND", 1, NULL);
}

int card_portent_of_betrayal(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( instance->targets[0].player == player ){
				untap_card(instance->targets[0].player, instance->targets[0].card);
				pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
			}
			else{
				effect_act_of_treason(player, card, instance->targets[0].player, instance->targets[0].card);
			}
			scry(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_priest_of_iroas(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME+GAA_CAN_TARGET, 3, 0, 0, 0, 0, 1, 0, &td, "TARGET_ENCHANTMENT");
}

int card_purphoros_god_of_the_forge(int player, int card, event_t event){

	test_definition_t this_test;
	default_test_definition(&this_test, TYPE_CREATURE);
	this_test.not_me = 1;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	indestructible(player, card, event);

	generic_creature_with_devotion(player, card, event, get_sleighted_color(player, card, COLOR_RED), 0, 5);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( new_specific_cip(player, card, event, player, 2, &this_test) ){
		damage_player(1-player, 2, player, card);
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		pump_subtype_until_eot(player, instance->parent_card, player, -1, 1, 0, 0, 0);
	}

	return generic_activated_ability(player, card, event, 0, 2, 0, 0, 0, 1, 0, 0, 0, 0);
}

int card_purphoross_emissary(int player, int card, event_t event){
	/* Purphoros's Emissary	|3|R
	 * Enchantment Creature - Ox 3/3
	 * Bestow |6|R
	 * Menace
	 * Enchanted creature gets +3/+3 and has menace. */

	card_instance_t *instance = get_card_instance( player, card );

	menace(player, card, event);

	if( instance->damage_target_player > -1 ){
		menace(instance->damage_target_player, instance->damage_target_card, event);
	}

	return generic_creature_with_bestow(player, card, event, 6, 0, 0, 0, 1, 0, 3, 3, 0, 0);
}

int card_rage_of_purphoros(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			cannot_regenerate_until_eot(player, card, instance->targets[0].player, instance->targets[0].card);
			damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, card);
			scry(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_rageblood_shaman(int player, int card, event_t event){
	return boost_creature_type(player, card, event, SUBTYPE_MINOTAUR, 1, 1, KEYWORD_TRAMPLE, BCT_CONTROLLER_ONLY);
}

// Satyr rambler --> vanilla

int card_spark_jolt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature_or_player(player, card, event, 1);
			scry(player, 1);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE_OR_PLAYER", 1, NULL);
}

int card_spearpoint_oread(int player, int card, event_t event){
	return generic_creature_with_bestow(player, card, event, 5, 0, 0, 0, 1, 0, 2, 2, KEYWORD_FIRST_STRIKE, 0);
}

static int effect_creatures_without_flying_cant_block_this_turn(int player, int card, event_t event)
{
  if (event == EVENT_BLOCK_LEGALITY && affected_card_controller != player && !check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING))
	event_result = 1;

  if ((event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY) && affect_me(player, card))
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_stoneshock_giant(int player, int card, event_t event)
{
  if (event == EVENT_BECAME_MONSTROUS)
	create_legacy_effect(player, card, effect_creatures_without_flying_cant_block_this_turn);

  return monstrosity(player, card, event, MANACOST_XR(6, 2), 3);
}

int card_stormbreath_dragon(int player, int card, event_t event){

	haste(player, card, event);

	if( event == EVENT_BECAME_MONSTROUS ){
		if( hand_count[1-player] > 0 ){
			damage_player(1-player, hand_count[1-player], player, card);
		}
	}

	return monstrosity(player, card, event, 5, 0, 0, 0, 2, 0, 3);
}

int card_titan_of_eternal_fire(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_state = TARGET_STATE_TAPPED | TARGET_STATE_SUMMONING_SICK;
	td.required_subtype = SUBTYPE_HUMAN;
	td.illegal_abilities = 0;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE ){
		if (!is_humiliated(player, card)	// the humans have the activated ability, not the Titan
			&& has_mana_for_activated_ability(player, card, MANACOST_R(1))){
			int p = player;
			int c;
			for(c = 0; c<active_cards_count[p]; c++){
				if( in_play(p, c) && is_what(p, c, TYPE_CREATURE) && ! is_tapped(p, c) && ! is_sick(p, c) && has_subtype(p, c, SUBTYPE_HUMAN) && can_use_activated_abilities(p, c)){
					td1.player = p;
					td1.card = c;
					td1.illegal_abilities = get_protections_from(p, c);

					if( can_target(&td1) ){
						return 1;
					}
				}
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if (select_target(player, card, &td, "Select a Human creature you control.", &instance->targets[0])){
			int p = instance->targets[0].player;
			int c = instance->targets[0].card;
			if (can_use_activated_abilities(p, c)
				&& charge_mana_for_activated_ability(p, c, MANACOST_R(1))){
				td1.player = p;
				td1.card = c;
				td1.illegal_abilities = get_protections_from(p, c);

				load_text(0, "TARGET_CREATURE_OR_PLAYER");
				if (can_target(&td1) && select_target(player, card, &td1, text_lines[0], &instance->targets[1])){
					tap_card(p, c);
					return 0;
				}
			}
		}
		spell_fizzled = 1;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		td1.player = instance->targets[0].player;
		td1.card = instance->targets[0].card;
		td1.illegal_abilities = get_protections_from(instance->targets[0].player, instance->targets[0].card);

		if (validate_arbitrary_target(&td1, instance->targets[1].player, instance->targets[1].card)){
			damage_creature(instance->targets[1].player, instance->targets[1].card, 1, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_titans_strength(int player, int card, event_t event){
	/* Titan's Strength	|R
	 * Instant
	 * Target creature gets +3/+1 until end of turn. Scry 1. */
	return vanilla_instant_pump(player, card, event, ANYBODY, player, 3, 1, VANILLA_PUMP_SCRY_1, 0);
}

// two-headed cerberus --> vanilla

// wild celebrants --> batterhorn

// Green

int card_agent_of_horizons(int player, int card, event_t event){
	return generic_shade(player, card, event, 0, 2, 0, 1, 0, 0, 0, 0, 0, 0, SP_KEYWORD_UNBLOCKABLE);
}

int card_anthousa_setessan_hero(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if (heroic(player, card, event)){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_LAND);
		td.illegal_type = TYPE_CREATURE;
		td.preferred_controller = player;
		td.allowed_controller = player;

		card_instance_t *instance = get_card_instance(player, card);

		int trgs;
		for (trgs = 0; trgs < 3 && can_target(&td) && pick_target(&td, "TARGET_LAND"); ++trgs){
			instance->number_of_targets = 1;
			land_animation2(player, card, instance->targets[0].player, instance->targets[0].card, 1, 2, 2, 0, 0, 0, NULL);
		}
	}

	return 0;
}

int card_arbor_colossus(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = 1-player;
	td.required_abilities = KEYWORD_FLYING;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_BECAME_MONSTROUS){
		if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return monstrosity(player, card, event, 3, 0, 0, 3, 0, 0, 3);
}

int card_artisans_sorrow(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			scry(player, 2);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "DISENCHANT", 1, NULL);
}

int card_boon_satyr(int player, int card, event_t event){
	return generic_creature_with_bestow(player, card, event, 3, 0, 0, 2, 0, 0, 4, 2, 0, 0);
}

int card_bow_of_nylea(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if (event == EVENT_ABILITIES){
		if (affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE)
			&& is_attacking(affected_card_controller, affected_card) && in_play(player, card)){
			deathtouch(affected_card_controller, affected_card, event);
		}
	}

	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		target_definition_t td_counter;
		default_target_definition(player, card, &td_counter, TYPE_CREATURE);
		td_counter.preferred_controller = player;

		target_definition_t td_flying;
		default_target_definition(player, card, &td_flying, TYPE_CREATURE);
		td_flying.required_abilities = KEYWORD_FLYING;

		card_instance_t* instance = get_card_instance(player, card);

		typedef enum{
			CHOICE_COUNTER = 1,
			CHOICE_DAMAGE = 2,
			CHOICE_LIFE = 3,
			CHOICE_GRAVEYARD = 4
		} Choice;

		Choice choice = DIALOG(player, card, event,
							   "+1/+1 counter", can_target(&td_counter), 3,
							   "Damage a flying creature", can_target(&td_flying), 4,
							   "Gain 3 life", 1, life[player] < 6 ? 5 : 2,
							   "Recycle graveyard", !graveyard_has_shroud(player), count_graveyard(player) > 0 && count_deck(player) < 2 ? 6 : 1);

		if (event == EVENT_CAN_ACTIVATE){
			return choice && generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, MANACOST_XG(1, 1), 0, 0, 0);
		} else if (event == EVENT_ACTIVATE){
			instance->number_of_targets = 0;

			if (charge_mana_for_activated_ability(player, card, MANACOST_XG(1, 1))){
				switch (choice){
					case CHOICE_COUNTER:	pick_target(&td_counter, "TARGET_CREATURE");	break;
					case CHOICE_DAMAGE:		pick_target(&td_flying, "TARGET_CREATURE");		break;
					case CHOICE_LIFE:		break;
					case CHOICE_GRAVEYARD:
						select_multiple_cards_from_graveyard(player, player, 0, AI_MAX_VALUE, NULL, 4, &instance->targets[0]);
						break;
				}
				if (spell_fizzled != 1){
					tap_card(player, card);
				}
			}
		} else {	// EVENT_RESOLVE_ACTIVATION
			switch (choice){
				case CHOICE_COUNTER:
					if (valid_target(&td_counter)){
						add_1_1_counter(instance->targets[0].player, instance->targets[0].card);
					}
					break;

				case CHOICE_DAMAGE:
					if (valid_target(&td_flying)){
						damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, instance->parent_card);
					}
					break;

				case CHOICE_LIFE:
					gain_life(player, 3);
					break;

				case CHOICE_GRAVEYARD:{
					int i, num_validated = 0;
					for (i = 0; i < 4; ++i){
						int selected = validate_target_from_grave(player, card, player, i);
						if (selected != -1){
							from_graveyard_to_deck(player, selected, 2);
							++num_validated;
						}
					}
					if (num_validated > 1){
						rearrange_bottom_x(player, player, num_validated);
					}
					break;
				}
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_centaur_battlemaster(int player, int card, event_t event){

	if( heroic(player, card, event) ){
		add_1_1_counters(player, card, 3);
	}

	return 0;
}

int card_commune_with_the_gods(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		int amount = 5;
		if( amount > count_deck(player) ){
			amount = count_deck(player);
		}

		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_ENCHANTMENT, "Select a creature or enchantment card.");
		this_test.create_minideck = amount;
		this_test.no_shuffle = 1;

		if( amount > 0 ){
			if( player == AI ){
				show_deck( HUMAN, deck_ptr[player], amount, "Cards revealed by Commune with the Gods.", 0, 0x7375B0 );
			}
			if( new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test) != -1 ){
				amount--;
			}
			if( amount > 0 ){
				mill(player, amount);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

static int defend_the_hearth_legacy(int player, int card, event_t event ){

	if (event == EVENT_PREVENT_DAMAGE){
		card_instance_t* damage = get_card_instance(affected_card_controller, affected_card);
		if (damage_card == damage->internal_card_id
			&& damage->info_slot > 0
			&& damage->damage_target_card == -1
			&& (damage->token_status & (STATUS_COMBAT_DAMAGE | STATUS_FIRST_STRIKE_DAMAGE))
			&& !(is_what(damage->damage_source_player, damage->damage_source_card, TYPE_CREATURE)
				 && check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER))){
			damage->info_slot = 0;
		}
	}

	if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_defend_the_hearth(int player, int card, event_t event){

	if(event == EVENT_RESOLVE_SPELL ){
		create_legacy_effect(player, card, defend_the_hearth_legacy);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_fade_into_antiquity(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "DISENCHANT", 1, NULL);
}

int card_feral_invocation(int player, int card, event_t event){
	flash(player, card, event);
	return generic_aura(player, card, event, player, 2, 2, 0, 0, 0, 0, 0);
}

int card_hunt_the_hunter(int player, int card, event_t event){
	/* Hunt the Hunter	|G
	 * Sorcery
	 * Target |Sgreen creature you control gets +2/+2 until end of turn. It fights target |Sgreen creature an opponent controls. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.required_color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;
	td1.required_color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td, "TARGET_CREATURE") ){
			new_pick_target(&td1, "TARGET_CREATURE", 1, 1);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) ){
			pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 2, 2);
			if ( validate_target(player, card, &td1, 1) ){
				fight(instance->targets[0].player, instance->targets[0].card, instance->targets[1].player, instance->targets[1].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_karametras_acolyte(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE && ! is_tapped(player, card) && ! is_sick(player, card) ){
		return can_produce_mana(player, card);
	}

	if( event == EVENT_ACTIVATE ){
		produce_mana_tapped(player, card, COLOR_GREEN, devotion(player, card, COLOR_GREEN, 0));
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( can_produce_mana(player, card) ){
			declare_mana_available(player, COLOR_GREEN, devotion(player, card, COLOR_GREEN, 0));
		}
	}

	return 0;
}

int card_leafcrown_dryad(int player, int card, event_t event){
	return generic_creature_with_bestow(player, card, event, 3, 0, 0, 1, 0, 0, 2, 2, KEYWORD_REACH, 0);
}

int card_mistcutter_hydra(int player, int card, event_t event){
	cannot_be_countered(player, card, event);
	haste(player, card, event);
	return card_ivy_elemental(player, card, event);
}

int card_nemesis_of_mortals(int player, int card, event_t event){

	if (event == EVENT_MODIFY_COST){
		COST_COLORLESS -= count_graveyard_by_type(player, TYPE_CREATURE);
	}

	if (event == EVENT_ACTIVATE || event == EVENT_CAN_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		int amount = 7 - count_graveyard_by_type(player, TYPE_CREATURE);
		if (amount < 0){
			amount = 0;
		}
		return monstrosity(player, card, event, MANACOST_XG(amount, 2), 5);
	}

	return 0;
}

int card_nessian_asp(int player, int card, event_t event){
	return monstrosity(player, card, event, 6, 0, 0, 1, 0, 0, 4);
}

int card_nylea_god_of_the_hunt(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	indestructible(player, card, event);

	generic_creature_with_devotion(player, card, event, get_sleighted_color(player, card, COLOR_GREEN), 0, 5);

	boost_creature_type(player, card, event, -1, 0, 0, KEYWORD_TRAMPLE, BCT_CONTROLLER_ONLY);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 2, 2);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, 3, 0, 0, 1, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_nyleas_disciple(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		gain_life(player, devotion(player, card, COLOR_GREEN, 0));
	}

	return 0;
}

int card_nyleas_emissary(int player, int card, event_t event){
	return generic_creature_with_bestow(player, card, event, 5, 0, 0, 1, 0, 0, 3, 3, KEYWORD_TRAMPLE, 0);
}

int card_nyleas_presence(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);
	td.allowed_controller = player;	// Approximation
	td.preferred_controller = player;

	card_instance_t *instance= get_card_instance(player, card);

	if (comes_into_play(player, card, event)){
		draw_cards(player, 1);
	}

	if( in_play(player, card) && instance->damage_target_player > -1 ){
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;
		if( event == EVENT_CAN_ACTIVATE ){
			if( ! is_tapped(p, c) && ! is_animated_and_sick(p, c) && can_produce_mana(p, c) ){
				return 1;
			}
		}

		if( event == EVENT_ACTIVATE ){
			produce_mana_tapped_all_one_color(p, c, COLOR_TEST_ANY_COLORED, 1);
			dispatch_event(p, c, EVENT_TAP_CARD);
		}

		if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
			if( ! is_tapped(p, c) && ! is_animated_and_sick(p, c) && can_produce_mana(p, c) ){
				mana_producer(player, card, event);
			}
		}

		set_special_flags2(p, c, SF2_PRISMATIC_OMEN);


		if( leaves_play(player, card, event)){
			remove_special_flags2(p, c, SF2_PRISMATIC_OMEN);
		}
	}

	return targeted_aura(player, card, event, &td, "TARGET_LAND");
}

int card_ordeal_of_nylea(int player, int card, event_t event){

	/* Ordeal of Nylea	|1|G
	 * Enchantment - Aura
	 * Enchant creature
	 * Whenever enchanted creature attacks, put a +1/+1 counter on it. Then if it has three or more +1/+1 counters on it, sacrifice ~.
	 * When you sacrifice ~, search your library for up to two basic land cards, put them onto the battlefield tapped, then shuffle your library. */

	if (resolve_graveyard_trigger(player, card, event)){
		get_card_instance(player, card)->targets[11].player = -1;

		tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 2);
	}

	return ordeal(player, card, event);
}

// pheres-band centaur --> vanilla

int card_polukranos_world_eater(int player, int card, event_t event){

	card_instance_t* instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if (event == EVENT_BECAME_MONSTROUS){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.preferred_controller = 1-player;
		td.allow_cancel = 0;

		int i = 0;
		while (i < instance->info_slot && can_target(&td)){
				int pos = instance->number_of_targets;
				char buffer[100];
				scnprintf(buffer, 100, "Select target creature an opponent controls (%d of %d).", i+1, instance->info_slot);
				if( new_pick_target(&td, buffer, pos, GS_LITERAL_PROMPT) ){
					add_state(instance->targets[pos].player, instance->targets[pos].card, STATE_TARGETTED);
					i++;
				}
		}

		for (i = 0; i < active_cards_count[1 - player]; ++i){
			if (in_play(1 - player, i) && is_what(1 - player, i, TYPE_CREATURE) && check_state(1 - player, i, STATE_TARGETTED)){
				remove_state(1 - player, i, STATE_TARGETTED);
				int q, dmg = 0;
				for(q=0; q<instance->number_of_targets; q++){
					if( instance->targets[q].card == i ){
						++dmg;
					}
				}
				damage_creature(1 - player, i, dmg, player, card);
				damage_creature(player, card, get_power(1 - player, i), 1 - player, i);
			}
		}
	}

	return monstrosity(player, card, event, MANACOST_XG(-2, 1), instance->info_slot);
}

int card_reverent_hunter(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		add_1_1_counters(player, card, devotion(player, card, COLOR_GREEN, 0));
	}

	return 0;
}

int card_satyr_hedonist(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) && instance->targets[1].player != 66 && has_mana(player, COLOR_RED, 1) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}

	if( event == EVENT_ACTIVATE ){
		instance->targets[1].player = 66;
		charge_mana(player, COLOR_RED, 1);
		if( spell_fizzled != 1 ){
			produce_mana(player, COLOR_RED, 3);
			tapped_for_mana_color = -2;
			kill_card(player, card, KILL_SACRIFICE);
		}
		instance->targets[1].player = 0;
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card) && instance->targets[1].player != 66 &&
		has_mana(player, COLOR_RED, 1)
	  ){
		declare_mana_available(player, COLOR_RED, 3);
	}

	return 0;
}

int card_satyr_piper(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_MUST_BE_BLOCKED);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST_XG(3, 1), 0, &td, "TARGET_CREATURE");
}

// sedge scorpion --> Deathgaze Cockatrice

int card_shredding_winds(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 7, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_staunch_hearted_warrior(int player, int card, event_t event){

	/* Pheres-Band Thunderhoof	|4|G
	 * Creature - Centaur Warrior 3/4
	 * Heroic - Whenever you cast a spell that targets ~, put two +1/+1 counters on ~. */

	/* Setessan Oathsworn	|1|G|G
	 * Creature - Satyr Warrior 1/1
	 * Heroic - Whenever you cast a spell that targets ~, put two +1/+1 counters on ~. */

	/* Staunch-Hearted Warrior	|3|G
	 * Creature - Human Warrior 2/2
	 * Heroic - Whenever you cast a spell that targets ~, put two +1/+1 counters on ~. */


	if( heroic(player, card, event) ){
		add_1_1_counters(player, card, 2);
	}

	return 0;
}

int card_sylvan_caryatid(int player, int card, event_t event){
	hexproof(player, card, event);
	return mana_producing_creature_all_one_color(player, card, event, 12, COLOR_TEST_ANY_COLORED, 1);
}

static int time_to_feed_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 && graveyard_from_play(instance->targets[0].player, instance->targets[0].card, event) ){
		gain_life(player, 3);
		kill_card(player, card, KILL_REMOVE);
	}

	if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_time_to_feed(int player, int card, event_t event){
	/* Time to Feed	|2|G
	 * Sorcery
	 * Choose target creature an opponent controls. When that creature dies this turn, you gain 3 life. Target creature you control fights that creature. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = 1-player;
	td1.preferred_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( pick_target(&td1, "TARGET_CREATURE") ){
			new_pick_target(&td, "TARGET_CREATURE", 1, 1);
		}
	}

	if(event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td1, 0) ){
			create_targetted_legacy_effect(player, card, &time_to_feed_legacy, instance->targets[0].player, instance->targets[0].card);
			if( validate_target(player, card, &td, 1) ){
				fight(instance->targets[1].player, instance->targets[1].card, instance->targets[0].player, instance->targets[0].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// voyaging satyr --> seeker of skybreak

// vulpine goliath --> vanilla

static int effect_warriors_lesson(int player, int card, event_t event)
{
  if (IS_DDBM_EVENT(event))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int packets;
	  if (instance->damage_target_card >= 0 && !is_humiliated(instance->damage_target_player, instance->damage_target_card)
		  && (packets = equipped_creature_deals_damage(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER)))
		draw_cards(player, packets);
	}

  if (event == EVENT_CLEANUP || event == EVENT_SHOULD_AI_PLAY)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}
int card_warriors_lesson(int player, int card, event_t event){

	/* Warriors' Lesson	|G
	 * Instant
	 * Until end of turn, up to two target creatures you control each gain "Whenever this creature deals combat damage to a player, draw a card." */

	if (!IS_GS_EVENT(player, card, event)){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if (event == EVENT_RESOLVE_SPELL){
		int i;
		for (i = 0; i < instance->number_of_targets; i++){
			if( validate_target(player, card, &td, i) ){
				create_targetted_legacy_effect(player, card, &effect_warriors_lesson, instance->targets[i].player, instance->targets[i].card);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_OPTIONAL_TARGET, &td, "TARGET_CREATURE", 2, NULL);
}

// Gold
int card_akroan_hoplite(int player, int card, event_t event)
{
  // Whenever ~ attacks, it gets +X/+0 until end of turn, where X is the number of attacking creatures you control.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	pump_until_eot(player, card, player, card, count_attackers(player),0);

  return 0;
}

int card_anax_and_cymede(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	vigilance(player, card, event);

	if( heroic(player, card, event) ){
		pump_subtype_until_eot(player, card, player, -1, 1, 1, KEYWORD_TRAMPLE, 0);
	}

	return 0;
}

int card_ashen_rider(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( this_dies_trigger(player, card, event, 2) ){
		if (can_target(&td) && pick_target(&td, "TARGET_PERMANENT")){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	if (comes_into_play(player, card, event) && can_target(&td) && pick_target(&td, "TARGET_PERMANENT")){
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
	}

	return 0;
}

static void ashiok_exiledby_remember(int player, int card, int t_player, int iid, int* ret_leg, int* ret_idx)
{
  if (is_what(-1, iid, TYPE_CREATURE))
	exiledby_remember(player, card, t_player, iid, ret_leg, ret_idx);
}

int card_ashiok_nightmare_weaver(int player, int card, event_t event)
{
  /* Ashiok, Nightmare Weaver	|1|U|B
   * Planeswalker - Ashiok (3)
   * +2: Exile the top three cards of target opponent's library.
   * -X: Put a creature card with converted mana cost X exiled with ~ onto the battlefield under your control. That creature is a Nightmare in addition to its
   * other types.
   * -10: Exile all cards from all opponents' hands and graveyards. */

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	ai_modifier += 48;

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  int loyalty = count_counters(player, card, COUNTER_LOYALTY);

	  target_definition_t td_opponent;
	  default_target_definition(player, card, &td_opponent, 0);
	  td_opponent.zone = TARGET_ZONE_PLAYERS;
	  td_opponent.allowed_controller = 1 - player;

	  int can_library = can_target(&td_opponent);
	  int priority_library = 50;

	  int priority_nightmare = 1, can_nightmare;
	  if (player == AI || ai_is_speculating == 1)
		{
		  priority_nightmare = exiledby_choose(player, card, CARD_ID_ASHIOK_NIGHTMARE_WEAVER, EXBY_MAX_VALUE|EXBY_TEST_CMC_LE, loyalty, "creature", 1);
		  can_nightmare = priority_nightmare > INT_MIN;
		}
	  else
		can_nightmare = exiledby_choose(player, card, CARD_ID_ASHIOK_NIGHTMARE_WEAVER, EXBY_FIRST_FOUND|EXBY_TEST_CMC_LE, loyalty, "creature", 1);

	  int priority_hand_gy = 200;

	  enum
	  {
		CHOICE_LIBRARY = 1,
		CHOICE_NIGHTMARE = 2,
		CHOICE_HAND_GY = 3,
		CHOICE_SHOW_EXILED = 4
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Exile top 3 cards of library", can_library, priority_library, +2,
						"Nightmare creature", can_nightmare, priority_nightmare, 0,
						"Exile hand and graveyard", 1, priority_hand_gy, -10,
						"(Show exiled creatures)", 1, -100, 999);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice && planeswalker(player, card, event, 3);
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;
		  ai_modifier += 48;

		  if (choice == CHOICE_SHOW_EXILED || charge_mana_for_activated_ability(player, card, MANACOST_X(0)))
			switch (choice)
			  {
				case CHOICE_LIBRARY:
				  instance->targets[0].player = 1 - player;
				  instance->targets[0].card = -1;
				  instance->number_of_targets = 1;
				  break;

				case CHOICE_NIGHTMARE:
				  {
					int rval = exiledby_choose(player, card, CARD_ID_ASHIOK_NIGHTMARE_WEAVER, EXBY_CHOOSE|EXBY_TEST_CMC_LE, loyalty, "creature", 1);
					int* loc = (int*)rval;
					if (!loc)
					  spell_fizzled = 1;
					else
					  {
						instance->targets[0].player = -1;
						instance->targets[0].card = *loc;
						int iid = *loc & ~0x80000000;
						add_counters_as_cost(player, card, COUNTER_LOYALTY, -get_cmc_by_id(cards_data[iid].id));
					  }
					break;
				  }

				case CHOICE_HAND_GY:
				  break;

				case CHOICE_SHOW_EXILED:
				  exiledby_choose(player, card, CARD_ID_ASHIOK_NIGHTMARE_WEAVER, EXBY_CHOOSE, 0, "creature", 0);
				  spell_fizzled = 1;
				  return 0;	// And don't fall through to planeswalker(), at least not until it honors spell_fizzled.
			  }
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		{
		  switch (choice)
			{
			  case CHOICE_LIBRARY:
				{
				  int i, leg = 0, idx = 0;
				  for (i = 0; i < 3 && deck_ptr[1-player][0] != -1; ++i)
					{
					  ashiok_exiledby_remember(player, instance->parent_card, 1-player, deck_ptr[1-player][0], &leg, &idx);
					  rfg_card_in_deck(1-player, 0);
					}
				  break;
				}

			  case CHOICE_NIGHTMARE:
				{
				  int* loc = exiledby_find(player, instance->parent_card, instance->targets[0].card, NULL, NULL);
				  if (!loc)
					spell_fizzled = 1;
				  else
					{
					  *loc = -1;
					  int owner = (instance->targets[0].card & 0x80000000) ? 1 : 0;
					  int iid = instance->targets[0].card & ~0x80000000;
					  if (remove_card_from_rfg(owner, cards_data[iid].id))
						{
						  int nightmare = add_card_to_hand(player, iid);
						  if (player != owner)
							get_card_instance(player, nightmare)->state ^= STATE_OWNED_BY_OPPONENT;
						  create_targetted_legacy_effect(player, instance->parent_card, empty, player, nightmare);
						  add_a_subtype(player, nightmare, SUBTYPE_NIGHTMARE);
						  put_into_play(player, nightmare);
						}
					  else
						spell_fizzled = 1;
					}
				  break;
				}

			  case CHOICE_HAND_GY:
				{
				  int i, leg = 0, idx = 0;
				  for (i = 0; i < active_cards_count[1-player]; ++i)
					if (in_hand(1-player, i))
					  {
						ashiok_exiledby_remember(player, instance->parent_card, 1-player, get_card_instance(1-player, i)->internal_card_id, &leg, &idx);
						kill_card(1-player, i, KILL_REMOVE);
					  }

				  for (i = 0; i < 500; ++i)
					ashiok_exiledby_remember(player, instance->parent_card, 1-player, get_grave(1-player)[i], &leg, &idx);

				  rfg_whole_graveyard(1-player);
				  break;
				}

			  case CHOICE_SHOW_EXILED:
				break;
			}
		}
	}

	return planeswalker(player, card, event, 3);
}

int card_battlewise_hoplite(int player, int card, event_t event){

	if( heroic(player, card, event) ){
		add_1_1_counter(player, card);
		scry(player, 1);
	}

	return 0;
}

int card_chronicler_of_heroes(int player, int card, event_t event){

	/* Chronicler of Heroes	|1|G|W
	 * Creature - Centaur Wizard 3/3
	 * When ~ enters the battlefield, draw a card if you control a creature with a +1/+1 counter on it. */

	if (comes_into_play(player, card, event) && has_any_counters(player, COUNTER_P1_P1, TYPE_CREATURE)){
		draw_a_card(player);
	}

	return 0;
}

int card_daxos_of_meletis(int player, int card, event_t event)
{
  /* Daxos of Meletis	|1|W|U
   * Legendary Creature - Human Soldier 2/2
   * ~ can't be blocked by creatures with power 3 or greater.
   * Whenever ~ deals combat damage to a player, exile the top card of that player's library. You gain life equal to that card's converted mana cost. Until end
   * of turn, you may cast that card and you may spend mana as though it were mana of any color to cast it. */

  check_legend_rule(player, card, event);

  if (event == EVENT_BLOCK_LEGALITY
	  && attacking_card_controller == player && attacking_card == card
	  && get_power(affected_card_controller, affected_card) >= 3)
	event_result = 1;

  if (has_combat_damage_been_inflicted_to_a_player(player, card, event))
	{
	  int iid = deck_ptr[1-player][0];
	  if (iid != -1)
		{
		  int csvid = cards_data[iid].id;
		  gain_life(player, get_cmc_by_id(csvid));
		  rfg_top_card_of_deck(1-player);
		  if (!is_what(-1, iid, TYPE_LAND))
			create_may_play_card_from_exile_effect(player, card, 1-player, csvid, MPCFE_UNTIL_EOT | MPCFE_FOR_CMC);
		}
	}

  return 0;
}

int card_destructive_revelry(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_ENCHANTMENT);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			damage_player(instance->targets[0].player, 2, player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "DISENCHANT", 1, NULL);
}

int card_fleecemane_lion(int player, int card, event_t event){
	if( is_monstrous(player, card) ){
		hexproof(player, card, event);
		indestructible(player, card, event);
	}
	return monstrosity(player, card, event, 3, 0, 0, 1, 0, 1, 1);
}

int card_horizon_chimera(int player, int card, event_t event){

	/* Horizon Chimera	|2|G|U
	 * Creature - Chimera 3/2
	 * Flash
	 * Flying, trample
	 * Whenever you draw a card, you gain 1 life. */

	if (card_drawn_trigger(player, card, event, player, RESOLVE_TRIGGER_MANDATORY)){
		gain_life(player, 1);
	}
	return flash(player, card, event);
}

int card_kragma_warcaller(int player, int card, event_t event)
{
  /* Kragma Warcaller	|3|B|R
   * Creature - Minotaur Warrior 2/3
   * Minotaur creatures you control have haste.
   * Whenever a Minotaur you control attacks, it gets +2/+0 until end of turn. */

  boost_subtype(player, card, event, SUBTYPE_MINOTAUR, 0,0, 0,SP_KEYWORD_HASTE, BCT_CONTROLLER_ONLY|BCT_INCLUDE_SELF);

  if (xtrigger_condition() == XTRIGGER_ATTACKING || event == EVENT_DECLARE_ATTACKERS)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.subtype = SUBTYPE_MINOTAUR;

	  int amt;
	  if ((amt = declare_attackers_trigger_test(player, card, event, DAT_TRACK, player, -1, &test)))
		{
		  card_instance_t* instance = get_card_instance(player, card);
		  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
		  for (--amt; amt >= 0; --amt)
			if (in_play(current_turn, attackers[amt]))
			  pump_until_eot(player, card, current_turn, attackers[amt], 2, 0);
		}
	}

  return 0;
}

int card_medomai_the_ageless(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( instance->targets[1].player > 0 ){
		if( current_turn == player && upkeep_trigger(player, card, event) ){
			instance->targets[1].player--;
		}
		cannot_attack(player, card, event);
	}

	if( damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE|DDBM_MUST_DAMAGE_PLAYER) ){
		time_walk_effect(player, card);
	}

	return 0;
}

int card_pharikas_mender(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_ENCHANTMENT, "Select a creature or enchantment card.");

		if( new_special_count_grave(player, &this_test) > 0 && !graveyard_has_shroud(2) ){
			new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
		}
	}

	return 0;
}

int card_polis_crusher(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	// protection_from_enchantments(player, card, event);

	if( is_monstrous(player, card) && damage_dealt_by_me(player, card, event, DDBM_MUST_BE_COMBAT_DAMAGE+DDBM_MUST_DAMAGE_PLAYER) ){
		target_definition_t td1;
		default_target_definition(player, card, &td1, TYPE_ENCHANTMENT);
		td1.allowed_controller = 1-player;
		td1.allow_cancel = 0;

		if (can_target(&td1) && pick_target(&td1, "TARGET_ENCHANTMENT")){
			card_instance_t* instance = get_card_instance(player, card);

			instance->number_of_targets = 1;
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return monstrosity(player, card, event, 4, 0, 0, 1, 1, 0, 3);
}

int card_prophet_of_kruphix(int player, int card, event_t event){

	if( event == EVENT_CAN_ACTIVATE ){
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && is_what(player, i, TYPE_CREATURE) && has_mana_to_cast_iid(player, event, get_original_internal_card_id(player, i)) ){
				return 1;
			}
		}
	}

	if( event == EVENT_ACTIVATE ){
		int pc[2][hand_count[player]];
		int pcc = 0;
		int i;
		for(i=0; i<active_cards_count[player]; i++){
			if( in_hand(player, i) && is_what(player, i, TYPE_CREATURE) && has_mana_to_cast_iid(player, event, get_original_internal_card_id(player, i)) ){
				pc[0][pcc] = get_original_internal_card_id(player, i);
				pc[1][pcc] = i;
				pcc++;
			}
		}
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_CREATURE, "Select a creature card to play.");
		int selected = select_card_from_zone(player, player, pc[0], pcc, 0, AI_MAX_VALUE, -1, &this_test);
		if( selected == -1 ){
			spell_fizzled = 1;
			return 0;
		}
		if( charge_mana_from_id(player, pc[1][selected], event, get_id(player, pc[1][selected])) ){
			play_card_in_hand_for_free(player, pc[1][selected]);
			cant_be_responded_to = 1;	// The spell will be respondable to, but this (fake) activation won't
		}
		else{
			spell_fizzled = 1;
		}
	}

	untap_permanents_during_opponents_untap(player, card, TYPE_CREATURE|TYPE_LAND, &get_card_instance(player, card)->info_slot);
	return 0;
}

int card_psychic_intrusion(int player, int card, event_t event){
	/* Psychic Intrusion	|3|U|B
	 * Sorcery
	 * Target opponent reveals his or her hand. You choose a nonland card from that player's graveyard or hand and exile it. You may cast that card for as long
	 * as it remains exiled, and you may spend mana as though it were mana of any color to cast that spell. */

	if (!IS_CASTING(player, card, event)){
		return 0;
	}

	if( event == EVENT_CAN_CAST ){
		return opponent_is_valid_target(player, card);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		target_opponent(player, card);
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( opponent_is_valid_target(player, card) && get_card_instance(player, card)->targets[0].player == 1-player ){
				int result = -1;
				if( hand_count[1-player] > 0 ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_LAND, "Opponent's hand: Select a card to exile.");
					this_test.type_flag = 1;
					result = new_global_tutor(player, 1-player, TUTOR_FROM_HAND, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
				}
				if( result == -1 && get_grave(1-player)[0] != -1 ){
					test_definition_t this_test;
					new_default_test_definition(&this_test, TYPE_LAND, "Opponent's graveyard: Select a card to exile.");
					this_test.type_flag = 1;
					result = new_global_tutor(player, 1-player, TUTOR_FROM_GRAVE, TUTOR_RFG, 0, AI_MAX_VALUE, &this_test);
				}
				if( result > -1 ){
					create_may_play_card_from_exile_effect(player, card, 1-player, result, MPCFE_FOR_CMC);
				}
			} else {
				spell_fizzled = 1;
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_reaper_of_the_wilds(int player, int card, event_t event){

	if( event == EVENT_GRAVEYARD_FROM_PLAY ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.not_me = 1;
		count_for_gfp_ability(player, card, event, ANYBODY, 0, &this_test);
	}

	if( in_play(player, card) && resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		int i;
		for(i=0; i<get_card_instance(player, card)->targets[11].card; i++){
			scry(player, 1);
		}
		get_card_instance(player, card)->targets[11].card = 0;
	}

	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);

		int ai_priority_deathtouch = (check_for_special_ability(player, card, SP_KEYWORD_DEATHTOUCH) ? -1
									  : current_phase == PHASE_AFTER_BLOCKING && ((is_attacking(player, card) && !is_unblocked(player, card))
																				  || (current_turn == 1-player && instance->blocking != 255)) ? 10
									  : -1);
		int ai_priority_hexproof = check_for_special_ability(player, card, SP_KEYWORD_HEXPROOF) ? -1 : 5;

		enum {
			CHOICE_DEATHTOUCH = 1,
			CHOICE_HEXPROOF = 2
		} choice = DIALOG(player, card, event,
						  "Gain deathtouch",	1, ai_priority_deathtouch,	DLG_MANA(MANACOST_B(1)),
						  "Gain hexproof",		1, ai_priority_hexproof,	DLG_MANA(MANACOST_XG(1,1)));

		if (event == EVENT_CAN_ACTIVATE){
			return can_use_activated_abilities(player, card) && choice;
		} else if (event == EVENT_RESOLVE_ACTIVATION){
			switch (choice){
				case CHOICE_DEATHTOUCH:
					pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
					break;
				case CHOICE_HEXPROOF:
					pump_ability_until_eot(player, instance->parent_card, player, instance->parent_card, 0, 0, 0, SP_KEYWORD_HEXPROOF);
					break;
			}
		}
	}

	return 0;
}

int card_sentry_of_the_underworld(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	vigilance(player, card, event);

	if( land_can_be_played & LCBP_REGENERATION ){
		if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 1) && can_pay_life(player, 3) ){
			return can_regenerate(player, card);
		}
		else if( event == EVENT_ACTIVATE ){
				if( charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 1) ){
					lose_life(player, 3);
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				 regenerate_target(player, instance->parent_card);
		}
	}

	return 0;
}

int card_shipwreck_singer(int player, int card, event_t event){

	if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t* instance = get_card_instance(player, card);

		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;

		int can_siren = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_CAN_TARGET, MANACOST_XU(1,1), 0, &td, "TARGET_CREATURE");
		int ai_priority_siren = current_turn == 1-player && current_phase <= PHASE_MAIN1 ? 1 : -1;

		int can_weaken = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED|GAA_NONSICK, MANACOST_XB(1,1), 0, NULL, NULL);
		int ai_priority_weaken = current_turn == 1-player && current_phase > PHASE_MAIN1 && current_phase < PHASE_MAIN2 ? 1 : -1;

		enum {
			CHOICE_SIREN = 1,
			CHOICE_WEAKEN = 2
		} choice = DIALOG(player, card, event,
						  "Creature attacks if able", can_siren, ai_priority_siren,
						  "Attacking creatures get -1/-1", can_weaken, ai_priority_weaken);

		if (event == EVENT_CAN_ACTIVATE){
			return choice;
		} else if (event == EVENT_ACTIVATE){
			instance->number_of_targets = 0;
			switch (choice){
				case CHOICE_SIREN:
					if (charge_mana_for_activated_ability(player, card, MANACOST_XU(1,1))){
						pick_target(&td, "TARGET_CREATURE");
					}
					break;
				case CHOICE_WEAKEN:
					if (charge_mana_for_activated_ability(player, card, MANACOST_XB(1,1))){
						tap_card(player, card);
					}
					break;
			}
		} else {	// EVENT_RESOLVE_ACTIVATION
			switch (choice){
				case CHOICE_SIREN:
					pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
										   0, 0, 0, SP_KEYWORD_MUST_ATTACK);
					break;
				case CHOICE_WEAKEN:{
					test_definition_t this_test;
					default_test_definition(&this_test, TYPE_CREATURE);
					this_test.state = STATE_ATTACKING;
					pump_creatures_until_eot(player, instance->parent_card, current_turn, 0, -1, -1, 0, 0, &this_test);
					break;
				}
			}
		}
	}
	return 0;
}

int card_spellheart_chimera(int player, int card, event_t event){
	/* Spellheart Chimera	|1|U|R
	 * Creature - Chimera 100/3
	 * Flying, trample
	 * ~'s power is equal to the number of instant and sorcery cards in your graveyard. */

	if (event == EVENT_POWER && affect_me(player, card) && !is_humiliated(player, card) && player != -1){
		event_result += count_graveyard_by_type(player, TYPE_INSTANT|TYPE_SORCERY);
	}

	return 0;
}

int card_steam_augury(int player, int card, event_t event)
{
  if (event == EVENT_CAN_CAST)
	return 1;
  else if (event == EVENT_CAST_SPELL && affect_me(player, card))
	ai_modifier += 48;
  else if (event == EVENT_RESOLVE_SPELL)
	{
	  effect_fof(player, 1-player, 5, TUTOR_GRAVE);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

const char* target_has_fate_counter(int who_chooses, int player, int card)
{
  return count_counters(player, card, COUNTER_FATE) <= 0 ? "no fate counter" : NULL;
}

const char* target_not_has_fate_counter(int who_chooses, int player, int card)
{
  return count_counters(player, card, COUNTER_FATE) > 0 ? "fate counter" : NULL;
}

int card_triad_of_fates(int player, int card, event_t event)
{
  /* Triad of Fates	|2|W|B
   * Legendary Creature - Human Wizard 3/3
   * |1, |T: Put a fate counter on another target creature.
   * |W, |T: Exile target creature that has a fate counter on it, then return it to the battlefield under its owner's control.
   * |B, |T: Exile target creature that has a fate counter on it. Its controller draws two cards. */

  check_legend_rule(player, card, event);

  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  if (event == EVENT_CAN_ACTIVATE
		  && (is_tapped(player, card) || is_sick(player, card) || !can_use_activated_abilities(player, card)))
		return 0;	// otherwise, check more carefully below

	  card_instance_t* instance = get_card_instance(player, card);

	  target_definition_t td_notme;
	  default_target_definition(player, card, &td_notme, TYPE_CREATURE);
	  td_notme.preferred_controller = player;
	  td_notme.special = TARGET_SPECIAL_NOT_ME;

	  target_definition_t td_fated;
	  default_target_definition(player, card, &td_fated, TYPE_CREATURE);
	  td_fated.preferred_controller = player;
	  td_fated.extra = (int)target_has_fate_counter;
	  td_fated.special = TARGET_SPECIAL_EXTRA_FUNCTION;

	  target_definition_t td_destroyed;
	  target_definition_t td_unfated;

	  int can_fate = has_mana_for_activated_ability(player, card, MANACOST_X(1)) && can_target(&td_notme);
	  int can_blink, can_exile;
	  if (can_target(&td_fated))
		{
		  can_blink = has_mana_for_activated_ability(player, card, MANACOST_W(1));
		  can_exile = has_mana_for_activated_ability(player, card, MANACOST_B(1));
		}
	  else
		can_blink = can_exile = 0;

	  int ai_priority_fate, ai_priority_blink, ai_priority_exile;

	  if (player == AI || ai_is_speculating == 1)
		{
		  /* This seems a poor idea - among other things, it'll let the ai blink a creature after it's done combat damage but before the combat damage dealt to
		   * it destroys it, like when combat damage used the stack.  On the other hand, it doesn't seem to work anyway. */
		  default_target_definition(player, card, &td_destroyed, TYPE_CREATURE);
		  td_destroyed.allowed_controller = player;
		  td_destroyed.preferred_controller = player;
		  td_destroyed.extra = (int)target_has_fate_counter;
		  td_destroyed.special = TARGET_SPECIAL_EXTRA_FUNCTION;
		  td_destroyed.required_state = TARGET_STATE_DESTROYED;
		  td_destroyed.allow_cancel = 0;

		  default_target_definition(player, card, &td_unfated, TYPE_CREATURE);
		  td_unfated.allowed_controller = player;
		  td_unfated.preferred_controller = player;
		  td_unfated.extra = (int)target_not_has_fate_counter;
		  td_unfated.special = TARGET_SPECIAL_EXTRA_FUNCTION | TARGET_SPECIAL_NOT_ME;
		  td_unfated.allow_cancel = 0;

		  ai_priority_fate = can_target(&td_unfated) ? 5 : -1;
		  ai_priority_blink = can_target(&td_destroyed) ? 10 : 3;	// specific value checked for below
		  ai_priority_exile = hand_count[player] < 1 ? 7 : 1;
		}
	  else
		ai_priority_fate = ai_priority_blink = ai_priority_exile = 0;

	  enum
	  {
		CHOICE_FATE = 1,
		CHOICE_BLINK = 2,
		CHOICE_EXILE = 3
	  } choice = DIALOG(player, card, event,
						"Add a fate counter", can_fate, ai_priority_fate,
						"Blink a creature", can_blink, ai_priority_blink,
						"Exile and draw", can_exile, ai_priority_exile);

	  if (event == EVENT_CAN_ACTIVATE)
		return (!choice ? 0
				: ai_priority_blink == 10 && (land_can_be_played & LCBP_REGENERATION) ? 99
				: 1);
	  else if (event == EVENT_ACTIVATE)
		{
		  instance->number_of_targets = 0;

		  if (player == AI && current_turn == 1-player && current_phase == PHASE_DISCARD && can_target(&td_unfated))
			ai_modifier += 48;

		  switch (choice)
			{
			  case CHOICE_FATE:
				if (charge_mana_for_activated_ability(player, card, MANACOST_X(1)))
				  {
					if ((player == AI || ai_is_speculating == 1) && can_target(&td_unfated) && pick_target(&td_unfated, "TARGET_CREATURE"))
					  tap_card(player, card);
					else if (pick_target(&td_notme, "TARGET_CREATURE"))
					  {
						ai_modifier -= 48;
						tap_card(player, card);
					  }
				  }
				break;
			  case CHOICE_BLINK:
				if (charge_mana_for_activated_ability(player, card, MANACOST_W(1)))
				  {
					if ((player == AI || ai_is_speculating == 1) && can_target(&td_destroyed) && pick_target(&td_destroyed, "TARGET_CREATURE"))
					  {
						ai_modifier += 64;
						tap_card(player, card);
					  }
					else if (pick_target(&td_fated, "TARGET_CREATURE"))
					  tap_card(player, card);
				  }
				break;
			  case CHOICE_EXILE:
				if (charge_mana_for_activated_ability(player, card, MANACOST_B(1))
					&& pick_target(&td_fated, "TARGET_CREATURE"))
				  tap_card(player, card);
				break;
			}
		}
	  else	// event == EVENT_RESOLVE_ACTIVATION
		{
		  switch (choice)
			{
			  case CHOICE_FATE:
				if (valid_target(&td_notme))
				  add_counter(instance->targets[0].player, instance->targets[0].card, COUNTER_FATE);
				break;
			  case CHOICE_BLINK:
				if (valid_target(&td_fated))
				  blink_effect(instance->targets[0].player, instance->targets[0].card, 0);
				break;
			  case CHOICE_EXILE:
				if (valid_target(&td_fated))
				  {
					kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
					draw_cards(instance->targets[0].player, 2);
				  }
				break;
			}
		}
	}

  return 0;
}

int card_tymaret_the_murder_king(int player, int card, event_t event){
	/* Tymaret, the Murder King	|B|R
	 * Legendary Creature - Zombie Warrior 2/2
	 * |1|R, Sacrifice another creature: ~ deals 2 damage to target player.
	 * |1|B, Sacrifice a creature: Return Tymaret from your graveyard to your hand. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if(event == EVENT_GRAVEYARD_ABILITY){
		if( has_mana_multi( player, 1, 1, 0, 0, 0, 0 ) && can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return GA_RETURN_TO_HAND;
		}
	}
	if( event == EVENT_PAY_FLASHBACK_COSTS ){
		charge_mana_multi( player, 1, 1, 0, 0, 0, 0 );
		if( spell_fizzled != 1 ){
			if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				return GAPAID_REMOVE;
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, 2, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET+GAA_SACRIFICE_CREATURE+GAA_NOT_ME_AS_TARGET, 1, 0, 0, 0, 1, 0, 0, &td, "TARGET_PLAYER");
}

int card_underworld_cerberus(int player, int card, event_t event){
	/* Underworld Cerberus	|3|B|R
	 * Creature - Hound 6/6
	 * ~ can't be blocked except by three or more creatures.
	 * Cards in graveyards can't be the targets of spells or abilities.
	 * When ~ dies, exile it and each player returns all creature cards from his or her graveyard to his or her hand. */

	minimum_blockers(player, card, event, 3);

	if( this_dies_trigger(player, card, event, 2) ){
		exile_from_owners_graveyard(player, card);

		test_definition_t test;
		new_default_test_definition(&test, TYPE_CREATURE, "");

		int num_returned[2];
		num_returned[current_turn] = from_grave_to_hand_multiple(current_turn, &test);
		num_returned[1-current_turn] = from_grave_to_hand_multiple(1-current_turn, &test);

		ai_modifier += 8 * (num_returned[AI] - num_returned[HUMAN]);
	}

	return 0;
}

int card_xenagos_the_reveler(int player, int card, event_t event){

	/* Xenagos, the Reveler	|2|R|G
	 * Planeswalker - Xenagos (3)
	 * +1: Add X mana in any combination of |R and/or |G to your mana pool, where X is the number of creatures you control.
	 * 0: Put a 2/2 |Sred and |Sgreen Satyr creature token with haste onto the battlefield.
	 * -6: Exile the top seven cards of your library. You may put any number of creature and/or land cards from among them onto the battlefield. */

	if (IS_ACTIVATING(event)){

		int priorities[3] = {0, 0, 0} ;
		if( event == EVENT_ACTIVATE ){
			priorities[0] = count_subtype(player, TYPE_CREATURE, -1)*3;
			priorities[1] = 15-(count_subtype(player, TYPE_CREATURE, -1)*5);
			priorities[2] = (internal_rand(5)*5)+((count_counters(player, card, COUNTER_LOYALTY)*5)-30);
		}

		enum{
			CHOICE_MANA = 1,
			CHOICE_SATYR,
			CHOICE_EXILE7
		}
		choice = DIALOG(player, card, event,
						DLG_RANDOM, DLG_PLANESWALKER,
						"Produce mana", 1, priorities[0], 1,
						"Generate a Satyr", 1, priorities[1], 0,
						"Exile top 7", 1, priorities[2], -6);

	  if (event == EVENT_CAN_ACTIVATE)
		{
		  if (!choice)
			return 0;
		}
	  else if (event == EVENT_RESOLVE_ACTIVATION)
		{
			switch (choice)
			{
				case CHOICE_MANA:
				{
					// Stupidly enough, the ruling is that this isn't a mana ability and it's supposed to happen at resolution.
					int amount = count_subtype(player, TYPE_CREATURE, -1);
					if (amount > 0){
						do {
							spell_fizzled = 0;
							produce_mana_any_combination_of_colors(player, COLOR_TEST_RED|COLOR_TEST_GREEN, amount, NULL);
						} while (spell_fizzled == 1);
					}
				}
				break;

				case CHOICE_SATYR:
				{
					token_generation_t token;
					default_token_definition(player, card, CARD_ID_SATYR, &token);
					token.pow = 2;
					token.tou = 2;
					token.color_forced = COLOR_TEST_RED|COLOR_TEST_GREEN;
					token.s_key_plus = SP_KEYWORD_HASTE;
					generate_token(&token);
				}
				break;

				case CHOICE_EXILE7:
				{
					int amount = MIN(7, count_deck(player));
					if( amount > 0 ){
						int *deck = deck_ptr[player];
						show_deck( 1-player, deck, amount, "Card exiled by Xenagos", 0, 0x7375B0 );

						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_CREATURE | TYPE_LAND, "Select a creature or land card.");
						this_test.create_minideck = amount;
						this_test.no_shuffle = 1;
						while( amount > 0 && new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_VALUE, &this_test) != -1 ){
								amount--;
								this_test.create_minideck--;
						}
						rfg_top_n_cards_of_deck(player, amount);
					}
				}
				break;
			}
		}
	}

	return planeswalker(player, card, event, 3);
}

// Artifacts
int card_akroan_horse(int player, int card, event_t event){

	if( comes_into_play(player, card, event) ){
		give_control(player, card, player, card);
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_SOLDIER, &token);
		token.t_player = 1-player;
		generate_token(&token);
	}

	return 0;
}

// Anvilwrought Raptor --> vanilla

// bronze sable --> vanilla


int card_burnished_hart(int player, int card, event_t event){

	/* Burnished Hart	|3
	 * Artifact Creature - Elk 2/2
	 * |3, Sacrifice ~: Search your library for up to two basic land cards, put them onto the battlefield tapped, then shuffle your library. */

	if(event == EVENT_RESOLVE_ACTIVATION ){
		tutor_basic_lands(player, TUTOR_PLAY_TAPPED, 2);
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME, 3, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_colossus_of_akros(int player, int card, event_t event){

	/* Colossus of Akros	|8
	 * Artifact Creature - Golem 10/10
	 * Defender, indestructible
	 * |10: Monstrosity 10.
	 * As long as ~ is monstrous, it has trample and can attack as though it didn't have defender. */

	indestructible(player, card, event);

	if (event == EVENT_ABILITIES && affect_me(player, card) && is_monstrous(player, card)){
		event_result |= KEYWORD_TRAMPLE;
		card_instance_t* instance = get_card_instance(player, card);
		instance->token_status |= STATUS_WALL_CAN_ATTACK;
	}

	return monstrosity(player, card, event, MANACOST_X(10), 10);
}

int card_flamecast_wheel(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 3, player, card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME|GAA_UNTAPPED|GAA_NONSICK|GAA_CAN_TARGET, 5, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_fleetfeather_sandals(int player, int card, event_t event){
	return vanilla_equipment(player, card, event, 2, 0, 0, KEYWORD_FLYING, SP_KEYWORD_HASTE);
}

// opaline unicorn --> card_generic_noncombat_1_mana_producing_creature

int card_prowlers_helm(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( is_equipping(player, card) ){
		if( event == EVENT_BLOCK_LEGALITY ){
			if( attacking_card_controller == instance->targets[8].player && attacking_card == instance->targets[8].card ){
				if( ! has_subtype(affected_card_controller, affected_card, SUBTYPE_WALL) ){
					event_result = 1;
				}
			}
		}
	}

	return basic_equipment(player, card, event, 2);
}

int card_pyxis_of_pandemonium(int player, int card, event_t event)
{
  if (event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
	{
	  card_instance_t* instance = get_card_instance(player, card);

	  int can_exile = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED|GAA_NONSICK, MANACOST_X(0), 0, 0, 0);
	  int can_return = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED|GAA_NONSICK|GAA_SACRIFICE_ME, MANACOST_X(7), 0, 0, 0);
	  int ai_priority_return = exiledby_count(player, card, player);

	  enum
	  {
		CHOICE_EXILE = 1,
		CHOICE_RETURN = 2
	  } choice = DIALOG(player, card, event,
						DLG_RANDOM,
						"Exile top cards", can_exile, 5,
						"Return permanents from exile", can_return, ai_priority_return);

	  if (event == EVENT_CAN_ACTIVATE)
		return choice;
	  else if (event == EVENT_ACTIVATE)
		switch (choice)
		  {
			case CHOICE_EXILE:
			  if (player == AI && current_turn == 1-player && current_phase == PHASE_DISCARD)
				ai_modifier += 48;
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(0)))
				tap_card(player, card);
			  break;

			case CHOICE_RETURN:
			  if (charge_mana_for_activated_ability(player, card, MANACOST_X(7)))
				{
				  tap_card(player, card);
				  instance->targets[1].player = exiledby_detach(player, card);
				  kill_card(player, card, KILL_SACRIFICE);
				}
			  break;
#if 0
			case CHOICE_DEBUG:
			  exiledby_choose(player, card, CARD_ID_PYXIS_OF_PANDEMONIUM, EXBY_CHOOSE, 0, NULL, 0);
			  cancel=1;
			  break;
#endif
		  }
	  else	// event == EVENT_RESOLVE_ACTIVATION
		{
		  int leg = 0, idx = 0, *loc;

		  switch (choice)
			{
			  case CHOICE_EXILE:
				if (deck_ptr[player][0] != -1)
				  {
					exiledby_remember(player, instance->parent_card, player, deck_ptr[player][0], &leg, &idx);
					play_sound_effect(WAV_DESTROY);
					obliterate_top_card_of_deck(player);
				  }
				if (deck_ptr[1-player][0] != -1)
				  {
					exiledby_remember(player, instance->parent_card, 1-player, deck_ptr[1-player][0], &leg, &idx);
					play_sound_effect(WAV_DESTROY);
					obliterate_top_card_of_deck(1-player);
				  }
				break;

			  case CHOICE_RETURN:
				while ((loc = exiledby_find_any(player-2, instance->targets[1].player, &leg, &idx)))
				  {
					int owner = (*loc & 0x80000000) ? 1 : 0;
					int iid = *loc & ~0x80000000;
					if (is_what(-1, iid, TYPE_PERMANENT))
					  put_into_play(owner, add_card_to_hand(owner, iid));
					else
					  add_card_to_rfg(owner, iid);
					*loc = -1;
				  }

				exiledby_destroy_detached(player, instance->targets[1].player);
				break;
			}
		}
	}

  return 0;
}

int card_witches_eye(int player, int card, event_t event, int equip_cost){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) ){
		if( is_equipping(player, card) &&
			generic_activated_ability(instance->targets[8].player, instance->targets[8].card, event, GAA_UNTAPPED, 1, 0, 0, 0, 0, 0, 0, 0, 0)
		  ){
			return 1;
		}
		return can_activate_basic_equipment(player, card, event, 1);
	}
	else if( event == EVENT_ACTIVATE ){
			int choice = 0;
			if (can_activate_basic_equipment(player, card, event, 1)){
				if( is_equipping(player, card) &&
					generic_activated_ability(instance->targets[8].player, instance->targets[8].card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, 1, 0, 0, 0, 0, 0, 0, 0, 0)
				  ){
					choice = do_dialog(player, player, card, -1, -1, " Equip\n Scry 1\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}

			instance->info_slot = 66 + choice;

			if( choice == 0 ){
				activate_basic_equipment(player, card, 1);
			}
			else if( choice == 1 ){
					if( charge_mana_for_activated_ability(instance->targets[8].player, instance->targets[8].card, 1, 0, 0, 0, 0, 0) ){
						tap_card(instance->targets[8].player, instance->targets[8].card);
					}
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if(event == EVENT_RESOLVE_ACTIVATION ){
			if( instance->info_slot == 66 ){
				resolve_activation_basic_equipment(player, card);
			}
			if( instance->info_slot == 67 ){
				scry(player, 1);
			}
	}
	return 0;
}

// Lands

int card_nykthos_shrine_to_nix(int player, int card, event_t event){
	/* Nykthos, Shrine to Nyx	""
	 * Legendary Land
	 * |T: Add |C to your mana pool.
	 * |2, |T: Choose a color. Add to your mana pool an amount of mana of that color equal to your devotion to that color. */

	card_instance_t* instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_ACTIVATE ){
		int choice = 0;
		int ai_choice = 0;
		if( has_mana(player, COLOR_COLORLESS, 3) ){
			if( devotion(player, -1, get_deck_color(player, player), 0) >= 3 ){
				ai_choice = 1;
			}
			choice = do_dialog(player, player, card, -1, -1, " Produce 1\n Produce Devotion mana\n Cancel", ai_choice);
		}

		if( choice == 0){
			produce_mana_tapped(player, card, COLOR_COLORLESS, 1);
		}
		else if( choice == 1 ){
				instance->state |= STATE_TAPPED;
				charge_mana(player, COLOR_COLORLESS, 2);
				if( spell_fizzled != 1 ){
					int clr = choose_a_color(player, get_deck_color(player, player));
					produce_mana_tapped(player, card, clr, devotion(player, -1, clr, 0));
				}
				else{
					instance->state &= ~STATE_TAPPED;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}
	else if( event == EVENT_COUNT_MANA && affect_me(player, card) ){
			if( ! is_tapped(player, card) && ! is_animated_and_sick(player, card) ){
				instance->info_slot = COLOR_TEST_COLORLESS;

				/* This doesn't really work - has_mana() isn't accurate during EVENT_COUNT_MANA.
				 * Proper solution here for mana sources that require mana to activate is to create a second pass after all the free mana sources have been
				 * counted.  Will want to move 0x472400, count_mana(), into C. */
				if( has_mana(player, COLOR_COLORLESS, 2) ){
					/* No good solution here - declare_mana_available_hex() forces the amount of mana produceable to always be the same color.
					 * Instead we have a choice of e.g. {1}, {-2}{W}, {-2}{U}{U}{U}, {-2}{G}{G}, etc. */
					int clr = get_deck_color(player, player);
					declare_mana_available(player, clr, devotion(player, -1, clr, 0));

					// Set info_slot for get_color_of_mana_produced_by_id() and get_colors_of_mana_land_could_produce_ignoring_costs()
					for (clr = COLOR_BLACK; clr <= COLOR_WHITE; ++clr){
						if (devotion(player, -1, clr, 0) > 0){
							instance->info_slot |= 1 << clr;
						}
					}
				}
				else{
					declare_mana_available(player, COLOR_COLORLESS, 1);
				}
			}
	}

	else{
		return mana_producer(player, card, event);
	}

	return 0;
}

// double lands --> new benalia

// "Face the Hydra" challenge deck
/*
Face the Hydra
Lands
0

Creatures
11 Hydra Head
4 Ravenous Brute Head
1 Savage Vigor Head
1 Snapping Fang Head
1 Shrieking Titan Head

Sorceries
5 Disorienting Glower
5 Distract the Hydra
4 Grown from the Stump
4 Hydra's Impenetrable Hide
3 Neck Tangle
4 Noxious Hydra Breath
2 Strike the Weak Spot
5 Swallow the Hero Whole
4 Torn Between Heads
6 Unified Lunge


How to Play

You can battle the Hydra alone or with friends (just replace "you" with "each player" in these rules).
At the end of any turn, if there are no Heads on the battlefield, you win! Use the regular Magic rules with the following exceptions:

    You start with up to two Hero cards on the battlefield. (You don't need a Hero to play.)
    Choose a starting number of Heads. Take that many cards named Hydra Head from the Challenge Deck and place them on the battlefield.
		Shuffle the remaining cards to form the Hydra's library.
    You go first. (Don't draw a card on your first turn.)
    Then the Hydra takes it's turn, and so on.
    You can attack Heads directly with your creatures. Any number of creatures can attack a single Head
    Growing New Heads: Whenever a Head leaves the battlefield, reveal the top two cards of the Hydra's library.
		Put any Heads onto the battlefield and any sorcery cards into the Hydra's graveyard.

The Hydra's Turn

    At the start of the Hydra's turn, untap any tapped Heads.
    Reveal the top card of the Hydra's library. The Hydra casts that card.
		When the spell resolves, if it's a Head, put it onto the battlefield.
		If it's a sorcery, follow its instructions and then put it into the Hydra's graveyard.
    The Hydra deals 1 damage to you for each untapped card named Hydra Head it controls and 2 damage to you for each untapped elite Head it controls.

Special Rules

    If the Hydra would be dealt damage or lose life, instead deal that much damage to a Head of your choice.
    Ignore effects that would cause the Hydra to draw or discard cards, or any impossible actions.
    If a Head would move to any other zone than the graveyard, instead put it into the Hydra's graveyard.
    You make any choices for the Hydra.
*/

// Functions
static void damage_opponent_on_eot(int player, int card, event_t event, int dmg){
	if(trigger_condition == TRIGGER_EOT &&  affect_me(player, card ) && reason_for_trigger_controller == player){
		if( current_turn == player && ! is_tapped(player, card) ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					if( has_subtype(player, card, SUBTYPE_ELITE) ){
						dmg++;
					}
					damage_player(1-player, dmg, player, card);
			}
		}
	}
}

static int generic_hydra_head(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->kill_code > 0 && instance->kill_code > KILL_SACRIFICE ){
		instance->kill_code = KILL_BURY;
	}
	if( leaves_play(player, card, event) ){
		int *deck = deck_ptr[player];
		int i = 0;
		while(deck[0] != -1 && i <2){
				if( is_what(-1, deck[0], TYPE_PERMANENT) ){
					put_into_play_a_card_from_deck(player, player, 0);
				}
				else{
					mill(player, 1);
				}
				i++;
		}
	}
	return 0;
}

// Cards
// creature (heads)
int card_hydra_head(int player, int card, event_t event){
	if( graveyard_from_play(player, card, event) ){
		gain_life(1-player, 2);
	}
	damage_opponent_on_eot(player, card, event, 1);
	return generic_hydra_head(player, card, event);
}

int card_ravenous_brute_head(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){
		draw_cards(1-player, 1);
		gain_life(1-player, 2);
	}
	if( graveyard_from_play(player, card, event) ){
		gain_life(1-player, 2);
	}
	damage_opponent_on_eot(player, card, event, 1);
	return generic_hydra_head(player, card, event);
}

int card_savage_vigor_head(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){
		draw_cards(1-player, 1);
		gain_life(1-player, 4);
	}
	if( current_turn == player && eot_trigger(player, card, event) ){
		int *deck = deck_ptr[player];
		if( deck[0] != -1 ){
			int iid = deck[0];
			remove_card_from_deck(player, 0);
			play_card_in_hand_for_free(player, add_card_to_hand(player, iid));	// Inaccurate - won't run draw or replace-draw triggers
		}
		if( ! is_tapped(player, card) ){
			damage_player(1-player, 2, player, card);
		}
	}
	return generic_hydra_head(player, card, event);
}

int card_shrieking_titan_head(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){
		draw_cards(1-player, 1);
		gain_life(1-player, 4);
	}
	if( current_turn == player && eot_trigger(player, card, event) ){
		discard(1-player, 0, player);
		if( ! is_tapped(player, card) ){
			int dmg = 1;
			if( has_subtype(player, card, SUBTYPE_ELITE) ){
				dmg++;
			}
			damage_player(1-player, dmg, player, card);
		}
	}
	return generic_hydra_head(player, card, event);
}

int card_snapping_fang_head(int player, int card, event_t event){
	if( this_dies_trigger(player, card, event, 2) ){
		draw_cards(1-player, 1);
		gain_life(1-player, 4);
	}
	damage_opponent_on_eot(player, card, event, 2);
	return generic_hydra_head(player, card, event);
}

// spells
int card_disorienting_glower(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		target_player_cant_cast_type(player, card, 1-player, TYPE_ANY);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_distract_the_hydra(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( sacrifice(player, card, 1-player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.required_subtype = SUBTYPE_HEAD;
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.who_chooses = 1-player;

			if( can_target(&td) && pick_target(&td, "TARGET_CREATURE") ){
				tap_card(instance->targets[0].player, instance->targets[0].card);
			}
		}
		else{
			lose_life(1-player, 3);
		}

		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_grown_from_the_stump(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		char msg[100] = "Select an Hydra Head to reanimate.";
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_ANY, msg);
		//this_test.id = CARD_ID_HYDRA_HEAD
		if( new_special_count_grave(player, &this_test) > 1 ){
			if( new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_FIRST_FOUND, &this_test) != -1 ){
				new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_PLAY, 1, AI_FIRST_FOUND, &this_test);
			}
		}
		else{
			int *deck = deck_ptr[player];
			if( deck[0] != -1 ){
				int count = 0;
				while( deck[count] != -1 ){
						if( has_subtype_by_id(cards_data[deck[count]].id, SUBTYPE_HEAD) ){
							break;
						}
				}
				show_deck( HUMAN, deck, count+1, "Cards revealed by Grown From the Stump", 0, 0x7375B0 );
				put_into_play_a_card_from_deck(player, player, count);
				count--;
				if( count > -1 ){
					mill(player, count+1);
				}
			}
		}

		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int indestructible_until_next_turn(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES)
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (affect_me(instance->damage_target_player, instance->damage_target_card))
		indestructible(instance->damage_target_player, instance->damage_target_card, event);
	}

  if (event == EVENT_BEGIN_TURN && current_turn == player)
	kill_card(player, card, KILL_REMOVE);

  return 0;
}

int card_hydra_impenetrable_hide(int player, int card, event_t event)
{
  // Heads are indestructible until the hydra's next turn.

  // (Heads can't be controlled, can they?  Even if so, I'm assuming that only the Hydra can cast this.)

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if (in_play(p, c) && has_subtype(p, c, SUBTYPE_HEAD))
			create_targetted_legacy_effect(player, card, indestructible_until_next_turn, p, c);

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_neck_tangle(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( count_subtype(player, TYPE_CREATURE, SUBTYPE_HEAD) > 4 ){
			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_CREATURE);
			td.required_subtype = SUBTYPE_HEAD;
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.allow_cancel = 0;

			card_instance_t *instance = get_card_instance(player, card);

			int count = 0;
			while( count < 2 ){
					if( pick_target(&td, "TARGET_CREATURE") ){
						instance->number_of_targets = 1;
						effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
					}
					count++;
			}
		}
		else{
			int *deck = deck_ptr[player];
			if( deck[0] != -1 ){
				int iid = deck[0];
				remove_card_from_deck(player, 0);
				play_card_in_hand_for_free(player, add_card_to_hand(player, iid));	// Inaccurate - won't run draw or replace-draw triggers
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_noxious_hydra_breath(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		int ai_choice = 0;

		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.state = STATE_TAPPED;
		this_test.subtype = SUBTYPE_HEAD;
		this_test.subtype_flag = 1;

		if( check_battlefield_for_special_card(player, card, 1-player, CBFSC_GET_COUNT, &this_test) > 1 && life[1-player] > 6 ){
			ai_choice = 1;
		}
		do_dialog(player, player, card, -1, -1, " 5 damage to opponents\n Destroy non-Head tapped creatures", ai_choice);
		if( ai_choice == 0 ){
			damage_player(1-player, 5, player, card);
		}
		else{
			new_manipulate_all(player, card, 2, &this_test, KILL_DESTROY);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_strike_the_weak_spot(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_HEAD;
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td)){
		return 1;
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		pick_target(&td, "TARGET_CREATURE");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( has_subtype(instance->targets[0].player, instance->targets[0].card, SUBTYPE_ELITE) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
				return card_time_walk(player, card, event);
			}
			else{
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

static int swallow_the_hero_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if( trigger_condition == TRIGGER_LEAVE_PLAY ){
		if( affect_me( player, card) && has_subtype(trigger_cause_controller, trigger_cause, SUBTYPE_HEAD) && reason_for_trigger_controller == player ){
			if(event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					int card_added = add_card_to_hand(instance->targets[0].player, get_internal_card_id_from_csv_id(instance->targets[0].card));
					put_into_play(instance->targets[0].player, card_added);
					kill_card(player, card, KILL_REMOVE);
			}
		}
	}
	return 0;
}

int card_swallow_the_hero_whole(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.allowed_controller = 1-player;
		td.who_chooses = 1-player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		if( pick_target(&td, "TARGET_CREATURE") ){
			int result = rfg_target_permanent(instance->targets[0].player, instance->targets[0].card);
			int legacy = create_legacy_effect(player, card, &swallow_the_hero_legacy);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[0].player = 1-player;
			if( result > available_slots ){
				result-=available_slots;
				leg->targets[0].player = player;
			}
			leg->targets[0].card = result;
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_torn_between_the_heads(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_CREATURE);
		td.required_subtype = SUBTYPE_HEAD;
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.allow_cancel = 0;

		card_instance_t *instance = get_card_instance(player, card);

		int count = 0;
		while( count < 2 && can_target(&td) ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					instance->number_of_targets = 1;
					effect_frost_titan(player, card, instance->targets[0].player, instance->targets[0].card);
				}
				count++;
		}
		damage_player(1-player, 5, player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_unified_lunge(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		damage_player(1-player, count_subtype(player, TYPE_PERMANENT, SUBTYPE_HEAD), player, card);
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

// Hero's paths

// the protector --> samite healer

int card_the_philosopher(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_the_avenger(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_DEATHTOUCH);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_the_warrior(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 0, 0, SP_KEYWORD_HASTE);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_the_hunter(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_CAN_TARGET, 3, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

// the harvester --> merfolk looter

int card_the_slayer(int player, int card, event_t event){
	if( event == EVENT_RESOLVE_SPELL ){
		gain_life(player, 7);
	}
	return 0;
}
