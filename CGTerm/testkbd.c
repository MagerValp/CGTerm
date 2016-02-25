#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <SDL.h>


char keytable[SDLK_LAST];

char *keys[] = {
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
  "*left shift*",
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
  "*right shift*",
  "equal",
  "arrowup",
  "slash",
  "one",
  "arrowleft",
  "*control*",
  "two",
  "space",
  "*commodore*",
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


void usage(void) {
  printf("Usage: testkbd [-k keyboard.kbd]\n");
}


int main(int argc, char *argv[]) {
  char linebuf[256], keyname[16];
  int line = 0, value;
  FILE *in;
  SDL_Event event;
  int keysym = 0, key;
  int shift, ctrl, alt;
  char *keyboardcfg = "keyboard.kbd";
  int opt;

  while ((opt = getopt(argc, argv, "k:")) != -1) {
    switch (opt) {

    case 'k':
      keyboardcfg = optarg;
      break;

    case 'h':
    default:
      usage();
      return(1);

    }
  }
  argc -= optind;
  argv += optind;

  if (argc) {
    usage();
    return(1);
  }


  memset(keytable, 64, sizeof(keytable));

  if ((in = fopen(keyboardcfg, "r")) == NULL) {
    printf("Couldn't open %s\n", keyboardcfg);
    return(1);
  }
  printf("Reading keyboard config from %s\n", keyboardcfg);

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


  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_NOPARACHUTE) < 0) {
    printf("Unable to init SDL: %s\n", SDL_GetError());
    return(1);
  }
  atexit(SDL_Quit);
  if ((SDL_SetVideoMode(320, 200, 8, SDL_SWSURFACE)) == NULL) {
    printf("Unable to open window: %s\n", SDL_GetError());
    return(1);
  }
  SDL_WM_SetCaption("Keyboard Test - F12 to quit", "Keyboard Test");

  for (;;) {
    while (SDL_PollEvent(&event)) {
      switch (event.type) {

      case SDL_KEYDOWN:
	shift = event.key.keysym.mod & KMOD_SHIFT ? 1 : 0;
	ctrl = event.key.keysym.mod & KMOD_CTRL ? 1 : 0;
	alt = event.key.keysym.mod & KMOD_ALT ? 1 : 0;
	keysym = event.key.keysym.sym;
	switch (keysym) {
	case SDLK_F12:
	  return(0);
	case SDLK_LSHIFT:
	case SDLK_RSHIFT:
	case SDLK_LCTRL:
	case SDLK_RCTRL:
	case SDLK_LALT:
	case SDLK_RALT:
	case SDLK_CAPSLOCK:
	  break;
	default:
	  if ((key = keytable[keysym]) != 64) {
	    printf("keysym %d = %s (%d)\n", keysym, keys[key], key);
	  } else {
	    printf("keysym %d is unmapped\n", keysym);
	  }
	  break;
	}

	break;

      case SDL_QUIT:
	return(0);
	break;
      }
    }

    SDL_Delay(100);
  }

  return(0);
}
