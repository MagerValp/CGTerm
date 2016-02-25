#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "config.h"
#include "keyboard.h"
#include "macro.h"
#include "ui.h"


int focus_count = 0;
Focus focus_focus[10];
void (*focus_handler[10])(SDL_keysym *);

Focus kbd_focus;
static SDL_bool seqfile_open = SDL_FALSE;
static FILE *seqfile;
static char keytable[SDLK_LAST];
static char *keys[] = {
  "del",
  "return",
  "right",
  "f7",
  "f1",
  "f3",
  "f5",
  "down",
  "three",
  "w",
  "a",
  "four",
  "z",
  "s",
  "e",
  " left shift ",
  "five",
  "r",
  "d",
  "six",
  "c",
  "f",
  "t",
  "x",
  "seven",
  "y",
  "g",
  "eight",
  "b",
  "h",
  "u",
  "v",
  "nine",
  "i",
  "j",
  "zero",
  "m",
  "k",
  "o",
  "n",
  "plus",
  "p",
  "l",
  "minus",
  "dot",
  "colon",
  "at",
  "comma",
  "pound",
  "star",
  "semicolon",
  "home",
  " right shift ",
  "equal",
  "arrowup",
  "slash",
  "one",
  "arrowleft",
  " control ",
  "two",
  "space",
  " commodore ",
  "q",
  "stop"
};


int matchkey(char *name) {
  int k;

  for (k = 0; k < 64; ++k) {
    if (strcmp(keys[k], name) == 0) {
      return(k);
    }
  }
  return(k);
}


int kbd_init(char *keyboardcfg) {
  char linebuf[256], keyname[16];
  int line = 0, value, key;
  FILE *in;

  SDL_EnableUNICODE(1);

  kbd_focus = FOCUS_TERM;

  memset(keytable, 64, sizeof(keytable));

  if ((in = fopen(keyboardcfg, "r")) == NULL) {
    printf("Couldn't open %s\n", keyboardcfg);
    return(1);
  }

  while (fgets(linebuf, 256, in) != NULL) {

    if (strlen(linebuf) >= sizeof(linebuf) - 1) {
      printf("line %d too long\n", line);
      fclose(in);
      return(1);
    }

    if (linebuf[0] == '#') {
    } else if (strlen(linebuf) >= 3) {

      if (sscanf(linebuf, "%15s %d \n", keyname, &value) == 2) {
	if ((key = matchkey(keyname)) != 64) {
	  keytable[value] = key;
	} else {
	  printf("Unknown key: %s\n", keyname);
	}
      } else {
	printf("Syntax error on line %d\n", line);
      }

    } else {

      if (!sscanf(linebuf, "%s", linebuf)) {
	printf("Syntax error on line %d\n", line);
      }

    }

    ++line;
  }

  if (ferror(in)) {
    printf("read error\n");
    fclose(in);
    return(1);
  }

  fclose(in);

  ui_init();

  return(0);
}


void kbd_add_focus(Focus focus, void (*handler)(SDL_keysym *)) {
  focus_focus[focus_count] = focus;
  focus_handler[focus_count] = handler;
  ++focus_count;
}


void kbd_loadseq(char *filename) {
  if (!seqfile_open) {
    if ((seqfile = fopen(filename, "rb"))) {
      seqfile_open = SDL_TRUE;
    } else {
      printf("opening %s failed\n", filename);
    }
  } else {
    puts("file is already open");
  }
}


void kbd_loadseq_abort(void) {
  if (seqfile_open) {
    fclose(seqfile);
    seqfile_open = SDL_FALSE;
  }
}


unsigned char translatekey(SDL_keysym *keysym, unsigned char *shift, unsigned char *ctrl, unsigned char *cbm) {
  *shift = keysym->mod & (KMOD_SHIFT | KMOD_CAPS) ? 1 : 0;
  *ctrl = keysym->mod & KMOD_CTRL ? 1 : 0;
  *cbm = keysym->mod & KMOD_ALT ? 1 : 0;
  switch (keysym->sym) {

  case SDLK_ESCAPE:
    ui_menu();
    return(64);
    break;

  case SDLK_PAGEUP:
    ui_pageup();
    return(64);
    break;

  case SDLK_PAGEDOWN:
    ui_pagedown();
    return(64);
    break;

  case SDLK_UP:
    *shift = 1;
    return(keytable[SDLK_DOWN]);
    break;

  case SDLK_LEFT:
    *shift = 1;
    return(keytable[SDLK_RIGHT]);
    break;

  case SDLK_F2:
    if (keytable[SDLK_F2] == 64) {
      *shift = 1;
      return(keytable[SDLK_F1]);
    }
    break;

  case SDLK_F4:
    if (keytable[SDLK_F4] == 64) {
      *shift = 1;
      return(keytable[SDLK_F3]);
    }
    break;

  case SDLK_F6:
    if (keytable[SDLK_F6] == 64) {
      *shift = 1;
      return(keytable[SDLK_F5]);
    }
    break;

  case SDLK_F8:
    if (keytable[SDLK_F8] == 64) {
      *shift = 1;
      return(keytable[SDLK_F7]);
    }
    break;

  default:
    break;
  }

  return(keytable[keysym->sym]);
}


int kbd_getkey(unsigned char *shift, unsigned char *ctrl, unsigned char *cbm) {
  SDL_Event event;
  int c, f;
  unsigned char key;

  if (SDL_PollEvent(&event)) {
    switch (event.type) {

    case SDL_QUIT:
      exit(1);
      break;

    case SDL_KEYDOWN:

      if (kbd_focus == FOCUS_TERM) {
	if (event.key.keysym.mod & KMOD_META) {
	  ui_metakey(&event.key.keysym);
	} else {
	  key = translatekey(&event.key.keysym, shift, ctrl, cbm);
	  if (macro_rec && key != 64) {
	    macrobuf_key[macro_len] = key;
	    macrobuf_shift[macro_len] = *shift;
	    macrobuf_ctrl[macro_len] = *ctrl;
	    macrobuf_cbm[macro_len] = *cbm;
	    if (++macro_len == macro_maxlen) {
	      macro_rec = 0;
	    }
	  }
	  return(key);
	}
      } else {
	for (f = 0; f < focus_count; ++f) {
	  if (f >= focus_count) {
	    printf("focus_fixme\n");
	    exit(1);
	  }
	  if (kbd_focus == focus_focus[f]) {
	    focus_handler[f](&event.key.keysym);
	    f = focus_count;
	  }
	}
      }
      break;

    }
  }

  if (seqfile_open) {
    if ((c = fgetc(seqfile)) == EOF) {
      fclose(seqfile);
      seqfile_open = SDL_FALSE;
      return(64);
    } else {
      return(c + 256);
    }
  }

  if (macro_play) {
    key = macrobuf_key[macro_ctr];
    *shift = macrobuf_shift[macro_ctr];
    *ctrl = macrobuf_ctrl[macro_ctr];
    *cbm = macrobuf_cbm[macro_ctr];
    if (++macro_ctr == macro_len) {
      macro_play = 0;
    }
    return(key);
  }

  return(64);

}
