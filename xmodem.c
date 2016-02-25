#include <string.h>
#include <SDL.h>
#include "gfx.h"
#include "menu.h"
#include "xfer.h"
#include "crc.h"


#define XM_SOH 0x01
#define XM_STX 0x02
#define XM_EOT 0x04
#define XM_ACK 0x06
#define XM_NAK 0x15
#define XM_CAN 0x18
#define XM_PAD 0x1a
#define XM_C   0x43


void xmodem_fail(char *message) {
  //printf("xmodem_fail: %s\n", message);
  menu_update_xfer_progress(message, xfer_saved_bytes, 0);
  gfx_vbl();
  while (xfer_recv_byte(1000) >= 0);
  xfer_send_byte(XM_CAN);
  xfer_send_byte(XM_CAN);
}


void xmodem_retry(char *message) {
  //printf("xmodem_retry: %s\n", message);
  menu_update_xfer_progress(message, xfer_saved_bytes, 0);
  gfx_vbl();
  while (xfer_recv_byte(1000) >= 0);
  xfer_send_byte(XM_NAK);
}


int xmodem_checksum(int blocksize, int usecrc) {
  int i;
  unsigned short remotecrc;
  unsigned char checksum = 0;
  unsigned short crc = 0;

  if (blocksize == 128 && usecrc == 0) {
    for (i = 0; i < 128; ++i) {
      checksum += xfer_buffer[i];
    }
    //printf("xfer_checksum: %02x (%02x)\n\n", checksum, xfer_buffer[i]);
    if (checksum == xfer_buffer[i]) {
      return(1);
    } else {
      return(0);
    }
  } else if (blocksize == 128 && usecrc) {
    crc = crc16_calc(xfer_buffer, 128);
    remotecrc = (xfer_buffer[128]<<8) | xfer_buffer[129];
    //printf("xfer_crc16: %04x (%04x)\n\n", crc, remotecrc);
    if (crc == remotecrc) {
      return(1);
    } else {
      return(0);
    }
  } else if (blocksize == 1024 && usecrc) {
    crc = crc16_calc(xfer_buffer, 1024);
    remotecrc = (xfer_buffer[1024]<<8) | xfer_buffer[1025];
    //printf("xfer_crc16: %04x (%04x)\n\n", crc, remotecrc);
    if (crc == remotecrc) {
      return(1);
    } else {
      return(0);
    }
  } else {
    return(0);
  }
}


static unsigned char xmodem_buffer[1024];
static unsigned int xmodem_buffer_len = 0;

int xmodem_save_data(unsigned char *data, int length) {
  if (data) {
    if (xmodem_buffer_len) {
      // flush previous block
      if (xfer_save_data(xmodem_buffer, xmodem_buffer_len) != xmodem_buffer_len) {
	return(0); // ouch
      }
    }
    // buffer new block
    memcpy(xmodem_buffer, data, length);
    xmodem_buffer_len = length;
  } else { // strip padding and flush
    if (xmodem_buffer_len) {
      while (xmodem_buffer_len && xmodem_buffer[xmodem_buffer_len - 1] == XM_PAD) {
	--xmodem_buffer_len;
      }
      if (xmodem_buffer_len) {
	if (xfer_save_data(xmodem_buffer, xmodem_buffer_len) != xmodem_buffer_len) {
	  return(0); // ouch
	}
      }
    }
  }
  return(length);
}


