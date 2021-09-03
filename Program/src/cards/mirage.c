#include "manalink.h"

// ----- GENERAL FUNCTIONS ----

static int check_unattach_from_phasing(int player, int card, int mode){
	card_instance_t* instance = get_card_instance(player, card);
	if (instance->damage_target_player < 0 || instance->damage_target_card < 0){
		return 0;	// not attached
	}
	card_instance_t* attached_to = get_card_instance(instance->damage_target_player, instance->damage_target_card);
	if (attached_to->internal_card_id < 0){
		return 1;	// attachment neither in play nor phased
	}
	if ((attached_to->state & STATE_INVISIBLE) || !(attached_to->state & STATE_IN_PLAY)){
		return 1;	// attachment neither in play nor phased
	}

	int attached_to_is_phased;
	if (mode == 0){	// phasing immediately, so either attachment is phasing after what it's attached to, or attachment is phasing alone
		attached_to_is_phased = attached_to->state & STATE_OUBLIETTED;
	} else {	// mode == 2 - second pass of two-pass phasing
		if (attached_to->state & STATE_OUBLIETTED){
			if (check_special_flags(instance->damage_target_player, instance->damage_target_card, SF_PHASED_OUT|SF_PHASED_OUT_INDIRECTLY) == (SF_PHASED_OUT|SF_PHASED_OUT_INDIRECTLY)){
				attached_to_is_phased = 0;	// attached_to will be phasing in, but hasn't yet
			} else {
				attached_to_is_phased = 1;	// attached_to has already phased out
			}
		} else {
			if (check_special_flags(instance->damage_target_player, instance->damage_target_card, SF_PHASED_OUT|SF_PHASED_OUT_INDIRECTLY) == 0){
				attached_to_is_phased = 0;	// attached_to has already phased in, or isn't phasing at all
			} else {
				attached_to_is_phased = 1;	// attached_to will be phasing out, but hasn't yet
			}
		}
	}

	if (attached_to_is_phased){
		int this_is_phased = instance->state & STATE_OUBLIETTED;
		return (is_token(instance->damage_target_player, instance->damage_target_card)
				|| !this_is_phased);
	} else {
		return 0;
	}
}

static void maybe_unattach_from_phasing(int player, int card, int mode){
	if (check_unattach_from_phasing(player, card, mode)){
		card_instance_t* instance = get_card_instance(player, card);
		if (has_subtype(player, card, SUBTYPE_EQUIPMENT)){
			instance->targets[8].player = instance->targets[8].card = -1;	// raw unattach
		}
		instance->damage_target_player = instance->damage_target_card = -1;
	}
}

static void phase_in_impl(int player, int card, int sf2, int mode){
	// mode = 0: phase in now with all attachments
	// mode = 1: set both SF_PHASED_OUT and SF_PHASED_OUT_INDIRECTLY on this and all phased-out attachments
	// mode = 2: phase this in now by itself
	card_instance_t *instance = get_card_instance(player, card);
	if (instance->internal_card_id > 0
		&& (instance->state & (STATE_OUBLIETTED|STATE_INVISIBLE|STATE_IN_PLAY)) == (STATE_OUBLIETTED|STATE_IN_PLAY)
		&& check_special_flags(player, card, sf2) == sf2){

		if (mode == 1){
			set_special_flags(player, card, SF_PHASED_OUT | SF_PHASED_OUT_INDIRECTLY);
		} else {
			play_sound_effect(WAV_DESTROY);	// the exile sound, despite its name
			instance->state &= ~STATE_OUBLIETTED;
			remove_special_flags(player, card, SF_PHASED_OUT | SF_PHASED_OUT_INDIRECTLY);
			if (get_id(player, card) == CARD_ID_TEFERIS_IMP){
				draw_cards(player, 1);
			}
			if (get_id(player, card) == CARD_ID_WARPING_WURM){
				add_1_1_counter(player, card);
			}
			if (get_id(player, card) == CARD_ID_SHIMMERING_EFREET){
				target_definition_t td;
				default_target_definition(player, card, &td, TYPE_CREATURE);
				td.allow_cancel = 0;

				if (can_target(&td) && pick_target(&td, "TARGET_CREATURE")){
					phase_out(instance->targets[0].player, instance->targets[0].card);
				}
			}
		}

		// attached auras, equipment, fortifications
		if (mode != 2){
			int p, c;
			for (p = 0; p < 2; ++p){
				for (c = 0; c < active_cards_count[p]; ++c){
					instance = get_card_instance(p, c);
					if (instance->internal_card_id > 0
						&& (instance->state & (STATE_OUBLIETTED|STATE_INVISIBLE|STATE_IN_PLAY)) == (STATE_OUBLIETTED|STATE_IN_PLAY)
						&& check_special_flags(p, c, SF_PHASED_OUT_INDIRECTLY)
						&& instance->damage_target_player == player && instance->damage_target_card == card){
						phase_in_impl(p, c, SF_PHASED_OUT_INDIRECTLY, mode);
					}
				}
			}
		}

		if (mode != 1){
			maybe_unattach_from_phasing(player, card, mode);
		}
	}
}

#if 0
void phase_in(int player, int card){
	phase_in_impl(player, card, SF_PHASED_OUT, 0);
}
#endif

static int phase_out_impl(int player, int card, int sf2, int mode){
	// mode = 0: phase out now with all attachments
	// mode = 1: set sf2 on this and SF_PHASED_OUT_INDIRECTLY on all phased-in attachments
	// mode = 2: phase this out now by itself, removing SF_PHASED_OUT_INDIRECTLY if both it and SF_PHASED_OUT are set
	if (in_play(player, card) && !check_special_flags(player, card, SF_CANNOT_PHASE_OUT)){
		card_instance_t* instance = get_card_instance(player, card);

		if (mode != 1){
			play_sound_effect(WAV_DESTROY);	// the exile sound, despite its name
			instance->state |= STATE_OUBLIETTED;
			if (get_id(player, card) == CARD_ID_TEFERIS_IMP){
				discard(player, 0, player);
			}
			if (get_id(player, card) == CARD_ID_ERTAIS_FAMILIAR){
				mill(player, 3);
			}
		}

		if ((sf2 & SF_PHASED_OUT) && (sf2 & SF_PHASED_OUT_INDIRECTLY)){
			sf2 &= ~SF_PHASED_OUT;
		}

		set_special_flags(player, card, sf2);

		if (mode != 2){
			// attached auras, equipment, fortifications
			int p, c;
			for (p = 0; p < 2; ++p){
				for (c = 0; c < active_cards_count[p]; ++c){
					if (in_play(p, c)){
						card_instance_t* attachment = get_card_instance(p, c);
						if (attachment->damage_target_player == player && attachment->damage_target_card == card){
							if (!(cards_data[attachment->internal_card_id].type & TYPE_EFFECT)){
								phase_out_impl(p, c, SF_PHASED_OUT_INDIRECTLY, mode);
							}
						}
					}
				}
			}
		}

		if (mode != 1){
			if (is_token(player, card)){
				obliterate_card(player, card);
			} else {
				maybe_unattach_from_phasing(player, card, mode);
			}
		}

		return 1;
	} else {
		return 0;
	}
}

int phase_out(int player, int card){
	return phase_out_impl(player, card, SF_PHASED_OUT, 0);
}

// There's some mild risk in checking results of new events; picking an unlikely number mitigates it some.
#define RESULT_PHASED_OUT	0x3a962ca
int phasing(int player, int card, event_t event){

	if (event == EVENT_PHASING){
		return phase_out_impl(player, card, SF_PHASED_OUT, 1) ? RESULT_PHASED_OUT : 0;
	}

	return 0;
}

void untap_phasing(int player, int shimmering_lands){
	/* This needs to be done in two passes to deal with the pathological case of a directly-phased-out equipment attached to a creature with phasing always
	 * phases in.  Otherwise it could phase in itself, and then immediately phase back out with the creature. */

	// First pass - just mark cards to phase in or out, so they really do happen simultaneously, and only once
	int c, any_phasing = 0;
	for (c = 0; c < active_cards_count[player]; ++c){
		card_instance_t* instance = get_card_instance(player, c);
		if (instance->internal_card_id >= 0){
			int state = instance->state & (STATE_OUBLIETTED | STATE_INVISIBLE | STATE_IN_PLAY);
			if (state == (STATE_OUBLIETTED | STATE_IN_PLAY)){
				phase_in_impl(player, c, SF_PHASED_OUT, 1);
				any_phasing = 1;
				continue;
			}

			if (shimmering_lands && is_what(player, c, TYPE_LAND)){
				static int shimmer_subtypes[] = {
					SUBTYPE_FOREST,
					SUBTYPE_ISLAND,
					SUBTYPE_MOUNTAIN,
					SUBTYPE_PLAINS,
					SUBTYPE_SWAMP,
					SUBTYPE_DESERT,
					SUBTYPE_GATE,
					SUBTYPE_LAIR,
					SUBTYPE_LOCUS,
					SUBTYPE_MINE,
					SUBTYPE_POWER_PLANT,
					SUBTYPE_TOWER,
					SUBTYPE_URZAS,
					-1,
				};
				int i;
				for (i = 0; shimmer_subtypes[i] >= 0; ++i){
					if ((shimmering_lands & (1<<i)) && has_subtype(player, c, shimmer_subtypes[i])){
						phase_out_impl(player, c, SF_PHASED_OUT, 1);
						any_phasing = 1;
						continue;
					}
				}
			}

			if (state == STATE_IN_PLAY	// the same as normal in_play(), when combined with the checks for internal_card_id, STATE_OUBLIETTED, STATE_INVISIBLE
				&& call_card_function(player, c, EVENT_PHASING) == RESULT_PHASED_OUT){
				any_phasing = 1;
			}
		}
	}

	if (any_phasing){
		// Second pass - phase all marked cards
		for (c = 0; c < active_cards_count[player]; ++c){
			card_instance_t* instance = get_card_instance(player, c);
			if (instance->internal_card_id >= 0){
				int state = instance->state & (STATE_OUBLIETTED | STATE_INVISIBLE | STATE_IN_PLAY);

				// Phase in only if phased out and marked both (from before)
				if (state == (STATE_OUBLIETTED | STATE_IN_PLAY)){
					phase_in_impl(player, c, SF_PHASED_OUT|SF_PHASED_OUT_INDIRECTLY, 2);
					continue;
				}

				// Phase out only if phased in and marked either or both (from before)
				if (state == STATE_IN_PLAY){
					int flags = check_special_flags(player, c, SF_PHASED_OUT|SF_PHASED_OUT_INDIRECTLY);
					if (flags){
						phase_out_impl(player, c, flags, 2);
					}
				}
			}
		}
	}
}

static int old_fetch(int player, int card, event_t event, subtype_t sub1, subtype_t sub2){
	comes_into_play_tapped(player, card, event);
	return fetchland(player, card, event, sub1, sub2, 0);
}

static int generic_mirage_knight(int player, int card, event_t event, int manacolor){

	flanking(player, card, event);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana(player, manacolor, 2) ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE){
			charge_mana(player, manacolor, 2);
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_ability_until_eot(player, card, player, instance->parent_card, 0, 0, KEYWORD_FIRST_STRIKE, 0);
	}

	return 0;
}

static int mirage_diamonds(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	return mana_producer(player, card, event);
}

int mirage_guildmage(int player, int card, event_t event, int color1, int color2){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.preferred_controller = player;
	td1.allowed_controller = player;

	target_definition_t td3;
	default_target_definition(player, card, &td3, TYPE_CREATURE);
	td3.zone = TARGET_ZONE_CREATURE_OR_PLAYER;

	enum{
		CHOICE_PUMP_P = 1,
		CHOICE_TOP_DECK,
		CHOICE_PUMP_T,
		CHOICE_DAMAGE,
		CHOICE_FIRST_STRIKE
	};

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( color1 == COLOR_BLACK || color2 == COLOR_BLACK ){
			if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_B(1), 0, &td, NULL) ){
				return 1;
			}
		}
		if( color1 == COLOR_BLUE || color2 == COLOR_BLUE ){
			if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_U(1), 0, &td1, NULL) ){
				return 1;
			}
		}
		if( color1 == COLOR_GREEN || color2 == COLOR_GREEN ){
			if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_G(1), 0, &td1, NULL) ){
				return 1;
			}
		}
		if( color1 == COLOR_RED || color2 == COLOR_RED ){
			if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(1), 0, &td3, NULL) ){
				return 1;
			}
		}
		if( color1 == COLOR_WHITE || color2 == COLOR_WHITE ){
			if( generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_W(1), 0, &td, NULL) ){
				return 1;
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		int abilities[5] ={ (( color1 == COLOR_BLACK || color2 == COLOR_BLACK ) && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_B(1), 0, &td, NULL)) ? 1 : 0,
							(( color1 == COLOR_BLUE || color2 == COLOR_BLUE ) && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_U(1), 0, &td1, NULL)) ? 1 : 0,
							(( color1 == COLOR_GREEN || color2 == COLOR_GREEN ) && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_G(1), 0, &td1, NULL)) ? 1 : 0,
							(( color1 == COLOR_RED || color2 == COLOR_RED ) && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_R(1), 0, &td3, NULL)) ? 1 : 0,
							(( color1 == COLOR_WHITE || color2 == COLOR_WHITE ) && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_W(1), 0, &td, NULL)) ? 1 : 0,
		};
		int choice = DIALOG(player, card, event, DLG_RANDOM, DLG_NO_STORAGE, DLG_OMIT_ILLEGAL,
							"Pump power", abilities[0], current_phase == PHASE_AFTER_BLOCKING ? 8 : 1,
							"Put on top of deck", abilities[1], 5,
							"Pump toughness", abilities[2], 5,
							"Damage target & yourself", abilities[3], 10,
							"Give First Strike", abilities[4], current_phase == PHASE_AFTER_BLOCKING ? 8 : 1);
		if( ! choice ){
			spell_fizzled = 1;
			return 0;
		}
		instance->number_of_targets = instance->info_slot = 0;
		if( charge_mana_for_activated_ability(player, card, 0,
															(choice == CHOICE_PUMP_P ? 1 : 0),
															(choice == CHOICE_TOP_DECK ? 1 : 0),
															(choice == CHOICE_PUMP_T ? 1 : 0),
															(choice == CHOICE_DAMAGE ? 1 : 0),
															(choice == CHOICE_FIRST_STRIKE ? 1 : 0))
		  ){
			if( choice == CHOICE_PUMP_P ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					tap_card(player, card);
				}
			}
			if( choice == CHOICE_TOP_DECK ){
				if( new_pick_target(&td1, "Select target creature you control.", 0, 1 | GS_LITERAL_PROMPT) ){
					tap_card(player, card);
				}
			}
			if( choice == CHOICE_PUMP_T ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					tap_card(player, card);
				}
			}
			if( choice == CHOICE_DAMAGE ){
				if( pick_target(&td3, "TARGET_CREATURE_OR_PLAYER") ){
					tap_card(player, card);
				}
			}
			if( choice == CHOICE_FIRST_STRIKE ){
				if( pick_target(&td, "TARGET_CREATURE") ){
					tap_card(player, card);
				}
			}
			instance->info_slot = choice;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == CHOICE_PUMP_P && valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 0);
		}
		if( instance->info_slot == CHOICE_TOP_DECK && valid_target(&td1) ){
			put_on_top_of_deck(instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == CHOICE_PUMP_T && valid_target(&td) ){
			pump_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 0, 1);
		}
		if( instance->info_slot == CHOICE_DAMAGE && valid_target(&td3) ){
			damage_target0(player, card, 1);
			damage_player(instance->parent_controller, 1, instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == CHOICE_FIRST_STRIKE && valid_target(&td) ){
			pump_ability_until_eot(instance->parent_controller, instance->parent_card, instance->targets[0].player, instance->targets[0].card,
									0, 0, KEYWORD_FIRST_STRIKE, 0);
		}
	}

	return 0;
}

