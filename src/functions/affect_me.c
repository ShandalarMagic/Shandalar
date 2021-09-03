#include "manalink.h"

int affect_me(int player, int card)
{
  if(player == affected_card_controller && card == affected_card ){
    return 1;
  }
  return 0;
}
