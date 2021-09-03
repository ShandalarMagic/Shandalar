#include "manalink.h"

int count_graveyard_by_color(int player, int selected_color){
	if( get_global_color_hack(player) & selected_color ){
		return count_graveyard(player);
	}

	const int *grave = get_grave(player);
	int count = 0;
	int result = 0;
	while( grave[count] != -1 ){
			if( ! is_what(-1, grave[count], TYPE_LAND) && (cards_data[grave[count]].color & selected_color) ){
				result++;
			}
			count++;
	}

	return result;
}


int count_graveyard_by_id(int player, int id){
	int i;
	int result = 0;
	for(i=0; i<2; i++){
		if( i == player || player == 2 ){
			const int *grave = get_grave(i);
			int count = 0;
			while( grave[count] != -1 ){
					if( cards_data[ grave[count] ].id == id ){
						result++;
					}
					count++;
			}
		}
	}
	return result;
}

// Count the number of cards in a zone
int count_zone(int player, const int* zone){
  int i;

  for (i = 0; i < 500; i++)
	if (zone[i] == -1)
	  return i;

  return 500;
}

int count_deck(int player){
	return count_zone(player, deck_ptr[player]);
}

int count_graveyard(int player){
	int result = 0;
	int i;
	for(i=0; i<2; i++){
		if( i == player || player == 2 ){
			result += count_zone(i, get_grave(i));
		}
	}
	return result;
}

int count_rfg(int player){
	return count_zone(player, rfg_ptr[player]);
}

void purge_rfg(int player){
	int *rfg = rfg_ptr[player];
	rfg[count_rfg(player) - 1] = -1;
}

static int count_zone_by_type(int player, int type, const int* zone){
	int count = 0;
	int i;

	for(i = 0; i < 500 && zone[i] != -1; i++){
		if( is_what(-1, zone[i], type) ){
			count++;
		}
	}

	return count;
}

int count_graveyard_by_type(int player, int type){
	int i;
	int result = 0;
	for(i=0; i<2; i++){
		if( i == player || player == 2 ){
			result += count_zone_by_type(i, type, get_grave(i));
		}
	}
	return result;
}

// Equivalent to count_graveyard_by_type(player, type) > 0, but stops when it finds the first qualifying card.
int any_in_graveyard_by_type(int player, int type)
{
  int p, i;
  for (p = 0; p <= 1; ++p)
	if (p == player || player == ANYBODY)
	  for (i = 0; i < 500 && get_grave(p)[i] != -1; ++i)
		if (is_what(-1, get_grave(p)[i], type))
		  return 1;
  return 0;
}

int count_graveyard_by_subtype(int player, subtype_t subtype)
{
  int p, i, count = 0;
  for (p = 0; p <= 1; ++p)
	if (p == player || player == ANYBODY)
	  for (i = 0; i < 500 && get_grave(p)[i] != -1; ++i)
		if (has_subtype(-1, get_grave(p)[i], subtype))
			++count;
		  return 1;
  return 0;
}

int any_in_graveyard_by_subtype(int player, subtype_t subtype)
{
  int p, i;
  for (p = 0; p <= 1; ++p)
	if (p == player || player == ANYBODY)
	  for (i = 0; i < 500 && get_grave(p)[i] != -1; ++i)
		if (has_subtype(-1, get_grave(p)[i], subtype))
		  return 1;
  return 0;
}

int count_deck_by_type(int player, int type){
	return count_zone_by_type(player, type, deck_ptr[player]);
}

int new_special_count_grave(int player, test_definition_t *this_test){
	int result = 0;
	int test_type = new_get_test_score(this_test);
	int i;
	for(i=0; i<2; i++){
		if( i == player || player == 2 ){
			int count = 0;
			const int *grave = get_grave(i);
			while( grave[count] != -1 ){
					if( new_make_test(player, grave[count], test_type, this_test) ){
						result++;
					}
					count++;
			}
		}
	}
	return result;
}

int special_count_grave(int player, int type, int flag1, int subtype, int flag2, int color, int flag3, int id, int flag4, int cc, int flag5){

	int count = count_graveyard(player)-1;
	int result = 0;
	const int *grave = get_grave(player);
	int test_type = get_test_score(type, flag1, subtype, flag2, color, flag3, id, flag4, cc, flag5);
	while( count > - 1 ){
			if( make_test(player, grave[count], test_type, type, flag1, subtype, flag2, color, flag3, id,
						  flag4, cc, flag5)
			  ){
				result++;
			}
			count--;
	}
	return result;
}
