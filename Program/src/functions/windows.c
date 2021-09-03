// -*- c-basic-offset:2 -*-
#include <windows.h>

#include "manalink.h"

int get_ability_image(int player, int card, int result);
void fetch_ability_tooltip_name(char* destination, int i);
void get_special_counters_name(char* dest, int csvid, int num);
void get_counter_name_by_type(char* dest, counter_t counter_type, int num);
int get_counter_type_by_instance(card_instance_t* instance);

static const int cardclass_offset_player = 0;
static const int cardclass_offset_card = 4;

#define hwnd_FullCardClass	((HWND)(EXE_DWORD(0x715ca0)))

LRESULT __stdcall wndproc_CardClass_hook(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam)
{
  switch (msg)
	{
	  case WM_COMMAND:	// Popup menu
		switch (wparam)
		  {
			case 0x72:	// Card name from Original Type submenu
			  {
				int player = GetWindowLongA(hwnd, cardclass_offset_player);
				int card = GetWindowLongA(hwnd, cardclass_offset_card);

				card_instance_t* instance = get_displayed_card_instance(player, card);
				int oici = instance->original_internal_card_id;
				int csvid = cards_data[oici].id;
				SendMessage(hwnd_FullCardClass, 0x401, csvid, 0);
			  }
			  return 0;

			case 0x264:	// Bury this card
			  if (debug_mode)
				{
				  int old_phasestopper = option_PhaseStoppers[current_turn][current_phase];
				  option_PhaseStoppers[current_turn][current_phase] &= ~1;

				  int player = GetWindowLongA(hwnd, cardclass_offset_player);
				  int card = GetWindowLongA(hwnd, cardclass_offset_card);

				  card_instance_t* instance = get_card_instance(player, card);
				  instance->token_status |= STATUS_OBLITERATED;	// prevents it from being put into graveyard

				  if (in_play(player, card) && is_what(player, card, TYPE_PERMANENT))
					kill_card(player, card, KILL_STATE_BASED_ACTION);	// Added
				  else
					kill_card(player, card, KILL_BURY);

				  option_PhaseStoppers[current_turn][current_phase] = old_phasestopper;
				}
			  return 0;
		  }
		break;	// And fall through to call to wndproc_CardClass() below

	  case 0x437:	// tooltips
		;char tooltip[100];
		tooltip[0] = 0;

		int player = GetWindowLongA(hwnd, cardclass_offset_player);
		int card = GetWindowLongA(hwnd, cardclass_offset_card);
		POINT pt;
		pt.x = LOWORD(lparam);
		pt.y = HIWORD(lparam);

		if (player != -1 && card == -1)
		  strcpy(tooltip, EXE_STR(0x6286f4));	// CUECARD_SMALLCARD[0] = Damage to player
		else
		  {
			RECT client_rect, rect;
			GetClientRect(hwnd, &client_rect);

			card_instance_t* instance = get_displayed_card_instance(player, card);

			if (instance->state & STATE_TAPPED)
			  {
				int v149 = pt.x;
				pt.x = pt.y;
				pt.y = client_rect.bottom - v149;
			  }

			int kwds = instance->regen_status;
			if ((kwds & KEYWORD_PROT_CREATURES) && instance->state & STATE_ATTACKING)
			  kwds |= KEYWORD_BANDING;
			if (kwds == -1)
			  kwds = 0;
			keyword_t displayable_keyword_bits = get_ability_image(player, card, kwds & 0x1FFFF);

			int i, ability_image = -1;
			if (displayable_keyword_bits)
			  for (i = 0; i < 32; ++i)
				{
				  keyword_t kw = i < 17 ? EXE_DWORD_PTR(0x5b84d4)[i] : 1 << i;	// displayed_keyword_order[]

				  EXE_FN(void, 0x4d43d0, RECT*, keyword_t, RECT*, keyword_t)(&rect, kw, &client_rect, displayable_keyword_bits);	// copy_rect_for_ability()

				  if (PtInRect(&rect, pt))
					{
					  ability_image = i;
					  break;
					}
				}

			int special_counters, damage;

			if ((instance->untap_status & 3) == 3
				&& (EXE_FN(void, 0x4d4c20, RECT*, RECT*)(&rect, &client_rect),	// copy_untap_status_rect()
					PtInRect(&rect, pt)))
			  strcpy(tooltip, EXE_STR(0x78f0cc));	// CUECARD_SMALLCARD[1] = This card will untap
			else if (ability_image != -1)
			  fetch_ability_tooltip_name(tooltip, ability_image);
			else if ((damage = instance->damage_on_card) > 0
					 && (EXE_FN(void, 0x4d3770, RECT*, RECT*)(&rect, &client_rect),	// copy_damage_rect()
						 PtInRect(&rect, pt)))
			  sprintf(tooltip, EXE_STR(0x786230), damage);	// CUECARD_SMALLCARD[2] = Damage: %d
			else if ((special_counters = instance->special_counters) > 0
					 && (EXE_FN(void, 0x4d3b60, RECT*, RECT*, int)(&rect, &client_rect, special_counters),	// copy_special_counter_rect()
						 PtInRect(&rect, pt)))
			  get_counter_name_by_type(tooltip, instance->special_counter_type, special_counters);
			else if (instance->counters2
					 && 90 * client_rect.bottom / 256 < pt.y && pt.y < 159 * client_rect.bottom / 256
					 && client_rect.left + 2 * (client_rect.right - client_rect.left) / 3 <= pt.x)
			  get_counter_name_by_type(tooltip, BYTE0(instance->targets[18].player), instance->counters2);
			else if (instance->counters3
					 && 90 * client_rect.bottom / 256 < pt.y && pt.y < 159 * client_rect.bottom / 256
					 && client_rect.left + (client_rect.right - client_rect.left) / 3 <= pt.x
					 && pt.x < client_rect.left + 2 * (client_rect.right - client_rect.left) / 3)
			  get_counter_name_by_type(tooltip, BYTE1(instance->targets[18].player), instance->counters3);
			else if (instance->counters4
					 && 90 * client_rect.bottom / 256 < pt.y && pt.y < 159 * client_rect.bottom / 256
					 && pt.x < client_rect.left + (client_rect.right - client_rect.left) / 3)
			  get_counter_name_by_type(tooltip, BYTE2(instance->targets[18].player), instance->counters4);
			else if (instance->counters5
					 && 53 * client_rect.bottom / 100 < pt.y && pt.y < 89 * client_rect.bottom / 100
					 && (client_rect.right + client_rect.left) / 2 <= pt.x)
			  get_counter_name_by_type(tooltip, BYTE3(instance->targets[18].player), instance->counters5);
			else if ((instance->counters || instance->counters_m1m1)
					 && 53 * client_rect.bottom / 100 < pt.y && pt.y < 89 * client_rect.bottom / 100
					 && pt.x < (client_rect.right + client_rect.left) / 2)
			  {
				if (instance->counters_m1m1 == 0)
				  get_counter_name_by_type(tooltip, COUNTER_P1_P1, instance->counters);
				else if (instance->counters == 0)
				  get_counter_name_by_type(tooltip, COUNTER_M1_M1, instance->counters_m1m1);
				else
				  {
					char m1m1[100];
					get_counter_name_by_type(m1m1, COUNTER_M1_M1, instance->counters_m1m1);
					char p1p1[100];
					get_counter_name_by_type(p1p1, COUNTER_P1_P1, instance->counters);
					scnprintf(tooltip, 100, "%s; %s", m1m1, p1p1);
				  }
			  }
			else if (((instance->state & STATE_OWNED_BY_OPPONENT) ? 1 : 0) != player && pt.y < 12 * client_rect.bottom / 100)
			  strcpy(tooltip, EXE_STR(0x7283b4));	// CUECARD_SMALLCARD[3] = Card is not controlled by owner
			else if (5 * client_rect.right / 100 < pt.x && pt.x < 95 * client_rect.right / 100
					 && 15 * client_rect.bottom / 100 < pt.y && pt.y < 95 * client_rect.bottom / 100)
			  {
				if (instance->state & (STATE_TARGETTED | STATE_CANNOT_TARGET))
				  {
					switch (instance->state & (STATE_TARGETTED | STATE_CANNOT_TARGET))
					  {
						case STATE_TARGETTED | STATE_CANNOT_TARGET:	strcpy(tooltip, EXE_STR(0x739dd0));	break;	// CUECARD_SMALLCARD[6] = Is a target, can't target again
						case STATE_TARGETTED:							strcpy(tooltip, EXE_STR(0x620d5c));	break;	// CUECARD_SMALLCARD[4] = Is a target
						case STATE_CANNOT_TARGET:						strcpy(tooltip, EXE_STR(0x737e24));	break;	// CUECARD_SMALLCARD[5] = Can't target this
					  }
				  }
				else if (instance->kill_code == KILL_DESTROY)
				  strcpy(tooltip, EXE_STR(0x786f08));	// CUECARD_SMALLCARD[7] = Dying
				else
				  {
					int is_summonsick = (instance->state & STATE_SUMMON_SICK) && (cards_data[instance->internal_card_id].type & TYPE_CREATURE);
					int is_phased = instance->state & STATE_OUBLIETTED;
					if (is_summonsick && is_phased)
					  {
						strcpy(tooltip, EXE_STR(0x728374));	// CUECARD_SMALLCARD[8] = Summoning sickness
						strcat(tooltip, ",");
						strcat(tooltip, EXE_STR(0x62bcf0));	// CUECARD_SMALLCARD[9] = Phased
					  }
					else if (is_summonsick)
					  strcpy(tooltip, EXE_STR(0x728374));	// CUECARD_SMALLCARD[8] = Summoning sickness
					else if (is_phased)
					  strcpy(tooltip, EXE_STR(0x62bcf0));	// CUECARD_SMALLCARD[9] = Phased
				  }
			  }
		  }

		if (tooltip[0])
		  strcpy((char *)wparam, tooltip);

		if (EXE_DWORD(0x787260) != 2)	// option_Layout
		  SendMessageA(hwnd, 0x111u, 0x6Eu, 0);

		return tooltip[0] != 0;
	}

  return EXE_STDCALL_FN(LRESULT, 0x489d60, HWND, UINT, WPARAM, LPARAM)(hwnd, msg, wparam, lparam);	// wndproc_CardClass()
}

