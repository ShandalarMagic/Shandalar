// -*- tab-width:4; c-basic-offset:2; -*-
// Drawcardlib: display card and mana cost graphics.
// drawcardlib.c: primary interface to Magic.exe; smallcards

#include "drawcardlib.h"

typedef struct
{
  WORD palVersion;
  WORD palNumEntries;
  PALETTEENTRY palPalEntry[256];
} Palette256;

Palette256 palette256 =
{
  0x300,
  256,
  {
	{  0,  0,  0,  0}, { 26, 24, 28,  0}, { 14, 18, 78,  0}, { 15, 18,109,  0},
	{ 22, 42, 79,  0}, { 20, 44,110,  0}, { 42, 23, 76,  0}, { 41, 22,110,  0},
	{ 45, 49, 76,  0}, { 44, 50,109,  0}, { 41, 84, 37,  0}, { 45, 82, 95,  0},
	{ 78, 15, 20,  0}, { 79, 18, 41,  0}, { 77, 46, 20,  0}, { 74, 50, 48,  0},
	{107, 15, 17,  0}, {108, 18, 46,  0}, {109, 47, 20,  0}, {110, 48, 44,  0},
	{ 86, 46, 86,  0}, { 92, 83, 42,  0}, { 79, 78, 78,  0}, { 76, 82,109,  0},
	{ 82,105, 83,  0}, { 81,107,112,  0}, {108, 82, 79,  0}, {109, 83,107,  0},
	{111,106, 82,  0}, {112,110,110,  0}, { 28, 36,155,  0}, { 29, 35,218,  0},
	{ 40, 88,158,  0}, { 34, 96,226,  0}, { 84, 47,154,  0}, { 85, 42,217,  0},
	{ 89,102,154,  0}, { 89,104,220,  0}, { 38,146, 40,  0}, { 39,154, 95,  0},
	{ 35,216, 42,  0}, { 34,214,100,  0}, { 95,149, 42,  0}, { 97,150,101,  0},
	{100,213, 37,  0}, { 98,209, 99,  0}, { 42,153,162,  0}, { 38,155,236,  0},
	{ 41,215,162,  0}, { 39,217,241,  0}, { 82,139,144,  0}, { 81,140,175,  0},
	{ 80,173,142,  0}, { 80,172,176,  0}, {112,140,144,  0}, {111,143,175,  0},
	{111,174,144,  0}, {114,170,177,  0}, { 80,142,207,  0}, { 78,144,243,  0},
	{ 80,172,208,  0}, { 79,175,246,  0}, {111,144,206,  0}, {110,146,241,  0},
	{113,173,207,  0}, {110,176,244,  0}, { 95,213,164,  0}, { 80,206,209,  0},
	{ 79,206,249,  0}, { 79,241,206,  0}, { 80,236,249,  0}, {113,204,208,  0},
	{112,205,248,  0}, {111,241,209,  0}, {111,237,249,  0}, {152, 40, 36,  0},
	{153, 39, 89,  0}, {144, 75, 19,  0}, {141, 79, 48,  0}, {142,111, 17,  0},
	{143,109, 49,  0}, {173, 76, 20,  0}, {173, 80, 45,  0}, {173,110, 19,  0},
	{174,111, 49,  0}, {141, 81, 78,  0}, {139, 82,108,  0}, {140,111, 82,  0},
	{139,115,110,  0}, {172, 81, 78,  0}, {173, 83,107,  0}, {173,113, 78,  0},
	{172,114,110,  0}, {217, 46, 34,  0}, {204, 49, 87,  0}, {202, 81, 18,  0},
	{203, 81, 48,  0}, {204,111, 20,  0}, {205,112, 50,  0}, {244, 78, 16,  0},
	{235, 79, 50,  0}, {234,111, 16,  0}, {234,113, 49,  0}, {212,103, 92,  0},
	{159, 46,152,  0}, {156, 49,217,  0}, {149,106,153,  0}, {149,105,225,  0},
	{208, 46,169,  0}, {215, 57,207,  0}, {213,102,159,  0}, {226,102,217,  0},
	{162,148, 34,  0}, {144,140, 82,  0}, {144,138,114,  0}, {141,167, 82,  0},
	{143,173,113,  0}, {177,140, 81,  0}, {173,143,113,  0}, {177,171, 79,  0},
	{176,171,115,  0}, {164,206, 48,  0}, {157,216, 94,  0}, {211,154, 43,  0},
	{205,145, 78,  0}, {202,147,110,  0}, {205,171, 79,  0}, {207,173,114,  0},
	{235,144, 81,  0}, {239,146,112,  0}, {237,173, 80,  0}, {242,176,113,  0},
	{219,205, 29,  0}, {227,209,104,  0}, {144,143,142,  0}, {141,147,172,  0},
	{145,171,146,  0}, {145,171,176,  0}, {171,147,141,  0}, {173,148,172,  0},
	{176,171,146,  0}, {175,176,174,  0}, {141,146,205,  0}, {141,145,241,  0},
	{143,176,205,  0}, {142,177,243,  0}, {172,146,205,  0}, {172,145,245,  0},
	{173,179,204,  0}, {173,178,242,  0}, {164,209,169,  0}, {145,202,209,  0},
	{144,207,245,  0}, {143,240,210,  0}, {144,239,249,  0}, {177,203,208,  0},
	{175,209,243,  0}, {177,237,208,  0}, {176,239,249,  0}, {202,148,141,  0},
	{205,148,171,  0}, {207,176,144,  0}, {204,180,174,  0}, {241,148,140,  0},
	{236,149,171,  0}, {241,177,143,  0}, {239,178,174,  0}, {203,145,207,  0},
	{204,147,241,  0}, {205,180,204,  0}, {204,179,242,  0}, {236,144,209,  0},
	{243,144,241,  0}, {237,182,203,  0}, {241,177,244,  0}, {209,202,146,  0},
	{208,203,179,  0}, {209,235,146,  0}, {208,235,178,  0}, {239,204,145,  0},
	{237,208,177,  0}, {243,236,145,  0}, {245,237,178,  0}, {209,209,207,  0},
	{206,211,240,  0}, {210,234,210,  0}, {208,240,247,  0}, {237,212,205,  0},
	{239,212,241,  0}, {242,234,210,  0}, {246,247,247,  0}, {254,254,254,  0},
	{222,222,222,  0}, {203,203,203,  0}, {184,184,184,  0}, {164,164,164,  0},
	{147,147,147,  0}, {125,125,125,  0}, {106,106,106,  0}, { 86, 86, 86,  0},
	{ 67, 67, 67,  0}, { 47, 47, 47,  0}, { 28, 28, 28,  0}, {152,167,205,  0},
	{121,145,206,  0}, {101,129,186,  0}, { 81,115,164,  0}, { 61, 99,144,  0},
	{ 40, 82,123,  0}, { 15, 66,104,  0}, { 69,230,255,  0}, { 37,211,252,  0},
	{ 12,186,241,  0}, { 15,149,202,  0}, { 16,117,167,  0}, { 16, 87,131,  0},
	{ 17,148,255,  0}, { 14,126,221,  0}, { 11,104,187,  0}, {  9, 83,154,  0},
	{  6, 61,120,  0}, {  3, 39, 86,  0}, {170,212,  7,  0}, {150,185,  8,  0},
	{130,158,  9,  0}, {111,132, 11,  0}, { 91,105, 12,  0}, {111,119,235,  0},
	{ 79, 85,188,  0}, { 63, 68,165,  0}, { 46, 50,142,  0}, {100,144,128,  0},
	{ 85,129,105,  0}, { 71,114, 83,  0}, { 56, 99, 60,  0}, { 36, 68, 33,  0},
	{  1,  1,  1,  0}, {  1,  1,  1,  0}, {  1,  1,  1,  0}, {  1,  1,  1,  0},
	{  1,  1,  1,  0}, {  1,  1,  1,  0}, {  1,  1,  1,  0}, {  1,  1,  1,  0},
	{  1,  1,  1,  0}, {  1,  1,  1,  0}, {  1,  1,  1,  0}, {  1,  1,  1,  0},
	{  1,  1,  1,  0}, {  1,  1,  1,  0}, {  1,  1,  1,  0}, {  1,  1,  1,  0},
	{  1,  1,  1,  0}, {  1,  1,  1,  0}, {  1,  1,  1,  0}, {255,255,255,  0}
  }
};
LOGPALETTE* logpalette = (LOGPALETTE*)(&palette256);
HPALETTE palette = NULL;

HANDLE pics[MAX_PICHANDLES_PLUS_1] = { 0 };
GpBitmap* gpics[MAX_PICHANDLES_PLUS_1] = { 0 };

Parent parent;

HDC screendc = 0;
ULONG_PTR gdiplus_token = 0;
CRITICAL_SECTION* critical_section_for_drawing = NULL;
CRITICAL_SECTION* critical_section_for_display = NULL;
HDC* spare_hdc = NULL;

card_instance_t* hack_instance = NULL;	// Instance of the currently-drawn card, if known.  Only accurate after a call to select_frame().

/**************** In Magic.exe ****************/
card_data_t* cards_data_exe = (void*)(0x7e7010);	// drawcardlib.c, drawfullcard.c
card_ptr_t** cards_ptr_manalink_exe = (void*)(0x73bb00);
card_ptr_t* cards_ptr_shandalar_exe = NULL;
void* get_displayed_card_instance_manalink_exe = (void*)(0x401ab0);	// for get_displayed_card_instance.asm
typedef int (*intFn_int)(int);
intFn_int get_internal_card_id_from_csv_id_exe = (intFn_int)(0x479df0);

void
popup(const char* title, const char* fmt, ...)
{
  char buf[8000];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, 8000, fmt, args);
  va_end(args);

  MessageBox(0, buf, title, MB_ICONERROR|MB_TASKMODAL);
}

card_instance_t*
get_displayed_card_instance(int player, int card)
{
  if (parent == PARENT_MANALINK)
	return get_displayed_card_instance_manalink(player, card);
  else if (parent == PARENT_SHANDALAR)
	return &(((card_instance_t (*)[151])0x7a82f0)[player][card]);
  else
	{
	  popup("drawcardlib.dll", "get_displayed_card_instance() called, but running from neither Shandalar nor Manalink (deckbuilder?)");
	  abort();
	  return NULL;
	}
}

