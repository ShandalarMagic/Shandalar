#include <ctype.h>
#include <limits.h>
#include "manalink.h"

// Functions to remember which cards are exiled by which.

// empty effect, used only for data storage and comparison purposes
static int exiledby_legacy(int player, int card, event_t event)
{
  /* Targets:
   * [0] through [17]: each of .player and .card are an iid of an exiled card.  0x80000000 is set if the card is owned by player 1.
   * [18].player: unused
   * [18].card: stores identifying id if this is detached - see exiledby_detach() */
  return 0;
}

static int exiledby_legacymatches(card_instance_t* instance, int player, int card)
{
  if (instance->internal_card_id != LEGACY_EFFECT_CUSTOM)
	return 0;

  if (instance->info_slot != (int)exiledby_legacy)
	return 0;

  if ((instance->state & (STATE_OUBLIETTED | STATE_INVISIBLE | STATE_IN_PLAY)) != STATE_IN_PLAY)	// not just in_play(player, card), since player may be -1 or -2
	return 0;

  if (player < 0)
	return instance->targets[18].card == card;
  else
	return instance->damage_target_player == player && instance->damage_target_card == card;
}

/* player/card: The card that exiled the cards to search for.
 * csvid: Csvid to display in the dialog.  Typically the same as player/card's; but must be provided since player/card may have been obliterated.
 * mode, testarg: Action and constraints per exiledby_choose_mode_t, documented in manalink.h.
 * cardtype_txt: Descriptor of cards exiled (e.g. "creature"), or NULL if arbitrary cards.
 * faceup: Whether the cards are exiled face-up (and thus should exist in rfg_ptr[][]) or face-down (and thus should not yet). */
int exiledby_choose(int player, int card, int csvid, exiledby_choose_mode_t mode, int testarg, const char* cardtype_txt, int faceup)
{
  int iids[1000];
  int* locs[1000];
  int num_found = 0;

  int tests = mode & ~EXBY_MODE_MASK;
  mode &= EXBY_MODE_MASK;

  if (mode != EXBY_MAX_VALUE && mode != EXBY_FIRST_FOUND)
	{
	  memset(iids, -1, 1000 * sizeof(int));
	  memset(locs, 0, 1000 * sizeof(int*));
	}

  int p = player;
  if (player < 0)
	player += 2;

  card_instance_t* instance;
  int i, leg, field, ai_choice = -1, highest_value = INT_MIN, can_cast = 0;
  /* We look at all cards each call, so if a card's no longer exiled, we remove it immediately.  Not quite as good as we'd like - if you exile two Atogs, then
   * Pull From Reality one, it'll still be listed twice. */
  for (leg = 0; leg < active_cards_count[player]; ++leg)
	{
	  instance = get_card_instance(player, leg);
	  if (exiledby_legacymatches(instance, p, card))
		for (i = 0; i < 18; ++i)
		  for (field = 0; field < 2; ++field)
			{
			  int* loc = field == 0 ? &instance->targets[i].player : &instance->targets[i].card;

			  if (*loc != -1)
				{
				  int plr = (*loc & 0x80000000) ? 1 : 0;
				  int iid = *loc & ~0x80000000;
				  if (faceup && !check_rfg(plr, cards_data[iid].id))
					*loc = -1;
				  else
					{
					  // > 0: pass test.  == 0: pass test, but just barely.  < 0: fail.
					  int test_cmc = (tests & EXBY_TEST_CMC_LE) ? testarg - get_cmc_by_id(cards_data[iid].id) : 99;
					  if (test_cmc < 0)
						continue;

					  int test_iid = ((tests & EXBY_TEST_IID) && !(iid == testarg || cards_data[iid].id == csvid)) ? -1 : 99;
					  if (test_iid < 0)
						continue;

					  int test_can_cast = 99;
					  if (tests & EXBY_TEST_CAN_CAST)
						{
						  test_can_cast = can_legally_play_iid_now(player, iid, testarg);
						  if (!test_can_cast)
							continue;
						  else if (test_can_cast != 99)
							test_can_cast = 1;
						}

					  int test_has_mana_to_cast = ((tests & EXBY_TEST_HAS_MANA_TO_CAST) && !has_mana_to_cast_iid(player, testarg, iid)) ? -1 : 99;
					  if (test_has_mana_to_cast < 0)
						continue;

					  int pass = MIN(test_cmc, test_iid);
					  pass = MIN(pass, test_can_cast);

					  if (pass < 0)
						continue;

					  if (mode == EXBY_FIRST_FOUND)
						return (int)loc;

					  if (mode == EXBY_CAN_CAST)
						{
						  if (test_can_cast == 99)
							return 99;
						  can_cast = 1;
						}

					  if (mode == EXBY_MAX_VALUE || mode == EXBY_CHOOSE)
						{
						  int val = my_base_value_by_id(iid);
						  if (pass == 0)
							val /= 2;
						  if (val > highest_value)
							{
							  highest_value = val;
							  ai_choice = num_found;
							}
						}

					  if (mode == EXBY_CHOOSE)
						{
						  iids[num_found] = iid;
						  locs[num_found] = loc;
						  ++num_found;
						}
					}
				}
			}
	}

  if (mode == EXBY_MAX_VALUE)
	return highest_value;

  if (mode == EXBY_FIRST_FOUND)
	return 0;

  if (mode == EXBY_CAN_CAST)
	return can_cast;

  if (IS_AI(player))
	{
	  if (ai_choice == -1)
		return 0;
	  else
		return (int)(locs[ai_choice]);
	}

  char buf[200];
  if (num_found == 0)
	{
	  sprintf(buf, "No %s%scards have been exiled by %s.", cardtype_txt ? cardtype_txt : "", cardtype_txt ? " " : "", cards_ptr[csvid]->name);
	  DIALOG(player, card, EVENT_ACTIVATE,
			 DLG_FULLCARD_CSVID(csvid),
			 DLG_MSG(buf));
	  return 0;
	}

  sprintf(buf, "%s%scards exiled by %s", cardtype_txt ? cardtype_txt : "", cardtype_txt ? " " : "", cards_ptr[csvid]->name);
  buf[0] = toupper(buf[0]);
  int selected = show_deck(player, iids, num_found, buf, 0, 0x7375B0);

  if (selected == -1){
	  return 0;
  } else {
	  return (int)(locs[selected]);
  }
}

