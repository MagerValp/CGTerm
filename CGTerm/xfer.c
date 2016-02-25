#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <SDL/SDL.h>
#include "net.h"
#include "timer.h"
#include "gfx.h"
#include "menu.h"
#include "xfer.h"
#include "xmodem.h"
#include "punter.h"
#include "diskimage.h"
#include "config.h"


char *xfer_tempdlname = "download.tmp";
char *xfer_tempulname = "upload.tmp";
char xfer_filename[256];

FILE *xfer_sendfile, *xfer_recvfile;

Direction xfer_direction;
Protocol xfer_protocol;
int xfer_cancel;
int xfer_saved_bytes;
int xfer_file_size;
unsigned char xfer_buffer[4096];
unsigned int xfer_starttime;
unsigned int xfer_last_kbd_check;

//int xfer_debug = 1;


void xfer_check_kbd(void) {
  SDL_Event event;

  if (SDL_PollEvent(&event)) {
    switch (event.type) {
    case SDL_QUIT:
      exit(1);
      break;
    case SDL_KEYDOWN:
      if (event.key.keysym.sym) {
      case SDLK_ESCAPE:
	xfer_cancel = 1;
	break;
      default:
	break;
      }
      break;
    }
  }
}


void xfer_send_byte(unsigned char c) {
  unsigned int t;

  t = timer_get_ticks();
  if (t > xfer_last_kbd_check + 20) {
    xfer_last_kbd_check = t;
    xfer_check_kbd();
  }
  /*
  if (xfer_debug) {
    t -= xfer_starttime;
    printf("xfer_send_byte[%3d.%03d]: %02x\n", t / 1000, t % 1000, c);
  }
  */
  net_send(c);
}


signed int xfer_recv_byte(int timeout) {
  signed int c;
  unsigned int starttime, t;

  starttime = timer_get_ticks();
  while ((c = net_receive()) == -1) {
    if (timer_get_ticks() > starttime + timeout) {
      t = timer_get_ticks() - xfer_starttime;
      printf("xfer_recv_byte[%3d.%03d]: timeout\n", t / 1000, t % 1000);
      return(-1);
    } else {
      timer_delay(1);
      if (timer_get_ticks() > xfer_last_kbd_check + 20) {
	xfer_check_kbd();
	xfer_last_kbd_check = timer_get_ticks();
      }
    }
  }

  /*
  if (xfer_debug) {
    t = timer_get_ticks() - xfer_starttime;
    printf("xfer_recv_byte[%3d.%03d]: %02x\n", t / 1000, t % 1000, c);
  }
  */
  return(c);
}


signed int xfer_recv_byte_error(int timeout, int errorcnt) {
  signed int c;

  while ((c = xfer_recv_byte(timeout)) == -1 && errorcnt) {
    --errorcnt;
  }
  return(c);
}


int xfer_save_data(unsigned char *data, int length) {
  int l;
  int written = 0;

  while (written < length) {
    if ((l = fwrite(data + written, 1, length - written, xfer_recvfile))) {
      written += l;
    } else {
      return(0);
    }
  }
  xfer_saved_bytes += written;
  return(written);
}


int xfer_load_data(unsigned char *data, int length) {
  int l;
  int read = 0;

  while (read < length) {
    if ((l = fread(data + read, 1, length - read, xfer_sendfile))) {
      read += l;
    } else {
      if (ferror(xfer_sendfile)) {
	return(0);
      } else {
	return(read);
      }
    }
  }
  return(read);
}


int xfer_recv(void) {
  int status = 0;

  xfer_filename[0] = 0;
  xfer_cancel = 0;
  xfer_starttime = timer_get_ticks();
  menu_draw_xfer_progress("No filename", xfer_direction, xfer_protocol);
  menu_show();
  gfx_vbl();

  if ((xfer_recvfile = fopen(xfer_tempdlname, "wb"))) {
    xfer_saved_bytes = 0;
    if (xfer_protocol == PROT_XMODEM) {
      status = xmodem_recv(0);
    } else if (xfer_protocol == PROT_XMODEMCRC) {
      status = xmodem_recv(1);
    } else if (xfer_protocol == PROT_XMODEM1K) {
      status = xmodem_recv(1);
    } else if (xfer_protocol == PROT_PUNTER) {
      status = punter_recv();
    }
    fclose(xfer_recvfile);
    return(status);
  } else {
    menu_draw_message("Couldn't open tempfile!");
    return(0);
  }
}


int xfer_copy_from_image(char *imgname, char *src, char *dest) {
  DiskImage *di;
  ImageFile *imgfile;
  unsigned char rawname[16];
  FILE *outh;
  char buffer[4096];
  int l;

  if ((di = di_load_image(imgname)) == NULL) {
    return(0);
  }

  di_rawname_from_name(rawname, src);

  if ((imgfile = di_open(di, rawname, T_PRG, "rb")) == NULL) {
    di_free_image(di);
    return(0);
  }

  if ((outh = fopen(dest, "wb")) == NULL) {
    di_close(imgfile);
    di_free_image(di);
  }

  while ((l = di_read(imgfile, buffer, 4096))) {
    if (fwrite(buffer, 1, l, outh) != l) {
      di_close(imgfile);
      di_free_image(di);
      fclose(outh);
      return(0);
    }
  }

  di_close(imgfile);
  di_free_image(di);
  fclose(outh);
  return(1);
}


