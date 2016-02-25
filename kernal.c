#include <stdio.h>
#include <SDL.h>
#include "gfx.h"
#include "keyboard.h"
#include "kernal.h"
#include "sound.h"


unsigned char screencode[256];
signed int scconv[] = {
  128,
  0,
  -64,
  -32,
  64,
  -64,
  -128,
  -128
};
unsigned char enableshiftcbm;
unsigned char rvson;


int kernal_init(void) {
  int c;

  for (c = 0; c < 256; ++c) {
    screencode[c] = c + scconv[c / 32];
  }
  screencode[255] = 94;
  gfx_setcursxy(0, 0);
  enableshiftcbm = 1;
  rvson = 0;

  gfx_setfont(1);
  gfx_bgcolor(0);
  ffd2(154);
  ffd2(147);

  return(0);
}


void ffd2(unsigned char a) {

  switch (a) {

    /* colors */

  case 5:
    gfx_fgcolor(COLOR_WHITE);
    break;

  case 28:
    gfx_fgcolor(COLOR_RED);
    break;

  case 30:
    gfx_fgcolor(COLOR_GREEN);
    break;

  case 31:
    gfx_fgcolor(COLOR_BLUE);
    break;

  case 129:
    gfx_fgcolor(COLOR_ORANGE);
    break;

  case 144:
    gfx_fgcolor(COLOR_BLACK);
    break;

  case 149:
    gfx_fgcolor(COLOR_BROWN);
    break;

  case 150:
    gfx_fgcolor(COLOR_LTRED);
    break;

  case 151:
    gfx_fgcolor(COLOR_DKGRAY);
    break;

  case 152:
    gfx_fgcolor(COLOR_GRAY);
    break;

  case 153:
    gfx_fgcolor(COLOR_LTGREEN);
    break;

  case 154:
    gfx_fgcolor(COLOR_LTBLUE);
    break;

  case 155:
    gfx_fgcolor(COLOR_LTGRAY);
    break;

  case 156:
    gfx_fgcolor(COLOR_PURPLE);
    break;

  case 158:
    gfx_fgcolor(COLOR_YELLOW);
    break;

  case 159:
    gfx_fgcolor(COLOR_CYAN);
    break;


    /* movement */

  case 10:
    break;

  case 13:
  case 141:
    gfx_setcursx(0);
    rvson = 0;
    /* fall through */

  case 17:
    gfx_cursdown();
    break;

  case 147:
    gfx_setcursxy(0, 0);
    gfx_cls();
    break;

  case 19:
    gfx_setcursxy(0, 0);
    break;

  case 20:
    gfx_delete();
    break;

  case 157:
    gfx_cursleft();
    break;

  case 29:
    gfx_cursright();
    break;

  case 145:
    gfx_cursup();
    break;

  case 148:
    gfx_insert();
    break;


    /* fonts */

  case 8:
    enableshiftcbm = 0;
    /* printf("shift+cbm disabled\n"); */
    break;

  case 9:
    enableshiftcbm = 1;
    /* printf("shift+cbm enabled\n"); */
    break;

  case 14:
    gfx_setfont(1);
    /* printf("switching to lower case\n"); */
    break;

  case 142:
    gfx_setfont(0);
    /* printf("switching to upper case\n"); */
    break;

  case 18:
    rvson = 1;
    break;

  case 146:
    rvson = 0;
    break;


    /* terminal */

  case 3:
    break;

  case 7:
    sound_play_sample(sound_bell);
    break;


    /* printables */

  default:
    if ((a >= 32 && a <= 127) || (a >= 160)) {
      gfx_draw_char(screencode[a] + rvson * 128);
      gfx_cursadvance();
    }
    break;

  }

}


void print(unsigned char *s) {
  while (*s) {
    ffd2(*s++);
  }
}


void print_ascii(unsigned char *s) {
  char c;

  while ((c = *s++)) {
    if (c >= 'A' && c <= 'Z') {
      c += 32;
    } else if (c >= 'a' && c <= 'z') {
      c -= 32;
    }
    ffd2(c);
  }
}


