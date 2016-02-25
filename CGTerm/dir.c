#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <ctype.h>
#include "diskimage.h"
#include "dir.h"


char *dir_type[] = {"DEL", "SEQ", "PRG", "USR", "REL", "CBM", "DIR"};


/* convert A0 padded petscii name to ascii */
char *make_name(unsigned char *rawname) {
  int l;
  char *name;

  for (l = 0; rawname[l] != 0xa0 && l < 16; ++l);
  if ((name = malloc(l+1)) == NULL) {
    return(NULL);
  }
  memcpy(name, rawname, l);
  name[l] = 0;
  for (l = 0; rawname[l]; ++l) {

  }
  return(name);
}


/* read directory */
Dir *dir_read_image(DiskImage *di) {
  unsigned char buffer[254];
  ImageFile *fh;
  Dir *dir;
  int offset;
  DirEntry *entry = NULL;

  if ((fh = di_open(di, "$", T_PRG, "rb")) == NULL) {
    return(NULL);
  }

  if ((dir = malloc(sizeof(*dir))) == NULL) {
    printf("couldn't allocate dir structure\n");
    goto ReadDirDone;
  }

  dir->numentries = 0;
  dir->title = NULL;
  dir->firstentry = NULL;

  if (di_read(fh, buffer, 254) != 254) {
    printf("BAM read failed\n");
    goto ReadDirDone;
  }

  dir->title = make_name(di_title(di));

  // add ..
  if ((dir->firstentry = malloc(sizeof(*(dir->firstentry)))) == NULL) {
    goto ReadDirDone;
  }
  entry = dir->firstentry;
  entry->prev = NULL;
  entry->next = NULL;
  if ((entry->name = malloc(3))) {
    strcpy(entry->name, "..");
  }
  memset(entry->rawname, 0xa0, 16);
  entry->rawname[0] = '.';
  entry->rawname[1] = '.';
  entry->type = T_DIR;
  entry->closed = 1;
  entry->locked = 0;
  entry->track = 0;
  entry->sector = 0;
  entry->size = 0;
  dir->numentries = 1;

  while (di_read(fh, buffer, 254) == 254) {
    for (offset = -2; offset < 254; offset += 32) {
      if (buffer[offset+2]) {
	//if (dir->numentries) {
	if ((entry->next = malloc(sizeof(*(entry->next)))) == NULL) {
	  goto ReadDirDone;
	}
	entry->next->prev = entry;
	entry = entry->next;
	  /*} else {
	  if ((dir->firstentry = malloc(sizeof(*(dir->firstentry)))) == NULL) {
	    goto ReadDirDone;
	  }
	  entry = dir->firstentry;
	  entry->prev = NULL;
	  }*/
	entry->next = NULL;
	entry->name = make_name(buffer + offset + 5);
	memcpy(entry->rawname, buffer + offset + 5, 16);
	entry->type = buffer[offset + 2] & 7;
	entry->closed = buffer[offset + 2] & 0x80;
	entry->locked = buffer[offset + 2] & 0x40;
	entry->track = buffer[offset + 3];
	entry->sector = buffer[offset + 4];
	entry->size = buffer[offset + 31]<<8 | buffer[offset + 30];
	++(dir->numentries);
      }
    }
  }

 ReadDirDone:
  di_close(fh);
  return(dir);
}


Dir *dir_read_opendir(DIR *dirhandle, char *path) {
  Dir *dir;
  DirEntry *entry = NULL;
  struct dirent *dirent;
  unsigned namelen;
  char *p;
#ifdef WINDOWS
  DIR *d;
  char namebuf[256];
  char *name;
  int len;

  /* Copy path to namebuf, add a slash, and remember where it ends */
  len = strlen(path);
  memcpy(namebuf, path, len);
  namebuf[len++] = '\\';
  name = namebuf + len;
#endif

  if ((dir = malloc(sizeof(*dir))) == NULL) {
    printf("couldn't allocate dir structure\n");
    return(NULL);
  }

  dir->numentries = 0;
  dir->title = NULL;
  dir->firstentry = NULL;

  if ((dir->title = malloc(strlen(path) + 1))) {
    strcpy(dir->title, path);
  }

/*
struct dirent {
	u_int32_t d_fileno;
	u_int16_t d_reclen;
	u_int8_t  d_type;
	u_int8_t  d_namlen;
	char	d_name[255 + 1];
}
*/

  while ((dirent = readdir(dirhandle))) {
    namelen = strlen(dirent->d_name);
    if ((namelen == 1) && (dirent->d_name[0] == '.')) {
      // skip .
    } else {
      if (dir->numentries) {
	if ((entry->next = malloc(sizeof(*(entry->next)))) == NULL) {
	  goto ReadDirDone;
	}
	entry->next->prev = entry;
	entry = entry->next;
      } else {
	if ((dir->firstentry = malloc(sizeof(*(dir->firstentry)))) == NULL) {
	  goto ReadDirDone;
	}
	entry = dir->firstentry;
	entry->prev = NULL;
      }
      entry->next = NULL;
      if ((entry->name = malloc(namelen + 1))) {
	memcpy(entry->name, dirent->d_name, namelen + 1);
      }
      memset(entry->rawname, 0xa0, sizeof(entry->rawname));
#ifdef WINDOWS
      memcpy(name, entry->name, namelen + 1);
      if ((d = opendir(name))) {
	entry->type = T_DIR;
	closedir(d);
      } else {
	entry->type = T_PRG;
      }
#else
      entry->type = dirent->d_type == DT_DIR ? T_DIR : T_PRG;
#endif
      if (entry->type == T_PRG) {
	if ((p = strrchr(entry->name, '.'))) {
	  if (strlen(p) == 4) {
	    if (p[1] == 'd' || p[1] == 'D') {
	      if (isdigit(p[2]) && isdigit(p[3])) {
		entry->type = T_DIR;
	      }
	    }
	  }
	}
      }
      entry->closed = 1;
      entry->locked = 0;
      entry->track = 0;
      entry->sector = 0;
      entry->size = 0;
      ++(dir->numentries);
    }
  }

 ReadDirDone:
  return(dir);
}




Dir *dir_read(char *path) {
  DIR *dirhandle;
  DiskImage *di;
  Dir *dir;

  if ((dirhandle = opendir(path))) {
    dir = dir_read_opendir(dirhandle, path);
    closedir(dirhandle);
  } else if ((di = di_load_image(path))) {
    dir = dir_read_image(di);
    di_free_image(di);
  } else {
    return(NULL);
  }
  return(dir);
}


/* free directory mem */
void dir_free(Dir *dir) {
  DirEntry *entry;
  DirEntry *next;

  entry = dir->firstentry;
  while (entry) {
    if (entry->name) {
      free(entry->name);
    }
    next = entry->next;
    free(entry);
    entry = next;
  }
  if (dir->title) {
    free(dir->title);
  }
  free(dir);
}


DirEntry *dir_find(Dir *dir, int entrynum) {
  DirEntry *de;
  de = dir->firstentry;
  while (entrynum--) {
    de = de->next;
  }
  return(de);
}
