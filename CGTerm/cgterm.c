#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include "kernal.h"
#include "gfx.h"
#include "keyboard.h"
#include "net.h"
#include "config.h"
#include "timer.h"
#include "sound.h"
#include "crc.h"


int sendcrlf = 0;
unsigned int lastsend = 0;
unsigned int lastrecv = 0;
unsigned int lastvbl = 0;
FILE *logh;


char *default_cgterm_cfg[] = {
#ifndef WINDOWS
  "keyboard = us.kbd",
#endif
  "#zoom = 2",
  "#fullscreen = no",
  "#columns = 40",
  "#localecho = no",
  "#senddelay = 5",
  "#recvdelay = 0",
  "#reconnect = 0",
  "#logfile = ",
  "#host = ",
  "#port = ",
  "#sound = yes",
  "#xferdir = ",
  "bookmark = Negative Format BBS, 209.151.141.59",
  "bookmark = The Warpzone BBS, 207.225.28.214, 6400",
  "bookmark = The Darkside BBS, thedarkside.ath.cx, 4000",
  "bookmark = The Rats Den, ratsdenbbs.dyndns.org",
  "bookmark = The PetSCII Playground, 216.116.236.146, 64",
  "bookmark = The Hidden, the-hidden.hopto.org",
  "bookmark = ADDiXiON, addixion.hopto.org",
  "bookmark = The Last Stand BBS, laststandbbs.net",
  "bookmark = Riktronics BBS, rkbbs.net",
  NULL
};


void print_net_status(int code, char *message) {
  if (code == 2) {
    ffd2(0x96);
    print_ascii(message);
    ffd2(0x05);
    ffd2(0x0d);
  }
}


void log_close(void) {
  if (logh) {
    fclose(logh);
  }
}


void usage(void) {
  puts("cgterm [-4|-8] [-d delay] [-f] [-k keyboard.kbd] [-o logfile] [-r seconds]");
  puts("       [-s] [-z zoom] [host [port]]");
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
  if (cfg_readconfig("cgterm.cfg") < 0) {
    return(1);
  }
  if (cfg_read == 0) {
    cfg_writeconfig(default_cgterm_cfg, "cgterm.cfg");
    if (cfg_readconfig("cgterm.cfg") < 0) {
      return(1);
    }
  }
#else
  strncpy(fname, cfg_homedir, 1000);
  strcat(fname, "/.cgtermrc");
  if (cfg_file_exists(fname)) {
    if (cfg_readconfig(fname) < 0) {
      return(1);
    }
  } else {
    strncpy(fname, cfg_prefix, 1000);
    strcat(fname, "/etc/cgterm.cfg");
    if (cfg_readconfig(fname) < 0) {
      return(1);
    }
  }
  if (cfg_read == 0) {
    strncpy(fname, cfg_homedir, 1000);
    strcat(fname, "/.cgtermrc");
    cfg_writeconfig(default_cgterm_cfg, fname);
  }
#endif
  
  argc = 1;
  
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
      usage();
      return(1);

    default:
      break;
    }
  }
  argc -= optind;
  argv += optind;

  if (crc_init()) {
    return(1);
  }
  if (gfx_init(cfg_fullscreen, "CGTerm")) {
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
    print("\x12\x1f            \x92\x9e  cg\x96tERM \x05" "1.7B2  \x12\x1f            ");
    print("\x12\x1f            \x92                \x12            ");
    print("\x12\x1f            \x9a\xbc\x92\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x12\x9a\xbe\x1f            ");
    print("\x92\x05\x0d");
  } else {
    print("                    \x12\x1f            \x9a\xac\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x9a\xbb\x1f            \x0d");
    print("                    \x12\x1f            \x92                \x12            \x0d");
    print("                    \x12\x1f            \x92\x9e  cg\x96tERM \x05" "1.7B2  \x12\x1f            \x0d");
    print("                    \x12\x1f            \x92                \x12            \x0d");
    print("                    \x12\x1f            \x9a\xbc\x92\x9f\xa2\xa2\x99\xa2\xa2\x9e\xa2\xa2\x05\xa2\xa2\x9e\xa2\xa2\x99\xa2\xa2\x9f\xa2\xa2\x12\x9a\xbe\x1f            \x0d");
    print("\x92\x05\x0d");
  }
  gfx_vbl();

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

  if (cfg_host) {
    if (net_connect(cfg_host, cfg_port, &print_net_status)) {
      print("\x96" "cONNECT FAILED.\x05\x0d");
      gfx_vbl();
    }
    cfg_nextreconnect = timer_get_ticks() + cfg_reconnect * 1000;
  }


  if (cfg_logfile) {
    if ((logh = fopen(cfg_logfile, "w")) == NULL) {
      printf("Couldn't open %s for writing\n", cfg_logfile);
      return(1);
    }
  }
  atexit(log_close);


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
	if (net_connect(cfg_host, cfg_port, &print_net_status)) {
	  print("\x96" "rECONNECT FAILED.\x05\x0d");
	  gfx_vbl();
	}
	cfg_nextreconnect = timer_get_ticks() + cfg_reconnect * 1000;
      }
    }

    if (cfg_senddelay) {
      if (timer_get_ticks() > lastsend + cfg_senddelay) {
	k = ffe4();
      } else {
	k = 0;
      }
    } else {
      k = ffe4();
    }

    c = -1;
    if (net_connected()) {
      if (cfg_recvdelay) {
	if (timer_get_ticks() > lastrecv + cfg_recvdelay) {
	  lastrecv = timer_get_ticks();
	  c = net_receive();
	}
      } else {
	c = net_receive();
      }
    }

    if (c == -2) {
      print("\x0d\x96" "dISCONNECTED!\x05\x0d");
      if (timer_get_ticks() < cfg_nextreconnect) {
	cfg_nextreconnect = timer_get_ticks() + cfg_reconnect * 1000;
      } else {
	cfg_nextreconnect = 0;
      }
    }

    if (k || c >= 0) {

      if (k) {
	net_send(k);
	lastsend = timer_get_ticks();
	if (k == 13 && sendcrlf) {
	  net_send(10);
	}
	if (cfg_localecho) {
	  ffd2(k);
	  if (logh) {
	    if (fputc(k, logh) == EOF) {
	      printf("Error writing to %s\n", cfg_logfile);
	      fclose(logh);
	      logh = NULL;
	    }
	  }
	}
      }

      if (c >= 0) {
	ffd2(c);
	if (logh) {
	  if (fputc(c, logh) == EOF) {
	    printf("Error writing to %s\n", cfg_logfile);
	    fclose(logh);
	    logh = NULL;
	  }
	}
      }

    } else {

      timer_delay(1);

    }

  }

}
