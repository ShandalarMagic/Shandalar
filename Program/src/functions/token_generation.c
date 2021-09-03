#include "manalink.h"

void default_token_definition(int player, int card, int id, token_generation_t *token)
{
  token->id = id;
  token->eff_player = player;
  token->eff_card = card;
  token->t_player = player;

  if (player >= 0 && card >= 0)	// Find the real source if this was given either an activation card or an effect card
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  if (instance->internal_card_id == activation_card)
		{
		  player = instance->parent_controller;
		  card = instance->parent_card;
		}
	  else if (instance->internal_card_id >= 0)	// No need to check backup_internal_card_id; if it's tokens from an effect card that's already left play, it's always a bug
		{
		  int csvid = cards_data[instance->internal_card_id].id;
		  if (csvid >= 901 && csvid <= 908)
			{
			  player = instance->damage_source_player;
			  card = instance->damage_source_card;
			}
		}
	}
  if (player >= 0 && card >= 0)
	{
	  token->s_player = player;
	  token->s_card = card;
	  token->no_sleight = 0;
	}
  else
	{
	  token->s_player = token->s_card = -1;
	  token->no_sleight = 1;
	}

  token->qty = 1;
  token->pow = -1;
  token->tou = -1;
  token->key_plus = 0;
  token->s_key_plus = 0;
  token->color_forced = 0;
  token->special_infos = 0;
  token->action = 0;
  token->action_argument = 0;
  token->legacy = 0;
  token->special_code_for_legacy = &empty;
  token->special_code_on_generation = NULL;
  token->keep_track_of_tokens_generated = 0;
  token->special_flags2 = 0;
}

static int get_copyable_special_infos(int t_player, int t_card);

// Create a token definition that's a copy of (t_player,t_card).
void copy_token_definition(int player, int card, token_generation_t* token, int t_player, int t_card)
{
  int csvid = get_id(t_player, t_card);
  default_token_definition(player, card, csvid, token);
  token->no_sleight = 1;

  if (cards_ptr[csvid]->card_type == 8)	// A token defined in manalink.csv
	{
	  card_instance_t* instance = get_card_instance(t_player, t_card);
	  if (check_special_flags(t_player, t_card, SF_DONT_COPY_TOKEN_SOURCE))
		token->s_player = token->s_card = -1;
	  else
		{
		  token->s_player = instance->damage_source_player;
		  token->s_card = instance->damage_source_card;
		}
	  token->pow = instance->targets[5].player;
	  token->tou = instance->targets[5].card;
	  token->key_plus = instance->targets[6].player;
	  token->s_key_plus = instance->targets[7].player;
	  token->color_forced = instance->targets[7].card;
	  token->special_flags2 = instance->targets[8].card;
	  token->special_infos = get_copyable_special_infos(t_player, t_card);
	}
}

int copy_token_characteristics_for_clone(int player, int card, int t_player, int t_card)
{
  int csvid = get_id(t_player, t_card);
  if (cards_ptr[csvid]->card_type == 8)	// A token defined in manalink.csv
	{
	  card_instance_t* instance = get_card_instance(player, card);
	  card_instance_t* token = get_card_instance(t_player, t_card);

	  int i;
	  for (i = 4; i <= 8; ++i)
		instance->targets[i] = token->targets[i];	// struct copy

	  if (instance->targets[7].card > 0)
		instance->initial_color = instance->targets[7].card & COLOR_TEST_ANY_COLORED;

	  instance->color = instance->initial_color & COLOR_TEST_ANY_COLORED;
	  instance->regen_status |= KEYWORD_RECALC_SET_COLOR;

	  set_special_infos(player, card, get_copyable_special_infos(t_player, t_card));

	  return 1;
	}
  else
	return 0;
}

/* Just like generic_token(), but supports storing the values on an effect card to set them on a different permanent.  The idea is that changing a permanent to
 * temporarily be a copy of a token shouldn't have to overwrite all of its targets. */
