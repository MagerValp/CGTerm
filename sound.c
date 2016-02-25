#include <string.h>
#include <SDL.h>


typedef struct snd {
  int playing;
  int current_sample;
  int current_position;
} SoundControl;

#define MAXSAMPLES 10

int sound_bell;

int sound_initok = 0;
SDL_AudioSpec sound_audiospec;
int sound_numsamples;
int sound_length[MAXSAMPLES];
Uint8 *sound_buffer[MAXSAMPLES];
SoundControl sound_control;


void fillbuffer(void *userdata, Uint8 *stream, int len) {
  if (len) {
    if (sound_control.playing) {
      if (sound_control.current_position + len >= sound_length[sound_control.current_sample]) {
	memcpy(stream, sound_buffer[sound_control.current_sample] + sound_control.current_position,
	       sound_length[sound_control.current_sample] - sound_control.current_position);
	memset(stream + sound_length[sound_control.current_sample] - sound_control.current_position,
	       sound_audiospec.silence,
	       len - (sound_length[sound_control.current_sample] - sound_control.current_position));
	sound_control.playing = 0;
      } else {
	memcpy(stream, sound_buffer[sound_control.current_sample] + sound_control.current_position, len);
	sound_control.current_position += len;
      }
    } else {
      memset(stream, sound_audiospec.silence, len);
    }
  }
}


int sound_init(void) {
  sound_initok = 0;

  sound_numsamples = 0;
  memset(sound_buffer, 0, sizeof(sound_buffer));
  sound_control.playing = SDL_FALSE;

  if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    printf("Audio init failed: %s\n", SDL_GetError());
    return(1);
  }
  sound_audiospec.freq = 22050;
  sound_audiospec.format = AUDIO_S16;
  sound_audiospec.channels = 1;
  sound_audiospec.samples = 1024;
  sound_audiospec.callback = fillbuffer;
  sound_audiospec.userdata = NULL;
  if (SDL_OpenAudio(&sound_audiospec, NULL) < 0) {
    printf("Audio init failed: %s\n", SDL_GetError());
    return(1);
  }
  SDL_PauseAudio(0);
  sound_initok = 1;
  return(0);
}


signed int sound_load_sample(char *filename) {
  int sample;

  if (sound_initok) {
    if (sound_numsamples >= MAXSAMPLES) {
      return(-1);
    }
    for (sample = 0; sound_buffer[sample]; ++sample);
    if (SDL_LoadWAV(filename, &sound_audiospec, &sound_buffer[sample], &sound_length[sample]) == NULL) {
      return(-1);
    }
    return(sample);
  } else {
    return(0);
  }
}


void sound_free_sample(int sample) {
  if (sound_buffer[sample]) {
    SDL_FreeWAV(sound_buffer[sample]);
    sound_buffer[sample] = 0;
  }
}


void sound_play_sample(int sample) {
  if (sound_initok) {
    if (sound_buffer[sample] && !sound_control.playing) {
      SDL_LockAudio();
      sound_control.current_sample = sample;
      sound_control.current_position = 0;
      sound_control.playing = SDL_TRUE;
      SDL_UnlockAudio();
    }
  }
}