card_ptr_t*
get_card_ptr(int csvid)
{
  if (parent == PARENT_MANALINK)
	return cards_ptr_manalink_exe[csvid];
  else if (parent == PARENT_SHANDALAR)
	return &cards_ptr_shandalar_exe[csvid];
  else
	{
	  popup("drawcardlib.dll", "get_card_ptr() called, but running from neither Shandalar nor Manalink (deckbuilder?)");
	  abort();
	  return NULL;
	}
}

static void
init_palette(void)
{
  PALETTEENTRY* entry = &logpalette->palPalEntry[0];
  int i;
  for (i = 0; i < 256; ++i, ++entry)
	{
	  BYTE red = entry->peRed;
	  entry->peRed = entry->peBlue;
	  entry->peBlue = red;

	  entry->peFlags = PC_EXPLICIT;
	}

  palette = CreatePalette(logpalette);
}

static int
init_gdiplus(void)
{
  if (!gdiplus_token)
	{
	  GdiplusStartupInput input;
	  input.GdiplusVersion = 1;
	  input.DebugEventCallback = NULL;
	  input.SuppressBackgroundThread = 0;
	  input.SuppressExternalCodecs = 0;

	  GdiplusStartup(&gdiplus_token, &input, NULL);

	  int i;
	  for (i = 0; i <= MAX_CFG; ++i)
		create_alpha_xforms(&configs[i]);

	  return 1;
	}
  return 0;
}

static void
close_gdiplus(void)
{
  int i;
  for (i = 0; i < MAX_PICHANDLES_PLUS_1; ++i)
	if (gpics[i])
	  {
		GdipDisposeImage(gpics[i]);
		gpics[i] = NULL;
	  }

  GdiplusShutdown(gdiplus_token);
}

void
del_obj(HGDIOBJ* obj)
{
  if (obj)
	{
	  DeleteObject(*obj);
	  *obj = NULL;
	}
}

static void
del_resources(void)
{
  /*
   * Explicitly call these cleanup functions from CardArtLib.dll, since this dll gets unloaded first, and Cardartlib.dll will crash if it tries to destroy its
   * images after GdiplusShutdown has been called.  How Mok's build can depend on CardArtLib.dll but get unloaded after it is beyond me. -Korath
   */
  DestroyAllBigArts();
  DestroyAllSmallArts();

  del_fonts_and_imgs();

  del_obj((HGDIOBJ*)(&palette));
  close_gdiplus();
}

static void
del_screendc(void)
{
  if (screendc)
	{
	  DeleteDC(screendc);
	  screendc = 0;
	  if (parent == PARENT_OTHER)
		{
		  DeleteCriticalSection(critical_section_for_drawing);
		  DeleteCriticalSection(critical_section_for_display);
		}
	}
}

static int
initialize(void)
{
  if (screendc)
	return 1;

#ifndef EXE_VOID_PTR
#  define EXE_VOID_PTR(addr)	((void*)(addr))	// as distinguished from EXE_PTR_VOID, which is another pointer to a pointer to void
#endif

  HMODULE parent_mod = GetModuleHandle(0);
  if (GetProcAddress(parent_mod, "szDeckName"))
	{
	  parent = PARENT_MANALINK;
	  critical_section_for_drawing = EXE_VOID_PTR(0x56b950);
	  critical_section_for_display = EXE_VOID_PTR(0x620288);
	  spare_hdc = EXE_VOID_PTR(0x4eb7ac);
	}
  else if (GetProcAddress(parent_mod, "_OpponFace"))
	{
	  parent = PARENT_SHANDALAR;
	  get_internal_card_id_from_csv_id_exe = (intFn_int)(0x56c705);
	  critical_section_for_drawing = EXE_VOID_PTR(0x5a9100);
	  critical_section_for_display = EXE_VOID_PTR(0x79fea0);
	  spare_hdc = EXE_VOID_PTR(0x585920);
	}
  else
	{
	  parent = PARENT_OTHER;
	  static CRITICAL_SECTION local_critical_section1, local_critical_section2;
	  critical_section_for_drawing = &local_critical_section1;
	  InitializeCriticalSection(critical_section_for_drawing);
	  critical_section_for_display = &local_critical_section2;
	  InitializeCriticalSection(critical_section_for_display);
	  HDC hdc_parent = GetDC(0);
	  static HDC local_spare_hdc = NULL;
	  local_spare_hdc = CreateCompatibleDC(hdc_parent);
	  spare_hdc = &local_spare_hdc;
	  ReleaseDC(0, hdc_parent);
	}

  HDC hdc = GetDC(NULL);
  screendc = CreateCompatibleDC(hdc);
  ReleaseDC(NULL, hdc);

  init_palette();

  if (!screendc)
	{
	  popup("drawcardlib.dll", "!screendc");
	  del_screendc();
	  return 0;
	}

  init_mana_tags();

  int i;
  for (i = 0; i <= MAX_CFG; ++i)
	{
	  configs[i].smallcard_loyalty_curr_alpha_xform = NULL;
	  configs[i].colorless_alpha_xform = NULL;
	}

  int result = read_cfg();
  result &= prepare_fonts_and_imgs();	// deliberately not && - always call both

  if (!result)
	del_resources();

  return result;
}

void
initialize_for_shandalar(card_data_t* real_cards_data, card_ptr_t* real_cards_ptr)
{
  // This can't be done by quering Shandlar.dll during parent detection above, since it won't have replaced them yet.
  cards_data_exe = real_cards_data;
  cards_ptr_shandalar_exe = real_cards_ptr;
}

BOOL WINAPI
DllMain(HINSTANCE dll, DWORD reason, LPVOID reserved)
{
  (void)dll;
  (void)reserved;

  switch (reason)
	{
	  case DLL_PROCESS_ATTACH:
		return initialize();

	  case DLL_PROCESS_DETACH:
		del_resources();
		del_screendc();
		return 1;

	  default:
		return 1;
	}
}

int
DestroyAllCardBackgrounds(void)
{
  // Obsolete.  Card backgrounds are freed on dll unload.
  return 1;
}

static void make_frame(PicHandleNames frame);

static void
make_frame_from_2_overlays(PicHandleNames target, PicHandleNames background, PicHandleNames overlay1, PicHandleNames overlay2)
{
  if (!gpics[background])
	make_frame(background);
  if (!gpics[overlay1])
	make_frame(overlay1);
  if (overlay2 != PICHANDLE_INVALID && !gpics[overlay2])
	make_frame(overlay2);

  UINT width, height;
  GdipGetImageWidth(gpics[background], &width);
  GdipGetImageHeight(gpics[background], &height);

  GpBitmap* gbmp_target = NULL;
  GdipCloneBitmapAreaI(0, 0, width, height, PixelFormat32bppARGB, gpics[background], &gbmp_target);

  GpGraphics* gfx_target = NULL;
  GdipGetImageGraphicsContext(gbmp_target, &gfx_target);

  GdipSetInterpolationMode(gfx_target, InterpolationModeHighQuality);
  GdipDrawImageI(gfx_target, gpics[overlay1], 0, 0);
  if (overlay2 != PICHANDLE_INVALID)
	GdipDrawImageI(gfx_target, gpics[overlay2], 0, 0);

  GdipDeleteGraphics(gfx_target);

  gpics[target] = gbmp_target;
}

static void
make_frame_from_overlay(PicHandleNames target, PicHandleNames background, PicHandleNames overlay)
{
  make_frame_from_2_overlays(target, background, overlay, PICHANDLE_INVALID);
}

static void
make_hybrid_frame_with_overlay(Config* config, PicHandleNames target, PicHandleNames left, PicHandleNames right, PicHandleNames overlay)
{
  if (!gpics[left])
	make_frame(left);
  if (!gpics[right])
	make_frame(right);
  if (overlay != PICHANDLE_INVALID && !gpics[overlay])
	make_frame(overlay);

  UINT lwidth, lheight, rwidth, rheight;
  GdipGetImageWidth(gpics[left], &lwidth);
  GdipGetImageHeight(gpics[left], &lheight);
  GdipGetImageWidth(gpics[right], &rwidth);
  GdipGetImageHeight(gpics[right], &rheight);

  GpBitmap* gbmp_target = NULL;
  GdipCloneBitmapAreaI(0, 0, lwidth, lheight, PixelFormat32bppARGB, gpics[left], &gbmp_target);

  // Add alpha channel
  GpRect r;
  r.X = 0;
  r.Y = 0;
  r.Width = rwidth;
  r.Height = rheight;
  BitmapData right_data;
  GdipBitmapLockBits(gpics[right], &r, ImageLockModeRead, PixelFormat32bppARGB, &right_data);

  // We want the final alpha to be a weighted average of source and target alpha, not a sum, but gdi+ gives no control over the channel!  So have to do it manually. :( :(
  BitmapData target_data;
  GdipBitmapLockBits(gbmp_target, &r, ImageLockModeWrite, PixelFormat32bppARGB, &target_data);

  double percent_solid_on_hybrid = (100 - config->frames_percent_mixed_on_hybrid) / 200.0;
  double m = 255.0 / (rwidth * (1 - 2 * percent_solid_on_hybrid));
  double b = -255.0 * percent_solid_on_hybrid / (1 - 2 * percent_solid_on_hybrid);

  UINT x, y;
  UINT minheight = MIN(right_data.Height, target_data.Height);
  UINT minwidth = MIN(right_data.Width, target_data.Width);
  INT right_stride = abs(right_data.Stride);
  INT target_stride = abs(target_data.Stride);
  for (y = 0; y < minheight; ++y)
	{
	  uint8_t* rightbits = (uint8_t*)right_data.Scan0 + y * right_stride;
	  uint8_t* targetbits = (uint8_t*)target_data.Scan0 + y * target_stride;
	  for (x = 0; x < minwidth; ++x, rightbits += 4, targetbits += 4)
		{
		  double blend = m*x + b;
		  if (blend < 0.0)
			blend = 0;
		  else if (blend > 255.0)
			blend = 255;

#define CLAMP_INTO(to)				\
		  if (k < 0.0)				\
			targetbits[to] = 0;		\
		  else if (k > 255.0)		\
			targetbits[to] = 255;	\
		  else						\
			targetbits[to] = k + 0.5

		  double k;
#define BLT_RGB(col)																								\
		  k = (255 - blend)*targetbits[3]*targetbits[col]/(255*255) + blend*rightbits[3]*rightbits[col]/(255*255);	\
		  CLAMP_INTO(col)

		  BLT_RGB(0);
		  BLT_RGB(1);
		  BLT_RGB(2);
#undef BLT_RGB

		  k = (255 - blend)*targetbits[3]/255 + blend*rightbits[3]/255;
		  CLAMP_INTO(3);
#undef CLAMP
		}
	}

  GdipBitmapUnlockBits(gbmp_target, &target_data);
  GdipBitmapUnlockBits(gpics[right], &right_data);

  if (overlay != PICHANDLE_INVALID)
	{
	  GpGraphics* gfx_target = NULL;
	  GdipGetImageGraphicsContext(gbmp_target, &gfx_target);

	  GdipSetInterpolationMode(gfx_target, InterpolationModeHighQuality);
	  GdipDrawImageI(gfx_target, gpics[overlay], 0, 0);

	  GdipDeleteGraphics(gfx_target);
	}

  gpics[target] = gbmp_target;
}