int token_characteristic_setting_effects(int player, int card, event_t event, int t_player, int t_card){
	if (check_special_flags2(player, card, SF2_TEMPORARY_COPY_OF_TOKEN)){
		return 0;
	}
	card_instance_t* instance = get_card_instance(player, card);
	if( event == EVENT_POWER && affect_me(t_player, t_card) && instance->targets[5].player >= 0 ){
		event_result += instance->targets[5].player - get_base_power(player, card);
	}
	if( event == EVENT_TOUGHNESS && affect_me(t_player, t_card) && instance->targets[5].card >= 0 ){
		event_result += instance->targets[5].card - get_base_toughness(player, card);
	}
	if( event == EVENT_ABILITIES && affect_me(t_player, t_card) && instance->targets[6].player > 0 ){
		event_result |= instance->targets[6].player;
	}
	if( event == EVENT_SET_COLOR && affect_me(t_player, t_card) ){
		if (!(card == t_card && player == t_player)){
			if (instance->targets[7].card > 0){
				event_result = instance->targets[7].card & COLOR_TEST_ANY_COLORED;
			}
		}	// otherwise handled by setting initial_color in set_token()
	}
	if( instance->targets[7].player > 0 && instance->targets[7].player > 0 ){
		// Deliberately use the attached-to card as the effect source, not the attachment
		special_abilities(t_player, t_card, event, instance->targets[7].player, t_player, t_card);
	}
	if (event == EVENT_CHANGE_TYPE && affect_me(t_player, t_card) && instance->targets[8].card > 0){
		if (!(land_can_be_played & LCBP_DURING_EVENT_CHANGE_TYPE_SECOND_PASS)){
			// So there's a chance to set SF2_TEMPORARY_COPY_OF_TOKEN.
			land_can_be_played |= LCBP_NEED_EVENT_CHANGE_TYPE_SECOND_PASS;
		} else {
			set_special_flags2(t_player, t_card, instance->targets[8].card);
		}
	}
	return 0;
}

int generic_token(int player, int card, event_t event){
	return token_characteristic_setting_effects(player, card, event, player, card);
}

static void set_token(int player, int card, token_generation_t *token ){
	card_instance_t *instance = get_card_instance(player, card);
	instance->damage_source_player = token->s_player;
	instance->damage_source_card = token->s_card;
	instance->targets[5].player = token->pow;
	instance->targets[5].card = token->tou;
	instance->targets[6].player = token->key_plus;
	// instance->targets[6].card unused
	instance->targets[7].player = token->s_key_plus;
	// instance->targets[7].card set below
	instance->targets[8].card = token->special_flags2;

	color_test_t forced = token->color_forced;
	int sleight = token->s_player >= 0 && token->s_card >= 0 && !token->no_sleight && !(forced > 0 && (forced & 0x80));
	if (forced <= 0){
		forced = get_card_instance(player, card)->color;
	}
	if (sleight){
		forced = get_sleighted_color_test(token->s_player, token->s_card, forced & COLOR_TEST_ANY_COLORED);
	} else {
		forced &= COLOR_TEST_ANY_COLORED;
	}

	instance->initial_color = forced;
	instance->color = forced;
	instance->regen_status |= KEYWORD_RECALC_SET_COLOR;

	instance->targets[7].card = forced | 0x80;

	set_special_flags2(player, card, token->special_flags2);

	convert_to_token(player, card);
	set_special_infos(player, card, token->special_infos);
}

void convert_to_token(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	instance->token_status |= STATUS_TOKEN;
}

