#include <stdlib.h>
#include <SDL.h>
#include "kernal.h"
#include "gfx.h"
#include "menu.h"
#include "config.h"
#include "macro.h"
#include "net.h"
#include "keyboard.h"
#include "ui.h"


struct menu chatmenu[] = {
  {1, "A", "Abort load or macro"},
  {2, "B", "Open bookmarks"},
  {3, "C", "Start/stop recording macro"},
  {4, "D", "Connect/disconnect"},
  {5, "F", "Toggle fullscreen mode"},
  {6, "L", "Load seq file"},
  {7, "Q", "Quit CGChat"},
  {8, "R", "Reconnect"},
  {9, "S", "Save screen to seq file"},
  {10, "V", "Play macro"},
  {11, "Alt", "Toggle case"},
  {0, NULL, NULL}
};


void ui_metakey(SDL_keysym *keysym) {
  switch (keysym->sym) {

  case SDLK_LALT:
  case SDLK_RALT:
    gfx_toggle_font();
    break;

  case SDLK_a:
    kbd_loadseq_abort();
    if (macro_play) {
      macro_play = 0;
    }
    break;

  case SDLK_b:
    if (net_connected()) {
      menu_draw_message("Disconnect first.");
      menu_show();
      kbd_focus = FOCUS_REQUESTER;
    } else {
      menu_draw_bookmarks();
      menu_show();
      kbd_focus = FOCUS_BOOKMARKS;
    }
    break;
    
  case SDLK_c:
    if (macro_rec) {
      macro_rec = 0;
    } else {
      macro_len = 0;
      macro_rec = 1;
    }
    break;

  case SDLK_d:
    if (net_connected()) {
      net_disconnect();
    } else {
      ui_inputcall(30, "Connect to host [port]:", cfg_host, &ui_connect, FOCUS_TERM);
    }
    break;

  case SDLK_f:
    gfx_toggle_fullscreen();
    break;

  case SDLK_l:
    ui_inputcall(30, "Load SEQ file:", "screen.seq", &kbd_loadseq, FOCUS_TERM);
    break;

  case SDLK_q:
    exit(0);
    break;

  case SDLK_r:
    if (!net_connected()) {
      if (cfg_host) {
	net_connect(cfg_host, cfg_port, &ui_display_net_status);
      }
    }
    break;

  case SDLK_s:
    ui_inputcall(30, "Save SEQ file:", "screen.seq", &gfx_savescreen, FOCUS_TERM);
    break;

  case SDLK_v:
    if (macro_len) {
      macro_play = 1;
      macro_ctr = 0;
    }
    break;

  default:
    break;
  }
}


void ui_menu(void) {
  menu_print_menu(chatmenu);
  menu_show();
  kbd_focus = FOCUS_MENU;
}


void ui_local_init(void) {
  return;
}
