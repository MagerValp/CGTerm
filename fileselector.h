typedef struct fileselector {
  unsigned char *title;
  int numentries;
  int current;
  int offset;
  int filesperpage;
  Dir *dir;
  DirEntry *selectedfile;
  unsigned char path[256];
} FileSelector;


/* Initialize an empty file selector */
FileSelector *fs_new(char *title, char *path);

/* Deallocate FileSelector object */
void fs_free(FileSelector *fs);

/* Draw file selector */
void fs_draw_name(FileSelector *fs, int entry, int line, int selected);
void fs_draw(FileSelector *fs);

signed int fs_read_dir(FileSelector *fs, char *path);

/* Let user select a file */  
signed char fs_select(FileSelector *fs);