#define smallcard_width		EXE_DWORD(0x7286f8)
#define smallcard_height	EXE_DWORD(0x739d50)

int set_smallcard_size(int mainwindow_width)
{
  static int setting_smallcard_size = 0;
  if (setting_smallcard_size == 0)	// unset
	{
	  read_settings();	// Since this function is usually called before the others that call read_settings() (pregame() and after_load_game())

	  setting_smallcard_size = get_setting(SETTING_SMALLCARD_SIZE);
	  if (setting_smallcard_size == 0)	// set to default
		setting_smallcard_size = -8;
	}

  int sz = setting_smallcard_size;
  if (sz < 0)
    sz = mainwindow_width / -sz;

  smallcard_width = smallcard_height = sz;

  return sz;
}

void update_hand_window(HWND hwnd)
{
  int num_cards = GetWindowLong(hwnd, 4);
  int cw = GetWindowLong(hwnd, 0);
  HWND* card_hwnds = (HWND *)cw;

  int v19, v22, v2, v3;
  EXE_FN(void, 0x47d850, int, int, int, int, int, int,
		 int*, int*, int*, int*)(smallcard_width, smallcard_height,
								 GetWindowLong(hwnd, 8),
								 GetWindowLong(hwnd, 12),
								 GetWindowLong(hwnd, 16),
								 GetWindowLong(hwnd, 20),
								 &v19, &v22, &v2, &v3);

  int hand_window_width = smallcard_width + 2*(v2 + v3);
  int hand_window_height = v19 + v22;

  if (hwnd == EXE_PTR_VOID(0x7158c8)	// hwnd_HandClass_player
	  || (hwnd == EXE_PTR_VOID(0x786dc4)	// hwnd_HandClass_opponent
		  && ((EXE_DWORD(0x628c24) & 2) || EXE_DWORD(0x628c28))))
	{
	  int cards_to_show = MIN(num_cards, get_setting(SETTING_HAND_SIZE));

	  if (cards_to_show > 0)
		{
		  int offset = EXE_DWORD(0x7A31A4);

		  hand_window_height += smallcard_height + offset * (cards_to_show - 1);

		  int x = v2 + v3;
		  int y = hand_window_height - v22 - smallcard_height;
		  int j = GetWindowLong(hwnd, 28);
		  HWND hwnd_after = NULL;

		  int i;
		  for (i = 1; i <= cards_to_show; ++i)
			{
			  MoveWindow(card_hwnds[j], x, y, smallcard_width, smallcard_height, 1);

			  if (hwnd_after)
				SetWindowPos(card_hwnds[j], hwnd_after, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE);
			  else
				BringWindowToTop(card_hwnds[j]);

			  hwnd_after = card_hwnds[j];

			  if (--j < 0)
				j = num_cards - 1;

			  y -= offset;
			}

		  for (i = cards_to_show; i < num_cards; ++i)
			{
			  MoveWindow(card_hwnds[j], -1, -1, 0, 0, 1);

			  if (--j < 0)
				j = num_cards - 1;
			}
		}
	  UpdateWindow(hwnd);
	  SetWindowPos(hwnd, 0, 0, 0, hand_window_width, hand_window_height, SWP_NOZORDER|SWP_NOMOVE);
	}
  else
	{
	  int i;
	  for (i = 0; i < num_cards; ++i)
		MoveWindow(card_hwnds[i], -1, -1, 0, 0, 1);
	  SetWindowPos(hwnd, 0, 0, 0, hand_window_width, hand_window_height, SWP_NOZORDER|SWP_NOMOVE);
	}
}

