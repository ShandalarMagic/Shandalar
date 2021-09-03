#include "manalink.h"

void damage_effects(int player, int card, event_t event){

	/* Nothing left here needs to be here; it can all move into the individual card functions, replacing the call to damage_effects().  Most, maybe all, of the
	 * trap conditions set by Rules Engine belong in effect_damage().  Some of the other stuff needs attention, e.g. War Elemental shouldn't be adding counters
	 * to itself until lethal damage is processed, the same way Spiritmonger and Fungusaur don't. */

	if( event == EVENT_DEAL_DAMAGE ){

		card_instance_t *instance = get_card_instance( player, card);

		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if (damage->internal_card_id != damage_card){
			/* Before we waste any more time here.  In particular, return before get_card_instance(damage->damage_source_player, damage->damage_source_card),
			 * which often fails with an ugly popup. */
			return;
		}

		enum {
			MODE_NONE = 0,
			MODE_FREEZE_WHEN_DAMAGE = 128,
		} mode = MODE_NONE;

		enum {
			GLOBALMODE_NONE = 0,
			GLOBALMODE_SHRIVELING_ROT = 64,
		} global_mode = GLOBALMODE_NONE;

		int csvid = get_id(player, card);

		if( check_for_special_ability(player, card, SP_KEYWORD_FREEZE_WHEN_DAMAGE) ||
			csvid == CARD_ID_KUMANO_MASTER_YAMABUSHI
		  ){
			mode |= MODE_FREEZE_WHEN_DAMAGE;
		}

		// Global modes
		if( is_what(player, card, TYPE_EFFECT) && instance->targets[2].card == CARD_ID_SHRIVELING_ROT ){
			global_mode |= GLOBALMODE_SHRIVELING_ROT;
		}

		if( damage->damage_source_player == player && damage->damage_source_card == card &&
			damage->info_slot > 0
		  ){
			if( damage->damage_target_card != -1 ){

				if( mode & MODE_FREEZE_WHEN_DAMAGE ){
					if( in_play(damage->damage_target_player, damage->damage_target_card) ){
						if( instance->targets[1].player < 2  ){
							instance->targets[1].player = 2;
						}
						int pos = instance->targets[1].player;
						if( pos < 10 ){
							instance->targets[pos].player = damage->damage_target_player;
							instance->targets[pos].card = damage->damage_target_card;
							instance->targets[1].player++;
						}
					}
				}
			}
			else{
				if( csvid == CARD_ID_SZADEK_LORD_OF_SECRETS && damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE) &&
					damage->damage_target_card == -1
				  ){
					add_1_1_counters(player, card, damage->info_slot);
					mill(damage->damage_target_player, damage->info_slot);
					damage->info_slot = 0;
				}
			}
		}
		else{
			if( csvid == CARD_ID_DRALNU_LICH_LORD ){
				if( damage->damage_target_player == player && damage->damage_target_card == card ){
					int good = 0;
					if( damage->info_slot > 0 ){
						good = damage->info_slot;
					}
					else{
						card_instance_t *trg = get_card_instance(damage->damage_source_player, damage->damage_source_card);
						if( trg->targets[16].player > 0 ){
							good = trg->targets[16].player;
						}
					}

					if( good > 0 ){
						if( instance->targets[9].player < 0 ){
							instance->targets[9].player = 0;
						}
						instance->targets[9].player += good;
					}
					damage->info_slot = 0;
				}
			}
			if( csvid == CARD_ID_LIVING_ARTIFACT ){
				if( damage->damage_target_player == player && damage->damage_target_card == -1 ){
					add_counters(player, card, COUNTER_VITALITY, damage->info_slot);
				}
			}
			if( csvid == CARD_ID_WAR_ELEMENTAL ){
				if( damage->damage_target_player == 1-player && damage->damage_target_card == -1 ){
					add_1_1_counters(player, card, damage->info_slot);
				}
			}
			if( csvid == CARD_ID_NIGHT_DEALINGS ){
				if( damage->damage_target_player == 1-player && damage->damage_source_player == player ){
					add_counters(player, card, COUNTER_THEFT, damage->info_slot);
				}
			}
			if( is_what(player, card, TYPE_EFFECT) && instance->targets[2].card == CARD_ID_DRAIN_LIFE ){
				if( damage->damage_target_player == instance->targets[0].player &&
					damage->damage_target_card == instance->targets[0].card
				  ){
					int max = damage->info_slot;
					if( damage->damage_target_card == -1 ){
						if( life[instance->targets[0].player] < max ){
							max = life[instance->targets[0].player];
						}
					}
					else{
						if( get_toughness(instance->targets[0].player, instance->targets[0].card) < max ){
							max = get_toughness(instance->targets[0].player, instance->targets[0].card);
						}
					}
					gain_life(player, max);
					kill_card(player, card, KILL_REMOVE);
				}
			}
			if( is_what(player, card, TYPE_EFFECT) && instance->targets[2].card == CARD_ID_BRIGHTFLAME ){
				if( get_id(damage->damage_source_player, damage->damage_source_card) == CARD_ID_BRIGHTFLAME ){
					gain_life(player, damage->info_slot);
				}
			}
			if( is_what(player, card, TYPE_EFFECT) && instance->targets[1].card == CARD_ID_SIMIC_BASILISK ){
				if( damage->damage_source_player == instance->targets[0].player &&
					damage->damage_source_card == instance->targets[0].card && damage->info_slot > 0 &&
					damage->damage_target_card != -1
				  ){
					if( instance->targets[1].player < 2 ){
						 instance->targets[1].player = 2;
					}
					int pos = instance->targets[1].player;
					if( pos < 10 ){
						instance->targets[pos].player = damage->damage_target_player;
						instance->targets[pos].card = damage->damage_target_card;
						instance->targets[1].player++;
					}
				}
			}
			if( csvid == CARD_ID_SPROUTING_PHYTOHYDRA || csvid == CARD_ID_VOLATILE_RIG ||
				csvid == CARD_ID_DEEP_SLUMBER_TITAN
			  ){
				if( damage->damage_target_player == player && damage->damage_target_card == card && damage->info_slot > 0
				  ){
					if( instance->targets[1].player < 0 ){
						 instance->targets[1].player = 0;
					}
					instance->targets[1].player++;
				}
			}
			if( csvid == CARD_ID_CIRCLE_OF_AFFLICTION ){
				if( damage->damage_target_player == player && damage->damage_target_card == -1 && damage->info_slot > 0 ){
					if( get_color(damage->damage_source_player, damage->damage_source_card) & instance->info_slot ){
						if( instance->targets[1].player < 0 ){
							 instance->targets[1].player = 0;
						}
						instance->targets[1].player++;
					}
				}
			}
			if( csvid == CARD_ID_GREATBOW_DOYEN ){
				if( damage->damage_source_player == player && damage->damage_target_card != -1 &&
					in_play(damage->damage_target_player, damage->damage_target_card) &&
					has_subtype(damage->damage_source_player, damage->damage_source_card, SUBTYPE_ARCHER)
				  ){
					if( instance->targets[1].player < 2  ){
						instance->targets[1].player = 2;
					}
					int pos = instance->targets[1].player;
					if( pos < 10 ){
						instance->targets[pos].player = damage->damage_target_player;
						instance->targets[pos].card = damage->info_slot;
						instance->targets[1].player++;
					}
				}
			}
			if( csvid == CARD_ID_OONAS_BLACKGUARD ){
				if( damage->damage_source_player == player && damage->damage_target_card == -1 &&
					in_play(damage->damage_source_player, damage->damage_source_card) &&
					count_1_1_counters(damage->damage_source_player, damage->damage_source_card) > 0 &&
					(damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE))
				  ){
					if( instance->targets[1].player < 0  ){
						instance->targets[1].player = 0;
					}
					instance->targets[1].player++;
				}
			}
			if( csvid == CARD_ID_DEUS_OF_CALAMITY ){
				if( damage->damage_source_player == player && damage->damage_source_card == card && damage->damage_target_card == -1 &&
					damage->info_slot > 5 ){
					instance->targets[1].player = 2+damage->damage_target_player;
				}
			}
			if (is_what(player, card, TYPE_EFFECT) && (instance->targets[2].card == CARD_ID_RUNESWORD || instance->targets[2].card == CARD_ID_BONE_SHAMAN)
				&& damage->damage_source_player == instance->damage_source_player
				&& damage->damage_source_card == instance->damage_source_card
				&& damage->damage_target_card >= 0 && in_play(damage->damage_target_player, damage->damage_target_card)){
				if( instance->targets[2].player < 3  ){
					instance->targets[2].player = 3;
				}
				int pos = instance->targets[2].player;
				if( pos < 10 ){
					instance->targets[pos].player = damage->damage_target_player;
					instance->targets[pos].card = damage->damage_target_card;
					instance->targets[2].player++;
				}
			}
			if( (global_mode & GLOBALMODE_SHRIVELING_ROT) && damage->damage_target_card != -1 &&
				in_play(damage->damage_target_player, damage->damage_target_card)
			  ){
				kill_card(damage->damage_target_player, damage->damage_target_card, KILL_DESTROY);
			}
		}
	}
}

