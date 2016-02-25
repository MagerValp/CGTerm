#define GFX_WIDTH 320
#define GFX_HEIGHT 200


extern unsigned char *gfx_0400;
extern unsigned char *gfx_d800;
extern signed int gfx_cursx, gfx_cursy;
extern int gfx_offset, gfx_maxoffset;
extern int gfx_cursdirection;


int gfx_init(int fullscreen, char *appname);
void gfx_setfont(int f);
void gfx_toggle_font(void);
void gfx_bgcolor(int c);
void gfx_fgcolor(int c);
void gfx_draw_char(int c);
void gfx_clear_line(int line, int color);
void gfx_copy_line(unsigned char *line, unsigned char *color, int y);
void gfx_cls(void);
void gfx_scrollup(void);
void gfx_vbl(void);
int gfx_getkey(unsigned char *shift, unsigned char *ctrl, unsigned char *alt);
void gfx_setcursxy(int x, int y);
#define gfx_setcursx(X) gfx_setcursxy((X), gfx_cursy)
void gfx_cursleft(void);
void gfx_cursright(void);
void gfx_cursup(void);
void gfx_cursdown(void);
void gfx_cursadvance(void);
void gfx_togglerev(void);
void gfx_updatecolor(void);
void gfx_delete(void);
void gfx_insert(void);
void gfx_conv_screen_to_pet(unsigned char *chars, unsigned char *colors, unsigned char *petsciibuf, int *lastcolor, int *reverse, int addcr, int width);
void gfx_savescreen(char *filename);
void gfx_toggle_fullscreen(void);
void gfx_set_offset(int offset);
void gfx_copy_rect(int rect_x, int rect_y, int rect_w, int rect_h, unsigned char *rect_0400, unsigned char *rect_d800);
void gfx_clear_rect(int rect_x, int rect_y, int rect_w, int rect_h);
void gfx_paste_rect(int rect_x, int rect_y, int rect_w, int rect_h, unsigned char *rect_0400, unsigned char *rect_d800);
