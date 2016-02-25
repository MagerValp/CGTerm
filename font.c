#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <SDL.h>
#include "font.h"


static SDL_Surface *font_draw_surface;
static Font *font_current;


int font_init(SDL_Surface *surface) {
  font_draw_surface = surface;
  font_current = NULL;
  return(0);
}


Font *font_load_font(char *filename, int charw, int charh, int fontw, int fonth) {
  SDL_Surface *tempsurface, *fontsurface;
  Font *font;
  int numchars;
  int x, y, c;
  SDL_Rect src, dest;

  numchars = fontw * fonth;

  if ((fontsurface = SDL_LoadBMP(filename)) == NULL) {
    printf("Unable to load %s: %s\n", filename, SDL_GetError());
    return(NULL);
  }
  if (fontsurface->format->BytesPerPixel != 1) {
    printf("%s can't be used as a font\n", filename);
    SDL_FreeSurface(fontsurface);
    return(NULL);
  }

  if (charw > fontsurface->w / fontw || charh > fontsurface->h / fonth) {
    printf("Font size mismatch\n");
    SDL_FreeSurface(fontsurface);
    return(NULL);
  }
  dest.w = src.w = charw;
  dest.h = src.h = charh;

  if ((tempsurface = SDL_CreateRGBSurface(0, numchars * charw, charh, 8, 0, 0, 0, 0)) == NULL) {
    printf("Surface allocation failed: %s\n", SDL_GetError());
    SDL_FreeSurface(fontsurface);
    return(NULL);
  }
  SDL_SetPalette(tempsurface, SDL_LOGPAL|SDL_PHYSPAL, fontsurface->format->palette->colors, 0, fontsurface->format->palette->ncolors);
  SDL_SetColorKey(tempsurface, SDL_SRCCOLORKEY|SDL_RLEACCEL, 0);

  c = 0;
  for (y = 0; y < fonth; ++y) {
    for (x = 0; x < fontw; ++x) {
      src.x = x * charw;
      src.y = y * charh;
      dest.x = c * charw;
      dest.y = 0;
      SDL_BlitSurface(fontsurface, &src, tempsurface, &dest);
      ++c;
    }
  }
  SDL_FreeSurface(fontsurface);

  if ((font = malloc(sizeof *font)) == NULL) {
    return(NULL);
  }
  /*
  font->surface = SDL_DisplayFormat(tempsurface);
  SDL_FreeSurface(tempsurface);
  if (font->surface == NULL) {
    free(font);
    return(NULL);
  }
  */
  font->surface = tempsurface;
  font->width = charw;
  font->height = charh;
  font->numchars = numchars;
  return(font);
}


void font_free(Font *font) {
  SDL_FreeSurface(font->surface);
  free(font);
}


Font *font_set_font(Font *font) {
  Font *oldfont;

  oldfont = font_current;
  font_current = font;
  return(oldfont);
}


SDL_Surface *font_set_draw_surface(SDL_Surface *surface) {
  SDL_Surface *oldsurface;

  oldsurface = font_draw_surface;
  font_draw_surface = surface;
  return(oldsurface);
}


void font_draw_string(int x, int y, char *text) {
  SDL_Rect src, dest;

  src.w = dest.w = font_current->width;
  src.h = dest.h = font_current->height;
  dest.x = x;
  dest.y = y;
  src.y = 0;
  while (*text) {
    src.x = (*text) * src.w;
    SDL_BlitSurface(font_current->surface, &src, font_draw_surface, &dest);
    ++text;
    dest.x += src.w;
  }
}
