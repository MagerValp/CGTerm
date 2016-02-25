#include <string.h>
#include <stdlib.h>
#include <SDL.h>
#include "kernal.h"
#include "gfx.h"
#include "config.h"
#include "timer.h"
#include "status.h"
#include "net.h"


#define BUFLEN 256

static unsigned char output_buffer[BUFLEN * 3 + 100];
static unsigned char input_buffer[BUFLEN];
static unsigned char color_buffer[BUFLEN];
static int input_maxlen = BUFLEN - 1;
static int input_len;
static int input_width;
static int input_offset;
static int input_pos;
static int input_color;
static int input_rvs;
static int input_startcolor;

static int outputline;


/*
HELO:protocol:message\r		connect greeting
TALK:username:message\r		public message
MESG:username:message\r		private message
STAT:message\r			status message
INFO:message\r			info message from server
LOCL:message\r			local client message
*/
void print_msg(unsigned char *msg) {
  unsigned char *p;
  unsigned char *data = msg + 5;

  if (strlen(msg) < 5) {
    print("\x96" "sERVER SENT GARBAGE!\x0d");
    return;
  }

  if (strncmp("helo:", msg, 5) == 0) {
    if (strncmp("64chat ", data, 7) != 0) {
      print("mALFORMED SERVER MESSAGE\x0d");
      return;
    }
    ffd2(5);
    print(data);
    ffd2(13);
  } else if (strncmp("talk:", msg, 5) == 0) {
    if ((p = strchr(data, ':')) == NULL) {
      print("\x96" "mALFORMED SERVER MESSAGE\x0d");
      return;
    }
    *p++ = 0;
    ffd2(158);
    print(data);
    ffd2(153);
    ffd2(' ');
    print(p);
    ffd2(13);
  } else if (strncmp("mesg:", msg, 5) == 0) {
    if ((p = strchr(data, ':')) == NULL) {
      print("\x96" "mALFORMED SERVER MESSAGE\x0d");
      return;
    }
    *p++ = 0;
    ffd2(156);
    print(data);
    ffd2(154);
    ffd2(' ');
    print(p);
    ffd2(13);
  } else if (strncmp("stat:", msg, 5) == 0) {
    ffd2(150);
    print(data);
    ffd2(13);
  } else if (strncmp("locl:", msg, 5) == 0) {
    ffd2(150);
    print(data);
    ffd2(13);
  } else if (strncmp("info:", msg, 5) == 0) {
    ffd2(155);
    print(data);
    ffd2(13);
  } else if (strncmp("noop:", msg, 5) == 0) {
    // do nothing
  } else {
    print("\x96" "uNKNOWN SERVER MESSAGE\x0d");
    print(msg);
    ffd2(13);
    return;
  }
}

void chat_print_msg(unsigned char *msg) {
  status_clear();
  gfx_setcursxy(0, outputline);
  print_msg(msg);
  outputline = gfx_cursy;
  gfx_setcursxy(input_pos, 24);
  gfx_fgcolor(input_color);
  status_draw();
}


void chat_print_net_status(int code, char *message) {
  char msg[80];
  int i = 0;
  char c;

  if (code == 2) {
    strcpy(msg, "locl:");
    while ((c = *message++) && i < 74) {
      if (c >= 'A' && c <= 'Z') {
	c += 32;
      } else if (c >= 'a' && c <= 'z') {
	c -= 32;
      }
      msg[i + 5] = c;
      ++i;
    }
    msg[i + 5] = 0;
    print_msg(msg);
  }
}


