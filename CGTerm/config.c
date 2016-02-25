#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include "config.h"


#define PREFIX "/usr/local"


#ifdef WINDOWS
#define DIRCHAR '\\'
#else
#define DIRCHAR '/'
#endif


int cfg_read = 0;

#ifdef WINDOWS
#else
char *cfg_prefix = PREFIX;
#endif
char *cfg_homedir = NULL;

char *cfg_host = NULL;
unsigned int cfg_port = 23;
#ifdef WINDOWS
char *cfg_keyboard = "windows.kbd";
#else
char *cfg_keyboard = PREFIX "/share/cgterm/us.kbd";
#endif
int cfg_zoom = 2;
char *cfg_logfile = NULL;
int cfg_fullscreen = 0;
int cfg_localecho = 0;
int cfg_senddelay = 0;
int cfg_recvdelay = 0;
int cfg_reconnect = 0;
unsigned int cfg_nextreconnect = 0;
int cfg_columns = 40;
int cfg_rows = 25;
int cfg_sound = 1;
int cfg_numbookmarks = 0;
char *cfg_bookmark_alias[10];
char *cfg_bookmark_host[10];
int cfg_bookmark_port[10];
char cfg_xferdir[256];
int cfg_editmode = 0;

char host[256];
char keyboard[256];
char logfile[256];
char prefix[256];

FILE *fh = NULL;


int cfg_init(char *argv0) {
#ifdef WINDOWS
  cfg_homedir = ".";
#else
  size_t l;

  if ((cfg_homedir = getenv("HOME")) == NULL) {
    printf("$HOME is not set, using current directory\n");
    cfg_homedir = ".";
  }
  if ((l = strlen(argv0)) > 22) {
    if (strncmp("/Contents/MacOS/CG", argv0 + l - 22, 18) == 0) {
      // we're running inside a macos .app
      strncpy(prefix, argv0, l - 13);
      prefix[l - 13] = 0;
      strcat(prefix, "/Resources");
      cfg_prefix = prefix;
      strcpy(keyboard, prefix);
      strcat(keyboard, "/share/cgterm/keyboard.kbd");
      cfg_keyboard = keyboard;
    }
  }
#endif
  getcwd(cfg_xferdir, 256);
  return(0);
}


void real_cfg_change_dir(char *dirbuffer, char *newdir) {
  size_t l, n;
  char *p;

  n = strlen(newdir);
  if (n == 0 || (n == 1 && strcmp(".", newdir) == 0)) {
    return;
  }
#ifdef WINDOWS
  if (newdir[n] == DIRCHAR) {
    newdir[n--] = 0;
  }
  if (newdir[1] == ':' && isalpha(newdir[0])) {
    strcpy(dirbuffer, newdir);
    return;
  }
#else
  if (newdir[0] == '/') {
    strcpy(dirbuffer, newdir);
    return;
  }
  if (newdir[n - 1] == DIRCHAR) {
    newdir[--n] = 0;
  }
#endif
  l = strlen(dirbuffer);
  if (l == 0 || (l == 1 && strcmp(".", dirbuffer) == 0)) {
    strcpy(dirbuffer, newdir);
    return;
  }
  if (n == 2 && strcmp("..", newdir) == 0) {
    if ((p = strrchr(dirbuffer, DIRCHAR))) {
      if (p != dirbuffer) {
#ifdef WINDOWS
	if (p == dirbuffer + 2 && dirbuffer[1] == ':') {
	  dirbuffer[3] = 0;
	  return;
	}
#endif
	*p = 0;
	return;
      } else {
	dirbuffer[1] = 0;
	return;
      }
    }
  }
  if (dirbuffer[l - 1] != DIRCHAR) {
    dirbuffer[l++] = DIRCHAR;
    dirbuffer[l] = 0;
  }
  strcat(dirbuffer, newdir);
}

int cfg_change_dir(char *dirbuffer, char *newdir) {
  real_cfg_change_dir(dirbuffer, newdir);
  return(1);
}



int cfg_file_exists(char *filename) {
  if ((fh = fopen(filename, "r")) == NULL) {
    return(0);
  } else {
    fclose(fh);
    return(1);
  }
}