int xmodem_recv(int usecrc) {
  int c;
  int blocknum, bufctr, errorcnt, blocksize;
  int nexthandshake;

  xmodem_buffer_len = 0;

  blocknum = 1;
  if (usecrc) {
    nexthandshake = XM_C;
  } else {
    nexthandshake = XM_NAK;
  }
  xfer_send_byte(nexthandshake);

  menu_update_xfer_progress("Starting...", xfer_saved_bytes, 0);
  gfx_vbl();

  for (;;) {

    if (xfer_cancel) {
      xmodem_fail("Cancelling...");
      return(0);
    }

    errorcnt = 0;
    while ((c = xfer_recv_byte(10000)) == -1 && errorcnt < 10) {
      //printf("Timeout...\n");
      xfer_send_byte(nexthandshake);
      ++errorcnt;
    }
    nexthandshake = XM_NAK;
    switch (c) {
    case -2:
      // connection closed
      xmodem_fail("Disconnected!");
      return(0);
      break;
    case -1:
      // timeout
      xmodem_fail("Timeout!");
      return(0);
      break;
    case XM_CAN:
      xmodem_fail("Remote cancel!");
      return(0);
      break;
    case XM_STX:
      //printf("xfer_recv_xmodem: receiving 1k block\n");
      blocksize = 1026;
      goto getblock;
    case XM_SOH:
      //printf("xfer_recv_xmodem: receiving 128 byte block\n");
      if (usecrc) {
	blocksize = 130;
      } else {
	blocksize = 129;
      }
    getblock:
      // new block
      c = xfer_recv_byte_error(1000, 10);
      if (c != (blocknum & 255) && c != ((blocknum - 1) & 255)) {
	//printf("xfer_recv_xmodem: got block %d, expected %d or %d\n", c, blocknum & 255, (blocknum - 1) & 255);
	xmodem_retry("Wrong block, retrying");
	return(0);
      }
      if (255 - xfer_recv_byte_error(1000, 10) != c) {
	xmodem_retry("Block mismatch, retrying");
      }
      if (c == ((blocknum - 1) & 255)) {
	--blocknum;
      }
      //printf("xfer_recv_xmodem: receiving block %d, %d bytes\n", blocknum, blocksize);
      //xfer_debug = 0;
      for (bufctr = 0; bufctr < blocksize && c >= 0; ++bufctr) {
	c = xfer_recv_byte_error(1000, 5);
	xfer_buffer[bufctr] = c;
	//printf("xfer_recv_xmodem: received byte %d\n", bufctr);
      }
      //xfer_debug = 1;
      if (c < 0) {
	//printf("bufctr = %d, c = %d\n", bufctr, c);
	// lost sync
	xmodem_retry("Lost sync, retrying...");
      } else {
	if (xmodem_checksum(blocksize & 0xfff0, usecrc)) {
	  if (blocknum == 0) {
	    //printf("not saving block 0\n");
	  } else {
	    if (xmodem_save_data(xfer_buffer, blocksize & 0xfff0)) {
	      menu_update_xfer_progress("Downloading...", xfer_saved_bytes, 0);
	      gfx_vbl();
	    } else {
	      xmodem_fail("Write error!");
	    }
	  }
	  xfer_send_byte(XM_ACK);
	  nexthandshake = XM_ACK;
	  ++blocknum;
	  //puts("");
	} else {
	  // checksum failed
	  xmodem_retry("Checksum failed, retrying...");
	}
      }
      break;
    case XM_EOT:
      // end of transmission
      xmodem_save_data(NULL, 0);
      xfer_send_byte(XM_ACK);
      menu_update_xfer_progress("Transfer complete", xfer_saved_bytes, 0);
      gfx_vbl();
      return(1);
      break;
    default:
      // purge and cancel
      xmodem_fail("Wtf!?");
      return(0);
      break;
    }

  }
}


int xmodem_load_block(int blocksize) {
  int l;

  l = xfer_load_data(xfer_buffer, blocksize);
  if (l == 0) {
    return(0);
  }
  if (l < blocksize) {
    memset(xfer_buffer + l, XM_PAD, blocksize - l);
  }
  return(blocksize);
}


