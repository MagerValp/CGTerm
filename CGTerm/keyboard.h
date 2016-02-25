typedef enum focus {
  FOCUS_TERM = 0,
  FOCUS_MENU,
  FOCUS_INPUTCALL,
  FOCUS_XFER,
  FOCUS_REQUESTER,
  FOCUS_BOOKMARKS,
  FOCUS_SELECTDIR,
  FOCUS_BKGCOL,
  FOCUS_DIRECTION,
  FOCUS_RECTANGLE
} Focus;


extern Focus kbd_focus;


int kbd_init(char *keyboardcfg);
int kbd_getkey(unsigned char *shift, unsigned char *ctrl, unsigned char *cbm);
void kbd_add_focus(Focus focus, void (*handler)(SDL_keysym *));
void kbd_loadseq(char *filename);
void kbd_loadseq_abort(void);