// Find the first card exiled by player/card starting at ret_leg/ret_idx.
int* exiledby_find_any(int player, int card, int* ret_leg, int* ret_idx)
{
  int leg = ret_leg ? *ret_leg : 0, idx = ret_idx ? *ret_idx : 0, *loc = NULL;
  card_instance_t* instance;

  int p = player;
  if (player < 0)
	player += 2;

  for (; leg < active_cards_count[player]; ++leg)
	{
	  instance = get_card_instance(player, leg);
	  if (exiledby_legacymatches(instance, p, card))
		for (; idx < 18; ++idx)
		  {
			if (instance->targets[idx].player != -1)
			  {
				loc = &instance->targets[idx].player;
				goto break_outer;
			  }
			if (instance->targets[idx].card != -1)
			  {
				loc = &instance->targets[idx].card;
				goto break_outer;
			  }
		  }
	  idx = 0;
	}
 break_outer:

  if (!loc)	// i.e., not found
	leg = idx = 0;

  // Remember last-searched position, so we start here again in subsequent searches this activation
  if (ret_leg)
	*ret_leg = leg;

  if (ret_idx)
	*ret_idx = idx;

  return loc;
}

// Find the first card exiled by player/card starting at ret_leg/ret_idx with stored value needle.
int* exiledby_find(int player, int card, int needle, int* ret_leg, int* ret_idx)
{
  int leg = ret_leg ? *ret_leg : 0, idx = ret_idx ? *ret_idx : 0, *loc = NULL;
  card_instance_t* instance;

  int p = player;
  if (player < 0)
	player += 2;

  for (; leg < active_cards_count[player]; ++leg)
	{
	  instance = get_card_instance(player, leg);
	  if (exiledby_legacymatches(instance, p, card))
		for (; idx < 18; ++idx)
		  {
			if (instance->targets[idx].player == needle)
			  {
				loc = &instance->targets[idx].player;
				goto break_outer;
			  }
			if (instance->targets[idx].card == needle)
			  {
				loc = &instance->targets[idx].card;
				goto break_outer;
			  }
		  }
	  idx = 0;
	}
 break_outer:

  if (!loc)	// i.e., not found
	leg = idx = 0;

  // Remember last-searched position, so we start here again in subsequent searches this activation
  if (ret_leg)
	*ret_leg = leg;

  if (ret_idx)
	*ret_idx = idx;

  return loc;
}

