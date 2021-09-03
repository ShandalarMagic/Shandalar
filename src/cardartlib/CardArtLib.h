#ifndef __CARDARTLIB_H__
#define __CARDARTLIB_H__

#include <windows.h>
#include <io.h>
#include <gdiplus.h>
#include <unordered_map>
#include <string>
#include <cstdio>
#include <cassert>
#include <memory>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DEFS_H
	struct card_data_t;
	struct card_ptr_t;
#endif

	int LoadBigArt(int id, int version, LONG width, int height);
	int LoadSmallArt(int id, int version, LONG width, int height);
	void DestroyBigArt(int id, int version);
	void DestroySmallArt(int id, int version);
	int IsBigArtRightSize(int id, int version, int width, int height);
	int IsSmallArtRightSize(int id, int version, int width, int height);
	void DestroyAllBigArts(void);
	void DestroyAllSmallArts(void);
	int IsBigArtIn(int id, int version);
	int IsSmallArtIn(int id, int version);
	int ReloadBigArtIfWrongSize(int id, int version, LONG width, int height);
	int ReloadSmallArtIfWrongSize(int id, int version);
	int DrawBigArt(HDC hdc, const RECT* rect, int id, int version);
	int DrawSmallArt(HDC hdc, const RECT* rect, int id, int version);
	void Cardartlib_initialize_for_shandalar(card_data_t* real_cards_data, card_ptr_t* real_cards_ptr);
	int WINAPI DllMain(HINSTANCE hDllHandle, DWORD nReason, LPVOID reserved);
#ifdef __cplusplus
}
#endif

#endif
