#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "gfx.h"
#include "menu.h"
#include "config.h"
#include "keyboard.h"
#include "net.h"
#include "xfer.h"
#include "kernal.h"
#include "dir.h"
#include "diskimage.h"
#include "fileselector.h"
#include "macro.h"
#include "ui.h"


struct menu editmenu[] = {
  {1, "A", "Abort load or macro"},
  {2, "B", "Background color"},
  {3, "C", "Start/stop recording macro"},
  {4, "D", "Cursor direction"},
  {5, "F", "Toggle fullscreen mode"},
  {6, "I", "Set transfer disk image"},
  {7, "L", "Load seq file"},
  {8, "M", "Edit mode"},
  {9, "Q", "Quit CGEdit"},
  {10, "R", "Rectangle commands"},
  {11, "S", "Save screen to seq file"},
  {12, "V", "Play macro"},
  {13, "Alt", "Toggle case"},
  {0, NULL, NULL}
};


static int ui_bkgcol = 0;

static int rect_x = 0;
static int rect_y = 0;
static int rect_w = 0;
static int rect_h = 0;
static unsigned char rect_0400[2000];
static unsigned char rect_d800[2000];

typedef enum selmode {
  SEL_DIR,
  SEL_FILE
} SelectMode;
FileSelector *fsel;
void (*select_done_call)(FileSelector *);
Focus select_focus;
SelectMode select_mode;


void kbd_select_dir(void (*donecall)(FileSelector *), Focus focus) {
  select_done_call = donecall;
  select_focus = focus;
  select_mode = SEL_DIR;
  fsel = fs_new("Select disk image", cfg_xferdir);
  if (fsel) {
    fs_draw(fsel);
    menu_show();
    kbd_focus = FOCUS_SELECTDIR;
  }
}


void ui_selectdirkey(SDL_keysym *keysym) {
  if (fsel->numentries == 0) {
    menu_hide();
    kbd_focus = FOCUS_TERM;
  }

  switch (keysym->sym) {

  case SDLK_HOME:
    fsel->current = 0;
    fsel->offset = 0;
    fs_draw(fsel);
    break;

  case SDLK_ESCAPE:
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  case SDLK_DELETE:
  case SDLK_BACKSPACE:
    cfg_change_dir(fsel->path, "..");
    fs_read_dir(fsel, fsel->path);
    fs_draw(fsel);
    break;

  case SDLK_SPACE:
    if (select_mode == SEL_DIR) {
      fsel->selectedfile = dir_find(fsel->dir, fsel->current + fsel->offset);
      if (fsel->selectedfile->type == T_DIR) {
	cfg_change_dir(fsel->path, fsel->selectedfile->name);
	fs_read_dir(fsel, fsel->path);
	fs_draw(fsel);
      }
    }
    break;

  case SDLK_RETURN:
  case SDLK_KP_ENTER:
    fsel->selectedfile = dir_find(fsel->dir, fsel->current + fsel->offset);
    if (select_mode == SEL_DIR) {
      menu_hide();
      kbd_focus = select_focus;
      select_done_call(fsel);
      fs_free(fsel);
    } else {
      if (fsel->selectedfile->type == T_PRG || fsel->selectedfile->type == T_SEQ) {
	menu_hide();
	kbd_focus = select_focus;
	select_done_call(fsel);
	fs_free(fsel);
      }
    }
    break;

  case SDLK_DOWN:
    if (fsel->current + fsel->offset + 1 < fsel->numentries) {
      if (fsel->current < fsel->filesperpage - 1) {
	fs_draw_name(fsel, fsel->current + fsel->offset, fsel->current, 0);
	fsel->current += 1;
	fs_draw_name(fsel, fsel->current + fsel->offset, fsel->current, 1);
      } else {
	fsel->current = 0;
	fsel->offset += fsel->filesperpage;
	fs_draw(fsel);
      }
    }
    break;

  case SDLK_UP:
    if (fsel->current + fsel->offset > 0) {
      if (fsel->current == 0) {
	fsel->current = fsel->filesperpage - 1;
	fsel->offset -= fsel->filesperpage;
	fs_draw(fsel);
      } else {
	fs_draw_name(fsel, fsel->current + fsel->offset, fsel->current, 0);
	fsel->current -= 1;
	fs_draw_name(fsel, fsel->current + fsel->offset, fsel->current, 1);
      }
    }
    break;

  default:
    break;
  }
}