// Returns total number of cards exiled by player/card owned by owned_by.
int exiledby_count(int player, int card, int owned_by)
{
  int leg = 0;
  int idx = 0;
  int count = 0;
  int* loc;

  int p = player;
  if (player < 0)
	player += 2;

  while ((loc = exiledby_find_any(p, card, &leg, &idx)))
	{
	  card_instance_t* instance = get_card_instance(player, leg);
	  if (owned_by == 0 ? !(*loc & 0x80000000)
		  : owned_by == 1 ? (*loc & 0x80000000)
		  : 1)
		++count;

	  if (loc == &instance->targets[idx].player
		  && instance->targets[idx].card != -1)
		{
		  loc = &instance->targets[idx].card;
		  if (owned_by == 0 ? !(*loc & 0x80000000)
			  : owned_by == 1 ? (*loc & 0x80000000)
			  : 1)
			++count;
		}

	  ++idx;
	}
  return count;
}

// Remembers that a card with iid owned by t_player was exiled.  This doesn't actually exiled the card; if the exiled card is to be face-up, the caller should do so.
void exiledby_remember(int player, int card, int t_player, int iid, int* ret_leg, int* ret_idx)
{
  if (iid == -1)
	return;

  ASSERT(player >= 0 && "exiledby_remember() cannot be called on detached effects");

  int* loc = exiledby_find(player, card, -1, ret_leg, ret_idx);

  if (!loc)
	{
	  int leg = create_targetted_legacy_effect(player, card, exiledby_legacy, player, card);
	  card_instance_t* instance = get_card_instance(player, leg);
	  instance->targets[0].player = instance->targets[0].card = -1;
	  instance->token_status |= STATUS_INVISIBLE_FX;
	  if (ret_leg)
		*ret_leg = leg;
	  if (ret_idx)
		*ret_idx = 0;
	  loc = &instance->targets[0].player;
	}

  ASSERT(!(iid & 0x80000000));
  if (t_player == 1)
	{
	  iid |= 0x80000000;
	  ASSERT(iid != -1);
	}
  *loc = iid;
}

/* Normally, these legacies are attached to the card that exiled them.  This is problematic when that permanent leaves the battlefield: all the attached effects
 * will be obliterated.  This function turns the effect cards into freestanding legacies, determines a unique id, and stores that id in their targets[18].card
 * so they can be distinguished from each other.  Returns the id.  Other functions in this package - except for exiledby_remember() - can then be called with
 * player=(player - 2), card=id to examine the legacies.  Caller will have to call exiledby_destroy_detached() later with that id, or else the legacies stay in place
 * permanently. */
int exiledby_detach(int player, int card)
{
  card_instance_t* instance;
  int leg, detach_id = 0;

  // First, find an unused detach_id.
  for (leg = 0; leg < active_cards_count[player]; ++leg)
	{
	  instance = get_card_instance(player, leg);
	  if (instance->internal_card_id == LEGACY_EFFECT_CUSTOM
		  && instance->info_slot == (int)exiledby_legacy
		  && instance->targets[18].card > detach_id)
		detach_id = instance->targets[18].card;
	}

  ++detach_id;

  // Now detach all exiledby legacies attached to player/card and set their detach_id.
  for (leg = 0; leg < active_cards_count[player]; ++leg)
	{
	  instance = get_card_instance(player, leg);
	  if (exiledby_legacymatches(instance, player, card))
		{
		  instance->damage_target_player = instance->damage_target_card = -1;
		  instance->targets[18].card = detach_id;
		}
	}

  return detach_id;
}

// Destroys all detached exiledby legacies set to detach_id.
void exiledby_destroy_detached(int player, int detach_id)
{
  int leg;
  for (leg = 0; leg < active_cards_count[player]; ++leg)
	if (exiledby_legacymatches(get_card_instance(player, leg), player - 2, detach_id))
	  kill_card(player, leg, KILL_REMOVE);
}

void exile_card_and_remember_it_on_exiledby(int player, int card, int t_player, int t_card){
	int owner = get_owner(t_player, t_card);
	int iid = get_card_instance(t_player, t_card)->original_internal_card_id;
	kill_card(t_player, t_card, KILL_REMOVE);
	exiledby_remember(player, card, owner, iid, NULL, NULL);
}

// empty effect, used only for data storage and comparison purposes
static int store_counters_legacy(int player, int card, event_t event)
{
  /* Targets:
   * [17].card: stores a "card" value used to identify a specific legacy of this type */
  return 0;
}

