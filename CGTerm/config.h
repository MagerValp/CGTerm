extern char *cfg_prefix;
extern char *cfg_homedir;

extern int cfg_read;

extern char *cfg_host;
extern unsigned int cfg_port;
extern char *cfg_keyboard;
extern int cfg_zoom;
extern char *cfg_logfile;
extern int cfg_fullscreen;
extern int cfg_localecho;
extern int cfg_senddelay;
extern int cfg_recvdelay;
extern int cfg_reconnect;
extern unsigned int cfg_nextreconnect;
extern int cfg_columns;
extern int cfg_rows;
extern int cfg_numbookmarks;
extern int cfg_sound;
extern char *cfg_bookmark_alias[];
extern char *cfg_bookmark_host[];
extern int cfg_bookmark_port[];
extern char cfg_xferdir[];
extern int cfg_editmode;


int cfg_init(char *argv0);
int cfg_file_exists(char *filename);
signed int cfg_readconfig(char *configfile);
void cfg_sethost(char *h);
int cfg_change_dir(char *dirbuffer, char *newdir);
void cfg_writeconfig(char **data, char *configfile);