// Sound effects.  Somewhat misplaced here, but this is as close as Manalink comes to "UI"
static const char* wav_filenames[WAV_HIGHEST + 1] =
{
  "artifact.wav",
  "buried.wav",
  "draw.wav",
  "enchant.wav",
  "endphase.wav",
  "endturn.wav",
  "instant.wav",
  "interupt.wav",
  "GREY.wav",
  "BLACK.wav",
  "BLUE.wav",
  "GREEN.wav",
  "RED.wav",
  "WHITE.wav",
  "lifeloss.wav",
  "sacrfice.wav",
  "sorcery.wav",
  "summon.wav",
  "tap.wav",
  "untap.wav",
  "attack2.wav",
  "block2.wav",
  "damage.wav",
  "destroy.wav",
  "discard.wav",
  "kill.wav",
  "regen.wav",
  "BLACKRED.wav",
  "GREENBLACK.wav",
  "WHITERED.wav",
  "WHITEGREEN.wav",
  "BLACKWHITE.wav",
  "GREENRED.wav",
  "GREENBLUE.wav",
  "WHITEBLUE.wav",
  "BLUEBLACK.wav",
  "REDBLUE.wav",
  "counter.wav",
  "fastfx.wav",
  "changec.wav",
  "changet.wav",
  "control.wav",
  "manaburn.wav",
  "shuffle.wav",
  "shell_loseduel.wav",
  "shell_winduel.wav",
  "aswanjag.wav",
  "callgrav.wav",
  "faerdrag.wav",
  "gembazar.wav",
  "necrazar.wav",
  "polkamix.wav",
  "pandora.wav",
  "prsmdrag.wav",
  "pwrstrgl.wav",
  "catatap.wav",
  "orcart.wav",
  "whimsy.wav",
  "rainbowk.wav",
  "toss.wav",
  "shell_shandalar.wav",
  "shell_tooltime.wav",
  "shell_helpme.wav",
  "shell_hallofrecords.wav",
  "shell_duelmenow.wav",
  "exp1_openfoil.wav",
  "exp1_openbox.wav",
  "exp1_outofpack.wav",
  "exp1_backinpack.wav",
  "Bloodthirst.wav",
  "Devour.wav",
  "Monstrosity.wav",
  "Evolve.wav",
  "Plus_Counter.wav",
  "Minus_Counter.wav",
  "Mill.wav",
  "Raise_Dead.wav",
  "Bushido.wav",
  "Equip.wav",
  "LifeGain.wav",
};