static int get_updated_tokens_number(int player, int number)
{
  card_instance_t* instance;
  int p, c;
  for (p = 0; p <= 1; ++p)
	for (c = 0; c < active_cards_count[p]; ++c)
	  if ((instance = in_play(p, c)))
		switch (cards_data[instance->internal_card_id].id)
		  {
			case CARD_ID_DOUBLING_SEASON:		// self only: doubles
			case CARD_ID_PARALLEL_LIVES:
			  if (p != player)
				continue;
			  // else fall through
			case CARD_ID_SELESNYA_LOFT_GARDENS:	// everyone: doubles
			case CARD_ID_PRIMAL_VIGOR:
			  number *= 2;
			  break;
		  }
  return number;
}

static int is_reserved_id(int csvid){
	int iid = get_internal_card_id_from_csv_id(csvid);
	return (cards_data[iid].cc[2] == 3	// vanguard card, edh, etc.
			|| cards_data[iid].type == TYPE_EFFECT);
}

void set_special_infos(int player, int card, int infos){
	card_instance_t *instance = get_card_instance(player, card);
	instance->targets[14].player = infos;
}

int get_special_infos(int player, int card){
	card_instance_t *instance = get_card_instance(player, card);
	if( instance->targets[14].player > -1 ){
		return instance->targets[14].player;
	}
	return 0;
}

static int get_copyable_special_infos(int t_player, int t_card)
{
  /* Augh.  Most special_infos are characteristic of the token, and so should be copied; some are added by the effect putting it into play, and so shouldn't.
   * But many of the latter are on token function that are shared among multiple ids and so are prohibitively difficult to test for; so only allow special_infos
   * that are whitelisted here. */
  int special_infos = get_special_infos(t_player, t_card);
  if (special_infos)
	switch (get_backup_id(t_player, t_card))
	  {
		case CARD_ID_ASSASSIN:
		  if (special_infos != 66)	// Vraska the Unseen assassin: has "Whenever this creature deals combat damage to a player, that player loses the game."
			special_infos = 0;
		  break;
		case CARD_ID_ASSEMBLY_WORKER:
		  if (special_infos != 66)	// Urza's Factory assembly-worker: does *not* have Mishra's Factory assembly-worker abilities
			special_infos = 0;
		  break;
		case CARD_ID_BAT:
		  if (special_infos != 66)	// Sengir Nosferatu bat: has "1B, Sacrifice this creature: Return an exiled card named Sengir Nosferatu to the battlefield under its owner's control."
			special_infos = 0;
		  break;
		case CARD_ID_CLERIC:
		  if (special_infos != 66)	// Deathpact Angel cleric: has "3WBB, T, Sacrifice this creature: Return a card named Deathpact Angel from your graveyard to the battlefield."
			special_infos = 0;
		  break;
		case CARD_ID_DRAGON:
		  if (special_infos != 66 // Dragon Broodmother dragon: has Devour 2
			  && special_infos != 67)	// Dragon Egg dragon: has "R: This creature gets +1/+0 until end of turn."
			special_infos = 0;
		  break;
		case CARD_ID_ELDRAZI_SPAWN:
		  if (special_infos != 66)	// Hedron Fields of Agadeem eldrazi: has annihilator 1; does *not* have eldrazi spawn mana generation.
			special_infos = 0;
		  break;
		case CARD_ID_ELEMENTAL:
		  if (special_infos != 67)	// Voice of Resurgence elemental: has "This creature's power and toughness are each equal to the number of creatures you control."
			special_infos = 0;
		  break;
		case CARD_ID_GOBLIN:
		  if (special_infos != 67)	// Goblin Assault goblin: has haste
			special_infos = 0;
		  break;
		case CARD_ID_ILLUSION:
		  if (special_infos != 66)	// Summoner's Bane illusion: does *not* have flying.
			special_infos = 0;
		  break;
		case CARD_ID_INSECT:
		  if (special_infos != 67)	// Carrion Call/Phyrexian Swarmlord/Trigon of Infestation insect: has infect
			special_infos = 0;
		  break;
		case CARD_ID_OOZE:
		  if (special_infos != 66	// Gutter Grime ooze: has "This creature's power and toughness are each equal to the number of slime counters on Gutter Grime."
			  && special_infos != 67)	// Mitotic Slime ooze: has "When this creature dies, put two 1/1 green Ooze creature tokens onto the battlefield."
			special_infos = 0;
		  break;
		case CARD_ID_SAPROLING:
		  if (special_infos != 66)	// Saproling Burst saproling: has "This creature's power and toughness are each equal to the number of fade counters on Saproling Burst."
			special_infos = 0;
		  break;
		case CARD_ID_WOLF:
		  if (special_infos != 66)	// Sound the Call wolf: has "This creature gets +1/+1 for each card named Sound the Call in each graveyard."
			special_infos = 0;
		  break;
		default:
		  special_infos = 0;
	  }
  return special_infos;
}

