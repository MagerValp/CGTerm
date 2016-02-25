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
#include "crc.h"
#include "chat.h"


static unsigned char data_buffer[1024];
static int data_maxlen = 1023;
static int data_len;

unsigned int lastvbl = 0;


char *default_cgchat_cfg[] = {
#ifndef WINDOWS
  "keyboard = us.kbd",
#endif
  "#zoom = 2",
  "#fullscreen = no",
  "#columns = 40",
  "#reconnect = 0",
  "#host = ",
  "#port = ",
  "#sound = yes",
  "bookmark = 64CHAT Beta Server, spike.ling.gu.se, 3172",
  NULL
};


void usage(void) {
  puts("cgchat [-4|-8] [-f] [-k keyboard.kbd] [-s] [-z zoom] [host [port]]");
}


void print_banner(int columns) {
  if (columns == 40) {
    print("\x12\x1f            \x9a\xac\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x9a\xbb\x1f            ");
    print("\x12\x1f            \x92                \x12            ");
    print("\x12\x1f            \x92\x9e   cg\x96" "cHAT \x05" "1.6   \x12\x1f            ");
    print("\x12\x1f            \x92                \x12            ");
    print("\x12\x1f            \x9a\xbc\x92\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x12\x9a\xbe\x1f            ");
    print("\x92\x05\x0d");
  } else {
    print("                    \x12\x1f            \x9a\xac\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x9a\xbb\x1f            \x0d");
    print("                    \x12\x1f            \x92                \x12            \x0d");
    print("                    \x12\x1f            \x92\x9e   cg\x96" "cHAT \x05" "1.6   \x12\x1f            \x0d");
    print("                    \x12\x1f            \x92                \x12            \x0d");
    print("                    \x12\x1f            \x9a\xbc\x92\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x12\x9a\xbe\x1f            \x0d");
    print("\x92\x05\x0d");
  }
}


int main(int argc, char *argv[]) {
  int c = 0;
  unsigned char k;
  int opt;
#ifndef WINDOWS
  char fname[1024];
#endif

  cfg_init(argv[0]);

#ifdef WINDOWS
  if (cfg_readconfig("cgchat.cfg") < 0) {
    return(1);
  }
  if (cfg_read == 0) {
    cfg_writeconfig(default_cgchat_cfg, "cgchat.cfg");
    if (cfg_readconfig("cgchat.cfg") < 0) {
      return(1);
    }
  }
#else
  strncpy(fname, cfg_homedir, 1000);
  strcat(fname, "/.cgchatrc");
  if (cfg_file_exists(fname)) {
    if (cfg_readconfig(fname) < 0) {
      return(1);
    }
  } else {
    strncpy(fname, cfg_prefix, 1000);
    strcat(fname, "/etc/cgchat.cfg");
    if (cfg_readconfig(fname) < 0) {
      return(1);
    }
  }
  if (cfg_read == 0) {
    strncpy(fname, cfg_homedir, 1000);
    strcat(fname, "/.cgchatrc");
    cfg_writeconfig(default_cgchat_cfg, fname);
  }
#endif

  while ((opt = getopt(argc, argv, "z:k:fs48")) != -1) {
    switch (opt) {

    case '4':
      cfg_columns = 40;
      break;

    case '8':
      cfg_columns = 80;
      break;

    case 'f':
      cfg_fullscreen = 1;
      break;

    case 'k':
      cfg_keyboard = optarg;
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

  if (crc_init()) {
    return(1);
  }
  if (gfx_init(cfg_fullscreen, "CGChat")) {
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

  print_banner(cfg_columns);

  if (argc == 0) {

    if (!cfg_host) {
      if (cfg_columns == 80) {
	print("                    ");
      }
      print("           \x96pRESS\x9e eSC\x96 FOR MENU\x05\x0d\x0d");
    }

  } else if (argc == 1 || argc == 2) {

    if (strchr(argv[0], '.') == NULL) {
      printf("Invalid hostname: %s\n", argv[0]);
      return(1);
    }

    cfg_host = argv[0];
    if (argc == 2) {
      cfg_port = (int)strtol(argv[1], (char **)NULL, 10);
    }

  } else {

    usage();
    return(1);

  }

  cfg_rows = 24;
  chat_init();

  if (cfg_host) {
    net_connect(cfg_host, cfg_port, &chat_print_net_status);
    cfg_nextreconnect = timer_get_ticks() + cfg_reconnect * 1000;
  }


  data_len = 0;
  data_buffer[0] = 0;

  for (;;) {

    if (timer_get_ticks() > lastvbl + 20) {
      if (timer_get_ticks() > lastvbl + 40) {
	lastvbl = timer_get_ticks();
      } else {
	lastvbl += 20;
      }
      gfx_vbl();
    }

    if (!net_connected() && cfg_host && cfg_reconnect && cfg_nextreconnect) {
      if (timer_get_ticks() > cfg_nextreconnect) {
	net_connect(cfg_host, cfg_port, &chat_print_net_status);
	cfg_nextreconnect = timer_get_ticks() + cfg_reconnect * 1000;
      }
    }

    k = ffe4();

    c = -1;
    if (net_connected()) {
      c = net_receive();
    }

    if (c == -2) {
      if (timer_get_ticks() < cfg_nextreconnect) {
	cfg_nextreconnect = timer_get_ticks() + cfg_reconnect * 1000;
      } else {
	cfg_nextreconnect = 0;
      }
    }

    if (k || c >= 0) {

      if (k) {
	chat_inputkey(k);
      }

      if (c >= 0) {
	if (data_len == data_maxlen) {
	  chat_print_msg("locl:lINEBUFFER FULL");
	  data_len = 0;
	}
	if (c == 13) {
	  if (data_len) {
	    chat_print_msg(data_buffer);
	    data_len = 0;
	  }
	} else {
	  data_buffer[data_len++] = c;
	  data_buffer[data_len] = 0;
	}
      }

    } else {

      timer_delay(1);

    }

  }

}
