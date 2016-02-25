#include <SDL/SDL.h>


unsigned int timer_get_ticks(void) {
  return(SDL_GetTicks());
}


void timer_delay(unsigned int delay) {
  SDL_Delay(delay);
}
