// -*- tab-width:4; c-basic-offset:2; -*-
// Drawcardlib: display card and mana cost graphics.
// drawcounters.c: draw counters

#include "drawcardlib.h"

WINGDIAPI BOOL WINAPI AlphaBlend(HDC,int,int,int,int,HDC,int,int,int,int,BLENDFUNCTION);

int suppress_next_counters = 0;

void
draw_special_counters(HDC hdc, const RECT* rect, int player, int card)
{
  // magic.exe:0x4d3a00

  // This is a bit odd - in Manalink, we're already in a critical_section_for_drawing; in Shandalar, we're not.

  if (!hdc || !rect)
	return;

  card_instance_t actual;
  card_instance_t* instance = get_displayed_card_instance(player, card);	// unsafe to dereference

  if (parent == PARENT_MANALINK)
	{
	  LeaveCriticalSection(critical_section_for_drawing);

	  EnterCriticalSection(critical_section_for_display);
	  memcpy(&actual, instance, sizeof(card_instance_t));
	  LeaveCriticalSection(critical_section_for_display);

	  EnterCriticalSection(critical_section_for_drawing);
	}
  else
	{
	  EnterCriticalSection(critical_section_for_display);
	  memcpy(&actual, instance, sizeof(card_instance_t));
	  LeaveCriticalSection(critical_section_for_display);
	}

  instance = &actual;	// now safe

  int counter_type = instance->special_counter_type;
  if (counter_type < 0 || counter_type >= COUNTER_end)
	return;

  int num = instance->special_counters;
  if (num <= 0)
	return;

  if (suppress_next_counters && counter_type == COUNTER_LOYALTY)
	return;

  RECT r;
  get_special_counter_rect(&r, rect, num);

  if (parent == PARENT_SHANDALAR)
	EnterCriticalSection(critical_section_for_drawing);

  unsigned int src_width, src_height;
  if (counters_renderer == RENDERER_GDIPLUS)
	{
	  GdipGetImageWidth(gpics[CARDCOUNTERS], &src_width);
	  GdipGetImageHeight(gpics[CARDCOUNTERS], &src_height);
	  src_width /= counters_num_columns;
	  src_height /= counters_num_rows;
	}
  else
	{
	  BITMAP bmp;
	  GetObject(pics[CARDCOUNTERS], sizeof(BITMAP), &bmp);
	  src_width = bmp.bmWidth / counters_num_columns;
	  src_height = bmp.bmHeight / counters_num_rows;
	}

  int scaled_height = r.bottom - r.top;
  int scaled_width = scaled_height * src_width / src_height;

  int spacing;
  if (num <= 1)
	spacing = scaled_width;
  else
	spacing = (r.right - r.left - scaled_width) / (num - 1);

  int src_xpos = (counter_type / counters_num_rows) * src_width;
  int src_ypos = (counter_type % counters_num_rows) * src_height;

  int dest_x = r.right - scaled_width;

  BLENDFUNCTION blend;
  int savedc = 0;

  if (counters_renderer != RENDERER_GDIPLUS)
	{
	  savedc = SaveDC(hdc);
	  SelectObject(*spare_hdc, pics[CARDCOUNTERS]);

	  blend.BlendOp = AC_SRC_OVER;
	  blend.BlendFlags = 0;
	  blend.SourceConstantAlpha = 255;
	  blend.AlphaFormat = AC_SRC_ALPHA;
	}

  int i;
  for (i = 0; i < num; ++i)
	{
	  r.left = dest_x;
	  r.right = dest_x + scaled_width;

	  if (counters_renderer == RENDERER_GDIPLUS)
		gdip_blt(hdc, &r, CARDCOUNTERS, src_xpos, src_ypos, src_width, src_height, NULL);
	  else
		AlphaBlend(hdc, r.left, r.top, r.right - r.left, r.bottom - r.top,
				   *spare_hdc, src_xpos, src_ypos, src_width, src_height, blend);

	  dest_x -= spacing;
	}

  if (counters_renderer != RENDERER_GDIPLUS)
	RestoreDC(hdc, savedc);

  if (parent == PARENT_SHANDALAR)
	LeaveCriticalSection(critical_section_for_drawing);
}

