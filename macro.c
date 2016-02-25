#define MACROSIZE 4096


int macro_rec = 0;
int macro_len = 0;
int macro_maxlen = MACROSIZE;
int macro_play = 0;
int macro_ctr = 0;
unsigned char macrobuf_key[MACROSIZE];
unsigned char macrobuf_shift[MACROSIZE];
unsigned char macrobuf_ctrl[MACROSIZE];
unsigned char macrobuf_cbm[MACROSIZE];