void cfg_sethost(char *h) {
  strcpy(host, h);
  cfg_host = host;
}


void addhost(int num, char *alias, char *hostname, int port) {
  char *ptr;
  char *chr;

  //printf("adding %s (%s:%d)\n", alias, hostname, port);
  if ((ptr = malloc(strlen(hostname) + 1)) == NULL) {
    printf("Malloc failed, prepare to crash\n"); // :P
  }
  strcpy(ptr, hostname);
  if ((chr = strchr(ptr, ','))) {
    *chr = 0;
  }
  cfg_bookmark_host[num] = ptr;

  if ((ptr = malloc(strlen(alias) + 1)) == NULL) {
    printf("Malloc failed, prepare to crash\n"); // :P
  }
  strcpy(ptr, alias);
  if (strlen(alias) > 27) {
    ptr[27] = 0;
  }
  if ((chr = strchr(ptr, ','))) {
    *chr = 0;
  }
  cfg_bookmark_alias[num] = ptr;

  cfg_bookmark_port[num] = port;
}


int addbookmark(char *line) {
  char alias[256], hostname[256];
  int port;

  if (cfg_numbookmarks >= 10) {
    printf("Too many bookmarks\n");
    return(0);
  }

  port = 23;

  // scanf is a piece of shit!
  if (sscanf(line, "bookmark = %[^,] , %[a-zA-Z0-9.-] , %d \n", alias, hostname, &port) == 3) {
  } else if (sscanf(line, "bookmark = %[a-zA-Z0-9.-] , %d \n", hostname, &port) == 2) {
    strcpy(alias, hostname);
  } else if (sscanf(line, "bookmark = %[^,] , %[a-zA-Z0-9.-] \n", alias, hostname) == 2) {
  } else if (sscanf(line, "bookmark = %[a-zA-Z0-9.-] \n", hostname) == 1) {
    strcpy(alias, hostname);
  } else {
    return(0);
  }

  addhost(cfg_numbookmarks, alias, hostname, port);
  ++cfg_numbookmarks;
  return(1);
}


