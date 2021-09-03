#ifndef DECKDLL_H	// -*- tab-width:8; c-basic-offset:2; -*-
#define DECKDLL_H 1
// DeckDll: standalone and integrated deckbuilder.

#include <string.h>
#include <math.h>
#include <stdio.h>

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <shlwapi.h>

// windows.h defines this to either LoadImageA or LoadImageW.
// We want the function in image.dll.
#undef LoadImage

#define DECKBUILDER 1

#include "../defs.h"
#include "resources.h"

struct DeckEntry;
struct GlobalDeckEntry;

extern "C"
{
  // Our exports
  BOOL WINAPI DllEntryPoint(HINSTANCE dll, DWORD reason, LPVOID reserved);
  BOOL WINAPI DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved);
  WPARAM WINAPI DeckBuilderMain(HWND parent_hwnd, int db_flags_1, int db_flags_2);
  WPARAM deckbuilder_main(HWND parent_hwnd, int db_flags_1, int db_flags_2);
  void Deckdll_initialize_for_shandalar(const card_ptr_t* i_raw_cards_ptr,
					int i_available_slots,
					char* i_card_coded,
					int (*check_card_count_fn)(const DeckEntry*, int, int),
					int (*is_valid_card_fn)(int),
					bool (*colors_match_fn)(iid_t, color_test_t),
					int (*check_colors_inout_edited_deck_fn)(const GlobalDeckEntry*, int, bool));

  // Imports
#define DLLIMPORT __attribute__((dllimport))
  // from image.dll
  DLLIMPORT HANDLE LoadImage(const char*, HPALETTE, int, int);
  // from Drawcardlib.dll
  DLLIMPORT int DrawFullCard(HDC hdc, const RECT* rect, const card_ptr_t* cp, int version, int big_art_style, int expand_text_box, const char* illus);
  DLLIMPORT int DrawSmallCard(HDC hdc, const RECT* card_rect, const card_ptr_t* cp, int version, int mode, int player, int card);
};

void fatal(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

#endif