unsigned char kbd_unshift[65] = {
  0x14, 0x0d, 0x1d, 0x88, 0x85, 0x86, 0x87, 0x11,
  0x33, 0x57, 0x41, 0x34, 0x5a, 0x53, 0x45, 0x01,
  0x35, 0x52, 0x44, 0x36, 0x43, 0x46, 0x54, 0x58,
  0x37, 0x59, 0x47, 0x38, 0x42, 0x48, 0x55, 0x56,
  0x39, 0x49, 0x4a, 0x30, 0x4d, 0x4b, 0x4f, 0x4e,
  0x2b, 0x50, 0x4c, 0x2d, 0x2e, 0x3a, 0x40, 0x2c,
  0x5c, 0x2a, 0x3b, 0x13, 0x01, 0x3d, 0x5e, 0x2f,
  0x31, 0x5f, 0x04, 0x32, 0x20, 0x02, 0x51, 0x03,
  0
};

unsigned char kbd_shift[65] = {
  0x94, 0x8d, 0x9d, 0x8c, 0x89, 0x8a, 0x8b, 0x91,
  0x23, 0xd7, 0xc1, 0x24, 0xda, 0xd3, 0xc5, 0x01,
  0x25, 0xd2, 0xc4, 0x26, 0xc3, 0xc6, 0xd4, 0xd8,
  0x27, 0xd9, 0xc7, 0x28, 0xc2, 0xc8, 0xd5, 0xd6,
  0x29, 0xc9, 0xca, 0x30, 0xcd, 0xcb, 0xcf, 0xce,
  0xdb, 0xd0, 0xcc, 0xdd, 0x3e, 0x5b, 0xba, 0x3c,
  0xa9, 0xc0, 0x5d, 0x93, 0x01, 0x3d, 0xde, 0x3f,
  0x21, 0x5f, 0x04, 0x22, 0xa0, 0x02, 0xd1, 0x83,
  0
};

unsigned char kbd_cbm[65] = {
  0x94, 0x8d, 0x9d, 0x8c, 0x89, 0x8a, 0x8b, 0x91,
  0x96, 0xb3, 0xb0, 0x97, 0xad, 0xae, 0xb1, 0x01,
  0x98, 0xb2, 0xac, 0x99, 0xbc, 0xbb, 0xa3, 0xbd,
  0x9a, 0xb7, 0xa5, 0x9b, 0xbf, 0xb4, 0xb8, 0xbe,
  0x29, 0xa2, 0xb5, 0x30, 0xa7, 0xa1, 0xb9, 0xaa,
  0xa6, 0xaf, 0xb6, 0xdc, 0x3e, 0x5b, 0xa4, 0x3c,
  0xa8, 0xdf, 0x5d, 0x93, 0x01, 0x3d, 0xde, 0x3f,
  0x81, 0x5f, 0x04, 0x95, 0xa0, 0x02, 0xab, 0x83,
  0
};

unsigned char kbd_ctrl[65] = {
  0000, 0000, 0000, 0000, 0000, 0000, 0000, 0000,
  0x1c, 0x17, 0x01, 0x9f, 0x1a, 0x13, 0x05, 0000,
  0x9c, 0x12, 0x04, 0x1e, 0x03, 0x06, 0x14, 0x18,
  0x1f, 0x19, 0x07, 0x9e, 0x02, 0x08, 0x15, 0x16,
  0x12, 0x09, 0x0a, 0x92, 0x0d, 0x0b, 0x0f, 0x0e,
  0000, 0x10, 0x0c, 0000, 0000, 0x1b, 0x00, 0000,
  0x1c, 0000, 0x1d, 0000, 0000, 0x1f, 0x1e, 0000,
  0x90, 0x06, 0000, 0x05, 0000, 0000, 0x11, 0000,
  0
};

unsigned char ffe4(void) {
  int k;
  unsigned char shift, ctrl, cbm;

  k = kbd_getkey(&shift, &ctrl, &cbm);
  if (k > 255) {
    return(k - 256);
  }

  if (shift) {
    return(kbd_shift[k]);
  } else if (cbm) {
    return(kbd_cbm[k]);
  } else if (ctrl) {
    return(kbd_ctrl[k]);
  } else {
    return(kbd_unshift[k]);
  }

}
