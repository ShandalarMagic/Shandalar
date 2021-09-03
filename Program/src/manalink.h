#ifndef MANALINK_H
#define MANALINK_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "defs.h"

#ifdef __cplusplus
extern "C" {
#endif

/* WtF?! Camelbox gcc does not understand NULL. Isn't it supposed to be part of standard C? */
#ifndef NULL
#define NULL ((void *)0)
#endif

#define opp (1-player)

extern trigger_t trigger_condition;
extern xtrigger_t xtrigger_impl_value_dont_use_directly;
static inline int xtrigger_condition(void)	// Deliberately int, so it can be compared against either a trigger_t or an xtrigger_t
{
  return trigger_condition == TRIGGER_XTRIGGER ? (int)xtrigger_impl_value_dont_use_directly : (int)trigger_condition;
}
extern enable_xtrigger_flags_t enable_xtrigger_flags;
extern int replace_milled;	// for XTRIGGER_REPLACE_MILL and XTRIGGER_REPLACE_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY
extern milled_t* cards_milled;	// for XTRIGGER_MILLED
extern int num_cards_milled;	// for XTRIGGER_MILLED
extern int gy_from_anywhere_pos, gy_from_anywhere_source;	// for XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY

extern card_txt_t* cards_txt;	// indexed by csvid
extern char* counter_names[COUNTER_end + 1];

extern target_t stack_cards[32];	// player/card pairs of the cards on the stack; stack_cards[0] is the leftmost; stack_cards[stack_size - 1] should always be the same as card_on_stack_controller/card_on_stack, so long as it's an actual card (card_on_stack/_controller never gets assigned for effect cards)
extern target_t stack_damage_targets[32];
extern stack_data_t stack_data[32];	// indexing same as stack_cards[].
extern int stack_phase[32];			// trigger that put the ability or card on the stack, if there was one; else the phase when it was put there.  Indexing same as stack_cards[].
extern int stack_size;				// number of cards on the stack

/* Global variables */
extern char ability_tooltip_names_exe[17][50];
extern int active_cards_count[2];
extern int active_player;
extern int affected_card;

/* A rating of how much the AI wants to keep a creature untapped and blocking.  Modify it during EVENT_ATTACK_RATING and EVENT_BLOCK_RATING.  The highest values
 * set in the exe is for Time Elemental, which gets +120 in EVENT_ATTACK_RATING (so the AI keeps it untapped and not attacking) and -120 in EVENT_DEFENSE_RATING
 * (so the AI doesn't want to block with it); Ali From Cairo, which gets +24 in EVENT_ATTACK_RATING and -128 in EVENT_DEFENSE_RATING; and Backfire, which adds
 * 24*power in EVENT_ATTACK_RATING for the creature it enchants.  Royal Assassin is +48/-48.  The most combat-oriented creature whose ratings are manipulated by
 * this is Ball Lightning, at -60 in EVENT_ATTACK_RATING and +60 in EVENT_DEFENSE_RATING. */
extern int ai_defensive_modifier;

extern int ai_modifier;
extern int ante;
extern int ante_cards[2][16];
extern int damage_card;
extern int affected_card_controller;
extern int forbid_attack;
extern int debug_mode;
extern int gauntlet_round;
extern int attacking_card;
extern int attacking_card_controller;
extern int game_number;
extern int cancel;	// alias of spell_fizzled
extern int cant_be_responded_to;	// If nonzero, then the top card or effect on the stack will resolve immediately and can't be responded to.  Should not be used on mana sources; if set while paying for mana, it'll make a non-spell effect being paid for unrespondable to, too.  See also tapped_for_mana_color.
extern card_data_t cards_data[];
extern card_data_t* cards_at_7c7000[];
extern int card_on_stack_controller;
extern int card_on_stack;
extern uint16_t cards_drawn_this_turn[2];
extern card_ptr_t *cards_ptr[]; /* need better name here */
extern int creature_count[2];
extern int current_phase;
extern int current_turn;
extern int *deck_ptr[];
extern int displayed_x_value;	// The amount shown in the dialog and claimed as spent when Flags:Act_Use_X is set
extern int event_flags;	// an event_flag_t
extern int event_result;
extern game_type_t game_type;
extern int *graveyard_ptr_mutable[];	// only use directly in deck.c, and only when changing
static inline const int* get_grave(int player){return graveyard_ptr_mutable[player];}
extern int graveyard_source[2][500];
extern int next_graveyard_source_id;

extern int hand_count[2];
extern char hunting_subtypes[15000];

extern int land_can_be_played;	// a land_can_be_played_t
extern int lands_played;
extern int ldoubleclicked;
extern int life[2];
extern int life_gained;	// Amount of life just gained during TRIGGER_GAIN_LIFE.
/* landsofcolor_controlled[player][color] contains the total number of all lands of the given color controlled by player.  It's possible it's meant to be the
 * total amount of mana produceable by all of player's lands, though.
 *
 * landsofcolor_controlled[player][COLOR_COLORLESS] is the total number of lands that aren't counted as a specific color, and [COLOR_ANY] keeps a count of all
 * lands.  Again, it might be intended to be colorless mana produceable and total mana produceable by lands. */
extern int landsofcolor_controlled[2][8];
/* basiclandtypes_controlled[player][color] contains the total number of lands controlled by player that are swamps/islands/forests/mountains/plains.
 * basiclandtypes_controlled[player][COLOR_COLORLESS] and [COLOR_ANY] keep a count of non-colored and total lands, respectively. */
extern int basiclandtypes_controlled[2][8];
extern int colors_of_lands_in_play[2];	// Contains a bitfield of all colors or mana at least one land that player controls can produce.  It's used only by the exe version of Fellwar Stone.
extern int raw_mana_available[2][8];	// mana declared via declare_mana_available().  Does *not* include any declared via declare_mana_available_hex(), nor any already in mana pool.
extern int max_x_value;
extern int must_attack;
extern int needed_mana_colors;
extern char opponent_name[100];
extern int opponent_skill;
extern char option_PhaseStoppers[2][38];
extern player_bits_t player_bits[2];
extern int poison_counters[2];
extern int pumpable_power[2];
extern int pumpable_toughness[2];
extern int reason_for_trigger_controller;
extern int *rfg_ptr[];
extern char strs_hunting_subtypenames[250][300];
extern int tapped_for_mana_color;	// Set to COLOR_COLORLESS..COLOR_ARTIFACT in the exe, or -1 for none.  In particular, it's cleared by tap_card() when called on a card with EA_MANA_SOURCE set.  Only ever compared to -1 except in Mana Flare.  If >= 0, then the current effect or card on the stack will resolve immediately and can't be responded to.  Should only be set >0 if a card is actually tapping to produce mana, not just producing mana in general; use -2 for non-respondable non-tapping mana production.  See also cant_be_responded_to.
extern int tapped_for_mana[COLOR_ARTIFACT + 1];	// If more than a single mana of a single color was tapped for, set tapped_for_mana_color to 0x100 and set the amounts in this.  Not an exe variable, but works closely with tapped_for_mana_color, which is.
extern char text_lines[249][300];
extern int total_power_of_creatures_by_color[2][8];	// 0x00739D60
extern int total_toughness_of_creatures_by_color[2][8];	// 0x0073A230
extern int trigger_cause;
extern int trigger_cause_controller;
extern int turn_count;
extern int types_of_cards_in_graveyard[2];
extern int spell_fizzled;	// alias of cancel
extern int suppress_draw;
extern int x_value;
extern int time_walk_flag;
extern int trace_mode;
extern uint8_t types_that_dont_untap;
extern int unknown62BCEC;
extern int mana_paid[7];	// Mana paid in the last successful call to charge_mana().
extern int mana_pool[16];
extern int mana_in_pool[2][8];	// Same array as mana_pool, but matches original microprose declaration.  Transitional until mana_pool[] uses are cleaned up.
extern int available_slots;
char card_coded[30000/8];
extern int creatures_dead_this_turn;
extern int activation_card;
extern int ai_is_speculating;
extern int iid_draw_a_card;

extern uint8_t mana_doesnt_drain_from_pool[2][7];	// Should not be set directly, except to add bits MANADRAIN_DOESNT_DRAIN or MANADRAIN_BECOMES_COLORLESS during EVENT_MANA_POOL_DRAINING.
extern int16_t number_of_attackers_declared;	// valid only after an EVENT_DECLARE_ATTACKERS

/***********************************************************************************************************************************************************
 *   The first of these four globals (charge_mana_addr_of_pre_mana_pool) is NULL and the other three have the addresses of their corresponding globals     *
 * except when a card is activated from within charge_mana().  The globals they're associated with (mana_pool[8*player], x_value, pay_mana_xbugrw, and     *
 * max_x_value) are copied into locals and then (except for mana_pool) zeroed.                                                                             *
 *    Since the activated card needs to know which colors of mana to activate for, the pay_mana_xbugrw array is consolidated into a color_test_t value and *
 * stored in needed_colors.  Unfortunately, 1) the *amount* of each color needed is lost, and we sometimes need it (e.g., for Axebane Guardian); and       *
 * 2) before the global values are restored but after needed_colors is itself zeroed, the EVENT_TAP_CARD that Mana Flare and similar cards work through is *
 * dispatched.                                                                                                                                             *
 *    So what we do is store the addresses of the four local variables that temporarily store these globals into four *other* globals sometime before we   *
 * start calling card functions, and restore the previous values there afterwards so there's no chance they'll have stale values.  The names are           *
 * deliberately chosen to be clunky so as to discourage use except in the narrow window where they're necessary.                                           *
 ***********************************************************************************************************************************************************/
extern int (*charge_mana_addr_of_pre_mana_pool)[8];		// copy of player's mana pool before activation
extern int *charge_mana_addr_of_x_value;				// copy of x_value before activation
extern int (*charge_mana_addr_of_pay_mana_xbugrw)[7];	// copy of PAY_MANA_COLORLESS...PAY_MANA_WHITE+1 before activation
extern int *charge_mana_addr_of_max_x_value;			// copy of max_x_value before activation

extern int LEGACY_EFFECT_CUSTOM;
extern int LEGACY_EFFECT_ACTIVATED;
extern int LEGACY_EFFECT_PUMP;
extern int LEGACY_EFFECT_GENERIC;
extern int LEGACY_EFFECT_ITH;
extern int LEGACY_EFFECT_ALADDIN;	// iid_legacy_control
extern int LEGACY_EFFECT_ASTERISK;
extern int LEGACY_EFFECT_ASWAN;
extern int LEGACY_EFFECT_DISINTEGRATE;
extern int LEGACY_EFFECT_SORCERESS_QUEEN;
extern int LEGACY_EFFECT_STONING;
extern int LEGACY_EFFECT_TIME_WALK;

extern char COST_COLORLESS;
extern char COST_BLACK;
extern char COST_BLUE;
extern char COST_GREEN;
extern char COST_RED;
extern char COST_WHITE;
extern char MANA_COLORLESS;
extern char MANA_BLACK;
extern char MANA_BLUE;
extern char MANA_GREEN;
extern char MANA_RED;
extern char MANA_WHITE;
extern char MANA_ARTIFACT;
extern char MANA_UNKNOWN;
extern int PAY_MANA_COLORLESS;
extern int PAY_MANA_BLACK;
extern int PAY_MANA_BLUE;
extern int PAY_MANA_GREEN;
extern int PAY_MANA_RED;
extern int PAY_MANA_WHITE;
extern int PAY_MANA_ARTIFACT;

char* set_legacy_effect_name_addr;

int pick_creature_to_regen(target_definition_t td);
int is_creature_dead(void);
void base_target_definition(int player, int card, target_definition_t *td, int type);	// Like default_target_definition, but doesn't set illegal_abilities.
void default_target_definition(int player, int card, target_definition_t *td, int type);
void counterspell_target_definition(int player, int card, target_definition_t *td, int type);
void counter_activated_target_definition(int player, int card, target_definition_t* td, int type);
int can_target(target_definition_t *td);
int target_available(int player, int card, target_definition_t *td);
extern const char* hack_prepend_prompt;	// Set this to a string to prepend it (and a colon and space) to all targeting prompts.  Set to NULL when done.
int pick_target(target_definition_t *td, const char *prompt);	// Selects a target into (td->player, td->card)->targets[0].  Loads prompt from Text.res.  Sets spell_fizzled if cancelled.
int new_pick_target(target_definition_t *td, const char *prompt, int ret_location, int sp_fizzled);	// Selects a target into (td->player, td->card)->targets[ret_location], or into the next unused slot (counting by instance->number_of_targets) if ret_location is -1.  Loads prompt from Text.res.  Sets spell_fizzled if cancelled and sp_fizzled is nonzero.
int pick_next_target_noload(target_definition_t *td, const char *prompt);	// Selects a target into (td->player, td->card)'s next unused target slot (counting by instance->number_of_targets).  prompt is used literally, not loaded from Text.res.  Sets spell_fizzled if cancelled.
int pick_up_to_n_targets(target_definition_t* td, const char* prompt, int num);	// Selects up to num targets starting from (td->player, td->card)->targets[0].  Returns actual number chosen (also in number_of_targets).  Set allow_cancel to 3 to show both Done and Cancel buttons.  Sets spell_fizzled only if the Cancel button is clicked, not if the Done button is.
int pick_up_to_n_targets_noload(target_definition_t* td, const char* prompt, int num);	// Just like pick_up_to_n_targets(), but uses prompt literally instead of loading from Text.res.
int mark_up_to_n_targets_noload(target_definition_t* td, const char* prompt, int num, char (*marked)[151]);	// Just like pick_up_to_n_targets_noload(), but doesn't change (td->player,td->card)'s number_of_targets or targets array, instead marking targets in marked[][].  If the Cancel button is pushed, returns 0 and sets cancel to 1, but leaves previously-chosen targets set in marked.  marked should be a char array[2][151] initialized to 0.  Set num to -1 to allow any number of targets.  This should only be used for non-targetted things resolving immediately after chosen, since they won't track change of control or be seen by when-this-becomes-targeted triggers.
int pick_next_target_arbitrary(target_definition_t *td, const char *prompt, int player, int card);	// Selects a target into next (player, card)'s unused target slot (counting by instance->number_of_targets), while using (td->player, td->card) as the targeting source.  Sets spell_fizzled if cancelled.
int pick_next_target_noload_arbitrary(target_definition_t *td, const char *prompt, int player, int card);	// Selects a target into next (player, card)'s unused target slot (counting by instance->number_of_targets), while using (td->player, td->card) as the targeting source.  prompt is used literally, not loaded from Text.res.  Sets spell_fizzled if cancelled.
int select_target(int player, int card, target_definition_t *td, const char *prompt, target_t *ret_location);	// Selects a target into ret_location, or (player, card)->targets[0] if NULL.  Targeting source is player/card instead of td->player/td->card.  prompt is used literally, not loaded from Text.res.  Never sets spell_fizzled.
int pick_player_duh(int player, int card, int preferred_controller, int allow_cancel);	// If duh mode is on, pick preferred_controller if he's a valid target, or else cancel if that's allowed.  Otherwise, forwards to pick_target(td, "TARGET_PLAYER").
int target_opponent(int player, int card);	// (player/card) targets 1-player in its targets[0] and returns nonzero, else returns 0.
int opponent_is_valid_target(int player, int card);	// Returns nonzero if (player/card) can target 1-player, else 0.  The difference from target_opponent() is that it doesn't assign the target, increase number_of_targets, or set cancel.
// Should be used only when actually validating an already-chosen target, i.e. during EVENT_RESOLVE_SPELL, EVENT_RESOLVE_ACTIVATION, resolution of a trigger, etc.
int validate_target(int player, int card, target_definition_t *td, int target_number);
int validate_arbitrary_target(target_definition_t* td, int tgt_player, int tgt_card);
int valid_target(target_definition_t *td);
// Identical to validate_target(), validate_arbitrary_target(), and valid_target() respectively, but for use when choosing a target.
int would_validate_target(int player, int card, target_definition_t *td, int target_number);
int would_validate_arbitrary_target(target_definition_t* td, int tgt_player, int tgt_card);
int would_valid_target(target_definition_t *td);

/* Functions in the exe */
void add_one_mana_any_color(int, int, event_t);
int another_copy_attached(int player, int card, int iid);	// returns nonzero if a card with internal_card_id iid is attached to get_card_instance(player, card)->targets[0]
int aura_ability(int, int, event_t, int);
int aura_pump(int, int, event_t, int, int);
void real_bounce_permanent(int, int);
int can_attack(int player, int card);
void cannot_block_exe(int, int, event_t);
void charge_mana(int, color_t, int);
int check_hand_for_type(int, int, type_t);
void check_legend_rule(int, int, event_t);
int check_mana_multi(int);
int choose_a_card(const char *, int, int );
int choose_a_color_exe(int player, const char* prompt, int use_color_names_instead_of_land, int ai_choice, int color_tests);
int choose_a_number(int, const char *, int );
int coin_flip(int player, const char* dialog_title, int show_dialog_if_animation_is_off);	// Last parameter should always be 1 except during game startup
int cop2(int, int, int, int);
void count_mana(void);
int damage_creature(int target_player, int target_card, int amount, int source_player, int source_card);
int damage_target0(int player, int card, int dmg);	// Convenient frontend to damage_creature().  Deals dmg damage to targets[0].player/card.  Promotes to parent if an activation card.  Does not validate.
int damage_creature_or_player(int source_player, int source_card, event_t event, int amount);	// Mostly a wrapper around damage_creature().  Also validates.  Damage is always dealt by activating_player/activating_card; the player/card given are used only in validation.  This usually isn't what you want - damage_target0 is better.  Avoid.
int damage_player(int target_player, int amount, int source_player, int source_card);	// Identical to damage_creature(target_player, -1, amount, source_player, source_card)
void deathtouch(int player, int card, event_t event);
void declare_mana_available(int player, color_t color, int amount);
void declare_mana_available_hex(int player, color_test_t colors, int amount);
void discard_exe(int player, int random, int there_isnt_actually_a_third_argument);	// Third argument used to be "this discard can be replaced by Library of Leng", which only replaces effects, not costs or end-of-turn discard (and used to be "if a card forces you to discard", so it gets the wrong value for Mind Bomb and Cursed Rack).  It gets ignored now instead of being forwarded to discard_card_exe().
void discard(int player, int flags, int player_who_controls_effect);	// flags is a discard_flags_t
void discard_card(int player, int card);
void discard_card_exe(int player, int card);
void new_discard_card(int player, int card, int player_who_controls_effect, int flags);
void multidiscard(int player, int quantity, int random);
void new_multidiscard(int player, int quantity, int flags, int player_who_controls_effect);
void new_discard_all(int player, int player_who_controls_effect);
int dispatch_event(int player, int card, event_t event);
int do_action_with_type_in_hand(int, int, type_t, int);
int do_dialog(int who_chooses, int bigcard_player, int bigcard_card, int smallcard_player, int smallcard_card, const char* options, int ai_choice);
int draw_a_card(int player);
int draw_cards_exe(int,int);
int effect_asterisk(int player, int card, event_t event);
int enchant_world(int, int, event_t);
void end_the_game(int);
int fear_exe(int, int, event_t);
void fear(int player, int card, event_t event);
int flash(int, int, event_t);
void real_gain_life(int, int);
int get_abilities(int player, int card, event_t event, int new_attacking_card);
int get_card_image_number(int csvid, int player, int card);
int get_sleighted_color(int, int, int);
int get_internal_card_id_from_csv_id(int);
int get_random_creature_type(int);
int has_mana(int, color_t, int);
void indestructible(int player, int card, event_t event);
int internal_rand(int upperbound);	// Returns a pseudorandom number between 0 and upperbound-1 inclusive.
int remember_ai_value(int player, int value);	// What this seems to do is, during speculation, store value for later retrieval and returns it; and during actual execution, retrieve the value that was stored during the best path.  Not well understood.
int recorded_rand(int player, int upperbound);	// Like internal_rand(), but if player is the AI, stores and retrieves the choice per remember_ai_value()
int islandhome(int, int, event_t);
int is_legal_block(int blocking_player, int blocking_card, int blocked_player, int blocked_card);
int is_tapped(int, int);
void kill_card(int, int, kill_t);
void load_text(int, const char *);
int mana_producer_fixed(int, int, event_t, int);
void obliterate_top_card_of_stack(void);
void put_card_or_activation_onto_stack(int player, int card, event_t event, int unknown1, int unknown2);	// unknown1 is almost always 0, occasionally player.  unknown2 always 0.  A reference to the card is put onto the stack for EVENT_RESOLVE_SPELL/EVENT_RESOLVE_TRIGGER, otherwise an activation card is created and put there instead.
int pro_black(int, int, event_t);
void produce_mana(int, int, int);
void real_lose_the_game(int);
int real_put_into_play(int, int);
int raw_put_iid_on_top_of_deck(int player, int iid);	// Put a newly-created card with internal_card_id iid on top of player's library.
int sprintf(char *str, const char *format, ...);
int rearrange_top_3(int, int, event_t);
int regenerate(int, int, event_t, int, int);
int regenerate_green(int , int, event_t);
int regenerate_target_exe(int, int);
int remove_card_from_deck(int, int);
void remove_card_from_grave(int player, int position);
int should_ai_play(int, int);	// equivalent to in_play(); has nothing to do with ai
int show_deck(int player, const int* deck, int num_cards, const char* prompt, int suppress_done_label, /* const char* */int done_label);	// no desire to cast all the references to 0x7375B0 to const char* (they're the localized "Done" string in Text.res:DIALOGBUTTONS[3]).  See also show_bottom_of_deck() below.
int signet(int, int, event_t, int, int);
int shuffle_exe(int, int);
int tap_card(int player, int card);
void target_player(int, int, event_t);
int type_tutor(int, int, event_t, int, int, int, int);
int urzatron_parts(int player);
void vigilance(int player, int card, event_t event);
int zombie_regen(int, int, event_t);
int card_argothian_enchantress(int , int, event_t);
int put_card_on_stack(int, int, int);
int put_card_on_stack2(int, int);
int put_card_on_stack3(int, int);
void apply_challenge_puzzle_upkeep(void);
void not_dying(void);
int get_protections_from(int, int);
int oubliette_effect(int, int, int, int);
int create_a_card_type(int);
int u4585f0(int, int);
int play_sound_effect(wav_t);
int u4b6860(int, int);
void display_error_message(const char*);
int set_centerwindow_txt(const char*);
int save_or_load_data(void* data, size_t size);
extern char byte_786DD4;
int dispatch_trigger_twice_once_with_each_player_as_reason(int reason_for_trig, trigger_t trig, const char *prompt, int a4);
extern char str_Draw_a_card[64];
int dispatch_event_to_single_card(int player, int card, event_t event, int new_attacking_card_controller, int new_attacking_card);
void put_card_in_graveyard(int player, int card);
void call_sub_437E20_unless_ai_is_speculating(void);
extern int dword_60A4B0;
int TENTATIVE_wait_for_network_result(int a1, signed int a2);
int TENTATIVE_send_network_result(int a1, signed int a2);
extern long dblword_628C10;
extern char byte_628C0C;

/* Our own functions */
int add_card_to_hand(int, int);
static inline int affect_me(int player, int card)	{ return card == affected_card && player == affected_card_controller; }
int boost_creature_type(int player, int card, event_t event, subtype_t subtype, int power, int toughness, int abilities, bct_t flags);
void boost_subtype(int player, int card, event_t event, subtype_t subtype, int power, int toughness, int abilities, int sp_abilities, bct_t flags);	// Doesn't work for special keywords that need to be active other than during EVENT_ABILITIES.
int call_card_fn(void* address, card_instance_t* instance, int player, int card, event_t event);	// Puts instance into esi, then calls address(player, card, event).
int call_card_function_i(card_instance_t* instance, int player, int card, event_t event);	// A frontend for call_card_fn() where address should be computed from instance.
int call_card_function(int player, int card, event_t event);	// A frontend for call_card_fn() where both address and instance should be computed from {player,card}.
int count_graveyard(int);
extern HackForceEffectChangeSource hack_force_effect_change_source;	// If an effect is created with source hack_force_effect_change_source.from, change it to be hack_force_effect_change_source.to before sending it to the exe.
int create_legacy_effect_exe(int player, int card, int iid, int t_player, int t_card);
int get_power(int, int);
int get_toughness(int, int);
card_instance_t* get_card_instance(int player, int card);
int get_cmc(int player, int card);
card_instance_t* get_displayed_card_instance(int player, int card);
int has_creature_type(int, int, subtype_t);
int has_threshold(int player);
card_instance_t* in_play(int player, int card);	// Returns get_card_instance(player, card) if in play, else NULL.
void protection_from_black(int, int, event_t);
void protection_from_red(int, int, event_t);
void protection_from_blue(int, int, event_t);
void protection_from_green(int, int, event_t);
void protection_from_white(int, int, event_t);
void unblockable(int player, int card, event_t event);

/* C functions */
int show_bottom_of_deck(int player, int* deck, int num_cards, const char* prompt, int count_in_deck);	// count_in_deck should be count_deck(player).  It can be less (to show cards from the middle of the deck, for some reason), but never more.
unsigned int num_bits_set(unsigned int v);
int select_card_from_graveyard(int, int, int, int);
void basic_upkeep(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);
void basic_upkeep_arbitrary(int triggering_player, int triggering_card, int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);	// As basic_upkeep(), but allows a different card to trigger.
int basic_upkeep_unpaid(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);	// As basic_upkeep(), but returns true if upkeep is unpaid instead of sacrificing.
int cumulative_upkeep(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);	// Returns 1 if upkeep is unpaid.
int cumulative_upkeep_arbitrary(int triggering_player, int triggering_card, int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);	// As cumulative_upkeep(), but allows a different card to trigger.
int cumulative_upkeep_hybrid(int player, int card, event_t event, int amt_colored, color_t first_color, color_t second_color, int amt_colorless);	// As cumulative_upkeep(), but charges hybrid mana.
int cumulative_upkeep_general(int player, int card, event_t event, int (*can_pay_fn)(int, int, int), int (*pay_fn)(int, int, int));	// As cumulative_upkeep(), but arbitrary functions instead of mana costs.  Both can_pay_fn() and pay_fn() should have signature int fn(int player, int card, int number_of_age_counters), and return nonzero if upkeep (can be/is) paid.
void convert_to_token(int, int);
void generate_token(token_generation_t *token);
void generate_token_by_id(int player, int card, int csvid);
int generate_reserved_token_by_id(int player, int csvid);	// Just like generate_token_by_id(), but returns the added token.  Only safe for non-cards, e.g. rules engine, deadbox, emblems, CARD_ID_SPECIAL_EFFECT.
void generate_tokens_by_id(int player, int card, int csvid, int howmany);
int comes_into_play(int player, int card, event_t event);
int comes_into_play_mode(int player, int card, event_t event, resolve_trigger_t trigger_mode);
int is_sick(int, int);
int is_animated_and_sick(int, int);

extern int counters_added;	// for XTRIGGER_1_1_COUNTERS
extern int hack_silent_counters;	// If greater than 0, don't play sound effects when adding counters - intended for non-game counters stored as counters, as on Rules Engine and Deadbox.  Be sure to reset it afterward.
void add_counters(int player, int card, counter_t type, int num);	// num must be > 0, or no effect
void add_counters_as_cost(int player, int card, counter_t type, int num);	// unaffected by doubling effects; num may be positive or negative
void add_counters_predoubled(int player, int card, counter_t type, int num);	// num must be > 0, or no effect.  Assumes caller has already called get_updated_counters_number().  This still accounts for putting counters on Melira's Keepers, but not onto effect cards.  Do not call for the latter.
counter_t choose_existing_counter_type(int who_chooses, int src_player, int src_card, int t_player, int t_card, cect_t mode, int smallcard_player, int smallcard_card);	// Returns COUNTER_invalid if cancelled, or if there are no counters.
void copy_counters(int t_player, int t_card, int src_player, int src_card, int raw);	// For each counter on {src_player, src_card}, add one of that type to {t_player, t_card}.  If raw is set, add the exact amount (even for Melira's Keepers or Melira, Sylvok Outcast, or if Doubling Season etc. are present).
int count_counters(int player, int card, counter_t type);	// use -1 to get total count
int count_counters_no_parent(int player, int card, counter_t type);	// Just like count_counters(), but if called for an activation_card, return the counters that were on th parent card as it was activated, not how many are on it now.
int count_counters_by_counter_and_card_type(int player, counter_t counter_type, type_t card_type);	// May use ANYBODY for player and/or -1 for counter_type to indicate any player's cards or any kind of counter.  Returns a total count.
void enters_the_battlefield_with_counters(int player, int card, event_t event, counter_t type, int number);
int get_counter_type_by_id(int id);
int get_updated_counters_number(int player, int card, counter_t type, int number);
int has_any_counters(int player, counter_t counter_type, type_t type);	// May use ANYBODY for player and/or -1 for counter_type to indicate any player's cards or any kind of counter.  Stops looking after the first match.
void move_counters(int t_player, int t_card, int src_player, int src_card, counter_t counter_type, int number);	// Moves number of counter_type from {src_player, src_card} to {t_player, t_card}.  Moves all counters of all types if counter_type is -1; moves all of counter_type if number is -1.
void remove_all_counters(int player, int card, counter_t type);	// use -1 to remove all of all types
void remove_counters(int player, int card, counter_t type, int num);	// num must be > 0, or no effect

static inline void add_counter(int player, int card, counter_t type)	{ add_counters(player, card, type, 1); }
static inline void remove_counter(int player, int card, counter_t type)	{ remove_counters(player, card, type, 1); }
static inline void add_1_1_counter(int player, int card)				{ add_counters(player, card, COUNTER_P1_P1, 1); }
static inline void add_1_1_counters(int player, int card, int num)		{ add_counters(player, card, COUNTER_P1_P1, num); }
static inline void remove_1_1_counter(int player, int card)				{ remove_counters(player, card, COUNTER_P1_P1, 1); }
static inline void remove_1_1_counters(int player, int card, int num)	{ remove_counters(player, card, COUNTER_P1_P1, num); }
static inline void add_minus1_minus1_counters(int player, int card, int num)	{ add_counters(player, card, COUNTER_M1_M1, num); }
static inline int count_1_1_counters(int player, int card)				{ return count_counters(player, card, COUNTER_P1_P1); }
static inline int count_minus1_minus1_counters(int player, int card)	{ return count_counters(player, card, COUNTER_M1_M1); }

int can_sorcery_be_played(int player, event_t event);
void untap_card(int player, int card);
void untap_card_no_event(int player, int card);	// Don't send EVENT_UNTAP_CARD events.  Use only if this card is untapping only as an implementation detail, e.g. it was tapped to prevent it from tapping to generate mana for its own activation cost, and the player cancelled during mana charging.
int pump_until_eot(int player, int card, int t_player, int t_card, int power, int toughness);
int pump_until_eot_merge_previous(int player, int card, int t_player, int t_card, int power, int toughness);	// If there's another pump_until_eot effect card originating from player/card attached to t_player/t_card, controlled by player, then add the power/toughness specified here to that effect and return it.  (If you've manually fiddled with the effect's other target settings, you're on your own; they're not checked for.)  Otherwise, create a new effect and return that instead.  In either case, set the effect's counter_power and counter_toughness so they can be seen by |n in the effect text - which must specifically have |n's in it or this'll be hopelessly confusing.
int pump_ability_until_eot(int player, int card, int t_player, int t_card, int power, int toughness, int ability, int sp_ability);
int pump_ability_until_eot_no_repeat(int player, int card, int t_player, int t_card, int ability, int sp_ability);	//Just like pump_ability_until_eot(), but can't pump power or toughness, and if the creature already has an effect card from {player,card} and has all the named keywords set, only gives the existing effect a new timestamp instead of making a new one.
void default_pump_ability_definition(int player, int card, pump_ability_t *pump, int pow, int tou, int key, int skey);
int pump_ability(int player, int card, int t_player, int t_card, pump_ability_t *pump);

int exalted(int, int, event_t, int, int);
int count_shrines( int, int );
int count_subtype(int, int, int);
int count_domain(int, int);
void shadow(int player, int card, event_t event);
int pick_card_from_deck(int);
int create_legacy_effect(int player, int card, int (*func_ptr)(int, int, event_t));
int create_legacy_activate(int playr, int card, int (*func_ptr)(int, int, event_t));
int create_targetted_legacy_effect(int player, int card, int (*func_ptr)(int, int, event_t), int t_player, int t_card);
int create_targetted_legacy_activate(int player, int card, int (*func_ptr)(int, int, event_t), int t_player, int t_card);
int create_targetted_legacy_effect_no_repeat(int player, int card, int (*func_ptr)(int, int, event_t), int target_player_id, int target_card);	// Just like create_targetted_legacy_effect(), but if there's an effect from the same source with the same func_ptr already affecting the card, just gives that a new timestamp instead.
int create_legacy_effect_for_opponent(int player, int card, int (*func_ptr)(int, int, event_t), int t_player, int t_card);	// Creates a legacy effect from (player,card) under control of 1-player.
int create_legacy_effect_from_iid(int player, int iid, int (*func_ptr)(int, int, event_t), int target_player, int target_card);	// Creates a sourceless effect card, with name/text/image from iid.
int find_repeat_legacy_effect(int player, int card, int (*func_ptr)(int, int, event_t), int target_player_id, int target_card);	// If there's already an effect card attached to {target_player_id,target_card} with func_ptr created by {player,card}, give it a new timestamp and return it; else return -1.
int effect_follows_control_of_attachment(int player, int card, int event);	// If this effect is controlled by a different player than the card it's attached to, then transfer control and ownership of the effect.  Calling effect should return 0 if this returns true.
int create_my_legacy(int player, int card, int (*func_ptr)(int, int, event_t));
int resolve_graveyard_trigger(int, int, event_t);
void does_not_untap(int, int, event_t);
int does_not_untap_until_im_tapped(int player, int card, int, int);
int does_not_untap_effect(int player, int card, int t_player, int t_card, int mode, int turns);
void gains_doesnt_untap_while_has_a_counter_and_remove_a_counter_at_upkeep(int src_player, int src_card, int t_player, int t_card, counter_t counter_type);
int get_id(int player, int card);
int get_backup_id(int player, int card);	// Probably unsafe to just fall back to backup_internal_card_id in get_id(); something might rely on it being -1 for cards not in play.
int get_original_id(int player, int card);
int count_cards_by_id(int player, int id);
int select_card_from_graveyard(int, int, int, int );
void echo(int, int, event_t, int, int, int, int, int, int);
void fading(int, int, event_t, int);
int count_graveyard_by_id(int, int);
int count_deck(int);
int count_graveyard(int);
int count_rfg(int);
void purge_rfg(int);
int count_graveyard_by_type(int player, int type);
int any_in_graveyard_by_type(int player, int type);	// Equivalent to count_graveyard_by_type(player, type) > 0, but stops when it finds the first qualifying card.
card_data_t* get_card_data(int, int);
card_instance_t* has_activation_on_stack(int player, int card);	// Returns the first activation card currently on the stack controlled by player that was activated for {player,card}, or NULL if not found.
int has_mana_multi(int player, int colorless, int black, int blue, int green, int red, int white);
int has_mana_multi_a(int player, int colorless_or_artifact, int black, int blue, int green, int red, int white);
int charge_mana_multi(int player, int colorless, int black, int blue, int green, int red, int white);
int charge_mana_multi_a(int player, int colorless, int black, int blue, int green, int red, int white, int artifact);
// These variants should be called instead of charge_mana()/charge_mana_multi() during EVENT_RESOLVE_TRIGGER, EVENT_RESOLVE_SPELL, and RESOLVE_ACTIVATION, so the prompt is correct.
int charge_mana_while_resolving(int player, int card, event_t event, int charged_player, color_t color, int amount);
int charge_mana_while_resolving_csvid(int csvid, event_t event, int charged_player, color_t color, int amount);
int charge_mana_multi_while_resolving(int player, int card, event_t event, int charged_player, int colorless, int black, int blue, int green, int red, int white);
int charge_mana_multi_while_resolving_csvid(int csvid, event_t event, int charged_player, int colorless, int black, int blue, int green, int red, int white);
int charge_mana_hybrid_while_resolving(int player, int card, event_t event, int amt_colored, color_t first_color, color_t second_color, int amt_colorless);
int charge_mana_for_activated_ability_while_resolving(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);	// An unusual case - the card is worded to be an activated, not a triggered ability, but we're charging for it during resolution.
int is_basic_land(int, int);
int is_basic_land_by_id(int);
int eot_trigger(int, int, event_t);
int eot_trigger_mode(int player, int card, event_t event, int t_player, resolve_trigger_t trig_mode);
int attack_if_able(int, int, event_t);
void all_must_attack_if_able(int player, event_t event, int subtype);
int has_subtype(int, int, subtype_t);
int put_top_card_of_deck_to_bottom(int player);	// Moves the card on top of player's library to the bottom, and returns its internal_card_id (or -1 if library is empty).  Equivalent to put_card_in_deck_to_bottom(player, 0).
int put_card_in_deck_to_bottom(int player, int position);	// Moves the card at deck_ptr[player][position] to the bottom of his library, and returns its internal_card_id (or -1 if position is higher than the number of cards in the library).
void obliterate_top_card_of_grave(int);
int get_random_card(void);
int cycling(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);
int cycling_hybrid(int player, int card, event_t event, int amt_colored, color_t first_color, color_t second_color, int amt_colorless);
int cycling_special_cost(int player, int card, event_t event, int mode);	// Keeps the cycling abilities with odd activation costs in close proximity to normal cycling implementation.
int landcycling(int player, int card, event_t event, int colorless, int land_subtype1, int land_subtype2);
void comes_into_play_tapped(int, int, event_t);
void play_card_in_hand_for_free(int player, int card);
void opponent_plays_card_in_your_hand_for_free(int player, int card);
int play_card_in_deck_for_free(int player, int t_player, int deck_pos);
int play_card_in_grave_for_free(int player, int t_player, int g_pos);
int play_card_in_grave_for_free_and_exile_it(int player, int t_player, int g_pos);
int play_card_in_exile_for_free(int player, int t_player, int csvid);
int play_spell_for_free(int player, int csvid);
int leaves_play(int player, int card, event_t event);	// Lets a card watch for itself leaving play.
int other_leaves_play(int triggering_player, int triggering_card, int t_player, int t_card, event_t event);	// As leaves_play(), but allows a card ({triggering_player,triggering_card}) to watch for a different card ({t_player,t_card}) leaving play, not itself.
int choose_a_color(int player, int ai_choice);	// Force a color choice.
int choose_a_color_and_show_legacy(int player, int card, int ai_t_player, int ret_location);
int choose_a_land(int player, int ai_choice);	// Force a basic land type choice.
int choose_a_color_from(int player, int ai_choice, color_test_t available_colors);	// Force a color choice from this bitfield of available colors.
void mill(int target_player, int amount);
void special_mill(int player, int card, int csvid, int target, int amount);
int count_permanents_by_type(int player, type_t type);
int count_deck_by_type(int player, int type);
int can_legally_play_iid(int player, int internal_card_id);	// Can a card with this iid be played (other than mana cost and timing restrictions)?
int can_legally_play_iid_now(int player, int internal_card_id, event_t event);	// Can a card with this iid be played (other than mana cost)?
int can_legally_play_csvid(int player, int csvid);	// Can a card with this csvid be played (other than mana cost and timing restrictions)?
int can_legally_play_csvid_now(int player, int csvid, event_t event);	// Can a card with this csvid be played (other than mana cost)?
int rfg_top_card_of_deck(int player);	// Exile the top card of player's library.
int generic_wish(int, int, event_t, int, int);
void generic_wish_effect(int player, int card, int type, int ai_card);
int was_card_in_hand(int, int);
int evoke(int, int, event_t, int, int, int, int, int, int);
int get_setting(int);
void read_settings(void);
void set_opponent_deck(int);
void shuffle_vanguard(int, int);
void challenge_mode_upkeep(int);
int get_challenge_round(void);
int is_unlocked(int, int, event_t, int);
void apply_challenge_vanguard(void);
void set_challenge_round(void);
void apply_challenge_onslaught(void);
int is_valid_card(int);
int get_challenge2(void);
int get_challenge3(void);
int get_challenge4(void);
int skip_your_draw_step(int, event_t);
card_instance_t* in_hand(int player, int card);
int get_turn_count(void);
int get_specific_turn_count(int player);
event_t get_actual_event(void);
int bridge_from_below(int, int, event_t );
int get_flashback(void);
int fetchland(int player, int card, event_t event, subtype_t subtype1, subtype_t subtype2, int lifecost);
int planeswalker(int, int, event_t, int);
int card_exploration(int, int, event_t );
int card_dissipate(int , int, event_t);
int basic_equipment(int, int, event_t, int);
int get_sum(int, type_t);
int get_hand_sum(int, type_t);
int gain_control(int, int, int, int);
int switch_control(int, int, int, int);
int give_control(int effect_source_player, int effect_source_card, int tar_player, int tar_card);
int give_control_until_eot(int effect_source_player, int effect_source_card, int tar_player, int tar_card);
void give_control_of_self(int player, int card);
int get_cmc_by_id(int);
int landfall(int player, int card, event_t event);
int landfall_mode(int player, int card, event_t event, resolve_trigger_t trigger_mode);
int get_momir(void);
void set_challenge_round_to_0(void);
const char* get_card_name_by_id(int csvid);
const char* get_card_name_by_instance(card_instance_t* instance);
int reveal_top_card(int player, int card, event_t event);
int create_card_name_legacy(int player, int card, int id);
int create_subtype_name_legacy(int player, int card, int subtype);
int create_time_walk_legacy(int player, int card);
int get_deck_color(int player, int whose_deck);	// If player == whose_deck or player is an AI (actual AI, or AI speculating for a human player), then returns a color_t of the most common color in whose_deck's library.  If player is a human and whose_deck isn't his, then always returns COLOR_BLACK.  If player is -1, then returns a color_test_t of the two most common colors in whose_deck's library.
void obliterate_card(int player, int card);
void obliterate_card_and_recycle(int player, int card);	// Obliterates card and immediately marks its slot as suitable for recycling.  Dangerous.  Should only be used if he card has never been visible to other cards and add_card_to_hand() has not been called again after this card was created.
void minimum_blockers(int player, int card, int event, int min);
int indes(int player, int card, event_t event);
int upkeep_trigger(int player, int card, event_t event);
int upkeep_trigger_mode(int player, int card, event_t event, resolve_trigger_t trigger_mode);
int global_enchantment(int player, int card, event_t event);
int get_color(int player, int card);
int get_color_by_internal_id(int player, int iid);
int get_color_by_internal_id_no_hack(int iid);
int get_color_real_or_iid(int player_or_owner, int player_or_negone, int card_or_iid);	// A shim between get_color() and get_color_by_internal_id().
void cascade(int player, int card, event_t event, int override);
void do_cascade(int player, int card, int);
int card_eureka(int, int, event_t);
int unearth(int player, event_t event, int colorless, int black, int blue, int green, int red, int white);
void devour(int player, int card, event_t event, int number);
int paying_mana(void);
int krovikan_horror(int, int, event_t);
void scrylike_effect(int player_choosing, int player_whose_library_is_seen, int number_of_cards);
void scry(int player, int number_of_cards);	// Only for cards that actually say "Scry"
void rearrange_top_x(int t_player, int who_chooses, int number);
void rearrange_bottom_x(int t_player, int who_chooses, int number);
int reanimate_permanent(int player, int card, int target_graveyard, int selected, reanimate_mode_t action);
int reanimate_permanent_with_effect(int player, int card, int target_graveyard, int selected, reanimate_mode_t action, int (*effect_fn)(int, int, event_t));	// Just like reanimate_permanent(), but attach an effect with function effect_fn to the animated permanent before putting it into play.
int reanimate_permanent_with_counter_and_effect(int player, int card, int target_graveyard, int selected, reanimate_mode_t action, counter_t counter_type, int (*effect_fn)(int, int, event_t));	// Just like reanimate_permanent_with_effect(), but also add a counter to it before putting it into play.
int reanimate_permanent_with_function(int player, int card, int target_graveyard, int selected, reanimate_mode_t action, void (*before_etb)(int, int));	// Just like reanimate_permanent(), but call an artibtrary function on {player,card} just before it's put on the battlefield.
int count_graveyard_by_subtype(int player, subtype_t subtype);
int any_in_graveyard_by_subtype(int player, subtype_t subtype);
void increase_trap_condition(int player, int, int);
int get_trap_condition(int player, int);
void set_trap_condition(int, int, int);
void tutor_lands(int player, tutor_t destination, int num);
void tutor_basic_land(int player, int put_in_play, int tap_it);
void tutor_basic_lands(int player, tutor_t destination, int num);
void cultivate(int player);	// Tutor two basic lands, one onto bf tapped, one into hand, then shuffle
int get_hacked_color(int player, int card, int);
int hybrid(int player, int card, event_t event);
void double_faced_card(int player, int card, event_t event);
void transform(int player, int card);
void turn_face_down(int player, int card);	// Beware, this only works for cards that originally had morph, cards with an ixidron_legacy attached, and cards that call can_exist_while_face_down().
int morph(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);
int casting_permanent_with_morph(int player, int card, event_t event);
int can_exist_while_face_down(int player, int card, event_t event);	// A card without morph that has a turn-self-face-down ability.
int manland_animated(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);
int manland_normal(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);
int animate_self(int player, int card, int pow, int tgh, keyword_t kws_to_add, sp_keyword_t sp_kws_to_add, color_test_t color, int permanent);	// Animates {player,card} by adding TYPE_CREATURE, with characteristic power, toughness, and keywords; continuously-added sp_keywords; and continuously-overriden color (unless color is 0).  Lasts until end of turn unless permanent is nonzero.  The animated card will have token_status & STATUS_LEGACY_TYPECHANGE set while animated by this effect.  Returns a legacy effect index, but you shouldn't do anything with it except maybe give it alternate text.
int animate_other(int src_player, int src_card, int tgt_player, int tgt_card, int pow, int tgh, keyword_t kws_to_add, sp_keyword_t sp_kws_to_add, color_test_t color, int permanent);	// As per animate_self(), but works on a different card than the effect source and doesn't set STATUS_LEGACY_TYPECHANGE.
void has_subtype_if_animated_self(int player, int card, event_t event, subtype_t subt);	// {player,card} has an additional subtype subt if it animated itself via animate_self().
void has_subtypes_if_animated_self(int player, int card, event_t event, subtype_t subt1, subtype_t subt2);	// {player,card} has two additional subtypes subt1 and subt2 if it animated itself via animate_self().
int rfg_card_in_hand(int player, int card);	// Exile an arbitrary player/card.  Works for cards on the stack, in play, or oublietted, not just in hand, despite the name.
int rfg_card_in_deck(int player, int pos);	// Exile the card at position pos in player's library.
int rfg_card_from_grave(int player, int position);	// Exile the card at position grave_id in player's graveyard.
int rfg_whole_library(int player);	// Exile player's entire library.
int rfg_whole_graveyard(int player);	// Exile player's entire graveyard.
void rfg_top_n_cards_of_deck(int player, int number);	// Exile the top number cards of player's library.
int add_card_to_rfg(int player, int iid);	// Add a card with internal_card_id iid to player's exile zone.  Does not remove it from anywhere else first.  Returns its position in rfg_ptr[player].
int add_csvid_to_rfg(int player, int csvid);	// As add_card_to_rfg(), but converts the csvid to an iid first.
int remove_card_from_rfg(int player, int csvid);	// Obliterates the highest-positioned card in player's exile zone with csvid.  Returns 1 if not found, else 0.
void remove_card_from_rfg_by_position(int player, int count);	// Obliterates the card at position count in player's exile zone.
int find_iid_in_rfg(int player, int iid);	// Return the highest position in player's exile zone with internal_card_id iid, or -1 if not found.
int check_rfg(int player, int csvid);	// Returns 1 if a card with csvid is in player's exile zone.
int obliterate_top_card_of_deck(int player);	// Remove the top card of player's library, without putting it anywhere else
void obliterate_top_n_cards_of_deck(int player, int number);	// Remove the top number cards of player's library, without putting them anywhere else
static inline void obliterate_card_in_grave(int player, int position){remove_card_from_grave(player, position);}	// The primary function the exe uses to remove a card from a player's graveyard.  Does not put the card anywhere else.
int mana_producer(int, int, event_t);
int mana_producer_tapped(int, int, event_t);
int is_equipment(int player, int card );
int graveyard_from_play(int player, int card, event_t event);
int duh_mode(int player);
int cannot_be_countered(int player, int card, event_t event);
int get_discard_controller(void);
int is_colorless(int player, int card);
int is_target(int player, int card, int controller_of_effect);
void hexproof(int player, int card, event_t event);
void tax_attack(int player, int card, event_t event);
int skip_your_draw_step(int player, event_t event);
void activate_astral_slide(void);
int get_cmc_by_internal_id(int id);
int get_attack_power(int player, int card);
int metalcraft(int player, int card);
int dealt_damage_to_player(int player, int card, int parent_player, int parent_card, int event);
//int autoselect_target(int, int , int, int ,int , int , int , int ,
//    int , int , int , int ,int ,int ,int ,int ,int ,int ,int,int ,int ,int );
int pump_subtype_until_eot(int player, int card, int, int, int, int, int, int);
void pump_color_until_eot(int player, int card, int, int, int, int, int, int);
int from_graveyard_to_deck(int player, int, int);
void rampage(int, int, int, int);
void damage_all(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
type_t get_type(int player, int card);	// Deals properly with flash permanents, enchanted evening, and planeswalkers.  In particular, this will return TARGET_TYPE_PLANESWALKER instead of TYPE_ENCHANTMENT if appropriate; make sure whatever you use it for can deal with that.
type_t get_type_with_iid(int player, int card, int iid);	// Just like get_type(), but the iid is already known (and possibly may not be easily determinable from just player/card, for instance if it's left play)
int is_what(int player, int card, int test_type);
int select_a_color(int player);
void discard_all(int);
void persist(int player, int card, event_t event);
int count_graveyard_by_color(int player, int);
int count_permanents_by_color(int player, int type, int colortest);
int is_token(int player, int card);
const char* get_sleighted_color_text(int player, int card, const char* fmt, int orig_color_t);	// fmt should have exactly one %s in it to receive the color text.  Returns a pointer to a static buffer.
const char* get_sleighted_color_text2(int player, int card, const char* fmt, int orig_color_t_1, int orig_color_t_2);	// As get_sleighted_color_text(), but fmt should have exactly two %s in it.
int get_sleighted_color_test(int player, int card, int orig_color_test);
int get_sleighted_protection(int player, int card, int orig_prot);
const char* get_hacked_land_text(int player, int card, const char* fmt, /*original subtypes*/...);	// fmt should have exactly one %s, %a, or %l in it to receive the land text for each subtype provided as a variable argument.  (%a will insert a "a " or "an " before the land text as approriate; %l will make unhacked land text all lowercase, appropriate for e.g. a landwalk ability rather than a type name.)  Returns a pointer to a static buffer.  Each variable argument can be a SUBTYPE_*, HARDCODED_SUBTYPE_*, or COLOR_*.
int get_hacked_walk(int player, int card, int orig_walk);
int get_hacked_subtype(int player, int card, int);
int get_storm_count(void);
int get_specific_storm_count(int player);
int get_stormcreature_count(int);
int ally_trigger(int player, int card, event_t event, int trigger_mode /*accepts RESOLVE_TRIGGER_DUH*/);
subtype_t from_hardcodedsubtype_to_subtype(hardcoded_subtype_t hcs);	// Only needed for engine.c:effect_asterisk(), field ASTERISK_SUBTYPE_OF_INFO_SLOT.  I'm reasonably certain that only gets called from card_swarm_of_rats() at 0x4140f0, for HARDCODED_SUBTYPE_RAT; that happens to have the same value as SUBTYPE_RAT.  But no reason to risk it.
int has_subtype_by_id(int, subtype_t subtype);
int pick_card_from_graveyard(int player, int, const char *);
void proliferate(int player, int card);
int changeling_switcher(int player, int card, event_t event);
int count_subtype_in_hand(int player, subtype_t);
int count_untapped_subtype(int, int, int);
int specific_spell_played(int player, int card, event_t event, int, int, int, int, int, int, int, int, int, int, int, int);
int specific_cip(int player, int card, event_t event, int, int, int, int, int, int, int, int, int, int, int, int);
void obliviation(int player, int card, int targ_player, int targ_card);
void return_from_oblivion(int player, int card, event_t event);
int metamorphosis(int player, int card, type_t type_to_search_for, kill_t kill_code);
int ninjutsu(int player, int card, event_t event, int, int, int, int, int, int, int);
int has_combat_damage_been_inflicted_to_opponent(int player, int card, event_t event);
int has_combat_damage_been_inflicted_to_a_player(int player, int card, event_t event);
int count_snow_permanents(int what_player, int type, int must_be_untapped);
int is_subtype_in_hand(int player, int);
int living_weapon(int player, int card, event_t event, int);
int shapeshift_target(int src_player, int src_card, int t_player, int t_card, int turn_into_player, int turn_into_card, int mode);	// Shapeshifts a single permanent.  mode is a shapeshift_t.
void shapeshift_all(int src_player, int src_card, int t_player, test_definition_t* test, int turn_into_player, int turn_into_card, int mode);	// Shapeshifts all cards matching test and controlled by t_player (or ANYBODY).
int impulse_effect(int player, int, int);
void lifelink(int player, int card, event_t event);
int is_snow_permanent(int player, int card);
int count_snow_permanents(int, int, int);
int count_free_mana(int player, int);
int is_attacking(int player, int card);
card_instance_t* in_play_and_attacking(int player, int card);
int count_attackers(int player);
int count_attackers_non_planeswalker(int player);
int has_mana_hybrid(int player, int, int, int, int);
int charge_mana_hybrid(int player, int card, int, int, int, int);
int has_snow_mana(int player, int, int);
int charge_snow_mana(int player, int card, int, int);
int al_cantrip(int player, int card, event_t event);
int select_a_protection(int player);
int sacrifice_at_end_of_combat(int player, int card, event_t event);
int remove_at_eot(int player, int card, event_t event);
int haste_and_remove_eot(int player, int card, event_t event);
void flanking(int player, int card, event_t event);
void granted_flanking(int s_player, int s_card, int t_player, int t_card, event_t event);
int phase_out(int player, int card);
int phasing(int player, int card, event_t event);
void untap_phasing(int player, int shimmering_lands);
int no_combat_damage_this_turn(int player, int card, event_t event);
int empty(int player, int card, event_t event);	// A permanent legacy effect that does nothing.
int empty_until_eot(int player, int card, event_t event);	// A legacy effect that does nothing except get removed at end of turn.
int total_playable_lands(int player);
void check_playable_lands(int player);
int check_playable_lands_legacy(int player, int card, event_t event);
void damage_effects(int player, int card, event_t event);
int generic_painland(int player, int card, event_t event);
void controller_sacrifices_a_creature(int player, int card);
void player_sacrifices_a_creature(int s_player, int s_card, int t_player);
int vineyard_effect(int player, int card, event_t event);
int get_dead_count(int player, int type);
void reanimate_all_dead_this_turn(int player, int);
int effect_cannot_attack(int player, int card, event_t event);	// Also suitable as part of an enchantment
int effect_cannot_attack_until_eot(int player, int card, event_t event);
int effect_defender_can_attack_until_eot(int player, int card, event_t event);
int effect_lose_defender_gain_flying_until_eot(int player, int card, event_t event);
int is_enchanted(int player, int card);
int cannot_attack(int player, int card, event_t event);
int generic_spike(int player, int card, event_t event, int);
int count_nonbasic_lands(int player);
int control_nonbasic_land(int player);
int player_reveals_x_and_discard(int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
void reshuffle_all_into_deck(int player, int);
int has_phyrexian_mana(int player, int cless, int black, int blue, int green, int red, int white, int clred_artifact);
int charge_phyrexian_mana(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white, int clred_artifact);
void lobotomy_effect(int player, int t_player, int csvid, int must_find_all);
int is_mana_producer_land(int player, int card);
int is_planeswalker(int player, int card);
int check_battlefield_for_subtype(int player, int type, int subtype);
int recycling_land(int player, int card, event_t event, int, int, int, int, int, int, int);
void make_all_untargettable(void);
void remove_untargettable_from_all(void);
int count_untapped_nonsick_subtype(int, int, int);
int check_for_untapped_nonsick_subtype(int, int, int);
void return_all_dead_this_turn_to_hand(int player, int);
int prevent_damage_sacrificing_a_creature(int player, int card, event_t event, int);
int haste_and_sacrifice_eot(int player, int card, event_t event );
void prevent_the_next_n_damage(int player, int card, int t_player, int t_card, int amount, prevent_t flags, int redir_to_player, int redir_to_card);
void reveal_target_player_hand(int player);
int get_most_common_cmc_nonland(int player);
int get_most_common_cmc_in_hand(int player, type_t type);
int get_highest_cmc(int player, type_t type);
int get_highest_cmc_nonland(int player);
int symbiotic_creature(int player, int card, event_t event, token_generation_t *token);
void put_on_bottom_of_deck(int player, int card);
int effect_act_of_treason(int player, int card, int, int);
int global_tutor(int player, int targ_player, int search_location, int destination, int must_select, int ai_selection_mode, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5);
int select_a_card(int player, int, int, int, int, int, int, int, int, int, int, int, int, int, int, int);
int select_a_subtype(int player, int card);
int get_updated_casting_cost(int player, int card, int iid, event_t event, int cless);	// exactly one of card and iid must be >= 0.  Player must always be valid.
int true_get_updated_casting_cost(int player, int card, int iid, event_t event, int cless);
int rfg_target_permanent(int player, int card);
void state_untargettable(int player, int card, int);
int check_for_ability(int player, int card, int);
int check_for_special_ability(int player, int card, int);
extern is_protected_from_mode_t protection_mode;
int is_protected_from(int protected_player, int protected_card, int from_player, int from_card, is_protected_from_mode_t new_protection_mode);
int is_protected_from_me(int player, int card, int, int);	// deprecated
int pick_targets_for_multidamage(int player, int card, int, int, target_definition_t *td, const char *prompt);
void multidamage(int player, int card, int, int, target_definition_t *td);
void nobody_can_attack(int player, int card, event_t event, int t_player);
void nobody_can_attack_until_eot(int player, int card, int t_player, int except_player, int except_card);	// Only except_player/except_card, and creatures not controlled by t_player, can attack this turn.  t_player can be 2 for anybody, and except_player/except_card can be -1 to not provide an exception.
void nobody_can_attack_until_your_next_turn(int player, int card, int t_player);
void target_player_skips_his_next_attack_step(int player, int card, int t_player, int this_turn_only);
int count_planeswalkers(int player, int);
int get_special_infos(int player, int card);
int is_legendary(int player, int card);
int fog_effect(int player, int card);
int fog_special(int player, int card, int t_player, int flags);	// flags is a fog_special_flags_t
// void agyrem_eot_effect(int player);
void blink_effect(int player, int card, void (*before_etb)(int player, int card));	// Exiles {t_player,t_card} and then returns it to the battlefield under its owner's control.  If before_etb is non-null, call that function with the card just before it re-enters the battlefield.
void pandemonium_effect(int player, int card);
void popup(const char* title, const char* fmt, ...);
extern char always_prompt_for_color;	// If set to nonzero, then the color for a mana sources that can tap for more than one kind of color is always prompted for.
extern color_test_t chosen_colors;	// Color or colors activately chosen to tap for.  Won't be set if the color is overridden by e.g. Contamination.  Only set for some of the functions in produce_mana.c; inspect them before inspecting this.
void produce_mana_multi(int player, int, int, int, int, int, int);
void produce_mana_tapped(int player, int card, color_t color, int amount);	// Produces the specified color/amount of mana (modified by Contamination if the card is a land) and taps card.  Only produces mana if card is < 0, though it'll still set tapped_for_mana_color, so you probably don't want to do this.
void produce_mana_tapped2(int player, int card, color_t color1, int amount1, color_t color2, int amount2);	// As produce_mana_tapped(), but for two colors of mana.
void produce_mana_tapped_multi(int player, int card, int colorless, int black, int blue, int green, int red, int white);	// As produce_mana_tapped, but for arbitrary amounts of colored/unrestricted-colorless mana.
int produce_mana_tapped_all_one_color(int player, int card, color_test_t available, int num_mana);	// As produce_mana_tapped(), but num_mana all of the same color (chosen from those set in available).  Cancellable.
int produce_mana_tapped_all_one_color_with_default(int player, int card, color_test_t available, int num_mana, color_test_t default_color);	// As produce_mana_tapped_all_one_color(), but if no specific color is needed and default_color is produceable, produce that.  Cancellable.
int produce_mana_all_one_color(int player, color_test_t available, int num_mana);	// As produce_mana_tapped_x_all_one_color(), but doesn't tap card, account for Contamination, or set tapped_for_mana_color.  Cancellable.
int produce_mana_tapped_any_combination_of_colors(int player, int card, color_test_t available, int num_mana, const char* prompt);	// As produce_mana_tapped(), but num_mana of any combination of the colors set in available.  If prompt is NULL, one will be constructed on-the-fly for each choose_a_color dialog, showing amount of mana left and colors chosen so far.  Cancellable.
int produce_mana_any_combination_of_colors(int player, color_test_t available, int num_mana, const char* prompt);	// As produce_mana_tapped_any_combination_of_colors(), but doesn't tap card, account for Contamination, or set tapped_for_mana_color.  Cancellable.
void produce_mana_of_any_type_tapped_for(int player, int card, int amount);	// Produces amount of mana in any combination of the types put into tapped_for_mana_color/tapped_for_mana[].  This is the back-end function of Mana Flare and similar effects.
void produce_mana_when_subtype_is_tapped(int allowed_player, event_t event, int type, int subtype, color_t color, int amount);	// Produces amount of color mana when allowed_player taps a card of type and subtype for mana.  allowed_player may be 2 to work for both players; as usual, subtype may be 0 or -1 to ignore subtype.  This is the back-end function of High Tide and similar effects.
int single_color_test_bit_to_color(color_test_t col);	// Converts from a color_test_t with exactly one bit set to a color_t.
int wild_growth_aura(int player, int card, event_t event, int subtype, color_t color, int amount);	// An enchant land that produces a specific amount of color of mana when enchanted land is tapped for mana.
int wild_growth_aura_all_one_color(int player, int card, event_t event, int subtype, color_test_t available, int num_mana);	// As wild_growth_aura(), but produces num_mana of any one color set in available.
int wild_growth_aura_any_combination_of_colors(int player, int card, event_t event, int subtype, color_test_t available, int num_mana);	// As wild_growth_aura(), but produces num_mana of any combination of colors set in available.
int mana_producing_creature(int player, int card, event_t event, int reluctance_to_fight, color_t color, int amount);	// A generic creature that taps to produce mana, all of a single constant color.  If reluctance_to_fight is nonzero, it will tend not to attack or block if its controller controls few lands of matching color.  Higher numbers are more reluctant; Llanowar Elves and Birds of Paradise use 24.
int mana_producing_creature_multi(int player, int card, event_t event, int reluctance_to_fight, int colorless, int black, int blue, int green, int red, int white);	// As mana_producing_creature(), but produces multiple colors.  reluctance_to_fight goes by total number of lands.
int mana_producing_creature_all_one_color(int player, int card, event_t event, int reluctance_to_fight, color_test_t available, int num_mana);	// As mana_producing_creature(), but produces num_mana all of a single color in available.  reluctance_to_fight goes by total number of lands.
int two_mana_land(int player, int card, event_t event, color_t color1, color_t color2);
int sac_land(int player, int card, event_t event, int base_color, int color1, int color2);
int sac_land_tapped(int player, int card, event_t event, int base_color, int color1, int color2);
int artifact_mana_all_one_color(int player, int card, event_t event, int amount, int sac);	// A generic artifact that taps to produce mana of its mana_color.  If sac is nonzero, then sacrifices itself when activated.
int permanents_you_control_can_tap_for_mana(int player, int card, event_t event, type_t type, int32_t subtype, color_t color, int amount);	// Implements "X you control have 'T: Add Y to your mana pool.'", where Y is a specific color and amount of mana.  Set subtype to -1 if just checking for a type, as usual. (Example: Sachi, Daughter of Seshiro - X is Shamans, and Y is |G|G.)
int permanents_you_control_can_tap_for_mana_all_one_color(int player, int card, event_t event, type_t type, int32_t subtype, color_test_t available, int num_mana);	// As permanents_you_control_can_tap_for_mana(), but choice of colors set in available.  (Example: Overlaid Terrain - Lands, and 2 of COLOR_TEST_ANY_COLORED.)
int tap_a_permanent_you_control_for_mana(int player, int card, event_t event, type_t type, int32_t subtype, color_t color, int amount);	// Implements "Tap an untapped X you control: Add Y to your mana pool.", where Y is a specific color and amount of mana. (Example: Seton, Krosan Protector - X is Druid and Y is |G.)  Differs from permanents_you_control_can_tap_for_mana() in that 1. this is correct, not an approximation; and 2. it can tap summoning-sick creatures.
int get_colors_of_mana_land_could_produce_ignoring_costs(int player, int card);
int get_color_of_mana_produced_by_id(int id, int info_slot, int player);
int all_lands_are_basiclandtype(int player, int card, int event, int whose_lands, int land_color, int land_subtype);	// Stores last-hacked color_test in info_slot.  No avoiding it; it's needed for get_color_of_mana_produced_by_id(), which doesn't get a card ref.
int put_into_play_a_card_from_deck(int player, int, int);
int cursed_permanent(int player, int card, event_t event, int dmg, target_definition_t *td, const char *prompt);
int draw_some_cards_if_you_want(int player, int card, int who_draws, int howmany);
int draw_up_to_n_cards(int player, int maximum);
int vanguard_card(int player, int card, event_t event, int, int, int);
int get_new_plane(int);
void planeswalk_to(int player, int, int);
void equip_target_creature(int equipment_controller, int equipment_card, int t_player, int t_card);
int has_my_colors(int player, int card, int, int);
void intimidate(int player, int card, event_t event);
void force_a_subtype(int player, int card, subtype_t subtype);
void add_a_subtype(int player, int card, subtype_t subtype);
int has_forced_subtype(int player, int card);
int has_added_subtype(int player, int card);
void reset_subtypes(int player, int card, int);
int get_added_subtype(int player, int card);
void negate_combat_damage_this_turn(int player, int card, int t_player, int t_card, int unused);
int is_equipping(int player, int card);
int check_for_equipment_targets(int player, int card);
void play_land_sound_effect(int player, int card);
void play_land_sound_effect_force_color(int player, int card, int colors);
int m10_lands(int player, int card, event_t event, int, int);
int boost_creature_by_color(int player, int card, event_t event, int, int, int, int, bct_t flags);
void modify_pt_and_abilities(int player, int card, event_t event, int, int, int);
void nice_creature_to_sacrifice(int player, int card);
int can_use_activated_abilities(int player, int card);
int can_produce_mana(int player, int card);
int is_humiliated(int player, int card);
int is_humiliated_by_instance(card_instance_t* instance);
void disable_nonmana_activated_abilities(int player, int card, int mode);
void disable_all_activated_abilities(int player, int card, int mode);
int humiliate(int src_player, int src_card, int t_player, int t_card, int mode);
int humiliate_and_set_pt_abilities(int player, int card, int t_player, int t_card, int mode, test_definition_t *hc);
int is_nice_creature_to_sacrifice(int player, int card);
int pick_creature_for_sacrifice(int player, int card, int);
void protection_from_subtype(int player, int card, event_t event, int);
int effect_unearth(int player, int card, event_t event);
int played_for_free(int player, int card);
void phantom_effect(int player, int card, event_t event, int requires_counter);
int is_tribal(int player, int card);
int copy_spell(int player, int);
int verify_legend_rule(int player, int card, int);
int true_verify_legend_rule(int player, int card, int);
int put_into_play(int player, int card);
void wither(int, int, event_t);
int shares_creature_subtype(int p0, int c0, int p1, int c1);	// Returns true if {p0,c0} shares at least one creature subtype with {p1,c1}.
int deathtouch_until_eot(int player, int card, event_t event);
void effect_fof(int player, int who_chooses, int number, int put_rest_where);	// 1-who_chooses separates the top number cards of player's deck into two piles.  who_chooses picks a pile to put in player's hand.  The rest are put in put_rest_where, which must currently be TUTOR_GRAVE or TUTOR_BOTTOM_OF_DECK.
int vanilla_equipment(int player, int card, event_t event, int equip_cost, int p_plus, int t_plus, int k_plus, int k_special);
int special_count_grave(int player, int, int, int, int, int, int, int, int, int, int);
void change_color(int player, int card, int t_player, int t_card, color_test_t new_color, change_color_t mode);
void give_haste(int player, int card);
void put_top_x_on_bottom(int player, int t_player, int remaining);
void put_top_x_on_bottom_in_random_order(int player, int amount);
int generic_aura(int player, int card, event_t event, int pref_controller, int power, int toughness, int abilities, int sp_abilities, int subtype, int color, int forbid);
int targeted_aura(int player, int card, event_t event, target_definition_t* td, const char* prompt);	// An aura that does absolutely nothing itself.
int targeted_aura_custom_prompt(int player, int card, event_t event, target_definition_t* td, const char* prompt);
int disabling_targeted_aura(int player, int card, event_t event, target_definition_t* td, const char* prompt, int mode);
int vanilla_aura(int player, int card, event_t event, int pref_controller);	// An aura with enchant creature that does absolutely nothing itself.
int disabling_aura(int player, int card, int event);	// A frontend to vanilla_aura() with AI settings for Pacifism-like auras.
int select_one_and_mill_the_rest(int player, int, int, int);
int select_one_and_put_the_rest_on_bottom(int player, int, int);
int remove_until_eot(int player, int card, int t_player, int t_card);
int regeneration(int player, int card, event_t event, int, int, int, int, int, int);
int equipments_attached_to_me(int player, int card, int mode);	// mode is an equipments_attached_to_me_mode_t
int reshuffle_hand_into_deck(int player, int);
int reshuffle_grave_into_deck(int player, int);
void negate_damage_this_turn(int player, int card, int, int, int);
void recalculate_rules_engine_and_deadbox(void);
int get_deadbox_card(int player);
int cip_damage_creature(int player, int card, event_t event, target_definition_t *td1, const char *prompt, int amount);
int get_base_value(int player, int card);
int target_and_divide_damage(int player, int card, target_definition_t* td /* if NULL, then defaults to any creature or player */, const char* prompt /* if NULL, defaults to "TARGET_CREATURE_OR_PLAYER" */, int total_damage);	// Selects total_damage targets, then deals 1 damage to each for each time it was targeted.  Returns number of different targets selected.
int divide_damage(int player, int card, target_definition_t* td /*optional*/);	// Deals 1 damage to each of {player,card}'s targets for each time it was damaged.  If td is non-null, validates.  Returns number of different valid targets.
int check_playable_permanents(int player, int, int);
int generic_shade(int player, int card, event_t event, int life_to_pay, int cless, int black, int blue, int green, int red, int white, int p_pump, int t_pump, int k_pump, int sp_key_pump);	// If p_pump >= 100, adds p_pump-100 +1/+1 counters, doesn't add power, and ignores t_pump.
int generic_shade_merge_pt(int player, int card, event_t event, int life_to_pay, int cless, int black, int blue, int green, int red, int white, int p_pump, int t_pump);	// Just like generic_shade(), but can't pump abilities or merge counters, and sends its effect through pump_until_eot_merge_previous() so they combine into a single effect card.  Requires that the card's effect text uses |n substitutions.
int generic_shade_amt_can_pump(int player, int card, int pump, int life_to_pay, int cless, int black, int blue, int green, int red, int white, int limit);	// Returns the number of times {player,card} can activate for the given life and mana cost, multiplied by pump.  If limit is >= 0, it's an additional constraint on the maximum number of times it can activate (e.g., to deal with a cost involving a sacrifice).
int generic_shade_tap(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white, int power, int toughness);	// Like generic_shade, but includes T in activation cost.  Only supports modifications to power or toughness, due to the limited card pool.
int generic_husk(int player, int card, event_t event, int type, int p_pump, int t_pump, int k_pump, int sp_key_pump);	// If p_pump >= 100, adds p_pump-100 +1/+1 counters, doesn't add power, and ignores t_pump.
int color_to_color_test(int);
int keyword_to_color(int);
void check_for_burning_vengence(int player);
int altar_equipment(int player, int card, event_t event, int, int, int, int);
int check_battlefield_for_id(int player, int);
int check_battlefield_for_targettable_id(int player, int card, int, int, int);
int store_and_trigger_graveyard_from_play(int player, int card, int event, int t_player, int trigger_mode, test_definition_t* test, int triggering_player, int triggering_card);	// Only needs to be called during EVENT_GRAVEYARD_FROM_PLAY and TRIGGER_GRAVEYARD_FROM_PLAY.  (More to the point, only need to initialize test then.)
int count_graveyard_from_play(int player, int card, int event, int, int, int, int, int, int, int, int, int, int, int);
void remove_summoning_sickness(int player, int card );
int check_for_cip_effects_removal(int player, int card);
int not_played_from_hand(int player, int card);
int indestructible_until_eot(int player, int card, event_t event);
void update_rules_engine(int);
int generic_subtype_searcher(int player, int card, event_t event, int, int, int);
int count_auras_enchanting_me(int player, int card);
int curse(int player, int card, event_t event);
int target_player_sacrifices_a_subtype(int player, int card, int, int, int);
int make_test(int player, int, int, int, int, int, int, int, int, int, int, int, int);
int get_test_score(int, int, int, int, int, int, int, int, int, int);
void reanimate_all(int player, int card, int targ_player, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5, reanimate_mode_t action);
int new_reanimate_all(int player, int card, int targ_player, test_definition_t *this_test, reanimate_mode_t action);
void human_moon_phases(int player, int card, event_t event);
void werewolf_moon_phases(int player, int card, event_t event);
static inline int morbid(void){ return creatures_dead_this_turn > 0; }
void set_starting_life_total(int player, int);
int get_starting_life_total(int player);
int has_mana_to_cast_iid(int player, event_t event, int iid);	// prefer this one to has_mana_to_cast_id() if you already have an internal_card_id
int has_mana_to_cast_id(int player, event_t event, int csvid);
int is_id_in_grave(int player, int);
int is_id_in_hand(int player, int);
int charge_mana_from_id(int player_to_charge, int card_to_charge_for/*optional; set to -1 to omit*/, event_t event, int csvid);
void remove_id_from_grave(int player, int csvid, remove_id_from_grave_t mode);
int make_test_in_play(int, int, int, int, int, int, int, int, int, int, int, int, int);
void skaabs(int player, int card, event_t event, int);
void unattach(int, int);
int tutor_random_permanent_from_grave(int player, int card, int t_player, int tutor_type, int type, int amount, reanimate_mode_t reanimation_mode);
void gain_life(int player, int amount);
int trigger_gain_life(int player, int card, event_t event, int who_gains, resolve_trigger_t trigger_mode);	// Returns the amount of life gained, which will always be at least 1.  EA_LICH must be set in event_flags for the trigger to be dispatched (so a card with Flags: Lich set in ct_all.csv must have been on the battlefield this turn, or set it manually each turn during EVENT_CAN_SKIP_TURN).
void add_state(int player, int card, int);
void remove_state(int player, int card, int);
void add_status(int player, int card, int);
void remove_status(int player, int card, int);
int effect_frost_titan(int player, int card, int t_player, int t_card);
int check_state(int player, int card, int);
void protection_from_creatures(int player, int card, event_t event);
void kill_my_legacy(int player, int card);
int must_be_blocked(int player, int card, event_t event);
void set_special_infos(int player, int card, int);
int control_basic_land(int player);
int can_draw_cards_as_cost(int player, int amount);
int draw_cards(int player, int amount);
void meddling_mage_effect(int player, int card, event_t event);
int is_phyrexian(int);
void untap_lands(int player, int card, int);
int until_eot_legacy(int player, int card, event_t event);
int artifact_animation(int player, int card, int, int, int, int, int, int, int);
void special_abilities(int player, int card, event_t event, int sp_abilities, int source_player, int source_card);
int everybody_must_block_me(int player, int card, event_t event);
void subtype_must_block_me(int player, int card, event_t event, subtype_t subtype, const char* prompt);
void produce_mana_from_colors(int player, int card, int);
int flip_a_coin(int player, int card);
int player_flips_a_coin(int player, int card, int who_flips);	// Returns a flip_a_coin_values
int vanilla_pump(int player, int card, event_t event, target_definition_t *td, int power, int toughness, int keyword, int sp_keyword);	// Can be used with sorceries and with any target restrictions, but the AI can't fully understand the power/toughness modifications.  td only needs to be set if IS_CASTING(player, card, event).
int vanilla_instant_pump(int player, int card, event_t event, int allowed_controller, int preferred_controller, int power, int toughness, int keyword, int sp_keyword);	// A more restricted version of generic_pump(), usable only for instants and only when there are no restrictions on targeting except for whose creature it can target; but this lets us teach the AI about the power/toughness modifications.
int shuffle(int player);
int count_creatures_by_power(int player, int, int);
void rfg_when_damage(int player, int card, event_t event);
int vanilla_creature_pumper(int player, int card, event_t event, int, int, int, int, int, int, int, int, int, int, int, target_definition_t *td);
int get_special_abilities(int player, int card);
int get_special_abilities_by_instance(card_instance_t* instance);
int real_count_colors(int);
int count_colors(int player, int card);
int evaluate_colors(int, int);
void vanishing(int player, int card, event_t event, int);
void land_animation(int player, int card);
void affinity(int player, int card, event_t event, int, int);
int end_of_combat_trigger(int player, int card, event_t event, int trigger_mode);
void set_life_total(int player, int amount);
int lifegaining_charm(int player, int card, event_t event, int, int, int, int);
int cip_lifegain(int player, int card, event_t event, int);
int check_battlefield_for_sacrifice(int player, int, int, int, int, int, int, int, int, int, int);
int pick_special_permanent_for_sacrifice(int player, int card, int, int, int, int, int, int, int, int, int, int, int);
int pick_permanent_for_sacrifice(int player, int card, int, int);
int altar_basic(int player, int card, event_t event, int, int);
int can_activate_altar_basic(int player, int card, int, int);
int altar_basic_activation(int player, int card, int, int);
int altar_extended(int player, int card, event_t event, int, int, int, int, int, int, int, int, int, int, int);
void cannot_lose_the_game_for_having_less_than_0_life(int player, int card, event_t event, int who_cant_lose);
void cannot_lose_the_game(int player, int card, event_t event, int who_cant_lose);
int nims(int player, int card, event_t event);
int damage_redirection(int player, int card, event_t event);
int generic_x_spell(int player, int card, event_t event, int, int, int);
int steal_permanent_from_target_opponent_deck(int player, int card, event_t event, test_definition_t *this_test);
int modular_legacy(int player, int card, event_t event);
int time_walk_effect(int player, int card);
int get_updated_equip_cost(int player, int card, int);
int can_activate_basic_equipment(int player, int card, event_t event, int equip_cost);
int resolve_activation_basic_equipment(int player, int card);
int activate_basic_equipment(int player, int card, int equip_cost);
int miracle(int player, int card, event_t event);
int get_rules_engine_infos(int player);
int can_pay_life(int player, int amount);
int lose_life(int player, int);
void store_attackers(int player, int card, event_t event, declare_attackers_trigger_t mode, int attacker_player, int attacker_card, test_definition_t* test);
int resolve_declare_attackers_trigger(int player, int card, event_t event, declare_attackers_trigger_t mode);
int declare_attackers_trigger(int player, int card, event_t event, declare_attackers_trigger_t mode, int attacker_player, int attacker_card);	// Stores an internal count of attackers in (player,card)->targets[1].player; returns that amount.
int declare_attackers_trigger_test(int player, int card, event_t event, declare_attackers_trigger_t mode, int attacker_player, int attacker_card, test_definition_t* test);	// As declare_attackers_trigger, but attacking cards must also pass test.
int pick_target_nonbasic_land(int player, int card, int);
int impose_sacrifice(int player, int card, int t_player, int amount, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5);
int can_sacrifice_this_as_cost(int player, int card);	// The simple, common case
int can_cause_sacrifice(int player, int t_player);
int new_can_sacrifice_as_cost(int player, int card, test_definition_t* test);	// Verify that "player" could sacrifice test->qty permanents matching test for paying a cost.
int max_can_sacrifice_as_cost(int player, int card, test_definition_t* test);	// Return the maximum number of permanents that player could sacrifice matching test for paying a cost.
int can_sacrifice_as_cost(int player, int amount, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5);
int can_sacrifice_type_as_cost(int player, int amount, type_t type);
int new_can_sacrifice(int player, int card, int t_player, test_definition_t* test);
int sacrifice_and_report_value(int player, int card, int t_player, int flags, test_definition_t *test);
int can_sacrifice(int player, int t_player, int amount, type_t type, subtype_t subtype);
int new_sacrifice(int player, int card, int t_player, sacrifice_t options, test_definition_t* test);
int mark_sacrifice(int player, int card, int t_player, sacrifice_t options, test_definition_t* test, char* marked_for_sacrifice);	// marked_for_sacrifice should be an array char[151].  It does not need to be initialized.  It'll get set to 1 for each permanent chosen to be sacrificed.
int controller_sacrifices_a_permanent(int player, int card, int req_type, sacrifice_t options);
int player_sacrifices_a_permanent(int src_player, int src_card, int t_player, int req_type, sacrifice_t options);
int player_sacrifices_a_hacked_land(int src_player, int src_card, int t_player, subtype_t land_subtype, sacrifice_t options);	// only usable with basic land subtypes; use player_sacrifices_a_permanent if you just want a land
int sacrifice(int player, int card, int t_player, int cannot_cancel, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5);	// The old interface.  Deprecated.
int undying(int player, int card, event_t event);
int is_subtype_dead(int player, int, int, int);
int charge_mana_for_double_x(int player, int color);	// Charges XX of color.  Returns total mana spent, not what X is.
int charge_mana_for_multi_x(int player, int color, int num_x);	// A generalization of charge_mana_for_double_x().  num_x==2 => charge XX.  num_x==3 => charge XXX.  etc.  Returns total mana spent, not what X is.
int insurrection_effect(int player, int card);
int cannot_regenerate_until_eot(int player, int card, int, int);
int damage_dealt_by_me(int player, int card, event_t event, ddbm_flags_t mode);	// Overwrites instance->targets[8].player, instance->targets[16].player, and for some modes instance->targets[1].player.
void check_damage(int player, int card, event_t event, ddbm_flags_t mode, int triggering_player, int triggering_card);
int resolve_damage_trigger(int player, int card, event_t event, ddbm_flags_t mode, int triggering_player, int triggering_card);
int damage_dealt_by_me_arbitrary(int player, int card, event_t event, ddbm_flags_t mode, int triggering_player, int triggering_card);	// Just like damage_dealt_by_me(), except can specify a different triggering_player and triggering_card that highlights for the trigger and which gets its targets[1,8,16].player overwritten instead of player/card.
void whenever_i_deal_damage_to_a_player_he_discards_a_card(int player, int card, int event, ddbm_flags_t mode, int discard_is_random);	// A common case.  Uses targets[1], [8], [16].player, as usual.  DDBM_MUST_DAMAGE_PLAYER and DDBM_TRACE_DAMAGED_PLAYERS are added to mode.
int attached_creature_deals_damage(int player, int card, event_t event, ddbm_flags_t mode);	// A common case of damage_dealt_by_me_arbitrary(), this triggers on damage dealt by the card (not necessarily a creature) {player,card} is attached to.
int equipped_creature_deals_damage(int player, int card, event_t event, ddbm_flags_t mode);	// A common case of damage_dealt_by_me_arbitrary(), this triggers on damage dealt by the card {player,card} is equipping.
void choose_who_attack(int player, int card);
int sengir_vampire_trigger(int player, int card, event_t event, int);
int reveal_x_and_choose_a_card_type(int player, int card, int to_reveal, int selected_type);
int target_must_block_me(int player, int card, int t_player, int t_card, int mode);
int freeze_when_damage(int player, int card, event_t event);
int has_dead_creature(int player);
int subtype_deals_damage(int player, int card, event_t event, int, int, int);
int target_player_skips_next_untap(int player, int card, int);
int bushido(int player, int card, event_t event, int);
void splice_onto_arcane(int player, int card);
void check_for_spliced_spells(int player, int card);
int arcane_with_splice(int player, int card, int event, int (*fn)(int,int,event_t), int x, int b, int u, int g, int r, int w);	// A handy frontend to Arcane instants and sorceries that also have Splice onto Arcane.  fn() is almost a normal card function; the only difference is that it shouldn't call kill_card(player, card, KILL_DESTROY) during EVENT_RESOLVE_SPELL.  Even handier when called via the ARCANE_WITH_SPLICE macro.  If x == -10, then send EVENT_CAN_SPLICE and EVENT_SPLICE to the supplied function; it should respond only by checking for and charging the splice cost respectively, returning 1 on success or 0 on failure.  This differs from the normal EVENT_CAN_SPLICE/EVENT_SPLICE behavior.
#define ARCANE_WITH_SPLICE(spell_fn, manacost)	static int spell_fn##_impl(int player, int card, event_t event); int spell_fn(int player, int card, event_t event) { return arcane_with_splice(player, card, event, spell_fn##_impl, manacost); } static int spell_fn##_impl(int player, int card, event_t event)
int arcane(int player, int card, int event, int (*fn)(int,int,event_t));	// As arcane_with_splice, but can't be spliced itself.
#define ARCANE(spell_fn)	static int spell_fn##_impl(int player, int card, event_t event); int spell_fn(int player, int card, event_t event) { return arcane(player, card, event, spell_fn##_impl); } static int spell_fn##_impl(int player, int card, event_t event)
int soulshift(int player, int card, event_t event, int cmc, int num);
int arcane_spirit_spell_trigger(int player, int card, event_t event, int);
int generic_activated_ability(int player, int card, event_t event, int mode, int x, int b, int u, int g, int r, int w, uint32_t variable_costs, target_definition_t *td, const char *prompt);
int granted_generic_activated_ability(int granting_player, int granting_card, int player, int card, event_t event, int mode, int x, int b, int u, int g, int r, int w, uint32_t variable_costs, target_definition_t *td, const char *prompt);
int attachment_granting_activated_ability(int player, int card, event_t event, int mode, int cless, int black, int blue, int green, int red, int white, uint32_t variable_costs, target_definition_t *td, const char *prompt);	// Uses targets[9] to store the player/card it was attached to at the time it was activated.  Should be called *before* the EVENT_RESOLVE_ACTIVATION handler if td is provided.
int aura_granting_activated_ability(int player, int card, event_t event, int preferred_controller, int mode, int cless, int black, int blue, int green, int red, int white, uint32_t variable_costs, target_definition_t *td, const char *prompt);	// Uses targets[9] to store the player/card it was attached to at the time it was activated.  Should be called *before* the EVENT_RESOLVE_ACTIVATION handler if td is provided.
int equipment_granting_activated_ability(int player, int card, event_t event, const char* dialog_option, int equip_cost, int mode, int cless, int black, int blue, int green, int red, int white, uint32_t variable_costs, target_definition_t *td, const char *prompt);	// Uses targets[9] to store the player/card it was attached to at the time it was activated and info_slot to store the choice made.  Returns 1 during EVENT_RESOLVE_ACTIVATION if the activated ability was selected, 0 if it was equipped.
int prevent_all_damage(int player, int card, event_t event);
int prevent_all_damage_to_target(int player, int card,  int, int, int );
int seek_grave_for_id_to_reanimate(int player, int card, int t_player, int id, reanimate_mode_t mode);
int pick_target_permanent(target_definition_t *td);
int bounce_permanent_at_upkeep(int player, int card, event_t event, target_definition_t *td);
void get_back_your_permanents(int player, int card, type_t typ);
int card_from_list(int player, int, int, int, int, int, int, int, int, int, int, int);
int name_a_card(int player, int ai_player, test_definition_t *test, int mode);
int totem_armor(int player, int card, event_t event, int, int, int, int);
int devouring(int player, int card);
int has_convoked_mana(int player, int card, int, int, int);
int has_convoked_mana_extended(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white);
int charge_convoked_mana(int player, int card, int, int, int);
int charge_convoked_mana_extended(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white);
int charge_convoked_mana_extended_generic(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white, int check_for_cc_modifier);
int cast_spell_with_convoke(int player, int card, event_t event);
int generic_spell_with_convoke(int player, int card, event_t event);
void default_test_definition( test_definition_t *this_test, int);
int manipulate_all(int player, int card, int p, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5, actions_t action);
int new_manipulate_all(int player, int card, int p, test_definition_t* this_test, actions_t action);
int manipulate_type(int player, int card, int p, type_t type, actions_t action);
int new_damage_all(int player, int card, int targ_player, int dmg, int mode, test_definition_t* this_test/*optional*/);
int new_global_tutor(int player, int targ_player, int search_location, int destination, int must_select, int ai_selection_mode, test_definition_t* this_test);
int tutor_from_grave_to_hand_except_for_dying_card(int player, int card, int must_select, int ai_selection_mode, test_definition_t* test);
int create_id_legacy(int player, int card, int, int, int);
void exile_if_would_be_put_into_graveyard(int player, int card, int, int, int);
int can_block_me(int blocked_player, int blocked_card, int blocking_player, int blocking_card);
int gain_control_until_eot(int player, int card, int t_player, int t_card);
int colors_shared(int, int);
int locate_id(int player, int);
int negate_ability_until_eot(int player, int card, int t_player, int t_card, keyword_t keyword);	// Affects all of t_player's creatures if t_card == -1, and all creatures if t_player == 2 as well.  t_player == 2, t_card != -1 is undefined behavior.  Returns created legacy if t_player != 2 and t_card != -1 (i.e., a single creature), otherwise -1.
int bloodthirst(int player, int card, event_t event, int);
int count_creatures_by_toughness(int player, int, int);
int ravnica_manachanger(int player, int card, event_t event, int, int, int, int, int);
int charge_changed_mana(int player, int card, int, int, int, int, int);
void exchange_control_of_target_permanents(int player, int card, int, int, int, int);
int dredge(int player, int card, event_t event, int);
int maze_of_ith_effect(int player, int card, int, int);
int new_get_test_score(test_definition_t *this_test);
void activate_oran_rief_pump(int player, int card);
int new_select_a_card(int player, int, int, int, int, int, test_definition_t *this_test);
int select_card_from_zone(int player, int, const int *s_location, int, int, int, int, test_definition_t *this_test);
int select_multiple_cards_from_graveyard(int player, int targ_player, int must_select_all /*if < 0, not enforced; but failing to select all is considered cancelling */, int ai_selection_mode, test_definition_t* this_test /*optional*/, int max_targets, target_t* ret_location);
void descending_insertion_sort_on_player(int number, target_t* array);	// Simple n^2 insertion sort on array[number].player, descending.  Poorly suited if number is greater than about 30.
int check_special_flags(int player, int card, int);
void set_special_flags(int player, int card, int);
void remove_special_flags(int player, int card, int);
int check_special_flags2(int player, int card, int);
void set_special_flags2(int player, int card, int);
void remove_special_flags2(int player, int card, int);
int check_special_flags3(int player, int card, int);
void set_special_flags3(int player, int card, int);
void remove_special_flags3(int player, int card, int);
int suspend_a_card(int player, int card, int, int, int);
int skip_next_turn(int player, int card, int);
int this_turn_is_being_skipped(void);	// Returns true if the turn about to be started will be skipped instead.  Accurate only during EVENT_CAN_SKIP_TURN (the first event of each turn).
void null_casting_cost(int player, int card);
void infinite_casting_cost(void);
int madness(int player, int card, event_t event, int, int, int, int, int, int);
int buyback(int player, int card, int, int, int, int, int, int);
int die_at_end_of_combat(int player, int card, event_t event);
int reveal_any_number_of_cards_of_selected_color(int player, int card, int);
int enchant_player(int player, int card, event_t event, target_definition_t *td);
int new_make_test_in_play(int player, int to_examine, int test_type, test_definition_t *this_test);
int suspend(int player, int card, event_t event, int turns, int colorless, int black, int blue, int green, int red, int white, target_definition_t* td, const char* prompt);
int count_upkeeps(int player);
int evincars(int player, int card, event_t event, color_test_t color);
int new_select_target_from_grave(int player, int card, int t_player, select_grave_t mode, int ai_mode, test_definition_t *this_test, int ret_location);	// Chooses a card in t_player's graveyard that matches this_test.  Sets {player,card}->targets[ret_location].player to its location in the graveyard and {player,card}->targets[ret_location].card to its internal_card_id.  Returns the location, or -1 if not chosen.  If anything else removes a lower card in the graveyard before the selected card is acted upon, this will fail.  Prefer select_target_from_grave_source()/select_target_from_either_grave() and validate_target_from_grave_source() in new code.
int select_target_from_grave_source(int player, int card, int t_player, select_grave_t mode, int ai_mode, test_definition_t *this_test, int ret_location);	// Similar to new_select_target_from_grave(), but puts the graveyard_source[][] value in targets[ret_location].card instead of the internal_card_id, so the card can be found even if it moves within the graveyard by the time it's validated.  Returns the internal_card_id, or -1 and sets cancel if not chosen.
int select_target_from_either_grave(int player, int card, select_grave_t mode, int ai_mode_own_graveyard, int ai_mode_opponent_graveyard, test_definition_t* test, int ret_location_graveyard, int ret_location_card);	// Prompts for a graveyard (if both have a targetable card), then prompts for a card.  Returns -1 and sets cancel if cancelled or there's no targets.  Otherwise identical to select_target_from_grave_source().
int validate_target_from_grave(int player, int card, int whose_graveyard, int target_location);	// Checks to see if get_grave(whose_graveyard)[{player,card}->targets[target_location].player] is still {player,card}->targets[target_location].card.  This will fail if anything else removes a lower card in the graveyard in response, even if the selected card is still there.  Prefer select_target_from_grave_source()/select_target_from_either_grave() and validate_target_from_grave_source() in new code.
int validate_target_from_grave_source(int player, int card, int whose_graveyard, int target_location);	// Similar to validate_target_from_grave(), but uses the values set in select_target_from_grave_source()/select_target_from_either_grave() to find the selected card even if it's moved within its graveyard.  Returns the position within whose_graveyard's graveyard if the card is still there, else -1.
void add_spore_counters(int player, int card, event_t event);
void saproling_from_fungus(int player, int card);
int can_make_saproling_from_fungus(int player, int card);
void from_grave_to_hand(int player, int grave_position, int unused);
int from_grave_to_hand_multiple(int player, test_definition_t* test);	// Returns all cards matching test in player's graveyard to his hand.  Returns the number of matching cards.
void from_grave_to_hand_triggers(int player, int card_added);	// Runs triggers specifically looking for a card moving from graveyard to hand, like Golgari Brownscale.  Avoid calling directly; use from_grave_to_hand(), from_grave_to_hand_multiple(), or new_global_tutor() instead.
int new_get_test_score(test_definition_t *this_test);
void pump_creatures_until_eot(int player, int card, int t_player, int alternate_legacy_text_number, int power, int toughness, int ability, int sp_ability, test_definition_t *this_test /* optional */);
void pump_creatures_until_eot_no_repeat(int player, int card, int t_player, int alternate_legacy_text_number, int ability, int sp_ability, test_definition_t *this_test /* optional */);	// Just like pump_creatures_until_eot(), but can't pump power or toughness, and if the creature already has an effect card from {player,card} and has all the named keywords set, only gives the existing effect a new timestamp instead of making a new one.
void pump_creatures_until_eot_merge_pt(int player, int card, int t_player, int power, int toughness, test_definition_t *this_test /* optional */);	// Just like pump_creatures_until_eot(), but can't pump abilities, and sends its effect through pump_until_eot_merge_previous() so they combine into a single effect card.  Requires that the card's effect text uses |n substitutions.
void pump_creatures_until_eot_merge_pt_alternate_legacy_text(int player, int card, int t_player, int power, int toughness, test_definition_t *this_test /* optional */, int alt_text_number);	// Just like pump_creatures_until_eot_merge_pt(), but allows an alternate legacy text number.
int slivercycling(int player, int card, event_t event);
int graft(int player, int card, event_t event, int);
int do_flashback(int player, int card, event_t, int, int, int, int, int, int);
int do_kicker(int player, int card, int, int, int, int, int, int);
int get_stormcolor_count(int player, color_t clr);	// Only honors COLOR_BLACK through COLOR_WHITE, and returns a maximum of 15.
int kicker(int player, int card, event_t event, int, int, int, int, int, int);
int kicked(int player, int card);
int force_a_subtype_until_eot(int player, int card, int, int, subtype_t subtype);
void new_default_test_definition(test_definition_t *this_test, int, const char* msg);
int count_defenders(int player);
int check_battlefield_for_special_card(int player, int card, int, int, test_definition_t *this_test);
int damage_dealt_to_me_arbitrary(int player, int card, event_t event, ddbm_flags_t mode, int triggering_player, int triggering_card);	// Sets triggering_player/card's targets[7].player to number of times damaged and targets[7].card to total amount of damage.  Uses targets[6] internally.  Most DDBM_* flags unimplemented.
void default_token_definition(int player, int card, int csvid, token_generation_t *token);
void copy_token_definition(int player, int card, token_generation_t* token, int t_player, int t_card);	// Create a token definition that's a copy of (t_player,t_card).
int copy_token_characteristics_for_clone(int player, int card, int t_player, int t_card);
int token_characteristic_setting_effects(int player, int card, event_t event, int t_player, int t_card);	// Just like generic_token(), but supports storing the values on an effect card to set them on a different permanent.  The idea is that changing a permanent to temporarily be a copy of a token shouldn't have to overwrite all of its targets.
int generic_token(int player, int card, event_t event);
int new_make_test(int player, int, int, test_definition_t *this_test);
int effect_mana_drain(int player, int card, event_t event);
int effect_scars_of_the_veteran(int player, int card, event_t event);
int get_power_sum(int player);
	int discard_trigger(int player, int card, event_t event, int t_player, int trigger_mode, int discard_trigger_mode);	// discard_trigger_mode is a discard_trigger_t
int champion(int player, int card, event_t event, int, int);
int evoked(int player, int card);
void lorwyn_need_subtype_land(int player, int card, event_t event, subtype_t subtype);
int new_effect_coercion(ec_definition_t *this_definition, test_definition_t *this_test);
void default_ec_definition(int target_player, int who_chooses, ec_definition_t *this_definition);
int clash(int player, int card);
int conspire(int player, int card, event_t event);
void set_pt_and_abilities(int player, int card, event_t event, int, int, int, int);
int set_pt_and_abilities_until_eot(int player, int card, int, int, int, int, int, int, int);
void copy_token(int player, int card, int, int);
int hybrid_casting(int player, int card, int);
int modify_cost_for_hybrid_spells(int player, int card, event_t event, int);
int liege(int player, int card, event_t event, int, int);
int card_filter_land(int player, int card, event_t event, int, int, int, int, const char * str);
int target_me(int player, int card, int, int);
int new_special_count_grave(int player, test_definition_t *this_test);
int new_specific_cip(int player, int card, event_t event, int t_player, int trigger_mode, test_definition_t* this_test/*optional*/);
int get_number_of_hybrid_mana(int player, int card, int, int);
int get_colors_for_hybrid(int player, int card, int, int);
void count_for_gfp_ability_and_store_values(int player, int card, int event, int t_player, int type, test_definition_t *this_test/*optional*/, int mode, int storage_location);
void count_for_gfp_ability(int player, int card, int event, int t_player, int type, test_definition_t *this_test/*optional*/);
int resolve_gfp_ability(int player, int card, event_t event, resolve_trigger_t trig_mode);
int get_random_card_in_hand(int player);
int can_regenerate(int player, int card);
int can_be_regenerated(int player, int card);
int regenerate_or_shield(int player, int card, int t_player, int t_card);
int can_pay_flashback(int player, int iid, event_t event, int colorless, int black, int blue, int green, int red, int white);
int pay_flashback(int player, int iid, event_t event, int colorless, int black, int blue, int green, int red, int white);
int my_base_value_by_id(int);
int my_base_value(int player, int card);
int has_flashback(int card);
int reveal_cards_from_your_hand(int player, int card, test_definition_t *this_test);
void reveal_card(int source_player, int source_card, int revealed_player, int revealed_card);	// source_player/source_card are the source of the effect.  If they're unknown, put the revealed player/card there and use -1,-1 for revealed_player, revealed_card.
void reveal_card_iid(int source_player, int source_card, int revealed_iid);
void look_at_iid(int source_player, int source_card, int revealed_iid, const char* prompt);	// Identical to reveal_card_iid, but arbitrary text so you can pretend it's not a reveal
int reveal_card_optional_iid(int source_player, int source_card, int revealed_iid, const char* prompt/*optional*/);	// Gives the human a choice whether to reveal a card or not.  The AI always does.  Returns 1 if revealed.
int regenerate_target(int player, int card);
int epic_legacy(int player, int card, event_t event);
void flip_card(int player, int card);
void check_for_turned_face_up_card_interactions(int player, int card, int new_identity);
void horsemanship2(int player, int card, event_t event);
int get_highest_power(int player );
void protection_from_multicolored(int player, int card, event_t event);
int new_get_special_permanent_for_sacrifice(int player, int card, test_definition_t* test);
void add_legacy_effect_to_all(int player, int card, int (*func_ptr)(int, int, event_t), int, test_definition_t *this_test);
void dark_confidant_effect(int player, int card, int);
int keyrune(int player, int card, event_t event, int, int, int, int, int, int, int, int, int, int, int, int);
int cantrip(int player, int card, int);
int multikicker(int player, int card, event_t event, int, int, int, int, int, int);
int do_multikicker(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white);
int pariah_effect(int player, int card, event_t event, int, int);
int monolith(int player, int card, event_t event, int);
int hidden_enchantment(int player, int card, event_t event, int type, test_definition_t *this_test, int subt);
int reveal_cards_from_hand(int player, int card, test_definition_t *this_test);
int hom_clockwork(int player, int card, event_t event, int);
int unleash(int player, int card, event_t event);
int detain(int player, int card, int, int);
int mind_funeral_effect(int player, int card, int);
int battalion(int player, int card, event_t event);
int can_cast_creature_from_grave(int player, int mode, event_t event);
void populate(int player, int card);
void cipher(int player, int card);
int bloodrush(int player, int card, event_t event, int, int, int, int, int, int, int, int, int, int);
int overload(int player, int card, event_t event, int, int, int, int, int, int);
int pay_overload(int player, int card, int, int, int, int, int, int);
void extort(int player, int card, event_t event);
int creatures_cannot_block(int player, int card, test_definition_t *this_test, int);
void creature1_cant_block_creature2_until_eot(int src_player, int src_card, int t_player, int t_card, int unblockable_player, int unblockable_card);
int scavenge(int player, int card, event_t event, int, int, int, int, int, int);
int create_random_test_deck(int player, int card, int, int, int, int);
int can_cast_spell_from_grave(int player, int mode);
int annihilator(int player, int card, event_t event, int);
int check_for_id_legacy(int player, int);
void oblivion_stone_effect(void);
int check_status(int player, int card, int);
int is_stolen(int player, int card);
int get_owner(int player, int card);
void mana_into_string(int, int, int, int, int, int, test_definition_t *this_test);
void haste(int player, int card, event_t event);
int get_tutoring_denial(int player);
void lose_the_game(int player);
void set_special_abilities(int player, int card, int, int);
int counterspell(int player, int card, event_t event, target_definition_t* td /*optional*/, int target_num);
int counterspell_validate(int player, int card, target_definition_t* td /*optional*/, int target_num);
int counterspell_resolve_unless_pay_x(int player, int card, target_definition_t* td /*optional*/, int target_num, int x);	// Returns 0 if didn't validate, 1 if validated but didn't pay, 2 if paid.
int can_counter_activated_ability(int player, int card, event_t event, target_definition_t* td /*optional*/);
int cast_counter_activated_ability(int player, int card, int target_pos);
int validate_counter_activated_ability(int player, int card, target_definition_t* td /*optional*/, int target_pos);
void raw_counter_activated_ability(int tgt_player, int tgt_card);
int resolve_counter_activated_ability(int player, int card, target_definition_t* td /*optional*/, int target_pos);	// validate_counter_activated_ability() then raw_counter_activated_ability()
int twincast(int player, int card, int event, target_definition_t* td /*optional*/, int* ret_copy /*optional*/);
int activate_twincast(int player, int card, int event, target_definition_t* td /*optional*/, int* ret_copy /*optional*/);
int land_animation2(int player, int card, int, int, int, int, int, int, int, int, test_definition_t *this_test);
int dungeon_geist_effect(int player, int card, event_t event);
int choose_a_dead_this_turn(int player, int card, int, int, int, test_definition_t *this_test, int);
int type_change_legacy(int player, int card, event_t event);
int turn_into_artifact(int player, int card, int, int, int);
int turn_into_creature(int player, int card, int, int, int, int, int);
int lead_the_stampede_effect(int player, int csvid, int amount);
void cannot_block(int player, int card, event_t event);
void give_shroud_to_player(int player, int card, event_t event);
void give_hexproof_to_player(int player, int card, event_t event);
int is_hardcoded_card(int player, int card);
int prevent_damage_until_eot(int player, int card, int, int, int);
void cloning(int player, int card, int t_player, int t_card);	// Should be called only before entering or as enters the battlefield
void cloning_and_verify_legend(int player, int card, int t_player, int t_card);	// Should be called if changing types after already being in play.
void cloning_card(int player, int card, event_t event);
void enters_the_battlefield_as_copy_of(int player, int card, event_t event, int t_player, int t_card);	// t_player and t_card only need to be initialized during EVENT_RESOLVE_SPELL.
int enters_the_battlefield_as_copy_of_any(int player, int card, event_t event, target_definition_t* td, const char* prompt);	// As enters_the_battlefield_as_copy_of(), but prompts for a target.  Returns nonzero if copies.  td only needs to be initialized during EVENT_RESOLVE_SPELL.
int enters_the_battlefield_as_copy_of_any_creature(int player, int card, event_t event);	// Common case of enters_the_battlefield_as_copy_of_any().
void get_an_additional_combat_phase(void);
void relentless_assault_effect(int player, int card);
int finest_hour_legacy(int player, int card, event_t event);
void legacy_name(char* title, int player, int card);
int alternate_legacy_text(int textnum, int player, int legacycard);	// Flags an effect card to use alternate text, set off with |#1, |#2, etc. tags in its text.  Works only on csvid 903 effects (which includes both LEGACY_EFFECT_CUSTOM and LEGACY_EFFECT_GENERIC) other than LEGACY_EFFECT_PUMP.  So it's nonfunctional with create_card_name_legacy().  Returns legacycard again.
int set_legacy_image(int player, int csvid, int legacycard);	// Sets {player,legacycard}'s image (and therefore title and text) to be csvid.  Works only on effect cards.  Returns legacycard again.
void al_pitchspell(int player, int card, event_t event, int, int);
int casting_al_pitchspell(int player, int card, event_t event, int, int);
void after_load_game(void);
int get_cost_mod_for_activated_abilities(int player, int card, int, int, int, int, int, int);
void set_cost_mod_for_activated_abilities(int player, int card, int, test_definition_t *this_test);
void remove_cost_mod_for_activated_abilities(int player, int card, int, test_definition_t *this_test);
int has_mana_for_activated_ability(int player, int card, int cless, int black, int blue, int green, int red, int white);
int charge_mana_for_activated_ability(int player, int card, int cless, int black, int blue, int green, int red, int white);
int tapsubtype_ability(int player, int card, int tap_req, target_definition_t *td);
int seal(int player, int card, event_t event, target_definition_t *td, const char *prompt);
void gain_control_until_source_is_in_play_and_tapped(int player, int card, int t_player, int t_card, gcus_t mode);
void choose_to_untap(int player, int card, event_t event);	// AI leaves tapped if in_play(targets[1])
int choosing_to_untap(int player, int card, event_t event);	// Call as if (choosing_to_untap(player, card, event)) { return should_ai_untap; }.  AI will leave tapped if that return value is 0, otherwise will untap.
int chroma(int player, int card, int clr1, int clr2);	// If card is supplied, counts *that card only*.  Usually you want -1.
int devotion(int player, int card, int clr1, int clr2);	// Exactly the same as chroma(player, -1, clr1, clr2), except sleights clr1 and clr2 unless card is -1.
int new_specific_spell_played(int player, int card, event_t event, int, int, test_definition_t *this_test);
int generic_spell(int player, int card, event_t event, int flags, target_definition_t *td, const char *prompt, int max_targets, test_definition_t *this_test);
int basic_spell(int player, int card, event_t event);
int graveyard_has_shroud(int player);
int is_x_spell(int player, int card);
void bounce_permanent(int, int);
void put_on_top_of_deck(int player, int card);
void shuffle_into_library(int player, int card);
void fight(int p1, int c1, int p2, int c2);
void spirit_link_effect(int player, int card, event_t event, int who_gains);
void arbitrary_can_block_additional(event_t event, int num_additional);	// (affected_card_controller, affected_card) can block an extra num_additional creatures.  A permanent with EA_SELECT_BLOCK ("Flags: Select Block") must be in play as blockers are chosen.
void creature_can_block_additional(int player, int card, event_t event, int num_additional);	// equivalent to if (affect_me(player, card)) arbitrary_can_block_additional(event, num_additional);
void attached_creature_can_block_additional(int player, int card, event_t event, int num_additional);	// a thin wrapper around creature_can_block_additional(instance->damage_target_player, instance->damage_target_card, event, num_additional)
int can_block_additional_until_eot(int player, int card, int t_player, int t_card, int num_additional);	// Create a legacy effect attached to (t_player, t_card) letting it block an additional num_additional creatures until end of turn.  Returns the legacy card.  Can also combine into pump_ability_until_eot effects by setting targets[5].player.
int is_unblocked(int player, int card);	// Returns 1 if {player,card} is attacking and unblocked.
int count_blockers(int player, int event);	// Returns the number of creatures player controls that are blocking anything.
int blocking(int player, int card, event_t event);	// Returns nonzero if {player,card} is blocking anything.
int count_creatures_this_is_blocking(int player, int card);	// Returns count of creatures being blocked by (player, card), dealing properly with both banding and multiblocker creatures.
int count_my_blockers(int player, int card);	// Returns number of creatures {player,card} is blocked by, dealing properly with banding.
int is_blocking(int blocking_card, int attack_card);	// Preconditions: attack_card is controlled by current_turn; blocking_card is controlled by 1-current_turn; in_play(current_turn, attack_card)
int is_blocking_or_blocked_by(int player1, int card1, int player2, int card2);	// player1/card1 and player2/card2 are interchangeable.
void for_each_creature_blocked_by_me(int player, int card, void (*fn)(int, int, int, int), int arg1, int arg2);	// Preconditions: (player,card) is blocking; fn is a function with signature void fn(int arg1, int arg2, int blocked_controller, int blocked_card).  fn() will be called with arguments arg1, arg2, blocked_controller, blocked_card for each (blocked_controller,blocked_card) blocked by (player,card). */
void mark_each_creature_blocked_by_me(int player, int card, char (*marked)[151]);	// A simple interface to for_each_creature_blocked_by_me() handling a common case.  marked should be a char array[2][151] initialized to 0; each element of marked[blocked_controller][blocked_card] will be set to 1.
void for_each_creature_blocking_me(int player, int card, void (*fn)(int, int, int, int), int arg1, int arg2);	// Preconditions: (player,card) is attacking; fn is a function with signature void fn(int arg1, int arg2, int blocking_controller, int blocking_card).  fn() will be called with arguments arg1, arg2, blocking_controller, blocking_card for each (blocking_controller,blocking_card) blocking (player,card). */
void mark_each_creature_blocking_me(int player, int card, char (*marked)[151]);	// A simple interface to for_each_creature_blocking_me() handling a common case.  marked should be a char array[2][151] initialized to 0; each element of marked[blocking_controller][blocking_card] will be set to 1.
int generic_licid(int player, int card, event_t event, int, int, int, int, int, int, int, int, int, int, int);
void aura_ability_for_color(int player, int card, event_t event, int, int, int, int, int);
void upkeep_trigger_ability(int player, int card, event_t event, int whose_upkeep);
void upkeep_trigger_ability_mode(int player, int card, event_t event, int whose_upkeep, resolve_trigger_t trigger_mode);
void untap_permanents_during_opponents_untap(int player, int card, type_t types, int* where_to_store);	// Untap all cards with type types controlled by player during his opponent's untap step.  Stores temporary data in where_to_store.
void separate_into_two_piles(int who_separates, const int* source_deck, int number, void* piles);	// Separates the first number cards in source_deck[] into two piles.  AI attempts to divide evenly by base_value.  Preconditions: piles is an array of int[2][number].  Elements of source_deck[] are internal_card_ids (such as from deck_ptr[], get_grave(), or rfg_ptr[]).  Postcondition: For each i in [0..number), one of piles[0][i] and piles[1][i] will be set to source_deck[i], and the other iid_draw_a_card.
int choose_between_two_piles(int chooser, int number, void* piles, int ai_selection_mode, const char* choice_txt);	// Chooses between two piles of cards each with number cards.  Preconditions: piles is an array of int[2][number].  Elements of piles[][] are internal_card_ids.  ai_selection_mode is AI_MAX_VALUE or AI_MIN_VALUE.  choice_txt is 11 chars or less.  Returns chosen pile, 0 or 1.
int raw_put_iid_on_top_of_graveyard(int player, int iid);	// Put a newly-created card with internal_card_id iid on top of player's graveyard and return its position.  Should only be used to replace cards temporarily removed as an implementation detail - doesn't run any "whenever a card is put into a graveyard" triggers.
void from_exile_to_graveyard(int player, int position_in_exile);	// Moves a card from a player's exile zone to his graveyard.
int turn_card_in_grave_face_down(int player, int position);	// Replaces the card in player's graveyard at position position to the card back image, and returns the internal_card_id that was there.  It's the caller's responsibility to turn it back with turn_card_in_grave_face_up(), using the same iid that was returned.
void turn_card_in_grave_face_up(int player, int position, int iid);	// Changes the card in player's graveyard at position position to the internal_card_id that was there.  Aborts if the current card there isn't face-down.  Please don't abuse this function to assign arbitrary values within a player's graveyard.
int exiledby_choose(int player, int card, int csvid, exiledby_choose_mode_t mode, int testarg, const char* cardtype_txt, int faceup);	// player/card: The card that exiled the cards to search for. * csvid: Csvid to display in the dialog.  Typically the same as player/card's; but must be provided since player/card may have been obliterated. * mode, testarg: Action and constraints per exiledby_choose_mode_t, documented in manalink.h. * cardtype_txt: Descriptor of cards exiled (e.g. "creature"), or NULL if arbitrary cards. * faceup: Whether the cards are exiled face-up (and thus should exist in rfg_ptr[][]) or face-down (and thus should not yet).
int* exiledby_find_any(int player, int card, int* ret_leg, int* ret_idx);	// Find the first card exiled by player/card starting at ret_leg/ret_idx.
int* exiledby_find(int player, int card, int needle, int* ret_leg, int* ret_idx);	// Find the first card exiled by player/card starting at ret_leg/ret_idx with stored value needle.
int exiledby_count(int player, int card, int owned_by);	// Returns total number of cards exiled by player/card owned by owned_by.
void exiledby_remember(int player, int card, int t_player, int iid, int* ret_leg, int* ret_idx);	// Remembers that a card with iid owned by t_player was exiled.  This doesn't actually exiled the card; if the exiled card is to be face-up, the caller should do so.
int exiledby_detach(int player, int card);	// Normally, these legacies are attached to the card that exiled them.  This is problematic when that permanent leaves the battlefield: all the attached effects will be obliterated.  This function turns the effect cards into freestanding legacies, determines a unique id, and stores that id in their targets[18].card so they can be distinguished from each other.  Returns the id.  Other functions in this package - except for exiledby_remember() - can then be called with player=(player-2), card=id to examine the legacies.  Caller will have to call exiledby_destroy_detached() later with that id, or else the legacies stay in place permanently.
void exiledby_destroy_detached(int player, int detach_id);	// Destroys all detached exiledby legacies set to detach_id.
void set_flags_when_spell_is_countered(int player, int card, int, int);
void exile_card_and_remember_it_on_exiledby(int player, int card, int t_player, int t_card);
const target_t* becomes_target_of_spell(int player, int card, event_t event, int t_player, int t_card, int controlled_by, resolve_trigger_t trigger_mode);	// When {t_player,t_card} becomes targeted by a spell controlled by controlled_by, returns the spell.  {player,card} is solely the card that highlights for the trigger.  Doesn't check is_humiliated() or in_play().
const target_t* becomes_target_of_spell_or_effect(int player, int card, event_t event, int t_player, int t_card, int controlled_by);	// When {t_player,t_card} becomes targeted by a spell or ability controlled by controlled_by, returns the spell or effect (else NULL).  {player,card} is solely the card that highlights for the trigger.  Doesn't check is_humiliated() or in_play().
const target_t* any_becomes_target(int player, int card, event_t event, int controller_of_target, int or_player, type_t targeted_type, int subtype, type_t targeting_type, int controller_of_trigger, resolve_trigger_t resolve_trigger);	// The general case.  Returns array of unique targeted_types (and players, if or_player is nonzero) controlled by controller_of_target currently being targeted by a targeting_type (TYPE_EFFECT: an effect; other types: a spell of that type being cast) controlled by controller_of_trigger, ending in a {-1,-1} entry; or NULL if none.
const target_t* any_creature_becomes_target_of_spell_or_effect(int player, int card, event_t event, int controller_of_target);	// A common case of any_becomes_target().  Returns array of unique creatures controlled by controller_of_target currently being targeted, ending in a {-1,-1} entry; or NULL if none.
int attached_creature_gains_sacrifice_when_becomes_target(int player, int card, event_t event);	// "(Creature this effect is attached to) gains 'When this creature becomes the target of a spell or ability, sacrifice it.'"
int kill_attachment_when_creature_is_targeted(int player, int card, event_t event, kill_t kill_code);	// "When enchanted creature becomes the target of a spell or ability, sacrifice/destroy/bury/exile (this aura)."
int kill_attachment_when_creature_is_targeted_by_spell(int player, int card, event_t event, kill_t kill_code);	// "When enchanted creature becomes the target of a spell, sacrifice/destroy/bury/exile (this aura)."
int get_original_internal_card_id(int player, int card);
int dispatch_event_with_initial_event_result(int new_affected_card_controller, int new_affected_card, event_t event, int initial_event_result);	// dispatch_event() forces the initial value of event_result to 0; this lets you set it arbitrarily.
int dispatch_event_with_attacker(int new_affected_card_controller, int new_affected_card, event_t event, int new_attacking_card_controller, int new_attacking_card);	// dispatch_event() forces attacking_card_controller to 1-player and attacking_card to -1; this lets you set them explicitly.  Returns the value of event_result as set by the cards' functions (before restoring event_result's previous value).
int dispatch_event_with_attacker_to_one_card(int new_affected_card_controller, int new_affected_card, event_t event, int new_attacking_card_controller, int new_attacking_card);	// Identical to dispatch_event_with_attacker(), but only sends the event to (new_affeced_card_controller,new_affected_card).  Differs from a bare call_card_function() in that it preserves affected_card_controller, affected_card, trigger_condition, event_result, etc.  Returns the value of event_result as set by the card function (before restoring event_result's previous value).
int dispatch_event_arbitrary_to_one_card(int player, int card, event_t event, int new_affected_card_controller, int new_affected_card, int new_attacking_card_controller, int new_attacking_card);	// Identical to dispatch_event_with_attacker_to_one_card(), but sends the event to an arbitrary (player, card) pair which can differ from (affected_card_controller, affected_card).
int dispatch_event_to_single_card_overriding_function(int player, int card, event_t event, int iid);	// Properly stashes globals then calls an arbitrary iid's card function for player/card (instead of that card's own one).  Returns what the function does.
int dispatch_trigger(int player, trigger_t trig, const char *prompt, int TENTATIVE_allow_response);	// Only sends the trigger with player == reason_for_trigger_controller.  Available only for backwards compatibility with exe handlers.  Avoid.
int dispatch_trigger2(int player, trigger_t trig, const char *prompt, int TENTATIVE_allow_response, int new_trigger_cause_controller, int new_trigger_cause);
int dispatch_xtrigger2(int player, xtrigger_t xtrig, const char *prompt, int TENTATIVE_allow_response, int new_trigger_cause_controller, int new_trigger_cause);
int get_color_from_remainder_text(int);
void damage_target_creature_or_player(int, int, int, int, int);
void put_into_play_aura_attached_to_target(int, int, int, int);
void pendrell_effect(int player, int card, event_t event, kill_t kill_mode);
void true_transform(int player, int card);
int this_dies_trigger(int player, int card, event_t event, resolve_trigger_t trigger_mode);
int this_dies_trigger_for_owner(int player, int card, event_t event, resolve_trigger_t trigger_mode);	// Same as this_dies_trigger(), but the card's owner, not its controller, gets the legacy effect if card is stolen as it dies.  In particular, when this returns true, player will be the owner since it's the legacy effect, not the original card, at that point.
int attached_creature_dies_trigger(int player, int card, event_t event, resolve_trigger_t trigger_mode);
int attached_creature_dies_trigger_for_controller(int player, int card, event_t event, resolve_trigger_t trigger_mode);	// controller of attached creature, not of {player,card}, gets the trigger
int find_in_owners_graveyard(int player, int card, int* owner, int* position);	// Given a (player,card) pair of a card that has left the battlefield, searches its owner's graveyard for the underlying card.  If found, return nonzero and set *owner and *position.
int exile_from_owners_graveyard(int player, int card);	// Given a (player,card) pair of a card that has left the battlefield, searches its owner's graveyard for the underlying card.  If found, exile it and return nonzero.
int exile_permanent_and_remember_it(int player, int card, int, int, int);
int quirion_ranger_ability(int player, int card, event_t event, target_definition_t* td_bounce, const char* prompt);
void type_uncounterable(int player, int card, event_t event, int t_player, int type, test_definition_t *this_test /*optional*/);
int my_damage_cannot_be_prevented(int player, int card, event_t event);
int damage_cannot_be_prevented_until_eot(int player, int card);
int can_ante_top_card_of_library(int player);
int ante_top_card_of_library(int player);
int has_type_dead_this_turn_in_grave(int player, test_definition_t *this_test);
int good_to_put_in_grave(int player, int card);
int check_card_for_rules_engine(int);
int is_artifact_creature_by_internal_id(int iid, int unused1, int unused2, int unused3);
int get_global_color_hack(int player);
void global_type_change(int player, int card, event_t event, int, int, test_definition_t *this_test, int, int, int, int, int);
void untap_only_1_permanent_per_upkeep(int player, int card, event_t event, int, int);
int can_cast_reaper_king(int player, int card, event_t event);
int casting_reaper_king(int player, int card, event_t event);
int get_color_for_monocolored_hybrid(int player, int card);
int get_number_of_monocolored_hybrid_mana(int player, int card, int);
int has_mana_for_monocolor_hybrid(int player, int, int, int);
int charge_mana_for_monocolor_hybrid(int player, int card, int, int, int);
char* type_text(char* buf, int maxlen, type_t type, type_text_t flags);	// Translates type into text, writing at most maxlen characters into buf (including terminating NUL).  Returns buf.
int switch_power_and_toughness_until_eot(int player, int card, int t_player, int t_card);
int ivory_gargoyle_impl(int player, int card, event_t event, int x, int b, int u, int g, int r, int w);
int pestilence_impl(int player, int card, event_t event, int sac_at_eot_if_no_creatures, color_t color);
int shrouded_lore_impl(int player, int card, event_t event, color_t color);
int thirst_impl(int player, int card, event_t event, int x, int b, int u, int g, int r, int w);
int generic_creature_with_bestow(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white, int pow, int tou, int key, int s_key);
int creature_with_targeted_bestow(int player, int card, event_t event, target_definition_t *td, int colorless, int black, int blue, int green, int red, int white, int pow, int tou, int key, int s_key);
void generic_creature_with_devotion(int player, int card, event_t event, int dev_color1, int dev_color2, int amount);	// Colors are sleighted internally.
int heroic(int player, int card, event_t event);
int heroic_mode(int player, int card, event_t event, resolve_trigger_t trigger_mode);	// Same as heroic, but not necessarily RESOLVE_TRIGGER_MANDATORY.
int modular(int player, int card, event_t event, int initial_counters);
int echoing_pump(int player, int card, event_t event, int p_plus, int t_plus);
void twiddle(int player, int card, int tgtnum);	// Resolve a Twiddle-like effect.
void ai_modifier_twiddle(int player, int card, int tgtnum);	// Change ai_modifier for a Twiddle-like effect.  Should happen in EVENT_ACTIVATE or EVENT_CAST_SPELL after targetting.
card_instance_t* combat_damage_being_prevented(event_t event);	// Returns a damage card during EVENT_PREVENT_DAMAGE if it's combat damage and doing more than 0.
card_instance_t* noncombat_damage_being_prevented(event_t event);	// Returns a damage card during EVENT_PREVENT_DAMAGE if it's noncombat damage and doing more than 0.
card_instance_t* damage_being_prevented(event_t event);	// Returns a damage card during EVENT_PREVENT_DAMAGE if it's doing more than 0.
card_instance_t* combat_damage_being_dealt(event_t event);	// Returns a damage card during EVENT_DEAL_DAMAGE if it's combat damage and doing more than 0.  damage->targets[16].player will hold the damage dealt if it's determinable, even in the case of wither/infect damage; this is no more robust than the previous workarounds were.
card_instance_t* noncombat_damage_being_dealt(event_t event);	// Returns a damage card during EVENT_DEAL_DAMAGE if it's noncombat damage and doing more than 0.  damage->targets[16].player will hold the damage dealt if it's determinable, even in the case of wither/infect damage; this is no more robust than the previous workarounds were.
card_instance_t* damage_being_dealt(event_t event);	// Returns a damage card during EVENT_DEAL_DAMAGE if it's doing more than 0.  damage->targets[16].player will hold the damage dealt if it's determinable, even in the case of wither/infect damage; this is no more robust than the previous workarounds were.
int damage_is_to_planeswalker(card_instance_t* damage);	// Returns nonzero if this damage card is being dealt to a planeswalker.
int reveal_top_cards_of_library(int player, int num_cards);	// [player] reveals the top [num_cards] of his library.
int reveal_top_cards_of_library_and_choose(int src_player, int src_card, int t_player, int num_cards, int must_select, tutor_t destination_chosen, int reveal_chosen, tutor_t destination_rest, int reveal_rest, test_definition_t* test);	// [reveal_rest or Look at] the top [num_cards] of your library.  Choose a [test] card from among them, [reveal_chosen], and move it to [destination_chosen].  Move the rest to [destination_rest].
int reveal_top_cards_of_library_and_choose_type(int src_player, int src_card, int t_player, int num_cards, int must_select, tutor_t destination_chosen, int reveal_chosen, tutor_t destination_rest, int reveal_rest, type_t legal_types);	// Identical to reveal_top_cards_of_library_and_choose(), but specify a type bitfield instead of a full test_definition_t.  Constructs an appropriate prompt.
void oath_of_lim_dul_lifeloss_trigger(int player, int card, int amount);
void declare_mana_available_maybe_hex(int player, color_test_t colors, int amount);	// Forwards to either declare_mana_available() or declare_mana_available_hex(), depending on whether more than one bit is set in colors.  declare_mana_available() is always preferable to declare_mana_available_hex(), which has limited slots and is much slower in has_mana(), charge_mana(), etc.  Calling declare_mana_available() directly instead of this is preferable too when the combination of colors is known.
void declare_mana_available_any_combination_of_colors(int player, color_test_t available, int amount);
void block(int player, int card, int player_to_block, int card_to_block);	// Makes player/card block player_to_block/card_to_block.  Doesn't check legality.
void remove_from_combat(int player, int card);
int get_base_power(int player, int card);
int get_base_power_iid(int owner, int iid);
int get_base_toughness(int player, int card);
int get_base_toughness_iid(int owner, int iid);
void holy_nimbus(int player, int card, event_t event, int cost);
void recalculate_all_cards_in_play(void);	// *SLOW*.  Make sure it's really needed before considering calling.
int unlimited_allowed_in_deck(int csvid);
int can_play_cards_as_though_they_had_flash(int player, int card, event_t event, target_definition_t* td, const char* prompt, int literal);
int enable_flash_land(int player, int card, event_t event, int x, int b, int u, int g, int r, int w, int required_type, int illegal_type, const char* prompt, int literal);
void allow_response_to_activation(int player, int card);
int effect_put_into_a_graveyard_this_way(int player, int card, event_t event);	// See effect_werewolf_ransacker() in dark_ascension.c for intended usage.
int is_monocolor_hybrid(card_ptr_t* cp);
int is_normal_hybrid(card_ptr_t* cp);
int is_mixed_hybrid(card_ptr_t* cp);
void create_spell_has_flashback_legacy(int player, int card, int graveyard_pos, flashback_legacy_t mode);
void increment_storm_count(int player, int mode);
int copy_spell_from_stack(int player, int t_player, int t_card);
void if_a_card_would_be_put_into_graveyard_from_play_exile_it_instead(int player, int card, int event, int t_player, test_definition_t* test /*optional*/);	// If test is NULL, then it defaults to all non-token cards.  Only needs to be called (and thus test only needs to be set) during EVENT_GRAVEYARD_FROM_PLAY.
int if_a_card_would_be_put_into_graveyard_from_library_do_something_instead(int player, int card, int event, int t_player, resolve_trigger_t trigger_mode, test_definition_t* test /*optional*/);	// Must set replace_milled as the very last thing after this returns true if anything is done with the card.  If test is NULL, then it defaults to all cards.  Only needs to be called (and thus test only needs to be set) if xtrigger_condition() == XTRIGGER_REPLACE_MILL.
int if_a_card_would_be_put_into_graveyard_from_anywhere_but_library_do_something_instead(int player, int card, int event, int t_player, resolve_trigger_t trigger_mode, test_definition_t* test /*optional*/);	// Must set replace_milled as the very last thing after this returns true if anything is done with the card.  If test is NULL, then it defaults to all cards.  Only needs to be called (and thus test only needs to be set) if xtrigger_condition() == XTRIGGER_REPLACE_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY.
int when_a_card_is_put_into_graveyard_from_anywhere_but_library_do_something(int player, int card, int event, int t_player, resolve_trigger_t trigger_mode, test_definition_t* test /*optional*/);	// A front end to XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY.  trigger_cause_controller (whose graveyard), trigger_cause (iid of card), gy_from_anywhere_pos (tentative position), and gy_from_anywhere_source (canonical graveyard_source[][] value) are all still valid.  Only needs to be called (and thus test only needs to be set) if xtrigger_condition() == XTRIGGER_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY.
void if_a_card_would_be_put_into_graveyard_from_anywhere_exile_it_instead(int player, int card, int event, int t_player, test_definition_t* test /*optional*/);	// A fairly common case.  If test is NULL, then it defaults to all non-token cards.  Only needs to be called (and thus test only needs to be set) during EVENT_GRAVEYARD_FROM_PLAY, XTRIGGER_REPLACE_CARD_TO_GY_FROM_ANYWHERE_BUT_LIBRARY, and XTRIGGER_REPLACE_MILL.
int transmute(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white, int my_cmc);
int spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand(int player, int card, event_t event, int max_cards, test_definition_t* test, int dont_kill_self);	// test only needs to be defined during EVENT_CAST_SPELL.
int spell_return_one_or_two_cards_from_gy_to_hand(int player, int card, event_t event, type_t type1, type_t type2);	// Choose one or both - Return target [type1] card from your graveyard to your hand; and/or return target [type2] card from your graveyard to your hand.  Not really appropriate if type1 == type2, for which you want spell_return_up_to_n_target_cards_from_your_graveyard_to_your_hand().
int create_may_play_card_from_exile_effect(int player, int card, int whose_exile_zone, int csvid, mpcfe_mode_t mode);	// If MPCFE_FACE_DOWN is set, the card should be obliterated instead of being put in to exile; otherwise, it should already be in exile before calling this.
int resolve_timetwister(int player, int card, event_t event);	// Does everything Timetwister does except kill the card at resolution.
int find_in_graveyard_by_source(int player, int source, int position_hint);	// Finds the card in player's graveyard whose graveyard_source[][] value is source, and returns its position or -1 if not found.  If position_hint is given, check there first.
int can_play_iid(int player, int event, int iid);
int can_activate_to_play_cards_from_graveyard(int player, int card, event_t event, type_t types);
int northern_southern_paladin(int player, int card, event_t event, color_t color);
int monstrosity(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white, int c_amount);
int is_monstrous(int player, int card);
void cannot_attack_alone(int player, int card, event_t event);
void cannot_block_alone(int player, int card, event_t event);
int exile_permanent_and_auras_attached(int player, int card, int t_player, int t_card, unsigned int mode);
int block_if_able(int player, int card, event_t event);
void force_activation_for_mana(int player, int card, color_test_t colors);	// Note that this doesn't check EVENT_CAN_ACTIVATE itself, or whether {player,card} has EA_MANA_SOURCE.
int inspired(int player, int card, event_t event);
int nightveil_specter_like_ability(int player, int card, event_t event, unsigned int flags, unsigned int cards_to_exile, unsigned int csvid_to_display);
void prevent_all_my_damage(int player, int card, event_t event, int flags);
void put_iid_under_the_first_x_cards_of_library(int player, int iid, int pos);
void put_permanent_under_the_first_x_card_of_its_owners_library(int player, int card, int pos);
int action_on_card(int player, int card, int p, int c, actions_t raw_action);
int action_on_target(int player, int card, unsigned int target_number, actions_t raw_action);
int manipulate_auras_enchanting_target(int player, int card, int t_player, int t_card, test_definition_t *this_test, actions_t action);
int aura_with_variable_pt_boost_depending_on_condition(int player, int card, event_t event, target_definition_t *td, test_definition_t *this_test, int pow, int tou, int pow2, int tou2);
int select_a_subtype_full_choice(int player, int card, int t_player, const int *zone, int show_legacy);
void init_subtype_text(void);
const char* raw_get_subtype_text(subtype_t subtype);	// Returns text corresponding to subtype.  Safely usable in display functions and targeting, since it doesn't call load_text().
const char* get_subtype_text(const char* fmt, subtype_t subtype);	// Any occurrence of %s or %a in fmt will be replaced by raw_get_subtype_text(subtype).  %a will also insert a "a " or "an " before the subtype's text as appropriate.  Returns a pointer to a static buffer.
int count_zone(int player, const int* zone);
int select_card_to_play_from_zone(int player, int card, event_t event, const int *zone, test_definition_t *test, int mode);
int card_drawn_trigger(int player, int card, event_t event, int who_draws, resolve_trigger_t trigger_mode);
void cannot_be_enchanted(int player, int card, event_t event);
int calc_initial_attack_rating_by_iid(int player, int iid);
void change_lands_into_new_land_type(int player, int card, event_t event, int orig_basic_land_type, int orig_basic_land_type_flag, int basic_land_type_to_become);
int land_into_new_basic_land_type(int internal_card_id_of_land_to_change, int basic_land_type_to_become);
void landhome(int player, int card, event_t event, subtype_t subtype);
int generic_suspend_legacy(int player, int card, event_t event);
int check_for_guardian_beast_protection(int t_player, int t_card);
void untap_only_one_permanent_type(int player, int card, event_t event, int affected_player, int type);
int basic_landtype_killer(int player, int card, event_t event, int basic_land_type);
void abyss_trigger(int player, int card, event_t event);	// At the beginning of each player's upkeep, destroy target nonartifact creature that player controls of his or her choice. It can't be regenerated.
void landwalk_disabling_card(int player, int card, event_t event, player_bits_t landwalk_disabled);
int play_card_in_exile_for_free_commander(int player, int t_player, int csvid, int mana_paid_for_this);
void real_counter_a_spell(int player, int card, int to_counter_player, int to_counter_card);
int manage_counterspell_linked_hacks(int player, int card, int to_counter_player, int to_counter_card);
int beginning_of_combat(int player, int card, event_t event, int aff_player, int aff_card);
int already_dead(int player, int card);
int get_cost_for_propaganda_like_effect(int player);
void uthden_creature(int player, int card, event_t event, int land_color);
int get_average_cmc(int player);
int generic_animating_aura(int player, int card, event_t event, target_definition_t *td, const char *prompt, int pow, int tou, int key, int s_key);
int generic_animating_aura_custom_prompt(int player, int card, event_t event, target_definition_t *td, const char *prompt, int pow, int tou, int key, int s_key);
int attach_aura_to_target(int aura_player, int aura_card, event_t event, int t_player, int t_card);
void volver(int player, int card, event_t event, int colorless, int black, int blue, int green, int red, int white,
			int colorless2, int black2, int blue2, int green2, int red2, int white2, int kicker1_priority, int kicker2_priority);
int generic_split_card(int player, int card, event_t event, int can_play_first_half, int priority_first_half,
						int colorless2, int black2, int blue2, int green2, int red2, int white2, int can_play_second_half, int priority_second_half,
						int can_be_fused, const char *name_first_half, const char *name_second_half);

void permanents_enters_battlefield_tapped(int player, int card, event_t event, int t_player, int type, test_definition_t *test);
int prevent_all_damage_dealt_by_target(int player, int card, int t_player, int t_card, int ddbm_flags);
int generic_stealing_aura(int player, int card, event_t event, target_definition_t *td, const char *prompt);
int sliver_with_shared_shade_ability(int player, int card, event_t event, int pow, int tou, int key, int s_key);
int shared_sliver_activated_ability(int player, int card, event_t event, int mode, int cless, int black, int blue, int green, int red, int white,
									uint32_t variable_costs, target_definition_t *td2, const char *prompt);
const char* sliver_to_regenerate(int who_chooses, int player, int card);
const char* activating_sliver(int who_chooses, int player, int card);
int aura_attached_to_me(int player, int card, event_t event, resolve_trigger_t trigger_mode, test_definition_t *test2);
int karoo(int player, int card, event_t event, int color1, int color2, int ai_card);
int gain_control_and_attach_as_aura(int player, int card, event_t event, int t_player, int t_card);
int persist_granted(int player, int card, event_t event);
void modify_cost_for_delve(int player, int card, event_t event);
int delving(int player, int card);
int cast_spell_with_delve(int player, int card);
int delve(int player, int card, event_t event);
void whenever_a_card_is_put_into_a_graveyard_from_anywhere_exile_that_card(int player, int card, event_t event, int t_player, int type, test_definition_t *this_test);
const char* target_is_monocolored(int who_chooses, int player, int card);
int is_plane_card(int id);
void immortal_enchantment(int player, int card, event_t event);
void pump_ability_by_test(int player, int card, int t_player, pump_ability_t *pump, test_definition_t *this_test);
void for_each_creature_damaged_by_me(int player, int card, event_t event, int flags, void (*fn)(int, int, int, int), int arg1, int arg2);
void lowland_basilisk_effect(int player, int card, int t_player, int t_card);
int legacy_effect_pump_ability_until_eot(int player, int card, event_t event );
int real_set_pt(int player, int card, int t_player, int t_card, int pow, int tou, int mode);
int set_pt_legacy(int player, int card, event_t event);
int humility_legacy(int player, int card, event_t event);
void disable_other_pt_setting_effects_attached_to_me(int t_player, int t_card);
unsigned int round_down_value(unsigned int orig_value);
unsigned int round_up_value(unsigned int orig_value);
void no_more_than_x_creatures_can_attack(int player, int card, event_t event, int t_player, int max_value);
void no_more_than_x_creatures_can_block(int player, int card, event_t event, int t_player, int max_value);
int us_cycling(int player, int card, event_t event); // AKA cycling(player, card, event, MANACOST_X(2))
void tutor_multiple_card_from_hand(int who_chooses, int t_player, int who_gets_the_card, int destination, int must_select, int ai_selection_mode, test_definition_t *this_test);
int prowess_mode(int player, int card, event_t event, int mode) __attribute__((warn_unused_result));
int prowess(int player, int card, event_t event);
int ferocious(int player, int card);
const char* is_face_down_creature(int who_chooses, int player, int card);
int can_block(int player, int card);
void select_blocker(int player, int card, int player_who_chooses_blockers);
void when_enchanted_permanent_dies_return_aura_to_hand(int player, int card, event_t event);
void bolster(int player, int card, int num_counters);
void dash(int player, int card, event_t event, int cless, int black, int blue, int green, int red, int white);
const char* is_multicolored(int who_chooses, int player, int card);	// targeting lambda
int fx_menace(int player, int card, event_t event);
void minimize_nondraining_mana(void);
void mana_doesnt_drain_until_eot(int player, color_t color, int amt);	// A specific amount of a specific color of mana already in player's mana pool doesn't drain until end of turn.  Unlike MANADRAIN_DOESNT_DRAIN and MANADRAIN_BECOMES_COLORLESS, this should be called immediately after adding the mana to the pool, not during EVENT_MANA_POOL_DRAINING.
void new_timestamp(int player, int card);	// Resets {player,card}'s timestamp to now.
void store_num_colors_paid_in_info_slot(int player, int card, event_t event);	// Sets {player,card}->info_slot to the number of different colors of mana paid to cast during EVENT_CAST_SPELL.  Must have the Modifies Casting Cost bit set.
int whenever_a_player_sacrifices_a_permanent(int player, int card, event_t event, int who_sacs, type_t typ, resolve_trigger_t mode);	// Uses {player, card}->targets[11].player.
card_instance_t* legacy_permanents_destroyed_this_way_accumulate(int src_player, int src_card, card_instance_t* previous_effect, int tgt_player, int tgt_card);	// Call once for each target.  previous_effect should be NULL on the first call, else the return value of the previous call to this function.  When the last target is destroyed (not counting ones that stop being destroyed for whatever reason), then the original card's function will be sent EVENT_RESOLVE_GENERAL_EFFECT, with player/card set to one of the effect cards created.  The number of targets that were destroyed will be in HIWORD(effect->eot_toughness).  The number of those that were controlled by player 0 will be in LOWORD(effect->targets[10].player), and the number that were controlled by player 1 will be in HIWORD(effect->targets[10].player).

/* Cards in already coded, for re-utilization purposes */
int card_aladdin(int, int, event_t);
int card_animate_artifact(int, int, event_t);
int card_aven_fisher(int, int, event_t);
int card_aven_flock(int, int, event_t);
int card_ball_lightning(int, int, event_t);
int card_black_lotus(int, int, event_t);
int card_boomerang(int, int, event_t);
int card_braingeyser_exe(int, int, event_t);
int card_burning_sands(int player, int card, event_t event);
int card_coercion(int, int, event_t);
int card_consecrate_land(int, int, event_t);
int card_control_magic(int, int, event_t);
int card_counterspell(int, int, event_t);
int card_crusade(int, int, event_t);
int card_drudge_skeletons(int, int, event_t);
int card_evil_presence(int, int, event_t);
int card_firebreathing(int, int, event_t);
int card_fog(int, int, event_t);
int card_glorious_anthem(int, int, event_t);
int card_hurr_jackal(int, int, event_t);
int card_hypnotic_specter(int, int, event_t);
int card_jokulhaups(int, int, event_t);
int card_last_gasp(int, int, event_t);
int card_lich_exe(int, int, event_t);
int card_lightning_blast(int, int, event_t);
int card_lightning_bolt(int, int, event_t);
int card_lim_duls_high_guard(int, int, event_t);
int card_living_lands(int, int, event_t);
int card_magical_hack(int, int, event_t);
int card_mana_flare(int, int, event_t);
int card_mana_leak(int, int, event_t);
int card_manta_riders(int, int, event_t);
int card_master_decoy(int, int, event_t);
int card_memory_lapse(int, int, event_t);
int card_nantuko_shade(int player, int card, event_t event);
int card_orcish_oriflamme(int, int, event_t);
int card_pacifism(int, int, event_t);
int card_phyrexian_arena(int, int, event_t);
int card_pit_scorpion(int, int, event_t);
int card_prodigal_sorcerer(int, int, event_t);
int card_pyroclasm(int, int, event_t);
int card_raise_dead(int, int, event_t);
int card_rampant_growth(int, int, event_t);
int card_regeneration(int, int, event_t);
int card_remove_soul(int, int, event_t);
int card_rod_of_ruin(int, int, event_t);
int card_ring_of_maruf(int, int, event_t);
int card_samite_healer(int, int, event_t);
int card_shivan_dragon(int, int, event_t);
int card_shock(int, int, event_t);
int card_sleight_of_mind(int, int, event_t);
int card_storm_seeker(int, int, event_t);
int card_sylvan_scrying(int, int, event_t);
int card_tawnoss_weaponry(int, int, event_t);
int card_the_rack(int, int, event_t);
int card_time_walk_exe(int, int, event_t);
int card_unsummon(int, int, event_t);
int card_volcanic_hammer(int, int, event_t);
int card_warriors_honor(int, int, event_t);
int card_wrath_of_god(int, int, event_t);
int card_zombie_master_exe(int, int, event_t);
int card_ancestral_recall_exe(int, int, event_t);
int card_swords_to_plowshares_exe(int, int, event_t);
int card_sengir_vampire(int, int, event_t);
int card_daring_apprentice(int, int, event_t);
int card_timetwister(int, int, event_t);
int card_xenic_poltergeist(int, int, event_t);
int card_nekrataal(int, int, event_t);
int card_eladamris_call(int, int, event_t);
int card_animate_dead(int, int, event_t);
int card_death_ward(int, int, event_t);
int card_gilded_lotus(int, int, event_t);
int card_guardian_angel_exe(int, int, event_t);
int card_river_boa(int, int, event_t);
int card_ghazban_ogre(int, int, event_t);
int card_sol_ring(int, int, event_t);
int card_icy_manipulator(int, int, event_t);
int card_mogg_fanatic(int, int, event_t);
int card_rabid_wombat(int, int, event_t);
int card_coastal_piracy(int, int, event_t);
int card_strip_mine(int, int, event_t);
int card_relic_bind(int, int, event_t);
int card_confiscate(int, int, event_t);
int card_flash_counter(int, int, event_t);
int card_steal_artifact(int, int, event_t);
int card_fireball_exe(int, int, event_t);
int card_disintegrate_exe(int, int, event_t);
int card_spell_blast_exe(int, int, event_t);
int card_power_sink_exe(int, int, event_t);
int card_detonate_exe(int, int, event_t);
int card_whimsy_exe(int, int, event_t);
int card_word_of_binding_exe(int, int, event_t);
int card_winter_blast_exe(int, int, event_t);
int card_alabaster_potion_exe(int, int, event_t);
int card_farmstead_exe(int, int, event_t);
int card_winter_orb(int, int, event_t);
int card_balance(int, int, event_t);
int card_merfolk_looter(int, int, event_t);
int card_kormus_bell(int, int, event_t);
int card_monstrous_growth(int, int, event_t);
int card_shatter(int, int, event_t);
int card_assembly_worker_exe(int, int, event_t);
int card_arena(int player, int card, event_t event);
int card_arrest(int player, int card, event_t event);
int card_basking_rootwalla(int, int, event_t);
int card_benevolent_unicorn(int player, int card, event_t event);
int card_cabal_coffers(int player, int card, event_t event);
int card_call_to_mind(int, int, event_t);
int card_day_of_judgment(int player, int card, event_t event);
int card_deathgazer(int player, int card, event_t event);
int card_demonic_tutor(int, int, event_t);
int card_diabolic_edict(int, int, event_t);
int card_dwarven_miner(int player, int card, event_t event);
int card_endless_cockroaches(int player, int card, event_t event);
int card_falkenrath_noble(int player, int card, event_t event);
int card_force_spike(int player, int card, event_t event);
int card_generic_noncombat_1_mana_producing_creature(int player, int card, event_t event);
int card_ghitu_war_cry(int player, int card, event_t event);
int card_goblin_shortcutter(int player, int card, event_t event);
int card_guardian_angel(int player, int card, event_t event );
int card_gush(int player, int card, event_t event);
int card_hermetic_study(int player, int card, event_t event);
int card_intimidation(int player, int card, event_t event );
int card_ivy_elemental(int player, int card, event_t event);
int card_journey_to_nowhere(int player, int card, event_t event);
int card_lowland_basilisk(int player, int card, event_t event);
int card_man_o_war(int player, int card, event_t event);
int card_master_of_diversion(int player, int card, event_t event);
int card_maze_of_ith(int player, int card, event_t event);
int card_memory_jar(int player, int card, event_t event);
int card_monk_realist(int player, int card, event_t event);
int card_negate(int player, int card, event_t event);
int card_no_mercy(int player, int card, event_t event);
int card_nomads_en_kor(int player, int card, event_t event);
int card_ogre_arsonist(int player, int card, event_t event);
int card_orchard_spirit(int player, int card, event_t event );
int true_phyrexian_negator(int player, int card, event_t event);
int card_pillar_of_flame(int player, int card, event_t event);
int card_quirion_ranger(int player, int card, event_t event);
int card_saber_ants(int player, int card, event_t event);
int card_skulking_ghost(int player, int card, event_t event);
int card_slith_predator(int player, int card, event_t event);
int card_spellbook(int player, int card, event_t event);
int card_spiketail_hatchling(int player, int card, event_t event);
int card_stealer_of_secrets(int player, int card, event_t event);
int card_thorn_elemental(int player, int card, event_t event);
int card_thorn_of_amethyst(int, int, event_t);
int card_thundermare(int player, int card, event_t event);
int card_time_walk(int, int, event_t);
int card_uktabi_orangutan(int player, int card, event_t event);
int card_urborg_tomb_of_yawgmoth(int player, int card, event_t event);
int card_wrap_in_vigor(int player, int card, event_t event);
int card_yew_spirit(int player, int card, event_t event);
int card_furnace_of_rath(int player, int card, event_t event);
int card_timberland_guide(int player, int card, event_t event);
int card_choke(int player, int card, event_t event);
int card_heavy_ballista(int player, int card, event_t event);
int card_fellwar_stone(int player, int card, event_t event);
int card_smoke(int player, int card, event_t event);
int card_shocker(int player, int card, event_t event);
int card_elvish_mystic(int player, int card, event_t event);
int card_auramancer(int player, int card, event_t event);
int card_gravedigger(int player, int card, event_t event);

void backtrace_and_abort(const char* header);
#define ASSERT(cond) do { if (!(cond)) backtrace_and_abort(__FILE__ ":" STRINGIZE(__LINE__) "\n" #cond); } while (0)

// 6 args in a row is just too many to verify correctness at a glance.
#define MANACOST_X(x)		(x), 0, 0, 0, 0, 0
#define MANACOST0			MANACOST_X(0)

#define MANACOST_XB(x,b)	(x), (b), 0, 0, 0, 0
#define  MANACOST_B(b)		MANACOST_XB(0,b)
#define MANACOST_XU(x,u)	(x), 0, (u), 0, 0, 0
#define  MANACOST_U(u)		MANACOST_XU(0,u)
#define MANACOST_XG(x,g)	(x), 0, 0, (g), 0, 0
#define  MANACOST_G(g)		MANACOST_XG(0,g)
#define MANACOST_XR(x,r)	(x), 0, 0, 0, (r), 0
#define  MANACOST_R(r)		MANACOST_XR(0,r)
#define MANACOST_XW(x,w)	(x), 0, 0, 0, 0, (w)
#define  MANACOST_W(w)		MANACOST_XW(0,w)

#define MANACOST_XBU(x,b,u)	(x), (b), (u), 0, 0, 0
#define  MANACOST_BU(  b,u)	MANACOST_XBU(0,b,u)
#define MANACOST_XUB(x,u,b)	MANACOST_XBU(x,b,u)
#define  MANACOST_UB(  u,b)	MANACOST_XUB(0,u,b)
#define MANACOST_XBG(x,b,g)	(x), (b), 0, (g), 0, 0
#define  MANACOST_BG(  b,g)	MANACOST_XBG(0,b,g)
#define MANACOST_XGB(x,g,b)	MANACOST_XBG(x,b,g)
#define  MANACOST_GB(  g,b)	MANACOST_XGB(0,g,b)
#define MANACOST_XBR(x,b,r)	(x), (b), 0, 0, (r), 0
#define  MANACOST_BR(  b,r)	MANACOST_XBR(0,b,r)
#define MANACOST_XRB(x,r,b)	MANACOST_XBR(x,b,r)
#define  MANACOST_RB(  r,b)	MANACOST_XRB(0,r,b)
#define MANACOST_XBW(x,b,w)	(x), (b), 0, 0, 0, (w)
#define  MANACOST_BW(  b,w)	MANACOST_XBW(0,b,w)
#define MANACOST_XWB(x,w,b)	MANACOST_XBW(x,b,w)
#define  MANACOST_WB(  w,b)	MANACOST_XWB(0,w,b)
#define MANACOST_XUG(x,u,g)	(x), 0, (u), (g), 0, 0
#define  MANACOST_UG(  u,g)	MANACOST_XUG(0,u,g)
#define MANACOST_XGU(x,g,u)	MANACOST_XUG(x,u,g)
#define  MANACOST_GU(  g,u)	MANACOST_XGU(0,g,u)
#define MANACOST_XUR(x,u,r)	(x), 0, (u), 0, (r), 0
#define  MANACOST_UR(  u,r)	MANACOST_XUR(0,u,r)
#define MANACOST_XRU(x,r,u)	MANACOST_XUR(x,u,r)
#define  MANACOST_RU(  r,u)	MANACOST_XRU(0,r,u)
#define MANACOST_XUW(x,u,w)	(x), 0, (u), 0, 0, (w)
#define  MANACOST_UW(  u,w)	MANACOST_XUW(0,u,w)
#define MANACOST_XWU(x,w,u)	MANACOST_XUW(x,u,w)
#define  MANACOST_WU(  w,u)	MANACOST_XWU(0,w,u)
#define MANACOST_XGR(x,g,r)	(x), 0, 0, (g), (r), 0
#define  MANACOST_GR(  g,r)	MANACOST_XGR(0,g,r)
#define MANACOST_XRG(x,r,g)	MANACOST_XGR(x,g,r)
#define  MANACOST_RG(  r,g)	MANACOST_XRG(0,r,g)
#define MANACOST_XGW(x,g,w)	(x), 0, 0, (g), 0, (w)
#define  MANACOST_GW(  g,w)	MANACOST_XGW(0,g,w)
#define MANACOST_XWG(x,w,g)	MANACOST_XGW(x,g,w)
#define  MANACOST_WG(  w,g)	MANACOST_XWG(0,w,g)
#define MANACOST_XRW(x,r,w)	(x), 0, 0, 0, (r), (w)
#define  MANACOST_RW(  r,w)	MANACOST_XRW(0,r,w)
#define MANACOST_XWR(x,w,r)	MANACOST_XRW(x,r,w)
// Khans of Tarkir "shards"
#define  MANACOST_BRW(b,r,w)	0, (b), 0, 0, (r), (w)
#define  MANACOST_BGW(b,g,w)	0, (b), 0, (g), 0, (w)
#define  MANACOST_URW(u,r,w)	0, 0, (u), 0, (r), (w)
#define  MANACOST_BUG(b,u,g)	0, (b),(u), (g), 0, 0
#define  MANACOST_UGR(u, g, r)	0, 0,(u), (g), (r), 0
#define  MANACOST_XBRW(x, b,r,w)	(x), (b), 0, 0, (r), (w)
#define  MANACOST_XBGW(x, b,g,w)	(x), (b), 0, (g), 0, (w)
#define  MANACOST_XURW(x, u,r,w)	(x), 0, (u), 0, (r), (w)
#define  MANACOST_XBUG(x, b,u,g)	(x), (b),(u), (g), 0, 0
#define  MANACOST_XUGR(x, u, g, r)	(x), 0,(u), (g), (r), 0

// Needed when a macro expects a single MANACOST macro as an argument.
#define MANACOST6(x,b,u,g,r,w) (x),(b),(u),(g),(r),(w)

// If color is an unknown color_t:
#define MANACOST_CLR(clr,amt)	(clr) == COLOR_COLORLESS ? (amt) : 0,	(clr) == COLOR_BLACK ? (amt) : 0,	(clr) == COLOR_BLUE ? (amt) : 0,	\
								(clr) == COLOR_GREEN ? (amt) : 0,		(clr) == COLOR_RED ? (amt) : 0,		(clr) == COLOR_WHITE ? (amt) : 0

// These checks are used *everywhere* and are far too verbose.
// The arguments are deliberately not parenthesized, so they're more likely to elicit compilation errors if misused.
#define CAN_ACTIVATE0(player, card)				(can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, MANACOST_X(0)))
#define CAN_ACTIVATE(player, card, manacost)	(can_use_activated_abilities(player, card) && has_mana_for_activated_ability(player, card, manacost))
#define CAN_TAP(player, card)					(!is_tapped(player, card) && !is_animated_and_sick(player, card))
#define CAN_TAP_FOR_MANA(player, card)			(CAN_TAP(player, card) && can_produce_mana(player, card))

#define IS_CASTING(player, card, event)	(event == EVENT_CAN_CAST || (event == EVENT_CAST_SPELL && affect_me(player, card)) || event == EVENT_RESOLVE_SPELL)
#define IS_ACTIVATING(event)			(event == EVENT_CAN_ACTIVATE || event == EVENT_ACTIVATE || event == EVENT_RESOLVE_ACTIVATION)
#define IS_ACTIVATING_FROM_HAND(event)	(event == EVENT_CAN_ACTIVATE_FROM_HAND || event == EVENT_ACTIVATE_FROM_HAND || event == EVENT_RESOLVE_ACTIVATION_FROM_HAND)
#define IS_CASTING_FROM_GRAVE(event)	(event == EVENT_GRAVEYARD_ABILITY || event == EVENT_PAY_FLASHBACK_COSTS)
#define IS_GAA_EVENT(event)				(IS_ACTIVATING(event) || event == EVENT_CLEANUP)
#define IS_GS_EVENT(player, card, event)		(IS_CASTING(player, card, event) || event == EVENT_CHANGE_TARGET || event == EVENT_CAN_CHANGE_TARGET)
#define IS_AURA_EVENT(player, card, event)		(IS_CASTING(player, card, event) || event == EVENT_CHANGE_TARGET || event == EVENT_CAN_CHANGE_TARGET || event == EVENT_CAN_MOVE_AURA || event == EVENT_MOVE_AURA || event == EVENT_RESOLVE_MOVING_AURA || event == EVENT_POWER || event == EVENT_TOUGHNESS || event == EVENT_CHANGE_TYPE || event == EVENT_ABILITIES || event == EVENT_SET_COLOR || event == EVENT_GET_SELECTED_CARD)
#define IS_DDBM_EVENT(event)			(event == EVENT_DEAL_DAMAGE || trigger_condition == TRIGGER_DEAL_DAMAGE)
#define IS_SHADE_EVENT(event)			(IS_ACTIVATING(event) || event == EVENT_POW_BOOST || event == EVENT_TOU_BOOST)

int vscnprintf(char *buf, size_t size, const char* fmt, va_list args);
int scnprintf(char * buf, size_t size, const char* fmt, ...);

#define DIALOG(player, card, event...)	dialog_impl(player, card, event, NULL, NULL, NULL, NULL)
  /* Usage:
   * DIALOG(player, card, event,
   *        [any number of options and their arguments]
   *        "Choice 1", legality_1, ai_priority_1,
   *        "Choice 2", legality_2, ai_priority_2,
   *        ...
   *        "Choice 74", legality_74, ai_priority_74);
   *
   * player/card should always be the player/card of the calling card, and event the calling event.
   *
   * Handles EVENT_CAN_CAST, EVENT_CAST_SPELL, EVENT_RESOLVE_SPELL;
   * and EVENT_CAN_ACTIVATE, EVENT_ACTIVATE, and EVENT_RESOLVE_ACTIVATION.
   * You most likely only want to use it for one of the two sets; so just calling DIALOG() at the start of a function without checking event is probably wrong.
   *
   * In EVENT_CAN_CAST and EVENT_CAN_ACTIVATE, this returns 1 if there's at least one legal option (and, if it's being chosen by the AI, its priority is 1 or
   * higher), or else 0.
   *
   * In EVENT_CAST_SPELL and EVENT_ACTIVATE, this displays the dialog, using the option with the highest AI priority for ai_choice.  It'll set spell_fizzled=1
   * and return 0 if no option is legal or the player chooses "Cancel" (which is added by default).  Otherwise, it returns the index of the chosen choice, with
   * the first choice being 1.  (It also stores the choice in BYTE0 of get_card_instance(player, card)->info_slot, so you shouldn't set anything else there; you
   * shouldn't need to inspect it directly, though.  Other bytes in info_slot are untouched, though BYTE1 will be used also if DLG_CHOOSE_TWO below is set.)
   *
   * In EVENT_RESOLVE_SPELL and EVENT_RESOLVE_ACTIVATION, just returns the choice given before (and stored in info_slot).  You could also inspect info_slot
   * directly, but be aware that bytes in it not specified as changing won't have changed - so you want instance->info_slot && 0x000000FF for the normal choice
   * and, if DLG_CHOOSE_TWO is given, (instance->info_slot & 0x0000FF00)>>8 for the second choice.  Calling this again is easier.
   *
   * Options that can be given in "[any number of options and their arguments]":
   *
   * DLG_RANDOM - If more than one choice with ai_priority > 0 is legal, then makes a weighted random choice among them.
   *              E.g., with ai_priority_1 == 5, ai_priority_2 = -1, ai_priority_3 = 2, ai_priority_4 = 7, there is a 5 out of 14 (5+2+7) chance of choosing
   *              option 1; a 2 out of 14 chance of choosing option 3, and a 7 out of 14 chance of choosing option 3.  Option 2 is never chosen, since its
   *              priority is 0 or below.
   *              If there are no legal choices with priority 1 or higher (and it got here anyway, instead of returning false in EVENT_CAN_CAST), always choose
   *              the highest priority.
   *              If an option has a priority of 1 000 000 or higher, always choose that one.  (It still makes sense to use DLG_RANDOM with such an option; it
   *              might not be legal, or DLG_CHOOSE_TWO below might also be given.)
   *              If this option isn't given, choose the choice with the highest ai_priority.
   *
   * DLG_RANDOM_NO_SAVE - As DLG_RANDOM, but doesn't save the choice during speculation.  Appropriate if the choice depends on a random event.
   *
   * DLG_NO_CANCEL - No "Cancel" option is added to the end.  Also allows the dialog to be displayed if there are no choices, just header text.
   *
   * DLG_OMIT_ILLEGAL - Don't display illegal options at all, instead of greying them out.  The returned value always corresponds to the options given as
   *                    arguments, even if some of them weren't displayed.  Probably shouldn't be set unless there's a specific reason not to display an option,
   *                    and instead add a setting in config.txt so end-users can choose the default on their own.
   *
   * DLG_CHOOSE_TWO(&second_choice) - After the first choice is made, make that choice illegal and prompt for a second choice.  Return its value in
   *                                  second_choice, and store it BYTE1 of info_slot.
   *
   * DLG_SORT_RESULTS - if DLG_CHOOSE_TWO is given and the second option chosen has a lower index than the first, swap them when returning the results.  For
   *                    example, with options "Choice_A", "Choice B", "Choice C", if the player picks "Choice C" then "Choice B", return 2 and put 3 in
   *                    second_choice, just as if he'd picked "Choice B" then "Choice C".
   *
   * DLG_FULLCARD(fullcard_player, fullcard_card) - show a specific card in the left half of the dialog.  If not given, this is player/card.
   *
   * DLG_FULLCARD_ID(internal_card_id) - Shows a card with the given internal card id for the fullcard, instead of an existing card in-play/in-hand/on-stack.
   *
   * DLG_FULLCARD_CSVID(csv_id) - Just like DLG_FULLCARD_ID, except use a csv id instead of an internal card id.  DLG_FULLCARD_ID is more efficient.
   *
   * DLG_SMALLCARD(smallcard_player, smallcard_card) - show a small card in the lower right corner of the dialog.  If not given, nothing is shown.
   *
   * DLG_SMALLCARD_ID(internal_card_id) - Shows a card with the given internal card id for the smallcard, instead of an existing card in-play/in-hand/on-stack.
   *
   * DLG_SMALLCARD_CSVID(csv_id) - Just like DLG_SMALLCARD_ID, except use a csv id instead of an internal card id.  DLG_SMALLCARD_ID is more efficient.
   *
   * DLG_WHO_CHOOSES(choosing_player) - choosing_player picks the choice instead of player.
   *
   * DLG_HEADER(header_txt) - Display header_txt before the choices.
   *
   * DLG_MSG(header_txt) - Equivalent to DLG_NO_STORAGE, DLG_NO_CANCEL, and DLG_HEADER(header_txt).
   *
   * DLG_NO_STORAGE - Always display the dialog in this call, as if event were EVENT_CAST_SPELL or EVENT_CAN_ACTIVATE.  (event must be still one of the six
   *                  normally-handled events, though.)  Don't store the result in (player,card)->info_slot, and don't set spell_fizzled=1 if cancelled.  Avoid
   *                  passing this after any choices.
   *
   * DLG_STORE_IN(location) - Store the choice in location, an unsigned char*, during EVENT_CAST_SPELL/EVENT_ACTIVATE instead of BYTE0(instance->info_slot); and
   *                          retrieve it from there during EVENT_RESOLVE_SPELL/EVENT_RESOLVE_ACTIVATION.  If DLG_CHOOSE_TWO() is also set, store its result in
   *                          ((unsigned char*)location) + 1.
   *
   * DLG_AUTOCHOOSE_IF_1 - If only one choice is legal, choose it automatically and don't display a dialog.  Ignored for the AI or if DLG_CHOOSE_TWO.
   *
   * DLG_NO_DISPLAY_FOR_AI - If who_chooses is the AI, don't display the dialog.
   *
   * DLG_PLANESWALKER - Add another int argument after each choice's ai_priority for the choice's loyalty cost.  If fewer than that many counters are on the
   *                    card, the choice is flagged illegal (in addition to the normal legality check); and planeswalker() will add or remove that many counters
   *                    if it's chosen and spell_fizzled != 1.  Stores in SBYTE1 of targets[9].player.
   *
   * DLG_MANA(MANACOST_*(...)) - This option is placed after a ["Text", legality, priority] triplet (or a ["Text", legality, priority, loyalty] quadruplet, if
   *                             DLG_PLANESWALKER is set).  In addition to the usual legality check, player must have the specified amount of mana available
   *                             during EVENT_CAN_CAST or EVENT_CAN_ACTIVATE for this choice to be legal; and in EVENT_CAST_SPELL or EVENT_ACTIVATE, this amount
   *                             is charged if this choice is picked.  Declining to pay the mana sets spell_fizzled = 1 and returns 0 from DIALOG().  For
   *                             EVENT_CAN_ACTIVATE and EVENT_ACTIVATE, (player,card) is passed to has_mana_for_activated_ability() /
   *                             charge_mana_for_activated_ability().  x_value will contain the amount of mana paid if one of the mana costs is below zero.
   *
   *                             If DLG_WHO_CHOOSES(choosing_player) is also set, it's still player's mana that's checked and charged, not choosing_player's.
   *
   *                             DLG_CHOOSE_TWO() is only partially supported, and it's probably a bad idea.  If set, mana requirements aren't added together to
   *                             see whether DIALOG() as a whole is legal during EVENT_CAN_CAST and EVENT_CAN_ACTIVATE, but they are for charging when the
   *                             choices have been made in EVENT_CAST_SPELL and EVENT_ACTIVATE.  It's also unsafe to use a <0 cost (to charge {X}), though might
   *                             be doable if exactly one option has an {X}.
   *
   * DLG_MANA6(x,b,u,g,r,w) - Just like DLG_MANA, but specifies the 6 mana costs explicitly instead of using a MANACOST macro.
   *
   * DLG_TARGET(td, prompt) - This option is placed after a ["Text", legality, priority] triplet (or a ["Text", legality, priority, loyalty] quadruplet, if
   *                          DLG_PLANESWALKER is set).  In addition to the usual legality check, can_target(td) must return true during EVENT_CAN_CAST or
   *                          EVENT_CAN_ACTIVATE for this choice to be legal; in EVENT_CAST_SPELL or EVENT_ACTIVATE, (player,card)'s number of targets is set to
   *                          0 and pick_target(td, prompt) is called, setting choice to 0 if targetting is cancelled; and in EVENT_RESOLVE_SPELL or
   *                          EVENT_RESOLVE_ACTIVATION, choice is set to 0 if valid_target(td) fails.
   *
   *                          Unless the loyalty cost for an option is set to 999, an activated mana cost will always be charged.  The amount is usually 0, of
   *                          course, but it can be overridden by DLG_MANA() or increased by activated ability cost increasers.
   *
   *                          If DLG_WHO_CHOOSES(choosing_playre) is also set, td.who_chooses should be manually set by the caller as well (if appropriate).
   *
   *                          This doesn't work with DLG_CHOOSE_TWO().
   *
   * DLG_LITERAL_TARGET(td, prompt) - As per DLG_TARGET, but prompt is used verbatim instead of loading from Text.res.
   *
   * DLG_TAP - Placed after an option triplet/quadruplet.  Calling card must be able to tap for this choice to be legal, and it does so during EVENT_ACTIVATE
   *           (or EVENT_CAST_SPELL, though this option makes little sense in that case).
   *
   * DLG_99 - Placed after an option triplet/quadruplet.  If this option is legal during EVENT_CAN_CAST/EVENT_CAN_ACTIVATE, DIALOG() returns 99 instead of 1.
   *          If DLG_CHOOSE_TWO is also set, then only one legal option needs DLG_99 for DIALOG() as a whole to return 99.
   *
   * DLG_NO_OP - Does nothing.  May be used as an alternative to another argumentless option, e.g. (should_tap ? DLG_TAP : DLG_NO_OP).
   */

// Implementation details - do not use directly
int dialog_impl(int player, int card, event_t event, ...);
typedef enum
{
#define DLGIMPL						"\x01"
  DLGIMPL_CH						=0x01,
#define DLG_RANDOM			DLGIMPL	"\x02"
  DLGIMPL_RANDOM_CH					=0x02,
#define DLG_NO_CANCEL		DLGIMPL	"\x03"
  DLGIMPL_NO_CANCEL_CH				=0x03,
#define DLG_OMIT_ILLEGAL	DLGIMPL	"\x04"
  DLGIMPL_OMIT_ILLEGAL_CH			=0x04,
#define DLG_CHOOSE_TWO(loc)	DLGIMPL	"\x05", (loc)
  DLGIMPL_CHOOSE_TWO_CH				=0x05,
#define DLG_FULLCARD(p,c)	DLGIMPL	"\x06", (p), (c)
  DLGIMPL_FULLCARD_CH				=0x06,
#define DLG_SMALLCARD(p,c)	DLGIMPL	"\x07", (p), (c)
  DLGIMPL_SMALLCARD_CH				=0x07,
#define DLG_WHO_CHOOSES(p)	DLGIMPL	"\x08", (p)
  DLGIMPL_WHO_CHOOSES_CH			=0x08,
#define DLG_HEADER(t)		DLGIMPL	"\x09", (t)
  DLGIMPL_HEADER_CH					=0x09,
#define DLG_MSG(t)			DLG_HEADER(t), DLG_NO_STORAGE, DLG_NO_CANCEL
#define DLG_FULLCARD_ID(i)	DLGIMPL	"\x0A", (i)
  DLGIMPL_FULLCARD_ID_CH			=0x0A,
#define DLG_FULLCARD_CSVID(i)	DLGIMPL	"\x0B", (i)
  DLGIMPL_FULLCARD_CSVID_CH			=0x0B,
#define DLG_NO_STORAGE		DLGIMPL	"\x0C"
  DLGIMPL_NO_STORAGE_CH				=0x0C,
#define DLG_SORT_RESULTS	DLGIMPL	"\x0D"
  DLGIMPL_SORT_RESULTS_CH			=0x0D,
#define DLG_AUTOCHOOSE_IF_1	DLGIMPL	"\x0E"
  DLGIMPL_AUTOCHOOSE_IF_1_CH		=0x0E,
#define DLG_PLANESWALKER	DLGIMPL	"\x0F"
  DLGIMPL_PLANESWALKER_CH			=0x0F,
#define DLG_MANA(x)			DLG_MANA6(x)
#define DLG_MANA6(x,b,u,g,r,w)	DLGIMPL	"\x10", (x), (b), (u), (g), (r), (w)
  DLGIMPL_MANA_CH					=0x10,
#define DLG_SMALLCARD_ID(i)	DLGIMPL	"\x11", (i)
  DLGIMPL_SMALLCARD_ID_CH			=0x11,
#define DLG_SMALLCARD_CSVID(i)	DLGIMPL	"\x12", (i)
  DLGIMPL_SMALLCARD_CSVID_CH		=0x12,
#define DLG_STORE_IN(p)		DLGIMPL	"\x13", (p)
  DLGIMPL_STORE_IN_CH				=0x13,
#define DLG_TARGET(td, prompt)	DLGIMPL	"\x14", (td), (prompt)
  DLGIMPL_TARGET_CH					=0x14,
#define DLG_TAP				DLGIMPL	"\x15"
  DLGIMPL_TAP_CH					=0x15,
#define DLG_NO_DISPLAY_FOR_AI	DLGIMPL	"\x16"
  DLGIMPL_NO_DISPLAY_FOR_AI_CH		=0x16,
#define DLG_NO_OP			DLGIMPL	"\x17"
  DLGIMPL_NO_OP_CH					=0x17,
#define DLG_LITERAL_TARGET(td, prompt)	DLGIMPL	"\x18", (td), (prompt)
  DLGIMPL_LITERAL_TARGET_CH			=0x18,
#define DLG_99				DLGIMPL	"\x19"
  DLGIMPL_99_CH						=0x19,
#define DLG_RANDOM_NO_SAVE	DLGIMPL	"\x1A"
  DLGIMPL_RANDOM_NO_SAVE_CH			=0x1A,
} DlgImplCh;

// C interface to std::set<void*>
typedef struct SetVoidptr_t* SetVoidptr;
int SetVoidptr_insert(SetVoidptr* s, void* val);	// Inserts val into the set described by s, creating the set first if s is NULL.  Returns nonzero if val was already in the set.

// C interface to std::sort
void sort_ArrayConstCharPtr(const char** arr, int num_elements);	// In-place textual sort on a num_elements-length array of const char*.

// C Interface to std::unordered_map<int, int>
typedef struct UnorderedMapIntInt_t* UnorderedMapIntInt;
int* UnorderedMapIntInt_fetch(UnorderedMapIntInt* m, int key);	// Returns a pointer to the value for key in the map described by m.  If key isn't in the map, returns NULL.
int* UnorderedMapIntInt_set(UnorderedMapIntInt* m, int key);	// Returns a pointer to the value for key in the map described by m, creating the map first if m is NULL.  If key isn't in the map, inserts it first.
void UnorderedMapIntInt_delete(UnorderedMapIntInt* m);	// Destroys the map described by m.

#ifdef __cplusplus
}
#endif

#endif
