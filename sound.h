extern int sound_bell;

int sound_init(void);
signed int sound_load_sample(char *filename);
void sound_free_sample(int sample);
void sound_play_sample(int sample);