int damage_creature(int tgt_player, int tgt_card, int32_t amt, int src_player, int src_card)
{
  // 0x4a66c0

  // This is the only place where damage cards are created; damage_creature_or_player() validates then forwards here, and damage_player() just forwards here.

  if (tgt_player == -1 || src_player == -1 || amt <= 0)
	return -1;

  int whose_bf = tgt_card == -1 ? tgt_player : src_player;

  int dmg_card = add_card_to_hand(whose_bf, damage_card);
  if (dmg_card == -1)
	return -1;

  land_can_be_played |= LCBP_PENDING_DAMAGE_CARDS;

  if (tgt_card != -1)
	{
	  card_instance_t* tgt_inst = get_card_instance(tgt_player, tgt_card);
	  if ((tgt_inst->token_status & STATUS_SPECIAL_BLOCKER)
		  && cards_data[tgt_inst->internal_card_id].code_pointer == 0x401010)	// card_multiblocker()
		{
		  tgt_player = tgt_inst->damage_source_player;
		  tgt_card = tgt_inst->damage_source_card;
		}
	}

  card_instance_t* dmg_inst = get_card_instance(whose_bf, dmg_card);
  dmg_inst->state |= STATE_IN_PLAY;
  --hand_count[whose_bf];
  dmg_inst->damage_target_player = tgt_player;
  dmg_inst->damage_target_card = tgt_card;
  dmg_inst->info_slot = amt;
  dmg_inst->damage_source_player = src_player;
  dmg_inst->damage_source_card = src_card;

  if (src_card == -1)
	{
	  dmg_inst->display_pic_csv_id = CARD_ID_SWAMP;
	  dmg_inst->display_pic_num = 0;
	  return dmg_card;
	}

  card_instance_t* src_inst = get_card_instance(src_player, src_card), *src_src_inst;

  int src_iid, src_typ, attacking;
  if (src_inst->internal_card_id == -1)
	{
	  src_iid = src_inst->backup_internal_card_id;
	  src_src_inst = src_inst;

	  src_typ = get_type_with_iid(src_player, src_card, src_iid);

	  attacking = src_inst->state & STATE_ATTACKING;

	  int csvid = cards_data[src_iid].id;
	  dmg_inst->display_pic_csv_id = csvid;
	  dmg_inst->display_pic_num = get_card_image_number(csvid, src_player, src_card);
	}
  else if (src_inst->internal_card_id == activation_card)
	{
	  // An activation card.

	  src_iid = src_inst->original_internal_card_id;
	  src_src_inst = get_card_instance(src_inst->parent_controller, src_inst->parent_card);

	  src_typ = get_type_with_iid(src_inst->parent_controller, src_inst->parent_card, src_iid);

	  attacking = 0;

	  dmg_inst->display_pic_csv_id = src_inst->display_pic_csv_id;
	  dmg_inst->display_pic_num = src_inst->display_pic_num;

	  dmg_inst->damage_source_player = src_inst->parent_controller;
	  dmg_inst->damage_source_card = src_inst->parent_card;
	}
  else if (cards_data[src_inst->internal_card_id].id == 902
		   || cards_data[src_inst->internal_card_id].id == 903
		   || cards_data[src_inst->internal_card_id].id == 907)
	{
	  /* An effect card.  Hopefully it's dealing damage the same turn it was created, or its source is still in play, otherwise we may end up looking at the
	   * wrong card's special_flags2 and sp_keywords.  I can't think of anything that leaves a damage-dealing legacy behind that lasts longer than eot. */

	  src_iid = src_inst->original_internal_card_id;
	  src_src_inst = get_card_instance(src_inst->damage_source_player, src_inst->damage_source_card);

	  src_typ = get_type_with_iid(src_inst->damage_source_player, src_inst->damage_source_card, src_iid);

	  attacking = 0;

	  dmg_inst->display_pic_csv_id = src_inst->display_pic_csv_id;
	  dmg_inst->display_pic_num = src_inst->display_pic_num;
	}
  else
	{
	  // A normal card.

	  src_iid = src_inst->internal_card_id;
	  src_src_inst = src_inst;

	  src_typ = get_type_with_iid(src_player, src_card, src_iid);

	  attacking = src_inst->state & STATE_ATTACKING;

	  int csvid = cards_data[src_iid].id;
	  dmg_inst->display_pic_csv_id = csvid;
	  dmg_inst->display_pic_num = get_card_image_number(csvid, src_player, src_card);
	}

  int src_colorbits = get_color(src_player, src_card);
  if (src_typ & TYPE_ARTIFACT)
	src_colorbits |= COLOR_TEST_ARTIFACT;

  dmg_inst->initial_color = dmg_inst->color = src_colorbits;

  dmg_inst->eot_toughness = dmg_card;

  if (attacking)
	{
	  if (current_phase == PHASE_FIRST_STRIKE_DAMAGE)
		dmg_inst->token_status |= STATUS_FIRST_STRIKE_DAMAGE;

	  if (current_phase == PHASE_NORMAL_COMBAT_DAMAGE)
		dmg_inst->token_status |= STATUS_COMBAT_DAMAGE;
	}

  /* For future use - store card's type, keyword, and sp_keyword abilities.
   * Only permanents can be humiliated or have sp_keyword abilities, since both share storage with the targets array, and instants and sorceries, in general,
   * actually use those for targets.  Instants and sorceries that actually *do* have a sp_keyword that affects damage dealing (currently deathtouch, wither,
   * infect, rfg_when_damage, and lifelink) need to be special-cased here.  Right now the only one that exists is Puncture Blast (wither). */
  dmg_inst->targets[3].player = src_typ;
  dmg_inst->targets[3].card = 0;


	if ( (!(src_typ & TYPE_PERMANENT) || is_humiliated_by_instance(src_src_inst)) && cards_data[src_iid].id != CARD_ID_PUNCTURE_BLAST
	  ){
		// Check for a non-humiliated Soulfire Grand Master
		int sgm_flag = 0;
		int i;
		for(i=0; i<active_cards_count[src_player]; i++){
			if( in_play(src_player, i) && get_id(src_player, i) == CARD_ID_SOULFIRE_GRAND_MASTER && ! is_humiliated(src_player, i) ){
				sgm_flag = 1;
				break;
			}
		}
		if( ! sgm_flag ){
			dmg_inst->regen_status = 0;
			dmg_inst->targets[16].card = 0;
		}
	}
  else
	{
	  dmg_inst->regen_status = src_src_inst->regen_status & ~KEYWORD_NONABILITIES;
	  if (src_src_inst->targets[16].card == -1)
		dmg_inst->targets[16].card = 0;
	  else
		dmg_inst->targets[16].card = src_src_inst->targets[16].card;
	}

  // Copy of the first 5 targets of the damage source. Needed for Bronze Horse
  // - Why?  What's wrong with looking at get_card_instance(damage->damage_source_player, damage->damage_source_card)?
  int k;
  for (k = MIN(src_inst->number_of_targets, 5); k >= 0; --k)
    dmg_inst->targets[k+5] = src_inst->targets[k];

  return dmg_card;
}