static void return_auras_and_counters_to_play(int player, int card){
  /* Targets:
   * [0].player: original owner of the main exiled card
   * [0].card: "original_internal_card_id" of the main exiled card
   * [1-17] : will store the exiled auras that originally enchanted the main exiled card.
			  For each pair, "player" contains the original owner of the exiled aura and "card" its "original_internal_card_id".
   * [18].player: copy of "mode" parameter of "exile_permanent_and_auras_attached", will be checked for EPAAC_RETURN_TO_PLAY_TAPPED
   * [18].card: contains a "card" value, used to match with "store_counters_legacy"
   */
	card_instance_t *instance = get_card_instance(player, card);
	int t_player = instance->targets[0].player;
	int card_added = -1;
	int mode = instance->targets[18].player;
	if( check_rfg(t_player, cards_data[instance->targets[0].card].id) ){
		card_added = add_card_to_hand(t_player, instance->targets[0].card);
		remove_card_from_rfg(t_player, cards_data[instance->targets[0].card].id);
		if (mode <= 0){
			put_into_play(t_player, card_added);
			return;
		}
		if (mode & EPAAC_RETURN_TO_PLAY_TAPPED){
			add_state(t_player, card_added, STATE_TAPPED);
		}
		put_into_play(t_player, card_added);
	}

	if( card_added > -1 ){
		if (mode & (EPAAC_STORE_COUNTERS_RETURN_EOT | EPAAC_STORE_COUNTERS_RETURN_IF_SOURCE_LEAVES_PLAY | EPAAC_TAWNOSS_COFFIN)){
			/* Search for a "store_counters_legacy" that matches. If it's found, add the counters stored on the legacy
			 * to the original card before putting it into play. */
			card_instance_t* leg;
			int count;
			for (count = 0; count < active_cards_count[player]; ++count ){
				if ((leg = in_play(player, count)) && leg->internal_card_id == LEGACY_EFFECT_CUSTOM && leg->info_slot == (int)store_counters_legacy &&
					leg->targets[17].card == instance->targets[18].card
				   ){
					copy_counters(t_player, card_added, player, count, 1);
					kill_card(player, count, KILL_REMOVE);
					break;
				}
			}
		}
	}
	

	// Then, retur to play all the exiled auras attached to the original exiled card.
	if (mode & (EPAAC_STORE_AURAS_RETURN_EOT | EPAAC_STORE_AURAS_RETURN_IF_SOURCE_LEAVES_PLAY | EPAAC_TAWNOSS_COFFIN)){
		int i;
		for(i=1; i<18; i++){
			if( instance->targets[i].player != -1 && check_rfg(instance->targets[i].player, cards_data[instance->targets[i].card].id) ){
				int aura = add_card_to_hand(instance->targets[i].player, instance->targets[i].card);
				remove_card_from_rfg(instance->targets[i].player, cards_data[instance->targets[i].card].id);
				card_instance_t *au = get_card_instance(instance->targets[i].player, aura);
				au->damage_target_player = t_player;
				au->damage_target_card = card_added;
				set_special_flags(instance->targets[i].player, aura, SF_TARGETS_ALREADY_SET);
				put_into_play(instance->targets[i].player, aura);
			}
		}
	}
}

static int return_auras_counters_on_eot(int player, int card, event_t event){
	if( eot_trigger(player, card, event) ){
		return_auras_and_counters_to_play(player, card);
		kill_card(player, card, KILL_REMOVE);
	}
	return 0;
}

static int return_auras_counters_if_source_leaves_play(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if (event == EVENT_SET_LEGACY_EFFECT_NAME && instance->targets[0].card > -1 ){
		scnprintf(set_legacy_effect_name_addr, 51, "%s", cards_ptr[cards_data[instance->targets[0].card].id]->name);
	}
	if( trigger_condition == TRIGGER_LEAVE_PLAY ){
		if( affect_me( player, card) && trigger_cause == instance->damage_target_card && trigger_cause_controller == instance->damage_target_player
			&& reason_for_trigger_controller == player )
		{
			if(event == EVENT_TRIGGER){
				event_result |= RESOLVE_TRIGGER_MANDATORY;
			}
			else if(event == EVENT_RESOLVE_TRIGGER){
					return_auras_and_counters_to_play(player, card);
					kill_card(player, card, KILL_REMOVE);
			}
		}
	}
	return 0;
}