signed int cfg_readconfig(char *configfile) {
  FILE *cfg;
  char linebuf[256], key[16], value[256];
  int line = 0;

  if ((cfg = fopen(configfile, "r")) == NULL) {
    return(0);
  } else {
    cfg_read = 1;
  }

  while (fgets(linebuf, sizeof(linebuf), cfg) != NULL) {

    if (strlen(linebuf) >= sizeof(linebuf) - 1) {
      printf("Line %d in %s is too long\n", line, configfile);
      fclose(cfg);
      return(-1);
    }

    if (linebuf[0] == '#') {
    } else if (strlen(linebuf) >= 3) {

      if (sscanf(linebuf, "%15s = %255s \n", key, value) == 2) {

	if (strcmp(key, "host") == 0) {
	  if (strchr(value, '.')) {
	    cfg_sethost(value);
	  } else {
	    printf("Invalid hostname in %s: %s\n", configfile, value);
	    fclose(cfg);
	    return(-1);
	  }
	} else if (strcmp(key, "port") == 0) {
	  cfg_port = (unsigned int)strtol(value, (char **)NULL, 10);
	  if (cfg_port <= 0 || cfg_port > 65535) {
	    printf("Invalid port number in %s: %s\n", configfile, value);
	    fclose(cfg);
	    return(-1);
	  }
	} else if (strcmp(key, "senddelay") == 0) {
	  cfg_senddelay = (int)strtol(value, (char **)NULL, 10);
	  if (cfg_senddelay < 0 || cfg_senddelay > 10000) {
	    printf("Invalid send delay in %s: %s\n", configfile, value);
	    fclose(cfg);
	    return(-1);
	  }
	} else if (strcmp(key, "receivedelay") == 0) {
	  cfg_recvdelay = (int)strtol(value, (char **)NULL, 10);
	  if (cfg_recvdelay < 0 || cfg_recvdelay > 10000) {
	    printf("Invalid receive delay in %s: %s\n", configfile, value);
	    fclose(cfg);
	    return(-1);
	  }
	} else if (strcmp(key, "keyboard") == 0) {
#ifdef WINDOWS
	    strcpy(keyboard, value);
#else
	  if (value[0] == '/') {
	    strcpy(keyboard, value);
	  } else {
	    strncpy(keyboard, cfg_prefix, 200);
	    strcat(keyboard, "/share/cgterm/");
	    strncat(keyboard, value, 40);
	  }
#endif
	  cfg_keyboard = keyboard;
	} else if (strcmp(key, "logfile") == 0) {
	  strcpy(logfile, value);
	  cfg_logfile = logfile;
	} else if (strcmp(key, "xferdir") == 0) {
	  cfg_change_dir(cfg_xferdir, value);
	} else if (strcmp(key, "localecho") == 0) {
	  if (strcmp("yes", value) == 0) {
	    cfg_localecho = 1;
	  } else if (strcmp("no", value) == 0) {
	    cfg_localecho = 0;
	  } else {
	    printf("Invalid localecho value in %s: %s\n", configfile, value);
	    fclose(cfg);
	    return(-1);
	  }
	} else if (strcmp(key, "fullscreen") == 0) {
	  if (strcmp("yes", value) == 0) {
	    cfg_fullscreen = 1;
	  } else if (strcmp("no", value) == 0) {
	    cfg_fullscreen = 0;
	  } else {
	    printf("Invalid fullscreen value in %s: %s\n", configfile, value);
	    fclose(cfg);
	    return(-1);
	  }
	} else if (strcmp(key, "sound") == 0) {
	  if (strcmp("yes", value) == 0) {
	    cfg_sound = 1;
	  } else if (strcmp("no", value) == 0) {
	    cfg_sound = 0;
	  } else {
	    printf("Invalid sound value in %s: %s\n", configfile, value);
	    fclose(cfg);
	    return(-1);
	  }
	} else if (strcmp(key, "zoom") == 0) {
	  cfg_zoom = (int)strtol(value, (char **)NULL, 10);
	  if (cfg_zoom <= 0 || cfg_zoom > 8) {
	    printf("Invalid zoom value in %s: %s\n", configfile, value);
	    fclose(cfg);
	    return(-1);
	  }
	} else if (strcmp(key, "reconnect") == 0) {
	  cfg_reconnect = (int)strtol(value, (char **)NULL, 10);
	  if (cfg_reconnect <= 0 || cfg_reconnect > 10000) {
	    printf("Invalid reconnect delay in %s: %s\n", configfile, value);
	    fclose(cfg);
	    return(-1);
	  }
	} else if (strcmp(key, "columns") == 0) {
	  cfg_columns = (int)strtol(value, (char **)NULL, 10);
	  if (cfg_columns != 40 && cfg_columns != 80) {
	    printf("Invalid number of columns in %s: %s\n", configfile, value);
	    fclose(cfg);
	    return(-1);
	  }
	} else if (strcmp(key, "bookmark") == 0) {
	  if (addbookmark(linebuf) == 0) {
	    printf("Syntax error in %s line %d\n", configfile, line + 1);
	    fclose(cfg);
	    return(-1);
	  }
	} else {
	  printf("Unknown config key in %s line %d\n", configfile, line + 1);
	  fclose(cfg);
	  return(-1);
	}

      } else {
	printf("Error in %s line %d\n", configfile, line + 1);
	fclose(cfg);
	return(-1);
      }

    } else {

      if (!sscanf(linebuf, "%s", linebuf)) {
	printf("Syntax error in %s line %d\n", configfile, line + 1);
	fclose(cfg);
	return(-1);
      }

    }

    ++line;
  }

  if (ferror(cfg)) {
    printf("read error\n");
    fclose(cfg);
    return(-1);
  }


  fclose(cfg);
  return(0);
}


void cfg_writeconfig(char **data, char *configfile) {
  FILE *cfg;

  if ((cfg = fopen(configfile, "w")) == NULL) {
    return;
  }

  while (*data) {
    fprintf(cfg, "%s\n", *data++);
  }

  fclose(cfg);
}
