#include <stdio.h>
#include <stdlib.h>
#include "diskimage.h"
#include "dir.h"
#include "xfer.h"


unsigned char filename[16] = {
  'T',
  'E',
  'S',
  'T',
  0xa0,
  0xa0,
  0xa0,
  0xa0,
  0xa0,
  0xa0,
  0xa0,
  0xa0,
  0xa0,
  0xa0,
  0xa0,
  0xa0
};
unsigned char buffer[4096];


int main(int argc, char **argv) {
  DiskImage *di;
  Dir *dir;
  DirEntry *entry;
  char quotename[19];
  TrackSector ts;
  ImageFile *imgfile;
  int i, l;
  FILE *in;
  unsigned char *rawname;

  if (argc != 4) {
    puts("Usage: testimage diskimage.d64 infile newname");
    return(1);
  }

  rawname = di_name_to_rawname(argv[3]);
  for (i = 0; i < 16; ++i) {
    printf("%02x ", rawname[i]);
  }
  puts("");

  if ((di = di_load_image(argv[1])) == NULL) {
    printf("Couldn't open %s\n", argv[1]);
    return(1);
  }

  for (ts.track = 1; ts.track <= di_tracks(di->type); ++ts.track) {
    printf("%2d: %2d ", ts.track, di_track_blocks_free(di, ts.track));
    for (ts.sector = 0; ts.sector < di_sectors_per_track(di->type, ts.track); ++ts.sector) {
      printf("%d", is_ts_free(di, ts));
    }
    puts("");
  }

  puts("");

  if ((imgfile = di_open(di, rawname, T_PRG, "wb")) == NULL) {
    puts("di_open failed");
    goto end;
  }

  if ((in = fopen(argv[2], "rb")) == NULL) {
    puts("fopen failed");
    goto end;
  }

  i = 0;
  while ((l = fread(buffer, 1, 4096, in))) {
    di_write(imgfile, buffer, l);
    printf("wrote %d bytes\n", i += l);
  }

  fclose(in);
  di_close(imgfile);

  dir = dir_read(argv[1]);
  printf("0 \"%-16s\"\n", dir->title);
  entry = dir->firstentry;
  while (entry) {
    sprintf(quotename, "\"%s\"", entry->name);
    printf("%-4d %-18s %c%s%c\n", entry->size, quotename, entry->closed ? ' ' : '*', dir_type[entry->type], entry->locked ? '<' : ' ');
    entry = entry->next;
  }

  printf("%d blocks free\n", di->blocksfree);

  dir_free(dir);

 end:
  di_free_image(di);
  free(rawname);
  return(0);
}
