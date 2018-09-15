
#include <Carbon/Carbon.h>
#include "musicai.h"
#include "adlmidi.h"
#include "mlimbs.h"
#include "Xmi.h"

#ifdef USE_SDL_MIXER

#include <SDL_mixer.h>

static Mix_Chunk *samples_by_channel[SND_MAX_SAMPLES];
static snd_digi_parms digi_parms_by_channel[SND_MAX_SAMPLES];

extern SDL_AudioStream *cutscene_audiostream;
extern struct ADL_MIDIPlayer *adlD;

extern void AudioStreamCallback(void *userdata, unsigned char *stream, int len);
extern void MusicCallback(void *userdata, Uint8 *stream, int len);

int snd_start_digital(void) {

	// Startup the sound system

    SDL_AudioSpec spec, obtained;
    spec.freq     = 48000;
    spec.format   = AUDIO_S16SYS;
    spec.channels = 2;
    spec.samples  = 2048;
    spec.callback = AudioStreamCallback;
    spec.userdata = (void *)&cutscene_audiostream;

    if (SDL_OpenAudio(&spec, &obtained)) {ERROR("Could not open SDL audio");}
    else {INFO("Opened Music Stream");}


    if (Mix_Init(MIX_INIT_MP3) < 0) {ERROR("%s: Init failed", __FUNCTION__);}

    if (Mix_OpenAudio(48000, AUDIO_S16SYS, 2, 2048) < 0)
      {ERROR("%s: Couldn't open audio device", __FUNCTION__);}

    Mix_AllocateChannels(SND_MAX_SAMPLES);

    Mix_HookMusic(MusicCallback, (void *)&adlD);


	InitReadXMI();


    atexit(Mix_CloseAudio);
    atexit(SDL_CloseAudio);

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

    //assume we want these too
    StopTheMusic();
    if (cutscene_audiostream != NULL) SDL_AudioStreamClear(cutscene_audiostream);
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

    int volume = (int)curr_vol_lev * 127 / 100; //convert from 0-100 to 0-127
    ForceMusicVolume(volume);
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
	
    StopTheMusic();

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
    StopTheMusic();
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