void
get_special_counter_rect(RECT* rect_dest, const RECT* rect_src, int num)
{
  // 0x4d3b60

  if (!rect_dest)
	return;

  if (!rect_src || num <= 0)
	{
	  SetRect(rect_dest, 0, 0, 0, 0);
	  return;
	}

  RECT r;
  r.left = rect_src->left + ((unsigned int)(18 * (rect_src->right - rect_src->left)) >> 8);
  r.right = rect_src->right - ((unsigned int)(51 * (rect_src->right - rect_src->left)) >> 8);
  r.top = rect_src->top + ((unsigned int)(20 * (rect_src->bottom - rect_src->top)) >> 8);
  r.bottom = r.top + 27 * (rect_src->bottom - rect_src->top) / 100;

  unsigned int src_width, src_height;
  if (counters_renderer == RENDERER_GDIPLUS)
	{
	  GdipGetImageWidth(gpics[CARDCOUNTERS], &src_width);
	  GdipGetImageHeight(gpics[CARDCOUNTERS], &src_height);
	  src_width /= counters_num_columns;
	  src_height /= counters_num_rows;
	}
  else
	{
	  BITMAP bmp;
	  GetObject(pics[CARDCOUNTERS], sizeof(BITMAP), &bmp);
	  src_width = bmp.bmWidth / counters_num_columns;
	  src_height = bmp.bmHeight / counters_num_rows;
	}

  int scaled_height = r.bottom - r.top;
  int scaled_width = scaled_height * src_width / src_height;

  int spacing;
  for (spacing = scaled_width;
	   spacing > 1 && r.left + scaled_width + spacing * (num - 1) > r.right;
	   --spacing)
	{}

  SetRect(rect_dest, r.left, r.top, r.left + scaled_width + spacing * (num - 1), r.bottom);
}

