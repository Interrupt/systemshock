
#include <Carbon/Carbon.h>
#include "musicai.h"
#include "adlmidi.h"
#include "mlimbs.h"
#include "Xmi.h"

#ifdef USE_SDL_MIXER

#include <SDL_mixer.h>

static Mix_Chunk *samples_by_channel[SND_MAX_SAMPLES];
static snd_digi_parms digi_parms_by_channel[SND_MAX_SAMPLES];
static Mix_Chunk *playing_audiolog_sample = NULL;

#define SAMPLE_RATE 44100
#define SAMPLES 4096
#define CHANNELS MLIMBS_MAX_CHANNELS

struct ADL_MIDIPlayer *adlDevice[MLIMBS_MAX_CHANNELS];

int snd_start_digital(void) {

	// Startup the sound system

	if(Mix_Init(MIX_INIT_MP3) < 0) {
		ERROR("%s: Init failed", __FUNCTION__);
	}

	if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024) < 0) {
		ERROR("%s: Couldn't open audio device", __FUNCTION__);
	}

	InitReadXMI();
	atexit(FreeXMI);
	atexit(ShutdownReadXMI);

	Mix_AllocateChannels(SND_MAX_SAMPLES);

	return OK;
}

int snd_sample_play(int snd_ref, int len, uchar *smp, struct snd_digi_parms *dprm) {

	// Play one of the VOC format sounds

	Mix_Chunk *sample = Mix_LoadWAV_RW(SDL_RWFromConstMem(smp, len), 1);
	if (sample == NULL) {
		DEBUG("%s: Failed to load sample", __FUNCTION__);
		return ERR_NOEFFECT;
	}

	int loops = dprm->loops > 0 ? dprm->loops - 1 : -1;
	int channel = Mix_PlayChannel(-1, sample, loops);
	if (channel < 0) {
		DEBUG("%s: Failed to play sample", __FUNCTION__);
		Mix_FreeChunk(sample);
		return ERR_NOEFFECT;
	}

	if (samples_by_channel[channel])
		Mix_FreeChunk(samples_by_channel[channel]);

	samples_by_channel[channel] = sample;
	digi_parms_by_channel[channel] = *dprm;
	snd_sample_reload_parms(&digi_parms_by_channel[channel]);

	return channel;
}

int snd_alog_play(int snd_ref, int len, Uint8 *smp, struct snd_digi_parms *dprm) {

	// Get rid of the last playing audiolog

	if(playing_audiolog_sample != NULL) {
		Mix_FreeChunk(playing_audiolog_sample);
		playing_audiolog_sample = NULL;
	}

	// Play one of the Audiolog sounds

	playing_audiolog_sample = Mix_QuickLoad_RAW(smp, len);
	if (playing_audiolog_sample == NULL) {
		DEBUG("%s: Failed to load sample", __FUNCTION__);
		return ERR_NOEFFECT;
	}

	int channel = Mix_PlayChannel(-1, playing_audiolog_sample, 0);
	if (channel < 0) {
		DEBUG("%s: Failed to play sample", __FUNCTION__);
		Mix_FreeChunk(playing_audiolog_sample);
		playing_audiolog_sample = NULL;
		return ERR_NOEFFECT;
	}

	digi_parms_by_channel[channel] = *dprm;
	Mix_Volume(channel, dprm->vol * 128 / 100);

	return channel;
}

void snd_end_sample(int hnd_id) {
	Mix_HaltChannel(hnd_id);
	if (samples_by_channel[hnd_id]) {
		Mix_FreeChunk(samples_by_channel[hnd_id]);
		samples_by_channel[hnd_id] = NULL;
	}
}

bool snd_sample_playing(int hnd_id) {
	return Mix_Playing(hnd_id);
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
	Mix_Volume(channel, (sdp->vol * 128) / 100);

	// sdp->pan ranges from 1 (left) to 127 (right)
	uint8_t right = 2 * sdp->pan;
	Mix_SetPanning(channel, 254 - right, right);
}

void MacTuneUpdateVolume(void) {
	extern uchar curr_vol_lev;
	float music_vol_mod = 0.8f;
	Mix_VolumeMusic(((curr_vol_lev * curr_vol_lev * 128 * music_vol_mod) / 10000) );
}

int is_playing = 0;

int MacTuneLoadTheme(char* theme_base, int themeID) {
	char filename[40];
	FILE *f;
	int i;
	
	#define NUM_SCORES                  8
	#define SUPERCHUNKS_PER_SCORE       4
	#define NUM_TRANSITIONS             9
	#define NUM_LAYERS                 32
	#define MAX_KEYS                   10
	#define NUM_LAYERABLE_SUPERCHUNKS  22
	#define KEY_BAR_RESOLUTION          2
	
	extern uchar track_table[NUM_SCORES][SUPERCHUNKS_PER_SCORE];
	extern uchar transition_table[NUM_TRANSITIONS];
	extern uchar layering_table[NUM_LAYERS][MAX_KEYS];
	extern uchar key_table[NUM_LAYERABLE_SUPERCHUNKS][KEY_BAR_RESOLUTION];
	
	for (i=0; i<NUM_THREADS; i++) StopTrack(i);
	FreeXMI();
	
	if (strncmp(theme_base, "thm", 3))
	{
	  // Try to use sblaster files, fall back to genmidi
	  sprintf(filename, "res/sound/sblaster/%s.xmi", theme_base);
	  int readFile = ReadXMI(filename);

	  if(!readFile) {
	  	sprintf(filename, "res/sound/genmidi/%s.xmi", theme_base);
	  	ReadXMI(filename);
	  }
	
	  StartTrack(0, 0, 127); //title music
	}
	else
	{
	  // Try to use sblaster files, fall back to genmidi
	  sprintf(filename, "res/sound/sblaster/thm%i.xmi", themeID);
	  int readFile = ReadXMI(filename);

	  if(!readFile) {
	  	sprintf(filename, "res/sound/genmidi/thm%i.xmi", themeID);
	  	ReadXMI(filename);
	  }
	
	  sprintf(filename, "res/sound/thm%i.bin", themeID);
	  f = fopen(filename, "rb");
	  if (f != 0)
	  {
	    fread(track_table,      NUM_SCORES * SUPERCHUNKS_PER_SCORE,             1, f);
	    fread(transition_table, NUM_TRANSITIONS,                                1, f);
	    fread(layering_table,   NUM_LAYERS * MAX_KEYS,                          1, f);
	    fread(key_table,        NUM_LAYERABLE_SUPERCHUNKS * KEY_BAR_RESOLUTION, 1, f);

	    fclose(f);
	  }
	}

	return OK;
}

void MacTuneKillCurrentTheme(void) {
	int i;
	for (i=0; i<NUM_THREADS; i++) StopTrack(i);
	Mix_HaltMusic();
}

#else

// Sound stubs that do nothing, when SDL Mixer is not found

int snd_start_digital(void) { return OK; }
int snd_sample_play(int snd_ref, int len, uchar *smp, struct snd_digi_parms *dprm) { return OK; }
int snd_alog_play(int snd_ref, int len, uchar *smp, struct snd_digi_parms *dprm) { return OK; }
void snd_end_sample(int hnd_id);
int MacTuneLoadTheme(char* theme_base, int themeID) { return OK; }
void MacTuneKillCurrentTheme(void) { }

#endif

// Unimplemented sound stubs

void snd_startup(void) { }
int snd_stop_digital(void) { return 1; }