// ---- CARDS ----
int card_abyssal_hunter(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			tap_card(instance->targets[0].player, instance->targets[0].card);
			damage_creature_or_player(player, card, event, get_power(player, instance->parent_card));
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 1, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

static int acidic_dagger_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_source_player == instance->targets[0].player &&
				damage->damage_source_card == instance->targets[0].card && damage->info_slot > 0 &&
				! has_subtype(damage->damage_target_player, damage->damage_target_card, SUBTYPE_WALL)
			  ){
				if( instance->targets[2].player < 3 ){
					instance->targets[2].player = 3;
				}
				int pos = instance->targets[2].player;
				instance->targets[pos].player = damage->damage_target_player;
				instance->targets[pos].card = damage->damage_target_card;
				instance->targets[2].player++;
			}
		}
	}

	if( instance->info_slot > 4 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) &&
		reason_for_trigger_controller == player ){
		if(event == EVENT_TRIGGER){
			event_result |= 2;
		}
		else if(event == EVENT_RESOLVE_TRIGGER){
				int i;
				for(i=3;i<instance->targets[2].player;i++){
					if( instance->targets[i].player != -1 && instance->targets[i].card != -1 &&
						in_play(instance->targets[i].player, instance->targets[i].card)
					  ){
						kill_card(instance->targets[i].player, instance->targets[i].card, KILL_DESTROY);
					}
				}
				instance->targets[2].player = 3;
		}
	}

	if( leaves_play(instance->targets[0].player, instance->targets[0].card, event) ){
		kill_card(instance->targets[1].player, instance->targets[1].card, KILL_SACRIFICE);
	}

	if( eot_trigger(player, card, event) ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_acidic_dagger(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( current_phase == PHASE_BEFORE_BLOCKING ){
			return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(4), 0, &td, NULL);
		}
	}

	if(event == EVENT_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_TARGET, MANACOST_X(4), 0, &td, NULL);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			int legacy = create_targetted_legacy_effect(player, instance->parent_card, &acidic_dagger_effect, instance->targets[0].player,  instance->targets[0].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[1].player = player;
			leg->targets[1].card = instance->parent_card;
		}
	}

	return 0;
}

int card_afiya_grove(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		add_1_1_counters(player, card, 3);
		instance->targets[1].player = 66;
	}

	if( count_1_1_counters(player, card) > 0 ){
		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			if( pick_target(&td, "TARGET_CREATURE") ){
				add_1_1_counter(instance->targets[0].player,  instance->targets[0].card);
				remove_1_1_counter(player, card);
				instance->number_of_targets = 1;
			}
		}
	}


	if( instance->targets[1].player == 66 && count_1_1_counters(player, card) < 1 ){
		kill_card(player, card, KILL_SACRIFICE);
	}

	return global_enchantment(player, card, event);
}

int card_aleatory(int player, int card, event_t event)
{
  // Cast ~ only during combat after blockers are declared.
  if (event == EVENT_CAN_CAST)
	return current_phase >= PHASE_AFTER_BLOCKING && current_phase < PHASE_MAIN2;

  // Flip a coin. If you win the flip, target creature gets +1/+1 until end of turn.
  // Draw a card at the beginning of the next turn's upkeep.
  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.preferred_controller = player;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  if (flip_a_coin(player, card))
			{
			  card_instance_t* instance = get_card_instance(player, card);
			  alternate_legacy_text(2, player, pump_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 1,1));
			}
		  alternate_legacy_text(1, player, cantrip(player, card, 1));
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_CREATURE", 1, NULL);
}

int card_amber_prison(int player, int card, event_t event)
{
  /* Amber Prison	|4
   * Artifact
   * You may choose not to untap ~ during your untap step.
   * |4, |T: Tap target artifact, creature, or land. That permanent doesn't untap during its controller's untap step for as long as ~ remains tapped. */

  choose_to_untap(player, card, event);

  if (!IS_GAA_EVENT(event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND);

  if (event == EVENT_RESOLVE_ACTIVATION && valid_target(&td))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  int pp = instance->parent_controller, pc = instance->parent_card;

	  tap_card(instance->targets[0].player, instance->targets[0].card);
	  does_not_untap_until_im_tapped(pp, pc, instance->targets[0].player, instance->targets[0].card);

	  card_instance_t* parent = get_card_instance(pp, pc);
	  parent->targets[1].player = instance->targets[0].player;
	  parent->targets[1].card = instance->targets[0].card;
	}

  return generic_activated_ability(player, card, event, GAA_UNTAPPED|GAA_CAN_TARGET, MANACOST_X(4), 0, &td, "ICY_MANIPULATOR");	//"Select target artifact, creature, or land."
}

int card_amulet_of_unmaking(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT | TYPE_CREATURE | TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_SORCERY_BE_PLAYED | GAA_CAN_TARGET | GAA_RFG_ME | GAA_LITERAL_PROMPT,
									MANACOST_X(5), 0, &td, "Select target artifact, creature or land.");
}

int card_afterlife(int player, int card, event_t event){
	/* Afterlife	|2|W
	 * Instant
	 * Destroy target creature. It can't be regenerated. Its controller puts a 1/1 |Swhite Spirit creature token with flying onto the battlefield. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 pick_target(&td, "TARGET_CREATURE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 if( valid_target(&td) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);

				token_generation_t token;
				default_token_definition(player, card, CARD_ID_SPIRIT, &token);
				token.t_player = instance->targets[0].player;
				token.key_plus = KEYWORD_FLYING;
				token.color_forced = COLOR_TEST_WHITE;
				generate_token(&token);
			 }
			 kill_card(player, card, KILL_DESTROY);
   }

   return 0;
}

int card_armorer_guildmage(int player, int card, event_t event){
	return mirage_guildmage(player, card, event, COLOR_BLACK, COLOR_GREEN);
}

int card_ashen_powder(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	char msg[100] = "Select a creature card.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);

	if( event == EVENT_RESOLVE_SPELL ){
		int selected = validate_target_from_grave_source(player, card, 1-player, 0);
		if( selected != -1 ){
			reanimate_permanent(player, card, 1-player, selected, REANIMATE_DEFAULT);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_GRAVE_RECYCLER_OPP_GRAVE, NULL, NULL, 1, &this_test);
}

int card_asmira_holy_avenger(int player, int card, event_t event){
	// original code : 0x453CE0

	/* Asmira, Holy Avenger	|2|G|W
	 * Legendary Creature - Human Cleric 2/3
	 * Flying
	 * At the beginning of each end step, put a +1/+1 counter on ~ for each creature put into your graveyard from the battlefield this turn. */

	check_legend_rule(player, card, event);

	if( eot_trigger(player, card, event) ){
		add_1_1_counters(player, card, get_dead_count(player, TYPE_CREATURE));
	}

	return 0;
}

int card_auspicious_ancestor(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){
		if( (get_color(trigger_cause_controller, trigger_cause) & COLOR_TEST_WHITE) && has_mana(player, COLOR_COLORLESS, 1) ){
			if( event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_AI(player);
			}
			else if( event == EVENT_RESOLVE_TRIGGER){
				charge_mana(player, COLOR_COLORLESS, 1);
				if( spell_fizzled != 1){
					gain_life(player, 1);
				}
			}
		}
	}

  else if( this_dies_trigger(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
	  gain_life(player, 3);
  }

  return 0;
}

int card_bad_river(int player, int card, event_t event){
	/* Bad River	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T, Sacrifice ~: Search your library for |Han Island or |H2Swamp card and put it onto the battlefield. Then shuffle your library. */
	return old_fetch(player, card, event, SUBTYPE_ISLAND, SUBTYPE_SWAMP);
}

static int barbed_foliage_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( eot_trigger(player, card, event) ){
		remove_special_flags(instance->targets[0].player, instance->targets[0].card, SF_FLANKING_REMOVED);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

int card_barbed_foliage(int player, int card, event_t event)
{
  // Whenever a creature attacks you, it loses flanking until end of turn.
  // Whenever a creature without flying attacks you, ~ deals 1 damage to it.
  int amt;
  if ((amt = declare_attackers_trigger(player, card, event, DAT_TRACK | DAT_ATTACKS_PLAYER, 1-player, -1)))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  unsigned char* attackers = (unsigned char*)(&instance->targets[2].player);
	  for (--amt; amt >= 0; --amt)
		if (in_play(current_turn, attackers[amt]))
		  {
			if (check_for_special_ability(current_turn, attackers[amt], SP_KEYWORD_FLANKING))
			  {
				set_special_flags(current_turn, attackers[amt], SF_FLANKING_REMOVED);
				create_targetted_legacy_effect(player, card, &barbed_foliage_effect, current_turn, attackers[amt]);
			  }

			if (!check_for_ability(current_turn, attackers[amt], KEYWORD_FLYING))
			  damage_creature(current_turn, attackers[amt], 1, player, card);
		  }
	}

  return global_enchantment(player, card, event);
}

// wall token --> vanilla

static int basalt_golem_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( end_of_combat_trigger(player, card, event, 2) ){
		if( can_sacrifice(player, instance->targets[0].player, 1, TYPE_CREATURE, 0) ){
			instance->damage_target_player = instance->damage_target_card = -1;
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_WALL, &token);
			token.t_player = instance->targets[0].player;
			token.pow = 0;
			token.tou = 2;
			token.action = TOKEN_ACTION_CONVERT_INTO_ARTIFACT;
			generate_token(&token);
		}
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_basalt_golem(int player, int card, event_t event){

	if(event == EVENT_BLOCK_LEGALITY ){
		if( player == attacking_card_controller && card == attacking_card ){
			if( is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) ){
				event_result = 1;
			}
		}
	}

	if( event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) ){
		int count = active_cards_count[1-player]-1;
		while(count > -1 ){
				if( is_what(1-player, count, TYPE_CREATURE) ){
					card_instance_t *this = get_card_instance(1-player, count);
					if( this->blocking == card ){
						create_targetted_legacy_effect(player, card, &basalt_golem_legacy, 1-player, count);
					}
				}
				count--;
		}
	}

	return 0;
}

int card_bazaar_of_wonders(int player, int card, event_t event){
	/*
	  Bazaar of Wonders |3|U|U
	  World Enchantment
	  When Bazaar of Wonders enters the battlefield, exile all cards from all graveyards.
	  Whenever a player casts a spell, counter it if a card with the same name is in a graveyard or a nontoken permanent with the same name is on the battlefield.
	*/
	card_instance_t *instance = get_card_instance( player, card );

	if( comes_into_play(player, card, event) ){
		int i;
		for(i=0; i<2; i++){
			int count = count_graveyard(i)-1;
			while( count > -1 ){
					rfg_card_from_grave(i, count);
					count--;
			}
		}
	}

	if( specific_spell_played(player, card, event, ANYBODY, RESOLVE_TRIGGER_MANDATORY, TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
		int id = get_id(instance->targets[1].player, instance->targets[1].card);
		int found = 0;
		int i;
		for(i=0; i<2 && ! found; i++){
			int count = count_graveyard(i)-1;
			while( count > -1 && ! found){
					if( cards_data[get_grave(i)[count]].id == id ){
						found = 1;
					}
					count--;
			}
		}
		if( ! found ){
			for(i=0; i<2 && ! found; i++){
				int count = active_cards_count[i]-1;
				while( count > -1 && ! found){
						if( in_play(i, count) && is_what(i, count, TYPE_PERMANENT) && ! is_token(i, count) ){
							if( get_id(i, count) == id ){
								found = 1;
							}
						}
						count--;
				}
			}
		}
		if( found == 1 ){
			real_counter_a_spell(player, card, instance->targets[1].player, instance->targets[1].card);
		}
	}

	return enchant_world(player, card, event);
}

int card_benevolent_unicorn(int player, int card, event_t event){

	if (event == EVENT_PREVENT_DAMAGE){
		card_instance_t* source = get_card_instance(affected_card_controller, affected_card);

		if (source->internal_card_id == damage_card
			&& source->info_slot > 0
			&& is_what(source->damage_source_player, source->damage_source_card, TYPE_SPELL)){
			source->info_slot--;
		}
	}

	return 0;
}

int card_benthic_djinn(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(player, 2);
	}

	return 0;
}

int card_bone_harvest(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 const int *grave = get_grave(player);
			 if( player != AI ){
				while( count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
					  int selected = pick_card_from_graveyard(player, player, "Select a Creature card");
					  if( cards_data[grave[selected]].type & TYPE_CREATURE ){
						 int card_added = add_card_to_hand(player, grave[selected]);
						 put_on_top_of_deck(player, card_added);
						 remove_card_from_grave(player, selected);
					  }
					  else{
						   break;
					  }
				}
			 }
			 else{
				  int count = count_graveyard(player)-1;
				  int total = 0;
				  while( total < 2 && count > -1 ){
					  if( cards_data[grave[count]].type & TYPE_CREATURE ){
						 int card_added = add_card_to_hand(player, grave[count]);
						 put_on_top_of_deck(player, card_added);
						 remove_card_from_grave(player, count);
						 total++;
					  }
					  count--;
				}
			}
			cantrip(player, card, 1);
			kill_card(player, card, KILL_DESTROY);
   }

	return 0;
}

int card_bone_mask(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.extra = damage_card;
	td.special = TARGET_SPECIAL_DAMAGE_PLAYER;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			int *deck = deck_ptr[player];
			int amount = target->info_slot;
			target->info_slot = 0;
			while( amount > 0 && deck[0] != -1 ){
					rfg_top_card_of_deck(player);
					amount--;
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_DAMAGE_PREVENTION | GAA_CAN_TARGET | GAA_UNTAPPED, MANACOST_X(2), 0, &td, "TARGET_DAMAGE");
}

int card_burning_palm_efreet(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 2, player, instance->parent_card);
			negate_ability_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, KEYWORD_FLYING);
		}
	}

	return generic_activated_ability(player, card, event, GAA_CAN_TARGET | GAA_LITERAL_PROMPT, MANACOST_XR(1, 2), 0, &td, "Select target creature with flying.");
}

int card_burning_shield_askari(int player, int card, event_t event){

	return generic_mirage_knight(player, card, event, COLOR_RED);
}

int card_cadaverous_bloom(int player, int card, event_t event){
	if( event == EVENT_CAN_ACTIVATE && hand_count[player] > 0){
		return can_produce_mana(player, card);
	}
	if( event == EVENT_ACTIVATE ){
		card_instance_t *instance= get_card_instance(player, card);
		target_definition_t td;
		default_target_definition(player, card, &td, TYPE_NONE);
		td.allowed_controller = player;
		td.preferred_controller = player;
		td.zone = TARGET_ZONE_HAND;
		td.illegal_abilities = 0;
		if( pick_target(&td, "TARGET_CARD") ){
			put_on_top_of_deck(player, instance->targets[0].card );
			rfg_top_card_of_deck(player);

			produce_mana_all_one_color(player, COLOR_TEST_BLACK | COLOR_TEST_GREEN, 2);
			tapped_for_mana_color = -2;
		}
		instance->number_of_targets = 0;
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && hand_count[player] > 0 && can_produce_mana(player, card) ){
		/* The usual problems with lots of calls to declare_mana_available_hex, detailed in card_axebane_guardian() in return_to_ravnica.c, apply here.  This is
		 * a slightly different case, since the mana is produced in packets of 2.  The approximation I'm going with here declares mana individually for the
		 * first three cards (up to 6 mana), then all in one packet for any remaining, so you don't run out of declaration slots after a 30-card Prosperity. */
		switch (hand_count[player]){
			default:declare_mana_available_hex(player, COLOR_TEST_BLACK | COLOR_TEST_GREEN, 2 * (hand_count[player] - 3));	// break deliberately missing
			case 3: declare_mana_available_hex(player, COLOR_TEST_BLACK | COLOR_TEST_GREEN, 2);	// break deliberately missing
			case 2: declare_mana_available_hex(player, COLOR_TEST_BLACK | COLOR_TEST_GREEN, 2);	// break deliberately missing
			case 1: declare_mana_available_hex(player, COLOR_TEST_BLACK | COLOR_TEST_GREEN, 2);	break;
		}
	}
	return global_enchantment(player, card, event);
}

int card_cadaverous_knight(int player, int card, event_t event){

	flanking(player, card, event);

	return regeneration(player, card, event, 1, 2, 0, 0, 0, 0);
}

int card_canopy_dragon(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_ABILITIES && affect_me(player, card) && instance->info_slot == 66 ){
		if( event_result & KEYWORD_TRAMPLE ){
			event_result &= ~KEYWORD_TRAMPLE;
		}
		event_result |= KEYWORD_FLYING;
	}

	if( event == EVENT_RESOLVE_ACTIVATION){
		card_instance_t *parent = get_card_instance( player, instance->parent_card );
		parent->info_slot = 66;
	}

	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return generic_activated_ability(player, card, event, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0);
}

int card_carrion(int player, int card, event_t event){
	/* Carrion	|1|B|B
	 * Instant
	 * As an additional cost to cast ~, sacrifice a creature.
	 * Put X 0/1 |Sblack Insect creature tokens onto the battlefield, where X is the sacrificed creature's power. */

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;
	td.illegal_abilities = 0;
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			int result = pick_creature_for_sacrifice(player, card, 0);
			if( result > -1  ){
				instance->targets[1].card = get_power(player, result);
				kill_card(player, result, KILL_SACRIFICE);
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_INSECT, &token);
			token.qty = instance->targets[1].card;
			token.pow = 0;
			token.color_forced = COLOR_TEST_BLACK;
			generate_token(&token);
			kill_card(player, card, KILL_DESTROY);
   }

   return 0;
}