// Convenient frontend to damage_creature().  Deals dmg damage to targets[0].player/card.  Promotes to parent if an activation card.  Does not validate.
int damage_target0(int player, int card, int dmg)
{
  card_instance_t* instance = get_card_instance(player, card);
  if (instance->internal_card_id == activation_card)
	{
	  player = instance->parent_controller;
	  card = instance->parent_card;
	}
  return damage_creature(instance->targets[0].player, instance->targets[0].card, dmg, player, card);
}

static int remove_indestructible_until_eot(int player, int card, event_t event){

	if( get_card_instance(player, card)->damage_target_player > -1 ){
		card_instance_t* instance = get_card_instance(player, card);
		int p = instance->damage_target_player;
		int c = instance->damage_target_card;

		if( event == EVENT_ABILITIES && affect_me(p, c) ){
			remove_special_ability(p, c, SP_KEYWORD_INDESTRUCTIBLE);
		}
	}

	if( event == EVENT_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

int effect_damage(int player, int card, event_t event)
{
  // 0x4a0f00

  /* Local data:
   * damage_source_player/damage_source_card: source of damage
   * damage_target_player/damage_target_card: object being damaged.  (This about the *only* card or effect where those four names are correct.)
   * info_slot: amount of damage to be dealt
   * initial_color: color of damage source at the time it dealt damage.  Querying the source is usually slightly more accurate, occasionally much less.
   * state & STATE_TAPPED: this damage card has applied its damage, either marking damage on a creature, making a player lose life, removing loyalty counters
   *                       from a planeswalker, or adding -1/-1 or poison counters from wither or infect; and giving life from lifelink.  I seem to recall
   *                       some exe cards setting STATE_TAPPED when they prevent damage from a damage card instead of setting info_slot to 0; that convention
   *                       hasn't been followed in C code.
   * display_pic_csv_id: csvid of damage source at the time it dealt damage.
   * regen_status: keywords of damage source at the time it dealt damage.  This effect is special-cased in get_abilities to never recalculate.
   * targets[3].player: types of damage source (i.e. artifact/creature/enchantment/instant/land/planeswalker/sorcery) at the time it dealt damage.
   * targets[3].card: &0x10: if a creature is dealt damage by this card, it can't regenerate until end of turn.
   * targets[4].player/card: Planeswalker to deal damage to instead of player.  Only valid once the attacking_planeswalker effect card resolves.  Ideally,
   *                         that should instead reattach the damage card to the planeswalker, but just about everything during the damage prevention and
   *                         resolution steps assume that damage to anything but a player is to a creature, and would likely get confused.  (On the other
   *                         hand, I'd expect them all to assume that damage to a player is always to that player, too, and nothing's obviously broken.)
   *                         Failing that, it would seem to make sense to assign this during damage_creature() when the damage card is created, rather than
   *                         waiting for the attacking_planeswalker effect card to do so.  It would certainly simplify Vraska the Unseen, though probably
   *                         all the complexity there would just have to move to damage_creature().  Maybe if something else needs to be made to check.
   * targets[5]-targets[9]: copy of the damage source firts 5 targets. Needed for Bronze Horse.
   * targets[16].card: special keywords of damage source at the time it dealt damage.  Only set if the damage source was a permanent.
   */

  card_instance_t* instance;
  int abils;

  if ((land_can_be_played & LCBP_DAMAGE_PREVENTION)
	  && affect_me(player, card)
	  && (instance = get_card_instance(player, card))
	  && instance->damage_target_card != -1
	  && ((abils = get_abilities(instance->damage_target_player, instance->damage_target_card, EVENT_ABILITIES, -1))
		  & (KEYWORD_PROT_SORCERIES|KEYWORD_PROT_INTERRUPTS|KEYWORD_PROT_INSTANTS|KEYWORD_SHROUD|KEYWORD_PROT_ARTIFACTS|KEYWORD_PROT_COLORED)))
	{
	  int dsp = instance->damage_source_player;
	  int dsc = instance->damage_source_card;

	  int source_clr = get_color(dsp, dsc) & COLOR_TEST_ANY_COLORED;

	  if ((source_clr & (abils >> 10))
		  || ((abils & KEYWORD_PROT_ARTIFACTS)
			  && (instance->targets[3].player & TYPE_ARTIFACT)))
		{
		  get_card_instance(player, card)->info_slot = 0;
		  kill_card(player, card, KILL_BURY);
		}
	}

  if (event == EVENT_DEAL_DAMAGE
	  && affect_me(player, card)
	  && (instance = get_card_instance(player, card))
	  && !(instance->state & STATE_TAPPED))
	{
	  instance->state |= STATE_TAPPED;
	  instance->token_status |= STATUS_INVISIBLE_FX;

	  if (instance->info_slot <= 0)
		return 0;

	  card_instance_t* damaged;
	  // If dealing combat damage to a planeswalker, targets[4] will hold it.  Otherwise, copy from damage_target_player/card.
	  if (instance->targets[4].player < 0 || instance->targets[4].card < 0)
		{
		  instance->targets[4].player = instance->damage_target_player;
		  instance->targets[4].card = instance->damage_target_card;
		  if (instance->targets[4].card == -1)
			damaged = NULL;
		  else if (!(damaged = in_play(instance->targets[4].player, instance->targets[4].card)))	// creature being damaged is no longer in play
			return 0;
		}
	  else
		{
		  /* Rule 506.4: A permanent is removed from combat... if it's a planeswalker that's being attacked and stops being a planeswalker.... A planeswalker
		   * that's removed from combat stops being attacked.
		   *
		   * Rule 506.4c: If a creature is attacking a planeswalker, removing that planeswalker from combat doesn't remove that creature from combat. It
		   * continues to be an attacking creature, although it is attacking neither a player nor a planeswalker. It may be blocked. If it is unblocked, it will
		   * deal no combat damage. */
		  if (!(damaged = in_play(instance->targets[4].player, instance->targets[4].card))
			  || !is_planeswalker(instance->targets[4].player, instance->targets[4].card))
			return 0;
		}

		if( instance->info_slot > get_trap_condition(player, TRAP_MAX_DAMAGE_DEALT) ||
			instance->info_slot > get_trap_condition(1-player, TRAP_MAX_DAMAGE_DEALT)
		  ){
			set_trap_condition(player, TRAP_MAX_DAMAGE_DEALT, instance->info_slot);
			set_trap_condition(1-player, TRAP_MAX_DAMAGE_DEALT, instance->info_slot);
		}

	  if (instance->targets[4].card == -1)
		{	// Damaging a player.
		  play_sound_effect(WAV_LIFELOSS);

		  /* There was much code here to store the damage source type, source, target, and amount for use with the original wording and implementations for
		   * Reverse Damage, Reverse Polarity, and Simulacrum.  All three have now been reimplemented with their modern wording (which is no longer
		   * retroactive); nothing else looks at the data.  It does get backed up for AI speculation and stored in save games, so we'll be able to use the
		   * storage space - and at 9664 bytes, there's plenty of it - for globals once the places where it's initialized get removed. */

		  int damaged_player = instance->targets[4].player;
		  int src_p = instance->damage_source_player;
		  int src_c = instance->damage_source_card;
		  int damage_dealt = instance->info_slot;

			// Record various types of damage taken
			increase_trap_condition(damaged_player, TRAP_DAMAGE_TAKEN, damage_dealt);

		  if (instance->targets[3].player & TYPE_ARTIFACT)
			increase_trap_condition(damaged_player, TRAP_ARTIFACT_DAMAGE_TAKEN, damage_dealt);

		  if (instance->targets[3].player & TYPE_PERMANENT)
			{
				if( damaged_player == 0 ){
					set_special_flags2(src_p, src_c, SF2_HAS_DAMAGED_PLAYER0);
				}
				if( damaged_player == 1 ){
					set_special_flags2(src_p, src_c, SF2_HAS_DAMAGED_PLAYER1);
				}
				if ((instance->targets[3].player & TYPE_CREATURE) && (instance->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE)))
				{
				  increase_trap_condition(damaged_player, TRAP_DAMAGED_BY_CREATURE, 1);

				  if (has_subtype(src_p, src_c, SUBTYPE_ROGUE))
					increase_trap_condition(src_p, TRAP_PROWL_ROGUE, 1);

				  if (has_subtype(src_p, src_c, SUBTYPE_GOBLIN))
					increase_trap_condition(src_p, TRAP_PROWL_GOBLIN, 1);

				  if (has_subtype(src_p, src_c, SUBTYPE_FAERIE))
					increase_trap_condition(src_p, TRAP_PROWL_FAERIE, 1);
				}
			}

		  if (damaged_player != src_p
			  && (instance->initial_color & COLOR_TEST_RED)
			  && (instance->targets[3].player & (TYPE_SPELL | TARGET_TYPE_PLANESWALKER)))
			increase_trap_condition(src_p, TRAP_CHANDRAS_PHOENIX, 1);

		  /* Even if this is reduced by EVENT_DAMAGE_REDUCTION below - that's for Ali from Cairo-like effects, and the ruling is that the damage is still dealt
		   * but the damaged player's life total doesn't change (or doesn't change as much).  Similarly, Platinum Emperion will prevent the life total from
		   * changing in lose_life() below, but the damage is still dealt and lifelink still applies. */
		  if (instance->targets[16].card & SP_KEYWORD_LIFELINK)
			gain_life(instance->damage_source_player, damage_dealt);

		  EXE_DWORD_PTR(0x4EF1EC)[damaged_player] += damage_dealt;	// damage_taken_this_turn[]

		  if (instance->regen_status & KEYWORD_INFECT)
			poison(damaged_player, damage_dealt);
		  else
			{
			  int new_life = life[damaged_player] - damage_dealt;

			  if (new_life < 7)
				new_life = dispatch_event_with_initial_event_result(player, card, EVENT_DAMAGE_REDUCTION, new_life);

			  if (new_life < life[damaged_player])
				lose_life(damaged_player, life[damaged_player] - new_life);
			  else if (new_life > life[damaged_player])
				gain_life(damaged_player, new_life - life[damaged_player]);
			}
			// Special effects when dealing damage, if they are unique for a card they will be implemented here, to save Flags
			/*
			 Chandra, Roaring Flame
			-7: Chandra, Roaring Flame deals 6 damage to each opponent.
				Each player dealt damage this way gets an emblem with
				"At the beginning of your upkeep, this emblem deals 3 damage to you.
			*/
			if (instance->display_pic_csv_id == CARD_ID_CHANDRA_ROARING_FLAME &&
				instance->targets[10].card == CARD_ID_CHANDRA_ROARING_FLAME
			  ){
				token_generation_t token;
				default_token_definition(player, card, CARD_ID_CHANDRA_ROARING_FLAME_EMBLEM, &token);
				token.t_player = damaged_player;
				generate_token(&token);
			}
			/*
			 Aurelia's Fury
			Aurelia's Fury deals X damage divided as you choose among any number of target creatures and/or players.
			Tap each creature dealt damage this way. Players dealt damage this way can't cast noncreature spells this turn.
			*/
			if (instance->display_pic_csv_id == CARD_ID_AURELIAS_FURY &&
				instance->targets[10].card == CARD_ID_AURELIAS_FURY
			  ){
				int card_added = generate_reserved_token_by_id(player, CARD_ID_SPECIAL_EFFECT);//128
				card_instance_t *legacy = get_card_instance(player, card_added);
				legacy->targets[2].player = 128;
				legacy->targets[3].player = damaged_player;
				legacy->targets[2].card = CARD_ID_AURELIAS_FURY;
				create_card_name_legacy(player, card_added, CARD_ID_AURELIAS_FURY);
			}

		}
	  else
		{	// Damaging a creature or a planeswalker.
		  if (instance->targets[16].card & SP_KEYWORD_LIFELINK)
			gain_life(instance->damage_source_player, instance->info_slot);

		  if (!is_what(instance->targets[4].player, instance->targets[4].card, TYPE_CREATURE))
			{
			  if (is_planeswalker(instance->targets[4].player, instance->targets[4].card))
				remove_counters(instance->targets[4].player, instance->targets[4].card, COUNTER_LOYALTY, instance->info_slot);
			  // Otherwise, no effect.
			}
		  else
			{
			  if (instance->targets[3].card & DMG_MUST_ATTACK_IF_DEALT_DAMAGE_THIS_WAY)
				pump_ability_until_eot(instance->damage_source_player, instance->damage_source_card,
								instance->targets[4].player, instance->targets[4].card, 0, 0, 0, SP_KEYWORD_MUST_ATTACK);

			  if (instance->targets[3].card & DMG_TAP_IF_DEALT_DAMAGE_THIS_WAY)
				tap_card(instance->targets[4].player, instance->targets[4].card);

			  //If a creature dealt damage by the targeted creature would die this turn, exile that creature instead.
			  if (instance->targets[3].card & DMG_EXILE_IF_LETHALLY_DAMAGED_THIS_WAY){
					set_special_flags(instance->targets[4].player, instance->targets[4].card, SF_LETHAL_DAMAGE_EXILE);
					exile_if_would_be_put_into_graveyard(instance->damage_source_player, instance->damage_source_card,
														 instance->targets[4].player, instance->targets[4].card, 1);
			  }
			  if (instance->targets[3].card & DMG_CANT_REGENERATE_IF_DEALT_DAMAGE_THIS_WAY )
				cannot_regenerate_until_eot(instance->damage_source_player, instance->damage_source_card,
											instance->targets[4].player, instance->targets[4].card);

			  if (instance->targets[16].card & SP_KEYWORD_DEATHTOUCH)
				{
				  // A bletcherous special case, but better than wasting a sp_keyword_t bit in addition to the special_flags_t bit
				  if (instance->display_pic_csv_id == CARD_ID_PHAGE_THE_UNTOUCHABLE
					  && (instance->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE)))
					set_special_flags(instance->targets[4].player, instance->targets[4].card, SF_LETHAL_DAMAGE_BURY);
				  else
					set_special_flags(instance->targets[4].player, instance->targets[4].card, SF_LETHAL_DAMAGE_DESTROY);
				}

			  if (instance->targets[16].card & SP_KEYWORD_RFG_WHEN_DAMAGE)
				set_special_flags(instance->targets[4].player, instance->targets[4].card, SF_EXILE_IF_DAMAGED);


				//Special cases. As long as it's an effect carried by only one card, there's no point of wasting flags
				/*
				 Pathway Arrows English
				Equipped creature has "{2}, {T}: This creature deals 1 damage to target creature.
									If a colorless creature is dealt damage this way, tap it."
				*/
				if( instance->targets[10].card == CARD_ID_PATHWAY_ARROWS &&
					get_color(instance->targets[4].player, instance->targets[4].card) == 0
				  ){
					tap_card(instance->targets[4].player, instance->targets[4].card);
				}
				/* Burn from Within	|X|R
				 * ~ deals X damage to target creature or player. If a creature is dealt damage this way,
				 * it loses indestructible until end of turn. If that creature would die this turn, exile it instead. */
				if( instance->targets[10].card == CARD_ID_BURN_FROM_WITHIN ){
					create_targetted_legacy_effect(instance->targets[4].player, instance->targets[4].card, &remove_indestructible_until_eot,
													player, card);
				}
				/* Illusory Ambushers
				Whenever Illusory Ambusher is dealt damage, draw that many cards.*/
				if( get_id(instance->targets[4].player, instance->targets[4].card) == CARD_ID_ILLUSORY_AMBUSHER ){
					draw_cards(instance->targets[4].player, instance->info_slot);
				}

			  if ((instance->targets[16].card & SP_KEYWORD_WITHER)
				  || (instance->regen_status & KEYWORD_INFECT))
				{
				  ++hack_silent_counters;
				  add_minus1_minus1_counters(instance->targets[4].player, instance->targets[4].card, instance->info_slot);
				  --hack_silent_counters;
				}
			  else
				damaged->damage_on_card += instance->info_slot;
			}

		  if (ai_is_speculating != 1)
			{
			  play_sound_effect(WAV_DAMAGE);

			  /* All this does is SendMessageA(hwnd_MainClass, 0x464u, 0xFFu, 0).  It's always called through a wrapper that checks ai_is_speculating != 1
			   * (except from C where it's already been checked).  wndproc_MainClass responds by calling TENTATIVE_redisplay_all(0, 0xff) at 0x437ec0. */
			  EXE_FN(void, 0x437e20, void)();
			}
		}
	}
  return 0;
}

