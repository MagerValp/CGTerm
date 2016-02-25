typedef struct direntry {
    struct direntry *next;
    struct direntry *prev;
    char *name;
    unsigned char rawname[16];
    unsigned int size;
    int type;
    int closed;
    int locked;
    int track;
    int sector;
} DirEntry;

typedef struct dir {
    int numentries;
    char *title;
    DirEntry *firstentry;
} Dir;

extern char *dir_type[];


Dir *dir_read(char *path);
void dir_free(Dir *dir);
DirEntry *dir_find(Dir *dir, int entrynum);