int card_catacomb_dragon(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS && is_attacking(player, card) ){
		int count = active_cards_count[1-player]-1;
		while( count > -1 ){
				if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
					card_instance_t *blocker_instance = get_card_instance(1-player, count);
					if( blocker_instance->blocking == card ){
						if( ! has_subtype(1-player, count, SUBTYPE_DRAGON) && ! is_what(1-player, count, TYPE_ARTIFACT) ){
							int actual_pow = get_power(1-player, count);
							pump_until_eot(player, card, 1-player, count, -(actual_pow/2), 0);
						}
					}
				}
				count--;
		}
	}

	return 0;
}

int card_celestial_dawn(int player, int card, event_t event){//UNUSEDCARD

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_SPELL ){
		int i;
		for(i=0; i<2; i++){
			int count = 0;
			while( count < active_cards_count[i] ){
					if( in_play(i, count) && !(i==player && count==card) && is_what(i, count, TYPE_PERMANENT) ){
						set_special_flags2(i, count, SF2_CELESTIAL_DAWN);
					}
					count++;
			}
		}
	}

	if( leaves_play(player, card, event) ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_ANY);
		this_test.id = get_id(player, card);
		this_test.not_me = 1;
		if( ! check_battlefield_for_special_card(player, card, 2, 0, &this_test) ){
			remove_special_flags2(2, -1, SF2_CELESTIAL_DAWN);
		}
	}

	if( event == EVENT_CAST_SPELL && ! affect_me(player, card) && is_what(affected_card_controller, affected_card, TYPE_PERMANENT) ){
		set_special_flags2(affected_card_controller, affected_card, SF2_CELESTIAL_DAWN);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		instance->targets[1].card = get_internal_card_id_from_csv_id(CARD_ID_PLAINS);
	}

	if( event == EVENT_CHANGE_TYPE && instance->targets[1].card > -1 ){
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_LAND) ){
			event_result = instance->targets[1].card;
		}
	}

	if( event == EVENT_MODIFY_COST_GLOBAL && affected_card_controller == player ){
		card_ptr_t* c = cards_ptr[ get_id(affected_card_controller, affected_card)  ];
		int amount = 0;
		if( c->req_black < 16 ){
			amount+=c->req_black;
			COST_BLACK-=c->req_black;
		}
		if( c->req_blue < 16 ){
			amount+=c->req_blue;
			COST_BLUE-=c->req_blue;
		}
		if( c->req_green < 16 ){
			amount+=c->req_green;
			COST_GREEN-=c->req_green;
		}
		if( c->req_red < 16 ){
			amount+=c->req_red;
			COST_RED-=c->req_red;
		}
		COST_WHITE+=amount;
	}

	if( event == EVENT_SET_COLOR && affected_card_controller == player ){
		event_result = COLOR_TEST_WHITE;
	}

	return global_enchantment(player, card, event);
}

int card_chaosphere(int player, int card, event_t event){

	if( ! is_humiliated(player, card) ){
		if( event == EVENT_ABILITIES && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			if( !(event_result & KEYWORD_FLYING) ){
				event_result |= KEYWORD_REACH;
			}
		}

		if( event == EVENT_BLOCK_LEGALITY && ! check_for_ability(attacking_card_controller, attacking_card, KEYWORD_FLYING) ){
			if( check_for_ability(affected_card_controller, affected_card, KEYWORD_FLYING) ){
				event_result = 1;
			}
		}
	}

	return enchant_world(player, card, event);
}

int card_charcoal_diamond(int player, int card, event_t event){

	return mirage_diamonds(player, card, event);
}

static int chariot_of_the_sun_effect(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( instance->targets[0].player > -1 ){
		modify_pt_and_abilities(instance->targets[0].player, instance->targets[0].card, event, 0, 1 - get_base_toughness(instance->targets[0].player, instance->targets[0].card), KEYWORD_FLYING);
	}

	return 0;
}

int card_chariot_of_the_sun(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.allowed_controller = player;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			create_targetted_legacy_effect(player, instance->parent_card, &chariot_of_the_sun_effect, instance->targets[0].player,  instance->targets[0].card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_choking_sands(int player, int card, event_t event)
{
  /* Choking Sands	|1|B|B
   * Sorcery
   * Destroy target non-|H2Swamp land. If that land was nonbasic, ~ deals 2 damage to the land's controller. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_LAND);
  td.required_subtype = get_hacked_subtype(player, card, SUBTYPE_SWAMP);
  td.special = TARGET_SPECIAL_ILLEGAL_SUBTYPE;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  card_instance_t* instance = get_card_instance(player, card);

		  int was_nonbasic = !is_basic_land(instance->targets[0].player, instance->targets[0].card);

		  kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

		  if (was_nonbasic)
			damage_player(instance->targets[0].player, 2, player, card);
		}

	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET|GS_LITERAL_PROMPT, &td,
					   get_hacked_land_text(player, card, "Select target non-%s land.", SUBTYPE_SWAMP), 1, NULL);
}

int card_circle_of_despair(int player, int card, event_t event){

	return prevent_damage_sacrificing_a_creature(player, card, event, 1);
}

int card_civic_guildmage(int player, int card, event_t event){
	return mirage_guildmage(player, card, event, COLOR_BLUE, COLOR_GREEN);
}

int card_crimson_hellkite(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
		damage_creature(instance->targets[0].player, instance->targets[0].card, instance->info_slot, player, instance->parent_card);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, -1, 0, 0, &td, "TARGET_CREATURE");
}

int card_crystal_vein(int player, int card, event_t event)
{
  // |T: Add |1 to your mana pool.
  // |T, Sacrifice ~: Add |2 to your mana pool.
  return sac_land(player, card, event, COLOR_COLORLESS, COLOR_COLORLESS, COLOR_COLORLESS);
}

int card_crystal_golem(int player, int card, event_t event){

	if( current_turn == player && eot_trigger(player, card, event) ){
		phase_out(player, card);
	}

	return 0;
}

int card_cursed_totem(int player, int card, event_t event){

	if( event == EVENT_CAST_SPELL && is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
		disable_all_activated_abilities(affected_card_controller, affected_card, 1);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_DISABLE_ALL_ACTIVATED_ABILITIES);
		manipulate_all(player, card, 1-player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_DISABLE_ALL_ACTIVATED_ABILITIES);
	}

	if( leaves_play(player, card, event) ){
		manipulate_all(player, card, player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_ENABLE_ALL_ACTIVATED_ABILITIES);
		manipulate_all(player, card, 1-player, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0, ACT_ENABLE_ALL_ACTIVATED_ABILITIES);
	}

	return 0;
}

int card_daring_apprentice(int player, int card, event_t event){
	/*
	  Daring Apprentice |1|U|U
	  Creature - Human Wizard 1/1
	  {T}, Sacrifice Daring Apprentice: Counter target spell.
	*/
	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_SPELL_ON_STACK | GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST0) ){
			instance->targets[0].player = card_on_stack_controller;
			instance->targets[0].card = card_on_stack;
			instance->number_of_targets = 1;
			tap_card(player, card);
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( counterspell_validate(player, card, NULL, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return 0;
}

int card_discordant_spirit(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn != player && event == EVENT_PREVENT_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

		if( damage_card != source->internal_card_id || source->info_slot <= 0 || source->damage_target_card != -1 ){
			return 0;
		}

		else{
			 if( instance->targets[1].card < 0 ){
				 instance->targets[1].card = 0;
			 }
			 instance->targets[1].card+= source->info_slot;
		}
	}
	else if( current_turn != player && eot_trigger(player, card, event) ){
			 add_1_1_counters(player, card, instance->targets[1].card);
	}

	else if( current_turn == player && eot_trigger(player, card, event) ){
			 while( count_1_1_counters(player, card) > 0 ){
					remove_1_1_counter(player, card);
			 }
			 instance->targets[1].card = 0;
	}

	return 0;
}

int card_dissipate(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) ){
		return counterspell(player, card, event, NULL, 0);
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( counterspell_validate(player, card, NULL, 0) ){
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

int card_divine_retribution(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, count_attackers(current_turn), player, card);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target attacking creature.", 1, NULL);
}

int card_dream_cache(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		draw_cards(player, 3);
		int max = MIN(2, hand_count[player]);
		if( max ){
			int choice = do_dialog(player, player, card, -1, -1, " Put 2 cards on top\n Put 2 cards on bottom", 1);
			test_definition_t this_test;
			if( choice == 0 ){
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put on top.");
			}
			else{
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card to put on bottom.");
			}
			int mode = choice == 0 ? TUTOR_DECK : TUTOR_BOTTOM_OF_DECK;
			while( max ){
					new_global_tutor(player, player, TUTOR_FROM_HAND, mode, 1, AI_MIN_VALUE, &this_test);
					max--;
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_dream_fighter(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS ){
		if( current_turn == player ){
			if( is_attacking(player, card) ){
				int good = 0;
				int count = active_cards_count[1-player]-1;
				while( count > -1 ){
						if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
							card_instance_t *instance = get_card_instance(1-player, count);
							if( instance->blocking == card ){
								phase_out(1-player, count);
								good = 1;
							}
						}
						count--;
				}
				if( good == 1 ){
					phase_out(player, card);
				}
			}
		}
		else{
			card_instance_t *instance = get_card_instance(player, card);
			if( instance->blocking < 255 ){
				phase_out(1-player, instance->blocking);
				phase_out(player, card);
			}
		}
	}

	return 0;
}

int card_dwarven_miner(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) &&
		has_mana_for_activated_ability(player, card, 2, 0, 0, 0, 1, 0) && ! is_tapped(player, card) && ! is_sick(player, card)
	  ){
		int result = control_nonbasic_land(player) + (2*control_nonbasic_land(1-player));
		if( result > 0 ){
			if( player != AI ){
				return 1;
			}
			else{
				if( result & 2 ){
					return 1;
				}
			}
		}
	}
	else if(event == EVENT_ACTIVATE ){
		if (charge_mana_for_activated_ability(player, card, MANACOST_XR(2,1)) && pick_target_nonbasic_land(player, card, 0)){
			tap_card(player, card);
		}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION && valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}
	return 0;
}

int card_early_harvest(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL){
		if( valid_target(&td) ){
			test_definition_t this_test;
			default_test_definition(&this_test, TYPE_LAND);
			this_test.subtype = SUBTYPE_BASIC;
			new_manipulate_all(player, card, instance->targets[0].player, &this_test, ACT_UNTAP);
		}
		kill_card(player, card, KILL_SACRIFICE);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_elixir_of_vitality(int player, int card, event_t event){

	comes_into_play_tapped(player, card, event);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_SACRIFICE_ME, MANACOST0, 0, NULL, NULL);
	}

	if(event == EVENT_ACTIVATE ){
		int choice = 0;
		if( has_mana_for_activated_ability(player, card, MANACOST_X(8)) ){
			choice = do_dialog(player, player, card, -1, -1, " Gain 4 life\n Gain 8 life\n Cancel", 1);
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		charge_mana_for_activated_ability(player, card, 8*choice, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			tap_card(player, card);
			instance->info_slot = choice+1;
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		gain_life(player, 4+((instance->info_slot-1)*4));
	}

	return 0;
}

int card_emberwilde_djinn(int player, int card, event_t event){
	if( current_turn != player && upkeep_trigger(player, card, event) ){
		int can_choose_life = can_pay_life(1-player, 2);
		int can_choose_mana = has_mana(1-player, COLOR_RED, 2);
		int choice;
		if (can_choose_life){
			if (can_choose_mana){
				choice = do_dialog(1-player, player, card, -1, -1, " Pay 2 life and gain control\n Pay RR and gain control\n Pass", 1);
			} else {
				choice = do_dialog(1-player, player, card, -1, -1, " Pay 2 life and gain control\n _Pay RR and gain control\n Pass", life[1-player] >= 3 ? 0 : 2);
			}
		} else if (can_choose_mana){
			choice = do_dialog(1-player, player, card, -1, -1, " _Pay 2 life and gain control\n Pay RR and gain control\n Pass", 1);
		} else {
			choice = 2;
		}

		if( choice != 2 ){
			if (choice == 0){
				lose_life(1-player, 2);
			} else {
				charge_mana(1-player, COLOR_RED, 2);
			}
			if( spell_fizzled != 1 ){
				give_control(player, card, player, card);
			}
		}
	}
	return 0;
}

int card_emberwilde_caliph(int player, int card, event_t event){

	attack_if_able(player, card, event);

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *source = get_card_instance(affected_card_controller, affected_card);

		if( damage_card != source->internal_card_id || source->info_slot <= 0 ||
			source->damage_source_player != player || source->damage_source_card != card){
			return 0;
		}

		else{
			 lose_life(player, source->info_slot);
		}
	}

	return 0;
}

int card_energy_bolt(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, 0);
	td.zone = TARGET_ZONE_PLAYERS;
	if( life[player] < 6 && player == AI ){
		td.preferred_controller = player;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return generic_spell(player, card, event, GS_X_SPELL | GS_CAN_TARGET, &td, NULL, 1, NULL);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( ! check_special_flags2(player, card, SF2_COPIED_FROM_STACK) ){
			instance->info_slot = x_value;
			set_special_flags2(player, card, SF2_X_SPELL);
			instance->targets[1].player = 0;
			instance->targets[1].player = 1+do_dialog(player, player, card, -1, -1, " Damage player\n Player gains life\n Cancel", life[player] < 6 ? 1 : 0);
			if( instance->targets[1].player == 3 ){
				spell_fizzled = 1;
				return 0;
			}
		}
		pick_target(&td, "TARGET_PLAYER");
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			if( instance->targets[1].player == 1 ){
				damage_target0(player, card, instance->info_slot);
			}
			if( instance->targets[1].player == 2 ){
				gain_life(instance->targets[0].player, instance->info_slot);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_enlightened_tutor(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	else if(event == EVENT_RESOLVE_SPELL ){
			global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, 1, TYPE_ARTIFACT | TYPE_ENCHANTMENT, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_femeref_archers(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, 4, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_femeref_knight(int player, int card, event_t event){
	flanking(player, card, event);
	return generic_shade(player, card, event, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, SP_KEYWORD_VIGILANCE);
}

int card_fire_diamond(int player, int card, event_t event){

	return mirage_diamonds(player, card, event);
}

int card_flash(int player, int card, event_t event){
	if(event == EVENT_CAN_CAST ){
		return 1;
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			char msg[100] = "Select a creature card to put into play.";
			test_definition_t this_test;
			new_default_test_definition(&this_test, TYPE_CREATURE, msg);
			int result = new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
			if( result != -1 ){
				card_ptr_t* c = cards_ptr[ get_id( player, result ) ];
				int colorless = c->req_colorless-2;
				if(colorless < 0 ){ colorless = 0; }
				charge_mana_multi(player, colorless, c->req_black, c->req_blue, c->req_green, c->req_red, c->req_white );
				if( spell_fizzled == 1 ){
					kill_card(player, result, KILL_SACRIFICE);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_flood_plain(int player, int card, event_t event){
	/* Flood Plain	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T, Sacrifice ~: Search your library for |Ha Plains or |H2Island card and put it onto the battlefield. Then shuffle your library. */
	return old_fetch(player, card, event, SUBTYPE_PLAINS, SUBTYPE_ISLAND);
}

int card_floodgate(int player, int card, event_t event){

  if( get_abilities(player, card, EVENT_ABILITIES, -1) & KEYWORD_FLYING ){
	  kill_card(player, card, KILL_SACRIFICE);
  }

  if( leaves_play(player, card, event) ){
	  int damage = count_subtype(player, TYPE_LAND, SUBTYPE_ISLAND)/2;
	  if( damage > 0 ){
		 damage_all(player, card, player, damage, 0, 0, KEYWORD_FLYING, 1, 0, 0, COLOR_TEST_BLUE, 1, 0, 0, -1, 0);
		 damage_all(player, card, 1-player, damage, 0, 0, KEYWORD_FLYING, 1, 0, 0, COLOR_TEST_BLUE, 1, 0, 0, -1, 0);
	  }
  }

  return 0;
}

int card_foratog(int player, int card, event_t event){
	/*
	  Foratog |2|G
	  Creature - Atog 1/2
	  {G}, Sacrifice a Forest: Foratog gets +2/+2 until end of turn.
	*/
	card_instance_t* instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_G(1), 0, NULL, NULL) ){
			return can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0);
		}
	}

	if( event == EVENT_ACTIVATE ){
		if( charge_mana_for_activated_ability(player, card, MANACOST_G(1)) ){
			if( ! sacrifice(player, card, player, 0, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0) ){
				spell_fizzled = 1;
			}
		}
	}

	if (event == EVENT_RESOLVE_ACTIVATION ){
		pump_until_eot(instance->parent_controller, instance->parent_card, instance->parent_controller, instance->parent_card, 2, 2);
	}
	return 0;
}

int card_forbidden_crypt(int player, int card, event_t event)
{
  /* Forbidden Crypt	|3|B|B
   * Enchantment
   * If you would draw a card, return a card from your graveyard to your hand instead. If you can't, you lose the game.
   * If a card would be put into your graveyard from anywhere, exile that card instead. */

  if (trigger_condition == TRIGGER_REPLACE_CARD_DRAW && affect_me(player, card) && reason_for_trigger_controller == player && !suppress_draw)
	{
	  if (event == EVENT_TRIGGER)
		event_result |= RESOLVE_TRIGGER_MANDATORY;

	  if (event == EVENT_RESOLVE_TRIGGER)
		{
		  if (get_grave(player)[0] == -1)
			lose_the_game(player);
		  else
			{
			  test_definition_t this_test;
			  new_default_test_definition(&this_test, 0, "Select a card to return to hand.");
			  new_global_tutor(player, player, TUTOR_FROM_GRAVE, TUTOR_HAND, 0, AI_MAX_VALUE, &this_test);
			  suppress_draw = 1;
			}
		}
	}

  if_a_card_would_be_put_into_graveyard_from_anywhere_exile_it_instead(player, card, event, player, NULL);

  if (event == EVENT_SHOULD_AI_PLAY)
	{
	  ai_modifier += (player == AI ? 12 : -12) * count_graveyard(player);
	  if (player == AI)
		ai_modifier -= count_deck(player) * 48;
	}

  return global_enchantment(player, card, event);
}

int card_forsaken_wastes(int player, int card, event_t event)
{
  /* Forsaken Wastes	|2|B
   * World Enchantment
   * Players can't gain life.
   * At the beginning of each player's upkeep, that player loses 1 life.
   * Whenever ~ becomes the target of a spell, that spell's controller loses 5 life. */

  enchant_world(player, card, event);

  const target_t* targeter = becomes_target_of_spell(player, card, event, player, card, ANYBODY, RESOLVE_TRIGGER_MANDATORY);
  if (targeter)
	lose_life(targeter->player, 5);

  upkeep_trigger_ability(player, card, event, 2);

  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	lose_life(current_turn, 1);

  return global_enchantment(player, card, event);
}

int card_frenetic_efreet(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int flip = flip_a_coin(player, instance->parent_card);
			if( flip == 1 ){
				phase_out(player, instance->parent_card);
			}
			else{
				kill_card(player, instance->parent_card, KILL_SACRIFICE);
			}
	}
	return 0;
}

int card_goblin_elite_infantry(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS ){
		if( current_turn == player ){
			if( is_attacking(player, card) && ! is_unblocked(player, card) ){
				pump_until_eot(player, card, player, card, -1, -1);
			}
		}
		else{
			card_instance_t *instance = get_card_instance(player, card);
			if( instance->blocking < 255 ){
				pump_until_eot(player, card, player, card, -1, -1);
			}
		}
	}

	return 0;
}

// goblin scout --> vanilla

int card_goblin_scouts(int player, int card, event_t event){
	/* Goblin Scouts	|3|R|R
	 * Sorcery
	 * Put three 1/1 |Sred Goblin Scout creature tokens with |H2mountainwalk onto the battlefield. */

	if(event == EVENT_RESOLVE_SPELL ){
		token_generation_t token;
		default_token_definition(player, card, CARD_ID_GOBLIN_SCOUT, &token);
		token.pow = token.tou = 1;
		token.qty = 3;
		token.color_forced = COLOR_TEST_RED;
		generate_token(&token);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_goblin_soothsayer(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) &&
		! is_tapped(player, card) && ! is_sick(player, card)
	  ){
		return can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0);
	}
	if( event == EVENT_ACTIVATE){
		if( charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 1, 0) ){
			if( sacrifice(player, card, player, 0, TYPE_PERMANENT, 0, SUBTYPE_GOBLIN, 0, 0, 0, 0, 0, -1, 0) ){
				tap_card(player, card);
			}
			else{
				spell_fizzled = 1;
			}
		}
	}

	else if( event == EVENT_RESOLVE_ACTIVATION ){
			int z;
			for( z=0; z < 2; z++){
				int count = 0;
				while(count < active_cards_count[z] ){
					if( in_play(z, count) && is_what(z, count, TYPE_CREATURE) &&
						(get_color(z, count) & COLOR_TEST_RED) ){
						pump_until_eot(player, instance->parent_card, z, count, 1, 1);
					}
					count++;
				}
			}
	}
	return 0;
}

int card_goblin_tinkerer(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int cmc = get_cmc(instance->targets[0].player, instance->targets[0].card);
		damage_creature(player, instance->parent_card, cmc, instance->targets[0].player, instance->targets[0].card);
		kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 1, 0, 0, &td1, "TARGET_ARTIFACT");
}

int card_granger_guildmage(int player, int card, event_t event){
	return mirage_guildmage(player, card, event, COLOR_RED, COLOR_WHITE);
}

int card_grasslands(int player, int card, event_t event){
	/* Grasslands	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T, Sacrifice ~: Search your library for |Ha Forest or |H2Plains card and put it onto the battlefield. Then shuffle your library. */
	return old_fetch(player, card, event, SUBTYPE_FOREST, SUBTYPE_PLAINS);
}

int card_grim_feast(int player, int card, event_t event){

	card_instance_t *instance= get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(event == EVENT_GRAVEYARD_FROM_PLAY){
		if( ! in_play(affected_card_controller, affected_card) || affect_me(player, card) ||
			affected_card_controller == player){ return 0; }
		card_instance_t *affected = get_card_instance(affected_card_controller, affected_card);
		if( is_what(affected_card_controller, affected_card, TYPE_CREATURE) && affected->kill_code > 0 &&
			affected->kill_code < 4) {
		   if( instance->targets[11].player < 0 ){
			  instance->targets[11].player = 0;
		   }
		   instance->targets[11].player+=get_toughness(affected_card_controller, affected_card);
		}
	}

	if( instance->targets[11].player > 0 && resolve_graveyard_trigger(player, card, event) == 1 ){
		gain_life(player, instance->targets[11].player);
		instance->targets[11].player = 0;
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		damage_player(player, 1, player, card);
	}

	return global_enchantment(player, card, event);
}

static int can_reanimated_aura(void){
	int result = 0;
	int i;
	for(i=0; i<2; i++){
		result |= (count_graveyard_by_subtype(i, SUBTYPE_AURA_CREATURE) && ! graveyard_has_shroud(i) ? (1+i) : 0);
	}
	return result;
}

int card_hakim_loreweaver(int player, int card, event_t event){

	/* Hakim, Loreweaver	|3|U|U
	 * Legendary Creature - Human Wizard 2/4
	 * Flying
	 * |U|U: Return target Aura card from your graveyard to the battlefield attached to ~. Activate this ability only during your upkeep and only if ~ isn't
	 * enchanted.
	 * |U|U, |T: Destroy all Auras attached to ~. */

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, 0, MANACOST_U(2), 0, NULL, NULL) ){
			if( ! is_enchanted(player, card) && any_in_graveyard_by_subtype(player, SUBTYPE_AURA_CREATURE) && ! graveyard_has_shroud(2) &&
				current_phase == PHASE_UPKEEP && current_turn == player && can_reanimated_aura()
			  ){
				return 1;
			}
		}
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_U(2), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE ){
		int choice = instance->info_slot = instance->number_of_targets = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, 0, MANACOST_U(2), 0, NULL, NULL) ){
			if( ! is_enchanted(player, card) && any_in_graveyard_by_subtype(player, SUBTYPE_AURA_CREATURE) && ! graveyard_has_shroud(2) &&
				current_phase == PHASE_UPKEEP && current_turn == player && can_reanimated_aura()
			  ){
				if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_U(2), 0, NULL, NULL) ){
					choice = do_dialog(player, player, card, -1, -1, " Reanimate Aura\n Destroy all attached Auras\n Cancel", 1);
				}
			}
			else{
				choice = 1;
			}
		}
		else{
			choice = 1;
		}
		if( choice == 2 ){
			spell_fizzled = 1;
			return 0;
		}
		instance->info_slot = 66+choice;
		if( charge_mana_for_activated_ability(player, card, MANACOST_U(2)) ){
			if( choice == 0 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ENCHANTMENT, "Select target Aura card with enchant creature.");
				this_test.subtype = SUBTYPE_AURA_CREATURE;
				int result = can_reanimated_aura();
				if( result == 3 ){
					select_target_from_either_grave(player, card, 0, AI_MAX_CMC, AI_MAX_CMC, &this_test, 0, 1);
				}
				else{
					instance->targets[0].player = result-1;
					if( select_target_from_grave_source(player, card, result-1, 0, AI_MAX_CMC, &this_test, 1) == -1 ){
						spell_fizzled = 1;
						return 0;
					}
				}
			}
			if( choice == 1 ){
				tap_card(player, card);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 ){
			int selected = validate_target_from_grave_source(player, card, instance->targets[0].player, 1);
			if( selected != -1 ){
				reanimate_permanent(player, instance->parent_card, instance->targets[0].player, selected, REANIMATE_ATTACH_AS_AURA);
			}
		}
		if( instance->info_slot == 67 ){
			int i;
			for(i=0; i<2; i++){
				int count = active_cards_count[i]-1;
				while(count > -1 ){
						if( in_play(i, count) && is_what(i, count, TYPE_ENCHANTMENT) && has_subtype(i, count, SUBTYPE_AURA) ){
							card_instance_t *aura = get_card_instance(i, count);
							if( aura->damage_target_player == instance->parent_controller && aura->damage_target_card == instance->parent_card ){
								kill_card(i, count, KILL_DESTROY);
							}
						}
						count--;
				}
			}
		}
	}

	return 0;
}