void xfer_send(char *filename) {
  char name[256];
  char *p;
  int deletetmp = 0;

  xfer_cancel = 0;
  xfer_starttime = timer_get_ticks();
  menu_draw_xfer_progress(filename, xfer_direction, xfer_protocol);
  menu_show();
  gfx_vbl();

  if ((p = strrchr(cfg_xferdir, '.')) && strlen(p) == 4 && (p[1] == 'd' || p[1] == 'D') && isdigit(p[2]) && isdigit(p[3])) {
    if (xfer_copy_from_image(cfg_xferdir, filename, xfer_tempulname)) {
      strcpy(name, xfer_tempulname);
      deletetmp = 1;
    } else {
      menu_draw_message("Couldn't open file!");
      menu_show();
      gfx_vbl();
      return;
    }
  } else {
    strcpy(name, cfg_xferdir);
#ifdef WINDOWS
    strcat(name, "\\");
#else
    strcat(name, "/");
#endif
    strcat(name, filename);
  }

  if ((xfer_sendfile = fopen(name, "rb"))) {

    if (fseek(xfer_sendfile, 0, SEEK_END)) {
      fclose(xfer_sendfile);
      menu_draw_message("Couldn't read file size!");
      menu_show();
      gfx_vbl();
      return;
    }
    xfer_file_size = ftell(xfer_sendfile);
    fseek(xfer_sendfile, 0, SEEK_SET);

    if (xfer_protocol == PROT_XMODEM) {
      xmodem_send(0);
    } else if (xfer_protocol == PROT_XMODEMCRC) {
      xmodem_send(0);
    } else if (xfer_protocol == PROT_XMODEM1K) {
      xmodem_send(1);
    } else if (xfer_protocol == PROT_PUNTER) {
      menu_draw_message("Not implemented yet");
      gfx_vbl();
    }
    fclose(xfer_sendfile);
  } else {
    menu_draw_message("Couldn't open file!");
    menu_show();
    gfx_vbl();
  }

  if (deletetmp) {
    remove(name);
  }
}


void xfer_save_file_in_image(char *filename) {
  FILE *from;
  DiskImage *di;
  ImageFile *to;
  unsigned char buf[4096];
  int bytesleft, l;
  unsigned char rawname[16];
  
  if ((di = di_load_image(cfg_xferdir)) == NULL) {
    menu_draw_message("Couldn't open disk image");
    menu_show();
    gfx_vbl();
    return;
  }

  if ((from = fopen(xfer_tempdlname, "rb")) == NULL) {
    menu_draw_message("Couldn't open temp file!");
    menu_show();
    gfx_vbl();
    return;
  }

  di_rawname_from_name(rawname, filename);

  to = di_open(di, rawname, T_PRG, "wb");
  if (to == NULL) {
    fclose(from);
    di_free_image(di);
    menu_draw_message("Couldn't write file!");
    menu_show();
    gfx_vbl();
    return;
  }

  bytesleft = xfer_saved_bytes;
  while (bytesleft) {
    l = fread(buf, 1, 4096, from);
    if (ferror(from)) {
      menu_draw_message("Read error!");
      goto done;
    }
    if (di_write(to, buf, l) != l) {
      menu_draw_message("Write error!");
      goto done;
    }
    bytesleft -= l;
  }

  if (bytesleft != 0) {
    menu_draw_message("I/O error!");
    goto done;
  }

  remove(xfer_tempdlname);

  sprintf(buf, "Saved %-24s", filename);
  menu_draw_message(buf);

 done:
  menu_show();
  gfx_vbl();
  fclose(from);
  di_close(to);
  di_free_image(di);
}



void xfer_save_file_in_dir(char *filename) {
  FILE *from, *to;
  unsigned char buf[4096];
  int bytesleft, l;
  char name[256];

  if ((from = fopen(xfer_tempdlname, "rb")) == NULL) {
    menu_draw_message("Couldn't open temp file!");
    menu_show();
    gfx_vbl();
    return;
  }

  strcpy(name, cfg_xferdir);
#ifdef WINDOWS
  strcat(name, "\\");
#else
  strcat(name, "/");
#endif
  strcat(name, filename);
  if ((to = fopen(name, "wb")) == NULL) {
    fclose(from);
    menu_draw_message("Couldn't write file!");
    menu_show();
    gfx_vbl();
    return;
  }

  bytesleft = xfer_saved_bytes;
  while (bytesleft) {
    l = fread(buf, 1, 4096, from);
    if (ferror(from)) {
      menu_draw_message("Read error!");
      goto done;
    }
    bytesleft -= fwrite(buf, 1, l, to);
    if (ferror(to)) {
      menu_draw_message("Write error!");
      goto done;
    }
  }

  if (bytesleft != 0) {
    menu_draw_message("I/O error!");
    goto done;
  }

  remove(xfer_tempdlname);

  sprintf(buf, "Saved %-24s", filename);
  menu_draw_message(buf);

 done:
  menu_show();
  gfx_vbl();
  fclose(from);
  fclose(to);
}


void xfer_save_file(char *filename) {
  char *p;

  if ((p = strrchr(cfg_xferdir, '.'))) {
    if (strlen(p) == 4) {
      if (p[1] == 'd' || p[1] == 'D') {
	if (isdigit(p[2]) && isdigit(p[3])) {
	  xfer_save_file_in_image(filename);
	  return;
	}
      }
    }
  }
  xfer_save_file_in_dir(filename);
}