void after_damage(void)
{
  // 0x477340

  int player, card;
  for (player = 0; player <= 1; ++player)
	for (card = 0; card < active_cards_count[player]; ++card)
	  if (in_play(player, card) && is_what(player, card, TYPE_CREATURE))
		{
		  int kill_mode = 0;
		  int flags = check_special_flags(player, card, SF_LETHAL_DAMAGE_DESTROY | SF_LETHAL_DAMAGE_BURY | SF_LETHAL_DAMAGE_EXILE|SF_EXILE_IF_DAMAGED);
		  if (flags)	// Prevent them from retriggering if this gets regenerated
			remove_special_flags(player, card, SF_LETHAL_DAMAGE_DESTROY | SF_LETHAL_DAMAGE_BURY | SF_LETHAL_DAMAGE_EXILE|SF_EXILE_IF_DAMAGED);

			if ((flags & SF_EXILE_IF_DAMAGED) // Pit Spawn/Sword of Kaldra
				|| ((flags & SF_LETHAL_DAMAGE_EXILE) && (int)get_card_instance(player, card)->damage_on_card >= get_toughness(player, card))
			  ){
				kill_mode = KILL_REMOVE;
			}
			else if (flags & SF_LETHAL_DAMAGE_BURY){		// Phage the Untouchable
					kill_mode = KILL_BURY;
			}
			else if ((flags & SF_LETHAL_DAMAGE_DESTROY)	// Deathtouch
						|| (int)get_card_instance(player, card)->damage_on_card >= get_toughness(player, card)
				   ){
						kill_mode = KILL_DESTROY;
			}

			if (kill_mode){
				kill_card(player, card, kill_mode);
			}
			else{
				if( check_special_flags3(player, card, SF3_ARCHANGEL_OF_THUNE_COUNTER) ){
					int amount = get_card_instance(player, card)->eot_toughness;
					if( amount ){
						get_card_instance(player, card)->eot_toughness = 0;
						add_1_1_counters(player, card, amount);
					}
				}
			}
		}

  /* The original exe version did this in one pass, calling the code_pointer of instance->original_internal_card_id if the card was no longer in play after the
   * call to kill_card() (such as from an exile-if-destroyed effect).  That's problematic on a couple levels.  Whatever removed the destroyed card from play
   * (instead of just flagging it STATE_DYING) could just as easily have removed an earlier or later card instead, and so would call EVENT_AFTER_DAMAGE for the
   * earlier card but not the later one; and the only cards that handle EVENT_AFTER_DAMAGE are Fungusaur and now Fungus Sliver, neither of which actually want
   * to do anything for not-in-play cards anyway.
   *
   * The exe version also only dispatched EVENT_AFTER_DAMAGE to creature cards.  This was sufficient for Fungusaur, but isn't for Fungus Sliver. */

  for (player = 0; player <= 1; ++player)
	for (card = 0; card < active_cards_count[player]; ++card)
	  if (in_play(player, card))
		dispatch_event_with_attacker_to_one_card(player, card, EVENT_AFTER_DAMAGE, 1-player, -1);
}