int card_hammer_of_bogardan(int player, int card, event_t event){

	/* Hammer of Bogardan	|1|R|R
	 * Sorcery
	 * ~ deals 3 damage to target creature or player.
	 * |2|R|R|R: Return ~ from your graveyard to your hand. Activate this ability only during your upkeep. */

	if( event == EVENT_GRAVEYARD_ABILITY_UPKEEP ){
		if( ! has_mana_multi(player, 2, 0, 0, 0, 3, 0) ){
			return -2;
		}
		int choice = do_dialog(player, player, card, -1, -1," Return Hammer to hand\n Leave Hammer be\n", 0);
		if( choice == 0 ){
			charge_mana_multi(player, 2, 0, 0, 0, 3, 0);
			if( spell_fizzled != 1 ){
				card_instance_t* instance = get_card_instance(player, card);
				instance->state &= ~STATE_INVISIBLE;
				hand_count[player]++;
				return -1;
			}
			return -2;
		}
		else{
			return -2;
		}
	}
	return card_volcanic_hammer(player, card, event);
}

int card_harbinger_of_night(int player, int card, event_t event){

	/* Harbinger of Night	|2|B|B
	 * Creature - Spirit 2/3
	 * At the beginning of your upkeep, put a -1/-1 counter on each creature. */

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		manipulate_type(player, card, ANYBODY, TYPE_CREATURE, ACT_ADD_COUNTERS(COUNTER_M1_M1, 1));
	}

	return 0;
}

int card_harmattan_efreet(int player, int card, event_t event){
	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	return vanilla_creature_pumper(player, card, event, MANACOST_XU(1, 2), 0, 0, 0, KEYWORD_FLYING, 0, &td);
}

int card_hivis_of_the_scale(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.required_subtype = SUBTYPE_DRAGON;
	td1.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			card_instance_t *parent = get_card_instance( player, instance->parent_card );
			gain_control(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card);
			parent->targets[1] = instance->targets[0];
		}
	}

	if( current_phase == PHASE_UNTAP && current_turn == player && event == EVENT_UNTAP && affect_me(player, card) ){
		int ai_choice = 0;
		if( instance->targets[1].player > -1 ){
			ai_choice = 1;
		}
		int choice = do_dialog(player, player, card, -1, -1, " Leave tapped\n Untap", ai_choice);
		if( choice == 0 ){
			instance->untap_status &= ~3;
		}
	}

	if( instance->targets[1].player > -1 && ! in_play(instance->targets[1].player, instance->targets[1].card) ){
		instance->targets[1].player = -1;
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td1, "TARGET_PERMANENT");
}

int card_illumination(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
	   int result = card_counterspell(player, card, event);
	   if( result > 0 && (is_what(card_on_stack_controller, card_on_stack, TYPE_ARTIFACT) ||
						  is_what(card_on_stack_controller, card_on_stack, TYPE_ENCHANTMENT)) ){
		   instance->targets[1].player = card_on_stack_controller;
		   instance->targets[1].card = get_cmc(card_on_stack_controller, card_on_stack);
		   return result;
	   }
	   else{
			return 0;
	   }
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			set_flags_when_spell_is_countered(player, card, instance->targets[1].player, instance->targets[1].card);
			gain_life(instance->targets[1].player, instance->targets[1].card);
			return card_counterspell(player, card, event);
   }

   return card_counterspell(player, card, event);
}

int card_infernal_contract(int player, int card, event_t event){
	if( event == EVENT_CAN_CAST ){
		return can_pay_life(player, 1);
	}
	if( event == EVENT_RESOLVE_SPELL ){
		int amount = (life[player]+1)/2;
		lose_life(player, amount);
		draw_cards(player, 4);
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_jolraels_centaur(int player, int card, event_t event){

	flanking(player, card, event);

	return 0;
}

int card_wood2(int player, int card, event_t event){


	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE){
			produce_mana(player, COLOR_RED, 1);
			kill_card(player, card, KILL_SACRIFICE);
	}

	else if( event == EVENT_COUNT_MANA && in_play(player, card) ){
			declare_mana_available(player, COLOR_RED, 1);
	}

	return 0;
}

static const char* is_wood(int who_chooses, int player, int card){
	if( get_id(player, card) == CARD_ID_WOOD ){
		return NULL;
	}
	return "you must choose a Wood card.";
}

int card_jungle_patrol(int player, int card, event_t event){
	/* Jungle Patrol	|3|G
	 * Creature - Human Soldier 3/2
	 * |1|G, |T: Put a 0/1 |Sgreen Wall creature token with defender named Wood onto the battlefield.
	 * Sacrifice a token named Wood: Add |R to your mana pool. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( can_produce_mana(player, card) && can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, CARD_ID_WOOD, 0, -1, 0) ){
			return 1;
		}
		return generic_activated_ability(player, card, event, GAA_UNTAPPED, MANACOST_XG(1, 1), 0, NULL, NULL);
	}

	if( event == EVENT_ACTIVATE){
		instance->info_slot = 0;
		int choice = 0;
		if( can_produce_mana(player, card) && can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, CARD_ID_WOOD, 0, -1, 0) ){
			if( ! paying_mana() && generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_UNTAPPED, MANACOST_XG(1, 1), 0, NULL, NULL)
			  ){
				choice = do_dialog(player, player, card, -1, -1, " Get mana\n Generate a Wood\n Cancel", 1);
			}
		}
		else{
			choice = 1;
		}

		if( choice == 0 ){
			instance->number_of_targets = 0;

			target_definition_t td;
			default_target_definition(player, card, &td, TYPE_PERMANENT);
			td.special = TARGET_SPECIAL_EXTRA_FUNCTION;
			td.extra = (int32_t)is_wood;
			td.allowed_controller = player;
			td.preferred_controller = player;
			td.illegal_abilities = 0;
			if( new_pick_target(&td, "Select a Wood to sacrifice.", 0, 1 | GS_LITERAL_PROMPT) ){
				kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
				produce_mana(player, COLOR_RED, 1);
			}
		}
		else if( choice == 1 ){
				if( charge_mana_for_activated_ability(player, card, MANACOST_XG(1, 1)) ){
					tap_card(player, card);
					instance->info_slot = 1;
				}
		}
		else{
			spell_fizzled = 1;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 ){
			generate_token_by_id(player, card, CARD_ID_WOOD);
		}
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card) &&
		can_sacrifice_as_cost(player, 1, TYPE_PERMANENT, 0, 0, 0, 0, 0, CARD_ID_WOOD, 0, -1, 0)
	  ){
		declare_mana_available(player, COLOR_RED, 1);
	}

	return 0;
}

int card_jungle_wurm(int player, int card, event_t event){

	if( event == EVENT_DECLARE_BLOCKERS ){
		if( current_turn == player ){
			if( is_attacking(player, card) ){
				int count = active_cards_count[1-player]-1;
				while( count > -1 ){
						int max = 0;
						if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) ){
							card_instance_t *instance = get_card_instance(1-player, count);
							if( instance->blocking == card ){
								max++;
								if( max > 1 ){
									pump_ability_until_eot(player, card, player, card, -1, -1, 0, 0);
								}
							}
						}
						count--;
				}
			}
		}
	}

	return 0;
}

int card_kukemssa_pirates(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allowed_controller =  1-player;
	td.preferred_controller = 1-player;

	if( event == EVENT_DECLARE_BLOCKERS && (instance->state & STATE_ATTACKING) && is_unblocked(player, card) &&
		can_target(&td) && pick_target(&td, "TARGET_ARTIFACT")
	  ){
		gain_control(player, card, instance->targets[0].player, instance->targets[0].card);
		instance->info_slot = 66;
	}

	card_instance_t* damage;
	if( instance->info_slot == 66 &&
		(damage = combat_damage_being_prevented(event)) &&
		damage->damage_source_player == player && damage->damage_source_card == card
	  ){
		damage->info_slot = 0;
	}

	if(event == EVENT_CLEANUP){
		instance->info_slot = 0;
	}

	return 0;
}

int card_lions_eye_diamond(int player, int card, event_t event){
	if( event == EVENT_COUNT_MANA && affect_me(player, card) && can_produce_mana(player, card) ){
		if( can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return card_black_lotus(player, card, event);
		}
	}
	if( event == EVENT_CAN_ACTIVATE && can_produce_mana(player, card) ){
		if( can_sacrifice_as_cost(player, 1, TYPE_ARTIFACT, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
			return card_black_lotus(player, card, event);
		}
	}
	else if( event == EVENT_ACTIVATE && spell_fizzled != 1){
			discard_all(player);
			return card_black_lotus(player, card, event);
	}
	else{
		return card_black_lotus(player, card, event);
	}
	return 0;
}

int card_locust_swarm(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_REGENERATION, MANACOST_G(1), 0, NULL, NULL) ){
			return 99;
		}
		if( generic_activated_ability(player, card, event, GAA_ONCE_PER_TURN, MANACOST_G(1), 0, NULL, NULL) ){
			return 1;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 0;
		int result = generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_REGENERATION, MANACOST_G(1), 0, NULL, NULL) ? 1 : 2;
		if( charge_mana_for_activated_ability(player, card, MANACOST_G(1)) ){
			instance->info_slot = result;
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 1 && can_regenerate(instance->parent_controller, instance->parent_card) ){
			regenerate_target(instance->parent_controller, instance->parent_card);
		}
		if( instance->info_slot == 2 ){
			untap_card(instance->parent_controller, instance->parent_card);
		}
	}

	if(event == EVENT_CLEANUP){
		instance->targets[2].player = 0;
	}

	return 0;
}

int card_lure_of_prey(int player, int card, event_t event){

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, get_sleighted_color_text(player, card, "Select a %s creature card.", COLOR_GREEN));
	this_test.color = get_sleighted_color_test(player, card, COLOR_TEST_GREEN);
	this_test.zone = TARGET_ZONE_HAND;

	if( event == EVENT_CAN_CAST && get_stormcreature_count(1-player) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( player == AI && ! check_battlefield_for_special_card(player, card, player, 0, &this_test) ){
				ai_modifier-=100;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			new_global_tutor(player, player, TUTOR_FROM_HAND, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_malignant_growth(int player, int card, event_t event){

	/* Malignant Growth	|3|G|U
	 * Enchantment
	 * Cumulative upkeep |1
	 * At the beginning of your upkeep, put a growth counter on ~.
	 * At the beginning of each opponent's draw step, that player draws an additional card for each growth counter on ~, then ~ deals damage to the player equal
	 * to the number of cards he or she drew this way. */

	cumulative_upkeep(player, card, event, MANACOST_X(1));

	if (event == EVENT_UPKEEP_TRIGGER_ABILITY){	// in addition to cumulative upkeep
		add_counter(player, card, COUNTER_GROWTH);
	}

	if( current_turn != player && event == EVENT_DRAW_PHASE ){
		int growth_counters = count_counters(player, card, COUNTER_GROWTH);
		event_result += growth_counters;
		damage_player(1-player, growth_counters, player, card);
	}

	return global_enchantment(player, card, event);
}

int card_marble_diamond(int player, int card, event_t event){

	return mirage_diamonds(player, card, event);
}

int card_maro(int player, int card, event_t event)
{
  /* Maro	|2|G|G
   * Creature - Elemental 100/100
   * ~'s power and toughness are each equal to the number of cards in your hand. */

  if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && !is_humiliated(player, card) && player != -1)
	event_result += hand_count[player];

  return 0;
}