static void
make_hybrid_frame(Config* config, PicHandleNames target, PicHandleNames left, PicHandleNames right)
{
  make_hybrid_frame_with_overlay(config, target, left, right, PICHANDLE_INVALID);
}

static void
make_frame(PicHandleNames frame)
{
  switch (frame)
	{
	  case CARDBACK:	case MANASYMBOLS:	case EXPANSION_SYMBOLS:	case WATERMARKS:	case CARDCOUNTERS:
	  case CARDBK_MIN_FRAMEPART ... CARDBK_MAX_FRAMEPART_ALL:
		make_gpic_from_pic(frame);
		break;

	  case MAX_PICHANDLES_PLUS_1:
		popup("drawcardlib.dll", "Attempted to make_frame(%s)", get_pichandlename(frame, 0));
		abort();

#define CFG(frameset)	(configs[CFG_##frameset])

#define MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(background, overlay_normal, nyx, overlay_nyx)	\
		if (nyx)																		\
		  make_frame_from_2_overlays(frame, background, overlay_nyx, overlay_normal);	\
		else																			\
		  make_frame_from_overlay(frame, background, overlay_normal)

#define MAKE_MONOCOLORS_WITH_VARYING_BACKGROUND(frameset, framename, nyx)	\
		   framename##BLACK:	MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_BLACK, CARDOV_##frameset##_B, nyx, CARDOV_##frameset##_BNYX);	break;	\
	  case framename##BLUE:		MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_BLUE,  CARDOV_##frameset##_U, nyx, CARDOV_##frameset##_UNYX);	break;	\
	  case framename##GREEN:	MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_GREEN, CARDOV_##frameset##_G, nyx, CARDOV_##frameset##_GNYX);	break;	\
	  case framename##RED:		MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_RED,   CARDOV_##frameset##_R, nyx, CARDOV_##frameset##_RNYX);	break;	\
	  case framename##WHITE:	MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_WHITE, CARDOV_##frameset##_W, nyx, CARDOV_##frameset##_WNYX)

#define MAKE_MONOCOLORS(frameset, framename, base, suffix, nyx, nyxsuffix)	\
		   framename##BLACK:	MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(base, CARDOV_##frameset##_B##suffix, nyx, CARDOV_##frameset##_##nyxsuffix##_NYX);	break;	\
	  case framename##BLUE:		MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(base, CARDOV_##frameset##_U##suffix, nyx, CARDOV_##frameset##_##nyxsuffix##_NYX);	break;	\
	  case framename##GREEN:	MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(base, CARDOV_##frameset##_G##suffix, nyx, CARDOV_##frameset##_##nyxsuffix##_NYX);	break;	\
	  case framename##RED:		MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(base, CARDOV_##frameset##_R##suffix, nyx, CARDOV_##frameset##_##nyxsuffix##_NYX);	break;	\
	  case framename##WHITE:	MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(base, CARDOV_##frameset##_W##suffix, nyx, CARDOV_##frameset##_##nyxsuffix##_NYX)

#define MAKE_ONE_MONOCOLOR_ARTIFACT_LAND(frameset, framename, letter, color)	\
	  FRAME_##framename##_ARTIFACT_LAND_##color:								\
		if (CFG(frameset).frames_artifact_land == 3)							\
		  make_hybrid_frame(&CFG(frameset), frame, FRAME_##framename##_ARTIFACT_##color, FRAME_##framename##_LAND_##color);	\
		else																	\
		  make_frame_from_overlay(frame, FRAME_##framename##_ARTIFACT_LAND_COLORLESS, CARDOV_##frameset##_##letter##LAND)

#define MAKE_MONOCOLOR_ARTIFACT_LANDS(frameset, framename)							\
		   MAKE_ONE_MONOCOLOR_ARTIFACT_LAND(frameset, framename, B, BLACK);	break;	\
	  case MAKE_ONE_MONOCOLOR_ARTIFACT_LAND(frameset, framename, U, BLUE);	break;	\
	  case MAKE_ONE_MONOCOLOR_ARTIFACT_LAND(frameset, framename, G, GREEN);	break;	\
	  case MAKE_ONE_MONOCOLOR_ARTIFACT_LAND(frameset, framename, R, RED);	break;	\
	  case MAKE_ONE_MONOCOLOR_ARTIFACT_LAND(frameset, framename, W, WHITE)

#define MAKE_HYBRIDS(frameset, hybrid, half)																\
		   hybrid##WHITE_BLUE:	make_hybrid_frame(&CFG(frameset), frame, half##WHITE, half##BLUE);	break;	\
	  case hybrid##BLUE_BLACK:	make_hybrid_frame(&CFG(frameset), frame, half##BLUE,  half##BLACK);	break;	\
	  case hybrid##BLACK_RED:	make_hybrid_frame(&CFG(frameset), frame, half##BLACK, half##RED);	break;	\
	  case hybrid##RED_GREEN:	make_hybrid_frame(&CFG(frameset), frame, half##RED,   half##GREEN);	break;	\
	  case hybrid##GREEN_WHITE:	make_hybrid_frame(&CFG(frameset), frame, half##GREEN, half##WHITE);	break;	\
	  case hybrid##WHITE_BLACK:	make_hybrid_frame(&CFG(frameset), frame, half##WHITE, half##BLACK);	break;	\
	  case hybrid##BLACK_GREEN:	make_hybrid_frame(&CFG(frameset), frame, half##BLACK, half##GREEN);	break;	\
	  case hybrid##GREEN_BLUE:	make_hybrid_frame(&CFG(frameset), frame, half##GREEN, half##BLUE);	break;	\
	  case hybrid##BLUE_RED:	make_hybrid_frame(&CFG(frameset), frame, half##BLUE,  half##RED);	break;	\
	  case hybrid##RED_WHITE:	make_hybrid_frame(&CFG(frameset), frame, half##RED,   half##WHITE)

#define MAKE_HYBRIDS_WITH_BAR(frameset, hybrid, half, bar)																		\
		   hybrid##WHITE_BLUE:	make_hybrid_frame_with_overlay(&CFG(frameset), frame, half##WHITE, half##BLUE, bar);	break;	\
	  case hybrid##BLUE_BLACK:	make_hybrid_frame_with_overlay(&CFG(frameset), frame, half##BLUE,  half##BLACK, bar);	break;	\
	  case hybrid##BLACK_RED:	make_hybrid_frame_with_overlay(&CFG(frameset), frame, half##BLACK, half##RED, bar);		break;	\
	  case hybrid##RED_GREEN:	make_hybrid_frame_with_overlay(&CFG(frameset), frame, half##RED,   half##GREEN, bar);	break;	\
	  case hybrid##GREEN_WHITE:	make_hybrid_frame_with_overlay(&CFG(frameset), frame, half##GREEN, half##WHITE, bar);	break;	\
	  case hybrid##WHITE_BLACK:	make_hybrid_frame_with_overlay(&CFG(frameset), frame, half##WHITE, half##BLACK, bar);	break;	\
	  case hybrid##BLACK_GREEN:	make_hybrid_frame_with_overlay(&CFG(frameset), frame, half##BLACK, half##GREEN, bar);	break;	\
	  case hybrid##GREEN_BLUE:	make_hybrid_frame_with_overlay(&CFG(frameset), frame, half##GREEN, half##BLUE, bar);	break;	\
	  case hybrid##BLUE_RED:	make_hybrid_frame_with_overlay(&CFG(frameset), frame, half##BLUE,  half##RED, bar);		break;	\
	  case hybrid##RED_WHITE:	make_hybrid_frame_with_overlay(&CFG(frameset), frame, half##RED,   half##WHITE, bar)

#define MAKE_FRAMESET(frameset, framename, nyx)																												\
		   FRAME_##framename##_SPELL_COLORLESS:																												\
		MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_COLORLESS, CARDOV_##frameset##_COLORLESS, nyx, CARDOV_##frameset##_COLORLESS_NYX);	break;	\
	  case MAKE_MONOCOLORS_WITH_VARYING_BACKGROUND(frameset, FRAME_##framename##_SPELL_, nyx);														break;	\
	  case MAKE_MONOCOLORS(frameset, FRAME_##framename##_SPELL_GOLD_, CARDBK_##frameset##_GOLD, GOLD, nyx, GOLD);									break;	\
	  case MAKE_HYBRIDS_WITH_BAR(frameset, FRAME_##framename##_SPELL_GOLD_, FRAME_##framename##_SPELL_GOLD_, CARDOV_##frameset##_GOLD_BAR);			break;	\
	  case FRAME_##framename##_SPELL_GOLD:																													\
		MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_GOLD, CARDOV_##frameset##_GOLD, nyx, CARDOV_##frameset##_GOLD_NYX);					break;	\
																																							\
	  case FRAME_##framename##_ARTIFACT_COLORLESS:																											\
		MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_ARTIFACT, CARDOV_##frameset##_ARTIFACT, nyx, CARDOV_##frameset##_ARTIFACT_NYX);		break;	\
	  case MAKE_MONOCOLORS(frameset, FRAME_##framename##_ARTIFACT_, CARDBK_##frameset##_ARTIFACT, ARTIFACT, nyx, ARTIFACT);							break;	\
	  case MAKE_MONOCOLORS(frameset, FRAME_##framename##_ARTIFACT_GOLD_, CARDBK_##frameset##_ARTIFACT, GOLD_ARTIFACT, nyx, ARTIFACT);				break;	\
	  case MAKE_HYBRIDS_WITH_BAR(frameset, FRAME_##framename##_ARTIFACT_GOLD_, FRAME_##framename##_ARTIFACT_GOLD_, CARDOV_##frameset##_GOLD_BAR);	break;	\
	  case FRAME_##framename##_ARTIFACT_GOLD:																												\
		MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_ARTIFACT, CARDOV_##frameset##_GOLD_ARTIFACT, nyx, CARDOV_##frameset##_ARTIFACT_NYX);	break;	\
																																							\
	  case FRAME_##framename##_LAND_COLORLESS:																												\
		MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_LAND, CARDOV_##frameset##_COLORLESS_LAND, nyx, CARDOV_##frameset##_LAND_NYX);			break;	\
	  case MAKE_MONOCOLORS(frameset, FRAME_##framename##_LAND_, CARDBK_##frameset##_LAND, LAND, nyx, LAND);											break;	\
	  case MAKE_MONOCOLORS(frameset, FRAME_##framename##_LAND_GOLD_, CARDBK_##frameset##_LAND, LAND, nyx, LAND);									break;	\
	  case MAKE_HYBRIDS_WITH_BAR(frameset, FRAME_##framename##_LAND_GOLD_, FRAME_##framename##_LAND_GOLD_, CARDOV_##frameset##_LAND_BAR);			break;	\
	  case FRAME_##framename##_LAND_GOLD:																													\
		MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_LAND, CARDOV_##frameset##_GOLD_LAND, nyx, CARDOV_##frameset##_LAND_NYX);				break;	\
																																							\
	  case FRAME_##framename##_ARTIFACT_LAND_COLORLESS:																										\
		if (CFG(frameset).frames_artifact_land == 2 || CFG(frameset).frames_artifact_land == 3)																\
		  make_hybrid_frame(&CFG(frameset), frame, FRAME_##framename##_ARTIFACT_COLORLESS, FRAME_##framename##_LAND_COLORLESS);								\
		else																																				\
		  MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX(CARDBK_##frameset##_ARTIFACT, CARDOV_##frameset##_COLORLESS_LAND, nyx, CARDOV_##frameset##_ARTIFACT_NYX);		\
		break;																																				\
	  case MAKE_MONOCOLOR_ARTIFACT_LANDS(frameset, framename);																						break;	\
	  case MAKE_HYBRIDS_WITH_BAR(frameset, FRAME_##framename##_ARTIFACT_LAND_GOLD_, FRAME_##framename##_ARTIFACT_LAND_, CARDOV_##frameset##_LAND_BAR);	break;\
	  case FRAME_##framename##_ARTIFACT_LAND_GOLD_BLACK:	case FRAME_##framename##_ARTIFACT_LAND_GOLD_BLUE:	case FRAME_##framename##_ARTIFACT_LAND_GOLD_GREEN:\
	  case FRAME_##framename##_ARTIFACT_LAND_GOLD_RED:	case FRAME_##framename##_ARTIFACT_LAND_GOLD_WHITE:	abort();	/* unused, directly or indirectly */\
	  case MAKE_ONE_MONOCOLOR_ARTIFACT_LAND(frameset, framename, GOLD_, GOLD);																		break;	\
																																							\
	  case MAKE_HYBRIDS(frameset, FRAME_##framename##_SPELL_HYBRID_, FRAME_##framename##_SPELL_)


	  case MAKE_FRAMESET(MODERN, MODERN, 0);				break;
	  case MAKE_FRAMESET(MODERN, MODERN_NYX, 1);			break;
	  case MAKE_FRAMESET(CLASSIC, CLASSIC, 0);				break;
	  case MAKE_FRAMESET(CLASSIC, CLASSIC_NYX, 1);			break;
	  case MAKE_FRAMESET(TIMESHIFT, TIMESHIFT, 0);			break;
	  case MAKE_FRAMESET(TIMESHIFT, TIMESHIFT_NYX, 1);		break;
	  case MAKE_FRAMESET(FUTURESHIFT, FUTURESHIFT, 0);		break;
	  case MAKE_FRAMESET(FUTURESHIFT, FUTURESHIFT_NYX, 1);	break;
	  case MAKE_FRAMESET(PLANESWALKER, PLANESWALKER, 0);	break;
	  case MAKE_FRAMESET(PLANESWALKER, PLANESWALKER_NYX, 1);break;
	  case MAKE_FRAMESET(TOKEN, TOKEN, 0);					break;
	  case MAKE_FRAMESET(TOKEN, TOKEN_NYX, 1);				break;
	  case MAKE_FRAMESET(SCHEME, SCHEME, 0);				break;
	  case MAKE_FRAMESET(SCHEME, SCHEME_NYX, 1);			break;
	  case MAKE_FRAMESET(PLANE, PLANE, 0);					break;
	  case MAKE_FRAMESET(PLANE, PLANE_NYX, 1);				break;
	  case MAKE_FRAMESET(VANGUARD, VANGUARD, 0);			break;
	  case MAKE_FRAMESET(VANGUARD, VANGUARD_NYX, 1);		break;
	  case MAKE_FRAMESET(INTERNAL, INTERNAL, 0);			break;
	  case MAKE_FRAMESET(INTERNAL, INTERNAL_NYX, 1);		break;
	  case MAKE_FRAMESET(MAGIC2015, MAGIC2015, 0);			break;
	  case MAKE_FRAMESET(MAGIC2015, MAGIC2015_NYX, 1);		break;
	  case MAKE_FRAMESET(MAGIC2015_HOLO, MAGIC2015_HOLO, 0);							break;
	  case MAKE_FRAMESET(MAGIC2015_HOLO, MAGIC2015_HOLO_NYX, 1);						break;
	  case MAKE_FRAMESET(MAGIC2015_PLANESWALKER, MAGIC2015_PLANESWALKER, 0);			break;
	  case MAKE_FRAMESET(MAGIC2015_PLANESWALKER, MAGIC2015_PLANESWALKER_NYX, 1);		break;

#undef CFG
#undef MAKE_FRAME_FROM_OVERLAY_MAYBE_NYX
#undef MAKE_MONOCOLORS_WITH_VARYING_BACKGROUND
#undef MAKE_MONOCOLORS
#undef MAKE_ONE_MONOCOLOR_ARTIFACT_LAND
#undef MAKE_MONOCOLOR_ARTIFACT_LANDS
#undef MAKE_HYBRIDS
#undef MAKE_FRAMESET
	}
}

static PicHandleNames
count_colors_to_hybrid_pic_handle_name(CountColors cc, PicHandleNames base, PicHandleNames deflt)
{
  switch (cc & COUNT_COLOR_COLOR_MASK)
	{
	  case COUNT_COLOR_WHITE | COUNT_COLOR_BLUE:	return base + (FRAME_MODERN_SPELL_HYBRID_WHITE_BLUE - FRAME_MODERN_SPELL_HYBRID_MIN);
	  case COUNT_COLOR_BLUE | COUNT_COLOR_BLACK:	return base + (FRAME_MODERN_SPELL_HYBRID_BLUE_BLACK - FRAME_MODERN_SPELL_HYBRID_MIN);
	  case COUNT_COLOR_BLACK | COUNT_COLOR_RED:		return base + (FRAME_MODERN_SPELL_HYBRID_BLACK_RED - FRAME_MODERN_SPELL_HYBRID_MIN);
	  case COUNT_COLOR_RED | COUNT_COLOR_GREEN:		return base + (FRAME_MODERN_SPELL_HYBRID_RED_GREEN - FRAME_MODERN_SPELL_HYBRID_MIN);
	  case COUNT_COLOR_GREEN | COUNT_COLOR_WHITE:	return base + (FRAME_MODERN_SPELL_HYBRID_GREEN_WHITE - FRAME_MODERN_SPELL_HYBRID_MIN);

	  case COUNT_COLOR_WHITE | COUNT_COLOR_BLACK:	return base + (FRAME_MODERN_SPELL_HYBRID_WHITE_BLACK - FRAME_MODERN_SPELL_HYBRID_MIN);
	  case COUNT_COLOR_BLACK | COUNT_COLOR_GREEN:	return base + (FRAME_MODERN_SPELL_HYBRID_BLACK_GREEN - FRAME_MODERN_SPELL_HYBRID_MIN);
	  case COUNT_COLOR_GREEN | COUNT_COLOR_BLUE:	return base + (FRAME_MODERN_SPELL_HYBRID_GREEN_BLUE - FRAME_MODERN_SPELL_HYBRID_MIN);
	  case COUNT_COLOR_BLUE | COUNT_COLOR_RED:		return base + (FRAME_MODERN_SPELL_HYBRID_BLUE_RED - FRAME_MODERN_SPELL_HYBRID_MIN);
	  case COUNT_COLOR_RED | COUNT_COLOR_WHITE:		return base + (FRAME_MODERN_SPELL_HYBRID_RED_WHITE - FRAME_MODERN_SPELL_HYBRID_MIN);

	  default:										return deflt;
	}
}

static int
is_token_by_instance(card_instance_t* instance)
{
  return ((instance->token_status & STATUS_TOKEN)
		  && (parent != PARENT_MANALINK
			  || (!(instance->targets[14].card != -1 && (instance->targets[14].card & SF_UNEARTH)))));
}

static int
get_special_type_cfgnum_by_type(int type)
{
  switch (type)
	{
	  case SUBTYPE_PLANE:			return CFG_PLANE;
	  case SUBTYPE_PLANESWALKER:	return CFG_PLANESWALKER;
	  case SUBTYPE_SCHEME:			return CFG_SCHEME;
	  case SUBTYPE_VANGUARD:		return CFG_VANGUARD;
	  case SUBTYPE_PRIVATE:			return CFG_INTERNAL;
	  default:						return -1;
	}
}

static int
get_special_type_cfgnum_by_cp(const card_ptr_t* cp)
{
  int cf;
  if ((cf = get_special_type_cfgnum_by_type(cp->types[0])) != -1)
	return cf;
  if ((cf = get_special_type_cfgnum_by_type(cp->types[1])) != -1)
	return cf;

  if (parent == PARENT_MANALINK || parent == PARENT_SHANDALAR)
	{
	  cp = get_card_ptr(cp->id);
	  if ((cf = get_special_type_cfgnum_by_type(cp->types[0])) != -1)
		return cf;
	  if ((cf = get_special_type_cfgnum_by_type(cp->types[1])) != -1)
		return cf;
	}

  return -1;
}

int
is_planeswalker_by_cp(const card_ptr_t* cp)
{
  return get_special_type_cfgnum_by_cp(cp) == CFG_PLANESWALKER;
}

PicHandleNames
select_frame(const card_ptr_t* cp, const Rarity* r)
{
  int typ;
  int clr;
  int ench_evening = 0;
  card_instance_t* instance = NULL;

  if ((cp->color & 0xFFFE) == 0xC3D4)	// magic number indicating player and card are encoded into color
	{
	  if (parent != PARENT_MANALINK && parent != PARENT_SHANDALAR)
		{
		  popup("drawcardlib.dll", "Encoded instance found, but running from neither Shandalar nor Manalink (deckbuilder?)");
		  abort();
		}
	  instance = hack_instance = get_displayed_card_instance(cp->color & 0x1, HIWORD(cp->color));

	  /* Activation cards on the stack, other than the top one, for some reason get their colors overwritten by 1.  This can be observed with the original
	   * executable/drawcardlib by picking a permanent that has a repeatable activation ability (I used Figure of Destiny), changing its color (I used
	   * Lifelace), activating it, and activating it again in response.  Double-right-clicking the top activation card on the stack will show the permanent with
	   * its laced color (green in my test); double-right-clicking the earlier activation card on the stack will show the card's default color (gold here).  I
	   * can't find what's doing the overwriting, so can't comment on whether it's intentional.  Note that this won't affect cards that are actually supposed to
	   * be temporarily colorless; they end up with 0, not 1, for color. */

	  typ = cards_data_exe[instance->internal_card_id].type;

	  if (typ & TYPE_EFFECT)
		typ |= cards_data_exe[instance->original_internal_card_id].type;

	  if (typ & TYPE_PERMANENT)
		{
		  if (parent == PARENT_MANALINK)
			{
			  if (instance->targets[17].player != -1)
				{
				  if (instance->targets[17].player & SF2_ENCHANTED_EVENING)
					{
					  typ |= TYPE_ENCHANTMENT;
					  ench_evening = 1;
					}
				  if (instance->targets[17].player & SF2_MYCOSYNTH_LATTICE)
					typ |= TYPE_ARTIFACT;
				}
			}
		  else if (parent == PARENT_SHANDALAR)
			{
			  if (instance->initial_color & 0x2)	// INSTANCE_FLAG_ENCHANTMENT
				{
				  typ |= TYPE_ENCHANTMENT;
				  ench_evening = 1;
				}
			}
		}

	  if (instance->color == COLOR_TEST_COLORLESS)
		{
		  if (cp->id == CARD_ID_GHOSTFIRE)	// Actually *supposed* to be COLOR_TEST_COLORLESS, and manacost_to_count_colors() will be wrong
			clr = 0;
		  else
			clr = manacost_to_count_colors(cp);

		  if (clr == 0 && !(typ & (TYPE_ARTIFACT | TYPE_LAND)))	// no mana cost (like, say, Ancestral Vision, Evermind, Garruk Relentless, or a token)
			switch (get_card_ptr(cards_data_exe[((typ & TYPE_EFFECT) ? (int32_t)instance->original_internal_card_id : instance->internal_card_id)].id)->color)
			  {
				case CP_COLOR_BLACK:	clr = COLOR_TEST_BLACK;	break;
				case CP_COLOR_BLUE:		clr = COLOR_TEST_BLUE;	break;
				case CP_COLOR_MULTI:	clr = COLOR_TEST_ANY & ~COLOR_TEST_COLORLESS;	break;	// force gold frame
				case CP_COLOR_GREEN:	clr = COLOR_TEST_GREEN;	break;
				case CP_COLOR_RED:		clr = COLOR_TEST_RED;	break;
				case CP_COLOR_WHITE:	clr = COLOR_TEST_WHITE;	break;
				case CP_COLOR_SPECIAL:	clr = -2;				break;

				case CP_COLOR_LESS:
				case CP_COLOR_ARTIFACT:
				case CP_COLOR_LAND:
				default:
				  clr = 0;
				  break;
			  }
		  else
			clr &= COLOR_TEST_ANY_COLORED;
		}
	  else
		{
		  clr = instance->color;
		  clr &= COLOR_TEST_ANY_COLORED;
		  if (clr && (typ & TYPE_LAND))
			{
			  typ &= ~TYPE_LAND;	// So it gets a fully-colored frame
			  typ |= TYPE_CREATURE;	// So if it's part enchantment, it still gets the Nyx frame.
			}
		}
	}
  else
	{
	  hack_instance = NULL;

	  switch (cp->card_type)
		{
		  case CP_TYPE_ARTIFACT:
			typ = TYPE_ARTIFACT;
			break;

		  case CP_TYPE_LAND:
			if (cp->id == CARD_ID_ANCIENT_DEN
				|| cp->id == CARD_ID_DARKSTEEL_CITADEL
				|| cp->id == CARD_ID_GREAT_FURNACE
				|| cp->id == CARD_ID_SEAT_OF_THE_SYNOD
				|| cp->id == CARD_ID_TREE_OF_TALES
				|| cp->id == CARD_ID_VAULT_OF_WHISPERS)
			  typ = TYPE_ARTIFACT | TYPE_LAND;
			else
			  typ = TYPE_LAND;
			break;

		  default:
			typ = 0;
			break;
		}

	  switch (cp->color)
		{
		  case CP_COLOR_BLACK:		clr = COLOR_TEST_BLACK;	break;
		  case CP_COLOR_BLUE:		clr = COLOR_TEST_BLUE;	break;
		  case CP_COLOR_MULTI:		clr = -1;				break;
		  case CP_COLOR_GREEN:		clr = COLOR_TEST_GREEN;	break;
		  case CP_COLOR_RED:		clr = COLOR_TEST_RED;	break;
		  case CP_COLOR_WHITE:		clr = COLOR_TEST_WHITE;	break;
		  case CP_COLOR_SPECIAL:	clr = -2;				break;

		  case CP_COLOR_LESS:
		  case CP_COLOR_ARTIFACT:
		  case CP_COLOR_LAND:
		  default:
			clr = 0;
			break;
		}
	}

  int special_type_cfgnum;

  if (force_frameset >= 0)
	cfg = &configs[force_frameset];
  else if ((special_type_cfgnum = get_special_type_cfgnum_by_cp(cp)) >= 0)
	{
	  cfg = &configs[special_type_cfgnum];
	  /* Most of these types will always be tokens, which is why they're checked for first;
	   * but Planeswalkers won't be, so using the Token frame for non-card ones is meaningful. */
	  if (special_type_cfgnum == CFG_PLANESWALKER)
		{
		  if (instance && is_token_by_instance(instance))
			cfg = &configs[CFG_TOKEN];
		  else if (r->default_expansion >= EXPANSION_MAGIC_2015)
			cfg = &configs[CFG_MAGIC2015_PLANESWALKER];
		}
	}
  else if (instance ? is_token_by_instance(instance) : BYTE0(cp->card_type) == CP_TYPE_TOKEN)
	cfg = &configs[CFG_TOKEN];
  else if ((r->flags & RARITYFLAG_TIMESHIFT)
		   && r->default_expansion == EXPANSION_PLANAR_CHAOS)
	cfg = &configs[CFG_TIMESHIFT];
  else if ((r->flags & RARITYFLAG_TIMESHIFT)
		   && r->default_expansion == EXPANSION_FUTURE_SIGHT)
	cfg = &configs[CFG_FUTURESHIFT];
  else if (r->default_expansion < EXPANSION_EIGHTH_EDITION
		   || r->default_expansion == EXPANSION_MASTERS_EDITION
		   || r->default_expansion == EXPANSION_MASTERS_EDITION_II
		   || r->default_expansion == EXPANSION_MASTERS_EDITION_III
		   || r->default_expansion == EXPANSION_MASTERS_EDITION_IV
		   || (r->default_expansion == EXPANSION_TIME_SPIRAL && r->default_rarity == SPECIAL))
	cfg = &configs[CFG_CLASSIC];
  else if (r->default_expansion < EXPANSION_MAGIC_2015)
	cfg = &configs[CFG_MODERN];
  else if (r->default_rarity == RARE || r->default_rarity == MYTHIC)
	cfg = &configs[CFG_MAGIC2015_HOLO];
  else
	cfg = &configs[CFG_MAGIC2015];

  switch (cfg->frames_force_colorless)
	{
	  case 1:
		typ = 0;
		if (clr != -2)
		  clr = 0;
		break;

	  case 2:
		typ = 0;
		clr = 0;
		break;
	}

  int nyx;
  if (force_nyx >= 0)
	nyx = force_nyx;
  else if (cfg == &configs[CFG_PLANESWALKER])
	// This fails if the card has become part-enchantment by means other than Enchanted Evening.  The only way to do so that I'm aware of is for something
	// like Memnarch to change it to an artifact, and then a different player to Copy Artifact it.
	nyx = ench_evening;	// typ & TYPE_ENCHANTMENT will always be true for planeswalkers anyway
  else if (instance
		   && (instance->state & (STATE_OUBLIETTED | STATE_IN_PLAY | STATE_JUST_CAST | STATE_INVISIBLE)))	// More ugly hacks to get around Theros enchantment-creatures' base cards being non-creatures
	nyx = ((typ & (TYPE_ARTIFACT|TYPE_LAND|TYPE_CREATURE))
		   && (typ & TYPE_ENCHANTMENT));
  else
	{
#ifndef USE_TYPES_FOR_NYX
	  nyx = r->flags & RARITYFLAG_NYX;
#else
	  // Since the base cards are the pure-enchantment versions, this isn't currently workable for the gods or bestow creatures (which leaves only the artifacts)
	  int enchantment = 0, nonenchantment = 0;
	  int i;
	  for (i = 0; i < 7; ++i)
		switch (cp->types[i])
		  {
			case SUBTYPE_ENCHANTMENT:
			  enchantment = 1;
			  if (nonenchantment)
				{
				  nyx = 1;
				  goto found;
				}
			  break;
			case SUBTYPE_ARTIFACT:
			case SUBTYPE_CREATURE:
			case SUBTYPE_LAND:
			case SUBTYPE_PLANESWALKER:
			  nonenchantment = 1;
			  if (enchantment)
				{
				  nyx = 1;
				  goto found;
				}
			  break;
		  }
	  // else
	  nyx = 0;
	found:;
#endif
	}

  // An ugly hack to make sure Theros enchantment-creatures show their power/toughness, even though the base cards are non-creature versions
  if ((r->flags & RARITYFLAG_NYX)
	  && cp->toughness <= 0
	  && (parent == PARENT_MANALINK || parent == PARENT_SHANDALAR)
	  && (!instance || !(instance->state & (STATE_OUBLIETTED | STATE_IN_PLAY | STATE_JUST_CAST | STATE_INVISIBLE)))
	  && (cp->id == CARD_ID_EREBOS_GOD_OF_THE_DEAD
		  || cp->id == CARD_ID_HELIOD_GOD_OF_THE_SUN
		  || cp->id == CARD_ID_NYLEA_GOD_OF_THE_HUNT
		  || cp->id == CARD_ID_PURPHOROS_GOD_OF_THE_FORGE
		  || cp->id == CARD_ID_THASSA_GOD_OF_THE_SEA))
	{
	  const card_ptr_t* base_cp = get_card_ptr(cp->id + 1);
	  hack_power = base_cp->power;
	  hack_toughness = base_cp->toughness;
	}
  else
	{
	  hack_power = cp->power;
	  hack_toughness = cp->toughness;
	}

  PicHandleNames frameset = cfg->frame_base;
  if (nyx)
	frameset += (FRAME_MODERN_NYX_MIN - FRAME_MODERN_MIN);

  PicHandleNames frame = frameset;

  if (clr == -2)
	frame = CARDBK_MODERN_SPECIAL;
  else if (typ & TYPE_LAND)
	{
	  int mana_clr;
	  if (instance)
		mana_clr = instance->mana_color;
	  else
		mana_clr = cp->mana_source_colors;

	  mana_clr &= (COLOR_TEST_ANY & ~COLOR_TEST_COLORLESS); // i.e., the five colors, but not colorless

	  if (typ & TYPE_ARTIFACT
		  && cfg->frames_artifact_land)
		switch (mana_clr)
		  {
			case COLOR_TEST_BLACK:	frame = FRAME_MODERN_ARTIFACT_LAND_BLACK;		break;
			case COLOR_TEST_BLUE:	frame = FRAME_MODERN_ARTIFACT_LAND_BLUE;		break;
			case COLOR_TEST_GREEN:	frame = FRAME_MODERN_ARTIFACT_LAND_GREEN;		break;
			case COLOR_TEST_RED:	frame = FRAME_MODERN_ARTIFACT_LAND_RED;			break;
			case COLOR_TEST_WHITE:	frame = FRAME_MODERN_ARTIFACT_LAND_WHITE;		break;
			case 0:					frame = FRAME_MODERN_ARTIFACT_LAND_COLORLESS;	break;
			default:
			  if (cfg->frames_percent_mixed_on_hybrid >= 0
				  && !(cfg->frames_no_early_hybrid_land
					   && (!r || r->default_expansion < EXPANSION_CLASSIC_SIXTH_EDITION)))
				frame = count_colors_to_hybrid_pic_handle_name(mana_clr, FRAME_MODERN_ARTIFACT_LAND_GOLD_MIN, FRAME_MODERN_ARTIFACT_LAND_GOLD);
			  else
				frame = FRAME_MODERN_ARTIFACT_LAND_GOLD;
			  break;
		  }
	  else
		switch (mana_clr)
		  {
			case COLOR_TEST_BLACK:	frame = FRAME_MODERN_LAND_BLACK;		break;
			case COLOR_TEST_BLUE:	frame = FRAME_MODERN_LAND_BLUE;			break;
			case COLOR_TEST_GREEN:	frame = FRAME_MODERN_LAND_GREEN;		break;
			case COLOR_TEST_RED:	frame = FRAME_MODERN_LAND_RED;			break;
			case COLOR_TEST_WHITE:	frame = FRAME_MODERN_LAND_WHITE;		break;
			case 0:					frame = FRAME_MODERN_LAND_COLORLESS;	break;
			default:
			  if (cfg->frames_percent_mixed_on_hybrid >= 0
				  && !(cfg->frames_no_early_hybrid_land
					   && (!r || r->default_expansion < EXPANSION_CLASSIC_SIXTH_EDITION)))
				frame = count_colors_to_hybrid_pic_handle_name(mana_clr, FRAME_MODERN_LAND_GOLD_MIN, FRAME_MODERN_LAND_GOLD);
			  else
				frame = FRAME_MODERN_LAND_GOLD;
			  break;
		  }
	}
  else if (typ & TYPE_ARTIFACT)
	switch (clr)
	  {
		case COLOR_TEST_BLACK:	frame = FRAME_MODERN_ARTIFACT_BLACK;		break;
		case COLOR_TEST_BLUE:	frame = FRAME_MODERN_ARTIFACT_BLUE;			break;
		case COLOR_TEST_GREEN:	frame = FRAME_MODERN_ARTIFACT_GREEN;		break;
		case COLOR_TEST_RED:	frame = FRAME_MODERN_ARTIFACT_RED;			break;
		case COLOR_TEST_WHITE:	frame = FRAME_MODERN_ARTIFACT_WHITE;		break;
		case 0:					frame = FRAME_MODERN_ARTIFACT_COLORLESS;	break;
		default:
		  if (cfg->frames_percent_mixed_on_hybrid >= 0 && !cfg->frames_two_color_as_gold)
			{
			  if (clr == -1)
				clr = manacost_to_count_colors(cp);
			  frame = count_colors_to_hybrid_pic_handle_name(clr, FRAME_MODERN_ARTIFACT_GOLD_MIN, FRAME_MODERN_ARTIFACT_GOLD);
			}
		  else
			frame = FRAME_MODERN_ARTIFACT_GOLD;
		  break;
	  }
  else
	switch (clr)
	  {
		case COLOR_TEST_BLACK:	frame = FRAME_MODERN_SPELL_BLACK;		break;
		case COLOR_TEST_BLUE:	frame = FRAME_MODERN_SPELL_BLUE;		break;
		case COLOR_TEST_GREEN:	frame = FRAME_MODERN_SPELL_GREEN;		break;
		case COLOR_TEST_RED:	frame = FRAME_MODERN_SPELL_RED;			break;
		case COLOR_TEST_WHITE:	frame = FRAME_MODERN_SPELL_WHITE;		break;
		case 0:					frame = FRAME_MODERN_SPELL_COLORLESS;	break;
		default:
		  if (cfg->frames_percent_mixed_on_hybrid >= 0)
			{
			  /* This is redundant to the previous call in the 0xC3D4 check only for fullcards of activation calls that are on the stack but not on the top,
			   * which I think is an unusual enough occurrence to not worry about. */
			  CountColors colors = manacost_to_count_colors(cp);
			  if (clr == -1)
				clr = colors & (COLOR_TEST_ANY & ~COLOR_TEST_COLORLESS);
			  /* Otherwise, we already have its current colors from the instance check above.  We still need to compute colors, however, solely to determine
			   * whether it's a hybrid card; in which case we draw the hybrid frame with its current colors, even if they're different. */

			  if (((colors & COUNT_COLOR_HYBRID)			// at least one hybrid mana symbol
				   && !(colors & COUNT_COLOR_MONO))			// no non-hybrid colored mana symbols
				  || cfg->frames_two_color_as_hybrid == 2	// And now the second call to manacost_to_count_colors() is redundant again.  Oh well.
				  || (cfg->frames_two_color_as_hybrid == 1
					  && (unsigned int)clr != (colors & (COLOR_TEST_ANY & ~COLOR_TEST_COLORLESS))))	// not original colors
				frame = count_colors_to_hybrid_pic_handle_name(clr, FRAME_MODERN_SPELL_HYBRID_MIN, FRAME_MODERN_SPELL_GOLD);
			  else if (cfg->frames_two_color_as_gold)
				frame = FRAME_MODERN_SPELL_GOLD;
			  else
				frame = count_colors_to_hybrid_pic_handle_name(clr, FRAME_MODERN_SPELL_GOLD_MIN, FRAME_MODERN_SPELL_GOLD);
			}
		  else
			frame = FRAME_MODERN_SPELL_GOLD;
		  break;
	  }

  if (frame == FRAME_MODERN_SPELL_COLORLESS && cp->id == CARD_ID_FACE_DOWN_CREATURE && cfg->frames_facedown_creature >= 0)
	frame = CARDBACK;
  else if (frame == CARDBK_MODERN_SPECIAL)	// on the frameparts scale, not the frame one
	frame = cfg->cardbk_base + (frame - CARDBK_MIN_FRAMEPART);
  else
	frame = frameset + (frame - FRAME_MODERN_MIN);

  if (!gpics[frame])
	make_frame(frame);

  return frame;
}

static RECT*
draw_smallcard_art(HDC hdc, const RECT* card_rect, int id, int version, int must_save_dc, int shrink_rect, int intersect_clip_with_shrunk_rect)
{
  int savedc = must_save_dc ? SaveDC(hdc) : 0;

  SetMapMode(hdc, MM_TEXT);

  static RECT small_art_rect;

  if (shrink_rect)
	{
	  int wid = card_rect->right - card_rect->left;
	  int hgt = card_rect->bottom - card_rect->top;

	  int art_left = (wid * cfg->smallcard_art.left) / 1000;
	  int art_top = (hgt * cfg->smallcard_art.top) / 1000;
	  int art_wid = (wid * (cfg->smallcard_art.right - cfg->smallcard_art.left)) / 1000;
	  int art_hgt = (hgt * (cfg->smallcard_art.bottom - cfg->smallcard_art.top)) / 1000;

	  SetRect(&small_art_rect, art_left, art_top, art_wid + art_left, art_hgt + art_top);

	  if (intersect_clip_with_shrunk_rect)
		{
		  IntersectClipRect(hdc, small_art_rect.left, small_art_rect.top, small_art_rect.right, small_art_rect.bottom);
		  CopyRect(&small_art_rect, card_rect);
		  OffsetRect(&small_art_rect, -small_art_rect.left, -small_art_rect.top);
		}
	}
  else
	{
	  CopyRect(&small_art_rect, card_rect);
	  OffsetRect(&small_art_rect, -small_art_rect.left, -small_art_rect.top);
	}
  card_rect = &small_art_rect;

  if (IsSmallArtIn(id, version))
	ReloadSmallArtIfWrongSize(id, version, card_rect->right - card_rect->left, card_rect->bottom - card_rect->top);
  else
	LoadSmallArt(id, version, card_rect->right - card_rect->left, card_rect->bottom - card_rect->top);

  DrawSmallArt(hdc, card_rect, id, version);

  if (must_save_dc)
	RestoreDC(hdc, savedc);

  return &small_art_rect;
}

static void
draw_smallcard_frame(HDC hdc, const RECT* dest_rect, PicHandleNames framenum, GpImageAttributes* alpha_xform)
{
  RECT card_rect;

  if (!hdc || !dest_rect)
	return;

  UINT width;
  GdipGetImageWidth(gpics[framenum], &width);

  SetRect(&card_rect, 0, 0, 800, cfg->smallcard_title_dest_height);

  gdip_blt(hdc, &card_rect, framenum,
		   0, cfg->smallcard_title_source_top,
		   width, cfg->smallcard_title_source_height, alpha_xform);

  SetRect(&card_rect, 0, cfg->smallcard_frame_dest_top, 800, 1120);

  gdip_blt(hdc, &card_rect, framenum,
		   0, cfg->smallcard_frame_source_top,
		   width, cfg->smallcard_frame_source_height, alpha_xform);
}

static void
draw_text_with_shadow_and_background(HDC hdc, RECT* rect, int x_offset, const TextWithShadow* tws, const char* str, COLORREF bkcolor)
{
  int len = strlen(str);

  if (bkcolor != (COLORREF)(-1))
	{
	  SetBkMode(hdc, OPAQUE);
	  SetBkColor(hdc, bkcolor);
	}
  else
	SetBkMode(hdc, TRANSPARENT);

  if (tws->shadow_visible)
	{
	  SetTextColor(hdc, tws->shadow_color.colorref);
	  OffsetRect(rect, tws->shadow_offset.x, tws->shadow_offset.y);
	  ExtTextOut(hdc, x_offset + rect->left, rect->top, ETO_CLIPPED, rect, str, len, NULL);
	  OffsetRect(rect, -tws->shadow_offset.x, -tws->shadow_offset.y);

	  if (bkcolor != (COLORREF)(-1))
		{
		  SetBkMode(hdc, TRANSPARENT);
		  bkcolor = (COLORREF)(-1);
		}
	}

  SetTextColor(hdc, tws->color.colorref);
  ExtTextOut(hdc, x_offset + rect->left, rect->top, ETO_CLIPPED, rect, str, len, NULL);

  if (bkcolor != (COLORREF)(-1))
	SetBkMode(hdc, TRANSPARENT);
}

void
draw_text_with_shadow(HDC hdc, RECT* rect, const TextWithShadow* tws, COLORREF override_color, const char* str, int len)
{
  if (len < 0)
	len = strlen(str);

  if (tws->shadow_visible)
	{
	  SetTextColor(hdc, tws->shadow_color.colorref);
	  OffsetRect(rect, tws->shadow_offset.x, tws->shadow_offset.y);
	  ExtTextOut(hdc, rect->left, rect->top, ETO_CLIPPED, rect, str, len, NULL);
	  OffsetRect(rect, -tws->shadow_offset.x, -tws->shadow_offset.y);
	}

  SetTextColor(hdc, override_color != (COLORREF)(-1) ? override_color : tws->color.colorref);
  ExtTextOut(hdc, rect->left, rect->top, ETO_CLIPPED, rect, str, len, NULL);
}

void
draw_text_with_shadow_at_x(HDC hdc, RECT* rect, int x_pos, const TextWithShadow* tws, COLORREF override_color, const char* str, int len)
{
  if (len < 0)
	len = strlen(str);

  if (tws->shadow_visible)
	{
	  SetTextColor(hdc, tws->shadow_color.colorref);
	  OffsetRect(rect, tws->shadow_offset.x, tws->shadow_offset.y);
	  ExtTextOut(hdc, x_pos + tws->shadow_offset.x, rect->top, ETO_CLIPPED, rect, str, len, NULL);
	  OffsetRect(rect, -tws->shadow_offset.x, -tws->shadow_offset.y);
	}

  SetTextColor(hdc, override_color != (COLORREF)(-1) ? override_color : tws->color.colorref);
  ExtTextOut(hdc, x_pos, rect->top, ETO_CLIPPED, rect, str, len, NULL);
}

static int
draw_small_card_title_impl(HDC hdc, const RECT* dest_rect, const char* card_title, int color, int controlled_by_owner)
{
  if (!hdc || !dest_rect || !card_title)
	return 0;

  int savedc = SaveDC(hdc);
  GdiFlush();

  SetMapMode(hdc,MM_ANISOTROPIC);
  SetWindowExtEx(hdc, 800, 1120, 0);
  SetViewportExtEx(hdc, dest_rect->right - dest_rect->left, dest_rect->bottom - dest_rect->top, NULL);
  SetWindowOrgEx(hdc, 0, 0, NULL);
  SetViewportOrgEx(hdc, dest_rect->left, dest_rect->top, NULL);

  int background_visible;
  COLORREF background_color;
  TextWithShadow* tws;

  /* configs[CFG_BASE] instead of cfg since cfg won't be set correctly when called through the DrawSmallCardTitle() api -
   * there isn't enough information given to find which card is being drawn. */
  if (controlled_by_owner)
	{
	  background_visible = configs[CFG_BASE].smallcard_title_owned_background_visible;
	  background_color   = configs[CFG_BASE].smallcard_title_owned_background_color.colorref;
	  switch ((int8_t)color)
		{
		  case 0:	tws = &configs[CFG_BASE].smallcard_title_owned_txt;					break;
		  case 2:	tws = &configs[CFG_BASE].smallcard_title_owned_must_activate_txt;	break;
		  default:	tws = &configs[CFG_BASE].smallcard_title_owned_can_activate_txt;	break;
		}
	}
  else
	{
	  background_visible = configs[CFG_BASE].smallcard_title_unowned_background_visible;
	  background_color   = configs[CFG_BASE].smallcard_title_unowned_background_color.colorref;
	  switch ((int8_t)color)
		{
		  case 0:	tws = &configs[CFG_BASE].smallcard_title_unowned_txt;				break;
		  case 2:	tws = &configs[CFG_BASE].smallcard_title_unowned_must_activate_txt;	break;
		  default:	tws = &configs[CFG_BASE].smallcard_title_unowned_can_activate_txt;	break;
		}
	}

  SelectObject(hdc, fonts[SMALLCARDTITLE_FONT]);
  SetTextAlign(hdc, TA_LEFT);

  if (!background_visible)
	background_color = (COLORREF)(-1);

  draw_text_with_shadow_and_background(hdc, &configs[CFG_BASE].smallcard_title, 0, tws, card_title, background_color);

  return RestoreDC(hdc, savedc);
}

int
DrawSmallCardTitle(HDC hdc, const RECT* dest_rect, const char* card_title, int color, int controlled_by_owner)
{
  EnterCriticalSection(critical_section_for_drawing);
#pragma message "This needs multiple injections so we can figure out which config to use."
  int rval = draw_small_card_title_impl(hdc, dest_rect, card_title, color, controlled_by_owner);
  LeaveCriticalSection(critical_section_for_drawing);
  return rval;
}

int
DrawSmallCard(HDC hdc, const RECT* card_rect, const card_ptr_t* cp, int version, int mode, int player, int card)
{
  if (!hdc || !card_rect || !cp)
	return 0;

  // player and card are only valid if mode == 73

  EnterCriticalSection(critical_section_for_drawing);
  int savedc = SaveDC(hdc);

  SetMapMode(hdc, MM_ANISOTROPIC);
  SetWindowExtEx(hdc, 800, 1120, NULL);
  SetViewportExtEx(hdc, card_rect->right - card_rect->left, card_rect->bottom - card_rect->top, NULL);
  SetWindowOrgEx(hdc, 0, 0, NULL);
  SetViewportOrgEx(hdc, card_rect->left, card_rect->top, NULL);

  const Rarity* r = get_rarity(cp->id);
  PicHandleNames framenum = select_frame(cp, r);
  int colorless_option;
  if (framenum == CARDBACK)
	{
	  colorless_option = cfg->frames_facedown_creature;
	  framenum = FRAME_MODERN_SPELL_COLORLESS;
	}
  else
	colorless_option = cfg->frames_translucent_colorless;

#pragma message "suppress for nyx frames"
  GpImageAttributes* alpha_xform = NULL;
  if (BASE_FRAME(framenum) == FRAME_MODERN_SPELL_COLORLESS && colorless_option >= 0)
	{
	  draw_smallcard_art(hdc, card_rect, cp->id, version, 1, 0, 0);
	  framenum = FRAMEPART_FROM_BASE(CARDOV_MODERN_COLORLESS, cfg);
	  alpha_xform = cfg->colorless_alpha_xform;
	  colorless_option |= FRAMES_TRANSLUCENT_COLORLESS_ENABLED;
	}
  else
	{
	  colorless_option = 0;
	  if (cfg->smallcard_art_first)
		draw_smallcard_art(hdc, card_rect, cp->id, version, 1, 1, 0);
	}

  if (!(colorless_option & FRAMES_TRANSLUCENT_COLORLESS_NO_SMALLCARD_FRAME))
	draw_smallcard_frame(hdc, card_rect, framenum, alpha_xform);

  if (cfg->smallcard_expansion_visible && r)
	blt_expansion_symbol(hdc, &cfg->smallcard_expansion, r->default_expansion, r->default_rarity, 1);

  draw_small_card_title_impl(hdc, card_rect, cp->name, 0, 1);

  /************************************************************************
   * frame		art1st->artfirst	!artfirst							  *
   * none				no			yes									  *
   * ovcolorless		no			no									  *
   * ovcolorless+redraw	yes			yes									  *
   *																	  *
   *    So if smallcard_art_first is set, only draw art here if we both   *
   * have the overlay-only colorless frame and the redraw art option set. *
   *    If smallcard_art_first isn't set, also draw if we don't have the  *
   * overlay-only colorless frame.										  *
   ************************************************************************/
  if ((colorless_option & FRAMES_TRANSLUCENT_COLORLESS_REDRAW_SMALLCARD_ART)
	  || (!cfg->smallcard_art_first && !(colorless_option & FRAMES_TRANSLUCENT_COLORLESS_ENABLED)))
	{
	  RECT* small_art_rect = draw_smallcard_art(hdc, card_rect, cp->id, version, 0, 1, colorless_option & FRAMES_TRANSLUCENT_COLORLESS_REDRAW_SMALLCARD_ART);

	  if (cfg->smallcard_artoutline_visible)
		draw_rect_outline(hdc, small_art_rect, cfg->smallcard_artoutline_color.colorref);
	}

  if (hack_instance && is_planeswalker_by_cp(cp)
	  && (parent == PARENT_MANALINK || parent == PARENT_SHANDALAR) && !(cards_data_exe[hack_instance->internal_card_id].type & TYPE_CREATURE)
	  && cfg->smallcard_show_loyalty_curr && hack_instance->special_counters > 0)
	{
	  GpImageAttributes* loy_alpha_xform = cfg->smallcard_loyalty_curr_alpha_xform;
	  if (!loy_alpha_xform
		  || (alpha_xform && cfg->frames_colorless_alpha < cfg->smallcard_loyalty_curr_alpha))
		loy_alpha_xform = alpha_xform;

	  gdip_blt_whole(hdc, &cfg->smallcard_loyalty_curr_box, FRAMEPART_FROM_BASE(LOYALTY_BASE_MODERN, cfg),
					 cfg->smallcard_loyalty_curr_alpha_xform ? cfg->smallcard_loyalty_curr_alpha_xform : alpha_xform);

	  SelectObject(hdc, fonts[SMALLCARDLOYALTYCURR_FONT]);
	  int pos = (cfg->smallcard_loyalty_curr.right + cfg->smallcard_loyalty_curr.left) / 2;

	  SetTextAlign(hdc, TA_CENTER);
	  char buf[10];
	  sprintf(buf, "%d", hack_instance->special_counters);

	  SetBkMode(hdc, TRANSPARENT);
	  SelectObject(hdc, GetStockObject(NULL_BRUSH));
	  draw_text_with_shadow_at_x(hdc, &cfg->smallcard_loyalty_curr, pos, &cfg->smallcard_loyalty_txt, -1, buf, -1);

	  if (cfg->smallcard_loyalty_curr_suppress_counters)
		suppress_next_counters = 1;
	}
  else
	suppress_next_counters = 0;

  if (cfg->smallcard_outline_visible)
	{
	  RECT rect = { 0, 0, 800, 1120 };
	  draw_rect_outline(hdc, &rect, cfg->smallcard_outline_color.colorref);
	}

  card_instance_t* instance;
  if (mode == 73)
	instance = get_displayed_card_instance(player, card);
  else
	instance = hack_instance;

  if (instance && (instance->state & STATE_IS_TRIGGERING) && cfg->smallcard_triggering_visible)
	{
	  RECT rect = { 0, 0, 800, 1120 };
	  gdip_blt_whole(hdc, &rect, FRAMEPART_FROM_BASE(TRIGGERING_MODERN, cfg), NULL);
	}

  int rval = RestoreDC(hdc, savedc);

  if (mode == 73)	// Drawing an effect card, so manually draw the counters, too
	{
	  // special counters
	  if (instance->special_counters)
		draw_special_counters(hdc, card_rect, player, card);

	  RECT rect_counter;
	  rect_counter.left = 7 * (card_rect->right - card_rect->left) / 100 + card_rect->left;
	  rect_counter.right = card_rect->right - 10 * (card_rect->right - card_rect->left) / 100;
	  rect_counter.top = 35 * (card_rect->bottom - card_rect->top) / 100 + card_rect->top;
	  rect_counter.bottom = 62 * (card_rect->bottom - card_rect->top) / 100 + card_rect->top;

	  draw_standard_counters(hdc, &rect_counter, player, card);
	}

  LeaveCriticalSection(critical_section_for_drawing);
  return rval;
}

void
draw_rect_outline(HDC hdc, const RECT* rect, COLORREF color)
{
  HGDIOBJ old_brush = SelectObject(hdc, GetStockObject(NULL_BRUSH));

  HPEN pen = CreatePen(PS_SOLID, 0, color);
  HGDIOBJ old_pen = SelectObject(hdc, pen);

  Rectangle(hdc, rect->left, rect->top, rect->right, rect->bottom);

  SelectObject(hdc, old_pen);
  SelectObject(hdc, old_brush);

  if (pen)
	DeleteObject(pen);
}

void
make_gpic_from_pic(PicHandleNames picnum)
{
  if (!gpics[picnum])
	{
	  BITMAP bmp;
	  GetObject(pics[picnum], sizeof(BITMAP), &bmp);

	  int just_initted = init_gdiplus();

	  GdipCreateBitmapFromScan0(bmp.bmWidth, bmp.bmHeight, bmp.bmWidth * 4, picnum == CARDBACK ? PixelFormat32bppRGB : PixelFormat32bppARGB, bmp.bmBits, &gpics[picnum]);

	  if (just_initted)
		{
		  if (picnum != CARDCOUNTERS)
			{
			  make_gpic_from_pic(CARDCOUNTERS);	// Otherwise the first card with counters tends not to draw them.
			  GetObject(pics[CARDCOUNTERS], sizeof(BITMAP), &bmp);
			}

		  // Premultiply alpha for gdi rendering
		  unsigned char* bits = bmp.bmBits;
		  int sz = bmp.bmWidth * bmp.bmHeight;
		  int px;
		  for (px = 0; px < sz; ++px)
			{
			  bits[0] = bits[0] * bits[3] / 256;
			  bits[1] = bits[1] * bits[3] / 256;
			  bits[2] = bits[2] * bits[3] / 256;
			  bits += 4;
			}
		}
	}
}

void
gdip_blt_whole(HDC hdc, const RECT* dest_rect, PicHandleNames frame, GpImageAttributes* alpha_xform)
{
  if (!gpics[frame])
	make_frame(frame);

  if (alpha_xform)
	{
	  UINT width, height;
	  GdipGetImageWidth(gpics[frame], &width);
	  GdipGetImageHeight(gpics[frame], &height);
	  gdip_blt(hdc, dest_rect, frame, 0, 0, width, height, alpha_xform);
	}
  else
	{
	  GpGraphics* gfx = NULL;

	  GdipCreateFromHDC(hdc, &gfx);
	  GdipSetInterpolationMode(gfx, InterpolationModeHighQualityBicubic);

	  GdipDrawImageRectI(gfx, gpics[frame], dest_rect->left, dest_rect->top, dest_rect->right - dest_rect->left, dest_rect->bottom - dest_rect->top);

	  GdipDeleteGraphics(gfx);
	}
}

void
gdip_blt(HDC hdc, const RECT* dest_rect, PicHandleNames frame, int src_x, int src_y, int width, int height, GpImageAttributes* alpha_xform)
{
  if (!gpics[frame])
	make_frame(frame);

  GpGraphics* gfx = NULL;

  GdipCreateFromHDC(hdc, &gfx);
  GdipSetInterpolationMode(gfx, InterpolationModeHighQualityBicubic);

  GdipDrawImageRectRectI(gfx, gpics[frame],
						 dest_rect->left, dest_rect->top, dest_rect->right - dest_rect->left, dest_rect->bottom - dest_rect->top,
						 src_x, src_y, width, height,
						 UnitPixel, alpha_xform, NULL, NULL);

  GdipDeleteGraphics(gfx);
}

const card_data_t*
get_card_data_from_csvid(uint32_t csvid)
{
  if (parent != PARENT_MANALINK && parent != PARENT_SHANDALAR)
	return NULL;

  static int32_t* csvid_to_iid = NULL;
  static uint32_t max_csvid = 8192;	// immediately doubled to 16384 in first call

  if (!csvid_to_iid || csvid > max_csvid)
	{
	  max_csvid *= 2;
	  csvid_to_iid = (int32_t*)malloc(sizeof(int32_t) * max_csvid);
	  memset(csvid_to_iid, -1, sizeof(int32_t) * max_csvid);

	  int i;
	  for (i = 0; cards_data_exe[i].id != (uint16_t)(-1); ++i)
		csvid_to_iid[cards_data_exe[i].id] = i;
	}

  if (csvid_to_iid[csvid] == -1)
	{
	  csvid_to_iid[csvid] = get_internal_card_id_from_csv_id_exe(csvid);	// Slow, and returns -1 unless a game has been started, but the only way to reliably find dynamically-created card types.
	  if (csvid_to_iid[csvid] == -1)
		csvid_to_iid[csvid] = -2;	// So we don't search again
	}

  if (csvid_to_iid[csvid] == -2)
	return NULL;
  else
	return &cards_data_exe[csvid_to_iid[csvid]];
}

int
find_rules_engine_csvid(void)
{
  if (parent == PARENT_MANALINK || parent == PARENT_SHANDALAR)
	{
	  static int card_id_rules_engine = -1;
	  if (card_id_rules_engine != -1)
		return card_id_rules_engine;

	  card_id_rules_engine = CARD_ID_RULES_ENGINE;	// If it's not found, don't search again
	  int iid;
	  for (iid = 0; cards_data_exe[iid].id != 0xffff; ++iid)
		if (cards_data_exe[iid].code_pointer == 0x20014f7)
		  {
			card_id_rules_engine = cards_data_exe[iid].id;
			break;
		  }

	  return card_id_rules_engine;
	}
  else	// Deck.dll - just use the hardcoded number csvid as of compilation time.
	return CARD_ID_RULES_ENGINE;
}
