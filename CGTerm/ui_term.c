#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include "gfx.h"
#include "menu.h"
#include "config.h"
#include "keyboard.h"
#include "net.h"
#include "kernal.h"
#include "xfer.h"
#include "dir.h"
#include "diskimage.h"
#include "fileselector.h"
#include "macro.h"
#include "ui.h"


struct menu termmenu[] = {
    {1, "A", "Abort load or macro"},
    {2, "B", "Open bookmarks"},
    {3, "C", "Start/stop recording macro"},
    {4, "D", "Connect/disconnect"},
    {5, "E", "Toggle local echo"},
    {6, "F", "Toggle fullscreen mode"},
    {7, "I", "Set transfer disk image"},
    {8, "L", "Load seq file"},
    {9, "Q", "Quit CGTerm"},
    {10, "R", "Reconnect"},
    {11, "S", "Save screen to seq file"},
    {12, "T", "Transfer file"},
    {13, "V", "Play macro"},
    {14, "Alt", "Toggle case"},
    {0, NULL, NULL}
};


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


void kbd_select_file(void (*donecall)(FileSelector *), Focus focus) {
    select_done_call = donecall;
    select_focus = focus;
    select_mode = SEL_FILE;
    fsel = fs_new("Select file", cfg_xferdir);
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


void select_send_file(FileSelector *fs) {
    xfer_send(fs->selectedfile->name);
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
            
        case SDLK_e:
            cfg_localecho ^= 1;
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
            
        case SDLK_t:
            if (net_connected()) {
                xfer_direction = 0;
                xfer_protocol = 0;
                menu_draw_xfer();
                menu_show();
                kbd_focus = FOCUS_XFER;
            } else {
                menu_draw_message("Not connected.");
                menu_show();
                kbd_focus = FOCUS_REQUESTER;
            }
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


void ui_xferkey(SDL_keysym *keysym) {
    switch (keysym->sym) {
            
        case SDLK_ESCAPE:
            menu_hide();
            kbd_focus = FOCUS_TERM;
            break;
            
        case SDLK_1:
            xfer_protocol = PROT_XMODEM1K;
            menu_update_xfer(xfer_direction, xfer_protocol);
            break;
            
        case SDLK_c:
            xfer_protocol = PROT_XMODEMCRC;
            menu_update_xfer(xfer_direction, xfer_protocol);
            break;
            
        case SDLK_p:
            xfer_protocol = PROT_PUNTER;
            menu_update_xfer(xfer_direction, xfer_protocol);
            break;
            
        case SDLK_r:
            xfer_direction = DIR_RECV;
            menu_update_xfer(xfer_direction, xfer_protocol);
            break;
            
        case SDLK_s:
            xfer_direction = DIR_SEND;
            menu_update_xfer(xfer_direction, xfer_protocol);
            break;
            
        case SDLK_x:
            xfer_protocol = PROT_XMODEM;
            menu_update_xfer(xfer_direction, xfer_protocol);
            break;
            
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (xfer_protocol) {
                if (xfer_direction == DIR_SEND) {
                    kbd_select_file(&select_send_file, FOCUS_REQUESTER);
                } else if (xfer_direction == DIR_RECV) {
                    if (xfer_recv()) {
                        ui_inputcall(30, "Enter file name:", xfer_filename, &xfer_save_file, FOCUS_REQUESTER);
                    }
                }
            }
            break;
            
        default:
            break;
    }
}


void ui_menu(void) {
    menu_print_menu(termmenu);
    menu_show();
    kbd_focus = FOCUS_MENU;
}


void ui_local_init(void) {
    kbd_add_focus(FOCUS_XFER, &ui_xferkey);
    kbd_add_focus(FOCUS_SELECTDIR, &ui_selectdirkey);
}