// Returns a damage card during EVENT_PREVENT_DAMAGE if it's combat damage and doing more than 0.
card_instance_t* combat_damage_being_prevented(event_t event)
{
  card_instance_t* damage;
  if (event == EVENT_PREVENT_DAMAGE
	  && (damage = get_card_instance(affected_card_controller, affected_card))
	  && damage->internal_card_id == damage_card
	  && (damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE))
	  && damage->info_slot > 0)
	return damage;

  return NULL;
}

// Returns a damage card during EVENT_PREVENT_DAMAGE if it's noncombat damage and doing more than 0.
card_instance_t* noncombat_damage_being_prevented(event_t event)
{
  card_instance_t* damage;
  if (event == EVENT_PREVENT_DAMAGE
	  && (damage = get_card_instance(affected_card_controller, affected_card))
	  && damage->internal_card_id == damage_card
	  && !(damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE))
	  && damage->info_slot > 0)
	return damage;

  return NULL;
}

// Returns a damage card during EVENT_PREVENT_DAMAGE if it's doing more than 0.
card_instance_t* damage_being_prevented(event_t event)
{
  card_instance_t* damage;
  if (event == EVENT_PREVENT_DAMAGE
	  && (damage = get_card_instance(affected_card_controller, affected_card))
	  && damage->internal_card_id == damage_card
	  && damage->info_slot > 0)
	return damage;

  return NULL;
}