static int real_generate_token(int player, int int_id, token_generation_t *token, int delay_put_into_play){
	int card_added = add_card_to_hand( player, int_id );
	set_token(player, card_added, token);
	if (delay_put_into_play){
		get_card_instance(player, card_added)->state |= STATE_INVISIBLE;
		--hand_count[player];
	} else {
		put_into_play(player, card_added);
	}
	return card_added;
}

void generate_token(token_generation_t *token){
	int int_id = get_internal_card_id_from_csv_id(token->id);
	if(!(cards_data[int_id].type & TYPE_ARTIFACT) && (token->action & TOKEN_ACTION_CONVERT_INTO_ARTIFACT)){// not is_what() - deliberately ignore SF2_MYCOSYNTH_LATTICE
		int_id = create_a_card_type(int_id);
		cards_data[int_id].type |= TYPE_ARTIFACT;
	}
	int number = token->qty;
	if( ! is_what(-1, int_id, TYPE_EFFECT) ){
		number = get_updated_tokens_number(token->t_player, number);
	}

	int i;
	int card_added = -1;

	ASSERT(token->eff_card != -1 || !(token->keep_track_of_tokens_generated > 0 || token->legacy == 1 ||
									  (token->action & (TOKEN_ACTION_EQUIP | TOKEN_ACTION_HASTE | TOKEN_ACTION_PUMP_POWER | TOKEN_ACTION_PUMP_TOUGHNESS))));

	card_instance_t *source = token->keep_track_of_tokens_generated ? get_card_instance(token->eff_player, token->eff_card) : NULL;
	for( i = 0; i < number; i++){
		card_added = real_generate_token(token->t_player, int_id, token, 1);

		card_instance_t* instance = get_card_instance(token->t_player, card_added);

		if (token->legacy == 1){
			create_targetted_legacy_effect(token->eff_player, token->eff_card, token->special_code_for_legacy, token->t_player, card_added);
		}

		if (token->action & TOKEN_ACTION_TAPPED){
			instance->state |= STATE_TAPPED;	// Not tap_card() to avoid an EVENT_TAP_CARD
		}
		if (token->action & TOKEN_ACTION_ATTACKING_UNTAPPED){
			instance->state |= STATE_ATTACKING;
			choose_who_attack(token->t_player, card_added);
		}
		if (token->action & TOKEN_ACTION_BLOCKING){
			card_instance_t* to_block = get_card_instance(1 - token->t_player, token->action_argument);
			instance->blocking = to_block->blocking == 255 ? token->action_argument : to_block->blocking;
			instance->state |= STATE_UNKNOWN8000|STATE_BLOCKING;
		}
		if ((token->action & TOKEN_ACTION_EQUIP) && i == 0){
			equip_target_creature(token->eff_player, token->eff_card, token->t_player, card_added);
		}
		if (token->action & TOKEN_ACTION_DONT_COPY_TOKEN_SOURCE){
			set_special_flags(token->t_player, card_added, SF_DONT_COPY_TOKEN_SOURCE);
		}
		if (token->action & (TOKEN_ACTION_HASTE | TOKEN_ACTION_PUMP_POWER | TOKEN_ACTION_PUMP_TOUGHNESS)){
			pump_ability_until_eot(token->eff_player, token->eff_card, token->t_player, card_added,
								   (token->action & TOKEN_ACTION_PUMP_POWER) ? token->action_argument : 0,
								   (token->action & TOKEN_ACTION_PUMP_TOUGHNESS) ? token->action_argument : 0,
								   0,
								   (token->action & TOKEN_ACTION_HASTE) ? SP_KEYWORD_HASTE : 0);
		}
		if (token->action & TOKEN_ACTION_ADD_SUBTYPE){
			add_a_subtype(token->t_player, card_added, token->action_argument);
		}

		if (token->special_code_on_generation){
			(*token->special_code_on_generation)(token, card_added, i);
		}

		if( (int)token->keep_track_of_tokens_generated > i ){
			source->targets[i].player = token->t_player;
			source->targets[i].card = card_added;
		}

		put_into_play(token->t_player, card_added);

		if ((token->action & TOKEN_ACTION_BLOCKING) && in_play(token->t_player, card_added)){
			play_sound_effect(WAV_BLOCK2);
			if (event_flags & EA_SELECT_BLOCK){
				dispatch_trigger2(current_turn, TRIGGER_BLOCKER_CHOSEN, EXE_STR(0x790074)/*PROMPT_BLOCKERSELECTION[0]*/, 0, token->t_player, card_added);
			}
		}

		// legendary tokens
		if( token->special_infos == 99 ){
			// I'm almost certain this is no longer used.
			int p;
			int kill_me = 0;
			int leg_id = get_id(token->t_player, card_added);
			for(p=0;p<2;p++){
				int c = active_cards_count[p]-1;
				while( c > -1 ){
						if( in_play(p, c) && ! (p==token->t_player && c==card_added) ){
							if( get_id(p, c) == leg_id && get_special_infos(p, c) == 99 ){
								kill_card(p, c, KILL_SACRIFICE);
								kill_me = 1;
							}
						}
						c--;
				}
			}
			if( kill_me == 1 ){
				kill_card(token->t_player, card_added, KILL_SACRIFICE);
			}
		}
	}
	if( token->keep_track_of_tokens_generated > 0  ){
		token->keep_track_of_tokens_generated = MIN(token->keep_track_of_tokens_generated, (unsigned int)number);
	}
}