void parse_cmd(unsigned char *chars, unsigned char *colors, int length, unsigned char *outdata) {
  unsigned char *data = NULL, *p;
  char cmd[BUFLEN * 3];
  int color, rev;

  outdata[0] = 0;
  rev = 0;

  if (*chars != '/') {
    strcpy(outdata, "talk:");
    color = 256;
    gfx_conv_screen_to_pet(chars, colors, outdata + 5, &color, &rev, 0, input_len);
    if (strlen(outdata + 5) == 0) {
      outdata[0] = 0;
    }
    return;
  }

  color = *colors;
  gfx_conv_screen_to_pet(chars, colors, cmd, &color, &rev, 0, input_len);
  if ((data = strchr(cmd, ' '))) {
    *data++ = 0;
  }

  // /register username password
  if (strcasecmp("/REG", cmd) == 0) {
    if (data && (p = strchr(data, ' '))) {
      strcpy(outdata, "regi:");
      *p = ':';
      strcat(outdata, data);
    } else {
      print_msg("locl:uSAGE: /REG <USERNAME> <PASSWORD>");
      return;
    }

    // /login username password
  } else if (strcasecmp("/LOGIN", cmd) == 0) {
    if (data && (p = strchr(data, ' '))) {
      strcpy(outdata, "auth:");
      *p = ':';
      strcat(outdata, data);
    } else {
      print_msg("locl:uSAGE: /LOGIN <USERNAME> <PASSWORD>");
      return;
    }

    // /nick nickname
  } else if (strcasecmp("/NICK", cmd) == 0) {
    if (data) {
      strcpy(outdata, "nick:");
      strcat(outdata, data);
    } else {
      print_msg("locl:uSAGE: /NICK <NICKNAME>");
      return;
    }

    // /quit message
  } else if (strcasecmp("/QUIT", cmd) == 0) {
    strcpy(outdata, "quit:");
    if (data) {
      strcat(outdata, data);
    } else {
      strcat(outdata, "nO REASON");
    }
    net_send_string(outdata);
    net_send(13);
    timer_delay(100);
    net_disconnect();
    exit(0);

    // /disconnect message
  } else if (strcasecmp("/DISCONNECT", cmd) == 0) {
    strcpy(outdata, "quit:");
    if (data) {
      strcat(outdata, data);
    } else {
      strcat(outdata, "nO REASON");
    }
    cfg_nextreconnect = 0;

    // /msg username message
  } else if (strcasecmp("/MSG", cmd) == 0) {
    if (data && (p = strchr(data, ' '))) {
      strcpy(outdata, "mesg:");
      *p = ':';
      strcat(outdata, data);
    } else {
      print_msg("locl:uSAGE: /MSG <USERNAME> <MESSAGE>");
      return;
    }

    // /msg username message
  } else if (strcasecmp("/PASS", cmd) == 0) {
    if (data && (p = strchr(data, ' '))) {
      strcpy(outdata, "pass:");
      *p = ':';
      strcat(outdata, data);
    } else {
      print_msg("locl:uSAGE: /PASS <OLDPASS> <NEWPASS>");
      return;
    }

    // /who
  } else if (strcasecmp("/WHO", cmd) == 0) {
    strcpy(outdata, "whol");

    // /whois username
  } else if (strcasecmp("/WHOIS", cmd) == 0) {
    if (data) {
      strcpy(outdata, "whoi:");
      strcat(outdata, data);
    } else {
      print_msg("locl:uSAGE: /WHOIS <USERNAME>");
      return;
    }

    // /cls
  } else if (strcasecmp("/CLS", cmd) == 0) {
    gfx_cls();
    outputline = 0;
    status_draw();

    // /connect server port
  } else if (strcasecmp("/CONNECT", cmd) == 0) {
    if (net_connected()) {
      print_msg("locl:aLREADY CONNECTED");
      return;
    }
    if (data) {
      if ((p = strchr(data, ' '))) {
	*p++ = 0;
	cfg_port = strtol(p, (char **)NULL, 10);
	if (cfg_port <= 0 || cfg_port > 65535) {
	  print_msg("locl:iLLEGAL PORT NUMBER");
	  return;
	}
      } else {
	cfg_port = 23;
      }
      cfg_sethost(data);
      net_connect(cfg_host, cfg_port, &chat_print_net_status);
    } else {
      print_msg("locl:uSAGE: /CONNECT <server> [port]");
      return;
    }

    // unknown command
  } else {
    print_msg("locl:uNKNOWN COMMAND");
    return;
  }
}


void input_draw(void) {
  unsigned char convbuf[80];

  memset(convbuf, ' ', 80);
  memcpy(convbuf, input_buffer + input_offset, input_len - input_offset);
  gfx_copy_line(convbuf, color_buffer + input_offset, 24);
  gfx_setcursxy(input_pos, 24);
  gfx_fgcolor(input_color);
}