int card_mind_bend(int player, int card, event_t event){
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	card_instance_t *instance = get_card_instance( player, card);
	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 1;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		int choice = do_dialog(player, player, card, -1, -1, " Change color word\n Change land word", 0);
		instance->targets[3].player = choice;

		if( instance->targets[3].player == 1 ){
			return card_magical_hack(player, card, event);
		}
		else if( instance->targets[3].player == 0 ){
			return card_sleight_of_mind(player, card, event);
		}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( instance->targets[3].player == 1 ){
				return card_magical_hack(player, card, event);
			}
			else if( instance->targets[3].player == 0 ){
				return card_sleight_of_mind(player, card, event);
			}
	}

	return 0;
}

static void add_fungus_counters(int player, int card, int blocked_controller, int blocked_card)
{
  add_counters(blocked_controller, blocked_card, COUNTER_FUNGUS, 4);
  gains_doesnt_untap_while_has_a_counter_and_remove_a_counter_at_upkeep(player, card, blocked_controller, blocked_card, COUNTER_FUNGUS);
}
int card_mindbender_spores(int player, int card, event_t event)
{
  /* Mindbender Spores	|2|G
   * Creature - Fungus Wall 0/1
   * Defender
   * Flying
   * Whenever ~ blocks a creature, put four fungus counters on that creature. The creature gains "This creature doesn't untap during your untap step if it has a
   * fungus counter on it" and "At the beginning of your upkeep, remove a fungus counter from this creature." */

  if (event == EVENT_DECLARE_BLOCKERS && blocking(player, card, event))
	for_each_creature_blocked_by_me(player, card, add_fungus_counters, player, card);

  return 0;
}

int card_misers_cage(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 1-player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( hand_count[1-player] > 4 ){
			damage_player(1-player, 2, player, card);
		}
	}

	return 0;
}

int card_mist_dragon(int player, int card, event_t event){

	if (event == EVENT_CAN_ACTIVATE){
		return can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(0));
	}

	card_instance_t* instance = get_card_instance(player, card);

	if (event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION){
		int choice = DIALOG(player, card, event,
							"Gain flying", 1, 2,
							"Lose flying", 1, 1,
							"Phase out", has_mana_for_activated_ability(player, card, MANACOST_XU(3,2)), 3);

		if (event == EVENT_ACTIVATE){
			if (choice == 3)
				charge_mana_for_activated_ability(player, card, MANACOST_XU(3,2));
		} else {	// event == EVENT_RESOLVE_ACTIVATION
			if (!in_play(player, instance->parent_card)){
				spell_fizzled = 1;
			} else {
				card_instance_t* parent = get_card_instance(player, instance->parent_card);
				switch (choice){
					case 1:	parent->targets[1].player = 66;	break;
					case 2:	parent->targets[1].player = 0;	break;
					case 3:	phase_out(player, instance->parent_card);	break;
				}
			}
		}
	}

	if( instance->targets[1].player == 66 && event == EVENT_ABILITIES && affect_me(player, card) ){
		event_result |= KEYWORD_FLYING;
	}

	return 0;
}

int card_moss_diamond(int player, int card, event_t event){

	return mirage_diamonds(player, card, event);
}

int card_mountain_valley(int player, int card, event_t event){
	/* Mountain Valley	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T, Sacrifice ~: Search your library for |Ha Mountain or |H2Forest card and put it onto the battlefield. Then shuffle your library. */
	return old_fetch(player, card, event, SUBTYPE_MOUNTAIN, SUBTYPE_FOREST);
}

int card_mtenda_griffin(int player, int card, event_t event){

	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_PERMANENT, "Select a Griffin card.");
	this_test.subtype = SUBTYPE_GRIFFIN;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_sick(player, card) ){
		if( has_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1) && new_special_count_grave(player, &this_test) && current_phase == PHASE_UPKEEP &&
			current_turn == player
		  ){
			return ! graveyard_has_shroud(2);
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 0, 0, 1);
		if( spell_fizzled != 1 ){
			int selected = new_select_target_from_grave(player, card, player, 0, AI_MAX_VALUE, &this_test, 0);
			if( selected != -1 ){
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
			bounce_permanent(player, instance->parent_card);
			from_grave_to_hand(player, selected, TUTOR_HAND);
		}
	}

	return 0;
}

int card_mtenda_lion(int player, int card, event_t event)
{
  // Whenever ~ attacks, defending player may pay |U. If that player does, prevent all combat damage that would be dealt by ~ this turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card)
	  && has_mana(1-current_turn, COLOR_BLUE, 1)
	  && do_dialog(1-current_turn, player, card, -1, -1, " Pay U\n Decline", 0) == 0
	  && charge_mana_while_resolving(player, card, EVENT_RESOLVE_TRIGGER, 1-current_turn, COLOR_BLUE, 1))
	negate_combat_damage_this_turn(player, card, player, card, 0);

  return 0;
}

int card_mystical_tutor(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_SPELL, "Select an instant or sorcery card.");
		new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_DECK, 0, AI_MAX_VALUE, &this_test);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_natural_balance(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_SPELL ){
		APNAP(p, {
				int amount = count_subtype(p, TYPE_LAND, -1);
				if( amount > 5 ){
					impose_sacrifice(player, card, p, amount-5, TYPE_LAND, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				}
				else if( amount < 5 ){
						test_definition_t this_test;
						new_default_test_definition(&this_test, TYPE_LAND, "Select a basic land card.");
						this_test.subtype = SUBTYPE_BASIC;
						this_test.qty = 5-amount;
						new_global_tutor(p, p, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_FIRST_FOUND, &this_test);
						if( 5-amount > 1 ){
							shuffle(p);
						}
				}
			};
		);
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_null_chamber(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card);

	if( event == EVENT_RESOLVE_SPELL ){
			int human_selection = -1;
			if( ai_is_speculating != 1 ){
				while(1){
					human_selection = choose_a_card("Choose a card", -1, -1);
					if( ! ( cards_data[ human_selection ].type & TYPE_LAND ) ){
						break;
					}
				}
			}
			if( human_selection != -1 ){
				create_card_name_legacy(player, card, cards_data[ human_selection ].id );
			}
			instance->targets[1].player = human_selection;

			int *deck = deck_ptr[HUMAN];
			int rnd = internal_rand(count_deck(HUMAN));
			while( is_what(-1, deck[rnd], TYPE_LAND) ){
					rnd++;
					if( deck[rnd] == -1 ){
						rnd = 0;
					}
			}
			instance->targets[2].player = deck[rnd];
			create_card_name_legacy(player, card, cards_data[ deck[rnd] ].id );
	}
	if( event == EVENT_MODIFY_COST_GLOBAL ){
		int i;
		for(i=1; i<3; i++ ){
			if( cards_data[instance->targets[i].player].id == get_id(affected_card_controller, affected_card) ){
				infinite_casting_cost();
			}
		}
	}
	return global_enchantment(player, card, event);
}

int card_paupers_cage(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, 1-player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( hand_count[1-player] < 3 ){
			damage_player(1-player, 2, player, card);
		}
	}

	return 0;
}

int card_pacifism(int player, int card, event_t event)
{
  if (event == EVENT_ABILITIES && in_play(player, card))
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->damage_target_player >= 0 && instance->damage_target_card >= 0)
		{
		  cannot_attack(instance->damage_target_player, instance->damage_target_card, event);
		  cannot_block(instance->damage_target_player, instance->damage_target_card, event);
		}
	}

  return disabling_aura(player, card, event);
}

int card_patagia_golem(int player, int card, event_t event){
	/*
	  Patagia Golem |4
	  Artifact Creature - Golem 2/3
	  {3}: Patagia Golem gains flying until end of turn.
	*/

	return generic_shade(player, card, event, 0, MANACOST_X(3), 0, 0, KEYWORD_FLYING, 0);
}

int card_pearl_dragon(int player, int card, event_t event){
	return generic_shade_merge_pt(player, card, event, 0, MANACOST_XW(1, 1), 0,1);
}

int card_phyrexian_dreadnought(int player, int card, event_t event){

	if( trigger_condition == TRIGGER_COMES_INTO_PLAY && affect_me(player, card) &&
		reason_for_trigger_controller == player ){

		int trig = 0;

		if( trigger_cause_controller == player && trigger_cause == card ){
			trig = 1;
		}

		if( trig == 1 && check_battlefield_for_id(2, CARD_ID_TORPOR_ORB) ){
			trig = 0;
		}

		if( trig == 1 ){
			if( event == EVENT_TRIGGER){
				event_result |= 2;
			}
			else if( event == EVENT_RESOLVE_TRIGGER ){
					card_instance_t *instance = get_card_instance(player, card);
					instance->state |= STATE_CANNOT_TARGET;
					target_definition_t td;
					default_target_definition(player, card, &td, TYPE_CREATURE);
					td.allowed_controller = player;
					td.preferred_controller = player;
					td.illegal_abilities = 0;
					int total_power = 0;

					int i=0;
					while( can_target(&td) && spell_fizzled != 1 ){
						if( i > 0 ){
							instance->targets[12] = instance->targets[0];
						}
						if( pick_target(&td, "LORD_OF_THE_PIT") ){
							card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
							target->state |= STATE_CANNOT_TARGET;
							if( i > 0 ){
								instance->targets[i] = instance->targets[0];
								instance->targets[0] = instance->targets[12];
							}
							i++;
						}
						else if (i > 0 ){
							instance->targets[0] = instance->targets[12];
						}
					 }

					int j=0;
					for(j=0;j<i;j++){
						total_power += get_power( instance->targets[j].player, instance->targets[j].card);
						kill_card( instance->targets[j].player, instance->targets[j].card, KILL_SACRIFICE);
					}
					if( total_power < 12 ){
						kill_card(player, card, KILL_SACRIFICE);
					}

					instance->state &= ~STATE_CANNOT_TARGET;
			}
		}
	}

	return 0;
}

