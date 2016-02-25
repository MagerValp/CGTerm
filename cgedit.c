#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL.h>
#include "kernal.h"
#include "gfx.h"
#include "keyboard.h"
#include "net.h"
#include "config.h"
#include "timer.h"
#include "sound.h"


static int lastvbl = 0;


char *default_cgedit_cfg[] = {
#ifndef WINDOWS
  "keyboard = us.kbd",
#endif
  "#zoom = 2",
  "#fullscreen = no",
  "#columns = 40",
  "#localecho = no",
  "#senddelay = 0",
  "#recvdelay = 0",
  "#reconnect = 0",
  "#logfile = ",
  "#host = ",
  "#port = ",
  "#sound = yes",
  "#xferdir = ",
  NULL
};


void usage(void) {
  puts("cgedit [-4|-8] [-f] [-k keyboard.kbd] [-s] [-z zoom]");
}


int main(int argc, char *argv[]) {
  unsigned char k;
  int opt;
  unsigned char *rowchar, *rowcolor;
#ifndef WINDOWS
  char fname[1024];
#endif

  cfg_init(argv[0]);

#ifdef WINDOWS
  if (cfg_readconfig("cgedit.cfg") < 0) {
    return(1);
  }
  if (cfg_read == 0) {
    cfg_writeconfig(default_cgedit_cfg, "cgedit.cfg");
    if (cfg_readconfig("cgedit.cfg") < 0) {
      return(1);
    }
  }
#else
  strncpy(fname, cfg_homedir, 1000);
  strcat(fname, "/.cgeditrc");
  if (cfg_file_exists(fname)) {
    if (cfg_readconfig(fname) < 0) {
      return(1);
    }
  } else {
    strncpy(fname, cfg_prefix, 1000);
    strcat(fname, "/etc/cgedit.cfg");
    if (cfg_readconfig(fname) < 0) {
      return(1);
    }
  }
  if (cfg_read == 0) {
    strncpy(fname, cfg_homedir, 1000);
    strcat(fname, "/.cgeditrc");
    cfg_writeconfig(default_cgedit_cfg, fname);
  }
#endif

  cfg_localecho = 1;

  while ((opt = getopt(argc, argv, "r:d:z:k:o:fs48")) != -1) {
    switch (opt) {

    case '4':
      cfg_columns = 40;
      break;

    case '8':
      cfg_columns = 80;
      break;

    case 'd':
      cfg_senddelay = strtol(optarg, (char **)NULL, 10);
      if (cfg_senddelay < 0 || cfg_senddelay > 10000) {
	usage();
	return(1);
      }
      break;

    case 'f':
      cfg_fullscreen = 1;
      break;

    case 'k':
      cfg_keyboard = optarg;
      break;

    case 'o':
      cfg_logfile = optarg;
      break;

    case 'r':
      cfg_reconnect = strtol(optarg, (char **)NULL, 10);
      if (cfg_reconnect < 1 || cfg_reconnect > 10000) {
	usage();
	return(1);
      }
      break;

    case 's':
      cfg_sound = 0;
      break;

    case 'z':
      if ((cfg_zoom = strtol(optarg, (char **)NULL, 10)) == 0) {
	usage();
	return(1);
      }
      break;

    case 'h':
    default:
      usage();
      return(1);

    }
  }
  argc -= optind;
  argv += optind;

  if (argc != 0) {
    usage();
    return(1);
  }

  if (gfx_init(cfg_fullscreen, "CGedit")) {
    return(1);
  }
  if (kbd_init(cfg_keyboard)) {
    return(1);
  }
  if (kernal_init()) {
    return(1);
  }
  if (cfg_sound && sound_init()) {
    printf("Sound init failed, sound disabled\n");
  } else {
#ifdef WINDOWS
    if ((sound_bell = sound_load_sample("bell.wav")) < 0) {
      printf("Couldn't load bell.wav\n");
      return(1);
    }
#else
    strncpy(fname, cfg_prefix, 1000);
    strcat(fname, "/share/cgterm/bell.wav");
    if ((sound_bell = sound_load_sample(fname)) < 0) {
      printf("Couldn't load %s\n", fname);
      return(1);
    }
#endif
  }

  if (cfg_columns == 40) {
    print("\x12\x1f            \x9a\xac\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x9a\xbb\x1f            ");
    print("\x12\x1f            \x92                \x12            ");
    print("\x12\x1f            \x92\x9e   cg\x96" "eDIT \x05" "1.6   \x12\x1f            ");
    print("\x12\x1f            \x92                \x12            ");
    print("\x12\x1f            \x9a\xbc\x92\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x12\x9a\xbe\x1f            ");
    print("\x92\x05\x0d");
  } else {
    print("                    \x12\x1f            \x9a\xac\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x9a\xbb\x1f            \x0d");
    print("                    \x12\x1f            \x92                \x12            \x0d");
    print("                    \x12\x1f            \x92\x9e   cg\x96" "eDIT \x05" "1.6   \x12\x1f            \x0d");
    print("                    \x12\x1f            \x92                \x12            \x0d");
    print("                    \x12\x1f            \x9a\xbc\x92\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x12\x9a\xbe\x1f            \x0d");
    print("\x92\x05\x0d");
    print("                    ");
  }
  print("           \x96pRESS\x9e eSC\x96 FOR MENU\x05\x0d\x0d");
  gfx_vbl();


  for (;;) {

    if (timer_get_ticks() > lastvbl + 20) {
      if (timer_get_ticks() > lastvbl + 40) {
	lastvbl = timer_get_ticks();
      } else {
	lastvbl += 20;
      }
      gfx_vbl();
    }

    if ((k = ffe4())) {
      if (cfg_editmode == 0) {
	ffd2(k);
      } else {
	rowchar = &gfx_0400[gfx_cursy * cfg_columns];
	rowcolor = &gfx_d800[gfx_cursy * cfg_columns];
	switch (k) {
	case ' ':
	  gfx_togglerev();
	  gfx_cursadvance();
	  break;
	case 5:
	  gfx_fgcolor(COLOR_WHITE);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 28:
	  gfx_fgcolor(COLOR_RED);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 30:
	  gfx_fgcolor(COLOR_GREEN);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 31:
	  gfx_fgcolor(COLOR_BLUE);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 129:
	  gfx_fgcolor(COLOR_ORANGE);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 144:
	  gfx_fgcolor(COLOR_BLACK);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 149:
	  gfx_fgcolor(COLOR_BROWN);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 150:
	  gfx_fgcolor(COLOR_LTRED);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 151:
	  gfx_fgcolor(COLOR_DKGRAY);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 152:
	  gfx_fgcolor(COLOR_GRAY);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 153:
	  gfx_fgcolor(COLOR_LTGREEN);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 154:
	  gfx_fgcolor(COLOR_LTBLUE);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 155:
	  gfx_fgcolor(COLOR_LTGRAY);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 156:
	  gfx_fgcolor(COLOR_PURPLE);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 158:
	  gfx_fgcolor(COLOR_YELLOW);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	case 159:
	  gfx_fgcolor(COLOR_CYAN);
	  gfx_updatecolor();
	  gfx_cursadvance();
	  break;
	default:
	  ffd2(k);
	  break;
	}
      }
    } else {
      timer_delay(1);
    }

  }

}