/* Returns a damage card during EVENT_DEAL_DAMAGE if it's combat damage and doing more than 0.  damage->targets[16].player will hold the damage dealt if it's
 * determinable, even in the case of wither/infect damage; this is no more robust than the previous workarounds were. */
card_instance_t* combat_damage_being_dealt(event_t event)
{
  card_instance_t* damage;
  if (event == EVENT_DEAL_DAMAGE
	  && (damage = get_card_instance(affected_card_controller, affected_card))
	  && damage->internal_card_id == damage_card
	  && (damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE)))
	{
	  if (damage->info_slot > 0)
		{
		  damage->targets[16].player = damage->info_slot;
		  return damage;
		}

	  card_instance_t* source = get_card_instance(damage->damage_source_player, damage->damage_source_card);
	  if (source->targets[16].player > 0)
		{
		  damage->targets[16].player = source->targets[16].player;
		  return damage;
		}
	}

  return NULL;
}

/* Returns a damage card during EVENT_DEAL_DAMAGE if it's noncombat damage and doing more than 0.  damage->targets[16].player will hold the damage dealt if it's
 * determinable, even in the case of wither/infect damage; this is no more robust than the previous workarounds were. */
card_instance_t* noncombat_damage_being_dealt(event_t event)
{
  card_instance_t* damage;
  if (event == EVENT_DEAL_DAMAGE
	  && (damage = get_card_instance(affected_card_controller, affected_card))
	  && damage->internal_card_id == damage_card
	  && !(damage->token_status & (STATUS_COMBAT_DAMAGE|STATUS_FIRST_STRIKE_DAMAGE)))
	{
	  if (damage->info_slot > 0)
		{
		  damage->targets[16].player = damage->info_slot;
		  return damage;
		}

	  card_instance_t* source = get_card_instance(damage->damage_source_player, damage->damage_source_card);
	  if (source->targets[16].player > 0)
		{
		  damage->targets[16].player = source->targets[16].player;
		  return damage;
		}
	}

  return NULL;
}