static int tawnoss_coffin_legacy(int player, int card, event_t event){
	card_instance_t *instance = get_card_instance(player, card);
	if (event == EVENT_SET_LEGACY_EFFECT_NAME && instance->targets[0].card > -1 ){
		scnprintf(set_legacy_effect_name_addr, 51, "%s", cards_ptr[cards_data[instance->targets[0].card].id]->name);
	}
	if( instance->damage_target_player > -1 ){
		player_bits[player] |= PB_SEND_EVENT_UNTAP_CARD_TO_ALL;
		if( leaves_play(instance->damage_target_player, instance->damage_target_card, event) ||
			(event == EVENT_UNTAP_CARD && affect_me(instance->damage_target_player, instance->damage_target_card))
		  ){
			return_auras_and_counters_to_play(player, card);
			kill_card(player, card, KILL_REMOVE);
		}
	}
	return 0;
}

static void store_permanent_on_legacy(card_instance_t* leg, int to_copy_player, int to_copy_card, int targ_min, int targ_max){
	targ_max = targ_max > 19 ? 19 : targ_max;
	int k;
	for(k=targ_min; k<targ_max; k++){
		if( leg->targets[k].player == -1 ){
			leg->targets[k].player = get_owner(to_copy_player, to_copy_card);
			leg->targets[k].card = get_original_internal_card_id(to_copy_player, to_copy_card);
			break;
		}
	}
}

int exile_permanent_and_auras_attached(int player, int card, int t_player, int t_card, unsigned int mode){
	int rvalue = -1;
	if (is_token(t_player, t_card) || ! is_what(-1, get_original_internal_card_id(t_player, t_card), TYPE_PERMANENT) ){
		manipulate_auras_enchanting_target(player, card, t_player, t_card, NULL, KILL_REMOVE);
	} 
	else {
		int legacy_auras = -1;
		card_instance_t* leg = NULL;

		if (mode & EPAAC_STORE_AURAS_RETURN_EOT){
			legacy_auras = set_legacy_image(player, get_id(t_player, t_card), create_legacy_effect(player, card, &return_auras_counters_on_eot));
		}
		else if (mode & EPAAC_STORE_AURAS_RETURN_IF_SOURCE_LEAVES_PLAY){
			legacy_auras = create_targetted_legacy_effect(player, card, &return_auras_counters_if_source_leaves_play, player, card);
			get_card_instance(player, legacy_auras)->eot_toughness = EVENT_SET_LEGACY_EFFECT_NAME;
		} 
		else if (mode & EPAAC_TAWNOSS_COFFIN){
			legacy_auras = create_targetted_legacy_effect(player, card, &tawnoss_coffin_legacy, player, card);
			get_card_instance(player, legacy_auras)->eot_toughness = EVENT_SET_LEGACY_EFFECT_NAME;
		}
		if (legacy_auras != -1){
			leg = get_card_instance(player, legacy_auras);
			leg->targets[18].player = mode;
			leg->targets[18].card = legacy_auras;
		}

		int p, count;
		card_instance_t* aura;
		for(p=0; p<2; p++){
			for (count = active_cards_count[p]-1; count >= 0; --count){
				if( (aura = in_play(p, count)) && is_what(p, count, TYPE_ENCHANTMENT) && has_subtype(p, count, SUBTYPE_AURA) &&
					aura->damage_target_player == t_player && aura->damage_target_card == t_card
				  ){
					if( legacy_auras != -1 && !is_token(p, count) ){
						store_permanent_on_legacy(leg, p, count, 1, 18);
					}
					kill_card(p, count, KILL_REMOVE);
				}
			}
		}
		if (mode & (EPAAC_STORE_COUNTERS_RETURN_EOT | EPAAC_STORE_COUNTERS_RETURN_IF_SOURCE_LEAVES_PLAY | EPAAC_TAWNOSS_COFFIN)){
			int legacy_counters = create_legacy_effect(player, card, &store_counters_legacy);
			card_instance_t *sc = get_card_instance(player, legacy_counters);
			sc->targets[17].card = legacy_auras;
			sc->token_status |= STATUS_INVISIBLE_FX;
			copy_counters(player, legacy_counters, t_player, t_card, 1);
		}
		if (leg){
			leg->targets[0].player = get_owner(t_player, t_card);
			leg->targets[0].card = get_card_instance(t_player, t_card)->original_internal_card_id;
			rvalue = legacy_auras;
		}
	}
	kill_card(t_player, t_card, KILL_REMOVE);
	return rvalue;
}