int play_sound_effect(wav_t soundnum)
{
  if (ai_is_speculating == 1)
    return 0;

  typedef struct	// very little idea what any of these fields do.
  {
	int field_0;
	int field_4;
	int field_8;
	int field_C;
	int field_10;
	int field_14;
	int field_18;
	int field_1C;
  } sound_t;

  sound_t snd;
  snd.field_0 = 300;
  snd.field_4 = 0;
  snd.field_8 = 0;
  snd.field_C = 0;
  snd.field_10 = 0;
  snd.field_14 = 0;
  snd.field_18 = soundnum;
  snd.field_1C = 0;

  if (soundnum >= WAV_ASWANJAG && soundnum <= WAV_TOSS)
    {
      snd.field_0 = 400;
      if (soundnum == WAV_CATATAP)
		snd.field_14 = -1;
      else
		snd.field_1C |= 4;
    }

  wav_t adj_soundnum = soundnum;
  if (!EXE_FN(int, 0x45a270, wav_t, wav_t*)(soundnum, &adj_soundnum))	// call_IsSndLoaded()
    {
      char path[300];
      strcpy(path, EXE_STR(0x715788));	// path_DuelSounds
      strcat(path, "\\");
      strcat(path, wav_filenames[soundnum]);
      EXE_FN(int, 0x45a1c0, const char*, int, sound_t*)(path, adj_soundnum, &snd);	// call_LoadSnd()
    }

  EXE_FN(int, 0x45a240, int, sound_t*)(adj_soundnum, 0);	// call_PlaySnd()

  return 1;
}