/* Returns a damage card during EVENT_DEAL_DAMAGE if it's doing more than 0.  damage->targets[16].player will hold the damage dealt if it's determinable, even
 * in the case of wither/infect damage; this is no more robust than the previous workarounds were. */
card_instance_t* damage_being_dealt(event_t event)
{
  card_instance_t* damage;
  if (event == EVENT_DEAL_DAMAGE
	  && (damage = get_card_instance(affected_card_controller, affected_card))
	  && damage->internal_card_id == damage_card)
	{
	  if (damage->info_slot > 0)
		{
		  damage->targets[16].player = damage->info_slot;
		  return damage;
		}

	  card_instance_t* source = get_card_instance(damage->damage_source_player, damage->damage_source_card);
	  if (source->targets[16].player > 0)
		{
		  damage->targets[16].player = source->targets[16].player;
		  return damage;
		}
	}

  return NULL;
}

// Returns nonzero if this damage card is being dealt to a planeswalker.
int damage_is_to_planeswalker(card_instance_t* damage)
{
  return (damage->damage_target_card == -1
		  ? ((damage->token_status & (STATUS_FIRST_STRIKE_DAMAGE|STATUS_COMBAT_DAMAGE))
			 && check_special_flags(damage->damage_source_player, damage->damage_source_card, SF_ATTACKING_PWALKER))
		  : is_planeswalker(damage->damage_target_player, damage->damage_target_card));
}