void
draw_standard_counters(HDC hdc, const RECT* rect, int player, int card)
{
  // This is a bit odd - in Manalink, we're already in a critical_section_for_drawing; in Shandalar, we're not.

  if (!hdc || !rect)
	return;

  card_instance_t actual;
  card_instance_t* instance = get_displayed_card_instance(player, card);	// unsafe to dereference

  if (parent == PARENT_MANALINK)
	{
	  LeaveCriticalSection(critical_section_for_drawing);

	  EnterCriticalSection(critical_section_for_display);
	  memcpy(&actual, instance, sizeof(card_instance_t));
	  LeaveCriticalSection(critical_section_for_display);

	  EnterCriticalSection(critical_section_for_drawing);
	}
  else
	{
	  EnterCriticalSection(critical_section_for_display);
	  memcpy(&actual, instance, sizeof(card_instance_t));
	  LeaveCriticalSection(critical_section_for_display);
	}

  instance = &actual;	// now safe

  int counters = instance->counters;
  int counters2 = instance->counters2;
  int counters3 = instance->counters3;
  int counters4 = instance->counters4;
  int counters5 = instance->counters5;
  int counters_m1m1 = instance->counters_m1m1;

  int counter_type_2;
  int counter_type_3;
  int counter_type_4;
  int counter_type_5;

  if (parent == PARENT_SHANDALAR)
	{
	  counter_type_2 = instance->unk52;
	  counter_type_3 = instance->unk53;
	  counter_type_4 = instance->card_color;
	  counter_type_5 = instance->unk5a;
	}
  else
	{
	  uint8_t* t18 = (void*)(&instance->targets[18].player);
	  counter_type_2 = t18[0];
	  counter_type_3 = t18[1];
	  counter_type_4 = t18[2];
	  counter_type_5 = t18[3];
	}

  if (!counters && !counters2 && !counters3 && !counters4 && !counters5 && !counters_m1m1)
	return;

  if (parent == PARENT_SHANDALAR)
	EnterCriticalSection(critical_section_for_drawing);

  unsigned int src_width, src_height;
  if (counters_renderer == RENDERER_GDIPLUS)
	{
	  GdipGetImageWidth(gpics[CARDCOUNTERS], &src_width);
	  GdipGetImageHeight(gpics[CARDCOUNTERS], &src_height);
	  src_width /= counters_num_columns;
	  src_height /= counters_num_rows;
	}
  else
	{
	  BITMAP bmp;
	  GetObject(pics[CARDCOUNTERS], sizeof(BITMAP), &bmp);
	  src_width = bmp.bmWidth / counters_num_columns;
	  src_height = bmp.bmHeight / counters_num_rows;
	}

  int scaled_height = rect->bottom - rect->top;
  int scaled_width = scaled_height * src_width / src_height;

  int one_third_card_width = (rect->right - rect->left) / 3;

  BLENDFUNCTION blend;
  RECT r;
  int spacing, dest_x, src_xpos, src_ypos, i, savedc = 0;

  if (counters_renderer != RENDERER_GDIPLUS)
	{
	  savedc = SaveDC(hdc);
	  SelectObject(*spare_hdc, pics[CARDCOUNTERS]);

	  blend.BlendOp = AC_SRC_OVER;
	  blend.BlendFlags = 0;
	  blend.SourceConstantAlpha = 255;
	  blend.AlphaFormat = AC_SRC_ALPHA;
	}

#define COUNTERS(which, init_x, init_y, imgnum, lambda)	do {																\
  if (which > 0 && imgnum != 255)																							\
	{																														\
	  for (spacing = scaled_width; scaled_width + spacing * ((which) - 1) > one_third_card_width && spacing > 2; --spacing)	\
		{}																													\
	  dest_x = (init_x) + spacing * ((which) - 1);																			\
	  src_xpos = ((imgnum) / counters_num_rows) * src_width;																\
	  src_ypos = ((imgnum) % counters_num_rows) * src_height;																\
	  r.top = (init_y);																										\
	  r.bottom = r.top + scaled_height;																						\
	  for (i = 0; i < (which); ++i)																							\
		{																													\
		  r.left = dest_x;																									\
		  r.right = r.left + scaled_width;																					\
																															\
		  { lambda; }																										\
																															\
		  if (counters_renderer == RENDERER_GDIPLUS)																		\
			gdip_blt(hdc, &r, CARDCOUNTERS, src_xpos, src_ypos, src_width, src_height, NULL);								\
		  else																												\
			AlphaBlend(hdc, r.left, r.top, r.right - r.left, r.bottom - r.top,												\
					   *spare_hdc, src_xpos, src_ypos, src_width, src_height, blend);										\
																															\
		  dest_x -= spacing;																								\
		}																													\
	}																														\
  } while (0)

  int left_edge_of_dest = rect->left;
  int top_edge_of_dest = rect->top;
  COUNTERS(counters4, left_edge_of_dest, top_edge_of_dest, counter_type_4,);

  int left_third_of_dest = left_edge_of_dest + one_third_card_width;
  COUNTERS(counters3, left_third_of_dest, top_edge_of_dest, counter_type_3,);

  int right_third_of_dest = left_third_of_dest + one_third_card_width;
  COUNTERS(counters2, right_third_of_dest, top_edge_of_dest, counter_type_2,);

  int middle_of_dest = (rect->right + rect->left) / 2;
  int left_sixth_of_dest = middle_of_dest - one_third_card_width;
  int bottom_third_of_dest = 2 * scaled_height / 3 + top_edge_of_dest;
  int p1p1m1m1 = counters + counters_m1m1, changed = 0;
  COUNTERS(p1p1m1m1, left_sixth_of_dest, bottom_third_of_dest, COUNTER_P1_P1,
		   {
			 if (!changed && i >= counters)
			   {
				 changed = 1;
				 src_xpos = (COUNTER_M1_M1 / counters_num_rows) * src_width;
				 src_ypos = (COUNTER_M1_M1 % counters_num_rows) * src_height;
			   }
		   });

  COUNTERS(counters5, middle_of_dest, bottom_third_of_dest, counter_type_5,);

  if (counters_renderer != RENDERER_GDIPLUS)
	RestoreDC(hdc, savedc);

  if (parent == PARENT_SHANDALAR)
	LeaveCriticalSection(critical_section_for_drawing);

#undef COUNTERS
}