void copy_token(int player, int card, int tok_player, int tok_card ){
	if( tok_player < 0 || tok_card < 0 ){
		return;
	}

	token_generation_t token;
	copy_token_definition(player, card, &token, tok_player, tok_card);
	generate_token(&token);
}

static int generate_token_by_id_impl(int player, int card, int csvid, int number, int reserved){
	token_generation_t token;
	default_token_definition(player, card, csvid, &token);
	int int_id = get_internal_card_id_from_csv_id(csvid);
	if (reserved){
		token.no_sleight = 1;
		number = 1;
	} else {
		number = get_updated_tokens_number(player, number);
		if (number < 1){
			number = 1;
		}
	}

	int i;
	int card_added = -1;
	for( i = 0; i < number; i++){
		card_added = real_generate_token(player, int_id, &token, 0);
	}

	return card_added;
}

void generate_token_by_id(int player, int card, int csvid){
	generate_token_by_id_impl(player, card, csvid, 1, is_reserved_id(csvid));
}

void generate_tokens_by_id(int player, int card, int csvid, int howmany){
	generate_token_by_id_impl(player, card, csvid, howmany, is_reserved_id(csvid));
}

int generate_reserved_token_by_id(int player, int csvid){
	ASSERT(is_reserved_id(csvid));
	// It's only safe to use -1 for card because we're sending 1 for reserved, and therefore will be setting no_sleight=1
	return generate_token_by_id_impl(player, -1, csvid, 1, 1);
}