void chat_inputkey(unsigned char key) {
  int i;

  switch (key) {

  default:
    if ((key >= 32 && key <= 127) || (key >= 160)) {
      if (key == '/' && input_pos + input_len == 0) {
	input_color = COLOR_YELLOW;
      }
      if (input_len < input_maxlen) {
	++input_len;
	i = strlen(input_buffer + input_pos + input_offset) + 1;
	memmove(input_buffer + input_pos + input_offset + 1,
		input_buffer + input_pos + input_offset,
		i);
	memmove(color_buffer + input_pos + input_offset + 1,
		color_buffer + input_pos + input_offset,
		i);
	input_buffer[input_pos + input_offset] = screencode[key] + (input_rvs<<7);
	input_buffer[input_len] = 0;
	color_buffer[input_pos + input_offset] = input_color;
	color_buffer[input_len] = 0;

	if (input_pos + input_offset < input_len) {
	  if (input_pos < input_width - 1) {
	    ++input_pos;
	  } else if (input_len > input_offset + input_pos) {
	    ++input_offset;
	  }
	}
      }
    }
    break;

  case 13:
    if (input_len) {
      if (input_buffer[0] != '/') {
	input_startcolor = input_color;
      }
      parse_cmd(input_buffer, color_buffer, input_len, output_buffer);
      if (output_buffer[0]) {
	net_send_string(output_buffer);
	net_send(13);
      }
      input_len = input_offset = input_pos = input_rvs = 0;
      input_color = input_startcolor;
      input_buffer[0] = 0;
    }
    break;

  case 29: // right
    if (input_pos + input_offset < input_len) {
      if (input_pos < input_width - 1) {
	++input_pos;
      } else if (input_len > input_offset + input_pos) {
	++input_offset;
      }
    }
    break;

  case 157: // left
    if (input_pos > 0) {
      --input_pos;
    } else if (input_offset > 0) {
      --input_offset;
    }
    break;

  case 19: // home
    if (input_pos || input_offset) {
      input_pos = input_offset = 0;
    }
    break;

  case 147: // clear
    input_pos = input_offset = input_len = 0;
    input_color = input_startcolor;
    input_buffer[0] = 0;
    break;

  case 20: // delete
    if (input_pos || input_offset) {
      if (input_pos) {
	--input_pos;
      } else {
	--input_offset;
      }
      i = input_offset + input_pos;
      while (input_buffer[i]) {
	input_buffer[i] = input_buffer[i+1];
	color_buffer[i] = color_buffer[i+1];
	++i;
      }
      --input_len;
      if (input_len == 0) {
	input_color = input_startcolor;
      }
    }
    break;

  case 148: // insert
    if (input_pos + input_offset < input_len) {
      i = input_offset + input_pos;
      while (input_buffer[i]) {
	input_buffer[i] = input_buffer[i+1];
	color_buffer[i] = color_buffer[i+1];
	++i;
      }
      --input_len;
      if (input_len == 0) {
	input_color = input_startcolor;
      }
    }
    break;

  case 18: // rvs on
    input_rvs = 1;
    break;

  case 146: // rvs off
    input_rvs = 0;
    break;

    // all the colors
  case 5:
    input_color = COLOR_WHITE;
    break;
  case 28:
    input_color = COLOR_RED;
    break;
  case 30:
    input_color = COLOR_GREEN;
    break;
  case 31:
    input_color = COLOR_BLUE;
    break;
  case 129:
    input_color = COLOR_ORANGE;
    break;
    /* except black
  case 144:
    input_color = COLOR_BLACK;
    break;
    */
  case 149:
    input_color = COLOR_BROWN;
    break;
  case 150:
    input_color = COLOR_LTRED;
    break;
  case 151:
    input_color = COLOR_DKGRAY;
    break;
  case 152:
    input_color = COLOR_GRAY;
    break;
  case 153:
    input_color = COLOR_LTGREEN;
    break;
  case 154:
    input_color = COLOR_LTBLUE;
    break;
  case 155:
    input_color = COLOR_LTGRAY;
    break;
  case 156:
    input_color = COLOR_PURPLE;
    break;
  case 158:
    input_color = COLOR_YELLOW;
    break;
  case 159:
    input_color = COLOR_CYAN;
    break;

  }

  input_draw();
}


void chat_init(void) {
  outputline = gfx_cursy;

  input_width = cfg_columns;
  input_len = input_offset = input_pos = input_rvs = 0;
  input_color = input_startcolor = COLOR_LTGREEN;
  input_buffer[0] = 0;

  status_init();
  status_draw();
  input_draw();
}
