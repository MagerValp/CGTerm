#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL/SDL.h>
#include "diskimage.h"
#include "dir.h"
#include "fileselector.h"
#include "menu.h"


static int filesperpage = 13;


/* Initialize an empty file selector */
FileSelector *fs_new(char *title, char *path) {
    FileSelector *fs;
    
    if ((fs = malloc(sizeof(*fs))) == NULL) {
        return(NULL);
    }
    
    fs->title = title;
    fs->numentries = 0;
    fs->current = 0;
    fs->offset = 0;
    fs->filesperpage = filesperpage;
    fs->dir = NULL;
    strcpy(fs->path, path);
    
    fs->selectedfile = NULL;
    
    fs_read_dir(fs, fs->path);
    return(fs);
}


/* Deallocate FileSelector object */
void fs_free(FileSelector *fs) {
    if (fs->dir) {
        dir_free(fs->dir);
    }
    free(fs);
}


signed int fs_read_dir(FileSelector *fs, char *path) {
    fs->offset = 0;
    fs->current = 0;
    fs->selectedfile = NULL;
    if (fs->numentries) {
        dir_free(fs->dir);
        fs->numentries = 0;
    }
    if ((fs->dir = dir_read(path)) == NULL) {
        /* couldn't read device... */
        puts("fs_read_dir failed!");
        return(-1);
    }
    if (fs->dir->numentries) {
        fs->numentries = fs->dir->numentries;
    } else {
        free(fs->dir);
        fs->numentries = 0;
    }
    return(fs->numentries);
}


void fs_draw_name(FileSelector *fs, int entry, int line, int selected) {
    DirEntry *de;
    
    de = fs->dir->firstentry;
    while (entry--) {
        de = de->next;
    }
    menu_fs_draw_line(line, de->name, selected, de->type == T_DIR ? 1 : 0);
}


/* Draw file selector */
void fs_draw(FileSelector *fs) {
    DirEntry *de;
    int l;
    
    menu_fs_draw(fs->title);
    
    if (fs->numentries) {
        de = fs->dir->firstentry;
        l = fs->offset;
        while (l--) {
            de = de->next;
        }
        l = 0;
        while (de && l < fs->filesperpage) {
            menu_fs_draw_line(l, de->name, l == fs->current ? 1 : 0, de->type == T_DIR ? 1 : 0);
            de = de->next;
            ++l;
        }
    }
}