void xmodem_send_block(unsigned char blocknum, int blocksize, int usecrc) {
  int i;
  unsigned char cksum;
  unsigned short crc;
  //int debug;

  //printf("xmodem_send_block: sending block %d, %d bytes\n", blocknum, blocksize);

  //puts("xmodem_send_block: sending block header");
  xfer_send_byte(blocksize == 128 ? XM_SOH : XM_STX);
  xfer_send_byte(blocknum & 0xff);
  xfer_send_byte(0xff ^ (blocknum & 0xff));

  //puts("xmodem_send_block: sending data");
  //debug = xfer_debug;
  //xfer_debug = 0;
  for (i = 0; i < blocksize; ++i) {
    xfer_send_byte(xfer_buffer[i]);
  }
  //xfer_debug = debug;

  if (usecrc) {
    crc = crc16_calc(xfer_buffer, blocksize);
    //puts("xmodem_send_block: sending crc");
    xfer_send_byte(crc>>8);
    xfer_send_byte(crc & 0xff);
  } else {
    cksum = 0;
    for (i = 0; i < 128; ++i) {
      cksum += xfer_buffer[i];
    }
    //puts("xmodem_send_block: sending checksum");
    xfer_send_byte(cksum);
  }
  //puts("");
}


int xmodem_send(int send1k) {
  int c, usecrc, blocknum, bytesleft, sentbytes, blocksize;

  //puts("xmodem_send: waiting for start code");

  menu_update_xfer_progress("Starting...", 0, xfer_file_size);
  gfx_vbl();

  c = 0;
  while (c != XM_C && c != XM_NAK) {
    c = xfer_recv_byte(1000);
    if (xfer_cancel) {
      menu_update_xfer_progress("Canceled", 0, xfer_file_size);
      gfx_vbl();
      return(0);
    }
  }

  if (c == XM_C) {
    //puts("xmodem_send: using CRC");
    usecrc = 1;
  } else {
    //puts("xmodem_send: using checksum");
    usecrc = 0;
  }

  /*
  menu_update_xfer_progress("Sending header...", 0, xfer_file_size);
  gfx_vbl();
  memset(xfer_buffer, 'X', 128);
  xmodem_send_block(0, 128, usecrc);
  */

  bytesleft = xfer_file_size;
  sentbytes = 0;
  blocknum = 1;
  c = XM_NAK;

  menu_update_xfer_progress("Uploading...", 0, xfer_file_size);
  gfx_vbl();

  if (bytesleft > 896 && send1k && usecrc) {
    blocksize = 1024;
  } else {
    blocksize = 128;
  }
  if (xmodem_load_block(blocksize) == 0) {
    menu_update_xfer_progress("Read error", 0, xfer_file_size);
    gfx_vbl();
    return(0);
  }
  xmodem_send_block(blocknum, blocksize, usecrc);

  while (bytesleft > 0) {
    c = xfer_recv_byte(10000);
    switch (c) {

    case -2:
      xmodem_fail("Disconnected!");
      return(0);
      break;

    case XM_CAN:
      //puts("xmodem_send: cancel");
      menu_update_xfer_progress("Canceled by remote!", sentbytes, xfer_file_size);
      gfx_vbl();
      return(0);
      break;

    case XM_ACK:
      //puts("xmodem_send: ack");
      if (blocknum != 0) {
	sentbytes += blocksize;
	bytesleft -= blocksize;
      }
      ++blocknum;
      menu_update_xfer_progress("Uploading...", sentbytes, xfer_file_size);
      gfx_vbl();
      if (bytesleft > 896 && send1k && usecrc) {
	blocksize = 1024;
      } else {
	blocksize = 128;
      }
      if (bytesleft > 0) {
	if (xmodem_load_block(blocksize) == 0) {
	  menu_update_xfer_progress("Read error", 0, xfer_file_size);
	  gfx_vbl();
	  return(0);
	}
	xmodem_send_block(blocknum, blocksize, usecrc);
      }
      break;

    case XM_NAK:
      //puts("xmodem_send: nak");
      menu_update_xfer_progress("Resending...", sentbytes, xfer_file_size);
      gfx_vbl();
      xmodem_send_block(blocknum, blocksize, usecrc);
      break;

    default:
      break;
    }
  }

  //puts("xmodem_send: file sent");

  menu_update_xfer_progress("Finishing...", sentbytes, xfer_file_size);
  gfx_vbl();

  xfer_send_byte(XM_EOT);
  if (xfer_recv_byte(10000) != XM_ACK) {
    xfer_send_byte(XM_EOT);
  }

  menu_update_xfer_progress("Transfer complete", sentbytes, xfer_file_size);
  gfx_vbl();
  return(1);
}