int card_phyrexian_purge(int player, int card, event_t event){

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller =  2;
  td.preferred_controller = 1-player;
  td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_pay_life(player, 3);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 int minlife = 3;
			 if( player == AI ){
				 minlife = 5;
			 }
			 int target_count = 0;

			 while( can_target(&td) && life[player]>= minlife ){
				   pick_target(&td, "TARGET_CREATURE");
				   instance->targets[target_count+1].player = instance->targets[0].player;
				   instance->targets[target_count+1].card = instance->targets[0].card;
				   target_count++;
				   card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
				   target->state |= STATE_CANNOT_TARGET;
				   lose_life(player, 3);
				   if( target_available(player, card, &td) > target_count ){
					   int choice = do_dialog(player, player, card, -1, -1, " Continue\n Stop", 0);
					   if( choice == 1 ){;
						   break;
					   }
				   }
			 }

			 if( target_count > 0 ){
				 int k;
				 for(k=0; k<target_count; k++){
					 card_instance_t *target = get_card_instance(instance->targets[k+1].player, instance->targets[k+1].card);
					 target->state &= ~STATE_CANNOT_TARGET;
				 }
				 instance->info_slot = target_count;
			 }
			 else{
				  spell_fizzled = 1;

			 }
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 int target_count = instance->info_slot;
			 int k;
			  for(k=0; k<target_count; k++){
				  if( validate_target(player, card, &td, k+1) ){
					  kill_card( instance->targets[k+1].player, instance->targets[k+1].card, KILL_DESTROY);
				  }
			  }
			  kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_phyrexian_tribute(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);
	td.allow_cancel = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_sacrifice_as_cost(player, 2, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			if( sacrifice(player, card, player, 0, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0) ){
				impose_sacrifice(player, card, player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
				pick_target(&td, "TARGET_ARTIFACT");
			}
			else{
				spell_fizzled = 1;
			}
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				kill_card( instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_phyrexian_vault(int player, int card, event_t event){

	if( event == EVENT_RESOLVE_ACTIVATION ){
		draw_cards(player, 1);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_SACRIFICE_CREATURE, 2, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_political_trickery(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
	td.preferred_controller = player;
	td.allowed_controller = player;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_LAND );
	td1.preferred_controller = 1-player;
	td1.allowed_controller = 1-player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return can_target(&td1);
	}

	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->number_of_targets = 0;
		if( new_pick_target(&td, "Select target land you control.", 0, 1 | GS_LITERAL_PROMPT) ){
			new_pick_target(&td1, "Select target land opponent controls.", 1, 1 | GS_LITERAL_PROMPT);
		}
	}

	if( event == EVENT_RESOLVE_SPELL ){
		if( validate_target(player, card, &td, 0) && validate_target(player, card, &td1, 1) ){
			exchange_control_of_target_permanents(player, card, instance->targets[0].player, instance->targets[0].card,
													instance->targets[1].player, instance->targets[1].card);
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_polymorph(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance( player, card );
	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.preferred_controller = player;

	if(event == EVENT_CAN_CAST){
		return can_target(&td);
	}

	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			pick_target(&td, "TARGET_CREATURE");
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				metamorphosis(instance->targets[0].player, instance->targets[0].card, TYPE_CREATURE, KILL_BURY);
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_preferred_selection(int player, int card, event_t event){

	if( current_turn == player && upkeep_trigger(player, card, event) ){
	}

	int *deck = deck_ptr[player];

	if( deck[0] != -1 ){
		upkeep_trigger_ability(player, card, event, player);

		if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
			int amount = 2;
			if( deck[1] == -1 ){
				amount = 1;
			}
			int selected = -1;
			while( selected == -1 ){
				   selected = show_deck( player, deck, amount, "Select a card", 0, 0x7375B0 );
			}

			int mode = 0;

			if( has_mana_multi(player, 2, 0, 0, 2, 0, 0) ){
				charge_mana_multi(player, 2, 0, 0, 2, 0, 0);
				if( spell_fizzled !=1 ){
					mode = 1;
				}
			}

			if( mode == 0 ){
				deck[count_deck(player)] = deck[selected];
				remove_card_from_deck(player, selected);
			}
			else if( mode == 1 ){
					 add_card_to_hand(player, deck[selected]);
					 remove_card_from_deck(player, selected);
					 kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}

	return global_enchantment(player, card, event);
}

int card_prismatic_boon(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) && has_mana(player, COLOR_COLORLESS, 1) ){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 int target_count = 0;
			 while( can_target(&td) && has_mana(player, COLOR_COLORLESS, 1) ){
					if( target_count > 0 ){
						td.allow_cancel = 0;
					}
					pick_target(&td, "TARGET_CREATURE");
					instance->targets[target_count+1].player = instance->targets[0].player;
					instance->targets[target_count+1].card = instance->targets[0].card;
					target_count++;
					charge_mana(player, COLOR_COLORLESS, 1);
					if( spell_fizzled != 1 ){
						state_untargettable(instance->targets[0].player, instance->targets[0].card, 1);
						if( target_available(player, card, &td) > target_count ){
							int choice = do_dialog(player, player, card, -1, -1, " Continue\n Stop", 0);
							if( choice == 1 ){;
								break;
							}
						}
					}
			 }

			 if( target_count > 0 ){
				 int k;
				 for(k=0; k<target_count; k++){
					 card_instance_t *target = get_card_instance(instance->targets[k+1].player, instance->targets[k+1].card);
					 target->state &= ~STATE_CANNOT_TARGET;
				 }
				 instance->info_slot = target_count;
			 }
			 else{
				  spell_fizzled = 1;

			 }
	}
	else if(event == EVENT_RESOLVE_SPELL ){
			int target_count = instance->info_slot;
			int keyword = select_a_protection(player) | KEYWORD_RECALC_SET_COLOR;
			int k;
			for(k=0; k<target_count; k++){
				if( validate_target(player, card, &td, k+1) ){
					  pump_ability_until_eot(player, card, instance->targets[k+1].player, instance->targets[k+1].card, 0, 0, keyword, 0);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_psychic_transfer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_LAND );
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
		if( valid_target(&td)  ){
			int value = life[1-player]-life[player];
			if( life[player]-life[1-player] > value ){
				value = life[player]-life[1-player];
			}
			if( value > 0 && value < 6 ){
				value = life[player];
				set_life_total(player, life[1-player]);
				set_life_total(1-player, value);
			}
		}
		kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_purgatory(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_GRAVEYARD_FROM_PLAY && in_play(affected_card_controller, affected_card) ){
		if( affected_card_controller == player && is_what(affected_card_controller, affected_card, TYPE_CREATURE) &&
			! is_token(affected_card_controller, affected_card) && ! is_stolen(affected_card_controller, affected_card)
		  ){
			card_instance_t *dead = get_card_instance(affected_card_controller, affected_card);
			if( dead->kill_code > 0 && dead->kill_code != KILL_REMOVE ){
				if( instance->targets[0].card < 1 ){
					instance->targets[0].card = 1;
				}
				int souls = instance->targets[0].card;
				if( souls < 10 ){
					instance->targets[souls].card = get_original_id(affected_card_controller, affected_card);
					instance->targets[0].card++;
				}
				exiledby_remember(player, card, player, get_original_internal_card_id(affected_card_controller, affected_card), NULL, NULL);
				dead->kill_code = KILL_REMOVE;
			}
		}
	}

	if( current_turn == player && trigger_condition == TRIGGER_UPKEEP ){
		if( has_mana(player, COLOR_COLORLESS, 4) && can_pay_life(player, 2) &&
			exiledby_choose(player, card, CARD_ID_PURGATORY, EXBY_FIRST_FOUND, 0, "creature", 1)
		  ){
			upkeep_trigger_ability_mode(player, card, event, player, RESOLVE_TRIGGER_AI(player));
		}
	}

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		if( has_mana(player, COLOR_COLORLESS, 4) && can_pay_life(player, 2) &&
			exiledby_choose(player, card, CARD_ID_PURGATORY, EXBY_FIRST_FOUND, 0, "creature", 1)
		  ){
			int rval = exiledby_choose(player, card, CARD_ID_PURGATORY, EXBY_CHOOSE, 0, "Creature", 1);
			charge_mana(player, COLOR_COLORLESS, 4);
			if( spell_fizzled != -1 ){
				int* loc = (int*)rval;
				if( loc ){
					int *loc2 = exiledby_find(player, card, *loc, NULL, NULL);
					int iid = *loc2 & ~0x80000000;
					*loc2 = -1;
					int card_added = add_card_to_hand(player, iid);
					put_into_play(player, card_added);
					lose_life(player, 2);
				}
			}
		}
	}

   return global_enchantment(player, card, event);
}

int card_purraj_of_urborg(int player, int card, event_t event){

  if( trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller){
	  int trig = 0;
	  if( (get_color(trigger_cause_controller, trigger_cause) & COLOR_TEST_BLACK) && has_mana(player, COLOR_BLACK, 1) ){
		  trig = 1;
	  }
	  if( trig > 0 ){
		  if( event == EVENT_TRIGGER){
			  event_result |= RESOLVE_TRIGGER_MANDATORY;
		  }
		  else if( event == EVENT_RESOLVE_TRIGGER){
				   charge_mana(player, COLOR_BLACK, 1);
				   if( spell_fizzled != 1){
					   add_1_1_counter(player, card);
				   }
		  }
	  }
  }

  else if( event == EVENT_ABILITIES && affect_me(player, card) && is_attacking(player, card) ){
		   event_result |= KEYWORD_FIRST_STRIKE;
  }

	return 0;
}

int card_quirion_elves(int player, int card, event_t event){

	/* Quirion Elves	|1|G
	 * Creature - Elf Druid 1/1
	 * As ~ enters the battlefield, choose a color.
	 * |T: Add |G to your mana pool.
	 * |T: Add one mana of the chosen color to your mana pool. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_SPELL ){
		int clr = get_deck_color(player, player);	// best color in deck is non-green? use it.
		if (clr == COLOR_GREEN){
			clr = get_deck_color(-1, player);		// find two best colors and pick the non-green one
			clr &= ~COLOR_TEST_GREEN;
			clr = single_color_test_bit_to_color(clr);
		}
		instance->info_slot = COLOR_TEST_GREEN | (1 << choose_a_color(player, clr));
	}

	return mana_producing_creature_all_one_color(player, card, event, 24, instance->info_slot, 1);
}

int card_rampant_growth(int player, int card, event_t event){
	/*
	  Rampant Growth |1|G
	  Sorcery, 1G (2)
	  Search your library for a basic land card and put that card onto the battlefield tapped. Then shuffle your library.
	*/
	if( event == EVENT_RESOLVE_SPELL ){
		tutor_basic_land(player, 1, 1);
		kill_card(player, card, KILL_DESTROY);
	}
	return generic_spell(player, card, event, 0, NULL, NULL, 0, NULL);
}

int card_rashida_scalebane(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_subtype = SUBTYPE_DRAGON;
	td.required_state = TARGET_STATE_ATTACKING;

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			gain_life(player, get_power(instance->targets[0].player, instance->targets[0].card));
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_BURY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_razor_pendulum(int player, int card, event_t event){

	if( eot_trigger(player, card, event) ){
		if( life[current_turn] <= 5 ){
			damage_player(current_turn, 2, player, card);
		}
	}

	return 0;
}

int card_reflect_damage(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ANY);
	td.extra = damage_card;
	td.required_type = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST && can_target(&td) ){
		return 0x63;
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 pick_target(&td, "TARGET_DAMAGE");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			card_instance_t *target = get_card_instance(instance->targets[0].player, instance->targets[0].card);
			target->damage_target_player = target->damage_source_player;
			target->damage_target_card = -1;
			kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_reign_of_chaos(int player, int card, event_t event)
{
  /* Reign of Chaos	|2|R|R
   * Sorcery
   * Choose one - Destroy target |H2Plains and target |Swhite creature; or destroy target |H2Island and target |Sblue creature. */

  if (!IS_CASTING(player, card, event))
	return 0;

  target_definition_t td_plains;
  default_target_definition(player, card, &td_plains, TYPE_LAND);
  td_plains.required_subtype = SUBTYPE_PLAINS;

  target_definition_t td_white_creature;
  default_target_definition(player, card, &td_white_creature, TYPE_CREATURE);
  td_white_creature.required_color = get_sleighted_color_test(player, card, COLOR_TEST_WHITE);

  target_definition_t td_island;
  default_target_definition(player, card, &td_island, TYPE_LAND);
  td_island.required_subtype = SUBTYPE_ISLAND;

  target_definition_t td_blue_creature;
  default_target_definition(player, card, &td_blue_creature, TYPE_CREATURE);
  td_blue_creature.required_color = get_sleighted_color_test(player, card, COLOR_TEST_BLUE);

  char prompt_w[200], prompt_u[200];
  if (ai_is_speculating == 1 || event != EVENT_CAST_SPELL)
	prompt_w[0] = prompt_u[0] = 0;
  else
	{
	  // Order is important here; get_sleighted_color_text can deal with the escaped %%, but get_hacked_land text can't.
	  strcpy(prompt_w, get_hacked_land_text(player, card, get_sleighted_color_text(player, card, "%%s and %s creature", COLOR_WHITE), SUBTYPE_PLAINS));
	  strcpy(prompt_u, get_hacked_land_text(player, card, get_sleighted_color_text(player, card, "%%s and %s creature", COLOR_BLUE), SUBTYPE_ISLAND));
	}

  enum
  {
	CHOICE_W = 1,
	CHOICE_U
  } choice = DIALOG(player, card, event,
					DLG_RANDOM, DLG_AUTOCHOOSE_IF_1,
					prompt_w, can_target(&td_plains) && can_target(&td_white_creature), 1,
					prompt_u, can_target(&td_island) && can_target(&td_blue_creature), 1);

  if (event == EVENT_CAN_CAST)
	return choice;
  else if (choice)
	{
	  target_definition_t *td_land, *td_creature;
	  if (choice == CHOICE_W)
		{
		  td_land = &td_plains;
		  td_creature = &td_white_creature;
		}
	  else
		{
		  td_land = &td_island;
		  td_creature = &td_blue_creature;
		}

	  card_instance_t* instance = get_card_instance(player, card);

	  if (event == EVENT_CAST_SPELL && affect_me(player, card))
		{
		  instance->number_of_targets = 0;
		  if (pick_next_target_noload(td_land, get_hacked_land_text(player, card, "Select target %s.",
																	choice == CHOICE_W ? SUBTYPE_PLAINS : SUBTYPE_ISLAND)))
			pick_next_target_noload(td_creature, get_sleighted_color_text(player, card, "Select target %s creature.",
																		  choice == CHOICE_W ? COLOR_WHITE : COLOR_BLUE));
		}

	  if (event == EVENT_RESOLVE_SPELL)
		{
		  if (validate_target(player, card, td_land, 0))
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);

		  if (validate_target(player, card, td_creature, 1))
			kill_card(instance->targets[1].player, instance->targets[1].card, KILL_DESTROY);

		  kill_card(player, card, KILL_DESTROY);
		}
	}

  return 0;
}

int card_reparations(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if(trigger_condition == TRIGGER_SPELL_CAST && affect_me(player, card) && player == reason_for_trigger_controller &&
	   1-player == trigger_cause_controller ){

		  int trig = 0;
		  int i;
		  card_instance_t *target = get_card_instance( trigger_cause_controller, trigger_cause);
		  for(i=0; i<5; i++ ){
			  if( target->targets[i].player == player ){
				 if( target->targets[i].card == -1 ){
					 trig = 1;
					 break;
				 }
				 else if( is_what(target->targets[i].player, target->targets[i].card, TYPE_CREATURE) ){
						  trig = 1;
						  break;

				 }
			  }
		  }

		  if( trig > 0 ){
			  if(event == EVENT_TRIGGER){
				 event_result |= 1+player;
			  }
			  else if(event == EVENT_RESOLVE_TRIGGER){
					  draw_cards(player, 1);
			  }
		  }
  }
  return global_enchantment(player, card, event);
}

int card_rocky_tar_pit(int player, int card, event_t event){
	/* Rocky Tar Pit	""
	 * Land
	 * ~ enters the battlefield tapped.
	 * |T, Sacrifice ~: Search your library for |Ha Swamp or |H2Mountain card and put it onto the battlefield. Then shuffle your library. */
	return old_fetch(player, card, event, SUBTYPE_SWAMP, SUBTYPE_MOUNTAIN);
}

int card_reality_ripple(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE | TYPE_ARTIFACT | TYPE_LAND);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return can_target(&td);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 pick_target(&td, "TARGET_PERMANENT");
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 if( valid_target(&td) ){
				 phase_out(instance->targets[0].player, instance->targets[0].card);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sacred_mesa(int player, int card, event_t event){
	/* Sacred Mesa	|2|W
	 * Enchantment
	 * At the beginning of your upkeep, sacrifice ~ unless you sacrifice a Pegasus.
	 * |1|W: Put a 1/1 |Swhite Pegasus creature token with flying onto the battlefield. */

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  && has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1) ){
		return 1;
	}

	if( event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 1);
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		generate_token_by_id(player, card, CARD_ID_PEGASUS);
	}

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int kill = 1;
		if( can_sacrifice(player, player, 1, TYPE_PERMANENT, SUBTYPE_PEGASUS) ){
			impose_sacrifice(player, card, player, 1, TYPE_PERMANENT, 0, SUBTYPE_PEGASUS, 0, 0, 0, 0, 0, -1, 0);
			kill = 0;
		}
		if( kill == 1 ){
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	return global_enchantment(player, card, event);
}

int card_sandbar_crocodile(int player, int card, event_t event){
	return phasing(player, card, event);
}

int card_savage_twister(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 instance->info_slot = x_value;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 damage_all(player, card, player, instance->info_slot, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			 damage_all(player, card, 1-player, instance->info_slot, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			 kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_sealed_fate(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE );
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST ){
		instance->targets[0].player = 1-player;
		if( would_valid_target(&td) ){
			return ! check_battlefield_for_id(2, CARD_ID_GADDOCK_TEEG);
		}
	}
	if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
		instance->info_slot = x_value;
		instance->targets[0].player = 1-player;
	}
	if( event == EVENT_RESOLVE_SPELL ){
			if( valid_target(&td) ){
				int t_player = instance->targets[0].player;
				int amount = instance->info_slot;
				if( count_deck(t_player) < amount ){
					amount = count_deck(t_player);
				}
				while( 1 ){
						int selected = show_deck( player, deck_ptr[t_player], amount, "Select a card to remove", 0, 0x7375B0 );
						if( selected != -1 ){
							remove_card_from_deck(t_player, selected);
							amount--;
							break;
						}
				}
				if( amount > 0  ){
					rearrange_top_x(t_player, player, amount);
				}
			}
			kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_seeds_of_innocence(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 int i;
			 for(i=0; i<2; i++){
				 int count = active_cards_count[i]-1;
				 while( count > -1 ){
					   if( in_play(i, count) && is_what(i, count, TYPE_ARTIFACT) ){
						   gain_life(i, get_cmc(i, count));
						   kill_card(i, count, KILL_BURY);
					   }
					   count--;
				 }
			 }
			 kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_serene_heart(int player, int card, event_t event)
{
  /* Serene Heart	|1|G
   * Instant
   * Destroy all Auras. */

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_ENCHANTMENT, "");
	  test.subtype = SUBTYPE_AURA;
	  test.subtype_flag = MATCH;
	  new_manipulate_all(player, card, ANYBODY, &test, KILL_DESTROY);
	}

  return 0;
}

int card_sewer_rats(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  &&
		has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) && instance->info_slot < 3 && can_pay_life(player, 1)
	  ){
		int result = 1;
		if( player == AI && life[player] < 6 ){
			result = 0;
		}
		return result;
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				lose_life(player, 1);
				instance->info_slot++;
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			pump_until_eot(player, card, player, instance->parent_card, 1, 0);
	}

	else if( event == EVENT_CLEANUP ){
			instance->info_slot = 0;
	}

	return 0;
}

int card_shadow_guildmage(int player, int card, event_t event){
	return mirage_guildmage(player, card, event, COLOR_BLUE, COLOR_RED);
}

int card_shallow_grave(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST && count_graveyard_by_type(player, TYPE_CREATURE) > 0 ){
		return 1;
	}
	else if( event == EVENT_RESOLVE_SPELL ){
			 int count = count_graveyard(player)-1;
			 const int *grave = get_grave(player);
			 while( count > -1 && grave[count] != -1 ){
				   if( cards_data[grave[count]].type & TYPE_CREATURE ){
						reanimate_permanent(player, card, player, count, REANIMATE_HASTE_AND_EXILE_AT_EOT);
						break;
				   }
				   count--;
			 }
			 kill_card(player, card, KILL_DESTROY);
	}
	return 0;
}

int card_shaper_guildmage(int player, int card, event_t event){
	return mirage_guildmage(player, card, event, COLOR_BLACK, COLOR_WHITE);
	return 0;
}

int card_shauku_endbringer(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	check_legend_rule(player, card, event);

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		lose_life(player, 3);
	}

	if( event == EVENT_ATTACK_LEGALITY && affect_me(player, card) && count_subtype(2, TYPE_CREATURE, -1) >= 2 ){
		event_result = 1;
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_REMOVE);
			add_1_1_counter(player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 0, 0, 0, 0, 0, 0, 0, &td, "TARGET_CREATURE");
}

int card_shimmer(int player, int card, event_t event){

	if (event == EVENT_CAN_CAST){
		return 1;
	}

	if (comes_into_play(player, card, event)){
		// These are all the current land subtypes from rule 205.3i; this card can choose non-basic land types
		int choice = do_dialog(player, player, card, -1, -1,
							   " Forest\n Island\n Mountain\n Plains\n Swamp\n Desert\n Gate\n Lair\n Locus\n Mine\n Power-Plant\n Tower\n Urza's", 0);

		card_instance_t* instance = get_card_instance( player, card );

		instance->info_slot = 1 << choice;
	}

	// Main effect handled directly in untap_phasing.

	return global_enchantment(player, card, event);
}

int card_sidar_jabari(int player, int card, event_t event){

	check_legend_rule(player, card, event);

	flanking(player, card, event);

	return card_master_of_diversion(player, card, event);
}

int card_skulking_ghost(int player, int card, event_t event){
  if (in_play(player, card)
	  && !is_humiliated(player, card)
	  && becomes_target_of_spell_or_effect(player, card, event, player, card, ANYBODY))
	kill_card(player, card, KILL_SACRIFICE);

  return 0;
}

int card_sky_diamond(int player, int card, event_t event){

	return mirage_diamonds(player, card, event);
}

int card_soulshriek(int player, int card, event_t event){

	if( ! IS_GS_EVENT(player, card, event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.allowed_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if(event == EVENT_RESOLVE_SPELL ){
		if( valid_target(&td) ){
			int amount = count_graveyard_by_type(player, TYPE_CREATURE);
			pump_ability_t pump;
			default_pump_ability_definition(player, card, &pump, amount, 0, 0, 0);
			pump.paue_flags = PAUE_END_AT_EOT | PAUE_REMOVE_TARGET_AT_EOT;
			pump.eot_removal_method = KILL_SACRIFICE;
			pump_ability(player, card, instance->targets[0].player, instance->targets[0].card, &pump);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return generic_spell(player, card, event, GS_CAN_TARGET | GS_LITERAL_PROMPT, &td, "Select target creature you control.", 1, NULL);
}

int spatial_binding_legacy(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( current_turn == player && upkeep_trigger(player, card, event) && count_upkeeps(player) > 0){
		remove_special_flags(instance->targets[0].player, instance->targets[0].card, SF_CANNOT_PHASE_OUT);
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int card_spatial_binding(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return global_enchantment(player, card, event);
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_PERMANENT);
	td.preferred_controller = player;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			set_special_flags(instance->targets[0].player, instance->targets[0].card, SF_CANNOT_PHASE_OUT);
			create_targetted_legacy_effect(instance->parent_controller, instance->parent_card, &spatial_binding_legacy, instance->targets[0].player, instance->targets[0].card);
		}
	}
	return generic_activated_ability(player, card, event, GAA_CAN_TARGET, MANACOST0, 1, &td, "TARGET_PERMANENT");
}

int card_spectral_guardian(int player, int card, event_t event){

	if( event == EVENT_ABILITIES  ){
		if( is_what(affected_card_controller, affected_card, TYPE_ARTIFACT) &&
			! is_what(affected_card_controller, affected_card, TYPE_CREATURE) ){
			event_result |= KEYWORD_SHROUD;
		}
	}

	return 0;
}

int card_spirit_of_the_night(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	pro_black(player, card, event);
	haste(player, card, event);
	if( event == EVENT_ABILITIES ){
		if( ! affect_me(player, card) ){ return 0; }
		card_instance_t *affected = get_card_instance(player, card);
		if( affected->state & STATE_ATTACKING  ){
			event_result |= KEYWORD_FIRST_STRIKE;
		}
	}
	return 0;
}

int card_stupor(int player, int card, event_t event)
{
  /* Stupor	|2|B
   * Sorcery
   * Target opponent discards a card at random, then discards a card. */

  if (!IS_GS_EVENT(player, card, event))
	return 0;

  target_definition_t td;
  default_target_definition(player, card, &td, 0);
  td.zone = TARGET_ZONE_PLAYERS;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  if (valid_target(&td))
		{
		  int t_player = get_card_instance(player, card)->targets[0].player;
		  discard(t_player, DISC_RANDOM, player);
		  discard(t_player, 0, player);
		}
	  kill_card(player, card, KILL_DESTROY);
	}

  return generic_spell(player, card, event, GS_CAN_TARGET, &td, "TARGET_PLAYER", 1, NULL);
}

int card_subterranean_spirit(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		test_definition_t this_test;
		default_test_definition(&this_test, TYPE_CREATURE);
		this_test.keyword = KEYWORD_FLYING;
		this_test.keyword_flag = 1;
		new_damage_all(player, instance->parent_card, 2, 1, 0, &this_test);
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK, 0, 0, 0, 0, 0, 0, 0, 0, 0);
}

int card_sunweb(int player, int card, event_t event){

	if(event == EVENT_BLOCK_LEGALITY && affect_me(player, card) && ! is_humiliated(player, card) ){
		if( get_power(attacking_card_controller, attacking_card) < 3 ){
			event_result = 1;
		}
	}

	return 0;
}

int card_superior_numbers(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_CAST){
		if( IS_AI(player) && player == AI ){
		   if( count_permanents_by_type(player, TYPE_CREATURE) > count_permanents_by_type(1-player, TYPE_CREATURE) ){
			   return 1;
		   }
		}
		else{
			 return 1;
	   }
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 if( player != AI ){
				 pick_target(&td, "TARGET_CREATURE");
			 }
			 else{
				  int count =  0;
				  int amount = count_permanents_by_type(player, TYPE_CREATURE) -
							   count_permanents_by_type(1- player, TYPE_CREATURE);
				  int prots = KEYWORD_SHROUD | get_protections_from(player, card);
				  while( count < active_cards_count[1-player] ){
						if( in_play(1-player, count) && is_what(1-player, count, TYPE_CREATURE) &&
							get_toughness(1-player, count) <= amount &&
							!(get_abilities(1-player, count, EVENT_ABILITIES, -1) & prots)
						  ){
							instance->targets[0].player = 1-player;
							instance->targets[0].card = count;
							break;
						}
						count++;
				  }
				  if( instance->targets[0].card == -1 ){
					  spell_fizzled = 1;
				  }
			 }
	}

	else if( event == EVENT_RESOLVE_SPELL ){
			 int amount = count_permanents_by_type(player, TYPE_CREATURE) -
						  count_permanents_by_type(1- player, TYPE_CREATURE);
			 if( valid_target(&td) && amount > 0 ){
				 damage_creature(instance->targets[0].player, instance->targets[0].card, amount, player, card);
			 }
			 kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_tainted_specter(int player, int card, event_t event){
	/*
	  Tainted Specter English |3|B
	  Creature - Specter 2/2
	  Flying
	  {1}{B}{B}, {T}: Target player discards a card unless he or she puts a card from his or her hand on top of his or her library.
	  If that player discards a card this way, Tainted Specter deals 1 damage to each creature and each player.
	  Activate this ability only any time you could cast a sorcery.
	*/

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance( player, card );

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			if( hand_count[instance->targets[0].player] > 0 ){
				test_definition_t this_test;
				new_default_test_definition(&this_test, TYPE_ANY, "Select a card.");

				test_definition_t this_test2;
				default_test_definition(&this_test2, TYPE_CREATURE);
				this_test2.keyword = KEYWORD_PROT_BLACK;
				this_test2.keyword_flag = DOESNT_MATCH;
				this_test2.toughness = 1;

				int ai_choice = 0;
				if( check_battlefield_for_special_card(player, card, instance->targets[0].player, CBFSC_GET_COUNT, &this_test2) > 1 ){
					ai_choice = 1;
				}
				int selected = new_select_a_card(instance->targets[0].player, instance->targets[0].player, TUTOR_FROM_HAND, 1, AI_MIN_VALUE, -1, &this_test);
				int choice = do_dialog(instance->targets[0].player, instance->targets[0].player, selected, -1, -1,
										" Discard this card\n Put it on top of deck", ai_choice);
				if( choice == 0 ){
					new_discard_card(instance->targets[0].player, selected, player, 0);
					APNAP(p, {new_damage_all(instance->parent_controller, instance->parent_card, p, 1, NDA_ALL_CREATURES | NDA_PLAYER_TOO, NULL);};);
				}
				else{
					put_on_top_of_deck(instance->targets[0].player, selected);
				}
			}
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED | GAA_CAN_SORCERY_BE_PLAYED | GAA_CAN_TARGET, MANACOST_XB(1, 2), 0, &td, "TARGET_PLAYER");
}

int card_taniwha(int player, int card, event_t event){

	int ph = phasing(player, card, event);
	if (ph){
		return ph;
	}

	check_legend_rule(player, card, event);

	if( current_turn == player && upkeep_trigger(player, card, event) ){
		int count = active_cards_count[player]-1;
		while( count > -1 ){
			   if( in_play(player, count) && is_what(player, count, TYPE_LAND) ){
				   phase_out(player, count);
			   }
			   count--;
		}
	}

	return 0;
}

int card_teekas_dragon(int player, int card, event_t event){

	rampage(player, card, event, 4);

	return 0;
}

// teferi's drake --> sandbar crocodile

// teferi's imp --> sandbar crocodile

int card_teferis_isle(int player, int card, event_t event)
{
  /* Teferi's Isle	""
   * Legendary Land
   * Phasing
   * ~ enters the battlefield tapped.
   * |T: Add |U|U to your mana pool. */

  int ph = phasing(player, card, event);
  if (ph)
	return ph;

  check_legend_rule(player, card, event);

  comes_into_play_tapped(player, card, event);

  if (event == EVENT_ACTIVATE)
	produce_mana_tapped(player, card, COLOR_BLUE, 2);
  else if (event == EVENT_COUNT_MANA && affect_me(player, card) && CAN_TAP_FOR_MANA(player, card))
	declare_mana_available(player, COLOR_BLUE, 2);
  else
	return mana_producer(player, card, event);

  return 0;
}

int card_telim_tor(int player, int card, event_t event)
{
  check_legend_rule(player, card, event);

  flanking(player, card, event);

  // Whenever ~ attacks, all attacking creatures with flanking get +1/+1 until end of turn.
  if (declare_attackers_trigger(player, card, event, 0, player, card))
	{
	  test_definition_t test;
	  new_default_test_definition(&test, TYPE_CREATURE, "");
	  test.keyword = SP_KEYWORD_FLANKING;
	  test.keyword_flag = F0_HAS_SP_KEYWORD;
	  test.state = STATE_ATTACKING;
	  pump_creatures_until_eot(player, card, current_turn, 2, 1,1, 0,0, &test);
	}

  return 0;
}

int card_telim_tors_darts(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.zone = TARGET_ZONE_PLAYERS;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_player(instance->targets[0].player, 1, player, instance->parent_card);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 2, 0, 0, 0, 0, 0, 0, &td, "TARGET_PLAYER");
}

int thirst_impl(int player, int card, event_t event, int x, int b, int u, int g, int r, int w)
{
  card_instance_t* instance = get_card_instance(player, card);
  int p = instance->damage_target_player, c = instance->damage_target_card;

  if (p >= 0 && c >= 0)
	{
	  // When ~ enters the battlefield, tap enchanted creature.
	  if (comes_into_play(player, card, event))
		tap_card(p, c);

	  // Enchanted creature doesn't untap during its controller's untap step.
	  does_not_untap(p, c, event);
	}

  // At the beginning of your upkeep, sacrifice ~ unless you pay ...
  basic_upkeep(player, card, event, x, b, u, g, r, w);

  // Enchant creature
  return disabling_aura(player, card, event);
}

int card_thirst(int player, int card, event_t event)
{
  return thirst_impl(player, card, event, MANACOST_U(1));
}

int card_tombstone_stairwell(int player, int card, event_t event)
{
  /* Tombstone Stairwell	|2|B|B
   * World Enchantment
   * Cumulative upkeep |1|B
   * At the beginning of each upkeep, if ~ is on the battlefield, each player puts a 2/2 |Sblack Zombie creature token with haste named Tombspawn onto the
   * battlefield for each creature card in his or her graveyard.
   * At the beginning of each end step or when ~ leaves the battlefield, destroy all tokens put onto the battlefield with ~. They can't be regenerated. */

  enchant_world(player, card, event);

  if (eot_trigger(player, card, event) || leaves_play(player, card, event))
	{
	  card_instance_t* inst;
	  int p, c;
	  for (p = 0; p <= 1; ++p)
		for (c = 0; c < active_cards_count[p]; ++c)
		  if ((inst = in_play(p, c)) && cards_data[inst->internal_card_id].id == CARD_ID_ZOMBIE
			  && get_special_infos(p, c) == 67 && inst->damage_source_player == player && inst->damage_source_card == card)
			kill_card(p, c, KILL_BURY);
	}

  upkeep_trigger_ability(player, card, event, ANYBODY);
  if (event == EVENT_UPKEEP_TRIGGER_ABILITY)
	{
	  if (current_turn == player)
		cumulative_upkeep(player, card, event, MANACOST_XB(1,1));

	  if (!in_play(player, card))
		return 0;

	  int p;
	  for (p = 0; p <= 1; ++p)
		{
		  int dead = count_graveyard_by_type(p, TYPE_CREATURE);
		  token_generation_t token;
		  default_token_definition(player, card, CARD_ID_ZOMBIE, &token);
		  token.t_player = p;
		  token.qty = dead;
		  token.action = TOKEN_ACTION_DONT_COPY_TOKEN_SOURCE;
		  token.s_key_plus = SP_KEYWORD_HASTE;
		  token.special_infos = 67;
		  generate_token(&token);
		}
	}

  return global_enchantment(player, card, event);
}

int card_tranquil_domain(int player, int card, event_t event)
{
  /* Tranquil Domain	|1|G
   * Instant
   * Destroy all non-Aura enchantments. */

	if (event == EVENT_RESOLVE_SPELL){
		test_definition_t test;
		new_default_test_definition(&test, TYPE_ENCHANTMENT, "");
		test.subtype = SUBTYPE_AURA;
		test.subtype_flag = DOESNT_MATCH;
		new_manipulate_all(player, card, ANYBODY, &test, KILL_DESTROY);
		kill_card(player, card, KILL_DESTROY);
	}

	return basic_spell(player, card, event);
}

int card_uktabi_faeries(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_ARTIFACT);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST_XG(3, 1), 0, &td, "TARGET_ARTIFACT");
}

int card_uktabi_wildcats(int player, int card, event_t event){
	/* Uktabi Wildcats	|4|G
	 * Creature - Cat 100/100
	 * ~'s power and toughness are each equal to the number of |H1Forests you control.
	 * |G, Sacrifice |Ha Forest: Regenerate ~. */

	if ((event == EVENT_POWER || event == EVENT_TOUGHNESS) && affect_me(player, card) && player != -1)
		event_result += count_subtype(player, TYPE_LAND, get_hacked_subtype(player, card, SUBTYPE_FOREST));

	if (card == -1){
		return 0;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( land_can_be_played & LCBP_REGENERATION ){
		if( event == EVENT_CAN_ACTIVATE && has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0) &&
			can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0)
		  ){
			return can_regenerate(player, card);
		}
		else if( event == EVENT_ACTIVATE ){
				charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
				if( spell_fizzled != 1 && !sacrifice(player, card, player, 0, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0) ){
					spell_fizzled = 1;
				}
		}
		else if( event == EVENT_RESOLVE_ACTIVATION ){
				 regenerate_target(instance->parent_controller, instance->parent_card);
		}
	}

	return 0;
}

