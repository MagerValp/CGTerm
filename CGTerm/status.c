#include <time.h>
#include <string.h>
#include "config.h"
#include "gfx.h"


unsigned char *status_time;
unsigned char status_chars[80];
unsigned char status_colors[80];

static unsigned char status_chars40[] = {
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 115, '0', '0', ':', '0', '0', 107, 64, 64,
};
static unsigned char status_colors40[] = {
  6, 4, 10, 7, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 15, 15, 12, 15, 15, 1, 1, 1
};
static unsigned char status_chars80[] = {
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
  64, 115, '0', '0', ':', '0', '0', 107, 64, 64,
};
static unsigned char status_colors80[] = {
  6, 4, 10, 7, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 1, 15, 15, 12, 15, 15, 1, 1, 1
};


void status_init(void) {
  memcpy(status_chars, cfg_columns == 40 ? status_chars40 : status_chars80, cfg_columns);
  memcpy(status_colors, cfg_columns == 40 ? status_colors40 : status_colors80, cfg_columns);
  status_time = status_chars + cfg_columns - 8;
}


void status_clear(void) {
  gfx_clear_line(23, 1);
}


void status_draw(void) {
  struct tm *tm;
  time_t t;

  t = time(NULL);
  tm = localtime(&t);
  status_time[0] = '0' + tm->tm_hour / 10;
  status_time[1] = '0' + tm->tm_hour % 10;
  status_time[3] = '0' + tm->tm_min / 10;
  status_time[4] = '0' + tm->tm_min % 10;
  gfx_copy_line(status_chars, status_colors, 23);
}