static int if_attached_creature_dies_do_something(int player, int card, event_t event){

	card_instance_t *instance = get_card_instance(player, card);
	int p = instance->damage_target_player;
	int c = instance->damage_target_card;

	int mode = instance->targets[1].card;
	// modes
	// 1<<0 --> Controller of source gains life equal to attached creature toughness [Abattoir Ghoul]
	// 1<<1 --> Controller of source may draw a card [Rot Wolf]

	if( p > -1 ){
		if( event == EVENT_GRAVEYARD_FROM_PLAY ){
			if( affect_me(p, c) && in_play(p, c) && get_card_instance(p, c)->kill_code > 0 &&
				get_card_instance(p, c)->kill_code < KILL_REMOVE &&
				! check_special_flags2(p, c, SF2_WILL_BE_EXILED_IF_PUTTED_IN_GRAVE)
			  ){
				if( mode && (mode & 1) ){
					instance->targets[1].player = get_toughness(p, c);
				}
				instance->targets[11].player = 1; //Otherwise "resolve_gfp_ability" won't trigger
				instance->damage_target_player = instance->damage_target_card = -1;
			}
		}
	}

	//This check is mandatory or the legacy card will be removed before EVENT_GRAVEYARD_ABILITY if source is killed in combat
	if( instance->targets[0].player > -1 && current_phase != PHASE_NORMAL_COMBAT_DAMAGE &&
		current_phase != PHASE_FIRST_STRIKE_DAMAGE
	  ){
		if( other_leaves_play(player, card, instance->targets[0].player, instance->targets[0].card, event) ){
			kill_card(player, card, KILL_REMOVE);
		}
	}

	if( resolve_gfp_ability(player, card, event, RESOLVE_TRIGGER_MANDATORY) ){
		if( mode && (mode & 1) && instance->targets[1].player > 0 ){
			gain_life(instance->targets[0].player, instance->targets[1].player);
		}
		if( mode && (mode & 2) ){
			if( do_dialog(instance->targets[0].player, player, card, -1, -1, " Draw a card\n Pass", 0) == 0 ){
				draw_cards(instance->targets[0].player, 1);
			}
		}
		kill_card(player, card, KILL_REMOVE);
	}

	if( current_phase == PHASE_CLEANUP ){
		kill_card(player, card, KILL_REMOVE);
	}

	return 0;
}

void if_a_creature_damaged_by_me_dies_do_something(int player, int card, event_t event, int mode){
	// modes
	// 1<<0 --> Controller of source gains life equal to attached creature toughness [Abattoir Ghoul]
	// 1<<1 --> Controller of source may draw a card [Rot Wolf]

	card_instance_t *instance = get_card_instance(player, card);

	if( ! in_play(player, card) || is_humiliated(player, card) ){
		return;
	}

	if( event == EVENT_DEAL_DAMAGE ){
		card_instance_t *damage= get_card_instance( affected_card_controller, affected_card);
		if( damage->internal_card_id == damage_card ){
			if( damage->damage_target_card != -1 && damage->damage_source_player == player &&
				damage->damage_source_card == card && damage->info_slot > 0
			  ){
				if( instance->info_slot < 0 ){
					instance->info_slot = 0;
				}
				if( instance->info_slot < 10 ){
					instance->targets[instance->info_slot].player = damage->damage_target_player;
					instance->targets[instance->info_slot].card = damage->damage_target_card;
					instance->info_slot++;
				}
			}
		}
	}

	if( instance->info_slot > 0 && trigger_condition == TRIGGER_DEAL_DAMAGE && affect_me(player, card) && reason_for_trigger_controller == player ){
		int i;
		for(i=0; i<instance->info_slot; i++){
			int legacy = create_targetted_legacy_effect(player, card, &if_attached_creature_dies_do_something,
														instance->targets[i].player, instance->targets[i].card);
			card_instance_t *leg = get_card_instance(player, legacy);
			leg->targets[0].player = player;
			leg->targets[0].card = card;
			leg->number_of_targets = 1;
			leg->targets[1].card = mode;
		}
		instance->info_slot = 0;
	}
}

