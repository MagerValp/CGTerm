typedef struct font {
  SDL_Surface *surface;
  int width;
  int height;
  int numchars;
} Font;


int font_init(SDL_Surface *surface);
Font *font_load_font(char *filename, int charw, int charh, int fontw, int fonth);
void font_free(Font *font);
Font *font_set_font(Font *font);
SDL_Surface *font_set_draw_surface(SDL_Surface *surface);
void font_draw_string(int x, int y, char *text);