int card_unerring_sling(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.required_abilities = KEYWORD_FLYING;
	td.required_state = TARGET_STATE_IN_COMBAT;

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.illegal_state = TARGET_STATE_TAPPED;
	td1.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && ! is_tapped(player, card) && ! is_animated_and_sick(player, card) &&
		has_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0) && can_target(&td1)
	  ){
		return can_target(&td);
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 3, 0, 0, 0, 0, 0);
		if( spell_fizzled != 1 && new_pick_target(&td1, "TARGET_CREATURE", 1, 1) ){
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 1);
			if( pick_target(&td, "TARGET_CREATURE")  ){
				tap_card(instance->targets[1].player, instance->targets[1].card);
				instance->targets[2].player = get_power(instance->targets[1].player, instance->targets[1].card);
				instance->number_of_targets = 1;
			}
			state_untargettable(instance->targets[1].player, instance->targets[1].card, 0);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			damage_creature(instance->targets[0].player, instance->targets[0].card, instance->targets[2].player, instance->targets[1].player, instance->targets[1].card);
		}
	}

	return 0;
}

int card_unfulfilled_desires(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST ){
		return 1;

	}

	else if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card)  &&
			 has_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) && can_pay_life(player, 1)
		 ){
			int result = 1;
			if( player == AI && life[player] < 6 ){
				result = 0;
			}
			return result;
	}
	else if( event == EVENT_ACTIVATE ){
			charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0);
			if( spell_fizzled != 1 ){
				lose_life(player, 1);
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			draw_cards(player, 1);
			discard(player, 0, player);
	}

	return global_enchantment(player, card, event);
}

