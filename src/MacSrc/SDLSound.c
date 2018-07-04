
#include <Carbon/Carbon.h>
#include "musicai.h"

#ifdef USE_SDL_MIXER

#include <SDL_mixer.h>

static Mix_Chunk *samples_by_channel[SND_MAX_SAMPLES];
static snd_digi_parms digi_parms_by_channel[SND_MAX_SAMPLES];

int snd_start_digital(void) {

	// Startup the sound system

	if(Mix_Init(MIX_INIT_MP3) < 0) {
		DebugString("SDL_Mixer: Init failed");
	}

	if(Mix_OpenAudio(44100, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
		DebugString("SDL_Mixer: Couldn't open audio device");	
	}

	Mix_AllocateChannels(SND_MAX_SAMPLES);

	return OK;
}

int snd_sample_play(int snd_ref, int len, uchar *smp, struct snd_digi_parms *dprm) {

	// Play one of the VOC format sounds

	Mix_Chunk *sample = Mix_LoadWAV_RW(SDL_RWFromConstMem(smp, len), 1);
	if (sample == NULL) {
		DebugString("SDL_Mixer: Failed to load sample");
		return ERR_NOEFFECT;
	}

	int channel = Mix_PlayChannel(-1, sample, 0);
	if (channel < 0) {
		DebugString("SDL_Mixer: Failed to play sample");
		Mix_FreeChunk(sample);
		return ERR_NOEFFECT;
	}

	if (samples_by_channel[channel])
		Mix_FreeChunk(samples_by_channel[channel]);

	samples_by_channel[channel] = sample;
	digi_parms_by_channel[channel] = *dprm;

	return channel;
}

void snd_end_sample(int hnd_id) {
	Mix_HaltChannel(hnd_id);
	if (samples_by_channel[hnd_id]) {
		Mix_FreeChunk(samples_by_channel[hnd_id]);
		samples_by_channel[hnd_id] = NULL;
	}
}

snd_digi_parms *snd_sample_parms(int hnd_id)
{
	return &digi_parms_by_channel[hnd_id];
}

void snd_kill_all_samples(void) {
	for (int channel = 0; channel < SND_MAX_SAMPLES; channel++) {
		snd_end_sample(channel);
	}
}

void snd_sample_reload_parms(snd_digi_parms *sdp) {
	// ignore if *sdp is not one of the items in digi_parms_by_channel[]
	if (sdp < digi_parms_by_channel || sdp > digi_parms_by_channel + SND_MAX_SAMPLES)
		return;
	int channel = sdp - digi_parms_by_channel;

	if (!Mix_Playing(channel))
		return;

	// sdp->vol ranges from 0..255
	Mix_Volume(channel, sdp->vol / 2);

	// sdp->pan ranges from 1 (left) to 127 (right)
	uint8_t right = 2 * sdp->pan;
	Mix_SetPanning(channel, 254 - right, right);
}

void MacTuneUpdateVolume(void) {
	extern uchar curr_vol_lev;
	Mix_VolumeMusic(curr_vol_lev * 128 / 100);
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
	Mix_PlayMusic(music, -1);
	MacTuneUpdateVolume();

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

void snd_startup(void) { }
int snd_stop_digital(void) { return 1; }
