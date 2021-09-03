#include "manalink.h"

int has_threshold(int player){
	const int *grave = get_grave(player);
	if( grave[6] != -1 ){
		return 1;
	}
	return 0;
}