int card_vaporous_djinn(int player, int card, event_t event){

	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int go_out = 1;
		if( has_mana(player, COLOR_BLUE, 2) ){
			int choice = do_dialog(player, player, card, -1, -1, " Pay upkeep\n Pass", 0);
			if( choice == 0 ){
				charge_mana(player, COLOR_BLUE, 2);
				if( spell_fizzled != 1 ){
					go_out = 0;
				}
			}
		}
		if( go_out == 1 ){
			phase_out(player, card);
		}
	}

	return 0;
}

int card_ventifact_bottle(int player, int card, event_t event){

	/* Ventifact Bottle	|3
	 * Artifact
	 * |X|1, |T: Put X charge counters on ~. Activate this ability only any time you could cast a sorcery.
	 * At the beginning of your precombat main phase, if ~ has a charge counter on it, tap it and remove all charge counters from it. Add |1 to your mana pool
	 * for each charge counter removed this way. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && CAN_ACTIVATE(player, card, MANACOST_X(IS_AI(player) ? 2 : 1)) && CAN_TAP(player, card)
		&& can_sorcery_be_played(player, event)
	  ){
		return 1;
	}
	else if( event == EVENT_ACTIVATE ){
			if( charge_mana_for_activated_ability(player, card, 1, 0, 0, 0, 0, 0) ){
				charge_mana(player, COLOR_COLORLESS, -1);
				if( spell_fizzled != 1 ){
					tap_card(player, card);
					instance->info_slot = x_value;
				}
			}
	}
	else if( event == EVENT_RESOLVE_ACTIVATION ){
			add_counters(player, card, COUNTER_CHARGE, instance->info_slot);
	}

	if( current_turn == player && current_phase == PHASE_MAIN1 && instance->targets[1].card != 66 ){
		instance->targets[1].card = 66;
		if ( count_counters(player, card, COUNTER_CHARGE) ){
			produce_mana(player, COLOR_COLORLESS, count_counters(player, card, COUNTER_CHARGE));
			remove_all_counters(player, card, COUNTER_CHARGE);
			tap_card(player, card);
		}
	}

	if( current_phase != PHASE_MAIN1 ){
		instance->targets[1].card = -1;
	}

	return 0;
}

static const char* targets_an_enchantment_in_play(int who_chooses, int player, int card)
{
	card_instance_t *instance = get_card_instance(player, card);
	int i;
	for(i=0; i<instance->number_of_targets; i++){
		if( instance->targets[i].card != -1 && in_play(instance->targets[i].player, instance->targets[i].card) &&
			is_what(instance->targets[i].player, instance->targets[i].card, TYPE_ENCHANTMENT)
		  ){
			return NULL;
		}
	}
	return "must target an enchantment in play.";
}

int card_vigilant_martyr(int player, int card, event_t event){

	if( ! IS_GAA_EVENT(event) ){
		return 0;
	}

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	target_definition_t td2;
	counterspell_target_definition(player, card, &td2, TYPE_ANY);
	td2.special = TARGET_SPECIAL_EXTRA_FUNCTION;
	td2.extra = (int32_t)targets_an_enchantment_in_play;

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		if( generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_SPELL_ON_STACK | GAA_UNTAPPED, MANACOST_W(2), 0, &td2, NULL) ){
			return 99;
		}
		if( generic_activated_ability(player, card, event, GAA_SACRIFICE_ME | GAA_CAN_TARGET, MANACOST0, 0, &td, NULL) ){
			return 99;
		}
	}

	if(event == EVENT_ACTIVATE ){
		instance->number_of_targets = 0;
		if( generic_activated_ability(player, card, EVENT_CAN_ACTIVATE, GAA_SACRIFICE_ME | GAA_SPELL_ON_STACK | GAA_UNTAPPED, MANACOST_W(2), 0, &td2, NULL) ){
			if( charge_mana_for_activated_ability(player, card, MANACOST_W(2)) ){
				instance->targets[0].player = card_on_stack_controller;
				instance->targets[0].card = card_on_stack;
				instance->number_of_targets = 1;
				tap_card(player, card);
				instance->info_slot = 67;
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
		else{
			if( new_pick_target(&td, "Select target creature to regenerate.", 0, 1 | GS_LITERAL_PROMPT) ){
				instance->info_slot = 66;
				kill_card(player, card, KILL_SACRIFICE);
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( instance->info_slot == 66 && valid_target(&td) && can_regenerate(instance->targets[0].player, instance->targets[0].card) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
		if( instance->info_slot == 67 && counterspell_validate(player, card, &td2, 0) ){
			real_counter_a_spell(player, card, instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_village_elder(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);
	td.preferred_controller = player;
	td.required_color = COLOR_TEST_GREEN;
	td.required_state = TARGET_STATE_DESTROYED;
	if( player == AI ){
		td.special = TARGET_SPECIAL_REGENERATION;
	}

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && can_target(&td) &&
		can_sacrifice_as_cost(player, 1, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0) && has_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0)
	  ){
		return 0x63;
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 0, 0, 1, 0, 0);
		if( spell_fizzled != 1 ){
			int selected = pick_special_permanent_for_sacrifice(player, card, 0, TYPE_LAND, 0, SUBTYPE_FOREST, 0, 0, 0, 0, 0, -1, 0);
			if( selected != -1 ){
				instance->targets[0].player = player;
				instance->targets[0].card = selected;
				if( new_pick_target(&td, "TARGET_CREATURE", 1, 1) ){
					if( can_regenerate(instance->targets[1].player, instance->targets[1].card) > 0 ){
						kill_card(instance->targets[0].player, instance->targets[0].card, KILL_SACRIFICE);
						instance->number_of_targets = 1;
						tap_card(player, card);
						instance->targets[0] = instance->targets[1];
					}
				}
			}
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			regenerate_target(instance->targets[0].player, instance->targets[0].card);
		}
	}

	return 0;
}

int card_vitalizing_cascade(int player, int card, event_t event)
{
  // 0x120282f

  /* Vitalizing Cascade	|X|G|W
   * Instant
   * You gain X plus 3 life. */

  if (!IS_CASTING(player, card, event))
	return 0;

  if (event == EVENT_CAN_CAST)
	return 1;

  if (event == EVENT_CAST_SPELL && !is_token(player, card))
	get_card_instance(player, card)->info_slot = x_value;

  if (event == EVENT_RESOLVE_SPELL)
	{
	  gain_life(player, get_card_instance(player, card)->info_slot + 3);
	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

// cat token --> rhino token

int card_waiting_in_the_weeds(int player, int card, event_t event){

	/* Waiting in the Weeds	|1|G|G
	 * Sorcery
	 * Each player puts a 1/1 |Sgreen Cat creature token onto the battlefield for each untapped |H2Forest he or she controls. */

	if( event == EVENT_CAN_CAST ){
		return 1;
	}

	if( event == EVENT_RESOLVE_SPELL ){
		test_definition_t this_test;
		new_default_test_definition(&this_test, TYPE_LAND, "");
		this_test.subtype = get_hacked_subtype(player, card, SUBTYPE_FOREST);
		this_test.state = STATE_TAPPED;
		this_test.state_flag = DOESNT_MATCH;

		int i;
		for(i=0; i<2; i++){
			token_generation_t token;
			default_token_definition(player, card, CARD_ID_CAT, &token);
			token.qty = check_battlefield_for_special_card(player, card, i, CBFSC_GET_COUNT, &this_test);
			token.t_player = i;
			token.pow = 1;
			token.tou = 1;
			token.color_forced = COLOR_TEST_GREEN;
			generate_token(&token);
		}
		kill_card(player, card, KILL_DESTROY);
	}

	return 0;
}

int card_wall_of_corpses(int player, int card, event_t event){

	target_definition_t td;
	default_target_definition(player, card, &td, TYPE_CREATURE);

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE && can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0) &&
		current_phase == PHASE_AFTER_BLOCKING
	  ){
		if( instance->blocking < 255 ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = instance->blocking;
			instance->number_of_targets = 1;
			if( would_valid_target(&td) ){
				return can_sacrifice_as_cost(player, 1, TYPE_CREATURE, 0, 0, 0, 0, 0, 0, 0, -1, 0);
			}
		}
	}

	if(event == EVENT_ACTIVATE ){
		charge_mana_for_activated_ability(player, card, 0, 1, 0, 0, 0, 0);
		if( spell_fizzled != 1 ){
			instance->targets[0].player = 1-player;
			instance->targets[0].card = instance->blocking;
			instance->number_of_targets = 1;
			kill_card(player, card, KILL_SACRIFICE);
		}
	}

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td) ){
			kill_card(instance->targets[0].player, instance->targets[0].card, KILL_DESTROY);
		}
	}

	return 0;
}

int card_wall_of_resistance(int player, int card, event_t event){

	/* Wall of Resistance	|1|W
	 * Creature - Wall 0/3
	 * Defender
	 * Flying
	 * At the beginning of each end step, if ~ was dealt damage this turn, put a +0/+1 counter on it. */

	card_instance_t* instance = get_card_instance(player, card);
	if( instance->info_slot != 66 && damage_dealt_to_me_arbitrary(player, card, event, 0, player, card) ){
		instance->info_slot = 66;
	}
	if( instance->info_slot == 66 && eot_trigger(player, card, event) ){
		add_counter(player, card, COUNTER_P0_P1);
		instance->info_slot = 0;
	}
	return 0;
}

int card_wall_of_roots(int player, int card, event_t event){

	/* Wall of Roots	|1|G
	 * Creature - Plant Wall 0/5
	 * Defender
	 * Put a -0/-1 counter on ~: Add |G to your mana pool. Activate this ability only once each turn. */

	card_instance_t *instance = get_card_instance(player, card);

	if( event == EVENT_CAN_ACTIVATE ){
		return instance->info_slot != 1 && can_produce_mana(player, card);
	}

	if(event == EVENT_ACTIVATE ){
		instance->info_slot = 1;
		add_counter(player, card, COUNTER_M0_M1);
		produce_mana(player, COLOR_GREEN, 1);
	}

	if( event == EVENT_COUNT_MANA && affect_me(player, card) && instance->info_slot != 1 && can_produce_mana(player, card) ){
		declare_mana_available(player, COLOR_GREEN, 1);
	}
	if( event == EVENT_CLEANUP ){
		instance->info_slot = 0;
	}

	return 0;
}

int card_warping_wurm(int player, int card, event_t event){
	int ph = phasing(player, card, event);
	if (ph){
		return ph;
	}


	upkeep_trigger_ability(player, card, event, player);

	if( event == EVENT_UPKEEP_TRIGGER_ABILITY ){
		int go_out = 1;
		if( has_mana_multi(player, 2, 0, 1, 1, 0, 0) ){
			int choice = do_dialog(player, player, card, -1, -1, " Pay upkeep\n Pass", 0);
			if( choice == 0 ){
				charge_mana_multi(player, 2, 0, 1, 1, 0, 0);
				if( spell_fizzled != 1 ){
					go_out = 0;
				}
			}
		}
		if( go_out == 1 ){
			phase_out(player, card);
		}
	}

	return 0;
}

int card_wildfire_emissary(int player, int card, event_t event)
{
  // 0x120240e

  /* Wildfire Emissary	|3|R
   * Creature - Efreet 2/4
   * Protection from |Swhite
   * |1|R: ~ gets +1/+0 until end of turn. */
  return generic_shade_merge_pt(player, card, event, 0, MANACOST_XR(1,1), 1,0);
}

int card_withering_boon(int player, int card, event_t event){

	if( event == EVENT_CAN_CAST){
		int result = card_remove_soul(player, card, event);
		if( result > 0 ){
			if( player == AI && can_pay_life(player, 3) ){
				return result;
			}
		}
	}
	else if( event == EVENT_CAST_SPELL && affect_me(player, card) ){
			 lose_life(player, 3);
			 return card_remove_soul(player, card, event);
	}

	return card_remove_soul(player, card, event);
}

int card_yare(int player, int card, event_t event)
{
  card_instance_t* instance = get_card_instance(player, card);

  target_definition_t td;
  default_target_definition(player, card, &td, TYPE_CREATURE);
  td.allowed_controller = 1 - current_turn;
  td.preferred_controller = player;

  if (event == EVENT_CAN_CAST)
	return can_target(&td);	// Unclear if "defending player controls" means this must be cast during combat.  I'm going to ignore it.

  if (event == EVENT_CAST_SPELL && affect_me(player, card))
	{
	  if (pick_target(&td, "TARGET_CREATURE")
		  && player == AI && instance->targets[0].player != AI)
		ai_modifier -= 48;
	}

  if (event == EVENT_RESOLVE_SPELL && valid_target(&td))
	{
	  int legacy = pump_ability_until_eot(player, card, instance->targets[0].player, instance->targets[0].card, 3, 0, 0, 0);
	  get_card_instance(player, legacy)->targets[5].player = 2;

	  kill_card(player, card, KILL_DESTROY);
	}

  return 0;
}

int card_zhalfirin_commander(int player, int card, event_t event){

	target_definition_t td1;
	default_target_definition(player, card, &td1, TYPE_CREATURE);
	td1.allowed_controller = player;
	td1.preferred_controller = player;
	td1.required_subtype = SUBTYPE_KNIGHT;
	td1.illegal_abilities = 0;

	card_instance_t *instance = get_card_instance( player, card );

	flanking(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		if( valid_target(&td1) ){
			pump_until_eot(player, instance->parent_card, instance->targets[0].player, instance->targets[0].card, 1, 1);
		}
	}
	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_CAN_TARGET, 1, 0, 0, 0, 0, 2, 0, &td1, "TARGET_CREATURE");
}

int card_zhalfirin_knight(int player, int card, event_t event){

	return generic_mirage_knight(player, card, event, COLOR_WHITE);
}

int card_zirilan_of_the_claw(int player, int card, event_t event){

	char msg[100] = "Select a Dragon creature.";
	test_definition_t this_test;
	new_default_test_definition(&this_test, TYPE_CREATURE, msg);
	this_test.subtype = SUBTYPE_DRAGON;

	card_instance_t *instance = get_card_instance( player, card );

	check_legend_rule(player, card, event);

	if( event == EVENT_RESOLVE_ACTIVATION ){
		int result = new_global_tutor(player, player, TUTOR_FROM_DECK, TUTOR_PLAY, 0, AI_MAX_CMC, &this_test);
		card_instance_t *parent = get_card_instance( player, instance->parent_card );
		parent->targets[2].card = 2+result;
		if( result > -1 ){
			create_targetted_legacy_effect(player, card, &haste_and_remove_eot, player, result);
		}
	}

	return generic_activated_ability(player, card, event, GAA_UNTAPPED+GAA_NONSICK+GAA_DECK_SEARCHER, 1, 0, 0, 0, 2, 0, 0, 0, 0);
}

int card_zuberi_golden_feather(int player, int card, event_t event){
	check_legend_rule(player, card, event);
	return boost_creature_type(player, card, event, SUBTYPE_GRIFFIN, 1, 1, 0, 0);
}
