
#include <Carbon/Carbon.h>
#include "musicai.h"

#ifdef USE_SDL_MIXER

#include <SDL_mixer.h>

// Hacky stubs to hook sound functions into SDL

int snd_start_digital(void) {

	// Startup the sound system

	if(Mix_Init(MIX_INIT_MP3) < 0) {
		DebugString("SDL_Mixer: Init failed");
	}

	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
		DebugString("SDL_Mixer: Couldn't open audio device");	
	}

	Mix_AllocateChannels(16);

	return OK;
}

int snd_sample_play(int snd_ref, int len, uchar *smp, struct snd_digi_parms *dprm) {

	// Play one of the VOC format sounds

	Mix_Chunk *sample = Mix_LoadWAV_RW(SDL_RWFromConstMem(smp, len), 0);
	Mix_PlayChannel(-1, sample, 0);

	return OK;
}

int MacTuneLoadTheme(char* theme_base, int themeID) {
	char filename[30];

	// Try to play some music! theme_base will be a string like 'thm0'

	// FIXME: This should really try to play the .xmi files in res/sound/genmidi if there is a way!
	// until then, I'm going to just attempt to play .mid files instead.

	// Build the file name
	strcpy(filename, "res/music/");
	strcat(filename, theme_base);
	strcat(filename, ".mid");

	printf("Playing music %s\n", filename);

	Mix_Music *music;
	music = Mix_LoadMUS(filename);
	Mix_VolumeMusic(128);
	Mix_PlayMusic(music, -1);

	return OK;
}

#else

// Sound stubs that do nothing, when SDL Mixer is not found

int snd_start_digital(void) { return OK; }
int snd_sample_play(int snd_ref, int len, uchar *smp, struct snd_digi_parms *dprm) { return OK; }
void snd_end_sample(int hnd_id);
int MacTuneLoadTheme(char* theme_base, int themeID) { return OK; }

#endif

// Unimplemented sound stubs

void snd_kill_all_samples(void) { }
void snd_sample_reload_parms(snd_digi_parms *sdp) { }
void snd_end_sample(int hnd_id) { }
void snd_startup(void) { }
int snd_stop_digital(void) { return 1; }

snd_digi_parms *snd_sample_parms(int hnd_id)
{
	snd_digi_parms parms;
	return &parms;
}