void select_set_xferdir(FileSelector *fs) {
  strcpy(cfg_xferdir, fs->path);
}


void ui_bkgcolkey(SDL_keysym *keysym) {
  switch (keysym->sym) {

  case SDLK_ESCAPE:
    menu_hide();
    kbd_focus = FOCUS_TERM;
    return;
    break;

  case SDLK_UP:
    ui_bkgcol = (ui_bkgcol + 1) & 15;
    break;

  case SDLK_DOWN:
    ui_bkgcol = (ui_bkgcol - 1) & 15;
    break;

  default:
    if (keysym->sym >= SDLK_0 && keysym->sym <= SDLK_9) {
      ui_bkgcol = keysym->sym - SDLK_0;
    } else if (keysym->sym >= SDLK_a && keysym->sym <= SDLK_f) {
      ui_bkgcol = keysym->sym - SDLK_a + 10;
    }
    break;

  }

  gfx_bgcolor(ui_bkgcol);
}


void ui_directionkey(SDL_keysym *keysym) {
  switch (keysym->sym) {

  case SDLK_ESCAPE:
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  case SDLK_RETURN:
    gfx_cursdirection = 0;
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  case SDLK_RIGHT:
    gfx_cursdirection = 1;
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  case SDLK_DOWN:
    gfx_cursdirection = 2;
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  case SDLK_LEFT:
    gfx_cursdirection = 3;
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  case SDLK_UP:
    gfx_cursdirection = 4;
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  default:
    break;
  }
}


#define SWAP(X, Y) {int temp; temp = (X); X = (Y); Y = temp;}


void ui_rectanglekey(SDL_keysym *keysym) {
  switch (keysym->sym) {

  case SDLK_ESCAPE:
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  case SDLK_s:
    rect_x = gfx_cursx;
    rect_y = gfx_cursy;
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  case SDLK_c:
  case SDLK_x:
    if (gfx_cursx < rect_x) {
      SWAP(gfx_cursx, rect_x);
    }
    if (gfx_cursy < rect_y) {
      SWAP(gfx_cursy, rect_y);
    }
    rect_w = gfx_cursx - rect_x + 1;
    rect_h = gfx_cursy - rect_y + 1;
    gfx_copy_rect(rect_x, rect_y, rect_w, rect_h, rect_0400, rect_d800);
    if (keysym->sym == SDLK_x) {
      gfx_clear_rect(rect_x, rect_y, rect_w, rect_h);
    }
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  case SDLK_v:
    if (rect_w) {
      gfx_paste_rect(gfx_cursx, gfx_cursy, rect_w, rect_h, rect_0400, rect_d800);
    }
    menu_hide();
    kbd_focus = FOCUS_TERM;
    break;

  default:
    break;
  }
}


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
    menu_draw_message("Change with cursor keys or 0-f");
    menu_show();
    kbd_focus = FOCUS_BKGCOL;
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
    menu_draw_message("Select direction");
    menu_show();
    kbd_focus = FOCUS_DIRECTION;
    break;

  case SDLK_f:
    gfx_toggle_fullscreen();
    break;

  case SDLK_i:
    kbd_select_dir(&select_set_xferdir, FOCUS_TERM);
    break;

  case SDLK_l:
    ui_inputcall(30, "Load SEQ file:", "screen.seq", &kbd_loadseq, FOCUS_TERM);
    break;

  case SDLK_m:
    cfg_editmode ^= 1;
    break;

  case SDLK_q:
    exit(0);
    break;

  case SDLK_r:
    menu_draw_rectangle();
    menu_show();
    kbd_focus = FOCUS_RECTANGLE;
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
  menu_print_menu(editmenu);
  menu_show();
  kbd_focus = FOCUS_MENU;
}


void ui_local_init(void) {
  kbd_add_focus(FOCUS_SELECTDIR, &ui_selectdirkey);
  kbd_add_focus(FOCUS_BKGCOL, &ui_bkgcolkey);
  kbd_add_focus(FOCUS_DIRECTION, &ui_directionkey);
  kbd_add_focus(FOCUS_RECTANGLE, &ui_rectanglekey);
}
