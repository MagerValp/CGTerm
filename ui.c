#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "gfx.h"
#include "menu.h"
#include "config.h"
#include "keyboard.h"
#include "net.h"
#include "kernal.h"
#include "xfer.h"
#include "macro.h"
#include "ui.h"


int input_maxlen = 256;
int input_width;
int input_offset;
int input_pos;
int input_len;
Focus input_focus;
void (*input_done_call)(char *);
char input_buffer[256];
char input_display_buffer[100];

void inputupdate(void) {
  strncpy(input_display_buffer, input_buffer + input_offset, input_width);
  menu_update_input(input_display_buffer, input_pos);
}


void ui_inputcall(int width, char *title, char *text, void (*donecall)(char *), Focus focus) {
  input_done_call = donecall;
  input_focus = focus;
  if (text) {
    strncpy(input_buffer, text, sizeof(input_buffer));
  } else {
    input_buffer[0] = 0;
  }
  input_width = width;
  if (text) {
    input_pos = input_len = strlen(text);
  } else {
    input_pos = input_len = 0;
  }
  if (input_len >= input_width) {
    input_offset = input_len - input_width + 1;
    input_pos -= input_offset;
  } else {
    input_offset = 0;
  }
  menu_draw_input(title);
  inputupdate();
  menu_show();
  kbd_focus = FOCUS_INPUTCALL;
}


void ui_inputkey(SDL_keysym *keysym) {
  int i;
  char c;

  switch (keysym->sym) {

  default:
    if (keysym->unicode <= 126 && keysym->unicode >= 32) {
      c = keysym->unicode;
      if (input_len < input_maxlen) {
	++input_len;
	memmove(input_buffer + input_pos + input_offset + 1,
		input_buffer + input_pos + input_offset,
		strlen(input_buffer + input_pos + input_offset) + 1);
	input_buffer[input_pos + input_offset] = c;
	input_buffer[input_len] = 0;

	if (input_pos + input_offset < input_len) {
	  if (input_pos < input_width - 1) {
	    ++input_pos;
	  } else if (input_len > input_offset + input_pos) {
	    ++input_offset;
	  }
	}

	inputupdate();
      }
    }
    break;

  case SDLK_RIGHT:
    if (input_pos + input_offset < input_len) {
      if (input_pos < input_width - 1) {
	++input_pos;
      } else if (input_len > input_offset + input_pos) {
	++input_offset;
      }
    }
    inputupdate();
    break;

  case SDLK_LEFT:
    if (input_pos > 0) {
      --input_pos;
    } else if (input_offset > 0) {
      --input_offset;
    }
    inputupdate();
    break;

  case SDLK_HOME:
    if (keysym->mod & KMOD_SHIFT) {
      input_pos = input_offset = input_len = 0;
      input_buffer[0] = 0;
    } else {
      if (input_pos || input_offset) {
	input_pos = input_offset = 0;
      }
    }
    inputupdate();
    break;

  case SDLK_RETURN:
  case SDLK_KP_ENTER:
    menu_hide();
    if (input_len) {
      kbd_focus = input_focus;
      input_done_call(input_buffer);
    } else {
      kbd_focus = FOCUS_TERM; // maybe
    }
    break;

  case SDLK_ESCAPE:
    menu_hide();
    kbd_focus = FOCUS_TERM; // maybe
    break;

  case SDLK_BACKSPACE:
    if (input_pos || input_offset) {
      if (input_pos) {
	--input_pos;
      } else {
	--input_offset;
      }
      i = input_offset + input_pos;
      while (input_buffer[i]) {
	input_buffer[i] = input_buffer[i+1];
	++i;
      }
      --input_len;
      inputupdate();
    }
    break;

  case SDLK_DELETE:
    if (input_pos + input_offset < input_len) {
      i = input_offset + input_pos;
      while (input_buffer[i]) {
	input_buffer[i] = input_buffer[i+1];
	++i;
      }
      --input_len;
      inputupdate();
    }
    break;

  }
}


void ui_display_net_status(int code, char *message) {
  if (code == 1) {
    kbd_focus = FOCUS_TERM;
    menu_hide();
  } else {
    menu_draw_message(message);
    menu_show();
    kbd_focus = FOCUS_REQUESTER;
  }
}


void ui_connect(char *host) {
  char *port;

  if ((port = strchr(input_buffer, ' '))) {
    *port++ = 0;
    cfg_port = strtol(port, (char **)NULL, 10);
    if (cfg_port <= 0 || cfg_port > 65535) {
      ui_display_net_status(2, "Illegal port number");
      return;
    }
  } else {
    cfg_port = 23;
  }
  cfg_sethost(input_buffer);
  net_connect(cfg_host, cfg_port, &ui_display_net_status);
}


void ui_menukey(SDL_keysym *keysym) {
  switch (keysym->sym) {

  case SDLK_ESCAPE:
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  default:
    menu_hide();
    kbd_focus = FOCUS_TERM;
    ui_metakey(keysym);
    break;
  }
}


void ui_requestkey(SDL_keysym *keysym) {
  switch (keysym->sym) {

  case SDLK_RETURN:
  case SDLK_ESCAPE:
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  default:
    break;
  }
}


void ui_bookmarkkey(SDL_keysym *keysym) {
  int b;

  switch (keysym->sym) {

  case SDLK_RETURN:
  case SDLK_ESCAPE:
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  default:
    if (keysym->sym >= SDLK_0 && keysym->sym <= SDLK_9) {
      b = keysym->sym - SDLK_0;
      if (b < cfg_numbookmarks) {
	menu_hide();
	kbd_focus = FOCUS_TERM;
	cfg_sethost(cfg_bookmark_host[b]);
	cfg_port = cfg_bookmark_port[b];
	net_connect(cfg_bookmark_host[b], cfg_bookmark_port[b], &ui_display_net_status);
      }
    }
    break;
  }
}


void ui_pageup(void) {
  if (gfx_offset > cfg_columns) {
    gfx_set_offset(gfx_offset - cfg_columns);
  }
}


void ui_pagedown(void) {
  if (gfx_offset < gfx_maxoffset) {
    gfx_set_offset(gfx_offset + cfg_columns);
  }
}


void ui_init(void) {
  kbd_add_focus(FOCUS_MENU, &ui_menukey);
  kbd_add_focus(FOCUS_INPUTCALL, &ui_inputkey);
  kbd_add_focus(FOCUS_REQUESTER, &ui_requestkey);
  kbd_add_focus(FOCUS_BOOKMARKS, &ui_bookmarkkey);
  ui_local_init();
}
